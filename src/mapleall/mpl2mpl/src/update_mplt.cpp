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
#include "update_mplt.h"
#include "bin_mpl_export.h"
#include "bin_mplt.h"

namespace maple {
bool M2MUpdateMplt::PhaseRun(MIRModule &m) {
  auto *cg = GET_ANALYSIS(M2MCallGraph, m);
  CHECK_FATAL(cg != nullptr, "Expecting a valid CallGraph, found nullptr.");
  BinaryMplt *binMplt = m.GetBinMplt();
  CHECK_FATAL(binMplt != nullptr, "Expecting a valid binMplt, found nullptr.");
  UpdateMplt update;
  update.UpdateCgField(*binMplt, *cg);
  delete m.GetBinMplt();
  m.SetBinMplt(nullptr);
  return false;
}

void M2MUpdateMplt::GetAnalysisDependence(AnalysisDep &aDep) const {
  aDep.AddRequired<M2MCallGraph>();
}
}  // namespace maple
