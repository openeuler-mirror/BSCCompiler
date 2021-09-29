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
  uint32_t a[4] = {1,2,3,4};
  uint32_t b[4] = {5,6,7,8};
  uint32x4_t A, B, C;

  A = vld1q_u32(a);
  B = vld1q_u32(b);
  C = veorq_u32(A, B);
  print_uint32x4(&C, 4);

  return 0;
}

