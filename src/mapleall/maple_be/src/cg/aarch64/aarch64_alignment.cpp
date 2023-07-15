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
#include <unordered_map>
#include "insn.h"
#include "loop.h"
#include "aarch64_cg.h"
#include "cg_option.h"
#include "cg_irbuilder.h"
#include "aarch64_alignment.h"

namespace maplebe {
void AArch64AlignAnalysis::FindLoopHeaderByDefault() {
  if (loopInfo.GetLoops().empty()) {
    return;
  }
  for (auto *loop : loopInfo.GetLoops()) {
    BB &header = loop->GetHeader();
    if (&header == aarFunc->GetFirstBB() || IsIncludeCall(header) || !IsInSizeRange(header)) {
      continue;
    }
    InsertLoopHeaderBBs(header);
  }
}

void AArch64AlignAnalysis::FindJumpTargetByDefault() {
  MapleUnorderedMap<LabelIdx, BB*> label2BBMap = aarFunc->GetLab2BBMap();
  if (label2BBMap.empty()) {
    return;
  }
  for (auto &iter : label2BBMap) {
    BB *jumpBB = iter.second;
    if (jumpBB == aarFunc->GetFirstBB() || !IsInSizeRange(*jumpBB) || HasFallthruEdge(*jumpBB)) {
      continue;
    }
    if (jumpBB != nullptr) {
      InsertJumpTargetBBs(*jumpBB);
    }
  }
}

bool AArch64AlignAnalysis::IsIncludeCall(BB &bb) {
  return bb.HasCall();
}

bool AArch64AlignAnalysis::IsInSizeRange(BB &bb) {
  uint64 size = 0;
  FOR_BB_INSNS_CONST(insn, &bb) {
    if (!insn->IsMachineInstruction() || insn->GetMachineOpcode() == MOP_pseudo_ret_int ||
        insn->GetMachineOpcode() == MOP_pseudo_ret_float) {
      continue;
    }
    size += kAlignInsnLength;
  }
  BB *curBB = &bb;
  while (curBB->GetNext() != nullptr && curBB->GetNext()->GetLabIdx() == 0) {
    FOR_BB_INSNS_CONST(insn, curBB->GetNext()) {
      if (!insn->IsMachineInstruction() || insn->GetMachineOpcode() == MOP_pseudo_ret_int ||
          insn->GetMachineOpcode() == MOP_pseudo_ret_float) {
        continue;
      }
      size += kAlignInsnLength;
    }
    curBB = curBB->GetNext();
  }
  AArch64AlignInfo targetInfo;
  if (CGOptions::GetAlignMinBBSize() == 0 || CGOptions::GetAlignMaxBBSize() == 0) {
    return false;
  }
  targetInfo.alignMinBBSize = (CGOptions::OptimizeForSize()) ? 16 : CGOptions::GetAlignMinBBSize();
  targetInfo.alignMaxBBSize = (CGOptions::OptimizeForSize()) ? 44 : CGOptions::GetAlignMaxBBSize();
  if (size <= targetInfo.alignMinBBSize || size >= targetInfo.alignMaxBBSize) {
    return false;
  }
  return true;
}

bool AArch64AlignAnalysis::HasFallthruEdge(BB &bb) {
  for (auto *iter : bb.GetPreds()) {
    if (iter == bb.GetPrev()) {
      return true;
    }
  }
  return false;
}

void AArch64AlignAnalysis::ComputeLoopAlign() {
  if (loopHeaderBBs.empty()) {
    return;
  }
  for (BB *bb : loopHeaderBBs) {
    bb->SetNeedAlign(true);
    if (CGOptions::GetLoopAlignPow() == 0) {
      return;
    }
    AArch64AlignInfo targetInfo;
    targetInfo.loopAlign = CGOptions::GetLoopAlignPow();
    if (alignInfos.find(bb) == alignInfos.end()) {
      alignInfos[bb] = targetInfo.loopAlign;
    } else {
      uint32 curPower = alignInfos[bb];
      alignInfos[bb] = (targetInfo.loopAlign < curPower) ? targetInfo.loopAlign : curPower;
    }
    bb->SetAlignPower(alignInfos[bb]);
  }
}

void AArch64AlignAnalysis::ComputeJumpAlign() {
  if (jumpTargetBBs.empty()) {
    return;
  }
  for (BB *bb : jumpTargetBBs) {
    bb->SetNeedAlign(true);
    if (CGOptions::GetJumpAlignPow() == 0) {
      return;
    }
    AArch64AlignInfo targetInfo;
    targetInfo.jumpAlign = (CGOptions::OptimizeForSize()) ? 3 : CGOptions::GetJumpAlignPow();
    if (alignInfos.find(bb) == alignInfos.end()) {
      alignInfos[bb] = targetInfo.jumpAlign;
    } else {
      uint32 curPower = alignInfos[bb];
      alignInfos[bb] = (targetInfo.jumpAlign < curPower) ? targetInfo.jumpAlign : curPower;
    }
    bb->SetAlignPower(alignInfos[bb]);
  }
}

uint32 AArch64AlignAnalysis::GetAlignRange(uint32 alignedVal, uint32 addr) const {
  if (addr == 0) {
    return addr;
  }
  uint32 range = (alignedVal - (((addr - 1) * kInsnSize) & (alignedVal - 1))) / kInsnSize - 1;
  return range;
}

bool AArch64AlignAnalysis::IsInSameAlignedRegion(uint32 addr1, uint32 addr2, uint32 alignedRegionSize) const {
  return (((addr1 - 1) * kInsnSize) / alignedRegionSize) == (((addr2 - 1) * kInsnSize) / alignedRegionSize);
}

uint32 AArch64AlignAnalysis::ComputeBBAlignNopNum(BB &bb, uint32 addr) const {
  uint32 alignedVal = (1U << bb.GetAlignPower());
  uint32 alignNopNum = GetAlignRange(alignedVal, addr);
  bb.SetAlignNopNum(alignNopNum);
  return alignNopNum;
}

uint64 AArch64AlignAnalysis::GetFreqThreshold() const {
  uint32 alignThreshold = CGOptions::GetAlignThreshold();
  if (alignThreshold < 1 || alignThreshold > 100) { // 1: min value, 100: max value
    alignThreshold = 100; // 100: default value
  }
  uint64 freqMax = 0;
  FOR_ALL_BB(bb, aarFunc) {
    if (bb != nullptr && bb->GetFrequency() > freqMax) {
      freqMax = bb->GetFrequency();
    }
  }

  return freqMax / alignThreshold;
}

uint64 AArch64AlignAnalysis::GetFallThruFreq(const BB &bb) const {
  uint64 fallthruFreq = 0;
  for (BB *pred : bb.GetPreds()) {
    if (pred == bb.GetPrev()) {
      fallthruFreq += pred->GetEdgeFreq(bb);
      break;
    }
  }
  return fallthruFreq;
}

uint64 AArch64AlignAnalysis::GetBranchFreq(const BB &bb) const {
  uint64 branchFreq = 0;
  for (BB *pred : bb.GetPreds()) {
    if (pred != bb.GetPrev()) {
      branchFreq += pred->GetEdgeFreq(bb);
    }
  }
  return branchFreq;
}

bool AArch64AlignAnalysis::MarkCondBranchAlign() {
  sameTargetBranches.clear();
  uint32 addr = 0;
  bool change = false;
  FOR_ALL_BB(bb, aarFunc) {
    if (bb != nullptr && bb->IsBBNeedAlign()) {
      addr += ComputeBBAlignNopNum(*bb, addr);
    }
    FOR_BB_INSNS(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      addr += insn->GetAtomicNum();
      MOperator mOp = insn->GetMachineOpcode();
      if ((mOp == MOP_wtbz || mOp == MOP_wtbnz || mOp == MOP_xtbz || mOp == MOP_xtbnz) && IsSplitInsn(*insn)) {
        ++addr;
      }
      if (!insn->IsCondBranch() || insn->GetOperandSize() == 0) {
        insn->SetAddress(addr);
        continue;
      }
      Operand &opnd = insn->GetOperand(insn->GetOperandSize() - 1);
      if (!opnd.IsLabelOpnd()) {
        insn->SetAddress(addr);
        continue;
      }
      LabelIdx targetIdx = static_cast<LabelOperand&>(opnd).GetLabelIndex();
      if (sameTargetBranches.find(targetIdx) == sameTargetBranches.end()) {
        sameTargetBranches[targetIdx] = addr;
        insn->SetAddress(addr);
        continue;
      }
      uint32 sameTargetAddr = sameTargetBranches[targetIdx];
      uint32 alignedRegionSize = 1 << kAlignRegionPower;
      /**
       * if two branches jump to the same target and their addresses are within an 16byte aligned region,
       * add a certain number of [nop] to move them out of the region.
       */
      if (IsInSameAlignedRegion(sameTargetAddr, addr, alignedRegionSize)) {
        uint32 nopNum = GetAlignRange(alignedRegionSize, addr) + 1;
        nopNum = nopNum > kAlignMaxNopNum ? 0 : nopNum;
        if (nopNum == 0) {
          break;
        }
        change = true;
        insn->SetNopNum(nopNum);
        for (uint32 i = 0; i < nopNum; i++) {
          addr += insn->GetAtomicNum();
        }
      } else {
        insn->SetNopNum(0);
      }
      sameTargetBranches[targetIdx] = addr;
      insn->SetAddress(addr);
    }
  }
  return change;
}

void AArch64AlignAnalysis::UpdateInsnId() {
  uint32 id = 0;
  FOR_ALL_BB(bb, aarFunc) {
    if (bb != nullptr && bb->IsBBNeedAlign()) {
      if (!(CGOptions::DoLoopAlign() && loopHeaderBBs.find(bb) != loopHeaderBBs.end())) {
        uint32 alignedVal = 1U << (bb->GetAlignPower());
        uint32 range = GetAlignRange(alignedVal, id);
        id = id + (range > kAlignPseudoSize ? range : kAlignPseudoSize);
      }
    }
    FOR_BB_INSNS(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      id += insn->GetAtomicNum();
      if (insn->IsCondBranch() && insn->GetNopNum() != 0) {
        id += insn->GetNopNum();
      }
      MOperator mOp = insn->GetMachineOpcode();
      if ((mOp == MOP_wtbz || mOp == MOP_wtbnz || mOp == MOP_xtbz || mOp == MOP_xtbnz) && IsSplitInsn(*insn)) {
        ++id;
      }
      insn->SetId(id);
      if (insn->GetMachineOpcode() == MOP_adrp_ldr && CGOptions::IsLazyBinding() && !aarFunc->GetCG()->IsLibcore()) {
        ++id;
      }
    }
  }
}

bool AArch64AlignAnalysis::MarkShortBranchSplit() {
  bool change = false;
  bool split;
  do {
    split = false;
    UpdateInsnId();
    for (auto *bb = aarFunc->GetFirstBB(); bb != nullptr && !split; bb = bb->GetNext()) {
      for (auto *insn = bb->GetLastInsn(); insn != nullptr && !split; insn = insn->GetPrev()) {
        if (!insn->IsMachineInstruction()) {
          continue;
        }
        MOperator mOp = insn->GetMachineOpcode();
        if (mOp != MOP_wtbz && mOp != MOP_wtbnz && mOp != MOP_xtbz && mOp != MOP_xtbnz) {
          continue;
        }
        if (IsSplitInsn(*insn)) {
          continue;
        }
        auto &labelOpnd = static_cast<LabelOperand&>(insn->GetOperand(kInsnThirdOpnd));
        if (aarFunc->DistanceCheck(*bb, labelOpnd.GetLabelIndex(), insn->GetId())) {
          continue;
        }
        split = true;
        change = true;
        MarkSplitInsn(*insn);
      }
    }
  } while (split);
  return change;
}

void AArch64AlignAnalysis::AddNopAfterMark() {
  FOR_ALL_BB(bb, aarFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (!insn->IsMachineInstruction() || !insn->IsCondBranch() || insn->GetNopNum() == 0) {
        continue;
      }
      /**
       * To minimize the performance loss of nop, we decided to place nop on an island before the current addr.
       * The island here is after [b, ret, br, blr].
       * To ensure correct insertion of the nop, the nop is inserted in the original position in the following cases:
       * 1. A branch with the same target exists before it.
       * 2. A branch whose nopNum value is not 0 exists before it.
       * 3. no BBs need to be aligned between the original location and the island.
       */
      std::unordered_map<LabelIdx, Insn*> targetCondBrs;
      bool findIsland = false;
      Insn *detect = insn->GetPrev();
      BB *region = bb;
      while (detect != nullptr || region != aarFunc->GetFirstBB()) {
        bool isBreak = false;
        while (detect == nullptr) {
          // If region's prev bb and detect both are nullptr, it should end the while loop.
          if (region->GetPrev() == nullptr) {
            isBreak = true;
            break;
          }
          region = region->GetPrev();
          detect = region->GetLastInsn();
        }
        if (isBreak) {
          break;
        }
        if (detect->GetMachineOpcode() == MOP_xuncond || detect->GetMachineOpcode() == MOP_xret ||
            detect->GetMachineOpcode() == MOP_xbr) {
          findIsland = true;
          break;
        }
        if (region->IsBBNeedAlign()) {
          break;
        }
        if (!detect->IsMachineInstruction() || !detect->IsCondBranch() || detect->GetOperandSize() == 0) {
          detect = detect->GetPrev();
          continue;
        }
        if (detect->GetNopNum() != 0) {
          break;
        }
        Operand &opnd = detect->GetOperand(detect->GetOperandSize() - 1);
        if (!opnd.IsLabelOpnd()) {
          detect = detect->GetPrev();
          continue;
        }
        LabelIdx targetIdx = static_cast<LabelOperand&>(opnd).GetLabelIndex();
        if (targetCondBrs.find(targetIdx) != targetCondBrs.end()) {
          break;
        }
        targetCondBrs[targetIdx] = detect;
        detect = detect->GetPrev();
      }
      uint32 nopNum = insn->GetNopNum();
      if (findIsland) {
        for (uint32 i = 0; i < nopNum; i++) {
          (void)bb->InsertInsnAfter(*detect, aarFunc->GetInsnBuilder()->BuildInsn<AArch64CG>(MOP_nop));
        }
      } else {
        for (uint32 i = 0; i < nopNum; i++) {
          (void)bb->InsertInsnBefore(*insn, aarFunc->GetInsnBuilder()->BuildInsn<AArch64CG>(MOP_nop));
        }
      }
    }
  }
}

