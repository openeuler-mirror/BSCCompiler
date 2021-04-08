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

#include <stdlib.h>

typedef struct { int i; long long j; } S1;
// IntCC: no intreg, entire s passed on stack
void foo0( int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8, S1 s )
{ if (s.j != 2) abort(); }

// IntCC: 1 intreg short, half of s passed on stack, half $a7 reg
void foo1( int i1, int i2, int i3, int i4, int i5, int i6, int i7, S1 s )
{ if (s.j != 2) abort(); }

// FpCC: 1 intreg short, move to IntCC: 1 stack
typedef struct { float i; int j; } S2;
void foo2( int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8, S2 s )
{ if (s.j != 4) abort(); }

// FpCC: 1 fpreg short, move to IntCC: 1 intreg + 1 stack
void foo3( float i1, float i2, float i3, float i4, float i5, float i6, float i7, float i8, S2 s )
{ if (s.j != 4) abort(); }

// FpCC: 2 fpregs short, move to IntCC: 2 stacks
typedef struct { double i; double j; float k; } S4;
void foo4( float i1, float i2, float i3, float i4, float i5, float i6, float i7, float i8, S4 s )
{ if (s.j != 6) abort(); }

// IntCC: 1 float on stack, 8 intregs
typedef struct { float i; int j; } S5;
void foo5( int i1, int i2, int i3, int i4, int i5, int i6, int i7,
           float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8,
           S5 s )
{ if (s.i != 7.0f || s.j != 8) abort(); }

// IntCC: 1 int on stack, 8 fpregs
void foo6( int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8,
           float f1, float f2, float f3, float f4, float f5, float f6, float f7,
           S5 s )
{ if (s.i != 7.0f || s.j != 8) abort(); }

int main()
{
  S1 s1 = { 1, 2 };
  foo0(1,2,3,4,5,6,7,8, s1);
  foo1(1,2,3,4,5,6,7, s1);

  S2 s2 = { 3, 4 };
  foo2(1,2,3,4,5,6,7,8, s2);
  foo3(0,0,0,0,0,0,0,0, s2);

  S4 s4 = { 5, 6 };
  foo4(0,0,0,0,0,0,0,0, s4);

  S5 s5 = { 7.0f, 8 };
  foo5( 1,2,3,4,5,6,7,    10,20,30,40,50,60,70,80, s5 );
  foo6( 1,2,3,4,5,6,7,8,  10,20,30,40,50,60,70,    s5 );

  return 0;
}
