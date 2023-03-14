/*===---- arm_neon.h - ARM Neon intrinsics ---------------------------------===
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 *===-----------------------------------------------------------------------===
 */

#ifndef __ARM_NEON_H
#define __ARM_NEON_H

#include <stdint.h>

typedef float float32_t;
#ifdef __aarch64__
typedef double float64_t;
#endif

typedef __attribute__((neon_vector_type(8))) int8_t int8x8_t;
typedef __attribute__((neon_vector_type(16))) int8_t int8x16_t;
typedef __attribute__((neon_vector_type(4))) int16_t int16x4_t;
typedef __attribute__((neon_vector_type(8))) int16_t int16x8_t;
typedef __attribute__((neon_vector_type(2))) int32_t int32x2_t;
typedef __attribute__((neon_vector_type(4))) int32_t int32x4_t;
typedef __attribute__((neon_vector_type(1))) int64_t int64x1_t;
typedef __attribute__((neon_vector_type(2))) int64_t int64x2_t;
typedef __attribute__((neon_vector_type(8))) uint8_t uint8x8_t;
typedef __attribute__((neon_vector_type(16))) uint8_t uint8x16_t;
typedef __attribute__((neon_vector_type(4))) uint16_t uint16x4_t;
typedef __attribute__((neon_vector_type(8))) uint16_t uint16x8_t;
typedef __attribute__((neon_vector_type(2))) uint32_t uint32x2_t;
typedef __attribute__((neon_vector_type(4))) uint32_t uint32x4_t;
typedef __attribute__((neon_vector_type(1))) uint64_t uint64x1_t;
typedef __attribute__((neon_vector_type(2))) uint64_t uint64x2_t;
typedef __attribute__((neon_vector_type(2))) float32_t float32x2_t;
typedef __attribute__((neon_vector_type(4))) float32_t float32x4_t;
#ifdef __aarch64__
typedef __attribute__((neon_vector_type(1))) float64_t float64x1_t;
typedef __attribute__((neon_vector_type(2))) float64_t float64x2_t;
#endif

typedef struct int8x8x2_t {
  int8x8_t val[2];
} int8x8x2_t;

typedef struct int8x16x2_t {
  int8x16_t val[2];
} int8x16x2_t;

typedef struct int16x4x2_t {
  int16x4_t val[2];
} int16x4x2_t;

typedef struct int16x8x2_t {
  int16x8_t val[2];
} int16x8x2_t;

typedef struct int32x2x2_t {
  int32x2_t val[2];
} int32x2x2_t;

typedef struct int32x4x2_t {
  int32x4_t val[2];
} int32x4x2_t;

typedef struct uint8x8x2_t {
  uint8x8_t val[2];
} uint8x8x2_t;

typedef struct uint8x16x2_t {
  uint8x16_t val[2];
} uint8x16x2_t;

typedef struct uint16x4x2_t {
  uint16x4_t val[2];
} uint16x4x2_t;

typedef struct uint16x8x2_t {
  uint16x8_t val[2];
} uint16x8x2_t;

typedef struct uint32x2x2_t {
  uint32x2_t val[2];
} uint32x2x2_t;

typedef struct uint32x4x2_t {
  uint32x4_t val[2];
} uint32x4x2_t;

typedef struct int64x1x2_t {
  int64x1_t val[2]
} int64x1x2_t;

typedef struct uint64x1x2_t {
  uint64x1_t val[2]
} uint64x1x2_t;

typedef struct int64x2x2_t {
  int64x2_t val[2]
} int64x2x2_t;

typedef struct uint64x2x2_t {
  uint64x2_t val[2]
} uint64x2x2_t;

typedef struct int8x8x3_t {
  int8x8_t val[3];
} int8x8x3_t;

typedef struct int8x16x3_t {
  int8x16_t val[3];
} int8x16x3_t;

typedef struct int16x4x3_t {
  int16x4_t val[3];
} int16x4x3_t;

typedef struct int16x8x3_t {
  int16x8_t val[3];
} int16x8x3_t;

typedef struct int32x2x3_t {
  int32x2_t val[3];
} int32x2x3_t;

typedef struct int32x4x3_t {
  int32x4_t val[3];
} int32x4x3_t;

typedef struct uint8x8x3_t {
  uint8x8_t val[3];
} uint8x8x3_t;

typedef struct uint8x16x3_t {
  uint8x16_t val[3];
} uint8x16x3_t;

typedef struct uint16x4x3_t {
  uint16x4_t val[3];
} uint16x4x3_t;

typedef struct uint16x8x3_t {
  uint16x8_t val[3];
} uint16x8x3_t;

typedef struct uint32x2x3_t {
  uint32x2_t val[3];
} uint32x2x3_t;

typedef struct uint32x4x3_t {
  uint32x4_t val[3];
} uint32x4x3_t;

typedef struct int64x1x3_t {
  int64x1_t val[3]
} int64x1x3_t;

typedef struct uint64x1x3_t {
  uint64x1_t val[3]
} uint64x1x3_t;

typedef struct int64x2x3_t {
  int64x2_t val[3]
} int64x2x3_t;

typedef struct uint64x2x3_t {
  uint64x2_t val[3]
} uint64x2x3_t;

typedef struct int8x8x4_t {
  int8x8_t val[4];
} int8x8x4_t;

typedef struct int8x16x4_t {
  int8x16_t val[4];
} int8x16x4_t;

typedef struct int16x4x4_t {
  int16x4_t val[4];
} int16x4x4_t;

typedef struct int16x8x4_t {
  int16x8_t val[4];
} int16x8x4_t;

typedef struct int32x2x4_t {
  int32x2_t val[4];
} int32x2x4_t;

typedef struct int32x4x4_t {
  int32x4_t val[4];
} int32x4x4_t;

typedef struct uint8x8x4_t {
  uint8x8_t val[4];
} uint8x8x4_t;

typedef struct uint8x16x4_t {
  uint8x16_t val[4];
} uint8x16x4_t;

typedef struct uint16x4x4_t {
  uint16x4_t val[4];
} uint16x4x4_t;

typedef struct uint16x8x4_t {
  uint16x8_t val[4];
} uint16x8x4_t;

typedef struct uint32x2x4_t {
  uint32x2_t val[4];
} uint32x2x4_t;

typedef struct uint32x4x4_t {
  uint32x4_t val[4];
} uint32x4x4_t;

typedef struct int64x1x4_t {
  int64x1_t val[4]
} int64x1x4_t;

typedef struct uint64x1x4_t {
  uint64x1_t val[4]
} uint64x1x4_t;

typedef struct int64x2x4_t {
  int64x2_t val[4]
} int64x2x4_t;

typedef struct uint64x2x4_t {
  uint64x2_t val[4]
} uint64x2x4_t;

typedef struct float32x2x2_t {
  float32x2_t val[2];
} float32x2x2_t;

// vecTy vector_abs(vecTy src)
// Create a vector by getting the absolute value of the elements in src.
int8x8_t __builtin_mpl_vector_abs_v8i8(int8x8_t);
int16x4_t __builtin_mpl_vector_abs_v4i16(int16x4_t);
int32x2_t __builtin_mpl_vector_abs_v2i32(int32x2_t);
int64x1_t __builtin_mpl_vector_abs_v1i64(int64x1_t);
float32x2_t __builtin_mpl_vector_abs_v2f32(float32x2_t);
float64x1_t __builtin_mpl_vector_abs_v1f64(float64x1_t);
int8x16_t __builtin_mpl_vector_abs_v16i8(int8x16_t);
int16x8_t __builtin_mpl_vector_abs_v8i16(int16x8_t);
int32x4_t __builtin_mpl_vector_abs_v4i32(int32x4_t);
int64x2_t __builtin_mpl_vector_abs_v2i64(int64x2_t);
float32x4_t __builtin_mpl_vector_abs_v4f32(float32x4_t);
float64x2_t __builtin_mpl_vector_abs_v2f64(float64x2_t);
int8x16_t __builtin_mpl_vector_absq_v16i8(int8x16_t);
int16x8_t __builtin_mpl_vector_absq_v8i16(int16x8_t);
int32x4_t __builtin_mpl_vector_absq_v4i32(int32x4_t);
int64x2_t __builtin_mpl_vector_absq_v2i64(int64x2_t);

// vecTy vector_abd
int8x8_t __builtin_mpl_vector_abd_v8i8(int8x8_t, int8x8_t);
int8x16_t __builtin_mpl_vector_abdq_v16i8(int8x16_t, int8x16_t);
int16x4_t __builtin_mpl_vector_abd_v4i16(int16x4_t, int16x4_t);
int16x8_t __builtin_mpl_vector_abdq_v8i16(int16x8_t, int16x8_t);
int32x2_t __builtin_mpl_vector_abd_v2i32(int32x2_t, int32x2_t);
int32x4_t __builtin_mpl_vector_abdq_v4i32(int32x4_t, int32x4_t);
uint8x8_t __builtin_mpl_vector_abd_v8u8(uint8x8_t, uint8x8_t);
uint8x16_t __builtin_mpl_vector_abdq_v16u8(uint8x16_t, uint8x16_t);
uint16x4_t __builtin_mpl_vector_abd_v4u16(uint16x4_t, uint16x4_t);
uint16x8_t __builtin_mpl_vector_abdq_v8u16(uint16x8_t, uint16x8_t);
uint32x2_t __builtin_mpl_vector_abd_v2u32(uint32x2_t, uint32x2_t);
uint32x4_t __builtin_mpl_vector_abdq_v4u32(uint32x4_t, uint32x4_t);

// vecTy vector_mov_narrow(vecTy src)
// Copies each element of the operand vector to the corresponding element of the destination vector.
// The result element is half the width of the operand element, and values are saturated to the result width.
// The results are the same type as the operands.

uint8x8_t __builtin_mpl_vector_mov_narrow_v8u16(uint16x8_t);
uint16x4_t __builtin_mpl_vector_mov_narrow_v4u32(uint32x4_t);
uint32x2_t __builtin_mpl_vector_mov_narrow_v2u64(uint64x2_t);
int8x8_t __builtin_mpl_vector_mov_narrow_v8i16(int16x8_t);
int16x4_t __builtin_mpl_vector_mov_narrow_v4i32(int32x4_t);
int32x2_t __builtin_mpl_vector_mov_narrow_v2i64(int64x2_t);

// vecTy vector_mvn
int8x8_t __builtin_mpl_vector_mvn_v8i8(int8x8_t);
int8x16_t __builtin_mpl_vector_mvnq_v16i8(int8x16_t);
uint8x8_t __builtin_mpl_vector_mvn_v8u8(uint8x8_t);
uint8x16_t __builtin_mpl_vector_mvnq_v16u8(uint8x16_t);

// vecTy vector_orn
int8x8_t __builtin_mpl_vector_orn_v8i8(int8x8_t, int8x8_t);
int8x16_t __builtin_mpl_vector_ornq_v16i8(int8x16_t, int8x16_t);
uint8x8_t __builtin_mpl_vector_orn_v8u8(uint8x8_t, uint8x8_t);
uint8x16_t __builtin_mpl_vector_ornq_v16u8(uint8x16_t, uint8x16_t);

// vecTy vector_cls
int8x8_t __builtin_mpl_vector_cls_v8i8(int8x8_t);
int8x16_t __builtin_mpl_vector_clsq_v16i8(int8x16_t);
int16x4_t __builtin_mpl_vector_cls_v4i16(int16x4_t);
int16x8_t __builtin_mpl_vector_clsq_v8i16(int16x8_t);
int32x2_t __builtin_mpl_vector_cls_v2i32(int32x2_t);
int32x4_t __builtin_mpl_vector_clsq_v4i32(int32x4_t);
int8x8_t __builtin_mpl_vector_cls_v8u8(uint8x8_t);
int8x16_t __builtin_mpl_vector_clsq_v16u8(uint8x16_t);
int16x4_t __builtin_mpl_vector_cls_v4u16(uint16x4_t);
int16x8_t __builtin_mpl_vector_clsq_v8u16(uint16x8_t);
int32x2_t __builtin_mpl_vector_cls_v2u32(uint32x2_t);
int32x4_t __builtin_mpl_vector_clsq_v4u32(uint32x4_t);

// vecTy vector_clz
int8x8_t __builtin_mpl_vector_clz_v8i8(int8x8_t);
int8x16_t __builtin_mpl_vector_clzq_v16i8(int8x16_t);
int16x4_t __builtin_mpl_vector_clz_v4i16(int16x4_t);
int16x8_t __builtin_mpl_vector_clzq_v8i16(int16x8_t);
int32x2_t __builtin_mpl_vector_clz_v2i32(int32x2_t);
int32x4_t __builtin_mpl_vector_clzq_v4i32(int32x4_t);
uint8x8_t __builtin_mpl_vector_clz_v8u8(uint8x8_t);
uint8x16_t __builtin_mpl_vector_clzq_v16u8(uint8x16_t);
uint16x4_t __builtin_mpl_vector_clz_v4u16(uint16x4_t);
uint16x8_t __builtin_mpl_vector_clzq_v8u16(uint16x8_t);
uint32x2_t __builtin_mpl_vector_clz_v2u32(uint32x2_t);
uint32x4_t __builtin_mpl_vector_clzq_v4u32(uint32x4_t);

// vecTy vector_cnt
int8x8_t __builtin_mpl_vector_cnt_v8i8(int8x8_t);
int8x16_t __builtin_mpl_vector_cntq_v16i8(int8x16_t);
uint8x8_t __builtin_mpl_vector_cnt_v8u8(uint8x8_t);
uint8x16_t __builtin_mpl_vector_cntq_v16u8(uint8x16_t);

// vecTy vector_bic
int8x8_t __builtin_mpl_vector_bic_v8i8(int8x8_t, int8x8_t);
int8x16_t __builtin_mpl_vector_bicq_v16i8(int8x16_t, int8x16_t);
int16x4_t __builtin_mpl_vector_bic_v4i16(int16x4_t, int16x4_t);
int16x8_t __builtin_mpl_vector_bicq_v8i16(int16x8_t, int16x8_t);
int32x2_t __builtin_mpl_vector_bic_v2i32(int32x2_t, int32x2_t);
int32x4_t __builtin_mpl_vector_bicq_v4i32(int32x4_t, int32x4_t);
int64x1_t __builtin_mpl_vector_bic_v1i64(int64x1_t, int64x1_t);
int64x2_t __builtin_mpl_vector_bicq_v2i64(int64x2_t, int64x2_t);
uint8x8_t __builtin_mpl_vector_bic_v8u8(uint8x8_t, uint8x8_t);
uint8x16_t __builtin_mpl_vector_bicq_v16u8(uint8x16_t, uint8x16_t);
uint16x4_t __builtin_mpl_vector_bic_v4u16(uint16x4_t, uint16x4_t);
uint16x8_t __builtin_mpl_vector_bicq_v8u16(uint16x8_t, uint16x8_t);
uint32x2_t __builtin_mpl_vector_bic_v2u32(uint32x2_t, uint32x2_t);
uint32x4_t __builtin_mpl_vector_bicq_v4u32(uint32x4_t, uint32x4_t);
uint64x1_t __builtin_mpl_vector_bic_v1u64(uint64x1_t, uint64x1_t);
uint64x2_t __builtin_mpl_vector_bicq_v2u64(uint64x2_t, uint64x2_t);

// vecTy vector_bsl
int8x8_t __builtin_mpl_vector_bsl_v8i8(uint8x8_t, int8x8_t, int8x8_t);
int8x16_t __builtin_mpl_vector_bslq_v16i8(uint8x16_t, int8x16_t, int8x16_t);
uint8x8_t __builtin_mpl_vector_bsl_v8u8(uint8x8_t, uint8x8_t, uint8x8_t);
uint8x16_t __builtin_mpl_vector_bslq_v16u8(uint8x16_t, uint8x16_t, uint8x16_t);

// vecTy2 vector_copy_lane
int8x8_t __builtin_mpl_vector_copy_lane_v8i8(int8x8_t, const int, int8x8_t, const int);
int8x16_t __builtin_mpl_vector_copyq_lane_v16i8(int8x16_t, const int, int8x8_t, const int);
int16x4_t __builtin_mpl_vector_copy_lane_v4i16(int16x4_t, const int, int16x4_t, const int);
int16x8_t __builtin_mpl_vector_copyq_lane_v8i16(int16x8_t, const int, int16x4_t, const int);
int32x2_t __builtin_mpl_vector_copy_lane_v2i32(int32x2_t, const int, int32x2_t, const int);
int32x4_t __builtin_mpl_vector_copyq_lane_v4i32(int32x4_t, const int, int32x2_t, const int);
int64x1_t __builtin_mpl_vector_copy_lane_v1i64(int64x1_t, const int, int64x1_t, const int);
int64x2_t __builtin_mpl_vector_copyq_lane_v2i64(int64x2_t, const int, int64x1_t, const int);
uint8x8_t __builtin_mpl_vector_copy_lane_v8u8(uint8x8_t, const int, uint8x8_t, const int);
uint8x16_t __builtin_mpl_vector_copyq_lane_v16u8(uint8x16_t, const int, uint8x8_t, const int);
uint16x4_t __builtin_mpl_vector_copy_lane_v4u16(uint16x4_t, const int, uint16x4_t, const int);
uint16x8_t __builtin_mpl_vector_copyq_lane_v8u16(uint16x8_t, const int, uint16x4_t, const int);
uint32x2_t __builtin_mpl_vector_copy_lane_v2u32(uint32x2_t, const int, uint32x2_t, const int);
uint32x4_t __builtin_mpl_vector_copyq_lane_v4u32(uint32x4_t, const int, uint32x2_t, const int);
uint64x1_t __builtin_mpl_vector_copy_lane_v1u64(uint64x1_t, const int, uint64x1_t, const int);
uint64x2_t __builtin_mpl_vector_copyq_lane_v2u64(uint64x2_t, const int, uint64x1_t, const int);
int8x8_t __builtin_mpl_vector_copy_laneq_v8i8(int8x8_t, const int, int8x16_t, const int);
int8x16_t __builtin_mpl_vector_copyq_laneq_v16i8(int8x16_t, const int, int8x16_t, const int);
int16x4_t __builtin_mpl_vector_copy_laneq_v4i16(int16x4_t, const int, int16x8_t, const int);
int16x8_t __builtin_mpl_vector_copyq_laneq_v8i16(int16x8_t, const int, int16x8_t, const int);
int32x2_t __builtin_mpl_vector_copy_laneq_v2i32(int32x2_t, const int, int32x4_t, const int);
int32x4_t __builtin_mpl_vector_copyq_laneq_v4i32(int32x4_t, const int, int32x4_t, const int);
int64x1_t __builtin_mpl_vector_copy_laneq_v1i64(int64x1_t, const int, int64x2_t, const int);
int64x2_t __builtin_mpl_vector_copyq_laneq_v2i64(int64x2_t, const int, int64x2_t, const int);
uint8x8_t __builtin_mpl_vector_copy_laneq_v8u8(uint8x8_t, const int, uint8x16_t, const int);
uint8x16_t __builtin_mpl_vector_copyq_laneq_v16u8(uint8x16_t, const int, uint8x16_t, const int);
uint16x4_t __builtin_mpl_vector_copy_laneq_v4u16(uint16x4_t, const int, uint16x8_t, const int);
uint16x8_t __builtin_mpl_vector_copyq_laneq_v8u16(uint16x8_t, const int, uint16x8_t, const int);
uint32x2_t __builtin_mpl_vector_copy_laneq_v2u32(uint32x2_t, const int, uint32x4_t, const int);
uint32x4_t __builtin_mpl_vector_copyq_laneq_v4u32(uint32x4_t, const int, uint32x4_t, const int);
uint64x1_t __builtin_mpl_vector_copy_laneq_v1u64(uint64x1_t, const int, uint64x2_t, const int);
uint64x2_t __builtin_mpl_vector_copyq_laneq_v2u64(uint64x2_t, const int, uint64x2_t, const int);

// vecTy vector_rbit
int8x8_t __builtin_mpl_vector_rbit_v8i8(int8x8_t);
int8x16_t __builtin_mpl_vector_rbitq_v16i8(int8x16_t);
uint8x8_t __builtin_mpl_vector_rbit_v8u8(uint8x8_t);
uint8x16_t __builtin_mpl_vector_rbitq_v16u8(uint8x16_t);

// vecTy vector_create
int8x8_t __builtin_mpl_vector_create_v8i8(uint64_t);
int16x4_t __builtin_mpl_vector_create_v4i16(uint64_t);
int32x2_t __builtin_mpl_vector_create_v2i32(uint64_t);
int64x1_t __builtin_mpl_vector_create_v1i64(uint64_t);
uint8x8_t __builtin_mpl_vector_create_v8u8(uint64_t);
uint16x4_t __builtin_mpl_vector_create_v4u16(uint64_t);
uint32x2_t __builtin_mpl_vector_create_v2u32(uint64_t);
uint64x1_t __builtin_mpl_vector_create_v1u64(uint64_t);

// vecTy vector_mov_n
int8x8_t __builtin_mpl_vector_mov_n_v8i8(int8_t);
int8x16_t __builtin_mpl_vector_movq_n_v16i8(int8_t);
int16x4_t __builtin_mpl_vector_mov_n_v4i16(int16_t);
int16x8_t __builtin_mpl_vector_movq_n_v8i16(int16_t);
int32x2_t __builtin_mpl_vector_mov_n_v2i32(int32_t);
int32x4_t __builtin_mpl_vector_movq_n_v4i32(int32_t);
int64x1_t __builtin_mpl_vector_mov_n_v1i64(int64_t);
int64x2_t __builtin_mpl_vector_movq_n_v2i64(int64_t);
uint8x8_t __builtin_mpl_vector_mov_n_v8u8(uint8_t);
uint8x16_t __builtin_mpl_vector_movq_n_v16u8(uint8_t);
uint16x4_t __builtin_mpl_vector_mov_n_v4u16(uint16_t);
uint16x8_t __builtin_mpl_vector_movq_n_v8u16(uint16_t);
uint32x4_t __builtin_mpl_vector_movq_n_v4u32(uint32_t);
uint32x2_t __builtin_mpl_vector_mov_n_v2u32(uint32_t);
uint64x1_t __builtin_mpl_vector_mov_n_v1u64(uint64_t);
uint64x2_t __builtin_mpl_vector_movq_n_v2u64(uint64_t);

// vecTy vector_dup_lane
int8x8_t __builtin_mpl_vector_dup_lane_v8i8(int8x8_t, const int);
int8x16_t __builtin_mpl_vector_dupq_lane_v16i8(int8x8_t, const int);
int16x4_t __builtin_mpl_vector_dup_lane_v4i16(int16x4_t, const int);
int16x8_t __builtin_mpl_vector_dupq_lane_v8i16(int16x4_t, const int);
int32x2_t __builtin_mpl_vector_dup_lane_v2i32(int32x2_t, const int);
int32x4_t __builtin_mpl_vector_dupq_lane_v4i32(int32x2_t, const int);
int64x1_t __builtin_mpl_vector_dup_lane_v1i64(int64x1_t, const int);
int64x2_t __builtin_mpl_vector_dupq_lane_v2i64(int64x1_t, const int);
uint8x8_t __builtin_mpl_vector_dup_lane_v8u8(uint8x8_t, const int);
uint8x16_t __builtin_mpl_vector_dupq_lane_v16u8(uint8x8_t, const int);
uint16x4_t __builtin_mpl_vector_dup_lane_v4u16(uint16x4_t, const int);
uint16x8_t __builtin_mpl_vector_dupq_lane_v8u16(uint16x4_t, const int);
uint32x4_t __builtin_mpl_vector_dupq_lane_v4u32(uint32x2_t, const int);
uint64x1_t __builtin_mpl_vector_dup_lane_v1u64(uint64x1_t, const int);
uint64x2_t __builtin_mpl_vector_dupq_lane_v2u64(uint64x1_t, const int);
int8x8_t __builtin_mpl_vector_dup_laneq_v8i8(int8x16_t, const int);
int8x16_t __builtin_mpl_vector_dupq_laneq_v16i8(int8x16_t, const int);
int16x4_t __builtin_mpl_vector_dup_laneq_v4i16(int16x8_t, const int);
int16x8_t __builtin_mpl_vector_dupq_laneq_v8i16(int16x8_t, const int);
int32x2_t __builtin_mpl_vector_dup_laneq_v2i32(int32x4_t, const int);
int32x4_t __builtin_mpl_vector_dupq_laneq_v4i32(int32x4_t, const int);
int64x1_t __builtin_mpl_vector_dup_laneq_v1i64(int64x2_t, const int);
int64x2_t __builtin_mpl_vector_dupq_laneq_v2i64(int64x2_t, const int);
uint8x8_t __builtin_mpl_vector_dup_laneq_v8u8(uint8x16_t, const int);
uint8x16_t __builtin_mpl_vector_dupq_laneq_v16u8(uint8x16_t, const int);
uint16x4_t __builtin_mpl_vector_dup_laneq_v4u16(uint16x8_t, const int);
uint16x8_t __builtin_mpl_vector_dupq_laneq_v8u16(uint16x8_t, const int);
uint32x2_t __builtin_mpl_vector_dup_laneq_v2u32(uint32x4_t, const int);
uint32x4_t __builtin_mpl_vector_dupq_laneq_v4u32(uint32x4_t, const int);
uint64x1_t __builtin_mpl_vector_dup_laneq_v1u64(uint64x2_t, const int);
uint64x2_t __builtin_mpl_vector_dupq_laneq_v2u64(uint64x2_t, const int);

// vecTy vector_combine
int64x2_t __builtin_mpl_vector_combine_v2i64(int64x1_t, int64x1_t);
uint64x2_t __builtin_mpl_vector_combine_v2u64(uint64x1_t, uint64x1_t);

// vecTy vector_dup_lane
int8_t __builtin_mpl_vector_dupb_lane_v8i8(int8x8_t, const int);
int16_t __builtin_mpl_vector_duph_lane_v4i16(int16x4_t, const int);
int32_t __builtin_mpl_vector_dups_lane_v2i32(int32x2_t, const int);
int64_t __builtin_mpl_vector_dupd_lane_v1i64(int64x1_t, const int);
uint8_t __builtin_mpl_vector_dupb_lane_v8u8(uint8x8_t, const int);
uint16_t __builtin_mpl_vector_duph_lane_v4u16(uint16x4_t, const int);
uint32_t __builtin_mpl_vector_dups_lane_v2u32(uint32x2_t, const int);
uint64_t __builtin_mpl_vector_dupd_lane_v1u64(uint64x1_t, const int);
int8_t __builtin_mpl_vector_dupb_laneq_v16i8(int8x16_t, const int);
int16_t __builtin_mpl_vector_duph_laneq_v8i16(int16x8_t, const int);
int32_t __builtin_mpl_vector_dups_laneq_v4i32(int32x4_t, const int);
int64_t __builtin_mpl_vector_dupd_laneq_v2i64(int64x2_t, const int);
uint8_t __builtin_mpl_vector_dupb_laneq_v16u8(uint8x16_t, const int);
uint16_t __builtin_mpl_vector_duph_laneq_v8u16(uint16x8_t, const int);
uint32_t __builtin_mpl_vector_dups_laneq_v4u32(uint32x4_t, const int);
uint64_t __builtin_mpl_vector_dupd_laneq_v2u64(uint64x2_t, const int);

// vecTy vector_rev
int8x8_t __builtin_mpl_vector_rev64_v8i8(int8x8_t);
int8x16_t __builtin_mpl_vector_rev64q_v16i8(int8x16_t);
int16x4_t __builtin_mpl_vector_rev64_v4i16(int16x4_t);
int16x8_t __builtin_mpl_vector_rev64q_v8i16(int16x8_t);
int32x2_t __builtin_mpl_vector_rev64_v2i32(int32x2_t);
int32x4_t __builtin_mpl_vector_rev64q_v4i32(int32x4_t);
uint8x8_t __builtin_mpl_vector_rev64_v8u8(uint8x8_t);
uint8x16_t __builtin_mpl_vector_rev64q_v16u8(uint8x16_t);
uint16x4_t __builtin_mpl_vector_rev64_v4u16(uint16x4_t);
uint16x8_t __builtin_mpl_vector_rev64q_v8u16(uint16x8_t);
uint32x2_t __builtin_mpl_vector_rev64_v2u32(uint32x2_t);
uint32x4_t __builtin_mpl_vector_rev64q_v4u32(uint32x4_t);
int8x8_t __builtin_mpl_vector_rev16_v8i8(int8x8_t);
int8x16_t __builtin_mpl_vector_rev16q_v16i8(int8x16_t);
uint8x8_t __builtin_mpl_vector_rev16_v8u8(uint8x8_t);
uint8x16_t __builtin_mpl_vector_rev16q_v16u8(uint8x16_t);

// vecTy vector_zip1
int8x8_t __builtin_mpl_vector_zip1_v8i8(int8x8_t, int8x8_t);
int8x16_t __builtin_mpl_vector_zip1q_v16i8(int8x16_t, int8x16_t);
int16x4_t __builtin_mpl_vector_zip1_v4i16(int16x4_t, int16x4_t);
int16x8_t __builtin_mpl_vector_zip1q_v8i16(int16x8_t, int16x8_t);
int32x2_t __builtin_mpl_vector_zip1_v2i32(int32x2_t, int32x2_t);
int32x4_t __builtin_mpl_vector_zip1q_v4i32(int32x4_t, int32x4_t);
int64x2_t __builtin_mpl_vector_zip1q_v2i64(int64x2_t, int64x2_t);
uint8x8_t __builtin_mpl_vector_zip1_v8u8(uint8x8_t, uint8x8_t);
uint8x16_t __builtin_mpl_vector_zip1q_v16u8(uint8x16_t, uint8x16_t);
uint16x4_t __builtin_mpl_vector_zip1_v4u16(uint16x4_t, uint16x4_t);
uint16x8_t __builtin_mpl_vector_zip1q_v8u16(uint16x8_t, uint16x8_t);
uint32x2_t __builtin_mpl_vector_zip1_v2u32(uint32x2_t, uint32x2_t);
uint32x4_t __builtin_mpl_vector_zip1q_v4u32(uint32x4_t, uint32x4_t);
uint64x2_t __builtin_mpl_vector_zip1q_v2u64(uint64x2_t, uint64x2_t);

// vecTy vector_zip2
int8x8_t __builtin_mpl_vector_zip2_v8i8(int8x8_t, int8x8_t);
int8x16_t __builtin_mpl_vector_zip2q_v16i8(int8x16_t, int8x16_t);
int16x4_t __builtin_mpl_vector_zip2_v4i16(int16x4_t, int16x4_t);
int16x8_t __builtin_mpl_vector_zip2q_v8i16(int16x8_t, int16x8_t);
int32x2_t __builtin_mpl_vector_zip2_v2i32(int32x2_t, int32x2_t);
int32x4_t __builtin_mpl_vector_zip2q_v4i32(int32x4_t, int32x4_t);
int64x2_t __builtin_mpl_vector_zip2q_v2i64(int64x2_t, int64x2_t);
uint8x8_t __builtin_mpl_vector_zip2_v8u8(uint8x8_t, uint8x8_t);
uint8x16_t __builtin_mpl_vector_zip2q_v16u8(uint8x16_t, uint8x16_t);
uint16x4_t __builtin_mpl_vector_zip2_v4u16(uint16x4_t, uint16x4_t);
uint16x8_t __builtin_mpl_vector_zip2q_v8u16(uint16x8_t, uint16x8_t);
uint32x2_t __builtin_mpl_vector_zip2_v2u32(uint32x2_t, uint32x2_t);
uint32x4_t __builtin_mpl_vector_zip2q_v4u32(uint32x4_t, uint32x4_t);
uint64x2_t __builtin_mpl_vector_zip2q_v2u64(uint64x2_t, uint64x2_t);

// vecTy vector_uzp1
int8x8_t __builtin_mpl_vector_uzp1_v8i8(int8x8_t, int8x8_t);
int8x16_t __builtin_mpl_vector_uzp1q_v16i8(int8x16_t, int8x16_t);
int16x4_t __builtin_mpl_vector_uzp1_v4i16(int16x4_t, int16x4_t);
int16x8_t __builtin_mpl_vector_uzp1q_v8i16(int16x8_t, int16x8_t);
int32x2_t __builtin_mpl_vector_uzp1_v2i32(int32x2_t, int32x2_t);
int32x4_t __builtin_mpl_vector_uzp1q_v4i32(int32x4_t, int32x4_t);
int64x2_t __builtin_mpl_vector_uzp1q_v2i64(int64x2_t, int64x2_t);
uint8x8_t __builtin_mpl_vector_uzp1_v8u8(uint8x8_t, uint8x8_t);
uint8x16_t __builtin_mpl_vector_uzp1q_v16u8(uint8x16_t, uint8x16_t);
uint16x4_t __builtin_mpl_vector_uzp1_v4u16(uint16x4_t, uint16x4_t);
uint16x8_t __builtin_mpl_vector_uzp1q_v8u16(uint16x8_t, uint16x8_t);
uint32x2_t __builtin_mpl_vector_uzp1_v2u32(uint32x2_t, uint32x2_t);
uint32x4_t __builtin_mpl_vector_uzp1q_v4u32(uint32x4_t, uint32x4_t);
uint64x2_t __builtin_mpl_vector_uzp1q_v2u64(uint64x2_t, uint64x2_t);

// vecTy vector_uzp2
int8x8_t __builtin_mpl_vector_uzp2_v8i8(int8x8_t, int8x8_t);
int8x16_t __builtin_mpl_vector_uzp2q_v16i8(int8x16_t, int8x16_t);
int16x4_t __builtin_mpl_vector_uzp2_v4i16(int16x4_t, int16x4_t);
int16x8_t __builtin_mpl_vector_uzp2q_v8i16(int16x8_t, int16x8_t);
int32x2_t __builtin_mpl_vector_uzp2_v2i32(int32x2_t, int32x2_t);
int32x4_t __builtin_mpl_vector_uzp2q_v4i32(int32x4_t, int32x4_t);
int64x2_t __builtin_mpl_vector_uzp2q_v2i64(int64x2_t, int64x2_t);
uint8x16_t __builtin_mpl_vector_uzp2q_v16u8(uint8x16_t, uint8x16_t);
uint8x8_t __builtin_mpl_vector_uzp2_v8u8(uint8x8_t, uint8x8_t);
uint16x4_t __builtin_mpl_vector_uzp2_v4u16(uint16x4_t, uint16x4_t);
uint16x8_t __builtin_mpl_vector_uzp2q_v8u16(uint16x8_t, uint16x8_t);
uint32x2_t __builtin_mpl_vector_uzp2_v2u32(uint32x2_t, uint32x2_t);
uint32x4_t __builtin_mpl_vector_uzp2q_v4u32(uint32x4_t, uint32x4_t);
uint64x2_t __builtin_mpl_vector_uzp2q_v2u64(uint64x2_t, uint64x2_t);

// vecTy vector_trn1
int8x8_t __builtin_mpl_vector_trn1_v8i8(int8x8_t, int8x8_t);
int8x16_t __builtin_mpl_vector_trn1q_v16i8(int8x16_t, int8x16_t);
int16x4_t __builtin_mpl_vector_trn1_v4i16(int16x4_t, int16x4_t);
int16x8_t __builtin_mpl_vector_trn1q_v8i16(int16x8_t, int16x8_t);
int32x2_t __builtin_mpl_vector_trn1_v2i32(int32x2_t, int32x2_t);
int32x4_t __builtin_mpl_vector_trn1q_v4i32(int32x4_t, int32x4_t);
int64x2_t __builtin_mpl_vector_trn1q_v2i64(int64x2_t, int64x2_t);
uint8x8_t __builtin_mpl_vector_trn1_v8u8(uint8x8_t, uint8x8_t);
uint8x16_t __builtin_mpl_vector_trn1q_v16u8(uint8x16_t, uint8x16_t);
uint16x4_t __builtin_mpl_vector_trn1_v4u16(uint16x4_t, uint16x4_t);
uint16x8_t __builtin_mpl_vector_trn1q_v8u16(uint16x8_t, uint16x8_t);
uint32x2_t __builtin_mpl_vector_trn1_v2u32(uint32x2_t, uint32x2_t);
uint32x4_t __builtin_mpl_vector_trn1q_v4u32(uint32x4_t, uint32x4_t);
uint64x2_t __builtin_mpl_vector_trn1q_v2u64(uint64x2_t, uint64x2_t);

// vecTy vector_trn2
int8x8_t __builtin_mpl_vector_trn2_v8i8(int8x8_t, int8x8_t);
int8x16_t __builtin_mpl_vector_trn2q_v16i8(int8x16_t, int8x16_t);
int16x4_t __builtin_mpl_vector_trn2_v4i16(int16x4_t, int16x4_t);
int16x8_t __builtin_mpl_vector_trn2q_v8i16(int16x8_t, int16x8_t);
int32x2_t __builtin_mpl_vector_trn2_v2i32(int32x2_t, int32x2_t);
int32x4_t __builtin_mpl_vector_trn2q_v4i32(int32x4_t, int32x4_t);
int64x2_t __builtin_mpl_vector_trn2q_v2i64(int64x2_t, int64x2_t);
uint8x8_t __builtin_mpl_vector_trn2_v8u8(uint8x8_t, uint8x8_t);
uint8x16_t __builtin_mpl_vector_trn2q_v16u8(uint8x16_t, uint8x16_t);
uint16x4_t __builtin_mpl_vector_trn2_v4u16(uint16x4_t, uint16x4_t);
uint16x8_t __builtin_mpl_vector_trn2q_v8u16(uint16x8_t, uint16x8_t);
uint32x2_t __builtin_mpl_vector_trn2_v2u32(uint32x2_t, uint32x2_t);
uint32x4_t __builtin_mpl_vector_trn2q_v4u32(uint32x4_t, uint32x4_t);
uint64x2_t __builtin_mpl_vector_trn2q_v2u64(uint64x2_t, uint64x2_t);

// vecTy vector_ld1
int8x8_t __builtin_mpl_vector_ld1_v8i8(int8_t const *);
int8x16_t __builtin_mpl_vector_ld1q_v16i8(int8_t const *);
int16x4_t __builtin_mpl_vector_ld1_v4i16(int16_t const *);
int16x8_t __builtin_mpl_vector_ld1q_v8i16(int16_t const *);
int32x2_t __builtin_mpl_vector_ld1_v2i32(int32_t const *);
int32x4_t __builtin_mpl_vector_ld1q_v4i32(int32_t const *);
int64x1_t __builtin_mpl_vector_ld1_v1i64(int64_t const *);
int64x2_t __builtin_mpl_vector_ld1q_v2i64(int64_t const *);
uint8x8_t __builtin_mpl_vector_ld1_v8u8(uint8_t const *);
uint8x16_t __builtin_mpl_vector_ld1q_v16u8(uint8_t const *);
uint16x4_t __builtin_mpl_vector_ld1_v4u16(uint16_t const *);
uint16x8_t __builtin_mpl_vector_ld1q_v8u16(uint16_t const *);
uint32x2_t __builtin_mpl_vector_ld1_v2u32(uint32_t const *);
uint32x4_t __builtin_mpl_vector_ld1q_v4u32(uint32_t const *);
uint64x1_t __builtin_mpl_vector_ld1_v1u64(uint64_t const *);
uint64x2_t __builtin_mpl_vector_ld1q_v2u64(uint64_t const *);

// vecTy vector_ld1_lane
int8x8_t __builtin_mpl_vector_ld1_lane_v8i8(int8_t const *, int8x8_t, const int);
int8x16_t __builtin_mpl_vector_ld1q_lane_v16i8(int8_t const *, int8x16_t, const int);
int16x4_t __builtin_mpl_vector_ld1_lane_v4i16(int16_t const *, int16x4_t, const int);
int16x8_t __builtin_mpl_vector_ld1q_lane_v8i16(int16_t const *, int16x8_t, const int);
int32x2_t __builtin_mpl_vector_ld1_lane_v2i32(int32_t const *, int32x2_t, const int);
int32x4_t __builtin_mpl_vector_ld1q_lane_v4i32(int32_t const *, int32x4_t, const int);
int64x1_t __builtin_mpl_vector_ld1_lane_v1i64(int64_t const *, int64x1_t, const int);
int64x2_t __builtin_mpl_vector_ld1q_lane_v2i64(int64_t const *, int64x2_t, const int);
uint8x8_t __builtin_mpl_vector_ld1_lane_v8u8(uint8_t const *, uint8x8_t, const int);
uint8x16_t __builtin_mpl_vector_ld1q_lane_v16u8(uint8_t const *, uint8x16_t, const int);
uint16x4_t __builtin_mpl_vector_ld1_lane_v4u16(uint16_t const *, uint16x4_t, const int);
uint16x8_t __builtin_mpl_vector_ld1q_lane_v8u16(uint16_t const *, uint16x8_t, const int);
uint32x2_t __builtin_mpl_vector_ld1_lane_v2u32(uint32_t const *, uint32x2_t, const int);
uint32x4_t __builtin_mpl_vector_ld1q_lane_v4u32(uint32_t const *, uint32x4_t, const int);
uint64x1_t __builtin_mpl_vector_ld1_lane_v1u64(uint64_t const *, uint64x1_t, const int);
uint64x2_t __builtin_mpl_vector_ld1q_lane_v2u64(uint64_t const *, uint64x2_t, const int);

// vecTy vector_ld1_dup
int8x8_t __builtin_mpl_vector_ld1_dup_v8i8(int8_t const *);
int8x16_t __builtin_mpl_vector_ld1q_dup_v16i8(int8_t const *);
int16x4_t __builtin_mpl_vector_ld1_dup_v4i16(int16_t const *);
int16x8_t __builtin_mpl_vector_ld1q_dup_v8i16(int16_t const *);
int32x2_t __builtin_mpl_vector_ld1_dup_v2i32(int32_t const *);
int32x4_t __builtin_mpl_vector_ld1q_dup_v4i32(int32_t const *);
int64x1_t __builtin_mpl_vector_ld1_dup_v1i64(int64_t const *);
int64x2_t __builtin_mpl_vector_ld1q_dup_v2i64(int64_t const *);
uint8x8_t __builtin_mpl_vector_ld1_dup_v8u8(uint8_t const *);
uint8x16_t __builtin_mpl_vector_ld1q_dup_v16u8(uint8_t const *);
uint16x4_t __builtin_mpl_vector_ld1_dup_v4u16(uint16_t const *);
uint16x8_t __builtin_mpl_vector_ld1q_dup_v8u16(uint16_t const *);
uint32x2_t __builtin_mpl_vector_ld1_dup_v2u32(uint32_t const *);
uint32x4_t __builtin_mpl_vector_ld1q_dup_v4u32(uint32_t const *);
uint64x1_t __builtin_mpl_vector_ld1_dup_v1u64(uint64_t const *);
uint64x2_t __builtin_mpl_vector_ld1q_dup_v2u64(uint64_t const *);

// vecTy vector_ld2
int8x8x2_t __builtin_mpl_vector_ld2_v8i8(int8_t const *);
int8x16x2_t __builtin_mpl_vector_ld2q_v16i8(int8_t const *);
int16x4x2_t __builtin_mpl_vector_ld2_v4i16(int16_t const *);
int16x8x2_t __builtin_mpl_vector_ld2q_v8i16(int16_t const *);
int32x2x2_t __builtin_mpl_vector_ld2_v2i32(int32_t const *);
int32x4x2_t __builtin_mpl_vector_ld2q_v4i32(int32_t const *);
uint8x8x2_t __builtin_mpl_vector_ld2_v8u8(uint8_t const *);
uint8x16x2_t __builtin_mpl_vector_ld2q_v16u8(uint8_t const *);
uint16x4x2_t __builtin_mpl_vector_ld2_v4u16(uint16_t const *);
uint16x8x2_t __builtin_mpl_vector_ld2q_v8u16(uint16_t const *);
uint32x2x2_t __builtin_mpl_vector_ld2_v2u32(uint32_t const *);
uint32x4x2_t __builtin_mpl_vector_ld2q_v4u32(uint32_t const *);
int64x1x2_t __builtin_mpl_vector_ld2_v1i64(int64_t const *);
uint64x1x2_t __builtin_mpl_vector_ld2_v1u64(uint64_t const *);
int64x2x2_t __builtin_mpl_vector_ld2q_v2i64(int64_t const *);
uint64x2x2_t __builtin_mpl_vector_ld2q_v2u64(uint64_t const *);

// vecTy vector_ld3
int8x8x3_t __builtin_mpl_vector_ld3_v8i8(int8_t const *);
int8x16x3_t __builtin_mpl_vector_ld3q_v16i8(int8_t const *);
int16x4x3_t __builtin_mpl_vector_ld3_v4i16(int16_t const *);
int16x8x3_t __builtin_mpl_vector_ld3q_v8i16(int16_t const *);
int32x2x3_t __builtin_mpl_vector_ld3_v2i32(int32_t const *);
int32x4x3_t __builtin_mpl_vector_ld3q_v4i32(int32_t const *);
uint8x8x3_t __builtin_mpl_vector_ld3_v8u8(uint8_t const *);
uint8x16x3_t __builtin_mpl_vector_ld3q_v16u8(uint8_t const *);
uint16x4x3_t __builtin_mpl_vector_ld3_v4u16(uint16_t const *);
uint16x8x3_t __builtin_mpl_vector_ld3q_v8u16(uint16_t const *);
uint32x2x3_t __builtin_mpl_vector_ld3_v2u32(uint32_t const *);
uint32x4x3_t __builtin_mpl_vector_ld3q_v4u32(uint32_t const *);
int64x1x3_t __builtin_mpl_vector_ld3_v1i64(int64_t const *);
uint64x1x3_t __builtin_mpl_vector_ld3_v1u64(uint64_t const *);
int64x2x3_t __builtin_mpl_vector_ld3q_v2i64(int64_t const *);
uint64x2x3_t __builtin_mpl_vector_ld3q_v2u64(uint64_t const *);

// vecTy vector_ld4
int8x8x4_t __builtin_mpl_vector_ld4_v8i8(int8_t const *);
int8x16x4_t __builtin_mpl_vector_ld4q_v16i8(int8_t const *);
int16x4x4_t __builtin_mpl_vector_ld4_v4i16(int16_t const *);
int16x8x4_t __builtin_mpl_vector_ld4q_v8i16(int16_t const *);
int32x2x4_t __builtin_mpl_vector_ld4_v2i32(int32_t const *);
int32x4x4_t __builtin_mpl_vector_ld4q_v4i32(int32_t const *);
uint8x8x4_t __builtin_mpl_vector_ld4_v8u8(uint8_t const *);
uint8x16x4_t __builtin_mpl_vector_ld4q_v16u8(uint8_t const *);
uint16x4x4_t __builtin_mpl_vector_ld4_v4u16(uint16_t const *);
uint16x8x4_t __builtin_mpl_vector_ld4q_v8u16(uint16_t const *);
uint32x2x4_t __builtin_mpl_vector_ld4_v2u32(uint32_t const *);
uint32x4x4_t __builtin_mpl_vector_ld4q_v4u32(uint32_t const *);
int64x1x4_t __builtin_mpl_vector_ld4_v1i64(int64_t const *);
uint64x1x4_t __builtin_mpl_vector_ld4_v1u64(uint64_t const *);
int64x2x4_t __builtin_mpl_vector_ld4q_v2i64(int64_t const *);
uint64x2x4_t __builtin_mpl_vector_ld4q_v2u64(uint64_t const *);

// vecTy vector_addl_low(vecTy src1, vecTy src2)
// Add each element of the source vector to second source widen the result into the destination vector.

int16x8_t __builtin_mpl_vector_addl_low_v8i8(int8x8_t, int8x8_t);
int32x4_t __builtin_mpl_vector_addl_low_v4i16(int16x4_t, int16x4_t);
int64x2_t __builtin_mpl_vector_addl_low_v2i32(int32x2_t, int32x2_t);
uint16x8_t __builtin_mpl_vector_addl_low_v8u8(uint8x8_t, uint8x8_t);
uint32x4_t __builtin_mpl_vector_addl_low_v4u16(uint16x4_t, uint16x4_t);
uint64x2_t __builtin_mpl_vector_addl_low_v2u32(uint32x2_t, uint32x2_t);

// vecTy vector_addl_high(vecTy src1, vecTy src2)
// Add each element of the source vector to upper half of second source widen the result into the destination vector.

int16x8_t __builtin_mpl_vector_addl_high_v8i8(int8x16_t, int8x16_t);
int32x4_t __builtin_mpl_vector_addl_high_v4i16(int16x8_t, int16x8_t);
int64x2_t __builtin_mpl_vector_addl_high_v2i32(int32x4_t, int32x4_t);
uint16x8_t __builtin_mpl_vector_addl_high_v8u8(uint8x16_t, uint8x16_t);
uint32x4_t __builtin_mpl_vector_addl_high_v4u16(uint16x8_t, uint16x8_t);
uint64x2_t __builtin_mpl_vector_addl_high_v2u32(uint32x4_t, uint32x4_t);

// vecTy vector_addw_low(vecTy src1, vecTy src2)
// Add each element of the source vector to second source widen the result into the destination vector.

int16x8_t __builtin_mpl_vector_addw_low_v8i8(int16x8_t, int8x8_t);
int32x4_t __builtin_mpl_vector_addw_low_v4i16(int32x4_t, int16x4_t);
int64x2_t __builtin_mpl_vector_addw_low_v2i32(int64x2_t, int32x2_t);
uint16x8_t __builtin_mpl_vector_addw_low_v8u8(uint16x8_t, uint8x8_t);
uint32x4_t __builtin_mpl_vector_addw_low_v4u16(uint32x4_t, uint16x4_t);
uint64x2_t __builtin_mpl_vector_addw_low_v2u32(uint64x2_t, uint32x2_t);

// vecTy vector_addw_high(vecTy src1, vecTy src2)
// Add each element of the source vector to upper half of second source widen the result into the destination vector.

int16x8_t __builtin_mpl_vector_addw_high_v8i8(int16x8_t, int8x16_t);
int32x4_t __builtin_mpl_vector_addw_high_v4i16(int32x4_t, int16x8_t);
int64x2_t __builtin_mpl_vector_addw_high_v2i32(int64x2_t, int32x4_t);
uint16x8_t __builtin_mpl_vector_addw_high_v8u8(uint16x8_t, uint8x16_t);
uint32x4_t __builtin_mpl_vector_addw_high_v4u16(uint32x4_t, uint16x8_t);
uint64x2_t __builtin_mpl_vector_addw_high_v2u32(uint64x2_t, uint32x4_t);

// vectTy vector_recpe
uint32x2_t __builtin_mpl_vector_recpe_v2u32(uint32x2_t);
uint32x4_t __builtin_mpl_vector_recpeq_v4u32(uint32x4_t);

// vectTy vector_pdd
int32x2_t __builtin_mpl_vector_padd_v2i32(int32x2_t, int32x2_t);
uint32x2_t __builtin_mpl_vector_padd_v2u32(uint32x2_t, uint32x2_t);
int8x8_t __builtin_mpl_vector_padd_v8i8(int8x8_t, int8x8_t);
int16x4_t __builtin_mpl_vector_padd_v4i16(int16x4_t, int16x4_t);
uint8x8_t __builtin_mpl_vector_padd_v8u8(uint8x8_t, uint8x8_t);
uint16x4_t __builtin_mpl_vector_padd_v4u16(uint16x4_t, uint16x4_t);
int8x16_t __builtin_mpl_vector_paddq_v16i8(int8x16_t, int8x16_t);
int16x8_t __builtin_mpl_vector_paddq_v8i16(int16x8_t, int16x8_t);
int32x4_t __builtin_mpl_vector_paddq_v4i32(int32x4_t, int32x4_t);
int64x2_t __builtin_mpl_vector_paddq_v2i64(int64x2_t, int64x2_t);
uint8x16_t __builtin_mpl_vector_paddq_v16u8(uint8x16_t, uint8x16_t);
uint16x8_t __builtin_mpl_vector_paddq_v8u16(uint16x8_t, uint16x8_t);
uint32x4_t __builtin_mpl_vector_paddq_v4u32(uint32x4_t, uint32x4_t);
uint64x2_t __builtin_mpl_vector_paddq_v2u64(uint64x2_t, uint64x2_t);
int64x1_t __builtin_mpl_vector_paddd_v2i64(int64x2_t);
uint64x1_t __builtin_mpl_vector_paddd_v2u64(uint64x2_t);

// vectTy vector_from_scalar(scalarTy val)
// Create a vector by replicating the scalar value to all elements of the vector.

int64x2_t __builtin_mpl_vector_from_scalar_v2i64(int64_t);
int32x4_t __builtin_mpl_vector_from_scalar_v4i32(int32_t);
int16x8_t __builtin_mpl_vector_from_scalar_v8i16(int16_t);
int8x16_t __builtin_mpl_vector_from_scalar_v16i8(int8_t);
uint64x2_t __builtin_mpl_vector_from_scalar_v2u64(uint64_t);
uint32x4_t __builtin_mpl_vector_from_scalar_v4u32(uint32_t);
uint16x8_t __builtin_mpl_vector_from_scalar_v8u16(uint16_t);
uint8x16_t __builtin_mpl_vector_from_scalar_v16u8(uint8_t);
float64x2_t __builtin_mpl_vector_from_scalar_v2f64(float64_t);
float32x4_t __builtin_mpl_vector_from_scalar_v4f32(float32_t);
int64x1_t __builtin_mpl_vector_from_scalar_v1i64(int64_t);
int32x2_t __builtin_mpl_vector_from_scalar_v2i32(int32_t);
int16x4_t __builtin_mpl_vector_from_scalar_v4i16(int16_t);
int8x8_t __builtin_mpl_vector_from_scalar_v8i8(int8_t);
uint64x1_t __builtin_mpl_vector_from_scalar_v1u64(uint64_t);
uint32x2_t __builtin_mpl_vector_from_scalar_v2u32(uint32_t);
uint16x4_t __builtin_mpl_vector_from_scalar_v4u16(uint16_t);
uint8x8_t __builtin_mpl_vector_from_scalar_v8u8(uint8_t);
float64x1_t __builtin_mpl_vector_from_scalar_v1f64(float64_t);
float32x2_t __builtin_mpl_vector_from_scalar_v2f32(float32_t);

// vecTy2 vector_madd(vecTy2 accum, vecTy1 src1, vecTy1 src2)
// Multiply the elements of src1 and src2, then accumulate into accum.
// Elements of vecTy2 are twice as long as elements of vecTy1.

int64x2_t __builtin_mpl_vector_madd_v2i32(int64x2_t, int32x2_t, int32x2_t);
int32x4_t __builtin_mpl_vector_madd_v4i16(int32x4_t, int16x4_t, int16x4_t);
int16x8_t __builtin_mpl_vector_madd_v8i8(int16x8_t, int8x8_t, int8x8_t);
uint64x2_t __builtin_mpl_vector_madd_v2u32(uint64x2_t, uint32x2_t, uint32x2_t);
uint32x4_t __builtin_mpl_vector_madd_v4u16(uint32x4_t, uint16x4_t, uint16x4_t);
uint16x8_t __builtin_mpl_vector_madd_v8u8(uint16x8_t, uint8x8_t, uint8x8_t);

// vecTy2 vector_mull_low(vecTy1 src1, vecTy1 src2)
// Multiply the elements of src1 and src2. Elements of vecTy2 are twice as long as elements of vecTy1.

int64x2_t __builtin_mpl_vector_mull_low_v2i32(int32x2_t, int32x2_t);
int32x4_t __builtin_mpl_vector_mull_low_v4i16(int16x4_t, int16x4_t);
int16x8_t __builtin_mpl_vector_mull_low_v8i8(int8x8_t, int8x8_t);
uint64x2_t __builtin_mpl_vector_mull_low_v2u32(uint32x2_t, uint32x2_t);
uint32x4_t __builtin_mpl_vector_mull_low_v4u16(uint16x4_t, uint16x4_t);
uint16x8_t __builtin_mpl_vector_mull_low_v8u8(uint8x8_t, uint8x8_t);

// vecTy2 vector_mull_high(vecTy1 src1, vecTy1 src2)
// Multiply the upper elements of src1 and src2. Elements of vecTy2 are twice as long as elements of vecTy1.

int64x2_t __builtin_mpl_vector_mull_high_v2i32(int32x4_t, int32x4_t);
int32x4_t __builtin_mpl_vector_mull_high_v4i16(int16x8_t, int16x8_t);
int16x8_t __builtin_mpl_vector_mull_high_v8i8(int8x16_t, int8x16_t);
uint64x2_t __builtin_mpl_vector_mull_high_v2u32(uint32x4_t, uint32x4_t);
uint32x4_t __builtin_mpl_vector_mull_high_v4u16(uint16x8_t, uint16x8_t);
uint16x8_t __builtin_mpl_vector_mull_high_v8u8(uint8x16_t, uint8x16_t);

// vecTy vector_merge(vecTy src1, vecTy src2, int n)
// Create a vector by concatenating the high elements of src1, starting with the nth element, followed by the low
//  elements of src2.

int64x2_t __builtin_mpl_vector_merge_v2i64(int64x2_t, int64x2_t, int32_t);
int32x4_t __builtin_mpl_vector_merge_v4i32(int32x4_t, int32x4_t, int32_t);
int16x8_t __builtin_mpl_vector_merge_v8i16(int16x8_t, int16x8_t, int32_t);
int8x16_t __builtin_mpl_vector_merge_v16i8(int8x16_t, int8x16_t, int32_t);
uint64x2_t __builtin_mpl_vector_merge_v2u64(uint64x2_t, uint64x2_t, int32_t);
uint32x4_t __builtin_mpl_vector_merge_v4u32(uint32x4_t, uint32x4_t, int32_t);
uint16x8_t __builtin_mpl_vector_merge_v8u16(uint16x8_t, uint16x8_t, int32_t);
uint8x16_t __builtin_mpl_vector_merge_v16u8(uint8x16_t, uint8x16_t, int32_t);
float64x2_t __builtin_mpl_vector_merge_v2f64(float64x2_t, float64x2_t, int32_t);
float32x4_t __builtin_mpl_vector_merge_v4f32(float32x4_t, float32x4_t, int32_t);
int64x1_t __builtin_mpl_vector_merge_v1i64(int64x1_t, int64x1_t, int32_t);
int32x2_t __builtin_mpl_vector_merge_v2i32(int32x2_t, int32x2_t, int32_t);
int16x4_t __builtin_mpl_vector_merge_v4i16(int16x4_t, int16x4_t, int32_t);
int8x8_t __builtin_mpl_vector_merge_v8i8(int8x8_t, int8x8_t, int32_t);
uint64x1_t __builtin_mpl_vector_merge_v1u64(uint64x1_t, uint64x1_t, int32_t);
uint32x2_t __builtin_mpl_vector_merge_v2u32(uint32x2_t, uint32x2_t, int32_t);
uint16x4_t __builtin_mpl_vector_merge_v4u16(uint16x4_t, uint16x4_t, int32_t);
uint8x8_t __builtin_mpl_vector_merge_v8u8(uint8x8_t, uint8x8_t, int32_t);
float64x1_t __builtin_mpl_vector_merge_v1f64(float64x1_t, float64x1_t, int32_t);
float32x2_t __builtin_mpl_vector_merge_v2f32(float32x2_t, float32x2_t, int32_t);

// vecTy2 vector_get_low(vecTy1 src)
// Create a vector from the low part of the source vector.

int64x1_t __builtin_mpl_vector_get_low_v2i64(int64x2_t);
int32x2_t __builtin_mpl_vector_get_low_v4i32(int32x4_t);
int16x4_t __builtin_mpl_vector_get_low_v8i16(int16x8_t);
int8x8_t __builtin_mpl_vector_get_low_v16i8(int8x16_t);
uint64x1_t __builtin_mpl_vector_get_low_v2u64(uint64x2_t);
uint32x2_t __builtin_mpl_vector_get_low_v4u32(uint32x4_t);
uint16x4_t __builtin_mpl_vector_get_low_v8u16(uint16x8_t);
uint8x8_t __builtin_mpl_vector_get_low_v16u8(uint8x16_t);
float32x2_t __builtin_mpl_vector_get_low_v2f32(float32x4_t);
float64x1_t __builtin_mpl_vector_get_low_v1f64(float64x2_t);
float64x1_t __builtin_mpl_vector_get_low_v2f64(float64x2_t);
float32x2_t __builtin_mpl_vector_get_low_v4f32(float32x4_t);

// vecTy2 vector_get_high(vecTy1 src)
// Create a vector from the high part of the source vector.

int64x1_t __builtin_mpl_vector_get_high_v2i64(int64x2_t);
int32x2_t __builtin_mpl_vector_get_high_v4i32(int32x4_t);
int16x4_t __builtin_mpl_vector_get_high_v8i16(int16x8_t);
int8x8_t __builtin_mpl_vector_get_high_v16i8(int8x16_t);
uint64x1_t __builtin_mpl_vector_get_high_v2u64(uint64x2_t);
uint32x2_t __builtin_mpl_vector_get_high_v4u32(uint32x4_t);
uint16x4_t __builtin_mpl_vector_get_high_v8u16(uint16x8_t);
uint8x8_t __builtin_mpl_vector_get_high_v16u8(uint8x16_t);
float64x1_t __builtin_mpl_vector_get_high_v2f64(float64x2_t);
float32x2_t __builtin_mpl_vector_get_high_v4f32(float32x4_t);
float32x2_t __builtin_mpl_vector_get_high_v2f32(float32x4_t);
float64x1_t __builtin_mpl_vector_get_high_v1f64(float64x1_t);

// scalarTy vector_get_element(vecTy src, int n)
// Get the nth element of the source vector.

float32_t __builtin_mpl_vector_get_element_v2f32(float32x2_t, const int);
float64_t __builtin_mpl_vector_get_element_v1f64(float64x1_t, const int);
float32_t __builtin_mpl_vector_get_element_v4f32(float32x4_t, const int);
float64_t __builtin_mpl_vector_get_element_v2f64(float64x2_t, const int);

// scalarTy vector_get_lane
uint8_t __builtin_mpl_vector_get_lane_v8u8(uint8x8_t, const int);
uint16_t __builtin_mpl_vector_get_lane_v4u16(uint16x4_t, const int);
uint32_t __builtin_mpl_vector_get_lane_v2u32(uint32x2_t, const int);
uint64_t __builtin_mpl_vector_get_lane_v1u64(uint64x1_t, const int);
int8_t __builtin_mpl_vector_get_lane_v8i8(int8x8_t, const int);
int16_t __builtin_mpl_vector_get_lane_v4i16(int16x4_t, const int);
int32_t __builtin_mpl_vector_get_lane_v2i32(int32x2_t, const int);
int64_t __builtin_mpl_vector_get_lane_v1i64(int64x1_t, const int);
uint8_t __builtin_mpl_vector_getq_lane_v16u8(uint8x16_t, const int);
uint16_t __builtin_mpl_vector_getq_lane_v8u16(uint16x8_t, const int);
uint32_t __builtin_mpl_vector_getq_lane_v4u32(uint32x4_t, const int);
uint64_t __builtin_mpl_vector_getq_lane_v2u64(uint64x2_t, const int);
int8_t __builtin_mpl_vector_getq_lane_v16i8(int8x16_t, const int);
int16_t __builtin_mpl_vector_getq_lane_v8i16(int16x8_t, const int);
int32_t __builtin_mpl_vector_getq_lane_v4i32(int32x4_t, const int);
int64_t __builtin_mpl_vector_getq_lane_v2i64(int64x2_t, const int);

// vecTy vector_set_element(ScalarTy value, VecTy vec, int n)
// Set the nth element of the source vector to value.

int64x2_t __builtin_mpl_vector_set_element_v2i64(int64_t, int64x2_t, const int);
int32x4_t __builtin_mpl_vector_set_element_v4i32(int32_t, int32x4_t, const int);
int16x8_t __builtin_mpl_vector_set_element_v8i16(int16_t, int16x8_t, const int);
int8x16_t __builtin_mpl_vector_set_element_v16i8(int8_t, int8x16_t, const int);
uint64x2_t __builtin_mpl_vector_set_element_v2u64(uint64_t, uint64x2_t, const int);
uint32x4_t __builtin_mpl_vector_set_element_v4u32(uint32_t, uint32x4_t, const int);
uint16x8_t __builtin_mpl_vector_set_element_v8u16(uint16_t, uint16x8_t, const int);
uint8x16_t __builtin_mpl_vector_set_element_v16u8(uint8_t, uint8x16_t, const int);
float64x2_t __builtin_mpl_vector_set_element_v2f64(float64_t, float64x2_t, const int);
float32x4_t __builtin_mpl_vector_set_element_v4f32(float32_t, float32x4_t, const int);
int64x1_t __builtin_mpl_vector_set_element_v1i64(int64_t, int64x1_t, const int);
int32x2_t __builtin_mpl_vector_set_element_v2i32(int32_t, int32x2_t, const int);
int16x4_t __builtin_mpl_vector_set_element_v4i16(int16_t, int16x4_t, const int);
int8x8_t __builtin_mpl_vector_set_element_v8i8(int8_t, int8x8_t, const int);
uint64x1_t __builtin_mpl_vector_set_element_v1u64(uint64_t, uint64x1_t, const int);
uint32x2_t __builtin_mpl_vector_set_element_v2u32(uint32_t, uint32x2_t, const int);
uint16x4_t __builtin_mpl_vector_set_element_v4u16(uint16_t, uint16x4_t, const int);
uint8x8_t __builtin_mpl_vector_set_element_v8u8(uint8_t, uint8x8_t, const int);
float64x1_t __builtin_mpl_vector_set_element_v1f64(float64_t, float64x1_t, const int);
float32x2_t __builtin_mpl_vector_set_element_v2f32(float32_t, float32x2_t, const int);

// vecTy2 vector_abdl(vectTy1 src2, vectTy2 src2)
// Create a widened vector by getting the abs value of subtracted arguments.

int16x8_t __builtin_mpl_vector_labssub_low_v8i8(int8x8_t, int8x8_t);
int32x4_t __builtin_mpl_vector_labssub_low_v4i16(int16x4_t, int16x4_t);
int64x2_t __builtin_mpl_vector_labssub_low_v2i32(int32x2_t, int32x2_t);
uint16x8_t __builtin_mpl_vector_labssub_low_v8u8(uint8x8_t, uint8x8_t);
uint32x4_t __builtin_mpl_vector_labssub_low_v4u16(uint16x4_t, uint16x4_t);
uint64x2_t __builtin_mpl_vector_labssub_low_v2u32(uint32x2_t, uint32x2_t);

// vecTy2 vector_abdl_high(vectTy1 src2, vectTy2 src2)
// Create a widened vector by getting the abs value of subtracted high arguments.

int16x8_t __builtin_mpl_vector_labssub_high_v8i8(int8x16_t, int8x16_t);
int32x4_t __builtin_mpl_vector_labssub_high_v4i16(int16x8_t, int16x8_t);
int64x2_t __builtin_mpl_vector_labssub_high_v2i32(int32x4_t, int32x4_t);
uint16x8_t __builtin_mpl_vector_labssub_high_v8u8(uint8x16_t, uint8x16_t);
uint32x4_t __builtin_mpl_vector_labssub_high_v4u16(uint16x8_t, uint16x8_t);
uint64x2_t __builtin_mpl_vector_labssub_high_v2u32(uint32x4_t, uint32x4_t);

// vecTy2 vector_narrow_low(vecTy1 src)
// Narrow each element of the source vector to half of the original width, writing the lower half into the destination
//  vector.

int32x2_t __builtin_mpl_vector_narrow_low_v2i64(int64x2_t);
int16x4_t __builtin_mpl_vector_narrow_low_v4i32(int32x4_t);
int8x8_t __builtin_mpl_vector_narrow_low_v8i16(int16x8_t);
uint32x2_t __builtin_mpl_vector_narrow_low_v2u64(uint64x2_t);
uint16x4_t __builtin_mpl_vector_narrow_low_v4u32(uint32x4_t);
uint8x8_t __builtin_mpl_vector_narrow_low_v8u16(uint16x8_t);

// vecTy2 vector_narrow_high(vecTy1 src1, vecTy2 src2)
// Narrow each element of the source vector to half of the original width, concatenate the upper half into the
//  destination vector.

int32x4_t __builtin_mpl_vector_narrow_high_v2i64(int32x2_t, int64x2_t);
int16x8_t __builtin_mpl_vector_narrow_high_v4i32(int16x4_t, int32x4_t);
int8x16_t __builtin_mpl_vector_narrow_high_v8i16(int8x8_t, int16x8_t);
uint32x4_t __builtin_mpl_vector_narrow_high_v2u64(uint32x2_t, uint64x2_t);
uint16x8_t __builtin_mpl_vector_narrow_high_v4u32(uint16x4_t, uint32x4_t);
uint8x16_t __builtin_mpl_vector_narrow_high_v8u16(uint8x8_t, uint16x8_t);

// vecTy1 vector_adapl(vecTy1 src1, vecTy2 src2)
// Vector pairwise addition and accumulate

int16x4_t __builtin_mpl_vector_pairwise_adalp_v8i8(int16x4_t, int8x8_t);
int32x2_t __builtin_mpl_vector_pairwise_adalp_v4i16(int32x2_t, int16x4_t);
int64x1_t __builtin_mpl_vector_pairwise_adalp_v2i32(int64x1_t, int32x2_t);
uint16x4_t __builtin_mpl_vector_pairwise_adalp_v8u8(uint16x4_t, uint8x8_t);
uint32x2_t __builtin_mpl_vector_pairwise_adalp_v4u16(uint32x2_t, uint16x4_t);
uint64x1_t __builtin_mpl_vector_pairwise_adalp_v2u32(uint64x1_t, uint32x2_t);
int16x8_t __builtin_mpl_vector_pairwise_adalp_v16i8(int16x8_t, int8x16_t);
int32x4_t __builtin_mpl_vector_pairwise_adalp_v8i16(int32x4_t, int16x8_t);
int64x2_t __builtin_mpl_vector_pairwise_adalp_v4i32(int64x2_t, int32x4_t);
uint16x8_t __builtin_mpl_vector_pairwise_adalp_v16u8(uint16x8_t, uint8x16_t);
uint32x4_t __builtin_mpl_vector_pairwise_adalp_v8u16(uint32x4_t, uint16x8_t);
uint64x2_t __builtin_mpl_vector_pairwise_adalp_v4u32(uint64x2_t, uint32x4_t);

// vecTy2 vector_pairwise_add(vecTy1 src)
// Add pairs of elements from the source vector and put the result into the destination vector, whose element size is
//  twice and the number of elements is half of the source vector type.

int64x2_t __builtin_mpl_vector_pairwise_add_v4i32(int32x4_t);
int32x4_t __builtin_mpl_vector_pairwise_add_v8i16(int16x8_t);
int16x8_t __builtin_mpl_vector_pairwise_add_v16i8(int8x16_t);
uint64x2_t __builtin_mpl_vector_pairwise_add_v4u32(uint32x4_t);
uint32x4_t __builtin_mpl_vector_pairwise_add_v8u16(uint16x8_t);
uint16x8_t __builtin_mpl_vector_pairwise_add_v16u8(uint8x16_t);
int64x1_t __builtin_mpl_vector_pairwise_add_v2i32(int32x2_t);
int32x2_t __builtin_mpl_vector_pairwise_add_v4i16(int16x4_t);
int16x4_t __builtin_mpl_vector_pairwise_add_v8i8(int8x8_t);
uint64x1_t __builtin_mpl_vector_pairwise_add_v2u32(uint32x2_t);
uint32x2_t __builtin_mpl_vector_pairwise_add_v4u16(uint16x4_t);
uint16x4_t __builtin_mpl_vector_pairwise_add_v8u8(uint8x8_t);

// vecTy vector_reverse(vecTy src)
// Create a vector by reversing the order of the elements in src.

int64x2_t __builtin_mpl_vector_reverse_v2i64(int64x2_t);
int32x4_t __builtin_mpl_vector_reverse_v4i32(int32x4_t);
int16x8_t __builtin_mpl_vector_reverse_v8i16(int16x8_t);
int8x16_t __builtin_mpl_vector_reverse_v16i8(int8x16_t);
uint64x2_t __builtin_mpl_vector_reverse_v2u64(uint64x2_t);
uint32x4_t __builtin_mpl_vector_reverse_v4u32(uint32x4_t);
uint16x8_t __builtin_mpl_vector_reverse_v8u16(uint16x8_t);
uint8x16_t __builtin_mpl_vector_reverse_v16u8(uint8x16_t);
float64x2_t __builtin_mpl_vector_reverse_v2f64(float64x2_t);
float32x4_t __builtin_mpl_vector_reverse_v4f32(float32x4_t);
int64x1_t __builtin_mpl_vector_reverse_v1i64(int64x1_t);
int32x2_t __builtin_mpl_vector_reverse_v2i32(int32x2_t);
int16x4_t __builtin_mpl_vector_reverse_v4i16(int16x4_t);
int8x8_t __builtin_mpl_vector_reverse_v8i8(int8x8_t);
uint64x1_t __builtin_mpl_vector_reverse_v1u64(uint64x1_t);
uint32x2_t __builtin_mpl_vector_reverse_v2u32(uint32x2_t);
uint16x4_t __builtin_mpl_vector_reverse_v4u16(uint16x4_t);
uint8x8_t __builtin_mpl_vector_reverse_v8u8(uint8x8_t);
float64x1_t __builtin_mpl_vector_reverse_v1f64(float64x1_t);
float32x2_t __builtin_mpl_vector_reverse_v2f32(float32x2_t);

// vecTy vector_shli(vecTy src, const int n)
// Shift each element in the vector left by n.

int64x2_t __builtin_mpl_vector_shli_v2i64(int64x2_t, const int);
int32x4_t __builtin_mpl_vector_shli_v4i32(int32x4_t, const int);
int16x8_t __builtin_mpl_vector_shli_v8i16(int16x8_t, const int);
int8x16_t __builtin_mpl_vector_shli_v16i8(int8x16_t, const int);
uint64x2_t __builtin_mpl_vector_shli_v2u64(uint64x2_t, const int);
uint32x4_t __builtin_mpl_vector_shli_v4u32(uint32x4_t, const int);
uint16x8_t __builtin_mpl_vector_shli_v8u16(uint16x8_t, const int);
uint8x16_t __builtin_mpl_vector_shli_v16u8(uint8x16_t, const int);
int64x1_t __builtin_mpl_vector_shli_v1i64(int64x1_t, const int);
int32x2_t __builtin_mpl_vector_shli_v2i32(int32x2_t, const int);
int16x4_t __builtin_mpl_vector_shli_v4i16(int16x4_t, const int);
int8x8_t __builtin_mpl_vector_shli_v8i8(int8x8_t, const int);
uint64x1_t __builtin_mpl_vector_shli_v1u64(uint64x1_t, const int);
uint32x2_t __builtin_mpl_vector_shli_v2u32(uint32x2_t, const int);
uint16x4_t __builtin_mpl_vector_shli_v4u16(uint16x4_t, const int);
uint8x8_t __builtin_mpl_vector_shli_v8u8(uint8x8_t, const int);

// vecTy vector_shln
int8x8_t __builtin_mpl_vector_shl_n_v8i8(int8x8_t, const int);
int8x16_t __builtin_mpl_vector_shlq_n_v16i8(int8x16_t, const int);
int16x4_t __builtin_mpl_vector_shl_n_v4i16(int16x4_t, const int);
int16x8_t __builtin_mpl_vector_shlq_n_v8i16(int16x8_t, const int);
int32x2_t __builtin_mpl_vector_shl_n_v2i32(int32x2_t, const int);
int32x4_t __builtin_mpl_vector_shlq_n_v4i32(int32x4_t, const int);
int64x1_t __builtin_mpl_vector_shl_n_v1i64(int64x1_t, const int);
int64x2_t __builtin_mpl_vector_shlq_n_v2i64(int64x2_t, const int);
uint8x8_t __builtin_mpl_vector_shl_n_v8u8(uint8x8_t, const int);
uint8x16_t __builtin_mpl_vector_shlq_n_v16u8(uint8x16_t, const int);
uint16x4_t __builtin_mpl_vector_shl_n_v4u16(uint16x4_t, const int);
uint16x8_t __builtin_mpl_vector_shlq_n_v8u16(uint16x8_t, const int);
uint32x2_t __builtin_mpl_vector_shl_n_v2u32(uint32x2_t, const int);
uint32x4_t__builtin_mpl_vector_shlq_n_v4u32(uint32x4_t, const int);
uint64x1_t __builtin_mpl_vector_shl_n_v1u64(uint64x1_t, const int);
uint64x2_t __builtin_mpl_vector_shlq_n_v2u64(uint64x2_t, const int);

// vecTy vector_shri(vecTy src, const int n)
// Shift each element in the vector right by n.

int64x2_t __builtin_mpl_vector_shri_v2i64(int64x2_t, const int);
int32x4_t __builtin_mpl_vector_shri_v4i32(int32x4_t, const int);
int16x8_t __builtin_mpl_vector_shri_v8i16(int16x8_t, const int);
int8x16_t __builtin_mpl_vector_shri_v16i8(int8x16_t, const int);
uint64x2_t __builtin_mpl_vector_shru_v2u64(uint64x2_t, const int);
uint32x4_t __builtin_mpl_vector_shru_v4u32(uint32x4_t, const int);
uint16x8_t __builtin_mpl_vector_shru_v8u16(uint16x8_t, const int);
uint8x16_t __builtin_mpl_vector_shru_v16u8(uint8x16_t, const int);
int64x1_t __builtin_mpl_vector_shri_v1i64(int64x1_t, const int);
int32x2_t __builtin_mpl_vector_shri_v2i32(int32x2_t, const int);
int16x4_t __builtin_mpl_vector_shri_v4i16(int16x4_t, const int);
int8x8_t __builtin_mpl_vector_shri_v8i8(int8x8_t, const int);
uint64x1_t __builtin_mpl_vector_shru_v1u64(uint64x1_t, const int);
uint32x2_t __builtin_mpl_vector_shru_v2u32(uint32x2_t, const int);
uint16x4_t __builtin_mpl_vector_shru_v4u16(uint16x4_t, const int);
uint8x8_t __builtin_mpl_vector_shru_v8u8(uint8x8_t, const int);

// vecTy vector_shrn
int8x8_t __builtin_mpl_vector_shr_n_v8i8(int8x8_t, const int);
int8x16_t __builtin_mpl_vector_shrq_n_v16i8(int8x16_t, const int);
int16x4_t __builtin_mpl_vector_shr_n_v4i16(int16x4_t, const int);
int16x8_t __builtin_mpl_vector_shrq_n_v8i16(int16x8_t, const int);
int32x2_t __builtin_mpl_vector_shr_n_v2i32(int32x2_t, const int);
int32x4_t __builtin_mpl_vector_shrq_n_v4i32(int32x4_t, const int);
int64x1_t __builtin_mpl_vector_shr_n_v1i64(int64x1_t, const int);
int64x2_t __builtin_mpl_vector_shrq_n_v2i64(int64x2_t, const int);
uint8x8_t __builtin_mpl_vector_shr_n_v8u8(uint8x8_t, const int);
uint8x16_t __builtin_mpl_vector_shrq_n_v16u8(uint8x16_t, const int);
uint16x4_t __builtin_mpl_vector_shr_n_v4u16(uint16x4_t, const int);
uint16x8_t __builtin_mpl_vector_shrq_n_v8u16(uint16x8_t, const int);
uint32x2_t __builtin_mpl_vector_shr_n_v2u32(uint32x2_t, const int);
uint32x4_t __builtin_mpl_vector_shrq_n_v4u32(uint32x4_t, const int);
uint64x1_t __builtin_mpl_vector_shr_n_v1u64(uint64x1_t, const int);
uint64x2_t __builtin_mpl_vector_shrq_n_v2u64(uint64x2_t, const int);

// vecTy2 vector_max
int8x8_t __builtin_mpl_vector_max_v8i8(int8x8_t, int8x8_t);
int8x16_t __builtin_mpl_vector_maxq_v16i8(int8x16_t, int8x16_t);
int16x4_t __builtin_mpl_vector_max_v4i16(int16x4_t, int16x4_t);
int16x8_t __builtin_mpl_vector_maxq_v8i16(int16x8_t, int16x8_t);
int32x2_t __builtin_mpl_vector_max_v2i32(int32x2_t, int32x2_t);
int32x4_t __builtin_mpl_vector_maxq_v4i32(int32x4_t, int32x4_t);
uint8x8_t __builtin_mpl_vector_max_v8u8(uint8x8_t, uint8x8_t);
uint8x16_t __builtin_mpl_vector_maxq_v16u8(uint8x16_t, uint8x16_t);
uint16x4_t __builtin_mpl_vector_max_v4u16(uint16x4_t, uint16x4_t);
uint16x8_t __builtin_mpl_vector_maxq_v8u16(uint16x8_t, uint16x8_t);
uint32x2_t __builtin_mpl_vector_max_v2u32(uint32x2_t, uint32x2_t);
uint32x4_t __builtin_mpl_vector_maxq_v4u32(uint32x4_t, uint32x4_t);

// vecTy2 vector_min
int8x8_t __builtin_mpl_vector_min_v8i8(int8x8_t, int8x8_t);
int8x16_t __builtin_mpl_vector_minq_v16i8(int8x16_t, int8x16_t);
int16x4_t __builtin_mpl_vector_min_v4i16(int16x4_t, int16x4_t);
int16x8_t __builtin_mpl_vector_minq_v8i16(int16x8_t, int16x8_t);
int32x2_t __builtin_mpl_vector_min_v2i32(int32x2_t, int32x2_t);
int32x4_t __builtin_mpl_vector_minq_v4i32(int32x4_t, int32x4_t);
uint8x8_t __builtin_mpl_vector_min_v8u8(uint8x8_t, uint8x8_t);
uint8x16_t __builtin_mpl_vector_minq_v16u8(uint8x16_t, uint8x16_t);
uint16x4_t __builtin_mpl_vector_min_v4u16(uint16x4_t, uint16x4_t);
uint16x8_t __builtin_mpl_vector_minq_v8u16(uint16x8_t, uint16x8_t);
uint32x2_t __builtin_mpl_vector_min_v2u32(uint32x2_t, uint32x2_t);
uint32x4_t __builtin_mpl_vector_minq_v4u32(uint32x4_t, uint32x4_t);

// vecTy2 vector_pmax
int8x8_t __builtin_mpl_vector_pmax_v8i8(int8x8_t, int8x8_t);
int16x4_t __builtin_mpl_vector_pmax_v4i16(int16x4_t, int16x4_t);
int32x2_t __builtin_mpl_vector_pmax_v2i32(int32x2_t, int32x2_t);
uint8x8_t __builtin_mpl_vector_pmax_v8u8(uint8x8_t, uint8x8_t);
uint16x4_t __builtin_mpl_vector_pmax_v4u16(uint16x4_t, uint16x4_t);
uint32x2_t __builtin_mpl_vector_pmax_v2u32(uint32x2_t, uint32x2_t);
int8x16_t __builtin_mpl_vector_pmaxq_v16i8(int8x16_t, int8x16_t);
int16x8_t __builtin_mpl_vector_pmaxq_v8i16(int16x8_t, int16x8_t);
int32x4_t __builtin_mpl_vector_pmaxq_v4i32(int32x4_t, int32x4_t);
uint8x16_t __builtin_mpl_vector_pmaxq_v16u8(uint8x16_t, uint8x16_t);
uint16x8_t __builtin_mpl_vector_pmaxq_v8u16(uint16x8_t, uint16x8_t);
uint32x4_t __builtin_mpl_vector_pmaxq_v4u32(uint32x4_t, uint32x4_t);

// vecTy2 vector_pmin
int8x8_t __builtin_mpl_vector_pmin_v8i8(int8x8_t, int8x8_t);
int16x4_t __builtin_mpl_vector_pmin_v4i16(int16x4_t, int16x4_t);
uint8x8_t __builtin_mpl_vector_pmin_v8u8(uint8x8_t, uint8x8_t);
uint32x2_t __builtin_mpl_vector_pmin_v2u32(uint32x2_t, uint32x2_t);
int8x16_t __builtin_mpl_vector_pminq_v16i8(int8x16_t, int8x16_t);
int16x8_t __builtin_mpl_vector_pminq_v8i16(int16x8_t, int16x8_t);
int32x4_t __builtin_mpl_vector_pminq_v4i32(int32x4_t, int32x4_t);
uint8x16_t __builtin_mpl_vector_pminq_v16u8(uint8x16_t, uint8x16_t);
uint16x8_t __builtin_mpl_vector_pminq_v8u16(uint16x8_t, uint16x8_t);
uint32x4_t __builtin_mpl_vector_pminq_v4u32(uint32x4_t, uint32x4_t);

// vecTy2 vector_maxv
int8x8_t __builtin_mpl_vector_maxv_v8i8(int8x8_t);
int8x16_t __builtin_mpl_vector_maxvq_v16i8(int8x16_t);
int16x4_t __builtin_mpl_vector_maxv_v4i16(int16x4_t);
int16x8_t __builtin_mpl_vector_maxvq_v8i16(int16x8_t);
int32x2_t __builtin_mpl_vector_maxv_v2i32(int32x2_t);
int32x4_t __builtin_mpl_vector_maxvq_v4i32(int32x4_t);
uint8x8_t __builtin_mpl_vector_maxv_v8u8(uint8x8_t);
uint8x16_t __builtin_mpl_vector_maxvq_v16u8(uint8x16_t);
uint16x4_t __builtin_mpl_vector_maxv_v4u16(uint16x4_t);
uint16x8_t __builtin_mpl_vector_maxvq_v8u16(uint16x8_t);
uint32x2_t __builtin_mpl_vector_maxv_v2u32(uint32x2_t);
uint32x4_t __builtin_mpl_vector_maxvq_v4u32(uint32x4_t);

// vecTy2 vector_minv
int8x8_t __builtin_mpl_vector_minv_v8i8(int8x8_t);
int8x16_t __builtin_mpl_vector_minvq_v16i8(int8x16_t);
int16x4_t __builtin_mpl_vector_minv_v4i16(int16x4_t);
int16x8_t __builtin_mpl_vector_minvq_v8i16(int16x8_t);
int32x2_t __builtin_mpl_vector_minv_v2i32(int32x2_t);
int32x4_t __builtin_mpl_vector_minvq_v4i32(int32x4_t);
uint8x8_t __builtin_mpl_vector_minv_v8u8(uint8x8_t);
uint8x16_t __builtin_mpl_vector_minvq_v16u8(uint8x16_t);
uint16x4_t __builtin_mpl_vector_minv_v4u16(uint16x4_t);
uint16x8_t __builtin_mpl_vector_minvq_v8u16(uint16x8_t);
uint32x2_t __builtin_mpl_vector_minv_v2u32(uint32x2_t);
uint32x4_t __builtin_mpl_vector_minvq_v4u32(uint32x4_t);

// vecTy2 vector_tst
uint8x8_t __builtin_mpl_vector_tst_v8i8(int8x8_t, int8x8_t);
uint8x16_t __builtin_mpl_vector_tstq_v16i8(int8x16_t, int8x16_t);
uint16x4_t __builtin_mpl_vector_tst_v4i16(int16x4_t, int16x4_t);
uint16x8_t __builtin_mpl_vector_tstq_v8i16(int16x8_t, int16x8_t);
uint32x2_t __builtin_mpl_vector_tst_v2i32(int32x2_t, int32x2_t);
uint32x4_t __builtin_mpl_vector_tstq_v4i32(int32x4_t, int32x4_t);
uint8x8_t __builtin_mpl_vector_tst_v8u8(uint8x8_t, uint8x8_t);
uint8x16_t __builtin_mpl_vector_tstq_v16u8(uint8x16_t, uint8x16_t);
uint16x4_t __builtin_mpl_vector_tst_v4u16(uint16x4_t, uint16x4_t);
uint16x8_t __builtin_mpl_vector_tstq_v8u16(uint16x8_t, uint16x8_t);
uint32x2_t __builtin_mpl_vector_tst_v2u32(uint32x2_t, uint32x2_t);
uint32x4_t __builtin_mpl_vector_tstq_v4u32(uint32x4_t, uint32x4_t);
uint64x1_t __builtin_mpl_vector_tst_v1i64(int64x1_t, int64x1_t);
uint64x2_t __builtin_mpl_vector_tstq_v2i64(int64x2_t, int64x2_t);
uint64x1_t __builtin_mpl_vector_tst_v1u64(uint64x1_t, uint64x1_t);
uint64x2_t __builtin_mpl_vector_tstq_v2u64(uint64x2_t, uint64x2_t);

// vecTy2 vector_qmovn
int8x8_t __builtin_mpl_vector_qmovnh_i16(int16x4_t);
int16x4_t __builtin_mpl_vector_qmovns_i32(int32x2_t);
int32x2_t __builtin_mpl_vector_qmovnd_i64(int64x1_t);
uint8x8_t __builtin_mpl_vector_qmovnh_u16(uint16x4_t);
uint16x4_t __builtin_mpl_vector_qmovns_u32(uint32x2_t);
uint32x2_t __builtin_mpl_vector_qmovnd_u64(uint64x1_t);

// vecTy2 vector_qmovun
uint8x8_t __builtin_mpl_vector_qmovun_v8u8(int16x8_t);
uint16x4_t __builtin_mpl_vector_qmovun_v4u16(int32x4_t);
uint32x2_t __builtin_mpl_vector_qmovun_v2u32(int64x2_t);
uint8x8_t __builtin_mpl_vector_qmovunh_i16(int16x4_t);
uint16x4_t __builtin_mpl_vector_qmovuns_i32(int32x2_t);
uint32x2_t __builtin_mpl_vector_qmovund_i64(int64x1_t);

// vecTy2 vector_qmovn_high
int8x16_t __builtin_mpl_vector_qmovn_high_v16i8(int8x8_t, int16x8_t);
int16x8_t __builtin_mpl_vector_qmovn_high_v8i16(int16x4_t, int32x4_t);
int32x4_t __builtin_mpl_vector_qmovn_high_v4i32(int32x2_t, int64x2_t);
uint8x16_t __builtin_mpl_vector_qmovn_high_v16u8(uint8x8_t, uint16x8_t);
uint16x8_t __builtin_mpl_vector_qmovn_high_v8u16(uint16x4_t, uint32x4_t);
uint32x4_t __builtin_mpl_vector_qmovn_high_v4u32(uint32x2_t, uint64x2_t);

// vecTy2 vector_qmovun_high
uint8x16_t __builtin_mpl_vector_qmovun_high_v16u8(uint8x8_t, int16x8_t);
uint16x8_t __builtin_mpl_vector_qmovun_high_v8u16(uint16x4_t, int32x4_t);
uint32x4_t __builtin_mpl_vector_qmovun_high_v4u32(uint32x2_t, int64x2_t);

// vecTy2 vector_mul_lane
int16x4_t __builtin_mpl_vector_mul_lane_v4i16(int16x4_t, int16x4_t, const int);
int16x8_t __builtin_mpl_vector_mulq_lane_v8i16(int16x8_t, int16x4_t, const int);
int32x2_t __builtin_mpl_vector_mul_lane_v2i32(int32x2_t, int32x2_t, const int);
int32x4_t __builtin_mpl_vector_mulq_lane_v4i32(int32x4_t, int32x2_t, const int);
uint16x4_t __builtin_mpl_vector_mul_lane_v4u16(uint16x4_t, uint16x4_t, const int);
uint16x8_t __builtin_mpl_vector_mulq_lane_v8u16(uint16x8_t, uint16x4_t, const int);
uint32x2_t __builtin_mpl_vector_mul_lane_v2u32(uint32x2_t, uint32x2_t, const int);
uint32x4_t __builtin_mpl_vector_mulq_lane_v4u32(uint32x4_t, uint32x2_t, const int);
int16x4_t __builtin_mpl_vector_mul_laneq_v4i16(int16x4_t, int16x8_t, const int);
int16x8_t __builtin_mpl_vector_mulq_laneq_v8i16(int16x8_t, int16x8_t, const int);
int32x2_t __builtin_mpl_vector_mul_laneq_v2i32(int32x2_t, int32x4_t, const int);
int32x4_t __builtin_mpl_vector_mulq_laneq_v4i32(int32x4_t, int32x4_t, const int);
uint16x4_t __builtin_mpl_vector_mul_laneq_v4u16(uint16x4_t, uint16x8_t, const int);
uint16x8_t __builtin_mpl_vector_mulq_laneq_v8u16(uint16x8_t, uint16x8_t, const int);
uint32x2_t __builtin_mpl_vector_mul_laneq_v2u32(uint32x2_t, uint32x4_t, const int);
uint32x4_t __builtin_mpl_vector_mulq_laneq_v4u32(uint32x4_t, uint32x4_t, const int);

// vecTy2 vector_mull_lane
int32x4_t __builtin_mpl_vector_mull_lane_v4i32(int16x4_t, int16x4_t, const int);
int64x2_t __builtin_mpl_vector_mull_lane_v2i64(int32x2_t, int32x2_t, const int);
uint32x4_t __builtin_mpl_vector_mull_lane_v4u32(uint16x4_t, uint16x4_t, const int);
uint64x2_t __builtin_mpl_vector_mull_lane_v2u64(uint32x2_t, uint32x2_t, const int);
int32x4_t __builtin_mpl_vector_mull_laneq_v4i32(int16x4_t, int16x8_t, const int);
int64x2_t __builtin_mpl_vector_mull_laneq_v2i64(int32x2_t, int32x4_t, const int);
uint32x4_t __builtin_mpl_vector_mull_laneq_v4u32(uint16x4_t, uint16x8_t, const int);
uint64x2_t __builtin_mpl_vector_mull_laneq_v2u64(uint32x2_t, uint32x4_t, const int);

// vecTy2 vector_mull_high_lane
int32x4_t __builtin_mpl_vector_mull_high_lane_v4i32(int16x8_t, int16x4_t, const int);
int64x2_t __builtin_mpl_vector_mull_high_lane_v2i64(int32x4_t, int32x2_t, const int);
uint32x4_t __builtin_mpl_vector_mull_high_lane_v4u32(uint16x8_t, uint16x4_t, const int);
uint64x2_t __builtin_mpl_vector_mull_high_lane_v2u64(uint32x4_t, uint32x2_t, const int);
int32x4_t __builtin_mpl_vector_mull_high_laneq_v4i32(int16x8_t, int16x8_t, const int);
int64x2_t __builtin_mpl_vector_mull_high_laneq_v2i64(int32x4_t, int32x4_t, const int);
uint32x4_t __builtin_mpl_vector_mull_high_laneq_v4u32(uint16x8_t, uint16x8_t, const int);
uint64x2_t __builtin_mpl_vector_mull_high_laneq_v2u64(uint32x4_t, uint32x4_t, const int);

// vecTy2 vector_neg
int8x8_t __builtin_mpl_vector_neg_v8i8(int8x8_t);
int8x16_t __builtin_mpl_vector_negq_v16i8(int8x16_t);
int16x4_t __builtin_mpl_vector_neg_v4i16(int16x4_t);
int16x8_t __builtin_mpl_vector_negq_v8i16(int16x8_t);
int32x2_t __builtin_mpl_vector_neg_v2i32(int32x2_t);
int32x4_t __builtin_mpl_vector_negq_v4i32(int32x4_t);
int64x1_t __builtin_mpl_vector_neg_v1i64(int64x1_t);
int64x2_t __builtin_mpl_vector_negq_v2i64(int64x2_t);

// vecTy2 vector_shift_narrow_low(vecTy1 src, const int n)
// Shift each element in the vector right by n, narrow each element to half of the original width (truncating), then
//  write the result to the lower half of the destination vector.

int32x2_t __builtin_mpl_vector_shr_narrow_low_v2i64(int64x2_t, const int);
int16x4_t __builtin_mpl_vector_shr_narrow_low_v4i32(int32x4_t, const int);
int8x8_t __builtin_mpl_vector_shr_narrow_low_v8i16(int16x8_t, const int);
uint32x2_t __builtin_mpl_vector_shr_narrow_low_v2u64(uint64x2_t, const int);
uint16x4_t __builtin_mpl_vector_shr_narrow_low_v4u32(uint32x4_t, const int);
uint8x8_t __builtin_mpl_vector_shr_narrow_low_v8u16(uint16x8_t, const int);

// scalarTy vector_sum(vecTy src)
// Sum all of the elements in the vector into a scalar.

int64_t __builtin_mpl_vector_sum_v2i64(int64x2_t);
int32_t __builtin_mpl_vector_sum_v4i32(int32x4_t);
int16_t __builtin_mpl_vector_sum_v8i16(int16x8_t);
int8_t __builtin_mpl_vector_sum_v16i8(int8x16_t);
uint64_t __builtin_mpl_vector_sum_v2u64(uint64x2_t);
uint32_t __builtin_mpl_vector_sum_v4u32(uint32x4_t);
uint16_t __builtin_mpl_vector_sum_v8u16(uint16x8_t);
uint8_t __builtin_mpl_vector_sum_v16u8(uint8x16_t);
float64_t __builtin_mpl_vector_sum_v2f64(float64x2_t);
float32_t __builtin_mpl_vector_sum_v4f32(float32x4_t);
int32_t __builtin_mpl_vector_sum_v2i32(int32x2_t);
int16_t __builtin_mpl_vector_sum_v4i16(int16x4_t);
int8_t __builtin_mpl_vector_sum_v8i8(int8x8_t);
uint32_t __builtin_mpl_vector_sum_v2u32(uint32x2_t);
uint16_t __builtin_mpl_vector_sum_v4u16(uint16x4_t);
uint8_t __builtin_mpl_vector_sum_v8u8(uint8x8_t);
float32_t __builtin_mpl_vector_sum_v2f32(float32x2_t);

// vecTy table_lookup(vecTy tbl, vecTy idx)
// Performs a table vector lookup.

float64x2_t __builtin_mpl_vector_table_lookup_v2f64(float64x2_t, float64x2_t);
float32x4_t __builtin_mpl_vector_table_lookup_v4f32(float32x4_t, float32x4_t);
float64x1_t __builtin_mpl_vector_table_lookup_v1f64(float64x1_t, float64x1_t);
float32x2_t __builtin_mpl_vector_table_lookup_v2f32(float32x2_t, float32x2_t);

// vecTy2 vector_widen_low(vecTy1 src)
// Widen each element of the source vector to half of the original width, writing the lower half into the destination
//  vector.

int64x2_t __builtin_mpl_vector_widen_low_v2i32(int32x2_t);
int32x4_t __builtin_mpl_vector_widen_low_v4i16(int16x4_t);
int16x8_t __builtin_mpl_vector_widen_low_v8i8(int8x8_t);
uint64x2_t __builtin_mpl_vector_widen_low_v2u32(uint32x2_t);
uint32x4_t __builtin_mpl_vector_widen_low_v4u16(uint16x4_t);
uint16x8_t __builtin_mpl_vector_widen_low_v8u8(uint8x8_t);

// vecTy2 vector_widen_high(vecTy1 src)
// Widen each element of the source vector to half of the original width, writing the higher half into the destination
//  vector.

int64x2_t __builtin_mpl_vector_widen_high_v2i32(int32x4_t);
int32x4_t __builtin_mpl_vector_widen_high_v4i16(int16x8_t);
int16x8_t __builtin_mpl_vector_widen_high_v8i8(int8x16_t);
uint64x2_t __builtin_mpl_vector_widen_high_v2u32(uint32x4_t);
uint32x4_t __builtin_mpl_vector_widen_high_v4u16(uint16x8_t);
uint16x8_t __builtin_mpl_vector_widen_high_v8u8(uint8x16_t);

// vecTy vector_load(scalarTy *ptr)
// Load the elements pointed to by ptr into a vector.

float64x2_t __builtin_mpl_vector_load_v2f64(float64_t const *);
float32x4_t __builtin_mpl_vector_load_v4f32(float32_t const *);
float64x1_t __builtin_mpl_vector_load_v1f64(float64_t const *);
float32x2_t __builtin_mpl_vector_load_v2f32(float32_t const *);

// void vector_store(scalarTy *ptr, vecTy src)
// Store the elements from src into the memory pointed to by ptr.

void __builtin_mpl_vector_store_v2i64(int64_t *, int64x2_t);
void __builtin_mpl_vector_store_v4i32(int32_t *, int32x4_t);
void __builtin_mpl_vector_store_v8i16(int16_t *, int16x8_t);
void __builtin_mpl_vector_store_v16i8(int8_t *, int8x16_t);
void __builtin_mpl_vector_store_v2u64(uint64_t *, uint64x2_t);
void __builtin_mpl_vector_store_v4u32(uint32_t *, uint32x4_t);
void __builtin_mpl_vector_store_v8u16(uint16_t *, uint16x8_t);
void __builtin_mpl_vector_store_v16u8(uint8_t *, uint8x16_t);
void __builtin_mpl_vector_store_v2f64(float64_t *, float64x2_t);
void __builtin_mpl_vector_store_v4f32(float32_t *, float32x4_t);
void __builtin_mpl_vector_store_v1i64(int64_t *, int64x1_t);
void __builtin_mpl_vector_store_v2i32(int32_t *, int32x2_t);
void __builtin_mpl_vector_store_v4i16(int16_t *, int16x4_t);
void __builtin_mpl_vector_store_v8i8(int8_t *, int8x8_t);
void __builtin_mpl_vector_store_v1u64(uint64_t *, uint64x1_t);
void __builtin_mpl_vector_store_v2u32(uint32_t *, uint32x2_t);
void __builtin_mpl_vector_store_v4u16(uint16_t *, uint16x4_t);
void __builtin_mpl_vector_store_v8u8(uint8_t *, uint8x8_t);
void __builtin_mpl_vector_store_v1f64(float64_t *, float64x1_t);
void __builtin_mpl_vector_store_v2f32(float32_t *, float32x2_t);

// vecTy vector_subl_low(vecTy src1, vecTy src2)
// Subtract each element of the source vector to second source widen the result into the destination vector.

int16x8_t __builtin_mpl_vector_subl_low_v8i8(int8x8_t, int8x8_t);
int32x4_t __builtin_mpl_vector_subl_low_v4i16(int16x4_t, int16x4_t);
int64x2_t __builtin_mpl_vector_subl_low_v2i32(int32x2_t, int32x2_t);
uint16x8_t __builtin_mpl_vector_subl_low_v8u8(uint8x8_t, uint8x8_t);
uint32x4_t __builtin_mpl_vector_subl_low_v4u16(uint16x4_t, uint16x4_t);
uint64x2_t __builtin_mpl_vector_subl_low_v2u32(uint32x2_t, uint32x2_t);

// vecTy vector_subl_high(vecTy src1, vecTy src2)
// Subtract each element of the source vector to upper half of second source widen the result into the destination
//  vector.

int16x8_t __builtin_mpl_vector_subl_high_v8i8(int8x16_t, int8x16_t);
int32x4_t __builtin_mpl_vector_subl_high_v4i16(int16x8_t, int16x8_t);
int64x2_t __builtin_mpl_vector_subl_high_v2i32(int32x4_t, int32x4_t);
uint16x8_t __builtin_mpl_vector_subl_high_v8u8(uint8x16_t, uint8x16_t);
uint32x4_t __builtin_mpl_vector_subl_high_v4u16(uint16x8_t, uint16x8_t);
uint64x2_t __builtin_mpl_vector_subl_high_v2u32(uint32x4_t, uint32x4_t);

// vecTy vector_subw_low(vecTy src1, vecTy src2)
// Subtract each element of the source vector to second source widen the result into the destination vector.

int16x8_t __builtin_mpl_vector_subw_low_v8i8(int16x8_t, int8x8_t);
int32x4_t __builtin_mpl_vector_subw_low_v4i16(int32x4_t, int16x4_t);
int64x2_t __builtin_mpl_vector_subw_low_v2i32(int64x2_t, int32x2_t);
uint16x8_t __builtin_mpl_vector_subw_low_v8u8(uint16x8_t, uint8x8_t);
uint32x4_t __builtin_mpl_vector_subw_low_v4u16(uint32x4_t, uint16x4_t);
uint64x2_t __builtin_mpl_vector_subw_low_v2u32(uint64x2_t, uint32x2_t);

// vecTy vector_subw_high(vecTy src1, vecTy src2)
// Subtract each element of the source vector to upper half of second source widen the result into the destination
//  vector.

int16x8_t __builtin_mpl_vector_subw_high_v8i8(int16x8_t, int8x16_t);
int32x4_t __builtin_mpl_vector_subw_high_v4i16(int32x4_t, int16x8_t);
int64x2_t __builtin_mpl_vector_subw_high_v2i32(int64x2_t, int32x4_t);
uint16x8_t __builtin_mpl_vector_subw_high_v8u8(uint16x8_t, uint8x16_t);
uint32x4_t __builtin_mpl_vector_subw_high_v4u16(uint32x4_t, uint16x8_t);
uint64x2_t __builtin_mpl_vector_subw_high_v2u32(uint64x2_t, uint32x4_t);

// Supported Neon Intrinsics
extern inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_lane_u8(uint8x8_t v, const int lane) {
  return __builtin_mpl_vector_get_lane_v8u8(v, lane);
}

extern inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_lane_u16(uint16x4_t v, const int lane) {
  return __builtin_mpl_vector_get_lane_v4u16(v, lane);
}

extern inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_lane_u32(uint32x2_t v, const int lane) {
  return __builtin_mpl_vector_get_lane_v2u32(v, lane);
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_lane_u64(uint64x1_t v, const int lane) {
  return __builtin_mpl_vector_get_lane_v1u64(v, lane);
}

extern inline int8_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_lane_s8(int8x8_t v, const int lane) {
  return __builtin_mpl_vector_get_lane_v8i8(v, lane);
}

extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_lane_s16(int16x4_t v, const int lane) {
  return __builtin_mpl_vector_get_lane_v4i16(v, lane);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_lane_s32(int32x2_t v, const int lane) {
  return __builtin_mpl_vector_get_lane_v2i32(v, lane);
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_lane_s64(int64x1_t v, const int lane) {
  return __builtin_mpl_vector_get_lane_v1i64(v, lane);
}

extern inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__)) vgetq_lane_u8(uint8x16_t v, const int lane) {
  return __builtin_mpl_vector_getq_lane_v16u8(v, lane);
}

extern inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__)) vgetq_lane_u16(uint16x8_t v,
                                                                                          const int lane) {
  return __builtin_mpl_vector_getq_lane_v8u16(v, lane);
}

extern inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__)) vgetq_lane_u32(uint32x4_t v,
                                                                                          const int lane) {
  return __builtin_mpl_vector_getq_lane_v4u32(v, lane);
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vgetq_lane_u64(uint64x2_t v,
                                                                                          const int lane) {
  return __builtin_mpl_vector_getq_lane_v2u64(v, lane);
}

extern inline int8_t __attribute__ ((__always_inline__, __gnu_inline__)) vgetq_lane_s8(int8x16_t v, const int lane) {
  return __builtin_mpl_vector_getq_lane_v16i8(v, lane);
}

extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vgetq_lane_s16(int16x8_t v, const int lane) {
  return __builtin_mpl_vector_getq_lane_v8i16(v, lane);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vgetq_lane_s32(int32x4_t v, const int lane) {
  return __builtin_mpl_vector_getq_lane_v4i32(v, lane);
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vgetq_lane_s64(int64x2_t v, const int lane) {
  return __builtin_mpl_vector_getq_lane_v2i64(v, lane);
}

// vabdl
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vabdl_s8(int8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_labssub_low_v8i8(a, b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vabdl_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_labssub_low_v4i16(a, b);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vabdl_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_labssub_low_v2i32(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vabdl_u8(uint8x8_t a, uint8x8_t b) {
  return __builtin_mpl_vector_labssub_low_v8u8(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vabdl_u16(uint16x4_t a, uint16x4_t b) {
  return __builtin_mpl_vector_labssub_low_v4u16(a, b);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vabdl_u32(uint32x2_t a, uint32x2_t b) {
  return __builtin_mpl_vector_labssub_low_v2u32(a, b);
}

// vabdl_high
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vabdl_high_s8(int8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_labssub_high_v8i8(a, b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vabdl_high_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_labssub_high_v4i16(a, b);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vabdl_high_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_labssub_high_v2i32(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vabdl_high_u8(uint8x16_t a, uint8x16_t b) {
  return __builtin_mpl_vector_labssub_high_v8u8(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vabdl_high_u16(uint16x8_t a,
                                                                                            uint16x8_t b) {
  return __builtin_mpl_vector_labssub_high_v4u16(a, b);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vabdl_high_u32(uint32x4_t a,
                                                                                            uint32x4_t b) {
  return __builtin_mpl_vector_labssub_high_v2u32(a, b);
}

// vabs
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vabs_s8(int8x8_t a) {
  return __builtin_mpl_vector_abs_v8i8(a);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vabsq_s8(int8x16_t a) {
  return __builtin_mpl_vector_abs_v16i8(a);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vabs_s16(int16x4_t a) {
  return __builtin_mpl_vector_abs_v4i16(a);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vabsq_s16(int16x8_t a) {
  return __builtin_mpl_vector_abs_v8i16(a);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vabs_s32(int32x2_t a) {
  return __builtin_mpl_vector_abs_v2i32(a);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vabsq_s32(int32x4_t a) {
  return __builtin_mpl_vector_abs_v4i32(a);
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vabs_s64(int64x1_t a) {
  return __builtin_mpl_vector_abs_v1i64(a);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vabsq_s64(int64x2_t a) {
  return __builtin_mpl_vector_abs_v2i64(a);
}


// vaddv
extern inline int8_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddv_s8(int8x8_t a) {
  return __builtin_mpl_vector_sum_v8i8(a);
}

extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddv_s16(int16x4_t a) {
  return __builtin_mpl_vector_sum_v4i16(a);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddv_s32(int32x2_t a) {
  return vget_lane_s32(__builtin_mpl_vector_padd_v2i32(a, a), 0);
}

extern inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddv_u8(uint8x8_t a) {
  return __builtin_mpl_vector_sum_v8u8(a);
}

extern inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddv_u16(uint16x4_t a) {
  return __builtin_mpl_vector_sum_v4u16(a);
}

extern inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddv_u32(uint32x2_t a) {
  return vget_lane_u32(__builtin_mpl_vector_padd_v2u32(a, a), 0);
}

extern inline float32_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddv_f32(float32x2_t a) {
  return __builtin_mpl_vector_sum_v2f32(a);
}

extern inline int8_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddvq_s8(int8x16_t a) {
  return __builtin_mpl_vector_sum_v16i8(a);
}

extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddvq_s16(int16x8_t a) {
  return __builtin_mpl_vector_sum_v8i16(a);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddvq_s32(int32x4_t a) {
  return __builtin_mpl_vector_sum_v4i32(a);
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddvq_s64(int64x2_t a) {
  return __builtin_mpl_vector_sum_v2i64(a);
}

extern inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddvq_u8(uint8x16_t a) {
  return __builtin_mpl_vector_sum_v16u8(a);
}

extern inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddvq_u16(uint16x8_t a) {
  return __builtin_mpl_vector_sum_v8u16(a);
}

extern inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddvq_u32(uint32x4_t a) {
  return __builtin_mpl_vector_sum_v4u32(a);
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddvq_u64(uint64x2_t a) {
  return __builtin_mpl_vector_sum_v2u64(a);
}

extern inline float32_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddvq_f32(float32x4_t a) {
  return __builtin_mpl_vector_sum_v4f32(a);
}

extern inline float64_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddvq_f64(float64x2_t a) {
  return __builtin_mpl_vector_sum_v2f64(a);
}

// vqmovn
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqmovn_u16(uint16x8_t a) {
  return __builtin_mpl_vector_mov_narrow_v8u16(a);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqmovn_u32(uint32x4_t a) {
  return __builtin_mpl_vector_mov_narrow_v4u32(a);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqmovn_u64(uint64x2_t a) {
  return __builtin_mpl_vector_mov_narrow_v2u64(a);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqmovn_s16(int16x8_t a) {
  return __builtin_mpl_vector_mov_narrow_v8i16(a);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqmovn_s32(int32x4_t a) {
  return __builtin_mpl_vector_mov_narrow_v4i32(a);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqmovn_s64(int64x2_t a) {
  return __builtin_mpl_vector_mov_narrow_v2i64(a);
}

// vaddl
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddl_s8(int8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_addl_low_v8i8(a, b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddl_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_addl_low_v4i16(a, b);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddl_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_addl_low_v2i32(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddl_u8(uint8x8_t a, uint8x8_t b) {
  return __builtin_mpl_vector_addl_low_v8u8(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddl_u16(uint16x4_t a, uint16x4_t b) {
  return __builtin_mpl_vector_addl_low_v4u16(a, b);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddl_u32(uint32x2_t a, uint32x2_t b) {
  return __builtin_mpl_vector_addl_low_v2u32(a, b);
}

// vaddl_high
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddl_high_s8(int8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_addl_high_v8i8(a, b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddl_high_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_addl_high_v4i16(a, b);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddl_high_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_addl_high_v2i32(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddl_high_u8(uint8x16_t a, uint8x16_t b) {
  return __builtin_mpl_vector_addl_high_v8u8(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddl_high_u16(uint16x8_t a,
                                                                                            uint16x8_t b) {
  return __builtin_mpl_vector_addl_high_v4u16(a, b);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddl_high_u32(uint32x4_t a,
                                                                                            uint32x4_t b) {
  return __builtin_mpl_vector_addl_high_v2u32(a, b);
}

// vaddw
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddw_s8(int16x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_addw_low_v8i8(a, b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddw_s16(int32x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_addw_low_v4i16(a, b);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddw_s32(int64x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_addw_low_v2i32(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddw_u8(uint16x8_t a, uint8x8_t b) {
  return __builtin_mpl_vector_addw_low_v8u8(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddw_u16(uint32x4_t a, uint16x4_t b) {
  return __builtin_mpl_vector_addw_low_v4u16(a, b);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddw_u32(uint64x2_t a, uint32x2_t b) {
  return __builtin_mpl_vector_addw_low_v2u32(a, b);
}

// vaddw_high
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddw_high_s8(int16x8_t a, int8x16_t b) {
  return __builtin_mpl_vector_addw_high_v8i8(a, b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddw_high_s16(int32x4_t a, int16x8_t b) {
  return __builtin_mpl_vector_addw_high_v4i16(a, b);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddw_high_s32(int64x2_t a, int32x4_t b) {
  return __builtin_mpl_vector_addw_high_v2i32(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddw_high_u8(uint16x8_t a, uint8x16_t b) {
  return __builtin_mpl_vector_addw_high_v8u8(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddw_high_u16(uint32x4_t a,
                                                                                            uint16x8_t b) {
  return __builtin_mpl_vector_addw_high_v4u16(a, b);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddw_high_u32(uint64x2_t a,
                                                                                            uint32x4_t b) {
  return __builtin_mpl_vector_addw_high_v2u32(a, b);
}

// vadd
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vadd_s8(int8x8_t a, int8x8_t b) {
  return a + b;
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vadd_s16(int16x4_t a, int16x4_t b) {
  return a + b;
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vadd_s32(int32x2_t a, int32x2_t b) {
  return a + b;
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vadd_s64(int64x1_t a, int64x1_t b) {
  return a + b;
}


extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vadd_u8(uint8x8_t a, uint8x8_t b) {
  return a + b;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vadd_u16(uint16x4_t a, uint16x4_t b) {
  return a + b;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vadd_u32(uint32x2_t a, uint32x2_t b) {
  return a + b;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vadd_u64(uint64x1_t a, uint64x1_t b) {
  return a + b;
}

extern inline float32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vadd_f32(float32x2_t a, float32x2_t b) {
  return a + b;
}

extern inline float64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vadd_f64(float64x1_t a, float64x1_t b) {
  return a + b;
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddq_s8(int8x16_t a, int8x16_t b) {
  return a + b;
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddq_s16(int16x8_t a, int16x8_t b) {
  return a + b;
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddq_s32(int32x4_t a, int32x4_t b) {
  return a + b;
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddq_s64(int64x2_t a, int64x2_t b) {
  return a + b;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddq_u8(uint8x16_t a, uint8x16_t b) {
  return a + b;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddq_u16(uint16x8_t a, uint16x8_t b) {
  return a + b;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddq_u32(uint32x4_t a, uint32x4_t b) {
  return a + b;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddq_u64(uint64x2_t a, uint64x2_t b) {
  return a + b;
}

extern inline float32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddq_f32(float32x4_t a, float32x4_t b) {
  return a + b;
}

extern inline float64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddq_f64(float64x2_t a, float64x2_t b) {
  return a + b;
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddd_s64(int64_t a, int64_t b) {
  return a + b;
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddd_u64(uint64_t a, uint64_t b) {
  return a + b;
}

// vand
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vand_s8(int8x8_t a, int8x8_t b) {
  return a & b;
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vandq_s8(int8x16_t a, int8x16_t b) {
  return a & b;
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vand_s16(int16x4_t a, int16x4_t b) {
  return a & b;
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vandq_s16(int16x8_t a, int16x8_t b) {
  return a & b;
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vand_s32(int32x2_t a, int32x2_t b) {
  return a & b;
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vandq_s32(int32x4_t a, int32x4_t b) {
  return a & b;
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vand_s64(int64x1_t a, int64x1_t b) {
  return a & b;
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vandq_s64(int64x2_t a, int64x2_t b) {
  return a & b;
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vand_u8(uint8x8_t a, uint8x8_t b) {
  return a & b;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vandq_u8(uint8x16_t a, uint8x16_t b) {
  return a & b;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vand_u16(uint16x4_t a, uint16x4_t b) {
  return a & b;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vandq_u16(uint16x8_t a, uint16x8_t b) {
  return a & b;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vand_u32(uint32x2_t a, uint32x2_t b) {
  return a & b;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vandq_u32(uint32x4_t a, uint32x4_t b) {
  return a & b;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vand_u64(uint64x1_t a, uint64x1_t b) {
  return a & b;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vandq_u64(uint64x2_t a, uint64x2_t b) {
  return a & b;
}

// vand
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vorr_s8(int8x8_t a, int8x8_t b) {
  return a | b;
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vorrq_s8(int8x16_t a, int8x16_t b) {
  return a | b;
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vorr_s16(int16x4_t a, int16x4_t b) {
  return a | b;
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vorrq_s16(int16x8_t a, int16x8_t b) {
  return a | b;
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vorr_s32(int32x2_t a, int32x2_t b) {
  return a | b;
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vorrq_s32(int32x4_t a, int32x4_t b) {
  return a | b;
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vorr_s64(int64x1_t a, int64x1_t b) {
  return a | b;
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vorrq_s64(int64x2_t a, int64x2_t b) {
  return a | b;
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vorr_u8(uint8x8_t a, uint8x8_t b) {
  return a | b;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vorrq_u8(uint8x16_t a, uint8x16_t b) {
  return a | b;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vorr_u16(uint16x4_t a, uint16x4_t b) {
  return a | b;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vorrq_u16(uint16x8_t a, uint16x8_t b) {
  return a | b;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vorr_u32(uint32x2_t a, uint32x2_t b) {
  return a | b;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vorrq_u32(uint32x4_t a, uint32x4_t b) {
  return a | b;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vorr_u64(uint64x1_t a, uint64x1_t b) {
  return a | b;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vorrq_u64(uint64x2_t a, uint64x2_t b) {
  return a | b;
}

// vdup
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vdup_n_s8(int8_t value) {
  return __builtin_mpl_vector_from_scalar_v8i8(value);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupq_n_s8(int8_t value) {
  return __builtin_mpl_vector_from_scalar_v16i8(value);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vdup_n_s16(int16_t value) {
  return __builtin_mpl_vector_from_scalar_v4i16(value);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupq_n_s16(int16_t value) {
  return __builtin_mpl_vector_from_scalar_v8i16(value);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vdup_n_s32(int32_t value) {
  return __builtin_mpl_vector_from_scalar_v2i32(value);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupq_n_s32(int32_t value) {
  return __builtin_mpl_vector_from_scalar_v4i32(value);
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vdup_n_s64(int64_t value) {
  return __builtin_mpl_vector_from_scalar_v1i64(value);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupq_n_s64(int64_t value) {
  return __builtin_mpl_vector_from_scalar_v2i64(value);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vdup_n_u8(uint8_t value) {
  return __builtin_mpl_vector_from_scalar_v8u8(value);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupq_n_u8(uint8_t value) {
  return __builtin_mpl_vector_from_scalar_v16u8(value);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vdup_n_u16(uint16_t value) {
  return __builtin_mpl_vector_from_scalar_v4u16(value);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupq_n_u16(uint16_t value) {
  return __builtin_mpl_vector_from_scalar_v8u16(value);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vdup_n_u32(uint32_t value) {
  return __builtin_mpl_vector_from_scalar_v2u32(value);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupq_n_u32(uint32_t value) {
  return __builtin_mpl_vector_from_scalar_v4u32(value);
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vdup_n_u64(uint64_t value) {
  return __builtin_mpl_vector_from_scalar_v1u64(value);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupq_n_u64(uint64_t value) {
  return __builtin_mpl_vector_from_scalar_v2u64(value);
}

extern inline float32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vdup_n_f32(float32_t value) {
  return __builtin_mpl_vector_from_scalar_v2f32(value);
}

extern inline float64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vdup_n_f64(float64_t value) {
  return __builtin_mpl_vector_from_scalar_v1f64(value);
}

extern inline float32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupq_n_f32(float32_t value) {
  return __builtin_mpl_vector_from_scalar_v4f32(value);
}

extern inline float64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupq_n_f64(float64_t value) {
  return __builtin_mpl_vector_from_scalar_v2f64(value);
}

// vceq
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vceq_s8(int8x8_t a, int8x8_t b) {
  return a == b;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqq_s8(int8x16_t a, int8x16_t b) {
  return a == b;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vceq_s16(int16x4_t a, int16x4_t b) {
  return a == b;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqq_s16(int16x8_t a, int16x8_t b) {
  return a == b;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vceq_s32(int32x2_t a, int32x2_t b) {
  return a == b;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqq_s32(int32x4_t a, int32x4_t b) {
  return a == b;
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vceq_u8(uint8x8_t a, uint8x8_t b) {
  return a == b;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqq_u8(uint8x16_t a, uint8x16_t b) {
  return a == b;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vceq_u16(uint16x4_t a, uint16x4_t b) {
  return a == b;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqq_u16(uint16x8_t a, uint16x8_t b) {
  return a == b;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vceq_u32(uint32x2_t a, uint32x2_t b) {
  return a == b;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqq_u32(uint32x4_t a, uint32x4_t b) {
  return a == b;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vceq_s64(int64x1_t a, int64x1_t b) {
  return a == b;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqq_s64(int64x2_t a, int64x2_t b) {
  return a == b;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vceq_u64(uint64x1_t a, uint64x1_t b) {
  return a == b;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqq_u64(uint64x2_t a, uint64x2_t b) {
  return a == b;
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqd_s64(int64_t a, int64_t b) {
  return (a == b) ? -1LL : 0LL;
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqd_u64(uint64_t a, uint64_t b) {
  return (a == b) ? -1LL : 0LL;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vceq_f32(float32x2_t a, float32x2_t b) {
  return a == b;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vceq_f64(float64x1_t a, float64x1_t b) {
  return a == b;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqq_f32(float32x4_t a, float32x4_t b) {
  return a == b;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqq_f64(float64x2_t a, float64x2_t b) {
  return a == b;
}

// vceqz
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqz_s8(int8x8_t a) {
  return a == 0;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqzq_s8(int8x16_t a) {
  return a == 0;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqz_s16(int16x4_t a) {
  return a == 0;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqzq_s16(int16x8_t a) {
  return a == 0;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqz_s32(int32x2_t a) {
  return a == 0;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqzq_s32(int32x4_t a) {
  return a == 0;
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqz_u8(uint8x8_t a) {
  return a == 0;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqzq_u8(uint8x16_t a) {
  return a == 0;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqz_u16(uint16x4_t a) {
  return a == 0;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqzq_u16(uint16x8_t a) {
  return a == 0;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqz_u32(uint32x2_t a) {
  return a == 0;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqzq_u32(uint32x4_t a) {
  return a == 0;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqz_s64(int64x1_t a) {
  return a == 0;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqzq_s64(int64x2_t a) {
  return a == 0;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqz_u64(uint64x1_t a) {
  return a == 0;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqzq_u64(uint64x2_t a) {
  return a == 0;
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqzd_s64(int64_t a) {
  return (a == 0) ? -1LL : 0LL;
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vceqzd_u64(uint64_t a) {
  return (a == 0) ? -1LL : 0LL;
}

// vcgt
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgt_s8(int8x8_t a, int8x8_t b) {
  return a > b;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgtq_s8(int8x16_t a, int8x16_t b) {
  return a > b;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgt_s16(int16x4_t a, int16x4_t b) {
  return a > b;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgtq_s16(int16x8_t a, int16x8_t b) {
  return a > b;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgt_s32(int32x2_t a, int32x2_t b) {
  return a > b;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgtq_s32(int32x4_t a, int32x4_t b) {
  return a > b;
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgt_u8(uint8x8_t a, uint8x8_t b) {
  return a > b;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgtq_u8(uint8x16_t a, uint8x16_t b) {
  return a > b;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgt_u16(uint16x4_t a, uint16x4_t b) {
  return a > b;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgtq_u16(uint16x8_t a, uint16x8_t b) {
  return a > b;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgt_u32(uint32x2_t a, uint32x2_t b) {
  return a > b;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgtq_u32(uint32x4_t a, uint32x4_t b) {
  return a > b;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgt_s64(int64x1_t a, int64x1_t b) {
  return a > b;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgtq_s64(int64x2_t a, int64x2_t b) {
  return a > b;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgt_u64(uint64x1_t a, uint64x1_t b) {
  return a > b;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgtq_u64(uint64x2_t a, uint64x2_t b) {
  return a > b;
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgtd_s64(int64_t a, int64_t b) {
  return (a > b) ? -1LL : 0LL;
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgtd_u64(uint64_t a, uint64_t b) {
  return (a > b) ? -1LL : 0LL;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgt_f32(float32x2_t a, float32x2_t b) {
  return a > b;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgt_f64(float64x1_t a, float64x1_t b) {
  return a > b;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgtq_f32(float32x4_t a, float32x4_t b) {
  return a > b;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgtq_f64(float64x2_t a, float64x2_t b) {
  return a > b;
}

// vcgtz
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgtz_s8(int8x8_t a) {
  return a > 0;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgtzq_s8(int8x16_t a) {
  return a > 0;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgtz_s16(int16x4_t a) {
  return a > 0;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgtzq_s16(int16x8_t a) {
  return a > 0;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgtz_s32(int32x2_t a) {
  return a > 0;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgtzq_s32(int32x4_t a) {
  return a > 0;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgtz_s64(int64x1_t a) {
  return a > 0;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgtzq_s64(int64x2_t a) {
  return a > 0;
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgtzd_s64(int64_t a) {
  return (a > 0) ? -1LL : 0LL;
}

// vcge
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcge_s8(int8x8_t a, int8x8_t b) {
  return a >= b;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgeq_s8(int8x16_t a, int8x16_t b) {
  return a >= b;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcge_s16(int16x4_t a, int16x4_t b) {
  return a >= b;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgeq_s16(int16x8_t a, int16x8_t b) {
  return a >= b;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcge_s32(int32x2_t a, int32x2_t b) {
  return a >= b;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgeq_s32(int32x4_t a, int32x4_t b) {
  return a >= b;
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcge_u8(uint8x8_t a, uint8x8_t b) {
  return a >= b;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgeq_u8(uint8x16_t a, uint8x16_t b) {
  return a >= b;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcge_u16(uint16x4_t a, uint16x4_t b) {
  return a >= b;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgeq_u16(uint16x8_t a, uint16x8_t b) {
  return a >= b;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcge_u32(uint32x2_t a, uint32x2_t b) {
  return a >= b;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgeq_u32(uint32x4_t a, uint32x4_t b) {
  return a >= b;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vcge_s64(int64x1_t a, int64x1_t b) {
  return a >= b;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgeq_s64(int64x2_t a, int64x2_t b) {
  return a >= b;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vcge_u64(uint64x1_t a, uint64x1_t b) {
  return a >= b;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgeq_u64(uint64x2_t a, uint64x2_t b) {
  return a >= b;
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vcged_s64(int64_t a, int64_t b) {
  return (a >= b) ? -1LL : 0LL;
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vcged_u64(uint64_t a, uint64_t b) {
  return (a >= b) ? -1LL : 0LL;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcge_f32(float32x2_t a, float32x2_t b) {
  return a >= b;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vcge_f64(float64x1_t a, float64x1_t b) {
  return a >= b;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgeq_f32(float32x4_t a, float32x4_t b) {
  return a >= b;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgeq_f64(float64x2_t a, float64x2_t b) {
  return a >= b;
}

// vcgez
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgez_s8(int8x8_t a) {
  return a >= 0;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgezq_s8(int8x16_t a) {
  return a >= 0;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgez_s16(int16x4_t a) {
  return a >= 0;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgezq_s16(int16x8_t a) {
  return a >= 0;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgez_s32(int32x2_t a) {
  return a >= 0;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgezq_s32(int32x4_t a) {
  return a >= 0;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgez_s64(int64x1_t a) {
  return a >= 0;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgezq_s64(int64x2_t a) {
  return a >= 0;
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vcgezd_s64(int64_t a) {
  return (a >= 0) ? -1LL : 0LL;
}

// vclt
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vclt_s8(int8x8_t a, int8x8_t b) {
  return a < b;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vcltq_s8(int8x16_t a, int8x16_t b) {
  return a < b;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vclt_s16(int16x4_t a, int16x4_t b) {
  return a < b;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcltq_s16(int16x8_t a, int16x8_t b) {
  return a < b;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vclt_s32(int32x2_t a, int32x2_t b) {
  return a < b;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcltq_s32(int32x4_t a, int32x4_t b) {
  return a < b;
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vclt_u8(uint8x8_t a, uint8x8_t b) {
  return a < b;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vcltq_u8(uint8x16_t a, uint8x16_t b) {
  return a < b;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vclt_u16(uint16x4_t a, uint16x4_t b) {
  return a < b;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcltq_u16(uint16x8_t a, uint16x8_t b) {
  return a < b;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vclt_u32(uint32x2_t a, uint32x2_t b) {
  return a < b;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcltq_u32(uint32x4_t a, uint32x4_t b) {
  return a < b;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vclt_s64(int64x1_t a, int64x1_t b) {
  return a < b;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcltq_s64(int64x2_t a, int64x2_t b) {
  return a < b;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vclt_u64(uint64x1_t a, uint64x1_t b) {
  return a < b;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcltq_u64(uint64x2_t a, uint64x2_t b) {
  return a < b;
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vcltd_s64(int64_t a, int64_t b) {
  return (a < b) ? -1LL : 0LL;
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vcltd_u64(uint64_t a, uint64_t b) {
  return (a < b) ? -1LL : 0LL;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vclt_f32(float32x2_t a, float32x2_t b) {
  return a < b;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vclt_f64(float64x1_t a, float64x1_t b) {
  return a < b;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcltq_f32(float32x4_t a, float32x4_t b) {
  return a < b;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcltq_f64(float64x2_t a, float64x2_t b) {
  return a < b;
}

// vcltz
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcltz_s8(int8x8_t a) {
  return a < 0;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vcltzq_s8(int8x16_t a) {
  return a < 0;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcltz_s16(int16x4_t a) {
  return a < 0;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcltzq_s16(int16x8_t a) {
  return a < 0;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcltz_s32(int32x2_t a) {
  return a < 0;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcltzq_s32(int32x4_t a) {
  return a < 0;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vcltz_s64(int64x1_t a) {
  return a < 0;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcltzq_s64(int64x2_t a) {
  return a < 0;
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vcltzd_s64(int64_t a) {
  return (a < 0) ? -1LL : 0LL;
}

// vcle
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcle_s8(int8x8_t a, int8x8_t b) {
  return a <= b;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vcleq_s8(int8x16_t a, int8x16_t b) {
  return a <= b;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcle_s16(int16x4_t a, int16x4_t b) {
  return a <= b;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcleq_s16(int16x8_t a, int16x8_t b) {
  return a <= b;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcle_s32(int32x2_t a, int32x2_t b) {
  return a <= b;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcleq_s32(int32x4_t a, int32x4_t b) {
  return a <= b;
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcle_u8(uint8x8_t a, uint8x8_t b) {
  return a <= b;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vcleq_u8(uint8x16_t a, uint8x16_t b) {
  return a <= b;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcle_u16(uint16x4_t a, uint16x4_t b) {
  return a <= b;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcleq_u16(uint16x8_t a, uint16x8_t b) {
  return a <= b;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcle_u32(uint32x2_t a, uint32x2_t b) {
  return a <= b;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcleq_u32(uint32x4_t a, uint32x4_t b) {
  return a <= b;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vcle_s64(int64x1_t a, int64x1_t b) {
  return a <= b;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcleq_s64(int64x2_t a, int64x2_t b) {
  return a <= b;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vcle_u64(uint64x1_t a, uint64x1_t b) {
  return a <= b;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcleq_u64(uint64x2_t a, uint64x2_t b) {
  return a <= b;
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vcled_s64(int64_t a, int64_t b) {
  return (a <= b) ? -1LL : 0LL;
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vcled_u64(uint64_t a, uint64_t b) {
  return (a <= b) ? -1LL : 0LL;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcle_f32(float32x2_t a, float32x2_t b) {
  return a <= b;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vcle_f64(float64x1_t a, float64x1_t b) {
  return a <= b;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcleq_f32(float32x4_t a, float32x4_t b) {
  return a <= b;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcleq_f64(float64x2_t a, float64x2_t b) {
  return a <= b;
}

// vclez
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vclez_s8(int8x8_t a) {
  return a <= 0;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vclezq_s8(int8x16_t a) {
  return a <= 0;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vclez_s16(int16x4_t a) {
  return a <= 0;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vclezq_s16(int16x8_t a) {
  return a <= 0;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vclez_s32(int32x2_t a) {
  return a <= 0;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vclezq_s32(int32x4_t a) {
  return a <= 0;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vclez_s64(int64x1_t a) {
  return a <= 0;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vclezq_s64(int64x2_t a) {
  return a <= 0;
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vclezd_s64(int64_t a) {
  return (a <= 0) ? -1LL : 0LL;
}

// veor
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) veor_s8(int8x8_t a, int8x8_t b) {
  return a ^ b;
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) veorq_s8(int8x16_t a, int8x16_t b) {
  return a ^ b;
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) veor_s16(int16x4_t a, int16x4_t b) {
  return a ^ b;
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) veorq_s16(int16x8_t a, int16x8_t b) {
  return a ^ b;
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) veor_s32(int32x2_t a, int32x2_t b) {
  return a ^ b;
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) veorq_s32(int32x4_t a, int32x4_t b) {
  return a ^ b;
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) veor_s64(int64x1_t a, int64x1_t b) {
  return a ^ b;
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) veorq_s64(int64x2_t a, int64x2_t b) {
  return a ^ b;
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) veor_u8(uint8x8_t a, uint8x8_t b) {
  return a ^ b;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) veorq_u8(uint8x16_t a, uint8x16_t b) {
  return a ^ b;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) veor_u16(uint16x4_t a, uint16x4_t b) {
  return a ^ b;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) veorq_u16(uint16x8_t a, uint16x8_t b) {
  return a ^ b;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) veor_u32(uint32x2_t a, uint32x2_t b) {
  return a ^ b;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) veorq_u32(uint32x4_t a, uint32x4_t b) {
  return a ^ b;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) veor_u64(uint64x1_t a, uint64x1_t b) {
  return a ^ b;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) veorq_u64(uint64x2_t a, uint64x2_t b) {
  return a ^ b;
}

// vext
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vext_s8(int8x8_t a, int8x8_t b,
                                                                                   const int n) {
  return __builtin_mpl_vector_merge_v8i8(a, b, n);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vextq_s8(int8x16_t a, int8x16_t b,
                                                                                     const int n) {
  return __builtin_mpl_vector_merge_v16i8(a, b, n);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vext_s16(int16x4_t a, int16x4_t b,
                                                                                     const int n) {
  return __builtin_mpl_vector_merge_v4i16(a, b, n);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vextq_s16(int16x8_t a, int16x8_t b,
                                                                                      const int n) {
  return __builtin_mpl_vector_merge_v8i16(a, b, n);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vext_s32(int32x2_t a, int32x2_t b,
                                                                                     const int n) {
  return __builtin_mpl_vector_merge_v2i32(a, b, n);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vextq_s32(int32x4_t a, int32x4_t b,
                                                                                      const int n) {
  return __builtin_mpl_vector_merge_v4i32(a, b, n);
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vext_s64(int64x1_t a, int64x1_t b,
                                                                                     const int n) {
  return __builtin_mpl_vector_merge_v1i64(a, b, n);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vextq_s64(int64x2_t a, int64x2_t b,
                                                                                      const int n) {
  return __builtin_mpl_vector_merge_v2i64(a, b, n);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vext_u8(uint8x8_t a, uint8x8_t b,
                                                                                    const int n) {
  return __builtin_mpl_vector_merge_v8u8(a, b, n);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vextq_u8(uint8x16_t a, uint8x16_t b,
                                                                                      const int n) {
  return __builtin_mpl_vector_merge_v16u8(a, b, n);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vext_u16(uint16x4_t a, uint16x4_t b,
                                                                                      const int n) {
  return __builtin_mpl_vector_merge_v4u16(a, b, n);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vextq_u16(uint16x8_t a, uint16x8_t b,
                                                                                       const int n) {
  return __builtin_mpl_vector_merge_v8u16(a, b, n);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vext_u32(uint32x2_t a, uint32x2_t b,
                                                                                      const int n) {
  return __builtin_mpl_vector_merge_v2u32(a, b, n);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vextq_u32(uint32x4_t a, uint32x4_t b,
                                                                                       const int n) {
  return __builtin_mpl_vector_merge_v4u32(a, b, n);
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vext_u64(uint64x1_t a, uint64x1_t b,
                                                                                      const int n) {
  return __builtin_mpl_vector_merge_v1u64(a, b, n);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vextq_u64(uint64x2_t a, uint64x2_t b,
                                                                                       const int n) {
  return __builtin_mpl_vector_merge_v2u64(a, b, n);
}

extern inline float32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vext_f32(float32x2_t a, float32x2_t b,
                                                                                       const int n) {
  return __builtin_mpl_vector_merge_v2f32(a, b, n);
}

extern inline float64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vext_f64(float64x1_t a, float64x1_t b,
                                                                                       const int n) {
  return __builtin_mpl_vector_merge_v1f64(a, b, n);
}

extern inline float32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vextq_f32(float32x4_t a, float32x4_t b,
                                                                                        const int n) {
  return __builtin_mpl_vector_merge_v4f32(a, b, n);
}

extern inline float64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vextq_f64(float64x2_t a, float64x2_t b,
                                                                                        const int n) {
  return __builtin_mpl_vector_merge_v2f64(a, b, n);
}

// vget_high
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_high_s8(int8x16_t a) {
  return __builtin_mpl_vector_get_high_v16i8(a);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_high_s16(int16x8_t a) {
  return __builtin_mpl_vector_get_high_v8i16(a);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_high_s32(int32x4_t a) {
  return __builtin_mpl_vector_get_high_v4i32(a);
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_high_s64(int64x2_t a) {
  return __builtin_mpl_vector_get_high_v2i64(a);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_high_u8(uint8x16_t a) {
  return __builtin_mpl_vector_get_high_v16u8(a);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_high_u16(uint16x8_t a) {
  return __builtin_mpl_vector_get_high_v8u16(a);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_high_u32(uint32x4_t a) {
  return __builtin_mpl_vector_get_high_v4u32(a);
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_high_u64(uint64x2_t a) {
  return __builtin_mpl_vector_get_high_v2u64(a);
}

extern inline float32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_high_f32(float32x4_t a) {
  return __builtin_mpl_vector_get_high_v2f32(a);
}

extern inline float64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_high_f64(float64x1_t a) {
  return __builtin_mpl_vector_get_high_v1f64(a);
}

// vget_lane

extern inline float32_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_lane_f32(float32x2_t v,
                                                                                          const int lane) {
  return __builtin_mpl_vector_get_element_v2f32(v, lane);
}

extern inline float64_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_lane_f64(float64x1_t v,
                                                                                          const int lane) {
  return __builtin_mpl_vector_get_element_v1f64(v, lane);
}

extern inline float32_t __attribute__ ((__always_inline__, __gnu_inline__)) vgetq_lane_f32(float32x4_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_get_element_v4f32(v, lane);
}

extern inline float64_t __attribute__ ((__always_inline__, __gnu_inline__)) vgetq_lane_f64(float64x2_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_get_element_v2f64(v, lane);
}

// vget_low
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_low_s8(int8x16_t a) {
  return __builtin_mpl_vector_get_low_v16i8(a);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_low_s16(int16x8_t a) {
  return __builtin_mpl_vector_get_low_v8i16(a);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_low_s32(int32x4_t a) {
  return __builtin_mpl_vector_get_low_v4i32(a);
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_low_s64(int64x2_t a) {
  return __builtin_mpl_vector_get_low_v2i64(a);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_low_u8(uint8x16_t a) {
  return __builtin_mpl_vector_get_low_v16u8(a);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_low_u16(uint16x8_t a) {
  return __builtin_mpl_vector_get_low_v8u16(a);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_low_u32(uint32x4_t a) {
  return __builtin_mpl_vector_get_low_v4u32(a);
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_low_u64(uint64x2_t a) {
  return __builtin_mpl_vector_get_low_v2u64(a);
}

extern inline float32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_low_f32(float32x4_t a) {
  return __builtin_mpl_vector_get_low_v2f32(a);
}

extern inline float64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vget_low_f64(float64x2_t a) {
  return __builtin_mpl_vector_get_low_v1f64(a);
}

// vld1

extern inline float32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1_f32(float32_t const *ptr) {
  return __builtin_mpl_vector_load_v2f32(ptr);
}

extern inline float64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1_f64(float64_t const *ptr) {
  return __builtin_mpl_vector_load_v1f64(ptr);
}

extern inline float32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1q_f32(float32_t const *ptr) {
  return __builtin_mpl_vector_load_v4f32(ptr);
}

extern inline float64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1q_f64(float64_t const *ptr) {
  return __builtin_mpl_vector_load_v2f64(ptr);
}

// vmlal
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_s8(int16x8_t a, int8x8_t b,
                                                                                     int8x8_t c) {
  return __builtin_mpl_vector_madd_v8i8(a, b, c);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_s16(int32x4_t a, int16x4_t b,
                                                                                      int16x4_t c) {
  return __builtin_mpl_vector_madd_v4i16(a, b, c);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_s32(int64x2_t a, int32x2_t b,
                                                                                      int32x2_t c) {
  return __builtin_mpl_vector_madd_v2i32(a, b, c);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_u8(uint16x8_t a, uint8x8_t b,
                                                                                      uint8x8_t c) {
  return __builtin_mpl_vector_madd_v8u8(a, b, c);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_u16(uint32x4_t a, uint16x4_t b,
                                                                                       uint16x4_t c) {
  return __builtin_mpl_vector_madd_v4u16(a, b, c);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_u32(uint64x2_t a, uint32x2_t b,
                                                                                       uint32x2_t c) {
  return __builtin_mpl_vector_madd_v2u32(a, b, c);
}

// vmovl
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovl_s8(int8x8_t a) {
  return __builtin_mpl_vector_widen_low_v8i8(a);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovl_s16(int16x4_t a) {
  return __builtin_mpl_vector_widen_low_v4i16(a);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovl_s32(int32x2_t a) {
  return __builtin_mpl_vector_widen_low_v2i32(a);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovl_u8(uint8x8_t a) {
  return __builtin_mpl_vector_widen_low_v8u8(a);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovl_u16(uint16x4_t a) {
  return __builtin_mpl_vector_widen_low_v4u16(a);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovl_u32(uint32x2_t a) {
  return __builtin_mpl_vector_widen_low_v2u32(a);
}

// vmovl_high
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovl_high_s8(int8x16_t a) {
  return __builtin_mpl_vector_widen_high_v8i8(a);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovl_high_s16(int16x8_t a) {
  return __builtin_mpl_vector_widen_high_v4i16(a);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovl_high_s32(int32x4_t a) {
  return __builtin_mpl_vector_widen_high_v2i32(a);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovl_high_u8(uint8x16_t a) {
  return __builtin_mpl_vector_widen_high_v8u8(a);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovl_high_u16(uint16x8_t a) {
  return __builtin_mpl_vector_widen_high_v4u16(a);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovl_high_u32(uint32x4_t a) {
  return __builtin_mpl_vector_widen_high_v2u32(a);
}

// vmovn
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovn_s16(int16x8_t a) {
  return __builtin_mpl_vector_narrow_low_v8i16(a);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovn_s32(int32x4_t a) {
  return __builtin_mpl_vector_narrow_low_v4i32(a);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovn_s64(int64x2_t a) {
  return __builtin_mpl_vector_narrow_low_v2i64(a);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovn_u16(uint16x8_t a) {
  return __builtin_mpl_vector_narrow_low_v8u16(a);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovn_u32(uint32x4_t a) {
  return __builtin_mpl_vector_narrow_low_v4u32(a);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovn_u64(uint64x2_t a) {
  return __builtin_mpl_vector_narrow_low_v2u64(a);
}

// vmovn_high
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovn_high_s16(int8x8_t r, int16x8_t a) {
  return __builtin_mpl_vector_narrow_high_v8i16(r, a);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovn_high_s32(int16x4_t r, int32x4_t a) {
  return __builtin_mpl_vector_narrow_high_v4i32(r, a);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovn_high_s64(int32x2_t r, int64x2_t a) {
  return __builtin_mpl_vector_narrow_high_v2i64(r, a);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovn_high_u16(uint8x8_t r, uint16x8_t a) {
  return __builtin_mpl_vector_narrow_high_v8u16(r, a);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovn_high_u32(uint16x4_t r,
                                                                                            uint32x4_t a) {
  return __builtin_mpl_vector_narrow_high_v4u32(r, a);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovn_high_u64(uint32x2_t r,
                                                                                            uint64x2_t a) {
  return __builtin_mpl_vector_narrow_high_v2u64(r, a);
}

// vmul
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmul_s8(int8x8_t a, int8x8_t b) {
  return a * b;
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vmulq_s8(int8x16_t a, int8x16_t b) {
  return a * b;
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmul_s16(int16x4_t a, int16x4_t b) {
  return a * b;
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmulq_s16(int16x8_t a, int16x8_t b) {
  return a * b;
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmul_s32(int32x2_t a, int32x2_t b) {
  return a * b;
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmulq_s32(int32x4_t a, int32x4_t b) {
  return a * b;
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmul_u8(uint8x8_t a, uint8x8_t b) {
  return a * b;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vmulq_u8(uint8x16_t a, uint8x16_t b) {
  return a * b;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmul_u16(uint16x4_t a, uint16x4_t b) {
  return a * b;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmulq_u16(uint16x8_t a, uint16x8_t b) {
  return a * b;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmul_u32(uint32x2_t a, uint32x2_t b) {
  return a * b;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmulq_u32(uint32x4_t a, uint32x4_t b) {
  return a * b;
}

// vmull
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_s8(int8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_mull_low_v8i8(a, b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_mull_low_v4i16(a, b);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_mull_low_v2i32(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_u8(uint8x8_t a, uint8x8_t b) {
  return __builtin_mpl_vector_mull_low_v8u8(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_u16(uint16x4_t a, uint16x4_t b) {
  return __builtin_mpl_vector_mull_low_v4u16(a, b);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_u32(uint32x2_t a, uint32x2_t b) {
  return __builtin_mpl_vector_mull_low_v2u32(a, b);
}

// vmull_high
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_high_s8(int8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_mull_high_v8i8(a, b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_high_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_mull_high_v4i16(a, b);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_high_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_mull_high_v2i32(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_high_u8(uint8x16_t a, uint8x16_t b) {
  return __builtin_mpl_vector_mull_high_v8u8(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_high_u16(uint16x8_t a,
                                                                                            uint16x8_t b) {
  return __builtin_mpl_vector_mull_high_v4u16(a, b);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_high_u32(uint32x4_t a,
                                                                                            uint32x4_t b) {
  return __builtin_mpl_vector_mull_high_v2u32(a, b);
}

// vor
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vor_s8(int8x8_t a, int8x8_t b) {
  return a | b;
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vorq_s8(int8x16_t a, int8x16_t b) {
  return a | b;
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vor_s16(int16x4_t a, int16x4_t b) {
  return a | b;
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vorq_s16(int16x8_t a, int16x8_t b) {
  return a | b;
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vor_s32(int32x2_t a, int32x2_t b) {
  return a | b;
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vorq_s32(int32x4_t a, int32x4_t b) {
  return a | b;
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vor_s64(int64x1_t a, int64x1_t b) {
  return a | b;
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vorq_s64(int64x2_t a, int64x2_t b) {
  return a | b;
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vor_u8(uint8x8_t a, uint8x8_t b) {
  return a | b;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vorq_u8(uint8x16_t a, uint8x16_t b) {
  return a | b;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vor_u16(uint16x4_t a, uint16x4_t b) {
  return a | b;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vorq_u16(uint16x8_t a, uint16x8_t b) {
  return a | b;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vor_u32(uint32x2_t a, uint32x2_t b) {
  return a | b;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vorq_u32(uint32x4_t a, uint32x4_t b) {
  return a | b;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vor_u64(uint64x1_t a, uint64x1_t b) {
  return a | b;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vorq_u64(uint64x2_t a, uint64x2_t b) {
  return a | b;
}

// vpadal (add and accumulate long pairwise)
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vpadal_s8(int16x4_t a, int8x8_t b) {
  return __builtin_mpl_vector_pairwise_adalp_v8i8(a, b);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vpadalq_s8(int16x8_t a, int8x16_t b) {
  return __builtin_mpl_vector_pairwise_adalp_v16i8(a, b);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vpadal_s16(int32x2_t a, int16x4_t b) {
  return __builtin_mpl_vector_pairwise_adalp_v4i16(a, b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vpadalq_s16(int32x4_t a, int16x8_t b) {
  return __builtin_mpl_vector_pairwise_adalp_v8i16(a, b);
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vpadal_s32(int64x1_t a, int32x2_t b) {
  return __builtin_mpl_vector_pairwise_adalp_v2i32(a, b);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vpadalq_s32(int64x2_t a, int32x4_t b) {
  return __builtin_mpl_vector_pairwise_adalp_v4i32(a, b);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vpadal_u8(uint16x4_t a, uint8x8_t b) {
  return __builtin_mpl_vector_pairwise_adalp_v8u8(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vpadalq_u8(uint16x8_t a, uint8x16_t b) {
  return __builtin_mpl_vector_pairwise_adalp_v16u8(a, b);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vpadal_u16(uint32x2_t a, uint16x4_t b) {
  return __builtin_mpl_vector_pairwise_adalp_v4u16(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vpadalq_u16(uint32x4_t a, uint16x8_t b) {
  return __builtin_mpl_vector_pairwise_adalp_v8u16(a, b);
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vpadal_u32(uint64x1_t a, uint32x2_t b) {
  return __builtin_mpl_vector_pairwise_adalp_v2u32(a, b);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vpadalq_u32(uint64x2_t a, uint32x4_t b) {
  return __builtin_mpl_vector_pairwise_adalp_v4u32(a, b);
}

// vpaddl
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vpaddl_s8(int8x8_t a) {
  return __builtin_mpl_vector_pairwise_add_v8i8(a);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vpaddlq_s8(int8x16_t a) {
  return __builtin_mpl_vector_pairwise_add_v16i8(a);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vpaddl_s16(int16x4_t a) {
  return __builtin_mpl_vector_pairwise_add_v4i16(a);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vpaddlq_s16(int16x8_t a) {
  return __builtin_mpl_vector_pairwise_add_v8i16(a);
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vpaddl_s32(int32x2_t a) {
  return __builtin_mpl_vector_pairwise_add_v2i32(a);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vpaddlq_s32(int32x4_t a) {
  return __builtin_mpl_vector_pairwise_add_v4i32(a);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vpaddl_u8(uint8x8_t a) {
  return __builtin_mpl_vector_pairwise_add_v8u8(a);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vpaddlq_u8(uint8x16_t a) {
  return __builtin_mpl_vector_pairwise_add_v16u8(a);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vpaddl_u16(uint16x4_t a) {
  return __builtin_mpl_vector_pairwise_add_v4u16(a);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vpaddlq_u16(uint16x8_t a) {
  return __builtin_mpl_vector_pairwise_add_v8u16(a);
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vpaddl_u32(uint32x2_t a) {
  return __builtin_mpl_vector_pairwise_add_v2u32(a);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vpaddlq_u32(uint32x4_t a) {
  return __builtin_mpl_vector_pairwise_add_v4u32(a);
}

// vreinterpret 8
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s16_s8(int8x8_t a) {
  return (int16x4_t)a;
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s32_s8(int8x8_t a) {
  return (int32x2_t)a;
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u8_s8(int8x8_t a) {
  return (uint8x8_t)a;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u16_s8(int8x8_t a) {
  return (uint16x4_t)a;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u32_s8(int8x8_t a) {
  return (uint32x2_t)a;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u64_s8(int8x8_t a) {
  return (uint64x1_t)a;
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s64_s8(int8x8_t a) {
  return (int64x1_t)a;
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s8_u8(uint8x8_t a) {
  return (int8x8_t)a;
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s16_u8(uint8x8_t a) {
  return (int16x4_t)a;
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s32_u8(uint8x8_t a) {
  return (int32x2_t)a;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u16_u8(uint8x8_t a) {
  return (uint16x4_t)a;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u32_u8(uint8x8_t a) {
  return (uint32x2_t)a;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u64_u8(uint8x8_t a) {
  return (uint64x1_t)a;
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s64_u8(uint8x8_t a) {
  return (int64x1_t)a;
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s16_s8(int8x16_t a) {
  return (int16x8_t)a;
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s32_s8(int8x16_t a) {
  return (int32x4_t)a;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u8_s8(int8x16_t a) {
  return (uint8x16_t)a;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u16_s8(int8x16_t a) {
  return (uint16x8_t)a;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u32_s8(int8x16_t a) {
  return (uint32x4_t)a;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u64_s8(int8x16_t a) {
  return (uint64x2_t)a;
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s64_s8(int8x16_t a) {
  return (int64x2_t)a;
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s8_u8(uint8x16_t a) {
  return (int8x16_t)a;
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s16_u8(uint8x16_t a) {
  return (int16x8_t)a;
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s32_u8(uint8x16_t a) {
  return (int32x4_t)a;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u16_u8(uint8x16_t a) {
  return (uint16x8_t)a;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u32_u8(uint8x16_t a) {
  return (uint32x4_t)a;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u64_u8(uint8x16_t a) {
  return (uint64x2_t)a;
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s64_u8(uint8x16_t a) {
  return (int64x2_t)a;
}

extern inline float32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_f32_s8(int8x8_t a) {
  return (float32x2_t)a;
}

extern inline float64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_f64_s8(int8x8_t a) {
  return (float64x1_t)a;
}

extern inline float32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_f32_u8(uint8x8_t a) {
  return (float32x2_t)a;
}

extern inline float64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_f64_u8(uint8x8_t a) {
  return (float64x1_t)a;
}

extern inline float32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_f32_s8(int8x16_t a) {
  return (float32x4_t)a;
}

extern inline float64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_f64_s8(int8x16_t a) {
  return (float64x2_t)a;
}

extern inline float32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_f32_u8(uint8x16_t a) {
  return (float32x4_t)a;
}

extern inline float64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_f64_u8(uint8x16_t a) {
  return (float64x2_t)a;
}

// vreinterpret 16
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s8_s16(int16x4_t a) {
  return (int8x8_t)a;
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s32_s16(int16x4_t a) {
  return (int32x2_t)a;
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u8_s16(int16x4_t a) {
  return (uint8x8_t)a;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u16_s16(int16x4_t a) {
  return (uint16x4_t)a;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u32_s16(int16x4_t a) {
  return (uint32x2_t)a;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u64_s16(int16x4_t a) {
  return (uint64x1_t)a;
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s64_s16(int16x4_t a) {
  return (int64x1_t)a;
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s8_u16(uint16x4_t a) {
  return (int8x8_t)a;
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s16_u16(uint16x4_t a) {
  return (int16x4_t)a;
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s32_u16(uint16x4_t a) {
  return (int32x2_t)a;
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u8_u16(uint16x4_t a) {
  return (uint8x8_t)a;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u32_u16(uint16x4_t a) {
  return (uint32x2_t)a;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u64_u16(uint16x4_t a) {
  return (uint64x1_t)a;
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s64_u16(uint16x4_t a) {
  return (int64x1_t)a;
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s8_u16(uint16x8_t a) {
  return (int8x16_t)a;
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s16_u16(uint16x8_t a) {
  return (int16x8_t)a;
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s32_u16(uint16x8_t a) {
  return (int32x4_t)a;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u8_u16(uint16x8_t a) {
  return (uint8x16_t)a;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u32_u16(uint16x8_t a) {
  return (uint32x4_t)a;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u64_u16(uint16x8_t a) {
  return (uint64x2_t)a;
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s64_u16(uint16x8_t a) {
  return (int64x2_t)a;
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s8_s16(int16x8_t a) {
  return (int8x16_t)a;
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s32_s16(int16x8_t a) {
  return (int32x4_t)a;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u8_s16(int16x8_t a) {
  return (uint8x16_t)a;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u16_s16(int16x8_t a) {
  return (uint16x8_t)a;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u32_s16(int16x8_t a) {
  return (uint32x4_t)a;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u64_s16(int16x8_t a) {
  return (uint64x2_t)a;
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s64_s16(int16x8_t a) {
  return (int64x2_t)a;
}

extern inline float32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_f32_s16(int16x4_t a) {
  return (float32x2_t)a;
}

extern inline float64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_f64_s16(int16x4_t a) {
  return (float64x1_t)a;
}

extern inline float32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_f32_u16(uint16x4_t a) {
  return (float32x2_t)a;
}

extern inline float64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_f64_u16(uint16x4_t a) {
  return (float64x1_t)a;
}

extern inline float32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_f32_s16(int16x8_t a) {
  return (float32x4_t)a;
}

extern inline float64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_f64_s16(int16x8_t a) {
  return (float64x2_t)a;
}

extern inline float32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_f32_u16(uint16x8_t a) {
  return (float32x4_t)a;
}

extern inline float64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_f64_u16(uint16x8_t a) {
  return (float64x2_t)a;
}

// vreinterpret 32
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s8_s32(int32x2_t a) {
  return (int8x8_t)a;
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s16_s32(int32x2_t a) {
  return (int16x4_t)a;
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u8_s32(int32x2_t a) {
  return (uint8x8_t)a;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u16_s32(int32x2_t a) {
  return (uint16x4_t)a;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u32_s32(int32x2_t a) {
  return (uint32x2_t)a;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u64_s32(int32x2_t a) {
  return (uint64x1_t)a;
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s64_s32(int32x2_t a) {
  return (int64x1_t)a;
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s8_u32(uint32x2_t a) {
  return (int8x8_t)a;
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s16_u32(uint32x2_t a) {
  return (int16x4_t)a;
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s32_u32(uint32x2_t a) {
  return (int32x2_t)a;
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u8_u32(uint32x2_t a) {
  return (uint8x8_t)a;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u16_u32(uint32x2_t a) {
  return (uint16x4_t)a;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u64_u32(uint32x2_t a) {
  return (uint64x1_t)a;
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s64_u32(uint32x2_t a) {
  return (int64x1_t)a;
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s8_s32(int32x4_t a) {
  return (int8x16_t)a;
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s16_s32(int32x4_t a) {
  return (int16x8_t)a;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u8_s32(int32x4_t a) {
  return (uint8x16_t)a;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u16_s32(int32x4_t a) {
  return (uint16x8_t)a;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u32_s32(int32x4_t a) {
  return (uint32x4_t)a;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u64_s32(int32x4_t a) {
  return (uint64x2_t)a;
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s64_s32(int32x4_t a) {
  return (int64x2_t)a;
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s8_u32(uint32x4_t a) {
  return (int8x16_t)a;
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s16_u32(uint32x4_t a) {
  return (int16x8_t)a;
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s32_u32(uint32x4_t a) {
  return (int32x4_t)a;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u8_u32(uint32x4_t a) {
  return (uint8x16_t)a;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u16_u32(uint32x4_t a) {
  return (uint16x8_t)a;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u64_u32(uint32x4_t a) {
  return (uint64x2_t)a;
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s64_u32(uint32x4_t a) {
  return (int64x2_t)a;
}

extern inline float32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_f32_s32(int32x2_t a) {
  return (float32x2_t)a;
}

extern inline float64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_f64_s32(int32x2_t a) {
  return (float64x1_t)a;
}

extern inline float32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_f32_u32(uint32x2_t a) {
  return (float32x2_t)a;
}

extern inline float64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_f64_u32(uint32x2_t a) {
  return (float64x1_t)a;
}

extern inline float32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_f32_s32(int32x4_t a) {
  return (float32x4_t)a;
}

extern inline float64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_f64_s32(int32x4_t a) {
  return (float64x2_t)a;
}

extern inline float32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_f32_u32(uint32x4_t a) {
  return (float32x4_t)a;
}

extern inline float64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_f64_u32(uint32x4_t a) {
  return (float64x2_t)a;
}

// vreinterpret 64
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s8_s64(int64x1_t a) {
  return (int8x8_t)a;
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s16_s64(int64x1_t a) {
  return (int16x4_t)a;
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s32_s64(int64x1_t a) {
  return (int32x2_t)a;
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u8_s64(int64x1_t a) {
  return (uint8x8_t)a;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u16_s64(int64x1_t a) {
  return (uint16x4_t)a;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u32_s64(int64x1_t a) {
  return (uint32x2_t)a;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u64_s64(int64x1_t a) {
  return (uint64x1_t)a;
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s8_u64(uint64x1_t a) {
  return (int8x8_t)a;
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s16_u64(uint64x1_t a) {
  return (int16x4_t)a;
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s32_u64(uint64x1_t a) {
  return (int32x2_t)a;
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u8_u64(uint64x1_t a) {
  return (uint8x8_t)a;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u16_u64(uint64x1_t a) {
  return (uint16x4_t)a;
}
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_u32_u64(uint64x1_t a) {
  return (uint32x2_t)a;
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_s64_u64(uint64x1_t a) {
  return (int64x1_t)a;
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s8_s64(int64x2_t a) {
  return (int8x16_t)a;
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s16_s64(int64x2_t a) {
  return (int16x8_t)a;
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s32_s64(int64x2_t a) {
  return (int32x4_t)a;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u8_s64(int64x2_t a) {
  return (uint8x16_t)a;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u16_s64(int64x2_t a) {
  return (uint16x8_t)a;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u32_s64(int64x2_t a) {
  return (uint32x4_t)a;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u64_s64(int64x2_t a) {
  return (uint64x2_t)a;
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s8_u64(uint64x2_t a) {
  return (int8x16_t)a;
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s16_u64(uint64x2_t a) {
  return (int16x8_t)a;
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s32_u64(uint64x2_t a) {
  return (int32x4_t)a;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u8_u64(uint64x2_t a) {
  return (uint8x16_t)a;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u16_u64(uint64x2_t a) {
  return (uint16x8_t)a;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_u32_u64(uint64x2_t a) {
  return (uint32x4_t)a;
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_s64_u64(uint64x2_t a) {
  return (int64x2_t)a;
}

extern inline float32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_f32_s64(int64x1_t a) {
  return (float32x2_t)a;
}

extern inline float64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_f64_s64(int64x1_t a) {
  return (float64x1_t)a;
}

extern inline float32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_f32_u64(uint64x1_t a) {
  return (float32x2_t)a;
}

extern inline float64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpret_f64_u64(uint64x1_t a) {
  return (float64x1_t)a;
}

extern inline float32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_f32_s64(int64x2_t a) {
  return (float32x4_t)a;
}

extern inline float64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_f64_s64(int64x2_t a) {
  return (float64x2_t)a;
}

extern inline float32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_f32_u64(uint64x2_t a) {
  return (float32x4_t)a;
}

extern inline float64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vreinterpretq_f64_u64(uint64x2_t a) {
  return (float64x2_t)a;
}

// vrev32
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrev32_s8(int8x8_t vec) {
  return __builtin_mpl_vector_reverse_v8i8(vec);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vrev32q_s8(int8x16_t vec) {
  return __builtin_mpl_vector_reverse_v16i8(vec);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrev32_s16(int16x4_t vec) {
  return __builtin_mpl_vector_reverse_v4i16(vec);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrev32q_s16(int16x8_t vec) {
  return __builtin_mpl_vector_reverse_v8i16(vec);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrev32_u8(uint8x8_t vec) {
  return __builtin_mpl_vector_reverse_v8u8(vec);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vrev32q_u8(uint8x16_t vec) {
  return __builtin_mpl_vector_reverse_v16u8(vec);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrev32_u16(uint16x4_t vec) {
  return __builtin_mpl_vector_reverse_v4u16(vec);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrev32q_u16(uint16x8_t vec) {
  return __builtin_mpl_vector_reverse_v8u16(vec);
}

// vset_lane
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vset_lane_u8(uint8_t a, uint8x8_t v,
                                                                                         const int lane) {
  return __builtin_mpl_vector_set_element_v8u8(a, v, lane);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vset_lane_u16(uint16_t a, uint16x4_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_set_element_v4u16(a, v, lane);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vset_lane_u32(uint32_t a, uint32x2_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_set_element_v2u32(a, v, lane);
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vset_lane_u64(uint64_t a, uint64x1_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_set_element_v1u64(a, v, lane);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vset_lane_s8(int8_t a, int8x8_t v,
                                                                                        const int lane) {
  return __builtin_mpl_vector_set_element_v8i8(a, v, lane);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vset_lane_s16(int16_t a, int16x4_t v,
                                                                                          const int lane) {
  return __builtin_mpl_vector_set_element_v4i16(a, v, lane);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vset_lane_s32(int32_t a, int32x2_t v,
                                                                                          const int lane) {
  return __builtin_mpl_vector_set_element_v2i32(a, v, lane);
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vset_lane_s64(int64_t a, int64x1_t v,
                                                                                          const int lane) {
  return __builtin_mpl_vector_set_element_v1i64(a, v, lane);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vsetq_lane_u8(uint8_t a, uint8x16_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_set_element_v16u8(a, v, lane);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsetq_lane_u16(uint16_t a, uint16x8_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_set_element_v8u16(a, v, lane);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsetq_lane_u32(uint32_t a, uint32x4_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_set_element_v4u32(a, v, lane);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsetq_lane_u64(uint64_t a, uint64x2_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_set_element_v2u64(a, v, lane);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vsetq_lane_s8(int8_t a, int8x16_t v,
                                                                                          const int lane) {
  return __builtin_mpl_vector_set_element_v16i8(a, v, lane);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsetq_lane_s16(int16_t a, int16x8_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_set_element_v8i16(a, v, lane);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsetq_lane_s32(int32_t a, int32x4_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_set_element_v4i32(a, v, lane);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsetq_lane_s64(int64_t a, int64x2_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_set_element_v2i64(a, v, lane);
}

extern inline float32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vset_lane_f32(float32_t a, float32x2_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_set_element_v2f32(a, v, lane);
}

extern inline float64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vset_lane_f64(float64_t a, float64x1_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_set_element_v1f64(a, v, lane);
}
extern inline float32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsetq_lane_f32(float32_t a, float32x4_t v,
                                                                                             const int lane) {
  return __builtin_mpl_vector_set_element_v4f32(a, v, lane);
}

extern inline float64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsetq_lane_f64(float64_t a, float64x2_t v,
                                                                                             const int lane) {
  return __builtin_mpl_vector_set_element_v2f64(a, v, lane);
}

// vshl
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vshl_s8(int8x8_t a, int8x8_t b) {
  return a << b;
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vshlq_s8(int8x16_t a, int8x16_t b) {
  return a << b;
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vshl_s16(int16x4_t a, int16x4_t b) {
  return a << b;
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vshlq_s16(int16x8_t a, int16x8_t b) {
  return a << b;
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vshl_s32(int32x2_t a, int32x2_t b) {
  return a << b;
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vshlq_s32(int32x4_t a, int32x4_t b) {
  return a << b;
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vshl_s64(int64x1_t a, int64x1_t b) {
  return a << b;
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vshlq_s64(int64x2_t a, int64x2_t b) {
  return a << b;
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vshl_u8(uint8x8_t a, int8x8_t b) {
  return a << b;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vshlq_u8(uint8x16_t a, int8x16_t b) {
  return a << b;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vshl_u16(uint16x4_t a, int16x4_t b) {
  return a << b;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vshlq_u16(uint16x8_t a, int16x8_t b) {
  return a << b;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vshl_u32(uint32x2_t a, int32x2_t b) {
  return a << b;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vshlq_u32(uint32x4_t a, int32x4_t b) {
  return a << b;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vshl_u64(uint64x1_t a, int64x1_t b) {
  return a << b;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vshlq_u64(uint64x2_t a, int64x2_t b) {
  return a << b;
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vshld_s64(int64_t a, int64_t b) {
  return a << b;
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vshld_u64(uint64_t a, int64_t b) {
  return a << b;
}

// vshl_n
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vshl_n_s8(int8x8_t a, const int n) {
  return __builtin_mpl_vector_shli_v8i8(a, n);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vshlq_n_s8(int8x16_t a, const int n) {
  return __builtin_mpl_vector_shli_v16i8(a, n);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vshl_n_s16(int16x4_t a, const int n) {
  return __builtin_mpl_vector_shli_v4i16(a, n);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vshlq_n_s16(int16x8_t a, const int n) {
  return __builtin_mpl_vector_shli_v8i16(a, n);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vshl_n_s32(int32x2_t a, const int n) {
  return __builtin_mpl_vector_shli_v2i32(a, n);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vshlq_n_s32(int32x4_t a, const int n) {
  return __builtin_mpl_vector_shli_v4i32(a, n);
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vshl_n_s64(int64x1_t a, const int n) {
  return __builtin_mpl_vector_shli_v1i64(a, n);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vshlq_n_s64(int64x2_t a, const int n) {
  return __builtin_mpl_vector_shli_v2i64(a, n);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vshl_n_u8(uint8x8_t a, const int n) {
  return __builtin_mpl_vector_shli_v8u8(a, n);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vshlq_n_u8(uint8x16_t a, const int n) {
  return __builtin_mpl_vector_shli_v16u8(a, n);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vshl_n_u16(uint16x4_t a, const int n) {
  return __builtin_mpl_vector_shli_v4u16(a, n);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vshlq_n_u16(uint16x8_t a, const int n) {
  return __builtin_mpl_vector_shli_v8u16(a, n);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vshl_n_u32(uint32x2_t a, const int n) {
  return __builtin_mpl_vector_shli_v2u32(a, n);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vshlq_n_u32(uint32x4_t a, const int n) {
  return __builtin_mpl_vector_shli_v4u32(a, n);
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vshl_n_u64(uint64x1_t a, const int n) {
  return __builtin_mpl_vector_shli_v1u64(a, n);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vshlq_n_u64(uint64x2_t a, const int n) {
  return __builtin_mpl_vector_shli_v2u64(a, n);
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vshld_n_s64(int64_t a, const int n) {
  return vget_lane_s64(vshl_n_s64(vdup_n_s64(a), n), 0);
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vshld_n_u64(uint64_t a, const int n) {
  return vget_lane_u64(vshl_n_u64(vdup_n_u64(a), n), 0);
}

// vshr
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vshr_s8(int8x8_t a, int8x8_t b) {
  return a >> b;
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrq_s8(int8x16_t a, int8x16_t b) {
  return a >> b;
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vshr_s16(int16x4_t a, int16x4_t b) {
  return a >> b;
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrq_s16(int16x8_t a, int16x8_t b) {
  return a >> b;
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vshr_s32(int32x2_t a, int32x2_t b) {
  return a >> b;
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrq_s32(int32x4_t a, int32x4_t b) {
  return a >> b;
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vshr_s64(int64x1_t a, int64x1_t b) {
  return a >> b;
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrq_s64(int64x2_t a, int64x2_t b) {
  return a >> b;
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vshr_u8(uint8x8_t a, uint8x8_t b) {
  return a >> b;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrq_u8(uint8x16_t a, uint8x16_t b) {
  return a >> b;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vshr_u16(uint16x4_t a, uint16x4_t b) {
  return a >> b;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrq_u16(uint16x8_t a, uint16x8_t b) {
  return a >> b;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vshr_u32(uint32x2_t a, uint32x2_t b) {
  return a >> b;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrq_u32(uint32x4_t a, uint32x4_t b) {
  return a >> b;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vshr_u64(uint64x1_t a, uint64x1_t b) {
  return a >> b;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrq_u64(uint64x2_t a, uint64x2_t b) {
  return a >> b;
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrd_s64(int64_t a, int64_t b) {
  return vget_lane_s64((vdup_n_s64(a) >> vdup_n_s64(b)), 0);
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrd_u64(uint64_t a, uint64_t b) {
  return vget_lane_u64((vdup_n_u64(a) >> vdup_n_u64(b)), 0);
}

// vshr_n
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vshr_n_s8(int8x8_t a, const int n) {
  return __builtin_mpl_vector_shri_v8i8(a, n);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrq_n_s8(int8x16_t a, const int n) {
  return __builtin_mpl_vector_shri_v16i8(a, n);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vshr_n_s16(int16x4_t a, const int n) {
  return __builtin_mpl_vector_shri_v4i16(a, n);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrq_n_s16(int16x8_t a, const int n) {
  return __builtin_mpl_vector_shri_v8i16(a, n);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vshr_n_s32(int32x2_t a, const int n) {
  return __builtin_mpl_vector_shri_v2i32(a, n);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrq_n_s32(int32x4_t a, const int n) {
  return __builtin_mpl_vector_shri_v4i32(a, n);
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vshr_n_s64(int64x1_t a, const int n) {
  return __builtin_mpl_vector_shri_v1i64(a, n);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrq_n_s64(int64x2_t a, const int n) {
  return __builtin_mpl_vector_shri_v2i64(a, n);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vshr_n_u8(uint8x8_t a, const int n) {
  return __builtin_mpl_vector_shru_v8u8(a, n);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrq_n_u8(uint8x16_t a, const int n) {
  return __builtin_mpl_vector_shru_v16u8(a, n);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vshr_n_u16(uint16x4_t a, const int n) {
  return __builtin_mpl_vector_shru_v4u16(a, n);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrq_n_u16(uint16x8_t a, const int n) {
  return __builtin_mpl_vector_shru_v8u16(a, n);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vshr_n_u32(uint32x2_t a, const int n) {
  return __builtin_mpl_vector_shru_v2u32(a, n);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrq_n_u32(uint32x4_t a, const int n) {
  return __builtin_mpl_vector_shru_v4u32(a, n);
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vshr_n_u64(uint64x1_t a, const int n) {
  return __builtin_mpl_vector_shru_v1u64(a, n);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrq_n_u64(uint64x2_t a, const int n) {
  return __builtin_mpl_vector_shru_v2u64(a, n);
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrd_n_s64(int64_t a, const int n) {
  return vget_lane_s64(vshr_n_s64(vdup_n_s64(a), n), 0);
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrd_n_u64(uint64_t a, const int n) {
  return vget_lane_u64(vshr_n_u64(vdup_n_u64(a), n), 0);
}

// vshrn_n
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrn_n_s16(int16x8_t a, const int n) {
  return __builtin_mpl_vector_shr_narrow_low_v8i16(a, n);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrn_n_s32(int32x4_t a, const int n) {
  return __builtin_mpl_vector_shr_narrow_low_v4i32(a, n);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrn_n_s64(int64x2_t a, const int n) {
  return __builtin_mpl_vector_shr_narrow_low_v2i64(a, n);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrn_n_u16(uint16x8_t a, const int n) {
  return __builtin_mpl_vector_shr_narrow_low_v8u16(a, n);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrn_n_u32(uint32x4_t a, const int n) {
  return __builtin_mpl_vector_shr_narrow_low_v4u32(a, n);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrn_n_u64(uint64x2_t a, const int n) {
  return __builtin_mpl_vector_shr_narrow_low_v2u64(a, n);
}

// vst1
extern inline void __attribute__ ((__always_inline__, __gnu_inline__)) vst1_s8(int8_t *ptr, int8x8_t val) {
  return __builtin_mpl_vector_store_v8i8(ptr, val);
}

extern inline void __attribute__ ((__always_inline__, __gnu_inline__)) vst1q_s8(int8_t *ptr, int8x16_t val) {
  return __builtin_mpl_vector_store_v16i8(ptr, val);
}

extern inline void __attribute__ ((__always_inline__, __gnu_inline__)) vst1_s16(int16_t *ptr, int16x4_t val) {
  return __builtin_mpl_vector_store_v4i16(ptr, val);
}

extern inline void __attribute__ ((__always_inline__, __gnu_inline__)) vst1q_s16(int16_t *ptr, int16x8_t val) {
  return __builtin_mpl_vector_store_v8i16(ptr, val);
}

extern inline void __attribute__ ((__always_inline__, __gnu_inline__)) vst1_s32(int32_t *ptr, int32x2_t val) {
  return __builtin_mpl_vector_store_v2i32(ptr, val);
}

extern inline void __attribute__ ((__always_inline__, __gnu_inline__)) vst1q_s32(int32_t *ptr, int32x4_t val) {
  return __builtin_mpl_vector_store_v4i32(ptr, val);
}

extern inline void __attribute__ ((__always_inline__, __gnu_inline__)) vst1_s64(int64_t *ptr, int64x1_t val) {
  return __builtin_mpl_vector_store_v1i64(ptr, val);
}

extern inline void __attribute__ ((__always_inline__, __gnu_inline__)) vst1q_s64(int64_t *ptr, int64x2_t val) {
  return __builtin_mpl_vector_store_v2i64(ptr, val);
}

extern inline void __attribute__ ((__always_inline__, __gnu_inline__)) vst1_u8(uint8_t *ptr, uint8x8_t val) {
  return __builtin_mpl_vector_store_v8u8(ptr, val);
}

extern inline void __attribute__ ((__always_inline__, __gnu_inline__)) vst1q_u8(uint8_t *ptr, uint8x16_t val) {
  return __builtin_mpl_vector_store_v16u8(ptr, val);
}

extern inline void __attribute__ ((__always_inline__, __gnu_inline__)) vst1_u16(uint16_t *ptr, uint16x4_t val) {
  return __builtin_mpl_vector_store_v4u16(ptr, val);
}

extern inline void __attribute__ ((__always_inline__, __gnu_inline__)) vst1q_u16(uint16_t *ptr, uint16x8_t val) {
  return __builtin_mpl_vector_store_v8u16(ptr, val);
}

extern inline void __attribute__ ((__always_inline__, __gnu_inline__)) vst1_u32(uint32_t *ptr, uint32x2_t val) {
  return __builtin_mpl_vector_store_v2u32(ptr, val);
}

extern inline void __attribute__ ((__always_inline__, __gnu_inline__)) vst1q_u32(uint32_t *ptr, uint32x4_t val) {
  return __builtin_mpl_vector_store_v4u32(ptr, val);
}

extern inline void __attribute__ ((__always_inline__, __gnu_inline__)) vst1_u64(uint64_t *ptr, uint64x1_t val) {
  return __builtin_mpl_vector_store_v1u64(ptr, val);
}

extern inline void __attribute__ ((__always_inline__, __gnu_inline__)) vst1q_u64(uint64_t *ptr, uint64x2_t val) {
  return __builtin_mpl_vector_store_v2u64(ptr, val);
}

extern inline void __attribute__ ((__always_inline__, __gnu_inline__)) vst1_f32(float32_t *ptr, float32x2_t val) {
  return __builtin_mpl_vector_store_v2f32(ptr, val);
}

extern inline void __attribute__ ((__always_inline__, __gnu_inline__)) vst1_f64(float64_t *ptr, float64x1_t val) {
  return  __builtin_mpl_vector_store_v1f64(ptr, val);
}

extern inline void __attribute__ ((__always_inline__, __gnu_inline__)) vst1q_f32(float32_t *ptr, float32x4_t val) {
  return __builtin_mpl_vector_store_v4f32(ptr, val);
}

extern inline void __attribute__ ((__always_inline__, __gnu_inline__)) vst1q_f64(float64_t *ptr, float64x2_t val) {
  return __builtin_mpl_vector_store_v2f64(ptr, val);
}

// vsub
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsub_s8(int8x8_t a, int8x8_t b) {
  return a - b;
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubq_s8(int8x16_t a, int8x16_t b) {
  return a - b;
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsub_s16(int16x4_t a, int16x4_t b) {
  return a - b;
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubq_s16(int16x8_t a, int16x8_t b) {
  return a - b;
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsub_s32(int32x2_t a, int32x2_t b) {
  return a - b;
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubq_s32(int32x4_t a, int32x4_t b) {
  return a - b;
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vsub_s64(int64x1_t a, int64x1_t b) {
  return a - b;
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubq_s64(int64x2_t a, int64x2_t b) {
  return a - b;
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsub_u8(uint8x8_t a, uint8x8_t b) {
  return a - b;
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubq_u8(uint8x16_t a, uint8x16_t b) {
  return a - b;
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsub_u16(uint16x4_t a, uint16x4_t b) {
  return a - b;
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubq_u16(uint16x8_t a, uint16x8_t b) {
  return a - b;
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsub_u32(uint32x2_t a, uint32x2_t b) {
  return a - b;
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubq_u32(uint32x4_t a, uint32x4_t b) {
  return a - b;
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vsub_u64(uint64x1_t a, uint64x1_t b) {
  return a - b;
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubq_u64(uint64x2_t a, uint64x2_t b) {
  return a - b;
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubd_s64(int64_t a, int64_t b) {
  return a - b;
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubd_u64(uint64_t a, uint64_t b) {
  return a - b;
}

extern inline float32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsub_f32(float32x2_t a, float32x2_t b) {
  return a - b;
}

extern inline float64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vsub_f64(float64x1_t a, float64x1_t b) {
  return a - b;
}

extern inline float32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubq_f32(float32x4_t a, float32x4_t b) {
  return a - b;
}

extern inline float64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubq_f64(float64x2_t a, float64x2_t b) {
  return a - b;
}

// vsub[lw]
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubl_s8(int8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_subl_low_v8i8(a, b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubl_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_subl_low_v4i16(a, b);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubl_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_subl_low_v2i32(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubl_u8(uint8x8_t a, uint8x8_t b) {
  return __builtin_mpl_vector_subl_low_v8u8(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubl_u16(uint16x4_t a, uint16x4_t b) {
  return __builtin_mpl_vector_subl_low_v4u16(a, b);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubl_u32(uint32x2_t a, uint32x2_t b) {
  return __builtin_mpl_vector_subl_low_v2u32(a, b);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubl_high_s8(int8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_subl_high_v8i8(a, b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubl_high_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_subl_high_v4i16(a, b);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubl_high_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_subl_high_v2i32(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubl_high_u8(uint8x16_t a, uint8x16_t b) {
  return __builtin_mpl_vector_subl_high_v8u8(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubl_high_u16(uint16x8_t a,
                                                                                            uint16x8_t b) {
  return __builtin_mpl_vector_subl_high_v4u16(a, b);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubl_high_u32(uint32x4_t a,
                                                                                            uint32x4_t b) {
  return __builtin_mpl_vector_subl_high_v2u32(a, b);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubw_s8(int16x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_subw_low_v8i8(a, b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubw_s16(int32x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_subw_low_v4i16(a, b);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubw_s32(int64x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_subw_low_v2i32(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubw_u8(uint16x8_t a, uint8x8_t b) {
  return __builtin_mpl_vector_subw_low_v8u8(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubw_u16(uint32x4_t a, uint16x4_t b) {
  return __builtin_mpl_vector_subw_low_v4u16(a, b);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubw_u32(uint64x2_t a, uint32x2_t b) {
  return __builtin_mpl_vector_subw_low_v2u32(a, b);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubw_high_s8(int16x8_t a, int8x16_t b) {
  return __builtin_mpl_vector_subw_high_v8i8(a, b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubw_high_s16(int32x4_t a, int16x8_t b) {
  return __builtin_mpl_vector_subw_high_v4i16(a, b);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubw_high_s32(int64x2_t a, int32x4_t b) {
  return __builtin_mpl_vector_subw_high_v2i32(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubw_high_u8(uint16x8_t a,
                                                                                           uint8x16_t b) {
  return __builtin_mpl_vector_subw_high_v8u8(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubw_high_u16(uint32x4_t a,
                                                                                            uint16x8_t b) {
  return __builtin_mpl_vector_subw_high_v4u16(a, b);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubw_high_u32(uint64x2_t a,
                                                                                            uint32x4_t b) {
  return __builtin_mpl_vector_subw_high_v2u32(a, b);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vabd_s8(int8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_abd_v8i8(a, b);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vabdq_s8(int8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_abdq_v16i8(a, b);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vabd_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_abd_v4i16(a, b);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vabdq_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_abdq_v8i16(a, b);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vabd_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_abd_v2i32(a, b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vabdq_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_abdq_v4i32(a, b);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vabd_u8(uint8x8_t a, uint8x8_t b) {
  return __builtin_mpl_vector_abd_v8u8(a, b);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vabdq_u8(uint8x16_t a, uint8x16_t b) {
  return __builtin_mpl_vector_abdq_v16u8(a, b);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vabd_u16(uint16x4_t a, uint16x4_t b) {
  return __builtin_mpl_vector_abd_v4u16(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vabdq_u16(uint16x8_t a, uint16x8_t b) {
  return __builtin_mpl_vector_abdq_v8u16(a, b);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vabd_u32(uint32x2_t a, uint32x2_t b) {
  return __builtin_mpl_vector_abd_v2u32(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vabdq_u32(uint32x4_t a, uint32x4_t b) {
  return __builtin_mpl_vector_abdq_v4u32(a, b);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmax_s8(int8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_max_v8i8(a, b);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vmaxq_s8(int8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_maxq_v16i8(a, b);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmax_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_max_v4i16(a, b);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmaxq_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_maxq_v8i16(a, b);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmax_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_max_v2i32(a, b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmaxq_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_maxq_v4i32(a, b);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmax_u8(uint8x8_t a, uint8x8_t b) {
  return __builtin_mpl_vector_max_v8u8(a, b);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vmaxq_u8(uint8x16_t a, uint8x16_t b) {
  return __builtin_mpl_vector_maxq_v16u8(a, b);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmax_u16(uint16x4_t a, uint16x4_t b) {
  return __builtin_mpl_vector_max_v4u16(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmaxq_u16(uint16x8_t a, uint16x8_t b) {
  return __builtin_mpl_vector_maxq_v8u16(a, b);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmax_u32(uint32x2_t a, uint32x2_t b) {
  return __builtin_mpl_vector_max_v2u32(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmaxq_u32(uint32x4_t a, uint32x4_t b) {
  return __builtin_mpl_vector_maxq_v4u32(a, b);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmin_s8(int8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_min_v8i8(a, b);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vminq_s8(int8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_minq_v16i8(a, b);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmin_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_min_v4i16(a, b);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vminq_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_minq_v8i16(a, b);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmin_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_min_v2i32(a, b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vminq_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_minq_v4i32(a, b);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmin_u8(uint8x8_t a, uint8x8_t b) {
  return __builtin_mpl_vector_min_v8u8(a, b);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vminq_u8(uint8x16_t a, uint8x16_t b) {
  return __builtin_mpl_vector_minq_v16u8(a, b);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmin_u16(uint16x4_t a, uint16x4_t b) {
  return __builtin_mpl_vector_min_v4u16(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vminq_u16(uint16x8_t a, uint16x8_t b) {
  return __builtin_mpl_vector_minq_v8u16(a, b);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmin_u32(uint32x2_t a, uint32x2_t b) {
  return __builtin_mpl_vector_min_v2u32(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vminq_u32(uint32x4_t a, uint32x4_t b) {
  return __builtin_mpl_vector_minq_v4u32(a, b);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vrecpe_u32(uint32x2_t a) {
  return __builtin_mpl_vector_recpe_v2u32(a);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrecpeq_u32(uint32x4_t a) {
  return __builtin_mpl_vector_recpeq_v4u32(a);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vpadd_s8(int8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_padd_v8i8(a, b);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vpadd_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_padd_v4i16(a, b);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vpadd_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_padd_v2i32(a, b);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vpadd_u8(uint8x8_t a, uint8x8_t b) {
  return __builtin_mpl_vector_padd_v8u8(a, b);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vpadd_u16(uint16x4_t a, uint16x4_t b) {
  return __builtin_mpl_vector_padd_v4u16(a, b);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vpadd_u32(uint32x2_t a, uint32x2_t b) {
  return __builtin_mpl_vector_padd_v2u32(a, b);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vpaddq_s8(int8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_paddq_v16i8(a, b);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vpaddq_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_paddq_v8i16(a, b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vpaddq_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_paddq_v4i32(a, b);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vpaddq_s64(int64x2_t a, int64x2_t b) {
  return __builtin_mpl_vector_paddq_v2i64(a, b);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vpaddq_u8(uint8x16_t a, uint8x16_t b) {
  return __builtin_mpl_vector_paddq_v16u8(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vpaddq_u16(uint16x8_t a, uint16x8_t b) {
  return __builtin_mpl_vector_paddq_v8u16(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vpaddq_u32(uint32x4_t a, uint32x4_t b) {
  return __builtin_mpl_vector_paddq_v4u32(a, b);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vpaddq_u64(uint64x2_t a, uint64x2_t b) {
  return __builtin_mpl_vector_paddq_v2u64(a, b);
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vpaddd_s64(int64x2_t a) {
  return vget_lane_s64(__builtin_mpl_vector_paddd_v2i64(a), 0);
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vpaddd_u64(uint64x2_t a) {
  return vget_lane_u64(__builtin_mpl_vector_paddd_v2u64(a), 0);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vpmax_s8(int8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_pmax_v8i8(a, b);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vpmax_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_pmax_v4i16(a, b);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vpmax_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_pmax_v2i32(a, b);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vpmax_u8(uint8x8_t a, uint8x8_t b) {
  return __builtin_mpl_vector_pmax_v8u8(a, b);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vpmax_u16(uint16x4_t a, uint16x4_t b) {
  return __builtin_mpl_vector_pmax_v4u16(a, b);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vpmax_u32(uint32x2_t a, uint32x2_t b) {
  return __builtin_mpl_vector_pmax_v2u32(a, b);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vpmaxq_s8(int8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_pmaxq_v16i8(a, b);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vpmaxq_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_pmaxq_v8i16(a, b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vpmaxq_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_pmaxq_v4i32(a, b);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vpmaxq_u8(uint8x16_t a, uint8x16_t b) {
  return __builtin_mpl_vector_pmaxq_v16u8(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vpmaxq_u16(uint16x8_t a, uint16x8_t b) {
  return __builtin_mpl_vector_pmaxq_v8u16(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vpmaxq_u32(uint32x4_t a, uint32x4_t b) {
  return __builtin_mpl_vector_pmaxq_v4u32(a, b);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vpmin_s8(int8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_pmin_v8i8(a, b);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vpmin_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_pmin_v4i16(a, b);
}

int32x2_t __builtin_mpl_vector_pmin_v2i32(int32x2_t a, int32x2_t b);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vpmin_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_pmin_v2i32(a, b);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vpmin_u8(uint8x8_t a, uint8x8_t b) {
  return __builtin_mpl_vector_pmin_v8u8(a, b);
}

uint16x4_t __builtin_mpl_vector_pmin_v4u16(uint16x4_t a, uint16x4_t b);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vpmin_u16(uint16x4_t a, uint16x4_t b) {
  return __builtin_mpl_vector_pmin_v4u16(a, b);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vpmin_u32(uint32x2_t a, uint32x2_t b) {
  return __builtin_mpl_vector_pmin_v2u32(a, b);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vpminq_s8(int8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_pminq_v16i8(a, b);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vpminq_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_pminq_v8i16(a, b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vpminq_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_pminq_v4i32(a, b);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vpminq_u8(uint8x16_t a, uint8x16_t b) {
  return __builtin_mpl_vector_pminq_v16u8(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vpminq_u16(uint16x8_t a, uint16x8_t b) {
  return __builtin_mpl_vector_pminq_v8u16(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vpminq_u32(uint32x4_t a, uint32x4_t b) {
  return __builtin_mpl_vector_pminq_v4u32(a, b);
}

extern inline int8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmaxv_s8(int8x8_t a) {
  return vget_lane_s8(__builtin_mpl_vector_maxv_v8i8(a), 0);
}

extern inline int8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmaxvq_s8(int8x16_t a) {
  return vgetq_lane_s8(__builtin_mpl_vector_maxvq_v16i8(a), 0);
}

extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vmaxv_s16(int16x4_t a) {
  return vget_lane_s16(__builtin_mpl_vector_maxv_v4i16(a), 0);
}

extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vmaxvq_s16(int16x8_t a) {
  return vgetq_lane_s16(__builtin_mpl_vector_maxvq_v8i16(a), 0);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vmaxv_s32(int32x2_t a) {
  return vget_lane_s32(__builtin_mpl_vector_maxv_v2i32(a), 0);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vmaxvq_s32(int32x4_t a) {
  return vgetq_lane_s32(__builtin_mpl_vector_maxvq_v4i32(a), 0);
}

extern inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmaxv_u8(uint8x8_t a) {
  return vget_lane_u8(__builtin_mpl_vector_maxv_v8u8(a), 0);
}

extern inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmaxvq_u8(uint8x16_t a) {
  return vgetq_lane_u8(__builtin_mpl_vector_maxvq_v16u8(a), 0);
}

extern inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__)) vmaxv_u16(uint16x4_t a) {
  return vget_lane_u16(__builtin_mpl_vector_maxv_v4u16(a), 0);
}

extern inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__)) vmaxvq_u16(uint16x8_t a) {
  return vgetq_lane_u16(__builtin_mpl_vector_maxvq_v8u16(a), 0);
}

extern inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__)) vmaxv_u32(uint32x2_t a) {
  return vget_lane_u32(__builtin_mpl_vector_maxv_v2u32(a), 0);
}

extern inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__)) vmaxvq_u32(uint32x4_t a) {
  return vgetq_lane_u32(__builtin_mpl_vector_maxvq_v4u32(a), 0);
}

extern inline int8_t __attribute__ ((__always_inline__, __gnu_inline__)) vminv_s8(int8x8_t a) {
  return vget_lane_s8(__builtin_mpl_vector_minv_v8i8(a), 0);
}

extern inline int8_t __attribute__ ((__always_inline__, __gnu_inline__)) vminvq_s8(int8x16_t a) {
  return vgetq_lane_s8(__builtin_mpl_vector_minvq_v16i8(a), 0);
}

extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vminv_s16(int16x4_t a) {
  return vget_lane_s16(__builtin_mpl_vector_minv_v4i16(a), 0);
}

extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vminvq_s16(int16x8_t a) {
  return vgetq_lane_s16(__builtin_mpl_vector_minvq_v8i16(a), 0);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vminv_s32(int32x2_t a) {
  return vget_lane_s32(__builtin_mpl_vector_minv_v2i32(a), 0);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vminvq_s32(int32x4_t a) {
  return vgetq_lane_s32(__builtin_mpl_vector_minvq_v4i32(a), 0);
}

extern inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__)) vminv_u8(uint8x8_t a) {
  return vget_lane_u8(__builtin_mpl_vector_minv_v8u8(a), 0);
}

extern inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__)) vminvq_u8(uint8x16_t a) {
  return vgetq_lane_u8(__builtin_mpl_vector_minvq_v16u8(a), 0);
}

extern inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__)) vminv_u16(uint16x4_t a) {
  return vget_lane_u16(__builtin_mpl_vector_minv_v4u16(a), 0);
}

extern inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__)) vminvq_u16(uint16x8_t a) {
  return vgetq_lane_u16(__builtin_mpl_vector_minvq_v8u16(a), 0);
}

extern inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__)) vminv_u32(uint32x2_t a) {
  return vget_lane_u32(__builtin_mpl_vector_minv_v2u32(a), 0);
}

extern inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__)) vminvq_u32(uint32x4_t a) {
  return vgetq_lane_u32(__builtin_mpl_vector_minvq_v4u32(a), 0);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtst_s8(int8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_tst_v8i8(a, b);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vtstq_s8(int8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_tstq_v16i8(a, b);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vtst_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_tst_v4i16(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtstq_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_tstq_v8i16(a, b);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vtst_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_tst_v2i32(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vtstq_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_tstq_v4i32(a, b);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtst_u8(uint8x8_t a, uint8x8_t b) {
  return __builtin_mpl_vector_tst_v8u8(a, b);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vtstq_u8(uint8x16_t a, uint8x16_t b) {
  return __builtin_mpl_vector_tstq_v16u8(a, b);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vtst_u16(uint16x4_t a, uint16x4_t b) {
  return __builtin_mpl_vector_tst_v4u16(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtstq_u16(uint16x8_t a, uint16x8_t b) {
  return __builtin_mpl_vector_tstq_v8u16(a, b);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vtst_u32(uint32x2_t a, uint32x2_t b) {
  return __builtin_mpl_vector_tst_v2u32(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vtstq_u32(uint32x4_t a, uint32x4_t b) {
  return __builtin_mpl_vector_tstq_v4u32(a, b);
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vtst_s64(int64x1_t a, int64x1_t b) {
  return __builtin_mpl_vector_tst_v1i64(a, b);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vtstq_s64(int64x2_t a, int64x2_t b) {
  return __builtin_mpl_vector_tstq_v2i64(a, b);
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vtst_u64(uint64x1_t a, uint64x1_t b) {
  return __builtin_mpl_vector_tst_v1u64(a, b);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vtstq_u64(uint64x2_t a, uint64x2_t b) {
  return __builtin_mpl_vector_tstq_v2u64(a, b);
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vtstd_s64(int64_t a, int64_t b) {
  return (a & b) ? -1ll : 0ll;
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vtstd_u64(uint64_t a, uint64_t b) {
  return (a & b) ? -1ll : 0ll;
}

extern inline int8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqmovnh_s16(int16_t a) {
  return vget_lane_s8(__builtin_mpl_vector_qmovnh_i16(__builtin_mpl_vector_from_scalar_v4i16(a)), 0);
}

extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqmovns_s32(int32_t a) {
  return vget_lane_s16(__builtin_mpl_vector_qmovns_i32(__builtin_mpl_vector_from_scalar_v2i32(a)), 0);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqmovnd_s64(int64_t a) {
  return vget_lane_s32(__builtin_mpl_vector_qmovnd_i64(__builtin_mpl_vector_from_scalar_v1i64(a)), 0);
}

extern inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqmovnh_u16(uint16_t a) {
  return vget_lane_u8(__builtin_mpl_vector_qmovnh_u16(__builtin_mpl_vector_from_scalar_v4u16(a)), 0);
}

extern inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqmovns_u32(uint32_t a) {
  return vget_lane_u16(__builtin_mpl_vector_qmovns_u32(__builtin_mpl_vector_from_scalar_v2u32(a)), 0);
}

extern inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqmovnd_u64(uint64_t a) {
  return vget_lane_u32(__builtin_mpl_vector_qmovnd_u64(__builtin_mpl_vector_from_scalar_v1u64(a)), 0);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqmovn_high_s16(int8x8_t r, int16x8_t a) {
  return __builtin_mpl_vector_qmovn_high_v16i8(r, a);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqmovn_high_s32(int16x4_t r, int32x4_t a) {
  return __builtin_mpl_vector_qmovn_high_v8i16(r, a);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqmovn_high_s64(int32x2_t r, int64x2_t a) {
  return __builtin_mpl_vector_qmovn_high_v4i32(r, a);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqmovn_high_u16(uint8x8_t r,
                                                                                             uint16x8_t a) {
  return __builtin_mpl_vector_qmovn_high_v16u8(r, a);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqmovn_high_u32(uint16x4_t r,
                                                                                             uint32x4_t a) {
  return __builtin_mpl_vector_qmovn_high_v8u16(r, a);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqmovn_high_u64(uint32x2_t r,
                                                                                             uint64x2_t a) {
  return __builtin_mpl_vector_qmovn_high_v4u32(r, a);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqmovun_s16(int16x8_t a) {
  return __builtin_mpl_vector_qmovun_v8u8(a);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqmovun_s32(int32x4_t a) {
  return __builtin_mpl_vector_qmovun_v4u16(a);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqmovun_s64(int64x2_t a) {
  return __builtin_mpl_vector_qmovun_v2u32(a);
}

extern inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqmovunh_s16(int16_t a) {
  return vget_lane_s8(__builtin_mpl_vector_qmovunh_i16(__builtin_mpl_vector_from_scalar_v4i16(a)), 0);
}

extern inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqmovuns_s32(int32_t a) {
  return vget_lane_s16(__builtin_mpl_vector_qmovuns_i32(__builtin_mpl_vector_from_scalar_v2i32(a)), 0);
}

extern inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqmovund_s64(int64_t a) {
  return vget_lane_s32(__builtin_mpl_vector_qmovund_i64(__builtin_mpl_vector_from_scalar_v1i64(a)), 0);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqmovun_high_s16(uint8x8_t r,
                                                                                              int16x8_t a) {
  return __builtin_mpl_vector_qmovun_high_v16u8(r, a);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqmovun_high_s32(uint16x4_t r,
                                                                                              int32x4_t a) {
  return __builtin_mpl_vector_qmovun_high_v8u16(r, a);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqmovun_high_s64(uint32x2_t r,
                                                                                              int64x2_t a) {
  return __builtin_mpl_vector_qmovun_high_v4u32(r, a);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmul_n_s16(int16x4_t a, int16_t b) {
  return a * (int16x4_t){b, b, b, b};
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmulq_n_s16(int16x8_t a, int16_t b) {
  return a * (int16x8_t){b, b, b, b, b, b, b, b};
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmul_n_s32(int32x2_t a, int32_t b) {
  return a * (int32x2_t){b, b};
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmulq_n_s32(int32x4_t a, int32_t b) {
  return a * (int32x4_t){b, b, b, b};
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmul_n_u16(uint16x4_t a, uint16_t b) {
  return a * (uint16x4_t){b, b, b, b};
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmulq_n_u16(uint16x8_t a, uint16_t b) {
  return a * (uint16x8_t){b, b, b, b, b, b, b, b};
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmul_n_u32(uint32x2_t a, uint32_t b) {
  return a * (uint32x2_t){b, b};
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmulq_n_u32(uint32x4_t a, uint32_t b) {
  return a * (uint32x4_t){b, b, b, b};
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmul_lane_s16(int16x4_t a, int16x4_t v,
                                                                                          const int lane) {
  return __builtin_mpl_vector_mul_lane_v4i16(a, v, lane);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmulq_lane_s16(int16x8_t a, int16x4_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_mulq_lane_v8i16(a, v, lane);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmul_lane_s32(int32x2_t a, int32x2_t v,
                                                                                          const int lane) {
  return __builtin_mpl_vector_mul_lane_v2i32(a, v, lane);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmulq_lane_s32(int32x4_t a, int32x2_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_mulq_lane_v4i32(a, v, lane);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmul_lane_u16(uint16x4_t a, uint16x4_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_mul_lane_v4u16(a, v, lane);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmulq_lane_u16(uint16x8_t a, uint16x4_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mulq_lane_v8u16(a, v, lane);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmul_lane_u32(uint32x2_t a, uint32x2_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_mul_lane_v2u32(a, v, lane);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmulq_lane_u32(uint32x4_t a, uint32x2_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mulq_lane_v4u32(a, v, lane);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmul_laneq_s16(int16x4_t a, int16x8_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_mul_laneq_v4i16(a, v, lane);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmulq_laneq_s16(int16x8_t a, int16x8_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mulq_laneq_v8i16(a, v, lane);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmul_laneq_s32(int32x2_t a, int32x4_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_mul_laneq_v2i32(a, v, lane);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmulq_laneq_s32(int32x4_t a, int32x4_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mulq_laneq_v4i32(a, v, lane);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmul_laneq_u16(uint16x4_t a, uint16x8_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mul_laneq_v4u16(a, v, lane);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmulq_laneq_u16(uint16x8_t a, uint16x8_t v,
                                                                                             const int lane) {
  return __builtin_mpl_vector_mulq_laneq_v8u16(a, v, lane);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmul_laneq_u32(uint32x2_t a, uint32x4_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mul_laneq_v2u32(a, v, lane);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmulq_laneq_u32(uint32x4_t a, uint32x4_t v,
                                                                                             const int lane) {
  return __builtin_mpl_vector_mulq_laneq_v4u32(a, v, lane);
}

int32x4_t __builtin_mpl_vector_mull_n_v4i32(int16x4_t a, int16_t b);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_n_s16(int16x4_t a, int16_t b) {
  return vmull_s16(a, ((int16x4_t){b, b, b, b}));
}

int64x2_t __builtin_mpl_vector_mull_n_v2i64(int32x2_t a, int32_t b);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_n_s32(int32x2_t a, int32_t b) {
  return vmull_s32(a, ((int32x2_t){b, b}));
}

uint32x4_t __builtin_mpl_vector_mull_n_v4u32(uint16x4_t a, uint16_t b);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_n_u16(uint16x4_t a, uint16_t b) {
  return vmull_u16(a, ((uint16x4_t){b, b, b, b}));
}

uint64x2_t __builtin_mpl_vector_mull_n_v2u64(uint32x2_t a, uint32_t b);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_n_u32(uint32x2_t a, uint32_t b) {
  return vmull_u32(a, ((uint32x2_t){b, b}));
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_high_n_s16(int16x8_t a, int16_t b) {
  return vmull_n_s16((vget_high_s16(a)), b);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_high_n_s32(int32x4_t a, int32_t b) {
  return vmull_n_s32((vget_high_s32(a)), b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_high_n_u16(uint16x8_t a,
                                                                                              uint16_t b) {
  return vmull_n_u16((vget_high_u16(a)), b);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_high_n_u32(uint32x4_t a,
                                                                                              uint32_t b) {
  return vmull_n_u32((vget_high_u32(a)), b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_lane_s16(int16x4_t a, int16x4_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_mull_lane_v4i32(a, v, lane);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_lane_s32(int32x2_t a, int32x2_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_mull_lane_v2i64(a, v, lane);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_lane_u16(uint16x4_t a, uint16x4_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mull_lane_v4u32(a, v, lane);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_lane_u32(uint32x2_t a, uint32x2_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mull_lane_v2u64(a, v, lane);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_high_lane_s16(int16x8_t a,
                                                                                                int16x4_t v,
                                                                                                const int lane) {
  return __builtin_mpl_vector_mull_high_lane_v4i32(a, v, lane);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_high_lane_s32(int32x4_t a,
                                                                                                int32x2_t v,
                                                                                                const int lane) {
  return __builtin_mpl_vector_mull_high_lane_v2i64(a, v, lane);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_high_lane_u16(uint16x8_t a,
                                                                                                 uint16x4_t v,
                                                                                                 const int lane) {
  return __builtin_mpl_vector_mull_high_lane_v4u32(a, v, lane);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_high_lane_u32(uint32x4_t a,
                                                                                                 uint32x2_t v,
                                                                                                 const int lane) {
  return __builtin_mpl_vector_mull_high_lane_v2u64(a, v, lane);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_laneq_s16(int16x4_t a, int16x8_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mull_laneq_v4i32(a, v, lane);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_laneq_s32(int32x2_t a, int32x4_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mull_laneq_v2i64(a, v, lane);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_laneq_u16(uint16x4_t a, uint16x8_t v,
                                                                                             const int lane) {
  return __builtin_mpl_vector_mull_laneq_v4u32(a, v, lane);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_laneq_u32(uint32x2_t a, uint32x4_t v,
                                                                                             const int lane) {
  return __builtin_mpl_vector_mull_laneq_v2u64(a, v, lane);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_high_laneq_s16(int16x8_t a,
                                                                                                 int16x8_t v,
                                                                                                 const int lane) {
  return __builtin_mpl_vector_mull_high_laneq_v4i32(a, v, lane);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_high_laneq_s32(int32x4_t a,
                                                                                                 int32x4_t v,
                                                                                                 const int lane) {
  return __builtin_mpl_vector_mull_high_laneq_v2i64(a, v, lane);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_high_laneq_u16(uint16x8_t a,
                                                                                                  uint16x8_t v,
                                                                                                  const int lane) {
  return __builtin_mpl_vector_mull_high_laneq_v4u32(a, v, lane);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmull_high_laneq_u32(uint32x4_t a,
                                                                                                  uint32x4_t v,
                                                                                                  const int lane) {
  return __builtin_mpl_vector_mull_high_laneq_v2u64(a, v, lane);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vneg_s8(int8x8_t a) {
  return __builtin_mpl_vector_neg_v8i8(a);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vnegq_s8(int8x16_t a) {
  return __builtin_mpl_vector_negq_v16i8(a);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vneg_s16(int16x4_t a) {
  return __builtin_mpl_vector_neg_v4i16(a);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vnegq_s16(int16x8_t a) {
  return __builtin_mpl_vector_negq_v8i16(a);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vneg_s32(int32x2_t a) {
  return __builtin_mpl_vector_neg_v2i32(a);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vnegq_s32(int32x4_t a) {
  return __builtin_mpl_vector_negq_v4i32(a);
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vneg_s64(int64x1_t a) {
  return __builtin_mpl_vector_neg_v1i64(a);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vnegq_s64(int64x2_t a) {
  return __builtin_mpl_vector_negq_v2i64(a);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmvn_s8(int8x8_t a) {
  return __builtin_mpl_vector_mvn_v8i8(a);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vmvnq_s8(int8x16_t a) {
  return __builtin_mpl_vector_mvnq_v16i8(a);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmvn_s16(int16x4_t a) {
  return (int8x8_t)__builtin_mpl_vector_mvn_v8i8((int8x8_t)a);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmvnq_s16(int16x8_t a) {
  return (int8x16_t)__builtin_mpl_vector_mvnq_v16i8((int8x16_t)a);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmvn_s32(int32x2_t a) {
  return (int8x8_t)__builtin_mpl_vector_mvn_v8i8((int8x8_t)a);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmvnq_s32(int32x4_t a) {
  return (int8x16_t)__builtin_mpl_vector_mvnq_v16i8((int8x16_t)a);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmvn_u8(uint8x8_t a) {
  return __builtin_mpl_vector_mvn_v8u8(a);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vmvnq_u8(uint8x16_t a) {
  return __builtin_mpl_vector_mvnq_v16u8(a);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmvn_u16(uint16x4_t a) {
  return (uint8x8_t)__builtin_mpl_vector_mvn_v8u8((uint8x8_t)a);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmvnq_u16(uint16x8_t a) {
  return (uint8x16_t)__builtin_mpl_vector_mvnq_v16u8((uint8x16_t)a);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmvn_u32(uint32x2_t a) {
  return (uint8x8_t)__builtin_mpl_vector_mvn_v8u8((uint8x8_t)a);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmvnq_u32(uint32x4_t a) {
  return (uint8x16_t)__builtin_mpl_vector_mvnq_v16u8((uint8x16_t)a);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vorn_s8(int8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_orn_v8i8(a, b);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vornq_s8(int8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_ornq_v16i8(a, b);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vorn_s16(int16x4_t a, int16x4_t b) {
  return (int8x8_t)__builtin_mpl_vector_orn_v8i8((int8x8_t)a, (int8x8_t)b);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vornq_s16(int16x8_t a, int16x8_t b) {
  return (int8x16_t)__builtin_mpl_vector_ornq_v16i8((int8x16_t)a, (int8x16_t)b);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vorn_s32(int32x2_t a, int32x2_t b) {
  return (int8x8_t)__builtin_mpl_vector_orn_v8i8((int8x8_t)a, (int8x8_t)b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vornq_s32(int32x4_t a, int32x4_t b) {
  return (int8x16_t)__builtin_mpl_vector_ornq_v16i8((int8x16_t)a, (int8x16_t)b);
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vorn_s64(int64x1_t a, int64x1_t b) {
  return (int8x8_t)__builtin_mpl_vector_orn_v8i8((int8x8_t)a, (int8x8_t)b);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vornq_s64(int64x2_t a, int64x2_t b) {
  return (int8x16_t)__builtin_mpl_vector_ornq_v16i8((int8x16_t)a, (int8x16_t)b);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vorn_u8(uint8x8_t a, uint8x8_t b) {
  return __builtin_mpl_vector_orn_v8u8(a, b);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vornq_u8(uint8x16_t a, uint8x16_t b) {
  return __builtin_mpl_vector_ornq_v16u8(a, b);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vorn_u16(uint16x4_t a, uint16x4_t b) {
  return (uint8x8_t)__builtin_mpl_vector_orn_v8u8((uint8x8_t)a, (uint8x8_t)b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vornq_u16(uint16x8_t a, uint16x8_t b) {
  return (uint8x16_t)__builtin_mpl_vector_ornq_v16u8((uint8x16_t)a, (uint8x16_t)b);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vorn_u32(uint32x2_t a, uint32x2_t b) {
  return (uint8x8_t)__builtin_mpl_vector_orn_v8u8((uint8x8_t)a, (uint8x8_t)b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vornq_u32(uint32x4_t a, uint32x4_t b) {
  return (uint8x16_t)__builtin_mpl_vector_ornq_v16u8((uint8x16_t)a, (uint8x16_t)b);
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vorn_u64(uint64x1_t a, uint64x1_t b) {
  return (uint8x8_t)__builtin_mpl_vector_orn_v8u8((uint8x8_t)a, (uint8x8_t)b);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vornq_u64(uint64x2_t a, uint64x2_t b) {
  return (uint8x16_t)__builtin_mpl_vector_ornq_v16u8((uint8x16_t)a, (uint8x16_t)b);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcls_s8(int8x8_t a) {
  return __builtin_mpl_vector_cls_v8i8(a);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vclsq_s8(int8x16_t a) {
  return __builtin_mpl_vector_clsq_v16i8(a);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcls_s16(int16x4_t a) {
  return __builtin_mpl_vector_cls_v4i16(a);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vclsq_s16(int16x8_t a) {
  return __builtin_mpl_vector_clsq_v8i16(a);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcls_s32(int32x2_t a) {
  return __builtin_mpl_vector_cls_v2i32(a);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vclsq_s32(int32x4_t a) {
  return __builtin_mpl_vector_clsq_v4i32(a);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcls_u8(uint8x8_t a) {
  return __builtin_mpl_vector_cls_v8u8(a);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vclsq_u8(uint8x16_t a) {
  return __builtin_mpl_vector_clsq_v16u8(a);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcls_u16(int16x4_t a) {
  return __builtin_mpl_vector_cls_v4u16(a);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vclsq_u16(uint16x8_t a) {
  return __builtin_mpl_vector_clsq_v8u16(a);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcls_u32(uint32x2_t a) {
  return __builtin_mpl_vector_cls_v2u32(a);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vclsq_u32(uint32x4_t a) {
  return __builtin_mpl_vector_clsq_v4u32(a);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vclz_s8(int8x8_t a) {
  return __builtin_mpl_vector_clz_v8i8(a);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vclzq_s8(int8x16_t a) {
  return __builtin_mpl_vector_clzq_v16i8(a);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vclz_s16(int16x4_t a) {
  return __builtin_mpl_vector_clz_v4i16(a);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vclzq_s16(int16x8_t a) {
  return __builtin_mpl_vector_clzq_v8i16(a);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vclz_s32(int32x2_t a) {
  return __builtin_mpl_vector_clz_v2i32(a);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vclzq_s32(int32x4_t a) {
  return __builtin_mpl_vector_clzq_v4i32(a);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vclz_u8(uint8x8_t a) {
  return __builtin_mpl_vector_clz_v8u8(a);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vclzq_u8(uint8x16_t a) {
  return __builtin_mpl_vector_clzq_v16u8(a);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vclz_u16(uint16x4_t a) {
  return __builtin_mpl_vector_clz_v4u16(a);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vclzq_u16(uint16x8_t a) {
  return __builtin_mpl_vector_clzq_v8u16(a);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vclz_u32(uint32x2_t a) {
  return __builtin_mpl_vector_clz_v2u32(a);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vclzq_u32(uint32x4_t a) {
  return __builtin_mpl_vector_clzq_v4u32(a);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcnt_s8(int8x8_t a) {
  return __builtin_mpl_vector_cnt_v8i8(a);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vcntq_s8(int8x16_t a) {
  return __builtin_mpl_vector_cntq_v16i8(a);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcnt_u8(uint8x8_t a) {
  return __builtin_mpl_vector_cnt_v8u8(a);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vcntq_u8(uint8x16_t a) {
  return __builtin_mpl_vector_cntq_v16u8(a);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vbic_s8(int8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_bic_v8i8(a, b);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vbicq_s8(int8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_bicq_v16i8(a, b);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vbic_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_bic_v4i16(a, b);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vbicq_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_bicq_v8i16(a, b);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vbic_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_bic_v2i32(a, b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vbicq_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_bicq_v4i32(a, b);
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vbic_s64(int64x1_t a, int64x1_t b) {
  return __builtin_mpl_vector_bic_v1i64(a, b);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vbicq_s64(int64x2_t a, int64x2_t b) {
  return __builtin_mpl_vector_bicq_v2i64(a, b);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vbic_u8(uint8x8_t a, uint8x8_t b) {
  return __builtin_mpl_vector_bic_v8u8(a, b);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vbicq_u8(uint8x16_t a, uint8x16_t b) {
  return __builtin_mpl_vector_bicq_v16u8(a, b);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vbic_u16(uint16x4_t a, uint16x4_t b) {
  return __builtin_mpl_vector_bic_v4u16(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vbicq_u16(uint16x8_t a, uint16x8_t b) {
  return __builtin_mpl_vector_bicq_v8u16(a, b);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vbic_u32(uint32x2_t a, uint32x2_t b) {
  return __builtin_mpl_vector_bic_v2u32(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vbicq_u32(uint32x4_t a, uint32x4_t b) {
  return __builtin_mpl_vector_bicq_v4u32(a, b);
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vbic_u64(uint64x1_t a, uint64x1_t b) {
  return __builtin_mpl_vector_bic_v1u64(a, b);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vbicq_u64(uint64x2_t a, uint64x2_t b) {
  return __builtin_mpl_vector_bicq_v2u64(a, b);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vbsl_s8(uint8x8_t a, int8x8_t b,
                                                                                   int8x8_t c) {
  return __builtin_mpl_vector_bsl_v8i8(a, b, c);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vbslq_s8(uint8x16_t a, int8x16_t b,
                                                                                     int8x16_t c) {
  return __builtin_mpl_vector_bslq_v16i8(a, b, c);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vbsl_s16(uint16x4_t a, int16x4_t b,
                                                                                     int16x4_t c) {
  return (int8x8_t)__builtin_mpl_vector_bsl_v8i8((int8x8_t)a, (int8x8_t)b, (int8x8_t)c);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vbslq_s16(uint16x8_t a, int16x8_t b,
                                                                                      int16x8_t c) {
  return (int8x16_t)__builtin_mpl_vector_bslq_v16i8((int8x16_t)a, (int8x16_t)b, (int8x16_t)c);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vbsl_s32(uint32x2_t a, int32x2_t b,
                                                                                     int32x2_t c) {
  return (int8x8_t)__builtin_mpl_vector_bsl_v8i8((int8x8_t)a, (int8x8_t)b, (int8x8_t)c);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vbslq_s32(uint32x4_t a, int32x4_t b,
                                                                                      int32x4_t c) {
  return (int8x16_t)__builtin_mpl_vector_bslq_v16i8((int8x16_t)a, (int8x16_t)b, (int8x16_t)c);
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vbsl_s64(uint64x1_t a, int64x1_t b,
                                                                                     int64x1_t c) {
  return (int8x8_t)__builtin_mpl_vector_bsl_v8i8((int8x8_t)a, (int8x8_t)b, (int8x8_t)c);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vbslq_s64(uint64x2_t a, int64x2_t b,
                                                                                      int64x2_t c) {
  return (int8x16_t)__builtin_mpl_vector_bslq_v16i8((int8x16_t)a, (int8x16_t)b, (int8x16_t)c);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vbsl_u8(uint8x8_t a, uint8x8_t b,
                                                                                    uint8x8_t c) {
  return __builtin_mpl_vector_bsl_v8u8(a, b, c);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vbslq_u8(uint8x16_t a, uint8x16_t b,
                                                                                      uint8x16_t c) {
  return __builtin_mpl_vector_bslq_v16u8(a, b, c);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vbsl_u16(uint16x4_t a, uint16x4_t b,
                                                                                      uint16x4_t c) {
  return (uint8x8_t)__builtin_mpl_vector_bsl_v8u8((uint8x8_t)a, (uint8x8_t)b, (uint8x8_t)c);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vbslq_u16(uint16x8_t a, uint16x8_t b,
                                                                                       uint16x8_t c) {
  return (uint8x16_t)__builtin_mpl_vector_bslq_v16u8((uint8x16_t)a, (uint8x16_t)b, (uint8x16_t)c);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vbsl_u32(uint32x2_t a, uint32x2_t b,
                                                                                      uint32x2_t c) {
  return (uint8x8_t)__builtin_mpl_vector_bsl_v8u8((uint8x8_t)a, (uint8x8_t)b, (uint8x8_t)c);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vbslq_u32(uint32x4_t a, uint32x4_t b,
                                                                                       uint32x4_t c) {
  return (uint8x16_t)__builtin_mpl_vector_bslq_v16u8((uint8x16_t)a, (uint8x16_t)b, (uint8x16_t)c);
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vbsl_u64(uint64x1_t a, uint64x1_t b,
                                                                                      uint64x1_t c) {
  return (uint8x8_t)__builtin_mpl_vector_bsl_v8u8((uint8x8_t)a, (uint8x8_t)b, (uint8x8_t)c);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vbslq_u64(uint64x2_t a, uint64x2_t b,
                                                                                       uint64x2_t c) {
  return (uint8x16_t)__builtin_mpl_vector_bslq_v16u8((uint8x16_t)a, (uint8x16_t)b, (uint8x16_t)c);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopy_lane_s8(int8x8_t a, const int lane1,
                                                                                         int8x8_t b, const int lane2) {
  return __builtin_mpl_vector_copy_lane_v8i8(a, lane1, b, lane2);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopyq_lane_s8(int8x16_t a, const int lane1,
                                                                                           int8x8_t b,
                                                                                           const int lane2) {
  return __builtin_mpl_vector_copyq_lane_v16i8(a, lane1, b, lane2);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopy_lane_s16(int16x4_t a, const int lane1,
                                                                                           int16x4_t b,
                                                                                           const int lane2) {
  return __builtin_mpl_vector_copy_lane_v4i16(a, lane1, b, lane2);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopyq_lane_s16(int16x8_t a,
                                                                                            const int lane1,
                                                                                            int16x4_t b,
                                                                                            const int lane2) {
  return __builtin_mpl_vector_copyq_lane_v8i16(a, lane1, b, lane2);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopy_lane_s32(int32x2_t a, const int lane1,
                                                                                           int32x2_t b,
                                                                                           const int lane2) {
  return __builtin_mpl_vector_copy_lane_v2i32(a, lane1, b, lane2);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopyq_lane_s32(int32x4_t a,
                                                                                            const int lane1,
                                                                                            int32x2_t b,
                                                                                            const int lane2) {
  return __builtin_mpl_vector_copyq_lane_v4i32(a, lane1, b, lane2);
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopy_lane_s64(int64x1_t a, const int lane1,
                                                                                           int64x1_t b,
                                                                                           const int lane2) {
  return __builtin_mpl_vector_copy_lane_v1i64(a, lane1, b, lane2);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopyq_lane_s64(int64x2_t a,
                                                                                            const int lane1,
                                                                                            int64x1_t b,
                                                                                            const int lane2) {
  return __builtin_mpl_vector_copyq_lane_v2i64(a, lane1, b, lane2);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopy_lane_u8(uint8x8_t a, const int lane1,
                                                                                          uint8x8_t b,
                                                                                          const int lane2) {
  return __builtin_mpl_vector_copy_lane_v8u8(a, lane1, b, lane2);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopyq_lane_u8(uint8x16_t a,
                                                                                            const int lane1,
                                                                                            uint8x8_t b,
                                                                                            const int lane2) {
  return __builtin_mpl_vector_copyq_lane_v16u8(a, lane1, b, lane2);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopy_lane_u16(uint16x4_t a,
                                                                                            const int lane1,
                                                                                            uint16x4_t b,
                                                                                            const int lane2) {
  return __builtin_mpl_vector_copy_lane_v4u16(a, lane1, b, lane2);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopyq_lane_u16(uint16x8_t a,
                                                                                             const int lane1,
                                                                                             uint16x4_t b,
                                                                                             const int lane2) {
  return __builtin_mpl_vector_copyq_lane_v8u16(a, lane1, b, lane2);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopy_lane_u32(uint32x2_t a,
                                                                                            const int lane1,
                                                                                            uint32x2_t b,
                                                                                            const int lane2) {
  return __builtin_mpl_vector_copy_lane_v2u32(a, lane1, b, lane2);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopyq_lane_u32(uint32x4_t a,
                                                                                             const int lane1,
                                                                                             uint32x2_t b,
                                                                                             const int lane2) {
  return __builtin_mpl_vector_copyq_lane_v4u32(a, lane1, b, lane2);
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopy_lane_u64(uint64x1_t a,
                                                                                            const int lane1,
                                                                                            uint64x1_t b,
                                                                                            const int lane2) {
  return __builtin_mpl_vector_copy_lane_v1u64(a, lane1, b, lane2);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopyq_lane_u64(uint64x2_t a,
                                                                                             const int lane1,
                                                                                             uint64x1_t b,
                                                                                             const int lane2) {
  return __builtin_mpl_vector_copyq_lane_v2u64(a, lane1, b, lane2);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopy_laneq_s8(int8x8_t a, const int lane1,
                                                                                          int8x16_t b,
                                                                                          const int lane2) {
  return __builtin_mpl_vector_copy_laneq_v8i8(a, lane1, b, lane2);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopyq_laneq_s8(int8x16_t a,
                                                                                            const int lane1,
                                                                                            int8x16_t b,
                                                                                            const int lane2) {
  return __builtin_mpl_vector_copyq_laneq_v16i8(a, lane1, b, lane2);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopy_laneq_s16(int16x4_t a,
                                                                                            const int lane1,
                                                                                            int16x8_t b,
                                                                                            const int lane2) {
  return __builtin_mpl_vector_copy_laneq_v4i16(a, lane1, b, lane2);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopyq_laneq_s16(int16x8_t a,
                                                                                             const int lane1,
                                                                                             int16x8_t b,
                                                                                             const int lane2) {
  return __builtin_mpl_vector_copyq_laneq_v8i16(a, lane1, b, lane2);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopy_laneq_s32(int32x2_t a,
                                                                                            const int lane1,
                                                                                            int32x4_t b,
                                                                                            const int lane2) {
  return __builtin_mpl_vector_copy_laneq_v2i32(a, lane1, b, lane2);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopyq_laneq_s32(int32x4_t a,
                                                                                             const int lane1,
                                                                                             int32x4_t b,
                                                                                             const int lane2) {
  return __builtin_mpl_vector_copyq_laneq_v4i32(a, lane1, b, lane2);
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopy_laneq_s64(int64x1_t a,
                                                                                            const int lane1,
                                                                                            int64x2_t b,
                                                                                            const int lane2) {
  return __builtin_mpl_vector_copy_laneq_v1i64(a, lane1, b, lane2);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopyq_laneq_s64(int64x2_t a,
                                                                                             const int lane1,
                                                                                             int64x2_t b,
                                                                                             const int lane2) {
  return __builtin_mpl_vector_copyq_laneq_v2i64(a, lane1, b, lane2);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopy_laneq_u8(uint8x8_t a,
                                                                                           const int lane1,
                                                                                           uint8x16_t b,
                                                                                           const int lane2) {
  return __builtin_mpl_vector_copy_laneq_v8u8(a, lane1, b, lane2);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopyq_laneq_u8(uint8x16_t a,
                                                                                             const int lane1,
                                                                                             uint8x16_t b,
                                                                                             const int lane2) {
  return __builtin_mpl_vector_copyq_laneq_v16u8(a, lane1, b, lane2);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopy_laneq_u16(uint16x4_t a,
                                                                                             const int lane1,
                                                                                             uint16x8_t b,
                                                                                             const int lane2) {
  return __builtin_mpl_vector_copy_laneq_v4u16(a, lane1, b, lane2);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopyq_laneq_u16(uint16x8_t a,
                                                                                              const int lane1,
                                                                                              uint16x8_t b,
                                                                                              const int lane2) {
  return __builtin_mpl_vector_copyq_laneq_v8u16(a, lane1, b, lane2);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopy_laneq_u32(uint32x2_t a,
                                                                                             const int lane1,
                                                                                             uint32x4_t b,
                                                                                             const int lane2) {
  return __builtin_mpl_vector_copy_laneq_v2u32(a, lane1, b, lane2);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopyq_laneq_u32(uint32x4_t a,
                                                                                              const int lane1,
                                                                                              uint32x4_t b,
                                                                                              const int lane2) {
  return __builtin_mpl_vector_copyq_laneq_v4u32(a, lane1, b, lane2);
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopy_laneq_u64(uint64x1_t a,
                                                                                             const int lane1,
                                                                                             uint64x2_t b,
                                                                                             const int lane2) {
  return __builtin_mpl_vector_copy_laneq_v1u64(a, lane1, b, lane2);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcopyq_laneq_u64(uint64x2_t a,
                                                                                              const int lane1,
                                                                                              uint64x2_t b,
                                                                                              const int lane2) {
  return __builtin_mpl_vector_copyq_laneq_v2u64(a, lane1, b, lane2);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrbit_s8(int8x8_t a) {
  return __builtin_mpl_vector_rbit_v8i8(a);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vrbitq_s8(int8x16_t a) {
  return __builtin_mpl_vector_rbitq_v16i8(a);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrbit_u8(uint8x8_t a) {
  return __builtin_mpl_vector_rbit_v8u8(a);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vrbitq_u8(uint8x16_t a) {
  return __builtin_mpl_vector_rbitq_v16u8(a);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcreate_s8(uint64_t a) {
  return __builtin_mpl_vector_create_v8i8(a);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcreate_s16(uint64_t a) {
  return __builtin_mpl_vector_create_v4i16(a);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcreate_s32(uint64_t a) {
  return __builtin_mpl_vector_create_v2i32(a);
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vcreate_s64(uint64_t a) {
  return __builtin_mpl_vector_create_v1i64(a);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcreate_u8(uint64_t a) {
  return __builtin_mpl_vector_create_v8u8(a);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcreate_u16(uint64_t a) {
  return __builtin_mpl_vector_create_v4u16(a);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcreate_u32(uint64_t a) {
  return __builtin_mpl_vector_create_v2u32(a);
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vcreate_u64(uint64_t a) {
  return __builtin_mpl_vector_create_v1u64(a);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmov_n_s8(int8_t value) {
  return __builtin_mpl_vector_mov_n_v8i8(value);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovq_n_s8(int8_t value) {
  return __builtin_mpl_vector_movq_n_v16i8(value);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmov_n_s16(int16_t value) {
  return __builtin_mpl_vector_mov_n_v4i16(value);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovq_n_s16(int16_t value) {
  return __builtin_mpl_vector_movq_n_v8i16(value);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmov_n_s32(int32_t value) {
  return __builtin_mpl_vector_mov_n_v2i32(value);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovq_n_s32(int32_t value) {
  return __builtin_mpl_vector_movq_n_v4i32(value);
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vmov_n_s64(int64_t value) {
  return __builtin_mpl_vector_mov_n_v1i64(value);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovq_n_s64(int64_t value) {
  return __builtin_mpl_vector_movq_n_v2i64(value);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmov_n_u8(uint8_t value) {
  return __builtin_mpl_vector_mov_n_v8u8(value);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovq_n_u8(uint8_t value) {
  return __builtin_mpl_vector_movq_n_v16u8(value);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmov_n_u16(uint16_t value) {
  return __builtin_mpl_vector_mov_n_v4u16(value);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovq_n_u16(uint16_t value) {
  return __builtin_mpl_vector_movq_n_v8u16(value);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmov_n_u32(uint32_t value) {
  return __builtin_mpl_vector_mov_n_v2u32(value);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovq_n_u32(uint32_t value) {
  return __builtin_mpl_vector_movq_n_v4u32(value);
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vmov_n_u64(uint64_t value) {
  return __builtin_mpl_vector_mov_n_v1u64(value);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmovq_n_u64(uint64_t value) {
  return __builtin_mpl_vector_movq_n_v2u64(value);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vdup_lane_s8(int8x8_t vec,
                                                                                        const int lane) {
  return __builtin_mpl_vector_dup_lane_v8i8(vec, lane);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupq_lane_s8(int8x8_t vec,
                                                                                          const int lane) {
  return __builtin_mpl_vector_dupq_lane_v16i8(vec, lane);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vdup_lane_s16(int16x4_t vec,
                                                                                          const int lane) {
  return __builtin_mpl_vector_dup_lane_v4i16(vec, lane);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupq_lane_s16(int16x4_t vec,
                                                                                           const int lane) {
  return __builtin_mpl_vector_dupq_lane_v8i16(vec, lane);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vdup_lane_s32(int32x2_t vec,
                                                                                          const int lane) {
  return __builtin_mpl_vector_dup_lane_v2i32(vec, lane);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupq_lane_s32(int32x2_t vec,
                                                                                           const int lane) {
  return __builtin_mpl_vector_dupq_lane_v4i32(vec, lane);
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vdup_lane_s64(int64x1_t vec,
                                                                                          const int lane) {
  return __builtin_mpl_vector_dup_lane_v1i64(vec, lane);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupq_lane_s64(int64x1_t vec,
                                                                                           const int lane) {
  return __builtin_mpl_vector_dupq_lane_v2i64(vec, lane);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vdup_lane_u8(uint8x8_t vec,
                                                                                         const int lane) {
  return __builtin_mpl_vector_dup_lane_v8u8(vec, lane);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupq_lane_u8(uint8x8_t vec,
                                                                                           const int lane) {
  return __builtin_mpl_vector_dupq_lane_v16u8(vec, lane);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vdup_lane_u16(uint16x4_t vec,
                                                                                           const int lane) {
  return __builtin_mpl_vector_dup_lane_v4u16(vec, lane);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupq_lane_u16(uint16x4_t vec,
                                                                                            const int lane) {
  return __builtin_mpl_vector_dupq_lane_v8u16(vec, lane);
}

uint32x2_t __builtin_mpl_vector_dup_lane_v2u32(uint32x2_t vec, const int lane);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vdup_lane_u32(uint32x2_t vec,
                                                                                           const int lane) {
  return __builtin_mpl_vector_dup_lane_v2u32(vec, lane);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupq_lane_u32(uint32x2_t vec,
                                                                                            const int lane) {
  return __builtin_mpl_vector_dupq_lane_v4u32(vec, lane);
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vdup_lane_u64(uint64x1_t vec,
                                                                                           const int lane) {
  return __builtin_mpl_vector_dup_lane_v1u64(vec, lane);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupq_lane_u64(uint64x1_t vec,
                                                                                            const int lane) {
  return __builtin_mpl_vector_dupq_lane_v2u64(vec, lane);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vdup_laneq_s8(int8x16_t vec,
                                                                                         const int lane) {
  return __builtin_mpl_vector_dup_laneq_v8i8(vec, lane);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupq_laneq_s8(int8x16_t vec,
                                                                                           const int lane) {
  return __builtin_mpl_vector_dupq_laneq_v16i8(vec, lane);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vdup_laneq_s16(int16x8_t vec,
                                                                                           const int lane) {
  return __builtin_mpl_vector_dup_laneq_v4i16(vec, lane);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupq_laneq_s16(int16x8_t vec,
                                                                                            const int lane) {
  return __builtin_mpl_vector_dupq_laneq_v8i16(vec, lane);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vdup_laneq_s32(int32x4_t vec,
                                                                                           const int lane) {
  return __builtin_mpl_vector_dup_laneq_v2i32(vec, lane);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupq_laneq_s32(int32x4_t vec,
                                                                                            const int lane) {
  return __builtin_mpl_vector_dupq_laneq_v4i32(vec, lane);
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vdup_laneq_s64(int64x2_t vec,
                                                                                           const int lane) {
  return __builtin_mpl_vector_dup_laneq_v1i64(vec, lane);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupq_laneq_s64(int64x2_t vec,
                                                                                            const int lane) {
  return __builtin_mpl_vector_dupq_laneq_v2i64(vec, lane);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vdup_laneq_u8(uint8x16_t vec,
                                                                                          const int lane) {
  return __builtin_mpl_vector_dup_laneq_v8u8(vec, lane);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupq_laneq_u8(uint8x16_t vec,
                                                                                            const int lane) {
  return __builtin_mpl_vector_dupq_laneq_v16u8(vec, lane);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vdup_laneq_u16(uint16x8_t vec,
                                                                                            const int lane) {
  return __builtin_mpl_vector_dup_laneq_v4u16(vec, lane);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupq_laneq_u16(uint16x8_t vec,
                                                                                             const int lane) {
  return __builtin_mpl_vector_dupq_laneq_v8u16(vec, lane);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vdup_laneq_u32(uint32x4_t vec,
                                                                                            const int lane) {
  return __builtin_mpl_vector_dup_laneq_v2u32(vec, lane);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupq_laneq_u32(uint32x4_t vec,
                                                                                             const int lane) {
  return __builtin_mpl_vector_dupq_laneq_v4u32(vec, lane);
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vdup_laneq_u64(uint64x2_t vec,
                                                                                            const int lane) {
  return __builtin_mpl_vector_dup_laneq_v1u64(vec, lane);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupq_laneq_u64(uint64x2_t vec,
                                                                                             const int lane) {
  return __builtin_mpl_vector_dupq_laneq_v2u64(vec, lane);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vcombine_s8(int8x8_t low, int8x8_t high) {
  return (int8x16_t)__builtin_mpl_vector_combine_v2i64((int64x1_t)low, high);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcombine_s16(int16x4_t low,
                                                                                         int16x4_t high) {
  return (int16x8_t)__builtin_mpl_vector_combine_v2i64((int64x1_t)low, high);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcombine_s32(int32x2_t low,
                                                                                         int32x2_t high) {
  return (int32x4_t)__builtin_mpl_vector_combine_v2i64((int64x1_t)low, high);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcombine_s64(int64x1_t low,
                                                                                         int64x1_t high) {
  return __builtin_mpl_vector_combine_v2i64(low, high);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vcombine_u8(uint8x8_t low,
                                                                                         uint8x8_t high) {
  return (uint8x16_t)__builtin_mpl_vector_combine_v2u64((uint64x1_t)low, high);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vcombine_u16(uint16x4_t low,
                                                                                          uint16x4_t high) {
  return (uint16x8_t)__builtin_mpl_vector_combine_v2u64((uint64x1_t)low, high);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vcombine_u32(uint32x2_t low,
                                                                                          uint32x2_t high) {
  return (uint32x4_t)__builtin_mpl_vector_combine_v2u64((uint64x1_t)low, high);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vcombine_u64(uint64x1_t low,
                                                                                          uint64x1_t high) {
  return __builtin_mpl_vector_combine_v2u64(low, high);
}

extern inline int8_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupb_lane_s8(int8x8_t vec, const int lane) {
  return __builtin_mpl_vector_dupb_lane_v8i8(vec, lane);
}

extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vduph_lane_s16(int16x4_t vec,
                                                                                         const int lane) {
  return __builtin_mpl_vector_duph_lane_v4i16(vec, lane);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vdups_lane_s32(int32x2_t vec,
                                                                                         const int lane) {
  return __builtin_mpl_vector_dups_lane_v2i32(vec, lane);
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupd_lane_s64(int64x1_t vec,
                                                                                         const int lane) {
  return __builtin_mpl_vector_dupd_lane_v1i64(vec, lane);
}

extern inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupb_lane_u8(uint8x8_t vec,
                                                                                        const int lane) {
  return __builtin_mpl_vector_dupb_lane_v8u8(vec, lane);
}

extern inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__)) vduph_lane_u16(uint16x4_t vec,
                                                                                          const int lane) {
  return __builtin_mpl_vector_duph_lane_v4u16(vec, lane);
}

extern inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__)) vdups_lane_u32(uint32x2_t vec,
                                                                                          const int lane) {
  return __builtin_mpl_vector_dups_lane_v2u32(vec, lane);
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupd_lane_u64(uint64x1_t vec,
                                                                                          const int lane) {
  return __builtin_mpl_vector_dupd_lane_v1u64(vec, lane);
}

extern inline int8_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupb_laneq_s8(int8x16_t vec,
                                                                                        const int lane) {
  return __builtin_mpl_vector_dupb_laneq_v16i8(vec, lane);
}

extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vduph_laneq_s16(int16x8_t vec,
                                                                                          const int lane) {
  return __builtin_mpl_vector_duph_laneq_v8i16(vec, lane);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vdups_laneq_s32(int32x4_t vec,
                                                                                          const int lane) {
  return __builtin_mpl_vector_dups_laneq_v4i32(vec, lane);
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupd_laneq_s64(int64x2_t vec,
                                                                                          const int lane) {
  return __builtin_mpl_vector_dupd_laneq_v2i64(vec, lane);
}

extern inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupb_laneq_u8(uint8x16_t vec,
                                                                                         const int lane) {
  return __builtin_mpl_vector_dupb_laneq_v16u8(vec, lane);
}

extern inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__)) vduph_laneq_u16(uint16x8_t vec,
                                                                                           const int lane) {
  return __builtin_mpl_vector_duph_laneq_v8u16(vec, lane);
}

extern inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__)) vdups_laneq_u32(uint32x4_t vec,
                                                                                           const int lane) {
  return __builtin_mpl_vector_dups_laneq_v4u32(vec, lane);
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vdupd_laneq_u64(uint64x2_t vec,
                                                                                           const int lane) {
  return __builtin_mpl_vector_dupd_laneq_v2u64(vec, lane);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrev64_s8(int8x8_t vec) {
  return __builtin_mpl_vector_rev64_v8i8(vec);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vrev64q_s8(int8x16_t vec) {
  return __builtin_mpl_vector_rev64q_v16i8(vec);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrev64_s16(int16x4_t vec) {
  return __builtin_mpl_vector_rev64_v4i16(vec);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrev64q_s16(int16x8_t vec) {
  return __builtin_mpl_vector_rev64q_v8i16(vec);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vrev64_s32(int32x2_t vec) {
  return __builtin_mpl_vector_rev64_v2i32(vec);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrev64q_s32(int32x4_t vec) {
  return __builtin_mpl_vector_rev64q_v4i32(vec);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrev64_u8(uint8x8_t vec) {
  return __builtin_mpl_vector_rev64_v8u8(vec);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vrev64q_u8(uint8x16_t vec) {
  return __builtin_mpl_vector_rev64q_v16u8(vec);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrev64_u16(uint16x4_t vec) {
  return __builtin_mpl_vector_rev64_v4u16(vec);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrev64q_u16(uint16x8_t vec) {
  return __builtin_mpl_vector_rev64q_v8u16(vec);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vrev64_u32(uint32x2_t vec) {
  return __builtin_mpl_vector_rev64_v2u32(vec);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrev64q_u32(uint32x4_t vec) {
  return __builtin_mpl_vector_rev64q_v4u32(vec);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrev16_s8(int8x8_t vec) {
  return __builtin_mpl_vector_rev16_v8i8(vec);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vrev16q_s8(int8x16_t vec) {
  return __builtin_mpl_vector_rev16q_v16i8(vec);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrev16_u8(uint8x8_t vec) {
  return __builtin_mpl_vector_rev16_v8u8(vec);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vrev16q_u8(uint8x16_t vec) {
  return __builtin_mpl_vector_rev16q_v16u8(vec);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip1_s8(int8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_zip1_v8i8(a, b);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip1q_s8(int8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_zip1q_v16i8(a, b);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip1_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_zip1_v4i16(a, b);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip1q_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_zip1q_v8i16(a, b);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip1_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_zip1_v2i32(a, b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip1q_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_zip1q_v4i32(a, b);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip1q_s64(int64x2_t a, int64x2_t b) {
  return __builtin_mpl_vector_zip1q_v2i64(a, b);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip1_u8(uint8x8_t a, uint8x8_t b) {
  return __builtin_mpl_vector_zip1_v8u8(a, b);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip1q_u8(uint8x16_t a, uint8x16_t b) {
  return __builtin_mpl_vector_zip1q_v16u8(a, b);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip1_u16(uint16x4_t a, uint16x4_t b) {
  return __builtin_mpl_vector_zip1_v4u16(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip1q_u16(uint16x8_t a, uint16x8_t b) {
  return __builtin_mpl_vector_zip1q_v8u16(a, b);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip1_u32(uint32x2_t a, uint32x2_t b) {
  return __builtin_mpl_vector_zip1_v2u32(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip1q_u32(uint32x4_t a, uint32x4_t b) {
  return __builtin_mpl_vector_zip1q_v4u32(a, b);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip1q_u64(uint64x2_t a, uint64x2_t b) {
  return __builtin_mpl_vector_zip1q_v2u64(a, b);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip2_s8(int8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_zip2_v8i8(a, b);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip2q_s8(int8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_zip2q_v16i8(a, b);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip2_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_zip2_v4i16(a, b);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip2q_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_zip2q_v8i16(a, b);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip2_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_zip2_v2i32(a, b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip2q_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_zip2q_v4i32(a, b);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip2q_s64(int64x2_t a, int64x2_t b) {
  return __builtin_mpl_vector_zip2q_v2i64(a, b);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip2_u8(uint8x8_t a, uint8x8_t b) {
  return __builtin_mpl_vector_zip2_v8u8(a, b);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip2q_u8(uint8x16_t a, uint8x16_t b) {
  return __builtin_mpl_vector_zip2q_v16u8(a, b);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip2_u16(uint16x4_t a, uint16x4_t b) {
  return __builtin_mpl_vector_zip2_v4u16(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip2q_u16(uint16x8_t a, uint16x8_t b) {
  return __builtin_mpl_vector_zip2q_v8u16(a, b);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip2_u32(uint32x2_t a, uint32x2_t b) {
  return __builtin_mpl_vector_zip2_v2u32(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip2q_u32(uint32x4_t a, uint32x4_t b) {
  return __builtin_mpl_vector_zip2q_v4u32(a, b);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip2q_u64(uint64x2_t a, uint64x2_t b) {
  return __builtin_mpl_vector_zip2q_v2u64(a, b);
}

extern inline int8x8x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip_s8(int8x8_t a, int8x8_t b) {
  return (int8x8x2_t){vzip1_s8((a), (b)), vzip2_s8((a), (b))};
}

extern inline int16x4x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip_s16(int16x4_t a, int16x4_t b) {
  return (int16x4x2_t){vzip1_s16((a), (b)), vzip2_s16((a), (b))};
}

extern inline uint8x8x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip_u8(uint8x8_t a, uint8x8_t b) {
  return (uint8x8x2_t){vzip1_u8((a), (b)), vzip2_u8((a), (b))};
}

extern inline uint16x4x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip_u16(uint16x4_t a, uint16x4_t b) {
  return (uint16x4x2_t){vzip1_u16((a), (b)), vzip2_u16((a), (b))};
}

extern inline int32x2x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip_s32(int32x2_t a, int32x2_t b) {
  return (int32x2x2_t){vzip1_s32((a), (b)), vzip2_s32((a), (b))};
}

extern inline uint32x2x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vzip_u32(uint32x2_t a, uint32x2_t b) {
  return (uint32x2x2_t){vzip1_u32((a), (b)), vzip2_u32((a), (b))};
}

extern inline int8x16x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vzipq_s8(int8x16_t a, int8x16_t b) {
  return (int8x16x2_t){vzip1q_s8((a), (b)), vzip2q_s8((a), (b))};
}

extern inline int16x8x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vzipq_s16(int16x8_t a, int16x8_t b) {
  return (int16x8x2_t){vzip1q_s16((a), (b)), vzip2q_s16((a), (b))};
}

extern inline int32x4x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vzipq_s32(int32x4_t a, int32x4_t b) {
  return (int32x4x2_t){vzip1q_s32((a), (b)), vzip2q_s32((a), (b))};
}

extern inline uint8x16x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vzipq_u8(uint8x16_t a, uint8x16_t b) {
  return (uint8x16x2_t){vzip1q_u8((a), (b)), vzip2q_u8((a), (b))};
}

extern inline uint16x8x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vzipq_u16(uint16x8_t a, uint16x8_t b) {
  return (uint16x8x2_t){vzip1q_u16((a), (b)), vzip2q_u16((a), (b))};
}

extern inline uint32x4x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vzipq_u32(uint32x4_t a, uint32x4_t b) {
  return (uint32x4x2_t){vzip1q_u32((a), (b)), vzip2q_u32((a), (b))};
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp1_s8(int8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_uzp1_v8i8(a, b);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp1q_s8(int8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_uzp1q_v16i8(a, b);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp1_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_uzp1_v4i16(a, b);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp1q_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_uzp1q_v8i16(a, b);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp1_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_uzp1_v2i32(a, b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp1q_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_uzp1q_v4i32(a, b);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp1q_s64(int64x2_t a, int64x2_t b) {
  return __builtin_mpl_vector_uzp1q_v2i64(a, b);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp1_u8(uint8x8_t a, uint8x8_t b) {
  return __builtin_mpl_vector_uzp1_v8u8(a, b);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp1q_u8(uint8x16_t a, uint8x16_t b) {
  return __builtin_mpl_vector_uzp1q_v16u8(a, b);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp1_u16(uint16x4_t a, uint16x4_t b) {
  return __builtin_mpl_vector_uzp1_v4u16(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp1q_u16(uint16x8_t a, uint16x8_t b) {
  return __builtin_mpl_vector_uzp1q_v8u16(a, b);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp1_u32(uint32x2_t a, uint32x2_t b) {
  return __builtin_mpl_vector_uzp1_v2u32(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp1q_u32(uint32x4_t a, uint32x4_t b) {
  return __builtin_mpl_vector_uzp1q_v4u32(a, b);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp1q_u64(uint64x2_t a, uint64x2_t b) {
  return __builtin_mpl_vector_uzp1q_v2u64(a, b);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp2_s8(int8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_uzp2_v8i8(a, b);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp2q_s8(int8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_uzp2q_v16i8(a, b);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp2_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_uzp2_v4i16(a, b);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp2q_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_uzp2q_v8i16(a, b);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp2_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_uzp2_v2i32(a, b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp2q_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_uzp2q_v4i32(a, b);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp2q_s64(int64x2_t a, int64x2_t b) {
  return __builtin_mpl_vector_uzp2q_v2i64(a, b);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp2_u8(uint8x8_t a, uint8x8_t b) {
  return __builtin_mpl_vector_uzp2_v8u8(a, b);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp2q_u8(uint8x16_t a, uint8x16_t b) {
  return __builtin_mpl_vector_uzp2q_v16u8(a, b);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp2_u16(uint16x4_t a, uint16x4_t b) {
  return __builtin_mpl_vector_uzp2_v4u16(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp2q_u16(uint16x8_t a, uint16x8_t b) {
  return __builtin_mpl_vector_uzp2q_v8u16(a, b);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp2_u32(uint32x2_t a, uint32x2_t b) {
  return __builtin_mpl_vector_uzp2_v2u32(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp2q_u32(uint32x4_t a, uint32x4_t b) {
  return __builtin_mpl_vector_uzp2q_v4u32(a, b);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp2q_u64(uint64x2_t a, uint64x2_t b) {
  return __builtin_mpl_vector_uzp2q_v2u64(a, b);
}

extern inline int8x8x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp_s8(int8x8_t a, int8x8_t b) {
  return (int8x8x2_t){vuzp1_s8((a), (b)), vuzp2_s8((a), (b))};
}

extern inline int16x4x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp_s16(int16x4_t a, int16x4_t b) {
  return (int16x4x2_t){vuzp1_s16((a), (b)), vuzp2_s16((a), (b))};
}

extern inline int32x2x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp_s32(int32x2_t a, int32x2_t b) {
  return (int32x2x2_t){vuzp1_s32((a), (b)), vuzp2_s32((a), (b))};
}

extern inline uint8x8x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp_u8(uint8x8_t a, uint8x8_t b) {
  return (uint8x8x2_t){vuzp1_u8((a), (b)), vuzp2_u8((a), (b))};
}

extern inline uint16x4x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp_u16(uint16x4_t a, uint16x4_t b) {
  return (uint16x4x2_t){vuzp1_u16((a), (b)), vuzp2_u16((a), (b))};
}

extern inline uint32x2x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzp_u32(uint32x2_t a, uint32x2_t b) {
  return (uint32x2x2_t){vuzp1_u32((a), (b)), vuzp2_u32((a), (b))};
}

extern inline int8x16x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzpq_s8(int8x16_t a, int8x16_t b) {
  return (int8x16x2_t){vuzp1q_s8((a), (b)), vuzp2q_s8((a), (b))};
}

extern inline int16x8x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzpq_s16(int16x8_t a, int16x8_t b) {
  return (int16x8x2_t){vuzp1q_s16((a), (b)), vuzp2q_s16((a), (b))};
}

extern inline int32x4x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzpq_s32(int32x4_t a, int32x4_t b) {
  return (int32x4x2_t){vuzp1q_s32((a), (b)), vuzp2q_s32((a), (b))};
}

extern inline uint8x16x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzpq_u8(uint8x16_t a, uint8x16_t b) {
  return (uint8x16x2_t){vuzp1q_u8((a), (b)), vuzp2q_u8((a), (b))};
}

extern inline uint16x8x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzpq_u16(uint16x8_t a, uint16x8_t b) {
  return (uint16x8x2_t){vuzp1q_u16((a), (b)), vuzp2q_u16((a), (b))};
}

extern inline uint32x4x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vuzpq_u32(uint32x4_t a, uint32x4_t b) {
  return (uint32x4x2_t){vuzp1q_u32((a), (b)), vuzp2q_u32((a), (b))};
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn1_s8(int8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_trn1_v8i8(a, b);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn1q_s8(int8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_trn1q_v16i8(a, b);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn1_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_trn1_v4i16(a, b);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn1q_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_trn1q_v8i16(a, b);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn1_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_trn1_v2i32(a, b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn1q_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_trn1q_v4i32(a, b);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn1q_s64(int64x2_t a, int64x2_t b) {
  return __builtin_mpl_vector_trn1q_v2i64(a, b);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn1_u8(uint8x8_t a, uint8x8_t b) {
  return __builtin_mpl_vector_trn1_v8u8(a, b);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn1q_u8(uint8x16_t a, uint8x16_t b) {
  return __builtin_mpl_vector_trn1q_v16u8(a, b);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn1_u16(uint16x4_t a, uint16x4_t b) {
  return __builtin_mpl_vector_trn1_v4u16(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn1q_u16(uint16x8_t a, uint16x8_t b) {
  return __builtin_mpl_vector_trn1q_v8u16(a, b);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn1_u32(uint32x2_t a, uint32x2_t b) {
  return __builtin_mpl_vector_trn1_v2u32(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn1q_u32(uint32x4_t a, uint32x4_t b) {
  return __builtin_mpl_vector_trn1q_v4u32(a, b);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn1q_u64(uint64x2_t a, uint64x2_t b) {
  return __builtin_mpl_vector_trn1q_v2u64(a, b);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn2_s8(int8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_trn2_v8i8(a, b);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn2q_s8(int8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_trn2q_v16i8(a, b);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn2_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_trn2_v4i16(a, b);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn2q_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_trn2q_v8i16(a, b);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn2_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_trn2_v2i32(a, b);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn2q_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_trn2q_v4i32(a, b);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn2q_s64(int64x2_t a, int64x2_t b) {
  return __builtin_mpl_vector_trn2q_v2i64(a, b);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn2_u8(uint8x8_t a, uint8x8_t b) {
  return __builtin_mpl_vector_trn2_v8u8(a, b);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn2q_u8(uint8x16_t a, uint8x16_t b) {
  return __builtin_mpl_vector_trn2q_v16u8(a, b);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn2_u16(uint16x4_t a, uint16x4_t b) {
  return __builtin_mpl_vector_trn2_v4u16(a, b);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn2q_u16(uint16x8_t a, uint16x8_t b) {
  return __builtin_mpl_vector_trn2q_v8u16(a, b);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn2_u32(uint32x2_t a, uint32x2_t b) {
  return __builtin_mpl_vector_trn2_v2u32(a, b);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn2q_u32(uint32x4_t a, uint32x4_t b) {
  return __builtin_mpl_vector_trn2q_v4u32(a, b);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn2q_u64(uint64x2_t a, uint64x2_t b) {
  return __builtin_mpl_vector_trn2q_v2u64(a, b);
}

extern inline int8x8x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn_s8(int8x8_t a, int8x8_t b) {
  return (int8x8x2_t){vtrn1_s8((a), (b)), vtrn2_s8((a), (b))};
}

extern inline int16x4x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn_s16(int16x4_t a, int16x4_t b) {
  return (int16x4x2_t){vtrn1_s16((a), (b)), vtrn2_s16((a), (b))};
}

extern inline uint8x8x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn_u8(uint8x8_t a, uint8x8_t b) {
  return (uint8x8x2_t){vtrn1_u8((a), (b)), vtrn2_u8((a), (b))};
}

extern inline uint16x4x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn_u16(uint16x4_t a, uint16x4_t b) {
  return (uint16x4x2_t){vtrn1_u16((a), (b)), vtrn2_u16((a), (b))};
}

extern inline int32x2x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn_s32(int32x2_t a, int32x2_t b) {
  return (int32x2x2_t){vtrn1_s32((a), (b)), vtrn2_s32((a), (b))};
}

extern inline uint32x2x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrn_u32(uint32x2_t a, uint32x2_t b) {
  return (uint32x2x2_t){vtrn1_u32((a), (b)), vtrn2_u32((a), (b))};
}

extern inline int8x16x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrnq_s8(int8x16_t a, int8x16_t b) {
  return (int8x16x2_t){vtrn1q_s8((a), (b)), vtrn2q_s8((a), (b))};
}

extern inline int16x8x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrnq_s16(int16x8_t a, int16x8_t b) {
  return (int16x8x2_t){vtrn1q_s16((a), (b)), vtrn2q_s16((a), (b))};
}

extern inline int32x4x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrnq_s32(int32x4_t a, int32x4_t b) {
  return (int32x4x2_t){vtrn1q_s32((a), (b)), vtrn2q_s32((a), (b))};
}

extern inline uint8x16x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrnq_u8(uint8x16_t a, uint8x16_t b) {
  return (uint8x16x2_t){vtrn1q_u8((a), (b)), vtrn2q_u8((a), (b))};
}

extern inline uint16x8x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrnq_u16(uint16x8_t a, uint16x8_t b) {
  return (uint16x8x2_t){vtrn1q_u16((a), (b)), vtrn2q_u16((a), (b))};
}

extern inline uint32x4x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vtrnq_u32(uint32x4_t a, uint32x4_t b) {
  return (uint32x4x2_t){vtrn1q_u32((a), (b)), vtrn2q_u32((a), (b))};
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1_s8(int8_t const *ptr) {
  return __builtin_mpl_vector_ld1_v8i8(ptr);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1q_s8(int8_t const *ptr) {
  return __builtin_mpl_vector_ld1q_v16i8(ptr);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1_s16(int16_t const *ptr) {
  return __builtin_mpl_vector_ld1_v4i16(ptr);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1q_s16(int16_t const *ptr) {
  return __builtin_mpl_vector_ld1q_v8i16(ptr);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1_s32(int32_t const *ptr) {
  return __builtin_mpl_vector_ld1_v2i32(ptr);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1q_s32(int32_t const *ptr) {
  return __builtin_mpl_vector_ld1q_v4i32(ptr);
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1_s64(int64_t const *ptr) {
  return __builtin_mpl_vector_ld1_v1i64(ptr);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1q_s64(int64_t const *ptr) {
  return __builtin_mpl_vector_ld1q_v2i64(ptr);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1_u8(uint8_t const *ptr) {
  return __builtin_mpl_vector_ld1_v8u8(ptr);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1q_u8(uint8_t const *ptr) {
  return __builtin_mpl_vector_ld1q_v16u8(ptr);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1_u16(uint16_t const *ptr) {
  return __builtin_mpl_vector_ld1_v4u16(ptr);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1q_u16(uint16_t const *ptr) {
  return __builtin_mpl_vector_ld1q_v8u16(ptr);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1_u32(uint32_t const *ptr) {
  return __builtin_mpl_vector_ld1_v2u32(ptr);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1q_u32(uint32_t const *ptr) {
  return __builtin_mpl_vector_ld1q_v4u32(ptr);
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1_u64(uint64_t const *ptr) {
  return __builtin_mpl_vector_ld1_v1u64(ptr);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1q_u64(uint64_t const *ptr) {
  return __builtin_mpl_vector_ld1q_v2u64(ptr);
}
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1_lane_s8(int8_t const *ptr, int8x8_t src,
                                                                                        const int lane) {
  return __builtin_mpl_vector_ld1_lane_v8i8(ptr, src, lane);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1q_lane_s8(int8_t const *ptr,
                                                                                          int8x16_t src,
                                                                                          const int lane) {
  return __builtin_mpl_vector_ld1q_lane_v16i8(ptr, src, lane);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1_lane_s16(int16_t const *ptr,
                                                                                          int16x4_t src,
                                                                                          const int lane) {
  return __builtin_mpl_vector_ld1_lane_v4i16(ptr, src, lane);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1q_lane_s16(int16_t const *ptr,
                                                                                           int16x8_t src,
                                                                                           const int lane) {
  return __builtin_mpl_vector_ld1q_lane_v8i16(ptr, src, lane);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1_lane_s32(int32_t const *ptr,
                                                                                          int32x2_t src,
                                                                                          const int lane) {
  return __builtin_mpl_vector_ld1_lane_v2i32(ptr, src, lane);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1q_lane_s32(int32_t const *ptr,
                                                                                           int32x4_t src,
                                                                                           const int lane) {
  return __builtin_mpl_vector_ld1q_lane_v4i32(ptr, src, lane);
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1_lane_s64(int64_t const *ptr,
                                                                                          int64x1_t src,
                                                                                          const int lane) {
  return __builtin_mpl_vector_ld1_lane_v1i64(ptr, src, lane);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1q_lane_s64(int64_t const *ptr,
                                                                                           int64x2_t src,
                                                                                           const int lane) {
  return __builtin_mpl_vector_ld1q_lane_v2i64(ptr, src, lane);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1_lane_u8(uint8_t const *ptr,
                                                                                         uint8x8_t src,
                                                                                         const int lane) {
  return __builtin_mpl_vector_ld1_lane_v8u8(ptr, src, lane);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1q_lane_u8(uint8_t const *ptr,
                                                                                           uint8x16_t src,
                                                                                           const int lane) {
  return __builtin_mpl_vector_ld1q_lane_v16u8(ptr, src, lane);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1_lane_u16(uint16_t const *ptr,
                                                                                           uint16x4_t src,
                                                                                           const int lane) {
  return __builtin_mpl_vector_ld1_lane_v4u16(ptr, src, lane);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1q_lane_u16(uint16_t const *ptr,
                                                                                            uint16x8_t src,
                                                                                            const int lane) {
  return __builtin_mpl_vector_ld1q_lane_v8u16(ptr, src, lane);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1_lane_u32(uint32_t const *ptr,
                                                                                           uint32x2_t src,
                                                                                           const int lane) {
  return __builtin_mpl_vector_ld1_lane_v2u32(ptr, src, lane);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1q_lane_u32(uint32_t const *ptr,
                                                                                            uint32x4_t src,
                                                                                            const int lane) {
  return __builtin_mpl_vector_ld1q_lane_v4u32(ptr, src, lane);
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1_lane_u64(uint64_t const *ptr,
                                                                                           uint64x1_t src,
                                                                                           const int lane) {
  return __builtin_mpl_vector_ld1_lane_v1u64(ptr, src, lane);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1q_lane_u64(uint64_t const *ptr,
                                                                                            uint64x2_t src,
                                                                                            const int lane) {
  return __builtin_mpl_vector_ld1q_lane_v2u64(ptr, src, lane);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1_dup_s8(int8_t const *ptr) {
  return __builtin_mpl_vector_ld1_dup_v8i8(ptr);
}

extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1q_dup_s8(int8_t const *ptr) {
  return __builtin_mpl_vector_ld1q_dup_v16i8(ptr);
}

extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1_dup_s16(int16_t const *ptr) {
  return __builtin_mpl_vector_ld1_dup_v4i16(ptr);
}

extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1q_dup_s16(int16_t const *ptr) {
  return __builtin_mpl_vector_ld1q_dup_v8i16(ptr);
}

extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1_dup_s32(int32_t const *ptr) {
  return __builtin_mpl_vector_ld1_dup_v2i32(ptr);
}

extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1q_dup_s32(int32_t const *ptr) {
  return __builtin_mpl_vector_ld1q_dup_v4i32(ptr);
}

extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1_dup_s64(int64_t const *ptr) {
  return __builtin_mpl_vector_ld1_dup_v1i64(ptr);
}

extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1q_dup_s64(int64_t const *ptr) {
  return __builtin_mpl_vector_ld1q_dup_v2i64(ptr);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1_dup_u8(uint8_t const *ptr) {
  return __builtin_mpl_vector_ld1_dup_v8u8(ptr);
}

extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1q_dup_u8(uint8_t const *ptr) {
  return __builtin_mpl_vector_ld1q_dup_v16u8(ptr);
}

extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1_dup_u16(uint16_t const *ptr) {
  return __builtin_mpl_vector_ld1_dup_v4u16(ptr);
}

extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1q_dup_u16(uint16_t const *ptr) {
  return __builtin_mpl_vector_ld1q_dup_v8u16(ptr);
}

extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1_dup_u32(uint32_t const *ptr) {
  return __builtin_mpl_vector_ld1_dup_v2u32(ptr);
}

extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1q_dup_u32(uint32_t const *ptr) {
  return __builtin_mpl_vector_ld1q_dup_v4u32(ptr);
}

extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1_dup_u64(uint64_t const *ptr) {
  return __builtin_mpl_vector_ld1_dup_v1u64(ptr);
}

extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld1q_dup_u64(uint64_t const *ptr) {
  return __builtin_mpl_vector_ld1q_dup_v2u64(ptr);
}
extern inline int8x8x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2_s8(int8_t const *ptr) {
  return __builtin_mpl_vector_ld2_v8i8(ptr);
}

extern inline int8x16x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2q_s8(int8_t const *ptr) {
  return __builtin_mpl_vector_ld2q_v16i8(ptr);
}

extern inline int16x4x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2_s16(int16_t const *ptr) {
  return __builtin_mpl_vector_ld2_v4i16(ptr);
}

extern inline int16x8x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2q_s16(int16_t const *ptr) {
  return __builtin_mpl_vector_ld2q_v8i16(ptr);
}

extern inline int32x2x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2_s32(int32_t const *ptr) {
  return __builtin_mpl_vector_ld2_v2i32(ptr);
}

extern inline int32x4x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2q_s32(int32_t const *ptr) {
  return __builtin_mpl_vector_ld2q_v4i32(ptr);
}

extern inline uint8x8x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2_u8(uint8_t const *ptr) {
  return __builtin_mpl_vector_ld2_v8u8(ptr);
}

extern inline uint8x16x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2q_u8(uint8_t const *ptr) {
  return __builtin_mpl_vector_ld2q_v16u8(ptr);
}

extern inline uint16x4x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2_u16(uint16_t const *ptr) {
  return __builtin_mpl_vector_ld2_v4u16(ptr);
}

extern inline uint16x8x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2q_u16(uint16_t const *ptr) {
  return __builtin_mpl_vector_ld2q_v8u16(ptr);
}

extern inline uint32x2x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2_u32(uint32_t const *ptr) {
  return __builtin_mpl_vector_ld2_v2u32(ptr);
}

extern inline uint32x4x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2q_u32(uint32_t const *ptr) {
  return __builtin_mpl_vector_ld2q_v4u32(ptr);
}

extern inline int64x1x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2_s64(int64_t const *ptr) {
  return __builtin_mpl_vector_ld2_v1i64(ptr);
}

extern inline uint64x1x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2_u64(uint64_t const *ptr) {
  return __builtin_mpl_vector_ld2_v1u64(ptr);
}

extern inline int64x2x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2q_s64(int64_t const *ptr) {
  return __builtin_mpl_vector_ld2q_v2i64(ptr);
}

extern inline uint64x2x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2q_u64(uint64_t const *ptr) {
  return __builtin_mpl_vector_ld2q_v2u64(ptr);
}

extern inline int8x8x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3_s8(int8_t const *ptr) {
  return __builtin_mpl_vector_ld3_v8i8(ptr);
}

extern inline int8x16x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3q_s8(int8_t const *ptr) {
  return __builtin_mpl_vector_ld3q_v16i8(ptr);
}

extern inline int16x4x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3_s16(int16_t const *ptr) {
  return __builtin_mpl_vector_ld3_v4i16(ptr);
}

extern inline int16x8x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3q_s16(int16_t const *ptr) {
  return __builtin_mpl_vector_ld3q_v8i16(ptr);
}

extern inline int32x2x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3_s32(int32_t const *ptr) {
  return __builtin_mpl_vector_ld3_v2i32(ptr);
}

extern inline int32x4x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3q_s32(int32_t const *ptr) {
  return __builtin_mpl_vector_ld3q_v4i32(ptr);
}

extern inline uint8x8x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3_u8(uint8_t const *ptr) {
  return __builtin_mpl_vector_ld3_v8u8(ptr);
}

extern inline uint8x16x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3q_u8(uint8_t const *ptr) {
  return __builtin_mpl_vector_ld3q_v16u8(ptr);
}

extern inline uint16x4x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3_u16(uint16_t const *ptr) {
  return __builtin_mpl_vector_ld3_v4u16(ptr);
}

extern inline uint16x8x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3q_u16(uint16_t const *ptr) {
  return __builtin_mpl_vector_ld3q_v8u16(ptr);
}

extern inline uint32x2x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3_u32(uint32_t const *ptr) {
  return __builtin_mpl_vector_ld3_v2u32(ptr);
}

extern inline uint32x4x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3q_u32(uint32_t const *ptr) {
  return __builtin_mpl_vector_ld3q_v4u32(ptr);
}

extern inline int64x1x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3_s64(int64_t const *ptr) {
  return __builtin_mpl_vector_ld3_v1i64(ptr);
}

extern inline uint64x1x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3_u64(uint64_t const *ptr) {
  return __builtin_mpl_vector_ld3_v1u64(ptr);
}

extern inline int64x2x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3q_s64(int64_t const *ptr) {
  return __builtin_mpl_vector_ld3q_v2i64(ptr);
}

extern inline uint64x2x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3q_u64(uint64_t const *ptr) {
  return __builtin_mpl_vector_ld3q_v2u64(ptr);
}

extern inline int8x8x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4_s8(int8_t const *ptr) {
  return __builtin_mpl_vector_ld4_v8i8(ptr);
}

extern inline int8x16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4q_s8(int8_t const *ptr) {
  return __builtin_mpl_vector_ld4q_v16i8(ptr);
}

extern inline int16x4x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4_s16(int16_t const *ptr) {
  return __builtin_mpl_vector_ld4_v4i16(ptr);
}

extern inline int16x8x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4q_s16(int16_t const *ptr) {
  return __builtin_mpl_vector_ld4q_v8i16(ptr);
}

extern inline int32x2x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4_s32(int32_t const *ptr) {
  return __builtin_mpl_vector_ld4_v2i32(ptr);
}

extern inline int32x4x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4q_s32(int32_t const *ptr) {
  return __builtin_mpl_vector_ld4q_v4i32(ptr);
}

extern inline uint8x8x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4_u8(uint8_t const *ptr) {
  return __builtin_mpl_vector_ld4_v8u8(ptr);
}

extern inline uint8x16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4q_u8(uint8_t const *ptr) {
  return __builtin_mpl_vector_ld4q_v16u8(ptr);
}

extern inline uint16x4x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4_u16(uint16_t const *ptr) {
  return __builtin_mpl_vector_ld4_v4u16(ptr);
}

extern inline uint16x8x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4q_u16(uint16_t const *ptr) {
  return __builtin_mpl_vector_ld4q_v8u16(ptr);
}

extern inline uint32x2x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4_u32(uint32_t const *ptr) {
  return __builtin_mpl_vector_ld4_v2u32(ptr);
}

extern inline uint32x4x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4q_u32(uint32_t const *ptr) {
  return __builtin_mpl_vector_ld4q_v4u32(ptr);
}

extern inline int64x1x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4_s64(int64_t const *ptr) {
  return __builtin_mpl_vector_ld4_v1i64(ptr);
}

extern inline uint64x1x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4_u64(uint64_t const *ptr) {
  return __builtin_mpl_vector_ld4_v1u64(ptr);
}

extern inline int64x2x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4q_s64(int64_t const *ptr) {
  return __builtin_mpl_vector_ld4q_v2i64(ptr);
}

extern inline uint64x2x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4q_u64(uint64_t const *ptr) {
  return __builtin_mpl_vector_ld4q_v2u64(ptr);
}

int8x8x2_t __builtin_mpl_vector_ld2_dup_v8i8(int8_t const *ptr);
extern inline int8x8x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2_dup_s8(int8_t const *ptr) {
  return __builtin_mpl_vector_ld2_dup_v8i8(ptr);
}

int8x16x2_t __builtin_mpl_vector_ld2q_dup_v16i8(int8_t const *ptr);
extern inline int8x16x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2q_dup_s8(int8_t const *ptr) {
  return __builtin_mpl_vector_ld2q_dup_v16i8(ptr);
}

int16x4x2_t __builtin_mpl_vector_ld2_dup_v4i16(int16_t const *ptr);
extern inline int16x4x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2_dup_s16(int16_t const *ptr) {
  return __builtin_mpl_vector_ld2_dup_v4i16(ptr);
}

int16x8x2_t __builtin_mpl_vector_ld2q_dup_v8i16(int16_t const *ptr);
extern inline int16x8x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2q_dup_s16(int16_t const *ptr) {
  return __builtin_mpl_vector_ld2q_dup_v8i16(ptr);
}

int32x2x2_t __builtin_mpl_vector_ld2_dup_v2i32(int32_t const *ptr);
extern inline int32x2x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2_dup_s32(int32_t const *ptr) {
  return __builtin_mpl_vector_ld2_dup_v2i32(ptr);
}

int32x4x2_t __builtin_mpl_vector_ld2q_dup_v4i32(int32_t const *ptr);
extern inline int32x4x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2q_dup_s32(int32_t const *ptr) {
  return __builtin_mpl_vector_ld2q_dup_v4i32(ptr);
}

uint8x8x2_t __builtin_mpl_vector_ld2_dup_v8u8(uint8_t const *ptr);
extern inline uint8x8x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2_dup_u8(uint8_t const *ptr) {
  return __builtin_mpl_vector_ld2_dup_v8u8(ptr);
}

uint8x16x2_t __builtin_mpl_vector_ld2q_dup_v16u8(uint8_t const *ptr);
extern inline uint8x16x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2q_dup_u8(uint8_t const *ptr) {
  return __builtin_mpl_vector_ld2q_dup_v16u8(ptr);
}

uint16x4x2_t __builtin_mpl_vector_ld2_dup_v4u16(uint16_t const *ptr);
extern inline uint16x4x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2_dup_u16(uint16_t const *ptr) {
  return __builtin_mpl_vector_ld2_dup_v4u16(ptr);
}

uint16x8x2_t __builtin_mpl_vector_ld2q_dup_v8u16(uint16_t const *ptr);
extern inline uint16x8x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2q_dup_u16(uint16_t const *ptr) {
  return __builtin_mpl_vector_ld2q_dup_v8u16(ptr);
}

uint32x2x2_t __builtin_mpl_vector_ld2_dup_v2u32(uint32_t const *ptr);
extern inline uint32x2x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2_dup_u32(uint32_t const *ptr) {
  return __builtin_mpl_vector_ld2_dup_v2u32(ptr);
}

uint32x4x2_t __builtin_mpl_vector_ld2q_dup_v4u32(uint32_t const *ptr);
extern inline uint32x4x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2q_dup_u32(uint32_t const *ptr) {
  return __builtin_mpl_vector_ld2q_dup_v4u32(ptr);
}

int64x1x2_t __builtin_mpl_vector_ld2_dup_v1i64(int64_t const *ptr);
extern inline int64x1x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2_dup_s64(int64_t const *ptr) {
  return __builtin_mpl_vector_ld2_dup_v1i64(ptr);
}

uint64x1x2_t __builtin_mpl_vector_ld2_dup_v1u64(uint64_t const *ptr);
extern inline uint64x1x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2_dup_u64(uint64_t const *ptr) {
  return __builtin_mpl_vector_ld2_dup_v1u64(ptr);
}

int64x2x2_t __builtin_mpl_vector_ld2q_dup_v2i64(int64_t const *ptr);
extern inline int64x2x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2q_dup_s64(int64_t const *ptr) {
  return __builtin_mpl_vector_ld2q_dup_v2i64(ptr);
}

uint64x2x2_t __builtin_mpl_vector_ld2q_dup_v2u64(uint64_t const *ptr);
extern inline uint64x2x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2q_dup_u64(uint64_t const *ptr) {
  return __builtin_mpl_vector_ld2q_dup_v2u64(ptr);
}

int8x8x3_t __builtin_mpl_vector_ld3_dup_v8i8(int8_t const *ptr);
extern inline int8x8x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3_dup_s8(int8_t const *ptr) {
  return __builtin_mpl_vector_ld3_dup_v8i8(ptr);
}

int8x16x3_t __builtin_mpl_vector_ld3q_dup_v16i8(int8_t const *ptr);
extern inline int8x16x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3q_dup_s8(int8_t const *ptr) {
  return __builtin_mpl_vector_ld3q_dup_v16i8(ptr);
}

int16x4x3_t __builtin_mpl_vector_ld3_dup_v4i16(int16_t const *ptr);
extern inline int16x4x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3_dup_s16(int16_t const *ptr) {
  return __builtin_mpl_vector_ld3_dup_v4i16(ptr);
}

int16x8x3_t __builtin_mpl_vector_ld3q_dup_v8i16(int16_t const *ptr);
extern inline int16x8x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3q_dup_s16(int16_t const *ptr) {
  return __builtin_mpl_vector_ld3q_dup_v8i16(ptr);
}

int32x2x3_t __builtin_mpl_vector_ld3_dup_v2i32(int32_t const *ptr);
extern inline int32x2x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3_dup_s32(int32_t const *ptr) {
  return __builtin_mpl_vector_ld3_dup_v2i32(ptr);
}

int32x4x3_t __builtin_mpl_vector_ld3q_dup_v4i32(int32_t const *ptr);
extern inline int32x4x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3q_dup_s32(int32_t const *ptr) {
  return __builtin_mpl_vector_ld3q_dup_v4i32(ptr);
}

uint8x8x3_t __builtin_mpl_vector_ld3_dup_v8u8(uint8_t const *ptr);
extern inline uint8x8x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3_dup_u8(uint8_t const *ptr) {
  return __builtin_mpl_vector_ld3_dup_v8u8(ptr);
}

uint8x16x3_t __builtin_mpl_vector_ld3q_dup_v16u8(uint8_t const *ptr);
extern inline uint8x16x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3q_dup_u8(uint8_t const *ptr) {
  return __builtin_mpl_vector_ld3q_dup_v16u8(ptr);
}

uint16x4x3_t __builtin_mpl_vector_ld3_dup_v4u16(uint16_t const *ptr);
extern inline uint16x4x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3_dup_u16(uint16_t const *ptr) {
  return __builtin_mpl_vector_ld3_dup_v4u16(ptr);
}

uint16x8x3_t __builtin_mpl_vector_ld3q_dup_v8u16(uint16_t const *ptr);
extern inline uint16x8x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3q_dup_u16(uint16_t const *ptr) {
  return __builtin_mpl_vector_ld3q_dup_v8u16(ptr);
}

uint32x2x3_t __builtin_mpl_vector_ld3_dup_v2u32(uint32_t const *ptr);
extern inline uint32x2x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3_dup_u32(uint32_t const *ptr) {
  return __builtin_mpl_vector_ld3_dup_v2u32(ptr);
}

uint32x4x3_t __builtin_mpl_vector_ld3q_dup_v4u32(uint32_t const *ptr);
extern inline uint32x4x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3q_dup_u32(uint32_t const *ptr) {
  return __builtin_mpl_vector_ld3q_dup_v4u32(ptr);
}

int64x1x3_t __builtin_mpl_vector_ld3_dup_v1i64(int64_t const *ptr);
extern inline int64x1x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3_dup_s64(int64_t const *ptr) {
  return __builtin_mpl_vector_ld3_dup_v1i64(ptr);
}

uint64x1x3_t __builtin_mpl_vector_ld3_dup_v1u64(uint64_t const *ptr);
extern inline uint64x1x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3_dup_u64(uint64_t const *ptr) {
  return __builtin_mpl_vector_ld3_dup_v1u64(ptr);
}

int64x2x3_t __builtin_mpl_vector_ld3q_dup_v2i64(int64_t const *ptr);
extern inline int64x2x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3q_dup_s64(int64_t const *ptr) {
  return __builtin_mpl_vector_ld3q_dup_v2i64(ptr);
}

uint64x2x3_t __builtin_mpl_vector_ld3q_dup_v2u64(uint64_t const *ptr);
extern inline uint64x2x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3q_dup_u64(uint64_t const *ptr) {
  return __builtin_mpl_vector_ld3q_dup_v2u64(ptr);
}

int8x8x4_t __builtin_mpl_vector_ld4_dup_v8i8(int8_t const *ptr);
extern inline int8x8x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4_dup_s8(int8_t const *ptr) {
  return __builtin_mpl_vector_ld4_dup_v8i8(ptr);
}

int8x16x4_t __builtin_mpl_vector_ld4q_dup_v16i8(int8_t const *ptr);
extern inline int8x16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4q_dup_s8(int8_t const *ptr) {
  return __builtin_mpl_vector_ld4q_dup_v16i8(ptr);
}

int16x4x4_t __builtin_mpl_vector_ld4_dup_v4i16(int16_t const *ptr);
extern inline int16x4x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4_dup_s16(int16_t const *ptr) {
  return __builtin_mpl_vector_ld4_dup_v4i16(ptr);
}

int16x8x4_t __builtin_mpl_vector_ld4q_dup_v8i16(int16_t const *ptr);
extern inline int16x8x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4q_dup_s16(int16_t const *ptr) {
  return __builtin_mpl_vector_ld4q_dup_v8i16(ptr);
}

int32x2x4_t __builtin_mpl_vector_ld4_dup_v2i32(int32_t const *ptr);
extern inline int32x2x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4_dup_s32(int32_t const *ptr) {
  return __builtin_mpl_vector_ld4_dup_v2i32(ptr);
}

int32x4x4_t __builtin_mpl_vector_ld4q_dup_v4i32(int32_t const *ptr);
extern inline int32x4x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4q_dup_s32(int32_t const *ptr) {
  return __builtin_mpl_vector_ld4q_dup_v4i32(ptr);
}

uint8x8x4_t __builtin_mpl_vector_ld4_dup_v8u8(uint8_t const *ptr);
extern inline uint8x8x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4_dup_u8(uint8_t const *ptr) {
  return __builtin_mpl_vector_ld4_dup_v8u8(ptr);
}

uint8x16x4_t __builtin_mpl_vector_ld4q_dup_v16u8(uint8_t const *ptr);
extern inline uint8x16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4q_dup_u8(uint8_t const *ptr) {
  return __builtin_mpl_vector_ld4q_dup_v16u8(ptr);
}

uint16x4x4_t __builtin_mpl_vector_ld4_dup_v4u16(uint16_t const *ptr);
extern inline uint16x4x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4_dup_u16(uint16_t const *ptr) {
  return __builtin_mpl_vector_ld4_dup_v4u16(ptr);
}

uint16x8x4_t __builtin_mpl_vector_ld4q_dup_v8u16(uint16_t const *ptr);
extern inline uint16x8x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4q_dup_u16(uint16_t const *ptr) {
  return __builtin_mpl_vector_ld4q_dup_v8u16(ptr);
}

uint32x2x4_t __builtin_mpl_vector_ld4_dup_v2u32(uint32_t const *ptr);
extern inline uint32x2x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4_dup_u32(uint32_t const *ptr) {
  return __builtin_mpl_vector_ld4_dup_v2u32(ptr);
}

uint32x4x4_t __builtin_mpl_vector_ld4q_dup_v4u32(uint32_t const *ptr);
extern inline uint32x4x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4q_dup_u32(uint32_t const *ptr) {
  return __builtin_mpl_vector_ld4q_dup_v4u32(ptr);
}

int64x1x4_t __builtin_mpl_vector_ld4_dup_v1i64(int64_t const *ptr);
extern inline int64x1x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4_dup_s64(int64_t const *ptr) {
  return __builtin_mpl_vector_ld4_dup_v1i64(ptr);
}

uint64x1x4_t __builtin_mpl_vector_ld4_dup_v1u64(uint64_t const *ptr);
extern inline uint64x1x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4_dup_u64(uint64_t const *ptr) {
  return __builtin_mpl_vector_ld4_dup_v1u64(ptr);
}

int64x2x4_t __builtin_mpl_vector_ld4q_dup_v2i64(int64_t const *ptr);
extern inline int64x2x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4q_dup_s64(int64_t const *ptr) {
  return __builtin_mpl_vector_ld4q_dup_v2i64(ptr);
}

uint64x2x4_t __builtin_mpl_vector_ld4q_dup_v2u64(uint64_t const *ptr);
extern inline uint64x2x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4q_dup_u64(uint64_t const *ptr) {
  return __builtin_mpl_vector_ld4q_dup_v2u64(ptr);
}

int16x4x2_t __builtin_mpl_vector_ld2_lane_v4i16(int16_t const *ptr, int16x4x2_t src, const int lane);
extern inline int16x4x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2_lane_s16(int16_t const *ptr,
                                                                                            int16x4x2_t src,
                                                                                            const int lane) {
  return __builtin_mpl_vector_ld2_lane_v4i16(ptr, src, lane);
}

int16x8x2_t __builtin_mpl_vector_ld2q_lane_v8i16(int16_t const *ptr, int16x8x2_t src, const int lane);
extern inline int16x8x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2q_lane_s16(int16_t const *ptr,
                                                                                             int16x8x2_t src,
                                                                                             const int lane) {
  return __builtin_mpl_vector_ld2q_lane_v8i16(ptr, src, lane);
}

int32x2x2_t __builtin_mpl_vector_ld2_lane_v2i32(int32_t const *ptr, int32x2x2_t src, const int lane);
extern inline int32x2x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2_lane_s32(int32_t const *ptr,
                                                                                            int32x2x2_t src,
                                                                                            const int lane) {
  return __builtin_mpl_vector_ld2_lane_v2i32(ptr, src, lane);
}

int32x4x2_t __builtin_mpl_vector_ld2q_lane_v4i32(int32_t const *ptr, int32x4x2_t src, const int lane);
extern inline int32x4x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2q_lane_s32(int32_t const *ptr,
                                                                                             int32x4x2_t src,
                                                                                             const int lane) {
  return __builtin_mpl_vector_ld2q_lane_v4i32(ptr, src, lane);
}

uint16x4x2_t __builtin_mpl_vector_ld2_lane_v4u16(uint16_t const *ptr, uint16x4x2_t src, const int lane);
extern inline uint16x4x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2_lane_u16(uint16_t const *ptr,
                                                                                             uint16x4x2_t src,
                                                                                             const int lane) {
  return __builtin_mpl_vector_ld2_lane_v4u16(ptr, src, lane);
}

uint16x8x2_t __builtin_mpl_vector_ld2q_lane_v8u16(uint16_t const *ptr, uint16x8x2_t src, const int lane);
extern inline uint16x8x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2q_lane_u16(uint16_t const *ptr,
                                                                                              uint16x8x2_t src,
                                                                                              const int lane) {
  return __builtin_mpl_vector_ld2q_lane_v8u16(ptr, src, lane);
}

uint32x2x2_t __builtin_mpl_vector_ld2_lane_v2u32(uint32_t const *ptr, uint32x2x2_t src, const int lane);
extern inline uint32x2x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2_lane_u32(uint32_t const *ptr,
                                                                                             uint32x2x2_t src,
                                                                                             const int lane) {
  return __builtin_mpl_vector_ld2_lane_v2u32(ptr, src, lane);
}

uint32x4x2_t __builtin_mpl_vector_ld2q_lane_v4u32(uint32_t const *ptr, uint32x4x2_t src, const int lane);
extern inline uint32x4x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2q_lane_u32(uint32_t const *ptr,
                                                                                              uint32x4x2_t src,
                                                                                              const int lane) {
  return __builtin_mpl_vector_ld2q_lane_v4u32(ptr, src, lane);
}

int8x8x2_t __builtin_mpl_vector_ld2_lane_v8i8(int8_t const *ptr, int8x8x2_t src, const int lane);
extern inline int8x8x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2_lane_s8(int8_t const *ptr,
                                                                                          int8x8x2_t src,
                                                                                          const int lane) {
  return __builtin_mpl_vector_ld2_lane_v8i8(ptr, src, lane);
}

uint8x8x2_t __builtin_mpl_vector_ld2_lane_v8u8(uint8_t const *ptr, uint8x8x2_t src, const int lane);
extern inline uint8x8x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2_lane_u8(uint8_t const *ptr,
                                                                                           uint8x8x2_t src,
                                                                                           const int lane) {
  return __builtin_mpl_vector_ld2_lane_v8u8(ptr, src, lane);
}

int8x16x2_t __builtin_mpl_vector_ld2q_lane_v16i8(int8_t const *ptr, int8x16x2_t src, const int lane);
extern inline int8x16x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2q_lane_s8(int8_t const *ptr,
                                                                                            int8x16x2_t src,
                                                                                            const int lane) {
  return __builtin_mpl_vector_ld2q_lane_v16i8(ptr, src, lane);
}

uint8x16x2_t __builtin_mpl_vector_ld2q_lane_v16u8(uint8_t const *ptr, uint8x16x2_t src, const int lane);
extern inline uint8x16x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2q_lane_u8(uint8_t const *ptr,
                                                                                             uint8x16x2_t src,
                                                                                             const int lane) {
  return __builtin_mpl_vector_ld2q_lane_v16u8(ptr, src, lane);
}

int64x1x2_t __builtin_mpl_vector_ld2_lane_v1i64(int64_t const *ptr, int64x1x2_t src, const int lane);
extern inline int64x1x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2_lane_s64(int64_t const *ptr,
                                                                                            int64x1x2_t src,
                                                                                            const int lane) {
  return __builtin_mpl_vector_ld2_lane_v1i64(ptr, src, lane);
}

int64x2x2_t __builtin_mpl_vector_ld2q_lane_v2i64(int64_t const *ptr, int64x2x2_t src, const int lane);
extern inline int64x2x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2q_lane_s64(int64_t const *ptr,
                                                                                             int64x2x2_t src,
                                                                                             const int lane) {
  return __builtin_mpl_vector_ld2q_lane_v2i64(ptr, src, lane);
}

uint64x1x2_t __builtin_mpl_vector_ld2_lane_v1u64(uint64_t const *ptr, uint64x1x2_t src, const int lane);
extern inline uint64x1x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2_lane_u64(uint64_t const *ptr,
                                                                                             uint64x1x2_t src,
                                                                                             const int lane) {
  return __builtin_mpl_vector_ld2_lane_v1u64(ptr, src, lane);
}

uint64x2x2_t __builtin_mpl_vector_ld2q_lane_v2u64(uint64_t const *ptr, uint64x2x2_t src, const int lane);
extern inline uint64x2x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vld2q_lane_u64(uint64_t const *ptr,
                                                                                              uint64x2x2_t src,
                                                                                              const int lane) {
  return __builtin_mpl_vector_ld2q_lane_v2u64(ptr, src, lane);
}

int16x4x3_t __builtin_mpl_vector_ld3_lane_v4i16(int16_t const *ptr, int16x4x3_t src, const int lane);
extern inline int16x4x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3_lane_s16(int16_t const *ptr,
                                                                                            int16x4x3_t src,
                                                                                            const int lane) {
  return __builtin_mpl_vector_ld3_lane_v4i16(ptr, src, lane);
}

int16x8x3_t __builtin_mpl_vector_ld3q_lane_v8i16(int16_t const *ptr, int16x8x3_t src, const int lane);
extern inline int16x8x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3q_lane_s16(int16_t const *ptr,
                                                                                             int16x8x3_t src,
                                                                                             const int lane) {
  return __builtin_mpl_vector_ld3q_lane_v8i16(ptr, src, lane);
}

int32x2x3_t __builtin_mpl_vector_ld3_lane_v2i32(int32_t const *ptr, int32x2x3_t src, const int lane);
extern inline int32x2x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3_lane_s32(int32_t const *ptr,
                                                                                            int32x2x3_t src,
                                                                                            const int lane) {
  return __builtin_mpl_vector_ld3_lane_v2i32(ptr, src, lane);
}

int32x4x3_t __builtin_mpl_vector_ld3q_lane_v4i32(int32_t const *ptr, int32x4x3_t src, const int lane);
extern inline int32x4x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3q_lane_s32(int32_t const *ptr,
                                                                                             int32x4x3_t src,
                                                                                             const int lane) {
  return __builtin_mpl_vector_ld3q_lane_v4i32(ptr, src, lane);
}

uint16x4x3_t __builtin_mpl_vector_ld3_lane_v4u16(uint16_t const *ptr, uint16x4x3_t src, const int lane);
extern inline uint16x4x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3_lane_u16(uint16_t const *ptr,
                                                                                             uint16x4x3_t src,
                                                                                             const int lane) {
  return __builtin_mpl_vector_ld3_lane_v4u16(ptr, src, lane);
}

uint16x8x3_t __builtin_mpl_vector_ld3q_lane_v8u16(uint16_t const *ptr, uint16x8x3_t src, const int lane);
extern inline uint16x8x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3q_lane_u16(uint16_t const *ptr,
                                                                                              uint16x8x3_t src,
                                                                                              const int lane) {
  return __builtin_mpl_vector_ld3q_lane_v8u16(ptr, src, lane);
}

uint32x2x3_t __builtin_mpl_vector_ld3_lane_v2u32(uint32_t const *ptr, uint32x2x3_t src, const int lane);
extern inline uint32x2x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3_lane_u32(uint32_t const *ptr,
                                                                                             uint32x2x3_t src,
                                                                                             const int lane) {
  return __builtin_mpl_vector_ld3_lane_v2u32(ptr, src, lane);
}

uint32x4x3_t __builtin_mpl_vector_ld3q_lane_v4u32(uint32_t const *ptr, uint32x4x3_t src, const int lane);
extern inline uint32x4x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3q_lane_u32(uint32_t const *ptr,
                                                                                              uint32x4x3_t src,
                                                                                              const int lane) {
  return __builtin_mpl_vector_ld3q_lane_v4u32(ptr, src, lane);
}

int8x8x3_t __builtin_mpl_vector_ld3_lane_v8i8(int8_t const *ptr, int8x8x3_t src, const int lane);
extern inline int8x8x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3_lane_s8(int8_t const *ptr,
                                                                                          int8x8x3_t src,
                                                                                          const int lane) {
  return __builtin_mpl_vector_ld3_lane_v8i8(ptr, src, lane);
}

uint8x8x3_t __builtin_mpl_vector_ld3_lane_v8u8(uint8_t const *ptr, uint8x8x3_t src, const int lane);
extern inline uint8x8x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3_lane_u8(uint8_t const *ptr,
                                                                                           uint8x8x3_t src,
                                                                                           const int lane) {
  return __builtin_mpl_vector_ld3_lane_v8u8(ptr, src, lane);
}

int8x16x3_t __builtin_mpl_vector_ld3q_lane_v16i8(int8_t const *ptr, int8x16x3_t src, const int lane);
extern inline int8x16x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3q_lane_s8(int8_t const *ptr,
                                                                                            int8x16x3_t src,
                                                                                            const int lane) {
  return __builtin_mpl_vector_ld3q_lane_v16i8(ptr, src, lane);
}

uint8x16x3_t __builtin_mpl_vector_ld3q_lane_v16u8(uint8_t const *ptr, uint8x16x3_t src, const int lane);
extern inline uint8x16x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3q_lane_u8(uint8_t const *ptr,
                                                                                             uint8x16x3_t src,
                                                                                             const int lane) {
  return __builtin_mpl_vector_ld3q_lane_v16u8(ptr, src, lane);
}

int64x1x3_t __builtin_mpl_vector_ld3_lane_v1i64(int64_t const *ptr, int64x1x3_t src, const int lane);
extern inline int64x1x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3_lane_s64(int64_t const *ptr,
                                                                                            int64x1x3_t src,
                                                                                            const int lane) {
  return __builtin_mpl_vector_ld3_lane_v1i64(ptr, src, lane);
}

int64x2x3_t __builtin_mpl_vector_ld3q_lane_v2i64(int64_t const *ptr, int64x2x3_t src, const int lane);
extern inline int64x2x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3q_lane_s64(int64_t const *ptr,
                                                                                             int64x2x3_t src,
                                                                                             const int lane) {
  return __builtin_mpl_vector_ld3q_lane_v2i64(ptr, src, lane);
}

uint64x1x3_t __builtin_mpl_vector_ld3_lane_v1u64(uint64_t const *ptr, uint64x1x3_t src, const int lane);
extern inline uint64x1x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3_lane_u64(uint64_t const *ptr,
                                                                                             uint64x1x3_t src,
                                                                                             const int lane) {
  return __builtin_mpl_vector_ld3_lane_v1u64(ptr, src, lane);
}

uint64x2x3_t __builtin_mpl_vector_ld3q_lane_v2u64(uint64_t const *ptr, uint64x2x3_t src, const int lane);
extern inline uint64x2x3_t __attribute__ ((__always_inline__, __gnu_inline__)) vld3q_lane_u64(uint64_t const *ptr,
                                                                                              uint64x2x3_t src,
                                                                                              const int lane) {
  return __builtin_mpl_vector_ld3q_lane_v2u64(ptr, src, lane);
}

int16x4x4_t __builtin_mpl_vector_ld4_lane_v4i16(int16_t const *ptr, int16x4x4_t src, const int lane);
extern inline int16x4x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4_lane_s16(int16_t const *ptr,
                                                                                            int16x4x4_t src,
                                                                                            const int lane) {
  return __builtin_mpl_vector_ld4_lane_v4i16(ptr, src, lane);
}

int16x8x4_t __builtin_mpl_vector_ld4q_lane_v8i16(int16_t const *ptr, int16x8x4_t src, const int lane);
extern inline int16x8x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4q_lane_s16(int16_t const *ptr,
                                                                                             int16x8x4_t src,
                                                                                             const int lane) {
  return __builtin_mpl_vector_ld4q_lane_v8i16(ptr, src, lane);
}

int32x2x4_t __builtin_mpl_vector_ld4_lane_v2i32(int32_t const *ptr, int32x2x4_t src, const int lane);
extern inline int32x2x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4_lane_s32(int32_t const *ptr,
                                                                                            int32x2x4_t src,
                                                                                            const int lane) {
  return __builtin_mpl_vector_ld4_lane_v2i32(ptr, src, lane);
}

int32x4x4_t __builtin_mpl_vector_ld4q_lane_v4i32(int32_t const *ptr, int32x4x4_t src, const int lane);
extern inline int32x4x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4q_lane_s32(int32_t const *ptr,
                                                                                             int32x4x4_t src,
                                                                                             const int lane) {
  return __builtin_mpl_vector_ld4q_lane_v4i32(ptr, src, lane);
}

uint16x4x4_t __builtin_mpl_vector_ld4_lane_v4u16(uint16_t const *ptr, uint16x4x4_t src, const int lane);
extern inline uint16x4x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4_lane_u16(uint16_t const *ptr,
                                                                                             uint16x4x4_t src,
                                                                                             const int lane) {
  return __builtin_mpl_vector_ld4_lane_v4u16(ptr, src, lane);
}

uint16x8x4_t __builtin_mpl_vector_ld4q_lane_v8u16(uint16_t const *ptr, uint16x8x4_t src, const int lane);
extern inline uint16x8x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4q_lane_u16(uint16_t const *ptr,
                                                                                              uint16x8x4_t src,
                                                                                              const int lane) {
  return __builtin_mpl_vector_ld4q_lane_v8u16(ptr, src, lane);
}

uint32x2x4_t __builtin_mpl_vector_ld4_lane_v2u32(uint32_t const *ptr, uint32x2x4_t src, const int lane);
extern inline uint32x2x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4_lane_u32(uint32_t const *ptr,
                                                                                             uint32x2x4_t src,
                                                                                             const int lane) {
  return __builtin_mpl_vector_ld4_lane_v2u32(ptr, src, lane);
}

uint32x4x4_t __builtin_mpl_vector_ld4q_lane_v4u32(uint32_t const *ptr, uint32x4x4_t src, const int lane);
extern inline uint32x4x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4q_lane_u32(uint32_t const *ptr,
                                                                                              uint32x4x4_t src,
                                                                                              const int lane) {
  return __builtin_mpl_vector_ld4q_lane_v4u32(ptr, src, lane);
}

int8x8x4_t __builtin_mpl_vector_ld4_lane_v8i8(int8_t const *ptr, int8x8x4_t src, const int lane);
extern inline int8x8x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4_lane_s8(int8_t const *ptr,
                                                                                          int8x8x4_t src,
                                                                                          const int lane) {
  return __builtin_mpl_vector_ld4_lane_v8i8(ptr, src, lane);
}

uint8x8x4_t __builtin_mpl_vector_ld4_lane_v8u8(uint8_t const *ptr, uint8x8x4_t src, const int lane);
extern inline uint8x8x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4_lane_u8(uint8_t const *ptr,
                                                                                           uint8x8x4_t src,
                                                                                           const int lane) {
  return __builtin_mpl_vector_ld4_lane_v8u8(ptr, src, lane);
}

int8x16x4_t __builtin_mpl_vector_ld4q_lane_v16i8(int8_t const *ptr, int8x16x4_t src, const int lane);
extern inline int8x16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4q_lane_s8(int8_t const *ptr,
                                                                                            int8x16x4_t src,
                                                                                            const int lane) {
  return __builtin_mpl_vector_ld4q_lane_v16i8(ptr, src, lane);
}

uint8x16x4_t __builtin_mpl_vector_ld4q_lane_v16u8(uint8_t const *ptr, uint8x16x4_t src, const int lane);
extern inline uint8x16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4q_lane_u8(uint8_t const *ptr,
                                                                                             uint8x16x4_t src,
                                                                                             const int lane) {
  return __builtin_mpl_vector_ld4q_lane_v16u8(ptr, src, lane);
}

int64x1x4_t __builtin_mpl_vector_ld4_lane_v1i64(int64_t const *ptr, int64x1x4_t src, const int lane);
extern inline int64x1x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4_lane_s64(int64_t const *ptr,
                                                                                            int64x1x4_t src,
                                                                                            const int lane) {
  return __builtin_mpl_vector_ld4_lane_v1i64(ptr, src, lane);
}

int64x2x4_t __builtin_mpl_vector_ld4q_lane_v2i64(int64_t const *ptr, int64x2x4_t src, const int lane);
extern inline int64x2x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4q_lane_s64(int64_t const *ptr,
                                                                                             int64x2x4_t src,
                                                                                             const int lane) {
  return __builtin_mpl_vector_ld4q_lane_v2i64(ptr, src, lane);
}

uint64x1x4_t __builtin_mpl_vector_ld4_lane_v1u64(uint64_t const *ptr, uint64x1x4_t src, const int lane);
extern inline uint64x1x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4_lane_u64(uint64_t const *ptr,
                                                                                             uint64x1x4_t src,
                                                                                             const int lane) {
  return __builtin_mpl_vector_ld4_lane_v1u64(ptr, src, lane);
}

uint64x2x4_t __builtin_mpl_vector_ld4q_lane_v2u64(uint64_t const *ptr, uint64x2x4_t src, const int lane);
extern inline uint64x2x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vld4q_lane_u64(uint64_t const *ptr,
                                                                                              uint64x2x4_t src, const
                                                                                              int lane) {
  return __builtin_mpl_vector_ld4q_lane_v2u64(ptr, src, lane);
}

int8x8_t __builtin_mpl_vector_tbl1_v8i8(int8x16_t, int8x8_t);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtbl1_s8(int8x8_t a, int8x8_t idx) {
  return __builtin_mpl_vector_tbl1_v8i8(vcombine_s8(a, (int8x8_t){0}), idx);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtbl1_u8(uint8x8_t a, uint8x8_t idx) {
  return __builtin_mpl_vector_tbl1_v8i8(vcombine_s8(a, (uint8x8_t){0}), idx);
}

int8x8_t __builtin_mpl_vector_qtbl1_v8i8(int8x16_t t, uint8x8_t idx);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbl1_s8(int8x16_t t, uint8x8_t idx) {
  return __builtin_mpl_vector_qtbl1_v8i8(t, idx);
}

int8x16_t __builtin_mpl_vector_qtbl1q_v16i8(int8x16_t t, uint8x16_t idx);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbl1q_s8(int8x16_t t, uint8x16_t idx) {
  return __builtin_mpl_vector_qtbl1q_v16i8(t, idx);
}

uint8x8_t __builtin_mpl_vector_qtbl1_v8u8(uint8x16_t t, uint8x8_t idx);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbl1_u8(uint8x16_t t, uint8x8_t idx) {
  return __builtin_mpl_vector_qtbl1_v8u8(t, idx);
}

uint8x16_t __builtin_mpl_vector_qtbl1q_v16u8(uint8x16_t t, uint8x16_t idx);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbl1q_u8(uint8x16_t t, uint8x16_t idx) {
  return __builtin_mpl_vector_qtbl1q_v16u8(t, idx);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtbl2_s8(int8x8x2_t a, int8x8_t idx) {
  return vqtbl1_s8(vcombine_s8(a.val[0], a.val[1]), idx);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtbl2_u8(uint8x8x2_t a, uint8x8_t idx) {
  return vqtbl1_u8(vcombine_u8(a.val[0], a.val[1]), idx);
}

int8x8_t __builtin_mpl_vector_tbl3_v8i8(int8x8x3_t a, int8x8_t idx);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtbl3_s8(int8x8x3_t a, int8x8_t idx) {
  return __builtin_mpl_vector_tbl3_v8i8(a, idx);
}

uint8x8_t __builtin_mpl_vector_tbl3_v8u8(uint8x8x3_t a, uint8x8_t idx);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtbl3_u8(uint8x8x3_t a, uint8x8_t idx) {
  return __builtin_mpl_vector_tbl3_v8u8(a, idx);
}

int8x8_t __builtin_mpl_vector_tbl4_v8i8(int8x8x4_t a, int8x8_t idx);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtbl4_s8(int8x8x4_t a, int8x8_t idx) {
  return __builtin_mpl_vector_tbl4_v8i8(a, idx);
}

uint8x8_t __builtin_mpl_vector_tbl4_v8u8(uint8x8x4_t a, uint8x8_t idx);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtbl4_u8(uint8x8x4_t a, uint8x8_t idx) {
  return __builtin_mpl_vector_tbl4_v8u8(a, idx);
}

int8x8_t __builtin_mpl_vector_qtbl2_v8i8(int8x16x2_t t, uint8x8_t idx);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbl2_s8(int8x16x2_t t, uint8x8_t idx) {
  return __builtin_mpl_vector_qtbl2_v8i8(t, idx);
}

int8x16_t __builtin_mpl_vector_qtbl2q_v16i8(int8x16x2_t t, uint8x16_t idx);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbl2q_s8(int8x16x2_t t, uint8x16_t idx) {
  return __builtin_mpl_vector_qtbl2q_v16i8(t, idx);
}

uint8x8_t __builtin_mpl_vector_qtbl2_v8u8(uint8x16x2_t t, uint8x8_t idx);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbl2_u8(uint8x16x2_t t, uint8x8_t idx) {
  return __builtin_mpl_vector_qtbl2_v8u8(t, idx);
}

uint8x16_t __builtin_mpl_vector_qtbl2q_v16u8(uint8x16x2_t t, uint8x16_t idx);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbl2q_u8(uint8x16x2_t t,
                                                                                        uint8x16_t idx) {
  return __builtin_mpl_vector_qtbl2q_v16u8(t, idx);
}

int8x8_t __builtin_mpl_vector_qtbl3_v8i8(int8x16x3_t t, uint8x8_t idx);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbl3_s8(int8x16x3_t t, uint8x8_t idx) {
  return __builtin_mpl_vector_qtbl3_v8i8(t, idx);
}

int8x16_t __builtin_mpl_vector_qtbl3q_v16i8(int8x16x3_t t, uint8x16_t idx);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbl3q_s8(int8x16x3_t t, uint8x16_t idx) {
  return __builtin_mpl_vector_qtbl3q_v16i8(t, idx);
}

uint8x8_t __builtin_mpl_vector_qtbl3_v8u8(uint8x16x3_t t, uint8x8_t idx);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbl3_u8(uint8x16x3_t t, uint8x8_t idx) {
  return __builtin_mpl_vector_qtbl3_v8u8(t, idx);
}

uint8x16_t __builtin_mpl_vector_qtbl3q_v16u8(uint8x16x3_t t, uint8x16_t idx);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbl3q_u8(uint8x16x3_t t,
                                                                                        uint8x16_t idx) {
  return __builtin_mpl_vector_qtbl3q_v16u8(t, idx);
}

int8x8_t __builtin_mpl_vector_qtbl4_v8i8(int8x16x4_t t, uint8x8_t idx);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbl4_s8(int8x16x4_t t, uint8x8_t idx) {
  return __builtin_mpl_vector_qtbl4_v8i8(t, idx);
}

int8x16_t __builtin_mpl_vector_qtbl4q_v16i8(int8x16x4_t t, uint8x16_t idx);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbl4q_s8(int8x16x4_t t, uint8x16_t idx) {
  return __builtin_mpl_vector_qtbl4q_v16i8(t, idx);
}

uint8x8_t __builtin_mpl_vector_qtbl4_v8u8(uint8x16x4_t t, uint8x8_t idx);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbl4_u8(uint8x16x4_t t, uint8x8_t idx) {
  return __builtin_mpl_vector_qtbl4_v8u8(t, idx);
}

uint8x16_t __builtin_mpl_vector_qtbl4q_v16u8(uint8x16x4_t t, uint8x16_t idx);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbl4q_u8(uint8x16x4_t t,
                                                                                        uint8x16_t idx) {
  return __builtin_mpl_vector_qtbl4q_v16u8(t, idx);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtbx1_s8(int8x8_t a, int8x8_t b,
                                                                                    int8x8_t idx) {
  uint8x8_t mask = vclt_u8((uint8x8_t)(idx), vmov_n_u8(8));
  int8x8_t tbl = vtbl1_s8(b, idx);
  return vbsl_s8(mask, tbl, a);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtbx1_u8(uint8x8_t a, uint8x8_t b,
                                                                                     uint8x8_t idx) {
  uint8x8_t mask = vclt_u8(idx, vmov_n_u8(8));
  uint8x8_t tbl = vtbl1_u8(b, idx);
  return vbsl_u8(mask, tbl, a);
}

int8x8_t __builtin_mpl_vector_qtbx1_v8i8(int8x8_t a, int8x16_t t, uint8x8_t idx);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbx1_s8(int8x8_t a, int8x16_t t,
                                                                                     uint8x8_t idx) {
  return __builtin_mpl_vector_qtbx1_v8i8(a, t, idx);
}

int8x16_t __builtin_mpl_vector_qtbx1q_v16i8(int8x16_t a, int8x16_t t, uint8x16_t idx);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbx1q_s8(int8x16_t a, int8x16_t t,
                                                                                       uint8x16_t idx) {
  return __builtin_mpl_vector_qtbx1q_v16i8(a, t, idx);
}

uint8x8_t __builtin_mpl_vector_qtbx1_v8u8(uint8x8_t a, uint8x16_t t, uint8x8_t idx);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbx1_u8(uint8x8_t a, uint8x16_t t,
                                                                                      uint8x8_t idx) {
  return __builtin_mpl_vector_qtbx1_v8u8(a, t, idx);
}

uint8x16_t __builtin_mpl_vector_qtbx1q_v16u8(uint8x16_t a, uint8x16_t t, uint8x16_t idx);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbx1q_u8(uint8x16_t a, uint8x16_t t,
                                                                                        uint8x16_t idx) {
  return __builtin_mpl_vector_qtbx1q_v16u8(a, t, idx);
}

extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtbx2_s8(int8x8_t a, int8x8x2_t b,
                                                                                    int8x8_t idx) {
  return vqtbx1_s8(a, vcombine_s8(b.val[0], b.val[1]), idx);
}

extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtbx2_u8(uint8x8_t a, uint8x8x2_t b,
                                                                                     uint8x8_t idx) {
  return vqtbx1_u8(a, vcombine_u8(b.val[0], b.val[1]), idx);
}

int8x8_t __builtin_mpl_vector_tbx3_v8i8(int8x8_t a, int8x8x3_t b, int8x8_t idx);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtbx3_s8(int8x8_t a, int8x8x3_t b,
                                                                                    int8x8_t idx) {
  return __builtin_mpl_vector_tbx3_v8i8(a, b, idx);
}

uint8x8_t __builtin_mpl_vector_tbx3_v8u8(uint8x8_t a, uint8x8x3_t b, uint8x8_t idx);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtbx3_u8(uint8x8_t a, uint8x8x3_t b,
                                                                                     uint8x8_t idx) {
  return __builtin_mpl_vector_tbx3_v8u8(a, b, idx);
}

int8x8_t __builtin_mpl_vector_tbx4_v8i8(int8x8_t a, int8x8x4_t b, int8x8_t idx);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtbx4_s8(int8x8_t a, int8x8x4_t b,
                                                                                    int8x8_t idx) {
  return __builtin_mpl_vector_tbx4_v8i8(a, b, idx);
}

uint8x8_t __builtin_mpl_vector_tbx4_v8u8(uint8x8_t a, uint8x8x4_t b, uint8x8_t idx);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vtbx4_u8(uint8x8_t a, uint8x8x4_t b,
                                                                                     uint8x8_t idx) {
  return __builtin_mpl_vector_tbx4_v8u8(a, b, idx);
}

int8x8_t __builtin_mpl_vector_qtbx1_v8i8(int8x8_t a, int8x16_t t, uint8x8_t idx);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbx1_s8(int8x8_t a, int8x16_t t,
                                                                                     uint8x8_t idx) {
  return __builtin_mpl_vector_qtbx1_v8i8(a, t, idx);
}

int8x16_t __builtin_mpl_vector_qtbx1q_v16i8(int8x16_t a, int8x16_t t, uint8x16_t idx);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbx1q_s8(int8x16_t a, int8x16_t t,
                                                                                       uint8x16_t idx) {
  return __builtin_mpl_vector_qtbx1q_v16i8(a, t, idx);
}

uint8x8_t __builtin_mpl_vector_qtbx1_v8u8(uint8x8_t a, uint8x16_t t, uint8x8_t idx);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbx1_u8(uint8x8_t a, uint8x16_t t,
                                                                                      uint8x8_t idx) {
  return __builtin_mpl_vector_qtbx1_v8u8(a, t, idx);
}

uint8x16_t __builtin_mpl_vector_qtbx1q_v16u8(uint8x16_t a, uint8x16_t t, uint8x16_t idx);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbx1q_u8(uint8x16_t a, uint8x16_t t,
                                                                                        uint8x16_t idx) {
  return __builtin_mpl_vector_qtbx1q_v16u8(a, t, idx);
}

int8x8_t __builtin_mpl_vector_qtbx2_v8i8(int8x8_t a, int8x16x2_t t, uint8x8_t idx);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbx2_s8(int8x8_t a, int8x16x2_t t,
                                                                                     uint8x8_t idx) {
  return __builtin_mpl_vector_qtbx2_v8i8(a, t, idx);
}

int8x16_t __builtin_mpl_vector_qtbx2q_v16i8(int8x16_t a, int8x16x2_t t, uint8x16_t idx);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbx2q_s8(int8x16_t a, int8x16x2_t t,
                                                                                       uint8x16_t idx) {
  return __builtin_mpl_vector_qtbx2q_v16i8(a, t, idx);
}

uint8x8_t __builtin_mpl_vector_qtbx2_v8u8(uint8x8_t a, uint8x16x2_t t, uint8x8_t idx);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbx2_u8(uint8x8_t a, uint8x16x2_t t,
                                                                                      uint8x8_t idx) {
  return __builtin_mpl_vector_qtbx2_v8u8(a, t, idx);
}

uint8x16_t __builtin_mpl_vector_qtbx2q_v16u8(uint8x16_t a, uint8x16x2_t t, uint8x16_t idx);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbx2q_u8(uint8x16_t a, uint8x16x2_t t,
                                                                                        uint8x16_t idx) {
  return __builtin_mpl_vector_qtbx2q_v16u8(a, t, idx);
}

int8x8_t __builtin_mpl_vector_qtbx3_v8i8(int8x8_t a, int8x16x3_t t, uint8x8_t idx);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbx3_s8(int8x8_t a, int8x16x3_t t,
                                                                                     uint8x8_t idx) {
  return __builtin_mpl_vector_qtbx3_v8i8(a, t, idx);
}

int8x16_t __builtin_mpl_vector_qtbx3q_v16i8(int8x16_t a, int8x16x3_t t, uint8x16_t idx);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbx3q_s8(int8x16_t a, int8x16x3_t t,
                                                                                       uint8x16_t idx) {
  return __builtin_mpl_vector_qtbx3q_v16i8(a, t, idx);
}

uint8x8_t __builtin_mpl_vector_qtbx3_v8u8(uint8x8_t a, uint8x16x3_t t, uint8x8_t idx);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbx3_u8(uint8x8_t a, uint8x16x3_t t,
                                                                                      uint8x8_t idx) {
  return __builtin_mpl_vector_qtbx3_v8u8(a, t, idx);
}

uint8x16_t __builtin_mpl_vector_qtbx3q_v16u8(uint8x16_t a, uint8x16x3_t t, uint8x16_t idx);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbx3q_u8(uint8x16_t a, uint8x16x3_t t,
                                                                                        uint8x16_t idx) {
  return __builtin_mpl_vector_qtbx3q_v16u8(a, t, idx);
}

int8x8_t __builtin_mpl_vector_qtbx4_v8i8(int8x8_t a, int8x16x4_t t, uint8x8_t idx);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbx4_s8(int8x8_t a, int8x16x4_t t,
                                                                                     uint8x8_t idx) {
  return __builtin_mpl_vector_qtbx4_v8i8(a, t, idx);
}

int8x16_t __builtin_mpl_vector_qtbx4q_v16i8(int8x16_t a, int8x16x4_t t, uint8x16_t idx);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbx4q_s8(int8x16_t a, int8x16x4_t t,
                                                                                       uint8x16_t idx) {
  return __builtin_mpl_vector_qtbx4q_v16i8(a, t, idx);
}

uint8x8_t __builtin_mpl_vector_qtbx4_v8u8(uint8x8_t a, uint8x16x4_t t, uint8x8_t idx);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbx4_u8(uint8x8_t a, uint8x16x4_t t,
                                                                                      uint8x8_t idx) {
  return __builtin_mpl_vector_qtbx4_v8u8(a, t, idx);
}

uint8x16_t __builtin_mpl_vector_qtbx4q_v16u8(uint8x16_t a, uint8x16x4_t t, uint8x16_t idx);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqtbx4q_u8(uint8x16_t a, uint8x16x4_t t,
                                                                                        uint8x16_t idx) {
  return __builtin_mpl_vector_qtbx4q_v16u8(a, t, idx);
}

int8x8_t __builtin_mpl_vector_hadd_v8i8(int8x8_t a, int8x8_t b);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vhadd_s8(int8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_hadd_v8i8(a, b);
}

int8x16_t __builtin_mpl_vector_haddq_v16i8(int8x16_t a, int8x16_t b);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vhaddq_s8(int8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_haddq_v16i8(a, b);
}

int16x4_t __builtin_mpl_vector_hadd_v4i16(int16x4_t a, int16x4_t b);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vhadd_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_hadd_v4i16(a, b);
}

int16x8_t __builtin_mpl_vector_haddq_v8i16(int16x8_t a, int16x8_t b);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vhaddq_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_haddq_v8i16(a, b);
}

int32x2_t __builtin_mpl_vector_hadd_v2i32(int32x2_t a, int32x2_t b);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vhadd_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_hadd_v2i32(a, b);
}

int32x4_t __builtin_mpl_vector_haddq_v4i32(int32x4_t a, int32x4_t b);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vhaddq_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_haddq_v4i32(a, b);
}

uint8x8_t __builtin_mpl_vector_hadd_v8u8(uint8x8_t a, uint8x8_t b);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vhadd_u8(uint8x8_t a, uint8x8_t b) {
  return __builtin_mpl_vector_hadd_v8u8(a, b);
}

uint8x16_t __builtin_mpl_vector_haddq_v16u8(uint8x16_t a, uint8x16_t b);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vhaddq_u8(uint8x16_t a, uint8x16_t b) {
  return __builtin_mpl_vector_haddq_v16u8(a, b);
}

uint16x4_t __builtin_mpl_vector_hadd_v4u16(uint16x4_t a, uint16x4_t b);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vhadd_u16(uint16x4_t a, uint16x4_t b) {
  return __builtin_mpl_vector_hadd_v4u16(a, b);
}

uint16x8_t __builtin_mpl_vector_haddq_v8u16(uint16x8_t a, uint16x8_t b);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vhaddq_u16(uint16x8_t a, uint16x8_t b) {
  return __builtin_mpl_vector_haddq_v8u16(a, b);
}

uint32x2_t __builtin_mpl_vector_hadd_v2u32(uint32x2_t a, uint32x2_t b);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vhadd_u32(uint32x2_t a, uint32x2_t b) {
  return __builtin_mpl_vector_hadd_v2u32(a, b);
}

uint32x4_t __builtin_mpl_vector_haddq_v4u32(uint32x4_t a, uint32x4_t b);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vhaddq_u32(uint32x4_t a, uint32x4_t b) {
  return __builtin_mpl_vector_haddq_v4u32(a, b);
}

int8x8_t __builtin_mpl_vector_rhadd_v8i8(int8x8_t a, int8x8_t b);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrhadd_s8(int8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_rhadd_v8i8(a, b);
}

int8x16_t __builtin_mpl_vector_rhaddq_v16i8(int8x16_t a, int8x16_t b);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vrhaddq_s8(int8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_rhaddq_v16i8(a, b);
}

int16x4_t __builtin_mpl_vector_rhadd_v4i16(int16x4_t a, int16x4_t b);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrhadd_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_rhadd_v4i16(a, b);
}

int16x8_t __builtin_mpl_vector_rhaddq_v8i16(int16x8_t a, int16x8_t b);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrhaddq_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_rhaddq_v8i16(a, b);
}

int32x2_t __builtin_mpl_vector_rhadd_v2i32(int32x2_t a, int32x2_t b);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vrhadd_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_rhadd_v2i32(a, b);
}

int32x4_t __builtin_mpl_vector_rhaddq_v4i32(int32x4_t a, int32x4_t b);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrhaddq_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_rhaddq_v4i32(a, b);
}

uint8x8_t __builtin_mpl_vector_rhadd_v8u8(uint8x8_t a, uint8x8_t b);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrhadd_u8(uint8x8_t a, uint8x8_t b) {
  return __builtin_mpl_vector_rhadd_v8u8(a, b);
}

uint8x16_t __builtin_mpl_vector_rhaddq_v16u8(uint8x16_t a, uint8x16_t b);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vrhaddq_u8(uint8x16_t a, uint8x16_t b) {
  return __builtin_mpl_vector_rhaddq_v16u8(a, b);
}

uint16x4_t __builtin_mpl_vector_rhadd_v4u16(uint16x4_t a, uint16x4_t b);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrhadd_u16(uint16x4_t a, uint16x4_t b) {
  return __builtin_mpl_vector_rhadd_v4u16(a, b);
}

uint16x8_t __builtin_mpl_vector_rhaddq_v8u16(uint16x8_t a, uint16x8_t b);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrhaddq_u16(uint16x8_t a, uint16x8_t b) {
  return __builtin_mpl_vector_rhaddq_v8u16(a, b);
}

uint32x2_t __builtin_mpl_vector_rhadd_v2u32(uint32x2_t a, uint32x2_t b);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vrhadd_u32(uint32x2_t a, uint32x2_t b) {
  return __builtin_mpl_vector_rhadd_v2u32(a, b);
}

uint32x4_t __builtin_mpl_vector_rhaddq_v4u32(uint32x4_t a, uint32x4_t b);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrhaddq_u32(uint32x4_t a, uint32x4_t b) {
  return __builtin_mpl_vector_rhaddq_v4u32(a, b);
}

int8x8_t __builtin_mpl_vector_addhn_v8i8(int16x8_t a, int16x8_t b);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddhn_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_addhn_v8i8(a, b);
}

int16x4_t __builtin_mpl_vector_addhn_v4i16(int32x4_t a, int32x4_t b);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddhn_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_addhn_v4i16(a, b);
}

int32x2_t __builtin_mpl_vector_addhn_v2i32(int64x2_t a, int64x2_t b);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddhn_s64(int64x2_t a, int64x2_t b) {
  return __builtin_mpl_vector_addhn_v2i32(a, b);
}

uint8x8_t __builtin_mpl_vector_addhn_v8u8(uint16x8_t a, uint16x8_t b);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddhn_u16(uint16x8_t a, uint16x8_t b) {
  return __builtin_mpl_vector_addhn_v8u8(a, b);
}

uint16x4_t __builtin_mpl_vector_addhn_v4u16(uint32x4_t a, uint32x4_t b);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddhn_u32(uint32x4_t a, uint32x4_t b) {
  return __builtin_mpl_vector_addhn_v4u16(a, b);
}

uint32x2_t __builtin_mpl_vector_addhn_v2u32(uint64x2_t a, uint64x2_t b);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddhn_u64(uint64x2_t a, uint64x2_t b) {
  return __builtin_mpl_vector_addhn_v2u32(a, b);
}

int8x16_t __builtin_mpl_vector_addhn_high_v16i8(int8x8_t r, int16x8_t a, int16x8_t b);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddhn_high_s16(int8x8_t r, int16x8_t a,
                                                                                            int16x8_t b) {
  return __builtin_mpl_vector_addhn_high_v16i8(r, a, b);
}

int16x8_t __builtin_mpl_vector_addhn_high_v8i16(int16x4_t r, int32x4_t a, int32x4_t b);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddhn_high_s32(int16x4_t r, int32x4_t a,
                                                                                            int32x4_t b) {
  return __builtin_mpl_vector_addhn_high_v8i16(r, a, b);
}

int32x4_t __builtin_mpl_vector_addhn_high_v4i32(int32x2_t r, int64x2_t a, int64x2_t b);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddhn_high_s64(int32x2_t r, int64x2_t a,
                                                                                            int64x2_t b) {
  return __builtin_mpl_vector_addhn_high_v4i32(r, a, b);
}

uint8x16_t __builtin_mpl_vector_addhn_high_v16u8(uint8x8_t r, uint16x8_t a, uint16x8_t b);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddhn_high_u16(uint8x8_t r, uint16x8_t a,
                                                                                             uint16x8_t b) {
  return __builtin_mpl_vector_addhn_high_v16u8(r, a, b);
}

uint16x8_t __builtin_mpl_vector_addhn_high_v8u16(uint16x4_t r, uint32x4_t a, uint32x4_t b);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddhn_high_u32(uint16x4_t r, uint32x4_t a,
                                                                                             uint32x4_t b) {
  return __builtin_mpl_vector_addhn_high_v8u16(r, a, b);
}

uint32x4_t __builtin_mpl_vector_addhn_high_v4u32(uint32x2_t r, uint64x2_t a, uint64x2_t b);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddhn_high_u64(uint32x2_t r, uint64x2_t a,
                                                                                             uint64x2_t b) {
  return __builtin_mpl_vector_addhn_high_v4u32(r, a, b);
}

int8x8_t __builtin_mpl_vector_raddhn_v8i8(int16x8_t a, int16x8_t b);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vraddhn_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_raddhn_v8i8(a, b);
}

int16x4_t __builtin_mpl_vector_raddhn_v4i16(int32x4_t a, int32x4_t b);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vraddhn_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_raddhn_v4i16(a, b);
}

int32x2_t __builtin_mpl_vector_raddhn_v2i32(int64x2_t a, int64x2_t b);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vraddhn_s64(int64x2_t a, int64x2_t b) {
  return __builtin_mpl_vector_raddhn_v2i32(a, b);
}

uint8x8_t __builtin_mpl_vector_raddhn_v8u8(uint16x8_t a, uint16x8_t b);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vraddhn_u16(uint16x8_t a, uint16x8_t b) {
  return __builtin_mpl_vector_raddhn_v8u8(a, b);
}

uint16x4_t __builtin_mpl_vector_raddhn_v4u16(uint32x4_t a, uint32x4_t b);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vraddhn_u32(uint32x4_t a, uint32x4_t b) {
  return __builtin_mpl_vector_raddhn_v4u16(a, b);
}

uint32x2_t __builtin_mpl_vector_raddhn_v2u32(uint64x2_t a, uint64x2_t b);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vraddhn_u64(uint64x2_t a, uint64x2_t b) {
  return __builtin_mpl_vector_raddhn_v2u32(a, b);
}

int8x16_t __builtin_mpl_vector_raddhn_high_v16i8(int8x8_t r, int16x8_t a, int16x8_t b);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vraddhn_high_s16(int8x8_t r, int16x8_t a,
                                                                                             int16x8_t b) {
  return __builtin_mpl_vector_raddhn_high_v16i8(r, a, b);
}

int16x8_t __builtin_mpl_vector_raddhn_high_v8i16(int16x4_t r, int32x4_t a, int32x4_t b);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vraddhn_high_s32(int16x4_t r, int32x4_t a,
                                                                                             int32x4_t b) {
  return __builtin_mpl_vector_raddhn_high_v8i16(r, a, b);
}

int32x4_t __builtin_mpl_vector_raddhn_high_v4i32(int32x2_t r, int64x2_t a, int64x2_t b);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vraddhn_high_s64(int32x2_t r, int64x2_t a,
                                                                                             int64x2_t b) {
  return __builtin_mpl_vector_raddhn_high_v4i32(r, a, b);
}

uint8x16_t __builtin_mpl_vector_raddhn_high_v16u8(uint8x8_t r, uint16x8_t a, uint16x8_t b);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vraddhn_high_u16(uint8x8_t r, uint16x8_t a,
                                                                                              uint16x8_t b) {
  return __builtin_mpl_vector_raddhn_high_v16u8(r, a, b);
}

uint16x8_t __builtin_mpl_vector_raddhn_high_v8u16(uint16x4_t r, uint32x4_t a, uint32x4_t b);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vraddhn_high_u32(uint16x4_t r,
                                                                                              uint32x4_t a,
                                                                                              uint32x4_t b) {
  return __builtin_mpl_vector_raddhn_high_v8u16(r, a, b);
}

uint32x4_t __builtin_mpl_vector_raddhn_high_v4u32(uint32x2_t r, uint64x2_t a, uint64x2_t b);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vraddhn_high_u64(uint32x2_t r,
                                                                                              uint64x2_t a,
                                                                                              uint64x2_t b) {
  return __builtin_mpl_vector_raddhn_high_v4u32(r, a, b);
}

int8x8_t __builtin_mpl_vector_qadd_v8i8(int8x8_t a, int8x8_t b);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqadd_s8(int8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_qadd_v8i8(a, b);
}

int8x16_t __builtin_mpl_vector_qaddq_v16i8(int8x16_t a, int8x16_t b);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqaddq_s8(int8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_qaddq_v16i8(a, b);
}

int16x4_t __builtin_mpl_vector_qadd_v4i16(int16x4_t a, int16x4_t b);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqadd_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_qadd_v4i16(a, b);
}

int16x8_t __builtin_mpl_vector_qaddq_v8i16(int16x8_t a, int16x8_t b);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqaddq_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_qaddq_v8i16(a, b);
}

int32x2_t __builtin_mpl_vector_qadd_v2i32(int32x2_t a, int32x2_t b);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqadd_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_qadd_v2i32(a, b);
}

int32x4_t __builtin_mpl_vector_qaddq_v4i32(int32x4_t a, int32x4_t b);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqaddq_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_qaddq_v4i32(a, b);
}

int64x1_t __builtin_mpl_vector_qadd_v1i64(int64x1_t a, int64x1_t b);
extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vqadd_s64(int64x1_t a, int64x1_t b) {
  return __builtin_mpl_vector_qadd_v1i64(a, b);
}

int64x2_t __builtin_mpl_vector_qaddq_v2i64(int64x2_t a, int64x2_t b);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqaddq_s64(int64x2_t a, int64x2_t b) {
  return __builtin_mpl_vector_qaddq_v2i64(a, b);
}

uint8x8_t __builtin_mpl_vector_qadd_v8u8(uint8x8_t a, uint8x8_t b);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqadd_u8(uint8x8_t a, uint8x8_t b) {
  return __builtin_mpl_vector_qadd_v8u8(a, b);
}

uint8x16_t __builtin_mpl_vector_qaddq_v16u8(uint8x16_t a, uint8x16_t b);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqaddq_u8(uint8x16_t a, uint8x16_t b) {
  return __builtin_mpl_vector_qaddq_v16u8(a, b);
}

uint16x4_t __builtin_mpl_vector_qadd_v4u16(uint16x4_t a, uint16x4_t b);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqadd_u16(uint16x4_t a, uint16x4_t b) {
  return __builtin_mpl_vector_qadd_v4u16(a, b);
}

uint16x8_t __builtin_mpl_vector_qaddq_v8u16(uint16x8_t a, uint16x8_t b);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqaddq_u16(uint16x8_t a, uint16x8_t b) {
  return __builtin_mpl_vector_qaddq_v8u16(a, b);
}

uint32x2_t __builtin_mpl_vector_qadd_v2u32(uint32x2_t a, uint32x2_t b);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqadd_u32(uint32x2_t a, uint32x2_t b) {
  return __builtin_mpl_vector_qadd_v2u32(a, b);
}

uint32x4_t __builtin_mpl_vector_qaddq_v4u32(uint32x4_t a, uint32x4_t b);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqaddq_u32(uint32x4_t a, uint32x4_t b) {
  return __builtin_mpl_vector_qaddq_v4u32(a, b);
}

uint64x1_t __builtin_mpl_vector_qadd_v1u64(uint64x1_t a, uint64x1_t b);
extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vqadd_u64(uint64x1_t a, uint64x1_t b) {
  return __builtin_mpl_vector_qadd_v1u64(a, b);
}

uint64x2_t __builtin_mpl_vector_qaddq_v2u64(uint64x2_t a, uint64x2_t b);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqaddq_u64(uint64x2_t a, uint64x2_t b) {
  return __builtin_mpl_vector_qaddq_v2u64(a, b);
}

extern inline int8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqaddb_s8(int8_t a, int8_t b) {
  return vget_lane_s8(vqadd_s8((int8x8_t){a}, (int8x8_t){b}), 0);
}

extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqaddh_s16(int16_t a, int16_t b) {
  return vget_lane_s16(vqadd_s16((int16x4_t){a}, (int16x4_t){b}), 0);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqadds_s32(int32_t a, int32_t b) {
  return vget_lane_s32(vqadd_s32((int32x2_t){a}, (int32x2_t){b}), 0);
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vqaddd_s64(int64_t a, int64_t b) {
  return vget_lane_s64(vqadd_s64((int64x1_t){a}, (int64x1_t){b}), 0);
}

extern inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqaddb_u8(uint8_t a, uint8_t b) {
  return vget_lane_u8(vqadd_u8((uint8x8_t){a}, (uint8x8_t){b}), 0);
}

extern inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqaddh_u16(uint16_t a, uint16_t b) {
  return vget_lane_u16(vqadd_u16((uint16x4_t){a}, (uint16x4_t){b}), 0);
}

extern inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqadds_u32(uint32_t a, uint32_t b) {
  return vget_lane_u32(vqadd_u32((uint32x2_t){a}, (uint32x2_t){b}), 0);
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vqaddd_u64(uint64_t a, uint64_t b) {
  return vget_lane_u64(vqadd_u64((uint64x1_t){a}, (uint64x1_t){b}), 0);
}

int8x8_t __builtin_mpl_vector_uqadd_v8i8(int8x8_t a, uint8x8_t b);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vuqadd_s8(int8x8_t a, uint8x8_t b) {
  return __builtin_mpl_vector_uqadd_v8i8(a, b);
}

int8x16_t __builtin_mpl_vector_uqaddq_v16i8(int8x16_t a, uint8x16_t b);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vuqaddq_s8(int8x16_t a, uint8x16_t b) {
  return __builtin_mpl_vector_uqaddq_v16i8(a, b);
}

int16x4_t __builtin_mpl_vector_uqadd_v4i16(int16x4_t a, uint16x4_t b);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vuqadd_s16(int16x4_t a, uint16x4_t b) {
  return __builtin_mpl_vector_uqadd_v4i16(a, b);
}

int16x8_t __builtin_mpl_vector_uqaddq_v8i16(int16x8_t a, uint16x8_t b);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vuqaddq_s16(int16x8_t a, uint16x8_t b) {
  return __builtin_mpl_vector_uqaddq_v8i16(a, b);
}

int32x2_t __builtin_mpl_vector_uqadd_v2i32(int32x2_t a, uint32x2_t b);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vuqadd_s32(int32x2_t a, uint32x2_t b) {
  return __builtin_mpl_vector_uqadd_v2i32(a, b);
}

int32x4_t __builtin_mpl_vector_uqaddq_v4i32(int32x4_t a, uint32x4_t b);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vuqaddq_s32(int32x4_t a, uint32x4_t b) {
  return __builtin_mpl_vector_uqaddq_v4i32(a, b);
}

int64x1_t __builtin_mpl_vector_uqadd_v1i64(int64x1_t a, uint64x1_t b);
extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vuqadd_s64(int64x1_t a, uint64x1_t b) {
  return __builtin_mpl_vector_uqadd_v1i64(a, b);
}

int64x2_t __builtin_mpl_vector_uqaddq_v2i64(int64x2_t a, uint64x2_t b);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vuqaddq_s64(int64x2_t a, uint64x2_t b) {
  return __builtin_mpl_vector_uqaddq_v2i64(a, b);
}

extern inline int8_t __attribute__ ((__always_inline__, __gnu_inline__)) vuqaddb_s8(int8_t a, int8_t b) {
  return vget_lane_s8(vuqadd_s8((int8x8_t){a}, (int8x8_t){b}), 0);
}

extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vuqaddh_s16(int16_t a, int16_t b) {
  return vget_lane_s16(vuqadd_s16((int16x4_t){a}, (int16x4_t){b}), 0);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vuqadds_s32(int32_t a, int32_t b) {
  return vget_lane_s32(vuqadd_s32((int32x2_t){a}, (int32x2_t){b}), 0);
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vuqaddd_s64(int64_t a, int64_t b) {
  return vget_lane_s64(vuqadd_s64((int64x1_t){a}, (int64x1_t){b}), 0);
}

uint8x8_t __builtin_mpl_vector_sqadd_v8u8(uint8x8_t a, int8x8_t b);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsqadd_u8(uint8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_sqadd_v8u8(a, b);
}

uint8x16_t __builtin_mpl_vector_sqaddq_v16u8(uint8x16_t a, int8x16_t b);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vsqaddq_u8(uint8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_sqaddq_v16u8(a, b);
}

uint16x4_t __builtin_mpl_vector_sqadd_v4u16(uint16x4_t a, int16x4_t b);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsqadd_u16(uint16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_sqadd_v4u16(a, b);
}

uint16x8_t __builtin_mpl_vector_sqaddq_v8u16(uint16x8_t a, int16x8_t b);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsqaddq_u16(uint16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_sqaddq_v8u16(a, b);
}

uint32x2_t __builtin_mpl_vector_sqadd_v2u32(uint32x2_t a, int32x2_t b);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsqadd_u32(uint32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_sqadd_v2u32(a, b);
}

uint32x4_t __builtin_mpl_vector_sqaddq_v4u32(uint32x4_t a, int32x4_t b);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsqaddq_u32(uint32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_sqaddq_v4u32(a, b);
}

uint64x1_t __builtin_mpl_vector_sqadd_v1u64(uint64x1_t a, int64x1_t b);
extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vsqadd_u64(uint64x1_t a, int64x1_t b) {
  return __builtin_mpl_vector_sqadd_v1u64(a, b);
}

uint64x2_t __builtin_mpl_vector_sqaddq_v2u64(uint64x2_t a, int64x2_t b);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsqaddq_u64(uint64x2_t a, int64x2_t b) {
  return __builtin_mpl_vector_sqaddq_v2u64(a, b);
}

extern inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsqaddb_u8(uint8_t a, uint8_t b) {
  return vget_lane_u8(vsqadd_u8((uint8x8_t){a}, (uint8x8_t){b}), 0);
}

extern inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__)) vsqaddh_u16(uint16_t a, uint16_t b) {
  return vget_lane_u16(vsqadd_u16((uint16x4_t){a}, (uint16x4_t){b}), 0);
}

extern inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__)) vsqadds_u32(uint32_t a, uint32_t b) {
  return vget_lane_u32(vsqadd_u32((uint32x2_t){a}, (uint32x2_t){b}), 0);
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vsqaddd_u64(uint64_t a, uint64_t b) {
  return vget_lane_u64(vsqadd_u64((uint64x1_t){a}, (uint64x1_t){b}), 0);
}

int8x8_t __builtin_mpl_vector_mla_v8i8(int8x8_t a, int8x8_t b, int8x8_t c);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmla_s8(int8x8_t a, int8x8_t b, int8x8_t c) {
  return __builtin_mpl_vector_mla_v8i8(a, b, c);
}

int8x16_t __builtin_mpl_vector_mlaq_v16i8(int8x16_t a, int8x16_t b, int8x16_t c);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlaq_s8(int8x16_t a, int8x16_t b,
                                                                                     int8x16_t c) {
  return __builtin_mpl_vector_mlaq_v16i8(a, b, c);
}

int16x4_t __builtin_mpl_vector_mla_v4i16(int16x4_t a, int16x4_t b, int16x4_t c);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmla_s16(int16x4_t a, int16x4_t b,
                                                                                     int16x4_t c) {
  return __builtin_mpl_vector_mla_v4i16(a, b, c);
}

int16x8_t __builtin_mpl_vector_mlaq_v8i16(int16x8_t a, int16x8_t b, int16x8_t c);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlaq_s16(int16x8_t a, int16x8_t b,
                                                                                      int16x8_t c) {
  return __builtin_mpl_vector_mlaq_v8i16(a, b, c);
}

int32x2_t __builtin_mpl_vector_mla_v2i32(int32x2_t a, int32x2_t b, int32x2_t c);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmla_s32(int32x2_t a, int32x2_t b,
                                                                                     int32x2_t c) {
  return __builtin_mpl_vector_mla_v2i32(a, b, c);
}

int32x4_t __builtin_mpl_vector_mlaq_v4i32(int32x4_t a, int32x4_t b, int32x4_t c);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlaq_s32(int32x4_t a, int32x4_t b,
                                                                                      int32x4_t c) {
  return __builtin_mpl_vector_mlaq_v4i32(a, b, c);
}

uint8x8_t __builtin_mpl_vector_mla_v8u8(uint8x8_t a, uint8x8_t b, uint8x8_t c);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmla_u8(uint8x8_t a, uint8x8_t b,
                                                                                    uint8x8_t c) {
  return __builtin_mpl_vector_mla_v8u8(a, b, c);
}

uint8x16_t __builtin_mpl_vector_mlaq_v16u8(uint8x16_t a, uint8x16_t b, uint8x16_t c);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlaq_u8(uint8x16_t a, uint8x16_t b,
                                                                                      uint8x16_t c) {
  return __builtin_mpl_vector_mlaq_v16u8(a, b, c);
}

uint16x4_t __builtin_mpl_vector_mla_v4u16(uint16x4_t a, uint16x4_t b, uint16x4_t c);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmla_u16(uint16x4_t a, uint16x4_t b,
                                                                                      uint16x4_t c) {
  return __builtin_mpl_vector_mla_v4u16(a, b, c);
}

uint16x8_t __builtin_mpl_vector_mlaq_v8u16(uint16x8_t a, uint16x8_t b, uint16x8_t c);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlaq_u16(uint16x8_t a, uint16x8_t b,
                                                                                       uint16x8_t c) {
  return __builtin_mpl_vector_mlaq_v8u16(a, b, c);
}

uint32x2_t __builtin_mpl_vector_mla_v2u32(uint32x2_t a, uint32x2_t b, uint32x2_t c);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmla_u32(uint32x2_t a, uint32x2_t b,
                                                                                      uint32x2_t c) {
  return __builtin_mpl_vector_mla_v2u32(a, b, c);
}

uint32x4_t __builtin_mpl_vector_mlaq_v4u32(uint32x4_t a, uint32x4_t b, uint32x4_t c);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlaq_u32(uint32x4_t a, uint32x4_t b,
                                                                                       uint32x4_t c) {
  return __builtin_mpl_vector_mlaq_v4u32(a, b, c);
}

int8x8_t __builtin_mpl_vector_mls_v8i8(int8x8_t a, int8x8_t b, int8x8_t c);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmls_s8(int8x8_t a, int8x8_t b, int8x8_t c) {
  return __builtin_mpl_vector_mls_v8i8(a, b, c);
}

int8x16_t __builtin_mpl_vector_mlsq_v16i8(int8x16_t a, int8x16_t b, int8x16_t c);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsq_s8(int8x16_t a, int8x16_t b,
                                                                                     int8x16_t c) {
  return __builtin_mpl_vector_mlsq_v16i8(a, b, c);
}

int16x4_t __builtin_mpl_vector_mls_v4i16(int16x4_t a, int16x4_t b, int16x4_t c);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmls_s16(int16x4_t a, int16x4_t b,
                                                                                     int16x4_t c) {
  return __builtin_mpl_vector_mls_v4i16(a, b, c);
}

int16x8_t __builtin_mpl_vector_mlsq_v8i16(int16x8_t a, int16x8_t b, int16x8_t c);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsq_s16(int16x8_t a, int16x8_t b,
                                                                                      int16x8_t c) {
  return __builtin_mpl_vector_mlsq_v8i16(a, b, c);
}

int32x2_t __builtin_mpl_vector_mls_v2i32(int32x2_t a, int32x2_t b, int32x2_t c);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmls_s32(int32x2_t a, int32x2_t b,
                                                                                     int32x2_t c) {
  return __builtin_mpl_vector_mls_v2i32(a, b, c);
}

int32x4_t __builtin_mpl_vector_mlsq_v4i32(int32x4_t a, int32x4_t b, int32x4_t c);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsq_s32(int32x4_t a, int32x4_t b,
                                                                                      int32x4_t c) {
  return __builtin_mpl_vector_mlsq_v4i32(a, b, c);
}

uint8x8_t __builtin_mpl_vector_mls_v8u8(uint8x8_t a, uint8x8_t b, uint8x8_t c);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmls_u8(uint8x8_t a, uint8x8_t b,
                                                                                    uint8x8_t c) {
  return __builtin_mpl_vector_mls_v8u8(a, b, c);
}

uint8x16_t __builtin_mpl_vector_mlsq_v16u8(uint8x16_t a, uint8x16_t b, uint8x16_t c);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsq_u8(uint8x16_t a, uint8x16_t b,
                                                                                      uint8x16_t c) {
  return __builtin_mpl_vector_mlsq_v16u8(a, b, c);
}

uint16x4_t __builtin_mpl_vector_mls_v4u16(uint16x4_t a, uint16x4_t b, uint16x4_t c);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmls_u16(uint16x4_t a, uint16x4_t b,
                                                                                      uint16x4_t c) {
  return __builtin_mpl_vector_mls_v4u16(a, b, c);
}

uint16x8_t __builtin_mpl_vector_mlsq_v8u16(uint16x8_t a, uint16x8_t b, uint16x8_t c);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsq_u16(uint16x8_t a, uint16x8_t b,
                                                                                       uint16x8_t c) {
  return __builtin_mpl_vector_mlsq_v8u16(a, b, c);
}

uint32x2_t __builtin_mpl_vector_mls_v2u32(uint32x2_t a, uint32x2_t b, uint32x2_t c);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmls_u32(uint32x2_t a, uint32x2_t b,
                                                                                      uint32x2_t c) {
  return __builtin_mpl_vector_mls_v2u32(a, b, c);
}

uint32x4_t __builtin_mpl_vector_mlsq_v4u32(uint32x4_t a, uint32x4_t b, uint32x4_t c);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsq_u32(uint32x4_t a, uint32x4_t b,
                                                                                       uint32x4_t c) {
  return __builtin_mpl_vector_mlsq_v4u32(a, b, c);
}

int16x8_t __builtin_mpl_vector_mlal_v8i16(int16x8_t a, int8x8_t b, int8x8_t c);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_s8(int16x8_t a, int8x8_t b,
                                                                                     int8x8_t c) {
  return __builtin_mpl_vector_mlal_v8i16(a, b, c);
}

int32x4_t __builtin_mpl_vector_mlal_v4i32(int32x4_t a, int16x4_t b, int16x4_t c);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_s16(int32x4_t a, int16x4_t b,
                                                                                      int16x4_t c) {
  return __builtin_mpl_vector_mlal_v4i32(a, b, c);
}

int64x2_t __builtin_mpl_vector_mlal_v2i64(int64x2_t a, int32x2_t b, int32x2_t c);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_s32(int64x2_t a, int32x2_t b,
                                                                                      int32x2_t c) {
  return __builtin_mpl_vector_mlal_v2i64(a, b, c);
}

uint16x8_t __builtin_mpl_vector_mlal_v8u16(uint16x8_t a, uint8x8_t b, uint8x8_t c);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_u8(uint16x8_t a, uint8x8_t b,
                                                                                      uint8x8_t c) {
  return __builtin_mpl_vector_mlal_v8u16(a, b, c);
}

uint32x4_t __builtin_mpl_vector_mlal_v4u32(uint32x4_t a, uint16x4_t b, uint16x4_t c);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_u16(uint32x4_t a, uint16x4_t b,
                                                                                       uint16x4_t c) {
  return __builtin_mpl_vector_mlal_v4u32(a, b, c);
}

uint64x2_t __builtin_mpl_vector_mlal_v2u64(uint64x2_t a, uint32x2_t b, uint32x2_t c);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_u32(uint64x2_t a, uint32x2_t b,
                                                                                       uint32x2_t c) {
  return __builtin_mpl_vector_mlal_v2u64(a, b, c);
}

int16x8_t __builtin_mpl_vector_mlal_high_v8i16(int16x8_t a, int8x16_t b, int8x16_t c);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_high_s8(int16x8_t a, int8x16_t b,
                                                                                          int8x16_t c) {
  return __builtin_mpl_vector_mlal_high_v8i16(a, b, c);
}

int32x4_t __builtin_mpl_vector_mlal_high_v4i32(int32x4_t a, int16x8_t b, int16x8_t c);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_high_s16(int32x4_t a, int16x8_t b,
                                                                                           int16x8_t c) {
  return __builtin_mpl_vector_mlal_high_v4i32(a, b, c);
}

int64x2_t __builtin_mpl_vector_mlal_high_v2i64(int64x2_t a, int32x4_t b, int32x4_t c);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_high_s32(int64x2_t a, int32x4_t b,
                                                                                           int32x4_t c) {
  return __builtin_mpl_vector_mlal_high_v2i64(a, b, c);
}

uint16x8_t __builtin_mpl_vector_mlal_high_v8u16(uint16x8_t a, uint8x16_t b, uint8x16_t c);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_high_u8(uint16x8_t a, uint8x16_t b,
                                                                                           uint8x16_t c) {
  return __builtin_mpl_vector_mlal_high_v8u16(a, b, c);
}

uint32x4_t __builtin_mpl_vector_mlal_high_v4u32(uint32x4_t a, uint16x8_t b, uint16x8_t c);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_high_u16(uint32x4_t a, uint16x8_t b,
                                                                                            uint16x8_t c) {
  return __builtin_mpl_vector_mlal_high_v4u32(a, b, c);
}

uint64x2_t __builtin_mpl_vector_mlal_high_v2u64(uint64x2_t a, uint32x4_t b, uint32x4_t c);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_high_u32(uint64x2_t a, uint32x4_t b,
                                                                                            uint32x4_t c) {
  return __builtin_mpl_vector_mlal_high_v2u64(a, b, c);
}

int16x8_t __builtin_mpl_vector_mlsl_v8i16(int16x8_t a, int8x8_t b, int8x8_t c);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_s8(int16x8_t a, int8x8_t b,
                                                                                     int8x8_t c) {
  return __builtin_mpl_vector_mlsl_v8i16(a, b, c);
}

int32x4_t __builtin_mpl_vector_mlsl_v4i32(int32x4_t a, int16x4_t b, int16x4_t c);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_s16(int32x4_t a, int16x4_t b,
                                                                                      int16x4_t c) {
  return __builtin_mpl_vector_mlsl_v4i32(a, b, c);
}

int64x2_t __builtin_mpl_vector_mlsl_v2i64(int64x2_t a, int32x2_t b, int32x2_t c);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_s32(int64x2_t a, int32x2_t b,
                                                                                      int32x2_t c) {
  return __builtin_mpl_vector_mlsl_v2i64(a, b, c);
}

uint16x8_t __builtin_mpl_vector_mlsl_v8u16(uint16x8_t a, uint8x8_t b, uint8x8_t c);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_u8(uint16x8_t a, uint8x8_t b,
                                                                                      uint8x8_t c) {
  return __builtin_mpl_vector_mlsl_v8u16(a, b, c);
}

uint32x4_t __builtin_mpl_vector_mlsl_v4u32(uint32x4_t a, uint16x4_t b, uint16x4_t c);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_u16(uint32x4_t a, uint16x4_t b,
                                                                                       uint16x4_t c) {
  return __builtin_mpl_vector_mlsl_v4u32(a, b, c);
}

uint64x2_t __builtin_mpl_vector_mlsl_v2u64(uint64x2_t a, uint32x2_t b, uint32x2_t c);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_u32(uint64x2_t a, uint32x2_t b,
                                                                                       uint32x2_t c) {
  return __builtin_mpl_vector_mlsl_v2u64(a, b, c);
}

int16x8_t __builtin_mpl_vector_mlsl_high_v8i16(int16x8_t a, int8x16_t b, int8x16_t c);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_high_s8(int16x8_t a, int8x16_t b,
                                                                                          int8x16_t c) {
  return __builtin_mpl_vector_mlsl_high_v8i16(a, b, c);
}

int32x4_t __builtin_mpl_vector_mlsl_high_v4i32(int32x4_t a, int16x8_t b, int16x8_t c);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_high_s16(int32x4_t a, int16x8_t b,
                                                                                           int16x8_t c) {
  return __builtin_mpl_vector_mlsl_high_v4i32(a, b, c);
}

int64x2_t __builtin_mpl_vector_mlsl_high_v2i64(int64x2_t a, int32x4_t b, int32x4_t c);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_high_s32(int64x2_t a, int32x4_t b,
                                                                                           int32x4_t c) {
  return __builtin_mpl_vector_mlsl_high_v2i64(a, b, c);
}

uint16x8_t __builtin_mpl_vector_mlsl_high_v8u16(uint16x8_t a, uint8x16_t b, uint8x16_t c);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_high_u8(uint16x8_t a, uint8x16_t b,
                                                                                           uint8x16_t c) {
  return __builtin_mpl_vector_mlsl_high_v8u16(a, b, c);
}

uint32x4_t __builtin_mpl_vector_mlsl_high_v4u32(uint32x4_t a, uint16x8_t b, uint16x8_t c);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_high_u16(uint32x4_t a, uint16x8_t b,
                                                                                            uint16x8_t c) {
  return __builtin_mpl_vector_mlsl_high_v4u32(a, b, c);
}

uint64x2_t __builtin_mpl_vector_mlsl_high_v2u64(uint64x2_t a, uint32x4_t b, uint32x4_t c);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_high_u32(uint64x2_t a, uint32x4_t b,
                                                                                            uint32x4_t c) {
  return __builtin_mpl_vector_mlsl_high_v2u64(a, b, c);
}

int16x4_t __builtin_mpl_vector_qdmulh_v4i16(int16x4_t a, int16x4_t b);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmulh_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_qdmulh_v4i16(a, b);
}

int16x8_t __builtin_mpl_vector_qdmulhq_v8i16(int16x8_t a, int16x8_t b);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmulhq_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_qdmulhq_v8i16(a, b);
}

int32x2_t __builtin_mpl_vector_qdmulh_v2i32(int32x2_t a, int32x2_t b);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmulh_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_qdmulh_v2i32(a, b);
}

int32x4_t __builtin_mpl_vector_qdmulhq_v4i32(int32x4_t a, int32x4_t b);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmulhq_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_qdmulhq_v4i32(a, b);
}

extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmulhh_s16(int16_t a, int16_t b) {
  return vget_lane_s16(vqdmulh_s16((int16x4_t){a}, (int16x4_t){b}), 0);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmulhs_s32(int32_t a, int32_t b) {
  return vget_lane_s32(vqdmulh_s32((int32x2_t){a}, (int32x2_t){b}), 0);
}

int16x4_t __builtin_mpl_vector_qrdmulh_v4i16(int16x4_t a, int16x4_t b);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrdmulh_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_qrdmulh_v4i16(a, b);
}

int16x8_t __builtin_mpl_vector_qrdmulhq_v8i16(int16x8_t a, int16x8_t b);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrdmulhq_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_qrdmulhq_v8i16(a, b);
}

int32x2_t __builtin_mpl_vector_qrdmulh_v2i32(int32x2_t a, int32x2_t b);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrdmulh_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_qrdmulh_v2i32(a, b);
}

int32x4_t __builtin_mpl_vector_qrdmulhq_v4i32(int32x4_t a, int32x4_t b);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrdmulhq_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_qrdmulhq_v4i32(a, b);
}

extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrdmulhh_s16(int16_t a, int16_t b) {
  return vget_lane_s16(vqrdmulh_s16((int16x4_t){a}, (int16x4_t){b}), 0);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrdmulhs_s32(int32_t a, int32_t b) {
  return vget_lane_s32(vqrdmulh_s32((int32x2_t){a}, (int32x2_t){b}), 0);
}

int32x4_t __builtin_mpl_vector_qdmull_v4i32(int16x4_t a, int16x4_t b);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmull_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_qdmull_v4i32(a, b);
}

int64x2_t __builtin_mpl_vector_qdmull_v2i64(int32x2_t a, int32x2_t b);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmull_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_qdmull_v2i64(a, b);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmullh_s16(int16_t a, int16_t b) {
  return vgetq_lane_s32(vqdmull_s16((int16x4_t){a}, (int16x4_t){b}), 0);
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmulls_s32(int32_t a, int32_t b) {
  return vgetq_lane_s64(vqdmull_s32((int32x2_t){a}, (int32x2_t){b}), 0);
}

int32x4_t __builtin_mpl_vector_qdmull_high_v4i32(int16x8_t a, int16x8_t b);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmull_high_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_qdmull_high_v4i32(a, b);
}

int64x2_t __builtin_mpl_vector_qdmull_high_v2i64(int32x4_t a, int32x4_t b);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmull_high_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_qdmull_high_v2i64(a, b);
}

int32x4_t __builtin_mpl_vector_qdmlal_v4i32(int32x4_t a, int16x4_t b, int16x4_t c);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlal_s16(int32x4_t a, int16x4_t b,
                                                                                        int16x4_t c) {
  return __builtin_mpl_vector_qdmlal_v4i32(a, b, c);
}

int64x2_t __builtin_mpl_vector_qdmlal_v2i64(int64x2_t a, int32x2_t b, int32x2_t c);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlal_s32(int64x2_t a, int32x2_t b,
                                                                                        int32x2_t c) {
  return __builtin_mpl_vector_qdmlal_v2i64(a, b, c);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlalh_s16(int32_t a, int16_t b,
                                                                                       int16_t c) {
  return vgetq_lane_s32(vqdmlal_s16((int32x4_t){a}, (int16x4_t){b}, (int16x4_t){c}), 0);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlals_s32(int32_t a, int16_t b,
                                                                                       int16_t c) {
  return vgetq_lane_s64(vqdmlal_s32((int64x2_t){a}, (int32x2_t){b}, (int32x2_t){c}), 0);
}

int32x4_t __builtin_mpl_vector_qdmlal_high_v4i32(int32x4_t a, int16x8_t b, int16x8_t c);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlal_high_s16(int32x4_t a, int16x8_t b,
                                                                                             int16x8_t c) {
  return __builtin_mpl_vector_qdmlal_high_v4i32(a, b, c);
}

int64x2_t __builtin_mpl_vector_qdmlal_high_v2i64(int64x2_t a, int32x4_t b, int32x4_t c);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlal_high_s32(int64x2_t a, int32x4_t b,
                                                                                             int32x4_t c) {
  return __builtin_mpl_vector_qdmlal_high_v2i64(a, b, c);
}

int32x4_t __builtin_mpl_vector_qdmlsl_v4i32(int32x4_t a, int16x4_t b, int16x4_t c);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlsl_s16(int32x4_t a, int16x4_t b,
                                                                                        int16x4_t c) {
  return __builtin_mpl_vector_qdmlsl_v4i32(a, b, c);
}

int64x2_t __builtin_mpl_vector_qdmlsl_v2i64(int64x2_t a, int32x2_t b, int32x2_t c);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlsl_s32(int64x2_t a, int32x2_t b,
                                                                                        int32x2_t c) {
  return __builtin_mpl_vector_qdmlsl_v2i64(a, b, c);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlslh_s16(int32_t a, int16_t b,
                                                                                       int16_t c) {
  return vgetq_lane_s32(vqdmlsl_s16((int32x4_t){a}, (int16x4_t){b}, (int16x4_t){c}), 0);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlsls_s32(int32_t a, int16_t b,
                                                                                       int16_t c) {
  return vgetq_lane_s64(vqdmlsl_s32((int64x2_t){a}, (int32x2_t){b}, (int32x2_t){c}), 0);
}

int32x4_t __builtin_mpl_vector_qdmlsl_high_v4i32(int32x4_t a, int16x8_t b, int16x8_t c);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlsl_high_s16(int32x4_t a, int16x8_t b,
                                                                                             int16x8_t c) {
  return __builtin_mpl_vector_qdmlsl_high_v4i32(a, b, c);
}

int64x2_t __builtin_mpl_vector_qdmlsl_high_v2i64(int64x2_t a, int32x4_t b, int32x4_t c);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlsl_high_s32(int64x2_t a, int32x4_t b,
                                                                                             int32x4_t c) {
  return __builtin_mpl_vector_qdmlsl_high_v2i64(a, b, c);
}

int32x4_t __builtin_mpl_vector_qdmlal_lane_v4i32(int32x4_t a, int16x4_t b, int16x4_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlal_lane_s16(int32x4_t a, int16x4_t b,
                                                                                             int16x4_t v,
                                                                                             const int lane) {
  return __builtin_mpl_vector_qdmlal_lane_v4i32(a, b, v, lane);
}

int64x2_t __builtin_mpl_vector_qdmlal_lane_v2i64(int64x2_t a, int32x2_t b, int32x2_t v, const int lane);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlal_lane_s32(int64x2_t a, int32x2_t b,
                                                                                             int32x2_t v,
                                                                                             const int lane) {
  return __builtin_mpl_vector_qdmlal_lane_v2i64(a, b, v, lane);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlalh_lane_s16(int32_t a, int16_t b,
                                                                                            int16x4_t v,
                                                                                            const int lane) {
  return vgetq_lane_s32(vqdmlal_lane_s16((int32x4_t){a}, (int16x4_t){b}, v, lane), 0);
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlals_lane_s32(int64_t a, int32_t b,
                                                                                            int32x2_t v,
                                                                                            const int lane) {
  return vgetq_lane_s64(vqdmlal_lane_s32((int64x2_t){a}, (int32x2_t){b}, v, lane), 0);
}

int32x4_t __builtin_mpl_vector_qdmlal_high_lane_v4i32(int32x4_t a, int16x8_t b, int16x4_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlal_high_lane_s16(int32x4_t a,
                                                                                                  int16x8_t b,
                                                                                                  int16x4_t v,
                                                                                                  const int lane) {
  return __builtin_mpl_vector_qdmlal_high_lane_v4i32(a, b, v, lane);
}

int64x2_t __builtin_mpl_vector_qdmlal_high_lane_v2i64(int64x2_t a, int32x4_t b, int32x2_t v, const int lane);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlal_high_lane_s32(int64x2_t a,
                                                                                                  int32x4_t b,
                                                                                                  int32x2_t v,
                                                                                                  const int lane) {
  return __builtin_mpl_vector_qdmlal_high_lane_v2i64(a, b, v, lane);
}

int32x4_t __builtin_mpl_vector_qdmlal_laneq_v4i32(int32x4_t a, int16x4_t b, int16x8_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlal_laneq_s16(int32x4_t a, int16x4_t b,
                                                                                              int16x8_t v,
                                                                                              const int lane) {
  return __builtin_mpl_vector_qdmlal_laneq_v4i32(a, b, v, lane);
}

int64x2_t __builtin_mpl_vector_qdmlal_laneq_v2i64(int64x2_t a, int32x2_t b, int32x4_t v, const int lane);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlal_laneq_s32(int64x2_t a, int32x2_t b,
                                                                                              int32x4_t v,
                                                                                              const int lane) {
  return __builtin_mpl_vector_qdmlal_laneq_v2i64(a, b, v, lane);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlalh_laneq_s16(int32_t a, int16_t b,
                                                                                             int16x8_t v,
                                                                                             const int lane) {
  return vgetq_lane_s32(vqdmlal_laneq_s16((int32x4_t){a}, (int16x4_t){b}, v, lane), 0);
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlals_laneq_s32(int64_t a, int32_t b,
                                                                                             int32x4_t v,
                                                                                             const int lane) {
  return vgetq_lane_s64(vqdmlal_laneq_s32((int64x2_t){a}, (int32x2_t){b}, v, lane), 0);
}

int32x4_t __builtin_mpl_vector_qdmlal_high_laneq_v4i32(int32x4_t a, int16x8_t b, int16x8_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlal_high_laneq_s16(int32x4_t a,
                                                                                                   int16x8_t b,
                                                                                                   int16x8_t v,
                                                                                                   const int lane) {
  return __builtin_mpl_vector_qdmlal_high_laneq_v4i32(a, b, v, lane);
}

int64x2_t __builtin_mpl_vector_qdmlal_high_laneq_v2i64(int64x2_t a, int32x4_t b, int32x4_t v, const int lane);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlal_high_laneq_s32(int64x2_t a,
                                                                                                   int32x4_t b,
                                                                                                   int32x4_t v,
                                                                                                   const int lane) {
  return __builtin_mpl_vector_qdmlal_high_laneq_v2i64(a, b, v, lane);
}

int32x4_t __builtin_mpl_vector_qdmlsl_lane_v4i32(int32x4_t a, int16x4_t b, int16x4_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlsl_lane_s16(int32x4_t a, int16x4_t b,
                                                                                             int16x4_t v,
                                                                                             const int lane) {
  return __builtin_mpl_vector_qdmlsl_lane_v4i32(a, b, v, lane);
}

int64x2_t __builtin_mpl_vector_qdmlsl_lane_v2i64(int64x2_t a, int32x2_t b, int32x2_t v, const int lane);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlsl_lane_s32(int64x2_t a, int32x2_t b,
                                                                                             int32x2_t v,
                                                                                             const int lane) {
  return __builtin_mpl_vector_qdmlsl_lane_v2i64(a, b, v, lane);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlslh_lane_s16(int32_t a, int16_t b,
                                                                                            int16x4_t v,
                                                                                            const int lane) {
  return vgetq_lane_s32(vqdmlsl_lane_s16((int32x4_t){a}, (int16x4_t){b}, v, lane), 0);
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlsls_lane_s32(int64_t a, int32_t b,
                                                                                            int32x2_t v,
                                                                                            const int lane) {
  return vgetq_lane_s64(vqdmlsl_lane_s32((int64x2_t){a}, (int32x2_t){b}, v, lane), 0);
}

int32x4_t __builtin_mpl_vector_qdmlsl_high_lane_v4i32(int32x4_t a, int16x8_t b, int16x4_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlsl_high_lane_s16(int32x4_t a,
                                                                                                  int16x8_t b,
                                                                                                  int16x4_t v,
                                                                                                  const int lane) {
  return __builtin_mpl_vector_qdmlsl_high_lane_v4i32(a, b, v, lane);
}

int64x2_t __builtin_mpl_vector_qdmlsl_high_lane_v2i64(int64x2_t a, int32x4_t b, int32x2_t v, const int lane);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlsl_high_lane_s32(int64x2_t a,
                                                                                                  int32x4_t b,
                                                                                                  int32x2_t v,
                                                                                                  const int lane) {
  return __builtin_mpl_vector_qdmlsl_high_lane_v2i64(a, b, v, lane);
}

int32x4_t __builtin_mpl_vector_qdmlsl_laneq_v4i32(int32x4_t a, int16x4_t b, int16x8_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlsl_laneq_s16(int32x4_t a, int16x4_t b,
                                                                                              int16x8_t v,
                                                                                              const int lane) {
  return __builtin_mpl_vector_qdmlsl_laneq_v4i32(a, b, v, lane);
}

int64x2_t __builtin_mpl_vector_qdmlsl_laneq_v2i64(int64x2_t a, int32x2_t b, int32x4_t v, const int lane);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlsl_laneq_s32(int64x2_t a, int32x2_t b,
                                                                                              int32x4_t v,
                                                                                              const int lane) {
  return __builtin_mpl_vector_qdmlsl_laneq_v2i64(a, b, v, lane);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlslh_laneq_s16(int32_t a, int16_t b,
                                                                                             int16x8_t v,
                                                                                             const int lane) {
  return vgetq_lane_s32(vqdmlsl_laneq_s16((int32x4_t){a}, (int16x4_t){b}, v, lane), 0);
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlsls_laneq_s32(int64_t a, int32_t b,
                                                                                             int32x4_t v,
                                                                                             const int lane) {
  return vgetq_lane_s64(vqdmlsl_laneq_s32((int64x2_t){a}, (int32x2_t){b}, v, lane), 0);
}

int32x4_t __builtin_mpl_vector_qdmlsl_high_laneq_v4i32(int32x4_t a, int16x8_t b, int16x8_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlsl_high_laneq_s16(int32x4_t a,
                                                                                                   int16x8_t b,
                                                                                                   int16x8_t v,
                                                                                                   const int lane) {
  return __builtin_mpl_vector_qdmlsl_high_laneq_v4i32(a, b, v, lane);
}

int64x2_t __builtin_mpl_vector_qdmlsl_high_laneq_v2i64(int64x2_t a, int32x4_t b, int32x4_t v, const int lane);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlsl_high_laneq_s32(int64x2_t a,
                                                                                                   int32x4_t b,
                                                                                                   int32x4_t v,
                                                                                                   const int lane) {
  return __builtin_mpl_vector_qdmlsl_high_laneq_v2i64(a, b, v, lane);
}

int32x4_t __builtin_mpl_vector_qdmull_n_v4i32(int16x4_t a, int16x4_t b);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmull_n_s16(int16x4_t a, int16_t b) {
  return __builtin_mpl_vector_qdmull_n_v4i32(a, (int16x4_t){b});
}

int64x2_t __builtin_mpl_vector_qdmull_n_v2i64(int32x2_t a, int32x2_t b);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmull_n_s32(int32x2_t a, int32_t b) {
  return __builtin_mpl_vector_qdmull_n_v2i64(a, (int32x2_t){b});
}

int32x4_t __builtin_mpl_vector_qdmull_high_n_v4i32(int16x8_t a, int16x8_t b);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmull_high_n_s16(int16x8_t a, int16_t b) {
  return __builtin_mpl_vector_qdmull_high_n_v4i32(a, (int16x8_t){b});
}

int64x2_t __builtin_mpl_vector_qdmull_high_n_v2i64(int32x4_t a, int32x4_t b);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmull_high_n_s32(int32x4_t a, int32_t b) {
  return __builtin_mpl_vector_qdmull_high_n_v2i64(a, (int32x4_t){b});
}

int32x4_t __builtin_mpl_vector_qdmull_lane_v4i32(int16x4_t a, int16x4_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmull_lane_s16(int16x4_t a, int16x4_t v,
                                                                                             const int lane) {
  return __builtin_mpl_vector_qdmull_lane_v4i32(a, v, lane);
}

int64x2_t __builtin_mpl_vector_qdmull_lane_v2i64(int32x2_t a, int32x2_t v, const int lane);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmull_lane_s32(int32x2_t a, int32x2_t v,
                                                                                             const int lane) {
  return __builtin_mpl_vector_qdmull_lane_v2i64(a, v, lane);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmullh_lane_s16(int16_t a, int16x4_t v,
                                                                                            const int lane) {
  return vgetq_lane_s32(vqdmull_lane_s16((int16x4_t){a}, v, lane), 0);
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmulls_lane_s32(int32_t a, int32x2_t v,
                                                                                            const int lane) {
  return vgetq_lane_s64(vqdmull_lane_s32((int32x2_t){a}, v, lane), 0);
}

int32x4_t __builtin_mpl_vector_qdmull_high_lane_v4i32(int16x8_t a, int16x4_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmull_high_lane_s16(int16x8_t a,
                                                                                                  int16x4_t v,
                                                                                                  const int lane) {
  return __builtin_mpl_vector_qdmull_high_lane_v4i32(a, v, lane);
}

int64x2_t __builtin_mpl_vector_qdmull_high_lane_v2i64(int32x4_t a, int32x2_t v, const int lane);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmull_high_lane_s32(int32x4_t a,
                                                                                                  int32x2_t v,
                                                                                                  const int lane) {
  return __builtin_mpl_vector_qdmull_high_lane_v2i64(a, v, lane);
}

int32x4_t __builtin_mpl_vector_qdmull_laneq_v4i32(int16x4_t a, int16x8_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmull_laneq_s16(int16x4_t a, int16x8_t v,
                                                                                              const int lane) {
  return __builtin_mpl_vector_qdmull_laneq_v4i32(a, v, lane);
}

int64x2_t __builtin_mpl_vector_qdmull_laneq_v2i64(int32x2_t a, int32x4_t v, const int lane);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmull_laneq_s32(int32x2_t a, int32x4_t v,
                                                                                              const int lane) {
  return __builtin_mpl_vector_qdmull_laneq_v2i64(a, v, lane);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmullh_laneq_s16(int16_t a, int16x8_t v,
                                                                                             const int lane) {
  return vgetq_lane_s32(vqdmull_laneq_s16((int16x4_t){a}, v, lane), 0);
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmulls_laneq_s32(int32_t a, int32x4_t v,
                                                                                             const int lane) {
  return vgetq_lane_s64(vqdmull_laneq_s32((int32x2_t){a}, v, lane), 0);
}

int32x4_t __builtin_mpl_vector_qdmull_high_laneq_v4i32(int16x8_t a, int16x8_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmull_high_laneq_s16(int16x8_t a,
                                                                                                   int16x8_t v,
                                                                                                   const int lane) {
  return __builtin_mpl_vector_qdmull_high_laneq_v4i32(a, v, lane);
}

int64x2_t __builtin_mpl_vector_qdmull_high_laneq_v2i64(int32x4_t a, int32x4_t v, const int lane);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmull_high_laneq_s32(int32x4_t a,
                                                                                                   int32x4_t v,
                                                                                                   const int lane) {
  return __builtin_mpl_vector_qdmull_high_laneq_v2i64(a, v, lane);
}

int16x4_t __builtin_mpl_vector_qdmulh_n_v4i16(int16x4_t a, int16x4_t b);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmulh_n_s16(int16x4_t a, int16_t b) {
  return __builtin_mpl_vector_qdmulh_n_v4i16(a, (int16x4_t){b});
}

int16x8_t __builtin_mpl_vector_qdmulhq_n_v8i16(int16x8_t a, int16x8_t b);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmulhq_n_s16(int16x8_t a, int16_t b) {
  return __builtin_mpl_vector_qdmulhq_n_v8i16(a, (int16x8_t){b});
}

int32x2_t __builtin_mpl_vector_qdmulh_n_v2i32(int32x2_t a, int32x2_t b);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmulh_n_s32(int32x2_t a, int32_t b) {
  return __builtin_mpl_vector_qdmulh_n_v2i32(a, (int32x2_t){b});
}

int32x4_t __builtin_mpl_vector_qdmulhq_n_v4i32(int32x4_t a, int32x4_t b);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmulhq_n_s32(int32x4_t a, int32_t b) {
  return __builtin_mpl_vector_qdmulhq_n_v4i32(a, (int32x4_t){b});
}

int16x4_t __builtin_mpl_vector_qdmulh_lane_v4i16(int16x4_t a, int16x4_t v, const int lane);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmulh_lane_s16(int16x4_t a, int16x4_t v,
                                                                                             const int lane) {
  return __builtin_mpl_vector_qdmulh_lane_v4i16(a, v, lane);
}

int16x8_t __builtin_mpl_vector_qdmulhq_lane_v8i16(int16x8_t a, int16x4_t v, const int lane);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmulhq_lane_s16(int16x8_t a, int16x4_t v,
                                                                                              const int lane) {
  return __builtin_mpl_vector_qdmulhq_lane_v8i16(a, v, lane);
}

int32x2_t __builtin_mpl_vector_qdmulh_lane_v2i32(int32x2_t a, int32x2_t v, const int lane);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmulh_lane_s32(int32x2_t a, int32x2_t v,
                                                                                             const int lane) {
  return __builtin_mpl_vector_qdmulh_lane_v2i32(a, v, lane);
}

int32x4_t __builtin_mpl_vector_qdmulhq_lane_v4i32(int32x4_t a, int32x2_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmulhq_lane_s32(int32x4_t a, int32x2_t v,
                                                                                              const int lane) {
  return __builtin_mpl_vector_qdmulhq_lane_v4i32(a, v, lane);
}

extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmulhh_lane_s16(int16_t a, int16x4_t v,
                                                                                            const int lane) {
  return vget_lane_s16(vqdmulh_lane_s16((int16x4_t){a}, v, lane), 0);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmulhs_lane_s32(int32_t a, int32x2_t v,
                                                                                            const int lane) {
  return vget_lane_s32(vqdmulh_lane_s32((int32x2_t){a}, v, lane), 0);
}

int16x4_t __builtin_mpl_vector_qdmulh_laneq_v4i16(int16x4_t a, int16x8_t v, const int lane);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmulh_laneq_s16(int16x4_t a, int16x8_t v,
                                                                                              const int lane) {
  return __builtin_mpl_vector_qdmulh_laneq_v4i16(a, v, lane);
}

int16x8_t __builtin_mpl_vector_qdmulhq_laneq_v8i16(int16x8_t a, int16x8_t v, const int lane);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmulhq_laneq_s16(int16x8_t a, int16x8_t v,
                                                                                               const int lane) {
  return __builtin_mpl_vector_qdmulhq_laneq_v8i16(a, v, lane);
}

int32x2_t __builtin_mpl_vector_qdmulh_laneq_v2i32(int32x2_t a, int32x4_t v, const int lane);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmulh_laneq_s32(int32x2_t a, int32x4_t v,
                                                                                              const int lane) {
  return __builtin_mpl_vector_qdmulh_laneq_v2i32(a, v, lane);
}

int32x4_t __builtin_mpl_vector_qdmulhq_laneq_v4i32(int32x4_t a, int32x4_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmulhq_laneq_s32(int32x4_t a, int32x4_t v,
                                                                                               const int lane) {
  return __builtin_mpl_vector_qdmulhq_laneq_v4i32(a, v, lane);
}

extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmulhh_laneq_s16(int16_t a, int16x8_t v,
                                                                                             const int lane) {
  return vget_lane_s16(vqdmulh_laneq_s16((int16x4_t){a}, v, lane), 0);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmulhs_laneq_s32(int32_t a, int32x4_t v,
                                                                                             const int lane) {
  return vget_lane_s32(vqdmulh_laneq_s32((int32x2_t){a}, v, lane), 0);
}

int16x4_t __builtin_mpl_vector_qrdmulh_n_v4i16(int16x4_t a, int16x4_t b);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrdmulh_n_s16(int16x4_t a, int16_t b) {
  return __builtin_mpl_vector_qrdmulh_n_v4i16(a, (int16x4_t){b});
}

int16x8_t __builtin_mpl_vector_qrdmulhq_n_v8i16(int16x8_t a, int16x8_t b);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrdmulhq_n_s16(int16x8_t a, int16_t b) {
  return __builtin_mpl_vector_qrdmulhq_n_v8i16(a, (int16x8_t){b});
}

int32x2_t __builtin_mpl_vector_qrdmulh_n_v2i32(int32x2_t a, int32x2_t b);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrdmulh_n_s32(int32x2_t a, int32_t b) {
  return __builtin_mpl_vector_qrdmulh_n_v2i32(a, (int32x2_t){b});
}

int32x4_t __builtin_mpl_vector_qrdmulhq_n_v4i32(int32x4_t a, int32x4_t b);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrdmulhq_n_s32(int32x4_t a, int32_t b) {
  return __builtin_mpl_vector_qrdmulhq_n_v4i32(a, (int32x4_t){b});
}

int16x4_t __builtin_mpl_vector_qrdmulh_lane_v4i16(int16x4_t a, int16x4_t v, const int lane);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrdmulh_lane_s16(int16x4_t a, int16x4_t v,
                                                                                              const int lane) {
  return __builtin_mpl_vector_qrdmulh_lane_v4i16(a, v, lane);
}

int16x8_t __builtin_mpl_vector_qrdmulhq_lane_v8i16(int16x8_t a, int16x4_t v, const int lane);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrdmulhq_lane_s16(int16x8_t a, int16x4_t v,
                                                                                               const int lane) {
  return __builtin_mpl_vector_qrdmulhq_lane_v8i16(a, v, lane);
}

int32x2_t __builtin_mpl_vector_qrdmulh_lane_v2i32(int32x2_t a, int32x2_t v, const int lane);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrdmulh_lane_s32(int32x2_t a, int32x2_t v,
                                                                                              const int lane) {
  return __builtin_mpl_vector_qrdmulh_lane_v2i32(a, v, lane);
}

int32x4_t __builtin_mpl_vector_qrdmulhq_lane_v4i32(int32x4_t a, int32x2_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrdmulhq_lane_s32(int32x4_t a, int32x2_t v,
                                                                                               const int lane) {
  return __builtin_mpl_vector_qrdmulhq_lane_v4i32(a, v, lane);
}

extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrdmulhh_lane_s16(int16_t a, int16x4_t v,
                                                                                             const int lane) {
  return vget_lane_s16(vqrdmulh_lane_s16((int16x4_t){a}, v, lane), 0);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrdmulhs_lane_s32(int32_t a, int32x2_t v,
                                                                                             const int lane) {
  return vget_lane_s32(vqrdmulh_lane_s32((int32x2_t){a}, v, lane), 0);
}

int16x4_t __builtin_mpl_vector_qrdmulh_laneq_v4i16(int16x4_t a, int16x8_t v, const int lane);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrdmulh_laneq_s16(int16x4_t a, int16x8_t v,
                                                                                               const int lane) {
  return __builtin_mpl_vector_qrdmulh_laneq_v4i16(a, v, lane);
}

int16x8_t __builtin_mpl_vector_qrdmulhq_laneq_v8i16(int16x8_t a, int16x8_t v, const int lane);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrdmulhq_laneq_s16(int16x8_t a,
                                                                                                int16x8_t v,
                                                                                                const int lane) {
  return __builtin_mpl_vector_qrdmulhq_laneq_v8i16(a, v, lane);
}

int32x2_t __builtin_mpl_vector_qrdmulh_laneq_v2i32(int32x2_t a, int32x4_t v, const int lane);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrdmulh_laneq_s32(int32x2_t a, int32x4_t v,
                                                                                               const int lane) {
  return __builtin_mpl_vector_qrdmulh_laneq_v2i32(a, v, lane);
}

int32x4_t __builtin_mpl_vector_qrdmulhq_laneq_v4i32(int32x4_t a, int32x4_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrdmulhq_laneq_s32(int32x4_t a,
                                                                                                int32x4_t v,
                                                                                                const int lane) {
  return __builtin_mpl_vector_qrdmulhq_laneq_v4i32(a, v, lane);
}

extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrdmulhh_laneq_s16(int16_t a, int16x8_t v,
                                                                                              const int lane) {
  return vget_lane_s16(vqrdmulh_laneq_s16((int16x4_t){a}, v, lane), 0);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrdmulhs_laneq_s32(int32_t a, int32x4_t v,
                                                                                              const int lane) {
  return vget_lane_s32(vqrdmulh_laneq_s32((int32x2_t){a}, v, lane), 0);
}

int32x4_t __builtin_mpl_vector_qdmlal_n_v4i32(int32x4_t a, int16x4_t b, int16x4_t c);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlal_n_s16(int32x4_t a, int16x4_t b,
                                                                                          int16_t c) {
  return __builtin_mpl_vector_qdmlal_n_v4i32(a, b, (int16x4_t){c});
}

int64x2_t __builtin_mpl_vector_qdmlal_n_v2i64(int64x2_t a, int32x2_t b, int32x2_t c);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlal_n_s32(int64x2_t a, int32x2_t b,
                                                                                          int32_t c) {
  return __builtin_mpl_vector_qdmlal_n_v2i64(a, b, (int32x2_t){c});
}

int32x4_t __builtin_mpl_vector_qdmlal_high_n_v4i32(int32x4_t a, int16x8_t b, int16x8_t c);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlal_high_n_s16(int32x4_t a, int16x8_t b,
                                                                                               int16_t c) {
  return __builtin_mpl_vector_qdmlal_high_n_v4i32(a, b, (int16x8_t){c});
}

int64x2_t __builtin_mpl_vector_qdmlal_high_n_v2i64(int64x2_t a, int32x4_t b, int32x4_t c);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlal_high_n_s32(int64x2_t a, int32x4_t b,
                                                                                               int32_t c) {
  return __builtin_mpl_vector_qdmlal_high_n_v2i64(a, b, (int32x4_t){c});
}

int32x4_t __builtin_mpl_vector_qdmlsl_n_v4i32(int32x4_t a, int16x4_t b, int16x4_t c);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlsl_n_s16(int32x4_t a, int16x4_t b,
                                                                                          int16_t c) {
  return __builtin_mpl_vector_qdmlsl_n_v4i32(a, b, (int16x4_t){c});
}

int64x2_t __builtin_mpl_vector_qdmlsl_n_v2i64(int64x2_t a, int32x2_t b, int32x2_t c);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlsl_n_s32(int64x2_t a, int32x2_t b,
                                                                                          int32_t c) {
  return __builtin_mpl_vector_qdmlsl_n_v2i64(a, b, (int32x2_t){c});
}

int32x4_t __builtin_mpl_vector_qdmlsl_high_n_v4i32(int32x4_t a, int16x8_t b, int16x8_t c);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlsl_high_n_s16(int32x4_t a, int16x8_t b,
                                                                                               int16_t c) {
  return __builtin_mpl_vector_qdmlsl_high_n_v4i32(a, b, (int16x8_t){c});
}

int64x2_t __builtin_mpl_vector_qdmlsl_high_n_v2i64(int64x2_t a, int32x4_t b, int32x4_t c);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqdmlsl_high_n_s32(int64x2_t a, int32x4_t b,
                                                                                               int32_t c) {
  return __builtin_mpl_vector_qdmlsl_high_n_v2i64(a, b, (int32x4_t){c});
}

int8x8_t __builtin_mpl_vector_hsub_v8i8(int8x8_t a, int8x8_t b);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vhsub_s8(int8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_hsub_v8i8(a, b);
}

int8x16_t __builtin_mpl_vector_hsubq_v16i8(int8x16_t a, int8x16_t b);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vhsubq_s8(int8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_hsubq_v16i8(a, b);
}

int16x4_t __builtin_mpl_vector_hsub_v4i16(int16x4_t a, int16x4_t b);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vhsub_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_hsub_v4i16(a, b);
}

int16x8_t __builtin_mpl_vector_hsubq_v8i16(int16x8_t a, int16x8_t b);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vhsubq_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_hsubq_v8i16(a, b);
}

int32x2_t __builtin_mpl_vector_hsub_v2i32(int32x2_t a, int32x2_t b);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vhsub_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_hsub_v2i32(a, b);
}

int32x4_t __builtin_mpl_vector_hsubq_v4i32(int32x4_t a, int32x4_t b);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vhsubq_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_hsubq_v4i32(a, b);
}

uint8x8_t __builtin_mpl_vector_hsub_v8u8(uint8x8_t a, uint8x8_t b);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vhsub_u8(uint8x8_t a, uint8x8_t b) {
  return __builtin_mpl_vector_hsub_v8u8(a, b);
}

uint8x16_t __builtin_mpl_vector_hsubq_v16u8(uint8x16_t a, uint8x16_t b);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vhsubq_u8(uint8x16_t a, uint8x16_t b) {
  return __builtin_mpl_vector_hsubq_v16u8(a, b);
}

uint16x4_t __builtin_mpl_vector_hsub_v4u16(uint16x4_t a, uint16x4_t b);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vhsub_u16(uint16x4_t a, uint16x4_t b) {
  return __builtin_mpl_vector_hsub_v4u16(a, b);
}

uint16x8_t __builtin_mpl_vector_hsubq_v8u16(uint16x8_t a, uint16x8_t b);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vhsubq_u16(uint16x8_t a, uint16x8_t b) {
  return __builtin_mpl_vector_hsubq_v8u16(a, b);
}

uint32x2_t __builtin_mpl_vector_hsub_v2u32(uint32x2_t a, uint32x2_t b);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vhsub_u32(uint32x2_t a, uint32x2_t b) {
  return __builtin_mpl_vector_hsub_v2u32(a, b);
}

uint32x4_t __builtin_mpl_vector_hsubq_v4u32(uint32x4_t a, uint32x4_t b);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vhsubq_u32(uint32x4_t a, uint32x4_t b) {
  return __builtin_mpl_vector_hsubq_v4u32(a, b);
}

int8x8_t __builtin_mpl_vector_subhn_v8i8(int16x8_t a, int16x8_t b);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubhn_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_subhn_v8i8(a, b);
}

int16x4_t __builtin_mpl_vector_subhn_v4i16(int32x4_t a, int32x4_t b);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubhn_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_subhn_v4i16(a, b);
}

int32x2_t __builtin_mpl_vector_subhn_v2i32(int64x2_t a, int64x2_t b);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubhn_s64(int64x2_t a, int64x2_t b) {
  return __builtin_mpl_vector_subhn_v2i32(a, b);
}

uint8x8_t __builtin_mpl_vector_subhn_v8u8(uint16x8_t a, uint16x8_t b);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubhn_u16(uint16x8_t a, uint16x8_t b) {
  return __builtin_mpl_vector_subhn_v8u8(a, b);
}

uint16x4_t __builtin_mpl_vector_subhn_v4u16(uint32x4_t a, uint32x4_t b);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubhn_u32(uint32x4_t a, uint32x4_t b) {
  return __builtin_mpl_vector_subhn_v4u16(a, b);
}

uint32x2_t __builtin_mpl_vector_subhn_v2u32(uint64x2_t a, uint64x2_t b);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubhn_u64(uint64x2_t a, uint64x2_t b) {
  return __builtin_mpl_vector_subhn_v2u32(a, b);
}

int8x16_t __builtin_mpl_vector_subhn_high_v16i8(int8x8_t r, int16x8_t a, int16x8_t b);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubhn_high_s16(int8x8_t r, int16x8_t a,
                                                                                            int16x8_t b) {
  return __builtin_mpl_vector_subhn_high_v16i8(r, a, b);
}

int16x8_t __builtin_mpl_vector_subhn_high_v8i16(int16x4_t r, int32x4_t a, int32x4_t b);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubhn_high_s32(int16x4_t r, int32x4_t a,
                                                                                            int32x4_t b) {
  return __builtin_mpl_vector_subhn_high_v8i16(r, a, b);
}

int32x4_t __builtin_mpl_vector_subhn_high_v4i32(int32x2_t r, int64x2_t a, int64x2_t b);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubhn_high_s64(int32x2_t r, int64x2_t a,
                                                                                            int64x2_t b) {
  return __builtin_mpl_vector_subhn_high_v4i32(r, a, b);
}

uint8x16_t __builtin_mpl_vector_subhn_high_v16u8(uint8x8_t r, uint16x8_t a, uint16x8_t b);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubhn_high_u16(uint8x8_t r, uint16x8_t a,
                                                                                             uint16x8_t b) {
  return __builtin_mpl_vector_subhn_high_v16u8(r, a, b);
}

uint16x8_t __builtin_mpl_vector_subhn_high_v8u16(uint16x4_t r, uint32x4_t a, uint32x4_t b);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubhn_high_u32(uint16x4_t r, uint32x4_t a,
                                                                                             uint32x4_t b) {
  return __builtin_mpl_vector_subhn_high_v8u16(r, a, b);
}

uint32x4_t __builtin_mpl_vector_subhn_high_v4u32(uint32x2_t r, uint64x2_t a, uint64x2_t b);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsubhn_high_u64(uint32x2_t r, uint64x2_t a,
                                                                                             uint64x2_t b) {
  return __builtin_mpl_vector_subhn_high_v4u32(r, a, b);
}

int8x8_t __builtin_mpl_vector_rsubhn_v8i8(int16x8_t a, int16x8_t b);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsubhn_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_rsubhn_v8i8(a, b);
}

int16x4_t __builtin_mpl_vector_rsubhn_v4i16(int32x4_t a, int32x4_t b);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsubhn_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_rsubhn_v4i16(a, b);
}

int32x2_t __builtin_mpl_vector_rsubhn_v2i32(int64x2_t a, int64x2_t b);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsubhn_s64(int64x2_t a, int64x2_t b) {
  return __builtin_mpl_vector_rsubhn_v2i32(a, b);
}

uint8x8_t __builtin_mpl_vector_rsubhn_v8u8(uint16x8_t a, uint16x8_t b);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsubhn_u16(uint16x8_t a, uint16x8_t b) {
  return __builtin_mpl_vector_rsubhn_v8u8(a, b);
}

uint16x4_t __builtin_mpl_vector_rsubhn_v4u16(uint32x4_t a, uint32x4_t b);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsubhn_u32(uint32x4_t a, uint32x4_t b) {
  return __builtin_mpl_vector_rsubhn_v4u16(a, b);
}

uint32x2_t __builtin_mpl_vector_rsubhn_v2u32(uint64x2_t a, uint64x2_t b);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsubhn_u64(uint64x2_t a, uint64x2_t b) {
  return __builtin_mpl_vector_rsubhn_v2u32(a, b);
}

int8x16_t __builtin_mpl_vector_rsubhn_high_v16i8(int8x8_t r, int16x8_t a, int16x8_t b);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsubhn_high_s16(int8x8_t r, int16x8_t a,
                                                                                             int16x8_t b) {
  return __builtin_mpl_vector_rsubhn_high_v16i8(r, a, b);
}

int16x8_t __builtin_mpl_vector_rsubhn_high_v8i16(int16x4_t r, int32x4_t a, int32x4_t b);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsubhn_high_s32(int16x4_t r, int32x4_t a,
                                                                                             int32x4_t b) {
  return __builtin_mpl_vector_rsubhn_high_v8i16(r, a, b);
}

int32x4_t __builtin_mpl_vector_rsubhn_high_v4i32(int32x2_t r, int64x2_t a, int64x2_t b);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsubhn_high_s64(int32x2_t r, int64x2_t a,
                                                                                             int64x2_t b) {
  return __builtin_mpl_vector_rsubhn_high_v4i32(r, a, b);
}

uint8x16_t __builtin_mpl_vector_rsubhn_high_v16u8(uint8x8_t r, uint16x8_t a, uint16x8_t b);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsubhn_high_u16(uint8x8_t r, uint16x8_t a,
                                                                                              uint16x8_t b) {
  return __builtin_mpl_vector_rsubhn_high_v16u8(r, a, b);
}

uint16x8_t __builtin_mpl_vector_rsubhn_high_v8u16(uint16x4_t r, uint32x4_t a, uint32x4_t b);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsubhn_high_u32(uint16x4_t r,
                                                                                              uint32x4_t a,
                                                                                              uint32x4_t b) {
  return __builtin_mpl_vector_rsubhn_high_v8u16(r, a, b);
}

uint32x4_t __builtin_mpl_vector_rsubhn_high_v4u32(uint32x2_t r, uint64x2_t a, uint64x2_t b);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsubhn_high_u64(uint32x2_t r,
                                                                                              uint64x2_t a,
                                                                                              uint64x2_t b) {
  return __builtin_mpl_vector_rsubhn_high_v4u32(r, a, b);
}

int8x8_t __builtin_mpl_vector_qsub_v8i8(int8x8_t a, int8x8_t b);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqsub_s8(int8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_qsub_v8i8(a, b);
}

int8x16_t __builtin_mpl_vector_qsubq_v16i8(int8x16_t a, int8x16_t b);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqsubq_s8(int8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_qsubq_v16i8(a, b);
}

int16x4_t __builtin_mpl_vector_qsub_v4i16(int16x4_t a, int16x4_t b);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqsub_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_qsub_v4i16(a, b);
}

int16x8_t __builtin_mpl_vector_qsubq_v8i16(int16x8_t a, int16x8_t b);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqsubq_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_qsubq_v8i16(a, b);
}

int32x2_t __builtin_mpl_vector_qsub_v2i32(int32x2_t a, int32x2_t b);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqsub_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_qsub_v2i32(a, b);
}

int32x4_t __builtin_mpl_vector_qsubq_v4i32(int32x4_t a, int32x4_t b);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqsubq_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_qsubq_v4i32(a, b);
}

int64x1_t __builtin_mpl_vector_qsub_v1i64(int64x1_t a, int64x1_t b);
extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vqsub_s64(int64x1_t a, int64x1_t b) {
  return __builtin_mpl_vector_qsub_v1i64(a, b);
}

int64x2_t __builtin_mpl_vector_qsubq_v2i64(int64x2_t a, int64x2_t b);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqsubq_s64(int64x2_t a, int64x2_t b) {
  return __builtin_mpl_vector_qsubq_v2i64(a, b);
}

uint8x8_t __builtin_mpl_vector_qsub_v8u8(uint8x8_t a, uint8x8_t b);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqsub_u8(uint8x8_t a, uint8x8_t b) {
  return __builtin_mpl_vector_qsub_v8u8(a, b);
}

uint8x16_t __builtin_mpl_vector_qsubq_v16u8(uint8x16_t a, uint8x16_t b);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqsubq_u8(uint8x16_t a, uint8x16_t b) {
  return __builtin_mpl_vector_qsubq_v16u8(a, b);
}

uint16x4_t __builtin_mpl_vector_qsub_v4u16(uint16x4_t a, uint16x4_t b);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqsub_u16(uint16x4_t a, uint16x4_t b) {
  return __builtin_mpl_vector_qsub_v4u16(a, b);
}

uint16x8_t __builtin_mpl_vector_qsubq_v8u16(uint16x8_t a, uint16x8_t b);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqsubq_u16(uint16x8_t a, uint16x8_t b) {
  return __builtin_mpl_vector_qsubq_v8u16(a, b);
}

uint32x2_t __builtin_mpl_vector_qsub_v2u32(uint32x2_t a, uint32x2_t b);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqsub_u32(uint32x2_t a, uint32x2_t b) {
  return __builtin_mpl_vector_qsub_v2u32(a, b);
}

uint32x4_t __builtin_mpl_vector_qsubq_v4u32(uint32x4_t a, uint32x4_t b);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqsubq_u32(uint32x4_t a, uint32x4_t b) {
  return __builtin_mpl_vector_qsubq_v4u32(a, b);
}

uint64x1_t __builtin_mpl_vector_qsub_v1u64(uint64x1_t a, uint64x1_t b);
extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vqsub_u64(uint64x1_t a, uint64x1_t b) {
  return __builtin_mpl_vector_qsub_v1u64(a, b);
}

uint64x2_t __builtin_mpl_vector_qsubq_v2u64(uint64x2_t a, uint64x2_t b);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqsubq_u64(uint64x2_t a, uint64x2_t b) {
  return __builtin_mpl_vector_qsubq_v2u64(a, b);
}

extern inline int8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqsubb_s8(int8_t a, int8_t b) {
  return vget_lane_s8(vqsub_s8((int8x8_t){a}, (int8x8_t){b}), 0);
}

extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqsubh_s16(int16_t a, int16_t b) {
  return vget_lane_s16(vqsub_s16((int16x4_t){a}, (int16x4_t){b}), 0);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqsubs_s32(int32_t a, int32_t b) {
  return vget_lane_s32(vqsub_s32((int32x2_t){a}, (int32x2_t){b}), 0);
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vqsubd_s64(int64_t a, int64_t b) {
  return vget_lane_s64(vqsub_s64((int64x1_t){a}, (int64x1_t){b}), 0);
}

extern inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqsubb_u8(uint8_t a, uint8_t b) {
  return vget_lane_u8(vqsub_u8((uint8x8_t){a}, (uint8x8_t){b}), 0);
}

extern inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqsubh_u16(uint16_t a, uint16_t b) {
  return vget_lane_u16(vqsub_u16((uint16x4_t){a}, (uint16x4_t){b}), 0);
}

extern inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqsubs_u32(uint32_t a, uint32_t b) {
  return vget_lane_u32(vqsub_u32((uint32x2_t){a}, (uint32x2_t){b}), 0);
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vqsubd_u64(uint64_t a, uint64_t b) {
  return vget_lane_u64(vqsub_u64((uint64x1_t){a}, (uint64x1_t){b}), 0);
}

int8x8_t __builtin_mpl_vector_aba_v8i8(int8x8_t a, int8x8_t b, int8x8_t c);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vaba_s8(int8x8_t a, int8x8_t b, int8x8_t c) {
  return __builtin_mpl_vector_aba_v8i8(a, b, c);
}

int8x16_t __builtin_mpl_vector_abaq_v16i8(int8x16_t a, int8x16_t b, int8x16_t c);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vabaq_s8(int8x16_t a, int8x16_t b,
                                                                                     int8x16_t c) {
  return __builtin_mpl_vector_abaq_v16i8(a, b, c);
}

int16x4_t __builtin_mpl_vector_aba_v4i16(int16x4_t a, int16x4_t b, int16x4_t c);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vaba_s16(int16x4_t a, int16x4_t b,
                                                                                     int16x4_t c) {
  return __builtin_mpl_vector_aba_v4i16(a, b, c);
}

int16x8_t __builtin_mpl_vector_abaq_v8i16(int16x8_t a, int16x8_t b, int16x8_t c);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vabaq_s16(int16x8_t a, int16x8_t b,
                                                                                      int16x8_t c) {
  return __builtin_mpl_vector_abaq_v8i16(a, b, c);
}

int32x2_t __builtin_mpl_vector_aba_v2i32(int32x2_t a, int32x2_t b, int32x2_t c);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vaba_s32(int32x2_t a, int32x2_t b,
                                                                                     int32x2_t c) {
  return __builtin_mpl_vector_aba_v2i32(a, b, c);
}

int32x4_t __builtin_mpl_vector_abaq_v4i32(int32x4_t a, int32x4_t b, int32x4_t c);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vabaq_s32(int32x4_t a, int32x4_t b,
                                                                                      int32x4_t c) {
  return __builtin_mpl_vector_abaq_v4i32(a, b, c);
}

uint8x8_t __builtin_mpl_vector_aba_v8u8(uint8x8_t a, uint8x8_t b, uint8x8_t c);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vaba_u8(uint8x8_t a, uint8x8_t b,
                                                                                    uint8x8_t c) {
  return __builtin_mpl_vector_aba_v8u8(a, b, c);
}

uint8x16_t __builtin_mpl_vector_abaq_v16u8(uint8x16_t a, uint8x16_t b, uint8x16_t c);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vabaq_u8(uint8x16_t a, uint8x16_t b,
                                                                                      uint8x16_t c) {
  return __builtin_mpl_vector_abaq_v16u8(a, b, c);
}

uint16x4_t __builtin_mpl_vector_aba_v4u16(uint16x4_t a, uint16x4_t b, uint16x4_t c);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vaba_u16(uint16x4_t a, uint16x4_t b,
                                                                                      uint16x4_t c) {
  return __builtin_mpl_vector_aba_v4u16(a, b, c);
}

uint16x8_t __builtin_mpl_vector_abaq_v8u16(uint16x8_t a, uint16x8_t b, uint16x8_t c);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vabaq_u16(uint16x8_t a, uint16x8_t b,
                                                                                       uint16x8_t c) {
  return __builtin_mpl_vector_abaq_v8u16(a, b, c);
}

uint32x2_t __builtin_mpl_vector_aba_v2u32(uint32x2_t a, uint32x2_t b, uint32x2_t c);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vaba_u32(uint32x2_t a, uint32x2_t b,
                                                                                      uint32x2_t c) {
  return __builtin_mpl_vector_aba_v2u32(a, b, c);
}

uint32x4_t __builtin_mpl_vector_abaq_v4u32(uint32x4_t a, uint32x4_t b, uint32x4_t c);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vabaq_u32(uint32x4_t a, uint32x4_t b,
                                                                                       uint32x4_t c) {
  return __builtin_mpl_vector_abaq_v4u32(a, b, c);
}

int16x8_t __builtin_mpl_vector_abal_v8i16(int16x8_t a, int8x8_t b, int8x8_t c);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vabal_s8(int16x8_t a, int8x8_t b,
                                                                                     int8x8_t c) {
  return __builtin_mpl_vector_abal_v8i16(a, b, c);
}

int32x4_t __builtin_mpl_vector_abal_v4i32(int32x4_t a, int16x4_t b, int16x4_t c);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vabal_s16(int32x4_t a, int16x4_t b,
                                                                                      int16x4_t c) {
  return __builtin_mpl_vector_abal_v4i32(a, b, c);
}

int64x2_t __builtin_mpl_vector_abal_v2i64(int64x2_t a, int32x2_t b, int32x2_t c);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vabal_s32(int64x2_t a, int32x2_t b,
                                                                                      int32x2_t c) {
  return __builtin_mpl_vector_abal_v2i64(a, b, c);
}

uint16x8_t __builtin_mpl_vector_abal_v8u16(uint16x8_t a, uint8x8_t b, uint8x8_t c);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vabal_u8(uint16x8_t a, uint8x8_t b,
                                                                                      uint8x8_t c) {
  return __builtin_mpl_vector_abal_v8u16(a, b, c);
}

uint32x4_t __builtin_mpl_vector_abal_v4u32(uint32x4_t a, uint16x4_t b, uint16x4_t c);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vabal_u16(uint32x4_t a, uint16x4_t b,
                                                                                       uint16x4_t c) {
  return __builtin_mpl_vector_abal_v4u32(a, b, c);
}

uint64x2_t __builtin_mpl_vector_abal_v2u64(uint64x2_t a, uint32x2_t b, uint32x2_t c);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vabal_u32(uint64x2_t a, uint32x2_t b,
                                                                                       uint32x2_t c) {
  return __builtin_mpl_vector_abal_v2u64(a, b, c);
}

int16x8_t __builtin_mpl_vector_abal_high_v8i16(int16x8_t a, int8x16_t b, int8x16_t c);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vabal_high_s8(int16x8_t a, int8x16_t b,
                                                                                          int8x16_t c) {
  return __builtin_mpl_vector_abal_high_v8i16(a, b, c);
}

int32x4_t __builtin_mpl_vector_abal_high_v4i32(int32x4_t a, int16x8_t b, int16x8_t c);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vabal_high_s16(int32x4_t a, int16x8_t b,
                                                                                           int16x8_t c) {
  return __builtin_mpl_vector_abal_high_v4i32(a, b, c);
}

int64x2_t __builtin_mpl_vector_abal_high_v2i64(int64x2_t a, int32x4_t b, int32x4_t c);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vabal_high_s32(int64x2_t a, int32x4_t b,
                                                                                           int32x4_t c) {
  return __builtin_mpl_vector_abal_high_v2i64(a, b, c);
}

uint16x8_t __builtin_mpl_vector_abal_high_v8u16(uint16x8_t a, uint8x16_t b, uint8x16_t c);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vabal_high_u8(uint16x8_t a, uint8x16_t b,
                                                                                           uint8x16_t c) {
  return __builtin_mpl_vector_abal_high_v8u16(a, b, c);
}

uint32x4_t __builtin_mpl_vector_abal_high_v4u32(uint32x4_t a, uint16x8_t b, uint16x8_t c);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vabal_high_u16(uint32x4_t a, uint16x8_t b,
                                                                                            uint16x8_t c) {
  return __builtin_mpl_vector_abal_high_v4u32(a, b, c);
}

uint64x2_t __builtin_mpl_vector_abal_high_v2u64(uint64x2_t a, uint32x4_t b, uint32x4_t c);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vabal_high_u32(uint64x2_t a, uint32x4_t b,
                                                                                            uint32x4_t c) {
  return __builtin_mpl_vector_abal_high_v2u64(a, b, c);
}

int8x8_t __builtin_mpl_vector_qabs_v8i8(int8x8_t a);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqabs_s8(int8x8_t a) {
  return __builtin_mpl_vector_qabs_v8i8(a);
}

int8x16_t __builtin_mpl_vector_qabsq_v16i8(int8x16_t a);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqabsq_s8(int8x16_t a) {
  return __builtin_mpl_vector_qabsq_v16i8(a);
}

int16x4_t __builtin_mpl_vector_qabs_v4i16(int16x4_t a);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqabs_s16(int16x4_t a) {
  return __builtin_mpl_vector_qabs_v4i16(a);
}

int16x8_t __builtin_mpl_vector_qabsq_v8i16(int16x8_t a);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqabsq_s16(int16x8_t a) {
  return __builtin_mpl_vector_qabsq_v8i16(a);
}

int32x2_t __builtin_mpl_vector_qabs_v2i32(int32x2_t a);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqabs_s32(int32x2_t a) {
  return __builtin_mpl_vector_qabs_v2i32(a);
}

int32x4_t __builtin_mpl_vector_qabsq_v4i32(int32x4_t a);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqabsq_s32(int32x4_t a) {
  return __builtin_mpl_vector_qabsq_v4i32(a);
}

int64x1_t __builtin_mpl_vector_qabs_v1i64(int64x1_t a);
extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vqabs_s64(int64x1_t a) {
  return __builtin_mpl_vector_qabs_v1i64(a);
}

int64x2_t __builtin_mpl_vector_qabsq_v2i64(int64x2_t a);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqabsq_s64(int64x2_t a) {
  return __builtin_mpl_vector_qabsq_v2i64(a);
}

extern inline int8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqabsb_s8(int8_t a) {
  return vget_lane_s8(vqabs_s8((int8x8_t){a}), 0);
}

extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqabsh_s16(int16_t a) {
  return vget_lane_s16(vqabs_s16((int16x4_t){a}), 0);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqabss_s32(int32_t a) {
  return vget_lane_s32(vqabs_s32((int32x2_t){a}), 0);
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vqabsd_s64(int64_t a) {
  return vget_lane_s64(vqabs_s64((int64x1_t){a}), 0);
}

uint32x2_t __builtin_mpl_vector_rsqrte_v2u32(uint32x2_t a);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsqrte_u32(uint32x2_t a) {
  return __builtin_mpl_vector_rsqrte_v2u32(a);
}

uint32x4_t __builtin_mpl_vector_rsqrteq_v4u32(uint32x4_t a);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsqrteq_u32(uint32x4_t a) {
  return __builtin_mpl_vector_rsqrteq_v4u32(a);
}

int16x4_t __builtin_mpl_vector_addlv_v8i8(int8x8_t a);
extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddlv_s8(int8x8_t a) {
  return vget_lane_s16(__builtin_mpl_vector_addlv_v8i8(a), 0);
}

int16x4_t __builtin_mpl_vector_addlvq_v16i8(int8x16_t a);
extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddlvq_s8(int8x16_t a) {
  return vget_lane_s16(__builtin_mpl_vector_addlvq_v16i8(a), 0);
}

int32x2_t __builtin_mpl_vector_addlv_v4i16(int16x4_t a);
extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddlv_s16(int16x4_t a) {
  return vget_lane_s32(__builtin_mpl_vector_addlv_v4i16(a), 0);
}

int32x2_t __builtin_mpl_vector_addlvq_v8i16(int16x8_t a);
extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddlvq_s16(int16x8_t a) {
  return vget_lane_s32(__builtin_mpl_vector_addlvq_v8i16(a), 0);
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddlv_s32(int32x2_t a) {
  return vget_lane_s64(vpaddl_s32(a), 0);
}

int64x1_t __builtin_mpl_vector_addlvq_v4i32(int32x4_t a);
extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddlvq_s32(int32x4_t a) {
  return vget_lane_s64(__builtin_mpl_vector_addlvq_v4i32(a), 0);
}

uint16x4_t __builtin_mpl_vector_addlv_v8u8(uint8x8_t a);
extern inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddlv_u8(uint8x8_t a) {
  return vget_lane_u16(__builtin_mpl_vector_addlv_v8u8(a), 0);
}

uint16x4_t __builtin_mpl_vector_addlvq_v16u8(uint8x16_t a);
extern inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddlvq_u8(uint8x16_t a) {
  return vget_lane_u16(__builtin_mpl_vector_addlvq_v16u8(a), 0);
}

uint32x2_t __builtin_mpl_vector_addlv_v4u16(uint16x4_t a);
extern inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddlv_u16(uint16x4_t a) {
  return vget_lane_u32(__builtin_mpl_vector_addlv_v4u16(a), 0);
}

uint32x2_t __builtin_mpl_vector_addlvq_v8u16(uint16x8_t a);
extern inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddlvq_u16(uint16x8_t a) {
  return vget_lane_u32(__builtin_mpl_vector_addlvq_v8u16(a), 0);
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddlv_u32(uint32x2_t a) {
  return vget_lane_u64(vpaddl_u32(a), 0);
}

uint64x1_t __builtin_mpl_vector_addlvq_v4u32(uint32x4_t a);
extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddlvq_u32(uint32x4_t a) {
  return vget_lane_u64(__builtin_mpl_vector_addlvq_v4u32(a), 0);
}

int8x8_t __builtin_mpl_vector_qshl_v8i8(int8x8_t a, int8x8_t b);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshl_s8(int8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_qshl_v8i8(a, b);
}

int8x16_t __builtin_mpl_vector_qshlq_v16i8(int8x16_t a, int8x16_t b);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlq_s8(int8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_qshlq_v16i8(a, b);
}

int16x4_t __builtin_mpl_vector_qshl_v4i16(int16x4_t a, int16x4_t b);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshl_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_qshl_v4i16(a, b);
}

int16x8_t __builtin_mpl_vector_qshlq_v8i16(int16x8_t a, int16x8_t b);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlq_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_qshlq_v8i16(a, b);
}

int32x2_t __builtin_mpl_vector_qshl_v2i32(int32x2_t a, int32x2_t b);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshl_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_qshl_v2i32(a, b);
}

int32x4_t __builtin_mpl_vector_qshlq_v4i32(int32x4_t a, int32x4_t b);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlq_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_qshlq_v4i32(a, b);
}

int64x1_t __builtin_mpl_vector_qshl_v1i64(int64x1_t a, int64x1_t b);
extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshl_s64(int64x1_t a, int64x1_t b) {
  return __builtin_mpl_vector_qshl_v1i64(a, b);
}

int64x2_t __builtin_mpl_vector_qshlq_v2i64(int64x2_t a, int64x2_t b);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlq_s64(int64x2_t a, int64x2_t b) {
  return __builtin_mpl_vector_qshlq_v2i64(a, b);
}

uint8x8_t __builtin_mpl_vector_qshl_v8u8(uint8x8_t a, int8x8_t b);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshl_u8(uint8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_qshl_v8u8(a, b);
}

uint8x16_t __builtin_mpl_vector_qshlq_v16u8(uint8x16_t a, int8x16_t b);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlq_u8(uint8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_qshlq_v16u8(a, b);
}

uint16x4_t __builtin_mpl_vector_qshl_v4u16(uint16x4_t a, int16x4_t b);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshl_u16(uint16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_qshl_v4u16(a, b);
}

uint16x8_t __builtin_mpl_vector_qshlq_v8u16(uint16x8_t a, int16x8_t b);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlq_u16(uint16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_qshlq_v8u16(a, b);
}

uint32x2_t __builtin_mpl_vector_qshl_v2u32(uint32x2_t a, int32x2_t b);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshl_u32(uint32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_qshl_v2u32(a, b);
}

uint32x4_t __builtin_mpl_vector_qshlq_v4u32(uint32x4_t a, int32x4_t b);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlq_u32(uint32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_qshlq_v4u32(a, b);
}

uint64x1_t __builtin_mpl_vector_qshl_v1u64(uint64x1_t a, int64x1_t b);
extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshl_u64(uint64x1_t a, int64x1_t b) {
  return __builtin_mpl_vector_qshl_v1u64(a, b);
}

uint64x2_t __builtin_mpl_vector_qshlq_v2u64(uint64x2_t a, int64x2_t b);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlq_u64(uint64x2_t a, int64x2_t b) {
  return __builtin_mpl_vector_qshlq_v2u64(a, b);
}

extern inline int8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlb_s8(int8_t a, int8_t b) {
  return vget_lane_s8(vqshl_s8((int8x8_t){a}, (int8x8_t){b}), 0);
}

extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlh_s16(int16_t a, int16_t b) {
  return vget_lane_s16(vqshl_s16((int16x4_t){a}, (int16x4_t){b}), 0);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshls_s32(int32_t a, int32_t b) {
  return vget_lane_s32(vqshl_s32((int32x2_t){a}, (int32x2_t){b}), 0);
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshld_s64(int64_t a, int64_t b) {
  return vget_lane_s64(vqshl_s64((int64x1_t){a}, (int64x1_t){b}), 0);
}

extern inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlb_u8(uint8_t a, uint8_t b) {
  return vget_lane_u8(vqshl_u8((uint8x8_t){a}, (uint8x8_t){b}), 0);
}

extern inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlh_u16(uint16_t a, uint16_t b) {
  return vget_lane_u16(vqshl_u16((uint16x4_t){a}, (uint16x4_t){b}), 0);
}

extern inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshls_u32(uint32_t a, uint32_t b) {
  return vget_lane_u32(vqshl_u32((uint32x2_t){a}, (uint32x2_t){b}), 0);
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshld_u64(uint64_t a, uint64_t b) {
  return vget_lane_u64(vqshl_u64((uint64x1_t){a}, (uint64x1_t){b}), 0);
}

int8x8_t __builtin_mpl_vector_qshl_n_v8i8(int8x8_t a, const int n);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshl_n_s8(int8x8_t a, const int n) {
  return __builtin_mpl_vector_qshl_n_v8i8(a, n);
}

int8x16_t __builtin_mpl_vector_qshlq_n_v16i8(int8x16_t a, const int n);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlq_n_s8(int8x16_t a, const int n) {
  return __builtin_mpl_vector_qshlq_n_v16i8(a, n);
}

int16x4_t __builtin_mpl_vector_qshl_n_v4i16(int16x4_t a, const int n);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshl_n_s16(int16x4_t a, const int n) {
  return __builtin_mpl_vector_qshl_n_v4i16(a, n);
}

int16x8_t __builtin_mpl_vector_qshlq_n_v8i16(int16x8_t a, const int n);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlq_n_s16(int16x8_t a, const int n) {
  return __builtin_mpl_vector_qshlq_n_v8i16(a, n);
}

int32x2_t __builtin_mpl_vector_qshl_n_v2i32(int32x2_t a, const int n);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshl_n_s32(int32x2_t a, const int n) {
  return __builtin_mpl_vector_qshl_n_v2i32(a, n);
}

int32x4_t __builtin_mpl_vector_qshlq_n_v4i32(int32x4_t a, const int n);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlq_n_s32(int32x4_t a, const int n) {
  return __builtin_mpl_vector_qshlq_n_v4i32(a, n);
}

int64x1_t __builtin_mpl_vector_qshl_n_v1i64(int64x1_t a, const int n);
extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshl_n_s64(int64x1_t a, const int n) {
  return __builtin_mpl_vector_qshl_n_v1i64(a, n);
}

int64x2_t __builtin_mpl_vector_qshlq_n_v2i64(int64x2_t a, const int n);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlq_n_s64(int64x2_t a, const int n) {
  return __builtin_mpl_vector_qshlq_n_v2i64(a, n);
}

uint8x8_t __builtin_mpl_vector_qshl_n_v8u8(uint8x8_t a, const int n);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshl_n_u8(uint8x8_t a, const int n) {
  return __builtin_mpl_vector_qshl_n_v8u8(a, n);
}

uint8x16_t __builtin_mpl_vector_qshlq_n_v16u8(uint8x16_t a, const int n);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlq_n_u8(uint8x16_t a, const int n) {
  return __builtin_mpl_vector_qshlq_n_v16u8(a, n);
}

uint16x4_t __builtin_mpl_vector_qshl_n_v4u16(uint16x4_t a, const int n);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshl_n_u16(uint16x4_t a, const int n) {
  return __builtin_mpl_vector_qshl_n_v4u16(a, n);
}

uint16x8_t __builtin_mpl_vector_qshlq_n_v8u16(uint16x8_t a, const int n);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlq_n_u16(uint16x8_t a, const int n) {
  return __builtin_mpl_vector_qshlq_n_v8u16(a, n);
}

uint32x2_t __builtin_mpl_vector_qshl_n_v2u32(uint32x2_t a, const int n);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshl_n_u32(uint32x2_t a, const int n) {
  return __builtin_mpl_vector_qshl_n_v2u32(a, n);
}

uint32x4_t __builtin_mpl_vector_qshlq_n_v4u32(uint32x4_t a, const int n);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlq_n_u32(uint32x4_t a, const int n) {
  return __builtin_mpl_vector_qshlq_n_v4u32(a, n);
}

uint64x1_t __builtin_mpl_vector_qshl_n_v1u64(uint64x1_t a, const int n);
extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshl_n_u64(uint64x1_t a, const int n) {
  return __builtin_mpl_vector_qshl_n_v1u64(a, n);
}

uint64x2_t __builtin_mpl_vector_qshlq_n_v2u64(uint64x2_t a, const int n);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlq_n_u64(uint64x2_t a, const int n) {
  return __builtin_mpl_vector_qshlq_n_v2u64(a, n);
}

extern inline int8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlb_n_s8(int8_t a, const int n) {
  return vget_lane_s64(vqshl_n_s8((int8x8_t){a}, n), 0);
}

extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlh_n_s16(int16_t a, const int n) {
  return vget_lane_s64(vqshl_n_s16((int16x4_t){a}, n), 0);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshls_n_s32(int32_t a, const int n) {
  return vget_lane_s64(vqshl_n_s32((int32x2_t){a}, n), 0);
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshld_n_s64(int64_t a, const int n) {
  return vget_lane_s64(vqshl_n_s64((int64x1_t){a}, n), 0);
}

extern inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlb_n_u8(uint8_t a, const int n) {
  return vget_lane_u64(vqshl_n_u8((uint8x8_t){a}, n), 0);
}

extern inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlh_n_u16(uint16_t a, const int n) {
  return vget_lane_u64(vqshl_n_u16((uint16x4_t){a}, n), 0);
}

extern inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshls_n_u32(uint32_t a, const int n) {
  return vget_lane_u64(vqshl_n_u32((uint32x2_t){a}, n), 0);
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshld_n_u64(uint64_t a, const int n) {
  return vget_lane_u64(vqshl_n_u64((uint64x1_t){a}, n), 0);
}

uint8x8_t __builtin_mpl_vector_qshlu_n_v8u8(int8x8_t a, const int n);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlu_n_s8(int8x8_t a, const int n) {
  return __builtin_mpl_vector_qshlu_n_v8u8(a, n);
}

uint8x16_t __builtin_mpl_vector_qshluq_n_v16u8(int8x16_t a, const int n);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshluq_n_s8(int8x16_t a, const int n) {
  return __builtin_mpl_vector_qshluq_n_v16u8(a, n);
}

uint16x4_t __builtin_mpl_vector_qshlu_n_v4u16(int16x4_t a, const int n);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlu_n_s16(int16x4_t a, const int n) {
  return __builtin_mpl_vector_qshlu_n_v4u16(a, n);
}

uint16x8_t __builtin_mpl_vector_qshluq_n_v8u16(int16x8_t a, const int n);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshluq_n_s16(int16x8_t a, const int n) {
  return __builtin_mpl_vector_qshluq_n_v8u16(a, n);
}

uint32x2_t __builtin_mpl_vector_qshlu_n_v2u32(int32x2_t a, const int n);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlu_n_s32(int32x2_t a, const int n) {
  return __builtin_mpl_vector_qshlu_n_v2u32(a, n);
}

uint32x4_t __builtin_mpl_vector_qshluq_n_v4u32(int32x4_t a, const int n);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshluq_n_s32(int32x4_t a, const int n) {
  return __builtin_mpl_vector_qshluq_n_v4u32(a, n);
}

uint64x1_t __builtin_mpl_vector_qshlu_n_v1u64(int64x1_t a, const int n);
extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlu_n_s64(int64x1_t a, const int n) {
  return __builtin_mpl_vector_qshlu_n_v1u64(a, n);
}

uint64x2_t __builtin_mpl_vector_qshluq_n_v2u64(int64x2_t a, const int n);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshluq_n_s64(int64x2_t a, const int n) {
  return __builtin_mpl_vector_qshluq_n_v2u64(a, n);
}

extern inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlub_n_s8(int8_t a, const int n) {
  return vget_lane_u64(vqshlu_n_s8((int8x8_t){a}, n), 0);
}

extern inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshluh_n_s16(int16_t a, const int n) {
  return vget_lane_u64(vqshlu_n_s16((int16x4_t){a}, n), 0);
}

extern inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlus_n_s32(int32_t a, const int n) {
  return vget_lane_u64(vqshlu_n_s32((int32x2_t){a}, n), 0);
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshlud_n_s64(int64_t a, const int n) {
  return vget_lane_u64(vqshlu_n_s64((int64x1_t){a}, n), 0);
}

int8x8_t __builtin_mpl_vector_rshl_v8i8(int8x8_t a, int8x8_t b);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshl_s8(int8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_rshl_v8i8(a, b);
}

int8x16_t __builtin_mpl_vector_rshlq_v16i8(int8x16_t a, int8x16_t b);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshlq_s8(int8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_rshlq_v16i8(a, b);
}

int16x4_t __builtin_mpl_vector_rshl_v4i16(int16x4_t a, int16x4_t b);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshl_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_rshl_v4i16(a, b);
}

int16x8_t __builtin_mpl_vector_rshlq_v8i16(int16x8_t a, int16x8_t b);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshlq_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_rshlq_v8i16(a, b);
}

int32x2_t __builtin_mpl_vector_rshl_v2i32(int32x2_t a, int32x2_t b);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshl_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_rshl_v2i32(a, b);
}

int32x4_t __builtin_mpl_vector_rshlq_v4i32(int32x4_t a, int32x4_t b);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshlq_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_rshlq_v4i32(a, b);
}

int64x1_t __builtin_mpl_vector_rshl_v1i64(int64x1_t a, int64x1_t b);
extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshl_s64(int64x1_t a, int64x1_t b) {
  return __builtin_mpl_vector_rshl_v1i64(a, b);
}

int64x2_t __builtin_mpl_vector_rshlq_v2i64(int64x2_t a, int64x2_t b);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshlq_s64(int64x2_t a, int64x2_t b) {
  return __builtin_mpl_vector_rshlq_v2i64(a, b);
}

uint8x8_t __builtin_mpl_vector_rshl_v8u8(uint8x8_t a, int8x8_t b);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshl_u8(uint8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_rshl_v8u8(a, b);
}

uint8x16_t __builtin_mpl_vector_rshlq_v16u8(uint8x16_t a, int8x16_t b);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshlq_u8(uint8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_rshlq_v16u8(a, b);
}

uint16x4_t __builtin_mpl_vector_rshl_v4u16(uint16x4_t a, int16x4_t b);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshl_u16(uint16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_rshl_v4u16(a, b);
}

uint16x8_t __builtin_mpl_vector_rshlq_v8u16(uint16x8_t a, int16x8_t b);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshlq_u16(uint16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_rshlq_v8u16(a, b);
}

uint32x2_t __builtin_mpl_vector_rshl_v2u32(uint32x2_t a, int32x2_t b);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshl_u32(uint32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_rshl_v2u32(a, b);
}

uint32x4_t __builtin_mpl_vector_rshlq_v4u32(uint32x4_t a, int32x4_t b);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshlq_u32(uint32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_rshlq_v4u32(a, b);
}

uint64x1_t __builtin_mpl_vector_rshl_v1u64(uint64x1_t a, int64x1_t b);
extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshl_u64(uint64x1_t a, int64x1_t b) {
  return __builtin_mpl_vector_rshl_v1u64(a, b);
}

uint64x2_t __builtin_mpl_vector_rshlq_v2u64(uint64x2_t a, int64x2_t b);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshlq_u64(uint64x2_t a, int64x2_t b) {
  return __builtin_mpl_vector_rshlq_v2u64(a, b);
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshld_s64(int64_t a, int64_t b) {
  return vget_lane_s64(vrshl_s64((int64x1_t){a}, (int64x1_t){b}), 0);
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshld_u64(uint64_t a, uint64_t b) {
  return vget_lane_u64(vrshl_u64((uint64x1_t){a}, (uint64x1_t){b}), 0);
}

int8x8_t __builtin_mpl_vector_qrshl_v8i8(int8x8_t a, int8x8_t b);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshl_s8(int8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_qrshl_v8i8(a, b);
}

int8x16_t __builtin_mpl_vector_qrshlq_v16i8(int8x16_t a, int8x16_t b);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshlq_s8(int8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_qrshlq_v16i8(a, b);
}

int16x4_t __builtin_mpl_vector_qrshl_v4i16(int16x4_t a, int16x4_t b);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshl_s16(int16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_qrshl_v4i16(a, b);
}

int16x8_t __builtin_mpl_vector_qrshlq_v8i16(int16x8_t a, int16x8_t b);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshlq_s16(int16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_qrshlq_v8i16(a, b);
}

int32x2_t __builtin_mpl_vector_qrshl_v2i32(int32x2_t a, int32x2_t b);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshl_s32(int32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_qrshl_v2i32(a, b);
}

int32x4_t __builtin_mpl_vector_qrshlq_v4i32(int32x4_t a, int32x4_t b);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshlq_s32(int32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_qrshlq_v4i32(a, b);
}

int64x1_t __builtin_mpl_vector_qrshl_v1i64(int64x1_t a, int64x1_t b);
extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshl_s64(int64x1_t a, int64x1_t b) {
  return __builtin_mpl_vector_qrshl_v1i64(a, b);
}

int64x2_t __builtin_mpl_vector_qrshlq_v2i64(int64x2_t a, int64x2_t b);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshlq_s64(int64x2_t a, int64x2_t b) {
  return __builtin_mpl_vector_qrshlq_v2i64(a, b);
}

uint8x8_t __builtin_mpl_vector_qrshl_v8u8(uint8x8_t a, int8x8_t b);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshl_u8(uint8x8_t a, int8x8_t b) {
  return __builtin_mpl_vector_qrshl_v8u8(a, b);
}

uint8x16_t __builtin_mpl_vector_qrshlq_v16u8(uint8x16_t a, int8x16_t b);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshlq_u8(uint8x16_t a, int8x16_t b) {
  return __builtin_mpl_vector_qrshlq_v16u8(a, b);
}

uint16x4_t __builtin_mpl_vector_qrshl_v4u16(uint16x4_t a, int16x4_t b);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshl_u16(uint16x4_t a, int16x4_t b) {
  return __builtin_mpl_vector_qrshl_v4u16(a, b);
}

uint16x8_t __builtin_mpl_vector_qrshlq_v8u16(uint16x8_t a, int16x8_t b);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshlq_u16(uint16x8_t a, int16x8_t b) {
  return __builtin_mpl_vector_qrshlq_v8u16(a, b);
}

uint32x2_t __builtin_mpl_vector_qrshl_v2u32(uint32x2_t a, int32x2_t b);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshl_u32(uint32x2_t a, int32x2_t b) {
  return __builtin_mpl_vector_qrshl_v2u32(a, b);
}

uint32x4_t __builtin_mpl_vector_qrshlq_v4u32(uint32x4_t a, int32x4_t b);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshlq_u32(uint32x4_t a, int32x4_t b) {
  return __builtin_mpl_vector_qrshlq_v4u32(a, b);
}

uint64x1_t __builtin_mpl_vector_qrshl_v1u64(uint64x1_t a, int64x1_t b);
extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshl_u64(uint64x1_t a, int64x1_t b) {
  return __builtin_mpl_vector_qrshl_v1u64(a, b);
}

uint64x2_t __builtin_mpl_vector_qrshlq_v2u64(uint64x2_t a, int64x2_t b);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshlq_u64(uint64x2_t a, int64x2_t b) {
  return __builtin_mpl_vector_qrshlq_v2u64(a, b);
}

extern inline int8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshlb_s8(int8_t a, int8_t b) {
  return vget_lane_s8(vqrshl_s8((int8x8_t){a}, (int8x8_t){b}), 0);
}

extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshlh_s16(int16_t a, int16_t b) {
  return vget_lane_s16(vqrshl_s16((int16x4_t){a}, (int16x4_t){b}), 0);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshls_s32(int32_t a, int32_t b) {
  return vget_lane_s32(vqrshl_s32((int32x2_t){a}, (int32x2_t){b}), 0);
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshld_s64(int64_t a, int64_t b) {
  return vget_lane_s64(vqrshl_s64((int64x1_t){a}, (int64x1_t){b}), 0);
}

extern inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshlb_u8(uint8_t a, int8_t b) {
  return vget_lane_u8(vqrshl_u8((uint8x8_t){a}, (int8x8_t){b}), 0);
}

extern inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshlh_u16(uint16_t a, int16_t b) {
  return vget_lane_u16(vqrshl_u16((uint16x4_t){a}, (int16x4_t){b}), 0);
}

extern inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshls_u32(uint32_t a, int32_t b) {
  return vget_lane_u32(vqrshl_u32((uint32x2_t){a}, (int32x2_t){b}), 0);
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshld_u64(uint64_t a, int64_t b) {
  return vget_lane_u64(vqrshl_u64((uint64x1_t){a}, (int64x1_t){b}), 0);
}

int16x8_t __builtin_mpl_vector_shll_n_v8i16(int8x8_t a, const int n);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vshll_n_s8(int8x8_t a, const int n) {
  return __builtin_mpl_vector_shll_n_v8i16(a, n);
}

int32x4_t __builtin_mpl_vector_shll_n_v4i32(int16x4_t a, const int n);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vshll_n_s16(int16x4_t a, const int n) {
  return __builtin_mpl_vector_shll_n_v4i32(a, n);
}

int64x2_t __builtin_mpl_vector_shll_n_v2i64(int32x2_t a, const int n);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vshll_n_s32(int32x2_t a, const int n) {
  return __builtin_mpl_vector_shll_n_v2i64(a, n);
}

uint16x8_t __builtin_mpl_vector_shll_n_v8u16(uint8x8_t a, const int n);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vshll_n_u8(uint8x8_t a, const int n) {
  return __builtin_mpl_vector_shll_n_v8u16(a, n);
}

uint32x4_t __builtin_mpl_vector_shll_n_v4u32(uint16x4_t a, const int n);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vshll_n_u16(uint16x4_t a, const int n) {
  return __builtin_mpl_vector_shll_n_v4u32(a, n);
}

uint64x2_t __builtin_mpl_vector_shll_n_v2u64(uint32x2_t a, const int n);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vshll_n_u32(uint32x2_t a, const int n) {
  return __builtin_mpl_vector_shll_n_v2u64(a, n);
}

int16x8_t __builtin_mpl_vector_shll_high_n_v8i16(int8x16_t a, const int n);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vshll_high_n_s8(int8x16_t a, const int n) {
  return __builtin_mpl_vector_shll_high_n_v8i16(a, n);
}

int32x4_t __builtin_mpl_vector_shll_high_n_v4i32(int16x8_t a, const int n);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vshll_high_n_s16(int16x8_t a, const int n) {
  return __builtin_mpl_vector_shll_high_n_v4i32(a, n);
}

int64x2_t __builtin_mpl_vector_shll_high_n_v2i64(int32x4_t a, const int n);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vshll_high_n_s32(int32x4_t a, const int n) {
  return __builtin_mpl_vector_shll_high_n_v2i64(a, n);
}

uint16x8_t __builtin_mpl_vector_shll_high_n_v8u16(uint8x16_t a, const int n);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vshll_high_n_u8(uint8x16_t a,
                                                                                             const int n) {
  return __builtin_mpl_vector_shll_high_n_v8u16(a, n);
}

uint32x4_t __builtin_mpl_vector_shll_high_n_v4u32(uint16x8_t a, const int n);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vshll_high_n_u16(uint16x8_t a,
                                                                                              const int n) {
  return __builtin_mpl_vector_shll_high_n_v4u32(a, n);
}

uint64x2_t __builtin_mpl_vector_shll_high_n_v2u64(uint32x4_t a, const int n);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vshll_high_n_u32(uint32x4_t a,
                                                                                              const int n) {
  return __builtin_mpl_vector_shll_high_n_v2u64(a, n);
}

int8x8_t __builtin_mpl_vector_sli_n_v8i8(int8x8_t a, int8x8_t b, const int n);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsli_n_s8(int8x8_t a, int8x8_t b,
                                                                                     const int n) {
  return __builtin_mpl_vector_sli_n_v8i8(a, b, n);
}

int8x16_t __builtin_mpl_vector_sliq_n_v16i8(int8x16_t a, int8x16_t b, const int n);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vsliq_n_s8(int8x16_t a, int8x16_t b,
                                                                                       const int n) {
  return __builtin_mpl_vector_sliq_n_v16i8(a, b, n);
}

int16x4_t __builtin_mpl_vector_sli_n_v4i16(int16x4_t a, int16x4_t b, const int n);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsli_n_s16(int16x4_t a, int16x4_t b,
                                                                                       const int n) {
  return __builtin_mpl_vector_sli_n_v4i16(a, b, n);
}

int16x8_t __builtin_mpl_vector_sliq_n_v8i16(int16x8_t a, int16x8_t b, const int n);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsliq_n_s16(int16x8_t a, int16x8_t b,
                                                                                        const int n) {
  return __builtin_mpl_vector_sliq_n_v8i16(a, b, n);
}

int32x2_t __builtin_mpl_vector_sli_n_v2i32(int32x2_t a, int32x2_t b, const int n);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsli_n_s32(int32x2_t a, int32x2_t b,
                                                                                       const int n) {
  return __builtin_mpl_vector_sli_n_v2i32(a, b, n);
}

int32x4_t __builtin_mpl_vector_sliq_n_v4i32(int32x4_t a, int32x4_t b, const int n);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsliq_n_s32(int32x4_t a, int32x4_t b,
                                                                                        const int n) {
  return __builtin_mpl_vector_sliq_n_v4i32(a, b, n);
}

int64x1_t __builtin_mpl_vector_sli_n_v1i64(int64x1_t a, int64x1_t b, const int n);
extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vsli_n_s64(int64x1_t a, int64x1_t b,
                                                                                       const int n) {
  return __builtin_mpl_vector_sli_n_v1i64(a, b, n);
}

int64x2_t __builtin_mpl_vector_sliq_n_v2i64(int64x2_t a, int64x2_t b, const int n);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsliq_n_s64(int64x2_t a, int64x2_t b,
                                                                                        const int n) {
  return __builtin_mpl_vector_sliq_n_v2i64(a, b, n);
}

uint8x8_t __builtin_mpl_vector_sli_n_v8u8(uint8x8_t a, uint8x8_t b, const int n);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsli_n_u8(uint8x8_t a, uint8x8_t b,
                                                                                      const int n) {
  return __builtin_mpl_vector_sli_n_v8u8(a, b, n);
}

uint8x16_t __builtin_mpl_vector_sliq_n_v16u8(uint8x16_t a, uint8x16_t b, const int n);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vsliq_n_u8(uint8x16_t a, uint8x16_t b,
                                                                                        const int n) {
  return __builtin_mpl_vector_sliq_n_v16u8(a, b, n);
}

uint16x4_t __builtin_mpl_vector_sli_n_v4u16(uint16x4_t a, uint16x4_t b, const int n);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsli_n_u16(uint16x4_t a, uint16x4_t b,
                                                                                        const int n) {
  return __builtin_mpl_vector_sli_n_v4u16(a, b, n);
}

uint16x8_t __builtin_mpl_vector_sliq_n_v8u16(uint16x8_t a, uint16x8_t b, const int n);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsliq_n_u16(uint16x8_t a, uint16x8_t b,
                                                                                         const int n) {
  return __builtin_mpl_vector_sliq_n_v8u16(a, b, n);
}

uint32x2_t __builtin_mpl_vector_sli_n_v2u32(uint32x2_t a, uint32x2_t b, const int n);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsli_n_u32(uint32x2_t a, uint32x2_t b,
                                                                                        const int n) {
  return __builtin_mpl_vector_sli_n_v2u32(a, b, n);
}

uint32x4_t __builtin_mpl_vector_sliq_n_v4u32(uint32x4_t a, uint32x4_t b, const int n);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsliq_n_u32(uint32x4_t a, uint32x4_t b,
                                                                                         const int n) {
  return __builtin_mpl_vector_sliq_n_v4u32(a, b, n);
}

uint64x1_t __builtin_mpl_vector_sli_n_v1u64(uint64x1_t a, uint64x1_t b, const int n);
extern inline uint64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vsli_n_u64(uint64x1_t a, uint64x1_t b,
                                                                                        const int n) {
  return __builtin_mpl_vector_sli_n_v1u64(a, b, n);
}

uint64x2_t __builtin_mpl_vector_sliq_n_v2u64(uint64x2_t a, uint64x2_t b, const int n);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsliq_n_u64(uint64x2_t a, uint64x2_t b,
                                                                                         const int n) {
  return __builtin_mpl_vector_sliq_n_v2u64(a, b, n);
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vslid_n_s64(int64_t a, int64_t b,
                                                                                      const int n) {
  return vget_lane_s64(vsli_n_s64((int64x1_t){a}, (int64x1_t){b}, n), 0);
}

extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vslid_n_u64(uint64_t a, uint64_t b,
                                                                                       const int n) {
  return vget_lane_u64(vsli_n_u64((uint64x1_t){a}, (uint64x1_t){b}, n), 0);
}

int8x8_t __builtin_mpl_vector_rshr_n_v8i8(int8x8_t a, const int n);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshr_n_s8(int8x8_t a, const int n) {
  return __builtin_mpl_vector_rshr_n_v8i8(a, n);
}

int8x16_t __builtin_mpl_vector_rshrq_n_v16i8(int8x16_t a, const int n);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshrq_n_s8(int8x16_t a, const int n) {
  return __builtin_mpl_vector_rshrq_n_v16i8(a, n);
}

int16x4_t __builtin_mpl_vector_rshr_n_v4i16(int16x4_t a, const int n);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshr_n_s16(int16x4_t a, const int n) {
  return __builtin_mpl_vector_rshr_n_v4i16(a, n);
}

int16x8_t __builtin_mpl_vector_rshrq_n_v8i16(int16x8_t a, const int n);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshrq_n_s16(int16x8_t a, const int n) {
  return __builtin_mpl_vector_rshrq_n_v8i16(a, n);
}

int32x2_t __builtin_mpl_vector_rshr_n_v2i32(int32x2_t a, const int n);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshr_n_s32(int32x2_t a, const int n) {
  return __builtin_mpl_vector_rshr_n_v2i32(a, n);
}

int32x4_t __builtin_mpl_vector_rshrq_n_v4i32(int32x4_t a, const int n);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshrq_n_s32(int32x4_t a, const int n) {
  return __builtin_mpl_vector_rshrq_n_v4i32(a, n);
}

int64x2_t __builtin_mpl_vector_rshrq_n_v2i64(int64x2_t a, const int n);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshrq_n_s64(int64x2_t a, const int n) {
  return __builtin_mpl_vector_rshrq_n_v2i64(a, n);
}

uint8x8_t __builtin_mpl_vector_rshr_n_v8u8(uint8x8_t a, const int n);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshr_n_u8(uint8x8_t a, const int n) {
  return __builtin_mpl_vector_rshr_n_v8u8(a, n);
}

uint8x16_t __builtin_mpl_vector_rshrq_n_v16u8(uint8x16_t a, const int n);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshrq_n_u8(uint8x16_t a, const int n) {
  return __builtin_mpl_vector_rshrq_n_v16u8(a, n);
}

uint16x4_t __builtin_mpl_vector_rshr_n_v4u16(uint16x4_t a, const int n);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshr_n_u16(uint16x4_t a, const int n) {
  return __builtin_mpl_vector_rshr_n_v4u16(a, n);
}

uint16x8_t __builtin_mpl_vector_rshrq_n_v8u16(uint16x8_t a, const int n);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshrq_n_u16(uint16x8_t a, const int n) {
  return __builtin_mpl_vector_rshrq_n_v8u16(a, n);
}

uint32x2_t __builtin_mpl_vector_rshr_n_v2u32(uint32x2_t a, const int n);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshr_n_u32(uint32x2_t a, const int n) {
  return __builtin_mpl_vector_rshr_n_v2u32(a, n);
}

uint32x4_t __builtin_mpl_vector_rshrq_n_v4u32(uint32x4_t a, const int n);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshrq_n_u32(uint32x4_t a, const int n) {
  return __builtin_mpl_vector_rshrq_n_v4u32(a, n);
}

uint64x2_t __builtin_mpl_vector_rshrq_n_v2u64(uint64x2_t a, const int n);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshrq_n_u64(uint64x2_t a, const int n) {
  return __builtin_mpl_vector_rshrq_n_v2u64(a, n);
}

int64x1_t __builtin_mpl_vector_rshrd_n_i64(int64x1_t a, const int n);
extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshrd_n_s64(int64_t a, const int n) {
  return vget_lane_s64(__builtin_mpl_vector_rshrd_n_i64((int64x1_t){a}, n), 0);
}

uint64x1_t __builtin_mpl_vector_rshrd_n_u64(uint64x1_t a, const int n);
extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshrd_n_u64(uint64_t a, const int n) {
  return vget_lane_u64(__builtin_mpl_vector_rshrd_n_u64((uint64x1_t){a}, n), 0);
}

int8x8_t __builtin_mpl_vector_sra_n_v8i8(int8x8_t a, int8x8_t b, const int n);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsra_n_s8(int8x8_t a, int8x8_t b,
                                                                                     const int n) {
  return __builtin_mpl_vector_sra_n_v8i8(a, b, n);
}

int8x16_t __builtin_mpl_vector_sraq_n_v16i8(int8x16_t a, int8x16_t b, const int n);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vsraq_n_s8(int8x16_t a, int8x16_t b,
                                                                                       const int n) {
  return __builtin_mpl_vector_sraq_n_v16i8(a, b, n);
}

int16x4_t __builtin_mpl_vector_sra_n_v4i16(int16x4_t a, int16x4_t b, const int n);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsra_n_s16(int16x4_t a, int16x4_t b,
                                                                                       const int n) {
  return __builtin_mpl_vector_sra_n_v4i16(a, b, n);
}

int16x8_t __builtin_mpl_vector_sraq_n_v8i16(int16x8_t a, int16x8_t b, const int n);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsraq_n_s16(int16x8_t a, int16x8_t b,
                                                                                        const int n) {
  return __builtin_mpl_vector_sraq_n_v8i16(a, b, n);
}

int32x2_t __builtin_mpl_vector_sra_n_v2i32(int32x2_t a, int32x2_t b, const int n);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsra_n_s32(int32x2_t a, int32x2_t b,
                                                                                       const int n) {
  return __builtin_mpl_vector_sra_n_v2i32(a, b, n);
}

int32x4_t __builtin_mpl_vector_sraq_n_v4i32(int32x4_t a, int32x4_t b, const int n);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsraq_n_s32(int32x4_t a, int32x4_t b,
                                                                                        const int n) {
  return __builtin_mpl_vector_sraq_n_v4i32(a, b, n);
}

int64x2_t __builtin_mpl_vector_sraq_n_v2i64(int64x2_t a, int64x2_t b, const int n);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsraq_n_s64(int64x2_t a, int64x2_t b,
                                                                                        const int n) {
  return __builtin_mpl_vector_sraq_n_v2i64(a, b, n);
}

uint8x8_t __builtin_mpl_vector_sra_n_v8u8(uint8x8_t a, uint8x8_t b, const int n);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsra_n_u8(uint8x8_t a, uint8x8_t b,
                                                                                      const int n) {
  return __builtin_mpl_vector_sra_n_v8u8(a, b, n);
}

uint8x16_t __builtin_mpl_vector_sraq_n_v16u8(uint8x16_t a, uint8x16_t b, const int n);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vsraq_n_u8(uint8x16_t a, uint8x16_t b,
                                                                                        const int n) {
  return __builtin_mpl_vector_sraq_n_v16u8(a, b, n);
}

uint16x4_t __builtin_mpl_vector_sra_n_v4u16(uint16x4_t a, uint16x4_t b, const int n);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsra_n_u16(uint16x4_t a, uint16x4_t b,
                                                                                        const int n) {
  return __builtin_mpl_vector_sra_n_v4u16(a, b, n);
}

uint16x8_t __builtin_mpl_vector_sraq_n_v8u16(uint16x8_t a, uint16x8_t b, const int n);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsraq_n_u16(uint16x8_t a, uint16x8_t b,
                                                                                         const int n) {
  return __builtin_mpl_vector_sraq_n_v8u16(a, b, n);
}

uint32x2_t __builtin_mpl_vector_sra_n_v2u32(uint32x2_t a, uint32x2_t b, const int n);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsra_n_u32(uint32x2_t a, uint32x2_t b,
                                                                                        const int n) {
  return __builtin_mpl_vector_sra_n_v2u32(a, b, n);
}

uint32x4_t __builtin_mpl_vector_sraq_n_v4u32(uint32x4_t a, uint32x4_t b, const int n);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsraq_n_u32(uint32x4_t a, uint32x4_t b,
                                                                                         const int n) {
  return __builtin_mpl_vector_sraq_n_v4u32(a, b, n);
}

uint64x2_t __builtin_mpl_vector_sraq_n_v2u64(uint64x2_t a, uint64x2_t b, const int n);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsraq_n_u64(uint64x2_t a, uint64x2_t b,
                                                                                         const int n) {
  return __builtin_mpl_vector_sraq_n_v2u64(a, b, n);
}

int64x1_t __builtin_mpl_vector_srad_n_i64(int64x1_t a, int64x1_t b, const int n);
extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vsrad_n_s64(int64_t a, int64_t b,
                                                                                      const int n) {
  return vget_lane_s64(__builtin_mpl_vector_srad_n_i64((int64x1_t){a}, (int64x1_t){b}, n), 0);
}

uint64x1_t __builtin_mpl_vector_srad_n_u64(uint64x1_t a, uint64x1_t b, const int n);
extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vsrad_n_u64(uint64_t a, uint64_t b,
                                                                                       const int n) {
  return vget_lane_u64(__builtin_mpl_vector_srad_n_u64((uint64x1_t){a}, (uint64x1_t){b}, n), 0);
}

int8x8_t __builtin_mpl_vector_rsra_n_v8i8(int8x8_t a, int8x8_t b, const int n);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsra_n_s8(int8x8_t a, int8x8_t b,
                                                                                      const int n) {
  return __builtin_mpl_vector_rsra_n_v8i8(a, b, n);
}

int8x16_t __builtin_mpl_vector_rsraq_n_v16i8(int8x16_t a, int8x16_t b, const int n);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsraq_n_s8(int8x16_t a, int8x16_t b,
                                                                                        const int n) {
  return __builtin_mpl_vector_rsraq_n_v16i8(a, b, n);
}

int16x4_t __builtin_mpl_vector_rsra_n_v4i16(int16x4_t a, int16x4_t b, const int n);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsra_n_s16(int16x4_t a, int16x4_t b,
                                                                                        const int n) {
  return __builtin_mpl_vector_rsra_n_v4i16(a, b, n);
}

int16x8_t __builtin_mpl_vector_rsraq_n_v8i16(int16x8_t a, int16x8_t b, const int n);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsraq_n_s16(int16x8_t a, int16x8_t b,
                                                                                         const int n) {
  return __builtin_mpl_vector_rsraq_n_v8i16(a, b, n);
}

int32x2_t __builtin_mpl_vector_rsra_n_v2i32(int32x2_t a, int32x2_t b, const int n);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsra_n_s32(int32x2_t a, int32x2_t b,
                                                                                        const int n) {
  return __builtin_mpl_vector_rsra_n_v2i32(a, b, n);
}

int32x4_t __builtin_mpl_vector_rsraq_n_v4i32(int32x4_t a, int32x4_t b, const int n);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsraq_n_s32(int32x4_t a, int32x4_t b,
                                                                                         const int n) {
  return __builtin_mpl_vector_rsraq_n_v4i32(a, b, n);
}

int64x2_t __builtin_mpl_vector_rsraq_n_v2i64(int64x2_t a, int64x2_t b, const int n);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsraq_n_s64(int64x2_t a, int64x2_t b,
                                                                                         const int n) {
  return __builtin_mpl_vector_rsraq_n_v2i64(a, b, n);
}

uint8x8_t __builtin_mpl_vector_rsra_n_v8u8(uint8x8_t a, uint8x8_t b, const int n);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsra_n_u8(uint8x8_t a, uint8x8_t b,
                                                                                       const int n) {
  return __builtin_mpl_vector_rsra_n_v8u8(a, b, n);
}

uint8x16_t __builtin_mpl_vector_rsraq_n_v16u8(uint8x16_t a, uint8x16_t b, const int n);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsraq_n_u8(uint8x16_t a, uint8x16_t b,
                                                                                         const int n) {
  return __builtin_mpl_vector_rsraq_n_v16u8(a, b, n);
}

uint16x4_t __builtin_mpl_vector_rsra_n_v4u16(uint16x4_t a, uint16x4_t b, const int n);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsra_n_u16(uint16x4_t a, uint16x4_t b,
                                                                                         const int n) {
  return __builtin_mpl_vector_rsra_n_v4u16(a, b, n);
}

uint16x8_t __builtin_mpl_vector_rsraq_n_v8u16(uint16x8_t a, uint16x8_t b, const int n);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsraq_n_u16(uint16x8_t a, uint16x8_t b,
                                                                                          const int n) {
  return __builtin_mpl_vector_rsraq_n_v8u16(a, b, n);
}

uint32x2_t __builtin_mpl_vector_rsra_n_v2u32(uint32x2_t a, uint32x2_t b, const int n);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsra_n_u32(uint32x2_t a, uint32x2_t b,
                                                                                         const int n) {
  return __builtin_mpl_vector_rsra_n_v2u32(a, b, n);
}

uint32x4_t __builtin_mpl_vector_rsraq_n_v4u32(uint32x4_t a, uint32x4_t b, const int n);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsraq_n_u32(uint32x4_t a, uint32x4_t b,
                                                                                          const int n) {
  return __builtin_mpl_vector_rsraq_n_v4u32(a, b, n);
}

uint64x2_t __builtin_mpl_vector_rsraq_n_v2u64(uint64x2_t a, uint64x2_t b, const int n);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsraq_n_u64(uint64x2_t a, uint64x2_t b,
                                                                                          const int n) {
  return __builtin_mpl_vector_rsraq_n_v2u64(a, b, n);
}

int64x1_t __builtin_mpl_vector_rsrad_n_i64(int64x1_t a, int64x1_t b, const int n);
extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsrad_n_s64(int64_t a, int64_t b,
                                                                                       const int n) {
  return vget_lane_s64(__builtin_mpl_vector_rsrad_n_i64((int64x1_t){a}, (int64x1_t){b}, n), 0);
}

uint64x1_t __builtin_mpl_vector_rsrad_n_u64(uint64x1_t a, uint64x1_t b, const int n);
extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vrsrad_n_u64(uint64_t a, uint64_t b,
                                                                                        const int n) {
  return vget_lane_u64(__builtin_mpl_vector_rsrad_n_u64((uint64x1_t){a}, (uint64x1_t){b}, n), 0);
}

int8x8_t __builtin_mpl_vector_shrn_n_v8i8(int16x8_t a, const int n);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrn_n_s16(int16x8_t a, const int n) {
  return __builtin_mpl_vector_shrn_n_v8i8(a, n);
}

int16x4_t __builtin_mpl_vector_shrn_n_v4i16(int32x4_t a, const int n);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrn_n_s32(int32x4_t a, const int n) {
  return __builtin_mpl_vector_shrn_n_v4i16(a, n);
}

int32x2_t __builtin_mpl_vector_shrn_n_v2i32(int64x2_t a, const int n);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrn_n_s64(int64x2_t a, const int n) {
  return __builtin_mpl_vector_shrn_n_v2i32(a, n);
}

uint8x8_t __builtin_mpl_vector_shrn_n_v8u8(uint16x8_t a, const int n);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrn_n_u16(uint16x8_t a, const int n) {
  return __builtin_mpl_vector_shrn_n_v8u8(a, n);
}

uint16x4_t __builtin_mpl_vector_shrn_n_v4u16(uint32x4_t a, const int n);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrn_n_u32(uint32x4_t a, const int n) {
  return __builtin_mpl_vector_shrn_n_v4u16(a, n);
}

uint32x2_t __builtin_mpl_vector_shrn_n_v2u32(uint64x2_t a, const int n);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrn_n_u64(uint64x2_t a, const int n) {
  return __builtin_mpl_vector_shrn_n_v2u32(a, n);
}

int8x16_t __builtin_mpl_vector_shrn_high_n_v16i8(int8x8_t r, int16x8_t a, const int n);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrn_high_n_s16(int8x8_t r, int16x8_t a,
                                                                                             const int n) {
  return __builtin_mpl_vector_shrn_high_n_v16i8(r, a, n);
}

int16x8_t __builtin_mpl_vector_shrn_high_n_v8i16(int16x4_t r, int32x4_t a, const int n);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrn_high_n_s32(int16x4_t r, int32x4_t a,
                                                                                             const int n) {
  return __builtin_mpl_vector_shrn_high_n_v8i16(r, a, n);
}

int32x4_t __builtin_mpl_vector_shrn_high_n_v4i32(int32x2_t r, int64x2_t a, const int n);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrn_high_n_s64(int32x2_t r, int64x2_t a,
                                                                                             const int n) {
  return __builtin_mpl_vector_shrn_high_n_v4i32(r, a, n);
}

uint8x16_t __builtin_mpl_vector_shrn_high_n_v16u8(uint8x8_t r, uint16x8_t a, const int n);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrn_high_n_u16(uint8x8_t r, uint16x8_t a,
                                                                                              const int n) {
  return __builtin_mpl_vector_shrn_high_n_v16u8(r, a, n);
}

uint16x8_t __builtin_mpl_vector_shrn_high_n_v8u16(uint16x4_t r, uint32x4_t a, const int n);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrn_high_n_u32(uint16x4_t r,
                                                                                              uint32x4_t a,
                                                                                              const int n) {
  return __builtin_mpl_vector_shrn_high_n_v8u16(r, a, n);
}

uint32x4_t __builtin_mpl_vector_shrn_high_n_v4u32(uint32x2_t r, uint64x2_t a, const int n);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vshrn_high_n_u64(uint32x2_t r,
                                                                                              uint64x2_t a,
                                                                                              const int n) {
  return __builtin_mpl_vector_shrn_high_n_v4u32(r, a, n);
}

uint8x8_t __builtin_mpl_vector_qshrun_n_v8u8(int16x8_t a, const int n);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshrun_n_s16(int16x8_t a, const int n) {
  return __builtin_mpl_vector_qshrun_n_v8u8(a, n);
}

uint16x4_t __builtin_mpl_vector_qshrun_n_v4u16(int32x4_t a, const int n);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshrun_n_s32(int32x4_t a, const int n) {
  return __builtin_mpl_vector_qshrun_n_v4u16(a, n);
}

uint32x2_t __builtin_mpl_vector_qshrun_n_v2u32(int64x2_t a, const int n);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshrun_n_s64(int64x2_t a, const int n) {
  return __builtin_mpl_vector_qshrun_n_v2u32(a, n);
}

extern inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshrunh_n_s16(int16_t a, const int n) {
  return vget_lane_u8(vqshrun_n_s16((int16x8_t){a}, n), 0);
}

extern inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshruns_n_s32(int32_t a, const int n) {
  return vget_lane_u16(vqshrun_n_s32((int32x4_t){a}, n), 0);
}

extern inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshrund_n_s64(int64_t a, const int n) {
  return vget_lane_u32(vqshrun_n_s64((int64x2_t){a}, n), 0);
}

uint8x16_t __builtin_mpl_vector_qshrun_high_n_v16u8(uint8x8_t r, int16x8_t a, const int n);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshrun_high_n_s16(uint8x8_t r,
                                                                                                int16x8_t a,
                                                                                                const int n) {
  return __builtin_mpl_vector_qshrun_high_n_v16u8(r, a, n);
}

uint16x8_t __builtin_mpl_vector_qshrun_high_n_v8u16(uint16x4_t r, int32x4_t a, const int n);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshrun_high_n_s32(uint16x4_t r,
                                                                                                int32x4_t a,
                                                                                                const int n) {
  return __builtin_mpl_vector_qshrun_high_n_v8u16(r, a, n);
}

uint32x4_t __builtin_mpl_vector_qshrun_high_n_v4u32(uint32x2_t r, int64x2_t a, const int n);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshrun_high_n_s64(uint32x2_t r,
                                                                                                int64x2_t a,
                                                                                                const int n) {
  return __builtin_mpl_vector_qshrun_high_n_v4u32(r, a, n);
}

int8x8_t __builtin_mpl_vector_qshrn_n_v8i8(int16x8_t a, const int n);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshrn_n_s16(int16x8_t a, const int n) {
  return __builtin_mpl_vector_qshrn_n_v8i8(a, n);
}

int16x4_t __builtin_mpl_vector_qshrn_n_v4i16(int32x4_t a, const int n);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshrn_n_s32(int32x4_t a, const int n) {
  return __builtin_mpl_vector_qshrn_n_v4i16(a, n);
}

int32x2_t __builtin_mpl_vector_qshrn_n_v2i32(int64x2_t a, const int n);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshrn_n_s64(int64x2_t a, const int n) {
  return __builtin_mpl_vector_qshrn_n_v2i32(a, n);
}

uint8x8_t __builtin_mpl_vector_qshrn_n_v8u8(uint16x8_t a, const int n);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshrn_n_u16(uint16x8_t a, const int n) {
  return __builtin_mpl_vector_qshrn_n_v8u8(a, n);
}

uint16x4_t __builtin_mpl_vector_qshrn_n_v4u16(uint32x4_t a, const int n);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshrn_n_u32(uint32x4_t a, const int n) {
  return __builtin_mpl_vector_qshrn_n_v4u16(a, n);
}

uint32x2_t __builtin_mpl_vector_qshrn_n_v2u32(uint64x2_t a, const int n);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshrn_n_u64(uint64x2_t a, const int n) {
  return __builtin_mpl_vector_qshrn_n_v2u32(a, n);
}

extern inline int8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshrnh_n_s16(int16_t a, const int n) {
  return vget_lane_s8(vqshrn_n_s16((int16x8_t){a}, n), 0);
}

extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshrns_n_s32(int32_t a, const int n) {
  return vget_lane_s16(vqshrn_n_s32((int32x4_t){a}, n), 0);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshrnd_n_s64(int64_t a, const int n) {
  return vget_lane_s32(vqshrn_n_s64((int64x2_t){a}, n), 0);
}

extern inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshrnh_n_u16(uint16_t a, const int n) {
  return vget_lane_u8(vqshrn_n_u16((uint16x8_t){a}, n), 0);
}

extern inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshrns_n_u32(uint32_t a, const int n) {
  return vget_lane_u16(vqshrn_n_u32((uint32x4_t){a}, n), 0);
}

extern inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshrnd_n_u64(uint64_t a, const int n) {
  return vget_lane_u32(vqshrn_n_u64((uint64x2_t){a}, n), 0);
}

int8x16_t __builtin_mpl_vector_qshrn_high_n_v16i8(int8x8_t r, int16x8_t a, const int n);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshrn_high_n_s16(int8x8_t r, int16x8_t a,
                                                                                              const int n) {
  return __builtin_mpl_vector_qshrn_high_n_v16i8(r, a, n);
}

int16x8_t __builtin_mpl_vector_qshrn_high_n_v8i16(int16x4_t r, int32x4_t a, const int n);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshrn_high_n_s32(int16x4_t r, int32x4_t a,
                                                                                              const int n) {
  return __builtin_mpl_vector_qshrn_high_n_v8i16(r, a, n);
}

int32x4_t __builtin_mpl_vector_qshrn_high_n_v4i32(int32x2_t r, int64x2_t a, const int n);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshrn_high_n_s64(int32x2_t r, int64x2_t a,
                                                                                              const int n) {
  return __builtin_mpl_vector_qshrn_high_n_v4i32(r, a, n);
}

uint8x16_t __builtin_mpl_vector_qshrn_high_n_v16u8(uint8x8_t r, uint16x8_t a, const int n);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshrn_high_n_u16(uint8x8_t r,
                                                                                               uint16x8_t a,
                                                                                               const int n) {
  return __builtin_mpl_vector_qshrn_high_n_v16u8(r, a, n);
}

uint16x8_t __builtin_mpl_vector_qshrn_high_n_v8u16(uint16x4_t r, uint32x4_t a, const int n);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshrn_high_n_u32(uint16x4_t r,
                                                                                               uint32x4_t a,
                                                                                               const int n) {
  return __builtin_mpl_vector_qshrn_high_n_v8u16(r, a, n);
}

uint32x4_t __builtin_mpl_vector_qshrn_high_n_v4u32(uint32x2_t r, uint64x2_t a, const int n);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqshrn_high_n_u64(uint32x2_t r,
                                                                                               uint64x2_t a,
                                                                                               const int n) {
  return __builtin_mpl_vector_qshrn_high_n_v4u32(r, a, n);
}

uint8x8_t __builtin_mpl_vector_qrshrun_n_v8u8(int16x8_t a, const int n);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshrun_n_s16(int16x8_t a, const int n) {
  return __builtin_mpl_vector_qrshrun_n_v8u8(a, n);
}

uint16x4_t __builtin_mpl_vector_qrshrun_n_v4u16(int32x4_t a, const int n);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshrun_n_s32(int32x4_t a, const int n) {
  return __builtin_mpl_vector_qrshrun_n_v4u16(a, n);
}

uint32x2_t __builtin_mpl_vector_qrshrun_n_v2u32(int64x2_t a, const int n);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshrun_n_s64(int64x2_t a, const int n) {
  return __builtin_mpl_vector_qrshrun_n_v2u32(a, n);
}

extern inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshrunh_n_s16(int16_t a, const int n) {
  return vget_lane_u8(vqrshrun_n_s16((int16x8_t){a}, n), 0);
}

extern inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshruns_n_s32(int32_t a, const int n) {
  return vget_lane_u16(vqrshrun_n_s32((int32x4_t){a}, n), 0);
}

extern inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshrund_n_s64(int64_t a, const int n) {
  return vget_lane_u32(vqrshrun_n_s64((int64x2_t){a}, n), 0);
}

uint8x16_t __builtin_mpl_vector_qrshrun_high_n_v16u8(uint8x8_t r, int16x8_t a, const int n);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshrun_high_n_s16(uint8x8_t r,
                                                                                                 int16x8_t a,
                                                                                                 const int n) {
  return __builtin_mpl_vector_qrshrun_high_n_v16u8(r, a, n);
}

uint16x8_t __builtin_mpl_vector_qrshrun_high_n_v8u16(uint16x4_t r, int32x4_t a, const int n);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshrun_high_n_s32(uint16x4_t r,
                                                                                                 int32x4_t a,
                                                                                                 const int n) {
  return __builtin_mpl_vector_qrshrun_high_n_v8u16(r, a, n);
}

uint32x4_t __builtin_mpl_vector_qrshrun_high_n_v4u32(uint32x2_t r, int64x2_t a, const int n);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshrun_high_n_s64(uint32x2_t r,
                                                                                                 int64x2_t a,
                                                                                                 const int n) {
  return __builtin_mpl_vector_qrshrun_high_n_v4u32(r, a, n);
}

int8x8_t __builtin_mpl_vector_qrshrn_n_v8i8(int16x8_t a, const int n);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshrn_n_s16(int16x8_t a, const int n) {
  return __builtin_mpl_vector_qrshrn_n_v8i8(a, n);
}

int16x4_t __builtin_mpl_vector_qrshrn_n_v4i16(int32x4_t a, const int n);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshrn_n_s32(int32x4_t a, const int n) {
  return __builtin_mpl_vector_qrshrn_n_v4i16(a, n);
}

int32x2_t __builtin_mpl_vector_qrshrn_n_v2i32(int64x2_t a, const int n);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshrn_n_s64(int64x2_t a, const int n) {
  return __builtin_mpl_vector_qrshrn_n_v2i32(a, n);
}

uint8x8_t __builtin_mpl_vector_qrshrn_n_v8u8(uint16x8_t a, const int n);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshrn_n_u16(uint16x8_t a, const int n) {
  return __builtin_mpl_vector_qrshrn_n_v8u8(a, n);
}

uint16x4_t __builtin_mpl_vector_qrshrn_n_v4u16(uint32x4_t a, const int n);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshrn_n_u32(uint32x4_t a, const int n) {
  return __builtin_mpl_vector_qrshrn_n_v4u16(a, n);
}

uint32x2_t __builtin_mpl_vector_qrshrn_n_v2u32(uint64x2_t a, const int n);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshrn_n_u64(uint64x2_t a, const int n) {
  return __builtin_mpl_vector_qrshrn_n_v2u32(a, n);
}

extern inline int8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshrnh_n_s16(int16_t a, const int n) {
  return vget_lane_s8(vqrshrn_n_s16((int16x8_t){a}, n), 0);
}

extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshrns_n_s32(int32_t a, const int n) {
  return vget_lane_s16(vqrshrn_n_s32((int32x4_t){a}, n), 0);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshrnd_n_s64(int64_t a, const int n) {
  return vget_lane_s32(vqrshrn_n_s64((int64x2_t){a}, n), 0);
}

extern inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshrnh_n_u16(uint16_t a, const int n) {
  return vget_lane_u8(vqrshrn_n_u16((uint16x8_t){a}, n), 0);
}

extern inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshrns_n_u32(uint32_t a, const int n) {
  return vget_lane_u16(vqrshrn_n_u32((uint32x4_t){a}, n), 0);
}

extern inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshrnd_n_u64(uint64_t a, const int n) {
  return vget_lane_u32(vqrshrn_n_u64((uint64x2_t){a}, n), 0);
}

int8x16_t __builtin_mpl_vector_qrshrn_high_n_v16i8(int8x8_t r, int16x8_t a, const int n);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshrn_high_n_s16(int8x8_t r, int16x8_t a,
                                                                                               const int n) {
  return __builtin_mpl_vector_qrshrn_high_n_v16i8(r, a, n);
}

int16x8_t __builtin_mpl_vector_qrshrn_high_n_v8i16(int16x4_t r, int32x4_t a, const int n);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshrn_high_n_s32(int16x4_t r, int32x4_t a,
                                                                                               const int n) {
  return __builtin_mpl_vector_qrshrn_high_n_v8i16(r, a, n);
}

int32x4_t __builtin_mpl_vector_qrshrn_high_n_v4i32(int32x2_t r, int64x2_t a, const int n);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshrn_high_n_s64(int32x2_t r, int64x2_t a,
                                                                                               const int n) {
  return __builtin_mpl_vector_qrshrn_high_n_v4i32(r, a, n);
}

uint8x16_t __builtin_mpl_vector_qrshrn_high_n_v16u8(uint8x8_t r, uint16x8_t a, const int n);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshrn_high_n_u16(uint8x8_t r,
                                                                                                uint16x8_t a,
                                                                                                const int n) {
  return __builtin_mpl_vector_qrshrn_high_n_v16u8(r, a, n);
}

uint16x8_t __builtin_mpl_vector_qrshrn_high_n_v8u16(uint16x4_t r, uint32x4_t a, const int n);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshrn_high_n_u32(uint16x4_t r,
                                                                                                uint32x4_t a,
                                                                                                const int n) {
  return __builtin_mpl_vector_qrshrn_high_n_v8u16(r, a, n);
}

uint32x4_t __builtin_mpl_vector_qrshrn_high_n_v4u32(uint32x2_t r, uint64x2_t a, const int n);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqrshrn_high_n_u64(uint32x2_t r,
                                                                                                uint64x2_t a,
                                                                                                const int n) {
  return __builtin_mpl_vector_qrshrn_high_n_v4u32(r, a, n);
}

int8x8_t __builtin_mpl_vector_rshrn_n_v8i8(int16x8_t a, const int n);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshrn_n_s16(int16x8_t a, const int n) {
  return __builtin_mpl_vector_rshrn_n_v8i8(a, n);
}

int16x4_t __builtin_mpl_vector_rshrn_n_v4i16(int32x4_t a, const int n);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshrn_n_s32(int32x4_t a, const int n) {
  return __builtin_mpl_vector_rshrn_n_v4i16(a, n);
}

int32x2_t __builtin_mpl_vector_rshrn_n_v2i32(int64x2_t a, const int n);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshrn_n_s64(int64x2_t a, const int n) {
  return __builtin_mpl_vector_rshrn_n_v2i32(a, n);
}

uint8x8_t __builtin_mpl_vector_rshrn_n_v8u8(uint16x8_t a, const int n);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshrn_n_u16(uint16x8_t a, const int n) {
  return __builtin_mpl_vector_rshrn_n_v8u8(a, n);
}

uint16x4_t __builtin_mpl_vector_rshrn_n_v4u16(uint32x4_t a, const int n);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshrn_n_u32(uint32x4_t a, const int n) {
  return __builtin_mpl_vector_rshrn_n_v4u16(a, n);
}

uint32x2_t __builtin_mpl_vector_rshrn_n_v2u32(uint64x2_t a, const int n);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshrn_n_u64(uint64x2_t a, const int n) {
  return __builtin_mpl_vector_rshrn_n_v2u32(a, n);
}

int8x16_t __builtin_mpl_vector_rshrn_high_n_v16i8(int8x8_t r, int16x8_t a, const int n);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshrn_high_n_s16(int8x8_t r,
                                                                                              int16x8_t a,
                                                                                              const int n) {
  return __builtin_mpl_vector_rshrn_high_n_v16i8(r, a, n);
}

int16x8_t __builtin_mpl_vector_rshrn_high_n_v8i16(int16x4_t r, int32x4_t a, const int n);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshrn_high_n_s32(int16x4_t r,
                                                                                              int32x4_t a,
                                                                                              const int n) {
  return __builtin_mpl_vector_rshrn_high_n_v8i16(r, a, n);
}

int32x4_t __builtin_mpl_vector_rshrn_high_n_v4i32(int32x2_t r, int64x2_t a, const int n);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshrn_high_n_s64(int32x2_t r, int64x2_t a,
                                                                                              const int n) {
  return __builtin_mpl_vector_rshrn_high_n_v4i32(r, a, n);
}

uint8x16_t __builtin_mpl_vector_rshrn_high_n_v16u8(uint8x8_t r, uint16x8_t a, const int n);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshrn_high_n_u16(uint8x8_t r,
                                                                                               uint16x8_t a,
                                                                                               const int n) {
  return __builtin_mpl_vector_rshrn_high_n_v16u8(r, a, n);
}

uint16x8_t __builtin_mpl_vector_rshrn_high_n_v8u16(uint16x4_t r, uint32x4_t a, const int n);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshrn_high_n_u32(uint16x4_t r,
                                                                                               uint32x4_t a,
                                                                                               const int n) {
  return __builtin_mpl_vector_rshrn_high_n_v8u16(r, a, n);
}

uint32x4_t __builtin_mpl_vector_rshrn_high_n_v4u32(uint32x2_t r, uint64x2_t a, const int n);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vrshrn_high_n_u64(uint32x2_t r,
                                                                                               uint64x2_t a,
                                                                                               const int n) {
  return __builtin_mpl_vector_rshrn_high_n_v4u32(r, a, n);
}

int8x8_t __builtin_mpl_vector_sri_n_v8i8(int8x8_t a, int8x8_t b, const int n);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsri_n_s8(int8x8_t a, int8x8_t b,
                                                                                     const int n) {
  return __builtin_mpl_vector_sri_n_v8i8(a, b, n);
}

int8x16_t __builtin_mpl_vector_sriq_n_v16i8(int8x16_t a, int8x16_t b, const int n);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vsriq_n_s8(int8x16_t a, int8x16_t b,
                                                                                       const int n) {
  return __builtin_mpl_vector_sriq_n_v16i8(a, b, n);
}

int16x4_t __builtin_mpl_vector_sri_n_v4i16(int16x4_t a, int16x4_t b, const int n);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsri_n_s16(int16x4_t a, int16x4_t b,
                                                                                       const int n) {
  return __builtin_mpl_vector_sri_n_v4i16(a, b, n);
}

int16x8_t __builtin_mpl_vector_sriq_n_v8i16(int16x8_t a, int16x8_t b, const int n);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsriq_n_s16(int16x8_t a, int16x8_t b,
                                                                                        const int n) {
  return __builtin_mpl_vector_sriq_n_v8i16(a, b, n);
}

int32x2_t __builtin_mpl_vector_sri_n_v2i32(int32x2_t a, int32x2_t b, const int n);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsri_n_s32(int32x2_t a, int32x2_t b,
                                                                                       const int n) {
  return __builtin_mpl_vector_sri_n_v2i32(a, b, n);
}

int32x4_t __builtin_mpl_vector_sriq_n_v4i32(int32x4_t a, int32x4_t b, const int n);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsriq_n_s32(int32x4_t a, int32x4_t b,
                                                                                        const int n) {
  return __builtin_mpl_vector_sriq_n_v4i32(a, b, n);
}

int64x2_t __builtin_mpl_vector_sriq_n_v2i64(int64x2_t a, int64x2_t b, const int n);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsriq_n_s64(int64x2_t a, int64x2_t b,
                                                                                        const int n) {
  return __builtin_mpl_vector_sriq_n_v2i64(a, b, n);
}

uint8x8_t __builtin_mpl_vector_sri_n_v8u8(uint8x8_t a, uint8x8_t b, const int n);
extern inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsri_n_u8(uint8x8_t a, uint8x8_t b,
                                                                                      const int n) {
  return __builtin_mpl_vector_sri_n_v8u8(a, b, n);
}

uint8x16_t __builtin_mpl_vector_sriq_n_v16u8(uint8x16_t a, uint8x16_t b, const int n);
extern inline uint8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vsriq_n_u8(uint8x16_t a, uint8x16_t b,
                                                                                        const int n) {
  return __builtin_mpl_vector_sriq_n_v16u8(a, b, n);
}

uint16x4_t __builtin_mpl_vector_sri_n_v4u16(uint16x4_t a, uint16x4_t b, const int n);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsri_n_u16(uint16x4_t a, uint16x4_t b,
                                                                                        const int n) {
  return __builtin_mpl_vector_sri_n_v4u16(a, b, n);
}

uint16x8_t __builtin_mpl_vector_sriq_n_v8u16(uint16x8_t a, uint16x8_t b, const int n);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vsriq_n_u16(uint16x8_t a, uint16x8_t b,
                                                                                         const int n) {
  return __builtin_mpl_vector_sriq_n_v8u16(a, b, n);
}

uint32x2_t __builtin_mpl_vector_sri_n_v2u32(uint32x2_t a, uint32x2_t b, const int n);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsri_n_u32(uint32x2_t a, uint32x2_t b,
                                                                                        const int n) {
  return __builtin_mpl_vector_sri_n_v2u32(a, b, n);
}

uint32x4_t __builtin_mpl_vector_sriq_n_v4u32(uint32x4_t a, uint32x4_t b, const int n);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vsriq_n_u32(uint32x4_t a, uint32x4_t b,
                                                                                         const int n) {
  return __builtin_mpl_vector_sriq_n_v4u32(a, b, n);
}

uint64x2_t __builtin_mpl_vector_sriq_n_v2u64(uint64x2_t a, uint64x2_t b, const int n);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vsriq_n_u64(uint64x2_t a, uint64x2_t b,
                                                                                         const int n) {
  return __builtin_mpl_vector_sriq_n_v2u64(a, b, n);
}

int64x1_t __builtin_mpl_vector_srid_n_i64(int64x1_t a, int64x1_t b, const int n);
extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vsrid_n_s64(int64_t a, int64_t b,
                                                                                      const int n) {
  return vget_lane_s64(__builtin_mpl_vector_srid_n_i64((int64x1_t){a}, (int64x1_t){b}, n), 0);
}

uint64x1_t __builtin_mpl_vector_srid_n_u64(uint64x1_t a, uint64x1_t b, const int n);
extern inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vsrid_n_u64(uint64_t a, uint64_t b,
                                                                                       const int n) {
  return vget_lane_u64(__builtin_mpl_vector_srid_n_u64((uint64x1_t){a}, (uint64x1_t){b}, n), 0);
}

int16x4_t __builtin_mpl_vector_mla_lane_v4i16(int16x4_t a, int16x4_t b, int16x4_t v, const int lane);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmla_lane_s16(int16x4_t a, int16x4_t b,
                                                                                          int16x4_t v, const int lane) {
  return __builtin_mpl_vector_mla_lane_v4i16(a, b, v, lane);
}

int16x8_t __builtin_mpl_vector_mlaq_lane_v8i16(int16x8_t a, int16x8_t b, int16x4_t v, const int lane);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlaq_lane_s16(int16x8_t a, int16x8_t b,
                                                                                           int16x4_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_mlaq_lane_v8i16(a, b, v, lane);
}

int32x2_t __builtin_mpl_vector_mla_lane_v2i32(int32x2_t a, int32x2_t b, int32x2_t v, const int lane);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmla_lane_s32(int32x2_t a, int32x2_t b,
                                                                                          int32x2_t v, const int lane) {
  return __builtin_mpl_vector_mla_lane_v2i32(a, b, v, lane);
}

int32x4_t __builtin_mpl_vector_mlaq_lane_v4i32(int32x4_t a, int32x4_t b, int32x2_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlaq_lane_s32(int32x4_t a, int32x4_t b,
                                                                                           int32x2_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_mlaq_lane_v4i32(a, b, v, lane);
}

uint16x4_t __builtin_mpl_vector_mla_lane_v4u16(uint16x4_t a, uint16x4_t b, uint16x4_t v, const int lane);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmla_lane_u16(uint16x4_t a, uint16x4_t b,
                                                                                           uint16x4_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_mla_lane_v4u16(a, b, v, lane);
}

uint16x8_t __builtin_mpl_vector_mlaq_lane_v8u16(uint16x8_t a, uint16x8_t b, uint16x4_t v, const int lane);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlaq_lane_u16(uint16x8_t a, uint16x8_t b,
                                                                                            uint16x4_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mlaq_lane_v8u16(a, b, v, lane);
}

uint32x2_t __builtin_mpl_vector_mla_lane_v2u32(uint32x2_t a, uint32x2_t b, uint32x2_t v, const int lane);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmla_lane_u32(uint32x2_t a, uint32x2_t b,
                                                                                           uint32x2_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_mla_lane_v2u32(a, b, v, lane);
}

uint32x4_t __builtin_mpl_vector_mlaq_lane_v4u32(uint32x4_t a, uint32x4_t b, uint32x2_t v, const int lane);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlaq_lane_u32(uint32x4_t a, uint32x4_t b,
                                                                                            uint32x2_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mlaq_lane_v4u32(a, b, v, lane);
}

int16x4_t __builtin_mpl_vector_mla_laneq_v4i16(int16x4_t a, int16x4_t b, int16x8_t v, const int lane);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmla_laneq_s16(int16x4_t a, int16x4_t b,
                                                                                           int16x8_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_mla_laneq_v4i16(a, b, v, lane);
}

int16x8_t __builtin_mpl_vector_mlaq_laneq_v8i16(int16x8_t a, int16x8_t b, int16x8_t v, const int lane);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlaq_laneq_s16(int16x8_t a, int16x8_t b,
                                                                                            int16x8_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mlaq_laneq_v8i16(a, b, v, lane);
}

int32x2_t __builtin_mpl_vector_mla_laneq_v2i32(int32x2_t a, int32x2_t b, int32x4_t v, const int lane);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmla_laneq_s32(int32x2_t a, int32x2_t b,
                                                                                           int32x4_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_mla_laneq_v2i32(a, b, v, lane);
}

int32x4_t __builtin_mpl_vector_mlaq_laneq_v4i32(int32x4_t a, int32x4_t b, int32x4_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlaq_laneq_s32(int32x4_t a, int32x4_t b,
                                                                                            int32x4_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mlaq_laneq_v4i32(a, b, v, lane);
}

uint16x4_t __builtin_mpl_vector_mla_laneq_v4u16(uint16x4_t a, uint16x4_t b, uint16x8_t v, const int lane);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmla_laneq_u16(uint16x4_t a, uint16x4_t b,
                                                                                            uint16x8_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mla_laneq_v4u16(a, b, v, lane);
}

uint16x8_t __builtin_mpl_vector_mlaq_laneq_v8u16(uint16x8_t a, uint16x8_t b, uint16x8_t v, const int lane);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlaq_laneq_u16(uint16x8_t a, uint16x8_t b,
                                                                                             uint16x8_t v,
                                                                                             const int lane) {
  return __builtin_mpl_vector_mlaq_laneq_v8u16(a, b, v, lane);
}

uint32x2_t __builtin_mpl_vector_mla_laneq_v2u32(uint32x2_t a, uint32x2_t b, uint32x4_t v, const int lane);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmla_laneq_u32(uint32x2_t a, uint32x2_t b,
                                                                                            uint32x4_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mla_laneq_v2u32(a, b, v, lane);
}

uint32x4_t __builtin_mpl_vector_mlaq_laneq_v4u32(uint32x4_t a, uint32x4_t b, uint32x4_t v, const int lane);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlaq_laneq_u32(uint32x4_t a, uint32x4_t b,
                                                                                             uint32x4_t v,
                                                                                             const int lane) {
  return __builtin_mpl_vector_mlaq_laneq_v4u32(a, b, v, lane);
}

int32x4_t __builtin_mpl_vector_mlal_lane_v4i32(int32x4_t a, int16x4_t b, int16x4_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_lane_s16(int32x4_t a, int16x4_t b,
                                                                                           int16x4_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_mlal_lane_v4i32(a, b, v, lane);
}

int64x2_t __builtin_mpl_vector_mlal_lane_v2i64(int64x2_t a, int32x2_t b, int32x2_t v, const int lane);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_lane_s32(int64x2_t a, int32x2_t b,
                                                                                           int32x2_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_mlal_lane_v2i64(a, b, v, lane);
}

uint32x4_t __builtin_mpl_vector_mlal_lane_v4u32(uint32x4_t a, uint16x4_t b, uint16x4_t v, const int lane);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_lane_u16(uint32x4_t a, uint16x4_t b,
                                                                                            uint16x4_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mlal_lane_v4u32(a, b, v, lane);
}

uint64x2_t __builtin_mpl_vector_mlal_lane_v2u64(uint64x2_t a, uint32x2_t b, uint32x2_t v, const int lane);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_lane_u32(uint64x2_t a, uint32x2_t b,
                                                                                            uint32x2_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mlal_lane_v2u64(a, b, v, lane);
}

int32x4_t __builtin_mpl_vector_mlal_high_lane_v4i32(int32x4_t a, int16x8_t b, int16x4_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_high_lane_s16(int32x4_t a,
                                                                                                int16x8_t b,
                                                                                                int16x4_t v,
                                                                                                const int lane) {
  return __builtin_mpl_vector_mlal_high_lane_v4i32(a, b, v, lane);
}

int64x2_t __builtin_mpl_vector_mlal_high_lane_v2i64(int64x2_t a, int32x4_t b, int32x2_t v, const int lane);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_high_lane_s32(int64x2_t a,
                                                                                                int32x4_t b,
                                                                                                int32x2_t v,
                                                                                                const int lane) {
  return __builtin_mpl_vector_mlal_high_lane_v2i64(a, b, v, lane);
}

uint32x4_t __builtin_mpl_vector_mlal_high_lane_v4u32(uint32x4_t a, uint16x8_t b, uint16x4_t v, const int lane);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_high_lane_u16(uint32x4_t a,
                                                                                                 uint16x8_t b,
                                                                                                 uint16x4_t v,
                                                                                                 const int lane) {
  return __builtin_mpl_vector_mlal_high_lane_v4u32(a, b, v, lane);
}

uint64x2_t __builtin_mpl_vector_mlal_high_lane_v2u64(uint64x2_t a, uint32x4_t b, uint32x2_t v, const int lane);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_high_lane_u32(uint64x2_t a,
                                                                                                 uint32x4_t b,
                                                                                                 uint32x2_t v,
                                                                                                 const int lane) {
  return __builtin_mpl_vector_mlal_high_lane_v2u64(a, b, v, lane);
}

int32x4_t __builtin_mpl_vector_mlal_laneq_v4i32(int32x4_t a, int16x4_t b, int16x8_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_laneq_s16(int32x4_t a, int16x4_t b,
                                                                                            int16x8_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mlal_laneq_v4i32(a, b, v, lane);
}

int64x2_t __builtin_mpl_vector_mlal_laneq_v2i64(int64x2_t a, int32x2_t b, int32x4_t v, const int lane);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_laneq_s32(int64x2_t a, int32x2_t b,
                                                                                            int32x4_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mlal_laneq_v2i64(a, b, v, lane);
}

uint32x4_t __builtin_mpl_vector_mlal_laneq_v4u32(uint32x4_t a, uint16x4_t b, uint16x8_t v, const int lane);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_laneq_u16(uint32x4_t a, uint16x4_t b,
                                                                                             uint16x8_t v,
                                                                                             const int lane) {
  return __builtin_mpl_vector_mlal_laneq_v4u32(a, b, v, lane);
}

uint64x2_t __builtin_mpl_vector_mlal_laneq_v2u64(uint64x2_t a, uint32x2_t b, uint32x4_t v, const int lane);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_laneq_u32(uint64x2_t a, uint32x2_t b,
                                                                                             uint32x4_t v,
                                                                                             const int lane) {
  return __builtin_mpl_vector_mlal_laneq_v2u64(a, b, v, lane);
}

int32x4_t __builtin_mpl_vector_mlal_high_laneq_v4i32(int32x4_t a, int16x8_t b, int16x8_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_high_laneq_s16(int32x4_t a,
                                                                                                 int16x8_t b,
                                                                                                 int16x8_t v,
                                                                                                 const int lane) {
  return __builtin_mpl_vector_mlal_high_laneq_v4i32(a, b, v, lane);
}

int64x2_t __builtin_mpl_vector_mlal_high_laneq_v2i64(int64x2_t a, int32x4_t b, int32x4_t v, const int lane);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_high_laneq_s32(int64x2_t a,
                                                                                                 int32x4_t b,
                                                                                                 int32x4_t v,
                                                                                                 const int lane) {
  return __builtin_mpl_vector_mlal_high_laneq_v2i64(a, b, v, lane);
}

uint32x4_t __builtin_mpl_vector_mlal_high_laneq_v4u32(uint32x4_t a, uint16x8_t b, uint16x8_t v, const int lane);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_high_laneq_u16(uint32x4_t a,
                                                                                                  uint16x8_t b,
                                                                                                  uint16x8_t v,
                                                                                                  const int lane) {
  return __builtin_mpl_vector_mlal_high_laneq_v4u32(a, b, v, lane);
}

uint64x2_t __builtin_mpl_vector_mlal_high_laneq_v2u64(uint64x2_t a, uint32x4_t b, uint32x4_t v, const int lane);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_high_laneq_u32(uint64x2_t a,
                                                                                                  uint32x4_t b,
                                                                                                  uint32x4_t v,
                                                                                                  const int lane) {
  return __builtin_mpl_vector_mlal_high_laneq_v2u64(a, b, v, lane);
}

int16x4_t __builtin_mpl_vector_mla_n_v4i16(int16x4_t a, int16x4_t b, int16x4_t c);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmla_n_s16(int16x4_t a, int16x4_t b,
                                                                                       int16_t c) {
  return __builtin_mpl_vector_mla_n_v4i16(a, b, (int16x4_t){c});
}

int16x8_t __builtin_mpl_vector_mlaq_n_v8i16(int16x8_t a, int16x8_t b, int16x8_t c);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlaq_n_s16(int16x8_t a, int16x8_t b,
                                                                                        int16_t c) {
  return __builtin_mpl_vector_mlaq_n_v8i16(a, b, (int16x8_t){c});
}

int32x2_t __builtin_mpl_vector_mla_n_v2i32(int32x2_t a, int32x2_t b, int32x2_t c);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmla_n_s32(int32x2_t a, int32x2_t b,
                                                                                       int32_t c) {
  return __builtin_mpl_vector_mla_n_v2i32(a, b, (int32x2_t){c});
}

int32x4_t __builtin_mpl_vector_mlaq_n_v4i32(int32x4_t a, int32x4_t b, int32x4_t c);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlaq_n_s32(int32x4_t a, int32x4_t b,
                                                                                        int32_t c) {
  return __builtin_mpl_vector_mlaq_n_v4i32(a, b, (int32x4_t){c});
}

uint16x4_t __builtin_mpl_vector_mla_n_v4u16(uint16x4_t a, uint16x4_t b, uint16x4_t c);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmla_n_u16(uint16x4_t a, uint16x4_t b,
                                                                                        uint16_t c) {
  return __builtin_mpl_vector_mla_n_v4u16(a, b, (uint16x4_t){c});
}

uint16x8_t __builtin_mpl_vector_mlaq_n_v8u16(uint16x8_t a, uint16x8_t b, uint16x8_t c);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlaq_n_u16(uint16x8_t a, uint16x8_t b,
                                                                                         uint16_t c) {
  return __builtin_mpl_vector_mlaq_n_v8u16(a, b, (uint16x8_t){c});
}

uint32x2_t __builtin_mpl_vector_mla_n_v2u32(uint32x2_t a, uint32x2_t b, uint32x2_t c);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmla_n_u32(uint32x2_t a, uint32x2_t b,
                                                                                        uint32_t c) {
  return __builtin_mpl_vector_mla_n_v2u32(a, b, (uint32x2_t){c});
}

uint32x4_t __builtin_mpl_vector_mlaq_n_v4u32(uint32x4_t a, uint32x4_t b, uint32x4_t c);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlaq_n_u32(uint32x4_t a, uint32x4_t b,
                                                                                         uint32_t c) {
  return __builtin_mpl_vector_mlaq_n_v4u32(a, b, (uint32x4_t){c});
}

int16x4_t __builtin_mpl_vector_mls_lane_v4i16(int16x4_t a, int16x4_t b, int16x4_t v, const int lane);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmls_lane_s16(int16x4_t a, int16x4_t b,
                                                                                          int16x4_t v, const int lane) {
  return __builtin_mpl_vector_mls_lane_v4i16(a, b, v, lane);
}

int16x8_t __builtin_mpl_vector_mlsq_lane_v8i16(int16x8_t a, int16x8_t b, int16x4_t v, const int lane);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsq_lane_s16(int16x8_t a, int16x8_t b,
                                                                                           int16x4_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_mlsq_lane_v8i16(a, b, v, lane);
}

int32x2_t __builtin_mpl_vector_mls_lane_v2i32(int32x2_t a, int32x2_t b, int32x2_t v, const int lane);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmls_lane_s32(int32x2_t a, int32x2_t b,
                                                                                          int32x2_t v, const int lane) {
  return __builtin_mpl_vector_mls_lane_v2i32(a, b, v, lane);
}

int32x4_t __builtin_mpl_vector_mlsq_lane_v4i32(int32x4_t a, int32x4_t b, int32x2_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsq_lane_s32(int32x4_t a, int32x4_t b,
                                                                                           int32x2_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_mlsq_lane_v4i32(a, b, v, lane);
}

uint16x4_t __builtin_mpl_vector_mls_lane_v4u16(uint16x4_t a, uint16x4_t b, uint16x4_t v, const int lane);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmls_lane_u16(uint16x4_t a, uint16x4_t b,
                                                                                           uint16x4_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_mls_lane_v4u16(a, b, v, lane);
}

uint16x8_t __builtin_mpl_vector_mlsq_lane_v8u16(uint16x8_t a, uint16x8_t b, uint16x4_t v, const int lane);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsq_lane_u16(uint16x8_t a, uint16x8_t b,
                                                                                            uint16x4_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mlsq_lane_v8u16(a, b, v, lane);
}

uint32x2_t __builtin_mpl_vector_mls_lane_v2u32(uint32x2_t a, uint32x2_t b, uint32x2_t v, const int lane);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmls_lane_u32(uint32x2_t a, uint32x2_t b,
                                                                                           uint32x2_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_mls_lane_v2u32(a, b, v, lane);
}

uint32x4_t __builtin_mpl_vector_mlsq_lane_v4u32(uint32x4_t a, uint32x4_t b, uint32x2_t v, const int lane);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsq_lane_u32(uint32x4_t a, uint32x4_t b,
                                                                                            uint32x2_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mlsq_lane_v4u32(a, b, v, lane);
}

int16x4_t __builtin_mpl_vector_mls_laneq_v4i16(int16x4_t a, int16x4_t b, int16x8_t v, const int lane);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmls_laneq_s16(int16x4_t a, int16x4_t b,
                                                                                           int16x8_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_mls_laneq_v4i16(a, b, v, lane);
}

int16x8_t __builtin_mpl_vector_mlsq_laneq_v8i16(int16x8_t a, int16x8_t b, int16x8_t v, const int lane);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsq_laneq_s16(int16x8_t a, int16x8_t b,
                                                                                            int16x8_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mlsq_laneq_v8i16(a, b, v, lane);
}

int32x2_t __builtin_mpl_vector_mls_laneq_v2i32(int32x2_t a, int32x2_t b, int32x4_t v, const int lane);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmls_laneq_s32(int32x2_t a, int32x2_t b,
                                                                                           int32x4_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_mls_laneq_v2i32(a, b, v, lane);
}

int32x4_t __builtin_mpl_vector_mlsq_laneq_v4i32(int32x4_t a, int32x4_t b, int32x4_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsq_laneq_s32(int32x4_t a, int32x4_t b,
                                                                                            int32x4_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mlsq_laneq_v4i32(a, b, v, lane);
}

uint16x4_t __builtin_mpl_vector_mls_laneq_v4u16(uint16x4_t a, uint16x4_t b, uint16x8_t v, const int lane);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmls_laneq_u16(uint16x4_t a, uint16x4_t b,
                                                                                            uint16x8_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mls_laneq_v4u16(a, b, v, lane);
}

uint16x8_t __builtin_mpl_vector_mlsq_laneq_v8u16(uint16x8_t a, uint16x8_t b, uint16x8_t v, const int lane);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsq_laneq_u16(uint16x8_t a, uint16x8_t b,
                                                                                             uint16x8_t v,
                                                                                             const int lane) {
  return __builtin_mpl_vector_mlsq_laneq_v8u16(a, b, v, lane);
}

uint32x2_t __builtin_mpl_vector_mls_laneq_v2u32(uint32x2_t a, uint32x2_t b, uint32x4_t v, const int lane);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmls_laneq_u32(uint32x2_t a, uint32x2_t b,
                                                                                            uint32x4_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mls_laneq_v2u32(a, b, v, lane);
}

uint32x4_t __builtin_mpl_vector_mlsq_laneq_v4u32(uint32x4_t a, uint32x4_t b, uint32x4_t v, const int lane);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsq_laneq_u32(uint32x4_t a, uint32x4_t b,
                                                                                             uint32x4_t v,
                                                                                             const int lane) {
  return __builtin_mpl_vector_mlsq_laneq_v4u32(a, b, v, lane);
}

int32x4_t __builtin_mpl_vector_mlsl_lane_v4i32(int32x4_t a, int16x4_t b, int16x4_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_lane_s16(int32x4_t a, int16x4_t b,
                                                                                           int16x4_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_mlsl_lane_v4i32(a, b, v, lane);
}

int64x2_t __builtin_mpl_vector_mlsl_lane_v2i64(int64x2_t a, int32x2_t b, int32x2_t v, const int lane);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_lane_s32(int64x2_t a, int32x2_t b,
                                                                                           int32x2_t v,
                                                                                           const int lane) {
  return __builtin_mpl_vector_mlsl_lane_v2i64(a, b, v, lane);
}

uint32x4_t __builtin_mpl_vector_mlsl_lane_v4u32(uint32x4_t a, uint16x4_t b, uint16x4_t v, const int lane);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_lane_u16(uint32x4_t a, uint16x4_t b,
                                                                                            uint16x4_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mlsl_lane_v4u32(a, b, v, lane);
}

uint64x2_t __builtin_mpl_vector_mlsl_lane_v2u64(uint64x2_t a, uint32x2_t b, uint32x2_t v, const int lane);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_lane_u32(uint64x2_t a, uint32x2_t b,
                                                                                            uint32x2_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mlsl_lane_v2u64(a, b, v, lane);
}

int32x4_t __builtin_mpl_vector_mlsl_high_lane_v4i32(int32x4_t a, int16x8_t b, int16x4_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_high_lane_s16(int32x4_t a,
                                                                                                int16x8_t b,
                                                                                                int16x4_t v,
                                                                                                const int lane) {
  return __builtin_mpl_vector_mlsl_high_lane_v4i32(a, b, v, lane);
}

int64x2_t __builtin_mpl_vector_mlsl_high_lane_v2i64(int64x2_t a, int32x4_t b, int32x2_t v, const int lane);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_high_lane_s32(int64x2_t a,
                                                                                                int32x4_t b,
                                                                                                int32x2_t v,
                                                                                                const int lane) {
  return __builtin_mpl_vector_mlsl_high_lane_v2i64(a, b, v, lane);
}

uint32x4_t __builtin_mpl_vector_mlsl_high_lane_v4u32(uint32x4_t a, uint16x8_t b, uint16x4_t v, const int lane);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_high_lane_u16(uint32x4_t a,
                                                                                                 uint16x8_t b,
                                                                                                 uint16x4_t v,
                                                                                                 const int lane) {
  return __builtin_mpl_vector_mlsl_high_lane_v4u32(a, b, v, lane);
}

uint64x2_t __builtin_mpl_vector_mlsl_high_lane_v2u64(uint64x2_t a, uint32x4_t b, uint32x2_t v, const int lane);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_high_lane_u32(uint64x2_t a,
                                                                                                 uint32x4_t b,
                                                                                                 uint32x2_t v,
                                                                                                 const int lane) {
  return __builtin_mpl_vector_mlsl_high_lane_v2u64(a, b, v, lane);
}

int32x4_t __builtin_mpl_vector_mlsl_laneq_v4i32(int32x4_t a, int16x4_t b, int16x8_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_laneq_s16(int32x4_t a, int16x4_t b,
                                                                                            int16x8_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mlsl_laneq_v4i32(a, b, v, lane);
}

int64x2_t __builtin_mpl_vector_mlsl_laneq_v2i64(int64x2_t a, int32x2_t b, int32x4_t v, const int lane);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_laneq_s32(int64x2_t a, int32x2_t b,
                                                                                            int32x4_t v,
                                                                                            const int lane) {
  return __builtin_mpl_vector_mlsl_laneq_v2i64(a, b, v, lane);
}

uint32x4_t __builtin_mpl_vector_mlsl_laneq_v4u32(uint32x4_t a, uint16x4_t b, uint16x8_t v, const int lane);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_laneq_u16(uint32x4_t a, uint16x4_t b,
                                                                                             uint16x8_t v,
                                                                                             const int lane) {
  return __builtin_mpl_vector_mlsl_laneq_v4u32(a, b, v, lane);
}

uint64x2_t __builtin_mpl_vector_mlsl_laneq_v2u64(uint64x2_t a, uint32x2_t b, uint32x4_t v, const int lane);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_laneq_u32(uint64x2_t a, uint32x2_t b,
                                                                                             uint32x4_t v,
                                                                                             const int lane) {
  return __builtin_mpl_vector_mlsl_laneq_v2u64(a, b, v, lane);
}

int32x4_t __builtin_mpl_vector_mlsl_high_laneq_v4i32(int32x4_t a, int16x8_t b, int16x8_t v, const int lane);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_high_laneq_s16(int32x4_t a,
                                                                                                 int16x8_t b,
                                                                                                 int16x8_t v,
                                                                                                 const int lane) {
  return __builtin_mpl_vector_mlsl_high_laneq_v4i32(a, b, v, lane);
}

int64x2_t __builtin_mpl_vector_mlsl_high_laneq_v2i64(int64x2_t a, int32x4_t b, int32x4_t v, const int lane);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_high_laneq_s32(int64x2_t a,
                                                                                                 int32x4_t b,
                                                                                                 int32x4_t v,
                                                                                                 const int lane) {
  return __builtin_mpl_vector_mlsl_high_laneq_v2i64(a, b, v, lane);
}

uint32x4_t __builtin_mpl_vector_mlsl_high_laneq_v4u32(uint32x4_t a, uint16x8_t b, uint16x8_t v, const int lane);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_high_laneq_u16(uint32x4_t a,
                                                                                                  uint16x8_t b,
                                                                                                  uint16x8_t v,
                                                                                                  const int lane) {
  return __builtin_mpl_vector_mlsl_high_laneq_v4u32(a, b, v, lane);
}

uint64x2_t __builtin_mpl_vector_mlsl_high_laneq_v2u64(uint64x2_t a, uint32x4_t b, uint32x4_t v, const int lane);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_high_laneq_u32(uint64x2_t a,
                                                                                                  uint32x4_t b,
                                                                                                  uint32x4_t v,
                                                                                                  const int lane) {
  return __builtin_mpl_vector_mlsl_high_laneq_v2u64(a, b, v, lane);
}

int32x4_t __builtin_mpl_vector_mlal_n_v4i32(int32x4_t a, int16x4_t b, int16x4_t c);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_n_s16(int32x4_t a, int16x4_t b,
                                                                                        int16_t c) {
  return __builtin_mpl_vector_mlal_n_v4i32(a, b, (int16x4_t){c});
}

int64x2_t __builtin_mpl_vector_mlal_n_v2i64(int64x2_t a, int32x2_t b, int32x2_t c);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_n_s32(int64x2_t a, int32x2_t b,
                                                                                        int32_t c) {
  return __builtin_mpl_vector_mlal_n_v2i64(a, b, (int32x2_t){c});
}

uint32x4_t __builtin_mpl_vector_mlal_n_v4u32(uint32x4_t a, uint16x4_t b, uint16x4_t c);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_n_u16(uint32x4_t a, uint16x4_t b,
                                                                                         uint16_t c) {
  return __builtin_mpl_vector_mlal_n_v4u32(a, b, (uint16x4_t){c});
}

uint64x2_t __builtin_mpl_vector_mlal_n_v2u64(uint64x2_t a, uint32x2_t b, uint32x2_t c);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_n_u32(uint64x2_t a, uint32x2_t b,
                                                                                         uint32_t c) {
  return __builtin_mpl_vector_mlal_n_v2u64(a, b, (uint32x2_t){c});
}

int32x4_t __builtin_mpl_vector_mlal_high_n_v4i32(int32x4_t a, int16x8_t b, int16x8_t c);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_high_n_s16(int32x4_t a, int16x8_t b,
                                                                                             int16_t c) {
  return __builtin_mpl_vector_mlal_high_n_v4i32(a, b, (int16x8_t){c});
}

int64x2_t __builtin_mpl_vector_mlal_high_n_v2i64(int64x2_t a, int32x4_t b, int32x4_t c);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_high_n_s32(int64x2_t a, int32x4_t b,
                                                                                             int32_t c) {
  return __builtin_mpl_vector_mlal_high_n_v2i64(a, b, (int32x4_t){c});
}

uint32x4_t __builtin_mpl_vector_mlal_high_n_v4u32(uint32x4_t a, uint16x8_t b, uint16x8_t c);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_high_n_u16(uint32x4_t a,
                                                                                              uint16x8_t b,
                                                                                              uint16_t c) {
  return __builtin_mpl_vector_mlal_high_n_v4u32(a, b, (uint16x8_t){c});
}

uint64x2_t __builtin_mpl_vector_mlal_high_n_v2u64(uint64x2_t a, uint32x4_t b, uint32x4_t c);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlal_high_n_u32(uint64x2_t a,
                                                                                              uint32x4_t b,
                                                                                              uint32_t c) {
  return __builtin_mpl_vector_mlal_high_n_v2u64(a, b, (uint32x4_t){c});
}

int16x4_t __builtin_mpl_vector_mls_n_v4i16(int16x4_t a, int16x4_t b, int16x4_t c);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmls_n_s16(int16x4_t a, int16x4_t b,
                                                                                       int16_t c) {
  return __builtin_mpl_vector_mls_n_v4i16(a, b, (int16x4_t){c});
}

int16x8_t __builtin_mpl_vector_mlsq_n_v8i16(int16x8_t a, int16x8_t b, int16x8_t c);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsq_n_s16(int16x8_t a, int16x8_t b,
                                                                                        int16_t c) {
  return __builtin_mpl_vector_mlsq_n_v8i16(a, b, (int16x8_t){c});
}

int32x2_t __builtin_mpl_vector_mls_n_v2i32(int32x2_t a, int32x2_t b, int32x2_t c);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmls_n_s32(int32x2_t a, int32x2_t b,
                                                                                       int32_t c) {
  return __builtin_mpl_vector_mls_n_v2i32(a, b, (int32x2_t){c});
}

int32x4_t __builtin_mpl_vector_mlsq_n_v4i32(int32x4_t a, int32x4_t b, int32x4_t c);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsq_n_s32(int32x4_t a, int32x4_t b,
                                                                                        int32_t c) {
  return __builtin_mpl_vector_mlsq_n_v4i32(a, b, (int32x4_t){c});
}

uint16x4_t __builtin_mpl_vector_mls_n_v4u16(uint16x4_t a, uint16x4_t b, uint16x4_t c);
extern inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmls_n_u16(uint16x4_t a, uint16x4_t b,
                                                                                        uint16_t c) {
  return __builtin_mpl_vector_mls_n_v4u16(a, b, (uint16x4_t){c});
}

uint16x8_t __builtin_mpl_vector_mlsq_n_v8u16(uint16x8_t a, uint16x8_t b, uint16x8_t c);
extern inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsq_n_u16(uint16x8_t a, uint16x8_t b,
                                                                                         uint16_t c) {
  return __builtin_mpl_vector_mlsq_n_v8u16(a, b, (uint16x8_t){c});
}

uint32x2_t __builtin_mpl_vector_mls_n_v2u32(uint32x2_t a, uint32x2_t b, uint32x2_t c);
extern inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmls_n_u32(uint32x2_t a, uint32x2_t b,
                                                                                        uint32_t c) {
  return __builtin_mpl_vector_mls_n_v2u32(a, b, (uint32x2_t){c});
}

uint32x4_t __builtin_mpl_vector_mlsq_n_v4u32(uint32x4_t a, uint32x4_t b, uint32x4_t c);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsq_n_u32(uint32x4_t a, uint32x4_t b,
                                                                                         uint32_t c) {
  return __builtin_mpl_vector_mlsq_n_v4u32(a, b, (uint32x4_t){c});
}

int32x4_t __builtin_mpl_vector_mlsl_n_v4i32(int32x4_t a, int16x4_t b, int16x4_t c);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_n_s16(int32x4_t a, int16x4_t b,
                                                                                        int16_t c) {
  return __builtin_mpl_vector_mlsl_n_v4i32(a, b, (int16x4_t){c});
}

int64x2_t __builtin_mpl_vector_mlsl_n_v2i64(int64x2_t a, int32x2_t b, int32x2_t c);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_n_s32(int64x2_t a, int32x2_t b,
                                                                                        int32_t c) {
  return __builtin_mpl_vector_mlsl_n_v2i64(a, b, (int32x2_t){c});
}

uint32x4_t __builtin_mpl_vector_mlsl_n_v4u32(uint32x4_t a, uint16x4_t b, uint16x4_t c);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_n_u16(uint32x4_t a, uint16x4_t b,
                                                                                         uint16_t c) {
  return __builtin_mpl_vector_mlsl_n_v4u32(a, b, (uint16x4_t){c});
}

uint64x2_t __builtin_mpl_vector_mlsl_n_v2u64(uint64x2_t a, uint32x2_t b, uint32x2_t c);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_n_u32(uint64x2_t a, uint32x2_t b,
                                                                                         uint32_t c) {
  return __builtin_mpl_vector_mlsl_n_v2u64(a, b, (uint32x2_t){c});
}

int32x4_t __builtin_mpl_vector_mlsl_high_n_v4i32(int32x4_t a, int16x8_t b, int16x8_t c);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_high_n_s16(int32x4_t a, int16x8_t b,
                                                                                             int16_t c) {
  return __builtin_mpl_vector_mlsl_high_n_v4i32(a, b, (int16x8_t){c});
}

int64x2_t __builtin_mpl_vector_mlsl_high_n_v2i64(int64x2_t a, int32x4_t b, int32x4_t c);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_high_n_s32(int64x2_t a, int32x4_t b,
                                                                                             int32_t c) {
  return __builtin_mpl_vector_mlsl_high_n_v2i64(a, b, (int32x4_t){c});
}

uint32x4_t __builtin_mpl_vector_mlsl_high_n_v4u32(uint32x4_t a, uint16x8_t b, uint16x8_t c);
extern inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_high_n_u16(uint32x4_t a,
                                                                                              uint16x8_t b,
                                                                                              uint16_t c) {
  return __builtin_mpl_vector_mlsl_high_n_v4u32(a, b, (uint16x8_t){c});
}

uint64x2_t __builtin_mpl_vector_mlsl_high_n_v2u64(uint64x2_t a, uint32x4_t b, uint32x4_t c);
extern inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vmlsl_high_n_u32(uint64x2_t a,
                                                                                              uint32x4_t b,
                                                                                              uint32_t c) {
  return __builtin_mpl_vector_mlsl_high_n_v2u64(a, b, (uint32x4_t){c});
}

int8x8_t __builtin_mpl_vector_qneg_v8i8(int8x8_t a);
extern inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqneg_s8(int8x8_t a) {
  return __builtin_mpl_vector_qneg_v8i8(a);
}

int8x16_t __builtin_mpl_vector_qnegq_v16i8(int8x16_t a);
extern inline int8x16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqnegq_s8(int8x16_t a) {
  return __builtin_mpl_vector_qnegq_v16i8(a);
}

int16x4_t __builtin_mpl_vector_qneg_v4i16(int16x4_t a);
extern inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqneg_s16(int16x4_t a) {
  return __builtin_mpl_vector_qneg_v4i16(a);
}

int16x8_t __builtin_mpl_vector_qnegq_v8i16(int16x8_t a);
extern inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqnegq_s16(int16x8_t a) {
  return __builtin_mpl_vector_qnegq_v8i16(a);
}

int32x2_t __builtin_mpl_vector_qneg_v2i32(int32x2_t a);
extern inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqneg_s32(int32x2_t a) {
  return __builtin_mpl_vector_qneg_v2i32(a);
}

int32x4_t __builtin_mpl_vector_qnegq_v4i32(int32x4_t a);
extern inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__)) vqnegq_s32(int32x4_t a) {
  return __builtin_mpl_vector_qnegq_v4i32(a);
}

int64x1_t __builtin_mpl_vector_qneg_v1i64(int64x1_t a);
extern inline int64x1_t __attribute__ ((__always_inline__, __gnu_inline__)) vqneg_s64(int64x1_t a) {
  return __builtin_mpl_vector_qneg_v1i64(a);
}

int64x2_t __builtin_mpl_vector_qnegq_v2i64(int64x2_t a);
extern inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__)) vqnegq_s64(int64x2_t a) {
  return __builtin_mpl_vector_qnegq_v2i64(a);
}

extern inline int8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqnegb_s8(int8_t a) {
  return vget_lane_s8(vqneg_s8((int8x8_t){a}), 0);
}

extern inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqnegh_s16(int16_t a) {
  return vget_lane_s16(vqneg_s16((int16x4_t){a}), 0);
}

extern inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqnegs_s32(int32_t a) {
  return vget_lane_s32(vqneg_s32((int32x2_t){a}), 0);
}

extern inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vqnegd_s64(int64_t a) {
  return vget_lane_s64(vqneg_s64((int64x1_t){a}), 0);
}

// unsupport
#define vadd_f16(a, b) (a + b)
#define vaddq_f16(a, b) (a + b)
#define vdup_n_f16(a) __builtin_mpl_vector_from_scalar_v4f16(a)
#define vdupq_n_f16(a) __builtin_mpl_vector_from_scalar_v8f16(a)
#define vceq_f16(a, b) (a == b)
#define vceqq_f16(a, b) (a == b)
#define vcgt_f16(a, b) (a > b)
#define vcgtq_f16(a, b) (a > b)
#define vcge_f16(a, b) (a >= b)
#define vcgeq_f16(a, b) (a >= b)
#define vclt_f16(a, b) (a < b)
#define vcltq_f16(a, b) (a < b)
#define vcle_f16(a, b) (a <= b)
#define vcleq_f16(a, b) (a <= b)
#define vext_f16(a, b, n) __builtin_mpl_vector_merge_v4f16(a, b, n)
#define vextq_f16(a, b, n) __builtin_mpl_vector_merge_v8f16(a, b, n)
#define vget_high_f16(a) __builtin_mpl_vector_get_high_v4f16(a)
#define vget_lane_f16(a, n) __builtin_mpl_vector_get_element_v4f16(a, n)
#define vgetq_lane_f16(a, n) __builtin_mpl_vector_get_element_v8f16(a, n)
#define vget_low_f16(a) __builtin_mpl_vector_get_low_v4f16(a)
#define vld1_f16(a) __builtin_mpl_vector_load_v4f16(a)
#define vld1q_f16(a) __builtin_mpl_vector_load_v8f16(a)
#define vreinterpret_f16_s8(a) ((float16x4_t)a)
#define vreinterpret_f16_u8(a) ((float16x4_t)a)
#define vreinterpretq_f16_s8(a) ((float16x8_t)a)
#define vreinterpretq_f16_u8(a) ((float16x8_t)a)
#define vreinterpret_f16_s16(a) ((float16x4_t)a)
#define vreinterpret_f16_u16(a) ((float16x4_t)a)
#define vreinterpretq_f16_s16(a) ((float16x8_t)a)
#define vreinterpretq_f16_u16(a) ((float16x8_t)a)
#define vreinterpret_f16_s32(a) ((float16x4_t)a)
#define vreinterpret_f16_u32(a) ((float16x4_t)a)
#define vreinterpretq_f16_s32(a) ((float16x8_t)a)
#define vreinterpretq_f16_u32(a) ((float16x8_t)a)
#define vreinterpret_f16_s64(a) ((float16x4_t)a)
#define vreinterpret_f16_u64(a) ((float16x4_t)a)
#define vreinterpretq_f16_s64(a) ((float16x8_t)a)
#define vreinterpretq_f16_u64(a) ((float16x8_t)a)
#define vset_lane_f16(v, a, n) __builtin_mpl_vector_set_element_v4f16(v, a, n)
#define vsetq_lane_f16(v, a, n) __builtin_mpl_vector_set_element_v8f16(v, a, n)
#define vst1_f16(p, v) __builtin_mpl_vector_store_v4f16(p, v)
#define vst1q_f16(p, v) __builtin_mpl_vector_store_v8f16(p, v)
#define vsub_f16(a, b) (a - b)
#define vsubq_f16(a, b) (a - b)
#define vceqz_p8(a) (a == 0)
#define vceqzq_p8(a) (a == 0)
#define vceqz_p64(a) (a == 0)
#define vceqzq_p64(a) (a == 0)

void __builtin_mpl_vector_st2_i8v8(int8_t *ptr, int8x8x2_t val);
#define vst2_s8(ptr, val)  __builtin_mpl_vector_st2_i8v8(ptr, val)

void __builtin_mpl_vector_st2q_i8v16(int8_t *ptr, int8x16x2_t val);
#define vst2q_s8(ptr, val)  __builtin_mpl_vector_st2q_i8v16(ptr, val)

void __builtin_mpl_vector_st2_i16v4(int16_t *ptr, int16x4x2_t val);
#define vst2_s16(ptr, val)  __builtin_mpl_vector_st2_i16v4(ptr, val)

void __builtin_mpl_vector_st2q_i16v8(int16_t *ptr, int16x8x2_t val);
#define vst2q_s16(ptr, val)  __builtin_mpl_vector_st2q_i16v8(ptr, val)

void __builtin_mpl_vector_st2_i32v2(int32_t *ptr, int32x2x2_t val);
#define vst2_s32(ptr, val)  __builtin_mpl_vector_st2_i32v2(ptr, val)

void __builtin_mpl_vector_st2q_i32v4(int32_t *ptr, int32x4x2_t val);
#define vst2q_s32(ptr, val)  __builtin_mpl_vector_st2q_i32v4(ptr, val)

void __builtin_mpl_vector_st2_u8v8(uint8_t *ptr, uint8x8x2_t val);
#define vst2_u8(ptr, val)  __builtin_mpl_vector_st2_u8v8(ptr, val)

void __builtin_mpl_vector_st2q_u8v16(uint8_t *ptr, uint8x16x2_t val);
#define vst2q_u8(ptr, val)  __builtin_mpl_vector_st2q_u8v16(ptr, val)

void __builtin_mpl_vector_st2_u16v4(uint16_t *ptr, uint16x4x2_t val);
#define vst2_u16(ptr, val)  __builtin_mpl_vector_st2_u16v4(ptr, val)

void __builtin_mpl_vector_st2q_u16v8(uint16_t *ptr, uint16x8x2_t val);
#define vst2q_u16(ptr, val)  __builtin_mpl_vector_st2q_u16v8(ptr, val)

void __builtin_mpl_vector_st2_u32v2(uint32_t *ptr, uint32x2x2_t val);
#define vst2_u32(ptr, val)  __builtin_mpl_vector_st2_u32v2(ptr, val)

void __builtin_mpl_vector_st2q_u32v4(uint32_t *ptr, uint32x4x2_t val);
#define vst2q_u32(ptr, val)  __builtin_mpl_vector_st2q_u32v4(ptr, val)

void __builtin_mpl_vector_st2_i64v1(int64_t *ptr, int64x1x2_t val);
#define vst2_s64(ptr, val)  __builtin_mpl_vector_st2_i64v1(ptr, val)

void __builtin_mpl_vector_st2_u64v1(uint64_t *ptr, uint64x1x2_t val);
#define vst2_u64(ptr, val)  __builtin_mpl_vector_st2_u64v1(ptr, val)

void __builtin_mpl_vector_st2q_i64v2(int64_t *ptr, int64x2x2_t val);
#define vst2q_s64(ptr, val)  __builtin_mpl_vector_st2q_i64v2(ptr, val)

void __builtin_mpl_vector_st2q_u64v2(uint64_t *ptr, uint64x2x2_t val);
#define vst2q_u64(ptr, val)  __builtin_mpl_vector_st2q_u64v2(ptr, val)

void __builtin_mpl_vector_st3_i8v8(int8_t *ptr, int8x8x3_t val);
#define vst3_s8(ptr, val)  __builtin_mpl_vector_st3_i8v8(ptr, val)

void __builtin_mpl_vector_st3q_i8v16(int8_t *ptr, int8x16x3_t val);
#define vst3q_s8(ptr, val)  __builtin_mpl_vector_st3q_i8v16(ptr, val)

void __builtin_mpl_vector_st3_i16v4(int16_t *ptr, int16x4x3_t val);
#define vst3_s16(ptr, val)  __builtin_mpl_vector_st3_i16v4(ptr, val)

void __builtin_mpl_vector_st3q_i16v8(int16_t *ptr, int16x8x3_t val);
#define vst3q_s16(ptr, val)  __builtin_mpl_vector_st3q_i16v8(ptr, val)

void __builtin_mpl_vector_st3_i32v2(int32_t *ptr, int32x2x3_t val);
#define vst3_s32(ptr, val)  __builtin_mpl_vector_st3_i32v2(ptr, val)

void __builtin_mpl_vector_st3q_i32v4(int32_t *ptr, int32x4x3_t val);
#define vst3q_s32(ptr, val)  __builtin_mpl_vector_st3q_i32v4(ptr, val)

void __builtin_mpl_vector_st3_u8v8(uint8_t *ptr, uint8x8x3_t val);
#define vst3_u8(ptr, val)  __builtin_mpl_vector_st3_u8v8(ptr, val)

void __builtin_mpl_vector_st3q_u8v16(uint8_t *ptr, uint8x16x3_t val);
#define vst3q_u8(ptr, val)  __builtin_mpl_vector_st3q_u8v16(ptr, val)

void __builtin_mpl_vector_st3_u16v4(uint16_t *ptr, uint16x4x3_t val);
#define vst3_u16(ptr, val)  __builtin_mpl_vector_st3_u16v4(ptr, val)

void __builtin_mpl_vector_st3q_u16v8(uint16_t *ptr, uint16x8x3_t val);
#define vst3q_u16(ptr, val)  __builtin_mpl_vector_st3q_u16v8(ptr, val)

void __builtin_mpl_vector_st3_u32v2(uint32_t *ptr, uint32x2x3_t val);
#define vst3_u32(ptr, val)  __builtin_mpl_vector_st3_u32v2(ptr, val)

void __builtin_mpl_vector_st3q_u32v4(uint32_t *ptr, uint32x4x3_t val);
#define vst3q_u32(ptr, val)  __builtin_mpl_vector_st3q_u32v4(ptr, val)

void __builtin_mpl_vector_st3_i64v1(int64_t *ptr, int64x1x3_t val);
#define vst3_s64(ptr, val)  __builtin_mpl_vector_st3_i64v1(ptr, val)

void __builtin_mpl_vector_st3_u64v1(uint64_t *ptr, uint64x1x3_t val);
#define vst3_u64(ptr, val)  __builtin_mpl_vector_st3_u64v1(ptr, val)

void __builtin_mpl_vector_st3q_i64v2(int64_t *ptr, int64x2x3_t val);
#define vst3q_s64(ptr, val)  __builtin_mpl_vector_st3q_i64v2(ptr, val)

void __builtin_mpl_vector_st3q_u64v2(uint64_t *ptr, uint64x2x3_t val);
#define vst3q_u64(ptr, val)  __builtin_mpl_vector_st3q_u64v2(ptr, val)

void __builtin_mpl_vector_st4_i8v8(int8_t *ptr, int8x8x4_t val);
#define vst4_s8(ptr, val)  __builtin_mpl_vector_st4_i8v8(ptr, val)

void __builtin_mpl_vector_st4q_i8v16(int8_t *ptr, int8x16x4_t val);
#define vst4q_s8(ptr, val)  __builtin_mpl_vector_st4q_i8v16(ptr, val)

void __builtin_mpl_vector_st4_i16v4(int16_t *ptr, int16x4x4_t val);
#define vst4_s16(ptr, val)  __builtin_mpl_vector_st4_i16v4(ptr, val)

void __builtin_mpl_vector_st4q_i16v8(int16_t *ptr, int16x8x4_t val);
#define vst4q_s16(ptr, val)  __builtin_mpl_vector_st4q_i16v8(ptr, val)

void __builtin_mpl_vector_st4_i32v2(int32_t *ptr, int32x2x4_t val);
#define vst4_s32(ptr, val)  __builtin_mpl_vector_st4_i32v2(ptr, val)

void __builtin_mpl_vector_st4q_i32v4(int32_t *ptr, int32x4x4_t val);
#define vst4q_s32(ptr, val)  __builtin_mpl_vector_st4q_i32v4(ptr, val)

void __builtin_mpl_vector_st4_u8v8(uint8_t *ptr, uint8x8x4_t val);
#define vst4_u8(ptr, val)  __builtin_mpl_vector_st4_u8v8(ptr, val)

void __builtin_mpl_vector_st4q_u8v16(uint8_t *ptr, uint8x16x4_t val);
#define vst4q_u8(ptr, val)  __builtin_mpl_vector_st4q_u8v16(ptr, val)

void __builtin_mpl_vector_st4_u16v4(uint16_t *ptr, uint16x4x4_t val);
#define vst4_u16(ptr, val)  __builtin_mpl_vector_st4_u16v4(ptr, val)

void __builtin_mpl_vector_st4q_u16v8(uint16_t *ptr, uint16x8x4_t val);
#define vst4q_u16(ptr, val)  __builtin_mpl_vector_st4q_u16v8(ptr, val)

void __builtin_mpl_vector_st4_u32v2(uint32_t *ptr, uint32x2x4_t val);
#define vst4_u32(ptr, val)  __builtin_mpl_vector_st4_u32v2(ptr, val)

void __builtin_mpl_vector_st4q_u32v4(uint32_t *ptr, uint32x4x4_t val);
#define vst4q_u32(ptr, val)  __builtin_mpl_vector_st4q_u32v4(ptr, val)

void __builtin_mpl_vector_st4_i64v1(int64_t *ptr, int64x1x4_t val);
#define vst4_s64(ptr, val)  __builtin_mpl_vector_st4_i64v1(ptr, val)

void __builtin_mpl_vector_st4_u64v1(uint64_t *ptr, uint64x1x4_t val);
#define vst4_u64(ptr, val)  __builtin_mpl_vector_st4_u64v1(ptr, val)

void __builtin_mpl_vector_st4q_i64v2(int64_t *ptr, int64x2x4_t val);
#define vst4q_s64(ptr, val)  __builtin_mpl_vector_st4q_i64v2(ptr, val)

void __builtin_mpl_vector_st4q_u64v2(uint64_t *ptr, uint64x2x4_t val);
#define vst4q_u64(ptr, val)  __builtin_mpl_vector_st4q_u64v2(ptr, val)

void __builtin_mpl_vector_st2_lane_i8v8(int8_t *ptr, int8x8x2_t val, const int lane);
#define vst2_lane_s8(ptr, val, lane)  __builtin_mpl_vector_st2_lane_i8v8(ptr, val, lane)

void __builtin_mpl_vector_st2_lane_u8v8(uint8_t *ptr, uint8x8x2_t val, const int lane);
#define vst2_lane_u8(ptr, val, lane)  __builtin_mpl_vector_st2_lane_u8v8(ptr, val, lane)

void __builtin_mpl_vector_st3_lane_i8v8(int8_t *ptr, int8x8x3_t val, const int lane);
#define vst3_lane_s8(ptr, val, lane)  __builtin_mpl_vector_st3_lane_i8v8(ptr, val, lane)

void __builtin_mpl_vector_st3_lane_u8v8(uint8_t *ptr, uint8x8x3_t val, const int lane);
#define vst3_lane_u8(ptr, val, lane)  __builtin_mpl_vector_st3_lane_u8v8(ptr, val, lane)

void __builtin_mpl_vector_st4_lane_i8v8(int8_t *ptr, int8x8x4_t val, const int lane);
#define vst4_lane_s8(ptr, val, lane)  __builtin_mpl_vector_st4_lane_i8v8(ptr, val, lane)

void __builtin_mpl_vector_st4_lane_u8v8(uint8_t *ptr, uint8x8x4_t val, const int lane);
#define vst4_lane_u8(ptr, val, lane)  __builtin_mpl_vector_st4_lane_u8v8(ptr, val, lane)

void __builtin_mpl_vector_st2_lane_i16v4(int16_t *ptr, int16x4x2_t val, const int lane);
#define vst2_lane_s16(ptr, val, lane)  __builtin_mpl_vector_st2_lane_i16v4(ptr, val, lane)

void __builtin_mpl_vector_st2q_lane_i16v8(int16_t *ptr, int16x8x2_t val, const int lane);
#define vst2q_lane_s16(ptr, val, lane)  __builtin_mpl_vector_st2q_lane_i16v8(ptr, val, lane)

void __builtin_mpl_vector_st2_lane_i32v2(int32_t *ptr, int32x2x2_t val, const int lane);
#define vst2_lane_s32(ptr, val, lane)  __builtin_mpl_vector_st2_lane_i32v2(ptr, val, lane)

void __builtin_mpl_vector_st2q_lane_i32v4(int32_t *ptr, int32x4x2_t val, const int lane);
#define vst2q_lane_s32(ptr, val, lane)  __builtin_mpl_vector_st2q_lane_i32v4(ptr, val, lane)

void __builtin_mpl_vector_st2_lane_u16v4(uint16_t *ptr, uint16x4x2_t val, const int lane);
#define vst2_lane_u16(ptr, val, lane)  __builtin_mpl_vector_st2_lane_u16v4(ptr, val, lane)

void __builtin_mpl_vector_st2q_lane_u16v8(uint16_t *ptr, uint16x8x2_t val, const int lane);
#define vst2q_lane_u16(ptr, val, lane)  __builtin_mpl_vector_st2q_lane_u16v8(ptr, val, lane)

void __builtin_mpl_vector_st2_lane_u32v2(uint32_t *ptr, uint32x2x2_t val, const int lane);
#define vst2_lane_u32(ptr, val, lane)  __builtin_mpl_vector_st2_lane_u32v2(ptr, val, lane)

void __builtin_mpl_vector_st2q_lane_u32v4(uint32_t *ptr, uint32x4x2_t val, const int lane);
#define vst2q_lane_u32(ptr, val, lane)  __builtin_mpl_vector_st2q_lane_u32v4(ptr, val, lane)

void __builtin_mpl_vector_st2q_lane_i8v16(int8_t *ptr, int8x16x2_t val, const int lane);
#define vst2q_lane_s8(ptr, val, lane)  __builtin_mpl_vector_st2q_lane_i8v16(ptr, val, lane)

void __builtin_mpl_vector_st2q_lane_u8v16(uint8_t *ptr, uint8x16x2_t val, const int lane);
#define vst2q_lane_u8(ptr, val, lane)  __builtin_mpl_vector_st2q_lane_u8v16(ptr, val, lane)

void __builtin_mpl_vector_st2_lane_i64v1(int64_t *ptr, int64x1x2_t val, const int lane);
#define vst2_lane_s64(ptr, val, lane)  __builtin_mpl_vector_st2_lane_i64v1(ptr, val, lane)

void __builtin_mpl_vector_st2q_lane_i64v2(int64_t *ptr, int64x2x2_t val, const int lane);
#define vst2q_lane_s64(ptr, val, lane)  __builtin_mpl_vector_st2q_lane_i64v2(ptr, val, lane)

void __builtin_mpl_vector_st2_lane_u64v1(uint64_t *ptr, uint64x1x2_t val, const int lane);
#define vst2_lane_u64(ptr, val, lane)  __builtin_mpl_vector_st2_lane_u64v1(ptr, val, lane)

void __builtin_mpl_vector_st2q_lane_u64v2(uint64_t *ptr, uint64x2x2_t val, const int lane);
#define vst2q_lane_u64(ptr, val, lane)  __builtin_mpl_vector_st2q_lane_u64v2(ptr, val, lane)

void __builtin_mpl_vector_st3_lane_i16v4(int16_t *ptr, int16x4x3_t val, const int lane);
#define vst3_lane_s16(ptr, val, lane)  __builtin_mpl_vector_st3_lane_i16v4(ptr, val, lane)

void __builtin_mpl_vector_st3q_lane_i16v8(int16_t *ptr, int16x8x3_t val, const int lane);
#define vst3q_lane_s16(ptr, val, lane)  __builtin_mpl_vector_st3q_lane_i16v8(ptr, val, lane)

void __builtin_mpl_vector_st3_lane_i32v2(int32_t *ptr, int32x2x3_t val, const int lane);
#define vst3_lane_s32(ptr, val, lane)  __builtin_mpl_vector_st3_lane_i32v2(ptr, val, lane)

void __builtin_mpl_vector_st3q_lane_i32v4(int32_t *ptr, int32x4x3_t val, const int lane);
#define vst3q_lane_s32(ptr, val, lane)  __builtin_mpl_vector_st3q_lane_i32v4(ptr, val, lane)

void __builtin_mpl_vector_st3_lane_u16v4(uint16_t *ptr, uint16x4x3_t val, const int lane);
#define vst3_lane_u16(ptr, val, lane)  __builtin_mpl_vector_st3_lane_u16v4(ptr, val, lane)

void __builtin_mpl_vector_st3q_lane_u16v8(uint16_t *ptr, uint16x8x3_t val, const int lane);
#define vst3q_lane_u16(ptr, val, lane)  __builtin_mpl_vector_st3q_lane_u16v8(ptr, val, lane)

void __builtin_mpl_vector_st3_lane_u32v2(uint32_t *ptr, uint32x2x3_t val, const int lane);
#define vst3_lane_u32(ptr, val, lane)  __builtin_mpl_vector_st3_lane_u32v2(ptr, val, lane)

void __builtin_mpl_vector_st3q_lane_u32v4(uint32_t *ptr, uint32x4x3_t val, const int lane);
#define vst3q_lane_u32(ptr, val, lane)  __builtin_mpl_vector_st3q_lane_u32v4(ptr, val, lane)

void __builtin_mpl_vector_st3q_lane_i8v16(int8_t *ptr, int8x16x3_t val, const int lane);
#define vst3q_lane_s8(ptr, val, lane)  __builtin_mpl_vector_st3q_lane_i8v16(ptr, val, lane)

void __builtin_mpl_vector_st3q_lane_u8v16(uint8_t *ptr, uint8x16x3_t val, const int lane);
#define vst3q_lane_u8(ptr, val, lane)  __builtin_mpl_vector_st3q_lane_u8v16(ptr, val, lane)

void __builtin_mpl_vector_st3_lane_i64v1(int64_t *ptr, int64x1x3_t val, const int lane);
#define vst3_lane_s64(ptr, val, lane)  __builtin_mpl_vector_st3_lane_i64v1(ptr, val, lane)

void __builtin_mpl_vector_st3q_lane_i64v2(int64_t *ptr, int64x2x3_t val, const int lane);
#define vst3q_lane_s64(ptr, val, lane)  __builtin_mpl_vector_st3q_lane_i64v2(ptr, val, lane)

void __builtin_mpl_vector_st3_lane_u64v1(uint64_t *ptr, uint64x1x3_t val, const int lane);
#define vst3_lane_u64(ptr, val, lane)  __builtin_mpl_vector_st3_lane_u64v1(ptr, val, lane)

void __builtin_mpl_vector_st3q_lane_u64v2(uint64_t *ptr, uint64x2x3_t val, const int lane);
#define vst3q_lane_u64(ptr, val, lane)  __builtin_mpl_vector_st3q_lane_u64v2(ptr, val, lane)

void __builtin_mpl_vector_st4_lane_i16v4(int16_t *ptr, int16x4x4_t val, const int lane);
#define vst4_lane_s16(ptr, val, lane)  __builtin_mpl_vector_st4_lane_i16v4(ptr, val, lane)

void __builtin_mpl_vector_st4q_lane_i16v8(int16_t *ptr, int16x8x4_t val, const int lane);
#define vst4q_lane_s16(ptr, val, lane)  __builtin_mpl_vector_st4q_lane_i16v8(ptr, val, lane)

void __builtin_mpl_vector_st4_lane_i32v2(int32_t *ptr, int32x2x4_t val, const int lane);
#define vst4_lane_s32(ptr, val, lane)  __builtin_mpl_vector_st4_lane_i32v2(ptr, val, lane)

void __builtin_mpl_vector_st4q_lane_i32v4(int32_t *ptr, int32x4x4_t val, const int lane);
#define vst4q_lane_s32(ptr, val, lane)  __builtin_mpl_vector_st4q_lane_i32v4(ptr, val, lane)

void __builtin_mpl_vector_st4_lane_u16v4(uint16_t *ptr, uint16x4x4_t val, const int lane);
#define vst4_lane_u16(ptr, val, lane)  __builtin_mpl_vector_st4_lane_u16v4(ptr, val, lane)

void __builtin_mpl_vector_st4q_lane_u16v8(uint16_t *ptr, uint16x8x4_t val, const int lane);
#define vst4q_lane_u16(ptr, val, lane)  __builtin_mpl_vector_st4q_lane_u16v8(ptr, val, lane)

void __builtin_mpl_vector_st4_lane_u32v2(uint32_t *ptr, uint32x2x4_t val, const int lane);
#define vst4_lane_u32(ptr, val, lane)  __builtin_mpl_vector_st4_lane_u32v2(ptr, val, lane)

void __builtin_mpl_vector_st4q_lane_u32v4(uint32_t *ptr, uint32x4x4_t val, const int lane);
#define vst4q_lane_u32(ptr, val, lane)  __builtin_mpl_vector_st4q_lane_u32v4(ptr, val, lane)

void __builtin_mpl_vector_st4q_lane_i8v16(int8_t *ptr, int8x16x4_t val, const int lane);
#define vst4q_lane_s8(ptr, val, lane)  __builtin_mpl_vector_st4q_lane_i8v16(ptr, val, lane)

void __builtin_mpl_vector_st4q_lane_u8v16(uint8_t *ptr, uint8x16x4_t val, const int lane);
#define vst4q_lane_u8(ptr, val, lane)  __builtin_mpl_vector_st4q_lane_u8v16(ptr, val, lane)

void __builtin_mpl_vector_st4_lane_i64v1(int64_t *ptr, int64x1x4_t val, const int lane);
#define vst4_lane_s64(ptr, val, lane)  __builtin_mpl_vector_st4_lane_i64v1(ptr, val, lane)

void __builtin_mpl_vector_st4q_lane_i64v2(int64_t *ptr, int64x2x4_t val, const int lane);
#define vst4q_lane_s64(ptr, val, lane)  __builtin_mpl_vector_st4q_lane_i64v2(ptr, val, lane)

void __builtin_mpl_vector_st4_lane_u64v1(uint64_t *ptr, uint64x1x4_t val, const int lane);
#define vst4_lane_u64(ptr, val, lane)  __builtin_mpl_vector_st4_lane_u64v1(ptr, val, lane)

void __builtin_mpl_vector_st4q_lane_u64v2(uint64_t *ptr, uint64x2x4_t val, const int lane);
#define vst4q_lane_u64(ptr, val, lane)  __builtin_mpl_vector_st4q_lane_u64v2(ptr, val, lane)


#endif // __ARM_NEON_H
