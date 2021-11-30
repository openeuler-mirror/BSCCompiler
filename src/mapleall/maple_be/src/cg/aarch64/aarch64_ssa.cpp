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

#include "aarch64_ssa.h"
#include "aarch64_cg.h"

namespace maplebe {
void AArch64CGSSAInfo::RenameInsn(Insn &insn) {
  auto opndNum = static_cast<int32>(insn.GetOperandSize());
  const AArch64MD *md = &AArch64CG::kMd[static_cast<AArch64Insn&>(insn).GetMachineOpcode()];
  if (md->IsPhi()) {
    return;
  }
  uint32 hasDef = 0; // debug use
  for (int i = opndNum - 1; i >= 0; --i) {
    Operand &opnd = insn.GetOperand(i);
    auto *opndProp = static_cast<AArch64OpndProp*>(md->operand[i]);
    if (opnd.IsList()) {
      ASSERT(!opndProp->IsDef(), "do not support def list in aarch64");
      RenameListOpnd(static_cast<AArch64ListOperand&>(opnd), insn.GetMachineOpcode() == MOP_asm, i, insn);
#if 1
      for (auto *op : static_cast<ListOperand&>(opnd).GetOperands()) {
        CHECK_FATAL(static_cast<RegOperand*>(op)->GetRegisterNumber() != hasDef, "check in armv8");
      }
#endif
    } else if (opnd.IsMemoryAccessOperand()) {
      auto &memOpnd = static_cast<AArch64MemOperand&>(opnd);
      if (!memOpnd.IsIntactIndexed()) {
        ASSERT(memOpnd.GetBaseRegister() != nullptr, "pre/post-index must have base register");
        ASSERT(hasDef != memOpnd.GetBaseRegister()->GetRegisterNumber(), "check in armv8");
        hasDef = memOpnd.GetBaseRegister()->GetRegisterNumber();
      }
      RenameMemOpnd(static_cast<AArch64MemOperand&>(opnd), insn);
    } else if (opnd.IsRegister()) {
      auto &regOpnd = static_cast<RegOperand&>(opnd);
      ASSERT(!(opndProp->IsRegUse() && hasDef == regOpnd.GetRegisterNumber()), "check in armv8");
      hasDef = opndProp->IsRegDef() ? regOpnd.GetRegisterNumber() : 0;
      if (regOpnd.GetRegisterNumber() != kRFLAG && regOpnd.IsVirtualRegister()) {
        if (opndProp->IsRegDef() && opndProp->IsRegUse()) {        /* both def use */
          insn.SetOperand(i, *GetRenamedOperand(regOpnd, false, insn));
          (void)GetRenamedOperand(regOpnd, true, insn);
        } else {
          insn.SetOperand(i, *GetRenamedOperand(regOpnd, opndProp->IsRegDef(), insn));
        }
      }
    } else {
      /* unsupport ssa opnd */
    }
  }
}

void AArch64CGSSAInfo::RenameListOpnd(AArch64ListOperand &listOpnd, bool isAsm, uint32 idx, Insn &curInsn) {
  /* record the orignal list order */
  std::list<RegOperand*> tempList;
  for (auto *op : listOpnd.GetOperands()) {
    if (op->IsSSAForm() || op->GetRegisterNumber() == kRFLAG || !op->IsVirtualRegister()) {
      listOpnd.RemoveOpnd(*op);
      tempList.push_back(op);
      continue;
    }
    listOpnd.RemoveOpnd(*op);
    RegOperand *renameOpnd = GetRenamedOperand(
        *op, isAsm ? idx == kAsmClobberListOpnd || idx == kAsmOutputListOpnd : false, curInsn);
    tempList.push_back(renameOpnd);
  }
  ASSERT(listOpnd.GetOperands().empty(), "need to clean list");
  listOpnd.GetOperands().assign(tempList.begin(), tempList.end());
}

void AArch64CGSSAInfo::RenameMemOpnd(AArch64MemOperand &memOpnd, Insn &curInsn) {
  RegOperand *base = memOpnd.GetBaseRegister();
  RegOperand *index = memOpnd.GetIndexRegister();
  bool needCopy = (base != nullptr && base->IsVirtualRegister()) || (index != nullptr && index->IsVirtualRegister());
  if (needCopy) {
    AArch64MemOperand *copyMem = CreateMemOperandOnSSA(memOpnd);
    if (base != nullptr && base->IsVirtualRegister()) {
      copyMem->SetBaseRegister(
          *static_cast<AArch64RegOperand*>(GetRenamedOperand(*base, !memOpnd.IsIntactIndexed(), curInsn)));
    }
    if (index != nullptr && index->IsVirtualRegister()) {
      copyMem->SetIndexRegister(*GetRenamedOperand(*index, false, curInsn));
    }
    curInsn.SetMemOpnd(copyMem);
  }
}

AArch64MemOperand *AArch64CGSSAInfo::CreateMemOperandOnSSA(AArch64MemOperand &memOpnd) {
  return static_cast<AArch64MemOperand*>(memOpnd.Clone(*memPool));
}

RegOperand *AArch64CGSSAInfo::GetRenamedOperand(RegOperand &vRegOpnd, bool isDef, Insn &curInsn) {
  if (vRegOpnd.IsVirtualRegister()) {
    ASSERT(!vRegOpnd.IsSSAForm(), "Unexpect ssa operand");
    if (isDef) {
      VRegVersion *newVersion = CreateNewVersion(vRegOpnd, curInsn);
      CHECK_FATAL(newVersion != nullptr, "get ssa version failed");
      return newVersion->GetSSAvRegOpnd();
    } else {
      VRegVersion *curVersion = GetVersion(vRegOpnd);
      if (curVersion == nullptr) {
        LogInfo::MapleLogger() << "WARNING: " << vRegOpnd.GetRegisterNumber() << " has no def info in function : "
                               << cgFunc->GetName() << " !\n";
        curVersion = CreateNewVersion(vRegOpnd, curInsn);
      }
      curVersion->addUseInsn(curInsn);
      return curVersion->GetSSAvRegOpnd();
    }
  }
  CHECK_FATAL(false, "Get Renamed operand failed");
  return nullptr;
}

RegOperand *AArch64CGSSAInfo::CreateSSAOperand(RegOperand &virtualOpnd) {
  constexpr uint32 ssaRegNObase = 100;
  regno_t ssaRegNO = GetAllSSAOperands().size() + ssaRegNObase;
  RegOperand *newVreg = memPool->New<AArch64RegOperand>(ssaRegNO, virtualOpnd.GetSize(), virtualOpnd.GetRegisterType());
  newVreg->SetOpndSSAForm();
  return newVreg;
}
}
