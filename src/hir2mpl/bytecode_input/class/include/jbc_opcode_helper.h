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
#ifndef HIR2MPL_INCLUDE_COMMON_JBC_OPCODE_HELPER_H
#define HIR2MPL_INCLUDE_COMMON_JBC_OPCODE_HELPER_H
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include "mpl_logging.h"
#include "jbc_opcode.h"
#include "jbc_class.h"

namespace maple {
class JBCOpcodeHelper {
 public:
  JBCOpcodeHelper(const jbc::JBCClassMethod &argMethod);
  ~JBCOpcodeHelper() = default;
  std::string GetLastErorr() const {
    return ssLastError.str();
  }
  std::vector<std::string> GetBaseTypeNamesForOP(const jbc::JBCOp &op, bool &success);
  static std::vector<jbc::JBCPrimType> GetInputPrimTypeForOP(const jbc::JBCOp &op, const jbc::JBCConstPool &constPool);
  static jbc::JBCPrimType GetOutputPrimTypeForOP(const jbc::JBCOp &op, const jbc::JBCConstPool &constPool);

 LLT_PRIVATE:
  using FuncPtrGetBaseTypeName = std::vector<std::string> (JBCOpcodeHelper::*)(const jbc::JBCOp &op, bool &success);
  std::vector<std::string> GetBaseTypeNamesForOPDefault(bool &success) const;
  std::vector<std::string> GetBaseTypeNamesForOPConst(const jbc::JBCOp &op, bool &success);
  std::vector<std::string> GetBaseTypeNamesForOPFieldOpr(const jbc::JBCOp &op, bool &success);
  std::vector<std::string> GetBaseTypeNamesForOPInvoke(const jbc::JBCOp &op, bool &success);
  std::vector<std::string> GetBaseTypeNamesForOPNew(const jbc::JBCOp &op, bool &success);
  std::vector<std::string> GetBaseTypeNamesForOPMultiANewArray(const jbc::JBCOp &op, bool &success);
  std::vector<std::string> GetBaseTypeNamesForOPTypeCheck(const jbc::JBCOp &op, bool &success);
  static std::map<jbc::JBCOpcode, std::vector<jbc::JBCPrimType>> InitMapOpInputPrimTypes();
  static std::map<jbc::JBCOpcodeKind, FuncPtrGetBaseTypeName> InitFuncPtrMapGetBaseTypeName();
  static void InitMapOpInputPrimTypesForConst(std::map<jbc::JBCOpcode, std::vector<jbc::JBCPrimType>> &ans);
  static void InitMapOpInputPrimTypesForLoad();

  const jbc::JBCClassMethod &method;
  std::stringstream ssLastError;
  static std::map<jbc::JBCOpcode, std::vector<jbc::JBCPrimType>> mapOpInputPrimTypes;
  static std::map<jbc::JBCOpcodeKind, FuncPtrGetBaseTypeName> funcPtrMapGetBaseTypeName;
};  // class JBCOpcodeHelper
}  // namespace maple
#endif
