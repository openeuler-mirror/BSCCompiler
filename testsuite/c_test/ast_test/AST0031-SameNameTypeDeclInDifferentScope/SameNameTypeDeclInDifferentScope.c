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
  struct A { int a; } aa;
  aa.a = 111;
  {
    struct A { double d; } dd;
    dd.d = 3.14;
    printf("dd.d = %f\n", dd.d);
    {
      struct A { char c; } cc;
      cc.c = 'a';
      printf("cc.c = %c\n", cc.c);
    }
  }

  printf("aa.a = %d\n", aa.a);
  return 0;
}
