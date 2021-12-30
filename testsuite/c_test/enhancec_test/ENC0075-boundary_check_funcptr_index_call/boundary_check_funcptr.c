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

int* (*func1)(int*, int*, int)  __attribute__((count_index(3,1,2), returns_count_index(3)));
int* (*func2)(int*, int*, int);
int* (*func3)(int*, int*, int)  __attribute__((count_index(3,1,2), returns_count_index(3)));
int* (*func4)(int*, int*, int)  __attribute__((count_index(3,1,2)));
int* (*func5)(int*, int*, int)  __attribute__((count_index(3,1), returns_count_index(3)));
int * vb __attribute__((count_index(1)));
int * vc __attribute__((returns_count_index(1)));

struct AA {
  int* (*func1)(int*, int*, int)  __attribute__((count_index(3,1,2), returns_count_index(3)));
  int* (*func2)(int *, int*, int);
};

struct BB {
  int *i __attribute__((count(10)));
};

__attribute__((count_index(3,1,2), returns_count_index(3)))
int *Test1(int* p, int *q, int len) {
  return p;
}

__attribute__((count_index(3,1), returns_count_index(3)))
int *Test2(int* p, int *q, int len) {
  return p;
}

int *Test3(int* p __attribute__((count_index(3))), int *q __attribute__((count_index(3))), int len) {
  return p;
}

int *Test4(int *p, int *q ,int len) {
  return p;
}

__attribute__((count("len",1,2), returns_count("len")))
int *Test5(int* p, int *q, int len) {
  return p;
}

int LEN = 10;
__attribute__((count(LEN,1,2), returns_count(LEN)))
int *Test6(int* p, int *q, int len) {
  return p;
}

__attribute__((returns_count("len")))
int *Test7(int* p __attribute__((count("len"))), int *q __attribute__((count("len"))), int len) {
 return p;
}

int Test() {
  int len = 5;
  int *p = g_p;
  int array[] = {1,2,3,4,5,6,7,8,9,10};
  int *q = array;
  int *x;
  struct BB b;
  struct BB *pb = &b;

  func1 = &Test1;  // CHECK-NOT: [[# @LINE ]] error
  
  func2 = &Test4;  // CHECK-NOT: [[# @LINE ]] error

  struct AA a;
  a.func1 = &Test1;  // CHECK-NOT: [[# @LINE ]] error

  struct AA *pa;
  pa = &a;
  pa->func1 = &Test7;  // CHECK-NOT: [[# @LINE ]] error

  pa->func1 = func3;  // CHECK-NOT: [[# @LINE ]] error

  int res = 0;
  int *r1 = (*func1)(p, q, 5);  // CHECK-NOT: [[# @LINE ]] error
  res += *r1;
  int *r2 = (*func2)(p, q, 10);  // CHECK-NOT: [[# @LINE ]] error
  res += *r2;
  int *r3 = (*func3)(p, q, len);  // CHECK-NOT: [[# @LINE ]] error
  res += *r3;
  // int *r4 = (*a.func1)(p, q, 10);  // COM:CHECK-NOT: [[# @LINE ]] error
  // res += *r4;
  int *r5 = (*pa->func1)(p, q, 5);  // CHECK-NOT: [[# @LINE ]] error
  res += *r5;
  int *r6 = (*func1)(b.i, p, 5);  // CHECK-NOT: [[# @LINE ]] error
  res += *r6;
  int *r7 = (*func1)(pb->i, q, 10);  // CHECK-NOT: [[# @LINE ]] error
  res += *r7;
  // int *r8 = (*func1)(r1, r2, 5);  // COM:CHECK: [[# @LINE ]] error
  int *r9 = (*func1)(r1, r3, 5);  // CHECK-NOT: [[# @LINE ]] error
  // int *r10 = (*func1)(r1, r3, 10);  // COM:CHECK: [[# @LINE ]] error

  return res;
}

int Test_arg(int* (*lfunc1)(int*, int*, int) __attribute__((count_index(3,1,2), returns_count_index(3))),
             int* (*lfunc2)(int*, int*, int),
             int* (*lfunc3)(int*, int*, int) __attribute__((count(10), count_index(3,1,2), returns_count_index(3)))) {
  lfunc1 = &Test1;  // CHECK-NOT: [[# @LINE ]] error

  lfunc2 = &Test4;  // CHECK-NOT: [[# @LINE ]] error

  int *r1 = (*func1)(g_p, g_p, 3);  // CHECK-NOT: [[# @LINE ]] error
  return 0;
}


int main() {
  return 0;
}
