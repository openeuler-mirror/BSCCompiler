/*
* Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "aarch64_dce.h"
#include "aarch64_operand.h"
namespace maplebe {
bool AArch64Dce::RemoveUnuseDef(VRegVersion &defVersion) {
  /* delete defs which have no uses */
  if (defVersion.GetAllUseInsns().empty()) {
    DUInsnInfo *defInsnInfo = defVersion.GetDefInsnInfo();
    if (defInsnInfo == nullptr) {
      return false;
    }
    CHECK_FATAL(defInsnInfo->GetInsn() != nullptr, "Get def insn failed");
    Insn *defInsn = defInsnInfo->GetInsn();
    /* have not support asm/neon opt yet */
    if (defInsn->GetMachineOpcode() == MOP_asm || defInsn->IsVectorOp()) {
      return false;
    }
    std::set<uint32> defRegs = defInsn->GetDefRegs();
    if (defRegs.size() != 1) {
      return false;
    }
    uint32 bothDUIdx = defInsn->GetBothDefUseOpnd();
    if (!(bothDUIdx != kInsnMaxOpnd && defInsnInfo->GetOperands().count(bothDUIdx))) {
      defInsn->GetBB()->RemoveInsn(*defInsn);
      if (defInsn->IsPhi()) {
        defInsn->GetBB()->RemovePhiInsn(defVersion.GetOriginalRegNO());
      }
      defVersion.MarkDeleted();
      uint32 opndNum = defInsn->GetOperandSize();
      for (int32 i = opndNum - 1; i >= 0; --i) {
        Operand &opnd = defInsn->GetOperand(static_cast<uint32>(i));
        A64DeleteRegUseVisitor deleteUseRegVisitor(*GetSSAInfo(), defInsn->GetId());
        opnd.Accept(deleteUseRegVisitor);
      }
      return true;
    }
  }
  return false;
}

void A64DeleteRegUseVisitor::Visit(RegOperand *v) {
  if (v->IsSSAForm()) {
    VRegVersion *regVersion = GetSSAInfo()->FindSSAVersion(v->GetRegisterNumber());
    MapleUnorderedMap<uint32, DUInsnInfo*> &useInfos = regVersion->GetAllUseInsns();
    auto it = useInfos.find(deleteInsnId);
    if (it != useInfos.end()) {
      useInfos.erase(it);
    }
  }
}
void A64DeleteRegUseVisitor::Visit(ListOperand *v) {
  for (auto *regOpnd : v->GetOperands()) {
    Visit(regOpnd);
  }
}
void A64DeleteRegUseVisitor::Visit(MemOperand *v) {
  auto *a64MemOpnd = static_cast<AArch64MemOperand*>(v);
  RegOperand *baseRegOpnd = a64MemOpnd->GetBaseRegister();
  RegOperand *indexRegOpnd = a64MemOpnd->GetIndexRegister();
  if (baseRegOpnd != nullptr && baseRegOpnd->IsSSAForm()) {
    Visit(baseRegOpnd);
  }
  if (indexRegOpnd != nullptr && indexRegOpnd->IsSSAForm()) {
    Visit(indexRegOpnd);
  }
}

void A64DeleteRegUseVisitor::Visit(PhiOperand *v) {
  for (auto phiOpndIt : v->GetOperands()) {
    Visit(phiOpndIt.second);
  }
}
}