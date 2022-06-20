/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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

#include "isel.h"
#include "factory.h"
#include "cg.h"
#include "standardize.h"
#include <map>
#include <utility>

namespace maplebe {
/* register,                       imm ,                         memory,                        cond */
#define DEF_FAST_ISEL_MAPPING_INT(SIZE)                                                                       \
MOperator fastIselMapI##SIZE[Operand::OperandType::kOpdPhi][Operand::OperandType::kOpdPhi] = {                \
{abstract::MOP_copy_rr_##SIZE, abstract::MOP_copy_ri_##SIZE, abstract::MOP_load_##SIZE, abstract::MOP_undef}, \
{abstract::MOP_undef,          abstract::MOP_undef,          abstract::MOP_undef,       abstract::MOP_undef}, \
{abstract::MOP_str_##SIZE,     abstract::MOP_undef,          abstract::MOP_undef,       abstract::MOP_undef}, \
{abstract::MOP_undef,          abstract::MOP_undef,          abstract::MOP_undef,       abstract::MOP_undef}, \
};
#define DEF_FAST_ISEL_MAPPING_FLOAT(SIZE)                                                                       \
MOperator fastIselMapF##SIZE[Operand::OperandType::kOpdPhi][Operand::OperandType::kOpdPhi] = {                  \
{abstract::MOP_copy_ff_##SIZE, abstract::MOP_copy_fi_##SIZE, abstract::MOP_load_f_##SIZE, abstract::MOP_undef}, \
{abstract::MOP_undef,          abstract::MOP_undef,          abstract::MOP_undef,         abstract::MOP_undef}, \
{abstract::MOP_str_f_##SIZE,   abstract::MOP_undef,          abstract::MOP_undef,         abstract::MOP_undef}, \
{abstract::MOP_undef,          abstract::MOP_undef,          abstract::MOP_undef,         abstract::MOP_undef}, \
};

DEF_FAST_ISEL_MAPPING_INT(8)
DEF_FAST_ISEL_MAPPING_INT(16)
DEF_FAST_ISEL_MAPPING_INT(32)
DEF_FAST_ISEL_MAPPING_INT(64)
DEF_FAST_ISEL_MAPPING_FLOAT(8)
DEF_FAST_ISEL_MAPPING_FLOAT(16)
DEF_FAST_ISEL_MAPPING_FLOAT(32)
DEF_FAST_ISEL_MAPPING_FLOAT(64)

#define DEF_SEL_MAPPING_TBL(SIZE)                                     \
MOperator SelMapping##SIZE(bool isInt, uint32 x, uint32 y) {          \
  return isInt ? fastIselMapI##SIZE[x][y] : fastIselMapF##SIZE[x][y]; \
}
#define USE_SELMAPPING_TBL(SIZE) \
{SIZE, SelMapping##SIZE}

DEF_SEL_MAPPING_TBL(8);
DEF_SEL_MAPPING_TBL(16);
DEF_SEL_MAPPING_TBL(32);
DEF_SEL_MAPPING_TBL(64);

std::map<uint32, std::function<MOperator (bool, uint32, uint32)>> fastIselMappingTable = {
    USE_SELMAPPING_TBL(8),
    USE_SELMAPPING_TBL(16),
    USE_SELMAPPING_TBL(32),
    USE_SELMAPPING_TBL(64)};

MOperator GetFastIselMop(Operand::OperandType dTy, Operand::OperandType sTy, PrimType type) {
  uint32 bitSize = GetPrimTypeBitSize(type);
  bool isInteger = IsPrimitiveInteger(type);
  auto tableDriven = fastIselMappingTable.find(bitSize);
  if (tableDriven != fastIselMappingTable.end())  {
    auto funcIt = tableDriven->second;
    return funcIt(isInteger, dTy, sTy);
  } else {
    CHECK_FATAL(false, "unsupport type");
  }
  return abstract::MOP_undef;
}

/* EXTEND TYPE: 8to16, 8to32, 8to64, 16to32, 16to64, 32to64 */
#define DEF_ZERO_EXTEND_MAPPING_INT(TYPE)                                                               \
MOperator fastZextMapI_##TYPE[Operand::OperandType::kOpdPhi][Operand::OperandType::kOpdPhi] = {         \
{abstract::MOP_zext_rr_##TYPE, abstract::MOP_zext_ri_##TYPE, abstract::MOP_undef, abstract::MOP_undef}, \
{abstract::MOP_undef,          abstract::MOP_undef,          abstract::MOP_undef, abstract::MOP_undef}, \
{abstract::MOP_undef,          abstract::MOP_undef,          abstract::MOP_undef, abstract::MOP_undef}, \
{abstract::MOP_undef,          abstract::MOP_undef,          abstract::MOP_undef, abstract::MOP_undef}, \
};
DEF_ZERO_EXTEND_MAPPING_INT(16_8)
DEF_ZERO_EXTEND_MAPPING_INT(32_8)
DEF_ZERO_EXTEND_MAPPING_INT(64_8)
DEF_ZERO_EXTEND_MAPPING_INT(32_16)
DEF_ZERO_EXTEND_MAPPING_INT(64_16)
DEF_ZERO_EXTEND_MAPPING_INT(64_32)
#define DEF_SIGN_EXTEND_MAPPING_INT(TYPE)                                                               \
MOperator fastSextMapI_##TYPE[Operand::OperandType::kOpdPhi][Operand::OperandType::kOpdPhi] = {         \
{abstract::MOP_sext_rr_##TYPE, abstract::MOP_sext_ri_##TYPE, abstract::MOP_undef, abstract::MOP_undef}, \
{abstract::MOP_undef,          abstract::MOP_undef,          abstract::MOP_undef, abstract::MOP_undef}, \
{abstract::MOP_undef,          abstract::MOP_undef,          abstract::MOP_undef, abstract::MOP_undef}, \
{abstract::MOP_undef,          abstract::MOP_undef,          abstract::MOP_undef, abstract::MOP_undef}, \
};
DEF_SIGN_EXTEND_MAPPING_INT(16_8)
DEF_SIGN_EXTEND_MAPPING_INT(32_8)
DEF_SIGN_EXTEND_MAPPING_INT(64_8)
DEF_SIGN_EXTEND_MAPPING_INT(32_16)
DEF_SIGN_EXTEND_MAPPING_INT(64_16)
DEF_SIGN_EXTEND_MAPPING_INT(64_32)
#define DEF_EXTEND_MAPPING_TBL(TYPE)                                        \
MOperator ExtendMapping_##TYPE(bool isSigned, uint32 x, uint32 y) {         \
  return isSigned ? fastSextMapI_##TYPE[x][y] : fastZextMapI_##TYPE[x][y];  \
}
DEF_EXTEND_MAPPING_TBL(16_8)
DEF_EXTEND_MAPPING_TBL(32_8)
DEF_EXTEND_MAPPING_TBL(64_8)
DEF_EXTEND_MAPPING_TBL(32_16)
DEF_EXTEND_MAPPING_TBL(64_16)
DEF_EXTEND_MAPPING_TBL(64_32)
/* Trunc Type: 16to8, 32to8, 64to8, 32to16, 64to16, 64to32 */
#define DEF_TRUNC_MAPPING_INT(TYPE)                                                                       \
MOperator fastTruncMapI_##TYPE[Operand::OperandType::kOpdPhi][Operand::OperandType::kOpdPhi] = {          \
{abstract::MOP_trunc_rr_##TYPE, abstract::MOP_trunc_ri_##TYPE, abstract::MOP_undef, abstract::MOP_undef}, \
{abstract::MOP_undef,          abstract::MOP_undef,          abstract::MOP_undef, abstract::MOP_undef},   \
{abstract::MOP_undef,          abstract::MOP_undef,          abstract::MOP_undef, abstract::MOP_undef},   \
{abstract::MOP_undef,          abstract::MOP_undef,          abstract::MOP_undef, abstract::MOP_undef},   \
};
DEF_TRUNC_MAPPING_INT(8_16)
DEF_TRUNC_MAPPING_INT(8_32)
DEF_TRUNC_MAPPING_INT(8_64)
DEF_TRUNC_MAPPING_INT(16_32)
DEF_TRUNC_MAPPING_INT(16_64)
DEF_TRUNC_MAPPING_INT(32_64)
#define DEF_TRUNC_MAPPING_TBL(TYPE)                                         \
MOperator TruncMapping_##TYPE(bool isSigned, uint32 x, uint32 y) {          \
  return fastTruncMapI_##TYPE[x][y];                                        \
}
DEF_TRUNC_MAPPING_TBL(8_16)
DEF_TRUNC_MAPPING_TBL(8_32)
DEF_TRUNC_MAPPING_TBL(8_64)
DEF_TRUNC_MAPPING_TBL(16_32)
DEF_TRUNC_MAPPING_TBL(16_64)
DEF_TRUNC_MAPPING_TBL(32_64)
using fromToTy = std::pair<uint32, uint32>; /* std::pair<from, to> */
std::map<fromToTy, std::function<MOperator (bool, uint32, uint32)>> fastCvtMappingTableI = {
    {std::make_pair(k8BitSize, k16BitSize), ExtendMapping_16_8}, /* Extend Mapping */
    {std::make_pair(k8BitSize, k32BitSize), ExtendMapping_32_8},
    {std::make_pair(k8BitSize, k64BitSize), ExtendMapping_64_8},
    {std::make_pair(k16BitSize, k32BitSize), ExtendMapping_32_16},
    {std::make_pair(k16BitSize, k64BitSize), ExtendMapping_64_16},
    {std::make_pair(k32BitSize, k64BitSize), ExtendMapping_64_32},
    {std::make_pair(k16BitSize, k8BitSize), TruncMapping_8_16}, /* Trunc Mapping */
    {std::make_pair(k32BitSize, k8BitSize), TruncMapping_8_32},
    {std::make_pair(k64BitSize, k8BitSize), TruncMapping_8_64},
    {std::make_pair(k32BitSize, k16BitSize), TruncMapping_16_32},
    {std::make_pair(k64BitSize, k16BitSize), TruncMapping_16_32},
    {std::make_pair(k64BitSize, k32BitSize), TruncMapping_32_64},
};

static MOperator GetFastCvtMopI(Operand::OperandType dOpndTy, Operand::OperandType sOpndTy,
    uint32 fromSize, uint32 toSize, bool isSigned) {
  MOperator mOp = abstract::MOP_undef;
  if (toSize <  k8BitSize || toSize >  k64BitSize) {
    CHECK_FATAL(false, "unsupport type");
  }
  if (fromSize <  k8BitSize || fromSize >  k64BitSize) {
    CHECK_FATAL(false, "unsupport type");
  }
  /* Extend: fromSize < toSize; Trunc: fromSize > toSize */
  auto tableDriven =  fastCvtMappingTableI.find(std::make_pair(fromSize, toSize));
  if (tableDriven == fastCvtMappingTableI.end()) {
    CHECK_FATAL(false, "unsupport cvt");
  }
  auto funcIt = tableDriven->second;
  mOp = funcIt(isSigned, dOpndTy, sOpndTy);
  if (mOp == abstract::MOP_undef) {
    CHECK_FATAL(false, "unsupport cvt");
  }
  return mOp;
}

enum BitIndex : maple::uint8 {
  k8BitIndex = 0,
  k16BitIndex,
  k32BitIndex,
  k64BitIndex,
  kBitIndexEnd,
};

BitIndex GetBitIndex(uint32 bitSize) {
  switch (bitSize) {
    case k8BitSize:
      return k8BitIndex;
    case k16BitSize:
      return k16BitIndex;
    case k32BitSize:
      return k32BitIndex;
    case k64BitSize:
      return k64BitIndex;
    default:
      CHECK_FATAL(false, "NIY, Not support size");
  }
}
/*
 * fast get MOperator
 * such as : and, or, shl ...
 */
#define DEF_MOPERATOR_MAPPING_FUNC(TYPE) [](uint32 bitSize) -> MOperator {                                          \
  /* 8-bits,                16-bits,                   32-bits,                   64-bits */                        \
  constexpr static std::array<MOperator, kBitIndexEnd> fastMapping_##TYPE =                                         \
      {abstract::MOP_##TYPE##_8, abstract::MOP_##TYPE##_16, abstract::MOP_##TYPE##_32, abstract::MOP_##TYPE##_64};  \
  return fastMapping_##TYPE[GetBitIndex(bitSize)];                                                                  \
}

void HandleDassign(StmtNode &stmt, MPISel &iSel) {
  auto &dassignNode = static_cast<DassignNode&>(stmt);
  ASSERT(dassignNode.GetOpCode() == OP_dassign, "expect dassign");
  BaseNode *rhs = dassignNode.GetRHS();
  ASSERT(rhs != nullptr, "get rhs of dassignNode failed");
  Operand* opndRhs = iSel.HandleExpr(dassignNode, *rhs);
  if (opndRhs == nullptr) {
    return;
  }
  /*value = 1 operand */
  iSel.SelectDassign(dassignNode, *opndRhs);
}

void HandleIassign(StmtNode &stmt, MPISel &iSel) {
  ASSERT(stmt.GetOpCode() == OP_iassign, "expect stmt");
  auto &iassignNode = static_cast<IassignNode&>(stmt);
  BaseNode *addr = iassignNode.Opnd(0);
  ASSERT(addr != nullptr, "null ptr check");
  BaseNode *rhs = iassignNode.GetRHS();

  if ((rhs != nullptr) && rhs->GetPrimType() != PTY_agg) {
    iSel.SelectIassign(iassignNode, iSel, *addr, *rhs);
  } else {
    CHECK_FATAL(false, "NIY");
  }
}

void HandleIassignoff(StmtNode &stmt, MPISel &iSel) {
  auto &iassignoffNode = static_cast<IassignoffNode&>(stmt);
  iSel.SelectIassignoff(iassignoffNode);
}

void HandleLabel(StmtNode &stmt, MPISel &iSel) {
  CGFunc *cgFunc = iSel.GetCurFunc();
  ASSERT(stmt.GetOpCode() == OP_label, "error");
  auto &label = static_cast<LabelNode&>(stmt);
  BB *newBB = cgFunc->StartNewBBImpl(false, label);
  newBB->AddLabel(label.GetLabelIdx());
  cgFunc->SetLab2BBMap(newBB->GetLabIdx(), *newBB);
  cgFunc->SetCurBB(*newBB);
}

void HandleGoto(StmtNode &stmt, MPISel &iSel) {
  CGFunc *cgFunc = iSel.GetCurFunc();
  cgFunc->UpdateFrequency(stmt);
  auto &gotoNode = static_cast<GotoNode&>(stmt);
  ASSERT(gotoNode.GetOpCode() == OP_goto, "expect dassign");
  cgFunc->SetCurBBKind(BB::kBBGoto);
  iSel.SelectGoto(gotoNode);
  cgFunc->SetCurBB(*cgFunc->StartNewBB(gotoNode));
  ASSERT(&stmt == &gotoNode, "stmt must be same as gotoNoe");
  if ((gotoNode.GetNext() != nullptr) && (gotoNode.GetNext()->GetOpCode() != OP_label)) {
    ASSERT(cgFunc->GetCurBB()->GetPrev()->GetLastStmt() == &stmt, "check the relation between BB and stmt");
  }
}

void HandleRangeGoto(StmtNode &stmt, MPISel &iSel) {
  auto &rangeGotoNode = static_cast<RangeGotoNode&>(stmt);
  ASSERT(rangeGotoNode.GetOpCode() == OP_rangegoto, "expect rangegoto");
  BaseNode *srcNode = rangeGotoNode.Opnd(0);
  Operand *srcOpnd = iSel.HandleExpr(rangeGotoNode, *srcNode);
  iSel.SelectRangeGoto(rangeGotoNode, *srcOpnd);
}

void HandleReturn(StmtNode &stmt, MPISel &iSel) {
  CGFunc *cgFunc = iSel.GetCurFunc();
  auto &retNode = static_cast<NaryStmtNode&>(stmt);
  ASSERT(retNode.NumOpnds() <= 1, "NYI return nodes number > 1");
  Operand *opnd = nullptr;
  if (retNode.NumOpnds() != 0) {
    opnd = iSel.HandleExpr(retNode, *retNode.Opnd(0));
    iSel.SelectReturn(*opnd);
  }
  cgFunc->SetCurBBKind(BB::kBBReturn);
  cgFunc->SetCurBB(*cgFunc->StartNewBB(retNode));
}

void HandleComment(StmtNode &stmt, MPISel &iSel) {
  return;
}

void HandleCall(StmtNode &stmt, MPISel &iSel) {
  CGFunc *cgFunc = iSel.GetCurFunc();
  ASSERT(stmt.GetOpCode() == OP_call, "error");
  auto &callNode = static_cast<CallNode&>(stmt);
  iSel.SelectCall(callNode);
  if (cgFunc->GetCurBB()->GetKind() != BB::kBBFallthru) {
    cgFunc->SetCurBB(*cgFunc->StartNewBB(callNode));
  }

  StmtNode *prevStmt = stmt.GetPrev();
  if (prevStmt == nullptr || prevStmt->GetOpCode() != OP_catch) {
    return;
  }
  if ((stmt.GetNext() != nullptr) && (stmt.GetNext()->GetOpCode() == OP_label)) {
    cgFunc->SetCurBB(*cgFunc->StartNewBBImpl(true, stmt));
  }
}

void HandleCondbr(StmtNode &stmt, MPISel &iSel) {
  CGFunc *cgFunc = iSel.GetCurFunc();
  auto &condGotoNode = static_cast<CondGotoNode&>(stmt);
  BaseNode *condNode = condGotoNode.Opnd(0);
  ASSERT(condNode != nullptr, "expect first operand of cond br");
  cgFunc->SetCurBBKind(BB::kBBIf);
  /* select cmpOp Insn and get the result "opnd0". However, the opnd0 is not used
   * in most backend architectures */
  Operand *opnd0 = iSel.HandleExpr(stmt, *condNode);
  iSel.SelectCondGoto(condGotoNode, *condNode, *opnd0);
  cgFunc->SetCurBB(*cgFunc->StartNewBB(condGotoNode));
}

Operand *HandleAddrof(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  auto &addrofNode = static_cast<AddrofNode&>(expr);
  return iSel.SelectAddrof(addrofNode, parent);
}

Operand *HandleShift(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  return iSel.SelectShift(static_cast<BinaryNode&>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
                          *iSel.HandleExpr(expr, *expr.Opnd(1)), parent);
}

Operand *HandleCvt(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  return iSel.SelectCvt(parent, static_cast<TypeCvtNode&>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)));
}

