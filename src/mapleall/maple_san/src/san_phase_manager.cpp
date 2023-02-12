#include "san_phase_manager.h"
#include "asan_phases.h"
#include "ubsan_phases.h"


namespace maple {

void MEModuleDoAsan::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
    // aDep.AddRequired<M2MKlassHierarchy>();
    aDep.SetPreservedAll();
}

void MEModuleDoAsan::DoPhasesPopulate(const maple::MIRModule &mirModule) {
  #define SAN_PHASE
  #include "phases.def"
  #undef SAN_PHASE
}

bool MEModuleDoAsan::FuncLevelRun(MeFunction &meFunc, AnalysisDataManager &serialADM) {
  bool changed = false;
  for (size_t i = 0; i < phasesSequence.size(); ++i) {
    SolveSkipFrom(MeOption::GetSkipFromPhase(), i);
    const MaplePhaseInfo *curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(phasesSequence[i]);
    if (!IsQuiet()) {
      LogInfo::MapleLogger() << "---Run maple_san " << (curPhase->IsAnalysis() ? "analysis" : "transform")
                             << " Phase [ " << curPhase->PhaseName() << " ]---\n";
    }
    if (curPhase->IsAnalysis()) {
      changed |= RunAnalysisPhase<MeFuncOptTy, MeFunction>(*curPhase, serialADM, meFunc);
    } else {
      changed |= RunTransformPhase<MeFuncOptTy, MeFunction>(*curPhase, serialADM, meFunc);
    }
    SolveSkipAfter(MeOption::GetSkipAfterPhase(), i);
  }
  return changed;
}

bool MEModuleDoAsan::PhaseRun(maple::MIRModule &m) {
  bool changed = false;
  // ModuleAddressSanitizer AsanModule(m);
  // AsanModule.instrumentModule();
  auto &compFuncList = m.GetFunctionList();
  auto admMempool = AllocateMemPoolInPhaseManager("ASAN phase manager's analysis data manager mempool");
  auto *serialADM = GetManagerMemPool()->New<AnalysisDataManager>(*(admMempool.get()));
  ClearAllPhases();
  DoPhasesPopulate(m);
  SetQuiet(MeOption::quiet);
  size_t i = 0;
  for (auto &func : std::as_const(compFuncList)) {
    ASSERT_NOT_NULL(func);
    ++i;
    if (func->IsEmpty()) {
        continue;
    }
    m.SetCurFunction(func);
    if (!IsQuiet()) {
      LogInfo::MapleLogger() << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>> Sanitize Function  < " << func->GetName()
                             << " id=" << func->GetPuidxOrigin() << " >---\n";
      /* prepare me func */
      auto meFuncMP = std::make_unique<ThreadLocalMemPool>(memPoolCtrler, "maple_san per-function mempool");
      auto meFuncStackMP = std::make_unique<StackMemPool>(memPoolCtrler, "");
      MemPool *versMP = new ThreadLocalMemPool(memPoolCtrler, "first verst mempool");
      MeFunction &meFunc = *(meFuncMP->New<MeFunction>(&m, func, meFuncMP.get(), *meFuncStackMP, versMP, meInput));
      func->SetMeFunc(&meFunc);
      meFunc.PartialInit();
      if (!IsQuiet()) {
        LogInfo::MapleLogger() << "---Preparing Function  < " << func->GetName() << " > [" << i - 1 << "] ---\n";
      }
      meFunc.Prepare();
      changed = FuncLevelRun(meFunc, *serialADM);
      meFunc.Release();
      serialADM->EraseAllAnalysisPhase();
    }
  }
  m.Emit("comb.san.mpl");
  return changed;
}

MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEModuleDoAsan, doModuleAsan)

MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEDoAsan, doAsan);
MAPLE_ANALYSIS_PHASE_REGISTER(MEDoVarCheck, doAsanVarCheck);
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEDoUbsanBound, doUbsanBound);

} // namespace maple