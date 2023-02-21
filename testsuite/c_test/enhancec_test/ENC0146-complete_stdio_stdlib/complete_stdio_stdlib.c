/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 */
// CHECK: [[# STDIO_FILENUM:]] "{{.*}}/libc_enhanced/include/stdio.h"
// CHECK: [[# STDLIB_FILENUM:]] "{{.*}}/libc_enhanced/include/stdlib.h"
#include <stdio.h>
#ifndef __STDIO_C_ENHANCED_H
#error "Should define __STDIO_C_ENHANCED_H when !defined __need_FILE && !defined __need___FILE"
#endif
// CHECK: LOC [[# STDIO_FILENUM]] 30
// CHECK: func &snprintf public varargs safed (var %str <* u8> restrict, var %arg|72 u64, var %arg|73 <* u8> restrict, ...) i32
// CHECK: LOC [[# STDIO_FILENUM]] 33
// CHECK: func &vsnprintf public safed (var %str <* u8> restrict, var %arg|75 u64, var %arg|76 <* u8> restrict, var %arg|77 <$__va_list>) i32
// CHECK: LOC [[# STDIO_FILENUM]] 37
// CHECK: func &fgets public safed (var %str <* u8> restrict, var %arg|79 i32, var %arg|80 <* <$_IO_FILE>> restrict) <* u8>

#include <stdlib.h>
#ifndef __STDLIB_C_ENHANCED_H
#error "Should define__STDLIB_C_ENHANCED_H when !defined __need_FILE && !defined __need_malloc_and_calloc"
#endif
// CHECK: LOC [[# STDLIB_FILENUM]] 24
// CHECK: func &malloc public safed (var %arg|112 u64) <* void>
// CHECK: LOC [[# STDLIB_FILENUM]] 26
// CHECK: func &calloc public safed (var %n_elements u64 used, var %element_size u64 used) <* void>
// CHECK: LOC [[# STDLIB_FILENUM]] 29
// CHECK: func &realloc public safed (var %arg|114 <* void>, var %arg|115 u64) <* void>
int main(int argc, char* argv[])
{

}
