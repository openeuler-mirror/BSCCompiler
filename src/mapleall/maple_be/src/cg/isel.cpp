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

namespace maplebe {
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

DEF_FAST_ISEL_MAPPING_INT(32)
DEF_FAST_ISEL_MAPPING_INT(64)
DEF_FAST_ISEL_MAPPING_FLOAT(32)
DEF_FAST_ISEL_MAPPING_FLOAT(64)


#define DEF_SEL_MAPPING_TBL(SIZE)                                     \
MOperator SelMapping##SIZE(bool isInt, uint32 x, uint32 y) {          \
  return isInt ? fastIselMapI##SIZE[x][y] : fastIselMapF##SIZE[x][y]; \
}
#define USE_SELMAPPING_TBL(SIZE) \
{SIZE, SelMapping##SIZE}

DEF_SEL_MAPPING_TBL(32);
DEF_SEL_MAPPING_TBL(64);

std::map<uint32, std::function<MOperator (bool, uint32, uint32)>> fastIselMappingTable = {
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

void HandleLabel(StmtNode &stmt, MPISel &iSel) {
  CGFunc *cgFunc = iSel.GetCurFunc();
  ASSERT(stmt.GetOpCode() == OP_label, "error");
  auto &label = static_cast<LabelNode&>(stmt);
  BB *newBB = cgFunc->StartNewBBImpl(false, label);
  newBB->AddLabel(label.GetLabelIdx());
  cgFunc->SetLab2BBMap(newBB->GetLabIdx(), *newBB);
  cgFunc->SetCurBB(*newBB);
}

void HandleReturn(StmtNode &stmt, MPISel &iSel) {
  CGFunc *cgFunc = iSel.GetCurFunc();
  auto &retNode = static_cast<NaryStmtNode&>(stmt);
  ASSERT(retNode.NumOpnds() <= 1, "NYI return nodes number > 1");
  Operand *opnd = nullptr;
  if (retNode.NumOpnds() != 0) {
    opnd = iSel.HandleExpr(retNode, *retNode.Opnd(0));
  }
  iSel.SelectReturn(*opnd);
  cgFunc->SetCurBBKind(BB::kBBReturn);
  cgFunc->SetCurBB(*cgFunc->StartNewBB(retNode));
}

Operand *HandleDread(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  auto &dreadNode = static_cast<AddrofNode&>(expr);
  return iSel.SelectDread(parent, dreadNode);
}

Operand *HandleAdd(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  return iSel.SelectAdd(static_cast<BinaryNode&>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
                        *iSel.HandleExpr(expr, *expr.Opnd(1)), parent);
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

using HandleStmtFactory = FunctionFactory<Opcode, void, StmtNode&, MPISel&>;
using HandleExprFactory = FunctionFactory<Opcode, maplebe::Operand*, const BaseNode&, BaseNode&, MPISel&>;
namespace isel {
void InitHandleStmtFactory() {
  RegisterFactoryFunction<HandleStmtFactory>(OP_label, HandleLabel);
  RegisterFactoryFunction<HandleStmtFactory>(OP_dassign, HandleDassign);
  RegisterFactoryFunction<HandleStmtFactory>(OP_return, HandleReturn);
}
void InitHandleExprFactory() {
  RegisterFactoryFunction<HandleExprFactory>(OP_dread, HandleDread);
  RegisterFactoryFunction<HandleExprFactory>(OP_add, HandleAdd);
  RegisterFactoryFunction<HandleExprFactory>(OP_constval, HandleConstVal);
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

void MPISel::SelectDassign(DassignNode &stmt, Operand &opndRhs) {
  SelectDassign(stmt.GetStIdx(), stmt.GetFieldID(), stmt.GetRHS()->GetPrimType(), opndRhs);
}

void MPISel::SelectDassign(StIdx stIdx, FieldID fieldId, PrimType rhsPType, Operand &opndRhs) {
  MIRSymbol *symbol = cgFunc->GetFunction().GetLocalOrGlobalSymbol(stIdx);
  if (fieldId != 0) {
    CHECK_FATAL(false, "NIY");
  }
  /* handle Rhs of dassign */

  /* Get symbol location */
  CGMemOperand &symbolMem = GetSymbolFromMemory(*symbol);
  /* Generate Insn */
  SelectCopy(symbolMem, opndRhs, rhsPType);
}

CGImmOperand *MPISel::SelectIntConst(MIRIntConst &intConst) {
  uint32 opndSz = GetPrimTypeSize(intConst.GetType().GetPrimType()) * kBitsPerByte;
  return &cgFunc->GetOpndBuilder()->CreateImm(opndSz, intConst.GetValue());
}

Operand* MPISel::SelectDread(const BaseNode &parent, AddrofNode &expr) {
  MIRSymbol *symbol = cgFunc->GetFunction().GetLocalOrGlobalSymbol(expr.GetStIdx());
  PrimType symType = symbol->GetType()->GetPrimType();
  uint32 dataSize = GetPrimTypeBitSize(symType);
   /* Get symbol location */
  CGMemOperand &symbolMem = GetSymbolFromMemory(*symbol);
  CGRegOperand &regOpnd = cgFunc->GetOpndBuilder()->CreateVReg(dataSize,
      cgFunc->GetRegTyFromPrimTy(symType));
  SelectCopy(regOpnd, symbolMem, symType);
  return &regOpnd;
}

Operand* MPISel::SelectAdd(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) {
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

void MPISel::SelectAdd(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) {
  MOperator mOp = abstract::MOP_add_32;
  Operand::OperandType opnd0Type = opnd0.GetKind();
  Operand::OperandType opnd1Type = opnd1.GetKind();

  if (opnd0Type != Operand::kOpdRegister) {
    /* add #imm, #imm */
    if (opnd1Type != Operand::kOpdRegister) {
      CGRegOperand opnd0Reg = cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType),
          cgFunc->GetRegTyFromPrimTy(primType));
      SelectCopy(opnd0Reg, opnd0,primType);
      SelectAdd(resOpnd,opnd0Reg, opnd1, primType);
      return;
    }
    /* add #imm, reg */
    SelectAdd(resOpnd, opnd1, opnd0, primType);  /* commutative */
    return;
  }

  /* add reg, reg */
  if (opnd1Type == Operand::kOpdRegister) {
    ASSERT(IsPrimitiveFloat(primType) || IsPrimitiveInteger(primType), "NYI add");
  } else if(opnd1Type == Operand::kOpdImmediate) {
    // add reg, imm
  }
  Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, InsnDescription::GetAbstractId(mOp));
  insn.AddOperandChain(resOpnd).AddOperandChain(opnd0).AddOperandChain(opnd1);
  cgFunc->GetCurBB()->AppendInsn(insn);
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
  bool withFreqInfo = mirFunc.HasFreqMap() && !mirFunc.GetFreqMap().empty();
  if (withFreqInfo) {
    cgFunc->GetCurBB()->SetFrequency(kFreqBase);
  }

  return stmt;
}

void MPISel::SelectCopy(Operand &dest, Operand &src, PrimType type) {
  if (dest.GetKind() == Operand::kOpdRegister) {
    SelectCopy(static_cast<CGRegOperand&>(dest), src, type);
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
  } else {
    CHECK_FATAL(false, "NIY, CPU supports more than memory and registers");
  }
}

void MPISel::SelectCopy(CGRegOperand &regDest, Operand &src, PrimType type) {
  if (src.GetKind() == Operand::kOpdImmediate) {
    SelectCopyInsn<CGRegOperand, CGImmOperand>(regDest, static_cast<CGImmOperand&>(src), type);
  } else if (src.GetKind() == Operand::kOpdMem) {
    SelectCopyInsn<CGRegOperand, CGMemOperand>(regDest, static_cast<CGMemOperand&>(src), type);
  } else {
    CHECK_FATAL(false, "NIY");
  }
}

template<typename destTy, typename srcTy>
void MPISel::SelectCopyInsn(destTy &dest, srcTy &src, PrimType type) {
  MOperator mop = GetFastIselMop(dest.GetKind(), src.GetKind(), type);
  CHECK_FATAL(mop != abstract::MOP_undef, "get mop failed");
  Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mop, InsnDescription::GetAbstractId(mop));
  if (dest.GetSize() != src.GetSize()) {
    CHECK_FATAL(false, "NIY");
  }
  if (insn.IsStore()) { /* common usage : commute for store */
    insn.AddOperandChain(src).AddOperandChain(dest);
  } else {
    insn.AddOperandChain(dest).AddOperandChain(src);
  }
  cgFunc->GetCurBB()->AppendInsn(insn);
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
