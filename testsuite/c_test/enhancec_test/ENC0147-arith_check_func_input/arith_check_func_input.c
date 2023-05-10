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

int f(int *p) {
  return *p;
}

int main(int argc, char **args) {
    if(argc < 2) {
        printf("need input offset\n");
        return -1;
    }
    int *p = (int*)malloc(sizeof(int) * 5);
    if (p == NULL) {
        return -2;
    }
    int offset = atoi(args[1]);
    // CHECK: [[# @LINE + 2]] warning: can't prove the pointer >= the lower bounds after calculation
    // CHECK: [[# @LINE + 1]] warning: can't prove the pointer < the upper bounds after calculation
    f(p + offset);
    return 0;
}
