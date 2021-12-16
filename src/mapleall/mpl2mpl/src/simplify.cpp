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
#include "constantfold.h"

namespace {
constexpr char kClassNameOfMath[] = "Ljava_2Flang_2FMath_3B";
constexpr char kFuncNamePrefixOfMathSqrt[] = "Ljava_2Flang_2FMath_3B_7Csqrt_7C_28D_29D";
constexpr char kFuncNamePrefixOfMathAbs[] = "Ljava_2Flang_2FMath_3B_7Cabs_7C";
constexpr char kFuncNamePrefixOfMathMax[] = "Ljava_2Flang_2FMath_3B_7Cmax_7C";
constexpr char kFuncNamePrefixOfMathMin[] = "Ljava_2Flang_2FMath_3B_7Cmin_7C";
constexpr char kFuncNameOfMathAbs[] = "abs";
constexpr char kFuncNameOfMemset[] = "memset";
constexpr char kFuncNameOfMemcpy[] = "memcpy";
constexpr char kFuncNameOfMemsetS[] = "memset_s";
constexpr uint32_t kSecondOpnd = 1;
constexpr uint32_t kThirdOpnd = 2;
constexpr uint64_t kSecurecMemMaxLen = 0x7fffffffUL;
} // namespace

namespace maple {
// If size (in byte) is bigger than this threshold, we won't expand memop
const uint32 SimplifyMemOp::thresholdMemsetExpand = 512;
const uint32 SimplifyMemOp::thresholdMemcpyExpand = 512;
const uint32 SimplifyMemOp::thresholdMemsetSExpand = 1024;
static uint32 kMaxMemoryBlockSizeToAssign = 8;  // in byte

static void MayPrintLog(bool debug, bool success, MemOpKind memOpKind, const char *str) {
  if (!debug) {
    return;
  }
  const char *memop = "";
  if (memOpKind == MEM_OP_memset) {
    memop = "memset";
  } else if (memOpKind == MEM_OP_memcpy) {
    memop = "memcpy";
  } else if (memOpKind == MEM_OP_memset_s) {
    memop = "memset_s";
  }
  LogInfo::MapleLogger() << memop << " expand " << (success ? "success: " : "failure: ") << str << std::endl;
}

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

void Simplify::SimplifyCallAssigned(StmtNode &stmt, BlockNode &block) {
  if (SimplifyMathMethod(stmt, block)) {
    return;
  }
  if (simplifyMemOp.AutoSimplify(stmt, block, false)) {
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
  // Example: if (condition) {
  //   Example: res = trueRes
  // Example: }
  // Example: else {
  //   Example: res = falseRes
  // Example: }
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
  simplifyMemOp.SetFunction(func);
  const bool debug = Options::dumpPhase == "simplify" &&
                     (Options::dumpFunc == "*" || Options::dumpFunc == func->GetName());
  simplifyMemOp.SetDebug(debug);
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

// Join `num` `byte`s into a number
// Example:
//   byte   num                output
//   0x0a    2                 0x0a0a
//   0x12    4             0x12121212
//   0xff    8     0xffffffffffffffff
static uint64 JoinBytes(int byte, uint32 num) {
  CHECK_FATAL(num <= 8, "not support");
  uint64 realByte = byte % 256;
  if (realByte == 0) {
    return 0;
  }
  uint64 result = 0;
  for (uint32 i = 0; i < num; ++i) {
    result += (realByte << (i * 8));
  }
  return result;
}

// Return Fold result expr, does not always return a constant expr
// Attention: Fold may modify the input expr, if foldExpr is not a nullptr, we should always replace expr with foldExpr
static BaseNode *FoldIntConst(BaseNode *expr, int64 &out, bool &isIntConst) {
  if (expr->GetOpCode() == OP_constval) {
    MIRConst *mirConst = static_cast<ConstvalNode*>(expr)->GetConstVal();
    if (mirConst->GetKind() == kConstInt) {
      out = static_cast<MIRIntConst*>(mirConst)->GetValue();
      isIntConst = true;
    }
    return nullptr;
  }
  BaseNode *foldExpr = nullptr;
  static ConstantFold cf(*theMIRModule);
  foldExpr = cf.Fold(expr);
  if (foldExpr != nullptr && foldExpr->GetOpCode() == OP_constval) {
    MIRConst *mirConst = static_cast<ConstvalNode*>(foldExpr)->GetConstVal();
    if (mirConst->GetKind() == kConstInt) {
      out = static_cast<MIRIntConst*>(mirConst)->GetValue();
      isIntConst = true;
    }
  }
  return foldExpr;
}

static BaseNode *ConstructConstvalNode(int64 val, PrimType primType, MIRBuilder &mirBuilder) {
  PrimType constPrimType = primType;
  if (IsPrimitiveFloat(primType)) {
    constPrimType = GetIntegerPrimTypeBySizeAndSign(GetPrimTypeBitSize(primType), false);
  }
  MIRType *constType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(constPrimType));
  MIRConst *mirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(val, *constType);
  BaseNode *ret = mirBuilder.CreateConstval(mirConst);
  if (IsPrimitiveFloat(primType)) {
    MIRType *floatType =  GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(primType));
    ret = mirBuilder.CreateExprRetype(*floatType, constPrimType, ret);
  }
  return ret;
}

static BaseNode *ConstructConstvalNode(int64 byte, int64 num, PrimType primType, MIRBuilder &mirBuilder) {
  auto val = JoinBytes(byte, num);
  return ConstructConstvalNode(val, primType, mirBuilder);
}

// Input total size of memory, split the memory into several blocks, the max block size is 8 bytes
// Example:
//   input        output
//     40     [ 8, 8, 8, 8, 8 ]
//     31     [ 8, 8, 8, 4, 2, 1 ]
static void SplitMemoryIntoBlocks(size_t totalMemorySize, std::vector<uint32> &blocks) {
  size_t leftSize = totalMemorySize;
  size_t curBlockSize = kMaxMemoryBlockSizeToAssign;  // max block size in byte
  while (curBlockSize > 0) {
    size_t n = leftSize / curBlockSize;
    blocks.insert(blocks.end(), n, curBlockSize);
    leftSize -= (n * curBlockSize);
    curBlockSize = curBlockSize >> 1;
  }
}

static bool IsComplexExpr(BaseNode *expr, MIRFunction &func) {
  Opcode op = expr->GetOpCode();
  if (op == OP_regread) {
    return false;
  }
  if (op == OP_dread) {
    auto *symbol = func.GetLocalOrGlobalSymbol(static_cast<DreadNode*>(expr)->GetStIdx());
    if (symbol->IsGlobal() || symbol->GetStorageClass() == kScPstatic) {
      return true;  // dread global/static var is complex expr because it will be lowered to adrp + add
    } else {
      return false;
    }
  }
  if (op == OP_addrof) {
    auto *symbol = func.GetLocalOrGlobalSymbol(static_cast<AddrofNode*>(expr)->GetStIdx());
    if (symbol->IsGlobal() || symbol->GetStorageClass() == kScPstatic) {
      return true;  // addrof global/static var is complex expr because it will be lowered to adrp + add
    } else {
      return false;
    }
  }
  return true;
}

// Input a address expr, output a memEntry to abstract this expr
bool MemEntry::ComputeMemEntry(BaseNode &expr, MIRFunction &func, MemEntry &memEntry, bool isLowLevel) {
  Opcode op = expr.GetOpCode();
  MIRType *memType = nullptr;
  switch (op) {
    case OP_dread: {
      const auto &concreteExpr = static_cast<const DreadNode&>(expr);
      auto *symbol = func.GetLocalOrGlobalSymbol(concreteExpr.GetStIdx());
      MIRType *curType = symbol->GetType();
      if (concreteExpr.GetFieldID() != 0) {
        curType = static_cast<MIRStructType*>(curType)->GetFieldType(concreteExpr.GetFieldID());
      }
      // Support kTypeScalar ptr if possible
      if (curType->GetKind() == kTypePointer) {
        memType = static_cast<MIRPtrType*>(curType)->GetPointedType();
      }
      break;
    }
    case OP_addrof: {
      const auto &concreteExpr = static_cast<const AddrofNode&>(expr);
      auto *symbol = func.GetLocalOrGlobalSymbol(concreteExpr.GetStIdx());
      MIRType *curType = symbol->GetType();
      if (concreteExpr.GetFieldID() != 0) {
        curType = static_cast<MIRStructType*>(curType)->GetFieldType(concreteExpr.GetFieldID());
      }
      memType = curType;
      break;
    }
    case OP_iread: {
      const auto &concreteExpr = static_cast<const IreadNode&>(expr);
      memType = concreteExpr.GetType();
      break;
    }
    case OP_iaddrof: {  // Do NOT call GetType because it is for OP_iread
      const auto &concreteExpr = static_cast<const IaddrofNode&>(expr);
      MIRType *curType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(concreteExpr.GetTyIdx());
      CHECK_FATAL(curType->IsMIRPtrType(), "must be MIRPtrType");
      curType = static_cast<MIRPtrType*>(curType)->GetPointedType();
      CHECK_FATAL(curType->IsStructType(), "must be MIRStructType");
      memType = static_cast<MIRStructType*>(curType)->GetFieldType(concreteExpr.GetFieldID());
      break;
    }
    case OP_regread: {
      if (isLowLevel && IsPrimitivePoint(expr.GetPrimType())) {
        memEntry.addrExpr = &expr;
        memEntry.memType = nullptr;  // we cannot infer high level memory type, this is allowed for low level expand
        return true;
      }
      const auto &concreteExpr = static_cast<const RegreadNode&>(expr);
      MIRPreg *preg = func.GetPregItem(concreteExpr.GetRegIdx());
      bool isFromDread = (preg->GetOp() == OP_dread);
      bool isFromAddrof = (preg->GetOp() == OP_addrof);
      if (isFromDread || isFromAddrof) {
        auto *symbol = preg->rematInfo.sym;
        auto fieldId = preg->fieldID;
        MIRType *curType = symbol->GetType();
        if (fieldId != 0) {
          curType = static_cast<MIRStructType*>(symbol->GetType())->GetFieldType(fieldId);
        }
        if (isFromDread && curType->GetKind() == kTypePointer) {
          curType = static_cast<MIRPtrType*>(curType)->GetPointedType();
        }
        memType = curType;
      }
      break;
    }
    default: {
      if (isLowLevel && IsPrimitivePoint(expr.GetPrimType())) {
        memEntry.addrExpr = &expr;
        memEntry.memType = nullptr;  // we cannot infer high level memory type, this is allowed for low level expand
        return true;
      }
      break;
    }
  }
  if (memType == nullptr) {
    return false;
  }
  memEntry.addrExpr = &expr;
  memEntry.memType = memType;
  return true;
}

BaseNode *MemEntry::BuildAsRhsExpr(MIRFunction &func) const {
  BaseNode *expr = nullptr;
  MIRBuilder *mirBuilder = func.GetModule()->GetMIRBuilder();
  if (addrExpr->GetOpCode() == OP_addrof) {
    // We prefer dread to iread
    // consider iaddrof if possible
    auto *addrof = static_cast<AddrofNode*>(addrExpr);
    auto *symbol = func.GetLocalOrGlobalSymbol(addrof->GetStIdx());
    expr = mirBuilder->CreateExprDread(*memType, addrof->GetFieldID(), *symbol);
  } else {
    MIRType *structPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*memType);
    expr = mirBuilder->CreateExprIread(*memType, *structPtrType, 0, addrExpr);
  }
  return expr;
}

