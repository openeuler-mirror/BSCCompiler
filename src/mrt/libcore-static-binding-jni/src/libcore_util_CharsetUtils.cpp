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
#include "libcore_util_CharsetUtils.h"
#include <cstdio>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include "securec.h"
#include "mrt_string.h"
#include "mrt_array.h"
#include "chelper.h"
#include "cpphelper.h"
#include "mstring_inline.h"
#include "marray_inline.h"

#ifndef UNIFIED_MACROS_DEF
#define UNIFIED_MACROS_DEF
#include "unified.macros.def"
#endif
using namespace std;
using namespace maplert;

#ifdef __cplusplus
extern "C" {
#endif

void Java_libcore_util_CharsetUtils_asciiBytesToChars___3BII_3C(JNIEnv*,
    jclass, jbyteArray javaBytes, jint offset, jint length, jcharArray javaChars) {
  if (javaBytes == nullptr || javaChars == nullptr) {
    return;
  }
  MArray *srcArray = reinterpret_cast<MArray*>(javaBytes);
  MArray *dstArray = reinterpret_cast<MArray*>(javaChars);
  int32_t srcElementCount = static_cast<int32_t>(srcArray->GetLength());
  if (UNLIKELY(offset < 0 || length < 0 || offset + length > srcElementCount || offset > INT_MAX - length)) {
    LOG(FATAL) << "the index to access javaBytes is illegal" << maple::endl;
  }
  jbyte *src = reinterpret_cast<jbyte*>(srcArray->ConvertToCArray()) + offset;
  jchar *dst = reinterpret_cast<jchar*>(dstArray->ConvertToCArray());
  jchar *end = dst + length;

  while (dst < end) {
    jchar tmp = static_cast<jchar>(static_cast<unsigned char>(*src++) & 0xff);
    *dst++ = (tmp < 0x80) ? tmp : 0xfffd;
  }
}

void Java_libcore_util_CharsetUtils_isoLatin1BytesToChars___3BII_3C(JNIEnv*,
    jclass, jbyteArray javaBytes, jint offset, jint length, jcharArray javaChars) {
  if (javaBytes == nullptr || javaChars == nullptr) {
    return;
  }
  MArray *srcArray = reinterpret_cast<MArray*>(javaBytes);
  MArray *dstArray = reinterpret_cast<MArray*>(javaChars);
  int32_t srcElementCount = static_cast<int32_t>(srcArray->GetLength());
  if (UNLIKELY(offset < 0 || length < 0 || offset + length > srcElementCount || offset > INT_MAX - length)) {
    LOG(FATAL) << "the index to access javaBytes is illegal" << maple::endl;
  }
  jbyte *src = reinterpret_cast<jbyte*>(srcArray->ConvertToCArray()) + offset;
  jchar *dst = reinterpret_cast<jchar*>(dstArray->ConvertToCArray());
  jchar *end = dst + length;

  while (dst < end) {
    *dst++ = static_cast<jchar>(static_cast<unsigned char>(*src++) & 0xff);
  }
}

jbyteArray Java_libcore_util_CharsetUtils_toAsciiBytes__Ljava_lang_String_2II(JNIEnv *env,
    jclass, jstring javaString, jint offset, jint length) {
  ScopedObjectAccess soa;
  const jchar replacementByte = 0x7f;
  MString *stringObj = reinterpret_cast<MString*>(javaString);
  DCHECK(stringObj != nullptr);
  MArray *res = stringObj->IsCompress() ? MStringToBytes(*stringObj, offset, length, replacementByte, 1) :
                                          MStringToBytes(*stringObj, offset, length, replacementByte, 0);
  return reinterpret_cast<jbyteArray>(MRT_JNI_AddLocalReference(env, reinterpret_cast<jobject>(res)));
}

jbyteArray Java_libcore_util_CharsetUtils_toIsoLatin1Bytes__Ljava_lang_String_2II(JNIEnv *env,
    jclass, jstring javaString, jint offset, jint length) {
  ScopedObjectAccess soa;
  const jchar replacementByte = 0xff;
  MString *stringObj = reinterpret_cast<MString*>(javaString);
  DCHECK(stringObj != nullptr);
  MArray *res = stringObj->IsCompress() ? MStringToBytes(*stringObj, offset, length, replacementByte, 1) :
                                          MStringToBytes(*stringObj, offset, length, replacementByte, 0);
  return reinterpret_cast<jbyteArray>(MRT_JNI_AddLocalReference(env, reinterpret_cast<jobject>(res)));
}

jbyteArray Java_libcore_util_CharsetUtils_toUtf8Bytes__Ljava_lang_String_2II(JNIEnv *env,
    jclass, jstring javaString, jint offset, jint length) {
  ScopedObjectAccess soa;
  MString *stringObj = reinterpret_cast<MString*>(javaString);
  DCHECK(stringObj != nullptr);
  MArray *res = stringObj->IsCompress() ? MStringToMutf8(*stringObj, offset, length, 1) :
                                          MStringToMutf8(*stringObj, offset, length, 0);
  return reinterpret_cast<jbyteArray>(MRT_JNI_AddLocalReference(env, reinterpret_cast<jobject>(res)));
}

#ifdef __cplusplus
}
#endif
