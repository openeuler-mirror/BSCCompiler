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

#define asize 500
int sum = 0;
void vararr(int sz) {
  int my[sz];
  volatile int a[asize], b[asize], c[asize];

  for (int i = 0; i< asize; i++) {
    a[i] = i; b[i] = i + 1; c[i] = i + 2;
    sum = a[i] + b[i] * c[i];
    if (i < sz) {
      my[i] = sum;
    }
  }

  if ((my[0] != 2) ||
      (my[1] != 7) ||
      (my[2] != 14) ||
      (my[3] != 23)) {
    abort();
  }
}

int main() {
  vararr(4);
}
