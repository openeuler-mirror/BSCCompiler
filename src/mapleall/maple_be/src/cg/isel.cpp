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
#include <map>
#include <utility>
#include "factory.h"
#include "cg.h"
#include "cgfunc.h"

namespace maplebe {
/* Does not support size larget than 64 bits */
#define PTY128MOD(pty) ((pty) = (((pty) == PTY_i128) ? PTY_i64 : (((pty) == PTY_u128) ? PTY_u64 : (pty))))
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

#define DEF_EXTEND_MAPPING_TBL(TYPE) [](bool isSigned) -> MOperator {                       \
  return isSigned ? abstract::MOP_sext_rr_##TYPE : abstract::MOP_zext_rr_##TYPE;            \
}
using fromToTy = std::pair<uint32, uint32>; /* std::pair<from, to> */
#define DEF_USE_EXTEND_MAPPING_TBL(FROMSIZE, TOSIZE)                                        \
{{k##FROMSIZE##BitSize, k##TOSIZE##BitSize}, DEF_EXTEND_MAPPING_TBL(TOSIZE##_##FROMSIZE)}

std::map<fromToTy, std::function<MOperator (bool)>> fastCvtMappingTableI = {
    DEF_USE_EXTEND_MAPPING_TBL(8, 16),      /* Extend Mapping */
    DEF_USE_EXTEND_MAPPING_TBL(8, 32),
    DEF_USE_EXTEND_MAPPING_TBL(8, 64),
    DEF_USE_EXTEND_MAPPING_TBL(16, 32),
    DEF_USE_EXTEND_MAPPING_TBL(16, 64),
    DEF_USE_EXTEND_MAPPING_TBL(32, 64),
};
#undef DEF_USE_EXTEND_MAPPING_TBL
#undef DEF_EXTEND_MAPPING_TBL

static MOperator GetFastCvtMopI(uint32 fromSize, uint32 toSize, bool isSigned) {
  if (toSize <  k8BitSize || toSize >  k64BitSize) {
    CHECK_FATAL(false, "unsupport type");
  }
  if (fromSize <  k8BitSize || fromSize >  k64BitSize) {
    CHECK_FATAL(false, "unsupport type");
  }
  /* Extend/Truncate: fromSize < toSize */
  auto tableDriven =  fastCvtMappingTableI.find({fromSize, toSize});
  if (tableDriven == fastCvtMappingTableI.end()) {
    CHECK_FATAL(false, "unsupport cvt");
  }
  MOperator mOp = tableDriven->second(isSigned);
  if (mOp == abstract::MOP_undef) {
    CHECK_FATAL(false, "unsupport cvt");
  }
  return mOp;
}

/*
 * fast get MOperator
 * such as : and, or, shl ...
 */
#define DEF_MOPERATOR_MAPPING_FUNC(TYPE) [](uint32 bitSize)->MOperator {                                            \
  /* 8-bits,                16-bits,                   32-bits,                   64-bits */                        \
  constexpr static std::array<MOperator, kBitIndexEnd> fastMapping_##TYPE =                                         \
      {abstract::MOP_##TYPE##_8, abstract::MOP_##TYPE##_16, abstract::MOP_##TYPE##_32, abstract::MOP_##TYPE##_64};  \
  return fastMapping_##TYPE[GetBitIndex(bitSize)];                                                                  \
}

#define DEF_FLOAT_MOPERATOR_MAPPING_FUNC(TYPE) [](uint32 bitSize)->MOperator {                                      \
  /* 8-bits,                16-bits,                   32-bits,                   64-bits */                        \
  constexpr static std::array<MOperator, kBitIndexEnd> fastMapping_f_##TYPE =                                       \
      {abstract::MOP_##TYPE##_f_8, abstract::MOP_##TYPE##_f_16,                                                     \
      abstract::MOP_##TYPE##_f_32, abstract::MOP_##TYPE##_f_64};                                                    \
  return fastMapping_f_##TYPE[GetBitIndex(bitSize)];                                                                \
}

void HandleDassign(StmtNode &stmt, MPISel &iSel) {
  ASSERT(stmt.GetOpCode() == OP_dassign, "expect dassign");
  auto &dassignNode = static_cast<DassignNode&>(stmt);
  BaseNode *rhs = dassignNode.GetRHS();
  ASSERT(rhs != nullptr, "get rhs of dassignNode failed");
  Operand* opndRhs = iSel.HandleExpr(dassignNode, *rhs);
  if (opndRhs == nullptr) {
    return;
  }
  iSel.SelectDassign(dassignNode, *opndRhs);
}

void HandleDassignoff(StmtNode &stmt, MPISel &iSel) {
  auto &dassignoffNode = static_cast<DassignoffNode&>(stmt);
  BaseNode *rhs = dassignoffNode.GetRHS();
  CHECK_FATAL(rhs->GetOpCode() == OP_constval, "dassignoffNode without constval");
  Operand *opnd0 = iSel.HandleExpr(dassignoffNode, *rhs);
  iSel.SelectDassignoff(dassignoffNode, *opnd0);
}

void HandleIassign(StmtNode &stmt, MPISel &iSel) {
  ASSERT(stmt.GetOpCode() == OP_iassign, "expect iassign");
  auto &iassignNode = static_cast<IassignNode&>(stmt);
  BaseNode *rhs = iassignNode.GetRHS();
  ASSERT(rhs != nullptr, "null ptr check");
  Operand *opndRhs = iSel.HandleExpr(stmt, *rhs);
  BaseNode *addr = iassignNode.Opnd(0);
  ASSERT(addr != nullptr, "null ptr check");
  Operand *opndAddr = iSel.HandleExpr(stmt, *addr);
  if (opndRhs == nullptr || opndAddr == nullptr) {
    return;
  }
  if (rhs->GetPrimType() != PTY_agg) {
    iSel.SelectIassign(iassignNode, *opndAddr, *opndRhs);
  } else {
    iSel.SelectAggIassign(iassignNode, *opndAddr, *opndRhs);
  }
}

void HandleRegassign(StmtNode &stmt, MPISel &iSel) {
  ASSERT(stmt.GetOpCode() == OP_regassign, "expect regAssign");
  auto &regAssignNode = static_cast<RegassignNode&>(stmt);
  BaseNode *operand = regAssignNode.Opnd(0);
  ASSERT(operand != nullptr, "get operand of regassignNode failed");
  Operand *opnd0 = iSel.HandleExpr(regAssignNode, *operand);
  iSel.SelectRegassign(regAssignNode, *opnd0);
}

void HandleIassignoff(StmtNode &stmt, MPISel &iSel) {
  auto &iassignoffNode = static_cast<IassignoffNode&>(stmt);
  iSel.SelectIassignoff(iassignoffNode);
}

void HandleLabel(StmtNode &stmt, const MPISel &iSel) {
  CGFunc *cgFunc = iSel.GetCurFunc();
  ASSERT(stmt.GetOpCode() == OP_label, "error");
  auto &label = static_cast<LabelNode&>(stmt);
  BB *newBB = cgFunc->StartNewBBImpl(false, label);
  newBB->SetKind(BB::kBBFallthru);
  newBB->AddLabel(label.GetLabelIdx());
  cgFunc->SetLab2BBMap(static_cast<int32>(newBB->GetLabIdx()), *newBB);
  cgFunc->SetCurBB(*newBB);

  if (cgFunc->GetCleanupLabel() == &label) {
    cgFunc->SetCleanupBB(*newBB);
  } else if (cgFunc->GetReturnLabel() == &label) {
    cgFunc->SetReturnBB(*newBB);
  }
}

void HandleGoto(StmtNode &stmt, MPISel &iSel) {
  CGFunc *cgFunc = iSel.GetCurFunc();
  cgFunc->UpdateFrequency(stmt);
  auto &gotoNode = static_cast<GotoNode&>(stmt);
  ASSERT(gotoNode.GetOpCode() == OP_goto, "expect goto");
  cgFunc->SetCurBBKind(BB::kBBGoto);
  iSel.SelectGoto(gotoNode);
  cgFunc->SetCurBB(*cgFunc->StartNewBB(gotoNode));
  ASSERT(&stmt == &gotoNode, "stmt must be same as gotoNoe");
  if ((gotoNode.GetNext() != nullptr) && (gotoNode.GetNext()->GetOpCode() != OP_label)) {
    ASSERT(cgFunc->GetCurBB()->GetPrev()->GetLastStmt() == &stmt, "check the relation between BB and stmt");
  }
}

void HandleIntrinCall(StmtNode &stmt, MPISel &iSel) {
  auto &call = static_cast<IntrinsiccallNode&>(stmt);
  iSel.SelectIntrinCall(call);
}

void HandleRangeGoto(StmtNode &stmt, MPISel &iSel) {
  CGFunc *cgFunc = iSel.GetCurFunc();
  auto &rangeGotoNode = static_cast<RangeGotoNode&>(stmt);
  ASSERT(rangeGotoNode.GetOpCode() == OP_rangegoto, "expect rangegoto");
  BaseNode *srcNode = rangeGotoNode.Opnd(0);
  Operand *srcOpnd = iSel.HandleExpr(rangeGotoNode, *srcNode);
  cgFunc->SetCurBBKind(BB::kBBRangeGoto);
  iSel.SelectRangeGoto(rangeGotoNode, *srcOpnd);
  cgFunc->SetCurBB(*cgFunc->StartNewBB(rangeGotoNode));
}

void HandleIgoto(StmtNode &stmt, MPISel &iSel) {
  CGFunc *cgFunc = iSel.GetCurFunc();
  auto &igotoNode = static_cast<UnaryStmtNode&>(stmt);
  BaseNode *targetNode = igotoNode.Opnd(0);
  Operand *targetOpnd = iSel.HandleExpr(igotoNode, *targetNode);
  iSel.SelectIgoto(*targetOpnd);
  cgFunc->SetCurBBKind(BB::kBBIgoto);
  cgFunc->SetCurBB(*cgFunc->StartNewBB(igotoNode));
}

void HandleReturn(StmtNode &stmt, MPISel &iSel) {
  CGFunc *cgFunc = iSel.GetCurFunc();
  auto &retNode = static_cast<NaryStmtNode&>(stmt);
  ASSERT(retNode.NumOpnds() <= 1, "NYI return nodes number > 1");
  if (retNode.NumOpnds() != 0) {
    iSel.SelectReturn(retNode);
  }
  iSel.SelectReturn(retNode.NumOpnds() == 0);
  /* return stmt will jump to the ret BB, so curBB is gotoBB */
  cgFunc->SetCurBBKind(BB::kBBGoto);
  cgFunc->SetCurBB(*cgFunc->StartNewBB(retNode));
}

void HandleComment(StmtNode &stmt [[maybe_unused]], MPISel &iSel [[maybe_unused]]) {
  return;
}

void HandleIcall(StmtNode &stmt, MPISel &iSel) {
  ASSERT(stmt.GetOpCode() == OP_icall || stmt.GetOpCode() == OP_icallproto, "error");
  auto &iCallNode = static_cast<IcallNode&>(stmt);
  Operand *opnd0 = iSel.HandleExpr(iCallNode, *iCallNode.Opnd(0));
  iSel.SelectIcall(iCallNode, *opnd0);
  iSel.SelectCallCommon(stmt, iSel);
}

void HandleCall(StmtNode &stmt, MPISel &iSel) {
  ASSERT(stmt.GetOpCode() == OP_call, "error");
  auto &callNode = static_cast<CallNode&>(stmt);
  iSel.SelectCall(callNode);
  iSel.SelectCallCommon(stmt, iSel);
}

void HandleCondbr(StmtNode &stmt, MPISel &iSel) {
  CGFunc *cgFunc = iSel.GetCurFunc();
  auto &condGotoNode = static_cast<CondGotoNode&>(stmt);
  BaseNode *condNode = condGotoNode.Opnd(0);
  ASSERT(condNode != nullptr, "expect first operand of cond br");
  /* select cmpOp Insn and get the result "opnd0". However, the opnd0 is not used
   * in most backend architectures */
  iSel.SelectCondGoto(condGotoNode, *condNode);
  cgFunc->SetCurBB(*cgFunc->StartNewBB(condGotoNode));
}

Operand *HandleAddrof(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  auto &addrofNode = static_cast<AddrofNode&>(expr);
  return iSel.SelectAddrof(addrofNode, parent);
}

Operand *HandleAddroffunc(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  auto &addrofNode = static_cast<AddroffuncNode&>(expr);
  return iSel.SelectAddrofFunc(addrofNode, parent);
}

Operand *HandleAddrofLabel(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  auto &addrofLabelNode = static_cast<AddroflabelNode&>(expr);
  return iSel.SelectAddrofLabel(addrofLabelNode, parent);
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

Operand *HandleRem(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  return iSel.SelectRem(static_cast<BinaryNode&>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
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

Operand *HandleConstStr(const BaseNode &parent [[maybe_unused]], BaseNode &expr, MPISel &iSel) {
  auto &constStrNode = static_cast<ConststrNode&>(expr);
  return iSel.SelectStrLiteral(constStrNode);
}

Operand *HandleTrunc(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  return iSel.SelectCvt(parent, static_cast<TypeCvtNode&>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)));
}

Operand *HandleConstVal(const BaseNode &parent, BaseNode &expr, const MPISel &iSel) {
  auto &constValNode = static_cast<ConstvalNode&>(expr);
  MIRConst *mirConst = constValNode.GetConstVal();
  ASSERT(mirConst != nullptr, "get constval of constvalnode failed");
  if (mirConst->GetKind() == kConstInt) {
    auto *mirIntConst = safe_cast<MIRIntConst>(mirConst);
    return iSel.SelectIntConst(*mirIntConst, constValNode.GetPrimType());
  } else if (mirConst->GetKind() == kConstDoubleConst) {
    auto *mirDoubleConst = safe_cast<MIRDoubleConst>(mirConst);
    return iSel.SelectFloatingConst(*mirDoubleConst, constValNode.GetPrimType(), parent);
  } else if (mirConst->GetKind() == kConstFloatConst) {
    auto *mirFloatConst = safe_cast<MIRFloatConst>(mirConst);
    return iSel.SelectFloatingConst(*mirFloatConst, constValNode.GetPrimType(), parent);
  } else {
    CHECK_FATAL(false, "NIY");
  }
  return nullptr;
}

Operand *HandleRegread(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  (void)parent;
  auto &regReadNode = static_cast<RegreadNode&>(expr);
  /* handle return Val */
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
  }
  return iSel.SelectCmpOp(static_cast<CompareNode&>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
                          *iSel.HandleExpr(expr, *expr.Opnd(1)), parent);
}

Operand *HandleAbs(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  return iSel.SelectAbs(static_cast<UnaryNode&>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
}

Operand *HandleAlloca(const BaseNode /* &parent */, BaseNode &expr, MPISel &iSel) {
  return iSel.SelectAlloca(static_cast<UnaryNode&>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)));
}

Operand *HandleCGArrayElemAdd(const BaseNode /* &parent */, const BaseNode &expr, MPISel &iSel) {
  return iSel.SelectCGArrayElemAdd(static_cast<const BinaryNode&>(expr));
}

void HandleAsm(StmtNode &stmt, MPISel &iSel) {
  iSel.SelectAsm(static_cast<AsmNode&>(stmt));
}

Operand *HandleSelect(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  /* 0,1,2 represent the first opnd and the second opnd and the third opnd of expr */
  Operand &trueOpnd = *iSel.HandleExpr(expr, *expr.Opnd(1));
  Operand &falseOpnd = *iSel.HandleExpr(expr, *expr.Opnd(2));
  Operand &condOpnd = *iSel.HandleExpr(expr, *expr.Opnd(0));
  if (condOpnd.IsImmediate()) {
    return (static_cast<ImmOperand&>(condOpnd).GetValue() == 0) ? &falseOpnd : &trueOpnd;
  }
  return iSel.SelectSelect(static_cast<TernaryNode&>(expr), condOpnd, trueOpnd, falseOpnd, parent);
}

Operand *HandleMin(const BaseNode /* &parent */, BaseNode &expr, MPISel &iSel) {
  return iSel.SelectMin(static_cast<BinaryNode&>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
                        *iSel.HandleExpr(expr, *expr.Opnd(1)));
}

Operand *HandleMax(const BaseNode /* &parent */, BaseNode &expr, MPISel &iSel) {
  return iSel.SelectMax(static_cast<BinaryNode&>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
                        *iSel.HandleExpr(expr, *expr.Opnd(1)));
}
Operand *HandleRetype(const BaseNode /* &parent */, BaseNode &expr, MPISel &iSel) {
  return iSel.SelectRetype(static_cast<TypeCvtNode&>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)));
}

