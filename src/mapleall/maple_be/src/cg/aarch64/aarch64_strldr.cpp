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
  std::map<Insn*, bool> InsnState;
  for (auto *ldrInsn : memUseInsnSet) {
    InsnState[ldrInsn] = true;
  }
  for (auto *ldrInsn : memUseInsnSet) {
    if (!ldrInsn->IsLoad() || (ldrInsn->GetResultNum() > 1) || ldrInsn->GetBB()->IsCleanup()) {
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
      InsnState[ldrInsn] = false;
      continue;
    }

    Operand &resOpnd = ldrInsn->GetOperand(kInsnFirstOpnd);
    Operand &srcOpnd = strInsn.GetOperand(strSrcIdx);
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
      InsnState[ldrInsn] = false;
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
  it++;
  for (; it != memUseInsnSet.end(); it++) {
    Insn *curInsn = *it;
    if (InsnState[curInsn] == false) {
      continue;
    }
    if (!curInsn->IsLoad() || (curInsn->GetResultNum() > 1) || curInsn->GetBB()->IsCleanup()) {
      continue;
    }
    InsnSet memDefInsnSet = cgFunc.GetRD()->FindDefForMemOpnd(*curInsn, kInsnSecondOpnd);
    ASSERT(!memDefInsnSet.empty(), "load insn should have definitions.");
    if (memDefInsnSet.size() > 1) {
      continue;
    }
    auto prevIt = it;
    do {
      prevIt--;
      Insn *prevInsn = *prevIt;
      if (InsnState[prevInsn] == false) {
        continue;
      }
      if (prevInsn->GetBB() != curInsn->GetBB()) {
        break;
      }
      if (!prevInsn->IsLoad() || (prevInsn->GetResultNum() > 1) || prevInsn->GetBB()->IsCleanup()) {
        continue;
      }
      InsnSet memDefInsnSet = cgFunc.GetRD()->FindDefForMemOpnd(*curInsn, kInsnSecondOpnd);
      ASSERT(!memDefInsnSet.empty(), "load insn should have definitions.");
      if (memDefInsnSet.size() > 1) {
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
        InsnState[curInsn] = false;
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
    movInsn = &cgFunc.GetCG()->BuildInstruction<AArch64Insn>(movMop, resRegOpnd, vregOpnd);
  } else {
    movInsn = &cgFunc.GetCG()->BuildInstruction<AArch64Insn>(movMop, resRegOpnd, srcRegOpnd);
  }
  if (&resRegOpnd == &srcRegOpnd && cgFunc.IsAfterRegAlloc()) {
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
    newMovInsn = &cgFunc.GetCG()->BuildInstruction<AArch64Insn>(newMop, *vregOpnd, srcRegOpnd);
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
  Insn &movInsn = cgFunc.GetCG()->BuildInstruction<AArch64Insn>(movMop, resRegOpnd, *vregOpnd);
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
    if (currInsn->IsCall()) {
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
    if (!ldrInsn->IsLoad() || ldrInsn->GetResultNum() > 1) {
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
    auto &srcOpnd = strInsn.GetOperand(strSrcIdx);

    if (resOpnd.GetSize() != srcOpnd.GetSize()) {
      return;
    }
    RegOperand &resRegOpnd = static_cast<RegOperand&>(resOpnd);
    MOperator movMop = SelectMovMop(resRegOpnd.IsOfFloatOrSIMDClass(), resRegOpnd.GetSize() == k64BitSize);
    Insn &movInsn = cgFunc.GetCG()->BuildInstruction<AArch64Insn>(movMop, resOpnd, srcOpnd);
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
  if (replaceRegDefSet.size() > k1BitSize) {
    return false;
  } else if (replaceRegDefSet.empty()) {
    return true;
  }
  Insn *replaceRegDefInsn = *replaceRegDefSet.begin();
  if (defInsn.GetBB() == currInsn.GetBB()) {
    /* check replace reg def between defInsn and currInsn */
    Insn *tmpInsn = defInsn.GetNext();
    while (tmpInsn != &currInsn) {
      if (tmpInsn == replaceRegDefInsn) {
        return false;
      }
      tmpInsn = tmpInsn->GetNext();
    }
  } else {
    if (replaceRegDefInsn->GetBB() != defInsn.GetBB()) {
      return false;
    }
    if (replaceRegDefInsn->GetId() > defInsn.GetId()) {
      return false;
    }
  }

  if (replaceRegDefInsn == &defInsn) {
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
  Operand &opnd0 = defInsn.GetOperand(kInsnFirstOpnd);
  for (uint32 i = kInsnSecondOpnd; i < defInsn.GetOperandSize(); i++) {
    Operand &opnd = defInsn.GetOperand(i);
    if (opnd0.IsRegister() && opnd.IsRegister()) {
      AArch64RegOperand &a64Opnd0 = static_cast<AArch64RegOperand&>(opnd0);
      AArch64RegOperand &a64OpndTmp = static_cast<AArch64RegOperand&>(opnd);
      if (a64Opnd0.IsPhysicalRegister() || a64OpndTmp.IsPhysicalRegister()) {
        return false;
      }
    }
    if (opnd.IsRegister()) {
      AArch64RegOperand &a64OpndTmp = static_cast<AArch64RegOperand&>(opnd);
      regno_t replaceRegNo = a64OpndTmp.GetRegisterNumber();
      InsnSet newRegDefSet = cgFunc.GetRD()->FindDefForRegOpnd(currInsn, replaceRegNo, true);
      return CheckReplaceReg(defInsn, currInsn, newRegDefSet, replaceRegNo);
    }
  }
  return true;
}

bool AArch64StoreLoadOpt::CheckNewAmount(Insn &insn, uint32 newAmount) {
  MOperator mOp = insn.GetMachineOpcode();
  switch (mOp) {
    case MOP_wstrb:
    case MOP_wldrsb:
    case MOP_xldrsb:
    case MOP_wldrb: {
      return newAmount == 0;
    }
    case MOP_wstrh:
    case MOP_wldrsh:
    case MOP_xldrsh:
    case MOP_wldrh: {
      return (newAmount == 0) || (newAmount == k1BitSize);
    }
    case MOP_wstr:
    case MOP_sstr:
    case MOP_wldr:
    case MOP_sldr:
    case MOP_xldrsw: {
      return (newAmount == 0) || (newAmount == k2BitSize);
    }
    case MOP_qstr:
    case MOP_qldr: {
      return (newAmount == 0) || (newAmount == k4BitSize);
    }
    default: {
      return (newAmount == 0) || (newAmount == k3ByteSize);
    }
  }
}

bool AArch64StoreLoadOpt::CheckNewMemOffset(Insn &insn, AArch64MemOperand *newMemOpnd, uint32 opndIdx) {
  AArch64CGFunc &a64CgFunc = static_cast<AArch64CGFunc&>(cgFunc);
  if ((newMemOpnd->GetOffsetImmediate() != nullptr) &&
      !a64CgFunc.IsOperandImmValid(insn.GetMachineOpcode(), newMemOpnd, opndIdx)) {
    return false;
  }
  uint32 newAmount = newMemOpnd->ShiftAmount();
  if (!CheckNewAmount(insn, newAmount)) {
    return false;
  }
  /* is ldp or stp, addrMode must be BOI */
  if ((opndIdx == kInsnThirdOpnd) && (newMemOpnd->GetAddrMode() != AArch64MemOperand::kAddrModeBOi)) {
    return false;
  }
  return true;
}

AArch64MemOperand *AArch64StoreLoadOpt::SelectReplaceExt(Insn &defInsn, RegOperand &base, bool isSigned) {
  AArch64MemOperand *newMemOpnd = nullptr;
  AArch64RegOperand *newOffset = static_cast<AArch64RegOperand*>(&defInsn.GetOperand(kInsnSecondOpnd));
  CHECK_FATAL(newOffset != nullptr, "newOffset is null!");
  /* defInsn is extend, currMemOpnd is same extend or shift */
  bool propExtend = (propMode == kPropShift) || ((propMode == kPropSignedExtend) && isSigned) ||
                    ((propMode == kPropUnsignedExtend) && !isSigned);
  if (propMode == kPropOffset) {
    newMemOpnd = cgFunc.GetMemoryPool()->New<AArch64MemOperand>(
        AArch64MemOperand::kAddrModeBOrX, k64BitSize, base, *newOffset, 0, isSigned);
  } else if (propExtend) {
    newMemOpnd = cgFunc.GetMemoryPool()->New<AArch64MemOperand>(
        AArch64MemOperand::kAddrModeBOrX, k64BitSize, base, *newOffset, amount, isSigned);
  } else {
    return nullptr;
  }
  return newMemOpnd;
}

/*
 * currAddrMode | defMop       | propMode | replaceAddrMode
 * =============================================================================
 * boi          | addrri       | base     | boi, update imm(offset)
 *              | addrrr       | base     | imm(offset) == 0(nullptr) ? borx : NA
 *              | adrpl12      | base     | imm(offset) == 0(nullptr) ? literal : NA
 *              | movrr        | base     | boi
 *              | movri        | base     | NA
 *              | extend/lsl   | base     | NA
 * =============================================================================
 * borx         | addrri       | offset   | NA
 * (noextend)   | addrrr       | offset   | NA
 *              | adrpl12      | offset   | NA
 *              | movrr        | offset   | borx
 *              | movri        | offset   | bori
 *              | extend/lsl   | offset   | borx(with extend)
 * =============================================================================
 * borx         | addrri       | extend   | NA
 * (extend)     | addrrr       | extend   | NA
 *              | adrpl12      | extend   | NA
 *              | movrr        | extend   | borx
 *              | movri        | extend   | NA
 *              | extend/lsl   | extend   | borx(with extend)
 * =============================================================================
 */
AArch64MemOperand *AArch64StoreLoadOpt::SelectReplaceMem(Insn &defInsn, RegOperand &base, Operand *offset) {
  AArch64MemOperand *newMemOpnd = nullptr;
  MOperator opCode = defInsn.GetMachineOpcode();
  AArch64RegOperand *newBase = static_cast<AArch64RegOperand*>(&defInsn.GetOperand(kInsnSecondOpnd));
  switch (opCode) {
    case MOP_xaddrri12:
    case MOP_waddrri12: {
      if (propMode == kPropBase) {
        AArch64ImmOperand *imm = static_cast<AArch64ImmOperand*>(&defInsn.GetOperand(kInsnThirdOpnd));
        int64 val = imm->GetValue();
        AArch64OfstOperand *newOffset = nullptr;
        if (offset == nullptr) {
          newOffset = cgFunc.GetMemoryPool()->New<AArch64OfstOperand>(imm->GetValue(), k32BitSize);
        } else {
          AArch64OfstOperand *ofst = static_cast<AArch64OfstOperand*>(offset);
          val += ofst->GetValue();
          newOffset = cgFunc.GetMemoryPool()->New<AArch64OfstOperand>(val, k32BitSize);
        }
        CHECK_FATAL(newOffset != nullptr, "newOffset is null!");
        newMemOpnd = cgFunc.GetMemoryPool()->New<AArch64MemOperand>(
            AArch64MemOperand::kAddrModeBOi, k64BitSize, *newBase, nullptr, newOffset, nullptr);
      }
      break;
    }
    case MOP_xaddrrr:
    case MOP_waddrrr:
    case MOP_dadd:
    case MOP_sadd: {
      if (propMode == kPropBase) {
        OfstOperand *ofstOpnd = static_cast<OfstOperand*>(offset);
        if (!ofstOpnd->IsZero()) {
          break;
        }
        AArch64RegOperand *newOffset = static_cast<AArch64RegOperand*>(&defInsn.GetOperand(kInsnThirdOpnd));
        CHECK_FATAL(newOffset != nullptr, "newOffset is null!");
        newMemOpnd = cgFunc.GetMemoryPool()->New<AArch64MemOperand>(
            AArch64MemOperand::kAddrModeBOrX, k64BitSize, *newBase, newOffset, nullptr, nullptr);
      }
      break;
    }
    case MOP_xadrpl12: {
      if (propMode == kPropBase) {
        OfstOperand *ofstOpnd = static_cast<OfstOperand*>(offset);
        CHECK_FATAL(ofstOpnd != nullptr, "oldOffset is null!");
        int64 val = ofstOpnd->GetValue();
        StImmOperand *offset1 = static_cast<StImmOperand*>(&defInsn.GetOperand(kInsnThirdOpnd));
        CHECK_FATAL(offset1 != nullptr, "offset1 is null!");
        val += offset1->GetOffset();
        AArch64OfstOperand *newOfsetOpnd = cgFunc.GetMemoryPool()->New<AArch64OfstOperand>(val, k32BitSize);
        CHECK_FATAL(newOfsetOpnd != nullptr, "newOfsetOpnd is null!");
        const MIRSymbol *addr = offset1->GetSymbol();
        newMemOpnd = cgFunc.GetMemoryPool()->New<AArch64MemOperand>(
            AArch64MemOperand::kAddrModeLo12Li, k64BitSize, *newBase, nullptr, newOfsetOpnd, addr);
      }
      break;
    }
    case MOP_xmovrr:
    case MOP_wmovrr: {
      if (propMode == kPropBase) {
        AArch64OfstOperand *offsetTmp = static_cast<AArch64OfstOperand*>(offset);
        CHECK_FATAL(offsetTmp != nullptr, "newOffset is null!");
        newMemOpnd = cgFunc.GetMemoryPool()->New<AArch64MemOperand>(
            AArch64MemOperand::kAddrModeBOi, k64BitSize, *newBase, nullptr, offsetTmp, nullptr);
      } else if (propMode == kPropOffset) {
        newMemOpnd = cgFunc.GetMemoryPool()->New<AArch64MemOperand>(
            AArch64MemOperand::kAddrModeBOrX, k64BitSize, base, *newBase, amount);
      } else if (propMode == kPropSignedExtend) {
        newMemOpnd = cgFunc.GetMemoryPool()->New<AArch64MemOperand>(
            AArch64MemOperand::kAddrModeBOrX, k64BitSize, base, *newBase, amount, true);
      } else {
        newMemOpnd = cgFunc.GetMemoryPool()->New<AArch64MemOperand>(
            AArch64MemOperand::kAddrModeBOrX, k64BitSize, base, *newBase, amount);
      }
      break;
    }
    case MOP_xmovri32:
    case MOP_xmovri64: {
      if (propMode == kPropOffset) {
        AArch64ImmOperand *imm = static_cast<AArch64ImmOperand*>(&defInsn.GetOperand(kInsnSecondOpnd));
        AArch64OfstOperand *newOffset = cgFunc.GetMemoryPool()->New<AArch64OfstOperand>(imm->GetValue(), k32BitSize);
        CHECK_FATAL(newOffset != nullptr, "newOffset is null!");
        newMemOpnd = cgFunc.GetMemoryPool()->New<AArch64MemOperand>(
            AArch64MemOperand::kAddrModeBOi, k64BitSize, base, nullptr, newOffset, nullptr);
      }
      break;
    }
    case MOP_xlslrri6:
    case MOP_wlslrri5: {
      AArch64ImmOperand *imm = static_cast<AArch64ImmOperand*>(&defInsn.GetOperand(kInsnThirdOpnd));
      AArch64RegOperand *newOffset = static_cast<AArch64RegOperand*>(&defInsn.GetOperand(kInsnSecondOpnd));
      CHECK_FATAL(newOffset != nullptr, "newOffset is null!");
      int64 shift = imm->GetValue();
      if (propMode == kPropOffset) {
        if ((shift < k4ByteSize) && (shift >= 0)) {
          newMemOpnd = cgFunc.GetMemoryPool()->New<AArch64MemOperand>(
              AArch64MemOperand::kAddrModeBOrX, k64BitSize, base, *newOffset, shift);
        }
      } else if (propMode == kPropShift) {
        shift += amount;
        if ((shift < k4ByteSize) && (shift >= 0)) {
          newMemOpnd = cgFunc.GetMemoryPool()->New<AArch64MemOperand>(
              AArch64MemOperand::kAddrModeBOrX, k64BitSize, base, *newOffset, shift);
        }
      }
      break;
    }
    case MOP_xsxtw64: {
      newMemOpnd = SelectReplaceExt(defInsn, base, true);
      break;
    }
    case MOP_xuxtw64: {
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
  AArch64MemOperand *newMemOpnd = SelectReplaceMem(*regDefInsn, base, offset);
  if (newMemOpnd == nullptr) {
    return false;
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

bool AArch64StoreLoadOpt::CanDoMemProp(Insn *insn) {
  if (!cgFunc.GetMirModule().IsCModule()) {
    return false;
  }
  if (!insn->IsMachineInstruction()) {
    return false;
  }

  if (insn->IsLoad() || insn->IsStore()) {
    if (insn->IsAtomic()) {
      return false;
    }
    AArch64MemOperand *currMemOpnd = static_cast<AArch64MemOperand*>(insn->GetMemOpnd());
    return currMemOpnd != nullptr;
  }
  return false;
}

void AArch64StoreLoadOpt::SelectPropMode(AArch64MemOperand &currMemOpnd) {
  AArch64MemOperand::AArch64AddressingMode currAddrMode = currMemOpnd.GetAddrMode();
  switch (currAddrMode) {
    case AArch64MemOperand::kAddrModeBOi:
      propMode = kPropBase;
      break;
    case AArch64MemOperand::kAddrModeBOrX:
      propMode = kPropOffset;
      amount = currMemOpnd.ShiftAmount();
      if (currMemOpnd.GetExtendAsString() == "LSL") {
        if (amount != 0) {
          propMode = kPropShift;
        }
        break;
      } else if (currMemOpnd.SignedExtend()) {
        propMode = kPropSignedExtend;
      } else if (currMemOpnd.UnsignedExtend()) {
        propMode = kPropUnsignedExtend;
      }
      break;
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
 *   add x1, x2, #immVal1
 *   ...(no def of x2)
 *   ldr/str x0, [x1, #immVal2]
 *   ======>
 *   add x1, x2, #immVal1
 *   ...
 *   ldr/str x0, [x2, #(immVal1 + immVal2)]
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
  AArch64MemOperand *currMemOpnd = static_cast<AArch64MemOperand*>(insn.GetMemOpnd());
  SelectPropMode(*currMemOpnd);
  RegOperand *base = currMemOpnd->GetBaseRegister();
  Operand *offset = currMemOpnd->GetOffset();
  bool memReplaced = false;

  if (propMode == kUndef) {
    return;
  } else if (propMode == kPropBase) {
    OfstOperand *immOffset = static_cast<OfstOperand*>(offset);
    CHECK_FATAL(immOffset != nullptr, "immOffset is nullptr!");
    regno_t baseRegNo = base->GetRegisterNumber();
    memReplaced = ReplaceMemOpnd(insn, baseRegNo, *base, immOffset);
  } else {
    AArch64RegOperand *regOffset = static_cast<AArch64RegOperand*>(offset);
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

void AArch64StoreLoadOpt::ProcessStrPair(Insn &insn) {
  const short memIndex = 2;
  short regIndex = 0;
  Operand &opnd = insn.GetOperand(memIndex);
  auto &memOpnd = static_cast<AArch64MemOperand&>(opnd);
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
    auto &regOpnd = static_cast<RegOperand&>(insn.GetOperand(regIndex));
    if (regOpnd.IsZeroRegister()) {
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
  auto &memOpnd = static_cast<AArch64MemOperand&>(opnd);
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

  auto *regOpnd = static_cast<RegOperand*>(insn.GetOpnd(regIndex));
  CHECK_NULL_FATAL(regOpnd);
  if (regOpnd->IsZeroRegister()) {
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
