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

// CHECK: [[# FILENUM:]] "{{.*}}/nonnull_check_ternary.c"

// expected-no-warning
#include <stddef.h>
int test2(int *a, int b) {
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: assertnonnull <&test2> (dread ptr %a)
  return (a != NULL) ? *a : b;
}

int test4(int *a, int b) {
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: assertnonnull <&test4> (dread ptr %a)
  return (a == NULL) ? b : *a;
}

int test5(int *x, int *y) __attribute__((nonnull(2))) {
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: assertnonnull <&test5> (dread ptr %x)
  return ( (x != NULL) ? (*x > 0) : (*y == 0) );
}

int test6(int *p, int *q) __attribute__((nonnull(2))) {
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: assertnonnull <&test6> (dread ptr %levVar{{.*}}
  return *((p != NULL) ? p : q);
}

int *test7(int *p, int *q) __attribute__((nonnull(2), returns_nonnull)) {
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: returnassertnonnull <&test7> (dread ptr %levVar{{.*}}
  return (p != NULL) ? p : q;
}

int test8(int a, int *p, int *q) __attribute__((nonnull)) {
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: assertnonnull <&test8> (dread ptr %levVar{{.*}}
  return *(a ? p : q);
}

int main() {
  return 0;
}
