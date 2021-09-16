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
#ifndef MAPLEBE_INCLUDE_CG_CRITICAL_EDGE_H
#define MAPLEBE_INCLUDE_CG_CRITICAL_EDGE_H

#include "cgbb.h"
#include "insn.h"

namespace maplebe {
class CriticalEdge{
 public:
  CriticalEdge(CGFunc &func, MemPool &mem)
      : cgFunc(&func),
        alloc(&mem),
        criticalEdges(alloc.Adapter())
        {}

  ~CriticalEdge() = default;

  void CollectCriticalEdges();
  void SplitCriticalEdges();

 private:
  CGFunc *cgFunc;
  MapleAllocator alloc;
  MapleVector<std::pair<BB*, BB*>> criticalEdges;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgCriticalEdge, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_CRITICAL_EDGE_H */
