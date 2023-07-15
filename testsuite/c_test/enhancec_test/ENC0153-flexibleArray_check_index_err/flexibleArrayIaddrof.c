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

struct B {
  int len;
  int fa[];
};

void iaddrof_check_fa_index_err() {
  struct A *a = (struct A*)malloc(sizeof(struct A) + 10);
  a->len = 10;
  // CHECK: [[# @LINE + 1]] error: the pointer >= the upper bounds when accessing the memory
  a->fa[12] = 1;
  // CHECK-NOT: [[# @LINE + 1]] error: the pointer >= the upper bounds when accessing the memory
  a->fa[9] = 1;

  struct B *b = (struct B*)malloc(sizeof(struct B) + 10 * sizeof(int));
  // CHECK-NOT: [[# @LINE + 1]] error: the pointer >= the upper bounds when accessing the memory
  b->fa[8] = 1;
  // CHECK-NOT: [[# @LINE + 1]] error: the pointer >= the upper bounds when accessing the memory
  b->fa[11] = 2;
}

int main() {
  return 0;
}
