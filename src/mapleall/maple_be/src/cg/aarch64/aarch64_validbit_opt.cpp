/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "aarch64_validbit_opt.h"
#include "aarch64_cg.h"

namespace maplebe {

void PropPattern::ValidateImplicitCvt(RegOperand &destReg, const RegOperand &srcReg, Insn &movInsn) const {
  ASSERT(movInsn.GetMachineOpcode() == MOP_xmovrr || movInsn.GetMachineOpcode() == MOP_wmovrr, "NIY explicit CVT");
  if (destReg.GetSize() == k64BitSize && srcReg.GetSize() == k32BitSize) {
    movInsn.SetMOP(AArch64CG::kMd[MOP_xuxtw64]);
  } else if (destReg.GetSize() == k32BitSize && srcReg.GetSize() == k64BitSize) {
    movInsn.SetMOP(AArch64CG::kMd[MOP_xubfxrri6i6]);
    movInsn.AddOperand(cgFunc->CreateImmOperand(PTY_i64, 0));
    movInsn.AddOperand(cgFunc->CreateImmOperand(PTY_i64, k32BitSize));
  } else {
    return;
  }
}

// prop ssa info and change implicit cvt to uxtw / ubfx
void PropPattern::ReplaceImplicitCvtAndProp(VRegVersion *destVersion, VRegVersion *srcVersion) const {
  MapleUnorderedMap<uint32, DUInsnInfo*> useList = destVersion->GetAllUseInsns();
  ssaInfo->ReplaceAllUse(destVersion, srcVersion);
  for (auto it = useList.begin(); it != useList.end(); ++it)  {
    Insn *useInsn = it->second->GetInsn();
    if (useInsn->GetMachineOpcode() == MOP_xmovrr || useInsn->GetMachineOpcode() == MOP_wmovrr) {
      auto &dstOpnd = useInsn->GetOperand(kFirstOpnd);
      auto &srcOpnd = useInsn->GetOperand(kSecondOpnd);
      ASSERT(dstOpnd.IsRegister() && srcOpnd.IsRegister(), "must be");
      auto &destReg = static_cast<RegOperand &>(dstOpnd);
      auto &srcReg = static_cast<RegOperand &>(srcOpnd);
      // for preg case, do not change mop because preg can not be proped later.
      if (useInsn->GetMachineOpcode() == MOP_wmovrr && destReg.IsPhysicalRegister()) {
        ssaInfo->InsertSafePropInsn(useInsn->GetId());
        continue;
      }
      ValidateImplicitCvt(destReg, srcReg, *useInsn);
    }
  }
}

void AArch64ValidBitOpt::DoOpt() {
  FOR_ALL_BB(bb, cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      OptPatternWithImplicitCvt(*bb, *insn);
    }
  }
  FOR_ALL_BB(bb, cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      OptCvt(*bb, *insn);
    }
  }
  FOR_ALL_BB(bb, cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      OptPregCvt(*bb, *insn);
    }
  }
}

// modify insn's mop from X to W
void RedundantExpandProp::ModifyInsnUseMopW() {
  static const std::map<MOperator, MOperator> kInsnMopX2WMap = {
      {MOP_xcbnz, MOP_wcbnz},
      {MOP_xcbz, MOP_wcbz},
      {MOP_xtbnz, MOP_wtbnz},
      {MOP_xtbz, MOP_wtbz},
      {MOP_xubfizrri6i6, MOP_wubfizrri5i5},
      {MOP_xsbfizrri6i6, MOP_wsbfizrri5i5},
      {MOP_xubfxrri6i6, MOP_wubfxrri5i5},
      {MOP_xsbfxrri6i6, MOP_wsbfxrri5i5},
      {MOP_xcmnri, MOP_wcmnri}
  };

  for (auto *insn : needModifyInsns) {
    auto iter = kInsnMopX2WMap.find(insn->GetMachineOpcode());
    CHECK_FATAL(iter != kInsnMopX2WMap.end(), "insn must be modified");
    insn->SetMOP(AArch64CG::kMd[iter->second]);
  }
}

void RedundantExpandProp::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  ModifyInsnUseMopW();

  auto &destOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  auto &srcOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  if (destOpnd.IsSSAForm() && srcOpnd.IsSSAForm()) {
    auto *destVersion = ssaInfo->FindSSAVersion(destOpnd.GetRegisterNumber());
    ASSERT(destVersion != nullptr, "find Version failed");
    auto *srcVersion = ssaInfo->FindSSAVersion(srcOpnd.GetRegisterNumber());
    ASSERT(srcVersion != nullptr, "find Version failed");
    ReplaceImplicitCvtAndProp(destVersion, srcVersion);
  } else {  // only change mop because there is no ssa version
    auto &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(MOP_wmovrr, destOpnd, srcOpnd);
    bb.ReplaceInsn(insn, newInsn);
    ssaInfo->ReplaceInsn(insn, newInsn);
  }
}

// check others insn can use mopW
bool RedundantExpandProp::CheckOtherInsnUseMopW(const RegOperand &defOpnd, DUInsnInfo &useInfo) const {
  auto defRegNO = defOpnd.GetRegisterNumber();
  const auto *useInsn = useInfo.GetInsn();
  for (auto [idx, count] : std::as_const(useInfo.GetOperands())) {
    const auto &opnd = useInsn->GetOperand(idx);
    auto *opndMd = useInsn->GetDesc()->GetOpndDes(idx);
    if (opnd.IsRegister()) {
      if (!opndMd->IsUse() || opnd.GetSize() != k32BitSize || opndMd->GetSize() != k32BitSize) {
        return false;
      }
    } else if (opnd.IsMemoryAccessOperand()) {
      const auto &memOpnd = static_cast<const MemOperand&>(opnd);
      if (memOpnd.GetAddrMode() != MemOperand::kBOE && memOpnd.GetAddrMode() != MemOperand::kBOL) {
        return false;
      }
      if (memOpnd.GetIndexRegister()->GetRegisterNumber() != defRegNO) {
        return false;
      }
      return (memOpnd.GetIndexRegister()->GetSize() == k32BitSize);
    } else if (opnd.IsList()) {
      for (const auto *regOpnd : static_cast<const ListOperand&>(opnd).GetOperands()) {
        if (regOpnd->GetRegisterNumber() == defRegNO && regOpnd->GetSize() != k32BitSize) {
          return false;
        }
      }
    }
  }
  return true;
}

bool RedundantExpandProp::CheckInsnCanUseMopW(const RegOperand &defOpnd, DUInsnInfo &useInfo) {
  auto *useInsn = useInfo.GetInsn();
  switch (useInsn->GetMachineOpcode()) {
    case MOP_xcbnz:
    case MOP_xcbz: {
      needModifyInsns.push_back(useInsn);
      return true;
    }
    case MOP_xtbnz:
    case MOP_xtbz: {
      ASSERT(useInsn->GetOperand(kInsnSecondOpnd).IsImmediate(), "tbz/tbnz second opnd must be immediate");
      needModifyInsns.push_back(useInsn);
      auto &immOpnd = static_cast<ImmOperand&>(useInsn->GetOperand(kInsnSecondOpnd));
      return (immOpnd.GetValue() < static_cast<int64>(k32BitSize));
    }
    case MOP_xubfizrri6i6:
    case MOP_xsbfizrri6i6:
    case MOP_xubfxrri6i6:
    case MOP_xsbfxrri6i6: {
      ASSERT(useInsn->GetOperand(kInsnThirdOpnd).IsImmediate(), "third opnd must be immediate");
      ASSERT(useInsn->GetOperand(kInsnFourthOpnd).IsImmediate(), "fourth opnd must be immediate");
      needModifyInsns.push_back(useInsn);
      auto &lsb = static_cast<ImmOperand&>(useInsn->GetOperand(kInsnThirdOpnd));
      auto &width = static_cast<ImmOperand&>(useInsn->GetOperand(kInsnFourthOpnd));
      return (lsb.GetValue() + width.GetValue() <= k32BitSize);
    };
    case MOP_xcmnri: {
      ASSERT(useInsn->GetOperand(kInsnThirdOpnd).IsImmediate(), "cmpri third opnd must be immediate");
      needModifyInsns.push_back(useInsn);
      auto &immOpnd = static_cast<ImmOperand&>(useInsn->GetOperand(kInsnThirdOpnd));
      return (immOpnd.GetValue() > INT32_MIN) && (immOpnd.GetValue() <= INT32_MAX);
    }
    default: {
      return CheckOtherInsnUseMopW(defOpnd, useInfo);
    }
  }
}

