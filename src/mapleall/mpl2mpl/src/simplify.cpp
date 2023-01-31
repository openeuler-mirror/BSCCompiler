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
#include "triple.h"
#include "simplify.h"
#include <functional>
#include <initializer_list>
#include <iostream>
#include <algorithm>
#include "constantfold.h"

namespace maple {

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
constexpr char kFuncNameOfMemcpyS[] = "memcpy_s";
constexpr char kFuncNameOfSprintf[] = "sprintf";
constexpr char kFuncNameOfSprintfS[] = "sprintf_s";
constexpr char kFuncNameOfSnprintfS[] = "snprintf_s";
constexpr char kFuncNameOfVsnprintfS[] = "vsnprintf_s";
constexpr uint64_t kSecurecMemMaxLen = 0x7fffffffUL;
static constexpr int32 kProbUnlikely = 1000;
constexpr uint32_t kMemOpDstOpndIdx = 0;
constexpr uint32_t kMemOpSDstSizeOpndIdx = 1;
constexpr uint32_t kMemOpSrcOpndIdx = 2;
constexpr uint32_t kMemOpSSrcOpndIdx = 2;
constexpr uint32_t kMemOpSSrcSizeOpndIdx = 3;
constexpr uint32_t kSprintfFmtOpndIdx = 1;
constexpr uint32_t kSprintfOrigOpndIdx = 2;
constexpr uint32_t kSprintfSOrigOpndIdx = 3;
constexpr uint32_t kSnprintfSFmtOpndIdx = 3;
constexpr uint32_t kSnprintfSOrigOpndIdx = 4;
constexpr uint32_t kFmtSizeOpndIdx = 2;
constexpr uint32_t kDstSizeOpndIdx = 1;
constexpr uint32_t kSprintfDstOpndIdx = 0;
constexpr int32 kSprintfErrNum = -1;
constexpr int32 kTruncate = -1;
constexpr std::string_view kTempAddrStr = "tempAddr";
constexpr std::string_view kUnderlineStr = "_";
constexpr std::string_view kVerticalLineStr = "__";
// Truncate the constant field of 'union' if it's written as scalar type (e.g. int),
// but accessed as bit-field type with smaller size.
// Return the truncated constant or the original constant 'fieldCst' if the constant doesn't need to be truncated.
MIRConst *TruncateUnionConstant(const MIRStructType &unionType, MIRConst *fieldCst, MIRType &unionFieldType) {
  if (!fieldCst || unionType.GetKind() != kTypeUnion) {
    return fieldCst;
  }

  auto *bitFieldType = safe_cast<MIRBitFieldType>(unionFieldType);
  auto *intCst = safe_cast<MIRIntConst>(fieldCst);

  if (!intCst) {
    return fieldCst;
  }

  if (!bitFieldType) {
    if (GetPrimTypeSize(unionFieldType.GetPrimType()) < GetPrimTypeSize(fieldCst->GetType().GetPrimType())) {
      return GlobalTables::GetIntConstTable().GetOrCreateIntConst(intCst->GetValue(), unionFieldType);
    }
    return fieldCst;
  }

  bool isBigEndian = MeOption::IsBigEndian() || Options::IsBigEndian();

  IntVal val = intCst->GetValue();
  uint8 bitSize = bitFieldType->GetFieldSize();

  if (bitSize >= val.GetBitWidth()) {
    return fieldCst;
  }

  if (isBigEndian) {
    val = val.LShr(val.GetBitWidth() - bitSize);
  } else {
    val = val & ((uint64(1) << bitSize) - 1);
  }

  return GlobalTables::GetIntConstTable().GetOrCreateIntConst(val, fieldCst->GetType());
}

} // namespace

// If size (in byte) is bigger than this threshold, we won't expand memop
const uint32 SimplifyOp::thresholdMemsetExpand = 512;
const uint32 SimplifyOp::thresholdMemcpyExpand = 512;
const uint32 SimplifyOp::thresholdMemsetSExpand = 1024;
const uint32 SimplifyOp::thresholdMemcpySExpand = 1024;
static const uint32 kMaxMemoryBlockSizeToAssign = 8;  // in byte

static void MayPrintLog(bool debug, bool success, OpKind opKind, const char *str) {
  if (!debug) {
    return;
  }
  const char *op = "";
  if (opKind == MEM_OP_memset) {
    op = "memset";
  } else if (opKind == MEM_OP_memcpy) {
    op = "memcpy";
  } else if (opKind == MEM_OP_memset_s) {
    op = "memset_s";
  } else if (opKind == MEM_OP_memcpy_s) {
    op = "memcpy_s";
  }
  LogInfo::MapleLogger() << op << " expand " << (success ? "success: " : "failure: ") << str << std::endl;
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
  simplifyMemOp.SetDebug(dump);
  simplifyMemOp.SetFunction(currFunc);
  if (simplifyMemOp.AutoSimplify(stmt, block, false)) {
    return;
  }
}

constexpr uint32 kUpperLimitOfFieldNum = 10;
static MIRStructType *GetDassignedStructType(const DassignNode *dassign, MIRFunction *func) {
  const auto &lhsStIdx = dassign->GetStIdx();
  auto lhsSymbol = func->GetLocalOrGlobalSymbol(lhsStIdx);
  ASSERT_NOT_NULL(lhsSymbol);
  auto lhsAggType = lhsSymbol->GetType();
  ASSERT_NOT_NULL(lhsAggType);
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
  ASSERT_NOT_NULL(rhsSymbol);
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
static StmtNode *SplitAggCopy(const AssignType *assignNode, MIRStructType *structureType,
                              BlockNode *block, MIRFunction *func) {
  auto *readNode = static_cast<RhsType *>(assignNode->GetRHS());
  auto rhsFieldID = readNode->GetFieldID();
  auto *rhsAggType = GetReadedStructureType(readNode, func);
  if (structureType != rhsAggType) {
    return nullptr;
  }

  for (FieldID id = 1; id <= static_cast<FieldID>(structureType->NumberOfFieldIDs()); ++id) {
    MIRType *fieldType = structureType->GetFieldType(id);
    if (fieldType->GetSize() == 0) {
      continue; // field size is zero for empty struct/union;
    }
    if (fieldType->GetKind() == kTypeBitField && static_cast<MIRBitFieldType *>(fieldType)->GetFieldSize() == 0) {
      continue; // bitfield size is zero
    }
    auto *newAssign = assignNode->CloneTree(func->GetCodeMemPoolAllocator());
    newAssign->SetFieldID(assignNode->GetFieldID() + id);
    auto *newRHS = static_cast<RhsType *>(newAssign->GetRHS());
    newRHS->SetFieldID(rhsFieldID + id);
    newRHS->SetPrimType(fieldType->GetPrimType());
    block->InsertAfter(assignNode, newAssign);
    newAssign->SetExpandFromArrayOfCharFunc(assignNode->IsExpandedFromArrayOfCharFunc());
    if (fieldType->IsMIRUnionType()) {
      id += static_cast<int32>(fieldType->NumberOfFieldIDs());
    }
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
    ASSERT_NOT_NULL(lhsSymbol);
    ASSERT_NOT_NULL(rhsSymbol);
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

struct BitFieldExtract {
  int64 byteOffset = 0;
  int64 extractStart = 0;
  int64 extractSize = 0;
  MIRType *extractType = nullptr;
};

static bool ExtractBitField(const MIRPtrType &type, FieldID fldID, BitFieldExtract &bfe) {
  // skip volatile field and packed struct
  if (type.IsPointedTypeVolatile(fldID)) {
    return false;
  }
  auto *fieldType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(type.GetPointedTyIdxWithFieldID(fldID));
  ASSERT_NOT_NULL(fieldType);
  if (fieldType->GetKind() != kTypeBitField || GetPrimTypeSize(fieldType->GetPrimType()) > GetPrimTypeSize(PTY_u64)) {
    return false;
  }
  auto bitOffset = type.GetPointedType()->GetBitOffsetFromBaseAddr(fldID);
  auto extractSize = static_cast<MIRBitFieldType*>(fieldType)->GetFieldSize();
  if ((bitOffset / LLONG_WIDTH) != ((bitOffset + extractSize) / LLONG_WIDTH)) {
    return false;
  }
  if (bitOffset % CHAR_WIDTH == 0 && (extractSize == CHAR_WIDTH || extractSize == SHRT_WIDTH ||
                                      extractSize == INT_WIDTH || extractSize == LLONG_WIDTH)) {
    return false;
  }
  auto byteOffset = (bitOffset / LLONG_WIDTH) * CHAR_WIDTH;  // expand the read length to 64 bit
  auto *readType = GlobalTables::GetTypeTable().GetUInt64();
  if ((bitOffset / INT_WIDTH) == ((bitOffset + extractSize) / INT_WIDTH)) {
    byteOffset = (bitOffset / INT_WIDTH) * INT_WIDTH / CHAR_WIDTH;  // expand the read length to 32 bit
    readType = GlobalTables::GetTypeTable().GetUInt32();
  }
  bfe.byteOffset = byteOffset;
  bfe.extractStart = bitOffset - byteOffset * CHAR_WIDTH;
  bfe.extractSize = extractSize;
  bfe.extractType = readType;
  return true;
}

// Bitfield can not write directly, when write 2 bitfields that belong to the same 4-bytes memory,
// we can expose the 4-bytes memory's read & write to remove partial/fully redundant. This function
// lowers bitfield write to `4-bytes memory's read + bits insert + 4-bytes memory write`.
StmtNode *Simplify::SimplifyBitFieldWrite(const IassignNode &iass) {
  if (iass.GetFieldID() == 0) {
    return nullptr;
  }
  auto *type = static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(iass.GetTyIdx()));
  BitFieldExtract bfe;
  if (!ExtractBitField(*type, iass.GetFieldID(), bfe)) {
    return nullptr;
  }
  CHECK_NULL_FATAL(bfe.extractType);
  auto *newIvarType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*bfe.extractType);
  auto *base = iass.Opnd(0);
  MIRBuilder *mirBuiler = currFunc->GetModule()->GetMIRBuilder();
  if (bfe.byteOffset != 0) {
    base = mirBuiler->CreateExprBinary(OP_add, PTY_a64, base,
        mirBuiler->CreateIntConst(static_cast<uint64>(bfe.byteOffset), PTY_a64));
  }
  auto *readIvar = mirBuiler->CreateExprIread(*bfe.extractType, *newIvarType, 0, base);

  if (MeOption::IsBigEndian()) {
    auto extractSize = GetPrimTypeBitSize(bfe.extractType->GetPrimType());
    ASSERT(bfe.extractStart + bfe.extractSize <= extractSize, "bit offset is out-of-range");
    bfe.extractStart = static_cast<int64>(extractSize) - (bfe.extractStart + bfe.extractSize);
  }

  auto *deposit = mirBuiler->CreateExprDepositbits(OP_depositbits, bfe.extractType->GetPrimType(),
      static_cast<uint32>(bfe.extractStart), static_cast<uint32>(bfe.extractSize), readIvar, iass.GetRHS());
  auto *newIass = mirBuiler->CreateStmtIassign(*newIvarType, 0, base, deposit);
  currBlock->ReplaceStmt1WithStmt2(&iass, newIass);
  return newIass;
}

void Simplify::ProcessStmt(StmtNode &stmt) {
  switch (stmt.GetOpCode()) {
    case OP_callassigned: {
      SimplifyCallAssigned(stmt, *currBlock);
      break;
    }
    case OP_intrinsiccall: {
      simplifyMemOp.SetDebug(dump);
      simplifyMemOp.SetFunction(currFunc);
      (void)simplifyMemOp.AutoSimplify(stmt, *currBlock, false);
      break;
    }
    case OP_dassign: {
      auto *newStmt = SplitDassignAggCopy(static_cast<DassignNode *>(&stmt), currBlock, currFunc);
      if (newStmt) {
        ProcessBlock(*newStmt);
      }
      break;
    }
    case OP_iassign: {
      auto *newStmt = SimplifyBitFieldWrite(static_cast<IassignNode&>(stmt));
      if (newStmt) {
        break;
      }
      newStmt = SplitIassignAggCopy(static_cast<IassignNode *>(&stmt), currBlock, currFunc);
      if (newStmt) {
        ProcessBlock(*newStmt);
      }
      break;
    }
    case OP_if:
    case OP_while:
    case OP_dowhile: {
      auto unaryStmt = static_cast<UnaryStmtNode&>(stmt);
      unaryStmt.SetRHS(SimplifyExpr(*unaryStmt.GetRHS()));
      return;
    }
    default: {
      break;
    }
  }
  for (size_t i = 0; i < stmt.NumOpnds(); ++i) {
    if (stmt.Opnd(i)) {
      stmt.SetOpnd(SimplifyExpr(*stmt.Opnd(i)), i);
    }
  }
}

BaseNode *Simplify::SimplifyExpr(BaseNode &expr) {
  switch (expr.GetOpCode()) {
    case OP_dread: {
      auto &dread = static_cast<DreadNode&>(expr);
      return ReplaceExprWithConst(dread);
    }
    case OP_iread: {
      return SimplifyBitFieldRead(static_cast<IreadNode&>(expr));
    }
    default: {
      for (auto i = 0; i < expr.GetNumOpnds(); i++) {
        if (expr.Opnd(i)) {
          expr.SetOpnd(SimplifyExpr(*expr.Opnd(i)), i);
        }
      }
      break;
    }
  }
  return &expr;
}

// Bitfield can not read directly, when read 2 bitfields that belong to the same 4-bytes memory,
// we can expose the 4-bytes memory's read to remove partial/fully redundant. This function lowers
// bitfield read to `4-bytes memory's read + bits extract.
BaseNode *Simplify::SimplifyBitFieldRead(IreadNode &iread) {
  if (iread.GetFieldID() == 0) {
    return &iread;
  }
  auto *type = static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(iread.GetTyIdx()));
  BitFieldExtract bfe;
  if (!ExtractBitField(*type, iread.GetFieldID(), bfe)) {
    return &iread;
  }
  CHECK_NULL_FATAL(bfe.extractType);
  auto *fieldType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(type->GetPointedTyIdxWithFieldID(iread.GetFieldID()));
  auto extractPtyp = IsSignedInteger(fieldType->GetPrimType()) ?
      GetSignedPrimType(bfe.extractType->GetPrimType()) : bfe.extractType->GetPrimType();
  auto *newIvarType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*bfe.extractType);
  auto *base = iread.Opnd(0);
  MIRBuilder *mirBuiler = currFunc->GetModule()->GetMIRBuilder();
  if (bfe.byteOffset != 0) {
    base = mirBuiler->CreateExprBinary(OP_add, PTY_a64, base,
        mirBuiler->CreateIntConst(static_cast<uint64>(bfe.byteOffset), PTY_a64));
  }
  auto *readIvar = mirBuiler->CreateExprIread(*bfe.extractType, *newIvarType, 0, base);

  if (MeOption::IsBigEndian()) {
    ASSERT(bfe.extractStart + bfe.extractSize <= GetPrimTypeBitSize(extractPtyp), "bit offset is out-of-range");
    bfe.extractStart = static_cast<int64>(GetPrimTypeBitSize(extractPtyp)) - (bfe.extractStart + bfe.extractSize);
  }

  auto *extract = mirBuiler->CreateExprExtractbits(OP_extractbits, extractPtyp, static_cast<uint32>(bfe.extractStart),
                                                   static_cast<uint32>(bfe.extractSize), readIvar);
  if (extractPtyp != iread.GetPrimType()) {
    return mirBuiler->CreateExprTypeCvt(OP_cvt, iread.GetPrimType(), extractPtyp, *extract);
  }
  return extract;
}

