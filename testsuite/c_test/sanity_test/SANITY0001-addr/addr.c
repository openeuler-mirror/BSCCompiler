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

int deref(int *iptr) {
  return *iptr;
}

int deref1(int *iptr) {
  int j = 8;
  j = *iptr + j;
  return j;
}

int addrof(int *iptr) {
  int j = 8;
  int *jptr = &j;
  *jptr = *iptr;
  return *jptr;
}

int main()
{
  int v = 5;
  int *vptr = &v;

  if (*vptr != 5) {
    abort();
  }
  if (deref(&v) != 5) {
    abort();
  }
  if (deref1(&v) != 13) {
    abort();
  }
  if (addrof(&v) != 5) {
    abort();
  }
}

