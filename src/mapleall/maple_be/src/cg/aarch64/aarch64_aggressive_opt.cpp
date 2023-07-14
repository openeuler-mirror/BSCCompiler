/*
* Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "aarch64_aggressive_opt.h"
#include "aarch64_cg.h"

namespace maplebe {
void AArch64CombineRedundantX16Opt::Run() {
  ResetInsnId();
  FOR_ALL_BB(bb, &aarFunc) {
    MemPool *localMp = memPoolCtrler.NewMemPool("combine redundant x16 in bb mempool", true);
    auto *localAlloc = new MapleAllocator(localMp);
    InitSegmentInfo(localMp, localAlloc);
    FOR_BB_INSNS(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      if (insn->GetMachineOpcode() == MOP_c_counter) {
        ASSERT(bb->GetFirstInsn() == insn, "invalid pgo counter-insn");
        continue;
      }
      if (HasUseOpndReDef(*insn)) {
        hasUseOpndReDef = true;
      }
      bool hasX16Def = HasX16Def(*insn);
      if (IsEndOfSegment(*insn, hasX16Def)) {
        // End of segment because it is not an add split instruction but def x16, need to clear previous segment infos
        if (segmentInfo->segUseInfos->size() > 1) {
          FindCommonX16DefInsns(localMp, localAlloc);
          CombineRedundantX16DefInsns(*bb);
        }
        if (hasX16Def) {
          if (HasX16Use(*insn)) {
            (void)recentX16DefPrevInsns->emplace_back(recentX16DefInsn);
          }
          recentSplitUseOpnd = GetAddUseOpnd(*insn);
          recentX16DefInsn = insn;
        }
        ClearCurrentSegmentInfos();
        continue;
      }
      if (hasX16Def) {
        if (isX16Used) {
          recentX16DefPrevInsns->clear();
        }
        RecordRecentSplitInsnInfo(*insn);
        isX16Used = false;
        continue;
      }
      if (!IsUseX16MemInsn(*insn)) {
        continue;
      }
      ComputeRecentAddImm();
      RecordUseX16InsnInfo(*insn, localMp, localAlloc);
    }
    if (segmentInfo->segUseInfos->size() > 1) {
      FindCommonX16DefInsns(localMp, localAlloc);
      CombineRedundantX16DefInsns(*bb);
    }
    ClearSegmentInfo(localMp, localAlloc);
  }
}

void AArch64CombineRedundantX16Opt::ResetInsnId() {
  uint32 id = 0;
  FOR_ALL_BB(bb, &aarFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (insn->IsMachineInstruction()) {
        insn->SetId(id++);
      }
    }
  }
}

bool AArch64CombineRedundantX16Opt::IsEndOfSegment(const Insn &insn, bool hasX16Def) {
  if (insn.IsCall() || (IsUseX16MemInsn(insn) && (recentSplitUseOpnd == nullptr || isSpecialX16Def))) {
    clearX16Def = true;
    return true;
  }
  if (!hasX16Def) {
    return false;
  }
  MOperator curMop = insn.GetMachineOpcode();
  if (curMop != MOP_waddrri12 && curMop != MOP_xaddrri12 && curMop != MOP_waddrri24 && curMop != MOP_xaddrri24 &&
      curMop != MOP_waddrrr && curMop != MOP_xaddrrr && curMop != MOP_wmovri32 && curMop != MOP_xmovri64 &&
      curMop != MOP_wmovkri16 && curMop != MOP_xmovkri16 && curMop != MOP_wmovzri16 && curMop != MOP_xmovzri16) {
    clearX16Def = true;
    return true;
  }
  RegOperand *useOpnd = nullptr;
  if (curMop != MOP_wmovri32 && curMop != MOP_xmovri64 && curMop != MOP_wmovkri16 && curMop != MOP_xmovkri16 &&
      curMop != MOP_wmovzri16 && curMop != MOP_xmovzri16) {
    if (curMop == MOP_waddrrr || curMop == MOP_xaddrrr) {
      auto &srcOpnd1 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
      auto &srcOpnd2 = static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd));
      if (srcOpnd1.GetRegisterNumber() != R16 && srcOpnd2.GetRegisterNumber() != R16) {
        return true;
      } else if (srcOpnd1.GetRegisterNumber() == R16) {
        useOpnd = &srcOpnd2;
      } else if (srcOpnd2.GetRegisterNumber() == R16) {
        useOpnd = &srcOpnd1;
      } else {
        CHECK_FATAL(false, "cannot run here");
      }
    } else {
      useOpnd = &static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
    }
    CHECK_FATAL(useOpnd != nullptr, "get useOpnd failed");
    if (recentSplitUseOpnd != nullptr && useOpnd->GetRegisterNumber() != recentSplitUseOpnd->GetRegisterNumber() &&
        useOpnd->GetRegisterNumber() != R16) {
      return true;
    }
  }
  if (hasX16Def && useOpnd != nullptr && useOpnd->GetRegisterNumber() != R16 && hasUseOpndReDef) {
    hasUseOpndReDef = false;
    return true;
  }
  return false;
}

void AArch64CombineRedundantX16Opt::ComputeRecentAddImm() {
  CHECK_FATAL(recentSplitUseOpnd != nullptr, "invalid srcOpnd");
  CHECK_FATAL(recentX16DefInsn != nullptr || !recentX16DefPrevInsns->empty(), "missing x16 def instruction");
  int64 curAddImm = 0;
  if (recentX16DefInsn != nullptr) {
    MOperator recentMop = recentX16DefInsn->GetMachineOpcode();
    CHECK_FATAL(recentMop != MOP_wmovri32 && recentMop != MOP_xmovri64, "check this recent x16-def insn");
    int64 immVal = GetAddImmValue(*recentX16DefInsn);
    /*
     * Process the following case:
     * add R16, R0, #2, LSL #12   (recentAddPrevInsn)
     * add R16, R16, #1536        (recentX16DefInsn)
     * ||
     * add R16, R29, #28672       (recentAddPrevInsn)
     * add R16, R16, #20480       (recentX16DefInsn)
     * ||
     * add R16, R29, #61440       (recentAddPrevInsn)
     * add R16, R16, #4, LSL #12  (recentAddPrevInsn)
     * add R16, R16, #2048        (recentX16DefInsn)
     * ||
     * mov R16, #-508
     * add R16, R1, R16
     * ||
     * mov R16, #0
     * ldr (use)
     */
    if (HasX16Use(*recentX16DefInsn)) {
      CHECK_FATAL(!recentX16DefPrevInsns->empty(), "missing addLsl instructions");
      for (uint32 i = 0; i < recentX16DefPrevInsns->size(); ++i) {
        ASSERT((*recentX16DefPrevInsns)[i] != nullptr, "invalid prev x16 def insns");
        curAddImm += GetAddImmValue(*(*recentX16DefPrevInsns)[i]);
      }
      curAddImm += immVal;
    } else {
      recentX16DefPrevInsns->clear();
      curAddImm = immVal;
    }
  }
  if (recentAddImm != 0 && curAddImm != recentAddImm) {
    isSameAddImm = false;
  }
  recentAddImm = curAddImm;
}