// Lower memset(MemEntry, byte, size) into a series of assign stmts and replace callStmt in the block
// with these assign stmts
bool MemEntry::ExpandMemset(int64 byte, int64 size, MIRFunction &func,
                            CallNode &callStmt, BlockNode &block, bool isLowLevel, bool debug,
                            ErrorNumber errorNumber) const {
  MemOpKind memOpKind = SimplifyMemOp::ComputeMemOpKind(callStmt) ;
  MemEntryKind memKind = GetKind();
  // we don't check size equality in the low level expand
  if (!isLowLevel) {
    CHECK_FATAL(memKind != kMemEntryUnknown, "invalid memKind");
    if (memType->GetSize() != size) {
      MayPrintLog(debug, false, memOpKind, "dst size and size arg are not equal");
      return false;
    }
    if (memOpKind == MEM_OP_memset_s) {
      MayPrintLog(debug, false, memOpKind, "all memset_s will be handled by cglower");
      return false;
    }
  }
  MIRBuilder *mirBuilder = func.GetModule()->GetMIRBuilder();

  bool addNullptrCheck = (memOpKind == MEM_OP_memset_s);
  LabelIdx nullLabIdx;
  if (isLowLevel) {  // For cglower, replace memset with a series of low-level iassignoff
    std::vector<uint32> blocks;
    SplitMemoryIntoBlocks(size, blocks);
    int32 offset = 0;
    // If blocks.size() > 1 and `dst` is not a leaf node,
    // we should extract common expr to avoid redundant expression
    BaseNode *realDstExpr = addrExpr;
    bool shouldExtractRhs = false;
    if (blocks.size() > 1) {
      if (IsComplexExpr(addrExpr, func)) {
        auto pregIdx = func.GetPregTab()->CreatePreg(PTY_ptr);
        StmtNode *regassign = mirBuilder->CreateStmtRegassign(PTY_ptr, pregIdx, addrExpr);
        block.InsertBefore(&callStmt, regassign);
        if (debug) {
          regassign->Dump(false);
        }
        realDstExpr = mirBuilder->CreateExprRegread(PTY_ptr, pregIdx);
      }
      if ((byte & 0xff) != 0) {
        shouldExtractRhs = true;  // rhs const is big, extract it to avoid redundant expression
      }
    }
    if (addNullptrCheck) {
      auto *cmpResType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_u8));
      auto *cmpOpndType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_ptr));
      auto *zeroNode = ConstructConstvalNode(0, PTY_u64, *mirBuilder);
      auto *checkDstExpr = mirBuilder->CreateExprCompare(OP_ne, *cmpResType, *cmpOpndType, realDstExpr, zeroNode);
      nullLabIdx = func.GetLabelTab()->CreateLabelWithPrefix('n');
      auto *checkStmt = mirBuilder->CreateStmtCondGoto(checkDstExpr, OP_brfalse, nullLabIdx);
      block.InsertBefore(&callStmt, checkStmt);
      if (debug) {
        checkStmt->Dump(false);
      }
    }
    BaseNode *readConst = nullptr;
    for (auto curSize : blocks) {
      // low level memset expand result:
      //   iassignoff <prim-type> <offset> (dstAddrExpr, constval <prim-type> xx)
      PrimType constType = GetIntegerPrimTypeBySizeAndSign(curSize * 8, false);
      BaseNode *rhsExpr = ConstructConstvalNode(byte, curSize, constType, *mirBuilder);
      if (shouldExtractRhs) {
        // we only need to extract u64 const once
        PregIdx pregIdx = func.GetPregTab()->CreatePreg(constType);
        auto *constAssign = mirBuilder->CreateStmtRegassign(constType, pregIdx, rhsExpr);
        block.InsertBefore(&callStmt, constAssign);
        if (debug) {
          constAssign->Dump(false);
        }
        readConst = mirBuilder->CreateExprRegread(constType, pregIdx);
        shouldExtractRhs = false;
      }
      if (readConst != nullptr && curSize == kMaxMemoryBlockSizeToAssign) {
        rhsExpr = readConst;
      }
      auto *iassignoff = mirBuilder->CreateStmtIassignoff(constType, offset, realDstExpr, rhsExpr);
      block.InsertBefore(&callStmt, iassignoff);
      if (debug) {
        iassignoff->Dump(false);
      }
      offset += curSize;
    }
  } else if (memKind == kMemEntryPrimitive) {
    BaseNode *rhsExpr = ConstructConstvalNode(byte, size, memType->GetPrimType(), *mirBuilder);
    StmtNode *newAssign = nullptr;
    if (addrExpr->GetOpCode() == OP_addrof) {  // We prefer dassign to iassign
      auto *addrof = static_cast<AddrofNode*>(addrExpr);
      auto *symbol = func.GetLocalOrGlobalSymbol(addrof->GetStIdx());
      newAssign = mirBuilder->CreateStmtDassign(*symbol, addrof->GetFieldID(), rhsExpr);
    } else {
      MIRType *memPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*memType);
      newAssign = mirBuilder->CreateStmtIassign(*memPtrType, 0, addrExpr, rhsExpr);
    }
    block.InsertBefore(&callStmt, newAssign);
    if (debug) {
      newAssign->Dump(false);
    }
  } else if (memKind == kMemEntryStruct) {
    constexpr bool expandForStruct = false;  // enable this when store-merge is powerful enough
    auto *structType = static_cast<MIRStructType*>(memType);
    if (!expandForStruct || structType->HasPadding()) {
      // We only expand memset for no-padding struct, because only in this case, element-wise and byte-wise
      // are equivalent
      MayPrintLog(debug, false, memOpKind, "struct type has padding, cannot do element-wise memory operand");
      return false;
    }

    size_t numFields = structType->NumberOfFieldIDs();
    // Build assign for each fields in the struct type
    // We should skip union fields
    for (size_t id = 1; id <= numFields; ++id) {
      MIRType *fieldType = structType->GetFieldType(id);
      // We only consider leaf field with valid type size
      if (fieldType->GetSize() == 0 || fieldType->GetPrimType() == PTY_agg) {
        continue;
      }
      if (fieldType->GetKind() == kTypeBitField && static_cast<MIRBitFieldType*>(fieldType)->GetFieldSize() == 0) {
        continue;
      }
      // now the fieldType is primitive type
      BaseNode *rhsExpr = ConstructConstvalNode(byte, fieldType->GetSize(), fieldType->GetPrimType(), *mirBuilder);
      StmtNode *fieldAssign = nullptr;
      if (addrExpr->GetOpCode() == OP_addrof) {
        auto *addrof = static_cast<AddrofNode*>(addrExpr);
        auto *symbol = func.GetLocalOrGlobalSymbol(addrof->GetStIdx());
        fieldAssign = mirBuilder->CreateStmtDassign(*symbol, addrof->GetFieldID() + id, rhsExpr);
      } else {
        MIRType *memPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*memType);
        fieldAssign = mirBuilder->CreateStmtIassign(*memPtrType, id, addrExpr, rhsExpr);
      }
      block.InsertBefore(&callStmt, fieldAssign);
      if (debug) {
        fieldAssign->Dump(false);
      }
    }
  } else if (memKind == kMemEntryArray) {
    // We only consider array with dim == 1 now, and element type must be primitive type
    auto *arrayType = static_cast<MIRArrayType*>(memType);
    if (arrayType->GetDim() != 1 || (arrayType->GetElemType()->GetKind() != kTypeScalar &&
        arrayType->GetElemType()->GetKind() != kTypePointer)) {
      MayPrintLog(debug, false, memOpKind, "array dim != 1 or array elements are not primtive type");
      return false;
    }
    MIRType *elemType = arrayType->GetElemType();
    if (elemType->GetSize() < 4) {
      MayPrintLog(debug, false, memOpKind, "element size < 4, don't expand it to  avoid to genearte lots of strb/strh");
      return false;
    }
    size_t elemCnt = arrayType->GetSizeArrayItem(0);
    if (elemType->GetSize() * elemCnt != size) {
      MayPrintLog(debug, false, memOpKind, "array size not equal");
      return false;
    }
    for (size_t i = 0; i < elemCnt; ++i) {
      BaseNode *indexExpr = ConstructConstvalNode(i, PTY_u32, *mirBuilder);
      auto *arrayExpr = mirBuilder->CreateExprArray(*arrayType, addrExpr, indexExpr);
      auto *newValOpnd = ConstructConstvalNode(byte, elemType->GetSize(), elemType->GetPrimType(), *mirBuilder);
      MIRType *elemPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*elemType);
      auto *arrayElementAssign = mirBuilder->CreateStmtIassign(*elemPtrType, 0, arrayExpr, newValOpnd);
      block.InsertBefore(&callStmt, arrayElementAssign);
      if (debug) {
        arrayElementAssign->Dump(false);
      }
    }
  } else {
    CHECK_FATAL(false, "impossible");
  }

  // handle memset return val
  auto *retAssign = GenMemopRetAssign(callStmt, func, isLowLevel, memOpKind, errorNumber);
  if (retAssign != nullptr) {
    block.InsertBefore(&callStmt, retAssign);
    if (debug) {
      retAssign->Dump(false);
    }
  }
  // return ERRNO_INVAL if memset_s dest is NULL
  if (isLowLevel && addNullptrCheck) {
    LabelIdx finalLabIdx = func.GetLabelTab()->CreateLabelWithPrefix('f');
    auto *finalLabelNode = mirBuilder->CreateStmtLabel(finalLabIdx);
    auto *gotoStmt = mirBuilder->CreateStmtGoto(OP_goto, finalLabIdx);
    auto *nullLabelNode = mirBuilder->CreateStmtLabel(nullLabIdx);
    block.InsertBefore(&callStmt, gotoStmt);
    block.InsertBefore(&callStmt, nullLabelNode);
    if (debug) {
      gotoStmt->Dump(0);
      nullLabelNode->Dump(0);
    }
    auto *errnoAssign = GenMemopRetAssign(callStmt, func, isLowLevel, memOpKind, ERRNO_INVAL);
    if (errnoAssign != nullptr) {
      block.InsertBefore(&callStmt, errnoAssign);
      if (debug) {
        errnoAssign->Dump(false);
      }
    }
    block.InsertBefore(&callStmt, finalLabelNode);
    if (debug) {
      finalLabelNode->Dump(0);
    }
  }
  block.RemoveStmt(&callStmt);
  return true;
}

