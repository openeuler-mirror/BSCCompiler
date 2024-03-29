/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "cg_irbuilder.h"
#include "aarch64_cg.h"
#include "aarch64_mem_reference.h"

namespace maplebe {
#define JAVALANG (cgFunc->GetMirModule().IsJavaModule())
#define CG_PEEP_DUMP CG_DEBUG_FUNC(*cgFunc)
namespace {
const std::string kMccLoadRef = "MCC_LoadRefField";
const std::string kMccLoadRefV = "MCC_LoadVolatileField";
const std::string kMccLoadRefS = "MCC_LoadRefStatic";
const std::string kMccLoadRefVS = "MCC_LoadVolatileStaticField";
const std::string kMccDummy = "MCC_Dummy";

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

MOperator GetMopUpdateAPSR(MOperator mop, bool &isAddShift) {
  MOperator newMop = MOP_undef;
  switch (mop) {
    case MOP_xaddrrr: {
      newMop = MOP_xaddsrrr;
      isAddShift = false;
      break;
    }
    case MOP_xaddrri12: {
      newMop = MOP_xaddsrri12;
      isAddShift = false;
      break;
    }
    case MOP_waddrrr: {
      newMop = MOP_waddsrrr;
      isAddShift = false;
      break;
    }
    case MOP_waddrri12: {
      newMop = MOP_waddsrri12;
      isAddShift = false;
      break;
    }
    case MOP_xaddrrrs: {
      newMop = MOP_xaddsrrrs;
      isAddShift = true;
      break;
    }
    case MOP_waddrrrs: {
      newMop = MOP_waddsrrrs;
      isAddShift = true;
      break;
    }
    default:
      break;
  }
  return newMop;
}

void AArch64CGPeepHole::Run() {
  bool optSuccess = false;
  FOR_ALL_BB(bb, cgFunc) {
    FOR_BB_INSNS_SAFE(insn, bb, nextInsn) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      if (ssaInfo != nullptr) {
        optSuccess |= DoSSAOptimize(*bb, *insn);
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
  manager->SetOptSuccess(false);
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
      manager->Optimize<AndAndCmpBranchesToTstPattern>(true);
      break;
    }
    case MOP_wcsetrc:
    case MOP_xcsetrc: {
      manager->Optimize<AndAndCmpBranchesToTstPattern>(true);
      manager->Optimize<AndCmpBranchesToCsetPattern>(true);
      manager->Optimize<ContinuousCmpCsetPattern>(true);
      break;
    }
    case MOP_waddrrr:
    case MOP_xaddrrr: {
      manager->Optimize<SimplifyMulArithmeticPattern>(true);
      manager->Optimize<CsetToCincPattern>(true);
      break;
    }
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
      manager->Optimize<LslAndToUbfizPattern>(true);
      manager->Optimize<ElimSpecificExtensionPattern>(true);
      break;
    }
    case MOP_wcselrrrc:
    case MOP_xcselrrrc: {
      manager->Optimize<AndAndCmpBranchesToTstPattern>(true);
      manager->Optimize<CselToCsetPattern>(true);
      manager->Optimize<CselToCsincRemoveMovPattern>(true);
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
      manager->Optimize<LslAndToUbfizPattern>(true);
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
    case MOP_waddrri12:
    case MOP_xaddrri12:
    case MOP_wsubrri12:
    case MOP_xsubrri12: {
      manager->Optimize<CombineSameArithmeticPattern>(true);
      break;
    }
    case MOP_wlslrri5: {
      manager->Optimize<CombineSameArithmeticPattern>(true);
      manager->Optimize<LslAndToUbfizPattern>(true);
      break;
    }
    case MOP_wubfxrri5i5:
    case MOP_xubfxrri6i6: {
      manager->Optimize<UbfxAndCbzToTbzPattern>(true);
      break;
    }
    case MOP_xmulrrr:
    case MOP_wmulrrr: {
      manager->Optimize<MulImmToShiftPattern>(!cgFunc->IsAfterRegAlloc());
      break;
    }
    case MOP_wcmpri:
    case MOP_xcmpri: {
      manager->Optimize<AddCmpZeroPatternSSA>(true);
      break;
    }
    default:
      break;
  }
  return manager->OptSuccess();
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
  if (!AArch64isa::CheckCondCode(condOpnd1)) {
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
    CondOperand &newCondOpnd = aarFunc->GetCondOperand(AArch64isa::GetReverseCC(prevCsetCondOpnd.GetCode()));
    regno_t tmpRegNO = 0;
    auto *tmpDefOpnd = aarFunc->CreateVirtualRegisterOperand(tmpRegNO,
        resOpnd.GetSize(), static_cast<RegOperand&>(resOpnd).GetRegisterType());
    tmpDefOpnd->SetValidBitsNum(k1BitSize);
    newCsetInsn = &cgFunc->GetInsnBuilder()->BuildInsn(
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
    newInsn = &cgFunc->GetInsnBuilder()->BuildInsn(
        newMop, insn.GetOperand(kInsnFirstOpnd), prevCsetInsn1->GetOperand(kInsnFirstOpnd));
  } else {
    newInsn = &cgFunc->GetInsnBuilder()->BuildInsn(
        newMop, insn.GetOperand(kInsnFirstOpnd), newCsetInsn->GetOperand(kInsnFirstOpnd));
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
  /*
   * If the value of srcOpnd of neg is 0, we can not do this optimization.
   * Because
   *   for cmp(subs): if NOT(0), the add calculation may overflow
   *   for cmn(adds): it has handled overflows before calling the addWithCarry function
   * When the value of srcOpnd of neg is 0, (neg + cmp) and (cmn) set different (c) and (v) in condition flags.
   *
   * But we can not get the value of register, so we can only restrict the condition codes which use (c) and (v) flags.
   */
  auto &ccReg = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  InsnSet useInsns = GetAllUseInsn(ccReg);
  for (auto *useInsn : useInsns) {
    if (useInsn == nullptr) {
      continue;
    }
    MOperator useMop = useInsn->GetMachineOpcode();
    if (useInsn->IsCondBranch() && useMop != MOP_beq && useMop != MOP_bne && useMop != MOP_bmi && useMop != MOP_bpl) {
      return false;
    }
    bool hasUnsupportedCode = false;
    for (uint32 i = 0; i < useInsn->GetOperandSize(); ++i) {
      if (useInsn->GetOperand(i).GetKind() == Operand::kOpdCond) {
        ConditionCode cond = static_cast<CondOperand&>(useInsn->GetOperand(i)).GetCode();
        /* in case of ignoring v flag
         *  adds xt, x0, x1 (0x8000000000000000) -> not set v
         *  ==>
         *  neg x1 x1 (0x8000000000000000) which is same for negative 0
         *  subs xt, x0, x1 () -> set v
         */
        if (cond != CC_EQ && cond != CC_NE && cond != CC_MI && cond != CC_PL) {
          hasUnsupportedCode = true;
          break;
        }
      }
    }
    if (hasUnsupportedCode) {
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
    newInsn = &(cgFunc->GetInsnBuilder()->BuildInsn(newMop, ccReg, opnd1, opnd2));
  } else {
    /* prevMop == MOP_winegrrs || prevMop == MOP_xinegrrs */
    MOperator newMop = (currMop == MOP_wcmprr) ? MOP_wcmnrrs : MOP_xcmnrrs;
    Operand &shiftOpnd = prevInsn->GetOperand(kInsnThirdOpnd);
    newInsn = &(cgFunc->GetInsnBuilder()->BuildInsn(newMop, ccReg, opnd1, opnd2, shiftOpnd));
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
  prevLdr1->SetMOP(AArch64CG::kMd[MOP_xldr]);
  prevLdr2->SetMOP(AArch64CG::kMd[MOP_xldr]);
  prevCmp->SetMOP(AArch64CG::kMd[MOP_xcmprr]);
}

bool LdrCmpPattern::CheckCondition(Insn &insn) {
  /* a pattern which breaks cfg
   * it is more suitable for peephole after pgo using */
  if (CGOptions::DoLiteProfGen() || CGOptions::DoLiteProfUse()) {
    return false;
  }
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
  if (currInsn->GetBB()->GetFirstMachineInsn() != ldr1) {
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
  if (!IsBne(prevBB->GetLastMachineInsn())) {
    return false;
  }
  bne2 = prevBB->GetLastMachineInsn();
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

bool LdrCmpPattern::CheckInsns() const {
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
  if (m1.GetAddrMode() != MemOperand::kBOI) {
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

MOperator CsetCbzToBeqPattern::SelectNewMop(ConditionCode condCode, bool inverse) const {
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
  Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(newMop, prevInsn->GetOperand(kInsnThirdOpnd), labelOpnd);
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
  Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(
      newMop, insn.GetOperand(kInsnFirstOpnd), prevSrcReg, newImmOpnd1, newImmOpnd2);
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
    case MOP_wmovri32:
    case MOP_xmovri64: {
      auto &immOpnd = static_cast<ImmOperand&>(insn.GetOperand(kInsnSecondOpnd));
      return immOpnd.IsZero();
    }
    default:
      return false;
  }
}

bool CselToCsetPattern::IsOpndDefByOne(const Insn &insn) const {
  MOperator movMop = insn.GetMachineOpcode();
  if ((movMop != MOP_wmovri32) && (movMop != MOP_xmovri64)) {
    return false;
  }
  auto &immOpnd = static_cast<ImmOperand&>(insn.GetOperand(kInsnSecondOpnd));
  return immOpnd.IsOne();
}

bool CselToCsetPattern::IsOpndDefByAllOnes(const Insn &insn) const {
  MOperator movMop = insn.GetMachineOpcode();
  if ((movMop != MOP_wmovri32) && (movMop != MOP_xmovri64)) {
    return false;
  }
  bool is32Bits = (insn.GetOperandSize(kInsnFirstOpnd) == k32BitSize);
  auto &immOpnd = static_cast<ImmOperand&>(insn.GetOperand(kInsnSecondOpnd));
  return immOpnd.IsAllOnes() || (is32Bits && immOpnd.IsAllOnes32bit()) ;
}

bool CselToCsetPattern::CheckZeroCondition(const Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  if (curMop != MOP_wcselrrrc && curMop != MOP_xcselrrrc) {
    return false;
  }
  RegOperand &useReg1 = static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd));
  RegOperand &useReg2 = static_cast<RegOperand &>(insn.GetOperand(kInsnThirdOpnd));
  if ((useReg1.GetRegisterNumber() == RZR && useReg2.GetRegisterNumber() == RZR) ||
      (useReg1.GetRegisterNumber() != RZR && useReg2.GetRegisterNumber() != RZR)) {
    return false;
  }
  isZeroBefore = (useReg1.GetRegisterNumber() == RZR);
  useReg = isZeroBefore ? &useReg2 : &useReg1;
  if (ssaInfo) {
    prevMovInsn = ssaInfo->GetDefInsn(*useReg);
  } else {
    prevMovInsn = insn.GetPreviousMachineInsn();
  }
  if (prevMovInsn == nullptr) {
    return false;
  }
  MOperator prevMop = prevMovInsn->GetMachineOpcode();
  if (prevMop != MOP_wmovri32 && prevMop != MOP_xmovri64) {
    return false;
  }
  if (prevMovInsn->GetOperandSize(kInsnFirstOpnd) != insn.GetOperandSize(kInsnFirstOpnd)) {
    return false;
  }
  if (!ssaInfo && (useReg->GetRegisterNumber() !=
                   static_cast<RegOperand &>(prevMovInsn->GetOperand(kInsnFirstOpnd)).GetRegisterNumber())) {
    return false;
  }
  ImmOperand &immOpnd = static_cast<ImmOperand &>(prevMovInsn->GetOperand(kInsnSecondOpnd));
  isOne = immOpnd.IsOne();
  isAllOnes = (prevMop == MOP_xmovri64 && immOpnd.IsAllOnes()) || (prevMop == MOP_wmovri32 && immOpnd.IsAllOnes32bit());
  if (!isOne && !isAllOnes) {
    return false;
  }
  return true;
}

bool CselToCsetPattern::CheckCondition(Insn &insn) {
  if (CheckZeroCondition(insn)) {
    return true;
  }
  if (!ssaInfo) {
    return false;
  }
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
  if (prevMop1 != MOP_wmovri32 && prevMop1 != MOP_xmovri64 &&
      prevMop1 != MOP_wmovrr && prevMop1 != MOP_xmovrr) {
    return false;
  }
  auto &useOpnd2 = static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd));
  prevMovInsn2 = ssaInfo->GetDefInsn(useOpnd2);
  if (prevMovInsn2 == nullptr) {
    return false;
  }
  MOperator prevMop2 = prevMovInsn2->GetMachineOpcode();
  if (prevMop2 != MOP_wmovri32 && prevMop2 != MOP_xmovri64 &&
      prevMop2 != MOP_wmovrr && prevMop2 != MOP_xmovrr) {
    return false;
  }
  return true;
}

Insn *CselToCsetPattern::BuildCondSetInsn(const Insn &cselInsn) const {
  RegOperand &dest = static_cast<RegOperand &>(cselInsn.GetOperand(kInsnFirstOpnd));
  bool is32Bits = (cselInsn.GetOperandSize(kInsnFirstOpnd) == k32BitSize);
  ConditionCode ccCode = static_cast<CondOperand &>(cselInsn.GetOperand(kInsnFourthOpnd)).GetCode();
  ASSERT(ccCode != kCcLast, "unknown cond, ccCode can't be kCcLast");
  AArch64CGFunc *func = static_cast<AArch64CGFunc *>(cgFunc);
  Operand &rflag = func->GetOrCreateRflag();
  if (isZeroBefore) {
    CondOperand &cond = func->GetCondOperand(AArch64isa::GetReverseCC(ccCode));
    if (isOne) {
      return &cgFunc->GetInsnBuilder()->BuildInsn((is32Bits ? MOP_wcsetrc : MOP_xcsetrc), dest, cond, rflag);
    } else if (isAllOnes) {
      return &cgFunc->GetInsnBuilder()->BuildInsn((is32Bits ? MOP_wcsetmrc : MOP_xcsetmrc), dest, cond, rflag);
    }
  } else {
    CondOperand &cond = func->GetCondOperand(ccCode);
    if (isOne) {
      return &cgFunc->GetInsnBuilder()->BuildInsn((is32Bits ? MOP_wcsetrc : MOP_xcsetrc), dest, cond, rflag);
    } else if (isAllOnes) {
      return &cgFunc->GetInsnBuilder()->BuildInsn((is32Bits ? MOP_wcsetmrc : MOP_xcsetmrc), dest, cond, rflag);
    }
  }
  return nullptr;
}

void CselToCsetPattern::ZeroRun(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  Insn *newInsn = BuildCondSetInsn(insn);
  if (newInsn == nullptr) {
    return;
  }
  bb.ReplaceInsn(insn, *newInsn);
  if (ssaInfo) {
    // update ssa info
    ssaInfo->ReplaceInsn(insn, *newInsn);
  } else if (static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd)).GetRegisterNumber() ==
             useReg->GetRegisterNumber()) {
    bb.RemoveInsn(*prevMovInsn);
  }
  optSuccess = true;
  SetCurrInsn(newInsn);
  // dump pattern info
  if (CG_PEEP_DUMP) {
    std::vector<Insn *> prevs;
    prevs.emplace_back(prevMovInsn);
    DumpAfterPattern(prevs, &insn, newInsn);
  }
}

void CselToCsetPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  if (CheckZeroCondition(insn)) {
    ZeroRun(bb, insn);
    return;
  }
  Operand &dstOpnd = insn.GetOperand(kInsnFirstOpnd);
  uint32 dstOpndSize = insn.GetOperandSize(kInsnFirstOpnd);
  MOperator newMop = MOP_undef;
  Operand &condOpnd = insn.GetOperand(kInsnFourthOpnd);
  Operand &rflag = insn.GetOperand(kInsnFifthOpnd);
  Insn *newInsn = nullptr;
  if (IsOpndDefByZero(*prevMovInsn2)) {
    if (IsOpndDefByOne(*prevMovInsn1)) {
      newMop = (dstOpndSize == k64BitSize ? MOP_xcsetrc : MOP_wcsetrc);
    } else if (IsOpndDefByAllOnes(*prevMovInsn1)) {
      newMop = (dstOpndSize == k64BitSize ? MOP_xcsetmrc : MOP_wcsetmrc);
    }
    newInsn = &(cgFunc->GetInsnBuilder()->BuildInsn(newMop, dstOpnd, condOpnd, rflag));
  } else if (IsOpndDefByZero(*prevMovInsn1)) {
    auto &origCondOpnd = static_cast<CondOperand&>(condOpnd);
    ConditionCode inverseCondCode = AArch64isa::GetReverseCC(origCondOpnd.GetCode());
    if (inverseCondCode == kCcLast) {
      return;
    }
    auto *aarFunc = static_cast<AArch64CGFunc*>(cgFunc);
    CondOperand &inverseCondOpnd = aarFunc->GetCondOperand(inverseCondCode);
    if (IsOpndDefByOne(*prevMovInsn2)) {
      newMop = (dstOpndSize == k64BitSize ? MOP_xcsetrc : MOP_wcsetrc);
    } else if (IsOpndDefByAllOnes(*prevMovInsn1)) {
      newMop = (dstOpndSize == k64BitSize ? MOP_xcsetmrc : MOP_wcsetmrc);
    }
    newInsn = &(cgFunc->GetInsnBuilder()->BuildInsn(newMop, dstOpnd, inverseCondOpnd, rflag));
  }
  if (newMop == MOP_undef || newInsn == nullptr) {
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

bool CselToMovPattern::CheckCondition(Insn &insn) {
  MOperator mop = insn.GetMachineOpcode();
  if (mop != MOP_wcselrrrc && mop != MOP_xcselrrrc) {
    return false;
  }

  if (!RegOperand::IsSameReg(insn.GetOperand(kInsnSecondOpnd), insn.GetOperand(kInsnThirdOpnd))) {
    return false;
  }

  return true;
}

void CselToMovPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }

  MOperator newMop = insn.GetMachineOpcode() == MOP_wcselrrrc ? MOP_wmovrr : MOP_xmovrr;
  Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(newMop, insn.GetOperand(kInsnFirstOpnd),
      insn.GetOperand(kInsnSecondOpnd));

  bb.ReplaceInsn(insn, newInsn);
}

bool CselToCsincRemoveMovPattern::IsOpndMovOneAndNewOpndOpt(const Insn &curInsn) {
  auto &insnThirdOpnd = static_cast<RegOperand&>(curInsn.GetOperand(kInsnThirdOpnd));
  auto &insnSecondOpnd = static_cast<RegOperand&>(curInsn.GetOperand(kInsnSecondOpnd));
  auto &origCondOpnd = static_cast<CondOperand&>(curInsn.GetOperand(kInsnFourthOpnd));
  Insn *insnThirdOpndDefInsn = ssaInfo->GetDefInsn(insnThirdOpnd);
  Insn *insnSecondOpndDefInsn = ssaInfo->GetDefInsn(insnSecondOpnd);
  if (insnThirdOpndDefInsn == nullptr || insnSecondOpndDefInsn == nullptr) {
    return false;
  }
  MOperator insnThirdOpndDefMop = insnThirdOpndDefInsn->GetMachineOpcode();
  MOperator insnSecondOpndDefMop = insnSecondOpndDefInsn->GetMachineOpcode();
  if (insnThirdOpndDefMop == MOP_wmovri32 || insnThirdOpndDefMop == MOP_xmovri64) {
    prevMovInsn = insnThirdOpndDefInsn;
  } else if (insnSecondOpndDefMop == MOP_wmovri32 || insnSecondOpndDefMop == MOP_xmovri64) {
    prevMovInsn = insnSecondOpndDefInsn;
    needReverseCond = true;
  } else {
    return false;
  }
  auto &prevMovImmOpnd = static_cast<ImmOperand&>(prevMovInsn->GetOperand(kInsnSecondOpnd));
  auto val = prevMovImmOpnd.GetValue();
  if (val != 1) {
    return false;
  }
  if (needReverseCond) {
    newSecondOpnd = &insnThirdOpnd;
    ConditionCode inverseCondCode = AArch64isa::GetReverseCC(origCondOpnd.GetCode());
    if (inverseCondCode == kCcLast) {
      return false;
    }
    auto *aarFunc = static_cast<AArch64CGFunc*>(cgFunc);
    CondOperand &inverseCondOpnd = aarFunc->GetCondOperand(inverseCondCode);
    cond = &inverseCondOpnd;
  } else {
    newSecondOpnd = &insnSecondOpnd;
    cond = &origCondOpnd;
  }
  return true;
}

bool CselToCsincRemoveMovPattern::CheckCondition(Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  if (curMop != MOP_xcselrrrc && curMop != MOP_wcselrrrc) {
    return false;
  }
  if (!IsOpndMovOneAndNewOpndOpt(insn)) {
    return false;
  }
  return true;
}

void CselToCsincRemoveMovPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  uint32 dstOpndSize = insn.GetOperandSize(kInsnFirstOpnd);
  MOperator newMop = (dstOpndSize == k64ByteSize) ? MOP_xcsincrrrc : MOP_wcsincrrrc;
  Operand &ccReg = insn.GetOperand(kInsnFifthOpnd);
  RegOperand &zeroOpnd = cgFunc->GetZeroOpnd(dstOpndSize);
  auto &insnFirstOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(newMop, insnFirstOpnd, *static_cast<RegOperand*>(newSecondOpnd),
                                                      zeroOpnd, *static_cast<CondOperand*>(cond), ccReg);
  bb.ReplaceInsn(insn, newInsn);
  // updata ssa info
  ssaInfo->ReplaceInsn(insn, newInsn);
  optSuccess = true;
  SetCurrInsn(&newInsn);
  // dump pattern info
  if (CG_PEEP_DUMP) {
    std::vector<Insn *> prevs;
    prevs.emplace_back(prevMovInsn);
    DumpAfterPattern(prevs, &insn, &newInsn);
  }
}

bool CsetToCincPattern::CheckDefInsn(const RegOperand &opnd, Insn &insn) {
  Insn *tempDefInsn = ssaInfo->GetDefInsn(opnd);
  if (tempDefInsn != nullptr && tempDefInsn->GetBB()->GetId() == insn.GetBB()->GetId()) {
    InsnSet useInsns = GetAllUseInsn(opnd);
    if (useInsns.size() != 1) {
      return false;
    }
    MOperator mop = tempDefInsn->GetMachineOpcode();
    if (mop == MOP_wcsetrc || mop == MOP_xcsetrc) {
      /* DefInsn and tempDefInsn are in the same BB. Select a close to useInsn(add) */
      if (!CheckRegTyCc(*tempDefInsn, insn)) {
        return false;
      }
      defInsn = tempDefInsn;
      return true;
    }
  }
  return false;
}

/* If a new ConditionCode is generated after csetInsn, this optimization is not performed. */
bool CsetToCincPattern::CheckRegTyCc(const Insn &tempDefInsn, Insn &insn) const {
  bool betweenUseAndDef = false;
  FOR_BB_INSNS_REV(bbInsn, insn.GetBB()) {
    if (!bbInsn->IsMachineInstruction()) {
      continue;
    }
    if (bbInsn->GetId() == insn.GetId()) {
      betweenUseAndDef = true;
    }
    if (betweenUseAndDef) {
      /* Select a close to useInsn(add) */
      if (defInsn != nullptr && bbInsn->GetId() == defInsn->GetId()) {
        return false;
      } else if (bbInsn->GetId() == tempDefInsn.GetId()) {
        return true;
      } else if (static_cast<RegOperand&>(bbInsn->GetOperand(kInsnFirstOpnd)).IsOfCC()) {
        return false;
      }
    }
  }
  return false;
}

bool CsetToCincPattern::CheckCondition(Insn &insn) {
  RegOperand &opnd2 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  RegOperand &opnd3 = static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd));
  bool opnd2Cset = CheckDefInsn(opnd2, insn);
  bool opnd3Cset = CheckDefInsn(opnd3, insn);
  if (opnd3Cset) {
    csetOpnd1 = kInsnThirdOpnd;
    return true;
  } else if (opnd2Cset) {
    csetOpnd1 = kInsnSecondOpnd;
    return true;
  }
  return false;
}

