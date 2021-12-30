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
int g_arr[] = {1,2,3,4,5};
int *g_p __attribute__((count(5))) = g_arr;

int* (*func1)(int*, int*, int*)  __attribute__((count(10,1,3), returns_count(10)));
int* (*func2)(int*, int*, int*);
int* (*func3)(int*, int*, int*)  __attribute__((count(10,1,3), returns_count(10)));
int* (*func4)(int*, int*, int*)  __attribute__((count(10,1,3)));
int* (*func5)(int*, int*, int*)  __attribute__((count(10,1), returns_count(10)));
int* (*func6)(int*, int*, int*)  __attribute__((count(10,1,3), returns_count(5)));
int * vc __attribute__((returns_count(10)));

struct AA {
  int* (*func1)(int*, int*, int*)  __attribute__((count(10,1,3), returns_count(10)));
  int* (*func2)(int *, int*, int*);
};

struct BB {
  int *i __attribute__((count(8)));
};

__attribute__((count(10,1,3), returns_count(10)))
int *Test1(int* p, int *q, int *w) {
  return p;
}

__attribute__((count(10,1), returns_count(10)))
int *Test2(int* p, int *q, int *w) {
  return p;
}

__attribute__((count(10,1,3)))
int *Test3(int* p, int *q, int *w) {
  return p;
}

int *Test4(int *p, int *q ,int *w) {
  return p;
}

int Test() {
  int i = 1;
  int *p = g_p;
  int array[] = {1,2,3,4,5,6,7,8,9,10};
  int *q = array;
  int *w __attribute__((count(5))) = array;
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
  func1 = func6;  // CHECK: [[# @LINE ]] error

  struct AA a;
  a.func1 = &Test1;  // CHECK-NOT: [[# @LINE ]] error
  a.func1 = &Test2;  // CHECK: [[# @LINE ]] error
  a.func1 = &Test3;  // CHECK: [[# @LINE ]] error
  a.func1 = &Test4;  // CHECK: [[# @LINE ]] error

  a.func1 = func2;  // CHECK: [[# @LINE ]] error
  a.func1 = func3;  // CHECK-NOT: [[# @LINE ]] error
  a.func1 = func4;  // CHECK: [[# @LINE ]] error
  a.func1 = func5;  // CHECK: [[# @LINE ]] error
  a.func1 = func6;  // CHECK: [[# @LINE ]] error

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
  int *r1 = (*func1)(q, p, x);  // COM:CHECK: [[# @LINE ]] error: boundaryless
  res += *r1;
  int *r2 = (*func2)(p, q, x);  // CHECK-NOT: [[# @LINE ]] error
  res += *r2;
  int *r3 = (*a.func1)(p, p, q);  // COM:CHECK: [[# @LINE ]] warning
  res += *r3;
  // int *r4 = (*pa->func1)(q, q, w);  // COM:CHECK: [[# @LINE ]] warning
  // res += *r4;
  // int *r5 = (*func1)(q, x, b.i);  // COM:CHECK: [[# @LINE ]] warning
  // res += *r5;
  // int *r6 = (*func6)(pb->i, p, q);  // COM:CHECK: [[# @LINE ]] warning
  // res += *r6;  // COM:CHECK-NOT: [[# @LINE ]] warning

  (*func1)(r1, r2, r3);  // CHECK-NOT: [[# @LINE ]] error
  (*func1)(r1, r3, r2);  // CHECK: [[# @LINE ]] error: boundaryless
  // (*func6)(r6, r3, r2);  // COM:CHECK: [[# @LINE ]] warning

  return res;
}

struct CC {
  int* (*func1)(int*, int*, int*)  __attribute__((count(10), count(10,1,2,3), returns_count(10)));
  int* (*func2)(int *, int*, int*)  __attribute__((count(10)));
};

__attribute__((count(10,1,2), count(5,3)))
int Test_var(int *p, int *q, int *w) {
  int* (*func11)(int*, int*, int*)  __attribute__((count(10), count(10,1,2,3), returns_count(10)));
  int* (*func12)(int*, int*, int*)  __attribute__((count(10,1,2,3), returns_count(10)));

  // (*func11)(p,q,w);  // COM:CHECK: [[# @LINE ]] warning
  // (*func12)(p,q,w);  // COM:CHECK: [[# @LINE ]] warning
  struct CC c;
  // (*c.func1)(p,q,w);  // COM:CHECK: [[# @LINE ]] warning
  (*c.func2)(p,q,w);  // CHECK-NOT: [[# @LINE ]] warning

  return 0;
}

int Test_arg(int* (*lfunc1)(int*, int*, int*) __attribute__((count(10,1,3), returns_count(10))),
             int* (*lfunc2)(int*, int*, int*),
             int* (*lfunc3)(int*, int*, int*) __attribute__((count(10), count(10,1,2,3), returns_count(10)))) {
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
  lfunc1 = func6;  // CHECK: [[# @LINE ]] error
  // int *r1 = (*func1)(g_p, g_p, g_p);  // COM:CHECK: [[# @LINE ]] warning
  return 0;
}

int main() {
  return 0;
}
