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
#include <complex.h>

int main() {
  float complex a = 1 + I;
  printf("%f + i%f\n", __real a , __imag a);
  float complex b = 1 + I;
  printf("%f + i%f\n", __real b , __imag b);
  float complex c = a + b;
  printf("%f + i%f\n", __real c , __imag c);
  complex double d = a - b;
  printf("%f + i%f\n", __real d , __imag d);
  return 0;
}
