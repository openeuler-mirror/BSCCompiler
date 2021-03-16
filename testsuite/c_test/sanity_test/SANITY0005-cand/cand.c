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

int i = 2;
int main() {
  int v = (((i > 0) && (i < 3)) ? 1 : 2);
  if ( v != 1 ) {
    abort();
  }

  v = (((i > 0) && (i < 2)) ? 1 : 2);
  if ( v != 2 ) {
    abort();
  }

  v = (((i > 2) && (i < 3)) ? 1 : 2);
  if ( v != 2 ) {
    abort();
  }
}
