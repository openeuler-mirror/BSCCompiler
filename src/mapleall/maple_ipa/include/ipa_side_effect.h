/*
 * Copyright (c) [2021-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_IPA_INCLUDE_IPASIDEEFFECT_H
#define MAPLE_IPA_INCLUDE_IPASIDEEFFECT_H
#include "me_phase_manager.h"
#include "ipa_phase_manager.h"

namespace maple {
class SideEffect {
 public:
  SideEffect(MeFunction *meFunc, Dominance *dom, AliasClass *alias, CallGraph *cg)
      : meFunc(meFunc), dom(dom), alias(alias), callGraph(cg) {
    defGlobal = false;
    defArg = false;
    useGlobal = false;
    vstsValueAliasWithFormal.resize(std::min(meFunc->GetMirFunc()->GetFormalCount(), kMaxParamCount));
  }
  ~SideEffect() {
    alias = nullptr;
    dom = nullptr;
    meFunc = nullptr;
    curFuncDesc = nullptr;
  }
  bool Perform(MeFunction &f);
  static const FuncDesc &GetFuncDesc(MeFunction &f);
  static const FuncDesc &GetFuncDesc(MIRFunction &f);
  static const std::map<std::string, FuncDesc> &GetWhiteList();

 private:
  void DealWithOperand(MeExpr *expr);
  void DealWithOst(OStIdx ostIdx);
  void DealWithStmt(MeStmt &stmt);
  void PropAllInfoFromCallee(const MeStmt &call, MIRFunction &callee);
  void PropParamInfoFromCallee(const MeStmt &call, MIRFunction &callee);
  void PropInfoFromOpnd(MeExpr &opnd, const PI &calleeParamInfo);
  void ParamInfoUpdater(size_t vstIdx, const PI &calleeParamInfo);
  void DealWithOst(const OriginalSt *ost);
  void DealWithReturn(const RetMeStmt &retMeStmt) const;
  void AnalysisFormalOst();
  void SolveVarArgs(MeFunction &f) const;
  void CollectFormalOst(MeFunction &f);
  void CollectAllLevelOst(size_t vstIdx, std::set<size_t> &result);

  std::set<std::pair<OriginalSt*, size_t>> analysisLater;
  std::vector<std::set<size_t>> vstsValueAliasWithFormal;
  MeFunction *meFunc = nullptr;
  FuncDesc *curFuncDesc = nullptr;
  Dominance *dom = nullptr;
  AliasClass *alias = nullptr;
  CallGraph *callGraph = nullptr;

  bool defGlobal = false;
  bool defArg = false;
  bool useGlobal = false;
};

MAPLE_SCC_PHASE_DECLARE_BEGIN(SCCSideEffect, SCCNode<CGNode>)
OVERRIDE_DEPENDENCE
MAPLE_SCC_PHASE_DECLARE_END
}  // namespace maple
#endif  // MAPLE_IPA_INCLUDE_IPASIDEEFFECT_H
