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
#include "simplify.h"
#include <iostream>
#include <algorithm>

namespace {
constexpr char kClassNameOfMath[] = "Ljava_2Flang_2FMath_3B";
constexpr char kFuncNamePrefixOfMathSqrt[] = "Ljava_2Flang_2FMath_3B_7Csqrt_7C_28D_29D";
constexpr char kFuncNamePrefixOfMathAbs[] = "Ljava_2Flang_2FMath_3B_7Cabs_7C";
constexpr char kFuncNamePrefixOfMathMax[] = "Ljava_2Flang_2FMath_3B_7Cmax_7C";
constexpr char kFuncNamePrefixOfMathMin[] = "Ljava_2Flang_2FMath_3B_7Cmin_7C";
constexpr char kFuncNameOfMathSqrt[] = "sqrt";
constexpr char kFuncNameOfMathAbs[] = "abs";
constexpr char kFuncNameOfMathMax[] = "max";
constexpr char kFuncNameOfMathMin[] = "min";
} // namespace

namespace maple {
bool Simplify::IsMathSqrt(const std::string funcName) {
  return (mirMod.IsCModule() && (strcmp(funcName.c_str(), kFuncNameOfMathSqrt) == 0)) ||
         (mirMod.IsJavaModule() && (strcmp(funcName.c_str(), kFuncNamePrefixOfMathSqrt) == 0));
}

bool Simplify::IsMathAbs(const std::string funcName) {
  return (mirMod.IsCModule() && (strcmp(funcName.c_str(), kFuncNameOfMathAbs) == 0)) ||
         (mirMod.IsJavaModule() && (strcmp(funcName.c_str(), kFuncNamePrefixOfMathAbs) == 0));
}

bool Simplify::IsMathMax(const std::string funcName) {
  return (mirMod.IsCModule() && (strcmp(funcName.c_str(), kFuncNameOfMathMax) == 0)) ||
         (mirMod.IsJavaModule() && (strcmp(funcName.c_str(), kFuncNamePrefixOfMathMax) == 0));
}

bool Simplify::IsMathMin(const std::string funcName) {
  return (mirMod.IsCModule() && (strcmp(funcName.c_str(), kFuncNameOfMathMin) == 0)) ||
         (mirMod.IsJavaModule() && (strcmp(funcName.c_str(), kFuncNamePrefixOfMathMin) == 0));
}

bool Simplify::SimplifyMathMethod(const StmtNode &stmt, BlockNode &block) {
  if (stmt.GetOpCode() != OP_callassigned) {
    return false;
  }
  auto &cnode = static_cast<const CallNode&>(stmt);
  MIRFunction *calleeFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(cnode.GetPUIdx());
  ASSERT(calleeFunc != nullptr, "null ptr check");
  const std::string &funcName = calleeFunc->GetName();
  if (funcName.empty()) {
    return false;
  }
  if (!mirMod.IsCModule() && !mirMod.IsJavaModule()) {
    return false;
  }
  if (mirMod.IsJavaModule() && funcName.find(kClassNameOfMath) == std::string::npos) {
    return false;
  }
  if (cnode.GetNumOpnds() == 0 || cnode.GetReturnVec().empty()) {
    return false;
  }

  auto *opnd0 = cnode.Opnd(0);
  ASSERT(opnd0 != nullptr, "null ptr check");
  auto *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(opnd0->GetPrimType());

  BaseNode *opExpr = nullptr;
  if (IsMathSqrt(funcName) && !IsPrimitiveFloat(opnd0->GetPrimType())) {
    opExpr = builder->CreateExprUnary(OP_sqrt, *type, opnd0);
  } else if (IsMathAbs(funcName)) {
    opExpr = builder->CreateExprUnary(OP_abs, *type, opnd0);
  } else if (IsMathMax(funcName)) {
    opExpr = builder->CreateExprBinary(OP_max, *type, opnd0, cnode.Opnd(1));
  } else if (IsMathMin(funcName)) {
    opExpr = builder->CreateExprBinary(OP_min, *type, opnd0, cnode.Opnd(1));
  }
  if (opExpr != nullptr) {
    auto stIdx = cnode.GetNthReturnVec(0).first;
    auto *dassign = builder->CreateStmtDassign(stIdx, 0, opExpr);
    block.ReplaceStmt1WithStmt2(&stmt, dassign);
    return true;
  }
  return false;
}

void Simplify::SimplifyCallAssigned(const StmtNode &stmt, BlockNode &block) {
  if (SimplifyMathMethod(stmt, block)) {
    return;
  }
}

void Simplify::ProcessFunc(MIRFunction *func) {
  if (func->IsEmpty()) {
    return;
  }
  SetCurrentFunction(*func);
  ProcessFuncStmt(*func);
}

void Simplify::ProcessFuncStmt(MIRFunction &func, StmtNode *stmt, BlockNode *block) {
  StmtNode *next = nullptr;
  if (block == nullptr) {
    block = func.GetBody();
  }
  if (stmt == nullptr) {
    stmt = (block == nullptr) ? nullptr : block->GetFirst();
  }
  while (stmt != nullptr) {
    next = stmt->GetNext();
    Opcode op = stmt->GetOpCode();
    switch (op) {
      case OP_if: {
        IfStmtNode *ifNode = static_cast<IfStmtNode*>(stmt);
        if (ifNode->GetThenPart() != nullptr && ifNode->GetThenPart()->GetFirst() != nullptr) {
          ProcessFuncStmt(func, ifNode->GetThenPart()->GetFirst(), ifNode->GetThenPart());
        }
        if (ifNode->GetElsePart() != nullptr && ifNode->GetElsePart()->GetFirst() != nullptr) {
          ProcessFuncStmt(func, ifNode->GetElsePart()->GetFirst(), ifNode->GetElsePart());
        }
        break;
      }
      case OP_dowhile:
      case OP_while: {
        WhileStmtNode *whileNode = static_cast<WhileStmtNode*>(stmt);
        if (whileNode->GetBody() != nullptr) {
          ProcessFuncStmt(func, whileNode->GetBody()->GetFirst(), whileNode->GetBody());
        }
        break;
      }
      case OP_callassigned: {
        SimplifyCallAssigned(*stmt, *block);
        break;
      }
      default: {
        break;
      }
    }
    stmt = next;
  }
}

void Simplify::Finish() {
}
}  // namespace maple
