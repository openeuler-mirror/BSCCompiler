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
  int c;
  int d;
  struct A singleA;
  int inB[2];
};

int main() {
  struct B s1;
  s1.inB[1] = 15;
  printf("%d\n", s1.inB[1]);

  struct A saa[4] = {{1,2},{3,4},{5,6},{7,8}};
  printf("%d\n",saa[0].a);
  printf("%d\n",saa[1].b);
  saa[0].a = 10;
  printf("%d\n",saa[0].a);
  saa[1].b = 20;
  saa[0].a = 30;
  printf("%d\n",saa[0].a);
  printf("%d\n",saa[1].b);
  printf("%d\n",saa[2].a);
  printf("%d\n",saa[2].b);
  printf("%d\n",saa[3].a);
  printf("%d\n",saa[3].b);
  return 0;
}
