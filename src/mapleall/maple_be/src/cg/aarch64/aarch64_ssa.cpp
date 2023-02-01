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
  const InsnDesc *md = insn.GetDesc();
  if (md->IsPhi()) {
    return;
  }
  for (int i = opndNum - 1; i >= 0; --i) {
    Operand &opnd = insn.GetOperand(static_cast<uint32>(i));
    auto *opndProp = (md->opndMD[static_cast<uint32>(i)]);
    A64SSAOperandRenameVisitor renameVisitor(*this, insn, *opndProp, i);
    opnd.Accept(renameVisitor);
  }
}

MemOperand *AArch64CGSSAInfo::CreateMemOperand(MemOperand &memOpnd, bool isOnSSA) const {
  return isOnSSA ? memOpnd.Clone(*cgFunc->GetMemoryPool()) : &memOpnd;
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
  regno_t ssaRegNO = static_cast<regno_t>(GetAllSSAOperands().size()) + ssaRegNObase;
  while (GetAllSSAOperands().count(ssaRegNO) != 0) {
    ssaRegNO++;
    ssaRegNObase++;
  }
  RegOperand *newVreg = memPool->New<RegOperand>(ssaRegNO,
      virtualOpnd.GetSize(), virtualOpnd.GetRegisterType());
  newVreg->SetOpndSSAForm();
  return newVreg;
}

void AArch64CGSSAInfo::ReplaceInsn(Insn &oriInsn, Insn &newInsn) {
  A64OpndSSAUpdateVsitor ssaUpdator(*this);
  auto updateInsnSSAInfo = [&ssaUpdator](Insn &curInsn, bool isDelete) {
    const InsnDesc *md = curInsn.GetDesc();
    for (uint32 i = 0; i < curInsn.GetOperandSize(); ++i) {
      Operand &opnd = curInsn.GetOperand(i);
      auto *opndProp = md->opndMD[i];
      if (isDelete) {
        ssaUpdator.MarkDecrease();
      } else {
        ssaUpdator.MarkIncrease();
      }
      ssaUpdator.SetInsnOpndInfo(curInsn, *opndProp, i);
      opnd.Accept(ssaUpdator);
    }
  };
  updateInsnSSAInfo(oriInsn, true);
  newInsn.SetId(oriInsn.GetId());
  updateInsnSSAInfo(newInsn, false);
  CHECK_FATAL(!ssaUpdator.HasDeleteDef(), "delete def point in replace insn, please check");
}

/* do not break binding between input and output operands in asm */
void AArch64CGSSAInfo::CheckAsmDUbinding(Insn &insn, const VRegVersion &toBeReplaced, VRegVersion &newVersion) {
  if (insn.GetMachineOpcode() == MOP_asm) {
    for (const auto &opndIt : static_cast<ListOperand&>(insn.GetOperand(kAsmOutputListOpnd)).GetOperands()) {
      if (opndIt->IsSSAForm()) {
        VRegVersion *defVersion = FindSSAVersion(opndIt->GetRegisterNumber());
        if (defVersion && defVersion->GetOriginalRegNO() == toBeReplaced.GetOriginalRegNO()) {
          insn.AddRegBinding(defVersion->GetOriginalRegNO(), newVersion.GetSSAvRegOpnd()->GetRegisterNumber());
        }
      }
    }
  }
}

