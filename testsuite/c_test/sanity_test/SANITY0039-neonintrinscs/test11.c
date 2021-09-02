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

uint32x2x2_t foo( uint32x4_t data, uint32x2x2_t a )
{
   uint32x2x2_t r = vzip_u32( vget_low_u32(data), vget_high_u32(data) );
   uint32x2_t t1 = r.val[0] == a.val[0];
   uint32x2_t t2 = r.val[1] == a.val[1];

   if (vget_lane_u32(t1, 0) != 0xffffffff ||
       vget_lane_u32(t1, 1) != 0xffffffff ||
       vget_lane_u32(t2, 0) != 0xffffffff ||
       vget_lane_u32(t2, 1) != 0xffffffff)
     abort();

   return r;
}

int main()
{
   uint32x4_t data = { 10, 20, 30, 40 };
   uint32x2x2_t a;
   uint32x2x2_t r;

   a.val[0] = vset_lane_u32(10, a.val[0], 0);
   a.val[0] = vset_lane_u32(30, a.val[0], 1);
   a.val[1] = vset_lane_u32(20, a.val[1], 0);
   a.val[1] = vset_lane_u32(40, a.val[1], 1);

   r = foo( data, a );

   uint32x2_t a1 = { 1, 3 };
   uint32x2_t b1 = { 2, 4 };
   uint32x2_t m1 = { 1, 2 };
   uint32x2_t m2 = { 4, 5 };

   uint32x2x2_t c = vzip_u32( a1, b1 );
   uint32x2_t r1 = c.val[0] == m1;  // vzip_u32(a,b).val[0] == m1;
   if (vget_lane_u32(r1, 0) != 0xffffffff ||
       vget_lane_u32(r1, 1) != 0xffffffff)
     abort();

   m1 = c.val[1] + 1;
   uint32x2_t r2 = m1 == m2;
   if (vget_lane_u32(r2, 0) != 0xffffffff ||
       vget_lane_u32(r2, 1) != 0xffffffff)
     abort();
}


