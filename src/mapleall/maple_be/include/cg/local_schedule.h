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
#ifndef MAPLEBE_INCLUDE_CG_LOCAL_SCHEDULE_H
#define MAPLEBE_INCLUDE_CG_LOCAL_SCHEDULE_H

#include "base_schedule.h"

namespace maplebe {
#define LOCAL_SCHEDULE_DUMP CG_DEBUG_FUNC(cgFunc)

class LocalSchedule : public BaseSchedule {
 public:
  LocalSchedule(MemPool &mp, CGFunc &f, ControlDepAnalysis &cdAna, DataDepAnalysis &dda)
      : BaseSchedule(mp, f, cdAna), intraDDA(dda) {}
  ~LocalSchedule() override = default;

  std::string PhaseName() const {
    return "localschedule";
  }
  void Run() override;
  bool CheckCondition(CDGRegion &region) const;
  void DoLocalScheduleForRegion(CDGRegion &region);
  using BaseSchedule::DoLocalSchedule;
  void DoLocalSchedule(CDGNode &cdgNode);

 protected:
  void InitInCDGNode(CDGNode &cdgNode);
  virtual void FinishScheduling(CDGNode &cdgNode) = 0;

  DataDepAnalysis &intraDDA;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgLocalSchedule, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
} /* namespace maplebe */

#endif  // MAPLEBE_INCLUDE_CG_LOCAL_SCHEDULE_H
