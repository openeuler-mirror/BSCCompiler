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
#include "proepilog.h"
#if defined(TARGAARCH64) && TARGAARCH64
#include "aarch64_proepilog.h"
#elif defined(TARGRISCV64) && TARGRISCV64
#include "riscv64_proepilog.h"
#endif
#if defined(TARGARM32) && TARGARM32
#include "arm32_proepilog.h"
#endif
#if defined(TARGX86_64) && TARGX86_64
#include "x64_proepilog.h"
#endif
#include "cgfunc.h"
#include "cg.h"
#include "cg_ssa_pre.h"
#include "cg_mc_ssa_pre.h"
#include "cg_ssu_pre.h"

namespace maplebe {
static inline bool CheckInsnNeedProEpilog(const CGFunc &cgFunc, const Insn &insn) {
  auto *regInfo = cgFunc.GetTargetRegInfo();
  auto checkRegisterNeedProEpilog = [regInfo](const RegOperand &regOpnd) {
    if (regOpnd.GetRegisterType() == kRegTyVary) {
      return true;
    }
    auto regno = regOpnd.GetRegisterNumber();
    if (regInfo->IsCalleeSavedReg(regno) || regInfo->IsFramePointReg(regno) ||
        regno == regInfo->GetStackPointReg()) {
      return true;
    }
    return false;
  };

  for (uint32 i = 0; i < insn.GetOperandSize(); ++i) {
    auto &opnd = insn.GetOperand(i);
    if (opnd.IsRegister()) {
      if (checkRegisterNeedProEpilog(static_cast<RegOperand&>(opnd))) {
        return true;
      }
    } else if (opnd.IsMemoryAccessOperand()) {
      auto &memOpnd = static_cast<MemOperand&>(opnd);
      if (memOpnd.GetBaseRegister() && checkRegisterNeedProEpilog(*memOpnd.GetBaseRegister())) {
        return true;
      }
      if (memOpnd.GetIndexRegister() && checkRegisterNeedProEpilog(*memOpnd.GetIndexRegister())) {
        return true;
      }
    } else if (opnd.IsList()) {
      auto &listOpnd = static_cast<ListOperand&>(opnd);
      for (auto *regOpnd : std::as_const(listOpnd.GetOperands())) {
        if (checkRegisterNeedProEpilog(*regOpnd)) {
          return true;
        }
      }
    }
  }
  return false;
}

static inline void CollectNeedProEpilogBB(CGFunc &cgFunc, MapleSet<uint32> &occBBs) {
  FOR_ALL_BB(bb, &cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (insn->IsImmaterialInsn() || !insn->IsMachineInstruction()) {
        continue;
      }
      if (insn->IsCall() || insn->IsSpecialCall() || CheckInsnNeedProEpilog(cgFunc, *insn)) {
        occBBs.insert(bb->GetId());
        break;
      }
    }
  }
}

// If all successors of curBB need to insert prolog, we can insert prolog at curBB.
// such as:
//    BB1:                       BB1:
//       ...                         insert prolog
//       bne BB3;                    ...
//    BB2:                           bne BB3;
//       insert prolog   ==>     BB2:
//       ...                         ...
//    BB3:                       BB3:
//       insert prolog               ...
//       ...
bool ProEpilogAnalysis::PrologBBHoist(const MapleSet<BBID> &saveBBs) {
  for (const auto bbId : saveBBs) {
    // saveBB post dom firstBB, it's no benefit
    if (pdomInfo.PostDominate(*cgFunc.GetBBFromID(bbId), *cgFunc.GetFirstBB())) {
      saveInfo.Clear();
      return false;
    }
  }

  for (const auto bbId : saveBBs) {
    auto &bbPreds = cgFunc.GetBBFromID(bbId)->GetPreds();
    if (bbPreds.size() != 1) {
      saveInfo.prologBBs.insert(bbId);
      continue;
    }
    // only one pred, check succs of pred are all in saveBBs
    auto *bbPred = bbPreds.front();
    if (saveInfo.prologBBs.count(bbPred->GetId()) != 0) {
      continue;
    }
    bool bbPredSuccHasProlog = true;
    for (const auto *succ : bbPred->GetSuccs()) {
      if (saveBBs.count(succ->GetId()) == 0) {
        bbPredSuccHasProlog = false;
      }
    }
    if (bbPredSuccHasProlog) {
      saveInfo.prologBBs.insert(bbPred->GetId());
    } else {
      saveInfo.prologBBs.insert(bbId);
    }
  }
  return true;
}

