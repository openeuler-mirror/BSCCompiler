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
constexpr char kFuncNameOfMathAbs[] = "abs";
} // namespace

namespace maple {
bool Simplify::IsMathSqrt(const std::string funcName) {
  return (mirMod.IsJavaModule() && (strcmp(funcName.c_str(), kFuncNamePrefixOfMathSqrt) == 0));
}

bool Simplify::IsMathAbs(const std::string funcName) {
  return (mirMod.IsCModule() && (strcmp(funcName.c_str(), kFuncNameOfMathAbs) == 0)) ||
         (mirMod.IsJavaModule() && (strcmp(funcName.c_str(), kFuncNamePrefixOfMathAbs) == 0));
}

bool Simplify::IsMathMax(const std::string funcName) {
  return (mirMod.IsJavaModule() && (strcmp(funcName.c_str(), kFuncNamePrefixOfMathMax) == 0));
}

bool Simplify::IsMathMin(const std::string funcName) {
  return (mirMod.IsJavaModule() && (strcmp(funcName.c_str(), kFuncNamePrefixOfMathMin) == 0));
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

StmtNode *Simplify::SplitAggCopy(StmtNode *stmt, BlockNode *block, MIRFunction *func) {
  if (stmt->GetOpCode() == OP_dassign && stmt->Opnd(0)->GetPrimType() == PTY_agg) {
    if (stmt->Opnd(0)->GetOpCode() == OP_dread) {
      auto *dassign = static_cast<const DassignNode *>(stmt);
      const auto &lhsStIdx = dassign->GetStIdx();
      auto *dread = static_cast<DreadNode *>(stmt->Opnd(0));
      const auto &rhsStIdx = dread->GetStIdx();

      auto lhsSymbol = func->GetLocalOrGlobalSymbol(lhsStIdx);
      auto lhsMIRType = lhsSymbol->GetType();
      if (lhsMIRType->GetKind() == kTypeUnion) {  // no need to split union's field
        return nullptr;
      }
      auto lhsFieldID = dassign->GetFieldID();
      if (lhsFieldID != 0) {
        CHECK_FATAL(lhsMIRType->IsStructType(), "only struct has non-zero fieldID");
        lhsMIRType = static_cast<MIRStructType *>(lhsMIRType)->GetFieldType(lhsFieldID);
      }

      auto rhsSymbol = func->GetLocalOrGlobalSymbol(rhsStIdx);
      auto rhsMIRType = rhsSymbol->GetType();
      auto rhsFieldID = dread->GetFieldID();
      if (rhsFieldID != 0) {
        CHECK_FATAL(rhsMIRType->IsStructType(), "only struct has non-zero fieldID");
        rhsMIRType = static_cast<MIRStructType *>(rhsMIRType)->GetFieldType(rhsFieldID);
      }

      if (!lhsSymbol->IsLocal() && !rhsSymbol->IsLocal()) {
        return nullptr;
      }

      if (lhsMIRType != rhsMIRType) {
        return nullptr;
      }

      if (lhsMIRType->IsStructType()) {
        auto *structType = static_cast<MIRStructType *>(lhsMIRType);
        constexpr uint32 upperLimitOfFieldNum = 10;
        if (lhsMIRType->NumberOfFieldIDs() > upperLimitOfFieldNum) {
          return nullptr;
        }
        auto mirBuiler = func->GetModule()->GetMIRBuilder();
        for (uint id = 1; id <= lhsMIRType->NumberOfFieldIDs(); ++id) {
          MIRType *fieldType = structType->GetFieldType(id);
          if (fieldType->GetSize() == 0) {
            continue; // field size is zero for empty struct/union;
          }
          if (fieldType->GetKind() == kTypeBitField && static_cast<MIRBitFieldType *>(fieldType)->GetFieldSize() == 0) {
            continue; // bitfield size is zero
          }
          auto newRHS = mirBuiler->CreateDread(*rhsSymbol, fieldType->GetPrimType());
          newRHS->SetFieldID(id + rhsFieldID);
          auto newDassign = mirBuiler->CreateStmtDassign(lhsStIdx, lhsFieldID + id, newRHS);
          newDassign->SetSrcPos(stmt->GetSrcPos());
          block->InsertAfter(stmt, newDassign);
        }
        auto newAssign = stmt->GetNext();
        block->RemoveStmt(stmt);
        return newAssign;
      }
    } else if (stmt->Opnd(0)->GetOpCode() == OP_iread) {
      auto *dassign = static_cast<const DassignNode *>(stmt);
      const auto &lhsStIdx = dassign->GetStIdx();
      auto *iread = static_cast<IreadNode *>(stmt->Opnd(0));

      auto lhsSymbol = func->GetLocalOrGlobalSymbol(lhsStIdx);
      auto lhsMIRType = lhsSymbol->GetType();
      if (lhsMIRType->GetKind() == kTypeUnion) {  // no need to split union's field
        return nullptr;
      }
      auto lhsFieldID = dassign->GetFieldID();
      if (lhsFieldID != 0) {
        CHECK_FATAL(lhsMIRType->IsStructType(), "only struct has non-zero fieldID");
        lhsMIRType = static_cast<MIRStructType *>(lhsMIRType)->GetFieldType(lhsFieldID);
      }

      auto pointerTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iread->GetTyIdx());
      auto rhsMIRType =
          GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<MIRPtrType*>(pointerTy)->GetPointedTyIdx());
      auto rhsFieldID = iread->GetFieldID();
      if (rhsFieldID != 0) {
        rhsMIRType = static_cast<MIRStructType *>(rhsMIRType)->GetFieldType(rhsFieldID);
      }

      if (lhsMIRType != rhsMIRType) {
        return nullptr;
      }

      if (lhsMIRType->IsStructType()) {
        auto *structType = static_cast<MIRStructType *>(lhsMIRType);
        constexpr uint32 upperLimitOfFieldNum = 10;
        if (lhsMIRType->NumberOfFieldIDs() > upperLimitOfFieldNum) {
          return nullptr;
        }
        auto mirBuiler = func->GetModule()->GetMIRBuilder();
        for (uint id = 1; id <= lhsMIRType->NumberOfFieldIDs(); ++id) {
          MIRType *fieldType = structType->GetFieldType(id);
          if (fieldType->GetSize() == 0) {
            continue; // field size is zero for empty struct/union;
          }
          if (fieldType->GetKind() == kTypeBitField && static_cast<MIRBitFieldType *>(fieldType)->GetFieldSize() == 0) {
            continue; // bitfield size is zero
          }
          auto newRHS = iread->CloneTree(*mirBuiler->GetCurrentFuncCodeMpAllocator());
          newRHS->SetPrimType(fieldType->GetPrimType());
          newRHS->SetFieldID(id + rhsFieldID);
          auto newDassign = mirBuiler->CreateStmtDassign(lhsStIdx, lhsFieldID + id, newRHS);
          newDassign->SetSrcPos(stmt->GetSrcPos());
          block->InsertBefore(stmt, newDassign);
        }
        auto newAssign = stmt->GetNext();
        block->RemoveStmt(stmt);
        return newAssign;
      }
    }
  }
  return nullptr;
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
      case OP_dassign: {
        auto newNext = SplitAggCopy(stmt, block, &func);
        next = newNext == nullptr ? next : newNext;
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
