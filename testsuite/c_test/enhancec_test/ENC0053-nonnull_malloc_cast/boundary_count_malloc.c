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

struct A {
  int *i __attribute__((nonnull));
  int *j;
};

struct B {
  int *i;
  int *j;
};

struct C {
  int *k;
  struct A a;
};

struct B* cvt(struct A *a) {
  return (struct B*)a;
}

struct A* cvt1(struct B *b) {
  return (struct A*)b;  // CHECK: [[# @LINE ]] error
}


__attribute__((noinline))
int get(int *buf __attribute__((count(10)))) {
  return buf[9];
}

int main() {
  struct A *a = (struct A*)malloc(sizeof(struct A)); // CHECK: [[# @LINE ]] error
  bzero((void*)a, sizeof(struct A));  // CHECK: [[# @LINE ]] error
  memset(a, 0, sizeof(struct A));  // CHECK: [[# @LINE ]] error

  struct B *b = (struct B*)malloc(sizeof(struct B));  // CHECK-NOT: [[# @LINE ]] error
  bzero((void*)b, sizeof(struct B));  // CHECK-NOT: [[# @LINE ]] error
  memset(b, 0, sizeof(struct B));  // CHECK-NOT: [[# @LINE ]] error

  struct C *c = malloc(sizeof(struct C));  // CHECK: [[# @LINE ]] error
  bzero((void*)c, sizeof(struct C));  // CHECK: [[# @LINE ]] error
  memset(c, 0, sizeof(struct C));  // CHECK: [[# @LINE ]] error

  b = cvt(a);  // CHECK-NOT: [[# @LINE ]] error
  b = cvt(b);  // CHECK: [[# @LINE ]] error
  a = cvt(a);  // CHECK: [[# @LINE ]] error


  return 0;
}
