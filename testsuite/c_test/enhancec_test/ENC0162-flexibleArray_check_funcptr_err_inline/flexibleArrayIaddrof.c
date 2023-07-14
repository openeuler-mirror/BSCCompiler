/*
 * Copyright (c) [2023-2023] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include <stdlib.h>
struct A {
  int len;
  int fa[] __attribute__((count("len")));
};

int* (*func1)(int*, int*)  __attribute__((byte_count(10, 1)));

__attribute__((byte_count(10, 1)))
int *Test1(int* p, int *q) {
  return p;
}

static inline void iaddof_check_funcPtr_err() {
  struct A *a = (struct A*)malloc(sizeof(struct A) + 10);
  a->len = 1;
  func1 = &Test1;
  // CHECK: [[# @LINE + 1]] error: the pointer's bounds does not match the function function_pointer declaration for the 1st argument when inlined to main
  (void)(*func1)(a->fa, a->fa);
}

int main() {
  iaddof_check_funcPtr_err();
  return 0;
}
