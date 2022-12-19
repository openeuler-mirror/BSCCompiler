/*
 * Copyright (c) [2022] Futurewei Technologies, Inc. All rights reserved.
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

#include "aarch64_standardize.h"
#include "aarch64_isa.h"
#include "aarch64_cg.h"
#include "insn.h"

namespace maplebe {

using namespace abstract;
static AbstractIR2Target abstract2TargetTable[kMopLast] {
  {MOP_undef, {{MOP_pseudo_none, {}, {}}}},

  {MOP_copy_ri_8, {{MOP_wmovri32, {kAbtractReg, kAbtractImm}, {0, 1}}}},
  {MOP_copy_rr_8, {{MOP_wmovrr, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_copy_ri_16, {{MOP_wmovri32, {kAbtractReg, kAbtractImm}, {0, 1}}}},
  {MOP_copy_rr_16, {{MOP_wmovrr, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_copy_ri_32, {{MOP_wmovri32, {kAbtractReg, kAbtractImm}, {0, 1}}}},
  {MOP_copy_rr_32, {{MOP_wmovrr, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_copy_ri_64, {{MOP_xmovri64, {kAbtractReg, kAbtractImm}, {0, 1}}}},
  {MOP_copy_rr_64, {{MOP_xmovrr, {kAbtractReg, kAbtractReg}, {0, 1}}}},

  {MOP_copy_fi_8, {{MOP_xvmovsr, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_copy_ff_8, {{MOP_xvmovs, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_copy_fi_16, {{MOP_xvmovsr, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_copy_ff_16, {{MOP_xvmovs, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_copy_fi_32, {{MOP_xvmovsr, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_copy_ff_32, {{MOP_xvmovs, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_copy_fi_64, {{MOP_xvmovdr, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_copy_ff_64, {{MOP_xvmovd, {kAbtractReg, kAbtractReg}, {0, 1}}}},

  {MOP_zext_rr_16_8, {{MOP_xuxtb32, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_sext_rr_16_8, {{MOP_xsxtb32, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_zext_rr_32_8, {{MOP_xuxtb32, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_sext_rr_32_8, {{MOP_xsxtb32, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_zext_rr_32_16, {{MOP_xuxth32, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_sext_rr_32_16, {{MOP_xsxth32, {kAbtractReg, kAbtractReg}, {0, 1}}}},

  {MOP_zext_rr_64_8, {{MOP_xuxtb32, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_sext_rr_64_8, {{MOP_xsxtb64, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_zext_rr_64_16, {{MOP_xuxth32, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_sext_rr_64_16, {{MOP_xsxth64, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_zext_rr_64_32, {{MOP_xuxtw64, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_sext_rr_64_32, {{MOP_xsxtw64, {kAbtractReg, kAbtractReg}, {0, 1}}}},

  {MOP_zext_rr_8_16, {{MOP_xuxtb32, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_sext_rr_8_16, {{MOP_xsxtb32, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_zext_rr_8_32, {{MOP_xuxtb32, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_sext_rr_8_32, {{MOP_xsxtb32, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_zext_rr_16_32, {{MOP_xuxth32, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_sext_rr_16_32, {{MOP_xsxth32, {kAbtractReg, kAbtractReg}, {0, 1}}}},

  {MOP_zext_rr_8_64, {{MOP_xuxtb32, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_sext_rr_8_64, {{MOP_xsxtb64, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_zext_rr_16_64, {{MOP_xuxth32, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_sext_rr_16_64, {{MOP_xsxth64, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_zext_rr_32_64, {{MOP_xuxtw64, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_sext_rr_32_64, {{MOP_xsxtw64, {kAbtractReg, kAbtractReg}, {0, 1}}}},

  {MOP_cvt_f32_u32, {{MOP_vcvtufr, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_cvt_f64_u32, {{MOP_vcvtudr, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_cvt_f32_u64, {{MOP_xvcvtufr, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_cvt_f64_u64, {{MOP_xvcvtudr, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_cvt_f32_i32, {{MOP_vcvtfr, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_cvt_f64_i32, {{MOP_vcvtdr, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_cvt_f32_i64, {{MOP_xvcvtfr, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_cvt_f64_i64, {{MOP_xvcvtdr, {kAbtractReg, kAbtractReg}, {0, 1}}}},

  {MOP_cvt_u32_f32, {{MOP_vcvturf, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_cvt_u64_f32, {{MOP_xvcvturf, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_cvt_u32_f64, {{MOP_vcvturd, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_cvt_u64_f64, {{MOP_xvcvturd, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_cvt_i32_f32, {{MOP_vcvtrf, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_cvt_i64_f32, {{MOP_xvcvtrf, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_cvt_i32_f64, {{MOP_vcvtrd, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_cvt_i64_f64, {{MOP_xvcvtrd, {kAbtractReg, kAbtractReg}, {0, 1}}}},

  {MOP_cvt_ff_64_32, {{MOP_xvcvtdf, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_cvt_ff_32_64, {{MOP_xvcvtfd, {kAbtractReg, kAbtractReg}, {0, 1}}}},

  {MOP_str_8, {{MOP_wstrb, {kAbtractReg, kAbtractMem}, {0, 1}}}},
  {MOP_str_16, {{MOP_wstrh, {kAbtractReg, kAbtractMem}, {0, 1}}}},
  {MOP_str_32, {{MOP_wstr, {kAbtractReg, kAbtractMem}, {0, 1}}}},
  {MOP_str_64, {{MOP_xstr, {kAbtractReg, kAbtractMem}, {0, 1}}}},
  {MOP_load_8, {{MOP_wldrb, {kAbtractReg, kAbtractMem}, {0, 1}}}},
  {MOP_load_16, {{MOP_wldrh, {kAbtractReg, kAbtractMem}, {0, 1}}}},
  {MOP_load_32, {{MOP_wldr, {kAbtractReg, kAbtractMem}, {0, 1}}}},
  {MOP_load_64, {{MOP_xldr, {kAbtractReg, kAbtractMem}, {0, 1}}}},
  {MOP_str_f_8, {{AArch64MOP_t::MOP_undef, {kAbtractNone}, {}}}},
  {MOP_str_f_16, {{AArch64MOP_t::MOP_undef, {kAbtractNone}, {}}}},
  {MOP_str_f_32, {{MOP_sstr, {kAbtractReg, kAbtractMem}, {0, 1}}}},
  {MOP_str_f_64, {{MOP_dstr, {kAbtractReg, kAbtractMem}, {0, 1}}}},
  {MOP_load_f_8, {{AArch64MOP_t::MOP_undef, {kAbtractNone}, {}}}},
  {MOP_load_f_16, {{AArch64MOP_t::MOP_undef, {kAbtractNone}, {}}}},
  {MOP_load_f_32, {{MOP_sldr, {kAbtractReg, kAbtractMem}, {0, 1}}}},
  {MOP_load_f_64, {{MOP_dldr, {kAbtractReg, kAbtractMem}, {0, 1}}}},

  {MOP_add_8, {{MOP_waddrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_add_16, {{MOP_waddrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_add_32, {{MOP_waddrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_add_64, {{MOP_xaddrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_sub_8, {{MOP_wsubrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_sub_16, {{MOP_wsubrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_sub_32, {{MOP_wsubrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_sub_64, {{MOP_xsubrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_or_8, {{MOP_wiorrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_or_16, {{MOP_wiorrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_or_32, {{MOP_wiorrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_or_64, {{MOP_xiorrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_xor_8, {{MOP_weorrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_xor_16, {{MOP_weorrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_xor_32, {{MOP_weorrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_xor_64, {{MOP_xeorrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_and_8, {{MOP_wandrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_and_16, {{MOP_wandrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_and_32, {{MOP_wandrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_and_64, {{MOP_xandrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},

  {MOP_and_f_8, {{AArch64MOP_t::MOP_undef, {kAbtractNone}, {}}}},
  {MOP_and_f_16, {{AArch64MOP_t::MOP_undef, {kAbtractNone}, {}}}},
  {MOP_and_f_32, {{AArch64MOP_t::MOP_undef, {kAbtractNone}, {}}}},
  {MOP_and_f_64, {{AArch64MOP_t::MOP_undef, {kAbtractNone}, {}}}},
  {MOP_add_f_8, {{AArch64MOP_t::MOP_undef, {kAbtractNone}, {}}}},
  {MOP_add_f_16, {{AArch64MOP_t::MOP_undef, {kAbtractNone}, {}}}},
  {MOP_add_f_32, {{MOP_sadd, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_add_f_64, {{MOP_dadd, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_sub_f_8, {{AArch64MOP_t::MOP_undef, {kAbtractNone}, {}}}},
  {MOP_sub_f_16, {{AArch64MOP_t::MOP_undef, {kAbtractNone}, {}}}},
  {MOP_sub_f_32, {{MOP_ssub, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_sub_f_64, {{MOP_dsub, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},

  {MOP_shl_8, {{MOP_wlslrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_shl_16, {{MOP_wlslrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_shl_32, {{MOP_wlslrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_shl_64, {{MOP_xlslrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_ashr_8, {{MOP_wasrrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_ashr_16, {{MOP_wasrrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_ashr_32, {{MOP_wasrrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_ashr_64, {{MOP_xasrrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_lshr_8, {{MOP_wlsrrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_lshr_16, {{MOP_wlsrrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_lshr_32, {{MOP_wlsrrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},
  {MOP_lshr_64, {{MOP_xlsrrrr, {kAbtractReg, kAbtractReg, kAbtractReg}, {0, 1, 2}}}},

  {MOP_neg_8, {{MOP_winegrr, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_neg_16, {{MOP_winegrr, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_neg_32, {{MOP_winegrr, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_neg_64, {{MOP_xinegrr, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_neg_f_8, {{AArch64MOP_t::MOP_undef, {kAbtractNone}, {}}}},
  {MOP_neg_f_16, {{AArch64MOP_t::MOP_undef, {kAbtractNone}, {}}}},
  {MOP_neg_f_32, {{MOP_wfnegrr, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_neg_f_64, {{MOP_xfnegrr, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_not_8, {{MOP_wnotrr, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_not_16, {{MOP_wnotrr, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_not_32, {{MOP_wnotrr, {kAbtractReg, kAbtractReg}, {0, 1}}}},
  {MOP_not_64, {{MOP_xnotrr, {kAbtractReg, kAbtractReg}, {0, 1}}}},

  {MOP_comment, {{MOP_nop, {kAbtractNone}, {}}}},
};

Operand *AArch64Standardize::GetInsnResult(Insn *insn) {
  for (uint32 i = 0; i < insn->GetOperandSize(); ++i) {
    if (insn->OpndIsDef(i)) {
      return &(insn->GetOperand(i));
    }
  }
  return nullptr;
}

Insn *AArch64Standardize::HandleTargetImm(Insn *insn, Insn *newInsn, uint32 idx, MOperator targetMop, uint8 order) {
  const InsnDesc *md = &AArch64CG::kMd[targetMop];
  ImmOperand &immOpnd = static_cast<ImmOperand&>(insn->GetOperand(idx));
  if (md->IsValidImmOpnd(immOpnd.GetValue())) {
    newInsn->SetOperand(order, immOpnd);
  } else {
    Operand *resOpnd = GetInsnResult(insn);
    CHECK_FATAL(resOpnd, "SelectTargetInsn: No result operand");
    AArch64CGFunc *a64func = static_cast<AArch64CGFunc*>(GetCgFunc());
    BB &saveCurBB = *GetCgFunc()->GetCurBB();
    a64func->GetDummyBB()->ClearInsns();
    GetCgFunc()->SetCurBB(*a64func->GetDummyBB());
    a64func->SelectCopyImm(*resOpnd, immOpnd, (resOpnd->GetSize() == k32BitSize) ? PTY_i32 : PTY_i64);
    insn->GetBB()->InsertBeforeInsn(*a64func->GetDummyBB(), *insn);
    GetCgFunc()->SetCurBB(saveCurBB);
    newInsn = nullptr;
  }
  return newInsn;
}

void AArch64Standardize::SelectTargetInsn(Insn *insn) {
  MOperator abstractMop = insn->GetMachineOpcode();
  CHECK_FATAL(abstractMop < kMopLast, "SelectTargetInsn: abstract instruction opcode out-of-bound");
  AbstractIR2Target &entry = abstract2TargetTable[abstractMop];
  CHECK_FATAL(entry.abstractMop == abstractMop, "SelectTargetInsn: Invalid abstract instruction");

  for (uint32 j = 0; j < entry.targetMap.size(); ++j) {
    TargetMopGen &targetMopGen = entry.targetMap[j];
    MOperator targetMop = targetMopGen.targetMop;
    if (targetMop == MOP_nop) {
      continue;
    }
    Insn *newInsn = &GetCgFunc()->GetInsnBuilder()->BuildInsn(targetMop, AArch64CG::kMd[targetMop]);
    newInsn->ResizeOpnds(targetMopGen.mappingOrder.size());
    for (uint32 i = 0; i < targetMopGen.mappingOrder.size(); ++i) {
      uint8 order = targetMopGen.mappingOrder[i];
      switch (targetMopGen.targetOpndAction[i]) {
        case kAbtractReg:
        case kAbtractMem:
          newInsn->SetOperand(order, insn->GetOperand(i));
          break;
        case kAbtractImm: {
          newInsn = HandleTargetImm(insn, newInsn, i, targetMop, order);
          break;
        }
        case kAbtractNone:
          break;
      }
    }
    if (newInsn) {
      insn->GetBB()->InsertInsnBefore(*insn, *newInsn);
    }
  }
  insn->GetBB()->RemoveInsn(*insn);
}

Operand *AArch64Standardize::UpdateRegister(Operand &opnd, std::map<regno_t, regno_t> &regMap, bool allocate) {
  if (!opnd.IsRegister()) {
    return &opnd;
  }
  RegOperand &regOpnd = static_cast<RegOperand&>(opnd);
  if (regOpnd.IsPhysicalRegister()) {
    if (allocate && opnd.GetSize() < k32BitSize) {
      opnd.SetSize(k32BitSize);
    }
    return &opnd;
  }
  if (!allocate && opnd.GetSize() >= k32BitSize) {
    return &opnd;
  }
  regno_t regno = regOpnd.GetRegisterNumber();
  regno_t mappedRegno;
  auto regItem = regMap.find(regno);
  if (regItem == regMap.end()) {
    if (allocate) {
      return &opnd;
    }
    regno_t vreg = GetCgFunc()->NewVReg(regOpnd.GetRegisterType(), k4ByteSize);
    regMap[regno] = mappedRegno = vreg;
  } else {
    mappedRegno = regItem->second;
  }
  if (!allocate) {
    return &opnd;
  }
  return &GetCgFunc()->GetOrCreateVirtualRegisterOperand(mappedRegno);
}

void AArch64Standardize::TraverseOperands(Insn *insn, std::map<regno_t, regno_t> &regMap,  bool allocate) {
  for (uint32 i = 0; i < insn->GetOperandSize(); i++) {
    Operand &opnd = insn->GetOperand(i);
    if (opnd.IsList()) {
      MapleList<RegOperand*> &list = static_cast<ListOperand&>(opnd).GetOperands();
      for (uint j = 0; j < list.size(); ++j) {
        RegOperand *lopnd = list.front();
        list.pop_front();
        list.push_back(static_cast<RegOperand*>(UpdateRegister(*lopnd, regMap, allocate)));
      }
    } else if (opnd.IsMemoryAccessOperand()) {
      MemOperand &mopnd = static_cast<MemOperand&>(opnd);
      Operand *base = mopnd.GetBaseRegister();
      if (base) {
        RegOperand *ropnd = static_cast<RegOperand*>(UpdateRegister(*base, regMap, allocate));
        mopnd.SetBaseRegister(*ropnd);
      }
    } else {
      insn->SetOperand(i, *UpdateRegister(opnd, regMap, allocate));
    }
  }
}

void AArch64Standardize::Legalize() {
  std::map<regno_t, regno_t> regMap;
  FOR_ALL_BB(bb, GetCgFunc()) {
    FOR_BB_INSNS(insn, bb) {
      TraverseOperands(insn, regMap, false);
    }
  }
  FOR_ALL_BB(bb, GetCgFunc()) {
    FOR_BB_INSNS(insn, bb) {
      TraverseOperands(insn, regMap, true);
    }
  }
}

void AArch64Standardize::StdzMov(Insn &insn) {
  SelectTargetInsn(&insn);
}

void AArch64Standardize::StdzStrLdr(Insn &insn) {
  SelectTargetInsn(&insn);
}

void AArch64Standardize::StdzBasicOp(Insn &insn) {
  SelectTargetInsn(&insn);
}

void AArch64Standardize::StdzUnaryOp(Insn &insn) {
  SelectTargetInsn(&insn);
}

void AArch64Standardize::StdzCvtOp(Insn &insn) {
  SelectTargetInsn(&insn);
}

void AArch64Standardize::StdzShiftOp(Insn &insn) {
  SelectTargetInsn(&insn);
}
void AArch64Standardize::StdzCommentOp(Insn &insn) {
  SelectTargetInsn(&insn);
}

}
