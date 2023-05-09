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
#include "aarch64_ico.h"
#include "ico.h"
#include "cg.h"
#include "cg_option.h"
#include "aarch64_isa.h"
#include "aarch64_insn.h"
#include "aarch64_cgfunc.h"
#include "aarch64_cg.h"
#include "aarch64_isa.h"

/*
 * This phase implements if-conversion optimization,
 * which tries to convert conditional branches into cset/csel instructions
 */
namespace maplebe {
void AArch64IfConversionOptimizer::InitOptimizePatterns() {
  singlePassPatterns.emplace_back(memPool->New<AArch64ICOIfThenElsePattern>(*cgFunc));
  if (cgFunc->GetMirModule().IsCModule()) {
    singlePassPatterns.emplace_back(memPool->New<AArch64ICOSameCondPattern>(*cgFunc));
  }
  singlePassPatterns.emplace_back(memPool->New<AArch64ICOMorePredsPattern>(*cgFunc));
}

/* build ccmp Insn */
Insn *AArch64ICOPattern::BuildCcmpInsn(ConditionCode ccCode, ConditionCode ccCode2,
    const Insn &cmpInsn, Insn *&moveInsn) const {
  Operand &opnd0 = cmpInsn.GetOperand(kInsnFirstOpnd);
  Operand &opnd1 = cmpInsn.GetOperand(kInsnSecondOpnd);
  Operand &opnd2 = cmpInsn.GetOperand(kInsnThirdOpnd);
  if (!opnd2.IsImmediate() && !opnd2.IsRegister()) {
      return nullptr;
  }
  /* ccmp has only int opnd */
  if (opnd2.IsImmediate() && !static_cast<ImmOperand&>(opnd2).IsIntImmediate()) {
    return nullptr;
  }
  if (opnd2.IsRegister() && !static_cast<RegOperand&>(opnd1).IsOfIntClass()) {
    return nullptr;
  }
  /* ccmp has only int opnd */
  if (!static_cast<RegOperand&>(opnd1).IsOfIntClass()) {
    return nullptr;
  }
  AArch64CGFunc *func = static_cast<AArch64CGFunc*>(cgFunc);
  uint32 nzcv = GetNZCV(ccCode2, false);
  if (nzcv == k16BitSize) {
    return nullptr;
  }
  ImmOperand &opnd3 = func->CreateImmOperand(PTY_u8, nzcv);
  CondOperand &cond = static_cast<AArch64CGFunc *>(cgFunc)->GetCondOperand(ccCode);
  uint32 dSize = opnd1.GetSize();

  MOperator mOpCode = (dSize == k64BitSize ? MOP_xccmprric : MOP_wccmprric);
  auto moveAble = static_cast<ImmOperand&>(opnd2).IsSingleInstructionMovable(
      AArch64CG::kMd[cmpInsn.GetMachineOpcode()].GetOpndDes(1)->GetSize());
  if (!moveAble) {
    return nullptr;
  }
  auto *newReg = &opnd2;
  if (opnd2.IsImmediate()) {
    if (static_cast<ImmOperand&>(opnd2).GetValue() >= k32BitSize) {
      newReg = cgFunc->GetTheCFG()->CreateVregFromReg(static_cast<RegOperand&>(opnd1));
      uint32 mOp = (opnd2.GetSize() == 64 ? (opnd2.IsImmediate() ? MOP_xmovri64 : MOP_xmovrr) :
          (opnd2.IsImmediate() ? MOP_wmovri32 : MOP_wmovrr));
      moveInsn = &cgFunc->GetInsnBuilder()->BuildInsn(mOp, *newReg, opnd2);
    } else {
      mOpCode = (dSize == k64BitSize ? MOP_xccmpriic : MOP_wccmpriic);
    }
  }
  std::vector<Operand*> opnds;
  opnds.emplace_back(&opnd0);
  opnds.emplace_back(&opnd1);
  opnds.emplace_back(newReg);
  opnds.emplace_back(&opnd3);
  opnds.emplace_back(&cond);
  opnds.emplace_back(&opnd0);
  return &cgFunc->GetInsnBuilder()->BuildInsn(mOpCode, opnds);
}

/* Rooted ccCode resource NZCV */
uint32 AArch64ICOPattern::GetNZCV(ConditionCode ccCode, bool inverse) {
  switch (ccCode) {
    case CC_EQ:
      return inverse ? k4BitSize : k0BitSize;
    case CC_HS:
      return inverse ? k2BitSize : k0BitSize;
    case CC_MI:
      return inverse ? k8BitSize : k0BitSize;
    case CC_VS:
      return inverse ? k1BitSize : k0BitSize;
    case CC_VC:
      return inverse ? k0BitSize : k1BitSize;
    case CC_LS:
      return inverse ? k4BitSize : k2BitSize;
    case CC_LO:
      return inverse ? k0BitSize : k2BitSize;
    case CC_NE:
      return inverse ? k0BitSize : k4BitSize;
    case CC_HI:
      return inverse ? k2BitSize : k4BitSize;
    case CC_PL:
      return inverse ? k0BitSize : k8BitSize;
    default:
      return k16BitSize;
  }
}

Insn *AArch64ICOPattern::BuildCmpInsn(const Insn &condBr) const {
  AArch64CGFunc *func = static_cast<AArch64CGFunc*>(cgFunc);
  RegOperand &reg = static_cast<RegOperand&>(condBr.GetOperand(0));
  PrimType ptyp = (reg.GetSize() == k64BitSize) ? PTY_u64 : PTY_u32;
  ImmOperand &numZero = func->CreateImmOperand(ptyp, 0);
  Operand &rflag = func->GetOrCreateRflag();
  MOperator mopCode = (reg.GetSize() == k64BitSize) ? MOP_xcmpri : MOP_wcmpri;
  Insn &cmpInsn = func->GetInsnBuilder()->BuildInsn(mopCode, rflag, reg, numZero);
  return &cmpInsn;
}

bool AArch64ICOIfThenElsePattern::IsExpansionMOperator(const Insn &insn) const {
  MOperator mOpCode = insn.GetMachineOpcode();
  return mOpCode >= MOP_xuxtb32 && mOpCode <= MOP_xuxtw64;
}

bool AArch64ICOIfThenElsePattern::IsMovMOperator(const Insn &insn) const {
  MOperator mOpCode = insn.GetMachineOpcode();
  return mOpCode >= MOP_xmovrr && mOpCode <= MOP_xvmovd;
}

bool AArch64ICOIfThenElsePattern::IsShiftMOperator(const Insn &insn) const {
  MOperator mOpCode = insn.GetMachineOpcode();
  return mOpCode >= MOP_xlslrri6 && mOpCode <= MOP_wrorrrr;
}

bool AArch64ICOIfThenElsePattern::IsEorMOperator(const Insn &insn) const {
  MOperator mOpCode = insn.GetMachineOpcode();
  return (mOpCode >= MOP_xeorrrr && mOpCode <= MOP_weorrri12);
}

bool AArch64ICOIfThenElsePattern::Has2SrcOpndSetInsn(const Insn &insn) const {
  return IsEorMOperator(insn) || IsShiftMOperator(insn) || cgFunc->GetTheCFG()->IsAddOrSubInsn(insn);
}

bool AArch64ICOIfThenElsePattern::IsSetInsnMOperator(const Insn &insn) const {
  return IsExpansionMOperator(insn) || IsMovMOperator(insn) || Has2SrcOpndSetInsn(insn);
}

bool AArch64ICOIfThenElsePattern::IsSetInsn(const Insn &insn, Operand **dest, std::vector<Operand*> &src) const {
  if (IsSetInsnMOperator(insn)) {
    *dest = &(insn.GetOperand(0));
    for (uint32 i = 1; i < insn.GetOperandSize(); ++i) {
      (void)src.emplace_back(&(insn.GetOperand(i)));
    }
    return true;
  }
  *dest = nullptr;
  src.clear();
  return false;
}

ConditionCode AArch64ICOPattern::Encode(MOperator mOp, bool inverse) const {
  switch (mOp) {
    case MOP_bmi:
      return inverse ? CC_PL : CC_MI;
    case MOP_bvc:
      return inverse ? CC_VS : CC_VC;
    case MOP_bls:
      return inverse ? CC_HI : CC_LS;
    case MOP_blt:
      return inverse ? CC_GE : CC_LT;
    case MOP_ble:
      return inverse ? CC_GT : CC_LE;
    case MOP_bcs:
      return inverse ? CC_CC : CC_CS;
    case MOP_bcc:
      return inverse ? CC_CS : CC_CC;
    case MOP_beq:
      return inverse ? CC_NE : CC_EQ;
    case MOP_bne:
      return inverse ? CC_EQ : CC_NE;
    case MOP_blo:
      return inverse ? CC_HS : CC_LO;
    case MOP_bpl:
      return inverse ? CC_MI : CC_PL;
    case MOP_bhs:
      return inverse ? CC_LO : CC_HS;
    case MOP_bvs:
      return inverse ? CC_VC : CC_VS;
    case MOP_bhi:
      return inverse ? CC_LS : CC_HI;
    case MOP_bgt:
      return inverse ? CC_LE : CC_GT;
    case MOP_bge:
      return inverse ? CC_LT : CC_GE;
    case MOP_wcbnz:
      return inverse ? CC_EQ : CC_NE;
    case MOP_xcbnz:
      return inverse ? CC_EQ : CC_NE;
    case MOP_wcbz:
      return inverse ? CC_NE : CC_EQ;
    case MOP_xcbz:
      return inverse ? CC_NE : CC_EQ;
    default:
      return kCcLast;
  }
}

Insn *AArch64ICOPattern::BuildCondSet(const Insn &branch, RegOperand &reg, bool inverse) const {
  ConditionCode ccCode = Encode(branch.GetMachineOpcode(), inverse);
  ASSERT(ccCode != kCcLast, "unknown cond, ccCode can't be kCcLast");
  AArch64CGFunc *func = static_cast<AArch64CGFunc*>(cgFunc);
  CondOperand &cond = func->GetCondOperand(ccCode);
  Operand &rflag = func->GetOrCreateRflag();
  MOperator mopCode = (reg.GetSize() == k64BitSize) ? MOP_xcsetrc : MOP_wcsetrc;
  return &func->GetInsnBuilder()->BuildInsn(mopCode, reg, cond, rflag);
}

Insn *AArch64ICOPattern::BuildCondSel(const Insn &branch, MOperator mOp, RegOperand &dst, RegOperand &src1,
                                      RegOperand &src2) const {
  ConditionCode ccCode = Encode(branch.GetMachineOpcode(), false);
  ASSERT(ccCode != kCcLast, "unknown cond, ccCode can't be kCcLast");
  CondOperand &cond = static_cast<AArch64CGFunc *>(cgFunc)->GetCondOperand(ccCode);
  Operand &rflag = static_cast<AArch64CGFunc *>(cgFunc)->GetOrCreateRflag();
  return &cgFunc->GetInsnBuilder()->BuildInsn(mOp, dst, src1, src2, cond, rflag);
}

void AArch64ICOIfThenElsePattern::GenerateInsnForImm(const Insn &branchInsn, Operand &ifDest, Operand &elseDest,
                                                     RegOperand &destReg, std::vector<Insn*> &generateInsn) const {
  ImmOperand &imm1 = static_cast<ImmOperand&>(ifDest);
  ImmOperand &imm2 = static_cast<ImmOperand&>(elseDest);
  bool inverse = imm1.IsZero() && imm2.IsOne();
  if (inverse || (imm2.IsZero() && imm1.IsOne())) {
    Insn *csetInsn = BuildCondSet(branchInsn, destReg, inverse);
    ASSERT(csetInsn != nullptr, "build a insn failed");
    generateInsn.emplace_back(csetInsn);
  } else if (imm1.GetValue() == imm2.GetValue()) {
    bool destIsIntTy = destReg.IsOfIntClass();
    MOperator mOp = destIsIntTy ? ((destReg.GetSize() == k64BitSize ? MOP_xmovri64 : MOP_wmovri32)) :
                    ((destReg.GetSize() == k64BitSize ? MOP_xdfmovri : MOP_wsfmovri));
    Insn &tempInsn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, destReg, imm1);
    generateInsn.emplace_back(&tempInsn);
  } else {
    bool destIsIntTy = destReg.IsOfIntClass();
    uint32 dSize = destReg.GetSize();
    bool isD64 = dSize == k64BitSize;
    MOperator mOp = destIsIntTy ? ((destReg.GetSize() == k64BitSize ? MOP_xmovri64 : MOP_wmovri32)) :
                    ((destReg.GetSize() == k64BitSize ? MOP_xdfmovri : MOP_wsfmovri));
    RegOperand *tempTarIf = nullptr;
    if (imm1.IsZero()) {
      tempTarIf = &cgFunc->GetZeroOpnd(dSize);
    } else {
      tempTarIf = cgFunc->GetTheCFG()->CreateVregFromReg(destReg);
      Insn &tempInsnIf = cgFunc->GetInsnBuilder()->BuildInsn(mOp, *tempTarIf, imm1);
      generateInsn.emplace_back(&tempInsnIf);
    }

    RegOperand *tempTarElse = nullptr;
    if (imm2.IsZero()) {
      tempTarElse = &cgFunc->GetZeroOpnd(dSize);
    } else {
      tempTarElse = cgFunc->GetTheCFG()->CreateVregFromReg(destReg);
      Insn &tempInsnElse = cgFunc->GetInsnBuilder()->BuildInsn(mOp, *tempTarElse, imm2);
      generateInsn.emplace_back(&tempInsnElse);
    }

    bool isIntTy = destReg.IsOfIntClass();
    MOperator mOpCode = isIntTy ? (isD64 ? MOP_xcselrrrc : MOP_wcselrrrc)
                                : (isD64 ? MOP_dcselrrrc : (dSize == k32BitSize ? MOP_scselrrrc : MOP_hcselrrrc));
    Insn *cselInsn = BuildCondSel(branchInsn, mOpCode, destReg, *tempTarIf, *tempTarElse);
    CHECK_FATAL(cselInsn != nullptr, "build a csel insn failed");
    generateInsn.emplace_back(cselInsn);
  }
}

