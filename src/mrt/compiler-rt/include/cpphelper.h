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
#ifndef MAPLE_RUNTIME_CPP_HELPER_H
#define MAPLE_RUNTIME_CPP_HELPER_H

#include <jni.h>
#include "thread_api.h"
#include "mm_utils.h"
#include "cinterface.h"
#include "saferegion.h"

// helper utilities for runtime C++ code.
namespace maplert {
// Scoped guard for saferegion.
class ScopedEnterSaferegion {
 public:
  __attribute__ ((always_inline))
  ScopedEnterSaferegion() {
    Mutator *mutator = CurrentMutatorPtr();
    stateChanged = (mutator != nullptr) ? mutator->EnterSaferegion(true) : false;
  }

  __attribute__ ((always_inline))
  ~ScopedEnterSaferegion() {
    if (LIKELY(stateChanged)) {
      Mutator *mutator = CurrentMutatorPtr(); // state changed, mutator must be not null
      (void)mutator->LeaveSaferegion();
    }
  }
 private:
  bool stateChanged;
};

// Scoped guard for access java objects from native code.
class ScopedObjectAccess {
 public:
  __attribute__ ((always_inline))
  ScopedObjectAccess()
      : mutator(CurrentMutator()),
        stateChanged(mutator.LeaveSaferegion()) {}

  __attribute__ ((always_inline))
  explicit ScopedObjectAccess(JNIEnv *env __attribute__((unused))) : ScopedObjectAccess() {}

  __attribute__ ((always_inline))
  explicit ScopedObjectAccess(maple::IThread &self __attribute__((unused))) : ScopedObjectAccess() {}

  __attribute__ ((always_inline))
  ~ScopedObjectAccess() {
    if (LIKELY(stateChanged)) {
      (void)mutator.EnterSaferegion(false);
    }
  }
 private:
  Mutator &mutator;
  bool stateChanged;
};

// Scoped enable GC for current thread.
class ScopedEnableGC {
 public:
  ScopedEnableGC() {
    MRT_GCInitThreadLocal(false);
  }

  __attribute__ ((always_inline))
  ~ScopedEnableGC() {
    MRT_GCFiniThreadLocal();
  }
};

extern "C" void MRT_SetCurrentCompiledMethod(void *func);
extern "C" void *MRT_GetCurrentCompiledMethod();

// return value is in d0 if return type is float/double.
// Since R2CForwardStubXX is assembly code, we have to wrap them with partially
// specialized class template. Note C++ does not support partially specializing
// template method/function. Thus we place these template functions as static methods
// in some template class.
#if defined(__arm__)
extern "C" jlong   R2CForwardStubLong(...);
extern "C" void    R2CForwardStubVoid(...);

// note arm compiler always passes return value via r0/r1 for functions with variadic argument list.
// refer to r2c_stub_arm.S to see how this is handled.
extern "C" jfloat  R2CForwardStubFloat(...);
extern "C" jdouble R2CForwardStubDouble(...);
#else
extern "C" jlong   R2CForwardStubLong(void *func, ...);
extern "C" void    R2CForwardStubVoid(void *func, ...);
extern "C" jfloat  R2CForwardStubFloat(void *func, ...);
extern "C" jdouble R2CForwardStubDouble(void *func, ...);
#endif

extern "C" jlong   R2CBoxedStubLong(void *func, jvalue *argJvalue, size_t stackSize, size_t dregSize);
extern "C" void    R2CBoxedStubVoid(void *func, jvalue *argJvalue, size_t stackSize, size_t dregSize);
extern "C" jfloat  R2CBoxedStubFloat(void *func, jvalue *argJvalue, size_t stackSize, size_t dregSize);
extern "C" jdouble R2CBoxedStubDouble(void *func, jvalue *argJvalue, size_t stackSize, size_t dregSize);

// generic stubs for calling java code from maple runtime.
// setup a stub frame to call java method represented by func
template<typename Ret>
class RuntimeStub {
 public:
  template<typename Func, typename... Args>
  static Ret FastCallCompiledMethod(Func &&func, Args&&... args) {
#if defined(__arm__)
    MRT_SetCurrentCompiledMethod(reinterpret_cast<void*>(func));
    Ret result = reinterpret_cast<Ret>(R2CForwardStubLong(std::forward<Args>(args)...));
#else
    Ret result = (Ret) R2CForwardStubLong(reinterpret_cast<void*>(func), std::forward<Args>(args)...);
#endif
    return result;
  }

