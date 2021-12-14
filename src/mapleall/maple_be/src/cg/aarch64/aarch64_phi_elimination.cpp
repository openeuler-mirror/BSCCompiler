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
  ASSERT(destOpnd.GetSize() == fromOpnd.GetSize(), "do not support this move in aarch64");
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
  return *insn;
}

RegOperand &AArch64PhiEliminate::GetCGVirtualOpearnd(RegOperand &ssaOpnd, Insn &curInsn) {
  VRegVersion *ssaVersion = GetSSAInfo()->FindSSAVersion(ssaOpnd.GetRegisterNumber());
  ASSERT(ssaVersion != nullptr, "find ssaVersion failed");
  RegOperand *regForRecreate = &ssaOpnd;
  if (GetSSAInfo()->IsNoDefVReg(ssaOpnd.GetRegisterNumber())) {
    regForRecreate = MakeRoomForNoDefVreg(ssaOpnd);
  }
  ASSERT(regForRecreate->IsSSAForm(), "Opnd is not in ssa form");
  RegOperand &newReg = cgFunc->GetOrCreateVirtualRegisterOperand(*regForRecreate);

  Insn *defInsn = ssaVersion->GetDefInsn();
  /*
   * case1 : both def/use
   * case2 : inline-asm  ( do not do aggressive optimization )
  */
  if (defInsn != nullptr) {
    if (!defInsn->IsRegDefined(ssaOpnd.GetRegisterNumber()) &&
        !defInsn->IsRegDefined(ssaVersion->GetSSAvRegOpnd()->GetRegisterNumber())) {
      if (defInsn->IsPhi()) {
        CHECK_FATAL(false, " check this case");
      }
    }
    /* case 1*/
    MOperator mOp = defInsn->GetMachineOpcode();
    const AArch64MD *md = &AArch64CG::kMd[mOp];
    uint32 defUseIdx = kInsnMaxOpnd;
    for (uint32 i = 0; i < defInsn->GetOperandSize(); ++i) {
      auto *opndProp = static_cast<AArch64OpndProp *>(md->operand[i]);
      if (opndProp->IsRegUse() && opndProp->IsDef()) {
        defUseIdx = i;
      }
    }
    if (defUseIdx != kInsnMaxOpnd) {
      Operand &preRegOpnd = defInsn->GetOperand(defUseIdx);
      ASSERT(preRegOpnd.IsRegister(), "unexpect operand type");
      newReg.SetRegisterNumber(static_cast<RegOperand&>(preRegOpnd).GetRegisterNumber());
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
      /* find last version in asm failed */
      if (LastVersion == nullptr) {
        newReg.SetRegisterNumber(ssaVersion->GetOriginalRegNO());
      } else {
        newReg.SetRegisterNumber(LastVersion->GetSSAvRegOpnd()->GetRegisterNumber());
      }
    }
  } else {
    newReg.SetRegisterNumber(ssaVersion->GetOriginalRegNO());
  }

  MaintainRematInfo(newReg, ssaOpnd, static_cast<AArch64Insn&>(curInsn).CopyOperands() >= 0);
  newReg.SetOpndOutOfSSAForm();
  return newReg;
}

void AArch64PhiEliminate::AppendMovAfterLastVregDef(BB &bb, Insn &movInsn) const {
  Insn *posInsn = nullptr;
  Insn *firstInsn = nullptr;
  FOR_BB_INSNS_REV(insn, &bb) {
    if (insn->IsPhi()) {
      posInsn = insn;
      break;
    }
    if (!insn->IsMachineInstruction()) {
      continue;
    }
    firstInsn = insn;
    std::set<uint32> defRegs = insn->GetDefRegs();
    for (auto defReg : defRegs) {
      if (!(AArch64isa::IsPhysicalRegister(defReg) || defReg == kRFLAG)) {
        posInsn = insn;
        break;
      }
    }
    if (posInsn != nullptr) {
      break;
    }
    DoRegLiveRangeOpt(*insn, movInsn);
  }
  if (posInsn == nullptr) {
    if (firstInsn == nullptr) { /* empty BB */
      bb.AppendInsn(movInsn);
    } else {
      bb.InsertInsnBefore(*firstInsn, movInsn);
    }
  } else {
    bb.InsertInsnAfter(*posInsn, movInsn);
  }
}

