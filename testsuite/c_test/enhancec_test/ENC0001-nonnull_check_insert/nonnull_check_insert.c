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


// CHECK: [[# FILENUM:]] "{{.*}}/ENC0001-nonnull_check_insert/nonnull_check_insert.c"

#include <stdio.h>

struct A {
  int *i;
};

struct B {
  struct A *a;
};


__attribute__((returns_nonnull, nonnull))
int *getNonnullPtr(int *ptr) {
  return ptr;
}

int *getNullablePtr(int *ptr) {
 return ptr;
}


__attribute__((nonnull))
void foo(int a, int *p, struct B *q) {
  int *p1;
  // pointer assignment
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NOT: assertnonnull (dread ptr %p)
  p1 = p;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NEXT: assertnonnull <&foo> (dread ptr %p1_{{.*}}
  p = p1;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 4 ]]
  // CHECK-NOT: assertnonnull (dread ptr %q)
  // CHECK-NEXT: assertnonnull <&foo> (iread ptr <* <$B>> 1 (dread ptr %q))
  // CHECK: assignassertnonnull <&foo> (iread ptr <* <$A>> 1 (iread ptr <* <$B>> 1 (dread ptr %q)))
  p = q->a->i;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 4 ]]
  // CHECK-NEXT: callassertnonnull <&getNonnullPtr, 0, &foo> (dread ptr %p1_{{.*}}
  // CHECK: callassigned &getNonnullPtr{{.*}}
  // CHECK-NOT: assertnonnull <&foo> (dread ptr %retVar_{{.*}}
  p = getNonnullPtr(p1);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 3 ]]
  // CHECK-NEXT: callassigned &getNullablePtr{{.*}}
  // CHECK: assertnonnull <&foo> (dread ptr %retVar_{{.*}}
  p = getNullablePtr(p1);

  // *ptr
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NEXT: assertnonnull <&foo> (dread ptr %p1_{{.*}}
  printf("*p=%d, *p1=%d\n", *p, *p1);

  // ptr->a
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 4 ]]
  // CHECK-NOT: assertnonnull (dread ptr %q)
  // CHECK-NEXT: assertnonnull <&foo> (iread ptr <* <$B>> 1 (dread ptr %q))
  // CHECK: assertnonnull <&foo> (iread ptr <* <$A>> 1 (iread ptr <* <$B>> 1 (dread ptr %q)))
  printf("*(q->a->i)=%d\n", *(q->a->i));
}

int main() {
  int a = 1;
  int *p = &a;

  struct A aa;
  struct B bb;
  struct B *q = &bb;
  q->a = &aa;
  q->a->i = p;

  // pointer caller
  foo(1, p, q);

  return 0;
}
