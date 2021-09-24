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

void print_uin32x2x2(uint32x2x2_t *a, int n) {
  uint32_t *p = (uint32_t *)a;
  int i;
  for (i = 0; i < n/2; i++) {
    printf("%d ", *(p+i));
  }
  printf("\n");
  for (i = n/2; i < n; i++) {
    printf("%d ", *(p+i));
  }
}

int main() {
  uint32x2_t A = {1,2}, B = {3,4};
  uint32x2x2_t C;

  C = vzip_u32(A, B);       //interleaves the pairs

  print_uin32x2x2(&C, 4);
  printf("\n");

  return 0;
}

