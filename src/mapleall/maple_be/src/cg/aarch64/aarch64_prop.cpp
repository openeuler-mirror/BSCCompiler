/*
 * Copyright (c) [2021-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include <climits>
#include "aarch64_isa.h"
#include "aarch64_cg.h"
#include "aarch64_reg_coalesce.h"
#include "aarch64_prop.h"

namespace maplebe {

#define PROP_DUMP CG_DEBUG_FUNC(cgFunc)

bool MayOverflow(const ImmOperand &value1, const ImmOperand &value2, bool is64Bit, bool isAdd, bool isSigned) {
  if (value1.GetVary() > 0 || value2.GetVary() > 0) {
    return false;
  }
  int64 cstA = value1.GetValue();
  int64 cstB = value2.GetValue();
  if (isAdd) {
    int64 res = static_cast<int64>(static_cast<uint64>(cstA) + static_cast<uint64>(cstB));
    if (!isSigned) {
      return static_cast<uint64>(res) < static_cast<uint64>(cstA);
    }
    uint32 rightShiftNumToGetSignFlag = (is64Bit ? k64BitSize : k32BitSize) - 1;
    return (static_cast<uint64>(res) >> rightShiftNumToGetSignFlag !=
            static_cast<uint64>(cstA) >> rightShiftNumToGetSignFlag) &&
           (static_cast<uint64>(res) >> rightShiftNumToGetSignFlag !=
            static_cast<uint64>(cstB) >> rightShiftNumToGetSignFlag);
  } else {
    /* sub */
    if (!isSigned) {
      return cstA < cstB;
    }
    int64 res = static_cast<int64>(static_cast<uint64>(cstA) - static_cast<uint64>(cstB));
    uint32 rightShiftNumToGetSignFlag = (is64Bit ? k64BitSize : k32BitSize) - 1;
    return (static_cast<uint64>(cstA) >> rightShiftNumToGetSignFlag !=
            static_cast<uint64>(cstB) >> rightShiftNumToGetSignFlag) &&
           (static_cast<uint64>(res) >> rightShiftNumToGetSignFlag !=
            static_cast<uint64>(cstA) >> rightShiftNumToGetSignFlag);
  }
}

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
  PropOptimizeManager::Optimize<CopyRegProp>(*cgFunc, GetSSAInfo(), GetRegll());
  PropOptimizeManager::Optimize<RedundantPhiProp>(*cgFunc, GetSSAInfo());
}

void AArch64Prop::TargetProp(Insn &insn) {
  A64ConstProp a64ConstProp(*memPool, *cgFunc, *GetSSAInfo(), insn);
  a64ConstProp.DoOpt();
  A64StrLdrProp a64StrLdrProp(*memPool, *cgFunc, *GetSSAInfo(), insn, *GetDce());
  a64StrLdrProp.DoOpt();
}