bool RedundantExpandProp::CheckPhiSrcRegDefW(const RegOperand &regOpnd) const {
  // all opnd of phiInsn should have the ValidBit of no more than 32 bits
  if (regOpnd.GetValidBitsNum() <= k32BitSize) {
    return true;
  }
  auto *regVersion = ssaInfo->FindSSAVersion(regOpnd.GetRegisterNumber());
  ASSERT(regVersion != nullptr, "find Version failed");
  if (regVersion->GetDefInsnInfo() == nullptr) {
    return true;  // undef reg
  }
  auto *defInsn = regVersion->GetDefInsnInfo()->GetInsn();
  ASSERT(defInsn != nullptr, "find defInsn failed");
  if (defInsn->GetMachineOpcode() == MOP_wmovrr) {
    // when mOp is wmov and src is preg, the validBit is 64-bits
    // however, the lower 32-bits can be used for calculation
    const auto &defUseOpnd = static_cast<RegOperand&>(defInsn->GetOperand(kInsnSecondOpnd));
    return !cgFunc->GetTargetRegInfo()->IsVirtualRegister(defUseOpnd);
  }
  return false;
}

// check defOpnd's all use-point can use mopW
bool RedundantExpandProp::CheckAllUseInsnCanUseMopW(const RegOperand &defOpnd, InsnSet &visitedInsn) {
  auto *defVersion = ssaInfo->FindSSAVersion(defOpnd.GetRegisterNumber());
  ASSERT(defVersion != nullptr, "find Version failed");
  for (auto destUseIt : defVersion->GetAllUseInsns()) {
    auto *useInsn = destUseIt.second->GetInsn();
    if (visitedInsn.count(useInsn) != 0) {
      continue;
    }
    visitedInsn.insert(useInsn);
    auto &propInsns = ssaInfo->GetSafePropInsns();
    if (std::find(propInsns.begin(), propInsns.end(), useInsn->GetId()) != propInsns.end()) {
      return false;
    }
    if (useInsn->IsPhi()) { // phi insn, need dfs check
      auto &phiDefOpnd = useInsn->GetOperand(kInsnFirstOpnd);
      CHECK_FATAL(phiDefOpnd.IsRegister(), "must be register");
      if (!CheckAllUseInsnCanUseMopW(static_cast<RegOperand&>(phiDefOpnd), visitedInsn)) {
        return false;
      }
      checkPhiInsns.emplace_back(&defOpnd, useInsn);
    } else if (!CheckInsnCanUseMopW(defOpnd, *destUseIt.second)) {
      return false;
    }
  }
  return true;
}

bool RedundantExpandProp::CheckCondition(Insn &insn) {
  ASSERT(insn.GetOperand(kInsnFirstOpnd).IsRegister(), "must be register");
  auto &destOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  if (!destOpnd.IsSSAForm()) {
    return false;
  }
  InsnSet visitedInsn;
  visitedInsn.emplace(&insn);
  if (!CheckAllUseInsnCanUseMopW(destOpnd, visitedInsn)) {
    return false;
  }
  if (!needModifyInsns.empty()) {
    // if insns needs to be modified, need check all phiInsnUseReg is defined as 32 bits.
    for (auto [noNeedCheckReg, phiInsn] : std::as_const(checkPhiInsns)) {
      ASSERT(phiInsn->GetOperand(kInsnSecondOpnd).IsPhi(), "expect phiOpnd");
      auto &phiOpnd = static_cast<PhiOperand&>(phiInsn->GetOperand(kInsnSecondOpnd));
      for (const auto phiOpndIt : std::as_const(phiOpnd.GetOperands())) {
        if (phiOpndIt.second == nullptr ||
            phiOpndIt.second->GetRegisterNumber() == noNeedCheckReg->GetRegisterNumber()) {
          continue;
        }
        if (!CheckPhiSrcRegDefW(*phiOpndIt.second)) {
          return false;
        }
      }
    }
  }
  return true;
}

// Patterns that may have implicit cvt
void AArch64ValidBitOpt::OptPatternWithImplicitCvt(BB &bb, Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  switch (curMop) {
    case MOP_bge:
    case MOP_blt:
    case MOP_beq:
    case MOP_bne: {
      OptimizeNoProp<CmpBranchesPattern>(bb, insn);
      break;
    }
    case MOP_wcsetrc:
    case MOP_xcsetrc: {
      OptimizeNoProp<CmpCsetVBPattern>(bb, insn);
      break;
    }
    case MOP_xasrrri6:
    case MOP_wasrrri5: {
      OptimizeNoProp<RSPattern>(bb, insn);
      break;
    }
    case MOP_xrevrr:
    case MOP_wrevrr:
    case MOP_wrevrr16: {
      OptimizeNoProp<RevCbzToCbzPattern>(bb, insn);
      break;
    }
    default:
      break;
  }
}

// In OptCvt, optimize all cvt
// Should convert implicit mov to explicit uxtw / ubfx in pattern.
void AArch64ValidBitOpt::OptCvt(BB &bb, Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  switch (curMop) {
    case MOP_xuxtb32:
    case MOP_xuxth32:
    case MOP_xuxtw64:
    case MOP_xsxtw64:
    case MOP_wubfxrri5i5:
    case MOP_xubfxrri6i6:
    case MOP_wsbfxrri5i5:
    case MOP_xsbfxrri6i6: {
      OptimizeProp<ExtValidBitPattern>(bb, insn);
      break;
    }
    case MOP_wandrri12:
    case MOP_xandrri13: {
      OptimizeProp<AndValidBitPattern>(bb, insn);
      break;
    }
    default:
      break;
  }
}

// patterns with uxtw vreg preg
void AArch64ValidBitOpt::OptPregCvt(BB &bb, Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  switch (curMop) {
    case MOP_xuxtw64:
    case MOP_xsxtw64: {
      OptimizeProp<RedundantExpandProp>(bb, insn);
      break;
    }
    default:
      break;
  }
}