/**
 * The insertion of nop affects the judgement of the addressing range of short branches,
 * and the splitting of short branches affects the calculation of the location and number of nop insertions.
 * In the iteration process of both, we only make some marks, wait for the fixed points, and fill in nop finally.
 */
void AArch64AlignAnalysis::ComputeCondBranchAlign() {
  bool condBrChange = false;
  bool shortBrChange = false;
  while (true) {
    condBrChange = MarkCondBranchAlign();
    if (!condBrChange) {
      break;
    }
    shortBrChange = MarkShortBranchSplit();
    if (!shortBrChange) {
      break;
    }
  }
  AddNopAfterMark();
}

uint32 AArch64AlignAnalysis::GetInlineAsmInsnNum(const Insn &insn) const {
  uint32 num = 0;
  MapleString asmStr = static_cast<StringOperand&>(insn.GetOperand(kAsmStringOpnd)).GetComment();
  // If there are multiple insns, add \n\t after each insn to separate them in general.
  for (size_t i = 0; (i = asmStr.find('\n', i)) != std::string::npos; i++) {
    num++;
  }

  size_t index = asmStr.length();
  index = index > 0 ? --index : 0;
  while (index > 0) {
    if ((asmStr[index] >= 'A' && asmStr[index] <= 'Z') || (asmStr[index] >= 'a' && asmStr[index] <= 'z')) {
      break;
    }
    index--;
    if (index == 0) {
      break;
    }
  }
  // The last instruction in inline asm does not use a newline character.
  if (index > 0 && asmStr.find('\n', index) == std::string::npos) {
    num++;
  }

  return num;
}

