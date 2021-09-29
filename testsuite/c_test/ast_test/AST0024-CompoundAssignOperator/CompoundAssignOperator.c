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
int g_a = 10;
int g_b = 20;

int main()
{
  int a = 15;
  int b = 35;

  b += a;           // +=
  g_b += g_a;
  a += g_a;
  printf("%d%d%d\n", b, g_b, a);
  b -= a;           // -=
  g_b -= g_a;
  a -= g_a;
  printf("%d%d%d\n", b, g_b, a);
  b *= a;           // *=
  g_b *= g_a;
  a *= g_a;
  printf("%d%d%d\n", b, g_b, a);
  b /= a;           // /=
  g_b /= g_a;
  a /= g_a;
  printf("%d%d%d\n", b, g_b, a);
  b %= a;           // %=
  g_b %= g_a;
  a %= g_a;
  printf("%d%d%d\n", b, g_b, a);

  // TODO: need more nested example
  b += g_a;
  g_b %= a;
  printf("%d%d\n", b, g_b);

  return 0;
}