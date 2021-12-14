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
      RenameListOpnd(static_cast<AArch64ListOperand&>(opnd), insn.GetMachineOpcode() == MOP_asm, i, insn);
#if DEBUG
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
    curInsn.SetMemOpnd(&static_cast<AArch64CGFunc*>(cgFunc)->GetOrCreateMemOpnd(*copyMem));
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
        curVersion = RenamedOperandSpecialCase(vRegOpnd, curInsn);
      }
      curVersion->addUseInsn(curInsn);
      return curVersion->GetSSAvRegOpnd();
    }
  }
  CHECK_FATAL(false, "Get Renamed operand failed");
  return nullptr;
}

VRegVersion *AArch64CGSSAInfo::RenamedOperandSpecialCase(RegOperand &vRegOpnd, Insn &curInsn) {
  LogInfo::MapleLogger() << "WARNING: " << vRegOpnd.GetRegisterNumber() << " has no def info in function : "
                         << cgFunc->GetName() << " !\n";
  /* occupy operand for no def vreg */
  if (!IncreaseSSAOperand(vRegOpnd.GetRegisterNumber(), nullptr)) {
    ASSERT(GetAllSSAOperands().find(vRegOpnd.GetRegisterNumber()) != GetAllSSAOperands().end(), "should find");
    AddNoDefVReg(vRegOpnd.GetRegisterNumber());
  }
  VRegVersion *version = CreateNewVersion(vRegOpnd, curInsn);
  version->SetDefInsn(nullptr, kDefByNo);
  return version;
}

RegOperand *AArch64CGSSAInfo::CreateSSAOperand(RegOperand &virtualOpnd) {
  regno_t ssaRegNO = GetAllSSAOperands().size() + SSARegNObase;
  while (GetAllSSAOperands().count(ssaRegNO)) {
    ssaRegNO++;
    SSARegNObase++;
  }
  RegOperand *newVreg = memPool->New<AArch64RegOperand>(ssaRegNO, virtualOpnd.GetSize(), virtualOpnd.GetRegisterType());
  newVreg->SetOpndSSAForm();
  return newVreg;
}

void AArch64CGSSAInfo::DumpInsnInSSAForm(const Insn &insn) const {
  auto &a64Insn = static_cast<const AArch64Insn&>(insn);
  MOperator mOp = a64Insn.GetMachineOpcode();
  const AArch64MD *md = &AArch64CG::kMd[mOp];
  ASSERT(md != nullptr, "md should not be nullptr");

  LogInfo::MapleLogger() << "< " << a64Insn.GetId() << " > ";
  LogInfo::MapleLogger() << md->name << "(" << mOp << ")";

  for (uint32 i = 0; i < a64Insn.GetOperandSize(); ++i) {
    Operand &opnd = a64Insn.GetOperand(i);
    LogInfo::MapleLogger() << " (opnd" << i << ": ";
    if (opnd.IsRegister()) {
      auto &regOpnd = static_cast<RegOperand&>(opnd);
      ASSERT(!opnd.IsConditionCode(), "both condi and reg");
      if (regOpnd.IsSSAForm()) {
        DumpA64SSAOpnd(regOpnd);
        continue;
      }
    } else if (opnd.IsList()) {
      if (DumpA64ListOpnd(static_cast<AArch64ListOperand&>(opnd))) {
        continue;
      }
    } else if (opnd.IsMemoryAccessOperand()) {
      if (DumpA64SSAMemOpnd(static_cast<AArch64MemOperand&>(opnd))) {
        continue;
      }
    } else if (opnd.IsPhi()) {
      DumpA64PhiOpnd(static_cast<AArch64PhiOperand&>(opnd));
      continue;
    }
    opnd.Dump();
    LogInfo::MapleLogger() << ")";
  }

  if (a64Insn.IsVectorOp()) {
    auto &vInsn = static_cast<const AArch64VectorInsn&>(insn);
    if (vInsn.GetNumOfRegSpec() != 0) {
      LogInfo::MapleLogger() << " (vecSpec: " << vInsn.GetNumOfRegSpec() << ")";
    }
  }
  LogInfo::MapleLogger() << "\n";
}

bool AArch64CGSSAInfo::DumpA64SSAMemOpnd(AArch64MemOperand &a64MemOpnd) const {
  bool hasDumpedSSAbase = false;
  bool hasDumpedSSAIndex = false;
  if (a64MemOpnd.GetBaseRegister() != nullptr && a64MemOpnd.GetBaseRegister()->IsSSAForm()) {
    LogInfo::MapleLogger() << "Mem: ";
    DumpA64SSAOpnd(*a64MemOpnd.GetBaseRegister());
    if (a64MemOpnd.GetAddrMode() == AArch64MemOperand::kAddrModeBOi) {
      LogInfo::MapleLogger() << "offset:";
      a64MemOpnd.GetOffsetOperand()->Dump();
    }
    hasDumpedSSAbase = true;
  }
  if (a64MemOpnd.GetIndexRegister() != nullptr && a64MemOpnd.GetIndexRegister()->IsSSAForm() ) {
    ASSERT(a64MemOpnd.GetAddrMode() == AArch64MemOperand::kAddrModeBOrX, "mem mode false");
    LogInfo::MapleLogger() << "offset:";
    DumpA64SSAOpnd(*a64MemOpnd.GetIndexRegister());
    hasDumpedSSAIndex = true;
  }
  return hasDumpedSSAbase | hasDumpedSSAIndex;
}

bool AArch64CGSSAInfo::DumpA64ListOpnd(AArch64ListOperand &list) const {
  bool hasDumpedSSA = false;
  for (auto regOpnd : list.GetOperands()) {
    if (regOpnd->IsSSAForm()) {
      DumpA64SSAOpnd(*regOpnd);
      hasDumpedSSA = true;
      continue;
    }
  }
  return hasDumpedSSA;
}

void AArch64CGSSAInfo::DumpA64PhiOpnd(AArch64PhiOperand &phi) const {
  for (auto phiListIt = phi.GetOperands().begin(); phiListIt != phi.GetOperands().end();) {
    DumpA64SSAOpnd(*(phiListIt->second));
    LogInfo::MapleLogger() << " fBB<" << phiListIt->first << ">";
    LogInfo::MapleLogger() << (++phiListIt == phi.GetOperands().end() ? ")" : ", ");
  }
}

void AArch64CGSSAInfo::DumpA64SSAOpnd(RegOperand &vRegOpnd) const {
  std::array<const std::string, kRegTyLast> prims = { "U", "R", "V", "C", "X", "Vra" };
  std::array<const std::string, kRegTyLast> classes = { "[U]", "[I]", "[F]", "[CC]", "[X87]", "[Vra]" };
  CHECK_FATAL(vRegOpnd.IsVirtualRegister() && vRegOpnd.IsSSAForm(), "only dump ssa opnd here");
  RegType regType = vRegOpnd.GetRegisterType();
  ASSERT(regType < kRegTyLast, "unexpected regType");
  auto ssaVit = GetAllSSAOperands().find(vRegOpnd.GetRegisterNumber());
  CHECK_FATAL(ssaVit != GetAllSSAOperands().end(), "find ssa version failed");
  LogInfo::MapleLogger() << "ssa_reg:" << prims[regType] << ssaVit->second->GetOriginalRegNO() << "_"
                         << ssaVit->second->GetVersionIdx() << " class: " << classes[regType] << " validBitNum: ["
                         << static_cast<uint32>(vRegOpnd.GetValidBitsNum()) << "]";
  LogInfo::MapleLogger() << ")";
}
}
