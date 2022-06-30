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
#include <stdio.h>
#include <stdlib.h>
#define MALLOC(n, type) ((type *) malloc((n)* sizeof(type)))

#define ARG_LEN 16
#define RET_LEN 16
#define ARG_BYTE_COUNT (ARG_LEN * sizeof(long))
#define RET_BYTE_COUNT (RET_LEN * sizeof(float))
#define NUM 2

int g_arg_count = ARG_BYTE_COUNT - 1;

float* __attribute__((count(ARG_LEN))) func(long* arg, int ret_len) \
__attribute__((returns_count_index(2)));

float* func(long* arg, int ret_len)
{
    for (int i = 0; i < ARG_LEN; i++) {
        arg[i] = 1;
    }
    ret_len = ret_len * sizeof(float) / sizeof(double);
    double* ret_p __attribute__((count(ret_len))) = MALLOC(ret_len, double);
    double* ret = ret_p;
    return (float*)ret;
}

int main(int argc, char* argv[])
{
    return 0;
}
