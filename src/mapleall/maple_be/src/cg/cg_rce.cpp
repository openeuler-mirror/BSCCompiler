/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "cg_rce.h"

namespace maplebe {
void RedundantComputeElim::Dump(const Insn *insn1, const Insn *insn2) const {
  CHECK_FATAL(insn1 && insn2, "dump insn is null");
  LogInfo::MapleLogger() << ">>>>>> SameRHSInsnPair in BB(" <<
      insn1->GetBB()->GetId() << ") at {" << kGcount << "} <<<<<<\n";
  insn1->Dump();
  insn2->Dump();
}

bool CgRedundantCompElim::PhaseRun(maplebe::CGFunc &f) {
  CGSSAInfo *ssaInfo = GET_ANALYSIS(CgSSAConstruct, f);
  CHECK_FATAL(ssaInfo != nullptr, "Get ssaInfo failed");
  MemPool *mp = GetPhaseMemPool();
  CHECK_FATAL(mp != nullptr, "get memPool failed");
  auto *rce = f.GetCG()->CreateRedundantCompElim(*mp, f, *ssaInfo);
  CHECK_FATAL(rce != nullptr, "rce instance create failed");
  rce->Run();
  return true;
}

void CgRedundantCompElim::GetAnalysisDependence(AnalysisDep &aDep) const {
  aDep.AddRequired<CgSSAConstruct>();
  aDep.AddPreserved<CgSSAConstruct>();
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgRedundantCompElim, cgredundantcompelim)
} /* namespace maplebe */