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

#define JAVALANG (mirModule.IsJavaModule())
#define CLANG (mirModule.IsCModule())

namespace maple {
bool IpaSccPM::PhaseRun(MIRModule &m) {
  if (theMIRModule->HasPartO2List()) {
    return false;
  }
  SetQuiet(true);
  bool oldProp = MeOption::propDuringBuild;
  bool oldMerge = MeOption::mergeStmts;
  bool oldLayout = MeOption::layoutWithPredict;
  MeOption::mergeStmts = false;
  MeOption::propDuringBuild = false;
  MeOption::layoutWithPredict = false;
  DoPhasesPopulate(m);
  bool changed = false;
  auto admMempool = AllocateMemPoolInPhaseManager("Ipa Phase Manager's Analysis Data Manager mempool");
  auto *serialADM = GetManagerMemPool()->New<AnalysisDataManager>(*(admMempool.get()));
  CallGraph *cg = GET_ANALYSIS(M2MCallGraph, m);
  // Need reverse sccV
  const MapleVector<SCCNode<CGNode>*> &topVec = cg->GetSCCTopVec();
  for (MapleVector<SCCNode<CGNode>*>::const_reverse_iterator it = topVec.rbegin(); it != topVec.rend(); ++it) {
    if (!IsQuiet()) {
      LogInfo::MapleLogger() << ">>>>>>>>>>> Optimizing SCC ---\n";
      (*it)->Dump();
    }
    auto meFuncMP = std::make_unique<ThreadLocalMemPool>(memPoolCtrler, "maple_ipa per-scc mempool");
    auto meFuncStackMP = std::make_unique<StackMemPool>(memPoolCtrler, "");
    bool runScc = false;
    for (auto *cgNode : (*it)->GetNodes()) {
      MIRFunction *func = cgNode->GetMIRFunction();
      if (func->IsEmpty()) {
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
      meFunc.IPAPrepare();
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
      changed |= RunAnalysisPhase<MapleSccPhase<SCCNode<CGNode>>, SCCNode<CGNode>>(
          *curPhase, *serialADM, **it);
    }
    serialADM->EraseAllAnalysisPhase();
  }
  MeOption::mergeStmts = oldMerge;
  MeOption::propDuringBuild = oldProp;
  MeOption::layoutWithPredict = oldLayout;
  return changed;
}

void IpaSccPM::DoPhasesPopulate(const MIRModule &mirModule) {
  (void)mirModule;
  AddPhase("sccprepare", true);
  AddPhase("prop_param_type",  MeOption::npeCheckMode != SafetyCheckMode::kNoCheck);
  AddPhase("prop_return_attr",  MeOption::npeCheckMode != SafetyCheckMode::kNoCheck);
  AddPhase("collect_ipa_info", true);
  AddPhase("sccsideeffect", true);
  AddPhase("sccemit", true);
}

void IpaSccPM::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<M2MCallGraph>();
  aDep.AddRequired<M2MKlassHierarchy>();
  aDep.AddPreserved<M2MCallGraph>();
  aDep.AddPreserved<M2MKlassHierarchy>();
}

void SCCPrepare::Dump(MeFunction &f, const std::string phaseName) {
  if (Options::dumpIPA) {
    LogInfo::MapleLogger() << ">>>>> Dump after " << phaseName << " <<<<<\n";
    f.Dump(false);
    LogInfo::MapleLogger() << ">>>>> Dump after End <<<<<\n\n";
  }
}

bool SCCPrepare::PhaseRun(SCCNode<CGNode> &scc) {
  SetQuiet(true);
  AddPhase("mecfgbuild", true);
  AddPhase("ssatab", true);
  AddPhase("aliasclass", true);
  AddPhase("ssa", true);
  AddPhase("irmapbuild", true);
  AddPhase("hprop", true);

  // Not like other phasemanager which use temp mempool to hold analysis results generated from the sub phases.
  // Here we use GetManagerMemPool which lives longer than this phase(manager) itself to hold all the analysis result.
  // So the following phase can access the result in this phase.
  result = GetManagerMemPool()->New<AnalysisDataManager>(*GetPhaseMemPool());
  for (auto *cgNode : scc.GetNodes()) {
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
      Dump(meFunc,phase->PhaseName());
    }
  }
  return false;
}

void SCCEmit::Dump(MeFunction &f, const std::string phaseName) {
  if (Options::dumpIPA) {
    LogInfo::MapleLogger() << ">>>>> Dump after " << phaseName << " <<<<<\n";
    f.DumpFunctionNoSSA();
    LogInfo::MapleLogger() << ">>>>> Dump after End <<<<<\n\n";
  }
}

bool SCCEmit::PhaseRun(SCCNode<CGNode> &scc) {
  SetQuiet(true);
  auto *map = GET_ANALYSIS(SCCPrepare, scc);
  if (map == nullptr) {
    return false;
  }
  auto admMempool = AllocateMemPoolInPhaseManager("Ipa Phase Manager's Analysis Data Manager mempool");
  auto *serialADM = GetManagerMemPool()->New<AnalysisDataManager>(*(admMempool.get()));
  serialADM->CopyAnalysisResultFrom(*map);
  for (auto *cgNode : scc.GetNodes()) {
    MIRFunction *func = cgNode->GetMIRFunction();
    if (func->GetBody() == nullptr) {
      continue;
    }
    MIRModule &m = *func->GetModule();
    m.SetCurFunction(func);
    const MaplePhaseInfo *phase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(&MEPreMeEmission::id);
    if (!IsQuiet()) {
      LogInfo::MapleLogger() << "---Run " << (phase->IsAnalysis() ? "analysis" : "transform")
                             << " Phase [ " << phase->PhaseName() << " ]---\n";
    }
    (void)RunAnalysisPhase<meFuncOptTy, MeFunction>(*phase, *serialADM, *func->GetMeFunc());
    Dump(*func->GetMeFunc(), phase->PhaseName());
  }
  return false;
}

void SCCEmit::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<SCCPrepare>();
}

MAPLE_ANALYSIS_PHASE_REGISTER(SCCPrepare, sccprepare)
MAPLE_ANALYSIS_PHASE_REGISTER(SCCCollectIpaInfo, collect_ipa_info);
MAPLE_ANALYSIS_PHASE_REGISTER(SCCPropReturnAttr, prop_return_attr);
MAPLE_TRANSFORM_PHASE_REGISTER(SCCPropParamType, prop_param_type);
MAPLE_ANALYSIS_PHASE_REGISTER(SCCSideEffect, sccsideeffect)
MAPLE_ANALYSIS_PHASE_REGISTER(SCCEmit, sccemit)
}  // namespace maple