Operand *HandleIntrinOp(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  auto &intrinsicopNode = static_cast<IntrinsicopNode&>(expr);
  switch (intrinsicopNode.GetIntrinsic()) {
    case INTRN_C_sin:
      return iSel.SelectCsin(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
    case INTRN_C_sinh:
      return iSel.SelectCsinh(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
    case INTRN_C_asin:
      return iSel.SelectCasin(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
    case INTRN_C_cos:
      return iSel.SelectCcos(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
    case INTRN_C_cosh:
      return iSel.SelectCcosh(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
    case INTRN_C_acos:
      return iSel.SelectCacos(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
    case INTRN_C_atan:
      return iSel.SelectCatan(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
    case INTRN_C_exp:
      return iSel.SelectCexp(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
    case INTRN_C_log:
      return iSel.SelectClog(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
    case INTRN_C_log10:
      return iSel.SelectClog10(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);

    case INTRN_C_sinf:
      return iSel.SelectCsinf(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
    case INTRN_C_sinhf:
      return iSel.SelectCsinhf(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
    case INTRN_C_asinf:
      return iSel.SelectCasinf(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
    case INTRN_C_cosf:
      return iSel.SelectCcosf(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
    case INTRN_C_coshf:
      return iSel.SelectCcoshf(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
    case INTRN_C_acosf:
      return iSel.SelectCacosf(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
    case INTRN_C_atanf:
      return iSel.SelectCatanf(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
    case INTRN_C_expf:
      return iSel.SelectCexpf(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
    case INTRN_C_logf:
      return iSel.SelectClogf(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
    case INTRN_C_log10f:
      return iSel.SelectClog10f(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);

    case INTRN_C_ffs:
      return iSel.SelectCffs(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
    case INTRN_C_memcmp:
      return iSel.SelectCmemcmp(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
    case INTRN_C_strlen:
      return iSel.SelectCstrlen(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
    case INTRN_C_strcmp:
      return iSel.SelectCstrcmp(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
    case INTRN_C_strncmp:
      return iSel.SelectCstrncmp(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
    case INTRN_C_strchr:
      return iSel.SelectCstrchr(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
    case INTRN_C_strrchr:
      return iSel.SelectCstrrchr(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);

    case INTRN_C_rev16_2:
    case INTRN_C_rev_4:
    case INTRN_C_rev_8:
      return iSel.SelectBswap(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
    case INTRN_C_clz32:
    case INTRN_C_clz64:
      return iSel.SelectCclz(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
    case INTRN_C_ctz32:
    case INTRN_C_ctz64:
      return iSel.SelectCctz(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
    default:
      CHECK_FATAL(false, "NIY, unsupported intrinsicop.");
      return nullptr;
  }
}

using HandleStmtFactory = FunctionFactory<Opcode, void, StmtNode&, MPISel&>;
using HandleExprFactory = FunctionFactory<Opcode, maplebe::Operand*, const BaseNode&, BaseNode&, MPISel&>;
namespace isel {
void InitHandleStmtFactory() {
  RegisterFactoryFunction<HandleStmtFactory>(OP_label, HandleLabel);
  RegisterFactoryFunction<HandleStmtFactory>(OP_dassign, HandleDassign);
  RegisterFactoryFunction<HandleStmtFactory>(OP_dassignoff, HandleDassignoff);
  RegisterFactoryFunction<HandleStmtFactory>(OP_iassign, HandleIassign);
  RegisterFactoryFunction<HandleStmtFactory>(OP_iassignoff, HandleIassignoff);
  RegisterFactoryFunction<HandleStmtFactory>(OP_regassign, HandleRegassign);
  RegisterFactoryFunction<HandleStmtFactory>(OP_return, HandleReturn);
  RegisterFactoryFunction<HandleStmtFactory>(OP_comment, HandleComment);
  RegisterFactoryFunction<HandleStmtFactory>(OP_call, HandleCall);
  RegisterFactoryFunction<HandleStmtFactory>(OP_icall, HandleIcall);
  RegisterFactoryFunction<HandleStmtFactory>(OP_icallproto, HandleIcall);
  RegisterFactoryFunction<HandleStmtFactory>(OP_goto, HandleGoto);
  RegisterFactoryFunction<HandleStmtFactory>(OP_intrinsiccall, HandleIntrinCall);
  RegisterFactoryFunction<HandleStmtFactory>(OP_rangegoto, HandleRangeGoto);
  RegisterFactoryFunction<HandleStmtFactory>(OP_igoto, HandleIgoto);
  RegisterFactoryFunction<HandleStmtFactory>(OP_brfalse, HandleCondbr);
  RegisterFactoryFunction<HandleStmtFactory>(OP_brtrue, HandleCondbr);
  RegisterFactoryFunction<HandleStmtFactory>(OP_eval, HandleEval);
  RegisterFactoryFunction<HandleStmtFactory>(OP_asm, HandleAsm);
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
  RegisterFactoryFunction<HandleExprFactory>(OP_addroffunc, HandleAddroffunc);
  RegisterFactoryFunction<HandleExprFactory>(OP_addroflabel, HandleAddrofLabel);
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
  RegisterFactoryFunction<HandleExprFactory>(OP_rem, HandleRem);
  RegisterFactoryFunction<HandleExprFactory>(OP_conststr, HandleConstStr);
  RegisterFactoryFunction<HandleExprFactory>(OP_le, HandleCmp);
  RegisterFactoryFunction<HandleExprFactory>(OP_ge, HandleCmp);
  RegisterFactoryFunction<HandleExprFactory>(OP_gt, HandleCmp);
  RegisterFactoryFunction<HandleExprFactory>(OP_lt, HandleCmp);
  RegisterFactoryFunction<HandleExprFactory>(OP_ne, HandleCmp);
  RegisterFactoryFunction<HandleExprFactory>(OP_eq, HandleCmp);
  RegisterFactoryFunction<HandleExprFactory>(OP_abs, HandleAbs);
  RegisterFactoryFunction<HandleExprFactory>(OP_alloca, HandleAlloca);
  RegisterFactoryFunction<HandleExprFactory>(OP_CG_array_elem_add, HandleCGArrayElemAdd);
  RegisterFactoryFunction<HandleExprFactory>(OP_select, HandleSelect);
  RegisterFactoryFunction<HandleExprFactory>(OP_min, HandleMin);
  RegisterFactoryFunction<HandleExprFactory>(OP_max, HandleMax);
  RegisterFactoryFunction<HandleExprFactory>(OP_retype, HandleRetype);
  RegisterFactoryFunction<HandleExprFactory>(OP_trunc, HandleTrunc);
  RegisterFactoryFunction<HandleExprFactory>(OP_intrinsicop, HandleIntrinOp);
  RegisterFactoryFunction<HandleExprFactory>(OP_trunc, HandleTrunc);
}
}

Operand *MPISel::HandleExpr(const BaseNode &parent, BaseNode &expr) {
  auto function = CreateProductFunction<HandleExprFactory>(expr.GetOpCode());
  CHECK_FATAL(function != nullptr, "unsupported opCode in HandleExpr()");
  return function(parent, expr, *this);
}

void MPISel::DoMPIS() {
  isel::InitHandleStmtFactory();
  isel::InitHandleExprFactory();
  GetCurFunc()->Link2ISel(this);
  SrcPosition lastLocPos = SrcPosition();
  SrcPosition lastMplPos = SrcPosition();
  StmtNode *secondStmt = HandleFuncEntry();
  for (StmtNode *stmt = secondStmt; stmt != nullptr; stmt = stmt->GetNext()) {
    /* insert Insn for .loc before cg for the stmt */
    GetCurFunc()->GenerateLoc(*stmt, lastLocPos, lastMplPos);

    auto function = CreateProductFunction<HandleStmtFactory>(stmt->GetOpCode());
    CHECK_FATAL(function != nullptr, "unsupported opCode or has been lowered before");
    function(*stmt, *this);
  }
  HandleFuncExit();
}

PrimType MPISel::GetIntegerPrimTypeFromSize(bool isSigned, uint32 bitSize) const {
  static constexpr std::array<PrimType, kBitIndexEnd> signedPrimType = {PTY_i8, PTY_i16, PTY_i32, PTY_i64};
  static constexpr std::array<PrimType, kBitIndexEnd> unsignedPrimType = {PTY_u8, PTY_u16, PTY_u32, PTY_u64};
  BitIndex index = GetBitIndex(bitSize);
  return isSigned ? signedPrimType[index] : unsignedPrimType[index];
}

void MPISel::SelectCallCommon(StmtNode &stmt, const MPISel &iSel) const {
  CGFunc *cgFuncTemp = iSel.GetCurFunc();
  if (cgFuncTemp->GetCurBB()->GetKind() != BB::kBBFallthru) {
    cgFuncTemp->SetCurBB(*cgFuncTemp->StartNewBB(stmt));
  }
  StmtNode *prevStmt = stmt.GetPrev();
  if (prevStmt == nullptr || prevStmt->GetOpCode() != OP_catch) {
    return;
  }
  if ((stmt.GetNext() != nullptr) && (stmt.GetNext()->GetOpCode() == OP_label)) {
    cgFuncTemp->SetCurBB(*cgFuncTemp->StartNewBBImpl(true, stmt));
  }
}

void MPISel::SelectBasicOp(Operand &resOpnd, Operand &opnd0, Operand &opnd1, MOperator mOp, PrimType primType) {
  RegOperand &firstOpnd = SelectCopy2Reg(opnd0, primType);
  RegOperand &secondOpnd = SelectCopy2Reg(opnd1, primType);
  Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, InsnDesc::GetAbstractId(mOp));
  (void)insn.AddOpndChain(resOpnd).AddOpndChain(firstOpnd).AddOpndChain(secondOpnd);
  cgFunc->GetCurBB()->AppendInsn(insn);
}

std::pair<FieldID, MIRType*> MPISel::GetFieldIdAndMirTypeFromMirNode(const BaseNode &node) {
  FieldID fieldId = 0;
  MIRType *mirType = nullptr;
  if (node.GetOpCode() == maple::OP_iread) {
    /* mirType stored in an addr. */
    auto &iread = static_cast<const IreadNode&>(node);
    fieldId = iread.GetFieldID();
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iread.GetTyIdx());
    MIRPtrType *pointerType = static_cast<MIRPtrType*>(type);
    ASSERT(pointerType != nullptr, "expect a pointer type at iread node");
    mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointerType->GetPointedTyIdx());
    if (mirType->GetKind() == kTypeArray) {
      MIRArrayType *arrayType = static_cast<MIRArrayType*>(mirType);
      mirType =  GlobalTables::GetTypeTable().GetTypeFromTyIdx(arrayType->GetElemTyIdx());
    }
  } else if (node.GetOpCode() == maple::OP_dassign) {
    /* mirSymbol */
    auto &dassign = static_cast<const DassignNode&>(node);
    fieldId = dassign.GetFieldID();
    MIRSymbol *symbol = cgFunc->GetFunction().GetLocalOrGlobalSymbol(dassign.GetStIdx());
    ASSERT(symbol != nullptr, "nullptr check");
    mirType = symbol->GetType();
  } else if (node.GetOpCode() == maple::OP_dread) {
    /* mirSymbol */
    auto &dread = static_cast<const AddrofNode&>(node);
    fieldId = dread.GetFieldID();
    MIRSymbol *symbol = cgFunc->GetFunction().GetLocalOrGlobalSymbol(dread.GetStIdx());
    CHECK_FATAL(symbol != nullptr, "symbol should not be nullptr");
    mirType = symbol->GetType();
  } else if (node.GetOpCode() == maple::OP_iassign) {
    auto &iassign = static_cast<const IassignNode&>(node);
    fieldId = iassign.GetFieldID();
    AddrofNode &addrofNode = static_cast<AddrofNode&>(iassign.GetAddrExprBase());
    MIRType *iassignMirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iassign.GetTyIdx());
    MIRPtrType *pointerType = nullptr;
    if (iassignMirType->GetPrimType() == PTY_agg) {
      MIRSymbol *addrSym = cgFunc->GetMirModule().CurFunction()->GetLocalOrGlobalSymbol(addrofNode.GetStIdx());
      MIRType *addrMirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(addrSym->GetTyIdx());
      addrMirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(addrMirType->GetTypeIndex());
      ASSERT(addrMirType->GetKind() == kTypePointer, "non-pointer");
      pointerType = static_cast<MIRPtrType*>(addrMirType);
    } else {
      ASSERT(iassignMirType->GetKind() == kTypePointer, "non-pointer");
      pointerType = static_cast<MIRPtrType*>(iassignMirType);
    }
    mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointerType->GetPointedTyIdx());
  } else {
    CHECK_FATAL(false, "unsupported OpCode");
  }
  return {fieldId, mirType};
}

MirTypeInfo MPISel::GetMirTypeInfoFormFieldIdAndMirType(FieldID fieldId, MIRType *mirType) {
  MirTypeInfo mirTypeInfo;
  /* fixup primType and offset */
  if (fieldId != 0) {
    ASSERT((mirType->IsMIRStructType() || mirType->IsMIRUnionType()), "non-structure");
    MIRStructType *structType = static_cast<MIRStructType*>(mirType);
    mirType = structType->GetFieldType(fieldId);
    mirTypeInfo.offset = structType->GetFieldOffsetFromBaseAddr(fieldId).byteOffset;
  }
  mirTypeInfo.primType = mirType->GetPrimType();
  // aggSize for AggType
  if (mirTypeInfo.primType == maple::PTY_agg) {
    mirTypeInfo.size = mirType->GetSize();
  }
  return mirTypeInfo;
}

MirTypeInfo MPISel::GetMirTypeInfoFromMirNode(const BaseNode &node) {
  auto [fieldId, mirType] = GetFieldIdAndMirTypeFromMirNode(node);
  return GetMirTypeInfoFormFieldIdAndMirType(fieldId, mirType);
}

void MPISel::SelectDassign(const DassignNode &stmt, Operand &opndRhs) {
  /* mirSymbol info */
  MIRSymbol *symbol = cgFunc->GetFunction().GetLocalOrGlobalSymbol(stmt.GetStIdx());
  MirTypeInfo symbolInfo = GetMirTypeInfoFromMirNode(stmt);
  CHECK_NULL_FATAL(symbol);
  /* Get symbol location */
  MemOperand &symbolMem = GetOrCreateMemOpndFromSymbol(*symbol, stmt.GetFieldID());
  /* rhs mirType info */
  PrimType rhsType = stmt.GetRHS()->GetPrimType();
  /* Generate Insn */
  if (rhsType == PTY_agg) {
    /* Agg Type */
    SelectAggDassign(symbolInfo, symbolMem, opndRhs, stmt);
    return;
  }
  PrimType memType = symbolInfo.primType;
  if (memType == PTY_agg) {
    memType = PTY_a64;
  }
  if (IsSymbolRequireIndirection(*symbol)) {
    RegOperand &regOpnd = cgFunc->GetOpndBuilder()->CreateVReg(k64BitSize, kRegTyInt);
    SelectCopy(regOpnd, symbolMem, PTY_a64, PTY_a64);
    MemOperand &stMem = GetOrCreateMemOpndFromSymbol(*symbol, stmt.GetFieldID(), &regOpnd);
    SelectCopy(stMem, opndRhs, memType, rhsType);
  } else {
    SelectCopy(symbolMem, opndRhs, memType, rhsType);
  }
  return;
}

void MPISel::SelectDassignoff(const DassignoffNode &stmt, Operand &opnd0) {
  MIRSymbol *symbol = cgFunc->GetFunction().GetLocalOrGlobalSymbol(stmt.stIdx);
  PrimType primType = stmt.GetPrimType();
  uint32 bitSize = GetPrimTypeBitSize(primType);
  MemOperand &memOpnd = GetOrCreateMemOpndFromSymbol(*symbol, bitSize, stmt.offset);

  SelectCopy(memOpnd, opnd0, primType);
}

void MPISel::SelectIassign(const IassignNode &stmt, Operand &opndAddr, Operand &opndRhs) {
  /* mirSymbol info */
  MirTypeInfo symbolInfo = GetMirTypeInfoFromMirNode(stmt);
  /* handle Lhs, generate (%Rxx) via Rxx */
  PrimType memType = symbolInfo.primType;
  if (memType == PTY_agg) {
    memType = PTY_a64;
  }
  RegOperand &lhsBaseOpnd = SelectCopy2Reg(opndAddr, stmt.Opnd(0)->GetPrimType());
  MemOperand &lhsMemOpnd = cgFunc->GetOpndBuilder()->CreateMem(lhsBaseOpnd, symbolInfo.offset,
      GetPrimTypeBitSize(memType));
  /* handle Rhs, get R## from Rhs */
  PrimType rhsType = stmt.GetRHS()->GetPrimType();
  /* mov %R##, (%Rxx) */
  SelectCopy(lhsMemOpnd, opndRhs, memType, rhsType);
}

void MPISel::SelectIassignoff(const IassignoffNode &stmt) {
  Operand *addr = HandleExpr(stmt, *stmt.Opnd(0));
  ASSERT(addr != nullptr, "null ptr check");
  Operand *rhs = HandleExpr(stmt, *stmt.Opnd(1));
  ASSERT(rhs != nullptr, "null ptr check");

  int32 offset = stmt.GetOffset();
  PrimType primType = stmt.GetPrimType();
  uint32 bitSize = GetPrimTypeBitSize(primType);
  RegOperand &addrReg = SelectCopy2Reg(*addr, PTY_a64);
  RegOperand &rhsReg = SelectCopy2Reg(*rhs, primType, stmt.Opnd(1)->GetPrimType());

  MemOperand &memOpnd = cgFunc->GetOpndBuilder()->CreateMem(addrReg, offset, bitSize);
  SelectCopy(memOpnd, rhsReg, primType);
}

ImmOperand *MPISel::SelectIntConst(const MIRIntConst &intConst, PrimType primType) const {
  return &cgFunc->GetOpndBuilder()->CreateImm(GetPrimTypeBitSize(primType), intConst.GetExtValue());
}

Operand *MPISel::SelectShift(const BinaryNode &node, Operand &opnd0,
                             Operand &opnd1, const BaseNode &parent [[maybe_unused]]) {
  PrimType primType = node.GetPrimType();
  RegOperand *resOpnd = nullptr;
  Opcode opcode = node.GetOpCode();

  if (IsPrimitiveInteger(primType)) {
    resOpnd = &(cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType),
        cgFunc->GetRegTyFromPrimTy(primType)));
    RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, primType, node.Opnd(0)->GetPrimType());
    SelectShift(*resOpnd, regOpnd0, opnd1, opcode, primType, node.Opnd(1)->GetPrimType());
  } else {
    CHECK_FATAL(false, "NIY vector cvt");
  }
  return resOpnd;
}

void MPISel::SelectShift(Operand &resOpnd, Operand &opnd0, Operand &opnd1, Opcode shiftDirect,
    PrimType opnd0Type, PrimType opnd1Type) {
  PTY128MOD(opnd0Type);
  if (opnd1.IsIntImmediate() && static_cast<ImmOperand&>(opnd1).GetValue() == 0) {
    SelectCopy(resOpnd, opnd0, opnd0Type);
    return;
  }

  uint32 dsize = GetPrimTypeBitSize(opnd0Type);
  MOperator mOp = abstract::MOP_undef;
  if (shiftDirect == OP_shl) {
    const static auto fastShlMappingFunc = DEF_MOPERATOR_MAPPING_FUNC(shl);
    mOp = fastShlMappingFunc(dsize);
  } else if (shiftDirect == OP_ashr) {
    const static auto fastAshrMappingFunc = DEF_MOPERATOR_MAPPING_FUNC(ashr);
    mOp = fastAshrMappingFunc(dsize);
  } else if (shiftDirect == OP_lshr) {
    const static auto fastLshrMappingFunc = DEF_MOPERATOR_MAPPING_FUNC(lshr);
    mOp = fastLshrMappingFunc(dsize);
  } else {
    CHECK_FATAL(false, "NIY, Not support shiftdirect case");
  }
  RegOperand &firstOpnd = SelectCopy2Reg(opnd0, opnd0Type);
  RegOperand &secondOpnd = SelectCopy2Reg(opnd1, opnd1Type);
  Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, InsnDesc::GetAbstractId(mOp));
  insn.AddOpndChain(resOpnd).AddOpndChain(firstOpnd).AddOpndChain(secondOpnd);
  cgFunc->GetCurBB()->AppendInsn(insn);
}

void MPISel::SelectRegassign(RegassignNode &stmt, Operand &opnd0) {
  PrimType rhsType = stmt.Opnd(0)->GetPrimType();
  PTY128MOD(rhsType);
  PregIdx pregIdx = stmt.GetRegIdx();
  PrimType regType = stmt.GetPrimType();
  PTY128MOD(regType);
  RegOperand &regOpnd = cgFunc->GetOpndBuilder()->CreateVReg(cgFunc->GetVirtualRegNOFromPseudoRegIdx(pregIdx),
      GetPrimTypeBitSize(regType), cgFunc->GetRegTyFromPrimTy(regType));
  SelectCopy(regOpnd, opnd0, regType, rhsType);
  if ((Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevel0) && (pregIdx >= 0)) {
    const SymbolAlloc *symLoc = cgFunc->GetMemlayout()->GetSpillLocOfPseduoRegister(pregIdx);
    int64 offset = static_cast<int64>(cgFunc->GetBaseOffset(*symLoc));
    MIRPreg *preg = cgFunc->GetFunction().GetPregTab()->PregFromPregIdx(pregIdx);
    uint32 bitLen = GetPrimTypeSize(preg->GetPrimType()) * kBitsPerByte;
    RegOperand &base = GetTargetBasicPointer(PTY_u64);
    MemOperand *dest = &cgFunc->GetOpndBuilder()->CreateMem(base, offset, bitLen);
    SelectCopy(*dest, regOpnd, preg->GetPrimType(), regType);
  }
}

RegOperand *MPISel::SelectRegread(RegreadNode &expr) {
  PregIdx pregIdx = expr.GetRegIdx();
  PrimType rhsType = expr.GetPrimType();
  RegOperand &reg = cgFunc->GetOpndBuilder()->CreateVReg(cgFunc->GetVirtualRegNOFromPseudoRegIdx(pregIdx),
      GetPrimTypeSize(rhsType) * kBitsPerByte, cgFunc->GetRegTyFromPrimTy(rhsType));
  if (Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevel0) {
    const SymbolAlloc *symLoc = cgFunc->GetMemlayout()->GetSpillLocOfPseduoRegister(pregIdx);
    int64 offset = static_cast<int64>(cgFunc->GetBaseOffset(*symLoc));
    MIRPreg *preg = cgFunc->GetFunction().GetPregTab()->PregFromPregIdx(pregIdx);
    uint32 bitLen = GetPrimTypeSize(preg->GetPrimType()) * kBitsPerByte;
    RegOperand &base = GetTargetBasicPointer(PTY_u64);
    MemOperand *src = &cgFunc->GetOpndBuilder()->CreateMem(base, offset, bitLen);
    SelectCopy(reg, *src, rhsType, preg->GetPrimType());
  }
  return &reg;
}

Operand *MPISel::SelectDread(const BaseNode &parent [[maybe_unused]], const AddrofNode &expr) {
  /* get mirSymbol info*/
  MIRSymbol *symbol = cgFunc->GetFunction().GetLocalOrGlobalSymbol(expr.GetStIdx());
  MirTypeInfo symbolInfo = GetMirTypeInfoFromMirNode(expr);
  PrimType symbolType = symbolInfo.primType;
  /* Get symbol location */
  MemOperand &symbolMem = GetOrCreateMemOpndFromSymbol(*symbol, expr.GetFieldID());
  PrimType primType = expr.GetPrimType();
  /* for AggType, return it's location in stack. */
  if (symbolType == maple::PTY_agg) {
    CHECK_FATAL(primType == maple::PTY_agg, "NIY");
    return &symbolMem;
  }
  /* Generate Insn */
  if (IsSymbolRequireIndirection(*symbol)) {
    RegOperand &regOpnd = cgFunc->GetOpndBuilder()->CreateVReg(k64BitSize, kRegTyInt);
    SelectCopy(regOpnd, symbolMem, PTY_a64, PTY_a64);
    RegOperand &regOpnd1 = cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType),
        cgFunc->GetRegTyFromPrimTy(primType));
    MemOperand &ldMem = GetOrCreateMemOpndFromSymbol(*symbol, expr.GetFieldID(), &regOpnd);
    SelectCopy(regOpnd1, ldMem, primType, symbolType);
    return &regOpnd1;
  } else {
    /* for BasicType, load symbolVal to register. */
    RegOperand &regOpnd = cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType),
        cgFunc->GetRegTyFromPrimTy(primType));
    SelectCopy(regOpnd, symbolMem, primType, symbolType);
    return &regOpnd;
  }
}

Operand *MPISel::SelectAdd(const BinaryNode &node, Operand &opnd0,
                           Operand &opnd1, const BaseNode &parent [[maybe_unused]]) {
  PrimType primType = node.GetPrimType();
  PTY128MOD(primType);
  RegOperand &resReg = cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType),
      cgFunc->GetRegTyFromPrimTy(primType));
  RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, primType, node.Opnd(0)->GetPrimType());
  RegOperand &regOpnd1 = SelectCopy2Reg(opnd1, primType, node.Opnd(1)->GetPrimType());
  SelectAdd(resReg, regOpnd0, regOpnd1, primType);
  return &resReg;
}

Operand *MPISel::SelectBand(const BinaryNode &node, Operand &opnd0,
                            Operand &opnd1, const BaseNode &parent [[maybe_unused]]) {
  PrimType primType = node.GetPrimType();
  RegOperand &resReg = cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType),
      cgFunc->GetRegTyFromPrimTy(primType));
  RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, primType, node.Opnd(0)->GetPrimType());
  RegOperand &regOpnd1 = SelectCopy2Reg(opnd1, primType, node.Opnd(1)->GetPrimType());
  SelectBand(resReg, regOpnd0, regOpnd1, primType);
  return &resReg;
}

Operand *MPISel::SelectSub(const BinaryNode &node, Operand &opnd0, Operand &opnd1,
                           const BaseNode &parent [[maybe_unused]]) {
  PrimType primType = node.GetPrimType();
  RegOperand &resOpnd = cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType),
      cgFunc->GetRegTyFromPrimTy(primType));
  RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, primType, node.Opnd(0)->GetPrimType());
  RegOperand &regOpnd1 = SelectCopy2Reg(opnd1, primType, node.Opnd(1)->GetPrimType());
  SelectSub(resOpnd, regOpnd0, regOpnd1, primType);
  return &resOpnd;
}

void MPISel::SelectExtractbits(RegOperand &resOpnd, RegOperand &opnd0, uint8 bitOffset,
                               uint8 bitSize, PrimType primType) {
  uint32 primBitSize = GetPrimTypeBitSize(primType);
  bool isSigned = IsSignedInteger(primType);
  if (bitOffset == 0 && !isSigned) {
    /*
     * resOpnd = opnd0 & ((1 << bitSize) - 1)
     */
    ImmOperand &imm = cgFunc->GetOpndBuilder()->CreateImm(primBitSize,
        (static_cast<int64>(1) << bitSize) - 1);
    SelectBand(resOpnd, opnd0, imm, primType);
  } else {
    /*
     * tmpOpnd = opnd0 << (primBitSize - bitSize - bitOffset)
     * resOpnd = tmpOpnd >> (primBitSize - bitSize)
     * if signed : use sar; else use shr
     */
    RegOperand &tmpOpnd = cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType),
        cgFunc->GetRegTyFromPrimTy(primType));
    ImmOperand &imm1Opnd = cgFunc->GetOpndBuilder()->CreateImm(primBitSize,
        primBitSize - bitSize - bitOffset);
    SelectShift(tmpOpnd, opnd0, imm1Opnd, OP_shl, primType, primType);
    Opcode opcode = isSigned ? OP_ashr : OP_lshr;
    ImmOperand &imm2Opnd = cgFunc->GetOpndBuilder()->CreateImm(primBitSize, primBitSize - bitSize);
    SelectShift(resOpnd, tmpOpnd, imm2Opnd, opcode, primType, primType);
  }
}

Operand *MPISel::SelectExtractbits(const BaseNode &parent [[maybe_unused]],
                                   ExtractbitsNode &node, Operand &opnd0) {
  PrimType fromType = node.Opnd(0)->GetPrimType();
  PrimType toType = node.GetPrimType();
  uint8 bitSize = node.GetBitsSize();
  RegOperand &resOpnd = cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(toType),
      cgFunc->GetRegTyFromPrimTy(toType));
  if (IsPrimitiveInteger(toType)) {
    // OP_extractbits or bitSize < 8-bit or bitSize is not pow of 2
    if (node.GetOpCode() == OP_extractbits || bitSize < k8BitSize || (bitSize & (bitSize - 1)) != 0) {
      SelectCopy(resOpnd, opnd0, toType, fromType);
      SelectExtractbits(resOpnd, resOpnd, node.GetBitsOffset(), bitSize, toType);
    } else {
      PrimType opndType = GetIntegerPrimTypeFromSize(node.GetOpCode() == OP_sext, bitSize);
      RegOperand &tmpRegOpnd = SelectCopy2Reg(opnd0, opndType, fromType);
      SelectIntCvt(resOpnd, tmpRegOpnd, toType, opndType);
    }
  } else {
    CHECK_FATAL(false, "NIY vector cvt");
  }
  return &resOpnd;
}

Operand *MPISel::SelectCvt(const BaseNode &parent [[maybe_unused]], const TypeCvtNode &node, Operand &opnd0) {
  PrimType fromType = node.FromType();
  PTY128MOD(fromType);
  PrimType toType = node.GetPrimType();
  PTY128MOD(toType);
  if (fromType == toType) {
    return &opnd0;
  }
  RegOperand *regOpnd0;
  if (!opnd0.IsRegister()) {
    RegOperand &result = cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(fromType),
        cgFunc->GetRegTyFromPrimTy(fromType));
    SelectCopy(result, opnd0, fromType, fromType);
    regOpnd0 = &result;
  } else {
    regOpnd0 = &static_cast<RegOperand&>(opnd0);
  }
  RegOperand *resOpnd = &cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(toType),
      cgFunc->GetRegTyFromPrimTy(toType));
  if (IsPrimitiveInteger(toType) && IsPrimitiveInteger(fromType)) {
    SelectIntCvt(*resOpnd, *regOpnd0, toType, fromType);
  } else if (IsPrimitiveFloat(toType) && IsPrimitiveInteger(fromType)) {
    SelectCvtInt2Float(*resOpnd, *regOpnd0, toType, fromType);
  } else if (IsPrimitiveFloat(toType) && IsPrimitiveFloat(fromType)) {
    SelectFloatCvt(*resOpnd, *regOpnd0, toType, fromType);
  } else if (IsPrimitiveInteger(toType) && IsPrimitiveFloat(fromType)) {
    SelectCvtFloat2Int(*resOpnd, *regOpnd0, toType, fromType);
  } else {
    CHECK_FATAL(false, "NIY cvt");
  }
  return resOpnd;
}