bool AArch64ValidBitOpt::SetValidBits(Insn &insn) {
  bool hasChange = false;
  auto checkChangeAndSetValidBitsNum = [&hasChange](RegOperand &dstOpnd, uint32 newVB) {
    if (dstOpnd.GetValidBitsNum() != newVB) {
      dstOpnd.SetValidBitsNum(newVB);
      hasChange = true;
    }
  };

  MOperator mop = insn.GetMachineOpcode();
  switch (mop) {
    // for case sbfx 32 64
    // we can not deduce that dst opnd valid bit num;
    case MOP_xsbfxrri6i6: {
      auto &dstOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
      checkChangeAndSetValidBitsNum(dstOpnd, k64BitSize);
      break;
    }
    case MOP_wcsetrc:
    case MOP_xcsetrc: {
      auto &dstOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
      checkChangeAndSetValidBitsNum(dstOpnd, k1BitSize);
      break;
    }
    case MOP_wmovri32:
    case MOP_xmovri64: {
      Operand &srcOpnd = insn.GetOperand(kInsnSecondOpnd);
      ASSERT(srcOpnd.IsIntImmediate(), "must be ImmOperand");
      auto &immOpnd = static_cast<ImmOperand&>(srcOpnd);
      auto &dstOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
      if (!cgFunc->GetTargetRegInfo()->IsVirtualRegister(dstOpnd)) {
        return false;
      }
      checkChangeAndSetValidBitsNum(dstOpnd, GetImmValidBit(immOpnd.GetValue(), dstOpnd.GetSize()));
      break;
    }
    case MOP_xmovrr:
    case MOP_wmovrr: {
      auto &dstOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
      auto &srcOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
      if (!cgFunc->GetTargetRegInfo()->IsVirtualRegister(dstOpnd)) {
        return false;
      }
      uint32 newVB = srcOpnd.GetValidBitsNum();
      if (!cgFunc->GetTargetRegInfo()->IsVirtualRegister(srcOpnd)) {
        newVB = (srcOpnd.GetRegisterNumber() == RZR ? k1BitSize : k64BitSize);
      }
      if (!(dstOpnd.GetSize() == k64BitSize && srcOpnd.GetSize() == k32BitSize) &&
          !(dstOpnd.GetSize() == k32BitSize && srcOpnd.GetSize() == k64BitSize)) {
        checkChangeAndSetValidBitsNum(dstOpnd, newVB);
      }
      break;
    }
    case MOP_wlsrrri5:
    case MOP_xlsrrri6: {
      Operand &opnd = insn.GetOperand(kInsnThirdOpnd);
      ASSERT(opnd.IsIntImmediate(), "must be ImmOperand");
      auto shiftBits = static_cast<int32>(static_cast<ImmOperand&>(opnd).GetValue());
      auto &dstOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
      auto &srcOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
      if ((static_cast<int32>(srcOpnd.GetValidBitsNum()) - shiftBits) <= 0) {
        checkChangeAndSetValidBitsNum(dstOpnd, k1BitSize);
      } else {
        checkChangeAndSetValidBitsNum(dstOpnd, srcOpnd.GetValidBitsNum() - static_cast<uint32>(shiftBits));
      }
      break;
    }
    case MOP_wlslrri5:
    case MOP_xlslrri6: {
      Operand &opnd = insn.GetOperand(kInsnThirdOpnd);
      ASSERT(opnd.IsIntImmediate(), "must be ImmOperand");
      auto shiftBits = static_cast<uint32>(static_cast<ImmOperand&>(opnd).GetValue());
      auto &dstOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
      auto &srcOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
      uint32 newVB = ((srcOpnd.GetValidBitsNum() + shiftBits) > srcOpnd.GetSize()) ?
                     srcOpnd.GetSize() : (srcOpnd.GetValidBitsNum() + shiftBits);
      checkChangeAndSetValidBitsNum(dstOpnd, newVB);
      break;
    }
    case MOP_wasrrri5:
    case MOP_xasrrri6: {
      auto &srcOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
      if ((mop == MOP_wasrrri5 && srcOpnd.GetValidBitsNum() < k32BitSize) ||
          (mop == MOP_xasrrri6 && srcOpnd.GetValidBitsNum() < k64BitSize)) {
        Operand &opnd = insn.GetOperand(kInsnThirdOpnd);
        ASSERT(opnd.IsIntImmediate(), "must be ImmOperand");
        auto shiftBits = static_cast<int32>(static_cast<ImmOperand&>(opnd).GetValue());
        auto &dstOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
        if ((static_cast<int32>(srcOpnd.GetValidBitsNum()) - shiftBits) <= 0) {
          checkChangeAndSetValidBitsNum(dstOpnd, k1BitSize);
        } else {
          checkChangeAndSetValidBitsNum(dstOpnd, srcOpnd.GetValidBitsNum() - static_cast<uint32>(shiftBits));
        }
      }
      break;
    }
    case MOP_xuxtb32:
    case MOP_xuxth32: {
      auto &dstOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
      auto &srcOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
      uint32 srcVB = srcOpnd.GetValidBitsNum();
      uint32 newVB = dstOpnd.GetValidBitsNum();
      newVB = (mop == MOP_xuxtb32) ? ((srcVB < k8BitSize) ? srcVB : k8BitSize) : newVB;
      newVB = (mop == MOP_xuxth32) ? ((srcVB < k16BitSize) ? srcVB : k16BitSize) : newVB;
      checkChangeAndSetValidBitsNum(dstOpnd, newVB);
      break;
    }
    case MOP_xuxtw64: {
      auto &dstOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
      auto &srcOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
      if (srcOpnd.GetValidBitsNum() <= k32BitSize) {
        checkChangeAndSetValidBitsNum(dstOpnd, srcOpnd.GetValidBitsNum());
      }
      break;
    }
    case MOP_wubfxrri5i5:
    case MOP_xubfxrri6i6: {
      auto &dstOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
      auto &widthOpnd = static_cast<ImmOperand&>(insn.GetOperand(kInsnFourthOpnd));
      checkChangeAndSetValidBitsNum(dstOpnd, static_cast<uint32>(widthOpnd.GetValue()));
      break;
    }
    case MOP_wldrb:
    case MOP_wldrh: {
      auto &dstOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
      uint32 newVB = (mop == MOP_wldrb) ? k8BitSize : k16BitSize;
      checkChangeAndSetValidBitsNum(dstOpnd, newVB);
      break;
    }
    case MOP_wandrrr:
    case MOP_xandrrr: {
      auto &dstOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
      uint32 src1VB = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd)).GetValidBitsNum();
      uint32 src2VB = static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd)).GetValidBitsNum();
      uint32 newVB = (src1VB <= src2VB ? src1VB : src2VB);
      checkChangeAndSetValidBitsNum(dstOpnd, newVB);
      break;
    }
    case MOP_wandrri12:
    case MOP_xandrri13: {
      auto &dstOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
      auto &immOpnd = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
      uint32 src1VB = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd)).GetValidBitsNum();
      uint32 src2VB = GetImmValidBit(immOpnd.GetValue(), dstOpnd.GetSize());
      uint32 newVB = (src1VB <= src2VB ? src1VB : src2VB);
      checkChangeAndSetValidBitsNum(dstOpnd, newVB);
      break;
    }
    case MOP_wiorrrr: {
      auto &dstOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
      uint32 src1VB = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd)).GetValidBitsNum();
      uint32 src2VB = static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd)).GetValidBitsNum();
      uint32 newVB = (src1VB >= src2VB ? src1VB : src2VB);
      if (newVB > k32BitSize) {
        newVB = k32BitSize;
      }
      checkChangeAndSetValidBitsNum(dstOpnd, newVB);
      break;
    }
    case MOP_xiorrrr: {
      auto &dstOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
      uint32 src1VB = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd)).GetValidBitsNum();
      uint32 src2VB = static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd)).GetValidBitsNum();
      uint32 newVB = (src1VB >= src2VB ? src1VB : src2VB);
      checkChangeAndSetValidBitsNum(dstOpnd, newVB);
      break;
    }
    case MOP_wiorrri12: {
      auto &dstOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
      auto &immOpnd = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
      uint32 src1VB = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd)).GetValidBitsNum();
      uint32 src2VB = GetImmValidBit(immOpnd.GetValue(), dstOpnd.GetSize());
      uint32 newVB = (src1VB >= src2VB ? src1VB : src2VB);
      if (newVB > k32BitSize) {
        newVB = k32BitSize;
      }
      checkChangeAndSetValidBitsNum(dstOpnd, newVB);
      break;
    }
    case MOP_xiorrri13: {
      auto &dstOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
      auto &immOpnd = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
      uint32 src1VB = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd)).GetValidBitsNum();
      uint32 src2VB = GetImmValidBit(immOpnd.GetValue(), dstOpnd.GetSize());
      uint32 newVB = (src1VB >= src2VB ? src1VB : src2VB);
      checkChangeAndSetValidBitsNum(dstOpnd, newVB);
      break;
    }
    case MOP_wrevrr16: {
      auto &dstOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
      auto &srcOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
      if (srcOpnd.GetValidBitsNum() <= k16BitSize) {
        dstOpnd.SetValidBitsNum(k16BitSize);
      }
      break;
    }
    case MOP_wubfizrri5i5:
    case MOP_xubfizrri6i6: {
      auto &dstOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
      auto &lsb = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
      auto &width = static_cast<ImmOperand&>(insn.GetOperand(kInsnFourthOpnd));
      uint32 newVB = static_cast<uint32>(lsb.GetValue() + width.GetValue());
      checkChangeAndSetValidBitsNum(dstOpnd, newVB);
      break;
    }
    default:
      break;
  }
  return hasChange;
}

