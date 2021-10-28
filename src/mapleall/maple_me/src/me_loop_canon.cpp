/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_loop_canon.h"
#include <iostream>
#include <algorithm>
#include "me_cfg.h"
#include "me_option.h"
#include "me_dominance.h"
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

bool MeLoopCanon::NeedConvert(MeFunction *func, BB &bb, BB &pred, MapleAllocator &localAlloc,
                              MapleMap<Key, bool> &swapSuccs) const {
  // loop is transformed
  if (pred.GetAttributes(kBBAttrArtificial)) {
    return false;
  }
  bb.SetAttributes(kBBAttrIsInLoop);
  pred.SetAttributes(kBBAttrIsInLoop);
  // do not convert do-while loop
  if ((bb.GetKind() != kBBCondGoto) || (&pred == &bb) || bb.GetAttributes(kBBAttrIsTry) ||
      bb.GetAttributes(kBBAttrIsCatch)) {
    return false;
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

void MeLoopCanon::Convert(MeFunction &func, BB &bb, BB &pred, MapleMap<Key, bool> &swapSuccs) {
  // if bb->fallthru is in loopbody, latchBB need convert condgoto and make original target as its fallthru
  bool swapSuccOfLatch = (swapSuccs.find(std::make_pair(&bb, &pred)) != swapSuccs.end());
  if (isDebugFunc) {
    LogInfo::MapleLogger() << "***loop convert: backedge bb->id " << bb.GetBBId() << " pred->id "
                           << pred.GetBBId();
    if (swapSuccOfLatch) {
      LogInfo::MapleLogger() << " need swap succs\n";
    } else {
      LogInfo::MapleLogger() << '\n';
    }
  }
  // new latchBB
  BB *latchBB = func.GetCfg()->NewBasicBlock();
  latchBB->SetAttributes(kBBAttrArtificial);
  latchBB->SetAttributes(kBBAttrIsInLoop); // latchBB is inloop
  // update pred bb
  pred.ReplaceSucc(&bb, latchBB); // replace pred.succ with latchBB and set pred of latchBB
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

void MeLoopCanon::ExecuteLoopCanon(MeFunction &func, Dominance &dom) {
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
    for (auto it = backEdges.begin(); it != backEdges.end(); ++it) {
      BB *bb = it->first;
      BB *pred = it->second;
      ASSERT(bb != nullptr, "bb should not be nullptr");
      ASSERT(pred != nullptr, "pred should not be nullptr");
      Convert(func, *bb, *pred, swapSuccs);
      if (isDebugFunc) {
        LogInfo::MapleLogger() << "-----------------Dump mefunction after loop convert-----------\n";
        func.Dump(true);
      }
    }
    isCFGChange = true;
  }
}

// Only when backedge or preheader is not try bb, update head map.
void MeLoopCanon::FindHeadBBs(MeFunction &func, Dominance &dom, const BB *bb,
                              std::map<BBId, std::vector<BB*>> &heads) const {
  if (bb == nullptr || bb == func.GetCfg()->GetCommonExitBB()) {
    return;
  }
  bool hasTry = false;
  bool hasIgoto = false;
  for (BB *pred : bb->GetPred()) {
    // backedge or preheader is try bb
    if (pred->GetAttributes(kBBAttrIsTry)) {
      hasTry = true;
    }
  }
  // backedge is constructed by igoto
  if (bb->GetBBLabel() != 0) {
    const MapleUnorderedSet<LabelIdx> &addrTakenLabels =
        func.GetMirFunc()->GetLabelTab()->GetAddrTakenLabels();
    if (addrTakenLabels.find(bb->GetBBLabel()) != addrTakenLabels.end()) {
      hasIgoto = true;
    }
  }
  if ((!hasTry) && (!hasIgoto)) {
    for (BB *pred : bb->GetPred()) {
      // add backedge bb
      if (dom.Dominate(*bb, *pred)) {
        if (heads.find(bb->GetBBId()) != heads.end()) {
          heads[bb->GetBBId()].push_back(pred);
        } else {
          std::vector<BB*> tails;
          tails.push_back(pred);
          heads[bb->GetBBId()] = tails;
        }
      }
    }
  }
  const auto &domChildren = dom.GetDomChildren(bb->GetBBId());
  for (auto bbit = domChildren.begin(); bbit != domChildren.end(); ++bbit) {
    FindHeadBBs(func, dom, func.GetCfg()->GetAllBBs().at(*bbit), heads);
  }
}

void MeLoopCanon::UpdateTheOffsetOfStmtWhenTargetBBIsChange(MeFunction &func, BB &curBB,
                                                            const BB &oldSuccBB, BB &newSuccBB) const {
  StmtNode *lastStmt = &(curBB.GetStmtNodes().back());
  if ((lastStmt->GetOpCode() == OP_brtrue || lastStmt->GetOpCode() == OP_brfalse) &&
      static_cast<CondGotoNode*>(lastStmt)->GetOffset() == oldSuccBB.GetBBLabel()) {
    LabelIdx label = func.GetOrCreateBBLabel(newSuccBB);
    static_cast<CondGotoNode*>(lastStmt)->SetOffset(label);
  } else if (lastStmt->GetOpCode() == OP_goto &&
             static_cast<GotoNode*>(lastStmt)->GetOffset() == oldSuccBB.GetBBLabel()) {
    LabelIdx label = func.GetOrCreateBBLabel(newSuccBB);
    static_cast<GotoNode*>(lastStmt)->SetOffset(label);
  } else if (lastStmt->GetOpCode() == OP_switch) {
    SwitchNode *switchNode = static_cast<SwitchNode*>(lastStmt);
    if (switchNode->GetDefaultLabel() == oldSuccBB.GetBBLabel()) {
      switchNode->SetDefaultLabel(func.GetOrCreateBBLabel(newSuccBB));
    }
    for (size_t i = 0; i < switchNode->GetSwitchTable().size(); ++i) {
      LabelIdx labelIdx = switchNode->GetCasePair(i).second;
      if (labelIdx == oldSuccBB.GetBBLabel()) {
        switchNode->UpdateCaseLabelAt(i, func.GetOrCreateBBLabel(newSuccBB));
      }
    }
  }
}

// merge backedges with the same headBB
void MeLoopCanon::Merge(MeFunction &func, const std::map<BBId, std::vector<BB*>> &heads) {
  MeCFG *cfg = func.GetCfg();
  for (auto iter = heads.begin(); iter != heads.end(); ++iter) {
    BB *head = cfg->GetBBFromID(iter->first);
    // skip case : check latch bb is already added
    // one pred is preheader bb and the other is latch bb
    if ((head->GetPred().size() == 2) &&
        (head->GetPred(0)->GetAttributes(kBBAttrArtificial)) &&
        (head->GetPred(0)->GetKind() == kBBFallthru) &&
        (head->GetPred(1)->GetAttributes(kBBAttrArtificial)) &&
        (head->GetPred(1)->GetKind() == kBBFallthru)) {
      continue;
    }
    auto *latchBB = cfg->NewBasicBlock();
    latchBB->SetAttributes(kBBAttrArtificial);
    latchBB->SetAttributes(kBBAttrIsInLoop); // latchBB is inloop
    latchBB->SetKind(kBBGoto);
    auto *newGotoStmt = func.GetMirFunc()->GetCodeMemPool()->New<GotoNode>(OP_goto);
    newGotoStmt->SetOffset(func.GetOrCreateBBLabel(*head));
    latchBB->AddStmtNode(newGotoStmt);
    for (BB *tail : iter->second) {
      tail->ReplaceSucc(head, latchBB);
      if (tail->GetStmtNodes().empty()) {
        continue;
      }
      UpdateTheOffsetOfStmtWhenTargetBBIsChange(func, *tail, *head, *latchBB);
    }
    head->AddPred(*latchBB);
    isCFGChange = true;
  }
}

void MeLoopCanon::AddPreheader(MeFunction &func, const std::map<BBId, std::vector<BB*>> &heads) {
  MeCFG *cfg = func.GetCfg();
  for (auto iter = heads.begin(); iter != heads.end(); ++iter) {
    BB *head =  cfg->GetBBFromID(iter->first);
    std::vector<BB*> preds;
    for (BB *pred : head->GetPred()) {
      // FindHeadBBs has filtered out this possibility.
      CHECK_FATAL(!pred->GetAttributes(kBBAttrIsTry), "can not support kBBAttrIsTry");
      if (std::find(iter->second.begin(), iter->second.end(), pred) == iter->second.end()) {
        preds.push_back(pred);
      }
    }
    // if the head is entry bb, we also need to add a preheader to replace it as new entry
    if (head->GetAttributes(kBBAttrIsEntry)) {
      auto *newEntry = func.GetCfg()->NewBasicBlock();
      func.GetCfg()->GetCommonEntryBB()->RemoveEntry(*head);
      func.GetCfg()->GetCommonEntryBB()->AddEntry(*newEntry);
      newEntry->SetKind(kBBFallthru);
      newEntry->SetAttributes(kBBAttrIsEntry);
      head->ClearAttributes(kBBAttrIsEntry);
      head->AddPred(*newEntry);
    }
    // If the num of backages is zero or one and bb kind is kBBFallthru, Preheader is already canonical.
    if (preds.empty()) {
      continue;
    }
    if (preds.size() == 1 && preds.front()->GetKind() == kBBFallthru) {
      continue;
    }
    // add preheader
    auto *preheader = cfg->NewBasicBlock();
    preheader->SetAttributes(kBBAttrArtificial);
    preheader->SetKind(kBBFallthru);
    for (BB *pred : preds) {
      pred->ReplaceSucc(head, preheader);
      if (pred->GetStmtNodes().empty()) {
        continue;
      }
      UpdateTheOffsetOfStmtWhenTargetBBIsChange(func, *pred, *head, *preheader);
    }
    head->AddPred(*preheader);
    isCFGChange = true;
    if (preheader->GetPred().size() > 1) {
      (void)func.GetOrCreateBBLabel(*preheader);
    }
  }
}

void MeLoopCanon::InsertNewExitBB(MeFunction &func, LoopDesc &loop) {
  MeCFG *cfg = func.GetCfg();
  for (auto pair : loop.inloopBB2exitBBs) {
    BB *curBB = cfg->GetBBFromID(pair.first);
    if (curBB->GetKind() == kBBIgoto) {
      continue;
    }
    for (auto succBB : *pair.second) {
      bool needNewExitBB = false;
      for (auto pred : succBB->GetPred()) {
        if (!loop.Has(*pred)) {
          needNewExitBB = true;
        }
      }
      if (!needNewExitBB) {
        continue;
      } else {
        BB *newExitBB = cfg->NewBasicBlock();
        newExitBB->SetKind(kBBFallthru);
        size_t index = succBB->GetPred().size();
        while (index > 0) {
          if (succBB->GetPred(index - 1) == curBB) {
            break;
          }
          --index;
        }
        curBB->ReplaceSucc(succBB, newExitBB);
        --index;
        succBB->AddPred(*newExitBB, index);

        loop.ReplaceInloopBB2exitBBs(*curBB, *succBB, *newExitBB);
        if (curBB->GetStmtNodes().empty()) {
          continue;
        }
        UpdateTheOffsetOfStmtWhenTargetBBIsChange(func, *curBB, *succBB, *newExitBB);
      }
    }
  }
}

void MeLoopCanon::InsertExitBB(MeFunction &func, LoopDesc &loop) {
  MeCFG *cfg = func.GetCfg();
  std::set<BB*> traveledBBs;
  std::queue<BB*> inLoopBBs;
  inLoopBBs.push(loop.head);
  CHECK_FATAL(loop.inloopBB2exitBBs.empty(), "inloopBB2exitBBs must be empty");
  while (!inLoopBBs.empty()) {
    BB *curBB = inLoopBBs.front();
    inLoopBBs.pop();
    if (curBB->GetKind() == kBBCondGoto) {
      if (curBB->GetSucc().size() == 1) {
        CHECK_FATAL(false, "return bb");
        loop.InsertInloopBB2exitBBs(*curBB, *cfg->GetCommonExitBB());
      }
    } else if (!curBB->GetStmtNodes().empty() && curBB->GetLast().GetOpCode() == OP_return) {
      CHECK_FATAL(false, "return bb");
      loop.InsertInloopBB2exitBBs(*curBB, *cfg->GetCommonExitBB());
    }
    for (BB *succ : curBB->GetSucc()) {
      if (traveledBBs.count(succ) != 0) {
        continue;
      }
      if (loop.Has(*succ)) {
        inLoopBBs.push(succ);
        traveledBBs.insert(succ);
      } else {
        loop.InsertInloopBB2exitBBs(*curBB, *succ);
      }
    }
  }
  InsertNewExitBB(func, loop);
}

void MeLoopCanon::NormalizationHeadAndPreHeaderOfLoop(MeFunction &func, Dominance &dom) {
  if (isDebugFunc) {
    LogInfo::MapleLogger() << "-----------------Dump mefunction before loop normalization----------\n";
    func.Dump(true);
    func.GetCfg()->DumpToFile("cfgbeforLoopNormalization");
  }
  std::map<BBId, std::vector<BB*>> heads;
  isCFGChange = false;
  FindHeadBBs(func, dom, func.GetCfg()->GetCommonEntryBB(), heads);
  AddPreheader(func, heads);
  Merge(func, heads);
  // cfg changes only if heads is not empty
  if (!heads.empty()) {
    isCFGChange = true;
  }
}

void MeLoopCanon::NormalizationExitOfLoop(MeFunction &func, IdentifyLoops &meLoop) {
  for (auto loop : meLoop.GetMeLoops()) {
    if (loop->HasTryBB()) {
      continue;
    }

    if (!loop->IsCanonicalLoop()) {
      loop->inloopBB2exitBBs.clear();
      InsertExitBB(func, *loop);
      loop->SetIsCanonicalLoop(true);
      isCFGChange = true;
    }
  }
  if (isDebugFunc) {
    LogInfo::MapleLogger() << "-----------------Dump mefunction after loop normalization-----------\n";
    func.Dump(true);
    func.GetCfg()->DumpToFile("cfgafterLoopNormalization");
  }
}

void MELoopCanaon::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>();
  aDep.SetPreservedAll();
}

bool MELoopCanaon::PhaseRun(maple::MeFunction &f) {
  auto *dom = GET_ANALYSIS(MEDominance, f);
  ASSERT(dom != nullptr, "dom is null in MeDoLoopCanon::Run");
  MemPool *loopCanonMp = GetPhaseMemPool();
  auto *meLoopCanon = loopCanonMp->New<MeLoopCanon>(DEBUGFUNC_NEWPM(f), *loopCanonMp);
  // 1. Converting loops to exhibit the do {} while (condition)
  meLoopCanon->ExecuteLoopCanon(f, *dom);
  if (meLoopCanon->IsCFGChange()) {
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &MEDominance::id);
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &MELoopAnalysis::id);
  }
  meLoopCanon->ResetIsCFGChange();
  dom = FORCE_GET(MEDominance);

  // 2. Add preheaderBB and normalization headBB for loop.
  meLoopCanon->NormalizationHeadAndPreHeaderOfLoop(f, *dom);
  if (meLoopCanon->IsCFGChange()) {
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &MEDominance::id);
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &MELoopAnalysis::id);
  }
  meLoopCanon->ResetIsCFGChange();
  IdentifyLoops *meLoop = FORCE_GET(MELoopAnalysis);
  if (meLoop == nullptr) {
    return false;
  }
  // 3. Normalization exitBB for loop.
  meLoopCanon->NormalizationExitOfLoop(f, *meLoop);
  if (meLoopCanon->IsCFGChange()) {
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &MEDominance::id);
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &MELoopAnalysis::id);
  }
  return true;
}
}  // namespace maple