void MPISel::SelectCvtFloat2Int(RegOperand &resOpnd, Operand &opnd0, PrimType toType, PrimType fromType) {
  uint32 toSize = GetPrimTypeBitSize(toType);
  bool isSigned = !IsPrimitiveUnsigned(toType);
#if TARGAARCH64
  /*
   *  Due to fp precision, should use one insn to perform cvt.
   */
  MOperator mOp = abstract::MOP_undef;
  switch (fromType) {
    case PTY_f64:
      mOp = (toSize <= k32BitSize) ? ((isSigned) ? MOP_cvt_i32_f64 : MOP_cvt_u32_f64) :
                                     ((isSigned) ? MOP_cvt_i64_f64 : MOP_cvt_u64_f64);
      break;
    case PTY_f32:
      mOp = (toSize <= k32BitSize) ? ((isSigned) ? MOP_cvt_i32_f32 : MOP_cvt_u32_f32) :
                                     ((isSigned) ? MOP_cvt_i64_f32 : MOP_cvt_u64_f32);
      break;
    default:
      CHECK_FATAL(false, "NYI");
  }
  Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, InsnDesc::GetAbstractId(mOp));
  (void)insn.AddOpndChain(resOpnd).AddOpndChain(opnd0);
  cgFunc->GetCurBB()->AppendInsn(insn);
