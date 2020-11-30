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
#ifndef _MPL_STACK_UNWIND_H_
#define _MPL_STACK_UNWIND_H_

#include <functional>
#include <string>
#include <map>
#include <jni.h>
#include "stack_unwinder.h"

namespace maplert {
const int kMethodDescOffset = 1;

const int kPrevInsnOffset = 1;

// offset used to tag a java frame, we adjust return address of a java frame
// with this adderss, so that we can quickly tell whether a frame is java or not.
const int kJavaFrameRaTag = 1;

class EHFrameInfo : public JavaFrameInfo {
 public:
  void LookupExceptionHandler(const _Unwind_Exception &unwindException);

  EHFrameInfo(const JavaFrame &frame, _Unwind_Exception &unwindException) : JavaFrameInfo(frame) {
    mUnwindException = &unwindException;
    // resolved is false in java function written with assembly
    if (resolved) {
      LookupExceptionHandler(unwindException);
    }
  }
  EHFrameInfo(const EHFrameInfo&) = default;
  ~EHFrameInfo() {
    mCatchCode = nullptr;
    mCleanupCode = nullptr;
    mGeneralHandler = nullptr;
    mUnwindException = nullptr;
  }
  const void *GetGeneralExceptionHandler() const {
    return mGeneralHandler;
  }

  const void *GetCatchCode() const {
    return mCatchCode;
  }

  const void *GetReturnAddress() const {
    return javaFrame.GetReturnAddress();
  }

  const void *GetFrameAddress() const {
    return javaFrame.GetFrameAddress();
  }

  // this will modify the return address of this frame to the exception handler, thus
  // after the epilogue is executed, control flow turns to the exception handler.
  inline void ChainToGeneralHandler(const void *handler) {
    if (mGeneralHandler != nullptr) {
      CallChain *fa = javaFrame.GetFrameAddress();
      fa->returnAddress = reinterpret_cast<const uint32_t*>(handler);
    }
  }

  // frameinfo is usually from the caller frame
  inline void ChainToCallerEHFrame(const EHFrameInfo &frameinfo) {
    uintptr_t handler = reinterpret_cast<uintptr_t>(frameinfo.GetGeneralExceptionHandler());
    ChainToGeneralHandler(reinterpret_cast<void*>(handler));
  }

  void Dump(const std::string msg, std::stringstream &ss) override {
    JavaFrameInfo::Dump(msg, ss);
    if (mCleanupCode != nullptr) {
      ss << "\t" << "cleanup code : " << std::hex << reinterpret_cast<uintptr_t>(mCleanupCode) << std::dec;
    }
    if (mCatchCode != nullptr) {
      ss << "\t" << "catch code   : " << std::hex << reinterpret_cast<uintptr_t>(mCatchCode) << std::dec;
    }
  }
 private:
  const void *mCatchCode   = nullptr;            // exception handler
  const void *mCleanupCode = nullptr;            // extended epilogue (including clean up code)
  const void *mGeneralHandler = nullptr;         // exception handler or extended epilogue (including clean up code).
  _Unwind_Exception *mUnwindException = nullptr; // this value is propagated from EHStackInfo.
};

class EHStackInfo {
 public:
  void Build(_Unwind_Exception &unwindException, bool isRet = false);
  void ChainAllEHFrames(const _Unwind_Exception &unwindException, bool isRet = false, bool isImplicitNPE = false);
  EHStackInfo() : priUnwindException(nullptr) {}
  explicit EHStackInfo(_Unwind_Exception &unwindException) {
    priUnwindException = &unwindException;
    Build(unwindException);
  }
  ~EHStackInfo() {
    priUnwindException = nullptr;
  }
 private:
  std::vector<EHFrameInfo> priEhStackInfo;
  BasicFrame C2RStub;
  _Unwind_Exception *priUnwindException;
};

void RaiseException(struct _Unwind_Exception &unwindException, bool isRet = false, bool isImplicitNPE = false);
} // end namespace maplert

#endif // _MPL_STACK_UNWIND_H_
