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

#include "stdarg.h"
#include "stdlib.h"

int a,b;
typedef int vfunc_t(int x, int y, ...);
int foo1( int x, int y, ... )
{
   va_list ap;

   va_start(ap, y);
   a = va_arg(ap, int);
   b = va_arg(ap, int);
   return (int)a + b;
}

typedef struct { int i; long long j; } S;
int foo2( int a0, int a1, ... )
{
   va_list ap;

   va_start(ap, a1);
   S b = va_arg(ap, S);
   if (b.i+b.j != 30)
     abort();
}

typedef struct { long long x; long long y; int z; } T;
void foo3( int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8, T x, ...)
{
   va_list ap;

   va_start(ap, x);
   a = va_arg(ap, int);
   if (a != 10)    // on the stack
     abort();
}

int main()
{
    vfunc_t *f = &foo1;
    int x;

    x = foo1(1,2,3,4);
    if (x != 7)
      abort();
    x = (*f)(1,2,5,6);
    if (x != 11)
      abort();

    S s = {10,20};
    foo2( 1,2, s );

    T y = { 1, 2, 3 };

    foo3(1,2,3,4,5,6,7,8, y,10 );
}


