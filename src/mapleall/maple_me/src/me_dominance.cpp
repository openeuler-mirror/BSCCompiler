/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_dominance.h"
#include <iostream>
#include "me_option.h"
#include "me_cfg.h"
#include "base_graph_node.h"
// This phase analyses the CFG of the given MeFunction, generates the dominator tree,
// and the dominance frontiers of each basic block using Keith Cooper's algorithm.
// For some backward data-flow problems, such as LiveOut,
// the reverse CFG(The CFG with its edges reversed) is always useful,
// so we also generates the above two structures on the reverse CFG.
namespace maple {
void MEDominance::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEMeCfg>();
  aDep.SetPreservedAll();
}

bool MEDominance::PhaseRun(maple::MeFunction &f) {
  MeCFG *cfg = f.GetCfg();
  ASSERT_NOT_NULL(cfg);
  MemPool *memPool = GetPhaseMemPool();
  auto alloc = MapleAllocator(memPool);
  auto nodeVec = memPool->New<MapleVector<BaseGraphNode*>>(alloc.Adapter());
  CHECK_NULL_FATAL(nodeVec);
  ConvertToVectorOfBasePtr<BB>(cfg->GetAllBBs(), *nodeVec);
  dom = memPool->New<Dominance>(*memPool, *nodeVec, *cfg->GetCommonEntryBB(), *cfg->GetCommonExitBB(), false);
  CHECK_NULL_FATAL(dom);
  pdom = memPool->New<Dominance>(*memPool, *nodeVec, *cfg->GetCommonExitBB(), *cfg->GetCommonEntryBB(), true);
  CHECK_NULL_FATAL(pdom);
  dom->Init();
  pdom->Init();
  if (DEBUGFUNC_NEWPM(f)) {
    LogInfo::MapleLogger() << "-----------------Dump dominance info and postdominance info---------\n";
    dom->DumpDoms();
    pdom->DumpDoms();
  }
  return false;
}
}  // namespace maple
