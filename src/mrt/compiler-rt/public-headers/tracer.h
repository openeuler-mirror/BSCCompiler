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
#ifndef __MAPLE_RUNTIME_TRACER_H
#define __MAPLE_RUNTIME_TRACER_H

#include <string>
#include "mrt_api_common.h"

namespace maplert {
class Tracer {
 public:
  // event, 0 means function enter, 1 means function exit
  virtual void LogMethodTraceEvent(const std::string funcName, int event) = 0;
  virtual ~Tracer() = default;
};

MRT_EXPORT void SetTracer(Tracer *tracer);

MRT_EXPORT Tracer *GetTracer();

static inline bool IsTracingEnabled() {
  return (GetTracer() == nullptr);
}
}; // namespace maplert

#endif // __MAPLE_RUNTIME_TRACER_H