#else
  PrimType newToType = toType;
  // cvt f64/32 -> u16 / u8 -> cvt f u32 + cvt u32 -> u8
  if (toSize < k32BitSize) {
    newToType = isSigned ? PTY_i32 : PTY_u32;
  }
  uint32 newToSize = GetPrimTypeBitSize(newToType);
  RegOperand &tmpFloatOpnd = cgFunc->GetOpndBuilder()->CreateVReg(newToSize, kRegTyFloat);
  SelectFloatCvt(tmpFloatOpnd, opnd0, newToType, fromType);
  MOperator mOp = abstract::MOP_undef;
  if (newToSize == k32BitSize) {
    mOp = isSigned ? abstract::MOP_cvt_i32_f32 : abstract::MOP_cvt_u32_f32;
  } else if (newToSize == k64BitSize) {
    mOp = isSigned ? abstract::MOP_cvt_i64_f64 : abstract::MOP_cvt_u64_f64;
  } else {
    CHECK_FATAL(false, "niy");
  }
  Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, InsnDesc::GetAbstractId(mOp));
  if (toSize == newToSize) {
    (void)insn.AddOpndChain(resOpnd).AddOpndChain(tmpFloatOpnd);
  } else if (toSize < newToSize) {
    RegOperand &tmpIntOpnd = cgFunc->GetOpndBuilder()->CreateVReg(newToSize, kRegTyFloat);
    (void)insn.AddOpndChain(tmpIntOpnd).AddOpndChain(tmpFloatOpnd);
    SelectIntCvt(resOpnd, tmpIntOpnd, toType, newToType);
  }
  cgFunc->GetCurBB()->AppendInsn(insn);
