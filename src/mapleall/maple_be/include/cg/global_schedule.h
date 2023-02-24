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
#ifndef MAPLEBE_INCLUDE_CG_GLOBAL_SCHEDULE_H
#define MAPLEBE_INCLUDE_CG_GLOBAL_SCHEDULE_H

#include "cgfunc.h"
#include "control_dep_analysis.h"
#include "data_dep_analysis.h"

namespace maplebe {
class GlobalSchedule {
 public:
  GlobalSchedule(MemPool &mp, CGFunc &f, ControlDepAnalysis &cdAna, InterDataDepAnalysis &interDDA)
      : gsMempool(mp), gsAlloc(&mp), cgFunc(f), cda(cdAna), idda(interDDA),
        dataNodes(gsAlloc.Adapter()) {}
  virtual ~GlobalSchedule() = default;

  void Run();

 protected:
  MemPool &gsMempool;
  MapleAllocator gsAlloc;
  CGFunc &cgFunc;
  ControlDepAnalysis &cda;
  InterDataDepAnalysis &idda;
  MapleVector<DepNode*> dataNodes;
};

MAPLE_FUNC_PHASE_DECLARE(CgGlobalSchedule, maplebe::CGFunc)
} /* namespace maplebe */

#endif  // MAPLEBE_INCLUDE_CG_GLOBAL_SCHEDULE_H