Operand *HandleExtractBits(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  return iSel.SelectExtractbits(parent, static_cast<ExtractbitsNode&>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)));
}

Operand *HandleDread(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  auto &dreadNode = static_cast<AddrofNode&>(expr);
  return iSel.SelectDread(parent, dreadNode);
}

Operand *HandleAdd(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  return iSel.SelectAdd(static_cast<BinaryNode&>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
                        *iSel.HandleExpr(expr, *expr.Opnd(1)), parent);
}

Operand *HandleBior(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  return iSel.SelectBior(static_cast<BinaryNode&>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
                         *iSel.HandleExpr(expr, *expr.Opnd(1)), parent);
}

Operand *HandleBxor(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  return iSel.SelectBxor(static_cast<BinaryNode&>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
                         *iSel.HandleExpr(expr, *expr.Opnd(1)), parent);
}

Operand *HandleSub(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  return iSel.SelectSub(static_cast<BinaryNode&>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
                        *iSel.HandleExpr(expr, *expr.Opnd(1)), parent);
}

Operand *HandleNeg(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  return iSel.SelectNeg(static_cast<UnaryNode&>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
}

Operand *HandleDiv(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  return iSel.SelectDiv(static_cast<BinaryNode&>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
                        *iSel.HandleExpr(expr, *expr.Opnd(1)), parent);
}

Operand *HandleBand(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  return iSel.SelectBand(static_cast<BinaryNode&>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
                         *iSel.HandleExpr(expr, *expr.Opnd(1)), parent);
}

Operand *HandleMpy(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  return iSel.SelectMpy(static_cast<BinaryNode&>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
                        *iSel.HandleExpr(expr, *expr.Opnd(1)), parent);
}

Operand *HandleConstStr(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  auto &constStrNode = static_cast<ConststrNode&>(expr);
  return iSel.SelectStrLiteral(constStrNode);
}

Operand *HandleConstVal(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  auto &constValNode = static_cast<ConstvalNode&>(expr);
  MIRConst *mirConst = constValNode.GetConstVal();
  ASSERT(mirConst != nullptr, "get constval of constvalnode failed");
  if (mirConst->GetKind() == kConstInt) {
    auto *mirIntConst = safe_cast<MIRIntConst>(mirConst);
    return iSel.SelectIntConst(*mirIntConst);
  } else {
    CHECK_FATAL(false, "NIY");
  }
  return nullptr;
}

Operand *HandleRegread(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  (void)parent;
  auto &regReadNode = static_cast<RegreadNode&>(expr);
  if (regReadNode.GetRegIdx() == -kSregRetval0 || regReadNode.GetRegIdx() == -kSregRetval1) {
    return &iSel.ProcessReturnReg(regReadNode.GetPrimType(), -(regReadNode.GetRegIdx()));
  }
  return iSel.SelectRegread(regReadNode);
}

Operand *HandleIread(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  auto &ireadNode = static_cast<IreadNode&>(expr);
  return iSel.SelectIread(parent, ireadNode);
}
Operand *HandleIreadoff(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  auto &ireadNode = static_cast<IreadoffNode&>(expr);
  return iSel.SelectIreadoff(parent, ireadNode);
}

Operand *HandleBnot(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  return iSel.SelectBnot(static_cast<UnaryNode&>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
}

void HandleEval(const StmtNode &stmt, MPISel &iSel) {
  (void)iSel.HandleExpr(stmt, *static_cast<const UnaryStmtNode&>(stmt).Opnd(0));
}

Operand *HandleDepositBits(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  return iSel.SelectDepositBits(static_cast<DepositbitsNode&>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
                                *iSel.HandleExpr(expr, *expr.Opnd(1)), parent);
}

Operand *HandleCmp(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  // fix opnd type before select insn
  PrimType targetPtyp = parent.GetPrimType();
  if (kOpcodeInfo.IsCompare(parent.GetOpCode())) {
    targetPtyp = static_cast<const CompareNode&>(parent).GetOpndType();
  } else if (kOpcodeInfo.IsTypeCvt(parent.GetOpCode())) {
    targetPtyp = static_cast<const TypeCvtNode&>(parent).FromType();
  }
  if (IsPrimitiveInteger(targetPtyp) && targetPtyp != expr.GetPrimType()) {
    expr.SetPrimType(targetPtyp);
  } else if (IsPrimitiveInteger(expr.GetPrimType()) && PTY_i32 != expr.GetPrimType()) {
    /* IR return is i32 */
    expr.SetPrimType(PTY_i32);
  }
  return iSel.SelectCmpOp(static_cast<CompareNode&>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
                          *iSel.HandleExpr(expr, *expr.Opnd(1)), parent);
}

using HandleStmtFactory = FunctionFactory<Opcode, void, StmtNode&, MPISel&>;
using HandleExprFactory = FunctionFactory<Opcode, maplebe::Operand*, const BaseNode&, BaseNode&, MPISel&>;
namespace isel {
void InitHandleStmtFactory() {
  RegisterFactoryFunction<HandleStmtFactory>(OP_label, HandleLabel);
  RegisterFactoryFunction<HandleStmtFactory>(OP_dassign, HandleDassign);
  RegisterFactoryFunction<HandleStmtFactory>(OP_iassign, HandleIassign);
  RegisterFactoryFunction<HandleStmtFactory>(OP_iassignoff, HandleIassignoff);
  RegisterFactoryFunction<HandleStmtFactory>(OP_return, HandleReturn);
  RegisterFactoryFunction<HandleStmtFactory>(OP_comment, HandleComment);
  RegisterFactoryFunction<HandleStmtFactory>(OP_call, HandleCall);
  RegisterFactoryFunction<HandleStmtFactory>(OP_goto, HandleGoto);
  RegisterFactoryFunction<HandleStmtFactory>(OP_rangegoto, HandleRangeGoto);
  RegisterFactoryFunction<HandleStmtFactory>(OP_brfalse, HandleCondbr);
  RegisterFactoryFunction<HandleStmtFactory>(OP_brtrue, HandleCondbr);
  RegisterFactoryFunction<HandleStmtFactory>(OP_eval, HandleEval);
}
void InitHandleExprFactory() {
  RegisterFactoryFunction<HandleExprFactory>(OP_dread, HandleDread);
  RegisterFactoryFunction<HandleExprFactory>(OP_add, HandleAdd);
  RegisterFactoryFunction<HandleExprFactory>(OP_sub, HandleSub);
  RegisterFactoryFunction<HandleExprFactory>(OP_neg, HandleNeg);
  RegisterFactoryFunction<HandleExprFactory>(OP_mul, HandleMpy);
  RegisterFactoryFunction<HandleExprFactory>(OP_constval, HandleConstVal);
  RegisterFactoryFunction<HandleExprFactory>(OP_regread, HandleRegread);
  RegisterFactoryFunction<HandleExprFactory>(OP_addrof, HandleAddrof);
  RegisterFactoryFunction<HandleExprFactory>(OP_shl, HandleShift);
  RegisterFactoryFunction<HandleExprFactory>(OP_lshr, HandleShift);
  RegisterFactoryFunction<HandleExprFactory>(OP_ashr, HandleShift);
  RegisterFactoryFunction<HandleExprFactory>(OP_cvt, HandleCvt);
  RegisterFactoryFunction<HandleExprFactory>(OP_zext, HandleExtractBits);
  RegisterFactoryFunction<HandleExprFactory>(OP_sext, HandleExtractBits);
  RegisterFactoryFunction<HandleExprFactory>(OP_extractbits, HandleExtractBits);
  RegisterFactoryFunction<HandleExprFactory>(OP_depositbits, HandleDepositBits);
  RegisterFactoryFunction<HandleExprFactory>(OP_band, HandleBand);
  RegisterFactoryFunction<HandleExprFactory>(OP_bior, HandleBior);
  RegisterFactoryFunction<HandleExprFactory>(OP_bxor, HandleBxor);
  RegisterFactoryFunction<HandleExprFactory>(OP_iread, HandleIread);
  RegisterFactoryFunction<HandleExprFactory>(OP_ireadoff, HandleIreadoff);
  RegisterFactoryFunction<HandleExprFactory>(OP_bnot, HandleBnot);
  RegisterFactoryFunction<HandleExprFactory>(OP_div, HandleDiv);
  RegisterFactoryFunction<HandleExprFactory>(OP_conststr, HandleConstStr);
  RegisterFactoryFunction<HandleExprFactory>(OP_le, HandleCmp);
  RegisterFactoryFunction<HandleExprFactory>(OP_ge, HandleCmp);
  RegisterFactoryFunction<HandleExprFactory>(OP_gt, HandleCmp);
  RegisterFactoryFunction<HandleExprFactory>(OP_lt, HandleCmp);
  RegisterFactoryFunction<HandleExprFactory>(OP_ne, HandleCmp);
  RegisterFactoryFunction<HandleExprFactory>(OP_eq, HandleCmp);
}
}

Operand *MPISel::HandleExpr(const BaseNode &parent, BaseNode &expr) {
  auto function = CreateProductFunction<HandleExprFactory>(expr.GetOpCode());
  CHECK_FATAL(function != nullptr, "unsupported opCode in HandleExpr()");
  return function(parent, expr, *this);
}

void MPISel::doMPIS() {
  isel::InitHandleStmtFactory();
  isel::InitHandleExprFactory();
  StmtNode *secondStmt = HandleFuncEntry();
  for (StmtNode *stmt = secondStmt; stmt != nullptr; stmt = stmt->GetNext()) {
    auto function = CreateProductFunction<HandleStmtFactory>(stmt->GetOpCode());
    CHECK_FATAL(function != nullptr, "unsupported opCode or has been lowered before");
    function(*stmt, *this);
  }
  HandleFuncExit();
}

void MPISel::SelectBasicOp(Operand &resOpnd, Operand &opnd0, Operand &opnd1, MOperator mOp, PrimType primType) {
  CGRegOperand &firstOpnd = SelectCopy2Reg(opnd0, primType);
  CGRegOperand &secondOpnd = SelectCopy2Reg(opnd1, primType);
  Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, InsnDescription::GetAbstractId(mOp));
  insn.AddOperandChain(resOpnd).AddOperandChain(firstOpnd).AddOperandChain(secondOpnd);
  cgFunc->GetCurBB()->AppendInsn(insn);
}

void MPISel::SelectDassign(const DassignNode &stmt, Operand &opndRhs) {
  SelectDassign(stmt.GetStIdx(), stmt.GetFieldID(), stmt.GetRHS()->GetPrimType(), opndRhs);
}

void MPISel::SelectDassign(StIdx stIdx, FieldID fieldId, PrimType rhsPType, Operand &opndRhs) {
  MIRSymbol *symbol = cgFunc->GetFunction().GetLocalOrGlobalSymbol(stIdx);
  /* Get symbol location */
  CGMemOperand &symbolMem = GetOrCreateMemOpndFromSymbol(*symbol, fieldId);
  /* Generate Insn */
  PrimType symType = symbol->GetType()->GetPrimType();
  if (fieldId != 0) {
    ASSERT(symbol->GetType()->GetKind() == kTypeStruct, "non-structure");
    symType = static_cast<MIRStructType*>(symbol->GetType())->GetFieldType(fieldId)->GetPrimType();
  }
  SelectCopy(symbolMem, opndRhs, symType, rhsPType);
}

void MPISel::SelectIassign(const IassignNode &stmt, MPISel &iSel, BaseNode &addr, BaseNode &rhs) {
  FieldID fieldId = stmt.GetFieldID();

  Operand* opndRhs = iSel.HandleExpr(stmt, rhs);
  Operand* opndAddr = iSel.HandleExpr(stmt, addr);
  if (opndRhs == nullptr || opndAddr == nullptr) {
    return;
  }
  /* handle Lhs, generate (%Rxx) via Rxx*/
  MIRPtrType *pointerType = static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(stmt.GetTyIdx()));
  ASSERT(pointerType != nullptr, "expect a pointer type at iassign node");
  MIRType *pointedType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointerType->GetPointedTyIdx());
  CGMemOperand *memOpndAddr = nullptr;
  if (opndAddr->IsRegister()) {
    int32 fieldOffset = 0;
    PrimType destType = pointedType->GetPrimType();
    if (fieldId != 0) {
      ASSERT(pointedType->GetKind() == kTypeStruct, "non-structure");
      MIRStructType *structType = static_cast<MIRStructType*>(pointedType);
      fieldOffset = static_cast<uint32>(cgFunc->GetBecommon().GetFieldOffset(*structType, fieldId).first);
      destType = structType->GetFieldType(fieldId)->GetPrimType();
      destType = (destType == PTY_agg) ? PTY_a64 : destType;
    }
    CGRegOperand *opnd = static_cast<CGRegOperand *>(opndAddr);
    uint32 size = GetPrimTypeBitSize(destType);
    memOpndAddr = &cgFunc->GetOpndBuilder()->CreateMem(*opnd, static_cast<int64>(fieldOffset), size);
  } else {
    CHECK_FATAL(false, "NIY");
  }
  /* handle Rhs, get R## from Rhs */
  CGRegOperand *regOpndRhs = nullptr;
  PrimType rhsType = rhs.GetPrimType();
  if (opndRhs->IsRegister()) {
    regOpndRhs = static_cast<CGRegOperand *> (opndRhs);
  } else {
    uint32 rhsSize = GetPrimTypeBitSize(rhsType);
    RegType regType;
    if (IsPrimitiveInteger(rhsType) || IsPrimitiveVectorInteger(rhsType)) {
      regType = kRegTyInt;
    } else {
      CHECK_FATAL(false, "NIY");
    }
    regOpndRhs = &cgFunc->GetOpndBuilder()->CreateVReg(rhsSize, regType);
    SelectCopy(*regOpndRhs, *opndRhs, rhsType);
  }
  /* mov %R##, (%Rxx) */
  SelectCopy(*memOpndAddr, *regOpndRhs, rhsType);
}

