/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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

__attribute__((returns_nonnull, nonnull))
int *testReturnPtr(int *ptr) {
  for (int i = 0; i < 10; ++i) {
    *ptr += i;
  }
  return ptr;
}

int* test(int x) {
  int *p;
  if (x > 3) {
    p = testReturnPtr(p);
    return p;
  } else {
    int *q;
    q = testReturnPtr(p);
    return q;
  }
}
int* test1(int x) {
  int *p;
  if (x > 3) {
    p = testReturnPtr(p);
    return p;
  } else {
    int *q;
    q = malloc(sizeof(int) * 10);
    return q;
  }
}

int main() {
  return 0;
}