RegOperand *AArch64ICOIfThenElsePattern::GenerateRegAndTempInsn(Operand &dest, const RegOperand &destReg,
                                                                std::vector<Insn*> &generateInsn) const {
  RegOperand *reg = nullptr;
  if (!dest.IsRegister()) {
    bool destIsIntTy = destReg.IsOfIntClass();
    bool isDest64 = destReg.GetSize() == k64BitSize;
    MOperator mOp = destIsIntTy ? (isDest64 ? MOP_xmovri64 : MOP_wmovri32) : (isDest64 ? MOP_xdfmovri : MOP_wsfmovri);
    reg = cgFunc->GetTheCFG()->CreateVregFromReg(destReg);
    ImmOperand &tempSrcElse = static_cast<ImmOperand&>(dest);
    if (tempSrcElse.IsZero()) {
      return &cgFunc->GetZeroOpnd(destReg.GetSize());
    }
    Insn &tempInsn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, *reg, tempSrcElse);
    generateInsn.emplace_back(&tempInsn);
    return reg;
  } else {
    return (static_cast<RegOperand*>(&dest));
  }
}

void AArch64ICOIfThenElsePattern::GenerateInsnForReg(const Insn &branchInsn, Operand &ifDest, Operand &elseDest,
                                                     RegOperand &destReg, std::vector<Insn*> &generateInsn) const {
  RegOperand *tReg = static_cast<RegOperand*>(&ifDest);
  RegOperand *eReg = static_cast<RegOperand*>(&elseDest);

  /* mov w0, w1   mov w0, w1  --> mov w0, w1 */
  if (eReg->GetRegisterNumber() == tReg->GetRegisterNumber()) {
    uint32 dSize = destReg.GetSize();
    bool srcIsIntTy = tReg->IsOfIntClass();
    bool destIsIntTy = destReg.IsOfIntClass();
    MOperator mOp;
    if (dSize == k64BitSize) {
      mOp = srcIsIntTy ? (destIsIntTy ? MOP_xmovrr : MOP_xvmovdr) : (destIsIntTy ? MOP_xvmovrd : MOP_xvmovd);
    } else {
      mOp = srcIsIntTy ? (destIsIntTy ? MOP_wmovrr : MOP_xvmovsr) : (destIsIntTy ? MOP_xvmovrs : MOP_xvmovs);
    }
    Insn &tempInsnIf = cgFunc->GetInsnBuilder()->BuildInsn(mOp, destReg, *tReg);
    generateInsn.emplace_back(&tempInsnIf);
  } else {
    uint32 dSize = destReg.GetSize();
    bool isIntTy = destReg.IsOfIntClass();
    MOperator mOpCode = isIntTy ? (dSize == k64BitSize ? MOP_xcselrrrc : MOP_wcselrrrc)
                                : (dSize == k64BitSize ? MOP_dcselrrrc : (dSize == k32BitSize ?
                                                                          MOP_scselrrrc : MOP_hcselrrrc));
    Insn *cselInsn = BuildCondSel(branchInsn, mOpCode, destReg, *tReg, *eReg);
    CHECK_FATAL(cselInsn != nullptr, "build a csel insn failed");
    generateInsn.emplace_back(cselInsn);
  }
}

