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

#include "cg_prop.h"

namespace maplebe {
void CGProp::DoCopyProp() {
  FOR_ALL_BB(bb, cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      /* change to optimize level opt */
      CopyProp(*insn);
    }
  }
  cgDce->DoDce();
}

void CGProp::DoTargetProp() {
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
  if (CGOptions::GetInstance().GetOptimizeLevel() == 2) {
    PropPatternOpt();
  }
}

bool CgCopyProp::PhaseRun(maplebe::CGFunc &f) {
  CGSSAInfo *ssaInfo = GET_ANALYSIS(CgSSAConstruct, f);
  CGProp *cgProp = f.GetCG()->CreateCGProp(*GetPhaseMemPool(),f, *ssaInfo);
  cgProp->DoCopyProp();
  return false;
}
void CgCopyProp::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<CgSSAConstruct>();
  aDep.AddPreserved<CgSSAConstruct>();
}

bool CgTargetProp::PhaseRun(maplebe::CGFunc &f) {
  CGSSAInfo *ssaInfo = GET_ANALYSIS(CgSSAConstruct, f);
  CGProp *cgProp = f.GetCG()->CreateCGProp(*GetPhaseMemPool(),f, *ssaInfo);
  cgProp->DoTargetProp();
  return false;
}
void CgTargetProp::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<CgSSAConstruct>();
  aDep.AddPreserved<CgSSAConstruct>();
}
}
