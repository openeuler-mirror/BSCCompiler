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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_GLOBAL_SCHEDULE_H
#define MAPLEBE_INCLUDE_CG_AARCH64_GLOBAL_SCHEDULE_H

#include "global_schedule.h"

namespace maplebe {
class AArch64GlobalSchedule : public GlobalSchedule {
 public:
  AArch64GlobalSchedule(MemPool &mp, CGFunc &f, ControlDepAnalysis &cdAna, DataDepAnalysis &dda)
      : GlobalSchedule(mp, f, cdAna, dda) {}
  ~AArch64GlobalSchedule() override = default;

  /* Verify global scheduling */
  void VerifyingSchedule(CDGRegion &region) override;

 protected:
  void FinishScheduling(CDGNode &cdgNode) override;
};
} /* namespace maplebe */

#endif  // MAPLEBE_INCLUDE_CG_AARCH64_GLOBAL_SCHEDULE_H
