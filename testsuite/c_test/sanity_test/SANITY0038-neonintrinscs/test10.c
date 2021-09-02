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

uint64x1_t foo1( uint64x2_t s )
{
   return vget_high_u64(s) + vget_low_u64(s);
}

uint16x4_t foo2( uint16x8_t s )
{
   return vget_high_u16(s) + vget_low_u16(s);
}

int64x1_t foo3( int32x2_t s )
{
   return vpaddl_s32(s);
}

int main()
{
   uint64x2_t s = { 10, 20 };
   uint64x1_t r1 = foo1( s );
   if ((long)r1 != 30)
     abort();

#if 0
   uint16x8_t s2 = { 1,1,1,1,2,2,2,2 };
   uint16x4_t r2 = foo2( s2 );
   if (vget_lane_u16(r2,0) != 3 ||
       vget_lane_u16(r2,3) != 3)
     abort();

   int32x2_t s3 = { 10, 20 };
   int64x1_t t3 = foo3( s3 );
   if ((long)t3 != 30)
     abort();
#endif
}

