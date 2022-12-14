/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_loop_inversion.h"

#include <algorithm>
#include <iostream>

#include "me_cfg.h"
#include "me_dominance.h"
#include "me_option.h"
#include "me_phase_manager.h"

namespace maple {
// This phase changes the control flow graph to canonicalize loops so that the
// resulting loop form can provide places to insert loop-invariant code hoisted
// from the loop body.  This means converting loops to exhibit the
//      do {} while (condition)
// loop form.
//
// Step 1: Loops are identified by the existence of a BB that dominates one of
// its predecessors.  For each such backedge, the function NeedConvert() is
// called to check if it needs to be re-structured.  If so, the backEdges are
// collected.
//
// Step2: The collected backEdges are sorted so that their loop restructuring
// can be processed in just one pass.
//
// Step3: Perform the conversion for the loop represented by each collected
// backedge in sorted order.
// sort backEdges if bb is used as pred in one backedge and bb in another backedge
// deal with the bb used as pred first
static bool CompareBackedge(const std::pair<BB*, BB*> &a, const std::pair<BB*, BB*> &b) {
  // second is pred, first is bb
  if ((a.second)->GetBBId() == (b.first)->GetBBId()) {
    return true;
  }
  if ((a.first)->GetBBId() == (b.second)->GetBBId()) {
    return false;
  }
  return (a.first)->GetBBId() < (b.first)->GetBBId();
}

bool MeLoopInversion::NeedConvert(MeFunction *func, BB &bb, BB &pred, MapleAllocator &localAlloc,
                                  MapleMap<Key, bool> &swapSuccs) const {
  // loop is transformed
  if (pred.GetAttributes(kBBAttrArtificial) && bb.GetAttributes(kBBAttrArtificial)) {
    return false;
  }
  bb.SetAttributes(kBBAttrIsInLoop);
  pred.SetAttributes(kBBAttrIsInLoop);
  // do not convert do-while loop
  if ((bb.GetKind() != kBBCondGoto) || (&pred == &bb) || bb.GetAttributes(kBBAttrIsTry) ||
      bb.GetAttributes(kBBAttrIsCatch)) {
    return false;
  }
  auto *condPrev = bb.GetLast().GetPrev();
  while (condPrev) {
    if (condPrev->GetOpCode() != OP_comment) {
      return false;
    }
    condPrev = condPrev->GetPrev();
  }

  if (bb.GetBBLabel() != 0) {
    const MapleUnorderedSet<LabelIdx> &addrTakenLabels = func->GetMirFunc()->GetLabelTab()->GetAddrTakenLabels();
    if (addrTakenLabels.find(bb.GetBBLabel()) != addrTakenLabels.end()) {
      return false;  // target of igoto cannot be cloned
    }
  }
  ASSERT(bb.GetSucc().size() == 2, "the number of bb's successors must equal 2");
  // if both succs are equal, return false
  if (bb.GetSucc().front() == bb.GetSucc().back()) {
    return false;
  }
  // check bb's succ both in loop body or not, such as
  //   1  <--
  //  / \   |
  //  2 |   |
  //  \ |   |
  //   3 ---|
  //  /
  MapleSet<BBId> inLoop(std::less<BBId>(), localAlloc.Adapter());
  MapleList<BB*> bodyList(localAlloc.Adapter());
  bodyList.push_back(&pred);
  while (!bodyList.empty()) {
    BB *curr = bodyList.front();
    bodyList.pop_front();
    // skip bb and bb is already in loop body(has been dealt with)
    if (curr == &bb || inLoop.count(curr->GetBBId()) == 1) {
      continue;
    }
    (void)inLoop.insert(curr->GetBBId());
    for (BB *tmpPred : curr->GetPred()) {
      ASSERT_NOT_NULL(tmpPred);
      bodyList.push_back(tmpPred);
      tmpPred->SetAttributes(kBBAttrIsInLoop);
    }
  }
  if ((inLoop.count(bb.GetSucc(0)->GetBBId()) == 1) && (inLoop.count(bb.GetSucc(1)->GetBBId()) == 1)) {
    return false;
  }
  // other case
  // fallthru is in loop body, latchBB need swap succs
  if (inLoop.count(bb.GetSucc().at(0)->GetBBId()) == 1) {
    (void)swapSuccs.insert(make_pair(std::make_pair(&bb, &pred), true));
  }
  return true;
}

void MeLoopInversion::Convert(MeFunction &func, BB &bb, BB &pred, MapleMap<Key, bool> &swapSuccs) {
  // if bb->fallthru is in loopbody, latchBB need convert condgoto and make original target as its fallthru
  bool swapSuccOfLatch = (swapSuccs.find(std::make_pair(&bb, &pred)) != swapSuccs.cend());
  if (isDebugFunc) {
    LogInfo::MapleLogger() << "***loop convert: backedge bb->id " << bb.GetBBId() << " pred->id " << pred.GetBBId();
    if (swapSuccOfLatch) {
      LogInfo::MapleLogger() << " need swap succs\n";
    } else {
      LogInfo::MapleLogger() << '\n';
    }
  }
  // new latchBB
  BB *latchBB = func.GetCfg()->NewBasicBlock();
  latchBB->SetAttributes(kBBAttrArtificial);
  latchBB->SetAttributes(kBBAttrIsInLoop);  // latchBB is inloop
  // update newBB frequency : copy predBB succFreq as latch frequency
  if (func.GetCfg()->UpdateCFGFreq()) {
    int idx = pred.GetSuccIndex(bb);
    ASSERT(idx >= 0 && idx < pred.GetSucc().size(), "sanity check");
    FreqType freq = pred.GetEdgeFreq(idx);
    latchBB->SetFrequency(freq);
    // update bb frequency: remove pred frequency since pred is deleted
    ASSERT(bb.GetFrequency() >= freq, "sanity check");
    bb.SetFrequency(bb.GetFrequency() - freq);
  }
  // update pred bb
  pred.ReplaceSucc(&bb, latchBB);  // replace pred.succ with latchBB and set pred of latchBB
  // update pred stmt if needed
  if (pred.GetKind() == kBBGoto) {
    ASSERT(pred.GetAttributes(kBBAttrIsTry) || pred.GetSucc().size() == 1, "impossible");
    ASSERT(!pred.GetStmtNodes().empty(), "impossible");
    ASSERT(pred.GetStmtNodes().back().GetOpCode() == OP_goto, "impossible");
    // delete goto stmt and make pred is fallthru
    pred.RemoveLastStmt();
    pred.SetKind(kBBFallthru);
  } else if (pred.GetKind() == kBBCondGoto) {
    // if replaced bb is goto target
    ASSERT(pred.GetAttributes(kBBAttrIsTry) || pred.GetSucc().size() == 2,
           "pred should have attr kBBAttrIsTry or have 2 successors");
    ASSERT(!pred.GetStmtNodes().empty(), "pred's stmtNodeList should not be empty");
    auto &condGotoStmt = static_cast<CondGotoNode&>(pred.GetStmtNodes().back());
    if (latchBB == pred.GetSucc().at(1)) {
      // latchBB is the new target
      LabelIdx label = func.GetOrCreateBBLabel(*latchBB);
      condGotoStmt.SetOffset(label);
    }
  } else if (pred.GetKind() == kBBFallthru) {
    // do nothing
  } else if (pred.GetKind() == kBBSwitch) {
    ASSERT(!pred.GetStmtNodes().empty(), "bb is empty");
    auto &switchStmt = static_cast<SwitchNode&>(pred.GetStmtNodes().back());
    ASSERT(bb.GetBBLabel() != 0, "wrong bb label");
    LabelIdx oldLabIdx = bb.GetBBLabel();
    LabelIdx label = func.GetOrCreateBBLabel(*latchBB);
    if (switchStmt.GetDefaultLabel() == oldLabIdx) {
      switchStmt.SetDefaultLabel(label);
    }
    for (size_t i = 0; i < switchStmt.GetSwitchTable().size(); i++) {
      LabelIdx labelIdx = switchStmt.GetCasePair(i).second;
      if (labelIdx == oldLabIdx) {
        switchStmt.UpdateCaseLabelAt(i, label);
      }
    }
  } else {
    CHECK_FATAL(false, "unexpected pred kind");
  }
  // clone instructions of bb to latchBB
  func.GetCfg()->CloneBasicBlock(*latchBB, bb);
  // clone bb's succ to latchBB
  for (BB *succ : bb.GetSucc()) {
    ASSERT(!latchBB->GetAttributes(kBBAttrIsTry) || latchBB->GetSucc().empty(),
           "loopcanon : tryblock should insert succ before handler");
    latchBB->AddSucc(*succ);
  }
  latchBB->SetKind(bb.GetKind());
  // update succFreq
  if (func.GetCfg()->UpdateCFGFreq()) {
    if (latchBB->GetKind() == kBBCondGoto) {
      BB *succInloop = swapSuccOfLatch ? bb.GetSucc(0) : bb.GetSucc(1);
      if ((latchBB->GetFrequency() != 0) && (succInloop->GetFrequency() > 0)) {
        // loop is executed
        int64_t latchFreq = latchBB->GetFrequency();
        if (swapSuccOfLatch) {
          // bb fallthru is in loop, frequency of bb -> exitbb is set 0
          // latchBB fallthru is loop exit
          int fallthrudiff = static_cast<int>(bb.GetSucc(0)->GetFrequency() - bb.GetFrequency());
          if (fallthrudiff >= 0) {
            bb.SetSuccFreq(0, bb.GetFrequency());
            bb.SetSuccFreq(1, 0);
            latchBB->PushBackSuccFreq(latchFreq - fallthrudiff);
            latchBB->PushBackSuccFreq(fallthrudiff);
          } else {  // assume loop body executed only once
            bb.SetSuccFreq(0, bb.GetSucc(0)->GetFrequency());
            bb.SetSuccFreq(1, bb.GetFrequency() - bb.GetSucc(0)->GetFrequency());
            latchBB->PushBackSuccFreq(latchBB->GetFrequency());
            latchBB->PushBackSuccFreq(0);
          }
        } else {
          // bb->fallthru is loop exit, edge frequency of  bb ->exitbb is set 0
          // latchBB fallthru is in loop
          int fallthrudiff = static_cast<int>(bb.GetSucc(1)->GetFrequency() - bb.GetFrequency());
          if (fallthrudiff >= 0) {
            bb.SetSuccFreq(1, bb.GetFrequency());
            bb.SetSuccFreq(0, 0);
            latchBB->PushBackSuccFreq(latchFreq - fallthrudiff);
            latchBB->PushBackSuccFreq(fallthrudiff);
          } else {
            bb.SetSuccFreq(1, bb.GetSucc(0)->GetFrequency());
            bb.SetSuccFreq(0, bb.GetFrequency() - bb.GetSucc(0)->GetFrequency());
            latchBB->PushBackSuccFreq(0);
            latchBB->PushBackSuccFreq(latchBB->GetFrequency());
          }
        }
      } else {
        // loop is not executed
        if (latchBB->GetFrequency() != 0) {
          if (swapSuccOfLatch) {
            bb.SetSuccFreq(0, 0);
            bb.SetSuccFreq(1, bb.GetFrequency());
          } else {
            bb.SetSuccFreq(0, bb.GetFrequency());
            bb.SetSuccFreq(1, 0);
          }
        }
        latchBB->PushBackSuccFreq(0);
        latchBB->PushBackSuccFreq(0);
      }
    } else if (latchBB->GetKind() == kBBFallthru || latchBB->GetKind() == kBBGoto) {
      FreqType newsuccFreq = (latchBB->GetFrequency() == 0) ? 0 : latchBB->GetSuccFreq()[0] - 1;
      latchBB->SetSuccFreq(0, newsuccFreq);  // loop is changed to do-while format
      bb.SetSuccFreq(0, bb.GetFrequency());
    } else {
      ASSERT(0, "NYI:: unexpected bb type");
    }
  }
  // swap latchBB's succ if needed
  if (swapSuccOfLatch) {
    // modify condBr stmt
    ASSERT(latchBB->GetKind() == kBBCondGoto, "impossible");
    auto &condGotoStmt = static_cast<CondGotoNode&>(latchBB->GetStmtNodes().back());
    ASSERT(condGotoStmt.IsCondBr(), "impossible");
    condGotoStmt.SetOpCode((condGotoStmt.GetOpCode() == OP_brfalse) ? OP_brtrue : OP_brfalse);
    BB *fallthru = latchBB->GetSucc(0);
    LabelIdx label = func.GetOrCreateBBLabel(*fallthru);
    condGotoStmt.SetOffset(label);
    // swap succ
    BB *tmp = latchBB->GetSucc(0);
    latchBB->SetSucc(0, latchBB->GetSucc(1));
    latchBB->SetSucc(1, tmp);
  }
}

void MeLoopInversion::ExecuteLoopInversion(MeFunction &func, const Dominance &dom) {
  // set MeCFG's has_do_while flag
  MeCFG *cfg = func.GetCfg();
  auto eIt = cfg->valid_end();
  for (auto bIt = cfg->valid_begin(); bIt != eIt; ++bIt) {
    if (bIt == cfg->common_entry() || bIt == cfg->common_exit()) {
      continue;
    }
    auto *bb = *bIt;
    if (bb->GetKind() != kBBCondGoto) {
      continue;
    }
    StmtNode *stmt = bb->GetStmtNodes().rbegin().base().d();
    if (stmt == nullptr) {
      continue;
    }
    CondGotoNode *condBr = safe_cast<CondGotoNode>(stmt);
    if (condBr == nullptr) {
      continue;
    }
    BB *brTargetbb = bb->GetSucc(1);
    if (dom.Dominate(*brTargetbb, *bb)) {
      cfg->SetHasDoWhile(true);
      break;
    }
  }
  MapleAllocator localAlloc(innerMp);
  MapleVector<std::pair<BB*, BB*>> backEdges(localAlloc.Adapter());
  MapleMap<Key, bool> swapSuccs(std::less<Key>(), localAlloc.Adapter());
  // collect backedge first: if bb dominator its pred, then the edge pred->bb is a backedge
  eIt = cfg->valid_end();
  for (auto bIt = cfg->valid_begin(); bIt != eIt; ++bIt) {
    if (bIt == cfg->common_entry() || bIt == cfg->common_exit()) {
      continue;
    }
    auto *bb = *bIt;
    const MapleVector<BB*> &preds = bb->GetPred();
    for (BB *pred : preds) {
      ASSERT(cfg->GetCommonEntryBB() != nullptr, "impossible");
      ASSERT_NOT_NULL(pred);
      // bb is reachable from entry && bb dominator pred
      if (dom.Dominate(*cfg->GetCommonEntryBB(), *bb) && dom.Dominate(*bb, *pred) &&
          !pred->GetAttributes(kBBAttrWontExit) && (NeedConvert(&func, *bb, *pred, localAlloc, swapSuccs))) {
        if (isDebugFunc) {
          LogInfo::MapleLogger() << "find backedge " << bb->GetBBId() << " <-- " << pred->GetBBId() << '\n';
        }
        backEdges.push_back(std::make_pair(bb, pred));
      }
    }
  }
  // l with the edge which shared bb is used as pred
  // if backedge 4->3 is converted first, it will create a new backedge
  // <new latchBB-> BB1>, which needs iteration to deal with.
  //                 1  <---
  //               /  \    |
  //              6   2    |
  //                  /    |
  //          |---> 3 -----|
  //          |     |
  //          ------4
  //
  sort(backEdges.begin(), backEdges.end(), CompareBackedge);
  if (!backEdges.empty()) {
    if (isDebugFunc) {
      LogInfo::MapleLogger() << "-----------------Dump mefunction before loop convert----------\n";
      func.Dump(true);
    }
    for (size_t i = 0; i < backEdges.size(); i++) {
      BB *bb = backEdges[i].first;
      BB *pred = backEdges[i].second;
      ASSERT(bb != nullptr, "bb should not be nullptr");
      ASSERT(pred != nullptr, "pred should not be nullptr");
      Convert(func, *bb, *pred, swapSuccs);
      for (auto *succ : bb->GetSucc()) {
        if (!dom.Dominate(*succ, *pred)) {
          continue;
        }
        for (BB *tmpPred : succ->GetPred()) {
          if (!tmpPred->IsSuccBB(*pred)) {
            continue;
          }
          if ((NeedConvert(&func, *succ, *tmpPred, localAlloc, swapSuccs))) {
            if (isDebugFunc) {
              LogInfo::MapleLogger() << "find new backedge " << succ->GetBBId() << " <-- " << tmpPred->GetBBId()
                                     << '\n';
            }
            backEdges.push_back(std::make_pair(succ, tmpPred));
          }
        }
      }
    }
    isCFGChange = true;
    if (isDebugFunc) {
      LogInfo::MapleLogger() << "-----------------Dump mefunction after loop convert-----------\n";
      func.Dump(true);
    }
  }
}

void MELoopInversion::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>();
  aDep.SetPreservedAll();
}

bool MELoopInversion::PhaseRun(maple::MeFunction &f) {
  auto *dom = EXEC_ANALYSIS(MEDominance, f)->GetDomResult();
  ASSERT(dom != nullptr, "dom is null in MeDoLoopCanon::Run");
  MemPool *loopInversionMp = GetPhaseMemPool();
  auto *meLoopInversion = loopInversionMp->New<MeLoopInversion>(DEBUGFUNC_NEWPM(f), *loopInversionMp);
  meLoopInversion->ExecuteLoopInversion(f, *dom);
  if (meLoopInversion->IsCFGChange()) {
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &MEDominance::id);
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &MELoopAnalysis::id);
    if (f.GetCfg()->UpdateCFGFreq() && (f.GetCfg()->DumpIRProfileFile())) {
      f.GetCfg()->DumpToFile("after-loopinversion", false, true);
    }
  }
  return true;
}
}  // namespace maple
