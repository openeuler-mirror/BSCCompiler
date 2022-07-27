/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "aarch64_peep.h"
#include "cg.h"
#include "mpl_logging.h"
#include "common_utils.h"
#include "cg_option.h"
#include "aarch64_utils.h"

namespace maplebe {
#define JAVALANG (cgFunc->GetMirModule().IsJavaModule())
#define CG_PEEP_DUMP CG_DEBUG_FUNC(*cgFunc)
namespace {
const std::string kMccLoadRef = "MCC_LoadRefField";
const std::string kMccLoadRefV = "MCC_LoadVolatileField";
const std::string kMccLoadRefS = "MCC_LoadRefStatic";
const std::string kMccLoadRefVS = "MCC_LoadVolatileStaticField";
const std::string kMccDummy = "MCC_Dummy";

const uint32 kSizeOfSextMopTable = 5;
const uint32 kSizeOfUextMopTable = 3;

MOperator sextMopTable[kSizeOfSextMopTable] = {
    MOP_xsxtb32, MOP_xsxtb64, MOP_xsxth32, MOP_xsxth64, MOP_xsxtw64
};

MOperator uextMopTable[kSizeOfUextMopTable] = {
    MOP_xuxtb32, MOP_xuxth32, MOP_xuxtw64
};

const std::string GetReadBarrierName(const Insn &insn) {
  constexpr int32 totalBarrierNamesNum = 5;
  std::array<std::string, totalBarrierNamesNum> barrierNames = {
    kMccLoadRef, kMccLoadRefV, kMccLoadRefS, kMccLoadRefVS, kMccDummy
  };
  if (insn.GetMachineOpcode() == MOP_xbl ||
      insn.GetMachineOpcode() == MOP_tail_call_opt_xbl) {
    auto &op = static_cast<FuncNameOperand&>(insn.GetOperand(kInsnFirstOpnd));
    const std::string &funcName = op.GetName();
    for (const std::string &singleBarrierName : barrierNames) {
      if (funcName == singleBarrierName) {
        return singleBarrierName;
      }
    }
  }
  return "";
}

MOperator GetLoadOperator(uint32 refSize, bool isVolatile) {
  if (refSize == k32BitSize) {
    return isVolatile ? MOP_wldar : MOP_wldr;
  }
  return isVolatile ? MOP_xldar : MOP_xldr;
}
}

static bool IsZeroRegister(const Operand &opnd) {
  if (!opnd.IsRegister()) {
    return false;
  }
  const RegOperand *regOpnd = static_cast<const RegOperand*>(&opnd);
  return regOpnd->GetRegisterNumber() == RZR;
}

void AArch64CGPeepHole::Run() {
  bool optSuccess = false;
  FOR_ALL_BB(bb, cgFunc) {
    FOR_BB_INSNS_SAFE(insn, bb, nextInsn) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      if (ssaInfo != nullptr) {
        optSuccess = DoSSAOptimize(*bb, *insn);
      } else {
        DoNormalOptimize(*bb, *insn);
      }
    }
  }
  if (optSuccess) {
    Run();
  }
}

bool AArch64CGPeepHole::DoSSAOptimize(BB &bb, Insn &insn) {
  MOperator thisMop = insn.GetMachineOpcode();
  manager = peepMemPool->New<PeepOptimizeManager>(*cgFunc, bb, insn, *ssaInfo);
  switch (thisMop) {
    case MOP_xandrrr:
    case MOP_wandrrr: {
      manager->Optimize<MvnAndToBicPattern>(true);
      break;
    }
    case MOP_wiorrri12:
    case MOP_xiorrri13: {
      manager->Optimize<OrrToMovPattern>(true);
      break;
    }
    case MOP_wcbz:
    case MOP_xcbz:
    case MOP_wcbnz:
    case MOP_xcbnz: {
      manager->Optimize<AndCbzToTbzPattern>(true);
      manager->Optimize<CsetCbzToBeqPattern>(true);
      manager->Optimize<OneHoleBranchPattern>(true);
      break;
    }
    case MOP_beq:
    case MOP_bne: {
      manager->Optimize<AndCmpBranchesToTbzPattern>(true);
      break;
    }
    case MOP_wcsetrc:
    case MOP_xcsetrc: {
      manager->Optimize<ContinuousCmpCsetPattern>(true);
      break;
    }
    case MOP_waddrrr:
    case MOP_xaddrrr:
    case MOP_dadd:
    case MOP_sadd:
    case MOP_wsubrrr:
    case MOP_xsubrrr:
    case MOP_dsub:
    case MOP_ssub:
    case MOP_xinegrr:
    case MOP_winegrr:
    case MOP_wfnegrr:
    case MOP_xfnegrr: {
      manager->Optimize<SimplifyMulArithmeticPattern>(true);
      break;
    }
    case MOP_wandrri12:
    case MOP_xandrri13: {
      manager->Optimize<LsrAndToUbfxPattern>(true);
      break;
    }
    case MOP_wcselrrrc:
    case MOP_xcselrrrc: {
      manager->Optimize<CselToCsetPattern>(true);
      break;
    }
    case MOP_wiorrrr:
    case MOP_xiorrrr:
    case MOP_wiorrrrs:
    case MOP_xiorrrrs: {
      manager->Optimize<LogicShiftAndOrrToExtrPattern>(true);
      break;
    }
    case MOP_bge:
    case MOP_ble:
    case MOP_blt:
    case MOP_bgt: {
      manager->Optimize<ZeroCmpBranchesToTbzPattern>(true);
      break;
    }
    case MOP_wcmprr:
    case MOP_xcmprr: {
      manager->Optimize<NegCmpToCmnPattern>(true);
      break;
    }
    case MOP_xlslrri6: {
      manager->Optimize<ExtLslToBitFieldInsertPattern>();
      manager->Optimize<CombineSameArithmeticPattern>(true);
      break;
    }
    case MOP_xsxtb32:
    case MOP_xsxtb64:
    case MOP_xsxth32:
    case MOP_xsxth64:
    case MOP_xsxtw64:
    case MOP_xuxtb32:
    case MOP_xuxth32:
    case MOP_xuxtw64: {
      manager->Optimize<ElimSpecificExtensionPattern>(true);
      break;
    }
    case MOP_wlsrrri5:
    case MOP_xlsrrri6:
    case MOP_wasrrri5:
    case MOP_xasrrri6:
    case MOP_wlslrri5:
    case MOP_waddrri12:
    case MOP_xaddrri12:
    case MOP_wsubrri12:
    case MOP_xsubrri12: {
      manager->Optimize<CombineSameArithmeticPattern>(true);
      break;
    }
    case MOP_wubfxrri5i5:
    case MOP_xubfxrri6i6: {
      manager->Optimize<UbfxAndCbzToTbzPattern>(true);
      break;
    }
    default:
      break;
  }
  return manager->OptSuccess();
}

bool ContinuousCmpCsetPattern::CheckCondCode(const CondOperand &condOpnd) const {
  switch (condOpnd.GetCode()) {
    case CC_NE:
    case CC_EQ:
    case CC_LT:
    case CC_GE:
    case CC_GT:
    case CC_LE:
      return true;
    default:
      return false;
  }
}

bool ContinuousCmpCsetPattern::CheckCondition(Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  if (curMop != MOP_wcsetrc && curMop != MOP_xcsetrc) {
    return false;
  }
  auto &condOpnd = static_cast<CondOperand&>(insn.GetOperand(kInsnSecondOpnd));
  if (condOpnd.GetCode() != CC_NE && condOpnd.GetCode() != CC_EQ) {
    return false;
  }
  reverse = (condOpnd.GetCode() == CC_EQ);
  auto &ccReg = static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd));
  prevCmpInsn = ssaInfo->GetDefInsn(ccReg);
  if (prevCmpInsn == nullptr) {
    return false;
  }
  MOperator prevCmpMop = prevCmpInsn->GetMachineOpcode();
  if (prevCmpMop != MOP_wcmpri && prevCmpMop != MOP_xcmpri) {
    return false;
  }
  if (!static_cast<ImmOperand&>(prevCmpInsn->GetOperand(kInsnThirdOpnd)).IsZero()) {
    return false;
  }
  auto &cmpCCReg = static_cast<RegOperand&>(prevCmpInsn->GetOperand(kInsnFirstOpnd));
  InsnSet useSet = GetAllUseInsn(cmpCCReg);
  if (useSet.size() > 1) {
    return false;
  }
  auto &cmpUseReg = static_cast<RegOperand&>(prevCmpInsn->GetOperand(kInsnSecondOpnd));
  prevCsetInsn1 = ssaInfo->GetDefInsn(cmpUseReg);
  if (prevCsetInsn1 == nullptr) {
    return false;
  }
  MOperator prevCsetMop1 = prevCsetInsn1->GetMachineOpcode();
  if (prevCsetMop1 != MOP_wcsetrc && prevCsetMop1 != MOP_xcsetrc) {
    return false;
  }
  auto &condOpnd1 = static_cast<CondOperand&>(prevCsetInsn1->GetOperand(kInsnSecondOpnd));
  if (!CheckCondCode(condOpnd1)) {
    return false;
  }
  auto &ccReg1 = static_cast<RegOperand&>(prevCsetInsn1->GetOperand(kInsnThirdOpnd));
  prevCmpInsn1 = ssaInfo->GetDefInsn(ccReg1);
  if (prevCmpInsn1 == nullptr) {
    return false;
  }
  if (IsCCRegCrossVersion(*prevCsetInsn1, *prevCmpInsn, ccReg1)) {
    return false;
  }
  return true;
}

AArch64CC_t ContinuousCmpCsetPattern::GetReverseCondCode(const CondOperand &condOpnd) const {
  switch (condOpnd.GetCode()) {
    case CC_NE:
      return CC_EQ;
    case CC_EQ:
      return CC_NE;
    case CC_LT:
      return CC_GE;
    case CC_GE:
      return CC_LT;
    case CC_GT:
      return CC_LE;
    case CC_LE:
      return CC_GT;
    default:
      CHECK_FATAL(false, "Not support yet.");
  }
  return kCcLast;
}

void ContinuousCmpCsetPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  auto *aarFunc = static_cast<AArch64CGFunc*>(cgFunc);
  MOperator curMop = insn.GetMachineOpcode();
  Operand &resOpnd = insn.GetOperand(kInsnFirstOpnd);
  Insn *newCsetInsn = nullptr;
  if (reverse) {
    MOperator prevCsetMop = prevCsetInsn1->GetMachineOpcode();
    auto &prevCsetCondOpnd = static_cast<CondOperand&>(prevCsetInsn1->GetOperand(kInsnSecondOpnd));
    CondOperand &newCondOpnd = aarFunc->GetCondOperand(GetReverseCondCode(prevCsetCondOpnd));
    regno_t tmpRegNO = 0;
    auto *tmpDefOpnd = aarFunc->CreateVirtualRegisterOperand(tmpRegNO,
        resOpnd.GetSize(), static_cast<RegOperand&>(resOpnd).GetRegisterType());
    tmpDefOpnd->SetValidBitsNum(k1BitSize);
    newCsetInsn = &cgFunc->GetCG()->BuildInstruction<AArch64Insn>(
        prevCsetMop, *tmpDefOpnd, newCondOpnd, prevCsetInsn1->GetOperand(kInsnThirdOpnd));
    BB *prevCsetBB = prevCsetInsn1->GetBB();
    (void)prevCsetBB->InsertInsnAfter(*prevCsetInsn1, *newCsetInsn);
    /* update ssa info */
    auto *a64SSAInfo = static_cast<AArch64CGSSAInfo*>(ssaInfo);
    a64SSAInfo->CreateNewInsnSSAInfo(*newCsetInsn);
    /* dump pattern info */
    if (CG_PEEP_DUMP) {
      std::vector<Insn*> prevs;
      prevs.emplace_back(prevCmpInsn1);
      prevs.emplace_back(&insn);
      DumpAfterPattern(prevs, prevCmpInsn, newCsetInsn);
    }
  }
  MOperator newMop = (curMop == MOP_wcsetrc) ? MOP_wmovrr : MOP_xmovrr;
  Insn *newInsn = nullptr;
  if (newCsetInsn == nullptr) {
    newInsn = &cgFunc->GetCG()->BuildInstruction<AArch64Insn>(newMop, insn.GetOperand(kInsnFirstOpnd),
                                                              prevCsetInsn1->GetOperand(kInsnFirstOpnd));
  } else {
    newInsn = &cgFunc->GetCG()->BuildInstruction<AArch64Insn>(newMop, insn.GetOperand(kInsnFirstOpnd),
                                                              newCsetInsn->GetOperand(kInsnFirstOpnd));
  }
  bb.ReplaceInsn(insn, *newInsn);
  /* update ssa info */
  ssaInfo->ReplaceInsn(insn, *newInsn);
  optSuccess = true;
  SetCurrInsn(newInsn);
  /* dump pattern info */
  if (CG_PEEP_DUMP) {
    std::vector<Insn*> prevs;
    prevs.emplace_back(prevCmpInsn1);
    prevs.emplace_back(prevCsetInsn1);
    if (newCsetInsn == nullptr) {
      (void)prevs.emplace_back(prevCmpInsn);
    } else {
      (void)prevs.emplace_back(newCsetInsn);
    }
    DumpAfterPattern(prevs, &insn, newInsn);
  }
}

bool NegCmpToCmnPattern::CheckCondition(Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  if (curMop != MOP_wcmprr && curMop != MOP_xcmprr) {
    return false;
  }
  auto &useReg = static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd));
  prevInsn = ssaInfo->GetDefInsn(useReg);
  if (prevInsn == nullptr) {
    return false;
  }
  MOperator prevMop = prevInsn->GetMachineOpcode();
  if (prevMop != MOP_winegrr && prevMop != MOP_xinegrr &&
      prevMop != MOP_winegrrs && prevMop != MOP_xinegrrs) {
    return false;
  }
  // Determine whether implicit conversion is existed.
  if ((prevMop == MOP_winegrr && curMop == MOP_xcmprr) || (prevMop == MOP_winegrrs && curMop == MOP_xcmprr) ||
      (prevMop == MOP_xinegrr && curMop == MOP_wcmprr) || (prevMop == MOP_winegrrs && curMop == MOP_xcmprr)) {
    return false;
  }
  auto &ccReg = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  InsnSet useInsns = GetAllUseInsn(ccReg);
  for (auto *useInsn : useInsns) {
    if (useInsn == nullptr) {
      continue;
    }
    MOperator useMop = useInsn->GetMachineOpcode();
    if (useMop == MOP_bhi || useMop == MOP_bls) {
      return false;
    }
    bool findUnsignedCond = false;
    for (uint32 i = 0; i < useInsn->GetOperandSize(); ++i) {
      if (useInsn->GetOperand(i).GetKind() == Operand::kOpdCond) {
        AArch64CC_t cond = static_cast<CondOperand&>(useInsn->GetOperand(i)).GetCode();
        /* in case of ignoring v flag
         *  adds xt, x0, x1 (0x8000000000000000) -> not set v
         *  ==>
         *  neg x1 x1 (0x8000000000000000) which is same for negative 0
         *  subs xt, x0, x1 () -> set v
         */
        if (cond == CC_HI || cond == CC_LS || cond == CC_GE || cond == CC_GT ||
            cond == CC_LE || cond == CC_LT) {
          findUnsignedCond = true;
          break;
        }
      }
    }
    if (findUnsignedCond) {
      return false;
    }
  }
  return true;
}

void NegCmpToCmnPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  Operand &opnd1 = insn.GetOperand(kInsnSecondOpnd);
  Operand &opnd2 = prevInsn->GetOperand(kInsnSecondOpnd);
  auto &ccReg = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  MOperator prevMop = prevInsn->GetMachineOpcode();
  MOperator currMop = insn.GetMachineOpcode();
  Insn *newInsn = nullptr;
  if (prevMop == MOP_winegrr || prevMop == MOP_xinegrr) {
    MOperator newMop = (currMop == MOP_wcmprr) ? MOP_wcmnrr : MOP_xcmnrr;
    newInsn = &(cgFunc->GetCG()->BuildInstruction<AArch64Insn>(newMop, ccReg, opnd1, opnd2));
  } else {
    /* prevMop == MOP_winegrrs || prevMop == MOP_xinegrrs */
    MOperator newMop = (currMop == MOP_wcmprr) ? MOP_wcmnrrs : MOP_xcmnrrs;
    Operand &shiftOpnd = prevInsn->GetOperand(kInsnThirdOpnd);
    newInsn = &(cgFunc->GetCG()->BuildInstruction<AArch64Insn>(newMop, ccReg, opnd1, opnd2, shiftOpnd));
  }
  CHECK_FATAL(newInsn != nullptr, "must create newInsn");
  bb.ReplaceInsn(insn, *newInsn);
  /* update ssa info */
  ssaInfo->ReplaceInsn(insn, *newInsn);
  optSuccess = true;
  SetCurrInsn(newInsn);
  /* dump pattern info */
  if (CG_PEEP_DUMP) {
    std::vector<Insn*> prevs;
    prevs.emplace_back(prevInsn);
    DumpAfterPattern(prevs, &insn, newInsn);
  }
}

void LdrCmpPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  bb.RemoveInsn(*ldr2);
  bb.RemoveInsn(*ldr1);
  bb.RemoveInsn(insn);
  bb.RemoveInsn(*bne1);
  prevLdr1->SetMOP(MOP_xldr);
  prevLdr2->SetMOP(MOP_xldr);
  prevCmp->SetMOP(MOP_xcmprr);
}

bool LdrCmpPattern::CheckCondition(Insn &insn) {
  if (currInsn != &insn) {
    return false;
  }
  if (!SetInsns()) {
    return false;
  }
  if (!CheckInsns()) {
    return false;
  }
  auto &reg0 = static_cast<RegOperand&>(currInsn->GetOperand(kInsnSecondOpnd));
  auto &reg1 = static_cast<RegOperand&>(currInsn->GetOperand(kInsnThirdOpnd));
  return !(IfOperandIsLiveAfterInsn(reg0, insn) || IfOperandIsLiveAfterInsn(reg1, insn));
}

/*
 * mopSeq:
 * ldr,ldr,cmp,bne
 */
bool LdrCmpPattern::SetInsns() {
  if (!IsLdr(currInsn->GetPreviousMachineInsn())) {
    return false;
  }
  ldr2 = currInsn->GetPreviousMachineInsn();
  if (!IsLdr(ldr2->GetPreviousMachineInsn())) {
    return false;
  }
  ldr1 = ldr2->GetPreviousMachineInsn();
  /* ldr1 must be firstInsn in currBB */
  if (currInsn->GetBB()->GetFirstInsn() != ldr1) {
    return false;
  }
  if (!IsBne(currInsn->GetNextMachineInsn())) {
    return false;
  }
  bne1 = currInsn->GetNextMachineInsn();
  BB *prevBB = currInsn->GetBB()->GetPrev();
  /* single prev, single pred */
  const MapleList<BB*> &predBBs = currInsn->GetBB()->GetPreds();
  if ((prevBB == nullptr) || (predBBs.size() != 1) || (prevBB != *predBBs.begin())) {
    return false;
  }
  if (!IsBne(prevBB->GetLastInsn())) {
    return false;
  }
  bne2 = prevBB->GetLastInsn();
  if (!IsCmp(bne2->GetPreviousMachineInsn())) {
    return false;
  }
  prevCmp = bne2->GetPreviousMachineInsn();
  if (!IsLdr(prevCmp->GetPreviousMachineInsn())) {
    return false;
  }
  prevLdr2 = prevCmp->GetPreviousMachineInsn();
  if (!IsLdr(prevLdr2->GetPreviousMachineInsn())) {
    return false;
  }
  prevLdr1 = prevLdr2->GetPreviousMachineInsn();
  return true;
}

bool LdrCmpPattern::CheckInsns() {
  auto &label1 = static_cast<LabelOperand&>(bne1->GetOperand(kInsnSecondOpnd));
  auto &label2 = static_cast<LabelOperand&>(bne2->GetOperand(kInsnSecondOpnd));
  if (label1.GetLabelIndex() != label2.GetLabelIndex()) {
    return false;
  }
  auto &reg0 = static_cast<RegOperand&>(currInsn->GetOperand(kInsnSecondOpnd));
  auto &reg1 = static_cast<RegOperand&>(currInsn->GetOperand(kInsnThirdOpnd));
  regno_t regno0 = reg0.GetRegisterNumber();
  regno_t regno1 = reg1.GetRegisterNumber();
  if (regno0 == regno1) {
    return false;
  }
  auto &mem1 = static_cast<MemOperand&>(ldr1->GetOperand(kInsnSecondOpnd));
  auto &preMem1 = static_cast<MemOperand&>(prevLdr1->GetOperand(kInsnSecondOpnd));
  auto &mem2 = static_cast<MemOperand&>(ldr2->GetOperand(kInsnSecondOpnd));
  auto &preMem2 = static_cast<MemOperand&>(prevLdr2->GetOperand(kInsnSecondOpnd));
  regno_t regnoBase0 = mem1.GetBaseRegister()->GetRegisterNumber();
  regno_t regnoBase1 = mem2.GetBaseRegister()->GetRegisterNumber();
  if (regnoBase0 == regnoBase1) {
    return false;
  }
  if ((regno0 == regnoBase0) || (regno0 == regnoBase1) || (regno1 == regnoBase0) || (regno1 == regnoBase1)) {
    return false;
  }
  if ((reg0 == static_cast<RegOperand&>(ldr2->GetOperand(kInsnFirstOpnd))) &&
      (reg0 == static_cast<RegOperand&>(prevLdr2->GetOperand(kInsnFirstOpnd))) &&
      (reg1 == static_cast<RegOperand&>(ldr1->GetOperand(kInsnFirstOpnd))) &&
      (reg1 == static_cast<RegOperand&>(prevLdr1->GetOperand(kInsnFirstOpnd)))) {
    if (MemOffet4Bit(preMem2, mem2) && MemOffet4Bit(preMem1, mem1)) {
      return true;
    }
  }
  if ((reg0 == static_cast<RegOperand&>(ldr1->GetOperand(kInsnFirstOpnd))) &&
      (reg0 == static_cast<RegOperand&>(prevLdr1->GetOperand(kInsnFirstOpnd))) &&
      (reg1 == static_cast<RegOperand&>(ldr2->GetOperand(kInsnFirstOpnd))) &&
      (reg1 == static_cast<RegOperand&>(prevLdr2->GetOperand(kInsnFirstOpnd)))) {
    if (MemOffet4Bit(preMem2, mem2) && MemOffet4Bit(preMem1, mem1)) {
      return true;
    }
  }
  return false;
}

bool LdrCmpPattern::MemOffet4Bit(const MemOperand &m1, const MemOperand &m2) const {
  if (m1.GetAddrMode() != m2.GetAddrMode()) {
    return false;
  }
  if (m1.GetAddrMode() != MemOperand::kAddrModeBOi) {
    return false;
  }
  if (m1.GetBaseRegister()->GetRegisterNumber() != m2.GetBaseRegister()->GetRegisterNumber()) {
    return false;
  }
  int64 offset = m2.GetOffsetOperand()->GetValue() - m1.GetOffsetOperand()->GetValue();
  return offset == k4BitSizeInt;
}

bool CsetCbzToBeqPattern::CheckCondition(Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  if (curMop != MOP_wcbz && curMop != MOP_xcbz && curMop != MOP_wcbnz && curMop != MOP_xcbnz) {
    return false;
  }
  auto &useReg = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  prevInsn = ssaInfo->GetDefInsn(useReg);
  if (prevInsn == nullptr) {
    return false;
  }
  MOperator prevMop = prevInsn->GetMachineOpcode();
  if (prevMop != MOP_wcsetrc && prevMop != MOP_xcsetrc) {
    return false;
  }
  auto &ccReg = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnThirdOpnd));
  if (IsCCRegCrossVersion(*prevInsn, insn, ccReg)) {
    return false;
  }
  return true;
}

MOperator CsetCbzToBeqPattern::SelectNewMop(AArch64CC_t condCode, bool inverse) const {
  switch (condCode) {
    case CC_NE:
      return inverse ? MOP_beq : MOP_bne;
    case CC_EQ:
      return inverse ? MOP_bne : MOP_beq;
    case CC_MI:
      return inverse ? MOP_bpl : MOP_bmi;
    case CC_PL:
      return inverse ? MOP_bmi : MOP_bpl;
    case CC_VS:
      return inverse ? MOP_bvc : MOP_bvs;
    case CC_VC:
      return inverse ? MOP_bvs : MOP_bvc;
    case CC_HI:
      return inverse ? MOP_bls : MOP_bhi;
    case CC_LS:
      return inverse ? MOP_bhi : MOP_bls;
    case CC_GE:
      return inverse ? MOP_blt : MOP_bge;
    case CC_LT:
      return inverse ? MOP_bge : MOP_blt;
    case CC_HS:
      return inverse ? MOP_blo : MOP_bhs;
    case CC_LO:
      return inverse ? MOP_bhs : MOP_blo;
    case CC_LE:
      return inverse ? MOP_bgt : MOP_ble;
    case CC_GT:
      return inverse ? MOP_ble : MOP_bgt;
    case CC_CS:
      return inverse ? MOP_bcc : MOP_bcs;
    default:
      return MOP_undef;
  }
}

void CsetCbzToBeqPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  MOperator curMop = insn.GetMachineOpcode();
  bool reverse = (curMop == MOP_wcbz || curMop == MOP_xcbz);
  auto &labelOpnd = static_cast<LabelOperand&>(insn.GetOperand(kInsnSecondOpnd));
  auto &condOpnd = static_cast<CondOperand&>(prevInsn->GetOperand(kInsnSecondOpnd));
  MOperator newMop = SelectNewMop(condOpnd.GetCode(), reverse);
  ASSERT(newMop != MOP_undef, "unknown condition code");
  Insn &newInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(newMop, prevInsn->GetOperand(kInsnThirdOpnd),
                                                                 labelOpnd);
  bb.ReplaceInsn(insn, newInsn);
  /* update ssa info */
  ssaInfo->ReplaceInsn(insn, newInsn);
  optSuccess = true;
  SetCurrInsn(&newInsn);
  /* dump pattern info */
  if (CG_PEEP_DUMP) {
    std::vector<Insn*> prevs;
    prevs.emplace_back(prevInsn);
    DumpAfterPattern(prevs, &insn, &newInsn);
  }
}

bool ExtLslToBitFieldInsertPattern::CheckCondition(Insn &insn) {
  auto &useReg = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  prevInsn = ssaInfo->GetDefInsn(useReg);
  if (prevInsn == nullptr) {
    return false;
  }
  MOperator prevMop = prevInsn->GetMachineOpcode();
  if (prevMop != MOP_xsxtw64 && prevMop != MOP_xuxtw64) {
    return false;
  }
  auto &immOpnd = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
  if (immOpnd.GetValue() > k32BitSize) {
    return false;
  }
  return true;
}

void ExtLslToBitFieldInsertPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  auto &prevSrcReg = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnSecondOpnd));
  cgFunc->InsertExtendSet(prevSrcReg.GetRegisterNumber());
  MOperator newMop = (prevInsn->GetMachineOpcode() == MOP_xsxtw64) ? MOP_xsbfizrri6i6 : MOP_xubfizrri6i6;
  auto *aarFunc = static_cast<AArch64CGFunc*>(cgFunc);
  auto &newImmOpnd1 = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
  ImmOperand &newImmOpnd2 = aarFunc->CreateImmOperand(k32BitSize, k6BitSize, false);
  Insn &newInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(newMop, insn.GetOperand(kInsnFirstOpnd),
                                                                 prevSrcReg, newImmOpnd1, newImmOpnd2);
  bb.ReplaceInsn(insn, newInsn);
  /* update ssa info */
  ssaInfo->ReplaceInsn(insn, newInsn);
  optSuccess = true;
  /* dump pattern info */
  if (CG_PEEP_DUMP) {
    std::vector<Insn*> prevs;
    prevs.emplace_back(prevInsn);
    DumpAfterPattern(prevs, &insn, &newInsn);
  }
}

bool CselToCsetPattern::IsOpndDefByZero(const Insn &insn) const {
  MOperator movMop = insn.GetMachineOpcode();
  switch (movMop) {
    case MOP_xmovrr:
    case MOP_wmovrr: {
      return IsZeroRegister(insn.GetOperand(kInsnSecondOpnd));
    }
    case MOP_xmovri32:
    case MOP_xmovri64: {
      auto &immOpnd = static_cast<ImmOperand&>(insn.GetOperand(kInsnSecondOpnd));
      return immOpnd.GetValue() == 0;
    }
    default:
      return false;
  }
}

bool CselToCsetPattern::IsOpndDefByOne(const Insn &insn) const {
  MOperator movMop = insn.GetMachineOpcode();
  if ((movMop != MOP_xmovri32) && (movMop != MOP_xmovri64)) {
    return false;
  }
  auto &immOpnd = static_cast<ImmOperand&>(insn.GetOperand(kInsnSecondOpnd));
  return immOpnd.GetValue() == 1;
}

bool CselToCsetPattern::CheckCondition(Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  if (curMop != MOP_wcselrrrc && curMop != MOP_xcselrrrc) {
    return false;
  }
  auto &useOpnd1 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  prevMovInsn1 = ssaInfo->GetDefInsn(useOpnd1);
  if (prevMovInsn1 == nullptr) {
    return false;
  }
  MOperator prevMop1 = prevMovInsn1->GetMachineOpcode();
  if (prevMop1 != MOP_xmovri32 && prevMop1 != MOP_xmovri64 &&
      prevMop1 != MOP_wmovrr && prevMop1 != MOP_xmovrr) {
    return false;
  }
  auto &useOpnd2 = static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd));
  prevMovInsn2 = ssaInfo->GetDefInsn(useOpnd2);
  if (prevMovInsn2 == nullptr) {
    return false;
  }
  MOperator prevMop2 = prevMovInsn2->GetMachineOpcode();
  if (prevMop2 != MOP_xmovri32 && prevMop2 != MOP_xmovri64 &&
      prevMop2 != MOP_wmovrr && prevMop2 != MOP_xmovrr) {
    return false;
  }
  return true;
}

AArch64CC_t CselToCsetPattern::GetInversedCondCode(const CondOperand &condOpnd) const {
  switch (condOpnd.GetCode()) {
    case CC_NE:
      return CC_EQ;
    case CC_EQ:
      return CC_NE;
    case CC_HS:
      return CC_LO;
    case CC_LO:
      return CC_HS;
    case CC_MI:
      return CC_PL;
    case CC_PL:
      return CC_MI;
    case CC_VS:
      return CC_VC;
    case CC_VC:
      return CC_VS;
    case CC_HI:
      return CC_LS;
    case CC_LS:
      return CC_HI;
    case CC_LT:
      return CC_GE;
    case CC_GE:
      return CC_LT;
    case CC_GT:
      return CC_LE;
    case CC_LE:
      return CC_GT;
    default:
      CHECK_FATAL(false, "Not support yet.");
  }
  return kCcLast;
}

void CselToCsetPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  Operand &dstOpnd = insn.GetOperand(kInsnFirstOpnd);
  MOperator newMop = (dstOpnd.GetSize() == k64BitSize ? MOP_xcsetrc : MOP_wcsetrc);
  Operand &condOpnd = insn.GetOperand(kInsnFourthOpnd);
  Operand &rflag = insn.GetOperand(kInsnFifthOpnd);
  Insn *newInsn = nullptr;
  if (IsOpndDefByOne(*prevMovInsn1) && IsOpndDefByZero(*prevMovInsn2)) {
    newInsn = &(cgFunc->GetCG()->BuildInstruction<AArch64Insn>(newMop, dstOpnd, condOpnd, rflag));
  } else if (IsOpndDefByZero(*prevMovInsn1) && IsOpndDefByOne(*prevMovInsn2)) {
    auto &origCondOpnd = static_cast<CondOperand&>(condOpnd);
    AArch64CC_t inverseCondCode = GetInversedCondCode(origCondOpnd);
    if (inverseCondCode == kCcLast) {
      return;
    }
    auto *aarFunc = static_cast<AArch64CGFunc*>(cgFunc);
    CondOperand &inverseCondOpnd = aarFunc->GetCondOperand(inverseCondCode);
    newInsn = &(cgFunc->GetCG()->BuildInstruction<AArch64Insn>(newMop, dstOpnd, inverseCondOpnd, rflag));
  }
  if (newInsn == nullptr) {
    return;
  }
  bb.ReplaceInsn(insn, *newInsn);
  /* update ssa info */
  ssaInfo->ReplaceInsn(insn, *newInsn);
  optSuccess = true;
  SetCurrInsn(newInsn);
  /* dump pattern info */
  if (CG_PEEP_DUMP) {
    std::vector<Insn*> prevs;
    prevs.emplace_back(prevMovInsn1);
    prevs.emplace_back(prevMovInsn2);
    DumpAfterPattern(prevs, &insn, newInsn);
  }
}

bool AndCmpBranchesToTbzPattern::CheckAndSelectPattern(const Insn &currInsn) {
  MOperator curMop = currInsn.GetMachineOpcode();
  MOperator prevAndMop = prevAndInsn->GetMachineOpcode();
  auto &andImmOpnd = static_cast<ImmOperand&>(prevAndInsn->GetOperand(kInsnThirdOpnd));
  auto &cmpImmOpnd = static_cast<ImmOperand&>(prevCmpInsn->GetOperand(kInsnThirdOpnd));
  if (cmpImmOpnd.GetValue() == 0) {
    tbzImmVal = GetLogValueAtBase2(andImmOpnd.GetValue());
    if (tbzImmVal < 0) {
      return false;
    }
    switch (curMop) {
      case MOP_beq:
        newMop = (prevAndMop == MOP_wandrri12) ?  MOP_wtbz : MOP_xtbz;
        break;
      case MOP_bne:
        newMop = (prevAndMop == MOP_wandrri12) ? MOP_wtbnz : MOP_xtbnz;
        break;
      default:
        return false;
    }
  } else {
    tbzImmVal = GetLogValueAtBase2(andImmOpnd.GetValue());
    int64 tmpVal = GetLogValueAtBase2(cmpImmOpnd.GetValue());
    if (tbzImmVal < 0 || tmpVal < 0 || tbzImmVal != tmpVal) {
      return false;
    }
    switch (curMop) {
      case MOP_beq:
        newMop = (prevAndMop == MOP_wandrri12) ?  MOP_wtbnz : MOP_xtbnz;
        break;
      case MOP_bne:
        newMop = (prevAndMop == MOP_wandrri12) ? MOP_wtbz : MOP_xtbz;
        break;
      default:
        return false;
    }
  }
  return true;
}

bool AndCmpBranchesToTbzPattern::CheckCondition(Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  if (curMop != MOP_beq && curMop != MOP_bne) {
    return false;
  }
  auto &ccReg = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  prevCmpInsn = ssaInfo->GetDefInsn(ccReg);
  if (prevCmpInsn == nullptr) {
    return false;
  }
  MOperator prevCmpMop = prevCmpInsn->GetMachineOpcode();
  if (prevCmpMop != MOP_wcmpri && prevCmpMop != MOP_xcmpri) {
    return false;
  }
  auto &cmpUseReg = static_cast<RegOperand&>(prevCmpInsn->GetOperand(kInsnSecondOpnd));
  prevAndInsn = ssaInfo->GetDefInsn(cmpUseReg);
  if (prevAndInsn == nullptr) {
    return false;
  }
  MOperator prevAndMop = prevAndInsn->GetMachineOpcode();
  if (prevAndMop != MOP_wandrri12 && prevAndMop != MOP_xandrri13) {
    return false;
  }
  CHECK_FATAL(prevAndInsn->GetOperand(kInsnFirstOpnd).GetSize() ==
              prevCmpInsn->GetOperand(kInsnSecondOpnd).GetSize(), "def-use reg size must be same based-on ssa");
  if (!CheckAndSelectPattern(insn)) {
    return false;
  }
  return true;
}

void AndCmpBranchesToTbzPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  auto *aarFunc = static_cast<AArch64CGFunc*>(cgFunc);
  auto &labelOpnd = static_cast<LabelOperand&>(insn.GetOperand(kInsnSecondOpnd));
  ImmOperand &tbzImmOpnd = aarFunc->CreateImmOperand(tbzImmVal, k8BitSize, false);
  Insn &newInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(newMop, prevAndInsn->GetOperand(kInsnSecondOpnd),
                                                                 tbzImmOpnd, labelOpnd);
  bb.ReplaceInsn(insn, newInsn);
  /* update ssa info */
  ssaInfo->ReplaceInsn(insn, newInsn);
  optSuccess = true;
  SetCurrInsn(&newInsn);
  /* dump pattern info */
  if (CG_PEEP_DUMP) {
    std::vector<Insn*> prevs;
    prevs.emplace_back(prevAndInsn);
    prevs.emplace_back(prevCmpInsn);
    DumpAfterPattern(prevs, &insn, &newInsn);
  }
}

bool ZeroCmpBranchesToTbzPattern::CheckAndSelectPattern(const Insn &currInsn) {
  MOperator currMop = currInsn.GetMachineOpcode();
  MOperator prevMop = prevInsn->GetMachineOpcode();
  switch (prevMop) {
    case MOP_wcmpri:
    case MOP_xcmpri: {
      regOpnd = &static_cast<RegOperand&>(prevInsn->GetOperand(kInsnSecondOpnd));
      auto &immOpnd = static_cast<ImmOperand&>(prevInsn->GetOperand(kInsnThirdOpnd));
      if (immOpnd.GetValue() != 0) {
        return false;
      }
      switch (currMop) {
        case MOP_bge:
          newMop = (regOpnd->GetSize() <= k32BitSize) ? MOP_wtbz : MOP_xtbz;
          break;
        case MOP_blt:
          newMop = (regOpnd->GetSize() <= k32BitSize) ? MOP_wtbnz : MOP_xtbnz;
          break;
        default:
          return false;
      }
      break;
    }
    case MOP_wcmprr:
    case MOP_xcmprr: {
      auto &regOpnd0 = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnSecondOpnd));
      auto &regOpnd1 = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnThirdOpnd));
      if (!IsZeroRegister(regOpnd0) && !IsZeroRegister(regOpnd1)) {
        return false;
      }
      switch (currMop) {
        case MOP_bge:
          if (IsZeroRegister(regOpnd1)) {
            regOpnd = &static_cast<RegOperand&>(prevInsn->GetOperand(kInsnSecondOpnd));
            newMop = (regOpnd->GetSize() <= k32BitSize) ? MOP_wtbz : MOP_xtbz;
          } else {
            return false;
          }
          break;
        case MOP_ble:
          if (IsZeroRegister(regOpnd0)) {
            regOpnd = &static_cast<RegOperand&>(prevInsn->GetOperand(kInsnThirdOpnd));
            newMop = (regOpnd->GetSize() <= k32BitSize) ? MOP_wtbz : MOP_xtbz;
          } else {
            return false;
          }
          break;
        case MOP_blt:
          if (IsZeroRegister(regOpnd1)) {
            regOpnd = &static_cast<RegOperand&>(prevInsn->GetOperand(kInsnSecondOpnd));
            newMop = (regOpnd->GetSize() <= k32BitSize) ? MOP_wtbnz : MOP_xtbnz;
          } else {
            return false;
          }
          break;
        case MOP_bgt:
          if (IsZeroRegister(regOpnd0)) {
            regOpnd = &static_cast<RegOperand&>(prevInsn->GetOperand(kInsnThirdOpnd));
            newMop = (regOpnd->GetSize() <= k32BitSize) ? MOP_wtbnz : MOP_xtbnz;
          } else {
            return false;
          }
          break;
        default:
          return false;
      }
      break;
    }
    default:
      return false;
  }
  return true;
}

bool ZeroCmpBranchesToTbzPattern::CheckCondition(Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  if (curMop != MOP_bge && curMop != MOP_ble && curMop != MOP_blt && curMop != MOP_bgt) {
    return false;
  }
  CHECK_FATAL(insn.GetOperand(kInsnSecondOpnd).IsLabel(), "must be labelOpnd");
  auto &ccReg = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  prevInsn = ssaInfo->GetDefInsn(ccReg);
  if (prevInsn == nullptr) {
    return false;
  }
  MOperator prevMop = prevInsn->GetMachineOpcode();
  if (prevMop != MOP_wcmpri && prevMop != MOP_xcmpri && prevMop != MOP_wcmprr && prevMop != MOP_xcmprr) {
    return false;
  }
  if (!CheckAndSelectPattern(insn)) {
    return false;
  }
  return true;
}

void ZeroCmpBranchesToTbzPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  CHECK_FATAL(regOpnd != nullptr, "must have regOpnd");
  auto *aarFunc = static_cast<AArch64CGFunc*>(cgFunc);
  ImmOperand &bitOpnd = aarFunc->CreateImmOperand(
      (regOpnd->GetSize() <= k32BitSize) ? (k32BitSize - 1) : (k64BitSize - 1), k8BitSize, false);
  auto &labelOpnd = static_cast<LabelOperand&>(insn.GetOperand(kInsnSecondOpnd));
  Insn &newInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(newMop, *static_cast<RegOperand*>(regOpnd),
                                                                 bitOpnd, labelOpnd);
  bb.ReplaceInsn(insn, newInsn);
  /* update ssa info */
  ssaInfo->ReplaceInsn(insn, newInsn);
  optSuccess = true;
  SetCurrInsn(&newInsn);
  /* dump pattern info */
  if (CG_PEEP_DUMP) {
    std::vector<Insn*> prevs;
    prevs.emplace_back(prevInsn);
    DumpAfterPattern(prevs, &insn, &newInsn);
  }
}

bool LsrAndToUbfxPattern::CheckCondition(Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  if (curMop != MOP_wandrri12 && curMop != MOP_xandrri13) {
    return false;
  }
  int64 immValue = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd)).GetValue();
  /* and_imm value must be (1 << n - 1) */
  if (immValue <= 0 ||
      (((static_cast<uint64>(immValue)) & (static_cast<uint64>(immValue) + 1)) != 0)) {
    return false;
  }
  auto &useReg = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  prevInsn = ssaInfo->GetDefInsn(useReg);
  if (prevInsn == nullptr) {
    return false;
  }
  MOperator prevMop = prevInsn->GetMachineOpcode();
  if (prevMop != MOP_wlsrrri5 && prevMop != MOP_xlsrrri6) {
    return false;
  }
  auto &prevDstOpnd = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
  auto &currUseOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  /* check def-use reg size found by ssa */
  CHECK_FATAL(prevDstOpnd.GetSize() == currUseOpnd.GetSize(), "def-use reg size must be same");
  auto &andDstReg = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  VRegVersion *andDstVersion = ssaInfo->FindSSAVersion(andDstReg.GetRegisterNumber());
  ASSERT(andDstVersion != nullptr, "find destReg Version failed");
  for (auto useDUInfoIt : andDstVersion->GetAllUseInsns()) {
    if (useDUInfoIt.second == nullptr) {
      continue;
    }
    Insn *useInsn = (useDUInfoIt.second)->GetInsn();
    if (useInsn == nullptr) {
      continue;
    }
    MOperator useMop = useInsn->GetMachineOpcode();
    /* combine [and & cbz --> tbz] first, to eliminate more insns becase of incompleted copy prop */
    if (useMop == MOP_wcbz || useMop == MOP_xcbz || useMop == MOP_wcbnz || useMop == MOP_xcbnz) {
      return false;
    }
  }
  return true;
}

void LsrAndToUbfxPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  auto *aarFunc = static_cast<AArch64CGFunc*>(cgFunc);
  bool is64Bits = (static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd)).GetSize() == k64BitSize);
  Operand &resOpnd = insn.GetOperand(kInsnFirstOpnd);
  Operand &srcOpnd = prevInsn->GetOperand(kInsnSecondOpnd);
  int64 immVal1 = static_cast<ImmOperand&>(prevInsn->GetOperand(kInsnThirdOpnd)).GetValue();
  Operand &immOpnd1 = is64Bits ? aarFunc->CreateImmOperand(immVal1, kMaxImmVal6Bits, false) :
                      aarFunc->CreateImmOperand(immVal1, kMaxImmVal5Bits, false);
  int64 tmpVal = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd)).GetValue();
  int64 immVal2 = __builtin_ffsll(tmpVal + 1) - 1;
  if ((immVal2 < k1BitSize) || (is64Bits && (immVal1 + immVal2) > k64BitSize) ||
      (!is64Bits && (immVal1 + immVal2) > k32BitSize)) {
    return;
  }
  Operand &immOpnd2 = is64Bits ? aarFunc->CreateImmOperand(immVal2, kMaxImmVal6Bits, false) :
                      aarFunc->CreateImmOperand(immVal2, kMaxImmVal5Bits, false);
  MOperator newMop = (is64Bits ? MOP_xubfxrri6i6 : MOP_wubfxrri5i5);
  Insn &newInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(newMop, resOpnd, srcOpnd,
                                                                 immOpnd1, immOpnd2);
  bb.ReplaceInsn(insn, newInsn);
  /* update ssa info */
  ssaInfo->ReplaceInsn(insn, newInsn);
  optSuccess = true;
  SetCurrInsn(&newInsn);
  /* dump pattern info */
  if (CG_PEEP_DUMP) {
    std::vector<Insn*> prevs;
    prevs.emplace_back(prevInsn);
    DumpAfterPattern(prevs, &insn, &newInsn);
  }
}

bool MvnAndToBicPattern::CheckCondition(Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  if (curMop != MOP_wandrrr && curMop != MOP_xandrrr) {
    return false;
  }
  auto &useReg1 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  auto &useReg2 = static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd));
  prevInsn1 = ssaInfo->GetDefInsn(useReg1);
  prevInsn2 = ssaInfo->GetDefInsn(useReg2);
  MOperator mop = insn.GetMachineOpcode();
  MOperator desMop = mop == MOP_xandrrr ? MOP_xnotrr : MOP_wnotrr;
  op1IsMvnDef = prevInsn1 != nullptr && prevInsn1->GetMachineOpcode() == desMop;
  op2IsMvnDef = prevInsn2 != nullptr && prevInsn2->GetMachineOpcode() == desMop;
  if (op1IsMvnDef || op2IsMvnDef) {
    return true;
  }
  return false;
}

void MvnAndToBicPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  MOperator newMop = insn.GetMachineOpcode() == MOP_xandrrr ? MOP_xbicrrr : MOP_wbicrrr;
  Insn *prevInsn = op1IsMvnDef ? prevInsn1 : prevInsn2;
  auto &prevOpnd1 = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnSecondOpnd));
  auto &opnd0 = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  auto &opnd1 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  auto &opnd2 = static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd));
  Insn &newInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(
      newMop, opnd0, op1IsMvnDef ? opnd2 : opnd1, prevOpnd1);
  /* update ssa info */
  ssaInfo->ReplaceInsn(insn, newInsn);
  bb.ReplaceInsn(insn, newInsn);
  optSuccess = true;
  SetCurrInsn(&newInsn);
  /* dump pattern info */
  if (CG_PEEP_DUMP) {
    std::vector<Insn*> prevs;
    prevs.emplace_back(prevInsn);
    DumpAfterPattern(prevs, &insn, &newInsn);
  }
}

bool AndCbzToTbzPattern::CheckCondition(Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  if (curMop != MOP_wcbz && curMop != MOP_xcbz && curMop != MOP_wcbnz && curMop != MOP_xcbnz) {
    return false;
  }
  auto &useReg = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  prevInsn = ssaInfo ? ssaInfo->GetDefInsn(useReg) : insn.GetPreviousMachineInsn();
  if (prevInsn == nullptr) {
    return false;
  }
  MOperator prevMop = prevInsn->GetMachineOpcode();
  if (prevMop != MOP_wandrri12 && prevMop != MOP_xandrri13) {
    return false;
  }
  if (!ssaInfo && (&(prevInsn->GetOperand(kInsnFirstOpnd)) != &(insn.GetOperand(kInsnFirstOpnd)))) {
    return false;
  }
  return true;
}

void AndCbzToTbzPattern::Run(BB &bb, Insn &insn) {
  auto *aarchFunc = static_cast<AArch64CGFunc*>(cgFunc);
  if (!CheckCondition(insn)) {
    return;
  }
  auto &andImm = static_cast<ImmOperand&>(prevInsn->GetOperand(kInsnThirdOpnd));
  int64 tbzVal = GetLogValueAtBase2(andImm.GetValue());
  if (tbzVal == -1) {
    return;
  }
  MOperator mOp = insn.GetMachineOpcode();
  MOperator newMop = MOP_undef;
  switch (mOp) {
    case MOP_wcbz:
      newMop = MOP_wtbz;
      break;
    case MOP_wcbnz:
      newMop = MOP_wtbnz;
      break;
    case MOP_xcbz:
      newMop = MOP_xtbz;
      break;
    case MOP_xcbnz:
      newMop = MOP_xtbnz;
      break;
    default:
      CHECK_FATAL(false, "must be cbz/cbnz");
      break;
  }
  auto &labelOpnd = static_cast<LabelOperand&>(insn.GetOperand(kInsnSecondOpnd));
  ImmOperand &tbzImm = aarchFunc->CreateImmOperand(tbzVal, k8BitSize, false);
  Insn &newInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(newMop, prevInsn->GetOperand(kInsnSecondOpnd),
                                                                 tbzImm, labelOpnd);
  bb.ReplaceInsn(insn, newInsn);
  if (ssaInfo) {
    /* update ssa info */
    ssaInfo->ReplaceInsn(insn, newInsn);
  }
  optSuccess = true;
  SetCurrInsn(&newInsn);
  /* dump pattern info */
  if (CG_PEEP_DUMP) {
    std::vector<Insn*> prevs;
    prevs.emplace_back(prevInsn);
    DumpAfterPattern(prevs, &insn, &newInsn);
  }
}

bool CombineSameArithmeticPattern::CheckCondition(Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  if (std::find(validMops.begin(), validMops.end(), curMop) == validMops.end()) {
    return false;
  }
  Operand &useOpnd = insn.GetOperand(kInsnSecondOpnd);
  CHECK_FATAL(useOpnd.IsRegister(), "expect regOpnd");
  prevInsn = ssaInfo->GetDefInsn(static_cast<RegOperand&>(useOpnd));
  if (prevInsn == nullptr) {
    return false;
  }
  if (prevInsn->GetMachineOpcode() != curMop) {
    return false;
  }
  auto &prevDefOpnd = prevInsn->GetOperand(kInsnFirstOpnd);
  CHECK_FATAL(prevDefOpnd.IsRegister(), "expect regOpnd");
  InsnSet useInsns = GetAllUseInsn(static_cast<RegOperand&>(prevDefOpnd));
  if (useInsns.size() > 1) {
    return false;
  }
  auto *aarFunc = static_cast<AArch64CGFunc*>(cgFunc);
  CHECK_FATAL(prevInsn->GetOperand(kInsnThirdOpnd).IsIntImmediate(), "expect immOpnd");
  CHECK_FATAL(insn.GetOperand(kInsnThirdOpnd).IsIntImmediate(), "expect immOpnd");
  auto &prevImmOpnd = static_cast<ImmOperand&>(prevInsn->GetOperand(kInsnThirdOpnd));
  auto &curImmOpnd = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
  int64 prevImm = prevImmOpnd.GetValue();
  int64 curImm = curImmOpnd.GetValue();
  newImmOpnd = &aarFunc->CreateImmOperand(prevImmOpnd.GetValue() + curImmOpnd.GetValue(),
                                          curImmOpnd.GetSize(), curImmOpnd.IsSignedValue());
  switch (curMop) {
    case MOP_wlsrrri5:
    case MOP_wasrrri5:
    case MOP_wlslrri5: {
      if ((prevImm + curImm) < k0BitSizeInt || (prevImm + curImm) >= k32BitSizeInt) {
        return false;
      }
      break;
    }
    case MOP_xlsrrri6:
    case MOP_xasrrri6:
    case MOP_xlslrri6: {
      if ((prevImm + curImm) < k0BitSizeInt || (prevImm + curImm) >= k64BitSizeInt) {
        return false;
      }
      break;
    }
    case MOP_waddrri12:
    case MOP_xaddrri12:
    case MOP_wsubrri12:
    case MOP_xsubrri12: {
      if (!newImmOpnd->IsSingleInstructionMovable()) {
        return false;
      }
      break;
    }
    default:
      return false;
  }
  return true;
}

void CombineSameArithmeticPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  Insn &newInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(insn.GetMachineOpcode(),
                                                                 insn.GetOperand(kInsnFirstOpnd),
                                                                 prevInsn->GetOperand(kInsnSecondOpnd),
                                                                 *newImmOpnd);
  bb.ReplaceInsn(insn, newInsn);
  /* update ssa info */
  ssaInfo->ReplaceInsn(insn, newInsn);
  optSuccess = true;
  SetCurrInsn(&newInsn);
  /* dump pattern info */
  if (CG_PEEP_DUMP) {
    std::vector<Insn*> prevs;
    (void)prevs.emplace_back(prevInsn);
    DumpAfterPattern(prevs, &insn, &newInsn);
  }
}

bool LogicShiftAndOrrToExtrPattern::CheckCondition(Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  if (curMop != MOP_wiorrrr && curMop != MOP_xiorrrr && curMop != MOP_wiorrrrs && curMop != MOP_xiorrrrs) {
    return false;
  }
  Operand &curDstOpnd = insn.GetOperand(kInsnFirstOpnd);
  is64Bits = (curDstOpnd.GetSize() == k64BitSize);
  if (curMop == MOP_wiorrrr || curMop == MOP_xiorrrr) {
    auto &useReg1 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
    Insn *prevInsn1 = ssaInfo->GetDefInsn(useReg1);
    auto &useReg2 = static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd));
    Insn *prevInsn2 = ssaInfo->GetDefInsn(useReg2);
    if (prevInsn1 == nullptr || prevInsn2 == nullptr) {
      return false;
    }
    MOperator prevMop1 = prevInsn1->GetMachineOpcode();
    MOperator prevMop2 = prevInsn2->GetMachineOpcode();
    if ((prevMop1 == MOP_wlsrrri5 || prevMop1 == MOP_xlsrrri6) &&
        (prevMop2 == MOP_wlslrri5 || prevMop2 == MOP_xlslrri6)) {
      prevLsrInsn = prevInsn1;
      prevLslInsn = prevInsn2;
    } else if ((prevMop2 == MOP_wlsrrri5 || prevMop2 == MOP_xlsrrri6) &&
               (prevMop1 == MOP_wlslrri5 || prevMop1 == MOP_xlslrri6)) {
      prevLsrInsn = prevInsn2;
      prevLslInsn = prevInsn1;
    } else {
      return false;
    }
    int64 prevLsrImmValue = static_cast<ImmOperand&>(prevLsrInsn->GetOperand(kInsnThirdOpnd)).GetValue();
    int64 prevLslImmValue = static_cast<ImmOperand&>(prevLslInsn->GetOperand(kInsnThirdOpnd)).GetValue();
    if ((prevLsrImmValue + prevLslImmValue) < 0) {
      return false;
    }
    if ((is64Bits && (prevLsrImmValue + prevLslImmValue) != k64BitSize) ||
        (!is64Bits && (prevLsrImmValue + prevLslImmValue) != k32BitSize)) {
      return false;
    }
    shiftValue = prevLsrImmValue;
  } else if (curMop == MOP_wiorrrrs || curMop == MOP_xiorrrrs) {
    auto &useReg = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
    Insn *prevInsn = ssaInfo->GetDefInsn(useReg);
    if (prevInsn == nullptr) {
      return false;
    }
    MOperator prevMop = prevInsn->GetMachineOpcode();
    if (prevMop != MOP_wlsrrri5 && prevMop != MOP_xlsrrri6 && prevMop != MOP_wlslrri5 && prevMop != MOP_xlslrri6) {
      return false;
    }
    int64 prevImm = static_cast<ImmOperand&>(prevInsn->GetOperand(kInsnThirdOpnd)).GetValue();
    auto &shiftOpnd = static_cast<BitShiftOperand&>(insn.GetOperand(kInsnFourthOpnd));
    uint32 shiftAmount = shiftOpnd.GetShiftAmount();
    if (shiftOpnd.GetShiftOp() == BitShiftOperand::kShiftLSL && (prevMop == MOP_wlsrrri5 || prevMop == MOP_xlsrrri6)) {
      prevLsrInsn = prevInsn;
      shiftValue = prevImm;
    } else if (shiftOpnd.GetShiftOp() == BitShiftOperand::kShiftLSR &&
               (prevMop == MOP_wlslrri5 || prevMop == MOP_xlslrri6)) {
      prevLslInsn = prevInsn;
      shiftValue = shiftAmount;
    } else {
      return false;
    }
    if (prevImm + static_cast<int64>(shiftAmount) < 0) {
      return false;
    }
    if ((is64Bits && (prevImm + static_cast<int64>(shiftAmount)) != k64BitSize) ||
        (!is64Bits && (prevImm + static_cast<int64>(shiftAmount)) != k32BitSize)) {
      return false;
    }
  } else {
    CHECK_FATAL(false, "must be above mop");
    return false;
  }
  return true;
}

void LogicShiftAndOrrToExtrPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  auto *aarFunc = static_cast<AArch64CGFunc*>(cgFunc);
  Operand &opnd1 = (prevLslInsn == nullptr ? insn.GetOperand(kInsnThirdOpnd) :
                    prevLslInsn->GetOperand(kInsnSecondOpnd));
  Operand &opnd2 = (prevLsrInsn == nullptr ? insn.GetOperand(kInsnThirdOpnd) :
                    prevLsrInsn->GetOperand(kInsnSecondOpnd));
  ImmOperand &immOpnd = is64Bits ? aarFunc->CreateImmOperand(shiftValue, kMaxImmVal6Bits, false) :
      aarFunc->CreateImmOperand(shiftValue, kMaxImmVal5Bits, false);
  MOperator newMop = is64Bits ? MOP_xextrrrri6 : MOP_wextrrrri5;
  Insn &newInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(newMop, insn.GetOperand(kInsnFirstOpnd),
                                                                 opnd1, opnd2, immOpnd);
  bb.ReplaceInsn(insn, newInsn);
  /* update ssa info */
  ssaInfo->ReplaceInsn(insn, newInsn);
  optSuccess = true;
  SetCurrInsn(&newInsn);
  /* dump pattern info */
  if (CG_PEEP_DUMP) {
    std::vector<Insn*> prevs;
    prevs.emplace_back(prevLsrInsn);
    prevs.emplace_back(prevLslInsn);
    DumpAfterPattern(prevs, &insn, &newInsn);
  }
}

void SimplifyMulArithmeticPattern::SetArithType(const Insn &currInsn) {
  MOperator mOp = currInsn.GetMachineOpcode();
  switch (mOp) {
    case MOP_waddrrr:
    case MOP_xaddrrr: {
      arithType = kAdd;
      isFloat = false;
      break;
    }
    case MOP_dadd:
    case MOP_sadd: {
      arithType = kFAdd;
      isFloat = true;
      break;
    }
    case MOP_wsubrrr:
    case MOP_xsubrrr: {
      arithType = kSub;
      isFloat = false;
      validOpndIdx = kInsnThirdOpnd;
      break;
    }
    case MOP_dsub:
    case MOP_ssub: {
      arithType = kFSub;
      isFloat = true;
      validOpndIdx = kInsnThirdOpnd;
      break;
    }
    case MOP_xinegrr:
    case MOP_winegrr: {
      arithType = kNeg;
      isFloat = false;
      validOpndIdx = kInsnSecondOpnd;
      break;
    }
    case MOP_wfnegrr:
    case MOP_xfnegrr: {
      arithType = kFNeg;
      isFloat = true;
      validOpndIdx = kInsnSecondOpnd;
      break;
    }
    default: {
      CHECK_FATAL(false, "must be above mop");
      break;
    }
  }
}

bool SimplifyMulArithmeticPattern::CheckCondition(Insn &insn) {
  if (arithType == kUndef || validOpndIdx < 0) {
    return false;
  }
  auto &useReg = static_cast<RegOperand&>(insn.GetOperand(static_cast<uint32>(validOpndIdx)));
  prevInsn = ssaInfo->GetDefInsn(useReg);
  if (prevInsn == nullptr) {
    return false;
  }
  regno_t useRegNO = useReg.GetRegisterNumber();
  VRegVersion *useVersion = ssaInfo->FindSSAVersion(useRegNO);
  ASSERT(useVersion != nullptr, "useVersion should not be nullptr");
  if (useVersion->GetAllUseInsns().size() > 1) {
    return false;
  }
  MOperator currMop = insn.GetMachineOpcode();
  if (currMop == MOP_dadd || currMop == MOP_sadd || currMop == MOP_dsub || currMop == MOP_ssub ||
      currMop == MOP_wfnegrr || currMop == MOP_xfnegrr) {
    isFloat = true;
  }
  MOperator prevMop = prevInsn->GetMachineOpcode();
  if (prevMop != MOP_wmulrrr && prevMop != MOP_xmulrrr && prevMop != MOP_xvmuld && prevMop != MOP_xvmuls) {
    return false;
  }
  if (isFloat && (prevMop == MOP_wmulrrr || prevMop == MOP_xmulrrr)) {
    return false;
  }
  if (!isFloat && (prevMop == MOP_xvmuld || prevMop == MOP_xvmuls)) {
    return false;
  }
  if ((currMop == MOP_xaddrrr) || (currMop == MOP_waddrrr)) {
    return true;
  }
  return CGOptions::IsFastMath();
}

void SimplifyMulArithmeticPattern::DoOptimize(BB &currBB, Insn &currInsn) {
  Operand &resOpnd = currInsn.GetOperand(kInsnFirstOpnd);
  Operand &opndMulOpnd1 = prevInsn->GetOperand(kInsnSecondOpnd);
  Operand &opndMulOpnd2 = prevInsn->GetOperand(kInsnThirdOpnd);
  bool is64Bits = (static_cast<RegOperand&>(resOpnd).GetSize() == k64BitSize);
  /* may overflow */
  if ((prevInsn->GetOperand(kInsnFirstOpnd).GetSize() == k32BitSize) && is64Bits) {
    return;
  }
  MOperator newMop = is64Bits ? curMop2NewMopTable[arithType][1] : curMop2NewMopTable[arithType][0];
  Insn *newInsn = nullptr;
  if (arithType == kNeg || arithType == kFNeg) {
    newInsn = &(cgFunc->GetCG()->BuildInstruction<AArch64Insn>(newMop, resOpnd, opndMulOpnd1, opndMulOpnd2));
  } else {
    Operand &opnd3 = (validOpndIdx == kInsnSecondOpnd) ? currInsn.GetOperand(kInsnThirdOpnd) :
        currInsn.GetOperand(kInsnSecondOpnd);
    newInsn = &(cgFunc->GetCG()->BuildInstruction<AArch64Insn>(newMop, resOpnd, opndMulOpnd1, opndMulOpnd2, opnd3));
  }
  CHECK_FATAL(newInsn != nullptr, "must create newInsn");
  currBB.ReplaceInsn(currInsn, *newInsn);
  /* update ssa info */
  ssaInfo->ReplaceInsn(currInsn, *newInsn);
  optSuccess = true;
  /* dump pattern info */
  if (CG_PEEP_DUMP) {
    std::vector<Insn*> prevs;
    prevs.emplace_back(prevInsn);
    DumpAfterPattern(prevs, &currInsn, newInsn);
  }
}

