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
// Test calling convention of struct return type
#include <stdlib.h>
typedef struct { int x; int y; } s1;
typedef struct { long long x; long long y; } s2;
typedef struct { long long x; long long y; int z; } s3;
typedef struct { float x; float y; } f1;
typedef struct { double x; double y; } f2;
typedef struct { float x; char y; char z; short w; } m1;
typedef struct { int x[4]; } a1;
typedef struct { double x[4]; } a2;
typedef struct { float x[2]; } f3;
typedef struct { float x[3]; } f4;
typedef struct { int x; float y; } m2;
typedef struct { double x; long long y; } m3;

s1 foo1(int i){ s1 x = {1,2}; return x; }             // 1 intreg return
s2 foo2(int i){ s2 x = {3,4}; return x; }             // 2 intregs return
s3 foo3(int i){ s3 x = {5,6,7}; return x; }           // hidden 1st arg, >16by
f1 foo4(int i){ f1 x = {8,9}; return x; }             // 2 fpregs return
f2 foo5(int i){ f2 x = {10,11}; return x; }           // 2 fpregs return
m1 foo6(int i){ m1 x = {12,13,14,15}; return x; }     // 1 intregs return
a1 foo7(int i){ a1 x = {16,17,18,19}; return x; }     // 2 intregs return
a2 foo8(int i){ a2 x = {20,21,22,23}; return x; }     // hidden 1st arg, >16by
f3 foo9(int i){ f3 x = {24,25}; return x; }           // 2 fpregs return
f4 foo10(int i){ f4 x = {26,27,28}; return x; }       // 2 intregs return
m2 foo11(int i){ m2 x = {29,30.0}; return x; }        // 1 intreg 1 fpreg
m3 foo12(int i){ m3 x = {31.0,32}; return x; }        // 1 fpreg 1 intreg

typedef struct { int a[2]; } sz8;  // 8 byte struct
typedef struct { int a[4]; } sz16;  // 16 byte struct
typedef struct { int a[8]; } sz32;  // 32 byte struct

sz8 call8()
{
   s1 x;
   s2 y;
   s3 e;
   f1 z;
   f2 w;
   f3 v;
   f4 u;
   m1 m;
   m2 n;
   m3 o;
   a1 a;
   a2 b;

   x = foo1( 0 );
   if (x.y != 2) abort();
   y = foo2( 0 );
   if (y.y != 4) abort();
   e = foo3( 0 );
   if (e.z != 7) abort();
   z = foo4( 0 );
   if (z.y != 9) abort();
   w = foo5( 0 );
   if (w.y != 11) abort();
   m = foo6( 0 );
   if (m.w != 15) abort();
   a = foo7( 0 );
   if (a.x[3] != 19) abort();
   b = foo8( 0 );
   if (b.x[3] != 23) abort();
   v = foo9( 0 );
   if (v.x[1] != 25) abort();
   u = foo10( 0 );
   if (u.x[2] != 28) abort();
   n = foo11( 0 );
   if (n.x != 29 || n.y != 30.0) abort();
   o = foo12( 0 );
   if (o.x != 31.0 || o.y != 32) abort();

   sz8 x8 = { 1, 2};
   return x8;
}

sz16 call16()
{
   s1 x;
   s2 y;
   s3 e;
   f1 z;
   f2 w;
   f3 v;
   f4 u;
   m1 m;
   m2 n;
   m3 o;
   a1 a;
   a2 b;

   x = foo1( 0 );
   if (x.y != 2) abort();
   y = foo2( 0 );
   if (y.y != 4) abort();
   e = foo3( 0 );
   if (e.z != 7) abort();
   z = foo4( 0 );
   if (z.y != 9) abort();
   w = foo5( 0 );
   if (w.y != 11) abort();
   m = foo6( 0 );
   if (m.w != 15) abort();
   a = foo7( 0 );
   if (a.x[3] != 19) abort();
   b = foo8( 0 );
   if (b.x[3] != 23) abort();
   v = foo9( 0 );
   if (v.x[1] != 25) abort();
   u = foo10( 0 );
   if (u.x[2] != 28) abort();
   n = foo11( 0 );
   if (n.x != 29 || n.y != 30.0) abort();
   o = foo12( 0 );
   if (o.x != 31.0 || o.y != 32) abort();

   sz16 x16 = { 1, 2, 3, 4};
   return x16;
}

sz32 call32()
{
   s1 x;
   s2 y;
   s3 e;
   f1 z;
   f2 w;
   f3 v;
   f4 u;
   m1 m;
   m2 n;
   m3 o;
   a1 a;
   a2 b;

   x = foo1( 0 );
   if (x.y != 2) abort();
   y = foo2( 0 );
   if (y.y != 4) abort();
   e = foo3( 0 );
   if (e.z != 7) abort();
   z = foo4( 0 );
   if (z.y != 9) abort();
   w = foo5( 0 );
   if (w.y != 11) abort();
   m = foo6( 0 );
   if (m.w != 15) abort();
   a = foo7( 0 );
   if (a.x[3] != 19) abort();
   b = foo8( 0 );
   if (b.x[3] != 23) abort();
   v = foo9( 0 );
   if (v.x[1] != 25) abort();
   u = foo10( 0 );
   if (u.x[2] != 28) abort();
   n = foo11( 0 );
   if (n.x != 29 || n.y != 30.0) abort();
   o = foo12( 0 );
   if (o.x != 31.0 || o.y != 32) abort();

   sz32 x32 = { 1, 2, 3, 4, 5, 6, 7, 8};
   return x32;
}

int main()
{
  call8();
  call16();
  call32();
}
