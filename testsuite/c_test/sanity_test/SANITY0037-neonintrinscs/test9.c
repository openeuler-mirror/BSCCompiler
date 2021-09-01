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

int main()
{
   uint16x8_t s1 = { 1, 2, 3, 4, 5, 6, 7, 8 };

   uint16x8_t r1 = s1 == s1;
   uint16x8_t r2 = vceqq_u16(s1, s1);
   uint16x8_t r3 = r1 == r2;         // all 1s
   int64x2_t r4 = (int64x2_t)r3;
   int64x2_t r5 = ~r4;               // all 0s
   uint32x4_t r6 = (uint32x4_t) (r5 < 0);  // zero compare instr
   if (vgetq_lane_u32(r6, 0) != 0 ||
       vgetq_lane_u32(r6, 1) != 0 ||
       vgetq_lane_u32(r6, 2) != 0 ||
       vgetq_lane_u32(r6, 3) != 0)
     abort();
}
