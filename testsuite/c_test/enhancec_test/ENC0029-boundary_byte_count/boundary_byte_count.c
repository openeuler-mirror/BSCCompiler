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


// CHECK: [[# FILENUM:]] "{{.*}}/boundary_byte_count.c"

#include <stdio.h>

__attribute__((returns_byte_count("len"), byte_count("len", 1)))
int *getBoundaryPtr(int *ptr, int len) {
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NEXT: assertle{{.*}}
  return ptr;
}

int *getPtr(int *ptr) {
 return ptr;
}


struct A {
  int *i;
};

struct B {
  struct A *a;
};

int main() {
  int a[5] = {1, 2, 3, 4, 5};
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 4 ]]{{$}}
  // CHECK: assertle{{.*}}
  // CHECK: dassign %_boundary.retVar{{.*}}.lower 0 (dread ptr %retVar{{.*}}
  // CHECK: dassign %_boundary.pa{{.*}}.lower 0 (dread ptr %_boundary.retVar{{.*}}.lower
  int *pa = getBoundaryPtr(a, 4 * 4);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 5 ]]{{$}}
  // CHECK: assertle{{.*}}
  // CHECK: dassign %_boundary.retVar{{.*}}.lower 0 (dread ptr %retVar{{.*}}
  // CHECK: assertge{{.*}}
  // CHECK: dassign %_boundary.pa1{{.*}}.lower 0 (dread ptr %_boundary.retVar{{.*}}.lower
  int *pa1 = getBoundaryPtr(a, 4 * 4) + 1;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NEXT: assertge{{.*}}
  printf("%d\n", *(pa+2));
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NEXT: assertge{{.*}}
  printf("%d\n", pa1[2]);

  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NOT: dassign %_boundary.pa2{{.*}}.lower{{.*}}
  int *pa2 = getPtr(a);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NOT: assertge{{.*}}
  printf("%d\n", pa2[2]);

  struct A aa;
  struct B bb[4];
  struct B *q = bb;
  q->a = &aa;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 4 ]]{{$}}
  // CHECK: assertle{{.*}}
  // CHECK: dassign %_boundary.retVar{{.*}}.lower 0 (dread ptr %retVar{{.*}}
  // CHECK: dassign %_boundary.A_i{{.*}}.lower 0 (dread ptr %_boundary.retVar{{.*}}.lower
  q->a->i = getBoundaryPtr(a, 4 * 4);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NEXT: assertge{{.*}}
  printf("%d\n", q->a->i[3]);
  return 0;
}
