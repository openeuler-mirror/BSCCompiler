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
#ifndef MAPLEBE_INCLUDE_CG_GLOBAL_SCHEDULE_H
#define MAPLEBE_INCLUDE_CG_GLOBAL_SCHEDULE_H

#include "base_schedule.h"

namespace maplebe {
#define GLOBAL_SCHEDULE_DUMP CG_DEBUG_FUNC(cgFunc)

class GlobalSchedule : public BaseSchedule {
 public:
  GlobalSchedule(MemPool &mp, CGFunc &f, ControlDepAnalysis &cdAna, DataDepAnalysis &dda)
      : BaseSchedule(mp, f, cdAna), interDDA(dda) {}
  ~GlobalSchedule() override = default;

  std::string PhaseName() const {
    return "globalschedule";
  }
  void Run() override;
  bool CheckCondition(CDGRegion &region);
  // Region-based global scheduling entry, using the list scheduling algorithm for scheduling insns in bb
  void DoGlobalSchedule(CDGRegion &region);

  // Verifying the Correctness of Global Scheduling
  virtual void VerifyingSchedule(CDGRegion &region) = 0;

 protected:
  void InitInCDGNode(CDGRegion &region, CDGNode &cdgNode, MemPool &cdgNodeMp);
  void PrepareCommonSchedInfo(CDGRegion &region, CDGNode &cdgNode, MemPool &cdgNodeMp);
  virtual void FinishScheduling(CDGNode &cdgNode) = 0;
  void ClearCDGNodeInfo(CDGRegion &region, CDGNode &cdgNode, MemPool *cdgNodeMp);

  DataDepAnalysis &interDDA;
};

MAPLE_FUNC_PHASE_DECLARE(CgGlobalSchedule, maplebe::CGFunc)
} // namespace maplebe

#endif  // MAPLEBE_INCLUDE_CG_GLOBAL_SCHEDULE_H
