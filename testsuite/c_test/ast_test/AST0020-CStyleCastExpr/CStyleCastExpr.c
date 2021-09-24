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
struct book {
    int a;
    int b;
};
int main() {
  double a = 2.2;
  int b = (int)a;
  int c = 3;
  double d = (double)c;
  struct book m;
  m.a = 1;
  m.b = 2;
  struct book n = (struct book)m;
  void *vPtr = (void*)&b;
  int *sPtr = (int*)vPtr;
  int arr[2];
  arr[0] = 7;
  arr[1] = 8;
  int *pArr = (int*)arr;
  printf("%d, %f, %d, %d, %d, %d\n", b, d, n.a, n.b, *sPtr, *pArr);
  return 0;
}
