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
#include "yieldpoint.h"
#include "loop.h"
#if TARGAARCH64
#include "aarch64_yieldpoint.h"
#elif defined(TARGRISCV64) && TARGRISCV64
#include "riscv64_yieldpoint.h"
#endif
#if TARGARM32
#include "arm32_yieldpoint.h"
#endif
#include "cgfunc.h"

namespace maplebe {
using namespace maple;

bool CgYieldPointInsertion::PhaseRun(maplebe::CGFunc &f) {
  YieldPointInsertion *yieldPoint = nullptr;
  (void)GetAnalysisInfoHook()->ForceRunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(&CgLoopAnalysis::id, f);
#if TARGAARCH64 || TARGRISCV64
  yieldPoint = GetPhaseAllocator()->New<AArch64YieldPointInsertion>(f);
#endif
#if TARGARM32
  yieldPoint = GetPhaseAllocator()->New<Arm32YieldPointInsertion>(f);
#endif
  yieldPoint->Run();
  return false;
}
}  /* namespace maplebe */
