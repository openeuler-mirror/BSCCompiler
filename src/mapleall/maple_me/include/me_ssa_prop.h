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
#ifndef MAPLE_ME_INCLUDE_ME_PROP_LOCAL_H
#define MAPLE_ME_INCLUDE_ME_PROP_LOCAL_H
#include "maple_phase_manager.h"
#include "me_toplevel_ssa.h"
namespace maple {
class SSAProp {
 public:
  SSAProp(MeFunction *f, Dominance *d);
  ~SSAProp() = default;
  void TraverseStmt(StmtNode *stmt, BB *currBB);
  void TraversalBB(BB &bb);
  void PropInFunc();

 private:
  void UpdateDef(VersionSt *vst);
  void UpdateDefOfMayDef(const StmtNode &stmt);
  BaseNode *CheckTruncation(BaseNode *lhs, BaseNode *rhs) const;
  bool CanOstProped(OriginalSt *ost) const;
  bool Propagatable(BaseNode *rhs) const;
  BaseNode *PropPhi(PhiNode *phi);
  BaseNode *PropVst(VersionSt *vst, bool checkPhi);
  BaseNode *PropExpr(BaseNode *expr, bool &subProp);

  BaseNode *SimplifyIreadAddrof(IreadSSANode *ireadSSA) const;
  BaseNode *SimplifyExpr(BaseNode *expr) const;
  StmtNode *SimplifyIassignAddrof(IassignNode *iassign, BB *bb) const;
  StmtNode *SimplifyStmt(StmtNode *stmt, BB *bb) const;
  MeFunction *func = nullptr;
  Dominance *dom = nullptr;
  SSATab *ssaTab = nullptr;
  std::vector<std::stack<VersionSt*>> vstLiveStack;
};

MAPLE_FUNC_PHASE_DECLARE(MESSAProp, MeFunction)
} // namespace maple
#endif // MAPLE_ME_INCLUDE_ME_PROP_LOCAL_H
