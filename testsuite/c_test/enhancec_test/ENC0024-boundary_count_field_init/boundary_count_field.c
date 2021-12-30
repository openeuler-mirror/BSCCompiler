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

int test_field_init() {
  int res = 0;
  // init field boundary
  // CHECK: dassign %_boundary.b{{.*}}.lower 0 (dread ptr %b{{.*}}
  // CHECK: dassign %_boundary.A_i{{.*}}.lower 0 (iread ptr {{.*}}
  struct A a;
  a.len = 4;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 3 ]]{{$}}
  // CHECK: assignassertle{{.*}}
  // CHECK: dassign %_boundary.a{{.*}}.lower 0 (dread ptr $g_p)
  a.i = g_p;

  struct B b;
  b.a = a;
  b.pa = &a;
 
  //b.a.len = 5;
  //b.pa->len = 5;

  struct B arr_b[5];

  struct B *pb = &b;

  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK: assertge{{.*}}
  res += func(*(b.a.i + 3));
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK: assertge{{.*}}
  res += func(pb->pa->i[3]);

  // CHECK: LOC [[# FILENUM]] [[# @LINE + 4 ]]{{$}}
  // CHECK: assertge
  // CHECK: assertle
  // CHECK: dassign %_boundary.A_i{{.*}}.lower 0 (dread ptr $g_p)
  pb->pa->i = g_p + 1;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 4 ]]{{$}}
  // CHECK: assertge
  // CHECK: assertle
  // CHECK: dassign %_boundary.b{{.*}}.lower 0 (dread ptr $g_p)
  b.a.i = g_p + 1;
  
  res += func(*(b.a.i + 3));
  res += func(pb->pa->i[3]);
  return res;
}

int test_field_init1(int i) {
  int res = 0;
  struct A a;
  a.len = 5;
  a.i = g_p;
  if (i > 0) {
    int arr[7] = {1, 2, 3, 4, 5, 6, 7};
    a.len = 7;
    a.i = arr;
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
    // CHECK: assertge
    res += func(*(a.i + 3));
  } else {
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
    // CHECK: assertge
    res += func(*(a.i + 3));
  }
  return res;
}

struct A ga;
int test_global_field_init1(int i) {
  int res = 0;
  ga.len = 5;
  ga.i = g_p;
  if (i > 0) {
    int arr[7] = {1, 2, 3, 4, 5, 6, 7};
    ga.len = 7;
    ga.i = arr;
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
    // CHECK: assertge
    res += func(*(ga.i + 3));
  } else {
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
    // CHECK: assertge
    res += func(*(ga.i + 3));
  }
  return res;
}

struct A *gpa;
int test_global_field_init2(int i) {
  int res = 0;
  struct A a;
  a.len = 5;
  a.i = g_p;
  gpa = &a;
  if (i > 0) {
    int arr[7] = {1, 2, 3, 4, 5, 6, 7};
    gpa->len = 7;
    gpa->i = arr;
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
    // CHECK: assertge
    res += func(*(gpa->i + 3));
  } else {
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
    // CHECK: assertge
    res += func(*(gpa->i + 3));
  }
  return res;
}


int main() {
  return 0;
}

