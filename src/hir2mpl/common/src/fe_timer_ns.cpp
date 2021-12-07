/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "fe_timer_ns.h"
#include "types_def.h"

namespace maple {
void FETimerNS::Start() {
  (void)clock_gettime(CLOCK_REALTIME, &timeStart);
}

void FETimerNS::Stop() {
  (void)clock_gettime(CLOCK_REALTIME, &timeEnd);
}

int64_t FETimerNS::GetTimeNS() const {
  const int64 nsInS = 1000000000;
  return nsInS * (timeEnd.tv_sec - timeStart.tv_sec) + (timeEnd.tv_nsec - timeStart.tv_nsec);
}
}  // namespace maple
