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

int32x4_t f1( int32x4_t p, int32x4_t q )
{
   return p << q;
}

uint32x2_t f2( uint32x2_t p, uint32x2_t q )
{
   return p >> q;
}

int main()
{
   int32x4_t x = vdupq_n_s32( 1 );
   int32x4_t y = vdupq_n_s32( 1 );

   int32x4_t m1 = f1( x, y );
   uint32x4_t c1 = m1 == 2;
   if (vgetq_lane_u32(c1, 0) != 0xffffffff ||
       vgetq_lane_u32(c1, 1) != 0xffffffff ||
       vgetq_lane_u32(c1, 2) != 0xffffffff ||
       vgetq_lane_u32(c1, 3) != 0xffffffff)
     abort();

   uint32x2_t x2 = vdup_n_u32( 2 );
   uint32x2_t y2 = vdup_n_u32( 1 );

   uint32x2_t m2 = f2( x2, y2 );
   uint32x2_t c2 = m2 == y2;
   if (vget_lane_u32(c2, 0) != 0xffffffff ||
       vget_lane_u32(c2, 1) != 0xffffffff)
     abort();
}
