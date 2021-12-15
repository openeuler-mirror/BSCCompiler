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
  uint32x4_t b128  = { 1,2,3,4 };
  volatile uint16x4_t b64_1 =  vmovn_u32(b128);
  volatile uint16x8_t b64_2 = vmovn_high_u32(b64_1, b128);
  if (vgetq_lane_u16(b64_2,0) != 1 ||
      vgetq_lane_u16(b64_2,1) != 2 ||
      vgetq_lane_u16(b64_2,2) != 3 ||
      vgetq_lane_u16(b64_2,3) != 4)
    abort();

  int32x4_t c128  = { 1,2,3,4 };
  int32x2_t c64_1 = { 10, 20 };
  int64x2_t c64_2 = vmovl_s32(c64_1);
  if (vgetq_lane_s64(c64_2, 0) != 10 ||
      vgetq_lane_s64(c64_2, 1) != 20)
    abort();
  int64x2_t c64_3 = vmovl_high_s32(c128);
  if (vgetq_lane_s64(c64_3, 0) != 3 ||
      vgetq_lane_s64(c64_3, 1) != 4)
    abort();

  uint32x4_t d128 = vdupq_n_u32(10);    // 10,10,10,10
  uint16x4_t d64  = { 1,2 };            // 1,2
  volatile uint32x4_t d128_1 = vaddw_u16(d128, d64);  // 11,12,0,0
  uint16x8_t d128_2 = { 0,0,0,0,13,14,0,0 };
  volatile uint32x4_t d128_3 = vaddw_high_u16(d128_1, d128_2); // 11,12,13,14
  if (vgetq_lane_u32(d128_3, 0) != 24 ||
      vgetq_lane_u32(d128_3, 1) != 26)
    abort();

  int16x4_t e64_1 = vdup_n_s16(20);     // 20,20,20,20
  int16x4_t e64_2 = vdup_n_s16(1);      // 1,1,1,1
  volatile int32x4_t e128 = vaddl_s16(e64_1, e64_2);  // 21,21,21,21
  if (vgetq_lane_s32(e128, 0) != vgetq_lane_s32(e128, 3) ||
      vgetq_lane_s32(e128, 1) != 21)
    abort();
  int32x4_t e128_2 = vdupq_n_s32(2);
  volatile int64x2_t e128_3 = vaddl_high_s32(e128, e128_2);
  if (vgetq_lane_s64(e128_3, 0) != 23 ||
      vgetq_lane_s64(e128_3, 1) != 23)
    abort();
}