void SimplifyMulArithmeticPattern::Run(BB &bb, Insn &insn) {
  SetArithType(insn);
  if (arithType == kAdd || arithType == kFAdd) {
    validOpndIdx = kInsnSecondOpnd;
    if (CheckCondition(insn)) {
      DoOptimize(bb, insn);
      return;
    } else {
      validOpndIdx = kInsnThirdOpnd;
    }
  }
  if (!CheckCondition(insn)) {
    return;
  }
  DoOptimize(bb, insn);
}

void ElimSpecificExtensionPattern::SetSpecificExtType(const Insn &currInsn) {
  MOperator mOp = currInsn.GetMachineOpcode();
  switch (mOp) {
    case MOP_xsxtb32: {
      is64Bits = false;
      extTypeIdx = SXTB;
      break;
    }
    case MOP_xsxtb64: {
      is64Bits = true;
      extTypeIdx = SXTB;
      break;
    }
    case MOP_xsxth32: {
      is64Bits = false;
      extTypeIdx = SXTH;
      break;
    }
    case MOP_xsxth64: {
      is64Bits = true;
      extTypeIdx = SXTH;
      break;
    }
    case MOP_xsxtw64: {
      is64Bits = true;
      extTypeIdx = SXTW;
      break;
    }
    case MOP_xuxtb32: {
      is64Bits = false;
      extTypeIdx = UXTB;
      break;
    }
    case MOP_xuxth32: {
      is64Bits = false;
      extTypeIdx = UXTH;
      break;
    }
    case MOP_xuxtw64: {
      is64Bits = true;
      extTypeIdx = UXTW;
      break;
    }
    default: {
      extTypeIdx = EXTUNDEF;
    }
  }
}

void ElimSpecificExtensionPattern::SetOptSceneType() {
  if (prevInsn->IsCall()) {
    sceneType = kSceneMov;
    return;
  }
  MOperator preMop = prevInsn->GetMachineOpcode();
  switch (preMop) {
    case MOP_wldr:
    case MOP_wldrb:
    case MOP_wldrsb:
    case MOP_wldrh:
    case MOP_wldrsh:
    case MOP_xldrsw: {
      sceneType = kSceneLoad;
      break;
    }
    case MOP_xmovri32:
    case MOP_xmovri64: {
      sceneType = kSceneMov;
      break;
    }
    case MOP_xsxtb32:
    case MOP_xsxtb64:
    case MOP_xsxth32:
    case MOP_xsxth64:
    case MOP_xsxtw64:
    case MOP_xuxtb32:
    case MOP_xuxth32:
    case MOP_xuxtw64: {
      sceneType = kSceneSameExt;
      break;
    }
    default: {
      sceneType = kSceneUndef;
    }
  }
}

void ElimSpecificExtensionPattern::ReplaceExtWithMov(Insn &currInsn) {
  auto &prevDstOpnd = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
  auto &currDstOpnd = static_cast<RegOperand&>(currInsn.GetOperand(kInsnFirstOpnd));
  MOperator newMop = is64Bits ? MOP_xmovrr : MOP_wmovrr;
  Insn &newInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(newMop, currDstOpnd, prevDstOpnd);
  currBB->ReplaceInsn(currInsn, newInsn);
  /* update ssa info */
  ssaInfo->ReplaceInsn(currInsn, newInsn);
  optSuccess = true;
  /* dump pattern info */
  if (CG_PEEP_DUMP) {
    std::vector<Insn*> prevs;
    prevs.emplace_back(prevInsn);
    DumpAfterPattern(prevs, &currInsn, &newInsn);
  }
}

void ElimSpecificExtensionPattern::ElimExtensionAfterMov(Insn &insn) {
  if (&insn == currBB->GetFirstInsn()) {
    return;
  }
  auto &prevDstOpnd = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
  auto &currDstOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  auto &currSrcOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  if (prevDstOpnd.GetSize() != currDstOpnd.GetSize()) {
    return;
  }
  MOperator currMop = insn.GetMachineOpcode();
  /* example 2) [mov w0, R0] is return value of call and return size is not of range */
  if (prevInsn->IsCall() && (currSrcOpnd.GetRegisterNumber() == R0 || currSrcOpnd.GetRegisterNumber() == V0) &&
      currDstOpnd.GetRegisterNumber() == currSrcOpnd.GetRegisterNumber()) {
    uint32 retSize = prevInsn->GetRetSize();
    if (retSize > 0 &&
        ((currMop == MOP_xuxtb32 && retSize <= k1ByteSize) ||
         (currMop == MOP_xuxth32 && retSize <= k2ByteSize) ||
         (currMop == MOP_xuxtw64 && retSize <= k4ByteSize))) {
      ReplaceExtWithMov(insn);
    }
    return;
  }
  if (prevInsn->IsCall() && prevInsn->GetIsCallReturnSigned()) {
    return;
  }
  auto &immMovOpnd = static_cast<ImmOperand&>(prevInsn->GetOperand(kInsnSecondOpnd));
  int64 value = immMovOpnd.GetValue();
  uint64 minRange = extValueRangeTable[extTypeIdx][0];
  uint64 maxRange = extValueRangeTable[extTypeIdx][1];
  if (currMop == MOP_xsxtb32 || currMop == MOP_xsxth32) {
    /* value should be in valid range */
    if (static_cast<uint64>(value) >= minRange && static_cast<uint64>(value) <= maxRange &&
        immMovOpnd.IsSingleInstructionMovable(currDstOpnd.GetSize())) {
      ReplaceExtWithMov(insn);
    }
  } else if (currMop == MOP_xuxtb32 || currMop == MOP_xuxth32) {
    if ((static_cast<uint64>(value) & minRange) == 0) {
      ReplaceExtWithMov(insn);
    }
  } else if (currMop == MOP_xuxtw64) {
    ReplaceExtWithMov(insn);
  } else {
    /* MOP_xsxtb64 & MOP_xsxth64 & MOP_xsxtw64 */
    if ((static_cast<uint64>(value) & minRange) == 0 && immMovOpnd.IsSingleInstructionMovable(currDstOpnd.GetSize())) {
      ReplaceExtWithMov(insn);
    }
  }
}

bool ElimSpecificExtensionPattern::IsValidLoadExtPattern(MOperator oldMop, MOperator newMop) const {
  if (oldMop == newMop) {
    return true;
  }
  auto *aarFunc = static_cast<AArch64CGFunc*>(cgFunc);
  auto *memOpnd = static_cast<MemOperand*>(prevInsn->GetMemOpnd());
  ASSERT(!prevInsn->IsStorePair(), "do not do ElimSpecificExtensionPattern for str pair");
  ASSERT(!prevInsn->IsLoadPair(), "do not do ElimSpecificExtensionPattern for ldr pair");
  if (memOpnd->GetAddrMode() == MemOperand::kAddrModeBOi &&
      !aarFunc->IsOperandImmValid(newMop, memOpnd, kInsnSecondOpnd)) {
    return false;
  }
  uint32 shiftAmount = memOpnd->ShiftAmount();
  if (shiftAmount == 0) {
    return true;
  }
  const AArch64MD *md = &AArch64CG::kMd[newMop];
  uint32 memSize = md->GetOperandSize() / k8BitSize;
  uint32 validShiftAmount = ((memSize == k8BitSize) ? k3BitSize : ((memSize == k4BitSize) ? k2BitSize :
      ((memSize == k2BitSize) ? k1BitSize : k0BitSize)));
  if (shiftAmount != validShiftAmount) {
    return false;
  }
  return true;
}

MOperator ElimSpecificExtensionPattern::SelectNewLoadMopByBitSize(MOperator lowBitMop) const {
  auto &prevDstOpnd = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
  switch (lowBitMop) {
    case MOP_wldrsb: {
      prevDstOpnd.SetSize(k64BitSize);
      return MOP_xldrsb;
    }
    case MOP_wldrsh: {
      prevDstOpnd.SetSize(k64BitSize);
      return MOP_xldrsh;
    }
    default:
      break;
  }
  return lowBitMop;
}

void ElimSpecificExtensionPattern::ElimExtensionAfterLoad(Insn &insn) {
  if (extTypeIdx == EXTUNDEF) {
    return;
  }
  MOperator prevOrigMop = prevInsn->GetMachineOpcode();
  for (uint8 i = 0; i < kPrevLoadPatternNum; i++) {
    ASSERT(extTypeIdx < SETS, "extTypeIdx must be lower than SETS");
    if (prevOrigMop != loadMappingTable[extTypeIdx][i][0]) {
      continue;
    }
    MOperator prevNewMop = loadMappingTable[extTypeIdx][i][1];
    if (!IsValidLoadExtPattern(prevOrigMop, prevNewMop)) {
      return;
    }
    if (is64Bits && extTypeIdx >= SXTB && extTypeIdx <= SXTW) {
      prevNewMop = SelectNewLoadMopByBitSize(prevNewMop);
    }
    auto &prevDstOpnd = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
    auto &currDstOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
    /* to avoid {mov [64], [32]} in the case of big endian */
    if (prevDstOpnd.GetSize() != currDstOpnd.GetSize()) {
      return;
    }

    auto *newMemOp =
        GetOrCreateMemOperandForNewMOP(*cgFunc, *prevInsn, prevNewMop);

    if (newMemOp == nullptr) {
      return;
    }

    auto *aarCGSSAInfo = static_cast<AArch64CGSSAInfo*>(ssaInfo);
    if (CG_PEEP_DUMP) {
      LogInfo::MapleLogger() << ">>>>>>> In " << GetPatternName() << " : <<<<<<<\n";
      if (prevOrigMop != prevNewMop) {
        LogInfo::MapleLogger() << "======= OrigPrevInsn : \n";
        prevInsn->Dump();
        aarCGSSAInfo->DumpInsnInSSAForm(*prevInsn);
      }
    }

    prevInsn->SetMemOpnd(newMemOp);
    prevInsn->SetMOP(prevNewMop);

    if ((prevOrigMop != prevNewMop) && CG_PEEP_DUMP) {
      LogInfo::MapleLogger() << "======= NewPrevInsn : \n";
      prevInsn->Dump();
      aarCGSSAInfo->DumpInsnInSSAForm(*prevInsn);
    }

    MOperator movMop = is64Bits ? MOP_xmovrr : MOP_wmovrr;
    Insn &newMovInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(movMop, insn.GetOperand(kInsnFirstOpnd),
                                                                      prevInsn->GetOperand(kInsnFirstOpnd));
    currBB->ReplaceInsn(insn, newMovInsn);
    /* update ssa info */
    ssaInfo->ReplaceInsn(insn, newMovInsn);
    optSuccess = true;
    /* dump pattern info */
    if (CG_PEEP_DUMP) {
      LogInfo::MapleLogger() << "======= ReplacedInsn :\n";
      insn.Dump();
      aarCGSSAInfo->DumpInsnInSSAForm(insn);
      LogInfo::MapleLogger() << "======= NewInsn :\n";
      newMovInsn.Dump();
      aarCGSSAInfo->DumpInsnInSSAForm(newMovInsn);
    }
  }
}

void ElimSpecificExtensionPattern::ElimExtensionAfterSameExt(Insn &insn) {
  if (extTypeIdx == EXTUNDEF) {
    return;
  }
  auto &prevDstOpnd = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
  auto &currDstOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  if (prevDstOpnd.GetSize() != currDstOpnd.GetSize()) {
    return;
  }
  MOperator prevMop = prevInsn->GetMachineOpcode();
  MOperator currMop = insn.GetMachineOpcode();
  for (uint8 i = 0; i < kSameExtPatternNum; i++) {
    ASSERT(extTypeIdx < SETS, "extTypeIdx must be lower than SETS");
    if (sameExtMappingTable[extTypeIdx][i][0] == MOP_undef || sameExtMappingTable[extTypeIdx][i][1] == MOP_undef) {
      continue;
    }
    if (prevMop == sameExtMappingTable[extTypeIdx][i][0] && currMop == sameExtMappingTable[extTypeIdx][i][1]) {
      ReplaceExtWithMov(insn);
    }
  }
}

bool ElimSpecificExtensionPattern::CheckCondition(Insn &insn) {
  auto &useReg = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  prevInsn = ssaInfo->GetDefInsn(useReg);
  InsnSet useInsns = GetAllUseInsn(useReg);
  if ((prevInsn == nullptr) || (useInsns.size() != 1)) {
    return false;
  }
  SetOptSceneType();
  SetSpecificExtType(insn);
  if (sceneType == kSceneUndef) {
    return false;
  }
  return true;
}

void ElimSpecificExtensionPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  if (sceneType == kSceneMov) {
    ElimExtensionAfterMov(insn);
  } else if (sceneType == kSceneLoad) {
    ElimExtensionAfterLoad(insn);
  } else if (sceneType == kSceneSameExt) {
    ElimExtensionAfterSameExt(insn);
  }
}

void OneHoleBranchPattern::FindNewMop(const BB &bb, const Insn &insn) {
  if (&insn != bb.GetLastInsn()) {
    return;
  }
  MOperator thisMop = insn.GetMachineOpcode();
  switch (thisMop) {
    case MOP_wcbz:
      newOp = MOP_wtbnz;
      break;
    case MOP_wcbnz:
      newOp = MOP_wtbz;
      break;
    case MOP_xcbz:
      newOp = MOP_xtbnz;
      break;
    case MOP_xcbnz:
      newOp = MOP_xtbz;
      break;
    default:
      break;
  }
}

/*
 * pattern1:
 *  uxtb w0, w1     <-----(ValidBitsNum <= 8)
 *  cbz w0, .label
 *  ===>
 *  cbz w1, .label
 *
 * pattern2:
 *  uxtb w2, w1     <-----(ValidBitsNum == 1)
 *  eor w3, w2, #1
 *  cbz w3, .label
 *  ===>
 *   tbnz w1, #0, .label
 */
void OneHoleBranchPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  LabelOperand &label = static_cast<LabelOperand&>(insn.GetOperand(kInsnSecondOpnd));
  bool pattern1 = (prevInsn->GetMachineOpcode() == MOP_xuxtb32) &&
                  (static_cast<RegOperand&>(prevInsn->GetOperand(kInsnSecondOpnd)).GetValidBitsNum() <= k8BitSize ||
                   static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd)).GetValidBitsNum() <= k8BitSize);
  if (pattern1) {
    Insn &newCbzInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(
        insn.GetMachineOpcode(), prevInsn->GetOperand(kInsnSecondOpnd), label);
    bb.ReplaceInsn(insn, newCbzInsn);
    ssaInfo->ReplaceInsn(insn, newCbzInsn);
    optSuccess = true;
    SetCurrInsn(&newCbzInsn);
    if (CG_PEEP_DUMP) {
      std::vector<Insn*> prevs;
      prevs.emplace_back(prevInsn);
      DumpAfterPattern(prevs, &newCbzInsn, nullptr);
    }
    return;
  }
  bool pattern2 = (prevInsn->GetMachineOpcode() == MOP_xeorrri13 || prevInsn->GetMachineOpcode() == MOP_weorrri12) &&
                  (static_cast<ImmOperand&>(prevInsn->GetOperand(kInsnThirdOpnd)).GetValue() == 1);
  if (pattern2) {
    if (!CheckPrePrevInsn()) {
      return;
    }
    AArch64CGFunc *aarch64CGFunc = static_cast<AArch64CGFunc*>(cgFunc);
    ImmOperand &oneHoleOpnd = aarch64CGFunc->CreateImmOperand(0, k8BitSize, false);
    auto &regOperand = static_cast<RegOperand&>(prePrevInsn->GetOperand(kInsnSecondOpnd));
    Insn &newTbzInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(newOp, regOperand, oneHoleOpnd, label);
    bb.ReplaceInsn(insn, newTbzInsn);
    ssaInfo->ReplaceInsn(insn, newTbzInsn);
    optSuccess = true;
    if (CG_PEEP_DUMP) {
      std::vector<Insn*> prevs;
      prevs.emplace_back(prevInsn);
      prevs.emplace_back(prePrevInsn);
      DumpAfterPattern(prevs, &newTbzInsn, nullptr);
    }
  }
}

bool OneHoleBranchPattern::CheckCondition(Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  if (curMop != MOP_wcbz && curMop != MOP_xcbz && curMop != MOP_wcbnz && curMop != MOP_xcbnz) {
    return false;
  }
  FindNewMop(*insn.GetBB(), insn);
  if (newOp == MOP_undef) {
    return false;
  }
  auto &useReg = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  prevInsn = ssaInfo->GetDefInsn(useReg);
  if (prevInsn == nullptr) {
    return false;
  }
  if (&(prevInsn->GetOperand(kInsnFirstOpnd)) != &(insn.GetOperand(kInsnFirstOpnd))) {
    return false;
  }
  return true;
}

bool OneHoleBranchPattern::CheckPrePrevInsn() {
  auto &useReg = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnSecondOpnd));
  prePrevInsn = ssaInfo->GetDefInsn(useReg);
  if (prePrevInsn == nullptr) {
    return false;
  }
  if (prePrevInsn->GetMachineOpcode() != MOP_xuxtb32 ||
      static_cast<RegOperand&>(prePrevInsn->GetOperand(kInsnSecondOpnd)).GetValidBitsNum() != 1) {
    return false;
  }
  if (&(prePrevInsn->GetOperand(kInsnFirstOpnd)) != &(prevInsn->GetOperand(kInsnSecondOpnd))) {
    return false;
  }
  return true;
}

void OrrToMovPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  RegOperand *reg1 = &static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  Insn &newInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(newMop, *reg1, *reg2);
  bb.ReplaceInsn(insn, newInsn);
  ssaInfo->ReplaceInsn(insn, newInsn);
  optSuccess = true;
  SetCurrInsn(&newInsn);
  if (CG_PEEP_DUMP) {
    std::vector<Insn*> prevs;
    prevs.emplace_back(&insn);
    DumpAfterPattern(prevs, &newInsn, nullptr);
  }
}

bool OrrToMovPattern::CheckCondition(Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  if (curMop != MOP_wiorrri12 && curMop != MOP_xiorrri13) {
    return false;
  }
  MOperator thisMop = insn.GetMachineOpcode();
  Operand *opndOfOrr = nullptr;
  switch (thisMop) {
    case MOP_wiorrri12: {  /* opnd1 is reg32 and opnd3 is immediate. */
      opndOfOrr = &(insn.GetOperand(kInsnThirdOpnd));
      reg2 = &static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
      newMop = MOP_wmovrr;
      break;
    }
    case MOP_xiorrri13: {  /* opnd1 is reg64 and opnd3 is immediate. */
      opndOfOrr = &(insn.GetOperand(kInsnThirdOpnd));
      reg2 = &static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
      newMop = MOP_xmovrr;
      break;
    }
    default:
      return false;
  }
  CHECK_FATAL(opndOfOrr->IsIntImmediate(), "expects immediate operand");
  ImmOperand *immOpnd = static_cast<ImmOperand*>(opndOfOrr);
  if (immOpnd->GetValue() != 0) {
    return false;
  }
  return true;
}

void AArch64CGPeepHole::DoNormalOptimize(BB &bb, Insn &insn) {
  MOperator thisMop = insn.GetMachineOpcode();
  manager = peepMemPool->New<PeepOptimizeManager>(*cgFunc, bb, insn);
  switch (thisMop) {
    /*
     * e.g.
     * execute before & after RA: manager->NormalPatternOpt<>(true)
     * execute before RA: manager->NormalPatternOpt<>(!cgFunc->IsAfterRegAlloc())
     * execute after RA: manager->NormalPatternOpt<>(cgFunc->IsAfterRegAlloc())
     */
    case MOP_xubfxrri6i6: {
      manager->NormalPatternOpt<UbfxToUxtwPattern>(!cgFunc->IsAfterRegAlloc());
      break;
    }
    case MOP_xmovzri16: {
      manager->NormalPatternOpt<LoadFloatPointPattern>(!cgFunc->IsAfterRegAlloc());
      break;
    }
    case MOP_wcmpri: {
      manager->NormalPatternOpt<LongIntCompareWithZPattern>(!cgFunc->IsAfterRegAlloc());
      break;
    }
    case MOP_wcmprr: {
      manager->NormalPatternOpt<LdrCmpPattern>(cgFunc->IsAfterRegAlloc());
      break;
    }
    case MOP_wmovrr:
    case MOP_xmovrr:
    case MOP_xvmovs:
    case MOP_xvmovd:
    case MOP_vmovuu:
    case MOP_vmovvv: {
      manager->NormalPatternOpt<RemoveMovingtoSameRegPattern>(cgFunc->IsAfterRegAlloc());
      break;
    }
    case MOP_wstrb:
    case MOP_wldrb:
    case MOP_wstrh:
    case MOP_wldrh:
    case MOP_xldr:
    case MOP_xstr:
    case MOP_wldr:
    case MOP_wstr:
    case MOP_dldr:
    case MOP_dstr:
    case MOP_sldr:
    case MOP_sstr:
    case MOP_qldr:
    case MOP_qstr: {
      manager->NormalPatternOpt<CombineContiLoadAndStorePattern>(cgFunc->IsAfterRegAlloc());
      manager->NormalPatternOpt<ContiLDRorSTRToSameMEMPattern>(cgFunc->IsAfterRegAlloc());
      manager->NormalPatternOpt<RemoveIdenticalLoadAndStorePattern>(cgFunc->IsAfterRegAlloc());
      break;
    }
    case MOP_xvmovrv:
    case MOP_xvmovrd: {
      manager->NormalPatternOpt<FmovRegPattern>(cgFunc->IsAfterRegAlloc());
      break;
    }
    case MOP_xsbfxrri6i6: {
      manager->NormalPatternOpt<SbfxOptPattern>(cgFunc->IsAfterRegAlloc());
      break;
    }
    case MOP_wcbz:
    case MOP_xcbz:
    case MOP_wcbnz:
    case MOP_xcbnz: {
      manager->NormalPatternOpt<AndCbzToTbzPattern>(!cgFunc->IsAfterRegAlloc());
      manager->NormalPatternOpt<CbnzToCbzPattern>(cgFunc->IsAfterRegAlloc());
      break;
    }
    case MOP_wsdivrrr: {
      manager->NormalPatternOpt<ReplaceDivToMultiPattern>(cgFunc->IsAfterRegAlloc());
      break;
    }
    case MOP_xbl: {
      if (JAVALANG) {
        manager->NormalPatternOpt<RemoveIncRefPattern>(!cgFunc->IsAfterRegAlloc());
        manager->NormalPatternOpt<RemoveDecRefPattern>(!cgFunc->IsAfterRegAlloc());
        manager->NormalPatternOpt<ReplaceIncDecWithIncPattern>(!cgFunc->IsAfterRegAlloc());
        manager->NormalPatternOpt<RemoveIncDecRefPattern>(cgFunc->IsAfterRegAlloc());
      }
      if (CGOptions::IsGCOnly() && CGOptions::DoWriteRefFieldOpt()) {
        manager->NormalPatternOpt<WriteFieldCallPattern>(!cgFunc->IsAfterRegAlloc());
      }
      break;
    }
    default:
      break;
  }
  /* skip if it is not a read barrier call. */
  if (GetReadBarrierName(insn) != "") {
    manager->NormalPatternOpt<InlineReadBarriersPattern>(!cgFunc->IsAfterRegAlloc());
  }
}
/* ======== CGPeepPattern End ======== */

void AArch64PeepHole::InitOpts() {
  optimizations.resize(kPeepholeOptsNum);
  optimizations[kEliminateSpecifcSXTOpt] = optOwnMemPool->New<EliminateSpecifcSXTAArch64>(cgFunc);
  optimizations[kEliminateSpecifcUXTOpt] = optOwnMemPool->New<EliminateSpecifcUXTAArch64>(cgFunc);
  optimizations[kCsetCbzToBeqOpt] = optOwnMemPool->New<CsetCbzToBeqOptAArch64>(cgFunc);
  optimizations[kAndCmpBranchesToCsetOpt] = optOwnMemPool->New<AndCmpBranchesToCsetAArch64>(cgFunc);
  optimizations[kAndCmpBranchesToTstOpt] = optOwnMemPool->New<AndCmpBranchesToTstAArch64>(cgFunc);
  optimizations[kAndCbzBranchesToTstOpt] = optOwnMemPool->New<AndCbzBranchesToTstAArch64>(cgFunc);
  optimizations[kZeroCmpBranchesOpt] = optOwnMemPool->New<ZeroCmpBranchesAArch64>(cgFunc);
  optimizations[kCselZeroOneToCsetOpt] = optOwnMemPool->New<CselZeroOneToCsetOpt>(cgFunc);
}

void AArch64PeepHole::Run(BB &bb, Insn &insn) {
  MOperator thisMop = insn.GetMachineOpcode();
  switch (thisMop) {
    case MOP_xsxtb32:
    case MOP_xsxth32:
    case MOP_xsxtb64:
    case MOP_xsxth64:
    case MOP_xsxtw64: {
      (static_cast<EliminateSpecifcSXTAArch64*>(optimizations[kEliminateSpecifcSXTOpt]))->Run(bb, insn);
      break;
    }
    case MOP_xuxtb32:
    case MOP_xuxth32:
    case MOP_xuxtw64: {
      (static_cast<EliminateSpecifcUXTAArch64*>(optimizations[kEliminateSpecifcUXTOpt]))->Run(bb, insn);
      break;
    }
    case MOP_wcbnz:
    case MOP_xcbnz: {
      (static_cast<CsetCbzToBeqOptAArch64*>(optimizations[kCsetCbzToBeqOpt]))->Run(bb, insn);
      break;
    }
    case MOP_wcbz:
    case MOP_xcbz: {
      (static_cast<CsetCbzToBeqOptAArch64*>(optimizations[kCsetCbzToBeqOpt]))->Run(bb, insn);
      break;
    }
    case MOP_wcsetrc:
    case MOP_xcsetrc: {
      (static_cast<AndCmpBranchesToCsetAArch64*>(optimizations[kAndCmpBranchesToCsetOpt]))->Run(bb, insn);
      break;
    }
    case MOP_xandrrr:
    case MOP_wandrrr:
    case MOP_wandrri12:
    case MOP_xandrri13: {
      (static_cast<AndCmpBranchesToTstAArch64*>(optimizations[kAndCmpBranchesToTstOpt]))->Run(bb, insn);
      (static_cast<AndCbzBranchesToTstAArch64*>(optimizations[kAndCbzBranchesToTstOpt]))->Run(bb, insn);
      break;
    }
    case MOP_wcselrrrc:
    case MOP_xcselrrrc: {
      (static_cast<CselZeroOneToCsetOpt*>(optimizations[kCselZeroOneToCsetOpt]))->Run(bb, insn);
      break;
    }
    default:
      break;
  }
  if (&insn == bb.GetLastInsn()) {
    (static_cast<ZeroCmpBranchesAArch64*>(optimizations[kZeroCmpBranchesOpt]))->Run(bb, insn);
  }
}

void AArch64PeepHole0::InitOpts() {
  optimizations.resize(kPeepholeOptsNum);
  optimizations[kRemoveIdenticalLoadAndStoreOpt] = optOwnMemPool->New<RemoveIdenticalLoadAndStoreAArch64>(cgFunc);
  optimizations[kComplexMemOperandOptAdd] = optOwnMemPool->New<ComplexMemOperandAddAArch64>(cgFunc);
  optimizations[kDeleteMovAfterCbzOrCbnzOpt] = optOwnMemPool->New<DeleteMovAfterCbzOrCbnzAArch64>(cgFunc);
  optimizations[kRemoveSxtBeforeStrOpt] = optOwnMemPool->New<RemoveSxtBeforeStrAArch64>(cgFunc);
  optimizations[kRemoveMovingtoSameRegOpt] = optOwnMemPool->New<RemoveMovingtoSameRegAArch64>(cgFunc);
}

void AArch64PeepHole0::Run(BB &bb, Insn &insn) {
  MOperator thisMop = insn.GetMachineOpcode();
  switch (thisMop) {
    case MOP_xstr:
    case MOP_wstr: {
      (static_cast<RemoveIdenticalLoadAndStoreAArch64*>(optimizations[kRemoveIdenticalLoadAndStoreOpt]))->Run(bb, insn);
      break;
    }
    case MOP_xaddrrr: {
      (static_cast<ComplexMemOperandAddAArch64*>(optimizations[kComplexMemOperandOptAdd]))->Run(bb, insn);
      break;
    }
    case MOP_wcbz:
    case MOP_xcbz:
    case MOP_wcbnz:
    case MOP_xcbnz: {
      (static_cast<DeleteMovAfterCbzOrCbnzAArch64*>(optimizations[kDeleteMovAfterCbzOrCbnzOpt]))->Run(bb, insn);
      break;
    }
    case MOP_wstrh:
    case MOP_wstrb: {
      (static_cast<RemoveSxtBeforeStrAArch64*>(optimizations[kRemoveSxtBeforeStrOpt]))->Run(bb, insn);
      break;
    }
    case MOP_wmovrr:
    case MOP_xmovrr:
    case MOP_xvmovs:
    case MOP_xvmovd:
    case MOP_vmovuu:
    case MOP_vmovvv: {
      (static_cast<RemoveMovingtoSameRegAArch64*>(optimizations[kRemoveMovingtoSameRegOpt]))->Run(bb, insn);
      break;
    }
    default:
      break;
  }
}

void AArch64PrePeepHole::InitOpts() {
  optimizations.resize(kPeepholeOptsNum);
  optimizations[kOneHoleBranchesPreOpt] = optOwnMemPool->New<OneHoleBranchesPreAArch64>(cgFunc);
  optimizations[kReplaceOrrToMovOpt] = optOwnMemPool->New<ReplaceOrrToMovAArch64>(cgFunc);
  optimizations[kReplaceCmpToCmnOpt] = optOwnMemPool->New<ReplaceCmpToCmnAArch64>(cgFunc);
  optimizations[kComplexMemOperandOpt] = optOwnMemPool->New<ComplexMemOperandAArch64>(cgFunc);
  optimizations[kComplexMemOperandPreOptAdd] = optOwnMemPool->New<ComplexMemOperandPreAddAArch64>(cgFunc);
  optimizations[kComplexMemOperandOptLSL] = optOwnMemPool->New<ComplexMemOperandLSLAArch64>(cgFunc);
  optimizations[kComplexMemOperandOptLabel] = optOwnMemPool->New<ComplexMemOperandLabelAArch64>(cgFunc);
  optimizations[kDuplicateExtensionOpt] = optOwnMemPool->New<ElimDuplicateExtensionAArch64>(cgFunc);
  optimizations[kEnhanceStrLdrAArch64Opt] = optOwnMemPool->New<EnhanceStrLdrAArch64>(cgFunc);
}

