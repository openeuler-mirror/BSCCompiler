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

constexpr uint32 kUpperLimitOfFieldNum = 10;
static MIRStructType *GetDassignedStructType(const DassignNode *dassign, MIRFunction *func) {
  const auto &lhsStIdx = dassign->GetStIdx();
  auto lhsSymbol = func->GetLocalOrGlobalSymbol(lhsStIdx);
  auto lhsAggType = lhsSymbol->GetType();
  if (!lhsAggType->IsStructType()) {
    return nullptr;
  }
  if (lhsAggType->GetKind() == kTypeUnion) {  // no need to split union's field
    return nullptr;
  }
  auto lhsFieldID = dassign->GetFieldID();
  if (lhsFieldID != 0) {
    CHECK_FATAL(lhsAggType->IsStructType(), "only struct has non-zero fieldID");
    lhsAggType = static_cast<MIRStructType *>(lhsAggType)->GetFieldType(lhsFieldID);
    if (!lhsAggType->IsStructType()) {
      return nullptr;
    }
    if (lhsAggType->GetKind() == kTypeUnion) {  // no need to split union's field
      return nullptr;
    }
  }
  if (static_cast<MIRStructType *>(lhsAggType)->NumberOfFieldIDs() > kUpperLimitOfFieldNum) {
    return nullptr;
  }
  return static_cast<MIRStructType *>(lhsAggType);
}

static MIRStructType *GetIassignedStructType(const IassignNode *iassign) {
  auto ptrTyIdx = iassign->GetTyIdx();
  auto *ptrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptrTyIdx);
  CHECK_FATAL(ptrType->IsMIRPtrType(), "must be pointer type");
  auto aggTyIdx = static_cast<MIRPtrType *>(ptrType)->GetPointedTyIdxWithFieldID(iassign->GetFieldID());
  auto *lhsAggType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(aggTyIdx);
  if (!lhsAggType->IsStructType()) {
    return nullptr;
  }
  if (lhsAggType->GetKind() == kTypeUnion) {
    return nullptr;
  }
  if (static_cast<MIRStructType *>(lhsAggType)->NumberOfFieldIDs() > kUpperLimitOfFieldNum) {
    return nullptr;
  }
  return static_cast<MIRStructType *>(lhsAggType);
}

static MIRStructType *GetReadedStructureType(const DreadNode *dread, const MIRFunction *func) {
  const auto &rhsStIdx = dread->GetStIdx();
  auto rhsSymbol = func->GetLocalOrGlobalSymbol(rhsStIdx);
  auto rhsAggType = rhsSymbol->GetType();
  auto rhsFieldID = dread->GetFieldID();
  if (rhsFieldID != 0) {
    CHECK_FATAL(rhsAggType->IsStructType(), "only struct has non-zero fieldID");
    rhsAggType = static_cast<MIRStructType *>(rhsAggType)->GetFieldType(rhsFieldID);
  }
  if (!rhsAggType->IsStructType()) {
    return nullptr;
  }
  return static_cast<MIRStructType *>(rhsAggType);
}

static MIRStructType *GetReadedStructureType(const IreadNode *iread, const MIRFunction*) {
  auto rhsPtrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iread->GetTyIdx());
  CHECK_FATAL(rhsPtrType->IsMIRPtrType(), "must be pointer type");
  auto rhsAggType = static_cast<MIRPtrType*>(rhsPtrType)->GetPointedType();
  auto rhsFieldID = iread->GetFieldID();
  if (rhsFieldID != 0) {
    CHECK_FATAL(rhsAggType->IsStructType(), "only struct has non-zero fieldID");
    rhsAggType = static_cast<MIRStructType *>(rhsAggType)->GetFieldType(rhsFieldID);
  }
  if (!rhsAggType->IsStructType()) {
    return nullptr;
  }
  return static_cast<MIRStructType *>(rhsAggType);
}