void AArch64CombineRedundantX16Opt::RecordRecentSplitInsnInfo(Insn &insn) {
  // Compute and record the addImm until it is used
  MOperator curMop = insn.GetMachineOpcode();
  // Special x16 Def
  if (curMop == MOP_wmovkri16 || curMop == MOP_xmovkri16 || curMop == MOP_wmovzri16 || curMop == MOP_xmovzri16) {
    isSpecialX16Def = true;
    return;
  }
  if (isSpecialX16Def) {
    return;
  }
  if (curMop != MOP_waddrri24 && curMop != MOP_xaddrri24 && curMop != MOP_waddrri12 && curMop != MOP_xaddrri12 &&
      curMop != MOP_waddrrr && curMop != MOP_xaddrrr && curMop != MOP_wmovri32 && curMop != MOP_xmovri64) {
    return;
  }
  if (curMop != MOP_wmovri32 && curMop != MOP_xmovri64) {
    if (curMop == MOP_waddrrr || curMop == MOP_xaddrrr) {
      auto &srcOpnd1 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
      auto &srcOpnd2 = static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd));
      if (srcOpnd1.GetRegisterNumber() == R16 || srcOpnd2.GetRegisterNumber() == R16) {
        (void)recentX16DefPrevInsns->emplace_back(recentX16DefInsn);
        recentSplitUseOpnd = (srcOpnd1.GetRegisterNumber() == R16 ? &srcOpnd2 : &srcOpnd1);
      } else {
        CHECK_FATAL(false, "cannot run here");
      }
    } else {
      auto &useOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
      if (useOpnd.GetRegisterNumber() == R16) {
        (void)recentX16DefPrevInsns->emplace_back(recentX16DefInsn);
      } else {
        recentSplitUseOpnd = &useOpnd;
      }
    }
  }
  recentX16DefInsn = &insn;
}

