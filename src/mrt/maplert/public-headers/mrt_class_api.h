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
#ifndef MAPLE_LIB_CORE_MPL_CLASS_H_
#define MAPLE_LIB_CORE_MPL_CLASS_H_

#include "mrt_api_common.h"
#include "jni.h"

#ifdef __cplusplus
namespace maplert {
extern "C" {
#endif

MRT_EXPORT char    *MRT_ReflectGetClassCharName(const jclass klass);
MRT_EXPORT jint     MRT_ReflectGetObjSize(const jclass klass);
MRT_EXPORT jboolean MRT_ReflectClassIsPrimitive(jclass klass);
MRT_EXPORT jstring  MRT_ReflectClassGetName(jclass klass);

MRT_EXPORT jclass   MRT_GetClassObject(void);
MRT_EXPORT jclass   MRT_GetClassClass(void);
MRT_EXPORT jclass   MRT_GetClassString(void);
MRT_EXPORT bool MRT_ClassIsSuperClassValid(jclass clazz);

MRT_EXPORT bool     MRT_ClassInitialized(jclass classInfo);
MRT_EXPORT void     MRT_InitProtectedMemoryForClinit();
MRT_EXPORT void     MRT_DumpClassClinit(std::ostream &os);
MRT_EXPORT int64_t  MRT_ClinitGetTotalTime();
MRT_EXPORT int64_t  MRT_ClinitGetTotalCount();
MRT_EXPORT void     MRT_ClinitResetStats();
MRT_EXPORT void     MRT_ClinitEnableCount(bool enable);
MRT_EXPORT void     MRT_BootstrapClinit(void);
MRT_EXPORT void     MRT_BootstrapWellKnown();

MRT_EXPORT void MRT_SaveProfile(const std::string &path, bool isSystemServer);
MRT_EXPORT void MRT_EnableMetaProfile();
MRT_EXPORT void MRT_DisableMetaProfile();
MRT_EXPORT void MRT_ClearMetaProfile();

// Returns the component size of the specified array class.
MRT_EXPORT size_t MRT_ReflectClassGetComponentSize(jclass klass);
#ifdef __cplusplus
} // extern "C"
} // namespace maplert
#endif
#endif //MAPLE_LIB_CORE_MPL_CLASS_H_
