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
#include <string.h>
#include <stdio.h>

union AA {
  long a;
  long b;
  long c;
  long d;
};

union AA test(long a, long b) {
  union AA aa;
  aa.d = 3;
  if (a > b) {
    while (a-- > aa.b) {
      aa.d += a * aa.c + b;
      printf("%ld\n", aa.d);
    }
  } else {
    while (b-- > aa.c) {
      aa.c += a * aa.c + b/aa.d;
      printf("%ld\n", aa.d);
    }
  }

  while (b++ < aa.c + aa.d) {
    aa.c += a * aa.c + b/aa.d;
    printf("%ld\n", aa.c);
  }

  return aa;
}

int main(void)
{
  test(18, 19);
  return 0;
}
