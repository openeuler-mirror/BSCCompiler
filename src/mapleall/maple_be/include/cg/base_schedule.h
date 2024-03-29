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
  virtual ~BaseSchedule() {
    listScheduler = nullptr;
  }

  virtual void Run() = 0;
  void InitInRegion(CDGRegion &region) const;
  void DoLocalSchedule(CDGRegion &region);
  bool DoDelayHeu() const {
    return doDelayHeu;
  }
  void SetDelayHeu() {
    doDelayHeu = true;
  }
  void SetUnitTest(bool flag) {
    isUnitTest = flag;
  }

 protected:
  void InitInsnIdAndLocInsn();
  // Using total number of machine instructions to control the end of the scheduling process
  void InitMachineInsnNum(CDGNode &cdgNode) const;
  uint32 CaculateOriginalCyclesOfBB(CDGNode &cdgNode) const;
  void DumpRegionInfoBeforeSchedule(CDGRegion &region) const;
  void DumpCDGNodeInfoBeforeSchedule(CDGNode &cdgNode) const;
  void DumpCDGNodeInfoAfterSchedule(CDGNode &cdgNode) const;
  void DumpInsnInfoByScheduledOrder(CDGNode &cdgNode) const;

  MemPool &schedMP;
  MapleAllocator schedAlloc;
  CGFunc &cgFunc;
  ControlDepAnalysis &cda;
  CommonScheduleInfo *commonSchedInfo = nullptr;
  ListScheduler *listScheduler = nullptr;
  bool doDelayHeu = false;
  bool isUnitTest = false;
};
} // namespace maplebe

#endif  // MAPLEBE_INCLUDE_CG_BASE_SCHEDULE_H
