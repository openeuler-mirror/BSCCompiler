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

#include "ast_util.h"
#include "clang/AST/AST.h"
#include "clang/Serialization/ASTDeserializationListener.h"
#include "mir_nodes.h"
#include "mir_builder.h"
#include "ast_macros.h"
#include "fe_utils_ast.h"

namespace maple {
int ast2mplDebug = 0;
int ast2mplDebugIndent = 0;
uint32_t ast2mplOption = kCheckAssertion;
const char *checkFuncName = nullptr;

void ASTUtil::AdjIndent(int n) {
  ast2mplDebugIndent += n;
}

void ASTUtil::SetIndent(int n) {
  ast2mplDebugIndent = n;
}

bool ASTUtil::ValidInName(char c) {
  return isalnum(c) || (c == '_' || c == '|' || c == ';' || c == '/' || c == '$');
}

bool ASTUtil::IsValidName(const std::string &name) {
  for (size_t i = 0; i < name.length(); ++i) {
    if (!ValidInName(name[i])) {
      return false;
    }
  }
  return true;
}

bool ASTUtil::IsSignedType(MIRType &type) {
  PrimType primType = type.GetPrimType();
  if (primType == PTY_i8 || primType == PTY_i16 || primType == PTY_i32 || primType == PTY_i64) {
    return true;
  }
  return false;
}

void ASTUtil::AdjustName(std::string &name) {
  for (size_t i = 0; name[i] != '\0'; ++i) {
    char c = name[i];
    if (ASTUtil::ValidInName(c)) {
      name[i] = c;
      continue;
    }
    switch (c) {
      case '(':
      case ')':
      case '<':
      case '>':
      case '-':
      case ' ':
        name[i] = '_';
        break;
      case '[':
        name[i] = 'A';
        break;
      case ']':
        name[i] = 'B';
        break;
      default:
        name[i] = '$';
        break;
    }
  }
}

std::string ASTUtil::GetAdjustVarName(const std::string &name, uint32_t &num) {
  std::stringstream ss;
  ss << name << "." << num;

  DEBUGPRINT2(ss.str());
  ++num;
  return ss.str();
}

std::string ASTUtil::GetNameWithSuffix(const std::string &origName, const std::string &suffix) {
  std::stringstream ss;
  ss << origName << suffix;

  return ss.str();
}

Opcode ASTUtil::CvtBinaryOpcode(uint32_t opcode, PrimType pty) {
  switch (opcode) {
    case clang::BO_Mul:
      return OP_mul;    // "*"
    case clang::BO_Div:
      return OP_div;    // "/"
    case clang::BO_Rem:
      return OP_rem;    // "%"
    case clang::BO_Add:
      return OP_add;    // "+"
    case clang::BO_Sub:
      return OP_sub;    // "-"
    case clang::BO_Shl:
      return OP_shl;    // "<<"
    case clang::BO_Shr:
      return IsUnsignedInteger(pty) ? OP_lshr : OP_ashr;   // ">>"
    case clang::BO_LT:
      return OP_lt;     // "<"
    case clang::BO_GT:
      return OP_gt;     // ">"
    case clang::BO_LE:
      return OP_le;     // "<="
    case clang::BO_GE:
      return OP_ge;     // ">="
    case clang::BO_EQ:
      return OP_eq;     // "=="
    case clang::BO_NE:
      return OP_ne;     // "!="
    case clang::BO_And:
      return OP_band;   // "&"
    case clang::BO_Xor:
      return OP_bxor;   // "^"
    case clang::BO_Or:
      return OP_bior;   // "|"
    case clang::BO_LAnd:
      return OP_land;   // "&&"
    case clang::BO_LOr:
      return OP_lior;   // "||"
    default:
      return OP_undef;
  }
}

// these do not have equivalent opcode in mapleir
Opcode ASTUtil::CvtBinaryAssignOpcode(uint32_t opcode) {
  switch (opcode) {
    case clang::BO_Assign:
      return OP_eq;     // "="
    case clang::BO_MulAssign:
      return OP_mul;    // "*="
    case clang::BO_DivAssign:
      return OP_div;    // "/="
    case clang::BO_RemAssign:
      return OP_rem;    // "%="
    case clang::BO_AddAssign:
      return OP_add;    // "+="
    case clang::BO_SubAssign:
      return OP_sub;    // "-="
    case clang::BO_ShlAssign:
      return OP_shl;    // "<<="
    case clang::BO_ShrAssign:
      return OP_lshr;   // ">>="
    case clang::BO_AndAssign:
      return OP_band;   // "&="
    case clang::BO_XorAssign:
      return OP_bxor;   // "^="
    case clang::BO_OrAssign:
      return OP_bior;   // "|="
    case clang::BO_Comma:
      return OP_undef;  // ","
    case clang::BO_PtrMemD:
      return OP_undef;  // ".*"
    case clang::BO_PtrMemI:
      return OP_undef;  // "->*"
    default:
      return OP_undef;
  }
}

Opcode ASTUtil::CvtUnaryOpcode(uint32_t opcode) {
  switch (opcode) {
    case clang::UO_Minus:
      return OP_neg;    // "-"
    case clang::UO_Not:
      return OP_bnot;   // "~"
    case clang::UO_LNot:
      return OP_lnot;   // "!"
    case clang::UO_PostInc:
      return OP_add;    // "++"
    case clang::UO_PostDec:
      return OP_sub;    // "--"
    case clang::UO_PreInc:
      return OP_add;    // "++"
    case clang::UO_PreDec:
      return OP_sub;    // "--"
    case clang::UO_AddrOf:
      return OP_addrof; // "&"
    case clang::UO_Deref:
      return OP_undef;  // "*"
    case clang::UO_Plus:
      return OP_undef;  // "+"
    case clang::UO_Real:
      return OP_undef;  // "__real"
    case clang::UO_Imag:
      return OP_undef;  // "__imag"
    case clang::UO_Extension:
      return OP_undef;  // "__extension__"
    case clang::UO_Coawait:
      return OP_undef;  // "co_await"
    default:
      CHECK_FATAL(false, "NYI ASTUtil::CvtUnaryOpcode", opcode);
  }
}

uint32 ASTUtil::GetDim(MIRType &type) {
  MIRType *ptrType = &type;
  if (type.GetKind() == kTypePointer) {
    auto *ptr = static_cast<MIRPtrType*>(&type);
    ptrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptr->GetPointedTyIdx());
  }
  uint32 dim = 0;
  while (ptrType->GetKind() == kTypeArray) {
    auto *array = static_cast<MIRArrayType*>(ptrType);
    ptrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(array->GetElemTyIdx());
    if (ptrType->GetKind() == kTypePointer) {
      auto *ptr = static_cast<MIRPtrType*>(ptrType);
      ptrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptr->GetPointedTyIdx());
    }
    ++dim;
  }

  DEBUGPRINT2(dim);
  return dim;
}

