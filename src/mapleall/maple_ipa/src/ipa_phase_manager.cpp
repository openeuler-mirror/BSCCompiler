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
#include "mpl_profdata_parser.h"
#include "ipa_collect.h"
#include "ipa_side_effect.h"
#include "prop_return_null.h"
#include "prop_parameter_type.h"

#define JAVALANG (mirModule.IsJavaModule())
#define CLANG (mirModule.IsCModule())

namespace maple {
bool IpaSccPM::timePhases = false;
void IpaSccPM::Init(MIRModule &m) {
  SetQuiet(true);
  m.SetInIPA(true);
  if (IpaSccPM::timePhases) {
    InitTimeHandler();
  }
  MeOption::mergeStmts = false;
  MeOption::propDuringBuild = false;
  MeOption::layoutWithPredict = false;
  ipaInfo = GetPhaseAllocator()->New<CollectIpaInfo>(m, *GetPhaseMemPool());
  DoPhasesPopulate(m);
}

bool IpaSccPM::PhaseRun(MIRModule &m) {
  if (theMIRModule->HasPartO2List()) {
    return false;
  }
  bool oldProp = MeOption::propDuringBuild;
  bool oldMerge = MeOption::mergeStmts;
  bool oldLayout = MeOption::layoutWithPredict;
  Init(m);
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
      changed |= RunAnalysisPhase<MapleSccPhase<SCCNode<CGNode>>, SCCNode<CGNode>>(*curPhase, *serialADM, **it);
    }
    serialADM->EraseAllAnalysisPhase();
  }
  MeOption::mergeStmts = oldMerge;
  MeOption::propDuringBuild = oldProp;
  MeOption::layoutWithPredict = oldLayout;
  if (Options::dumpPhase == "outline") {
    ipaInfo->Dump();
  }
  m.SetInIPA(false);
  if (IpaSccPM::timePhases) {
    LogInfo::MapleLogger() << "==================  IpaSccPM  ==================";
    DumpPhaseTime();
  }
  return changed;
}

void IpaSccPM::DoPhasesPopulate(const MIRModule &mirModule) {
  (void)mirModule;
  if (Options::profileGen) {
    AddPhase("sccprofile", true);
  } else {
    AddPhase("sccprepare", true);
    AddPhase("prop_param_type",  MeOption::npeCheckMode != SafetyCheckMode::kNoCheck);
    AddPhase("prop_return_attr",  MeOption::npeCheckMode != SafetyCheckMode::kNoCheck);
    if (!Options::profileUse) {
      AddPhase("collect_ipa_info", true);
    }
    AddPhase("sccsideeffect", Options::sideEffect);
    AddPhase("sccemit", true);
  }
}

void IpaSccPM::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<M2MCallGraph>();
  aDep.AddRequired<M2MKlassHierarchy>();
  aDep.AddPreserved<M2MCallGraph>();
  aDep.AddPreserved<M2MKlassHierarchy>();
  if (Options::profileUse) {
    aDep.AddRequired<MMplProfDataParser>();
    aDep.AddPreserved<MMplProfDataParser>();
  }
}

void SCCPrepare::DumpSCCPrepare(const MeFunction &f, const std::string phaseName) const {
  if (Options::dumpIPA && (Options::dumpFunc == f.GetName() || Options::dumpFunc == "*")) {
    LogInfo::MapleLogger() << ">>>>> Dump after " << phaseName << " <<<<<\n";
    f.Dump(false);
    LogInfo::MapleLogger() << ">>>>> Dump after End <<<<<\n\n";
  }
}

bool SCCPrepare::PhaseRun(SCCNode<CGNode> &scc) {
  if (IpaSccPM::timePhases && Options::dumpIPA) {
    InitTimeHandler();
  }
  SetQuiet(true);
  AddPhase("mecfgbuild", true);
  if (Options::profileUse) {
    AddPhase("splitcriticaledge", true);
    AddPhase("profileUse", true);
  }
  AddPhase("ssatab", true);
  AddPhase("aliasclass", true);
  AddPhase("ssa", true);
  AddPhase("irmapbuild", true);
  AddPhase("objSize", true);
  AddPhase("hprop", true);
  AddPhase("identloops", Options::enableGInline);  // for me_predict when collecting inline summary

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
        (void)RunAnalysisPhase<MeFuncOptTy, MeFunction>(*phase, *result, meFunc, 1);
      } else {
        (void)RunTransformPhase<MeFuncOptTy, MeFunction>(*phase, *result, meFunc, 1);
      }
      DumpSCCPrepare(meFunc, phase->PhaseName());
    }
  }
  if (IpaSccPM::timePhases && Options::dumpIPA) {
    LogInfo::MapleLogger() << "================== SccPrepare ==================";
    scc.Dump();
    DumpPhaseTime();
  }
  return false;
}

void SCCEmit::DumpSCCEmit(MeFunction &f, const std::string phaseName) const {
  if (Options::dumpIPA && (f.GetName() == Options::dumpFunc || Options::dumpFunc == "*")) {
    LogInfo::MapleLogger() << ">>>>> Dump after " << phaseName << " <<<<<\n";
    f.GetMirFunc()->Dump();
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
      LogInfo::MapleLogger() << "  ---call " << (phase->IsAnalysis() ? "analysis" : "transform")
                             << " Phase [ " << phase->PhaseName() << " ]---\n";
    }
    (void)RunAnalysisPhase<MeFuncOptTy, MeFunction>(*phase, *serialADM, *func->GetMeFunc());
    DumpSCCEmit(*func->GetMeFunc(), phase->PhaseName());
    delete func->GetMeFunc()->GetPmeMempool();
    func->GetMeFunc()->SetPmeMempool(nullptr);
  }
  serialADM->EraseAllAnalysisPhase();
  return false;
}

void SCCEmit::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<SCCPrepare>();
}

bool SCCProfile::PhaseRun(SCCNode<CGNode> &scc) {
  SetQuiet(true);
  AddPhase("mecfgbuild", true);
  if (Options::profileGen) {
    AddPhase("splitcriticaledge", true);
    AddPhase("profileGen", true);
  }
  AddPhase("profgenEmit", true);
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
        (void)RunAnalysisPhase<MeFuncOptTy, MeFunction>(*phase, *result, meFunc, 1);
      } else {
        (void)RunTransformPhase<MeFuncOptTy, MeFunction>(*phase, *result, meFunc, 1);
      }
    }
  }
  return false;
}

MAPLE_ANALYSIS_PHASE_REGISTER(SCCPrepare, sccprepare)
MAPLE_ANALYSIS_PHASE_REGISTER(SCCProfile, sccprofile)
MAPLE_ANALYSIS_PHASE_REGISTER(SCCCollectIpaInfo, collect_ipa_info);
MAPLE_ANALYSIS_PHASE_REGISTER(SCCPropReturnAttr, prop_return_attr);
MAPLE_TRANSFORM_PHASE_REGISTER(SCCPropParamType, prop_param_type);
MAPLE_ANALYSIS_PHASE_REGISTER(SCCSideEffect, sccsideeffect)
MAPLE_ANALYSIS_PHASE_REGISTER(SCCEmit, sccemit)
}  // namespace maple
