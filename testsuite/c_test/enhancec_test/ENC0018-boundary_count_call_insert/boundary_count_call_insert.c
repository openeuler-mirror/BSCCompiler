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


// CHECK: [[# FILENUM:]] "{{.*}}/boundary_count_call_insert.c"

#include <stdio.h>

int get(int *buf __attribute__((count(10)))) {
  return buf[1];
}

void foo(int w, int x, int y, int z) {
  int res;
  int A[10] = {0};
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK: callassertle{{.*}}
  res = get(A);

  int B[20] = {0};
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK: callassertle{{.*}}
  get(B);

  int *J = A;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK: callassertle{{.*}}
  get(J);

  int *K = B;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK: callassertle{{.*}}
  get(K);
}

struct S {
  char info[2];
  long a, b;
  char tag[2];
};

struct A {
  char info[2];
  short s;
};

__attribute__((count(10)))
void f(char *buf) {
  return;
}

void bar() {
  char array[12];
  char array2d[3][12];
  struct A a;

  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK: assertle{{.*}}
  f(&array[0]);
  char *x = &array[0];
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK: assertle{{.*}}
  f(x);

  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK: assertle{{.*}}
  f(array2d[0]);
  char *y = array2d[0];
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK: assertle{{.*}}
  f(y);

  char *z = a.info;

  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK: assertle{{.*}}
  f(&array[1]);
  char *w = &array[3];
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK: assertle{{.*}}
  f(&array2d[1][3]);
}

int main() {
  return 0;
}
