/*
 * Copyright (C) [2020] Futurewei Technologies, Inc. All rights reverved.
 *
 * Licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */

#include <stdlib.h>

void foo1(int x) { // x = (0, 1, -1, 1, 0, 0)
  int a1 = x++ == 0;
  int a2 = x-- != 0;
  int a3 = --x < 0;
  int a4 = x+2 > 0;
  int a5 = ++x <= 0;
  int a6 = x >= 0;
  if (a1+a2+a3+a4+a5+a6 != 6) {
    abort();
  }
}

void foo2(int x) { // x = (1, 2, 0, 2, 1, 1)
  int a1 = x == 1;
  int a2 = ++x != 1;
  int a3 = x-2 < 1;
  int a4 = x > 1;
  int a5 = --x <= 1;
  int a6 = x >= 1;
  if (a1+a2+a3+a4+a5+a6 != 6) {
    abort();
  }
}

void foo3(int x, int y) { // x&y = (1==1, 1!=2, 0<2, 0>-1, 0<=0, 0>=0)
  int a1 = x == y;
  int a2 = x != ++y;
  int a3 = --x < y--;
  int a4 = x > y-2;
  int a5 = x <= --y;
  int a6 = x >= --y;
  if (a1+a2+a3+a4+a5+a6 != 6) {
    abort();
  }
}

void foo4(unsigned x) { // x = (0, 1, 0xf-f, 1, 0, 0)
  unsigned a1 = x++ == 0;
  unsigned a2 = x-- != 0;
  unsigned a3 = (--x < 0) + 1;
  unsigned a4 = x+2 > 0;
  unsigned a5 = ++x <= 0;
  unsigned a6 = x >= 0;
  if (a1+a2+a3+a4+a5+a6 != 6) {
    abort();
  }
}

void foo5(unsigned x) { // x = (1, 2, 0, 0xf-f, 1, 1)
  unsigned a1 = x == 1;
  unsigned a2 = ++x != 1;
  unsigned a3 = x-2 < 1;
  unsigned a4 = x-3 > 1;
  unsigned a5 = --x <= 1;
  unsigned a6 = x >= 1;
  if (a1+a2+a3+a4+a5+a6 != 6) {
    abort();
  }
}

void foo6(unsigned x, unsigned y) { // x&y = (1==1, 1!=2, 0<2, 0xf-f>1, 0<=0, 0>=0,
  //        0xf-f>=0)
  unsigned a1 = x == y;
  unsigned a2 = x != ++y;
  unsigned a3 = --x < y--;
  unsigned a4 = --x > y;
  unsigned a5 = ++x <= --y;
  unsigned a6 = x >= y;
  unsigned a7 = --x >= y;
  if (a1+a2+a3+a4+a5+a6+a7 != 7) {
    abort();
  }
}

void foo7(unsigned x) { // x = (4096, 4097, 4094, 4097, 4096, 4096)
  unsigned a1 = x == 4096;
  unsigned a2 = ++x != 4096;
  unsigned a3 = x-3 < 4095;
  unsigned a4 = x > 4096;
  unsigned a5 = --x <= 4096;
  unsigned a6 = x >= 4096;
  if (a1+a2+a3+a4+a5+a6 != 6) {
    abort();
  }
}

int main() {
  foo1(0);
  foo2(1);
  foo3(1, 1);
  foo4(0);
  foo5(1);
  foo6(1, 1);
  foo7(4096);
}
