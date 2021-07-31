/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
#include "loop.h"
#include "cg.h"
#include "optimize_common.h"

namespace maplebe {
#define LOOP_ANALYSIS_DUMP_NEWPM CG_DEBUG_FUNC(f)

static void PrintLoopInfo(const LoopHierarchy &loop) {
  LogInfo::MapleLogger() << "header " << loop.GetHeader()->GetId();
  if (loop.otherLoopEntries.size()) {
    LogInfo::MapleLogger() << " multi-header ";
    for (auto en : loop.otherLoopEntries) {
      LogInfo::MapleLogger() << en->GetId() << " ";
    }
  }
  if (loop.GetOuterLoop() != nullptr) {
    LogInfo::MapleLogger() << " parent " << loop.GetOuterLoop()->GetHeader()->GetId();
  }
  LogInfo::MapleLogger() << " backedge ";
  for (auto *bb : loop.GetBackedge()) {
    LogInfo::MapleLogger() << bb->GetId() << " ";
  }
  LogInfo::MapleLogger() << "\n members ";
  for (auto *bb : loop.GetLoopMembers()) {
    LogInfo::MapleLogger() << bb->GetId() << " ";
  }
  if (!loop.GetInnerLoops().empty()) {
    LogInfo::MapleLogger() << "\n inner_loop_headers ";
    for (auto *inner : loop.GetInnerLoops()) {
      LogInfo::MapleLogger() << inner->GetHeader()->GetId() << " ";
    }
  }
  LogInfo::MapleLogger() << "\n";
}

static void PrintInner(const LoopHierarchy &loop, uint32 level) {
  for (auto *inner : loop.GetInnerLoops()) {
    LogInfo::MapleLogger() << "loop-level-" << level << "\n";
    PrintLoopInfo(*inner);
    PrintInner(*inner, level + 1);
  }
}

void LoopHierarchy::PrintLoops(const std::string &name) const {
  LogInfo::MapleLogger() << name << "\n";
  for (const LoopHierarchy *loop = this; loop != nullptr; loop = loop->next) {
    PrintLoopInfo(*loop);
  }
  for (const LoopHierarchy *loop = this; loop != nullptr; loop = loop->next) {
    PrintInner(*loop, 1);
  }
}

void CGFuncLoops::CheckOverlappingInnerLoops(const MapleVector<CGFuncLoops*> &innerLoops,
                                             const MapleVector<BB*> &loopMembers) const {
  for (auto iloop : innerLoops) {
    CHECK_FATAL(iloop->loopMembers.size() > 0, "Empty loop");
    for (auto bb: iloop->loopMembers) {
      if (find(loopMembers.begin(), loopMembers.end(), bb) != loopMembers.end()) {
        LogInfo::MapleLogger() << "Error: inconsistent loop member";
        CHECK_FATAL(0, "loop member overlap with inner loop");
      }
    }
    CheckOverlappingInnerLoops(iloop->innerLoops, loopMembers);
  }
}

void CGFuncLoops::CheckLoops() const {
  // Make sure backedge -> header relationship holds
  for (auto bEdge: backedge) {
    if (find(bEdge->GetSuccs().begin(), bEdge->GetSuccs().end(), header) == bEdge->GetSuccs().end()) {
      bool inOtherEntry = false;
      for (auto entry: multiEntries) {
        if (find(bEdge->GetSuccs().begin(), bEdge->GetSuccs().end(), entry) != bEdge->GetSuccs().end()) {
          inOtherEntry = true;
          break;
        }
      }
      if (inOtherEntry == false) {
        if (find(bEdge->GetEhSuccs().begin(), bEdge->GetEhSuccs().end(), header) == bEdge->GetEhSuccs().end()) {
          LogInfo::MapleLogger() << "Error: inconsistent loop backedge";
          CHECK_FATAL(0, "loop backedge does not go to loop header");
        }
      }
    }
    if (find(header->GetPreds().begin(), header->GetPreds().end(), bEdge) == header->GetPreds().end()) {
      bool inOtherEntry = false;
      for (auto entry: multiEntries) {
        if (find(entry->GetPreds().begin(), entry->GetPreds().end(), bEdge) == entry->GetPreds().end()) {
          inOtherEntry = true;
          break;
        }
      }
      if (inOtherEntry == false) {
        if (find(header->GetEhPreds().begin(), header->GetEhPreds().end(), bEdge) == header->GetEhPreds().end()) {
          LogInfo::MapleLogger() << "Error: inconsistent loop header";
          CHECK_FATAL(0, "loop header does not have a backedge");
        }
      }
    }
  }

  // Make sure containing loop members do not overlap
  CheckOverlappingInnerLoops(innerLoops, loopMembers);

  if (innerLoops.empty() == false) {
    for (auto lp : innerLoops) {
      lp->CheckLoops();
    }
  }
}

void CGFuncLoops::PrintLoops(const CGFuncLoops &funcLoop) const {
  LogInfo::MapleLogger() << "loop_level(" << funcLoop.loopLevel << ") ";
  LogInfo::MapleLogger() << "header " << funcLoop.GetHeader()->GetId() << " ";
  if (multiEntries.size()) {
    LogInfo::MapleLogger() << "other-header ";
    for (auto bb : multiEntries) {
      LogInfo::MapleLogger() << bb->GetId() << " ";
    }
  }
  if (funcLoop.GetOuterLoop() != nullptr) {
    LogInfo::MapleLogger() << "parent " << funcLoop.GetOuterLoop()->GetHeader()->GetId() << " ";
  }
  LogInfo::MapleLogger() << "backedge ";
  for (auto *bb : funcLoop.GetBackedge()) {
    LogInfo::MapleLogger() << bb->GetId() << " ";
  }
  LogInfo::MapleLogger() << "\n members ";
  for (auto *bb : funcLoop.GetLoopMembers()) {
    LogInfo::MapleLogger() << bb->GetId() << " ";
  }
  LogInfo::MapleLogger() << "\n";
  if (!funcLoop.GetInnerLoops().empty()) {
    LogInfo::MapleLogger() << " inner_loop_headers ";
    for (auto *inner : funcLoop.GetInnerLoops()) {
      LogInfo::MapleLogger() << inner->GetHeader()->GetId() << " ";
    }
    LogInfo::MapleLogger() << "\n";
    for (auto *inner : funcLoop.GetInnerLoops()) {
      PrintLoops(*inner);
    }
  }
}

bool LoopFinder::DetectLoopSub(BB *header, BB *back) {
  if (recurseVisited[header->GetId()]) {
    return false;
  }
  recurseVisited[header->GetId()] = true;
  for (auto succ : header->GetSuccs()) {
    if (succ == back) {
      return true;
    }
  }
  // not reachable yet, continue searching
  for (auto succ : header->GetSuccs()) {
    if (DetectLoopSub(succ, back)) {
      return true;
    }
  }
  for (auto ehSucc : header->GetEhSuccs()) {
    if (ehSucc == back) {
      return true;
    }
  }
  for (auto ehSucc : header->GetEhSuccs()) {
    if (DetectLoopSub(ehSucc, back)) {
      return true;
    }
  }
  return false;
}

void LoopFinder::Insert(BB *bb, BB *header, std::set<BB *> &extraHeader) {
  if (find(candidate.begin(), candidate.end(), bb) == candidate.end()) {
    recurseVisited.clear();
    recurseVisited.resize(cgFunc->NumBBs());
    if (DetectLoopSub(header, bb)) {
      candidate.push_back(bb);
      stack.push(bb);
    } else {
      // since loop member predecessor is not reachable from loop header,
      // it is another entry to the loop.
      // If a backedge is to some bb not the header, then it should be
      // considered a different loop.
      for (auto succ : bb->GetSuccs()) {
        if (find(candidate.begin(), candidate.end(), succ) != candidate.end()) {
          extraHeader.insert(succ);
          break;
        }
      }
      for (auto ehsucc : bb->GetEhSuccs()) {
        if (find(candidate.begin(), candidate.end(), ehsucc) != candidate.end()) {
          extraHeader.insert(ehsucc);
          break;
        }
      }
    }
  }
}

void LoopFinder::DetectLoop(BB *header, BB *back) {
  std::set<BB *> moreHeaders;
  candidate.push_back(header);
  candidate.push_back(back);
  stack.push(back);
  while (stack.empty() == false) {
    BB *bb = stack.top();
    stack.pop();
    if (bb == header) {
      if (find(candidate.begin(), candidate.end(), bb) == candidate.end()) {
        candidate.push_back(bb);
      }
    } else {
      for (auto ibb : bb->GetPreds()) {
        Insert(ibb, header, moreHeaders);
      }
    }
    if (bb != header) {
      for (auto ibb : bb->GetEhPreds()) {
        Insert(ibb, header, moreHeaders);
      }
    }
  }
  LoopHierarchy *simple_loop = memPool->New<LoopHierarchy>(*memPool);
  for (auto ibb : candidate) {
    simple_loop->InsertLoopMembers(*ibb);
  }
  candidate.clear();
  simple_loop->SetHeader(*header);
  simple_loop->InsertBackedge(*back);
  simple_loop->otherLoopEntries.insert(moreHeaders.begin(), moreHeaders.end());

  if (loops) {
    loops->SetPrev(simple_loop);
  }
  simple_loop->SetNext(loops);
  loops = simple_loop;
}

void LoopFinder::FindBackedge() {
  while (!dfsBBs.empty()) {
    bool childPushed = false;
    BB *bb = dfsBBs.top();
    dfsBBs.pop();
    CHECK_FATAL(bb != nullptr, "bb is null in LoopFinder::FindBackedge");
    visitedBBs[bb->GetId()] = true;
    if (bb->GetLevel() == 0) {
      bb->SetLevel(1);
    }
    std::stack<BB*> succs;
    /* Mimic more of the recursive DFS by reversing the order of the succs. */
    for (auto *succBB : bb->GetSuccs()) {
      succs.push(succBB);
    }
    PushBackedge(*bb, succs, childPushed);
    for (auto *ehSuccBB : bb->GetEhSuccs()) {
      succs.push(ehSuccBB);
    }
    PushBackedge(*bb, succs, childPushed);
    /* Remove duplicate bb that are visited from top of stack */
    if (!dfsBBs.empty()) {
      BB *nextBB = dfsBBs.top();
      while (nextBB != nullptr) {
        if (visitedBBs[nextBB->GetId()]) {
          dfsBBs.pop();
        } else {
          break;
        }
        if (!dfsBBs.empty()) {
          nextBB = dfsBBs.top();
        } else {
          break;
        }
      }
    }
    if (!childPushed && !dfsBBs.empty()) {
      /* reached the bottom of visited chain, reset level to the next visit bb */
      bb->SetLevel(0);
      BB *nextBB = dfsBBs.top();
      if (sortedBBs[nextBB->GetId()]) {
        nextBB = sortedBBs[nextBB->GetId()];
        /* All bb up to the top of stack bb's parent's child (its sibling) */
        while (1) {
          /* get parent bb */
          BB *parentBB = sortedBBs[bb->GetId()];
          if ((parentBB == nullptr) || (parentBB == nextBB)) {
            break;
          }
          parentBB->SetLevel(0);
          bb = parentBB;
        }
      }
    }
  }
}

void LoopFinder::PushBackedge(BB &bb, std::stack<BB*> &succs, bool &childPushed) {
  while (!succs.empty()) {
    BB *succBB = succs.top();
    ASSERT(succBB != nullptr, "unexpected null bb in LoopFinder::PushBackedge");
    succs.pop();
    if (!visitedBBs[succBB->GetId()]) {
      childPushed = true;
      succBB->SetLevel(bb.GetLevel() + 1);
      sortedBBs[succBB->GetId()] = &bb;  /* tracking parent of traversed child */
      dfsBBs.push(succBB);
    } else if ((succBB->GetLevel() != 0) && (bb.GetLevel() >= succBB->GetLevel())) {
      /* Backedge bb -> succBB */
      DetectLoop(succBB, &bb);
      bb.PushBackLoopSuccs(*succBB);
      succBB->PushBackLoopPreds(bb);
    }
  }
}

void LoopFinder::MergeLoops() {
  for (LoopHierarchy *loopHierarchy1 = loops; loopHierarchy1 != nullptr; loopHierarchy1 = loopHierarchy1->GetNext()) {
    for (LoopHierarchy *loopHierarchy2 = loopHierarchy1->GetNext(); loopHierarchy2 != nullptr;
         loopHierarchy2 = loopHierarchy2->GetNext()) {
      if (loopHierarchy1->GetHeader() != loopHierarchy2->GetHeader()) {
        continue;
      }
      for (auto *bb : loopHierarchy2->GetLoopMembers()) {
        loopHierarchy1->InsertLoopMembers(*bb);
      }
      for (auto bb : loopHierarchy2->otherLoopEntries) {
        loopHierarchy1->otherLoopEntries.insert(bb);
      }
      for (auto *bb : loopHierarchy2->GetBackedge()) {
        loopHierarchy1->InsertBackedge(*bb);
      }
      loopHierarchy2->GetPrev()->SetNext(loopHierarchy2->GetNext());
      if (loopHierarchy2->GetNext() != nullptr) {
        loopHierarchy2->GetNext()->SetPrev(loopHierarchy2->GetPrev());
      }
    }
  }
}

void LoopFinder::SortLoops() {
  LoopHierarchy *head = nullptr;
  LoopHierarchy *next1 = nullptr;
  LoopHierarchy *next2 = nullptr;
  bool swapped;
  do {
    swapped = false;
    for (LoopHierarchy *loopHierarchy1 = loops; loopHierarchy1 != nullptr;) {
      /* remember loopHierarchy1's prev in case if loopHierarchy1 moved */
      head = loopHierarchy1;
      next1 = loopHierarchy1->GetNext();
      for (LoopHierarchy *loopHierarchy2 = loopHierarchy1->GetNext(); loopHierarchy2 != nullptr;) {
        next2 = loopHierarchy2->GetNext();

        if (loopHierarchy1->GetLoopMembers().size() > loopHierarchy2->GetLoopMembers().size()) {
          if (head->GetPrev() == nullptr) {
            /* remove loopHierarchy2 from list */
            loopHierarchy2->GetPrev()->SetNext(loopHierarchy2->GetNext());
            if (loopHierarchy2->GetNext() != nullptr) {
              loopHierarchy2->GetNext()->SetPrev(loopHierarchy2->GetPrev());
            }
            /* link loopHierarchy2 as head */
            loops = loopHierarchy2;
            loopHierarchy2->SetPrev(nullptr);
            loopHierarchy2->SetNext(head);
            head->SetPrev(loopHierarchy2);
          } else {
            loopHierarchy2->GetPrev()->SetNext(loopHierarchy2->GetNext());
            if (loopHierarchy2->GetNext() != nullptr) {
              loopHierarchy2->GetNext()->SetPrev(loopHierarchy2->GetPrev());
            }
            head->GetPrev()->SetNext(loopHierarchy2);
            loopHierarchy2->SetPrev(head->GetPrev());
            loopHierarchy2->SetNext(head);
            head->SetPrev(loopHierarchy2);
          }
          head = loopHierarchy2;
          swapped = true;
        }
        loopHierarchy2 = next2;
      }
      loopHierarchy1 = next1;
    }
  } while (swapped);
}

void LoopFinder::UpdateOuterForInnerLoop(BB *bb, LoopHierarchy *outer) {
  if (outer == nullptr) {
    return;
  }
  for (auto ito = outer->GetLoopMembers().begin(); ito != outer->GetLoopMembers().end();) {
    if (*ito == bb) {
      ito = outer->EraseLoopMembers(ito);
    } else {
      ++ito;
    }
  }
  if (outer->GetOuterLoop() != nullptr) {
    UpdateOuterForInnerLoop(bb, const_cast<LoopHierarchy *>(outer->GetOuterLoop()));
  }
}

void LoopFinder::UpdateOuterLoop(LoopHierarchy *loop) {
  for (auto inner : loop->GetInnerLoops()) {
    UpdateOuterLoop(inner);
  }
  for (auto *bb : loop->GetLoopMembers()) {
    UpdateOuterForInnerLoop(bb, const_cast<LoopHierarchy *>(loop->GetOuterLoop()));
  }
}

void LoopFinder::CreateInnerLoop(LoopHierarchy &inner, LoopHierarchy &outer) {
  outer.InsertInnerLoops(inner);
  inner.SetOuterLoop(outer);
  if (loops == &inner) {
    loops = inner.GetNext();
  } else {
    LoopHierarchy *prev = loops;
    for (LoopHierarchy *loopHierarchy1 = loops->GetNext(); loopHierarchy1 != nullptr;
         loopHierarchy1 = loopHierarchy1->GetNext()) {
      if (loopHierarchy1 == &inner) {
        prev->SetNext(prev->GetNext()->GetNext());
      }
      prev = loopHierarchy1;
    }
  }
}

void LoopFinder::DetectInnerLoop() {
  bool innerCreated;
  do {
    innerCreated = false;
    for (LoopHierarchy *loopHierarchy1 = loops; loopHierarchy1 != nullptr;
         loopHierarchy1 = loopHierarchy1->GetNext()) {
      for (LoopHierarchy *loopHierarchy2 = loopHierarchy1->GetNext(); loopHierarchy2 != nullptr;
           loopHierarchy2 = loopHierarchy2->GetNext()) {
        if (loopHierarchy1->GetHeader() != loopHierarchy2->GetHeader()) {
          for (auto *bb : loopHierarchy2->GetLoopMembers()) {
            if (loopHierarchy1->GetHeader() != bb) {
              continue;
            }
            CreateInnerLoop(*loopHierarchy1, *loopHierarchy2);
            innerCreated = true;
            break;
          }
          if (innerCreated) {
            break;
          }
        }
      }
      if (innerCreated) {
        break;
      }
    }
  } while (innerCreated);

  for (LoopHierarchy *outer = loops; outer != nullptr; outer = outer->GetNext()) {
    UpdateOuterLoop(outer);
  }
}

static void CopyLoopInfo(LoopHierarchy &from, CGFuncLoops &to, CGFuncLoops *parent, MemPool &memPool) {
  to.SetHeader(*const_cast<BB*>(from.GetHeader()));
  for (auto bb : from.otherLoopEntries) {
    to.AddMultiEntries(*bb);
  }
  for (auto *bb : from.GetLoopMembers()) {
    to.AddLoopMembers(*bb);
    bb->SetLoop(to);
  }
  for (auto *bb : from.GetBackedge()) {
    to.AddBackedge(*bb);
  }
  if (!from.GetInnerLoops().empty()) {
    for (auto *inner : from.GetInnerLoops()) {
      CGFuncLoops *floop = memPool.New<CGFuncLoops>(memPool);
      to.AddInnerLoops(*floop);
      floop->SetLoopLevel(to.GetLoopLevel() + 1);
      CopyLoopInfo(*inner, *floop, &to, memPool);
    }
  }
  if (parent != nullptr) {
    to.SetOuterLoop(*parent);
  }
}

void LoopFinder::UpdateCGFunc() {
  for (LoopHierarchy *loop = loops; loop != nullptr; loop = loop->GetNext()) {
    CGFuncLoops *floop = cgFunc->GetMemoryPool()->New<CGFuncLoops>(*cgFunc->GetMemoryPool());
    cgFunc->PushBackLoops(*floop);
    floop->SetLoopLevel(1);    /* top level */
    CopyLoopInfo(*loop, *floop, nullptr, *cgFunc->GetMemoryPool());
  }
}

void LoopFinder::FormLoopHierarchy() {
  visitedBBs.clear();
  visitedBBs.resize(cgFunc->NumBBs(), false);
  sortedBBs.clear();
  sortedBBs.resize(cgFunc->NumBBs(), nullptr);
  FOR_ALL_BB(bb, cgFunc) {
    bb->SetLevel(0);
  }
  bool changed;
  do {
    changed = false;
    FOR_ALL_BB(bb, cgFunc) {
      if (!visitedBBs[bb->GetId()]) {
        dfsBBs.push(bb);
        FindBackedge();
        changed = true;
      }
    }
  } while (changed);
  /*
   * FIX : Should merge the partial loops at the time of initial
   * construction.  And make the linked list as a sorted set,
   * then the merge and sort phases below can go away.
   *
   * Start merging the loops with the same header
   */
  MergeLoops();
  /* order loops from least number of members */
  SortLoops();
  DetectInnerLoop();
  UpdateCGFunc();
}

bool CgLoopAnalysis::PhaseRun(maplebe::CGFunc &f) {
  f.ClearLoopInfo();
  MemPool *loopMemPool = GetPhaseMemPool();
  LoopFinder *loopFinder = loopMemPool->New<LoopFinder>(f, *loopMemPool);
  loopFinder->FormLoopHierarchy();

  if (LOOP_ANALYSIS_DUMP_NEWPM) {
    /* do dot gen after detection so the loop backedge can be properly colored using the loop info */
    DotGenerator::GenerateDot("buildloop", f, f.GetMirModule(), true, f.GetName());
  }
#if DEBUG
  for (const auto *lp : f.GetLoops()) {
    lp->CheckLoops();
  }
#endif

  return false;
}
MAPLE_ANALYSIS_PHASE_REGISTER(CgLoopAnalysis, loopanalysis)
}  /* namespace maplebe */
