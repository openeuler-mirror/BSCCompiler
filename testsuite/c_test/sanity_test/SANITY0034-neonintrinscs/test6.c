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
#include <string.h>

int main()
{
   char s[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
   uint8x16_t p = vld1q_u8((void*)s);
   uint8x16_t idx = vrev32q_u8(p);
   char r[17] = {0};
   uint8x16_t x;

   x = vqtbl1q_u8(p, idx);  // reverse
   vst1q_u8((void*)r, x);
   //printf("r = %s\n", r);
   char answer[] =  {5, 4, 3, 2, 9, 8, 7, 6, 13, 12, 11, 10, 0, 16, 15, 14};
   if (memcmp(r, answer, 16))
     abort();
}
