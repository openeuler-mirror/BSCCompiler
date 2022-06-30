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

int *g_ptr = (int [4]) { 4, 8, 16, 32 };
double *g_ptr1 = (double [4]) { 1.1, 1.2, 1.3, 1.4 };

int *g_a = &(int){6};
int main() {
  int *ptr = (int [4]) { 4, 8, 16, 32 };
  printf("%d %d %f\n", g_ptr[1], ptr[2], g_ptr1[1]);

  int *a = &(int){7};
  float *b = &(float){1.1};
  printf("%d %d %f\n", *g_a, *a, *b);
  return 0;
}