template <class RhsType, class AssignType>
static StmtNode *SplitAggCopy(AssignType *assignNode, MIRStructType *structureType,
                              BlockNode *block, MIRFunction *func) {
  auto *readNode = static_cast<RhsType *>(assignNode->GetRHS());
  auto rhsFieldID = readNode->GetFieldID();
  auto *rhsAggType = GetReadedStructureType(readNode, func);
  if (structureType != rhsAggType) {
    return nullptr;
  }

  for (uint id = 1; id <= structureType->NumberOfFieldIDs(); ++id) {
    MIRType *fieldType = structureType->GetFieldType(id);
    if (fieldType->GetSize() == 0) {
      continue; // field size is zero for empty struct/union;
    }
    if (fieldType->GetKind() == kTypeBitField && static_cast<MIRBitFieldType *>(fieldType)->GetFieldSize() == 0) {
      continue; // bitfield size is zero
    }
    auto *newDassign = assignNode->CloneTree(func->GetCodeMemPoolAllocator());
    newDassign->SetFieldID(assignNode->GetFieldID() + id);
    auto *newRHS = static_cast<RhsType *>(newDassign->GetRHS());
    newRHS->SetFieldID(rhsFieldID + id);
    newRHS->SetPrimType(fieldType->GetPrimType());
    block->InsertAfter(assignNode, newDassign);
  }
  auto newAssign = assignNode->GetNext();
  block->RemoveStmt(assignNode);
  return newAssign;
}

static StmtNode *SplitDassignAggCopy(DassignNode *dassign, BlockNode *block, MIRFunction *func) {
  auto *rhs = dassign->GetRHS();
  if (rhs->GetPrimType() != PTY_agg) {
    return nullptr;
  }

  auto *lhsAggType = GetDassignedStructType(dassign, func);
  if (lhsAggType == nullptr) {
    return nullptr;
  }

  if (rhs->GetOpCode() == OP_dread) {
    auto *lhsSymbol = func->GetLocalOrGlobalSymbol(dassign->GetStIdx());
    auto *rhsSymbol = func->GetLocalOrGlobalSymbol(static_cast<DreadNode *>(rhs)->GetStIdx());
    if (!lhsSymbol->IsLocal() && !rhsSymbol->IsLocal()) {
      return nullptr;
    }

    return SplitAggCopy<DreadNode>(dassign, lhsAggType, block, func);
  } else if (rhs->GetOpCode() == OP_iread) {
    return SplitAggCopy<IreadNode>(dassign, lhsAggType, block, func);
  }
  return nullptr;
}

static StmtNode *SplitIassignAggCopy(IassignNode *iassign, BlockNode *block, MIRFunction *func) {
  auto rhs = iassign->GetRHS();
  if (rhs->GetPrimType() != PTY_agg) {
    return nullptr;
  }

  auto *lhsAggType = GetIassignedStructType(iassign);
  if (lhsAggType == nullptr) {
    return nullptr;
  }

  if (rhs->GetOpCode() == OP_dread) {
    return SplitAggCopy<DreadNode>(iassign, lhsAggType, block, func);
  } else if (rhs->GetOpCode() == OP_iread) {
    return SplitAggCopy<IreadNode>(iassign, lhsAggType, block, func);
  }
  return nullptr;
}

bool UseGlobalVar(const BaseNode *expr) {
  if (expr->GetOpCode() == OP_addrof || expr->GetOpCode() == OP_dread) {
    StIdx stIdx = static_cast<const AddrofNode*>(expr)->GetStIdx();
    if (stIdx.IsGlobal()) {
      return true;
    }
  }
  for (size_t i = 0; i < expr->GetNumOpnds(); ++i) {
    if (UseGlobalVar(expr->Opnd(i))) {
      return true;
    }
  }
  return false;
}

