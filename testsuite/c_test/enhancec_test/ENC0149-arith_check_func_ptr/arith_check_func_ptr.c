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

int sum(int a, int b) {
    return a + b;
}

int plus(int(*total)(int a, int b) __attribute__((count(1)))) {
    int x = 1;
    int y = 1;
	return (*total)(x, y);
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
    int (*f)(int, int) __attribute__((count(1)))= sum;
    int a = plus((f + offset));
    printf("plus()的值为：%d\n", a);
    return 0;
}