void MPISel::SelectIassignoff(const IassignoffNode &stmt) {
  Operand *addr = HandleExpr(stmt, *stmt.Opnd(0));
  ASSERT(addr != nullptr, "null ptr check");
  Operand *rhs = HandleExpr(stmt, *stmt.Opnd(1));
  ASSERT(rhs != nullptr, "null ptr check");

  int32 offset = stmt.GetOffset();
  PrimType primType = stmt.GetPrimType();
  uint32 bitSize = GetPrimTypeBitSize(primType);
  CGRegOperand &addrReg = SelectCopy2Reg(*addr, PTY_a64);
  CGRegOperand &rhsReg = SelectCopy2Reg(*rhs, primType);

  CGMemOperand &memOpnd = cgFunc->GetOpndBuilder()->CreateMem(addrReg, offset, bitSize);
  SelectCopy(memOpnd, rhsReg, primType);
}

CGImmOperand *MPISel::SelectIntConst(MIRIntConst &intConst) {
  uint32 opndSz = GetPrimTypeSize(intConst.GetType().GetPrimType()) * kBitsPerByte;
  return &cgFunc->GetOpndBuilder()->CreateImm(opndSz, intConst.GetValue());
}

Operand *MPISel::SelectShift(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) {
  PrimType dtype = node.GetPrimType();
  bool isSigned = IsSignedInteger(dtype);
  uint32 dsize = GetPrimTypeBitSize(dtype);
  bool is64Bits = (dsize == k64BitSize);
  bool isFloat = IsPrimitiveFloat(dtype);
  CGRegOperand *resOpnd = nullptr;
  Opcode opcode = node.GetOpCode();

  if (false) {
    /* TODO : primitive vector */
  } else {
    PrimType primType = isFloat ? dtype : (is64Bits ? (isSigned ? PTY_i64 : PTY_u64) :
        (isSigned ? PTY_i32 : PTY_u32));
    resOpnd = &(cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType),
        cgFunc->GetRegTyFromPrimTy(primType)));
    SelectShift(*resOpnd, opnd0, opnd1, opcode, primType);
  }

  return resOpnd;
}

