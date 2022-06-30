/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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

// CHECK: [[# FILENUM:]] "{{.*}}/nonnull_check_if_err.c"
#include <stdio.h>
#include <stdlib.h>
#define MALLOC(n, type) ((type *) malloc((n)* sizeof(type)))

#define ARG_LEN 16
#define RET_LEN 16
#define ARG_BYTE_COUNT (ARG_LEN * sizeof (char))
#define RET_BYTE_COUNT (RET_LEN * sizeof(int))
#define NUM 2
#define OFFSET ARG_LEN

int g_arg_count = ARG_BYTE_COUNT - 1;

__attribute__((count("arg_len", 1)))
int* func(char* arg, int arg_len, int ret_len) __attribute__((returns_byte_count("ret_len")));

int* func(char* arg, int arg_len, int ret_len)
{
    for (int i = 0; i < arg_len; i++) {
        arg[i] = 1;
    }
    ret_len = ret_len / sizeof(int);
    int* ret_p __attribute__((count(ret_len))) = MALLOC(ret_len, int);
    int* ret = ret_p;
    return ret;
}

int main(int argc, char* argv[])
{
    int arg_len = ARG_LEN + OFFSET;
    int ret_len = RET_LEN;
    char* arg __attribute__((count(arg_len))) = MALLOC(arg_len, char);
    if (argc != 1) {
        arg_len = ARG_LEN * NUM + OFFSET;
    }
    int* ret = func(arg + OFFSET, arg_len, ret_len * sizeof(int));
    return 0;
}
