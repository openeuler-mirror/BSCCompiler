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
#include "ssa_pre.h"

namespace maple {
class SSAEPre : public SSAPre {
 public:
  SSAEPre(IRMap &map, Dominance &dom, MemPool &memPool, MemPool &mp2, PreKind kind,
          uint32 limit, bool includeRef, bool lhsIvar)
      : SSAPre(map, dom, memPool, mp2, kind, limit), epreIncludeRef(includeRef), enableLHSIvar(lhsIvar) {}

  virtual ~SSAEPre() = default;

 private:
  void GenerateSaveLHSRealocc(MeRealOcc &realOcc, ScalarMeExpr &regOrVar);
  void GenerateSaveRealOcc(MeRealOcc &realOcc);
  bool ReserveCalFuncAddrForDecouple(MeExpr &meExpr) const;
  void GenerateReloadRealOcc(MeRealOcc &realOcc);
  MeExpr *PhiOpndFromRes(MeRealOcc &realZ, size_t j) const;
  void ComputeVarAndDfPhis();
  void BuildWorkListExpr(MeStmt &meStmt, int32 seqStmt, MeExpr&, bool isReBuild, MeExpr *tempVar, bool isRootExpr);
  void BuildWorkListIvarLHSOcc(MeStmt &meStmt, int32 seqStmt, bool isReBuild, MeExpr *tempVar);
  void CollectVarForMeExpr(MeExpr &meExpr, std::vector<MeExpr*> &varVec) const;
  void CollectVarForCand(MeRealOcc &realOcc, std::vector<MeExpr*> &varVec) const;
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

  bool epreIncludeRef;
  bool enableLHSIvar;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_SSAEPRE_H
