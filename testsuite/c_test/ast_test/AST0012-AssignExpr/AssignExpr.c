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

int main() {
  int a = 0;
  int b = 1;
  int c = 2;
  int *pb = &b;
  int *pc = &c;
  struct A sa;
  struct A sb;
  struct A *psb = &sb;
  sa.a = 0;
  sa.b = 0;
  sb.a = 0;
  sb.b = 0;
  printf("%d,", a);
  a = 1;
  printf("%d,", a);
  printf("%d,", *pb);
  pb = pc;
  printf("%d,", *pc);
  *pc = 4;
  printf("%d,", *pc);
  printf("%d,", sa.a);
  printf("%d,", sa.b);
  sa.a = 1;
  sa.b = 2;
  printf("%d,", sa.a);
  printf("%d,", sa.b);
  printf("%d,", psb->a);
  printf("%d,", psb->b);
  psb->a = 3;
  psb->b = 4;
  printf("%d,", psb->a);
  printf("%d,", psb->b);

  int arr[] = {1,2,3,4,5};
  int *parr = arr;
  printf("%d,", *(arr + 1));
  printf("%d,", *(1 + arr));
  printf("%d\n", *(1 + arr - 2 + 1));
  return 0;
}
