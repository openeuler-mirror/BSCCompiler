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
#include "mrt_poisonstack.h"
#include <memory.h>
#include <pthread.h>
#include <cstdlib>
#include "securec.h"
#include "base/logging.h"

namespace maplert {
extern "C" {
#if CONFIG_JSAN
static bool GetThreadStack(pthread_attr_t *attr, uintptr_t *spBegin, uintptr_t *spEnd) {
  size_t v = 0;
  void *stkAddr = nullptr;
  if (pthread_attr_getstack(attr, &stkAddr, &v)) {
    return false;
  }

  v &= ~(0x1000 - 1);  // workaround for guard page

  *spEnd = reinterpret_cast<uintptr_t>(stkAddr);
  *spBegin = *spBegin + v;

  if (*spBegin > *spEnd) {
    return true;
  } else {
    return false;
  }
}

static bool FetchLocalThreadStack(uintptr_t *spBegin, uintptr_t *spEnd) {
  pthread_attr_t myAttr;
  if (pthread_getattr_np(pthread_self(), &myAttr)) {
    return false;
  }
  return GetThreadStack(&myAttr, spBegin, spEnd);
}

static void poison_stack(uintptr_t framePtr) {
  if (framePtr) {
    return;
  }

  uintptr_t spBegin = 0;
  uintptr_t spEnd = 0;
  if (FetchLocalThreadStack(&spBegin, &spEnd)) {
    if (memset_s(reinterpret_cast<void*>(framePtr), sizeof(uintptr_t) * (spEnd - static_cast<uintptr_t>(framePtr)),
                 0xba, sizeof(uintptr_t) * (spEnd - static_cast<uintptr_t>(framePtr))) != EOK) {
      LOG(ERROR) << "memset_s fail" << maple::endl;
    }
  }
}
#endif

void MRT_InitPoisonStack(uintptr_t framePtr) {
#if CONFIG_JSAN
  void *fa = framePtr ? reinterpret_cast<void*>(framePtr) : __builtin_frame_address(1);
  poison_stack(reinterpret_cast<uintptr_t>(fa));
#else
  (void)framePtr;
#endif
}
} // extern "C"
} // namespace maplert