void AArch64PhiEliminate::DoRegLiveRangeOpt(Insn &insn, Insn &movInsn) const {
  Operand &exisitReg = movInsn.GetOperand(kInsnSecondOpnd);
  ASSERT(exisitReg.IsRegister(), "second operand of mov must be register");
  auto &curInsn = static_cast<AArch64Insn&>(insn);
  ASSERT(!curInsn.IsRegDefined(static_cast<RegOperand&>(exisitReg).GetRegisterNumber()), "cannot have def here!!!");
  /* replace register type */
  for (int i = 0; i < curInsn.GetOperandSize(); ++i) {
    if (curInsn.GetOperand(i).IsRegister()) {
      auto &regOpnd = static_cast<RegOperand&>(curInsn.GetOperand(i));
      if (regOpnd.GetRegisterNumber() == static_cast<RegOperand&>(exisitReg).GetRegisterNumber()) {
        insn.SetOperand(i, movInsn.GetOperand(kInsnFirstOpnd));
      }
    }
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
        RecordRematInfo(destOpnd.GetRegisterNumber(), fPreg->GetPregNo());
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

void AArch64PhiEliminate::ReCreateListOperand(ListOperand &lOpnd, Insn &curInsn) {
  std::list<RegOperand*> tempRegStore;
  for (auto *regOpnd : lOpnd.GetOperands()) {
    if (regOpnd->IsSSAForm()) {
      tempRegStore.push_back(&GetCGVirtualOpearnd(*regOpnd, curInsn));
    } else {
      tempRegStore.push_back(regOpnd);
    }
    lOpnd.RemoveOpnd(*regOpnd);
  }
  ASSERT(lOpnd.GetOperands().empty(), "need to clean list");
  lOpnd.GetOperands().assign(tempRegStore.begin(), tempRegStore.end());
}

void AArch64PhiEliminate::ReCreateRegOperand(Insn &insn) {
  auto opndNum = static_cast<int32>(insn.GetOperandSize());
  for (int i = opndNum - 1; i >= 0; --i) {
    Operand &opnd = insn.GetOperand(i);
    if (opnd.IsList()) {
      ReCreateListOperand(static_cast<AArch64ListOperand&>(opnd), insn);
    } else if (opnd.IsMemoryAccessOperand()) {
      auto &memOpnd = static_cast<AArch64MemOperand&>(opnd);
      RegOperand *baseRegOpnd = memOpnd.GetBaseRegister();
      RegOperand *indexRegOpnd = memOpnd.GetIndexRegister();
      if ((baseRegOpnd != nullptr && baseRegOpnd->IsSSAForm()) ||
          (indexRegOpnd != nullptr && indexRegOpnd->IsSSAForm())) {
        if (baseRegOpnd != nullptr && baseRegOpnd->IsSSAForm()) {
          memOpnd.SetBaseRegister(static_cast<AArch64RegOperand&>(GetCGVirtualOpearnd(*baseRegOpnd, insn)));
        }
        if (indexRegOpnd != nullptr && indexRegOpnd->IsSSAForm()) {
          memOpnd.SetIndexRegister(GetCGVirtualOpearnd(*indexRegOpnd, insn));
        }
      }
    } else if (opnd.IsRegister()) {
      auto &regOpnd = static_cast<RegOperand&>(opnd);
      if (regOpnd.IsSSAForm()) {
        ASSERT(regOpnd.GetRegisterNumber() != kRFLAG, "both condi and reg");
        insn.SetOperand(i, GetCGVirtualOpearnd(regOpnd, insn));
      }
    } else if (opnd.IsPhi()) {
      for (auto phiOpndIt : static_cast<AArch64PhiOperand&>(opnd).GetOperands()) {
        if (phiOpndIt.second->IsSSAForm()) {
          static_cast<AArch64PhiOperand&>(opnd).GetOperands()[phiOpndIt.first] =
              &GetCGVirtualOpearnd(*(phiOpndIt.second), insn);
        }
      }
    } else {
      /* unsupport ssa opnd */
    }
  }
}
}
