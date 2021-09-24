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
struct A {
  int i[2];
  float f;
};
const int cInt = 111;
enum {
  e0,
  e1
};
union U {
  char c;
  int i;
  short s;
};

union U u = { 10 };

struct A Aarray[2] = { { { e0, e1 }, 11.1f },
                       { { sizeof(int), 4 }, 22.2f }
                     };
struct A a = { { 5, e1 }, 33.3f };
int i[3] = { 3 + 1, 2, 1};

//
struct D {
  int y;
  double z;
  int i;
  int j;
  struct E {
    int i;
    int j;
    struct F {
      int y;
    } f;
  } e;
} d;
struct F *pf = &d.e.f;

int main() {
  printf("a.i[0] = %d\na.i[1] = %d\n", a.i[0], a.i[1]);
  printf("i[0] = %d\ni[1] = %d\ni[2] = %d\n", i[0], i[1], i[2]);
  printf("u.i = %d\n", u.i);
  pf->y = 2;
  printf("d.e.f.y=%d, pf=%d\n", d.e.f.y, pf->y);
  
  return 0;
}