BaseNode *Simplify::ReplaceExprWithConst(DreadNode &dread) {
  auto stIdx = dread.GetStIdx();
  auto fieldId = dread.GetFieldID();
  auto *symbol = currFunc->GetLocalOrGlobalSymbol(stIdx);
  ASSERT_NOT_NULL(symbol);
  auto *symbolConst = symbol->GetKonst();
  if (!currFunc->GetModule()->IsCModule() || !symbolConst ||
      !stIdx.IsGlobal() || !IsSymbolReplaceableWithConst(*symbol)) {
    return &dread;
  }
  if (fieldId != 0) {
    symbolConst = GetElementConstFromFieldId(fieldId, *symbolConst);
  }
  if (!symbolConst || !IsConstRepalceable(*symbolConst)) {
    return &dread;
  }
  // bitfield need extension
  if (fieldId != 0) {
    MIRType *fldType = static_cast<MIRStructType*>(symbol->GetType())->GetFieldType(fieldId);
    if (fldType->IsMIRBitFieldType()) {
      IntVal v = static_cast<MIRIntConst*>(symbolConst)->GetValue();
      uint8 fldSize = static_cast<MIRBitFieldType*>(fldType)->GetFieldSize();
      symbolConst =
          GlobalTables::GetIntConstTable().GetOrCreateIntConst(v.GetExtValue(fldSize), symbolConst->GetType());
    }
  }
  return currFunc->GetModule()->GetMIRBuilder()->CreateConstval(symbolConst);
}

bool Simplify::IsSymbolReplaceableWithConst(const MIRSymbol &symbol) const {
  return (symbol.GetStorageClass() == kScFstatic && !symbol.HasPotentialAssignment()) ||
      symbol.GetAttrs().GetAttr(ATTR_const);
}

bool Simplify::IsConstRepalceable(const MIRConst &mirConst) const {
  switch (mirConst.GetKind()) {
    case kConstInt:
    case kConstFloatConst:
    case kConstDoubleConst:
    case kConstFloat128Const:
    case kConstLblConst:
      return true;
    default:
      return false;
  }
}

MIRConst *Simplify::GetElementConstFromFieldId(FieldID fieldId, MIRConst &mirConst) {
  FieldID currFieldId = 1;
  MIRConst *resultConst = nullptr;
  auto originAggConst = static_cast<MIRAggConst &>(mirConst);
  auto originAggType = static_cast<MIRStructType &>(originAggConst.GetType());
  bool reached = false;
  bool isUpperLayerUnion = false;
  std::function<void(MIRConst *, MIRType *)> traverseAgg = [&] (MIRConst *currConst, MIRType *currType) {
    if (isUpperLayerUnion && (!currConst || currConst->GetKind() != kConstAggConst)) {
      reached = currFieldId == fieldId;
      resultConst = reached ? currConst : resultConst;
      return;
    }
    auto *currAggConst = safe_cast<MIRAggConst>(currConst);
    auto *currAggType = safe_cast<MIRStructType>(currType);
    ASSERT_NOT_NULL(currAggConst);
    ASSERT_NOT_NULL(currAggType);
    for (size_t iter = 0; iter < currAggType->GetFieldsSize() && !reached; ++iter) {
      auto *fieldConst = currAggConst->GetAggConstElement(iter + 1);
      auto *fieldType = originAggType.GetFieldType(currFieldId);

      if (currFieldId == fieldId) {
        if (auto *truncCst = TruncateUnionConstant(*currAggType, fieldConst, *fieldType)) {
          resultConst = truncCst;
        } else {
          resultConst = TruncateUnionConstant(*currAggType, fieldConst, *fieldType);
        }
        reached = true;
        return;
      }

      ++currFieldId;
      if (fieldType->GetKind() == kTypeUnion || fieldType->GetKind() == kTypeStruct) {
        bool isPrevUpperLayerUnion = isUpperLayerUnion;
        isUpperLayerUnion = currAggType->GetKind() == kTypeUnion;
        traverseAgg(fieldConst, fieldType);
        isUpperLayerUnion = isPrevUpperLayerUnion;
      }
    }
  };
  traverseAgg(&mirConst, &mirConst.GetType());
  if (!reached) {
    auto fieldPrimType = originAggType.GetFieldType(fieldId)->GetPrimType();
    auto resultPrimType = IsPrimitiveInteger(fieldPrimType) ? fieldPrimType :
        GetPrimTypeBitSize(fieldPrimType) > k32BitSize ? PTY_u64 : PTY_u32;
    auto *resultType = GlobalTables::GetTypeTable().GetPrimType(resultPrimType);
    resultConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, *resultType);
  }
  return resultConst;
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
  const uint8 firstByte = 0xff;

  uint64 realByte = static_cast<uint64>(byte & firstByte);
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
static BaseNode *FoldIntConst(BaseNode *expr, uint64 &out, bool &isIntConst) {
  if (expr->GetOpCode() == OP_constval) {
    MIRConst *mirConst = static_cast<ConstvalNode*>(expr)->GetConstVal();
    if (mirConst->GetKind() == kConstInt) {
      out = static_cast<uint64>(static_cast<MIRIntConst*>(mirConst)->GetExtValue());
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
      out = static_cast<uint64>(static_cast<MIRIntConst*>(mirConst)->GetExtValue());
      isIntConst = true;
    }
  }
  return foldExpr;
}

static BaseNode *ConstructConstvalNode(uint64 val, PrimType primType, MIRBuilder &mirBuilder) {
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

static BaseNode *ConstructConstvalNode(int64 byte, uint64 num, PrimType primType, MIRBuilder &mirBuilder) {
  auto val = JoinBytes(byte, static_cast<uint32>(num));
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
    blocks.insert(blocks.cend(), n, curBlockSize);
    leftSize -= (n * curBlockSize);
    curBlockSize = curBlockSize >> 1;
  }
}

static bool IsComplexExpr(const BaseNode *expr, MIRFunction &func) {
  Opcode op = expr->GetOpCode();
  if (op == OP_regread) {
    return false;
  }
  if (op == OP_dread) {
    auto *symbol = func.GetLocalOrGlobalSymbol(static_cast<const DreadNode*>(expr)->GetStIdx());
    ASSERT_NOT_NULL(symbol);
    if (symbol->IsGlobal() || symbol->GetStorageClass() == kScPstatic) {
      return true;  // dread global/static var is complex expr because it will be lowered to adrp + add
    } else {
      return false;
    }
  }
  if (op == OP_addrof) {
    auto *symbol = func.GetLocalOrGlobalSymbol(static_cast<const AddrofNode*>(expr)->GetStIdx());
    ASSERT_NOT_NULL(symbol);
    if (symbol->IsGlobal() || symbol->GetStorageClass() == kScPstatic) {
      return true;  // addrof global/static var is complex expr because it will be lowered to adrp + add
    } else {
      return false;
    }
  }
  return true;
}

// if addr is never nullptr
static bool IsAddrSafe(const BaseNode &expr) {
  if (expr.GetOpCode() == OP_addrof) {
    return true;
  }
  if (expr.GetOpCode() == OP_iaddrof) {
    auto iaddrExpr = static_cast<const IaddrofNode &>(expr);
    auto type = static_cast<MIRPtrType *>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(iaddrExpr.GetTyIdx()))
                    ->GetPointedType();
    if (type->IsMIRStructType()) {
      return static_cast<MIRStructType *>(type)->GetBitOffsetFromBaseAddr(iaddrExpr.GetFieldID()) != 0;
    }
  }
  return false;
}

// Input a address expr, output a memEntry to abstract this expr
bool MemEntry::ComputeMemEntry(BaseNode &expr, MIRFunction &func, MemEntry &memEntry, bool isLowLevel) {
  Opcode op = expr.GetOpCode();
  MIRType *mirType = nullptr;
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
        mirType = static_cast<MIRPtrType*>(curType)->GetPointedType();
      }
      break;
    }
    case OP_addrof: {
      const auto &concreteExpr = static_cast<const AddrofNode&>(expr);
      auto *symbol = func.GetLocalOrGlobalSymbol(concreteExpr.GetStIdx());
      MIRType *curType = symbol->GetType();
      if (concreteExpr.GetFieldID() != 0) {
        ASSERT_NOT_NULL(curType);
        curType = static_cast<MIRStructType*>(curType)->GetFieldType(concreteExpr.GetFieldID());
      }
      mirType = curType;
      break;
    }
    case OP_iread: {
      const auto &concreteExpr = static_cast<const IreadNode&>(expr);
      mirType = concreteExpr.GetType();
      break;
    }
    case OP_iaddrof: {  // Do NOT call GetType because it is for OP_iread
      const auto &concreteExpr = static_cast<const IaddrofNode&>(expr);
      MIRType *curType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(concreteExpr.GetTyIdx());
      CHECK_FATAL(curType->IsMIRPtrType(), "must be MIRPtrType");
      curType = static_cast<MIRPtrType*>(curType)->GetPointedType();
      CHECK_FATAL(curType->IsStructType(), "must be MIRStructType");
      mirType = static_cast<MIRStructType*>(curType)->GetFieldType(concreteExpr.GetFieldID());
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
        mirType = curType;
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
  if (mirType == nullptr) {
    return false;
  }
  memEntry.addrExpr = &expr;
  memEntry.memType = mirType;
  return true;
}

