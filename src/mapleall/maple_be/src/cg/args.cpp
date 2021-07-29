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
#include "args.h"
#if TARGAARCH64
#include "aarch64_args.h"
#elif TARGRISCV64
#include "riscv64_args.h"
#endif
#if TARGARM32
#include "arm32_args.h"
#endif
#include "cgfunc.h"

namespace maplebe {
using namespace maple;
bool CgMoveRegArgs::PhaseRun(maplebe::CGFunc &f) {
  MemPool *memPool = GetPhaseMemPool();
  MoveRegArgs *movRegArgs = nullptr;
#if TARGAARCH64 || TARGRISCV64
  movRegArgs = memPool->New<AArch64MoveRegArgs>(f);
#endif
#if TARGARM32
  movRegArgs = memPool->New<Arm32MoveRegArgs>(f);
#endif
  movRegArgs->Run();
  return true;
}
MAPLE_TRANSFORM_PHASE_REGISTER(CgMoveRegArgs, moveargs)
}  /* namespace maplebe */