void AArch64PrePeepHole::Run(BB &bb, Insn &insn) {
  MOperator thisMop = insn.GetMachineOpcode();
  switch (thisMop) {
    case MOP_wiorrri12:
    case MOP_xiorrri13: {
      (static_cast<ReplaceOrrToMovAArch64*>(optimizations[kReplaceOrrToMovOpt]))->Run(bb, insn);
      break;
    }
    case MOP_xmovri32:
    case MOP_xmovri64: {
      (static_cast<ReplaceCmpToCmnAArch64*>(optimizations[kReplaceCmpToCmnOpt]))->Run(bb, insn);
      break;
    }
    case MOP_xadrpl12: {
      (static_cast<ComplexMemOperandAArch64*>(optimizations[kComplexMemOperandOpt]))->Run(bb, insn);
      break;
    }
    case MOP_xaddrrr: {
      (static_cast<ComplexMemOperandPreAddAArch64*>(optimizations[kComplexMemOperandPreOptAdd]))->Run(bb, insn);
      break;
    }
    case MOP_xaddrrrs: {
      (static_cast<ComplexMemOperandLSLAArch64*>(optimizations[kComplexMemOperandOptLSL]))->Run(bb, insn);
      break;
    }
    case MOP_xsxtb32:
    case MOP_xsxth32:
    case MOP_xsxtb64:
    case MOP_xsxth64:
    case MOP_xsxtw64:
    case MOP_xuxtb32:
    case MOP_xuxth32:
    case MOP_xuxtw64: {
      (static_cast<ElimDuplicateExtensionAArch64*>(optimizations[kDuplicateExtensionOpt]))->Run(bb, insn);
      break;
    }
    case MOP_xldli: {
      (static_cast<ComplexMemOperandLabelAArch64*>(optimizations[kComplexMemOperandOptLabel]))->Run(bb, insn);
      break;
    }
    case MOP_xldr:
    case MOP_xstr:
    case MOP_wldr:
    case MOP_wstr:
    case MOP_dldr:
    case MOP_dstr:
    case MOP_sldr:
    case MOP_sstr: {
      (static_cast<EnhanceStrLdrAArch64*>(optimizations[kEnhanceStrLdrAArch64Opt]))->Run(bb, insn);
      break;
    }
    default:
      break;
  }
  if (&insn == bb.GetLastInsn()) {
    (static_cast<OneHoleBranchesPreAArch64*>(optimizations[kOneHoleBranchesPreOpt]))->Run(bb, insn);
  }
}

void AArch64PrePeepHole1::InitOpts() {
  optimizations.resize(kPeepholeOptsNum);
  optimizations[kOneHoleBranchesOpt] = optOwnMemPool->New<OneHoleBranchesAArch64>(cgFunc);
  optimizations[kAndCmpBranchesToTbzOpt] = optOwnMemPool->New<AndCmpBranchesToTbzAArch64>(cgFunc);
  optimizations[kComplexExtendWordLslOpt] = optOwnMemPool->New<ComplexExtendWordLslAArch64>(cgFunc);
}

void AArch64PrePeepHole1::Run(BB &bb, Insn &insn) {
  MOperator thisMop = insn.GetMachineOpcode();
  switch (thisMop) {
    case MOP_xsxtw64:
    case MOP_xuxtw64: {
      (static_cast<ComplexExtendWordLslAArch64*>(optimizations[kComplexExtendWordLslOpt]))->Run(bb, insn);
      break;
    }
    default:
      break;
  }
  if (&insn == bb.GetLastInsn()) {
    switch (thisMop) {
      case MOP_wcbz:
      case MOP_wcbnz:
      case MOP_xcbz:
      case MOP_xcbnz: {
        (static_cast<OneHoleBranchesAArch64*>(optimizations[kOneHoleBranchesOpt]))->Run(bb, insn);
        break;
      }
      case MOP_beq:
      case MOP_bne: {
        (static_cast<AndCmpBranchesToTbzAArch64*>(optimizations[kAndCmpBranchesToTbzOpt]))->Run(bb, insn);
        break;
      }
      default:
        break;
    }
  }
}

bool RemoveIdenticalLoadAndStorePattern::CheckCondition(Insn &insn) {
  nextInsn = insn.GetNextMachineInsn();
  if (nextInsn == nullptr) {
    return false;
  }
  return true;
}

void RemoveIdenticalLoadAndStorePattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  MOperator mop1 = insn.GetMachineOpcode();
  MOperator mop2 = nextInsn->GetMachineOpcode();
  if ((mop1 == MOP_wstr && mop2 == MOP_wstr) || (mop1 == MOP_xstr && mop2 == MOP_xstr)) {
    if (IsMemOperandsIdentical(insn, *nextInsn)) {
      bb.RemoveInsn(insn);
    }
  } else if ((mop1 == MOP_wstr && mop2 == MOP_wldr) || (mop1 == MOP_xstr && mop2 == MOP_xldr)) {
    if (IsMemOperandsIdentical(insn, *nextInsn)) {
      bb.RemoveInsn(*nextInsn);
    }
  }
}

bool RemoveIdenticalLoadAndStorePattern::IsMemOperandsIdentical(const Insn &insn1, const Insn &insn2) const {
  regno_t regNO1 = static_cast<RegOperand&>(insn1.GetOperand(kInsnFirstOpnd)).GetRegisterNumber();
  regno_t regNO2 = static_cast<RegOperand&>(insn2.GetOperand(kInsnFirstOpnd)).GetRegisterNumber();
  if (regNO1 != regNO2) {
    return false;
  }
  /* Match only [base + offset] */
  auto &memOpnd1 = static_cast<MemOperand&>(insn1.GetOperand(kInsnSecondOpnd));
  if (memOpnd1.GetAddrMode() != MemOperand::kAddrModeBOi || !memOpnd1.IsIntactIndexed()) {
    return false;
  }
  auto &memOpnd2 = static_cast<MemOperand&>(insn2.GetOperand(kInsnSecondOpnd));
  if (memOpnd2.GetAddrMode() != MemOperand::kAddrModeBOi || !memOpnd1.IsIntactIndexed()) {
    return false;
  }
  Operand *base1 = memOpnd1.GetBaseRegister();
  Operand *base2 = memOpnd2.GetBaseRegister();
  if (!((base1 != nullptr) && base1->IsRegister()) || !((base2 != nullptr) && base2->IsRegister())) {
    return false;
  }

  regno_t baseRegNO1 = static_cast<RegOperand*>(base1)->GetRegisterNumber();
  /* First insn re-write base addr   reg1 <- [ reg1 + offset ] */
  if (baseRegNO1 == regNO1) {
    return false;
  }

  regno_t baseRegNO2 = static_cast<RegOperand*>(base2)->GetRegisterNumber();
  if (baseRegNO1 != baseRegNO2) {
    return false;
  }

  return memOpnd1.GetOffsetImmediate()->GetOffsetValue() == memOpnd2.GetOffsetImmediate()->GetOffsetValue();
}

void RemoveIdenticalLoadAndStoreAArch64::Run(BB &bb, Insn &insn) {
  Insn *nextInsn = insn.GetNextMachineInsn();
  if (nextInsn == nullptr) {
    return;
  }
  MOperator mop1 = insn.GetMachineOpcode();
  MOperator mop2 = nextInsn->GetMachineOpcode();
  if ((mop1 == MOP_wstr && mop2 == MOP_wstr) || (mop1 == MOP_xstr && mop2 == MOP_xstr)) {
    if (IsMemOperandsIdentical(insn, *nextInsn)) {
      bb.RemoveInsn(insn);
    }
  } else if ((mop1 == MOP_wstr && mop2 == MOP_wldr) || (mop1 == MOP_xstr && mop2 == MOP_xldr)) {
    if (IsMemOperandsIdentical(insn, *nextInsn)) {
      bb.RemoveInsn(*nextInsn);
    }
  }
}

bool RemoveIdenticalLoadAndStoreAArch64::IsMemOperandsIdentical(const Insn &insn1, const Insn &insn2) const {
  regno_t regNO1 = static_cast<RegOperand&>(insn1.GetOperand(kInsnFirstOpnd)).GetRegisterNumber();
  regno_t regNO2 = static_cast<RegOperand&>(insn2.GetOperand(kInsnFirstOpnd)).GetRegisterNumber();
  if (regNO1 != regNO2) {
    return false;
  }
  /* Match only [base + offset] */
  auto &memOpnd1 = static_cast<MemOperand&>(insn1.GetOperand(kInsnSecondOpnd));
  if (memOpnd1.GetAddrMode() != MemOperand::kAddrModeBOi || !memOpnd1.IsIntactIndexed()) {
    return false;
  }
  auto &memOpnd2 = static_cast<MemOperand&>(insn2.GetOperand(kInsnSecondOpnd));
  if (memOpnd2.GetAddrMode() != MemOperand::kAddrModeBOi || !memOpnd1.IsIntactIndexed()) {
    return false;
  }
  Operand *base1 = memOpnd1.GetBaseRegister();
  Operand *base2 = memOpnd2.GetBaseRegister();
  if (!((base1 != nullptr) && base1->IsRegister()) || !((base2 != nullptr) && base2->IsRegister())) {
    return false;
  }

  regno_t baseRegNO1 = static_cast<RegOperand*>(base1)->GetRegisterNumber();
  /* First insn re-write base addr   reg1 <- [ reg1 + offset ] */
  if (baseRegNO1 == regNO1) {
    return false;
  }

  regno_t baseRegNO2 = static_cast<RegOperand*>(base2)->GetRegisterNumber();
  if (baseRegNO1 != baseRegNO2) {
    return false;
  }

  return memOpnd1.GetOffsetImmediate()->GetOffsetValue() == memOpnd2.GetOffsetImmediate()->GetOffsetValue();
}

bool RemoveMovingtoSameRegPattern::CheckCondition(Insn &insn) {
  ASSERT(insn.GetOperand(kInsnFirstOpnd).IsRegister(), "expects registers");
  ASSERT(insn.GetOperand(kInsnSecondOpnd).IsRegister(), "expects registers");
  auto &reg1 = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  auto &reg2 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  /* remove mov x0,x0 when it cast i32 to i64 */
  if ((reg1.GetRegisterNumber() == reg2.GetRegisterNumber()) && (reg1.GetSize() >= reg2.GetSize())) {
    return true;
  }
  return false;
}

void RemoveMovingtoSameRegPattern::Run(BB &bb, Insn &insn) {
  /* remove mov x0,x0 when it cast i32 to i64 */
  if (CheckCondition(insn)) {
    bb.RemoveInsn(insn);
  }
}

void RemoveMovingtoSameRegAArch64::Run(BB &bb, Insn &insn) {
  ASSERT(insn.GetOperand(kInsnFirstOpnd).IsRegister(), "expects registers");
  ASSERT(insn.GetOperand(kInsnSecondOpnd).IsRegister(), "expects registers");
  auto &reg1 = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  auto &reg2 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  /* remove mov x0,x0 when it cast i32 to i64 */
  if ((reg1.GetRegisterNumber() == reg2.GetRegisterNumber()) && (reg1.GetSize() >= reg2.GetSize())) {
    bb.RemoveInsn(insn);
  }
}

void EnhanceStrLdrAArch64::Run(BB &bb, Insn &insn) {
  Insn *prevInsn = insn.GetPrev();
  if (!cgFunc.GetMirModule().IsCModule()) {
    return;
  }

  if (prevInsn == nullptr) {
    return;
  }
  Operand &memOpnd = insn.GetOperand(kInsnSecondOpnd);
  CHECK_FATAL(memOpnd.GetKind() == Operand::kOpdMem, "Unexpected operand in EnhanceStrLdrAArch64");
  auto &a64MemOpnd = static_cast<MemOperand&>(memOpnd);
  RegOperand *baseOpnd = a64MemOpnd.GetBaseRegister();
  MOperator prevMop = prevInsn->GetMachineOpcode();
  if (IsEnhanceAddImm(prevMop) && a64MemOpnd.GetAddrMode() == MemOperand::kAddrModeBOi &&
      a64MemOpnd.GetOffsetImmediate()->GetValue() == 0) {
    auto &addDestOpnd = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
    if (baseOpnd == &addDestOpnd && !IfOperandIsLiveAfterInsn(addDestOpnd, insn)) {
      auto &concreteMemOpnd = static_cast<MemOperand&>(memOpnd);
      auto *origBaseReg = concreteMemOpnd.GetBaseRegister();
      concreteMemOpnd.SetBaseRegister(
          static_cast<RegOperand&>(prevInsn->GetOperand(kInsnSecondOpnd)));
      auto &ofstOpnd = static_cast<ImmOperand&>(prevInsn->GetOperand(kInsnThirdOpnd));
      OfstOperand &offOpnd = static_cast<AArch64CGFunc&>(cgFunc).GetOrCreateOfstOpnd(
          static_cast<uint64>(ofstOpnd.GetValue()), k32BitSize);
      offOpnd.SetVary(ofstOpnd.GetVary());
      auto *origOffOpnd = concreteMemOpnd.GetOffsetImmediate();
      concreteMemOpnd.SetOffsetOperand(offOpnd);
      if (!static_cast<AArch64CGFunc&>(cgFunc).IsOperandImmValid(insn.GetMachineOpcode(), &memOpnd, kInsnSecondOpnd)) {
        // If new offset is invalid, undo it
        concreteMemOpnd.SetBaseRegister(*static_cast<RegOperand*>(origBaseReg));
        concreteMemOpnd.SetOffsetOperand(*origOffOpnd);
        return;
      }
      bb.RemoveInsn(*prevInsn);
    }
  }
}

bool EnhanceStrLdrAArch64::IsEnhanceAddImm(MOperator prevMop) const {
  return prevMop == MOP_xaddrri12 ||  prevMop == MOP_waddrri12;
}

bool IsSameRegisterOperation(const RegOperand &desMovOpnd,
                             const RegOperand &uxtDestOpnd,
                             const RegOperand &uxtFromOpnd) {
  return ((desMovOpnd.GetRegisterNumber() == uxtDestOpnd.GetRegisterNumber()) &&
          (uxtDestOpnd.GetRegisterNumber() == uxtFromOpnd.GetRegisterNumber()));
}

bool CombineContiLoadAndStorePattern::IsRegNotSameMemUseInInsn(const Insn &insn, regno_t regNO, bool isStore,
                                                               int64 baseOfst) const {
  uint32 opndNum = insn.GetOperandSize();
  bool sameMemAccess = false; /* both store or load */
  if (insn.IsStore() == isStore) {
    sameMemAccess = true;
  }
  for (uint32 i = 0; i < opndNum; ++i) {
    Operand &opnd = insn.GetOperand(i);
    if (opnd.IsList()) {
      auto &listOpnd = static_cast<ListOperand&>(opnd);
      for (auto listElem : listOpnd.GetOperands()) {
        RegOperand *regOpnd = static_cast<RegOperand*>(listElem);
        ASSERT(regOpnd != nullptr, "parameter operand must be RegOperand");
        if (regNO == regOpnd->GetRegisterNumber()) {
          return true;
        }
      }
    } else if (opnd.IsMemoryAccessOperand()) {
      auto &memOperand = static_cast<MemOperand&>(opnd);
      RegOperand *base = memOperand.GetBaseRegister();
      /* need check offset as well */
      regno_t stackBaseRegNO = cgFunc->UseFP() ? R29 : RSP;
      if (!sameMemAccess && base != nullptr) {
        regno_t curBaseRegNO = base->GetRegisterNumber();
        int64 memBarrierRange = static_cast<int64>(insn.IsLoadStorePair() ? k16BitSize : k8BitSize);
        if (!(curBaseRegNO == regNO && memOperand.GetAddrMode() == MemOperand::kAddrModeBOi &&
            memOperand.GetOffsetImmediate() != nullptr &&
            (memOperand.GetOffsetImmediate()->GetOffsetValue() <= (baseOfst - memBarrierRange) ||
            memOperand.GetOffsetImmediate()->GetOffsetValue() >= (baseOfst + memBarrierRange)))) {
          return true;
        }
      }
      /* do not trust the following situation :
       * str x1, [x9]
       * str x6, [x2]
       * str x3, [x9, #8]
       */
      if (isStore && regNO != stackBaseRegNO && base != nullptr &&
          base->GetRegisterNumber() != stackBaseRegNO && base->GetRegisterNumber() != regNO) {
        return true;
      }
      if (isStore && base != nullptr && base->GetRegisterNumber() == regNO) {
        if (memOperand.GetAddrMode() == MemOperand::kAddrModeBOi && memOperand.GetOffsetImmediate() != nullptr) {
          int64 curOffset = memOperand.GetOffsetImmediate()->GetOffsetValue();
          if (memOperand.GetSize() == k64BitSize) {
            uint32 memBarrierRange = insn.IsLoadStorePair() ? k16BitSize : k8BitSize;
            if (curOffset < baseOfst + memBarrierRange && curOffset > baseOfst - static_cast<int32>(memBarrierRange)) {
              return true;
            }
          } else if (memOperand.GetSize() == k32BitSize) {
            uint32 memBarrierRange = insn.IsLoadStorePair() ? k8BitSize : k4BitSize;
            if (curOffset < baseOfst + memBarrierRange && curOffset > baseOfst - memBarrierRange) {
              return true;
            }
          }
        }
      }
    } else if (opnd.IsConditionCode()) {
      Operand &rflagOpnd = cgFunc->GetOrCreateRflag();
      RegOperand &rflagReg = static_cast<RegOperand&>(rflagOpnd);
      if (rflagReg.GetRegisterNumber() == regNO) {
        return true;
      }
    } else if (opnd.IsRegister()) {
      if (!isStore && static_cast<RegOperand&>(opnd).GetRegisterNumber() == regNO) {
        return true;
      }
    }
  }
  return false;
}

std::vector<Insn*> CombineContiLoadAndStorePattern::FindPrevStrLdr(Insn &insn, regno_t destRegNO,
                                                                   regno_t memBaseRegNO, int64 baseOfst) const {
  std::vector<Insn*> prevContiInsns;
  bool isStr = insn.IsStore();
  for (Insn *curInsn = insn.GetPrev(); curInsn != nullptr; curInsn = curInsn->GetPrev()) {
    if (!curInsn->IsMachineInstruction()) {
      continue;
    }
    if (curInsn->IsRegDefined(memBaseRegNO)) {
      return prevContiInsns;
    }
    if (IsRegNotSameMemUseInInsn(*curInsn, memBaseRegNO, insn.IsStore(), static_cast<int32>(baseOfst))) {
      return prevContiInsns;
    }
    /* return continuous STD/LDR insn */
    if (((isStr && curInsn->IsStore()) || (!isStr && curInsn->IsLoad())) && !curInsn->IsLoadStorePair()) {
      auto *memOperand = static_cast<MemOperand*>(curInsn->GetMemOpnd());
      /* do not combine ldr r0, label */
      if (memOperand != nullptr) {
        auto *baseRegOpnd = static_cast<RegOperand*>(memOperand->GetBaseRegister());
        ASSERT(baseRegOpnd == nullptr || !baseRegOpnd->IsVirtualRegister(),
            "physical register has not been allocated?");
        if (memOperand->GetAddrMode() == MemOperand::kAddrModeBOi &&
            baseRegOpnd->GetRegisterNumber() == memBaseRegNO) {
          prevContiInsns.emplace_back(curInsn);
        }
      }
    }
    /* check insn that changes the data flow */
    regno_t stackBaseRegNO = cgFunc->UseFP() ? R29 : RSP;
    /* ldr x8, [x21, #8]
     * call foo()
     * ldr x9, [x21, #16]
     * although x21 is a calleeSave register, there is no guarantee data in memory [x21] is not changed
     */
    if (curInsn->IsCall() && (!AArch64Abi::IsCalleeSavedReg(static_cast<AArch64reg>(destRegNO)) ||
        memBaseRegNO != stackBaseRegNO)) {
      return prevContiInsns;
    }
    /* store opt should not cross call due to stack args */
    if (curInsn->IsCall() && isStr) {
      return prevContiInsns;
    }
    if (curInsn->GetMachineOpcode() == MOP_asm) {
      return prevContiInsns;
    }
    if (static_cast<AArch64Insn*>(curInsn)->IsRegDefOrUse(destRegNO)) {
      return prevContiInsns;
    }
  }
  return prevContiInsns;
}

Insn *CombineContiLoadAndStorePattern::FindValidSplitAddInsn(Insn &combineInsn, const RegOperand &baseOpnd) const {
  Insn *splitAdd = nullptr;
  for (Insn *cursor = combineInsn.GetPrev(); cursor != nullptr; cursor = cursor->GetPrev()) {
    if (!cursor->IsMachineInstruction()) {
      continue;
    }
    if (cursor->IsCall()) {
      break;
    }
    if (cursor->IsRegDefined(baseOpnd.GetRegisterNumber())) {
      break;
    }
    MOperator mOp = cursor->GetMachineOpcode();
    if (mOp != MOP_xaddrri12 && mOp != MOP_waddrri12) {
      continue;
    }
    auto &destOpnd = static_cast<RegOperand&>(cursor->GetOperand(kInsnFirstOpnd));
    if (destOpnd.GetRegisterNumber() != R16 || destOpnd.GetSize() != baseOpnd.GetSize()) {
      continue;
    }
    auto &useOpnd = static_cast<RegOperand&>(cursor->GetOperand(kInsnSecondOpnd));
    /*
     * split add as following:
     * add R16, R0, #2, LSL #12
     * add R16, R16, #1536
     */
    if (useOpnd.GetRegisterNumber() != baseOpnd.GetRegisterNumber()) {
      if (useOpnd.GetRegisterNumber() == R16) {
        Insn *defInsn = cursor->GetPrev();
        CHECK_FATAL(defInsn != nullptr, "invalid defInsn");
        CHECK_FATAL(defInsn->GetMachineOpcode() == MOP_xaddrri24 || defInsn->GetMachineOpcode() == MOP_waddrri24,
                    "split with wrong add");
        auto &opnd = static_cast<RegOperand&>(defInsn->GetOperand(kInsnSecondOpnd));
        if (opnd.GetRegisterNumber() == baseOpnd.GetRegisterNumber()) {
          splitAdd = cursor;
        }
      }
      break;
    } else {
      splitAdd = cursor;
      break;
    }
  }
  return splitAdd;
}

bool CombineContiLoadAndStorePattern::FindTmpRegOnlyUseAfterCombineInsn(const Insn &curInsn) const {
  /*
   * avoid the case as following:
   *   add R16, R20, #28672
   *   stp R1, R2, [R16, #408]
   *   ldr R1, [R0, #14032]         add R16, R0, #3, LSL #12
   *                      ====>     add R16, R16, #1536     (this r16 will clobber use of ldp R2, R3)
   *                                ldp R1, R0, [R16, #208]
   *   ldp R2, R3, [R16, #424]
   *   ldr R0, [R0, #14036]
   */
  for (Insn *cursor = curInsn.GetNext(); cursor != nullptr; cursor = cursor->GetNext()) {
    MOperator mOp = cursor->GetMachineOpcode();
    if (mOp == MOP_xaddrri12 || mOp == MOP_waddrri12 || mOp == MOP_xaddrri24 || mOp == MOP_waddrri24) {
      auto &destOpnd = static_cast<RegOperand&>(cursor->GetOperand(kInsnFirstOpnd));
      if (destOpnd.GetRegisterNumber() == R16) {
        return false;
      }
    }
    if (!cursor->IsLoad() && !cursor->IsStore() && !cursor->IsLoadStorePair()) {
      continue;
    }
    if (cursor->IsLoadLabel() || cursor->IsLoadAddress()) {
      continue;
    }
    uint32 memIdx = (cursor->IsLoadStorePair() ? kInsnThirdOpnd : kInsnSecondOpnd);
    auto &curMemOpnd = static_cast<MemOperand&>(cursor->GetOperand(memIdx));
    RegOperand *baseOpnd = curMemOpnd.GetBaseRegister();
    if (baseOpnd != nullptr && baseOpnd->GetRegisterNumber() == R16) {
      return true;
    }
  }
  return false;
}

bool CombineContiLoadAndStorePattern::PlaceSplitAddInsn(const Insn &curInsn, Insn &combineInsn,
                                                        const MemOperand &memOperand,
                                                        RegOperand &baseOpnd, uint32 bitLen) const {
  Insn *cursor = nullptr;
  MemOperand *maxOfstMem = nullptr;
  int64 maxOfstVal = 0;
  MOperator mop = curInsn.GetMachineOpcode();
  OfstOperand *ofstOpnd = memOperand.GetOffsetImmediate();
  int64 ofstVal = ofstOpnd->GetOffsetValue();
  auto &aarFunc = static_cast<AArch64CGFunc&>(*cgFunc);
  for (cursor = curInsn.GetNext(); cursor != nullptr; cursor = cursor->GetNext()) {
    if (!cursor->IsMachineInstruction()) {
      continue;
    }
    if (cursor->GetMachineOpcode() == mop && (cursor->IsLoad() || cursor->IsStore())) {
      auto &curMemOpnd = static_cast<MemOperand&>(cursor->GetOperand(kInsnSecondOpnd));
      RegOperand *curBaseOpnd = curMemOpnd.GetBaseRegister();
      if (curMemOpnd.GetAddrMode() == MemOperand::kAddrModeBOi && RegOperand::IsSameReg(baseOpnd, *curBaseOpnd)) {
        OfstOperand *curOfstOpnd = curMemOpnd.GetOffsetImmediate();
        CHECK_FATAL(curOfstOpnd != nullptr, "invalid OfstOperand");
        if (curOfstOpnd->GetOffsetValue() > ofstVal &&
            (curOfstOpnd->GetOffsetValue() - ofstVal) < MemOperand::GetMaxPairPIMM(bitLen) &&
            !aarFunc.IsOperandImmValid(combineInsn.GetMachineOpcode(), &curMemOpnd, kInsnThirdOpnd)) {
          maxOfstMem = &curMemOpnd;
          maxOfstVal = curOfstOpnd->GetOffsetValue();
        }
      }
    }
    if (cursor->IsRegDefined(baseOpnd.GetRegisterNumber())) {
      break;
    }
    if (cursor->IsRegDefined(R16)) {
      break;
    }
  }
  MemOperand *newMemOpnd = nullptr;
  if (maxOfstMem == nullptr) {
    newMemOpnd = &aarFunc.SplitOffsetWithAddInstruction(memOperand, bitLen, static_cast<AArch64reg>(R16),
                                                        false, &combineInsn, true);
  } else {
    RegOperand *addResOpnd = aarFunc.GetBaseRegForSplit(R16);
    ImmOperand &immAddend = aarFunc.SplitAndGetRemained(*maxOfstMem, bitLen, maxOfstVal, true);
    newMemOpnd = &aarFunc.CreateReplacementMemOperand(bitLen, *addResOpnd, ofstVal - immAddend.GetValue());
    if (!(aarFunc.IsOperandImmValid(combineInsn.GetMachineOpcode(), newMemOpnd, kInsnThirdOpnd))) {
      newMemOpnd = &aarFunc.SplitOffsetWithAddInstruction(memOperand, bitLen, static_cast<AArch64reg>(R16),
                                                          false, &combineInsn, true);
    } else {
      aarFunc.SelectAddAfterInsn(*addResOpnd, baseOpnd, immAddend, PTY_i64, false, combineInsn);
    }
  }
  if (!(aarFunc.IsOperandImmValid(combineInsn.GetMachineOpcode(), newMemOpnd, kInsnThirdOpnd))) {
    return false;
  }
  combineInsn.SetOperand(kInsnThirdOpnd, *newMemOpnd);
  return true;
}

bool CombineContiLoadAndStorePattern::SplitOfstWithAddToCombine(const Insn &curInsn, Insn &combineInsn,
                                                                const MemOperand &memOperand) const {
  auto *baseRegOpnd = static_cast<RegOperand*>(memOperand.GetBaseRegister());
  auto *ofstOpnd = static_cast<OfstOperand*>(memOperand.GetOffsetImmediate());
  ASSERT(baseRegOpnd && ofstOpnd, "get baseOpnd and ofstOpnd failed");
  CHECK_FATAL(combineInsn.GetOperand(kInsnFirstOpnd).GetSize() == combineInsn.GetOperand(kInsnSecondOpnd).GetSize(),
              "the size must equal");
  if (baseRegOpnd->GetRegisterNumber() == R16) {
    return false;
  }
  Insn *splitAdd = FindValidSplitAddInsn(combineInsn, *baseRegOpnd);
  const AArch64MD *md = &AArch64CG::kMd[combineInsn.GetMachineOpcode()];
  auto *opndProp = md->operand[kInsnFirstOpnd];
  auto &aarFunc = static_cast<AArch64CGFunc&>(*cgFunc);
  if (splitAdd == nullptr) {
    if (combineInsn.IsLoadStorePair() && ofstOpnd->GetOffsetValue() < 0) {
      return false; /* do not split */
    }
    if (FindTmpRegOnlyUseAfterCombineInsn(combineInsn)) {
      return false;
    }
    /* create and place addInsn */
    return PlaceSplitAddInsn(curInsn, combineInsn, memOperand, *baseRegOpnd, opndProp->GetSize());
  } else {
    auto &newBaseReg = static_cast<RegOperand&>(splitAdd->GetOperand(kInsnFirstOpnd));
    auto &addImmOpnd = static_cast<ImmOperand&>(splitAdd->GetOperand(kInsnThirdOpnd));
    int64 addVal = 0;
    if (static_cast<RegOperand&>(splitAdd->GetOperand(kInsnSecondOpnd)).GetRegisterNumber() == R16) {
      Insn *defInsn = splitAdd->GetPrev();
      CHECK_FATAL(defInsn->GetMachineOpcode() == MOP_xaddrri24 || defInsn->GetMachineOpcode() == MOP_waddrri24,
                  "split with wrong add");
      auto &immOpnd = static_cast<ImmOperand&>(defInsn->GetOperand(kInsnThirdOpnd));
      auto &shiftOpnd = static_cast<LogicalShiftLeftOperand&>(defInsn->GetOperand(kInsnFourthOpnd));
      addVal = static_cast<int64>((static_cast<uint64>(immOpnd.GetValue()) << shiftOpnd.GetShiftAmount())) +
          addImmOpnd.GetValue();
    } else {
      addVal = addImmOpnd.GetValue();
    }
    auto *newOfstOpnd = &aarFunc.CreateOfstOpnd(static_cast<uint64>(ofstOpnd->GetOffsetValue() - addVal),
        ofstOpnd->GetSize());
    auto *newMemOpnd = aarFunc.CreateMemOperand(MemOperand::kAddrModeBOi, opndProp->GetSize(),
        newBaseReg, nullptr, newOfstOpnd, memOperand.GetSymbol());
    if (!(static_cast<AArch64CGFunc&>(*cgFunc).IsOperandImmValid(combineInsn.GetMachineOpcode(), newMemOpnd,
                                                                 kInsnThirdOpnd))) {
      return PlaceSplitAddInsn(curInsn, combineInsn, memOperand, *baseRegOpnd, opndProp->GetSize());
    }
    combineInsn.SetOperand(kInsnThirdOpnd, *newMemOpnd);
    return true;
  }
}

bool CombineContiLoadAndStorePattern::CheckCondition(Insn &insn) {
  memOpnd = static_cast<MemOperand*>(insn.GetMemOpnd());
  ASSERT(memOpnd != nullptr, "get mem operand failed");
  if (memOpnd->GetAddrMode() != MemOperand::kAddrModeBOi) {
    return false;
  }
  if (!doAggressiveCombine) {
    return false;
  }
  return true;
}

