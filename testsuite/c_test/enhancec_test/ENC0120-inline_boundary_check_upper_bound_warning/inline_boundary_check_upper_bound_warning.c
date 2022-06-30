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

int g_index = 2;

__attribute__((count_index(2, 1)))
static inline void caller(int *p, size_t size, int index) {
  p = p + index; // CHECK: [[# @LINE ]] warning: can't prove the pointer >= the lower bounds after calculation when inlined to main
  // CHECK: [[# @LINE - 1 ]] warning: can't prove the pointer < the upper bounds after calculation when inlined to main
}

int main() {
  int size = 3;
  int *p = (int*)malloc(sizeof(int) * size);
  caller(p, size, g_index);
  return 0;
}
