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

struct A {
 int a;
 int b;
};
struct B {
  struct A a;
  struct A *pa;
  int d;
  char c;
  double f[1];
};

int main() {
  struct A a;
  struct A *pa = &a;
  a.a = 1;
  a.b = 2;
  printf("%d,", a.a);
  printf("%d,", a.b);
  printf("%d,", pa->a);
  printf("%d\n", pa->b);

  // add more cases
  struct B b;
  struct B *pb = &b;
  b.pa = &a;
  b.a.a = 11;
  b.a.b = 22;
  b.d = -32766;
  b.c = 'b';
  printf("%c", pb->c);
  printf("%d", b.d);
  b.f[0] = 1.1;
  printf("%d\n", pb->a.a + pb->a.b + pb->d);
  b.pa = &b.a;
  printf("%d%d", b.pa->a, b.pa->b);
  printf("%d", b.a.a + b.a.b + b.d);

  struct C {
    struct B b;
    struct B *pb;
  } c;
  c.pb = &c.b;
  struct C *pc = &c;
  c.b = b;
  pc->b = b;
  c.b.pa = &a;
  c.b.a.a = 11;
  c.b.a.b = 22;
  c.b.d = -32766;
  c.b.c = 'b';
  // c.b.f[0] = 1.11;                     // fail case
  c.b.pa = &c.b.a;
  printf("%c%c%c%c", c.b.c, pc->b.c, c.pb->c, pc->pb->c);
  printf("%d%d%d%d", c.b.pa->a, c.b.a.a, c.pb->a.a, c.pb->pa->a);
  printf("%d%d%d%d", pc->b.pa->a, pc->b.a.a, pc->pb->a.a, pc->pb->pa->a);
  return 0;
}
