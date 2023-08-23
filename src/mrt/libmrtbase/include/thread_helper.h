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

#ifndef __MAPLE_THREAD_HELPER_H
#define __MAPLE_THREAD_HELPER_H

#include <cstdint>
#include <map>
#include <functional>
#include "base/mutex.h"
#include "thread_api.h"
#include "gc_callback.h"

namespace maple {

class ThreadHelper {
 public:
  static const uint32_t kInvalidThreadId = 0;
  static bool is_started_;

  // Hold ThreadList::list_
  static std::map<uint32_t, IThread*> threadMap;

  static IThread *FindThreadByThreadId(uint32_t threadId);

  static bool SuspendHandler() {
    IThread *iSelf = IThread::Current();
    // native thread only, avoid SIGSEGV Nesting
    if (UNLIKELY(iSelf == nullptr)) {
      return false;
    }

    if (LIKELY(iSelf->CheckSuspend() != nullptr)) {
      return false;
    }

    iSelf->SetSuspend(true);
    return true;
  }
};

extern std::function<void(maplert::RefVisitor&)> visitGCRootsFunc;
extern std::function<void(irtVisitFunc&)> VisitWeakGRTFunc;
extern std::function<void(maplert::RefVisitor&)> visitWeakGRTFunc;
extern std::function<void(irtVisitFunc&)> VisitWeakGRTConcurrentFunc;
extern std::function<void(uint32_t, jobject)> ClearWeakGRTReferenceFunc;
extern std::function<void(maplert::RefVisitor&)> FixLockedObjectsFunc;

extern std::function<void(maplert::RefVisitor&)> visitGCLocalRefRootsFunc;
extern std::function<void(maplert::RefVisitor&)> visitGCGlobalRefRootsFunc;
extern std::function<void(rootObjectFunc&)> VisitGCThreadExceptionRootsFunc;

// this provides a unified way to pre-initialize classes for each module (dex/so)
typedef void *PreinitModuleClassesCallback(void*);
extern std::function<void(PreinitModuleClassesCallback)> PreinitModuleClassesFunc;
}  // namespace maple
#endif  // __MAPLE_THREAD_HELPER_H
