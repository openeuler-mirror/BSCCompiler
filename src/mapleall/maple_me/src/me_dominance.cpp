/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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

// This phase analyses the CFG of the given MeFunction, generates the dominator tree,
// and the dominance frontiers of each basic block using Keith Cooper's algorithm.
// For some backward data-flow problems, such as LiveOut,
// the reverse CFG(The CFG with its edges reversed) is always useful,
// so we also generates the above two structures on the reverse CFG.
namespace maple {
AnalysisResult *MeDoDominance::Run(MeFunction *func, MeFuncResultMgr*, ModuleResultMgr*) {
  MemPool *memPool = NewMemPool();
  auto *dom = memPool->New<Dominance>(*memPool, *NewMemPool(), func->GetAllBBs(),
                                      *func->GetCommonEntryBB(), *func->GetCommonExitBB());
  dom->GenPostOrderID();
  dom->ComputeDominance();
  dom->ComputeDomFrontiers();
  dom->ComputeDomChildren();
  dom->ComputeIterDomFrontiers();
  size_t num = 0;
  dom->ComputeDtPreorder(*func->GetCommonEntryBB(), num);
  dom->GetDtPreOrder().resize(num);
  dom->ComputeDtDfn();

  dom->PdomGenPostOrderID();
  dom->ComputePostDominance();
  dom->ComputePdomFrontiers();
  dom->ComputePdomChildren();
  dom->ComputeIterPdomFrontiers();
  num = 0;
  dom->ComputePdtPreorder(*func->GetCommonExitBB(), num);
  dom->ResizePdtPreOrder(num);
  dom->ComputePdtDfn();
  if (DEBUGFUNC(func)) {
    LogInfo::MapleLogger() << "-----------------Dump dominance info and postdominance info---------\n";
    dom->DumpDoms();
    dom->DumpPdoms();
  }
  return dom;
}
}  // namespace maple
