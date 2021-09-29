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
#include <stdio.h>
enum G {
  g0 = 1,
  g1 = sizeof(int),
  g2,
};

int main() {
  enum L {
	l0 = 2,
	l1,
	l2 = sizeof(float)
  };

  printf("g0 = %d, g1 = %d, g2 = %d\nl0 = %d, l1 = %d, l2 = %d\n", g0, g1, g2, l0, l1, l2);
  return 0;
}
