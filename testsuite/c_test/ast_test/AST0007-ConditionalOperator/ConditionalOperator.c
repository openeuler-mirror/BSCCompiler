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
  int a = 9;
  int b = (a < 10) ? 1 : 2;
  printf("%d %d\n", a, b); // 9 1
  // nested false expr
  b = (a < 9) ? 1 : a++;
  printf("%d %d\n", a, b); // 10 9
  // multilevel conditional expr
  (a < 10) ? (a = 1) : (b < 9) ? 1 : (a = 2, a++);
  printf("%d %d\n", a, b); // 3 9
  // noncomparative conditional expr
  double c = 0.0;
  b = c++ ? 1 : a++;
  printf("%d %d %f\n", a, b, c); // 4 3 1.000000
  return 0;
}