bool MemEntry::ExpandMemcpy(const MemEntry &srcMem, int64 copySize, MIRFunction &func,
                            CallNode &callStmt, BlockNode &block, bool isLowLevel, bool debug) const {
  MemOpKind memOpKind = MEM_OP_memcpy;
  MemEntryKind memKind = GetKind();
  if (!isLowLevel) {  // check type consistency and memKind only for high level expand
    if (memType != srcMem.memType) {
      return false;
    }
    CHECK_FATAL(memKind != kMemEntryUnknown, "invalid memKind");
  }
  MIRBuilder *mirBuilder = func.GetModule()->GetMIRBuilder();
  StmtNode *newAssign = nullptr;
  if (isLowLevel) {  // For cglower, replace memcpy with a series of low-level iassignoff
    std::vector<uint32> blocks;
    SplitMemoryIntoBlocks(copySize, blocks);
    int32 offset = 0;
    // If blocks.size() > 1 and `src` or `dst` is not a leaf node,
    // we should extract common expr to avoid redundant expression
    BaseNode *realSrcExpr = srcMem.addrExpr;
    BaseNode *realDstExpr = addrExpr;
    if (blocks.size() > 1) {
      if (IsComplexExpr(addrExpr, func)) {
        auto pregIdx = func.GetPregTab()->CreatePreg(PTY_ptr);
        StmtNode *regassign = mirBuilder->CreateStmtRegassign(PTY_ptr, pregIdx, addrExpr);
        block.InsertBefore(&callStmt, regassign);
        if (debug) {
          regassign->Dump(false);
        }
        realDstExpr = mirBuilder->CreateExprRegread(PTY_ptr, pregIdx);
      }
      if (IsComplexExpr(srcMem.addrExpr, func)) {
        auto pregIdx = func.GetPregTab()->CreatePreg(PTY_ptr);
        StmtNode *regassign = mirBuilder->CreateStmtRegassign(PTY_ptr, pregIdx, srcMem.addrExpr);
        block.InsertBefore(&callStmt, regassign);
        if (debug) {
          regassign->Dump(false);
        }
        realSrcExpr = mirBuilder->CreateExprRegread(PTY_ptr, pregIdx);
      }
    }
    auto *ptrType = GlobalTables::GetTypeTable().GetPtrType();
    for (auto curSize : blocks) {
      // low level memcpy expand result:
      // It seems ireadoff has not been supported by cg HandleFunction, so we use iread instead of ireadoff
      // [not support] iassignoff <prim-type> <offset> (dstAddrExpr, ireadoff <prim-type> <offset> (srcAddrExpr))
      // [ok] iassignoff <prim-type> <offset> (dstAddrExpr, iread <prim-type> <type> (add ptr (srcAddrExpr, offset)))
      PrimType constType = GetIntegerPrimTypeBySizeAndSign(curSize * 8, false);
      MIRType *constMIRType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(constType));
      auto *constMIRPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*constMIRType);
      BaseNode *rhsAddrExpr = realSrcExpr;
      if (offset != 0) {
        auto *offsetConstExpr = ConstructConstvalNode(offset, PTY_u64, *mirBuilder);
        rhsAddrExpr = mirBuilder->CreateExprBinary(OP_add, *ptrType, realSrcExpr, offsetConstExpr);
      }
      BaseNode *rhsExpr = mirBuilder->CreateExprIread(*constMIRType, *constMIRPtrType, 0, rhsAddrExpr);
      auto *iassignoff = mirBuilder->CreateStmtIassignoff(constType, offset, realDstExpr, rhsExpr);
      block.InsertBefore(&callStmt, iassignoff);
      if (debug) {
        iassignoff->Dump(false);
      }
      offset += curSize;
    }
  } else if (memKind == kMemEntryPrimitive || memKind == kMemEntryStruct) {
    // Do low level expand for all struct memcpy for now
    if (memKind == kMemEntryStruct) {
      MayPrintLog(debug, false, memOpKind, "Do low level expand for all struct memcpy for now");
      return false;
    }
    if (addrExpr->GetOpCode() == OP_addrof) {  // We prefer dassign to iassign
      auto *addrof = static_cast<AddrofNode*>(addrExpr);
      auto *symbol = func.GetLocalOrGlobalSymbol(addrof->GetStIdx());
      newAssign = mirBuilder->CreateStmtDassign(*symbol, addrof->GetFieldID(), srcMem.BuildAsRhsExpr(func));
    } else {
      MIRType *memPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*memType);
      newAssign = mirBuilder->CreateStmtIassign(*memPtrType, 0, addrExpr, srcMem.BuildAsRhsExpr(func));
    }
    block.InsertBefore(&callStmt, newAssign);
    if (debug) {
      newAssign->Dump(false);
    }
    // split struct agg copy
    if (memKind == kMemEntryStruct) {
      if (newAssign->GetOpCode() == OP_dassign) {
        (void)SplitDassignAggCopy(static_cast<DassignNode*>(newAssign), &block, &func);
      } else if (newAssign->GetOpCode() == OP_iassign) {
        (void)SplitIassignAggCopy(static_cast<IassignNode*>(newAssign), &block, &func);
      } else {
        CHECK_FATAL(false, "impossible");
      }
    }
  } else if (memKind == kMemEntryArray) {
    // We only consider array with dim == 1 now, and element type must be primitive type
    auto *arrayType = static_cast<MIRArrayType*>(memType);
    if (arrayType->GetDim() != 1 || (arrayType->GetElemType()->GetKind() != kTypeScalar &&
        arrayType->GetElemType()->GetKind() != kTypePointer)) {
      MayPrintLog(debug, false, memOpKind, "array dim != 1 or array elements are not primtive type");
      return false;
    }
    MIRType *elemType = arrayType->GetElemType();
    if (elemType->GetSize() < 4) {
      MayPrintLog(debug, false, memOpKind, "element size < 4, don't expand it to  avoid to genearte lots of strb/strh");
      return false;
    }
    size_t elemCnt = arrayType->GetSizeArrayItem(0);
    if (elemType->GetSize() * elemCnt != copySize) {
      MayPrintLog(debug, false, memOpKind, "array size not equal");
      return false;
    }
    // if srcExpr is too complex (for example: addrof expr of global/static array), let cg expand it
    if (elemCnt > 1 && IsComplexExpr(srcMem.addrExpr, func)) {
      MayPrintLog(debug, false, memOpKind,
                  "srcExpr is too complex, let cg expand it to avoid redundant inst");
      return false;
    }
    MIRType *elemPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*elemType);
    MIRType *u32Type = GlobalTables::GetTypeTable().GetUInt32();
    for (size_t i = 0; i < elemCnt; ++i) {
      ConstvalNode *indexExpr = mirBuilder->CreateConstval(
          GlobalTables::GetIntConstTable().GetOrCreateIntConst(i, *u32Type));
      auto *arrayExpr = mirBuilder->CreateExprArray(*arrayType, addrExpr, indexExpr);
      auto *rhsArrayExpr = mirBuilder->CreateExprArray(*arrayType, srcMem.addrExpr, indexExpr);
      auto *rhsIreadExpr = mirBuilder->CreateExprIread(*elemType, *elemPtrType, 0, rhsArrayExpr);
      auto *arrayElemAssign = mirBuilder->CreateStmtIassign(*elemPtrType, 0, arrayExpr, rhsIreadExpr);
      block.InsertBefore(&callStmt, arrayElemAssign);
      if (debug) {
        arrayElemAssign->Dump(false);
      }
    }
  } else {
    CHECK_FATAL(false, "impossible");
  }

  // handle memcpy return val
  auto *retAssign = GenMemopRetAssign(callStmt, func, isLowLevel, memOpKind);
  if (retAssign != nullptr) {
    block.InsertBefore(&callStmt, retAssign);
    if (debug) {
      retAssign->Dump(false);
    }
  }
  block.RemoveStmt(&callStmt);
  return true;
}

