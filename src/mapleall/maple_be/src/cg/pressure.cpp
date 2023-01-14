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
#include "pressure.h"
#if TARGAARCH64
#include "aarch64_schedule.h"
#elif defined(TARGRISCV64) && TARGRISCV64
#include "riscv64_schedule.h"
#endif
#include "deps.h"

namespace maplebe {
/* ------- RegPressure function -------- */
int32 RegPressure::maxRegClassNum = 0;

/* print regpressure information */
void RegPressure::DumpRegPressure() const {
  PRINT_STR_VAL("Priority: ", priority);
  PRINT_STR_VAL("maxDepth: ", maxDepth);
  PRINT_STR_VAL("near: ", near);
  PRINT_STR_VAL("callNum: ", callNum);

  LogInfo::MapleLogger() << "\n";
}
} /* namespace maplebe */

