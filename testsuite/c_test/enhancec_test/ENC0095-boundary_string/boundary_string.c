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

#include <stdio.h>

char *g_s = "12345";
char g_s_arr[10] = "12345";
char g_s_arr1[] = "12345";
char *g_s_arr2[2] = {"12345", "123456"};

struct A {
  int i[2][2];
  char *j;
};

struct B {
  int i;
  double j;
  struct A a;
};

union U {
  int i;
  char *c;
};

union U1 {
 char *c;
 int i;
};

struct C {
  int i;
  struct A a;
  union U1 u1;
};

#define STR "123456"

struct A g_a = {{{1,2},{1,2}}, "12345678"};
struct A g_a1 = {.j = "12345678"};
struct B g_b = {1, 2.1, {{{1,2},{1,2}}, "123456789"}};
struct B g_b_arr[2] = { {1, 2.1, { {{1,2},{1,2}}, "123456789"} }, 
                        {1, 2.2, { {{1,2},{1,2}}, "123456"} } };
union U g_u = {.c="12345"};
struct C g_c_arr[2] = { {1, { {{1,2},{1,2}}, "123456789"}, "1234567" },
                        {1, { {{1,2},{1,2}}, "123456"}, "12345678" }};


int get(char *buf __attribute__((count(5)))) {
  return buf[1];
}

void test() {
  int res = 0;
  res += get(STR);  // CHECK-NOT: [[# @LINE ]] error

  char *str = "abcdefg";
  char str_arr[10] = "abcdefg";
  char str_arr1[] = "abcdefg";
  res += get(str);  // CHECK-NOT: [[# @LINE ]] error
  res += get(str_arr);  // CHECK-NOT: [[# @LINE ]] error
  res += get(str_arr1);  // CHECK-NOT: [[# @LINE ]] error
  char *str1 = str;
  res += get(str1);  // CHECK-NOT: [[# @LINE ]] error

  res += get(g_s);  // CHECK-NOT: [[# @LINE ]] error
  res += get(g_s_arr);  // CHECK-NOT: [[# @LINE ]] error
  res += get(g_s_arr1);  // CHECK-NOT: [[# @LINE ]] error
  res += get(g_s_arr2[1]);  // CHECK-NOT: [[# @LINE ]] error

  res += get(g_a.j);  // CHECK-NOT: [[# @LINE ]] error
  res += get(g_b.a.j);  // CHECK-NOT: [[# @LINE ]] error
  res += get(g_b_arr[1].a.j);  // CHECK-NOT: [[# @LINE ]] error

  res += get(g_u.c);  // CHECK-NOT: [[# @LINE ]] error
  res += get(g_c_arr[1].a.j);  // CHECK-NOT: [[# @LINE ]] error
  res += get(g_c_arr[1].u1.c);  // CHECK-NOT: [[# @LINE ]] error

  char *l = g_s_arr;
  res += get(l);  // CHECK-NOT: [[# @LINE ]] error

  struct B b = {1, 2.1, {{{1,2},{1,2}}, "123456789"}};
//  struct B b_arr[2] = { {1, 2.1, { {1,2}, "123456789"}},
//                          {1, 2.2, { {1,2}, "123456"}}};
  struct B b_arr[2];
  res += get(b.a.j);  // CHECK-NOT: [[# @LINE ]] error
  b_arr[1].a.j = "abcdf";
  res += get(b_arr[1].a.j);  // CHECK-NOT: [[# @LINE ]] error

  union U u = {.c="12345"};
  res += get(u.c);  // CHECK-NOT: [[# @LINE ]] error

  // char *i;
  // res += get(i);  // COM:CHECK: [[# @LINE ]] error
  
}

int main() {
  return 0;
}
