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

__attribute__((returns_byte_count_index(1)))
int *func(int size, int *p __attribute__((count(10)))) {
  struct A a;
  return a.fa;
}

int* (*func1)(int*, int*) __attribute__((byte_count(10, 1)));

int *Test(int *p __attribute__((byte_count(10))), int *q) {
  return p;
}
int n;
void addrof_check_fa_index_warning() {
  struct A a;
  a.len = 10;
  // CHECK: [[# @LINE + 2]] warning: can't prove the pointer >= the lower bounds when accessing the memory
  // CHECK: [[# @LINE + 1]] warning: can't prove the pointer < the upper bounds when accessing the memory
  a.fa[n] = 1;
}

void iaddrof_check_fa_index_warning() {
  struct A *a = (struct A *)malloc(sizeof(struct A) + 10);
  a->len = 10;
  // CHECK: [[# @LINE + 2]] warning: can't prove the pointer >= the lower bounds when accessing the memory
  // CHECK: [[# @LINE + 1]] warning: can't prove the pointer < the upper bounds when accessing the memory
  a->fa[n] = 1;
}

static inline void addrof_check_fa_index_warning_inline() {
  struct A a;
  a.len = 10;
  // CHECK: [[# @LINE + 2]] warning: can't prove the pointer >= the lower bounds when accessing the memory and inlined to main
  // CHECK: [[# @LINE + 1]] warning: can't prove the pointer < the upper bounds when accessing the memory and inlined to main
  a.fa[n] = 1;
}

static inline void iaddrof_check_fa_index_warning_inline() {
  struct A *a = (struct A *)malloc(sizeof(struct A) + 10);
  a->len = 10;
  // CHECK: [[# @LINE + 2]] warning: can't prove the pointer >= the lower bounds when accessing the memory and inlined to main
  // CHECK: [[# @LINE + 1]] warning: can't prove the pointer < the upper bounds when accessing the memory and inlined to main
  a->fa[n] = 1;
}

int *addrof_check_call_func_warning() {
  struct A a;
  // CHECK: [[# @LINE + 1]] warning: can't prove pointer's bounds match the function func declaration for the 2nd argument
  int *p = func(1, a.fa);
  a.len = 10;
  // CHECK-NOT: [[# @LINE + 1]] warning: can't prove pointer's bounds match the function func declaration for the 2nd argument
  p = func(1, a.fa);
  return p;
}

int *iaddrof_check_call_func_warning() {
  struct A *a = (struct A *)malloc(sizeof(struct A) + 10);
  // CHECK: [[# @LINE + 1]] warning: can't prove pointer's bounds match the function func declaration for the 2nd argument
  int *p = func(1, a->fa);
  a->len = 10;
  // CHECK-NOT: [[# @LINE + 1]] warning: can't prove pointer's bounds match the function func declaration for the 2nd argument
  p = func(1, a->fa);
  return p;
}

static inline int *addrof_check_call_func_warning_inline() {
  struct A a;
  // CHECK: [[# @LINE + 1]] warning: can't prove pointer's bounds match the function func declaration for the 2nd argument when inlined to main
  int *p = func(1, a.fa);
  return p;
}

static inline int *iaddrof_check_call_func_warning_inline() {
  struct A *a = (struct A *)malloc(sizeof(struct A) + 10);
  // CHECK: [[# @LINE + 1]] warning: can't prove pointer's bounds match the function func declaration for the 2nd argument when inlined to main
  int *p = func(1, a->fa);
  return p;
}

__attribute__((returns_byte_count_index(1)))
int *addrof_check_return_warning(int size) {
  struct A a;
  // CHECK: [[# @LINE + 1]] warning: can't prove return value's bounds match the function declaration for addrof_check_return_warning
  return a.fa;
}

__attribute__((returns_byte_count_index(1)))
int *iaddrof_check_return_warning(int size) {
  struct A *a = (struct A *)malloc(sizeof(struct A) + 10);
  // CHECK: [[# @LINE + 1]] warning: can't prove return value's bounds match the function declaration for iaddrof_check_return_warning
  return a->fa;
}

__attribute__((returns_byte_count_index(1)))
static inline int *addrof_check_return_warning_inline(int size) {
  struct A a;
  // CHECK: [[# @LINE + 1]] warning: can't prove return value's bounds match the function declaration for addrof_check_return_warning_inline when inlined to main
  return a.fa;
}

