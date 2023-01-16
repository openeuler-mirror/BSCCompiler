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

#include "x64_standardize.h"
#include "x64_isa.h"
#include "x64_cg.h"
#include "insn.h"

namespace maplebe {
#define DEFINE_MAPPING(ABSTRACT_IR, X64_MOP, ...) {ABSTRACT_IR, X64_MOP},
std::unordered_map<MOperator, X64MOP_t> x64AbstractMapping = {
#include "x64_abstract_mapping.def"
};

static inline X64MOP_t GetMopFromAbstraceIRMop(MOperator mOp) {
  auto iter = x64AbstractMapping.find(mOp);
  if (iter == x64AbstractMapping.end()) {
    CHECK_FATAL(false, "NIY mapping");
  }
  CHECK_FATAL(iter->second != x64::MOP_begin, "NIY mapping");
  return iter->second;
}

void X64Standardize::StdzMov(maplebe::Insn &insn) {
  X64MOP_t directlyMappingMop = GetMopFromAbstraceIRMop(insn.GetMachineOpcode());
  insn.SetMOP(X64CG::kMd[directlyMappingMop]);
  insn.CommuteOperands(kInsnFirstOpnd, kInsnSecondOpnd);
}

void X64Standardize::StdzStrLdr(Insn &insn) {
  /* abstract ir store is in same order with x86, so reverse twice */
  if (insn.IsStore()) {
    insn.CommuteOperands(kInsnFirstOpnd, kInsnSecondOpnd);
  }
  StdzMov(insn);
}

void X64Standardize::StdzBasicOp(Insn &insn) {
  X64MOP_t directlyMappingMop = GetMopFromAbstraceIRMop(insn.GetMachineOpcode());
  insn.SetMOP(X64CG::kMd[directlyMappingMop]);
  Operand &dest = insn.GetOperand(kInsnFirstOpnd);
  Operand &src2 = insn.GetOperand(kInsnThirdOpnd);
  insn.CleanAllOperand();
  insn.AddOpndChain(src2).AddOpndChain(dest);
}

void X64Standardize::StdzUnaryOp(Insn &insn) {
  MOperator mOp = insn.GetMachineOpcode();
  if (mOp == abstract::MOP_neg_f_32 || mOp == abstract::MOP_neg_f_64) {
    StdzFloatingNeg(insn);
    return;
  }
  X64MOP_t directlyMappingMop = GetMopFromAbstraceIRMop(insn.GetMachineOpcode());
  insn.SetMOP(X64CG::kMd[directlyMappingMop]);
  Operand &dest = insn.GetOperand(kInsnFirstOpnd);
  insn.CleanAllOperand();
  insn.AddOpndChain(dest);
}

void X64Standardize::StdzCvtOp(Insn &insn) {
  uint32 OpndDesSize = insn.GetDesc()->GetOpndDes(kInsnFirstOpnd)->GetSize();
  uint32 destSize = OpndDesSize;
  uint32 OpndSrcSize = insn.GetDesc()->GetOpndDes(kInsnSecondOpnd)->GetSize();
  uint32 srcSize = OpndSrcSize;
  switch (insn.GetMachineOpcode()) {
    case abstract::MOP_zext_rr_64_32:
      destSize = k32BitSize;
      break;
    case abstract::MOP_cvt_f32_u32:
      srcSize = k64BitSize;
      break;
    case abstract::MOP_cvt_u32_f32:
      destSize = k64BitSize;
      break;
    case abstract::MOP_zext_rr_8_16:
    case abstract::MOP_sext_rr_8_16:
    case abstract::MOP_zext_rr_8_32:
    case abstract::MOP_sext_rr_8_32:
    case abstract::MOP_zext_rr_16_32:
    case abstract::MOP_sext_rr_16_32:
    case abstract::MOP_zext_rr_8_64:
    case abstract::MOP_sext_rr_8_64:
    case abstract::MOP_zext_rr_16_64:
    case abstract::MOP_sext_rr_16_64:
    case abstract::MOP_sext_rr_32_64:
      /* reverse operands */
      destSize = OpndSrcSize;
      srcSize = OpndDesSize;
      break;
    case abstract::MOP_zext_rr_32_64:
      srcSize = k32BitSize;
      destSize = k32BitSize;
    default:
      break;
  }
  MOperator directlyMappingMop = GetMopFromAbstraceIRMop(insn.GetMachineOpcode());
  if (directlyMappingMop != abstract::MOP_undef) {
    insn.SetMOP(X64CG::kMd[directlyMappingMop]);
    Operand *opnd0 = &insn.GetOperand(kInsnSecondOpnd);
    RegOperand *src = static_cast<RegOperand*>(opnd0);
    if (srcSize != OpndSrcSize) {
      src = &GetCgFunc()->GetOpndBuilder()->CreateVReg(src->GetRegisterNumber(),
          srcSize, src->GetRegisterType());
    }
    Operand *opnd1 = &insn.GetOperand(kInsnFirstOpnd);
    RegOperand *dest = static_cast<RegOperand*>(opnd1);
    if (destSize != OpndDesSize) {
      dest = &GetCgFunc()->GetOpndBuilder()->CreateVReg(dest->GetRegisterNumber(),
          destSize, dest->GetRegisterType());
    }
    insn.CleanAllOperand();
    insn.AddOpndChain(*src).AddOpndChain(*dest);
  } else {
    CHECK_FATAL(false, "NIY mapping");
  }
}

/* x86 does not have floating point neg instruction
 * neg_f   operand0  operand1
 * ==>
 * movd    xmm0 R1
 * 64: movabsq 0x8000000000000000 R2
 *     xorq R2 R1
 * 32: xorl 0x80000000 R1
 * movd R1 xmm0
*/
void X64Standardize::StdzFloatingNeg(Insn &insn) {
  MOperator mOp = insn.GetMachineOpcode();
  uint32 bitSize = mOp == abstract::MOP_neg_f_32 ? k32BitSize : k64BitSize;

  // mov dest -> tmpOperand0
  MOperator movOp = mOp == abstract::MOP_neg_f_32 ? x64::MOP_movd_fr_r : x64::MOP_movq_fr_r;
  RegOperand *tmpOperand0 = &GetCgFunc()->GetOpndBuilder()->CreateVReg(bitSize, kRegTyInt);
  Insn &movInsn0 = GetCgFunc()->GetInsnBuilder()->BuildInsn(movOp, X64CG::kMd[movOp]);
  Operand &dest = insn.GetOperand(kInsnFirstOpnd);
  movInsn0.AddOpndChain(dest).AddOpndChain(*tmpOperand0);
  insn.GetBB()->InsertInsnBefore(insn, movInsn0);

  // 32 : xorl   0x80000000         tmpOperand0
  // 64 : movabs 0x8000000000000000 tmpOperand1
  //      xorq   tmpOperand1        tmpOperand0
  ImmOperand &imm = GetCgFunc()->GetOpndBuilder()->CreateImm(bitSize, (static_cast<int64>(1) << (bitSize - 1)));
  if (mOp == abstract::MOP_neg_f_64) {
    Operand *tmpOperand1 = &GetCgFunc()->GetOpndBuilder()->CreateVReg(k64BitSize, kRegTyInt);
    Insn &movabs = GetCgFunc()->GetInsnBuilder()->BuildInsn(x64::MOP_movabs_i_r, X64CG::kMd[x64::MOP_movabs_i_r]);
    movabs.AddOpndChain(imm).AddOpndChain(*tmpOperand1);
    insn.GetBB()->InsertInsnBefore(insn, movabs);

    MOperator xorOp = x64::MOP_xorq_r_r;
    Insn &xorq = GetCgFunc()->GetInsnBuilder()->BuildInsn(xorOp, X64CG::kMd[xorOp]);
    xorq.AddOpndChain(*tmpOperand1).AddOpndChain(*tmpOperand0);
    insn.GetBB()->InsertInsnBefore(insn, xorq);
  } else {
    MOperator xorOp = x64::MOP_xorl_i_r;
    Insn &xorq = GetCgFunc()->GetInsnBuilder()->BuildInsn(xorOp, X64CG::kMd[xorOp]);
    xorq.AddOpndChain(imm).AddOpndChain(*tmpOperand0);
    insn.GetBB()->InsertInsnBefore(insn, xorq);
  }

  // mov tmpOperand0 -> dest
  Insn &movq = GetCgFunc()->GetInsnBuilder()->BuildInsn(movOp, X64CG::kMd[movOp]);
  movq.AddOpndChain(*tmpOperand0).AddOpndChain(dest);
  insn.GetBB()->InsertInsnBefore(insn, movq);

  insn.GetBB()->RemoveInsn(insn);
  return;
}

void X64Standardize::StdzShiftOp(Insn &insn) {
  RegOperand *countOpnd = static_cast<RegOperand*>(&insn.GetOperand(kInsnThirdOpnd));
  /* count operand cvt -> PTY_u8 */
  if (countOpnd->GetSize() != GetPrimTypeBitSize(PTY_u8)) {
    countOpnd = &GetCgFunc()->GetOpndBuilder()->CreateVReg(countOpnd->GetRegisterNumber(),
        GetPrimTypeBitSize(PTY_u8), countOpnd->GetRegisterType());
  }
  /* copy count operand to cl(rcx) register */
  RegOperand &clOpnd = GetCgFunc()->GetOpndBuilder()->CreatePReg(x64::RCX, GetPrimTypeBitSize(PTY_u8), kRegTyInt);
  X64MOP_t copyMop = x64::MOP_movb_r_r;
  Insn &copyInsn = GetCgFunc()->GetInsnBuilder()->BuildInsn(copyMop, X64CG::kMd[copyMop]);
  copyInsn.AddOpndChain(*countOpnd).AddOpndChain(clOpnd);
  insn.GetBB()->InsertInsnBefore(insn, copyInsn);
  /* shift OP */
  X64MOP_t directlyMappingMop = GetMopFromAbstraceIRMop(insn.GetMachineOpcode());
  insn.SetMOP(X64CG::kMd[directlyMappingMop]);
  RegOperand &destOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  insn.CleanAllOperand();
  insn.AddOpndChain(clOpnd).AddOpndChain(destOpnd);
}

void X64Standardize::StdzCommentOp(Insn &insn) {
  insn.GetBB()->RemoveInsn(insn);
}

}
