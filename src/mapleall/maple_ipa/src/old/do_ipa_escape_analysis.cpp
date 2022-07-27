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
#include "do_ipa_escape_analysis.h"

namespace maple {
#ifdef NOT_USED
AnalysisResult *DoIpaEA::Run(MeFunction *func, MeFuncResultMgr *m, ModuleResultMgr *mrm) {
  if (func == nullptr) {
    return nullptr;
  }
  MIRFunction *mirFunc = func->GetMirFunc();
  const std::map<GStrIdx, EAConnectionGraph*> &summaryMap = mirFunc->GetModule()->GetEASummary();
  if (!mirFunc->GetModule()->IsInIPA() && summaryMap.size() == 0) {
    return nullptr;
  }
  CHECK_FATAL(mrm != nullptr, "Needs module result manager for ipa");
  KlassHierarchy *kh = static_cast<KlassHierarchy*>(mrm->GetAnalysisResult(MoPhase_CHA, &func->GetMIRModule()));
  CHECK_FATAL(kh != nullptr, "KlassHierarchy phase has problem");
  MeIRMap *irMap = static_cast<MeIRMap*>(m->GetAnalysisResult(MeFuncPhase_IRMAPBUILD, func));
  CHECK_FATAL(irMap != nullptr, "irMap phase has problem");

  CallGraph *pcg = nullptr;
  if (mirFunc->GetModule()->IsInIPA()) {
    pcg = static_cast<CallGraph*>(mrm->GetAnalysisResult(MoPhase_CALLGRAPH_ANALYSIS, &func->GetMIRModule()));
  }
  MemPool *eaMemPool = memPoolCtrler.NewMemPool(PhaseName(), false /* isLcalPool */);
  mirFunc->GetModule()->SetCurFunction(mirFunc);

  if (IPAEscapeAnalysis::kDebug) {
    LogInfo::MapleLogger() << "=======IPAEA BEGIN======== " << mirFunc->GetName() << std::endl;
  }

  IPAEscapeAnalysis ipaEA(kh, irMap, func, eaMemPool, pcg);
  ipaEA.ConstructConnGraph();
  func->GetMirFunc()->GetEACG()->TrimGlobalNode();
  if (!mirFunc->GetModule()->IsInIPA()) {
    auto it = summaryMap.find(func->GetMirFunc()->GetNameStrIdx());
    if (it != summaryMap.end() && it->second != nullptr) {
      it->second->DeleteEACG();
    }
  }
  if (!mirFunc->GetModule()->IsInIPA() && IPAEscapeAnalysis::kDebug) {
    func->GetMirFunc()->GetEACG()->CountObjEAStatus();
  }
  if (IPAEscapeAnalysis::kDebug) {
    LogInfo::MapleLogger() << "=======IPAEA END========" << mirFunc->GetName() << std::endl;
  }

  delete eaMemPool;
  return nullptr;
}

AnalysisResult *DoIpaEAOpt::Run(MeFunction *func, MeFuncResultMgr *mgr, ModuleResultMgr *mrm) {
  if (func == nullptr) {
    return nullptr;
  }
  MIRFunction *mirFunc = func->GetMirFunc();
  const std::map<GStrIdx, EAConnectionGraph*> &summaryMap = mirFunc->GetModule()->GetEASummary();
  if (!mirFunc->GetModule()->IsInIPA() && summaryMap.size() == 0) {
    return nullptr;
  }
  CHECK_FATAL(mrm != nullptr, "Needs module result manager for ipa");
  KlassHierarchy *kh = static_cast<KlassHierarchy*>(mrm->GetAnalysisResult(MoPhase_CHA, &func->GetMIRModule()));
  CHECK_FATAL(kh != nullptr, "KlassHierarchy phase has problem");
  MeIRMap *irMap = static_cast<MeIRMap*>(mgr->GetAnalysisResult(MeFuncPhase_IRMAPBUILD, func));
  CHECK_FATAL(irMap != nullptr, "irMap phase has problem");

  mgr->InvalidAnalysisResult(MeFuncPhase_MELOOP, func);
  IdentifyLoops *meLoop = static_cast<IdentifyLoops*>(mgr->GetAnalysisResult(MeFuncPhase_MELOOP, func));
  CHECK_FATAL(meLoop != nullptr, "meLoop phase has problem");
  meLoop->MarkBB();

  CallGraph *pcg = nullptr;
  if (mirFunc->GetModule()->IsInIPA()) {
    pcg = static_cast<CallGraph*>(mrm->GetAnalysisResult(MoPhase_CALLGRAPH_ANALYSIS, &func->GetMIRModule()));
  }
  MemPool *eaMemPool = memPoolCtrler.NewMemPool(PhaseName(), false /* isLcalPool */);
  mirFunc->GetModule()->SetCurFunction(mirFunc);

  if (IPAEscapeAnalysis::kDebug) {
    LogInfo::MapleLogger() << "=======IPAEAOPT BEGIN======== " << mirFunc->GetName() << std::endl;
  }

  IPAEscapeAnalysis ipaEA(kh, irMap, func, eaMemPool, pcg);
  ipaEA.DoOptimization();
  if (IPAEscapeAnalysis::kDebug) {
    LogInfo::MapleLogger() << "=======IPAEAOPT END========" << mirFunc->GetName() << std::endl;
  }

  delete eaMemPool;
  return nullptr;
}
#endif
}
