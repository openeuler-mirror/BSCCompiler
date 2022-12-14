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
constexpr uint32 kNumOfImpExprUpper = 64;
class IpaClone : public AnalysisResult {
 public:
  IpaClone(MIRModule *mod, MemPool *memPool, MIRBuilder &builder)
      : AnalysisResult(memPool), mirModule(mod), allocator(memPool), mirBuilder(builder), curFunc(nullptr) {}
  ~IpaClone() override {
    mirModule = nullptr;
    curFunc = nullptr;
  }

  static MIRSymbol *IpaCloneLocalSymbol(const MIRSymbol &oldSym, const MIRFunction &newFunc);
  static void IpaCloneSymbols(MIRFunction &newFunc, const MIRFunction &oldFunc);
  static void IpaCloneLabels(MIRFunction &newFunc, const MIRFunction &oldFunc);
  static void IpaClonePregTable(MIRFunction &newFunc, const MIRFunction &oldFunc);
  MIRFunction *IpaCloneFunction(MIRFunction &originalFunction, const std::string &fullName) const;
  MIRFunction *IpaCloneFunctionWithFreq(MIRFunction &originalFunction,
                                        const std::string &fullName, FreqType callSiteFreq) const;
  bool IsBrCondOrIf(Opcode op) const;
  void DoIpaClone();
  void InitParams();
  void CopyFuncInfo(MIRFunction &originalFunction, MIRFunction &newFunc) const;
  void IpaCloneArgument(MIRFunction &originalFunction, ArgVector &argument) const;
  void RemoveUnneedParameter(MIRFunction *newFunc, uint32 paramIndex, int64_t value) const;
  void DecideCloneFunction(std::vector<ImpExpr> &result, uint32 paramIndex, std::map<uint32,
                           std::vector<int64_t>> &evalMap) const;
  void ReplaceIfCondtion(MIRFunction *newFunc, std::vector<ImpExpr> &result, uint64_t res) const;
  void RemoveSwitchCase(MIRFunction &newFunc, SwitchNode &switchStmt, std::vector<int64_t> &calleeValue) const;
  void RemoveUnneedSwitchCase(MIRFunction &newFunc, std::vector<ImpExpr> &result,
                              std::vector<int64_t> &calleeValue) const;
  bool CheckImportantExprHasBr(const std::vector<ImpExpr> &exprVec) const;
  void EvalCompareResult(std::vector<ImpExpr> &result, std::map<uint32, std::vector<int64_t>> &evalMap,
                         std::map<int64_t, std::vector<CallerSummary>> &summary, uint32 index) const;
  void EvalImportantExpression(MIRFunction *func, std::vector<ImpExpr> &result);
  bool CheckCostModel(uint32 paramIndex, std::vector<int64_t> &calleeValue, std::vector<ImpExpr> &result) const;
  void ComupteValue(const IntVal& value, const IntVal& paramValue, const CompareNode &cond, uint64_t &bitRes) const;
  void CloneNoImportantExpressFunction(MIRFunction *func, uint32 paramIndex) const;
  void ModifyParameterSideEffect(MIRFunction *newFunc, uint32 paramIndex) const;

 private:
  MIRModule *mirModule;
  MapleAllocator allocator;
  MIRBuilder &mirBuilder;
  MIRFunction *curFunc;
  uint32 numOfCloneVersions = 0;
  uint32 numOfImpExprLowBound = 0;
  uint32 numOfImpExprHighBound = 0;
  uint32 numOfCallSiteLowBound = 0;
  uint32 numOfCallSiteUpBound = 0;
  uint32 numOfConstpropValue = 0;
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
