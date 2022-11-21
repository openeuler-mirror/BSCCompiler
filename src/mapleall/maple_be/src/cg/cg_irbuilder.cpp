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

#include "cg_irbuilder.h"
#include "isa.h"
#include "cg.h"
#include "cfi.h"
#include "dbg.h"

namespace maplebe {
Insn &InsnBuilder::BuildInsn(MOperator opCode, const InsnDesc &idesc) {
  auto *newInsn = mp->New<Insn>(*mp, opCode);
  newInsn->SetInsnDescrption(idesc);
  IncreaseInsnNum();
  return *newInsn;
}

Insn &InsnBuilder::BuildInsn(MOperator opCode, Operand &o0) {
  const InsnDesc &tMd = Globals::GetInstance()->GetTarget()->GetTargetMd(opCode);
  return BuildInsn(opCode, tMd).AddOpndChain(o0);
}
Insn &InsnBuilder::BuildInsn(MOperator opCode, Operand &o0, Operand &o1) {
  const InsnDesc &tMd = Globals::GetInstance()->GetTarget()->GetTargetMd(opCode);
  return BuildInsn(opCode, tMd).AddOpndChain(o0).AddOpndChain(o1);
}
Insn &InsnBuilder::BuildInsn(MOperator opCode, Operand &o0, Operand &o1, Operand &o2) {
  const InsnDesc &tMd = Globals::GetInstance()->GetTarget()->GetTargetMd(opCode);
  return BuildInsn(opCode, tMd).AddOpndChain(o0).AddOpndChain(o1).AddOpndChain(o2);
}

Insn &InsnBuilder::BuildInsn(MOperator opCode, Operand &o0, Operand &o1, Operand &o2, Operand &o3) {
  const InsnDesc &tMd = Globals::GetInstance()->GetTarget()->GetTargetMd(opCode);
  return BuildInsn(opCode, tMd).AddOpndChain(o0).AddOpndChain(o1).AddOpndChain(o2).AddOpndChain(o3);
}

Insn &InsnBuilder::BuildInsn(MOperator opCode, Operand &o0, Operand &o1, Operand &o2, Operand &o3, Operand &o4) {
  const InsnDesc &tMd = Globals::GetInstance()->GetTarget()->GetTargetMd(opCode);
  Insn &nI = BuildInsn(opCode, tMd);
  return nI.AddOpndChain(o0).AddOpndChain(o1).AddOpndChain(o2).AddOpndChain(o3).AddOpndChain(o4);
}

Insn &InsnBuilder::BuildInsn(MOperator opCode, std::vector<Operand*> &opnds) {
  const InsnDesc &tMd = Globals::GetInstance()->GetTarget()->GetTargetMd(opCode);
  Insn &nI = BuildInsn(opCode, tMd);
  for (auto *opnd : opnds) {
    nI.AddOperand(*opnd);
  }
  return nI;
}

Insn &InsnBuilder::BuildCfiInsn(MOperator opCode) {
  auto *nI = mp->New<cfi::CfiInsn>(*mp, opCode);
  IncreaseInsnNum();
  return *nI;
}
Insn &InsnBuilder::BuildDbgInsn(MOperator opCode) {
  auto *nI = mp->New<mpldbg::DbgInsn>(*mp, opCode);
  IncreaseInsnNum();
  return *nI;
}
Insn &InsnBuilder::BuildCommentInsn(CommentOperand &comment) {
  Insn &insn = BuildInsn(abstract::MOP_comment, InsnDesc::GetAbstractId(abstract::MOP_comment));
  insn.AddOperand(comment);
  return insn;
}
VectorInsn &InsnBuilder::BuildVectorInsn(MOperator opCode, const InsnDesc &idesc) {
  auto *newInsn = mp->New<VectorInsn>(*mp, opCode);
  newInsn->SetInsnDescrption(idesc);
  IncreaseInsnNum();
  return *newInsn;
}

ImmOperand &OperandBuilder::CreateImm(uint32 size, int64 value, MemPool *mp) {
  return mp ? *mp->New<ImmOperand>(value, size, false) : *alloc.New<ImmOperand>(value, size, false);
}

ImmOperand &OperandBuilder::CreateImm(const MIRSymbol &symbol, int64 offset, int32 relocs, MemPool *mp) {
  return mp ? *mp->New<ImmOperand>(symbol, offset, relocs, false) :
      *alloc.New<ImmOperand>(symbol, offset, relocs, false);
}

MemOperand &OperandBuilder::CreateMem(uint32 size, MemPool *mp) {
  return mp ? *mp->New<MemOperand>(size) : *alloc.New<MemOperand>(size);
}

MemOperand &OperandBuilder::CreateMem(RegOperand &baseOpnd, int64 offset, uint32 size, MemPool *mp) {
  ImmOperand &ofstOperand = CreateImm(baseOpnd.GetSize(), offset);
  if (mp != nullptr) {
    return *mp->New<MemOperand>(size, baseOpnd, ofstOperand);
  }
  return *alloc.New<MemOperand>(size, baseOpnd, ofstOperand);
}

MemOperand &OperandBuilder::CreateMem(uint32 size, RegOperand &baseOpnd, ImmOperand &ofstOperand, MemPool *mp) {
  if (mp != nullptr) {
    return *mp->New<MemOperand>(size, baseOpnd, ofstOperand);
  }
  return *alloc.New<MemOperand>(size, baseOpnd, ofstOperand);
}

MemOperand &OperandBuilder::CreateMem(uint32 size, RegOperand &baseOpnd, ImmOperand &ofstOperand,
                                      const MIRSymbol &symbol, MemPool *mp) {
  if (mp != nullptr) {
    return *mp->New<MemOperand>(size, baseOpnd, ofstOperand, symbol);
  }
  return *alloc.New<MemOperand>(size, baseOpnd, ofstOperand, symbol);
}

RegOperand &OperandBuilder::CreateVReg(uint32 size, RegType type, MemPool *mp) {
  virtualRegNum++;
  regno_t vRegNO = kBaseVirtualRegNO + virtualRegNum;
  return mp ? *mp->New<RegOperand>(vRegNO, size, type) : *alloc.New<RegOperand>(vRegNO, size, type);
}

RegOperand &OperandBuilder::CreateVReg(regno_t vRegNO, uint32 size, RegType type, MemPool *mp) {
  return mp ? *mp->New<RegOperand>(vRegNO, size, type) : *alloc.New<RegOperand>(vRegNO, size, type);
}

RegOperand &OperandBuilder::CreatePReg(regno_t pRegNO, uint32 size, RegType type, MemPool *mp) {
  return mp ? *mp->New<RegOperand>(pRegNO, size, type) : *alloc.New<RegOperand>(pRegNO, size, type);
}

ListOperand &OperandBuilder::CreateList(MemPool *mp) {
  return mp ? *mp->New<ListOperand>(alloc) : *alloc.New<ListOperand>(alloc);
}

FuncNameOperand &OperandBuilder::CreateFuncNameOpnd(MIRSymbol &symbol, MemPool *mp){
  return mp ? *mp->New<FuncNameOperand>(symbol) : *alloc.New<FuncNameOperand>(symbol);
}

LabelOperand &OperandBuilder::CreateLabel(const char *parent, LabelIdx idx, MemPool *mp){
  return mp ? *mp->New<LabelOperand>(parent, idx, *mp) : *alloc.New<LabelOperand>(parent, idx, *alloc.GetMemPool());
}

CommentOperand &OperandBuilder::CreateComment(const std::string &s, MemPool *mp) {
  return mp ? *mp->New<CommentOperand>(s, *mp) : *alloc.New<CommentOperand>(s, *alloc.GetMemPool());
}

CommentOperand &OperandBuilder::CreateComment(const MapleString &s, MemPool *mp) {
  return mp ? *mp->New<CommentOperand>(s.c_str(), *mp) : *alloc.New<CommentOperand>(s.c_str(), *alloc.GetMemPool());
}
}
