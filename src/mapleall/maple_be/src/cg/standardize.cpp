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
#include "standardize.h"
namespace maplebe {
void Standardize::DoStandardize() {
  /* two address mapping first */
  FOR_ALL_BB(bb, cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (NeedTwoAddressMapping(*insn)) {
        TwoAddressMapping(*insn);
      }
    }
  }

  /* standardize for each op */
  FOR_ALL_BB(bb, cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      MOperator mOp = insn->GetMachineOpcode();
      switch (mOp) {
        case abstract::MOP_copy_ri_32:
          STDZcopyri(*insn);
          break;
        case abstract::MOP_str_32:
          STDZstr(*insn);
          break;
        case abstract::MOP_copy_rr_32:
          STDZcopyrr(*insn);
          break;
        case abstract::MOP_load_32:
          STDZload(*insn);
          break;
        case abstract::MOP_add_32:
          STDZaddrr(*insn);
          break;
        default:
          break;
      }
    }
  }
}

/* mop(dest, src1, src2) -> mov(src1,dest), mop(src2, dest) */
void Standardize::TwoAddressMapping(Insn &insn) {
  Operand &dest = insn.GetOperand(kInsnFirstOpnd);
  Operand &src1 = insn.GetOperand(kInsnSecondOpnd);
  MOperator mOp = abstract::MOP_copy_rr_32;
  Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, InsnDescription::GetAbstractId(mOp));
  newInsn.AddOperandChain(dest).AddOperandChain(src1);
  cgFunc->GetCurBB()->InsertInsnBefore(insn,newInsn);
}
}
