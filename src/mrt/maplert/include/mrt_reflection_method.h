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
#ifndef MRT_REFLECTION_METHOD_H
#define MRT_REFLECTION_METHOD_H
#include "mrt_annotation.h"
namespace maplert {
// declaring method for use in method reflection
MString *ReflectMethodGetName(const MMethod &methodObj);
MObject *ReflectMethodGetReturnType(const MMethod &methodObj);
MObject *ReflectMethodInvoke(const MMethod &methodObj, MObject *obj, const MArray *arg, uint8_t frames);
MObject *ReflectInvokeJavaMethodFromArrayArgsJobject(MObject*, const MMethod&, const MArray*, uint8_t);
void ReflectInvokeJavaMethodFromArrayArgsVoid(MObject *obj, const MMethod &method, const MArray *arg, uint8_t frames);
}  // namespace maplert
#endif  // MRT_REFLECTION_METHOD_H
