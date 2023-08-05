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
// The main process of AArch64CombineRedundantX16Opt
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
      // For the following case, useOpnd(x10) is redefined between two x16 definition points,
      // so the two x16s cannot be shared, they should be in two segments of combining optimization.
      // e.g.
      // add x16, x10, #1024
      // ...
      // lsl x10, x10, #4   (x10 is redefined)
      // ...
      // add x16, x10, #1024
      if (HasUseOpndReDef(*insn)) {
        hasUseOpndReDef = true;
      }
      bool hasX16Def = HasX16Def(*insn);
      // 1. The instruction ends the current segment.
      if (IsEndOfSegment(*insn, hasX16Def)) {
        ProcessAtEndOfSegment(*bb, *insn, hasX16Def, localMp, localAlloc);
        continue;
      }
      // 2. The instruction does not end the current segment, but def x16 for splitting, record def info.
      if (hasX16Def) {
        // Before the instruction, there has instructions that use the old x16 value
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
      // 3. The instruction does not end the current segment, but use x16, then we compute split immediate and
      //    record use info.
      ComputeRecentAddImm();
      RecordUseX16InsnInfo(*insn, localMp, localAlloc);
    }
    // Process the last segment in bb
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

// We divide instructions that can share x16 value into segments, when the following scenario occurs,
// the current segment ends, its all infos are needed to be cleared, and a new segment is started:
// (1) meet call instruction
// (2) meet special x16 def (not add/mov/movz/movk mops that split immediate)
// (3) meet useOpnd has been redefined
// e.g.
// add x16, x2, #3072        ----
// ldp x3, x4, [x16, #184]      |
// ...                          |
// add x16, x2, #3072           |
// ldp x3, x4, [x16, #200]   ----   =====>  the first common segment
//
// add x16, x1, #2048        ----
// ldp x5, x6, [x16, #8]        |
// ...                          |
// add x16, x1, #2048           |
// ldp x5, x6, [x16, #16]   ----   =====>  the second common segment
bool AArch64CombineRedundantX16Opt::IsEndOfSegment(const Insn &insn, bool hasX16Def) {
  if (insn.IsCall() || (IsUseX16MemInsn(insn) && (recentSplitUseOpnd == nullptr || isSpecialX16Def))) {
    return true;
  }
  if (!hasX16Def) {
    return false;
  }
  MOperator curMop = insn.GetMachineOpcode();
  if (curMop != MOP_waddrri12 && curMop != MOP_xaddrri12 && curMop != MOP_waddrri24 && curMop != MOP_xaddrri24 &&
      curMop != MOP_waddrrr && curMop != MOP_xaddrrr && curMop != MOP_wmovri32 && curMop != MOP_xmovri64 &&
      curMop != MOP_wmovkri16 && curMop != MOP_xmovkri16 && curMop != MOP_wmovzri16 && curMop != MOP_xmovzri16) {
    isIrrelevantX16Def = true;
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

void AArch64CombineRedundantX16Opt::ProcessAtEndOfSegment(BB &bb, Insn &insn, bool hasX16Def, MemPool *localMp,
                                                          MapleAllocator *localAlloc) {
  // If the current segment ends, we handle the x16 common optimization for the segment following
  if (segmentInfo->segUseInfos->size() > 1) {
    FindCommonX16DefInsns(localMp, localAlloc);
    CombineRedundantX16DefInsns(bb);
  }
  std::vector<Insn*> tmpRecentX16DefPrevInsns;
  tmpRecentX16DefPrevInsns.assign(recentX16DefPrevInsns->begin(), recentX16DefPrevInsns->end());
  // For the instructions that both use and define x16, we need to record x16 prev define insn
  // e.g.
  // add x16, sp, #1, LSL #12    ----> $recentX16DefInsn
  // add x16, x16, #512   ----> $insn
  bool hasX16Use = HasX16Use(insn);
  if (hasX16Use) {
    (void)tmpRecentX16DefPrevInsns.emplace_back(recentX16DefInsn);
  }
  // Clear old segment infos
  ClearCurrentSegmentInfos();
  // Record new segment infos:
  // if the current segment ends and the instruction defines x16 for splitting, we record data infos following
  if (hasX16Def && !isIrrelevantX16Def) {
    recentSplitUseOpnd = GetAddUseOpnd(insn);
    recentX16DefInsn = &insn;
    if (hasX16Use) {
      recentX16DefPrevInsns->assign(tmpRecentX16DefPrevInsns.begin(), tmpRecentX16DefPrevInsns.end());
    }
  }
  // Clear isIrrelevantX16Def after using
  isIrrelevantX16Def = false;
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
  RegOperand *baseOpnd = memOpnd.GetBaseRegister();
  if (baseOpnd == nullptr || baseOpnd->GetRegisterNumber() != R16) {
    return false;
  }
  // We are not allowed pre-index/post-index memory instruction that use x16 to split offset, because
  // it implicitly redefines the x16 and it is difficult to the interval calculation and valid offset judgement,
  // so we do not combine x16 in AddSubMergeLdStPattern of cgpostpeephole.
  // e.g.
  // add x16, sp, #1, LSL #12
  // stp x27, x28, [x16, #16]!    --\->   it's wrong case
  // If such opportunities still exist after x16 combining optimization, add related opt in this phase.
  CHECK_FATAL(!memOpnd.IsPreIndexed() && !memOpnd.IsPostIndexed(), "dangerous insn that implicitly redefines x16");
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

// Find the common x16 def value for all use x16 instructions in the current segment:
// 1. If the split immediate is same, we can just use the value to combine;
// 2. else, we calculate the valid interval for each use x16 insns and find a valid immediate within the
//    interval that meets the multiple requirements for all use x16 insns.
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

// The interval algorithm to find a valid common split immediate.
// 1. |______| |______| |______|
//    interval disjoint, no common immediate;
// 2. |______|
//       |______|
//         |______|
//    Ideally, there shares the same immediate;
// 3. |______|          |______|
//        |______|          |______|
//    The first two insns can share one immediate, and the last two insns can share one different immediate;
// 4. |______|             (1)
//         |______|        (2)
//               |______|  (3)
//    (1)(2) share one immediate or (2)(3) share one immediate, we choose the first two
// After the above calculation, we get a valid interval, then we traverse the interval and check whether the
// offset meets the multiple requirement(e.g. <pimm>/4 or <pimm>/8 ...)
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
    commonAddImmOpnd.SetVary(oldImmOpnd.GetVary());
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

// 1. For recentX16DefInsn, the mop are in {movri, addrrr, addrri, addrrre}
// 2. For other insn, we need to check every operand of insn
bool AArch64CombineRedundantX16Opt::HasX16Use(const Insn &insn) const{
  uint32 opndNum = insn.GetOperandSize();
  for (uint32 i = 0; i < opndNum; ++i) {
    if (!insn.OpndIsUse(i)) {
      continue;
    }
    Operand &opnd = insn.GetOperand(i);
    switch (opnd.GetKind()) {
      case Operand::kOpdRegister: {
        if (static_cast<RegOperand&>(opnd).GetRegisterNumber() == R16) {
          return true;
        }
        break;
      }
      case Operand::kOpdMem: {
        RegOperand *baseOpnd = static_cast<MemOperand&>(opnd).GetBaseRegister();
        if (baseOpnd != nullptr && baseOpnd->GetRegisterNumber() == R16) {
          return true;
        }
        RegOperand *indexOpnd = static_cast<MemOperand&>(opnd).GetIndexRegister();
        if (indexOpnd != nullptr && indexOpnd->GetRegisterNumber() == R16) {
          return true;
        }
        break;
      }
      case Operand::kOpdList: {
        for (auto *opndElem : static_cast<ListOperand&>(opnd).GetOperands()) {
          if (opndElem->GetRegisterNumber() == R16) {
            return true;
          }
        }
        break;
      }
      default:
        continue;
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
  isX16Used = false;
  hasUseOpndReDef = false;
  isSpecialX16Def = false;
  isIrrelevantX16Def = false;
}

void AArch64AggressiveOpt::DoOpt() {
  if (!CGOptions::IsArm64ilp32()) {
    Optimize<AArch64CombineRedundantX16Opt>();
  }
}
} /* namespace maplebe */