__attribute__((returns_byte_count_index(1)))
static inline int *iaddrof_check_return_warning_inline(int size) {
  struct A *a = (struct A *)malloc(sizeof(struct A) + 10);
  // CHECK: [[# @LINE + 1]] warning: can't prove return value's bounds match the function declaration for iaddrof_check_return_warning_inline when inlined to main
  return a->fa;
}

int *addof_check_calcu_warning() {
  struct A a;
  // CHECK: [[# @LINE + 1]] warning: can't prove the pointer < the upper bounds after calculation
  int *p = a.fa + 1;
  return p;
}

int *iaddof_check_calcu_warning() {
  struct A *a = (struct A *)malloc(sizeof(struct A) + 10);
  // CHECK: [[# @LINE + 1]] warning: can't prove the pointer < the upper bounds after calculation
  int *p = a->fa + 1;
  return p;
}

static inline int *addof_check_calcu_warning_inline() {
  struct A a;
  // CHECK: [[# @LINE + 1]] warning: can't prove the pointer < the upper bounds after calculation when inlined to main
  int *p = a.fa + 1;
  return p;
}

static inline int *iaddof_check_calcu_warning_inline() {
  struct A *a = (struct A *)malloc(sizeof(struct A) + 10);
  // CHECK: [[# @LINE + 1]] warning: can't prove the pointer < the upper bounds after calculation when inlined to main
  int *p = a->fa + 1;
  return p;
}

void addof_check_assign_warning() {
  struct A a;
  int *t __attribute__((count((10))));
  //  CHECK: [[# @LINE + 1]] warning: can't prove l-value's upper bounds <= r-value's upper bounds
  t = a.fa;
}

void iaddof_check_assign_warning() {
  struct A *a = (struct A *)malloc(sizeof(struct A) + 10);
  int *t __attribute__((count((10))));
  //  CHECK: [[# @LINE + 1]] warning: can't prove l-value's upper bounds <= r-value's upper bounds
  t = a->fa;
}

static inline void addof_check_assign_warning_inline() {
  struct A a;
  int *t __attribute__((count((10))));
  //  CHECK: [[# @LINE + 1]] warning: can't prove l-value's upper bounds <= r-value's upper bounds when inlined to main
  t = a.fa;
}

static inline void iaddof_check_assign_warning_inline() {
  struct A *a = (struct A *)malloc(sizeof(struct A) + 10);
  int *t __attribute__((count((10))));
  //  CHECK: [[# @LINE + 1]] warning: can't prove l-value's upper bounds <= r-value's upper bounds when inlined to main
  t = a->fa;
}

void addof_check_funcPtr_warning() {
  struct A a;
  func1 = &Test;
  //  CHECK: [[# @LINE + 1]] warning: can't prove pointer's bounds match the function function_pointer declaration for the 1st argument
  (void)(*func1)(a.fa, a.fa);
}

void iaddof_check_funcPtr_warning() {
  struct A *a = (struct A *)malloc(sizeof(struct A) + 10);
  func1 = &Test;
  //  CHECK: [[# @LINE + 1]] warning: can't prove pointer's bounds match the function function_pointer declaration for the 1st argument
  (void)(*func1)(a->fa, a->fa);
}

static inline void addof_check_funcPtr_warning_inline() {
  struct A a;
  func1 = &Test;
  //  CHECK: [[# @LINE + 1]] warning: can't prove pointer's bounds match the function function_pointer declaration for the 1st argument when inlined to main
  (void)(*func1)(a.fa, a.fa);
}

static inline void iaddof_check_funcPtr_warning_inline() {
  struct A *a = (struct A *)malloc(sizeof(struct A) + 10);
  func1 = &Test;
  //  CHECK: [[# @LINE + 1]] warning: can't prove pointer's bounds match the function function_pointer declaration for the 1st argument when inlined to main
  (void)(*func1)(a->fa, a->fa);
}

int main() {
  int n = 5;
  addrof_check_fa_index_warning_inline();
  iaddrof_check_fa_index_warning_inline();
  (void)addrof_check_call_func_warning_inline();
  (void)iaddrof_check_call_func_warning_inline();
  (void)addrof_check_return_warning_inline(n);
  (void)iaddrof_check_return_warning_inline(n);
  (void)addof_check_calcu_warning_inline();
  (void)iaddof_check_calcu_warning_inline();
  addof_check_assign_warning_inline();
  iaddof_check_assign_warning_inline();
  addof_check_funcPtr_warning_inline();
  iaddof_check_funcPtr_warning_inline();

  return 0;
}