bool AArch64ValidBitOpt::SetPhiValidBits(Insn &insn) {
  Operand &defOpnd = insn.GetOperand(kInsnFirstOpnd);
  ASSERT(defOpnd.IsRegister(), "expect register");
  auto &defRegOpnd = static_cast<RegOperand&>(defOpnd);
  Operand &phiOpnd = insn.GetOperand(kInsnSecondOpnd);
  ASSERT(phiOpnd.IsPhi(), "expect phiList");
  auto &phiList = static_cast<PhiOperand&>(phiOpnd);
  int32 maxVB = -1;
  for (const auto phiOpndIt : phiList.GetOperands()) {
    if (phiOpndIt.second != nullptr) {
      maxVB = (maxVB < static_cast<int32>(phiOpndIt.second->GetValidBitsNum())) ?
              static_cast<int32>(phiOpndIt.second->GetValidBitsNum()) : maxVB;
    }
  }
  if (maxVB >= static_cast<int32>(k0BitSize) && static_cast<uint32>(maxVB) != defRegOpnd.GetValidBitsNum()) {
    defRegOpnd.SetValidBitsNum(static_cast<uint32>(maxVB));
    return true;
  }
  return false;
}

static bool IsZeroRegister(const Operand &opnd) {
  if (!opnd.IsRegister()) {
    return false;
  }
  const RegOperand *regOpnd = static_cast<const RegOperand*>(&opnd);
  return regOpnd->GetRegisterNumber() == RZR;
}

bool AndValidBitPattern::CheckImmValidBit(int64 andImm, uint32 andImmVB, int64 shiftImm) const {
  if ((__builtin_ffs(static_cast<int>(andImm)) - 1 == shiftImm) &&
      ((static_cast<uint64>(andImm) >> static_cast<uint64>(shiftImm)) ==
      ((1UL << (andImmVB - static_cast<uint64>(shiftImm))) - 1))) {
    return true;
  }
  return false;
}

bool AndValidBitPattern::CheckCondition(Insn &insn) {
  MOperator mOp = insn.GetMachineOpcode();
  if (mOp == MOP_wandrri12) {
    newMop = MOP_wmovrr;
  } else if (mOp == MOP_xandrri13) {
    newMop = MOP_xmovrr;
  }
  if (newMop == MOP_undef) {
    return false;
  }
  CHECK_FATAL(insn.GetOperand(kInsnFirstOpnd).IsRegister(), "must be register!");
  CHECK_FATAL(insn.GetOperand(kInsnSecondOpnd).IsRegister(), "must be register!");
  CHECK_FATAL(insn.GetOperand(kInsnThirdOpnd).IsImmediate(), "must be imm!");
  desReg = static_cast<RegOperand*>(&insn.GetOperand(kInsnFirstOpnd));
  srcReg = static_cast<RegOperand*>(&insn.GetOperand(kInsnSecondOpnd));
  auto &andImm = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
  int64 immVal = andImm.GetValue();
  uint32 validBit = srcReg->GetValidBitsNum();
  if (validBit <= k8BitSize && immVal == 0xFF) {
    return true;
  } else if (validBit <= k16BitSize && immVal == 0xFFFF) {
    return true;
  } else if (validBit <= k32BitSize && immVal == 0xFFFFFFFF) {
    return true;
  }
  /* and R287[32], R286[64], #255 */
  if ((desReg->GetSize() < srcReg->GetSize()) && (srcReg->GetValidBitsNum() > desReg->GetSize())) {
    return false;
  }
  InsnSet useInsns = GetAllUseInsn(*desReg);
  if (useInsns.size() == 1) {
    Insn *useInsn = *useInsns.begin();
    MOperator useMop = useInsn->GetMachineOpcode();
    if (useMop != MOP_wasrrri5 && useMop != MOP_xasrrri6 && useMop != MOP_wlsrrri5 && useMop != MOP_xlsrrri6) {
      return false;
    }
    Operand &shiftOpnd = useInsn->GetOperand(kInsnThirdOpnd);
    CHECK_FATAL(shiftOpnd.IsImmediate(), "must be immediate");
    int64 shiftImm = static_cast<ImmOperand&>(shiftOpnd).GetValue();
    uint32 andImmVB = ValidBitOpt::GetImmValidBit(andImm.GetValue(), desReg->GetSize());
    if ((srcReg->GetValidBitsNum() == andImmVB) && CheckImmValidBit(andImm.GetValue(), andImmVB, shiftImm)) {
      return true;
    }
  }
  return false;
}

void AndValidBitPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  if (desReg != nullptr && desReg->IsSSAForm() && srcReg != nullptr && srcReg->IsSSAForm()) {
    destVersion = ssaInfo->FindSSAVersion(desReg->GetRegisterNumber());
    ASSERT(destVersion != nullptr, "find Version failed");
    srcVersion = ssaInfo->FindSSAVersion(srcReg->GetRegisterNumber());
    ASSERT(srcVersion != nullptr, "find Version failed");
    //prop ssa info
    cgFunc->InsertExtendSet(srcVersion->GetSSAvRegOpnd()->GetRegisterNumber());
    ReplaceImplicitCvtAndProp(destVersion, srcVersion);
  } else {
    return;
  }
}

bool ExtValidBitPattern::RealUseMopX(const RegOperand &defOpnd, InsnSet &visitedInsn) {
  VRegVersion *vdestVersion = ssaInfo->FindSSAVersion(defOpnd.GetRegisterNumber());
  for (auto destUseIt : vdestVersion->GetAllUseInsns()) {
    Insn *useInsn = destUseIt.second->GetInsn();
    if (visitedInsn.count(useInsn) != 0) {
      continue;
    }
    visitedInsn.insert(useInsn);
    if (useInsn->IsPhi()) {
      auto &phiDefOpnd = useInsn->GetOperand(kInsnFirstOpnd);
      CHECK_FATAL(phiDefOpnd.IsRegister(), "must be register");
      auto &phiRegDefOpnd = static_cast<RegOperand&>(phiDefOpnd);
      if (RealUseMopX(phiRegDefOpnd, visitedInsn)) {
        return true;
      }
    }
    if (useInsn->GetMachineOpcode() == MOP_xuxtw64) {
      return true;
    }
    const InsnDesc *useMD = &AArch64CG::kMd[useInsn->GetMachineOpcode()];
    for (auto &opndUseIt : as_const(destUseIt.second->GetOperands())) {
      const OpndDesc *useProp = useMD->GetOpndDes(opndUseIt.first);
      if (useProp->GetSize() == k64BitSize) {
        return true;
      }
    }
  }
  return false;
}

// case1
// uxth/uxtb R0 R1 (redundant)
// strh/strb R0 R2