void A64ConstProp::DoOpt() {
  if (curInsn->GetMachineOpcode() == MOP_wmovri32 || curInsn->GetMachineOpcode() == MOP_xmovri64) {
    Operand &destOpnd = curInsn->GetOperand(kInsnFirstOpnd);
    CHECK_FATAL(destOpnd.IsRegister(), "must be reg operand");
    auto &destReg = static_cast<RegOperand&>(destOpnd);
    if (destReg.IsSSAForm()) {
      VRegVersion *destVersion = ssaInfo->FindSSAVersion(destReg.GetRegisterNumber());
      ASSERT(destVersion != nullptr, "find Version failed");
      Operand &constOpnd = curInsn->GetOperand(kInsnSecondOpnd);
      CHECK_FATAL(constOpnd.IsImmediate(), "must be imm operand");
      auto &immOperand = static_cast<ImmOperand&>(constOpnd);
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

void A64ConstProp::ZeroRegProp(DUInsnInfo &useDUInfo, RegOperand &toReplaceReg) const {
  auto *useInsn = useDUInfo.GetInsn();
  const InsnDesc *md = &AArch64CG::kMd[(useInsn->GetMachineOpcode())];
  /* special case */
  bool isSpecficCase = useInsn->GetMachineOpcode() == MOP_wbfirri5i5 || useInsn->GetMachineOpcode() == MOP_xbfirri6i6;
  isSpecficCase = (useDUInfo.GetOperands().size() == 1) &&
      (useDUInfo.GetOperands().begin()->first == kInsnSecondOpnd) && isSpecficCase;
  if (useInsn->IsStore() || md->IsCondDef() || isSpecficCase)  {
    RegOperand &zeroOpnd = cgFunc->GetZeroOpnd(toReplaceReg.GetSize());
    for (auto &opndIt : as_const(useDUInfo.GetOperands())) {
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
    case MOP_xandrrr:
    case MOP_xandrrrs:
      return MOP_xandrri13;
    case MOP_wandrrr:
    case MOP_wandrrrs:
      return MOP_wandrri12;
    case MOP_xeorrrr:
    case MOP_xeorrrrs:
      return MOP_xeorrri13;
    case MOP_weorrrr:
    case MOP_weorrrrs:
      return MOP_weorrri12;
    case MOP_xiorrrr:
    case MOP_xiorrrrs:
    case MOP_xbfirri6i6:
      return MOP_xiorrri13;
    case MOP_wiorrrr:
    case MOP_wiorrrrs:
    case MOP_wbfirri5i5:
      return MOP_wiorrri12;
    case MOP_xmovrr:
    case MOP_xlslrri6:
    case MOP_xlsrrri6:
    case MOP_xasrrri6: {
      return MOP_xmovri64;
    }
    case MOP_wmovrr:
    case MOP_wlslrri5:
    case MOP_wlsrrri5:
    case MOP_wasrrri5: {
      return MOP_wmovri32;
    }
    default:
      CHECK_FATAL(false, "NYI");
      break;
  }
  return MOP_undef;
}

MOperator A64ConstProp::GetFoldMopAndVal(int64 &newVal, int64 constVal, const Insn &arithInsn) {
  MOperator arithMop = arithInsn.GetMachineOpcode();
  MOperator newMop = MOP_undef;
  switch (arithMop) {
    case MOP_waddrrr:
    case MOP_xaddrrr: {
      // If overflow, assign the overflowed value to newVal
      if (constVal < 0 && constVal < INT64_MIN - constVal) {
        newVal = 2 * (INT64_MAX + constVal + 1);
      } else if (constVal > 0 && constVal > INT64_MAX - constVal) {
        newVal = 2 * (INT64_MIN + constVal);
      } else {
        newVal = constVal + constVal;
      }
      newMop = (arithMop == MOP_waddrrr) ? MOP_wmovri32 : MOP_xmovri64;
      break;
    }
    case MOP_waddrrrs:
    case MOP_xaddrrrs: {
      auto &shiftOpnd = static_cast<BitShiftOperand&>(arithInsn.GetOperand(kInsnFourthOpnd));
      uint32 amount = shiftOpnd.GetShiftAmount();
      BitShiftOperand::ShiftOp sOp = shiftOpnd.GetShiftOp();
      switch (sOp) {
        case BitShiftOperand::kShiftLSL: {
          newVal = constVal + static_cast<int64>((static_cast<uint64>(constVal)) << amount);
          break;
        }
        case BitShiftOperand::kShiftLSR: {
          newVal = constVal + (static_cast<unsigned>(constVal) >> amount);
          break;
        }
        case BitShiftOperand::kShiftASR: {
          newVal = constVal + (static_cast<unsigned>(constVal) >> amount);
          break;
        }
        default:
          CHECK_FATAL(false, "NYI");
          break;
      }
      newMop = (arithMop == MOP_waddrrrs) ? MOP_wmovri32 : MOP_xmovri64;
      break;
    }
    case MOP_wsubrrr:
    case MOP_xsubrrr: {
      newVal = 0;
      newMop = (arithMop == MOP_wsubrrr) ? MOP_wmovri32 : MOP_xmovri64;
      break;
    }
    case MOP_wsubrrrs:
    case MOP_xsubrrrs: {
      auto &shiftOpnd = static_cast<BitShiftOperand&>(arithInsn.GetOperand(kInsnFourthOpnd));
      uint32 amount = shiftOpnd.GetShiftAmount();
      BitShiftOperand::ShiftOp sOp = shiftOpnd.GetShiftOp();
      switch (sOp) {
        case BitShiftOperand::kShiftLSL: {
          newVal = constVal - static_cast<int64>((static_cast<uint64>(constVal)) << amount);
          break;
        }
        case BitShiftOperand::kShiftLSR: {
          newVal = constVal - static_cast<int64>((static_cast<uint64>(constVal) >> amount));
          break;
        }
        case BitShiftOperand::kShiftASR: {
          newVal = constVal - static_cast<int64>((static_cast<uint64>(constVal) >> amount));
          break;
        }
        default:
          CHECK_FATAL(false, "NYI");
          break;
      }
      newMop = (arithMop == MOP_wsubrrrs) ? MOP_wmovri32 : MOP_xmovri64;
      break;
    }
    default:
      return MOP_undef;
  }
  return newMop;
}

void A64ConstProp::ReplaceInsnAndUpdateSSA(Insn &oriInsn, Insn &newInsn) const {
  ssaInfo->ReplaceInsn(oriInsn, newInsn);
  oriInsn.GetBB()->ReplaceInsn(oriInsn, newInsn);
  /* dump insn replacement here */
}

bool A64ConstProp::MovConstReplace(DUInsnInfo &useDUInfo, ImmOperand &constOpnd) const {
  Insn *useInsn = useDUInfo.GetInsn();
  MOperator curMop = useInsn->GetMachineOpcode();
  if (useDUInfo.GetOperands().size() == 1) {
    MOperator newMop = GetRegImmMOP(curMop, false);
    Operand &destOpnd = useInsn->GetOperand(kInsnFirstOpnd);
    if (constOpnd.IsSingleInstructionMovable(destOpnd.GetSize())) {
      auto useOpndInfoIt = useDUInfo.GetOperands().cbegin();
      uint32 useOpndIdx = useOpndInfoIt->first;
      ASSERT(useOpndIdx == kInsnSecondOpnd, "invalid instruction in ssa form");
      if (useOpndIdx == kInsnSecondOpnd) {
        Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(newMop, destOpnd, constOpnd);
        ReplaceInsnAndUpdateSSA(*useInsn, newInsn);
        return true;
      }
    }
  } else {
    ASSERT(false, "invalid instruction in ssa form");
  }
  return false;
}

bool A64ConstProp::ArithConstReplaceForOneOpnd(Insn &useInsn, DUInsnInfo &useDUInfo,
                                               ImmOperand &constOpnd, ArithmeticType aT) {
  MOperator curMop = useInsn.GetMachineOpcode();
  MOperator newMop = GetRegImmMOP(curMop, false);
  auto useOpndInfoIt = useDUInfo.GetOperands().cbegin();
  uint32 useOpndIdx = useOpndInfoIt->first;
  CHECK_FATAL(useOpndIdx == kInsnSecondOpnd || useOpndIdx == kInsnThirdOpnd, "check this insn");
  Insn *newInsn = nullptr;
  if (static_cast<AArch64CGFunc*>(cgFunc)->IsOperandImmValid(newMop, &constOpnd, kInsnThirdOpnd)) {
    if (useOpndIdx == kInsnThirdOpnd) {
      newInsn = &cgFunc->GetInsnBuilder()->BuildInsn(
          newMop, useInsn.GetOperand(kInsnFirstOpnd), useInsn.GetOperand(kInsnSecondOpnd), constOpnd);
    } else if (useOpndIdx == kInsnSecondOpnd && aT == kAArch64Add) { /* swap operand due to legality in aarch */
      newInsn = &cgFunc->GetInsnBuilder()->BuildInsn(
          newMop, useInsn.GetOperand(kInsnFirstOpnd), useInsn.GetOperand(kInsnThirdOpnd), constOpnd);
    }
  }
  /* try aggressive opt in aarch64 add and sub */
  if (newInsn == nullptr && (aT == kAArch64Add || aT == kAArch64Sub)) {
    auto *tempImm = static_cast<ImmOperand*>(constOpnd.Clone(*constPropMp));
    /* try aarch64 imm shift mode */
    tempImm->SetValue(static_cast<int64>(static_cast<uint64>(tempImm->GetValue()) >> 12));
    // ISSUE233: It can prop after 12 shifts to the right.
    auto *zeroImm = &(static_cast<AArch64CGFunc*>(cgFunc)->
      CreateImmOperand(0, constOpnd.GetSize(), true));
    /* value in immOpnd is signed */
    if (MayOverflow(*zeroImm, constOpnd, constOpnd.GetSize() == 64, false, true)) {
      return false;
    }
    /* (constA - var) can not reversal to (var + (-constA)) */
    if (useOpndIdx == kInsnSecondOpnd && aT == kAArch64Sub) {
      return false;
    }
    /* Addition and subtraction reversal */
    tempImm->SetValue(-constOpnd.GetValue());
    newMop = GetReversalMOP(newMop);
    if (static_cast<AArch64CGFunc*>(cgFunc)->IsOperandImmValid(newMop, tempImm, kInsnThirdOpnd)) {
      auto *cgImm = static_cast<ImmOperand*>(tempImm->Clone(*cgFunc->GetMemoryPool()));
      newInsn = &cgFunc->GetInsnBuilder()->BuildInsn(
          newMop, useInsn.GetOperand(kInsnFirstOpnd), useInsn.GetOperand(kInsnSecondOpnd), *cgImm);
      if (useOpndIdx == kInsnSecondOpnd) { /* swap operand due to legality in aarch */
        newInsn->SetOperand(kInsnSecondOpnd, useInsn.GetOperand(kInsnThirdOpnd));
      }
    }
  }
  if (newInsn == nullptr) {
    return false;
  }
  ReplaceInsnAndUpdateSSA(useInsn, *newInsn);
  return true;
}

bool A64ConstProp::ArithmeticConstReplace(DUInsnInfo &useDUInfo, ImmOperand &constOpnd, ArithmeticType aT) {
  Insn *useInsn = useDUInfo.GetInsn();
  CHECK_FATAL(useInsn != nullptr, "get useInsn failed");
  if (useDUInfo.GetOperands().size() == 1) {
    return ArithConstReplaceForOneOpnd(*useInsn, useDUInfo, constOpnd, aT);
  } else if (useDUInfo.GetOperands().size() == 2) {
    /* only support add & sub now */
    int64 newValue = 0;
    MOperator newMop = GetFoldMopAndVal(newValue, constOpnd.GetValue(), *useInsn);
    if (newMop == MOP_undef) {
      return false;
    }
    bool isSigned = (newValue < 0);
    auto *tempImm = static_cast<ImmOperand*>(constOpnd.Clone(*constPropMp));
    tempImm->SetValue(newValue);
    tempImm->SetSigned(isSigned);
    if (tempImm->IsSingleInstructionMovable()) {
      auto *newImmOpnd = static_cast<ImmOperand*>(tempImm->Clone(*cgFunc->GetMemoryPool()));
      auto &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(
          newMop, useInsn->GetOperand(kInsnFirstOpnd), *newImmOpnd);
      ReplaceInsnAndUpdateSSA(*useInsn, newInsn);
      return true;
    }
  } else {
    ASSERT(false, "invalid instruction in ssa form");
  }
  return false;
}

bool A64ConstProp::ArithmeticConstFold(DUInsnInfo &useDUInfo, const ImmOperand &constOpnd,
                                       ArithmeticType aT) const {
  Insn *useInsn = useDUInfo.GetInsn();
  if (useDUInfo.GetOperands().size() == 1) {
    Operand &existedImm = useInsn->GetOperand(kInsnThirdOpnd);
    ASSERT(existedImm.IsImmediate(), "must be");
    Operand &destOpnd = useInsn->GetOperand(kInsnFirstOpnd);
    bool is64Bit = destOpnd.GetSize() == k64BitSize;
    ImmOperand *foldConst = CanDoConstFold(constOpnd, static_cast<ImmOperand&>(existedImm), aT, is64Bit);
    if (foldConst != nullptr) {
      MOperator newMop = is64Bit ? MOP_xmovri64 : MOP_wmovri32;
      Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(newMop, destOpnd, *foldConst);
      ReplaceInsnAndUpdateSSA(*useInsn, newInsn);
      return true;
    }
  }
  return false;
}

bool A64ConstProp::ShiftConstReplace(DUInsnInfo &useDUInfo, const ImmOperand &constOpnd) {
  Insn *useInsn = useDUInfo.GetInsn();
  MOperator curMop = useInsn->GetMachineOpcode();
  if (useDUInfo.GetOperands().size() == 1) {
    auto useOpndInfoIt = useDUInfo.GetOperands().cbegin();
    uint32 useOpndIdx = useOpndInfoIt->first;
    if (useOpndIdx == kInsnThirdOpnd) {
      auto &shiftBit = static_cast<BitShiftOperand&>(useInsn->GetOperand(kInsnFourthOpnd));
      int64 val = constOpnd.GetValue();
      if (shiftBit.GetShiftOp() == BitShiftOperand::kShiftLSL) {     // only have one useInsn
        val = static_cast<int64>(static_cast<uint64>(val) << shiftBit.GetShiftAmount());
      } else if (shiftBit.GetShiftOp() == BitShiftOperand::kShiftLSR) {
        val = static_cast<int64>(static_cast<uint64>(val) >> shiftBit.GetShiftAmount());
      } else if (shiftBit.GetShiftOp() == BitShiftOperand::kShiftASR) {
        val = static_cast<int64>(static_cast<uint64>(val) >> shiftBit.GetShiftAmount());
      } else {
        CHECK_FATAL(false, "shift type is not defined");
      }
      auto *newImm = static_cast<ImmOperand*>(constOpnd.Clone(*constPropMp));
      newImm->SetValue(val);
      MOperator newMop = GetRegImmMOP(curMop, false);
      if (static_cast<AArch64CGFunc*>(cgFunc)->IsOperandImmValid(newMop, newImm, kInsnThirdOpnd)) {
        auto *cgNewImm = static_cast<ImmOperand*>(constOpnd.Clone(*cgFunc->GetMemoryPool()));
        Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(
            newMop, useInsn->GetOperand(kInsnFirstOpnd), useInsn->GetOperand(kInsnSecondOpnd), *cgNewImm);
        ReplaceInsnAndUpdateSSA(*useInsn, newInsn);
        return true;
      }
    }
  }
  return false;
}

bool A64ConstProp::ConstProp(DUInsnInfo &useDUInfo, ImmOperand &constOpnd) {
  MOperator curMop = useDUInfo.GetInsn()->GetMachineOpcode();
  switch (curMop) {
    case MOP_xmovrr:
    case MOP_wmovrr: {
      return MovConstReplace(useDUInfo, constOpnd);
    }
    case MOP_xsubrrr:
    case MOP_wsubrrr: {
      return ArithmeticConstReplace(useDUInfo, constOpnd, kAArch64Sub);
    }
    case MOP_xaddrrr:
    case MOP_waddrrr: {
      return ArithmeticConstReplace(useDUInfo, constOpnd, kAArch64Add);
    }
    case MOP_xaddrri12:
    case MOP_waddrri12: {
      return ArithmeticConstFold(useDUInfo, constOpnd, kAArch64Add);
    }
    case MOP_xsubrri12:
    case MOP_wsubrri12: {
      return ArithmeticConstFold(useDUInfo, constOpnd, kAArch64Sub);
    }
    case MOP_xandrrr:
    case MOP_wandrrr:
    case MOP_xeorrrr:
    case MOP_weorrrr:
    case MOP_xiorrrr:
    case MOP_wiorrrr: {
      return ArithmeticConstReplace(useDUInfo, constOpnd, kAArch64Logic);
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
    case MOP_wbfirri5i5:
    case MOP_xbfirri6i6: {
      return BitInsertReplace(useDUInfo, constOpnd);
    }
    case MOP_wcmprr:
    case MOP_xcmprr: {
      return ReplaceCmpToCmnOrConstPropCmp(useDUInfo, constOpnd);
    }
    case MOP_wlslrri5:
    case MOP_wlsrrri5:
    case MOP_wasrrri5:
    case MOP_xlslrri6:
    case MOP_xlsrrri6:
    case MOP_xasrrri6:
      return MovLslConstToMov(useDUInfo, constOpnd);
    default:
      break;
  }
  return false;
}

/*
 * mov w1, #imm
 * cmp w0, w1
 *
 *
 * if imm < 0        ===> cmn w0, #imm
 * else if imm > 0   ===> cmp w0, #imm
 */
bool A64ConstProp::ReplaceCmpToCmnOrConstPropCmp(DUInsnInfo &useDUInfo, ImmOperand &constOpnd) {
  Insn *useInsn = useDUInfo.GetInsn();
  MOperator curMop = curInsn->GetMachineOpcode();
  MOperator useMop = useInsn->GetMachineOpcode();
  if (useDUInfo.GetOperands().size() != 1) {
    return false;
  }
  auto &cmpFirstOpnd = static_cast<RegOperand &>(useInsn->GetOperand(kInsnSecondOpnd));
  Operand &regFlag = useInsn->GetOperand(kInsnFirstOpnd);
  auto useOpndInfoIt = useDUInfo.GetOperands().cbegin();
  uint32 useOpndIdx = useOpndInfoIt->first;
  Insn *newInsn = nullptr;
  auto iVal = (curMop == MOP_wmovri32) ? static_cast<int32>(constOpnd.GetValue()) :
                                         static_cast<int64>(constOpnd.GetValue());
  if (iVal < 0 && iVal >= kNegativeImmLowerLimit) {
    iVal = iVal * (-1);
    MOperator newMop = useMop == MOP_wcmprr ? MOP_wcmnri : MOP_xcmnri;
    ImmOperand &newOpnd = static_cast<AArch64CGFunc*>(cgFunc)->CreateImmOperand(iVal, constOpnd.GetSize(),
                                                                                constOpnd.IsSignedValue());
    if (!static_cast<AArch64CGFunc*>(cgFunc)->IsOperandImmValid(newMop, &newOpnd, kInsnThirdOpnd)) {
      return false;
    }
    if (useOpndIdx == kInsnSecondOpnd) {
      return false;
    } else if (useOpndIdx == kInsnThirdOpnd) {
      newInsn = &cgFunc->GetInsnBuilder()->BuildInsn(newMop, regFlag, cmpFirstOpnd, newOpnd);
    } else {
      CHECK_FATAL(false, "Cannot replace the cmp bit cmn");
    }
    ReplaceInsnAndUpdateSSA(*useInsn, *newInsn);
    return true;
  } else if (iVal > 0) {
    MOperator newMop = useMop == MOP_wcmprr ? MOP_wcmpri : MOP_xcmpri;
    if (!static_cast<AArch64CGFunc*>(cgFunc)->IsOperandImmValid(newMop, static_cast<ImmOperand*>(&constOpnd),
                                                                kInsnThirdOpnd)) {
      return false;
    }
    if (useOpndIdx == kInsnSecondOpnd) {
      return false;
    } else if (useOpndIdx == kInsnThirdOpnd) {
      newInsn = &cgFunc->GetInsnBuilder()->BuildInsn(newMop, regFlag, cmpFirstOpnd,
                                                     *static_cast<ImmOperand*>(&constOpnd));
    } else {
      CHECK_FATAL(false, "Cannot prop const to cmp");
    }
    ReplaceInsnAndUpdateSSA(*useInsn, *newInsn);
    return true;
  }
  return false;
}

/*    mov	w1, #1
      lsl	w1, w1, #6  ===> mov w1, #xx
*/
bool A64ConstProp::MovLslConstToMov(DUInsnInfo &useDUInfo, const ImmOperand &constOpnd) {
  Insn *useInsn = useDUInfo.GetInsn();
  MOperator curMop = useInsn->GetMachineOpcode();
  if (useDUInfo.GetOperands().size() == 1) {  // only have one useInsn
    auto useOpndInfoIt = useDUInfo.GetOperands().cbegin();
    uint32 useOpndIdx = useOpndInfoIt->first;
    if (useOpndIdx == kInsnSecondOpnd) {
      auto &shiftBitImm = static_cast<ImmOperand &>(useInsn->GetOperand(kInsnThirdOpnd));
      int64 val = constOpnd.GetValue();
      uint64 shift = static_cast<uint64>(shiftBitImm.GetValue());
      constexpr uint32 k63Bits = 63;
      if (curMop == MOP_wlslrri5 || curMop == MOP_xlslrri6) {
        val = static_cast<int64>(static_cast<uint64>(val) << shift);
      } else if (curMop == MOP_wlsrrri5 || curMop == MOP_xlsrrri6 ||
                 ((curMop == MOP_wasrrri5 || curMop == MOP_xasrrri6) &&
                  ((static_cast<uint64>(val) & (1ULL << k63Bits)) == 0))) {
        val = static_cast<int64>(static_cast<uint64>(val) >> shift);
      } else if (((curMop == MOP_wasrrri5 || curMop == MOP_xasrrri6) &&
                 ((static_cast<uint64>(val) & (1ULL << k63Bits)) == (1ULL << k63Bits)))) {
        uint64 leadOnes = ~(ULLONG_MAX >> shift);
        val = static_cast<int64>((static_cast<uint64>(val) >> shift) | leadOnes);
      } else {
        CHECK_FATAL(false, "shift mop is not defined");
      }
      auto *newImm = static_cast<ImmOperand *>(constOpnd.Clone(*constPropMp));
      newImm->SetValue(val);
      MOperator newMop = GetRegImmMOP(curMop, false);
      if (static_cast<AArch64CGFunc *>(cgFunc)->IsOperandImmValid(newMop, newImm, kInsnSecondOpnd)) {
        auto *cgNewImm = static_cast<ImmOperand *>(newImm->Clone(*cgFunc->GetMemoryPool()));
        Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(newMop, useInsn->GetOperand(kInsnFirstOpnd), *cgNewImm);
        ReplaceInsnAndUpdateSSA(*useInsn, newInsn);
        return true;
      }
    }
  }
  return false;
}

bool A64ConstProp::BitInsertReplace(DUInsnInfo &useDUInfo, const ImmOperand &constOpnd) const {
  Insn *useInsn = useDUInfo.GetInsn();
  MOperator curMop = useInsn->GetMachineOpcode();
  if (useDUInfo.GetOperands().size() == 1) {
    auto useOpndInfoIt = useDUInfo.GetOperands().cbegin();
    uint32 useOpndIdx = useOpndInfoIt->first;
    if (useOpndIdx == kInsnSecondOpnd) {
      auto &lsbOpnd = static_cast<ImmOperand&>(useInsn->GetOperand(kInsnThirdOpnd));
      auto &widthOpnd = static_cast<ImmOperand&>(useInsn->GetOperand(kInsnFourthOpnd));
      auto val = static_cast<uint64>(constOpnd.GetValue());
      /* bfi width in the range [1 -64] */
      auto width = static_cast<uint64>(widthOpnd.GetValue());
      /*  bit number of the lsb of the destination bitfield */
      auto lsb = static_cast<uint64>(lsbOpnd.GetValue());
      val = val & ((1U << width) - 1U);
      if (__builtin_popcountl(val) == static_cast<int64>(width)) {
        val = val << lsb;
        MOperator newMop = GetRegImmMOP(curMop, false);
        Operand &newOpnd = cgFunc->CreateImmOperand(PTY_i64, static_cast<int64>(val));
        if (static_cast<AArch64CGFunc*>(cgFunc)->IsOperandImmValid(newMop, &newOpnd, kInsnThirdOpnd)) {
          RegOperand *defOpnd = useInsn->GetSSAImpDefOpnd();
          CHECK_FATAL(defOpnd, "check ssaInfo of the defOpnd");
          Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(
              newMop, *defOpnd, useInsn->GetOperand(kInsnFirstOpnd), newOpnd);
          ReplaceInsnAndUpdateSSA(*useInsn, newInsn);
          return true;
        }
      }
    }
  }
  return false;
}

ImmOperand *A64ConstProp::CanDoConstFold(
    const ImmOperand &value1, const ImmOperand &value2, ArithmeticType aT, bool is64Bit) const {
  auto *tempImm = static_cast<ImmOperand*>(value1.Clone(*constPropMp));
  int64 newVal = 0;
  bool isSigned = value1.IsSignedValue();
  if (value1.IsSignedValue() != value2.IsSignedValue()) {
    isSigned = false;
  }
  if (MayOverflow(value1, value2, is64Bit, aT == kAArch64Add, isSigned)) {
    return nullptr;
  }
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
  if (!is64Bit && isSigned && (newVal > INT_MAX || newVal < INT_MIN)) {
    return nullptr;
  }
  if (!is64Bit && !isSigned && (newVal > UINT_MAX || newVal < 0)) {
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
  return canBeMove ? static_cast<ImmOperand*>(tempImm->Clone(*cgFunc->GetMemoryPool())) : nullptr;
}

void A64StrLdrProp::DoOpt() {
  ASSERT(curInsn != nullptr, "not input insn");
  bool tryOptAgain = false;
  do {
    tryOptAgain = false;
    Init();
    MemOperand *currMemOpnd = StrLdrPropPreCheck(*curInsn);
    if (currMemOpnd != nullptr && memPropMode != kUndef) {
      /* can be changed to recursive propagation */
      if (ReplaceMemOpnd(*currMemOpnd)) {
        tryOptAgain = true;
      }
      replaceVersions.clear();
    }
  } while (tryOptAgain);
}

bool A64StrLdrProp::IsSameOpndsOfInsn(const Insn &insn1, const Insn &insn2, uint32 opndIdx) const {
  Operand &opnd = insn2.GetOperand(opndIdx);
  Operand::OperandType opndType = opnd.GetKind();
  switch (opndType) {
    case Operand::kOpdRegister: {
      if (!static_cast<RegOperand&>(opnd).Equals(static_cast<RegOperand&>(insn1.GetOperand(opndIdx)))) {
        return false;
      }
      break;
    }
    case Operand::kOpdImmediate: {
      if (!static_cast<ImmOperand&>(opnd).Equals(static_cast<ImmOperand&>(insn1.GetOperand(opndIdx)))) {
        return false;
      }
      break;
    }
    case Operand::kOpdExtend: {
      if (!static_cast<ExtendShiftOperand&>(opnd).Equals(
          static_cast<ExtendShiftOperand&>(insn1.GetOperand(opndIdx)))) {
        return false;
      }
      break;
    }
    case Operand::kOpdShift: {
      if (!static_cast<BitShiftOperand&>(opnd).Equals(
          static_cast<BitShiftOperand&>(insn1.GetOperand(opndIdx)))) {
        return false;
      }
      break;
    }
    default:
      return false;
  }
  return true;
}

bool A64StrLdrProp::IsPhiInsnValid(const Insn &phiInsn) {
  std::vector<Insn*> validDefInsns;
  auto &phiOpnd = static_cast<PhiOperand&>(phiInsn.GetOperand(kInsnSecondOpnd));
  for (const auto useIt : phiOpnd.GetOperands()) {
    ASSERT(useIt.second != nullptr, "get phiUseOpnd failed");
    Insn *defPhiInsn = ssaInfo->GetDefInsn(*useIt.second);
    /* check only one layer of phi */
    if (defPhiInsn == nullptr || defPhiInsn->IsPhi()) {
      return false;
    }
    (void)validDefInsns.emplace_back(defPhiInsn);
  }
  if (validDefInsns.empty()) {
    return false;
  }
  MOperator mOp = validDefInsns[0]->GetMachineOpcode();
  uint32 opndNum = validDefInsns[0]->GetOperandSize();
  for (uint32 insnIdx = 1; insnIdx < validDefInsns.size(); ++insnIdx) {
    Insn *insn = validDefInsns[insnIdx];
    if (insn->GetMachineOpcode() != mOp) {
      return false;
    }
    if (insn->GetOperandSize() != opndNum) {
      return false;
    }
    for (uint i = 0 ; i < opndNum; ++i) {
      if (insn->OpndIsDef(i)) {
        continue;
      }
      if (!IsSameOpndsOfInsn(*validDefInsns[0], *insn, i)) {
        return false;
      }
    }
  }
  defInsn = validDefInsns[0];
  return true;
}

Insn *A64StrLdrProp::GetDefInsn(const RegOperand &regOpnd, std::vector<Insn*> &allUseInsns) const {
  Insn *insn = nullptr;
  if (regOpnd.IsSSAForm()) {
    VRegVersion *replacedV = ssaInfo->FindSSAVersion(regOpnd.GetRegisterNumber());
    CHECK_FATAL(replacedV != nullptr, "replacedV should not be nullptr");
    if (replacedV->GetDefInsnInfo() != nullptr) {
      for (auto it : replacedV->GetAllUseInsns()) {
        (void)allUseInsns.emplace_back(it.second->GetInsn());
      }
      insn = replacedV->GetDefInsnInfo()->GetInsn();
    }
  }
  return insn;
}

bool A64StrLdrProp::ReplaceMemOpnd(const MemOperand &currMemOpnd) {
  RegOperand *replacedReg = nullptr;
  if (memPropMode == kPropBase) {
    replacedReg = currMemOpnd.GetBaseRegister();
  } else {
    Operand *offset = currMemOpnd.GetOffset();
    ASSERT(offset->IsRegister(), "must be");
    replacedReg = static_cast<RegOperand*>(offset);
  }
  CHECK_FATAL(replacedReg != nullptr, "check this insn");
  std::vector<Insn*> allUseInsns;
  std::vector<MemOperand*> newMemOpnds;
  defInsn = GetDefInsn(*replacedReg, allUseInsns);
  if (defInsn == nullptr) {
    return false;
  }
  if (defInsn->IsPhi() && !IsPhiInsnValid(*defInsn)) {
    return false;
  }
  for (auto useInsn : allUseInsns) {
    MemOperand *oldMemOpnd = StrLdrPropPreCheck(*useInsn);
    if (!CheckSameReplace(*replacedReg, oldMemOpnd)) {
      return false;
    }
    ASSERT(oldMemOpnd != nullptr, "oldMemOpnd should not be nullptr");
    MemOperand *newMemOpnd = SelectReplaceMem(*oldMemOpnd);
    if (newMemOpnd != nullptr) {
      uint32 opndIdx = GetMemOpndIdx(oldMemOpnd, *useInsn);
      if (CheckNewMemOffset(*useInsn, *newMemOpnd, opndIdx)) {
        newMemOpnds.emplace_back(newMemOpnd);
        continue;
      }
    }
    return false;
  }
  /* due to register pressure, do not do partial prop */
  for (size_t i = 0; i < newMemOpnds.size(); ++i) {
    DoMemReplace(*replacedReg, *newMemOpnds[i], *allUseInsns[i]);
  }
  return true;
}

bool A64StrLdrProp::CheckSameReplace(const RegOperand &replacedReg, const MemOperand *memOpnd) const {
  if (memOpnd != nullptr && memPropMode != kUndef) {
    if (memPropMode == kPropBase) {
      return replacedReg.GetRegisterNumber() ==  memOpnd->GetBaseRegister()->GetRegisterNumber();
    } else {
      Operand *offset = memOpnd->GetOffset();
      ASSERT(offset != nullptr, "offset should not be nullptr");
      ASSERT(offset->IsRegister(), "must be");
      return replacedReg.GetRegisterNumber() == static_cast<RegOperand*>(offset)->GetRegisterNumber();
    }
  }
  return false;
}

uint32 A64StrLdrProp::GetMemOpndIdx(MemOperand *newMemOpnd, const Insn &insn) const {
  uint32 opndIdx = kInsnMaxOpnd;
  if (insn.IsLoadPair() || insn.IsStorePair()) {
    ASSERT(newMemOpnd->GetOffsetImmediate() != nullptr, "unexpect insn");
    opndIdx = kInsnThirdOpnd;
  } else {
    opndIdx = kInsnSecondOpnd;
  }
  return opndIdx;
}

void A64StrLdrProp::DoMemReplace(const RegOperand &replacedReg, MemOperand &newMem, Insn &useInsn) {
  VRegVersion *replacedV = ssaInfo->FindSSAVersion(replacedReg.GetRegisterNumber());
  ASSERT(replacedV != nullptr, "must in ssa form");
  uint32 opndIdx = GetMemOpndIdx(&newMem, useInsn);
  replacedV->RemoveUseInsn(useInsn, opndIdx);
  if (replacedV->GetAllUseInsns().empty()) {
    (void)cgDce->RemoveUnuseDef(*replacedV);
  }
  for (auto &replaceit : as_const(replaceVersions)) {
    replaceit.second->AddUseInsn(*ssaInfo, useInsn, opndIdx);
  }
  useInsn.SetOperand(opndIdx, newMem);
}

MemOperand *A64StrLdrProp::StrLdrPropPreCheck(const Insn &insn) {
  memPropMode = kUndef;
  if (insn.IsLoad() || insn.IsStore()) {
    if (insn.IsAtomic()) {
      return nullptr;
    }
    auto *currMemOpnd = static_cast<MemOperand*>(insn.GetMemOpnd());
    if (currMemOpnd != nullptr) {
      memPropMode = SelectStrLdrPropMode(*currMemOpnd);
      if (memPropMode == kUndef) {
        return nullptr;
      }
      return currMemOpnd;
    }
  }
  return nullptr;
}

MemPropMode A64StrLdrProp::SelectStrLdrPropMode(const MemOperand &currMemOpnd) {
  MemOperand::AArch64AddressingMode currAddrMode = currMemOpnd.GetAddrMode();
  MemPropMode innerMemPropMode = kUndef;
  switch (currAddrMode) {
    case MemOperand::kBOI: {
      innerMemPropMode = kPropBase;
      break;
    }
    case MemOperand::kBOR: {
      innerMemPropMode = kPropOffset;
      break;
    }
    case MemOperand::kBOE: {
      if (currMemOpnd.SignedExtend()) {
        innerMemPropMode = kPropSignedExtend;
      } else if (currMemOpnd.UnsignedExtend()) {
        innerMemPropMode = kPropUnsignedExtend;
      }
      break;
    }
    case MemOperand::kBOL: {
      auto amount = currMemOpnd.ShiftAmount();
      CHECK_FATAL(amount != 0, "check currMemOpnd!");
      innerMemPropMode = kPropShift;
      break;
    }
    default:
      innerMemPropMode = kUndef;
  }
  return innerMemPropMode;
}

MemOperand *A64StrLdrProp::SelectReplaceMem(const MemOperand &currMemOpnd) {
  MemOperand *newMemOpnd = nullptr;
  Operand *offset = currMemOpnd.GetOffset();
  RegOperand *base = currMemOpnd.GetBaseRegister();
  MOperator opCode = defInsn->GetMachineOpcode();
  switch (opCode) {
    case MOP_xsubrri12:
    case MOP_wsubrri12: {
      RegOperand *replace = GetReplaceReg(static_cast<RegOperand&>(defInsn->GetOperand(kInsnSecondOpnd)));
      if (replace != nullptr) {
        auto &immOpnd = static_cast<ImmOperand&>(defInsn->GetOperand(kInsnThirdOpnd));
        // sub can not prop vary imm
        CHECK_FATAL(immOpnd.GetVary() != kUnAdjustVary, "NIY, imm wrong vary type");
        int64 defVal = -(immOpnd.GetValue());
        newMemOpnd = HandleArithImmDef(*replace, offset, defVal, currMemOpnd.GetSize());
      }
      break;
    }
    case MOP_xaddrri12:
    case MOP_waddrri12: {
      RegOperand *replace = GetReplaceReg(static_cast<RegOperand&>(defInsn->GetOperand(kInsnSecondOpnd)));
      if (replace != nullptr) {
        auto &immOpnd = static_cast<ImmOperand&>(defInsn->GetOperand(kInsnThirdOpnd));
        int64 defVal = immOpnd.GetValue();
        newMemOpnd =
            HandleArithImmDef(*replace, offset, defVal, currMemOpnd.GetSize(), immOpnd.GetVary());
      }
      break;
    }
    case MOP_xaddrri24:
    case MOP_waddrri24: {
      RegOperand *replace = GetReplaceReg(static_cast<RegOperand&>(defInsn->GetOperand(kInsnSecondOpnd)));
      if (replace != nullptr) {
        auto &immOpnd = static_cast<ImmOperand&>(defInsn->GetOperand(kInsnThirdOpnd));
        auto &shiftOpnd = static_cast<BitShiftOperand&>(defInsn->GetOperand(kInsnFourthOpnd));
        CHECK_FATAL(shiftOpnd.GetShiftAmount() == 12, "invalid shiftAmount");
        CHECK_FATAL(immOpnd.GetVary() != kUnAdjustVary, "NIY, imm wrong vary type");
        auto defVal = static_cast<int64>(static_cast<uint64>(immOpnd.GetValue()) << shiftOpnd.GetShiftAmount());
        newMemOpnd = HandleArithImmDef(*replace, offset, defVal, currMemOpnd.GetSize());
      }
      break;
    }
    case MOP_xsubrri24:
    case MOP_wsubrri24: {
      RegOperand *replace = GetReplaceReg(static_cast<RegOperand&>(defInsn->GetOperand(kInsnSecondOpnd)));
      if (replace != nullptr) {
        auto &immOpnd = static_cast<ImmOperand&>(defInsn->GetOperand(kInsnThirdOpnd));
        auto &shiftOpnd = static_cast<BitShiftOperand&>(defInsn->GetOperand(kInsnFourthOpnd));
        CHECK_FATAL(shiftOpnd.GetShiftAmount() == 12, "invalid shiftAmount");
        CHECK_FATAL(immOpnd.GetVary() != kUnAdjustVary, "NIY, imm wrong vary type");
        int64 defVal = -static_cast<int64>(static_cast<uint64>(immOpnd.GetValue()) << shiftOpnd.GetShiftAmount());
        newMemOpnd = HandleArithImmDef(*replace, offset, defVal, currMemOpnd.GetSize());
      }
      break;
    }
    case MOP_xaddrrr:
    case MOP_waddrrr:
    case MOP_dadd:
    case MOP_sadd: {
      if (memPropMode == kPropBase) {
        CHECK_NULL_FATAL(offset);
        auto *ofstOpnd = static_cast<ImmOperand*>(offset);
        if (!ofstOpnd->IsZero()) {
          break;
        }
        RegOperand *replace = GetReplaceReg(static_cast<RegOperand &>(defInsn->GetOperand(kInsnSecondOpnd)));
        RegOperand *newOfst = GetReplaceReg(static_cast<RegOperand &>(defInsn->GetOperand(kInsnThirdOpnd)));
        if (replace != nullptr && replace->GetSize() != k32BitSize && newOfst != nullptr) {
          if (replace->GetSize() == k64BitSize && newOfst->GetSize() == k32BitSize) {
            ExtendShiftOperand &exOpnd =
                static_cast<AArch64CGFunc*>(cgFunc)->CreateExtendShiftOperand(ExtendShiftOperand::kUXTW, 0, k8BitSize);
            newMemOpnd = static_cast<AArch64CGFunc*>(cgFunc)->CreateMemOperand(currMemOpnd.GetSize(), *replace,
                                                                               *newOfst, exOpnd);
          } else {
            newMemOpnd =
                static_cast<AArch64CGFunc*>(cgFunc)->CreateMemOperand(currMemOpnd.GetSize(), *replace, *newOfst);
          }
        }
      }
      break;
    }
    case MOP_xaddrrrs:
    case MOP_waddrrrs: {
      if (memPropMode == kPropBase) {
        CHECK_NULL_FATAL(offset);
        auto *ofstOpnd = static_cast<ImmOperand*>(offset);
        if (!ofstOpnd->IsZero()) {
          break;
        }
        RegOperand *newBaseOpnd = GetReplaceReg(
            static_cast<RegOperand&>(defInsn->GetOperand(kInsnSecondOpnd)));
        RegOperand *newIndexOpnd = GetReplaceReg(
            static_cast<RegOperand&>(defInsn->GetOperand(kInsnThirdOpnd)));
        auto &shift = static_cast<BitShiftOperand&>(defInsn->GetOperand(kInsnFourthOpnd));
        if (shift.GetShiftOp() != BitShiftOperand::kShiftLSL) {
          break;
        }
        if (newBaseOpnd != nullptr && newIndexOpnd != nullptr &&
            MemOperand::CheckNewAmount(currMemOpnd.GetSize(), shift.GetShiftAmount())) {
          newMemOpnd = static_cast<AArch64CGFunc*>(cgFunc)->CreateMemOperand(
              currMemOpnd.GetSize(), *newBaseOpnd, *newIndexOpnd, shift);
        }
      }
      break;
    }
    case MOP_xadrpl12: {
      if (memPropMode == kPropBase) {
        if (currMemOpnd.GetSize() >= 128) {
          // We can not be sure that the page offset is 16-byte aligned
          break;
        }
        auto *ofstOpnd = static_cast<ImmOperand*>(offset);
        CHECK_NULL_FATAL(ofstOpnd);
        int64 val = ofstOpnd->GetValue();
        auto *offset1 = static_cast<StImmOperand*>(&defInsn->GetOperand(kInsnThirdOpnd));
        CHECK_FATAL(offset1 != nullptr, "offset1 is null!");
        val += offset1->GetOffset();
        OfstOperand *newOfsetOpnd = &static_cast<AArch64CGFunc*>(cgFunc)->CreateOfstOpnd(static_cast<uint64>(val),
            k32BitSize);
        CHECK_FATAL(newOfsetOpnd != nullptr, "newOfsetOpnd is null!");
        const MIRSymbol *addr = offset1->GetSymbol();
        /* do not guarantee rodata alignment at Os */
        if (CGOptions::OptimizeForSize() && addr->IsReadOnly()) {
          break;
        }
        RegOperand *replace = GetReplaceReg(
            static_cast<RegOperand&>(defInsn->GetOperand(kInsnSecondOpnd)));
        if (replace != nullptr) {
          newMemOpnd = static_cast<AArch64CGFunc*>(cgFunc)->CreateMemOperand(
              currMemOpnd.GetSize(), *replace, *newOfsetOpnd, *addr);
        }
      }
      break;
    }
    /* do this in const prop ? */
    case MOP_wmovri32:
    case MOP_xmovri64: {
      if (memPropMode == kPropOffset) {
        auto *imm = static_cast<ImmOperand*>(&defInsn->GetOperand(kInsnSecondOpnd));
        OfstOperand *newOffset = &static_cast<AArch64CGFunc*>(cgFunc)->CreateOfstOpnd(
            static_cast<uint64>(imm->GetValue()), k32BitSize);
        CHECK_FATAL(newOffset != nullptr, "newOffset is null!");
        newOffset->SetVary(imm->GetVary());
        newMemOpnd = static_cast<AArch64CGFunc*>(cgFunc)->CreateMemOperand(currMemOpnd.GetSize(), *base, *newOffset);
      }
      break;
    }
    case MOP_xlslrri6:
    case MOP_wlslrri5: {
      auto *imm = static_cast<ImmOperand*>(&defInsn->GetOperand(kInsnThirdOpnd));
      RegOperand *newOfst = GetReplaceReg(static_cast<RegOperand&>(defInsn->GetOperand(kInsnSecondOpnd)));
      if (newOfst == nullptr || memPropMode == kPropBase) {
        break;
      }
      auto shift = static_cast<uint32>(static_cast<int32>(imm->GetValue()));
      shift += currMemOpnd.ShiftAmount();
      if (newOfst->GetSize() == base->GetSize()) {
        if ((memPropMode == kPropOffset || memPropMode == kPropShift) &&
            MemOperand::CheckNewAmount(currMemOpnd.GetSize(), shift)) {
          BitShiftOperand &shiftOperand =
              static_cast<AArch64CGFunc*>(cgFunc)->CreateBitShiftOperand(BitShiftOperand::kShiftLSL, shift, k8BitSize);
          newMemOpnd = static_cast<AArch64CGFunc*>(cgFunc)->CreateMemOperand(
              currMemOpnd.GetSize(), *base, *newOfst, shiftOperand);
        }
      } else if (newOfst->GetSize() == k32BitSize && memPropMode == kPropOffset) { /* lsl has implicit conversion */
        newMemOpnd = SelectReplaceExt(*base, shift, false, currMemOpnd.GetSize());
      }
      break;
    }
    case MOP_xsxtw64: {
      if (memPropMode == kPropBase || memPropMode == kPropUnsignedExtend) {
        break;
      }
      newMemOpnd = SelectReplaceExt(*base, static_cast<uint32>(currMemOpnd.ShiftAmount()),
                                    true, currMemOpnd.GetSize());
      break;
    }
    case MOP_xuxtw64: {
      if (memPropMode == kPropBase || memPropMode == kPropSignedExtend) {
        break;
      }
      newMemOpnd = SelectReplaceExt(*base, static_cast<uint32>(currMemOpnd.ShiftAmount()),
                                    false, currMemOpnd.GetSize());
      break;
    }
    default:
      break;
  }
  return newMemOpnd;
}

RegOperand *A64StrLdrProp::GetReplaceReg(RegOperand &a64Reg) {
  if (a64Reg.IsSSAForm()) {
    regno_t ssaIndex = a64Reg.GetRegisterNumber();
    replaceVersions[ssaIndex] = ssaInfo->FindSSAVersion(ssaIndex);
    ASSERT(replaceVersions.size() <= 2, "CHECK THIS CASE IN A64PROP");
    return &a64Reg;
  }
  return nullptr;
}

MemOperand *A64StrLdrProp::HandleArithImmDef(RegOperand &replace, Operand *oldOffset, int64 defVal,
                                             uint32 memSize, VaryType varyType) const {
  if (memPropMode != kPropBase) {
    return nullptr;
  }
  OfstOperand *newOfstImm = nullptr;
  if (oldOffset == nullptr) {
    newOfstImm = &static_cast<AArch64CGFunc*>(cgFunc)->CreateOfstOpnd(static_cast<uint64>(defVal), k32BitSize);
  } else {
    auto *ofstOpnd = static_cast<OfstOperand*>(oldOffset);
    CHECK_FATAL(ofstOpnd != nullptr, "oldOffsetOpnd is null");
    newOfstImm = &static_cast<AArch64CGFunc*>(cgFunc)->CreateOfstOpnd(
        static_cast<uint64>(defVal + ofstOpnd->GetValue()), k32BitSize);
  }
  CHECK_FATAL(newOfstImm != nullptr, "newOffset is null!");
  newOfstImm->SetVary(varyType);
  return static_cast<AArch64CGFunc*>(cgFunc)->CreateMemOperand(memSize, replace, *newOfstImm);
}

MemOperand *A64StrLdrProp::SelectReplaceExt(RegOperand &base, uint32 amount, bool isSigned, uint32 memSize) {
  MemOperand *newMemOpnd = nullptr;
  RegOperand *newOfst = GetReplaceReg(static_cast<RegOperand&>(defInsn->GetOperand(kInsnSecondOpnd)));
  if (newOfst == nullptr) {
    return nullptr;
  }
  if (!MemOperand::CheckNewAmount(memSize, amount)) {
    return nullptr;
  }
  /* defInsn is extend, currMemOpnd is same extend or shift */
  if (isSigned) {
    ExtendShiftOperand &extendOperand =
        static_cast<AArch64CGFunc*>(cgFunc)->CreateExtendShiftOperand(ExtendShiftOperand::kSXTW, amount, k32BitSize);
    newMemOpnd = static_cast<AArch64CGFunc*>(cgFunc)->CreateMemOperand(memSize, base, *newOfst, extendOperand);
  } else { /* isUnSigned */
    ExtendShiftOperand &extendOperand =
        static_cast<AArch64CGFunc*>(cgFunc)->CreateExtendShiftOperand(ExtendShiftOperand::kUXTW, amount, k32BitSize);
    newMemOpnd = static_cast<AArch64CGFunc*>(cgFunc)->CreateMemOperand(memSize, base, *newOfst, extendOperand);
  }
  return newMemOpnd;
}

bool A64StrLdrProp::CheckNewMemOffset(const Insn &insn, MemOperand &newMemOpnd, uint32 opndIdx) const {
  auto *a64CgFunc = static_cast<AArch64CGFunc*>(cgFunc);
  if ((newMemOpnd.GetOffsetImmediate() != nullptr) &&
      !a64CgFunc->IsOperandImmValid(insn.GetMachineOpcode(), &newMemOpnd, opndIdx)) {
    return false;
  }
  /* is ldp or stp, addrMode must be BOI */
  if ((opndIdx == kInsnThirdOpnd) && (newMemOpnd.GetAddrMode() != MemOperand::kBOI)) {
    return false;
  }
  return true;
}

void AArch64Prop::PropPatternOpt() {
  PropOptimizeManager::Optimize<ExtendMovPattern>(*cgFunc, GetSSAInfo());
  PropOptimizeManager::Optimize<ExtendShiftPattern>(*cgFunc, GetSSAInfo());
  PropOptimizeManager::Optimize<FpSpConstProp>(*cgFunc, GetSSAInfo());
  PropOptimizeManager::Optimize<A64PregCopyPattern>(*cgFunc, GetSSAInfo());
  PropOptimizeManager::Optimize<A64ConstFoldPattern>(*cgFunc, GetSSAInfo());
}

bool ExtendShiftPattern::IsSwapInsn(const Insn &insn) const {
  MOperator op = insn.GetMachineOpcode();
  switch (op) {
    case MOP_xaddrrr:
    case MOP_waddrrr:
    case MOP_xiorrrr:
    case MOP_wiorrrr:
    case MOP_wandrrr:
    case MOP_xandrrr:
      return true;
    default:
      return false;
  }
}

void ExtendShiftPattern::SetExMOpType(const Insn &use) {
  MOperator op = use.GetMachineOpcode();
  switch (op) {
    case MOP_xaddrrr:
    case MOP_xxwaddrrre:
    case MOP_xaddrrrs: {
      exMOpType = kExAdd;
      is64BitSize = true;
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
      is64BitSize = true;
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
      is64BitSize = true;
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
      is64BitSize = true;
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
      is64BitSize = true;
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
      is64BitSize = true;
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
      is64BitSize = true;
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
      is64BitSize = true;
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
      is64BitSize = true;
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
      is64BitSize = true;
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
      is64BitSize = true;
      break;
    }
    case MOP_wiorrrr:
    case MOP_wiorrrrs: {
      lsMOpType = kLwIor;
      break;
    }
    case MOP_xandrrr:
    case MOP_xandrrrs: {
      lsMOpType = kLxAnd;
      break;
    }
    case MOP_wandrrr:
    case MOP_wandrrrs: {
      lsMOpType = kLwAnd;
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
    case MOP_xlslrri6: shiftOp = BitShiftOperand::kShiftLSL;
      break;
    case MOP_xlsrrri6:
    case MOP_wlsrrri5:
    case MOP_wubfxrri5i5:
    case MOP_xubfxrri6i6: shiftOp = BitShiftOperand::kShiftLSR;
      break;
    case MOP_xasrrri6:
    case MOP_wasrrri5: shiftOp = BitShiftOperand::kShiftASR;
      break;
    case MOP_wextrrrri5:
    case MOP_xextrrrri6: shiftOp = BitShiftOperand::kShiftROR;
      break;
    default: {
      extendOp = ExtendShiftOperand::kUndef;
      shiftOp = BitShiftOperand::kUndef;
    }
  }
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


/* Check whether ExtendShiftPattern optimization can be performed. */
SuffixType ExtendShiftPattern::CheckOpType(const Operand &lastOpnd) const {
  /* Assign values to useType and defType. */
  uint32 useType = kNoSuffix;
  uint32 defType = shiftOp;
  if (extendOp != ExtendShiftOperand::kUndef) {
    defType = kExten;
  }
  if (lastOpnd.IsOpdShift()) {
    BitShiftOperand lastShiftOpnd = static_cast<const BitShiftOperand&>(lastOpnd);
    useType = lastShiftOpnd.GetShiftOp();
  } else if (lastOpnd.IsOpdExtend()) {
    ExtendShiftOperand lastExtendOpnd = static_cast<const ExtendShiftOperand&>(lastOpnd);
    useType = kExten;
    /* two insn is exten and exten ,value is exten(oneself) */
    if (useType == defType && extendOp != lastExtendOpnd.GetExtendOp()) {
      return kNoSuffix;
    }
  }
  return kDoOptimizeTable[useType][defType];
}

constexpr uint32 kExMopTypeSize = 9;
constexpr uint32 kLsMopTypeSize = 17;

MOperator exMopTable[kExMopTypeSize] = {
    MOP_undef, MOP_xxwaddrrre, MOP_wwwaddrrre, MOP_xxwsubrrre, MOP_wwwsubrrre,
    MOP_xwcmnrre, MOP_wwcmnrre, MOP_xwcmprre, MOP_wwcmprre
};
MOperator lsMopTable[kLsMopTypeSize] = {
    MOP_undef, MOP_xaddrrrs, MOP_waddrrrs, MOP_xsubrrrs, MOP_wsubrrrs,
    MOP_xcmnrrs, MOP_wcmnrrs, MOP_xcmprrs, MOP_wcmprrs, MOP_xeorrrrs,
    MOP_weorrrrs, MOP_xinegrrs, MOP_winegrrs, MOP_xiorrrrs, MOP_wiorrrrs,
    MOP_xandrrrs, MOP_wandrrrs
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
  SuffixType optType = CheckOpType(lastOpnd);
  Operand *shiftOpnd = nullptr;
  if (optType == kNoSuffix) {
    return;
  }else if (optType == kExten) {
    replaceOp = exMopTable[exMOpType];
    if (amount > k4BitSize) {
      return;
    }
    shiftOpnd = &a64CGFunc.CreateExtendShiftOperand(extendOp, amount, static_cast<int32>(k64BitSize));
  } else {
    replaceOp = lsMopTable[lsMOpType];
    if ((is64BitSize && amount >= k64BitSize) || (!is64BitSize && amount >= k32BitSize)) {
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
    replaceUseInsn = &cgFunc.GetInsnBuilder()->BuildInsn(replaceOp, firstOpnd, *secondOpnd, *shiftOpnd);
  } else {
    Operand &thirdOpnd = def.GetOperand(kInsnSecondOpnd);
    replaceUseInsn = &cgFunc.GetInsnBuilder()->BuildInsn(replaceOp, firstOpnd, *secondOpnd, thirdOpnd, *shiftOpnd);
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
    auto &immOpnd = (shiftOp == BitShiftOperand::kShiftROR ?
      static_cast<ImmOperand&>(defInsn->GetOperand(kInsnFourthOpnd)) :
      static_cast<ImmOperand&>(defInsn->GetOperand(kInsnThirdOpnd)));
    offset = static_cast<uint32>(immOpnd.GetValue());
  }
  amount += offset;

  ReplaceUseInsn(insn, *defInsn, amount);
}

void ExtendShiftPattern::DoExtendShiftOpt(Insn &insn) {
  if (!CheckAllOpndCondition(insn)) {
    return;
  }
  Optimize(*curInsn);
  if (optSuccess) {
    DoExtendShiftOpt(*newInsn);
  }
}

void ExtendShiftPattern::SwapOpnd(Insn &insn) {
  Insn *swapInsn = &cgFunc.GetInsnBuilder()->BuildInsn(insn.GetMachineOpcode(),
                                                       insn.GetOperand(kInsnFirstOpnd),
                                                       insn.GetOperand(kInsnThirdOpnd),
                                                       insn.GetOperand(kInsnSecondOpnd));
  insn.GetBB()->ReplaceInsn(insn, *swapInsn);
  optSsaInfo->ReplaceInsn(insn, *swapInsn);
  curInsn = swapInsn;
  replaceIdx = kInsnThirdOpnd;
}

bool ExtendShiftPattern::CheckAllOpndCondition(Insn &insn) {
  Init();
  SetLsMOpType(insn);
  SetExMOpType(insn);
  curInsn = &insn;
  if (IsSwapInsn(insn)) {
    if (CheckCondition(insn)) {
      return true;
    }
    Init();
    SetLsMOpType(insn);
    SetExMOpType(insn);
    replaceIdx = kInsnSecondOpnd;
    if (CheckCondition(insn)) {
      SwapOpnd(insn);
      return true;
    }
  } else {
    return CheckCondition(insn);
  }
  return false;
}

/* check and set:
 * exMOpType, lsMOpType, extendOp, shiftOp, defInsn
 */
bool ExtendShiftPattern::CheckCondition(Insn &insn) {
  if ((exMOpType == kExUndef) && (lsMOpType == kLsUndef)) {
    return false;
  }
  auto &regOperand = static_cast<RegOperand&>(insn.GetOperand(replaceIdx));
  regno_t regNo = regOperand.GetRegisterNumber();
  VRegVersion *useVersion = optSsaInfo->FindSSAVersion(regNo);
  defInsn = FindDefInsn(useVersion);
  // useVersion must not be nullptr when defInsn is not nullptr
  if (!defInsn || (useVersion->GetAllUseInsns().size() > 1)) {
    return false;
  }
  SelectExtendOrShift(*defInsn);
  /* defInsn must be shift or extend */
  if ((extendOp == ExtendShiftOperand::kUndef) && (shiftOp == BitShiftOperand::kUndef)) {
    return false;
  }
  Operand &defSrcOpnd = defInsn->GetOperand(kInsnSecondOpnd);
  CHECK_FATAL(defSrcOpnd.IsRegister(), "defSrcOpnd must be register!");
  if (shiftOp == BitShiftOperand::kShiftROR) {
    if (lsMOpType != kLxEor && lsMOpType != kLwEor && lsMOpType != kLxIor && lsMOpType != kLwIor) {
      return false;
    }
    Operand &defThirdOpnd = defInsn->GetOperand(kInsnThirdOpnd);
    CHECK_FATAL(defThirdOpnd.IsRegister(), "defThirdOpnd must be register");
    if (static_cast<RegOperand&>(defSrcOpnd).GetRegisterNumber() !=
        static_cast<RegOperand&>(defThirdOpnd).GetRegisterNumber()) {
      return false;
    }
  }
  auto &regDefSrc = static_cast<RegOperand&>(defSrcOpnd);
  if (regDefSrc.IsPhysicalRegister()) {
    return false;
  }
  /*
   * has Implict cvt
   *
   * avoid cases as following:
   *   lsr  x2, x2, #8
   *   ubfx w2, x2, #0, #32                lsr  x2, x2, #8
   *   eor  w0, w0, w2           ===>      eor  w0, w0, x2     ==\=>  eor w0, w0, w2, LSR #8
   *
   * the truncation causes the wrong value by shift right
   * shift left does not matter
   */
  if (useVersion->HasImplicitCvt() && shiftOp != BitShiftOperand::kUndef) {
    return false;
  }
  // Check pattern ubfx merged into bitshiftOperand LSR, for example:
  // ubfx  w0, w1, #m, #n
  // and   w2, w0, w3        ===>    and  w2, w3, w1, LSR #m
  // Condition: m + n = regsize
  // Check pattern ubfx merged into extendshiftOperand UXTH/UXTB, for example:
  // ubfx  w0, w1, #0, #16
  // add   w2, w0, w3        ===>    add  w2, w3, w1, UXTH
  MOperator defMop = defInsn->GetMachineOpcode();
  if (defMop == MOP_xubfxrri6i6 || defMop == MOP_wubfxrri5i5) {
    int64 mValue = static_cast<ImmOperand&>(defInsn->GetOperand(kInsnThirdOpnd)).GetValue();
    int64 nValue = static_cast<ImmOperand&>(defInsn->GetOperand(kInsnFourthOpnd)).GetValue();
    int64 size = is64BitSize ? static_cast<int64>(k64BitSize) : static_cast<int64>(k32BitSize);
    if (mValue == static_cast<int64>(k0BitSize) && nValue == static_cast<int64>(k16BitSize)) {
      shiftOp = BitShiftOperand::kUndef;
      extendOp = ExtendShiftOperand::kUXTH;
      return true;
    } else if (mValue == static_cast<int64>(k0BitSize) && nValue == static_cast<int64>(k8BitSize)) {
      shiftOp = BitShiftOperand::kUndef;
      extendOp = ExtendShiftOperand::kUXTB;
      return true;
    }
    if ((mValue + nValue) != size) {
      return false;
    }
  }
  if ((shiftOp == BitShiftOperand::kShiftLSR || shiftOp == BitShiftOperand::kShiftASR) &&
      (defSrcOpnd.GetSize() > regOperand.GetSize())) {
    return false;
  }
  // The following conversions are not equivalent:
  // sxtb_32 w0, w0
  // cmp_64 x2, x0
  // ==/=>
  // cmp_64 x2, w0, SXTB
  if ((extendOp == ExtendShiftOperand::kSXTB || extendOp == ExtendShiftOperand::kSXTH) &&
      (insn.GetOperandSize(replaceIdx) > defInsn->GetOperandSize(kInsnSecondOpnd))) {
    return false;
  }
  regno_t defSrcRegNo = regDefSrc.GetRegisterNumber();
  /* check regDefSrc */
  VRegVersion *replaceUseV = optSsaInfo->FindSSAVersion(defSrcRegNo);
  CHECK_FATAL(replaceUseV != nullptr, "useVRegVersion must not be null based on ssa");
  if (replaceUseV->GetAllUseInsns().size() > 1 && shiftOp != BitShiftOperand::kShiftROR) {
    return false;
  }
  return true;
}

void ExtendShiftPattern::Init() {
  replaceOp = MOP_undef;
  extendOp = ExtendShiftOperand::kUndef;
  shiftOp = BitShiftOperand::kUndef;
  defInsn = nullptr;
  newInsn = nullptr;
  replaceIdx = kInsnThirdOpnd;
  optSuccess = false;
  exMOpType = kExUndef;
  lsMOpType = kLsUndef;
  is64BitSize = false;
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
        ImmOperand &imm = static_cast<ImmOperand&>(defInsn->GetOperand(kInsnThirdOpnd));
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
  if (firstOpnd.IsPhysicalRegister() || secondOpnd.IsPhysicalRegister()) {
    return false;
  }
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
  insn.SetMOP(AArch64CG::kMd[replaceMop]);
}

void CopyRegProp::Run() {
  FOR_ALL_BB(bb, &cgFunc) {
    if (bb->IsAtomicBuiltInBB()) {
      continue;
    }
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

bool CopyRegProp::IsValidCopyProp(const RegOperand &dstReg, const RegOperand &srcReg) const {
  ASSERT(destVersion != nullptr, "find destVersion failed");
  ASSERT(srcVersion != nullptr, "find srcVersion failed");
  LiveInterval *dstll = nullptr;
  LiveInterval *srcll = nullptr;
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
    if (useInsn->IsPhi() && dstReg.GetSize() != srcReg.GetSize()) {
      return false;
    }
    dstll = regll->GetLiveInterval(dstRegNO);
    srcll = regll->GetLiveInterval(srcRegNO);
    ASSERT(dstll != nullptr, "dstll should not be nullptr");
    ASSERT(srcll != nullptr, "srcll should not be nullptr");
    static_cast<AArch64LiveIntervalAnalysis*>(regll)->CheckInterference(*dstll, *srcll);
    BB *useBB = useInsn->GetBB();
    if (dstll->IsConflictWith(srcRegNO) &&
        /* support override value when the version is not transphi */
        (((useBB->IsInPhiDef(srcRegNO) || useBB->IsInPhiList(srcRegNO)) && useBB->HasCriticalEdge()) ||
        useBB->IsInPhiList(dstRegNO))) {
      return false;
    }

    // destReg was used as use-def and has conflict with srcReg, can not prop it
    if (dstll->IsConflictWith(srcRegNO)) {
      for (auto [opndIdx, _] : useDUInfoIt.second->GetOperands()) {
        if (useInsn->GetDesc()->GetOpndDes(opndIdx)->IsRegDef()) {
          return false;
        }
      }
    }
  }
  if (dstll && srcll) {
    regll->CoalesceLiveIntervals(*dstll, *srcll);
  }
  return true;
}

bool CopyRegProp::CheckCondition(Insn &insn) {
  if (Globals::GetInstance()->GetTarget()->IsEffectiveCopy(insn)) {
    MOperator mOp = insn.GetMachineOpcode();
    if (mOp == MOP_xmovrr || mOp == MOP_wmovrr || mOp == MOP_xvmovs || mOp == MOP_xvmovd) {
      Operand &destOpnd = insn.GetOperand(kInsnFirstOpnd);
      Operand &srcOpnd = insn.GetOperand(kInsnSecondOpnd);
      ASSERT(destOpnd.IsRegister() && srcOpnd.IsRegister(), "must be");
      auto &destReg = static_cast<RegOperand &>(destOpnd);
      auto &srcReg = static_cast<RegOperand &>(srcOpnd);
      if (srcReg.GetRegisterNumber() == RZR) {
        insn.SetMOP(AArch64CG::kMd[mOp == MOP_xmovrr ? MOP_xmovri64 : MOP_wmovri32]);
        insn.SetOperand(kInsnSecondOpnd, cgFunc.CreateImmOperand(PTY_u64, 0));
      }
      if (destReg.IsSSAForm() && srcReg.IsSSAForm()) {
        if (destReg.GetSize() != srcReg.GetSize()) {
          ValidateImplicitCvt(destReg, srcReg, insn);
          return false;
        }
        if (destReg.GetValidBitsNum() >= srcReg.GetValidBitsNum()) {
          destReg.SetValidBitsNum(srcReg.GetValidBitsNum());
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

bool CopyRegProp::CanBePropagated(const Insn &insn) const {
  MOperator curMop = insn.GetMachineOpcode();
  if (curMop != MOP_wbfirri5i5 && curMop != MOP_xbfirri6i6) {
    return true;
  }
  auto &useOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  VRegVersion *version = optSsaInfo->FindSSAVersion(useOpnd.GetRegisterNumber());
  CHECK_FATAL(version != nullptr, "get SSAVersion failed");
  Insn *defInsn = FindDefInsn(version);
  if (defInsn != nullptr) {
    MOperator defMop = defInsn->GetMachineOpcode();
    // If constProp to bfi can be optimized, copyProp can be performed
    if (defMop == MOP_wmovri32 || defMop == MOP_xmovri64) {
      auto &widthOpnd = static_cast<ImmOperand&>(insn.GetOperand(kInsnFourthOpnd));
      auto width = static_cast<uint64>(widthOpnd.GetValue());
      auto &constOpnd = static_cast<ImmOperand&>(defInsn->GetOperand(kInsnSecondOpnd));
      auto val = static_cast<uint64>(constOpnd.GetValue());
      val = val & ((1U << width) - 1U);
      if (__builtin_popcountl(val) == static_cast<int64>(width)) {
        return true;
      }
    }
  }

  // If srcVersion use after both def&use insn, the mov cannot be propagated
  bool hasCrossDUUse = false;
  for (auto duInfoIt : srcVersion->GetAllUseInsns()) {
    CHECK_FATAL(duInfoIt.second != nullptr, "get DUInsnInfo failed");
    Insn *useInsn = duInfoIt.second->GetInsn();
    MOperator useMop = useInsn->GetMachineOpcode();
    if (useInsn->GetSSAImpDefOpnd() == nullptr && useMop != MOP_xmovrr && useMop != MOP_wmovrr &&
        useMop != MOP_xvmovs && useMop != MOP_xvmovd && useInsn->GetId() > insn.GetId()) {
      hasCrossDUUse = true;
      break;
    }
  }
  return !hasCrossDUUse;
}

void CopyRegProp::ReplaceAllUseForCopyProp() {
  MapleUnorderedMap<uint32, DUInsnInfo*> &useList = destVersion->GetAllUseInsns();
  Insn *srcRegDefInsn = FindDefInsn(srcVersion);
  for (auto it = useList.begin(); it != useList.end();) {
    Insn *useInsn = it->second->GetInsn();
    if (srcRegDefInsn != nullptr && srcRegDefInsn->GetSSAImpDefOpnd() != nullptr &&
        useInsn->GetSSAImpDefOpnd() != nullptr) {
      if (!CanBePropagated(*useInsn)) {
        ++it;
        continue;
      }
    }
    auto *a64SSAInfo = static_cast<AArch64CGSSAInfo*>(optSsaInfo);
    a64SSAInfo->CheckAsmDUbinding(*useInsn, *destVersion, *srcVersion);
    for (auto &opndIt : it->second->GetOperands()) {
      Operand &opnd = useInsn->GetOperand(opndIt.first);
      A64ReplaceRegOpndVisitor replaceRegOpndVisitor(cgFunc, *useInsn, opndIt.first,
                                                     *destVersion->GetSSAvRegOpnd(), *srcVersion->GetSSAvRegOpnd());
      opnd.Accept(replaceRegOpndVisitor);
      srcVersion->AddUseInsn(*optSsaInfo, *useInsn, opndIt.first);
      it->second->ClearDU(opndIt.first);
    }
    AArch64CG::UpdateMopOfPropedInsn(*useInsn);
    it = useList.erase(it);
  }
}

void CopyRegProp::Optimize(Insn &insn) {
  ReplaceAllUseForCopyProp();
  if (cgFunc.IsExtendReg(destVersion->GetSSAvRegOpnd()->GetRegisterNumber())) {
    cgFunc.InsertExtendSet(srcVersion->GetSSAvRegOpnd()->GetRegisterNumber());
  }
}

void CopyRegProp::ValidateImplicitCvt(RegOperand &destReg, const RegOperand &srcReg, Insn &movInsn) {
  ASSERT(movInsn.GetMachineOpcode() == MOP_xmovrr || movInsn.GetMachineOpcode() == MOP_wmovrr, "NIY explicit CVT");
  if (destReg.GetSize() == k64BitSize && srcReg.GetSize() == k32BitSize) {
    movInsn.SetMOP(AArch64CG::kMd[MOP_xuxtw64]);
  } else if (destReg.GetSize() == k32BitSize && srcReg.GetSize() == k64BitSize) {
    movInsn.SetMOP(AArch64CG::kMd[MOP_xubfxrri6i6]);
    movInsn.AddOperand(cgFunc.CreateImmOperand(PTY_i64, 0));
    movInsn.AddOperand(cgFunc.CreateImmOperand(PTY_i64, k32BitSize));
  } else {
    CHECK_FATAL(false, " unknown explicit integer cvt,  need implement in ssa prop ");
  }
  destReg.SetValidBitsNum(k32BitSize);
}

void RedundantPhiProp::Run() {
  FOR_ALL_BB(bb, &cgFunc) {
    for (auto &phiIt : as_const(bb->GetPhiInsns())) {
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
    uint32 srcRegNO = phiOpnd.GetOperands().cbegin()->second->GetRegisterNumber();
    srcVersion = optSsaInfo->FindSSAVersion(srcRegNO);
    ASSERT(srcVersion != nullptr, "find Version failed");
    return true;
  }
  return false;
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
  std::set<uint32> defRegs = insn.GetDefRegs();
  auto &a64CGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  if (defRegs.size() <= 1) {
    if (insn.ScanReg(RSP)) {
      fpSpBase = &a64CGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
      /* not safe due to varied sp in alloca */
      if (cgFunc.HasVLAOrAlloca()) {
        return false;
      }
    }
    if (insn.ScanReg(RFP)) {
      ASSERT(fpSpBase == nullptr, " unexpect for both sp fp using ");
      fpSpBase = &a64CGFunc.GetOrCreatePhysicalRegisterOperand(RFP, k64BitSize, kRegTyInt);
    }
    // vary reg prop
    if (insn.GetMachineOpcode() == MOP_xaddrri12 || insn.GetMachineOpcode() == MOP_xsubrri12) {
      auto &regOpnd1 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
      if (regOpnd1.IsOfVary()) {
        fpSpBase = static_cast<RegOperand*>(regOpnd1.Clone(*a64CGFunc.GetMemoryPool()));
      }
    }
    if (fpSpBase == nullptr) {
      return false;
    }
    if (insn.GetMachineOpcode() == MOP_xaddrri12) {
      aT = kAArch64Add;
      if (GetValidSSAInfo(insn.GetOperand(kInsnFirstOpnd))) {
        shiftOpnd = &static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
        return true;
      }
    } else if (insn.GetMachineOpcode() == MOP_xsubrri12) {
      aT = kAArch64Sub;
      auto &immOpnd = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
      // sub can not prop vary imm
      if (immOpnd.GetVary() == kUnAdjustVary) {
        return false;
      }
      if (GetValidSSAInfo(insn.GetOperand(kInsnFirstOpnd))) {
        shiftOpnd = &static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
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
  if (useInsn.IsAtomic()) {
    return;
  }
  if (useInsn.IsStore() || useInsn.IsLoad()) {
    if (useDUInfo.GetOperands().size() == 1) {
      auto useOpndIt = useDUInfo.GetOperands().begin();
      if (useOpndIt->first == kInsnSecondOpnd || useOpndIt->first == kInsnThirdOpnd) {
        ASSERT(useOpndIt->second == 1, "multiple use in memory opnd");
        auto *a64memOpnd = static_cast<MemOperand*>(useInsn.GetMemOpnd());
        CHECK_FATAL(a64memOpnd != nullptr, "a64memOpnd should not be nullptr");
        if (a64memOpnd->GetAddrMode() == MemOperand::kBOI) {
          auto *ofstOpnd = static_cast<OfstOperand*>(a64memOpnd->GetOffsetImmediate());
          CHECK_NULL_FATAL(ofstOpnd);
          int64 newVal = ArithmeticFold(ofstOpnd->GetValue(), kAArch64Add);
          auto *newOfstImm = &static_cast<AArch64CGFunc&>(cgFunc).CreateOfstOpnd(static_cast<uint64>(newVal),
              k64BitSize);
          if ((ofstOpnd != nullptr && ofstOpnd->GetVary() == kUnAdjustVary) || shiftOpnd->GetVary() == kUnAdjustVary) {
            newOfstImm->SetVary(kUnAdjustVary);
          }
          auto *newMem =
              static_cast<AArch64CGFunc&>(cgFunc).CreateMemOperand(a64memOpnd->GetSize(), *fpSpBase, *newOfstImm);
          if (static_cast<AArch64CGFunc&>(cgFunc).IsOperandImmValid(useMop, newMem, useOpndIt->first)) {
            useInsn.SetMemOpnd(newMem);
            useDUInfo.DecreaseDU(useOpndIt->first);
            replaced->CheckDeadUse(useInsn);
            auto *newRegVersion = optSsaInfo->FindSSAVersion(newMem->GetBaseRegister()->GetRegisterNumber());
            if (newRegVersion != nullptr) {
              newRegVersion->AddUseInsn(*optSsaInfo, useInsn, useOpndIt->first);
            }
          }
        }
      }
    } else {
      /*
       * case : store stack location on stack
       * add x1, sp, #8
       *  ...
       * store x1 [x1, #16]
       * not prop , not benefit to live range yet
       */
      return;
    }
  }
}

void FpSpConstProp::PropInArith(DUInsnInfo &useDUInfo, Insn &useInsn, ArithmeticType curAT) {
  if (useDUInfo.GetOperands().size() == 1) {
    auto &a64cgFunc = static_cast<AArch64CGFunc&>(cgFunc);
    MOperator useMop = useInsn.GetMachineOpcode();
    ASSERT(useDUInfo.GetOperands().begin()->first == kInsnSecondOpnd, "NIY");
    ASSERT(useDUInfo.GetOperands().begin()->second == 1, "multiple use in add/sub");
    auto &curVal = static_cast<ImmOperand&>(useInsn.GetOperand(kInsnThirdOpnd));
    ImmOperand &newVal = a64cgFunc.CreateImmOperand(ArithmeticFold(curVal.GetValue(), curAT),
                                                    curVal.GetSize(), false);
    if (newVal.GetValue() < 0) {
      newVal.Negate();
      useMop = A64ConstProp::GetReversalMOP(useMop);
    }
    if (curVal.GetVary() == kUnAdjustVary || shiftOpnd->GetVary() == kUnAdjustVary) {
      newVal.SetVary(kUnAdjustVary);
    }
    // can not prop unvary imm to  sub
    if (newVal.GetVary() == kUnAdjustVary && useInsn.GetMachineOpcode() == MOP_xsubrri12) {
      return;
    }
    if (static_cast<AArch64CGFunc&>(cgFunc).IsOperandImmValid(useMop, &newVal, kInsnThirdOpnd)) {
      Insn &newInsn =
          cgFunc.GetInsnBuilder()->BuildInsn(useMop, useInsn.GetOperand(kInsnFirstOpnd), *fpSpBase, newVal);
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
    auto &newVal = *static_cast<ImmOperand*>(shiftOpnd->Clone(*cgFunc.GetMemoryPool()));
    Insn &newInsn = cgFunc.GetInsnBuilder()->BuildInsn(oriMop, useInsn.GetOperand(kInsnFirstOpnd), *fpSpBase, newVal);
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

A64ConstFoldPattern::TypeAndSize A64ConstFoldPattern::SelectFoldTypeAndCheck64BitSize(const Insn &insn) const {
  MOperator mOp = insn.GetMachineOpcode();
  switch (mOp) {
    case MOP_waddrri12: return std::pair<FoldType, bool>{kAdd, false};
    case MOP_xaddrri12: return std::pair<FoldType, bool>{kAdd, true};
    case MOP_wsubrri12: return std::pair<FoldType, bool>{kSub, false};
    case MOP_xsubrri12: return std::pair<FoldType, bool>{kSub, true};
    case MOP_wlslrri5: return std::pair<FoldType, bool>{kLsl, false};
    case MOP_xlslrri6: return std::pair<FoldType, bool>{kLsl, true};
    case MOP_wlsrrri5: return std::pair<FoldType, bool>{kLsr, false};
    case MOP_xlsrrri6: return std::pair<FoldType, bool>{kLsr, true};
    case MOP_wasrrri5: return std::pair<FoldType, bool>{kAsr, false};
    case MOP_xasrrri6: return std::pair<FoldType, bool>{kAsr, true};
    case MOP_wandrri12: return std::pair<FoldType, bool>{kAnd, false};
    case MOP_xandrri13: return std::pair<FoldType, bool>{kAnd, true};
    case MOP_wiorrri12: return std::pair<FoldType, bool>{kOrr, false};
    case MOP_xiorrri13: return std::pair<FoldType, bool>{kOrr, true};
    case MOP_weorrri12: return std::pair<FoldType, bool>{kEor, false};
    case MOP_xeorrri13: return std::pair<FoldType, bool>{kEor, true};
    default:
      return std::pair<FoldType, bool>{kFoldUndef, false};
  }
}

bool A64ConstFoldPattern::IsDefInsnValid(const Insn &curInsn, const Insn &validDefInsn) {
  std::pair<FoldType, bool> defInfo = SelectFoldTypeAndCheck64BitSize(validDefInsn);
  defFoldType = defInfo.first;
  if (defFoldType == kFoldUndef) {
    return false;
  }
  /* do not optimize MOP_x and MOP_w */
  if (is64Bit != defInfo.second) {
    return false;
  }
  ASSERT(curInsn.GetOperand(kInsnFirstOpnd).IsRegister() &&
         validDefInsn.GetOperand(kInsnSecondOpnd).IsRegister(), "must be");
  dstOpnd = &static_cast<RegOperand&>(curInsn.GetOperand(kInsnFirstOpnd));
  srcOpnd = &static_cast<RegOperand&>(validDefInsn.GetOperand(kInsnSecondOpnd));
  if (dstOpnd->IsPhysicalRegister() || srcOpnd->IsPhysicalRegister()) {
    return false;
  }
  optType = constFoldTable[useFoldType][defFoldType];
  if (optType == kOptUndef) {
    return false;
  }
  ASSERT(defInsn->GetOperand(kInsnFirstOpnd).IsRegister(), "must be");
  defDstOpnd = &static_cast<RegOperand&>(defInsn->GetOperand(kInsnFirstOpnd));
  return true;
}

bool A64ConstFoldPattern::IsPhiInsnValid(const Insn &curInsn, const Insn &phiInsn) {
  std::vector<Insn*> validDefInsns;
  auto &phiOpnd = static_cast<PhiOperand&>(phiInsn.GetOperand(kInsnSecondOpnd));
  for (const auto useIt : phiOpnd.GetOperands()) {
    ASSERT(useIt.second != nullptr, "get phiUseOpnd failed");
    Insn *defPhiInsn = optSsaInfo->GetDefInsn(*useIt.second);
    /* check only one layer of phi */
    if (defPhiInsn == nullptr || defPhiInsn->IsPhi()) {
      return false;
    }
    (void)validDefInsns.emplace_back(defPhiInsn);
  }
  if (validDefInsns.empty()) {
    return false;
  }
  if (!IsDefInsnValid(curInsn, *validDefInsns[0])) {
    return false;
  }
  MOperator mOp = validDefInsns[0]->GetMachineOpcode();
  CHECK_FATAL(validDefInsns[0]->GetOperand(kInsnSecondOpnd).IsRegister(), "check this insn");
  CHECK_FATAL(validDefInsns[0]->GetOperand(kInsnThirdOpnd).IsImmediate(), "check this insn");
  auto &validSrcOpnd = static_cast<RegOperand&>(validDefInsns[0]->GetOperand(kInsnSecondOpnd));
  auto &validImmOpnd = static_cast<ImmOperand&>(validDefInsns[0]->GetOperand(kInsnThirdOpnd));
  uint32 opndNum = validDefInsns[0]->GetOperandSize();
  for (uint32 insnIdx = 1; insnIdx < validDefInsns.size(); ++insnIdx) {
    Insn *insn = validDefInsns[insnIdx];
    if (insn->GetMachineOpcode() != mOp) {
      return false;
    }
    if (insn->GetOperandSize() != opndNum) {
      return false;
    }
    if (!insn->GetOperand(kInsnSecondOpnd).IsRegister() || !insn->GetOperand(kInsnThirdOpnd).IsImmediate()) {
      return false;
    }
    if (!static_cast<RegOperand&>(insn->GetOperand(kInsnSecondOpnd)).Equals(validSrcOpnd) ||
        !static_cast<ImmOperand&>(insn->GetOperand(kInsnThirdOpnd)).Equals(validImmOpnd)) {
      return false;
    }
  }
  defInsn = validDefInsns[0];
  return true;
}

bool A64ConstFoldPattern::IsCompleteOptimization() const {
  VRegVersion *defDstVersion = optSsaInfo->FindSSAVersion(defDstOpnd->GetRegisterNumber());
  ASSERT(defDstVersion != nullptr, "get defDstVersion failed");
  /* check all uses of dstOpnd of defInsn to avoid spill */
  for (auto useInfoIt : defDstVersion->GetAllUseInsns()) {
    ASSERT(useInfoIt.second != nullptr, "get duInsnInfo failed");
    Insn *useInsn = useInfoIt.second->GetInsn();
    ASSERT(useInsn != nullptr, "get useInsn failed");
    if (useInsn->IsPhi()) {
      continue;
    }
    std::pair<FoldType, bool> useInfo = SelectFoldTypeAndCheck64BitSize(*useInsn);
    if (useInfo.first == kFoldUndef) {
      return false;
    }
  }
  return true;
}

bool A64ConstFoldPattern::CheckCondition(Insn &insn) {
  std::pair<FoldType, bool> useInfo = SelectFoldTypeAndCheck64BitSize(insn);
  useFoldType = useInfo.first;
  if (useFoldType == kFoldUndef) {
    return false;
  }
  is64Bit = useInfo.second;
  ASSERT(insn.GetOperand(kInsnSecondOpnd).IsRegister(), "check this insn");
  auto &useOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  defInsn = optSsaInfo->GetDefInsn(useOpnd);
  if (defInsn == nullptr) {
    return false;
  }
  if (defInsn->IsPhi()) {
    return IsPhiInsnValid(insn, *defInsn);
  } else {
    return IsDefInsnValid(insn, *defInsn);
  }
}

MOperator A64ConstFoldPattern::GetNewMop(bool isNegativeVal, MOperator curMop) const {
  MOperator newMop = MOP_undef;
  switch (useFoldType) {
    case kAdd:
      newMop = (isNegativeVal ? (is64Bit ? MOP_xsubrri12 : MOP_wsubrri12) : curMop);
      break;
    case kSub:
      newMop = (isNegativeVal ? curMop : (is64Bit ? MOP_xaddrri12 : MOP_waddrri12));
      break;
    case kLsl:
      newMop = (isNegativeVal ? (is64Bit ? MOP_xlsrrri6 : MOP_wlsrrri5) : curMop);
      break;
    case kLsr:
      newMop = (isNegativeVal ? (is64Bit ? MOP_xlslrri6 : MOP_wlslrri5) : curMop);
      break;
    case kAsr:
      newMop = (isNegativeVal ? MOP_undef : curMop);
      break;
    case kAnd:
    case kOrr:
    case kEor:
      return curMop;
    default:
      return MOP_undef;
  }
  return newMop;
}

ImmOperand &A64ConstFoldPattern::GetNewImmOpnd(const ImmOperand &immOpnd, int64 newImmVal) const {
  auto &a64Func = static_cast<AArch64CGFunc&>(cgFunc);
  switch (useFoldType) {
    case kAdd:
    case kSub:
    case kLsl:
    case kLsr:
    case kAsr:
      return (newImmVal < 0 ?
              a64Func.CreateImmOperand(-newImmVal, immOpnd.GetSize(), immOpnd.IsSignedValue()) :
              a64Func.CreateImmOperand(newImmVal, immOpnd.GetSize(), immOpnd.IsSignedValue()));
    case kAnd:
    case kOrr:
    case kEor:
      return a64Func.CreateImmOperand(newImmVal, immOpnd.GetSize(), immOpnd.IsSignedValue());
    default:
      CHECK_FATAL(false, "can not run here");
  }
}

void A64ConstFoldPattern::ReplaceWithNewInsn(Insn &insn, const ImmOperand &immOpnd, int64 newImmVal) {
  ASSERT(insn.GetOperand(kInsnThirdOpnd).IsImmediate(), "check this insn");
  auto &useImmOpnd = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
  auto &a64Func = static_cast<AArch64CGFunc&>(cgFunc);
  MOperator curMop = insn.GetMachineOpcode();
  MOperator newMop = GetNewMop(newImmVal < 0, curMop);
  VaryType resVaryType = kNotVary;
  if (immOpnd.GetVary() == kUnAdjustVary && useImmOpnd.GetVary() == kUnAdjustVary) {
    return;
  }
  if (immOpnd.GetVary() == kUnAdjustVary || useImmOpnd.GetVary() == kUnAdjustVary) {
    resVaryType = kUnAdjustVary;
  }
  if (resVaryType == kUnAdjustVary && optType != kPositive) {
    return;
  }
  ImmOperand &newImmOpnd = GetNewImmOpnd(immOpnd, newImmVal);
  newImmOpnd.SetVary(resVaryType);
  if (!a64Func.IsOperandImmValid(newMop, &newImmOpnd, kInsnThirdOpnd)) {
    return;
  }
  if (useFoldType == kLsl || useFoldType == kLsr || useFoldType == kAsr) {
    if (newImmVal < 0 || (is64Bit && newImmVal >= k64BitSize) || (!is64Bit && newImmVal >= k32BitSize)) {
      return;
    }
  }
  Insn &newInsn = cgFunc.GetInsnBuilder()->BuildInsn(newMop, *dstOpnd, *srcOpnd, newImmOpnd);
  insn.GetBB()->ReplaceInsn(insn, newInsn);
  /* update ssa info */
  optSsaInfo->ReplaceInsn(insn, newInsn);
  if (PROP_DUMP) {
    LogInfo::MapleLogger() << ">>>>>>> In A64ConstFoldPattern : <<<<<<<\n";
    LogInfo::MapleLogger() << "=======ReplaceInsn :\n";
    insn.Dump();
    LogInfo::MapleLogger() << "=======NewInsn :\n";
    newInsn.Dump();
  }
}

int64 A64ConstFoldPattern::GetNewImmVal(const Insn &insn, const ImmOperand &defImmOpnd) const {
  ASSERT(insn.GetOperand(kInsnThirdOpnd).IsImmediate(), "check this insn");
  auto &useImmOpnd = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
  int64 newImmVal = 0;
  switch (optType) {
    case kPositive:
      newImmVal = useImmOpnd.GetValue() + defImmOpnd.GetValue();
      break;
    case kNegativeDef:
      newImmVal = useImmOpnd.GetValue() - defImmOpnd.GetValue();
      break;
    case kNegativeUse:
      newImmVal = defImmOpnd.GetValue() - useImmOpnd.GetValue();
      break;
    case kNegativeBoth:
      newImmVal = -defImmOpnd.GetValue() - useImmOpnd.GetValue();
      break;
    case kLogicalAnd:
      newImmVal = static_cast<int64>(
          static_cast<uint64>(defImmOpnd.GetValue()) & static_cast<uint64>(useImmOpnd.GetValue()));
      break;
    case kLogicalOrr:
      newImmVal = static_cast<int64>(
          static_cast<uint64>(defImmOpnd.GetValue()) | static_cast<uint64>(useImmOpnd.GetValue()));
      break;
    case kLogicalEor:
      newImmVal = static_cast<int64>(
          static_cast<uint64>(defImmOpnd.GetValue()) ^ static_cast<uint64>(useImmOpnd.GetValue()));
      break;
    default:
      CHECK_FATAL(false, "can not be here");
  }
  return newImmVal;
}

void A64ConstFoldPattern::Optimize(Insn &insn) {
  ASSERT(defInsn->GetOperand(kInsnThirdOpnd).IsImmediate(), "check this insn");
  auto &defImmOpnd = static_cast<ImmOperand&>(defInsn->GetOperand(kInsnThirdOpnd));
  ASSERT(insn.GetOperand(kInsnThirdOpnd).IsImmediate(), "check this insn");
  auto &useImmOpnd = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
  int64 newImmVal = GetNewImmVal(insn, defImmOpnd);
  if (newImmVal == 0 && optType != kLogicalAnd && optType != kLogicalOrr && optType != kLogicalEor) {
    // when one of the imm is unvary, should not do prop , 0 (unvary) will be adjust in frame finalize.
    if (useImmOpnd.GetVary() == kUnAdjustVary || defImmOpnd.GetVary() == kUnAdjustVary) {
      return;
    }
    VRegVersion *dstVersion = optSsaInfo->FindSSAVersion(dstOpnd->GetRegisterNumber());
    VRegVersion *srcVersion = optSsaInfo->FindSSAVersion(srcOpnd->GetRegisterNumber());
    CHECK_FATAL(dstVersion != nullptr, "get dstVersion failed");
    CHECK_FATAL(srcVersion != nullptr, "get srcVersion failed");
    if (cgFunc.IsExtendReg(dstOpnd->GetRegisterNumber())) {
      cgFunc.InsertExtendSet(srcOpnd->GetRegisterNumber());
    }
    optSsaInfo->ReplaceAllUse(dstVersion, srcVersion);
  } else if (IsCompleteOptimization()) {
    ReplaceWithNewInsn(insn, defImmOpnd, newImmVal);
  }
}

void A64ConstFoldPattern::Run() {
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

bool A64PregCopyPattern::DFSFindValidDefInsns(Insn *curDefInsn, std::vector<regno_t> &visitedPhiDefs,
                                              std::unordered_map<uint32, bool> &visited) {
  if (curDefInsn == nullptr) {
    return false;
  }
  /*
   * avoid the case as following:
   * R113 and R117 define each other.
   *                               [BB5]     ----------------------------
   *                    phi: R113, (R111<4>, R117<9>)                   |
   *                              /     \                               |
   *                             /       \                              |
   *                        [BB6]  ----  [BB7]                          |
   *            add R116, R113, #4     phi: R117, (R113<5>, R116<6>)    |
   *                                     /   \                          |
   *                                    /     \                         |
   *                                 [BB8]    [BB28]                    |
   *                                   /                                |
   *                                  /                                 |
   *                                 [BB9] ------ [BB5]                 |
   *                             mov R1, R117  --------------------------
   *
   * but the cases as following is right:
   * (1)
   *                              [BB124]
   *                       add R339, R336, #345      --------  is found twice
   *                              /    \
   *                             /      \
   *                            /     [BB125]
   *                            \        /
   *                             \      /
   *                              [BB56]
   *                       phi: R370, (R339<124>, R339<125>)
   *                                |
   *                                |
   *                             [BB61]
   *                           mov R0, R370
   * (2)
   *                                [BB17]
   *                       phi: R242, (R241<14>, R218<53>)    ------- is found twice
   *                              /          \
   *                             /            \
   *                            /           [BB26]     [BB32]
   *                            \               \      /
   *                             \               [BB27]
   *                              \      phi: R273, (R242<26>, R320<32>)
   *                           [BB25]           /
   *                                \        [BB42]
   *                                 \       /
   *                                  [BB43]
   *                        phi: R321, (R242<25>, R273<42>)
   *                                    |
   *                                  [BB47]
   *                               mov R0, R321
   */
  if (visited[curDefInsn->GetId()] && curDefInsn->IsPhi() && !visitedPhiDefs.empty()) {
    auto &curPhiOpnd = static_cast<const PhiOperand&>(curDefInsn->GetOperand(kInsnSecondOpnd));
    for (auto &curPhiListIt : curPhiOpnd.GetOperands()) {
      auto &curUseOpnd = static_cast<RegOperand&>(*curPhiListIt.second);
      if (std::find(visitedPhiDefs.begin(), visitedPhiDefs.end(), curUseOpnd.GetRegisterNumber()) !=
          visitedPhiDefs.end()) {
        return false;
      }
    }
  }
  if (visited[curDefInsn->GetId()]) {
    return true;
  }
  visited[curDefInsn->GetId()] = true;
  if (!curDefInsn->IsPhi()) {
    CHECK_FATAL(curDefInsn->IsMachineInstruction(), "expect valid insn");
    (void)validDefInsns.emplace_back(curDefInsn);
    return true;
  }
  auto &phiOpnd = static_cast<const PhiOperand&>(curDefInsn->GetOperand(kInsnSecondOpnd));
  for (auto &phiListIt : phiOpnd.GetOperands()) {
    auto &useOpnd = static_cast<RegOperand&>(*phiListIt.second);
    VRegVersion *useVersion = optSsaInfo->FindSSAVersion(useOpnd.GetRegisterNumber());
    Insn *defInsn = FindDefInsn(useVersion);
    if (defInsn == nullptr) {
      return false;
    }
    if (defInsn->IsPhi()) {
      auto &curPhiDef = static_cast<RegOperand&>(curDefInsn->GetOperand(kInsnFirstOpnd));
      (void)visitedPhiDefs.emplace_back(curPhiDef.GetRegisterNumber());
    }
    if (!DFSFindValidDefInsns(defInsn, visitedPhiDefs, visited)) {
      return false;
    }
  }
  return true;
}

bool A64PregCopyPattern::CheckMultiUsePoints(const Insn &defInsn) const {
  Operand &dstOpnd = defInsn.GetOperand(kInsnFirstOpnd);
  CHECK_FATAL(dstOpnd.IsRegister(), "dstOpnd must be register");
  VRegVersion *defVersion = optSsaInfo->FindSSAVersion(static_cast<RegOperand&>(dstOpnd).GetRegisterNumber());
  ASSERT(defVersion != nullptr, "defVersion should not be nullptr");
  /* use: (phi) or (mov preg) */
  for (auto &useInfoIt : defVersion->GetAllUseInsns()) {
    DUInsnInfo *useInfo = useInfoIt.second;
    CHECK_FATAL(useInfo, "get useDUInfo failed");
    Insn *useInsn = useInfo->GetInsn();
    CHECK_FATAL(useInsn, "get useInsn failed");
    if (!useInsn->IsPhi() && useInsn->GetMachineOpcode() != MOP_wmovrr && useInsn->GetMachineOpcode() != MOP_xmovrr) {
      return false;
    }
    if ((useInsn->GetMachineOpcode() == MOP_wmovrr || useInsn->GetMachineOpcode() == MOP_xmovrr) &&
         !static_cast<RegOperand&>(useInsn->GetOperand(kInsnFirstOpnd)).IsPhysicalRegister()) {
      return false;
    }
  }
  return true;
}

bool A64PregCopyPattern::HasValidDefInsnDependency() {
  /* Look for reg dependency between validDefInsns */
  for (auto *ti : validDefInsns) {
    const auto *md = &AArch64CG::kMd[ti->GetMachineOpcode()];
    for (uint32 i = 0; i < ti->GetOperandSize(); i++) {
      if (ti->GetOperand(i).IsRegister() && md->GetOpndDes(i)->IsRegUse() &&
          static_cast<RegOperand&>(ti->GetOperand(i)).IsVirtualRegister()) {
        regno_t regno = static_cast<RegOperand&>(ti->GetOperand(i)).GetRegisterNumber();
        VRegVersion *useVersion = optSsaInfo->FindSSAVersion(regno);
        Insn *def = FindDefInsn(useVersion);
        if (def) {
          auto it = find(validDefInsns.begin(), validDefInsns.end(), def);
          if (it != validDefInsns.end()) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

bool A64PregCopyPattern::CheckPhiCaseCondition(Insn &defInsn) {
  std::unordered_map<uint32, bool> visited;
  std::vector<regno_t> visitedPhiDefs;
  visitedPhiDefs.emplace_back(static_cast<RegOperand&>(defInsn.GetOperand(kInsnFirstOpnd)).GetRegisterNumber());
  if (!DFSFindValidDefInsns(&defInsn, visitedPhiDefs, visited)) {
    return false;
  }
  if (!CheckValidDefInsn(*(validDefInsns[0]))) {
    return false;
  }
  MOperator defMop = validDefInsns[0]->GetMachineOpcode();
  uint32 defOpndNum = validDefInsns[0]->GetOperandSize();
  for (size_t i = 1; i < validDefInsns.size(); ++i) {
    if (defMop != validDefInsns[i]->GetMachineOpcode()) {
      return false;
    }
    if (!CheckMultiUsePoints(*validDefInsns[i])) {
      return false;
    }
    for (uint32 idx = 0; idx < defOpndNum; ++idx) {
      if (validDefInsns[0]->OpndIsDef(idx) && validDefInsns[i]->OpndIsDef(idx)) {
        continue;
      }
      Operand &opnd1 = validDefInsns[0]->GetOperand(idx);
      Operand &opnd2 = validDefInsns[i]->GetOperand(idx);
      if (!opnd1.Equals(opnd2)) {
        if (differIdx == -1) {
          differIdx = static_cast<int>(idx);
        } else if (static_cast<int32>(idx) != differIdx) {
          // Only one operand of two instructions can be different
          return false;
        }
        // We only propagate register
        if (!validDefInsns[0]->GetOperand(static_cast<uint32>(differIdx)).IsRegister() ||
            !validDefInsns[i]->GetOperand(static_cast<uint32>(differIdx)).IsRegister()) {
          return false;
        }
        auto &differOpnd1 = static_cast<RegOperand&>(validDefInsns[0]->GetOperand(static_cast<uint32>(differIdx)));
        auto &differOpnd2 = static_cast<RegOperand&>(validDefInsns[i]->GetOperand(static_cast<uint32>(differIdx)));
        /* avoid cc reg */
        if (!differOpnd1.IsOfIntClass() || !differOpnd2.IsOfIntClass() ||
            differOpnd1.IsPhysicalRegister() || differOpnd2.IsPhysicalRegister()) {
          return false;
        }
        VRegVersion *differVersion1 = optSsaInfo->FindSSAVersion(differOpnd1.GetRegisterNumber());
        VRegVersion *differVersion2 = optSsaInfo->FindSSAVersion(differOpnd2.GetRegisterNumber());
        if (!differVersion1 || !differVersion2) {
          return false;
        }
        // The PregPropagationOpt cannot be done if the original versions of the two operands from different
        // validDefInsns are not the same.
        if (differVersion1->GetOriginalRegNO() != differVersion2->GetOriginalRegNO()) {
          return false;
        }
        differOrigNO = differVersion1->GetOriginalRegNO();
      }
    }
    if (differIdx <= 0) {
      return false;
    }
    auto defRegNO = static_cast<RegOperand&>(defInsn.GetOperand(kInsnFirstOpnd)).GetRegisterNumber();
    if (differOrigNO == optSsaInfo->FindSSAVersion(defRegNO)->GetOriginalRegNO()) {
      return false;
    }
  }
  if (differIdx == -1) {
    return false;
  }
  if (HasValidDefInsnDependency()) {
    return false;
  }
  return true;
}

bool A64PregCopyPattern::CheckUselessDefInsn(const Insn &defInsn) const {
  Operand &dstOpnd = defInsn.GetOperand(kInsnFirstOpnd);
  CHECK_FATAL(dstOpnd.IsRegister(), "dstOpnd must be register");
  VRegVersion *defVersion = optSsaInfo->FindSSAVersion(static_cast<RegOperand&>(dstOpnd).GetRegisterNumber());
  ASSERT(defVersion != nullptr, "defVersion should not be nullptr");
  if (defVersion->GetAllUseInsns().size() == 1) {
    return true;
  }
  /*
   * avoid the case as following
   * In a loop:
   *                      [BB43]
   *            phi: R356, (R345<42>, R377<63>)
   *                     /           \
   *                    /             \
   *                 [BB44]            \
   *             add R377, R356, #1    /
   *             mov R1, R377         /
   *                  bl             /
   *                     \          /
   *                      \        /
   *                        [BB63]
   */
  for (auto &useInfoIt : defVersion->GetAllUseInsns()) {
    DUInsnInfo *useInfo = useInfoIt.second;
    CHECK_FATAL(useInfo, "get useDUInfo failed");
    Insn *useInsn = useInfo->GetInsn();
    CHECK_FATAL(useInsn, "get useInsn failed");
    if (useInsn->IsPhi()) {
      auto &phiDefOpnd = static_cast<RegOperand&>(useInsn->GetOperand(kInsnFirstOpnd));
      uint32 opndNum = defInsn.GetOperandSize();
      for (uint32 i = 0; i < opndNum; ++i) {
        if (defInsn.OpndIsDef(i)) {
          continue;
        }
        Operand &opnd = defInsn.GetOperand(i);
        if (opnd.IsRegister() && static_cast<RegOperand&>(opnd).GetRegisterNumber() == phiDefOpnd.GetRegisterNumber()) {
          return false;
        }
      }
    }
  }
  return true;
}

bool A64PregCopyPattern::CheckValidDefInsn(const Insn &defInsn) {
  const auto *md = defInsn.GetDesc();
  CHECK_FATAL(md != nullptr, "expect valid AArch64MD");
  /* this pattern applies to all basicOps */
  if (md->IsMove() || md->IsStore() || md->IsLoad() || md->IsLoadStorePair() || md->IsLoadAddress() || md->IsCall() ||
      md->IsDMB() || md->IsVectorOp() || md->IsCondDef() || md->IsCondBranch() || md->IsUnCondBranch()) {
    return false;
  }
  uint32 opndNum = defInsn.GetOperandSize();
  for (uint32 i = 0; i < opndNum; ++i) {
    Operand &opnd = defInsn.GetOperand(i);
    if (!opnd.IsRegister() && !opnd.IsImmediate() && !opnd.IsOpdShift() && !opnd.IsOpdExtend()) {
      return false;
    }
    if (opnd.IsRegister()) {
      auto &regOpnd = static_cast<RegOperand&>(opnd);
      if (cgFunc.IsSPOrFP(regOpnd) || regOpnd.IsPhysicalRegister() ||
          (!regOpnd.IsOfIntClass() && !regOpnd.IsOfFloatOrSIMDClass())) {
        return false;
      }
    }
  }
  return true;
}

bool A64PregCopyPattern::CheckCondition(Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  if (curMop != MOP_xmovrr && curMop != MOP_wmovrr) {
    return false;
  }
  auto &dstOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  if (!dstOpnd.IsPhysicalRegister()) {
    return false;
  }
  regno_t useRegNO = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd)).GetRegisterNumber();
  VRegVersion *useVersion = optSsaInfo->FindSSAVersion(useRegNO);
  Insn *defInsn = FindDefInsn(useVersion);
  if (defInsn == nullptr) {
    return false;
  }
  Operand &defDstOpnd = defInsn->GetOperand(kInsnFirstOpnd);
  /* avoid inline-asm */
  if (!defDstOpnd.IsRegister()) {
    return false;
  }
  if (!CheckMultiUsePoints(*defInsn)) {
    return false;
  }
  if (defInsn->IsPhi()) {
    isCrossPhi = true;
    firstPhiInsn = defInsn;
    return CheckPhiCaseCondition(*defInsn);
  } else {
    if (!CheckValidDefInsn(*defInsn)) {
      return false;
    }
    if (!CheckUselessDefInsn(*defInsn)) {
      return false;
    }
    (void)validDefInsns.emplace_back(defInsn);
  }
  return true;
}

Insn &A64PregCopyPattern::CreateNewPhiInsn(std::unordered_map<uint32, RegOperand*> &newPhiList, Insn *curInsn) {
  CHECK_FATAL(!newPhiList.empty(), "empty newPhiList");
  RegOperand *differOrigOpnd = cgFunc.GetVirtualRegisterOperand(differOrigNO);
  CHECK_FATAL(differOrigOpnd != nullptr, "get original opnd default");
  PhiOperand &phiList = optSsaInfo->CreatePhiOperand();
  for (auto &it : newPhiList) {
    phiList.InsertOpnd(it.first, *it.second);
  }
  Insn &phiInsn = cgFunc.GetCG()->BuildPhiInsn(*differOrigOpnd, phiList);
  optSsaInfo->CreateNewInsnSSAInfo(phiInsn);
  BB *bb = curInsn->GetBB();
  (void)bb->InsertInsnBefore(*curInsn, phiInsn);
  // phiDef-ssaRegNO, phiInsn
  bb->AddPhiInsn(static_cast<RegOperand&>(phiInsn.GetOperand(kInsnFirstOpnd)).GetRegisterNumber(), phiInsn);
  return phiInsn;
}

/*
 * Check whether the required phi is available, do not insert phi repeatedly.
 */
RegOperand *A64PregCopyPattern::CheckAndGetExistPhiDef(Insn &phiInsn, std::vector<regno_t> &validDifferRegNOs) const {
  /*
   * For the case as following, there are two different ssaVersion for a same original Vreg:
   * [in original form]
   *       lsl R227, R227, #3        lsl R227, R227, #3
   *                       \         /
   *                        \       /
   *                       mov R2, R227
   *
   * [in ssa form]
   *       lsl R173, R163, #3      lsl R172, R164, #3
   *                       \        /
   *                        \      /
   *                  phi R175, (R173, R172)    ----->   need to create new phi(R165) for (R163, R164)
   *                  mov R2, R175                       lsl R2, R165, #3
   */
  if (IsDifferentSSAVersion()) {
    return nullptr;
  }

  std::set<regno_t> validDifferOrigRegNOs;
  for (regno_t ssaRegNO : validDifferRegNOs) {
    VRegVersion *version = optSsaInfo->FindSSAVersion(ssaRegNO);
    (void)validDifferOrigRegNOs.insert(version->GetOriginalRegNO());
  }
  auto &phiInsns = phiInsn.GetBB()->GetPhiInsns();
  for (auto &phiIt : as_const(phiInsns)) {
    auto &def = static_cast<RegOperand&>(phiIt.second->GetOperand(kInsnFirstOpnd));
    VRegVersion *defVersion = optSsaInfo->FindSSAVersion(def.GetRegisterNumber());
    ASSERT(defVersion != nullptr, "defVersion should not be nullptr");
    /*
     * if the phi of the change point has been created (according to original regNO), return the phiDefOpnd.
     * But, there is a problem: the phiDefOpnd of the same original regNO is not the required phi.
     * For example: (in parentheses is the original regNO)
     *           add R110(R80), R106(R80), #1              add R122(R80), R118(R80), #1
     *                                       \            /
     *                                        \          /
     *                              (1) phi: R123(R80), [R110, R122]
     *                                  mov R0, R123
     *           It will return R123 of phi(1) because the differOrigNO is 80, but that's not what we want,
     *           we need to create a new phi(2): R140(R80), [R106, R118].
     *           so we need to check whether all phiOpnds have correct ssaRegNO.
     */
    if (defVersion->GetOriginalRegNO() == differOrigNO) {
      auto &phiOpnd = static_cast<const PhiOperand&>(phiIt.second->GetOperand(kInsnSecondOpnd));
      if (phiOpnd.GetOperands().size() == validDifferRegNOs.size()) {
        bool exist = true;
        for (auto &phiListIt : phiOpnd.GetOperands()) {
          VRegVersion *phiUseVersion = optSsaInfo->FindSSAVersion(
              static_cast<RegOperand*>(phiListIt.second)->GetRegisterNumber());
          if (validDifferOrigRegNOs.find(phiUseVersion->GetOriginalRegNO()) == validDifferOrigRegNOs.end()) {
            exist = false;
            break;
          }
        }
        if (exist) {
          return &static_cast<RegOperand&>(phiIt.second->GetOperand(kInsnFirstOpnd));
        }
      }
    }
  }
  return nullptr;
}

bool A64PregCopyPattern::IsDifferentSSAVersion() const {
  for (auto *insn : validDefInsns) {
    ASSERT(insn->GetOperandSize() > 0, "invalid insn to do prop");
    Operand &dstOpnd = insn->GetOperand(kInsnFirstOpnd);
    CHECK_FATAL(dstOpnd.IsRegister(), "invalid insn to do prop");
    regno_t dstSSARegNo = static_cast<RegOperand&>(dstOpnd).GetRegisterNumber();
    VRegVersion *ssaVersion = optSsaInfo->FindSSAVersion(dstSSARegNo);
    if (ssaVersion->GetOriginalRegNO() == differOrigNO) {
      return true;
    }
  }
  return false;
}

RegOperand &A64PregCopyPattern::DFSBuildPhiInsn(Insn *curInsn, std::unordered_map<uint32, RegOperand*> &visited) {
  CHECK_FATAL(curInsn, "curInsn must not be null");
  if (visited[curInsn->GetId()] != nullptr) {
    return *visited[curInsn->GetId()];
  }
  if (!curInsn->IsPhi()) {
    return static_cast<RegOperand&>(curInsn->GetOperand(static_cast<uint32>(differIdx)));
  }
  std::unordered_map<uint32, RegOperand*> differPhiList;
  std::vector<regno_t> validDifferRegNOs;
  auto &phiOpnd = static_cast<const PhiOperand&>(curInsn->GetOperand(kInsnSecondOpnd));
  for (auto &phiListIt : phiOpnd.GetOperands()) {
    auto &useOpnd = static_cast<RegOperand&>(*phiListIt.second);
    VRegVersion *useVersion = optSsaInfo->FindSSAVersion(useOpnd.GetRegisterNumber());
    Insn *defInsn = FindDefInsn(useVersion);
    CHECK_FATAL(defInsn != nullptr, "get defInsn failed");
    RegOperand &phiDefOpnd = DFSBuildPhiInsn(defInsn, visited);
    (void)differPhiList.emplace(phiListIt.first, &phiDefOpnd);
    (void)validDifferRegNOs.emplace_back(phiDefOpnd.GetRegisterNumber());
  }
  /*
   * The phi in control flow may already exists.
   * For example:
   *                              [BB26]                         [BB45]
   *                          add R191, R103, R187           add R166, R103, R164
   *                                              \         /
   *                                               \       /
   *                                                [BB27]
   *                                      phi: R192, (R191<26>, R166<45>)  ------ curInsn
   *                                      phi: R194, (R187<26>, R164<45>)  ------ the phi witch we need already exists
   *                                                 /                            validDifferRegNOs : [187, 164]
   *                                                /
   *                   [BB28]                    [BB46]
   *              add R215, R103, R211           /
   *                                  \         /
   *                                   \       /
   *                                    [BB29]
   *                            phi: R216, (R215<28>, R192<46>)
   *                            phi: R218, (R211<28>, R194<46>)  ------ the phi witch we need already exists
   *                            mov R0, R216                            validDifferRegNOs : [211, 194]
   */
  RegOperand *existPhiDef = CheckAndGetExistPhiDef(*curInsn, validDifferRegNOs);
  if (existPhiDef == nullptr) {
    Insn &phiInsn = CreateNewPhiInsn(differPhiList, curInsn);
    visited[curInsn->GetId()] = &static_cast<RegOperand&>(phiInsn.GetOperand(kInsnFirstOpnd));
    existPhiDef = &static_cast<RegOperand&>(phiInsn.GetOperand(kInsnFirstOpnd));
  }
  return *existPhiDef;
}

void A64PregCopyPattern::Optimize(Insn &insn) {
  Insn *defInsn = *validDefInsns.begin();
  MOperator newMop = defInsn->GetMachineOpcode();
  Operand &dstOpnd = insn.GetOperand(kInsnFirstOpnd);
  Insn &newInsn = cgFunc.GetInsnBuilder()->BuildInsn(newMop, AArch64CG::kMd[newMop]);
  uint32 opndNum = defInsn->GetOperandSize();
  newInsn.ResizeOpnds(opndNum);
  if (!isCrossPhi) {
    for (uint32 i = 0; i < opndNum; ++i) {
      if (defInsn->OpndIsDef(i)) {
        newInsn.SetOperand(i, dstOpnd);
      } else {
        newInsn.SetOperand(i, defInsn->GetOperand(i));
      }
    }
  } else {
    std::unordered_map<uint32, RegOperand*> visited;
    auto *differPhiDefOpnd = &DFSBuildPhiInsn(firstPhiInsn, visited);
    CHECK_FATAL(differPhiDefOpnd, "get differPhiDefOpnd failed");
    for (uint32 i = 0; i < opndNum; ++i) {
      if (defInsn->OpndIsDef(i)) {
        newInsn.SetOperand(i, dstOpnd);
      } else if (i == static_cast<uint32>(differIdx)) {
        newInsn.SetOperand(i, *differPhiDefOpnd);
      } else {
        newInsn.SetOperand(i, defInsn->GetOperand(i));
      }
    }
  }
  insn.GetBB()->ReplaceInsn(insn, newInsn);
  /* update ssa info */
  optSsaInfo->ReplaceInsn(insn, newInsn);

  if (PROP_DUMP) {
    LogInfo::MapleLogger() << ">>>>>>> In A64PregCopyPattern : <<<<<<<\n";
    LogInfo::MapleLogger() << "======= ReplaceInsn :\n";
    insn.Dump();
    LogInfo::MapleLogger() << "======= NewInsn :\n";
    newInsn.Dump();
  }
}

void A64PregCopyPattern::Run() {
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
  validDefInsns.clear();
  validDefInsns.shrink_to_fit();
}

void A64ReplaceRegOpndVisitor::Visit(RegOperand *v) {
  (void)v;
  insn->SetOperand(idx, *newReg);
}
void A64ReplaceRegOpndVisitor::Visit(MemOperand *a64memOpnd) {
  bool changed = false;
  CHECK_FATAL(a64memOpnd->IsIntactIndexed(), "NYI post/pre index model");
  auto *cpyMem = a64memOpnd->Clone(*cgFunc->GetMemoryPool());
  if (cpyMem->GetBaseRegister() != nullptr &&
      cpyMem->GetBaseRegister()->GetRegisterNumber() == oldReg->GetRegisterNumber()) {
    cpyMem->SetBaseRegister(*static_cast<RegOperand*>(newReg));
    changed = true;
  }
  if (cpyMem->GetIndexRegister() != nullptr &&
      cpyMem->GetIndexRegister()->GetRegisterNumber() == oldReg->GetRegisterNumber()) {
    CHECK_FATAL(!changed, "base reg is equal to index reg");
    cpyMem->SetIndexRegister(*newReg);
    changed = true;
  }
  if (changed) {
    insn->SetMemOpnd(cpyMem);
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
