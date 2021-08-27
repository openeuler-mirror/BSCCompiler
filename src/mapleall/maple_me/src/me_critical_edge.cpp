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
#include "me_critical_edge.h"
#include <iostream>
#include "me_cfg.h"
#include "me_option.h"
#include "me_dominance.h"
#include "me_function.h"
#include "me_loop_analysis.h"

// This phase finds critical edges and split them into two, because their
// presence would restrict the optimizations performed by SSAPRE-based phases.
// An edge is a critical edge when its pred has more than one succ and its succ
// has more than one pred
//     pred
//    /    \
//  newbb   \   <-- newbb (newbb is an empty bb but may carry a label)
//  \ /      \
//  succ
//
// newbb is always appended at the end of bb_vec_ and pred/succ will be updated.
// The bblayout phase will determine the final layout order of the bbs.
namespace maple {
// This function is used to find the eh edge when predBB is tryBB.
void MeSplitCEdge::UpdateNewBBInTry(MeFunction &func, BB &newBB, const BB &pred) const {
  auto tryBB = &pred;
  newBB.SetAttributes(kBBAttrIsTry);
  // The bb which is returnBB and only have return stmt would not have eh edge,
  // so need find a try bb which have eh edge continue.
  if (pred.IsReturnBB()) {
    int i = 1;
    tryBB = func.GetCfg()->GetBBFromID(newBB.GetBBId() - i);
    while (tryBB == nullptr || tryBB->IsReturnBB()) {
      ++i;
      tryBB = func.GetCfg()->GetBBFromID(newBB.GetBBId() - i);
    }
    ASSERT_NOT_NULL(tryBB);
    ASSERT(tryBB->GetAttributes(kBBAttrIsTry), "must be try");
  }
  bool setEHEdge = false;
  for (auto *candCatch : tryBB->GetSucc()) {
    if (candCatch != nullptr && candCatch->GetAttributes(kBBAttrIsCatch)) {
      setEHEdge = true;
      newBB.AddSucc(*candCatch);
    }
  }
  ASSERT(setEHEdge, "must set eh edge");
}

void MeSplitCEdge::UpdateGotoLabel(BB &newBB, MeFunction &func, BB &pred, BB &succ) const {
  auto &gotoStmt = static_cast<CondGotoNode&>(pred.GetStmtNodes().back());
  BB *gotoBB = pred.GetSucc().at(1);
  LabelIdx oldLabelIdx = gotoStmt.GetOffset();
  if (oldLabelIdx != gotoBB->GetBBLabel()) {
    // original gotoBB is replaced by newBB
    LabelIdx label = func.GetOrCreateBBLabel(*gotoBB);
    gotoStmt.SetOffset(label);
  }
  if (isDebugFunc) {
    LogInfo::MapleLogger() << "******after break: dump updated condgoto_BB *****\n";
    pred.Dump(&func.GetMIRModule());
    newBB.Dump(&func.GetMIRModule());
    succ.Dump(&func.GetMIRModule());
  }
}

void MeSplitCEdge::UpdateCaseLabel(BB &newBB, MeFunction &func, BB &pred, BB &succ) const {
  auto &switchStmt = static_cast<SwitchNode&>(pred.GetStmtNodes().back());
  LabelIdx oldLabelIdx = succ.GetBBLabel();
  LabelIdx label = func.GetOrCreateBBLabel(newBB);
  if (switchStmt.GetDefaultLabel() == oldLabelIdx) {
    switchStmt.SetDefaultLabel(label);
  }
  for (size_t i = 0; i < switchStmt.GetSwitchTable().size(); ++i) {
    LabelIdx labelIdx = switchStmt.GetCasePair(i).second;
    if (labelIdx == oldLabelIdx) {
      switchStmt.UpdateCaseLabelAt(i, label);
    }
  }
  if (isDebugFunc) {
    LogInfo::MapleLogger() << "******after break: dump updated switchBB *****\n";
    pred.Dump(&func.GetMIRModule());
    newBB.Dump(&func.GetMIRModule());
    succ.Dump(&func.GetMIRModule());
  }
}

void MeSplitCEdge::DealWithTryBB(MeFunction &func, BB &pred, BB &succ, BB *&newBB, bool &isInsertAfterPred) const {
  if (!succ.GetStmtNodes().empty() && succ.GetStmtNodes().front().GetOpCode() == OP_try) {
    newBB = &func.GetCfg()->InsertNewBasicBlock(pred, false);
    isInsertAfterPred = true;
    if (pred.GetAttributes(kBBAttrIsTryEnd)) {
      newBB->SetAttributes(kBBAttrIsTryEnd);
      pred.ClearAttributes(kBBAttrIsTryEnd);
      func.GetCfg()->SetTryBBByOtherEndTryBB(newBB, &pred);
    }
  } else {
    newBB = &func.GetCfg()->InsertNewBasicBlock(succ);
    isInsertAfterPred = false;
  }
}

void MeSplitCEdge::BreakCriticalEdge(MeFunction &func, BB &pred, BB &succ) const {
  if (isDebugFunc) {
    LogInfo::MapleLogger() << "******before break : critical edge : BB" << pred.GetBBId() << " -> BB" <<
        succ.GetBBId() << "\n";
    pred.Dump(&func.GetMIRModule());
    succ.Dump(&func.GetMIRModule());
  }
  ASSERT(!succ.GetAttributes(kBBAttrIsCatch), "BreakCriticalEdge: cannot break an EH edge");
  // create newBB and set pred/succ
  BB *newBB = nullptr;
  MeCFG *cfg = func.GetCfg();
  // use replace instead of remove/add to keep position in pred/succ
  bool isInsertAfterPred = false;
  bool needUpdateTryAttr = true;
  size_t index = succ.GetPred().size();
  if (&pred == cfg->GetCommonEntryBB()) {
    newBB = &cfg->InsertNewBasicBlock(*cfg->GetFirstBB());
    newBB->SetAttributes(kBBAttrIsEntry);
    succ.ClearAttributes(kBBAttrIsEntry);
    pred.RemoveEntry(succ);
    pred.AddEntry(*newBB);
    needUpdateTryAttr = false;
  } else {
    if (pred.GetAttributes(kBBAttrIsTry)) {
      DealWithTryBB(func, pred, succ, newBB, isInsertAfterPred);
    } else {
      newBB = cfg->NewBasicBlock();
      needUpdateTryAttr = false;
    }
    while (index > 0) {
      if (succ.GetPred(index - 1) == &pred) {
        break;
      }
      index--;
    }
    pred.ReplaceSucc(&succ, newBB);
  }
  // pred has been remove for pred vector of succ
  // means size reduced, so index reduced
  index--;
  succ.AddPred(*newBB, index);
  newBB->SetKind(kBBFallthru);  // default kind
  newBB->SetAttributes(kBBAttrArtificial);

  if (needUpdateTryAttr && isInsertAfterPred && pred.GetAttributes(kBBAttrIsTry)) {
    UpdateNewBBInTry(func, *newBB, pred);
  }
  if (needUpdateTryAttr && !isInsertAfterPred && succ.GetAttributes(kBBAttrIsTry) &&
      (succ.GetStmtNodes().empty() || succ.GetStmtNodes().front().GetOpCode() != OP_try)) {
    UpdateNewBBInTry(func, *newBB, succ);
  }
  // update statement offset if succ is goto target
  if (pred.GetKind() == kBBCondGoto) {
    UpdateGotoLabel(*newBB, func, pred, succ);
  } else if (pred.GetKind() == kBBSwitch) {
    UpdateCaseLabel(*newBB, func, pred, succ);
  }
}

void ScanEdgeInFunc(MeCFG &cfg, std::vector<std::pair<BB*, BB*>> &criticalEdge) {
  auto eIt = cfg.valid_end();
  for (auto bIt = cfg.valid_begin(); bIt != eIt; ++bIt) {
    if (bIt == cfg.common_exit()) {
      continue;
    }
    auto *bb = *bIt;
    const MapleVector<BB*> &preds = bb->GetPred();
    // skip fallthrough bb or bb is handler block
    if (preds.size() < 2 || bb->GetAttributes(kBBAttrIsCatch)) {
      continue;
    }
    // current BB is a merge
    for (BB *pred : preds) {
      if (pred->GetKind() == kBBGoto || pred->GetKind() == kBBIgoto) {
        continue;
      }
      if (pred->GetSucc().size() > 1) {
        // pred has more than one succ
        criticalEdge.push_back(std::make_pair(pred, bb));
      }
    }
  }
  // separate treatment for commonEntryBB's succ BBs
  for (BB *entryBB : cfg.GetCommonEntryBB()->GetSucc()) {
    if (!entryBB->GetPred().empty()) {
      criticalEdge.push_back(std::make_pair(cfg.GetCommonEntryBB(), entryBB));
    }
  }
}

void MESplitCEdge::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEMeCfg>();
  aDep.PreservedAllExcept<MEDominance>();
  aDep.PreservedAllExcept<MELoopAnalysis>();
}

bool MESplitCEdge::PhaseRun(maple::MeFunction &f) {
  MeCFG *cfg = GET_ANALYSIS(MEMeCfg, f);
  std::vector<std::pair<BB*, BB*>> criticalEdge;
  ScanEdgeInFunc(*cfg, criticalEdge);
  if (!criticalEdge.empty()) {
    bool enableDebug = DEBUGFUNC_NEWPM(f);
    if (enableDebug) {
      LogInfo::MapleLogger() << "*******************before break dump function*****************\n";
      f.DumpFunctionNoSSA();
      f.GetCfg()->DumpToFile("cfgbeforebreak");
    }
    MeSplitCEdge mscedge = MeSplitCEdge(enableDebug);
    for (auto it = criticalEdge.begin(); it != criticalEdge.end(); ++it) {
      mscedge.BreakCriticalEdge(f, *((*it).first), *((*it).second));
    }
    if (enableDebug) {
      LogInfo::MapleLogger() << "******************after break dump function******************\n";
      f.Dump(true);
      f.GetCfg()->DumpToFile("cfgafterbreak");
    }
  }
  return false;
}
}  // namespace maple
