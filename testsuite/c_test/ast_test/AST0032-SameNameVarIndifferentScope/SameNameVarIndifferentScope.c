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

enum {
  e = 0
};
extern int aa;
int aa;
int main() {
  aa = 123;
  int x = 1;
  printf("e = %d\n", e);
  enum E {
    e = 11
  };
  {
    printf("e = %d\n", e);
    enum EE {
      e = 22
    };
    printf("x = %d\n", x);
    int x = 2;
    {
      printf("e = %d\n", e);
      printf("x = %d\n", x);
      int x = 3;
      enum EEE {
        e = 33
      };
      printf("e = %d\n", e);
      printf("x = %d\n", x);
    }
    printf("x = %d\n", x);
  }
  printf("e = %d\n", e);
  printf("x = %d\n", x);
  printf("aa = %d\n", aa);
  return 0;
}