Operand *AArch64ICOIfThenElsePattern::GetDestReg(const std::map<Operand*, std::vector<Operand*>> &destSrcMap,
                                                 const RegOperand &destReg) const {
  Operand *dest = nullptr;
  for (const auto &destSrcPair : destSrcMap) {
    ASSERT(destSrcPair.first->IsRegister(), "opnd must be register");
    RegOperand *destRegInMap = static_cast<RegOperand*>(destSrcPair.first);
    ASSERT(destRegInMap != nullptr, "nullptr check");
    if (destRegInMap->GetRegisterNumber() == destReg.GetRegisterNumber()) {
      if (destSrcPair.second.size() > 1) {
        dest = destSrcPair.first;
      } else {
        dest = destSrcPair.second[0];
      }
      break;
    }
  }
  return dest;
}

bool AArch64ICOIfThenElsePattern::CheckHasSameDestSize(std::vector<Insn*> &lInsn, std::vector<Insn*> &rInsn) const {
  if (lInsn.size() == rInsn.size() && rInsn.size() == 1 && IsMovMOperator(*lInsn[0]) && IsMovMOperator(*rInsn[0])) {
    RegOperand *rDestReg = static_cast<RegOperand*>(&rInsn[0]->GetOperand(0));
    RegOperand *lDestReg = static_cast<RegOperand*>(&lInsn[0]->GetOperand(0));
    if (lDestReg->GetRegisterNumber() == rDestReg->GetRegisterNumber() &&
        rInsn[0]->GetOperandSize(0) != lInsn[0]->GetOperandSize(0)) {
      return false;
    }
  }
  return true;
}

bool AArch64ICOIfThenElsePattern::BuildCondMovInsn(const BB &bb,
                                                   const DestSrcMap &destSrcTempMap,
                                                   bool elseBBIsProcessed,
                                                   std::vector<Insn*> &generateInsn,
                                                   const Insn *toBeRremoved2CmpBB) const {
  Insn *branchInsn = cgFunc->GetTheCFG()->FindLastCondBrInsn(*cmpBB);
  FOR_BB_INSNS_CONST(insn, (&bb)) {
    if (!insn->IsMachineInstruction() || insn->IsBranch()) {
      continue;
    }
    Operand *dest = nullptr;
    std::vector<Operand*> src;

    if (!IsSetInsn(*insn, &dest, src)) {
      ASSERT(false, "insn check");
    }
    ASSERT(dest->IsRegister(), "register check");
    RegOperand *destReg = static_cast<RegOperand*>(dest);

    Operand *elseDest = GetDestReg(destSrcTempMap.elseDestSrcMap, *destReg);
    Operand *ifDest = GetDestReg(destSrcTempMap.ifDestSrcMap, *destReg);

    if (elseBBIsProcessed) {
      if (elseDest != nullptr) {
        continue;
      }
      elseDest = dest;
      ASSERT(ifDest != nullptr, "null ptr check");
      if (!bb.GetLiveOut()->TestBit(destReg->GetRegisterNumber()) && insn != toBeRremoved2CmpBB) {
        // When another branch does not assign a value to destReg and destReg is not used outside bb,
        // it means that the instruction is redundant.
        // However, when the instruction is modified in MoveSetInsn2CmpBB,
        // the instruction cannot be deleted because there is a use point in the bb.
        continue;
      }
    } else {
      ASSERT(elseDest != nullptr, "null ptr check");
      if (ifDest == nullptr) {
        if (!bb.GetLiveOut()->TestBit(destReg->GetRegisterNumber()) && insn != toBeRremoved2CmpBB) {
          continue;
        }
        ifDest = dest;
      }
    }

    /* generate cset or csel instruction */
    ASSERT(ifDest != nullptr, "null ptr check");
    if (ifDest->IsIntImmediate() && elseDest->IsIntImmediate()) {
      GenerateInsnForImm(*branchInsn, *ifDest, *elseDest, *destReg, generateInsn);
    } else {
      RegOperand *tReg = GenerateRegAndTempInsn(*ifDest, *destReg, generateInsn);
      RegOperand *eReg = GenerateRegAndTempInsn(*elseDest, *destReg, generateInsn);
      if ((tReg->GetRegisterType() != eReg->GetRegisterType()) ||
          (tReg->GetRegisterType() != destReg->GetRegisterType())) {
        return false;
      }
      GenerateInsnForReg(*branchInsn, *tReg, *eReg, *destReg, generateInsn);
    }
  }

  return true;
}

bool AArch64ICOIfThenElsePattern::CheckHasSameDest(std::vector<Insn*> &lInsn, std::vector<Insn*> &rInsn) const {
  for (size_t i = 0; i < lInsn.size(); ++i) {
    if (Has2SrcOpndSetInsn(*lInsn[i])) {
      bool hasSameDest = false;
      for (size_t j = 0; j < rInsn.size(); ++j) {
        RegOperand *rDestReg = static_cast<RegOperand*>(&rInsn[j]->GetOperand(0));
        RegOperand *lDestReg = static_cast<RegOperand*>(&lInsn[i]->GetOperand(0));
        if (lDestReg->GetRegisterNumber() == rDestReg->GetRegisterNumber() &&
            rInsn[j]->GetOperandSize(0) == lInsn[i]->GetOperandSize(0)) {
          hasSameDest = true;
          break;
        }
      }
      if (!hasSameDest) {
        return false;
      }
    }
  }
  return true;
}

bool AArch64ICOIfThenElsePattern::CheckModifiedRegister(Insn &insn, std::map<Operand*,
    std::vector<Operand*>> &destSrcMap, std::vector<Operand*> &src,
    std::map<Operand*, Insn*> &dest2InsnMap, Insn *&toBeRremovedOutOfCurrBB) const {
  /* src was modified in this blcok earlier */
  for (auto srcOpnd : src) {
    if (srcOpnd->IsRegister()) {
      auto &srcReg = static_cast<const RegOperand&>(*srcOpnd);
      for (const auto &destSrcPair : destSrcMap) {
        ASSERT(destSrcPair.first->IsRegister(), "opnd must be register");
        RegOperand *mapSrcReg = static_cast<RegOperand*>(destSrcPair.first);
        if (mapSrcReg->GetRegisterNumber() == srcReg.GetRegisterNumber()) {
          if (toBeRremovedOutOfCurrBB == nullptr && mapSrcReg->IsVirtualRegister() &&
              !insn.GetBB()->GetLiveOut()->TestBit(srcReg.GetRegisterNumber()) &&
              // If the insn is Has2SrcOpndSetInsn, the insn cannot be moved:
              // =========init=========:
              // cmp R4, R5
              // mov R102, imm:1
              // lsl R0, R102, R100
              // =========after MoveSetInsn2CmpBB=========:
              // mov R1226, imm:1
              // cmp R4, R5
              // mov R102, R1226
              // lsl R0, R102, R100
              // =========after mov Has2SrcOpndSetInsn before cmpInsn=========:
              // mov R1226, imm:1
              // lsl R0, R102, R100 ====> error: use undefined reg R102
              // cmp R4, R5
              // mov R102, R122
              !Has2SrcOpndSetInsn(insn)) {
            ASSERT(dest2InsnMap.find(mapSrcReg) != dest2InsnMap.end(), "must find");
            toBeRremovedOutOfCurrBB = dest2InsnMap[mapSrcReg];
            continue;
          }
          return false;
        }
      }
    }
  }
  auto &dest = insn.GetOperand(0);
  /* dest register was modified earlier in this block */
  ASSERT(dest.IsRegister(), "opnd must be register");
  auto &destReg = static_cast<const RegOperand&>(dest);
  for (const auto &destSrcPair : destSrcMap) {
    ASSERT(destSrcPair.first->IsRegister(), "opnd must be register");
    RegOperand *mapSrcReg = static_cast<RegOperand*>(destSrcPair.first);
    if (mapSrcReg->GetRegisterNumber() == destReg.GetRegisterNumber()) {
      return false;
    }
  }

  /* src register is modified later in this block, will not be processed */
  for (auto srcOpnd : src) {
    if (srcOpnd->IsRegister()) {
      RegOperand &srcReg = static_cast<RegOperand&>(*srcOpnd);
      if (destReg.IsOfFloatOrSIMDClass() && srcReg.GetRegisterNumber() == RZR) {
        return false;
      }
      for (Insn *tmpInsn = &insn; tmpInsn != nullptr; tmpInsn = tmpInsn->GetNext()) {
        Operand *tmpDest = nullptr;
        std::vector<Operand*> tmpSrc;
        if (IsSetInsn(*tmpInsn, &tmpDest, tmpSrc) && tmpDest->Equals(*srcOpnd)) {
          ASSERT(tmpDest->IsRegister(), "opnd must be register");
          RegOperand *tmpDestReg = static_cast<RegOperand*>(tmpDest);
          if (srcReg.GetRegisterNumber() == tmpDestReg->GetRegisterNumber()) {
            return false;
          }
        }
      }
    }
  }

  /* add/sub insn's dest register does not exist in cmp insn. */
  return CheckModifiedInCmpInsn(insn);
}

