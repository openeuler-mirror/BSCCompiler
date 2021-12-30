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


// CHECK: [[# FILENUM:]] "{{.*}}/boundary_count_insert.c"

#include <stdio.h>

struct A {
  int *i;
};

struct B {
  struct A *a;
};

// Lack of attribute parameters means that only one pointer parameter is implicitly marked as boundary var in func.
__attribute__((count("len")))
void foo1(int len, int *p) {
 // CHECK: dassign %_boundary.p.lower 0 (dread ptr %p)
 // CHECK: LOC [[# FILENUM]] [[# @LINE + 1 ]]{{$}}
 return;
}

__attribute__((count("len", 2, 3)))
void foo(int len, int *p, struct B *q) {
  // The boundary assignment: l-value with or without a boundary, r-value with a boundary
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 3 ]]{{$}}
  // CHECK: assertge{{.*}}
  // CHECK: dassign %_boundary.p1{{.*}}.0.[[# IDX_P1:]].upper 0 (dread ptr %_boundary.p.upper)
  int *p1 = len + p - 3 + 1;  //pointer computed assignment
  if (len) {
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
    // CHECK-NEXT: assertge{{.*}}
    printf("%d\n", p[1]);
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
    // CHECK-NEXT: assertge{{.*}}
    printf("%d\n", *(p+1));
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
    // CHECK-NOT: assertge{{.*}}
    printf("%d\n", q->a->i[1]);
    // The boundary assignment and inserting empty r-value boundary: l-value with a boundary, the r-value without a boundary
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 3 ]]{{$}}
    // CHECK-NOT: assertge{{.*}}
    // CHECK: dassign %_boundary.p1{{.*}}.0.[[# IDX_P1]].upper 0 (dread ptr %_boundary.A_i.[[# IDX_I:]].upper)
    p1 = q->a->i;
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
    // CHECK-NEXT: assertge{{.*}}
    printf("%d\n", p1[0]);
  } else {
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 3 ]]{{$}}
    // CHECK-NOT: assertge{{.*}}
    // CHECK: dassign %_boundary.A_i.[[# IDX_I:]].upper  0 (dread ptr %_boundary.{{.*}}.0.[[# IDX_P1:]].upper
    q->a->i = p1;
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
    // CHECK-NEXT: assertge{{.*}}
    printf("%d\n", q->a->i[0]);
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
    // CHECK-NEXT: assertge{{.*}}
    printf("%d\n", *(q->a->i+1));
  }
  struct A aa;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 3 ]]{{$}}
  // CHECK-NOT: assertge{{.*}}
  // CHECK: dassign %_boundary.aa{{.*}}.1.[[# IDX_aa:]].upper 0 (dread ptr %_boundary.p.upper)
  aa.i = p;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 3 ]]{{$}}
  // CHECK-NOT: assertge{{.*}}
  // CHECK: dassign %_boundary.p1{{.*}}.0.[[# IDX_P1]].upper 0 (dread ptr %_boundary.aa{{.*}}.1.[[# IDX_aa]].upper)
  p1 = aa.i;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NEXT: assertge{{.*}}
  printf("%d\n", *(aa.i+2));
}

int main() {
  int a[5] = {1, 2, 3, 4, 5};
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK: dassign %_boundary.pa{{.*}}.upper 0 (add ptr (addrof ptr %a{{.*}}
  int *pa = a;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 3 ]]{{$}}
  // CHECK: assertge{{.*}}
  // CHECK: dassign %_boundary.pa1{{.*}}.upper 0 (add ptr (addrof ptr %a{{.*}}
  int *pa1 = a + 1;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NEXT: assertge{{.*}}
  printf("%d\n", *(pa+2));
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NEXT: assertge{{.*}}
  printf("%d\n", pa1[2]);
  struct A aa;
  struct B bb[5];
  struct B *q = bb;
  q->a = &aa;
  q->a->i = pa;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 3 ]]{{$}}
  // CHECK: callassertle{{.*}}
  // CHECK: callassertle{{.*}}
  foo(5, pa, q);

  return 0;
}
