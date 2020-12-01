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
#ifndef MRT_PROFILE_H_
#define MRT_PROFILE_H_
#include "jni.h"
#include "mclass.h"
#include "methodmeta.h"
#include "fieldmeta.h"
namespace maplert {
class FieldMeta;
class MethodMetaBase;
void InsertReflectionString(const char *kHotString);
void InsertClassMetadata(const MClass &klass);
void InsertMethodMetadata(const MethodMetaBase *kMethod);
void InsertMethodMetadata(const MClass &cls);
void InsertFieldMetadata(FieldMeta *fieldMeta);
void InsertFieldMetadata(const MClass &cls);
void InsertMethodSignature(const MethodMeta &method);

#define __MRT_Profile_CString(hotString)        if (VLOG_IS_ON(profiler))  InsertReflectionString(hotString);
#define __MRT_Profile_ClassMeta(klass)          if (VLOG_IS_ON(profiler))  InsertClassMetadata(klass);
#define __MRT_Profile_MethodMeta(methodOrCls) if (VLOG_IS_ON(profiler))  InsertMethodMetadata(methodOrCls);
#define __MRT_Profile_FieldMeta(fieldOrCls)   if (VLOG_IS_ON(profiler))  InsertFieldMetadata(fieldOrCls);
#define __MRT_Profile_MethodParameterTypes(method) if (VLOG_IS_ON(profiler))  InsertMethodSignature(method);
} // namespace maplert

#endif // MRT_PROFILE_H_
