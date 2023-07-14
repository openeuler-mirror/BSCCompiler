/*
 * Copyright (c) [2023-2023] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include <stdlib.h>
struct A {
  int len;
  int fa[] __attribute__((count("len")));
};

__attribute__((returns_count(10)))
static inline int *addrof_check_return_err() {
  struct A a;
  a.len = 9;
  // CHECK: [[# @LINE + 1]] error: return value's bounds does not match the function declaration for addrof_check_return_err when inlined to main
  return a.fa;
}

int main() {
  addrof_check_return_err();
  return 0;
}
