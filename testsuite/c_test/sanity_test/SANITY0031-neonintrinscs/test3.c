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

uint32_t foo(void *p) {
   uint32x4_t u1 = vld1q_u32(p);
   uint32_t uit = vgetq_lane_u32(u1, 3);
   unsigned int ui = uit;
   if (ui != 4)
     abort();
   return uit;
}

int main()
{
   unsigned int s[] = {1, 2, 3, 4};
   uint32_t ui = foo( &s );
   uint32x4_t t = {1, 1, 1, 1};
   t = vsetq_lane_u32(ui, t, 3);
   ui = vgetq_lane_u32(t, 1);
   if (ui != 1)
     abort();
   if (vgetq_lane_u32(t, 3) != 4)
     abort();
   // printf("t: 0x%x 0x%x\n", ui, vgetq_lane_u32(t, 3));
}
