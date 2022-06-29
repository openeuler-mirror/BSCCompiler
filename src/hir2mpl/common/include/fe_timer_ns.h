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
#ifndef HIR2MPL_INCLUDE_COMMON_FE_TIMER_NS_H
#define HIR2MPL_INCLUDE_COMMON_FE_TIMER_NS_H
#include <ctime>
#include <cstdlib>

namespace maple {
class FETimerNS {
 public:
  FETimerNS() = default;
  ~FETimerNS() = default;
  void Start();
  void Stop();
  int64_t GetTimeNS() const;

 private:
  struct timespec timeStart = {0, 0};
  struct timespec timeEnd = {0, 0};
};  // class FETimerNS
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_COMMON_FE_TIMER_NS_H