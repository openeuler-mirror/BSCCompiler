/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include_next <stdlib.h>
#ifndef __STDLIB_C_ENHANCED_H
// no stdlib header guard if (defined __need_malloc_and_calloc => !defined _STDLIB_H)
#ifdef _STDLIB_H
#define __STDLIB_C_ENHANCED_H
#ifdef C_ENHANCED
#include "c_enhanced.h"

BRCNTI(1)
SAFE void *malloc(size_t);

SAFE void *calloc(size_t n_elements, size_t element_size) BRCNT(n_elements * element_size);

BRCNTI(2)
SAFE void *realloc(void *, size_t);

#endif // C_ENHANCED
#endif // _STDLIB_H
#endif  // __STDLIB_C_ENHANCED_H
