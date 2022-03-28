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
#ifndef MAPLE_ME_INCLUDE_ME_PHASE_MANAGER_H
#define MAPLE_ME_INCLUDE_ME_PHASE_MANAGER_H
#include <vector>
#include <string>
#include "mempool.h"
#include "mempool_allocator.h"
#include "mir_module.h"
#include "mir_function.h"
#include "maple_phase_manager.h"
#include "me_dominance.h"
#include "me_cfg.h"
#include "me_alias_class.h"
#include "me_bypath_eh.h"
#include "me_critical_edge.h"
#include "me_profile_gen.h"
#include "me_profile_use.h"
#include "me_loop_canon.h"
#include "me_loop_inversion.h"
#include "me_analyzector.h"
#include "me_value_range_prop.h"
#include "me_abco.h"
#include "me_dse.h"
#include "me_hdse.h"
#include "me_prop.h"
#include "copy_prop.h"
#include "me_ssa_prop.h"
#include "me_rename2preg.h"
#include "me_loop_unrolling.h"
#include "me_cfg_opt.h"
#include "meconstprop.h"
#include "me_bb_analyze.h"
#include "me_ssa_lpre.h"
#include "me_ssa_epre.h"
#include "me_stmt_pre.h"
#include "me_store_pre.h"
#include "me_cond_based_rc.h"
#include "me_cond_based_npc.h"
#include "me_check_cast.h"
#include "me_placement_rc.h"
#include "me_subsum_rc.h"
#include "me_predict.h"
#include "me_side_effect.h"
#include "do_ipa_escape_analysis.h"
#include "me_gc_lowering.h"
#include "me_gc_write_barrier_opt.h"
#include "preg_renamer.h"
#include "me_ssa_devirtual.h"
#include "me_delegate_rc.h"
#include "me_analyze_rc.h"
#include "me_may2dassign.h"
#include "me_loop_analysis.h"
#include "me_ssa.h"
#include "me_toplevel_ssa.h"
#include "me_irmap_build.h"
#include "me_bb_layout.h"
#include "me_emit.h"
#include "me_rc_lowering.h"
#include "gen_check_cast.h"
#include "me_fsaa.h"
#include "simplifyCFG.h"
#include "me_ivopts.h"
#if MIR_JAVA
#include "sync_select.h"
#endif  // MIR_JAVA
#include "me_ssa_tab.h"
#include "mpl_timer.h"
#include "constantfold.h"
#include "me_verify.h"
#include "lfo_inject_iv.h"
#include "pme_emit.h"
#include "lfo_iv_canon.h"
#include "cfg_opt.h"
#include "lfo_dep_test.h"
#include "me_autovec.h"
#include "lfo_unroll.h"
#include "me_safety_warning.h"
#include "me_sink.h"

namespace maple {
using meFuncOptTy = MapleFunctionPhase<MeFunction>;

/* ==== new phase manager ==== */
class MeFuncPM : public FunctionPM {
 public:
  explicit MeFuncPM(MemPool *memPool) : FunctionPM(memPool, &id) {}
  PHASECONSTRUCTOR(MeFuncPM);
  std::string PhaseName() const override;
  ~MeFuncPM() override {}
  static bool genMeMpl;
  static bool timePhases;

  void SetMeInput(const std::string &str) {
    meInput = str;
  }

  bool PhaseRun(MIRModule &m) override;
 private:
  bool SkipFuncForMe(const MIRModule &m, const MIRFunction &func, uint64 range);
  bool FuncLevelRun(MeFunction &f, AnalysisDataManager &serialADM);
  void GetAnalysisDependence(AnalysisDep &aDep) const override;
  void DumpMEIR(const MeFunction &f, const std::string phaseName, bool isBefore);
  virtual void DoPhasesPopulate(const MIRModule &m);

  std::string meInput = "";
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_PHASE_MANAGER_H
