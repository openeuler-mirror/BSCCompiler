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

int *func(int *p, int len)  __attribute__((count("len", 1))) {
  return p;
}

__attribute__((returns_count(5)))
int* test() {
  int i = 1;
  int *p = &i;
  __Unsafe__ {
    func(p, 2);
  return p;
  }
}

__Unsafe__ int main() {
  int *q __attribute__((count(5))) = malloc(sizeof(int) * 5);
  int *p = q + 6;

  int i = 1;
  int *w = &i;
  func(w, 2);
  q = w;
  return 0;
}
