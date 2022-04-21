/*
 * Copyright (c) [2021-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "ipa_phase_manager.h"
#include "pme_emit.h"
#include "ipa_gcovprofile.h"

namespace maple {

void IpaProfile::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  if (Options::profileUse) {
    aDep.AddRequired<M2MGcovParser>();
    aDep.AddPreserved<M2MGcovParser>();
  }
}

bool IpaProfile::PhaseRun(MIRModule &m) {
  SetQuiet(true);
  // keep original options value
  bool oldProp = MeOption::propDuringBuild;
  bool oldMerge = MeOption::mergeStmts;
  uint8 oldOptLevel = MeOption::optLevel;
  bool oldLayout = MeOption::layoutWithPredict;
  MeOption::mergeStmts = false;
  MeOption::propDuringBuild = false;
  MeOption::layoutWithPredict = false;
  MeOption::optLevel = 3;

  AddPhase("mecfgbuild", true);
  AddPhase("profileUse", true);
  AddPhase("emitforipa", true);
  // Not like other phasemanager which use temp mempool to hold analysis results generated from the sub phases.
  // Here we use GetManagerMemPool which lives longer than this phase(manager) itself to hold all the analysis result.
  // So the following phase can access the result in this phase.
  result = GetManagerMemPool()->New<AnalysisDataManager>(*GetPhaseMemPool());
  auto iter = m.GetFunctionList().begin();
  for (; iter != m.GetFunctionList().end(); iter++) {
    MIRFunction *func = *iter;
    if (func == nullptr || func->GetFuncSymbol()->GetStorageClass() == kScUnused ||
        func->IsEmpty()) {
      continue;
    }
    m.SetCurFunction(func);
    auto meFuncMP = std::make_unique<ThreadLocalMemPool>(memPoolCtrler, "ipa gcovprofile mempool");
    auto meFuncStackMP = std::make_unique<StackMemPool>(memPoolCtrler, "");
    MemPool *versMP = new ThreadLocalMemPool(memPoolCtrler, "first verst mempool");
    MeFunction &meFunc = *(meFuncMP->New<MeFunction>(&m, func, meFuncMP.get(), *meFuncStackMP, versMP, "unknown"));
    func->SetMeFunc(&meFunc);
    meFunc.PartialInit();
    if (!IsQuiet()) {
      LogInfo::MapleLogger() << "---Preparing Function for scc phase < " << func->GetName() << " > ---\n";
    }
    meFunc.Prepare();

    for (size_t i = 0; i < phasesSequence.size(); ++i) {
      const MaplePhaseInfo *phase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(phasesSequence[i]);
      if (!IsQuiet()) {
        LogInfo::MapleLogger() << " >> Prepare " << (phase->IsAnalysis() ? "analysis" : "transform")
                               << " Phase [ " << phase->PhaseName() << " ] <<\n";
      }
      if (phase->IsAnalysis()) {
        (void)RunAnalysisPhase<meFuncOptTy, MeFunction>(*phase, *result, meFunc, 1);
      } else {
        (void)RunTransformPhase<meFuncOptTy, MeFunction>(*phase, *result, meFunc, 1);
      }
    }
  }

  // restore option value
  MeOption::mergeStmts = oldMerge;
  MeOption::propDuringBuild = oldProp;
  MeOption::optLevel = oldOptLevel;
  MeOption::layoutWithPredict = oldLayout;
  return false;
}
}  // namespace maple
