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
#include "cg.h"
#include "standardize.h"

namespace maplebe {

void Standardize::DoStandardize() {
  /* two address mapping first */
  FOR_ALL_BB(bb, cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (insn->IsDbgInsn()) {
        continue;
      }
      if (insn->IsMachineInstruction()) {
        continue;
      }
      if (NeedAddressMapping(*insn)) {
        AddressMapping(*insn);
      }
    }
  }
  Legalize();
  /* standardize for each op */
  FOR_ALL_BB(bb, cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (insn->IsDbgInsn()) {
        continue;
      }
      if (insn->IsMachineInstruction()) {
        continue;
      }
      if (insn->IsMove()) {
        StdzMov(*insn);
      } else if (insn->IsStore() || insn->IsLoad()) {
        StdzStrLdr(*insn);
      } else if (insn->IsBasicOp()) {
        StdzBasicOp(*insn);
      } else if (insn->IsUnaryOp()) {
        StdzUnaryOp(*insn);
      } else if (insn->IsConversion()) {
        StdzCvtOp(*insn);
      } else if (insn->IsShift()) {
        StdzShiftOp(*insn);
      } else if (insn->IsComment()) {
        StdzCommentOp(*insn);
      } else {
        LogInfo::MapleLogger() << "Need STDZ function for " << insn->GetDesc()->GetName() << "\n";
        CHECK_FATAL(false, "NIY");
      }
    }
  }
}

void Standardize::AddressMapping(Insn &insn) const {
  Operand &dest = insn.GetOperand(kInsnFirstOpnd);
  Operand &src1 = insn.GetOperand(kInsnSecondOpnd);
  uint32 destSize = dest.GetSize();
  CHECK_FATAL(dest.IsRegister(), "AddressMapping: not reg operand");
  bool isInt = static_cast<RegOperand&>(dest).GetRegisterType() == kRegTyInt ? true : false;
  MOperator mOp = abstract::MOP_undef;
  switch (destSize) {
    case k8BitSize:
      if (isInt) {
        mOp = abstract::MOP_copy_rr_8;
      } else {
        mOp = abstract::MOP_copy_ff_8;
      }
      break;
    case k16BitSize:
      if (isInt) {
        mOp = abstract::MOP_copy_rr_16;
      } else {
        mOp = abstract::MOP_copy_ff_16;
      }
      break;
    case k32BitSize:
      if (isInt) {
        mOp = abstract::MOP_copy_rr_32;
      } else {
        mOp = abstract::MOP_copy_ff_32;
      }
      break;
    case k64BitSize:
      if (isInt) {
        mOp = abstract::MOP_copy_rr_64;
      } else {
        mOp = abstract::MOP_copy_ff_64;
      }
      break;
    default:
      break;
  }
  CHECK_FATAL(mOp != abstract::MOP_undef, "do two address mapping failed");
  Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, InsnDesc::GetAbstractId(mOp));
  (void)newInsn.AddOpndChain(dest).AddOpndChain(src1);
  (void)insn.GetBB()->InsertInsnBefore(insn, newInsn);
}

bool InstructionStandardize::PhaseRun(maplebe::CGFunc &f) {
  Standardize *stdz = f.GetCG()->CreateStandardize(*GetPhaseMemPool(), f);
  stdz->DoStandardize();
  return true;
}
}
