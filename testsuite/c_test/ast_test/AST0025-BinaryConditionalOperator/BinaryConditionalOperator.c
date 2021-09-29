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
  int a = 0;
  // comparative conditioan: a < 10, get true val = (a < 10) = 1
  int b = (a < 10) ?: 1;
  printf("%d %d\n", a, b); // 0 1
  // noncomparative condition: a = 0, get false val
  b = a ?: 2;
  printf("%d %d\n", a, b); // 0 2

  b = a++ ?: 2;
  printf("%d %d\n", a, b); // 1 2
  // noncomparative conditioan: a = 10, get true val = 10
  a = 10;
  b = a++ ?: 2;
  printf("%d %d\n", a, b); // 11 10
  // comparative conditioan: a < 9, get false val
  b = (a < 9) ?: a++;
  printf("%d %d\n", a, b); // 12 11
  // multilevel conditional expr
  b = (a < 10) ?: (b < 9) ?: (a = 2, a++);
  printf("%d %d\n", a, b); // 3 2


  // double type noncomparative condition, get false val
  double c = 0.0;
  double d = c++ ?: 2.0;
  printf("%f %f\n", c, d); // 1.000000 2.000000
  // double type noncomparative condition, get true val
  c = 5.0;
  d = c ?: 2.0;
  printf("%f %f\n", c, d); // 5.000000 5.000000
  // double type comparative condition, get true val
  d = (c < 10.0) ?: 2.0;
  printf("%f %f\n", c, d); // 5.000000 1.000000
  // double type comparative condition, get false val
  d = (c < 1.0) ?: 2.0;
  printf("%f %f\n", c, d); // 5.000000 2.000000
  return 0;
}

