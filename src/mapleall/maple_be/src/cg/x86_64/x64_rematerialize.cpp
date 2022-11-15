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

#include "x64_rematerialize.h"
#include "x64_cgfunc.h"
#include "reg_alloc_color_ra.h"

namespace maplebe {
std::vector<Insn*> X64Rematerializer::RematerializeForConstval(CGFunc &cgFunc,
    RegOperand &regOp, const LiveRange &lr) {
  std::vector<Insn*> insns;
  auto intConst = static_cast<const MIRIntConst*>(rematInfo.mirConst);
  ImmOperand *immOp = &cgFunc.GetOpndBuilder()->CreateImm(
      GetPrimTypeBitSize(intConst->GetType().GetPrimType()), intConst->GetExtValue());
  MOperator movOp = x64::MOP_begin;
  switch (lr.GetSpillSize()) {
    case k8BitSize:
      movOp = x64::MOP_movb_i_r;
      break;
    case k16BitSize:
      movOp = x64::MOP_movw_i_r;
      break;
    case k32BitSize:
      movOp = x64::MOP_movl_i_r;
      break;
    case k64BitSize:
      movOp = x64::MOP_movq_i_r;
      break;
    default:
      break;
  }
  CHECK_FATAL(movOp != x64::MOP_begin, "NIY, unkown mop");
  insns.push_back(&cgFunc.GetInsnBuilder()->BuildInsn(movOp, *immOp, regOp));
  return insns;
}

std::vector<Insn*> X64Rematerializer::RematerializeForAddrof(CGFunc &cgFunc,
    RegOperand &regOp, int32 offset) {
  std::vector<Insn*> insns;
  CHECK_FATAL(false, "NIY");
  return insns;
}

std::vector<Insn*> X64Rematerializer::RematerializeForDread(CGFunc &cgFunc,
    RegOperand &regOp, int32 offset, PrimType type) {
  std::vector<Insn*> insns;
  CHECK_FATAL(false, "NIY");
  return insns;
}

}  /* namespace maplebe */