bool AArch64ICOIfThenElsePattern::CheckCondMoveBB(BB *bb, std::map<Operand*, std::vector<Operand*>> &destSrcMap,
    std::vector<Operand*> &destRegs, std::vector<Insn*> &setInsn, Insn *&toBeRremovedOutOfCurrBB) const {
  std::map<Operand*, Insn*> dest2InsnMap; // CheckModifiedRegister will ensure that dest is defined only once.
  if (bb == nullptr) {
    return true;
  }
  FOR_BB_INSNS(insn, bb) {
    if (!insn->IsMachineInstruction() || insn->IsBranch()) {
      continue;
    }
    Operand *dest = nullptr;
    std::vector<Operand*> src;

    if (!IsSetInsn(*insn, &dest, src)) {
      return false;
    }
    ASSERT(dest != nullptr, "null ptr check");
    ASSERT(src.size() != 0, "null ptr check");

    if (!dest->IsRegister()) {
      return false;
    }

    for (auto srcOpnd : src) {
      if (!(srcOpnd->IsConstImmediate()) && !srcOpnd->IsRegister()) {
        return false;
      }
    }

    if (flagOpnd != nullptr) {
      RegOperand *flagReg = static_cast<RegOperand*>(flagOpnd);
      regno_t flagRegNO = flagReg->GetRegisterNumber();
      if (bb->GetLiveOut()->TestBit(flagRegNO)) {
        return false;
      }
    }

    if (!CheckModifiedRegister(*insn, destSrcMap, src, dest2InsnMap, toBeRremovedOutOfCurrBB)) {
      return false;
    }

    if (insn != toBeRremovedOutOfCurrBB) {
      if (IsExpansionMOperator(*insn)) {
        if (toBeRremovedOutOfCurrBB == nullptr && static_cast<RegOperand*>(dest)->IsVirtualRegister()) {
          toBeRremovedOutOfCurrBB = insn;
        } else {
          return false;
        }
      }
    }

    (void)destSrcMap.insert(std::make_pair(dest, src));
    destRegs.emplace_back(dest);
    (void)setInsn.emplace_back(insn);
    dest2InsnMap[dest] = insn;
  }
  return true;
}

bool AArch64ICOIfThenElsePattern::CheckModifiedInCmpInsn(const Insn &insn, bool movInsnBeforeCmp) const {
  /* insn's dest register does not exist in cmp insn. */
  bool check = movInsnBeforeCmp ? IsSetInsnMOperator(insn) : Has2SrcOpndSetInsn(insn);
  if (check) {
    RegOperand &insnDestReg = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
    if (flagOpnd) {
      RegOperand &cmpReg = static_cast<RegOperand&>(cmpInsn->GetOperand(kInsnFirstOpnd));
      if (insnDestReg.GetRegisterNumber() == cmpReg.GetRegisterNumber()) {
        return false;
      }
    } else {
      RegOperand &cmpReg1 = static_cast<RegOperand&>(cmpInsn->GetOperand(kInsnSecondOpnd));
      if (cmpInsn->GetOperand(kInsnThirdOpnd).IsRegister()) {
        RegOperand &cmpReg2 = static_cast<RegOperand&>(cmpInsn->GetOperand(kInsnThirdOpnd));
        if (insnDestReg.GetRegisterNumber() == cmpReg1.GetRegisterNumber() ||
            insnDestReg.GetRegisterNumber() == cmpReg2.GetRegisterNumber()) {
          return false;
        }
      } else {
        if (insnDestReg.GetRegisterNumber() == cmpReg1.GetRegisterNumber()) {
          return false;
        }
      }
    }
  }
  return true;
}

// If the first insn of if/else bb has the same machine opcode and src opnds, put the insn in cmpBB, do opt like:
// cmp BB:                          cmp BB:
//                                   lsr  w1, w0, #1 (change)
//   cmp  w5, #1                     cmp   w5, #1
//   beq                             beq
// if bb:                          if bb:
//   lsr  w1, w0, #1 (change) =>
//   mov  w3, w1                     mov  w3, w1
// else bb:                        else bb:
//   lsr  w2, w0, #1 (change)
//   mov  w4, w2                     mov  w4, w1 (change)
bool AArch64ICOIfThenElsePattern::DoHostBeforeDoCselOpt(BB &ifBB, BB &elseBB) const {
  auto firstInsnOfIfBB = ifBB.GetFirstMachineInsn();
  auto firstInsnOfElseBB = elseBB.GetFirstMachineInsn();
  if (firstInsnOfIfBB == nullptr || firstInsnOfElseBB == nullptr) {
    return false;
  }
  if (!IsSetInsnMOperator(*firstInsnOfIfBB) ||
      firstInsnOfIfBB->GetMachineOpcode() != firstInsnOfElseBB->GetMachineOpcode() ||
      firstInsnOfIfBB->GetOperandSize() != firstInsnOfElseBB->GetOperandSize()) {
    return false;
  }
  if (!firstInsnOfIfBB->GetOperand(0).IsRegister() || !firstInsnOfElseBB->GetOperand(0).IsRegister()) {
    return false;
  }
  auto *destOpndOfInsnInIfBB = static_cast<RegOperand*>(&firstInsnOfIfBB->GetOperand(0));
  auto *destOpndOfInsnInElseBB = static_cast<RegOperand*>(&firstInsnOfElseBB->GetOperand(0));
  if (!destOpndOfInsnInIfBB->IsVirtualRegister() || !destOpndOfInsnInElseBB->IsVirtualRegister()) {
    return false;
  }
  for (uint32 i = 1; i < firstInsnOfIfBB->GetOperandSize(); ++i) {
    auto *opndOfInsnInIfBB = &firstInsnOfIfBB->GetOperand(i);
    auto *opndOfInsnInElseBB = &firstInsnOfElseBB->GetOperand(i);
    if (opndOfInsnInIfBB->IsRegister() && static_cast<RegOperand*>(opndOfInsnInIfBB)->Equals(*opndOfInsnInElseBB)) {
      continue;
    }
    if (opndOfInsnInIfBB->GetKind() == Operand::kOpdImmediate &&
        static_cast<ImmOperand*>(opndOfInsnInIfBB)->Equals(*opndOfInsnInElseBB)) {
      continue;
    }
    return false;
  }
  if (ifBB.GetLiveOut()->TestBit(destOpndOfInsnInIfBB->GetRegisterNumber()) ||
      ifBB.GetLiveOut()->TestBit(destOpndOfInsnInElseBB->GetRegisterNumber())) {
    return false;
  }
  FOR_BB_INSNS(insn, &elseBB) {
    if (!insn->IsMachineInstruction()) {
      continue;
    }
    for (uint32 i = 0; i < insn->GetOperandSize(); ++i) {
      auto *opnd = &insn->GetOperand(i);
      switch (opnd->GetKind()) {
        case Operand::kOpdRegister: {
          if (destOpndOfInsnInElseBB->Equals(*opnd)) {
            insn->SetOperand(i, *destOpndOfInsnInIfBB);
          }
          break;
        }
        case Operand::kOpdMem: {
          auto *memOpnd = static_cast<MemOperand*>(opnd);
          RegOperand *base = memOpnd->GetBaseRegister();
          RegOperand *index = memOpnd->GetIndexRegister();
          if (base != nullptr && destOpndOfInsnInElseBB->Equals(*base)) {
            memOpnd->SetBaseRegister(*destOpndOfInsnInIfBB);
          }
          if (index != nullptr && destOpndOfInsnInElseBB->Equals(*index)) {
            memOpnd->SetIndexRegister(*destOpndOfInsnInIfBB);
          }
          break;
        }
        case Operand::kOpdList: {
          auto *listOpnd = static_cast<ListOperand*>(opnd);
          auto& opndList = listOpnd->GetOperands();
          std::list<RegOperand*> tempList;
          while (!opndList.empty()) {
            auto* op = opndList.front();
            opndList.pop_front();
            if (destOpndOfInsnInElseBB->Equals(*op)) {
              tempList.push_back(destOpndOfInsnInElseBB);
            } else {
              tempList.push_back(op);
            }
          }
          listOpnd->GetOperands().assign(tempList.begin(), tempList.end());
          break;
        }
        default:
          break;
      }
    }
  }
  ifBB.RemoveInsn(*firstInsnOfIfBB);
  elseBB.RemoveInsn(*firstInsnOfElseBB);
  (void)cmpBB->InsertInsnBefore(*cmpInsn, *firstInsnOfIfBB);
  return true;
}

