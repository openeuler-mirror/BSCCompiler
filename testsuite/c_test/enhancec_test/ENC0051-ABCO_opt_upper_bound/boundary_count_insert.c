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
#include <limits.h>

__attribute__((count("len", 2)))
int test2(int len, int *p) {
  if (len != 1) {
    return 0;
  }
  int res = 0;
  for (int i = 0; i <= 10; i++) {
    for (int j = 4; ; j--) {
      switch (j) {
        case 2:
          res += p[j - 4];
          break;
        default:
          break;
      }
    }
  }
  return res;
}

int main() {
  return 0;
}