void MPISel::SelectShift(Operand &resOpnd, Operand &opnd0, Operand &opnd1, Opcode shiftDirect,
                         PrimType primType) {
  if (opnd1.IsIntImmediate() && static_cast<CGImmOperand&>(opnd1).GetValue() == 0) {
    SelectCopy(resOpnd, opnd0, primType);
    return;
  }

  uint32 dsize = GetPrimTypeBitSize(primType);
  MOperator mOp = abstract::MOP_undef;
  if (shiftDirect == OP_shl) {
    static auto fastShlMappingFunc = DEF_MOPERATOR_MAPPING_FUNC(shl);
    mOp = fastShlMappingFunc(dsize);
  } else if (shiftDirect == OP_ashr) {
    static auto fastAshrMappingFunc = DEF_MOPERATOR_MAPPING_FUNC(ashr);
    mOp = fastAshrMappingFunc(dsize);
  } else if (shiftDirect == OP_lshr) {
    static auto fastLshrMappingFunc = DEF_MOPERATOR_MAPPING_FUNC(lshr);
    mOp = fastLshrMappingFunc(dsize);
  } else {
    CHECK_FATAL(false, "NIY, Not support shiftdirect case");
  }
  SelectBasicOp(resOpnd, opnd0, opnd1, mOp, primType);
}

