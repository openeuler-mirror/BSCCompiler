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

int a = 0x80000001;
unsigned int ua = 0x00000002;
unsigned int val = 0xffffff84;

int main()
{
  int c = a;

  c += a | 0;
  c += a & 1;
  c += a ^ 0;
  c += a >> 0;
  c += a << 0;

  c += a | 1;
  c += a & 5;
  c += a ^ 1;
  c += a >> 1;
  c += a << 1;

  c += a | 0x7ff;
  c += a & 0x7ff;
  c += a ^ 0x7ff;
  c += a >> 16;
  c += a << 16;

  c += a | 0xfff;
  c += a & 0xfff;
  c += a ^ 0xfff;
  c += a >> 24;
  c += a << 24;

  c += a | 0xffff;
  c += a & 0xffff;
  c += a ^ 0xffff;
  c += a >> 31;
  c += a << 31;

  unsigned int u = ua;
  u += ua >> 0;
  u += ua >> 1;
  u += ua >> 16;
  u += a >> 24;
  u += a >> 31;

  if (c != 0xc102af83) {
    abort();
  }
  if (u != 0xffffff84) {
    abort();
  }
  if (u != val) {
    abort();
  }
}
