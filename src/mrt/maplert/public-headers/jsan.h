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
#ifndef JSAN_H
#define JSAN_H


#include <list>
#include <queue>
#include <unordered_map>
#include "jni.h"
#include "mrt_mm_config_common.h"

namespace maplert {
typedef uintptr_t address_t;

#define JSAN_ADD_OBJ(objAddr, objSize)
#define JSAN_ADD_CLASS_METADATA(objAddr)
#define JSAN_FREE(obj, RealFreeFunc, internalSize) internalSize = RealFreeFunc(obj)
#define JSAN_CHECK_OBJ(obj)

static inline void JsanliteFree(address_t) {
  return;
}

static inline void JsanliteError(address_t) {
  return;
}

static inline size_t JsanliteGetPayloadSize(size_t) {
  return 0;
};

static inline void JsanliteInit() {
  return;
}
} // namespace maplert

#endif
