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
    recentX16DefInsn = nullptr;
  }

  void Run();

 private:
  struct UseX16InsnInfo {
    void InsertAddPrevInsns(MapleVector<Insn*> &recentPrevInsns) const {
      for (auto insn : recentPrevInsns) {
        addPrevInsns->emplace_back(insn);
      }
    }

    uint32 infoId = 0;

    Insn *memInsn = nullptr;
    MapleVector<Insn*> *addPrevInsns = nullptr;
    Insn *addInsn = nullptr;

    uint32 memSize = 0;
    int64 originalOfst = 0;
    int64 curOfst = 0;
    int64 curAddImm = 0;

    int64 minValidAddImm = 0;
    int64 maxValidAddImm = 0;
  };

  struct CombineInfo {
    int64 combineAddImm = 0;
    RegOperand *addUseOpnd = nullptr;
    MapleVector<UseX16InsnInfo*> *combineUseInfos = nullptr;
  };

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

    if (clearX16Def) {
      recentX16DefPrevInsns->clear();
      recentX16DefInsn = nullptr;
      recentSplitUseOpnd = nullptr;
    }

    recentAddImm = 0;
    isSameAddImm = true;
    clearX16Def = false;
    isX16Used = false;
    hasUseOpndReDef = false;
    isSpecialX16Def = false;
  }

  void InitSegmentInfo(MemPool *tmpMp, MapleAllocator *tmpAlloc);
  void ClearSegmentInfo(MemPool *tmpMp, MapleAllocator *tmpAlloc);
  void ResetInsnId();
  bool IsEndOfSegment(Insn &insn, bool hasX16Def);
  void ComputeRecentAddImm();
  void RecordRecentSplitInsnInfo(Insn &insn);
  bool IsUseX16MemInsn(Insn &insn);
  void RecordUseX16InsnInfo(Insn &insn, MemPool *tmpMp, MapleAllocator *tmpAlloc);
  void ComputeValidAddImmInterval(UseX16InsnInfo &x16UseInfo, bool isPair);
  void FindCommonX16DefInsns(MemPool *tmpMp, MapleAllocator *tmpAlloc);
  void ProcessSameAddImmCombineInfo(MemPool *tmpMp, MapleAllocator *tmpAlloc) const;
  void ProcessIntervalIntersectionCombineInfo(MemPool *tmpMp, MapleAllocator *tmpAlloc);
  void CombineRedundantX16DefInsns(BB &bb);

  bool HasX16Def(const Insn &insn) const;
  bool HasX16Use(const Insn &insn) const;
  bool HasUseOpndReDef(const Insn &insn) const;
  uint32 GetMemSizeFromMD(Insn &insn);
  RegOperand *GetAddUseOpnd(Insn &insn);
  uint32 GetMemOperandIdx(Insn &insn);
  int64 GetAddImmValue(Insn &insn);
  CombineInfo *CreateCombineInfo(int64 addImm, uint32 startIdx, uint32 endIdx, MemPool *tmpMp, MapleAllocator *tmpAlloc);
  void RoundInterval(int64 &minInv, int64 &maxInv, uint32 startIdx, uint32 endIdx);
  bool IsImmValidWithMemSize(uint32 memSize, int64 imm);

  AArch64CGFunc &aarFunc;
  SegmentInfo *segmentInfo = nullptr;
  MapleVector<Insn*> *recentX16DefPrevInsns = nullptr;
  Insn *recentX16DefInsn = nullptr;
  RegOperand *recentSplitUseOpnd = nullptr;
  int64 recentAddImm = 0;
  bool isSameAddImm = true;
  bool clearX16Def = false;
  bool isX16Used = false;
  bool hasUseOpndReDef = false;
  bool isSpecialX16Def = false;  // For ignoring movz/movk that def x16 pattern
};

class AArch64AggressiveOpt : public CGAggressiveOpt {
 public:
  explicit AArch64AggressiveOpt(CGFunc &func) : CGAggressiveOpt(func) {}
  ~AArch64AggressiveOpt() override = default;

  void DoOpt() override;
};
} /* namespace maplebe */
#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AGGRESSIVE_OPT_H */