// handle memset, memcpy return val
StmtNode *MemEntry::GenMemopRetAssign(CallNode &callStmt, MIRFunction &func, bool isLowLevel, MemOpKind memOpKind,
                                      ErrorNumber errorNumber) {
  const auto &retVec = callStmt.GetReturnVec();
  if (retVec.empty()) {
    return nullptr;
  }
  MIRBuilder *mirBuilder = func.GetModule()->GetMIRBuilder();
  BaseNode *rhs = callStmt.Opnd(0);  // for memset, memcpy
  if (memOpKind == MEM_OP_memset_s) {
    // memset_s must return an errorNumber
    MIRType *constType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_i32));
    MIRConst *mirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(errorNumber, *constType);
    rhs = mirBuilder->CreateConstval(mirConst);
  }
  StIdx stIdx = retVec[0].first;
  if (stIdx.FullIdx() != 0) {
    auto *retAssign = mirBuilder->CreateStmtDassign(retVec[0].first, 0, rhs);
    return retAssign;
  }
  PregIdx pregIdx = retVec[0].second.GetPregIdx();
  if (pregIdx != 0) {
    auto pregType = func.GetPregTab()->GetPregTableItem(pregIdx)->GetPrimType();
    auto *retAssign = mirBuilder->CreateStmtRegassign(pregType, pregIdx, rhs);
    if (isLowLevel) {
      retAssign->GetRHS()->SetPrimType(pregType);
    }
    return retAssign;
  }
  return nullptr;
}