std::string ASTUtil::GetTypeString(MIRType &type) {
  std::stringstream ss;
  if (ast2mplDebug > kDebugLevelThree) {
    type.Dump(1);
  }
  switch (type.GetKind()) {
    case kTypeScalar:
      ss << FEUtilAST::Type2Label(type.GetPrimType());
      break;
    case kTypeStruct:
    case kTypeClass:
    case kTypeInterface:
    case kTypeBitField:
    case kTypeByName:
    case kTypeParam: {
      ss << GlobalTables::GetStrTable().GetStringFromStrIdx(type.GetNameStrIdx()).c_str() << ";";
      break;
    }
    case kTypeArray: {
      auto &array = static_cast<MIRArrayType&>(type);
      MIRType *elemType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(array.GetElemTyIdx());
      uint32 dim = GetDim(type);
      for (size_t i = 0; i < dim; ++i) {
        ss << 'A';
      }
      ss << GetTypeString(*elemType);
      break;
    }
    case kTypePointer: {
      auto &ptr = static_cast<MIRPtrType&>(type);
      MIRType *pType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptr.GetPointedTyIdx());
      ss << "P";
      std::string str = GetTypeString(*pType);
      ss << str;
      break;
    }
    default:
      break;
  }

  return ss.str();
}

void ASTUtil::DumpMplTypes() {
  uint32 size = static_cast<uint32>(GlobalTables::GetTypeTable().GetTypeTableSize());
  DEBUGPRINT(size);
  DEBUGPRINT_S(" dump type table ");
  for (uint32 i = 1; i < size; ++i) {
    MIRType *type = GlobalTables::GetTypeTable().GetTypeTable()[i];
    std::string name = GlobalTables::GetStrTable().GetStringFromStrIdx(type->GetNameStrIdx());
    (void)printf("%-4u %4u %s\n", i, type->GetNameStrIdx().GetIdx(), name.c_str());
  }
  DEBUGPRINT_S(" end dump type table ");
}

void ASTUtil::DumpMplStrings() {
  size_t size = GlobalTables::GetStrTable().StringTableSize();
  DEBUGPRINT(size);
  DEBUGPRINT_S(" dump string table ");
  for (size_t i = 1; i < size; ++i) {
    MIRType *type = GlobalTables::GetTypeTable().GetTypeTable()[i];
    const std::string &str = GlobalTables::GetStrTable().GetStringFromStrIdx(type->GetNameStrIdx());
    (void)printf("%-4d %4d %s\n", static_cast<int>(i), static_cast<int>(str.length()), str.c_str());
  }
  DEBUGPRINT_S(" end dump string table ");
}

bool ASTUtil::IsVoidPointerType(const TyIdx &tyIdx) {
  MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
  auto *ptr = static_cast<MIRPtrType*>(type);
  type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptr->GetPointedTyIdx());
  if (type->GetPrimType() == PTY_void) {
    return true;
  }
  return false;
}

std::string ASTUtil::AdjustFuncName(std::string funcName) {
  std::size_t found = funcName.find('\"');
  const std::size_t offsetByTwoChar = 2;  // skip the replaced char
  while (found != std::string::npos) {
    (void)funcName.replace(found, 1, "\\\"");
    found += offsetByTwoChar;
    found = funcName.find('\"', found);
  }
  return funcName;
}

bool ASTUtil::InsertFuncSet(const GStrIdx &idx) {
  static std::set<GStrIdx> funcIdxSet;
  return funcIdxSet.insert(idx).second;
}
}  // namespace maple
