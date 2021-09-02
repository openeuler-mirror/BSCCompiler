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
#include <stdio.h>
#include <stdlib.h>

uint32x4_t foo(void *p) {
   uint16x8_t u16 = vld1q_u16(p);
   return vpaddlq_u16(u16);
}

uint16_t foo2(void *p) {
   uint16x8_t u16 = vld1q_u16(p);
   return vaddvq_u16(u16);
}

int main()
{
   char s[] = {1,1,1,1, 2,2,2,2, 3,3,3,3, 4,4,4,4};
   uint32x4_t f1 = foo( &s );
   uint32_t t1, t2, t3, t4;
   t1 = vgetq_lane_u32(f1, 0);
   // printf("t0: 0x%x\n", t1);
   t2 = vgetq_lane_u32(f1, 1);
   // printf("t1: 0x%x\n", t2);
   t3 = vgetq_lane_u32(f1, 2);
   // printf("t2: 0x%x\n", t3);
   t4 = vgetq_lane_u32(f1, 3);
   // printf("t3: 0x%x\n", t4);
   if (t1 != 0x202 || t2 != 0x404 || t3 != 0x606 || t4 != 0x808)
     abort();

   // printf("-- Test 2: --\n");
   uint16_t f2 = foo2( &s );
   // printf("f2: %d\n", f2);
   if (f2 != 0x1414)
     abort();
}
