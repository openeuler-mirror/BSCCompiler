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
#define LOOP_ANALYSIS_DUMP CG_DEBUG_FUNC(cgFunc)
#define LOOP_ANALYSIS_DUMP_NEWPM CG_DEBUG_FUNC_NEWPM(f, PhaseName())

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

void LoopFinder::formLoop(BB* headBB, BB* backBB) {
  ASSERT(headBB != nullptr && backBB != nullptr, "headBB or backBB is nullptr");
  LoopHierarchy *simple_loop = memPool->New<LoopHierarchy>(*memPool);

  if ( headBB != backBB) {
    ASSERT(!dfsBBs.empty(), "dfsBBs is empty");
    ASSERT(onPathBBs[headBB->GetId()], "headBB is not on execution path");
    std::stack<BB*> tempStk;

    tempStk.push(dfsBBs.top());
    dfsBBs.pop();

    while (tempStk.top() != headBB && !dfsBBs.empty()) {
      tempStk.push(dfsBBs.top());
      dfsBBs.pop();
    }

    while (!tempStk.empty()) {
      BB *topBB = tempStk.top();
      tempStk.pop();

      if (onPathBBs[topBB->GetId()]) {
        simple_loop->InsertLoopMembers(*topBB);
      }
      dfsBBs.push(topBB);
    }
  }
  // Note: backBB is NOT on dfsBBs
  simple_loop->InsertLoopMembers(*backBB);
  simple_loop->SetHeader(*headBB);
  simple_loop->InsertBackedge(*backBB);

  if (loops) {
    loops->SetPrev(simple_loop);
  }
  simple_loop->SetNext(loops);
  loops = simple_loop;
}

void LoopFinder::seekBackEdge(BB* bb, MapleList<BB*> succs) {
  for (const auto succBB : succs) {
    if (!visitedBBs[succBB->GetId()]) {
      dfsBBs.push(succBB);
    } else {
      if (onPathBBs[succBB->GetId()]) {
        formLoop(succBB, bb);
        bb->PushBackLoopSuccs(*succBB);
        succBB->PushBackLoopPreds(*bb);
      }
    }
  }
}

void LoopFinder::seekCycles() {
  while (!dfsBBs.empty()) {
    BB *bb = dfsBBs.top();
    if (visitedBBs[bb->GetId()]) {
      onPathBBs[bb->GetId()] = false;
      dfsBBs.pop();
      continue;
    }

    visitedBBs[bb->GetId()] = true;
    onPathBBs[bb->GetId()] = true;
    seekBackEdge(bb, bb->GetSuccs());
    seekBackEdge(bb, bb->GetEhSuccs());
  }
}

void LoopFinder::markExtraEntries() {
  ASSERT(dfsBBs.empty(), "dfsBBs is NOT empty");
  std::vector<bool> inLoop;
  inLoop.resize(cgFunc->NumBBs());

  for (LoopHierarchy *loop = loops; loop != nullptr; loop = loop->GetNext()) {
    fill(visitedBBs.begin(), visitedBBs.end(), false);
    fill(inLoop.begin(), inLoop.end(), false);
    fill(visitedBBs.begin(), visitedBBs.end(), false);
    for (auto *bb : loop->GetLoopMembers()) {
      inLoop[bb->GetId()] = true;
    }

    FOR_ALL_BB(bb, cgFunc) {
      if (!visitedBBs[bb->GetId()]) {
        dfsBBs.push(bb);
        while (!dfsBBs.empty()) {
          BB *bb = dfsBBs.top();
          if (visitedBBs[bb->GetId()]) {
            onPathBBs[bb->GetId()] = false;
            dfsBBs.pop();
            continue;
          } else {
            visitedBBs[bb->GetId()] = true;
            onPathBBs[bb->GetId()] = true;
            for (const auto succBB : bb->GetSuccs()) {
              // check if entering a loop. Entry to a loop is considered as its path does not go through the loop's head
              if (inLoop[succBB->GetId()] &&
                  succBB->GetId() != loop->GetHeader()->GetId() &&
                  !onPathBBs[loop->GetHeader()->GetId()] &&
                  loop->otherLoopEntries.find(succBB) == loop->otherLoopEntries.end()) {
                loop->otherLoopEntries.insert(succBB);
              }
              if (!visitedBBs[succBB->GetId()]) {
                dfsBBs.push(succBB);
              }
            }
          }
        }
      }
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
  onPathBBs.clear();
  onPathBBs.resize(cgFunc->NumBBs(), false);

  FOR_ALL_BB(bb, cgFunc) {
    bb->SetLevel(0);
  }
  bool changed;
  do {
    changed = false;
    FOR_ALL_BB(bb, cgFunc) {
      if (!visitedBBs[bb->GetId()]) {
        dfsBBs.push(bb);
        seekCycles();
        changed = true;
      }
    }
  } while (changed);

  markExtraEntries();
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

AnalysisResult *CgDoLoopAnalysis::Run(CGFunc *cgFunc, CgFuncResultMgr *cgFuncResultMgr) {
  (void)cgFuncResultMgr;
  CHECK_FATAL(cgFunc != nullptr, "nullptr check");
  cgFunc->ClearLoopInfo();
  MemPool *loopMemPool = NewMemPool();
  LoopFinder *loopFinder = loopMemPool->New<LoopFinder>(*cgFunc, *loopMemPool);
  loopFinder->FormLoopHierarchy();

  if (LOOP_ANALYSIS_DUMP) {
    /* do dot gen after detection so the loop backedge can be properly colored using the loop info */
    DotGenerator::GenerateDot("buildloop", *cgFunc, cgFunc->GetMirModule(), true, cgFunc->GetName());
  }
#if DEBUG
  for (const auto *lp : cgFunc->GetLoops()) {
    lp->CheckLoops();
  }
#endif

  return loopFinder;
}

bool CgLoopAnalysis::PhaseRun(maplebe::CGFunc &f) {
  if (LOOP_ANALYSIS_DUMP_NEWPM) {
    DotGenerator::GenerateDot("buildloop", f, f.GetMirModule());
  }
  f.ClearLoopInfo();
  MemPool *loopMemPool = GetPhaseMemPool();
  LoopFinder *loopFinder = loopMemPool->New<LoopFinder>(f, *loopMemPool);
  loopFinder->FormLoopHierarchy();

#if DEBUG
  for (const auto *lp : f.GetLoops()) {
    lp->CheckLoops();
  }
#endif

  return false;
}
MAPLE_ANALYSIS_PHASE_REGISTER(CgLoopAnalysis, loopanalysis)
}  /* namespace maplebe */
