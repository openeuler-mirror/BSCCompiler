/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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

#include "aarch64_isa.h"
#include "insn.h"

namespace maplebe {
/*
 * Get the ldp/stp corresponding to ldr/str
 * mop : a ldr or str machine operator
 */
MOperator GetMopPair(MOperator mop) {
  switch (mop) {
    case MOP_xldr:
      return MOP_xldp;
    case MOP_wldr:
      return MOP_wldp;
    case MOP_xstr:
      return MOP_xstp;
    case MOP_wstr:
      return MOP_wstp;
    case MOP_dldr:
      return MOP_dldp;
    case MOP_qldr:
      return MOP_qldp;
    case MOP_sldr:
      return MOP_sldp;
    case MOP_dstr:
      return MOP_dstp;
    case MOP_sstr:
      return MOP_sstp;
    case MOP_qstr:
      return MOP_qstp;
    default:
      ASSERT(false, "should not run here");
      return MOP_undef;
  }
}
namespace AArch64isa {
MOperator FlipConditionOp(MOperator flippedOp) {
  switch (flippedOp) {
    case AArch64MOP_t::MOP_beq:
      return AArch64MOP_t::MOP_bne;
    case AArch64MOP_t::MOP_bge:
      return AArch64MOP_t::MOP_blt;
    case AArch64MOP_t::MOP_bgt:
      return AArch64MOP_t::MOP_ble;
    case AArch64MOP_t::MOP_bhi:
      return AArch64MOP_t::MOP_bls;
    case AArch64MOP_t::MOP_bhs:
      return AArch64MOP_t::MOP_blo;
    case AArch64MOP_t::MOP_ble:
      return AArch64MOP_t::MOP_bgt;
    case AArch64MOP_t::MOP_blo:
      return AArch64MOP_t::MOP_bhs;
    case AArch64MOP_t::MOP_bls:
      return AArch64MOP_t::MOP_bhi;
    case AArch64MOP_t::MOP_blt:
      return AArch64MOP_t::MOP_bge;
    case AArch64MOP_t::MOP_bne:
      return AArch64MOP_t::MOP_beq;
    case AArch64MOP_t::MOP_bpl:
      return AArch64MOP_t::MOP_bmi;
    case AArch64MOP_t::MOP_xcbnz:
      return AArch64MOP_t::MOP_xcbz;
    case AArch64MOP_t::MOP_wcbnz:
      return AArch64MOP_t::MOP_wcbz;
    case AArch64MOP_t::MOP_xcbz:
      return AArch64MOP_t::MOP_xcbnz;
    case AArch64MOP_t::MOP_wcbz:
      return AArch64MOP_t::MOP_wcbnz;
    case AArch64MOP_t::MOP_wtbnz:
      return AArch64MOP_t::MOP_wtbz;
    case AArch64MOP_t::MOP_wtbz:
      return AArch64MOP_t::MOP_wtbnz;
    case AArch64MOP_t::MOP_xtbnz:
      return AArch64MOP_t::MOP_xtbz;
    case AArch64MOP_t::MOP_xtbz:
      return AArch64MOP_t::MOP_xtbnz;
    default:
      break;
  }
  return AArch64MOP_t::MOP_undef;
}

uint32 GetJumpTargetIdx(const Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  switch (curMop) {
    /* unconditional jump */
    case MOP_xuncond: {
      return kInsnFirstOpnd;
    }
    case MOP_xbr: {
      ASSERT(insn.GetOperandSize() == 2, "ERR");
      return kInsnSecondOpnd;
    }
    /* conditional jump */
    case MOP_bcc:
    case MOP_bcs:
    case MOP_bmi:
    case MOP_bvc:
    case MOP_bls:
    case MOP_blt:
    case MOP_ble:
    case MOP_blo:
    case MOP_beq:
    case MOP_bpl:
    case MOP_bhs:
    case MOP_bvs:
    case MOP_bhi:
    case MOP_bgt:
    case MOP_bge:
    case MOP_bne:
    case MOP_wcbz:
    case MOP_xcbz:
    case MOP_wcbnz:
    case MOP_xcbnz: {
      return kInsnSecondOpnd;
    }
    case MOP_wtbz:
    case MOP_xtbz:
    case MOP_wtbnz:
    case MOP_xtbnz: {
      return kInsnThirdOpnd;
    }
    default:
      CHECK_FATAL(false, "Not a jump insn");
  }
  return kInsnFirstOpnd;
}

bool IsSub(const Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  switch (curMop) {
    case MOP_xsubrrr:
    case MOP_xsubrrrs:
    case MOP_xsubrri24:
    case MOP_xsubrri12:
    case MOP_wsubrrr:
    case MOP_wsubrrrs:
    case MOP_wsubrri24:
    case MOP_wsubrri12:
      return true;
    default:
      return false;
  }
}

MOperator GetMopSub2Subs(const Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  switch (curMop) {
    case MOP_xsubrrr:
      return MOP_xsubsrrr;
    case MOP_xsubrrrs:
      return MOP_xsubsrrrs;
    case MOP_xsubrri24:
      return MOP_xsubsrri24;
    case MOP_xsubrri12:
      return MOP_xsubsrri12;
    case MOP_wsubrrr:
      return MOP_wsubsrrr;
    case MOP_wsubrrrs:
      return MOP_wsubsrrrs;
    case MOP_wsubrri24:
      return MOP_wsubsrri24;
    case MOP_wsubrri12:
      return MOP_wsubsrri12;
    default:
      return curMop;
  }
}

// This api is only used for cgir verify, implemented by calling the memopndofst interface.
int64 GetMemOpndOffsetValue(MOperator mOp, Operand *o) {
  auto *memOpnd = static_cast<MemOperand*>(o);
  CHECK_FATAL(memOpnd != nullptr, "memOpnd should not be nullptr");
  // kBOR memOpnd has no offsetvalue, so return 0 for verify.
  if (memOpnd->GetAddrMode() == MemOperand::kBOR) {
    return 0;
  }
  OfstOperand *ofStOpnd = memOpnd->GetOffsetImmediate();
  int64 offsetValue = ofStOpnd ? ofStOpnd->GetOffsetValue() : 0LL;
  return offsetValue;
}
} /* namespace AArch64isa */
}  /* namespace maplebe */
