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
#include "java_lang_StringFactory.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <iostream>
#include "mrt_string.h"
#include "mstring_inline.h"
#include "exception/mrt_exception.h"
#include "libs.h"
using namespace std;
using namespace maplert;

#ifdef __cplusplus
extern "C" {
#endif

MRT_EXPORT jstring Native_java_lang_StringFactory_newStringFromString__Ljava_lang_String_2(jstring toCopy) {
  if (UNLIKELY(toCopy == nullptr)) {
    MRT_ThrowNewExceptionUnw("java/lang/NullPointerException",
                             "Attempt to invoke a virtual method on a null object reference");
    return nullptr;
  }
  MString *stringObj = reinterpret_cast<MString*>(toCopy);
  jstring newStr = reinterpret_cast<jstring>(StringNewStringFromString(*stringObj));
  MRT_MemoryBarrier();
  return newStr;
}

jstring Java_java_lang_StringFactory_newStringFromString__Ljava_lang_String_2(JNIEnv *env, jclass, jstring toCopy) {
  ScopedObjectAccess soa;
  if (toCopy == nullptr) {
    MRT_ThrowNewException("java/lang/NullPointerException",
                          "Attempt to invoke a virtual method on a null object reference");
    return nullptr;
  }
  MString *stringObj = reinterpret_cast<MString*>(toCopy);
  jstring newStr = reinterpret_cast<jstring>(StringNewStringFromString(*stringObj));
  MRT_MemoryBarrier();
  return reinterpret_cast<jstring>(MRT_JNI_AddLocalReference(env, newStr));
}

#ifdef __OPENJDK__
MRT_EXPORT jstring Native_java_lang_StringFactory_nativeStringFromChars___3CII(
    jarray ja,
    jint i,
    jint i2) {
  if (ja == nullptr) {
    jstring emptyString = reinterpret_cast<jstring>(MString::NewEmptyStringObject());
    MRT_MemoryBarrier();
    return emptyString;
  }
  jstring res = reinterpret_cast<jstring>(
      StringNewStringFromCharArray(i, static_cast<uint32_t>(i2), *reinterpret_cast<MArray*>(ja)));
  MRT_MemoryBarrier();
  return res;
}

jstring Java_java_lang_StringFactory_nativeStringFromChars___3CII(
    JNIEnv *env,
    jclass,
    jarray ja,
    jint i,
    jint i2) {
  ScopedObjectAccess soa;
  jstring res = Native_java_lang_StringFactory_nativeStringFromChars___3CII(ja, i, i2);
  return reinterpret_cast<jstring>(MRT_JNI_AddLocalReference(env, res));
}
#else // libcore
// Ljava_2Flang_2FStringFactory_3B_7CnewStringFromChars_7C_28IIAC_29Ljava_2Flang_2FString_3B
MRT_EXPORT jstring Native_java_lang_StringFactory_newStringFromChars__II_3C(
    jint i,
    jint i2,
    jarray ja) {
  if (UNLIKELY(ja == nullptr)) {
    MRT_ThrowNewExceptionUnw("java/lang/NullPointerException",
                             "Attempt to invoke a virtual method on a null object reference");
    return nullptr;
  }

  jstring res = reinterpret_cast<jstring>(
      StringNewStringFromCharArray(i, static_cast<uint32_t>(i2), *reinterpret_cast<MArray*>(ja)));
  MRT_MemoryBarrier();
  return res;
}

jstring Java_java_lang_StringFactory_newStringFromChars__II_3C(
    JNIEnv *env,
    jclass,
    jint i,
    jint i2,
    jarray ja) {
  ScopedObjectAccess soa;
  jstring res = Native_java_lang_StringFactory_newStringFromChars__II_3C(i, i2, ja);
  return reinterpret_cast<jstring>(MRT_JNI_AddLocalReference(env, res));
}
#endif // __OPENJDK__

#ifdef __OPENJDK__
MRT_EXPORT jstring Native_java_lang_StringFactory_nativeStringFromBytes___3BIII(
    jbyteArray javaData, jint high, jint offset, jint byteCount) {
  if (javaData == nullptr && high == 0 && offset == 0 && byteCount == 0) {
    jstring emptyString = reinterpret_cast<jstring>(MString::NewEmptyStringObject());
    return emptyString;
  }

  MArray *arrayObj = reinterpret_cast<MArray*>(javaData);
  DCHECK(arrayObj != nullptr) ;
  int32_t dataSize = arrayObj->GetLength();
  if ((offset < 0) || (byteCount < 0) || (offset > (INT_MAX - byteCount)) || (byteCount > (dataSize - offset))) {
    MRT_ThrowNewExceptionUnw("java/lang/StringIndexOutOfBoundsException");
    return nullptr;
  }
  jstring res = reinterpret_cast<jstring>(
      StringNewStringFromByteArray(*arrayObj, high, offset, static_cast<uint32_t>(byteCount)));
  MRT_MemoryBarrier();
  return res;
}

jstring Java_java_lang_StringFactory_nativeStringFromBytes___3BIII(
    JNIEnv *env, jclass, jbyteArray javaData, jint high, jint offset, jint byteCount) {
  ScopedObjectAccess soa;
  if (javaData == nullptr && high == 0 && offset == 0 && byteCount == 0) {
    jstring emptyString = reinterpret_cast<jstring>(MString::NewEmptyStringObject());
    return reinterpret_cast<jstring>(MRT_JNI_AddLocalReference(env, emptyString));
  }

  MArray *arrayObj = reinterpret_cast<MArray*>(javaData);
  DCHECK(arrayObj != nullptr);
  int32_t dataSize = arrayObj->GetLength();
  if ((offset < 0) || (byteCount < 0) || (offset > (INT_MAX - byteCount)) || (byteCount > (dataSize - offset))) {
    MRT_ThrowNewException("java/lang/StringIndexOutOfBoundsException", nullptr);
    return nullptr;
  }
  jstring res = reinterpret_cast<jstring>(
      StringNewStringFromByteArray(*arrayObj, high, offset, static_cast<uint32_t>(byteCount)));
  MRT_MemoryBarrier();
  return reinterpret_cast<jstring>(MRT_JNI_AddLocalReference(env, res));
}
#endif // __OPENJDK__

MRT_EXPORT jstring Native_java_lang_StringFactory_newStringFromBytes___3BIII(
    jbyteArray javaData, jint high, jint offset, jint byteCount) {
  if (UNLIKELY(javaData == nullptr)) {
    MRT_ThrowNewExceptionUnw("java/lang/NullPointerException",
                             "Attempt to invoke a virtual method on a null object reference");
    return nullptr;
  }

  MArray *arrayObj = reinterpret_cast<MArray*>(javaData);
  DCHECK(arrayObj != nullptr);
  int32_t dataSize = static_cast<int32_t>(arrayObj->GetLength());
  if ((offset < 0) || (byteCount < 0) || (offset > (INT_MAX - byteCount)) || (byteCount > (dataSize - offset))) {
    MRT_ThrowNewExceptionUnw("java/lang/StringIndexOutOfBoundsException");
    return nullptr;
  }
  jstring res = reinterpret_cast<jstring>(
      StringNewStringFromByteArray(*arrayObj, high, offset, static_cast<uint32_t>(byteCount)));
  MRT_MemoryBarrier();
  return res;
}

jstring Java_java_lang_StringFactory_newStringFromBytes___3BIII(
    JNIEnv *env, jclass, jbyteArray javaData, jint high, jint offset, jint byteCount) {
  ScopedObjectAccess soa;
  if (javaData == nullptr) {
    MRT_ThrowNewException("java/lang/NullPointerException",
                          "Attempt to invoke a virtual method on a null object reference");
    return nullptr;
  }

  MArray *arrayObj = reinterpret_cast<MArray*>(javaData);
  DCHECK(arrayObj != nullptr);
  int32_t dataSize = static_cast<int32_t>(arrayObj->GetLength());
  if ((offset < 0) || (byteCount < 0) || (offset > (INT_MAX - byteCount)) || (byteCount > (dataSize - offset))) {
    MRT_ThrowNewException("java/lang/StringIndexOutOfBoundsException", nullptr);
    return nullptr;
  }
  jstring res = reinterpret_cast<jstring>(
      StringNewStringFromByteArray(*arrayObj, high, offset, static_cast<uint32_t>(byteCount)));
  MRT_MemoryBarrier();
  return reinterpret_cast<jstring>(MRT_JNI_AddLocalReference(env, res));
}

#ifdef __OPENJDK__
jstring Java_java_lang_StringFactory_newStringFromCodePoints___3III(
    JNIEnv *env,
    jclass,
    jarray javaData,
    jint offset,
    jint count) {
  ScopedObjectAccess soa;
  MArray *arrayObj = reinterpret_cast<MArray*>(javaData);
  DCHECK(arrayObj != nullptr);
  int32_t dataSize = arrayObj->GetLength();
  if ((offset < 0) || (count < 0) || ((offset + count) < 0) || (count > (dataSize - offset))) {
    MRT_ThrowNewException("java/lang/StringIndexOutOfBoundsException");
    return nullptr;
  }
  if (count == 0 && offset <= dataSize) {
    jstring emptyString = reinterpret_cast<jstring>(MString::NewEmptyStringObject());
    MRT_MemoryBarrier();
    return emptyString;
  }
  jstring res = reinterpret_cast<jstring>(StringNewStringFromCodePoints(*arrayObj, offset, count));
  MRT_MemoryBarrier();
  return reinterpret_cast<jstring>(MRT_JNI_AddLocalReference(env, res));
}
#endif // __OPENJDK__

#ifdef __cplusplus
}
#endif
