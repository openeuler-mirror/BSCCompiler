/*
 * Copyright (c) [2021] Futurewei Technologies Co.,Ltd.All rights reserved.
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
#include <iostream>
#include <algorithm>
#include "me_option.h"
#include "mir_module.h"
#include "mir_lower.h"
#include "mir_builder.h"
#include "me_loop_analysis.h"
#include "me_autovec.h"
#include "lfo_loop_vec.h"
#include "seqvec.h"

namespace maple {
bool MEAutoVectorization::PhaseRun(MeFunction &f) {
  // generate lfo IR
  PreMeEmitter *lfoemit = GET_ANALYSIS(MEPreMeEmission, f);
  CHECK_NULL_FATAL(lfoemit);

  if (MeOption::loopVec) {
    // step 1: get dependence graph for each loop
    auto *lfodepInfo = GET_ANALYSIS(MELfoDepTest, f);
    CHECK_NULL_FATAL(lfodepInfo);

    if (DEBUGFUNC_NEWPM(f)) {
      LogInfo::MapleLogger() << "\n**** Before loop vectorization ****\n";
      f.GetMirFunc()->Dump(false);
    }

    // run loop vectorization
    LoopVectorization loopVec(GetPhaseMemPool(), lfoemit, lfodepInfo, DEBUGFUNC_NEWPM(f));
    loopVec.Perform();

    if (DEBUGFUNC_NEWPM(f)) {
      LogInfo::MapleLogger() << "\n\n\n**** After loop vectorization ****\n";
      f.GetMirFunc()->Dump(false);
    }
  }

  if (MeOption::seqVec) {
    // run sequence vectorization
    SeqVectorize seqVec(GetPhaseMemPool(), lfoemit, DEBUGFUNC_NEWPM(f));
    seqVec.Perform();
    if (DEBUGFUNC_NEWPM(f)) {
      LogInfo::MapleLogger() << "\n\n\n**** After sequence vectorization ****\n";
      f.GetMirFunc()->Dump(false);
    }
  }

  return false;
}

void MEAutoVectorization::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEPreMeEmission>();
  aDep.AddRequired<MELfoDepTest>();
  aDep.PreservedAllExcept<MEDominance>();
  aDep.PreservedAllExcept<MELoopAnalysis>();
}
}  // namespace maple
