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
#include "native_binding_utils.h"
#include "base/logging.h"
#include "thread_api.h"
#include "mrt_class_api.h"
#include "mrt_string_api.h"
#include "mrt_array_api.h"
#include "chelper.h"
#include "libs.h"
#include "mrt_well_known.h"

namespace maplert {
#ifdef __cplusplus
extern "C" {
#endif

// The set of "C" functions exported to compiler for native function calls
//
// used in compiler-generated "weak" static-bound native function.
// on qemu/arm64-server platform (without all the native-libraries available), it will report an warning.
// but on android, it will throw an UnsatisfiedLinkError.
//
// return a "dummy" object in case the calling native functions needs to return a jobject
jobject MCC_CannotFindNativeMethod(const char *signature) {
  VLOG(jni) << signature << "is weak symbol and throw java/lang/UnsatisfiedLinkError!" << maple::endl;
#ifdef __ANDROID__
  JNIEnv *env = maple::IThread::Current()->GetJniEnv();
  env->ThrowNew(reinterpret_cast<jclass>(WellKnown::GetMClassUnsatisfiedLinkError()), signature);
  return (jobject) nullptr;
#else
  return reinterpret_cast<jobject>(MRT_GetClassClass());
#endif //__ANDROID__
}

#ifndef __ANDROID__
static uint64_t dummyJstr[3] = { 0 };  // field size 3
#endif // __ANDROID__

// a version returns a literal string.
jstring MCC_CannotFindNativeMethod_S(const char *signature) {
  VLOG(jni) << signature << "is weak symbol and throw java/lang/UnsatisfiedLinkError!" << maple::endl;
#ifdef __ANDROID__
  JNIEnv *env = maple::IThread::Current()->GetJniEnv();
  env->ThrowNew(reinterpret_cast<jclass>(WellKnown::GetMClassUnsatisfiedLinkError()), signature);
  return (jstring) nullptr;
#else
  if (dummyJstr[0] == 0) {
    dummyJstr[0] = reinterpret_cast<uint64_t>(MRT_GetClassString());
  }
  return reinterpret_cast<jstring>(dummyJstr);
#endif //__ANDROID__
}

#ifndef __ANDROID__
static uint64_t dummyJarray[3] = { 0 }; // field size 3
#endif //__ANDROID__

// a version returns an empty array.
jobject MCC_CannotFindNativeMethod_A(const char *signature) {
  VLOG(jni) << signature << "is weak symbol and throw java/lang/UnsatisfiedLinkError!" << maple::endl;
#ifdef __ANDROID__
  JNIEnv *env = maple::IThread::Current()->GetJniEnv();
  env->ThrowNew(reinterpret_cast<jclass>(WellKnown::GetMClassUnsatisfiedLinkError()), signature);
  return (jobject) nullptr;
#else
  if (dummyJarray[0] == 0) {
    dummyJarray[0] = reinterpret_cast<uint64_t>(MRT_GetPrimitiveArrayClassJint());
  }
  return reinterpret_cast<jobject>(dummyJarray);
#endif //__ANDROID__
}

void *MCC_DummyNativeMethodPtr() {
#if defined(__ANDROID__)
  VLOG(jni) << "[NOTE] : MCC_DummyNativeMethodPtr is called for encountering null native method!" << maple::endl;
#endif //__ANDROID__
  return reinterpret_cast<void*>(&MCC_DummyNativeMethodPtr);
}

void *MCC_FindNativeMethodPtr(uintptr_t **regFuncTabAddr) {
  VLOG(jni) << "[NOTE] : MCC_FindNativeMethodPtr is invoked by function address at " <<
      reinterpret_cast<uintptr_t>(regFuncTabAddr)  << maple::endl;
  void *nativeMethodPtr = reinterpret_cast<void*>(&MCC_DummyNativeMethodPtr);
#if defined(__ANDROID__)
  maple::IThread *tSelf = maple::IThread::Current();
  JNIEnv *env = tSelf->GetJniEnv();
  nativeMethodPtr = tSelf->FindNativeMethodPtr(env, regFuncTabAddr, true);
  if (nativeMethodPtr == nullptr) {
    nativeMethodPtr = reinterpret_cast<void*>(&MCC_DummyNativeMethodPtr);
  }
#else
  // Currently we're reusing native registration functions, which will
  // trigger exceptions for openjdk core library. disable exception during registration.
  maple::IThread *tSelf = maple::IThread::Current();
  JNIEnv *env = tSelf->GetJniEnv();
  nativeMethodPtr = tSelf->FindNativeMethodPtr(env, regFuncTabAddr, false);
  if (nativeMethodPtr == nullptr) {
    nativeMethodPtr = reinterpret_cast<void*>(&MCC_DummyNativeMethodPtr);
  }
#endif //__ANDROID__
  return (jobject) nativeMethodPtr;
}

void *MCC_FindNativeMethodPtrWithoutException(uintptr_t **regFuncTabAddr) {
  VLOG(jni) << "[NOTE] : MCC_FindNativeMethodPtrWithoutException is invoked by function address at " <<
      reinterpret_cast<uintptr_t>(regFuncTabAddr)  << maple::endl;
  void *nativeMethodPtr = reinterpret_cast<void*>(&MCC_DummyNativeMethodPtr);
  maple::IThread *tSelf = maple::IThread::Current();
  JNIEnv *env = tSelf->GetJniEnv();
  nativeMethodPtr = tSelf->FindNativeMethodPtr(env, regFuncTabAddr, false);
  if (nativeMethodPtr == nullptr) {
    nativeMethodPtr = reinterpret_cast<void*>(&MCC_DummyNativeMethodPtr);
  }
  return static_cast<jobject>(nativeMethodPtr);
}


#ifdef __cplusplus
}
#endif
} // maplert
