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

 __attribute__((nonnull(1,2,3), returns_nonnull)) 
int *Test5(int* p, int *q, int *w) {
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
  func2 = &Test4;  // CHECK-NOT: [[# @LINE ]] error

  struct AA a;
  a.func1 = &Test1;  // CHECK-NOT: [[# @LINE ]] error

  struct AA *pa;
  pa = &a;
  pa->func1 = &Test1;  // CHECK-NOT: [[# @LINE ]] error

  int res = 0;
  int *r1 = (*func1)(p, q, w);  // COM:CHECK-NOT: [[# @LINE ]] warning: Dereference{{.*}}
  res += *r1;  // CHECK-NOT: [[# @LINE ]] warning: Dereference{{.*}}
  int *r2 = (*func2)(p, q, w);  // COM:CHECK-NOT: [[# @LINE ]] warning: Dereference{{.*}}
  // return nullable checking
  res += *r2;  // CHECK: [[# @LINE ]] warning
  int *r3 = (*a.func1)(p, q, w);  // COM:CHECK-NOT: [[# @LINE ]] warning: Dereference{{.*}}
  res += *r3;  // CHECK-NOT: [[# @LINE ]] warning
  int *r4 = (*pa->func1)(p, q, w);  // COM:CHECK-NOT: [[# @LINE ]] warning: Dereference{{.*}}
  res += *r4;  // CHECK-NOT: [[# @LINE ]] warning
  // COM:CHECK-NOT: [[# @LINE + 1 ]] warning: Dereference{{.*}}
  int *r5 = (*func1)(p, x, b.i);  // CHECK: [[# @LINE ]] warning: nullable pointer{{.*}} 3rd argument
  res += *r5;  // CHECK-NOT: [[# @LINE ]] warning
  // COM: CHECK: [[# @LINE + 2 ]] warning: nullable pointer{{.*}} 1st argument
  // CHECK: [[# @LINE + 1 ]] warning: nullable pointer{{.*}} 3rd argument
  int *r6 = (*func1)(pb->i, p, x);  // COM:CHECK-NOT: [[# @LINE ]] warning: Dereference{{.*}}
  res += *r6;  // CHECK-NOT: [[# @LINE ]] warning

  return res;
}

struct CC {
  int* (*func1)(int*, int*, int*)  __attribute__((nonnull, nonnull(1,2,3), returns_nonnull));
  int* (*func2)(int *, int*, int*)  __attribute__((nonnull));
};

__attribute__((nonnull))
int Test11(int *p, int *q, int *w) {
  int* (*func11)(int*, int*, int*)  __attribute__((nonnull, nonnull(1,2,3), returns_nonnull)) = &Test5;
  int* (*func12)(int*, int*, int*)  __attribute__((nonnull(1,2,3), returns_nonnull));

  (*func11)(p,q,w);  // CHECK-NOT: [[# @LINE ]] warning
  (*func12)(p,q,w);  // CHECK: [[# @LINE ]] warning
  struct CC c;
  (*c.func1)(p,q,w);  // CHECK-NOT: [[# @LINE ]] warning
  (*c.func2)(p,q,w);  // CHECK-NOT: [[# @LINE ]] warning
  return 0;
}

int Test_arg(int* (*lfunc1)(int*, int*, int*) __attribute__((nonnull(1,3), returns_nonnull)),
             int* (*lfunc2)(int*, int*, int*) __attribute__((nonnull)),
             int* (*lfunc3)(int*, int*, int*) __attribute__((nonnull, nonnull(1,3), returns_nonnull))) {
  lfunc1 = &Test1;  // CHECK-NOT: [[# @LINE ]] error

  lfunc2 = &Test4;  // CHECK-NOT: [[# @LINE ]] error

  int i = 1;
  int *p = &i;
  int *q;
  int *r1 = (*lfunc1)(p, q, p);  // CHECK-NOT: [[# @LINE ]] warning
  int *r2 = (*lfunc2)(p, q, p);  // CHECK-NOT: [[# @LINE ]] warning
  int *r3 = (*lfunc3)(p, q, p);  // CHECK-NOT: [[# @LINE ]] warning
  int *r4 = (*lfunc3)(p, q, q);  // CHECK: [[# @LINE ]] warning
  
  return 0;
}


int main() {
  return 0;
}
