/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 */
// CHECK: [[# STDLIB_FILENUM:]] "{{.*}}/usr/include/stdlib.h"
#define __GLIBC_INTERNAL_STARTING_HEADER_IMPLEMENTATION
#include <bits/libc-header-start.h>

#define __need_FILE
#include <stdio.h>
#ifdef __STDIO_C_ENHANCED_H
#error "Should not define __STDIO_C_ENHANCED_H when __need_FILE is defined"
#endif

#define __need___FILE
#include <stdio.h>
#ifdef __STDIO_C_ENHANCED_H
#error "Should not define __STDIO_C_ENHANCED_H when __need___FILE is defined"
#endif

#define __need_malloc_and_calloc
#include <stdlib.h>
#ifdef __STDLIB_C_ENHANCED_H
#error "Should not define __STDLIB_C_ENHANCED_H when __need_malloc_and_calloc is defined"
#endif
// CHECK: LOC [[# STDLIB_FILENUM]] 443
// CHECK: func &malloc public extern (var %__size u64) <* void>
// CHECK: LOC [[# STDLIB_FILENUM]] 445
// CHECK: func &calloc public extern (var %__nmemb u64, var %__size u64) <* void>
int main(int argc, char* argv[])
{

}
