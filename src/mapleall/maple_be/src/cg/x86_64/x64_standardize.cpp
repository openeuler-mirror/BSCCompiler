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
bool X64Standardize::TryFastTargetIRMapping(maplebe::Insn &insn) {
  const InsnDescription &targetMd = X64CG::kMd[insn.GetMachineOpcode()];
  auto cmpFunc =[](const InsnDescription &left, const InsnDescription &right)->bool {
    uint32 leftPropClear = left.properties | ISABSTRACT;
    uint32 rightPropClear = left.properties | ISABSTRACT;
    if ((left.opndMD.size() == right.opndMD.size()) && (leftPropClear == rightPropClear)) {
      for (size_t i = 0; i < left.opndMD.size(); ++i) {
        if (left.opndMD[i] != right.opndMD[i]) {
          return false;
        }
      }
      if (CGOptions::kVerboseAsm) {
        LogInfo::MapleLogger() << "Do fast mapping for " << left.name << " to " << right.name << " successfully\n";
      }
      return true;
    }
    return false;
  };
  if (insn.GetInsnDescrption()->IsSame(targetMd, cmpFunc)) {
    insn.SetMOP(targetMd);
    return true;
  }
  return false;
}

void X64Standardize::StdzMov(maplebe::Insn &insn) {
  MOperator directlyMappingMop = abstract::MOP_undef;
  switch (insn.GetMachineOpcode()) {
    case abstract::MOP_copy_ri_32:
      directlyMappingMop = x64::MOP_movl_i_r;
      break;
    case abstract::MOP_copy_rr_32:
      directlyMappingMop = x64::MOP_movl_r_r;
      break;
    default:
      break;
  }
  if (directlyMappingMop != abstract::MOP_undef) {
    insn.SetMOP(X64CG::kMd[directlyMappingMop]);
    insn.CommuteOperands(kInsnFirstOpnd, kInsnSecondOpnd);
  } else {
    CHECK_FATAL(false, "NIY mapping");
  }
}

void X64Standardize::StdzStrLdr(Insn &insn) {
  MOperator directlyMappingMop = abstract::MOP_undef;
  switch (insn.GetMachineOpcode()) {
    case abstract::MOP_str_32:
      directlyMappingMop = x64::MOP_movl_r_m;
      break;
    case abstract::MOP_load_32:
      directlyMappingMop = x64::MOP_movl_m_r;
      break;
    default:
      break;
  }
  if (directlyMappingMop != abstract::MOP_undef) {
    insn.SetMOP(X64CG::kMd[directlyMappingMop]);
    insn.CommuteOperands(kInsnFirstOpnd, kInsnSecondOpnd);
  } else {
    CHECK_FATAL(false, "NIY mapping");
  }
}

void X64Standardize::StdzBasicOp(Insn &insn) {
  MOperator directlyMappingMop = abstract::MOP_undef;
  switch (insn.GetMachineOpcode()) {
    case abstract::MOP_add_32:
      directlyMappingMop = x64::MOP_addl_r_r;
      break;
    default:
      break;
  }
  if (directlyMappingMop != abstract::MOP_undef) {
    insn.SetMOP(X64CG::kMd[directlyMappingMop]);
    Operand &dest = insn.GetOperand(kInsnFirstOpnd);
    Operand &src2 = insn.GetOperand(kInsnThirdOpnd);
    insn.CleanAllOperand();
    insn.AddOperandChain(src2).AddOperandChain(dest);
  } else {
    CHECK_FATAL(false, "NIY mapping");
  }
}
}
