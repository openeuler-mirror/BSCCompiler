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

void print_uint16x8(uint16x8_t *a, int n) {
  uint16_t *p = (uint16_t *)a;
  int i;
  for (i = 0; i < n; i++) {
    printf("%d ", *(p+i));
  }
  printf("\n");
}

int main() {
  uint16x8_t A = {1,2,3,4,5,6,7,8},
             B = {1,2,4,3,5,6,8,7};
  uint16x8_t C;

  C = vceqq_u16(A, B);
  print_uint16x8(&C, 8);

  return 0;
}

