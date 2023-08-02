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
#ifndef JAVA_MRT_ARRAY_H_
#define JAVA_MRT_ARRAY_H_

#include "jni.h"
#include "mrt_array_api.h"
#include "mclass.h"

namespace maplert {
void *MRT_JavaArrayToCArray(jarray javaArray);
void MRT_ObjectArrayCopy(address_t javaSrc, address_t javaDst, jint srcPos, jint dstPos, jint length,
                         bool check = false);
void ThrowArrayStoreException(const MObject &srcComponent, int index, const MClass &dstComponentType);
bool AssignableCheckingObjectCopy(const MClass &dstComponentType, MClass *&lastAssignableComponentType,
                                  const MObject *srcComponent);
} // namespace maplert

#endif // JAVA_MRT_ARRAY_H_