Operand *MPISel::SelectDread(const BaseNode &parent, const AddrofNode &expr) {
  MIRSymbol *symbol = cgFunc->GetFunction().GetLocalOrGlobalSymbol(expr.GetStIdx());
  /* Get symbol location */
  CGMemOperand &symbolMem = GetOrCreateMemOpndFromSymbol(*symbol, expr.GetFieldID());
  PrimType primType = expr.GetPrimType();
  PrimType symType = symbol->GetType()->GetPrimType();
  if (expr.GetFieldID() != 0) {
    ASSERT(symbol->GetType()->GetKind() == kTypeStruct, "non-structure");
    symType = static_cast<MIRStructType*>(symbol->GetType())->GetFieldType(expr.GetFieldID())->GetPrimType();
  }
  CGRegOperand &regOpnd = cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType),
      cgFunc->GetRegTyFromPrimTy(symType));
  /* Generate Insn */
  SelectCopy(regOpnd, symbolMem, primType, symType);
  return &regOpnd;
}

Operand *MPISel::SelectAdd(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) {
  PrimType dtype = node.GetPrimType();
  bool isSigned = IsSignedInteger(dtype);
  uint32 dsize = GetPrimTypeBitSize(dtype);
  bool is64Bits = (dsize == k64BitSize);
  bool isFloat = IsPrimitiveFloat(dtype);
  PrimType primType =
      isFloat ? dtype : ((is64Bits ? (isSigned ? PTY_i64 : PTY_u64) : (isSigned ? PTY_i32 : PTY_u32)));
  CGRegOperand &resReg = cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(dtype),
      cgFunc->GetRegTyFromPrimTy(primType));
  SelectAdd(resReg, opnd0, opnd1, primType);
  return &resReg;
}

