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
  uint8_t a[16] = {'a','a','b','b','c','c','d','d',
                  'a','a','b','b','c','c','d','d'};
  uint8_t b[6] = {'e','f','g','h','j','k'};
  uint8x16_t A, B, C;

  A = vld1q_u8(a);
  B = vld1q_u8(b);
  C = vextq_u8(A, B, 3);  
  print_uint8x16(&C,16);

  return 0;
}