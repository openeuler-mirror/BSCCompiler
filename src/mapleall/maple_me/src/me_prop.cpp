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
#include "me_prop.h"
#include "me_option.h"
#include "me_dominance.h"
#include "me_ssa_update.h"

// This phase perform copy propagation optimization based on SSA representation.
// The propagation will not apply to ivar's of ref type unless the option
// --propiloadref is enabled.
//
// Copy propagation works by conducting a traversal over the program.  When it
// encounters a variable reference, it uses its SSA representation to look up
// its assigned value and try to do the substitution.
namespace {
const std::set<std::string> propWhiteList {
#define PROPILOAD(funcname) #funcname,
#include "propiloadlist.def"
#undef PROPILOAD
};
}

namespace maple {
void MEMeProp::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>();
  aDep.AddRequired<MEIRMapBuild>();
  aDep.SetPreservedAll();
}

bool MEMeProp::PhaseRun(maple::MeFunction &f) {
  if (f.hpropRuns >= MeOption::hpropRunsLimit) {
    if (!MeOption::quiet) {
      LogInfo::MapleLogger() << "  == " << PhaseName() << " skipped\n";
    }
    return false;
  }
  f.hpropRuns++;
  auto *dom = GET_ANALYSIS(MEDominance, f);
  CHECK_NULL_FATAL(dom);
  auto *hMap = GET_ANALYSIS(MEIRMapBuild, f);
  CHECK_NULL_FATAL(hMap);
  bool propIloadRef = MeOption::propIloadRef;
  if (!propIloadRef) {
    MIRSymbol *fnSt = GlobalTables::GetGsymTable().GetSymbolFromStidx(f.GetMirFunc()->GetStIdx().Idx());
    CHECK_FATAL(fnSt, "fnSt is nullptr");
    const std::string &funcName = fnSt->GetName();
    propIloadRef = propWhiteList.find(funcName) != propWhiteList.end();
    if (DEBUGFUNC_NEWPM(f)) {
      if (propIloadRef) {
        LogInfo::MapleLogger() << "propiloadref enabled because function is in white list";
      }
    }
  }
  auto propConfig = Prop::PropConfig {
      MeOption::propBase, propIloadRef, MeOption::propGlobalRef, MeOption::propFinaliLoadRef,
      MeOption::propIloadRefNonParm, MeOption::propAtPhi, MeOption::propWithInverse || f.IsLfo()
  };
  MeProp meProp(*hMap, *dom, *ApplyTempMemPool(), propConfig, MeOption::propLimit);
  meProp.isLfo = f.IsLfo();
  meProp.TraversalBB(*f.GetCfg()->GetCommonEntryBB());
  if (DEBUGFUNC_NEWPM(f)) {
    LogInfo::MapleLogger() << "\nNumber of propagations performed by hprop: " << meProp.propsPerformed << std::endl;
    LogInfo::MapleLogger() << "============== After Copy Propagation  =============" << '\n';
    f.Dump(false);
  }

  if (!meProp.CandsForSSAUpdate().empty()) {
    MeSSAUpdate ssaUpdate(f, *f.GetMeSSATab(), *dom, meProp.CandsForSSAUpdate());
    ssaUpdate.Run();
  }
  return true;
}
}  // namespace maple