Operand *MPISel::SelectBand(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) {
  PrimType dtype = node.GetPrimType();
  bool isSigned = IsSignedInteger(dtype);
  uint32 dsize = GetPrimTypeBitSize(dtype);
  bool is64Bits = (dsize == k64BitSize);
  bool isFloat = IsPrimitiveFloat(dtype);
  PrimType primType =
      isFloat ? dtype : ((is64Bits ? (isSigned ? PTY_i64 : PTY_u64) : (isSigned ? PTY_i32 : PTY_u32)));
  CGRegOperand &resReg = cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(dtype),
      cgFunc->GetRegTyFromPrimTy(primType));
  SelectBand(resReg, opnd0, opnd1, primType);
  return &resReg;
}

Operand *MPISel::SelectSub(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) {
  PrimType dtype = node.GetPrimType();
  bool isSigned = IsSignedInteger(dtype);
  uint32 dsize = GetPrimTypeBitSize(dtype);
  bool is64Bits = (dsize == k64BitSize);
  bool isFloat = IsPrimitiveFloat(dtype);
  CGRegOperand *resOpnd = nullptr;
  PrimType primType =
      isFloat ? dtype : ((is64Bits ? (isSigned ? PTY_i64 : PTY_u64) : (isSigned ? PTY_i32 : PTY_u32)));
  resOpnd = &cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(dtype),
      cgFunc->GetRegTyFromPrimTy(primType));
  SelectSub(*resOpnd, opnd0, opnd1, primType);
  return resOpnd;
}

void MPISel::SelectExtractbits(CGRegOperand &resOpnd, CGRegOperand &opnd0, uint8 bitOffset,
                               uint8 bitSize, PrimType primType) {
  /*
   * opcode is extractbits, need this
   * tmpOpnd = opnd0 << (primBitSize - bitSize - bitOffset)
   * resOpnd = tmpOpnd >> (primBitSize - bitSize)
   * if signed : use sar; else use shr
   */
  uint32 primBitSize = GetPrimTypeBitSize(primType);
  CGRegOperand &tmpOpnd = cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType),
      cgFunc->GetRegTyFromPrimTy(primType));
  CGImmOperand &imm1Opnd = cgFunc->GetOpndBuilder()->CreateImm(k8BitSize, primBitSize - bitSize - bitOffset);
  SelectShift(tmpOpnd, opnd0, imm1Opnd, OP_shl, primType);
  Opcode opcode = IsSignedInteger(primType) ? OP_ashr : OP_lshr;
  CGImmOperand &imm2Opnd = cgFunc->GetOpndBuilder()->CreateImm(k8BitSize, primBitSize - bitSize);
  SelectShift(resOpnd, tmpOpnd, imm2Opnd, opcode, primType);
}

Operand *MPISel::SelectExtractbits(const BaseNode &parent, const ExtractbitsNode &node, Operand &opnd0) {
  PrimType toType = node.GetPrimType();
  CGRegOperand *resOpnd = nullptr;
  if (IsPrimitiveInteger(toType)) {
    CGRegOperand *tmpOpnd = &cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(toType),
        cgFunc->GetRegTyFromPrimTy(toType));
    SelectIntCvt(*tmpOpnd, opnd0, toType);
    if (node.GetOpCode() == OP_extractbits) {
      resOpnd = &cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(toType),
          cgFunc->GetRegTyFromPrimTy(toType));
      SelectExtractbits(*resOpnd, *tmpOpnd, node.GetBitsOffset(), node.GetBitsSize(), toType);
    } else {
      resOpnd = tmpOpnd;
    }
  } else {
    CHECK_FATAL(false, "NIY vector cvt");
  }
  ASSERT(resOpnd != nullptr, "null check");
  return resOpnd;
}

Operand *MPISel::SelectCvt(const BaseNode &parent, const TypeCvtNode &node, Operand &opnd0) {
  PrimType fromType = node.FromType();
  PrimType toType = node.GetPrimType();
  if (fromType == toType) {
    return &opnd0;
  }
  CGRegOperand *resOpnd = &cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(toType),
      cgFunc->GetRegTyFromPrimTy(toType));
  if (IsPrimitiveInteger(toType) || IsPrimitiveInteger(fromType)) {
    SelectIntCvt(*resOpnd, opnd0, toType);
  } else {
    CHECK_FATAL(false, "NIY vector cvt");
  }
  return resOpnd;
}

void MPISel::SelectIntCvt(maplebe::CGRegOperand &resOpnd, maplebe::Operand &opnd0, maple::PrimType toType) {
  uint32 fromSize = opnd0.GetSize();
  uint32 toSize = GetPrimTypeBitSize(toType);
  /*
   * It is redundancy to insert "nop" casts (unsigned 32 -> singed 32) in abstract CG IR
   * The signedness of operands would be shown in the expression.
   */
  if (toSize == fromSize) {
    resOpnd = static_cast<CGRegOperand&>(opnd0);
    return;
  }
  bool isSigned = !IsPrimitiveUnsigned(toType);
  MOperator mOp = GetFastCvtMopI(resOpnd.GetKind(), opnd0.GetKind(), fromSize, toSize, isSigned);
  Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, InsnDescription::GetAbstractId(mOp));
  insn.AddOperandChain(resOpnd).AddOperandChain(opnd0);
  cgFunc->GetCurBB()->AppendInsn(insn);
  return;
}

void MPISel::SelectSub(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) {
  static auto fastSubMappingFunc = DEF_MOPERATOR_MAPPING_FUNC(sub);
  MOperator mOp = fastSubMappingFunc(GetPrimTypeBitSize(primType));
  SelectBasicOp(resOpnd, opnd0, opnd1, mOp, primType);
}

void MPISel::SelectBand(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) {
  static auto fastBandMappingFunc = DEF_MOPERATOR_MAPPING_FUNC(and);
  MOperator mOp = fastBandMappingFunc(GetPrimTypeBitSize(primType));
  SelectBasicOp(resOpnd, opnd0, opnd1, mOp, primType);
}

void MPISel::SelectAdd(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) {
  static auto fastAddMappingFunc = DEF_MOPERATOR_MAPPING_FUNC(add);
  MOperator mOp = fastAddMappingFunc(GetPrimTypeBitSize(primType));
  SelectBasicOp(resOpnd, opnd0, opnd1, mOp, primType);
}

