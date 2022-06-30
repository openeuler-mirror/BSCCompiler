/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include <string.h>

int main (void) {
    int i;
    int arr1[100];
    int arr2[100];
    int arr3[100];

    arr1[1] = 2;
    arr3[0] = 3;
    for (i = 0; i <= 100; i++) {
        arr2[i] = i + 1; // CHECK: [[# @LINE ]] error: the pointer >= the upper bounds when accessing the memory
        /* Not compliant in iteration i==100 */
    }
    return arr1[1] - 2;
}