bool AArch64CombineRedundantX16Opt::IsUseX16MemInsn(const Insn &insn) const {
  if ((!insn.IsLoadStorePair() && !insn.IsLoad() && !insn.IsStore()) || insn.IsAtomic()) {
    return false;
  }
  const InsnDesc *md = insn.GetDesc();
  if (insn.IsLoadLabel() || md->IsLoadAddress()) {
    return false;
  }
  uint32 memOpndIdx = GetMemOperandIdx(insn);
  auto &memOpnd = static_cast<MemOperand&>(insn.GetOperand(memOpndIdx));
  if (memOpnd.IsPostIndexed() || memOpnd.IsPreIndexed()) {
    return false;
  }
  RegOperand *baseOpnd = memOpnd.GetBaseRegister();
  if (baseOpnd == nullptr || baseOpnd->GetRegisterNumber() != R16) {
    return false;
  }
  CHECK_FATAL(memOpnd.GetAddrMode() == MemOperand::kBOI || memOpnd.GetAddrMode() == MemOperand::kLo12Li,
              "invalid mem instruction which uses x16");
  return true;
}

void AArch64CombineRedundantX16Opt::RecordUseX16InsnInfo(Insn &insn, MemPool *tmpMp, MapleAllocator *tmpAlloc) {
  uint32 memOpndIdx = GetMemOperandIdx(insn);
  auto &memOpnd = static_cast<MemOperand&>(insn.GetOperand(memOpndIdx));
  OfstOperand *ofstOpnd = memOpnd.GetOffsetImmediate();
  auto *x16UseInfo = tmpMp->New<UseX16InsnInfo>();
  x16UseInfo->memInsn = &insn;
  x16UseInfo->addPrevInsns = tmpMp->New<MapleVector<Insn*>>(tmpAlloc->Adapter());
  x16UseInfo->InsertAddPrevInsns(*recentX16DefPrevInsns);
  x16UseInfo->addInsn = recentX16DefInsn;
  x16UseInfo->curAddImm = recentAddImm;
  x16UseInfo->curOfst = (ofstOpnd == nullptr ? 0 : ofstOpnd->GetOffsetValue());
  x16UseInfo->originalOfst = x16UseInfo->curOfst + x16UseInfo->curAddImm;
  /* Get memSize from md */
  x16UseInfo->memSize = GetMemSizeFromMD(insn);
  ComputeValidAddImmInterval(*x16UseInfo, insn.IsLoadStorePair());
  (void)segmentInfo->segUseInfos->emplace_back(x16UseInfo);
  isX16Used = true;
}

