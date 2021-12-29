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
#include "stdlib.h"

int main()
{
   uint32x4_t a = vdupq_n_u32(10);
   uint16x8_t b = vdupq_n_u16(2);
   volatile uint32x4_t c = vpadalq_u16(a, b);
   if (vgetq_lane_u32(c,0) != 14 ||
       vgetq_lane_u32(c,1) != 14 ||
       vgetq_lane_u32(c,2) != 14 ||
       vgetq_lane_u32(c,3) != 14)
    abort();

   int64x1_t x = vdup_n_s64(3);
   int32x2_t y = vdup_n_s32(2);
   volatile int64x1_t z = vpadal_s32(x, y);
   if ((int64_t)z != 7)
     abort();

   int32x2_t i = vdup_n_s32(10);
   int32x2_t j = vdup_n_s32(7);
   volatile int64x2_t k = vabdl_s32(j, i);  // 3 3
   if (vgetq_lane_s64(k,0) != 3 ||
       vgetq_lane_s64(k,1) != 3)
    abort();

   uint32x4_t i2 = vdupq_n_u32(100);
   uint32x4_t j2 = vdupq_n_u32(1);
   volatile uint64x2_t l2 = vabdl_high_u32(j2, i2);
   if (vgetq_lane_u64(l2,0) != 99 ||
       vgetq_lane_u64(l2,1) != 99)
    abort();
}