/* Combining 2 STRs into 1 stp or 2 LDRs into 1 ldp */
void CombineContiLoadAndStorePattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  MOperator thisMop = insn.GetMachineOpcode();
  ASSERT(insn.GetOperand(kInsnFirstOpnd).IsRegister(), "unexpect operand");
  auto &destOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  auto *baseRegOpnd = static_cast<RegOperand*>(memOpnd->GetBaseRegister());
  OfstOperand *offsetOpnd = memOpnd->GetOffsetImmediate();
  CHECK_FATAL(offsetOpnd != nullptr, "offset opnd lost");
  ASSERT(baseRegOpnd == nullptr || !baseRegOpnd->IsVirtualRegister(), "physical register has not been allocated?");
  std::vector<Insn*> prevContiInsnVec = FindPrevStrLdr(
      insn, destOpnd.GetRegisterNumber(), baseRegOpnd->GetRegisterNumber(), offsetOpnd->GetOffsetValue());
  for (auto prevContiInsn : prevContiInsnVec) {
    ASSERT(prevContiInsn != nullptr, "get previous consecutive instructions failed");
    auto *prevMemOpnd = static_cast<MemOperand*>(prevContiInsn->GetMemOpnd());
    if (memOpnd->GetIndexOpt() != prevMemOpnd->GetIndexOpt()) {
      continue;
    }
    OfstOperand *prevOffsetOpnd = prevMemOpnd->GetOffsetImmediate();
    CHECK_FATAL(offsetOpnd != nullptr && prevOffsetOpnd != nullptr, "both conti str/ldr have no offset");
    auto &prevDestOpnd = static_cast<RegOperand&>(prevContiInsn->GetOperand(kInsnFirstOpnd));
    uint32 memSize = static_cast<const AArch64Insn&>(insn).GetLoadStoreSize();
    uint32 prevMemSize = static_cast<const AArch64Insn&>(*prevContiInsn).GetLoadStoreSize();
    if (prevDestOpnd.GetRegisterType() != destOpnd.GetRegisterType()) {
      continue;
    }
    int64 offsetVal = offsetOpnd->GetOffsetValue();
    int64 prevOffsetVal = prevOffsetOpnd->GetOffsetValue();
    auto diffVal = std::abs(offsetVal - prevOffsetVal);
    regno_t destRegNO = destOpnd.GetRegisterNumber();
    regno_t prevDestRegNO = prevDestOpnd.GetRegisterNumber();
    if (insn.IsStore() && memOpnd->IsStackArgMem() && prevMemOpnd->IsStackArgMem() &&
        (memSize == k4ByteSize || memSize == k8ByteSize) && diffVal == k8BitSize &&
        (prevMemSize == k4ByteSize || prevMemSize == k8ByteSize) &&
        (destOpnd.GetValidBitsNum() == memSize * k8BitSize) &&
        (prevDestOpnd.GetValidBitsNum() == prevMemSize * k8BitSize)) {
      RegOperand &newDest = static_cast<AArch64CGFunc*>(cgFunc)->GetOrCreatePhysicalRegisterOperand(
          static_cast<AArch64reg>(destRegNO), k64BitSize, destOpnd.GetRegisterType());
      RegOperand &newPrevDest = static_cast<AArch64CGFunc*>(cgFunc)->GetOrCreatePhysicalRegisterOperand(
          static_cast<AArch64reg>(prevDestRegNO), k64BitSize, prevDestOpnd.GetRegisterType());
      MemOperand *combineMemOpnd = (offsetVal < prevOffsetVal) ? memOpnd : prevMemOpnd;
      CG *cg = cgFunc->GetCG();
      MOperator mopPair = (destOpnd.GetRegisterType() == kRegTyInt) ? MOP_xstp : MOP_dstp;
      if ((static_cast<AArch64CGFunc&>(*cgFunc).IsOperandImmValid(mopPair, combineMemOpnd, kInsnThirdOpnd))) {
        Insn &combineInsn = (offsetVal < prevOffsetVal) ?
                            cg->BuildInstruction<AArch64Insn>(mopPair, newDest, newPrevDest, *combineMemOpnd) :
                            cg->BuildInstruction<AArch64Insn>(mopPair, newPrevDest, newDest, *combineMemOpnd);
        bb.InsertInsnAfter(*prevContiInsn, combineInsn);
        RemoveInsnAndKeepComment(bb, insn, *prevContiInsn);
        return;
      }
    }
    if (memSize != prevMemSize  ||
        thisMop != prevContiInsn->GetMachineOpcode() || prevDestOpnd.GetSize() != destOpnd.GetSize()) {
      continue;
    }
    /* do combination str/ldr -> stp/ldp */
    if ((insn.IsStore() || destRegNO != prevDestRegNO) || (destRegNO == RZR && prevDestRegNO == RZR)) {
      if ((memSize == k8ByteSize && diffVal == k8BitSize) ||
          (memSize == k4ByteSize && diffVal == k4BitSize) ||
          (memSize == k16ByteSize && diffVal == k16BitSize)) {
        CG *cg = cgFunc->GetCG();
        MOperator mopPair = GetMopPair(thisMop);
        MemOperand *combineMemOpnd = (offsetVal < prevOffsetVal) ? memOpnd : prevMemOpnd;
        Insn &combineInsn = (offsetVal < prevOffsetVal) ?
                            cg->BuildInstruction<AArch64Insn>(mopPair, destOpnd, prevDestOpnd, *combineMemOpnd) :
                            cg->BuildInstruction<AArch64Insn>(mopPair, prevDestOpnd, destOpnd, *combineMemOpnd);
        bb.InsertInsnAfter(*prevContiInsn, combineInsn);
        if (!(static_cast<AArch64CGFunc&>(*cgFunc).IsOperandImmValid(mopPair, combineMemOpnd, kInsnThirdOpnd)) &&
            !SplitOfstWithAddToCombine(insn, combineInsn, *combineMemOpnd)) {
            bb.RemoveInsn(combineInsn);
            return;
        }
        RemoveInsnAndKeepComment(bb, insn, *prevContiInsn);
        return;
      }
    }
    /* do combination strb/ldrb -> strh/ldrh -> str/ldr */
    if (destRegNO == prevDestRegNO && destRegNO == RZR && prevDestRegNO == RZR) {
      if ((memSize == k1ByteSize && diffVal == k1ByteSize) ||  (memSize == k2ByteSize && diffVal == k2ByteSize)) {
        CG *cg = cgFunc->GetCG();
        MOperator mopPair = GetMopHigherByte(thisMop);
        if (offsetVal < prevOffsetVal) {
          if (static_cast<AArch64CGFunc&>(*cgFunc).IsOperandImmValid(mopPair, memOpnd, kInsnSecondOpnd)) {
            Insn &combineInsn = cg->BuildInstruction<AArch64Insn>(mopPair, destOpnd, *memOpnd);
            bb.InsertInsnAfter(*prevContiInsn, combineInsn);
            RemoveInsnAndKeepComment(bb, insn, *prevContiInsn);
            return;
          }
        } else {
          if (static_cast<AArch64CGFunc&>(*cgFunc).IsOperandImmValid(mopPair, prevMemOpnd, kInsnSecondOpnd)) {
            Insn &combineInsn = cg->BuildInstruction<AArch64Insn>(mopPair, prevDestOpnd, *prevMemOpnd);
            bb.InsertInsnAfter(*prevContiInsn, combineInsn);
            RemoveInsnAndKeepComment(bb, insn, *prevContiInsn);
            return;
          }
        }
      }
    }
  }
}

MOperator CombineContiLoadAndStorePattern::GetMopHigherByte(MOperator mop) const {
  switch (mop) {
    case MOP_wldrb:
      return MOP_wldrh;
    case MOP_wstrb:
      return MOP_wstrh;
    case MOP_wldrh:
      return MOP_wldr;
    case MOP_wstrh:
      return MOP_wstr;
    default:
      ASSERT(false, "should not run here");
      return MOP_undef;
  }
}

void CombineContiLoadAndStorePattern::RemoveInsnAndKeepComment(BB &bb, Insn &insn, Insn &prevInsn) const {
  /* keep the comment */
  Insn *nn = prevInsn.GetNextMachineInsn();
  std::string newComment = "";
  MapleString comment = insn.GetComment();
  if (comment.c_str() != nullptr && strlen(comment.c_str()) > 0) {
    newComment += comment.c_str();
  }
  comment = prevInsn.GetComment();
  if (comment.c_str() != nullptr && strlen(comment.c_str()) > 0) {
    newComment = newComment + "  " + comment.c_str();
  }
  if (newComment.c_str() != nullptr && strlen(newComment.c_str()) > 0) {
    ASSERT(nn != nullptr, "nn should not be nullptr");
    nn->SetComment(newComment);
  }
  bb.RemoveInsn(insn);
  bb.RemoveInsn(prevInsn);
}

void EliminateSpecifcSXTAArch64::Run(BB &bb, Insn &insn) {
  MOperator thisMop = insn.GetMachineOpcode();
  Insn *prevInsn = insn.GetPrev();
  while (prevInsn != nullptr && prevInsn->GetMachineOpcode() == 0) {
    prevInsn = prevInsn->GetPrev();
  }
  if (prevInsn == nullptr) {
    return;
  }
  auto &regOpnd0 = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  auto &regOpnd1 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  if (&insn != bb.GetFirstInsn() && regOpnd0.GetRegisterNumber() == regOpnd1.GetRegisterNumber() &&
      prevInsn->IsMachineInstruction()) {
    if (prevInsn->GetMachineOpcode() == MOP_xmovri32 || prevInsn->GetMachineOpcode() == MOP_xmovri64) {
      auto &dstMovOpnd = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
      if (dstMovOpnd.GetRegisterNumber() != regOpnd1.GetRegisterNumber()) {
        return;
      }
      Operand &opnd = prevInsn->GetOperand(kInsnSecondOpnd);
      if (opnd.IsIntImmediate()) {
        auto &immOpnd = static_cast<ImmOperand&>(opnd);
        int64 value = immOpnd.GetValue();
        if (thisMop == MOP_xsxtb32) {
          /* value should in range between -127 and 127 */
          if (value >= static_cast<int64>(0xFFFFFFFFFFFFFF80) && value <= 0x7F &&
              immOpnd.IsSingleInstructionMovable(regOpnd0.GetSize())) {
            bb.RemoveInsn(insn);
          }
        } else if (thisMop == MOP_xsxth32) {
          /* value should in range between -32678 and 32678 */
          if (value >= static_cast<int64>(0xFFFFFFFFFFFF8000) && value <= 0x7FFF &&
              immOpnd.IsSingleInstructionMovable(regOpnd0.GetSize())) {
            bb.RemoveInsn(insn);
          }
        } else {
          uint64 flag = 0xFFFFFFFFFFFFFF80; /* initialize the flag with fifty-nine 1s at top */
          if (thisMop == MOP_xsxth64) {
            flag = 0xFFFFFFFFFFFF8000;      /* specify the flag with forty-nine 1s at top in this case */
          } else if (thisMop == MOP_xsxtw64) {
            flag = 0xFFFFFFFF80000000;      /* specify the flag with thirty-three 1s at top in this case */
          }
          if ((static_cast<uint64>(value) & flag) == 0 && immOpnd.IsSingleInstructionMovable(regOpnd0.GetSize())) {
            auto *aarch64CGFunc = static_cast<AArch64CGFunc*>(&cgFunc);
            RegOperand &dstOpnd = aarch64CGFunc->GetOrCreatePhysicalRegisterOperand(
                static_cast<AArch64reg>(dstMovOpnd.GetRegisterNumber()), k64BitSize, dstMovOpnd.GetRegisterType());
            prevInsn->SetOperand(kInsnFirstOpnd, dstOpnd);
            prevInsn->SetMOP(MOP_xmovri64);
            bb.RemoveInsn(insn);
          }
        }
      }
    } else if (prevInsn->GetMachineOpcode() == MOP_wldrsb) {
      auto &dstMovOpnd = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
      if (dstMovOpnd.GetRegisterNumber() != regOpnd1.GetRegisterNumber()) {
        return;
      }
      if (thisMop == MOP_xsxtb32) {
        bb.RemoveInsn(insn);
      }
    } else if (prevInsn->GetMachineOpcode() == MOP_wldrsh) {
      auto &dstMovOpnd = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
      if (dstMovOpnd.GetRegisterNumber() != regOpnd1.GetRegisterNumber()) {
        return;
      }
      if (thisMop == MOP_xsxth32) {
        bb.RemoveInsn(insn);
      }
    }
  }
}

void EliminateSpecifcUXTAArch64::Run(BB &bb, Insn &insn) {
  MOperator thisMop = insn.GetMachineOpcode();
  Insn *prevInsn = insn.GetPreviousMachineInsn();
  if (prevInsn == nullptr) {
    return;
  }
  auto &regOpnd0 = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  auto &regOpnd1 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  if (prevInsn->IsCall() &&
      prevInsn->GetIsCallReturnUnsigned() &&
      regOpnd0.GetRegisterNumber() == regOpnd1.GetRegisterNumber() &&
      (regOpnd1.GetRegisterNumber() == R0 || regOpnd1.GetRegisterNumber() == V0)) {
    uint32 retSize = prevInsn->GetRetSize();
    if (retSize > 0 &&
        ((thisMop == MOP_xuxtb32 && retSize <= k1ByteSize) ||
         (thisMop == MOP_xuxth32 && retSize <= k2ByteSize) ||
         (thisMop == MOP_xuxtw64 && retSize <= k4ByteSize))) {
      bb.RemoveInsn(insn);
    }
    return;
  }
  if (&insn == bb.GetFirstInsn() || regOpnd0.GetRegisterNumber() != regOpnd1.GetRegisterNumber() ||
      !prevInsn->IsMachineInstruction()) {
    return;
  }
  if (cgFunc.GetMirModule().GetSrcLang() == kSrcLangC && prevInsn->IsCall() && prevInsn->GetIsCallReturnSigned()) {
    return;
  }
  if (thisMop == MOP_xuxtb32) {
    if (prevInsn->GetMachineOpcode() == MOP_xmovri32 || prevInsn->GetMachineOpcode() == MOP_xmovri64) {
      auto &dstMovOpnd = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
      if (!IsSameRegisterOperation(dstMovOpnd, regOpnd1, regOpnd0)) {
        return;
      }
      Operand &opnd = prevInsn->GetOperand(kInsnSecondOpnd);
      if (opnd.IsIntImmediate()) {
        auto &immOpnd = static_cast<ImmOperand&>(opnd);
        int64 value = immOpnd.GetValue();
        /* check the top 56 bits of value */
        if ((static_cast<uint64>(value) & 0xFFFFFFFFFFFFFF00) == 0) {
          bb.RemoveInsn(insn);
        }
      }
    } else if (prevInsn->GetMachineOpcode() == MOP_wldrb) {
      auto &dstOpnd = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
      if (dstOpnd.GetRegisterNumber() != regOpnd1.GetRegisterNumber()) {
        return;
      }
      bb.RemoveInsn(insn);
    }
  } else if (thisMop == MOP_xuxth32) {
    if (prevInsn->GetMachineOpcode() == MOP_xmovri32 || prevInsn->GetMachineOpcode() == MOP_xmovri64) {
      auto &dstMovOpnd = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
      if (!IsSameRegisterOperation(dstMovOpnd, regOpnd1, regOpnd0)) {
        return;
      }
      Operand &opnd = prevInsn->GetOperand(kInsnSecondOpnd);
      if (opnd.IsIntImmediate()) {
        auto &immOpnd = static_cast<ImmOperand&>(opnd);
        int64 value = immOpnd.GetValue();
        if ((static_cast<uint64>(value) & 0xFFFFFFFFFFFF0000) == 0) {
          bb.RemoveInsn(insn);
        }
      }
    } else if (prevInsn->GetMachineOpcode() == MOP_wldrh) {
      auto &dstOpnd = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
      if (dstOpnd.GetRegisterNumber() != regOpnd1.GetRegisterNumber()) {
        return;
      }
      bb.RemoveInsn(insn);
    }
  } else {
    /* this_mop == MOP_xuxtw64 */
    if (prevInsn->GetMachineOpcode() == MOP_xmovri32 || prevInsn->GetMachineOpcode() == MOP_wldrsb ||
        prevInsn->GetMachineOpcode() == MOP_wldrb || prevInsn->GetMachineOpcode() == MOP_wldrsh ||
        prevInsn->GetMachineOpcode() == MOP_wldrh || prevInsn->GetMachineOpcode() == MOP_wldr) {
      auto &dstOpnd = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
      if (!IsSameRegisterOperation(dstOpnd, regOpnd1, regOpnd0)) {
        return;
      }
      /* 32-bit ldr does zero-extension by default, so this conversion can be skipped */
      bb.RemoveInsn(insn);
    }
  }
}

bool FmovRegPattern::CheckCondition(Insn &insn) {
  nextInsn = insn.GetNextMachineInsn();
  if (nextInsn == nullptr) {
    return false;
  }
  if (&insn == insn.GetBB()->GetFirstInsn()) {
    return false;
  }
  prevInsn = insn.GetPrev();
  auto &curSrcRegOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  auto &prevSrcRegOpnd = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnSecondOpnd));
  /* same src freg */
  if (curSrcRegOpnd.GetRegisterNumber() != prevSrcRegOpnd.GetRegisterNumber()) {
    return false;
  }
  return true;
}

void FmovRegPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  MOperator thisMop = insn.GetMachineOpcode();
  MOperator prevMop = prevInsn->GetMachineOpcode();
  MOperator newMop;
  uint32 doOpt = 0;
  if (prevMop == MOP_xvmovrv && thisMop == MOP_xvmovrv) {
    doOpt = k32BitSize;
    newMop = MOP_wmovrr;
  } else if (prevMop == MOP_xvmovrd && thisMop == MOP_xvmovrd) {
    doOpt = k64BitSize;
    newMop = MOP_xmovrr;
  }
  if (doOpt == 0) {
    return;
  }
  auto &curDstRegOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  regno_t curDstReg = curDstRegOpnd.GetRegisterNumber();
  CG *cg = cgFunc->GetCG();
  /* optimize case 1 */
  auto &prevDstRegOpnd = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
  regno_t prevDstReg = prevDstRegOpnd.GetRegisterNumber();
  auto *aarch64CGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  RegOperand &dst =
      aarch64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(curDstReg), doOpt, kRegTyInt);
  RegOperand &src =
      aarch64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(prevDstReg), doOpt, kRegTyInt);
  Insn &newInsn = cg->BuildInstruction<AArch64Insn>(newMop, dst, src);
  bb.InsertInsnBefore(insn, newInsn);
  bb.RemoveInsn(insn);
  RegOperand &newOpnd =
      aarch64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(prevDstReg), doOpt, kRegTyInt);
  uint32 opndNum = nextInsn->GetOperandSize();
  for (uint32 opndIdx = 0; opndIdx < opndNum; ++opndIdx) {
    Operand &opnd = nextInsn->GetOperand(opndIdx);
    if (opnd.IsMemoryAccessOperand()) {
      auto &memOpnd = static_cast<MemOperand&>(opnd);
      Operand *base = memOpnd.GetBaseRegister();
      if (base != nullptr) {
        if (base->IsRegister()) {
          auto *reg = static_cast<RegOperand*>(base);
          if (reg->GetRegisterNumber() == curDstReg) {
            memOpnd.SetBaseRegister(newOpnd);
          }
        }
      }
      Operand *offset = memOpnd.GetIndexRegister();
      if (offset != nullptr) {
        if (offset->IsRegister()) {
          auto *reg = static_cast<RegOperand*>(offset);
          if (reg->GetRegisterNumber() == curDstReg) {
            memOpnd.SetIndexRegister(newOpnd);
          }
        }
      }
    } else if (opnd.IsRegister()) {
      /* Check if it is a source operand. */
      const AArch64MD *md = &AArch64CG::kMd[static_cast<AArch64Insn*>(nextInsn)->GetMachineOpcode()];
      auto *regProp = md->operand[opndIdx];
      if (regProp->IsUse()) {
        auto &reg = static_cast<RegOperand&>(opnd);
        if (reg.GetRegisterNumber() == curDstReg) {
          nextInsn->SetOperand(opndIdx, newOpnd);
        }
      }
    }
  }
}

bool SbfxOptPattern::CheckCondition(Insn &insn) {
  nextInsn = insn.GetNextMachineInsn();
  if (nextInsn == nullptr) {
    return false;
  }
  auto &curDstRegOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  uint32 opndNum = nextInsn->GetOperandSize();
  const AArch64MD *md = &AArch64CG::kMd[static_cast<AArch64Insn*>(nextInsn)->GetMachineOpcode()];
  for (uint32 opndIdx = 0; opndIdx < opndNum; ++opndIdx) {
    Operand &opnd = nextInsn->GetOperand(opndIdx);
    /* Check if it is a source operand. */
    if (opnd.IsMemoryAccessOperand() || opnd.IsList()) {
      return false;
    } else if (opnd.IsRegister()) {
      auto &reg = static_cast<RegOperand&>(opnd);
      auto *regProp = md->operand[opndIdx];
      if (reg.GetRegisterNumber() == curDstRegOpnd.GetRegisterNumber()) {
        if (reg.GetSize() != k32BitSize) {
          return false;
        }
        if (regProp->IsDef()) {
          toRemove = true;
        } else {
          (void)cands.emplace_back(opndIdx);
        }
      }
    }
  }
  return cands.size() != 0;
}

void SbfxOptPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  auto &srcRegOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  RegOperand &newReg = static_cast<AArch64CGFunc*>(cgFunc)->GetOrCreatePhysicalRegisterOperand(
      static_cast<AArch64reg>(srcRegOpnd.GetRegisterNumber()), k32BitSize, srcRegOpnd.GetRegisterType());
  // replace use point of opnd in nextInsn
  for (auto i: cands) {
    nextInsn->SetOperand(i, newReg);
  }
  if (toRemove) {
    bb.RemoveInsn(insn);
  }
}

bool CbnzToCbzPattern::CheckCondition(Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  if (curMop != MOP_wcbnz && curMop != MOP_xcbnz) {
    return false;
  }
  /* reg has to be R0, since return value is in R0 */
  auto &regOpnd0 = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  if (regOpnd0.GetRegisterNumber() != R0) {
    return false;
  }
  nextBB = insn.GetBB()->GetNext();
  /* Make sure nextBB can only be reached by bb */
  if (nextBB->GetPreds().size() > 1 || nextBB->GetEhPreds().empty()) {
    return false;
  }
  /* Next insn should be a mov R0 = 0 */
  movInsn = nextBB->GetFirstMachineInsn();
  if (movInsn == nullptr) {
    return false;
  }
  MOperator movInsnMop = movInsn->GetMachineOpcode();
  if (movInsnMop != MOP_xmovri32 && movInsnMop != MOP_xmovri64) {
    return false;
  }
  auto &movDest = static_cast<RegOperand&>(movInsn->GetOperand(kInsnFirstOpnd));
  if (movDest.GetRegisterNumber() != R0) {
    return false;
  }
  auto &movImm = static_cast<ImmOperand&>(movInsn->GetOperand(kInsnSecondOpnd));
  if (movImm.GetValue() != 0) {
    return false;
  }
  Insn *nextBrInsn = movInsn->GetNextMachineInsn();
  if (nextBrInsn == nullptr) {
    return false;
  }
  if (nextBrInsn->GetMachineOpcode() != MOP_xuncond) {
    return false;
  }
  /* Is nextBB branch to the return-bb? */
  if (nextBB->GetSuccs().size() != 1) {
    return false;
  }
  return true;
}

void CbnzToCbzPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  MOperator thisMop = insn.GetMachineOpcode();
  BB *targetBB = nullptr;
  auto it = bb.GetSuccsBegin();
  if (*it == nextBB) {
    ++it;
  }
  targetBB = *it;
  /* Make sure when nextBB is empty, targetBB is fallthru of bb. */
  if (targetBB != nextBB->GetNext()) {
    return;
  }
  BB *nextBBTarget = *(nextBB->GetSuccsBegin());
  if (nextBBTarget->GetKind() != BB::kBBReturn) {
    return;
  }
  /* Control flow looks nice, instruction looks nice */
  Operand &brTarget = brInsn->GetOperand(kInsnFirstOpnd);
  insn.SetOperand(kInsnSecondOpnd, brTarget);
  if (thisMop == MOP_wcbnz) {
    insn.SetMOP(MOP_wcbz);
  } else {
    insn.SetMOP(MOP_xcbz);
  }
  nextBB->RemoveInsn(*movInsn);
  nextBB->RemoveInsn(*brInsn);
  /* nextBB is now a fallthru bb, not a goto bb */
  nextBB->SetKind(BB::kBBFallthru);
  /*
   * fix control flow, we have bb, nextBB, targetBB, nextBB_target
   * connect bb -> nextBB_target erase targetBB
   */
  it = bb.GetSuccsBegin();
  CHECK_FATAL(it != bb.GetSuccsEnd(), "succs is empty.");
  if (*it == targetBB) {
    bb.EraseSuccs(it);
    bb.PushFrontSuccs(*nextBBTarget);
  } else {
    ++it;
    bb.EraseSuccs(it);
    bb.PushBackSuccs(*nextBBTarget);
  }
  for (auto targetBBIt = targetBB->GetPredsBegin(); targetBBIt != targetBB->GetPredsEnd(); ++targetBBIt) {
    if (*targetBBIt == &bb) {
      targetBB->ErasePreds(targetBBIt);
      break;
    }
  }
  for (auto nextIt = nextBBTarget->GetPredsBegin(); nextIt != nextBBTarget->GetPredsEnd(); ++nextIt) {
    if (*nextIt == nextBB) {
      nextBBTarget->ErasePreds(nextIt);
      break;
    }
  }
  nextBBTarget->PushBackPreds(bb);

  /* nextBB has no target, originally just branch target */
  nextBB->EraseSuccs(nextBB->GetSuccsBegin());
  ASSERT(nextBB->GetSuccs().empty(), "peep: branch target incorrect");
  /* Now make nextBB fallthru to targetBB */
  nextBB->PushFrontSuccs(*targetBB);
  targetBB->PushBackPreds(*nextBB);
}

void CsetCbzToBeqOptAArch64::Run(BB &bb, Insn &insn) {
  Insn *insn1 = insn.GetPreviousMachineInsn();
  if (insn1 == nullptr) {
    return;
  }
  /* prevInsn must be "cset" insn */
  MOperator opCode1 = insn1->GetMachineOpcode();
  if (opCode1 != MOP_xcsetrc && opCode1 != MOP_wcsetrc) {
    return;
  }

  auto &tmpRegOp1 = static_cast<RegOperand&>(insn1->GetOperand(kInsnFirstOpnd));
  regno_t baseRegNO1 = tmpRegOp1.GetRegisterNumber();
  auto &tmpRegOp2 = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  regno_t baseRegNO2 = tmpRegOp2.GetRegisterNumber();
  if (baseRegNO1 != baseRegNO2) {
    return;
  }
  /* If the reg will be used later, we shouldn't optimize the cset insn here */
  if (IfOperandIsLiveAfterInsn(tmpRegOp2, insn)) {
    return;
  }
  MOperator opCode = insn.GetMachineOpcode();
  bool reverse = (opCode == MOP_xcbz || opCode == MOP_wcbz);
  Operand &rflag = static_cast<AArch64CGFunc*>(&cgFunc)->GetOrCreateRflag();
  auto &label = static_cast<LabelOperand&>(insn.GetOperand(kInsnSecondOpnd));
  auto &cond = static_cast<CondOperand&>(insn1->GetOperand(kInsnSecondOpnd));
  MOperator jmpOperator = SelectMOperator(cond.GetCode(), reverse);
  CHECK_FATAL((jmpOperator != MOP_undef), "unknown condition code");
  Insn &newInsn = cgFunc.GetCG()->BuildInstruction<AArch64Insn>(jmpOperator, rflag, label);
  bb.RemoveInsn(*insn1);
  bb.ReplaceInsn(insn, newInsn);
}

MOperator CsetCbzToBeqOptAArch64::SelectMOperator(AArch64CC_t condCode, bool inverse) const {
  switch (condCode) {
    case CC_NE:
      return inverse ? MOP_beq : MOP_bne;
    case CC_EQ:
      return inverse ? MOP_bne : MOP_beq;
    case CC_MI:
      return inverse ? MOP_bpl : MOP_bmi;
    case CC_PL:
      return inverse ? MOP_bmi : MOP_bpl;
    case CC_VS:
      return inverse ? MOP_bvc : MOP_bvs;
    case CC_VC:
      return inverse ? MOP_bvs : MOP_bvc;
    case CC_HI:
      return inverse ? MOP_bls : MOP_bhi;
    case CC_LS:
      return inverse ? MOP_bhi : MOP_bls;
    case CC_GE:
      return inverse ? MOP_blt : MOP_bge;
    case CC_LT:
      return inverse ? MOP_bge : MOP_blt;
    case CC_HS:
      return inverse ? MOP_blo : MOP_bhs;
    case CC_LO:
      return inverse ? MOP_bhs : MOP_blo;
    case CC_LE:
      return inverse ? MOP_bgt : MOP_ble;
    case CC_GT:
      return inverse ? MOP_ble : MOP_bgt;
    case CC_CS:
      return inverse ? MOP_bcc : MOP_bcs;
    default:
      return MOP_undef;
  }
}

bool ContiLDRorSTRToSameMEMPattern::CheckCondition(Insn &insn) {
  prevInsn = insn.GetPrev();
  while (prevInsn != nullptr && prevInsn->GetMachineOpcode() == 0 && prevInsn != insn.GetBB()->GetFirstInsn()) {
    prevInsn = prevInsn->GetPrev();
  }
  if (!insn.IsMachineInstruction() || prevInsn == nullptr) {
    return false;
  }
  MOperator thisMop = insn.GetMachineOpcode();
  MOperator prevMop = prevInsn->GetMachineOpcode();
  /*
   * store regB, RegC, offset
   * load regA, RegC, offset
   */
  if ((thisMop == MOP_xldr && prevMop == MOP_xstr) || (thisMop == MOP_wldr && prevMop == MOP_wstr) ||
      (thisMop == MOP_dldr && prevMop == MOP_dstr) || (thisMop == MOP_sldr && prevMop == MOP_sstr)) {
    loadAfterStore = true;
  }
  /*
   * load regA, RegC, offset
   * load regB, RegC, offset
   */
  if ((thisMop == MOP_xldr || thisMop == MOP_wldr || thisMop == MOP_dldr || thisMop == MOP_sldr) &&
      prevMop == thisMop) {
    loadAfterLoad = true;
  }
  if (!loadAfterStore && !loadAfterLoad) {
    return false;
  }
  ASSERT(insn.GetOperand(kInsnSecondOpnd).IsMemoryAccessOperand(), "expects mem operands");
  ASSERT(prevInsn->GetOperand(kInsnSecondOpnd).IsMemoryAccessOperand(), "expects mem operands");
  return true;
}

void ContiLDRorSTRToSameMEMPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  MOperator thisMop = insn.GetMachineOpcode();
  auto &memOpnd1 = static_cast<MemOperand&>(insn.GetOperand(kInsnSecondOpnd));
  MemOperand::AArch64AddressingMode addrMode1 = memOpnd1.GetAddrMode();
  if (addrMode1 != MemOperand::kAddrModeBOi || (!memOpnd1.IsIntactIndexed())) {
    return;
  }

  auto *base1 = static_cast<RegOperand*>(memOpnd1.GetBaseRegister());
  ASSERT(base1 == nullptr || !base1->IsVirtualRegister(), "physical register has not been allocated?");
  OfstOperand *offset1 = memOpnd1.GetOffsetImmediate();

  auto &memOpnd2 = static_cast<MemOperand&>(prevInsn->GetOperand(kInsnSecondOpnd));
  MemOperand::AArch64AddressingMode addrMode2 = memOpnd2.GetAddrMode();
  if (addrMode2 != MemOperand::kAddrModeBOi || (!memOpnd2.IsIntactIndexed())) {
    return;
  }

  auto *base2 = static_cast<RegOperand*>(memOpnd2.GetBaseRegister());
  ASSERT(base2 == nullptr || !base2->IsVirtualRegister(), "physical register has not been allocated?");
  OfstOperand *offset2 = memOpnd2.GetOffsetImmediate();

  if (base1 == nullptr || base2 == nullptr || offset1 == nullptr || offset2 == nullptr) {
    return;
  }

  auto &reg1 = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  auto &reg2 = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
  int64 offsetVal1 = offset1->GetOffsetValue();
  int64 offsetVal2 = offset2->GetOffsetValue();
  if (base1->GetRegisterNumber() != base2->GetRegisterNumber() ||
      reg1.GetRegisterType() != reg2.GetRegisterType() || reg1.GetSize() != reg2.GetSize() ||
      offsetVal1 != offsetVal2) {
    return;
  }
  if (loadAfterStore && reg1.GetRegisterNumber() != reg2.GetRegisterNumber()) {
    /* replace it with mov */
    MOperator newOp = MOP_wmovrr;
    if (reg1.GetRegisterType() == kRegTyInt) {
      newOp = (reg1.GetSize() <= k32BitSize) ? MOP_wmovrr : MOP_xmovrr;
    } else if (reg1.GetRegisterType() == kRegTyFloat) {
      newOp = (reg1.GetSize() <= k32BitSize) ? MOP_xvmovs : MOP_xvmovd;
    }
    Insn *nextInsn = insn.GetNext();
    while (nextInsn != nullptr && nextInsn->GetMachineOpcode() == 0 && nextInsn != bb.GetLastInsn()) {
      nextInsn = nextInsn->GetNext();
    }
    bool moveSameReg = false;
    if (nextInsn && nextInsn->GetIsSpill() && !IfOperandIsLiveAfterInsn(reg1, *nextInsn)) {
      MOperator nextMop = nextInsn->GetMachineOpcode();
      if ((thisMop == MOP_xldr && nextMop == MOP_xstr) || (thisMop == MOP_wldr && nextMop == MOP_wstr) ||
          (thisMop == MOP_dldr && nextMop == MOP_dstr) || (thisMop == MOP_sldr && nextMop == MOP_sstr)) {
        nextInsn->Insn::SetOperand(kInsnFirstOpnd, reg2);
        moveSameReg = true;
      }
    }
    if (!moveSameReg) {
      CG *cg = cgFunc->GetCG();
      bb.InsertInsnAfter(*prevInsn, cg->BuildInstruction<AArch64Insn>(newOp, reg1, reg2));
    }
    bb.RemoveInsn(insn);
  } else if (reg1.GetRegisterNumber() == reg2.GetRegisterNumber() &&
             base1->GetRegisterNumber() != reg2.GetRegisterNumber()) {
    bb.RemoveInsn(insn);
  }
}

