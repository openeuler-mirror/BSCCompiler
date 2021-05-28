/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "live.h"
#include "loop.h"
#include "mpl_timer.h"
#include "args.h"
#include "yieldpoint.h"
#include "label_creation.h"
#include "offset_adjust.h"
#include "proepilog.h"
#include "ra_opt.h"
namespace maplebe {
#define JAVALANG (module.IsJavaModule())
#define CLANG (module.GetSrcLang() == kSrcLangC)

void CgFuncPhaseManager::RunFuncPhase(CGFunc &func, FuncPhase &phase) {
  /*
   * 1. check options.enable(phase.id())
   * 2. options.tracebeforePhase(phase.id()) dumpIR before
   */
  if (!CGOptions::IsQuiet()) {
    LogInfo::MapleLogger() << "---Run Phase [ " << phase.PhaseName() << " ]---\n";
  }

  /* 3. run: skip mplcg phase except "emit" if no cfg in CGFunc */
  AnalysisResult *analysisRes = nullptr;
  if ((func.NumBBs() > 0) || (phase.GetPhaseID() == kCGFuncPhaseEMIT)) {
    analysisRes = phase.Run(&func, &arFuncManager);
    phase.ClearMemPoolsExcept(analysisRes == nullptr ? nullptr : analysisRes->GetMempool());
  }

  if (analysisRes != nullptr) {
    /* if phase is an analysis Phase, add result to arm */
    arFuncManager.AddResult(phase.GetPhaseID(), func, *analysisRes);
  }
}

void CgFuncPhaseManager::RegisterFuncPhases() {
  /* register all Funcphases defined in cg_phases.def */
#define FUNCTPHASE(id, cgPhase)                                                                           \
  do {                                                                                                    \
    RegisterPhase(id, *(new (GetMemAllocator()->GetMemPool()->Malloc(sizeof(cgPhase(id)))) cgPhase(id))); \
  } while (0);

#define FUNCAPHASE(id, cgPhase)                                                                           \
  do {                                                                                                    \
    RegisterPhase(id, *(new (GetMemAllocator()->GetMemPool()->Malloc(sizeof(cgPhase(id)))) cgPhase(id))); \
    arFuncManager.AddAnalysisPhase(id, (static_cast<FuncPhase*>(GetPhase(id))));                          \
  } while (0);
#include "cg_phases.def"
#undef FUNCTPHASE
#undef FUNCAPHASE
}

#define ADDPHASE(phaseName)                 \
  if (!CGOptions::IsSkipPhase(phaseName)) { \
    phases.push_back(phaseName);            \
  }

void CgFuncPhaseManager::AddPhases(std::vector<std::string> &phases) {
  if (phases.empty()) {
    if (cgPhaseType == kCgPhaseMainOpt) {
      /* default phase sequence */
      ADDPHASE("layoutstackframe");
      ADDPHASE("createstartendlabel");
      ADDPHASE("buildehfunc");
      ADDPHASE("handlefunction");
      ADDPHASE("moveargs");

      if (CGOptions::DoEBO()) {
        ADDPHASE("ebo");
      }
      if (CGOptions::DoPrePeephole()) {
        ADDPHASE("prepeephole");
      }
      if (CGOptions::DoICO()) {
        ADDPHASE("ico");
      }
      if (!CLANG && CGOptions::DoCFGO()) {
        ADDPHASE("cfgo");
      }

#if TARGAARCH64 || TARGRISCV64
      if (JAVALANG && CGOptions::DoStoreLoadOpt()) {
        ADDPHASE("storeloadopt");
      }

      if (CGOptions::DoGlobalOpt()) {
        ADDPHASE("globalopt");
      }

      if ((JAVALANG && CGOptions::DoStoreLoadOpt()) || CGOptions::DoGlobalOpt()) {
        ADDPHASE("clearrdinfo");
      }
#endif

      if (CGOptions::DoPrePeephole()) {
        ADDPHASE("prepeephole1");
      }

      if (CGOptions::DoEBO()) {
        ADDPHASE("ebo1");
      }
      if (CGOptions::DoPreSchedule()) {
        ADDPHASE("prescheduling");
      }

     if (!CGOptions::DoPreLSRAOpt()) {
     } else {
       ADDPHASE("raopt");
     }

      ADDPHASE("regalloc");
      ADDPHASE("generateproepilog");
      ADDPHASE("offsetadjustforfplr");

      if (CGOptions::DoPeephole()) {
        ADDPHASE("peephole0");
      }

      if (CGOptions::DoEBO()) {
        ADDPHASE("postebo");
      }
      if (CGOptions::DoCFGO()) {
        ADDPHASE("postcfgo");
      }
      if (CGOptions::DoPeephole()) {
        ADDPHASE("peephole");
      }

      ADDPHASE("gencfi");
      if (JAVALANG && CGOptions::IsInsertYieldPoint()) {
        ADDPHASE("yieldpoint");
      }

      if (CGOptions::DoSchedule()) {
        ADDPHASE("scheduling");
      }
      ADDPHASE("fixshortbranch");

      ADDPHASE("emit");
    }
  }
  for (const auto &phase : phases) {
    AddPhase(phase);
  }
  ASSERT(phases.size() == GetPhaseSequence()->size(), "invalid phase name");
}

void CgFuncPhaseManager::Emit(CGFunc &func) {
  PhaseID id = kCGFuncPhaseEMIT;
  FuncPhase *funcPhase = static_cast<FuncPhase*>(GetPhase(id));
  CHECK_FATAL(funcPhase != nullptr, "funcPhase is null in CgFuncPhaseManager::Run");

  const std::string kPhaseBeforeEmit = "fixshortbranch";
  funcPhase->SetPreviousPhaseName(kPhaseBeforeEmit); /* prev phase name is for filename used in emission after phase */
  MPLTimer timer;
  bool timePhases = CGOptions::IsEnableTimePhases();
  if (timePhases) {
    timer.Start();
  }
  RunFuncPhase(func, *funcPhase);
  if (timePhases) {
    timer.Stop();
    phaseTimers.back() += timer.ElapsedMicroseconds();
  }

  const std::string &phaseName = funcPhase->PhaseName();  /* new phase name */
  bool dumpPhases = CGOptions::DumpPhase(phaseName);
  bool dumpFunc = CGOptions::FuncFilter(func.GetName());
  if (((CGOptions::IsDumpAfter() && dumpPhases) || dumpPhases) && dumpFunc) {
    LogInfo::MapleLogger() << "******** CG IR After " << phaseName << ": *********" << "\n";
    func.DumpCGIR();
  }
}

void CgFuncPhaseManager::Run(CGFunc &func) {
  if (!CGOptions::IsQuiet()) {
    LogInfo::MapleLogger() << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>> Optimizing Function  < " << func.GetName() << " >---\n";
  }
  std::string phaseName = "";
  /* each function level phase */
  bool dumpFunc = CGOptions::FuncFilter(func.GetName());
  size_t phaseIndex = 0;
  bool timePhases = CGOptions::IsEnableTimePhases();
  bool skipFromFlag = false;
  bool skipAfterFlag = false;
  MPLTimer timer;
  MPLTimer iteratorTimer;
  if (timePhases) {
    iteratorTimer.Start();
  }
  time_t loopBodyTime = 0;
  for (auto it = PhaseSequenceBegin(); it != PhaseSequenceEnd(); it++, ++phaseIndex) {
    PhaseID id = GetPhaseId(it);
    if (id == kCGFuncPhaseEMIT) {
      continue;
    }
    FuncPhase *funcPhase = static_cast<FuncPhase*>(GetPhase(id));
    CHECK_FATAL(funcPhase != nullptr, "funcPhase is null in CgFuncPhaseManager::Run");

    if (!skipFromFlag && CGOptions::IsSkipFromPhase(funcPhase->PhaseName())) {
      skipFromFlag = true;
    }
    if (skipFromFlag) {
      while (funcPhase->CanSkip() && (++it != PhaseSequenceEnd())) {
        id = GetPhaseId(it);
        funcPhase = static_cast<FuncPhase*>(GetPhase(id));
        CHECK_FATAL(funcPhase != nullptr, "null ptr check ");
      }
    }

    funcPhase->SetPreviousPhaseName(phaseName); /* prev phase name is for filename used in emission after phase */
    phaseName = funcPhase->PhaseName();         /* new phase name */
    if (CGOptions::UseRange()) {
      if (!CGOptions::IsInRange() &&
          ((phaseName == "ebo") || (phaseName == "ebo1") || (phaseName == "postebo") ||
           (phaseName == "ico") || (phaseName == "cfgo") ||
           (phaseName == "peephole0") || (phaseName == "peephole"))) {
        continue;
      }
    }
    bool dumpPhase = IS_STR_IN_SET(CGOptions::GetDumpPhases(), phaseName);
    if (CGOptions::IsDumpBefore() && dumpFunc && dumpPhase) {
      LogInfo::MapleLogger() << "******** CG IR Before " << phaseName << ": *********" << "\n";
      func.DumpCGIR();
    }
    if (timePhases) {
      timer.Start();
    }
    RunFuncPhase(func, *funcPhase);
    if (timePhases) {
      timer.Stop();
      CHECK_FATAL(phaseIndex < phaseTimers.size(), "invalid index for phaseTimers");
      phaseTimers[phaseIndex] += timer.ElapsedMicroseconds();
      loopBodyTime += timer.ElapsedMicroseconds();
    }
    bool dumpPhases = CGOptions::DumpPhase(phaseName);
    if (((CGOptions::IsDumpAfter() && dumpPhase) || dumpPhases) && dumpFunc) {
      if (id == kCGFuncPhaseBUILDEHFUNC) {
        LogInfo::MapleLogger() << "******** Maple IR After buildehfunc: *********" << "\n";
        func.GetFunction().Dump();
      }
      LogInfo::MapleLogger() << "******** CG IR After " << phaseName << ": *********" << "\n";
      func.DumpCGIR();
    }
    if (!skipAfterFlag && CGOptions::IsSkipAfterPhase(funcPhase->PhaseName())) {
      skipAfterFlag = true;
    }
    if (skipAfterFlag) {
      while (++it != PhaseSequenceEnd()) {
        id = GetPhaseId(it);
        funcPhase = static_cast<FuncPhase*>(GetPhase(id));
        CHECK_FATAL(funcPhase != nullptr, "null ptr check ");
        if (!funcPhase->CanSkip()) {
          break;
        }
      }
      --it;  /* restore iterator */
    }
  }
  if (timePhases) {
    iteratorTimer.Stop();
    extraPhasesTimer["iterator"] += iteratorTimer.ElapsedMicroseconds() - loopBodyTime;
  }
}

void CgFuncPhaseManager::ClearPhaseNameInfo() {
  for (auto it = PhaseSequenceBegin(); it != PhaseSequenceEnd(); ++it) {
    PhaseID id = GetPhaseId(it);
    FuncPhase *funcPhase = static_cast<FuncPhase*>(GetPhase(id));
    if (funcPhase == nullptr) {
      continue;
    }
    funcPhase->ClearString();
  }
}

int64 CgFuncPhaseManager::GetOptimizeTotalTime() const {
  int64 total = 0;
  for (size_t i = 0; i < phaseTimers.size(); ++i) {
    total += phaseTimers[i];
  }
  return total;
}

int64 CgFuncPhaseManager::GetExtraPhasesTotalTime() const {
  int64 total = 0;
  for (auto &timer : extraPhasesTimer) {
    total += timer.second;
  }
  return total;
}

time_t CgFuncPhaseManager::parserTime = 0;

int64 CgFuncPhaseManager::DumpCGTimers() {
  auto TimeLogger = [](const std::string &itemName, time_t itemTimeUs, time_t totalTimeUs) {
    LogInfo::MapleLogger() << std::left << std::setw(25) << itemName <<
                              std::setw(10) << std::right << std::fixed << std::setprecision(2) <<
                              (kPercent * itemTimeUs / totalTimeUs) << "%" << std::setw(10) <<
                              std::setprecision(0) << (itemTimeUs / kMicroSecPerMilliSec) << "ms\n";
  };
  int64 parseTimeTotal = parserTime;
  if (parseTimeTotal != 0) {
    LogInfo::MapleLogger() << "==================== PARSER ====================\n";
    TimeLogger("parser", parserTime, parseTimeTotal);
  }

  int64 phasesTotal = GetOptimizeTotalTime();
  phasesTotal += GetExtraPhasesTotalTime();
  std::ios::fmtflags flag(LogInfo::MapleLogger().flags());
  LogInfo::MapleLogger() << "================== TIMEPHASES ==================\n";
  for (auto &extraTimer : extraPhasesTimer) {
    TimeLogger(extraTimer.first, extraTimer.second, phasesTotal);
  }
  LogInfo::MapleLogger() << "================================================\n";
  for (size_t i = 0; i < phaseTimers.size(); ++i) {
    CHECK_FATAL(phasesTotal != 0, "calculation check");
    /*
     * output information by specified format, setw function parameter specifies show width
     * setprecision function parameter specifies precision
     */
    TimeLogger(registeredPhases[phaseSequences[i]]->PhaseName(), phaseTimers[i], phasesTotal);
  }
  LogInfo::MapleLogger() << "================================================\n";
  LogInfo::MapleLogger() << "=================== SUMMARY ====================\n";
  std::vector<std::pair<std::string, time_t>> timeSum;
  if (parseTimeTotal != 0) {
    timeSum.emplace_back(std::pair<std::string, time_t>{ "parser", parseTimeTotal });
  }
  timeSum.emplace_back(std::pair<std::string, time_t>{ "cgphase", phasesTotal });
  int64 total = parseTimeTotal + phasesTotal;
  timeSum.emplace_back(std::pair<std::string, time_t>{ "Total", total });
  for (size_t i = 0; i < timeSum.size(); ++i) {
    TimeLogger(timeSum[i].first, timeSum[i].second, total);
  }
  LogInfo::MapleLogger() << "================================================\n";
  LogInfo::MapleLogger().flags(flag);
  return phasesTotal;
}
}  /* namespace maplebe */
