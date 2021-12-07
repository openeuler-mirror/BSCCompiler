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
#include "me_phase_manager.h"

#define JAVALANG (mirModule.IsJavaModule())
#define CLANG (mirModule.IsCModule())

namespace maple {
bool MeFuncPM::genMeMpl = false;
bool MeFuncPM::timePhases = false;

void MeFuncPM::DumpMEIR(MeFunction &f, const std::string phaseName, bool isBefore) {
  bool dumpFunc = MeOption::DumpFunc(f.GetName());
  bool dumpPhase = MeOption::DumpPhase(phaseName);
  if (MeOption::dumpBefore && dumpFunc && dumpPhase && isBefore) {
    LogInfo::MapleLogger() << ">>>>> Dump before " << phaseName << " <<<<<\n";
    if (phaseName != "meemit") {
      f.Dump(false);
    } else {
      f.DumpFunctionNoSSA();
    }
    LogInfo::MapleLogger() << ">>>>> Dump before End <<<<<\n";
    return;
  }
  if ((MeOption::dumpAfter || dumpPhase) && dumpFunc && !isBefore) {
    LogInfo::MapleLogger() << ">>>>> Dump after " << phaseName << " <<<<<\n";
    if (phaseName != "meemit") {
      f.Dump(false);
    } else {
      f.DumpFunctionNoSSA();
    }
    LogInfo::MapleLogger() << ">>>>> Dump after End <<<<<\n\n";
  }
}

bool MeFuncPM::SkipFuncForMe(MIRModule &m, const MIRFunction &func, uint64 range) {
  // when partO2 is set, skip func which not exists in partO2FuncList.
  if (m.HasPartO2List() && !m.IsInPartO2List(func.GetNameStrIdx())) {
    return true;
  }
  if (func.IsEmpty() || (MeOption::useRange && (range < MeOption::range[0] || range > MeOption::range[1]))) {
    return true;
  }
  if (func.HasSetjmp()) {
    LogInfo::MapleLogger() << "Function  < " << func.GetName() << " not optimized because it has setjmp\n";
    return true;
  }
  return false;
}

bool MeFuncPM::PhaseRun(maple::MIRModule &m) {
  bool changed = false;
  auto &compFuncList = m.GetFunctionList();
  auto admMempool = AllocateMemPoolInPhaseManager("me phase manager's analysis data manager mempool");
  auto *serialADM = GetManagerMemPool()->New<AnalysisDataManager>(*(admMempool.get()));
  SetQuiet(MeOption::quiet);
  if (MeFuncPM::timePhases) {
    InitTimeHandler();
  }
  DoPhasesPopulate(m);
  for (size_t i = 0; i < compFuncList.size(); ++i) {
    MIRFunction *func = compFuncList[i];
    ASSERT_NOT_NULL(func);
    if (SkipFuncForMe(m, *func, i)) {
      continue;
    }
    m.SetCurFunction(func);
    if (!IsQuiet()) {
      LogInfo::MapleLogger() << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>> Optimizing Function  < " << func->GetName()
                             << " id=" << func->GetPuidxOrigin() << " >---\n";
    }
    /* prepare me func */
    auto meFuncMP = std::make_unique<ThreadLocalMemPool>(memPoolCtrler, "maple_me per-function mempool");
    auto meFuncStackMP = std::make_unique<StackMemPool>(memPoolCtrler, "");
    MemPool *versMP = new ThreadLocalMemPool(memPoolCtrler, "first verst mempool");
    MeFunction &meFunc = *(meFuncMP->New<MeFunction>(&m, func, meFuncMP.get(), *meFuncStackMP, versMP, meInput));
    func->SetMeFunc(&meFunc);
    meFunc.PartialInit();
#if DEBUG
    globalMIRModule = &m;
    globalFunc = &meFunc;
#endif
    if (!IsQuiet()) {
      LogInfo::MapleLogger() << "---Preparing Function  < " << func->GetName() << " > [" << i << "] ---\n";
    }
    meFunc.Prepare();
    FuncLevelRun(meFunc, *serialADM);
    serialADM->EraseAllAnalysisPhase();
  }
  if (genMeMpl) {
    m.Emit("comb.me.mpl");
  }
  if (MeFuncPM::timePhases) {
    DumpPhaseTime();
  }
  return changed;
}

bool MeFuncPM::FuncLevelRun(MeFunction &meFunc, AnalysisDataManager &serialADM) {
  bool changed = false;
  for (size_t i = 0; i < phasesSequence.size(); ++i) {
    SolveSkipFrom(MeOption::GetSkipFromPhase(), i);
    const MaplePhaseInfo *curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(phasesSequence[i]);
    if (!IsQuiet()) {
      LogInfo::MapleLogger() << "---Run Me " << (curPhase->IsAnalysis() ? "analysis" : "transform")
                             << " Phase [ " << curPhase->PhaseName() << " ]---\n";
    }
    DumpMEIR(meFunc, curPhase->PhaseName(), true);
    if (curPhase->IsAnalysis()) {
      changed |= RunAnalysisPhase<meFuncOptTy, MeFunction>(*curPhase, serialADM, meFunc);
    } else {
      changed |= RunTransformPhase<meFuncOptTy, MeFunction>(*curPhase, serialADM, meFunc);
    }
    DumpMEIR(meFunc, curPhase->PhaseName(), false);
    SolveSkipAfter(MeOption::GetSkipAfterPhase(), i);
  }
  return changed;
}

void MeFuncPM::DoPhasesPopulate(const maple::MIRModule &mirModule) {
#define ME_PHASE
#include "phases.def"
#undef ME_PHASE
}

void MeFuncPM::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<M2MClone>();
  aDep.AddRequired<M2MKlassHierarchy>();
  aDep.AddPreserved<M2MKlassHierarchy>();
}

MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MeFuncPM, meFuncPM)

MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MEMeCfg, mecfgbuild)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MEVerify, meverify)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MEAliasClass, aliasclass)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MESSATab, ssatab)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MELfoPreEmission, lfopreemit)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MEAnalyzeCtor, analyzector)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MEDominance, dominance)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MELoopAnalysis, identloops)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MELfoDepTest, deptest)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MESSADevirtual, ssadevirt)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MESideEffect, sideeffect)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MESSA, ssa)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MEBBLayout, bblayout)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MEIRMapBuild, irmapbuild)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MEPredict, predict)

MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEAnalyzeRC, analyzerc)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEStorePre, storepre)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MESubsumRC, subsumrc)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MESSARename2Preg, rename2preg)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEMeProp, hprop)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEMeSink, sink)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEIVOpts, ivopts)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MESyncSelect, syncselect)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEMay2Dassign, may2dassign)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MECondBasedRC, condbasedrc)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MECondBasedNPC, condbasednpc)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEPregRename, pregrename)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEStmtPre, stmtpre)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MECfgOpt, cfgopt)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEAutoVectorization, autovec)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MECFGOPT, cfgOpt)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MELoopUnrolling, loopunrolling)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEHdse, hdse)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MELfoIVCanon, ivcanon)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEValueRangePropagation, valueRangePropagation)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MESafetyWarning, safetyWarning)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEBypathEH, bypatheh)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEProfUse, profileUse)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEGCLowering, gclowering)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MELfoInjectIV, injectiv)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MECopyProp, copyprop)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MERCLowering, rclowering)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MESSALPre, lpre)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MELoopCanaon, loopcanon)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEDelegateRC, delegaterc)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEFSAA, fsaa)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MESplitCEdge, splitcriticaledge)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MECheckCastOpt, checkcastopt)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MESSAEPre, epre)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MESimplifyCFG, simplifyCFG)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEProfGen, profileGen)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEPlacementRC, placementrc)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEDse, dse)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEABCOpt, abcopt)
MAPLE_TRANSFORM_PHASE_REGISTER(MEEmit, meemit)
MAPLE_TRANSFORM_PHASE_REGISTER(EmitForIPA, emitforipa)
}  // namespace maple