void AArch64AlignAnalysis::ComputeInsnAddr() {
  uint32 addr = 0;
  FOR_ALL_BB(bb, aarFunc) {
    if (bb != nullptr && bb->IsBBNeedAlign()) {
      addr += ComputeBBAlignNopNum(*bb, addr);
    }

    FOR_BB_INSNS(insn, bb) {
      if (insn == nullptr || !insn->IsMachineInstruction()) {
        continue;
      }

      MOperator mOp = insn->GetMachineOpcode();
      if (mOp == MOP_asm) {
        addr += GetInlineAsmInsnNum(*insn);
      } else {
        addr += insn->GetAtomicNum();
      }
      if ((mOp == MOP_wtbz || mOp == MOP_wtbnz || mOp == MOP_xtbz || mOp == MOP_xtbnz) && IsSplitInsn(*insn)) {
        ++addr;
      }
      insn->SetAddress(addr);
    }
  }
}

bool AArch64AlignAnalysis::MarkForLoop() {
  if (loopHeaderBBs.empty()) {
    return false;
  }

  ComputeInsnAddr();
  bool changed = false;
  for (BB *header: loopHeaderBBs) {
    if (!header->IsBBNeedAlign()) {
      continue;
    }
    Insn *insn = header->GetFirstInsn();
    while (insn != nullptr && !insn->IsMachineInstruction()) {
      insn = insn->GetNext();
    }

    uint32 nopNum = header->GetAlignNopNum();
    // Set the nop(s) on the first machine instruction of loop header bb to check whether
    // the number of nop of bb changes after ComputeInsnAddr.
    if (insn != nullptr && insn->IsMachineInstruction() && nopNum != 0) {
      if (nopNum != insn->GetNopNum()) {
        changed = true;
        insn->SetNopNum(nopNum);
      }
    }
  }
  return changed;
}

