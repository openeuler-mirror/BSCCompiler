/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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

#include "loop.h"
#include "cg_prop.h"

namespace maplebe {
void CGProp::DoCopyProp() {
  CopyProp();
  cgDce->DoDce();
}

void CGProp::DoTargetProp() {
  DoCopyProp();
  /* instruction level opt */
  FOR_ALL_BB(bb, cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      TargetProp(*insn);
    }
  }
  /* pattern  level opt */
  if (CGOptions::GetInstance().GetOptimizeLevel() == CGOptions::kLevel2) {
    PropPatternOpt();
  }
}

Insn *PropOptimizePattern::FindDefInsn(const VRegVersion *useVersion) const {
  if (!useVersion) {
    return nullptr;
  }
  DUInsnInfo *defInfo = useVersion->GetDefInsnInfo();
  if (!defInfo) {
    return nullptr;
  }
  return defInfo->GetInsn();
}

bool CgCopyProp::PhaseRun(maplebe::CGFunc &f) {
  CGSSAInfo *ssaInfo = GET_ANALYSIS(CgSSAConstruct, f);
  LiveIntervalAnalysis *ll = GET_ANALYSIS(CGliveIntervalAnalysis, f);
  CGProp *cgProp = f.GetCG()->CreateCGProp(*GetPhaseMemPool(), f, *ssaInfo, *ll);
  cgProp->DoCopyProp();
  ll->ClearBFS();
  return false;
}
void CgCopyProp::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<CgSSAConstruct>();
  aDep.AddRequired<CGliveIntervalAnalysis>();
  aDep.AddPreserved<CgSSAConstruct>();
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgCopyProp, cgcopyprop)

bool CgTargetProp::PhaseRun(maplebe::CGFunc &f) {
  CGSSAInfo *ssaInfo = GET_ANALYSIS(CgSSAConstruct, f);
  LiveIntervalAnalysis *ll = GET_ANALYSIS(CGliveIntervalAnalysis, f);
  CGProp *cgProp = f.GetCG()->CreateCGProp(*GetPhaseMemPool(), f, *ssaInfo, *ll);
  cgProp->DoTargetProp();
  ll->ClearBFS();
  return false;
}
void CgTargetProp::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<CgSSAConstruct>();
  aDep.AddRequired<CGliveIntervalAnalysis>();
  aDep.AddPreserved<CgSSAConstruct>();
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgTargetProp, cgtargetprop)
}
