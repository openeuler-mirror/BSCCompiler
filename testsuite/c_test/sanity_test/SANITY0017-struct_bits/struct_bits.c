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

#include <stdio.h>
#include <stdlib.h>

typedef struct x {
  long x;
  unsigned int z1:3;
  unsigned int z2:12;
  int z3:2;
  int z4:10;
  int z5:5;
  int y;
  char z[32];
} x;

typedef struct y {
  long x;
  long long x1 : 5;
  long long x2 : 16;
  unsigned long long x3 : 33;
  long long x4 : 10;
  int y;
} y;

void foo() {
  x xxx;
  xxx.x = 1;
  xxx.y = 2;
  xxx.z1 = 0x7;
  xxx.z2 = 0x800;
  xxx.z3 = 0x3;
  xxx.z4 = 0x201;
  xxx.z5 = 0x11;
  xxx.z[0] = 4;
  xxx.z[31] = 6;
  xxx.z[1] = 5;

  if (xxx.z1 != 7) {
    abort();
  }
  if (xxx.z2 != 0x800) {
    abort();
  }
  if (xxx.z3 != -1) {
    abort();
  }
  if (xxx.z4 != -511) {
    abort();
  }
  if (xxx.z5 != -15) {
    abort();
  }
}

void bar() {
  y yyy;

  yyy.x = 3;
  yyy.y = 4;
  yyy.x1 = 7;
  yyy.x2 = 0xff;
  yyy.x3 = 0x800f0008;
  yyy.x4 = 0x15;
}

int main() {
  foo();
  bar();
  return 0;
}

