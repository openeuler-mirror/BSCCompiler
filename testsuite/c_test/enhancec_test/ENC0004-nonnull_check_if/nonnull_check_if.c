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

// CHECK: [[# FILENUM:]] "{{.*}}/nonnull_check_if.c"

// expected-no-warning
#include <stddef.h>

/* check if */
int test1(int *x) {
  if (x != NULL) {
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
    // CHECK-NEXT: assertnonnull <&test1> (dread ptr %x)
    return *x;
  }
  return 0;
}

int test2(int *x) {
  if (x == NULL) {
    return 0;
  } else {
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
    // CHECK-NEXT: assertnonnull <&test2> (dread ptr %x)
    return *x;
  }
}

int test3(int *x) {
  if (!x) {
    return 0;
  }
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NEXT: assertnonnull <&test3> (dread ptr %x)
  return *x;
}

/*  check if or */
int test4(int *a, int *b) {
  if (a == NULL || b == NULL) {
    return 0;
  } else {
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 3 ]]
    // CHECK-NEXT: assertnonnull <&test4> (dread ptr %a)
    // CHECK: assertnonnull <&test4> (dread ptr %b)
    return *a + *b;
  }
}

/* chenck if and */
int test5(int *a, int b) {
  if (a != NULL && b > 0) {
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
    // CHECK-NEXT: assertnonnull <&test5> (dread ptr %a)
    return *a + b;
  }
  return b;
}

/* check if nested */
int test6(int *x, int b) {
  if (x != NULL) {
    if (b > 0) {
      // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
      // CHECK-NEXT: assertnonnull <&test6> (dread ptr %x)
      return *x + b;
    }
  }
  return 0;
}

/* check multi_if */
int *bar(int *b) {
  return b;
}

int test7(int *a) {
  int *b;
  if (a == NULL) return 0;

  b = bar(b);

  if (b == NULL) return 1;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 3 ]]
  // CHECK-NEXT: assertnonnull <&test7> (dread ptr %a)
  // CHECK: assertnonnull <&test7> (dread ptr %b{{.*}}
  return *a + *b;
}



int main() {
  return 0;
}
