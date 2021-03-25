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
// Test calling convention of passing struct param
//
#include <stdlib.h>
typedef struct { int x; int y; } s1;
typedef struct { long long x; long long y; } s2;
typedef struct { long long x; long long y; int z; } s3;
typedef struct { float x; float y; } f1;
typedef struct { double x; double y; } f2;
typedef struct { float x; char y; char z; short w; } m1;
typedef struct { int x[4]; } a1;
typedef struct { float x[4]; } a2;

void foo1( s1 x ){ if (x.y != 2) abort(); }     // 1 intreg
void foo2( s2 x ){ if (x.y != 4) abort(); }     // 2 intregs
void foo3( f1 x ){ if (x.y != 9.0f) abort(); }  // 2 fpregs
void foo4( f2 x ){ if (x.y != 11.0) abort(); }  // 2 fpregs
void foo5( m1 x ){ if (x.w != 15) abort(); }    // 1 intreg
void foo6( int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8, s2 x) {
   if (x.y != 4) abort();                       // 8 intregs + 16-by stk
}
void foo7( int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8, s3 x) {;
   if (x.z != 7) abort();                       // 8 intregs + 8-by stk addr + 24-byte s3 anywhere
}
void foo8( a1 x ){ if (x.x[3] != 19) abort(); }   // 2 intregs
void foo9( a2 x ){ if (x.x[3] != 23) abort(); }   // 2 intregs

int main()
{
   s1 x = { 1, 2 };
   s2 y = { 3, 4 };
   s3 e = { 5, 6, 7 };
   f1 z = { 8, 9 };
   f2 w = { 10, 11 };;
   m1 m = { 12, 13, 14, 15 };
   a1 a = { 16, 17, 18, 19 };
   a2 b = { 20, 21, 22, 23 };

   foo1( x );
   foo2( y );
   foo3( z );
   foo4( w );
   foo5( m );
   foo6( 1,2,3,4,5,6,7,8,y );
   foo7( 1,2,3,4,5,6,7,8,e );
   foo8( a );
   foo9( b );

   return 0;
}
