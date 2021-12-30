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


#include <stddef.h>

//_Noreturn int panic(int err) { exit(err); }
__attribute__((noreturn))
int panic(int err) { 
  exit(err);
}

int no_return(int *x) {
  if (x == NULL) {
    exit(-1); //  CHECK-NOT: [[# @LINE ]] warning
  }
  return *x;  // COM:CHECK-NOT: [[# @LINE ]] warning
}

int no_return1(int *x) {
  if (x == NULL) {
    panic(1);
  }
  return *x;  // COM:CHECK-NOT: [[# @LINE ]] warning
}

int main() {
  return 0;
}
