/*
* Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEBE_INCLUDE_CG_BASE_SCHEDULE_H
#define MAPLEBE_INCLUDE_CG_BASE_SCHEDULE_H

#include "cgfunc.h"
#include "control_dep_analysis.h"
#include "data_dep_analysis.h"
#include "list_scheduler.h"

namespace maplebe {
class BaseSchedule {
 public:
  BaseSchedule(MemPool &mp, CGFunc &f, ControlDepAnalysis &cdAna, bool doDelay = false)
      : schedMP(mp), schedAlloc(&mp), cgFunc(f), cda(cdAna), doDelayHeu(doDelay) {}
  virtual ~BaseSchedule() = default;

  virtual void Run() = 0;
  void DoLocalSchedule(CDGRegion &region);
  bool DoDelayHeu() {
    return doDelayHeu;
  }
  void SetDelayHeu() {
    doDelayHeu = true;
  }

 protected:
  void InitInsnIdAndLocInsn();
  void InitInRegion(CDGRegion &region);
  void DumpRegionInfoBeforeSchedule(CDGRegion &region) const;
  void DumpCDGNodeInfoBeforeSchedule(CDGNode &cdgNode) const;
  void DumpCDGNodeInfoAfterSchedule(CDGNode &cdgNode) const;
  virtual void DumpInsnInfoByScheduledOrder(BB &curBB) const = 0;

  MemPool &schedMP;
  MapleAllocator schedAlloc;
  CGFunc &cgFunc;
  ControlDepAnalysis &cda;
  CommonScheduleInfo *commonSchedInfo = nullptr;
  ListScheduler *listScheduler = nullptr;
  bool doDelayHeu = false;
};
} /* namespace maplebe */

#endif  // MAPLEBE_INCLUDE_CG_BASE_SCHEDULE_H