Insn* AArch64AlignAnalysis::FindTargetIsland(BB &bb) const {
  BB *targetBB = &bb;
  Insn *targetInsn = targetBB->GetLastInsn();
  while (targetInsn != nullptr || (targetBB != aarFunc->GetFirstBB() && targetBB != nullptr)) {
    while (targetInsn == nullptr && targetBB->GetPrev() != nullptr) {
      targetBB = targetBB->GetPrev();
      targetInsn = targetBB->GetLastInsn();
    }
    if (targetInsn == nullptr && targetBB->GetPrev() == nullptr) {
      break;
    }
    // Don't insert nop across aligned bb in avoid to mutual influence.
    if (targetBB->IsBBNeedAlign() || targetInsn->GetNopNum() != 0) {
      break;
    }
    if (targetInsn->GetMachineOpcode() == MOP_xuncond || targetInsn->GetMachineOpcode() == MOP_xret ||
        targetInsn->GetMachineOpcode() == MOP_xbr) {
      return targetInsn;
    }
    targetInsn = targetInsn->GetPrev();
  }
  return nullptr;
}

void AArch64AlignAnalysis::AddNopForLoopAfterMark() {
  FOR_ALL_BB(bb, aarFunc) {
    // just add nop for loop header bb
    if (loopHeaderBBs.find(bb) == loopHeaderBBs.end()) {
      continue;
    }

    if (!bb->IsBBNeedAlign()) {
      continue;
    }

    if (bb->GetAlignNopNum() == 0) {
      bb->SetNeedAlign(false);
      bb->SetAlignPower(0);
      continue;
    }

    BB *targetBB = bb->GetPrev();
    uint32 nopNum = bb->GetAlignNopNum();
    Insn *targetInsn = FindTargetIsland(*targetBB);
    if (targetInsn != nullptr) {
      for (uint32 i = 0; i < nopNum; i++) {
        (void)bb->InsertInsnAfter(*targetInsn, aarFunc->GetInsnBuilder()->BuildInsn<AArch64CG>(MOP_nop));
      }
    } else {
      BB *prevBB = bb->GetPrev();
      while (prevBB != nullptr && prevBB->GetLastInsn() == nullptr) {
        prevBB = prevBB->GetPrev();
      }
      if (prevBB == nullptr) {
        for (uint32 i = 0; i < nopNum; i++) {
          (void)bb->InsertInsnBefore(*bb->GetFirstInsn(), aarFunc->GetInsnBuilder()->BuildInsn<AArch64CG>(MOP_nop));
        }
      } else {
        for (uint32 i = 0; i < nopNum; i++) {
          (void)bb->InsertInsnAfter(*prevBB->GetLastInsn(),
                                    aarFunc->GetInsnBuilder()->BuildInsn<AArch64CG>(MOP_nop));
        }
      }
    }
    bb->SetNeedAlign(false);
    bb->SetAlignPower(0);
    bb->SetAlignNopNum(0);
  }
}