BaseNode *MemEntry::BuildAsRhsExpr(MIRFunction &func, MIRType *accessType) const {
  BaseNode *expr = nullptr;
  MIRBuilder *mirBuilder = func.GetModule()->GetMIRBuilder();
  if (addrExpr->GetOpCode() == OP_addrof) {
    // We prefer dread to iread
    // consider iaddrof if possible
    auto *addrof = static_cast<AddrofNode*>(addrExpr);
    auto *symbol = func.GetLocalOrGlobalSymbol(addrof->GetStIdx());
    expr = mirBuilder->CreateExprDread(*memType, addrof->GetFieldID(), *symbol);
  } else {
    if (accessType == nullptr) {
      accessType = memType;
    }
    MIRType *structPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*accessType);
    expr = mirBuilder->CreateExprIread(*accessType, *structPtrType, 0, addrExpr);
  }
  return expr;
}

static void InsertBeforeAndMayPrintStmt(BlockNode &block, const StmtNode &anchor, bool debug, StmtNode *stmt) {
  if (stmt == nullptr) {
    return;
  }
  block.InsertBefore(&anchor, stmt);
  if (debug) {
    stmt->Dump(0);
  }
}

static void InsertBeforeAndMayPrintStmtList(BlockNode &block, const StmtNode &anchor,
                                      bool debug, std::initializer_list<StmtNode*> stmtList) {
  for (StmtNode *stmt : stmtList) {
    if (stmt == nullptr) {
      continue;
    }
    block.InsertBefore(&anchor, stmt);
    if (debug) {
      stmt->Dump(0);
    }
  }
}

class BlockOperationHelper {
 public:
  BlockOperationHelper(StmtNode &stmtNode, MIRFunction &mirFunc, BlockNode &blockNode, bool lowLevel, bool isDebug)
      : stmt(stmtNode),
        func(mirFunc),
        block(blockNode),
        mirBuilder(func.GetModule()->GetMIRBuilder()),
        isLowLevel(lowLevel),
        debug(isDebug) {}

  void InsertLableNode(LabelIdx labIdx) {
    auto *labelNode = mirBuilder->CreateStmtLabel(labIdx);
    InsertBeforeAndMayPrintStmt(block, stmt, debug, labelNode);
  }

  // dassign %ret (constval i32 errNumber)
  // goto @labIdx
  void InsertRetAssignAndGoto(LabelIdx labIdx, OpKind memOpKind, ErrorNumber errNumber) {
    auto retAssign = MemEntry::GenRetAssign(stmt, func, isLowLevel, memOpKind, errNumber);
    auto *gotoLabel = mirBuilder->CreateStmtGoto(OP_goto, labIdx);
    InsertBeforeAndMayPrintStmtList(block, stmt, debug, {retAssign, gotoLabel});
  }

  // memset of the addrExpr with c
  StmtNode *InsertMemsetCallStmt(BaseNode *addrExpr, BaseNode *c) {
    MapleVector<BaseNode *> args(mirBuilder->GetCurrentFuncCodeMpAllocator()->Adapter());
    args.push_back(addrExpr);
    args.push_back(c);
    args.push_back(stmt.Opnd(kMemOpSDstSizeOpndIdx));
    auto memsetFunc = mirBuilder->GetOrCreateFunction(kFuncNameOfMemset, TyIdx(PTY_void));
    auto *callMemset = mirBuilder->CreateStmtCallAssigned(memsetFunc->GetPuidx(), args, nullptr, OP_callassigned);
    callMemset->SetSrcPos(stmt.GetSrcPos());
    InsertBeforeAndMayPrintStmt(block, stmt, debug, callMemset);
    return callMemset;
  }

  // brtrue @label (op lhs rhs)
  void CreateAndInsertCheckStmt(Opcode op, BaseNode *lhs, BaseNode *rhs, LabelIdx label) {
    auto cmpResType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_u8));
    auto cmpU64Type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_u64));
    auto cmpStmt = mirBuilder->CreateExprCompare(op, *cmpResType, *cmpU64Type, lhs, rhs);
    auto checkStmt = mirBuilder->CreateStmtCondGoto(cmpStmt, OP_brtrue, label);
    checkStmt->SetBranchProb(kProbUnlikely);
    checkStmt->SetSrcPos(stmt.GetSrcPos());
    InsertBeforeAndMayPrintStmt(block, stmt, debug, checkStmt);
  }

  // @label
  // dassign %ret (constval i32 errnNumber)
  // goto @finalLab
  void HandleError(LabelIdx branchLabIdx, LabelIdx finalLabIdx, ErrorNumber errNumber, OpKind memOpKind) {
    InsertLableNode(branchLabIdx);
    InsertRetAssignAndGoto(finalLabIdx, memOpKind, errNumber);
  }

  // @curLab
  // brture @condLab (eq (dread dstSizeOpnd, constval u64 0))
  // dassign %ret (constval i32 errnNumber)
  // goto @finalLab
  void HandleErrorWithDstSizeCheck(LabelIdx curLabIdx, LabelIdx condLabIdx, LabelIdx finalLabIdx, OpKind memOpKind,
                                   ErrorNumber errNumer, bool isDstSizeConst, int64 dstSize) {
    InsertLableNode(curLabIdx);
    if (!isDstSizeConst) {
      CreateAndInsertCheckStmt(OP_eq, stmt.Opnd(kMemOpSDstSizeOpndIdx), ConstructConstvalNode(0, PTY_u64, *mirBuilder),
                               condLabIdx);
    } else if (dstSize == 0) {
      InsertRetAssignAndGoto(finalLabIdx, memOpKind, ERRNO_RANGE);
    }
    InsertRetAssignAndGoto(finalLabIdx, memOpKind, errNumer);
  }

  // @label
  // brture @condLab (eq (dstSizeOpnd, constval u64 0))
  // call &memset()
  // dassign %ret (constval i32 errnNumber)
  // goto @finalLab
  void HandleErrorWithDstSizeCheckAndReset(LabelIdx curLabIdx, LabelIdx condLabIdx, LabelIdx finalLabIdx,
                                           OpKind memOpKind, ErrorNumber errNumer, bool isDstSizeConst, int64 dstSize) {
    InsertLableNode(curLabIdx);
    if (!isDstSizeConst) {
      CreateAndInsertCheckStmt(OP_eq, stmt.Opnd(kMemOpSDstSizeOpndIdx), ConstructConstvalNode(0, PTY_u64, *mirBuilder),
                               condLabIdx);
    } else if (dstSize == 0) {
      InsertRetAssignAndGoto(finalLabIdx, memOpKind, ERRNO_RANGE);
    }
    (void)InsertMemsetCallStmt(stmt.Opnd(kMemOpDstOpndIdx), stmt.Opnd(kMemOpSSrcOpndIdx));
    InsertRetAssignAndGoto(finalLabIdx, memOpKind, errNumer);
  }

  // @label
  // call &memset()
  // dassign %ret (constval i32 errnNumber)
  // goto @finalLab
  void HandleErrorAndReset(LabelIdx curLabIdx, LabelIdx finalLabIdx, OpKind memOpKind, ErrorNumber errNumer) {
    InsertLableNode(curLabIdx);
    (void)InsertMemsetCallStmt(stmt.Opnd(kMemOpDstOpndIdx), ConstructConstvalNode(0, PTY_i32, *mirBuilder));
    InsertRetAssignAndGoto(finalLabIdx, memOpKind, errNumer);
  }

 private:
  StmtNode &stmt;
  MIRFunction &func;
  BlockNode &block;
  MIRBuilder *mirBuilder;
  bool isLowLevel;
  bool debug;
};

// Create maple IR to check whether `expr1` and `expr2` are overlapped
// brfalse @@o1 (ge u8 ptr (
//   abs ptr (sub ptr (regread ptr %1, regread ptr %2)),
//   constval u64 xxx))
static CondGotoNode *CreateOverlapCheckStmt(BaseNode &expr1, BaseNode &expr2, BaseNode &size, LabelIdx labIdx,
    MIRBuilder *mirBuilder) {
  auto cmpResType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_u8));
  auto cmpU64Type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_ptr));
  auto cmpI64Type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_i64));
  auto *checkExpr = mirBuilder->CreateExprCompare(OP_ge, *cmpResType, *cmpU64Type,
      mirBuilder->CreateExprUnary(OP_abs, *cmpU64Type,
          mirBuilder->CreateExprBinary(OP_sub, *cmpI64Type, &expr1, &expr2)),
      &size);
  auto *checkStmt = mirBuilder->CreateStmtCondGoto(checkExpr, OP_brfalse, labIdx);
  return checkStmt;
}

static BaseNode *TryToExtractComplexExpr(BaseNode *expr, MIRFunction &func, BlockNode &block,
                                         const StmtNode &anchor, bool debug) {
  if (!IsComplexExpr(expr, func)) {
    return expr;
  }
  MIRBuilder *mirBuilder = func.GetModule()->GetMIRBuilder();
  auto pregIdx = func.GetPregTab()->CreatePreg(PTY_ptr);
  StmtNode *regassign = mirBuilder->CreateStmtRegassign(PTY_ptr, pregIdx, expr);
  InsertBeforeAndMayPrintStmt(block, anchor, debug, regassign);
  auto *extractedExpr = mirBuilder->CreateExprRegread(PTY_ptr, pregIdx);
  return extractedExpr;
}

static BaseNode *TryExtractExpr(BaseNode &expr, MIRFunction &func, BlockNode &block, const StmtNode &anchor, bool isSrc,
                                bool debug) {
  static int extractIdx = 0;
  if (IsComplexExpr(&expr, func)) {
    auto mirBuilder = func.GetModule()->GetMIRBuilder();
    MemEntry dstMemEntry;
    bool valid = MemEntry::ComputeMemEntry(expr, func, dstMemEntry, false);
    if (!valid) {
      return nullptr;
    }
    // if src expr is array type, do not extract it to avoid redundant inst
    if (isSrc && dstMemEntry.memType->IsMIRArrayType()) {
      return nullptr;
    }
    std::string tempAddrName(kUnderlineStr);
    (void)tempAddrName.append(std::to_string(func.GetPuidx()))
        .append(kUnderlineStr)
        .append(std::to_string(anchor.GetStmtID()))
        .append(kVerticalLineStr)
        .append(kTempAddrStr)
        .append(kUnderlineStr)
        .append(std::to_string(extractIdx++));
    if (GlobalTables::GetStrTable().GetStrIdxFromName(tempAddrName) != 0) {
      return nullptr;
    }
    MIRPtrType ty(dstMemEntry.memType->GetTypeIndex());
    auto tIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&ty);
    auto symb = mirBuilder->CreateSymbol(tIdx, tempAddrName, kStVar, kScAuto, &func, kScopeLocal);
    symb->SetIsTmp(true);
    auto dAssignStmt = mirBuilder->CreateStmtDassign(symb->GetStIdx(), 0, &expr);
    InsertBeforeAndMayPrintStmt(block, anchor, debug, dAssignStmt);
    return mirBuilder->CreateExprDread(*symb);
  }
  return nullptr;
}

void MemEntry::ExpandMemsetLowLevel(int64 byte, uint64 size, MIRFunction &func, StmtNode &stmt, BlockNode &block,
                                    OpKind memOpKind, bool debug, ErrorNumber errorNumber) const {
  MIRBuilder *mirBuilder = func.GetModule()->GetMIRBuilder();
  std::vector<uint32> blocks;
  SplitMemoryIntoBlocks(size, blocks);
  int32 offset = 0;
  // If blocks.size() > 1 and `dst` is not a leaf node,
  // we should extract common expr to avoid redundant expression
  BaseNode *realDstExpr = addrExpr;
  if (blocks.size() > 1) {
    realDstExpr = TryToExtractComplexExpr(addrExpr, func, block, stmt, debug);
  }
  BaseNode *readConst = nullptr;
  // rhs const is big, extract it to avoid redundant expression
  bool shouldExtractRhs = blocks.size() > 1 && (byte & 0xff) != 0;
  for (auto curSize : blocks) {
    // low level memset expand result:
    //   iassignoff <prim-type> <offset> (dstAddrExpr, constval <prim-type> xx)
    PrimType constType = GetIntegerPrimTypeBySizeAndSign(curSize * 8, false);
    BaseNode *rhsExpr = ConstructConstvalNode(byte, curSize, constType, *mirBuilder);
    if (shouldExtractRhs) {
      // we only need to extract u64 const once
      PregIdx pregIdx = func.GetPregTab()->CreatePreg(constType);
      auto *constAssign = mirBuilder->CreateStmtRegassign(constType, pregIdx, rhsExpr);
      InsertBeforeAndMayPrintStmt(block, stmt, debug, constAssign);
      readConst = mirBuilder->CreateExprRegread(constType, pregIdx);
      shouldExtractRhs = false;
    }
    if (readConst != nullptr && curSize == kMaxMemoryBlockSizeToAssign) {
      rhsExpr = readConst;
    }
    auto *iassignoff = mirBuilder->CreateStmtIassignoff(constType, offset, realDstExpr, rhsExpr);
    InsertBeforeAndMayPrintStmt(block, stmt, debug, iassignoff);
    if (debug) {
      iassignoff->Dump(0);
    }
    offset += static_cast<int32>(curSize);
  }
  // handle memset return val
  auto *retAssign = GenRetAssign(stmt, func, true, memOpKind, errorNumber);
  InsertBeforeAndMayPrintStmt(block, stmt, debug, retAssign);
  // return ERRNO_INVAL if memset_s dest is NULL
  block.RemoveStmt(&stmt);
}

