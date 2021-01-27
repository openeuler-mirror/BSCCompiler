/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include <iostream>
#include <vector>
#include <string>
#include "me_dominance.h"
#include "me_phase.h"
#include "me_cfg.h"
#include "me_alias_class.h"
#include "me_bypath_eh.h"
#include "me_critical_edge.h"
#include "me_profile_gen.h"
#include "me_profile_use.h"
#include "me_loop_canon.h"
#include "me_analyzector.h"
#include "me_abco.h"
#include "me_dse.h"
#include "me_hdse.h"
#include "me_prop.h"
#include "me_rename2preg.h"
#include "me_ssa_lpre.h"
#include "me_ssa_epre.h"
#include "me_stmt_pre.h"
#include "me_store_pre.h"
#include "me_cond_based_rc.h"
#include "me_cond_based_npc.h"
#include "preg_renamer.h"
#include "me_ssa_devirtual.h"
#include "me_delegate_rc.h"
#include "me_analyze_rc.h"
#include "me_may2dassign.h"
#include "me_loop_analysis.h"
#include "me_ssa.h"
#include "me_irmap_build.h"
#include "me_bb_layout.h"
#include "me_emit.h"
#include "me_rc_lowering.h"
#include "gen_check_cast.h"
#include "me_ssa_tab.h"
#include "mpl_timer.h"

#define JAVALANG (mirModule.IsJavaModule())