void AArch64ICOIfThenElsePattern::UpdateTemps(std::vector<Operand*> &destRegs, std::vector<Insn*> &setInsn,
    std::map<Operand*, std::vector<Operand*>> &destSrcMap, const Insn &oldInsn, Insn *newInsn) const {
  for (auto it = setInsn.begin(); it != setInsn.end(); ++it) {
    if (*it == &oldInsn) {
      (void)setInsn.erase(it);
      break;
    }
  }
  if (newInsn != nullptr) {
    setInsn.push_back(newInsn);
    return;
  }
  auto &opnd = oldInsn.GetOperand(0);
  if (!opnd.IsRegister()) {
    return;
  }
  for (auto it = destRegs.begin(); it != destRegs.end(); ++it) {
    if (opnd.Equals(**it)) {
      (void)destRegs.erase(it);
      break;
    }
  }

  for (auto it = destSrcMap.cbegin(); it != destSrcMap.cend(); ++it) {
    if (opnd.Equals(*(it->first))) {
      (void)destSrcMap.erase(it);
      break;
    }
  }
}

void AArch64ICOIfThenElsePattern::RevertMoveInsns(BB *bb, Insn *prevInsnInBB, Insn *newInsnOfBB,
    Insn *insnInBBToBeRremovedOutOfCurrBB) const {
  if (bb == nullptr || insnInBBToBeRremovedOutOfCurrBB == nullptr) {
    return;
  }
  if (newInsnOfBB != nullptr) {
    bb->RemoveInsn(*newInsnOfBB);
  }
  cmpBB->RemoveInsn(*insnInBBToBeRremovedOutOfCurrBB);
  if (prevInsnInBB != nullptr) {
    (void)bb->InsertInsnAfter(*prevInsnInBB, *insnInBBToBeRremovedOutOfCurrBB);
  } else {
    bb->InsertInsnBegin(*insnInBBToBeRremovedOutOfCurrBB);
  }
}

Insn *AArch64ICOIfThenElsePattern::MoveSetInsn2CmpBB(Insn &toBeRremoved2CmpBB, BB &currBB,
    std::map<Operand*, std::vector<Operand*>> &destSrcMap) const {
  // When moving the instruction to the front of cmp, need to create a new register because it may change the semantics:
  // cmpBB:                        cmpBB:
  //                                 uxth  w2, w1 (change)
  //   cmp  w5, #1                   cmp   w5, #1
  //   beq                           beq
  //
  // if bb:                        if bb:
  //   eor  w0, w3, w4     =>        eor   w0, w3, w4
  //
  // else bb:                      else bb:
  //   uxth w0, w1                   mov   w0, w2 (change)
  auto &oldDestReg = static_cast<RegOperand&>(static_cast<RegOperand&>(toBeRremoved2CmpBB.GetOperand(0)));
  ASSERT(oldDestReg.IsVirtualRegister(), "must be vreg");
  auto &newDestReg = cgFunc->CreateVirtualRegisterOperand(
      cgFunc->NewVReg(oldDestReg.GetRegisterType(), oldDestReg.GetSize() / k8BitSize));
  toBeRremoved2CmpBB.SetOperand(0, newDestReg);
  uint32 mOp = (oldDestReg.GetSize() == 64) ? MOP_xmovrr : MOP_wmovrr;
  auto *newInsn = &(cgFunc->GetInsnBuilder()->BuildInsn(mOp, oldDestReg, newDestReg));
  (void)currBB.InsertInsnBefore(toBeRremoved2CmpBB, *newInsn);
  currBB.RemoveInsn(toBeRremoved2CmpBB);
  (void)cmpBB->InsertInsnBefore(*cmpInsn, toBeRremoved2CmpBB);
  destSrcMap[&oldDestReg].clear();
  destSrcMap[&oldDestReg].push_back(&newDestReg);
  return newInsn;
}

