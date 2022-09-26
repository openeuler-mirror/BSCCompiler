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
unsigned int* func_a(int* p)
{
    unsigned int* ret = (unsigned int*) malloc(sizeof(unsigned int));
    ret = NULL;
    return ret; // CHECK: [[# @LINE ]] error: func_a return nonnull but got null pointer
}

int main(int argc, char* argv[])
{
    return 0;
}
