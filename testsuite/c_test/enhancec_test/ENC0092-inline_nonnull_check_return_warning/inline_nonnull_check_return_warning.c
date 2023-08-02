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

unsigned int* __attribute__((returns_nonnull)) func_a(int*  __attribute__((nonnull)) p) ;
unsigned int* __attribute__((returns_nonnull)) func_b(int p) ;
unsigned int g_b = 0;


inline unsigned int* func_a(int* p)
{
    int a = *p;
    *p = a + 1;
    g_b = g_b + 1;
    unsigned int* ret = (unsigned int*) malloc(sizeof(unsigned int));
    ret = &g_b;
    if (a > 1) {
        ret = NULL;
    }
    return ret; // CHECK: [[# @LINE ]] warning: func_a return nonnull but got nullable pointer
}

unsigned int* func_b(int p)
{
    g_b = g_b + 1;
    unsigned int* ret = (unsigned int*) malloc(sizeof(unsigned int));
    if (p > 1) {
        ret = NULL;
    } else {
        ret = &g_b;
    }
    return ret; // CHECK: [[# @LINE ]] warning: func_b return nonnull but got nullable pointer
}

int main(int argc, char* argv[])
{
    int c = 0;
    int* cPtr = NULL;
    if (argc == 1) {
        c = 23;
        cPtr = &c;
    } else {
        c = 1;
        cPtr = &c;
    }
    if (cPtr != NULL) {
        unsigned int* __attribute__((nonnull)) ret = func_a(cPtr);
        printf("%d, %d\n", *ret, c);
    }
    c = 1;
    cPtr = &c;
    unsigned int* __attribute__((nonnull)) ret = func_b(1);
    printf("%d, %d\n", *ret, c);

    return 0;
}
