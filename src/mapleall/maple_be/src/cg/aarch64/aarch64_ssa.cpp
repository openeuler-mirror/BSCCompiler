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
  for (int i = opndNum - 1; i >= 0; --i) {
    Operand &opnd = insn.GetOperand(static_cast<uint32>(i));
    auto *opndProp = static_cast<AArch64OpndProp*>(md->operand[static_cast<uint32>(i)]);
    A64SSAOperandRenameVisitor renameVisitor(*this, insn, *opndProp, i);
    opnd.Accept(renameVisitor);
  }
}

AArch64MemOperand *AArch64CGSSAInfo::CreateMemOperand(AArch64MemOperand &memOpnd, bool isOnSSA) {
  return isOnSSA ? static_cast<AArch64MemOperand*>(memOpnd.Clone(*memPool)) :
      &static_cast<AArch64CGFunc*>(cgFunc)->GetOrCreateMemOpnd(memOpnd);
}

RegOperand *AArch64CGSSAInfo::GetRenamedOperand(RegOperand &vRegOpnd, bool isDef, Insn &curInsn, uint32 idx) {
  if (vRegOpnd.IsVirtualRegister()) {
    ASSERT(!vRegOpnd.IsSSAForm(), "Unexpect ssa operand");
    if (isDef) {
      VRegVersion *newVersion = CreateNewVersion(vRegOpnd, curInsn, idx);
      CHECK_FATAL(newVersion != nullptr, "get ssa version failed");
      return newVersion->GetSSAvRegOpnd();
    } else {
      VRegVersion *curVersion = GetVersion(vRegOpnd);
      if (curVersion == nullptr) {
        curVersion = RenamedOperandSpecialCase(vRegOpnd, curInsn, idx);
      }
      curVersion->AddUseInsn(*this, curInsn, idx);
      return curVersion->GetSSAvRegOpnd();
    }
  }
  ASSERT(false, "Get Renamed operand failed");
  return nullptr;
}

VRegVersion *AArch64CGSSAInfo::RenamedOperandSpecialCase(RegOperand &vRegOpnd, Insn &curInsn, uint32 idx) {
  LogInfo::MapleLogger() << "WARNING: " << vRegOpnd.GetRegisterNumber() << " has no def info in function : "
                         << cgFunc->GetName() << " !\n";
  /* occupy operand for no def vreg */
  if (!IncreaseSSAOperand(vRegOpnd.GetRegisterNumber(), nullptr)) {
    ASSERT(GetAllSSAOperands().find(vRegOpnd.GetRegisterNumber()) != GetAllSSAOperands().end(), "should find");
    AddNoDefVReg(vRegOpnd.GetRegisterNumber());
  }
  VRegVersion *version = CreateNewVersion(vRegOpnd, curInsn, idx);
  version->SetDefInsn(nullptr, kDefByNo);
  return version;
}

RegOperand *AArch64CGSSAInfo::CreateSSAOperand(RegOperand &virtualOpnd) {
  regno_t ssaRegNO = static_cast<regno_t>(GetAllSSAOperands().size()) + SSARegNObase;
  while (GetAllSSAOperands().count(ssaRegNO)) {
    ssaRegNO++;
    SSARegNObase++;
  }
  RegOperand *newVreg = memPool->New<AArch64RegOperand>(ssaRegNO, virtualOpnd.GetSize(), virtualOpnd.GetRegisterType());
  newVreg->SetOpndSSAForm();
  return newVreg;
}