/* Convert conditional branches into cset/csel instructions */
bool AArch64ICOIfThenElsePattern::DoOpt(BB *ifBB, BB *elseBB, BB &joinBB) {
  Insn *condBr = cgFunc->GetTheCFG()->FindLastCondBrInsn(*cmpBB);
  ASSERT(condBr != nullptr, "nullptr check");
  cmpInsn = FindLastCmpInsn(*cmpBB);
  flagOpnd = nullptr;
  /* for cbnz and cbz institution */
  if (cgFunc->GetTheCFG()->IsCompareAndBranchInsn(*condBr)) {
    Operand &opnd0 = condBr->GetOperand(0);
    if (opnd0.IsRegister() && static_cast<RegOperand&>(opnd0).GetRegisterNumber() == RZR) {
      return false;
    }
    cmpInsn = condBr;
    flagOpnd = &(opnd0);
  }

  /* tbz will not be optimized */
  MOperator mOperator = condBr->GetMachineOpcode();
  if (mOperator == MOP_xtbz || mOperator == MOP_wtbz || mOperator == MOP_xtbnz || mOperator == MOP_wtbnz) {
    return false;
  }
  if (cmpInsn == nullptr) {
    return false;
  }
  if (ifBB != nullptr && elseBB != nullptr) {
    while (DoHostBeforeDoCselOpt(*ifBB, *elseBB)) {}
  }

  std::vector<Operand*> ifDestRegs;
  std::vector<Insn*> ifSetInsn;
  std::vector<Operand*> elseDestRegs;
  std::vector<Insn*> elseSetInsn;

  std::map<Operand*, std::vector<Operand*>> ifDestSrcMap;
  std::map<Operand*, std::vector<Operand*>> elseDestSrcMap;

  Insn *insnInElseBBToBeRremovedOutOfCurrBB = nullptr;
  Insn *insnInIfBBToBeRremovedOutOfCurrBB = nullptr;

  if (!CheckCondMoveBB(elseBB, elseDestSrcMap, elseDestRegs, elseSetInsn, insnInElseBBToBeRremovedOutOfCurrBB) ||
      (ifBB != nullptr && !CheckCondMoveBB(ifBB, ifDestSrcMap, ifDestRegs, ifSetInsn,
                                           insnInIfBBToBeRremovedOutOfCurrBB))) {
    return false;
  }
  if (insnInElseBBToBeRremovedOutOfCurrBB != nullptr &&
      !CheckModifiedInCmpInsn(*insnInElseBBToBeRremovedOutOfCurrBB, true)) {
    return false;
  }
  if (insnInIfBBToBeRremovedOutOfCurrBB != nullptr &&
      !CheckModifiedInCmpInsn(*insnInIfBBToBeRremovedOutOfCurrBB, true)) {
    return false;
  }
  // If ifBB and elseBB are the same bb, there is no need to move statement forward.
  if (ifBB == elseBB &&
      (insnInElseBBToBeRremovedOutOfCurrBB != nullptr || insnInIfBBToBeRremovedOutOfCurrBB != nullptr)) {
    return false;
  }
  if (!CheckHasSameDest(ifSetInsn, elseSetInsn) || !CheckHasSameDest(elseSetInsn, ifSetInsn) ||
      !CheckHasSameDestSize(ifSetInsn, elseSetInsn)) {
    return false;
  }

  size_t count = elseDestRegs.size();

  for (size_t i = 0; i < ifDestRegs.size(); ++i) {
    bool foundInElse = false;
    for (size_t j = 0; j < elseDestRegs.size(); ++j) {
      RegOperand *elseDestReg = static_cast<RegOperand*>(elseDestRegs[j]);
      RegOperand *ifDestReg = static_cast<RegOperand*>(ifDestRegs[i]);
      if (ifDestReg->GetRegisterNumber() == elseDestReg->GetRegisterNumber()) {
        if (Has2SrcOpndSetInsn(*ifSetInsn[i]) && Has2SrcOpndSetInsn(*elseSetInsn[j])) {
          return false;
        }
        foundInElse = true;
        break;
      }
    }
    if (foundInElse) {
      continue;
    } else {
      ++count;
    }
  }
  if (count > kThreshold) {
    return false;
  }
  Insn *newInsnOfIfBB = nullptr;
  Insn *newInsnOfElseBB = nullptr;
  Insn *prevInsnInIfBB = nullptr;
  Insn *prevInsnInElseBB = nullptr;
  if (insnInElseBBToBeRremovedOutOfCurrBB != nullptr) {
    prevInsnInElseBB = insnInElseBBToBeRremovedOutOfCurrBB->GetPrev();
    ASSERT_NOT_NULL(elseBB);
    newInsnOfElseBB = MoveSetInsn2CmpBB(*insnInElseBBToBeRremovedOutOfCurrBB, *elseBB, elseDestSrcMap);
    UpdateTemps(elseDestRegs, elseSetInsn, elseDestSrcMap, *insnInElseBBToBeRremovedOutOfCurrBB, newInsnOfElseBB);
  }
  if (insnInIfBBToBeRremovedOutOfCurrBB != nullptr) {
    prevInsnInIfBB = insnInIfBBToBeRremovedOutOfCurrBB->GetPrev();
    ASSERT_NOT_NULL(ifBB);
    newInsnOfIfBB = MoveSetInsn2CmpBB(*insnInIfBBToBeRremovedOutOfCurrBB, *ifBB, ifDestSrcMap);
    UpdateTemps(ifDestRegs, ifSetInsn, ifDestSrcMap, *insnInIfBBToBeRremovedOutOfCurrBB, newInsnOfIfBB);
  }

  /* generate insns */
  std::vector<Insn*> elseGenerateInsn;
  std::vector<Insn*> ifGenerateInsn;
  bool elseBBProcessResult = false;
  DestSrcMap destSrcTempMap(ifDestSrcMap, elseDestSrcMap);
  if (elseBB != nullptr) {
    elseBBProcessResult = BuildCondMovInsn(*elseBB, destSrcTempMap, false, elseGenerateInsn, newInsnOfElseBB);
  }
  bool ifBBProcessResult = false;
  if (ifBB != nullptr) {
    ifBBProcessResult = BuildCondMovInsn(*ifBB, destSrcTempMap, true, ifGenerateInsn, newInsnOfIfBB);
  }
  if ((elseBB != nullptr && !elseBBProcessResult) || (ifBB != nullptr && !ifBBProcessResult)) {
    RevertMoveInsns(elseBB, prevInsnInElseBB, newInsnOfElseBB, insnInElseBBToBeRremovedOutOfCurrBB);
    RevertMoveInsns(ifBB, prevInsnInIfBB, newInsnOfIfBB, insnInIfBBToBeRremovedOutOfCurrBB);
    return false;
  }

  /* insert insn */
  if (cgFunc->GetTheCFG()->IsCompareAndBranchInsn(*condBr)) {
    Insn *innerCmpInsn = BuildCmpInsn(*condBr);
    cmpBB->InsertInsnBefore(*cmpInsn, *innerCmpInsn);
    cmpInsn = innerCmpInsn;
  }

  if (elseBB != nullptr) {
    cmpBB->SetKind(elseBB->GetKind());
  } else {
    ASSERT(ifBB != nullptr, "ifBB should not be nullptr");
    cmpBB->SetKind(ifBB->GetKind());
  }

  for (auto setInsn : ifSetInsn) {
    if (Has2SrcOpndSetInsn(*setInsn)) {
      (void)cmpBB->InsertInsnBefore(*cmpInsn, *setInsn);
    }
  }

  for (auto setInsn : elseSetInsn) {
    if (Has2SrcOpndSetInsn(*setInsn)) {
      (void)cmpBB->InsertInsnBefore(*cmpInsn, *setInsn);
    }
  }

  /* delete condBr */
  cmpBB->RemoveInsn(*condBr);
  /* Insert goto insn after csel insn. */
  if (cmpBB->GetKind() == BB::kBBGoto || cmpBB->GetKind() == BB::kBBIf) {
    if (elseBB != nullptr) {
      (void)cmpBB->InsertInsnAfter(*cmpBB->GetLastInsn(), *elseBB->GetLastInsn());
    } else {
      ASSERT(ifBB != nullptr, "ifBB should not be nullptr");
      (void)cmpBB->InsertInsnAfter(*cmpBB->GetLastInsn(), *ifBB->GetLastInsn());
    }
  }

  /* Insert instructions in branches after cmpInsn */
  for (auto itr = elseGenerateInsn.crbegin(); itr != elseGenerateInsn.crend(); ++itr) {
    (void)cmpBB->InsertInsnAfter(*cmpInsn, **itr);
  }
  for (auto itr = ifGenerateInsn.crbegin(); itr != ifGenerateInsn.crend(); ++itr) {
    (void)cmpBB->InsertInsnAfter(*cmpInsn, **itr);
  }

  /* Remove branches and merge join */
  if (ifBB != nullptr) {
    BB *prevLast = ifBB->GetPrev();
    cgFunc->GetTheCFG()->RemoveBB(*ifBB);
    if (ifBB->GetId() == cgFunc->GetLastBB()->GetId()) {
      cgFunc->SetLastBB(*prevLast);
    }
  }
  if (elseBB != nullptr) {
    BB *prevLast = elseBB->GetPrev();
    cgFunc->GetTheCFG()->RemoveBB(*elseBB);
    if (elseBB->GetId() == cgFunc->GetLastBB()->GetId()) {
      cgFunc->SetLastBB(*prevLast);
    }
  }
  /* maintain won't exit bb info. */
  if ((ifBB != nullptr && ifBB->IsWontExit()) || (elseBB != nullptr && elseBB->IsWontExit())) {
    cgFunc->GetCommonExitBB()->PushBackPreds(*cmpBB);
  }

  if (cmpBB->GetKind() != BB::kBBIf && cmpBB->GetNext() == &joinBB &&
      !maplebe::CGCFG::InLSDA(joinBB.GetLabIdx(), cgFunc->GetEHFunc()) &&
      cgFunc->GetTheCFG()->CanMerge(*cmpBB, joinBB)) {
    maplebe::CGCFG::MergeBB(*cmpBB, joinBB, *cgFunc);
    keepPosition = true;
  }
  return true;
}

/*
 * Find IF-THEN-ELSE or IF-THEN basic block pattern,
 * and then invoke DoOpt(...) to finish optimize.
 */
bool AArch64ICOIfThenElsePattern::Optimize(BB &curBB) {
  if (curBB.GetKind() != BB::kBBIf) {
    return false;
  }
  BB *ifBB = nullptr;
  BB *elseBB = nullptr;
  BB *joinBB = nullptr;

  BB *thenDest = CGCFG::GetTargetSuc(curBB);
  BB *elseDest = curBB.GetNext();
  CHECK_FATAL(thenDest != nullptr, "then_dest is null in ITEPattern::Optimize");
  CHECK_FATAL(elseDest != nullptr, "else_dest is null in ITEPattern::Optimize");
  /* IF-THEN-ELSE */
  if (thenDest->NumPreds() == 1 && thenDest->NumSuccs() == 1 && elseDest->NumSuccs() == 1 &&
      elseDest->NumPreds() == 1 && thenDest->GetSuccs().front() == elseDest->GetSuccs().front()) {
    ifBB = thenDest;
    elseBB = elseDest;
    joinBB = thenDest->GetSuccs().front();
  } else if (elseDest->NumPreds() == 1 && elseDest->NumSuccs() == 1 && elseDest->GetSuccs().front() == thenDest) {
    /* IF-THEN */
    ifBB = nullptr;
    elseBB = elseDest;
    joinBB = thenDest;
  } else if (thenDest->NumPreds() == 1 && thenDest->NumSuccs() == 1 && thenDest->GetSuccs().front() == elseDest) {
    ifBB = thenDest;
    elseBB = nullptr;
    joinBB = elseDest;
  } else {
    /* not a form we can handle */
    return false;
  }
  if (elseBB != nullptr &&
      (CGCFG::InLSDA(elseBB->GetLabIdx(), cgFunc->GetEHFunc()) ||
       CGCFG::InSwitchTable(elseBB->GetLabIdx(), *cgFunc) ||
       !elseBB->HasMachineInsn())) {
    return false;
  }

  if (ifBB != nullptr &&
      (CGCFG::InLSDA(ifBB->GetLabIdx(), cgFunc->GetEHFunc()) ||
       CGCFG::InSwitchTable(ifBB->GetLabIdx(), *cgFunc))) {
    return false;
  }
  cmpBB = &curBB;
  return DoOpt(ifBB, elseBB, *joinBB);
}