Operand* MPISel::SelectNeg(const UnaryNode &node, Operand &opnd0, const BaseNode &parent) {
  PrimType dtype = node.GetPrimType();

  CGRegOperand *resOpnd = nullptr;
  if (!IsPrimitiveVector(dtype)) {
    resOpnd = &cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(dtype),
        cgFunc->GetRegTyFromPrimTy(dtype));
    SelectNeg(*resOpnd, opnd0, dtype);
  } else {
    /* vector operand */
    CHECK_FATAL(false, "NIY");
  }
  return resOpnd;
}

void MPISel::SelectNeg(Operand &resOpnd, Operand &opnd0, PrimType primType) {
  MOperator mOp = abstract::MOP_neg_32;
  Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, InsnDescription::GetAbstractId(mOp));
  insn.AddOperandChain(resOpnd).AddOperandChain(opnd0);
  cgFunc->GetCurBB()->AppendInsn(insn);
}

Operand *MPISel::SelectBior(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) {
  PrimType primType = node.GetPrimType();
  CGRegOperand *resOpnd = &cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType),
      cgFunc->GetRegTyFromPrimTy(primType));
  SelectBior(*resOpnd, opnd0, opnd1, primType);
  return resOpnd;
}

void MPISel::SelectBior(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) {
  static auto fastBiorMappingFunc = DEF_MOPERATOR_MAPPING_FUNC(or);
  MOperator mOp = fastBiorMappingFunc(GetPrimTypeBitSize(primType));
  SelectBasicOp(resOpnd, opnd0, opnd1, mOp, primType);
}

Operand *MPISel::SelectBxor(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) {
  PrimType dtype = node.GetPrimType();
  uint32 dsize = GetPrimTypeBitSize(dtype);
  CGRegOperand *resOpnd = nullptr;
  if (dsize == k32BitSize) {
    resOpnd = &cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(dtype),
        cgFunc->GetRegTyFromPrimTy(dtype));
    MOperator mOp = abstract::MOP_xor_32;
    Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, InsnDescription::GetAbstractId(mOp));
    insn.AddOperandChain(*resOpnd).AddOperandChain(opnd0).AddOperandChain(opnd1);
    cgFunc->GetCurBB()->AppendInsn(insn);
  } else {
    CHECK_FATAL(false, "NIY");
  }
  return resOpnd;
}

static MIRType *GetPointedToType(const MIRPtrType &pointerType) {
  MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointerType.GetPointedTyIdx());
  if (mirType->GetKind() == kTypeArray) {
    MIRArrayType *arrayType = static_cast<MIRArrayType*>(mirType);
    return GlobalTables::GetTypeTable().GetTypeFromTyIdx(arrayType->GetElemTyIdx());
  }
  if (mirType->GetKind() == kTypeFArray || mirType->GetKind() == kTypeJArray) {
    MIRFarrayType *farrayType = static_cast<MIRFarrayType*>(mirType);
    return GlobalTables::GetTypeTable().GetTypeFromTyIdx(farrayType->GetElemTyIdx());
  }
  return mirType;
}

Operand *MPISel::SelectIread(const BaseNode &parent, const IreadNode &expr, int extraOffset) {
  FieldID fieldId = expr.GetFieldID();
  MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(expr.GetTyIdx());
  MIRPtrType *pointerType = static_cast<MIRPtrType*>(type);
  ASSERT(pointerType != nullptr, "expect a pointer type at iread node");
  MIRType *pointedType = nullptr;
  pointedType = GetPointedToType(*pointerType);
  int32 fieldOffset = 0;
  PrimType destType = pointedType->GetPrimType();
  if (fieldId != 0) {
    ASSERT(pointedType->GetKind() == kTypeStruct, "non-structure");
    MIRStructType *structType = static_cast<MIRStructType*>(pointedType);
    fieldOffset = static_cast<uint32>(cgFunc->GetBecommon().GetFieldOffset(*structType, fieldId).first);
    destType = structType->GetFieldType(fieldId)->GetPrimType();
  }
  PrimType primType = expr.GetPrimType();
  Operand *addrOpnd = HandleExpr(expr, *expr.Opnd(0));
  CGRegOperand &addrOnReg = SelectCopy2Reg(*addrOpnd, PTY_a64);
  CGMemOperand &memOpnd = cgFunc->GetOpndBuilder()->CreateMem(addrOnReg,
      static_cast<int64>(fieldOffset) + extraOffset, GetPrimTypeBitSize(destType));
  CGRegOperand &result = cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType),
      cgFunc->GetRegTyFromPrimTy(primType));
  SelectCopy(result, memOpnd, primType);
  return &result;
}

Operand *MPISel::SelectIreadoff(const BaseNode &parent, const IreadoffNode &ireadoff) {
  int32 offset = ireadoff.GetOffset();
  PrimType primType = ireadoff.GetPrimType();
  uint32 bitSize = GetPrimTypeBitSize(primType);

  Operand *addrOpnd = HandleExpr(ireadoff, *ireadoff.Opnd(0));
  CGRegOperand &addrOnReg = SelectCopy2Reg(*addrOpnd, PTY_a64);
  CGMemOperand &memOpnd = cgFunc->GetOpndBuilder()->CreateMem(addrOnReg, offset, bitSize);
  CGRegOperand &result = cgFunc->GetOpndBuilder()->CreateVReg(bitSize,
      cgFunc->GetRegTyFromPrimTy(primType));
  SelectCopy(result, memOpnd, primType);
  return &result;
}

static inline uint64 CreateDepositBitsImm1(uint32 primBitSize, uint8 bitOffset, uint8 bitSize) {
  /* $imm1 = 1(primBitSize - bitSize - bitOffset)0(bitSize)1(bitOffset) */
  uint64 val = UINT64_MAX;      // 0xFFFFFFFFFFFFFFFF
  val <<= (bitSize + bitOffset);
  val |= (static_cast<uint64>(1) << bitOffset) - 1;
  return val;
}

Operand *MPISel::SelectDepositBits(const DepositbitsNode &node, Operand &opnd0, Operand &opnd1,
                                   const BaseNode &parent) {
  uint8 bitOffset = node.GetBitsOffset();
  uint8 bitSize = node.GetBitsSize();
  PrimType primType = node.GetPrimType();
  uint32 primBitSize = GetPrimTypeBitSize(primType);
  ASSERT((primBitSize == k64BitSize) || (bitOffset < k32BitSize), "wrong bitSize");

  /*
   * resOpnd = (opnd0 and $imm1) or (opnd1 << bitOffset)
   * $imm1 = 1(primBitSize - bitSize - bitOffset)0(bitSize)1(bitOffset)
   */
  uint64 imm1Val = CreateDepositBitsImm1(primBitSize, bitOffset, bitSize);
  CGImmOperand &imm1Opnd = cgFunc->GetOpndBuilder()->CreateImm(primBitSize,
      static_cast<int64>(imm1Val));
  /* and */
  CGRegOperand &tmpOpnd = cgFunc->GetOpndBuilder()->CreateVReg(primBitSize,
      cgFunc->GetRegTyFromPrimTy(primType));
  SelectBand(tmpOpnd, opnd0, imm1Opnd, primType);

  CGRegOperand &resOpnd = cgFunc->GetOpndBuilder()->CreateVReg(primBitSize,
      cgFunc->GetRegTyFromPrimTy(primType));
  if (opnd1.IsIntImmediate()) {
    /* opnd1 is immediate, imm2 = opnd1.val << bitOffset */
    int64 imm2Val = static_cast<CGImmOperand&>(opnd1).GetValue() << bitOffset;
    CGImmOperand &imm2Opnd = cgFunc->GetOpndBuilder()->CreateImm(primBitSize, imm2Val);
    /* or */
    SelectBior(resOpnd, tmpOpnd, imm2Opnd, primType);
  } else {
    CGRegOperand &regOpnd1 = SelectCopy2Reg(opnd1, primType);
    /* shift -- (opnd1 << bitOffset) */
    CGRegOperand &shiftOpnd = cgFunc->GetOpndBuilder()->CreateVReg(primBitSize,
        cgFunc->GetRegTyFromPrimTy(primType));
    CGImmOperand &countOpnd = cgFunc->GetOpndBuilder()->CreateImm(k8BitSize, bitOffset);
    SelectShift(shiftOpnd, regOpnd1, countOpnd, OP_shl, primType);
    /* or */
    SelectBior(resOpnd, tmpOpnd, shiftOpnd, primType);
  }
  return &resOpnd;
}