MemOpKind SimplifyMemOp::ComputeMemOpKind(StmtNode &stmt) {
  // lowered memop function (such as memset) may be a call, not callassigned
  if (stmt.GetOpCode() != OP_callassigned && stmt.GetOpCode() != OP_call) {
    return MEM_OP_unknown;
  }
  auto &callStmt = static_cast<CallNode&>(stmt);
  MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callStmt.GetPUIdx());
  if (!func->GetAttr(FUNCATTR_extern)) {
    return MEM_OP_unknown;  // function from libc is always marked as extern
  }
  const char *funcName = func->GetName().c_str();
  if (strcmp(funcName, kFuncNameOfMemset) == 0) {
    return MEM_OP_memset;
  }
  if (strcmp(funcName, kFuncNameOfMemcpy) == 0) {
    return MEM_OP_memcpy;
  }
  if (strcmp(funcName, kFuncNameOfMemsetS) == 0) {
    return MEM_OP_memset_s;
  }
  return MEM_OP_unknown;
}

bool SimplifyMemOp::AutoSimplify(StmtNode &stmt, BlockNode &block, bool isLowLevel) const {
  MemOpKind memOpKind = ComputeMemOpKind(stmt);
  switch (memOpKind) {
    case MEM_OP_memset:
    case MEM_OP_memset_s: {
      return SimplifyMemset(stmt, block, isLowLevel);
    }
    case MEM_OP_memcpy: {
      return SimplifyMemcpy(stmt, block, isLowLevel);
    }
    default:
      break;
  }
  return false;
}

