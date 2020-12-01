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
#ifndef MPLFE_INCLUDE_COMMON_JBC_FUNCTION_H
#define MPLFE_INCLUDE_COMMON_JBC_FUNCTION_H
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
class JBCBBPesudoCatchPred : public GeneralBB {
 public:
  static const uint8 kBBKindPesudoCatchPred = GeneralBBKind::kBBKindExt + 1;
  JBCBBPesudoCatchPred()
      : GeneralBB(kBBKindPesudoCatchPred) {}
  ~JBCBBPesudoCatchPred() = default;
};  // class JBCBBPesudoCatchPred

class JBCFunctionCFG : public GeneralCFG {
 public:
  JBCFunctionCFG(const jbc::JBCClassMethod &argMethod, const GeneralStmt &argStmtHead, const GeneralStmt &argStmtTail)
      : GeneralCFG(argStmtHead, argStmtTail),
        method(argMethod) {}

  ~JBCFunctionCFG() = default;

 protected:
  std::unique_ptr<GeneralBB> NewGeneralBBImpl() const override {
    return std::make_unique<JBCBB>(method.GetConstPool());
  }

 private:
  const jbc::JBCClassMethod &method;
};  // class JBCFunctionCFG

class JBCFunction : public FEFunction {
 public:
  JBCFunction(const JBCClassMethod2FEHelper &argMethodHelper, MIRFunction &mirFunc,
              const std::unique_ptr<FEFunctionPhaseResult> &argPhaseResultTotal);
  ~JBCFunction();

 LLT_PROTECTED:
  // run phase routines
  bool GenerateGeneralStmt(const std::string &phaseName) override;
  bool BuildGeneralBB(const std::string &phaseName) override;
  bool BuildGeneralCFG(const std::string &phaseName) override;
  bool LabelLabelIdx(const std::string &phaseName);
  bool CheckJVMStack(const std::string &phaseName);
  bool GenerateArgVarList(const std::string &phaseName) override;
  bool ProcessFunctionArgs(const std::string &phaseName);
  bool EmitLocalVarInfo(const std::string &phaseName);
  bool EmitToFEIRStmt(const std::string &phaseName) override;

  // interface implement
  void InitImpl() override;
  void PreProcessImpl() override;
  void ProcessImpl() override;
  void FinishImpl() override;
  bool PreProcessTypeNameIdx() override;
  void GenerateGeneralStmtFailCallBack() override;
  void GenerateGeneralDebugInfo() override;
  bool VerifyGeneral() override;
  void VerifyGeneralFailCallBack() override;
  std::string GetGeneralFuncName() const override;

  GeneralBB *NewGeneralBB() override;
  GeneralBB *NewGeneralBB(uint8 argBBKind) override;
  bool HasThis() override {
    return methodHelper.HasThis();
  }
  void EmitToFEIRStmt(const JBCBB &bb);

 LLT_PRIVATE:
  const JBCClassMethod2FEHelper &methodHelper;
  const jbc::JBCClassMethod &method;
  JBCStack2FEHelper stack2feHelper;
  JBCFunctionContext context;
  bool error = false;
  GeneralBB *pesudoBBCatchPred = nullptr;

  bool PreBuildJsrInfo(const jbc::JBCAttrCode &code);
  bool BuildStmtFromInstruction(const jbc::JBCAttrCode &code);
  GeneralStmt *BuildStmtFromInstructionForBranch(const jbc::JBCOp &op);
  GeneralStmt *BuildStmtFromInstructionForGoto(const jbc::JBCOp &op);
  GeneralStmt *BuildStmtFromInstructionForSwitch(const jbc::JBCOp &op);
  GeneralStmt *BuildStmtFromInstructionForJsr(const jbc::JBCOp &op);
  GeneralStmt *BuildStmtFromInstructionForRet(const jbc::JBCOp &op);
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
  GeneralStmt *BuildAndUpdateLabel(uint32 dstPC, const std::unique_ptr<GeneralStmt> &srcStmt);
  void ArrangeStmts();
  bool CheckJVMStackResult();
  void InitStack2FEHelper();
  uint32 CalculateMaxSwapSize() const;
  bool NeedConvertToInt32(const std::unique_ptr<FEIRVar> &var);
  void AppendFEIRStmts(std::list<UniqueFEIRStmt> &stmts);
  void InsertFEIRStmtsBefore(FEIRStmt &pos, std::list<UniqueFEIRStmt> &stmts);
};  // class JBCFunction
}  // namespace maple
#endif  // MPLFE_INCLUDE_COMMON_JBC_FUNCTION_H
