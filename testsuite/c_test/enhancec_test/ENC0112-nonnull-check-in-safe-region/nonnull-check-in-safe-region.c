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

void func_safe_region_in_safe(int argc, char* argv[])
{
    int valA = 0;
    int *ptr = argc == 1 ? NULL : &valA;
    int valB = 233;
    SAFE {
        valB = *ptr;    // CHECK: [[# @LINE ]] error
    }
    printf("%d\n", valB);

}

void func_safe_region_not_in_safe(int argc, char* argv[])
{
    int valA = 0;
    int *ptr = argc == 1 ? NULL : &valA;
    int valB = 233;
    valB = *ptr;    // CHECK-NOT: [[# @LINE ]] error
    printf("%d\n", valB);

}

int main(int argc, char* argv[])
{
    return 0;
}

