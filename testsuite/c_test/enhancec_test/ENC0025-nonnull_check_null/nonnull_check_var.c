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

struct A {
  int *i;
  int *j  __attribute__((nonnull));
};

int *gp;
int *gq __attribute__((nonnull));  // CHECK: [[# @LINE ]] error
int *gr __attribute__((nonnull)) = NULL;  // CHECK: [[# @LINE ]] error
int i = 3;
int *gw __attribute__((nonnull)) = &i; // CHECK-NOT: [[# @LINE ]] error

__attribute__((returns_nonnull))
int *foo(int *f __attribute__((nonnull))) {
  int i = 1;
  int *p = NULL;  // CHECK-NOT: [[# @LINE ]] error
  int *q __attribute__((nonnull));  // CHECK: [[# @LINE ]] error
  int *r __attribute__((nonnull)) = NULL;  // CHECK: [[# @LINE ]] error
  int *w __attribute__((nonnull)) = &i;  // CHECK-NOT: [[# @LINE ]] error
  w = NULL;  // CHECK: [[# @LINE ]] error
  w = f;  // CHECK-NOT: [[# @LINE ]] error
  int *x __attribute__((nonnull)) = f; // COM:CHECK-NOT: [[# @LINE ]] error
  p = f;  // CHECK-NOT: [[# @LINE ]] error
 return NULL;  // CHECK: [[# @LINE ]] error
}

void test_field() {
  struct A a1;
  a1.i = NULL;  // CHECK-NOT: [[# @LINE ]] error
  a1.j = NULL;  // CHECK: [[# @LINE ]] error

  struct A a2  = {NULL,   // CHECK-NOT: [[# @LINE ]] error
                  NULL};  // CHECK: [[# @LINE ]] error

  struct A *pa;
  pa = &a1;
  pa->i = NULL;  // CHECK-NOT: [[# @LINE ]] error
  pa->j = NULL;  // CHECK: [[# @LINE ]] error
  pa->i = &i;   // CHECK-NOT: [[# @LINE ]] error
}

struct A g_a1 = {NULL,  // CHECK-NOT: [[# @LINE ]] error
                 NULL};  // CHECK: [[# @LINE ]] error

struct A g_a2 = {&i, 
                 &i};  // CHECK-NOT: [[# @LINE ]] error

struct X {
  int i[2][2];
  char *j __attribute__((nonnull));
};

struct B {
  int i;
  double j;
  struct X a;
};

union U {
  int i;
  char *c __attribute__((nonnull));
};

union U1 {
 char *c __attribute__((nonnull));
 int i;
};

struct D {
 char *k  __attribute__((nonnull));
};

struct C {
  int i;
  struct X a;
  struct D d[3];
  int j;
  union U1 u1;
};

struct X g_x = {{{1,2},{1,2}}, NULL};  // CHECK: [[# @LINE ]] error
struct X g_x1 = {.j = NULL};  // CHECK: [[# @LINE ]] error
struct B g_b = {1, 2.1, {{{1,2},{1,2}}, NULL}};  // CHECK: [[# @LINE ]] error
struct B g_b_arr[2] = { {1, 2.1, { {{1,2},{1,2}}, "123456789"} },  // CHECK-NOT: [[# @LINE ]] error
                        {1, 2.2, { {{1,2},{1,2}}, NULL} } };  // CHECK: [[# @LINE ]] error
union U g_u = {.c=NULL};  // CHECK: [[# @LINE ]] error
struct C g_c_arr[3] = { {1, { {{1,2},{1,2}}, NULL}, {"1", NULL, NULL}, 2, NULL },  // CHECK-COUNT-4: [[# @LINE ]] error
                        {1, { {{1,2},{1,2}}, "123456"}, {"1", "2", "3"}, 2, NULL },   // CHECK: [[# @LINE ]] error
                        {1, { {{1,2},{1,2}}, "123456"}, {"1", "2", "3"}, 2, "123456" } };  // CHECK-NOT: [[# @LINE ]] error


int main() {
  foo(NULL);  // CHECK: [[# @LINE ]] error
  
  struct C c_arr[3] = { {1, { {{1,2},{1,2}}, NULL}, {"1", NULL, NULL}, 2, NULL },  // CHECK-COUNT-4: [[# @LINE ]] error
                          {1, { {{1,2},{1,2}}, "123456"}, {"1", "2", "3"}, 2, NULL },   // CHECK: [[# @LINE ]] error
                          {1, { {{1,2},{1,2}}, "123456"}, {"1", "2", "3"}, 2, "123456" } };  // CHECK-NOT: [[# @LINE ]] error
  return 0;
}
