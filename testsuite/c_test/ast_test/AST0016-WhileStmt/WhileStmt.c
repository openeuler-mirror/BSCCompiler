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
  int i = 0;
  while (++i < 5) {
    if (i == 3) {
      continue;
    }
    printf("%d,", i);
  }
  i = 0;
  while (++i < 5) {
    if (i == 3) {
      break;
    }
    printf("%d,", i);
  }
  i = 1;
  while (i) {
    i = i + 1;
    printf("%d,", i);
    if (i > 3) {
      i = 0;
    }
  }
  int a = 0;
  while (1) {
    if (a >= 5) {
      break;
    }
    printf("%d,", a);
    a = a + 1;
  }
  printf("0\n");
  return 0;
}