void AArch64AlignAnalysis::AddNopForLoop() {
  bool loopChange = false;
  bool shortBrChange = false;
  for (;;) {
    loopChange = MarkForLoop();
    if (!loopChange) {
      break;
    }
    shortBrChange = MarkShortBranchSplit();
    if (!shortBrChange) {
      break;
    }
  }
  AddNopForLoopAfterMark();
}

void AArch64AlignAnalysis::FindJumpTargetByFrequency() {
  MapleUnorderedMap<LabelIdx, BB*> label2BBMap = aarFunc->GetLab2BBMap();
  if (label2BBMap.empty()) {
    return;
  }

  for (auto &iter : label2BBMap) {
    BB *jumpBB = iter.second;
    if (jumpBB == nullptr) {
      continue;
    }
    // Select bb for alignment:
    // 1. The bb is frequently invoked.
    // 2. The predecessor of bb is not frequently invoked or almost never executed.
    // 3. The jump edge is frequently invoked.
    // The following values 2 and 10 are gcc empirical values.
    if (!HasFallthruEdge(*jumpBB) && (GetBranchFreq(*jumpBB) >= GetFreqThreshold() || (jumpBB->GetPrev() != nullptr &&
        jumpBB->GetFrequency() > jumpBB->GetPrev()->GetFrequency() * 10)) &&
        jumpBB->GetPrev()->GetFrequency() <= (aarFunc->GetFirstBB()->GetFrequency() / 2)) {
      InsertJumpTargetBBs(*jumpBB);
    }
  }
}

void AArch64AlignAnalysis::FindLoopHeaderByFrequency() {
  if (loopInfo.GetLoops().empty()) {
    return;
  }

  uint32 loopIterations = CGOptions::GetAlignLoopIterations();

  for (auto *loop : loopInfo.GetLoops()) {
    BB header = loop->GetHeader();
    uint64 branchFreq = GetBranchFreq(header);
    uint64 fallthruFreq = GetFallThruFreq(header);
    // Select bb for alignment: The bb is frequently invoked.
    if (HasFallthruEdge(header) && (branchFreq + fallthruFreq > GetFreqThreshold()) &&
        (branchFreq > fallthruFreq * loopIterations) &&
        !(header.GetSuccsSize() == 1 && *header.GetSuccsBegin() == aarFunc->GetLastBB())) {
      InsertLoopHeaderBBs(header);
    }
  }
}
} /* namespace maplebe */
