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
#include "aarch64_phi_elimination.h"
#include "aarch64_cg.h"

namespace maplebe {
RegOperand &AArch64PhiEliminate::CreateTempRegForCSSA(RegOperand &oriOpnd) {
  return *phiEliAlloc.New<AArch64RegOperand>(GetAndIncreaseTempRegNO(), oriOpnd.GetSize(), oriOpnd.GetRegisterType());
}

Insn &AArch64PhiEliminate::CreateMov(RegOperand &destOpnd, RegOperand &fromOpnd) {
  ASSERT(destOpnd.GetRegisterType() == fromOpnd.GetRegisterType(), "do not support this move in aarch64");
  bool is64bit = destOpnd.GetSize() == k64BitSize;
  bool isFloat = destOpnd.IsOfFloatOrSIMDClass();
  Insn *insn = nullptr;
  if (destOpnd.GetSize() == k128BitSize) {
    ASSERT(isFloat, "unexpect 128bit int operand in aarch64");
    insn = &cgFunc->GetCG()->BuildInstruction<AArch64VectorInsn>(MOP_vmovvv, destOpnd, fromOpnd);
    auto *vecSpecSrc = cgFunc->GetMemoryPool()->New<VectorRegSpec>(k128BitSize >> k3ByteSize, k8BitSize);
    auto *vecSpecDest = cgFunc->GetMemoryPool()->New<VectorRegSpec>(k128BitSize >> k3ByteSize, k8BitSize);
    static_cast<AArch64VectorInsn*>(insn)->PushRegSpecEntry(vecSpecDest);
    static_cast<AArch64VectorInsn*>(insn)->PushRegSpecEntry(vecSpecSrc);
  } else {
    insn = &cgFunc->GetCG()->BuildInstruction<AArch64Insn>(
        is64bit ? isFloat ? MOP_xvmovd : MOP_xmovrr : isFloat ? MOP_xvmovs : MOP_wmovrr, destOpnd, fromOpnd);
  }
  /* copy remat info */
  MaintainRematInfo(destOpnd, fromOpnd, true);
  ASSERT(insn != nullptr, "create move insn failed");
  insn->SetIsPhiMovInsn(true);
  return *insn;
}

RegOperand &AArch64PhiEliminate::GetCGVirtualOpearnd(RegOperand &ssaOpnd, const Insn &curInsn) {
  VRegVersion *ssaVersion = GetSSAInfo()->FindSSAVersion(ssaOpnd.GetRegisterNumber());
  ASSERT(ssaVersion != nullptr, "find ssaVersion failed");
  ASSERT(!ssaVersion->IsDeleted(), "ssaVersion has been deleted");
  RegOperand *regForRecreate = &ssaOpnd;
  if (curInsn.GetMachineOpcode() != MOP_asm && !curInsn.IsVectorOp() && ssaVersion->GetAllUseInsns().empty()) {
    CHECK_FATAL(false, "plz delete dead version");
  }
  if (GetSSAInfo()->IsNoDefVReg(ssaOpnd.GetRegisterNumber())) {
    regForRecreate = MakeRoomForNoDefVreg(ssaOpnd);
  } else {
    ASSERT(regForRecreate->IsSSAForm(), "Opnd is not in ssa form");
  }
  RegOperand &newReg = cgFunc->GetOrCreateVirtualRegisterOperand(*regForRecreate);

  DUInsnInfo *defInfo = ssaVersion->GetDefInsnInfo();
  Insn *defInsn = defInfo != nullptr ? defInfo->GetInsn() : nullptr;
  /*
   * case1 : both def/use
   * case2 : inline-asm  (do not do aggressive optimization) "0"
   * case3 : cc flag operand
   */
  if (defInsn != nullptr) {
    /* case 1*/
    uint32 defUseIdx = defInsn->GetBothDefUseOpnd();
    if (defUseIdx != kInsnMaxOpnd) {
      if (defInfo->GetOperands().count(defUseIdx)) {
        CHECK_FATAL(defInfo->GetOperands()[defUseIdx] == 1, "multiple definiation");
        Operand &preRegOpnd = defInsn->GetOperand(defUseIdx);
        ASSERT(preRegOpnd.IsRegister(), "unexpect operand type");
        newReg.SetRegisterNumber(static_cast<RegOperand&>(preRegOpnd).GetRegisterNumber());
      }
    }
    /* case 2 */
    if (defInsn->GetMachineOpcode() == MOP_asm) {
      auto &inputList = static_cast<ListOperand&>(defInsn->GetOperand(kAsmInputListOpnd));
      VRegVersion *LastVersion = nullptr;
      for (auto inputReg : inputList.GetOperands()) {
        LastVersion = GetSSAInfo()->FindSSAVersion(inputReg->GetRegisterNumber());
        if (LastVersion != nullptr && LastVersion->GetOriginalRegNO() == ssaVersion->GetOriginalRegNO()) {
          break;
        }
        LastVersion = nullptr;
      }
      if (LastVersion != nullptr) {
        newReg.SetRegisterNumber(LastVersion->GetSSAvRegOpnd()->GetRegisterNumber());
      }
    }
    /* case 3 */
    if (ssaVersion->GetOriginalRegNO() == kRFLAG) {
      newReg.SetRegisterNumber(kRFLAG);
    }
  } else {
    newReg.SetRegisterNumber(ssaVersion->GetOriginalRegNO());
  }

  MaintainRematInfo(newReg, ssaOpnd, true);
  newReg.SetOpndOutOfSSAForm();
  return newReg;
}

void AArch64PhiEliminate::AppendMovAfterLastVregDef(BB &bb, Insn &movInsn) const {
  Insn *posInsn = nullptr;
  bool isPosPhi = false;
  FOR_BB_INSNS_REV(insn, &bb) {
    if (insn->IsPhi()) {
      posInsn = insn;
      isPosPhi = true;
      break;
    }
    if (!insn->IsMachineInstruction()) {
      continue;
    }
    if (insn->IsBranch()) {
      posInsn = insn;
      continue;
    }
    break;
  }
  CHECK_FATAL(posInsn != nullptr, "insert mov for phi failed");
  if (isPosPhi) {
    bb.InsertInsnAfter(*posInsn, movInsn);
  } else {
    bb.InsertInsnBefore(*posInsn, movInsn);
  }
}

/* copy remat info */
void AArch64PhiEliminate::MaintainRematInfo(RegOperand &destOpnd, RegOperand &fromOpnd, bool isCopy) {
  if (CGOptions::GetRematLevel() > 0 && isCopy) {
    if (fromOpnd.IsSSAForm()) {
      VRegVersion *fromSSAVersion = GetSSAInfo()->FindSSAVersion(fromOpnd.GetRegisterNumber());
      regno_t rematRegNO = fromSSAVersion->GetOriginalRegNO();
      MIRPreg *fPreg = static_cast<AArch64CGFunc*>(cgFunc)->GetPseudoRegFromVirtualRegNO(rematRegNO);
      if (fPreg != nullptr) {
        PregIdx fPregIdx = cgFunc->GetFunction().GetPregTab()->GetPregIdxFromPregno(fPreg->GetPregNo());
        RecordRematInfo(destOpnd.GetRegisterNumber(), fPregIdx);
      }
    } else {
      regno_t rematRegNO = fromOpnd.GetRegisterNumber();
      PregIdx fPreg = FindRematInfo(rematRegNO);
      if (fPreg > 0) {
        RecordRematInfo(destOpnd.GetRegisterNumber(), fPreg);
      }
    }
  }
}

void AArch64PhiEliminate::ReCreateRegOperand(Insn &insn) {
  auto opndNum = static_cast<int32>(insn.GetOperandSize());
  for (int i = opndNum - 1; i >= 0; --i) {
    Operand &opnd = insn.GetOperand(static_cast<uint32>(i));
    A64OperandPhiElmVisitor a64OpndPhiElmVisitor(this, insn, i);
    opnd.Accept(a64OpndPhiElmVisitor);
  }
}

void A64OperandPhiElmVisitor::Visit(RegOperand *v) {
  auto *a64RegOpnd = static_cast<AArch64RegOperand*>(v);
  if (a64RegOpnd->IsSSAForm()) {
    ASSERT(a64RegOpnd->GetRegisterNumber() != kRFLAG, "both condi and reg");
    insn->SetOperand(idx, a64PhiEliminator->GetCGVirtualOpearnd(*a64RegOpnd, *insn));
  }
}

void A64OperandPhiElmVisitor::Visit(ListOperand *v) {
  std::list<RegOperand*> tempRegStore;
  auto& opndList = v->GetOperands();

  while (!opndList.empty()) {
    auto *regOpnd = opndList.front();
    opndList.pop_front();

    if (regOpnd->IsSSAForm()) {
      tempRegStore.push_back(&a64PhiEliminator->GetCGVirtualOpearnd(*regOpnd, *insn));
    } else {
      tempRegStore.push_back(regOpnd);
    }
  }

  ASSERT(v->GetOperands().empty(), "need to clean list");
  v->GetOperands().assign(tempRegStore.begin(), tempRegStore.end());
}

void A64OperandPhiElmVisitor::Visit(MemOperand *v) {
  auto *a64MemOpnd = static_cast<AArch64MemOperand*>(v);
  RegOperand *baseRegOpnd = a64MemOpnd->GetBaseRegister();
  RegOperand *indexRegOpnd = a64MemOpnd->GetIndexRegister();
  if ((baseRegOpnd != nullptr && baseRegOpnd->IsSSAForm()) ||
      (indexRegOpnd != nullptr && indexRegOpnd->IsSSAForm())) {
    if (baseRegOpnd != nullptr && baseRegOpnd->IsSSAForm()) {
      a64MemOpnd->SetBaseRegister(
          static_cast<AArch64RegOperand&>(a64PhiEliminator->GetCGVirtualOpearnd(*baseRegOpnd, *insn)));
    }
    if (indexRegOpnd != nullptr && indexRegOpnd->IsSSAForm()) {
      a64MemOpnd->SetIndexRegister(a64PhiEliminator->GetCGVirtualOpearnd(*indexRegOpnd, *insn));
    }
  }
}
}
