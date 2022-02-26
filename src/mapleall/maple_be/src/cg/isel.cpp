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

namespace maplebe {
void HandleDassign(StmtNode &stmt, MPISel &iSel) {
  auto &dassignNode = static_cast<DassignNode&>(stmt);
  ASSERT(dassignNode.GetOpCode() == OP_dassign, "expect dassign");
  BaseNode *rhs = dassignNode.GetRHS();
  ASSERT(rhs != nullptr, "get rhs of dassignNode failed");
  CGOperand* opndRhs = iSel.HandleExpr(dassignNode, *rhs);
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

}

CGOperand *HandleDread(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  auto &dreadNode = static_cast<AddrofNode&>(expr);
  return iSel.SelectDread(parent, dreadNode);
}

CGOperand *HandleAdd(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
  return iSel.SelectAdd(static_cast<BinaryNode&>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
                        *iSel.HandleExpr(expr, *expr.Opnd(1)), parent);
}

CGOperand *HandleConstVal(const BaseNode &parent, BaseNode &expr, MPISel &iSel) {
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
using HandleExprFactory = FunctionFactory<Opcode, maplebe::CGOperand*, const BaseNode&, BaseNode&, MPISel&>;
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

CGOperand *MPISel::HandleExpr(const BaseNode &parent, BaseNode &expr) {
  auto function = CreateProductFunction<HandleExprFactory>(expr.GetOpCode());
  CHECK_FATAL(function != nullptr, "unsupported opCode in HandleExpr()");
  return function(parent, expr, *this);
}

void MPISel::doMPIS() {
  isel::InitHandleStmtFactory();
  isel::InitHandleExprFactory();
  StmtNode *secondStmt = HandleFuncEntry();
  for (StmtNode *stmt = secondStmt; stmt != nullptr; stmt = stmt->GetNext()) {
   /* dassign %a_2_3 0 (constval i32 1)*/
    auto function = CreateProductFunction<HandleStmtFactory>(stmt->GetOpCode());
    CHECK_FATAL(function != nullptr, "unsupported opCode or has been lowered before");
    function(*stmt, *this);
  }
  HandleFuncExit();
}

void MPISel::SelectDassign(DassignNode &stmt, CGOperand &opndRhs) {
  SelectDassign(stmt.GetStIdx(), stmt.GetFieldID(), stmt.GetRHS()->GetPrimType(), opndRhs);
}

void MPISel::SelectDassign(StIdx stIdx, FieldID fieldId, PrimType rhsPType, CGOperand &opndRhs) {
  MIRSymbol *symbol = cgFunc->GetFunction().GetLocalOrGlobalSymbol(stIdx);
  if (fieldId != 0) {
    CHECK_FATAL(false, "NIY");
  }
  /* handle Rhs of dassign */

  /* Get symbol location */
  CGMemOperand &symbolMem = GetSymbolFromMemory(*symbol);
  /* Generate Insn */
  SelectCopy(symbolMem, opndRhs);
}

CGImmOperand *MPISel::SelectIntConst(MIRIntConst &intConst) {
  uint32 opndSz = GetPrimTypeSize(intConst.GetType().GetPrimType()) * kBitsPerByte;
  return &cgFunc->GetOpndBuilder()->CreateImm(opndSz, intConst.GetValue());
}

CGOperand* MPISel::SelectDread(const BaseNode &parent, AddrofNode &expr) {
  return nullptr;
}

CGOperand* MPISel::SelectAdd(BinaryNode &node, CGOperand &opnd0, CGOperand &opnd1, const BaseNode &parent) {
  return nullptr;
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

void MPISel::SelectCopy(CGOperand &dest, CGOperand &src) {
  if (dest.GetOpndKind() == CGOperand::kOpdRegister) {
    SelectCopy(static_cast<CGRegOperand&>(dest), src);
  } else if (dest.GetOpndKind() == CGOperand::kOpdMemory) {
    if (src.GetOpndKind() != CGOperand::kOpdRegister) {
      CGRegOperand &tempReg = cgFunc->GetOpndBuilder()->CreateVReg(src.GetSize());
      SelectCopy(tempReg, src);
      SelectCopyInsn<CGMemOperand, CGRegOperand>(static_cast<CGMemOperand&>(dest), tempReg);
    } else {
      SelectCopyInsn<CGMemOperand, CGRegOperand>(static_cast<CGMemOperand&>(dest), static_cast<CGRegOperand&>(src));
    }

  } else {
    CHECK_FATAL(false, "NIY, CPU supports more than memory and registers");
  }
}

void MPISel::SelectCopy(CGRegOperand &regDest, CGOperand &src) {
  if (src.GetOpndKind() == CGOperand::kOpdImmediate) {
    SelectCopyInsn<CGRegOperand, CGImmOperand>(regDest, static_cast<CGImmOperand&>(src));
  } else {
    CHECK_FATAL(false, "NIY");
  }
}

template<typename destTy, typename srcTy>
void MPISel::SelectCopyInsn(destTy &dest, srcTy &src) {
  MOperator mop = GetFastIselMop(dest.GetOpndKind(), src.GetOpndKind());
  CHECK_FATAL(mop != isel::kMOP_undef, "get mop failed");
  CGInsn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mop);
  if (dest.GetSize() != src.GetSize()) {
    CHECK_FATAL(false, "NIY");
  }
  insn.AddOperandChain(dest).AddOperandChain(src);
  cgFunc->GetCurBB()->AppendInsn(insn);
}

MOperator fastIselMap[CGOperand::OperandType::kOpdUndef][CGOperand::OperandType::kOpdUndef] = {
    /*register,         imm ,              memory,           cond */
    {isel::kMOP_copyrr, isel::kMOP_copyri, isel::kMOP_load,  isel::kMOP_undef},     /* reg    */
    {isel::kMOP_undef,  isel::kMOP_undef,  isel::kMOP_undef, isel::kMOP_undef},     /* imm    */
    {isel::kMOP_str ,   isel::kMOP_undef,  isel::kMOP_undef, isel::kMOP_undef},     /* memory */
    {isel::kMOP_undef,  isel::kMOP_undef,  isel::kMOP_undef, isel::kMOP_undef},     /* cond   */
};
MOperator MPISel::GetFastIselMop(CGOperand::OperandType dTy, CGOperand::OperandType sTy) {
  return fastIselMap[dTy][sTy];
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
  f.DumpCGIR(true);
  return true;
}
}
