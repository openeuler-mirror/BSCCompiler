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

int A[5];
int foo(int i) {
  A[3] = i + A[2];
  return A[(i >= 0 && i < 5) ? i : 0];
}

float B[15];
float bar(int i) {
  B[3] = i + B[2];
  return B[(i >= 0 && i < 5) ? i : 0];
}

// variable array type
void vararr(int sz) {
  int my[sz];
  int i;
  for (i = 0; i < sz; ++i) {
    my[i] = i;
  }
  for (i = 0; i < sz; ++i) {
    if (my[i] != i) {
      abort();
    }
  }
}

int main()
{
  vararr(4);

  A[0] = 0;
  A[2] = A[3] = 2;
  if (A[3] != 2) {
    abort();
  }
  if (foo(2) != 2) {
    abort();
  }
  if (A[3] != 4) {
    abort();
  }
  if (foo(5) != 0) {
    abort();
  }
  if (A[3] != 7) {
    abort();
  }

  B[0] = 0;
  B[1] = B[2] = B[3] = 3;
  if (B[3] != 3) {
    abort();
  }
  if (bar(1) != 3) {
    abort();
  }
  if (B[3] != 4) {
    abort();
  }
  if (bar(5) != 0) {
    abort();
  }
  if (B[3] != 8) {
    abort();
  }

  return 0;
}
