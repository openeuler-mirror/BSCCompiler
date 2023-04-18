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
#include "me_ssa_epre.h"
#include "me_dominance.h"
#include "me_ssa_update.h"
#include "me_phase_manager.h"
#include "me_placement_rc.h"
#include "me_loop_analysis.h"
#include "me_hdse.h"
#include "me_stack_protect.h"

namespace {
const std::set<std::string> propWhiteList {
#define PROPILOAD(funcName) #funcName,
#include "propiloadlist.def"
#undef PROPILOAD
};
}

// accumulate the BBs that are in the iterated dominance frontiers of bb in
// the set dfSet, visiting each BB only once
namespace maple {
void MeSSAEPre::BuildWorkList() {
  auto cfg = func->GetCfg();
  const auto &preOrderDt = dom->GetDtPreOrder();
  for (auto &bbID : preOrderDt) {
    BB *bb = cfg->GetAllBBs().at(bbID);
    BuildWorkListBB(bb);
  }
}

bool MeSSAEPre::IsThreadObjField(const IvarMeExpr &expr) const {
  if (klassHierarchy == nullptr) {
    return false;
  }
  if (expr.GetFieldID() == 0) {
    return false;
  }
  auto *type = static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(expr.GetTyIdx()));
  TyIdx runnableInterface = klassHierarchy->GetKlassFromLiteral("Ljava_2Flang_2FRunnable_3B")->GetTypeIdx();
  Klass *klass = klassHierarchy->GetKlassFromTyIdx(type->GetPointedTyIdx());
  if (klass == nullptr) {
    return false;
  }
  for (Klass *inter : klass->GetImplInterfaces()) {
    if (inter->GetTypeIdx() == runnableInterface) {
      return true;
    }
  }
  return false;
}

bool MESSAEPre::PhaseRun(maple::MeFunction &f) {
  static uint32 puCount = 0;  // count PU to support the eprePULimit option
  if (puCount > MeOption::eprePULimit) {
    ++puCount;
    return false;
  }
  // make irmapbuild first because previous phase may invalid all analysis results
  auto *irMap = GET_ANALYSIS(MEIRMapBuild, f);
  ASSERT(irMap != nullptr, "irMap phase has problem");
  auto dominancePhase = EXEC_ANALYSIS(MEDominance, f);
  auto dom = dominancePhase->GetDomResult();
  ASSERT(dom != nullptr, "dominance construction has problem");
  auto pdom = dominancePhase->GetPdomResult();
  ASSERT(pdom != nullptr, "postdominance construction has problem");

  // checking the need for stack protection
  if (Options::stackProtectorStrong) {
    MeStackProtect checker(f);
    checker.CheckAddrofStack();
  }

  KlassHierarchy *kh = nullptr;
  if (f.GetMIRModule().IsJavaModule()) {
    MaplePhase *it = GetAnalysisInfoHook()->GetTopLevelAnalyisData<M2MKlassHierarchy, MIRModule>(f.GetMIRModule());
    kh = static_cast<M2MKlassHierarchy*>(it)->GetResult();
    CHECK_FATAL(kh != nullptr, "KlassHierarchy phase has problem");
  }

  IdentifyLoops *identLoops = GET_ANALYSIS(MELoopAnalysis, f);
  CHECK_NULL_FATAL(identLoops);

  bool eprePULimitSpecified = MeOption::eprePULimit != UINT32_MAX;
  uint32 epreLimitUsed =
      (eprePULimitSpecified && puCount != MeOption::eprePULimit) ? UINT32_MAX : MeOption::epreLimit;
  MemPool *ssaPreMemPool = ApplyTempMemPool();
  bool epreIncludeRef = MeOption::epreIncludeRef;
  if (!MeOption::gcOnly && propWhiteList.find(f.GetName()) != propWhiteList.end()) {
    epreIncludeRef = false;
  }
  MeSSAEPre ssaPre(f, *irMap, *dom, *pdom, kh, *ssaPreMemPool, *ApplyTempMemPool(), epreLimitUsed, epreIncludeRef,
                   MeOption::epreLocalRefVar, MeOption::epreLHSIvar);
  if (f.GetMirFunc()->GetFuncProfData() && MeOption::epreUseProfile) {
    if (MeOption::usePgoRange) {
      if (f.GetMirFunc()->GetPuidxOrigin() >= MeOption::pgoRange[0] &&
          f.GetMirFunc()->GetPuidxOrigin() <= MeOption::pgoRange[1]) {
        ssaPre.doMinCut = true;
      }
    } else {
      ssaPre.doMinCut = true;
    }
    if (ssaPre.doMinCut) {
      ssaPre.SetPreUseProfileLimit(MeOption::epreUseProfileLimit);
    }
  }
  ssaPre.SetSpillAtCatch(MeOption::spillAtCatch);
  if (MeOption::strengthReduction && !f.GetMIRModule().IsJavaModule()) {
    ssaPre.strengthReduction = true;
    if (MeOption::doLFTR) {
      ssaPre.doLFTR = true;
    }
  }
  if ((f.GetHints() & kPlacementRCed) != 0) {
    ssaPre.SetPlacementRC(true);
  }
  if (eprePULimitSpecified && puCount == MeOption::eprePULimit && epreLimitUsed != UINT32_MAX) {
    LogInfo::MapleLogger() << "applying EPRE limit " << epreLimitUsed << " in function " <<
        f.GetMirFunc()->GetName() << "\n";
  }
  if (DEBUGFUNC_NEWPM(f)) {
    ssaPre.SetSSAPreDebug(true);
  }
  if (!ssaPre.doMinCut) {
    ssaPre.ApplySSAPRE();
  } else {
    ssaPre.ApplyMCSSAPRE();
  }
  if (!ssaPre.GetCandsForSSAUpdate().empty()) {
    MeSSAUpdate ssaUpdate(f, *f.GetMeSSATab(), *dom, ssaPre.GetCandsForSSAUpdate());
    ssaUpdate.Run();
  }
  if ((f.GetHints() & kPlacementRCed) && ssaPre.GetAddedNewLocalRefVars()) {
    PlacementRC placeRC(f, *dom, *pdom, *ssaPreMemPool, DEBUGFUNC_NEWPM(f));
    placeRC.preKind = MeSSUPre::kSecondDecrefPre;
    placeRC.ApplySSUPre();
  }
  if (ssaPre.strengthReduction && !MeOption::ivopts) { // for deleting redundant injury repairs
    auto *aliasClass = FORCE_GET(MEAliasClass);
    MeHDSE hdse(f, *dom, *pdom, *f.GetIRMap(), aliasClass, DEBUGFUNC_NEWPM(f));
    if (!MeOption::quiet) {
      LogInfo::MapleLogger() << "  == " << PhaseName() << " invokes [ " << hdse.PhaseName() << " ] ==\n";
    }
    hdse.hdseKeepRef = MeOption::dseKeepRef;
    hdse.DoHDSESafely(&f, *GetAnalysisInfoHook());
  }
  ++puCount;
  return true;
}

void MESSAEPre::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEIRMapBuild>();
  aDep.AddRequired<MELoopCanon>();
  aDep.AddRequired<MEDominance>();
  aDep.AddRequired<MELoopAnalysis>();
  aDep.SetPreservedAll();
}
}  // namespace maple
