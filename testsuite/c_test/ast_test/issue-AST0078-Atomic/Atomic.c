/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
  int x;
  __atomic_store_n(&x, 10, 0);
  int y = __atomic_load_n(&x, 0);
  __atomic_add_fetch(&y, 1, 0);
  printf("y = %d\n", y);

  x = 11;
  if (__atomic_compare_exchange_n(&y, &x, 12, 0, 0, 0)) {
    printf("y = %d\n", y);
    printf("x = %d\n", x);
  }
  __atomic_exchange_n(&y, 13, 0);
  printf("y = %d\n", y);
  return 0;
}
