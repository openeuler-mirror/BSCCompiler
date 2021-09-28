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
#ifndef AST2MPL_INCLUDE_ASTUTIL_H
#define AST2MPL_INCLUDE_ASTUTIL_H
#include "clang/AST/AST.h"
#include "mir_type.h"
#include "ast_macros.h"

namespace maple {
extern int ast2mplDebug;
extern int ast2mplDebugIndent;
extern uint32_t ast2mplOption;
extern const char *checkFuncName;

class ASTUtil {
 public:
  static const int opcodeNameLength = 228;
  static const char *opcodeName[opcodeNameLength];
  static void AdjIndent(int n);
  static void SetIndent(int n);
  static bool ValidInName(char c);
  static bool IsValidName(const std::string &name);
  static bool IsSignedType(MIRType &type);
  static void AdjustName(std::string &name);
  static std::string GetAdjustVarName(const std::string &name, uint32_t &num);
  static std::string GetNameWithSuffix(const std::string &origName, const std::string &suffix);

  static void DumpMplStrings();
  static void DumpMplTypes();

  static uint32 GetDim(MIRType &type);
  static std::string GetTypeString(MIRType &type);

  static MIRType *CvtPrimType(const clang::QualType type);
  static Opcode CvtUnaryOpcode(uint32_t opcode);
  static Opcode CvtBinaryOpcode(uint32_t opcode, PrimType pty = PTY_begin);
  static Opcode CvtBinaryAssignOpcode(uint32_t opcode);

  static bool IsVoidPointerType(const TyIdx &tyIdx);
  static std::string AdjustFuncName(std::string funcName);
  static bool InsertFuncSet(const GStrIdx &idx);

  template <typename Range, typename Value = typename Range::value_type>
  static std::string Join(const Range &elements, const char *delimiter) {
    std::ostringstream os;
    auto b = begin(elements);
    auto e = end(elements);
    if (b != e) {
      std::copy(b, prev(e), std::ostream_iterator<Value>(os, delimiter));
      b = prev(e);
    }
    if (b != e) {
      os << *b;
    }
    return os.str();
  }
};
}  // namespace maple
#endif  // AST2MPL_INCLUDE_ASTUTIL_H_