/* If( cmp || cmp ) then
 * or
 * If( cmp && cmp ) then */
bool AArch64ICOSameCondPattern::Optimize(BB &secondIfBB) {
  if (secondIfBB.GetKind() != BB::kBBIf || secondIfBB.NumPreds() != 1) {
    return false;
  }
  if (secondIfBB.GetPreds().size() != 1) {
    return false;
  }
  BB *firstIfBB = *secondIfBB.GetPredsBegin();
  /* firstIfBB's nextBB is secondIfBB */
  if (firstIfBB->GetKind() != BB::kBBIf) {
    return false;
  }
  if (firstIfBB->GetNext() == nullptr || secondIfBB.GetNext() == nullptr) {
    return false;
  }
  if (firstIfBB->GetSuccs().size() != kOperandNumBinary || secondIfBB.GetSuccs().size() != kOperandNumBinary) {
    return false;
  }
  auto *firstIfSucc0 = firstIfBB->GetSuccs().front();
  auto *firstIfSucc1 = firstIfBB->GetSuccs().back();
  if (firstIfBB->GetNext()->GetLabIdx() != firstIfSucc0->GetLabIdx() &&
      firstIfBB->GetNext()->GetLabIdx() != firstIfSucc1->GetLabIdx()) {
    return false;
  }
  auto *secondIfSucc0 = secondIfBB.GetSuccs().front();
  auto *secondIfSucc1 = secondIfBB.GetSuccs().back();
  if (secondIfBB.GetNext()->GetLabIdx() != secondIfSucc0->GetLabIdx() &&
      secondIfBB.GetNext()->GetLabIdx() != secondIfSucc1->GetLabIdx()) {
    return false;
  }
  if (secondIfBB.GetLabIdx() != firstIfSucc0->GetLabIdx() &&
      secondIfBB.GetLabIdx() != firstIfSucc1->GetLabIdx()) {
    return false;
  }
  return DoOpt(*firstIfBB, secondIfBB);
}

bool AArch64ICOPattern::IsReverseMop(MOperator mOperator1, MOperator mOperator2) const {
  return (mOperator1 == MOP_beq && mOperator2 == MOP_bne) ||
      (mOperator1 == MOP_blt && mOperator2 == MOP_bge) ||
      (mOperator1 == MOP_ble && mOperator2 == MOP_bgt) ||
      (mOperator1 == MOP_blo && mOperator2 == MOP_bhs) ||
      (mOperator1 == MOP_bhi && mOperator2 == MOP_bls) ||
      (mOperator1 == MOP_bpl && mOperator2 == MOP_bmi) ||
      (mOperator1 == MOP_bvc && mOperator2 == MOP_bvs) ||
      (mOperator1 == MOP_bne && mOperator2 == MOP_beq) ||
      (mOperator1 == MOP_bge && mOperator2 == MOP_blt) ||
      (mOperator1 == MOP_bgt && mOperator2 == MOP_ble) ||
      (mOperator1 == MOP_bhs && mOperator2 == MOP_blo) ||
      (mOperator1 == MOP_bls && mOperator2 == MOP_bhi) ||
      (mOperator1 == MOP_bmi && mOperator2 == MOP_bpl) ||
      (mOperator1 == MOP_bvs && mOperator2 == MOP_bvc);
}

bool AArch64ICOPattern::CheckMop(MOperator mOperator) const {
  switch (mOperator) {
    case MOP_beq:
    case MOP_bne:
    case MOP_blt:
    case MOP_ble:
    case MOP_bgt:
    case MOP_bge:
    case MOP_blo:
    case MOP_bls:
    case MOP_bhs:
    case MOP_bhi:
    case MOP_bpl:
    case MOP_bmi:
    case MOP_bvc:
    case MOP_bvs:
      return true;
    default:
      return false;
  }
}

bool AArch64ICOPattern::CheckMopOfCmp(MOperator mOperator) const {
  switch (mOperator) {
    case MOP_wcmpri:
    case MOP_wcmprr:
    case MOP_xcmpri:
    case MOP_xcmprr:
      return true;
    default:
      return false;
  }
}

/* branchInsn1 is firstIfBB's LastCondBrInsn
 * branchInsn2 is secondIfBB's LastCondBrInsn
 *
 * Limitations: branchInsn1 is the same as branchInsn2
 * */
bool AArch64ICOSameCondPattern::DoOpt(BB &firstIfBB, BB &secondIfBB) const {
  Insn *branchInsn1 = cgFunc->GetTheCFG()->FindLastCondBrInsn(firstIfBB);
  ASSERT(branchInsn1 != nullptr, "nullptr check");
  Insn *cmpInsn1 = FindLastCmpInsn(firstIfBB);
  MOperator mOperator1 = branchInsn1->GetMachineOpcode();
  Insn *branchInsn2 = cgFunc->GetTheCFG()->FindLastCondBrInsn(secondIfBB);
  ASSERT(branchInsn2 != nullptr, "nullptr check");
  Insn *cmpInsn2 = FindLastCmpInsn(secondIfBB);
  MOperator mOperator2 = branchInsn2->GetMachineOpcode();
  if (cmpInsn1 == nullptr || cmpInsn2 == nullptr) {
    return false;
  }
  if (!CheckMopOfCmp(cmpInsn1->GetMachineOpcode()) || !CheckMopOfCmp(cmpInsn2->GetMachineOpcode())) {
    return false;
  }
  /* tbz and cbz will not be optimizsed */
  if (!CheckMop(mOperator1) || !CheckMop(mOperator2)) {
    return false;
  }
  /* two BB has same branch */
  std::vector<LabelOperand*> labelOpnd1 = GetLabelOpnds(*branchInsn1);
  std::vector<LabelOperand*> labelOpnd2 = GetLabelOpnds(*branchInsn2);

  if (labelOpnd1.size() != 1 || labelOpnd1.size() != 1) {
    return false;
  }
  auto fallthruBBOfFirstIf = firstIfBB.GetNext();
  bool fallthruIsSendIfBB = (fallthruBBOfFirstIf->GetLabIdx() == secondIfBB.GetLabIdx());
  // Determine if two if bbs have the same successor.
  if (fallthruIsSendIfBB) {
    if (labelOpnd1[0]->GetLabelIndex() != labelOpnd2[0]->GetLabelIndex() &&
        labelOpnd1[0]->GetLabelIndex() != secondIfBB.GetNext()->GetLabIdx()) {
      return false;
    }
  } else {
    if (fallthruBBOfFirstIf->GetLabIdx() != labelOpnd2[0]->GetLabelIndex()) {
      return false;
    }
  }
  /* secondifBB only has branchInsn and cmpInsn */
  FOR_BB_INSNS_REV(insn, &secondIfBB) {
    if (!insn->IsMachineInstruction()) {
      continue;
    }
    if (insn != branchInsn2 && insn != cmpInsn2) {
      return false;
    }
  }
  ConditionCode ccCode = Encode(branchInsn1->GetMachineOpcode(), fallthruIsSendIfBB);
  bool inverseCCCode2 = false;
  // Determine the value of flag NZCV.
  if (fallthruIsSendIfBB) {
    if (labelOpnd2[0]->GetLabelIndex() == labelOpnd1[0]->GetLabelIndex()) {
      inverseCCCode2 = true;
    }
  } else {
    if (labelOpnd2[0]->GetLabelIndex() == fallthruBBOfFirstIf->GetLabIdx()) {
      inverseCCCode2 = true;
    }
  }
  ConditionCode ccCode2 = Encode(branchInsn2->GetMachineOpcode(), inverseCCCode2);
  ASSERT(ccCode != kCcLast, "unknown cond, ccCode can't be kCcLast");
  Insn *movInsn = nullptr;
  /* build ccmp Insn */
  Insn *ccmpInsn = BuildCcmpInsn(ccCode, ccCode2, *cmpInsn2, movInsn);
  if (ccmpInsn == nullptr) {
    return false;
  }
  auto *nextBB = secondIfBB.GetNext();
  /* insert ccmp Insn */
  firstIfBB.InsertInsnBefore(*branchInsn1, *ccmpInsn);
  if (movInsn != nullptr) {
    firstIfBB.InsertInsnBefore(*cmpInsn1, *movInsn);
  }
  if (fallthruIsSendIfBB) {
    firstIfBB.ReplaceInsn(*branchInsn1, *branchInsn2);
  } else {
    LabelOperand &targetOpnd = cgFunc->GetOrCreateLabelOperand(*nextBB);
    auto &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(AArch64isa::FlipConditionOp(branchInsn2->GetMachineOpcode()),
        branchInsn2->GetOperand(0), targetOpnd);
    firstIfBB.ReplaceInsn(*branchInsn1, newInsn);
  }
  auto secondIfLabel =  secondIfBB.GetLabIdx();
  auto &label2BBMap = cgFunc->GetLab2BBMap();
  if (secondIfLabel != MIRLabelTable::GetDummyLabel()) {
    label2BBMap.erase(secondIfLabel);
  }
  cgFunc->GetTheCFG()->RemoveBB(secondIfBB);
  if (nextBB->GetLabIdx() != labelOpnd1[0]->GetLabelIndex()) {
    label2BBMap.emplace(nextBB->GetLabIdx(), nextBB);
    firstIfBB.PushFrontSuccs(*nextBB);
    nextBB->PushFrontPreds(firstIfBB);
  }
  return true;
}
/*
 * find the preds all is ifBB
 */
