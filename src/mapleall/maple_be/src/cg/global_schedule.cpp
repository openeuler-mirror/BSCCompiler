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

#include "global_schedule.h"
#include "optimize_common.h"
#include "aarch64_data_dep_base.h"
#include "cg.h"

namespace maplebe {
void GlobalSchedule::Run() {
  FCDG *fcdg = cda.GetFCDG();
  CHECK_FATAL(fcdg != nullptr, "control dependence analysis failed");
  cda.GenerateFCDGDot();
  cda.GenerateCFGDot();
  DotGenerator::GenerateDot("globalsched", cgFunc, cgFunc.GetMirModule(),
                            true, cgFunc.GetName());
  for (auto region : fcdg->GetAllRegions()) {
    if (region == nullptr) {
      continue;
    }
    idda.Run(*region, dataNodes);
    idda.GenerateInterDDGDot(dataNodes);
  }
}

void CgGlobalSchedule::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<CgControlDepAnalysis>();
}

bool CgGlobalSchedule::PhaseRun(maplebe::CGFunc &f) {
  MemPool *gsMemPool = GetPhaseMemPool();
  ControlDepAnalysis *cda = GET_ANALYSIS(CgControlDepAnalysis, f);
  MAD *mad = Globals::GetInstance()->GetMAD();
  // Need move to target
  auto *ddb = gsMemPool->New<AArch64DataDepBase>(*gsMemPool, f, *mad);
  auto *idda = gsMemPool->New<InterDataDepAnalysis>(f, *gsMemPool, *ddb);
  auto *globalSched = gsMemPool->New<GlobalSchedule>(*gsMemPool, f, *cda, *idda);
  globalSched->Run();
  return true;
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgGlobalSchedule, globalschedule)
} /* namespace maplebe */
