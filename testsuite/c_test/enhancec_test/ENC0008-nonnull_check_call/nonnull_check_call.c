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

// CHECK: [[# FILENUM:]] "{{.*}}/nonnull_check_call.c"

#include <stddef.h>
void foo_nullable(int *a) {
  return;
}
void foo_nonnull(int *a) __attribute__((nonnull)) {
  return;
}

void call(int *a, int *b) __attribute__((nonnull(2))) {
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NOT: assertnonnull (dread ptr %a)
  foo_nullable(a);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NEXT: callassertnonnull <&foo_nonnull, 0, &call> (dread ptr %a)
  foo_nonnull(a); // expected-warning
  if (a) {
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
    // CHECK-NEXT: callassertnonnull <&foo_nonnull, 0, &call> (dread ptr %a)
    foo_nonnull(a);
  }
  foo_nullable(b);
  foo_nonnull(b);

  int x;
  foo_nullable(&x);
  foo_nonnull(&x);

  int *c = a;
  foo_nullable(c);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NEXT: callassertnonnull <&foo_nonnull, 0, &call> (dread ptr %c{{.*}}
  foo_nonnull(c); // expected-warning

  int *d = b;
  foo_nullable(d);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NEXT: callassertnonnull <&foo_nonnull, 0, &call> (dread ptr %d{{.*}}
  foo_nonnull(d);

  int *e = &x;
  foo_nullable(e);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NEXT: callassertnonnull <&foo_nonnull, 0, &call> (dread ptr %e{{.*}}
  foo_nonnull(e);
}

int main() {
  return 0;
}