// Lower memset(MemEntry, byte, size) into a series of assign stmts and replace callStmt in the block
// with these assign stmts
bool MemEntry::ExpandMemset(int64 byte, uint64 size, MIRFunction &func,
                            StmtNode &stmt, BlockNode &block, bool isLowLevel, bool debug,
                            ErrorNumber errorNumber) const {
  OpKind memOpKind = SimplifyOp::ComputeOpKind(stmt) ;
  MemEntryKind memKind = GetKind();
  // we don't check size equality in the low level expand
  if (!isLowLevel) {
    if (memKind == kMemEntryUnknown) {
      MayPrintLog(debug, false, memOpKind, "unsupported dst memory type, is it a bitfield?");
      return false;
    }
    if (memType->GetSize() != size) {
      MayPrintLog(debug, false, memOpKind, "dst size and size arg are not equal");
      return false;
    }
  }
  MIRBuilder *mirBuilder = func.GetModule()->GetMIRBuilder();

  if (isLowLevel) {  // For cglower, replace memset with a series of low-level iassignoff
    ExpandMemsetLowLevel(byte, size, func, stmt, block, memOpKind, debug, errorNumber);
    return true;
  }

  if (memKind == kMemEntryPrimitive) {
    BaseNode *rhsExpr = ConstructConstvalNode(byte, size, memType->GetPrimType(), *mirBuilder);
    StmtNode *newAssign = nullptr;
    if (addrExpr->GetOpCode() == OP_addrof) {  // We prefer dassign to iassign
      auto *addrof = static_cast<AddrofNode*>(addrExpr);
      auto *symbol = func.GetLocalOrGlobalSymbol(addrof->GetStIdx());
      newAssign = mirBuilder->CreateStmtDassign(*symbol, addrof->GetFieldID(), rhsExpr);
    } else {
      MIRType *memPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*memType);
      newAssign = mirBuilder->CreateStmtIassign(*memPtrType, 0, addrExpr, rhsExpr);
      static_cast<IassignNode *>(newAssign)->SetExpandFromArrayOfCharFunc(true);
    }
    InsertBeforeAndMayPrintStmt(block, stmt, debug, newAssign);
  } else if (memKind == kMemEntryStruct) {
    auto *structType = static_cast<MIRStructType*>(memType);
    // struct size should be small enough, struct field size should be big enough
    constexpr uint32 maxStructSize = 64;  // in byte
    constexpr uint32 minFieldSize = 4;    // in byte
    size_t structSize = structType->GetSize();
    size_t numFields = structType->NumberOfFieldIDs();
    // Relax restrictions when store-merge is powerful enough
    if (numFields != 0) {
      bool expandIt = (structSize <= maxStructSize && (structSize / numFields >= minFieldSize) &&
                       !structType->HasPadding());
      if (!expandIt) {
        // We only expand memset for no-padding struct, because only in this case, element-wise and byte-wise
        // are equivalent
        MayPrintLog(debug, false, memOpKind,
                    "struct type has padding, or struct sum size is too big, or filed size is too small");
        return false;
      }
    }
    bool hasArrayField = false;
    for (uint32 id = 1; id <= numFields; ++id) {
      auto *fieldType = structType->GetFieldType(id);
      if (fieldType->GetKind() == kTypeArray) {
        hasArrayField = true;
        break;
      }
    }
    if (hasArrayField) {
      // struct with array fields is not supported to expand for now, enhance it when needed
      MayPrintLog(debug, false, memOpKind, "struct with array fields is not supported to expand");
      return false;
    }

    // Build assign for each fields in the struct type
    // We should skip union fields
    for (FieldID id = 1; static_cast<size_t>(id) <= numFields; ++id) {
      MIRType *fieldType = structType->GetFieldType(id);
      // We only consider leaf field with valid type size
      if (fieldType->GetSize() == 0 || fieldType->GetPrimType() == PTY_agg) {
        continue;
      }
      if (fieldType->GetKind() == kTypeBitField && static_cast<MIRBitFieldType*>(fieldType)->GetFieldSize() == 0) {
        continue;
      }
      // now the fieldType is primitive type
      BaseNode *rhsExpr = ConstructConstvalNode(byte, fieldType->GetSize(),
                                                fieldType->GetPrimType(), *mirBuilder);
      StmtNode *fieldAssign = nullptr;
      if (addrExpr->GetOpCode() == OP_addrof) {
        auto *addrof = static_cast<AddrofNode*>(addrExpr);
        auto *symbol = func.GetLocalOrGlobalSymbol(addrof->GetStIdx());
        fieldAssign = mirBuilder->CreateStmtDassign(*symbol, addrof->GetFieldID() + id, rhsExpr);
      } else {
        MIRType *memPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*memType);
        fieldAssign = mirBuilder->CreateStmtIassign(*memPtrType, id, addrExpr, rhsExpr);
        static_cast<IassignNode *>(fieldAssign)->SetExpandFromArrayOfCharFunc(true);
      }
      InsertBeforeAndMayPrintStmt(block, stmt, debug, fieldAssign);
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
    uint64 elemCnt = static_cast<uint64>(arrayType->GetSizeArrayItem(0));
    if (elemType->GetSize() * elemCnt != size) {
      MayPrintLog(debug, false, memOpKind, "array size not equal");
      return false;
    }
    for (size_t i = 0; i < elemCnt; ++i) {
      BaseNode *indexExpr = ConstructConstvalNode(i, PTY_u32, *mirBuilder);
      auto *arrayExpr = mirBuilder->CreateExprArray(*arrayType, addrExpr, indexExpr);
      auto *newValOpnd = ConstructConstvalNode(byte, elemType->GetSize(),
                                               elemType->GetPrimType(), *mirBuilder);
      MIRType *elemPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*elemType);
      auto *arrayElementAssign = mirBuilder->CreateStmtIassign(*elemPtrType, 0, arrayExpr, newValOpnd);
      arrayElementAssign->SetExpandFromArrayOfCharFunc(true);
      InsertBeforeAndMayPrintStmt(block, stmt, debug, arrayElementAssign);
    }
  } else {
    CHECK_FATAL(false, "impossible");
  }

  // handle memset return val
  auto *retAssign = GenRetAssign(stmt, func, isLowLevel, memOpKind, errorNumber);
  InsertBeforeAndMayPrintStmt(block, stmt, debug, retAssign);
  block.RemoveStmt(&stmt);
  return true;
}

static std::pair<StmtNode*, StmtNode*> GenerateMemoryCopyPair(MIRBuilder *mirBuilder, BaseNode *rhs,
    BaseNode *lhs, uint32 offset, uint32 curSize, PregIdx tmpRegIdx) {
  auto *ptrType = GlobalTables::GetTypeTable().GetPtrType();
  PrimType constType = GetIntegerPrimTypeBySizeAndSign(curSize * 8, false);
  MIRType *constMIRType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(constType));
  auto *constMIRPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*constMIRType);
  BaseNode *rhsAddrExpr = rhs;
  if (offset != 0) {
    auto *offsetConstExpr = ConstructConstvalNode(offset, PTY_u64, *mirBuilder);
    rhsAddrExpr = mirBuilder->CreateExprBinary(OP_add, *ptrType, rhs, offsetConstExpr);
  }
  BaseNode *rhsExpr = mirBuilder->CreateExprIread(*constMIRType, *constMIRPtrType, 0, rhsAddrExpr);
  auto *regassign = mirBuilder->CreateStmtRegassign(PTY_u64, tmpRegIdx, rhsExpr);
  auto *iassignoff = mirBuilder->CreateStmtIassignoff(constType, static_cast<int32>(offset), lhs,
      mirBuilder->CreateExprRegread(PTY_u64, tmpRegIdx));
  return { regassign, iassignoff };
}

void MemEntry::ExpandMemcpyLowLevel(const MemEntry &srcMem, uint64 copySize, MIRFunction &func,
                                    StmtNode &stmt, BlockNode &block, OpKind memOpKind, bool debug,
                                    ErrorNumber errorNumber) const {
  if (errorNumber == ERRNO_RANGE) {
    auto *retAssign = GenRetAssign(stmt, func, true, memOpKind, errorNumber);
    InsertBeforeAndMayPrintStmt(block, stmt, debug, retAssign);
    block.RemoveStmt(&stmt);
    return;
  }
  MIRBuilder *mirBuilder = func.GetModule()->GetMIRBuilder();
  std::vector<uint32> blocks;
  SplitMemoryIntoBlocks(copySize, blocks);
  uint32 offset = 0;
  // If blocks.size() > 1 and `src` or `dst` is not a leaf node,
  // we should extract common expr to avoid redundant expression
  BaseNode *realSrcExpr = srcMem.addrExpr;
  BaseNode *realDstExpr = addrExpr;
  if (blocks.size() > 1) {
    realDstExpr = TryToExtractComplexExpr(addrExpr, func, block, stmt, debug);
    realSrcExpr = TryToExtractComplexExpr(srcMem.addrExpr, func, block, stmt, debug);
  }
  auto *ptrType = GlobalTables::GetTypeTable().GetPtrType();
  // memory copy optimization
  PregIdx tmpRegIdx1 = 0;
  PregIdx tmpRegIdx2 = 0;
  for (uint32 i = 0; i < blocks.size(); ++i) {
    uint32 curSize = blocks[i];
    bool canMergedWithNextSize = (i + 1 < blocks.size()) && blocks[i + 1] == curSize;
    if (!canMergedWithNextSize) {
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
      InsertBeforeAndMayPrintStmt(block, stmt, debug, iassignoff);
      offset += curSize;
      continue;
    }

    // merge two str/ldr into a stp/ldp
    if (tmpRegIdx1 == 0 || tmpRegIdx2 == 0) {
      tmpRegIdx1 = func.GetPregTab()->CreatePreg(PTY_u64);
      tmpRegIdx2 = func.GetPregTab()->CreatePreg(PTY_u64);
    }
    auto pair1 = GenerateMemoryCopyPair(mirBuilder, realSrcExpr, realDstExpr, offset, curSize, tmpRegIdx1);
    auto pair2 = GenerateMemoryCopyPair(mirBuilder, realSrcExpr, realDstExpr, offset + curSize, curSize, tmpRegIdx2);
    // insert order: regassign1, regassign2, iassignoff1, iassignoff2
    InsertBeforeAndMayPrintStmtList(block, stmt, debug, {pair1.first, pair2.first, pair1.second, pair2.second});
    offset += (2 * curSize);
    ++i;
  }
  // handle memcpy return val
  auto *retAssign = GenRetAssign(stmt, func, true, memOpKind, errorNumber);
  InsertBeforeAndMayPrintStmt(block, stmt, debug, retAssign);
  block.RemoveStmt(&stmt);
}

