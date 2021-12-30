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


// CHECK: [[# FILENUM:]] "{{.*}}/boundary_count_array_insert.c"

struct A {
  int info[5];
  short s;
};

#include <stdio.h>
int main() {
  int a[5] = {1, 2, 3, 4, 5};
  int i = 1;

  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NOT: assertge{{.*}}
  printf("%d\n", a[2]);  // skip checking in constant subscript
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NEXT: assertge{{.*}}
  printf("%d\n", a[i]);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NEXT: assertge{{.*}}
  printf("%d\n", *(a + 2));

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

  int b[3][2] = {{1,2},{3,4},{5,6}};
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NOT: assertge{{.*}}
  printf("%d\n", b[2][1]);  // skip checking in constant subscript
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NEXT: assertge{{.*}}
  printf("%d\n", b[i][1]);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NEXT: assertge{{.*}}
  printf("%d\n", *(*(b + 2) + 1));

  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK: dassign %_boundary.pb{{.*}}.upper 0 (add ptr (addrof ptr %b{{.*}}
  int (*pb)[2] = b;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NEXT: assertge{{.*}}
  printf("%d\n", *(*(pb + 2) + 1));

  int c[4][3][2] = {{{1,2},{3,4},{5,6}},{{1,2},{3,4},{5,6}},{{1,2},{3,4},{5,6}},{{1,2},{3,4},{5,6}}};
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NOT: assertge{{.*}}
  printf("%d\n", c[3][2][1]);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NEXT: assertge{{.*}}
  printf("%d\n", c[i][2][1]);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NEXT: assertge{{.*}}
  printf("%d\n", *(*(*(c + 3) + 2) + 1));

  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK: dassign %_boundary.pc{{.*}}.upper 0 (add ptr (addrof ptr %c{{.*}}
  int (*pc)[3][2] = c;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NEXT: assertge{{.*}}
  printf("%d\n", *(*(*(pc + 3) + 2) + 1));

  /* addrof array/sub-array */
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 3 ]]{{$}}
  // CHECK-NEXT: assertge{{.*}}
  // CHECK: dassign %_boundary.x{{.*}}.lower 0  (addrof ptr %a{{.*}}
  int *x = &a[i];  // i = 1
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NEXT: assertge{{.*}}
  printf("%d\n", *(x - 1));  // a[0]
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 3 ]]{{$}}
  // CHECK-NEXT: assertge{{.*}}
  // CHECK: dassign %_boundary.y{{.*}}.lower 0  (addrof ptr %c{{.*}}
  int *y = &c[i][2][1];
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NEXT: assertge{{.*}}
  printf("%d\n", *(y - 11));  // c[0][0][0]
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 4 ]]{{$}}
  // CHECK-NEXT: assertge{{.*}}
  // CHECK: assertge{{.*}}
  // CHECK: dassign %_boundary.z{{.*}}.lower 0  (addrof ptr %c{{.*}}
  int *z = c[i][2] + 1;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NEXT: assertge{{.*}}
  printf("%d\n", *(z - 11));  // c[0][0][0]

  /* array of struct */
  struct A aa = {{1,2,3,4,5}, 1};
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NEXT: assertge{{.*}}
  printf("%d\n", *(aa.info + 2));
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NEXT: assertge{{.*}}
  printf("%d\n", aa.info[i]);

  // CHECK: LOC [[# FILENUM]] [[# @LINE + 3 ]]{{$}}
  // CHECK: assertge{{.*}}
  // CHECK: dassign %_boundary.pinfo{{.*}}.lower 0  (addrof ptr %aa{{.*}} 1)
  int *pinfo = aa.info + 1;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NEXT: assertge{{.*}}
  printf("%d\n", pinfo[2]);

  struct A *paa = &aa;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NEXT: assertge{{.*}}
  printf("%d\n", paa->info[i]);
  return 0;
}
