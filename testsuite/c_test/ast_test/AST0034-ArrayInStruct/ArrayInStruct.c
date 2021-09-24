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
struct C {
 char f;
};
struct B {
 char c;
 char d;
 struct C inc;
};
struct A {
  int a;
  int b;
  char inArr[2][2];
  double inArr1[2][2];
  struct B inb;
};

int main() {
  struct A sa = {1, 2, {'a', 'b','c'},{2.5, 6.8,8.5,45.2},{'d','f',{'g'}}};
  char c1 = sa.inArr[0][1];
  char c2 = sa.inArr[1][0];
  double c11 = sa.inArr1[0][1];
  double c21 = sa.inArr1[1][0];
  double c22 = sa.inArr1[1][1];
  sa.inArr1[1][1] = 22.2;
  double c23 = sa.inArr1[1][1];
  printf("%c\n",c1);
  printf("%c\n",c2);
  printf("%f\n",c11);
  printf("%f\n",c21);
  printf("%f\n",c22);
  printf("%f\n",sa.inArr1[1][1]);
  printf("%f\n",c23);
  printf("%c\n",sa.inb.inc.f);
  return 0;
}

