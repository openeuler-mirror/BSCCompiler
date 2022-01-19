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
#ifndef HIR2MPL_INCLUDE_COMMON_JBC_FUNCTION_H
#define HIR2MPL_INCLUDE_COMMON_JBC_FUNCTION_H
#include <map>
#include <vector>
#include <set>
#include <deque>
#include <tuple>
#include "fe_configs.h"
#include "fe_function.h"
#include "jbc_class.h"
#include "jbc_attr.h"
#include "jbc_stmt.h"
#include "jbc_bb.h"
#include "jbc_class2fe_helper.h"
#include "jbc_stack2fe_helper.h"
#include "jbc_function_context.h"

namespace maple {
class JBCBBPesudoCatchPred : public FEIRBB {
 public:
  static const uint8 kBBKindPesudoCatchPred = FEIRBBKind::kBBKindExt + 1;
  JBCBBPesudoCatchPred()
      : FEIRBB(kBBKindPesudoCatchPred) {}
  ~JBCBBPesudoCatchPred() = default;
};  // class JBCBBPesudoCatchPred

class JBCFunction : public FEFunction {
 public:
  JBCFunction(const JBCClassMethod2FEHelper &argMethodHelper, MIRFunction &mirFunc,
              const std::unique_ptr<FEFunctionPhaseResult> &argPhaseResultTotal);
  ~JBCFunction();

 LLT_PROTECTED:
  // run phase routines
  bool GenerateGeneralStmt(const std::string &phaseName) override;
  bool LabelLabelIdx(const std::string &phaseName);
  bool CheckJVMStack(const std::string &phaseName);
  bool GenerateArgVarList(const std::string &phaseName) override;
  bool GenerateAliasVars(const std::string &phaseName) override;
  bool ProcessFunctionArgs(const std::string &phaseName);
  bool EmitLocalVarInfo(const std::string &phaseName);
  bool EmitToFEIRStmt(const std::string &phaseName) override;

  // interface implement
  void InitImpl() override;
  void PreProcessImpl() override;
  bool ProcessImpl() override;
  void FinishImpl() override;
  bool PreProcessTypeNameIdx() override;
  void GenerateGeneralStmtFailCallBack() override;
  void GenerateGeneralDebugInfo() override;
  bool VerifyGeneral() override;
  void VerifyGeneralFailCallBack() override;
  std::string GetGeneralFuncName() const override;

  bool HasThis() override {
    return methodHelper.HasThis();
  }

  bool IsNative() override {
    return methodHelper.IsNative();
  }

  void EmitToFEIRStmt(const JBCBB &bb);

 LLT_PRIVATE:
  const JBCClassMethod2FEHelper &methodHelper;
  const jbc::JBCClassMethod &method;
  JBCStack2FEHelper stack2feHelper;
  JBCFunctionContext context;
  bool error = false;
  FEIRBB *pesudoBBCatchPred = nullptr;

  bool PreBuildJsrInfo(const jbc::JBCAttrCode &code);
  bool BuildStmtFromInstruction(const jbc::JBCAttrCode &code);
  FEIRStmt *BuildStmtFromInstructionForBranch(const jbc::JBCOp &op);
  FEIRStmt *BuildStmtFromInstructionForGoto(const jbc::JBCOp &op);
  FEIRStmt *BuildStmtFromInstructionForSwitch(const jbc::JBCOp &op);
  FEIRStmt *BuildStmtFromInstructionForJsr(const jbc::JBCOp &op);
  FEIRStmt *BuildStmtFromInstructionForRet(const jbc::JBCOp &op);
  void BuildStmtForCatch(const jbc::JBCAttrCode &code);
  void BuildStmtForTry(const jbc::JBCAttrCode &code);
  void BuildTryInfo(const std::map<std::pair<uint32, uint32>, std::vector<uint32>> &rawInfo,
                    std::map<uint32, uint32> &outMapStartEnd,
                    std::map<uint32, std::vector<uint32>> &outMapStartCatch);
  void BuildTryInfoCatch(const std::map<std::pair<uint32, uint32>, std::vector<uint32>> &rawInfo,
                         const std::deque<std::pair<uint32, uint32>> &blockQueue,
                         uint32 startPos,
                         std::map<uint32, std::vector<uint32>> &outMapStartCatch);
  void BuildStmtForLOC(const jbc::JBCAttrCode &code);
  void BuildStmtForInstComment(const jbc::JBCAttrCode &code);
  FEIRStmt *BuildAndUpdateLabel(uint32 dstPC, const std::unique_ptr<FEIRStmt> &srcStmt);
  void ArrangeStmts();
  bool CheckJVMStackResult();
  void InitStack2FEHelper();
  uint32 CalculateMaxSwapSize() const;
  bool NeedConvertToInt32(const std::unique_ptr<FEIRVar> &var);
};  // class JBCFunction
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_COMMON_JBC_FUNCTION_H
