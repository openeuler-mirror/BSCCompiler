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
  for (int i = 0; i < 5; i++) {
    printf("%d,", i);
  }
  int a = 0;
  for (; a < 5; a++) {
    printf("%d,", a);
  }
  int b = 0;
  for (;; b++) {
    printf("%d,", b);
    if (b >= 5) {
      break;
    }
  }
  int c = 10;
  for (;;) {
    printf("%d,", c);
    c = c - 1;
    if (c <= 5) {
      break;
    }
  }
  for (int i = 0; i < 5; i++) {
    if (i == 2) {
      continue;
    }
    printf("%d,", i);
  }
  printf("0\n");
  return 0;
}
