/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *
 *     http://license.coscl.org.cn/MulanPSL
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v1 for more details.
 */
#ifndef MPL2MPL_INCLUDE_SIMPLIFY_H
#define MPL2MPL_INCLUDE_SIMPLIFY_H
#include "module_phase.h"
#include "phase_impl.h"
#include "factory.h"
namespace maple {
class Simplify : public FuncOptimizeImpl {
 public:
  Simplify(MIRModule &mod, KlassHierarchy *kh, bool dump) : FuncOptimizeImpl(mod, kh, dump), mirMod(mod) {
  }
  Simplify(const Simplify &other) = delete;
  Simplify &operator=(const Simplify &other) = delete;
  ~Simplify() = default;
  FuncOptimizeImpl *Clone() override {
    CHECK_FATAL(false, "Simplify has pointer, should not be Cloned");
  }

  void ProcessFunc(MIRFunction *func) override;
  void ProcessFuncStmt(MIRFunction &func, StmtNode *stmt = nullptr, BlockNode *block = nullptr);
  void Finish() override;

 private:
  MIRModule &mirMod;
  bool IsMathSqrt(const std::string funcName);
  bool IsMathAbs(const std::string funcName);
  bool IsMathMin(const std::string funcName);
  bool IsMathMax(const std::string funcName);
  bool SimplifyMathMethod(const StmtNode &stmt, BlockNode &block);
  void SimplifyCallAssigned(const StmtNode &stmt, BlockNode &block);
  StmtNode *SplitAggCopy(StmtNode *stmt, BlockNode *block, MIRFunction *func);
};
class DoSimplify : public ModulePhase {
 public:
  explicit DoSimplify(ModulePhaseID id) : ModulePhase(id) {}

  std::string PhaseName() const override {
    return "simplify";
  }

  AnalysisResult *Run(MIRModule *mod, ModuleResultMgr *mrm) override {
    OPT_TEMPLATE(Simplify);
    return nullptr;
  }

  ~DoSimplify() = default;
};
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_SIMPLIFY_H
