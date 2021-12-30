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

// CHECK: [[# FILENUM:]] "{{.*}}/nonnull_return_check_insert.c"

#include <stdio.h>

__attribute__((returns_nonnull, nonnull))
int *getNonnullPtr(int *ptr) {
  return ptr;
}

int *getNullablePtr(int *ptr) {
 return ptr;
}

__attribute__((returns_nonnull))
int* test(int i) {
  int *ptr = &i;
  if (i) {
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 4 ]]{{$}}
    // CHECK-NEXT: callassertnonnull <&getNonnullPtr, 0> (dread ptr %ptr_{{.*}}
    // CHECK-NEXT: callassigned &getNonnullPtr{{.*}}
    // CHECK-NOT: assertnonnull (dread ptr %retVar_{{.*}}
    return getNonnullPtr(ptr);
  } else {
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 3 ]]{{$}}
    // CHECK-NEXT: callassigned &getNullablePtr{{.*}}
    // CHECK-NEXT: returnassertnonnull <&test> (dread ptr %retVar_{{.*}}
    return getNullablePtr(ptr);
  }
}

int main() {
 int *i = test(1);
 return 0;
}

