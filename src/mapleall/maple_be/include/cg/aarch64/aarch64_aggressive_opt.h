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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AGGRESSIVE_OPT_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AGGRESSIVE_OPT_H

#include "cg_aggressive_opt.h"
#include "aarch64_cgfunc.h"

namespace maplebe {
class AArch64CombineRedundantX16Opt {
 public:
  explicit AArch64CombineRedundantX16Opt(CGFunc &func) : aarFunc(static_cast<AArch64CGFunc&>(func)) {}
  ~AArch64CombineRedundantX16Opt() {
    recentX16DefPrevInsns = nullptr;
    recentX16DefInsn = nullptr;
  }

  void Run();

 private:
  // Record memory instructions that use x16 info
  struct UseX16InsnInfo {
    void InsertAddPrevInsns(MapleVector<Insn*> &recentPrevInsns) const {
      for (auto insn : recentPrevInsns) {
        addPrevInsns->emplace_back(insn);
      }
    }

    uint32 infoId = 0; // unique id of the info, which is accumulated

    Insn *memInsn = nullptr; // the memory instruction that use x16. e.g. ldr x1, [x16, #8]
    // If there has x16 accumulation, record all related instructions.
    // e.g.
    // add x16, sp, #1, LSL #12   ---->  push in $addPrevInsns
    // add x16, x16, #512   ----> set $addInsn
    // str x1, [x16, #8]
    MapleVector<Insn*> *addPrevInsns = nullptr;
    Insn *addInsn = nullptr; // the split instruction that def x16

    uint32 memSize = 0; // used to calculate valid offset in memory instruction
    int64 originalOfst = 0; // the offset before optimize of memory instruction
    int64 curOfst = 0; // the offset after optimize of memory instruction
    int64 curAddImm = 0; // the split immediate after optimize of x16-add

    // The offset of memory instruction is in a valid range,
    // so we use min-value and max-value to indicate the valid split immediate range: [minValidAddImm, maxValidAddImm],
    // values within the range are all valid for the current $memInsn
    // e.g.
    // a valid split immediate case:
    // add x16, useOpnd, #minValidAddImm
    // str x1, [x16, #validOffset]
    int64 minValidAddImm = 0;
    int64 maxValidAddImm = 0;
  };

  // Record related info after combining optimization
  struct CombineInfo {
    // the first found common split immediate that satisfies all memInsns in the current segment
    int64 combineAddImm = 0;
    // e.g. add x16, x10(addUseOpnd), #1024
    RegOperand *addUseOpnd = nullptr;
    // Because the offsets of memory insns are discrete points, and the multiple requirements must be met,
    // therefore, there may be multiple splitting x16-def insns shared within a segment.
    MapleVector<UseX16InsnInfo*> *combineUseInfos = nullptr;
  };

  // The minimum unit of the combine optimization,
  // the instructions in it can all share the same value split by x16.
  struct SegmentInfo {
    MapleVector<UseX16InsnInfo*> *segUseInfos = nullptr;
    MapleVector<CombineInfo*> *segCombineInfos = nullptr;
  };

  void ClearCurrentSegmentInfos() {
    segmentInfo->segUseInfos->clear();
    for (auto *combineInfo : *segmentInfo->segCombineInfos) {
      combineInfo->combineUseInfos->clear();
    }
    segmentInfo->segCombineInfos->clear();

    recentX16DefPrevInsns->clear();
    recentX16DefInsn = nullptr;
    recentSplitUseOpnd = nullptr;

    recentAddImm = 0;
    isSameAddImm = true;
    isX16Used = false;
    hasUseOpndReDef = false;
    isSpecialX16Def = false;
  }

  void InitSegmentInfo(MemPool *tmpMp, MapleAllocator *tmpAlloc);
  void ClearSegmentInfo(MemPool *tmpMp, MapleAllocator *tmpAlloc);
  void ResetInsnId();
  bool IsEndOfSegment(const Insn &insn, bool hasX16Def);
  void ProcessAtEndOfSegment(BB &bb, Insn &insn, bool hasX16Def, MemPool *localMp, MapleAllocator *localAlloc);
  void ComputeRecentAddImm();
  void RecordRecentSplitInsnInfo(Insn &insn);
  bool IsUseX16MemInsn(const Insn &insn) const;
  void RecordUseX16InsnInfo(Insn &insn, MemPool *tmpMp, MapleAllocator *tmpAlloc);
  void ComputeValidAddImmInterval(UseX16InsnInfo &x16UseInfo, bool isPair);
  void FindCommonX16DefInsns(MemPool *tmpMp, MapleAllocator *tmpAlloc) const;
  void ProcessSameAddImmCombineInfo(MemPool *tmpMp, MapleAllocator *tmpAlloc) const;
  void ProcessIntervalIntersectionCombineInfo(MemPool *tmpMp, MapleAllocator *tmpAlloc) const;
  void CombineRedundantX16DefInsns(BB &bb);

  bool HasX16Def(const Insn &insn) const;
  bool HasX16Use(const Insn &insn) const;
  bool HasUseOpndReDef(const Insn &insn) const;
  uint32 GetMemSizeFromMD(const Insn &insn) const;
  RegOperand *GetAddUseOpnd(const Insn &insn);
  uint32 GetMemOperandIdx(const Insn &insn) const;
  int64 GetAddImmValue(const Insn &insn) const;
  CombineInfo *CreateCombineInfo(int64 addImm, uint32 startIdx, uint32 endIdx, MemPool &tmpMp,
                                 MapleAllocator *tmpAlloc) const;
  void RoundInterval(int64 &minInv, int64 &maxInv, uint32 startIdx, uint32 endIdx);
  bool IsImmValidWithMemSize(uint32 memSize, int64 imm) const;

  AArch64CGFunc &aarFunc;
  SegmentInfo *segmentInfo = nullptr;
  MapleVector<Insn*> *recentX16DefPrevInsns = nullptr;
  Insn *recentX16DefInsn = nullptr;
  RegOperand *recentSplitUseOpnd = nullptr;
  int64 recentAddImm = 0;
  bool isSameAddImm = true;
  bool isX16Used = false;
  bool hasUseOpndReDef = false;
  bool isSpecialX16Def = false;  // For ignoring movz/movk that def x16 pattern
  // We only care about x16 def insns for splitting, such as add*/mov*.
  // This filed identifies whether the x16 def insn is irrelevant, for example, ubfx/sub ...
  bool isIrrelevantX16Def = false;
};

class AArch64AggressiveOpt : public CGAggressiveOpt {
 public:
  explicit AArch64AggressiveOpt(CGFunc &func) : CGAggressiveOpt(func) {}
  ~AArch64AggressiveOpt() override = default;

  void DoOpt() override;
};
} /* namespace maplebe */
#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AGGRESSIVE_OPT_H */