bool AArch64ICOMorePredsPattern::Optimize(BB &curBB) {
  if (curBB.GetKind() != BB::kBBGoto) {
    return false;
  }
  for (BB *preBB : curBB.GetPreds()) {
    if (preBB->GetKind() != BB::kBBIf) {
      return false;
    }
  }
  for (BB *succsBB : curBB.GetSuccs()) {
    if (succsBB->GetKind() != BB::kBBFallthru) {
      return false;
    }
    if (succsBB->NumPreds() > 2) {
      return false;
    }
  }
  Insn *gotoBr = curBB.GetLastMachineInsn();
  ASSERT(gotoBr != nullptr, "gotoBr should not be nullptr");
  auto &gotoLabel = static_cast<LabelOperand&>(gotoBr->GetOperand(gotoBr->GetOperandSize() - 1));
  for (BB *preBB : curBB.GetPreds()) {
    Insn *condBr = cgFunc->GetTheCFG()->FindLastCondBrInsn(*preBB);
    ASSERT(condBr != nullptr, "nullptr check");
    Operand &condBrLastOpnd = condBr->GetOperand(condBr->GetOperandSize() - 1);
    ASSERT(condBrLastOpnd.IsLabelOpnd(), "label Operand must be exist in branch insn");
    auto &labelOpnd = static_cast<LabelOperand&>(condBrLastOpnd);
    if (labelOpnd.GetLabelIndex() != curBB.GetLabIdx()) {
      return false;
    }
    if (gotoLabel.GetLabelIndex() != preBB->GetNext()->GetLabIdx()) {
      /* do not if convert if 'else' clause present */
      return false;
    }
  }
  return DoOpt(curBB);
}

/* this BBGoto only has mov Insn and Branch */
bool AArch64ICOMorePredsPattern::CheckGotoBB(BB &gotoBB, std::vector<Insn*> &movInsn) const {
  FOR_BB_INSNS(insn, &gotoBB) {
    if (!insn->IsMachineInstruction()) {
      continue;
    }
    if (insn->IsMove()) {
      movInsn.push_back(insn);
      continue;
    }
    if (insn->GetId() != gotoBB.GetLastInsn()->GetId()) {
      return false;
    } else if (!insn->IsBranch()) { /* last Insn is Branch */
      return false;
    }
  }
  return true;
}

/* this BBGoto only has mov Insn */
bool AArch64ICOMorePredsPattern::MovToCsel(std::vector<Insn*> &movInsn, std::vector<Insn*> &cselInsn,
                                           const Insn &branchInsn) const {
  Operand &branchOpnd0 = branchInsn.GetOperand(kInsnFirstOpnd);
  regno_t branchRegNo;
  if (branchOpnd0.IsRegister()) {
    branchRegNo = static_cast<RegOperand&>(branchOpnd0).GetRegisterNumber();
  }
  for (Insn *insn:movInsn) {
    /* use mov build csel */
    Operand &opnd0 = insn->GetOperand(kInsnFirstOpnd);
    Operand &opnd1 = insn->GetOperand(kInsnSecondOpnd);
    ConditionCode ccCode = AArch64ICOPattern::Encode(branchInsn.GetMachineOpcode(), false);
    ASSERT(ccCode != kCcLast, "unknown cond, ccCode can't be kCcLast");
    CondOperand &cond = static_cast<AArch64CGFunc *>(cgFunc)->GetCondOperand(ccCode);
    Operand &rflag = static_cast<AArch64CGFunc *>(cgFunc)->GetOrCreateRflag();
    RegOperand &regOpnd0 = static_cast<RegOperand&>(opnd0);
    RegOperand &regOpnd1 = static_cast<RegOperand&>(opnd1);
    /* movInsn's opnd1 is Immediate */
    if (opnd1.IsImmediate()) {
      return false;
    }
    /* opnd0 and opnd1 hsa same type and size */
    if (regOpnd0.GetSize() != regOpnd1.GetSize() || (regOpnd0.IsOfIntClass() != regOpnd1.IsOfIntClass())) {
      return false;
    }
    /* The branchOpnd0 cannot be modified for csel. */
    regno_t movRegNo0 = static_cast<RegOperand&>(opnd0).GetRegisterNumber();
    if (branchOpnd0.IsRegister() && branchRegNo == movRegNo0) {
      return false;
    }
    uint32 dSize = regOpnd0.GetSize();
    bool isIntTy = regOpnd0.IsOfIntClass();
    MOperator mOpCode = isIntTy ? (dSize == k64BitSize ? MOP_xcselrrrc : MOP_wcselrrrc)
                                : (dSize == k64BitSize ? MOP_dcselrrrc : (dSize == k32BitSize ?
                                                                          MOP_scselrrrc : MOP_hcselrrrc));
    (void)cselInsn.emplace_back(&cgFunc->GetInsnBuilder()->BuildInsn(mOpCode, opnd0, opnd1, opnd0, cond, rflag));
  }
  if (cselInsn.size() < 1) {
    return false;
  }
  return true;
}

bool AArch64ICOMorePredsPattern::DoOpt(BB &gotoBB) const {
  std::vector<Insn*> movInsn;
  std::vector<std::vector<Insn*>> presCselInsn;
  std::vector<BB*> presBB;
  Insn *branchInsn = gotoBB.GetLastMachineInsn();
  if (branchInsn == nullptr || !branchInsn->IsUnCondBranch()) {
    return false;
  }
  /* get preds's new label */
  std::vector<LabelOperand*> labelOpnd = GetLabelOpnds(*branchInsn);
  if (labelOpnd.size() != 1) {
    return false;
  }
  if (!CheckGotoBB(gotoBB, movInsn)) {
    return false;
  }
  /* Check all preBB, Exclude gotoBBs that cannot be optimized. */
  for (BB *preBB : gotoBB.GetPreds()) {
    Insn *condBr = cgFunc->GetTheCFG()->FindLastCondBrInsn(*preBB);
    ASSERT(condBr != nullptr, "nullptr check");

    /* tbz/cbz will not be optimized */
    MOperator mOperator = condBr->GetMachineOpcode();
    if (!CheckMop(mOperator)) {
      return false;
    }
    std::vector<Insn*> cselInsn;
    if (!MovToCsel(movInsn, cselInsn, *condBr)) {
      return false;
    }
    if (cselInsn.size() < 1) {
      return false;
    }
    presCselInsn.emplace_back(cselInsn);
    presBB.emplace_back(preBB);
  }
  /* modifies presBB */
  for (size_t i = 0; i < presCselInsn.size(); ++i) {
    BB *preBB = presBB[i];
    Insn *condBr = cgFunc->GetTheCFG()->FindLastCondBrInsn(*preBB);
    std::vector<Insn*> cselInsn = presCselInsn[i];
    /* insert csel insn */
    for (Insn *csel : cselInsn) {
      preBB->InsertInsnBefore(*condBr, *csel);
    }
    /* new condBr */
    condBr->SetOperand(condBr->GetOperandSize() - 1, *labelOpnd[0]);
  }
  /* Remove branches and merge gotoBB */
  cgFunc->GetTheCFG()->RemoveBB(gotoBB);
  return true;
}

}  /* namespace maplebe */