// Try to replace the call to memset with a series of assign operations (including dassign, iassign, iassignoff), which
// is usually profitable for small memory size.
// This function is called in two places, one in mpl2mpl simplify, another in cglower:
// (1) mpl2mpl memset expand (isLowLevel == false)
//   for primitive type, array type with element size < 4 bytes and struct type without padding
// (2) cglower memset expand
//   for array type with element size >= 4 bytes and struct type with paddings
bool SimplifyMemOp::SimplifyMemset(StmtNode &stmt, BlockNode &block, bool isLowLevel) const {
  MemOpKind memOpKind = ComputeMemOpKind(stmt);
  if (memOpKind != MEM_OP_memset && memOpKind != MEM_OP_memset_s) {
    return false;
  }
  uint32 dstOpndIdx = 0;
  uint32 dstSizeOpndIdx = 0;  // only used by memset_s
  uint32 srcOpndIdx = 1;
  uint32 srcSizeOpndIdx = 2;
  bool isSafeVersion = memOpKind == MEM_OP_memset_s;
  if (isSafeVersion) {
    dstSizeOpndIdx = 1;
    srcOpndIdx = 2;
    srcSizeOpndIdx = 3;
  }
  auto &callStmt = static_cast<CallNode&>(stmt);
  if (debug) {
    LogInfo::MapleLogger() << "[funcName] " << func->GetName() << std::endl;
    callStmt.Dump(false);
  }

  int64 size = 0;  // memset's 'src size'
  bool isIntConst = false;
  BaseNode *foldSizeExpr = FoldIntConst(callStmt.Opnd(srcSizeOpndIdx), size, isIntConst);
  if (foldSizeExpr != nullptr) {
    callStmt.SetOpnd(foldSizeExpr, srcSizeOpndIdx);
  }
  // memset's 'src size' must be a const value, otherwise we can not expand it
  if (!isIntConst) {
    MayPrintLog(debug, false, memOpKind, "size is not int const");
    return false;
  }
  // If the size is too big, we won't expand it
  uint32 thresholdExpand = (isSafeVersion ? thresholdMemsetSExpand : thresholdMemsetExpand);
  if (size > thresholdExpand) {
    MayPrintLog(debug, false, memOpKind, "size is too big");
    return false;
  }
  if (size == 0) {
    MayPrintLog(debug, false, memOpKind, "memset with size 0");
    return false;
  }
  ErrorNumber errNum = ERRNO_OK;
  // for memset_s, dstSize must be also a const value, and dstSize should >= srcSize
  if (isSafeVersion) {
    int64 dstSize = 0;
    bool isDstSizeConst = false;
    BaseNode *foldDstSizeExpr = FoldIntConst(callStmt.Opnd(dstSizeOpndIdx), dstSize, isDstSizeConst);
    if (foldDstSizeExpr != nullptr) {
      callStmt.SetOpnd(foldDstSizeExpr, dstSizeOpndIdx);
    }
    if (!isDstSizeConst) {
      MayPrintLog(debug, false, memOpKind, "dst size is not int const");
      return false;
    }
    if (dstSize == 0 || dstSize > kSecurecMemMaxLen) {
      size = 0;
      errNum = ERRNO_RANGE;
    }
    if (size > dstSize) {
      size = dstSize;  // if size > dstSize, set `size` to `dstSize`
      errNum = ERRNO_RANGE_AND_RESET;
    }
  }

  int64 val = 0;
  isIntConst = false;
  BaseNode *foldValExpr = FoldIntConst(callStmt.Opnd(srcOpndIdx), val, isIntConst);
  if (foldValExpr != nullptr) {
    callStmt.SetOpnd(foldValExpr, srcOpndIdx);
  }
  // memset's second argument 'val' should also be a const value
  if (!isIntConst) {
    MayPrintLog(debug, false, memOpKind, "val is not int const");
    return false;
  }

  MemEntry dstMemEntry;
  bool valid = MemEntry::ComputeMemEntry(*callStmt.Opnd(dstOpndIdx), *func, dstMemEntry, isLowLevel);
  if (!valid) {
    MayPrintLog(debug, false, memOpKind, "dstMemEntry is invalid");
    return false;
  }
  bool ret = false;
  if (size != 0) {
    ret = dstMemEntry.ExpandMemset(val, size, *func, callStmt, block, isLowLevel, debug, errNum);
  } else {
    // if size == 0, no need to set memory, just return error nummber
    auto *retAssign = dstMemEntry.GenMemopRetAssign(callStmt, *func, isLowLevel, memOpKind, errNum);
    if (retAssign != nullptr) {
      block.InsertBefore(&callStmt, retAssign);
      if (debug) {
        retAssign->Dump(false);
      }
    }
    block.RemoveStmt(&callStmt);
    ret = true;
  }
  if (ret) {
    MayPrintLog(debug, true, memOpKind, "well done");
  }
  return ret;
}

