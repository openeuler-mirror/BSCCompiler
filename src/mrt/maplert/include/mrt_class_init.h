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
#ifndef JAVA_MRT_CLASS_INIT_H_
#define JAVA_MRT_CLASS_INIT_H_

#include <iostream>
#include "mrt_class_api.h"  // exported-api declaration.
#include "mrt_object.h"
#include "cinterface.h"

namespace maplert {
#ifdef __cplusplus
extern "C" {
#endif

ClassInitState MRT_TryInitClass(const MClass &classInfo, bool recursive = true);
ClassInitState MRT_TryInitClassOnDemand(const MClass &classInfo);
bool MRT_InitClassIfNeeded(const MClass &classInfo);
#ifdef __cplusplus
}
#endif
}
#endif // JAVA_MRT_CLASS_INIT_H_