#endif
}

void MPISel::SelectCvtInt2Float(RegOperand &resOpnd, Operand &opnd0, PrimType toType, PrimType fromType) {
  uint32 fromSize = GetPrimTypeBitSize(fromType);
  bool isSigned = !IsPrimitiveUnsigned(fromType);
#if TARGAARCH64
  /* Due to fp precision, convert is done with one instruction */
  MOperator mOp = abstract::MOP_undef;
  switch (toType) {
    case PTY_f64:
      mOp = (fromSize <= k32BitSize) ? ((isSigned) ? MOP_cvt_f64_i32 : MOP_cvt_f64_u32) :
                                       ((isSigned) ? MOP_cvt_f64_i64 : MOP_cvt_f64_u64);
      break;
    case PTY_f32:
      mOp = (fromSize <= k32BitSize) ? ((isSigned) ? MOP_cvt_f32_i32 : MOP_cvt_f32_u32) :
                                       ((isSigned) ? MOP_cvt_f32_i64 : MOP_cvt_f32_u64);
      break;
    default:
      CHECK_FATAL(false, "NYI");
  }
  RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, fromType);
  Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, InsnDesc::GetAbstractId(mOp));
  (void)insn.AddOpndChain(resOpnd).AddOpndChain(regOpnd0);
  cgFunc->GetCurBB()->AppendInsn(insn);
