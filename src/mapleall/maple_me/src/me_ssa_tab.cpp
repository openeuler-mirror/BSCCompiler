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
#include "me_ssa_tab.h"
#include "me_cfg.h"
#include <cstdlib>
#include "mpl_timer.h"
#include "me_dominance.h"
#include "ssa_tab.h"

// allocate the data structure to store SSA information
namespace maple {
bool MESSATab::PhaseRun(maple::MeFunction &f) {
  auto *cfg = f.GetCfg();
  MPLTimer timer;
  timer.Start();
  if (DEBUGFUNC_NEWPM(f)) {
    LogInfo::MapleLogger() << "\n============== SSA and AA preparation =============" << '\n';
  }
  MemPool *memPool = GetPhaseMemPool();
  // allocate ssaTab including its SSAPart to store SSA information for statements
  ssaTab = memPool->New<SSATab>(memPool, f.GetVersMp(), &f.GetMIRModule(), &f);
  f.SetMeSSATab(ssaTab);
#if DEBUG
  globalSSATab = ssaTab;
#endif
  // pass through the program statements
  for (auto bIt = cfg->valid_begin(); bIt != cfg->valid_end(); ++bIt) {
    auto *bb = *bIt;
    for (auto &stmt : bb->GetStmtNodes()) {
      ssaTab->CreateSSAStmt(stmt, bb);  // this adds the SSANodes for exprs
    }
  }
  if (DEBUGFUNC_NEWPM(f)) {
    timer.Stop();
    LogInfo::MapleLogger() << "ssaTab consumes cumulatively " << timer.Elapsed() << "seconds " << '\n';
  }
  return false;
}

void MESSATab::GetAnalysisDependence(AnalysisDep &aDep) const {
  aDep.AddRequired<MEMeCfg>();
}
}  // namespace maple
