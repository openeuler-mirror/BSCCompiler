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
#include "cg_dce.h"
#include "cg.h"
namespace maplebe {
void CGDce::DoDce() {
  for (auto &ssaIt : GetSSAInfo()->GetAllSSAOperands()) {
    if (ssaIt.second != nullptr && !ssaIt.second->IsDeleted()) {
      RemoveUnuseDef(*ssaIt.second);
    }
  }
}

bool CgDce::PhaseRun(maplebe::CGFunc &f) {
  CGSSAInfo *ssaInfo = GET_ANALYSIS(CgSSAConstruct, f);
  CGDce *cgDce = f.GetCG()->CreateCGDce(*GetPhaseMemPool(),f, *ssaInfo);
  cgDce->DoDce();
  return false;
}

void CgDce::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<CgSSAConstruct>();
  aDep.AddPreserved<CgSSAConstruct>();
}
}
