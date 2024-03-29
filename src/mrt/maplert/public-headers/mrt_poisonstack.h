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
#ifndef MAPLEALL_MAPLERT_JAVA_ANDROID_MRT_INCLUDE_MRT_POISIONSTACK_H_
#define MAPLEALL_MAPLERT_JAVA_ANDROID_MRT_INCLUDE_MRT_POISIONSTACK_H_

#include <cstdint>
#include "mrt_api_common.h"
#ifdef __cplusplus
namespace maplert {
extern "C" {
#endif // __cplusplus

MRT_EXPORT void MRT_InitPoisonStack(uintptr_t framePtr);

#ifdef __cplusplus
} // extern "C"
} // namespace maplert
#endif // __cplusplus
#endif