#else
  MOperator mOp = abstract::MOP_undef;
  PrimType newFromType = PTY_begin;
  if (fromSize == k32BitSize) {
    mOp = isSigned ? abstract::MOP_cvt_f32_i32 : abstract::MOP_cvt_f32_u32;
    newFromType = PTY_f32;
  } else if (fromSize == k64BitSize) {
    mOp = isSigned ? abstract::MOP_cvt_f64_i64 : abstract::MOP_cvt_f64_u64;
    newFromType = PTY_f64;
  } else {
    CHECK_FATAL(false, "niy");
  }
  RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, fromType);
  RegOperand &tmpFloatOpnd = cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(newFromType),
      cgFunc->GetRegTyFromPrimTy(newFromType));
  Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, InsnDesc::GetAbstractId(mOp));
  (void)insn.AddOpndChain(tmpFloatOpnd).AddOpndChain(regOpnd0);
  cgFunc->GetCurBB()->AppendInsn(insn);
  SelectFloatCvt(resOpnd, tmpFloatOpnd, toType, newFromType);
#endif
}

void MPISel::SelectIntCvt(RegOperand &resOpnd, Operand &opnd0, PrimType toType, PrimType fromType) {
  uint32 fromSize = GetPrimTypeBitSize(fromType);
  uint32 toSize = GetPrimTypeBitSize(toType);
  /*
   * It is redundancy to insert "nop" casts (unsigned 32 -> singed 32) in abstract CG IR
   * The signedness of operands would be shown in the expression.
   */
  bool isSigned = !IsPrimitiveUnsigned(fromType);
  uint32 bitSize = opnd0.GetSize();
  PrimType opndType = GetIntegerPrimTypeFromSize(isSigned, bitSize);
  RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, fromType, opndType);
  if (toSize <= fromSize) {
    resOpnd = cgFunc->GetOpndBuilder()->CreateVReg(regOpnd0.GetRegisterNumber(),
        GetPrimTypeBitSize(toType), cgFunc->GetRegTyFromPrimTy(toType));
    return;
  }
  MOperator mOp = GetFastCvtMopI(fromSize, toSize, isSigned);
  Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, InsnDesc::GetAbstractId(mOp));
  (void)insn.AddOpndChain(resOpnd).AddOpndChain(regOpnd0);
  cgFunc->GetCurBB()->AppendInsn(insn);
  return;
}

void MPISel::SelectFloatCvt(RegOperand &resOpnd, Operand &opnd0, PrimType toType, PrimType fromType) {
  uint32 fromSize = GetPrimTypeBitSize(fromType);
  uint32 toSize = GetPrimTypeBitSize(toType);
  RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, fromType);
  if (fromSize == toSize) {
    resOpnd = regOpnd0;
    return;
  }
  MOperator mOp = abstract::MOP_undef;
  if (fromSize == k32BitSize && toSize == k64BitSize) {
    mOp = abstract::MOP_cvt_ff_64_32;
  } else if (fromSize == k64BitSize && toSize == k32BitSize) {
    mOp = abstract::MOP_cvt_ff_32_64;
  } else {
    CHECK_FATAL(false, "niy");
  }
  Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, InsnDesc::GetAbstractId(mOp));
  (void)insn.AddOpndChain(resOpnd).AddOpndChain(regOpnd0);
  cgFunc->GetCurBB()->AppendInsn(insn);
  return;
}

void MPISel::SelectSub(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) {
  MOperator mOp = abstract::MOP_undef;
  if (IsPrimitiveInteger(primType)) {
    const static auto fastSubMappingFunc = DEF_MOPERATOR_MAPPING_FUNC(sub);
    mOp = fastSubMappingFunc(GetPrimTypeBitSize(primType));
  } else {
    const static auto fastSubFloatMappingFunc = DEF_FLOAT_MOPERATOR_MAPPING_FUNC(sub);
    mOp = fastSubFloatMappingFunc(GetPrimTypeBitSize(primType));
  }
  SelectBasicOp(resOpnd, opnd0, opnd1, mOp, primType);
}

void MPISel::SelectBand(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) {
  MOperator mOp = abstract::MOP_undef;
  if (IsPrimitiveInteger(primType)) {
    const static auto fastAndMappingFunc = DEF_MOPERATOR_MAPPING_FUNC(and);
    mOp = fastAndMappingFunc(GetPrimTypeBitSize(primType));
  } else {
    const static auto fastAndFloatMappingFunc = DEF_FLOAT_MOPERATOR_MAPPING_FUNC(and);
    mOp = fastAndFloatMappingFunc(GetPrimTypeBitSize(primType));
  }
  SelectBasicOp(resOpnd, opnd0, opnd1, mOp, primType);
}

void MPISel::SelectAdd(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) {
  MOperator mOp = abstract::MOP_undef;
  if (IsPrimitiveInteger(primType)) {
    const static auto fastAddMappingFunc = DEF_MOPERATOR_MAPPING_FUNC(add);
    mOp = fastAddMappingFunc(GetPrimTypeBitSize(primType));
  } else {
    const static auto fastAddFloatMappingFunc = DEF_FLOAT_MOPERATOR_MAPPING_FUNC(add);
    mOp = fastAddFloatMappingFunc(GetPrimTypeBitSize(primType));
  }
  SelectBasicOp(resOpnd, opnd0, opnd1, mOp, primType);
}

Operand* MPISel::SelectNeg(const UnaryNode &node, Operand &opnd0, const BaseNode &parent [[maybe_unused]]) {
  PrimType dtype = node.GetPrimType();

  RegOperand *resOpnd = nullptr;
  if (!IsPrimitiveVector(dtype)) {
    resOpnd = &cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(dtype),
        cgFunc->GetRegTyFromPrimTy(dtype));
    RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, dtype, node.Opnd(0)->GetPrimType());
    SelectNeg(*resOpnd, regOpnd0, dtype);
  } else {
    /* vector operand */
    CHECK_FATAL(false, "NIY");
  }
  return resOpnd;
}

