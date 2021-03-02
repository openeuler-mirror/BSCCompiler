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

int i = 1;
float f = 2.2;
long long ll = 3;
double d = 4.4;

int fi(){ return i; }
float ff(){ return f; }
long long fll(){ return ll; }
double fd(){ return d; }

int main() {
  if (fi() != 1) {
    abort();
  }
  if (ff() != (float)2.2) {
    abort();
  }
  if (fll() != 3) {
    abort();
  }
  if (fd() != 4.4) {
    abort();
  }
  return 0;
}

