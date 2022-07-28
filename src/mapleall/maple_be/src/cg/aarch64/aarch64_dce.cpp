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
    if (defInsn->GetMachineOpcode() == MOP_asm || defInsn->IsVectorOp() || defInsn->IsAtomic()) {
      return false;
    }
    std::set<uint32> defRegs = defInsn->GetDefRegs();
    if (defRegs.size() != 1) {
      return false;
    }
    uint32 bothDUIdx = defInsn->GetBothDefUseOpnd();
    if (bothDUIdx == kInsnMaxOpnd ||
        (defInsnInfo->GetOperands().count(bothDUIdx) > 0 && defInsnInfo->GetOperands().at(bothDUIdx) == 1)) {
      defInsn->GetBB()->RemoveInsn(*defInsn);
      if (defInsn->IsPhi()) {
        defInsn->GetBB()->RemovePhiInsn(defVersion.GetOriginalRegNO());
      }
      defVersion.MarkDeleted();
      uint32 opndNum = defInsn->GetOperandSize();
      for (uint32 i = opndNum; i > 0; --i) {
        Operand &opnd = defInsn->GetOperand(i - 1);
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
    ASSERT(regVersion != nullptr, "regVersion should not be nullptr");
    MapleUnorderedMap<uint32, DUInsnInfo*> &useInfos = regVersion->GetAllUseInsns();
    auto it = useInfos.find(deleteInsnId);
    if (it != useInfos.end()) {
      useInfos.erase(it);
    }
  }
}
void A64DeleteRegUseVisitor::Visit(ListOperand *v) {
  for (auto *regOpnd : std::as_const(v->GetOperands())) {
    Visit(regOpnd);
  }
}
void A64DeleteRegUseVisitor::Visit(MemOperand *v) {
  RegOperand *baseRegOpnd = v->GetBaseRegister();
  RegOperand *indexRegOpnd = v->GetIndexRegister();
  if (baseRegOpnd != nullptr && baseRegOpnd->IsSSAForm()) {
    Visit(baseRegOpnd);
  }
  if (indexRegOpnd != nullptr && indexRegOpnd->IsSSAForm()) {
    Visit(indexRegOpnd);
  }
}

void A64DeleteRegUseVisitor::Visit(PhiOperand *v) {
  for (auto &phiOpndIt : std::as_const(v->GetOperands())) {
    Visit(phiOpndIt.second);
  }
}
}
