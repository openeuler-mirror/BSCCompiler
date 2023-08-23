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
#include "aarch64_strldr.h"
#include "aarch64_reaching.h"
#include "aarch64_cgfunc.h"
#include "common_utils.h"

namespace maplebe {
using namespace maple;

static MOperator SelectMovMop(bool isFloatOrSIMD, bool is64Bit) {
  return isFloatOrSIMD ? (is64Bit ? MOP_xvmovd : MOP_xvmovs)
                       : (is64Bit ? MOP_xmovrr : MOP_wmovrr);
}

void AArch64StoreLoadOpt::Run() {
  DoStoreLoadOpt();
}

/*
 * Transfer: store x100, [MEM]
 *           ... // May exist branches.
 *           load  x200, [MEM]
 *       ==>
 *        OPT_VERSION_STR_LIVE:
 *           store x100, [MEM]
 *           ... // May exist branches. if x100 not dead here.
 *           mov   x200, x100
 *       OPT_VERSION_STR_DIE:
 *           store x100, [MEM]
 *           mov x9000(new reg), x100
 *           ... // May exist branches. if x100 dead here.
 *           mov   x200, x9000
 *  Params:
 *    strInsn: indicate store insn.
 *    strSrcIdx: index of source register operand of store insn. (x100 in this example)
 *    memSeq: represent first memOpreand or second memOperand
 *    memUseInsnSet: insns using memOperand
 */
void AArch64StoreLoadOpt::DoLoadToMoveTransfer(Insn &strInsn, short strSrcIdx,
                                               short memSeq, const InsnSet &memUseInsnSet) {
  /* stp instruction need two registers, str only need one register */
  ASSERT(strSrcIdx < kDivide2, "CG internal error.");
  /* Find x100's definition insn. */
  InsnSet regDefInsnSet = cgFunc.GetRD()->FindDefForRegOpnd(strInsn, strSrcIdx);
  ASSERT(!regDefInsnSet.empty(), "RegOperand is used before defined");
  if (regDefInsnSet.size() != 1) {
    return;
  }
  std::map<Insn*, bool> insnState;
  for (auto *ldrInsn : memUseInsnSet) {
    insnState[ldrInsn] = true;
  }
  for (auto *ldrInsn : memUseInsnSet) {
    if (!ldrInsn->IsLoad() || (ldrInsn->GetDefRegs().size() > 1) || ldrInsn->GetBB()->IsCleanup()) {
      continue;
    }

    if (HasMemBarrier(*ldrInsn, strInsn)) {
      continue;
    }

    /* ldr x200, [mem], mem index is 1, x200 index is 0 */
    InsnSet memDefInsnSet = cgFunc.GetRD()->FindDefForMemOpnd(*ldrInsn, kInsnSecondOpnd);
    ASSERT(!memDefInsnSet.empty(), "load insn should have definitions.");
    /* If load has multiple definition, continue. */
    if (memDefInsnSet.size() > 1) {
      insnState[ldrInsn] = false;
      continue;
    }

    Operand &resOpnd = ldrInsn->GetOperand(kInsnFirstOpnd);
    Operand &srcOpnd = strInsn.GetOperand(static_cast<uint32>(strSrcIdx));
    if (resOpnd.GetSize() != srcOpnd.GetSize()) {
      return;
    }

    auto &resRegOpnd = static_cast<RegOperand&>(resOpnd);
    auto &srcRegOpnd = static_cast<RegOperand&>(srcOpnd);
    if (resRegOpnd.GetRegisterType() != srcRegOpnd.GetRegisterType()) {
      continue;
    }

    /* Check if use operand of store is live at load insn. */
    if (cgFunc.GetRD()->RegIsLiveBetweenInsn(srcRegOpnd.GetRegisterNumber(), strInsn, *ldrInsn)) {
      GenerateMoveLiveInsn(resRegOpnd, srcRegOpnd, *ldrInsn, strInsn, memSeq);
      insnState[ldrInsn] = false;
    } else if (!cgFunc.IsAfterRegAlloc()) {
      GenerateMoveDeadInsn(resRegOpnd, srcRegOpnd, *ldrInsn, strInsn, memSeq);
    }

    if (CG_DEBUG_FUNC(cgFunc)) {
      LogInfo::MapleLogger() << "Do store-load optimization 1: str version";
      LogInfo::MapleLogger() << cgFunc.GetName() << '\n';
      LogInfo::MapleLogger() << "Store insn: ";
      strInsn.Dump();
      LogInfo::MapleLogger() << "Load insn: ";
      ldrInsn->Dump();
    }
  }
  auto it = memUseInsnSet.begin();
  ++it;
  for (; it != memUseInsnSet.end(); ++it) {
    Insn *curInsn = *it;
    if (!insnState[curInsn]) {
      continue;
    }
    if (!curInsn->IsLoad() || (curInsn->GetDefRegs().size() > 1) || curInsn->GetBB()->IsCleanup()) {
      continue;
    }
    InsnSet memDefInsnSet = cgFunc.GetRD()->FindDefForMemOpnd(*curInsn, kInsnSecondOpnd);
    ASSERT(!memDefInsnSet.empty(), "load insn should have definitions.");
    if (memDefInsnSet.size() > 1) {
      continue;
    }
    auto prevIt = it;
    do {
      --prevIt;
      Insn *prevInsn = *prevIt;
      if (!insnState[prevInsn]) {
        continue;
      }
      if (prevInsn->GetBB() != curInsn->GetBB()) {
        break;
      }
      if (!prevInsn->IsLoad() || (prevInsn->GetDefRegs().size() > 1) || prevInsn->GetBB()->IsCleanup()) {
        continue;
      }
      InsnSet memoryDefInsnSet = cgFunc.GetRD()->FindDefForMemOpnd(*curInsn, kInsnSecondOpnd);
      ASSERT(!memoryDefInsnSet.empty(), "load insn should have definitions.");
      if (memoryDefInsnSet.size() > 1) {
        break;
      }
      Operand &resOpnd = curInsn->GetOperand(kInsnFirstOpnd);
      Operand &srcOpnd = prevInsn->GetOperand(kInsnFirstOpnd);
      if (resOpnd.GetSize() != srcOpnd.GetSize()) {
        continue;
      }

      auto &resRegOpnd = static_cast<RegOperand&>(resOpnd);
      auto &srcRegOpnd = static_cast<RegOperand&>(srcOpnd);
      if (resRegOpnd.GetRegisterType() != srcRegOpnd.GetRegisterType()) {
        continue;
      }
      /* Check if use operand of store is live at load insn. */
      if (cgFunc.GetRD()->FindRegDefBetweenInsn(srcRegOpnd.GetRegisterNumber(),
          prevInsn->GetNext(), curInsn->GetPrev()).empty()) {
        GenerateMoveLiveInsn(resRegOpnd, srcRegOpnd, *curInsn, *prevInsn, memSeq);
        insnState[curInsn] = false;
      }
      break;
    } while (prevIt != memUseInsnSet.begin());
  }
}

void AArch64StoreLoadOpt::GenerateMoveLiveInsn(RegOperand &resRegOpnd, RegOperand &srcRegOpnd,
                                               Insn &ldrInsn, Insn &strInsn, short memSeq) {
  MOperator movMop = SelectMovMop(resRegOpnd.IsOfFloatOrSIMDClass(), resRegOpnd.GetSize() == k64BitSize);
  Insn *movInsn = nullptr;
  if (str2MovMap[&strInsn][memSeq] != nullptr && !cgFunc.IsAfterRegAlloc()) {
    Insn *movInsnOfStr = str2MovMap[&strInsn][memSeq];
    auto &vregOpnd = static_cast<RegOperand&>(movInsnOfStr->GetOperand(kInsnFirstOpnd));
    movInsn = &cgFunc.GetInsnBuilder()->BuildInsn(movMop, resRegOpnd, vregOpnd);
  } else {
    movInsn = &cgFunc.GetInsnBuilder()->BuildInsn(movMop, resRegOpnd, srcRegOpnd);
  }
  if (resRegOpnd.GetRegisterNumber() == srcRegOpnd.GetRegisterNumber() &&
      cgFunc.IsAfterRegAlloc()) {
    ldrInsn.GetBB()->RemoveInsn(ldrInsn);
    cgFunc.GetRD()->InitGenUse(*ldrInsn.GetBB(), false);
    return;
  }
  movInsn->SetId(ldrInsn.GetId());
  ldrInsn.GetBB()->ReplaceInsn(ldrInsn, *movInsn);
  if (CG_DEBUG_FUNC(cgFunc)) {
    LogInfo::MapleLogger() << "replace ldrInsn:\n";
    ldrInsn.Dump();
    LogInfo::MapleLogger() << "with movInsn:\n";
    movInsn->Dump();
  }
  /* Add comment. */
  MapleString newComment = ldrInsn.GetComment();
  if (strInsn.IsStorePair()) {
    newComment += ";  stp-load live version.";
  } else {
    newComment += ";  str-load live version.";
  }
  movInsn->SetComment(newComment);
  cgFunc.GetRD()->InitGenUse(*ldrInsn.GetBB(), false);
}

void AArch64StoreLoadOpt::GenerateMoveDeadInsn(RegOperand &resRegOpnd, RegOperand &srcRegOpnd,
                                               Insn &ldrInsn, Insn &strInsn, short memSeq) {
  Insn *newMovInsn = nullptr;
  RegOperand *vregOpnd = nullptr;

  if (str2MovMap[&strInsn][memSeq] == nullptr) {
    RegType regTy = srcRegOpnd.IsOfFloatOrSIMDClass() ? kRegTyFloat : kRegTyInt;
    regno_t vRegNO =
        cgFunc.NewVReg(regTy, srcRegOpnd.GetSize() <= k32BitSize ? k4ByteSize : k8ByteSize);
    /* generate a new vreg, check if the size of DataInfo is big enough */
    if (vRegNO >= cgFunc.GetRD()->GetRegSize(*strInsn.GetBB())) {
      cgFunc.GetRD()->EnlargeRegCapacity(vRegNO);
    }
    vregOpnd = &cgFunc.CreateVirtualRegisterOperand(vRegNO);
    MOperator newMop = SelectMovMop(resRegOpnd.IsOfFloatOrSIMDClass(), resRegOpnd.GetSize() == k64BitSize);
    newMovInsn = &cgFunc.GetInsnBuilder()->BuildInsn(newMop, *vregOpnd, srcRegOpnd);
    newMovInsn->SetId(strInsn.GetId() + memSeq + 1);
    strInsn.GetBB()->InsertInsnAfter(strInsn, *newMovInsn);
    str2MovMap[&strInsn][memSeq] = newMovInsn;
    /* update DataInfo */
    cgFunc.GetRD()->UpdateInOut(*strInsn.GetBB(), true);
  } else {
    newMovInsn = str2MovMap[&strInsn][memSeq];
    vregOpnd = &static_cast<RegOperand&>(newMovInsn->GetOperand(kInsnFirstOpnd));
  }
  MOperator movMop = SelectMovMop(resRegOpnd.IsOfFloatOrSIMDClass(), resRegOpnd.GetSize() == k64BitSize);
  Insn &movInsn = cgFunc.GetInsnBuilder()->BuildInsn(movMop, resRegOpnd, *vregOpnd);
  movInsn.SetId(ldrInsn.GetId());
  ldrInsn.GetBB()->ReplaceInsn(ldrInsn, movInsn);
  if (CG_DEBUG_FUNC(cgFunc)) {
    LogInfo::MapleLogger() << "replace ldrInsn:\n";
    ldrInsn.Dump();
    LogInfo::MapleLogger() << "with movInsn:\n";
    movInsn.Dump();
  }

  /* Add comment. */
  MapleString newComment = ldrInsn.GetComment();
  if (strInsn.IsStorePair()) {
    newComment += ";  stp-load die version.";
  } else {
    newComment += ";  str-load die version.";
  }
  movInsn.SetComment(newComment);
  cgFunc.GetRD()->InitGenUse(*ldrInsn.GetBB(), false);
}

bool AArch64StoreLoadOpt::HasMemBarrier(const Insn &ldrInsn, const Insn &strInsn) const {
  if (!cgFunc.GetMirModule().IsCModule()) {
    return false;
  }
  const Insn *currInsn = strInsn.GetNext();
  while (currInsn != &ldrInsn) {
    if (currInsn == nullptr) {
      return false;
    }
    if (currInsn->IsMachineInstruction() && currInsn->IsCall()) {
      return true;
    }
    currInsn = currInsn->GetNext();
  }
  return false;
}

/*
 * Transfer: store wzr, [MEM]
 *           ... // May exist branches.
 *           load  x200, [MEM]
 *        ==>
 *        OPT_VERSION_STP_ZERO / OPT_VERSION_STR_ZERO:
 *            store wzr, [MEM]
 *            ... // May exist branches. if x100 not dead here.
 *            mov   x200, wzr
 *
 *  Params:
 *    stInsn: indicate store insn.
 *    strSrcIdx: index of source register operand of store insn. (wzr in this example)
 *    memUseInsnSet: insns using memOperand
 */
void AArch64StoreLoadOpt::DoLoadZeroToMoveTransfer(const Insn &strInsn, short strSrcIdx,
                                                   const InsnSet &memUseInsnSet) const {
  /* comment for strInsn should be only added once */
  for (auto *ldrInsn : memUseInsnSet) {
    /* Currently we don't support useInsn is ldp insn. */
    if (!ldrInsn->IsLoad() || ldrInsn->GetDefRegs().size() > 1) {
      continue;
    }
    if (HasMemBarrier(*ldrInsn, strInsn)) {
      continue;
    }
    /* ldr reg, [mem], the index of [mem] is 1 */
    InsnSet defInsnForUseInsns = cgFunc.GetRD()->FindDefForMemOpnd(*ldrInsn, 1);
    /* If load has multiple definition, continue. */
    if (defInsnForUseInsns.size() > 1) {
      continue;
    }

    auto &resOpnd = ldrInsn->GetOperand(0);
    auto &srcOpnd = strInsn.GetOperand(static_cast<uint32>(strSrcIdx));

    if (resOpnd.GetSize() != srcOpnd.GetSize()) {
      return;
    }
    RegOperand &resRegOpnd = static_cast<RegOperand&>(resOpnd);
    MOperator movMop = SelectMovMop(resRegOpnd.IsOfFloatOrSIMDClass(), resRegOpnd.GetSize() == k64BitSize);
    Insn &movInsn = cgFunc.GetInsnBuilder()->BuildInsn(movMop, resOpnd, srcOpnd);
    movInsn.SetId(ldrInsn->GetId());
    ldrInsn->GetBB()->ReplaceInsn(*ldrInsn, movInsn);

    /* Add comment. */
    MapleString newComment = ldrInsn->GetComment();
    newComment += ",  str-load zero version";
    movInsn.SetComment(newComment);
  }
}

bool AArch64StoreLoadOpt::CheckStoreOpCode(MOperator opCode) const {
  switch (opCode) {
    case MOP_wstr:
    case MOP_xstr:
    case MOP_sstr:
    case MOP_dstr:
    case MOP_wstp:
    case MOP_xstp:
    case MOP_sstp:
    case MOP_dstp:
    case MOP_wstrb:
    case MOP_wstrh:
      return true;
    default:
      return false;
  }
}

void AArch64StoreLoadOpt::MemPropInit() {
  propMode = kUndef;
  amount = 0;
  removeDefInsn = false;
}

bool AArch64StoreLoadOpt::CheckReplaceReg(Insn &defInsn, Insn &currInsn, InsnSet &replaceRegDefSet,
                                          regno_t replaceRegNo) {
  if (replaceRegDefSet.empty()) {
    return true;
  }
  if (defInsn.GetBB() == currInsn.GetBB()) {
    /* check replace reg def between defInsn and currInsn */
    Insn *tmpInsn = defInsn.GetNext();
    while (tmpInsn != nullptr && tmpInsn != &currInsn) {
      if (replaceRegDefSet.find(tmpInsn) != replaceRegDefSet.end()) {
        return false;
      }
      tmpInsn = tmpInsn->GetNext();
    }
  } else {
    regno_t defRegno = static_cast<RegOperand&>(defInsn.GetOperand(kInsnFirstOpnd)).GetRegisterNumber();
    if (defRegno == replaceRegNo) {
      auto *defLoop = loopInfo.GetBBLoopParent(defInsn.GetBB()->GetId());
      auto defLoopId = defLoop ? defLoop->GetHeader().GetId() : 0;
      auto *curLoop = loopInfo.GetBBLoopParent(currInsn.GetBB()->GetId());
      auto curLoopId = curLoop ? curLoop->GetHeader().GetId() : 0;
      if (defLoopId != curLoopId) {
        return false;
      }
    }
    AArch64ReachingDefinition *a64RD = static_cast<AArch64ReachingDefinition*>(cgFunc.GetRD());
    if (a64RD->HasRegDefBetweenInsnGlobal(replaceRegNo, defInsn, currInsn)) {
      return false;
    }
  }

  if (replaceRegDefSet.size() == 1 && *replaceRegDefSet.begin() == &defInsn) {
    /* lsl x1, x1, #3    <-----should be removed after replace MemOperand of ldrInsn.
     * ldr x0, [x0,x1]   <-----should be single useInsn for x1
     */
    InsnSet newRegUseSet = cgFunc.GetRD()->FindUseForRegOpnd(defInsn, replaceRegNo, true);
    if (newRegUseSet.size() != k1BitSize) {
      return false;
    }
    removeDefInsn = true;
  }
  return true;
}

bool AArch64StoreLoadOpt::CheckDefInsn(Insn &defInsn, Insn &currInsn) {
  if (defInsn.GetOperandSize() < k2ByteSize) {
    return false;
  }
  for (uint32 i = kInsnSecondOpnd; i < defInsn.GetOperandSize(); i++) {
    Operand &opnd = defInsn.GetOperand(i);
    if (defInsn.IsMove() && opnd.IsRegister() && !cgFunc.IsSPOrFP(static_cast<RegOperand&>(opnd))) {
      return false;
    }
    if (opnd.IsRegister()) {
      RegOperand &a64OpndTmp = static_cast<RegOperand&>(opnd);
      regno_t replaceRegNo = a64OpndTmp.GetRegisterNumber();
      InsnSet newRegDefSet = cgFunc.GetRD()->FindDefForRegOpnd(currInsn, replaceRegNo, true);
      if (!CheckReplaceReg(defInsn, currInsn, newRegDefSet, replaceRegNo)) {
        return false;
      }
    }
  }
  return true;
}

bool AArch64StoreLoadOpt::CheckNewMemOffset(const Insn &insn, MemOperand *newMemOpnd, uint32 opndIdx) {
  AArch64CGFunc &a64CgFunc = static_cast<AArch64CGFunc&>(cgFunc);
  if ((newMemOpnd->GetOffsetImmediate() != nullptr) &&
      !a64CgFunc.IsOperandImmValid(insn.GetMachineOpcode(), newMemOpnd, opndIdx)) {
    return false;
  }
  /* is ldp or stp, addrMode must be BOI */
  if ((opndIdx == kInsnThirdOpnd) && (newMemOpnd->GetAddrMode() != MemOperand::kBOI)) {
    return false;
  }
  return true;
}

MemOperand *AArch64StoreLoadOpt::SelectReplaceExt(const Insn &defInsn, RegOperand &base, bool isSigned) {
  MemOperand *newMemOpnd = nullptr;
  RegOperand *newOffset = static_cast<RegOperand*>(&defInsn.GetOperand(kInsnSecondOpnd));
  CHECK_FATAL(newOffset != nullptr, "newOffset is null!");
  /* defInsn is extend, currMemOpnd is same extend or shift */
  if (propMode == kPropOffset) {
    newMemOpnd = static_cast<AArch64CGFunc&>(cgFunc).CreateMemOperand(memSize, base, *newOffset);
  } else if ((propMode == kPropSignedExtend) && isSigned && MemOperand::CheckNewAmount(memSize, amount)) {
    ExtendShiftOperand &extendOperand =
        static_cast<AArch64CGFunc&>(cgFunc).CreateExtendShiftOperand(ExtendShiftOperand::kSXTW, amount, k32BitSize);
    newMemOpnd = static_cast<AArch64CGFunc&>(cgFunc).CreateMemOperand(memSize, base, *newOffset, extendOperand);
  } else if ((propMode == kPropUnsignedExtend) && !isSigned && MemOperand::CheckNewAmount(memSize, amount)) {
    ExtendShiftOperand &extendOperand =
        static_cast<AArch64CGFunc&>(cgFunc).CreateExtendShiftOperand(ExtendShiftOperand::kUXTW, amount, k32BitSize);
    newMemOpnd = static_cast<AArch64CGFunc&>(cgFunc).CreateMemOperand(memSize, base, *newOffset, extendOperand);
  } else if (propMode == kPropShift && MemOperand::CheckNewAmount(memSize, amount)) {
    BitShiftOperand &bitOperand =
        static_cast<AArch64CGFunc&>(cgFunc).CreateBitShiftOperand(BitShiftOperand::kShiftLSL, amount, k32BitSize);
    newMemOpnd = static_cast<AArch64CGFunc&>(cgFunc).CreateMemOperand(memSize, base, *newOffset, bitOperand);
  }
  return newMemOpnd;
}

MemOperand *AArch64StoreLoadOpt::HandleArithImmDef(RegOperand &replace, Operand *oldOffset,
                                                   int64 defVal, VaryType varyType) {
  if (propMode != kPropBase) {
    return nullptr;
  }
  OfstOperand *newOfstImm = nullptr;
  if (oldOffset == nullptr) {
    newOfstImm = &static_cast<AArch64CGFunc&>(cgFunc).CreateOfstOpnd(static_cast<uint64>(defVal), k32BitSize);
  } else {
    auto *ofstOpnd = static_cast<OfstOperand*>(oldOffset);
    CHECK_FATAL(ofstOpnd != nullptr, "oldOffsetOpnd is null");
    int64 newOffVal = defVal + ofstOpnd->GetValue();
    newOfstImm = &static_cast<AArch64CGFunc&>(cgFunc).CreateOfstOpnd(static_cast<uint64>(newOffVal), k32BitSize);
  }
  CHECK_FATAL(newOfstImm != nullptr, "newOffset is null!");
  newOfstImm->SetVary(varyType);
  return static_cast<AArch64CGFunc&>(cgFunc).CreateMemOperand(memSize, replace, *newOfstImm);
}

/*
 * limit to adjacent bb to avoid ra spill.
 */
bool AArch64StoreLoadOpt::IsAdjacentBB(Insn &defInsn, Insn &curInsn) const {
  if (defInsn.GetBB() == curInsn.GetBB()) {
    return true;
  }
  for (auto *bb : defInsn.GetBB()->GetSuccs()) {
    if (bb == curInsn.GetBB()) {
      return true;
    }
    if (bb->IsSoloGoto()) {
      BB *tragetBB = CGCFG::GetTargetSuc(*bb);
      if (tragetBB == curInsn.GetBB()) {
        return true;
      }
    }
  }
  return false;
}

/*
 * currAddrMode | defMop       | propMode | replaceAddrMode
 * =============================================================================
 * boi          | addrri       | base     | boi, update imm(offset)
 *              | addrrr       | base     | imm(offset) == 0(nullptr) ? borx : NA
 *              | subrri       | base     | boi, update imm(offset)
 *              | subrrr       | base     | NA
 *              | adrpl12      | base     | imm(offset) == 0(nullptr) ? literal : NA
 *              | movrr        | base     | boi
 *              | movri        | base     | NA
 *              | extend/lsl   | base     | NA
 * =============================================================================
 * borx         | addrri       | offset   | NA
 * (noextend)   | addrrr       | offset   | NA
 *              | subrri       | offset   | NA
 *              | subrrr       | offset   | NA
 *              | adrpl12      | offset   | NA
 *              | movrr        | offset   | borx
 *              | movri        | offset   | bori
 *              | extend/lsl   | offset   | borx(with extend)
 * =============================================================================
 * borx         | addrri       | extend   | NA
 * (extend)     | addrrr       | extend   | NA
 *              | subrri       | extend   | NA
 *              | subrrr       | extend   | NA
 *              | adrpl12      | extend   | NA
 *              | movrr        | extend   | borx
 *              | movri        | extend   | NA
 *              | extend/lsl   | extend   | borx(with extend)
 * =============================================================================
 */
MemOperand *AArch64StoreLoadOpt::SelectReplaceMem(Insn &defInsn, Insn &curInsn,
                                                  RegOperand &base, Operand *offset) {
  MemOperand *newMemOpnd = nullptr;
  MOperator opCode = defInsn.GetMachineOpcode();
  RegOperand *replace = static_cast<RegOperand*>(&defInsn.GetOperand(kInsnSecondOpnd));
  Operand *currMemOpnd = curInsn.GetMemOpnd();
  CHECK_NULL_FATAL(currMemOpnd);
  memSize = currMemOpnd->GetSize();
  switch (opCode) {
    case MOP_xsubrri12:
    case MOP_wsubrri12: {
      if (!IsAdjacentBB(defInsn, curInsn)) {
        break;
      }
      auto &immOpnd = static_cast<ImmOperand&>(defInsn.GetOperand(kInsnThirdOpnd));
      // sub can not prop vary imm
      CHECK_FATAL(immOpnd.GetVary() != kUnAdjustVary, "NIY, imm wrong vary type");
      int64 defVal = -(immOpnd.GetValue());
      newMemOpnd = HandleArithImmDef(*replace, offset, defVal);
      break;
    }
    case MOP_xaddrri12:
    case MOP_waddrri12: {
      auto &immOpnd = static_cast<ImmOperand&>(defInsn.GetOperand(kInsnThirdOpnd));
      int64 defVal = immOpnd.GetValue();
      newMemOpnd = HandleArithImmDef(*replace, offset, defVal, immOpnd.GetVary());
      break;
    }
    case MOP_xaddrrr:
    case MOP_waddrrr:
    case MOP_dadd:
    case MOP_sadd: {
      if (propMode == kPropBase) {
        ImmOperand *ofstOpnd = static_cast<ImmOperand*>(offset);
        if (!ofstOpnd->IsZero()) {
          break;
        }
        RegOperand *newOffset = static_cast<RegOperand*>(&defInsn.GetOperand(kInsnThirdOpnd));
        CHECK_FATAL(newOffset != nullptr, "newOffset is null!");
        if (replace->GetSize() != newOffset->GetSize()) {
          break;
        }
        newMemOpnd = static_cast<AArch64CGFunc&>(cgFunc).CreateMemOperand(memSize, *replace, *newOffset);
      }
      break;
    }
    case MOP_xadrpl12: {
      if (propMode == kPropBase) {
        ImmOperand *ofstOpnd = static_cast<ImmOperand*>(offset);
        CHECK_NULL_FATAL(ofstOpnd);
        int64 val = ofstOpnd->GetValue();
        StImmOperand *offset1 = static_cast<StImmOperand*>(&defInsn.GetOperand(kInsnThirdOpnd));
        CHECK_FATAL(offset1 != nullptr, "offset1 is null!");
        val += offset1->GetOffset();
        OfstOperand *newOfsetOpnd = &static_cast<AArch64CGFunc&>(cgFunc).CreateOfstOpnd(
            static_cast<uint64>(val), k32BitSize);
        CHECK_FATAL(newOfsetOpnd != nullptr, "newOfsetOpnd is null!");
        const MIRSymbol *addr = offset1->GetSymbol();
        /* do not guarantee rodata alignment at Os */
        if (CGOptions::OptimizeForSize() && addr->IsReadOnly()) {
          break;
        }
        newMemOpnd = static_cast<AArch64CGFunc&>(cgFunc).CreateMemOperand(memSize, *replace, *newOfsetOpnd, *addr);
      }
      break;
    }
    case MOP_xmovrr:
    case MOP_wmovrr: {
      if (propMode == kPropBase) {
        OfstOperand *offsetTmp = static_cast<OfstOperand*>(offset);
        CHECK_FATAL(offsetTmp != nullptr, "newOffset is null!");
        newMemOpnd = static_cast<AArch64CGFunc&>(cgFunc).CreateMemOperand(memSize, *replace, *offsetTmp);
      } else if (propMode == kPropOffset) { /* if newOffset is SP, swap base and newOffset */
        if (cgFunc.IsSPOrFP(*replace)) {
          newMemOpnd = static_cast<AArch64CGFunc&>(cgFunc).CreateMemOperand(memSize, *replace, base);
        } else {
          newMemOpnd = static_cast<AArch64CGFunc&>(cgFunc).CreateMemOperand(memSize, base, *replace);
        }
      }
      break;
    }
    case MOP_wmovri32:
    case MOP_xmovri64: {
      if (propMode == kPropOffset) {
        ImmOperand *imm = static_cast<ImmOperand*>(&defInsn.GetOperand(kInsnSecondOpnd));
        OfstOperand *newOffset = &static_cast<AArch64CGFunc&>(cgFunc).CreateOfstOpnd(
            static_cast<uint64>(imm->GetValue()), k32BitSize);
        CHECK_FATAL(newOffset != nullptr, "newOffset is null!");
        newMemOpnd = static_cast<AArch64CGFunc&>(cgFunc).CreateMemOperand(memSize, base, *newOffset);
      }
      break;
    }
    case MOP_xlslrri6:
    case MOP_wlslrri5: {
      ImmOperand *imm = static_cast<ImmOperand*>(&defInsn.GetOperand(kInsnThirdOpnd));
      RegOperand *newOffset = static_cast<RegOperand*>(&defInsn.GetOperand(kInsnSecondOpnd));
      CHECK_FATAL(newOffset != nullptr, "newOffset is null!");
      uint32 shift = static_cast<uint32>(imm->GetValue());
      /* lsl has Implicit Conversion, use uxtw instead. */
      if (opCode == MOP_xlslrri6 && newOffset->GetSize() == k32BitSize && propMode == kPropOffset &&
          MemOperand::CheckNewAmount(memSize, shift)) {
        propMode = kPropUnsignedExtend;
        ExtendShiftOperand &exOpnd =
            static_cast<AArch64CGFunc&>(cgFunc).CreateExtendShiftOperand(ExtendShiftOperand::kUXTW, shift, k8BitSize);
        newMemOpnd = static_cast<AArch64CGFunc&>(cgFunc).CreateMemOperand(memSize, base, *newOffset, exOpnd);
        break;
      }
      if (propMode == kPropOffset) {
        if (MemOperand::CheckNewAmount(memSize, shift)) {
          BitShiftOperand &shiftOperand =
              static_cast<AArch64CGFunc&>(cgFunc).CreateBitShiftOperand(BitShiftOperand::kShiftLSL, shift, k8BitSize);
          newMemOpnd =
              static_cast<AArch64CGFunc&>(cgFunc).CreateMemOperand(memSize, base, *newOffset, shiftOperand);
        }
      } else if (propMode == kPropShift) {
        shift += amount;
        if (MemOperand::CheckNewAmount(memSize, shift)) {
          BitShiftOperand &shiftOperand =
              static_cast<AArch64CGFunc&>(cgFunc).CreateBitShiftOperand(BitShiftOperand::kShiftLSL, shift, k8BitSize);
          newMemOpnd =
              static_cast<AArch64CGFunc&>(cgFunc).CreateMemOperand(memSize, base, *newOffset, shiftOperand);
        }
      }
      break;
    }
    case MOP_xsxtw64: {
      if (propMode == kPropUnsignedExtend || propMode == kPropBase) {
        break;
      }
      propMode = kPropSignedExtend;
      newMemOpnd = SelectReplaceExt(defInsn, base, true);
      break;
    }
    case MOP_xuxtw64: {
      if (propMode == kPropSignedExtend || propMode == kPropBase) {
        break;
      }
      propMode = kPropUnsignedExtend;
      newMemOpnd = SelectReplaceExt(defInsn, base, false);
      break;
    }
    default:
      break;
  }
  return newMemOpnd;
}

bool AArch64StoreLoadOpt::ReplaceMemOpnd(Insn &insn, regno_t regNo, RegOperand &base, Operand *offset) {
  AArch64ReachingDefinition *a64RD = static_cast<AArch64ReachingDefinition*>(cgFunc.GetRD());
  CHECK_FATAL((a64RD != nullptr), "check a64RD!");
  InsnSet regDefSet = a64RD->FindDefForRegOpnd(insn, regNo, true);
  if (regDefSet.size() != k1BitSize) {
    return false;
  }
  Insn *regDefInsn = *regDefSet.begin();
  if (!CheckDefInsn(*regDefInsn, insn)) {
    return false;
  }

  MemOperand *newMemOpnd = SelectReplaceMem(*regDefInsn, insn, base, offset);
  if (newMemOpnd == nullptr) {
    return false;
  }

  /* check new memOpnd */
  if (newMemOpnd->GetBaseRegister() != nullptr) {
    InsnSet regDefSetForNewBase =
        a64RD->FindDefForRegOpnd(insn, newMemOpnd->GetBaseRegister()->GetRegisterNumber(), true);
    if (regDefSetForNewBase.size() != k1BitSize) {
      return false;
    }
  }
  if (newMemOpnd->GetIndexRegister() != nullptr) {
    InsnSet regDefSetForNewIndex =
        a64RD->FindDefForRegOpnd(insn, newMemOpnd->GetIndexRegister()->GetRegisterNumber(), true);
    if (regDefSetForNewIndex.size() != k1BitSize) {
      return false;
    }
  }

  uint32 opndIdx;
  if (insn.IsLoadPair() || insn.IsStorePair()) {
    if (newMemOpnd->GetOffsetImmediate() == nullptr) {
      return false;
    }
    opndIdx = kInsnThirdOpnd;
  } else {
    opndIdx = kInsnSecondOpnd;
  }
  if (!CheckNewMemOffset(insn, newMemOpnd, opndIdx)) {
    return false;
  }
  if (CG_DEBUG_FUNC(cgFunc)) {
    std::cout << "replace insn:" << std::endl;
    insn.Dump();
  }
  insn.SetOperand(opndIdx, *newMemOpnd);
  if (CG_DEBUG_FUNC(cgFunc)) {
    std::cout << "new insn:" << std::endl;
    insn.Dump();
  }
  if (removeDefInsn) {
    if (CG_DEBUG_FUNC(cgFunc)) {
      std::cout << "remove insn:" << std::endl;
      regDefInsn->Dump();
    }
    regDefInsn->GetBB()->RemoveInsn(*regDefInsn);
  }
  cgFunc.GetRD()->InitGenUse(*regDefInsn->GetBB(), false);
  cgFunc.GetRD()->UpdateInOut(*insn.GetBB(), false);
  cgFunc.GetRD()->UpdateInOut(*insn.GetBB(), true);
  return true;
}

bool AArch64StoreLoadOpt::CanDoMemProp(const Insn *insn) {
  if (!cgFunc.GetMirModule().IsCModule()) {
    return false;
  }
  if (!insn->IsMachineInstruction()) {
    return false;
  }
  if (insn->GetMachineOpcode() == MOP_qstr) {
    return false;
  }

  if (insn->IsLoad() || insn->IsStore()) {
    if (insn->IsAtomic()) {
      return false;
    }
    // It is not desired to propagate on 128bit reg with immediate offset
    // which may cause linker to issue misalignment error
    if (insn->IsAtomic() || insn->GetOperand(0).GetSize() == k128BitSize) {
      return false;
    }
    MemOperand *currMemOpnd = static_cast<MemOperand*>(insn->GetMemOpnd());
    return currMemOpnd != nullptr;
  }
  return false;
}

void AArch64StoreLoadOpt::SelectPropMode(const MemOperand &currMemOpnd) {
  MemOperand::AArch64AddressingMode currAddrMode = currMemOpnd.GetAddrMode();
  switch (currAddrMode) {
    case MemOperand::kBOI: {
      propMode = kPropBase;
      break;
    }
    case MemOperand::kBOR: {
      propMode = kPropOffset;
      break;
    }
    case MemOperand::kBOE: {
      if (currMemOpnd.SignedExtend()) {
        propMode = kPropSignedExtend;
      } else if (currMemOpnd.UnsignedExtend()) {
        propMode = kPropUnsignedExtend;
      }
      amount = currMemOpnd.ShiftAmount();
      break;
    }
    case MemOperand::kBOL: {
      amount = currMemOpnd.ShiftAmount();
      CHECK_FATAL(amount != 0, "check currMemOpnd!");
      propMode = kPropShift;
      break;
    }
    default:
      propMode = kUndef;
  }
}

/*
 * Optimize: store x100, [MEM]
 *           ... // May exist branches.
 *           load  x200, [MEM]
 *        ==>
 *        OPT_VERSION_STP_LIVE / OPT_VERSION_STR_LIVE:
 *           store x100, [MEM]
 *           ... // May exist branches. if x100 not dead here.
 *           mov   x200, x100
 *        OPT_VERSION_STP_DIE / OPT_VERSION_STR_DIE:
 *           store x100, [MEM]
 *           mov x9000(new reg), x100
 *           ... // May exist branches. if x100 dead here.
 *           mov   x200, x9000
 *
 *  Note: x100 may be wzr/xzr registers.
 */
void AArch64StoreLoadOpt::DoStoreLoadOpt() {
  AArch64CGFunc &a64CgFunc = static_cast<AArch64CGFunc&>(cgFunc);
  if (a64CgFunc.IsIntrnCallForC()) {
    return;
  }
  FOR_ALL_BB(bb, &a64CgFunc) {
    FOR_BB_INSNS_SAFE(insn, bb, next) {
      MOperator mOp = insn->GetMachineOpcode();
      if (CanDoMemProp(insn)) {
        MemProp(*insn);
      }
      if (a64CgFunc.GetMirModule().IsCModule() && cgFunc.GetRD()->OnlyAnalysisReg()) {
        continue;
      }
      if (!insn->IsMachineInstruction() || !insn->IsStore() || !CheckStoreOpCode(mOp) ||
          (a64CgFunc.GetMirModule().IsCModule() && !a64CgFunc.IsAfterRegAlloc()) ||
          (!a64CgFunc.GetMirModule().IsCModule() && a64CgFunc.IsAfterRegAlloc())) {
        continue;
      }
      if (insn->IsStorePair()) {
        ProcessStrPair(*insn);
        continue;
      }
      ProcessStr(*insn);
    }
  }
}

/*
 * PropBase:
 *   add/sub x1, x2, #immVal1
 *   ...(no def of x2)
 *   ldr/str x0, [x1, #immVal2]
 *   ======>
 *   add/sub x1, x2, #immVal1
 *   ...
 *   ldr/str x0, [x2, #(immVal1 + immVal2)/#(-immVal1 + immVal2)]
 *
 * PropOffset:
 *   sxtw x2, w2
 *   lsl x1, x2, #1~3
 *   ...(no def of x2)
 *   ldr/str x0, [x0, x1]
 *   ======>
 *   sxtw x2, w2
 *   lsl x1, x2, #1~3
 *   ...
 *   ldr/str x0, [x0, w2, sxtw 1~3]
 */
void AArch64StoreLoadOpt::MemProp(Insn &insn) {
  MemPropInit();
  MemOperand *currMemOpnd = static_cast<MemOperand*>(insn.GetMemOpnd());
  SelectPropMode(*currMemOpnd);
  RegOperand *base = currMemOpnd->GetBaseRegister();
  Operand *offset = currMemOpnd->GetOffset();
  bool memReplaced = false;

  if (propMode == kUndef) {
    return;
  } else if (propMode == kPropBase) {
    ImmOperand *immOffset = static_cast<ImmOperand*>(offset);
    CHECK_NULL_FATAL(immOffset);
    regno_t baseRegNo = base->GetRegisterNumber();
    memReplaced = ReplaceMemOpnd(insn, baseRegNo, *base, immOffset);
  } else {
    RegOperand *regOffset = static_cast<RegOperand*>(offset);
    if (regOffset == nullptr) {
      return;
    }
    regno_t offsetRegNo = regOffset->GetRegisterNumber();
    memReplaced = ReplaceMemOpnd(insn, offsetRegNo, *base, regOffset);
  }

  /* if prop success, find more prop chance */
  if (memReplaced) {
    MemProp(insn);
  }
}

/*
 * Assume stack(FP) will not be varied out of pro/epi log
 * PreIndex:
 *   add/sub x1, x1 #immVal1
 *   ...(no def/use of x1)
 *   ldr/str x0, [x1]
 *   ======>
 *   ldr/str x0, [x1, #immVal1]!
 *
 * PostIndex:
 *   ldr/str x0, [x1]
 *   ...(no def/use of x1)
 *   add/sub x1, x1, #immVal1
 *   ======>
 *   ldr/str x0, [x1],  #immVal1
 */
void AArch64StoreLoadOpt::StrLdrIndexModeOpt(Insn &currInsn) {
  auto *curMemopnd = static_cast<MemOperand*>(currInsn.GetMemOpnd());
  ASSERT(curMemopnd != nullptr, " get memopnd failed");
  /* one instruction cannot define one register twice */
  if (!CanDoIndexOpt(*curMemopnd) || currInsn.IsRegDefined(curMemopnd->GetBaseRegister()->GetRegisterNumber())) {
    return;
  }
  MemOperand *newMemopnd = SelectIndexOptMode(currInsn, *curMemopnd);
  if (newMemopnd != nullptr) {
    currInsn.SetMemOpnd(newMemopnd);
  }
}

bool AArch64StoreLoadOpt::CanDoIndexOpt(const MemOperand &currMemOpnd) {
  if (currMemOpnd.GetAddrMode() != MemOperand::kBOI) {
    return false;
  }
  ASSERT(currMemOpnd.GetOffsetImmediate() != nullptr, " kBOI memopnd have no offset imm");
  if (!currMemOpnd.GetOffsetImmediate()->IsImmOffset()) {
    return false;
  }
  if (cgFunc.IsSPOrFP(*currMemOpnd.GetBaseRegister())) {
    return false;
  }
  OfstOperand *a64Ofst = currMemOpnd.GetOffsetImmediate();
  if (a64Ofst == nullptr) {
    return false;
  }
  return a64Ofst->GetValue() == 0;
}

int64 AArch64StoreLoadOpt::GetOffsetForNewIndex(Insn &defInsn, Insn &insn,
                                                regno_t baseRegNO, uint32 memOpndSize) const {
  bool subMode = defInsn.GetMachineOpcode() == MOP_wsubrri12 || defInsn.GetMachineOpcode() == MOP_xsubrri12;
  bool addMode = defInsn.GetMachineOpcode() == MOP_waddrri12 || defInsn.GetMachineOpcode() == MOP_xaddrri12;
  if (addMode || subMode) {
    ASSERT(static_cast<RegOperand&>(defInsn.GetOperand(kInsnFirstOpnd)).GetRegisterNumber() == baseRegNO,
           "check def opnd");
    auto &srcOpnd = static_cast<RegOperand&>(defInsn.GetOperand(kInsnSecondOpnd));
    if (srcOpnd.GetRegisterNumber() == baseRegNO && defInsn.GetBB() == insn.GetBB()) {
      int64 offsetVal = static_cast<ImmOperand&>(defInsn.GetOperand(kInsnThirdOpnd)).GetValue();
      if (!MemOperand::IsSIMMOffsetOutOfRange(offsetVal, memOpndSize == k64BitSize, insn.IsLoadStorePair())) {
        return subMode ? -offsetVal : offsetVal;
      }
    }
  }
  return kMaxPimm8; /* simm max value cannot excced pimm max value */
};


MemOperand *AArch64StoreLoadOpt::SelectIndexOptMode(Insn &insn, const MemOperand &curMemOpnd) {
  AArch64ReachingDefinition *a64RD = static_cast<AArch64ReachingDefinition*>(cgFunc.GetRD());
  ASSERT((a64RD != nullptr), "check a64RD!");
  regno_t baseRegisterNO = curMemOpnd.GetBaseRegister()->GetRegisterNumber();
  auto &a64cgFunc = static_cast<AArch64CGFunc&>(cgFunc);
  /* pre index */
  InsnSet regDefSet = a64RD->FindDefForRegOpnd(insn, baseRegisterNO, true);
  if (regDefSet.size() == k1BitSize) {
    Insn *defInsn = *regDefSet.begin();
    int64 defOffset = GetOffsetForNewIndex(*defInsn, insn, baseRegisterNO, curMemOpnd.GetSize());
    if (defOffset < kMaxPimm8) {
      InsnSet tempCheck;
      (void)a64RD->FindRegUseBetweenInsn(baseRegisterNO, defInsn->GetNext(), insn.GetPrev(), tempCheck);
      if (tempCheck.empty() && (defInsn->GetBB() == insn.GetBB())) {
        auto &newMem =
            a64cgFunc.CreateMemOpnd(*curMemOpnd.GetBaseRegister(), defOffset, curMemOpnd.GetSize());
        ASSERT(newMem.GetOffsetImmediate() != nullptr, "need offset for memopnd in this case");
        newMem.SetAddrMode(MemOperand::kPreIndex);
        insn.GetBB()->RemoveInsn(*defInsn);
        return &newMem;
      }
    }
  }
  /* post index */
  std::vector<Insn*> refDefVec = a64RD->FindRegDefBetweenInsn(baseRegisterNO, &insn, insn.GetBB()->GetLastInsn(), true);
  if (!refDefVec.empty()) {
    Insn *defInsn = refDefVec.back();
    int64 defOffset = GetOffsetForNewIndex(*defInsn, insn, baseRegisterNO, curMemOpnd.GetSize());
    if (defOffset < kMaxPimm8) {
      InsnSet tempCheck;
      (void)a64RD->FindRegUseBetweenInsn(baseRegisterNO, insn.GetNext(), defInsn->GetPrev(), tempCheck);
      if (tempCheck.empty() && (defInsn->GetBB() == insn.GetBB())) {
        auto &newMem = a64cgFunc.CreateMemOpnd(
            *curMemOpnd.GetBaseRegister(), defOffset, curMemOpnd.GetSize());
        ASSERT(newMem.GetOffsetImmediate() != nullptr, "need offset for memopnd in this case");
        newMem.SetAddrMode(MemOperand::kPostIndex);
        insn.GetBB()->RemoveInsn(*defInsn);
        return &newMem;
      }
    }
  }
  return nullptr;
}

void AArch64StoreLoadOpt::ProcessStrPair(Insn &insn) {
  const short memIndex = 2;
  short regIndex = 0;
  Operand &opnd = insn.GetOperand(memIndex);
  auto &memOpnd = static_cast<MemOperand&>(opnd);
  RegOperand *base = memOpnd.GetBaseRegister();
  if ((base == nullptr) || !(cgFunc.GetRD()->IsFrameReg(*base))) {
    return;
  }
  if (cgFunc.IsAfterRegAlloc() && !insn.IsSpillInsn()) {
    return;
  }
  ASSERT(memOpnd.GetIndexRegister() == nullptr, "frame MemOperand must not be exist register index");
  InsnSet memUseInsnSet;
  for (int i = 0; i != kMaxMovNum; ++i) {
    memUseInsnSet.clear();
    if (i == 0) {
      regIndex = 0;
      memUseInsnSet = cgFunc.GetRD()->FindUseForMemOpnd(insn, memIndex);
    } else {
      regIndex = 1;
      memUseInsnSet = cgFunc.GetRD()->FindUseForMemOpnd(insn, memIndex, true);
    }
    if (memUseInsnSet.empty()) {
      return;
    }
    auto &regOpnd = static_cast<RegOperand&>(insn.GetOperand(static_cast<uint32>(regIndex)));
    if (regOpnd.GetRegisterNumber() == RZR) {
      DoLoadZeroToMoveTransfer(insn, regIndex, memUseInsnSet);
    } else {
      DoLoadToMoveTransfer(insn, regIndex, i, memUseInsnSet);
    }
  }
}

void AArch64StoreLoadOpt::ProcessStr(Insn &insn) {
  /* str x100, [mem], mem index is 1, x100 index is 0; */
  const short memIndex = 1;
  const short regIndex = 0;
  Operand &opnd = insn.GetOperand(memIndex);
  auto &memOpnd = static_cast<MemOperand&>(opnd);
  RegOperand *base = memOpnd.GetBaseRegister();
  if ((base == nullptr) || !(cgFunc.GetRD()->IsFrameReg(*base))) {
    return;
  }

  if (cgFunc.IsAfterRegAlloc() && !insn.IsSpillInsn()) {
    return;
  }
  ASSERT(memOpnd.GetIndexRegister() == nullptr, "frame MemOperand must not be exist register index");

  InsnSet memUseInsnSet = cgFunc.GetRD()->FindUseForMemOpnd(insn, memIndex);
  if (memUseInsnSet.empty()) {
    return;
  }

  auto *regOpnd = static_cast<RegOperand*>(&insn.GetOperand(regIndex));
  CHECK_NULL_FATAL(regOpnd);
  if (regOpnd->GetRegisterNumber() == RZR) {
    DoLoadZeroToMoveTransfer(insn, regIndex, memUseInsnSet);
  } else {
    DoLoadToMoveTransfer(insn, regIndex, 0, memUseInsnSet);
  }
  if (cgFunc.IsAfterRegAlloc() && insn.IsSpillInsn()) {
    InsnSet newmemUseInsnSet = cgFunc.GetRD()->FindUseForMemOpnd(insn, memIndex);
    if (newmemUseInsnSet.empty()) {
      insn.GetBB()->RemoveInsn(insn);
    }
  }
}
}  /* namespace maplebe */