  template<typename Func, typename... Args>
  static jvalue FastCallCompiledMethodJ(Func &&func, Args&&... args) {
    jvalue result;
#if defined(__arm__)
    MRT_SetCurrentCompiledMethod(reinterpret_cast<void*>(func));
    result.j = R2CForwardStubLong(std::forward<Args>(args)...);
#else
    result.j = R2CForwardStubLong(reinterpret_cast<void*>(func), std::forward<Args>(args)...);
#endif
    return result;
  }

  template<typename Func>
  static Ret SlowCallCompiledMethod(Func func, jvalue *argJvalue, size_t stackSize, size_t dregSize) {
    Ret result = (Ret)R2CBoxedStubLong(reinterpret_cast<void*>(func), argJvalue, stackSize, dregSize);
    return result;
  }

  template<typename Func>
  static jvalue SlowCallCompiledMethodJ(Func func, jvalue *argJvalue, size_t stackSize, size_t dregSize) {
    jvalue result;
    result.j = R2CBoxedStubLong(reinterpret_cast<void*>(func), argJvalue, stackSize, dregSize);
    return result;
  }
};

// specialized RuntimeStub for returning void
template<>
class RuntimeStub<void> {
 public:
  template<typename Func, typename... Args>
  static void FastCallCompiledMethod(Func &&func, Args&&... args) {
#if defined(__arm__)
    MRT_SetCurrentCompiledMethod(reinterpret_cast<void*>(func));
    R2CForwardStubVoid(std::forward<Args>(args)...);
#else
    R2CForwardStubVoid(reinterpret_cast<void*>(func), std::forward<Args>(args)...);
#endif
    return;
  }

  template<typename Func>
  static void SlowCallCompiledMethod(Func func, jvalue *argJvalue, size_t stackSize, size_t dregSize) {
    R2CBoxedStubVoid(reinterpret_cast<void*>(func), argJvalue, stackSize, dregSize);
    return;
  }
};

template<>
class RuntimeStub<jfloat> {
 public:
  template<typename Func, typename... Args>
  static jfloat FastCallCompiledMethod(Func &&func, Args&&... args) {
#if defined(__arm__)
    MRT_SetCurrentCompiledMethod(reinterpret_cast<void*>(func));
    jfloat result = R2CForwardStubFloat(std::forward<Args>(args)...);
#else
    jfloat result = R2CForwardStubFloat(reinterpret_cast<void*>(func), std::forward<Args>(args)...);
#endif
    return result;
  }

  template<typename Func>
  static jfloat SlowCallCompiledMethod(Func func, jvalue *argJvalue, size_t stackSize, size_t dregSize) {
    jfloat result = R2CBoxedStubFloat(reinterpret_cast<void*>(func), argJvalue, stackSize, dregSize);
    return result;
  }
};

template<>
class RuntimeStub<jdouble> {
 public:
  template<typename Func, typename... Args>
  static jdouble FastCallCompiledMethod(Func &&func, Args&&... args) {
#if defined(__arm__)
    MRT_SetCurrentCompiledMethod(reinterpret_cast<void*>(func));
    jdouble result = R2CForwardStubDouble(std::forward<Args>(args)...);
#else
    jdouble result = R2CForwardStubDouble(reinterpret_cast<void*>(func), std::forward<Args>(args)...);
#endif
    return result;
  }

  template<typename Func>
  static jdouble SlowCallCompiledMethod(Func func, jvalue *argJvalue, size_t stackSize, size_t dregSize) {
    jdouble result = R2CBoxedStubDouble(reinterpret_cast<void*>(func), argJvalue, stackSize, dregSize);
    return result;
  }
};

inline jobject MRT_JNI_AddLocalReference(JNIEnv *env, jobject objAddr) {
  if (env == nullptr) {
    // it must be from static-binding native method,
    // which it's in libcore-static-binding-jni/etc/static-binding-list.txt list.
    // we needn't do encode, as native stub needn't do decode.
    return objAddr;
  }

  maple::IThread *self = maple::IThread::Current();
  return reinterpret_cast<jobject>(
      (self == nullptr) ? nullptr : self->JniAddObj2LocalRefTbl(env, (reinterpret_cast<address_t>(objAddr))));
}
} // namespace maplert

#endif // MAPLE_RUNTIME_CPP_HELPER_H
