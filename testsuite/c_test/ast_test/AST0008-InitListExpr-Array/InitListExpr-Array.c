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
  int arr1[10] = {1 ,2, 3};
  double arr2[10] = {1.5, 2.5 ,8.5};
  char arr3[10] = {'a', 'b'};
  char arr4[2][3] = {'a', 'b','c','d','e'};
  char arr5[2][2][2] = {{'a', 'b','c','d'},{{'b','c'},{'d','e'}}};
  printf("%d\n", arr1[2]);
  printf("%f\n", arr2[2]);
  printf("%c\n", arr3[0]);
  printf("%c\n", arr4[1][1]);
  arr4[1][1] = 'z';
  char c = arr4[1][1];
  printf("%c\n", arr4[1][1]);
  printf("%c\n",c);
  printf("%c\n", arr5[0][1][1]);
  arr5[0][1][1] = 'w';
  char c1 = arr5[0][1][1];
  printf("%c\n",c1);
  printf("%c\n", arr5[0][1][1]);
  return 0;
}

