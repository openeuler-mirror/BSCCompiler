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
#ifndef MAPLE_ME_INCLUDE_MEPLACEMENTRC_H
#define MAPLE_ME_INCLUDE_MEPLACEMENTRC_H
#include "me_ssu_pre.h"

namespace maple {
class PlacementRC : public MeSSUPre {
 public:
  PlacementRC(MeFunction &f, Dominance &dom, MemPool &memPool, bool enabledDebug)
      : MeSSUPre(f, dom, memPool, kDecrefPre, enabledDebug), placementRCTemp(nullptr),
        bbHasReal(f.GetCfg()->GetAllBBs().size(), false, spreAllocator.Adapter()) {}

  virtual ~PlacementRC() = default;

 private:
  // Step 6 methods
  UnaryMeStmt *GenIncrefAtEntry(OriginalSt &ost) const;
  UnaryMeStmt *GenerateDecrefreset(OStIdx ostIdx, BB &bb) const;
  void CollectDecref(VarMeExpr *var, MeStmt *decrefStmt);
  void HandleThrowOperand(SRealOcc &realOcc, ThrowMeStmt &thwStmt);
  void CheckAndInsert(BB &bb, SOcc *realOcc);
  bool GoStraightToThrow(const BB &bb) const;
  void FindRealPredBB(const BB &bb, std::vector<bool> &visitedBBs, std::set<BBId> &lambdaRealSet) const;
  SOcc *FindLambdaReal(const SLambdaOcc &occ) const;
  void CodeMotion() override;
  // Step 0 methods
  void CreateEmptyCleanupIntrinsics() override;
  void GenEntryOcc4Formals();
  void CreatePhiOcc(const OStIdx &ostIdx, BB &bb, MePhiNode &phi, VarMeExpr &var);
  void CreateRealOcc(const OStIdx &ostIdx, MeStmt *meStmt, VarMeExpr &var, bool causedByDef = false);
  void CreateUseOcc(const OStIdx &ostIdx, BB &bb, VarMeExpr &var);
  void FindLocalrefvarUses(MeExpr &meExpr, MeStmt *meStmt);
  void BuildWorkListBB(BB *bb) override;
  void ResetHasReal();
  void UpdateHasRealWithRealOccs(const MapleVector<SOcc*> &realOccs);
  void SetFormalParamsAttr(MIRFunction &mirFunc) const;
  void DeleteEntryIncref(SRealOcc &realOcc, const UnaryMeStmt *entryIncref) const;
  void UpdateCatchBlocks2Insert(const BB &lastUseBB);
  void ReplaceOpndWithReg(MeExpr &opnd, BB &lastUseBB, const SRealOcc &realOcc, uint32 idx) const;
  void HandleCanInsertAfterStmt(const SRealOcc &realOcc, UnaryMeStmt &decrefStmt, BB &lastUseBB);
  void HandleCannotInsertAfterStmt(const SRealOcc &realOcc, UnaryMeStmt &decrefStmt, BB &lastUseBB);
  void CodeMotionForSLambdares(SLambdaResOcc &lambdaResOcc);
  void CodeMotionForReal(SOcc &occ, const UnaryMeStmt *entryIncref);
  void SetNeedIncref(MeStmt &stmt) const;
  void ReplaceRHSWithPregForDassign(MeStmt &stmt, BB &bb) const;
  bool DoesDassignInsertedForCallAssigned(MeStmt &stmt, BB &bb) const;
  void LookForRealOccOfStores(MeStmt &stmt, BB &bb);
  void LookForUseOccOfLocalRefVars(MeStmt &stmt);
  void TraverseStatementsBackwards(BB &bb);
  void AddCleanupArg();
  MeStmt *GetDefStmt(BB &bb);

  void PerCandInit() override {
    catchBlocks2Insert.clear();
  }

  // Step 6 variables
  VarMeExpr *placementRCTemp;  // localrefvar temp created to handle throw operand (always zero version)
  MapleVector<bool> bbHasReal;
};

class MeDoPlacementRC : public MeFuncPhase {
 public:
  explicit MeDoPlacementRC(MePhaseID id) : MeFuncPhase(id) {}

  virtual ~MeDoPlacementRC() = default;

  AnalysisResult *Run(MeFunction *ir, MeFuncResultMgr *m, ModuleResultMgr *mrm) override;

  std::string PhaseName() const override {
    return "placementrc";
  }
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MEPLACEMENTRC_H