// case2
// uxth R0 R1 (redundant)
// rev R2 R0
// if there are insns that only use 8/16 bit of the register ,this pattern should be expanded.
bool ExtValidBitPattern::CheckRedundantUxtbUxth(const Insn &insn) {
  RegOperand *destOpnd = nullptr;
  RegOperand *srcOpnd = nullptr;
  std::set<MOperator> checkMops;
  if (insn.GetMachineOpcode() == MOP_xuxth32) {
    destOpnd = &static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
    srcOpnd = &static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
    checkMops = {MOP_wstrh, MOP_wrevrr16};
  }
  if (insn.GetMachineOpcode() == MOP_xuxtb32) {
    destOpnd = &static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
    srcOpnd = &static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
    checkMops = {MOP_wstrb};
  }
  if (destOpnd != nullptr && destOpnd->IsSSAForm() && srcOpnd != nullptr) {
    destVersion = ssaInfo->FindSSAVersion(destOpnd->GetRegisterNumber());
    ASSERT(destVersion != nullptr, "find Version failed");
    for (auto destUseIt : destVersion->GetAllUseInsns()) {
      Insn *useInsn = destUseIt.second->GetInsn();
      if (useInsn->GetMachineOpcode() == MOP_wandrri12 || useInsn->GetMachineOpcode() == MOP_xandrri13) {
        // uxtb  R101, R100
        // and   R102, R101, #14
        // when imm value = 0xFF, the and insn will be deleted by AndValidBitPattern
        CHECK_FATAL(useInsn->GetOperand(kInsnThirdOpnd).IsImmediate(), "must be imm!");
        auto &andImm = static_cast<ImmOperand&>(useInsn->GetOperand(kInsnThirdOpnd));
        if (andImm.GetValue() < 0) {  // when immediate number is less than 0, high-bits must be 1
          return false;
        }
        if (insn.GetMachineOpcode() == MOP_xuxth32 && andImm.GetValue() >= 0xFFFF) {
          return false;
        } else if (insn.GetMachineOpcode() == MOP_xuxtb32 && andImm.GetValue() >= 0xFF) {
          return false;
        }
      } else if (checkMops.find(useInsn->GetMachineOpcode()) == checkMops.end()) {
        return false;
      }
    }
    return true;
  }
  return false;
}

bool ExtValidBitPattern::CheckValidCvt(const Insn &insn) {
  // extend to all shift pattern in future
  RegOperand *destOpnd = nullptr;
  RegOperand *srcOpnd = nullptr;
  if (insn.GetMachineOpcode() == MOP_xuxtw64) {
    destOpnd = &static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
    srcOpnd = &static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  }
  if (insn.GetMachineOpcode() == MOP_xubfxrri6i6 || insn.GetMachineOpcode() == MOP_xsbfxrri6i6) {
    destOpnd = &static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
    srcOpnd = &static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
    auto &lsb = static_cast<ImmOperand &>(insn.GetOperand(kInsnThirdOpnd));
    auto &width = static_cast<ImmOperand &>(insn.GetOperand(kInsnFourthOpnd));
    if ((lsb.GetValue() != 0) || (width.GetValue() != k32BitSize)) {
      return false;
    }
  }
  if (destOpnd != nullptr && destOpnd->IsSSAForm() && srcOpnd != nullptr && srcOpnd->IsSSAForm()) {
    destVersion = ssaInfo->FindSSAVersion(destOpnd->GetRegisterNumber());
    srcVersion = ssaInfo->FindSSAVersion(srcOpnd->GetRegisterNumber());
    ASSERT(destVersion != nullptr, "find Version failed");
    for (auto destUseIt : destVersion->GetAllUseInsns()) {
      Insn *useInsn = destUseIt.second->GetInsn();
      // check case:
      // uxtw R1 R0
      // uxtw R2 R1
      // this case is forbidden becasue, these two insns would be removed and caused error.
      // the first insn would be removed because R1's only has 32 bit use point.
      // the second insn would be removed because R1's validbit is 32 bit (when we removed first one, we don't
      // update the validbit of R1.)
      if (useInsn->GetMachineOpcode() == MOP_xuxtw64) {
        return false;
      }
      // recursively check all real use mop, if there is one mop that use 64 bit size reg, do not optimize
      if (useInsn->IsPhi()) {
        auto &defOpnd = static_cast<RegOperand&>(useInsn->GetOperand(kInsnFirstOpnd));
        InsnSet visitedInsn;
        (void)visitedInsn.insert(useInsn);
        if (RealUseMopX(defOpnd, visitedInsn)) {
          return false;
        }
      }
      int32 lastOpndId = static_cast<int32>(useInsn->GetOperandSize() - 1);
      const InsnDesc *md = useInsn->GetDesc();
      // check case:
      // uxtw R1 R0
      // mopX R2 R1(64)
      for (int32 i = lastOpndId; i >= 0; --i) {
        auto *reg = (md->opndMD[static_cast<uint32>(i)]);
        auto &opnd = useInsn->GetOperand(static_cast<uint32>(i));
        if (reg->IsUse() && opnd.IsRegister() &&
            static_cast<RegOperand&>(opnd).GetRegisterNumber() == destOpnd->GetRegisterNumber()) {
          if (reg->GetSize() == k32BitSize) {
            continue;
          } else {
            return false;
          }
        }
      }
    }
    return true;
  }
  return false;
}

bool ExtValidBitPattern::CheckCondition(Insn &insn) {
  Operand &dstOpnd = insn.GetOperand(kInsnFirstOpnd);
  Operand &srcOpnd = insn.GetOperand(kInsnSecondOpnd);
  CHECK_FATAL(dstOpnd.IsRegister() && srcOpnd.IsRegister(), "must be register");
  MOperator mOp = insn.GetMachineOpcode();
  switch (mOp) {
    case MOP_xuxtb32:
    case MOP_xuxth32: {
      auto maxValidBit = (mOp == MOP_xuxtb32 ? k8BitSize : k16BitSize);
      if (CheckRedundantUxtbUxth(insn) ||
          (static_cast<RegOperand&>(srcOpnd).GetValidBitsNum() <= maxValidBit &&
          !static_cast<RegOperand&>(srcOpnd).IsPhysicalRegister())) { // Do not optimize callee ensuring vb of parameter
        newMop = MOP_wmovrr;
        break;
      }
      return false;
    }
    case MOP_xuxtw64: {
      if (CheckValidCvt(insn) || (static_cast<RegOperand&>(srcOpnd).GetValidBitsNum() <= k32BitSize &&
          !static_cast<RegOperand&>(srcOpnd).IsPhysicalRegister())) { // Do not optimize callee ensuring vb of parameter
        if (static_cast<RegOperand&>(srcOpnd).IsSSAForm() && srcVersion != nullptr) {
          srcVersion->SetImplicitCvt();
        }
        newMop = MOP_wmovrr;
        break;
      }
      return false;
    }
    case MOP_xsxtw64: {
      if (static_cast<RegOperand&>(srcOpnd).GetValidBitsNum() >= k32BitSize) {
        return false;
      }
      newMop = MOP_xmovrr;
      break;
    }
    case MOP_wubfxrri5i5:
    case MOP_xubfxrri6i6:
    case MOP_wsbfxrri5i5:
    case MOP_xsbfxrri6i6: {
      Operand &immOpnd1 = insn.GetOperand(kInsnThirdOpnd);
      Operand &immOpnd2 = insn.GetOperand(kInsnFourthOpnd);
      CHECK_FATAL(immOpnd1.IsImmediate(), "must be immediate");
      CHECK_FATAL(immOpnd2.IsImmediate(), "must be immediate");
      int64 lsb = static_cast<ImmOperand&>(immOpnd1).GetValue();
      int64 width = static_cast<ImmOperand&>(immOpnd2).GetValue();
      auto &srcRegOpnd = static_cast<RegOperand&>(srcOpnd);
      if (CheckValidCvt(insn)) {
        if (srcRegOpnd.IsSSAForm() && srcVersion != nullptr) {
          srcVersion->SetImplicitCvt();
        }
        newMop = MOP_xmovrr;
        break;
      }
      if (lsb != 0 || srcRegOpnd.GetValidBitsNum() > width ||
          srcRegOpnd.IsPhysicalRegister()) { // Do not optimize callee ensuring vb of parameter
        return false;
      }
      if ((mOp == MOP_wsbfxrri5i5 || mOp == MOP_xsbfxrri6i6) && srcRegOpnd.GetValidBitsNum() == width) {
        return false;
      }
      if (srcRegOpnd.IsSSAForm() && srcVersion != nullptr) {
        srcVersion->SetImplicitCvt();
      }
      if (mOp == MOP_wubfxrri5i5 || mOp == MOP_wsbfxrri5i5) {
        newMop = MOP_wmovrr;
      } else if (mOp == MOP_xubfxrri6i6 || mOp == MOP_xsbfxrri6i6) {
        newMop = MOP_xmovrr;
      }
      break;
    }
    default:
      return false;
  }
  newDstOpnd = &static_cast<RegOperand&>(dstOpnd);
  newSrcOpnd = &static_cast<RegOperand&>(srcOpnd);
  return true;
}

void ExtValidBitPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  MOperator mOp = insn.GetMachineOpcode();
  switch (mOp) {
    case MOP_xuxtb32:
    case MOP_xuxth32:
    case MOP_xuxtw64:
    case MOP_xsxtw64:
    case MOP_wubfxrri5i5:
    case MOP_xubfxrri6i6:
    case MOP_wsbfxrri5i5:
    case MOP_xsbfxrri6i6: {
      if (newDstOpnd != nullptr && newDstOpnd->IsSSAForm() && newSrcOpnd != nullptr && newSrcOpnd->IsSSAForm()) {
        destVersion = ssaInfo->FindSSAVersion(newDstOpnd->GetRegisterNumber());
        ASSERT(destVersion != nullptr, "find Version failed");
        srcVersion = ssaInfo->FindSSAVersion(newSrcOpnd->GetRegisterNumber());
        ASSERT(srcVersion != nullptr, "find Version failed");
        cgFunc->InsertExtendSet(srcVersion->GetSSAvRegOpnd()->GetRegisterNumber());
        ReplaceImplicitCvtAndProp(destVersion, srcVersion);
      } else if (newDstOpnd != nullptr && newSrcOpnd != nullptr) {
        // only change mop because there is no ssa version
        Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(newMop, *newDstOpnd, *newSrcOpnd);
        bb.ReplaceInsn(insn, newInsn);
        ssaInfo->ReplaceInsn(insn, newInsn);
        if (newDstOpnd->GetSize() > newSrcOpnd->GetSize() || newDstOpnd->GetSize() != newDstOpnd->GetValidBitsNum()) {
          ssaInfo->InsertSafePropInsn(newInsn.GetId());
        }
      }
      break;
    }
    default:
      return;
  }
}

bool CmpCsetVBPattern::IsContinuousCmpCset(const Insn &curInsn) const {
  auto &csetDstReg = static_cast<RegOperand&>(curInsn.GetOperand(kInsnFirstOpnd));
  CHECK_FATAL(csetDstReg.IsSSAForm(), "dstOpnd must be ssa form");
  VRegVersion *dstVersion = ssaInfo->FindSSAVersion(csetDstReg.GetRegisterNumber());
  ASSERT(dstVersion != nullptr, "find vRegVersion failed");
  for (auto useDUInfoIt : dstVersion->GetAllUseInsns()) {
    if (useDUInfoIt.second == nullptr) {
      continue;
    }
    Insn *useInsn = useDUInfoIt.second->GetInsn();
    if (useInsn == nullptr) {
      continue;
    }
    MOperator useMop = useInsn->GetMachineOpcode();
    if (useMop == MOP_wcmpri || useMop == MOP_xcmpri) {
      auto &ccDstReg = static_cast<RegOperand&>(useInsn->GetOperand(kInsnFirstOpnd));
      CHECK_FATAL(ccDstReg.IsSSAForm(), "dstOpnd must be ssa form");
      VRegVersion *ccDstVersion = ssaInfo->FindSSAVersion(ccDstReg.GetRegisterNumber());
      ASSERT(ccDstVersion != nullptr, "find vRegVersion failed");
      for (auto ccUseDUInfoIt : ccDstVersion->GetAllUseInsns()) {
        if (ccUseDUInfoIt.second == nullptr) {
          continue;
        }
        Insn *ccUseInsn = ccUseDUInfoIt.second->GetInsn();
        if (ccUseInsn == nullptr) {
          continue;
        }
        MOperator ccUseMop = ccUseInsn->GetMachineOpcode();
        if (ccUseMop == MOP_wcsetrc || ccUseMop == MOP_xcsetrc) {
          return true;
        }
      }
    }
  }
  return false;
}

bool CmpCsetVBPattern::OpndDefByOneValidBit(const Insn &defInsn) const {
  if (defInsn.IsPhi()) {
    return (static_cast<RegOperand&>(cmpInsn->GetOperand(kInsnSecondOpnd)).GetValidBitsNum() == k1BitSize) ||
           (static_cast<RegOperand&>(cmpInsn->GetOperand(kInsnSecondOpnd)).GetValidBitsNum() == k0BitSize);
  }
  MOperator defMop = defInsn.GetMachineOpcode();
  switch (defMop) {
    case MOP_wcsetrc:
    case MOP_xcsetrc:
      return true;
    case MOP_wmovri32:
    case MOP_xmovri64: {
      Operand &defOpnd = defInsn.GetOperand(kInsnSecondOpnd);
      ASSERT(defOpnd.IsIntImmediate(), "expects ImmOperand");
      auto &defConst = static_cast<ImmOperand&>(defOpnd);
      int64 defConstValue = defConst.GetValue();
      return (defConstValue == 0 || defConstValue == 1);
    }
    case MOP_xmovrr:
    case MOP_wmovrr:
      return IsZeroRegister(defInsn.GetOperand(kInsnSecondOpnd));
    case MOP_wlsrrri5:
    case MOP_xlsrrri6: {
      Operand &opnd2 = defInsn.GetOperand(kInsnThirdOpnd);
      ASSERT(opnd2.IsIntImmediate(), "expects ImmOperand");
      auto &opndImm = static_cast<ImmOperand&>(opnd2);
      int64 shiftBits = opndImm.GetValue();
      return ((defMop == MOP_wlsrrri5 && shiftBits == (k32BitSize - 1)) ||
              (defMop == MOP_xlsrrri6 && shiftBits == (k64BitSize - 1)));
    }
    default:
      return false;
  }
}

