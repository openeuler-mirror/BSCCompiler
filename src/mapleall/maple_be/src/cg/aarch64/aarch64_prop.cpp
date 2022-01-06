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
#include "aarch64_prop.h"
#include "aarch64_isa.h"

namespace maplebe {
void AArch64Prop::ReplaceAllUse(VRegVersion *toBeReplaced, VRegVersion *newVersion) {
  MapleUnorderedMap<uint32, DUInsnInfo*> &useList = toBeReplaced->GetAllUseInsns();
  for (auto it = useList.begin(); it != useList.end();) {
    Insn *useInsn = it->second->GetInsn();
    if (useInsn->GetMachineOpcode() == MOP_asm) {
      it++;
      continue;
    }
    for (auto &opndIt : it->second->GetOperands()) {
      Operand &opnd = useInsn->GetOperand(opndIt.first);
      A64ReplaceRegOpndVisitor replaceRegOpndVisitor(*cgFunc, *useInsn,
          opndIt.first, *toBeReplaced->GetSSAvRegOpnd(), *newVersion->GetSSAvRegOpnd());
      opnd.Accept(replaceRegOpndVisitor);
      newVersion->AddUseInsn(*GetSSAInfo(), *useInsn, opndIt.first);
      it->second->ClearDU(opndIt.first);
    }
    it = useList.erase(it);
  }
}

void AArch64Prop::CopyProp(Insn &insn) {
  if (insn.IsEffectiveCopy()) {
    MOperator mOp = insn.GetMachineOpcode();
    if (mOp == MOP_xmovrr || mOp == MOP_wmovrr || mOp == MOP_xvmovs || mOp == MOP_xvmovd) {
      Operand &destOpnd = insn.GetOperand(kInsnFirstOpnd);
      Operand &srcOpnd = insn.GetOperand(kInsnSecondOpnd);
      CHECK_FATAL(destOpnd.IsRegister() && srcOpnd.IsRegister(), "must be");
      auto &destReg = static_cast<RegOperand&>(destOpnd);
      auto &srcReg = static_cast<RegOperand&>(srcOpnd);
      if (destReg.IsSSAForm() && srcReg.IsSSAForm()) {
        /* open in future */
        if (destReg.GetSize() != srcReg.GetSize()) {
          return;
        }
        if (destReg.GetValidBitsNum() != srcReg.GetValidBitsNum()) {
          CHECK_FATAL(false, "u match valid size");
        }
        VRegVersion *destVersion = GetSSAInfo()->FindSSAVersion(destReg.GetRegisterNumber());
        ASSERT(destVersion != nullptr, "find Version failed");
        VRegVersion *srcVersion = GetSSAInfo()->FindSSAVersion(srcReg.GetRegisterNumber());
        ASSERT(srcVersion != nullptr, "find Version failed");
        /* do not extend register live range at current stage */
        if (destVersion->GetOriginalRegNO() != srcVersion->GetOriginalRegNO()) {
          return;
        }
        ReplaceAllUse(destVersion, srcVersion);
      }
    }
  }
}

void AArch64Prop::TargetProp(Insn &insn) {
  A64StrLdrProp a64StrLdrProp(*memPool, *cgFunc, *GetSSAInfo(), insn);
  a64StrLdrProp.DoOpt();
}

void A64StrLdrProp::DoOpt() {
  ASSERT(curInsn != nullptr, "not input insn");
  AArch64MemOperand *currMemOpnd = StrLdrPropPreCheck(*curInsn);
  if (currMemOpnd != nullptr && memPropMode != kUndef) {
    /* can be changed to recursive propagation */
    (void)ReplaceMemOpnd(*currMemOpnd, nullptr);
    replaceVersions.clear();
  }
}

bool A64StrLdrProp::ReplaceMemOpnd(AArch64MemOperand &currMemOpnd, Insn *defInsn) {
  auto GetDefInsn =  [&](RegOperand &regOpnd, std::vector<Insn*> &allUseInsns)->void{
    if (regOpnd.IsSSAForm() && defInsn == nullptr) {
      VRegVersion *replacedV = ssaInfo->FindSSAVersion(regOpnd.GetRegisterNumber());
      if (replacedV->GetDefInsnInfo() != nullptr) {
        for (auto it : replacedV->GetAllUseInsns()) {
          allUseInsns.emplace_back(it.second->GetInsn());
        }
        defInsn = replacedV->GetDefInsnInfo()->GetInsn();
      }
    }
  };
  RegOperand *replacedReg = nullptr;
  std::vector<Insn*> allUseInsns;
  std::vector<AArch64MemOperand*> newMemOpnds;
  bool doFullReplaceProp = true; /* due to register pressure, do not do partial prop */
  if (memPropMode == kPropBase) {
    replacedReg = currMemOpnd.GetBaseRegister();
  } else {
    Operand *offset = currMemOpnd.GetOffset();
    ASSERT(offset->IsRegister(), "must be");
    replacedReg = static_cast<AArch64RegOperand*>(offset);
  }
  CHECK_FATAL(replacedReg != nullptr, "check this insn");
  GetDefInsn(*replacedReg, allUseInsns);
  if (defInsn != nullptr) {
    for (auto useInsn : allUseInsns) {
      AArch64MemOperand *oldMemOpnd = StrLdrPropPreCheck(*useInsn, memPropMode);
      if (CheckSameReplace(*replacedReg, oldMemOpnd)) {
        AArch64MemOperand *newMemOpnd = SelectReplaceMem(*defInsn, *oldMemOpnd);
        if (newMemOpnd != nullptr) {
          uint32 opndIdx = GetMemOpndIdx(oldMemOpnd, *useInsn);
          if (CheckNewMemOffset(*useInsn, newMemOpnd, opndIdx)) {
            newMemOpnds.emplace_back(newMemOpnd);
            continue;
          }
        }
      }
      doFullReplaceProp = false;
      break;
    }
  }
  if (doFullReplaceProp) {
    for (size_t i = 0; i < newMemOpnds.size(); ++i) {
      DoMemReplace(*replacedReg, *newMemOpnds[i], *allUseInsns[i]);
    }
    return true;
  }
  return false;
}

bool A64StrLdrProp::CheckSameReplace(RegOperand &replacedReg, AArch64MemOperand *memOpnd) {
  if (memOpnd != nullptr && memPropMode != kUndef) {
    if (memPropMode == kPropBase) {
      return replacedReg.GetRegisterNumber() ==  memOpnd->GetBaseRegister()->GetRegisterNumber();
    } else {
      Operand *offset = memOpnd->GetOffset();
      ASSERT(offset->IsRegister(), "must be");
      return replacedReg.GetRegisterNumber() == static_cast<AArch64RegOperand*>(offset)->GetRegisterNumber();
    }
  }
  return false;
}

uint32 A64StrLdrProp::GetMemOpndIdx(AArch64MemOperand *newMemOpnd, Insn &insn) {
  int32 opndIdx = kInsnMaxOpnd;
  if (insn.IsLoadPair() || insn.IsStorePair()) {
    ASSERT(newMemOpnd->GetOffsetImmediate() != nullptr, "unexpect insn");
    opndIdx = kInsnThirdOpnd;
  } else {
    opndIdx = kInsnSecondOpnd;
  }
  return opndIdx;
}

void A64StrLdrProp::DoMemReplace(RegOperand &replacedReg, AArch64MemOperand &newMem, Insn &useInsn) {
  VRegVersion *replacedV = ssaInfo->FindSSAVersion(replacedReg.GetRegisterNumber());
  ASSERT(replacedV != nullptr, "must in ssa form");
  uint32 opndIdx = GetMemOpndIdx(&newMem, useInsn);
  replacedV->RemoveUseInsn(useInsn, opndIdx);
  for (auto &replaceit : replaceVersions) {
    replaceit.second->AddUseInsn(*ssaInfo, useInsn, opndIdx);
  }
  useInsn.SetOperand(opndIdx, newMem);
  replaceVersions.clear();
}

AArch64MemOperand *A64StrLdrProp::StrLdrPropPreCheck(Insn &insn, MemPropMode prevMod) {
  memPropMode = kUndef;
  if (insn.IsLoad() || insn.IsStore()) {
    if (insn.IsAtomic() || insn.GetOperand(0).GetSize() == k128BitSize) {
      return nullptr;
    }
    auto *currMemOpnd = static_cast<AArch64MemOperand*>(insn.GetMemOpnd());
    if (currMemOpnd != nullptr) {
      memPropMode = SelectStrLdrPropMode(*currMemOpnd);
      if (prevMod != kUndef) {
        if (prevMod != memPropMode) {
          memPropMode = prevMod;
          return nullptr;
        }
      }
      return currMemOpnd;
    }
  }
  return nullptr;
}

MemPropMode A64StrLdrProp::SelectStrLdrPropMode(AArch64MemOperand &currMemOpnd) {
  AArch64MemOperand::AArch64AddressingMode currAddrMode = currMemOpnd.GetAddrMode();
  MemPropMode innerMemPropMode = kUndef;
  switch (currAddrMode) {
    case AArch64MemOperand::kAddrModeBOi: {
      if (!currMemOpnd.IsPreIndexed() && !currMemOpnd.IsPostIndexed()) {
        innerMemPropMode = kPropBase;
      }
      break;
    }
    case AArch64MemOperand::kAddrModeBOrX: {
      innerMemPropMode = kPropOffset;
      uint32 amount = currMemOpnd.ShiftAmount();
      if (currMemOpnd.GetExtendAsString() == "LSL") {
        if (amount != 0) {
          innerMemPropMode = kPropShift;
        }
        break;
      } else if (currMemOpnd.SignedExtend()) {
        innerMemPropMode = kPropSignedExtend;
      } else if (currMemOpnd.UnsignedExtend()) {
        innerMemPropMode = kPropUnsignedExtend;
      }
      break;
    }
    default:
      innerMemPropMode = kUndef;
  }
  return innerMemPropMode;
}

AArch64MemOperand *A64StrLdrProp::SelectReplaceMem(Insn &defInsn,  AArch64MemOperand &currMemOpnd) {
  AArch64MemOperand *newMemOpnd = nullptr;
  Operand *offset = currMemOpnd.GetOffset();
  RegOperand *base = currMemOpnd.GetBaseRegister();
  MOperator opCode = defInsn.GetMachineOpcode();
  switch (opCode) {
    case MOP_xsubrri12:
    case MOP_wsubrri12: {
      AArch64RegOperand *replace = GetReplaceReg(static_cast<AArch64RegOperand&>(defInsn.GetOperand(kInsnSecondOpnd)));
      if (replace != nullptr) {
        auto &immOpnd = static_cast<AArch64ImmOperand&>(defInsn.GetOperand(kInsnThirdOpnd));
        int64 defVal = -(immOpnd.GetValue());
        newMemOpnd = HandleArithImmDef(*replace, offset, defVal);
      }
      break;
    }
    case MOP_xaddrri12:
    case MOP_waddrri12: {
      AArch64RegOperand *replace = GetReplaceReg(static_cast<AArch64RegOperand&>(defInsn.GetOperand(kInsnSecondOpnd)));
      if (replace != nullptr) {
        auto &immOpnd = static_cast<AArch64ImmOperand&>(defInsn.GetOperand(kInsnThirdOpnd));
        int64 defVal = immOpnd.GetValue();
        newMemOpnd = HandleArithImmDef(*replace, offset, defVal);
      }
      break;
    }
    case MOP_xaddrrr:
    case MOP_waddrrr:
    case MOP_dadd:
    case MOP_sadd: {
      if (memPropMode == kPropBase) {
        auto *ofstOpnd = static_cast<OfstOperand*>(offset);
        if (!ofstOpnd->IsZero()) {
          break;
        }
        AArch64RegOperand *replace = GetReplaceReg(
            static_cast<AArch64RegOperand&>(defInsn.GetOperand(kInsnSecondOpnd)));
        AArch64RegOperand *newOfst = GetReplaceReg(static_cast<AArch64RegOperand&>(defInsn.GetOperand(kInsnThirdOpnd)));
        if (replace != nullptr && newOfst != nullptr) {
          newMemOpnd = cgFunc->GetMemoryPool()->New<AArch64MemOperand>(
              AArch64MemOperand::kAddrModeBOrX, currMemOpnd.GetSize(), *replace, newOfst, nullptr, nullptr);
        }
      }
      break;
    }
    case MOP_xadrpl12: {
      if (memPropMode == kPropBase) {
        auto *ofstOpnd = static_cast<OfstOperand*>(offset);
        CHECK_FATAL(ofstOpnd != nullptr, "oldOffset is null!");
        int64 val = ofstOpnd->GetValue();
        auto *offset1 = static_cast<StImmOperand*>(&defInsn.GetOperand(kInsnThirdOpnd));
        CHECK_FATAL(offset1 != nullptr, "offset1 is null!");
        val += offset1->GetOffset();
        AArch64OfstOperand *newOfsetOpnd = cgFunc->GetMemoryPool()->New<AArch64OfstOperand>(val, k32BitSize);
        CHECK_FATAL(newOfsetOpnd != nullptr, "newOfsetOpnd is null!");
        const MIRSymbol *addr = offset1->GetSymbol();
        AArch64RegOperand *replace = GetReplaceReg(
            static_cast<AArch64RegOperand&>(defInsn.GetOperand(kInsnSecondOpnd)));
        if (replace != nullptr) {
          newMemOpnd = cgFunc->GetMemoryPool()->New<AArch64MemOperand>(
              AArch64MemOperand::kAddrModeLo12Li, currMemOpnd.GetSize(), *replace, nullptr, newOfsetOpnd, addr);
        }
      }
      break;
    }
    /* do this in const prop ? */
    case MOP_xmovri32:
    case MOP_xmovri64: {
      if (memPropMode == kPropOffset) {
        auto *imm = static_cast<AArch64ImmOperand*>(&defInsn.GetOperand(kInsnSecondOpnd));
        AArch64OfstOperand *newOffset = cgFunc->GetMemoryPool()->New<AArch64OfstOperand>(imm->GetValue(), k32BitSize);
        CHECK_FATAL(newOffset != nullptr, "newOffset is null!");
        newMemOpnd = cgFunc->GetMemoryPool()->New<AArch64MemOperand>(
            AArch64MemOperand::kAddrModeBOi, currMemOpnd.GetSize(), *base, nullptr, newOffset, nullptr);
      }
      break;
    }
    case MOP_xlslrri6:
    case MOP_wlslrri5: {
      auto *imm = static_cast<AArch64ImmOperand*>(&defInsn.GetOperand(kInsnThirdOpnd));
      AArch64RegOperand *newOfst = GetReplaceReg(static_cast<AArch64RegOperand&>(defInsn.GetOperand(kInsnSecondOpnd)));
      if (newOfst != nullptr) {
        int64 shift = imm->GetValue();
        if (memPropMode == kPropOffset) {
          if ((shift < k4ByteSize) && (shift >= 0)) {
            newMemOpnd = cgFunc->GetMemoryPool()->New<AArch64MemOperand>(
                AArch64MemOperand::kAddrModeBOrX, currMemOpnd.GetSize(), *base, *newOfst, shift);
          }
        } else if (memPropMode == kPropShift) {
          shift += currMemOpnd.ShiftAmount();
          if ((shift < k4ByteSize) && (shift >= 0)) {
            newMemOpnd = cgFunc->GetMemoryPool()->New<AArch64MemOperand>(
                AArch64MemOperand::kAddrModeBOrX, currMemOpnd.GetSize(), *base, *newOfst, shift);
          }
        }
      }
      break;
    }
    case MOP_xsxtw64: {
      newMemOpnd = SelectReplaceExt(defInsn, *base, currMemOpnd.ShiftAmount(),true);
      break;
    }
    case MOP_xuxtw64: {
      newMemOpnd = SelectReplaceExt(defInsn, *base, currMemOpnd.ShiftAmount(), false);
      break;
    }
    default:
      break;
  }
  return newMemOpnd;
}

AArch64RegOperand *A64StrLdrProp::GetReplaceReg(AArch64RegOperand &a64Reg) {
  if (a64Reg.IsSSAForm()) {
    regno_t ssaIndex = a64Reg.GetRegisterNumber();
    replaceVersions[ssaIndex] = ssaInfo->FindSSAVersion(ssaIndex);
    ASSERT(replaceVersions.size() <= 2, "CHECK THIS CASE IN A64PROP");
    return &a64Reg;
  }
  return nullptr;
}

AArch64MemOperand *A64StrLdrProp::HandleArithImmDef(AArch64RegOperand &replace, Operand *oldOffset, int64 defVal) {
  if (memPropMode != kPropBase) {
    return nullptr;
  }
  AArch64OfstOperand *newOfstImm = nullptr;
  if (oldOffset == nullptr) {
    newOfstImm = cgFunc->GetMemoryPool()->New<AArch64OfstOperand>(defVal, k32BitSize);
  } else {
    auto *ofstOpnd = static_cast<AArch64OfstOperand*>(oldOffset);
    CHECK_FATAL(ofstOpnd != nullptr, "oldOffsetOpnd is null");
    newOfstImm = cgFunc->GetMemoryPool()->New<AArch64OfstOperand>(defVal + ofstOpnd->GetValue(), k32BitSize);
  }
  CHECK_FATAL(newOfstImm != nullptr, "newOffset is null!");
  return cgFunc->GetMemoryPool()->New<AArch64MemOperand>(AArch64MemOperand::kAddrModeBOi, k64BitSize,
                                                        replace, nullptr, newOfstImm, nullptr);
}

AArch64MemOperand *A64StrLdrProp::SelectReplaceExt(Insn &defInsn, RegOperand &base, uint32 amount, bool isSigned) {
  AArch64MemOperand *newMemOpnd = nullptr;
  AArch64RegOperand *newOfst = GetReplaceReg(static_cast<AArch64RegOperand&>(defInsn.GetOperand(kInsnSecondOpnd)));
  if (newOfst == nullptr) {
    return nullptr;
  }
  /* defInsn is extend, currMemOpnd is same extend or shift */
  bool propExtend = (memPropMode == kPropShift) || ((memPropMode == kPropSignedExtend) && isSigned) ||
                    ((memPropMode == kPropUnsignedExtend) && !isSigned);
  if (memPropMode == kPropOffset) {
    newMemOpnd = cgFunc->GetMemoryPool()->New<AArch64MemOperand>(
        AArch64MemOperand::kAddrModeBOrX, k64BitSize, base, *newOfst, 0, isSigned);
  } else if (propExtend) {
    newMemOpnd = cgFunc->GetMemoryPool()->New<AArch64MemOperand>(
        AArch64MemOperand::kAddrModeBOrX, k64BitSize, base, *newOfst, amount, isSigned);
  } else {
    return nullptr;
  }
  return newMemOpnd;
}

bool A64StrLdrProp::CheckNewMemOffset(Insn &insn, AArch64MemOperand *newMemOpnd, uint32 opndIdx) {
  auto *a64CgFunc = static_cast<AArch64CGFunc*>(cgFunc);
  if ((newMemOpnd->GetOffsetImmediate() != nullptr) &&
      !a64CgFunc->IsOperandImmValid(insn.GetMachineOpcode(), newMemOpnd, opndIdx)) {
    return false;
  }
  uint32 newAmount = newMemOpnd->ShiftAmount();
  if (!AArch64StoreLoadOpt::CheckNewAmount(insn, newAmount)) {
    return false;
  }
  /* is ldp or stp, addrMode must be BOI */
  if ((opndIdx == kInsnThirdOpnd) && (newMemOpnd->GetAddrMode() != AArch64MemOperand::kAddrModeBOi)) {
    return false;
  }
  return true;
}

void A64ReplaceRegOpndVisitor::Visit(RegOperand *v) {
  (void)v;
  insn->SetOperand(idx, *newReg);
}
void A64ReplaceRegOpndVisitor::Visit(MemOperand *v) {
  auto *a64memOpnd = static_cast<AArch64MemOperand*>(v);
  bool changed = false;
  CHECK_FATAL(a64memOpnd->IsIntactIndexed(), "NYI post/pre index model");
  StackMemPool tempMemPool(memPoolCtrler, "temp mempool for A64ReplaceRegOpndVisitor");
  auto *copyMem = static_cast<AArch64MemOperand*>(a64memOpnd->Clone(tempMemPool));
  if (copyMem->GetBaseRegister() != nullptr &&
      copyMem->GetBaseRegister()->GetRegisterNumber() == oldReg->GetRegisterNumber()) {
    copyMem->SetBaseRegister(*static_cast<AArch64RegOperand*>(newReg));
    changed = true;
  }
  if (copyMem->GetIndexRegister() != nullptr &&
      copyMem->GetIndexRegister()->GetRegisterNumber() == oldReg->GetRegisterNumber()) {
    CHECK_FATAL(!changed, "base reg is equal to index reg");
    copyMem->SetIndexRegister(*newReg);
    changed = true;
  }
  if (changed) {
    insn->SetMemOpnd(&static_cast<AArch64CGFunc*>(cgFunc)->GetOrCreateMemOpnd(*copyMem));
  }
}
void A64ReplaceRegOpndVisitor::Visit(ListOperand *v) {
  for (auto &it : v->GetOperands()) {
    if (it->GetRegisterNumber() == oldReg->GetRegisterNumber()) {
      it = newReg;
    }
  }
}
void A64ReplaceRegOpndVisitor::Visit(PhiOperand *v) {
  for (auto &it : v->GetOperands()) {
    if (it.second->GetRegisterNumber() == oldReg->GetRegisterNumber()) {
      it.second = newReg;
    }
  }
}
}

