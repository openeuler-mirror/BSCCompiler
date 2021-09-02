/*
2
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
#include <stdio.h>
#include <stdlib.h>

int32x4_t foo1(void *p) {
   return vandq_s32( vld1q_s32(p), vdupq_n_s32(3));
}

uint16x8_t foo2(void *p, void *q) {
   return vandq_u16( vld1q_u16(p), vld1q_u16(q));
}

int main()
{
   int s[] = {7,7,7,7};
   int32x4_t r = foo1( &s );
   int t[4];
   vst1q_s32((void*)&t, r);
   // printf("r: %x %x %x %x\n", t[0], t[1], t[2], t[3]);
   if (t[0] != 3 || t[1] != 3 || t[2] != 3 || t[3] != 3)
     abort();

   // printf("-- Test 4b: uint16x8_t --\n");
   unsigned short s2[] = {7,7,7,7,7,7,7,7};
   unsigned short t2[] = {3,3,3,3,3,3,3,3};
   uint32x4_t r2 = vpaddlq_u16( foo2( &s2, &t2 ) );
   // printf("r2: %x %x %x %x\n",
   //     vgetq_lane_u32(r2, 0),
   //     vgetq_lane_u32(r2, 1),
   //     vgetq_lane_u32(r2, 2),
   //     vgetq_lane_u32(r2, 0));
   if (vgetq_lane_u32(r2, 0) != 6 ||
       vgetq_lane_u32(r2, 1) != 6 ||
       vgetq_lane_u32(r2, 2) != 6 ||
       vgetq_lane_u32(r2, 0) != 6)
     abort();
}