void AArch64CGSSAInfo::ReplaceInsn(Insn &oriInsn, Insn &newInsn) {
  A64OpndSSAUpdateVsitor ssaUpdator(*this);
  auto UpdateInsnSSAInfo = [&ssaUpdator](Insn &curInsn, bool isDelete) {
    const AArch64MD *md = &AArch64CG::kMd[static_cast<AArch64Insn&>(curInsn).GetMachineOpcode()];
    for (uint32 i = 0; i < curInsn.GetOperandSize(); ++i) {
      Operand &opnd = curInsn.GetOperand(i);
      auto *opndProp = static_cast<AArch64OpndProp*>(md->operand[i]);
      if (isDelete) {
        ssaUpdator.MarkDecrease();
      } else {
        ssaUpdator.MarkIncrease();
      }
      ssaUpdator.SetInsnOpndInfo(curInsn, *opndProp, i);
      opnd.Accept(ssaUpdator);
    }
  };
  UpdateInsnSSAInfo(oriInsn, true);
  newInsn.SetId(oriInsn.GetId());
  UpdateInsnSSAInfo(newInsn, false);
  CHECK_FATAL(!ssaUpdator.HasDeleteDef(), "delete def point in replace insn, please check");
}

void AArch64CGSSAInfo::ReplaceAllUse(VRegVersion *toBeReplaced, VRegVersion *newVersion) {
  MapleUnorderedMap<uint32, DUInsnInfo*> &useList = toBeReplaced->GetAllUseInsns();
  for (auto it = useList.begin(); it != useList.end();) {
    Insn *useInsn = it->second->GetInsn();
    if (useInsn->GetMachineOpcode() == MOP_asm) {
      ++it;
      continue;
    }
    for (auto &opndIt : it->second->GetOperands()) {
      Operand &opnd = useInsn->GetOperand(opndIt.first);
      A64ReplaceRegOpndVisitor replaceRegOpndVisitor(
          *cgFunc, *useInsn, opndIt.first, *toBeReplaced->GetSSAvRegOpnd(), *newVersion->GetSSAvRegOpnd());
      opnd.Accept(replaceRegOpndVisitor);
      newVersion->AddUseInsn(*this, *useInsn, opndIt.first);
      it->second->ClearDU(opndIt.first);
    }
    it = useList.erase(it);
  }
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
    A64SSAOperandDumpVisitor a64OpVisitor(GetAllSSAOperands());
    opnd.Accept(a64OpVisitor);
    if (!a64OpVisitor.HasDumped()) {
      opnd.Dump();
      LogInfo::MapleLogger() << ")";
    }
  }
  if (a64Insn.IsVectorOp()) {
    auto &vInsn = static_cast<const AArch64VectorInsn&>(insn);
    if (vInsn.GetNumOfRegSpec() != 0) {
      LogInfo::MapleLogger() << " (vecSpec: " << vInsn.GetNumOfRegSpec() << ")";
    }
  }
  LogInfo::MapleLogger() << "\n";
}

void A64SSAOperandRenameVisitor::Visit(RegOperand *v) {
  auto *regOpnd = static_cast<AArch64RegOperand*>(v);
  auto *a64OpndProp = static_cast<AArch64OpndProp*>(opndProp);
  if (regOpnd->IsVirtualRegister()) {
    if (a64OpndProp->IsRegDef() && a64OpndProp->IsRegUse()) {        /* both def use */
      insn->SetOperand(idx, *ssaInfo->GetRenamedOperand(*regOpnd, false, *insn, idx));
      (void)ssaInfo->GetRenamedOperand(*regOpnd, true, *insn, idx);
    } else {
      insn->SetOperand(idx, *ssaInfo->GetRenamedOperand(*regOpnd, a64OpndProp->IsRegDef(), *insn, idx));
    }
  }
}

void A64SSAOperandRenameVisitor::Visit(MemOperand*v) {
  auto *a64MemOpnd = static_cast<AArch64MemOperand*>(v);
  RegOperand *base = a64MemOpnd->GetBaseRegister();
  RegOperand *index = a64MemOpnd->GetIndexRegister();
  bool needCopy = (base != nullptr && base->IsVirtualRegister()) || (index != nullptr && index->IsVirtualRegister());
  if (needCopy) {
    AArch64MemOperand *copyMem = ssaInfo->CreateMemOperand(*a64MemOpnd, true);
    if (base != nullptr && base->IsVirtualRegister()) {
      bool isDef = !a64MemOpnd->IsIntactIndexed();
      copyMem->SetBaseRegister(
          *static_cast<AArch64RegOperand*>(ssaInfo->GetRenamedOperand(*base, isDef, *insn, idx)));
    }
    if (index != nullptr && index->IsVirtualRegister()) {
      copyMem->SetIndexRegister(*ssaInfo->GetRenamedOperand(*index, false, *insn, idx));
    }
    insn->SetMemOpnd(ssaInfo->CreateMemOperand(*copyMem, false));
  }
}

