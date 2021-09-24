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

typedef int MyInt;
typedef struct A { int i; } StructA;
int main() {
  typedef float MyFloat;
  StructA a;
  a.i = 22;
  typedef struct A { double d; } StructA;
  StructA a0;
  a0.d = 1.414;
  MyInt i = 11;
  MyFloat f = 3.14;
  printf("i = %d\n", i);
  printf("f = %f\n", f);
  printf("a.i = %d\n", a.i);
  printf("a0.d = %f\n", a0.d);
  return 0;
}