bool CmpCsetVBPattern::CheckCondition(Insn &csetInsn) {
  MOperator curMop = csetInsn.GetMachineOpcode();
  if (curMop != MOP_wcsetrc && curMop != MOP_xcsetrc) {
    return false;
  }
  /* combine [continuous cmp & cset] first, to eliminate more insns */
  if (IsContinuousCmpCset(csetInsn)) {
    return false;
  }
  RegOperand &ccReg = static_cast<RegOperand&>(csetInsn.GetOperand(kInsnThirdOpnd));
  regno_t ccRegNo = ccReg.GetRegisterNumber();
  cmpInsn = ssaInfo->GetDefInsn(ccReg);
  CHECK_NULL_FATAL(cmpInsn);
  MOperator mop = cmpInsn->GetMachineOpcode();
  if ((mop != MOP_wcmpri) && (mop != MOP_xcmpri)) {
    return false;
  }
  VRegVersion *ccRegVersion = ssaInfo->FindSSAVersion(ccRegNo);
  CHECK_NULL_FATAL(ccRegVersion);
  if (ccRegVersion->GetAllUseInsns().size() > k1BitSize) {
    return false;
  }
  Operand &cmpSecondOpnd = cmpInsn->GetOperand(kInsnThirdOpnd);
  CHECK_FATAL(cmpSecondOpnd.IsIntImmediate(), "expects ImmOperand");
  auto &cmpConst = static_cast<ImmOperand&>(cmpSecondOpnd);
  cmpConstVal = cmpConst.GetValue();
  /* get ImmOperand, must be 0 or 1 */
  if ((cmpConstVal != 0) && (cmpConstVal != k1BitSize)) {
    return false;
  }
  Operand &cmpFirstOpnd = cmpInsn->GetOperand(kInsnSecondOpnd);
  CHECK_FATAL(cmpFirstOpnd.IsRegister(), "cmpFirstOpnd must be register!");
  RegOperand &cmpReg = static_cast<RegOperand&>(cmpFirstOpnd);
  Insn *defInsn = ssaInfo->GetDefInsn(cmpReg);
  if (defInsn == nullptr) {
    return false;
  }
  if (defInsn->GetMachineOpcode() == MOP_wmovrr || defInsn->GetMachineOpcode() == MOP_xmovrr) {
    auto &srcOpnd = static_cast<RegOperand&>(defInsn->GetOperand(kInsnSecondOpnd));
    if (!srcOpnd.IsVirtualRegister()) {
      return false;
    }
  }
  return ((cmpReg.GetValidBitsNum() == k1BitSize) || (cmpReg.GetValidBitsNum() == k0BitSize) ||
          OpndDefByOneValidBit(*defInsn));
}

void CmpCsetVBPattern::Run(BB &bb, Insn &csetInsn) {
  if (!CheckCondition(csetInsn)) {
    return;
  }
  Operand &csetFirstOpnd = csetInsn.GetOperand(kInsnFirstOpnd);
  Operand &cmpFirstOpnd = cmpInsn->GetOperand(kInsnSecondOpnd);
  auto &cond = static_cast<CondOperand&>(csetInsn.GetOperand(kInsnSecondOpnd));
  Insn *newInsn = nullptr;

  /* cmpFirstOpnd == 1 */
  if ((cmpConstVal == 0 && cond.GetCode() == CC_NE) || (cmpConstVal == 1 && cond.GetCode() == CC_EQ)) {
    MOperator mopCode = (cmpFirstOpnd.GetSize() == k64BitSize) ? MOP_xmovrr : MOP_wmovrr;
    newInsn = &cgFunc->GetInsnBuilder()->BuildInsn(mopCode, csetFirstOpnd, cmpFirstOpnd);
  } else if ((cmpConstVal == 1 && cond.GetCode() == CC_NE) || (cmpConstVal == 0 && cond.GetCode() == CC_EQ)) {
    /* cmpFirstOpnd == 0 */
    MOperator mopCode = (cmpFirstOpnd.GetSize() == k64BitSize) ? MOP_xeorrri13 : MOP_weorrri12;
    ImmOperand &one = static_cast<AArch64CGFunc*>(cgFunc)->CreateImmOperand(1, k8BitSize, false);
    newInsn = &cgFunc->GetInsnBuilder()->BuildInsn(mopCode, csetFirstOpnd, cmpFirstOpnd, one);
  }
  if (newInsn == nullptr) {
    return;
  }
  bb.ReplaceInsn(csetInsn, *newInsn);
  ssaInfo->ReplaceInsn(csetInsn, *newInsn);
  if (CG_VALIDBIT_OPT_DUMP && (newInsn != nullptr)) {
    std::vector<Insn*> prevInsns;
    prevInsns.emplace_back(cmpInsn);
    prevInsns.emplace_back(&csetInsn);
    DumpAfterPattern(prevInsns, newInsn, nullptr);
  }
}

void CmpBranchesPattern::SelectNewMop(MOperator mop) {
  switch (mop) {
    case MOP_bge: {
      newMop = is64Bit ? MOP_xtbnz : MOP_wtbnz;
      break;
    }
    case MOP_blt: {
      newMop = is64Bit ? MOP_xtbz : MOP_wtbz;
      break;
    }
    case MOP_beq: {
      newMop = (static_cast<uint32>(newImmVal) == k0BitSize) ? (is64Bit ? MOP_xcbz : MOP_wcbz)
                                                             : (is64Bit ? MOP_xcbnz : MOP_wcbnz);
      break;
    }
    case MOP_bne: {
      newMop = (static_cast<uint32>(newImmVal) == k0BitSize) ? (is64Bit ? MOP_xcbnz : MOP_wcbnz)
                                                             : (is64Bit ? MOP_xcbz : MOP_wcbz);
      break;
    }
    default:
      break;
  }
}

bool CmpBranchesPattern::CheckCondition(Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  isEqOrNe = (curMop == MOP_beq) || (curMop == MOP_bne);
  if (!isEqOrNe && (curMop != MOP_bge) && (curMop != MOP_blt)) {
    return false;
  }
  auto &ccReg = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
  prevCmpInsn = ssaInfo->GetDefInsn(ccReg);
  if (prevCmpInsn == nullptr) {
    return false;
  }
  MOperator cmpMop = prevCmpInsn->GetMachineOpcode();
  if (cmpMop != MOP_wcmpri && cmpMop != MOP_xcmpri) {
    return false;
  }
  is64Bit = (cmpMop == MOP_xcmpri);
  auto &cmpUseOpnd = static_cast<RegOperand &>(prevCmpInsn->GetOperand(kInsnSecondOpnd));
  auto &cmpImmOpnd = static_cast<ImmOperand &>(prevCmpInsn->GetOperand(kInsnThirdOpnd));
  int64 cmpImmVal = cmpImmOpnd.GetValue();
  if (isEqOrNe) {
    newImmVal = cmpImmVal;
    if (cmpUseOpnd.GetValidBitsNum() > k1BitSize ||
        (cmpImmVal != static_cast<int64>(k0BitSize) && cmpImmVal != static_cast<int64>(k1BitSize))) {
      return false;
    }
  } else {
    newImmVal = ValidBitOpt::GetLogValueAtBase2(cmpImmVal);
    if (newImmVal < 0 || cmpUseOpnd.GetValidBitsNum() != (newImmVal + 1)) {
      return false;
    }
  }
  SelectNewMop(curMop);
  if (newMop == MOP_undef) {
    return false;
  }
  return true;
}

void CmpBranchesPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  auto *aarFunc = static_cast<AArch64CGFunc *>(cgFunc);
  auto &labelOpnd = static_cast<LabelOperand &>(insn.GetOperand(kInsnSecondOpnd));
  ImmOperand &newImmOpnd = aarFunc->CreateImmOperand(newImmVal, k8BitSize, false);
  Insn &newInsn = isEqOrNe
                      ? cgFunc->GetInsnBuilder()->BuildInsn(newMop, prevCmpInsn->GetOperand(kInsnSecondOpnd), labelOpnd)
                      : cgFunc->GetInsnBuilder()->BuildInsn(newMop, prevCmpInsn->GetOperand(kInsnSecondOpnd),
                                                            newImmOpnd, labelOpnd);
  bb.ReplaceInsn(insn, newInsn);
  /* update ssa info */
  ssaInfo->ReplaceInsn(insn, newInsn);
  /* dump pattern info */
  if (CG_VALIDBIT_OPT_DUMP) {
    std::vector<Insn *> prevs;
    prevs.emplace_back(prevCmpInsn);
    DumpAfterPattern(prevs, &insn, &newInsn);
  }
}

bool RSPattern::CheckCondition(Insn &insn) {
  if (!insn.GetOperand(kInsnSecondOpnd).IsRegister() && !insn.GetOperand(kInsnThirdOpnd).IsImmediate()) {
    return false;
  }
  RegOperand regOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  ImmOperand &oldImmOperand = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
  if (regOpnd.GetValidBitsNum() < oldImmOperand.GetValue()) {
    newMop = insn.GetMachineOpcode() == MOP_xasrrri6 ? MOP_xmovri64 : MOP_wmovri32;
    oldImmSize = oldImmOperand.GetSize();
    oldImmIsSigned = oldImmOperand.IsSignedValue();
    return true;
  }
  return false;
}

void RSPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  auto *aarFunc = static_cast<AArch64CGFunc *>(cgFunc);
  ImmOperand &newImmOpnd = aarFunc->CreateImmOperand(0, oldImmSize, oldImmIsSigned);
  Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(newMop, insn.GetOperand(kInsnFirstOpnd), newImmOpnd);
  bb.ReplaceInsn(insn, newInsn);
  // update ssa info
  ssaInfo->ReplaceInsn(insn, newInsn);
}

void RevCbzToCbzPattern::SetRevValue(MOperator mop, const uint64 &oldVal, uint64 &newVal, bool isLogicalImm16) const {
  if (mop == MOP_wrevrr16) {
    // 32bits mask for wrev16
    // 0x000000FFU, 0x0000FF00U
    newVal = (((oldVal & 0x000000FFU) << k8BitSize) | ((oldVal & 0x0000FF00U) >> k8BitSize));
    // if rev16 use point validbit is 16bit, construct a valid imm12 number as much as possible
    if (isLogicalImm16) {
      newVal |= (newVal << k16BitSize);
    }
  } else if (mop == MOP_wrevrr) {
    // 32bits mask for wrev
    // 0x000000FFU, 0x0000FF00U, 0x00FF0000U, 0xFF000000U
    newVal = (((oldVal & 0x000000FFU) << k24BitSize) | ((oldVal & 0x0000FF00U) << k8BitSize) |
              ((oldVal & 0x00FF0000U) >> k8BitSize)  | ((oldVal & 0xFF000000U) >> k24BitSize));
  } else if (mop == MOP_xrevrr) {
    // 64bits mask for xrev
    // 0x00000000000000FFULL, 0x000000000000FF00ULL, 0x0000000000FF0000ULL, 0x00000000FF000000ULL
    // 0x000000FF00000000ULL, 0x0000FF0000000000ULL, 0x00FF000000000000ULL, 0xFF00000000000000ULL
    newVal = (((oldVal & 0x00000000000000FFULL) << k56BitSize) |
              ((oldVal & 0x000000000000FF00ULL) << k40BitSize) |
              ((oldVal & 0x0000000000FF0000ULL) << k24BitSize) |
              ((oldVal & 0x00000000FF000000ULL) << k8BitSize)  |
              ((oldVal & 0x000000FF00000000ULL) >> k8BitSize)  |
              ((oldVal & 0x0000FF0000000000ULL) >> k24BitSize) |
              ((oldVal & 0x00FF000000000000ULL) >> k40BitSize) |
              ((oldVal & 0xFF00000000000000ULL) >> k56BitSize));
  }
}

bool RevCbzToCbzPattern::CheckCondition(Insn &insn) {
  auto *curReg = static_cast<RegOperand*>(&(insn.GetOperand(kInsnFirstOpnd)));
  VRegVersion *curVersion = ssaInfo->FindSSAVersion(curReg->GetRegisterNumber());
  ASSERT(curVersion != nullptr, "find Version failed");
  auto *useList = &(curVersion->GetAllUseInsns());

  while (useList->size() == k1BitSize) {
    Insn *nextInsn = useList->begin()->second->GetInsn();
    if (nextInsn == nullptr) {
      return false;
    }
    MOperator useMop = nextInsn->GetMachineOpcode();
    if ((useMop == MOP_wcbz || useMop == MOP_xcbz || useMop == MOP_wcbnz || useMop == MOP_xcbnz)) {
      cbzInsn = nextInsn;
      return true;
    }
    if ((useMop == MOP_wcmpri || useMop == MOP_xcmpri)) {
      curReg = static_cast<RegOperand*>(&(nextInsn->GetOperand(kInsnFirstOpnd)));
      curVersion = ssaInfo->FindSSAVersion(curReg->GetRegisterNumber());
      ASSERT(curVersion != nullptr, "find Version failed");
      useList = &(curVersion->GetAllUseInsns());
      if (useList->size() != k1BitSize) {
        return false;
      }
      Insn *condInsn = useList->begin()->second->GetInsn();
      if (condInsn == nullptr) {
        return false;
      }
      MOperator condMop = condInsn->GetMachineOpcode();
      if ((condMop != MOP_beq) && (condMop != MOP_bne)) {
        return false;
      }
      oldInsns.push_back(nextInsn);
      return true;
    }
    if (((useMop == MOP_weorrri12) || (useMop == MOP_xeorrri13) || (useMop == MOP_wandrri12) ||
        (useMop == MOP_xandrri13) || (useMop == MOP_wiorrri12) || (useMop == MOP_xiorrri13))) {
      oldInsns.push_back(nextInsn);
      curReg = static_cast<RegOperand*>(&(nextInsn->GetOperand(kInsnFirstOpnd)));
      curVersion = ssaInfo->FindSSAVersion(curReg->GetRegisterNumber());
      ASSERT(curVersion != nullptr, "find Version failed");
      useList = &(curVersion->GetAllUseInsns());
      continue;
    }
    break;
  }
  return false;
}

void RevCbzToCbzPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  auto &revUseReg = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  MOperator revMop = insn.GetMachineOpcode();
  bool isValidBits16 = false;
  if ((revMop == MOP_wrevrr16) && (revUseReg.GetValidBitsNum() <= k16BitSize)) {
    isValidBits16 = true;
  }

  for (size_t i = 0; i < oldInsns.size(); ++i) {
    auto *oldInsn = oldInsns[i];
    MOperator mop = oldInsn->GetMachineOpcode();
    auto &oldImmOpnd = static_cast<ImmOperand&>(oldInsn->GetOperand(kInsnThirdOpnd));
    uint64 oldValue = static_cast<uint64>(oldImmOpnd.GetValue());
    uint64 newValue = 0;
    uint32 immSize = kMaxImmVal12Bits;
    bool isLogicalImm = true;
    if ((mop == MOP_xcmpri) || (mop == MOP_wcmpri)) {
      isLogicalImm = false;
    }
    if ((revMop == MOP_xrevrr) && isLogicalImm) {
      immSize = kMaxImmVal13Bits;
    }
    SetRevValue(revMop, oldValue, newValue, (isLogicalImm && isValidBits16));
    auto *aarFunc = static_cast<AArch64CGFunc*>(cgFunc);
    ImmOperand &newImmOpnd = aarFunc->CreateImmOperand(static_cast<int64>(newValue), immSize, false);
    Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(mop, oldInsn->GetOperand(kInsnFirstOpnd),
                                                        oldInsn->GetOperand(kInsnSecondOpnd), newImmOpnd);
    if (i == 0) {
      newInsn.SetOperand(kInsnSecondOpnd, revUseReg);
    }
    if (!VERIFY_INSN(&newInsn)) {
      return;
    }
    newInsns.push_back(&newInsn);
  }

  if (oldInsns.size() == 0) {
    CHECK_FATAL(cbzInsn != nullptr, "cbzInsn can't be nullptr");
    Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(cbzInsn->GetMachineOpcode(),
                                                        revUseReg, cbzInsn->GetOperand(kInsnSecondOpnd));
    cbzInsn->GetBB()->ReplaceInsn(*cbzInsn, newInsn);
    ssaInfo->ReplaceInsn(*cbzInsn, newInsn);
  }

  for (size_t i = 0; i < oldInsns.size(); ++i) {
    oldInsns[i]->GetBB()->ReplaceInsn(*oldInsns[i], *newInsns[i]);
    ssaInfo->ReplaceInsn(*oldInsns[i], *newInsns[i]);
  }
}

} /* namespace maplebe */