void A64SSAOperandRenameVisitor::Visit(ListOperand *v) {
  auto *a64ListOpnd = static_cast<AArch64ListOperand*>(v);
  bool isAsm = insn->GetMachineOpcode() == MOP_asm;
  /* record the orignal list order */
  std::list<RegOperand*> tempList;
  for (auto *op : a64ListOpnd->GetOperands()) {
    if (op->IsSSAForm() || !op->IsVirtualRegister()) {
      a64ListOpnd->RemoveOpnd(*op);
      tempList.push_back(op);
      continue;
    }
    a64ListOpnd->RemoveOpnd(*op);
    bool isDef = isAsm ? idx == kAsmClobberListOpnd || idx == kAsmOutputListOpnd : false;
    RegOperand *renameOpnd = ssaInfo->GetRenamedOperand(*op, isDef, *insn, idx);
    tempList.push_back(renameOpnd);
  }
  ASSERT(a64ListOpnd->GetOperands().empty(), "need to clean list");
  a64ListOpnd->GetOperands().assign(tempList.begin(), tempList.end());
}

void A64OpndSSAUpdateVsitor::Visit(RegOperand *v) {
  auto *regOpnd = static_cast<AArch64RegOperand*>(v);
  auto *a64OpndProp = static_cast<AArch64OpndProp*>(opndProp);
  if (regOpnd->IsSSAForm()) {
    CHECK_FATAL(!(a64OpndProp->IsRegDef() && a64OpndProp->IsRegUse()), "do not support yet");
    if (a64OpndProp->IsRegDef()){
     UpdateRegDef(regOpnd->GetRegisterNumber());
    } else if (a64OpndProp->IsRegUse()) {
      UpdateRegUse(regOpnd->GetRegisterNumber());
    } else {
      ASSERT(false, "invalid opnd");
    }
  }
}

void A64OpndSSAUpdateVsitor::Visit(maplebe::MemOperand *v) {
  auto *a64MemOpnd = static_cast<AArch64MemOperand*>(v);
  RegOperand *base = a64MemOpnd->GetBaseRegister();
  RegOperand *index = a64MemOpnd->GetIndexRegister();
  if (base != nullptr && base->IsSSAForm()) {
    if (a64MemOpnd->IsIntactIndexed()) {
      UpdateRegUse(base->GetRegisterNumber());
    } else {
      UpdateRegDef(base->GetRegisterNumber());
    }
  }
  if (index != nullptr && index->IsSSAForm()) {
    UpdateRegUse(index->GetRegisterNumber());
  }
}

void A64OpndSSAUpdateVsitor::Visit(ListOperand *v) {
  auto *a64ListOpnd = static_cast<AArch64ListOperand*>(v);
  /* do not handle asm here, so there is no list def */
  if (insn->GetMachineOpcode() == MOP_asm) {
    ASSERT(false, "do not support asm yet");
    return;
  }
  for (auto *op : a64ListOpnd->GetOperands()) {
    if (op->IsSSAForm()) {
      UpdateRegUse(op->GetRegisterNumber());
    }
  }
}

void A64OpndSSAUpdateVsitor::UpdateRegUse(uint32 ssaIdx) {
  VRegVersion *curVersion = ssaInfo->FindSSAVersion(ssaIdx);
  if (isDecrease) {
    curVersion->RemoveUseInsn(*insn, idx);
  } else {
    curVersion->AddUseInsn(*ssaInfo, *insn, idx);
  }
}

