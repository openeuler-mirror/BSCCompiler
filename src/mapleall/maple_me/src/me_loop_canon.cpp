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

#include <algorithm>
#include <iostream>

#include "me_cfg.h"
#include "me_dominance.h"
#include "me_option.h"
#include "me_phase_manager.h"

namespace maple {
// This phase ensures that every loop has a preheader and a single latch,
// and also ensures all exits are dominated by the loop head.

// Only when backedge or preheader is not try bb, update head map.
void MeLoopCanon::FindHeadBBs(Dominance &dom, const BB *bb, std::map<BBId, std::vector<BB*>> &heads) const {
  if (bb == nullptr || bb == func.GetCfg()->GetCommonExitBB()) {
    return;
  }
  bool hasTry = bb->GetAttributes(kBBAttrIsTry) || bb->GetAttributes(kBBAttrWontExit);
  bool hasIgoto = false;
  for (BB *pred : bb->GetPred()) {
    // backedge or preheader is try bb
    if (pred->GetAttributes(kBBAttrIsTry) || bb->GetAttributes(kBBAttrWontExit)) {
      hasTry = true;
    }
  }
  // backedge is constructed by igoto
  if (bb->GetBBLabel() != 0) {
    const MapleUnorderedSet<LabelIdx> &addrTakenLabels = func.GetMirFunc()->GetLabelTab()->GetAddrTakenLabels();
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
    FindHeadBBs(dom, func.GetCfg()->GetAllBBs().at(*bbit), heads);
  }
}

void MeLoopCanon::UpdateTheOffsetOfStmtWhenTargetBBIsChange(BB &curBB, const BB &oldSuccBB, BB &newSuccBB) const {
  MeStmt &lastStmt = curBB.GetMeStmts().back();
  if ((lastStmt.GetOp() == OP_brtrue || lastStmt.GetOp() == OP_brfalse) &&
      static_cast<CondGotoMeStmt&>(lastStmt).GetOffset() == oldSuccBB.GetBBLabel()) {
    auto label = func.GetOrCreateBBLabel(newSuccBB);
    static_cast<CondGotoMeStmt&>(lastStmt).SetOffset(label);
  } else if (lastStmt.GetOp() == OP_goto && static_cast<GotoMeStmt&>(lastStmt).GetOffset() == oldSuccBB.GetBBLabel()) {
    auto label = func.GetOrCreateBBLabel(newSuccBB);
    static_cast<GotoMeStmt&>(lastStmt).SetOffset(label);
  } else if (lastStmt.GetOp() == OP_switch) {
    SwitchMeStmt &switchStmt = static_cast<SwitchMeStmt&>(lastStmt);
    auto label = func.GetOrCreateBBLabel(newSuccBB);
    if (switchStmt.GetDefaultLabel() == oldSuccBB.GetBBLabel()) {
      switchStmt.SetDefaultLabel(label);
    }
    for (size_t i = 0; i < switchStmt.GetSwitchTable().size(); ++i) {
      LabelIdx labelIdx = switchStmt.GetSwitchTable().at(i).second;
      if (labelIdx == oldSuccBB.GetBBLabel()) {
        switchStmt.SetCaseLabel(i, label);
      }
    }
  }
}

// used to split some preds of SPLITTEDBB into MERGEDBB, these preds are saved in SPLITLIST
void MeLoopCanon::SplitPreds(const std::vector<BB*> &splitList, BB *splittedBB, BB *mergedBB) {
  if (splitList.size() == 1) {
    // quick split
    auto *pred = splitList[0];
    int index = pred->GetSuccIndex(*splittedBB);
    // add frequency to mergedBB
    if (updateFreqs) {
      int idx = pred->GetSuccIndex(*splittedBB);
      ASSERT(idx >= 0 && idx < pred->GetSucc().size(), "sanity check");
      uint64_t freq = pred->GetEdgeFreq(static_cast<uint32>(idx));
      mergedBB->SetFrequency(freq);
      mergedBB->PushBackSuccFreq(freq);
    }
    splittedBB->ReplacePred(pred, mergedBB);
    pred->AddSucc(*mergedBB, static_cast<size_t>(static_cast<uint32>(index)));
    if (updateFreqs) {
      pred->AddSuccFreq(mergedBB->GetFrequency(), static_cast<size_t>(static_cast<uint32>(index)));
    }
    if (!pred->GetMeStmts().empty()) {
      UpdateTheOffsetOfStmtWhenTargetBBIsChange(*pred, *splittedBB, *mergedBB);
    }
    isCFGChange = true;
    return;
  }
  // create phi list for mergedBB
  for (auto &phiIter : std::as_const(splittedBB->GetMePhiList())) {
    auto *latchLhs = func.GetIRMap()->CreateRegOrVarMeExprVersion(phiIter.first);
    auto *latchPhi = func.GetIRMap()->CreateMePhi(*latchLhs);
    latchPhi->SetDefBB(mergedBB);
    mergedBB->GetMePhiList().emplace(latchLhs->GetOstIdx(), latchPhi);
    phiIter.second->GetOpnds().push_back(latchLhs);
  }
  uint64_t freq = 0;
  for (BB *pred : splitList) {
    auto pos = splittedBB->GetPredIndex(*pred);
    for (auto &phiIter : std::as_const(mergedBB->GetMePhiList())) {
      // replace phi opnd
      phiIter.second->GetOpnds().push_back(splittedBB->GetMePhiList().at(phiIter.first)->GetOpnd(pos));
    }
    splittedBB->RemovePhiOpnd(pos);
    if (updateFreqs) {
      int idx = pred->GetSuccIndex(*splittedBB);
      ASSERT(idx >= 0 && idx < pred->GetSucc().size(), "sanity check");
      freq = pred->GetEdgeFreq(static_cast<size_t>(static_cast<uint32>(idx)));
      mergedBB->SetFrequency(mergedBB->GetFrequency() + freq);
    }
    pred->ReplaceSucc(splittedBB, mergedBB);
    if (updateFreqs) {
      int idx = pred->GetSuccIndex(*mergedBB);
      pred->SetSuccFreq(idx, freq);
    }
    if (pred->GetMeStmts().empty()) {
      continue;
    }
    UpdateTheOffsetOfStmtWhenTargetBBIsChange(*pred, *splittedBB, *mergedBB);
  }
  // may introduce redundant phi node, clean it
  for (auto phiIter = mergedBB->GetMePhiList().begin(); phiIter != mergedBB->GetMePhiList().end();) {
    auto *phi = phiIter->second;
    auto *phiOpnd0 = phi->GetOpnd(0);
    auto foundDiff = std::find_if(phi->GetOpnds().begin(), phi->GetOpnds().end(),
                                  [phiOpnd0](ScalarMeExpr *opnd) { return opnd != phiOpnd0; });
    if (foundDiff == phi->GetOpnds().end()) {
      auto &opnds = splittedBB->GetMePhiList()[phiIter->first]->GetOpnds();
      // mergedBB is always the last pred of splittedBB
      opnds[opnds.size() - 1] = phiOpnd0;
      phiIter = mergedBB->GetMePhiList().erase(phiIter);
    } else {
      ++phiIter;
    }
  }
  splittedBB->AddPred(*mergedBB);
  if (updateFreqs) {
    mergedBB->PushBackSuccFreq(mergedBB->GetFrequency());
  }
  isCFGChange = true;
}

// merge backedges with the same headBB
void MeLoopCanon::Merge(const std::map<BBId, std::vector<BB*>> &heads) {
  MeCFG *cfg = func.GetCfg();
  for (auto iter = heads.begin(); iter != heads.end(); ++iter) {
    BB *head = cfg->GetBBFromID(iter->first);
    // skip case : check latch bb is already added
    // one pred is preheader bb and the other is latch bb
    if ((head->GetPred().size() == 2) && (head->GetPred(0)->GetAttributes(kBBAttrArtificial)) &&
        (head->GetPred(0)->GetKind() == kBBFallthru) && (head->GetPred(1)->GetAttributes(kBBAttrArtificial)) &&
        (head->GetPred(1)->GetKind() == kBBFallthru)) {
      continue;
    }
    auto *latchBB = cfg->NewBasicBlock();
    latchBB->SetAttributes(kBBAttrArtificial);
    latchBB->SetAttributes(kBBAttrIsInLoop); // latchBB is inloop
    latchBB->SetKind(kBBFallthru);
    SplitPreds(iter->second, head, latchBB);
  }
}

void MeLoopCanon::AddPreheader(const std::map<BBId, std::vector<BB*>> &heads) {
  MeCFG *cfg = func.GetCfg();
  for (auto iter = heads.begin(); iter != heads.end(); ++iter) {
    BB *head = cfg->GetBBFromID(iter->first);
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
      isCFGChange = true;
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
    SplitPreds(preds, head, preheader);
    if (preheader->GetPred().size() > 1) {
      (void)func.GetOrCreateBBLabel(*preheader);
    }
  }
}

void MeLoopCanon::InsertExitBB(LoopDesc &loop) {
  std::set<BB*> traveledBBs;
  std::queue<BB*> inLoopBBs;
  MeCFG *cfg = func.GetCfg();
  inLoopBBs.push(loop.head);
  CHECK_FATAL(loop.inloopBB2exitBBs.empty(), "inloopBB2exitBBs must be empty");
  while (!inLoopBBs.empty()) {
    BB *curBB = inLoopBBs.front();
    inLoopBBs.pop();
    if (curBB->GetKind() == kBBCondGoto) {
      if (curBB->GetSucc().size() == 1) {
        // When the size of succs is one, one of succs may be commonExitBB. Need insert to loopBB2exitBBs.
        CHECK_FATAL(false, "return bb");
      }
    } else if (!curBB->GetMeStmts().empty() && curBB->GetLastMe()->IsReturn()) {
      CHECK_FATAL(false, "return bb");
    }
    for (BB *succ : curBB->GetSucc()) {
      if (traveledBBs.count(succ) != 0) {
        continue;
      }
      if (loop.Has(*succ)) {
        inLoopBBs.push(succ);
        traveledBBs.insert(succ);
        continue;
      }
      bool needNewExitBB = false;
      for (auto pred : succ->GetPred()) {
        if (!loop.Has(*pred)) {
          needNewExitBB = true;
          break;
        }
      }
      if (needNewExitBB) {
        // break the critical edge for code sinking
        BB *newExitBB = cfg->NewBasicBlock();
        newExitBB->SetKind(kBBFallthru);
        auto pos = succ->GetPredIndex(*curBB);
        uint64_t freq = 0;
        if (updateFreqs) {
          int idx = curBB->GetSuccIndex(*succ);
          ASSERT(idx >= 0 && idx < curBB->GetSuccFreq().size(), "sanity check");
          freq = curBB->GetSuccFreq()[static_cast<uint32>(idx)];
        }
        curBB->ReplaceSucc(succ, newExitBB);
        succ->AddPred(*newExitBB, pos);
        if (updateFreqs) {
          newExitBB->SetFrequency(freq);
          newExitBB->PushBackSuccFreq(freq);
        }
        if (!curBB->GetMeStmts().empty()) {
          UpdateTheOffsetOfStmtWhenTargetBBIsChange(*curBB, *succ, *newExitBB);
        }
        succ = newExitBB;
        isCFGChange = true;
      }
      loop.InsertInloopBB2exitBBs(*curBB, *succ);
    }
  }
}

void MeLoopCanon::NormalizationHeadAndPreHeaderOfLoop(Dominance &dom) {
  if (isDebugFunc) {
    LogInfo::MapleLogger() << "-----------------Dump mefunction before loop normalization----------\n";
    func.Dump(true);
    func.GetCfg()->DumpToFile("cfgbeforLoopNormalization", false, func.GetCfg()->UpdateCFGFreq());
  }
  std::map<BBId, std::vector<BB*>> heads;
  isCFGChange = false;
  FindHeadBBs(dom, func.GetCfg()->GetCommonEntryBB(), heads);
  AddPreheader(heads);
  Merge(heads);
}

void MeLoopCanon::NormalizationExitOfLoop(IdentifyLoops &meLoop) {
  for (auto loop : meLoop.GetMeLoops()) {
    if (loop->HasTryBB() || loop->HasIGotoBB()) {
      continue;
    }
    loop->inloopBB2exitBBs.clear();
    InsertExitBB(*loop);
    loop->SetIsCanonicalLoop(true);
  }
  if (isDebugFunc) {
    LogInfo::MapleLogger() << "-----------------Dump mefunction after loop normalization-----------\n";
    func.Dump(true);
    func.GetCfg()->DumpToFile("cfgafterLoopNormalization", false, func.GetCfg()->UpdateCFGFreq());
  }
}

void MELoopCanon::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>();
  aDep.AddRequired<MEIRMapBuild>();
  aDep.SetPreservedAll();
}

