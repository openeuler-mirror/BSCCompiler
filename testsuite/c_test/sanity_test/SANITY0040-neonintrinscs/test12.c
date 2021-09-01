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
#include "arm_neon.h"
#include <stdlib.h>

/* Testing calling convention of 64x1 */

uint64x1_t foo1( uint64x1_t x, uint64x1_t y )
{
   return x + y;
}

int64x1_t G = { 40 };
int64x1_t foo2() {
   return G;
}

int64x1_t foo3( int64x1_t x )
{
   return x;
}

typedef struct S {
   int x;
   int64x1_t y;
} SS;

int main()
{
   uint64x1_t a = { 10 };
   uint64x1_t b = { 20 };

   uint64x1_t r1 = foo1( a, b );
   if ((uint64_t)r1 != 30)
     abort();

   int64x1_t r2 = foo3( foo2() );
   if ((int64_t)r2 != 40)
     abort();

   SS s1 = { 1, 50 };
   int64x1_t r3 = foo3( s1.y );
   if ((int64_t)r3 != 50)
     abort();

   SS s2 = { 1, 50 };
   SS *ps = &s2;
   int64x1_t r4 = foo3( ps->y );
   if ((int64_t)r4 != 50)
     abort();

   int64x1_t c = { 60 };
   int64x1_t *p = &c;
   int64x1_t **q = &p;
   int64x1_t r5 = foo3( **q );
   if ((int64_t)r5 != 60)
     abort();

    int64x1_t (*fp)(int64x1_t) = foo3;
    uint64x1_t r6 = (uint64x1_t)foo3( fp(c) );
    if ((int64_t)r6 != 60)
      abort();
}
