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
#include "bin_mplt.h"
#include "becommon.h"
#include "lower.h"
#include "lmbc_memlayout.h"
#include "lmbc_lower.h"

#define JAVALANG (mirModule.IsJavaModule())
#define CLANG (mirModule.IsCModule())

namespace maple {
bool MeFuncPM::genMeMpl = false;
bool MeFuncPM::genMapleBC = false;
bool MeFuncPM::genLMBC = false;
bool MeFuncPM::timePhases = false;

void MeFuncPM::DumpMEIR(const MeFunction &f, const std::string phaseName, bool isBefore) {
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
    if (phaseName != "meemit" && phaseName != "lfounroll") {
      f.Dump(false);
    } else {
      f.DumpFunctionNoSSA();
    }
    LogInfo::MapleLogger() << ">>>>> Dump after End <<<<<\n\n";
  }
}

bool MeFuncPM::SkipFuncForMe(const MIRFunction &func, uint64 range) {
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
  auto userDefinedOptLevel = MeOption::optLevel;
  DoPhasesPopulate(m);
  size_t i = 0;
  for (auto func : compFuncList) {
    ++i;
    ASSERT_NOT_NULL(func);
    if (SkipFuncForMe(*func, i - 1)) {
      if (!func->IsEmpty()) {
        // mir cg lower should not be skipped
        MIRLower mirLower(m, func);
        mirLower.SetLowerCG();
        mirLower.SetMirFunc(func);
        mirLower.LowerFunc(*func);
      }
      continue;
    }
    if (userDefinedOptLevel == 2 && m.HasPartO2List()) {
      if (m.IsInPartO2List(func->GetNameStrIdx())) {
        MeOption::optLevel = 2;
      } else {
        MeOption::optLevel = 0;
      }
      ClearAllPhases();
      DoPhasesPopulate(m);
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
    if (genLMBC) {
      meFunc.genLMBC = true;
    }
    func->SetMeFunc(&meFunc);
    meFunc.PartialInit();
#if DEBUG
    globalMIRModule = &m;
    globalFunc = &meFunc;
#endif
    if (!IsQuiet()) {
      LogInfo::MapleLogger() << "---Preparing Function  < " << func->GetName() << " > [" << i - 1 << "] ---\n";
    }
    meFunc.Prepare();
    (void)FuncLevelRun(meFunc, *serialADM);
    meFunc.Release();
    serialADM->EraseAllAnalysisPhase();
  }
  if (genMeMpl) {
    m.Emit("comb.me.mpl");
  }
  if (genMapleBC) {
    m.SetFlavor(kFlavorMbc);
    // output .mbc
    BinaryMplt binMplt(m);
    std::string modFileName = m.GetFileName();
    std::string::size_type lastdot = modFileName.find_last_of(".");

    binMplt.GetBinExport().not2mplt = true;
    std::string filestem = modFileName.substr(0, lastdot);
    binMplt.Export(filestem + ".mbc", nullptr);
  }
  if (genLMBC) {
    m.SetFlavor(kFlavorLmbc);
    GlobalMemLayout globalMemLayout(&m, &m.GetMPAllocator());
    maplebe::BECommon beCommon(m);
    maplebe::CGLowerer cgLower(m, beCommon, false, false);
    cgLower.RegisterBuiltIns();
    cgLower.RegisterExternalLibraryFunctions();
    for (auto func : compFuncList) {
      if (func->GetBody() == nullptr) {
        continue;
      }
      if (!IsQuiet()) {
        LogInfo::MapleLogger() << ">>>> Generating LMBC for Function  < " << func->GetName() << " >\n";
      }
      m.SetCurFunction(func);
      cgLower.LowerFunc(*func);
      MemPool *layoutMp = memPoolCtrler.NewMemPool("layout mempool", true);
      MapleAllocator layoutAlloc(layoutMp);
      LMBCMemLayout localMemLayout(func, &globalMemLayout.seg_GPbased, &layoutAlloc);
      localMemLayout.LayoutStackFrame();
      LMBCLowerer lmbcLowerer(&m, &beCommon, func, &globalMemLayout, &localMemLayout);
      lmbcLowerer.LowerFunction();
      func->SetFrameSize(localMemLayout.StackFrameSize());
      memPoolCtrler.DeleteMemPool(layoutMp);
    }
    globalMemLayout.seg_GPbased.size =
        maplebe::RoundUp(static_cast<uint64>(globalMemLayout.seg_GPbased.size), GetPrimTypeSize(PTY_ptr));
    m.SetGlobalMemSize(globalMemLayout.seg_GPbased.size);
    // output .lmbc
    BinaryMplt binMplt(m);
    std::string modFileName = m.GetFileName();
    std::string::size_type lastdot = modFileName.find_last_of(".");

    binMplt.GetBinExport().not2mplt = true;
    std::string filestem = modFileName.substr(0, lastdot);
    binMplt.Export(filestem + ".lmbc", nullptr);
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
      changed |= RunAnalysisPhase<MeFuncOptTy, MeFunction>(*curPhase, serialADM, meFunc);
    } else {
      changed |= RunTransformPhase<MeFuncOptTy, MeFunction>(*curPhase, serialADM, meFunc);
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
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MECfgVerifyFrequency, cfgverifyfreq)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MEVerify, meverify)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MEAliasClass, aliasclass)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MESSATab, ssatab)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MEPreMeEmission, premeemit)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MEAnalyzeCtor, analyzector)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MEDominance, dominance)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MELoopAnalysis, identloops)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MELfoDepTest, deptest)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MESSADevirtual, ssadevirt)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MESideEffect, sideeffect)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MESSA, ssa)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(METopLevelSSA, toplevelssa)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MEBBLayout, bblayout)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MEIRMapBuild, irmapbuild)
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(MEPredict, predict)

MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEAnalyzeRC, analyzerc)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEStorePre, storepre)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MESubsumRC, subsumrc)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MESSARename2Preg, rename2preg)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEMeProp, hprop)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MESSAProp, ssaprop)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEMeSink, sink)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEIVOpts, ivopts)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MECodeFactoring, codefactoring)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MESyncSelect, syncselect)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEMay2Dassign, may2dassign)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MECondBasedRC, condbasedrc)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MECondBasedNPC, condbasednpc)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEPregRename, pregrename)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEStmtPre, stmtpre)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MECfgOpt, cfgopt)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEAutoVectorization, autovec)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MELfoUnroll, lfounroll)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MECFGOPT, cfgOpt)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MELoopUnrolling, loopunrolling)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEHdse, hdse)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MELfoIVCanon, ivcanon)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEValueRangePropagation, valueRangePropagation)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEJumpThreading, jumpThreading)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MESafetyWarning, safetyWarning)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEBypathEH, bypatheh)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEProfUse, profileUse)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEGCLowering, gclowering)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MELfoInjectIV, injectiv)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MECopyProp, copyprop)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MERCLowering, rclowering)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MESSALPre, lpre)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MELoopCanon, loopcanon)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MELoopInversion, loopinversion)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEDelegateRC, delegaterc)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEFSAA, fsaa)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MESplitCEdge, splitcriticaledge)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MECheckCastOpt, checkcastopt)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MESSAEPre, epre)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEOptimizeCFGNoSSA, optimizeCFGNoSSA)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEOptimizeCFG, optimizeCFG)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MESLPVectorizer, slp)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEProfGen, profileGen)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEPlacementRC, placementrc)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEDse, dse)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEABCOpt, abcopt)
MAPLE_TRANSFORM_PHASE_REGISTER(MEEmit, meemit)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(ProfileGenEmit, profgenEmit);
}  // namespace maple
