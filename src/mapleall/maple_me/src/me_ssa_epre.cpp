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
#include "me_placement_rc.h"

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
  const MapleVector<BBId> &preOrderDt = dom->GetDtPreOrder();
  for (auto &bbID : preOrderDt) {
    BB *bb = func->GetAllBBs().at(bbID);
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

AnalysisResult *MeDoSSAEPre::Run(MeFunction *func, MeFuncResultMgr *m, ModuleResultMgr *mrm) {
  static uint32 puCount = 0;  // count PU to support the eprePULimit option
  if (puCount > MeOption::eprePULimit) {
    ++puCount;
    return nullptr;
  }
  auto *dom = static_cast<Dominance*>(m->GetAnalysisResult(MeFuncPhase_DOMINANCE, func));
  ASSERT(dom != nullptr, "dominance phase has problem");
  auto *irMap = static_cast<MeIRMap*>(m->GetAnalysisResult(MeFuncPhase_IRMAPBUILD, func));
  ASSERT(irMap != nullptr, "irMap phase has problem");
  KlassHierarchy *kh = nullptr;
  if (func->GetMIRModule().IsJavaModule()) {
    kh = static_cast<KlassHierarchy*>(mrm->GetAnalysisResult(MoPhase_CHA, &func->GetMIRModule()));
    CHECK_FATAL(kh != nullptr, "KlassHierarchy phase has problem");
  }
  bool eprePULimitSpecified = MeOption::eprePULimit != UINT32_MAX;
  uint32 epreLimitUsed =
      (eprePULimitSpecified && puCount != MeOption::eprePULimit) ? UINT32_MAX : MeOption::epreLimit;
  MemPool *ssaPreMemPool = NewMemPool();
  bool epreIncludeRef = MeOption::epreIncludeRef;
  if (!MeOption::gcOnly && propWhiteList.find(func->GetName()) != propWhiteList.end()) {
    epreIncludeRef = false;
  }
  MeSSAEPre ssaPre(*func, *irMap, *dom, kh, *ssaPreMemPool, *NewMemPool(), epreLimitUsed, epreIncludeRef,
                   MeOption::epreLocalRefVar, MeOption::epreLHSIvar);
  ssaPre.SetSpillAtCatch(MeOption::spillAtCatch);
  if (func->GetHints() & kPlacementRCed) {
    ssaPre.SetPlacementRC(true);
  }
  if (eprePULimitSpecified && puCount == MeOption::eprePULimit && epreLimitUsed != UINT32_MAX) {
    LogInfo::MapleLogger() << "applying EPRE limit " << epreLimitUsed << " in function " <<
        func->GetMirFunc()->GetName() << "\n";
  }
  if (DEBUGFUNC(func)) {
    ssaPre.SetSSAPreDebug(true);
  }
  ssaPre.ApplySSAPRE();
  if (!ssaPre.GetCandsForSSAUpdate().empty()) {
    MemPool *tmp = NewMemPool();
    MeSSAUpdate ssaUpdate(*func, *func->GetMeSSATab(), *dom, ssaPre.GetCandsForSSAUpdate(), *tmp);
    ssaUpdate.Run();
  }
  if ((func->GetHints() & kPlacementRCed) && ssaPre.GetAddedNewLocalRefVars()) {
    PlacementRC placeRC(*func, *dom, *ssaPreMemPool, DEBUGFUNC(func));
    placeRC.preKind = MeSSUPre::kSecondDecrefPre;
    placeRC.ApplySSUPre();
  }
  if (DEBUGFUNC(func)) {
    LogInfo::MapleLogger() << "\n============== EPRE =============" << "\n";
    func->Dump(false);
  }
  ++puCount;
  return nullptr;
}
}  // namespace maple
