/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "cg_phasemanager.h"
#include <vector>
#include <string>
#include "cg_option.h"
#include "ebo.h"
#include "cfgo.h"
#include "ico.h"
#include "reaching.h"
#include "schedule.h"
#include "global.h"
#include "strldr.h"
#include "peep.h"
#if TARGAARCH64
#include "aarch64_fixshortbranch.h"
#elif TARGRISCV64
#include "riscv64_fixshortbranch.h"
#endif
#if TARGARM32
#include "live_range.h"
#endif
#include "cg_dominance.h"
#include "loop.h"
#include "cg_critical_edge.h"
#include "mpl_timer.h"
#include "args.h"
#include "yieldpoint.h"
#include "label_creation.h"
#include "offset_adjust.h"
#include "proepilog.h"
#include "ra_opt.h"
#include "alignment.h"

#if TARGAARCH64
#include "aarch64/aarch64_cg.h"
#include "aarch64/aarch64_emitter.h"
#elif TARGARM32
#include "arm32/arm32_cg.h"
#include "arm32/arm32_emitter.h"
#else
#error "Unsupported target"
#endif

namespace maplebe {
#define JAVALANG (module.IsJavaModule())
#define CLANG (module.GetSrcLang() == kSrcLangC)

#define RELEASE(pointer)      \
  do {                        \
    if (pointer != nullptr) { \
      delete pointer;         \
      pointer = nullptr;      \
    }                         \
  } while (0)

void CgFuncPM::GenerateOutPutFile(MIRModule &m) {
  CHECK_FATAL(cg != nullptr, "cg is null");
  CHECK_FATAL(cg->GetEmitter(), "emitter is null");
  if (!cgOptions->SuppressFileInfo()) {
    cg->GetEmitter()->EmitFileInfo(m.GetInputFileName());
  }
  if (cgOptions->WithDwarf()) {
    cg->GetEmitter()->EmitDIHeader();
  }
  InitProfile(m);
}

bool CgFuncPM::FuncLevelRun(CGFunc &cgFunc, AnalysisDataManager &serialADM) {
  bool changed = false;
  for (size_t i = 0; i < phasesSequence.size(); ++i) {
    SolveSkipFrom(CGOptions::GetSkipFromPhase(), i);
    const MaplePhaseInfo *curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(phasesSequence[i]);
    if (!IsQuiet()) {
      LogInfo::MapleLogger() << "---Run MplCG " << (curPhase->IsAnalysis() ? "analysis" : "transform")
                             << " Phase [ " << curPhase->PhaseName() << " ]---\n";
    }
    if (curPhase->IsAnalysis()) {
      changed |= RunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(*curPhase, serialADM, cgFunc);
    } else  {
      DumpFuncCGIR(cgFunc, curPhase->PhaseName(), true);
      changed |= RunTransformPhase<MapleFunctionPhase<CGFunc>, CGFunc>(*curPhase, serialADM, cgFunc);
      DumpFuncCGIR(cgFunc, curPhase->PhaseName(), false);
    }
    SolveSkipAfter(CGOptions::GetSkipAfterPhase(), i);
  }
  return changed;
}

void CgFuncPM::PostOutPut(MIRModule &m) {
  cg->GetEmitter()->EmitHugeSoRoutines(true);
  if (cgOptions->WithDwarf()) {
    cg->GetEmitter()->EmitDIFooter();
  }
  /* Emit global info */
  EmitGlobalInfo(m);
}

/* =================== new phase manager ===================  */
bool CgFuncPM::PhaseRun(MIRModule &m) {
  CreateCGAndBeCommon(m);
  bool changed = false;
  if (cgOptions->IsRunCG()) {
    GenerateOutPutFile(m);

    /* Run the cg optimizations phases */
    PrepareLower(m);

    uint32 countFuncId = 0;
    unsigned long rangeNum = 0;
    DoPhasesPopulate(m);

    auto admMempool = AllocateMemPoolInPhaseManager("cg phase manager's analysis data manager mempool");
    auto *serialADM = GetManagerMemPool()->New<AnalysisDataManager>(*(admMempool.get()));
    for (auto it = m.GetFunctionList().begin(); it != m.GetFunctionList().end(); ++it) {
      ASSERT(serialADM->CheckAnalysisInfoEmpty(), "clean adm before function run");
      MIRFunction *mirFunc = *it;
      if (mirFunc->GetBody() == nullptr) {
        continue;
      }
      if (!IsQuiet()) {
        LogInfo::MapleLogger() << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>> Optimizing Function  < " << mirFunc->GetName()
                               << " id=" << mirFunc->GetPuidxOrigin() << " >---\n";
      }
      /* LowerIR. */
      m.SetCurFunction(mirFunc);
      if (cg->DoConstFold()) {
        ConstantFold cf(m);
        cf.Simplify(mirFunc->GetBody());
      }

      DoFuncCGLower(m, *mirFunc);
      /* create CGFunc */
      MIRSymbol *funcSt = GlobalTables::GetGsymTable().GetSymbolFromStidx(mirFunc->GetStIdx().Idx());
      auto funcMp = std::make_unique<ThreadLocalMemPool>(memPoolCtrler, funcSt->GetName());
      auto stackMp = std::make_unique<StackMemPool>(funcMp->GetCtrler(), "");
      MapleAllocator funcScopeAllocator(funcMp.get());
      mirFunc->SetPuidxOrigin(++countFuncId);
      CGFunc *cgFunc = cg->CreateCGFunc(m, *mirFunc, *beCommon, *funcMp, *stackMp, funcScopeAllocator, countFuncId);
      CHECK_FATAL(cgFunc != nullptr, "Create CG Function failed in cg_phase_manager");
      CG::SetCurCGFunc(*cgFunc);

      if (cgOptions->WithDwarf()) {
        cgFunc->SetDebugInfo(m.GetDbgInfo());
      }
      /* Run the cg optimizations phases. */
      if (CGOptions::UseRange() && rangeNum >= CGOptions::GetRangeBegin() && rangeNum <= CGOptions::GetRangeEnd()) {
        CGOptions::EnableInRange();
      }
      changed = FuncLevelRun(*cgFunc, *serialADM);
      /* Delete mempool. */
      mirFunc->ReleaseCodeMemory();
      ++rangeNum;
      CGOptions::DisableInRange();
    }
    PostOutPut(m);
  } else {
    LogInfo::MapleLogger(kLlErr) << "Skipped generating .s because -no-cg is given" << '\n';
  }
  RELEASE(cg);
  RELEASE(beCommon);
  return changed;
}

void CgFuncPM::DoPhasesPopulate(const MIRModule &module) {
  ADDMAPLECGPHASE("layoutstackframe", true);
  ADDMAPLECGPHASE("createstartendlabel", true);
  ADDMAPLECGPHASE("buildehfunc", true);
  ADDMAPLECGPHASE("handlefunction", true);
  ADDMAPLECGPHASE("moveargs", true);
  ADDMAPLECGPHASE("ebo", CGOptions::DoEBO());
  ADDMAPLECGPHASE("prepeephole", CGOptions::DoPrePeephole())
  ADDMAPLECGPHASE("ico", CGOptions::DoICO())
  ADDMAPLECGPHASE("cfgo", !CLANG && CGOptions::DoCFGO());
#if TARGAARCH64
  ADDMAPLECGPHASE("storeloadopt", CGOptions::DoStoreLoadOpt())
  ADDMAPLECGPHASE("globalopt", CGOptions::DoGlobalOpt())
  ADDMAPLECGPHASE("clearrdinfo", (CGOptions::DoStoreLoadOpt()) || CGOptions::DoGlobalOpt())
#endif
  ADDMAPLECGPHASE("prepeephole1", CGOptions::DoPrePeephole())
  ADDMAPLECGPHASE("ebo1", CGOptions::DoEBO());
  ADDMAPLECGPHASE("prescheduling", !JAVALANG && CGOptions::DoPreSchedule());
  ADDMAPLECGPHASE("raopt", CGOptions::DoPreLSRAOpt());
  ADDMAPLECGPHASE("cgsplitcriticaledge", CLANG);
  ADDMAPLECGPHASE("regalloc", true);
  ADDMAPLECGPHASE("storeloadopt", CLANG && CGOptions::DoStoreLoadOpt())
  ADDMAPLECGPHASE("clearrdinfo", CLANG && (CGOptions::DoStoreLoadOpt() || CGOptions::DoGlobalOpt()))
  ADDMAPLECGPHASE("generateproepilog", true);
  ADDMAPLECGPHASE("offsetadjustforfplr", true);
  ADDMAPLECGPHASE("dbgfixcallframeoffsets", true);
  ADDMAPLECGPHASE("cfgo", CLANG && CGOptions::DoCFGO());
  ADDMAPLECGPHASE("peephole0", CGOptions::DoPeephole())
  ADDMAPLECGPHASE("postebo", CGOptions::DoEBO());
  ADDMAPLECGPHASE("postcfgo", CGOptions::DoCFGO());
  ADDMAPLECGPHASE("peephole", CGOptions::DoPeephole())
  ADDMAPLECGPHASE("gencfi", !CLANG);
  ADDMAPLECGPHASE("yieldpoint", JAVALANG && CGOptions::IsInsertYieldPoint());
  ADDMAPLECGPHASE("scheduling", CGOptions::DoSchedule());
  ADDMAPLECGPHASE("alignanalysis", CLANG);
  ADDMAPLECGPHASE("fixshortbranch", true);
  ADDMAPLECGPHASE("cgemit", true);
}

void CgFuncPM::DumpFuncCGIR(CGFunc &f, const std::string phaseName, bool isBefore) {
  bool dumpFunc = CGOptions::FuncFilter(f.GetName());
  bool dumpPhase = IS_STR_IN_SET(CGOptions::GetDumpPhases(), phaseName);
  if (CGOptions::IsDumpBefore() && dumpFunc && dumpPhase && isBefore) {
    LogInfo::MapleLogger() << "******** CG IR Before " << phaseName << ": *********" << "\n";
    f.DumpCGIR();
    return;
  }
  bool dumpPhases = CGOptions::DumpPhase(phaseName);
  if (((CGOptions::IsDumpAfter() && dumpPhase) || dumpPhases) && dumpFunc && !isBefore) {
    if (phaseName == "buildehfunc") {
      LogInfo::MapleLogger() << "******** Maple IR After buildehfunc: *********" << "\n";
      f.GetFunction().Dump();
    }
    LogInfo::MapleLogger() << "******** CG IR After " << phaseName << ": *********" << "\n";
    f.DumpCGIR();
  }
}

void CgFuncPM::EmitGlobalInfo(MIRModule &m) const {
  EmitDuplicatedAsmFunc(m);
  EmitFastFuncs(m);
  if (cgOptions->IsGenerateObjectMap()) {
    cg->GenerateObjectMaps(*beCommon);
  }
  cg->GetEmitter()->EmitGlobalVariable();
  EmitDebugInfo(m);
  cg->GetEmitter()->CloseOutput();
}

void CgFuncPM::InitProfile(MIRModule &m)  const {
  if (!CGOptions::IsProfileDataEmpty()) {
    uint32 dexNameIdx = m.GetFileinfo(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("INFO_filename"));
    const std::string &dexName = GlobalTables::GetStrTable().GetStringFromStrIdx(GStrIdx(dexNameIdx));
    bool deCompressSucc = m.GetProfile().DeCompress(CGOptions::GetProfileData(), dexName);
    if (!deCompressSucc) {
      LogInfo::MapleLogger() << "WARN: DeCompress() " << CGOptions::GetProfileData() << "failed in mplcg()\n";
    }
  }
}

void CgFuncPM::CreateCGAndBeCommon(MIRModule &m) {
  ASSERT(cgOptions != nullptr, "New cg phase manager running FAILED  :: cgOptions unset");
#if TARGAARCH64 || TARGRISCV64
  cg = new AArch64CG(m, *cgOptions, cgOptions->GetEHExclusiveFunctionNameVec(), CGOptions::GetCyclePatternMap());
  cg->SetEmitter(*m.GetMemPool()->New<AArch64AsmEmitter>(*cg, m.GetOutputFileName()));
#elif TARGARM32
  cg = new Arm32CG(m, *cgOptions, cgOptions->GetEHExclusiveFunctionNameVec(), CGOptions::GetCyclePatternMap());
  cg->SetEmitter(*m.GetMemPool()->New<Arm32AsmEmitter>(*cg, m.GetOutputFileName()));
#else
#error "unknown platform"
#endif
  /*
   * Must be done before creating any BECommon instances.
   *
   * BECommon, when constructed, will calculate the type, size and align of all types.  As a side effect, it will also
   * lower ptr and ref types into a64. That will drop the information of what a ptr or ref points to.
   *
   * All metadata generation passes which depend on the pointed-to type must be done here.
   */
  cg->GenPrimordialObjectList(m.GetBaseName());
  /* We initialize a couple of BECommon's tables using the size information of GlobalTables.type_table_.
   * So, BECommon must be allocated after all the parsing is done and user-defined types are all acounted.
   */
  beCommon = new BECommon(m);
  Globals::GetInstance()->SetBECommon(*beCommon);

  /* If a metadata generation pass depends on object layout it must be done after creating BECommon. */
  cg->GenExtraTypeMetadata(cgOptions->GetClassListFile(), m.GetBaseName());

  if (cg->NeedInsertInstrumentationFunction()) {
    CHECK_FATAL(cgOptions->IsInsertCall(), "handling of --insert-call is not correct");
    cg->SetInstrumentationFunction(cgOptions->GetInstrumentationFunction());
  }
  if (!m.IsCModule()) {
    CGOptions::EnableFramePointer();
  }
}

void CgFuncPM::PrepareLower(MIRModule &m) {
  mirLower = GetManagerMemPool()->New<MIRLower>(m, nullptr);
  mirLower->Init();
  cgLower = GetManagerMemPool()->New<CGLowerer>(m,
      *beCommon, cg->GenerateExceptionHandlingCode(), cg->GenerateVerboseCG());
  cgLower->RegisterBuiltIns();
  if (m.IsJavaModule()) {
    cgLower->InitArrayClassCacheTableIndex();
  }
  cgLower->RegisterExternalLibraryFunctions();
  cgLower->SetCheckLoadStore(CGOptions::IsCheckArrayStore());
  if (cg->AddStackGuard() || m.HasPartO2List()) {
    cg->AddStackGuardvar();
  }
}

void CgFuncPM::DoFuncCGLower(const MIRModule &m, MIRFunction &mirFunc) {
  if (m.GetFlavor() <= kFeProduced) {
    mirLower->SetLowerCG();
    mirLower->SetMirFunc(&mirFunc);
    mirLower->LowerFunc(mirFunc);
  }
  bool dumpAll = (CGOptions::GetDumpPhases().find("*") != CGOptions::GetDumpPhases().end());
  bool dumpFunc = CGOptions::FuncFilter(mirFunc.GetName());
  if (!cg->IsQuiet() || (dumpAll && dumpFunc)) {
    LogInfo::MapleLogger() << "************* before CGLowerer **************" << '\n';
    mirFunc.Dump();
  }
  cgLower->LowerFunc(mirFunc);
  if (!cg->IsQuiet() || (dumpAll && dumpFunc)) {
    LogInfo::MapleLogger() << "************* after  CGLowerer **************" << '\n';
    mirFunc.Dump();
    LogInfo::MapleLogger() << "************* end    CGLowerer **************" << '\n';
  }
}

void CgFuncPM::EmitDuplicatedAsmFunc(MIRModule &m) const {
  if (CGOptions::IsDuplicateAsmFileEmpty()) {
    return;
  }

  struct stat buffer;
  if (stat(CGOptions::GetDuplicateAsmFile().c_str(), &buffer) != 0) {
    return;
  }

  std::ifstream duplicateAsmFileFD(CGOptions::GetDuplicateAsmFile());

  if (!duplicateAsmFileFD.is_open()) {
    duplicateAsmFileFD.close();
    ERR(kLncErr, " %s open failed!", CGOptions::GetDuplicateAsmFile().c_str());
    return;
  }
  std::string contend;
  bool onlyForFramework = false;
  bool isFramework = IsFramework(m);

  while (getline(duplicateAsmFileFD, contend)) {
    if (!contend.compare("#Libframework_start")) {
      onlyForFramework = true;
    }

    if (!contend.compare("#Libframework_end")) {
      onlyForFramework = false;
    }

    if (onlyForFramework && !isFramework) {
      continue;
    }

    (void)cg->GetEmitter()->Emit(contend + "\n");
  }
  duplicateAsmFileFD.close();
}

void CgFuncPM::EmitFastFuncs(const MIRModule &m) const {
  if (CGOptions::IsFastFuncsAsmFileEmpty() || !(m.IsJavaModule())) {
    return;
  }

  struct stat buffer;
  if (stat(CGOptions::GetFastFuncsAsmFile().c_str(), &buffer) != 0) {
    return;
  }

  std::ifstream fastFuncsAsmFileFD(CGOptions::GetFastFuncsAsmFile());
  if (fastFuncsAsmFileFD.is_open()) {
    std::string contend;
    (void)cg->GetEmitter()->Emit("#define ENABLE_LOCAL_FAST_FUNCS 1\n");

    while (getline(fastFuncsAsmFileFD, contend)) {
      (void)cg->GetEmitter()->Emit(contend + "\n");
    }
  }
  fastFuncsAsmFileFD.close();
}

void CgFuncPM::EmitDebugInfo(const MIRModule &m) const {
  if (!cgOptions->WithDwarf()) {
    return;
  }
  cg->GetEmitter()->SetupDBGInfo(m.GetDbgInfo());
  cg->GetEmitter()->EmitDIHeaderFileInfo();
  cg->GetEmitter()->EmitDIDebugInfoSection(m.GetDbgInfo());
  cg->GetEmitter()->EmitDIDebugAbbrevSection(m.GetDbgInfo());
  cg->GetEmitter()->EmitDIDebugARangesSection();
  cg->GetEmitter()->EmitDIDebugRangesSection();
  cg->GetEmitter()->EmitDIDebugLineSection();
  cg->GetEmitter()->EmitDIDebugStrSection();
}

bool CgFuncPM::IsFramework(MIRModule &m) const {
  auto &funcList = m.GetFunctionList();
  for (auto it = funcList.begin(); it != funcList.end(); ++it) {
    MIRFunction *mirFunc = *it;
    ASSERT(mirFunc != nullptr, "nullptr check");
    if (mirFunc->GetBody() != nullptr &&
        mirFunc->GetName() == "Landroid_2Fos_2FParcel_3B_7CnativeWriteString_7C_28JLjava_2Flang_2FString_3B_29V") {
      return true;
    }
  }
  return false;
}
MAPLE_TRANSFORM_PHASE_REGISTER(CgFuncPM, cgFuncPhaseManager)

MAPLE_ANALYSIS_PHASE_REGISTER(CgLiveAnalysis, liveanalysis)
MAPLE_ANALYSIS_PHASE_REGISTER(CgLoopAnalysis, loopanalysis)
MAPLE_ANALYSIS_PHASE_REGISTER(CgReachingDefinition, reachingdefinition)

MAPLE_TRANSFORM_PHASE_REGISTER(CgHandleFunction, handlefunction)
MAPLE_TRANSFORM_PHASE_REGISTER(CgFixCFLocOsft, dbgfixcallframeoffsets)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgGlobalOpt, globalopt)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgPreScheduling, prescheduling)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgScheduling, scheduling)
MAPLE_TRANSFORM_PHASE_REGISTER(CgGenProEpiLog, generateproepilog)
MAPLE_TRANSFORM_PHASE_REGISTER(CgMoveRegArgs, moveargs)
MAPLE_TRANSFORM_PHASE_REGISTER(CgCreateLabel, createstartendlabel)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgCfgo, cfgo)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgPostCfgo, postcfgo)
MAPLE_TRANSFORM_PHASE_REGISTER(CgYieldPointInsertion, yieldpoint)
MAPLE_TRANSFORM_PHASE_REGISTER(CgRaOpt, raopt)
MAPLE_TRANSFORM_PHASE_REGISTER(CgBuildEHFunc, buildehfunc)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgPrePeepHole0, prepeephole)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgPrePeepHole1, prepeephole1)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgPeepHole0, peephole0)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgPeepHole1, peephole)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgEbo0, ebo)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgEbo1, ebo1)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgPostEbo, postebo)
MAPLE_TRANSFORM_PHASE_REGISTER(CgClearRDInfo, clearrdinfo)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgIco, ico)
MAPLE_TRANSFORM_PHASE_REGISTER(CgLayoutFrame, layoutstackframe)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgStoreLoadOpt, storeloadopt)
MAPLE_TRANSFORM_PHASE_REGISTER(CgFPLROffsetAdjustment, offsetadjustforfplr)
MAPLE_TRANSFORM_PHASE_REGISTER(CgEmission, cgemit)
MAPLE_TRANSFORM_PHASE_REGISTER(CgAlignAnalysis, alignanalysis)
MAPLE_TRANSFORM_PHASE_REGISTER(CgFixShortBranch, fixshortbranch)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgCriticalEdge, cgsplitcriticaledge)
MAPLE_TRANSFORM_PHASE_REGISTER(CgRegAlloc, regalloc)
MAPLE_TRANSFORM_PHASE_REGISTER(CgGenCfi, gencfi)
}  /* namespace maplebe */
