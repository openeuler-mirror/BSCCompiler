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

// CHECK: [[# FILENUM:]] "{{.*}}/nonnull_check_cast.c"

// expected-no-warning

#include <stddef.h>

struct Foo {
  unsigned int a;
};

int foo(void *f) {
  if (f == NULL)
    return 0;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NEXT: assertnonnull (dread ptr %f)
  if (((struct Foo *)f)->a > 0)
    return 1;

  return -1;
}

int main() {
  return 0;
}
