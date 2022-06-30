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

#include <stdlib.h>
__attribute__((noinline))
int get(int *buf __attribute__((count(10)))) {
  return buf[9];
}

int main() {
  int *q;
  int *p __attribute__((count(6)));

  int *D = malloc(sizeof(int) * 10);
  get(D);  // CHECK-NOT: [[# @LINE ]] error

  int *F = calloc(sizeof(int), 10);

  get(F);  // CHECK-NOT: [[# @LINE ]] error

  int *E = malloc(sizeof(int) * 3);
  get(E);  // CHECK: [[# @LINE ]] error
  return 0;
}
