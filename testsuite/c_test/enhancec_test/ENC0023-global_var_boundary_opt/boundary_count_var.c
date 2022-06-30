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


// CHECK: [[# FILENUM:]] "{{.*}}/boundary_count_var.c"

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
int *g_p __attribute__((count(5))) = g_a;

int func(int x) {
  return x;
}

int test_local() {
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: dassign %_boundary.q{{.*}}.lower 0 (dread ptr %q{{.*}}
  int *q __attribute__((count(3)));
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: dassign %_boundary.q1{{.*}}.lower 0 (dread ptr %_boundary.q{{.*}}
  int *q1 = q;
  int *q2;
  int res = 0;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 4 ]]
  // CHECK-NEXT: dassign %_boundary.p{{.*}}.lower 0 (dread ptr %p{{.*}}
  // CHECK: assignassertle{{.*}}
  // CHECK: dassign %_boundary.p{{.*}}.lower 0 (addrof ptr $g_a)
  int *p __attribute__((count(5))) = g_a;
  res += func(*(p + 4));
  int *p1 = p + 1;
  res += func(*(p1 + 3));

  // CHECK: LOC [[# FILENUM]] [[# @LINE + 4 ]]
  // CHECK: assertge{{.*}}
  // CHECK: assignassertle{{.*}}
  // CHECK: dassign %_boundary.q{{.*}}.lower 0 (dread ptr %_boundary.p{{.*}}
  q = p + 1;
  q1 = p;
  q2 = p;
  res += func(q[2]);
  res += func(q1[4]);
  res += func(q2[4]);
  return res;
}

int test_global() {
  int res = 0;
  res += func(g_a[2]);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NEXT: assertge{{.*}}
  res += func(g_p[2]);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: dassign %_boundary.p{{.*}}.lower 0 (dread ptr $g_p)
  int *p = g_p;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 4 ]]
  // CHECK: dassign %_boundary.p1{{.*}}.lower 0 (dread ptr %p1{{.*}}
  // CHECK: assignassertle{{.*}}
  // CHECK: dassign %_boundary.p1{{.*}}.lower 0 (dread ptr $g_p)
  int *p1 __attribute__((count(3))) = g_p;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NEXT: dassign %_boundary.p2{{.*}}.lower 0 (dread ptr %p2{{.*}}
  int *p2 __attribute__((count(3)));
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 3 ]]
  // CHECK: assignassertle{{.*}}
  // CHECK: dassign %_boundary.p2{{.*}}.lower 0 (dread ptr $g_p)
  p2 = g_p;
  res += func(p1[2]);
  res += func(p2[2]);
  return res;
}

int test_global_assign1(int a) {
  int res = 0;
  res += func(g_p[2]);
 
  if (a > 0) {
    res += func(g_p[2]);
  } else {
    int arr[7] = {1, 2, 3, 4, 5, 6, 7};
    g_p = arr;
    res += func(g_p[2]);
  }
  return res;
}

int test_global_assign2(int a) {
  int res = 0;
  res += func(g_p[2]);

  if (a > 0) {
    int arr[7] = {1, 2, 3, 4, 5, 6, 7};
    g_p = arr;
    res += func(g_p[2]);
  } else {
    res += func(g_p[2]);
  }
  return res;
}

int main() {
  return 0;
}
