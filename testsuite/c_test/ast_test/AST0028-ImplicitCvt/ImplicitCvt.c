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
#include <limits.h>

long a = 65538;
int b = 23;

int main() {
  if (ULONG_MAX >= 2147483647) {
    printf("true");
  }
  long c = 655351111;
  int d = 23;
  if (c > d) {
    printf("true");
  }
  if (a > b) {
    printf("true");
  }
  float e = 2.2f;
  double f = 2.3;
  if (f > e) {
    printf("true");
  }
  int i = e;
  int j = f;
  int h = 0;
  h = e;
  printf("%f", e);
  printf("%f", f);
  printf("%ld", a);
  printf("%d", i);
  printf("%d", j);
  printf("%d", h);
  return 0;
}