void AArch64CombineRedundantX16Opt::ComputeValidAddImmInterval(UseX16InsnInfo &x16UseInfo, bool isPair) {
  uint32 memSize = x16UseInfo.memSize;
  // get valid offset interval: min, max
  int64 minValidOfst = INT_MAX;
  int64 maxValidOfst = INT_MIN;
  switch (memSize) {
    case maplebe::k8BitSize: {
      CHECK_FATAL(!isPair, "invalid memSize in load/store pair");
      minValidOfst = static_cast<int64>(maplebe::k0BitSizeInt);
      maxValidOfst = static_cast<int64>(kMaxPimm8);
      break;
    }
    case maplebe::k16BitSize: {
      CHECK_FATAL(!isPair, "invalid memSize in load/store pair");
      minValidOfst = static_cast<int64>(maplebe::k0BitSizeInt);
      maxValidOfst = static_cast<int64>(kMaxPimm16);
      break;
    }
    case maplebe::k32BitSize: {
      minValidOfst = (isPair ? static_cast<int64>(kMinSimm32) : static_cast<int64>(maplebe::k0BitSizeInt));
      maxValidOfst = (isPair ? static_cast<int64>(kMaxSimm32Pair) : static_cast<int64>(kMaxPimm32));
      break;
    }
    case maplebe::k64BitSize: {
      minValidOfst = (isPair ? static_cast<int64>(kMinSimm64) : static_cast<int64>(maplebe::k0BitSizeInt));
      maxValidOfst = (isPair ? static_cast<int64>(kMaxSimm64Pair) : static_cast<int64>(kMaxPimm64));
      break;
    }
    case maplebe::k128BitSize: {
      minValidOfst = (isPair ? static_cast<int64>(kMinSimm128Pair) : static_cast<int64>(maplebe::k0BitSizeInt));
      maxValidOfst = (isPair ? static_cast<int64>(kMaxSimm128Pair) : static_cast<int64>(kMaxPimm128));
      break;
    }
    default:
      CHECK_FATAL(false, "invalid memSize");
  }
  CHECK_FATAL(minValidOfst != INT_MIN && maxValidOfst != INT_MAX, "invalid interval of valid offset");

  x16UseInfo.minValidAddImm = x16UseInfo.originalOfst - maxValidOfst;
  x16UseInfo.maxValidAddImm = x16UseInfo.originalOfst - minValidOfst;
}

void AArch64CombineRedundantX16Opt::FindCommonX16DefInsns(MemPool *tmpMp, MapleAllocator *tmpAlloc) const {
  if (isSameAddImm) {
    ProcessSameAddImmCombineInfo(tmpMp, tmpAlloc);
  } else {
    ProcessIntervalIntersectionCombineInfo(tmpMp, tmpAlloc);
  }
}

void AArch64CombineRedundantX16Opt::ProcessSameAddImmCombineInfo(MemPool *tmpMp, MapleAllocator *tmpAlloc) const {
  CHECK_FATAL(recentSplitUseOpnd != nullptr && recentAddImm != 0, "find split insn info failed");
  auto *newCombineInfo = tmpMp->New<CombineInfo>();
  newCombineInfo->combineAddImm = recentAddImm;
  newCombineInfo->addUseOpnd = recentSplitUseOpnd;
  newCombineInfo->combineUseInfos = tmpMp->New<MapleVector<UseX16InsnInfo*>>(tmpAlloc->Adapter());
  newCombineInfo->combineUseInfos = segmentInfo->segUseInfos;
  (void)segmentInfo->segCombineInfos->emplace_back(newCombineInfo);
}

