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
#include "me_phase_manager.h"
#include "ipa_phase_manager.h"
#include "prop_return_null.h"
#include <iostream>
#include <vector>
#include <string>

#define JAVALANG (mirModule.IsJavaModule())
#define CLANG (mirModule.IsCModule())

namespace maple {
bool IpaSccPM::PhaseRun(MIRModule &m) {
  SetQuiet(true);
  bool oldProp = MeOption::propDuringBuild;
  bool oldMerge = MeOption::mergeStmts;
  MeOption::mergeStmts = false;
  MeOption::propDuringBuild = false;
  DoPhasesPopulate(m);
  bool changed = false;
  auto admMempool = AllocateMemPoolInPhaseManager("Ipa Phase Manager's Analysis Data Manager mempool");
  auto *serialADM = GetManagerMemPool()->New<AnalysisDataManager>(*(admMempool.get()));
  CallGraph *cg = GET_ANALYSIS(M2MCallGraph, m);
  // Need reverse sccV
  const MapleVector<SCCNode*> &topVec = cg->GetSCCTopVec();
  for (MapleVector<SCCNode*>::const_reverse_iterator it = topVec.rbegin(); it != topVec.rend(); ++it) {
    if (!IsQuiet()) {
      LogInfo::MapleLogger() << ">>>>>>>>>>> Optimizing SCC ---\n";
      (*it)->Dump();
    }
    auto meFuncMP = std::make_unique<ThreadLocalMemPool>(memPoolCtrler, "maple_ipa per-scc mempool");
    auto meFuncStackMP = std::make_unique<StackMemPool>(memPoolCtrler, "");
    bool runScc = false;
    for (auto *cgNode : (*it)->GetCGNodes()) {
      MIRFunction *func = cgNode->GetMIRFunction();
      if (func->IsEmpty() || !func->GetBody()) {
        continue;
      }
      runScc = true;
      m.SetCurFunction(func);
      MemPool *versMP = new ThreadLocalMemPool(memPoolCtrler, "first verst mempool");
      MeFunction &meFunc = *(meFuncMP->New<MeFunction>(&m, func, meFuncMP.get(), *meFuncStackMP, versMP, "unknown"));
      func->SetMeFunc(&meFunc);
      meFunc.PartialInit();
      if (!IsQuiet()) {
        LogInfo::MapleLogger() << "---Preparing Function for scc phase < " << func->GetName() << " > ---\n";
      }
      meFunc.Prepare();
    }
    if (!runScc) {
      continue;
    }
    for (size_t i = 0; i < phasesSequence.size(); ++i) {
      const MaplePhaseInfo *curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(phasesSequence[i]);
      if (!IsQuiet()) {
        LogInfo::MapleLogger() << "---Run scc " << (curPhase->IsAnalysis() ? "analysis" : "transform")
                               << " Phase [ " << curPhase->PhaseName() << " ]---\n";
      }
      changed |= RunAnalysisPhase<MapleSccPhase<SCCNode>, SCCNode>(*curPhase, *serialADM, **it);
    }
    serialADM->EraseAllAnalysisPhase();
  }
  MeOption::mergeStmts = oldMerge;
  MeOption::propDuringBuild = oldProp;
  return changed;
}

void IpaSccPM::DoPhasesPopulate(const MIRModule &mirModule) {
  (void)mirModule;
  AddPhase("sccprepare", true);
  AddPhase("prop_return_attr", true);
  AddPhase("sccemit", true);
}

void IpaSccPM::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<M2MCallGraph>();
  aDep.AddRequired<M2MKlassHierarchy>();
  aDep.AddPreserved<M2MCallGraph>();
  aDep.AddPreserved<M2MKlassHierarchy>();
}

bool SCCPrepare::PhaseRun(SCCNode &scc) {
  SetQuiet(true);
  AddPhase("mecfgbuild", true);
  AddPhase("ssatab", true);
  AddPhase("aliasclass", true);
  AddPhase("ssa", true);
  AddPhase("irmapbuild", true);

  // Not like other phasemanager which use temp mempool to hold analysis results generated from the sub phases.
  // Here we use GetPhaseMemPool which lives longer than this phase(manager) itself to hold all the analysis result.
  // So the following phase can access the result in this phase.
  result = GetManagerMemPool()->New<AnalysisDataManager>(*GetPhaseMemPool());
  for (auto *cgNode : scc.GetCGNodes()) {
    MIRFunction *func = cgNode->GetMIRFunction();
    if (func->IsEmpty()) {
      continue;
    }
    MIRModule &m = *func->GetModule();
    m.SetCurFunction(func);
    MeFunction &meFunc = *func->GetMeFunc();
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
  return false;
}

bool SCCEmit::PhaseRun(SCCNode &scc) {
  SetQuiet(true);
  auto *map = GET_ANALYSIS(SCCPrepare, scc);
  if (map == nullptr) {
    return false;
  }
  auto admMempool = AllocateMemPoolInPhaseManager("Ipa Phase Manager's Analysis Data Manager mempool");
  auto *serialADM = GetManagerMemPool()->New<AnalysisDataManager>(*(admMempool.get()));
  serialADM->CopyAnalysisResultFrom(*map);
  for (auto *cgNode : scc.GetCGNodes()) {
    MIRFunction *func = cgNode->GetMIRFunction();
    if (func->GetBody() == nullptr) {
      continue;
    }
    MIRModule &m = *func->GetModule();
    m.SetCurFunction(func);
    const MaplePhaseInfo *phase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(&EmitForIPA::id);
    if (!IsQuiet()) {
      LogInfo::MapleLogger() << "---Run " << (phase->IsAnalysis() ? "analysis" : "transform")
                             << " Phase [ " << phase->PhaseName() << " ]---\n";
    }
    (void)RunTransformPhase<meFuncOptTy, MeFunction>(*phase, *serialADM, *func->GetMeFunc());
  }
  return false;
}

void SCCEmit::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<SCCPrepare>();
}

MAPLE_ANALYSIS_PHASE_REGISTER(SCCPrepare, sccprepare)
MAPLE_ANALYSIS_PHASE_REGISTER(SCCPropReturnAttr, prop_return_attr);
MAPLE_ANALYSIS_PHASE_REGISTER(SCCEmit, sccemit)
}  // namespace maple
