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
  double d1 = 10;
  double d2 = 20;
  int i = 0;

  if (d1 <= d2) {
    i++;
  } else {
    abort();
  }

  if (d2 >= d1) {
    i++;
  } else {
    abort();
  }

  if (d2 > d1) {
    i++;
  } else {
    abort();
  }

  if (d1 < d2 && d2 > d1) {
    i++;
  } else {
    abort();
  }

  if (d1 != d2) {
    i++;
  } else {
    abort();
  }

  d1 = 20;
  if (d1 <= d2) {
    if (d1 == d2) {
      i++;
    } else {
      abort();
    }
  }

  float f1 = 10;
  float f2 = 20;

  if (f1 <= f2) {
    i++;
  } else {
    abort();
  }

  if (f2 >= f1) {
    i++;
  } else {
    abort();
  }

  if (f2 > f1) {
    i++;
  } else {
    abort();
  }

  if (f1 < f2 && f2 > f1) {
    i++;
  } else {
    abort();
  }

  if (f1 != f2) {
    i++;
  } else {
    abort();
  }

  f1 = 20;
  if (f1 <= f2) {
    if (f1 == f2) {
      i++;
    } else {
      abort();
    }
  }

  if (i != 12) {
    abort();
  }
}