bool RemoveIncDecRefPattern::CheckCondition(Insn &insn) {
  if (insn.GetMachineOpcode() !=  MOP_xbl) {
    return false;
  }
  prevInsn = insn.GetPreviousMachineInsn();
  if (prevInsn == nullptr) {
    return false;
  }
  MOperator prevMop = prevInsn->GetMachineOpcode();
  if (prevMop != MOP_xmovrr) {
    return false;
  }
  auto &target = static_cast<FuncNameOperand&>(insn.GetOperand(kInsnFirstOpnd));
  if (target.GetName() != "MCC_IncDecRef_NaiveRCFast") {
    return false;
  }
  if (static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd)).GetRegisterNumber() != R1 ||
      static_cast<RegOperand&>(prevInsn->GetOperand(kInsnSecondOpnd)).GetRegisterNumber() != R0) {
    return false;
  }
  return true;
}

void RemoveIncDecRefPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  bb.RemoveInsn(*prevInsn);
  bb.RemoveInsn(insn);
}

#ifdef USE_32BIT_REF
constexpr uint32 kRefSize = 32;
#else
constexpr uint32 kRefSize = 64;
#endif

void CselZeroOneToCsetOpt::Run(BB &bb, Insn &insn) {
  Operand &trueValueOp = insn.GetOperand(kInsnSecondOpnd);
  Operand &falseValueOp = insn.GetOperand(kInsnThirdOpnd);
  Operand *trueTempOp = nullptr;
  Operand *falseTempOp = nullptr;

  /* find fixed value in BB */
  if (!trueValueOp.IsIntImmediate()) {
    trueMovInsn = FindFixedValue(trueValueOp, bb, trueTempOp, insn);
  }
  if (!falseValueOp.IsIntImmediate()) {
    falseMovInsn = FindFixedValue(falseValueOp, bb, falseTempOp, insn);
  }

  ASSERT(trueTempOp != nullptr, "trueTempOp should not be nullptr");
  ASSERT(falseTempOp != nullptr, "falseTempOp should not be nullptr");
  /* csel to cset */
  if ((trueTempOp->IsIntImmediate() || IsZeroRegister(*trueTempOp)) &&
      (falseTempOp->IsIntImmediate() || IsZeroRegister(*falseTempOp))) {
    ImmOperand *imm1 = static_cast<ImmOperand*>(trueTempOp);
    ImmOperand *imm2 = static_cast<ImmOperand*>(falseTempOp);
    bool inverse = imm1->IsOne() && (imm2->IsZero() || IsZeroRegister(*imm2));
    if (inverse || ((imm1->IsZero() || IsZeroRegister(*imm1)) && imm2->IsOne())) {
      Operand &reg = insn.GetOperand(kInsnFirstOpnd);
      CondOperand &condOperand = static_cast<CondOperand&>(insn.GetOperand(kInsnFourthOpnd));
      MOperator mopCode = (reg.GetSize() == k64BitSize) ? MOP_xcsetrc : MOP_wcsetrc;
      /* get new cond  ccCode */
      AArch64CC_t ccCode = inverse ? condOperand.GetCode() : GetReverseCond(condOperand);
      if (ccCode == kCcLast) {
        return;
      }
      AArch64CGFunc *func = static_cast<AArch64CGFunc*>(cgFunc);
      CondOperand &cond = func->GetCondOperand(ccCode);
      Operand &rflag = func->GetOrCreateRflag();
      Insn &csetInsn = func->GetCG()->BuildInstruction<AArch64Insn>(mopCode, reg, cond, rflag);
      if (CGOptions::DoCGSSA() && CGOptions::GetInstance().GetOptimizeLevel() < 0) {
        CHECK_FATAL(false, "check this case in ssa opt");
      }
      insn.GetBB()->ReplaceInsn(insn, csetInsn);
    }
  }
}

Insn *CselZeroOneToCsetOpt::FindFixedValue(Operand &opnd, BB &bb, Operand *&tempOp, const Insn &insn) const {
  tempOp = &opnd;
  bool alreadyFindCsel = false;
  bool isRegDefined = false;
  regno_t regno = static_cast<RegOperand&>(opnd).GetRegisterNumber();
  FOR_BB_INSNS_REV(defInsn, &bb) {
    if (!defInsn->IsMachineInstruction() || defInsn->IsBranch()) {
      continue;
    }
    /* find csel */
    if (defInsn->GetId() == insn.GetId()) {
      alreadyFindCsel = true;
    }
    /* find def defined */
    if (alreadyFindCsel) {
      isRegDefined = defInsn->IsRegDefined(regno);
    }
    /* if def defined is movi do this opt */
    if (isRegDefined) {
      MOperator thisMop = defInsn->GetMachineOpcode();
      if (thisMop == MOP_xmovri32 || thisMop == MOP_xmovri64) {
        if (&defInsn->GetOperand(kInsnFirstOpnd) == &opnd) {
          tempOp = &(defInsn->GetOperand(kInsnSecondOpnd));
          return defInsn;
        }
      } else {
        return nullptr;
      }
    }
  }
  return nullptr;
}

AArch64CC_t CselZeroOneToCsetOpt::GetReverseCond(const CondOperand &cond) const {
  switch (cond.GetCode()) {
    case CC_NE:
      return CC_EQ;
    case CC_EQ:
      return CC_NE;
    case CC_HS:
      return CC_LO;
    case CC_LO:
      return CC_HS;
    case CC_MI:
      return CC_PL;
    case CC_PL:
      return CC_MI;
    case CC_VS:
      return CC_VC;
    case CC_VC:
      return CC_VS;
    case CC_HI:
      return CC_LS;
    case CC_LS:
      return CC_HI;
    case CC_LT:
      return CC_GE;
    case CC_GE:
      return CC_LT;
    case CC_GT:
      return CC_LE;
    case CC_LE:
      return CC_GT;
    default:
      CHECK_FATAL(false, "Not support yet.");
  }
  return kCcLast;
}

bool InlineReadBarriersPattern::CheckCondition(Insn &insn) {
  /* Inline read barriers only enabled for GCONLY. */
  if (!CGOptions::IsGCOnly()) {
    return false;
  }
  return true;
}

void InlineReadBarriersPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  const std::string &barrierName = GetReadBarrierName(insn);
  CG *cg = cgFunc->GetCG();
  if (barrierName == kMccDummy) {
    /* remove dummy call. */
    bb.RemoveInsn(insn);
  } else {
    /* replace barrier function call with load instruction. */
    bool isVolatile = (barrierName == kMccLoadRefV || barrierName == kMccLoadRefVS);
    bool isStatic = (barrierName == kMccLoadRefS || barrierName == kMccLoadRefVS);
    /* refSize is 32 if USE_32BIT_REF defined, otherwise 64. */
    const uint32 refSize = kRefSize;
    auto *aarch64CGFunc = static_cast<AArch64CGFunc*>(cgFunc);
    MOperator loadOp = GetLoadOperator(refSize, isVolatile);
    RegOperand &regOp = aarch64CGFunc->GetOrCreatePhysicalRegisterOperand(R0, refSize, kRegTyInt);
    AArch64reg addrReg = isStatic ? R0 : R1;
    MemOperand &addr = aarch64CGFunc->CreateMemOpnd(addrReg, 0, refSize);
    Insn &loadInsn = cg->BuildInstruction<AArch64Insn>(loadOp, regOp, addr);
    bb.ReplaceInsn(insn, loadInsn);
  }
  bool isTailCall = (insn.GetMachineOpcode() == MOP_tail_call_opt_xbl);
  if (isTailCall) {
    /* add 'ret' instruction for tail call optimized load barrier. */
    Insn &retInsn = cg->BuildInstruction<AArch64Insn>(MOP_xret);
    bb.AppendInsn(retInsn);
    bb.SetKind(BB::kBBReturn);
  }
}

bool ReplaceDivToMultiPattern::CheckCondition(Insn &insn) {
  prevInsn = insn.GetPreviousMachineInsn();
  if (prevInsn == nullptr) {
    return false;
  }
  prePrevInsn = prevInsn->GetPreviousMachineInsn();
  auto &sdivOpnd1 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  auto &sdivOpnd2 = static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd));
  if (sdivOpnd1.GetRegisterNumber() == sdivOpnd2.GetRegisterNumber() || sdivOpnd1.GetRegisterNumber() == R16 ||
      sdivOpnd2.GetRegisterNumber() == R16 || prePrevInsn == nullptr) {
    return false;
  }
  MOperator prevMop = prevInsn->GetMachineOpcode();
  MOperator prePrevMop = prePrevInsn->GetMachineOpcode();
  if ((prevMop > 0) && (prevMop == MOP_wmovkri16) && (prePrevMop > 0) && (prePrevMop == MOP_xmovri32)) {
    return true;
  }
  return false;
}

void ReplaceDivToMultiPattern::Run(BB &bb, Insn &insn) {
  if (CheckCondition(insn)) {
    auto &sdivOpnd1 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
    auto &sdivOpnd2 = static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd));
  /* Check if dest operand of insn is idential with  register of prevInsn and prePrevInsn. */
    if ((&(prevInsn->GetOperand(kInsnFirstOpnd)) != &sdivOpnd2) ||
        (&(prePrevInsn->GetOperand(kInsnFirstOpnd)) != &sdivOpnd2)) {
      return;
    }
    auto &prevLsl = static_cast<LogicalShiftLeftOperand&>(prevInsn->GetOperand(kInsnThirdOpnd));
    if (prevLsl.GetShiftAmount() != k16BitSize) {
      return;
    }
    auto &prevImmOpnd = static_cast<ImmOperand&>(prevInsn->GetOperand(kInsnSecondOpnd));
    auto &prePrevImmOpnd = static_cast<ImmOperand&>(prePrevInsn->GetOperand(kInsnSecondOpnd));
    /*
     * expect the immediate value of first mov is 0x086A0 which matches 0x186A0
     * because 0x10000 is ignored in 32 bits register
     */
    if ((prevImmOpnd.GetValue() != 1) || (prePrevImmOpnd.GetValue() != 34464)) {
      return;
    }
    auto *aarch64CGFunc = static_cast<AArch64CGFunc*>(cgFunc);
    CG *cg = cgFunc->GetCG();
    /* mov   w16, #0x588f */
    RegOperand &tempOpnd = aarch64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(R16),
                                                                             k64BitSize, kRegTyInt);
    /* create a immedate operand with this specific value */
    ImmOperand &multiplierLow = aarch64CGFunc->CreateImmOperand(0x588f, k32BitSize, false);
    Insn &multiplierLowInsn = cg->BuildInstruction<AArch64Insn>(MOP_xmovri32, tempOpnd, multiplierLow);
    bb.InsertInsnBefore(*prePrevInsn, multiplierLowInsn);

    /*
     * movk    w16, #0x4f8b, LSL #16
     * create a immedate operand with this specific value
     */
    ImmOperand &multiplierHigh = aarch64CGFunc->CreateImmOperand(0x4f8b, k32BitSize, false);
    LogicalShiftLeftOperand *multiplierHighLsl = aarch64CGFunc->GetLogicalShiftLeftOperand(k16BitSize, true);
    Insn &multiplierHighInsn =
        cg->BuildInstruction<AArch64Insn>(MOP_wmovkri16, tempOpnd, multiplierHigh, *multiplierHighLsl);
    bb.InsertInsnBefore(*prePrevInsn, multiplierHighInsn);

    /* smull   x16, w0, w16 */
    Insn &newSmullInsn =
        cg->BuildInstruction<AArch64Insn>(MOP_xsmullrrr, tempOpnd, sdivOpnd1, tempOpnd);
    bb.InsertInsnBefore(*prePrevInsn, newSmullInsn);

    /* asr     x16, x16, #32 */
    ImmOperand &dstLsrImmHigh = aarch64CGFunc->CreateImmOperand(k32BitSize, k32BitSize, false);
    Insn &dstLsrInsnHigh =
        cg->BuildInstruction<AArch64Insn>(MOP_xasrrri6, tempOpnd, tempOpnd, dstLsrImmHigh);
    bb.InsertInsnBefore(*prePrevInsn, dstLsrInsnHigh);

    /* add     x16, x16, w0, SXTW */
    Operand &sxtw = aarch64CGFunc->CreateExtendShiftOperand(ExtendShiftOperand::kSXTW, 0, 3);
    Insn &addInsn =
        cg->BuildInstruction<AArch64Insn>(MOP_xxwaddrrre, tempOpnd, tempOpnd, sdivOpnd1, sxtw);
    bb.InsertInsnBefore(*prePrevInsn, addInsn);

    /* asr     x16, x16, #17 */
    ImmOperand &dstLsrImmChange = aarch64CGFunc->CreateImmOperand(17, k32BitSize, false);
    Insn &dstLsrInsnChange =
        cg->BuildInstruction<AArch64Insn>(MOP_xasrrri6, tempOpnd, tempOpnd, dstLsrImmChange);
    bb.InsertInsnBefore(*prePrevInsn, dstLsrInsnChange);

    /* add     x2, x16, x0, LSR #31 */
    auto &sdivOpnd0 = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
    regno_t sdivOpnd0RegNO = sdivOpnd0.GetRegisterNumber();
    RegOperand &extendSdivOpnd0 =
        aarch64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(sdivOpnd0RegNO),
                                                          k64BitSize, kRegTyInt);

    regno_t sdivOpnd1RegNum = sdivOpnd1.GetRegisterNumber();
    RegOperand &extendSdivOpnd1 =
        aarch64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(sdivOpnd1RegNum),
                                                          k64BitSize, kRegTyInt);
    /* shift bit amount is thirty-one at this insn */
    BitShiftOperand &addLsrOpnd = aarch64CGFunc->CreateBitShiftOperand(BitShiftOperand::kShiftLSR, 31, 6);
    Insn &addLsrInsn = cg->BuildInstruction<AArch64Insn>(MOP_xaddrrrs, extendSdivOpnd0, tempOpnd,
                                                         extendSdivOpnd1, addLsrOpnd);
    bb.InsertInsnBefore(*prePrevInsn, addLsrInsn);

    /*
     * remove insns
     * Check if x1 is used after sdiv insn, and if it is in live-out.
     */
    if (sdivOpnd2.GetRegisterNumber() != sdivOpnd0.GetRegisterNumber()) {
      if (IfOperandIsLiveAfterInsn(sdivOpnd2, insn)) {
        /* Only remove div instruction. */
        bb.RemoveInsn(insn);
        return;
      }
    }

    bb.RemoveInsn(*prePrevInsn);
    bb.RemoveInsn(*prevInsn);
    bb.RemoveInsn(insn);
  }
}

Insn *AndCmpBranchesToCsetAArch64::FindPreviousCmp(Insn &insn) const {
  regno_t defRegNO = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd)).GetRegisterNumber();
  for (Insn *curInsn = insn.GetPrev(); curInsn != nullptr; curInsn = curInsn->GetPrev()) {
    if (!curInsn->IsMachineInstruction()) {
      continue;
    }
    if (curInsn->GetMachineOpcode() == MOP_wcmpri || curInsn->GetMachineOpcode() == MOP_xcmpri) {
      return curInsn;
    }
    /*
     * if any def/use of CC or insn defReg between insn and curInsn, stop searching and return nullptr.
     */
    if (static_cast<AArch64Insn*>(curInsn)->IsRegDefOrUse(defRegNO) ||
        static_cast<AArch64Insn*>(curInsn)->IsRegDefOrUse(kRFLAG)) {
      return nullptr;
    }
  }
  return nullptr;
}

void AndCmpBranchesToCsetAArch64::Run(BB &bb, Insn &insn) {
   /* prevInsn must be "cmp" insn */
  Insn *prevInsn = FindPreviousCmp(insn);
  if (prevInsn == nullptr) {
    return;
  }
  /* prevPrevInsn must be "and" insn */
  Insn *prevPrevInsn = prevInsn->GetPreviousMachineInsn();
  if (prevPrevInsn == nullptr ||
      (prevPrevInsn->GetMachineOpcode() != MOP_wandrri12 && prevPrevInsn->GetMachineOpcode() != MOP_xandrri13)) {
    return;
  }

  auto &csetCond = static_cast<CondOperand&>(insn.GetOperand(kInsnSecondOpnd));
  auto &cmpImm = static_cast<ImmOperand&>(prevInsn->GetOperand(kInsnThirdOpnd));
  int64 cmpImmVal = cmpImm.GetValue();
  auto &andImm = static_cast<ImmOperand&>(prevPrevInsn->GetOperand(kInsnThirdOpnd));
  int64 andImmVal = andImm.GetValue();
  if ((csetCond.GetCode() == CC_EQ && cmpImmVal == andImmVal) ||
      (csetCond.GetCode() == CC_NE && cmpImmVal == 0)) {
    /* if flag_reg of "cmp" is live later, we can't remove cmp insn. */
    auto &flagReg = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
    if (IfOperandIsLiveAfterInsn(flagReg, insn)) {
      return;
    }

    auto &csetReg = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
    auto &prevInsnSecondReg = prevInsn->GetOperand(kInsnSecondOpnd);
    bool isRegDiff = !RegOperand::IsSameRegNO(csetReg, prevInsnSecondReg);
    if (isRegDiff && IfOperandIsLiveAfterInsn(static_cast<RegOperand&>(prevInsnSecondReg), insn)) {
      return;
    }
    if (andImmVal == 1) {
      if (!RegOperand::IsSameRegNO(prevInsnSecondReg, prevPrevInsn->GetOperand(kInsnFirstOpnd))) {
        return;
      }
      /* save the "and" insn only. */
      bb.RemoveInsn(insn);
      bb.RemoveInsn(*prevInsn);
      if (isRegDiff) {
        prevPrevInsn->Insn::SetOperand(kInsnFirstOpnd, csetReg);
      }
    } else {
      if (!RegOperand::IsSameReg(prevInsnSecondReg, prevPrevInsn->GetOperand(kInsnFirstOpnd)) ||
          !RegOperand::IsSameReg(prevInsnSecondReg, prevPrevInsn->GetOperand(kInsnSecondOpnd))) {
        return;
      }

      /* andImmVal is n power of 2 */
      int n = LogValueAtBase2(andImmVal);
      if (n < 0) {
        return;
      }

      /* create ubfx insn */
      MOperator ubfxOp = (csetReg.GetSize() <= k32BitSize) ? MOP_wubfxrri5i5 : MOP_xubfxrri6i6;
      if (ubfxOp == MOP_wubfxrri5i5 && static_cast<uint32>(n) >= k32BitSize) {
        return;
      }
      auto &dstReg = static_cast<RegOperand&>(csetReg);
      auto &srcReg = static_cast<RegOperand&>(prevInsnSecondReg);
      CG *cg = cgFunc.GetCG();
      auto *aarch64CGFunc = static_cast<AArch64CGFunc*>(&cgFunc);
      ImmOperand &bitPos = aarch64CGFunc->CreateImmOperand(n, k8BitSize, false);
      ImmOperand &bitSize = aarch64CGFunc->CreateImmOperand(1, k8BitSize, false);
      Insn &ubfxInsn = cg->BuildInstruction<AArch64Insn>(ubfxOp, dstReg, srcReg, bitPos, bitSize);
      bb.InsertInsnBefore(*prevPrevInsn, ubfxInsn);
      bb.RemoveInsn(insn);
      bb.RemoveInsn(*prevInsn);
      bb.RemoveInsn(*prevPrevInsn);
    }
  }
}

void AndCmpBranchesToTstAArch64::Run(BB &bb, Insn &insn) {
  /* nextInsn must be "cmp" insn */
  Insn *nextInsn = insn.GetNextMachineInsn();
  if (nextInsn == nullptr ||
      (nextInsn->GetMachineOpcode() != MOP_wcmpri && nextInsn->GetMachineOpcode() != MOP_xcmpri)) {
    return;
  }
  /* nextNextInsn must be "beq" or "bne" insn */
  Insn *nextNextInsn = nextInsn->GetNextMachineInsn();
  if (nextNextInsn == nullptr ||
      (nextNextInsn->GetMachineOpcode() != MOP_beq && nextNextInsn->GetMachineOpcode() != MOP_bne)) {
    return;
  }
  auto &andRegOp = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  regno_t andRegNO1 = andRegOp.GetRegisterNumber();
  auto &cmpRegOp2 = static_cast<RegOperand&>(nextInsn->GetOperand(kInsnSecondOpnd));
  regno_t cmpRegNO2 = cmpRegOp2.GetRegisterNumber();
  if (andRegNO1 != cmpRegNO2) {
    return;
  }
  /* If the reg will be used later, we shouldn't optimize the and insn here */
  if (IfOperandIsLiveAfterInsn(andRegOp, *nextInsn)) {
    return;
  }
  Operand &immOpnd = nextInsn->GetOperand(kInsnThirdOpnd);
  ASSERT(immOpnd.IsIntImmediate(), "expects ImmOperand");
  auto &defConst = static_cast<ImmOperand&>(immOpnd);
  int64 defConstValue = defConst.GetValue();
  if (defConstValue != 0) {
    return;
  }
  /* build tst insn */
  Operand &andOpnd3 = insn.GetOperand(kInsnThirdOpnd);
  auto &andRegOp2 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  MOperator newOp = MOP_undef;
  if (andOpnd3.IsRegister()) {
    newOp = (andRegOp2.GetSize() <= k32BitSize) ? MOP_wtstrr : MOP_xtstrr;
  } else {
    newOp = (andRegOp2.GetSize() <= k32BitSize) ? MOP_wtstri32 : MOP_xtstri64;
  }
  Operand &rflag = static_cast<AArch64CGFunc*>(&cgFunc)->GetOrCreateRflag();
  Insn &newInsn = cgFunc.GetCG()->BuildInstruction<AArch64Insn>(newOp, rflag, andRegOp2, andOpnd3);
  if (CGOptions::DoCGSSA() && CGOptions::GetInstance().GetOptimizeLevel() < 0) {
    CHECK_FATAL(false, "check this case in ssa opt");
  }
  bb.InsertInsnAfter(*nextInsn, newInsn);
  bb.RemoveInsn(insn);
  bb.RemoveInsn(*nextInsn);
}

void AndCbzBranchesToTstAArch64::Run(BB &bb, Insn &insn) {
  /* nextInsn must be "cbz" or "cbnz" insn */
  Insn *nextInsn = insn.GetNextMachineInsn();
  if (nextInsn == nullptr ||
      (nextInsn->GetMachineOpcode() != MOP_wcbz && nextInsn->GetMachineOpcode() != MOP_xcbz)) {
    return;
  }
  auto &andRegOp = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  regno_t andRegNO1 = andRegOp.GetRegisterNumber();
  auto &cbzRegOp2 = static_cast<RegOperand&>(nextInsn->GetOperand(kInsnFirstOpnd));
  regno_t cbzRegNO2 = cbzRegOp2.GetRegisterNumber();
  if (andRegNO1 != cbzRegNO2) {
    return;
  }
  /* If the reg will be used later, we shouldn't optimize the and insn here */
  if (IfOperandIsLiveAfterInsn(andRegOp, *nextInsn)) {
    return;
  }
  /* build tst insn */
  Operand &andOpnd3 = insn.GetOperand(kInsnThirdOpnd);
  auto &andRegOp2 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  auto &andRegOp3 = static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd));
  MOperator newTstOp = MOP_undef;
  if (andOpnd3.IsRegister()) {
    newTstOp = (andRegOp2.GetSize() <= k32BitSize && andRegOp3.GetSize() <= k32BitSize) ? MOP_wtstrr : MOP_xtstrr;
  } else {
    newTstOp = (andRegOp2.GetSize() <= k32BitSize && andRegOp3.GetSize() <= k32BitSize) ? MOP_wtstri32 : MOP_xtstri64;
  }
  Operand &rflag = static_cast<AArch64CGFunc*>(&cgFunc)->GetOrCreateRflag();
  Insn &newInsnTst = cgFunc.GetCG()->BuildInstruction<AArch64Insn>(newTstOp, rflag, andRegOp2, andOpnd3);
  if (andOpnd3.IsImmediate()) {
    if (!static_cast<ImmOperand&>(andOpnd3).IsBitmaskImmediate(andRegOp2.GetSize())) {
      return;
    }
  }
  /* build beq insn */
  MOperator opCode = nextInsn->GetMachineOpcode();
  bool reverse = (opCode == MOP_xcbz || opCode == MOP_wcbz);
  auto &label = static_cast<LabelOperand&>(nextInsn->GetOperand(kInsnSecondOpnd));
  MOperator jmpOperator = reverse ? MOP_beq : MOP_bne;
  Insn &newInsnJmp = cgFunc.GetCG()->BuildInstruction<AArch64Insn>(jmpOperator, rflag, label);
  bb.ReplaceInsn(insn, newInsnTst);
  bb.ReplaceInsn(*nextInsn, newInsnJmp);
}

void ZeroCmpBranchesAArch64::Run(BB &bb, Insn &insn) {
  Insn *prevInsn = insn.GetPreviousMachineInsn();
  if (!insn.IsBranch() || insn.GetOperandSize() <= kInsnSecondOpnd || prevInsn == nullptr) {
    return;
  }
  if (!insn.GetOperand(kInsnSecondOpnd).IsLabel()) {
    return;
  }
  LabelOperand *label = &static_cast<LabelOperand&>(insn.GetOperand(kInsnSecondOpnd));
  RegOperand *regOpnd = nullptr;
  RegOperand *reg0 = nullptr;
  RegOperand *reg1 = nullptr;
  MOperator newOp = MOP_undef;
  ImmOperand *imm = nullptr;
  switch (prevInsn->GetMachineOpcode()) {
    case MOP_wcmpri:
    case MOP_xcmpri: {
      regOpnd = &static_cast<RegOperand&>(prevInsn->GetOperand(kInsnSecondOpnd));
      imm = &static_cast<ImmOperand&>(prevInsn->GetOperand(kInsnThirdOpnd));
      if (imm->GetValue() != 0) {
        return;
      }
      if (insn.GetMachineOpcode() == MOP_bge) {
        newOp = (regOpnd->GetSize() <= k32BitSize) ? MOP_wtbz : MOP_xtbz;
      } else if (insn.GetMachineOpcode() == MOP_blt) {
        newOp = (regOpnd->GetSize() <= k32BitSize) ? MOP_wtbnz : MOP_xtbnz;
      } else {
        return;
      }
      break;
    }
    case MOP_wcmprr:
    case MOP_xcmprr: {
      reg0 = &static_cast<RegOperand&>(prevInsn->GetOperand(kInsnSecondOpnd));
      reg1 = &static_cast<RegOperand&>(prevInsn->GetOperand(kInsnThirdOpnd));
      if (!IsZeroRegister(*reg0) && !IsZeroRegister(*reg1)) {
        return;
      }
      switch (insn.GetMachineOpcode()) {
        case MOP_bge:
          if (IsZeroRegister(*reg1)) {
            regOpnd = &static_cast<RegOperand&>(prevInsn->GetOperand(kInsnSecondOpnd));
            newOp = (regOpnd->GetSize() <= k32BitSize) ? MOP_wtbz : MOP_xtbz;
          } else {
            return;
          }
          break;
        case MOP_ble:
          if (IsZeroRegister(*reg0)) {
            regOpnd = &static_cast<RegOperand&>(prevInsn->GetOperand(kInsnThirdOpnd));
            newOp = (regOpnd->GetSize() <= k32BitSize) ? MOP_wtbz : MOP_xtbz;
          } else {
            return;
          }
          break;
        case MOP_blt:
          if (IsZeroRegister(*reg1)) {
            regOpnd = &static_cast<RegOperand&>(prevInsn->GetOperand(kInsnSecondOpnd));
            newOp = (regOpnd->GetSize() <= k32BitSize) ? MOP_wtbnz : MOP_xtbnz;
          } else {
            return;
          }
          break;
        case MOP_bgt:
          if (IsZeroRegister(*reg0)) {
            regOpnd = &static_cast<RegOperand&>(prevInsn->GetOperand(kInsnThirdOpnd));
            newOp = (regOpnd->GetSize() <= k32BitSize) ? MOP_wtbnz : MOP_xtbnz;
          } else {
            return;
          }
          break;
        default:
          return;
      }
      break;
    }
    default:
      return;
  }
  CG *cg = cgFunc.GetCG();
  auto aarch64CGFunc = static_cast<AArch64CGFunc*>(&cgFunc);
  ImmOperand &bitp = aarch64CGFunc->CreateImmOperand(
      (regOpnd->GetSize() <= k32BitSize) ? (k32BitSize - 1) : (k64BitSize - 1), k8BitSize, false);
  bb.InsertInsnAfter(
      insn, cg->BuildInstruction<AArch64Insn>(newOp, *static_cast<RegOperand*>(regOpnd), bitp, *label));
  bb.RemoveInsn(insn);
  bb.RemoveInsn(*prevInsn);
}

void ElimDuplicateExtensionAArch64::Run(BB &bb, Insn &insn) {
  (void)bb;
  Insn *prevInsn = insn.GetPreviousMachineInsn();
  if (prevInsn == nullptr) {
    return;
  }
  uint32 index;
  uint32 upper;
  bool is32bits = false;
  MOperator *table = nullptr;
  MOperator thisMop = insn.GetMachineOpcode();
  switch (thisMop) {
    case MOP_xsxtb32:
      is32bits = true;
      [[clang::fallthrough]];
    case MOP_xsxtb64:
      table = sextMopTable;
      index = 0;
      upper = kSizeOfSextMopTable;
      break;
    case MOP_xsxth32:
      is32bits = true;
      [[clang::fallthrough]];
    case MOP_xsxth64:
      table = sextMopTable;
      index = 2;
      upper = kSizeOfSextMopTable;
      break;
    case MOP_xsxtw64:
      table = sextMopTable;
      index = 4;
      upper = kSizeOfSextMopTable;
      break;
    case MOP_xuxtb32:
      is32bits = true;
      table = uextMopTable;
      index = 0;
      upper = kSizeOfUextMopTable;
      break;
    case MOP_xuxth32:
      is32bits = true;
      table = uextMopTable;
      index = 1;
      upper = kSizeOfUextMopTable;
      break;
    case MOP_xuxtw64:
      table = uextMopTable;
      index = 2;
      upper = kSizeOfUextMopTable;
      break;
    default:
      CHECK_FATAL(false, "Unexpected mop");
  }
  MOperator prevMop = prevInsn->GetMachineOpcode();
  for (uint32 i = index; i < upper; ++i) {
    if (prevMop == table[i]) {
      Operand &prevDestOpnd = prevInsn->GetOperand(kInsnFirstOpnd);
      regno_t dest = static_cast<RegOperand&>(prevDestOpnd).GetRegisterNumber();
      regno_t src = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd)).GetRegisterNumber();
      if (dest == src) {
        insn.SetMOP(is32bits ? MOP_wmovrr : MOP_xmovrr);
        if (upper == kSizeOfSextMopTable &&
            static_cast<RegOperand&>(prevDestOpnd).GetValidBitsNum() !=
            static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd)).GetValidBitsNum()) {
          if (is32bits) {
            insn.GetOperand(kInsnFirstOpnd).SetSize(k64BitSize);
            insn.SetMOP(MOP_xmovrr);
          } else {
            prevDestOpnd.SetSize(k64BitSize);
            prevInsn->SetMOP(prevMop == MOP_xsxtb32 ? MOP_xsxtb64 : MOP_xsxth64);
          }
        }
      }
      break;
    }
  }
}

