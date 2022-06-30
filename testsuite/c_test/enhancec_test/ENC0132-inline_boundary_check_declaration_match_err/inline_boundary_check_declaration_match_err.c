/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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



#include <stdio.h>
#include <stdlib.h>

__attribute__((count(10, 1)))
void func1(int* p);
static inline void func2() {
  int *p = (int*)malloc(sizeof(int));
  func1(p); // CHECK: [[# @LINE ]] error: the pointer's bounds does not match the function func1 declaration for the 1st argument when inlined to main
}

int main() {
  func2();
  return 0;
}

