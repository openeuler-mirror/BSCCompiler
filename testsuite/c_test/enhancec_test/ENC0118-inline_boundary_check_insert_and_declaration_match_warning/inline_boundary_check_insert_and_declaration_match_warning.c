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

int g_size = 3;

__attribute__((count_index(2, 1)))
void callee(int *p, size_t size) {
  return;
}

static inline void caller(size_t size) {
  int *p = (int*)malloc(sizeof(int) * size);
  callee(p, 3); // CHECK: [[# @LINE ]] warning: can't prove pointer's bounds match the function callee declaration for the 1st argument when inlined to main
}

int main() {
  caller(g_size);
  return 0;
}