void MPISel::SelectNeg(Operand &resOpnd, Operand &opnd0, PrimType primType) const {
  MOperator mOp = abstract::MOP_undef;
  if (IsPrimitiveInteger(primType)) {
    const static auto fastNegMappingFunc = DEF_MOPERATOR_MAPPING_FUNC(neg);
    mOp = fastNegMappingFunc(GetPrimTypeBitSize(primType));
  } else {
    const static auto fastNegFloatMappingFunc = DEF_FLOAT_MOPERATOR_MAPPING_FUNC(neg);
    mOp = fastNegFloatMappingFunc(GetPrimTypeBitSize(primType));
  }
  Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, InsnDesc::GetAbstractId(mOp));
  (void)insn.AddOpndChain(resOpnd).AddOpndChain(opnd0);
  cgFunc->GetCurBB()->AppendInsn(insn);
}

Operand *MPISel::SelectBior(const BinaryNode &node, Operand &opnd0,
                            Operand &opnd1, const BaseNode &parent [[maybe_unused]]) {
  PrimType primType = node.GetPrimType();
  RegOperand *resOpnd = &cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType),
      cgFunc->GetRegTyFromPrimTy(primType));
  RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, primType, node.Opnd(0)->GetPrimType());
  RegOperand &regOpnd1 = SelectCopy2Reg(opnd1, primType, node.Opnd(1)->GetPrimType());
  SelectBior(*resOpnd, regOpnd0, regOpnd1, primType);
  return resOpnd;
}

void MPISel::SelectBior(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) {
  const static auto fastBiorMappingFunc = DEF_MOPERATOR_MAPPING_FUNC(or);
  MOperator mOp = fastBiorMappingFunc(GetPrimTypeBitSize(primType));
  SelectBasicOp(resOpnd, opnd0, opnd1, mOp, primType);
}

Operand *MPISel::SelectBxor(const BinaryNode &node, Operand &opnd0,
                            Operand &opnd1, const BaseNode &parent [[maybe_unused]]) {
  PrimType primType = node.GetPrimType();
  RegOperand *resOpnd = &cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType),
      cgFunc->GetRegTyFromPrimTy(primType));
  RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, primType, node.Opnd(0)->GetPrimType());
  RegOperand &regOpnd1 = SelectCopy2Reg(opnd1, primType, node.Opnd(1)->GetPrimType());
  SelectBxor(*resOpnd, regOpnd0, regOpnd1, primType);
  return resOpnd;
}

void MPISel::SelectBxor(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) {
  const static auto fastBxorMappingFunc = DEF_MOPERATOR_MAPPING_FUNC(xor);
  MOperator mOp = fastBxorMappingFunc(GetPrimTypeBitSize(primType));
  SelectBasicOp(resOpnd, opnd0, opnd1, mOp, primType);
}

MemOperand *MPISel::GetOrCreateMemOpndFromIreadNode(const IreadNode &expr, PrimType primType, int offset) {
  /* get rhs*/
  Operand *addrOpnd = HandleExpr(expr, *expr.Opnd(0));
  RegOperand &addrOnReg = SelectCopy2Reg(*addrOpnd, PTY_a64);
  /* Generate memOpnd */
  MemOperand &memOpnd = cgFunc->GetOpndBuilder()->CreateMem(addrOnReg,
      offset, GetPrimTypeBitSize(primType));
  return &memOpnd;
}

Operand *MPISel::SelectIread(const BaseNode &parent [[maybe_unused]], const IreadNode &expr, int extraOffset) {
  /* get lhs mirType info */
  MirTypeInfo lhsInfo = GetMirTypeInfoFromMirNode(expr);
  /* get memOpnd */
  MemOperand &memOpnd = *GetOrCreateMemOpndFromIreadNode(expr, lhsInfo.primType, lhsInfo.offset + extraOffset);
  /* for AggType, return addr it self. */
  if (lhsInfo.primType == PTY_agg) {
    return &memOpnd;
  }
  /* for BasicType, load val in addr to register. */
  PrimType primType = expr.GetPrimType();
  PTY128MOD(primType);
  RegOperand &result = cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType),
      cgFunc->GetRegTyFromPrimTy(primType));
  SelectCopy(result, memOpnd, primType, lhsInfo.primType);
  return &result;
}

Operand *MPISel::SelectIreadoff(const BaseNode &parent [[maybe_unused]], const IreadoffNode &ireadoff) {
  int32 offset = ireadoff.GetOffset();
  PrimType primType = ireadoff.GetPrimType();
  uint32 bitSize = GetPrimTypeBitSize(primType);

  Operand *addrOpnd = HandleExpr(ireadoff, *ireadoff.Opnd(0));
  RegOperand &addrOnReg = SelectCopy2Reg(*addrOpnd, PTY_a64);
  MemOperand &memOpnd = cgFunc->GetOpndBuilder()->CreateMem(addrOnReg, offset, bitSize);
  RegOperand &result = cgFunc->GetOpndBuilder()->CreateVReg(bitSize,
      cgFunc->GetRegTyFromPrimTy(primType));
  SelectCopy(result, memOpnd, primType);
  return &result;
}

static inline uint64 CreateDepositBitsImm1(uint32 primBitSize, uint8 bitOffset, uint8 bitSize) {
  /* $imm1 = 1(primBitSize - bitSize - bitOffset)0(bitSize)1(bitOffset) */
  uint64 val = UINT64_MAX;      // 0xFFFFFFFFFFFFFFFF
  if (bitSize + bitOffset >= primBitSize) {
    val = 0;
  } else {
    val <<= (bitSize + bitOffset);
  }
  val |= (static_cast<uint64>(1) << bitOffset) - 1;
  return val;
}

Operand *MPISel::SelectDepositBits(const DepositbitsNode &node, Operand &opnd0, Operand &opnd1,
                                   const BaseNode &parent [[maybe_unused]]) {
  uint8 bitOffset = node.GetBitsOffset();
  uint8 bitSize = node.GetBitsSize();
  PrimType primType = node.GetPrimType();
  uint32 primBitSize = GetPrimTypeBitSize(primType);
  ASSERT((primBitSize == k64BitSize) || (bitOffset < k32BitSize), "wrong bitSize");
  ASSERT(bitSize < k64BitSize, "wrong bitSize");

  RegOperand &resOpnd = cgFunc->GetOpndBuilder()->CreateVReg(primBitSize,
      cgFunc->GetRegTyFromPrimTy(primType));
  /*
   * resOpnd = (opnd0 and $imm1) or ((opnd1 << bitOffset) and (~$imm1));
   * $imm1 = 1(primBitSize - bitSize - bitOffset)0(bitSize)1(bitOffset)
   */
  uint64 imm1Val = CreateDepositBitsImm1(primBitSize, bitOffset, bitSize);
  ImmOperand &imm1Opnd = cgFunc->GetOpndBuilder()->CreateImm(primBitSize,
      static_cast<int64>(imm1Val));
  /* and */
  SelectBand(resOpnd, opnd0, imm1Opnd, primType);
  if (opnd1.IsIntImmediate()) {
    // opnd1 is immediate, imm2 = (opnd1.val << bitOffset) & (~$imm1)
    int64 imm2Val = static_cast<int64>((static_cast<uint64>(static_cast<ImmOperand&>(opnd1).GetValue()) <<
        bitOffset) & (~imm1Val));
    ImmOperand &imm2Opnd = cgFunc->GetOpndBuilder()->CreateImm(primBitSize, imm2Val);
    /* or */
    SelectBior(resOpnd, resOpnd, imm2Opnd, primType);
  } else {
    RegOperand &tmpOpnd = cgFunc->GetOpndBuilder()->CreateVReg(primBitSize,
        cgFunc->GetRegTyFromPrimTy(primType));
    SelectCopy(tmpOpnd, opnd1, primType, node.Opnd(1)->GetPrimType());
    /* shift -- (opnd1 << bitOffset) */
    ImmOperand &countOpnd = cgFunc->GetOpndBuilder()->CreateImm(primBitSize, bitOffset);
    SelectShift(tmpOpnd, tmpOpnd, countOpnd, OP_shl, primType, primType);
    /* and (~$imm1) */
    ImmOperand &nonImm1Opnd = cgFunc->GetOpndBuilder()->CreateImm(primBitSize, (~imm1Val));
    SelectBand(tmpOpnd, tmpOpnd, nonImm1Opnd, primType);
    /* or */
    SelectBior(resOpnd, resOpnd, tmpOpnd, primType);
  }
  return &resOpnd;
}

Operand *MPISel::SelectAlloca(UnaryNode &node, Operand &opnd0) {
  ASSERT(node.GetPrimType() == PTY_a64, "wrong type");
  PrimType srcType = node.Opnd(0)->GetPrimType();
  RegOperand &sizeOpnd = cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(PTY_u64),
      cgFunc->GetRegTyFromPrimTy(PTY_u64));
  SelectCopy(sizeOpnd, opnd0, PTY_u64, srcType);

  /* stack byte alignment */
  uint32 stackPtrAlignment = cgFunc->GetMemlayout()->GetStackPtrAlignment();
  RegOperand &aliOp = cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(PTY_u64),
      cgFunc->GetRegTyFromPrimTy(PTY_u64));
  SelectAdd(aliOp, sizeOpnd, cgFunc->GetOpndBuilder()->CreateImm(k64BitSize,
      stackPtrAlignment - 1), PTY_u64);
  ImmOperand &shiftOpnd = cgFunc->GetOpndBuilder()->CreateImm(k64BitSize,
      __builtin_ctz(stackPtrAlignment));
  SelectShift(aliOp, aliOp, shiftOpnd, OP_lshr, PTY_u64, PTY_u64);
  SelectShift(aliOp, aliOp, shiftOpnd, OP_shl, PTY_u64, PTY_u64);

  RegOperand &spOpnd = GetTargetStackPointer(PTY_u64);
  SelectSub(spOpnd, spOpnd, aliOp, PTY_u64);

  RegOperand &resOpnd = cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(PTY_u64),
      cgFunc->GetRegTyFromPrimTy(PTY_u64));
  uint32 argsToStkpassSize = cgFunc->GetMemlayout()->SizeOfArgsToStackPass();
  if (argsToStkpassSize > 0) {
    SelectAdd(resOpnd, spOpnd, cgFunc->GetOpndBuilder()->CreateImm(k64BitSize, argsToStkpassSize), PTY_u64);
  } else {
    SelectCopy(resOpnd, spOpnd, PTY_u64);
  }
  return &resOpnd;
}