void CsetToCincPattern::Run(BB &bb, Insn &insn) {
  RegOperand &opnd1 = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  /* Exclude other patterns that have been optimized. */
  Insn *newAddInsn = ssaInfo->GetDefInsn(opnd1);
  if (newAddInsn == nullptr) {
    return;
  }
  MOperator mop = newAddInsn->GetMachineOpcode();
  if (mop != MOP_waddrrr && mop != MOP_xaddrrr) {
    return;
  }
  if (!CheckCondition(insn) || defInsn == nullptr || csetOpnd1 == 0) {
    return;
  }
  MOperator newMop = (insn.GetMachineOpcode() == MOP_waddrrr) ? MOP_wcincrc : MOP_xcincrc;
  int32 cincOpnd2 = (csetOpnd1 == kInsnSecondOpnd) ? kInsnThirdOpnd : kInsnSecondOpnd;
  RegOperand &opnd2 = static_cast<RegOperand&>(insn.GetOperand(static_cast<uint32>(cincOpnd2)));
  Operand &condOpnd = defInsn->GetOperand(kInsnSecondOpnd);
  Operand &rflag = defInsn->GetOperand(kInsnThirdOpnd);
  Insn *newInsn = &(cgFunc->GetInsnBuilder()->BuildInsn(newMop, opnd1, opnd2, condOpnd, rflag));
  bb.ReplaceInsn(insn, *newInsn);
  /* update ssa info */
  ssaInfo->ReplaceInsn(insn, *newInsn);
  optSuccess = true;
  SetCurrInsn(newInsn);
  /* dump pattern info */
  if (CG_PEEP_DUMP) {
    std::vector<Insn*> prevs;
    (void)prevs.emplace_back(defInsn);
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
  Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(
      newMop, prevAndInsn->GetOperand(kInsnSecondOpnd), tbzImmOpnd, labelOpnd);
  if (!VERIFY_INSN(&newInsn)) {
    return;
  }
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

bool AndAndCmpBranchesToTstPattern::CheckAndSelectPattern() {
  MOperator prevAndMop = prevAndInsn->GetMachineOpcode();
  MOperator prevPrevAndMop = prevPrevAndInsn->GetMachineOpcode();
  if (prevAndMop != prevPrevAndMop) {
    return false;
  }
  auto &prevAndImmOpnd = static_cast<ImmOperand&>(prevAndInsn->GetOperand(kInsnThirdOpnd));
  auto &prevPrevAndImmOpnd = static_cast<ImmOperand&>(prevPrevAndInsn->GetOperand(kInsnThirdOpnd));
  if (prevAndImmOpnd.GetValue() == prevPrevAndImmOpnd.GetValue() &&
      ((static_cast<uint64>(prevAndImmOpnd.GetValue()) & static_cast<uint64>(prevAndImmOpnd.GetValue() + 1)) == 0) &&
      ((static_cast<uint64>(prevPrevAndImmOpnd.GetValue()) &
       static_cast<uint64>(prevPrevAndImmOpnd.GetValue() + 1)) == 0)) {
    bool isWOrX = (prevAndMop == MOP_wandrri12 && prevPrevAndMop == MOP_wandrri12);
    newEorMop = isWOrX ? MOP_weorrrr : MOP_xeorrrr;
    newTstMop = isWOrX ? MOP_wtstri32 : MOP_xtstri64;
    tstImmVal = prevAndImmOpnd.GetValue();
    return true;
  }
  return false;
}

bool AndAndCmpBranchesToTstPattern::CheckCondInsn(const Insn &insn) {
  if (insn.GetMachineOpcode() == MOP_bne || insn.GetMachineOpcode() == MOP_beq) {
    ccReg = static_cast<RegOperand*>(&insn.GetOperand(kInsnFirstOpnd));
    return true;
  }
  if (!insn.IsCondDef()) {
    return false;
  }
  CondOperand *condOpnd = nullptr;
  for (uint32 i = 0; i < insn.GetOperandSize(); ++i) {
    if (insn.GetDesc()->GetOpndDes(i) == &OpndDesc::Cond) {
      condOpnd = static_cast<CondOperand*>(&insn.GetOperand(i));
    } else if (insn.GetDesc()->GetOpndDes(i) == &OpndDesc::CCS) {
      ccReg = static_cast<RegOperand*>(&insn.GetOperand(i));
    }
  }
  if (condOpnd == nullptr || ccReg == nullptr) {
    return false;
  }
  return (condOpnd->GetCode() == CC_NE || condOpnd->GetCode() == CC_EQ);
}

Insn *AndAndCmpBranchesToTstPattern::CheckAndGetPrevAndDefInsn(const RegOperand &regOpnd) const {
  if (!regOpnd.IsSSAForm()) {
    return nullptr;
  }
  auto *regVersion = ssaInfo->FindSSAVersion(regOpnd.GetRegisterNumber());
  ASSERT(regVersion != nullptr, "UseVRegVersion must not be null based on ssa");
  if (regVersion->GetAllUseInsns().size() != 1) { // only one use point can do opt
    return nullptr;
  }
  auto *defInfo = regVersion->GetDefInsnInfo();
  if (defInfo == nullptr) {
    return nullptr;
  }
  auto andMop = defInfo->GetInsn()->GetMachineOpcode();
  if (andMop != MOP_wandrri12 && andMop != MOP_xandrri13) {
    return nullptr;
  }
  return defInfo->GetInsn();
}

bool AndAndCmpBranchesToTstPattern::CheckCondition(Insn &insn) {
  if (!CheckCondInsn(insn)) {
    return false;
  }
  prevCmpInsn = ssaInfo->GetDefInsn(*ccReg);
  if (prevCmpInsn == nullptr) {
    return false;
  }
  MOperator prevCmpMop = prevCmpInsn->GetMachineOpcode();
  if (prevCmpMop != MOP_wcmprr && prevCmpMop != MOP_xcmprr) {
    return false;
  }
  prevAndInsn = CheckAndGetPrevAndDefInsn(static_cast<RegOperand&>(prevCmpInsn->GetOperand(kInsnThirdOpnd)));
  if (prevAndInsn == nullptr) {
    return false;
  }
  prevPrevAndInsn = CheckAndGetPrevAndDefInsn(static_cast<RegOperand&>(prevCmpInsn->GetOperand(kInsnSecondOpnd)));
  if (prevPrevAndInsn == nullptr) {
    return false;
  }
  if (!CheckAndSelectPattern()) {
    return false;
  }
  return true;
}

void AndAndCmpBranchesToTstPattern ::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  auto *a64Func = static_cast<AArch64CGFunc*>(cgFunc);
  regno_t tmpRegNO = 0;
  Operand &andOpnd = prevAndInsn->GetOperand(kInsnSecondOpnd);
  auto *tmpDefOpnd = a64Func->CreateVirtualRegisterOperand(tmpRegNO, andOpnd.GetSize(),
                                                           static_cast<RegOperand&>(andOpnd).GetRegisterType());
  Insn &newEorInsn = cgFunc->GetInsnBuilder()->BuildInsn(newEorMop, *tmpDefOpnd,
                                                         prevPrevAndInsn->GetOperand(kInsnSecondOpnd),
                                                         prevAndInsn->GetOperand(kInsnSecondOpnd));
  BB *preCmpBB = prevCmpInsn->GetBB();
  (void)preCmpBB->InsertInsnBefore(*prevCmpInsn, newEorInsn);
  /* update ssa info */
  ssaInfo->CreateNewInsnSSAInfo(newEorInsn);
  ImmOperand &tstImmOpnd = a64Func->CreateImmOperand(tstImmVal, k8BitSize, false);
  Insn &newTstInsn = cgFunc->GetInsnBuilder()->BuildInsn(newTstMop, prevCmpInsn->GetOperand(kInsnFirstOpnd),
                                                         newEorInsn.GetOperand(kInsnFirstOpnd), tstImmOpnd);
  bb.ReplaceInsn(*prevCmpInsn, newTstInsn);
  /* update ssa info */
  ssaInfo->ReplaceInsn(*prevCmpInsn, newTstInsn);
  optSuccess = true;
  /* dump pattern info */
  if (CG_PEEP_DUMP) {
    std::vector<Insn*> prevs;
    prevs.emplace_back(prevPrevAndInsn);
    prevs.emplace_back(prevAndInsn);
    prevs.emplace_back(prevCmpInsn);
    DumpAfterPattern(prevs, &newEorInsn, &newTstInsn);
  }
}

bool ZeroCmpBranchesToTbzPattern::CheckAndSelectPattern(const Insn &currInsn) {
  MOperator currMop = currInsn.GetMachineOpcode();
  MOperator prevMop = preInsn->GetMachineOpcode();
  switch (prevMop) {
    case MOP_wcmpri:
    case MOP_xcmpri: {
      regOpnd = &static_cast<RegOperand&>(preInsn->GetOperand(kInsnSecondOpnd));
      auto &immOpnd = static_cast<ImmOperand&>(preInsn->GetOperand(kInsnThirdOpnd));
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
      auto &regOpnd0 = static_cast<RegOperand&>(preInsn->GetOperand(kInsnSecondOpnd));
      auto &regOpnd1 = static_cast<RegOperand&>(preInsn->GetOperand(kInsnThirdOpnd));
      if (!IsZeroRegister(regOpnd0) && !IsZeroRegister(regOpnd1)) {
        return false;
      }
      switch (currMop) {
        case MOP_bge:
          if (IsZeroRegister(regOpnd1)) {
            regOpnd = &static_cast<RegOperand&>(preInsn->GetOperand(kInsnSecondOpnd));
            newMop = (regOpnd->GetSize() <= k32BitSize) ? MOP_wtbz : MOP_xtbz;
          } else {
            return false;
          }
          break;
        case MOP_ble:
          if (IsZeroRegister(regOpnd0)) {
            regOpnd = &static_cast<RegOperand&>(preInsn->GetOperand(kInsnThirdOpnd));
            newMop = (regOpnd->GetSize() <= k32BitSize) ? MOP_wtbz : MOP_xtbz;
          } else {
            return false;
          }
          break;
        case MOP_blt:
          if (IsZeroRegister(regOpnd1)) {
            regOpnd = &static_cast<RegOperand&>(preInsn->GetOperand(kInsnSecondOpnd));
            newMop = (regOpnd->GetSize() <= k32BitSize) ? MOP_wtbnz : MOP_xtbnz;
          } else {
            return false;
          }
          break;
        case MOP_bgt:
          if (IsZeroRegister(regOpnd0)) {
            regOpnd = &static_cast<RegOperand&>(preInsn->GetOperand(kInsnThirdOpnd));
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
  preInsn = ssaInfo->GetDefInsn(ccReg);
  if (preInsn == nullptr) {
    return false;
  }
  MOperator prevMop = preInsn->GetMachineOpcode();
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
  Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(newMop, *static_cast<RegOperand*>(regOpnd), bitOpnd, labelOpnd);
  if (!VERIFY_INSN(&newInsn)) {
    return;
  }
  bb.ReplaceInsn(insn, newInsn);
  /* update ssa info */
  ssaInfo->ReplaceInsn(insn, newInsn);
  optSuccess = true;
  SetCurrInsn(&newInsn);
  /* dump pattern info */
  if (CG_PEEP_DUMP) {
    std::vector<Insn*> prevs;
    prevs.emplace_back(preInsn);
    DumpAfterPattern(prevs, &insn, &newInsn);
  }
}

bool LsrAndToUbfxPattern::CheckIntersectedCondition(const Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  MOperator prevMop = prevInsn->GetMachineOpcode();
  int64 lsb = static_cast<ImmOperand&>(prevInsn->GetOperand(kInsnThirdOpnd)).GetValue();
  int64 width = __builtin_popcountll(static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd)).GetValue());
  if (lsb + width <= k32BitSize) {
    return true;
  } else if (curMop == MOP_wandrri12 && prevMop == MOP_xlsrrri6 && lsb >= k32BitSize && (lsb + width) <= k64BitSize) {
    isWXSumOutOfRange = true;
    return isWXSumOutOfRange;
  }
  return false;
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
  if (((curMop == MOP_wandrri12 && prevMop == MOP_xlsrrri6) || (curMop == MOP_xandrri13 && prevMop == MOP_wlsrrri5)) &&
    !CheckIntersectedCondition(insn)) {
    return false;
  }
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
  // If isWXSumOutOfRange returns true, newInsn will be 64bit
  bool is64Bits = isWXSumOutOfRange ? true : (insn.GetOperandSize(kInsnFirstOpnd) == k64BitSize);
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
  Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(newMop, resOpnd, srcOpnd, immOpnd1, immOpnd2);
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

bool LslAndToUbfizPattern::CheckCondition(Insn &insn) {
  MOperator mop = insn.GetMachineOpcode();
  RegOperand &opnd2 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  defInsn = ssaInfo->GetDefInsn(opnd2);
  InsnSet useInsns = GetAllUseInsn(opnd2);
  if (useInsns.size() != 1 || defInsn == nullptr) {
    return false;
  }
  MOperator defMop = defInsn->GetMachineOpcode();
  if ((mop == MOP_wandrri12 || mop == MOP_xandrri13) && (defMop == MOP_wlslrri5 || defMop == MOP_xlslrri6)) {
    return true;
  } else if ((defMop == MOP_wandrri12 || defMop == MOP_xandrri13) && (mop == MOP_wlslrri5 || mop == MOP_xlslrri6)) {
    /* lsl w1, w2, #n. insn and w1's useInsn can do prop, skipping this pattern */
    for (auto *useInsn : GetAllUseInsn(static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd)))) {
      if (useInsn == nullptr) {
        continue;
      }
      if (!CheckUseInsnMop(*useInsn)) {
        return false;
      }
    }
    return true;
  }
  return false;
}

bool LslAndToUbfizPattern::CheckUseInsnMop(const Insn &useInsn) const {
  if (useInsn.IsLoad() || useInsn.IsStore()) {
    return false;
  }
  MOperator useMop = useInsn.GetMachineOpcode();
  switch (useMop) {
    case MOP_xeorrrr:
    case MOP_xeorrrrs:
    case MOP_weorrrr:
    case MOP_weorrrrs:
    case MOP_xiorrrr:
    case MOP_xiorrrrs:
    case MOP_wiorrrr:
    case MOP_wiorrrrs:
    case MOP_xaddrrr:
    case MOP_xxwaddrrre:
    case MOP_xaddrrrs:
    case MOP_waddrrr:
    case MOP_wwwaddrrre:
    case MOP_waddrrrs:
    case MOP_waddrri12:
    case MOP_xaddrri12:
    case MOP_xsubrrr:
    case MOP_xxwsubrrre:
    case MOP_xsubrrrs:
    case MOP_wsubrrr:
    case MOP_wwwsubrrre:
    case MOP_wsubrrrs:
    case MOP_xinegrr:
    case MOP_winegrr:
    case MOP_xsxtb32:
    case MOP_xsxtb64:
    case MOP_xsxth32:
    case MOP_xsxth64:
    case MOP_xuxtb32:
    case MOP_xuxth32:
    case MOP_xsxtw64:
    case MOP_xubfxrri6i6:
    case MOP_xcmprr:
    case MOP_xwcmprre:
    case MOP_xcmprrs:
    case MOP_wcmprr:
    case MOP_wwcmprre:
    case MOP_wcmprrs:
      return false;
    default:
      break;
  }
  return true;
}

void LslAndToUbfizPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  MOperator mop = insn.GetMachineOpcode();
  Insn *newInsn = nullptr;
  if (mop == MOP_wandrri12 || mop == MOP_xandrri13) {
    newInsn = BuildNewInsn(insn, *defInsn, insn);
  }
  if (mop == MOP_wlslrri5 || mop == MOP_xlslrri6) {
    newInsn = BuildNewInsn(*defInsn, insn, insn);
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
    (void)prevs.emplace_back(defInsn);
    DumpAfterPattern(prevs, &insn, newInsn);
  }
}

// Build ubfiz insn or mov insn
Insn *LslAndToUbfizPattern::BuildNewInsn(const Insn &andInsn, const Insn &lslInsn, const Insn &useInsn) const {
  uint64 andImmValue = static_cast<uint64>(static_cast<ImmOperand&>(andInsn.GetOperand(kInsnThirdOpnd)).GetValue());
  uint64 lslImmValue = static_cast<uint64>(static_cast<ImmOperand&>(lslInsn.GetOperand(kInsnThirdOpnd)).GetValue());
  MOperator useMop = useInsn.GetMachineOpcode();
  // isLslAnd means true -> lsl + and, false -> and + lsl
  bool isLslAnd = (useMop == MOP_wandrri12) || (useMop == MOP_xandrri13);
  // judgment need to set non-zero value
  uint64 judgment = 1;
  // When useInsn is lsl, check whether the value of immValue is 2^n-1.
  // When useInsn is and, check whether the value of immValue is (2^n-1) << m
  if (isLslAnd) {
    if ((andImmValue >> lslImmValue) != 0) {
      judgment = (andImmValue >> lslImmValue) & ((andImmValue >> lslImmValue) + 1);
    }
  } else {
    judgment = andImmValue & (andImmValue + 1);
  }
  if (judgment != 0) {
    return nullptr;
  }
  RegOperand &ubfizOpnd1 = static_cast<RegOperand&>(useInsn.GetOperand(kInsnFirstOpnd));
  uint32 opnd1Size = ubfizOpnd1.GetSize();
  RegOperand &ubfizOpnd2 = static_cast<RegOperand&>(defInsn->GetOperand(kInsnSecondOpnd));
  uint32 opnd2Size = ubfizOpnd2.GetSize();
  ImmOperand &ubfizOpnd3 = static_cast<ImmOperand&>(lslInsn.GetOperand(kInsnThirdOpnd));
  uint32 mValue = static_cast<uint32>(ubfizOpnd3.GetValue());
  uint32 nValue = 0;
  if (isLslAnd) {
    nValue = static_cast<uint32>(__builtin_popcountll(andImmValue >> lslImmValue));
  } else {
    nValue = static_cast<uint32>(__builtin_popcountll(andImmValue));
  }
  auto *aarFunc = static_cast<AArch64CGFunc*>(cgFunc);
  if (opnd1Size != opnd2Size || (mValue + nValue) > opnd1Size) {
    return nullptr;
  }
  MOperator addMop = andInsn.GetMachineOpcode();
  MOperator newMop = (addMop == MOP_wandrri12) ? MOP_wubfizrri5i5 : MOP_xubfizrri6i6;
  uint32 size = (addMop == MOP_wandrri12) ? kMaxImmVal5Bits : kMaxImmVal6Bits;
  ImmOperand &ubfizOpnd4 = aarFunc->CreateImmOperand(nValue, size, false);
  Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(newMop, ubfizOpnd1, ubfizOpnd2, ubfizOpnd3, ubfizOpnd4);
  return &newInsn;
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
  Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(
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
  Insn *newInsn = nullptr;
  // if bit offset is invalid (implicit zero), then it should be a unconditional branch
  if (!aarchFunc->IsOperandImmValid(newMop, &tbzImm, kInsnSecondOpnd)) {
    // cfg adjustment in ssa is complicate, so we just bypass this pattern if imm is invalid.
    if (ssaInfo) {
      return;
    }
    bool delEdgeWithTarget = false;
    if (newMop == MOP_wtbz) {
      newInsn = &aarchFunc->GetInsnBuilder()->BuildInsn(MOP_xuncond, labelOpnd);
      bb.SetKind(BB::kBBGoto);
    } else if (newMop == MOP_wtbnz) {
      bb.SetKind(BB::kBBFallthru);
      bb.RemoveInsn(insn);
      delEdgeWithTarget = true;
    } else {
      CHECK_FATAL(false, "only wtbz/wtbnz can have invalid imm");
    }
    auto *bbFt = bb.GetNext();
    auto *targetBB = cgFunc->GetBBFromLab2BBMap(labelOpnd.GetLabelIndex());
    if (targetBB != bbFt) {   // when targetBB is ftBB, we cannot remove preds/succs
      auto *delEdgeBB = delEdgeWithTarget ? targetBB : bbFt;
      delEdgeBB->RemovePreds(bb);
      bb.RemoveSuccs(*delEdgeBB);
    }
  } else {
    newInsn = &cgFunc->GetInsnBuilder()->BuildInsn(newMop, prevInsn->GetOperand(kInsnSecondOpnd),
                                                   tbzImm, labelOpnd);
  }
  if (newInsn != nullptr) {
    bb.ReplaceInsn(insn, *newInsn);
    SetCurrInsn(newInsn);
  }
  if (ssaInfo) {
    /* update ssa info */
    ssaInfo->ReplaceInsn(insn, *newInsn);
  }
  optSuccess = true;
  /* dump pattern info */
  if (CG_PEEP_DUMP) {
    std::vector<Insn*> prevs;
    prevs.emplace_back(prevInsn);
    DumpAfterPattern(prevs, &insn, newInsn);
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
  // prop vary attr
  if (prevImmOpnd.GetVary() == kUnAdjustVary && curImmOpnd.GetVary() == kUnAdjustVary) {
    return false;
  }
  if (prevImmOpnd.GetVary() == kUnAdjustVary || curImmOpnd.GetVary() == kUnAdjustVary) {
    newImmOpnd->SetVary(kUnAdjustVary);
  }
  if (prevImmOpnd.GetVary() == kAdjustVary || curImmOpnd.GetVary() == kAdjustVary) {
    newImmOpnd->SetVary(kAdjustVary);
  }
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
  Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(insn.GetMachineOpcode(),
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
  Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(newMop, insn.GetOperand(kInsnFirstOpnd), opnd1, opnd2, immOpnd);
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
    newInsn = &(cgFunc->GetInsnBuilder()->BuildInsn(newMop, resOpnd, opndMulOpnd1, opndMulOpnd2));
  } else {
    Operand &opnd3 = (validOpndIdx == kInsnSecondOpnd) ? currInsn.GetOperand(kInsnThirdOpnd) :
        currInsn.GetOperand(kInsnSecondOpnd);
    newInsn = &(cgFunc->GetInsnBuilder()->BuildInsn(newMop, resOpnd, opndMulOpnd1, opndMulOpnd2, opnd3));
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
    case MOP_wandrri12: {
      is64Bits = false;
      extTypeIdx = AND;
      break;
    }
    case MOP_xandrri13: {
      is64Bits = true;
      extTypeIdx = AND;
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
    case MOP_wmovri32:
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
  Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(newMop, currDstOpnd, prevDstOpnd);
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
  if (&insn == currBB->GetFirstMachineInsn() || extTypeIdx == AND) {
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
  CHECK_FATAL(memOpnd != nullptr, "invalid memOpnd");

  ASSERT(!prevInsn->IsStorePair(), "do not do ElimSpecificExtensionPattern for str pair");
  ASSERT(!prevInsn->IsLoadPair(), "do not do ElimSpecificExtensionPattern for ldr pair");
  if (memOpnd->GetAddrMode() == MemOperand::kBOI &&
      !aarFunc->IsOperandImmValid(newMop, memOpnd, kInsnSecondOpnd)) {
    return false;
  }
  uint32 shiftAmount = memOpnd->ShiftAmount();
  if (shiftAmount == 0) {
    return true;
  }
  const InsnDesc *md = &AArch64CG::kMd[newMop];
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
  if (extTypeIdx == AND) {
    auto &immOpnd = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
    if (immOpnd.GetValue() != 0xff) {
      return;
    }
  }
  MOperator prevOrigMop = prevInsn->GetMachineOpcode();
  for (uint8 i = 0; i < kPrevLoadPatternNum; i++) {
    ASSERT(extTypeIdx < EXTTYPESIZE, "extTypeIdx must be lower than EXTTYPESIZE");
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
    auto *newMemOp = GetOrCreateMemOperandForNewMOP(*cgFunc, *prevInsn, prevNewMop);
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
    prevInsn->SetMOP(AArch64CG::kMd[prevNewMop]);

    if ((prevOrigMop != prevNewMop) && CG_PEEP_DUMP) {
      LogInfo::MapleLogger() << "======= NewPrevInsn : \n";
      prevInsn->Dump();
      aarCGSSAInfo->DumpInsnInSSAForm(*prevInsn);
    }

    MOperator movMop = is64Bits ? MOP_xmovrr : MOP_wmovrr;
    Insn &newMovInsn = cgFunc->GetInsnBuilder()->BuildInsn(movMop, insn.GetOperand(kInsnFirstOpnd),
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
  if (extTypeIdx == EXTUNDEF || extTypeIdx == AND) {
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
    ASSERT(extTypeIdx < EXTTYPESIZE, "extTypeIdx must be lower than EXTTYPESIZE");
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

void ElimSpecificExtensionPattern::Run(BB& /* bb */, Insn &insn) {
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
  if (&insn != bb.GetLastMachineInsn()) {
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
    Insn &newCbzInsn = cgFunc->GetInsnBuilder()->BuildInsn(
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
    Insn &newTbzInsn = cgFunc->GetInsnBuilder()->BuildInsn(newOp, regOperand, oneHoleOpnd, label);
    if (!VERIFY_INSN(&newTbzInsn)) {
      return;
    }
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
  Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(newMop, *reg1, *reg2);
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
    case MOP_wubfxrri5i5: {
      manager->NormalPatternOpt<UbfxAndCbzToTbzPattern>(!cgFunc->IsAfterRegAlloc());
      break;
    }
    case MOP_xubfxrri6i6: {
      manager->NormalPatternOpt<UbfxToUxtwPattern>(!cgFunc->IsAfterRegAlloc());
      if (!manager->OptSuccess()) {
        manager->NormalPatternOpt<UbfxAndCbzToTbzPattern>(!cgFunc->IsAfterRegAlloc());
      }
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
      if (!manager->OptSuccess()) {
        manager->NormalPatternOpt<ContiLDRorSTRToSameMEMPattern>(cgFunc->IsAfterRegAlloc());
      }
      if (!manager->OptSuccess()) {
        manager->NormalPatternOpt<RemoveIdenticalLoadAndStorePattern>(cgFunc->IsAfterRegAlloc());
      }
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
    case MOP_xandrrr:
    case MOP_wandrrr:
    case MOP_wandrri12:
    case MOP_xandrri13: {
      manager->NormalPatternOpt<AndCbzBranchesToTstPattern>(cgFunc->IsAfterRegAlloc());
      break;
    }
    case MOP_wcbz:
    case MOP_xcbz:
    case MOP_wcbnz:
    case MOP_xcbnz: {
      manager->NormalPatternOpt<AndCbzToTbzPattern>(!cgFunc->IsAfterRegAlloc());
      if (!manager->OptSuccess()) {
        manager->NormalPatternOpt<CbnzToCbzPattern>(cgFunc->IsAfterRegAlloc());
      }
      break;
    }
    case MOP_xsxtb32:
    case MOP_xsxth32:
    case MOP_xsxtb64:
    case MOP_xsxth64:
    case MOP_xsxtw64: {
      manager->NormalPatternOpt<EliminateSpecifcSXTPattern>(cgFunc->IsAfterRegAlloc());
      if (!manager->OptSuccess() && thisMop == MOP_xsxtw64) {
        manager->NormalPatternOpt<ComplexExtendWordLslPattern>(cgFunc->IsAfterRegAlloc());
      }
      break;
    }
    case MOP_xuxtb32:
    case MOP_xuxth32:
    case MOP_xuxtw64: {
      manager->NormalPatternOpt<EliminateSpecifcUXTPattern>(cgFunc->IsAfterRegAlloc());
      if (!manager->OptSuccess() && thisMop == MOP_xuxtw64) {
        manager->NormalPatternOpt<ComplexExtendWordLslPattern>(cgFunc->IsAfterRegAlloc());
      }
      break;
    }
    case MOP_wsdivrrr: {
      manager->NormalPatternOpt<ReplaceDivToMultiPattern>(cgFunc->IsAfterRegAlloc());
      break;
    }
    case MOP_xrevrr:
    case MOP_wrevrr:
    case MOP_wrevrr16: {
      manager->NormalPatternOpt<NormRevTbzToTbzPattern>(!cgFunc->IsAfterRegAlloc());
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
    case MOP_xaddrri12:
    case MOP_xsubrri12: {
      manager->NormalPatternOpt<AddSubMergeLdStPattern>(cgFunc->IsAfterRegAlloc());
      break;
    }
    case MOP_wcselrrrc:
    case MOP_xcselrrrc: {
      manager->NormalPatternOpt<CselToCsetPattern>(cgFunc->IsAfterRegAlloc());
      manager->NormalPatternOpt<CselToMovPattern>(!cgFunc->IsAfterRegAlloc());
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

void AArch64PeepHole0::InitOpts() {
  optimizations.resize(kPeepholeOptsNum);
  optimizations[kDeleteMovAfterCbzOrCbnzOpt] = optOwnMemPool->New<DeleteMovAfterCbzOrCbnzAArch64>(cgFunc);
}

void AArch64PeepHole0::Run(BB &bb, Insn &insn) {
  MOperator thisMop = insn.GetMachineOpcode();
  switch (thisMop) {
    case MOP_wcbz:
    case MOP_xcbz:
    case MOP_wcbnz:
    case MOP_xcbnz: {
      (static_cast<DeleteMovAfterCbzOrCbnzAArch64*>(optimizations[kDeleteMovAfterCbzOrCbnzOpt]))->Run(bb, insn);
      break;
    }
    default:
      break;
  }
}

void AArch64PrePeepHole::InitOpts() {
  optimizations.resize(kPeepholeOptsNum);
  optimizations[kComplexMemOperandOpt] = optOwnMemPool->New<ComplexMemOperandAArch64>(cgFunc);
  optimizations[kComplexMemOperandPreOptAdd] = optOwnMemPool->New<ComplexMemOperandPreAddAArch64>(cgFunc);
  optimizations[kEnhanceStrLdrAArch64Opt] = optOwnMemPool->New<EnhanceStrLdrAArch64>(cgFunc);
}

void AArch64PrePeepHole::Run(BB &bb, Insn &insn) {
  MOperator thisMop = insn.GetMachineOpcode();
  switch (thisMop) {
    case MOP_xadrpl12: {
      (static_cast<ComplexMemOperandAArch64*>(optimizations[kComplexMemOperandOpt]))->Run(bb, insn);
      break;
    }
    case MOP_xaddrrr: {
      (static_cast<ComplexMemOperandPreAddAArch64*>(optimizations[kComplexMemOperandPreOptAdd]))->Run(bb, insn);
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
}

void AArch64PrePeepHole1::InitOpts() {
  optimizations.resize(kPeepholeOptsNum);
  optimizations[kAddCmpZeroOpt] = optOwnMemPool->New<AddCmpZeroAArch64>(cgFunc);
  optimizations[kCombineMovInsnBeforeCSelOpt] = optOwnMemPool->New<CombineMovInsnBeforeCSelAArch64>(cgFunc);
}

void AArch64PrePeepHole1::Run(BB &bb, Insn &insn) {
  MOperator thisMop = insn.GetMachineOpcode();
  switch (thisMop) {
    case MOP_wcmpri:
    case MOP_xcmpri: {
      (static_cast<AddCmpZeroAArch64*>(optimizations[kAddCmpZeroOpt]))->Run(bb, insn);
      break;
    }
    case MOP_wcselrrrc:
    case MOP_xcselrrrc: {
      (static_cast<CombineMovInsnBeforeCSelAArch64*>(optimizations[kCombineMovInsnBeforeCSelOpt]))->Run(bb, insn);
      break;
    }
    default:
      break;
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
    uint32 srcOpndSize1 = insn.GetOperand(kInsnFirstOpnd).GetSize();
    uint32 srcOpndSize2 = nextInsn->GetOperand(kInsnFirstOpnd).GetSize();
    if (srcOpndSize1 == srcOpndSize2 && IsMemOperandsIdentical(insn, *nextInsn)) {
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
  if (memOpnd1.GetAddrMode() != MemOperand::kBOI) {
    return false;
  }
  auto &memOpnd2 = static_cast<MemOperand&>(insn2.GetOperand(kInsnSecondOpnd));
  if (memOpnd2.GetAddrMode() != MemOperand::kBOI) {
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

bool MulImmToShiftPattern::CheckCondition(Insn &insn) {
  auto &useReg = static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd));
  movInsn = ssaInfo->GetDefInsn(useReg);
  if (movInsn == nullptr) {
    return false;
  }
  MOperator prevMop = movInsn->GetMachineOpcode();
  if (prevMop != MOP_wmovri32 && prevMop != MOP_xmovri64) {
    return false;
  }
  ImmOperand &immOpnd = static_cast<ImmOperand&>(movInsn->GetOperand(kInsnSecondOpnd));
  if (immOpnd.IsNegative()) {
    return false;
  }
  uint64 immVal = static_cast<uint64>(immOpnd.GetValue());
  if (immVal == 0) {
    shiftVal = 0;
    newMop = insn.GetMachineOpcode() == MOP_xmulrrr ? MOP_xmovri64 : MOP_wmovri32;
    return true;
  }
  /* power of 2 */
  if ((immVal & (immVal - 1)) != 0) {
    return false;
  }
  shiftVal = static_cast<uint32>(log2(immVal));
  newMop = (prevMop == MOP_xmovri64) ? MOP_xlslrri6 : MOP_wlslrri5;
  return true;
}

void MulImmToShiftPattern::Run(BB &bb, Insn &insn) {
  /* mov x0,imm and mul to shift */
  if (!CheckCondition(insn)) {
    return;
  }
  auto *aarch64CGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  ImmOperand &immOpnd = aarch64CGFunc->CreateImmOperand(shiftVal, k32BitSize, false);
  Insn *newInsn;
  if (newMop == MOP_xmovri64 || newMop == MOP_wmovri32) {
    newInsn = &cgFunc->GetInsnBuilder()->BuildInsn(newMop, insn.GetOperand(kInsnFirstOpnd), immOpnd);
  } else {
    newInsn = &cgFunc->GetInsnBuilder()->BuildInsn(newMop, insn.GetOperand(kInsnFirstOpnd),
                                                   insn.GetOperand(kInsnSecondOpnd), immOpnd);
  }
  bb.ReplaceInsn(insn, *newInsn);
  /* update ssa info */
  ssaInfo->ReplaceInsn(insn, *newInsn);
  optSuccess = true;
  SetCurrInsn(newInsn);
  if (CG_PEEP_DUMP) {
    std::vector<Insn*> prevs;
    prevs.emplace_back(movInsn);
    DumpAfterPattern(prevs, &insn, newInsn);
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
  if (IsEnhanceAddImm(prevMop) && a64MemOpnd.GetAddrMode() == MemOperand::kBOI &&
      a64MemOpnd.GetOffsetImmediate()->GetValue() == 0) {
    auto &addDestOpnd = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
    if (baseOpnd == &addDestOpnd && !IfOperandIsLiveAfterInsn(addDestOpnd, insn)) {
      auto &concreteMemOpnd = static_cast<MemOperand&>(memOpnd);
      auto *origBaseReg = concreteMemOpnd.GetBaseRegister();
      concreteMemOpnd.SetBaseRegister(
          static_cast<RegOperand&>(prevInsn->GetOperand(kInsnSecondOpnd)));
      auto &ofstOpnd = static_cast<ImmOperand&>(prevInsn->GetOperand(kInsnThirdOpnd));
      OfstOperand &offOpnd = static_cast<AArch64CGFunc&>(cgFunc).CreateOfstOpnd(
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

bool CombineContiLoadAndStorePattern::IsRegNotSameMemUseInInsn(const Insn &checkInsn, const Insn &curInsn,
                                                               regno_t curBaseRegNO, bool isCurStore,
                                                               int64 curBaseOfst, int64 curBaseMemRange) const {
  uint32 opndNum = checkInsn.GetOperandSize();
  for (uint32 i = 0; i < opndNum; ++i) {
    Operand &opnd = checkInsn.GetOperand(i);
    if (opnd.IsList()) {
      auto &listOpnd = static_cast<const ListOperand&>(opnd);
      for (auto &listElem : listOpnd.GetOperands()) {
        auto *regOpnd = static_cast<RegOperand*>(listElem);
        ASSERT(regOpnd != nullptr, "parameter operand must be RegOperand");
        if (curBaseRegNO == regOpnd->GetRegisterNumber()) {
          return true;
        }
      }
    } else if (opnd.IsMemoryAccessOperand()) {
      auto &memOperand = static_cast<MemOperand&>(opnd);
      RegOperand *checkBaseReg = memOperand.GetBaseRegister();
      // If the BASEREG of the two MEM insns are different, we use cg-mem-reference to check the alias of two MEM:
      // if there is no alias, we can combine MEM pair cross the MEM insn.
      // e.g.
      // str x1, [x9]
      // str x6, [x2]
      // str x3, [x9, #8]
      regno_t stackBaseRegNO = cgFunc->UseFP() ? R29 : RSP;
      if ((isCurStore || checkInsn.IsStore()) && checkBaseReg != nullptr && !(curBaseRegNO == stackBaseRegNO &&
          checkBaseReg->GetRegisterNumber() == stackBaseRegNO) && checkBaseReg->GetRegisterNumber() != curBaseRegNO &&
          AArch64MemReference::HasAliasMemoryDep(checkInsn, curInsn, kDependenceTypeNone)) {
        return true;
      }
      // Check memory overlap
      if ((isCurStore || checkInsn.IsStore()) && checkBaseReg != nullptr &&
          memOperand.GetAddrMode() == MemOperand::kBOI && memOperand.GetOffsetImmediate() != nullptr) {
        // If memInsn is split with x16, we need to find the actual base register
        int64 checkOffset = memOperand.GetOffsetImmediate()->GetOffsetValue();
        regno_t checkRegNO = checkBaseReg->GetRegisterNumber();
        if (checkRegNO == R16) {
          const Insn *prevInsn = checkInsn.GetPrev();
          // Before cgaggressiveopt, the def and use of R16 must be adjacent, and the def of R16 must be addrri,
          // otherwise, the process is conservative and the mem insn that can be combined is not search forward.
          if (prevInsn == nullptr || prevInsn->GetMachineOpcode() != MOP_xaddrri12 ||
              static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd)).GetRegisterNumber() != R16) {
            return true;
          }
          checkOffset += static_cast<ImmOperand&>(prevInsn->GetOperand(kInsnThirdOpnd)).GetValue();
        }
        auto curMemRange = static_cast<int64>(checkInsn.GetMemoryByteSize());
        //      curOfst          curOfst+curMemRange
        // |______|_/_/_/_/_/_/_/_/_/_/_|____________|
        if ((curBaseOfst >= checkOffset && curBaseOfst < (checkOffset + curMemRange)) ||
            (checkOffset >= curBaseOfst && checkOffset < (curBaseOfst + curBaseMemRange))) {
          return true;
        }
      }
    } else if (opnd.IsConditionCode()) {
      auto &rflagOpnd = static_cast<RegOperand&>(cgFunc->GetOrCreateRflag());
      if (rflagOpnd.GetRegisterNumber() == curBaseRegNO) {
        return true;
      }
    } else if (opnd.IsRegister()) {
      if (!isCurStore && static_cast<RegOperand&>(opnd).GetRegisterNumber() == curBaseRegNO) {
        return true;
      }
    }
  }
  return false;
}

std::vector<Insn*> CombineContiLoadAndStorePattern::FindPrevStrLdr(Insn &insn, regno_t destRegNO,
                                                                   regno_t memBaseRegNO, int64 baseOfst) const {
  std::vector<Insn*> prevContiInsns;
  for (Insn *curInsn = insn.GetPrev(); curInsn != nullptr; curInsn = curInsn->GetPrev()) {
    if (!curInsn->IsMachineInstruction()) {
      continue;
    }
    if (curInsn->IsRegDefined(memBaseRegNO)) {
      return prevContiInsns;
    }
    ASSERT(insn.GetOperand(kInsnSecondOpnd).IsMemoryAccessOperand(), "invalid mem insn");
    auto baseMemRange = static_cast<int64>(insn.GetMemoryByteSize());
    if (IsRegNotSameMemUseInInsn(*curInsn, insn, memBaseRegNO, insn.IsStore(), static_cast<int32>(baseOfst),
                                 baseMemRange)) {
      return prevContiInsns;
    }
    // record continuous STD/LDR insn
    if (!curInsn->IsLoadStorePair() && ((insn.IsStore() && curInsn->IsStore()) ||
                                        (insn.IsLoad() && curInsn->IsLoad()))) {
      auto *memOperand = static_cast<MemOperand*>(curInsn->GetMemOpnd());
      // do not combine ldr r0, label
      if (memOperand != nullptr) {
        auto *baseRegOpnd = static_cast<RegOperand*>(memOperand->GetBaseRegister());
        ASSERT(baseRegOpnd == nullptr || !baseRegOpnd->IsVirtualRegister(), "physical reg has not been allocated?");
        if ((memOperand->GetAddrMode() == MemOperand::kBOI) && baseRegOpnd->GetRegisterNumber() == memBaseRegNO) {
          prevContiInsns.emplace_back(curInsn);
        }
      }
    }
    // ldr x8, [x21, #8]
    // call foo()
    // ldr x9, [x21, #16]
    // although x21 is a calleeSave register, there is no guarantee data in memory [x21] is not changed
    if (curInsn->IsCall() || curInsn->GetMachineOpcode() == MOP_asm || curInsn->ScanReg(destRegNO)) {
      return prevContiInsns;
    }
  }
  return prevContiInsns;
}

bool CombineContiLoadAndStorePattern::CheckCondition(Insn &insn) {
  MOperator mop = insn.GetMachineOpcode();
  if (mop == MOP_wldrb || mop == MOP_wldrh) {
    return false;
  }
  auto *curMemOpnd = static_cast<MemOperand*>(insn.GetMemOpnd());
  ASSERT(curMemOpnd != nullptr, "get mem operand failed");
  if (!doAggressiveCombine || curMemOpnd->GetAddrMode() != MemOperand::kBOI) {
    return false;
  }
  return true;
}

// Combining 2 STRs into 1 stp or 2 LDRs into 1 ldp
void CombineContiLoadAndStorePattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }

  auto *curMemOpnd = static_cast<MemOperand*>(insn.GetMemOpnd());
  ASSERT(curMemOpnd->GetAddrMode() == MemOperand::kBOI, "invalid continues mem insn");
  OfstOperand *curOfstOpnd = curMemOpnd->GetOffsetImmediate();
  int64 curOfstVal = curOfstOpnd ? curOfstOpnd->GetOffsetValue() : 0;

  auto *baseRegOpnd = static_cast<RegOperand*>(curMemOpnd->GetBaseRegister());
  ASSERT(baseRegOpnd == nullptr || !baseRegOpnd->IsVirtualRegister(), "physical register has not been allocated?");
  auto &curDestOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));

  std::vector<Insn*> prevContiInsnVec = FindPrevStrLdr(insn, curDestOpnd.GetRegisterNumber(),
                                                       baseRegOpnd->GetRegisterNumber(), curOfstVal);

  for (auto prevContiInsn : prevContiInsnVec) {
    ASSERT(prevContiInsn != nullptr, "get previous consecutive instructions failed");
    auto *prevMemOpnd = static_cast<MemOperand*>(prevContiInsn->GetMemOpnd());
    ASSERT(prevMemOpnd->GetAddrMode() == MemOperand::kBOI, "invalid continues mem insn");
    OfstOperand *prevOfstOpnd = prevMemOpnd->GetOffsetImmediate();
    int64 prevOfstVal = prevOfstOpnd ? prevOfstOpnd->GetOffsetValue() : 0;
    auto &prevDestOpnd = static_cast<RegOperand&>(prevContiInsn->GetOperand(kInsnFirstOpnd));
    if (prevDestOpnd.GetRegisterType() != curDestOpnd.GetRegisterType()) {
      continue;
    }

    MemOperand *combineMemOpnd = (curOfstVal < prevOfstVal) ? curMemOpnd : prevMemOpnd;
    if (IsValidNormalLoadOrStorePattern(insn, *prevContiInsn, *curMemOpnd, curOfstVal, prevOfstVal)) {
      // Process normal mem pair
      MOperator newMop = GetMopPair(insn.GetMachineOpcode(), true);
      Insn *combineInsn = GenerateMemPairInsn(newMop, curDestOpnd, prevDestOpnd, *combineMemOpnd,
                                              curOfstVal < prevOfstVal);
      ASSERT(combineInsn != nullptr, "create combineInsn failed");
      bb.InsertInsnAfter(*prevContiInsn, *combineInsn);
      if (!(static_cast<AArch64CGFunc&>(*cgFunc).IsOperandImmValid(newMop, combineMemOpnd,
          isPairAfterCombine ? kInsnThirdOpnd : kInsnSecondOpnd))) {
        if (FindUseX16AfterInsn(*prevContiInsn)) {
          // Do not combine Insns when x16 was used after curInsn
          bb.RemoveInsn(*combineInsn);
          return;
        }
        SPLIT_INSN(combineInsn, cgFunc);
      }
      RemoveInsnAndKeepComment(bb, insn, *prevContiInsn);
      SetCurrInsn(combineInsn);
      optSuccess = true;
      return;
    } else if (IsValidStackArgLoadOrStorePattern(insn, *prevContiInsn, *curMemOpnd, *prevMemOpnd,
                                                 curOfstVal, prevOfstVal)) {
      // Process stack-arg mem pair
      regno_t curDestRegNo = curDestOpnd.GetRegisterNumber();
      regno_t prevDestRegNo = prevDestOpnd.GetRegisterNumber();
      RegOperand &newDest = static_cast<AArch64CGFunc*>(cgFunc)->GetOrCreatePhysicalRegisterOperand(
          static_cast<AArch64reg>(curDestRegNo), k64BitSize, curDestOpnd.GetRegisterType());
      RegOperand &newPrevDest = static_cast<AArch64CGFunc*>(cgFunc)->GetOrCreatePhysicalRegisterOperand(
          static_cast<AArch64reg>(prevDestRegNo), k64BitSize, prevDestOpnd.GetRegisterType());
      MOperator newMop = (curDestOpnd.GetRegisterType() == kRegTyInt) ? MOP_xstp : MOP_dstp;
      if (!(static_cast<AArch64CGFunc&>(*cgFunc).IsOperandImmValid(newMop, combineMemOpnd, kInsnThirdOpnd))) {
        return;
      }
      Insn *combineInsn = GenerateMemPairInsn(newMop, newDest, newPrevDest, *combineMemOpnd, curOfstVal < prevOfstVal);
      bb.InsertInsnAfter(*prevContiInsn, *combineInsn);
      RemoveInsnAndKeepComment(bb, insn, *prevContiInsn);
      SetCurrInsn(combineInsn);
      optSuccess = true;
      return;
    }
  }
}

bool CombineContiLoadAndStorePattern::FindUseX16AfterInsn(const Insn &curInsn) const {
  for (Insn *cursor = curInsn.GetNext(); cursor != nullptr; cursor = cursor->GetNext()) {
    if (!cursor->IsMachineInstruction()) {
      continue;
    }
    for (uint32 defRegNo : cursor->GetDefRegs()) {
      if (defRegNo == R16) {
        return false;
      }
    }
    if ((!cursor->IsLoad() && !cursor->IsStore() && !cursor->IsLoadStorePair()) || cursor->IsAtomic()) {
      continue;
    }
    const InsnDesc *md = &AArch64CG::kMd[cursor->GetMachineOpcode()];
    if (cursor->IsLoadLabel() || md->IsLoadAddress()) {
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

Insn *CombineContiLoadAndStorePattern::GenerateMemPairInsn(MOperator newMop, RegOperand &curDestOpnd,
                                                           RegOperand &prevDestOpnd, MemOperand &combineMemOpnd,
                                                           bool isCurDestFirst) {
  ASSERT(newMop != MOP_undef, "invalid MOperator");
  Insn *combineInsn = nullptr;
  if (isPairAfterCombine) { // for ldr/str --> ldp/stp
    combineInsn = (isCurDestFirst) ? &cgFunc->GetInsnBuilder()->BuildInsn(newMop, curDestOpnd,
                                                                          prevDestOpnd, combineMemOpnd) :
                                     &cgFunc->GetInsnBuilder()->BuildInsn(newMop, prevDestOpnd,
                                                                          curDestOpnd, combineMemOpnd);
  } else { // for strb/strh --> strh/str, curDestOpnd == preDestOpnd
    combineInsn = &cgFunc->GetInsnBuilder()->BuildInsn(newMop, curDestOpnd, combineMemOpnd);
    combineMemOpnd.SetSize(newMop == MOP_wstrh ? maplebe::k16BitSize : maplebe::k32BitSize);
  }
  return combineInsn;
}

bool CombineContiLoadAndStorePattern::IsValidNormalLoadOrStorePattern(const Insn &insn, const Insn &prevInsn,
                                                                      const MemOperand &memOpnd,
                                                                      int64 curOfstVal, int64 prevOfstVal) {
  if (memOpnd.IsStackArgMem()) {
    return false;
  }
  ASSERT(insn.GetOperand(kInsnFirstOpnd).IsRegister(), "unexpect operand");
  ASSERT(prevInsn.GetOperand(kInsnFirstOpnd).IsRegister(), "unexpect operand");
  auto &curDestOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  auto &prevDestOpnd = static_cast<RegOperand&>(prevInsn.GetOperand(kInsnFirstOpnd));
  if (prevDestOpnd.GetRegisterType() != curDestOpnd.GetRegisterType() ||
      curDestOpnd.GetSize() != prevDestOpnd.GetSize()) {
    return false;
  }
  uint32 memSize = insn.GetMemoryByteSize();
  uint32 prevMemSize = prevInsn.GetMemoryByteSize();
  if (memSize != prevMemSize) {
    return false;
  }

  int64 diffVal = std::abs(curOfstVal - prevOfstVal);
  if ((memSize == k1ByteSize && diffVal == k1BitSize) || (memSize == k2ByteSize && diffVal == k2BitSize) ||
      (memSize == k4ByteSize && diffVal == k4BitSize) || (memSize == k8ByteSize && diffVal == k8BitSize)) {
    MOperator curMop = insn.GetMachineOpcode();
    ASSERT(curMop != MOP_wldrb && curMop != MOP_wldrh, "invalid mem insn that cannot be combined");
    if (curMop == MOP_wstrb || curMop == MOP_wstrh) {
      isPairAfterCombine = false;
    }

    regno_t destRegNO = curDestOpnd.GetRegisterNumber();
    regno_t prevDestRegNO = prevDestOpnd.GetRegisterNumber();
    if (destRegNO == RZR && prevDestRegNO == RZR) {
      return true;
    }

    if (insn.IsLoad() && destRegNO == prevDestRegNO) {
      return false;
    }

    if ((curMop == MOP_wstrb || curMop == MOP_wstrh) && (destRegNO != RZR || prevDestRegNO != RZR)) {
      return false;
    }

    return true;
  }

  return false;
}

bool CombineContiLoadAndStorePattern::IsValidStackArgLoadOrStorePattern(const Insn &curInsn, const Insn &prevInsn,
                                                                        const MemOperand &curMemOpnd,
                                                                        const MemOperand &prevMemOpnd,
                                                                        int64 curOfstVal, int64 prevOfstVal) const {
  if (!curInsn.IsStore()) {
    return false;
  }
  if (!curMemOpnd.IsStackArgMem() || !prevMemOpnd.IsStackArgMem()) {
    return false;
  }
  auto &curDestOpnd = static_cast<RegOperand&>(curInsn.GetOperand(kInsnFirstOpnd));
  auto &prevDestOpnd = static_cast<RegOperand&>(prevInsn.GetOperand(kInsnFirstOpnd));
  uint32 memSize = curInsn.GetMemoryByteSize();
  uint32 prevMemSize = prevInsn.GetMemoryByteSize();
  auto diffVal = std::abs(curOfstVal - prevOfstVal);
  if ((memSize == k4ByteSize || memSize == k8ByteSize) && (prevMemSize == k4ByteSize || prevMemSize == k8ByteSize) &&
      (diffVal == k8BitSize) && (curDestOpnd.GetValidBitsNum() == memSize * k8BitSize) &&
      (prevDestOpnd.GetValidBitsNum() == prevMemSize * k8BitSize)) {
    return true;
  }
  return false;
}

void CombineContiLoadAndStorePattern::RemoveInsnAndKeepComment(BB &bb, Insn &insn, Insn &prevInsn) const {
  // keep the comment
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

bool EliminateSpecifcSXTPattern::CheckCondition(Insn &insn) {
  BB *bb = insn.GetBB();
  if (bb->GetFirstMachineInsn() == &insn) {
    BB *prevBB = bb->GetPrev();
    if (prevBB != nullptr && (bb->GetPreds().size() == 1) && (*(bb->GetPreds().cbegin()) == prevBB)) {
      prevInsn = prevBB->GetLastMachineInsn();
    }
  } else {
    prevInsn = insn.GetPreviousMachineInsn();
  }
  if (prevInsn == nullptr) {
    return false;
  }
  return true;
}

void EliminateSpecifcSXTPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  MOperator thisMop = insn.GetMachineOpcode();
  auto &regOpnd0 = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  auto &regOpnd1 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  if (regOpnd0.GetRegisterNumber() == regOpnd1.GetRegisterNumber() && prevInsn->IsMachineInstruction()) {
    if (prevInsn->GetMachineOpcode() == MOP_wmovri32 || prevInsn->GetMachineOpcode() == MOP_xmovri64) {
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
            optSuccess = true;
            return;
          }
        } else if (thisMop == MOP_xsxth32) {
          /* value should in range between -32678 and 32678 */
          if (value >= static_cast<int64>(0xFFFFFFFFFFFF8000) && value <= 0x7FFF &&
              immOpnd.IsSingleInstructionMovable(regOpnd0.GetSize())) {
            bb.RemoveInsn(insn);
            optSuccess = true;
            return;
          }
        } else {
          uint64 flag = 0xFFFFFFFFFFFFFF80; /* initialize the flag with fifty-nine 1s at top */
          if (thisMop == MOP_xsxth64) {
            flag = 0xFFFFFFFFFFFF8000;      /* specify the flag with forty-nine 1s at top in this case */
          } else if (thisMop == MOP_xsxtw64) {
            flag = 0xFFFFFFFF80000000;      /* specify the flag with thirty-three 1s at top in this case */
          }
          if ((static_cast<uint64>(value) & flag) == 0 && immOpnd.IsSingleInstructionMovable(regOpnd0.GetSize())) {
            auto *aarch64CGFunc = static_cast<AArch64CGFunc*>(cgFunc);
            RegOperand &dstOpnd = aarch64CGFunc->GetOrCreatePhysicalRegisterOperand(
                static_cast<AArch64reg>(dstMovOpnd.GetRegisterNumber()), k64BitSize, dstMovOpnd.GetRegisterType());
            prevInsn->SetOperand(kInsnFirstOpnd, dstOpnd);
            prevInsn->SetMOP(AArch64CG::kMd[MOP_xmovri64]);
            bb.RemoveInsn(insn);
            optSuccess = true;
            return;
          }
        }
      }
    }
  }
}

bool EliminateSpecifcUXTPattern::CheckCondition(Insn &insn) {
  BB *bb = insn.GetBB();
  if (bb->GetFirstMachineInsn() == &insn) {
    BB *prevBB = bb->GetPrev();
    if (prevBB != nullptr && (bb->GetPreds().size() == 1) && (*(bb->GetPreds().cbegin()) == prevBB)) {
      prevInsn = prevBB->GetLastMachineInsn();
    }
  } else {
    prevInsn = insn.GetPreviousMachineInsn();
  }
  if (prevInsn == nullptr) {
    return false;
  }
  return true;
}
void EliminateSpecifcUXTPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  MOperator thisMop = insn.GetMachineOpcode();
  auto &regOpnd0 = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  auto &regOpnd1 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  if (&insn == bb.GetFirstMachineInsn() || regOpnd0.GetRegisterNumber() != regOpnd1.GetRegisterNumber() ||
      !prevInsn->IsMachineInstruction()) {
    return;
  }
  if (cgFunc->GetMirModule().GetSrcLang() == kSrcLangC && prevInsn->IsCall() && prevInsn->GetIsCallReturnSigned()) {
    return;
  }
  if (thisMop == MOP_xuxtb32) {
    if (prevInsn->GetMachineOpcode() == MOP_wmovri32 || prevInsn->GetMachineOpcode() == MOP_xmovri64) {
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
          optSuccess = true;
          return;
        }
      }
    } else if (prevInsn->GetMachineOpcode() == MOP_wldrb) {
      auto &dstOpnd = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
      if (dstOpnd.GetRegisterNumber() != regOpnd1.GetRegisterNumber()) {
        return;
      }
      bb.RemoveInsn(insn);
      optSuccess = true;
      return;
    }
  } else if (thisMop == MOP_xuxth32) {
    if (prevInsn->GetMachineOpcode() == MOP_wmovri32 || prevInsn->GetMachineOpcode() == MOP_xmovri64) {
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
          optSuccess = true;
          return;
        }
      }
    } else if (prevInsn->GetMachineOpcode() == MOP_wldrh) {
      auto &dstOpnd = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
      if (dstOpnd.GetRegisterNumber() != regOpnd1.GetRegisterNumber()) {
        return;
      }
      bb.RemoveInsn(insn);
      optSuccess = true;
      return;
    }
  } else {
    /* this_mop == MOP_xuxtw64 */
    if (prevInsn->GetMachineOpcode() == MOP_wmovri32 || prevInsn->GetMachineOpcode() == MOP_wldrsb ||
        prevInsn->GetMachineOpcode() == MOP_wldrb || prevInsn->GetMachineOpcode() == MOP_wldrsh ||
        prevInsn->GetMachineOpcode() == MOP_wldrh || prevInsn->GetMachineOpcode() == MOP_wldr) {
      auto &dstOpnd = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
      if (!IsSameRegisterOperation(dstOpnd, regOpnd1, regOpnd0)) {
        return;
      }
      /* 32-bit ldr does zero-extension by default, so this conversion can be skipped */
      bb.RemoveInsn(insn);
      optSuccess = true;
      return;
    }
  }
}

bool FmovRegPattern::CheckCondition(Insn &insn) {
  nextInsn = insn.GetNextMachineInsn();
  if (nextInsn == nullptr) {
    return false;
  }
  if (&insn == insn.GetBB()->GetFirstMachineInsn()) {
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
  /* optimize case 1 */
  auto &prevDstRegOpnd = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
  regno_t prevDstReg = prevDstRegOpnd.GetRegisterNumber();
  auto *aarch64CGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  RegOperand &dst =
      aarch64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(curDstReg), doOpt, kRegTyInt);
  RegOperand &src =
      aarch64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(prevDstReg), doOpt, kRegTyInt);
  Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(newMop, dst, src);
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
      auto *regProp = nextInsn->GetDesc()->opndMD[opndIdx];
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
  const InsnDesc *md = nextInsn->GetDesc();
  for (uint32 opndIdx = 0; opndIdx < opndNum; ++opndIdx) {
    Operand &opnd = nextInsn->GetOperand(opndIdx);
    /* Check if it is a source operand. */
    if (opnd.IsMemoryAccessOperand() || opnd.IsList()) {
      return false;
    } else if (opnd.IsRegister()) {
      auto &reg = static_cast<RegOperand&>(opnd);
      auto *regProp = md->opndMD[opndIdx];
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
  if (movInsnMop != MOP_wmovri32 && movInsnMop != MOP_xmovri64) {
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
    insn.SetMOP(AArch64CG::kMd[MOP_wcbz]);
  } else {
    insn.SetMOP(AArch64CG::kMd[MOP_xcbz]);
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

bool ContiLDRorSTRToSameMEMPattern::CheckCondition(Insn &insn) {
  prevInsn = insn.GetPrev();
  while (prevInsn != nullptr && (prevInsn->GetMachineOpcode() == 0 || !prevInsn->IsMachineInstruction()) &&
         prevInsn != insn.GetBB()->GetFirstMachineInsn()) {
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
  if (addrMode1 != MemOperand::kBOI) {
    return;
  }

  auto *base1 = static_cast<RegOperand*>(memOpnd1.GetBaseRegister());
  ASSERT(base1 == nullptr || !base1->IsVirtualRegister(), "physical register has not been allocated?");
  OfstOperand *offset1 = memOpnd1.GetOffsetImmediate();

  auto &memOpnd2 = static_cast<MemOperand&>(prevInsn->GetOperand(kInsnSecondOpnd));
  MemOperand::AArch64AddressingMode addrMode2 = memOpnd2.GetAddrMode();
  if (addrMode2 != MemOperand::kBOI) {
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
    while (nextInsn != nullptr && nextInsn->GetMachineOpcode() == 0 && nextInsn != bb.GetLastMachineInsn()) {
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
      (void)bb.InsertInsnAfter(*prevInsn, cgFunc->GetInsnBuilder()->BuildInsn(newOp, reg1, reg2));
    }
    SetCurrInsn(insn.GetNextMachineInsn());
    optSuccess = true;
    bb.RemoveInsn(insn);
  } else if (reg1.GetRegisterNumber() == reg2.GetRegisterNumber() &&
             base1->GetRegisterNumber() != reg2.GetRegisterNumber()) {
    SetCurrInsn(insn.GetNextMachineInsn());
    optSuccess = true;
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

bool InlineReadBarriersPattern::CheckCondition(Insn& /* insn */) {
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
    Insn &loadInsn = cgFunc->GetInsnBuilder()->BuildInsn(loadOp, regOp, addr);
    bb.ReplaceInsn(insn, loadInsn);
  }
  bool isTailCall = (insn.GetMachineOpcode() == MOP_tail_call_opt_xbl);
  if (isTailCall) {
    /* add 'ret' instruction for tail call optimized load barrier. */
    Insn &retInsn = cgFunc->GetInsnBuilder()->BuildInsn<AArch64CG>(MOP_xret);
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
  if ((prevMop > 0) && (prevMop == MOP_wmovkri16) && (prePrevMop > 0) && (prePrevMop == MOP_wmovri32)) {
    return true;
  }
  return false;
}

void ReplaceDivToMultiPattern::Run(BB &bb, Insn &insn) {
  if (CheckCondition(insn)) {
    auto &sdivOpnd1 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
    auto &sdivOpnd2 = static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd));
    /* Check if dest operand of insn is idential with  register of prevInsn and prePrevInsn. */
    auto &prevReg = prevInsn->GetOperand(kInsnFirstOpnd);
    auto &prePrevReg = prePrevInsn->GetOperand(kInsnFirstOpnd);
    if (!prevReg.IsRegister() ||
        !prePrevReg.IsRegister() ||
        static_cast<RegOperand&>(prevReg).GetRegisterNumber() != sdivOpnd2.GetRegisterNumber() ||
        static_cast<RegOperand&>(prePrevReg).GetRegisterNumber() != sdivOpnd2.GetRegisterNumber()) {
      return;
    }
    auto &prevLsl = static_cast<BitShiftOperand&>(prevInsn->GetOperand(kInsnThirdOpnd));
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
    /* mov   w16, #0x588f */
    RegOperand &tempOpnd = aarch64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(R16),
                                                                             k64BitSize, kRegTyInt);
    /* create a immedate operand with this specific value */
    ImmOperand &multiplierLow = aarch64CGFunc->CreateImmOperand(0x588f, k32BitSize, false);
    Insn &multiplierLowInsn = cgFunc->GetInsnBuilder()->BuildInsn(MOP_wmovri32, tempOpnd, multiplierLow);
    bb.InsertInsnBefore(*prePrevInsn, multiplierLowInsn);

    /*
     * movk    w16, #0x4f8b, LSL #16
     * create a immedate operand with this specific value
     */
    ImmOperand &multiplierHigh = aarch64CGFunc->CreateImmOperand(0x4f8b, k32BitSize, false);
    BitShiftOperand *multiplierHighLsl = aarch64CGFunc->GetLogicalShiftLeftOperand(k16BitSize, true);
    Insn &multiplierHighInsn =
        cgFunc->GetInsnBuilder()->BuildInsn(MOP_wmovkri16, tempOpnd, multiplierHigh, *multiplierHighLsl);
    bb.InsertInsnBefore(*prePrevInsn, multiplierHighInsn);

    /* smull   x16, w0, w16 */
    Insn &newSmullInsn =
        cgFunc->GetInsnBuilder()->BuildInsn(MOP_xsmullrrr, tempOpnd, sdivOpnd1, tempOpnd);
    bb.InsertInsnBefore(*prePrevInsn, newSmullInsn);

    /* asr     x16, x16, #32 */
    ImmOperand &dstLsrImmHigh = aarch64CGFunc->CreateImmOperand(k32BitSize, k32BitSize, false);
    Insn &dstLsrInsnHigh =
        cgFunc->GetInsnBuilder()->BuildInsn(MOP_xasrrri6, tempOpnd, tempOpnd, dstLsrImmHigh);
    bb.InsertInsnBefore(*prePrevInsn, dstLsrInsnHigh);

    /* add     x16, x16, w0, SXTW */
    Operand &sxtw = aarch64CGFunc->CreateExtendShiftOperand(ExtendShiftOperand::kSXTW, 0, 3);
    Insn &addInsn =
        cgFunc->GetInsnBuilder()->BuildInsn(MOP_xxwaddrrre, tempOpnd, tempOpnd, sdivOpnd1, sxtw);
    bb.InsertInsnBefore(*prePrevInsn, addInsn);

    /* asr     x16, x16, #17 */
    ImmOperand &dstLsrImmChange = aarch64CGFunc->CreateImmOperand(17, k32BitSize, false);
    Insn &dstLsrInsnChange =
        cgFunc->GetInsnBuilder()->BuildInsn(MOP_xasrrri6, tempOpnd, tempOpnd, dstLsrImmChange);
    bb.InsertInsnBefore(*prePrevInsn, dstLsrInsnChange);

    /* add     x2, x16, x0, LSR #31 */
    auto &sdivOpnd0 = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
    regno_t sdivOpnd0RegNO = sdivOpnd0.GetRegisterNumber();
    RegOperand &extSdivO0 =
        aarch64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(sdivOpnd0RegNO),
                                                          k64BitSize, kRegTyInt);

    regno_t sdivOpnd1RegNum = sdivOpnd1.GetRegisterNumber();
    RegOperand &extSdivO1 =
        aarch64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(sdivOpnd1RegNum),
                                                          k64BitSize, kRegTyInt);
    /* shift bit amount is thirty-one at this insn */
    BitShiftOperand &addLsrOpnd = aarch64CGFunc->CreateBitShiftOperand(BitShiftOperand::kShiftLSR, 31, 6);
    Insn &addLsrInsn = cgFunc->GetInsnBuilder()->BuildInsn(MOP_xaddrrrs, extSdivO0, tempOpnd, extSdivO1, addLsrOpnd);
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

bool AndCmpBranchesToCsetPattern::CheckCondition(Insn &insn) {
  /* prevInsn must be "cmp" insn */
  auto &ccReg = static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd));
  prevCmpInsn = ssaInfo->GetDefInsn(ccReg);
  if (prevCmpInsn == nullptr) {
    return false;
  }
  MOperator prevCmpMop = prevCmpInsn->GetMachineOpcode();
  if (prevCmpMop != MOP_wcmpri && prevCmpMop != MOP_xcmpri) {
    return false;
  }
  /* prevPrevInsn must be "and" insn */
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
  auto &csetCond = static_cast<CondOperand&>(insn.GetOperand(kInsnSecondOpnd));
  auto &cmpImm = static_cast<ImmOperand&>(prevCmpInsn->GetOperand(kInsnThirdOpnd));
  int64 cmpImmVal = cmpImm.GetValue();
  auto &andImm = static_cast<ImmOperand&>(prevAndInsn->GetOperand(kInsnThirdOpnd));
  int64 andImmVal = andImm.GetValue();
  if ((csetCond.GetCode() == CC_EQ && cmpImmVal == andImmVal) ||
      (csetCond.GetCode() == CC_NE && cmpImmVal == 0)) {
    /* guaranteed unique point of use */
    auto &flagReg = static_cast<RegOperand&>(prevCmpInsn->GetOperand(kInsnFirstOpnd));
    InsnSet cmpFirstUseSet = GetAllUseInsn(flagReg);
    if (cmpFirstUseSet.size() > 1) {
      return false;
    }
    /* guaranteed unique point of use */
    auto &prevInsnSecondReg = prevCmpInsn->GetOperand(kInsnSecondOpnd);
    InsnSet cmpSecondUseSet = GetAllUseInsn(static_cast<RegOperand&>(prevInsnSecondReg));
    if (cmpSecondUseSet.size() > 1) {
      return false;
    }
    return true;
  }
  return false;
}

void AndCmpBranchesToCsetPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  RegOperand *dstOpnd = nullptr;
  RegOperand *srcOpnd = nullptr;
  auto &andImm = static_cast<ImmOperand&>(prevAndInsn->GetOperand(kInsnThirdOpnd));
  int64 andImmVal = andImm.GetValue();
  if (andImmVal == 1) {
    /* Method 1: Delete cmp and cset, and replace cset with and. */
    dstOpnd = &static_cast<RegOperand&>(prevAndInsn->GetOperand(kInsnFirstOpnd));
    srcOpnd = &static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
    if (dstOpnd->IsPhysicalRegister() || srcOpnd->IsPhysicalRegister()) {
      return;
    }
    VRegVersion *dstVersion = ssaInfo->FindSSAVersion(dstOpnd->GetRegisterNumber());
    VRegVersion *srcVersion = ssaInfo->FindSSAVersion(srcOpnd->GetRegisterNumber());
    CHECK_FATAL(dstVersion != nullptr, "get dstVersion failed");
    CHECK_FATAL(srcVersion != nullptr, "get srcVersion failed");
    /* Ensure that there is no use point */
    auto &insnDefReg = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
    InsnSet csetFirstUseSet = GetAllUseInsn(insnDefReg);
    if (csetFirstUseSet.size() < 1) {
      return;
    }
    /* update ssa info */
    ssaInfo->ReplaceAllUse(srcVersion, dstVersion);
    optSuccess = true;
    /* dump pattern info */
    if (CG_PEEP_DUMP) {
      std::vector<Insn*> prevs;
      prevs.emplace_back(prevAndInsn);
      prevs.emplace_back(prevCmpInsn);
      DumpAfterPattern(prevs, &insn, prevAndInsn);
    }
  } else {
    /* andImmVal is n power of 2 */
    int64 n = GetLogValueAtBase2(andImmVal);
    if (n < 0) {
      return;
    }
    /* Method 2: ubfx replaces cset. */
    /* create ubfx insn */
    auto &csetReg = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
    MOperator ubfxOp = (csetReg.GetSize() <= k32BitSize) ? MOP_wubfxrri5i5 : MOP_xubfxrri6i6;
    if (ubfxOp == MOP_wubfxrri5i5 && static_cast<uint32>(n) >= k32BitSize) {
      return;
    }
    auto &dstReg = static_cast<RegOperand&>(csetReg);
    auto &prevAndInsnSecondReg = prevAndInsn->GetOperand(kInsnSecondOpnd);
    auto &srcReg = static_cast<RegOperand&>(prevAndInsnSecondReg);
    auto *aarch64CGFunc = static_cast<AArch64CGFunc*>(cgFunc);
    ImmOperand &bitPos = aarch64CGFunc->CreateImmOperand(n, k8BitSize, false);
    ImmOperand &bitSize = aarch64CGFunc->CreateImmOperand(1, k8BitSize, false);
    Insn &ubfxInsn = cgFunc->GetInsnBuilder()->BuildInsn(ubfxOp, dstReg, srcReg, bitPos, bitSize);
    bb.ReplaceInsn(insn, ubfxInsn);
    /* update ssa info */
    ssaInfo->ReplaceInsn(insn, ubfxInsn);
    optSuccess = true;
    SetCurrInsn(&ubfxInsn);
    /* dump pattern info */
    if (CG_PEEP_DUMP) {
      std::vector<Insn*> prevs;
      prevs.emplace_back(prevAndInsn);
      prevs.emplace_back(prevCmpInsn);
      DumpAfterPattern(prevs, &insn, &ubfxInsn);
    }
  }
}

bool AndCbzBranchesToTstPattern::CheckCondition(Insn &insn) {
  /* nextInsn must be "cbz" or "cbnz" insn */
  Insn *nextInsn = insn.GetNextMachineInsn();
  if (nextInsn == nullptr ||
      (nextInsn->GetMachineOpcode() != MOP_wcbz && nextInsn->GetMachineOpcode() != MOP_xcbz)) {
    return false;
  }
  auto &andRegOp1 = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  regno_t andRegNo1 = andRegOp1.GetRegisterNumber();
  auto &cbzRegOp1 = static_cast<RegOperand&>(nextInsn->GetOperand(kInsnFirstOpnd));
  regno_t cbzRegNo1 = cbzRegOp1.GetRegisterNumber();
  if (andRegNo1 != cbzRegNo1) {
    return false;
  }
  /* If the reg will be used later, we shouldn't optimize the and insn here */
  if (IfOperandIsLiveAfterInsn(andRegOp1, *nextInsn)) {
    return false;
  }
  auto &andRegOp2 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  Operand &andOpnd3 = insn.GetOperand(kInsnThirdOpnd);
  if (andOpnd3.IsImmediate() && !static_cast<ImmOperand&>(andOpnd3).IsBitmaskImmediate(andRegOp2.GetSize())) {
    return false;
  }
  /* avoid redefine cc-reg */
  if (static_cast<AArch64CGFunc*>(cgFunc)->GetRflag() != nullptr) {
    return false;
  }
  return true;
}

void AndCbzBranchesToTstPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  Insn *nextInsn = insn.GetNextMachineInsn();
  CHECK_NULL_FATAL(nextInsn);
  /* build tst insn */
  auto &andRegOp2 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  Operand &andOpnd3 = insn.GetOperand(kInsnThirdOpnd);
  MOperator newTstOp = MOP_undef;
  if (andOpnd3.IsRegister()) {
    newTstOp = (insn.GetMachineOpcode() == MOP_wandrrr) ? MOP_wtstrr : MOP_xtstrr;
  } else {
    newTstOp = (insn.GetMachineOpcode() == MOP_wandrri12) ? MOP_wtstri32 : MOP_xtstri64;
  }
  Operand &rflag = static_cast<AArch64CGFunc*>(cgFunc)->GetOrCreateRflag();
  Insn &newInsnTst = cgFunc->GetInsnBuilder()->BuildInsn(newTstOp, rflag, andRegOp2, andOpnd3);

  /* build beq insn */
  MOperator opCode = nextInsn->GetMachineOpcode();
  bool reverse = (opCode == MOP_xcbz || opCode == MOP_wcbz);
  auto &label = static_cast<LabelOperand&>(nextInsn->GetOperand(kInsnSecondOpnd));
  MOperator jmpOperator = reverse ? MOP_beq : MOP_bne;
  Insn &newInsnJmp = cgFunc->GetInsnBuilder()->BuildInsn(jmpOperator, rflag, label);
  bb.ReplaceInsn(insn, newInsnTst);
  bb.ReplaceInsn(*nextInsn, newInsnJmp);
}

// help function for DeleteMovAfterCbzOrCbnz
// input:  bb: the bb to be checked out
//         checkCbz: to check out BB end with cbz or cbnz, if cbz, input true
//         opnd: for MOV reg, #0, opnd indicate reg
// return: according to cbz, return true if insn is cbz or cbnz and the first operand of cbz(cbnz) is same as input
//         operand
bool DeleteMovAfterCbzOrCbnzAArch64::PredBBCheck(BB &bb, bool checkCbz, const Operand &opnd, bool is64BitOnly) const {
  if (bb.GetKind() != BB::kBBIf) {
    return false;
  }

  if (bb.GetNext() == maplebe::CGCFG::GetTargetSuc(bb)) {
    return false;
  }

  Insn *condBr = cgcfg->FindLastCondBrInsn(bb);
  ASSERT(condBr != nullptr, "condBr must be found");
  if (!cgcfg->IsCompareAndBranchInsn(*condBr)) {
    return false;
  }
  MOperator mOp = condBr->GetMachineOpcode();
  if (is64BitOnly && checkCbz && mOp != MOP_xcbz) {
    return false;
  }
  if (is64BitOnly && !checkCbz && mOp != MOP_xcbnz) {
    return false;
  }
  if (!is64BitOnly && checkCbz && mOp != MOP_xcbz && mOp != MOP_wcbz) {
    return false;
  }
  if (!is64BitOnly && !checkCbz && mOp != MOP_xcbnz && mOp != MOP_wcbnz) {
    return false;
  }
  return RegOperand::IsSameRegNO(condBr->GetOperand(kInsnFirstOpnd), opnd);
}

bool DeleteMovAfterCbzOrCbnzAArch64::OpndDefByMovZero(const Insn &insn) const {
  MOperator defMop = insn.GetMachineOpcode();
  switch (defMop) {
    case MOP_wmovri32:
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

// check whether predefine insn of first operand of test_insn is exist in current BB
bool DeleteMovAfterCbzOrCbnzAArch64::NoPreDefine(Insn &testInsn) const {
  Insn *nextInsn = nullptr;
  for (Insn *insn = testInsn.GetBB()->GetFirstMachineInsn(); insn != nullptr && insn != &testInsn; insn = nextInsn) {
    nextInsn = insn->GetNextMachineInsn();
    if (!insn->IsMachineInstruction()) {
      continue;
    }
    ASSERT(!insn->IsCall(), "CG internal error, call insn should not be at the middle of the BB.");
    const InsnDesc *md = insn->GetDesc();
    uint32 opndNum = insn->GetOperandSize();
    for (uint32 i = 0; i < opndNum; ++i) {
      Operand &opnd = insn->GetOperand(i);
      if (!md->opndMD[i]->IsDef()) {
        continue;
      }
      if (opnd.IsMemoryAccessOperand()) {
        auto &memOpnd = static_cast<MemOperand&>(opnd);
        RegOperand *base = memOpnd.GetBaseRegister();
        ASSERT(base != nullptr, "nullptr check");
        ASSERT(base->IsRegister(), "expects RegOperand");
        if (RegOperand::IsSameRegNO(*base, testInsn.GetOperand(kInsnFirstOpnd)) &&
            memOpnd.GetAddrMode() == MemOperand::kBOI &&
            (memOpnd.IsPostIndexed() || memOpnd.IsPreIndexed())) {
          return false;
        }
      } else if (opnd.IsList()) {
        for (auto &operand : static_cast<const ListOperand&>(opnd).GetOperands()) {
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

bool DeleteMovAfterCbzOrCbnzAArch64::NoMoreThan32BitUse(Insn &testInsn) const {
  auto &testOpnd = static_cast<RegOperand&>(testInsn.GetOperand(kFirstOpnd));
  InsnSet regUseInsnSet = cgFunc.GetRD()->FindUseForRegOpnd(testInsn, kInsnFirstOpnd, false);
  for (auto useInsn : regUseInsnSet) {
    MOperator mop = useInsn->GetMachineOpcode();
    if (mop == MOP_pseudo_ret_int) {
      if (cgFunc.GetFunction().GetReturnType()->GetSize() > k4ByteSize) {
        return false;
      }
      continue;
    }
    uint32 optSize = useInsn->GetOperandSize();
    const InsnDesc *md = useInsn->GetDesc();
    for (uint32 i = 0; i < optSize; i++) {
      auto &opnd = useInsn->GetOperand(i);
      const auto *opndDesc = md->GetOpndDes(i);
      if (opndDesc->IsDef()) {
        continue;
      }
      if (opnd.IsRegister()) {
        auto &regOpnd = static_cast<RegOperand&>(opnd);
        if (RegOperand::IsSameRegNO(regOpnd, testOpnd) &&
            opndDesc->GetSize() > k32BitSize) {
            return false;
        }
      } else if (opnd.IsMemoryAccessOperand()) {
        auto &memOpnd = static_cast<MemOperand&>(opnd);
        auto *baseOpnd = memOpnd.GetBaseRegister();
        auto *indexOpnd = memOpnd.GetIndexRegister();
        if ((baseOpnd != nullptr) && (RegOperand::IsSameRegNO(*baseOpnd, testOpnd)) &&
            (baseOpnd->GetSize() > k32BitSize)) {
            return false;
        }
        if ((indexOpnd != nullptr) && (RegOperand::IsSameRegNO(*indexOpnd, testOpnd)) &&
            (indexOpnd->GetSize() > k32BitSize)) {
            return false;
        }
      } else if (opnd.IsList()) {
        auto &listOpnd = static_cast<ListOperand&>(opnd);
        for (auto *regOpnd : std::as_const(listOpnd.GetOperands())) {
          if (RegOperand::IsSameRegNO(*regOpnd, testOpnd) &&
              regOpnd->GetSize() > k32BitSize) {
              return false;
          }
        }
      }
    }
  }
  return true;
}

void DeleteMovAfterCbzOrCbnzAArch64::ProcessBBHandle(BB *processBB, const BB &bb, const Insn &insn) const {
  ASSERT(processBB != nullptr, "process_bb is null in ProcessBBHandle");
  MOperator condBrMop = insn.GetMachineOpcode();
  bool is64BitOnly = (condBrMop == MOP_xcbz || condBrMop == MOP_xcbnz);
  FOR_BB_INSNS_SAFE(processInsn, processBB, nextProcessInsn) {
    nextProcessInsn = processInsn->GetNextMachineInsn();
    if (!processInsn->IsMachineInstruction()) {
      continue;
    }
    // register may be a caller save register
    if (processInsn->IsCall()) {
      break;
    }
    if (!OpndDefByMovZero(*processInsn) || !NoPreDefine(*processInsn) ||
        !RegOperand::IsSameRegNO(processInsn->GetOperand(kInsnFirstOpnd), insn.GetOperand(kInsnFirstOpnd))) {
      continue;
    }
    bool toDoOpt = true;
    // process elseBB, other preds must be cbz
    if (condBrMop == MOP_wcbnz || condBrMop == MOP_xcbnz) {
      // check out all preds of process_bb
      for (auto *processBBPred : processBB->GetPreds()) {
        if (processBBPred == &bb) {
          continue;
        }
        if (!PredBBCheck(*processBBPred, true, processInsn->GetOperand(kInsnFirstOpnd), is64BitOnly)) {
          toDoOpt = false;
          break;
        }
      }
    } else {
      // process ifBB, other preds can be cbz or cbnz(one at most)
      for (auto processBBPred : processBB->GetPreds()) {
        if (processBBPred == &bb) {
          continue;
        }
        // for cbnz pred, there is one at most
        if (!PredBBCheck(*processBBPred, processBBPred != processBB->GetPrev(),
                         processInsn->GetOperand(kInsnFirstOpnd), is64BitOnly)) {
          toDoOpt = false;
          break;
        }
      }
    }
    if (!is64BitOnly && !NoMoreThan32BitUse(*processInsn)) {
      toDoOpt = false;
    }
    if (!toDoOpt) {
      continue;
    }
    processBB->RemoveInsn(*processInsn);
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

Insn *CombineMovInsnBeforeCSelAArch64::FindPrevMovInsn(const Insn &insn, regno_t regNo) const {
  for (Insn *curInsn = insn.GetPreviousMachineInsn(); curInsn != nullptr; curInsn = curInsn->GetPreviousMachineInsn()) {
    MOperator mop = curInsn->GetMachineOpcode();
    if ((mop == MOP_wmovri32 || mop == MOP_xmovri64) &&
        static_cast<RegOperand&>(curInsn->GetOperand(kInsnFirstOpnd)).GetRegisterNumber() == regNo) {
      return curInsn;
    }
    // If the register is redefined between the mov and csel insns, the optimization cannot be performed.
    if (curInsn->IsRegDefined(regNo)) {
      break;
    }
  }
  return nullptr;
}

Insn *CombineMovInsnBeforeCSelAArch64::FindPrevCmpInsn(const Insn &insn) const {
  for (Insn *curInsn = insn.GetPreviousMachineInsn(); curInsn != nullptr; curInsn = curInsn->GetPreviousMachineInsn()) {
    MOperator mop = curInsn->GetMachineOpcode();
    if (mop == MOP_wcmpri || mop == MOP_xcmpri) {
      return curInsn;
    }
  }
  return nullptr;
}

bool CombineMovInsnBeforeCSelAArch64::CheckCondition(Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  if (curMop != MOP_wcselrrrc && curMop != MOP_xcselrrrc) {
    return false;
  }

  auto &condOpnd = static_cast<CondOperand&>(insn.GetOperand(kInsnFourthOpnd));
  if (condOpnd.GetCode() != CC_NE && condOpnd.GetCode() != CC_EQ) {
    return false;
  }

  auto &opnd1 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  auto &opnd2 = static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd));
  regno_t regNo1 = opnd1.GetRegisterNumber();
  regno_t regNo2 = opnd2.GetRegisterNumber();
  if (regNo1 == regNo2) {
    return false;
  }

  insnMov1 = FindPrevMovInsn(insn, regNo1);
  if (insnMov1 == nullptr) {
    return false;
  }
  insnMov2 = FindPrevMovInsn(insn, regNo2);
  if (insnMov2 == nullptr) {
    return false;
  }
  cmpInsn = FindPrevCmpInsn(insn);
  if (cmpInsn == nullptr) {
    return false;
  }

  auto &cmpImmOpnd = static_cast<ImmOperand&>(cmpInsn->GetOperand(kInsnThirdOpnd));
  auto &immOpnd1 = static_cast<ImmOperand&>(insnMov1->GetOperand(kInsnSecondOpnd));
  auto &immOpnd2 = static_cast<ImmOperand&>(insnMov2->GetOperand(kInsnSecondOpnd));
  auto maxImm = std::max(immOpnd1.GetValue(), immOpnd2.GetValue());
  auto minImm = std::min(immOpnd1.GetValue(), immOpnd2.GetValue());
  // to avoid difference value of imm1 and imm2 overflows
  if (minImm < 0 && maxImm >= minImm - INT64_MIN) {
    return false;
  }
  auto diffValue = maxImm - minImm;
  if (cmpImmOpnd.GetValue() != diffValue) {
    return false;
  }
  // condition 5
  if (immOpnd1.GetValue() < immOpnd2.GetValue() && condOpnd.GetCode() != CC_NE && diffValue != 1) {
    return false;
  }
  // condition 6
  if (immOpnd1.GetValue() > immOpnd2.GetValue() && condOpnd.GetCode() != CC_EQ && diffValue != 1) {
    return false;
  }
  if (immOpnd1.GetValue() < immOpnd2.GetValue()) {
    needReverseCond = true;
  }
  if (diffValue == 1 && ((immOpnd1.GetValue() < immOpnd2.GetValue() && condOpnd.GetCode() != CC_NE) ||
                         (immOpnd1.GetValue() > immOpnd2.GetValue() && condOpnd.GetCode() != CC_EQ))) {
    needCsetInsn = true;
  }

  if (IfOperandIsLiveAfterInsn(opnd1, insn) || IfOperandIsLiveAfterInsn(opnd2, insn)) {
    return false;
  }

  return true;
}

void CombineMovInsnBeforeCSelAArch64::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }

  uint32 opndSize = insn.GetDesc()->opndMD[kInsnFirstOpnd]->GetSize();
  MOperator mOp = opndSize <= k32BitSize ? MOP_waddrri12 : MOP_xaddrri12;
  auto &opnd0 = insn.GetOperand(kInsnFirstOpnd);
  auto &cmpSrcopnd = cmpInsn->GetOperand(kInsnSecondOpnd);
  auto *aarFunc = static_cast<AArch64CGFunc*>(&cgFunc);
  auto *condOpnd = static_cast<CondOperand*>(&insn.GetOperand(kInsnFourthOpnd));
  CondOperand &reverseCondOpnd = aarFunc->GetCondOperand(AArch64isa::GetReverseCC(condOpnd->GetCode()));
  if (needReverseCond) {
    condOpnd = &reverseCondOpnd;
  }

  // csel insn or cset insn
  Insn *newInsn = nullptr;
  // cset insn
  if (needCsetInsn) {
    MOperator newMop = opndSize <= k32BitSize ? MOP_wcsetrc : MOP_xcsetrc;
    newInsn = &cgFunc.GetInsnBuilder()->BuildInsn(newMop, opnd0, *condOpnd, insn.GetOperand(kInsnFifthOpnd));
  } else {
    // csel insn
    newInsn = &cgFunc.GetInsnBuilder()->BuildInsn(insn.GetMachineOpcode(), opnd0, cmpSrcopnd,
        cgFunc.GetZeroOpnd(opndSize), *condOpnd, insn.GetOperand(kInsnFifthOpnd));
  }

  auto &immOpnd1 = static_cast<ImmOperand&>(insnMov1->GetOperand(kInsnSecondOpnd));
  auto &immOpnd2 = static_cast<ImmOperand&>(insnMov2->GetOperand(kInsnSecondOpnd));
  int64 value = immOpnd1.GetValue() > immOpnd2.GetValue() ? immOpnd2.GetValue() : immOpnd1.GetValue();
  // add Insn
  auto &newImmOpnd = aarFunc->CreateImmOperand(value, k12BitSize, false);
  Insn &addInsn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, opnd0, opnd0, newImmOpnd);

  bb.RemoveInsn(*insnMov2);
  bb.RemoveInsn(*insnMov1);
  bb.InsertInsnAfter(insn, addInsn);
  bb.ReplaceInsn(insn, *newInsn);
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
  if ((static_cast<BitShiftOperand&>(insn1->GetOperand(kInsnThirdOpnd)).GetShiftAmount() != 0) ||
      (static_cast<BitShiftOperand&>(insn2->GetOperand(kInsnThirdOpnd)).GetShiftAmount() != k16BitSize) ||
      (static_cast<BitShiftOperand&>(insn3->GetOperand(kInsnThirdOpnd)).GetShiftAmount() != k32BitSize) ||
      (static_cast<BitShiftOperand&>(insn4->GetOperand(kInsnThirdOpnd)).GetShiftAmount() !=
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
    Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(MOP_xldli, insn4->GetOperand(kInsnFirstOpnd),
                                                        target);
    bb.InsertInsnAfter(*insn4, newInsn);
    bb.RemoveInsn(*insn1);
    bb.RemoveInsn(*insn2);
    bb.RemoveInsn(*insn3);
    bb.RemoveInsn(*insn4);
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
    Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(optInsn[3]->GetMachineOpcode(),
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
    if (memOpnd->GetAddrMode() != MemOperand::kBOI) {
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

    auto &newBaseOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
    MemOperand *newMemOpnd =
        aarch64CGFunc->CreateMemOperand(memOpnd->GetSize(), newBaseOpnd, offOpnd, *stImmOpnd.GetSymbol());
    if (!aarch64CGFunc->IsOperandImmValid(nextMop, newMemOpnd, nextInsn->GetMemOpndIdx())) {
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
    nextInsn->SetMemOpnd(newMemOpnd);
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
      ExtendShiftOperand &exOp = aarch64CGFunc->CreateExtendShiftOperand(ExtendShiftOperand::kUXTW, 0, k32BitSize);
      MemOperand *newMemOpnd = aarch64CGFunc->CreateMemOperand(memOpnd->GetSize(), newBaseOpnd, newIndexOpnd, exOp);
      nextInsn->SetOperand(kInsnSecondOpnd, *newMemOpnd);
    } else {
      MemOperand *newMemOpnd = aarch64CGFunc->CreateMemOperand(memOpnd->GetSize(), newBaseOpnd, newIndexOpnd);
      nextInsn->SetOperand(kInsnSecondOpnd, *newMemOpnd);
    }
    bb.RemoveInsn(insn);
  }
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
  if (!insn.IsCall() || insn.GetMachineOpcode() == MOP_xblr) {
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
  Insn &strInsn = cgFunc->GetInsnBuilder()->BuildInsn(MOP_xstr, *currentCallParam.fieldValue, addr);
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

void UbfxToUxtwPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  Insn *newInsn = &cgFunc->GetInsnBuilder()->BuildInsn(
      MOP_xuxtw64, insn.GetOperand(kInsnFirstOpnd), insn.GetOperand(kInsnSecondOpnd));
  bb.ReplaceInsn(insn, *newInsn);
  SetCurrInsn(newInsn);
  optSuccess = true;
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

bool NormRevTbzToTbzPattern::CheckCondition(Insn &insn) {
  auto &revReg = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  for (Insn *nextInsn = insn.GetNextMachineInsn(); nextInsn != nullptr; nextInsn = nextInsn->GetNextMachineInsn()) {
    MOperator useMop = nextInsn->GetMachineOpcode();
    auto &useReg = static_cast<RegOperand&>(nextInsn->GetOperand(kInsnFirstOpnd));
    if ((useMop == MOP_wtbnz || useMop == MOP_xtbnz || useMop == MOP_wtbz || useMop == MOP_xtbz) &&
        useReg.Equals(revReg)) {
      if (IfOperandIsLiveAfterInsn(useReg, *nextInsn)) {
        return false;
      }
      tbzInsn = nextInsn;
      return true;
    }
    uint32 opndSize = nextInsn->GetOperandSize();
    for (uint32 i = 0; i < opndSize; i++) {
      auto &duOpnd = nextInsn->GetOperand(i);
      if (!duOpnd.IsRegister()) {
        continue;
      }
      if ((static_cast<RegOperand&>(duOpnd)).GetRegisterNumber() != revReg.GetRegisterNumber()) {
        continue;
      }
      return false;
    }
  }
  return false;
}

void NormRevTbzToTbzPattern::SetRev16Value(const uint32 &oldValue, uint32 &revValue) const {
  switch (oldValue / k8BitSize) {
    case k0BitSize:
    case k2BitSize:
    case k4BitSize:
    case k6BitSize:
      revValue = oldValue + k8BitSize;
      break;
    case k1BitSize:
    case k3BitSize:
    case k5BitSize:
    case k7BitSize:
      revValue = oldValue - k8BitSize;
      break;
    default:
      CHECK_FATAL(false, "revValue must be the above value");
  }
}

void NormRevTbzToTbzPattern::SetWrevValue(const uint32 &oldValue, uint32 &revValue) const {
  switch (oldValue / k8BitSize) {
    case k0BitSize: {
      revValue = oldValue + k24BitSize;
      break;
    }
    case k1BitSize: {
      revValue = oldValue + k8BitSize;
      break;
    }
    case k2BitSize: {
      revValue = oldValue - k8BitSize;
      break;
    }
    case k4BitSize: {
      revValue = oldValue - k24BitSize;
      break;
    }
    default:
      CHECK_FATAL(false, "revValue must be the above value");
  }
}

void NormRevTbzToTbzPattern::SetXrevValue(const uint32 &oldValue, uint32 &revValue) const {
  switch (oldValue / k8BitSize) {
    case k0BitSize:
      revValue = oldValue + k56BitSize;
      break;
    case k1BitSize:
      revValue = oldValue + k40BitSize;
      break;
    case k2BitSize:
      revValue = oldValue + k24BitSize;
      break;
    case k3BitSize:
      revValue = oldValue + k8BitSize;
      break;
    case k4BitSize:
      revValue = oldValue - k8BitSize;
      break;
    case k5BitSize:
      revValue = oldValue - k24BitSize;
      break;
    case k6BitSize:
      revValue = oldValue - k40BitSize;
      break;
    case k7BitSize:
      revValue = oldValue - k56BitSize;
      break;
    default:
      CHECK_FATAL(false, "revValue must be the above value");
  }
}

void NormRevTbzToTbzPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  auto &oldImmOpnd1 = static_cast<ImmOperand&>(tbzInsn->GetOperand(kInsnSecondOpnd));
  uint32 oldValue = static_cast<uint32>(oldImmOpnd1.GetValue());
  uint32 revValue = k0BitSize;
  MOperator curMop = insn.GetMachineOpcode();
  if (curMop == MOP_wrevrr16) {
    SetRev16Value(oldValue, revValue);
  } else if (curMop == MOP_wrevrr) {
    SetWrevValue(oldValue, revValue);
  } else if (curMop == MOP_xrevrr) {
    SetXrevValue(oldValue, revValue);
  }
  auto *aarFunc = static_cast<AArch64CGFunc*>(cgFunc);
  ImmOperand &newImmOpnd = aarFunc->CreateImmOperand(revValue, k6BitSize, false);
  MOperator useMop = tbzInsn->GetMachineOpcode();
  Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(useMop, insn.GetOperand(kInsnSecondOpnd),
                                                      newImmOpnd, tbzInsn->GetOperand(kInsnThirdOpnd));
  if (!VERIFY_INSN(&newInsn)) {
    return;
  }
  bb.ReplaceInsn(*tbzInsn, newInsn);
  optSuccess = true;
  /* dump pattern info */
  if (CG_PEEP_DUMP) {
    std::vector<Insn*> prevs;
    (void)prevs.emplace_back(&insn);
    DumpAfterPattern(prevs, tbzInsn, &newInsn);
  }
}

Insn *AddSubMergeLdStPattern::FindRegInBB(const Insn &insn, bool isAbove) const {
  regno_t regNO = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd)).GetRegisterNumber();
  for (Insn *resInsn = isAbove ? insn.GetPreviousMachineInsn() : insn.GetNextMachineInsn(); resInsn != nullptr;
       resInsn = isAbove ? resInsn->GetPreviousMachineInsn() : resInsn->GetNextMachineInsn()) {
    if (resInsn->GetDesc()->IsCall() || resInsn->GetDesc()->IsInlineAsm() || resInsn->GetDesc()->IsSpecialIntrinsic()) {
      return nullptr;
    }
    if (resInsn->ScanReg(regNO)) {
      return resInsn;
    }
  }
  return nullptr;
}

bool AddSubMergeLdStPattern::CheckCondition(Insn &insn) {
  insnDefReg = &static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
  insnUseReg = &static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd));
  regno_t insnDefRegNO = insnDefReg->GetRegisterNumber();
  regno_t insnUseRegNO = insnUseReg->GetRegisterNumber();
  if (insnDefRegNO != insnUseRegNO) {
    return false;
  }
  // Do not combine x16 until cgaggressiveopt
  if (insnDefReg->IsPhysicalRegister() && insnDefRegNO == R16) {
    return false;
  }
  nextInsn = FindRegInBB(insn, false);
  prevInsn = FindRegInBB(insn, true);
  isAddSubFront = CheckIfCanBeMerged(nextInsn, insn);
  isLdStFront = CheckIfCanBeMerged(prevInsn, insn);
  // If prev & next all can be merged, only one will be merged, otherwise #imm will be add/sub twice.
  if (isAddSubFront && isLdStFront) {
    isLdStFront = false;
  }
  return isAddSubFront || isLdStFront;
}

bool AddSubMergeLdStPattern::CheckIfCanBeMerged(const Insn *adjacentInsn, const Insn& /* insn */) {
  if (adjacentInsn == nullptr || adjacentInsn->IsVectorOp() || (!adjacentInsn->AccessMem())) {
    return false;
  }
  Operand &opnd = adjacentInsn->IsLoadStorePair() ? adjacentInsn->GetOperand(kInsnThirdOpnd)
                                                  : adjacentInsn->GetOperand(kInsnSecondOpnd);
  if (opnd.GetKind() != Operand::kOpdMem) {
    return false;
  }
  MemOperand *memOpnd = &static_cast<MemOperand &>(opnd);
  // load/store memopnd offset value must be #0
  if (memOpnd->GetAddrMode() != MemOperand::kBOI || AArch64isa::GetMemOpndOffsetValue(memOpnd) != 0) {
    return false;
  }
  RegOperand *memUseReg = memOpnd->GetBaseRegister();
  regno_t insnDefRegNO = insnDefReg->GetRegisterNumber();
  regno_t memUseRegNO = memUseReg->GetRegisterNumber();
  if (insnDefRegNO != memUseRegNO) {
    return false;
  }
  // When load/store insn def & use regno are the same, it will trigger unpredictable transfer with writeback.
  regno_t ldstDefRegNO0 = static_cast<RegOperand &>(adjacentInsn->GetOperand(kInsnFirstOpnd)).GetRegisterNumber();
  if (ldstDefRegNO0 == memUseRegNO) {
    return false;
  }
  if (adjacentInsn->IsLoadStorePair()) {
    regno_t ldstDefRegNO1 = static_cast<RegOperand &>(adjacentInsn->GetOperand(kInsnSecondOpnd)).GetRegisterNumber();
    if (ldstDefRegNO1 == memUseRegNO) {
      return false;
    }
  }
  return true;
}

void AddSubMergeLdStPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  insnToBeReplaced = isAddSubFront ? nextInsn : prevInsn;
  // isInsnAdd returns true -- add, isInsnAdd returns false -- sub.
  isInsnAdd = (insn.GetMachineOpcode() == MOP_xaddrri12);
  int64 immVal = static_cast<ImmOperand &>(insn.GetOperand(kInsnThirdOpnd)).GetValue();
  // Pre/Post-index simm cannot be absent, when ofstVal is #0, the assembly file will appear memopnd: [x0]!
  if (immVal == static_cast<int64>(k0BitSize)) {
    return;
  }
  Operand &opnd = insnToBeReplaced->IsLoadStorePair() ? insnToBeReplaced->GetOperand(kInsnThirdOpnd)
                                                      : insnToBeReplaced->GetOperand(kInsnSecondOpnd);
  MemOperand *memOpnd = &static_cast<MemOperand &>(opnd);
  ImmOperand &newImmOpnd =
      static_cast<AArch64CGFunc *>(cgFunc)->CreateImmOperand((isInsnAdd ? immVal : (-immVal)), k64BitSize, true);
  MemOperand *newMemOpnd = static_cast<AArch64CGFunc *>(cgFunc)->CreateMemOperand(
      memOpnd->GetSize(), *insnUseReg, newImmOpnd, (isAddSubFront ? MemOperand::kPreIndex : MemOperand::kPostIndex));
  Insn *newInsn = nullptr;
  if (insnToBeReplaced->IsLoadStorePair()) {
    newInsn = &static_cast<AArch64CGFunc *>(cgFunc)->GetInsnBuilder()->BuildInsn(
        insnToBeReplaced->GetMachineOpcode(), insnToBeReplaced->GetOperand(kInsnFirstOpnd),
        insnToBeReplaced->GetOperand(kInsnSecondOpnd), *newMemOpnd);
  } else {
    newInsn = &static_cast<AArch64CGFunc *>(cgFunc)->GetInsnBuilder()->BuildInsn(
        insnToBeReplaced->GetMachineOpcode(), insnToBeReplaced->GetOperand(kInsnFirstOpnd), *newMemOpnd);
  }
  if (!VERIFY_INSN(newInsn)) {
    return;
  } else {
    // Both [RSP, #imm]! and [RSP], #imm should be set true for stackdef.
    if (insnUseReg->GetRegisterNumber() == RSP) {
      newInsn->SetStackDef(true);
    }
    bb.ReplaceInsn(*insnToBeReplaced, *newInsn);
    bb.RemoveInsn(insn);
  }
}

void UbfxAndCbzToTbzPattern::Run(BB& /* bb */, Insn &insn) {
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
  Insn *newInsn = &cgFunc->GetInsnBuilder()->BuildInsn(newMop, opnd2, imm3, label);
  if (!VERIFY_INSN(newInsn)) {
    return;
  }
  BB *useInsnBB = useInsn->GetBB();
  useInsnBB->ReplaceInsn(*useInsn, *newInsn);
  if (ssaInfo) {
    // update ssa info
    ssaInfo->ReplaceInsn(*useInsn, *newInsn);
  } else {
    useInsnBB->RemoveInsn(insn);
  }
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
  if (ssaInfo) {
    InsnSet useInsns = GetAllUseInsn(opnd1);
    if (useInsns.size() != 1) {
      return false;
    }
    useInsn = *useInsns.begin();
  } else {
    useInsn = insn.GetNextMachineInsn();
  }
  if (useInsn == nullptr) {
    return false;
  }
  if (!ssaInfo) {
    regno_t regNO1 = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd)).GetRegisterNumber();
    regno_t regNO2 = static_cast<RegOperand&>(useInsn->GetOperand(kInsnFirstOpnd)).GetRegisterNumber();
    if ((regNO1 != regNO2) ||
        IfOperandIsLiveAfterInsn(static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd)), *useInsn)) {
      return false;
    }
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

bool AddCmpZeroAArch64::CheckAddCmpZeroCheckAdd(const Insn &prevInsn, const Insn &insn) const {
  MOperator mop = prevInsn.GetMachineOpcode();
  switch (mop) {
    case MOP_xaddrrr:
    case MOP_waddrrr:
    case MOP_xaddrrrs:
    case MOP_waddrrrs: {
      RegOperand opnd0 = static_cast<RegOperand&>(prevInsn.GetOperand(kInsnFirstOpnd));
      RegOperand opnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
      if (opnd0.Equals(opnd) && insn.GetDesc()->GetOpndDes(kInsnSecondOpnd)->GetSize() ==
          prevInsn.GetDesc()->GetOpndDes(kInsnFirstOpnd)->GetSize()) {
        return true;
      } else {
        return false;
      }
    }
    case MOP_waddrri12:
    case MOP_xaddrri12: {
      RegOperand opnd0 = static_cast<RegOperand&>(prevInsn.GetOperand(kInsnFirstOpnd));
      RegOperand opnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
      if (!(opnd0.Equals(opnd) && insn.GetDesc()->GetOpndDes(kInsnSecondOpnd)->GetSize() ==
          prevInsn.GetDesc()->GetOpndDes(kInsnFirstOpnd)->GetSize())) {
        return false;
      }
      auto &immOpnd = static_cast<ImmOperand&>(prevInsn.GetOperand(kInsnThirdOpnd));
      auto *aarch64CGFunc = static_cast<AArch64CGFunc*>(&cgFunc);
      if (aarch64CGFunc->IsOperandImmValid(prevInsn.GetMachineOpcode(), &immOpnd, kInsnThirdOpnd)) {
        return true;
      } else {
        return false;
      }
    }
    default:
      break;
  }
  return false;
}

bool AddCmpZeroAArch64::CheckAddCmpZeroContinue(const Insn &insn, const RegOperand &opnd) const {
  // check if insn will redef target reg or status reg
  if (insn.GetDesc()->IsCall() || insn.GetDesc()->IsSpecialCall()) {
    return false;
  }
  for (uint32 i = 0; i < insn.GetOperandSize(); ++i) {
    if (insn.GetDesc()->GetOpndDes(i) == &OpndDesc::CCS) {
      return false;
    }
    if (insn.GetOperand(i).IsRegister()) {
      RegOperand &opnd0 = static_cast<RegOperand&>(insn.GetOperand(i));
      if (insn.GetDesc()->GetOpndDes(i)->IsDef() && opnd0.RegNumEqual(opnd)) {
        return false;
      }
    }
  }
  return true;
}

Insn* AddCmpZeroAArch64::CheckAddCmpZeroAArch64Pattern(Insn &insn, const RegOperand &opnd) {
  Insn *prevInsn = insn.GetPrev();
  while (prevInsn != nullptr) {
    if (!prevInsn->IsMachineInstruction()) {
      prevInsn = prevInsn->GetPrev();
      continue;
    }
    if (CheckAddCmpZeroCheckAdd(*prevInsn, insn)) {
      if (CheckAddCmpZeroCheckCond(insn)) {
        return prevInsn;
      } else {
        return nullptr;
      }
    }
    if (!CheckAddCmpZeroContinue(*prevInsn, opnd)) {
      return nullptr;
    }
    prevInsn = prevInsn->GetPrev();
  }
  return nullptr;
}

bool AddCmpZeroAArch64::CheckAddCmpZeroCheckCond(const Insn &insn) const {
  Insn *nextInsn = insn.GetNext();
  while (nextInsn != nullptr) {
    if (!nextInsn->IsMachineInstruction()) {
      nextInsn = nextInsn->GetNext();
      continue;
    }
    for (uint32 i = 0; i < nextInsn->GetOperandSize(); ++i) {
      if (nextInsn->GetDesc()->GetOpndDes(i) == &OpndDesc::Cond) {
        CondOperand& cond = static_cast<CondOperand&>(nextInsn->GetOperand(i));
        if (cond.GetCode() == CC_EQ) {
          return true;
        } else {
          return false;
        }
      }
    }
    nextInsn = nextInsn->GetNext();
  }
  return false;
}

void AddCmpZeroAArch64::Run(BB &bb, Insn &insn) {
  MOperator mop = insn.GetMachineOpcode();
  if (mop != MOP_wcmpri && mop != MOP_xcmpri) {
    return;
  }
  auto &opnd2 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  auto &opnd3 = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
  if (!opnd3.IsZero()) {
    return;
  }

  Insn *prevAddInsn = CheckAddCmpZeroAArch64Pattern(insn, opnd2);
  if (!prevAddInsn) {
    return;
  }
  bool isAddShift = false;
  MOperator newMop = GetMopUpdateAPSR(prevAddInsn->GetMachineOpcode(), isAddShift);
  Insn *newInsn = nullptr;
  if (isAddShift) {
    newInsn = &cgFunc.GetInsnBuilder()->BuildInsn(newMop, insn.GetOperand(kInsnFirstOpnd),
                                                  prevAddInsn->GetOperand(kInsnFirstOpnd),
                                                  prevAddInsn->GetOperand(kInsnSecondOpnd),
                                                  prevAddInsn->GetOperand(kInsnThirdOpnd),
                                                  prevAddInsn->GetOperand(kInsnFourthOpnd));
  } else {
    newInsn = &cgFunc.GetInsnBuilder()->BuildInsn(newMop, insn.GetOperand(kInsnFirstOpnd),
                                                  prevAddInsn->GetOperand(kInsnFirstOpnd),
                                                  prevAddInsn->GetOperand(kInsnSecondOpnd),
                                                  prevAddInsn->GetOperand(kInsnThirdOpnd));
  }
  bb.ReplaceInsn(*prevAddInsn, *newInsn);
  bb.RemoveInsn(insn);
}

bool ComplexExtendWordLslPattern::CheckCondition(Insn &insn) {
  if (insn.GetMachineOpcode() != MOP_xsxtw64 && insn.GetMachineOpcode() != MOP_xuxtw64) {
    return false;
  }
  useInsn = insn.GetNextMachineInsn();
  if (useInsn == nullptr) {
    return false;
  }
  MOperator nextMop = useInsn->GetMachineOpcode();
  if (nextMop != MOP_xlslrri6) {
    return false;
  }
  return true;
}

void ComplexExtendWordLslPattern::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  MOperator curMop = insn.GetMachineOpcode();
  auto &lslImmOpnd = static_cast<ImmOperand&>(useInsn->GetOperand(kInsnThirdOpnd));
  ASSERT(lslImmOpnd.GetValue() >= 0, "invalid immOpnd of lsl");
  if (lslImmOpnd.GetValue() > k32BitSize) {
    return;
  }
  auto &extDefOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  auto &lslUseOpnd = static_cast<RegOperand&>(useInsn->GetOperand(kInsnSecondOpnd));
  regno_t extDefRegNO = extDefOpnd.GetRegisterNumber();
  regno_t lslUseRegNO = lslUseOpnd.GetRegisterNumber();
  if (extDefRegNO != lslUseRegNO || IfOperandIsLiveAfterInsn(extDefOpnd, *useInsn)) {
    return;
  }

  MOperator mopNew = (curMop == MOP_xsxtw64 ? MOP_xsbfizrri6i6 : MOP_xubfizrri6i6);
  auto &extUseOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  auto &lslDefOpnd = static_cast<RegOperand&>(useInsn->GetOperand(kInsnFirstOpnd));
  ImmOperand &newImmOpnd = static_cast<AArch64CGFunc*>(cgFunc)->CreateImmOperand(k32BitSize, k6BitSize, false);
  Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(mopNew, lslDefOpnd, extUseOpnd, lslImmOpnd, newImmOpnd);
  bb.RemoveInsn(*useInsn);
  bb.ReplaceInsn(insn, newInsn);
  optSuccess = true;
}

bool AddCmpZeroPatternSSA::CheckCondition(Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  if (curMop != MOP_wcmpri && curMop != MOP_xcmpri) {
    return false;
  }
  auto &immOpnd = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
  if (!immOpnd.IsZero()) {
    return false;
  }

  auto &ccReg = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  prevAddInsn = ssaInfo->GetDefInsn(ccReg);
  if (prevAddInsn == nullptr) {
    return false;
  }
  MOperator prevAddMop = prevAddInsn->GetMachineOpcode();
  if (prevAddMop != MOP_xaddrrr && prevAddMop != MOP_xaddrri12 &&
      prevAddMop != MOP_waddrrr && prevAddMop != MOP_waddrri12 &&
      prevAddMop != MOP_xaddrrrs && prevAddMop != MOP_waddrrrs) {
    return false;
  }
  Insn *nextInsn = insn.GetNext();
  while (nextInsn != nullptr) {
    if (!nextInsn->IsMachineInstruction()) {
      return false;
    }
    for (uint32 i = 0; i < nextInsn->GetOperandSize(); ++i) {
      if (nextInsn->GetDesc()->GetOpndDes(i) == &OpndDesc::Cond) {
        CondOperand& cond = static_cast<CondOperand&>(nextInsn->GetOperand(i));
        if (cond.GetCode() == CC_EQ) {
          return true;
        } else {
          return false;
        }
      }
    }
    nextInsn = nextInsn->GetNext();
  }
  return false;
}

void AddCmpZeroPatternSSA::Run(BB &bb, Insn &insn) {
  if (!CheckCondition(insn)) {
    return;
  }
  bool isShiftAdd = false;
  MOperator prevAddMop = prevAddInsn->GetMachineOpcode();
  MOperator newAddMop = GetMopUpdateAPSR(prevAddMop, isShiftAdd);
  ASSERT(newAddMop != MOP_undef, "unknown Add code");
  /*
   * Since new opnd can not be defined in SSA ReplaceInsn, we should avoid pattern matching again.
   * For "adds" can only be inserted in this phase, so we could do a simple check.
   */
  Insn *nextInsn = insn.GetNext();
  while (nextInsn != nullptr) {
    if (!nextInsn->IsMachineInstruction()) {
      nextInsn = nextInsn->GetNext();
      continue;
    }
    MOperator nextMop = nextInsn->GetMachineOpcode();
    if (nextMop == newAddMop) {
      return;
    }
    nextInsn = nextInsn->GetNext();
  }

  Insn *newInsn = nullptr;
  Operand &rflag = insn.GetOperand(kInsnFirstOpnd);
  if (isShiftAdd) {
    newInsn = &cgFunc->GetInsnBuilder()->BuildInsn(newAddMop, rflag, prevAddInsn->GetOperand(kInsnFirstOpnd),
                                                   prevAddInsn->GetOperand(kInsnSecondOpnd),
                                                   prevAddInsn->GetOperand(kInsnThirdOpnd),
                                                   prevAddInsn->GetOperand(kInsnFourthOpnd));
  } else {
    newInsn = &cgFunc->GetInsnBuilder()->BuildInsn(newAddMop, rflag, prevAddInsn->GetOperand(kInsnFirstOpnd),
                                                   prevAddInsn->GetOperand(kInsnSecondOpnd),
                                                   prevAddInsn->GetOperand(kInsnThirdOpnd));
  }
  bb.InsertInsnAfter(insn, *newInsn);
  /* update ssa info */
  auto *a64SSAInfo = static_cast<AArch64CGSSAInfo*>(ssaInfo);
  a64SSAInfo->CreateNewInsnSSAInfo(*newInsn);
  SetCurrInsn(newInsn);

  /* dump pattern info */
  if (CG_PEEP_DUMP) {
    std::vector<Insn*> prevs;
    prevs.emplace_back(prevAddInsn);
    prevs.emplace_back(&insn);
    DumpAfterPattern(prevs, newInsn, nullptr);
  }
}
}  /* namespace maplebe */