void AArch64CombineRedundantX16Opt::ProcessIntervalIntersectionCombineInfo(MemPool *tmpMp,
                                                                           MapleAllocator *tmpAlloc) const {
  ASSERT(segmentInfo->segUseInfos != nullptr, "uninitialized useInsnInfos");
  if (segmentInfo->segUseInfos->empty()) {
    return;
  }
  int64 minInv = (*segmentInfo->segUseInfos)[0]->minValidAddImm;
  int64 maxInv = (*segmentInfo->segUseInfos)[0]->maxValidAddImm;
  uint32 startIdx = 0;
  for (uint32 i = 1; i < segmentInfo->segUseInfos->size(); ++i) {
    int64 curMin = (*segmentInfo->segUseInfos)[i]->minValidAddImm;
    int64 curMax = (*segmentInfo->segUseInfos)[i]->maxValidAddImm;
    if (maxInv < curMin || curMax < minInv) {
      // There's no intersection, subsequent insns cannot share one add-insn with previous insns
      CombineInfo *combineInfo = CreateCombineInfo(minInv, startIdx, i - 1, *tmpMp, tmpAlloc);
      (void)segmentInfo->segCombineInfos->emplace_back(combineInfo);
      minInv = curMin;
      maxInv = curMax;
      startIdx = i;
      continue;
    }
    // Preserve valid addImm for all insns in [startIdx, i-1]
    int64 prevMinInv = minInv;
    // Compute interval intersection
    minInv = std::max(minInv, curMin);
    maxInv = std::min(maxInv, curMax);

    bool findCommonImm = false;
    for (int64 k = minInv; k <= maxInv; ++k) {
      bool isAllPrevInsnsValid = true;
      for (uint32 j = startIdx; j <= i; ++j) {
        UseX16InsnInfo *useInsnInfo = (*segmentInfo->segUseInfos)[j];
        int64 origOfst = useInsnInfo->originalOfst;
        uint32 memSize = GetMemSizeFromMD(*useInsnInfo->memInsn);
        if (!IsImmValidWithMemSize(memSize, origOfst - k)) {
          isAllPrevInsnsValid = false;
          break;
        }
      }
      if (isAllPrevInsnsValid) {
        findCommonImm = true;
        minInv = k;
        break;
      }
    }
    if (!findCommonImm) {
      CombineInfo *combineInfo = CreateCombineInfo(prevMinInv, startIdx, i - 1, *tmpMp, tmpAlloc);
      startIdx = i;
      (void)segmentInfo->segCombineInfos->emplace_back(combineInfo);
    }
  }
  CombineInfo *combineInfo = CreateCombineInfo(minInv, startIdx,
                                               static_cast<uint32>(segmentInfo->segUseInfos->size() - 1), *tmpMp,
                                               tmpAlloc);
  (void)segmentInfo->segCombineInfos->emplace_back(combineInfo);
}

void AArch64CombineRedundantX16Opt::CombineRedundantX16DefInsns(BB &bb) {
  MapleVector<CombineInfo*> *combineInfos = segmentInfo->segCombineInfos;
  if (combineInfos->empty()) {
    return;
  }
  for (uint32 i = 0; i < combineInfos->size(); ++i) {
    CombineInfo *combineInfo = (*combineInfos)[i];
    if (combineInfo->combineUseInfos->size() <= 1) {
      continue;
    }
    UseX16InsnInfo *firstInsnInfo = (*combineInfo->combineUseInfos)[0];
    auto &oldImmOpnd = static_cast<ImmOperand&>(firstInsnInfo->addInsn->GetOperand(kInsnThirdOpnd));
    auto &commonAddImmOpnd = aarFunc.CreateImmOperand(
        combineInfo->combineAddImm, oldImmOpnd.GetSize(), oldImmOpnd.IsSignedValue());
    uint32 size = combineInfo->addUseOpnd->GetSize();
    aarFunc.SelectAddAfterInsnBySize(firstInsnInfo->addInsn->GetOperand(kInsnFirstOpnd), *combineInfo->addUseOpnd,
                                     commonAddImmOpnd, size, false, *firstInsnInfo->addInsn);
    for (auto *useInfo : *combineInfo->combineUseInfos) {
      bb.RemoveInsn(*useInfo->addInsn);
      for (auto *insn : *useInfo->addPrevInsns) {
        bb.RemoveInsn(*insn);
      }
      Insn *memInsn = useInfo->memInsn;
      uint32 memOpndIdx = GetMemOperandIdx(*memInsn);
      auto &memOpnd = static_cast<MemOperand&>(memInsn->GetOperand(memOpndIdx));
      OfstOperand &newOfstOpnd = aarFunc.CreateOfstOpnd(static_cast<uint64>(useInfo->originalOfst -
                                                        combineInfo->combineAddImm),
                                                        memOpnd.GetOffsetImmediate()->GetSize());
      memOpnd.SetOffsetOperand(newOfstOpnd);
      CHECK_FATAL(aarFunc.IsOperandImmValid(memInsn->GetMachineOpcode(), &memOpnd, memOpndIdx),
                  "invalid offset after combine split add insns");
    }
  }
}

bool AArch64CombineRedundantX16Opt::HasX16Def(const Insn &insn) const {
  for (uint32 defRegNo : insn.GetDefRegs()) {
    if (defRegNo == R16) {
      return true;
    }
  }
  return false;
}

