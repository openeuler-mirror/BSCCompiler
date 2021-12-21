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
    Operand &opnd = insn.GetOperand(i);
    auto *opndProp = static_cast<AArch64OpndProp*>(md->operand[i]);
    A64SSAOperandRenameVisitor renameVisitor(*this, insn, *opndProp, i);
    opnd.Accept(renameVisitor);
  }
}

AArch64MemOperand *AArch64CGSSAInfo::CreateMemOperand(AArch64MemOperand &memOpnd, bool isOnSSA) {
  return isOnSSA ? static_cast<AArch64MemOperand*>(memOpnd.Clone(*memPool)) :
      &static_cast<AArch64CGFunc*>(cgFunc)->GetOrCreateMemOpnd(memOpnd);
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
  if (regOpnd->GetRegisterNumber() != kRFLAG && regOpnd->IsVirtualRegister()) {
    if (a64OpndProp->IsRegDef() && a64OpndProp->IsRegUse()) {        /* both def use */
      insn->SetOperand(idx, *ssaInfo->GetRenamedOperand(*regOpnd, false, *insn));
      (void)ssaInfo->GetRenamedOperand(*regOpnd, true, *insn);
    } else {
      insn->SetOperand(idx, *ssaInfo->GetRenamedOperand(*regOpnd, a64OpndProp->IsRegDef(), *insn));
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
      copyMem->SetBaseRegister(
          *static_cast<AArch64RegOperand*>(ssaInfo->GetRenamedOperand(*base, !a64MemOpnd->IsIntactIndexed(), *insn)));
    }
    if (index != nullptr && index->IsVirtualRegister()) {
      copyMem->SetIndexRegister(*ssaInfo->GetRenamedOperand(*index, false, *insn));
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
    if (op->IsSSAForm() || op->GetRegisterNumber() == kRFLAG || !op->IsVirtualRegister()) {
      a64ListOpnd->RemoveOpnd(*op);
      tempList.push_back(op);
      continue;
    }
    a64ListOpnd->RemoveOpnd(*op);
    bool isDef = isAsm ? idx == kAsmClobberListOpnd || idx == kAsmOutputListOpnd : false;
    RegOperand *renameOpnd = ssaInfo->GetRenamedOperand(*op, isDef, *insn);
    tempList.push_back(renameOpnd);
  }
  ASSERT(a64ListOpnd->GetOperands().empty(), "need to clean list");
  a64ListOpnd->GetOperands().assign(tempList.begin(), tempList.end());
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
