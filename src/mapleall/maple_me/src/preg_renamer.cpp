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
#include "alias_class.h"
#include "mir_builder.h"
#include "me_irmap_build.h"
#include "union_find.h"
#include "me_verify.h"
#include "me_dominance.h"
#include "me_phase_manager.h"
#include "preg_renamer.h"

namespace maple {
void PregRenamer::RunSelf() {
  // BFS the graph of register phi node;
  const MapleVector<MeExpr *> &regMeExprTable = meirmap->GetVerst2MeExprTable();
  MIRPregTable *pregTab = func->GetMirFunc()->GetPregTab();
  std::vector<bool> firstAppearTable(pregTab->GetPregTable().size());
  UnionFind unionFind(*mp, regMeExprTable.size());
  auto *cfg = func->GetCfg();
  // iterate all the bbs' phi to setup the union
  for (BB *bb : cfg->GetAllBBs()) {
    if (bb == nullptr || bb == cfg->GetCommonEntryBB() || bb == cfg->GetCommonExitBB()) {
      continue;
    }
    MapleMap<OStIdx, MePhiNode *> &mePhiList =  bb->GetMePhiList();
    for (auto it = mePhiList.cbegin(); it != mePhiList.cend(); ++it) {
      if (!it->second->GetIsLive()) {
        continue;
      }
      OriginalSt *ost = func->GetMeSSATab()->GetOriginalStTable().GetOriginalStFromID(it->first);
      ASSERT_NOT_NULL(ost);
      if (!ost->IsPregOst()) { // only handle reg phi
        continue;
      }
      if (ost->GetIndirectLev() != 0) {
        continue;
      }
      MePhiNode *meRegPhi = it->second;
      size_t vstIdx = meRegPhi->GetLHS()->GetVstIdx();
      size_t nOpnds = meRegPhi->GetOpnds().size();
      for (size_t i = 0; i < nOpnds; ++i) {
        unionFind.Union(vstIdx, meRegPhi->GetOpnd(i)->GetVstIdx());
      }
    }
  }
  std::map<uint32, std::vector<uint32> > root2childrenMap;
  for (uint32 i = 0; i < regMeExprTable.size(); ++i) {
    MeExpr *meExpr = regMeExprTable[i];
    if (meExpr == nullptr || meExpr->GetMeOp() != kMeOpReg) {
      continue;
    }
    auto *regMeExpr = static_cast<RegMeExpr*>(meExpr);
    auto *defStmt = regMeExpr->GetDefByMeStmt();
    if (defStmt != nullptr && defStmt->GetOp() == OP_asm) {
      continue;  // it could be binded with other regs or vars in asm so keep them in original format
    }
    if (regMeExpr->GetRegIdx() < 0) {
      continue;  // special register
    }
    if (PrimitiveType(regMeExpr->GetPrimType()).IsVector()) {
      // can be removed after mplbe support this
      continue;
    }
    if (regMeExpr->GetOst()->GetIndirectLev() != 0) {
      continue;
    }
    uint32 rootVstIdx = unionFind.Root(i);

    auto mpIt = root2childrenMap.find(rootVstIdx);
    if (mpIt == root2childrenMap.end()) {
      std::vector<uint32> vec(1, i);
      root2childrenMap[rootVstIdx] = vec;
    } else {
      std::vector<uint32> &vec = mpIt->second;
      vec.push_back(i);
    }
  }
  for (auto it = root2childrenMap.begin(); it != root2childrenMap.end(); ++it) {
    std::vector<uint32> &vec = it->second;
    bool isIntryOrZerov = false; // in try block or zero version
    for (uint32 i = 0; i < vec.size(); ++i) {
      uint32 vstIdx = vec[i];
      ASSERT(vstIdx < regMeExprTable.size(), "over size");
      auto *tregMeExpr = static_cast<RegMeExpr*>(regMeExprTable[vstIdx]);
      if (tregMeExpr->GetDefBy() == kDefByNo ||
          (tregMeExpr->DefByBB() != nullptr && tregMeExpr->DefByBB()->GetAttributes(kBBAttrIsTry))) {
        isIntryOrZerov = true;
        break;
      }
    }
    if (isIntryOrZerov) {
      continue;
    }

    if (vec.size() == 1) { // if by itself, make sure it is live
      uint32 vstIdx = it->first;
      RegMeExpr *regMeExpr = static_cast<RegMeExpr*>(regMeExprTable[vstIdx]);
      if (regMeExpr->GetDefBy() == kDefByNo) {
        continue;  // will not rename if there is no def
      }
      if (regMeExpr->GetDefBy() == kDefByPhi) {
        MePhiNode *defPhi = &regMeExpr->GetDefPhi();
        if (!defPhi->GetIsLive()) {
          continue;
        }
      } else if (regMeExpr->GetDefBy() == kDefByStmt) {
        MeStmt *defStmt = regMeExpr->GetDefStmt();
        if (defStmt == nullptr || !defStmt->GetIsLive()) {
          continue;
        }
      } else if (regMeExpr->GetDefBy() == kDefByMustDef) {
        MustDefMeNode *mustDef = &regMeExpr->GetDefMustDef();
        if (!mustDef->GetIsLive()) {
          continue;
        }
      } else {
        continue;
      }
    }

    // get all the nodes in candidates the same register
    auto *regMeexpr = static_cast<RegMeExpr*>(regMeExprTable[it->first]);
    PregIdx oldPredIdx = regMeexpr->GetRegIdx();
    ASSERT(static_cast<uint32>(oldPredIdx) < firstAppearTable.size(), "oversize ");
    if (!firstAppearTable[static_cast<size_t>(static_cast<uint32>(oldPredIdx))]) {
      // use the previous register
      firstAppearTable[static_cast<uint32_t>(oldPredIdx)] = true;
      continue;
    }
    PregIdx newPregIdx = pregTab->ClonePreg(*pregTab->PregFromPregIdx(oldPredIdx));
    OriginalStTable &ostTab = func->GetMeSSATab()->GetOriginalStTable();
    // no need to find here, because the newPregIdx is the newest
    OriginalSt *newOst = ostTab.CreatePregOriginalSt(newPregIdx, func->GetMirFunc()->GetPuidx());
    MIRPreg *oldPreg = pregTab->PregFromPregIdx(oldPredIdx);
    if (oldPreg->GetOp() != OP_undef) {
      // carry over fields in MIRPreg to support rematerialization
      MIRPreg *newPreg = pregTab->PregFromPregIdx(newPregIdx);
      newPreg->SetOp(oldPreg->GetOp());
      newPreg->rematInfo = oldPreg->rematInfo;
      newPreg->fieldID = oldPreg->fieldID;
    }
    if (DEBUGFUNC(func)) {
      LogInfo::MapleLogger() << "%" << pregTab->PregFromPregIdx(regMeexpr->GetRegIdx())->GetPregNo();
      LogInfo::MapleLogger() << " renamed to %" << pregTab->PregFromPregIdx(newPregIdx)->GetPregNo() << std::endl;
    }
    // reneme all the register
    for (uint32 i = 0; i < vec.size(); ++i) {
      auto *canRegNode =  static_cast<RegMeExpr*>(regMeExprTable[vec[i]]);
      canRegNode->SetOst(newOst);  // rename it to a new register
      if (canRegNode->IsDefByPhi()) {
        // update philist key with new ost index
        MePhiNode &phiNode = canRegNode->GetDefPhi();
        BB *bb = phiNode.GetDefBB();
        OriginalSt *oldOst = ostTab.FindOrCreatePregOriginalSt(oldPredIdx, func->GetMirFunc()->GetPuidx());
        bb->GetMePhiList().erase(oldOst->GetIndex());
        bb->GetMePhiList().emplace(newOst->GetIndex(), &phiNode);
      }
    }
  }
}

void MEPregRename::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEIRMapBuild>();
  aDep.SetPreservedAll();
}

bool MEPregRename::PhaseRun(maple::MeFunction &f) {
  if (f.HasWriteInputAsmNode()) {
    if (!MeOption::quiet) {
      LogInfo::MapleLogger() << "  == " << PhaseName() << " skipped due to inline asm with wirting input operand\n";
    }
    return false;
  }
  auto *irMap = GET_ANALYSIS(MEIRMapBuild, f);
  PregRenamer pregRenamer(GetPhaseMemPool(), &f, irMap);
  pregRenamer.RunSelf();
  if (DEBUGFUNC_NEWPM(f)) {
    LogInfo::MapleLogger() << "------------after pregrename:-------------------\n";
    f.Dump(false);
  }
  if (MeOption::meVerify) {
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &MEDominance::id);
    auto *dom = FORCE_EXEC(MEDominance)->GetDomResult();
    ASSERT(dom != nullptr, "dominance phase has problem");
    MeVerify verify(f);
    for (auto &bb : f.GetCfg()->GetAllBBs()) {
      if (bb == nullptr) {
        continue;
      }
      verify.VerifyPhiNode(*bb, *dom);
    }
  }
  return true;
}
}  // namespace maple
