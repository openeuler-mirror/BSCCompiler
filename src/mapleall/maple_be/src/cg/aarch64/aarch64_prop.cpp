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
#include "aarch64_cg.h"
#include <climits>

namespace maplebe {

#define PROP_DUMP CG_DEBUG_FUNC(cgFunc)

bool AArch64Prop::IsInLimitCopyRange(VRegVersion *toBeReplaced) {
  uint32 baseID = toBeReplaced->GetDefInsnInfo()->GetInsn()->GetId();
  MapleUnorderedMap<uint32, DUInsnInfo*> &useList = toBeReplaced->GetAllUseInsns();
  for (auto it : useList) {
    if (it.second->GetInsn()->GetId() - baseID > k16BitSize) {
      return false;
    }
  }
  return true;
}

void AArch64Prop::CopyProp() {
  PropOptimizeManager optManager(*cgFunc, GetSSAInfo());
  optManager.Optimize<CopyRegProp>();
  optManager.Optimize<ValidBitNumberProp>();
  optManager.Optimize<RedundantPhiProp>();
}

void AArch64Prop::TargetProp(Insn &insn) {
  A64ConstProp a64ConstProp(*memPool, *cgFunc, *GetSSAInfo(), insn);
  a64ConstProp.DoOpt();
  A64StrLdrProp a64StrLdrProp(*memPool, *cgFunc, *GetSSAInfo(), insn, *GetDce());
  a64StrLdrProp.DoOpt();
}

void A64ConstProp::DoOpt() {
  if (curInsn->GetMachineOpcode() == MOP_xmovri32 || curInsn->GetMachineOpcode() == MOP_xmovri64) {
    Operand &destOpnd = curInsn->GetOperand(kInsnFirstOpnd);
    CHECK_FATAL(destOpnd.IsRegister(), "must be reg operand");
    auto &destReg = static_cast<RegOperand&>(destOpnd);
    if (destReg.IsSSAForm()) {
      VRegVersion *destVersion = ssaInfo->FindSSAVersion(destReg.GetRegisterNumber());
      ASSERT(destVersion != nullptr, "find Version failed");
      Operand &constOpnd = curInsn->GetOperand(kInsnSecondOpnd);
      CHECK_FATAL(constOpnd.IsImmediate(), "must be imm operand");
      auto &immOperand = static_cast<AArch64ImmOperand&>(constOpnd);
      bool isZero = immOperand.IsZero();
      for (auto useDUInfoIt : destVersion->GetAllUseInsns()) {
        if (isZero) {
          ZeroRegProp(*useDUInfoIt.second, *destVersion->GetSSAvRegOpnd());
          destVersion->CheckDeadUse(*useDUInfoIt.second->GetInsn());
        }
        (void)ConstProp(*useDUInfoIt.second, immOperand);
      }
    }
  }
}

void A64ConstProp::ZeroRegProp(DUInsnInfo &useDUInfo, RegOperand &toReplaceReg) {
  auto *useInsn = static_cast<AArch64Insn*>(useDUInfo.GetInsn());
  const AArch64MD *md = &AArch64CG::kMd[(useInsn->GetMachineOpcode())];
  /* special case */
  bool isSpecficCase = useInsn->GetMachineOpcode() == MPO_wbfirri5i5 || useInsn->GetMachineOpcode() == MPO_xbfirri6i6;
  isSpecficCase &= (useDUInfo.GetOperands().size() == 1) && (useDUInfo.GetOperands().begin()->first == kInsnSecondOpnd);
  if (useInsn->IsStore() || md->IsCondDef() || isSpecficCase)  {
    AArch64RegOperand &zeroOpnd = toReplaceReg.GetSize() == k64BitSize ?
                                  AArch64RegOperand::Get64bitZeroRegister() : AArch64RegOperand::Get32bitZeroRegister();
    for (auto &opndIt : useDUInfo.GetOperands()) {
      if (useInsn->IsStore() && opndIt.first != 0) {
        return;
      }
      Operand &opnd = useInsn->GetOperand(opndIt.first);
      A64ReplaceRegOpndVisitor replaceRegOpndVisitor(*cgFunc, *useInsn,
          opndIt.first, toReplaceReg, zeroOpnd);
      opnd.Accept(replaceRegOpndVisitor);
      useDUInfo.ClearDU(opndIt.first);
    }
  }
}

MOperator A64ConstProp::GetReversalMOP(MOperator arithMop) {
  switch (arithMop) {
    case MOP_waddrri12:
      return MOP_wsubrri12;
    case MOP_xaddrri12:
      return MOP_xsubrri12;
    case MOP_xsubrri12:
      return MOP_xaddrri12;
    case MOP_wsubrri12:
      return MOP_waddrri12;
    default:
      CHECK_FATAL(false, "NYI");
      break;
  }
  return MOP_undef;
}

MOperator A64ConstProp::GetRegImmMOP(MOperator regregMop, bool withLeftShift) {
  switch (regregMop) {
    case MOP_xaddrrrs:
    case MOP_xaddrrr: {
      return withLeftShift ? MOP_xaddrri24 : MOP_xaddrri12;
    }
    case MOP_waddrrrs:
    case MOP_waddrrr: {
      return withLeftShift ? MOP_waddrri24 : MOP_waddrri12;
    }
    case MOP_xsubrrrs:
    case MOP_xsubrrr: {
      return withLeftShift ? MOP_xsubrri24 : MOP_xsubrri12;
    }
    case MOP_wsubrrrs:
    case MOP_wsubrrr: {
      return withLeftShift ? MOP_wsubrri24 : MOP_wsubrri12;
    }
    case MOP_xandrrrs:
      return MOP_xandrri13;
    case MOP_wandrrrs:
      return MOP_wandrri12;
    case MOP_xeorrrrs:
      return MOP_xeorrri13;
    case MOP_weorrrrs:
      return MOP_weorrri12;
    case MOP_xiorrrrs:
      return MOP_xiorrri13;
    case MOP_wiorrrrs:
      return MOP_wiorrri12;
    case MOP_xmovrr: {
      return MOP_xmovri64;
    }
    case MOP_wmovrr: {
      return MOP_xmovri32;
    }
    default:
      CHECK_FATAL(false, "NYI");
      break;
  }
  return MOP_undef;
}

void A64ConstProp::ReplaceInsnAndUpdateSSA(Insn &oriInsn, Insn &newInsn) {
  ssaInfo->ReplaceInsn(oriInsn, newInsn);
  oriInsn.GetBB()->ReplaceInsn(oriInsn, newInsn);
  /* dump insn replacement here */
}

bool A64ConstProp::MovConstReplace(DUInsnInfo &useDUInfo, AArch64ImmOperand &constOpnd) {
  Insn *useInsn = useDUInfo.GetInsn();
  MOperator curMop = useInsn->GetMachineOpcode();
  if (useDUInfo.GetOperands().size() == 1) {
    MOperator newMop = GetRegImmMOP(curMop, false);
    Operand &destOpnd = useInsn->GetOperand(kInsnFirstOpnd);
    if (constOpnd.IsSingleInstructionMovable(destOpnd.GetSize())) {
      auto useOpndInfoIt = useDUInfo.GetOperands().begin();
      uint32 useOpndIdx = useOpndInfoIt->first;
      ASSERT(useOpndIdx == kInsnSecondOpnd, "invalid instruction in ssa form");
      if (useOpndIdx == kInsnSecondOpnd) {
        Insn &newInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(newMop, destOpnd, constOpnd);
        ReplaceInsnAndUpdateSSA(*useInsn, newInsn);
        return true;
      }
    }
  } else {
    ASSERT(false, "invalid instruction in ssa form");
  }
  return false;
}

/* support add now */
bool A64ConstProp::ArithmeticConstReplace(DUInsnInfo &useDUInfo, AArch64ImmOperand &constOpnd, ArithmeticType aT) {
  Insn *useInsn = useDUInfo.GetInsn();
  MOperator curMop = useInsn->GetMachineOpcode();
  if (useDUInfo.GetOperands().size() == 1) {
    MOperator newMop = GetRegImmMOP(curMop, false);
    auto useOpndInfoIt = useDUInfo.GetOperands().begin();
    uint32 useOpndIdx = useOpndInfoIt->first;
    CHECK_FATAL(useOpndIdx == kInsnSecondOpnd || useOpndIdx == kInsnThirdOpnd, "check this insn");
    Insn *newInsn = nullptr;
    if (static_cast<AArch64CGFunc*>(cgFunc)->IsOperandImmValid(newMop, &constOpnd, kInsnThirdOpnd)) {
      newInsn = &cgFunc->GetCG()->BuildInstruction<AArch64Insn>(
          newMop, useInsn->GetOperand(kInsnFirstOpnd), useInsn->GetOperand(kInsnSecondOpnd), constOpnd);
      if (useOpndIdx == kInsnSecondOpnd) { /* swap operand due to legality in aarch */
        newInsn->SetOperand(kInsnSecondOpnd, useInsn->GetOperand(kInsnThirdOpnd));
      }
    }
    /* try aggressive opt in aarch64 add and sub */
    if (newInsn == nullptr && (aT == kAArch64Add || aT == kAArch64Sub)) {
      auto *tempImm = static_cast<AArch64ImmOperand*>(constOpnd.Clone(*constPropMp));
      /* try aarch64 imm shift mode */
      tempImm->SetValue(tempImm->GetValue() >> 12);
      if (static_cast<AArch64CGFunc*>(cgFunc)->IsOperandImmValid(newMop, tempImm, kInsnThirdOpnd) &&
          CGOptions::GetInstance().GetOptimizeLevel() < 0) {
        ASSERT(false, "NIY");
      }
      /* Addition and subtraction reversal */
      tempImm->SetValue(-constOpnd.GetValue());
      newMop = GetReversalMOP(newMop);
      if (static_cast<AArch64CGFunc*>(cgFunc)->IsOperandImmValid(newMop, tempImm, kInsnThirdOpnd)) {
        auto *cgImm = static_cast<AArch64ImmOperand*>(tempImm->Clone(*cgFunc->GetMemoryPool()));
        newInsn = &cgFunc->GetCG()->BuildInstruction<AArch64Insn>(
            newMop, useInsn->GetOperand(kInsnFirstOpnd), useInsn->GetOperand(kInsnSecondOpnd), *cgImm);
        if (useOpndIdx == kInsnSecondOpnd) { /* swap operand due to legality in aarch */
          newInsn->SetOperand(kInsnSecondOpnd, useInsn->GetOperand(kInsnThirdOpnd));
        }
      }
    }
    if (newInsn != nullptr) {
      ReplaceInsnAndUpdateSSA(*useInsn, *newInsn);
      return true;
    }
  } else if (useDUInfo.GetOperands().size() == 2) {
    /* no case in SPEC 2017 */
    ASSERT(false, "should be optimized by other phase");
  } else {
    ASSERT(false, "invalid instruction in ssa form");
  }
  return false;
}

bool A64ConstProp::ArithmeticConstFold(DUInsnInfo &useDUInfo, const AArch64ImmOperand &constOpnd,
                                       ArithmeticType aT) {
  Insn *useInsn = useDUInfo.GetInsn();
  if (useDUInfo.GetOperands().size() == 1) {
    Operand &existedImm = useInsn->GetOperand(kInsnThirdOpnd);
    ASSERT(existedImm.IsImmediate(), "must be");
    Operand &destOpnd = useInsn->GetOperand(kInsnFirstOpnd);
    bool is64Bit = destOpnd.GetSize() == k64BitSize;
    AArch64ImmOperand *foldConst = CanDoConstFold(constOpnd, static_cast<AArch64ImmOperand&>(existedImm), aT, is64Bit);
    if (foldConst != nullptr) {
      MOperator newMop = is64Bit ? MOP_xmovri64 : MOP_xmovri32;
      Insn &newInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(newMop, destOpnd, *foldConst);
      ReplaceInsnAndUpdateSSA(*useInsn, newInsn);
      return true;
    }
  }
  return false;
}

bool A64ConstProp::ShiftConstReplace(DUInsnInfo &useDUInfo, const AArch64ImmOperand &constOpnd) {
  Insn *useInsn = useDUInfo.GetInsn();
  MOperator curMop = useInsn->GetMachineOpcode();
  if (useDUInfo.GetOperands().size() == 1) {
    auto useOpndInfoIt = useDUInfo.GetOperands().begin();
    uint32 useOpndIdx = useOpndInfoIt->first;
    if (useOpndIdx == kInsnThirdOpnd) {
      auto &shiftBit = static_cast<BitShiftOperand&>(useInsn->GetOperand(kInsnFourthOpnd));
      int64 val = constOpnd.GetValue();
      if (shiftBit.GetShiftOp() == BitShiftOperand::kLSL) {
        val = val << shiftBit.GetShiftAmount();
      } else if (shiftBit.GetShiftOp() == BitShiftOperand::kLSR) {
        val = val >> shiftBit.GetShiftAmount();
      } else if (shiftBit.GetShiftOp() == BitShiftOperand::kASR) {
        val = static_cast<int64>((static_cast<uint64>(val)) >> shiftBit.GetShiftAmount());
      } else {
        CHECK_FATAL(false, "shift type is not defined");
      }
      auto *newImm = static_cast<AArch64ImmOperand*>(constOpnd.Clone(*constPropMp));
      newImm->SetValue(val);
      MOperator newMop = GetRegImmMOP(curMop, false);
      if (static_cast<AArch64CGFunc*>(cgFunc)->IsOperandImmValid(newMop, newImm, kInsnThirdOpnd)) {
        auto *cgNewImm = static_cast<AArch64ImmOperand*>(constOpnd.Clone(*cgFunc->GetMemoryPool()));
        Insn &newInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(
            newMop, useInsn->GetOperand(kInsnFirstOpnd), useInsn->GetOperand(kInsnSecondOpnd), *cgNewImm);
        ReplaceInsnAndUpdateSSA(*useInsn, newInsn);
        return true;
      }
    }
  }
  return false;
}

bool A64ConstProp::ConstProp(DUInsnInfo &useDUInfo, AArch64ImmOperand &constOpnd) {
  MOperator curMop = useDUInfo.GetInsn()->GetMachineOpcode();
  switch (curMop) {
    case MOP_xmovrr:
    case MOP_wmovrr: {
      return MovConstReplace(useDUInfo, constOpnd);
    }
    case MOP_xsubrrr:
    case MOP_wsubrrr: {
      break;
    }
    case MOP_xaddrrr:
    case MOP_waddrrr: {
      return ArithmeticConstReplace(useDUInfo, constOpnd, kAArch64Add);
    }
    case MOP_waddrri12:
    case MOP_xaddrri12: {
      return ArithmeticConstFold(useDUInfo, constOpnd, kAArch64Add);

    }
    case MOP_xsubrri12:
    case MOP_wsubrri12: {
      return ArithmeticConstFold(useDUInfo, constOpnd, kAArch64Sub);
    }
    case MOP_xiorrrrs:
    case MOP_wiorrrrs:
    case MOP_xeorrrrs:
    case MOP_weorrrrs:
    case MOP_xandrrrs:
    case MOP_wandrrrs:
    case MOP_xaddrrrs:
    case MOP_waddrrrs:
    case MOP_wsubrrrs:
    case MOP_xsubrrrs: {
     return ShiftConstReplace(useDUInfo, constOpnd);
    }
    default:
      break;
  }
  return false;
}

AArch64ImmOperand *A64ConstProp::CanDoConstFold(
    const AArch64ImmOperand &value1, const AArch64ImmOperand &value2, ArithmeticType aT, bool is64Bit) {
  auto *tempImm = static_cast<AArch64ImmOperand*>(value1.Clone(*constPropMp));
  int64 newVal = 0;
  switch (aT) {
    case kAArch64Add : {
      newVal = value1.GetValue() + value2.GetValue();
      break;
    }
    case kAArch64Sub : {
      newVal = value1.GetValue() - value2.GetValue();
      break;
    }
    default:
      return nullptr;
  }
  if (!is64Bit && newVal > INT_MAX) {
    return nullptr;
  }
  if (newVal < 0) {
    tempImm->SetSigned();
  }
  tempImm->SetValue(newVal);
  if (value2.GetVary() == kUnAdjustVary) {
    tempImm->SetVary(kUnAdjustVary);
  }
  bool canBeMove = tempImm->IsSingleInstructionMovable(k64BitSize);
  return canBeMove ? static_cast<AArch64ImmOperand*>(tempImm->Clone(*cgFunc->GetMemoryPool())): nullptr;
}

void A64StrLdrProp::DoOpt() {
  ASSERT(curInsn != nullptr, "not input insn");
  bool tryOptAgain = false;
  do {
    tryOptAgain = false;
    AArch64MemOperand *currMemOpnd = StrLdrPropPreCheck(*curInsn);
    if (currMemOpnd != nullptr && memPropMode != kUndef) {
      /* can be changed to recursive propagation */
      if (ReplaceMemOpnd(*currMemOpnd, nullptr)) {
        tryOptAgain = true;
      }
      replaceVersions.clear();
    }
  } while (tryOptAgain);
}

bool A64StrLdrProp::ReplaceMemOpnd(const AArch64MemOperand &currMemOpnd, const Insn *defInsn) {
  auto GetDefInsn = [&defInsn, this](const RegOperand &regOpnd,
      std::vector<Insn*> &allUseInsns)->void{
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
  } else {
    doFullReplaceProp = false;
  }
  if (doFullReplaceProp) {
    for (size_t i = 0; i < newMemOpnds.size(); ++i) {
      DoMemReplace(*replacedReg, *newMemOpnds[i], *allUseInsns[i]);
    }
    return true;
  }
  return false;
}

bool A64StrLdrProp::CheckSameReplace(const RegOperand &replacedReg, const AArch64MemOperand *memOpnd) {
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

uint32 A64StrLdrProp::GetMemOpndIdx(AArch64MemOperand *newMemOpnd, const Insn &insn) {
  uint32 opndIdx = kInsnMaxOpnd;
  if (insn.IsLoadPair() || insn.IsStorePair()) {
    ASSERT(newMemOpnd->GetOffsetImmediate() != nullptr, "unexpect insn");
    opndIdx = kInsnThirdOpnd;
  } else {
    opndIdx = kInsnSecondOpnd;
  }
  return opndIdx;
}

void A64StrLdrProp::DoMemReplace(const RegOperand &replacedReg, AArch64MemOperand &newMem, Insn &useInsn) {
  VRegVersion *replacedV = ssaInfo->FindSSAVersion(replacedReg.GetRegisterNumber());
  ASSERT(replacedV != nullptr, "must in ssa form");
  uint32 opndIdx = GetMemOpndIdx(&newMem, useInsn);
  replacedV->RemoveUseInsn(useInsn, opndIdx);
  if (replacedV->GetAllUseInsns().empty()) {
    (void)cgDce->RemoveUnuseDef(*replacedV);
  }
  for (auto &replaceit : replaceVersions) {
    replaceit.second->AddUseInsn(*ssaInfo, useInsn, opndIdx);
  }
  useInsn.SetOperand(opndIdx, newMem);
}

AArch64MemOperand *A64StrLdrProp::StrLdrPropPreCheck(const Insn &insn, MemPropMode prevMod) {
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

MemPropMode A64StrLdrProp::SelectStrLdrPropMode(const AArch64MemOperand &currMemOpnd) {
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
      auto amount = currMemOpnd.ShiftAmount();
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

AArch64MemOperand *A64StrLdrProp::SelectReplaceMem(const Insn &defInsn,  const AArch64MemOperand &currMemOpnd) {
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
        newMemOpnd = HandleArithImmDef(*replace, offset, defVal, currMemOpnd.GetSize());
      }
      break;
    }
    case MOP_xaddrri12:
    case MOP_waddrri12: {
      AArch64RegOperand *replace = GetReplaceReg(static_cast<AArch64RegOperand&>(defInsn.GetOperand(kInsnSecondOpnd)));
      if (replace != nullptr) {
        auto &immOpnd = static_cast<AArch64ImmOperand&>(defInsn.GetOperand(kInsnThirdOpnd));
        int64 defVal = immOpnd.GetValue();
        newMemOpnd = HandleArithImmDef(*replace, offset, defVal, currMemOpnd.GetSize());
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
    case MOP_xaddrrrs:
    case MOP_waddrrrs: {
      if (memPropMode == kPropBase) {
        auto *ofstOpnd = static_cast<OfstOperand*>(offset);
        if (!ofstOpnd->IsZero()) {
          break;
        }
        AArch64RegOperand *newBaseOpnd = GetReplaceReg(
            static_cast<AArch64RegOperand&>(defInsn.GetOperand(kInsnSecondOpnd)));
        AArch64RegOperand *newIndexOpnd = GetReplaceReg(
            static_cast<AArch64RegOperand&>(defInsn.GetOperand(kInsnThirdOpnd)));
        auto &shift = static_cast<BitShiftOperand&>(defInsn.GetOperand(kInsnFourthOpnd));
        if (shift.GetShiftOp() != BitShiftOperand::kLSL) {
          break;
        }
        if (newBaseOpnd != nullptr && newIndexOpnd != nullptr) {
          newMemOpnd = cgFunc->GetMemoryPool()->New<AArch64MemOperand>(
              AArch64MemOperand::kAddrModeBOrX, currMemOpnd.GetSize(), *newBaseOpnd, *newIndexOpnd,
              shift.GetShiftAmount(), false);
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
      newMemOpnd = SelectReplaceExt(defInsn, *base, static_cast<uint32>(currMemOpnd.ShiftAmount()),
                                    true, currMemOpnd.GetSize());
      break;
    }
    case MOP_xuxtw64: {
      newMemOpnd = SelectReplaceExt(defInsn, *base, static_cast<uint32>(currMemOpnd.ShiftAmount()),
                                    false, currMemOpnd.GetSize());
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

AArch64MemOperand *A64StrLdrProp::HandleArithImmDef(AArch64RegOperand &replace, Operand *oldOffset,
                                                    int64 defVal, uint32 memSize) {
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
  return cgFunc->GetMemoryPool()->New<AArch64MemOperand>(AArch64MemOperand::kAddrModeBOi, memSize,
                                                        replace, nullptr, newOfstImm, nullptr);
}

AArch64MemOperand *A64StrLdrProp::SelectReplaceExt(const Insn &defInsn, RegOperand &base, uint32 amount,
                                                   bool isSigned, uint32 memSize) {
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
        AArch64MemOperand::kAddrModeBOrX, memSize, base, *newOfst, 0, isSigned);
  } else if (propExtend) {
    newMemOpnd = cgFunc->GetMemoryPool()->New<AArch64MemOperand>(
        AArch64MemOperand::kAddrModeBOrX, memSize, base, *newOfst, amount, isSigned);
  } else {
    return nullptr;
  }
  return newMemOpnd;
}

bool A64StrLdrProp::CheckNewMemOffset(const Insn &insn, AArch64MemOperand *newMemOpnd, uint32 opndIdx) {
  auto *a64CgFunc = static_cast<AArch64CGFunc*>(cgFunc);
  if ((newMemOpnd->GetOffsetImmediate() != nullptr) &&
      !a64CgFunc->IsOperandImmValid(insn.GetMachineOpcode(), newMemOpnd, opndIdx)) {
    return false;
  }
  auto newAmount = static_cast<uint32>(newMemOpnd->ShiftAmount());
  if (!AArch64StoreLoadOpt::CheckNewAmount(insn, newAmount)) {
    return false;
  }
  /* is ldp or stp, addrMode must be BOI */
  if ((opndIdx == kInsnThirdOpnd) && (newMemOpnd->GetAddrMode() != AArch64MemOperand::kAddrModeBOi)) {
    return false;
  }
  return true;
}

void AArch64Prop::PropPatternOpt() {
  PropOptimizeManager optManager(*cgFunc, GetSSAInfo());
  optManager.Optimize<ExtendMovPattern>();
  /* need peephole optimize */
  if (CGOptions::GetInstance().GetOptimizeLevel() < 0) {
    optManager.Optimize<ExtendShiftPattern>();
  }
  optManager.Optimize<FpSpConstProp>();
}

void ExtendShiftPattern::SetExMOpType(const Insn &use) {
  MOperator op = use.GetMachineOpcode();
  switch (op) {
    case MOP_xaddrrr:
    case MOP_xxwaddrrre:
    case MOP_xaddrrrs: {
      exMOpType = kExAdd;
      break;
    }
    case MOP_waddrrr:
    case MOP_wwwaddrrre:
    case MOP_waddrrrs: {
      exMOpType = kEwAdd;
      break;
    }
    case MOP_xsubrrr:
    case MOP_xxwsubrrre:
    case MOP_xsubrrrs: {
      exMOpType = kExSub;
      break;
    }
    case MOP_wsubrrr:
    case MOP_wwwsubrrre:
    case MOP_wsubrrrs: {
      exMOpType = kEwSub;
      break;
    }
    case MOP_xcmnrr:
    case MOP_xwcmnrre:
    case MOP_xcmnrrs: {
      exMOpType = kExCmn;
      break;
    }
    case MOP_wcmnrr:
    case MOP_wwcmnrre:
    case MOP_wcmnrrs: {
      exMOpType = kEwCmn;
      break;
    }
    case MOP_xcmprr:
    case MOP_xwcmprre:
    case MOP_xcmprrs: {
      exMOpType = kExCmp;
      break;
    }
    case MOP_wcmprr:
    case MOP_wwcmprre:
    case MOP_wcmprrs: {
      exMOpType = kEwCmp;
      break;
    }
    default: {
      exMOpType = kExUndef;
    }
  }
}

void ExtendShiftPattern::SetLsMOpType(const Insn &use) {
  MOperator op = use.GetMachineOpcode();
  switch (op) {
    case MOP_xaddrrr:
    case MOP_xaddrrrs: {
      lsMOpType = kLxAdd;
      break;
    }
    case MOP_waddrrr:
    case MOP_waddrrrs: {
      lsMOpType = kLwAdd;
      break;
    }
    case MOP_xsubrrr:
    case MOP_xsubrrrs: {
      lsMOpType = kLxSub;
      break;
    }
    case MOP_wsubrrr:
    case MOP_wsubrrrs: {
      lsMOpType = kLwSub;
      break;
    }
    case MOP_xcmnrr:
    case MOP_xcmnrrs: {
      lsMOpType = kLxCmn;
      break;
    }
    case MOP_wcmnrr:
    case MOP_wcmnrrs: {
      lsMOpType = kLwCmn;
      break;
    }
    case MOP_xcmprr:
    case MOP_xcmprrs: {
      lsMOpType = kLxCmp;
      break;
    }
    case MOP_wcmprr:
    case MOP_wcmprrs: {
      lsMOpType = kLwCmp;
      break;
    }
    case MOP_xeorrrr:
    case MOP_xeorrrrs: {
      lsMOpType = kLxEor;
      break;
    }
    case MOP_weorrrr:
    case MOP_weorrrrs: {
      lsMOpType = kLwEor;
      break;
    }
    case MOP_xinegrr:
    case MOP_xinegrrs: {
      lsMOpType = kLxNeg;
      replaceIdx = kInsnSecondOpnd;
      break;
    }
    case MOP_winegrr:
    case MOP_winegrrs: {
      lsMOpType = kLwNeg;
      replaceIdx = kInsnSecondOpnd;
      break;
    }
    case MOP_xiorrrr:
    case MOP_xiorrrrs: {
      lsMOpType = kLxIor;
      break;
    }
    case MOP_wiorrrr:
    case MOP_wiorrrrs: {
      lsMOpType = kLwIor;
      break;
    }
    default: {
      lsMOpType = kLsUndef;
    }
  }
}

void ExtendShiftPattern::SelectExtendOrShift(const Insn &def) {
  MOperator op = def.GetMachineOpcode();
  switch (op) {
    case MOP_xsxtb32:
    case MOP_xsxtb64: extendOp = ExtendShiftOperand::kSXTB;
      break;
    case MOP_xsxth32:
    case MOP_xsxth64: extendOp = ExtendShiftOperand::kSXTH;
      break;
    case MOP_xsxtw64: extendOp = ExtendShiftOperand::kSXTW;
      break;
    case MOP_xuxtb32: extendOp = ExtendShiftOperand::kUXTB;
      break;
    case MOP_xuxth32: extendOp = ExtendShiftOperand::kUXTH;
      break;
    case MOP_xuxtw64: extendOp = ExtendShiftOperand::kUXTW;
      break;
    case MOP_wlslrri5:
    case MOP_xlslrri6: shiftOp = BitShiftOperand::kLSL;
      break;
    case MOP_xlsrrri6:
    case MOP_wlsrrri5: shiftOp = BitShiftOperand::kLSR;
      break;
    case MOP_xasrrri6:
    case MOP_wasrrri5: shiftOp = BitShiftOperand::kASR;
      break;
    default: {
      extendOp = ExtendShiftOperand::kUndef;
      shiftOp = BitShiftOperand::kUndef;
    }
  }
}

/* first use must match SelectExtendOrShift */
bool ExtendShiftPattern::CheckDefUseInfo(uint32 size) {
  auto &regOperand = static_cast<AArch64RegOperand&>(defInsn->GetOperand(kInsnFirstOpnd));
  Operand &defSrcOpnd = defInsn->GetOperand(kInsnSecondOpnd);
  CHECK_FATAL(defSrcOpnd.IsRegister(), "defSrcOpnd must be register!");
  auto &regDefSrc = static_cast<AArch64RegOperand&>(defSrcOpnd);
  if (regDefSrc.IsPhysicalRegister()) {
    return false;
  }
  regno_t defSrcRegNo = regDefSrc.GetRegisterNumber();
  /* check regDefSrc */
  Insn *defSrcInsn = nullptr;
  VRegVersion *useVersion = optSsaInfo->FindSSAVersion(defSrcRegNo);
  CHECK_FATAL(useVersion != nullptr, "useVRegVersion must not be null based on ssa");
  DUInsnInfo *defInfo = useVersion->GetDefInsnInfo();
  if (defInfo == nullptr) {
    return false;
  }
  defSrcInsn = defInfo->GetInsn();

  const AArch64MD *md = &AArch64CG::kMd[static_cast<AArch64Insn*>(defSrcInsn)->GetMachineOpcode()];
  if ((size != regOperand.GetSize()) && md->IsMove()) {
    return false;
  }
  return true;
}

/* Optimize ExtendShiftPattern:
 * ==========================================================
 *           nosuffix  LSL   LSR   ASR      extrn   (def)
 * nosuffix |   F    | LSL | LSR | ASR |    extrn  |
 * LSL      |   F    | LSL |  F  |  F  |    extrn  |
 * LSR      |   F    |  F  | LSR |  F  |     F     |
 * ASR      |   F    |  F  |  F  | ASR |     F     |
 * exten    |   F    |  F  |  F  |  F  |exten(self)|
 * (use)
 * ===========================================================
 */
constexpr uint32 kExtenAddShiftNum = 5;
ExtendShiftPattern::SuffixType optTable[kExtenAddShiftNum][kExtenAddShiftNum] = {
        { ExtendShiftPattern::kNoSuffix, ExtendShiftPattern::kLSL, ExtendShiftPattern::kLSR,
                ExtendShiftPattern::kASR, ExtendShiftPattern::kExten },
        { ExtendShiftPattern::kNoSuffix, ExtendShiftPattern::kLSL, ExtendShiftPattern::kNoSuffix,
                ExtendShiftPattern::kNoSuffix, ExtendShiftPattern::kExten },
        { ExtendShiftPattern::kNoSuffix, ExtendShiftPattern::kNoSuffix, ExtendShiftPattern::kLSR,
                ExtendShiftPattern::kNoSuffix, ExtendShiftPattern::kNoSuffix },
        { ExtendShiftPattern::kNoSuffix, ExtendShiftPattern::kNoSuffix, ExtendShiftPattern::kNoSuffix,
                ExtendShiftPattern::kASR, ExtendShiftPattern::kNoSuffix },
        { ExtendShiftPattern::kNoSuffix, ExtendShiftPattern::kNoSuffix, ExtendShiftPattern::kNoSuffix,
                ExtendShiftPattern::kNoSuffix, ExtendShiftPattern::kExten }
};

/* Check whether ExtendShiftPattern optimization can be performed. */
ExtendShiftPattern::SuffixType ExtendShiftPattern::CheckOpType(const Operand &lastOpnd) const {
  /* Assign values to useType and defType. */
  uint32 useType = ExtendShiftPattern::kNoSuffix;
  uint32 defType = shiftOp;
  if (extendOp != ExtendShiftOperand::kUndef) {
    defType = ExtendShiftPattern::kExten;
  }
  if (lastOpnd.IsOpdShift()) {
    BitShiftOperand lastShiftOpnd = static_cast<const BitShiftOperand&>(lastOpnd);
    useType = lastShiftOpnd.GetShiftOp();
  } else if (lastOpnd.IsOpdExtend()) {
    ExtendShiftOperand lastExtendOpnd = static_cast<const ExtendShiftOperand&>(lastOpnd);
    useType = ExtendShiftPattern::kExten;
    /* two insn is exten and exten ,value is exten(oneself) */
    if (useType == defType && extendOp != lastExtendOpnd.GetExtendOp()) {
      return ExtendShiftPattern::kNoSuffix;
    }
  }
  return optTable[useType][defType];
}

constexpr uint32 kExMopTypeSize = 9;
constexpr uint32 kLsMopTypeSize = 15;

MOperator exMopTable[kExMopTypeSize] = {
        MOP_undef, MOP_xxwaddrrre, MOP_wwwaddrrre, MOP_xxwsubrrre, MOP_wwwsubrrre,
        MOP_xwcmnrre, MOP_wwcmnrre, MOP_xwcmprre, MOP_wwcmprre
};
MOperator lsMopTable[kLsMopTypeSize] = {
        MOP_undef, MOP_xaddrrrs, MOP_waddrrrs, MOP_xsubrrrs, MOP_wsubrrrs,
        MOP_xcmnrrs, MOP_wcmnrrs, MOP_xcmprrs, MOP_wcmprrs, MOP_xeorrrrs,
        MOP_weorrrrs, MOP_xinegrrs, MOP_winegrrs, MOP_xiorrrrs, MOP_wiorrrrs
};
/* new Insn extenType:
 * =====================
 * (useMop)   (defMop) (newmop)
 * | nosuffix |  all  | all|
 * | exten    |  ex   | ex |
 * |  ls      |  ex   | ls |
 * |  asr     |  !asr | F  |
 * |  !asr    |  asr  | F  |
 * (useMop)   (defMop)
 * =====================
 */
void ExtendShiftPattern::ReplaceUseInsn(Insn &use, const Insn &def, uint32 amount) {
  AArch64CGFunc &a64CGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  uint32 lastIdx = use.GetOperandSize() - k1BitSize;
  Operand &lastOpnd = use.GetOperand(lastIdx);
  ExtendShiftPattern::SuffixType optType = CheckOpType(lastOpnd);
  Operand *shiftOpnd = nullptr;
  if (optType == ExtendShiftPattern::kNoSuffix) {
    return;
  }else if (optType == ExtendShiftPattern::kExten) {
    replaceOp = exMopTable[exMOpType];
    if (amount > k4BitSize) {
      return;
    }
    shiftOpnd = &a64CGFunc.CreateExtendShiftOperand(extendOp, amount, static_cast<int32>(k64BitSize));
  } else {
    replaceOp = lsMopTable[lsMOpType];
    if (amount >= k32BitSize) {
      return;
    }
    shiftOpnd = &a64CGFunc.CreateBitShiftOperand(shiftOp, amount, static_cast<int32>(k64BitSize));
  }
  if (replaceOp == MOP_undef) {
    return;
  }

  Insn *replaceUseInsn = nullptr;
  Operand &firstOpnd = use.GetOperand(kInsnFirstOpnd);
  Operand *secondOpnd = &use.GetOperand(kInsnSecondOpnd);
  if (replaceIdx == kInsnSecondOpnd) { /* replace neg insn */
    secondOpnd = &def.GetOperand(kInsnSecondOpnd);
    replaceUseInsn = &cgFunc.GetCG()->BuildInstruction<AArch64Insn>(replaceOp, firstOpnd, *secondOpnd, *shiftOpnd);
  } else {
    Operand &thirdOpnd = def.GetOperand(kInsnSecondOpnd);
    replaceUseInsn = &cgFunc.GetCG()->BuildInstruction<AArch64Insn>(replaceOp, firstOpnd, *secondOpnd,
                                                                    thirdOpnd, *shiftOpnd);
  }
  use.GetBB()->ReplaceInsn(use, *replaceUseInsn);
  if (PROP_DUMP) {
    LogInfo::MapleLogger() << ">>>>>>> In ExtendShiftPattern : <<<<<<<\n";
    LogInfo::MapleLogger() << "=======ReplaceInsn :\n";
    use.Dump();
    LogInfo::MapleLogger() << "=======NewInsn :\n";
    replaceUseInsn->Dump();
  }
  /* update ssa info */
  optSsaInfo->ReplaceInsn(use, *replaceUseInsn);
  newInsn = replaceUseInsn;
  optSuccess = true;
}

/*
 * pattern1:
 * UXTB/UXTW X0, W1              <---- def x0
 * ....                          <---- (X0 not used)
 * AND/SUB/EOR X0, X1, X0        <---- use x0
 * ======>
 * AND/SUB/EOR X0, X1, W1 UXTB/UXTW
 *
 * pattern2:
 * LSL/LSR X0, X1, #8
 * ....(X0 not used)
 * AND/SUB/EOR X0, X1, X0
 * ======>
 * AND/SUB/EOR X0, X1, X1 LSL/LSR #8
 */
void ExtendShiftPattern::Optimize(Insn &insn) {
  uint32 amount = 0;
  uint32 offset = 0;
  uint32 lastIdx = insn.GetOperandSize() - k1BitSize;
  Operand &lastOpnd = insn.GetOperand(lastIdx);
  if (lastOpnd.IsOpdShift()) {
    auto &lastShiftOpnd = static_cast<BitShiftOperand&>(lastOpnd);
    amount = lastShiftOpnd.GetShiftAmount();
  } else if (lastOpnd.IsOpdExtend()) {
    auto &lastExtendOpnd = static_cast<ExtendShiftOperand&>(lastOpnd);
    amount = lastExtendOpnd.GetShiftAmount();
  }
  if (shiftOp != BitShiftOperand::kUndef) {
    auto &immOpnd = static_cast<AArch64ImmOperand&>(defInsn->GetOperand(kInsnThirdOpnd));
    offset = static_cast<uint32>(immOpnd.GetValue());
  }
  amount += offset;

  ReplaceUseInsn(insn, *defInsn, amount);
}

void ExtendShiftPattern::DoExtendShiftOpt(Insn &insn) {
  Init();
  if (!CheckCondition(insn)) {
    return;
  }
  Optimize(insn);
  if (optSuccess) {
    DoExtendShiftOpt(*newInsn);
  }
}

/* check and set:
 * exMOpType, lsMOpType, extendOp, shiftOp, defInsn
 */
bool ExtendShiftPattern::CheckCondition(Insn &insn) {
  SetLsMOpType(insn);
  SetExMOpType(insn);
  if ((exMOpType == kExUndef) && (lsMOpType == kLsUndef)) {
    return false;
  }
  auto &regOperand = static_cast<AArch64RegOperand&>(insn.GetOperand(replaceIdx));
  regno_t regNo = regOperand.GetRegisterNumber();
  VRegVersion *useVersion = optSsaInfo->FindSSAVersion(regNo);
  if (useVersion == nullptr) {
    return false;
  }
  DUInsnInfo *defInfo = useVersion->GetDefInsnInfo();
  if (defInfo == nullptr) {
    return false;
  }
  defInsn = defInfo->GetInsn();
  SelectExtendOrShift(*defInsn);
  if (useVersion->HasImplicitCvt() && shiftOp != BitShiftOperand::kUndef) {
    return false;
  }
  /* defInsn must be shift or extend */
  if ((extendOp == ExtendShiftOperand::kUndef) && (shiftOp == BitShiftOperand::kUndef)) {
    return false;
  }
  return CheckDefUseInfo(regOperand.GetSize());
}

void ExtendShiftPattern::Init() {
  replaceOp = MOP_undef;
  extendOp = ExtendShiftOperand::kUndef;
  shiftOp = BitShiftOperand::kUndef;
  defInsn = nullptr;
  replaceIdx = kInsnThirdOpnd;
  newInsn = nullptr;
  optSuccess = false;
  exMOpType = kExUndef;
  lsMOpType = kLsUndef;
}

void ExtendShiftPattern::Run() {
  if (!cgFunc.GetMirModule().IsCModule()) {
    return;
  }
  FOR_ALL_BB_REV(bb, &cgFunc) {
    FOR_BB_INSNS_REV(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      DoExtendShiftOpt(*insn);
    }
  }
}

void ExtendMovPattern::Run() {
  if (!cgFunc.GetMirModule().IsCModule()) {
    return;
  }
  FOR_ALL_BB(bb, &cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      if (!CheckCondition(*insn)) {
        continue;
      }
      Optimize(*insn);
    }
  }
}

bool ExtendMovPattern::CheckSrcReg(regno_t srcRegNo, uint32 validNum) {
  InsnSet srcDefSet;
  VRegVersion *useVersion = optSsaInfo->FindSSAVersion(srcRegNo);
  CHECK_FATAL(useVersion != nullptr, "useVRegVersion must not be null based on ssa");
  DUInsnInfo *defInfo = useVersion->GetDefInsnInfo();
  if (defInfo == nullptr) {
    return false;
  }
  Insn *insn = defInfo->GetInsn();
  srcDefSet.insert(insn);
  /* reserve insn set for non ssa version. */
  for (auto defInsn : srcDefSet) {
    CHECK_FATAL((defInsn != nullptr), "defInsn is null!");
    MOperator mOp = defInsn->GetMachineOpcode();
    switch (mOp) {
      case MOP_wiorrri12:
      case MOP_weorrri12: {
        /* check immVal if mop is OR */
        AArch64ImmOperand &imm = static_cast<AArch64ImmOperand&>(defInsn->GetOperand(kInsnThirdOpnd));
        uint32 bitNum = static_cast<uint32>(imm.GetValue());
        if ((bitNum >> validNum) != 0) {
          return false;
        }
      }
      case MOP_wandrri12: {
        /* check defSrcReg */
        RegOperand &defSrcRegOpnd = static_cast<RegOperand&>(defInsn->GetOperand(kInsnSecondOpnd));
        regno_t defSrcRegNo = defSrcRegOpnd.GetRegisterNumber();
        if (!CheckSrcReg(defSrcRegNo, validNum)) {
          return false;
        }
        break;
      }
      case MOP_wandrrr: {
        /* check defSrcReg */
        RegOperand &defSrcRegOpnd1 = static_cast<RegOperand&>(defInsn->GetOperand(kInsnSecondOpnd));
        RegOperand &defSrcRegOpnd2 = static_cast<RegOperand&>(defInsn->GetOperand(kInsnThirdOpnd));
        regno_t defSrcRegNo1 = defSrcRegOpnd1.GetRegisterNumber();
        regno_t defSrcRegNo2 = defSrcRegOpnd2.GetRegisterNumber();
        if (!CheckSrcReg(defSrcRegNo1, validNum) && !CheckSrcReg(defSrcRegNo2, validNum)) {
          return false;
        }
        break;
      }
      case MOP_wiorrrr:
      case MOP_weorrrr: {
        /* check defSrcReg */
        RegOperand &defSrcRegOpnd1 = static_cast<RegOperand&>(defInsn->GetOperand(kInsnSecondOpnd));
        RegOperand &defSrcRegOpnd2 = static_cast<RegOperand&>(defInsn->GetOperand(kInsnThirdOpnd));
        regno_t defSrcRegNo1 = defSrcRegOpnd1.GetRegisterNumber();
        regno_t defSrcRegNo2 = defSrcRegOpnd2.GetRegisterNumber();
        if (!CheckSrcReg(defSrcRegNo1, validNum) || !CheckSrcReg(defSrcRegNo2, validNum)) {
          return false;
        }
        break;
      }
      case MOP_wldrb: {
        if (validNum != k8BitSize) {
          return false;
        }
        break;
      }
      case MOP_wldrh: {
        if (validNum != k16BitSize) {
          return false;
        }
        break;
      }
      default:
        return false;
    }
  }
  return true;
}

bool ExtendMovPattern::BitNotAffected(const Insn &insn, uint32 validNum) {
  RegOperand &firstOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  RegOperand &secondOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  regno_t desRegNo = firstOpnd.GetRegisterNumber();
  regno_t srcRegNo = secondOpnd.GetRegisterNumber();
  VRegVersion *useVersion = optSsaInfo->FindSSAVersion(desRegNo);
  CHECK_FATAL(useVersion != nullptr, "useVRegVersion must not be null based on ssa");
  DUInsnInfo *defInfo = useVersion->GetDefInsnInfo();
  if (defInfo == nullptr) {
    return false;
  }
  if (!CheckSrcReg(srcRegNo, validNum)) {
    return false;
  }
  replaceMop = MOP_wmovrr;
  return true;
}

bool ExtendMovPattern::CheckCondition(Insn &insn) {
  MOperator mOp = insn.GetMachineOpcode();
  switch (mOp) {
    case MOP_xuxtb32: return BitNotAffected(insn, k8BitSize);
    case MOP_xuxth32: return BitNotAffected(insn, k16BitSize);
    default: return false;
  }
}

/* No initialization required */
void ExtendMovPattern::Init() {
  replaceMop = MOP_undef;
}

void ExtendMovPattern::Optimize(Insn &insn) {
  insn.SetMOperator(replaceMop);
}

void CopyRegProp::Run() {
  FOR_ALL_BB(bb, &cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      Init();
      if (!CheckCondition(*insn)) {
        continue;
      }
      Optimize(*insn);
    }
  }
}

bool CopyRegProp::IsValidCopyProp(RegOperand &dstReg, RegOperand &srcReg) {
  ASSERT(destVersion != nullptr, "find destVersion failed");
  ASSERT(srcVersion != nullptr, "find srcVersion failed");
  if (destVersion->GetOriginalRegNO() == srcVersion->GetOriginalRegNO()) {
    return true;
  }
  regno_t dstRegNO = dstReg.GetRegisterNumber();
  regno_t srcRegNO = srcReg.GetRegisterNumber();
  for (auto useDUInfoIt : destVersion->GetAllUseInsns()) {
    if (useDUInfoIt.second == nullptr) {
      continue;
    }
    Insn *useInsn = (useDUInfoIt.second)->GetInsn();
    if (useInsn == nullptr) {
      continue;
    }
    BB *useBB = useInsn->GetBB();
    if (((useBB->IsInPhiDef(srcRegNO) || useBB->IsInPhiList(srcRegNO)) && useBB->HasCriticalEdge()) ||
        useBB->IsInPhiList(dstRegNO)) {
      return false;
    }
  }
  return true;
}

bool CopyRegProp::CheckCondition(Insn &insn) {
  if (insn.IsEffectiveCopy()) {
    MOperator mOp = insn.GetMachineOpcode();
    if (mOp == MOP_xmovrr || mOp == MOP_wmovrr || mOp == MOP_xvmovs || mOp == MOP_xvmovd) {
      Operand &destOpnd = insn.GetOperand(kInsnFirstOpnd);
      Operand &srcOpnd = insn.GetOperand(kInsnSecondOpnd);
      ASSERT(destOpnd.IsRegister() && srcOpnd.IsRegister(), "must be");
      auto &destReg = static_cast<RegOperand &>(destOpnd);
      auto &srcReg = static_cast<RegOperand &>(srcOpnd);
      CHECK_FATAL(srcReg.GetRegisterNumber() != RZR, "CHECK ZERO REGISTER");
      if (destReg.IsSSAForm() && srcReg.IsSSAForm()) {
        /* case for ExplicitExtendProp  */
        if (destReg.GetSize() != srcReg.GetSize()) {
          VaildateImplicitCvt(destReg, srcReg, insn);
          return false;
        }
        if (destReg.GetValidBitsNum() >= srcReg.GetValidBitsNum()) {
          destReg.SetValidBitsNum(srcReg.GetValidBitsNum());
        } else {
          CHECK_FATAL(false, "do not support explicit extract bit in mov");
          return false;
        }
        destVersion = optSsaInfo->FindSSAVersion(destReg.GetRegisterNumber());
        ASSERT(destVersion != nullptr, "find Version failed");
        srcVersion = optSsaInfo->FindSSAVersion(srcReg.GetRegisterNumber());
        ASSERT(srcVersion != nullptr, "find Version failed");
        if (!IsValidCopyProp(destReg, srcReg)) {
          return false;
        }
        return true;
      } else {
        /* should be eliminated by ssa peep */
      }
    }
  }
  return false;
}

void CopyRegProp::Optimize(Insn &insn) {
  optSsaInfo->ReplaceAllUse(destVersion, srcVersion);
}

void CopyRegProp::VaildateImplicitCvt(RegOperand &destReg, const RegOperand &srcReg, Insn &movInsn) {
  ASSERT(movInsn.GetMachineOpcode() == MOP_xmovrr || movInsn.GetMachineOpcode() == MOP_wmovrr, "NIY explicit CVT");
  if (destReg.GetSize() == k64BitSize && srcReg.GetSize() == k32BitSize) {
    movInsn.SetMOperator(MOP_xuxtw64);
  } else if (destReg.GetSize() == k32BitSize && srcReg.GetSize() == k64BitSize) {
    movInsn.SetMOperator(MOP_xubfxrri6i6);
    movInsn.AddOperand(cgFunc.CreateImmOperand(PTY_i64, 0));
    movInsn.AddOperand(cgFunc.CreateImmOperand(PTY_i64, k32BitSize));
  } else {
    CHECK_FATAL(false, " unknown explicit integer cvt,  need implement in ssa prop ");
  }
  destReg.SetValidBitsNum(k32BitSize);
}

void RedundantPhiProp::Run() {
  FOR_ALL_BB(bb, &cgFunc) {
    for (auto phiIt : bb->GetPhiInsns()) {
      Init();
      if (!CheckCondition(*phiIt.second)) {
        continue;
      }
      Optimize(*phiIt.second);
    }
  }
}

void RedundantPhiProp::Optimize(Insn &insn) {
  optSsaInfo->ReplaceAllUse(destVersion, srcVersion);
}

bool RedundantPhiProp::CheckCondition(Insn &insn) {
  ASSERT(insn.IsPhi(), "must be phi insn here");
  auto &phiOpnd = static_cast<PhiOperand&>(insn.GetOperand(kInsnSecondOpnd));
  if (phiOpnd.IsRedundancy()) {
    auto &phiDestReg = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
    destVersion = optSsaInfo->FindSSAVersion(phiDestReg.GetRegisterNumber());
    ASSERT(destVersion != nullptr, "find Version failed");
    uint32 srcRegNO = phiOpnd.GetOperands().begin()->second->GetRegisterNumber();
    srcVersion = optSsaInfo->FindSSAVersion(srcRegNO);
    ASSERT(srcVersion != nullptr, "find Version failed");
    return true;
  }
  return false;
}

bool ValidBitNumberProp::CheckCondition(Insn &insn) {
  /* extend to all shift pattern in future */
  RegOperand *destOpnd = nullptr;
  RegOperand *srcOpnd = nullptr;
  if (insn.GetMachineOpcode() == MOP_xuxtw64) {
    destOpnd = &static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
    srcOpnd = &static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  }
  if (insn.GetMachineOpcode() == MOP_xubfxrri6i6) {
    destOpnd= &static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
    srcOpnd= &static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
    auto &lsb = static_cast<AArch64ImmOperand &>(insn.GetOperand(kInsnThirdOpnd));
    auto &width = static_cast<AArch64ImmOperand &>(insn.GetOperand(kInsnFourthOpnd));
    if ((lsb.GetValue() != 0) || (width.GetValue() != k32BitSize)) {
      return false;
    }
  }
  if (destOpnd != nullptr && destOpnd->IsSSAForm() &&
      srcOpnd != nullptr && srcOpnd->IsSSAForm()) {
    destVersion = optSsaInfo->FindSSAVersion(destOpnd->GetRegisterNumber());
    ASSERT(destVersion != nullptr, "find Version failed");
    srcVersion = optSsaInfo->FindSSAVersion(srcOpnd->GetRegisterNumber());
    ASSERT(srcVersion != nullptr, "find Version failed");
    if (destVersion->HasImplicitCvt()) {
      return false;
    }
    for (auto destUseIt : destVersion->GetAllUseInsns()) {
      Insn *useInsn = destUseIt.second->GetInsn();
      if (useInsn->GetMachineOpcode() == MOP_xuxtw64) {
        return false;
      }
    }
    srcVersion->SetImplicitCvt();
    return true;
  }
  return false;
}

void ValidBitNumberProp::Optimize(Insn &insn) {
  optSsaInfo->ReplaceAllUse(destVersion, srcVersion);
  cgFunc.InsertExtendSet(srcVersion->GetSSAvRegOpnd()->GetRegisterNumber());
}

void ValidBitNumberProp::Run() {
  FOR_ALL_BB(bb, &cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      Init();
      if (!CheckCondition(*insn)) {
        continue;
      }
      Optimize(*insn);
    }
  }
}

void FpSpConstProp::Run() {
  FOR_ALL_BB(bb, &cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      Init();
      if (!CheckCondition(*insn)) {
        continue;
      }
      Optimize(*insn);
    }
  }
}

bool FpSpConstProp::CheckCondition(Insn &insn) {
  auto &a64Insn = static_cast<AArch64Insn&>(insn);
  std::set<uint32> defRegs = a64Insn.GetDefRegs();
  auto &a64CGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  if (defRegs.size() <= 1) {
    if (a64Insn.IsRegDefOrUse(RSP)) {
      fpSpBase = &a64CGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
      /* not safe due to varied sp in alloca */
      if (cgFunc.HasVLAOrAlloca()) {
        return false;
      }
    }
    if (a64Insn.IsRegDefOrUse(RFP)) {
      ASSERT(fpSpBase == nullptr, " unexpect for both sp fp using ");
      fpSpBase = &a64CGFunc.GetOrCreatePhysicalRegisterOperand(RFP, k64BitSize, kRegTyInt);
    }
    if (fpSpBase == nullptr) {
      return false;
    }
    if (insn.GetMachineOpcode() == MOP_xaddrri12) {
      aT = kAArch64Add;
      if (GetValidSSAInfo(insn.GetOperand(kInsnFirstOpnd))) {
        shiftOpnd = &static_cast<AArch64ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
        return true;
      }
    } else if (insn.GetMachineOpcode() == MOP_xsubrri12) {
      aT = kAArch64Sub;
      if (GetValidSSAInfo(insn.GetOperand(kInsnFirstOpnd))) {
        shiftOpnd = &static_cast<AArch64ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
        return true;
      }
    }
  }
  return false;
}

bool FpSpConstProp::GetValidSSAInfo(Operand &opnd) {
  if (opnd.IsRegister()) {
    auto &regOpnd = static_cast<RegOperand&>(opnd);
    if (regOpnd.IsSSAForm()) {
      replaced = optSsaInfo->FindSSAVersion(regOpnd.GetRegisterNumber());
      ASSERT(replaced != nullptr, "find ssa version failed in FpSpConstProp");
      return true;
    }
  }
  return false;
}

int64 FpSpConstProp::ArithmeticFold(int64 valInUse, ArithmeticType useAT) const {
  int64 valInDef = shiftOpnd->GetValue();
  int64 returnVal = 0;
  CHECK_FATAL(aT == kAArch64Add || aT == kAArch64Sub, "unsupport sp/fp arthimetic  in aarch64");
  if (useAT == aT) {
    returnVal = valInUse + valInDef;
  } else {
    returnVal = valInUse - valInDef;
  }
  return returnVal;
}

void FpSpConstProp::PropInMem(DUInsnInfo &useDUInfo, Insn &useInsn) {
  MOperator useMop = useInsn.GetMachineOpcode();
  if (useInsn.IsStore() || useInsn.IsLoad()) {
    if (useDUInfo.GetOperands().size() == 1) {
      auto useOpndIt = useDUInfo.GetOperands().begin();
      if (useOpndIt->first == kInsnSecondOpnd || useOpndIt->first == kInsnThirdOpnd) {
        ASSERT(useOpndIt->second == 1, "multiple use in memory opnd");
        auto *a64memOpnd = static_cast<AArch64MemOperand*>(useInsn.GetMemOpnd());
        if (a64memOpnd->IsIntactIndexed() && a64memOpnd->GetAddrMode() == AArch64MemOperand::kAddrModeBOi) {
          auto *ofstOpnd = static_cast<AArch64OfstOperand*>(a64memOpnd->GetOffsetImmediate());
          CHECK_FATAL(ofstOpnd != nullptr, "oldOffsetOpnd is null");
          int64 newVal = ArithmeticFold(ofstOpnd->GetValue(), kAArch64Add);
          auto *newOfstImm = cgFunc.GetMemoryPool()->New<AArch64OfstOperand>(newVal, k64BitSize);
          if (ofstOpnd->GetVary() == kUnAdjustVary || shiftOpnd->GetVary() == kUnAdjustVary) {
            newOfstImm->SetVary(kUnAdjustVary);
          }
          auto *newMem = cgFunc.GetMemoryPool()->New<AArch64MemOperand>(
              AArch64MemOperand::kAddrModeBOi, a64memOpnd->GetSize(), *fpSpBase,
              nullptr, newOfstImm, nullptr);
          if (static_cast<AArch64CGFunc&>(cgFunc).IsOperandImmValid(useMop, newMem, useOpndIt->first)) {
            useInsn.SetMemOpnd(newMem);
            useDUInfo.DecreaseDU(useOpndIt->first);
            replaced->CheckDeadUse(useInsn);
          }
        }
      }
    } else {
      CHECK_FATAL(false, "NYI");
    }
  }
}

void FpSpConstProp::PropInArith(DUInsnInfo &useDUInfo, Insn &useInsn, ArithmeticType curAT) {
  if (useDUInfo.GetOperands().size() == 1) {
    auto &a64cgFunc = static_cast<AArch64CGFunc&>(cgFunc);
    MOperator useMop = useInsn.GetMachineOpcode();
    ASSERT(useDUInfo.GetOperands().begin()->first == kInsnSecondOpnd, "NIY");
    ASSERT(useDUInfo.GetOperands().begin()->second == 1, "multiple use in add/sub");
    auto &curVal = static_cast<AArch64ImmOperand&>(useInsn.GetOperand(kInsnThirdOpnd));
    AArch64ImmOperand &newVal = a64cgFunc.CreateImmOperand(ArithmeticFold(curVal.GetValue(), curAT),
                                                           curVal.GetSize(), false);
    if (newVal.GetValue() < 0) {
      newVal.Negate();
      useMop = A64ConstProp::GetReversalMOP(useMop);
    }
    if (curVal.GetVary() == kUnAdjustVary || shiftOpnd->GetVary() == kUnAdjustVary) {
      newVal.SetVary(kUnAdjustVary);
    }
    if (static_cast<AArch64CGFunc&>(cgFunc).IsOperandImmValid(useMop, &newVal, kInsnThirdOpnd)) {
      Insn &newInsn =
          cgFunc.GetCG()->BuildInstruction<AArch64Insn>(useMop, useInsn.GetOperand(kInsnFirstOpnd), *fpSpBase, newVal);
      useInsn.GetBB()->ReplaceInsn(useInsn, newInsn);
      optSsaInfo->ReplaceInsn(useInsn, newInsn);
    }
  } else {
    CHECK_FATAL(false, "NYI");
  }
}

void FpSpConstProp::PropInCopy(DUInsnInfo &useDUInfo, Insn &useInsn, MOperator oriMop) {
  if (useDUInfo.GetOperands().size() == 1) {
    ASSERT(useDUInfo.GetOperands().begin()->first == kInsnSecondOpnd, "NIY");
    ASSERT(useDUInfo.GetOperands().begin()->second == 1, "multiple use in add/sub");
    auto &newVal = *static_cast<AArch64ImmOperand*>(shiftOpnd->Clone(*cgFunc.GetMemoryPool()));
    Insn &newInsn = cgFunc.GetCG()->BuildInstruction<AArch64Insn>(
        oriMop, useInsn.GetOperand(kInsnFirstOpnd), *fpSpBase, newVal);
    useInsn.GetBB()->ReplaceInsn(useInsn, newInsn);
    optSsaInfo->ReplaceInsn(useInsn, newInsn);
  } else {
    CHECK_FATAL(false, "NYI");
  }
}

void FpSpConstProp::Optimize(Insn &insn) {
  for (auto &useInsnInfo : replaced->GetAllUseInsns()) {
    Insn *useInsn = useInsnInfo.second->GetInsn();
    MOperator useMop = useInsn->GetMachineOpcode();
    PropInMem(*useInsnInfo.second, *useInsn);
    switch (useMop) {
      case MOP_xmovrr:
      case MOP_wmovrr:
        PropInCopy(*useInsnInfo.second, *useInsn, insn.GetMachineOpcode());
        break;
      case MOP_xaddrri12:
        PropInArith(*useInsnInfo.second, *useInsn, kAArch64Add);
        break;
      case MOP_xsubrri12:
        PropInArith(*useInsnInfo.second, *useInsn, kAArch64Sub);
        break;
      default:
        break;
    }
  }
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
  auto *cpyMem = static_cast<AArch64MemOperand*>(a64memOpnd->Clone(tempMemPool));
  if (cpyMem->GetBaseRegister() != nullptr &&
      cpyMem->GetBaseRegister()->GetRegisterNumber() == oldReg->GetRegisterNumber()) {
    cpyMem->SetBaseRegister(*static_cast<AArch64RegOperand*>(newReg));
    changed = true;
  }
  if (cpyMem->GetIndexRegister() != nullptr &&
      cpyMem->GetIndexRegister()->GetRegisterNumber() == oldReg->GetRegisterNumber()) {
    CHECK_FATAL(!changed, "base reg is equal to index reg");
    cpyMem->SetIndexRegister(*newReg);
    changed = true;
  }
  if (changed) {
    insn->SetMemOpnd(&static_cast<AArch64CGFunc*>(cgFunc)->GetOrCreateMemOpnd(*cpyMem));
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
  auto &phiDest = static_cast<RegOperand&>(insn->GetOperand(kInsnFirstOpnd));
  if (phiDest.GetValidBitsNum() > v->GetLeastCommonValidBit()) {
    phiDest.SetValidBitsNum(v->GetLeastCommonValidBit());
  }
}
}

