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
#ifndef __STDLIB_C_ENHANCED_H
#define __STDLIB_C_ENHANCED_H
#include_next <stdlib.h>
#include "c_enhanced.h"

BRCNTI(1)
SAFE void *malloc(size_t);

SAFE void *calloc(size_t n_elements, size_t element_size) BRCNT(n_elements * element_size);

BRCNTI(2)
SAFE void *realloc(void *, size_t);

#endif  // __STDLIB_C_ENHANCED_H