bool MemEntry::ExpandMemcpy(const MemEntry &srcMem, uint64 copySize, MIRFunction &func,
                            StmtNode &stmt, BlockNode &block, bool isLowLevel, bool debug,
                            ErrorNumber errorNumber) const {
  OpKind memOpKind = SimplifyOp::ComputeOpKind(stmt);
  MemEntryKind memKind = GetKind();
  MIRType *accessType = nullptr;
  if (!isLowLevel) {  // check type consistency and memKind only for high level expand
    if (memType != srcMem.memType) {
      if (srcMem.memType->GetPrimType() != PTY_void) {
        return false;
      }
      accessType = memType;
    }
    CHECK_FATAL(memKind != kMemEntryUnknown, "invalid memKind");
  }
  MIRBuilder *mirBuilder = func.GetModule()->GetMIRBuilder();
  StmtNode *newAssign = nullptr;
  if (isLowLevel) {  // For cglower, replace memcpy with a series of low-level iassignoff
    ExpandMemcpyLowLevel(srcMem, copySize, func, stmt, block, memOpKind, debug, errorNumber);
    return true;
  }

  // Do low level expand for all struct memcpy for now
  if (memKind == kMemEntryStruct) {
    MayPrintLog(debug, false, memOpKind, "Do low level expand for all struct memcpy for now");
    return false;
  }

  if (memKind == kMemEntryPrimitive) {
    if (addrExpr->GetOpCode() == OP_addrof) {  // We prefer dassign to iassign
      auto *addrof = static_cast<AddrofNode*>(addrExpr);
      auto *symbol = func.GetLocalOrGlobalSymbol(addrof->GetStIdx());
      newAssign = mirBuilder->CreateStmtDassign(*symbol, addrof->GetFieldID(),
                                                srcMem.BuildAsRhsExpr(func, accessType));
    } else {
      MIRType *memPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*memType);
      newAssign = mirBuilder->CreateStmtIassign(*memPtrType, 0, addrExpr,
                                                srcMem.BuildAsRhsExpr(func, accessType));
      static_cast<IassignNode *>(newAssign)->SetExpandFromArrayOfCharFunc(true);
    }
    InsertBeforeAndMayPrintStmt(block, stmt, debug, newAssign);
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
          GlobalTables::GetIntConstTable().GetOrCreateIntConst(static_cast<int64>(i), *u32Type));
      auto *arrayExpr = mirBuilder->CreateExprArray(*arrayType, addrExpr, indexExpr);
      auto *rhsArrayExpr = mirBuilder->CreateExprArray(*arrayType, srcMem.addrExpr, indexExpr);
      auto *rhsIreadExpr = mirBuilder->CreateExprIread(*elemType, *elemPtrType, 0, rhsArrayExpr);
      auto *arrayElemAssign = mirBuilder->CreateStmtIassign(*elemPtrType, 0, arrayExpr, rhsIreadExpr);
      arrayElemAssign->SetExpandFromArrayOfCharFunc(true);
      InsertBeforeAndMayPrintStmt(block, stmt, debug, arrayElemAssign);
    }
  } else {
    CHECK_FATAL(false, "impossible");
  }

  // handle memcpy return val
  auto *retAssign = GenRetAssign(stmt, func, isLowLevel, memOpKind, errorNumber);
  InsertBeforeAndMayPrintStmt(block, stmt, debug, retAssign);
  block.RemoveStmt(&stmt);
  return true;
}

StmtNode *MemEntry::GenEndZeroAssign(StmtNode &stmt, MIRFunction &func, bool isLowLevel, uint64 count) const {
  MIRBuilder *mirBuilder = func.GetModule()->GetMIRBuilder();
  StmtNode *newAssign = nullptr;
  if (isLowLevel) {
    newAssign = mirBuilder->CreateStmtIassignoff(PTY_u8, count, addrExpr,
                                                 ConstructConstvalNode(0, PTY_u8, *mirBuilder));
    return newAssign;
  }
  MemEntryKind memKind = GetKind();
  auto *base = mirBuilder->CreateExprBinary(OP_add, PTY_a64, static_cast<IassignNode&>(stmt).Opnd(0),
                                            mirBuilder->CreateIntConst(count, PTY_a64));
  if (memKind == kMemEntryArray) {
    auto *arrayType = static_cast<MIRArrayType *>(memType);
    MIRType *elemType = arrayType->GetElemType();
    MIRType *elemPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*elemType);
    /* if size check not success, we assign '\0' to dst[count] */
    newAssign = mirBuilder->CreateStmtIassign(*elemPtrType, 0, base,
                                              ConstructConstvalNode(0, elemType->GetSize(), elemType->GetPrimType(),
                                                                    *mirBuilder));
  } else if (memKind == kMemEntryPrimitive) {
    if (base->GetOpCode() == OP_addrof) {
      auto *addrof = static_cast<AddrofNode*>(static_cast<BaseNode *>(base));
      auto *symbol = func.GetLocalOrGlobalSymbol(addrof->GetStIdx());
      newAssign = mirBuilder->CreateStmtDassign(*symbol, addrof->GetFieldID(),
                                                ConstructConstvalNode(0, PTY_u8, *mirBuilder));
    } else {
      MIRType *memPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*memType);
      newAssign = mirBuilder->CreateStmtIassign(*memPtrType, 0, base,
                                                ConstructConstvalNode(0, PTY_u8, *mirBuilder));
    }
  }
  return newAssign;
}

// handle memset, memcpy, sprintf return val
StmtNode *MemEntry::GenRetAssign(StmtNode &stmt, MIRFunction &func, bool isLowLevel, OpKind opKind, int32 returnVal) {
  if (stmt.GetOpCode() != OP_call && stmt.GetOpCode() != OP_callassigned) {
    return nullptr;
  }
  auto &callStmt = static_cast<CallNode&>(stmt);
  const auto &retVec = callStmt.GetReturnVec();
  if (retVec.empty()) {
    return nullptr;
  }
  MIRBuilder *mirBuilder = func.GetModule()->GetMIRBuilder();
  BaseNode *rhs = callStmt.Opnd(0);  // for memset, memcpy
  switch (opKind) {
    case MEM_OP_memset_s:
    case MEM_OP_memcpy_s:
    case SPRINTF_OP_sprintf:
    case SPRINTF_OP_sprintf_s:{
      // memset_s memcpy_s sprintf_s must return an returnVal
      MIRType *constType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_i32));
      MIRConst *mirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
          static_cast<uint64>(static_cast<int64>(returnVal)),
          *constType);
      rhs = mirBuilder->CreateConstval(mirConst);
      break;
    }
    default: {
      break;
    }
  }

  if (!retVec[0].second.IsReg()) {
    auto *retAssign = mirBuilder->CreateStmtDassign(retVec[0].first, 0, rhs);
    return retAssign;
  }
  PregIdx pregIdx = retVec[0].second.GetPregIdx();
  auto pregType = func.GetPregTab()->GetPregTableItem(static_cast<uint32>(pregIdx))->GetPrimType();
  auto *retAssign = mirBuilder->CreateStmtRegassign(pregType, pregIdx, rhs);
  if (isLowLevel) {
    retAssign->GetRHS()->SetPrimType(pregType);
  }
  return retAssign;
}

OpKind SimplifyOp::ComputeOpKind(StmtNode &stmt) {
  if (stmt.GetOpCode() == OP_intrinsiccall) {
    auto intrinsicID = static_cast<IntrinsiccallNode&>(stmt).GetIntrinsic();
    if (intrinsicID == INTRN_C_memset) {
      return MEM_OP_memset;
    } else if (intrinsicID == INTRN_C_memcpy) {
      return MEM_OP_memcpy;
    }
  }
  // lowered memop function (such as memset) may be a call, not callassigned
  if (stmt.GetOpCode() != OP_callassigned && stmt.GetOpCode() != OP_call) {
    return MEM_OP_unknown;
  }
  auto &callStmt = static_cast<CallNode&>(stmt);
  MIRFunction *mirFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callStmt.GetPUIdx());
  const char *funcName = mirFunc->GetName().c_str();
  static const std::unordered_map<std::string, OpKind> hashFuncName = {
      {kFuncNameOfMemset, MEM_OP_memset},
      {kFuncNameOfMemcpy, MEM_OP_memcpy},
      {kFuncNameOfMemsetS, MEM_OP_memset_s},
      {kFuncNameOfMemcpyS, MEM_OP_memcpy_s},
      {kFuncNameOfSprintf, SPRINTF_OP_sprintf},
      {kFuncNameOfSprintfS, SPRINTF_OP_sprintf_s},
      {kFuncNameOfSnprintfS, SPRINTF_OP_snprintf_s},
      {kFuncNameOfVsnprintfS, SPRINTF_OP_vsnprintf_s}
  };
  auto iter = hashFuncName.find(funcName);
  if (iter != hashFuncName.end()) {
    return iter->second;
  }
  return MEM_OP_unknown;
}

bool SimplifyOp::AutoSimplify(StmtNode &stmt, BlockNode &block, bool isLowLevel) {
  OpKind opKind = ComputeOpKind(stmt);
  switch (opKind) {
    case MEM_OP_memset:
    case MEM_OP_memset_s: {
      return SimplifyMemset(stmt, block, isLowLevel);
    }
    case MEM_OP_memcpy:
    case MEM_OP_memcpy_s: {
      return SimplifyMemcpy(stmt, block, isLowLevel);
    }
    case SPRINTF_OP_sprintf:
    case SPRINTF_OP_sprintf_s:
    case SPRINTF_OP_snprintf_s:
    case SPRINTF_OP_vsnprintf_s:{
      auto simplifySprintf = sprintfMap.find(opKind)->second;
      return simplifySprintf->ReplaceSprintfIfNeeded(stmt, block, isLowLevel, opKind);
    }
    default:
      break;
  }
  return false;
}

StmtNode *SprintfBaseOper::InsertMemcpyCallStmt(const MapleVector<BaseNode *> &args, StmtNode &stmt,
                                                BlockNode &block, int32 retVal, bool isLowLevel) {
  MIRBuilder *mirBuilder = op.GetFunction()->GetModule()->GetMIRBuilder();
  auto memcpyFunc = mirBuilder->GetOrCreateFunction(kFuncNameOfMemcpy, TyIdx(PTY_void));
  auto memcpyCallStmt = mirBuilder->CreateStmtCallAssigned(memcpyFunc->GetPuidx(), args, nullptr, OP_callassigned);
  memcpyCallStmt->SetSrcPos(stmt.GetSrcPos());
  ASSERT_NOT_NULL(memcpyFunc->GetFuncSymbol());
  memcpyFunc->GetFuncSymbol()->SetAppearsInCode(true);
  memcpyFunc->AllocSymTab();
  // handle memcpy return val
  auto *retAssign = MemEntry::GenRetAssign(stmt, *op.GetFunction(), isLowLevel, MEM_OP_memcpy, retVal);
  InsertBeforeAndMayPrintStmtList(block, stmt, op.IsDebug(), {memcpyCallStmt, retAssign});
  return memcpyCallStmt;
}

static bool GetFmtSrcSize(const StmtNode &stmt, const ConststrNode &fmt, uint32 &fmtOpndIdx, const OpKind &opKind,
                          uint64 &srcSize) {
  uint32 origOpndIdx = (opKind == SPRINTF_OP_snprintf_s || opKind == SPRINTF_OP_vsnprintf_s) ?
      kSnprintfSOrigOpndIdx : kSprintfSOrigOpndIdx;
  const std::string fmtStr = GlobalTables::GetUStrTable().GetStringFromStrIdx(fmt.GetStrIdx());
  if (strchr(fmtStr.c_str(), '%') == nullptr) { /* eg: snprintf_s(dst, dstMax, count, constStr) */
    srcSize = strlen(fmtStr.c_str());
    return true;
  } else if (strcmp (fmtStr.c_str(), "%s") == 0) { /* eg: snprintf_s(dst, dstMax, count, "%s", constStr) */
    BaseNode *orig = stmt.Opnd(origOpndIdx);
    if (orig->GetOpCode() == OP_conststr) {
      auto cs = static_cast<ConststrNode *>(orig);
      const std::string origStr = GlobalTables::GetUStrTable().GetStringFromStrIdx(cs->GetStrIdx());
      srcSize = strlen(origStr.c_str());
      fmtOpndIdx = origOpndIdx;
      return true;
    }
  }
  return false;
}

void SprintfBaseOper::ProcessRetValue(StmtNode &stmt, BlockNode &block, OpKind opKind, int32 retVal, bool isLowLevel) {
  auto retAssign = MemEntry::GenRetAssign(stmt, *op.GetFunction(), isLowLevel, opKind, retVal);
  InsertBeforeAndMayPrintStmt(block, stmt, op.IsDebug(), retAssign);
}

bool SprintfBaseOper::CheckInvalidPara(uint64 count, uint64 dstMax, uint64 srcSize) {
  if (count == 0 || srcSize == 0 || dstMax== 0 || dstMax > kSecurecMemMaxLen|| count > kSecurecMemMaxLen) {
    return false;
  }
  return true;
}

