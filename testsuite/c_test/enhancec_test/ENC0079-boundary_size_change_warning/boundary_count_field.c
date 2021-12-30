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
  int *i __attribute__((count("len")));
  int len;
};

struct B {
  struct A a;
  struct A *pa;
};

int g_a[] = {1,2,3,4,5};
int size = 5;
int *g_p __attribute__((count(size))) = g_a;


int func(int x) {
  return x;
}

__Safe__ int* func1(int *p) {
  return p;
}

__Unsafe__ int test_global() {
  int res = 0;
  res += func(*(g_p + 4));
  int arr[] = {1,2,3,4};
  g_p = arr;
  __Safe__ {
    size = 4;  // CHECK: [[# @LINE ]] warning
    int *p_size = &size;  // CHECK: [[# @LINE ]] warning
    p_size = func1(&size);  // CHECK: [[# @LINE ]] warning
  }
  int *p_size = &size;  // CHECK-NOT: [[# @LINE ]] warning
  return res;
}

int test_field_init() {
  int res = 0;
  struct A a;
  a.len = 5;  // CHECK: [[# @LINE ]] warning
  int *p_size = &a.len;  // CHECK: [[# @LINE ]] warning
  p_size = func1(&a.len);  // CHECK: [[# @LINE ]] warning
  a.i = g_p;

  __Unsafe__ {
    a.len = 5;  // CHECK-NOT: [[# @LINE ]] warning
    p_size = &a.len;  // CHECK-NOT: [[# @LINE ]] warning
    p_size = func1(&a.len);  // CHECK-NOT: [[# @LINE ]] warning
  }

  struct B b;
  b.a = a;
  b.pa = &a;

  struct B arr_b[5];
  struct B *pb = &b;
  pb->pa->len = 4;  // CHECK: [[# @LINE ]] warning
  p_size = &pb->pa->len;  // CHECK: [[# @LINE ]] warning
  p_size = func1(&pb->pa->len);  // CHECK: [[# @LINE ]] warning
  res += func(pb->pa->i[3]);

  __Unsafe__ {
    pb->pa->len = 4;  // CHECK-NOT: [[# @LINE ]] warning
    p_size = &pb->pa->len;  // CHECK-NOT: [[# @LINE ]] warning
  }

  return res;
}

int test_local_var() {
  int res = 0;
  int size = 5;
  int *p __attribute__((count(size))) = g_a;
  size = 4;  // CHECK-NOT: [[# @LINE ]] warning
  res += func(p[3]);
  return res;
}

int main() {
  return 0;
}