StmtNode *Simplify::SimplifyToSelect(MIRFunction *func, IfStmtNode *ifNode, BlockNode *block) {
  // if (condition) {
  //   res = trueRes
  // }
  // else {
  //   res = falseRes
  // }
  // =================
  // res = select condition ? trueRes : falseRes
  if (ifNode->GetPrev() != nullptr && ifNode->GetPrev()->GetOpCode() == OP_label) {
    // simplify shortCircuit will stop opt in cfg_opt, and generate extra compare
    auto *labelNode = static_cast<LabelNode*>(ifNode->GetPrev());
    const std::string &labelName = func->GetLabelTabItem(labelNode->GetLabelIdx());
    if (labelName.find("shortCircuit") != std::string::npos) {
      return nullptr;
    }
  }
  if (ifNode->GetThenPart() == nullptr || ifNode->GetElsePart() == nullptr) {
    return nullptr;
  }
  StmtNode *thenFirst = ifNode->GetThenPart()->GetFirst();
  StmtNode *elseFirst = ifNode->GetElsePart()->GetFirst();
  if (thenFirst == nullptr || elseFirst == nullptr) {
    return nullptr;
  }
  // thenpart and elsepart has only one stmt
  if (thenFirst->GetNext() != nullptr || elseFirst->GetNext() != nullptr) {
    return nullptr;
  }
  if (thenFirst->GetOpCode() != OP_dassign || elseFirst->GetOpCode() != OP_dassign) {
    return nullptr;
  }
  auto *thenDass = static_cast<DassignNode*>(thenFirst);
  auto *elseDass = static_cast<DassignNode*>(elseFirst);
  if (thenDass->GetStIdx() != elseDass->GetStIdx() || thenDass->GetFieldID() != elseDass->GetFieldID()) {
    return nullptr;
  }
  // iread has sideeffect : may cause deref error
  if (HasIreadExpr(thenDass->GetRHS()) || HasIreadExpr(elseDass->GetRHS())) {
    return nullptr;
  }
  // Check if the operand of the select node is complex enough
  // we should not simplify it to if-then-else for either functionality or performance reason
  if (thenDass->GetRHS()->GetPrimType() == PTY_agg || elseDass->GetRHS()->GetPrimType() == PTY_agg) {
    return nullptr;
  }
  constexpr size_t maxDepth = 3;
  if (MaxDepth(thenDass->GetRHS()) > maxDepth || MaxDepth(elseDass->GetRHS()) > maxDepth) {
    return nullptr;
  }
  if (UseGlobalVar(thenDass->GetRHS()) || UseGlobalVar(elseDass->GetRHS())) {
    return nullptr;
  }
  MIRBuilder *mirBuiler = func->GetModule()->GetMIRBuilder();
  MIRType *type = GlobalTables::GetTypeTable().GetPrimType(thenDass->GetRHS()->GetPrimType());
  auto *selectExpr =
      mirBuiler->CreateExprTernary(OP_select, *type, ifNode->Opnd(0), thenDass->GetRHS(), elseDass->GetRHS());
  auto *newDassign = mirBuiler->CreateStmtDassign(thenDass->GetStIdx(), thenDass->GetFieldID(), selectExpr);
  newDassign->SetSrcPos(ifNode->GetSrcPos());
  block->InsertBefore(ifNode, newDassign);
  block->RemoveStmt(ifNode);
  return newDassign;
}

void Simplify::ProcessFunc(MIRFunction *func) {
  if (func->IsEmpty()) {
    return;
  }
  SetCurrentFunction(*func);
  ProcessFuncStmt(*func);
}

void Simplify::ProcessFuncStmt(MIRFunction &func, StmtNode *stmt, BlockNode *block) {
  if (block == nullptr) {
    block = func.GetBody();
  }
  if (stmt == nullptr) {
    stmt = (block == nullptr) ? nullptr : block->GetFirst();
  }
  while (stmt != nullptr) {
    StmtNode *next = stmt->GetNext();
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
        auto newNext = SplitDassignAggCopy(static_cast<DassignNode *>(stmt), block, &func);
        next = newNext == nullptr ? next : newNext;
        break;
      }
      case OP_iassign: {
        auto newNext = SplitIassignAggCopy(static_cast<IassignNode *>(stmt), block, &func);
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

void M2MSimplify::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<M2MKlassHierarchy>();
  aDep.SetPreservedAll();
}

bool M2MSimplify::PhaseRun(maple::MIRModule &m) {
  OPT_TEMPLATE_NEWPM(Simplify, m)
  return true;
}
}  // namespace maple
