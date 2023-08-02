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
#ifndef MAPLE_LIB_CORE_MPL_PRIMITIVE_H_
#define MAPLE_LIB_CORE_MPL_PRIMITIVE_H_

#include "mrt_api_common.h"

#ifdef __cplusplus
namespace maplert {
extern "C" {
#endif

//  PrimitiveClass
MRT_EXPORT jclass MRT_GetPrimitiveClassJboolean(void);
MRT_EXPORT jclass MRT_GetPrimitiveClassJbyte(void);
MRT_EXPORT jclass MRT_GetPrimitiveClassJchar(void);
MRT_EXPORT jclass MRT_GetPrimitiveClassJdouble(void);
MRT_EXPORT jclass MRT_GetPrimitiveClassJfloat(void);
MRT_EXPORT jclass MRT_GetPrimitiveClassJint(void);
MRT_EXPORT jclass MRT_GetPrimitiveClassJlong(void);
MRT_EXPORT jclass MRT_GetPrimitiveClassJshort(void);
MRT_EXPORT jclass MRT_GetPrimitiveClassVoid(void);

#ifdef __cplusplus
} // extern "C"
} // namespace maplert
#endif

#endif //MAPLE_LIB_CORE_MPL_PRIMITIVE_H_
