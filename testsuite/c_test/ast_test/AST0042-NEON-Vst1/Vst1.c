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
#include "stdio.h"
#include <arm_neon.h>

int main() {
  unsigned char a[16] = {'a','a','b','b','c','c','d','d',
                         'a','a','b','b','c','c','d','d'};
  unsigned char b[16];
  uint8x16_t A;

  A = vld1q_u8(a);
  vst1q_u8(b, A);
  for (int i = 0; i < 16; i++) {
    printf("%c ", b[i]);
  }
  printf("\n");

  return 0;
}
