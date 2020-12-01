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
#ifndef MAPLE_LIB_CORE_MPL_STRING_H_
#define MAPLE_LIB_CORE_MPL_STRING_H_

#include <functional>
#include "mrt_api_common.h"
#include "jni.h"

#ifdef __cplusplus
namespace maplert {
extern "C" {
#endif
MRT_EXPORT jint MRT_StringGetStringLength(jstring jstr);
MRT_EXPORT void MRT_ScanLiteralPoolRoots(std::function<void(uintptr_t)> visitRoot);
MRT_EXPORT void MRT_ReleaseStringUTFChars(jstring jstr, const char *chars);
MRT_EXPORT void MRT_ReleaseStringChars(jstring jstr, const jchar *chars);
MRT_EXPORT jstring MRT_NewHeapJStr(const jchar *ca, jint len, bool isJNI = false);
MRT_EXPORT bool MRT_IsStrCompressed(const jstring jstr);
MRT_EXPORT char *MRT_GetStringContentsPtrRaw(jstring jstr);
MRT_EXPORT jchar *MRT_GetStringContentsPtrCopy(jstring jstr, jboolean *isCopy);
MRT_EXPORT size_t MRT_GetStringObjectSize(jstring jstr);
// only for MUTF-8 string JNI
MRT_EXPORT jstring MRT_NewStringMUTF(const char *inMutf, size_t inMutfLen, bool isJNI = false);
MRT_EXPORT jsize MRT_GetStringMUTFLength(jstring jstr);
MRT_EXPORT char *MRT_GetStringMUTFChars(jstring jstr, jboolean *isCopy);
MRT_EXPORT void MRT_GetStringMUTFRegion(jstring jstr, jsize start, jsize length, char *buf);

#ifdef __cplusplus
} // extern "C"
} // namespace maplert
#endif

#endif // MAPLE_LIB_CORE_MPL_STRING_H_
