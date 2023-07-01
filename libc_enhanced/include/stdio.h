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
#include_next <stdio.h>
#ifndef __STDIO_C_ENHANCED_H
// no stdio header guard if (defined __need_FILE or defined __need___FILE => !defined STDIO_H)
#ifdef _STDIO_H
#define __STDIO_C_ENHANCED_H
#ifdef C_ENHANCED
#include "c_enhanced.h"
#include <stdarg.h>

// Fix EOF not defined error
#ifndef EOF
# define EOF (-1)
#endif

CNTI(2, 1)
SAFE int snprintf(char *__restrict str, size_t, const char *__restrict, ...);

CNTI(2, 1)
SAFE int vsnprintf(char *__restrict str, size_t, const char *__restrict, va_list);

#ifdef __FILE_defined
CNTI(2, 1)
SAFE char *fgets(char *__restrict str, int, FILE *__restrict);
#endif

#endif // C_ENHANCED
#endif // _STDIO_H
#endif // __STDIO_C_ENHANCED_H