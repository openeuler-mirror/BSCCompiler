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
struct S {
  float f;
  int i[3];
};
union U {
  char ch;
  int i;
  char c[4];
};
union U g = {};
struct SU {
  int i;
  union U u;
};
int main() {
  struct S s0 = {};
  struct S s = { 3.14f, { 3, 4, 5, } };
  printf("%f, %d, %d, %d\n", s.f, s.i[0], s.i[1], s.i[2]);
  union U u = {{0}};
  union U u0 = {};
  union U u1 = { 2 };
  union U u2 = { 'a', 2 };
  printf("%d %d %d %d %c %d\n", u.i, u0.i, u1.c[2], u1.i, u2.ch, u2.c[1]);

  struct SU su = { 22, { 33 }};
  printf("%d\n", su.u.i);
  return 0;
}