bool SprintfBaseOper::DealWithDstOrEndZero(StmtNode &stmt, BlockNode &block, bool isLowLevel, uint64 count) {
  MemEntry dstMemEntry;
  bool valid = MemEntry::ComputeMemEntry(*(stmt.Opnd(0)), *op.GetFunction(), dstMemEntry, isLowLevel);
  if (!valid) {
    return false;
  }
  auto newAssign = dstMemEntry.GenEndZeroAssign(stmt, *op.GetFunction(), isLowLevel, count);
  if (newAssign == nullptr) {
    return false;
  }
  InsertBeforeAndMayPrintStmt(block, stmt, op.IsDebug(), newAssign);
  return true;
}

bool SprintfBaseOper::CompareDstMaxSrcSize(StmtNode &stmt, BlockNode &block, uint64 dstMax, uint64 srcSize,
                                           bool isLowLevel) {
  uint64 count = srcSize >= dstMax ? 0 : srcSize;
  if (!DealWithDstOrEndZero(stmt, block, isLowLevel, count)) {
    return false;
  }
  int32 retVal = srcSize >= dstMax ? kTruncate : static_cast<int32>(srcSize);
  ProcessRetValue(stmt, block, SPRINTF_OP_sprintf_s, retVal, isLowLevel);
  return true;
}

bool SprintfBaseOper::CompareCountSrcSize(StmtNode &stmt, BlockNode &block, uint64 count, uint64 srcSize,
                                          bool isLowLevel) {
  if (!DealWithDstOrEndZero(stmt, block, isLowLevel, std::min(count, srcSize))) {
    return false;
  }
  int32 retVal = srcSize > count ? kTruncate : static_cast<int32>(std::min(srcSize, count));
  ProcessRetValue(stmt, block, SPRINTF_OP_sprintf_s, retVal, isLowLevel);
  return true;
}

bool SprintfBaseOper::DealWithFmtConstStr(StmtNode &stmt, BaseNode *fmt, BlockNode &block, bool isLowLevel) {
  StmtNode *sprintfCallStmt = &stmt;
  const std::string fmtStr =
      GlobalTables::GetUStrTable().GetStringFromStrIdx(static_cast<ConststrNode*>(fmt)->GetStrIdx());
  uint64 srcSize = strlen(fmtStr.c_str());
  /* If the format doesn't contain % args or %%, the format is a literal string constant. use memcpy */
  if (strchr(fmtStr.c_str(), '%') == nullptr) {
    if (!ReplaceSprintfWithMemcpy(stmt, block, kSprintfFmtOpndIdx, srcSize, isLowLevel)) {
      return false;
    }
  } else if (strcmp (fmtStr.c_str(), "%s") == 0) {
    BaseNode *orig = sprintfCallStmt->Opnd(kSprintfOrigOpndIdx);
    if (orig == nullptr || orig->GetOpCode() != OP_conststr) {
      return false;
    } else {
      const std::string origStr =
          GlobalTables::GetUStrTable().GetStringFromStrIdx(static_cast<ConststrNode*>(orig)->GetStrIdx());
      srcSize = strlen(origStr.c_str());
      if (!ReplaceSprintfWithMemcpy(stmt, block, kSprintfOrigOpndIdx, srcSize, isLowLevel)) {
        return false;
      }
    }
  } else {
    return false;
  }
  if (!DealWithDstOrEndZero(stmt, block, isLowLevel, srcSize)) {
    return false;
  }
  block.RemoveStmt(&stmt);
  return true;
}

bool SprintfBaseOper::CheckCondIfNeedReplace(const StmtNode &stmt, uint32_t opIdx) {
  return stmt.NumOpnds() <= (opIdx + 1);
}

bool SprintfBaseOper::IsCountConst(StmtNode &stmt, uint64 &count, uint32_t opndIdx) {
  bool isConst = false;
  BaseNode *foldCountExpr = FoldIntConst(stmt.Opnd(opndIdx), count, isConst);
  if (foldCountExpr != nullptr) {
    stmt.SetOpnd(foldCountExpr, opndIdx);
  }
  return isConst;
}

bool SimplifySnprintfS::GetDstMaxOrCountSize(StmtNode &stmt, uint64 &dstMax, uint64 &count) {
  return IsCountConst(stmt, count, kFmtSizeOpndIdx) && IsCountConst(stmt, dstMax, kDstSizeOpndIdx);
}

bool SimplifySprintfS::GetDstMaxOrCountSize(StmtNode &stmt, uint64 &dstMax, uint64 &count __attribute__((unused))) {
  return IsCountConst(stmt, dstMax, kDstSizeOpndIdx);
}

bool SprintfBaseOper::ReplaceSprintfWithMemcpy(StmtNode &stmt, BlockNode &block, uint32 opndIdx, uint64 copySize,
                                               bool isLowLevel) {
  MapleVector<BaseNode *> args(op.GetFunction()->GetCodeMempoolAllocator().Adapter());
  MIRBuilder *mirBuilder = op.GetFunction()->GetModule()->GetMIRBuilder();
  /* size check success, Convert sprintf into memcpy */
  args.push_back(stmt.Opnd(kSprintfDstOpndIdx));
  args.push_back(stmt.Opnd(opndIdx));
  args.push_back(ConstructConstvalNode(copySize, PTY_u64, *mirBuilder));
  auto memcpyCallStmt = InsertMemcpyCallStmt(args, stmt, block, static_cast<int32>(copySize), isLowLevel);
  if (memcpyCallStmt == nullptr) {
    return false;
  }
  ProcessRetValue(stmt, block, SPRINTF_OP_sprintf, static_cast<int32>(copySize), isLowLevel);
  (void)op.SimplifyMemcpy(*memcpyCallStmt, block, isLowLevel);

  return true;
}

/* Simplify a call to the sprintf  with arguments DEST, FMT, and ORIG.
   ORIG may be null if this is a 2-argument call.  We don't attempt to
   simplify calls with more than 3 arguments.
   Return true if simplification was possible, otherwise false. */
bool SimplifySprintf::ReplaceSprintfIfNeeded(StmtNode &stmt, BlockNode &block, bool isLowLevel, const OpKind &opKind) {
  if (!CheckCondIfNeedReplace(stmt, kSprintfOrigOpndIdx)) {
    return false;
  }
  StmtNode *sprintfCallStmt = &stmt;
  if (op.IsDebug()) {
    LogInfo::MapleLogger() << "[funcName] " << op.GetFunction()->GetName() << std::endl;
    sprintfCallStmt->Dump(0);
  }
  BaseNode *fmt = sprintfCallStmt->Opnd(kSprintfFmtOpndIdx);
  // sprintf's fmt must be a const string, otherwise we can not optimize it
  if (fmt->GetOpCode() == OP_conststr) {
    return DealWithFmtConstStr(stmt, fmt, block, isLowLevel);
  }
  return false;
}

// expand snprintf_s and vsnprintf_s call statement
bool SimplifySnprintfS::ReplaceSprintfIfNeeded(StmtNode &stmt, BlockNode &block, bool isLowLevel,
                                               const OpKind &opKind) {
  if (!CheckCondIfNeedReplace(stmt, kSnprintfSOrigOpndIdx)) {
    return false;
  }
  uint32 fmtOpndIdx = 3;
  uint64 srcSize = 0;
  uint64 dstMax = 0;
  uint64 count = 0;
  if (!GetDstMaxOrCountSize(stmt, dstMax, count)) {
    return false;
  }
  BaseNode *fmt = stmt.Opnd(kSnprintfSFmtOpndIdx);
  if (fmt->GetOpCode() == OP_conststr) {
    if (!GetFmtSrcSize(stmt, static_cast<const ConststrNode &>(*fmt), fmtOpndIdx, opKind, srcSize)) {
      return false;
    }
    if (!CheckInvalidPara(count, dstMax, srcSize)) {
      if (!DealWithDstOrEndZero(stmt, block, isLowLevel, 0)) {
        return false;
      }
      if (!isLowLevel) {
        ProcessRetValue(stmt, block, SPRINTF_OP_sprintf_s, kSprintfErrNum, isLowLevel);
        return false;
      }
      ProcessRetValue(stmt, block, SPRINTF_OP_sprintf_s, kSprintfErrNum, isLowLevel);
    } else if (dstMax <= count) {
      /* if dstMax <= count, we need compare dstMax and srcSize */
      if (!ReplaceSprintfWithMemcpy(stmt, block, fmtOpndIdx, dstMax, isLowLevel)) {
        return false;
      }
      bool isSuccess = CompareDstMaxSrcSize(stmt, block, dstMax, srcSize, isLowLevel);
      if (!isSuccess) {
        block.RemoveStmt(&stmt);
        return false;
      }
    } else {
      /* if dstMax > count, we need compare count and srcSize */
      if (!ReplaceSprintfWithMemcpy(stmt, block, fmtOpndIdx, std::min(count, srcSize), isLowLevel)) {
        return false;
      }
      bool isSuccess = CompareCountSrcSize(stmt, block, count, srcSize, isLowLevel);
      if (!isSuccess) {
        block.RemoveStmt(&stmt);
        return false;
      }
    }
  } else {
    // We do not support non-constant str.
    return false;
  }
  block.RemoveStmt(&stmt);
  return true;
}

// expand sprintf_s call statement
bool SimplifySprintfS::ReplaceSprintfIfNeeded(StmtNode &stmt, BlockNode &block, bool isLowLevel,
                                              const OpKind &opKind) {
  if (!CheckCondIfNeedReplace(stmt, kSprintfSOrigOpndIdx)) {
    return false;
  }
  uint32 fmtOpndIdx = 2;
  uint64 srcSize = 0;
  uint64 dstMax = 0;
  uint64 count = 0;
  if (!GetDstMaxOrCountSize(stmt, dstMax, count)) {
    return false;
  }
  /* count is unused, give 1 to pass CheckInvalidPara */
  count = 1;
  BaseNode *fmt = stmt.Opnd(fmtOpndIdx);
  if (fmt->GetOpCode() == OP_conststr) {
    if (!GetFmtSrcSize(stmt, static_cast<const ConststrNode &>(*fmt), fmtOpndIdx, opKind, srcSize)) {
      return false;
    }
    if (!CheckInvalidPara(count, dstMax, srcSize)) {
      if (!DealWithDstOrEndZero(stmt, block, isLowLevel, 0)) {
        return false;
      }
      if (!isLowLevel) {
        ProcessRetValue(stmt, block, SPRINTF_OP_sprintf_s, kSprintfErrNum, isLowLevel);
        return false;
      }
      ProcessRetValue(stmt, block, SPRINTF_OP_sprintf_s, kSprintfErrNum, isLowLevel);
    } else {
      uint64 copySize = dstMax <= srcSize ? dstMax : srcSize;
      if (!ReplaceSprintfWithMemcpy(stmt, block, fmtOpndIdx, copySize, isLowLevel)) {
        return false;
      }
      bool isSuccess = CompareDstMaxSrcSize(stmt, block, dstMax, srcSize, isLowLevel);
      if (!isSuccess) {
        block.RemoveStmt(&stmt);
        return false;
      }
      int32 retVal = dstMax <= srcSize ? kTruncate : static_cast<int32>(srcSize);
      ProcessRetValue(stmt, block, SPRINTF_OP_sprintf_s, retVal, isLowLevel);
    }
  } else {
    // We do not support non-constant str.
    return false;
  }
  block.RemoveStmt(&stmt);
  return true;
}

