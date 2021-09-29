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

int main() {
  int a[3] = {1, 2, 3};
  double f[3] = {1.0, 2.1, 3.14};
  int b[10];
  b[2] = 10;
  b[3] = 100;

  char c[5];
  c[3] = 'a';
  c[4] = 'b';

  printf("%d", b[2]);
  printf("%d", b[3]);
  printf("%c", c[3]);
  printf("%c\n", c[4]);
  printf("%d%d%d", a[0], a[1], a[2]);
  int a_ = a[0] + a[1] + a[2];
  printf("%d", a_);
  printf("%lf%lf%lf", f[0], f[1], f[2]);
  // fail cases
  // double f[3] = {1, 2.1, 3.14};
  // printf("%d%d%d", a[0], a[1], a[1+1]);
  // printf("%d%d%d", a[0] + a[1] + a[2]);
  // double f_ = f[0] + f[1] + f[2];
  // printf("\n%d%d%d",*(a), *(a+1), *(a+2));
  // printf("%d%d%d",*&a[0], *&a[1+1], *&a[2]);
  // printf("\n%f%f%f",*&f[0], *&f[1+1], *&f[2]);
  return 0;
}