Operand *MPISel::SelectCGArrayElemAdd(const BinaryNode &node) {
  BaseNode *opnd0 = node.Opnd(0);
  BaseNode *opnd1 = node.Opnd(1);
  ASSERT(opnd1->GetOpCode() == OP_constval, "NIY, opnd1->op should be OP_constval.");

  switch (opnd0->GetOpCode()) {
    case OP_regread: {
      return SelectRegread(static_cast<RegreadNode&>(*opnd0));
    }
    case OP_addrof: {
      Operand *addrOpnd = SelectAddrof(static_cast<AddrofNode&>(*opnd0), node);

      /* OP_constval */
      ConstvalNode *constvalNode = static_cast<ConstvalNode*>(opnd1);
      MIRConst *mirConst = constvalNode->GetConstVal();
      ASSERT(mirConst->GetKind() == kConstInt, "NIY");
      MIRIntConst *mirIntConst = static_cast<MIRIntConst*>(mirConst);
      Operand *immOpnd = SelectIntConst(*mirIntConst, constvalNode->GetPrimType());

      Operand &resOpnd = cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(PTY_a64),
          cgFunc->GetRegTyFromPrimTy(PTY_a64));
      SelectAdd(resOpnd, *addrOpnd, *immOpnd, node.GetPrimType());
      return &resOpnd;
    }
    default:
      CHECK_FATAL(false, "cannot handle opnd0.");
  }
}

StmtNode *MPISel::HandleFuncEntry() const {
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
RegOperand &MPISel::SelectCopy2Reg(Operand &src, PrimType toType, PrimType fromType) {
  uint32 fromSize = GetPrimTypeBitSize(fromType);
  uint32 toSize = GetPrimTypeBitSize(toType);
  bool isReg = src.IsRegister();
  uint32 srcRegSize = isReg ? src.GetSize() : 0;
  if ((isReg && fromSize == toSize) || (fromType == PTY_unknown && isReg && srcRegSize == toSize)) {
    return static_cast<RegOperand&>(src);
  }
  RegOperand &dest = cgFunc->GetOpndBuilder()->CreateVReg(toSize, cgFunc->GetRegTyFromPrimTy(toType));
  if (fromType == PTY_unknown || fromSize == toSize) {
    SelectCopy(dest, src, toType);
  } else if (fromSize != toSize) {
    SelectCopy(dest, src, toType, fromType);
  }
  return dest;
}
/* This function copy/load/store src to a dest, Once the src and dest types
 * are different, implicit conversion is executed here. */
void MPISel::SelectCopy(Operand &dest, Operand &src, PrimType toType, PrimType fromType) {
  PTY128MOD(fromType);
  PTY128MOD(toType);
  uint32 fromSize = GetPrimTypeBitSize(fromType);
  uint32 toSize = GetPrimTypeBitSize(toType);
  if (fromType != PTY_unknown && fromSize != toSize) {
    RegOperand &srcRegOpnd = SelectCopy2Reg(src, fromType);
    RegOperand &dstRegOpnd = cgFunc->GetOpndBuilder()->CreateVReg(toSize, cgFunc->GetRegTyFromPrimTy(toType));
    SelectIntCvt(dstRegOpnd, srcRegOpnd, toType, fromType);
    SelectCopy(dest, dstRegOpnd, toType);
  } else {
    if (dest.GetKind() == Operand::kOpdMem || src.GetKind() == Operand::kOpdMem) {
      if ((dest.GetKind() == Operand::kOpdMem && src.GetKind() == Operand::kOpdRegister) ||
          (dest.GetKind() == Operand::kOpdRegister && src.GetKind() == Operand::kOpdMem)) {
        SelectCopyInsn(dest, src, toType);
      } else if (src.GetKind() != Operand::kOpdRegister) {
        RegOperand &tempReg = cgFunc->GetOpndBuilder()->CreateVReg(toSize, cgFunc->GetRegTyFromPrimTy(toType));
        SelectCopyInsn(tempReg, src, toType);
        SelectCopyInsn(dest, tempReg, toType);
      } else {
        SelectCopyInsn(dest, src, toType);
      }
    } else if (dest.GetKind() == Operand::kOpdRegister) {
      SelectCopyInsn(dest, src, toType);
    }else {
      CHECK_FATAL(false, "NIY, CPU supports more than memory and registers");
    }
  }
}

void MPISel::SelectCopyInsn(Operand &dest, Operand &src, PrimType type) {
  MOperator mop = GetFastIselMop(dest.GetKind(), src.GetKind(), type);
  CHECK_FATAL(mop != abstract::MOP_undef, "get mop failed");
  Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mop, InsnDesc::GetAbstractId(mop));
  /* For Store, md defines [src, dest] */
  if (insn.IsStore()) {
    (void)insn.AddOpndChain(src).AddOpndChain(dest);
  } else {
    (void)insn.AddOpndChain(dest).AddOpndChain(src);
  }
  cgFunc->GetCurBB()->AppendInsn(insn);
}

Operand *MPISel::SelectBnot(const UnaryNode &node, Operand &opnd0, const BaseNode &parent [[maybe_unused]]) {
  PrimType dtype = node.GetPrimType();

  RegOperand *resOpnd = nullptr;
  if (!IsPrimitiveVector(dtype)) {
    resOpnd = &cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(dtype),
        cgFunc->GetRegTyFromPrimTy(dtype));
    RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, dtype, node.Opnd(0)->GetPrimType());
    SelectBnot(*resOpnd, regOpnd0, dtype);
  } else {
    /* vector operand */
    CHECK_FATAL(false, "NIY");
  }
  return resOpnd;
}

void MPISel::SelectBnot(Operand &resOpnd, Operand &opnd0, PrimType primType) const {
  const static auto fastBnotMappingFunc = DEF_MOPERATOR_MAPPING_FUNC(not);
  MOperator mOp = fastBnotMappingFunc(GetPrimTypeBitSize(primType));
  Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, InsnDesc::GetAbstractId(mOp));
  (void)insn.AddOpndChain(resOpnd).AddOpndChain(opnd0);
  cgFunc->GetCurBB()->AppendInsn(insn);
}

Operand *MPISel::SelectMin(const BinaryNode &node, Operand &opnd0, Operand &opnd1) {
  PrimType primType = node.GetPrimType();
  RegOperand &resOpnd = cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType),
      cgFunc->GetRegTyFromPrimTy(primType));
  SelectMin(resOpnd, opnd0, opnd1, primType);
  return &resOpnd;
}

void MPISel::SelectMin(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) {
  SelectMinOrMax(true, resOpnd, opnd0, opnd1, primType);
}


Operand *MPISel::SelectMax(const BinaryNode &node, Operand &opnd0, Operand &opnd1) {
  PrimType primType = node.GetPrimType();
  RegOperand &resOpnd = cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType),
      cgFunc->GetRegTyFromPrimTy(primType));
  SelectMax(resOpnd, opnd0, opnd1, primType);
  return &resOpnd;
}

void MPISel::SelectMax(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) {
  SelectMinOrMax(false, resOpnd, opnd0, opnd1, primType);
}

Operand *MPISel::SelectRetype(const TypeCvtNode &node, Operand &opnd0) {
  PrimType fromType = node.Opnd(0)->GetPrimType();
  PrimType toType = node.GetPrimType();
  ASSERT(GetPrimTypeSize(fromType) == GetPrimTypeSize(toType), "retype bit widith doesn' match");
  if (IsPrimitivePoint(fromType) && IsPrimitivePoint(toType)) {
    return &SelectCopy2Reg(opnd0, toType);
  }
  if (IsPrimitiveVector(fromType) || IsPrimitiveVector(toType)) {
    return &SelectCopy2Reg(opnd0, toType);
  }
  if (IsPrimitiveInteger(fromType) && IsPrimitiveInteger(toType)) {
    return &SelectCopy2Reg(opnd0, toType, fromType);
  }
  if (IsPrimitiveInteger(fromType) && IsPrimitiveFloat(toType)) {
    RegOperand *resOpnd = &cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(toType),
      cgFunc->GetRegTyFromPrimTy(toType));
    SelectCvtInt2Float(*resOpnd, opnd0, toType, fromType);
    return resOpnd;
  }
  if (IsPrimitiveFloat(fromType) && IsPrimitiveInteger(toType)) {
    RegOperand *resOpnd = &cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(toType),
      cgFunc->GetRegTyFromPrimTy(toType));
    SelectCvtFloat2Int(*resOpnd, opnd0, toType, fromType);
    return resOpnd;
  }
  CHECK_FATAL(false, "NIY, retype");
  return nullptr;
}

void InstructionSelector::GetAnalysisDependence(AnalysisDep &aDep) const {
  aDep.AddRequired<AbstractBuilder>();
  aDep.AddPreserved<AbstractBuilder>();
}

bool InstructionSelector::PhaseRun(maplebe::CGFunc &f) {
  AbstractIRBuilder *aIRBuilder = GET_ANALYSIS(AbstractBuilder, f);
  MPISel *mpIS = f.GetCG()->CreateMPIsel(*GetPhaseMemPool(), *aIRBuilder, f);
  mpIS->DoMPIS();
  return true;
}

bool AbstractBuilder::PhaseRun(maplebe::CGFunc &f) {
  (void)f;
  aIRBuilder = GetPhaseMemPool()->New<AbstractIRBuilder>(*GetPhaseMemPool());
  return false;
}
}
