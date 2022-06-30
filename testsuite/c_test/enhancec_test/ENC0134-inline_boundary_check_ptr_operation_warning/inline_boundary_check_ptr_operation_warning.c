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

void func() {
  int *p __attribute__((count(5)));
  int *q = p - 5; // CHECK: [[# @LINE ]] warning: the pointer < the lower bounds after calculation
}

void func1() {
  int *p __attribute__((count(5)));
  int *q = p + 5; // CHECK: [[# @LINE ]] warning: the pointer >= the upper bounds after calculation
}

static inline void func2() {
  int *p __attribute__((count(5)));
  int *q = p + 5; // CHECK: [[# @LINE ]] warning: the pointer >= the upper bounds after calculation when inlined to main
}

static inline void func3() {
  int *p __attribute__((count(5)));
  int *q = p - 5; // CHECK: [[# @LINE ]] warning: the pointer < the lower bounds after calculation when inlined to main
}

int main() {
  func();
  func1();
  func2();
  func3();
  return 0;
}

