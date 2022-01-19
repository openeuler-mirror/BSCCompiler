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
#ifndef MPL_FE_BC_INPUT_BC_FUNTION_H
#define MPL_FE_BC_INPUT_BC_FUNTION_H
#include "fe_function.h"
#include "bc_class2fe_helper.h"

namespace maple {
namespace bc {
class BCFunction : public FEFunction {
 public:
  BCFunction(const BCClassMethod2FEHelper &argMethodHelper, MIRFunction &mirFunc,
              const std::unique_ptr<FEFunctionPhaseResult> &argPhaseResultTotal);
  virtual ~BCFunction() = default;

 protected:
  bool GenerateGeneralStmt(const std::string &phaseName) override {
    WARN(kLncWarn, "Phase: %s may not need.", phaseName.c_str());
    return true;
  }

  bool GenerateArgVarList(const std::string &phaseName) override;
  bool GenerateAliasVars(const std::string &phaseName) override;

  bool PreProcessTypeNameIdx() override {
    return true;
  }

  void GenerateGeneralStmtFailCallBack() override {}

  void GenerateGeneralDebugInfo() override {}

  bool VerifyGeneral() override {
    return true;
  }

  void VerifyGeneralFailCallBack() override {}

  bool HasThis() override {
    return methodHelper.HasThis();
  }

  bool IsNative() override {
    return methodHelper.IsNative();
  }

  bool EmitToFEIRStmt(const std::string &phaseName) override;

  void InitImpl() override;
  void SetMIRFunctionInfo();
  void PreProcessImpl() override;
  bool ProcessImpl() override;
  bool ProcessFEIRFunction() override;
  void FinishImpl() override;
  bool EmitToMIR(const std::string &phaseName) override;

  const BCClassMethod2FEHelper &methodHelper;
  std::unique_ptr<BCClassMethod> &method;
  bool error = false;
};
}  // namespace bc
}  // namespace maple
#endif