bool AArch64CombineRedundantX16Opt::HasUseOpndReDef(const Insn &insn) const {
  for (uint32 defRegNo : insn.GetDefRegs()) {
    if (recentSplitUseOpnd != nullptr && defRegNo == recentSplitUseOpnd->GetRegisterNumber()) {
      return true;
    }
  }
  return false;
}

bool AArch64CombineRedundantX16Opt::HasX16Use(const Insn &insn) const{
  MOperator mop = insn.GetMachineOpcode();
  if (mop == MOP_wmovri32 || mop == MOP_xmovri64) {
    return false;
  } else if (mop == MOP_waddrrr || mop == MOP_xaddrrr) {
    auto &srcOpnd1 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
    auto &srcOpnd2 = static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd));
    if (srcOpnd1.GetRegisterNumber() == R16 || srcOpnd2.GetRegisterNumber() == R16) {
      return true;
    }
  } else {
    auto &useOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
    if (useOpnd.GetRegisterNumber() == R16) {
      return true;
    }
  }
  return false;
}

uint32 AArch64CombineRedundantX16Opt::GetMemSizeFromMD(const Insn &insn) const {
  const InsnDesc *md = &AArch64CG::kMd[insn.GetMachineOpcode()];
  ASSERT(md != nullptr, "get md failed");
  uint32 memOpndIdx = GetMemOperandIdx(insn);
  const OpndDesc *od = md->GetOpndDes(memOpndIdx);
  ASSERT(od != nullptr, "get od failed");
  return od->GetSize();
}

RegOperand *AArch64CombineRedundantX16Opt::GetAddUseOpnd(const Insn &insn) {
  MOperator curMop = insn.GetMachineOpcode();
  if (curMop != MOP_wmovri32 && curMop != MOP_xmovri64) {
    if (curMop == MOP_waddrrr || curMop == MOP_xaddrrr) {
      auto &srcOpnd1 = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
      auto &srcOpnd2 = static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd));
      if (srcOpnd1.GetRegisterNumber() != R16 && srcOpnd2.GetRegisterNumber() != R16) {
        return nullptr;
      } else if (srcOpnd1.GetRegisterNumber() == R16) {
        return &srcOpnd2;
      } else if (srcOpnd2.GetRegisterNumber() == R16) {
        return &srcOpnd1;
      } else {
        CHECK_FATAL(false, "cannot run here");
      }
    } else {
      return &static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
    }
  }
  return nullptr;
}

uint32 AArch64CombineRedundantX16Opt::GetMemOperandIdx(const Insn &insn) const {
  if (insn.IsLoadStorePair()) {
    return kInsnThirdOpnd;
  } else {
    return kInsnSecondOpnd;
  }
}

int64 AArch64CombineRedundantX16Opt::GetAddImmValue(const Insn &insn) const {
  MOperator mop = insn.GetMachineOpcode();
  if (mop == MOP_waddrri24 || mop == MOP_xaddrri24) {
    auto &shiftOpnd = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
    auto &amountOpnd = static_cast<BitShiftOperand&>(insn.GetOperand(kInsnFourthOpnd));
    return static_cast<int64>(static_cast<uint64>(shiftOpnd.GetValue()) << amountOpnd.GetShiftAmount());
  } else if (mop == MOP_waddrri12 || mop == MOP_xaddrri12) {
    auto &immOpnd = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
    return immOpnd.GetValue();
  } else if (mop == MOP_wmovri32 || mop == MOP_xmovri64) {
    auto &immOpnd = static_cast<ImmOperand&>(insn.GetOperand(kInsnSecondOpnd));
    return immOpnd.GetValue();
  } else if (mop == MOP_waddrrr || mop == MOP_xaddrrr) {
    return 0;
  } else {
    CHECK_FATAL(false, "cannot run here");
  }
}

