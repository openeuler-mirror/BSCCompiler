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
Insn &AArch64PhiEliminate::CreateMoveCopyRematInfo(RegOperand &destOpnd, RegOperand &fromOpnd) const {
  ASSERT(destOpnd.GetSize() == fromOpnd.GetSize(), "do not support this move in aarch64");
  ASSERT(destOpnd.GetRegisterType() == fromOpnd.GetRegisterType(), "do not support this move in aarch64");
  bool is64bit = destOpnd.GetSize() == k64BitSize;
  bool isFloat = destOpnd.IsOfFloatOrSIMDClass();
  Insn *insn = nullptr;
  if (destOpnd.GetSize() == k128BitSize) {
    ASSERT(isFloat, "unexpect 128bit int operand in aarch64");
    insn = &cgFunc->GetCG()->BuildInstruction<AArch64VectorInsn>(MOP_vmovvv, destOpnd, fromOpnd);
    VectorRegSpec *vecSpecSrc = cgFunc->GetMemoryPool()->New<VectorRegSpec>(k128BitSize >> k3ByteSize, k8BitSize);
    VectorRegSpec *vecSpecDest = cgFunc->GetMemoryPool()->New<VectorRegSpec>(k128BitSize >> k3ByteSize, k8BitSize);
    static_cast<AArch64VectorInsn*>(insn)->PushRegSpecEntry(vecSpecDest);
    static_cast<AArch64VectorInsn*>(insn)->PushRegSpecEntry(vecSpecSrc);
  } else {
    insn = &cgFunc->GetCG()->BuildInstruction<AArch64Insn>(
        is64bit ? isFloat ? MOP_xvmovd : MOP_xmovrr : isFloat ? MOP_xvmovs : MOP_wmovrr, destOpnd, fromOpnd);
  }
  /* copy remat info */
  if (CGOptions::GetRematLevel() > 0) {
    MIRPreg *fPreg = static_cast<AArch64CGFunc*>(cgFunc)->GetPseudoRegFromVirtualRegNO(fromOpnd.GetRegisterNumber());
    MIRPreg *tPreg = static_cast<AArch64CGFunc*>(cgFunc)->GetPseudoRegFromVirtualRegNO(destOpnd.GetRegisterNumber());
    if (fPreg != nullptr && tPreg == nullptr) {
      // Create a new vreg/preg for the upper bits of the address
      PregIdx pregIdx = cgFunc->GetFunction().GetPregTab()->ClonePreg(*fPreg);
      // Register this vreg mapping
      cgFunc->RegisterVregMapping(destOpnd.GetRegisterNumber(), pregIdx);
    }
  }
  ASSERT(insn != nullptr, "create move insn failed");
  return *insn;
}

RegOperand &AArch64PhiEliminate::GetCGVirtualOpearnd(RegOperand &ssaOpnd) {
  VRegVersion *ssaVersion = GetSSAInfo()->FindSSAVersion(ssaOpnd.GetRegisterNumber());
  RegOperand *tempReg = static_cast<RegOperand*>(ssaOpnd.Clone(*phiEliAlloc.GetMemPool()));
  tempReg->SetRegisterNumber(ssaVersion->GetOriginalRegNO());
  RegOperand &newReg = cgFunc->GetOrCreateVirtualRegisterOperand(*tempReg);
  newReg.SetOpndOutOfSSAForm();
  return newReg;
}

void AArch64PhiEliminate::AppendMovAfterLastVregDef(BB &bb, Insn &movInsn) const {
  Insn *posInsn = nullptr;
  Insn *firstInsn = nullptr;
  FOR_BB_INSNS_REV(insn, &bb) {
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
  }
  if (posInsn == nullptr) {
    CHECK_FATAL(firstInsn != nullptr, "dont have insn in BB");
    bb.InsertInsnBefore(*firstInsn, movInsn);
  } else {
    bb.InsertInsnAfter(*posInsn, movInsn);
  }
}

void AArch64PhiEliminate::ReCreateListOperand(ListOperand &lOpnd) {
  std::list<RegOperand*> tempRegStore;
  for (auto *regOpnd : lOpnd.GetOperands()) {
    if (regOpnd->IsSSAForm()) {
      tempRegStore.push_back(&GetCGVirtualOpearnd(*regOpnd));
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
      ReCreateListOperand(static_cast<AArch64ListOperand&>(opnd));
    } else if (opnd.IsMemoryAccessOperand()) {
      auto &memOpnd = static_cast<AArch64MemOperand&>(opnd);
      RegOperand *baseRegOpnd = memOpnd.GetBaseRegister();
      RegOperand *indexRegOpnd = memOpnd.GetIndexRegister();
      if ((baseRegOpnd != nullptr && baseRegOpnd->IsSSAForm()) ||
          (indexRegOpnd != nullptr && indexRegOpnd->IsSSAForm())) {
        if (baseRegOpnd != nullptr && baseRegOpnd->IsSSAForm()) {
          memOpnd.SetBaseRegister(static_cast<AArch64RegOperand&>(GetCGVirtualOpearnd(*baseRegOpnd)));
        }
        if (indexRegOpnd != nullptr && indexRegOpnd->IsSSAForm()) {
          memOpnd.SetIndexRegister(GetCGVirtualOpearnd(*indexRegOpnd));
        }
        insn.SetMemOpnd(&static_cast<AArch64CGFunc*>(cgFunc)->GetOrCreateMemOpnd(memOpnd));
      }
    } else if (opnd.IsRegister()) {
      auto &regOpnd = static_cast<RegOperand&>(opnd);
      if (regOpnd.IsSSAForm()) {
        ASSERT(regOpnd.GetRegisterNumber() != kRFLAG, "both condi and reg");
        insn.SetOperand(i, GetCGVirtualOpearnd(regOpnd));
      }
    } else if (opnd.IsPhi()) {
      for (auto phiOpndIt : static_cast<AArch64PhiOperand&>(opnd).GetOperands()) {
        if (phiOpndIt.second->IsSSAForm()) {
          static_cast<AArch64PhiOperand&>(opnd).GetOperands()[phiOpndIt.first] =
              &GetCGVirtualOpearnd(*(phiOpndIt.second));
        }
      }
    } else {
        /* unsupport ssa opnd */
    }
  }
}
}