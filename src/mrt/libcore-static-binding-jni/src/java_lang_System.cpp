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
#include "java_lang_System.h"
#include "mrt_array.h"
#include "exception/mrt_exception.h"
#include "libs.h"
using namespace maplert;

#ifdef __cplusplus
extern "C" {
#endif

static void ThrowArrayStoreExceptionSrcNotArray(const MObject &javaObjSrc) {
  std::ostringstream msg;
  std::string srcBinaryName;
  javaObjSrc.GetClass()->GetBinaryName(srcBinaryName);
  msg << "source of type " << srcBinaryName << " is not an array";
  MRT_ThrowNewExceptionUnw("java/lang/ArrayStoreException", msg.str().c_str());
}

static void ThrowArrayStoreExceptionDstNotArray(const MObject &javaObjDst) {
  std::ostringstream msg;
  std::string dstName;
  javaObjDst.GetClass()->GetBinaryName(dstName);
  msg << "destination of type " << dstName << " is not an array";
  MRT_ThrowNewExceptionUnw("java/lang/ArrayStoreException", msg.str().c_str());
}

static void ThrowArrayIndexOutOfBoundsException(const int32_t szLength, const jint srcPos,
                                                const int32_t dstLength, const jint dstPos, const jint length) {
    string s = "src.length=" + std::to_string(szLength) + " srcPos=" + std::to_string(srcPos) + " dst.length=" +
        std::to_string(dstLength) + " dstPos=" + std::to_string(dstPos) + " length=" + std::to_string(length);
    MRT_ThrowNewExceptionUnw("java/lang/ArrayIndexOutOfBoundsException", s.c_str());
}

static void ThrowArrayStoreExceptionIncompatibleTypes(const MClass &srcComponentType, const MClass &dstComponentType) {
  std::ostringstream msg;
  std::string srcBinaryName;
  std::string dstBinaryName;
  srcComponentType.GetBinaryName(srcBinaryName);
  dstComponentType.GetBinaryName(dstBinaryName);
  msg << "Incompatible types: src=" << srcBinaryName << "[], dst=" << dstBinaryName << "[]";
  MRT_ThrowNewExceptionUnw("java/lang/ArrayStoreException", msg.str().c_str());
}

static void AssignableObjectCopy(const MArray &javaSrc, jint srcPos, MArray &javaDst, jint dstPos, jint length,
                                 bool check = false) {
  MRT_ObjectArrayCopy(reinterpret_cast<address_t>(&javaSrc), reinterpret_cast<address_t>(&javaDst),
                      srcPos, dstPos, length, check);
}

static void PrimitiveCopy(const MArray &javaSrc, jint srcPos, MArray &javaDst, jint dstPos, jint length) {
  uint8_t *src = javaSrc.ConvertToCArray();
  uint8_t *dst = javaDst.ConvertToCArray();
  size_t szElem = javaSrc.GetElementSize();

  uint8_t *srcStart = src + szElem * static_cast<size_t>(srcPos);
  uint8_t *dstStart = dst + szElem * static_cast<size_t>(dstPos);

  size_t arrayLen = szElem * static_cast<size_t>(length);
  // primitive array copy tend to copy to different arrays, with relative large length
  if (src != dst) {
    if (memcpy_s(dstStart, arrayLen, srcStart, arrayLen) != EOK) {
      LOG(ERROR) << "In PrimitiveCopy Function memcpy_s() return wrong" << maple::endl;
    }
  } else {
    if (memmove_s(dstStart, arrayLen, srcStart, arrayLen) != EOK) {
      LOG(ERROR) << "Function memmove_s() failed." << maple::endl;
    }
  }
}

static void NativeSystemArraycopy(const MArray &srcArray, jint srcPos, MArray &dstArray, jint dstPos, jint length) {
  const MObject *javaObjSrc = static_cast<const MObject*>(&srcArray);
  MObject *javaObjDst = static_cast<MObject*>(&dstArray);
  // Make sure source and destination are both arrays.
  if (UNLIKELY(!javaObjSrc->IsArray())) {
    ThrowArrayStoreExceptionSrcNotArray(*javaObjSrc);
    return;
  }
  if (UNLIKELY(!javaObjDst->IsArray())) {
    ThrowArrayStoreExceptionDstNotArray(*javaObjDst);
    return;
  }

  // Bounds checking.
  int32_t szLength = static_cast<int32_t>(srcArray.GetLength());
  int32_t dstLength = static_cast<int32_t>(dstArray.GetLength());
  if (UNLIKELY(srcPos < 0) || UNLIKELY(dstPos < 0) || UNLIKELY(length < 0) ||
      UNLIKELY(srcPos > szLength - length) || UNLIKELY(dstPos > dstLength - length)) {
    ThrowArrayIndexOutOfBoundsException(szLength, srcPos, dstLength, dstPos, length);
    return;
  }
  // saw a lot of arraycopy with 0 length so we add a fast path here.
  if (length == 0) {
    return;
  }
  // same array and offset, skip copy
  if (&srcArray == &dstArray && srcPos == dstPos) {
    return;
  }

  // check two array have same element class. Otherwise ArrayStoreException
  MClass *dstComponentType = dstArray.GetClass()->GetComponentClass();
  MClass *srcComponentType = srcArray.GetClass()->GetComponentClass();

  if (LIKELY((dstComponentType == srcComponentType))) {
    srcArray.IsObjectArray() ? AssignableObjectCopy(srcArray, srcPos, dstArray, dstPos, length, false) :
                               PrimitiveCopy(srcArray, srcPos, dstArray, dstPos, length);
    return;
  }

  if (UNLIKELY(dstComponentType->IsPrimitiveClass() || srcComponentType->IsPrimitiveClass())) {
    ThrowArrayStoreExceptionIncompatibleTypes(*srcComponentType, *dstComponentType);
    return;
  }

  if (LIKELY(dstComponentType == WellKnown::GetMClassObject()) ||
      dstComponentType->IsAssignableFrom(*srcComponentType)) {
    AssignableObjectCopy(srcArray, srcPos, dstArray, dstPos, length, false);
    return;
  }

  AssignableObjectCopy(srcArray, srcPos, dstArray, dstPos, length, true);
}

MRT_EXPORT void Native_java_lang_System_arraycopy__Ljava_lang_Object_2ILjava_lang_Object_2II(
    jobject javaSrc,  // source array
    jint srcPos,    // start idx
    jobject javaDst,  // dst array
    jint dstPos,    // start idx
    jint length) {   // nr. array elements to copy
  if (UNLIKELY(javaSrc == nullptr)) {
    MRT_ThrowNewExceptionUnw("java/lang/NullPointerException", "src == null");
    return;
  }
  if (UNLIKELY(javaDst == nullptr)) {
    MRT_ThrowNewExceptionUnw("java/lang/NullPointerException", "dst == null");
    return;
  }
  NativeSystemArraycopy(*reinterpret_cast<MArray*>(javaSrc), srcPos,
                        *reinterpret_cast<MArray*>(javaDst), dstPos, length);
}

void Java_java_lang_System_arraycopy__Ljava_lang_Object_2ILjava_lang_Object_2II(
    JNIEnv*,
    jclass,
    jobject javaSrc,  // source array
    jint srcPos,    // start idx
    jobject javaDst,  // dst array
    jint dstPos,    // start idx
    jint length) {  // nr. array elements to copy
  maplert::ScopedObjectAccess soa;
  if (UNLIKELY(javaSrc == nullptr)) {
    MRT_ThrowNewExceptionUnw("java/lang/NullPointerException", "src == null");
    return;
  }
  if (UNLIKELY(javaDst == nullptr)) {
    MRT_ThrowNewExceptionUnw("java/lang/NullPointerException", "dst == null");
    return;
  }
  NativeSystemArraycopy(*reinterpret_cast<MArray*>(javaSrc), srcPos,
                        *reinterpret_cast<MArray*>(javaDst), dstPos, length);
}

void SystemArraycopyTUnchecked(const jobject javaSrc, jint srcPos, jobject javaDst, jint dstPos, jint count) {
  if (count == 0) {
    return;
  }
  if (UNLIKELY(count < 0)) {
    string s = "length=" + std::to_string(count);
    MRT_ThrowNewExceptionRet("java/lang/ArrayIndexOutOfBoundsException", s.c_str());
    return;
  }
  if (javaSrc == javaDst && srcPos == dstPos) {
    return;
  }
  PrimitiveCopy(*reinterpret_cast<MArray*>(javaSrc), srcPos, *reinterpret_cast<MArray*>(javaDst), dstPos, count);
}


MRT_EXPORT void Native_java_lang_System_arraycopyCharUnchecked___3CI_3CII(jobject javaSrc, jint srcPos,
                                                                          jobject javaDst, jint dstPos, jint count) {
  SystemArraycopyTUnchecked(javaSrc, srcPos, javaDst, dstPos, count);
}

MRT_EXPORT void Native_java_lang_System_arraycopyByteUnchecked___3BI_3BII(jobject javaSrc, jint srcPos,
                                                                          jobject javaDst, jint dstPos, jint count) {
  SystemArraycopyTUnchecked(javaSrc, srcPos, javaDst, dstPos, count);
}

MRT_EXPORT void Native_java_lang_System_arraycopyShortUnchecked___3SI_3SII(jobject javaSrc, jint srcPos,
                                                                           jobject javaDst, jint dstPos, jint count) {
  SystemArraycopyTUnchecked(javaSrc, srcPos, javaDst, dstPos, count);
}

MRT_EXPORT void Native_java_lang_System_arraycopyIntUnchecked___3II_3III(jobject javaSrc, jint srcPos,
                                                                         jobject javaDst, jint dstPos, jint count) {
  SystemArraycopyTUnchecked(javaSrc, srcPos, javaDst, dstPos, count);
}

MRT_EXPORT void Native_java_lang_System_arraycopyLongUnchecked___3JI_3JII(jobject javaSrc, jint srcPos,
                                                                          jobject javaDst, jint dstPos, jint count) {
  SystemArraycopyTUnchecked(javaSrc, srcPos, javaDst, dstPos, count);
}

MRT_EXPORT void Native_java_lang_System_arraycopyFloatUnchecked___3FI_3FII(jobject javaSrc, jint srcPos,
                                                                           jobject javaDst, jint dstPos, jint count) {
  SystemArraycopyTUnchecked(javaSrc, srcPos, javaDst, dstPos, count);
}

MRT_EXPORT void Native_java_lang_System_arraycopyDoubleUnchecked___3DI_3DII(jobject javaSrc, jint srcPos,
                                                                            jobject javaDst, jint dstPos, jint count) {
  SystemArraycopyTUnchecked(javaSrc, srcPos, javaDst, dstPos, count);
}

MRT_EXPORT void Native_java_lang_System_arraycopyBooleanUnchecked___3ZI_3ZII(jobject javaSrc, jint srcPos,
                                                                             jobject javaDst, jint dstPos, jint count) {
  SystemArraycopyTUnchecked(javaSrc, srcPos, javaDst, dstPos, count);
}

// 33 Ljava/lang/System;|arraycopyCharUnchecked|([CI[CII)V
void Java_java_lang_System_arraycopyCharUnchecked___3CI_3CII(JNIEnv*, jclass, jobject javaSrc, jint srcPos,
                                                             jobject javaDst, jint dstPos, jint count) {
  maplert::ScopedObjectAccess soa;
  SystemArraycopyTUnchecked(javaSrc, srcPos, javaDst, dstPos, count);
}

void Java_java_lang_System_arraycopyByteUnchecked___3BI_3BII(JNIEnv*, jclass, jobject javaSrc, jint srcPos,
                                                             jobject javaDst, jint dstPos, jint count) {
  maplert::ScopedObjectAccess soa;
  SystemArraycopyTUnchecked(javaSrc, srcPos, javaDst, dstPos, count);
}

void Java_java_lang_System_arraycopyShortUnchecked___3SI_3SII(JNIEnv*, jclass, jobject javaSrc, jint srcPos,
                                                              jobject javaDst, jint dstPos, jint count) {
  maplert::ScopedObjectAccess soa;
  SystemArraycopyTUnchecked(javaSrc, srcPos, javaDst, dstPos, count);
}

void Java_java_lang_System_arraycopyIntUnchecked___3II_3III(JNIEnv*, jclass, jobject javaSrc, jint srcPos,
                                                            jobject javaDst, jint dstPos, jint count) {
  maplert::ScopedObjectAccess soa;
  SystemArraycopyTUnchecked(javaSrc, srcPos, javaDst, dstPos, count);
}

void Java_java_lang_System_arraycopyLongUnchecked___3JI_3JII(JNIEnv*, jclass, jobject javaSrc, jint srcPos,
                                                             jobject javaDst, jint dstPos, jint count) {
  maplert::ScopedObjectAccess soa;
  SystemArraycopyTUnchecked(javaSrc, srcPos, javaDst, dstPos, count);
}

void Java_java_lang_System_arraycopyFloatUnchecked___3FI_3FII(JNIEnv*, jclass, jobject javaSrc, jint srcPos,
                                                              jobject javaDst, jint dstPos, jint count) {
  maplert::ScopedObjectAccess soa;
  SystemArraycopyTUnchecked(javaSrc, srcPos, javaDst, dstPos, count);
}

void Java_java_lang_System_arraycopyDoubleUnchecked___3DI_3DII(JNIEnv*, jclass, jobject javaSrc, jint srcPos,
                                                               jobject javaDst, jint dstPos, jint count) {
  maplert::ScopedObjectAccess soa;
  SystemArraycopyTUnchecked(javaSrc, srcPos, javaDst, dstPos, count);
}

void Java_java_lang_System_arraycopyBooleanUnchecked___3ZI_3ZII(JNIEnv*, jclass, jobject javaSrc, jint srcPos,
                                                                jobject javaDst, jint dstPos, jint count) {
  maplert::ScopedObjectAccess soa;
  SystemArraycopyTUnchecked(javaSrc, srcPos, javaDst, dstPos, count);
}

#ifdef __cplusplus
}
#endif
