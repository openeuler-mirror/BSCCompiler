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
#include <stdlib_c_enhanced.h>
__attribute__((count(10, 1)))
void func1(int* p) {
  printf("func1");
}

int main() {
  int *q;
  int *l = NULL;
  int *p __attribute__((count(10))) = malloc(sizeof(int) * 10);
  if (*p) {
    q = l;
  } else {
    q = p;
  }
  func1(q);
  return 0;
}
