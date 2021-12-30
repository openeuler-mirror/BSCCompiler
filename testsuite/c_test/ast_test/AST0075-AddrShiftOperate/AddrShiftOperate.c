/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include <stddef.h>

int a[6] = {12345, 1, 2, 3, 4, 5};

int main() {
  int *b = &a[2];
  // CHECK: iread i32 <* i32> 0 (add ptr (dread ptr %b_21_3, constval i64 8))
  printf("%d\n", *(b + 2));
  // CHECK: iread i32 <* i32> 0 (add ptr (constval i64 8, dread ptr %b_21_3))
  printf("%d\n", *(2 + b));
  // CHECK: iread i32 <* i32> 0 (add ptr (dread ptr %b_21_3, constval i64 -8))
  printf("%d\n", *(b + (-2)));
  int i = 2;
  // CHECK:      iread i32 <* i32> 0 (add ptr (
  // CHECK-NEXT:   dread ptr %b_21_3,
  // CHECK-NEXT:   mul i64 (
  // CHECK-NEXT:     cvt i64 i32 (dread i32 %i_28_3),
  // CHECK-NEXT:     constval i64 4)))
  printf("%d\n", *(b + i));
  // CHECK:      iread i32 <* i32> 0 (sub ptr (
  // CHECK-NEXT:   dread ptr %b_21_3,
  // CHECK-NEXT:   mul i64 (
  // CHECK-NEXT:     cvt i64 i32 (dread i32 %i_28_3),
  // CHECK-NEXT:     constval i64 4)))
  printf("%d\n", *(b - i));
  unsigned int j = 2;
  // CHECK:      iread i32 <* i32> 0 (add ptr (
  // CHECK-NEXT:   dread ptr %b_21_3,
  // CHECK-NEXT:   mul ptr (
  // CHECK-NEXT:     cvt ptr u32 (dread u32 %j_41_3),
  // CHECK-NEXT:     constval ptr 4)))
  printf("%d\n", *(b + j));
  // CHECK:      iread i32 <* i32> 0 (sub ptr (
  // CHECK-NEXT:   dread ptr %b_21_3,
  // CHECK-NEXT:   mul ptr (
  // CHECK-NEXT:     cvt ptr u32 (dread u32 %j_41_3),
  // CHECK-NEXT:     constval ptr 4)))
  printf("%d\n", *(b - j));
  int k = -2;
  // CHECK:      iread i32 <* i32> 0 (add ptr (
  // CHECK-NEXT:   dread ptr %b_21_3,
  // CHECK-NEXT:   mul i64 (
  // CHECK-NEXT:     cvt i64 i32 (dread i32 %k_54_3),
  // CHECK-NEXT:     constval i64 4)))
  printf("%d\n", *(b + k));
  // CHECK:      iread i32 <* i32> 0 (sub ptr (
  // CHECK-NEXT:   dread ptr %b_21_3,
  // CHECK-NEXT:   mul i64 (
  // CHECK-NEXT:     cvt i64 i32 (dread i32 %k_54_3),
  // CHECK-NEXT:     constval i64 4)))
  printf("%d\n", *(b - k));
  return 0;
}

