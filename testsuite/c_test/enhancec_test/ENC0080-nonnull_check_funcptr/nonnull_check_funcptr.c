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

#include <stddef.h>
int* (*func1)(int*, int*, int*)  __attribute__((nonnull(1,3), returns_nonnull));
int* (*func2)(int*, int*, int*);
int* (*func3)(int*, int*, int*)  __attribute__((nonnull(1,3), returns_nonnull));
int* (*func4)(int*, int*, int*)  __attribute__((nonnull(1,3)));
int* (*func5)(int*, int*, int*)  __attribute__((nonnull(1), returns_nonnull));
int * vc __attribute__((returns_nonnull));

struct AA {
  int* (*func1)(int*, int*, int*)  __attribute__((nonnull(1,3), returns_nonnull));
  int* (*func2)(int *, int*, int*);
};

struct BB {
  int *i;
};

__attribute__((nonnull(1,3), returns_nonnull))
int *Test1(int* p, int *q, int *w) {
  return p;
}

__attribute__((nonnull(1), returns_nonnull))
int *Test2(int* p, int *q, int *w) {
  return p;
}

__attribute__((nonnull(1,3)))
int *Test3(int* p, int *q, int *w) {
  return p;
}

int *Test4(int *p, int *q ,int *w) {
  return p;
}

int Test() {
  int i = 1;
  int *p = &i;
  int *q = &i;
  int *w = &i;
  int *x;
  struct BB b;
  struct BB *pb = &b;

  func1 = &Test1;  // CHECK-NOT: [[# @LINE ]] error
  func1 = &Test2;  // CHECK: [[# @LINE ]] error
  func1 = &Test3;  // CHECK: [[# @LINE ]] error
  func1 = &Test4;  // CHECK: [[# @LINE ]] error

  func2 = &Test1;  // CHECK: [[# @LINE ]] error
  func2 = &Test2;  // CHECK: [[# @LINE ]] error
  func2 = &Test3;  // CHECK: [[# @LINE ]] error
  func2 = &Test4;  // CHECK-NOT: [[# @LINE ]] error

  func1 = func2;  // CHECK: [[# @LINE ]] error
  func1 = func3;  // CHECK-NOT: [[# @LINE ]] error
  func1 = func4;  // CHECK: [[# @LINE ]] error
  func1 = func5;  // CHECK: [[# @LINE ]] error

  struct AA a;
  a.func1 = &Test1;  // CHECK-NOT: [[# @LINE ]] error
  a.func1 = &Test2;  // CHECK: [[# @LINE ]] error
  a.func1 = &Test3;  // CHECK: [[# @LINE ]] error
  a.func1 = &Test4;  // CHECK: [[# @LINE ]] error

  a.func1 = func2;  // CHECK: [[# @LINE ]] error
  a.func1 = func3;  // CHECK-NOT: [[# @LINE ]] error
  a.func1 = func4;  // CHECK: [[# @LINE ]] error
  a.func1 = func5;  // CHECK: [[# @LINE ]] error

  func1 = a.func1;  // CHECK-NOT: [[# @LINE ]] error
  func1 = a.func2;  // CHECK: [[# @LINE ]] error

  struct AA *pa;
  pa = &a;
  pa->func1 = &Test1;  // CHECK-NOT: [[# @LINE ]] error
  pa->func1 = &Test2;  // CHECK: [[# @LINE ]] error
  pa->func1 = &Test3;  // CHECK: [[# @LINE ]] error
  pa->func1 = &Test4;  // CHECK: [[# @LINE ]] error

  pa->func1 = func2;  // CHECK: [[# @LINE ]] error
  pa->func1 = func3;  // CHECK-NOT: [[# @LINE ]] error
  pa->func1 = func4;  // CHECK: [[# @LINE ]] error
  pa->func1 = func5;  // CHECK: [[# @LINE ]] error

  func1 = pa->func1;  // CHECK-NOT: [[# @LINE ]] error
  func1 = pa->func2;  // CHECK: [[# @LINE ]] error

  int res = 0;
  return res;
}

struct CC {
  int* (*func1)(int*, int*, int*)  __attribute__((nonnull, nonnull(1,2,3), returns_nonnull));
  int* (*func2)(int *, int*, int*)  __attribute__((nonnull));
};

__attribute__((nonnull))
int Test11(int *p, int *q, int *w) {
  int* (*func11)(int*, int*, int*)  __attribute__((nonnull, nonnull(1,2,3), returns_nonnull));  // // CHECK: [[# @LINE ]] error
  int* (*func12)(int*, int*, int*)  __attribute__((nonnull(1,2,3), returns_nonnull));

  return 0;
}

int Test_arg(int* (*lfunc1)(int*, int*, int*) __attribute__((nonnull(1,3), returns_nonnull)),
             int* (*lfunc2)(int*, int*, int*) __attribute__((nonnull)),
             int* (*lfunc3)(int*, int*, int*) __attribute__((nonnull, nonnull(1,3), returns_nonnull))) {
  lfunc1 = &Test1;  // CHECK-NOT: [[# @LINE ]] error
  lfunc1 = &Test2;  // CHECK: [[# @LINE ]] error
  lfunc1 = &Test3;  // CHECK: [[# @LINE ]] error
  lfunc1 = &Test4;  // CHECK: [[# @LINE ]] error

  lfunc2 = &Test1;  // CHECK: [[# @LINE ]] error
  lfunc2 = &Test2;  // CHECK: [[# @LINE ]] error
  lfunc2 = &Test3;  // CHECK: [[# @LINE ]] error
  lfunc2 = &Test4;  // CHECK-NOT: [[# @LINE ]] error

  lfunc1 = func2;  // CHECK: [[# @LINE ]] error
  lfunc1 = func3;  // CHECK-NOT: [[# @LINE ]] error
  lfunc1 = func4;  // CHECK: [[# @LINE ]] error
  lfunc1 = func5;  // CHECK: [[# @LINE ]] error
  
  return 0;
}


int main() {
  return 0;
}
