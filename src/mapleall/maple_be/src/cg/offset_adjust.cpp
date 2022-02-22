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
#include "offset_adjust.h"
#if TARGAARCH64
#include "aarch64_offset_adjust.h"
#elif TARGRISCV64
#include "riscv64_offset_adjust.h"
#endif
#if TARGARM32
#include "arm32_offset_adjust.h"
#endif

#include "cgfunc.h"

namespace maplebe {
using namespace maple;
bool CgFrameFinalize::PhaseRun(maplebe::CGFunc &f) {
  FrameFinalize *offsetAdjustment = nullptr;
#if TARGAARCH64 || TARGRISCV64
  offsetAdjustment = GetPhaseAllocator()->New<AArch64FPLROffsetAdjustment>(f);
#endif
#if TARGARM32
  offsetAdjustment = GetPhaseAllocator()->New<Arm32FPLROffsetAdjustment>(f);
#endif
  offsetAdjustment->Run();
  return false;
}

}  /* namespace maplebe */
