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
#include "x64_peep.h"
#include "cg.h"
#include "mpl_logging.h"
#include "common_utils.h"
#include "cg_option.h"
#include "x64_cg.h"

namespace maplebe {
void X64CGPeepHole::Run() {
  FOR_ALL_BB(bb, cgFunc) {
    FOR_BB_INSNS_SAFE(insn, bb, nextInsn) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      if (ssaInfo == nullptr) {
        DoNormalOptimize(*bb, *insn);
      }
    }
  }
}

bool X64CGPeepHole::DoSSAOptimize(BB &bb, Insn &insn) {
  CHECK_FATAL(false, "x64 does not support ssa optimize");
  return false;
}

bool RemoveMovingtoSameRegPattern::CheckCondition(Insn &insn) {
  ASSERT(insn.GetOperand(kInsnFirstOpnd).IsRegister(), "expects registers");
  ASSERT(insn.GetOperand(kInsnSecondOpnd).IsRegister(), "expects registers");
  auto &reg1 = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  auto &reg2 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  /* remove mov x0,x0 when it cast i32 to i64 */
  if ((reg1.GetRegisterNumber() == reg2.GetRegisterNumber()) && (reg1.GetSize() >= reg2.GetSize())) {
    return true;
  }
  return false;
}

void RemoveMovingtoSameRegPattern::Run(BB &bb, Insn &insn) {
  /* remove mov x0,x0 when it cast i32 to i64 */
  if (CheckCondition(insn)) {
    bb.RemoveInsn(insn);
  }
}

void X64CGPeepHole::DoNormalOptimize(BB &bb, Insn &insn) {
  MOperator thisMop = insn.GetMachineOpcode();
  manager = peepMemPool->New<PeepOptimizeManager>(*cgFunc, bb, insn);
  switch (thisMop) {
    case MOP_movb_r_r:
    case MOP_movw_r_r:
    case MOP_movl_r_r:
    case MOP_movq_r_r: {
      manager->NormalPatternOpt<RemoveMovingtoSameRegPattern>(true);
      break;
    }
    default:
      break;
  }
  }
}  /* namespace maplebe */
