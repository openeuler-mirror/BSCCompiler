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

// void print_float64x1(float64x1_t *a, int n) {
//   float64_t *p = (float64_t *)a;
//   int i;
//   for (i = 0; i < n; i++) {
//     printf("%f ", *(p+i));
//   }
//   printf("\n");
// }

// int main() {
//   float64x1_t A = {2.2}, B = {4.4};
//   float64x1_t C;
//   C = __builtin_mpl_vector_merge_v1f64(A, B, 0);
//   print_float64x1(&C,1);

//   return 0;
// }

// int main() {
//   int64x1_t A = {2};
//   int64_t B;
//   B = __builtin_mpl_vector_get_element_v1i64(A, 0);
//   printf("%d", B);

//   return 0;
// }

void print_int64x1(int64x1_t *a, int n) {
  int64_t *p = (int64_t *)a;
  int i;
  for (i = 0; i < n; i++) {
    printf("%d ", *(p+i));
  }
  printf("\n");
}

int main() {
  int64x1_t A = {2};
  A = __builtin_mpl_vector_set_element_v1i64(9, A, 0);
  print_int64x1(&A, 1);

  return 0;
}

