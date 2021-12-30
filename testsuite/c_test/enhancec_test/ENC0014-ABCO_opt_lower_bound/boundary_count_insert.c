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
void test1(int len, int *p) {
  if (len < 0) {
    return;
  }
  for (int i = len; i >= 0; --i) {
    printf("%d\n", p[i]);
  }
}

__attribute__((count("len", 2)))
int test2(int len, int *p, int lo) {
  int res = 0;
  if (len < 0) {
    return res;
  }
  for (int i = 10; i <= 20; i++) {
    for (int j = 4; j < i - 5; j++) {
      res += p[j - 4];
    }
  }
  return res;
}

int main() {
  return 0;
}
