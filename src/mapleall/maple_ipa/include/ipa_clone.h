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
#ifndef MAPLE_IPA_INCLUDE_IPACLONE_H
#define MAPLE_IPA_INCLUDE_IPACLONE_H
#include "mir_module.h"
#include "mir_function.h"
#include "mir_builder.h"
#include "mempool.h"
#include "mempool_allocator.h"
#include "class_hierarchy_phase.h"
#include "me_ir.h"
#include "maple_phase_manager.h"
namespace maple {
constexpr uint32 kNumOfCloneVersions = 2;
constexpr uint32 kNumOfImpExprLowBound = 2;
constexpr uint32 kNumOfImpExprHighBound = 5;
constexpr uint32 kNumOfCallSiteLowBound = 2;
constexpr uint32 kNumOfCallSiteUpBound = 10;
constexpr uint32 kNumOfConstpropValue = 2;
constexpr uint32 kNumOfImpExprUpper = 64;

class IpaClone : public AnalysisResult {
 public:
  IpaClone(MIRModule *mod, MemPool *memPool, MIRBuilder &builder)
      : AnalysisResult(memPool), mirModule(mod), allocator(memPool), mirBuilder(builder), curFunc(nullptr) {}
  ~IpaClone() = default;

  static MIRSymbol *IpaCloneLocalSymbol(const MIRSymbol &oldSym, const MIRFunction &newFunc);
  static void IpaCloneSymbols(MIRFunction &newFunc, const MIRFunction &oldFunc);
  static void IpaCloneLabels(MIRFunction &newFunc, const MIRFunction &oldFunc);
  static void IpaClonePregTable(MIRFunction &newFunc, const MIRFunction &oldFunc);
  MIRFunction *IpaCloneFunction(MIRFunction &originalFunction, const std::string &newBaseFuncName) const;
  void DoIpaClone();
  void CopyFuncInfo(MIRFunction &originalFunction, MIRFunction &newFunc) const;
  void IpaCloneArgument(MIRFunction &originalFunction, ArgVector &argument) const;
  void RemoveUnneedParameter(MIRFunction *func, uint32 paramIndex, int64_t value);
  void DecideCloneFunction(std::vector<ImpExpr> &result, uint32 paramIndex, std::map<uint32,
                           std::vector<int64_t>> &evalMap);
  void ReplaceIfCondtion(MIRFunction *newFunc, std::vector<ImpExpr> &result, uint64_t res);
  void EvalCompareResult(std::vector<ImpExpr> &result, std::map<uint32, std::vector<int64_t>> &evalMap,
                         std::map<int64_t, std::vector<CallerSummary>> &summary, uint32 index);
  void EvalImportantExpression(MIRFunction *func, std::vector<ImpExpr> &result);
  bool CheckCostModel(MIRFunction *newFunc, uint32 paramIndex, std::vector<int64_t> &calleeValue, uint32 impSize);
  template<typename dataT>
  void ComupteValue(dataT value, dataT paramValue, CompareNode *cond, uint64_t &bitRes);
  void CloneNoImportantExpressFunction(MIRFunction *func, uint32 paramIndex);
  void ModifyParameterSideEffect(MIRFunction *newFunc, uint32 paramIndex);

 private:
  MIRModule *mirModule;
  MapleAllocator allocator;
  MIRBuilder &mirBuilder;
  MIRFunction *curFunc;
};
MAPLE_MODULE_PHASE_DECLARE_BEGIN(M2MIpaClone)
  IpaClone *GetResult() {
    return cl;
  }
  IpaClone *cl = nullptr;
OVERRIDE_DEPENDENCE
MAPLE_MODULE_PHASE_DECLARE_END
}  // namespace maple
#endif  // MAPLE_IPA_INCLUDE_IPACLONE_H
