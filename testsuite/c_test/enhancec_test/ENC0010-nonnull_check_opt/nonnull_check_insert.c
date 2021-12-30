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

struct A {
  int *i;
};

struct B {
  struct A *a;
};


__attribute__((returns_nonnull, nonnull))
int *testReturnPtr(int *ptr) {
  for (int i = 0; i < 10; ++i) {
    *ptr += i;
  }
  return ptr;
}

int test1(int *arg1, int *arg2) {
  if (arg1 == NULL) {
    return 1;
  }
  printf("*arg1=%d ", *arg1);
  if (arg2 == NULL) {
    return 2;
  }
  printf("*arg2=%d ", *arg2);
  return 3;
}

int test2(int *arg1) {
  int a = *arg1;
  for (int i = 0; i < 10; ++i) {
    a += *arg1;
  }
  return a;
}

void test3(int *arg1) {
  if (arg1 != NULL) {
    int *temp = testReturnPtr(arg1);
    for (int i = 0; i < 10; ++i) {
      (*temp)++;
    }
    printf("%d\n", *temp);
  }
}

int main() {
  int a = 1;
  int *p = &a;
  int b = 2;
  int *q = &b;
  printf("%d\n", test1(NULL, p));
  printf("%d\n", test1(q, NULL));
  printf("%d\n", test1(p, q));

  printf("%d\n", test2(p));
  printf("%d\n", *testReturnPtr(q));
  test3(q);

  return 0;
}
