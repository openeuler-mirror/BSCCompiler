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
#include "isolate_fastpath.h"
#if TARGAARCH64
#include "aarch64_isolate_fastpath.h"
#endif
#include "cgfunc.h"

namespace maplebe {
using namespace maple;

bool CgIsolateFastPath::PhaseRun(maplebe::CGFunc &f) {
  IsolateFastPath *isolateFastPath = nullptr;
#if TARGAARCH64
  isolateFastPath = GetPhaseAllocator()->New<AArch64IsolateFastPath>(f);
#endif
  isolateFastPath->Run();
  return false;
}
}  /* namespace maplebe */