bool MELoopCanon::PhaseRun(maple::MeFunction &f) {
  auto *irMap = GET_ANALYSIS(MEIRMapBuild, f);
  CHECK_FATAL(irMap != nullptr, "irMap phase has problem");
  auto *dom = GET_ANALYSIS(MEDominance, f);
  ASSERT(dom != nullptr, "dom is null in MeDoLoopCanon::Run");
  MemPool *loopCanonMp = GetPhaseMemPool();
  bool updateFrequency = (Options::profileUse && f.GetMirFunc()->GetFuncProfData());
  auto *meLoopCanon = loopCanonMp->New<MeLoopCanon>(f, DEBUGFUNC_NEWPM(f), updateFrequency);

  // 1. Add preheaderBB and normalization headBB for loop.
  meLoopCanon->NormalizationHeadAndPreHeaderOfLoop(*dom);
  if (meLoopCanon->IsCFGChange()) {
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &MEDominance::id);
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &MELoopAnalysis::id);
  }
  IdentifyLoops *meLoop = FORCE_GET(MELoopAnalysis);
  if (meLoop == nullptr) {
    return false;
  }
  // 2. Normalization exitBB for loop.
  meLoopCanon->ResetIsCFGChange();
  meLoopCanon->NormalizationExitOfLoop(*meLoop);
  if (meLoopCanon->IsCFGChange()) {
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &MEDominance::id);
    if (updateFrequency && (f.GetCfg()->DumpIRProfileFile())) {
      f.GetCfg()->DumpToFile("after-loopcanon", false, true);
    }
  }
  return true;
}
}  // namespace maple
