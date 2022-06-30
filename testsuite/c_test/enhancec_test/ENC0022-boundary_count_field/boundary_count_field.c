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


// CHECK: [[# FILENUM:]] "{{.*}}/boundary_count_field.c"

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

int test_field(struct B b, struct B *pb) {
  int res = 0;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: assertge{{.*}}
  res += func(*(b.a.i + 4));
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: assertge{{.*}}
  res += func(pb->pa->i[4]);
}

int test_field1(struct B b, int i) {
  // init field boundary
  // CHECK: dassign %_boundary.b{{.*}}.lower 0{{.*}}
  int res = 0;
  if (i > 0) {
    int arr[7] = {1, 2, 3, 4, 5, 6, 7};
    b.a.len = 7;
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 3 ]]
    // CHECK: assignassertle{{.*}}
    // CHECK: dassign %_boundary.b{{.*}}.lower 0{{.*}}
    b.a.i = arr;
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
    // CHECK: assertge{{.*}}
    res += func(*(b.a.i + 4));
  } else {
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
    // CHECK: assertge{{.*}}
    res += func(*(b.a.i + 4));
  }
  return res;
}

__attribute__((count("len", 1)))
int test_arg(int* a, struct B *b, int len) {
  int res = 0;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: assertge{{.*}}
  res += func(a[1]);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: assertge{{.*}}
  res += func(b->a.i[1]);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 3 ]]
  // CHECK: assignassertle{{.*}}
  // CHECK: dassign %_boundary.a.upper 0 (add ptr {{.*}}
  a = g_p;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 3 ]]
  // CHECK: assignassertle{{.*}}
  // CHECK: dassign %_boundary.B_i{{.*}}
  b->a.i = g_p;
  res += func(a[1]);
  return res;
}

struct A ga;
void test_global_field_init1(int i) {
  int res = 0;
  ga.len = 5;
  ga.i = g_p;
}

int test_global_field(int i) {
  int res = 0;
  if (i > 0) {
    int arr[7] = {1, 2, 3, 4, 5, 6, 7};
    ga.len = 7;
    ga.i = arr;
    res += func(*(ga.i + 3));
  } else {
    res += func(*(ga.i + 3));
  }
  return res;
}


int main() {
  return 0;
}

