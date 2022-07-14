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
#ifndef MAPLE_IPA_INCLUDE_COLLECT_IPA_INFO_H
#define MAPLE_IPA_INCLUDE_COLLECT_IPA_INFO_H
#include "mir_nodes.h"
#include "mir_builder.h"
#include "call_graph.h"
#include "me_ir.h"
#include "me_irmap.h"
#include "dominance.h"
#include "class_hierarchy.h"
#include "maple_phase.h"
#include "ipa_phase_manager.h"
#include "mir_module.h"
namespace maple {
union ParamValue {
  bool valueBool;
  int64_t valueInt;
  float valueFloat;
  double valueDouble;
};

enum ValueType {
  kBool,
  kInt,
  kFloat,
  kDouble,
};

class CollectIpaInfo {
 public:
  CollectIpaInfo(MIRModule &mod, AnalysisDataManager &dataMap)
      : module(mod), builder(*mod.GetMIRBuilder()),
        dataMap(dataMap), curFunc(nullptr) {}
  virtual ~CollectIpaInfo() = default;
  void RunOnScc(maple::SCCNode<CGNode> &scc);
  void UpdateCaleeParaAboutFloat(MeStmt &meStmt, float paramValue, uint32 index, CallerSummary &summary);
  void UpdateCaleeParaAboutDouble(MeStmt &meStmt, double paramValue, uint32 index, CallerSummary &summary);
  void UpdateCaleeParaAboutInt(MeStmt &meStmt, int64_t paramValue, uint32 index, CallerSummary &summary);
  bool IsConstKindValue(MeExpr *expr);
  bool CheckImpExprStmt(const MeStmt &meStmt);
  bool CollectImportantExpression(const MeStmt &meStmt, uint32 &index);
  void TraversalMeStmt(MeStmt &meStmt);
  bool IsParameterOrUseParameter(const VarMeExpr *varExpr, uint32 &index);
  void Perform(const MeFunction &func);

 private:
  MIRModule &module;
  MIRBuilder &builder;
  AnalysisDataManager &dataMap;
  MIRFunction *curFunc;
};
MAPLE_SCC_PHASE_DECLARE_BEGIN(SCCCollectIpaInfo, maple::SCCNode<CGNode>)
OVERRIDE_DEPENDENCE
MAPLE_SCC_PHASE_DECLARE_END
}  // namespace maple
#endif  // MAPLE_IPA_INCLUDE_COLLECT_IPA_INFO_H
