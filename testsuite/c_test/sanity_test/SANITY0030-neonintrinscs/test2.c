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

uint8x16_t foo(void *p) {
   uint8x16_t u1 = vld1q_u8(p);
   uint8x16_t u2 = vrev32q_u8(u1);        /* reverse */
   return vextq_u8(u1, u2, 8);            /* merge at 8 */
}

int main()
{
   char s[] = {1,1,2,2, 3,3,4,4, 5,5,6,6, 7,7,8,8};
   uint8x16_t f2 = foo( &s );
   long u8m[2];
   vst1q_u8((void*)&u8m, f2);
   // printf("f2: 0x%lx,0x%lx\n", u8m[0], u8m[1]);
   if (u8m[0] != 0x808070706060505 || u8m[1] != 0x303040401010202)
     abort();
}
