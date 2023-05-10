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
#ifndef MAPLEBE_INCLUDE_CG_LIVE_H
#define MAPLEBE_INCLUDE_CG_LIVE_H

#include "cg_phase.h"
#include "insn.h"
#include "cgbb.h"
#include "sparse_datainfo.h"
#include "cgfunc.h"

namespace maplebe {
class LiveAnalysis : public AnalysisResult {
 public:
  LiveAnalysis(CGFunc &func, MemPool &memPool)
      : AnalysisResult(&memPool), cgFunc(&func), memPool(&memPool), alloc(&memPool), stackMp(func.GetStackMemPool()) {}
  ~LiveAnalysis() override = default;

  void AnalysisLive();
  void Dump() const;
  void DumpInfo(const SparseDataInfo &info) const;
  void InitBB(BB &bb);
  void InitAndGetDefUse();
  bool GenerateLiveOut(BB &bb) const;
  bool GenerateLiveIn(BB &bb);
  void BuildInOutforFunc();
  void DealWithInOutOfCleanupBB();
  void InsertInOutOfCleanupBB();
  void ResetLiveSet();
  void ClearInOutDataInfo();
  void EnlargeSpaceForLiveAnalysis(BB &currBB);
  void GetBBDefUse(BB &bb) const;
  void ProcessAsmListOpnd(BB &bb, Operand &opnd, uint32 idx) const;
  void ProcessListOpnd(BB &bb, Operand &opnd, bool isDef) const;
  void ProcessMemOpnd(BB &bb, Operand &opnd) const;
  void ProcessCondOpnd(BB &bb) const;
  void CollectLiveInfo(BB &bb, const Operand &opnd, bool isDef, bool isUse) const;

  SparseDataInfo *NewLiveIn(uint32 maxRegCount) {
    return memPool->New<SparseDataInfo>(maxRegCount, alloc);
  }

  SparseDataInfo *NewLiveOut(uint32 maxRegCount) {
    return memPool->New<SparseDataInfo>(maxRegCount, alloc);
  }

  SparseDataInfo *NewDef(uint32 maxRegCount) {
    return memPool->New<SparseDataInfo>(maxRegCount, alloc);
  }

  SparseDataInfo *NewUse(uint32 maxRegCount) {
    return memPool->New<SparseDataInfo>(maxRegCount, alloc);
  }

  virtual void GenerateReturnBBDefUse(BB &bb) const = 0;
  virtual void ProcessCallInsnParam(BB &bb, const Insn &insn) const = 0;
  virtual bool CleanupBBIgnoreReg(regno_t reg) = 0;
  virtual void InitEhDefine(BB &bb) = 0;

 protected:
  int iteration = 0;
  CGFunc *cgFunc;
  MemPool *memPool;
  MapleAllocator alloc;
  StackMemPool &stackMp;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgLiveAnalysis, maplebe::CGFunc)
  LiveAnalysis *GetResult() {
    return live;
  }
  LiveAnalysis *live = nullptr;
MAPLE_FUNC_PHASE_DECLARE_END
}  /* namespace maplebe */
#endif  /* MAPLEBE_INCLUDE_CG_LIVE_H */