void A64OpndSSAUpdateVsitor::UpdateRegDef(uint32 ssaIdx) {
  VRegVersion *curVersion = ssaInfo->FindSSAVersion(ssaIdx);
  if (isDecrease) {
    deletedDef.emplace(ssaIdx);
    curVersion->MarkDeleted();
  } else {
    if (deletedDef.count(ssaIdx)) {
      deletedDef.erase(ssaIdx);
      curVersion->MarkRecovery();
    } else {
      CHECK_FATAL(false, "do no support new define in ssaUpdating");
    }
    ASSERT(!insn->IsPhi(), "do no support yet");
    curVersion->SetDefInsn(ssaInfo->CreateDUInsnInfo(insn, idx), kDefByInsn);
  }
}

void A64SSAOperandDumpVisitor::Visit(RegOperand *v) {
  auto *a64RegOpnd = static_cast<AArch64RegOperand*>(v);
  ASSERT(!a64RegOpnd->IsConditionCode(), "both condi and reg");
  if (a64RegOpnd->IsSSAForm()) {
    std::array<const std::string, kRegTyLast> prims = { "U", "R", "V", "C", "X", "Vra" };
    std::array<const std::string, kRegTyLast> classes = { "[U]", "[I]", "[F]", "[CC]", "[X87]", "[Vra]" };
    CHECK_FATAL(a64RegOpnd->IsVirtualRegister() && a64RegOpnd->IsSSAForm(), "only dump ssa opnd here");
    RegType regType = a64RegOpnd->GetRegisterType();
    ASSERT(regType < kRegTyLast, "unexpected regType");
    auto ssaVit = allSSAOperands.find(a64RegOpnd->GetRegisterNumber());
    CHECK_FATAL(ssaVit != allSSAOperands.end(), "find ssa version failed");
    LogInfo::MapleLogger() << "ssa_reg:" << prims[regType] << ssaVit->second->GetOriginalRegNO() << "_"
                           << ssaVit->second->GetVersionIdx() << " class: " << classes[regType] << " validBitNum: ["
                           << static_cast<uint32>(a64RegOpnd->GetValidBitsNum()) << "]";
    LogInfo::MapleLogger() << ")";
    SetHasDumped();
  }
}

void A64SSAOperandDumpVisitor::Visit(ListOperand *v) {
  auto *a64listOpnd = static_cast<AArch64ListOperand*>(v);
  for (auto regOpnd : a64listOpnd->GetOperands()) {
    if (regOpnd->IsSSAForm()) {
      Visit(regOpnd);
      continue;
    }
  }
}

void A64SSAOperandDumpVisitor::Visit(MemOperand *v) {
  auto *a64MemOpnd = static_cast<AArch64MemOperand*>(v);
  if (a64MemOpnd->GetBaseRegister() != nullptr && a64MemOpnd->GetBaseRegister()->IsSSAForm()) {
    LogInfo::MapleLogger() << "Mem: ";
    Visit(a64MemOpnd->GetBaseRegister());
    if (a64MemOpnd->GetAddrMode() == AArch64MemOperand::kAddrModeBOi) {
      LogInfo::MapleLogger() << "offset:";
      a64MemOpnd->GetOffsetOperand()->Dump();
    }
  }
  if (a64MemOpnd->GetIndexRegister() != nullptr && a64MemOpnd->GetIndexRegister()->IsSSAForm() ) {
    ASSERT(a64MemOpnd->GetAddrMode() == AArch64MemOperand::kAddrModeBOrX, "mem mode false");
    LogInfo::MapleLogger() << "offset:";
    Visit(a64MemOpnd->GetIndexRegister());
  }
}

void A64SSAOperandDumpVisitor::Visit(PhiOperand *v) {
  auto *phi = static_cast<AArch64PhiOperand*>(v);
  for (auto phiListIt = phi->GetOperands().begin(); phiListIt != phi->GetOperands().end();) {
    Visit(phiListIt->second);
    LogInfo::MapleLogger() << " fBB<" << phiListIt->first << ">";
    LogInfo::MapleLogger() << (++phiListIt == phi->GetOperands().end() ? ")" : ", ");
  }
}
}