/*
 * help function for DeleteMovAfterCbzOrCbnz
 * input:
 *        bb: the bb to be checked out
 *        checkCbz: to check out BB end with cbz or cbnz, if cbz, input true
 *        opnd: for MOV reg, #0, opnd indicate reg
 * return:
 *        according to cbz, return true if insn is cbz or cbnz and the first operand of cbz(cbnz) is same as input
 *      operand
 */
bool DeleteMovAfterCbzOrCbnzAArch64::PredBBCheck(BB &bb, bool checkCbz, const Operand &opnd) const {
  if (bb.GetKind() != BB::kBBIf) {
    return false;
  }

  Insn *condBr = cgcfg->FindLastCondBrInsn(bb);
  ASSERT(condBr != nullptr, "condBr must be found");
  if (!cgcfg->IsCompareAndBranchInsn(*condBr)) {
    return false;
  }
  MOperator mOp = condBr->GetMachineOpcode();
  if (checkCbz && mOp != MOP_wcbz && mOp != MOP_xcbz) {
    return false;
  }
  if (!checkCbz && mOp != MOP_xcbnz && mOp != MOP_wcbnz) {
    return false;
  }
  return RegOperand::IsSameRegNO(condBr->GetOperand(kInsnFirstOpnd), opnd);
}

bool DeleteMovAfterCbzOrCbnzAArch64::OpndDefByMovZero(const Insn &insn) const {
  MOperator defMop = insn.GetMachineOpcode();
  switch (defMop) {
    case MOP_xmovri32:
    case MOP_xmovri64: {
      Operand &defOpnd = insn.GetOperand(kInsnSecondOpnd);
      ASSERT(defOpnd.IsIntImmediate(), "expects ImmOperand");
      auto &defConst = static_cast<ImmOperand&>(defOpnd);
      int64 defConstValue = defConst.GetValue();
      if (defConstValue == 0) {
        return true;
      }
      return false;
    }
    case MOP_xmovrr:
    case MOP_wmovrr: {
      Operand &secondOpnd = insn.GetOperand(kInsnSecondOpnd);
      ASSERT(secondOpnd.IsRegister(), "expects RegOperand here");
      auto &regOpnd = static_cast<RegOperand&>(secondOpnd);
      return IsZeroRegister(regOpnd);
    }
    default:
      return false;
  }
}

/* check whether predefine insn of first operand of test_insn is exist in current BB */
bool DeleteMovAfterCbzOrCbnzAArch64::NoPreDefine(Insn &testInsn) const {
  Insn *nextInsn = nullptr;
  for (Insn *insn = testInsn.GetBB()->GetFirstInsn(); insn != nullptr && insn != &testInsn; insn = nextInsn) {
    nextInsn = insn->GetNextMachineInsn();
    if (!insn->IsMachineInstruction()) {
      continue;
    }
    ASSERT(!insn->IsCall(), "CG internal error, call insn should not be at the middle of the BB.");
    const AArch64MD *md = &AArch64CG::kMd[static_cast<AArch64Insn*>(insn)->GetMachineOpcode()];
    uint32 opndNum = insn->GetOperandSize();
    for (uint32 i = 0; i < opndNum; ++i) {
      Operand &opnd = insn->GetOperand(i);
      OpndProp *regProp = md->operand[i];
      if (!regProp->IsDef()) {
        continue;
      }
      if (opnd.IsMemoryAccessOperand()) {
        auto &memOpnd = static_cast<MemOperand&>(opnd);
        RegOperand *base = memOpnd.GetBaseRegister();
        ASSERT(base != nullptr, "nullptr check");
        ASSERT(base->IsRegister(), "expects RegOperand");
        if (RegOperand::IsSameRegNO(*base, testInsn.GetOperand(kInsnFirstOpnd)) &&
            memOpnd.GetAddrMode() == MemOperand::kAddrModeBOi &&
            (memOpnd.IsPostIndexed() || memOpnd.IsPreIndexed())) {
          return false;
        }
      } else if (opnd.IsList()) {
        for (auto operand : static_cast<ListOperand&>(opnd).GetOperands()) {
          if (RegOperand::IsSameRegNO(testInsn.GetOperand(kInsnFirstOpnd), *operand)) {
            return false;
          }
        }
      } else if (opnd.IsRegister()) {
        if (RegOperand::IsSameRegNO(testInsn.GetOperand(kInsnFirstOpnd), opnd)) {
          return false;
        }
      }
    }
  }
  return true;
}
void DeleteMovAfterCbzOrCbnzAArch64::ProcessBBHandle(BB *processBB, const BB &bb, const Insn &insn) const {
  ASSERT(processBB != nullptr, "process_bb is null in ProcessBBHandle");
  FOR_BB_INSNS_SAFE(processInsn, processBB, nextProcessInsn) {
    nextProcessInsn = processInsn->GetNextMachineInsn();
    if (!processInsn->IsMachineInstruction()) {
      continue;
    }
    /* register may be a caller save register */
    if (processInsn->IsCall()) {
      break;
    }
    if (!OpndDefByMovZero(*processInsn) || !NoPreDefine(*processInsn) ||
        !RegOperand::IsSameRegNO(processInsn->GetOperand(kInsnFirstOpnd), insn.GetOperand(kInsnFirstOpnd))) {
      continue;
    }
    bool toDoOpt = true;
    MOperator condBrMop = insn.GetMachineOpcode();
    /* process elseBB, other preds must be cbz */
    if (condBrMop == MOP_wcbnz || condBrMop == MOP_xcbnz) {
      /* check out all preds of process_bb */
      for (auto *processBBPred : processBB->GetPreds()) {
        if (processBBPred == &bb) {
          continue;
        }
        if (!PredBBCheck(*processBBPred, true, processInsn->GetOperand(kInsnFirstOpnd))) {
          toDoOpt = false;
          break;
        }
      }
    } else {
      /* process ifBB, other preds can be cbz or cbnz(one at most) */
      for (auto processBBPred : processBB->GetPreds()) {
        if (processBBPred == &bb) {
          continue;
        }
        /* for cbnz pred, there is one at most */
        if (!PredBBCheck(*processBBPred, processBBPred != processBB->GetPrev(),
                         processInsn->GetOperand(kInsnFirstOpnd))) {
          toDoOpt = false;
          break;
        }
      }
    }
    if (!toDoOpt) {
      continue;
    }
    processBB->RemoveInsn(*processInsn);
  }
}

/* ldr wn, [x1, wn, SXTW]
 * add x2, wn, x2
 */
bool ComplexMemOperandAddAArch64::IsExpandBaseOpnd(const Insn &insn, const Insn &prevInsn) const {
  MOperator prevMop = prevInsn.GetMachineOpcode();
  if (prevMop >= MOP_wldrsb && prevMop <= MOP_xldr &&
      prevInsn.GetOperand(kInsnFirstOpnd).Equals(insn.GetOperand(kInsnSecondOpnd))) {
    return true;
  }
  return false;
}

void ComplexMemOperandAddAArch64::Run(BB &bb, Insn &insn) {
  AArch64CGFunc *aarch64CGFunc = static_cast<AArch64CGFunc*>(&cgFunc);
  Insn *nextInsn = insn.GetNextMachineInsn();
  if (nextInsn == nullptr) {
    return;
  }
  Insn *prevInsn = insn.GetPreviousMachineInsn();
  MOperator thisMop = insn.GetMachineOpcode();
  if (thisMop != MOP_xaddrrr && thisMop != MOP_waddrrr) {
    return;
  }
  MOperator nextMop = nextInsn->GetMachineOpcode();
  if ((nextMop > 0) &&
      ((nextMop >= MOP_wldrsb && nextMop <= MOP_dldr) || (nextMop >= MOP_wstrb && nextMop <= MOP_dstr))) {
    if (!IsMemOperandOptPattern(insn, *nextInsn)) {
      return;
    }
    MemOperand *memOpnd = static_cast<MemOperand*>(nextInsn->GetMemOpnd());
    auto newBaseOpnd = static_cast<RegOperand*>(&insn.GetOperand(kInsnSecondOpnd));
    auto newIndexOpnd = static_cast<RegOperand*>(&insn.GetOperand(kInsnThirdOpnd));
    regno_t memBaseOpndRegNO = newBaseOpnd->GetRegisterNumber();
    if (newBaseOpnd->GetSize() <= k32BitSize && prevInsn != nullptr && IsExpandBaseOpnd(insn, *prevInsn)) {
      newBaseOpnd = &aarch64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(memBaseOpndRegNO),
                                                                       k64BitSize, kRegTyInt);
    }
    if (newBaseOpnd->GetSize() != k64BitSize) {
      return;
    }
    if (newIndexOpnd->GetSize() <= k32BitSize) {
      MemOperand &newMemOpnd =
          aarch64CGFunc->GetOrCreateMemOpnd(MemOperand::kAddrModeBOrX, memOpnd->GetSize(), newBaseOpnd,
                                            newIndexOpnd, 0, false);
      nextInsn->SetOperand(kInsnSecondOpnd, newMemOpnd);
    } else {
      MemOperand &newMemOpnd =
          aarch64CGFunc->GetOrCreateMemOpnd(MemOperand::kAddrModeBOrX, memOpnd->GetSize(), newBaseOpnd,
                                            newIndexOpnd, nullptr, nullptr);
      nextInsn->SetOperand(kInsnSecondOpnd, newMemOpnd);
    }
    bb.RemoveInsn(insn);
  }
}

void DeleteMovAfterCbzOrCbnzAArch64::Run(BB &bb, Insn &insn) {
  if (bb.GetKind() != BB::kBBIf) {
    return;
  }
  if (&insn != cgcfg->FindLastCondBrInsn(bb)) {
    return;
  }
  if (!cgcfg->IsCompareAndBranchInsn(insn)) {
    return;
  }
  BB *processBB = nullptr;
  if (bb.GetNext() == maplebe::CGCFG::GetTargetSuc(bb)) {
    return;
  }

  MOperator condBrMop = insn.GetMachineOpcode();
  if (condBrMop == MOP_wcbnz || condBrMop == MOP_xcbnz) {
    processBB = bb.GetNext();
  } else {
    processBB = maplebe::CGCFG::GetTargetSuc(bb);
  }

  ProcessBBHandle(processBB, bb, insn);
}

MOperator OneHoleBranchesPreAArch64::FindNewMop(const BB &bb, const Insn &insn) const {
  MOperator newOp = MOP_undef;
  if (&insn != bb.GetLastInsn()) {
    return newOp;
  }
  MOperator thisMop = insn.GetMachineOpcode();
  if (thisMop != MOP_wcbz && thisMop != MOP_wcbnz && thisMop != MOP_xcbz && thisMop != MOP_xcbnz) {
    return newOp;
  }
  switch (thisMop) {
    case MOP_wcbz:
      newOp = MOP_wtbnz;
      break;
    case MOP_wcbnz:
      newOp = MOP_wtbz;
      break;
    case MOP_xcbz:
      newOp = MOP_xtbnz;
      break;
    case MOP_xcbnz:
      newOp = MOP_xtbz;
      break;
    default:
      CHECK_FATAL(false, "can not touch here");
      break;
  }
  return newOp;
}

void OneHoleBranchesPreAArch64::Run(BB &bb, Insn &insn) {
  AArch64CGFunc *aarch64CGFunc = static_cast<AArch64CGFunc*>(&cgFunc);
  MOperator newOp = FindNewMop(bb, insn);
  if (newOp == MOP_undef) {
    return;
  }
  Insn *prevInsn = insn.GetPreviousMachineInsn();
  LabelOperand &label = static_cast<LabelOperand&>(insn.GetOperand(kInsnSecondOpnd));
  if (prevInsn != nullptr && prevInsn->GetMachineOpcode() == MOP_xuxtb32 &&
      (static_cast<RegOperand&>(prevInsn->GetOperand(kInsnSecondOpnd)).GetValidBitsNum() <= k8BitSize ||
       static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd)).GetValidBitsNum() <= k8BitSize)) {
    if (&(prevInsn->GetOperand(kInsnFirstOpnd)) != &(insn.GetOperand(kInsnFirstOpnd))) {
      return;
    }
    if (IfOperandIsLiveAfterInsn(static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd)), insn)) {
      return;
    }
    insn.SetOperand(kInsnFirstOpnd, prevInsn->GetOperand(kInsnSecondOpnd));
    if (CGOptions::DoCGSSA()) {
      CHECK_FATAL(false, "check this case in ssa opt");
    }
    bb.RemoveInsn(*prevInsn);
  }
  if (prevInsn != nullptr &&
      (prevInsn->GetMachineOpcode() == MOP_xeorrri13 || prevInsn->GetMachineOpcode() == MOP_weorrri12) &&
      static_cast<ImmOperand&>(prevInsn->GetOperand(kInsnThirdOpnd)).GetValue() == 1) {
    if (&(prevInsn->GetOperand(kInsnFirstOpnd)) != &(insn.GetOperand(kInsnFirstOpnd))) {
      return;
    }
    Insn *prevPrevInsn = prevInsn->GetPreviousMachineInsn();
    if (prevPrevInsn == nullptr) {
      return;
    }
    if (prevPrevInsn->GetMachineOpcode() != MOP_xuxtb32 ||
        static_cast<RegOperand&>(prevPrevInsn->GetOperand(kInsnSecondOpnd)).GetValidBitsNum() != 1) {
      return;
    }
    if (&(prevPrevInsn->GetOperand(kInsnFirstOpnd)) != &(prevInsn->GetOperand(kInsnSecondOpnd))) {
      return;
    }
    ImmOperand &oneHoleOpnd = aarch64CGFunc->CreateImmOperand(0, k8BitSize, false);
    auto &regOperand = static_cast<RegOperand&>(prevPrevInsn->GetOperand(kInsnSecondOpnd));
    if (CGOptions::DoCGSSA()) {
      CHECK_FATAL(false, "check this case in ssa opt");
    }
    bb.InsertInsnAfter(insn, cgFunc.GetCG()->BuildInstruction<AArch64Insn>(newOp, regOperand, oneHoleOpnd, label));
    bb.RemoveInsn(insn);
    bb.RemoveInsn(*prevInsn);
    bb.RemoveInsn(*prevPrevInsn);
  }
}

bool LoadFloatPointPattern::FindLoadFloatPoint(Insn &insn) {
  MOperator mOp = insn.GetMachineOpcode();
  optInsn.clear();
  if (mOp != MOP_xmovzri16) {
    return false;
  }
  optInsn.emplace_back(&insn);

  Insn *insnMov2 = insn.GetNextMachineInsn();
  if (insnMov2 == nullptr) {
    return false;
  }
  if (insnMov2->GetMachineOpcode() != MOP_xmovkri16) {
    return false;
  }
  optInsn.emplace_back(insnMov2);

  Insn *insnMov3 = insnMov2->GetNextMachineInsn();
  if (insnMov3 == nullptr) {
    return false;
  }
  if (insnMov3->GetMachineOpcode() != MOP_xmovkri16) {
    return false;
  }
  optInsn.emplace_back(insnMov3);

  Insn *insnMov4 = insnMov3->GetNextMachineInsn();
  if (insnMov4 == nullptr) {
    return false;
  }
  if (insnMov4->GetMachineOpcode() != MOP_xmovkri16) {
    return false;
  }
  optInsn.emplace_back(insnMov4);
  return true;
}

bool LoadFloatPointPattern::IsPatternMatch() {
  int insnNum = 0;
  Insn *insn1 = optInsn[insnNum];
  Insn *insn2 = optInsn[++insnNum];
  Insn *insn3 = optInsn[++insnNum];
  Insn *insn4 = optInsn[++insnNum];
  if ((static_cast<RegOperand&>(insn1->GetOperand(kInsnFirstOpnd)).GetRegisterNumber() !=
       static_cast<RegOperand&>(insn2->GetOperand(kInsnFirstOpnd)).GetRegisterNumber()) ||
      (static_cast<RegOperand&>(insn2->GetOperand(kInsnFirstOpnd)).GetRegisterNumber() !=
       static_cast<RegOperand&>(insn3->GetOperand(kInsnFirstOpnd)).GetRegisterNumber()) ||
      (static_cast<RegOperand&>(insn3->GetOperand(kInsnFirstOpnd)).GetRegisterNumber() !=
       static_cast<RegOperand&>(insn4->GetOperand(kInsnFirstOpnd)).GetRegisterNumber())) {
    return false;
  }
  if ((static_cast<LogicalShiftLeftOperand&>(insn1->GetOperand(kInsnThirdOpnd)).GetShiftAmount() != 0) ||
      (static_cast<LogicalShiftLeftOperand&>(insn2->GetOperand(kInsnThirdOpnd)).GetShiftAmount() !=
       k16BitSize) ||
      (static_cast<LogicalShiftLeftOperand&>(insn3->GetOperand(kInsnThirdOpnd)).GetShiftAmount() !=
       k32BitSize) ||
      (static_cast<LogicalShiftLeftOperand&>(insn4->GetOperand(kInsnThirdOpnd)).GetShiftAmount() !=
       (k16BitSize + k32BitSize))) {
    return false;
  }
  return true;
}

bool LoadFloatPointPattern::CheckCondition(Insn &insn) {
  if (FindLoadFloatPoint(insn) && IsPatternMatch()) {
    return true;
  }
  return false;
}

void LoadFloatPointPattern::Run(BB &bb, Insn &insn) {
  /* logical shift left values in three optimized pattern */
  if (CheckCondition(insn)) {
    int insnNum = 0;
    Insn *insn1 = optInsn[insnNum];
    Insn *insn2 = optInsn[++insnNum];
    Insn *insn3 = optInsn[++insnNum];
    Insn *insn4 = optInsn[++insnNum];
    auto &movConst1 = static_cast<ImmOperand&>(insn1->GetOperand(kInsnSecondOpnd));
    auto &movConst2 = static_cast<ImmOperand&>(insn2->GetOperand(kInsnSecondOpnd));
    auto &movConst3 = static_cast<ImmOperand&>(insn3->GetOperand(kInsnSecondOpnd));
    auto &movConst4 = static_cast<ImmOperand&>(insn4->GetOperand(kInsnSecondOpnd));
    /* movk/movz's immOpnd is 16-bit unsigned immediate */
    uint64 value = static_cast<uint64>(movConst1.GetValue()) +
                   (static_cast<uint64>(movConst2.GetValue()) << k16BitSize) +
                   (static_cast<uint64>(movConst3.GetValue()) << k32BitSize) +
                   (static_cast<uint64>(movConst4.GetValue()) << (k16BitSize + k32BitSize));

    LabelIdx lableIdx = cgFunc->CreateLabel();
    AArch64CGFunc *aarch64CGFunc = static_cast<AArch64CGFunc*>(cgFunc);
    LabelOperand &target = aarch64CGFunc->GetOrCreateLabelOperand(lableIdx);
    cgFunc->InsertLabelMap(lableIdx, value);
    Insn &newInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(MOP_xldli, insn4->GetOperand(kInsnFirstOpnd),
                                                                  target);
    bb.InsertInsnAfter(*insn4, newInsn);
    bb.RemoveInsn(*insn1);
    bb.RemoveInsn(*insn2);
    bb.RemoveInsn(*insn3);
    bb.RemoveInsn(*insn4);
  }
}

void ReplaceOrrToMovAArch64::Run(BB &bb, Insn &insn) {
  Operand *opndOfOrr = nullptr;
  ImmOperand *immOpnd = nullptr;
  RegOperand *reg1 = nullptr;
  RegOperand *reg2 = nullptr;
  MOperator thisMop = insn.GetMachineOpcode();
  MOperator newMop = MOP_undef;
  switch (thisMop) {
    case MOP_wiorrri12: {  /* opnd1 is reg32 and opnd3 is immediate. */
      opndOfOrr = &(insn.GetOperand(kInsnThirdOpnd));
      reg2 = &static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
      newMop = MOP_wmovrr;
      break;
    }
    case MOP_xiorrri13: {  /* opnd1 is reg64 and opnd3 is immediate. */
      opndOfOrr = &(insn.GetOperand(kInsnThirdOpnd));
      reg2 = &static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
      newMop = MOP_xmovrr;
      break;
    }
    default:
      break;
  }
  ASSERT(opndOfOrr->IsIntImmediate(), "expects immediate operand");
  immOpnd = static_cast<ImmOperand*>(opndOfOrr);
  if (immOpnd->GetValue() == 0) {
    reg1 = &static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
    if (CGOptions::DoCGSSA()) {
      CHECK_FATAL(false, "check this case in ssa opt");
    }
    bb.ReplaceInsn(insn, cgFunc.GetCG()->BuildInstruction<AArch64Insn>(newMop, *reg1, *reg2));
  }
}

void ReplaceCmpToCmnAArch64::Run(BB &bb, Insn &insn) {
  AArch64CGFunc *aarch64CGFunc = static_cast<AArch64CGFunc*>(&cgFunc);
  MOperator thisMop = insn.GetMachineOpcode();
  MOperator nextMop = MOP_undef;
  MOperator newMop = MOP_undef;
  uint64 negOne = UINT64_MAX;
  switch (thisMop) {
    case MOP_xmovri32: {
      nextMop = MOP_wcmprr;
      newMop = MOP_wcmnri;
      negOne = UINT32_MAX;
      break;
    }
    case MOP_xmovri64: {
      nextMop = MOP_xcmprr;
      newMop = MOP_xcmnri;
      break;
    }
    default:
      break;
  }
  Operand *opnd1OfMov = &(insn.GetOperand(kInsnFirstOpnd));
  Operand *opnd2OfMov = &(insn.GetOperand(kInsnSecondOpnd));
  if (opnd2OfMov->IsIntImmediate()) {
    ImmOperand *immOpnd = static_cast<ImmOperand*>(opnd2OfMov);
    int64 iVal = immOpnd->GetValue();
    if ((kNegativeImmLowerLimit <= iVal && iVal < 0) || static_cast<uint64>(iVal) == negOne) {
      Insn *nextInsn = insn.GetNextMachineInsn();  /* get the next insn to judge if it is a cmp instruction. */
      if (nextInsn != nullptr) {
        if (nextInsn->GetMachineOpcode() == nextMop) {
          Operand *opndCmp2 = &(nextInsn->GetOperand(kInsnSecondOpnd));
          Operand *opndCmp3 = &(nextInsn->GetOperand(kInsnThirdOpnd));  /* get the third operand of cmp */
          /* if the first operand of mov equals the third operand of cmp, match the pattern. */
          if (opnd1OfMov == opndCmp3) {
            if (iVal == static_cast<int64>(negOne)) {
              iVal = -1;
            }
            ImmOperand &newOpnd = aarch64CGFunc->CreateImmOperand(iVal * (-1), immOpnd->GetSize(), false);
            Operand &regFlag = nextInsn->GetOperand(kInsnFirstOpnd);
            bb.ReplaceInsn(*nextInsn, cgFunc.GetCG()->BuildInstruction<AArch64Insn>(MOperator(newMop), regFlag,
                                                                                    *opndCmp2, newOpnd));
          }
        }
      }
    }
  }
}

bool RemoveIncRefPattern::CheckCondition(Insn &insn) {
  MOperator mOp = insn.GetMachineOpcode();
  if (mOp != MOP_xbl) {
    return false;
  }
  auto &target = static_cast<FuncNameOperand&>(insn.GetOperand(kInsnFirstOpnd));
  if (target.GetName() != "MCC_IncDecRef_NaiveRCFast") {
    return false;
  }
  insnMov2 = insn.GetPreviousMachineInsn();
  if (insnMov2 == nullptr) {
    return false;
  }
  MOperator mopMov2 = insnMov2->GetMachineOpcode();
  if (mopMov2 != MOP_xmovrr) {
    return false;
  }
  insnMov1 = insnMov2->GetPreviousMachineInsn();
  if (insnMov1 == nullptr) {
    return false;
  }
  MOperator mopMov1 = insnMov1->GetMachineOpcode();
  if (mopMov1 != MOP_xmovrr) {
    return false;
  }
  if (static_cast<RegOperand&>(insnMov1->GetOperand(kInsnSecondOpnd)).GetRegisterNumber() !=
      static_cast<RegOperand&>(insnMov2->GetOperand(kInsnSecondOpnd)).GetRegisterNumber()) {
    return false;
  }
  auto &mov2Dest = static_cast<RegOperand&>(insnMov2->GetOperand(kInsnFirstOpnd));
  auto &mov1Dest = static_cast<RegOperand&>(insnMov1->GetOperand(kInsnFirstOpnd));
  if (mov1Dest.IsVirtualRegister() || mov2Dest.IsVirtualRegister() || mov1Dest.GetRegisterNumber() != R0 ||
      mov2Dest.GetRegisterNumber() != R1) {
    return false;
  }
  return true;
}

void RemoveIncRefPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  bb.RemoveInsn(insn);
  bb.RemoveInsn(*insnMov2);
  bb.RemoveInsn(*insnMov1);
}

bool LongIntCompareWithZPattern::FindLondIntCmpWithZ(Insn &insn) {
  MOperator thisMop = insn.GetMachineOpcode();
  optInsn.clear();
  /* forth */
  if (thisMop != MOP_wcmpri) {
    return false;
  }
  (void)optInsn.emplace_back(&insn);

  /* third */
  Insn *preInsn1 = insn.GetPreviousMachineInsn();
  if (preInsn1 == nullptr) {
    return false;
  }
  MOperator preMop1 = preInsn1->GetMachineOpcode();
  if (preMop1 != MOP_wcsincrrrc) {
    return false;
  }
  (void)optInsn.emplace_back(preInsn1);

  /* second */
  Insn *preInsn2 = preInsn1->GetPreviousMachineInsn();
  if (preInsn2 == nullptr) {
    return false;
  }
  MOperator preMop2 = preInsn2->GetMachineOpcode();
  if (preMop2 != MOP_wcsinvrrrc) {
    return false;
  }
  (void)optInsn.emplace_back(preInsn2);

  /* first */
  Insn *preInsn3 = preInsn2->GetPreviousMachineInsn();
  if (preInsn3 == nullptr) {
    return false;
  }
  MOperator preMop3 = preInsn3->GetMachineOpcode();
  if (preMop3 != MOP_xcmpri) {
    return false;
  }
  (void)optInsn.emplace_back(preInsn3);
  return true;
}

bool LongIntCompareWithZPattern::IsPatternMatch() {
  constexpr int insnLen = 4;
  if (optInsn.size() != insnLen) {
    return false;
  }
  int insnNum = 0;
  Insn *insn1 = optInsn[insnNum];
  Insn *insn2 = optInsn[++insnNum];
  Insn *insn3 = optInsn[++insnNum];
  Insn *insn4 = optInsn[++insnNum];
  ASSERT(insnNum == 3, " this specific case has three insns");
  if (IsZeroRegister(insn3->GetOperand(kInsnSecondOpnd)) && IsZeroRegister(insn3->GetOperand(kInsnThirdOpnd)) &&
      IsZeroRegister(insn2->GetOperand(kInsnThirdOpnd)) &&
      &(insn2->GetOperand(kInsnFirstOpnd)) == &(insn2->GetOperand(kInsnSecondOpnd)) &&
      static_cast<CondOperand&>(insn3->GetOperand(kInsnFourthOpnd)).GetCode() == CC_GE &&
      static_cast<CondOperand&>(insn2->GetOperand(kInsnFourthOpnd)).GetCode() == CC_LE &&
      static_cast<ImmOperand&>(insn1->GetOperand(kInsnThirdOpnd)).GetValue() == 0 &&
      static_cast<ImmOperand&>(insn4->GetOperand(kInsnThirdOpnd)).GetValue() == 0) {
    return true;
  }
  return false;
}

bool LongIntCompareWithZPattern::CheckCondition(Insn &insn) {
  if (FindLondIntCmpWithZ(insn) && IsPatternMatch()) {
    return true;
  }
  return false;
}

void LongIntCompareWithZPattern::Run(BB &bb, Insn &insn) {
  /* found pattern */
  if (CheckCondition(insn)) {
    Insn &newInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(optInsn[3]->GetMachineOpcode(),
                                                                   optInsn[3]->GetOperand(kInsnFirstOpnd),
                                                                   optInsn[3]->GetOperand(kInsnSecondOpnd),
                                                                   optInsn[3]->GetOperand(kInsnThirdOpnd));
    /* use newInsn to replace  the third optInsn */
    bb.ReplaceInsn(*optInsn[0], newInsn);
    optInsn.clear();
  }
}

void ComplexMemOperandAArch64::Run(BB &bb, Insn &insn) {
  AArch64CGFunc *aarch64CGFunc = static_cast<AArch64CGFunc*>(&cgFunc);
  Insn *nextInsn = insn.GetNextMachineInsn();
  if (nextInsn == nullptr) {
    return;
  }
  MOperator thisMop = insn.GetMachineOpcode();
  if (thisMop != MOP_xadrpl12) {
    return;
  }

  MOperator nextMop = nextInsn->GetMachineOpcode();
  if (nextMop > 0 &&
      ((nextMop >= MOP_wldrsb && nextMop <= MOP_dldp) || (nextMop >= MOP_wstrb && nextMop <= MOP_dstp))) {
    /* Check if base register of nextInsn and the dest operand of insn are identical. */
    MemOperand *memOpnd = static_cast<MemOperand*>(nextInsn->GetMemOpnd());
    ASSERT(memOpnd != nullptr, "memOpnd is null in AArch64Peep::ComplexMemOperandAArch64");

    /* Only for AddrMode_B_OI addressing mode. */
    if (memOpnd->GetAddrMode() != MemOperand::kAddrModeBOi) {
      return;
    }

    /* Only for intact memory addressing. */
    if (!memOpnd->IsIntactIndexed()) {
      return;
    }

    auto &regOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));

    /* Avoid linking issues when object is not 16byte aligned */
    if (memOpnd->GetSize() == k128BitSize) {
      return;
    }

    /* Check if dest operand of insn is idential with base register of nextInsn. */
    if (memOpnd->GetBaseRegister() != &regOpnd) {
      return;
    }

    /* Check if x0 is used after ldr insn, and if it is in live-out. */
    if (IfOperandIsLiveAfterInsn(regOpnd, *nextInsn)) {
      return;
    }

    /* load store pairs cannot have relocation */
    if (nextInsn->IsLoadStorePair() && insn.GetOperand(kInsnThirdOpnd).IsStImmediate()) {
      return;
    }

    auto &stImmOpnd = static_cast<StImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
    OfstOperand &offOpnd = aarch64CGFunc->GetOrCreateOfstOpnd(
        stImmOpnd.GetOffset() + memOpnd->GetOffsetImmediate()->GetOffsetValue(), k32BitSize);

    /* do not guarantee rodata alignment at Os */
    if (CGOptions::OptimizeForSize() && stImmOpnd.GetSymbol()->IsReadOnly()) {
      return;
    }

    /* avoid relocation */
    if ((offOpnd.GetValue() % static_cast<int8>(kBitsPerByte)) != 0) {
      return;
    }

    if (cgFunc.GetMirModule().IsCModule()) {
      Insn *prevInsn = insn.GetPrev();
      MOperator prevMop = prevInsn->GetMachineOpcode();
      if (prevMop != MOP_xadrp) {
        return;
      } else {
        auto &prevStImmOpnd = static_cast<StImmOperand&>(prevInsn->GetOperand(kInsnSecondOpnd));
        prevStImmOpnd.SetOffset(offOpnd.GetValue());
      }
    }
    auto &newBaseOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
    MemOperand &newMemOpnd =
        aarch64CGFunc->GetOrCreateMemOpnd(MemOperand::kAddrModeLo12Li, memOpnd->GetSize(),
                                          &newBaseOpnd, nullptr, &offOpnd, stImmOpnd.GetSymbol());

    nextInsn->SetMemOpnd(&newMemOpnd);
    bb.RemoveInsn(insn);
    CHECK_FATAL(!CGOptions::IsLazyBinding() || cgFunc.GetCG()->IsLibcore(),
        "this pattern can't be found in this phase");
  }
}