bool SimplifyMemOp::SimplifyMemcpy(StmtNode &stmt, BlockNode &block, bool isLowLevel) const {
  MemOpKind memOpKind = ComputeMemOpKind(stmt);
  if (memOpKind != MEM_OP_memcpy) {
    return false;
  }
  auto &callStmt = static_cast<CallNode&>(stmt);
  if (debug) {
    LogInfo::MapleLogger() << "[funcName] " << func->GetName() << std::endl;
    callStmt.Dump(false);
  }

  int64 copySize = 0;
  bool isIntConst = false;
  BaseNode *foldCopySizeExpr = FoldIntConst(callStmt.Opnd(kThirdOpnd), copySize, isIntConst);
  if (foldCopySizeExpr != nullptr) {
    callStmt.SetOpnd(foldCopySizeExpr, kThirdOpnd);
  }
  if (!isIntConst) {
    MayPrintLog(debug, false, memOpKind, "copy size is not an int const");
    return false;
  }
  if (copySize > thresholdMemcpyExpand) {
    MayPrintLog(debug, false, memOpKind, "size is too big");
    return false;
  }
  if (copySize == 0) {
    MayPrintLog(debug, false, memOpKind, "memcpy with copy size 0");
    return false;
  }
  MemEntry dstMemEntry;
  bool valid = MemEntry::ComputeMemEntry(*callStmt.Opnd(0), *func, dstMemEntry, isLowLevel);
  if (!valid) {
    MayPrintLog(debug, false, memOpKind, "dstMemEntry is invalid");
    return false;
  }
  MemEntry srcMemEntry;
  valid = MemEntry::ComputeMemEntry(*callStmt.Opnd(kSecondOpnd), *func, srcMemEntry, isLowLevel);
  if (!valid) {
    MayPrintLog(debug, false, memOpKind, "srcMemEntry is invalid");
    return false;
  }
  // We don't check type consistency when doing low level expand
  if (!isLowLevel) {
    if (dstMemEntry.memType != srcMemEntry.memType) {
      MayPrintLog(debug, false, memOpKind, "dst and src have different type");
      return false;   // entryType must be identical
    }
    if (dstMemEntry.memType->GetSize() != copySize) {
      MayPrintLog(debug, false, memOpKind, "copy size != dst memory size");
      return false;  // copy size should equal to dst memory size, we maybe allow smaller copy size later
    }
  }
  bool ret = dstMemEntry.ExpandMemcpy(srcMemEntry, copySize, *func, callStmt, block, isLowLevel, debug);
  if (ret) {
    MayPrintLog(debug, true, memOpKind, "well done");
  }
  return ret;
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
