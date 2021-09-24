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

void print_uint32x4(uint32x4_t *a, int n) {
  uint32_t *p = (uint32_t *)a;
  int i;
  for (i = 0; i < n; i++) {
    printf("%d ", *(p+i));
  }
  printf("\n");
}


int main() {
  uint16_t data[8] = {2,3,4,5,6,7,8,9};
  uint16x8_t A;
  uint32x4_t B;

  A = vld1q_u16(data);
  B = vpaddlq_u16(A);  //add pairs of adjacent unsigned integer values
  print_uint32x4(&B, 4);

  return 0;
}