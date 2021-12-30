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

// CHECK: [[# FILENUM:]] "{{.*}}/nonnull_check_if_err.c"

#include <stddef.h>

/*  check if and */
int test5(int *a, int *b) {
  if (a == NULL && b == NULL) {
    return 0;
  } else {
    // expected-warning
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 3 ]]{{$}}
    // CHECK-NEXT: assertnonnull (dread ptr %a)
    // CHECK-NEXT: assertnonnull (dread ptr %b)
    return *a + *b;
  }
}

int main() {
  return 0;
}