// expand memset_s call statement, return pointer of memset call statement node to be expanded in the next step, return
// nullptr if memset_s is expanded completely.
StmtNode *SimplifyOp::PartiallyExpandMemsetS(StmtNode &stmt, BlockNode &block) const{
  ErrorNumber errNum = ERRNO_OK;

  uint64 srcSize = 0;
  bool isSrcSizeConst = false;
  BaseNode *foldSrcSizeExpr = FoldIntConst(stmt.Opnd(kMemOpSSrcSizeOpndIdx), srcSize, isSrcSizeConst);
  if (foldSrcSizeExpr != nullptr) {
    stmt.SetOpnd(foldSrcSizeExpr, kMemOpSDstSizeOpndIdx);
  }

  uint64 dstSize = 0;
  bool isDstSizeConst = false;
  BaseNode *foldDstSizeExpr = FoldIntConst(stmt.Opnd(kMemOpSDstSizeOpndIdx), dstSize, isDstSizeConst);
  if (foldDstSizeExpr != nullptr) {
    stmt.SetOpnd(foldDstSizeExpr, kMemOpSDstSizeOpndIdx);
  }
  if (isDstSizeConst) {
    if ((srcSize > dstSize && dstSize == 0) || dstSize > kSecurecMemMaxLen) {
      errNum = ERRNO_RANGE;
    }
  }

  MIRBuilder *mirBuilder = func->GetModule()->GetMIRBuilder();
  auto dstOpnd = stmt.Opnd(kMemOpDstOpndIdx);
  bool isDstAddrSafe = IsAddrSafe(*dstOpnd);
  auto newExpr = TryExtractExpr(*dstOpnd, *func, block, stmt, false, debug);
  if (newExpr) {
    stmt.SetOpnd(newExpr, kMemOpDstOpndIdx);
  }

  BlockOperationHelper helper(stmt, *func, block, false, debug);
  LabelIdx finalLabIdx = func->GetLabelTab()->CreateLabelWithPrefix('f');
  if (errNum != ERRNO_OK || (isSrcSizeConst && isDstSizeConst && dstSize == 0 && srcSize == 0)) {
    auto errnoAssign = MemEntry::GenRetAssign(stmt, *func, true, MEM_OP_memset_s, errNum);
    InsertBeforeAndMayPrintStmt(block, stmt, debug, errnoAssign);
    block.RemoveStmt(&stmt);
    return nullptr;
  } else {
    LabelIdx dstSizeCheckLabIdx, srcSizeCheckLabIdx, nullPtrLabIdx;
    if (!isDstSizeConst) {
      // check if dst size is greater than maxlen
      dstSizeCheckLabIdx = func->GetLabelTab()->CreateLabelWithPrefix('n');  // 'n' means nullptr
      helper.CreateAndInsertCheckStmt(OP_gt, stmt.Opnd(kMemOpSDstSizeOpndIdx),
                                      ConstructConstvalNode(kSecurecMemMaxLen, PTY_u64, *mirBuilder),
                                      dstSizeCheckLabIdx);
    }

    // check if dst is nullptr
    if (!isDstAddrSafe) {
      nullPtrLabIdx = func->GetLabelTab()->CreateLabelWithPrefix('n');  // 'n' means nullptr
      helper.CreateAndInsertCheckStmt(OP_eq, stmt.Opnd(kMemOpDstOpndIdx),
                                      ConstructConstvalNode(0, PTY_u64, *mirBuilder), nullPtrLabIdx);
    }

    if (isDstSizeConst && isSrcSizeConst) {
      if (srcSize > dstSize) {
        auto *callStmt = helper.InsertMemsetCallStmt(stmt.Opnd(kMemOpDstOpndIdx), stmt.Opnd(kMemOpSSrcOpndIdx));
        helper.InsertRetAssignAndGoto(finalLabIdx, MEM_OP_memset_s, ERRNO_RANGE_AND_RESET);
        if (!isDstAddrSafe) {
          helper.HandleError(nullPtrLabIdx, finalLabIdx, ERRNO_INVAL, MEM_OP_memset_s);
        }
        helper.InsertLableNode(finalLabIdx);
        block.RemoveStmt(&stmt);
        return callStmt;
      }
    } else {
      // check if src size is greater than dst size
      srcSizeCheckLabIdx = func->GetLabelTab()->CreateLabelWithPrefix('n');  // 'n' means nullptr
      helper.CreateAndInsertCheckStmt(OP_gt, stmt.Opnd(kMemOpSSrcSizeOpndIdx), stmt.Opnd(kMemOpSDstSizeOpndIdx),
                                      srcSizeCheckLabIdx);
    }

    MapleVector<BaseNode *> args(func->GetCodeMempoolAllocator().Adapter());
    args.push_back(stmt.Opnd(kMemOpDstOpndIdx));
    args.push_back(stmt.Opnd(kMemOpSSrcOpndIdx));
    args.push_back(stmt.Opnd(kMemOpSSrcSizeOpndIdx));
    auto memFunc = mirBuilder->GetOrCreateFunction(kFuncNameOfMemset, TyIdx(PTY_void));
    auto memCallStmt = mirBuilder->CreateStmtCallAssigned(memFunc->GetPuidx(), args, nullptr, OP_callassigned);
    memCallStmt->SetSrcPos(stmt.GetSrcPos());
    InsertBeforeAndMayPrintStmt(block, stmt, debug, memCallStmt);
    helper.InsertRetAssignAndGoto(finalLabIdx, MEM_OP_memset_s, errNum);

    if (!isSrcSizeConst || !isDstSizeConst) {
      // handle src size error
      // in memset_s, we need check if dstSize is 0 before handle srcSize > dstSize, and memset dst to zero
      helper.HandleErrorWithDstSizeCheckAndReset(srcSizeCheckLabIdx, dstSizeCheckLabIdx, finalLabIdx, MEM_OP_memset_s,
                                                 ERRNO_RANGE_AND_RESET, isDstSizeConst, dstSize);
    }

    // handle dst nullptr error
    // in memset_s, we need check if dstSize is 0 before handle dst == nullptr
    if (!isDstAddrSafe) {
      helper.HandleErrorWithDstSizeCheck(nullPtrLabIdx, dstSizeCheckLabIdx, finalLabIdx, MEM_OP_memset_s, ERRNO_INVAL,
                                         isDstSizeConst, dstSize);
    }

    if (!isDstSizeConst) {
      // handle dst size error
      helper.HandleError(dstSizeCheckLabIdx, finalLabIdx, ERRNO_RANGE, MEM_OP_memset_s);
    }
    auto *finalLabelNode = mirBuilder->CreateStmtLabel(finalLabIdx);
    InsertBeforeAndMayPrintStmt(block, stmt, debug, finalLabelNode);
    block.RemoveStmt(&stmt);
    return memCallStmt;
  }
}

// Try to replace the call to memset with a series of assign operations (including dassign, iassign, iassignoff), which
// is usually profitable for small memory size.
// This function is called in two places, one in mpl2mpl simplify, another in cglower:
// (1) mpl2mpl memset expand (isLowLevel == false)
//   for primitive type, array type with element size < 4 bytes and struct type without padding
// (2) cglower memset expand
//   for array type with element size >= 4 bytes and struct type with paddings
bool SimplifyOp::SimplifyMemset(StmtNode &stmt, BlockNode &block, bool isLowLevel) {
  OpKind memOpKind = ComputeOpKind(stmt);
  if (memOpKind != MEM_OP_memset && memOpKind != MEM_OP_memset_s) {
    return false;
  }
  uint32 dstOpndIdx = 0;
  uint32 srcOpndIdx = 1;
  uint32 srcSizeOpndIdx = 2;
  bool isSafeVersion = memOpKind == MEM_OP_memset_s;
  if (debug) {
    LogInfo::MapleLogger() << "[funcName] " << func->GetName() << std::endl;
    stmt.Dump(0);
  }

  StmtNode *memsetCallStmt = &stmt;
  if (memOpKind == MEM_OP_memset_s && !isLowLevel) {
    memsetCallStmt = PartiallyExpandMemsetS(stmt, block);
    if (!memsetCallStmt) {
      return true;  // Expand memset_s completely, no extra memset is generated, so just return true
    }
  } else if (!isLowLevel) {
    auto newExpr = TryExtractExpr(*stmt.Opnd(kMemOpDstOpndIdx), *func, block, stmt, false, debug);
    if (newExpr) {
      stmt.SetOpnd(newExpr, kMemOpDstOpndIdx);
    }
  }

  uint64 srcSize = 0;
  bool isSrcSizeConst = false;
  BaseNode *foldSrcSizeExpr = FoldIntConst(memsetCallStmt->Opnd(srcSizeOpndIdx), srcSize, isSrcSizeConst);
  if (foldSrcSizeExpr != nullptr) {
    memsetCallStmt->SetOpnd(foldSrcSizeExpr, srcSizeOpndIdx);
  }

  if (isSrcSizeConst) {
    // If the size is too big, we won't expand it
    uint32 thresholdExpand = (isSafeVersion ? thresholdMemsetSExpand : thresholdMemsetExpand);
    if (srcSize > thresholdExpand) {
      MayPrintLog(debug, false, memOpKind, "size is too big");
      return false;
    }
    if (srcSize == 0) {
      if (memOpKind == MEM_OP_memset) {
        auto *retAssign = MemEntry::GenRetAssign(stmt, *func, isLowLevel, memOpKind);
        InsertBeforeAndMayPrintStmt(block, *memsetCallStmt, debug, retAssign);
      }
      block.RemoveStmt(memsetCallStmt);
      return true;
    }
  }

  // memset's 'src size' must be a const value, otherwise we can not expand it
  if (!isSrcSizeConst) {
    MayPrintLog(debug, false, memOpKind, "size is not int const");
    return false;
  }

  ErrorNumber errNum = ERRNO_OK;

  uint64 val = 0;
  bool isIntConst = false;
  BaseNode *foldValExpr = FoldIntConst(memsetCallStmt->Opnd(srcOpndIdx), val, isIntConst);
  if (foldValExpr != nullptr) {
    memsetCallStmt->SetOpnd(foldValExpr, srcOpndIdx);
  }
  // memset's second argument 'val' should also be a const value
  if (!isIntConst) {
    MayPrintLog(debug, false, memOpKind, "val is not int const");
    return false;
  }

  MemEntry dstMemEntry;
  bool valid = MemEntry::ComputeMemEntry(*(memsetCallStmt->Opnd(dstOpndIdx)), *func, dstMemEntry, isLowLevel);
  if (!valid) {
    MayPrintLog(debug, false, memOpKind, "dstMemEntry is invalid");
    return false;
  }
  bool ret = false;
  if (srcSize != 0) {
    ret = dstMemEntry.ExpandMemset(val, static_cast<uint64>(srcSize), *func, *memsetCallStmt, block, isLowLevel,
                                   debug, errNum);
  } else {
    // if size == 0, no need to set memory, just return error nummber
    auto *retAssign = MemEntry::GenRetAssign(*memsetCallStmt, *func, isLowLevel, memOpKind, errNum);
    InsertBeforeAndMayPrintStmt(block, *memsetCallStmt, debug, retAssign);
    block.RemoveStmt(memsetCallStmt);
    ret = true;
  }
  if (ret) {
    MayPrintLog(debug, true, memOpKind, "well done");
  }
  return ret;
}

