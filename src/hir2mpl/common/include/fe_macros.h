/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef HIR2MPL_INCLUDE_FE_MACROS_H
#define HIR2MPL_INCLUDE_FE_MACROS_H
#include <cstdio>
#include <sys/time.h>
#include <iostream>
#include <sstream>
#include "fe_options.h"
#include "mpl_logging.h"

#define TIMEIT(message, stmt) do {                                                                                 \
  if (FEOptions::GetInstance().IsDumpTime()) {                                                                     \
    struct timeval start, end;                                                                                     \
    (void)gettimeofday(&start, NULL);                                                                              \
    stmt;                                                                                                          \
    (void)gettimeofday(&end, NULL);                                                                                \
    float t = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)) * 1.0 / 1000000.0; \
    INFO(kLncInfo, "[TIME] %s: %.3f sec", message, t);                                                             \
  } else {                                                                                                         \
    stmt;                                                                                                          \
  }                                                                                                                \
} while (0)                                                                                                        \

#define PHASE_TIMER(message, stmt) do {                                               \
  if (FEOptions::GetInstance().IsDumpPhaseTime()) {                                   \
    struct timespec start = {0, 0};                                                   \
    struct timespec end = {0, 0};                                                     \
    (void)clock_gettime(CLOCK_REALTIME, &start);                                      \
    stmt;                                                                             \
    (void)clock_gettime(CLOCK_REALTIME, &end);                                        \
    constexpr int64_t nsInS = 1000000000;                                             \
    int64_t t = (end.tv_sec - start.tv_sec) * nsInS + (end.tv_nsec - start.tv_nsec);  \
    INFO(kLncInfo, "[PhaseTime]   %s: %lld ns", message, t);                          \
  } else {                                                                            \
    stmt;                                                                             \
  }                                                                                   \
} while (0)                                                                           \

#define FE_INFO_LEVEL(level, fmt, ...) do {               \
  if (FEOptions::GetInstance().GetDumpLevel() >= level) { \
    INFO(kLncInfo, fmt, ##__VA_ARGS__);                   \
  }                                                       \
} while (0)                                               \

#define BIT_XOR(v1, v2) (((v1) && (!(v2))) || ((!(v1)) && (v2)))

#define TRY_DO(run) do {        \
  CHECK_FATAL(run, "%s", #run); \
} while (0)

// for user
#define FE_ERR(num, loc, fmt, ...) do {                                                                             \
  if (PRINT_LEVEL_USER <= kLlErr) {                                                                                 \
      std::ostringstream ss;                                                                                        \
      ss << FEManager::GetModule().GetFileNameFromFileNum(loc.fileIdx) << ":" << loc.line << " error: " << fmt;     \
      logInfo.InsertUserErrorMessage(                                                                               \
          loc, logInfo.EmitLogToStringForUser(num, kLlErr, ss.str().c_str(), ##__VA_ARGS__));                       \
  }                                                                                                                 \
} while (0)

#define FE_WARN(num, loc, fmt, ...) do {                                                                            \
  if (PRINT_LEVEL_USER <= kLlWarn) {                                                                                \
      std::ostringstream ss;                                                                                        \
      ss << FEManager::GetModule().GetFileNameFromFileNum(loc.fileIdx) << ":" << loc.line << " warning: " << fmt;   \
      logInfo.InsertUserWarnMessage(                                                                                \
          loc, logInfo.EmitLogToStringForUser(num, kLlWarn, ss.str().c_str(), ##__VA_ARGS__));                      \
  }                                                                                                                 \
} while (0)
#endif  // ~HIR2MPL_INCLUDE_FE_MACROS_H