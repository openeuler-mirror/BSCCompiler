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
#include "exception/exception_handling.h"

#include <vector>
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <link.h>
#include "libs.h"
#include "exception/mpl_exception.h"
#include "exception/mrt_exception.h"
#include "exception_store.h"
#include "chosen.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

namespace maplert {
void EHFrameInfo::LookupExceptionHandler(const _Unwind_Exception &unwindException) {
  EHTable ehTable(GetStartPC(), GetEndPC(), GetJavaFrame().ip);
  ehTable.ScanExceptionTable(kSearchPhase, true, unwindException);
  mGeneralHandler = reinterpret_cast<void*>(ehTable.results.landingPad);

  if (ehTable.results.caughtByJava) {
    MExceptionHeader *exceptionHeader = GetThrownExceptionHeader(unwindException);
    exceptionHeader->tTypeIndex = ehTable.results.tTypeIndex;
    exceptionHeader->caughtByJava = true; // for a matching handler
    mCleanupCode = nullptr;
    mCatchCode = reinterpret_cast<void*>(ehTable.results.landingPad);
  } else {
    mCatchCode = nullptr;
    mCleanupCode = reinterpret_cast<void*>(ehTable.results.landingPad);
  }

  // no general handler for this frame means it is an abnormal frame.
  __MRT_ASSERT(mGeneralHandler != nullptr || (!javaFrame.HasFrameStructure()), "");
  return;
}

void EHStackInfo::Build(_Unwind_Exception &unwindException, bool isRet) {
  JavaFrame frame;
  // If UnwindFinish is returned here, it means that no Java frame has been found
  // on the stack in the current n2j block, so no build operation is required in
  // the exception handling phase.
  if (MapleStack::GetLastJavaFrame(frame, nullptr, true) == kUnwindFinish) {
    return;
  }

  (void)isRet;

  if (VLOG_IS_ON(eh)) {
    MplCheck(frame.IsCompiledFrame(), "Build should start at java frame");
  }

  size_t n = 0;
  while (n < MAPLE_STACK_UNWIND_STEP_MAX && frame.IsCompiledFrame()) {
    EHFrameInfo frameInfo(frame, unwindException);

    // the latest java frame might be a fake frame, we ignore this fake frame
    const JavaFrame& calleeframe = frameInfo.GetJavaFrame();
    if (calleeframe.HasFrameStructure()) {
      priEhStackInfo.push_back(frameInfo);
    };

    if (frameInfo.GetCatchCode()) {
      if (VLOG_IS_ON(eh)) {
        std::stringstream ss;
        frameInfo.Dump(">>>> Building eh stack: handler at", ss);
        EHLOG(INFO) << ss.str() << maple::endl;
      }
      break;
    }

    if (VLOG_IS_ON(eh)) {
      std::stringstream ss;
      frameInfo.Dump(">>>> Building eh stack: unwind at", ss);
      EHLOG(INFO) << ss.str() << maple::endl;
    }

    (void)calleeframe.UnwindToNominalCaller(frame);
    ++n;
  }
}

// chain all java frames in this exception-handling stack
// this is done by rewriting the return address of callee frame with the general
// handler of caller frame.
void EHStackInfo::ChainAllEHFrames(const _Unwind_Exception &unwindException, bool isRet, bool isImplicitNPE) {
  MExceptionHeader *exceptionHeader = GetThrownExceptionHeader(unwindException);
  // If the array is equal to 0, it represents a special scenario of a Java method
  // implemented natively.
  if (priEhStackInfo.size() == 0) {
  (void)isRet;
  (void)isImplicitNPE;
    return;
  }


  EHFrameInfo *currentFrameInfo = &priEhStackInfo[0];
  for (unsigned i = 1; i < priEhStackInfo.size(); ++i) {
    EHFrameInfo *callerFrameInfo = &priEhStackInfo[i];
    currentFrameInfo->ChainToCallerEHFrame(*callerFrameInfo);
    currentFrameInfo = callerFrameInfo;
  }

  // if exception is caught by java frame, the last frame in priEhStackInfo holds the handler.
  // otherwise, we treat the return address of last frame in priEhStackInfo as the handler.
  if (exceptionHeader->caughtByJava) {
    MRT_SetReliableUnwindContextStatus();
  } else {
    if (currentFrameInfo->GetJavaFrame().HasFrameStructure()) {
      currentFrameInfo->ChainToGeneralHandler(currentFrameInfo->GetReturnAddress());
    }
    MRT_ThrowExceptionUnsafe(GetThrownObject(unwindException));
    MRT_ClearThrowingException();
  }

}

// This function is called by HandleJavaSignalStub and aims to check whether the
// segv signal results into a java exception. If so it returns PrepareArgsForExceptionCatcher as
// the continuation point for HandleJavaSignalStub.
extern "C" uintptr_t IsThrowingExceptionByRet() {
  // Check whether there is an exception thrown by TLS.
  MrtClass *thrownObject = reinterpret_cast<MrtClass*>(maple::ExceptionVisitor::GetExceptionAddress());
  if (thrownObject != nullptr) {
    return reinterpret_cast<uintptr_t>(&PrepareArgsForExceptionCatcher);
  } else {
    return 0;
  }
}

struct HandlerCatcherArgs {
  uintptr_t uwException;
  intptr_t  typeIndex;
  uintptr_t topJavaHandler;
};

extern "C" void MRT_GetHandlerCatcherArgs(struct HandlerCatcherArgs *cArgs) {
  (void)cArgs;
}

#if defined(__arm__)
extern "C" MRT_EXPORT UnwindReasonCode AdaptationFunc(_Unwind_State, _Unwind_Exception*, _Unwind_Context*) {
  return _URC_CONTINUE_UNWIND;
}
#endif


ATTR_NO_SANITIZE_ADDRESS
void RaiseException(struct _Unwind_Exception &unwindException, bool isRet, bool isImplicitNPE) {
  {
    EHStackInfo ehStack;
    ehStack.Build(unwindException, isRet);

    // 1. chain all java frames in this exception-handling stack
    ehStack.ChainAllEHFrames(unwindException, isRet, isImplicitNPE);
  }

    // 2. the entry point of exception handler is the extended epilogue of top java frame
    // here we invoke _Unwind_Resume in libgcc which invokes _Unwind_RaiseException_Phase2.
    // we adjust the personality to trick _Unwind_RaiseException_Phase2 to continue to
    // the entry point of exception handler.
#if defined(__aarch64__)
    unwindException.private_1 = 0;
#elif defined(__arm__)
    // Add an adaptation function, the first time you enter __gnu_Unwind_Resume will be executed.
    unwindException.unwinder_cache.reserved2 = reinterpret_cast<uintptr_t>(&AdaptationFunc);
    unwindException.unwinder_cache.reserved3 = reinterpret_cast<uintptr_t>(__builtin_return_address(0));
#endif
    _Unwind_Resume(&unwindException);
}
} // namespace maplert