AArch64CombineRedundantX16Opt::CombineInfo *AArch64CombineRedundantX16Opt::CreateCombineInfo(
    int64 addImm, uint32 startIdx, uint32 endIdx, MemPool &tmpMp, MapleAllocator *tmpAlloc) const {
  auto *combineInfo = tmpMp.New<CombineInfo>();
  combineInfo->combineAddImm = addImm;
  combineInfo->addUseOpnd = recentSplitUseOpnd;
  combineInfo->combineUseInfos = tmpMp.New<MapleVector<UseX16InsnInfo*>>(tmpAlloc->Adapter());
  for (uint32 j = startIdx; j <= endIdx; ++j) {
    (void)combineInfo->combineUseInfos->emplace_back((*segmentInfo->segUseInfos)[j]);
  }
  return combineInfo;
}

void AArch64CombineRedundantX16Opt::RoundInterval(int64 &minInv, int64 &maxInv, uint32 startIdx, uint32 endIdx) {
  // Compute max memSize
  uint32 maxMemSize = 0;
  for (uint32 i = startIdx; i <= endIdx; ++i) {
    UseX16InsnInfo *useInsnInfo = (*segmentInfo->segUseInfos)[i];
    maxMemSize = std::max(maxMemSize, useInsnInfo->memSize);
  }
  // Round interval
  if (maxMemSize == maplebe::k32BitSize) {
    minInv = (minInv % k4BitSizeInt != 0) ? (minInv + (k4BitSizeInt - minInv % k4BitSizeInt)) : minInv;
    maxInv = (maxInv % k4BitSizeInt != 0) ? (maxInv - maxInv % k4BitSizeInt) : maxInv;
  } else if (maxMemSize == maplebe::k64BitSize) {
    minInv = (minInv % k8BitSizeInt != 0) ? (minInv + (k8BitSizeInt - minInv % k8BitSizeInt)) : minInv;
    maxInv = (maxInv % k8BitSizeInt != 0) ? (maxInv - maxInv % k8BitSizeInt) : maxInv;
  } else if (maxMemSize == maplebe::k128BitSize) {
    minInv = (minInv % k16BitSizeInt != 0) ? (minInv + (k16BitSizeInt - minInv % k16BitSizeInt)) : minInv;
    maxInv = (maxInv % k16BitSizeInt != 0) ? (maxInv - maxInv % k16BitSizeInt) : maxInv;
  }
}

bool AArch64CombineRedundantX16Opt::IsImmValidWithMemSize(uint32 memSize, int64 imm) const {
  if ((memSize == maplebe::k32BitSize && imm % 4 == 0) ||
      (memSize == maplebe::k64BitSize && imm % 8 == 0) ||
      (memSize == maplebe::k128BitSize && imm % 16 == 0)) {
    return true;
  }
  return false;
}

void AArch64CombineRedundantX16Opt::InitSegmentInfo(MemPool *tmpMp, MapleAllocator *tmpAlloc) {
  CHECK_FATAL(tmpMp != nullptr, "get memPool failed");
  CHECK_FATAL(tmpAlloc != nullptr, "get allocator failed");
  segmentInfo = tmpMp->New<SegmentInfo>();
  segmentInfo->segUseInfos = tmpMp->New<MapleVector<UseX16InsnInfo*>>(tmpAlloc->Adapter());
  segmentInfo->segCombineInfos = tmpMp->New<MapleVector<CombineInfo*>>(tmpAlloc->Adapter());
  recentX16DefPrevInsns = tmpMp->New<MapleVector<Insn*>>(tmpAlloc->Adapter());
}

void AArch64CombineRedundantX16Opt::ClearSegmentInfo(MemPool *tmpMp, MapleAllocator *tmpAlloc) {
  CHECK_FATAL(tmpMp != nullptr, "get memPool failed");
  CHECK_FATAL(tmpAlloc != nullptr, "get allocator failed");
  segmentInfo = nullptr;
  recentX16DefPrevInsns = nullptr;
  delete tmpMp;
  delete tmpAlloc;

  recentX16DefInsn = nullptr;
  recentSplitUseOpnd = nullptr;
  recentAddImm = 0;
  isSameAddImm = true;
  clearX16Def = false;
  isX16Used = false;
  hasUseOpndReDef = false;
  isSpecialX16Def = false;
}

void AArch64AggressiveOpt::DoOpt() {
  if (!CGOptions::IsArm64ilp32()) {
    Optimize<AArch64CombineRedundantX16Opt>();
  }
}
} /* namespace maplebe */