void AArch64CGSSAInfo::ReplaceAllUse(VRegVersion *toBeReplaced, VRegVersion *newVersion) {
  MapleUnorderedMap<uint32, DUInsnInfo*> &useList = toBeReplaced->GetAllUseInsns();
  for (auto it = useList.begin(); it != useList.end();) {
    Insn *useInsn = it->second->GetInsn();
    CheckAsmDUbinding(*useInsn, *toBeReplaced, *newVersion);
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

void AArch64CGSSAInfo::CreateNewInsnSSAInfo(Insn &newInsn) {
  uint32 opndNum = newInsn.GetOperandSize();
  MarkInsnsInSSA(newInsn);
  for (uint32 i = 0; i < opndNum; i++) {
    Operand &opnd = newInsn.GetOperand(i);
    auto *opndProp = newInsn.GetDesc()->opndMD[i];
    if (opndProp->IsDef() && opndProp->IsUse()) {
      CHECK_FATAL(false, "do not support both def and use");
    }
    if (opndProp->IsDef()) {
      CHECK_FATAL(opnd.IsRegister(), "defOpnd must be reg");
      auto &defRegOpnd = static_cast<RegOperand&>(opnd);
      regno_t defRegNO = defRegOpnd.GetRegisterNumber();
      uint32 defVIdx = IncreaseVregCount(defRegNO);
      RegOperand *defSSAOpnd = CreateSSAOperand(defRegOpnd);
      newInsn.SetOperand(i, *defSSAOpnd);
      auto *defVersion = memPool->New<VRegVersion>(ssaAlloc, *defSSAOpnd, defVIdx, defRegNO);
      auto *defInfo = CreateDUInsnInfo(&newInsn, i);
      defVersion->SetDefInsn(defInfo, kDefByInsn);
      if (!IncreaseSSAOperand(defSSAOpnd->GetRegisterNumber(), defVersion)) {
        CHECK_FATAL(false, "insert ssa operand failed");
      }
      uint32 curSSAVregCount = cgFunc->GetSSAvRegCount();
      cgFunc->SetSSAvRegCount(++curSSAVregCount);
    } else if (opndProp->IsUse()) {
      A64OpndSSAUpdateVsitor ssaUpdator(*this);
      ssaUpdator.MarkIncrease();
      ssaUpdator.SetInsnOpndInfo(newInsn, *opndProp, i);
      opnd.Accept(ssaUpdator);
    }
  }
}

void AArch64CGSSAInfo::DumpInsnInSSAForm(const Insn &insn) const {
  MOperator mOp = insn.GetMachineOpcode();
  const InsnDesc *md = insn.GetDesc();
  ASSERT(md != nullptr, "md should not be nullptr");

  LogInfo::MapleLogger() << "< " << insn.GetId() << " > ";
  LogInfo::MapleLogger() << md->name << "(" << mOp << ")";

  for (uint32 i = 0; i < insn.GetOperandSize(); ++i) {
    Operand &opnd = insn.GetOperand(i);
    LogInfo::MapleLogger() << " (opnd" << i << ": ";
    A64SSAOperandDumpVisitor a64OpVisitor(GetAllSSAOperands());
    opnd.Accept(a64OpVisitor);
    if (!a64OpVisitor.HasDumped()) {
      A64OpndDumpVisitor dumpVisitor(*md->GetOpndDes(i));
      opnd.Accept(dumpVisitor);
      LogInfo::MapleLogger() << ")";
    }
  }
  if (insn.GetNumOfRegSpec() != 0) {
    LogInfo::MapleLogger() << " (vecSpec: " << insn.GetNumOfRegSpec() << ")";
  }
  LogInfo::MapleLogger() << "\n";
}

void A64SSAOperandRenameVisitor::Visit(RegOperand *v) {
  if (v->IsVirtualRegister()) {
    if (opndDes->IsRegDef() && opndDes->IsRegUse()) {        /* both def use */
      insn->SetOperand(idx, *ssaInfo->GetRenamedOperand(*v, false, *insn, idx));
      RegOperand *ssaDefOpnd = ssaInfo->GetRenamedOperand(*v, true, *insn, idx);
      insn->SetSSAImpDefOpnd(ssaDefOpnd);
    } else {
      insn->SetOperand(idx, *ssaInfo->GetRenamedOperand(*v, opndDes->IsRegDef(), *insn, idx));
    }
  }
}

void A64SSAOperandRenameVisitor::Visit(MemOperand *v) {
  RegOperand *base = v->GetBaseRegister();
  RegOperand *index = v->GetIndexRegister();
  bool needCopy = (base != nullptr && base->IsVirtualRegister()) || (index != nullptr && index->IsVirtualRegister());
  if (needCopy) {
    MemOperand *cpyMem = ssaInfo->CreateMemOperand(*v, true);
    if (base != nullptr && base->IsVirtualRegister()) {
      bool isDef = !v->IsIntactIndexed();
      cpyMem->SetBaseRegister(*ssaInfo->GetRenamedOperand(*base, isDef, *insn, idx));
    }
    if (index != nullptr && index->IsVirtualRegister()) {
      cpyMem->SetIndexRegister(*ssaInfo->GetRenamedOperand(*index, false, *insn, idx));
    }
    insn->SetMemOpnd(ssaInfo->CreateMemOperand(*cpyMem, false));
  }
}

void A64SSAOperandRenameVisitor::Visit(ListOperand *v) {
  bool isAsm = insn->GetMachineOpcode() == MOP_asm;
  /* record the orignal list order */
  std::list<RegOperand*> tempList;
  auto& opndList = v->GetOperands();
  while (!opndList.empty()) {
    auto* op = opndList.front();
    opndList.pop_front();

    if (op->IsSSAForm() || !op->IsVirtualRegister()) {
      tempList.push_back(op);
      continue;
    }

    bool isDef =
        isAsm && (idx == kAsmClobberListOpnd || idx == kAsmOutputListOpnd);
    RegOperand *renameOpnd = ssaInfo->GetRenamedOperand(*op, isDef, *insn, idx);
    tempList.push_back(renameOpnd);
  }
  ASSERT(v->GetOperands().empty(), "need to clean list");
  v->GetOperands().assign(tempList.begin(), tempList.end());
}

void A64OpndSSAUpdateVsitor::Visit(RegOperand *v) {
  if (v->IsSSAForm()) {
    if (opndDes->IsRegDef() && opndDes->IsRegUse()) {
      UpdateRegUse(v->GetRegisterNumber());
      ASSERT(insn->GetSSAImpDefOpnd(), "must be");
      UpdateRegDef(insn->GetSSAImpDefOpnd()->GetRegisterNumber());
    } else {
      if (opndDes->IsRegDef()) {
        UpdateRegDef(v->GetRegisterNumber());
      } else if (opndDes->IsRegUse()) {
        UpdateRegUse(v->GetRegisterNumber());
      } else if (IsPhi()) {
        UpdateRegUse(v->GetRegisterNumber());
      } else {
        ASSERT(false, "invalid opnd");
      }
    }
  }
}

void A64OpndSSAUpdateVsitor::Visit(maplebe::MemOperand *v) {
  RegOperand *base = v->GetBaseRegister();
  RegOperand *index = v->GetIndexRegister();
  if (base != nullptr && base->IsSSAForm()) {
    if (v->IsIntactIndexed()) {
      UpdateRegUse(base->GetRegisterNumber());
    } else {
      UpdateRegDef(base->GetRegisterNumber());
    }
  }
  if (index != nullptr && index->IsSSAForm()) {
    UpdateRegUse(index->GetRegisterNumber());
  }
}

void A64OpndSSAUpdateVsitor::Visit(PhiOperand *v) {
  SetPhi(true);
  for (auto phiListIt = v->GetOperands().cbegin(); phiListIt != v->GetOperands().cend(); ++phiListIt) {
    Visit(phiListIt->second);
  }
  SetPhi(false);
}

void A64OpndSSAUpdateVsitor::Visit(ListOperand *v) {
  /* do not handle asm here, so there is no list def */
  if (insn->GetMachineOpcode() == MOP_asm) {
    ASSERT(false, "do not support asm yet");
    return;
  }
  for (const auto *op : v->GetOperands()) {
    if (op->IsSSAForm()) {
      UpdateRegUse(op->GetRegisterNumber());
    }
  }
}

void A64OpndSSAUpdateVsitor::UpdateRegUse(uint32 ssaIdx) const {
  VRegVersion *curVersion = ssaInfo->FindSSAVersion(ssaIdx);
  CHECK_NULL_FATAL(curVersion);
  if (isDecrease) {
    curVersion->RemoveUseInsn(*insn, idx);
  } else {
    curVersion->AddUseInsn(*ssaInfo, *insn, idx);
  }
}

void A64OpndSSAUpdateVsitor::UpdateRegDef(uint32 ssaIdx) {
  VRegVersion *curVersion = ssaInfo->FindSSAVersion(ssaIdx);
  CHECK_NULL_FATAL(curVersion);
  if (isDecrease) {
    deletedDef.emplace(ssaIdx);
    curVersion->MarkDeleted();
  } else {
    if (deletedDef.count(ssaIdx) != 0) {
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
  ASSERT(!v->IsConditionCode(), "both condi and reg");
  if (v->IsSSAForm()) {
    std::array<const std::string, kRegTyLast> prims = { "U", "R", "V", "C", "X", "Vra" };
    std::array<const std::string, kRegTyLast> classes = { "[U]", "[I]", "[F]", "[CC]", "[X87]", "[Vra]" };
    CHECK_FATAL(v->IsVirtualRegister() && v->IsSSAForm(), "only dump ssa opnd here");
    RegType regType = v->GetRegisterType();
    ASSERT(regType < kRegTyLast, "unexpected regType");
    auto ssaVit = allSSAOperands.find(v->GetRegisterNumber());
    CHECK_FATAL(ssaVit != allSSAOperands.end(), "find ssa version failed");
    LogInfo::MapleLogger() << "ssa_reg:" << prims[regType] << ssaVit->second->GetOriginalRegNO() << "_"
                           << ssaVit->second->GetVersionIdx() << " class: " << classes[regType] << " validBitNum: ["
                           << static_cast<uint32>(v->GetValidBitsNum()) << "]";
    LogInfo::MapleLogger() << ")";
    SetHasDumped();
  }
}

void A64SSAOperandDumpVisitor::Visit(ListOperand *v) {
  for (const auto regOpnd : v->GetOperands()) {
    if (regOpnd->IsSSAForm()) {
      Visit(regOpnd);
      continue;
    }
  }
}

void A64SSAOperandDumpVisitor::Visit(MemOperand *v) {
  if (v->GetBaseRegister() != nullptr && v->GetBaseRegister()->IsSSAForm()) {
    LogInfo::MapleLogger() << "Mem: ";
    Visit(v->GetBaseRegister());
    if (v->GetAddrMode() == MemOperand::kBOI) {
      LogInfo::MapleLogger() << "offset:";
      v->GetOffsetOperand()->Dump();
    }
  }
  if (v->GetIndexRegister() != nullptr && v->GetIndexRegister()->IsSSAForm()) {
    ASSERT(v->GetAddrMode() == MemOperand::kBOR, "mem mode false");
    LogInfo::MapleLogger() << "offset:";
    Visit(v->GetIndexRegister());
  }
}

void A64SSAOperandDumpVisitor::Visit(PhiOperand *v) {
  for (auto phiListIt = v->GetOperands().cbegin(); phiListIt != v->GetOperands().cend();) {
    Visit(phiListIt->second);
    LogInfo::MapleLogger() << " fBB<" << phiListIt->first << ">";
    LogInfo::MapleLogger() << (++phiListIt == v->GetOperands().end() ? ")" : ", ");
  }
}
}
