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

void foo1()
{
   uint32x2_t c = {1,1};

   uint8x16_t x = vdupq_n_u8(10);
   uint64x2_t y = vreinterpretq_u64_u8(x);
   uint64x2_t z = vmlal_u32(y, c, vdup_n_u32(1));
   uint32x4_t r = vreinterpretq_u32_u64(z);
   //printf("r: 0x%x 0x%x 0x%x 0x%x\n", vgetq_lane_u32(r, 0), vgetq_lane_u32(r, 1),
   //     vgetq_lane_u32(r, 2), vgetq_lane_u32(r, 3));
   // printf("z = 0x%lx, 0x%lx\n", vget_high_u64(z), vget_low_u64(z));
   if (vgetq_lane_u32(r, 0) != 0xa0a0a0b || vgetq_lane_u32(r, 1) != 0xa0a0a0a ||
       vgetq_lane_u32(r, 2) != 0xa0a0a0b || vgetq_lane_u32(r, 3) != 0xa0a0a0a)
     abort();
}

void foo2() {
   uint64x2_t x = { 2, 4 };
   uint32x4_t zero = { 0, 0, 0, 0 };
   uint32x4_t r = veorq_u32( vreinterpretq_u32_u64(x), zero );
   if (vgetq_lane_u32(r, 0) != 2)
     abort();
   if (vgetq_lane_u32(r, 1) != 0)
     abort();
   if (vgetq_lane_u32(r, 2) != 4)
     abort();
   if (vgetq_lane_u32(r, 3) != 0)
     abort();
}

void foo3() {
   uint64x2_t x = { 2, 4 };
   uint64x2_t zero = { 0, 0 };
   uint32x4_t r = vreinterpretq_u32_u64( veorq_u64( x, zero ) );
   if (vgetq_lane_u32(r, 0) != 2)
     abort();
   if (vgetq_lane_u32(r, 1) != 0)
     abort();
   if (vgetq_lane_u32(r, 2) != 4)
     abort();
   if (vgetq_lane_u32(r, 3) != 0)
     abort();
}

void foo4() {
  uint32x2_t a = {1, 0};
  uint32x2_t b = vdup_n_u32(10);
  uint32x4_t r = vreinterpretq_u32_u64( vmull_u32(a, b) );
  if (vgetq_lane_u32(r, 0) != 0xa || vgetq_lane_u32(r, 1) != 0 ||
      vgetq_lane_u32(r, 2) != 0   || vgetq_lane_u32(r, 3) != 0)
    abort();
}

int main()
{
   foo1();
   foo2();
   foo3();
   foo4();
}
