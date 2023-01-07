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
#ifndef __STRING_C_ENHANCED_H
#define __STRING_C_ENHANCED_H
#include_next <string.h>
#include "c_enhanced.h"
BCNTI(3, 1, 2)
SAFE void *memcpy(void *__restrict __dest, const void *__restrict __src, size_t count);

BCNTI(3, 1, 2)
SAFE void *memmove(void *__dest, const void *__src, size_t count);

BCNTI(3, 1)
SAFE void *memset(void *__s, int __c, size_t count);

BCNTI(3, 1, 2)
SAFE int memcmp (const void *__s1, const void *__s2, size_t __n);

#endif // __STRING_C_ENHANCED_H
