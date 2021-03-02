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

int main() {
  struct PSas {
    short a, b;
    int *p;
  }(*PSas0), (*PSas1);

  struct PSas pSas0, pSas1, pSasa[10];

  int i = 5;
  PSas0 = &pSas0;
  PSas1 = &pSas1;

  pSasa[8].a = 2;
  (*PSas0).a = 3;
  (*PSas1).p = &i;

  if (pSasa[8].a != 2 || (*PSas0).a != 3 || *((*PSas1).p) != 5) {
    abort();
  }

  return 0;
}
