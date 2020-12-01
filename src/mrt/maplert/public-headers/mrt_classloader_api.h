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
#ifndef MRT_CLASS_CLASSLOADER_API_H_
#define MRT_CLASS_CLASSLOADER_API_H_

#include "jni.h"
#include "mrt_api_common.h"
#ifdef __cplusplus
namespace maplert {
extern "C" {
#endif
// C interfaces
MRT_EXPORT bool MRT_IsClassInitialized(jclass klass);
MRT_EXPORT jobject MRT_GetBootClassLoader();
MRT_EXPORT jobject MRT_GetClassLoader(jclass klass);
MRT_EXPORT jclass MRT_GetClassByClassLoader(jobject classLoader, const std::string className);
MRT_EXPORT jclass MRT_GetClassByContextClass(jclass klass, const std::string className);
MRT_EXPORT jclass MRT_GetClassByContextObject(jobject obj, const std::string className);
MRT_EXPORT jclass MRT_GetNativeContexClass(); // Get the class loader by current native context
MRT_EXPORT jobject MRT_GetNativeContexClassLoader(); // Get the contextclass by current native context
MRT_EXPORT jclass MRT_GetClass(jclass klass, const std::string className);
MRT_EXPORT void MRT_RegisterDynamicClass(jobject classLoader, jclass klass);
MRT_EXPORT void MRT_UnregisterDynamicClass(jobject classLoader, jclass klass);
#ifdef __cplusplus
} // extern "C"
} // namespace maplert
#endif
#endif // MRT_CLASS_LOCATOR_MGR_H_
