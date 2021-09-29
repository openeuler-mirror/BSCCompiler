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

void print_uint8x16(uint8x16_t *a, int n) {
  uint8_t *p = (uint8_t *)a;
  int i;
  for (i = 0; i < n; i++) {
    printf("%c ", *(p+i));
  }
  printf("\n");
}

int main() {
  uint8_t data[8] = {'a','a','b','b','c','c','d','d'};
  uint8x16_t A, B;

  A = vld1q_u8(data);
  B = vrev32q_u8(A);
  print_uint8x16(&B, 8);

  return 0;
}