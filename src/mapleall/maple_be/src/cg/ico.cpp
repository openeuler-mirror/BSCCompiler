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
#include "ico.h"
#include "cg_option.h"
#ifdef TARGAARCH64
#include "aarch64_ico.h"
#include "aarch64_isa.h"
#include "aarch64_insn.h"
#elif TARGRISCV64
#include "riscv64_ico.h"
#include "riscv64_isa.h"
#include "riscv64_insn.h"
#elif TARGARM32
#include "arm32_ico.h"
#include "arm32_isa.h"
#include "arm32_insn.h"
#endif
#include "cg.h"

/*
 * This phase implements if-conversion optimization,
 * which tries to convert conditional branches into cset/csel instructions
 */
#define ICO_DUMP_NEWPM CG_DEBUG_FUNC(f)
namespace maplebe {
Insn *ICOPattern::FindLastCmpInsn(BB &bb) const {
  if (bb.GetKind() != BB::kBBIf) {
    return nullptr;
  }
  FOR_BB_INSNS_REV(insn, (&bb)) {
    if (cgFunc->GetTheCFG()->GetInsnModifier()->IsCompareInsn(*insn)) {
      return insn;
    }
  }
  return nullptr;
}

/*
 * Find IF-THEN-ELSE or IF-THEN basic block pattern,
 * and then invoke DoOpt(...) to finish optimize.
 */
bool ICOPattern::Optimize(BB &curBB) {
  if (curBB.GetKind() != BB::kBBIf) {
    return false;
  }
  BB *ifBB = nullptr;
  BB *elseBB = nullptr;
  BB *joinBB = nullptr;

  BB *thenDest = cgFunc->GetTheCFG()->GetTargetSuc(curBB);
  BB *elseDest = curBB.GetNext();
  CHECK_FATAL(thenDest != nullptr, "then_dest is null in ITEPattern::Optimize");
  CHECK_FATAL(elseDest != nullptr, "else_dest is null in ITEPattern::Optimize");
  /* IF-THEN-ELSE */
  if (thenDest->NumPreds() == 1 && thenDest->NumSuccs() == 1 && elseDest->NumSuccs() == 1 &&
      elseDest->NumPreds() == 1 && thenDest->GetSuccs().front() == elseDest->GetSuccs().front()) {
    ifBB = thenDest;
    elseBB = elseDest;
    joinBB = thenDest->GetSuccs().front();
  } else if (elseDest->NumPreds() == 1 && elseDest->NumSuccs() == 1 && elseDest->GetSuccs().front() == thenDest) {
    /* IF-THEN */
    ifBB = nullptr;
    elseBB = elseDest;
    joinBB = thenDest;
  } else {
    /* not a form we can handle */
    return false;
  }
  if (cgFunc->GetTheCFG()->InLSDA(elseBB->GetLabIdx(), *cgFunc->GetEHFunc()) ||
      cgFunc->GetTheCFG()->InSwitchTable(elseBB->GetLabIdx(), *cgFunc)) {
    return false;
  }

  if (ifBB != nullptr &&
      (cgFunc->GetTheCFG()->InLSDA(ifBB->GetLabIdx(), *cgFunc->GetEHFunc()) ||
       cgFunc->GetTheCFG()->InSwitchTable(ifBB->GetLabIdx(), *cgFunc))) {
    return false;
  }
  return DoOpt(curBB, ifBB, elseBB, *joinBB);
}

bool CgIco::PhaseRun(maplebe::CGFunc &f) {
  LiveAnalysis *live = GET_ANALYSIS(CgLiveAnalysis, f);
  if (ICO_DUMP_NEWPM) {
    DotGenerator::GenerateDot("ico-before", f, f.GetMirModule());
  }
  MemPool *memPool = GetPhaseMemPool();
  IfConversionOptimizer *ico = nullptr;
#if TARGAARCH64 || TARGRISCV64
  ico = memPool->New<AArch64IfConversionOptimizer>(f, *memPool);
#endif
#if TARGARM32
  ico = memPool->New<Arm32IfConversionOptimizer>(f, *memPool);
#endif
  const std::string &funcClass = f.GetFunction().GetBaseClassName();
  const std::string &funcName = f.GetFunction().GetBaseFuncName();
  std::string name = funcClass + funcName;
  ico->Run(name);
  if (ICO_DUMP_NEWPM) {
    DotGenerator::GenerateDot("ico-after", f, f.GetMirModule());
  }
  /* the live range info may changed, so invalid the info. */
  if (live != nullptr) {
    live->ClearInOutDataInfo();
  }
  return false;
}
void CgIco::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<CgLiveAnalysis>();
  aDep.PreservedAllExcept<CgLiveAnalysis>();
}
}  /* namespace maplebe */
