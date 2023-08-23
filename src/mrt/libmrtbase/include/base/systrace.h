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
#ifndef MAPLE_RUNTIME_BASE_SYSTRACE_H
#define MAPLE_RUNTIME_BASE_SYSTRACE_H

#include <string>
#include <stdarg.h>
#include <cstdio>

#ifdef __ANDROID__
#define ATRACE_TAG ATRACE_TAG_DALVIK

#include <cutils/trace.h>
#include "securec.h"

#ifndef ATRACE_TAG_RUNTIME
#define ATRACE_TAG_RUNTIME (1 << 23)
#endif

#else
#define ATRACE_BEGIN(label)
#define ATRACE_END()
static inline bool ATRACE_ENABLED() {
  return false;
}
#endif  // __ANDROID__

namespace maple {

class ScopedTrace {
 public:
  explicit ScopedTrace(const char *name, ...);

  explicit ScopedTrace(const std::string &name, ...);

  ~ScopedTrace();
};

extern "C" {

void RuntimeTraceBegin(uint64_t tag, const char *name, ...);

void RuntimeTraceEnd();
}

}  // namespace maple
#endif  // MAPLE_RUNTIME_BASE_SYSTRACE_H