StmtNode *MPISel::HandleFuncEntry() {
  MIRFunction &mirFunc = cgFunc->GetFunction();
  BlockNode *block = mirFunc.GetBody();

  ASSERT(block != nullptr, "get func body block failed in CGFunc::GenerateInstruction");

  StmtNode *stmt = block->GetFirst();
  if (stmt == nullptr) {
    return nullptr;
  }
  ASSERT(stmt->GetOpCode() == OP_label, "The first statement should be a label");
  HandleLabel(*stmt, *this);
  cgFunc->SetFirstBB(*cgFunc->GetCurBB());
  stmt = stmt->GetNext();
  if (stmt == nullptr) {
    return nullptr;
  }
  cgFunc->SetCurBB(*cgFunc->StartNewBBImpl(false, *stmt));
  bool withFreqInfo = mirFunc.HasFreqMap() && !mirFunc.GetLastFreqMap().empty();
  if (withFreqInfo) {
    cgFunc->GetCurBB()->SetFrequency(kFreqBase);
  }

  return stmt;
}

/* This function loads src to a register, the src can be an imm, mem or a label.
 * Once the source and result(destination) types are different,
 * implicit conversion is executed here.*/
CGRegOperand &MPISel::SelectCopy2Reg(Operand &src, PrimType dtype) {
  ASSERT(src.GetSize() == GetPrimTypeBitSize(dtype), "NIY");
  if (src.IsRegister()) {
    return static_cast<CGRegOperand&>(src);
  }
  CGRegOperand &dest = cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(dtype),
      cgFunc->GetRegTyFromPrimTy(dtype));
  SelectCopy(dest, src, dtype);
  return dest;
}

void MPISel::SelectCopy(Operand &dest, Operand &src, PrimType toType, PrimType fromType) {
  if (fromType != toType) {
    CGRegOperand &srcRegOpnd = SelectCopy2Reg(src, fromType);
    CGRegOperand &dstRegOpnd = cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(toType),
        cgFunc->GetRegTyFromPrimTy(toType));
    SelectIntCvt(dstRegOpnd, srcRegOpnd, toType);
    SelectCopy(dest, dstRegOpnd, toType);
  } else {
    SelectCopy(dest, src, toType);
  }
}

void MPISel::SelectCopy(Operand &dest, Operand &src, PrimType type) {
  ASSERT(dest.GetSize() == src.GetSize(), "NIY");
  if (dest.GetKind() == Operand::kOpdRegister){
    if (src.GetKind() == Operand::kOpdImmediate) {
      SelectCopyInsn<CGRegOperand, CGImmOperand>(static_cast<CGRegOperand&>(dest),
          static_cast<CGImmOperand&>(src), type);
    } else if (src.GetKind() == Operand::kOpdMem) {
      SelectCopyInsn<CGRegOperand, CGMemOperand>(static_cast<CGRegOperand&>(dest),
          static_cast<CGMemOperand&>(src), type);
    } else if (src.GetKind() == Operand::kOpdRegister) {
      SelectCopyInsn<CGRegOperand, CGRegOperand>(static_cast<CGRegOperand&>(dest),
          static_cast<CGRegOperand&>(src), type);
    } else {
      CHECK_FATAL(false, "NIY");
    }
  } else if (dest.GetKind() == Operand::kOpdMem) {
    if (src.GetKind() != Operand::kOpdRegister) {
      CGRegOperand &tempReg = cgFunc->GetOpndBuilder()->CreateVReg(src.GetSize(),
          cgFunc->GetRegTyFromPrimTy(type));
      SelectCopy(tempReg, src, type);
      SelectCopyInsn<CGMemOperand, CGRegOperand>(static_cast<CGMemOperand&>(dest), tempReg, type);
    } else {
      SelectCopyInsn<CGMemOperand, CGRegOperand>(static_cast<CGMemOperand&>(dest),
          static_cast<CGRegOperand&>(src), type);
    }
  }else {
    CHECK_FATAL(false, "NIY, CPU supports more than memory and registers");
  }
  return;
}

template<typename destTy, typename srcTy>
void MPISel::SelectCopyInsn(destTy &dest, srcTy &src, PrimType type) {
  MOperator mop = GetFastIselMop(dest.GetKind(), src.GetKind(), type);
  CHECK_FATAL(mop != abstract::MOP_undef, "get mop failed");
  Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mop, InsnDescription::GetAbstractId(mop));
  insn.AddOperandChain(dest).AddOperandChain(src);
  cgFunc->GetCurBB()->AppendInsn(insn);
}

Operand *MPISel::SelectBnot(const UnaryNode &node, Operand &opnd0, const BaseNode &parent) {
  PrimType dtype = node.GetPrimType();
  ASSERT(IsPrimitiveInteger(dtype), "bnot expect integer");
  uint32 dsize = GetPrimTypeBitSize(dtype);
  CGRegOperand *resOpnd = nullptr;
  resOpnd = &cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(dtype),
      cgFunc->GetRegTyFromPrimTy(dtype));
  MOperator mOp = abstract::MOP_undef;
  switch (dsize) {
    case k32BitSize:
      mOp = abstract::MOP_not_32;
      break;
    case k64BitSize:
      mOp = abstract::MOP_not_64;
      break;
    default:
      CHECK_FATAL(false, "NIY, unsupported type(16bit or 8 bit) for bnot insn");
      break;
  }
  Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, InsnDescription::GetAbstractId(mOp));
  insn.AddOperandChain(*resOpnd).AddOperandChain(opnd0);
  cgFunc->GetCurBB()->AppendInsn(insn);
  return resOpnd;
}

CGRegOperand *MPISel::SelectRegread(RegreadNode &expr) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

void MPISel::HandleFuncExit() {
  BlockNode *block = cgFunc->GetFunction().GetBody();
  ASSERT(block != nullptr, "get func body block failed in CGFunc::GenerateInstruction");
  cgFunc->GetCurBB()->SetLastStmt(*block->GetLast());
  /* TODO : Set lastbb's frequency */
  cgFunc->SetLastBB(*cgFunc->GetCurBB());
  cgFunc->SetCleanupBB(*cgFunc->GetCurBB()->GetPrev());
}

bool InstructionSelector::PhaseRun(maplebe::CGFunc &f) {
  MPISel *mpIS = f.GetCG()->CreateMPIsel(*GetPhaseMemPool(), f);
  mpIS->doMPIS();
  Standardize *stdz = f.GetCG()->CreateStandardize(*GetPhaseMemPool(), f);
  stdz->DoStandardize();
  return true;
}
}
