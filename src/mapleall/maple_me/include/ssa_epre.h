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
#ifndef MAPLE_ME_INCLUDE_SSAEPRE_H
#define MAPLE_ME_INCLUDE_SSAEPRE_H
#include "mc_ssa_pre.h"

namespace maple {
class SSAEPre : public McSSAPre {
 public:
  SSAEPre(IRMap &map, Dominance &dom, Dominance &pdom, MemPool &memPool, MemPool &mp2, PreKind kind,
          uint32 limit, bool includeRef, bool lhsIvar)
      : McSSAPre(map, dom, pdom, memPool, mp2, kind, limit), epreIncludeRef(includeRef), enableLHSIvar(lhsIvar) {}

  virtual ~SSAEPre() = default;

 private:
  MeExpr *GetTruncExpr(const IvarMeExpr &theLHS, MeExpr &savedRHS) const;
  void GenerateSaveLHSRealocc(MeRealOcc &realOcc, ScalarMeExpr &regOrVar);
  void GenerateSaveRealOcc(MeRealOcc &realOcc) override;
  bool ReserveCalFuncAddrForDecouple(MeExpr &meExpr) const;
  void GenerateReloadRealOcc(MeRealOcc &realOcc) override;
  MeExpr *PhiOpndFromRes(MeRealOcc &realZ, size_t j) const override;
  void ComputeVarAndDfPhis() override;
  void BuildWorkListExpr(MeStmt &meStmt, int32 seqStmt, MeExpr &meExpr, bool isRebuild,
                         MeExpr *tempVar, bool isRootExpr, bool insertSorted) override;
  void BuildWorkListIvarLHSOcc(MeStmt &meStmt, int32 seqStmt, bool isReBuild, MeExpr *tempVar) override;
  void CollectVarForMeExpr(MeExpr &meExpr, std::vector<MeExpr*> &varVec) const override;
  void CollectVarForCand(MeRealOcc &realOcc, std::vector<MeExpr*> &varVec) const override;
  bool LeafIsVolatile(const MeExpr *x) const {
    const VarMeExpr *v = safe_cast<VarMeExpr>(x);
    return v != nullptr && v->IsVolatile();
  }
  virtual bool IsThreadObjField(const IvarMeExpr &expr) const {
    (void)expr;
    return false;
  }

  virtual bool CfgHasDoWhile() const {
    return false;
  }
  // here starts methods related to strength reduction
  bool AllVarsSameVersion(const MeRealOcc &realocc1, const MeRealOcc &realocc2) const override;
  ScalarMeExpr *ResolveOneInjuringDef(ScalarMeExpr *regx) const;
  ScalarMeExpr *ResolveAllInjuringDefs(ScalarMeExpr *regx) const override;
  MeExpr *ResolveAllInjuringDefs(MeExpr *x) const override {
    if (!workCand->isSRCand) {
      return x;
    }
    if (x->GetMeOp() != kMeOpVar && x->GetMeOp() != kMeOpReg) {
      return x;
    }
    return static_cast<MeExpr *>(ResolveAllInjuringDefs(static_cast<ScalarMeExpr *>(x)));
  }
  void SubstituteOpnd(MeExpr *x, MeExpr *oldopnd, MeExpr *newopnd) override;
  bool OpndInDefOcc(const MeExpr *opnd, MeOccur *defocc, uint32 i) const;
  void SRSetNeedRepair(MeOccur *useocc, std::set<MeStmt *> *needRepairInjuringDefs) override;
  MeExpr *InsertRepairStmt(MeExpr *temp, int64 increAmt, MeStmt *injuringDef) const;
  MeExpr *SRRepairOpndInjuries(MeExpr *curopnd, MeOccur *defocc, int32 i,
      MeExpr *tempAtDef, std::set<MeStmt *> *needRepairInjuringDefs,
      std::set<MeStmt *> *repairedInjuringDefs);
  MeExpr *SRRepairInjuries(MeOccur *useocc,
      std::set<MeStmt *> *needRepairInjuringDefs,
      std::set<MeStmt *> *repairedInjuringDefs) override;

  bool epreIncludeRef;
  bool enableLHSIvar;
  // here starts methods related to linear function test replacement
  ScalarMeExpr *FindScalarVersion(ScalarMeExpr *scalar, MeStmt *stmt) const;
  OpMeExpr *FormLFTRCompare(MeRealOcc *compOcc, MeExpr *regorvar) override;
  void CreateCompOcc(MeStmt *meStmt, int seqStmt, OpMeExpr *compare, bool isRebuilt) override;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_SSAEPRE_H
