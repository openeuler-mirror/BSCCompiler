/*
 * Copyright (C) [2021] Futurewei Technologies, Inc. All rights reverved.
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

//
// Test calling convention of passing half-half struct param
//
#include <stdlib.h>
typedef struct { long long i; int j; } S0;
typedef struct { long long i; int j; } S1;
typedef struct { long long i; int j; } S2;

// IntCC: 1 intreg short, half of s passed on stack, half in fpreg
void foo1( int i1, int i2, int i3, S0 r, int i4, int i5, int i6, int i7, S1 s, S1 t )
{
  if (r.j != 2) abort();
  if (s.j != 4) abort();
  if (t.j != 6) abort();
}

void foo2( int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8, S1 s, S1 t )
{
  if (s.j != 4) abort();
  if (t.j != 6) abort();
}

int main()
{
    S0 x = { 1, 2 };
    S1 y = { 3, 4 };
    S1 z = { 5, 6 };

    foo1(1,2,3,x,4,5,6,7, y, z);
    foo2(0,0,0,0,0,0,0,0, y, z);
}
