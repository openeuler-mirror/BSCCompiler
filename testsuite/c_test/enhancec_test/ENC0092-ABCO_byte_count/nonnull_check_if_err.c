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
    ret_len = ret_len / sizeof(long);
    int* ret_p __attribute__((count(ret_len))) = MALLOC(ret_len, int);
    int* ret = ret_p;
    return ret; // need error
}

int* func1(char* arg, int arg_len, int ret_len) __attribute__((returns_byte_count("ret_len")));
int* func1(char* arg, int arg_len, int ret_len)
{
    ret_len = ret_len / 7;
    int* ret_p __attribute__((count(ret_len))) = MALLOC(ret_len, int);
    int* ret = ret_p;
    return ret; // need error
}
int* func2(char* arg, int arg_len, int ret_len) __attribute__((returns_byte_count("ret_len")));
int* func2(char* arg, int arg_len, int ret_len)
{
    ret_len = ret_len;
    int* ret_p __attribute__((count(ret_len))) = MALLOC(ret_len, int);
    int* ret = ret_p;
    return ret; // need delete the assert
}
int* func3(char* arg, int arg_len, int ret_len) __attribute__((returns_byte_count("ret_len")));
int* func3(char* arg, int arg_len, int ret_len)
{
    ret_len = ret_len * 3;
    int* ret_p __attribute__((count(ret_len))) = MALLOC(ret_len, int);
    int* ret = ret_p;
    return ret; // need delete the assert
}

int* func4(char* arg, int arg_len, int ret_len) __attribute__((returns_byte_count("ret_len")));
int* func4(char* arg, int arg_len, int ret_len)
{
    ret_len = ret_len / 2;
    int* ret_p __attribute__((count(ret_len))) = MALLOC(ret_len, int);
    int* ret = ret_p;
    return ret; // need delete the asert
}


int main(int argc, char* argv[])
{
    return 0;
}