void ProEpilogAnalysis::Analysis() {
  SsaPreWorkCand wkCand(&alloc);
  CollectNeedProEpilogBB(cgFunc, wkCand.occBBs);
  if (cgFunc.GetFunction().GetFuncProfData() == nullptr) {
    DoSavePlacementOpt(&cgFunc, &domInfo, &loopInfo, &wkCand);
  } else {
    DoProfileGuidedSavePlacement(&cgFunc, &domInfo, &loopInfo, &wkCand);
  }
  if (wkCand.saveAtEntryBBs.empty() || wkCand.saveAtProlog) {
    return;
  }
  if (!PrologBBHoist(wkCand.saveAtEntryBBs)) {
    saveInfo.Clear();
    return;
  }

  SPreWorkCand swkCand(&alloc);
  swkCand.occBBs = wkCand.occBBs;
  swkCand.saveBBs = saveInfo.prologBBs;
  DoRestorePlacementOpt(&cgFunc, &pdomInfo, &swkCand);
  if (swkCand.saveBBs.empty() || swkCand.restoreAtEpilog) {
    saveInfo.Clear();
    return;
  }
  for (auto bbId : swkCand.restoreAtEntryBBs) {
    // saveBB post dom firstBB, it's no benefit
    if (pdomInfo.PostDominate(*cgFunc.GetBBFromID(bbId), *cgFunc.GetFirstBB())) {
      saveInfo.Clear();
      return;
    }
    saveInfo.epilogBBs.insert(bbId);
  }
  for (auto bbId : swkCand.restoreAtExitBBs) {
    // saveBB post dom firstBB, it's no benefit
    if (pdomInfo.PostDominate(*cgFunc.GetBBFromID(bbId), *cgFunc.GetFirstBB())) {
      saveInfo.Clear();
      return;
    }
    auto *lastInsn =  cgFunc.GetBBFromID(bbId)->GetLastMachineInsn();
    if (lastInsn != nullptr && (lastInsn->IsBranch() || lastInsn->IsTailCall()) &&
        CheckInsnNeedProEpilog(cgFunc, *lastInsn)) {
      saveInfo.Clear();
      return;
    }
    saveInfo.epilogBBs.insert(bbId);
  }

  if (CG_DEBUG_FUNC(cgFunc)) {
    LogInfo::MapleLogger() << "Dump result of " << PhaseName() << "\n";
    saveInfo.Dump();
  }
}

void CgProEpilogAnalysis::GetAnalysisDependence(AnalysisDep &aDep) const {
  aDep.AddRequired<CgLoopAnalysis>();
  aDep.AddRequired<CgDomAnalysis>();
  aDep.AddRequired<CgPostDomAnalysis>();
  aDep.SetPreservedAll();
}

bool CgProEpilogAnalysis::PhaseRun(maplebe::CGFunc &f) {
   if (f.GetMirModule().GetSrcLang() != kSrcLangC || f.GetFunction().IsVarargs()) {
    return false;
  }

  if (CG_DEBUG_FUNC(f)) {
    DotGenerator::GenerateDot("before-proepiloganalysis", f, f.GetMirModule());
  }

  auto *dom = GET_ANALYSIS(CgDomAnalysis, f);
  CHECK_FATAL(dom != nullptr, "null ptr check");
  auto *pdom = GET_ANALYSIS(CgPostDomAnalysis, f);
  CHECK_FATAL(pdom != nullptr, "null ptr check");
  auto *loop = GET_ANALYSIS(CgLoopAnalysis, f);
  CHECK_FATAL(loop != nullptr, "null ptr check");

  proepilogAnalysis = f.GetCG()->CreateProEpilogAnalysis(*GetPhaseMemPool(), f, *dom, *pdom, *loop);
  if (proepilogAnalysis->NeedProEpilog()) {
    proepilogAnalysis->Analysis();
  }

  return false;
};

MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgProEpilogAnalysis, proepiloganalysis)

bool CgGenProEpiLog::PhaseRun(maplebe::CGFunc &f) {
  GenProEpilog *genPE = nullptr;
#if (defined(TARGAARCH64) && TARGAARCH64) || (defined(TARGRISCV64) && TARGRISCV64)
  const ProEpilogSaveInfo *saveInfo = nullptr;
  if (Globals::GetInstance()->GetOptimLevel() > CGOptions::kLevel0 && !CGOptions::OptimizeForSize()) {
    MaplePhase *it = GetAnalysisInfoHook()->ForceRunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(
        &CgProEpilogAnalysis::id, f);
    saveInfo = static_cast<CgProEpilogAnalysis*>(it)->GetResult();
  }
  genPE = GetPhaseAllocator()->New<AArch64GenProEpilog>(f, *ApplyTempMemPool(), saveInfo);
#endif
#if defined(TARGARM32) && TARGARM32
  genPE = GetPhaseAllocator()->New<Arm32GenProEpilog>(f);
#endif
#if defined(TARGX86_64) && TARGX86_64
  genPE = GetPhaseAllocator()->New<X64GenProEpilog>(f);
#endif
  genPE->Run();
  return false;
}
}  /* namespace maplebe */