void ComplexMemOperandPreAddAArch64::Run(BB &bb, Insn &insn) {
  AArch64CGFunc *aarch64CGFunc = static_cast<AArch64CGFunc*>(&cgFunc);
  Insn *nextInsn = insn.GetNextMachineInsn();
  if (nextInsn == nullptr) {
    return;
  }
  MOperator thisMop = insn.GetMachineOpcode();
  if (thisMop != MOP_xaddrrr && thisMop != MOP_waddrrr) {
    return;
  }
  MOperator nextMop = nextInsn->GetMachineOpcode();
  if (nextMop > 0 &&
      ((nextMop >= MOP_wldrsb && nextMop <= MOP_dldr) || (nextMop >= MOP_wstrb && nextMop <= MOP_dstr))) {
    if (!IsMemOperandOptPattern(insn, *nextInsn)) {
      return;
    }
    MemOperand *memOpnd = static_cast<MemOperand*>(nextInsn->GetMemOpnd());
    auto &newBaseOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
    if (newBaseOpnd.GetSize() != k64BitSize) {
      return;
    }
    auto &newIndexOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd));
    if (newIndexOpnd.GetSize() <= k32BitSize) {
      MemOperand &newMemOpnd =
          aarch64CGFunc->GetOrCreateMemOpnd(MemOperand::kAddrModeBOrX, memOpnd->GetSize(), &newBaseOpnd,
                                            &newIndexOpnd, 0, false);
      nextInsn->SetOperand(kInsnSecondOpnd, newMemOpnd);
    } else {
      auto *newOfstOpnd = &aarch64CGFunc->GetOrCreateOfstOpnd(0, k32BitSize);
      MemOperand &newMemOpnd =
          aarch64CGFunc->GetOrCreateMemOpnd(MemOperand::kAddrModeBOrX, memOpnd->GetSize(), &newBaseOpnd,
                                            &newIndexOpnd, newOfstOpnd, nullptr);
      nextInsn->SetOperand(kInsnSecondOpnd, newMemOpnd);
    }
    bb.RemoveInsn(insn);
  }
}

bool ComplexMemOperandLSLAArch64::CheckShiftValid(const Insn &insn,
                                                  const BitShiftOperand &lsl) const {
  /* check if shift amount is valid */
  uint32 lslAmount = lsl.GetShiftAmount();
  constexpr uint8 twoShiftBits = 2;
  constexpr uint8 threeShiftBits = 3;
  uint8 memSize = static_cast<const AArch64Insn&>(insn).GetLoadStoreSize();
  if ((memSize == k32BitSize && (lsl.GetShiftAmount() != 0 && lslAmount != twoShiftBits)) ||
      (memSize == k64BitSize && (lsl.GetShiftAmount() != 0 && lslAmount != threeShiftBits))) {
    return false;
  }
  if (memSize != (k8BitSize << lslAmount)) {
    return false;
  }
  return true;
}

void ComplexMemOperandLSLAArch64::Run(BB &bb, Insn &insn) {
  AArch64CGFunc *aarch64CGFunc = static_cast<AArch64CGFunc*>(&cgFunc);
  Insn *nextInsn = insn.GetNextMachineInsn();
  if (nextInsn == nullptr) {
    return;
  }
  MOperator thisMop = insn.GetMachineOpcode();
  if (thisMop != MOP_xaddrrrs) {
    return;
  }
  MOperator nextMop = nextInsn->GetMachineOpcode();
  if (nextMop > 0 &&
      ((nextMop >= MOP_wldrsb && nextMop <= MOP_dldr) || (nextMop >= MOP_wstrb && nextMop <= MOP_dstr))) {
    /* Check if base register of nextInsn and the dest operand of insn are identical. */
    MemOperand *memOpnd = static_cast<MemOperand*>(nextInsn->GetMemOpnd());
    ASSERT(memOpnd != nullptr, "null ptr check");

    /* Only for AddrMode_B_OI addressing mode. */
    if (memOpnd->GetAddrMode() != MemOperand::kAddrModeBOi) {
      return;
    }

    /* Only for immediate is  0. */
    if (memOpnd->GetOffsetImmediate()->GetOffsetValue() != 0) {
      return;
    }

    /* Only for intact memory addressing. */
    if (!memOpnd->IsIntactIndexed()) {
      return;
    }

    auto &regOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));

    /* Check if dest operand of insn is idential with base register of nextInsn. */
    if (memOpnd->GetBaseRegister() != &regOpnd) {
      return;
    }

#ifdef USE_32BIT_REF
    if (nextInsn->IsAccessRefField() && nextInsn->GetOperand(kInsnFirstOpnd).GetSize() > k32BitSize) {
      return;
    }
#endif

    /* Check if x0 is used after ldr insn, and if it is in live-out. */
    if (IfOperandIsLiveAfterInsn(regOpnd, *nextInsn)) {
      return;
    }
    auto &lsl = static_cast<BitShiftOperand&>(insn.GetOperand(kInsnFourthOpnd));
    if (!CheckShiftValid(*nextInsn, lsl)) {
      return;
    }
    auto &newBaseOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
    auto &newIndexOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd));
    MemOperand &newMemOpnd =
        aarch64CGFunc->GetOrCreateMemOpnd(MemOperand::kAddrModeBOrX, memOpnd->GetSize(), &newBaseOpnd,
                                          &newIndexOpnd, static_cast<int32>(lsl.GetShiftAmount()),
                                          false);
    nextInsn->SetOperand(kInsnSecondOpnd, newMemOpnd);
    bb.RemoveInsn(insn);
  }
}

void ComplexMemOperandLabelAArch64::Run(BB &bb, Insn &insn) {
  Insn *nextInsn = insn.GetNextMachineInsn();
  if (nextInsn == nullptr) {
    return;
  }
  MOperator thisMop = insn.GetMachineOpcode();
  if (thisMop != MOP_xldli) {
    return;
  }
  MOperator nextMop = nextInsn->GetMachineOpcode();
  if (nextMop != MOP_xvmovdr) {
    return;
  }
  auto &regOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  if (regOpnd.GetRegisterNumber() !=
      static_cast<RegOperand&>(nextInsn->GetOperand(kInsnSecondOpnd)).GetRegisterNumber()) {
    return;
  }

  /* Check if x0 is used after ldr insn, and if it is in live-out. */
  if (IfOperandIsLiveAfterInsn(regOpnd, *nextInsn)) {
    return;
  }
  if (CGOptions::DoCGSSA()) {
    /* same as CombineFmovLdrPattern in ssa */
    CHECK_FATAL(false, "check this case in ssa");
  }
  Insn &newInsn = cgFunc.GetCG()->BuildInstruction<AArch64Insn>(MOP_dldli, nextInsn->GetOperand(kInsnFirstOpnd),
                                                                insn.GetOperand(kInsnSecondOpnd));
  bb.InsertInsnAfter(*nextInsn, newInsn);
  bb.RemoveInsn(insn);
  bb.RemoveInsn(*nextInsn);
}

static bool MayThrowBetweenInsn(const Insn &prevCallInsn, const Insn &currCallInsn) {
  for (Insn *insn = prevCallInsn.GetNext(); insn != nullptr && insn != &currCallInsn; insn = insn->GetNext()) {
    if (insn->MayThrow()) {
      return true;
    }
  }
  return false;
}

/*
 * mov R0, vreg1 / R0      -> objDesignateInsn
 * add vreg2, vreg1, #imm  -> fieldDesignateInsn
 * mov R1, vreg2           -> fieldParamDefInsn
 * mov R2, vreg3           -> fieldValueDefInsn
 */
bool WriteFieldCallPattern::WriteFieldCallOptPatternMatch(const Insn &writeFieldCallInsn, WriteRefFieldParam &param) {
  Insn *fieldValueDefInsn = writeFieldCallInsn.GetPreviousMachineInsn();
  if (fieldValueDefInsn == nullptr || fieldValueDefInsn->GetMachineOpcode() != MOP_xmovrr) {
    return false;
  }
  Operand &fieldValueDefInsnDestOpnd = fieldValueDefInsn->GetOperand(kInsnFirstOpnd);
  auto &fieldValueDefInsnDestReg = static_cast<RegOperand&>(fieldValueDefInsnDestOpnd);
  if (fieldValueDefInsnDestReg.GetRegisterNumber() != R2) {
    return false;
  }
  paramDefInsns.emplace_back(fieldValueDefInsn);
  param.fieldValue = &(fieldValueDefInsn->GetOperand(kInsnSecondOpnd));
  Insn *fieldParamDefInsn = fieldValueDefInsn->GetPreviousMachineInsn();
  if (fieldParamDefInsn == nullptr || fieldParamDefInsn->GetMachineOpcode() != MOP_xmovrr) {
    return false;
  }
  Operand &fieldParamDestOpnd = fieldParamDefInsn->GetOperand(kInsnFirstOpnd);
  auto &fieldParamDestReg = static_cast<RegOperand&>(fieldParamDestOpnd);
  if (fieldParamDestReg.GetRegisterNumber() != R1) {
    return false;
  }
  paramDefInsns.emplace_back(fieldParamDefInsn);
  Insn *fieldDesignateInsn = fieldParamDefInsn->GetPreviousMachineInsn();
  if (fieldDesignateInsn == nullptr || fieldDesignateInsn->GetMachineOpcode() != MOP_xaddrri12) {
    return false;
  }
  Operand &fieldParamDefSrcOpnd = fieldParamDefInsn->GetOperand(kInsnSecondOpnd);
  Operand &fieldDesignateDestOpnd = fieldDesignateInsn->GetOperand(kInsnFirstOpnd);
  if (!RegOperand::IsSameReg(fieldParamDefSrcOpnd, fieldDesignateDestOpnd)) {
    return false;
  }
  Operand &fieldDesignateBaseOpnd = fieldDesignateInsn->GetOperand(kInsnSecondOpnd);
  param.fieldBaseOpnd = &(static_cast<RegOperand&>(fieldDesignateBaseOpnd));
  auto &immOpnd = static_cast<ImmOperand&>(fieldDesignateInsn->GetOperand(kInsnThirdOpnd));
  param.fieldOffset = immOpnd.GetValue();
  paramDefInsns.emplace_back(fieldDesignateInsn);
  Insn *objDesignateInsn = fieldDesignateInsn->GetPreviousMachineInsn();
  if (objDesignateInsn == nullptr || objDesignateInsn->GetMachineOpcode() != MOP_xmovrr) {
    return false;
  }
  Operand &objDesignateDestOpnd = objDesignateInsn->GetOperand(kInsnFirstOpnd);
  auto &objDesignateDestReg = static_cast<RegOperand&>(objDesignateDestOpnd);
  if (objDesignateDestReg.GetRegisterNumber() != R0) {
    return false;
  }
  Operand &objDesignateSrcOpnd = objDesignateInsn->GetOperand(kInsnSecondOpnd);
  if (RegOperand::IsSameReg(objDesignateDestOpnd, objDesignateSrcOpnd) ||
      !RegOperand::IsSameReg(objDesignateSrcOpnd, fieldDesignateBaseOpnd)) {
    return false;
  }
  param.objOpnd = &(objDesignateInsn->GetOperand(kInsnSecondOpnd));
  paramDefInsns.emplace_back(objDesignateInsn);
  return true;
}

bool WriteFieldCallPattern::IsWriteRefFieldCallInsn(const Insn &insn) const {
  if (!insn.IsCall() || insn.IsIndirectCall()) {
    return false;
  }
  Operand *targetOpnd = insn.GetCallTargetOperand();
  ASSERT(targetOpnd != nullptr, "targetOpnd must not be nullptr");
  if (!targetOpnd->IsFuncNameOpnd()) {
    return false;
  }
  auto *target = static_cast<FuncNameOperand*>(targetOpnd);
  const MIRSymbol *funcSt = target->GetFunctionSymbol();
  ASSERT(funcSt->GetSKind() == kStFunc, "the kind of funcSt is unreasonable");
  const std::string &funcName = funcSt->GetName();
  return funcName == "MCC_WriteRefField" || funcName == "MCC_WriteVolatileField";
}

bool WriteFieldCallPattern::CheckCondition(Insn &insn) {
  nextInsn = insn.GetNextMachineInsn();
  if (nextInsn == nullptr) {
    return false;
  }
  if (!IsWriteRefFieldCallInsn(insn)) {
    return false;
  }
  if (!hasWriteFieldCall) {
    if (!WriteFieldCallOptPatternMatch(insn, firstCallParam)) {
      return false;
    }
    prevCallInsn = &insn;
    hasWriteFieldCall = true;
    return false;
  }
  if (!WriteFieldCallOptPatternMatch(insn, currentCallParam)) {
    return false;
  }
  if (prevCallInsn == nullptr || MayThrowBetweenInsn(*prevCallInsn, insn)) {
    return false;
  }
  if (firstCallParam.objOpnd == nullptr || currentCallParam.objOpnd == nullptr ||
      currentCallParam.fieldBaseOpnd == nullptr) {
    return false;
  }
  if (!RegOperand::IsSameReg(*firstCallParam.objOpnd, *currentCallParam.objOpnd)) {
    return false;
  }
  return true;
}

void WriteFieldCallPattern::Run(BB &bb, Insn &insn) {
  paramDefInsns.clear();
  if (!CheckCondition(insn)) {
    return;
  }
  auto *aarCGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  MemOperand &addr =
      aarCGFunc->CreateMemOpnd(*currentCallParam.fieldBaseOpnd, currentCallParam.fieldOffset, k64BitSize);
  Insn &strInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(MOP_xstr, *currentCallParam.fieldValue, addr);
  strInsn.AppendComment("store reference field");
  strInsn.MarkAsAccessRefField(true);
  bb.InsertInsnAfter(insn, strInsn);
  for (Insn *paramDefInsn : paramDefInsns) {
    bb.RemoveInsn(*paramDefInsn);
  }
  bb.RemoveInsn(insn);
  prevCallInsn = &strInsn;
  nextInsn = strInsn.GetNextMachineInsn();
}

bool RemoveDecRefPattern::CheckCondition(Insn &insn) {
  if (insn.GetMachineOpcode() != MOP_xbl) {
    return false;
  }
  auto &target = static_cast<FuncNameOperand&>(insn.GetOperand(kInsnFirstOpnd));
  if (target.GetName() != "MCC_DecRef_NaiveRCFast") {
    return false;
  }
  prevInsn = insn.GetPreviousMachineInsn();
  if (prevInsn == nullptr) {
    return false;
  }
  MOperator mopMov = prevInsn->GetMachineOpcode();
  if ((mopMov != MOP_xmovrr && mopMov != MOP_xmovri64) ||
      static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd)).GetRegisterNumber() != R0) {
    return false;
  }
  Operand &srcOpndOfMov = prevInsn->GetOperand(kInsnSecondOpnd);
  if (!IsZeroRegister(srcOpndOfMov) &&
      !(srcOpndOfMov.IsImmediate() && static_cast<ImmOperand&>(srcOpndOfMov).GetValue() == 0)) {
    return false;
  }
  return true;
}

void RemoveDecRefPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  bb.RemoveInsn(*prevInsn);
  bb.RemoveInsn(insn);
}

/*
 * We optimize the following pattern in this function:
 * and x1, x1, #imm (is n power of 2)
 * cbz/cbnz x1, .label
 * =>
 * and x1, x1, #imm (is n power of 2)
 * tbnz/tbz x1, #n, .label
 */
void OneHoleBranchesAArch64::Run(BB &bb, Insn &insn) {
  AArch64CGFunc *aarch64CGFunc = static_cast<AArch64CGFunc*>(&cgFunc);
  if (&insn != bb.GetLastInsn()) {
    return;
  }
  /* check cbz/cbnz insn */
  MOperator thisMop = insn.GetMachineOpcode();
  if (thisMop != MOP_wcbz && thisMop != MOP_wcbnz && thisMop != MOP_xcbz && thisMop != MOP_xcbnz) {
    return;
  }
  /* check and insn */
  Insn *prevInsn = insn.GetPreviousMachineInsn();
  if (prevInsn == nullptr) {
    return;
  }
  MOperator prevMop = prevInsn->GetMachineOpcode();
  if (prevMop != MOP_wandrri12 && prevMop != MOP_xandrri13) {
    return;
  }
  /* check opearnd of two insns */
  if (&(prevInsn->GetOperand(kInsnFirstOpnd)) != &(insn.GetOperand(kInsnFirstOpnd))) {
    return;
  }
  auto &imm = static_cast<ImmOperand&>(prevInsn->GetOperand(kInsnThirdOpnd));
  int n = LogValueAtBase2(imm.GetValue());
  if (n < 0) {
    return;
  }

  /* replace insn */
  auto &label = static_cast<LabelOperand&>(insn.GetOperand(kInsnSecondOpnd));
  MOperator newOp = MOP_undef;
  switch (thisMop) {
    case MOP_wcbz:
      newOp = MOP_wtbz;
      break;
    case MOP_wcbnz:
      newOp = MOP_wtbnz;
      break;
    case MOP_xcbz:
      newOp = MOP_xtbz;
      break;
    case MOP_xcbnz:
      newOp = MOP_xtbnz;
      break;
    default:
      CHECK_FATAL(false, "can not touch here");
      break;
  }
  ImmOperand &oneHoleOpnd = aarch64CGFunc->CreateImmOperand(n, k8BitSize, false);
  (void)bb.InsertInsnAfter(insn, cgFunc.GetCG()->BuildInstruction<AArch64Insn>(
      newOp, prevInsn->GetOperand(kInsnSecondOpnd), oneHoleOpnd, label));
  bb.RemoveInsn(insn);
}

bool ReplaceIncDecWithIncPattern::CheckCondition(Insn &insn) {
  if (insn.GetMachineOpcode() != MOP_xbl) {
    return false;
  }
  target = &static_cast<FuncNameOperand&>(insn.GetOperand(kInsnFirstOpnd));
  if (target->GetName() != "MCC_IncDecRef_NaiveRCFast") {
    return false;
  }
  prevInsn = insn.GetPreviousMachineInsn();
  if (prevInsn == nullptr) {
    return false;
  }
  MOperator mopMov = prevInsn->GetMachineOpcode();
  if (mopMov != MOP_xmovrr) {
    return false;
  }
  if (static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd)).GetRegisterNumber() != R1 ||
      !IsZeroRegister(prevInsn->GetOperand(kInsnSecondOpnd))) {
    return false;
  }
  return true;
}

void ReplaceIncDecWithIncPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  std::string funcName = "MCC_IncRef_NaiveRCFast";
  GStrIdx strIdx = GlobalTables::GetStrTable().GetStrIdxFromName(funcName);
  MIRSymbol *st = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(strIdx, true);
  if (st == nullptr) {
    LogInfo::MapleLogger() << "WARNING: Replace IncDec With Inc fail due to no MCC_IncRef_NaiveRCFast func\n";
    return;
  }
  bb.RemoveInsn(*prevInsn);
  target->SetFunctionSymbol(*st);
}


void AndCmpBranchesToTbzAArch64::Run(BB &bb, Insn &insn) {
  AArch64CGFunc *aarch64CGFunc = static_cast<AArch64CGFunc*>(&cgFunc);
  if (&insn != bb.GetLastInsn()) {
    return;
  }
  MOperator mopB = insn.GetMachineOpcode();
  if (mopB != MOP_beq && mopB != MOP_bne) {
    return;
  }
  auto &label = static_cast<LabelOperand&>(insn.GetOperand(kInsnSecondOpnd));
  /* get the instruction before bne/beq, expects its type is cmp. */
  Insn *prevInsn = insn.GetPreviousMachineInsn();
  if (prevInsn == nullptr) {
    return;
  }
  MOperator prevMop = prevInsn->GetMachineOpcode();
  if (prevMop != MOP_wcmpri && prevMop != MOP_xcmpri) {
    return;
  }

  /* get the instruction before "cmp", expect its type is "and". */
  Insn *prevPrevInsn = prevInsn->GetPreviousMachineInsn();
  if (prevPrevInsn == nullptr) {
    return;
  }
  MOperator mopAnd = prevPrevInsn->GetMachineOpcode();
  if (mopAnd != MOP_wandrri12 && mopAnd != MOP_xandrri13) {
    return;
  }

  /*
   * check operand
   *
   * the real register of "cmp" and "and" must be the same.
   */
  if (&(prevInsn->GetOperand(kInsnSecondOpnd)) != &(prevPrevInsn->GetOperand(kInsnFirstOpnd))) {
    return;
  }

  uint32 opndIdx = 2;
  if (!prevPrevInsn->GetOperand(opndIdx).IsIntImmediate() || !prevInsn->GetOperand(opndIdx).IsIntImmediate()) {
    return;
  }
  auto &immAnd = static_cast<ImmOperand&>(prevPrevInsn->GetOperand(opndIdx));
  auto &immCmp = static_cast<ImmOperand&>(prevInsn->GetOperand(opndIdx));
  if (immCmp.GetValue() == 0) {
    int n = LogValueAtBase2(immAnd.GetValue());
    if (n < 0) {
      return;
    }
    /* judge whether the flag_reg and "w0" is live later. */
    auto &flagReg = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
    auto &cmpReg = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnSecondOpnd));
    if (FindRegLiveOut(flagReg, *prevInsn->GetBB()) || FindRegLiveOut(cmpReg, *prevInsn->GetBB())) {
      return;
    }
    MOperator mopNew = MOP_undef;
    switch (mopB) {
      case MOP_beq:
        if (mopAnd == MOP_wandrri12) {
          mopNew = MOP_wtbz;
        } else if (mopAnd == MOP_xandrri13) {
          mopNew = MOP_xtbz;
        }
        break;
      case MOP_bne:
        if (mopAnd == MOP_wandrri12) {
          mopNew = MOP_wtbnz;
        } else if (mopAnd == MOP_xandrri13) {
          mopNew = MOP_xtbnz;
        }
        break;
      default:
        CHECK_FATAL(false, "expects beq or bne insn");
        break;
    }
    ImmOperand &newImm = aarch64CGFunc->CreateImmOperand(n, k8BitSize, false);
    (void)bb.InsertInsnAfter(insn, cgFunc.GetCG()->BuildInstruction<AArch64Insn>(mopNew,
        prevPrevInsn->GetOperand(kInsnSecondOpnd), newImm, label));
    bb.RemoveInsn(insn);
    bb.RemoveInsn(*prevInsn);
    bb.RemoveInsn(*prevPrevInsn);
  } else {
    int n = LogValueAtBase2(immAnd.GetValue());
    int m = LogValueAtBase2(immCmp.GetValue());
    if (n < 0 || m < 0 || n != m) {
      return;
    }
    /* judge whether the flag_reg and "w0" is live later. */
    auto &flagReg = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
    auto &cmpReg = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnSecondOpnd));
    if (FindRegLiveOut(flagReg, *prevInsn->GetBB()) || FindRegLiveOut(cmpReg, *prevInsn->GetBB())) {
      return;
    }
    MOperator mopNew = MOP_undef;
    switch (mopB) {
      case MOP_beq:
        if (mopAnd == MOP_wandrri12) {
          mopNew = MOP_wtbnz;
        } else if (mopAnd == MOP_xandrri13) {
          mopNew = MOP_xtbnz;
        }
        break;
      case MOP_bne:
        if (mopAnd == MOP_wandrri12) {
          mopNew = MOP_wtbz;
        } else if (mopAnd == MOP_xandrri13) {
          mopNew = MOP_xtbz;
        }
        break;
      default:
        CHECK_FATAL(false, "expects beq or bne insn");
        break;
    }
    ImmOperand &newImm = aarch64CGFunc->CreateImmOperand(n, k8BitSize, false);
    (void)bb.InsertInsnAfter(insn, cgFunc.GetCG()->BuildInstruction<AArch64Insn>(mopNew,
        prevPrevInsn->GetOperand(kInsnSecondOpnd), newImm, label));
    bb.RemoveInsn(insn);
    bb.RemoveInsn(*prevInsn);
    bb.RemoveInsn(*prevPrevInsn);
  }
}

void RemoveSxtBeforeStrAArch64::Run(BB &bb, Insn &insn) {
  MOperator mop = insn.GetMachineOpcode();
  Insn *prevInsn = insn.GetPreviousMachineInsn();
  if (prevInsn == nullptr) {
    return;
  }
  MOperator prevMop = prevInsn->GetMachineOpcode();
  if (!(mop == MOP_wstrh && prevMop == MOP_xsxth32) && !(mop == MOP_wstrb && prevMop == MOP_xsxtb32)) {
    return;
  }
  auto &prevOpnd0 = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
  if (IfOperandIsLiveAfterInsn(prevOpnd0, insn)) {
    return;
  }
  auto &prevOpnd1 = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnSecondOpnd));
  regno_t prevRegNO0 = prevOpnd0.GetRegisterNumber();
  regno_t prevRegNO1 = prevOpnd1.GetRegisterNumber();
  regno_t regNO0 = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd)).GetRegisterNumber();
  if (prevRegNO0 != prevRegNO1) {
    return;
  }
  if (prevRegNO0 == regNO0) {
    bb.RemoveInsn(*prevInsn);
    return;
  }
  insn.SetOperand(0, prevOpnd1);
  bb.RemoveInsn(*prevInsn);
}

void UbfxToUxtwPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  Insn *newInsn = &cgFunc->GetCG()->BuildInstruction<AArch64Insn>(
      MOP_xuxtw64, insn.GetOperand(kInsnFirstOpnd), insn.GetOperand(kInsnSecondOpnd));
  bb.ReplaceInsn(insn, *newInsn);
  if (CG_PEEP_DUMP) {
    std::vector<Insn*> prevs;
    prevs.emplace_back(&insn);
    DumpAfterPattern(prevs, newInsn, nullptr);
  }
}

bool UbfxToUxtwPattern::CheckCondition(Insn &insn) {
  ImmOperand &imm0 = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
  ImmOperand &imm1 = static_cast<ImmOperand&>(insn.GetOperand(kInsnFourthOpnd));
  if ((imm0.GetValue() != 0) || (imm1.GetValue() != k32BitSize)) {
    return false;
  }
  return true;
}

void UbfxAndCbzToTbzPattern::Run(BB &bb, Insn &insn) {
  Operand &opnd2 = static_cast<Operand&>(insn.GetOperand(kInsnSecondOpnd));
  ImmOperand &imm3 = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
  if (!CheckCondition(insn)) {
    return;
  }
  auto &label = static_cast<LabelOperand&>(useInsn->GetOperand(kInsnSecondOpnd));
  MOperator nextMop = useInsn->GetMachineOpcode();
  switch (nextMop) {
    case MOP_wcbz:
    case MOP_xcbz:
      newMop = opnd2.GetSize() == k64BitSize ? MOP_xtbz : MOP_wtbz;
      break;
    case MOP_wcbnz:
    case MOP_xcbnz:
      newMop = opnd2.GetSize() == k64BitSize ? MOP_xtbnz : MOP_wtbnz;
      break;
    default:
      return;
  }
  if (newMop == MOP_undef) {
    return;
  }
  Insn *newInsn = &cgFunc->GetCG()->BuildInstruction<AArch64Insn>(newMop, opnd2, imm3, label);
  BB *useInsnBB = useInsn->GetBB();
  useInsnBB->ReplaceInsn(*useInsn, *newInsn);
  /* update ssa info */
  ssaInfo->ReplaceInsn(*useInsn, *newInsn);
  optSuccess = true;
  if (CG_PEEP_DUMP) {
    std::vector<Insn*> prevs;
    (void)prevs.emplace_back(useInsn);
    DumpAfterPattern(prevs, newInsn, nullptr);
  }
}

bool UbfxAndCbzToTbzPattern::CheckCondition(Insn &insn) {
  ImmOperand &imm4 = static_cast<ImmOperand&>(insn.GetOperand(kInsnFourthOpnd));
  RegOperand &opnd1 = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  InsnSet useInsns = GetAllUseInsn(opnd1);
  if (useInsns.size() != 1) {
    return false;
  }
  useInsn = *useInsns.begin();
  if (useInsn == nullptr) {
    return false;
  }
  if (imm4.GetValue() == 1) {
    switch (useInsn->GetMachineOpcode()) {
      case MOP_wcbz:
      case MOP_xcbz:
      case MOP_wcbnz:
      case MOP_xcbnz:
        return true;
      default:
        break;
    }
  }
  return false;
}

bool ComplexExtendWordLslAArch64::IsExtendWordLslPattern(const Insn &insn) const {
  Insn *nextInsn = insn.GetNext();
  if (nextInsn == nullptr) {
    return false;
  }
  MOperator nextMop = nextInsn->GetMachineOpcode();
  if (nextMop != MOP_xlslrri6) {
    return false;
  }
  return true;
}

void ComplexExtendWordLslAArch64::Run(BB &bb, Insn &insn) {
  if (!IsExtendWordLslPattern(insn)) {
    return;
  }
  MOperator mop = insn.GetMachineOpcode();
  Insn *nextInsn = insn.GetNext();
  auto &nextOpnd2 = static_cast<ImmOperand&>(nextInsn->GetOperand(kInsnThirdOpnd));
  if (nextOpnd2.GetValue() > k32BitSize) {
    return;
  }
  auto &opnd0 = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  auto &nextOpnd1 = static_cast<RegOperand&>(nextInsn->GetOperand(kInsnSecondOpnd));
  regno_t regNO0 = opnd0.GetRegisterNumber();
  regno_t nextRegNO1 = nextOpnd1.GetRegisterNumber();
  if (regNO0 != nextRegNO1 || IfOperandIsLiveAfterInsn(opnd0, *nextInsn)) {
    return;
  }
  auto &opnd1 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  auto &nextOpnd0 = static_cast<RegOperand&>(nextInsn->GetOperand(kInsnFirstOpnd));
  regno_t regNO1 = opnd1.GetRegisterNumber();
  cgFunc.InsertExtendSet(regNO1);
  MOperator mopNew = mop == MOP_xsxtw64 ? MOP_xsbfizrri6i6 : MOP_xubfizrri6i6;
  auto *aarch64CGFunc = static_cast<AArch64CGFunc*>(&cgFunc);
  RegOperand &reg1 = aarch64CGFunc->GetOrCreateVirtualRegisterOperand(regNO1);
  ImmOperand &newImm = aarch64CGFunc->CreateImmOperand(k32BitSize, k6BitSize, false);
  Insn &newInsnSbfiz = cgFunc.GetCG()->BuildInstruction<AArch64Insn>(mopNew,
      nextOpnd0, reg1, nextOpnd2, newImm);
  bb.RemoveInsn(*nextInsn);
  bb.ReplaceInsn(insn, newInsnSbfiz);
}
}  /* namespace maplebe */