StmtNode *SimplifyOp::PartiallyExpandMemcpyS(StmtNode &stmt, BlockNode &block) {
  ErrorNumber errNum = ERRNO_OK;

  uint64 srcSize = 0;
  bool isSrcSizeConst = false;
  BaseNode *foldSrcSizeExpr = FoldIntConst(stmt.Opnd(kMemOpSSrcSizeOpndIdx), srcSize, isSrcSizeConst);
  if (foldSrcSizeExpr != nullptr) {
    stmt.SetOpnd(foldSrcSizeExpr, kMemOpSDstSizeOpndIdx);
  }

  uint64 dstSize = 0;
  bool isDstSizeConst = false;
  BaseNode *foldDstSizeExpr = FoldIntConst(stmt.Opnd(kMemOpSDstSizeOpndIdx), dstSize, isDstSizeConst);
  if (foldDstSizeExpr != nullptr) {
    stmt.SetOpnd(foldDstSizeExpr, kMemOpSDstSizeOpndIdx);
  }
  if (isDstSizeConst) {
    if ((dstSize == 0) || static_cast<uint64>(dstSize) > kSecurecMemMaxLen) {
      errNum = ERRNO_RANGE;
    }
  }

  MIRBuilder *mirBuilder = func->GetModule()->GetMIRBuilder();
  auto dstOpnd = stmt.Opnd(kMemOpDstOpndIdx);
  auto srcOpnd = stmt.Opnd(kMemOpSSrcOpndIdx);
  bool isDstAddrSafe = IsAddrSafe(*dstOpnd);
  bool isSrcAddrSafe = IsAddrSafe(*srcOpnd);
  auto newDst = TryExtractExpr(*dstOpnd, *func, block, stmt, false, debug);
  if (newDst) {
    stmt.SetOpnd(newDst, kMemOpDstOpndIdx);
  }
  auto newSrc = TryExtractExpr(*srcOpnd, *func, block, stmt, true, debug);
  if (newSrc) {
    stmt.SetOpnd(newSrc, kMemOpSSrcOpndIdx);
  }

  BlockOperationHelper helper(stmt, *func, block, false, debug);
  LabelIdx finalLabIdx = func->GetLabelTab()->CreateLabelWithPrefix('f');
  if (errNum != ERRNO_OK || (isSrcSizeConst && isDstSizeConst && dstSize == 0 && srcSize == 0)) {
    auto errnoAssign = MemEntry::GenRetAssign(stmt, *func, true, MEM_OP_memcpy_s, errNum);
    InsertBeforeAndMayPrintStmt(block, stmt, debug, errnoAssign);
    block.RemoveStmt(&stmt);
    return nullptr;
  } else {
    LabelIdx dstSizeCheckLabIdx, srcSizeCheckLabIdx, dstNullPtrLabIdx, srcNullPtrLabIdx, addrEqLabIdx, overlapLabIdx;
    if (!isDstSizeConst) {
      // check if dst size is greater than maxlen or equal to 0
      dstSizeCheckLabIdx = func->GetLabelTab()->CreateLabelWithPrefix('n');  // 'n' means nullptr

      helper.CreateAndInsertCheckStmt(OP_eq, stmt.Opnd(kMemOpSDstSizeOpndIdx),
                                      ConstructConstvalNode(0, PTY_u64, *mirBuilder),
                                      dstSizeCheckLabIdx);
      helper.CreateAndInsertCheckStmt(OP_gt, stmt.Opnd(kMemOpSDstSizeOpndIdx),
                                      ConstructConstvalNode(kSecurecMemMaxLen, PTY_u64, *mirBuilder),
                                      dstSizeCheckLabIdx);
    }

    // check if dst is nullptr
    if (!isDstAddrSafe) {
      dstNullPtrLabIdx = func->GetLabelTab()->CreateLabelWithPrefix('n');  // 'n' means nullptr
      helper.CreateAndInsertCheckStmt(OP_eq, stmt.Opnd(kMemOpDstOpndIdx),
                                      ConstructConstvalNode(0, PTY_u64, *mirBuilder), dstNullPtrLabIdx);
    }
    // check if src is nullptr
    if (!isSrcAddrSafe) {
      srcNullPtrLabIdx = func->GetLabelTab()->CreateLabelWithPrefix('n');  // 'n' means nullptr
      helper.CreateAndInsertCheckStmt(OP_eq, stmt.Opnd(kMemOpSSrcOpndIdx),
                                      ConstructConstvalNode(0, PTY_u64, *mirBuilder), srcNullPtrLabIdx);
    }

    if (isDstSizeConst && isSrcSizeConst) {
      if (srcSize > dstSize) {
        // handle srcsize > dstsize
        auto *callStmt =
            helper.InsertMemsetCallStmt(stmt.Opnd(kMemOpDstOpndIdx), ConstructConstvalNode(0, PTY_i32, *mirBuilder));
        helper.InsertRetAssignAndGoto(finalLabIdx, MEM_OP_memcpy_s, ERRNO_RANGE_AND_RESET);
        // handle src = nullptr
        if (!isSrcAddrSafe) {
          helper.HandleErrorAndReset(srcNullPtrLabIdx, finalLabIdx, MEM_OP_memcpy_s, ERRNO_INVAL_AND_RESET);
        }
        // handle dst = nullptr
        if (!isDstAddrSafe) {
          helper.HandleError(dstNullPtrLabIdx, finalLabIdx, ERRNO_INVAL, MEM_OP_memcpy_s);
        }
        helper.InsertLableNode(finalLabIdx);
        block.RemoveStmt(&stmt);
        return callStmt;
      }
    } else {
      // check if src size is greater than dst size
      srcSizeCheckLabIdx = func->GetLabelTab()->CreateLabelWithPrefix('n');  // 'n' means nullptr
      helper.CreateAndInsertCheckStmt(OP_gt, stmt.Opnd(kMemOpSSrcSizeOpndIdx), stmt.Opnd(kMemOpSDstSizeOpndIdx),
                                      srcSizeCheckLabIdx);
    }

    // check addr equal
    addrEqLabIdx = func->GetLabelTab()->CreateLabelWithPrefix('e');
    helper.CreateAndInsertCheckStmt(OP_eq, stmt.Opnd(kMemOpDstOpndIdx), stmt.Opnd(kMemOpSSrcOpndIdx), addrEqLabIdx);

    // check overlap
    overlapLabIdx = func->GetLabelTab()->CreateLabelWithPrefix('o');  // 'o' means overlap
    auto *checkOverlapStmt = CreateOverlapCheckStmt(*stmt.Opnd(kMemOpDstOpndIdx), *stmt.Opnd(kMemOpSSrcOpndIdx),
                                                    *stmt.Opnd(kMemOpSSrcSizeOpndIdx), overlapLabIdx, mirBuilder);
    InsertBeforeAndMayPrintStmt(block, stmt, debug, checkOverlapStmt);

    // insert memcpy call
    MapleVector<BaseNode *> args(func->GetCodeMempoolAllocator().Adapter());
    args.push_back(stmt.Opnd(kMemOpDstOpndIdx));
    args.push_back(stmt.Opnd(kMemOpSSrcOpndIdx));
    args.push_back(stmt.Opnd(kMemOpSSrcSizeOpndIdx));
    auto memFunc = mirBuilder->GetOrCreateFunction(kFuncNameOfMemcpy, TyIdx(PTY_void));
    auto memCallStmt = mirBuilder->CreateStmtCallAssigned(memFunc->GetPuidx(), args, nullptr, OP_callassigned);
    memCallStmt->SetSrcPos(stmt.GetSrcPos());
    InsertBeforeAndMayPrintStmt(block, stmt, debug, memCallStmt);
    helper.InsertRetAssignAndGoto(finalLabIdx, MEM_OP_memcpy_s, errNum);

    // Add handler IR if dst and src are overlapped
    helper.HandleErrorAndReset(overlapLabIdx, finalLabIdx, MEM_OP_memcpy_s, ERRNO_OVERLAP_AND_RESET);

    // handle addr equal
    helper.HandleError(addrEqLabIdx, finalLabIdx, ERRNO_OK, MEM_OP_memcpy_s);

    if (!isSrcSizeConst || !isDstSizeConst) {
      // handle src size error
      helper.HandleErrorAndReset(srcSizeCheckLabIdx, finalLabIdx, MEM_OP_memcpy_s, ERRNO_RANGE_AND_RESET);
    }

    // handle src nullptr error
    if (!isSrcAddrSafe) {
      helper.HandleErrorAndReset(srcNullPtrLabIdx, finalLabIdx, MEM_OP_memcpy_s, ERRNO_INVAL_AND_RESET);
    }

    // handle dst nullptr error
    if (!isDstAddrSafe) {
      helper.HandleError(dstNullPtrLabIdx, finalLabIdx, ERRNO_INVAL, MEM_OP_memcpy_s);
    }

    if (!isDstSizeConst) {
      // handle dst size error
      helper.HandleError(dstSizeCheckLabIdx, finalLabIdx, ERRNO_RANGE, MEM_OP_memcpy_s);
    }
    helper.InsertLableNode(finalLabIdx);
    block.RemoveStmt(&stmt);
    return memCallStmt;
  }
}

bool SimplifyOp::SimplifyMemcpy(StmtNode &stmt, BlockNode &block, bool isLowLevel) {
  // do not expand memcpy on Os to reduce codesize
  if (Options::optForSize) {
    return false;
  }

  OpKind memOpKind = ComputeOpKind(stmt);
  if (memOpKind != MEM_OP_memcpy && memOpKind != MEM_OP_memcpy_s) {
    return false;
  }
  uint32 dstOpndIdx = 0;
  uint32 srcOpndIdx = 1;
  uint32 srcSizeOpndIdx = 2;
  bool isSafeVersion = memOpKind == MEM_OP_memcpy_s;
  if (debug) {
    LogInfo::MapleLogger() << "[funcName] " << func->GetName() << std::endl;
    stmt.Dump(0);
  }

  StmtNode* memcpyCallStmt = &stmt;
  if (memOpKind == MEM_OP_memcpy_s) {
    memcpyCallStmt = PartiallyExpandMemcpyS(stmt, block);
    if (!memcpyCallStmt) {
      return true;  // Expand memcpy_s completely, no extra memcpy is generated, so just return true
    }
  } else if (!isLowLevel) {
    auto newDst = TryExtractExpr(*stmt.Opnd(kMemOpDstOpndIdx), *func, block, stmt, false, debug);
    if (newDst) {
      stmt.SetOpnd(newDst, kMemOpDstOpndIdx);
    }
    auto newSrc = TryExtractExpr(*stmt.Opnd(kMemOpSrcOpndIdx), *func, block, stmt, true, debug);
    if (newSrc) {
      stmt.SetOpnd(newSrc, kMemOpSrcOpndIdx);
    }
  }

  uint64 srcSize = 0;
  bool isIntConst = false;
  BaseNode *foldCopySizeExpr = FoldIntConst(memcpyCallStmt->Opnd(srcSizeOpndIdx), srcSize, isIntConst);
  if (foldCopySizeExpr != nullptr) {
    memcpyCallStmt->SetOpnd(foldCopySizeExpr, srcSizeOpndIdx);
  }
  if (!isIntConst) {
    MayPrintLog(debug, false, memOpKind, "src size is not an int const");
    return false;
  }
  uint32 thresholdExpand = (isSafeVersion ? thresholdMemcpySExpand : thresholdMemcpyExpand);
  if (srcSize > thresholdExpand) {
    MayPrintLog(debug, false, memOpKind, "size is too big");
    return false;
  }
  if (srcSize == 0) {
    if (memOpKind == MEM_OP_memcpy) {
      auto *retAssign = MemEntry::GenRetAssign(stmt, *func, isLowLevel, memOpKind);
      InsertBeforeAndMayPrintStmt(block, *memcpyCallStmt, debug, retAssign);
    }
    block.RemoveStmt(memcpyCallStmt);
    return true;
  }
  uint64 copySize = srcSize;
  ErrorNumber errNum = ERRNO_OK;

  MemEntry dstMemEntry;
  bool valid = MemEntry::ComputeMemEntry(*memcpyCallStmt->Opnd(dstOpndIdx), *func, dstMemEntry, isLowLevel);
  if (!valid) {
    MayPrintLog(debug, false, memOpKind, "dstMemEntry is invalid");
    return false;
  }
  MemEntry srcMemEntry;
  valid = MemEntry::ComputeMemEntry(*memcpyCallStmt->Opnd(srcOpndIdx), *func, srcMemEntry, isLowLevel);
  if (!valid) {
    MayPrintLog(debug, false, memOpKind, "srcMemEntry is invalid");
    return false;
  }
  // We don't check type consistency when doing low level expand
  if (!isLowLevel) {
    if (dstMemEntry.memType != srcMemEntry.memType) {
      // any type <-- void (feasible)
      if (srcMemEntry.memType->GetPrimType() != PTY_void) {
        MayPrintLog(debug, false, memOpKind, "dst and src have different type");
        return false;   // entryType must be identical
      }
    }
    if (dstMemEntry.memType->GetSize() != static_cast<uint64>(srcSize)) {
      MayPrintLog(debug, false, memOpKind, "copy size != dst memory size");
      return false;  // copy size should equal to dst memory size, we maybe allow smaller copy size later
    }
  }
  bool ret = false;
  if (copySize != 0) {
    ret = dstMemEntry.ExpandMemcpy(srcMemEntry, copySize, *func, *memcpyCallStmt, block,
                                   isLowLevel, debug, errNum);
  } else {
    // if copySize == 0, no need to copy memory, just return error number
    auto *retAssign = MemEntry::GenRetAssign(*memcpyCallStmt, *func, isLowLevel, memOpKind, errNum);
    InsertBeforeAndMayPrintStmt(block, *memcpyCallStmt, debug, retAssign);
    block.RemoveStmt(memcpyCallStmt);
    ret = true;
  }
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
  auto *kh = GET_ANALYSIS(M2MKlassHierarchy, m);
  ASSERT_NOT_NULL((kh));
  std::unique_ptr<FuncOptimizeImpl> funcOptImpl = std::make_unique<Simplify>(m, kh, *GetPhaseMemPool(),
                                                                             TRACE_MAPLE_PHASE);
  ASSERT_NOT_NULL(funcOptImpl);
  FuncOptimizeIterator opt(PhaseName(), std::move(funcOptImpl));
  opt.Init();
  opt.Run();
  return true;
}
}  // namespace maple