namespace maple {
void MeFuncPhaseManager::RunFuncPhase(MeFunction *func, MeFuncPhase *phase) {
  // 1. check options.enable(phase.id())
  // 2. options.tracebeforePhase(phase.id()) dumpIR before
  if (!MeOption::quiet) {
    LogInfo::MapleLogger() << "---Run Phase [ " << phase->PhaseName() << " ]---\n";
  }
  // 3. tracetime(phase.id())
  // 4. run: skip mplme phase except "emit" if no cfg in MeFunction
  AnalysisResult *analysisRes = nullptr;
  MePhaseID phaseID = phase->GetPhaseId();
  if ((func->NumBBs() > 0) || (phaseID == MeFuncPhase_EMIT)) {
    analysisRes = phase->Run(func, &arFuncManager, modResMgr);
    phase->ClearMemPoolsExcept(analysisRes == nullptr ? nullptr : analysisRes->GetMempool());
    phase->ClearString();
  }
  if (analysisRes != nullptr) {
    // if phase is an analysis Phase, add result to arm
    arFuncManager.AddResult(phase->GetPhaseId(), *func, *analysisRes);
  }
}

void MeFuncPhaseManager::RegisterFuncPhases() {
  // register all Funcphases defined in me_phases.def
#define FUNCTPHASE(id, mePhase)                                               \
  do {                                                                        \
    void *buf = GetMemAllocator()->GetMemPool()->Malloc(sizeof(mePhase(id))); \
    CHECK_FATAL(buf != nullptr, "null ptr check");                            \
    RegisterPhase(id, *(new (buf) mePhase(id)));                              \
  } while (0);
#define FUNCAPHASE(id, mePhase)                                                    \
  do {                                                                             \
    void *buf = GetMemAllocator()->GetMemPool()->Malloc(sizeof(mePhase(id)));      \
    CHECK_FATAL(buf != nullptr, "null ptr check");                                 \
    RegisterPhase(id, *(new (buf) mePhase(id)));                                   \
    arFuncManager.AddAnalysisPhase(id, (static_cast<MeFuncPhase*>(GetPhase(id)))); \
  } while (0);
#include "me_phases.def"
#undef FUNCTPHASE
#undef FUNCAPHASE
}

void MeFuncPhaseManager::AddPhasesNoDefault(const std::vector<std::string> &phases) {
  for (size_t i = 0; i < phases.size(); ++i) {
    PhaseManager::AddPhase(phases[i]);
  }
  ASSERT(phases.size() == GetPhaseSequence()->size(), "invalid phase name");
}

void MeFuncPhaseManager::AddPhases(const std::unordered_set<std::string> &skipPhases) {
  auto addPhase = [&](const std::string &phase) {
    auto it = skipPhases.find(phase);
    if (it == skipPhases.end()) {
      PhaseManager::AddPhase(phase);
    }
  };
  if (mePhaseType == kMePhaseMainopt) {
    // default phase sequence
    addPhase("ssatab");
    addPhase("aliasclass");
    addPhase("ssa");
    addPhase("rclowering");
    addPhase("bblayout");
    addPhase("emit");
  }
}

// match sub string of function name
bool MeFuncPhaseManager::FuncFilter(const std::string &filter, const std::string &name) {
  return (filter == "*") || (name.find(filter) != std::string::npos);
}

void MeFuncPhaseManager::IPACleanUp(MeFunction *func) {
  ASSERT(func != nullptr, "null ptr check");
  GetAnalysisResultManager()->InvalidAllResults();
  memPoolCtrler.DeleteMemPool(func->GetMemPool());
}

void MeFuncPhaseManager::Run(MIRFunction *mirFunc, uint64 rangeNum, const std::string &meInput,
                             MemPoolCtrler &localMpCtrler) {
  if (!MeOption::quiet) {
    LogInfo::MapleLogger() << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>> Optimizing Function  < " << mirFunc->GetName()
                           << " id=" << mirFunc->GetPuidxOrigin() << " >---\n";
  }
  MPLTimer runPhasetimer;
  MPLTimer funcPrepareTimer;
  MPLTimer iteratorTimer;
  MPLTimer invalidTimer;
  if (timePhases) {
    funcPrepareTimer.Start();
  }
  MemPool *funcMP = localMpCtrler.NewMemPool("maple_me per-function mempool");
  MemPool *versMP = localMpCtrler.NewMemPool("first verst mempool");
  MeFunction &func = *(funcMP->New<MeFunction>(&mirModule, mirFunc, funcMP, versMP, meInput));
  func.PartialInit(false);
#if DEBUG
  globalMIRModule = &mirModule;
  globalFunc = &func;
#endif
  func.Prepare(rangeNum);
  if (timePhases) {
    funcPrepareTimer.Stop();
    extraMeTimers["prepareFunc"] += funcPrepareTimer.ElapsedMicroseconds();
  }
  if (ipa) {
    mirFunc->SetMeFunc(&func);
  }
  std::string phaseName = "";
  MeFuncPhase *changeCFGPhase = nullptr;
  // each function level phase
  bool dumpFunc = FuncFilter(MeOption::dumpFunc, func.GetName());
  size_t phaseIndex = 0;
  if (timePhases) {
    iteratorTimer.Start();
  }
  long runPhasesTime = 0;
  for (auto it = PhaseSequenceBegin(); it != PhaseSequenceEnd(); ++it, ++phaseIndex) {
    if (timePhases) {
      runPhasetimer.Start();
    }
    PhaseID id = GetPhaseId(it);
    auto *p = static_cast<MeFuncPhase*>(GetPhase(id));
    p->SetPreviousPhaseName(phaseName); // prev phase name is for filename used in emission after phase
    phaseName = p->PhaseName();         // new phase name
    bool dumpPhase = MeOption::DumpPhase(phaseName);
    RunFuncPhase(&func, p);
    if ((MeOption::dumpAfter || dumpPhase) && dumpFunc) {
      LogInfo::MapleLogger() << ">>>>> Dump after " << phaseName << " <<<<<\n";
      if (phaseName != "emit") {
        func.Dump(false);
      }
      LogInfo::MapleLogger() << ">>>>> Dump after End <<<<<\n\n";
    }
    if (p->IsChangedCFG()) {
      changeCFGPhase = p;
      p->ClearChangeCFG();
      break;
    }
    if (timePhases) {
      runPhasetimer.Stop();
      phaseTimers[phaseIndex] += runPhasetimer.ElapsedMicroseconds();
      runPhasesTime += runPhasetimer.ElapsedMicroseconds();
    }
  }
  if (timePhases) {
    iteratorTimer.Stop();
    extraMeTimers["iterator"] += iteratorTimer.ElapsedMicroseconds() - runPhasesTime;
    runPhasesTime = 0;
  }

  if (!ipa) {
    invalidTimer.Start();
    GetAnalysisResultManager()->InvalidAllResults();
    invalidTimer.Stop();
    extraMeTimers["invalidResult"] += invalidTimer.ElapsedMicroseconds();
  }
  if (changeCFGPhase != nullptr) {
    if (ipa) {
      CHECK_FATAL(false, "phases in ipa will not chang cfg.");
    }
    if (timePhases) {
      funcPrepareTimer.Start();
    }
    // do all the phases start over
    MemPool *versMemPool = localMpCtrler.NewMemPool("second verst mempool");
    MeFunction function(&mirModule, mirFunc, funcMP, versMemPool, meInput);
    function.PartialInit(true);
    function.Prepare(rangeNum);
    if (timePhases) {
      funcPrepareTimer.Stop();
      extraMeTimers["prepareFunc"] += funcPrepareTimer.ElapsedMicroseconds();
      iteratorTimer.Start();
    }
    for (auto it = PhaseSequenceBegin(); it != PhaseSequenceEnd(); ++it) {
      if (timePhases) {
        runPhasetimer.Start();
      }
      PhaseID id = GetPhaseId(it);
      auto *p = static_cast<MeFuncPhase*>(GetPhase(id));
      if (p == changeCFGPhase) {
        continue;
      }
      p->SetPreviousPhaseName(phaseName); // prev phase name is for filename used in emission after phase
      phaseName = p->PhaseName();         // new phase name
      bool dumpPhase = MeOption::DumpPhase(phaseName);
      RunFuncPhase(&function, p);
      if ((MeOption::dumpAfter || dumpPhase) && dumpFunc) {
        LogInfo::MapleLogger() << ">>>>>Second time Dump after " << phaseName << " <<<<<\n";
        if (phaseName != "emit") {
          function.Dump(false);
        }
        LogInfo::MapleLogger() << ">>>>> Second time Dump after End <<<<<\n\n";
      }
      if (timePhases) {
        runPhasetimer.Stop();
        phaseTimers[phaseIndex] += runPhasetimer.ElapsedMicroseconds();
        runPhasesTime += runPhasetimer.ElapsedMicroseconds();
      }
    }
    if (timePhases) {
      iteratorTimer.Stop();
      extraMeTimers["iterator"] += iteratorTimer.ElapsedMicroseconds() - runPhasesTime;
      runPhasesTime = 0;
      invalidTimer.Start();
    }

    GetAnalysisResultManager()->InvalidAllResults();

    if (timePhases) {
      invalidTimer.Stop();
      extraMeTimers["invalidResult"] += invalidTimer.ElapsedMicroseconds();
    }
  }
  if (!ipa) {
    if (timePhases) {
      invalidTimer.Start();
    }

    localMpCtrler.DeleteMemPool(funcMP);

    if (timePhases) {
      invalidTimer.Stop();
      extraMeTimers["invalidResult"] += invalidTimer.ElapsedMicroseconds();
    }
  }
}

MeFuncPhaseManager &MeFuncPhaseManager::Clone(MemPool &mp, MemPoolCtrler &ctrler) const {
  auto *fpm = mp.New<MeFuncPhaseManager>(&mp, mirModule, modResMgr);
  fpm->RegisterFuncPhases();
  fpm->SetGenMeMpl(genMeMpl);
  fpm->SetTimePhases(timePhases);
  fpm->SetIPA(ipa);
  fpm->SetMePhase(mePhaseType);
  for (PhaseID id : phaseSequences) {
    fpm->AddPhase(registeredPhases.find(id)->second->PhaseName());
  }
  // set memPoolCtrler for phases
  for (auto pair : fpm->GetRegisteredPhases()) {
    Phase *phase = pair.second;
    phase->SetMpCtrler(&ctrler);
  }
  return *fpm;
}
}  // namespace maple
