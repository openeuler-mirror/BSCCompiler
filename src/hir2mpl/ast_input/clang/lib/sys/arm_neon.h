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
//     Create a vector by getting the absolute value of the elements in src.
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

// vecTy vector_mov_narrow(vecTy src)
//     copies each element of the operand vector to the corresponding element of the destination vector.
//     The result element is half the width of the operand element, and values are saturated to the result width.
//     The results are the same type as the operands.
uint8x8_t __builtin_mpl_vector_mov_narrow_v8u16(uint16x8_t);
uint16x4_t __builtin_mpl_vector_mov_narrow_v4u32(uint32x4_t);
uint32x2_t __builtin_mpl_vector_mov_narrow_v2u64(uint64x2_t);
int8x8_t __builtin_mpl_vector_mov_narrow_v8i16(int16x8_t);
int16x4_t __builtin_mpl_vector_mov_narrow_v4i32(int32x4_t);
int32x2_t __builtin_mpl_vector_mov_narrow_v2i64(int64x2_t);

// vecTy vector_addl_low(vecTy src1, vecTy src2)
//     Add each element of the source vector to second source
//     widen the result into the destination vector.
int16x8_t __builtin_mpl_vector_addl_low_v8i8(int8x8_t, int8x8_t);
int32x4_t __builtin_mpl_vector_addl_low_v4i16(int16x4_t, int16x4_t);
int64x2_t __builtin_mpl_vector_addl_low_v2i32(int32x2_t, int32x2_t);
uint16x8_t __builtin_mpl_vector_addl_low_v8u8(uint8x8_t, uint8x8_t);
uint32x4_t __builtin_mpl_vector_addl_low_v4u16(uint16x4_t, uint16x4_t);
uint64x2_t __builtin_mpl_vector_addl_low_v2u32(uint32x2_t, uint32x2_t);

// vecTy vector_addl_high(vecTy src1, vecTy src2)
//     Add each element of the source vector to upper half of second source
//     widen the result into the destination vector.
int16x8_t __builtin_mpl_vector_addl_high_v8i8(int8x16_t, int8x16_t);
int32x4_t __builtin_mpl_vector_addl_high_v4i16(int16x8_t, int16x8_t);
int64x2_t __builtin_mpl_vector_addl_high_v2i32(int32x4_t, int32x4_t);
uint16x8_t __builtin_mpl_vector_addl_high_v8u8(uint8x16_t, uint8x16_t);
uint32x4_t __builtin_mpl_vector_addl_high_v4u16(uint16x8_t, uint16x8_t);
uint64x2_t __builtin_mpl_vector_addl_high_v2u32(uint32x4_t, uint32x4_t);

// vecTy vector_addw_low(vecTy src1, vecTy src2)
//     Add each element of the source vector to second source
//     widen the result into the destination vector.
int16x8_t __builtin_mpl_vector_addw_low_v8i8(int16x8_t, int8x8_t);
int32x4_t __builtin_mpl_vector_addw_low_v4i16(int32x4_t, int16x4_t);
int64x2_t __builtin_mpl_vector_addw_low_v2i32(int64x2_t, int32x2_t);
uint16x8_t __builtin_mpl_vector_addw_low_v8u8(uint16x8_t, uint8x8_t);
uint32x4_t __builtin_mpl_vector_addw_low_v4u16(uint32x4_t, uint16x4_t);
uint64x2_t __builtin_mpl_vector_addw_low_v2u32(uint64x2_t, uint32x2_t);

// vecTy vector_addw_high(vecTy src1, vecTy src2)
//     Add each element of the source vector to upper half of second source
//     widen the result into the destination vector.
int16x8_t __builtin_mpl_vector_addw_high_v8i8(int16x8_t, int8x16_t);
int32x4_t __builtin_mpl_vector_addw_high_v4i16(int32x4_t, int16x8_t);
int64x2_t __builtin_mpl_vector_addw_high_v2i32(int64x2_t, int32x4_t);
uint16x8_t __builtin_mpl_vector_addw_high_v8u8(uint16x8_t, uint8x16_t);
uint32x4_t __builtin_mpl_vector_addw_high_v4u16(uint32x4_t, uint16x8_t);
uint64x2_t __builtin_mpl_vector_addw_high_v2u32(uint64x2_t, uint32x4_t);

// vectTy vector_from_scalar(scalarTy val)
//      Create a vector by replicating the scalar value to all elements of the
//      vector.
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
//      Multiply the elements of src1 and src2, then accumulate into accum.
//      Elements of vecTy2 are twice as long as elements of vecTy1.
int64x2_t __builtin_mpl_vector_madd_v2i32(int64x2_t, int32x2_t, int32x2_t);
int32x4_t __builtin_mpl_vector_madd_v4i16(int32x4_t, int16x4_t, int16x4_t);
int16x8_t __builtin_mpl_vector_madd_v8i8(int16x8_t, int8x8_t, int8x8_t);
uint64x2_t __builtin_mpl_vector_madd_v2u32(uint64x2_t, uint32x2_t, uint32x2_t);
uint32x4_t __builtin_mpl_vector_madd_v4u16(uint32x4_t, uint16x4_t, uint16x4_t);
uint16x8_t __builtin_mpl_vector_madd_v8u8(uint16x8_t, uint8x8_t, uint8x8_t);

// vecTy2 vector_mull_low(vecTy1 src1, vecTy1 src2)
//      Multiply the elements of src1 and src2. Elements of vecTy2 are twice as
//      long as elements of vecTy1.
int64x2_t __builtin_mpl_vector_mull_low_v2i32(int32x2_t, int32x2_t);
int32x4_t __builtin_mpl_vector_mull_low_v4i16(int16x4_t, int16x4_t);
int16x8_t __builtin_mpl_vector_mull_low_v8i8(int8x8_t, int8x8_t);
uint64x2_t __builtin_mpl_vector_mull_low_v2u32(uint32x2_t, uint32x2_t);
uint32x4_t __builtin_mpl_vector_mull_low_v4u16(uint16x4_t, uint16x4_t);
uint16x8_t __builtin_mpl_vector_mull_low_v8u8(uint8x8_t, uint8x8_t);

// vecTy2 vector_mull_high(vecTy1 src1, vecTy1 src2)
//      Multiply the upper elements of src1 and src2. Elements of vecTy2 are twice
//      as long as elements of vecTy1.
int64x2_t __builtin_mpl_vector_mull_high_v2i32(int32x4_t, int32x4_t);
int32x4_t __builtin_mpl_vector_mull_high_v4i16(int16x8_t, int16x8_t);
int16x8_t __builtin_mpl_vector_mull_high_v8i8(int8x16_t, int8x16_t);
uint64x2_t __builtin_mpl_vector_mull_high_v2u32(uint32x4_t, uint32x4_t);
uint32x4_t __builtin_mpl_vector_mull_high_v4u16(uint16x8_t, uint16x8_t);
uint16x8_t __builtin_mpl_vector_mull_high_v8u8(uint8x16_t, uint8x16_t);

// vecTy vector_merge(vecTy src1, vecTy src2, int n)
//     Create a vector by concatenating the high elements of src1, starting
//     with the nth element, followed by the low elements of src2.
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
//     Create a vector from the low part of the source vector.
int64x1_t __builtin_mpl_vector_get_low_v2i64(int64x2_t);
int32x2_t __builtin_mpl_vector_get_low_v4i32(int32x4_t);
int16x4_t __builtin_mpl_vector_get_low_v8i16(int16x8_t);
int8x8_t __builtin_mpl_vector_get_low_v16i8(int8x16_t);
uint64x1_t __builtin_mpl_vector_get_low_v2u64(uint64x2_t);
uint32x2_t __builtin_mpl_vector_get_low_v4u32(uint32x4_t);
uint16x4_t __builtin_mpl_vector_get_low_v8u16(uint16x8_t);
uint8x8_t __builtin_mpl_vector_get_low_v16u8(uint8x16_t);
float64x1_t __builtin_mpl_vector_get_low_v2f64(float64x2_t);
float32x2_t __builtin_mpl_vector_get_low_v4f32(float32x4_t);

// vecTy2 vector_get_high(vecTy1 src)
//     Create a vector from the high part of the source vector.
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

// scalarTy vector_get_element(vecTy src, int n)
//     Get the nth element of the source vector.
float64_t __builtin_mpl_vector_get_element_v2f64(float64x2_t, int32_t);
float32_t __builtin_mpl_vector_get_element_v4f32(float32x4_t, int32_t);
float64_t __builtin_mpl_vector_get_element_v1f64(float64x1_t, int32_t);
float32_t __builtin_mpl_vector_get_element_v2f32(float32x2_t, int32_t);

// vecTy vector_set_element(ScalarTy value, VecTy vec, int n)
//     Set the nth element of the source vector to value.
int64x2_t __builtin_mpl_vector_set_element_v2i64(int64_t, int64x2_t, int32_t);
int32x4_t __builtin_mpl_vector_set_element_v4i32(int32_t, int32x4_t, int32_t);
int16x8_t __builtin_mpl_vector_set_element_v8i16(int16_t, int16x8_t, int32_t);
int8x16_t __builtin_mpl_vector_set_element_v16i8(int8_t, int8x16_t, int32_t);
uint64x2_t __builtin_mpl_vector_set_element_v2u64(uint64_t, uint64x2_t,
                                                  int32_t);
uint32x4_t __builtin_mpl_vector_set_element_v4u32(uint32_t, uint32x4_t,
                                                  int32_t);
uint16x8_t __builtin_mpl_vector_set_element_v8u16(uint16_t, uint16x8_t,
                                                  int32_t);
uint8x16_t __builtin_mpl_vector_set_element_v16u8(uint8_t, uint8x16_t, int32_t);
float64x2_t __builtin_mpl_vector_set_element_v2f64(float64_t, float64x2_t,
                                                   int32_t);
float32x4_t __builtin_mpl_vector_set_element_v4f32(float32_t, float32x4_t,
                                                   int32_t);
int64x1_t __builtin_mpl_vector_set_element_v1i64(int64_t, int64x1_t, int32_t);
int32x2_t __builtin_mpl_vector_set_element_v2i32(int32_t, int32x2_t, int32_t);
int16x4_t __builtin_mpl_vector_set_element_v4i16(int16_t, int16x4_t, int32_t);
int8x8_t __builtin_mpl_vector_set_element_v8i8(int8_t, int8x8_t, int32_t);
uint64x1_t __builtin_mpl_vector_set_element_v1u64(uint64_t, uint64x1_t,
                                                  int32_t);
uint32x2_t __builtin_mpl_vector_set_element_v2u32(uint32_t, uint32x2_t,
                                                  int32_t);
uint16x4_t __builtin_mpl_vector_set_element_v4u16(uint16_t, uint16x4_t,
                                                  int32_t);
uint8x8_t __builtin_mpl_vector_set_element_v8u8(uint8_t, uint8x8_t, int32_t);
float64x1_t __builtin_mpl_vector_set_element_v1f64(float64_t, float64x1_t,
                                                   int32_t);
float32x2_t __builtin_mpl_vector_set_element_v2f32(float32_t, float32x2_t,
                                                   int32_t);

// vecTy2 vector_abdl(vectTy1 src2, vectTy2 src2)
//     Create a widened vector by getting the abs value of subtracted arguments.
int16x8_t __builtin_mpl_vector_labssub_low_v8i8(int8x8_t, int8x8_t);
int32x4_t __builtin_mpl_vector_labssub_low_v4i16(int16x4_t, int16x4_t);
int64x2_t __builtin_mpl_vector_labssub_low_v2i32(int32x2_t, int32x2_t);
uint16x8_t __builtin_mpl_vector_labssub_low_v8u8(uint8x8_t, uint8x8_t);
uint32x4_t __builtin_mpl_vector_labssub_low_v4u16(uint16x4_t, uint16x4_t);
uint64x2_t __builtin_mpl_vector_labssub_low_v2u32(uint32x2_t, uint32x2_t);

// vecTy2 vector_abdl_high(vectTy1 src2, vectTy2 src2)
//     Create a widened vector by getting the abs value of subtracted high arguments.
int16x8_t __builtin_mpl_vector_labssub_high_v8i8(int8x16_t, int8x16_t);
int32x4_t __builtin_mpl_vector_labssub_high_v4i16(int16x8_t, int16x8_t);
int64x2_t __builtin_mpl_vector_labssub_high_v2i32(int32x4_t, int32x4_t);
uint16x8_t __builtin_mpl_vector_labssub_high_v8u8(uint8x16_t, uint8x16_t);
uint32x4_t __builtin_mpl_vector_labssub_high_v4u16(uint16x8_t, uint16x8_t);
uint64x2_t __builtin_mpl_vector_labssub_high_v2u32(uint32x4_t, uint32x4_t);

// vecTy2 vector_narrow_low(vecTy1 src)
//     Narrow each element of the source vector to half of the original width,
//     writing the lower half into the destination vector.
int32x2_t __builtin_mpl_vector_narrow_low_v2i64(int64x2_t);
int16x4_t __builtin_mpl_vector_narrow_low_v4i32(int32x4_t);
int8x8_t __builtin_mpl_vector_narrow_low_v8i16(int16x8_t);
uint32x2_t __builtin_mpl_vector_narrow_low_v2u64(uint64x2_t);
uint16x4_t __builtin_mpl_vector_narrow_low_v4u32(uint32x4_t);
uint8x8_t __builtin_mpl_vector_narrow_low_v8u16(uint16x8_t);

// vecTy2 vector_narrow_high(vecTy1 src1, vecTy2 src2)
//     Narrow each element of the source vector to half of the original width,
//     concatenate the upper half into the destination vector.
int32x4_t __builtin_mpl_vector_narrow_high_v2i64(int32x2_t, int64x2_t);
int16x8_t __builtin_mpl_vector_narrow_high_v4i32(int16x4_t, int32x4_t);
int8x16_t __builtin_mpl_vector_narrow_high_v8i16(int8x8_t, int16x8_t);
uint32x4_t __builtin_mpl_vector_narrow_high_v2u64(uint32x2_t, uint64x2_t);
uint16x8_t __builtin_mpl_vector_narrow_high_v4u32(uint16x4_t, uint32x4_t);
uint8x16_t __builtin_mpl_vector_narrow_high_v8u16(uint8x8_t, uint16x8_t);

// vecTy1 vector_adapl(vecTy1 src1, vecTy2 src2)
//     Vector pairwise addition and accumulate
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
//     Add pairs of elements from the source vector and put the result into the
//     destination vector, whose element size is twice and the number of
//     elements is half of the source vector type.
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
//     Create a vector by reversing the order of the elements in src.
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
//     Shift each element in the vector left by n.
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

// vecTy vector_shri(vecTy src, const int n)
//     Shift each element in the vector right by n.
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

// vecTy2 vector_shift_narrow_low(vecTy1 src, const int n)
//     Shift each element in the vector right by n, narrow each element to half
//     of the original width (truncating), then write the result to the lower
//     half of the destination vector.
int32x2_t __builtin_mpl_vector_shr_narrow_low_v2i64(int64x2_t, const int);
int16x4_t __builtin_mpl_vector_shr_narrow_low_v4i32(int32x4_t, const int);
int8x8_t __builtin_mpl_vector_shr_narrow_low_v8i16(int16x8_t, const int);
uint32x2_t __builtin_mpl_vector_shr_narrow_low_v2u64(uint64x2_t, const int);
uint16x4_t __builtin_mpl_vector_shr_narrow_low_v4u32(uint32x4_t, const int);
uint8x8_t __builtin_mpl_vector_shr_narrow_low_v8u16(uint16x8_t, const int);

// scalarTy vector_sum(vecTy src)
//     Sum all of the elements in the vector into a scalar.
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
//     Performs a table vector lookup.
float64x2_t __builtin_mpl_vector_table_lookup_v2f64(float64x2_t, float64x2_t);
float32x4_t __builtin_mpl_vector_table_lookup_v4f32(float32x4_t, float32x4_t);
float64x1_t __builtin_mpl_vector_table_lookup_v1f64(float64x1_t, float64x1_t);
float32x2_t __builtin_mpl_vector_table_lookup_v2f32(float32x2_t, float32x2_t);

// vecTy2 vector_widen_low(vecTy1 src)
//     Widen each element of the source vector to half of the original width,
//     writing the lower half into the destination vector.
int64x2_t __builtin_mpl_vector_widen_low_v2i32(int32x2_t);
int32x4_t __builtin_mpl_vector_widen_low_v4i16(int16x4_t);
int16x8_t __builtin_mpl_vector_widen_low_v8i8(int8x8_t);
uint64x2_t __builtin_mpl_vector_widen_low_v2u32(uint32x2_t);
uint32x4_t __builtin_mpl_vector_widen_low_v4u16(uint16x4_t);
uint16x8_t __builtin_mpl_vector_widen_low_v8u8(uint8x8_t);

// vecTy2 vector_widen_high(vecTy1 src)
//     Widen each element of the source vector to half of the original width,
//     writing the higher half into the destination vector.
int64x2_t __builtin_mpl_vector_widen_high_v2i32(int32x4_t);
int32x4_t __builtin_mpl_vector_widen_high_v4i16(int16x8_t);
int16x8_t __builtin_mpl_vector_widen_high_v8i8(int8x16_t);
uint64x2_t __builtin_mpl_vector_widen_high_v2u32(uint32x4_t);
uint32x4_t __builtin_mpl_vector_widen_high_v4u16(uint16x8_t);
uint16x8_t __builtin_mpl_vector_widen_high_v8u8(uint8x16_t);

// vecTy vector_load(scalarTy *ptr)
//     Load the elements pointed to by ptr into a vector.
float64x2_t __builtin_mpl_vector_load_v2f64(float64_t *);
float32x4_t __builtin_mpl_vector_load_v4f32(float32_t *);
float64x1_t __builtin_mpl_vector_load_v1f64(float64_t *);
float32x2_t __builtin_mpl_vector_load_v2f32(float32_t *);

// void vector_store(scalarTy *ptr, vecTy src)
//     Store the elements from src into the memory pointed to by ptr.
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
//     Subtract each element of the source vector to second source
//     widen the result into the destination vector.
int16x8_t __builtin_mpl_vector_subl_low_v8i8(int8x8_t, int8x8_t);
int32x4_t __builtin_mpl_vector_subl_low_v4i16(int16x4_t, int16x4_t);
int64x2_t __builtin_mpl_vector_subl_low_v2i32(int32x2_t, int32x2_t);
uint16x8_t __builtin_mpl_vector_subl_low_v8u8(uint8x8_t, uint8x8_t);
uint32x4_t __builtin_mpl_vector_subl_low_v4u16(uint16x4_t, uint16x4_t);
uint64x2_t __builtin_mpl_vector_subl_low_v2u32(uint32x2_t, uint32x2_t);

// vecTy vector_subl_high(vecTy src1, vecTy src2)
//     Subtract each element of the source vector to upper half of second source
//     widen the result into the destination vector.
int16x8_t __builtin_mpl_vector_subl_high_v8i8(int8x16_t, int8x16_t);
int32x4_t __builtin_mpl_vector_subl_high_v4i16(int16x8_t, int16x8_t);
int64x2_t __builtin_mpl_vector_subl_high_v2i32(int32x4_t, int32x4_t);
uint16x8_t __builtin_mpl_vector_subl_high_v8u8(uint8x16_t, uint8x16_t);
uint32x4_t __builtin_mpl_vector_subl_high_v4u16(uint16x8_t, uint16x8_t);
uint64x2_t __builtin_mpl_vector_subl_high_v2u32(uint32x4_t, uint32x4_t);

// vecTy vector_subw_low(vecTy src1, vecTy src2)
//     Subtract each element of the source vector to second source
//     widen the result into the destination vector.
int16x8_t __builtin_mpl_vector_subw_low_v8i8(int16x8_t, int8x8_t);
int32x4_t __builtin_mpl_vector_subw_low_v4i16(int32x4_t, int16x4_t);
int64x2_t __builtin_mpl_vector_subw_low_v2i32(int64x2_t, int32x2_t);
uint16x8_t __builtin_mpl_vector_subw_low_v8u8(uint16x8_t, uint8x8_t);
uint32x4_t __builtin_mpl_vector_subw_low_v4u16(uint32x4_t, uint16x4_t);
uint64x2_t __builtin_mpl_vector_subw_low_v2u32(uint64x2_t, uint32x2_t);

// vecTy vector_subw_high(vecTy src1, vecTy src2)
//     Subtract each element of the source vector to upper half of second source
//     widen the result into the destination vector.
int16x8_t __builtin_mpl_vector_subw_high_v8i8(int16x8_t, int8x16_t);
int32x4_t __builtin_mpl_vector_subw_high_v4i16(int32x4_t, int16x8_t);
int64x2_t __builtin_mpl_vector_subw_high_v2i32(int64x2_t, int32x4_t);
uint16x8_t __builtin_mpl_vector_subw_high_v8u8(uint16x8_t, uint8x16_t);
uint32x4_t __builtin_mpl_vector_subw_high_v4u16(uint32x4_t, uint16x8_t);
uint64x2_t __builtin_mpl_vector_subw_high_v2u32(uint64x2_t, uint32x4_t);

// *************************
// Supported Neon Intrinsics
// *************************

// vabdl
#define vabdl_s8(a, b) __builtin_mpl_vector_labssub_low_v8i8(a, b)
#define vabdl_s16(a, b) __builtin_mpl_vector_labssub_low_v4i16(a, b)
#define vabdl_s32(a, b) __builtin_mpl_vector_labssub_low_v2i32(a, b)
#define vabdl_u8(a, b) __builtin_mpl_vector_labssub_low_v8u8(a, b)
#define vabdl_u16(a, b) __builtin_mpl_vector_labssub_low_v4u16(a, b)
#define vabdl_u32(a, b) __builtin_mpl_vector_labssub_low_v2u32(a, b)

// vabdl_high
#define vabdl_high_s8(a, b) __builtin_mpl_vector_labssub_high_v8i8(a, b)
#define vabdl_high_s16(a, b) __builtin_mpl_vector_labssub_high_v4i16(a, b)
#define vabdl_high_s32(a, b) __builtin_mpl_vector_labssub_high_v2i32(a, b)
#define vabdl_high_u8(a, b) __builtin_mpl_vector_labssub_high_v8u8(a, b)
#define vabdl_high_u16(a, b) __builtin_mpl_vector_labssub_high_v4u16(a, b)
#define vabdl_high_u32(a, b) __builtin_mpl_vector_labssub_high_v2u32(a, b)

// vabs
#define vabs_s8(a) __builtin_mpl_vector_abs_v8i8(a)
#define vabs_s16(a) __builtin_mpl_vector_abs_v4i16(a)
#define vabs_s32(a) __builtin_mpl_vector_abs_v2i32(a)
#define vabs_s64(a) __builtin_mpl_vector_abs_v1i64(a)
#define vabs_f32(a) __builtin_mpl_vector_abs_v2f32(a)
#define vabs_f64(a) __builtin_mpl_vector_abs_v1f64(a)
#define vabsq_s8(a) __builtin_mpl_vector_abs_v16i8(a)
#define vabsq_s16(a) __builtin_mpl_vector_abs_v8i16(a)
#define vabsq_s32(a) __builtin_mpl_vector_abs_v4i32(a)
#define vabsq_s64(a) __builtin_mpl_vector_abs_v2i64(a)
#define vabsq_f32(a) __builtin_mpl_vector_abs_v4f32(a)
#define vabsq_f64(a) __builtin_mpl_vector_abs_v2f64(a)

// vaddv
#define vaddv_s8(a)  __builtin_mpl_vector_sum_v8i8(a)
#define vaddv_s16(a) __builtin_mpl_vector_sum_v4i16(a)
#define vaddv_s32(a) (vget_lane_s32(__builtin_mpl_vector_padd_v2i32(a, a), 0))
#define vaddv_u8(a) __builtin_mpl_vector_sum_v8u8(a)
#define vaddv_u16(a) __builtin_mpl_vector_sum_v4u16(a)
#define vaddv_u32(a) (vget_lane_u32(__builtin_mpl_vector_padd_v2u32(a, a), 0))
#define vaddv_f32(a) __builtin_mpl_vector_sum_v2f32(a)
#define vaddvq_s8(a) __builtin_mpl_vector_sum_v16i8(a)
#define vaddvq_s16(a) __builtin_mpl_vector_sum_v8i16(a)
#define vaddvq_s32(a) __builtin_mpl_vector_sum_v4i32(a)
#define vaddvq_s64(a) __builtin_mpl_vector_sum_v2i64(a)
#define vaddvq_u8(a) __builtin_mpl_vector_sum_v16u8(a)
#define vaddvq_u16(a) __builtin_mpl_vector_sum_v8u16(a)
#define vaddvq_u32(a) __builtin_mpl_vector_sum_v4u32(a)
#define vaddvq_u64(a) __builtin_mpl_vector_sum_v2u64(a)
#define vaddvq_f32(a) __builtin_mpl_vector_sum_v4f32(a)
#define vaddvq_f64(a) __builtin_mpl_vector_sum_v2f64(a)

// vqmovn
#define vqmovn_u16(a) __builtin_mpl_vector_mov_narrow_v8u16(a)
#define vqmovn_u32(a) __builtin_mpl_vector_mov_narrow_v4u32(a)
#define vqmovn_u64(a) __builtin_mpl_vector_mov_narrow_v2u64(a)
#define vqmovn_s16(a) __builtin_mpl_vector_mov_narrow_v8i16(a)
#define vqmovn_s32(a) __builtin_mpl_vector_mov_narrow_v4i32(a)
#define vqmovn_s64(a) __builtin_mpl_vector_mov_narrow_v2i64(a)

// vaddl
#define vaddl_s8(a, b) __builtin_mpl_vector_addl_low_v8i8(a, b)
#define vaddl_s16(a, b) __builtin_mpl_vector_addl_low_v4i16(a, b)
#define vaddl_s32(a, b) __builtin_mpl_vector_addl_low_v2i32(a, b)
#define vaddl_u8(a, b) __builtin_mpl_vector_addl_low_v8u8(a, b)
#define vaddl_u16(a, b) __builtin_mpl_vector_addl_low_v4u16(a, b)
#define vaddl_u32(a, b) __builtin_mpl_vector_addl_low_v2u32(a, b)

// vaddl_high
#define vaddl_high_s8(a, b) __builtin_mpl_vector_addl_high_v8i8(a, b)
#define vaddl_high_s16(a, b) __builtin_mpl_vector_addl_high_v4i16(a, b)
#define vaddl_high_s32(a, b) __builtin_mpl_vector_addl_high_v2i32(a, b)
#define vaddl_high_u8(a, b) __builtin_mpl_vector_addl_high_v8u8(a, b)
#define vaddl_high_u16(a, b) __builtin_mpl_vector_addl_high_v4u16(a, b)
#define vaddl_high_u32(a, b) __builtin_mpl_vector_addl_high_v2u32(a, b)

// vaddw
#define vaddw_s8(a, b) __builtin_mpl_vector_addw_low_v8i8(a, b)
#define vaddw_s16(a, b) __builtin_mpl_vector_addw_low_v4i16(a, b)
#define vaddw_s32(a, b) __builtin_mpl_vector_addw_low_v2i32(a, b)
#define vaddw_u8(a, b) __builtin_mpl_vector_addw_low_v8u8(a, b)
#define vaddw_u16(a, b) __builtin_mpl_vector_addw_low_v4u16(a, b)
#define vaddw_u32(a, b) __builtin_mpl_vector_addw_low_v2u32(a, b)

// vaddw_high
#define vaddw_high_s8(a, b) __builtin_mpl_vector_addw_high_v8i8(a, b)
#define vaddw_high_s16(a, b) __builtin_mpl_vector_addw_high_v4i16(a, b)
#define vaddw_high_s32(a, b) __builtin_mpl_vector_addw_high_v2i32(a, b)
#define vaddw_high_u8(a, b) __builtin_mpl_vector_addw_high_v8u8(a, b)
#define vaddw_high_u16(a, b) __builtin_mpl_vector_addw_high_v4u16(a, b)
#define vaddw_high_u32(a, b) __builtin_mpl_vector_addw_high_v2u32(a, b)

// vadd
#define vadd_s8(a, b) (a + b)
#define vadd_s16(a, b) (a + b)
#define vadd_s32(a, b) (a + b)
#define vadd_s64(a, b) (a + b)
#define vadd_u8(a, b) (a + b)
#define vadd_u16(a, b) (a + b)
#define vadd_u32(a, b) (a + b)
#define vadd_u64(a, b) (a + b)
#define vadd_f16(a, b) (a + b)
#define vadd_f32(a, b) (a + b)
#define vadd_f64(a, b) (a + b)
#define vaddq_s8(a, b) (a + b)
#define vaddq_s16(a, b) (a + b)
#define vaddq_s32(a, b) (a + b)
#define vaddq_s64(a, b) (a + b)
#define vaddq_u8(a, b) (a + b)
#define vaddq_u16(a, b) (a + b)
#define vaddq_u32(a, b) (a + b)
#define vaddq_u64(a, b) (a + b)
#define vaddq_f16(a, b) (a + b)
#define vaddq_f32(a, b) (a + b)
#define vaddq_f64(a, b) (a + b)
#define vaddd_s64(a, b) (a + b)
#define vaddd_u64(a, b) (a + b)

// vand
#define vand_s8(a, b) (a & b)
#define vand_s16(a, b) (a & b)
#define vand_s32(a, b) (a & b)
#define vand_s64(a, b) (a & b)
#define vand_u8(a, b) (a & b)
#define vand_u16(a, b) (a & b)
#define vand_u32(a, b) (a & b)
#define vand_u64(a, b) (a & b)
#define vandq_s8(a, b) (a & b)
#define vandq_s16(a, b) (a & b)
#define vandq_s32(a, b) (a & b)
#define vandq_s64(a, b) (a & b)
#define vandq_u8(a, b) (a & b)
#define vandq_u16(a, b) (a & b)
#define vandq_u32(a, b) (a & b)
#define vandq_u64(a, b) (a & b)

// vand
#define vorr_s8(a, b) (a | b)
#define vorr_s16(a, b) (a | b)
#define vorr_s32(a, b) (a | b)
#define vorr_s64(a, b) (a | b)
#define vorr_u8(a, b) (a | b)
#define vorr_u16(a, b) (a | b)
#define vorr_u32(a, b) (a | b)
#define vorr_u64(a, b) (a | b)
#define vorrq_s8(a, b) (a | b)
#define vorrq_s16(a, b) (a | b)
#define vorrq_s32(a, b) (a | b)
#define vorrq_s64(a, b) (a | b)
#define vorrq_u8(a, b) (a | b)
#define vorrq_u16(a, b) (a | b)
#define vorrq_u32(a, b) (a | b)
#define vorrq_u64(a, b) (a | b)

// vdup
#define vdup_n_s8(a)  __builtin_mpl_vector_from_scalar_v8i8(a)
#define vdup_n_s16(a) __builtin_mpl_vector_from_scalar_v4i16(a)
#define vdup_n_s32(a) __builtin_mpl_vector_from_scalar_v2i32(a)
#define vdup_n_s64(a) __builtin_mpl_vector_from_scalar_v1i64(a)
#define vdup_n_u8(a) __builtin_mpl_vector_from_scalar_v8u8(a)
#define vdup_n_u16(a) __builtin_mpl_vector_from_scalar_v4u16(a)
#define vdup_n_u32(a) __builtin_mpl_vector_from_scalar_v2u32(a)
#define vdup_n_u64(a) __builtin_mpl_vector_from_scalar_v1u64(a)
#define vdup_n_f16(a) __builtin_mpl_vector_from_scalar_v4f16(a)
#define vdup_n_f32(a) __builtin_mpl_vector_from_scalar_v2f32(a)
#define vdup_n_f64(a) __builtin_mpl_vector_from_scalar_v1f64(a)
#define vdupq_n_s8(a) __builtin_mpl_vector_from_scalar_v16i8(a)
#define vdupq_n_s16(a) __builtin_mpl_vector_from_scalar_v8i16(a)
#define vdupq_n_s32(a) __builtin_mpl_vector_from_scalar_v4i32(a)
#define vdupq_n_s64(a) __builtin_mpl_vector_from_scalar_v2i64(a)
#define vdupq_n_u8(a) __builtin_mpl_vector_from_scalar_v16u8(a)
#define vdupq_n_u16(a) __builtin_mpl_vector_from_scalar_v8u16(a)
#define vdupq_n_u32(a) __builtin_mpl_vector_from_scalar_v4u32(a)
#define vdupq_n_u64(a) __builtin_mpl_vector_from_scalar_v2u64(a)
#define vdupq_n_f16(a) __builtin_mpl_vector_from_scalar_v8f16(a)
#define vdupq_n_f32(a) __builtin_mpl_vector_from_scalar_v4f32(a)
#define vdupq_n_f64(a) __builtin_mpl_vector_from_scalar_v2f64(a)

// vceq
#define vceq_s8(a, b) (a == b)
#define vceq_s16(a, b) (a == b)
#define vceq_s32(a, b) (a == b)
#define vceq_s64(a, b) (a == b)
#define vceq_u8(a, b) (a == b)
#define vceq_u16(a, b) (a == b)
#define vceq_u32(a, b) (a == b)
#define vceq_u64(a, b) (a == b)
#define vceq_f16(a, b) (a == b)
#define vceq_f32(a, b) (a == b)
#define vceq_f64(a, b) (a == b)
#define vceqq_s8(a, b) (a == b)
#define vceqq_s16(a, b) (a == b)
#define vceqq_s32(a, b) (a == b)
#define vceqq_s64(a, b) (a == b)
#define vceqq_u8(a, b) (a == b)
#define vceqq_u16(a, b) (a == b)
#define vceqq_u32(a, b) (a == b)
#define vceqq_u64(a, b) (a == b)
#define vceqq_f16(a, b) (a == b)
#define vceqq_f32(a, b) (a == b)
#define vceqq_f64(a, b) (a == b)
#define vceqd_s64(a, b) ((a == b) ? -1LL : 0LL)
#define vceqd_u64(a, b) ((a == b) ? -1LL : 0LL)

// vceqz
#define vceqz_s8(a) (a == 0)
#define vceqzq_s8(a) (a == 0)
#define vceqz_s16(a) (a == 0)
#define vceqzq_s16(a) (a == 0)
#define vceqz_s32(a) (a == 0)
#define vceqzq_s32(a) (a == 0)
#define vceqz_u8(a) (a == 0)
#define vceqzq_u8(a) (a == 0)
#define vceqz_u16(a) (a == 0)
#define vceqzq_u16(a) (a == 0)
#define vceqz_u32(a) (a == 0)
#define vceqzq_u32(a) (a == 0)
#define vceqz_p8(a) (a == 0)
#define vceqzq_p8(a) (a == 0)
#define vceqz_s64(a) (a == 0)
#define vceqzq_s64(a) (a == 0)
#define vceqz_u64(a) (a == 0)
#define vceqzq_u64(a) (a == 0)
#define vceqz_p64(a) (a == 0)
#define vceqzq_p64(a) (a == 0)
#define vceqzd_s64(a) ((a == 0) ? -1LL : 0LL)
#define vceqzd_u64(a) ((a == 0) ? -1LL : 0LL)

// vcgt
#define vcgt_s8(a, b) (a > b)
#define vcgt_s16(a, b) (a > b)
#define vcgt_s32(a, b) (a > b)
#define vcgt_s64(a, b) (a > b)
#define vcgt_u8(a, b) (a > b)
#define vcgt_u16(a, b) (a > b)
#define vcgt_u32(a, b) (a > b)
#define vcgt_u64(a, b) (a > b)
#define vcgt_f16(a, b) (a > b)
#define vcgt_f32(a, b) (a > b)
#define vcgt_f64(a, b) (a > b)
#define vcgtq_s8(a, b) (a > b)
#define vcgtq_s16(a, b) (a > b)
#define vcgtq_s32(a, b) (a > b)
#define vcgtq_s64(a, b) (a > b)
#define vcgtq_u8(a, b) (a > b)
#define vcgtq_u16(a, b) (a > b)
#define vcgtq_u32(a, b) (a > b)
#define vcgtq_u64(a, b) (a > b)
#define vcgtq_f16(a, b) (a > b)
#define vcgtq_f32(a, b) (a > b)
#define vcgtq_f64(a, b) (a > b)
#define vcgtd_s64(a, b) ((a > b) ? -1LL : 0LL)
#define vcgtd_u64(a, b) ((a > b) ? -1LL : 0LL)

// vcgtz
#define vcgtz_s8(a) (a > 0)
#define vcgtzq_s8(a) (a > 0)
#define vcgtz_s16(a) (a > 0)
#define vcgtzq_s16(a) (a > 0)
#define vcgtz_s32(a) (a > 0)
#define vcgtzq_s32(a) (a > 0)
#define vcgtz_s64(a) (a > 0)
#define vcgtzq_s64(a) (a > 0)
#define vcgtzd_s64(a) ((a > 0) ? -1LL : 0LL)

// vcge
#define vcge_s8(a, b) (a >= b)
#define vcge_s16(a, b) (a >= b)
#define vcge_s32(a, b) (a >= b)
#define vcge_s64(a, b) (a >= b)
#define vcge_u8(a, b) (a >= b)
#define vcge_u16(a, b) (a >= b)
#define vcge_u32(a, b) (a >= b)
#define vcge_u64(a, b) (a >= b)
#define vcge_f16(a, b) (a >= b)
#define vcge_f32(a, b) (a >= b)
#define vcge_f64(a, b) (a >= b)
#define vcgeq_s8(a, b) (a >= b)
#define vcgeq_s16(a, b) (a >= b)
#define vcgeq_s32(a, b) (a >= b)
#define vcgeq_s64(a, b) (a >= b)
#define vcgeq_u8(a, b) (a >= b)
#define vcgeq_u16(a, b) (a >= b)
#define vcgeq_u32(a, b) (a >= b)
#define vcgeq_u64(a, b) (a >= b)
#define vcgeq_f16(a, b) (a >= b)
#define vcgeq_f32(a, b) (a >= b)
#define vcgeq_f64(a, b) (a >= b)
#define vcged_s64(a, b) ((a >= b) ? -1LL : 0LL)
#define vcged_u64(a, b) ((a >= b) ? -1LL : 0LL)

// vcgez
#define vcgez_s8(a) (a >= 0)
#define vcgezq_s8(a) (a >= 0)
#define vcgez_s16(a) (a >= 0)
#define vcgezq_s16(a) (a >= 0)
#define vcgez_s32(a) (a >= 0)
#define vcgezq_s32(a) (a >= 0)
#define vcgez_s64(a) (a >= 0)
#define vcgezq_s64(a) (a >= 0)
#define vcgezd_s64(a) ((a >= 0) ? -1LL : 0LL)

// vclt
#define vclt_s8(a, b) (a < b)
#define vclt_s16(a, b) (a < b)
#define vclt_s32(a, b) (a < b)
#define vclt_s64(a, b) (a < b)
#define vclt_u8(a, b) (a < b)
#define vclt_u16(a, b) (a < b)
#define vclt_u32(a, b) (a < b)
#define vclt_u64(a, b) (a < b)
#define vclt_f16(a, b) (a < b)
#define vclt_f32(a, b) (a < b)
#define vclt_f64(a, b) (a < b)
#define vcltq_s8(a, b) (a < b)
#define vcltq_s16(a, b) (a < b)
#define vcltq_s32(a, b) (a < b)
#define vcltq_s64(a, b) (a < b)
#define vcltq_u8(a, b) (a < b)
#define vcltq_u16(a, b) (a < b)
#define vcltq_u32(a, b) (a < b)
#define vcltq_u64(a, b) (a < b)
#define vcltq_f16(a, b) (a < b)
#define vcltq_f32(a, b) (a < b)
#define vcltq_f64(a, b) (a < b)
#define vcltd_s64(a, b) ((a < b) ? -1LL : 0LL)
#define vcltd_u64(a, b) ((a < b) ? -1LL : 0LL)

// vcltz
#define vcltz_s8(a) (a < 0)
#define vcltzq_s8(a) (a < 0)
#define vcltz_s16(a) (a < 0)
#define vcltzq_s16(a) (a < 0)
#define vcltz_s32(a) (a < 0)
#define vcltzq_s32(a) (a < 0)
#define vcltz_s64(a) (a < 0)
#define vcltzq_s64(a) (a < 0)
#define vcltzd_s64(a) ((a < 0) ? -1LL : 0LL)

// vcle
#define vcle_s8(a, b) (a <= b)
#define vcle_s16(a, b) (a <= b)
#define vcle_s32(a, b) (a <= b)
#define vcle_s64(a, b) (a <= b)
#define vcle_u8(a, b) (a <= b)
#define vcle_u16(a, b) (a <= b)
#define vcle_u32(a, b) (a <= b)
#define vcle_u64(a, b) (a <= b)
#define vcle_f16(a, b) (a <= b)
#define vcle_f32(a, b) (a <= b)
#define vcle_f64(a, b) (a <= b)
#define vcleq_s8(a, b) (a <= b)
#define vcleq_s16(a, b) (a <= b)
#define vcleq_s32(a, b) (a <= b)
#define vcleq_s64(a, b) (a <= b)
#define vcleq_u8(a, b) (a <= b)
#define vcleq_u16(a, b) (a <= b)
#define vcleq_u32(a, b) (a <= b)
#define vcleq_u64(a, b) (a <= b)
#define vcleq_f16(a, b) (a <= b)
#define vcleq_f32(a, b) (a <= b)
#define vcleq_f64(a, b) (a <= b)
#define vcled_s64(a, b) ((a <= b) ? -1LL : 0LL)
#define vcled_u64(a, b) ((a <= b) ? -1LL : 0LL)

// vclez
#define vclez_s8(a) (a <= 0)
#define vclezq_s8(a) (a <= 0)
#define vclez_s16(a) (a <= 0)
#define vclezq_s16(a) (a <= 0)
#define vclez_s32(a) (a <= 0)
#define vclezq_s32(a) (a <= 0)
#define vclez_s64(a) (a <= 0)
#define vclezq_s64(a) (a <= 0)
#define vclezd_s64(a) ((a <= 0) ? -1LL : 0LL)

// veor
#define veor_s8(a, b) (a ^ b)
#define veor_s16(a, b) (a ^ b)
#define veor_s32(a, b) (a ^ b)
#define veor_s64(a, b) (a ^ b)
#define veor_u8(a, b) (a ^ b)
#define veor_u16(a, b) (a ^ b)
#define veor_u32(a, b) (a ^ b)
#define veor_u64(a, b) (a ^ b)
#define veorq_s8(a, b) (a ^ b)
#define veorq_s16(a, b) (a ^ b)
#define veorq_s32(a, b) (a ^ b)
#define veorq_s64(a, b) (a ^ b)
#define veorq_u8(a, b) (a ^ b)
#define veorq_u16(a, b) (a ^ b)
#define veorq_u32(a, b) (a ^ b)
#define veorq_u64(a, b) (a ^ b)

// vext
#define vext_s8(a, b, n)  __builtin_mpl_vector_merge_v8i8(a, b, n)
#define vext_s16(a, b, n) __builtin_mpl_vector_merge_v4i16(a, b, n)
#define vext_s32(a, b, n) __builtin_mpl_vector_merge_v2i32(a, b, n)
#define vext_s64(a, b, n) __builtin_mpl_vector_merge_v1i64(a, b, n)
#define vext_u8(a, b, n) __builtin_mpl_vector_merge_v8u8(a, b, n)
#define vext_u16(a, b, n) __builtin_mpl_vector_merge_v4u16(a, b, n)
#define vext_u32(a, b, n) __builtin_mpl_vector_merge_v2u32(a, b, n)
#define vext_u64(a, b, n) __builtin_mpl_vector_merge_v1u64(a, b, n)
#define vext_f16(a, b, n) __builtin_mpl_vector_merge_v4f16(a, b, n)
#define vext_f32(a, b, n) __builtin_mpl_vector_merge_v2f32(a, b, n)
#define vext_f64(a, b, n) __builtin_mpl_vector_merge_v1f64(a, b, n)
#define vextq_s8(a, b, n) __builtin_mpl_vector_merge_v16i8(a, b, n)
#define vextq_s16(a, b, n) __builtin_mpl_vector_merge_v8i16(a, b, n)
#define vextq_s32(a, b, n) __builtin_mpl_vector_merge_v4i32(a, b, n)
#define vextq_s64(a, b, n) __builtin_mpl_vector_merge_v2i64(a, b, n)
#define vextq_u8(a, b, n) __builtin_mpl_vector_merge_v16u8(a, b, n)
#define vextq_u16(a, b, n) __builtin_mpl_vector_merge_v8u16(a, b, n)
#define vextq_u32(a, b, n) __builtin_mpl_vector_merge_v4u32(a, b, n)
#define vextq_u64(a, b, n) __builtin_mpl_vector_merge_v2u64(a, b, n)
#define vextq_f16(a, b, n) __builtin_mpl_vector_merge_v8f16(a, b, n)
#define vextq_f32(a, b, n) __builtin_mpl_vector_merge_v4f32(a, b, n)
#define vextq_f64(a, b, n) __builtin_mpl_vector_merge_v2f64(a, b, n)

// vget_high
#define vget_high_s8(a)  __builtin_mpl_vector_get_high_v16i8(a)
#define vget_high_s16(a) __builtin_mpl_vector_get_high_v8i16(a)
#define vget_high_s32(a) __builtin_mpl_vector_get_high_v4i32(a)
#define vget_high_s64(a) __builtin_mpl_vector_get_high_v2i64(a)
#define vget_high_u8(a) __builtin_mpl_vector_get_high_v16u8(a)
#define vget_high_u16(a) __builtin_mpl_vector_get_high_v8u16(a)
#define vget_high_u32(a) __builtin_mpl_vector_get_high_v4u32(a)
#define vget_high_u64(a) __builtin_mpl_vector_get_high_v2u64(a)
#define vget_high_f16(a) __builtin_mpl_vector_get_high_v4f16(a)
#define vget_high_f32(a) __builtin_mpl_vector_get_high_v2f32(a)
#define vget_high_f64(a) __builtin_mpl_vector_get_high_v1f64(a)

// vget_lane
#define vget_lane_f16(a, n) __builtin_mpl_vector_get_element_v4f16(a, n)
#define vget_lane_f32(a, n) __builtin_mpl_vector_get_element_v2f32(a, n)
#define vget_lane_f64(a, n) __builtin_mpl_vector_get_element_v1f64(a, n)
#define vgetq_lane_f16(a, n) __builtin_mpl_vector_get_element_v8f16(a, n)
#define vgetq_lane_f32(a, n) __builtin_mpl_vector_get_element_v4f32(a, n)
#define vgetq_lane_f64(a, n) __builtin_mpl_vector_get_element_v2f64(a, n)

// vget_low
#define vget_low_s8(a)  __builtin_mpl_vector_get_low_v16i8(a)
#define vget_low_s16(a) __builtin_mpl_vector_get_low_v8i16(a)
#define vget_low_s32(a) __builtin_mpl_vector_get_low_v4i32(a)
#define vget_low_s64(a) __builtin_mpl_vector_get_low_v2i64(a)
#define vget_low_u8(a) __builtin_mpl_vector_get_low_v16u8(a)
#define vget_low_u16(a) __builtin_mpl_vector_get_low_v8u16(a)
#define vget_low_u32(a) __builtin_mpl_vector_get_low_v4u32(a)
#define vget_low_u64(a) __builtin_mpl_vector_get_low_v2u64(a)
#define vget_low_f16(a) __builtin_mpl_vector_get_low_v4f16(a)
#define vget_low_f32(a) __builtin_mpl_vector_get_low_v2f32(a)
#define vget_low_f64(a) __builtin_mpl_vector_get_low_v1f64(a)

// vld1
#define vld1_f16(a) __builtin_mpl_vector_load_v4f16(a)
#define vld1_f32(a) __builtin_mpl_vector_load_v2f32(a)
#define vld1_f64(a) __builtin_mpl_vector_load_v1f64(a)
#define vld1q_f16(a) __builtin_mpl_vector_load_v8f16(a)
#define vld1q_f32(a) __builtin_mpl_vector_load_v4f32(a)
#define vld1q_f64(a) __builtin_mpl_vector_load_v2f64(a)

// vmlal
#define vmlal_s8(acc, a, b)  __builtin_mpl_vector_madd_v8i8(acc, a, b)
#define vmlal_s16(acc, a, b) __builtin_mpl_vector_madd_v4i16(acc, a, b)
#define vmlal_s32(acc, a, b) __builtin_mpl_vector_madd_v2i32(acc, a, b)
#define vmlal_u8(acc, a, b) __builtin_mpl_vector_madd_v8u8(acc, a, b)
#define vmlal_u16(acc, a, b) __builtin_mpl_vector_madd_v4u16(acc, a, b)
#define vmlal_u32(acc, a, b) __builtin_mpl_vector_madd_v2u32(acc, a, b)

// vmovl
#define vmovl_s32(a) __builtin_mpl_vector_widen_low_v2i32(a)
#define vmovl_s16(a) __builtin_mpl_vector_widen_low_v4i16(a)
#define vmovl_s8(a) __builtin_mpl_vector_widen_low_v8i8(a)
#define vmovl_u32(a) __builtin_mpl_vector_widen_low_v2u32(a)
#define vmovl_u16(a) __builtin_mpl_vector_widen_low_v4u16(a)
#define vmovl_u8(a) __builtin_mpl_vector_widen_low_v8u8(a)

// vmovl_high
#define vmovl_high_s32(a) __builtin_mpl_vector_widen_high_v2i32(a)
#define vmovl_high_s16(a) __builtin_mpl_vector_widen_high_v4i16(a)
#define vmovl_high_s8(a) __builtin_mpl_vector_widen_high_v8i8(a)
#define vmovl_high_u32(a) __builtin_mpl_vector_widen_high_v2u32(a)
#define vmovl_high_u16(a) __builtin_mpl_vector_widen_high_v4u16(a)
#define vmovl_high_u8(a) __builtin_mpl_vector_widen_high_v8u8(a)

// vmovn
#define vmovn_s64(a) __builtin_mpl_vector_narrow_low_v2i64(a)
#define vmovn_s32(a) __builtin_mpl_vector_narrow_low_v4i32(a)
#define vmovn_s16(a) __builtin_mpl_vector_narrow_low_v8i16(a)
#define vmovn_u64(a) __builtin_mpl_vector_narrow_low_v2u64(a)
#define vmovn_u32(a) __builtin_mpl_vector_narrow_low_v4u32(a)
#define vmovn_u16(a) __builtin_mpl_vector_narrow_low_v8u16(a)

// vmovn_high
#define vmovn_high_s64(a, b) __builtin_mpl_vector_narrow_high_v2i64(a, b)
#define vmovn_high_s32(a, b) __builtin_mpl_vector_narrow_high_v4i32(a, b)
#define vmovn_high_s16(a, b) __builtin_mpl_vector_narrow_high_v8i16(a, b)
#define vmovn_high_u64(a, b) __builtin_mpl_vector_narrow_high_v2u64(a, b)
#define vmovn_high_u32(a, b) __builtin_mpl_vector_narrow_high_v4u32(a, b)
#define vmovn_high_u16(a, b) __builtin_mpl_vector_narrow_high_v8u16(a, b)

// vmul
#define vmul_s8(a, b) (a * b)
#define vmulq_s8(a, b) (a * b)
#define vmul_s16(a, b) (a * b)
#define vmulq_s16(a, b) (a * b)
#define vmul_s32(a, b) (a * b)
#define vmulq_s32(a, b) (a * b)
#define vmul_u8(a, b) (a * b)
#define vmulq_u8(a, b) (a * b)
#define vmul_u16(a, b) (a * b)
#define vmulq_u16(a, b) (a * b)
#define vmul_u32(a, b) (a * b)
#define vmulq_u32(a, b) (a * b)

// vmull
#define vmull_s8(a, b)  __builtin_mpl_vector_mull_low_v8i8(a, b)
#define vmull_s16(a, b) __builtin_mpl_vector_mull_low_v4i16(a, b)
#define vmull_s32(a, b) __builtin_mpl_vector_mull_low_v2i32(a, b)
#define vmull_u8(a, b) __builtin_mpl_vector_mull_low_v8u8(a, b)
#define vmull_u16(a, b) __builtin_mpl_vector_mull_low_v4u16(a, b)
#define vmull_u32(a, b) __builtin_mpl_vector_mull_low_v2u32(a, b)

// vmull_high
#define vmull_high_s8(a, b)  __builtin_mpl_vector_mull_high_v8i8(a, b)
#define vmull_high_s16(a, b) __builtin_mpl_vector_mull_high_v4i16(a, b)
#define vmull_high_s32(a, b) __builtin_mpl_vector_mull_high_v2i32(a, b)
#define vmull_high_u8(a, b) __builtin_mpl_vector_mull_high_v8u8(a, b)
#define vmull_high_u16(a, b) __builtin_mpl_vector_mull_high_v4u16(a, b)
#define vmull_high_u32(a, b) __builtin_mpl_vector_mull_high_v2u32(a, b)

// vor
#define vor_s8(a, b) (a | b)
#define vor_s16(a, b) (a | b)
#define vor_s32(a, b) (a | b)
#define vor_s64(a, b) (a | b)
#define vor_u8(a, b) (a | b)
#define vor_u16(a, b) (a | b)
#define vor_u32(a, b) (a | b)
#define vor_u64(a, b) (a | b)
#define vorq_s8(a, b) (a | b)
#define vorq_s16(a, b) (a | b)
#define vorq_s32(a, b) (a | b)
#define vorq_s64(a, b) (a | b)
#define vorq_u8(a, b) (a | b)
#define vorq_u16(a, b) (a | b)
#define vorq_u32(a, b) (a | b)
#define vorq_u64(a, b) (a | b)

// vpadal (add and accumulate long pairwise)
#define vpadal_s8(a, b) __builtin_mpl_vector_pairwise_adalp_v8i8(a, b)
#define vpadal_s16(a, b) __builtin_mpl_vector_pairwise_adalp_v4i16(a, b)
#define vpadal_s32(a, b) __builtin_mpl_vector_pairwise_adalp_v2i32(a, b)
#define vpadal_u8(a, b) __builtin_mpl_vector_pairwise_adalp_v8u8(a, b)
#define vpadal_u16(a, b) __builtin_mpl_vector_pairwise_adalp_v4u16(a, b)
#define vpadal_u32(a, b) __builtin_mpl_vector_pairwise_adalp_v2u32(a, b)
#define vpadalq_s8(a, b) __builtin_mpl_vector_pairwise_adalp_v16i8(a, b)
#define vpadalq_s16(a, b) __builtin_mpl_vector_pairwise_adalp_v8i16(a, b)
#define vpadalq_s32(a, b) __builtin_mpl_vector_pairwise_adalp_v4i32(a, b)
#define vpadalq_u8(a, b) __builtin_mpl_vector_pairwise_adalp_v16u8(a, b)
#define vpadalq_u16(a, b) __builtin_mpl_vector_pairwise_adalp_v8u16(a, b)
#define vpadalq_u32(a, b) __builtin_mpl_vector_pairwise_adalp_v4u32(a, b)

// vpaddl
#define vpaddl_s8(a)  __builtin_mpl_vector_pairwise_add_v8i8(a)
#define vpaddl_s16(a) __builtin_mpl_vector_pairwise_add_v4i16(a)
#define vpaddl_s32(a) __builtin_mpl_vector_pairwise_add_v2i32(a)
#define vpaddl_u8(a) __builtin_mpl_vector_pairwise_add_v8u8(a)
#define vpaddl_u16(a) __builtin_mpl_vector_pairwise_add_v4u16(a)
#define vpaddl_u32(a) __builtin_mpl_vector_pairwise_add_v2u32(a)
#define vpaddlq_s8(a)  __builtin_mpl_vector_pairwise_add_v16i8(a)
#define vpaddlq_s16(a) __builtin_mpl_vector_pairwise_add_v8i16(a)
#define vpaddlq_s32(a) __builtin_mpl_vector_pairwise_add_v4i32(a)
#define vpaddlq_u8(a) __builtin_mpl_vector_pairwise_add_v16u8(a)
#define vpaddlq_u16(a) __builtin_mpl_vector_pairwise_add_v8u16(a)
#define vpaddlq_u32(a) __builtin_mpl_vector_pairwise_add_v4u32(a)

// vreinterpret 8
#define vreinterpret_s16_s8(a) ((int16x4_t)a)
#define vreinterpret_s32_s8(a) ((int32x2_t)a)
#define vreinterpret_u8_s8(a) ((uint8x8_t)a)
#define vreinterpret_u16_s8(a) ((uint16x4_t)a)
#define vreinterpret_u32_s8(a) ((uint32x2_t)a)
#define vreinterpret_u64_s8(a) ((uint64x1_t)a)
#define vreinterpret_s64_s8(a) ((int64x1_t)a)
#define vreinterpret_s8_u8(a) ((int8x8_t)a)
#define vreinterpret_s16_u8(a) ((int16x4_t)a)
#define vreinterpret_s32_u8(a) ((int32x2_t)a)
#define vreinterpret_u16_u8(a) ((uint16x4_t)a)
#define vreinterpret_u32_u8(a) ((uint32x2_t)a)
#define vreinterpret_u64_u8(a) ((uint64x1_t)a)
#define vreinterpret_s64_u8(a) ((int64x1_t)a)
#define vreinterpret_f16_s8(a) ((float16x4_t)a)
#define vreinterpret_f32_s8(a) ((float32x2_t)a)
#define vreinterpret_f64_s8(a) ((float64x1_t)a)
#define vreinterpret_f16_u8(a) ((float16x4_t)a)
#define vreinterpret_f32_u8(a) ((float32x2_t)a)
#define vreinterpret_f64_u8(a) ((float64x1_t)a)
#define vreinterpretq_s16_s8(a) ((int16x8_t)a)
#define vreinterpretq_s32_s8(a) ((int32x4_t)a)
#define vreinterpretq_u8_s8(a) ((uint8x16_t)a)
#define vreinterpretq_u16_s8(a) ((uint16x8_t)a)
#define vreinterpretq_u32_s8(a) ((uint32x4_t)a)
#define vreinterpretq_u64_s8(a) ((uint64x2_t)a)
#define vreinterpretq_s64_s8(a) ((int64x2_t)a)
#define vreinterpretq_s8_u8(a) ((int8x16_t)a)
#define vreinterpretq_s16_u8(a) ((int16x8_t)a)
#define vreinterpretq_s32_u8(a) ((int32x4_t)a)
#define vreinterpretq_u16_u8(a) ((uint16x8_t)a)
#define vreinterpretq_u32_u8(a) ((uint32x4_t)a)
#define vreinterpretq_u64_u8(a) ((uint64x2_t)a)
#define vreinterpretq_s64_u8(a) ((int64x2_t)a)
#define vreinterpretq_f16_s8(a) ((float16x8_t)a)
#define vreinterpretq_f32_s8(a) ((float32x4_t)a)
#define vreinterpretq_f64_s8(a) ((float64x2_t)a)
#define vreinterpretq_f16_u8(a) ((float16x8_t)a)
#define vreinterpretq_f32_u8(a) ((float32x4_t)a)
#define vreinterpretq_f64_u8(a) ((float64x2_t)a)

// vreinterpret 16
#define vreinterpret_s8_s16(a) ((int8x8_t)a)
#define vreinterpret_s32_s16(a) ((int32x2_t)a)
#define vreinterpret_u8_s16(a) ((uint8x8_t)a)
#define vreinterpret_u16_s16(a) ((uint16x4_t)a)
#define vreinterpret_u32_s16(a) ((uint32x2_t)a)
#define vreinterpret_u64_s16(a) ((uint64x1_t)a)
#define vreinterpret_s64_s16(a) ((int64x1_t)a)
#define vreinterpret_s8_u16(a) ((int8x8_t)a)
#define vreinterpret_s16_u16(a) ((int16x4_t)a)
#define vreinterpret_s32_u16(a) ((int32x2_t)a)
#define vreinterpret_u8_u16(a) ((uint8x8_t)a)
#define vreinterpret_u32_u16(a) ((uint32x2_t)a)
#define vreinterpret_u64_u16(a) ((uint64x1_t)a)
#define vreinterpret_s64_u16(a) ((int64x1_t)a)
#define vreinterpret_f16_s16(a) ((float16x4_t)a)
#define vreinterpret_f32_s16(a) ((float32x2_t)a)
#define vreinterpret_f64_s16(a) ((float64x1_t)a)
#define vreinterpret_f16_u16(a) ((float16x4_t)a)
#define vreinterpret_f32_u16(a) ((float32x2_t)a)
#define vreinterpret_f64_u16(a) ((float64x1_t)a)
#define vreinterpretq_s8_s16(a) ((int8x16_t)a)
#define vreinterpretq_s32_s16(a) ((int32x4_t)a)
#define vreinterpretq_u8_s16(a) ((uint8x16_t)a)
#define vreinterpretq_u16_s16(a) ((uint16x8_t)a)
#define vreinterpretq_u32_s16(a) ((uint32x4_t)a)
#define vreinterpretq_u64_s16(a) ((uint64x2_t)a)
#define vreinterpretq_s64_s16(a) ((int64x2_t)a)
#define vreinterpretq_s8_u16(a) ((int8x16_t)a)
#define vreinterpretq_s16_u16(a) ((int16x8_t)a)
#define vreinterpretq_s32_u16(a) ((int32x4_t)a)
#define vreinterpretq_u8_u16(a) ((uint8x16_t)a)
#define vreinterpretq_u32_u16(a) ((uint32x4_t)a)
#define vreinterpretq_u64_u16(a) ((uint64x2_t)a)
#define vreinterpretq_s64_u16(a) ((int64x2_t)a)
#define vreinterpretq_f16_s16(a) ((float16x8_t)a)
#define vreinterpretq_f32_s16(a) ((float32x4_t)a)
#define vreinterpretq_f64_s16(a) ((float64x2_t)a)
#define vreinterpretq_f16_u16(a) ((float16x8_t)a)
#define vreinterpretq_f32_u16(a) ((float32x4_t)a)
#define vreinterpretq_f64_u16(a) ((float64x2_t)a)

// vreinterpret 32

#define vreinterpret_s8_s32(a) ((int8x8_t)a)
#define vreinterpret_s16_s32(a) ((int16x4_t)a)
#define vreinterpret_u8_s32(a) ((uint8x8_t)a)
#define vreinterpret_u16_s32(a) ((uint16x4_t)a)
#define vreinterpret_u32_s32(a) ((uint32x2_t)a)
#define vreinterpret_u64_s32(a) ((uint64x1_t)a)
#define vreinterpret_s64_s32(a) ((int64x1_t)a)
#define vreinterpret_s8_u32(a) ((int8x8_t)a)
#define vreinterpret_s16_u32(a) ((int16x4_t)a)
#define vreinterpret_s32_u32(a) ((int32x2_t)a)
#define vreinterpret_u8_u32(a) ((uint8x8_t)a)
#define vreinterpret_u16_u32(a) ((uint16x4_t)a)
#define vreinterpret_u64_u32(a) ((uint64x1_t)a)
#define vreinterpret_s64_u32(a) ((int64x1_t)a)
#define vreinterpret_f16_s32(a) ((float16x4_t)a)
#define vreinterpret_f32_s32(a) ((float32x2_t)a)
#define vreinterpret_f64_s32(a) ((float64x1_t)a)
#define vreinterpret_f16_u32(a) ((float16x4_t)a)
#define vreinterpret_f32_u32(a) ((float32x2_t)a)
#define vreinterpret_f64_u32(a) ((float64x1_t)a)
#define vreinterpretq_s8_s32(a) ((int8x16_t)a)
#define vreinterpretq_s16_s32(a) ((int16x8_t)a)
#define vreinterpretq_u8_s32(a) ((uint8x16_t)a)
#define vreinterpretq_u16_s32(a) ((uint16x8_t)a)
#define vreinterpretq_u32_s32(a) ((uint32x4_t)a)
#define vreinterpretq_u64_s32(a) ((uint64x2_t)a)
#define vreinterpretq_s64_s32(a) ((int64x2_t)a)
#define vreinterpretq_s8_u32(a) ((int8x16_t)a)
#define vreinterpretq_s16_u32(a) ((int16x8_t)a)
#define vreinterpretq_s32_u32(a) ((int32x4_t)a)
#define vreinterpretq_u8_u32(a) ((uint8x16_t)a)
#define vreinterpretq_u16_u32(a) ((uint16x8_t)a)
#define vreinterpretq_u64_u32(a) ((uint64x2_t)a)
#define vreinterpretq_s64_u32(a) ((int64x2_t)a)
#define vreinterpretq_f16_s32(a) ((float16x8_t)a)
#define vreinterpretq_f32_s32(a) ((float32x4_t)a)
#define vreinterpretq_f64_s32(a) ((float64x2_t)a)
#define vreinterpretq_f16_u32(a) ((float16x8_t)a)
#define vreinterpretq_f32_u32(a) ((float32x4_t)a)
#define vreinterpretq_f64_u32(a) ((float64x2_t)a)

// vreinterpret 64
#define vreinterpret_s8_s64(a) ((int8x8_t)a)
#define vreinterpret_s16_s64(a) ((int16x4_t)a)
#define vreinterpret_s32_s64(a) ((int32x2_t)a)
#define vreinterpret_u8_s64(a) ((uint8x8_t)a)
#define vreinterpret_u16_s64(a) ((uint16x4_t)a)
#define vreinterpret_u32_s64(a) ((uint32x2_t)a)
#define vreinterpret_u64_s64(a) ((uint64x1_t)a)
#define vreinterpret_s8_u64(a) ((int8x8_t)a)
#define vreinterpret_s16_u64(a) ((int16x4_t)a)
#define vreinterpret_s32_u64(a) ((int32x2_t)a)
#define vreinterpret_u8_u64(a) ((uint8x8_t)a)
#define vreinterpret_u16_u64(a) ((uint16x4_t)a)
#define vreinterpret_u32_u64(a) ((uint32x2_t)a)
#define vreinterpret_s64_u64(a) ((int64x1_t)a)
#define vreinterpret_f16_s64(a) ((float16x4_t)a)
#define vreinterpret_f32_s64(a) ((float32x2_t)a)
#define vreinterpret_f64_s64(a) ((float64x1_t)a)
#define vreinterpret_f16_u64(a) ((float16x4_t)a)
#define vreinterpret_f32_u64(a) ((float32x2_t)a)
#define vreinterpret_f64_u64(a) ((float64x1_t)a)
#define vreinterpretq_s8_s64(a) ((int8x16_t)a)
#define vreinterpretq_s16_s64(a) ((int16x8_t)a)
#define vreinterpretq_s32_s64(a) ((int32x4_t)a)
#define vreinterpretq_u8_s64(a) ((uint8x16_t)a)
#define vreinterpretq_u16_s64(a) ((uint16x8_t)a)
#define vreinterpretq_u32_s64(a) ((uint32x4_t)a)
#define vreinterpretq_u64_s64(a) ((uint64x2_t)a)
#define vreinterpretq_s8_u64(a) ((int8x16_t)a)
#define vreinterpretq_s16_u64(a) ((int16x8_t)a)
#define vreinterpretq_s32_u64(a) ((int32x4_t)a)
#define vreinterpretq_u8_u64(a) ((uint8x16_t)a)
#define vreinterpretq_u16_u64(a) ((uint16x8_t)a)
#define vreinterpretq_u32_u64(a) ((uint32x4_t)a)
#define vreinterpretq_s64_u64(a) ((int64x2_t)a)
#define vreinterpretq_f16_s64(a) ((float16x8_t)a)
#define vreinterpretq_f32_s64(a) ((float32x4_t)a)
#define vreinterpretq_f64_s64(a) ((float64x2_t)a)
#define vreinterpretq_f16_u64(a) ((float16x8_t)a)
#define vreinterpretq_f32_u64(a) ((float32x4_t)a)
#define vreinterpretq_f64_u64(a) ((float64x2_t)a)

// vrev32
#define vrev32_s8(a) __builtin_mpl_vector_reverse_v8i8(a)
#define vrev32_s16(a) __builtin_mpl_vector_reverse_v4i16(a)
#define vrev32_u8(a) __builtin_mpl_vector_reverse_v8u8(a)
#define vrev32_u16(a) __builtin_mpl_vector_reverse_v4u16(a)
#define vrev32q_s8(a) __builtin_mpl_vector_reverse_v16i8(a)
#define vrev32q_s16(a) __builtin_mpl_vector_reverse_v8i16(a)
#define vrev32q_u8(a) __builtin_mpl_vector_reverse_v16u8(a)
#define vrev32q_u16(a) __builtin_mpl_vector_reverse_v8u16(a)

// vset_lane
#define vset_lane_s8(v, a, n)  __builtin_mpl_vector_set_element_v8i8(v, a, n)
#define vset_lane_s16(v, a, n) __builtin_mpl_vector_set_element_v4i16(v, a, n)
#define vset_lane_s32(v, a, n) __builtin_mpl_vector_set_element_v2i32(v, a, n)
#define vset_lane_s64(v, a, n) __builtin_mpl_vector_set_element_v1i64(v, a, n)
#define vset_lane_u8(v, a, n) __builtin_mpl_vector_set_element_v8u8(v, a, n)
#define vset_lane_u16(v, a, n) __builtin_mpl_vector_set_element_v4u16(v, a, n)
#define vset_lane_u32(v, a, n) __builtin_mpl_vector_set_element_v2u32(v, a, n)
#define vset_lane_u64(v, a, n) __builtin_mpl_vector_set_element_v1u64(v, a, n)
#define vset_lane_f16(v, a, n) __builtin_mpl_vector_set_element_v4f16(v, a, n)
#define vset_lane_f32(v, a, n) __builtin_mpl_vector_set_element_v2f32(v, a, n)
#define vset_lane_f64(v, a, n) __builtin_mpl_vector_set_element_v1f64(v, a, n)
#define vsetq_lane_s8(v, a, n) __builtin_mpl_vector_set_element_v16i8(v, a, n)
#define vsetq_lane_s16(v, a, n) __builtin_mpl_vector_set_element_v8i16(v, a, n)
#define vsetq_lane_s32(v, a, n) __builtin_mpl_vector_set_element_v4i32(v, a, n)
#define vsetq_lane_s64(v, a, n) __builtin_mpl_vector_set_element_v2i64(v, a, n)
#define vsetq_lane_u8(v, a, n) __builtin_mpl_vector_set_element_v16u8(v, a, n)
#define vsetq_lane_u16(v, a, n) __builtin_mpl_vector_set_element_v8u16(v, a, n)
#define vsetq_lane_u32(v, a, n) __builtin_mpl_vector_set_element_v4u32(v, a, n)
#define vsetq_lane_u64(v, a, n) __builtin_mpl_vector_set_element_v2u64(v, a, n)
#define vsetq_lane_f16(v, a, n) __builtin_mpl_vector_set_element_v8f16(v, a, n)
#define vsetq_lane_f32(v, a, n) __builtin_mpl_vector_set_element_v4f32(v, a, n)
#define vsetq_lane_f64(v, a, n) __builtin_mpl_vector_set_element_v2f64(v, a, n)

// vshl
#define vshl_s8(a, b) (a << b)
#define vshl_s16(a, b) (a << b)
#define vshl_s32(a, b) (a << b)
#define vshl_s64(a, b) (a << b)
#define vshl_u8(a, b) (a << b)
#define vshl_u16(a, b) (a << b)
#define vshl_u32(a, b) (a << b)
#define vshl_u64(a, b) (a << b)
#define vshlq_s8(a, b) (a << b)
#define vshlq_s16(a, b) (a << b)
#define vshlq_s32(a, b) (a << b)
#define vshlq_s64(a, b) (a << b)
#define vshlq_u8(a, b) (a << b)
#define vshlq_u16(a, b) (a << b)
#define vshlq_u32(a, b) (a << b)
#define vshlq_u64(a, b) (a << b)
#define vshld_s64(a, b) (a << b)
#define vshld_u64(a, b) (a << b)

// vshl_n
#define vshlq_n_s64(a, n) __builtin_mpl_vector_shli_v2i64(a, n)
#define vshlq_n_s32(a, n) __builtin_mpl_vector_shli_v4i32(a, n)
#define vshlq_n_s16(a, n) __builtin_mpl_vector_shli_v8i16(a, n)
#define vshlq_n_s8(a, n) __builtin_mpl_vector_shli_v16i8(a, n)
#define vshlq_n_u64(a, n) __builtin_mpl_vector_shli_v2u64(a, n)
#define vshlq_n_u32(a, n) __builtin_mpl_vector_shli_v4u32(a, n)
#define vshlq_n_u16(a, n) __builtin_mpl_vector_shli_v8u16(a, n)
#define vshlq_n_u8(a, n) __builtin_mpl_vector_shli_v16u8(a, n)
#define vshl_n_s64(a, n) __builtin_mpl_vector_shli_v1i64(a, n)
#define vshl_n_s32(a, n) __builtin_mpl_vector_shli_v2i32(a, n)
#define vshl_n_s16(a, n) __builtin_mpl_vector_shli_v4i16(a, n)
#define vshl_n_s8(a, n) __builtin_mpl_vector_shli_v8i8(a, n)
#define vshl_n_u64(a, n) __builtin_mpl_vector_shli_v1u64(a, n)
#define vshl_n_u32(a, n) __builtin_mpl_vector_shli_v2u32(a, n)
#define vshl_n_u16(a, n) __builtin_mpl_vector_shli_v4u16(a, n)
#define vshl_n_u8(a, n) __builtin_mpl_vector_shli_v8u8(a, n)
#define vshld_n_s64(a, n) (vget_lane_s64(vshl_n_s64(vdup_n_s64(a), n), 0))
#define vshld_n_u64(a, n) (vget_lane_u64(vshl_n_u64(vdup_n_u64(a), n), 0))

// vshr
#define vshr_s8(a, b) (a >> b)
#define vshr_s16(a, b) (a >> b)
#define vshr_s32(a, b) (a >> b)
#define vshr_s64(a, b) (a >> b)
#define vshr_u8(a, b) (a >> b)
#define vshr_u16(a, b) (a >> b)
#define vshr_u32(a, b) (a >> b)
#define vshr_u64(a, b) (a >> b)
#define vshrq_s8(a, b) (a >> b)
#define vshrq_s16(a, b) (a >> b)
#define vshrq_s32(a, b) (a >> b)
#define vshrq_s64(a, b) (a >> b)
#define vshrq_u8(a, b) (a >> b)
#define vshrq_u16(a, b) (a >> b)
#define vshrq_u32(a, b) (a >> b)
#define vshrq_u64(a, b) (a >> b)
#define vshrd_s64(a, b) (vget_lane_s64((vdup_n_s64(a) >> vdup_n_s64(b)), 0))
#define vshrd_u64(a, b) (vget_lane_u64((vdup_n_u64(a) >> vdup_n_u64(b)), 0))

// vshr_n
#define vshrq_n_s64(a, n) __builtin_mpl_vector_shri_v2i64(a, n)
#define vshrq_n_s32(a, n) __builtin_mpl_vector_shri_v4i32(a, n)
#define vshrq_n_s16(a, n) __builtin_mpl_vector_shri_v8i16(a, n)
#define vshrq_n_s8(a, n) __builtin_mpl_vector_shri_v16i8(a, n)
#define vshrq_n_u64(a, n) __builtin_mpl_vector_shru_v2u64(a, n)
#define vshrq_n_u32(a, n) __builtin_mpl_vector_shru_v4u32(a, n)
#define vshrq_n_u16(a, n) __builtin_mpl_vector_shru_v8u16(a, n)
#define vshrq_n_u8(a, n) __builtin_mpl_vector_shru_v16u8(a, n)
#define vshr_n_s64(a, n) __builtin_mpl_vector_shri_v1i64(a, n)
#define vshr_n_s32(a, n) __builtin_mpl_vector_shri_v2i32(a, n)
#define vshr_n_s16(a, n) __builtin_mpl_vector_shri_v4i16(a, n)
#define vshr_n_s8(a, n) __builtin_mpl_vector_shri_v8i8(a, n)
#define vshr_n_u64(a, n) __builtin_mpl_vector_shru_v1u64(a, n)
#define vshr_n_u32(a, n) __builtin_mpl_vector_shru_v2u32(a, n)
#define vshr_n_u16(a, n) __builtin_mpl_vector_shru_v4u16(a, n)
#define vshr_n_u8(a, n) __builtin_mpl_vector_shru_v8u8(a, n)
#define vshrd_n_s64(a, n) (vget_lane_s64(vshr_n_s64(vdup_n_s64(a), n), 0))
#define vshrd_n_u64(a, n) (vget_lane_u64(vshr_n_u64(vdup_n_u64(a), n), 0))

// vshrn_n
#define vshrn_n_s16(a, n) __builtin_mpl_vector_shr_narrow_low_v8i16(a, n)
#define vshrn_n_s32(a, n) __builtin_mpl_vector_shr_narrow_low_v4i32(a, n)
#define vshrn_n_s64(a, n) __builtin_mpl_vector_shr_narrow_low_v2i64(a, n)
#define vshrn_n_u16(a, n) __builtin_mpl_vector_shr_narrow_low_v8u16(a, n)
#define vshrn_n_u32(a, n) __builtin_mpl_vector_shr_narrow_low_v4u32(a, n)
#define vshrn_n_u64(a, n) __builtin_mpl_vector_shr_narrow_low_v2u64(a, n)

// vst1
#define vst1_s8(p, v)  __builtin_mpl_vector_store_v8i8(p, v)
#define vst1_s16(p, v) __builtin_mpl_vector_store_v4i16(p, v)
#define vst1_s32(p, v) __builtin_mpl_vector_store_v2i32(p, v)
#define vst1_s64(p, v) __builtin_mpl_vector_store_v1i64(p, v)
#define vst1_u8(p, v) __builtin_mpl_vector_store_v8u8(p, v)
#define vst1_u16(p, v) __builtin_mpl_vector_store_v4u16(p, v)
#define vst1_u32(p, v) __builtin_mpl_vector_store_v2u32(p, v)
#define vst1_u64(p, v) __builtin_mpl_vector_store_v1u64(p, v)
#define vst1_f16(p, v) __builtin_mpl_vector_store_v4f16(p, v)
#define vst1_f32(p, v) __builtin_mpl_vector_store_v2f32(p, v)
#define vst1_f64(p, v) __builtin_mpl_vector_store_v1f64(p, v)
#define vst1q_s8(p, v) __builtin_mpl_vector_store_v16i8(p, v)
#define vst1q_s16(p, v) __builtin_mpl_vector_store_v8i16(p, v)
#define vst1q_s32(p, v) __builtin_mpl_vector_store_v4i32(p, v)
#define vst1q_s64(p, v) __builtin_mpl_vector_store_v2i64(p, v)
#define vst1q_u8(p, v) __builtin_mpl_vector_store_v16u8(p, v)
#define vst1q_u16(p, v) __builtin_mpl_vector_store_v8u16(p, v)
#define vst1q_u32(p, v) __builtin_mpl_vector_store_v4u32(p, v)
#define vst1q_u64(p, v) __builtin_mpl_vector_store_v2u64(p, v)
#define vst1q_f16(p, v) __builtin_mpl_vector_store_v8f16(p, v)
#define vst1q_f32(p, v) __builtin_mpl_vector_store_v4f32(p, v)
#define vst1q_f64(p, v) __builtin_mpl_vector_store_v2f64(p, v)

// vsub
#define vsub_s8(a, b) (a - b)
#define vsub_s16(a, b) (a - b)
#define vsub_s32(a, b) (a - b)
#define vsub_s64(a, b) (a - b)
#define vsub_u8(a, b) (a - b)
#define vsub_u16(a, b) (a - b)
#define vsub_u32(a, b) (a - b)
#define vsub_u64(a, b) (a - b)
#define vsub_f16(a, b) (a - b)
#define vsub_f32(a, b) (a - b)
#define vsub_f64(a, b) (a - b)
#define vsubq_s8(a, b) (a - b)
#define vsubq_s16(a, b) (a - b)
#define vsubq_s32(a, b) (a - b)
#define vsubq_s64(a, b) (a - b)
#define vsubq_u8(a, b) (a - b)
#define vsubq_u16(a, b) (a - b)
#define vsubq_u32(a, b) (a - b)
#define vsubq_u64(a, b) (a - b)
#define vsubq_f16(a, b) (a - b)
#define vsubq_f32(a, b) (a - b)
#define vsubq_f64(a, b) (a - b)
#define vsubd_s64(a, b) (a - b)
#define vsubd_u64(a, b) (a - b)

// vsub[lw]
#define vsubl_s8(a, b) __builtin_mpl_vector_subl_low_v8i8(a, b)
#define vsubl_s16(a, b) __builtin_mpl_vector_subl_low_v4i16(a, b)
#define vsubl_s32(a, b) __builtin_mpl_vector_subl_low_v2i32(a, b)
#define vsubl_u8(a, b) __builtin_mpl_vector_subl_low_v8u8(a, b)
#define vsubl_u16(a, b) __builtin_mpl_vector_subl_low_v4u16(a, b)
#define vsubl_u32(a, b) __builtin_mpl_vector_subl_low_v2u32(a, b)
#define vsubl_high_s8(a, b) __builtin_mpl_vector_subl_high_v8i8(a, b)
#define vsubl_high_s16(a, b) __builtin_mpl_vector_subl_high_v4i16(a, b)
#define vsubl_high_s32(a, b) __builtin_mpl_vector_subl_high_v2i32(a, b)
#define vsubl_high_u8(a, b) __builtin_mpl_vector_subl_high_v8u8(a, b)
#define vsubl_high_u16(a, b) __builtin_mpl_vector_subl_high_v4u16(a, b)
#define vsubl_high_u32(a, b) __builtin_mpl_vector_subl_high_v2u32(a, b)
#define vsubw_s8(a, b) __builtin_mpl_vector_subw_low_v8i8(a, b)
#define vsubw_s16(a, b) __builtin_mpl_vector_subw_low_v4i16(a, b)
#define vsubw_s32(a, b) __builtin_mpl_vector_subw_low_v2i32(a, b)
#define vsubw_u8(a, b) __builtin_mpl_vector_subw_low_v8u8(a, b)
#define vsubw_u16(a, b) __builtin_mpl_vector_subw_low_v4u16(a, b)
#define vsubw_u32(a, b) __builtin_mpl_vector_subw_low_v2u32(a, b)
#define vsubw_high_s8(a, b) __builtin_mpl_vector_subw_high_v8i8(a, b)
#define vsubw_high_s16(a, b) __builtin_mpl_vector_subw_high_v4i16(a, b)
#define vsubw_high_s32(a, b) __builtin_mpl_vector_subw_high_v2i32(a, b)
#define vsubw_high_u8(a, b) __builtin_mpl_vector_subw_high_v8u8(a, b)
#define vsubw_high_u16(a, b) __builtin_mpl_vector_subw_high_v4u16(a, b)
#define vsubw_high_u32(a, b) __builtin_mpl_vector_subw_high_v2u32(a, b)

uint8_t __builtin_mpl_vector_get_lane_v8u8(uint8x8_t a, const int lane);
#define vget_lane_u8(a, lane)  __builtin_mpl_vector_get_lane_v8u8(a, lane)

uint16_t __builtin_mpl_vector_get_lane_v4u16(uint16x4_t a, const int lane);
#define vget_lane_u16(a, lane)  __builtin_mpl_vector_get_lane_v4u16(a, lane)

uint32_t __builtin_mpl_vector_get_lane_v2u32(uint32x2_t a, const int lane);
#define vget_lane_u32(a, lane)  __builtin_mpl_vector_get_lane_v2u32(a, lane)

uint64_t __builtin_mpl_vector_get_lane_v1u64(uint64x1_t a, const int lane);
#define vget_lane_u64(a, lane)  __builtin_mpl_vector_get_lane_v1u64(a, lane)

int8_t __builtin_mpl_vector_get_lane_v8i8(int8x8_t a, const int lane);
#define vget_lane_s8(a, lane)  __builtin_mpl_vector_get_lane_v8i8(a, lane)

int16_t __builtin_mpl_vector_get_lane_v4i16(int16x4_t a, const int lane);
#define vget_lane_s16(a, lane)  __builtin_mpl_vector_get_lane_v4i16(a, lane)

int32_t __builtin_mpl_vector_get_lane_v2i32(int32x2_t a, const int lane);
#define vget_lane_s32(a, lane)  __builtin_mpl_vector_get_lane_v2i32(a, lane)

int64_t __builtin_mpl_vector_get_lane_v1i64(int64x1_t a, const int lane);
#define vget_lane_s64(a, lane)  __builtin_mpl_vector_get_lane_v1i64(a, lane)

uint8_t __builtin_mpl_vector_getq_lane_v16u8(uint8x16_t a, const int lane);
#define vgetq_lane_u8(a, lane)  __builtin_mpl_vector_getq_lane_v16u8(a, lane)

uint16_t __builtin_mpl_vector_getq_lane_v8u16(uint16x8_t a, const int lane);
#define vgetq_lane_u16(a, lane)  __builtin_mpl_vector_getq_lane_v8u16(a, lane)

uint32_t __builtin_mpl_vector_getq_lane_v4u32(uint32x4_t a, const int lane);
#define vgetq_lane_u32(a, lane)  __builtin_mpl_vector_getq_lane_v4u32(a, lane)

uint64_t __builtin_mpl_vector_getq_lane_v2u64(uint64x2_t a, const int lane);
#define vgetq_lane_u64(a, lane)  __builtin_mpl_vector_getq_lane_v2u64(a, lane)

int8_t __builtin_mpl_vector_getq_lane_v16i8(int8x16_t a, const int lane);
#define vgetq_lane_s8(a, lane)  __builtin_mpl_vector_getq_lane_v16i8(a, lane)

int16_t __builtin_mpl_vector_getq_lane_v8i16(int16x8_t a, const int lane);
#define vgetq_lane_s16(a, lane)  __builtin_mpl_vector_getq_lane_v8i16(a, lane)

int32_t __builtin_mpl_vector_getq_lane_v4i32(int32x4_t a, const int lane);
#define vgetq_lane_s32(a, lane)  __builtin_mpl_vector_getq_lane_v4i32(a, lane)

int64_t __builtin_mpl_vector_getq_lane_v2i64(int64x2_t a, const int lane);
#define vgetq_lane_s64(a, lane)  __builtin_mpl_vector_getq_lane_v2i64(a, lane)

int8x8_t __builtin_mpl_vector_abd_v8i8(int8x8_t a, int8x8_t b);
#define vabd_s8(a, b)  __builtin_mpl_vector_abd_v8i8(a, b)

int8x16_t __builtin_mpl_vector_abdq_v16i8(int8x16_t a, int8x16_t b);
#define vabdq_s8(a, b)  __builtin_mpl_vector_abdq_v16i8(a, b)

int16x4_t __builtin_mpl_vector_abd_v4i16(int16x4_t a, int16x4_t b);
#define vabd_s16(a, b)  __builtin_mpl_vector_abd_v4i16(a, b)

int16x8_t __builtin_mpl_vector_abdq_v8i16(int16x8_t a, int16x8_t b);
#define vabdq_s16(a, b)  __builtin_mpl_vector_abdq_v8i16(a, b)

int32x2_t __builtin_mpl_vector_abd_v2i32(int32x2_t a, int32x2_t b);
#define vabd_s32(a, b)  __builtin_mpl_vector_abd_v2i32(a, b)

int32x4_t __builtin_mpl_vector_abdq_v4i32(int32x4_t a, int32x4_t b);
#define vabdq_s32(a, b)  __builtin_mpl_vector_abdq_v4i32(a, b)

uint8x8_t __builtin_mpl_vector_abd_v8u8(uint8x8_t a, uint8x8_t b);
#define vabd_u8(a, b)  __builtin_mpl_vector_abd_v8u8(a, b)

uint8x16_t __builtin_mpl_vector_abdq_v16u8(uint8x16_t a, uint8x16_t b);
#define vabdq_u8(a, b)  __builtin_mpl_vector_abdq_v16u8(a, b)

uint16x4_t __builtin_mpl_vector_abd_v4u16(uint16x4_t a, uint16x4_t b);
#define vabd_u16(a, b)  __builtin_mpl_vector_abd_v4u16(a, b)

uint16x8_t __builtin_mpl_vector_abdq_v8u16(uint16x8_t a, uint16x8_t b);
#define vabdq_u16(a, b)  __builtin_mpl_vector_abdq_v8u16(a, b)

uint32x2_t __builtin_mpl_vector_abd_v2u32(uint32x2_t a, uint32x2_t b);
#define vabd_u32(a, b)  __builtin_mpl_vector_abd_v2u32(a, b)

uint32x4_t __builtin_mpl_vector_abdq_v4u32(uint32x4_t a, uint32x4_t b);
#define vabdq_u32(a, b)  __builtin_mpl_vector_abdq_v4u32(a, b)

int8x8_t __builtin_mpl_vector_max_v8i8(int8x8_t a, int8x8_t b);
#define vmax_s8(a, b)  __builtin_mpl_vector_max_v8i8(a, b)

int8x16_t __builtin_mpl_vector_maxq_v16i8(int8x16_t a, int8x16_t b);
#define vmaxq_s8(a, b)  __builtin_mpl_vector_maxq_v16i8(a, b)

int16x4_t __builtin_mpl_vector_max_v4i16(int16x4_t a, int16x4_t b);
#define vmax_s16(a, b)  __builtin_mpl_vector_max_v4i16(a, b)

int16x8_t __builtin_mpl_vector_maxq_v8i16(int16x8_t a, int16x8_t b);
#define vmaxq_s16(a, b)  __builtin_mpl_vector_maxq_v8i16(a, b)

int32x2_t __builtin_mpl_vector_max_v2i32(int32x2_t a, int32x2_t b);
#define vmax_s32(a, b)  __builtin_mpl_vector_max_v2i32(a, b)

int32x4_t __builtin_mpl_vector_maxq_v4i32(int32x4_t a, int32x4_t b);
#define vmaxq_s32(a, b)  __builtin_mpl_vector_maxq_v4i32(a, b)

uint8x8_t __builtin_mpl_vector_max_v8u8(uint8x8_t a, uint8x8_t b);
#define vmax_u8(a, b)  __builtin_mpl_vector_max_v8u8(a, b)

uint8x16_t __builtin_mpl_vector_maxq_v16u8(uint8x16_t a, uint8x16_t b);
#define vmaxq_u8(a, b)  __builtin_mpl_vector_maxq_v16u8(a, b)

uint16x4_t __builtin_mpl_vector_max_v4u16(uint16x4_t a, uint16x4_t b);
#define vmax_u16(a, b)  __builtin_mpl_vector_max_v4u16(a, b)

uint16x8_t __builtin_mpl_vector_maxq_v8u16(uint16x8_t a, uint16x8_t b);
#define vmaxq_u16(a, b)  __builtin_mpl_vector_maxq_v8u16(a, b)

uint32x2_t __builtin_mpl_vector_max_v2u32(uint32x2_t a, uint32x2_t b);
#define vmax_u32(a, b)  __builtin_mpl_vector_max_v2u32(a, b)

uint32x4_t __builtin_mpl_vector_maxq_v4u32(uint32x4_t a, uint32x4_t b);
#define vmaxq_u32(a, b)  __builtin_mpl_vector_maxq_v4u32(a, b)

int8x8_t __builtin_mpl_vector_min_v8i8(int8x8_t a, int8x8_t b);
#define vmin_s8(a, b)  __builtin_mpl_vector_min_v8i8(a, b)

int8x16_t __builtin_mpl_vector_minq_v16i8(int8x16_t a, int8x16_t b);
#define vminq_s8(a, b)  __builtin_mpl_vector_minq_v16i8(a, b)

int16x4_t __builtin_mpl_vector_min_v4i16(int16x4_t a, int16x4_t b);
#define vmin_s16(a, b)  __builtin_mpl_vector_min_v4i16(a, b)

int16x8_t __builtin_mpl_vector_minq_v8i16(int16x8_t a, int16x8_t b);
#define vminq_s16(a, b)  __builtin_mpl_vector_minq_v8i16(a, b)

int32x2_t __builtin_mpl_vector_min_v2i32(int32x2_t a, int32x2_t b);
#define vmin_s32(a, b)  __builtin_mpl_vector_min_v2i32(a, b)

int32x4_t __builtin_mpl_vector_minq_v4i32(int32x4_t a, int32x4_t b);
#define vminq_s32(a, b)  __builtin_mpl_vector_minq_v4i32(a, b)

uint8x8_t __builtin_mpl_vector_min_v8u8(uint8x8_t a, uint8x8_t b);
#define vmin_u8(a, b)  __builtin_mpl_vector_min_v8u8(a, b)

uint8x16_t __builtin_mpl_vector_minq_v16u8(uint8x16_t a, uint8x16_t b);
#define vminq_u8(a, b)  __builtin_mpl_vector_minq_v16u8(a, b)

uint16x4_t __builtin_mpl_vector_min_v4u16(uint16x4_t a, uint16x4_t b);
#define vmin_u16(a, b)  __builtin_mpl_vector_min_v4u16(a, b)

uint16x8_t __builtin_mpl_vector_minq_v8u16(uint16x8_t a, uint16x8_t b);
#define vminq_u16(a, b)  __builtin_mpl_vector_minq_v8u16(a, b)

uint32x2_t __builtin_mpl_vector_min_v2u32(uint32x2_t a, uint32x2_t b);
#define vmin_u32(a, b)  __builtin_mpl_vector_min_v2u32(a, b)

uint32x4_t __builtin_mpl_vector_minq_v4u32(uint32x4_t a, uint32x4_t b);
#define vminq_u32(a, b)  __builtin_mpl_vector_minq_v4u32(a, b)

uint32x2_t __builtin_mpl_vector_recpe_v2u32(uint32x2_t a);
#define vrecpe_u32(a)  __builtin_mpl_vector_recpe_v2u32(a)

uint32x4_t __builtin_mpl_vector_recpeq_v4u32(uint32x4_t a);
#define vrecpeq_u32(a)  __builtin_mpl_vector_recpeq_v4u32(a)

int8x8_t __builtin_mpl_vector_padd_v8i8(int8x8_t a, int8x8_t b);
#define vpadd_s8(a, b)  __builtin_mpl_vector_padd_v8i8(a, b)

int16x4_t __builtin_mpl_vector_padd_v4i16(int16x4_t a, int16x4_t b);
#define vpadd_s16(a, b)  __builtin_mpl_vector_padd_v4i16(a, b)

int32x2_t __builtin_mpl_vector_padd_v2i32(int32x2_t a, int32x2_t b);
#define vpadd_s32(a, b)  __builtin_mpl_vector_padd_v2i32(a, b)

uint8x8_t __builtin_mpl_vector_padd_v8u8(uint8x8_t a, uint8x8_t b);
#define vpadd_u8(a, b)  __builtin_mpl_vector_padd_v8u8(a, b)

uint16x4_t __builtin_mpl_vector_padd_v4u16(uint16x4_t a, uint16x4_t b);
#define vpadd_u16(a, b)  __builtin_mpl_vector_padd_v4u16(a, b)

uint32x2_t __builtin_mpl_vector_padd_v2u32(uint32x2_t a, uint32x2_t b);
#define vpadd_u32(a, b)  __builtin_mpl_vector_padd_v2u32(a, b)

int8x16_t __builtin_mpl_vector_paddq_v16i8(int8x16_t a, int8x16_t b);
#define vpaddq_s8(a, b)  __builtin_mpl_vector_paddq_v16i8(a, b)

int16x8_t __builtin_mpl_vector_paddq_v8i16(int16x8_t a, int16x8_t b);
#define vpaddq_s16(a, b)  __builtin_mpl_vector_paddq_v8i16(a, b)

int32x4_t __builtin_mpl_vector_paddq_v4i32(int32x4_t a, int32x4_t b);
#define vpaddq_s32(a, b)  __builtin_mpl_vector_paddq_v4i32(a, b)

int64x2_t __builtin_mpl_vector_paddq_v2i64(int64x2_t a, int64x2_t b);
#define vpaddq_s64(a, b)  __builtin_mpl_vector_paddq_v2i64(a, b)

uint8x16_t __builtin_mpl_vector_paddq_v16u8(uint8x16_t a, uint8x16_t b);
#define vpaddq_u8(a, b)  __builtin_mpl_vector_paddq_v16u8(a, b)

uint16x8_t __builtin_mpl_vector_paddq_v8u16(uint16x8_t a, uint16x8_t b);
#define vpaddq_u16(a, b)  __builtin_mpl_vector_paddq_v8u16(a, b)

uint32x4_t __builtin_mpl_vector_paddq_v4u32(uint32x4_t a, uint32x4_t b);
#define vpaddq_u32(a, b)  __builtin_mpl_vector_paddq_v4u32(a, b)

uint64x2_t __builtin_mpl_vector_paddq_v2u64(uint64x2_t a, uint64x2_t b);
#define vpaddq_u64(a, b)  __builtin_mpl_vector_paddq_v2u64(a, b)

int64x1_t __builtin_mpl_vector_paddd_v2i64(int64x2_t a);
#define vpaddd_s64(a)  (vget_lane_s64(__builtin_mpl_vector_paddd_v2i64(a), 0))

uint64x1_t __builtin_mpl_vector_paddd_v2u64(uint64x2_t a);
#define vpaddd_u64(a)  (vget_lane_u64(__builtin_mpl_vector_paddd_v2u64(a), 0))

int8x8_t __builtin_mpl_vector_pmax_v8i8(int8x8_t a, int8x8_t b);
#define vpmax_s8(a, b)  __builtin_mpl_vector_pmax_v8i8(a, b)

int16x4_t __builtin_mpl_vector_pmax_v4i16(int16x4_t a, int16x4_t b);
#define vpmax_s16(a, b)  __builtin_mpl_vector_pmax_v4i16(a, b)

int32x2_t __builtin_mpl_vector_pmax_v2i32(int32x2_t a, int32x2_t b);
#define vpmax_s32(a, b)  __builtin_mpl_vector_pmax_v2i32(a, b)

uint8x8_t __builtin_mpl_vector_pmax_v8u8(uint8x8_t a, uint8x8_t b);
#define vpmax_u8(a, b)  __builtin_mpl_vector_pmax_v8u8(a, b)

uint16x4_t __builtin_mpl_vector_pmax_v4u16(uint16x4_t a, uint16x4_t b);
#define vpmax_u16(a, b)  __builtin_mpl_vector_pmax_v4u16(a, b)

uint32x2_t __builtin_mpl_vector_pmax_v2u32(uint32x2_t a, uint32x2_t b);
#define vpmax_u32(a, b)  __builtin_mpl_vector_pmax_v2u32(a, b)

int8x16_t __builtin_mpl_vector_pmaxq_v16i8(int8x16_t a, int8x16_t b);
#define vpmaxq_s8(a, b)  __builtin_mpl_vector_pmaxq_v16i8(a, b)

int16x8_t __builtin_mpl_vector_pmaxq_v8i16(int16x8_t a, int16x8_t b);
#define vpmaxq_s16(a, b)  __builtin_mpl_vector_pmaxq_v8i16(a, b)

int32x4_t __builtin_mpl_vector_pmaxq_v4i32(int32x4_t a, int32x4_t b);
#define vpmaxq_s32(a, b)  __builtin_mpl_vector_pmaxq_v4i32(a, b)

uint8x16_t __builtin_mpl_vector_pmaxq_v16u8(uint8x16_t a, uint8x16_t b);
#define vpmaxq_u8(a, b)  __builtin_mpl_vector_pmaxq_v16u8(a, b)

uint16x8_t __builtin_mpl_vector_pmaxq_v8u16(uint16x8_t a, uint16x8_t b);
#define vpmaxq_u16(a, b)  __builtin_mpl_vector_pmaxq_v8u16(a, b)

uint32x4_t __builtin_mpl_vector_pmaxq_v4u32(uint32x4_t a, uint32x4_t b);
#define vpmaxq_u32(a, b)  __builtin_mpl_vector_pmaxq_v4u32(a, b)

int8x8_t __builtin_mpl_vector_pmin_v8i8(int8x8_t a, int8x8_t b);
#define vpmin_s8(a, b)  __builtin_mpl_vector_pmin_v8i8(a, b)

int16x4_t __builtin_mpl_vector_pmin_v4i16(int16x4_t a, int16x4_t b);
#define vpmin_s16(a, b)  __builtin_mpl_vector_pmin_v4i16(a, b)

int32x2_t __builtin_mpl_vector_pmin_v2i32(int32x2_t a, int32x2_t b);
#define vpmin_s32(a, b)  __builtin_mpl_vector_pmin_v2i32(a, b)

uint8x8_t __builtin_mpl_vector_pmin_v8u8(uint8x8_t a, uint8x8_t b);
#define vpmin_u8(a, b)  __builtin_mpl_vector_pmin_v8u8(a, b)

uint16x4_t __builtin_mpl_vector_pmin_v4u16(uint16x4_t a, uint16x4_t b);
#define vpmin_u16(a, b)  __builtin_mpl_vector_pmin_v4u16(a, b)

uint32x2_t __builtin_mpl_vector_pmin_v2u32(uint32x2_t a, uint32x2_t b);
#define vpmin_u32(a, b)  __builtin_mpl_vector_pmin_v2u32(a, b)

int8x16_t __builtin_mpl_vector_pminq_v16i8(int8x16_t a, int8x16_t b);
#define vpminq_s8(a, b)  __builtin_mpl_vector_pminq_v16i8(a, b)

int16x8_t __builtin_mpl_vector_pminq_v8i16(int16x8_t a, int16x8_t b);
#define vpminq_s16(a, b)  __builtin_mpl_vector_pminq_v8i16(a, b)

int32x4_t __builtin_mpl_vector_pminq_v4i32(int32x4_t a, int32x4_t b);
#define vpminq_s32(a, b)  __builtin_mpl_vector_pminq_v4i32(a, b)

uint8x16_t __builtin_mpl_vector_pminq_v16u8(uint8x16_t a, uint8x16_t b);
#define vpminq_u8(a, b)  __builtin_mpl_vector_pminq_v16u8(a, b)

uint16x8_t __builtin_mpl_vector_pminq_v8u16(uint16x8_t a, uint16x8_t b);
#define vpminq_u16(a, b)  __builtin_mpl_vector_pminq_v8u16(a, b)

uint32x4_t __builtin_mpl_vector_pminq_v4u32(uint32x4_t a, uint32x4_t b);
#define vpminq_u32(a, b)  __builtin_mpl_vector_pminq_v4u32(a, b)

int8x8_t __builtin_mpl_vector_maxv_v8i8(int8x8_t a);
#define vmaxv_s8(a)  vget_lane_s8(__builtin_mpl_vector_maxv_v8i8(a), 0)

int8x16_t __builtin_mpl_vector_maxvq_v16i8(int8x16_t a);
#define vmaxvq_s8(a)  vgetq_lane_s8(__builtin_mpl_vector_maxvq_v16i8(a), 0)

int16x4_t __builtin_mpl_vector_maxv_v4i16(int16x4_t a);
#define vmaxv_s16(a)  vget_lane_s16(__builtin_mpl_vector_maxv_v4i16(a), 0)

int16x8_t __builtin_mpl_vector_maxvq_v8i16(int16x8_t a);
#define vmaxvq_s16(a)  vgetq_lane_s16(__builtin_mpl_vector_maxvq_v8i16(a), 0)

int32x2_t __builtin_mpl_vector_maxv_v2i32(int32x2_t a);
#define vmaxv_s32(a)  vget_lane_s32(__builtin_mpl_vector_maxv_v2i32(a), 0)

int32x4_t __builtin_mpl_vector_maxvq_v4i32(int32x4_t a);
#define vmaxvq_s32(a)  vgetq_lane_s32(__builtin_mpl_vector_maxvq_v4i32(a), 0)

uint8x8_t __builtin_mpl_vector_maxv_v8u8(uint8x8_t a);
#define vmaxv_u8(a)  vget_lane_u8(__builtin_mpl_vector_maxv_v8u8(a), 0)

uint8x16_t __builtin_mpl_vector_maxvq_v16u8(uint8x16_t a);
#define vmaxvq_u8(a)  vgetq_lane_u8(__builtin_mpl_vector_maxvq_v16u8(a), 0)

uint16x4_t __builtin_mpl_vector_maxv_v4u16(uint16x4_t a);
#define vmaxv_u16(a)  vget_lane_u16(__builtin_mpl_vector_maxv_v4u16(a), 0)

uint16x8_t __builtin_mpl_vector_maxvq_v8u16(uint16x8_t a);
#define vmaxvq_u16(a)  vgetq_lane_u16(__builtin_mpl_vector_maxvq_v8u16(a), 0)

uint32x2_t __builtin_mpl_vector_maxv_v2u32(uint32x2_t a);
#define vmaxv_u32(a)  vget_lane_u32(__builtin_mpl_vector_maxv_v2u32(a), 0)

uint32x4_t __builtin_mpl_vector_maxvq_v4u32(uint32x4_t a);
#define vmaxvq_u32(a)  vgetq_lane_u32(__builtin_mpl_vector_maxvq_v4u32(a), 0)

int8x8_t __builtin_mpl_vector_minv_v8i8(int8x8_t a);
#define vminv_s8(a)  vget_lane_s8(__builtin_mpl_vector_minv_v8i8(a), 0)

int8x16_t __builtin_mpl_vector_minvq_v16i8(int8x16_t a);
#define vminvq_s8(a)  vgetq_lane_s8(__builtin_mpl_vector_minvq_v16i8(a), 0)

int16x4_t __builtin_mpl_vector_minv_v4i16(int16x4_t a);
#define vminv_s16(a)  vget_lane_s16(__builtin_mpl_vector_minv_v4i16(a), 0)

int16x8_t __builtin_mpl_vector_minvq_v8i16(int16x8_t a);
#define vminvq_s16(a)  vgetq_lane_s16(__builtin_mpl_vector_minvq_v8i16(a), 0)

int32x2_t __builtin_mpl_vector_minv_v2i32(int32x2_t a);
#define vminv_s32(a)  vget_lane_s32(__builtin_mpl_vector_minv_v2i32(a), 0)

int32x4_t __builtin_mpl_vector_minvq_v4i32(int32x4_t a);
#define vminvq_s32(a)  vgetq_lane_s32(__builtin_mpl_vector_minvq_v4i32(a), 0)

uint8x8_t __builtin_mpl_vector_minv_v8u8(uint8x8_t a);
#define vminv_u8(a)  vget_lane_u8(__builtin_mpl_vector_minv_v8u8(a), 0)

uint8x16_t __builtin_mpl_vector_minvq_v16u8(uint8x16_t a);
#define vminvq_u8(a)  vgetq_lane_u8(__builtin_mpl_vector_minvq_v16u8(a), 0)

uint16x4_t __builtin_mpl_vector_minv_v4u16(uint16x4_t a);
#define vminv_u16(a)  vget_lane_u16(__builtin_mpl_vector_minv_v4u16(a), 0)

uint16x8_t __builtin_mpl_vector_minvq_v8u16(uint16x8_t a);
#define vminvq_u16(a)  vgetq_lane_u16(__builtin_mpl_vector_minvq_v8u16(a), 0)

uint32x2_t __builtin_mpl_vector_minv_v2u32(uint32x2_t a);
#define vminv_u32(a)  vget_lane_u32(__builtin_mpl_vector_minv_v2u32(a), 0)

uint32x4_t __builtin_mpl_vector_minvq_v4u32(uint32x4_t a);
#define vminvq_u32(a)  vgetq_lane_u32(__builtin_mpl_vector_minvq_v4u32(a), 0)

uint8x8_t __builtin_mpl_vector_tst_v8i8(int8x8_t a, int8x8_t b);
#define vtst_s8(a, b)  __builtin_mpl_vector_tst_v8i8(a, b)

uint8x16_t __builtin_mpl_vector_tstq_v16i8(int8x16_t a, int8x16_t b);
#define vtstq_s8(a, b)  __builtin_mpl_vector_tstq_v16i8(a, b)

uint16x4_t __builtin_mpl_vector_tst_v4i16(int16x4_t a, int16x4_t b);
#define vtst_s16(a, b)  __builtin_mpl_vector_tst_v4i16(a, b)

uint16x8_t __builtin_mpl_vector_tstq_v8i16(int16x8_t a, int16x8_t b);
#define vtstq_s16(a, b)  __builtin_mpl_vector_tstq_v8i16(a, b)

uint32x2_t __builtin_mpl_vector_tst_v2i32(int32x2_t a, int32x2_t b);
#define vtst_s32(a, b)  __builtin_mpl_vector_tst_v2i32(a, b)

uint32x4_t __builtin_mpl_vector_tstq_v4i32(int32x4_t a, int32x4_t b);
#define vtstq_s32(a, b)  __builtin_mpl_vector_tstq_v4i32(a, b)

uint8x8_t __builtin_mpl_vector_tst_v8u8(uint8x8_t a, uint8x8_t b);
#define vtst_u8(a, b)  __builtin_mpl_vector_tst_v8u8(a, b)

uint8x16_t __builtin_mpl_vector_tstq_v16u8(uint8x16_t a, uint8x16_t b);
#define vtstq_u8(a, b)  __builtin_mpl_vector_tstq_v16u8(a, b)

uint16x4_t __builtin_mpl_vector_tst_v4u16(uint16x4_t a, uint16x4_t b);
#define vtst_u16(a, b)  __builtin_mpl_vector_tst_v4u16(a, b)

uint16x8_t __builtin_mpl_vector_tstq_v8u16(uint16x8_t a, uint16x8_t b);
#define vtstq_u16(a, b)  __builtin_mpl_vector_tstq_v8u16(a, b)

uint32x2_t __builtin_mpl_vector_tst_v2u32(uint32x2_t a, uint32x2_t b);
#define vtst_u32(a, b)  __builtin_mpl_vector_tst_v2u32(a, b)

uint32x4_t __builtin_mpl_vector_tstq_v4u32(uint32x4_t a, uint32x4_t b);
#define vtstq_u32(a, b)  __builtin_mpl_vector_tstq_v4u32(a, b)

uint64x1_t __builtin_mpl_vector_tst_v1i64(int64x1_t a, int64x1_t b);
#define vtst_s64(a, b)  __builtin_mpl_vector_tst_v1i64(a, b)

uint64x2_t __builtin_mpl_vector_tstq_v2i64(int64x2_t a, int64x2_t b);
#define vtstq_s64(a, b)  __builtin_mpl_vector_tstq_v2i64(a, b)

uint64x1_t __builtin_mpl_vector_tst_v1u64(uint64x1_t a, uint64x1_t b);
#define vtst_u64(a, b)  __builtin_mpl_vector_tst_v1u64(a, b)

uint64x2_t __builtin_mpl_vector_tstq_v2u64(uint64x2_t a, uint64x2_t b);
#define vtstq_u64(a, b)  __builtin_mpl_vector_tstq_v2u64(a, b)

#define vtstd_s64(a, b) ((a & b) ? -1ll : 0ll)

#define vtstd_u64(a, b) ((a & b) ? -1ll : 0ll)

int8x8_t __builtin_mpl_vector_qmovnh_i16(int16x4_t a);
#define vqmovnh_s16(a)  vget_lane_s8( \
  __builtin_mpl_vector_qmovnh_i16(__builtin_mpl_vector_from_scalar_v4i16(a)), 0)

int16x4_t __builtin_mpl_vector_qmovns_i32(int32x2_t a);
#define vqmovns_s32(a)  vget_lane_s16( \
  __builtin_mpl_vector_qmovns_i32(__builtin_mpl_vector_from_scalar_v2i32(a)), 0)

int32x2_t __builtin_mpl_vector_qmovnd_i64(int64x1_t a);
#define vqmovnd_s64(a)  vget_lane_s32( \
  __builtin_mpl_vector_qmovnd_i64(__builtin_mpl_vector_from_scalar_v1i64(a)), 0)

uint8x8_t __builtin_mpl_vector_qmovnh_u16(uint16x4_t a);
#define vqmovnh_u16(a)  vget_lane_u8( \
  __builtin_mpl_vector_qmovnh_u16(__builtin_mpl_vector_from_scalar_v4u16(a)), 0)

uint16x4_t __builtin_mpl_vector_qmovns_u32(uint32x2_t a);
#define vqmovns_u32(a)  vget_lane_u16( \
  __builtin_mpl_vector_qmovns_u32(__builtin_mpl_vector_from_scalar_v2u32(a)), 0)

uint32x2_t __builtin_mpl_vector_qmovnd_u64(uint64x1_t a);
#define vqmovnd_u64(a)  vget_lane_u32( \
  __builtin_mpl_vector_qmovnd_u64(__builtin_mpl_vector_from_scalar_v1u64(a)), 0)

int8x16_t __builtin_mpl_vector_qmovn_high_v16i8(int8x8_t a, int16x8_t b);
#define vqmovn_high_s16(a, b)  __builtin_mpl_vector_qmovn_high_v16i8(a, b)

int16x8_t __builtin_mpl_vector_qmovn_high_v8i16(int16x4_t a, int32x4_t b);
#define vqmovn_high_s32(a, b)  __builtin_mpl_vector_qmovn_high_v8i16(a, b)

int32x4_t __builtin_mpl_vector_qmovn_high_v4i32(int32x2_t a, int64x2_t b);
#define vqmovn_high_s64(a, b)  __builtin_mpl_vector_qmovn_high_v4i32(a, b)

uint8x16_t __builtin_mpl_vector_qmovn_high_v16u8(uint8x8_t a, uint16x8_t b);
#define vqmovn_high_u16(a, b)  __builtin_mpl_vector_qmovn_high_v16u8(a, b)

uint16x8_t __builtin_mpl_vector_qmovn_high_v8u16(uint16x4_t a, uint32x4_t b);
#define vqmovn_high_u32(a, b)  __builtin_mpl_vector_qmovn_high_v8u16(a, b)

uint32x4_t __builtin_mpl_vector_qmovn_high_v4u32(uint32x2_t a, uint64x2_t b);
#define vqmovn_high_u64(a, b)  __builtin_mpl_vector_qmovn_high_v4u32(a, b)

uint8x8_t __builtin_mpl_vector_qmovun_v8u8(int16x8_t a);
#define vqmovun_s16(a)  __builtin_mpl_vector_qmovun_v8u8(a)

uint16x4_t __builtin_mpl_vector_qmovun_v4u16(int32x4_t a);
#define vqmovun_s32(a)  __builtin_mpl_vector_qmovun_v4u16(a)

uint32x2_t __builtin_mpl_vector_qmovun_v2u32(int64x2_t a);
#define vqmovun_s64(a)  __builtin_mpl_vector_qmovun_v2u32(a)

uint8x8_t __builtin_mpl_vector_qmovunh_i16(int16x4_t a);
#define vqmovunh_s16(a)  vget_lane_s8( \
  __builtin_mpl_vector_qmovunh_i16(__builtin_mpl_vector_from_scalar_v4i16(a)), 0)

uint16x4_t __builtin_mpl_vector_qmovuns_i32(int32x2_t a);
#define vqmovuns_s32(a)  vget_lane_s16( \
  __builtin_mpl_vector_qmovuns_i32(__builtin_mpl_vector_from_scalar_v2i32(a)), 0)

uint32x2_t __builtin_mpl_vector_qmovund_i64(int64x1_t a);
#define vqmovund_s64(a)  vget_lane_s32( \
  __builtin_mpl_vector_qmovund_i64(__builtin_mpl_vector_from_scalar_v1i64(a)), 0)

uint8x16_t __builtin_mpl_vector_qmovun_high_v16u8(uint8x8_t a, int16x8_t b);
#define vqmovun_high_s16(a, b)  __builtin_mpl_vector_qmovun_high_v16u8(a, b)

uint16x8_t __builtin_mpl_vector_qmovun_high_v8u16(uint16x4_t a, int32x4_t b);
#define vqmovun_high_s32(a, b)  __builtin_mpl_vector_qmovun_high_v8u16(a, b)

uint32x4_t __builtin_mpl_vector_qmovun_high_v4u32(uint32x2_t a, int64x2_t b);
#define vqmovun_high_s64(a, b)  __builtin_mpl_vector_qmovun_high_v4u32(a, b)

int16x4_t __builtin_mpl_vector_mul_n_v4i16(int16x4_t a, int16_t b);
#define vmul_n_s16(a, b)  (a * (int16x4_t){b, b, b, b})

int16x8_t __builtin_mpl_vector_mulq_n_v8i16(int16x8_t a, int16_t b);
#define vmulq_n_s16(a, b)  (a * (int16x8_t){b, b, b, b, b, b, b, b})

int32x2_t __builtin_mpl_vector_mul_n_v2i32(int32x2_t a, int32_t b);
#define vmul_n_s32(a, b)  (a * (int32x2_t){b, b})

int32x4_t __builtin_mpl_vector_mulq_n_v4i32(int32x4_t a, int32_t b);
#define vmulq_n_s32(a, b)  (a * (int32x4_t){b, b, b, b})

uint16x4_t __builtin_mpl_vector_mul_n_v4u16(uint16x4_t a, uint16_t b);
#define vmul_n_u16(a, b)  (a * (uint16x4_t){b, b, b, b})

uint16x8_t __builtin_mpl_vector_mulq_n_v8u16(uint16x8_t a, uint16_t b);
#define vmulq_n_u16(a, b)  (a * (uint16x8_t){b, b, b, b, b, b, b, b})

uint32x2_t __builtin_mpl_vector_mul_n_v2u32(uint32x2_t a, uint32_t b);
#define vmul_n_u32(a, b)  (a * (uint32x2_t){b, b})

uint32x4_t __builtin_mpl_vector_mulq_n_v4u32(uint32x4_t a, uint32_t b);
#define vmulq_n_u32(a, b)  (a * (uint32x4_t){b, b, b, b})

int16x4_t __builtin_mpl_vector_mul_lane_v4i16(int16x4_t a, int16x4_t b, const int c);
#define vmul_lane_s16(a, b, c)  __builtin_mpl_vector_mul_lane_v4i16(a, b, c)

int16x8_t __builtin_mpl_vector_mulq_lane_v8i16(int16x8_t a, int16x4_t b, const int c);
#define vmulq_lane_s16(a, b, c)  __builtin_mpl_vector_mulq_lane_v8i16(a, b, c)

int32x2_t __builtin_mpl_vector_mul_lane_v2i32(int32x2_t a, int32x2_t b, const int c);
#define vmul_lane_s32(a, b, c)  __builtin_mpl_vector_mul_lane_v2i32(a, b, c)

int32x4_t __builtin_mpl_vector_mulq_lane_v4i32(int32x4_t a, int32x2_t b, const int c);
#define vmulq_lane_s32(a, b, c)  __builtin_mpl_vector_mulq_lane_v4i32(a, b, c)

uint16x4_t __builtin_mpl_vector_mul_lane_v4u16(uint16x4_t a, uint16x4_t b, const int c);
#define vmul_lane_u16(a, b, c)  __builtin_mpl_vector_mul_lane_v4u16(a, b, c)

uint16x8_t __builtin_mpl_vector_mulq_lane_v8u16(uint16x8_t a, uint16x4_t b, const int c);
#define vmulq_lane_u16(a, b, c)  __builtin_mpl_vector_mulq_lane_v8u16(a, b, c)

uint32x2_t __builtin_mpl_vector_mul_lane_v2u32(uint32x2_t a, uint32x2_t b, const int c);
#define vmul_lane_u32(a, b, c)  __builtin_mpl_vector_mul_lane_v2u32(a, b, c)

uint32x4_t __builtin_mpl_vector_mulq_lane_v4u32(uint32x4_t a, uint32x2_t b, const int c);
#define vmulq_lane_u32(a, b, c)  __builtin_mpl_vector_mulq_lane_v4u32(a, b, c)

int16x4_t __builtin_mpl_vector_mul_laneq_v4i16(int16x4_t a, int16x8_t b, const int c);
#define vmul_laneq_s16(a, b, c)  __builtin_mpl_vector_mul_laneq_v4i16(a, b, c)

int16x8_t __builtin_mpl_vector_mulq_laneq_v8i16(int16x8_t a, int16x8_t b, const int c);
#define vmulq_laneq_s16(a, b, c)  __builtin_mpl_vector_mulq_laneq_v8i16(a, b, c)

int32x2_t __builtin_mpl_vector_mul_laneq_v2i32(int32x2_t a, int32x4_t b, const int c);
#define vmul_laneq_s32(a, b, c)  __builtin_mpl_vector_mul_laneq_v2i32(a, b, c)

int32x4_t __builtin_mpl_vector_mulq_laneq_v4i32(int32x4_t a, int32x4_t b, const int c);
#define vmulq_laneq_s32(a, b, c)  __builtin_mpl_vector_mulq_laneq_v4i32(a, b, c)

uint16x4_t __builtin_mpl_vector_mul_laneq_v4u16(uint16x4_t a, uint16x8_t b, const int c);
#define vmul_laneq_u16(a, b, c)  __builtin_mpl_vector_mul_laneq_v4u16(a, b, c)

uint16x8_t __builtin_mpl_vector_mulq_laneq_v8u16(uint16x8_t a, uint16x8_t b, const int c);
#define vmulq_laneq_u16(a, b, c)  __builtin_mpl_vector_mulq_laneq_v8u16(a, b, c)

uint32x2_t __builtin_mpl_vector_mul_laneq_v2u32(uint32x2_t a, uint32x4_t b, const int c);
#define vmul_laneq_u32(a, b, c)  __builtin_mpl_vector_mul_laneq_v2u32(a, b, c)

uint32x4_t __builtin_mpl_vector_mulq_laneq_v4u32(uint32x4_t a, uint32x4_t b, const int c);
#define vmulq_laneq_u32(a, b, c)  __builtin_mpl_vector_mulq_laneq_v4u32(a, b, c)

int32x4_t __builtin_mpl_vector_mull_n_v4i32(int16x4_t a, int16_t b);
#define vmull_n_s16(a, b)  (vmull_s16(a, ((int16x4_t){b, b, b, b})))

int64x2_t __builtin_mpl_vector_mull_n_v2i64(int32x2_t a, int32_t b);
#define vmull_n_s32(a, b)  (vmull_s32(a, ((int32x2_t){b, b})))

uint32x4_t __builtin_mpl_vector_mull_n_v4u32(uint16x4_t a, uint16_t b);
#define vmull_n_u16(a, b)  (vmull_u16(a, ((uint16x4_t){b, b, b, b})))

uint64x2_t __builtin_mpl_vector_mull_n_v2u64(uint32x2_t a, uint32_t b);
#define vmull_n_u32(a, b)  (vmull_u32(a, ((uint32x2_t){b, b})))

int32x4_t __builtin_mpl_vector_mull_high_n_v4i32(int16x8_t a, int16_t b);
#define vmull_high_n_s16(a, b)  vmull_n_s16((vget_high_s16(a)), b)

int64x2_t __builtin_mpl_vector_mull_high_n_v2i64(int32x4_t a, int32_t b);
#define vmull_high_n_s32(a, b)  vmull_n_s32((vget_high_s32(a)), b)

uint32x4_t __builtin_mpl_vector_mull_high_n_v4u32(uint16x8_t a, uint16_t b);
#define vmull_high_n_u16(a, b)  vmull_n_u16((vget_high_u16(a)), b)

uint64x2_t __builtin_mpl_vector_mull_high_n_v2u64(uint32x4_t a, uint32_t b);
#define vmull_high_n_u32(a, b)  vmull_n_u32((vget_high_u32(a)), b)

int32x4_t __builtin_mpl_vector_mull_lane_v4i32(int16x4_t a, int16x4_t b, const int c);
#define vmull_lane_s16(a, b, c)  __builtin_mpl_vector_mull_lane_v4i32(a, b, c)

int64x2_t __builtin_mpl_vector_mull_lane_v2i64(int32x2_t a, int32x2_t b, const int c);
#define vmull_lane_s32(a, b, c)  __builtin_mpl_vector_mull_lane_v2i64(a, b, c)

uint32x4_t __builtin_mpl_vector_mull_lane_v4u32(uint16x4_t a, uint16x4_t b, const int c);
#define vmull_lane_u16(a, b, c)  __builtin_mpl_vector_mull_lane_v4u32(a, b, c)

uint64x2_t __builtin_mpl_vector_mull_lane_v2u64(uint32x2_t a, uint32x2_t b, const int c);
#define vmull_lane_u32(a, b, c)  __builtin_mpl_vector_mull_lane_v2u64(a, b, c)

int32x4_t __builtin_mpl_vector_mull_high_lane_v4i32(int16x8_t a, int16x4_t b, const int c);
#define vmull_high_lane_s16(a, b, c)  __builtin_mpl_vector_mull_high_lane_v4i32(a, b, c)

int64x2_t __builtin_mpl_vector_mull_high_lane_v2i64(int32x4_t a, int32x2_t b, const int c);
#define vmull_high_lane_s32(a, b, c)  __builtin_mpl_vector_mull_high_lane_v2i64(a, b, c)

uint32x4_t __builtin_mpl_vector_mull_high_lane_v4u32(uint16x8_t a, uint16x4_t b, const int c);
#define vmull_high_lane_u16(a, b, c)  __builtin_mpl_vector_mull_high_lane_v4u32(a, b, c)

uint64x2_t __builtin_mpl_vector_mull_high_lane_v2u64(uint32x4_t a, uint32x2_t b, const int c);
#define vmull_high_lane_u32(a, b, c)  __builtin_mpl_vector_mull_high_lane_v2u64(a, b, c)

int32x4_t __builtin_mpl_vector_mull_laneq_v4i32(int16x4_t a, int16x8_t b, const int c);
#define vmull_laneq_s16(a, b, c)  __builtin_mpl_vector_mull_laneq_v4i32(a, b, c)

int64x2_t __builtin_mpl_vector_mull_laneq_v2i64(int32x2_t a, int32x4_t b, const int c);
#define vmull_laneq_s32(a, b, c)  __builtin_mpl_vector_mull_laneq_v2i64(a, b, c)

uint32x4_t __builtin_mpl_vector_mull_laneq_v4u32(uint16x4_t a, uint16x8_t b, const int c);
#define vmull_laneq_u16(a, b, c)  __builtin_mpl_vector_mull_laneq_v4u32(a, b, c)

uint64x2_t __builtin_mpl_vector_mull_laneq_v2u64(uint32x2_t a, uint32x4_t b, const int c);
#define vmull_laneq_u32(a, b, c)  __builtin_mpl_vector_mull_laneq_v2u64(a, b, c)

int32x4_t __builtin_mpl_vector_mull_high_laneq_v4i32(int16x8_t a, int16x8_t b, const int c);
#define vmull_high_laneq_s16(a, b, c)  __builtin_mpl_vector_mull_high_laneq_v4i32(a, b, c)

int64x2_t __builtin_mpl_vector_mull_high_laneq_v2i64(int32x4_t a, int32x4_t b, const int c);
#define vmull_high_laneq_s32(a, b, c)  __builtin_mpl_vector_mull_high_laneq_v2i64(a, b, c)

uint32x4_t __builtin_mpl_vector_mull_high_laneq_v4u32(uint16x8_t a, uint16x8_t b, const int c);
#define vmull_high_laneq_u16(a, b, c)  __builtin_mpl_vector_mull_high_laneq_v4u32(a, b, c)

uint64x2_t __builtin_mpl_vector_mull_high_laneq_v2u64(uint32x4_t a, uint32x4_t b, const int c);
#define vmull_high_laneq_u32(a, b, c)  __builtin_mpl_vector_mull_high_laneq_v2u64(a, b, c)

int8x8_t __builtin_mpl_vector_neg_v8i8(int8x8_t a);
#define vneg_s8(a)  __builtin_mpl_vector_neg_v8i8(a)

int8x16_t __builtin_mpl_vector_negq_v16i8(int8x16_t a);
#define vnegq_s8(a)  __builtin_mpl_vector_negq_v16i8(a)

int16x4_t __builtin_mpl_vector_neg_v4i16(int16x4_t a);
#define vneg_s16(a)  __builtin_mpl_vector_neg_v4i16(a)

int16x8_t __builtin_mpl_vector_negq_v8i16(int16x8_t a);
#define vnegq_s16(a)  __builtin_mpl_vector_negq_v8i16(a)

int32x2_t __builtin_mpl_vector_neg_v2i32(int32x2_t a);
#define vneg_s32(a)  __builtin_mpl_vector_neg_v2i32(a)

int32x4_t __builtin_mpl_vector_negq_v4i32(int32x4_t a);
#define vnegq_s32(a)  __builtin_mpl_vector_negq_v4i32(a)

int64x1_t __builtin_mpl_vector_neg_v1i64(int64x1_t a);
#define vneg_s64(a)  __builtin_mpl_vector_neg_v1i64(a)

int64_t __builtin_mpl_vector_negd_v1i64(int64_t a);
#define vnegd_s64(a)  __builtin_mpl_vector_negd_v1i64(a)

int64x2_t __builtin_mpl_vector_negq_v2i64(int64x2_t a);
#define vnegq_s64(a)  __builtin_mpl_vector_negq_v2i64(a)

int8x8_t __builtin_mpl_vector_mvn_v8i8(int8x8_t a);
#define vmvn_s8(a)  __builtin_mpl_vector_mvn_v8i8(a)

int8x16_t __builtin_mpl_vector_mvnq_v16i8(int8x16_t a);
#define vmvnq_s8(a)  __builtin_mpl_vector_mvnq_v16i8(a)

int16x4_t __builtin_mpl_vector_mvn_v4i16(int16x4_t a);
#define vmvn_s16(a)  ((int8x8_t)__builtin_mpl_vector_mvn_v8i8((int8x8_t)a))

int16x8_t __builtin_mpl_vector_mvnq_v8i16(int16x8_t a);
#define vmvnq_s16(a)  ((int8x16_t)__builtin_mpl_vector_mvnq_v16i8((int8x16_t)a))

int32x2_t __builtin_mpl_vector_mvn_v2i32(int32x2_t a);
#define vmvn_s32(a)  ((int8x8_t)__builtin_mpl_vector_mvn_v8i8((int8x8_t)a))

int32x4_t __builtin_mpl_vector_mvnq_v4i32(int32x4_t a);
#define vmvnq_s32(a)  ((int8x16_t)__builtin_mpl_vector_mvnq_v16i8((int8x16_t)a))

uint8x8_t __builtin_mpl_vector_mvn_v8u8(uint8x8_t a);
#define vmvn_u8(a)  __builtin_mpl_vector_mvn_v8u8(a)

uint8x16_t __builtin_mpl_vector_mvnq_v16u8(uint8x16_t a);
#define vmvnq_u8(a)  __builtin_mpl_vector_mvnq_v16u8(a)

uint16x4_t __builtin_mpl_vector_mvn_v4u16(uint16x4_t a);
#define vmvn_u16(a)  ((uint8x8_t)__builtin_mpl_vector_mvn_v8u8((uint8x8_t)a))

uint16x8_t __builtin_mpl_vector_mvnq_v8u16(uint16x8_t a);
#define vmvnq_u16(a)  ((uint8x16_t)__builtin_mpl_vector_mvnq_v16u8((uint8x16_t)a))

uint32x2_t __builtin_mpl_vector_mvn_v2u32(uint32x2_t a);
#define vmvn_u32(a)  ((uint8x8_t)__builtin_mpl_vector_mvn_v8u8((uint8x8_t)a))

uint32x4_t __builtin_mpl_vector_mvnq_v4u32(uint32x4_t a);
#define vmvnq_u32(a)  ((uint8x16_t)__builtin_mpl_vector_mvnq_v16u8((uint8x16_t)a))

int8x8_t __builtin_mpl_vector_orn_v8i8(int8x8_t a, int8x8_t b);
#define vorn_s8(a, b)  ((int8x8_t)__builtin_mpl_vector_orn_v8i8((int8x8_t)a, (int8x8_t)b))

int8x16_t __builtin_mpl_vector_ornq_v16i8(int8x16_t a, int8x16_t b);
#define vornq_s8(a, b)  ((int8x16_t)__builtin_mpl_vector_ornq_v16i8((int8x16_t)a, (int8x16_t)b))

int16x4_t __builtin_mpl_vector_orn_v4i16(int16x4_t a, int16x4_t b);
#define vorn_s16(a, b)  ((int8x8_t)__builtin_mpl_vector_orn_v8i8((int8x8_t)a, (int8x8_t)b))

int16x8_t __builtin_mpl_vector_ornq_v8i16(int16x8_t a, int16x8_t b);
#define vornq_s16(a, b)  ((int8x16_t)__builtin_mpl_vector_ornq_v16i8((int8x16_t)a, (int8x16_t)b))

int32x2_t __builtin_mpl_vector_orn_v2i32(int32x2_t a, int32x2_t b);
#define vorn_s32(a, b)  ((int8x8_t)__builtin_mpl_vector_orn_v8i8((int8x8_t)a, (int8x8_t)b))

int32x4_t __builtin_mpl_vector_ornq_v4i32(int32x4_t a, int32x4_t b);
#define vornq_s32(a, b)  ((int8x16_t)__builtin_mpl_vector_ornq_v16i8((int8x16_t)a, (int8x16_t)b))

int64x1_t __builtin_mpl_vector_orn_v1i64(int64x1_t a, int64x1_t b);
#define vorn_s64(a, b)  ((int8x8_t)__builtin_mpl_vector_orn_v8i8((int8x8_t)a, (int8x8_t)b))

int64x2_t __builtin_mpl_vector_ornq_v2i64(int64x2_t a, int64x2_t b);
#define vornq_s64(a, b)  ((int8x16_t)__builtin_mpl_vector_ornq_v16i8((int8x16_t)a, (int8x16_t)b))

uint8x8_t __builtin_mpl_vector_orn_v8u8(uint8x8_t a, uint8x8_t b);
#define vorn_u8(a, b)  ((uint8x8_t)__builtin_mpl_vector_orn_v8u8((uint8x8_t)a, (uint8x8_t)b))

uint8x16_t __builtin_mpl_vector_ornq_v16u8(uint8x16_t a, uint8x16_t b);
#define vornq_u8(a, b)  ((uint8x16_t)__builtin_mpl_vector_ornq_v16u8((uint8x16_t)a, (uint8x16_t)b))

uint16x4_t __builtin_mpl_vector_orn_v4u16(uint16x4_t a, uint16x4_t b);
#define vorn_u16(a, b)  ((uint8x8_t)__builtin_mpl_vector_orn_v8u8((uint8x8_t)a, (uint8x8_t)b))

uint16x8_t __builtin_mpl_vector_ornq_v8u16(uint16x8_t a, uint16x8_t b);
#define vornq_u16(a, b)  ((uint8x16_t)__builtin_mpl_vector_ornq_v16u8((uint8x16_t)a, (uint8x16_t)b))

uint32x2_t __builtin_mpl_vector_orn_v2u32(uint32x2_t a, uint32x2_t b);
#define vorn_u32(a, b)  ((uint8x8_t)__builtin_mpl_vector_orn_v8u8((uint8x8_t)a, (uint8x8_t)b))

uint32x4_t __builtin_mpl_vector_ornq_v4u32(uint32x4_t a, uint32x4_t b);
#define vornq_u32(a, b)  ((uint8x16_t)__builtin_mpl_vector_ornq_v16u8((uint8x16_t)a, (uint8x16_t)b))

uint64x1_t __builtin_mpl_vector_orn_v1u64(uint64x1_t a, uint64x1_t b);
#define vorn_u64(a, b)  ((uint8x8_t)__builtin_mpl_vector_orn_v8u8((uint8x8_t)a, (uint8x8_t)b))

uint64x2_t __builtin_mpl_vector_ornq_v2u64(uint64x2_t a, uint64x2_t b);
#define vornq_u64(a, b)  ((uint8x16_t)__builtin_mpl_vector_ornq_v16u8((uint8x16_t)a, (uint8x16_t)b))

int8x8_t __builtin_mpl_vector_cls_v8i8(int8x8_t a);
#define vcls_s8(a)  __builtin_mpl_vector_cls_v8i8(a)

int8x16_t __builtin_mpl_vector_clsq_v16i8(int8x16_t a);
#define vclsq_s8(a)  __builtin_mpl_vector_clsq_v16i8(a)

int16x4_t __builtin_mpl_vector_cls_v4i16(int16x4_t a);
#define vcls_s16(a)  __builtin_mpl_vector_cls_v4i16(a)

int16x8_t __builtin_mpl_vector_clsq_v8i16(int16x8_t a);
#define vclsq_s16(a)  __builtin_mpl_vector_clsq_v8i16(a)

int32x2_t __builtin_mpl_vector_cls_v2i32(int32x2_t a);
#define vcls_s32(a)  __builtin_mpl_vector_cls_v2i32(a)

int32x4_t __builtin_mpl_vector_clsq_v4i32(int32x4_t a);
#define vclsq_s32(a)  __builtin_mpl_vector_clsq_v4i32(a)

int8x8_t __builtin_mpl_vector_cls_v8u8(uint8x8_t a);
#define vcls_u8(a)  __builtin_mpl_vector_cls_v8u8(a)

int8x16_t __builtin_mpl_vector_clsq_v16u8(uint8x16_t a);
#define vclsq_u8(a)  __builtin_mpl_vector_clsq_v16u8(a)

int16x4_t __builtin_mpl_vector_cls_v4u16(uint16x4_t a);
#define vcls_u16(a)  __builtin_mpl_vector_cls_v4u16(a)

int16x8_t __builtin_mpl_vector_clsq_v8u16(uint16x8_t a);
#define vclsq_u16(a)  __builtin_mpl_vector_clsq_v8u16(a)

int32x2_t __builtin_mpl_vector_cls_v2u32(uint32x2_t a);
#define vcls_u32(a)  __builtin_mpl_vector_cls_v2u32(a)

int32x4_t __builtin_mpl_vector_clsq_v4u32(uint32x4_t a);
#define vclsq_u32(a)  __builtin_mpl_vector_clsq_v4u32(a)

int8x8_t __builtin_mpl_vector_clz_v8i8(int8x8_t a);
#define vclz_s8(a)  __builtin_mpl_vector_clz_v8i8(a)

int8x16_t __builtin_mpl_vector_clzq_v16i8(int8x16_t a);
#define vclzq_s8(a)  __builtin_mpl_vector_clzq_v16i8(a)

int16x4_t __builtin_mpl_vector_clz_v4i16(int16x4_t a);
#define vclz_s16(a)  __builtin_mpl_vector_clz_v4i16(a)

int16x8_t __builtin_mpl_vector_clzq_v8i16(int16x8_t a);
#define vclzq_s16(a)  __builtin_mpl_vector_clzq_v8i16(a)

int32x2_t __builtin_mpl_vector_clz_v2i32(int32x2_t a);
#define vclz_s32(a)  __builtin_mpl_vector_clz_v2i32(a)

int32x4_t __builtin_mpl_vector_clzq_v4i32(int32x4_t a);
#define vclzq_s32(a)  __builtin_mpl_vector_clzq_v4i32(a)

uint8x8_t __builtin_mpl_vector_clz_v8u8(uint8x8_t a);
#define vclz_u8(a)  __builtin_mpl_vector_clz_v8u8(a)

uint8x16_t __builtin_mpl_vector_clzq_v16u8(uint8x16_t a);
#define vclzq_u8(a)  __builtin_mpl_vector_clzq_v16u8(a)

uint16x4_t __builtin_mpl_vector_clz_v4u16(uint16x4_t a);
#define vclz_u16(a)  __builtin_mpl_vector_clz_v4u16(a)

uint16x8_t __builtin_mpl_vector_clzq_v8u16(uint16x8_t a);
#define vclzq_u16(a)  __builtin_mpl_vector_clzq_v8u16(a)

uint32x2_t __builtin_mpl_vector_clz_v2u32(uint32x2_t a);
#define vclz_u32(a)  __builtin_mpl_vector_clz_v2u32(a)

uint32x4_t __builtin_mpl_vector_clzq_v4u32(uint32x4_t a);
#define vclzq_u32(a)  __builtin_mpl_vector_clzq_v4u32(a)

int8x8_t __builtin_mpl_vector_cnt_v8i8(int8x8_t a);
#define vcnt_s8(a)  __builtin_mpl_vector_cnt_v8i8(a)

int8x16_t __builtin_mpl_vector_cntq_v16i8(int8x16_t a);
#define vcntq_s8(a)  __builtin_mpl_vector_cntq_v16i8(a)

uint8x8_t __builtin_mpl_vector_cnt_v8u8(uint8x8_t a);
#define vcnt_u8(a)  __builtin_mpl_vector_cnt_v8u8(a)

uint8x16_t __builtin_mpl_vector_cntq_v16u8(uint8x16_t a);
#define vcntq_u8(a)  __builtin_mpl_vector_cntq_v16u8(a)

int8x8_t __builtin_mpl_vector_bic_v8i8(int8x8_t a, int8x8_t b);
#define vbic_s8(a, b)  __builtin_mpl_vector_bic_v8i8(a, b)

int8x16_t __builtin_mpl_vector_bicq_v16i8(int8x16_t a, int8x16_t b);
#define vbicq_s8(a, b)  __builtin_mpl_vector_bicq_v16i8(a, b)

int16x4_t __builtin_mpl_vector_bic_v4i16(int16x4_t a, int16x4_t b);
#define vbic_s16(a, b)  __builtin_mpl_vector_bic_v4i16(a, b)

int16x8_t __builtin_mpl_vector_bicq_v8i16(int16x8_t a, int16x8_t b);
#define vbicq_s16(a, b)  __builtin_mpl_vector_bicq_v8i16(a, b)

int32x2_t __builtin_mpl_vector_bic_v2i32(int32x2_t a, int32x2_t b);
#define vbic_s32(a, b)  __builtin_mpl_vector_bic_v2i32(a, b)

int32x4_t __builtin_mpl_vector_bicq_v4i32(int32x4_t a, int32x4_t b);
#define vbicq_s32(a, b)  __builtin_mpl_vector_bicq_v4i32(a, b)

int64x1_t __builtin_mpl_vector_bic_v1i64(int64x1_t a, int64x1_t b);
#define vbic_s64(a, b)  __builtin_mpl_vector_bic_v1i64(a, b)

int64x2_t __builtin_mpl_vector_bicq_v2i64(int64x2_t a, int64x2_t b);
#define vbicq_s64(a, b)  __builtin_mpl_vector_bicq_v2i64(a, b)

uint8x8_t __builtin_mpl_vector_bic_v8u8(uint8x8_t a, uint8x8_t b);
#define vbic_u8(a, b)  __builtin_mpl_vector_bic_v8u8(a, b)

uint8x16_t __builtin_mpl_vector_bicq_v16u8(uint8x16_t a, uint8x16_t b);
#define vbicq_u8(a, b)  __builtin_mpl_vector_bicq_v16u8(a, b)

uint16x4_t __builtin_mpl_vector_bic_v4u16(uint16x4_t a, uint16x4_t b);
#define vbic_u16(a, b)  __builtin_mpl_vector_bic_v4u16(a, b)

uint16x8_t __builtin_mpl_vector_bicq_v8u16(uint16x8_t a, uint16x8_t b);
#define vbicq_u16(a, b)  __builtin_mpl_vector_bicq_v8u16(a, b)

uint32x2_t __builtin_mpl_vector_bic_v2u32(uint32x2_t a, uint32x2_t b);
#define vbic_u32(a, b)  __builtin_mpl_vector_bic_v2u32(a, b)

uint32x4_t __builtin_mpl_vector_bicq_v4u32(uint32x4_t a, uint32x4_t b);
#define vbicq_u32(a, b)  __builtin_mpl_vector_bicq_v4u32(a, b)

uint64x1_t __builtin_mpl_vector_bic_v1u64(uint64x1_t a, uint64x1_t b);
#define vbic_u64(a, b)  __builtin_mpl_vector_bic_v1u64(a, b)

uint64x2_t __builtin_mpl_vector_bicq_v2u64(uint64x2_t a, uint64x2_t b);
#define vbicq_u64(a, b)  __builtin_mpl_vector_bicq_v2u64(a, b)

int8x8_t __builtin_mpl_vector_bsl_v8i8(uint8x8_t a, int8x8_t b, int8x8_t c);
#define vbsl_s8(a, b, c)  ((int8x8_t)__builtin_mpl_vector_bsl_v8i8((int8x8_t)a, (int8x8_t)b, (int8x8_t)c))

int8x16_t __builtin_mpl_vector_bslq_v16i8(uint8x16_t a, int8x16_t b, int8x16_t c);
#define vbslq_s8(a, b, c)  ((int8x16_t)__builtin_mpl_vector_bslq_v16i8((int8x16_t)a, (int8x16_t)b, (int8x16_t)c))

int16x4_t __builtin_mpl_vector_bsl_v4i16(uint16x4_t a, int16x4_t b, int16x4_t c);
#define vbsl_s16(a, b, c)  ((int8x8_t)__builtin_mpl_vector_bsl_v8i8((int8x8_t)a, (int8x8_t)b, (int8x8_t)c))

int16x8_t __builtin_mpl_vector_bslq_v8i16(uint16x8_t a, int16x8_t b, int16x8_t c);
#define vbslq_s16(a, b, c)  ((int8x16_t)__builtin_mpl_vector_bslq_v16i8((int8x16_t)a, (int8x16_t)b, (int8x16_t)c))

int32x2_t __builtin_mpl_vector_bsl_v2i32(uint32x2_t a, int32x2_t b, int32x2_t c);
#define vbsl_s32(a, b, c)  ((int8x8_t)__builtin_mpl_vector_bsl_v8i8((int8x8_t)a, (int8x8_t)b, (int8x8_t)c))

int32x4_t __builtin_mpl_vector_bslq_v4i32(uint32x4_t a, int32x4_t b, int32x4_t c);
#define vbslq_s32(a, b, c)  ((int8x16_t)__builtin_mpl_vector_bslq_v16i8((int8x16_t)a, (int8x16_t)b, (int8x16_t)c))

int64x1_t __builtin_mpl_vector_bsl_v1i64(uint64x1_t a, int64x1_t b, int64x1_t c);
#define vbsl_s64(a, b, c)  ((int8x8_t)__builtin_mpl_vector_bsl_v8i8((int8x8_t)a, (int8x8_t)b, (int8x8_t)c))

int64x2_t __builtin_mpl_vector_bslq_v2i64(uint64x2_t a, int64x2_t b, int64x2_t c);
#define vbslq_s64(a, b, c)  ((int8x16_t)__builtin_mpl_vector_bslq_v16i8((int8x16_t)a, (int8x16_t)b, (int8x16_t)c))

uint8x8_t __builtin_mpl_vector_bsl_v8u8(uint8x8_t a, uint8x8_t b, uint8x8_t c);
#define vbsl_u8(a, b, c)  ((uint8x8_t)__builtin_mpl_vector_bsl_v8u8((uint8x8_t)a, (uint8x8_t)b, (uint8x8_t)c))

uint8x16_t __builtin_mpl_vector_bslq_v16u8(uint8x16_t a, uint8x16_t b, uint8x16_t c);
#define vbslq_u8(a, b, c)  ((uint8x16_t)__builtin_mpl_vector_bslq_v16u8((uint8x16_t)a, (uint8x16_t)b, (uint8x16_t)c))

uint16x4_t __builtin_mpl_vector_bsl_v4u16(uint16x4_t a, uint16x4_t b, uint16x4_t c);
#define vbsl_u16(a, b, c)  ((uint8x8_t)__builtin_mpl_vector_bsl_v8u8((uint8x8_t)a, (uint8x8_t)b, (uint8x8_t)c))

uint16x8_t __builtin_mpl_vector_bslq_v8u16(uint16x8_t a, uint16x8_t b, uint16x8_t c);
#define vbslq_u16(a, b, c)  ((uint8x16_t)__builtin_mpl_vector_bslq_v16u8((uint8x16_t)a, (uint8x16_t)b, (uint8x16_t)c))

uint32x2_t __builtin_mpl_vector_bsl_v2u32(uint32x2_t a, uint32x2_t b, uint32x2_t c);
#define vbsl_u32(a, b, c)  ((uint8x8_t)__builtin_mpl_vector_bsl_v8u8((uint8x8_t)a, (uint8x8_t)b, (uint8x8_t)c))

uint32x4_t __builtin_mpl_vector_bslq_v4u32(uint32x4_t a, uint32x4_t b, uint32x4_t c);
#define vbslq_u32(a, b, c)  ((uint8x16_t)__builtin_mpl_vector_bslq_v16u8((uint8x16_t)a, (uint8x16_t)b, (uint8x16_t)c))

uint64x1_t __builtin_mpl_vector_bsl_v1u64(uint64x1_t a, uint64x1_t b, uint64x1_t c);
#define vbsl_u64(a, b, c)  ((uint8x8_t)__builtin_mpl_vector_bsl_v8u8((uint8x8_t)a, (uint8x8_t)b, (uint8x8_t)c))

uint64x2_t __builtin_mpl_vector_bslq_v2u64(uint64x2_t a, uint64x2_t b, uint64x2_t c);
#define vbslq_u64(a, b, c)  ((uint8x16_t)__builtin_mpl_vector_bslq_v16u8((uint8x16_t)a, (uint8x16_t)b, (uint8x16_t)c))

int8x8_t __builtin_mpl_vector_copy_lane_v8i8(int8x8_t a, const int lane1, int8x8_t b, const int lane2);
#define vcopy_lane_s8(a, lane1, b, lane2)  __builtin_mpl_vector_copy_lane_v8i8(a, lane1, b, lane2)

int8x16_t __builtin_mpl_vector_copyq_lane_v16i8(int8x16_t a, const int lane1, int8x8_t b, const int lane2);
#define vcopyq_lane_s8(a, lane1, b, lane2)  __builtin_mpl_vector_copyq_lane_v16i8(a, lane1, b, lane2)

int16x4_t __builtin_mpl_vector_copy_lane_v4i16(int16x4_t a, const int lane1, int16x4_t b, const int lane2);
#define vcopy_lane_s16(a, lane1, b, lane2)  __builtin_mpl_vector_copy_lane_v4i16(a, lane1, b, lane2)

int16x8_t __builtin_mpl_vector_copyq_lane_v8i16(int16x8_t a, const int lane1, int16x4_t b, const int lane2);
#define vcopyq_lane_s16(a, lane1, b, lane2)  __builtin_mpl_vector_copyq_lane_v8i16(a, lane1, b, lane2)

int32x2_t __builtin_mpl_vector_copy_lane_v2i32(int32x2_t a, const int lane1, int32x2_t b, const int lane2);
#define vcopy_lane_s32(a, lane1, b, lane2)  __builtin_mpl_vector_copy_lane_v2i32(a, lane1, b, lane2)

int32x4_t __builtin_mpl_vector_copyq_lane_v4i32(int32x4_t a, const int lane1, int32x2_t b, const int lane2);
#define vcopyq_lane_s32(a, lane1, b, lane2)  __builtin_mpl_vector_copyq_lane_v4i32(a, lane1, b, lane2)

int64x1_t __builtin_mpl_vector_copy_lane_v1i64(int64x1_t a, const int lane1, int64x1_t b, const int lane2);
#define vcopy_lane_s64(a, lane1, b, lane2)  __builtin_mpl_vector_copy_lane_v1i64(a, lane1, b, lane2)

int64x2_t __builtin_mpl_vector_copyq_lane_v2i64(int64x2_t a, const int lane1, int64x1_t b, const int lane2);
#define vcopyq_lane_s64(a, lane1, b, lane2)  __builtin_mpl_vector_copyq_lane_v2i64(a, lane1, b, lane2)

uint8x8_t __builtin_mpl_vector_copy_lane_v8u8(uint8x8_t a, const int lane1, uint8x8_t b, const int lane2);
#define vcopy_lane_u8(a, lane1, b, lane2)  __builtin_mpl_vector_copy_lane_v8u8(a, lane1, b, lane2)

uint8x16_t __builtin_mpl_vector_copyq_lane_v16u8(uint8x16_t a, const int lane1, uint8x8_t b, const int lane2);
#define vcopyq_lane_u8(a, lane1, b, lane2)  __builtin_mpl_vector_copyq_lane_v16u8(a, lane1, b, lane2)

uint16x4_t __builtin_mpl_vector_copy_lane_v4u16(uint16x4_t a, const int lane1, uint16x4_t b, const int lane2);
#define vcopy_lane_u16(a, lane1, b, lane2)  __builtin_mpl_vector_copy_lane_v4u16(a, lane1, b, lane2)

uint16x8_t __builtin_mpl_vector_copyq_lane_v8u16(uint16x8_t a, const int lane1, uint16x4_t b, const int lane2);
#define vcopyq_lane_u16(a, lane1, b, lane2)  __builtin_mpl_vector_copyq_lane_v8u16(a, lane1, b, lane2)

uint32x2_t __builtin_mpl_vector_copy_lane_v2u32(uint32x2_t a, const int lane1, uint32x2_t b, const int lane2);
#define vcopy_lane_u32(a, lane1, b, lane2)  __builtin_mpl_vector_copy_lane_v2u32(a, lane1, b, lane2)

uint32x4_t __builtin_mpl_vector_copyq_lane_v4u32(uint32x4_t a, const int lane1, uint32x2_t b, const int lane2);
#define vcopyq_lane_u32(a, lane1, b, lane2)  __builtin_mpl_vector_copyq_lane_v4u32(a, lane1, b, lane2)

uint64x1_t __builtin_mpl_vector_copy_lane_v1u64(uint64x1_t a, const int lane1, uint64x1_t b, const int lane2);
#define vcopy_lane_u64(a, lane1, b, lane2)  __builtin_mpl_vector_copy_lane_v1u64(a, lane1, b, lane2)

uint64x2_t __builtin_mpl_vector_copyq_lane_v2u64(uint64x2_t a, const int lane1, uint64x1_t b, const int lane2);
#define vcopyq_lane_u64(a, lane1, b, lane2)  __builtin_mpl_vector_copyq_lane_v2u64(a, lane1, b, lane2)

int8x8_t __builtin_mpl_vector_copy_laneq_v8i8(int8x8_t a, const int lane1, int8x16_t b, const int lane2);
#define vcopy_laneq_s8(a, lane1, b, lane2)  __builtin_mpl_vector_copy_laneq_v8i8(a, lane1, b, lane2)

int8x16_t __builtin_mpl_vector_copyq_laneq_v16i8(int8x16_t a, const int lane1, int8x16_t b, const int lane2);
#define vcopyq_laneq_s8(a, lane1, b, lane2)  __builtin_mpl_vector_copyq_laneq_v16i8(a, lane1, b, lane2)

int16x4_t __builtin_mpl_vector_copy_laneq_v4i16(int16x4_t a, const int lane1, int16x8_t b, const int lane2);
#define vcopy_laneq_s16(a, lane1, b, lane2)  __builtin_mpl_vector_copy_laneq_v4i16(a, lane1, b, lane2)

int16x8_t __builtin_mpl_vector_copyq_laneq_v8i16(int16x8_t a, const int lane1, int16x8_t b, const int lane2);
#define vcopyq_laneq_s16(a, lane1, b, lane2)  __builtin_mpl_vector_copyq_laneq_v8i16(a, lane1, b, lane2)

int32x2_t __builtin_mpl_vector_copy_laneq_v2i32(int32x2_t a, const int lane1, int32x4_t b, const int lane2);
#define vcopy_laneq_s32(a, lane1, b, lane2)  __builtin_mpl_vector_copy_laneq_v2i32(a, lane1, b, lane2)

int32x4_t __builtin_mpl_vector_copyq_laneq_v4i32(int32x4_t a, const int lane1, int32x4_t b, const int lane2);
#define vcopyq_laneq_s32(a, lane1, b, lane2)  __builtin_mpl_vector_copyq_laneq_v4i32(a, lane1, b, lane2)

int64x1_t __builtin_mpl_vector_copy_laneq_v1i64(int64x1_t a, const int lane1, int64x2_t b, const int lane2);
#define vcopy_laneq_s64(a, lane1, b, lane2)  __builtin_mpl_vector_copy_laneq_v1i64(a, lane1, b, lane2)

int64x2_t __builtin_mpl_vector_copyq_laneq_v2i64(int64x2_t a, const int lane1, int64x2_t b, const int lane2);
#define vcopyq_laneq_s64(a, lane1, b, lane2)  __builtin_mpl_vector_copyq_laneq_v2i64(a, lane1, b, lane2)

uint8x8_t __builtin_mpl_vector_copy_laneq_v8u8(uint8x8_t a, const int lane1, uint8x16_t b, const int lane2);
#define vcopy_laneq_u8(a, lane1, b, lane2)  __builtin_mpl_vector_copy_laneq_v8u8(a, lane1, b, lane2)

uint8x16_t __builtin_mpl_vector_copyq_laneq_v16u8(uint8x16_t a, const int lane1, uint8x16_t b, const int lane2);
#define vcopyq_laneq_u8(a, lane1, b, lane2)  __builtin_mpl_vector_copyq_laneq_v16u8(a, lane1, b, lane2)

uint16x4_t __builtin_mpl_vector_copy_laneq_v4u16(uint16x4_t a, const int lane1, uint16x8_t b, const int lane2);
#define vcopy_laneq_u16(a, lane1, b, lane2)  __builtin_mpl_vector_copy_laneq_v4u16(a, lane1, b, lane2)

uint16x8_t __builtin_mpl_vector_copyq_laneq_v8u16(uint16x8_t a, const int lane1, uint16x8_t b, const int lane2);
#define vcopyq_laneq_u16(a, lane1, b, lane2)  __builtin_mpl_vector_copyq_laneq_v8u16(a, lane1, b, lane2)

uint32x2_t __builtin_mpl_vector_copy_laneq_v2u32(uint32x2_t a, const int lane1, uint32x4_t b, const int lane2);
#define vcopy_laneq_u32(a, lane1, b, lane2)  __builtin_mpl_vector_copy_laneq_v2u32(a, lane1, b, lane2)

uint32x4_t __builtin_mpl_vector_copyq_laneq_v4u32(uint32x4_t a, const int lane1, uint32x4_t b, const int lane2);
#define vcopyq_laneq_u32(a, lane1, b, lane2)  __builtin_mpl_vector_copyq_laneq_v4u32(a, lane1, b, lane2)

uint64x1_t __builtin_mpl_vector_copy_laneq_v1u64(uint64x1_t a, const int lane1, uint64x2_t b, const int lane2);
#define vcopy_laneq_u64(a, lane1, b, lane2)  __builtin_mpl_vector_copy_laneq_v1u64(a, lane1, b, lane2)

uint64x2_t __builtin_mpl_vector_copyq_laneq_v2u64(uint64x2_t a, const int lane1, uint64x2_t b, const int lane2);
#define vcopyq_laneq_u64(a, lane1, b, lane2)  __builtin_mpl_vector_copyq_laneq_v2u64(a, lane1, b, lane2)

int8x8_t __builtin_mpl_vector_rbit_v8i8(int8x8_t a);
#define vrbit_s8(a)  __builtin_mpl_vector_rbit_v8i8(a)

int8x16_t __builtin_mpl_vector_rbitq_v16i8(int8x16_t a);
#define vrbitq_s8(a)  __builtin_mpl_vector_rbitq_v16i8(a)

uint8x8_t __builtin_mpl_vector_rbit_v8u8(uint8x8_t a);
#define vrbit_u8(a)  __builtin_mpl_vector_rbit_v8u8(a)

uint8x16_t __builtin_mpl_vector_rbitq_v16u8(uint8x16_t a);
#define vrbitq_u8(a)  __builtin_mpl_vector_rbitq_v16u8(a)

int8x8_t __builtin_mpl_vector_create_v8i8(uint64_t a);
#define vcreate_s8(a)  __builtin_mpl_vector_create_v8i8(a)

int16x4_t __builtin_mpl_vector_create_v4i16(uint64_t a);
#define vcreate_s16(a)  __builtin_mpl_vector_create_v4i16(a)

int32x2_t __builtin_mpl_vector_create_v2i32(uint64_t a);
#define vcreate_s32(a)  __builtin_mpl_vector_create_v2i32(a)

int64x1_t __builtin_mpl_vector_create_v1i64(uint64_t a);
#define vcreate_s64(a)  __builtin_mpl_vector_create_v1i64(a)

uint8x8_t __builtin_mpl_vector_create_v8u8(uint64_t a);
#define vcreate_u8(a)  __builtin_mpl_vector_create_v8u8(a)

uint16x4_t __builtin_mpl_vector_create_v4u16(uint64_t a);
#define vcreate_u16(a)  __builtin_mpl_vector_create_v4u16(a)

uint32x2_t __builtin_mpl_vector_create_v2u32(uint64_t a);
#define vcreate_u32(a)  __builtin_mpl_vector_create_v2u32(a)

uint64x1_t __builtin_mpl_vector_create_v1u64(uint64_t a);
#define vcreate_u64(a)  __builtin_mpl_vector_create_v1u64(a)

int8x8_t __builtin_mpl_vector_mov_n_v8i8(int8_t a);
#define vmov_n_s8(a)  __builtin_mpl_vector_mov_n_v8i8(a)

int8x16_t __builtin_mpl_vector_movq_n_v16i8(int8_t a);
#define vmovq_n_s8(a)  __builtin_mpl_vector_movq_n_v16i8(a)

int16x4_t __builtin_mpl_vector_mov_n_v4i16(int16_t a);
#define vmov_n_s16(a)  __builtin_mpl_vector_mov_n_v4i16(a)

int16x8_t __builtin_mpl_vector_movq_n_v8i16(int16_t a);
#define vmovq_n_s16(a)  __builtin_mpl_vector_movq_n_v8i16(a)

int32x2_t __builtin_mpl_vector_mov_n_v2i32(int32_t a);
#define vmov_n_s32(a)  __builtin_mpl_vector_mov_n_v2i32(a)

int32x4_t __builtin_mpl_vector_movq_n_v4i32(int32_t a);
#define vmovq_n_s32(a)  __builtin_mpl_vector_movq_n_v4i32(a)

int64x1_t __builtin_mpl_vector_mov_n_v1i64(int64_t a);
#define vmov_n_s64(a)  __builtin_mpl_vector_mov_n_v1i64(a)

int64x2_t __builtin_mpl_vector_movq_n_v2i64(int64_t a);
#define vmovq_n_s64(a)  __builtin_mpl_vector_movq_n_v2i64(a)

uint8x8_t __builtin_mpl_vector_mov_n_v8u8(uint8_t a);
#define vmov_n_u8(a)  __builtin_mpl_vector_mov_n_v8u8(a)

uint8x16_t __builtin_mpl_vector_movq_n_v16u8(uint8_t a);
#define vmovq_n_u8(a)  __builtin_mpl_vector_movq_n_v16u8(a)

uint16x4_t __builtin_mpl_vector_mov_n_v4u16(uint16_t a);
#define vmov_n_u16(a)  __builtin_mpl_vector_mov_n_v4u16(a)

uint16x8_t __builtin_mpl_vector_movq_n_v8u16(uint16_t a);
#define vmovq_n_u16(a)  __builtin_mpl_vector_movq_n_v8u16(a)

uint32x2_t __builtin_mpl_vector_mov_n_v2u32(uint32_t a);
#define vmov_n_u32(a)  __builtin_mpl_vector_mov_n_v2u32(a)

uint32x4_t __builtin_mpl_vector_movq_n_v4u32(uint32_t a);
#define vmovq_n_u32(a)  __builtin_mpl_vector_movq_n_v4u32(a)

uint64x1_t __builtin_mpl_vector_mov_n_v1u64(uint64_t a);
#define vmov_n_u64(a)  __builtin_mpl_vector_mov_n_v1u64(a)

uint64x2_t __builtin_mpl_vector_movq_n_v2u64(uint64_t a);
#define vmovq_n_u64(a)  __builtin_mpl_vector_movq_n_v2u64(a)

int8x8_t __builtin_mpl_vector_dup_lane_v8i8(int8x8_t a, const int lane);
#define vdup_lane_s8(a, lane)  __builtin_mpl_vector_dup_lane_v8i8(a, lane)

int8x16_t __builtin_mpl_vector_dupq_lane_v16i8(int8x8_t a, const int lane);
#define vdupq_lane_s8(a, lane)  __builtin_mpl_vector_dupq_lane_v16i8(a, lane)

int16x4_t __builtin_mpl_vector_dup_lane_v4i16(int16x4_t a, const int lane);
#define vdup_lane_s16(a, lane)  __builtin_mpl_vector_dup_lane_v4i16(a, lane)

int16x8_t __builtin_mpl_vector_dupq_lane_v8i16(int16x4_t a, const int lane);
#define vdupq_lane_s16(a, lane)  __builtin_mpl_vector_dupq_lane_v8i16(a, lane)

int32x2_t __builtin_mpl_vector_dup_lane_v2i32(int32x2_t a, const int lane);
#define vdup_lane_s32(a, lane)  __builtin_mpl_vector_dup_lane_v2i32(a, lane)

int32x4_t __builtin_mpl_vector_dupq_lane_v4i32(int32x2_t a, const int lane);
#define vdupq_lane_s32(a, lane)  __builtin_mpl_vector_dupq_lane_v4i32(a, lane)

int64x1_t __builtin_mpl_vector_dup_lane_v1i64(int64x1_t a, const int lane);
#define vdup_lane_s64(a, lane)  __builtin_mpl_vector_dup_lane_v1i64(a, lane)

int64x2_t __builtin_mpl_vector_dupq_lane_v2i64(int64x1_t a, const int lane);
#define vdupq_lane_s64(a, lane)  __builtin_mpl_vector_dupq_lane_v2i64(a, lane)

uint8x8_t __builtin_mpl_vector_dup_lane_v8u8(uint8x8_t a, const int lane);
#define vdup_lane_u8(a, lane)  __builtin_mpl_vector_dup_lane_v8u8(a, lane)

uint8x16_t __builtin_mpl_vector_dupq_lane_v16u8(uint8x8_t a, const int lane);
#define vdupq_lane_u8(a, lane)  __builtin_mpl_vector_dupq_lane_v16u8(a, lane)

uint16x4_t __builtin_mpl_vector_dup_lane_v4u16(uint16x4_t a, const int lane);
#define vdup_lane_u16(a, lane)  __builtin_mpl_vector_dup_lane_v4u16(a, lane)

uint16x8_t __builtin_mpl_vector_dupq_lane_v8u16(uint16x4_t a, const int lane);
#define vdupq_lane_u16(a, lane)  __builtin_mpl_vector_dupq_lane_v8u16(a, lane)

uint32x2_t __builtin_mpl_vector_dup_lane_v2u32(uint32x2_t a, const int lane);
#define vdup_lane_u32(a, lane)  __builtin_mpl_vector_dup_lane_v2u32(a, lane)

uint32x4_t __builtin_mpl_vector_dupq_lane_v4u32(uint32x2_t a, const int lane);
#define vdupq_lane_u32(a, lane)  __builtin_mpl_vector_dupq_lane_v4u32(a, lane)

uint64x1_t __builtin_mpl_vector_dup_lane_v1u64(uint64x1_t a, const int lane);
#define vdup_lane_u64(a, lane)  __builtin_mpl_vector_dup_lane_v1u64(a, lane)

uint64x2_t __builtin_mpl_vector_dupq_lane_v2u64(uint64x1_t a, const int lane);
#define vdupq_lane_u64(a, lane)  __builtin_mpl_vector_dupq_lane_v2u64(a, lane)

int8x8_t __builtin_mpl_vector_dup_laneq_v8i8(int8x16_t a, const int lane);
#define vdup_laneq_s8(a, lane)  __builtin_mpl_vector_dup_laneq_v8i8(a, lane)

int8x16_t __builtin_mpl_vector_dupq_laneq_v16i8(int8x16_t a, const int lane);
#define vdupq_laneq_s8(a, lane)  __builtin_mpl_vector_dupq_laneq_v16i8(a, lane)

int16x4_t __builtin_mpl_vector_dup_laneq_v4i16(int16x8_t a, const int lane);
#define vdup_laneq_s16(a, lane)  __builtin_mpl_vector_dup_laneq_v4i16(a, lane)

int16x8_t __builtin_mpl_vector_dupq_laneq_v8i16(int16x8_t a, const int lane);
#define vdupq_laneq_s16(a, lane)  __builtin_mpl_vector_dupq_laneq_v8i16(a, lane)

int32x2_t __builtin_mpl_vector_dup_laneq_v2i32(int32x4_t a, const int lane);
#define vdup_laneq_s32(a, lane)  __builtin_mpl_vector_dup_laneq_v2i32(a, lane)

int32x4_t __builtin_mpl_vector_dupq_laneq_v4i32(int32x4_t a, const int lane);
#define vdupq_laneq_s32(a, lane)  __builtin_mpl_vector_dupq_laneq_v4i32(a, lane)

int64x1_t __builtin_mpl_vector_dup_laneq_v1i64(int64x2_t a, const int lane);
#define vdup_laneq_s64(a, lane)  __builtin_mpl_vector_dup_laneq_v1i64(a, lane)

int64x2_t __builtin_mpl_vector_dupq_laneq_v2i64(int64x2_t a, const int lane);
#define vdupq_laneq_s64(a, lane)  __builtin_mpl_vector_dupq_laneq_v2i64(a, lane)

uint8x8_t __builtin_mpl_vector_dup_laneq_v8u8(uint8x16_t a, const int lane);
#define vdup_laneq_u8(a, lane)  __builtin_mpl_vector_dup_laneq_v8u8(a, lane)

uint8x16_t __builtin_mpl_vector_dupq_laneq_v16u8(uint8x16_t a, const int lane);
#define vdupq_laneq_u8(a, lane)  __builtin_mpl_vector_dupq_laneq_v16u8(a, lane)

uint16x4_t __builtin_mpl_vector_dup_laneq_v4u16(uint16x8_t a, const int lane);
#define vdup_laneq_u16(a, lane)  __builtin_mpl_vector_dup_laneq_v4u16(a, lane)

uint16x8_t __builtin_mpl_vector_dupq_laneq_v8u16(uint16x8_t a, const int lane);
#define vdupq_laneq_u16(a, lane)  __builtin_mpl_vector_dupq_laneq_v8u16(a, lane)

uint32x2_t __builtin_mpl_vector_dup_laneq_v2u32(uint32x4_t a, const int lane);
#define vdup_laneq_u32(a, lane)  __builtin_mpl_vector_dup_laneq_v2u32(a, lane)

uint32x4_t __builtin_mpl_vector_dupq_laneq_v4u32(uint32x4_t a, const int lane);
#define vdupq_laneq_u32(a, lane)  __builtin_mpl_vector_dupq_laneq_v4u32(a, lane)

uint64x1_t __builtin_mpl_vector_dup_laneq_v1u64(uint64x2_t a, const int lane);
#define vdup_laneq_u64(a, lane)  __builtin_mpl_vector_dup_laneq_v1u64(a, lane)

uint64x2_t __builtin_mpl_vector_dupq_laneq_v2u64(uint64x2_t a, const int lane);
#define vdupq_laneq_u64(a, lane)  __builtin_mpl_vector_dupq_laneq_v2u64(a, lane)

int8x16_t __builtin_mpl_vector_combine_v16i8(int8x8_t a, int8x8_t high);
#define vcombine_s8(a, high)  (int8x16_t)__builtin_mpl_vector_combine_v2i64((int64x1_t)a, high)

int16x8_t __builtin_mpl_vector_combine_v8i16(int16x4_t a, int16x4_t high);
#define vcombine_s16(a, high)  (int16x8_t)__builtin_mpl_vector_combine_v2i64((int64x1_t)a, high)

int32x4_t __builtin_mpl_vector_combine_v4i32(int32x2_t a, int32x2_t high);
#define vcombine_s32(a, high)  (int32x4_t)__builtin_mpl_vector_combine_v2i64((int64x1_t)a, high)

int64x2_t __builtin_mpl_vector_combine_v2i64(int64x1_t a, int64x1_t high);
#define vcombine_s64(a, high)  __builtin_mpl_vector_combine_v2i64(a, high)

uint8x16_t __builtin_mpl_vector_combine_v16u8(uint8x8_t a, uint8x8_t high);
#define vcombine_u8(a, high)  (uint8x16_t)__builtin_mpl_vector_combine_v2u64((uint64x1_t)a, high)

uint16x8_t __builtin_mpl_vector_combine_v8u16(uint16x4_t a, uint16x4_t high);
#define vcombine_u16(a, high)  (uint16x8_t)__builtin_mpl_vector_combine_v2u64((uint64x1_t)a, high)

uint32x4_t __builtin_mpl_vector_combine_v4u32(uint32x2_t a, uint32x2_t high);
#define vcombine_u32(a, high)  (uint32x4_t)__builtin_mpl_vector_combine_v2u64((uint64x1_t)a, high)

uint64x2_t __builtin_mpl_vector_combine_v2u64(uint64x1_t a, uint64x1_t high);
#define vcombine_u64(a, high)  __builtin_mpl_vector_combine_v2u64(a, high)

int8_t __builtin_mpl_vector_dupb_lane_v8i8(int8x8_t a, const int lane);
#define vdupb_lane_s8(a, lane)  __builtin_mpl_vector_dupb_lane_v8i8(a, lane)

int16_t __builtin_mpl_vector_duph_lane_v4i16(int16x4_t a, const int lane);
#define vduph_lane_s16(a, lane)  __builtin_mpl_vector_duph_lane_v4i16(a, lane)

int32_t __builtin_mpl_vector_dups_lane_v2i32(int32x2_t a, const int lane);
#define vdups_lane_s32(a, lane)  __builtin_mpl_vector_dups_lane_v2i32(a, lane)

int64_t __builtin_mpl_vector_dupd_lane_v1i64(int64x1_t a, const int lane);
#define vdupd_lane_s64(a, lane)  __builtin_mpl_vector_dupd_lane_v1i64(a, lane)

uint8_t __builtin_mpl_vector_dupb_lane_v8u8(uint8x8_t a, const int lane);
#define vdupb_lane_u8(a, lane)  __builtin_mpl_vector_dupb_lane_v8u8(a, lane)

uint16_t __builtin_mpl_vector_duph_lane_v4u16(uint16x4_t a, const int lane);
#define vduph_lane_u16(a, lane)  __builtin_mpl_vector_duph_lane_v4u16(a, lane)

uint32_t __builtin_mpl_vector_dups_lane_v2u32(uint32x2_t a, const int lane);
#define vdups_lane_u32(a, lane)  __builtin_mpl_vector_dups_lane_v2u32(a, lane)

uint64_t __builtin_mpl_vector_dupd_lane_v1u64(uint64x1_t a, const int lane);
#define vdupd_lane_u64(a, lane)  __builtin_mpl_vector_dupd_lane_v1u64(a, lane)

int8_t __builtin_mpl_vector_dupb_laneq_v16i8(int8x16_t a, const int lane);
#define vdupb_laneq_s8(a, lane)  __builtin_mpl_vector_dupb_laneq_v16i8(a, lane)

int16_t __builtin_mpl_vector_duph_laneq_v8i16(int16x8_t a, const int lane);
#define vduph_laneq_s16(a, lane)  __builtin_mpl_vector_duph_laneq_v8i16(a, lane)

int32_t __builtin_mpl_vector_dups_laneq_v4i32(int32x4_t a, const int lane);
#define vdups_laneq_s32(a, lane)  __builtin_mpl_vector_dups_laneq_v4i32(a, lane)

int64_t __builtin_mpl_vector_dupd_laneq_v2i64(int64x2_t a, const int lane);
#define vdupd_laneq_s64(a, lane)  __builtin_mpl_vector_dupd_laneq_v2i64(a, lane)

uint8_t __builtin_mpl_vector_dupb_laneq_v16u8(uint8x16_t a, const int lane);
#define vdupb_laneq_u8(a, lane)  __builtin_mpl_vector_dupb_laneq_v16u8(a, lane)

uint16_t __builtin_mpl_vector_duph_laneq_v8u16(uint16x8_t a, const int lane);
#define vduph_laneq_u16(a, lane)  __builtin_mpl_vector_duph_laneq_v8u16(a, lane)

uint32_t __builtin_mpl_vector_dups_laneq_v4u32(uint32x4_t a, const int lane);
#define vdups_laneq_u32(a, lane)  __builtin_mpl_vector_dups_laneq_v4u32(a, lane)

uint64_t __builtin_mpl_vector_dupd_laneq_v2u64(uint64x2_t a, const int lane);
#define vdupd_laneq_u64(a, lane)  __builtin_mpl_vector_dupd_laneq_v2u64(a, lane)

int8x8_t __builtin_mpl_vector_rev64_v8i8(int8x8_t a);
#define vrev64_s8(a)  __builtin_mpl_vector_rev64_v8i8(a)

int8x16_t __builtin_mpl_vector_rev64q_v16i8(int8x16_t a);
#define vrev64q_s8(a)  __builtin_mpl_vector_rev64q_v16i8(a)

int16x4_t __builtin_mpl_vector_rev64_v4i16(int16x4_t a);
#define vrev64_s16(a)  __builtin_mpl_vector_rev64_v4i16(a)

int16x8_t __builtin_mpl_vector_rev64q_v8i16(int16x8_t a);
#define vrev64q_s16(a)  __builtin_mpl_vector_rev64q_v8i16(a)

int32x2_t __builtin_mpl_vector_rev64_v2i32(int32x2_t a);
#define vrev64_s32(a)  __builtin_mpl_vector_rev64_v2i32(a)

int32x4_t __builtin_mpl_vector_rev64q_v4i32(int32x4_t a);
#define vrev64q_s32(a)  __builtin_mpl_vector_rev64q_v4i32(a)

uint8x8_t __builtin_mpl_vector_rev64_v8u8(uint8x8_t a);
#define vrev64_u8(a)  __builtin_mpl_vector_rev64_v8u8(a)

uint8x16_t __builtin_mpl_vector_rev64q_v16u8(uint8x16_t a);
#define vrev64q_u8(a)  __builtin_mpl_vector_rev64q_v16u8(a)

uint16x4_t __builtin_mpl_vector_rev64_v4u16(uint16x4_t a);
#define vrev64_u16(a)  __builtin_mpl_vector_rev64_v4u16(a)

uint16x8_t __builtin_mpl_vector_rev64q_v8u16(uint16x8_t a);
#define vrev64q_u16(a)  __builtin_mpl_vector_rev64q_v8u16(a)

uint32x2_t __builtin_mpl_vector_rev64_v2u32(uint32x2_t a);
#define vrev64_u32(a)  __builtin_mpl_vector_rev64_v2u32(a)

uint32x4_t __builtin_mpl_vector_rev64q_v4u32(uint32x4_t a);
#define vrev64q_u32(a)  __builtin_mpl_vector_rev64q_v4u32(a)

int8x8_t __builtin_mpl_vector_rev16_v8i8(int8x8_t a);
#define vrev16_s8(a)  __builtin_mpl_vector_rev16_v8i8(a)

int8x16_t __builtin_mpl_vector_rev16q_v16i8(int8x16_t a);
#define vrev16q_s8(a)  __builtin_mpl_vector_rev16q_v16i8(a)

uint8x8_t __builtin_mpl_vector_rev16_v8u8(uint8x8_t a);
#define vrev16_u8(a)  __builtin_mpl_vector_rev16_v8u8(a)

uint8x16_t __builtin_mpl_vector_rev16q_v16u8(uint8x16_t a);
#define vrev16q_u8(a)  __builtin_mpl_vector_rev16q_v16u8(a)

int8x8_t __builtin_mpl_vector_zip1_v8i8(int8x8_t a, int8x8_t b);
#define vzip1_s8(a, b)  __builtin_mpl_vector_zip1_v8i8(a, b)

int8x16_t __builtin_mpl_vector_zip1q_v16i8(int8x16_t a, int8x16_t b);
#define vzip1q_s8(a, b)  __builtin_mpl_vector_zip1q_v16i8(a, b)

int16x4_t __builtin_mpl_vector_zip1_v4i16(int16x4_t a, int16x4_t b);
#define vzip1_s16(a, b)  __builtin_mpl_vector_zip1_v4i16(a, b)

int16x8_t __builtin_mpl_vector_zip1q_v8i16(int16x8_t a, int16x8_t b);
#define vzip1q_s16(a, b)  __builtin_mpl_vector_zip1q_v8i16(a, b)

int32x2_t __builtin_mpl_vector_zip1_v2i32(int32x2_t a, int32x2_t b);
#define vzip1_s32(a, b)  __builtin_mpl_vector_zip1_v2i32(a, b)

int32x4_t __builtin_mpl_vector_zip1q_v4i32(int32x4_t a, int32x4_t b);
#define vzip1q_s32(a, b)  __builtin_mpl_vector_zip1q_v4i32(a, b)

int64x2_t __builtin_mpl_vector_zip1q_v2i64(int64x2_t a, int64x2_t b);
#define vzip1q_s64(a, b)  __builtin_mpl_vector_zip1q_v2i64(a, b)

uint8x8_t __builtin_mpl_vector_zip1_v8u8(uint8x8_t a, uint8x8_t b);
#define vzip1_u8(a, b)  __builtin_mpl_vector_zip1_v8u8(a, b)

uint8x16_t __builtin_mpl_vector_zip1q_v16u8(uint8x16_t a, uint8x16_t b);
#define vzip1q_u8(a, b)  __builtin_mpl_vector_zip1q_v16u8(a, b)

uint16x4_t __builtin_mpl_vector_zip1_v4u16(uint16x4_t a, uint16x4_t b);
#define vzip1_u16(a, b)  __builtin_mpl_vector_zip1_v4u16(a, b)

uint16x8_t __builtin_mpl_vector_zip1q_v8u16(uint16x8_t a, uint16x8_t b);
#define vzip1q_u16(a, b)  __builtin_mpl_vector_zip1q_v8u16(a, b)

uint32x2_t __builtin_mpl_vector_zip1_v2u32(uint32x2_t a, uint32x2_t b);
#define vzip1_u32(a, b)  __builtin_mpl_vector_zip1_v2u32(a, b)

uint32x4_t __builtin_mpl_vector_zip1q_v4u32(uint32x4_t a, uint32x4_t b);
#define vzip1q_u32(a, b)  __builtin_mpl_vector_zip1q_v4u32(a, b)

uint64x2_t __builtin_mpl_vector_zip1q_v2u64(uint64x2_t a, uint64x2_t b);
#define vzip1q_u64(a, b)  __builtin_mpl_vector_zip1q_v2u64(a, b)

int8x8_t __builtin_mpl_vector_zip2_v8i8(int8x8_t a, int8x8_t b);
#define vzip2_s8(a, b)  __builtin_mpl_vector_zip2_v8i8(a, b)

int8x16_t __builtin_mpl_vector_zip2q_v16i8(int8x16_t a, int8x16_t b);
#define vzip2q_s8(a, b)  __builtin_mpl_vector_zip2q_v16i8(a, b)

int16x4_t __builtin_mpl_vector_zip2_v4i16(int16x4_t a, int16x4_t b);
#define vzip2_s16(a, b)  __builtin_mpl_vector_zip2_v4i16(a, b)

int16x8_t __builtin_mpl_vector_zip2q_v8i16(int16x8_t a, int16x8_t b);
#define vzip2q_s16(a, b)  __builtin_mpl_vector_zip2q_v8i16(a, b)

int32x2_t __builtin_mpl_vector_zip2_v2i32(int32x2_t a, int32x2_t b);
#define vzip2_s32(a, b)  __builtin_mpl_vector_zip2_v2i32(a, b)

int32x4_t __builtin_mpl_vector_zip2q_v4i32(int32x4_t a, int32x4_t b);
#define vzip2q_s32(a, b)  __builtin_mpl_vector_zip2q_v4i32(a, b)

int64x2_t __builtin_mpl_vector_zip2q_v2i64(int64x2_t a, int64x2_t b);
#define vzip2q_s64(a, b)  __builtin_mpl_vector_zip2q_v2i64(a, b)

uint8x8_t __builtin_mpl_vector_zip2_v8u8(uint8x8_t a, uint8x8_t b);
#define vzip2_u8(a, b)  __builtin_mpl_vector_zip2_v8u8(a, b)

uint8x16_t __builtin_mpl_vector_zip2q_v16u8(uint8x16_t a, uint8x16_t b);
#define vzip2q_u8(a, b)  __builtin_mpl_vector_zip2q_v16u8(a, b)

uint16x4_t __builtin_mpl_vector_zip2_v4u16(uint16x4_t a, uint16x4_t b);
#define vzip2_u16(a, b)  __builtin_mpl_vector_zip2_v4u16(a, b)

uint16x8_t __builtin_mpl_vector_zip2q_v8u16(uint16x8_t a, uint16x8_t b);
#define vzip2q_u16(a, b)  __builtin_mpl_vector_zip2q_v8u16(a, b)

uint32x2_t __builtin_mpl_vector_zip2_v2u32(uint32x2_t a, uint32x2_t b);
#define vzip2_u32(a, b)  __builtin_mpl_vector_zip2_v2u32(a, b)

uint32x4_t __builtin_mpl_vector_zip2q_v4u32(uint32x4_t a, uint32x4_t b);
#define vzip2q_u32(a, b)  __builtin_mpl_vector_zip2q_v4u32(a, b)

uint64x2_t __builtin_mpl_vector_zip2q_v2u64(uint64x2_t a, uint64x2_t b);
#define vzip2q_u64(a, b)  __builtin_mpl_vector_zip2q_v2u64(a, b)

#define vzip_s8(a, b)  (int8x8x2_t){vzip1_s8((a), (b)), vzip2_s8((a), (b))}
#define vzip_s16(a, b)  (int16x4x2_t){vzip1_s16((a), (b)), vzip2_s16((a), (b))}
#define vzip_u8(a, b)  (uint8x8x2_t){vzip1_u8((a), (b)), vzip2_u8((a), (b))}
#define vzip_u16(a, b)  (uint16x4x2_t){vzip1_u16((a), (b)), vzip2_u16((a), (b))}
#define vzip_s32(a, b)  (int32x2x2_t){vzip1_s32((a), (b)), vzip2_s32((a), (b))}
#define vzip_u32(a, b)  (uint32x2x2_t){vzip1_u32((a), (b)), vzip2_u32((a), (b))}
#define vzipq_s8(a, b)  (int8x16x2_t){vzip1q_s8((a), (b)), vzip2q_s8((a), (b))}
#define vzipq_s16(a, b)  (int16x8x2_t){vzip1q_s16((a), (b)), vzip2q_s16((a), (b))}
#define vzipq_s32(a, b)  (int32x4x2_t){vzip1q_s32((a), (b)), vzip2q_s32((a), (b))}
#define vzipq_u8(a, b)  (uint8x16x2_t){vzip1q_u8((a), (b)), vzip2q_u8((a), (b))}
#define vzipq_u16(a, b)  (uint16x8x2_t){vzip1q_u16((a), (b)), vzip2q_u16((a), (b))}
#define vzipq_u32(a, b)  (uint32x4x2_t){vzip1q_u32((a), (b)), vzip2q_u32((a), (b))}

int8x8_t __builtin_mpl_vector_uzp1_v8i8(int8x8_t a, int8x8_t b);
#define vuzp1_s8(a, b)  __builtin_mpl_vector_uzp1_v8i8(a, b)

int8x16_t __builtin_mpl_vector_uzp1q_v16i8(int8x16_t a, int8x16_t b);
#define vuzp1q_s8(a, b)  __builtin_mpl_vector_uzp1q_v16i8(a, b)

int16x4_t __builtin_mpl_vector_uzp1_v4i16(int16x4_t a, int16x4_t b);
#define vuzp1_s16(a, b)  __builtin_mpl_vector_uzp1_v4i16(a, b)

int16x8_t __builtin_mpl_vector_uzp1q_v8i16(int16x8_t a, int16x8_t b);
#define vuzp1q_s16(a, b)  __builtin_mpl_vector_uzp1q_v8i16(a, b)

int32x2_t __builtin_mpl_vector_uzp1_v2i32(int32x2_t a, int32x2_t b);
#define vuzp1_s32(a, b)  __builtin_mpl_vector_uzp1_v2i32(a, b)

int32x4_t __builtin_mpl_vector_uzp1q_v4i32(int32x4_t a, int32x4_t b);
#define vuzp1q_s32(a, b)  __builtin_mpl_vector_uzp1q_v4i32(a, b)

int64x2_t __builtin_mpl_vector_uzp1q_v2i64(int64x2_t a, int64x2_t b);
#define vuzp1q_s64(a, b)  __builtin_mpl_vector_uzp1q_v2i64(a, b)

uint8x8_t __builtin_mpl_vector_uzp1_v8u8(uint8x8_t a, uint8x8_t b);
#define vuzp1_u8(a, b)  __builtin_mpl_vector_uzp1_v8u8(a, b)

uint8x16_t __builtin_mpl_vector_uzp1q_v16u8(uint8x16_t a, uint8x16_t b);
#define vuzp1q_u8(a, b)  __builtin_mpl_vector_uzp1q_v16u8(a, b)

uint16x4_t __builtin_mpl_vector_uzp1_v4u16(uint16x4_t a, uint16x4_t b);
#define vuzp1_u16(a, b)  __builtin_mpl_vector_uzp1_v4u16(a, b)

uint16x8_t __builtin_mpl_vector_uzp1q_v8u16(uint16x8_t a, uint16x8_t b);
#define vuzp1q_u16(a, b)  __builtin_mpl_vector_uzp1q_v8u16(a, b)

uint32x2_t __builtin_mpl_vector_uzp1_v2u32(uint32x2_t a, uint32x2_t b);
#define vuzp1_u32(a, b)  __builtin_mpl_vector_uzp1_v2u32(a, b)

uint32x4_t __builtin_mpl_vector_uzp1q_v4u32(uint32x4_t a, uint32x4_t b);
#define vuzp1q_u32(a, b)  __builtin_mpl_vector_uzp1q_v4u32(a, b)

uint64x2_t __builtin_mpl_vector_uzp1q_v2u64(uint64x2_t a, uint64x2_t b);
#define vuzp1q_u64(a, b)  __builtin_mpl_vector_uzp1q_v2u64(a, b)

int8x8_t __builtin_mpl_vector_uzp2_v8i8(int8x8_t a, int8x8_t b);
#define vuzp2_s8(a, b)  __builtin_mpl_vector_uzp2_v8i8(a, b)

int8x16_t __builtin_mpl_vector_uzp2q_v16i8(int8x16_t a, int8x16_t b);
#define vuzp2q_s8(a, b)  __builtin_mpl_vector_uzp2q_v16i8(a, b)

int16x4_t __builtin_mpl_vector_uzp2_v4i16(int16x4_t a, int16x4_t b);
#define vuzp2_s16(a, b)  __builtin_mpl_vector_uzp2_v4i16(a, b)

int16x8_t __builtin_mpl_vector_uzp2q_v8i16(int16x8_t a, int16x8_t b);
#define vuzp2q_s16(a, b)  __builtin_mpl_vector_uzp2q_v8i16(a, b)

int32x2_t __builtin_mpl_vector_uzp2_v2i32(int32x2_t a, int32x2_t b);
#define vuzp2_s32(a, b)  __builtin_mpl_vector_uzp2_v2i32(a, b)

int32x4_t __builtin_mpl_vector_uzp2q_v4i32(int32x4_t a, int32x4_t b);
#define vuzp2q_s32(a, b)  __builtin_mpl_vector_uzp2q_v4i32(a, b)

int64x2_t __builtin_mpl_vector_uzp2q_v2i64(int64x2_t a, int64x2_t b);
#define vuzp2q_s64(a, b)  __builtin_mpl_vector_uzp2q_v2i64(a, b)

uint8x8_t __builtin_mpl_vector_uzp2_v8u8(uint8x8_t a, uint8x8_t b);
#define vuzp2_u8(a, b)  __builtin_mpl_vector_uzp2_v8u8(a, b)

uint8x16_t __builtin_mpl_vector_uzp2q_v16u8(uint8x16_t a, uint8x16_t b);
#define vuzp2q_u8(a, b)  __builtin_mpl_vector_uzp2q_v16u8(a, b)

uint16x4_t __builtin_mpl_vector_uzp2_v4u16(uint16x4_t a, uint16x4_t b);
#define vuzp2_u16(a, b)  __builtin_mpl_vector_uzp2_v4u16(a, b)

uint16x8_t __builtin_mpl_vector_uzp2q_v8u16(uint16x8_t a, uint16x8_t b);
#define vuzp2q_u16(a, b)  __builtin_mpl_vector_uzp2q_v8u16(a, b)

uint32x2_t __builtin_mpl_vector_uzp2_v2u32(uint32x2_t a, uint32x2_t b);
#define vuzp2_u32(a, b)  __builtin_mpl_vector_uzp2_v2u32(a, b)

uint32x4_t __builtin_mpl_vector_uzp2q_v4u32(uint32x4_t a, uint32x4_t b);
#define vuzp2q_u32(a, b)  __builtin_mpl_vector_uzp2q_v4u32(a, b)

uint64x2_t __builtin_mpl_vector_uzp2q_v2u64(uint64x2_t a, uint64x2_t b);
#define vuzp2q_u64(a, b)  __builtin_mpl_vector_uzp2q_v2u64(a, b)

#define vuzp_s8(a, b)  (int8x8x2_t){vuzp1_s8((a), (b)), vuzp2_s8((a), (b))}
#define vuzp_s16(a, b)  (int16x4x2_t){vuzp1_s16((a), (b)), vuzp2_s16((a), (b))}
#define vuzp_s32(a, b)  (int32x2x2_t){vuzp1_s32((a), (b)), vuzp2_s32((a), (b))}
#define vuzp_u8(a, b)  (uint8x8x2_t){vuzp1_u8((a), (b)), vuzp2_u8((a), (b))}
#define vuzp_u16(a, b)  (uint16x4x2_t){vuzp1_u16((a), (b)), vuzp2_u16((a), (b))}
#define vuzp_u32(a, b)  (uint32x2x2_t){vuzp1_u32((a), (b)), vuzp2_u32((a), (b))}
#define vuzpq_s8(a, b)  (int8x16x2_t){vuzp1q_s8((a), (b)), vuzp2q_s8((a), (b))}
#define vuzpq_s16(a, b)  (int16x8x2_t){vuzp1q_s16((a), (b)), vuzp2q_s16((a), (b))}
#define vuzpq_s32(a, b)  (int32x4x2_t){vuzp1q_s32((a), (b)), vuzp2q_s32((a), (b))}
#define vuzpq_u8(a, b)  (uint8x16x2_t){vuzp1q_u8((a), (b)), vuzp2q_u8((a), (b))}
#define vuzpq_u16(a, b)  (uint16x8x2_t){vuzp1q_u16((a), (b)), vuzp2q_u16((a), (b))}
#define vuzpq_u32(a, b)  (uint32x4x2_t){vuzp1q_u32((a), (b)), vuzp2q_u32((a), (b))}

int8x8_t __builtin_mpl_vector_trn1_v8i8(int8x8_t a, int8x8_t b);
#define vtrn1_s8(a, b)  __builtin_mpl_vector_trn1_v8i8(a, b)

int8x16_t __builtin_mpl_vector_trn1q_v16i8(int8x16_t a, int8x16_t b);
#define vtrn1q_s8(a, b)  __builtin_mpl_vector_trn1q_v16i8(a, b)

int16x4_t __builtin_mpl_vector_trn1_v4i16(int16x4_t a, int16x4_t b);
#define vtrn1_s16(a, b)  __builtin_mpl_vector_trn1_v4i16(a, b)

int16x8_t __builtin_mpl_vector_trn1q_v8i16(int16x8_t a, int16x8_t b);
#define vtrn1q_s16(a, b)  __builtin_mpl_vector_trn1q_v8i16(a, b)

int32x2_t __builtin_mpl_vector_trn1_v2i32(int32x2_t a, int32x2_t b);
#define vtrn1_s32(a, b)  __builtin_mpl_vector_trn1_v2i32(a, b)

int32x4_t __builtin_mpl_vector_trn1q_v4i32(int32x4_t a, int32x4_t b);
#define vtrn1q_s32(a, b)  __builtin_mpl_vector_trn1q_v4i32(a, b)

int64x2_t __builtin_mpl_vector_trn1q_v2i64(int64x2_t a, int64x2_t b);
#define vtrn1q_s64(a, b)  __builtin_mpl_vector_trn1q_v2i64(a, b)

uint8x8_t __builtin_mpl_vector_trn1_v8u8(uint8x8_t a, uint8x8_t b);
#define vtrn1_u8(a, b)  __builtin_mpl_vector_trn1_v8u8(a, b)

uint8x16_t __builtin_mpl_vector_trn1q_v16u8(uint8x16_t a, uint8x16_t b);
#define vtrn1q_u8(a, b)  __builtin_mpl_vector_trn1q_v16u8(a, b)

uint16x4_t __builtin_mpl_vector_trn1_v4u16(uint16x4_t a, uint16x4_t b);
#define vtrn1_u16(a, b)  __builtin_mpl_vector_trn1_v4u16(a, b)

uint16x8_t __builtin_mpl_vector_trn1q_v8u16(uint16x8_t a, uint16x8_t b);
#define vtrn1q_u16(a, b)  __builtin_mpl_vector_trn1q_v8u16(a, b)

uint32x2_t __builtin_mpl_vector_trn1_v2u32(uint32x2_t a, uint32x2_t b);
#define vtrn1_u32(a, b)  __builtin_mpl_vector_trn1_v2u32(a, b)

uint32x4_t __builtin_mpl_vector_trn1q_v4u32(uint32x4_t a, uint32x4_t b);
#define vtrn1q_u32(a, b)  __builtin_mpl_vector_trn1q_v4u32(a, b)

uint64x2_t __builtin_mpl_vector_trn1q_v2u64(uint64x2_t a, uint64x2_t b);
#define vtrn1q_u64(a, b)  __builtin_mpl_vector_trn1q_v2u64(a, b)

int8x8_t __builtin_mpl_vector_trn2_v8i8(int8x8_t a, int8x8_t b);
#define vtrn2_s8(a, b)  __builtin_mpl_vector_trn2_v8i8(a, b)

int8x16_t __builtin_mpl_vector_trn2q_v16i8(int8x16_t a, int8x16_t b);
#define vtrn2q_s8(a, b)  __builtin_mpl_vector_trn2q_v16i8(a, b)

int16x4_t __builtin_mpl_vector_trn2_v4i16(int16x4_t a, int16x4_t b);
#define vtrn2_s16(a, b)  __builtin_mpl_vector_trn2_v4i16(a, b)

int16x8_t __builtin_mpl_vector_trn2q_v8i16(int16x8_t a, int16x8_t b);
#define vtrn2q_s16(a, b)  __builtin_mpl_vector_trn2q_v8i16(a, b)

int32x2_t __builtin_mpl_vector_trn2_v2i32(int32x2_t a, int32x2_t b);
#define vtrn2_s32(a, b)  __builtin_mpl_vector_trn2_v2i32(a, b)

int32x4_t __builtin_mpl_vector_trn2q_v4i32(int32x4_t a, int32x4_t b);
#define vtrn2q_s32(a, b)  __builtin_mpl_vector_trn2q_v4i32(a, b)

int64x2_t __builtin_mpl_vector_trn2q_v2i64(int64x2_t a, int64x2_t b);
#define vtrn2q_s64(a, b)  __builtin_mpl_vector_trn2q_v2i64(a, b)

uint8x8_t __builtin_mpl_vector_trn2_v8u8(uint8x8_t a, uint8x8_t b);
#define vtrn2_u8(a, b)  __builtin_mpl_vector_trn2_v8u8(a, b)

uint8x16_t __builtin_mpl_vector_trn2q_v16u8(uint8x16_t a, uint8x16_t b);
#define vtrn2q_u8(a, b)  __builtin_mpl_vector_trn2q_v16u8(a, b)

uint16x4_t __builtin_mpl_vector_trn2_v4u16(uint16x4_t a, uint16x4_t b);
#define vtrn2_u16(a, b)  __builtin_mpl_vector_trn2_v4u16(a, b)

uint16x8_t __builtin_mpl_vector_trn2q_v8u16(uint16x8_t a, uint16x8_t b);
#define vtrn2q_u16(a, b)  __builtin_mpl_vector_trn2q_v8u16(a, b)

uint32x2_t __builtin_mpl_vector_trn2_v2u32(uint32x2_t a, uint32x2_t b);
#define vtrn2_u32(a, b)  __builtin_mpl_vector_trn2_v2u32(a, b)

uint32x4_t __builtin_mpl_vector_trn2q_v4u32(uint32x4_t a, uint32x4_t b);
#define vtrn2q_u32(a, b)  __builtin_mpl_vector_trn2q_v4u32(a, b)

uint64x2_t __builtin_mpl_vector_trn2q_v2u64(uint64x2_t a, uint64x2_t b);
#define vtrn2q_u64(a, b)  __builtin_mpl_vector_trn2q_v2u64(a, b)

#define vtrn_s8(a, b)  (int8x8x2_t){vtrn1_s8((a), (b)), vtrn2_s8((a), (b))}
#define vtrn_s16(a, b)  (int16x4x2_t){vtrn1_s16((a), (b)), vtrn2_s16((a), (b))}
#define vtrn_s32(a, b)  (int32x2x2_t){vtrn1_s32((a), (b)), vtrn2_s32((a), (b))}
#define vtrn_u8(a, b)  (uint8x8x2_t){vtrn1_u8((a), (b)), vtrn2_u8((a), (b))}
#define vtrn_u16(a, b)  (uint16x4x2_t){vtrn1_u16((a), (b)), vtrn2_u16((a), (b))}
#define vtrn_u32(a, b)  (uint32x2x2_t){vtrn1_u32((a), (b)), vtrn2_u32((a), (b))}
#define vtrnq_s8(a, b)  (int8x16x2_t){vtrn1q_s8((a), (b)), vtrn2q_s8((a), (b))}
#define vtrnq_s16(a, b)  (int16x8x2_t){vtrn1q_s16((a), (b)), vtrn2q_s16((a), (b))}
#define vtrnq_s32(a, b)  (int32x4x2_t){vtrn1q_s32((a), (b)), vtrn2q_s32((a), (b))}
#define vtrnq_u8(a, b)  (uint8x16x2_t){vtrn1q_u8((a), (b)), vtrn2q_u8((a), (b))}
#define vtrnq_u16(a, b)  (uint16x8x2_t){vtrn1q_u16((a), (b)), vtrn2q_u16((a), (b))}
#define vtrnq_u32(a, b)  (uint32x4x2_t){vtrn1q_u32((a), (b)), vtrn2q_u32((a), (b))}

int8x8_t __builtin_mpl_vector_ld1_i8v8(int8_t const *ptr);
#define vld1_s8(ptr)  __builtin_mpl_vector_ld1_i8v8(ptr)

int8x16_t __builtin_mpl_vector_ld1q_i8v16(int8_t const *ptr);
#define vld1q_s8(ptr)  __builtin_mpl_vector_ld1q_i8v16(ptr)

int16x4_t __builtin_mpl_vector_ld1_i16v4(int16_t const *ptr);
#define vld1_s16(ptr)  __builtin_mpl_vector_ld1_i16v4(ptr)

int16x8_t __builtin_mpl_vector_ld1q_i16v8(int16_t const *ptr);
#define vld1q_s16(ptr)  __builtin_mpl_vector_ld1q_i16v8(ptr)

int32x2_t __builtin_mpl_vector_ld1_i32v2(int32_t const *ptr);
#define vld1_s32(ptr)  __builtin_mpl_vector_ld1_i32v2(ptr)

int32x4_t __builtin_mpl_vector_ld1q_i32v4(int32_t const *ptr);
#define vld1q_s32(ptr)  __builtin_mpl_vector_ld1q_i32v4(ptr)

int64x1_t __builtin_mpl_vector_ld1_i64v1(int64_t const *ptr);
#define vld1_s64(ptr)  __builtin_mpl_vector_ld1_i64v1(ptr)

int64x2_t __builtin_mpl_vector_ld1q_i64v2(int64_t const *ptr);
#define vld1q_s64(ptr)  __builtin_mpl_vector_ld1q_i64v2(ptr)

uint8x8_t __builtin_mpl_vector_ld1_u8v8(uint8_t const *ptr);
#define vld1_u8(ptr)  __builtin_mpl_vector_ld1_u8v8(ptr)

uint8x16_t __builtin_mpl_vector_ld1q_u8v16(uint8_t const *ptr);
#define vld1q_u8(ptr)  __builtin_mpl_vector_ld1q_u8v16(ptr)

uint16x4_t __builtin_mpl_vector_ld1_u16v4(uint16_t const *ptr);
#define vld1_u16(ptr)  __builtin_mpl_vector_ld1_u16v4(ptr)

uint16x8_t __builtin_mpl_vector_ld1q_u16v8(uint16_t const *ptr);
#define vld1q_u16(ptr)  __builtin_mpl_vector_ld1q_u16v8(ptr)

uint32x2_t __builtin_mpl_vector_ld1_u32v2(uint32_t const *ptr);
#define vld1_u32(ptr)  __builtin_mpl_vector_ld1_u32v2(ptr)

uint32x4_t __builtin_mpl_vector_ld1q_u32v4(uint32_t const *ptr);
#define vld1q_u32(ptr)  __builtin_mpl_vector_ld1q_u32v4(ptr)

uint64x1_t __builtin_mpl_vector_ld1_u64v1(uint64_t const *ptr);
#define vld1_u64(ptr)  __builtin_mpl_vector_ld1_u64v1(ptr)

uint64x2_t __builtin_mpl_vector_ld1q_u64v2(uint64_t const *ptr);
#define vld1q_u64(ptr)  __builtin_mpl_vector_ld1q_u64v2(ptr)

int8x8_t __builtin_mpl_vector_ld1_lane_i8v8(int8_t const *ptr, int8x8_t src, const int lane);
#define vld1_lane_s8(ptr, src, lane)  __builtin_mpl_vector_ld1_lane_i8v8(ptr, src, lane)

int8x16_t __builtin_mpl_vector_ld1q_lane_i8v16(int8_t const *ptr, int8x16_t src, const int lane);
#define vld1q_lane_s8(ptr, src, lane)  __builtin_mpl_vector_ld1q_lane_i8v16(ptr, src, lane)

int16x4_t __builtin_mpl_vector_ld1_lane_i16v4(int16_t const *ptr, int16x4_t src, const int lane);
#define vld1_lane_s16(ptr, src, lane)  __builtin_mpl_vector_ld1_lane_i16v4(ptr, src, lane)

int16x8_t __builtin_mpl_vector_ld1q_lane_i16v8(int16_t const *ptr, int16x8_t src, const int lane);
#define vld1q_lane_s16(ptr, src, lane)  __builtin_mpl_vector_ld1q_lane_i16v8(ptr, src, lane)

int32x2_t __builtin_mpl_vector_ld1_lane_i32v2(int32_t const *ptr, int32x2_t src, const int lane);
#define vld1_lane_s32(ptr, src, lane)  __builtin_mpl_vector_ld1_lane_i32v2(ptr, src, lane)

int32x4_t __builtin_mpl_vector_ld1q_lane_i32v4(int32_t const *ptr, int32x4_t src, const int lane);
#define vld1q_lane_s32(ptr, src, lane)  __builtin_mpl_vector_ld1q_lane_i32v4(ptr, src, lane)

int64x1_t __builtin_mpl_vector_ld1_lane_i64v1(int64_t const *ptr, int64x1_t src, const int lane);
#define vld1_lane_s64(ptr, src, lane)  __builtin_mpl_vector_ld1_lane_i64v1(ptr, src, lane)

int64x2_t __builtin_mpl_vector_ld1q_lane_i64v2(int64_t const *ptr, int64x2_t src, const int lane);
#define vld1q_lane_s64(ptr, src, lane)  __builtin_mpl_vector_ld1q_lane_i64v2(ptr, src, lane)

uint8x8_t __builtin_mpl_vector_ld1_lane_u8v8(uint8_t const *ptr, uint8x8_t src, const int lane);
#define vld1_lane_u8(ptr, src, lane)  __builtin_mpl_vector_ld1_lane_u8v8(ptr, src, lane)

uint8x16_t __builtin_mpl_vector_ld1q_lane_u8v16(uint8_t const *ptr, uint8x16_t src, const int lane);
#define vld1q_lane_u8(ptr, src, lane)  __builtin_mpl_vector_ld1q_lane_u8v16(ptr, src, lane)

uint16x4_t __builtin_mpl_vector_ld1_lane_u16v4(uint16_t const *ptr, uint16x4_t src, const int lane);
#define vld1_lane_u16(ptr, src, lane)  __builtin_mpl_vector_ld1_lane_u16v4(ptr, src, lane)

uint16x8_t __builtin_mpl_vector_ld1q_lane_u16v8(uint16_t const *ptr, uint16x8_t src, const int lane);
#define vld1q_lane_u16(ptr, src, lane)  __builtin_mpl_vector_ld1q_lane_u16v8(ptr, src, lane)

uint32x2_t __builtin_mpl_vector_ld1_lane_u32v2(uint32_t const *ptr, uint32x2_t src, const int lane);
#define vld1_lane_u32(ptr, src, lane)  __builtin_mpl_vector_ld1_lane_u32v2(ptr, src, lane)

uint32x4_t __builtin_mpl_vector_ld1q_lane_u32v4(uint32_t const *ptr, uint32x4_t src, const int lane);
#define vld1q_lane_u32(ptr, src, lane)  __builtin_mpl_vector_ld1q_lane_u32v4(ptr, src, lane)

uint64x1_t __builtin_mpl_vector_ld1_lane_u64v1(uint64_t const *ptr, uint64x1_t src, const int lane);
#define vld1_lane_u64(ptr, src, lane)  __builtin_mpl_vector_ld1_lane_u64v1(ptr, src, lane)

uint64x2_t __builtin_mpl_vector_ld1q_lane_u64v2(uint64_t const *ptr, uint64x2_t src, const int lane);
#define vld1q_lane_u64(ptr, src, lane)  __builtin_mpl_vector_ld1q_lane_u64v2(ptr, src, lane)

int8x8_t __builtin_mpl_vector_ld1_dup_i8v8(int8_t const *ptr);
#define vld1_dup_s8(ptr)  __builtin_mpl_vector_ld1_dup_i8v8(ptr)

int8x16_t __builtin_mpl_vector_ld1q_dup_i8v16(int8_t const *ptr);
#define vld1q_dup_s8(ptr)  __builtin_mpl_vector_ld1q_dup_i8v16(ptr)

int16x4_t __builtin_mpl_vector_ld1_dup_i16v4(int16_t const *ptr);
#define vld1_dup_s16(ptr)  __builtin_mpl_vector_ld1_dup_i16v4(ptr)

int16x8_t __builtin_mpl_vector_ld1q_dup_i16v8(int16_t const *ptr);
#define vld1q_dup_s16(ptr)  __builtin_mpl_vector_ld1q_dup_i16v8(ptr)

int32x2_t __builtin_mpl_vector_ld1_dup_i32v2(int32_t const *ptr);
#define vld1_dup_s32(ptr)  __builtin_mpl_vector_ld1_dup_i32v2(ptr)

int32x4_t __builtin_mpl_vector_ld1q_dup_i32v4(int32_t const *ptr);
#define vld1q_dup_s32(ptr)  __builtin_mpl_vector_ld1q_dup_i32v4(ptr)

int64x1_t __builtin_mpl_vector_ld1_dup_i64v1(int64_t const *ptr);
#define vld1_dup_s64(ptr)  __builtin_mpl_vector_ld1_dup_i64v1(ptr)

int64x2_t __builtin_mpl_vector_ld1q_dup_i64v2(int64_t const *ptr);
#define vld1q_dup_s64(ptr)  __builtin_mpl_vector_ld1q_dup_i64v2(ptr)

uint8x8_t __builtin_mpl_vector_ld1_dup_u8v8(uint8_t const *ptr);
#define vld1_dup_u8(ptr)  __builtin_mpl_vector_ld1_dup_u8v8(ptr)

uint8x16_t __builtin_mpl_vector_ld1q_dup_u8v16(uint8_t const *ptr);
#define vld1q_dup_u8(ptr)  __builtin_mpl_vector_ld1q_dup_u8v16(ptr)

uint16x4_t __builtin_mpl_vector_ld1_dup_u16v4(uint16_t const *ptr);
#define vld1_dup_u16(ptr)  __builtin_mpl_vector_ld1_dup_u16v4(ptr)

uint16x8_t __builtin_mpl_vector_ld1q_dup_u16v8(uint16_t const *ptr);
#define vld1q_dup_u16(ptr)  __builtin_mpl_vector_ld1q_dup_u16v8(ptr)

uint32x2_t __builtin_mpl_vector_ld1_dup_u32v2(uint32_t const *ptr);
#define vld1_dup_u32(ptr)  __builtin_mpl_vector_ld1_dup_u32v2(ptr)

uint32x4_t __builtin_mpl_vector_ld1q_dup_u32v4(uint32_t const *ptr);
#define vld1q_dup_u32(ptr)  __builtin_mpl_vector_ld1q_dup_u32v4(ptr)

uint64x1_t __builtin_mpl_vector_ld1_dup_u64v1(uint64_t const *ptr);
#define vld1_dup_u64(ptr)  __builtin_mpl_vector_ld1_dup_u64v1(ptr)

uint64x2_t __builtin_mpl_vector_ld1q_dup_u64v2(uint64_t const *ptr);
#define vld1q_dup_u64(ptr)  __builtin_mpl_vector_ld1q_dup_u64v2(ptr)

int8x8x2_t __builtin_mpl_vector_ld2_i8v8(int8_t const *ptr);
#define vld2_s8(ptr)  __builtin_mpl_vector_ld2_i8v8(ptr)

int8x16x2_t __builtin_mpl_vector_ld2q_i8v16(int8_t const *ptr);
#define vld2q_s8(ptr)  __builtin_mpl_vector_ld2q_i8v16(ptr)

int16x4x2_t __builtin_mpl_vector_ld2_i16v4(int16_t const *ptr);
#define vld2_s16(ptr)  __builtin_mpl_vector_ld2_i16v4(ptr)

int16x8x2_t __builtin_mpl_vector_ld2q_i16v8(int16_t const *ptr);
#define vld2q_s16(ptr)  __builtin_mpl_vector_ld2q_i16v8(ptr)

int32x2x2_t __builtin_mpl_vector_ld2_i32v2(int32_t const *ptr);
#define vld2_s32(ptr)  __builtin_mpl_vector_ld2_i32v2(ptr)

int32x4x2_t __builtin_mpl_vector_ld2q_i32v4(int32_t const *ptr);
#define vld2q_s32(ptr)  __builtin_mpl_vector_ld2q_i32v4(ptr)

uint8x8x2_t __builtin_mpl_vector_ld2_u8v8(uint8_t const *ptr);
#define vld2_u8(ptr)  __builtin_mpl_vector_ld2_u8v8(ptr)

uint8x16x2_t __builtin_mpl_vector_ld2q_u8v16(uint8_t const *ptr);
#define vld2q_u8(ptr)  __builtin_mpl_vector_ld2q_u8v16(ptr)

uint16x4x2_t __builtin_mpl_vector_ld2_u16v4(uint16_t const *ptr);
#define vld2_u16(ptr)  __builtin_mpl_vector_ld2_u16v4(ptr)

uint16x8x2_t __builtin_mpl_vector_ld2q_u16v8(uint16_t const *ptr);
#define vld2q_u16(ptr)  __builtin_mpl_vector_ld2q_u16v8(ptr)

uint32x2x2_t __builtin_mpl_vector_ld2_u32v2(uint32_t const *ptr);
#define vld2_u32(ptr)  __builtin_mpl_vector_ld2_u32v2(ptr)

uint32x4x2_t __builtin_mpl_vector_ld2q_u32v4(uint32_t const *ptr);
#define vld2q_u32(ptr)  __builtin_mpl_vector_ld2q_u32v4(ptr)

int64x1x2_t __builtin_mpl_vector_ld2_i64v1(int64_t const *ptr);
#define vld2_s64(ptr)  __builtin_mpl_vector_ld2_i64v1(ptr)

uint64x1x2_t __builtin_mpl_vector_ld2_u64v1(uint64_t const *ptr);
#define vld2_u64(ptr)  __builtin_mpl_vector_ld2_u64v1(ptr)

int64x2x2_t __builtin_mpl_vector_ld2q_i64v2(int64_t const *ptr);
#define vld2q_s64(ptr)  __builtin_mpl_vector_ld2q_i64v2(ptr)

uint64x2x2_t __builtin_mpl_vector_ld2q_u64v2(uint64_t const *ptr);
#define vld2q_u64(ptr)  __builtin_mpl_vector_ld2q_u64v2(ptr)

int8x8x3_t __builtin_mpl_vector_ld3_i8v8(int8_t const *ptr);
#define vld3_s8(ptr)  __builtin_mpl_vector_ld3_i8v8(ptr)

int8x16x3_t __builtin_mpl_vector_ld3q_i8v16(int8_t const *ptr);
#define vld3q_s8(ptr)  __builtin_mpl_vector_ld3q_i8v16(ptr)

int16x4x3_t __builtin_mpl_vector_ld3_i16v4(int16_t const *ptr);
#define vld3_s16(ptr)  __builtin_mpl_vector_ld3_i16v4(ptr)

int16x8x3_t __builtin_mpl_vector_ld3q_i16v8(int16_t const *ptr);
#define vld3q_s16(ptr)  __builtin_mpl_vector_ld3q_i16v8(ptr)

int32x2x3_t __builtin_mpl_vector_ld3_i32v2(int32_t const *ptr);
#define vld3_s32(ptr)  __builtin_mpl_vector_ld3_i32v2(ptr)

int32x4x3_t __builtin_mpl_vector_ld3q_i32v4(int32_t const *ptr);
#define vld3q_s32(ptr)  __builtin_mpl_vector_ld3q_i32v4(ptr)

uint8x8x3_t __builtin_mpl_vector_ld3_u8v8(uint8_t const *ptr);
#define vld3_u8(ptr)  __builtin_mpl_vector_ld3_u8v8(ptr)

uint8x16x3_t __builtin_mpl_vector_ld3q_u8v16(uint8_t const *ptr);
#define vld3q_u8(ptr)  __builtin_mpl_vector_ld3q_u8v16(ptr)

uint16x4x3_t __builtin_mpl_vector_ld3_u16v4(uint16_t const *ptr);
#define vld3_u16(ptr)  __builtin_mpl_vector_ld3_u16v4(ptr)

uint16x8x3_t __builtin_mpl_vector_ld3q_u16v8(uint16_t const *ptr);
#define vld3q_u16(ptr)  __builtin_mpl_vector_ld3q_u16v8(ptr)

uint32x2x3_t __builtin_mpl_vector_ld3_u32v2(uint32_t const *ptr);
#define vld3_u32(ptr)  __builtin_mpl_vector_ld3_u32v2(ptr)

uint32x4x3_t __builtin_mpl_vector_ld3q_u32v4(uint32_t const *ptr);
#define vld3q_u32(ptr)  __builtin_mpl_vector_ld3q_u32v4(ptr)

int64x1x3_t __builtin_mpl_vector_ld3_i64v1(int64_t const *ptr);
#define vld3_s64(ptr)  __builtin_mpl_vector_ld3_i64v1(ptr)

uint64x1x3_t __builtin_mpl_vector_ld3_u64v1(uint64_t const *ptr);
#define vld3_u64(ptr)  __builtin_mpl_vector_ld3_u64v1(ptr)

int64x2x3_t __builtin_mpl_vector_ld3q_i64v2(int64_t const *ptr);
#define vld3q_s64(ptr)  __builtin_mpl_vector_ld3q_i64v2(ptr)

uint64x2x3_t __builtin_mpl_vector_ld3q_u64v2(uint64_t const *ptr);
#define vld3q_u64(ptr)  __builtin_mpl_vector_ld3q_u64v2(ptr)

int8x8x4_t __builtin_mpl_vector_ld4_i8v8(int8_t const *ptr);
#define vld4_s8(ptr)  __builtin_mpl_vector_ld4_i8v8(ptr)

int8x16x4_t __builtin_mpl_vector_ld4q_i8v16(int8_t const *ptr);
#define vld4q_s8(ptr)  __builtin_mpl_vector_ld4q_i8v16(ptr)

int16x4x4_t __builtin_mpl_vector_ld4_i16v4(int16_t const *ptr);
#define vld4_s16(ptr)  __builtin_mpl_vector_ld4_i16v4(ptr)

int16x8x4_t __builtin_mpl_vector_ld4q_i16v8(int16_t const *ptr);
#define vld4q_s16(ptr)  __builtin_mpl_vector_ld4q_i16v8(ptr)

int32x2x4_t __builtin_mpl_vector_ld4_i32v2(int32_t const *ptr);
#define vld4_s32(ptr)  __builtin_mpl_vector_ld4_i32v2(ptr)

int32x4x4_t __builtin_mpl_vector_ld4q_i32v4(int32_t const *ptr);
#define vld4q_s32(ptr)  __builtin_mpl_vector_ld4q_i32v4(ptr)

uint8x8x4_t __builtin_mpl_vector_ld4_u8v8(uint8_t const *ptr);
#define vld4_u8(ptr)  __builtin_mpl_vector_ld4_u8v8(ptr)

uint8x16x4_t __builtin_mpl_vector_ld4q_u8v16(uint8_t const *ptr);
#define vld4q_u8(ptr)  __builtin_mpl_vector_ld4q_u8v16(ptr)

uint16x4x4_t __builtin_mpl_vector_ld4_u16v4(uint16_t const *ptr);
#define vld4_u16(ptr)  __builtin_mpl_vector_ld4_u16v4(ptr)

uint16x8x4_t __builtin_mpl_vector_ld4q_u16v8(uint16_t const *ptr);
#define vld4q_u16(ptr)  __builtin_mpl_vector_ld4q_u16v8(ptr)

uint32x2x4_t __builtin_mpl_vector_ld4_u32v2(uint32_t const *ptr);
#define vld4_u32(ptr)  __builtin_mpl_vector_ld4_u32v2(ptr)

uint32x4x4_t __builtin_mpl_vector_ld4q_u32v4(uint32_t const *ptr);
#define vld4q_u32(ptr)  __builtin_mpl_vector_ld4q_u32v4(ptr)

int64x1x4_t __builtin_mpl_vector_ld4_i64v1(int64_t const *ptr);
#define vld4_s64(ptr)  __builtin_mpl_vector_ld4_i64v1(ptr)

uint64x1x4_t __builtin_mpl_vector_ld4_u64v1(uint64_t const *ptr);
#define vld4_u64(ptr)  __builtin_mpl_vector_ld4_u64v1(ptr)

int64x2x4_t __builtin_mpl_vector_ld4q_i64v2(int64_t const *ptr);
#define vld4q_s64(ptr)  __builtin_mpl_vector_ld4q_i64v2(ptr)

uint64x2x4_t __builtin_mpl_vector_ld4q_u64v2(uint64_t const *ptr);
#define vld4q_u64(ptr)  __builtin_mpl_vector_ld4q_u64v2(ptr)

int8x8x2_t __builtin_mpl_vector_ld2_dup_i8v8(int8_t const *ptr);
#define vld2_dup_s8(ptr)  __builtin_mpl_vector_ld2_dup_i8v8(ptr)

int8x16x2_t __builtin_mpl_vector_ld2q_dup_i8v16(int8_t const *ptr);
#define vld2q_dup_s8(ptr)  __builtin_mpl_vector_ld2q_dup_i8v16(ptr)

int16x4x2_t __builtin_mpl_vector_ld2_dup_i16v4(int16_t const *ptr);
#define vld2_dup_s16(ptr)  __builtin_mpl_vector_ld2_dup_i16v4(ptr)

int16x8x2_t __builtin_mpl_vector_ld2q_dup_i16v8(int16_t const *ptr);
#define vld2q_dup_s16(ptr)  __builtin_mpl_vector_ld2q_dup_i16v8(ptr)

int32x2x2_t __builtin_mpl_vector_ld2_dup_i32v2(int32_t const *ptr);
#define vld2_dup_s32(ptr)  __builtin_mpl_vector_ld2_dup_i32v2(ptr)

int32x4x2_t __builtin_mpl_vector_ld2q_dup_i32v4(int32_t const *ptr);
#define vld2q_dup_s32(ptr)  __builtin_mpl_vector_ld2q_dup_i32v4(ptr)

uint8x8x2_t __builtin_mpl_vector_ld2_dup_u8v8(uint8_t const *ptr);
#define vld2_dup_u8(ptr)  __builtin_mpl_vector_ld2_dup_u8v8(ptr)

uint8x16x2_t __builtin_mpl_vector_ld2q_dup_u8v16(uint8_t const *ptr);
#define vld2q_dup_u8(ptr)  __builtin_mpl_vector_ld2q_dup_u8v16(ptr)

uint16x4x2_t __builtin_mpl_vector_ld2_dup_u16v4(uint16_t const *ptr);
#define vld2_dup_u16(ptr)  __builtin_mpl_vector_ld2_dup_u16v4(ptr)

uint16x8x2_t __builtin_mpl_vector_ld2q_dup_u16v8(uint16_t const *ptr);
#define vld2q_dup_u16(ptr)  __builtin_mpl_vector_ld2q_dup_u16v8(ptr)

uint32x2x2_t __builtin_mpl_vector_ld2_dup_u32v2(uint32_t const *ptr);
#define vld2_dup_u32(ptr)  __builtin_mpl_vector_ld2_dup_u32v2(ptr)

uint32x4x2_t __builtin_mpl_vector_ld2q_dup_u32v4(uint32_t const *ptr);
#define vld2q_dup_u32(ptr)  __builtin_mpl_vector_ld2q_dup_u32v4(ptr)

int64x1x2_t __builtin_mpl_vector_ld2_dup_i64v1(int64_t const *ptr);
#define vld2_dup_s64(ptr)  __builtin_mpl_vector_ld2_dup_i64v1(ptr)

uint64x1x2_t __builtin_mpl_vector_ld2_dup_u64v1(uint64_t const *ptr);
#define vld2_dup_u64(ptr)  __builtin_mpl_vector_ld2_dup_u64v1(ptr)

int64x2x2_t __builtin_mpl_vector_ld2q_dup_i64v2(int64_t const *ptr);
#define vld2q_dup_s64(ptr)  __builtin_mpl_vector_ld2q_dup_i64v2(ptr)

uint64x2x2_t __builtin_mpl_vector_ld2q_dup_u64v2(uint64_t const *ptr);
#define vld2q_dup_u64(ptr)  __builtin_mpl_vector_ld2q_dup_u64v2(ptr)

int8x8x3_t __builtin_mpl_vector_ld3_dup_i8v8(int8_t const *ptr);
#define vld3_dup_s8(ptr)  __builtin_mpl_vector_ld3_dup_i8v8(ptr)

int8x16x3_t __builtin_mpl_vector_ld3q_dup_i8v16(int8_t const *ptr);
#define vld3q_dup_s8(ptr)  __builtin_mpl_vector_ld3q_dup_i8v16(ptr)

int16x4x3_t __builtin_mpl_vector_ld3_dup_i16v4(int16_t const *ptr);
#define vld3_dup_s16(ptr)  __builtin_mpl_vector_ld3_dup_i16v4(ptr)

int16x8x3_t __builtin_mpl_vector_ld3q_dup_i16v8(int16_t const *ptr);
#define vld3q_dup_s16(ptr)  __builtin_mpl_vector_ld3q_dup_i16v8(ptr)

int32x2x3_t __builtin_mpl_vector_ld3_dup_i32v2(int32_t const *ptr);
#define vld3_dup_s32(ptr)  __builtin_mpl_vector_ld3_dup_i32v2(ptr)

int32x4x3_t __builtin_mpl_vector_ld3q_dup_i32v4(int32_t const *ptr);
#define vld3q_dup_s32(ptr)  __builtin_mpl_vector_ld3q_dup_i32v4(ptr)

uint8x8x3_t __builtin_mpl_vector_ld3_dup_u8v8(uint8_t const *ptr);
#define vld3_dup_u8(ptr)  __builtin_mpl_vector_ld3_dup_u8v8(ptr)

uint8x16x3_t __builtin_mpl_vector_ld3q_dup_u8v16(uint8_t const *ptr);
#define vld3q_dup_u8(ptr)  __builtin_mpl_vector_ld3q_dup_u8v16(ptr)

uint16x4x3_t __builtin_mpl_vector_ld3_dup_u16v4(uint16_t const *ptr);
#define vld3_dup_u16(ptr)  __builtin_mpl_vector_ld3_dup_u16v4(ptr)

uint16x8x3_t __builtin_mpl_vector_ld3q_dup_u16v8(uint16_t const *ptr);
#define vld3q_dup_u16(ptr)  __builtin_mpl_vector_ld3q_dup_u16v8(ptr)

uint32x2x3_t __builtin_mpl_vector_ld3_dup_u32v2(uint32_t const *ptr);
#define vld3_dup_u32(ptr)  __builtin_mpl_vector_ld3_dup_u32v2(ptr)

uint32x4x3_t __builtin_mpl_vector_ld3q_dup_u32v4(uint32_t const *ptr);
#define vld3q_dup_u32(ptr)  __builtin_mpl_vector_ld3q_dup_u32v4(ptr)

int64x1x3_t __builtin_mpl_vector_ld3_dup_i64v1(int64_t const *ptr);
#define vld3_dup_s64(ptr)  __builtin_mpl_vector_ld3_dup_i64v1(ptr)

uint64x1x3_t __builtin_mpl_vector_ld3_dup_u64v1(uint64_t const *ptr);
#define vld3_dup_u64(ptr)  __builtin_mpl_vector_ld3_dup_u64v1(ptr)

int64x2x3_t __builtin_mpl_vector_ld3q_dup_i64v2(int64_t const *ptr);
#define vld3q_dup_s64(ptr)  __builtin_mpl_vector_ld3q_dup_i64v2(ptr)

uint64x2x3_t __builtin_mpl_vector_ld3q_dup_u64v2(uint64_t const *ptr);
#define vld3q_dup_u64(ptr)  __builtin_mpl_vector_ld3q_dup_u64v2(ptr)

int8x8x4_t __builtin_mpl_vector_ld4_dup_i8v8(int8_t const *ptr);
#define vld4_dup_s8(ptr)  __builtin_mpl_vector_ld4_dup_i8v8(ptr)

int8x16x4_t __builtin_mpl_vector_ld4q_dup_i8v16(int8_t const *ptr);
#define vld4q_dup_s8(ptr)  __builtin_mpl_vector_ld4q_dup_i8v16(ptr)

int16x4x4_t __builtin_mpl_vector_ld4_dup_i16v4(int16_t const *ptr);
#define vld4_dup_s16(ptr)  __builtin_mpl_vector_ld4_dup_i16v4(ptr)

int16x8x4_t __builtin_mpl_vector_ld4q_dup_i16v8(int16_t const *ptr);
#define vld4q_dup_s16(ptr)  __builtin_mpl_vector_ld4q_dup_i16v8(ptr)

int32x2x4_t __builtin_mpl_vector_ld4_dup_i32v2(int32_t const *ptr);
#define vld4_dup_s32(ptr)  __builtin_mpl_vector_ld4_dup_i32v2(ptr)

int32x4x4_t __builtin_mpl_vector_ld4q_dup_i32v4(int32_t const *ptr);
#define vld4q_dup_s32(ptr)  __builtin_mpl_vector_ld4q_dup_i32v4(ptr)

uint8x8x4_t __builtin_mpl_vector_ld4_dup_u8v8(uint8_t const *ptr);
#define vld4_dup_u8(ptr)  __builtin_mpl_vector_ld4_dup_u8v8(ptr)

uint8x16x4_t __builtin_mpl_vector_ld4q_dup_u8v16(uint8_t const *ptr);
#define vld4q_dup_u8(ptr)  __builtin_mpl_vector_ld4q_dup_u8v16(ptr)

uint16x4x4_t __builtin_mpl_vector_ld4_dup_u16v4(uint16_t const *ptr);
#define vld4_dup_u16(ptr)  __builtin_mpl_vector_ld4_dup_u16v4(ptr)

uint16x8x4_t __builtin_mpl_vector_ld4q_dup_u16v8(uint16_t const *ptr);
#define vld4q_dup_u16(ptr)  __builtin_mpl_vector_ld4q_dup_u16v8(ptr)

uint32x2x4_t __builtin_mpl_vector_ld4_dup_u32v2(uint32_t const *ptr);
#define vld4_dup_u32(ptr)  __builtin_mpl_vector_ld4_dup_u32v2(ptr)

uint32x4x4_t __builtin_mpl_vector_ld4q_dup_u32v4(uint32_t const *ptr);
#define vld4q_dup_u32(ptr)  __builtin_mpl_vector_ld4q_dup_u32v4(ptr)

int64x1x4_t __builtin_mpl_vector_ld4_dup_i64v1(int64_t const *ptr);
#define vld4_dup_s64(ptr)  __builtin_mpl_vector_ld4_dup_i64v1(ptr)

uint64x1x4_t __builtin_mpl_vector_ld4_dup_u64v1(uint64_t const *ptr);
#define vld4_dup_u64(ptr)  __builtin_mpl_vector_ld4_dup_u64v1(ptr)

int64x2x4_t __builtin_mpl_vector_ld4q_dup_i64v2(int64_t const *ptr);
#define vld4q_dup_s64(ptr)  __builtin_mpl_vector_ld4q_dup_i64v2(ptr)

uint64x2x4_t __builtin_mpl_vector_ld4q_dup_u64v2(uint64_t const *ptr);
#define vld4q_dup_u64(ptr)  __builtin_mpl_vector_ld4q_dup_u64v2(ptr)

int16x4x2_t __builtin_mpl_vector_ld2_lane_i16v4(int16_t const *ptr, int16x4x2_t src, const int lane);
#define vld2_lane_s16(ptr, src, lane)  __builtin_mpl_vector_ld2_lane_i16v4(ptr, src, lane)

int16x8x2_t __builtin_mpl_vector_ld2q_lane_i16v8(int16_t const *ptr, int16x8x2_t src, const int lane);
#define vld2q_lane_s16(ptr, src, lane)  __builtin_mpl_vector_ld2q_lane_i16v8(ptr, src, lane)

int32x2x2_t __builtin_mpl_vector_ld2_lane_i32v2(int32_t const *ptr, int32x2x2_t src, const int lane);
#define vld2_lane_s32(ptr, src, lane)  __builtin_mpl_vector_ld2_lane_i32v2(ptr, src, lane)

int32x4x2_t __builtin_mpl_vector_ld2q_lane_i32v4(int32_t const *ptr, int32x4x2_t src, const int lane);
#define vld2q_lane_s32(ptr, src, lane)  __builtin_mpl_vector_ld2q_lane_i32v4(ptr, src, lane)

uint16x4x2_t __builtin_mpl_vector_ld2_lane_u16v4(uint16_t const *ptr, uint16x4x2_t src, const int lane);
#define vld2_lane_u16(ptr, src, lane)  __builtin_mpl_vector_ld2_lane_u16v4(ptr, src, lane)

uint16x8x2_t __builtin_mpl_vector_ld2q_lane_u16v8(uint16_t const *ptr, uint16x8x2_t src, const int lane);
#define vld2q_lane_u16(ptr, src, lane)  __builtin_mpl_vector_ld2q_lane_u16v8(ptr, src, lane)

uint32x2x2_t __builtin_mpl_vector_ld2_lane_u32v2(uint32_t const *ptr, uint32x2x2_t src, const int lane);
#define vld2_lane_u32(ptr, src, lane)  __builtin_mpl_vector_ld2_lane_u32v2(ptr, src, lane)

uint32x4x2_t __builtin_mpl_vector_ld2q_lane_u32v4(uint32_t const *ptr, uint32x4x2_t src, const int lane);
#define vld2q_lane_u32(ptr, src, lane)  __builtin_mpl_vector_ld2q_lane_u32v4(ptr, src, lane)

int8x8x2_t __builtin_mpl_vector_ld2_lane_i8v8(int8_t const *ptr, int8x8x2_t src, const int lane);
#define vld2_lane_s8(ptr, src, lane)  __builtin_mpl_vector_ld2_lane_i8v8(ptr, src, lane)

uint8x8x2_t __builtin_mpl_vector_ld2_lane_u8v8(uint8_t const *ptr, uint8x8x2_t src, const int lane);
#define vld2_lane_u8(ptr, src, lane)  __builtin_mpl_vector_ld2_lane_u8v8(ptr, src, lane)

int8x16x2_t __builtin_mpl_vector_ld2q_lane_i8v16(int8_t const *ptr, int8x16x2_t src, const int lane);
#define vld2q_lane_s8(ptr, src, lane)  __builtin_mpl_vector_ld2q_lane_i8v16(ptr, src, lane)

uint8x16x2_t __builtin_mpl_vector_ld2q_lane_u8v16(uint8_t const *ptr, uint8x16x2_t src, const int lane);
#define vld2q_lane_u8(ptr, src, lane)  __builtin_mpl_vector_ld2q_lane_u8v16(ptr, src, lane)

int64x1x2_t __builtin_mpl_vector_ld2_lane_i64v1(int64_t const *ptr, int64x1x2_t src, const int lane);
#define vld2_lane_s64(ptr, src, lane)  __builtin_mpl_vector_ld2_lane_i64v1(ptr, src, lane)

int64x2x2_t __builtin_mpl_vector_ld2q_lane_i64v2(int64_t const *ptr, int64x2x2_t src, const int lane);
#define vld2q_lane_s64(ptr, src, lane)  __builtin_mpl_vector_ld2q_lane_i64v2(ptr, src, lane)

uint64x1x2_t __builtin_mpl_vector_ld2_lane_u64v1(uint64_t const *ptr, uint64x1x2_t src, const int lane);
#define vld2_lane_u64(ptr, src, lane)  __builtin_mpl_vector_ld2_lane_u64v1(ptr, src, lane)

uint64x2x2_t __builtin_mpl_vector_ld2q_lane_u64v2(uint64_t const *ptr, uint64x2x2_t src, const int lane);
#define vld2q_lane_u64(ptr, src, lane)  __builtin_mpl_vector_ld2q_lane_u64v2(ptr, src, lane)

int16x4x3_t __builtin_mpl_vector_ld3_lane_i16v4(int16_t const *ptr, int16x4x3_t src, const int lane);
#define vld3_lane_s16(ptr, src, lane)  __builtin_mpl_vector_ld3_lane_i16v4(ptr, src, lane)

int16x8x3_t __builtin_mpl_vector_ld3q_lane_i16v8(int16_t const *ptr, int16x8x3_t src, const int lane);
#define vld3q_lane_s16(ptr, src, lane)  __builtin_mpl_vector_ld3q_lane_i16v8(ptr, src, lane)

int32x2x3_t __builtin_mpl_vector_ld3_lane_i32v2(int32_t const *ptr, int32x2x3_t src, const int lane);
#define vld3_lane_s32(ptr, src, lane)  __builtin_mpl_vector_ld3_lane_i32v2(ptr, src, lane)

int32x4x3_t __builtin_mpl_vector_ld3q_lane_i32v4(int32_t const *ptr, int32x4x3_t src, const int lane);
#define vld3q_lane_s32(ptr, src, lane)  __builtin_mpl_vector_ld3q_lane_i32v4(ptr, src, lane)

uint16x4x3_t __builtin_mpl_vector_ld3_lane_u16v4(uint16_t const *ptr, uint16x4x3_t src, const int lane);
#define vld3_lane_u16(ptr, src, lane)  __builtin_mpl_vector_ld3_lane_u16v4(ptr, src, lane)

uint16x8x3_t __builtin_mpl_vector_ld3q_lane_u16v8(uint16_t const *ptr, uint16x8x3_t src, const int lane);
#define vld3q_lane_u16(ptr, src, lane)  __builtin_mpl_vector_ld3q_lane_u16v8(ptr, src, lane)

uint32x2x3_t __builtin_mpl_vector_ld3_lane_u32v2(uint32_t const *ptr, uint32x2x3_t src, const int lane);
#define vld3_lane_u32(ptr, src, lane)  __builtin_mpl_vector_ld3_lane_u32v2(ptr, src, lane)

uint32x4x3_t __builtin_mpl_vector_ld3q_lane_u32v4(uint32_t const *ptr, uint32x4x3_t src, const int lane);
#define vld3q_lane_u32(ptr, src, lane)  __builtin_mpl_vector_ld3q_lane_u32v4(ptr, src, lane)

int8x8x3_t __builtin_mpl_vector_ld3_lane_i8v8(int8_t const *ptr, int8x8x3_t src, const int lane);
#define vld3_lane_s8(ptr, src, lane)  __builtin_mpl_vector_ld3_lane_i8v8(ptr, src, lane)

uint8x8x3_t __builtin_mpl_vector_ld3_lane_u8v8(uint8_t const *ptr, uint8x8x3_t src, const int lane);
#define vld3_lane_u8(ptr, src, lane)  __builtin_mpl_vector_ld3_lane_u8v8(ptr, src, lane)

int8x16x3_t __builtin_mpl_vector_ld3q_lane_i8v16(int8_t const *ptr, int8x16x3_t src, const int lane);
#define vld3q_lane_s8(ptr, src, lane)  __builtin_mpl_vector_ld3q_lane_i8v16(ptr, src, lane)

uint8x16x3_t __builtin_mpl_vector_ld3q_lane_u8v16(uint8_t const *ptr, uint8x16x3_t src, const int lane);
#define vld3q_lane_u8(ptr, src, lane)  __builtin_mpl_vector_ld3q_lane_u8v16(ptr, src, lane)

int64x1x3_t __builtin_mpl_vector_ld3_lane_i64v1(int64_t const *ptr, int64x1x3_t src, const int lane);
#define vld3_lane_s64(ptr, src, lane)  __builtin_mpl_vector_ld3_lane_i64v1(ptr, src, lane)

int64x2x3_t __builtin_mpl_vector_ld3q_lane_i64v2(int64_t const *ptr, int64x2x3_t src, const int lane);
#define vld3q_lane_s64(ptr, src, lane)  __builtin_mpl_vector_ld3q_lane_i64v2(ptr, src, lane)

uint64x1x3_t __builtin_mpl_vector_ld3_lane_u64v1(uint64_t const *ptr, uint64x1x3_t src, const int lane);
#define vld3_lane_u64(ptr, src, lane)  __builtin_mpl_vector_ld3_lane_u64v1(ptr, src, lane)

uint64x2x3_t __builtin_mpl_vector_ld3q_lane_u64v2(uint64_t const *ptr, uint64x2x3_t src, const int lane);
#define vld3q_lane_u64(ptr, src, lane)  __builtin_mpl_vector_ld3q_lane_u64v2(ptr, src, lane)

int16x4x4_t __builtin_mpl_vector_ld4_lane_i16v4(int16_t const *ptr, int16x4x4_t src, const int lane);
#define vld4_lane_s16(ptr, src, lane)  __builtin_mpl_vector_ld4_lane_i16v4(ptr, src, lane)

int16x8x4_t __builtin_mpl_vector_ld4q_lane_i16v8(int16_t const *ptr, int16x8x4_t src, const int lane);
#define vld4q_lane_s16(ptr, src, lane)  __builtin_mpl_vector_ld4q_lane_i16v8(ptr, src, lane)

int32x2x4_t __builtin_mpl_vector_ld4_lane_i32v2(int32_t const *ptr, int32x2x4_t src, const int lane);
#define vld4_lane_s32(ptr, src, lane)  __builtin_mpl_vector_ld4_lane_i32v2(ptr, src, lane)

int32x4x4_t __builtin_mpl_vector_ld4q_lane_i32v4(int32_t const *ptr, int32x4x4_t src, const int lane);
#define vld4q_lane_s32(ptr, src, lane)  __builtin_mpl_vector_ld4q_lane_i32v4(ptr, src, lane)

uint16x4x4_t __builtin_mpl_vector_ld4_lane_u16v4(uint16_t const *ptr, uint16x4x4_t src, const int lane);
#define vld4_lane_u16(ptr, src, lane)  __builtin_mpl_vector_ld4_lane_u16v4(ptr, src, lane)

uint16x8x4_t __builtin_mpl_vector_ld4q_lane_u16v8(uint16_t const *ptr, uint16x8x4_t src, const int lane);
#define vld4q_lane_u16(ptr, src, lane)  __builtin_mpl_vector_ld4q_lane_u16v8(ptr, src, lane)

uint32x2x4_t __builtin_mpl_vector_ld4_lane_u32v2(uint32_t const *ptr, uint32x2x4_t src, const int lane);
#define vld4_lane_u32(ptr, src, lane)  __builtin_mpl_vector_ld4_lane_u32v2(ptr, src, lane)

uint32x4x4_t __builtin_mpl_vector_ld4q_lane_u32v4(uint32_t const *ptr, uint32x4x4_t src, const int lane);
#define vld4q_lane_u32(ptr, src, lane)  __builtin_mpl_vector_ld4q_lane_u32v4(ptr, src, lane)

int8x8x4_t __builtin_mpl_vector_ld4_lane_i8v8(int8_t const *ptr, int8x8x4_t src, const int lane);
#define vld4_lane_s8(ptr, src, lane)  __builtin_mpl_vector_ld4_lane_i8v8(ptr, src, lane)

uint8x8x4_t __builtin_mpl_vector_ld4_lane_u8v8(uint8_t const *ptr, uint8x8x4_t src, const int lane);
#define vld4_lane_u8(ptr, src, lane)  __builtin_mpl_vector_ld4_lane_u8v8(ptr, src, lane)

int8x16x4_t __builtin_mpl_vector_ld4q_lane_i8v16(int8_t const *ptr, int8x16x4_t src, const int lane);
#define vld4q_lane_s8(ptr, src, lane)  __builtin_mpl_vector_ld4q_lane_i8v16(ptr, src, lane)

uint8x16x4_t __builtin_mpl_vector_ld4q_lane_u8v16(uint8_t const *ptr, uint8x16x4_t src, const int lane);
#define vld4q_lane_u8(ptr, src, lane)  __builtin_mpl_vector_ld4q_lane_u8v16(ptr, src, lane)

int64x1x4_t __builtin_mpl_vector_ld4_lane_i64v1(int64_t const *ptr, int64x1x4_t src, const int lane);
#define vld4_lane_s64(ptr, src, lane)  __builtin_mpl_vector_ld4_lane_i64v1(ptr, src, lane)

int64x2x4_t __builtin_mpl_vector_ld4q_lane_i64v2(int64_t const *ptr, int64x2x4_t src, const int lane);
#define vld4q_lane_s64(ptr, src, lane)  __builtin_mpl_vector_ld4q_lane_i64v2(ptr, src, lane)

uint64x1x4_t __builtin_mpl_vector_ld4_lane_u64v1(uint64_t const *ptr, uint64x1x4_t src, const int lane);
#define vld4_lane_u64(ptr, src, lane)  __builtin_mpl_vector_ld4_lane_u64v1(ptr, src, lane)

uint64x2x4_t __builtin_mpl_vector_ld4q_lane_u64v2(uint64_t const *ptr, uint64x2x4_t src, const int lane);
#define vld4q_lane_u64(ptr, src, lane)  __builtin_mpl_vector_ld4q_lane_u64v2(ptr, src, lane)

void __builtin_mpl_vector_st1_lane_i8v8(int8_t *ptr, int8x8_t val, const int lane);
#define vst1_lane_s8(ptr, val, lane)  __builtin_mpl_vector_st1_lane_i8v8(ptr, val, lane)

void __builtin_mpl_vector_st1q_lane_i8v16(int8_t *ptr, int8x16_t val, const int lane);
#define vst1q_lane_s8(ptr, val, lane)  __builtin_mpl_vector_st1q_lane_i8v16(ptr, val, lane)

void __builtin_mpl_vector_st1_lane_i16v4(int16_t *ptr, int16x4_t val, const int lane);
#define vst1_lane_s16(ptr, val, lane)  __builtin_mpl_vector_st1_lane_i16v4(ptr, val, lane)

void __builtin_mpl_vector_st1q_lane_i16v8(int16_t *ptr, int16x8_t val, const int lane);
#define vst1q_lane_s16(ptr, val, lane)  __builtin_mpl_vector_st1q_lane_i16v8(ptr, val, lane)

void __builtin_mpl_vector_st1_lane_i32v2(int32_t *ptr, int32x2_t val, const int lane);
#define vst1_lane_s32(ptr, val, lane)  __builtin_mpl_vector_st1_lane_i32v2(ptr, val, lane)

void __builtin_mpl_vector_st1q_lane_i32v4(int32_t *ptr, int32x4_t val, const int lane);
#define vst1q_lane_s32(ptr, val, lane)  __builtin_mpl_vector_st1q_lane_i32v4(ptr, val, lane)

void __builtin_mpl_vector_st1_lane_i64v1(int64_t *ptr, int64x1_t val, const int lane);
#define vst1_lane_s64(ptr, val, lane)  __builtin_mpl_vector_st1_lane_i64v1(ptr, val, lane)

void __builtin_mpl_vector_st1q_lane_i64v2(int64_t *ptr, int64x2_t val, const int lane);
#define vst1q_lane_s64(ptr, val, lane)  __builtin_mpl_vector_st1q_lane_i64v2(ptr, val, lane)

void __builtin_mpl_vector_st1_lane_u8v8(uint8_t *ptr, uint8x8_t val, const int lane);
#define vst1_lane_u8(ptr, val, lane)  __builtin_mpl_vector_st1_lane_u8v8(ptr, val, lane)

void __builtin_mpl_vector_st1q_lane_u8v16(uint8_t *ptr, uint8x16_t val, const int lane);
#define vst1q_lane_u8(ptr, val, lane)  __builtin_mpl_vector_st1q_lane_u8v16(ptr, val, lane)

void __builtin_mpl_vector_st1_lane_u16v4(uint16_t *ptr, uint16x4_t val, const int lane);
#define vst1_lane_u16(ptr, val, lane)  __builtin_mpl_vector_st1_lane_u16v4(ptr, val, lane)

void __builtin_mpl_vector_st1q_lane_u16v8(uint16_t *ptr, uint16x8_t val, const int lane);
#define vst1q_lane_u16(ptr, val, lane)  __builtin_mpl_vector_st1q_lane_u16v8(ptr, val, lane)

void __builtin_mpl_vector_st1_lane_u32v2(uint32_t *ptr, uint32x2_t val, const int lane);
#define vst1_lane_u32(ptr, val, lane)  __builtin_mpl_vector_st1_lane_u32v2(ptr, val, lane)

void __builtin_mpl_vector_st1q_lane_u32v4(uint32_t *ptr, uint32x4_t val, const int lane);
#define vst1q_lane_u32(ptr, val, lane)  __builtin_mpl_vector_st1q_lane_u32v4(ptr, val, lane)

void __builtin_mpl_vector_st1_lane_u64v1(uint64_t *ptr, uint64x1_t val, const int lane);
#define vst1_lane_u64(ptr, val, lane)  __builtin_mpl_vector_st1_lane_u64v1(ptr, val, lane)

void __builtin_mpl_vector_st1q_lane_u64v2(uint64_t *ptr, uint64x2_t val, const int lane);
#define vst1q_lane_u64(ptr, val, lane)  __builtin_mpl_vector_st1q_lane_u64v2(ptr, val, lane)

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

int8x8_t __builtin_mpl_vector_tbl1_i8v8(int8x16_t a, int8x8_t idx);
static inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__))
vtbl1_s8(int8x8_t a, int8x8_t idx) {
  return __builtin_mpl_vector_tbl1_i8v8(vcombine_s8(a, (int8x8_t){0}), idx);
}

uint8x8_t __builtin_mpl_vector_tbl1_u8v8(uint16x8_t a, uint8x8_t idx);
static inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__))
vtbl1_u8(uint8x8_t a, uint8x8_t idx) {
  return __builtin_mpl_vector_tbl1_i8v8(vcombine_s8(a, (uint8x8_t){0}), idx);
}

int8x8_t __builtin_mpl_vector_qtbl1_i8v8(int8x16_t t, uint8x8_t idx);
#define vqtbl1_s8(t, idx)  __builtin_mpl_vector_qtbl1_i8v8(t, idx)

int8x16_t __builtin_mpl_vector_qtbl1q_i8v16(int8x16_t t, uint8x16_t idx);
#define vqtbl1q_s8(t, idx)  __builtin_mpl_vector_qtbl1q_i8v16(t, idx)

uint8x8_t __builtin_mpl_vector_qtbl1_u8v8(uint8x16_t t, uint8x8_t idx);
#define vqtbl1_u8(t, idx)  __builtin_mpl_vector_qtbl1_u8v8(t, idx)

uint8x16_t __builtin_mpl_vector_qtbl1q_u8v16(uint8x16_t t, uint8x16_t idx);
#define vqtbl1q_u8(t, idx)  __builtin_mpl_vector_qtbl1q_u8v16(t, idx)

static inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__))
vtbl2_s8(int8x8x2_t a, int8x8_t idx) {
  return vqtbl1_s8(vcombine_s8(a.val[0], a.val[1]), idx);
}

static inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__))
vtbl2_u8(uint8x8x2_t a, uint8x8_t idx) {
  return vqtbl1_u8(vcombine_u8(a.val[0], a.val[1]), idx);
}

int8x8_t __builtin_mpl_vector_tbl3_i8v8(int8x8x3_t a, int8x8_t idx);
#define vtbl3_s8(a, idx)  __builtin_mpl_vector_tbl3_i8v8(a, idx)

uint8x8_t __builtin_mpl_vector_tbl3_u8v8(uint8x8x3_t a, uint8x8_t idx);
#define vtbl3_u8(a, idx)  __builtin_mpl_vector_tbl3_u8v8(a, idx)

int8x8_t __builtin_mpl_vector_tbl4_i8v8(int8x8x4_t a, int8x8_t idx);
#define vtbl4_s8(a, idx)  __builtin_mpl_vector_tbl4_i8v8(a, idx)

uint8x8_t __builtin_mpl_vector_tbl4_u8v8(uint8x8x4_t a, uint8x8_t idx);
#define vtbl4_u8(a, idx)  __builtin_mpl_vector_tbl4_u8v8(a, idx)

int8x8_t __builtin_mpl_vector_qtbl2_i8v8(int8x16x2_t t, uint8x8_t idx);
#define vqtbl2_s8(t, idx)  __builtin_mpl_vector_qtbl2_i8v8(t, idx)

int8x16_t __builtin_mpl_vector_qtbl2q_i8v16(int8x16x2_t t, uint8x16_t idx);
#define vqtbl2q_s8(t, idx)  __builtin_mpl_vector_qtbl2q_i8v16(t, idx)

uint8x8_t __builtin_mpl_vector_qtbl2_u8v8(uint8x16x2_t t, uint8x8_t idx);
#define vqtbl2_u8(t, idx)  __builtin_mpl_vector_qtbl2_u8v8(t, idx)

uint8x16_t __builtin_mpl_vector_qtbl2q_u8v16(uint8x16x2_t t, uint8x16_t idx);
#define vqtbl2q_u8(t, idx)  __builtin_mpl_vector_qtbl2q_u8v16(t, idx)

int8x8_t __builtin_mpl_vector_qtbl3_i8v8(int8x16x3_t t, uint8x8_t idx);
#define vqtbl3_s8(t, idx)  __builtin_mpl_vector_qtbl3_i8v8(t, idx)

int8x16_t __builtin_mpl_vector_qtbl3q_i8v16(int8x16x3_t t, uint8x16_t idx);
#define vqtbl3q_s8(t, idx)  __builtin_mpl_vector_qtbl3q_i8v16(t, idx)

uint8x8_t __builtin_mpl_vector_qtbl3_u8v8(uint8x16x3_t t, uint8x8_t idx);
#define vqtbl3_u8(t, idx)  __builtin_mpl_vector_qtbl3_u8v8(t, idx)

uint8x16_t __builtin_mpl_vector_qtbl3q_u8v16(uint8x16x3_t t, uint8x16_t idx);
#define vqtbl3q_u8(t, idx)  __builtin_mpl_vector_qtbl3q_u8v16(t, idx)

int8x8_t __builtin_mpl_vector_qtbl4_i8v8(int8x16x4_t t, uint8x8_t idx);
#define vqtbl4_s8(t, idx)  __builtin_mpl_vector_qtbl4_i8v8(t, idx)

int8x16_t __builtin_mpl_vector_qtbl4q_i8v16(int8x16x4_t t, uint8x16_t idx);
#define vqtbl4q_s8(t, idx)  __builtin_mpl_vector_qtbl4q_i8v16(t, idx)

uint8x8_t __builtin_mpl_vector_qtbl4_u8v8(uint8x16x4_t t, uint8x8_t idx);
#define vqtbl4_u8(t, idx)  __builtin_mpl_vector_qtbl4_u8v8(t, idx)

uint8x16_t __builtin_mpl_vector_qtbl4q_u8v16(uint8x16x4_t t, uint8x16_t idx);
#define vqtbl4q_u8(t, idx)  __builtin_mpl_vector_qtbl4q_u8v16(t, idx)

static inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__))
vtbx1_s8(int8x8_t a, int8x8_t b, int8x8_t idx) {
  uint8x8_t mask = vclt_u8((uint8x8_t)(idx), vmov_n_u8(8));
  int8x8_t tbl = vtbl1_s8(b, idx);
  return vbsl_s8(mask, tbl, a);
}

static inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__))
vtbx1_u8(uint8x8_t a, uint8x8_t b, uint8x8_t idx) {
  uint8x8_t mask = vclt_u8(idx, vmov_n_u8(8));
  uint8x8_t tbl = vtbl1_u8(b, idx);
  return vbsl_u8(mask, tbl, a);
}

int8x8_t __builtin_mpl_vector_qtbx1_i8v8(int8x8_t a, int8x16_t t, uint8x8_t idx);
#define vqtbx1_s8(a, t, idx)  __builtin_mpl_vector_qtbx1_i8v8(a, t, idx)

int8x16_t __builtin_mpl_vector_qtbx1q_i8v16(int8x16_t a, int8x16_t t, uint8x16_t idx);
#define vqtbx1q_s8(a, t, idx)  __builtin_mpl_vector_qtbx1q_i8v16(a, t, idx)

uint8x8_t __builtin_mpl_vector_qtbx1_u8v8(uint8x8_t a, uint8x16_t t, uint8x8_t idx);
#define vqtbx1_u8(a, t, idx)  __builtin_mpl_vector_qtbx1_u8v8(a, t, idx)

uint8x16_t __builtin_mpl_vector_qtbx1q_u8v16(uint8x16_t a, uint8x16_t t, uint8x16_t idx);
#define vqtbx1q_u8(a, t, idx)  __builtin_mpl_vector_qtbx1q_u8v16(a, t, idx)

static inline int8x8_t __attribute__ ((__always_inline__, __gnu_inline__))
vtbx2_s8(int8x8_t a, int8x8x2_t b, int8x8_t idx) {
  return vqtbx1_s8(a, vcombine_s8(b.val[0], b.val[1]), idx);
}

static inline uint8x8_t __attribute__ ((__always_inline__, __gnu_inline__))
vtbx2_u8(uint8x8_t a, uint8x8x2_t b, uint8x8_t idx) {
  return vqtbx1_u8(a, vcombine_u8(b.val[0], b.val[1]), idx);
}

int8x8_t __builtin_mpl_vector_tbx3_i8v8(int8x8_t a, int8x8x3_t b, int8x8_t idx);
#define vtbx3_s8(a, b, idx)  __builtin_mpl_vector_tbx3_i8v8(a, b, idx)

uint8x8_t __builtin_mpl_vector_tbx3_u8v8(uint8x8_t a, uint8x8x3_t b, uint8x8_t idx);
#define vtbx3_u8(a, b, idx)  __builtin_mpl_vector_tbx3_u8v8(a, b, idx)

int8x8_t __builtin_mpl_vector_tbx4_i8v8(int8x8_t a, int8x8x4_t b, int8x8_t idx);
#define vtbx4_s8(a, b, idx)  __builtin_mpl_vector_tbx4_i8v8(a, b, idx)

uint8x8_t __builtin_mpl_vector_tbx4_u8v8(uint8x8_t a, uint8x8x4_t b, uint8x8_t idx);
#define vtbx4_u8(a, b, idx)  __builtin_mpl_vector_tbx4_u8v8(a, b, idx)

int8x8_t __builtin_mpl_vector_qtbx2_i8v8(int8x8_t a, int8x16x2_t t, uint8x8_t idx);
#define vqtbx2_s8(a, t, idx)  __builtin_mpl_vector_qtbx2_i8v8(a, t, idx)

int8x16_t __builtin_mpl_vector_qtbx2q_i8v16(int8x16_t a, int8x16x2_t t, uint8x16_t idx);
#define vqtbx2q_s8(a, t, idx)  __builtin_mpl_vector_qtbx2q_i8v16(a, t, idx)

uint8x8_t __builtin_mpl_vector_qtbx2_u8v8(uint8x8_t a, uint8x16x2_t t, uint8x8_t idx);
#define vqtbx2_u8(a, t, idx)  __builtin_mpl_vector_qtbx2_u8v8(a, t, idx)

uint8x16_t __builtin_mpl_vector_qtbx2q_u8v16(uint8x16_t a, uint8x16x2_t t, uint8x16_t idx);
#define vqtbx2q_u8(a, t, idx)  __builtin_mpl_vector_qtbx2q_u8v16(a, t, idx)

int8x8_t __builtin_mpl_vector_qtbx3_i8v8(int8x8_t a, int8x16x3_t t, uint8x8_t idx);
#define vqtbx3_s8(a, t, idx)  __builtin_mpl_vector_qtbx3_i8v8(a, t, idx)

int8x16_t __builtin_mpl_vector_qtbx3q_i8v16(int8x16_t a, int8x16x3_t t, uint8x16_t idx);
#define vqtbx3q_s8(a, t, idx)  __builtin_mpl_vector_qtbx3q_i8v16(a, t, idx)

uint8x8_t __builtin_mpl_vector_qtbx3_u8v8(uint8x8_t a, uint8x16x3_t t, uint8x8_t idx);
#define vqtbx3_u8(a, t, idx)  __builtin_mpl_vector_qtbx3_u8v8(a, t, idx)

uint8x16_t __builtin_mpl_vector_qtbx3q_u8v16(uint8x16_t a, uint8x16x3_t t, uint8x16_t idx);
#define vqtbx3q_u8(a, t, idx)  __builtin_mpl_vector_qtbx3q_u8v16(a, t, idx)

int8x8_t __builtin_mpl_vector_qtbx4_i8v8(int8x8_t a, int8x16x4_t t, uint8x8_t idx);
#define vqtbx4_s8(a, t, idx)  __builtin_mpl_vector_qtbx4_i8v8(a, t, idx)

int8x16_t __builtin_mpl_vector_qtbx4q_i8v16(int8x16_t a, int8x16x4_t t, uint8x16_t idx);
#define vqtbx4q_s8(a, t, idx)  __builtin_mpl_vector_qtbx4q_i8v16(a, t, idx)

uint8x8_t __builtin_mpl_vector_qtbx4_u8v8(uint8x8_t a, uint8x16x4_t t, uint8x8_t idx);
#define vqtbx4_u8(a, t, idx)  __builtin_mpl_vector_qtbx4_u8v8(a, t, idx)

uint8x16_t __builtin_mpl_vector_qtbx4q_u8v16(uint8x16_t a, uint8x16x4_t t, uint8x16_t idx);
#define vqtbx4q_u8(a, t, idx)  __builtin_mpl_vector_qtbx4q_u8v16(a, t, idx)

int8x8_t __builtin_mpl_vector_hadd_i8v8(int8x8_t a, int8x8_t b);
#define vhadd_s8(a, b)  __builtin_mpl_vector_hadd_i8v8(a, b)

int8x16_t __builtin_mpl_vector_haddq_i8v16(int8x16_t a, int8x16_t b);
#define vhaddq_s8(a, b)  __builtin_mpl_vector_haddq_i8v16(a, b)

int16x4_t __builtin_mpl_vector_hadd_i16v4(int16x4_t a, int16x4_t b);
#define vhadd_s16(a, b)  __builtin_mpl_vector_hadd_i16v4(a, b)

int16x8_t __builtin_mpl_vector_haddq_i16v8(int16x8_t a, int16x8_t b);
#define vhaddq_s16(a, b)  __builtin_mpl_vector_haddq_i16v8(a, b)

int32x2_t __builtin_mpl_vector_hadd_i32v2(int32x2_t a, int32x2_t b);
#define vhadd_s32(a, b)  __builtin_mpl_vector_hadd_i32v2(a, b)

int32x4_t __builtin_mpl_vector_haddq_i32v4(int32x4_t a, int32x4_t b);
#define vhaddq_s32(a, b)  __builtin_mpl_vector_haddq_i32v4(a, b)

uint8x8_t __builtin_mpl_vector_hadd_u8v8(uint8x8_t a, uint8x8_t b);
#define vhadd_u8(a, b)  __builtin_mpl_vector_hadd_u8v8(a, b)

uint8x16_t __builtin_mpl_vector_haddq_u8v16(uint8x16_t a, uint8x16_t b);
#define vhaddq_u8(a, b)  __builtin_mpl_vector_haddq_u8v16(a, b)

uint16x4_t __builtin_mpl_vector_hadd_u16v4(uint16x4_t a, uint16x4_t b);
#define vhadd_u16(a, b)  __builtin_mpl_vector_hadd_u16v4(a, b)

uint16x8_t __builtin_mpl_vector_haddq_u16v8(uint16x8_t a, uint16x8_t b);
#define vhaddq_u16(a, b)  __builtin_mpl_vector_haddq_u16v8(a, b)

uint32x2_t __builtin_mpl_vector_hadd_u32v2(uint32x2_t a, uint32x2_t b);
#define vhadd_u32(a, b)  __builtin_mpl_vector_hadd_u32v2(a, b)

uint32x4_t __builtin_mpl_vector_haddq_u32v4(uint32x4_t a, uint32x4_t b);
#define vhaddq_u32(a, b)  __builtin_mpl_vector_haddq_u32v4(a, b)

int8x8_t __builtin_mpl_vector_rhadd_i8v8(int8x8_t a, int8x8_t b);
#define vrhadd_s8(a, b)  __builtin_mpl_vector_rhadd_i8v8(a, b)

int8x16_t __builtin_mpl_vector_rhaddq_i8v16(int8x16_t a, int8x16_t b);
#define vrhaddq_s8(a, b)  __builtin_mpl_vector_rhaddq_i8v16(a, b)

int16x4_t __builtin_mpl_vector_rhadd_i16v4(int16x4_t a, int16x4_t b);
#define vrhadd_s16(a, b)  __builtin_mpl_vector_rhadd_i16v4(a, b)

int16x8_t __builtin_mpl_vector_rhaddq_i16v8(int16x8_t a, int16x8_t b);
#define vrhaddq_s16(a, b)  __builtin_mpl_vector_rhaddq_i16v8(a, b)

int32x2_t __builtin_mpl_vector_rhadd_i32v2(int32x2_t a, int32x2_t b);
#define vrhadd_s32(a, b)  __builtin_mpl_vector_rhadd_i32v2(a, b)

int32x4_t __builtin_mpl_vector_rhaddq_i32v4(int32x4_t a, int32x4_t b);
#define vrhaddq_s32(a, b)  __builtin_mpl_vector_rhaddq_i32v4(a, b)

uint8x8_t __builtin_mpl_vector_rhadd_u8v8(uint8x8_t a, uint8x8_t b);
#define vrhadd_u8(a, b)  __builtin_mpl_vector_rhadd_u8v8(a, b)

uint8x16_t __builtin_mpl_vector_rhaddq_u8v16(uint8x16_t a, uint8x16_t b);
#define vrhaddq_u8(a, b)  __builtin_mpl_vector_rhaddq_u8v16(a, b)

uint16x4_t __builtin_mpl_vector_rhadd_u16v4(uint16x4_t a, uint16x4_t b);
#define vrhadd_u16(a, b)  __builtin_mpl_vector_rhadd_u16v4(a, b)

uint16x8_t __builtin_mpl_vector_rhaddq_u16v8(uint16x8_t a, uint16x8_t b);
#define vrhaddq_u16(a, b)  __builtin_mpl_vector_rhaddq_u16v8(a, b)

uint32x2_t __builtin_mpl_vector_rhadd_u32v2(uint32x2_t a, uint32x2_t b);
#define vrhadd_u32(a, b)  __builtin_mpl_vector_rhadd_u32v2(a, b)

uint32x4_t __builtin_mpl_vector_rhaddq_u32v4(uint32x4_t a, uint32x4_t b);
#define vrhaddq_u32(a, b)  __builtin_mpl_vector_rhaddq_u32v4(a, b)

int8x8_t __builtin_mpl_vector_addhn_i8v8(int16x8_t a, int16x8_t b);
#define vaddhn_s16(a, b)  __builtin_mpl_vector_addhn_i8v8(a, b)

int16x4_t __builtin_mpl_vector_addhn_i16v4(int32x4_t a, int32x4_t b);
#define vaddhn_s32(a, b)  __builtin_mpl_vector_addhn_i16v4(a, b)

int32x2_t __builtin_mpl_vector_addhn_i32v2(int64x2_t a, int64x2_t b);
#define vaddhn_s64(a, b)  __builtin_mpl_vector_addhn_i32v2(a, b)

uint8x8_t __builtin_mpl_vector_addhn_u8v8(uint16x8_t a, uint16x8_t b);
#define vaddhn_u16(a, b)  __builtin_mpl_vector_addhn_u8v8(a, b)

uint16x4_t __builtin_mpl_vector_addhn_u16v4(uint32x4_t a, uint32x4_t b);
#define vaddhn_u32(a, b)  __builtin_mpl_vector_addhn_u16v4(a, b)

uint32x2_t __builtin_mpl_vector_addhn_u32v2(uint64x2_t a, uint64x2_t b);
#define vaddhn_u64(a, b)  __builtin_mpl_vector_addhn_u32v2(a, b)

int8x16_t __builtin_mpl_vector_addhn_high_i8v16(int8x8_t r, int16x8_t a, int16x8_t b);
#define vaddhn_high_s16(r, a, b)  __builtin_mpl_vector_addhn_high_i8v16(r, a, b)

int16x8_t __builtin_mpl_vector_addhn_high_i16v8(int16x4_t r, int32x4_t a, int32x4_t b);
#define vaddhn_high_s32(r, a, b)  __builtin_mpl_vector_addhn_high_i16v8(r, a, b)

int32x4_t __builtin_mpl_vector_addhn_high_i32v4(int32x2_t r, int64x2_t a, int64x2_t b);
#define vaddhn_high_s64(r, a, b)  __builtin_mpl_vector_addhn_high_i32v4(r, a, b)

uint8x16_t __builtin_mpl_vector_addhn_high_u8v16(uint8x8_t r, uint16x8_t a, uint16x8_t b);
#define vaddhn_high_u16(r, a, b)  __builtin_mpl_vector_addhn_high_u8v16(r, a, b)

uint16x8_t __builtin_mpl_vector_addhn_high_u16v8(uint16x4_t r, uint32x4_t a, uint32x4_t b);
#define vaddhn_high_u32(r, a, b)  __builtin_mpl_vector_addhn_high_u16v8(r, a, b)

uint32x4_t __builtin_mpl_vector_addhn_high_u32v4(uint32x2_t r, uint64x2_t a, uint64x2_t b);
#define vaddhn_high_u64(r, a, b)  __builtin_mpl_vector_addhn_high_u32v4(r, a, b)

int8x8_t __builtin_mpl_vector_raddhn_i8v8(int16x8_t a, int16x8_t b);
#define vraddhn_s16(a, b)  __builtin_mpl_vector_raddhn_i8v8(a, b)

int16x4_t __builtin_mpl_vector_raddhn_i16v4(int32x4_t a, int32x4_t b);
#define vraddhn_s32(a, b)  __builtin_mpl_vector_raddhn_i16v4(a, b)

int32x2_t __builtin_mpl_vector_raddhn_i32v2(int64x2_t a, int64x2_t b);
#define vraddhn_s64(a, b)  __builtin_mpl_vector_raddhn_i32v2(a, b)

uint8x8_t __builtin_mpl_vector_raddhn_u8v8(uint16x8_t a, uint16x8_t b);
#define vraddhn_u16(a, b)  __builtin_mpl_vector_raddhn_u8v8(a, b)

uint16x4_t __builtin_mpl_vector_raddhn_u16v4(uint32x4_t a, uint32x4_t b);
#define vraddhn_u32(a, b)  __builtin_mpl_vector_raddhn_u16v4(a, b)

uint32x2_t __builtin_mpl_vector_raddhn_u32v2(uint64x2_t a, uint64x2_t b);
#define vraddhn_u64(a, b)  __builtin_mpl_vector_raddhn_u32v2(a, b)

int8x16_t __builtin_mpl_vector_raddhn_high_i8v16(int8x8_t r, int16x8_t a, int16x8_t b);
#define vraddhn_high_s16(r, a, b)  __builtin_mpl_vector_raddhn_high_i8v16(r, a, b)

int16x8_t __builtin_mpl_vector_raddhn_high_i16v8(int16x4_t r, int32x4_t a, int32x4_t b);
#define vraddhn_high_s32(r, a, b)  __builtin_mpl_vector_raddhn_high_i16v8(r, a, b)

int32x4_t __builtin_mpl_vector_raddhn_high_i32v4(int32x2_t r, int64x2_t a, int64x2_t b);
#define vraddhn_high_s64(r, a, b)  __builtin_mpl_vector_raddhn_high_i32v4(r, a, b)

uint8x16_t __builtin_mpl_vector_raddhn_high_u8v16(uint8x8_t r, uint16x8_t a, uint16x8_t b);
#define vraddhn_high_u16(r, a, b)  __builtin_mpl_vector_raddhn_high_u8v16(r, a, b)

uint16x8_t __builtin_mpl_vector_raddhn_high_u16v8(uint16x4_t r, uint32x4_t a, uint32x4_t b);
#define vraddhn_high_u32(r, a, b)  __builtin_mpl_vector_raddhn_high_u16v8(r, a, b)

uint32x4_t __builtin_mpl_vector_raddhn_high_u32v4(uint32x2_t r, uint64x2_t a, uint64x2_t b);
#define vraddhn_high_u64(r, a, b)  __builtin_mpl_vector_raddhn_high_u32v4(r, a, b)

int8x8_t __builtin_mpl_vector_qadd_i8v8(int8x8_t a, int8x8_t b);
#define vqadd_s8(a, b)  __builtin_mpl_vector_qadd_i8v8(a, b)

int8x16_t __builtin_mpl_vector_qaddq_i8v16(int8x16_t a, int8x16_t b);
#define vqaddq_s8(a, b)  __builtin_mpl_vector_qaddq_i8v16(a, b)

int16x4_t __builtin_mpl_vector_qadd_i16v4(int16x4_t a, int16x4_t b);
#define vqadd_s16(a, b)  __builtin_mpl_vector_qadd_i16v4(a, b)

int16x8_t __builtin_mpl_vector_qaddq_i16v8(int16x8_t a, int16x8_t b);
#define vqaddq_s16(a, b)  __builtin_mpl_vector_qaddq_i16v8(a, b)

int32x2_t __builtin_mpl_vector_qadd_i32v2(int32x2_t a, int32x2_t b);
#define vqadd_s32(a, b)  __builtin_mpl_vector_qadd_i32v2(a, b)

int32x4_t __builtin_mpl_vector_qaddq_i32v4(int32x4_t a, int32x4_t b);
#define vqaddq_s32(a, b)  __builtin_mpl_vector_qaddq_i32v4(a, b)

int64x1_t __builtin_mpl_vector_qadd_i64v1(int64x1_t a, int64x1_t b);
#define vqadd_s64(a, b)  __builtin_mpl_vector_qadd_i64v1(a, b)

int64x2_t __builtin_mpl_vector_qaddq_i64v2(int64x2_t a, int64x2_t b);
#define vqaddq_s64(a, b)  __builtin_mpl_vector_qaddq_i64v2(a, b)

uint8x8_t __builtin_mpl_vector_qadd_u8v8(uint8x8_t a, uint8x8_t b);
#define vqadd_u8(a, b)  __builtin_mpl_vector_qadd_u8v8(a, b)

uint8x16_t __builtin_mpl_vector_qaddq_u8v16(uint8x16_t a, uint8x16_t b);
#define vqaddq_u8(a, b)  __builtin_mpl_vector_qaddq_u8v16(a, b)

uint16x4_t __builtin_mpl_vector_qadd_u16v4(uint16x4_t a, uint16x4_t b);
#define vqadd_u16(a, b)  __builtin_mpl_vector_qadd_u16v4(a, b)

uint16x8_t __builtin_mpl_vector_qaddq_u16v8(uint16x8_t a, uint16x8_t b);
#define vqaddq_u16(a, b)  __builtin_mpl_vector_qaddq_u16v8(a, b)

uint32x2_t __builtin_mpl_vector_qadd_u32v2(uint32x2_t a, uint32x2_t b);
#define vqadd_u32(a, b)  __builtin_mpl_vector_qadd_u32v2(a, b)

uint32x4_t __builtin_mpl_vector_qaddq_u32v4(uint32x4_t a, uint32x4_t b);
#define vqaddq_u32(a, b)  __builtin_mpl_vector_qaddq_u32v4(a, b)

uint64x1_t __builtin_mpl_vector_qadd_u64v1(uint64x1_t a, uint64x1_t b);
#define vqadd_u64(a, b)  __builtin_mpl_vector_qadd_u64v1(a, b)

uint64x2_t __builtin_mpl_vector_qaddq_u64v2(uint64x2_t a, uint64x2_t b);
#define vqaddq_u64(a, b)  __builtin_mpl_vector_qaddq_u64v2(a, b)

static inline int8_t __attribute__ ((__always_inline__, __gnu_inline__))
vqaddb_s8(int8_t a, int8_t b) {
  return vget_lane_s8(vqadd_s8((int8x8_t){a}, (int8x8_t){b}), 0);
}

static inline int16_t __attribute__ ((__always_inline__, __gnu_inline__))
vqaddh_s16(int16_t a, int16_t b) {
  return vget_lane_s16(vqadd_s16((int16x4_t){a}, (int16x4_t){b}), 0);
}

static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqadds_s32(int32_t a, int32_t b) {
  return vget_lane_s32(vqadd_s32((int32x2_t){a}, (int32x2_t){b}), 0);
}

static inline int64_t __attribute__ ((__always_inline__, __gnu_inline__))
vqaddd_s64(int64_t a, int64_t b) {
  return vget_lane_s64(vqadd_s64((int64x1_t){a}, (int64x1_t){b}), 0);
}

static inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__))
vqaddb_u8(uint8_t a, uint8_t b) {
  return vget_lane_u8(vqadd_u8((uint8x8_t){a}, (uint8x8_t){b}), 0);
}

static inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__))
vqaddh_u16(uint16_t a, uint16_t b) {
  return vget_lane_u16(vqadd_u16((uint16x4_t){a}, (uint16x4_t){b}), 0);
}

static inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqadds_u32(uint32_t a, uint32_t b) {
  return vget_lane_u32(vqadd_u32((uint32x2_t){a}, (uint32x2_t){b}), 0);
}

static inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__))
vqaddd_u64(uint64_t a, uint64_t b) {
  return vget_lane_u64(vqadd_u64((uint64x1_t){a}, (uint64x1_t){b}), 0);
}

int8x8_t __builtin_mpl_vector_uqadd_i8v8(int8x8_t a, uint8x8_t b);
#define vuqadd_s8(a, b)  __builtin_mpl_vector_uqadd_i8v8(a, b)

int8x16_t __builtin_mpl_vector_uqaddq_i8v16(int8x16_t a, uint8x16_t b);
#define vuqaddq_s8(a, b)  __builtin_mpl_vector_uqaddq_i8v16(a, b)

int16x4_t __builtin_mpl_vector_uqadd_i16v4(int16x4_t a, uint16x4_t b);
#define vuqadd_s16(a, b)  __builtin_mpl_vector_uqadd_i16v4(a, b)

int16x8_t __builtin_mpl_vector_uqaddq_i16v8(int16x8_t a, uint16x8_t b);
#define vuqaddq_s16(a, b)  __builtin_mpl_vector_uqaddq_i16v8(a, b)

int32x2_t __builtin_mpl_vector_uqadd_i32v2(int32x2_t a, uint32x2_t b);
#define vuqadd_s32(a, b)  __builtin_mpl_vector_uqadd_i32v2(a, b)

int32x4_t __builtin_mpl_vector_uqaddq_i32v4(int32x4_t a, uint32x4_t b);
#define vuqaddq_s32(a, b)  __builtin_mpl_vector_uqaddq_i32v4(a, b)

int64x1_t __builtin_mpl_vector_uqadd_i64v1(int64x1_t a, uint64x1_t b);
#define vuqadd_s64(a, b)  __builtin_mpl_vector_uqadd_i64v1(a, b)

int64x2_t __builtin_mpl_vector_uqaddq_i64v2(int64x2_t a, uint64x2_t b);
#define vuqaddq_s64(a, b)  __builtin_mpl_vector_uqaddq_i64v2(a, b)

static inline int8_t __attribute__ ((__always_inline__, __gnu_inline__))
vuqaddb_s8(int8_t a, int8_t b) {
  return vget_lane_s8(vuqadd_s8((int8x8_t){a}, (int8x8_t){b}), 0);
}

static inline int16_t __attribute__ ((__always_inline__, __gnu_inline__))
vuqaddh_s16(int16_t a, int16_t b) {
  return vget_lane_s16(vuqadd_s16((int16x4_t){a}, (int16x4_t){b}), 0);
}

static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__))
vuqadds_s32(int32_t a, int32_t b) {
  return vget_lane_s32(vuqadd_s32((int32x2_t){a}, (int32x2_t){b}), 0);
}

static inline int64_t __attribute__ ((__always_inline__, __gnu_inline__))
vuqaddd_s64(int64_t a, int64_t b) {
  return vget_lane_s64(vuqadd_s64((int64x1_t){a}, (int64x1_t){b}), 0);
}

uint8x8_t __builtin_mpl_vector_sqadd_u8v8(uint8x8_t a, int8x8_t b);
#define vsqadd_u8(a, b)  __builtin_mpl_vector_sqadd_u8v8(a, b)

uint8x16_t __builtin_mpl_vector_sqaddq_u8v16(uint8x16_t a, int8x16_t b);
#define vsqaddq_u8(a, b)  __builtin_mpl_vector_sqaddq_u8v16(a, b)

uint16x4_t __builtin_mpl_vector_sqadd_u16v4(uint16x4_t a, int16x4_t b);
#define vsqadd_u16(a, b)  __builtin_mpl_vector_sqadd_u16v4(a, b)

uint16x8_t __builtin_mpl_vector_sqaddq_u16v8(uint16x8_t a, int16x8_t b);
#define vsqaddq_u16(a, b)  __builtin_mpl_vector_sqaddq_u16v8(a, b)

uint32x2_t __builtin_mpl_vector_sqadd_u32v2(uint32x2_t a, int32x2_t b);
#define vsqadd_u32(a, b)  __builtin_mpl_vector_sqadd_u32v2(a, b)

uint32x4_t __builtin_mpl_vector_sqaddq_u32v4(uint32x4_t a, int32x4_t b);
#define vsqaddq_u32(a, b)  __builtin_mpl_vector_sqaddq_u32v4(a, b)

uint64x1_t __builtin_mpl_vector_sqadd_u64v1(uint64x1_t a, int64x1_t b);
#define vsqadd_u64(a, b)  __builtin_mpl_vector_sqadd_u64v1(a, b)

uint64x2_t __builtin_mpl_vector_sqaddq_u64v2(uint64x2_t a, int64x2_t b);
#define vsqaddq_u64(a, b)  __builtin_mpl_vector_sqaddq_u64v2(a, b)

static inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__))
vsqaddb_u8(uint8_t a, uint8_t b) {
  return vget_lane_u8(vsqadd_u8((uint8x8_t){a}, (uint8x8_t){b}), 0);
}

static inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__))
vsqaddh_u16(uint16_t a, uint16_t b) {
  return vget_lane_u16(vsqadd_u16((uint16x4_t){a}, (uint16x4_t){b}), 0);
}

static inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__))
vsqadds_u32(uint32_t a, uint32_t b) {
  return vget_lane_u32(vsqadd_u32((uint32x2_t){a}, (uint32x2_t){b}), 0);
}

static inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__))
vsqaddd_u64(uint64_t a, uint64_t b) {
  return vget_lane_u64(vsqadd_u64((uint64x1_t){a}, (uint64x1_t){b}), 0);
}

int8x8_t __builtin_mpl_vector_mla_i8v8(int8x8_t a, int8x8_t b, int8x8_t c);
#define vmla_s8(a, b, c)  __builtin_mpl_vector_mla_i8v8(a, b, c)

int8x16_t __builtin_mpl_vector_mlaq_i8v16(int8x16_t a, int8x16_t b, int8x16_t c);
#define vmlaq_s8(a, b, c)  __builtin_mpl_vector_mlaq_i8v16(a, b, c)

int16x4_t __builtin_mpl_vector_mla_i16v4(int16x4_t a, int16x4_t b, int16x4_t c);
#define vmla_s16(a, b, c)  __builtin_mpl_vector_mla_i16v4(a, b, c)

int16x8_t __builtin_mpl_vector_mlaq_i16v8(int16x8_t a, int16x8_t b, int16x8_t c);
#define vmlaq_s16(a, b, c)  __builtin_mpl_vector_mlaq_i16v8(a, b, c)

int32x2_t __builtin_mpl_vector_mla_i32v2(int32x2_t a, int32x2_t b, int32x2_t c);
#define vmla_s32(a, b, c)  __builtin_mpl_vector_mla_i32v2(a, b, c)

int32x4_t __builtin_mpl_vector_mlaq_i32v4(int32x4_t a, int32x4_t b, int32x4_t c);
#define vmlaq_s32(a, b, c)  __builtin_mpl_vector_mlaq_i32v4(a, b, c)

uint8x8_t __builtin_mpl_vector_mla_u8v8(uint8x8_t a, uint8x8_t b, uint8x8_t c);
#define vmla_u8(a, b, c)  __builtin_mpl_vector_mla_u8v8(a, b, c)

uint8x16_t __builtin_mpl_vector_mlaq_u8v16(uint8x16_t a, uint8x16_t b, uint8x16_t c);
#define vmlaq_u8(a, b, c)  __builtin_mpl_vector_mlaq_u8v16(a, b, c)

uint16x4_t __builtin_mpl_vector_mla_u16v4(uint16x4_t a, uint16x4_t b, uint16x4_t c);
#define vmla_u16(a, b, c)  __builtin_mpl_vector_mla_u16v4(a, b, c)

uint16x8_t __builtin_mpl_vector_mlaq_u16v8(uint16x8_t a, uint16x8_t b, uint16x8_t c);
#define vmlaq_u16(a, b, c)  __builtin_mpl_vector_mlaq_u16v8(a, b, c)

uint32x2_t __builtin_mpl_vector_mla_u32v2(uint32x2_t a, uint32x2_t b, uint32x2_t c);
#define vmla_u32(a, b, c)  __builtin_mpl_vector_mla_u32v2(a, b, c)

uint32x4_t __builtin_mpl_vector_mlaq_u32v4(uint32x4_t a, uint32x4_t b, uint32x4_t c);
#define vmlaq_u32(a, b, c)  __builtin_mpl_vector_mlaq_u32v4(a, b, c)

int8x8_t __builtin_mpl_vector_mls_i8v8(int8x8_t a, int8x8_t b, int8x8_t c);
#define vmls_s8(a, b, c)  __builtin_mpl_vector_mls_i8v8(a, b, c)

int8x16_t __builtin_mpl_vector_mlsq_i8v16(int8x16_t a, int8x16_t b, int8x16_t c);
#define vmlsq_s8(a, b, c)  __builtin_mpl_vector_mlsq_i8v16(a, b, c)

int16x4_t __builtin_mpl_vector_mls_i16v4(int16x4_t a, int16x4_t b, int16x4_t c);
#define vmls_s16(a, b, c)  __builtin_mpl_vector_mls_i16v4(a, b, c)

int16x8_t __builtin_mpl_vector_mlsq_i16v8(int16x8_t a, int16x8_t b, int16x8_t c);
#define vmlsq_s16(a, b, c)  __builtin_mpl_vector_mlsq_i16v8(a, b, c)

int32x2_t __builtin_mpl_vector_mls_i32v2(int32x2_t a, int32x2_t b, int32x2_t c);
#define vmls_s32(a, b, c)  __builtin_mpl_vector_mls_i32v2(a, b, c)

int32x4_t __builtin_mpl_vector_mlsq_i32v4(int32x4_t a, int32x4_t b, int32x4_t c);
#define vmlsq_s32(a, b, c)  __builtin_mpl_vector_mlsq_i32v4(a, b, c)

uint8x8_t __builtin_mpl_vector_mls_u8v8(uint8x8_t a, uint8x8_t b, uint8x8_t c);
#define vmls_u8(a, b, c)  __builtin_mpl_vector_mls_u8v8(a, b, c)

uint8x16_t __builtin_mpl_vector_mlsq_u8v16(uint8x16_t a, uint8x16_t b, uint8x16_t c);
#define vmlsq_u8(a, b, c)  __builtin_mpl_vector_mlsq_u8v16(a, b, c)

uint16x4_t __builtin_mpl_vector_mls_u16v4(uint16x4_t a, uint16x4_t b, uint16x4_t c);
#define vmls_u16(a, b, c)  __builtin_mpl_vector_mls_u16v4(a, b, c)

uint16x8_t __builtin_mpl_vector_mlsq_u16v8(uint16x8_t a, uint16x8_t b, uint16x8_t c);
#define vmlsq_u16(a, b, c)  __builtin_mpl_vector_mlsq_u16v8(a, b, c)

uint32x2_t __builtin_mpl_vector_mls_u32v2(uint32x2_t a, uint32x2_t b, uint32x2_t c);
#define vmls_u32(a, b, c)  __builtin_mpl_vector_mls_u32v2(a, b, c)

uint32x4_t __builtin_mpl_vector_mlsq_u32v4(uint32x4_t a, uint32x4_t b, uint32x4_t c);
#define vmlsq_u32(a, b, c)  __builtin_mpl_vector_mlsq_u32v4(a, b, c)

int16x8_t __builtin_mpl_vector_mlal_i16v8(int16x8_t a, int8x8_t b, int8x8_t c);
#define vmlal_s8(a, b, c)  __builtin_mpl_vector_mlal_i16v8(a, b, c)

int32x4_t __builtin_mpl_vector_mlal_i32v4(int32x4_t a, int16x4_t b, int16x4_t c);
#define vmlal_s16(a, b, c)  __builtin_mpl_vector_mlal_i32v4(a, b, c)

int64x2_t __builtin_mpl_vector_mlal_i64v2(int64x2_t a, int32x2_t b, int32x2_t c);
#define vmlal_s32(a, b, c)  __builtin_mpl_vector_mlal_i64v2(a, b, c)

uint16x8_t __builtin_mpl_vector_mlal_u16v8(uint16x8_t a, uint8x8_t b, uint8x8_t c);
#define vmlal_u8(a, b, c)  __builtin_mpl_vector_mlal_u16v8(a, b, c)

uint32x4_t __builtin_mpl_vector_mlal_u32v4(uint32x4_t a, uint16x4_t b, uint16x4_t c);
#define vmlal_u16(a, b, c)  __builtin_mpl_vector_mlal_u32v4(a, b, c)

uint64x2_t __builtin_mpl_vector_mlal_u64v2(uint64x2_t a, uint32x2_t b, uint32x2_t c);
#define vmlal_u32(a, b, c)  __builtin_mpl_vector_mlal_u64v2(a, b, c)

int16x8_t __builtin_mpl_vector_mlal_high_i16v8(int16x8_t a, int8x16_t b, int8x16_t c);
#define vmlal_high_s8(a, b, c)  __builtin_mpl_vector_mlal_high_i16v8(a, b, c)

int32x4_t __builtin_mpl_vector_mlal_high_i32v4(int32x4_t a, int16x8_t b, int16x8_t c);
#define vmlal_high_s16(a, b, c)  __builtin_mpl_vector_mlal_high_i32v4(a, b, c)

int64x2_t __builtin_mpl_vector_mlal_high_i64v2(int64x2_t a, int32x4_t b, int32x4_t c);
#define vmlal_high_s32(a, b, c)  __builtin_mpl_vector_mlal_high_i64v2(a, b, c)

uint16x8_t __builtin_mpl_vector_mlal_high_u16v8(uint16x8_t a, uint8x16_t b, uint8x16_t c);
#define vmlal_high_u8(a, b, c)  __builtin_mpl_vector_mlal_high_u16v8(a, b, c)

uint32x4_t __builtin_mpl_vector_mlal_high_u32v4(uint32x4_t a, uint16x8_t b, uint16x8_t c);
#define vmlal_high_u16(a, b, c)  __builtin_mpl_vector_mlal_high_u32v4(a, b, c)

uint64x2_t __builtin_mpl_vector_mlal_high_u64v2(uint64x2_t a, uint32x4_t b, uint32x4_t c);
#define vmlal_high_u32(a, b, c)  __builtin_mpl_vector_mlal_high_u64v2(a, b, c)

int16x8_t __builtin_mpl_vector_mlsl_i16v8(int16x8_t a, int8x8_t b, int8x8_t c);
#define vmlsl_s8(a, b, c)  __builtin_mpl_vector_mlsl_i16v8(a, b, c)

int32x4_t __builtin_mpl_vector_mlsl_i32v4(int32x4_t a, int16x4_t b, int16x4_t c);
#define vmlsl_s16(a, b, c)  __builtin_mpl_vector_mlsl_i32v4(a, b, c)

int64x2_t __builtin_mpl_vector_mlsl_i64v2(int64x2_t a, int32x2_t b, int32x2_t c);
#define vmlsl_s32(a, b, c)  __builtin_mpl_vector_mlsl_i64v2(a, b, c)

uint16x8_t __builtin_mpl_vector_mlsl_u16v8(uint16x8_t a, uint8x8_t b, uint8x8_t c);
#define vmlsl_u8(a, b, c)  __builtin_mpl_vector_mlsl_u16v8(a, b, c)

uint32x4_t __builtin_mpl_vector_mlsl_u32v4(uint32x4_t a, uint16x4_t b, uint16x4_t c);
#define vmlsl_u16(a, b, c)  __builtin_mpl_vector_mlsl_u32v4(a, b, c)

uint64x2_t __builtin_mpl_vector_mlsl_u64v2(uint64x2_t a, uint32x2_t b, uint32x2_t c);
#define vmlsl_u32(a, b, c)  __builtin_mpl_vector_mlsl_u64v2(a, b, c)

int16x8_t __builtin_mpl_vector_mlsl_high_i16v8(int16x8_t a, int8x16_t b, int8x16_t c);
#define vmlsl_high_s8(a, b, c)  __builtin_mpl_vector_mlsl_high_i16v8(a, b, c)

int32x4_t __builtin_mpl_vector_mlsl_high_i32v4(int32x4_t a, int16x8_t b, int16x8_t c);
#define vmlsl_high_s16(a, b, c)  __builtin_mpl_vector_mlsl_high_i32v4(a, b, c)

int64x2_t __builtin_mpl_vector_mlsl_high_i64v2(int64x2_t a, int32x4_t b, int32x4_t c);
#define vmlsl_high_s32(a, b, c)  __builtin_mpl_vector_mlsl_high_i64v2(a, b, c)

uint16x8_t __builtin_mpl_vector_mlsl_high_u16v8(uint16x8_t a, uint8x16_t b, uint8x16_t c);
#define vmlsl_high_u8(a, b, c)  __builtin_mpl_vector_mlsl_high_u16v8(a, b, c)

uint32x4_t __builtin_mpl_vector_mlsl_high_u32v4(uint32x4_t a, uint16x8_t b, uint16x8_t c);
#define vmlsl_high_u16(a, b, c)  __builtin_mpl_vector_mlsl_high_u32v4(a, b, c)

uint64x2_t __builtin_mpl_vector_mlsl_high_u64v2(uint64x2_t a, uint32x4_t b, uint32x4_t c);
#define vmlsl_high_u32(a, b, c)  __builtin_mpl_vector_mlsl_high_u64v2(a, b, c)

int16x4_t __builtin_mpl_vector_qdmulh_i16v4(int16x4_t a, int16x4_t b);
#define vqdmulh_s16(a, b)  __builtin_mpl_vector_qdmulh_i16v4(a, b)

int16x8_t __builtin_mpl_vector_qdmulhq_i16v8(int16x8_t a, int16x8_t b);
#define vqdmulhq_s16(a, b)  __builtin_mpl_vector_qdmulhq_i16v8(a, b)

int32x2_t __builtin_mpl_vector_qdmulh_i32v2(int32x2_t a, int32x2_t b);
#define vqdmulh_s32(a, b)  __builtin_mpl_vector_qdmulh_i32v2(a, b)

int32x4_t __builtin_mpl_vector_qdmulhq_i32v4(int32x4_t a, int32x4_t b);
#define vqdmulhq_s32(a, b)  __builtin_mpl_vector_qdmulhq_i32v4(a, b)

static inline int16_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmulhh_s16(int16_t a, int16_t b) {
  return vget_lane_s16(vqdmulh_s16((int16x4_t){a}, (int16x4_t){b}), 0);
}

static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmulhs_s32(int32_t a, int32_t b) {
  return vget_lane_s32(vqdmulh_s32((int32x2_t){a}, (int32x2_t){b}), 0);
}

int16x4_t __builtin_mpl_vector_qrdmulh_i16v4(int16x4_t a, int16x4_t b);
#define vqrdmulh_s16(a, b)  __builtin_mpl_vector_qrdmulh_i16v4(a, b)

int16x8_t __builtin_mpl_vector_qrdmulhq_i16v8(int16x8_t a, int16x8_t b);
#define vqrdmulhq_s16(a, b)  __builtin_mpl_vector_qrdmulhq_i16v8(a, b)

int32x2_t __builtin_mpl_vector_qrdmulh_i32v2(int32x2_t a, int32x2_t b);
#define vqrdmulh_s32(a, b)  __builtin_mpl_vector_qrdmulh_i32v2(a, b)

int32x4_t __builtin_mpl_vector_qrdmulhq_i32v4(int32x4_t a, int32x4_t b);
#define vqrdmulhq_s32(a, b)  __builtin_mpl_vector_qrdmulhq_i32v4(a, b)

static inline int16_t __attribute__ ((__always_inline__, __gnu_inline__))
vqrdmulhh_s16(int16_t a, int16_t b) {
  return vget_lane_s16(vqrdmulh_s16((int16x4_t){a}, (int16x4_t){b}), 0);
}

static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqrdmulhs_s32(int32_t a, int32_t b) {
  return vget_lane_s32(vqrdmulh_s32((int32x2_t){a}, (int32x2_t){b}), 0);
}

int32x4_t __builtin_mpl_vector_qdmull_i32v4(int16x4_t a, int16x4_t b);
#define vqdmull_s16(a, b)  __builtin_mpl_vector_qdmull_i32v4(a, b)

int64x2_t __builtin_mpl_vector_qdmull_i64v2(int32x2_t a, int32x2_t b);
#define vqdmull_s32(a, b)  __builtin_mpl_vector_qdmull_i64v2(a, b)

static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmullh_s16(int16_t a, int16_t b) {
  return vgetq_lane_s32(vqdmull_s16((int16x4_t){a}, (int16x4_t){b}), 0);
}

static inline int64_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmulls_s32(int32_t a, int32_t b) {
  return vgetq_lane_s64(vqdmull_s32((int32x2_t){a}, (int32x2_t){b}), 0);
}

int32x4_t __builtin_mpl_vector_qdmull_high_i32v4(int16x8_t a, int16x8_t b);
#define vqdmull_high_s16(a, b)  __builtin_mpl_vector_qdmull_high_i32v4(a, b)

int64x2_t __builtin_mpl_vector_qdmull_high_i64v2(int32x4_t a, int32x4_t b);
#define vqdmull_high_s32(a, b)  __builtin_mpl_vector_qdmull_high_i64v2(a, b)

int32x4_t __builtin_mpl_vector_qdmlal_i32v4(int32x4_t a, int16x4_t b, int16x4_t c);
#define vqdmlal_s16(a, b, c)  __builtin_mpl_vector_qdmlal_i32v4(a, b, c)

int64x2_t __builtin_mpl_vector_qdmlal_i64v2(int64x2_t a, int32x2_t b, int32x2_t c);
#define vqdmlal_s32(a, b, c)  __builtin_mpl_vector_qdmlal_i64v2(a, b, c)

static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmlalh_s16(int32_t a, int16_t b, int16_t c) {
  return vgetq_lane_s32(vqdmlal_s16((int32x4_t){a}, (int16x4_t){b}, (int16x4_t){c}), 0);
}

static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmlals_s32(int32_t a, int16_t b, int16_t c) {
  return vgetq_lane_s64(vqdmlal_s32((int64x2_t){a}, (int32x2_t){b}, (int32x2_t){c}), 0);
}

int32x4_t __builtin_mpl_vector_qdmlal_high_i32v4(int32x4_t a, int16x8_t b, int16x8_t c);
#define vqdmlal_high_s16(a, b, c)  __builtin_mpl_vector_qdmlal_high_i32v4(a, b, c)

int64x2_t __builtin_mpl_vector_qdmlal_high_i64v2(int64x2_t a, int32x4_t b, int32x4_t c);
#define vqdmlal_high_s32(a, b, c)  __builtin_mpl_vector_qdmlal_high_i64v2(a, b, c)

int32x4_t __builtin_mpl_vector_qdmlsl_i32v4(int32x4_t a, int16x4_t b, int16x4_t c);
#define vqdmlsl_s16(a, b, c)  __builtin_mpl_vector_qdmlsl_i32v4(a, b, c)

int64x2_t __builtin_mpl_vector_qdmlsl_i64v2(int64x2_t a, int32x2_t b, int32x2_t c);
#define vqdmlsl_s32(a, b, c)  __builtin_mpl_vector_qdmlsl_i64v2(a, b, c)

static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmlslh_s16(int32_t a, int16_t b, int16_t c) {
  return vgetq_lane_s32(vqdmlsl_s16((int32x4_t){a}, (int16x4_t){b}, (int16x4_t){c}), 0);
}

static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmlsls_s32(int32_t a, int16_t b, int16_t c) {
  return vgetq_lane_s64(vqdmlsl_s32((int64x2_t){a}, (int32x2_t){b}, (int32x2_t){c}), 0);
}

int32x4_t __builtin_mpl_vector_qdmlsl_high_i32v4(int32x4_t a, int16x8_t b, int16x8_t c);
#define vqdmlsl_high_s16(a, b, c)  __builtin_mpl_vector_qdmlsl_high_i32v4(a, b, c)

int64x2_t __builtin_mpl_vector_qdmlsl_high_i64v2(int64x2_t a, int32x4_t b, int32x4_t c);
#define vqdmlsl_high_s32(a, b, c)  __builtin_mpl_vector_qdmlsl_high_i64v2(a, b, c)

int32x4_t __builtin_mpl_vector_qdmlal_lane_i32v4(int32x4_t a, int16x4_t b, int16x4_t v, const int lane);
#define vqdmlal_lane_s16(a, b, v, lane)  __builtin_mpl_vector_qdmlal_lane_i32v4(a, b, v, lane)

int64x2_t __builtin_mpl_vector_qdmlal_lane_i64v2(int64x2_t a, int32x2_t b, int32x2_t v, const int lane);
#define vqdmlal_lane_s32(a, b, v, lane)  __builtin_mpl_vector_qdmlal_lane_i64v2(a, b, v, lane)

static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmlalh_lane_s16(int32_t a, int16_t b, int16x4_t v, const int lane) {
  return vgetq_lane_s32(vqdmlal_lane_s16((int32x4_t){a}, (int16x4_t){b}, v, lane), 0);
}

static inline int64_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmlals_lane_s32(int64_t a, int32_t b, int32x2_t v, const int lane) {
  return vgetq_lane_s64(vqdmlal_lane_s32((int64x2_t){a}, (int32x2_t){b}, v, lane), 0);
}

int32x4_t __builtin_mpl_vector_qdmlal_high_lane_i32v4(int32x4_t a, int16x8_t b, int16x4_t v, const int lane);
#define vqdmlal_high_lane_s16(a, b, v, lane)  __builtin_mpl_vector_qdmlal_high_lane_i32v4(a, b, v, lane)

int64x2_t __builtin_mpl_vector_qdmlal_high_lane_i64v2(int64x2_t a, int32x4_t b, int32x2_t v, const int lane);
#define vqdmlal_high_lane_s32(a, b, v, lane)  __builtin_mpl_vector_qdmlal_high_lane_i64v2(a, b, v, lane)

int32x4_t __builtin_mpl_vector_qdmlal_laneq_i32v4(int32x4_t a, int16x4_t b, int16x8_t v, const int lane);
#define vqdmlal_laneq_s16(a, b, v, lane)  __builtin_mpl_vector_qdmlal_laneq_i32v4(a, b, v, lane)

int64x2_t __builtin_mpl_vector_qdmlal_laneq_i64v2(int64x2_t a, int32x2_t b, int32x4_t v, const int lane);
#define vqdmlal_laneq_s32(a, b, v, lane)  __builtin_mpl_vector_qdmlal_laneq_i64v2(a, b, v, lane)

static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmlalh_laneq_s16(int32_t a, int16_t b, int16x8_t v, const int lane) {
  return vgetq_lane_s32(vqdmlal_laneq_s16((int32x4_t){a}, (int16x4_t){b}, v, lane), 0);
}

static inline int64_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmlals_laneq_s32(int64_t a, int32_t b, int32x4_t v, const int lane) {
  return vgetq_lane_s64(vqdmlal_laneq_s32((int64x2_t){a}, (int32x2_t){b}, v, lane), 0);
}

int32x4_t __builtin_mpl_vector_qdmlal_high_laneq_i32v4(int32x4_t a, int16x8_t b, int16x8_t v, const int lane);
#define vqdmlal_high_laneq_s16(a, b, v, lane)  __builtin_mpl_vector_qdmlal_high_laneq_i32v4(a, b, v, lane)

int64x2_t __builtin_mpl_vector_qdmlal_high_laneq_i64v2(int64x2_t a, int32x4_t b, int32x4_t v, const int lane);
#define vqdmlal_high_laneq_s32(a, b, v, lane)  __builtin_mpl_vector_qdmlal_high_laneq_i64v2(a, b, v, lane)

int32x4_t __builtin_mpl_vector_qdmlsl_lane_i32v4(int32x4_t a, int16x4_t b, int16x4_t v, const int lane);
#define vqdmlsl_lane_s16(a, b, v, lane)  __builtin_mpl_vector_qdmlsl_lane_i32v4(a, b, v, lane)

int64x2_t __builtin_mpl_vector_qdmlsl_lane_i64v2(int64x2_t a, int32x2_t b, int32x2_t v, const int lane);
#define vqdmlsl_lane_s32(a, b, v, lane)  __builtin_mpl_vector_qdmlsl_lane_i64v2(a, b, v, lane)

static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmlslh_lane_s16(int32_t a, int16_t b, int16x4_t v, const int lane) {
  return vgetq_lane_s32(vqdmlsl_lane_s16((int32x4_t){a}, (int16x4_t){b}, v, lane), 0);
}

static inline int64_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmlsls_lane_s32(int64_t a, int32_t b, int32x2_t v, const int lane) {
  return vgetq_lane_s64(vqdmlsl_lane_s32((int64x2_t){a}, (int32x2_t){b}, v, lane), 0);
}

int32x4_t __builtin_mpl_vector_qdmlsl_high_lane_i32v4(int32x4_t a, int16x8_t b, int16x4_t v, const int lane);
#define vqdmlsl_high_lane_s16(a, b, v, lane)  __builtin_mpl_vector_qdmlsl_high_lane_i32v4(a, b, v, lane)

int64x2_t __builtin_mpl_vector_qdmlsl_high_lane_i64v2(int64x2_t a, int32x4_t b, int32x2_t v, const int lane);
#define vqdmlsl_high_lane_s32(a, b, v, lane)  __builtin_mpl_vector_qdmlsl_high_lane_i64v2(a, b, v, lane)

int32x4_t __builtin_mpl_vector_qdmlsl_laneq_i32v4(int32x4_t a, int16x4_t b, int16x8_t v, const int lane);
#define vqdmlsl_laneq_s16(a, b, v, lane)  __builtin_mpl_vector_qdmlsl_laneq_i32v4(a, b, v, lane)

int64x2_t __builtin_mpl_vector_qdmlsl_laneq_i64v2(int64x2_t a, int32x2_t b, int32x4_t v, const int lane);
#define vqdmlsl_laneq_s32(a, b, v, lane)  __builtin_mpl_vector_qdmlsl_laneq_i64v2(a, b, v, lane)

static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmlslh_laneq_s16(int32_t a, int16_t b, int16x8_t v, const int lane) {
  return vgetq_lane_s32(vqdmlsl_laneq_s16((int32x4_t){a}, (int16x4_t){b}, v, lane), 0);
}

static inline int64_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmlsls_laneq_s32(int64_t a, int32_t b, int32x4_t v, const int lane) {
  return vgetq_lane_s64(vqdmlsl_laneq_s32((int64x2_t){a}, (int32x2_t){b}, v, lane), 0);
}

int32x4_t __builtin_mpl_vector_qdmlsl_high_laneq_i32v4(int32x4_t a, int16x8_t b, int16x8_t v, const int lane);
#define vqdmlsl_high_laneq_s16(a, b, v, lane)  __builtin_mpl_vector_qdmlsl_high_laneq_i32v4(a, b, v, lane)

int64x2_t __builtin_mpl_vector_qdmlsl_high_laneq_i64v2(int64x2_t a, int32x4_t b, int32x4_t v, const int lane);
#define vqdmlsl_high_laneq_s32(a, b, v, lane)  __builtin_mpl_vector_qdmlsl_high_laneq_i64v2(a, b, v, lane)

int16x8_t __builtin_mpl_vector_mull_i16v8(int8x8_t a, int8x8_t b);
#define vmull_s8(a, b)  __builtin_mpl_vector_mull_i16v8(a, b)

int32x4_t __builtin_mpl_vector_mull_i32v4(int16x4_t a, int16x4_t b);
#define vmull_s16(a, b)  __builtin_mpl_vector_mull_i32v4(a, b)

int64x2_t __builtin_mpl_vector_mull_i64v2(int32x2_t a, int32x2_t b);
#define vmull_s32(a, b)  __builtin_mpl_vector_mull_i64v2(a, b)

uint16x8_t __builtin_mpl_vector_mull_u16v8(uint8x8_t a, uint8x8_t b);
#define vmull_u8(a, b)  __builtin_mpl_vector_mull_u16v8(a, b)

uint32x4_t __builtin_mpl_vector_mull_u32v4(uint16x4_t a, uint16x4_t b);
#define vmull_u16(a, b)  __builtin_mpl_vector_mull_u32v4(a, b)

uint64x2_t __builtin_mpl_vector_mull_u64v2(uint32x2_t a, uint32x2_t b);
#define vmull_u32(a, b)  __builtin_mpl_vector_mull_u64v2(a, b)

int16x8_t __builtin_mpl_vector_mull_high_i16v8(int8x16_t a, int8x16_t b);
#define vmull_high_s8(a, b)  __builtin_mpl_vector_mull_high_i16v8(a, b)

int32x4_t __builtin_mpl_vector_mull_high_i32v4(int16x8_t a, int16x8_t b);
#define vmull_high_s16(a, b)  __builtin_mpl_vector_mull_high_i32v4(a, b)

int64x2_t __builtin_mpl_vector_mull_high_i64v2(int32x4_t a, int32x4_t b);
#define vmull_high_s32(a, b)  __builtin_mpl_vector_mull_high_i64v2(a, b)

uint16x8_t __builtin_mpl_vector_mull_high_u16v8(uint8x16_t a, uint8x16_t b);
#define vmull_high_u8(a, b)  __builtin_mpl_vector_mull_high_u16v8(a, b)

uint32x4_t __builtin_mpl_vector_mull_high_u32v4(uint16x8_t a, uint16x8_t b);
#define vmull_high_u16(a, b)  __builtin_mpl_vector_mull_high_u32v4(a, b)

int32x4_t __builtin_mpl_vector_qdmull_n_i32v4(int16x4_t a, int16x4_t b);
static inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmull_n_s16(int16x4_t a, int16_t b) {
  return __builtin_mpl_vector_qdmull_n_i32v4(a, (int16x4_t){b});
}

int64x2_t __builtin_mpl_vector_qdmull_n_i64v2(int32x2_t a, int32x2_t b);
static inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmull_n_s32(int32x2_t a, int32_t b) {
  return __builtin_mpl_vector_qdmull_n_i64v2(a, (int32x2_t){b});
}

int32x4_t __builtin_mpl_vector_qdmull_high_n_i32v4(int16x8_t a, int16x8_t b);
static inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmull_high_n_s16(int16x8_t a, int16_t b) {
  return __builtin_mpl_vector_qdmull_high_n_i32v4(a, (int16x8_t){b});
}

int64x2_t __builtin_mpl_vector_qdmull_high_n_i64v2(int32x4_t a, int32x4_t b);
static inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmull_high_n_s32(int32x4_t a, int32_t b) {
  return __builtin_mpl_vector_qdmull_high_n_i64v2(a, (int32x4_t){b});
}

int32x4_t __builtin_mpl_vector_qdmull_lane_i32v4(int16x4_t a, int16x4_t v, const int lane);
#define vqdmull_lane_s16(a, v, lane)  __builtin_mpl_vector_qdmull_lane_i32v4(a, v, lane)

int64x2_t __builtin_mpl_vector_qdmull_lane_i64v2(int32x2_t a, int32x2_t v, const int lane);
#define vqdmull_lane_s32(a, v, lane)  __builtin_mpl_vector_qdmull_lane_i64v2(a, v, lane)

static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmullh_lane_s16(int16_t a, int16x4_t v, const int lane) {
  return vgetq_lane_s32(vqdmull_lane_s16((int16x4_t){a}, v, lane), 0);
}

static inline int64_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmulls_lane_s32(int32_t a, int32x2_t v, const int lane) {
  return vgetq_lane_s64(vqdmull_lane_s32((int32x2_t){a}, v, lane), 0);
}

int32x4_t __builtin_mpl_vector_qdmull_high_lane_i32v4(int16x8_t a, int16x4_t v, const int lane);
#define vqdmull_high_lane_s16(a, v, lane)  __builtin_mpl_vector_qdmull_high_lane_i32v4(a, v, lane)

int64x2_t __builtin_mpl_vector_qdmull_high_lane_i64v2(int32x4_t a, int32x2_t v, const int lane);
#define vqdmull_high_lane_s32(a, v, lane)  __builtin_mpl_vector_qdmull_high_lane_i64v2(a, v, lane)

int32x4_t __builtin_mpl_vector_qdmull_laneq_i32v4(int16x4_t a, int16x8_t v, const int lane);
#define vqdmull_laneq_s16(a, v, lane)  __builtin_mpl_vector_qdmull_laneq_i32v4(a, v, lane)

int64x2_t __builtin_mpl_vector_qdmull_laneq_i64v2(int32x2_t a, int32x4_t v, const int lane);
#define vqdmull_laneq_s32(a, v, lane)  __builtin_mpl_vector_qdmull_laneq_i64v2(a, v, lane)

static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmullh_laneq_s16(int16_t a, int16x8_t v, const int lane) {
  return vgetq_lane_s32(vqdmull_laneq_s16((int16x4_t){a}, v, lane), 0);
}

static inline int64_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmulls_laneq_s32(int32_t a, int32x4_t v, const int lane) {
  return vgetq_lane_s64(vqdmull_laneq_s32((int32x2_t){a}, v, lane), 0);
}

int32x4_t __builtin_mpl_vector_qdmull_high_laneq_i32v4(int16x8_t a, int16x8_t v, const int lane);
#define vqdmull_high_laneq_s16(a, v, lane)  __builtin_mpl_vector_qdmull_high_laneq_i32v4(a, v, lane)

int64x2_t __builtin_mpl_vector_qdmull_high_laneq_i64v2(int32x4_t a, int32x4_t v, const int lane);
#define vqdmull_high_laneq_s32(a, v, lane)  __builtin_mpl_vector_qdmull_high_laneq_i64v2(a, v, lane)

int16x4_t __builtin_mpl_vector_qdmulh_n_i16v4(int16x4_t a, int16x4_t b);
static inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmulh_n_s16(int16x4_t a, int16_t b) {
  return __builtin_mpl_vector_qdmulh_n_i16v4(a, (int16x4_t){b});
}

int16x8_t __builtin_mpl_vector_qdmulhq_n_i16v8(int16x8_t a, int16x8_t b);
static inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmulhq_n_s16(int16x8_t a, int16_t b) {
  return __builtin_mpl_vector_qdmulhq_n_i16v8(a, (int16x8_t){b});
}

int32x2_t __builtin_mpl_vector_qdmulh_n_i32v2(int32x2_t a, int32x2_t b);
static inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmulh_n_s32(int32x2_t a, int32_t b) {
  return __builtin_mpl_vector_qdmulh_n_i32v2(a, (int32x2_t){b});
}

int32x4_t __builtin_mpl_vector_qdmulhq_n_i32v4(int32x4_t a, int32x4_t b);
static inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmulhq_n_s32(int32x4_t a, int32_t b) {
  return __builtin_mpl_vector_qdmulhq_n_i32v4(a, (int32x4_t){b});
}

int16x4_t __builtin_mpl_vector_qdmulh_lane_i16v4(int16x4_t a, int16x4_t v, const int lane);
#define vqdmulh_lane_s16(a, v, lane)  __builtin_mpl_vector_qdmulh_lane_i16v4(a, v, lane)

int16x8_t __builtin_mpl_vector_qdmulhq_lane_i16v8(int16x8_t a, int16x4_t v, const int lane);
#define vqdmulhq_lane_s16(a, v, lane)  __builtin_mpl_vector_qdmulhq_lane_i16v8(a, v, lane)

int32x2_t __builtin_mpl_vector_qdmulh_lane_i32v2(int32x2_t a, int32x2_t v, const int lane);
#define vqdmulh_lane_s32(a, v, lane)  __builtin_mpl_vector_qdmulh_lane_i32v2(a, v, lane)

int32x4_t __builtin_mpl_vector_qdmulhq_lane_i32v4(int32x4_t a, int32x2_t v, const int lane);
#define vqdmulhq_lane_s32(a, v, lane)  __builtin_mpl_vector_qdmulhq_lane_i32v4(a, v, lane)

static inline int16_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmulhh_lane_s16(int16_t a, int16x4_t v, const int lane) {
  return vget_lane_s16(vqdmulh_lane_s16((int16x4_t){a}, v, lane), 0);
}

static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmulhs_lane_s32(int32_t a, int32x2_t v, const int lane) {
  return vget_lane_s32(vqdmulh_lane_s32((int32x2_t){a}, v, lane), 0);
}

int16x4_t __builtin_mpl_vector_qdmulh_laneq_i16v4(int16x4_t a, int16x8_t v, const int lane);
#define vqdmulh_laneq_s16(a, v, lane)  __builtin_mpl_vector_qdmulh_laneq_i16v4(a, v, lane)

int16x8_t __builtin_mpl_vector_qdmulhq_laneq_i16v8(int16x8_t a, int16x8_t v, const int lane);
#define vqdmulhq_laneq_s16(a, v, lane)  __builtin_mpl_vector_qdmulhq_laneq_i16v8(a, v, lane)

int32x2_t __builtin_mpl_vector_qdmulh_laneq_i32v2(int32x2_t a, int32x4_t v, const int lane);
#define vqdmulh_laneq_s32(a, v, lane)  __builtin_mpl_vector_qdmulh_laneq_i32v2(a, v, lane)

int32x4_t __builtin_mpl_vector_qdmulhq_laneq_i32v4(int32x4_t a, int32x4_t v, const int lane);
#define vqdmulhq_laneq_s32(a, v, lane)  __builtin_mpl_vector_qdmulhq_laneq_i32v4(a, v, lane)

static inline int16_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmulhh_laneq_s16(int16_t a, int16x8_t v, const int lane) {
  return vget_lane_s16(vqdmulh_laneq_s16((int16x4_t){a}, v, lane), 0);
}

static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmulhs_laneq_s32(int32_t a, int32x4_t v, const int lane) {
  return vget_lane_s32(vqdmulh_laneq_s32((int32x2_t){a}, v, lane), 0);
}

int16x4_t __builtin_mpl_vector_qrdmulh_n_i16v4(int16x4_t a, int16x4_t b);
static inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__))
vqrdmulh_n_s16(int16x4_t a, int16_t b) {
  return __builtin_mpl_vector_qrdmulh_n_i16v4(a, (int16x4_t){b});
}

int16x8_t __builtin_mpl_vector_qrdmulhq_n_i16v8(int16x8_t a, int16x8_t b);
static inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__))
vqrdmulhq_n_s16(int16x8_t a, int16_t b) {
  return __builtin_mpl_vector_qrdmulhq_n_i16v8(a, (int16x8_t){b});
}

int32x2_t __builtin_mpl_vector_qrdmulh_n_i32v2(int32x2_t a, int32x2_t b);
static inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__))
vqrdmulh_n_s32(int32x2_t a, int32_t b) {
  return __builtin_mpl_vector_qrdmulh_n_i32v2(a, (int32x2_t){b});
}

int32x4_t __builtin_mpl_vector_qrdmulhq_n_i32v4(int32x4_t a, int32x4_t b);
static inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__))
vqrdmulhq_n_s32(int32x4_t a, int32_t b) {
  return __builtin_mpl_vector_qrdmulhq_n_i32v4(a, (int32x4_t){b});
}

int16x4_t __builtin_mpl_vector_qrdmulh_lane_i16v4(int16x4_t a, int16x4_t v, const int lane);
#define vqrdmulh_lane_s16(a, v, lane)  __builtin_mpl_vector_qrdmulh_lane_i16v4(a, v, lane)

int16x8_t __builtin_mpl_vector_qrdmulhq_lane_i16v8(int16x8_t a, int16x4_t v, const int lane);
#define vqrdmulhq_lane_s16(a, v, lane)  __builtin_mpl_vector_qrdmulhq_lane_i16v8(a, v, lane)

int32x2_t __builtin_mpl_vector_qrdmulh_lane_i32v2(int32x2_t a, int32x2_t v, const int lane);
#define vqrdmulh_lane_s32(a, v, lane)  __builtin_mpl_vector_qrdmulh_lane_i32v2(a, v, lane)

int32x4_t __builtin_mpl_vector_qrdmulhq_lane_i32v4(int32x4_t a, int32x2_t v, const int lane);
#define vqrdmulhq_lane_s32(a, v, lane)  __builtin_mpl_vector_qrdmulhq_lane_i32v4(a, v, lane)

static inline int16_t __attribute__ ((__always_inline__, __gnu_inline__))
vqrdmulhh_lane_s16(int16_t a, int16x4_t v, const int lane) {
  return vget_lane_s16(vqrdmulh_lane_s16((int16x4_t){a}, v, lane), 0);
}

static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqrdmulhs_lane_s32(int32_t a, int32x2_t v, const int lane) {
  return vget_lane_s32(vqrdmulh_lane_s32((int32x2_t){a}, v, lane), 0);
}

int16x4_t __builtin_mpl_vector_qrdmulh_laneq_i16v4(int16x4_t a, int16x8_t v, const int lane);
#define vqrdmulh_laneq_s16(a, v, lane)  __builtin_mpl_vector_qrdmulh_laneq_i16v4(a, v, lane)

int16x8_t __builtin_mpl_vector_qrdmulhq_laneq_i16v8(int16x8_t a, int16x8_t v, const int lane);
#define vqrdmulhq_laneq_s16(a, v, lane)  __builtin_mpl_vector_qrdmulhq_laneq_i16v8(a, v, lane)

int32x2_t __builtin_mpl_vector_qrdmulh_laneq_i32v2(int32x2_t a, int32x4_t v, const int lane);
#define vqrdmulh_laneq_s32(a, v, lane)  __builtin_mpl_vector_qrdmulh_laneq_i32v2(a, v, lane)

int32x4_t __builtin_mpl_vector_qrdmulhq_laneq_i32v4(int32x4_t a, int32x4_t v, const int lane);
#define vqrdmulhq_laneq_s32(a, v, lane)  __builtin_mpl_vector_qrdmulhq_laneq_i32v4(a, v, lane)

static inline int16_t __attribute__ ((__always_inline__, __gnu_inline__))
vqrdmulhh_laneq_s16(int16_t a, int16x8_t v, const int lane) {
  return vget_lane_s16(vqrdmulh_laneq_s16((int16x4_t){a}, v, lane), 0);
}

static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqrdmulhs_laneq_s32(int32_t a, int32x4_t v, const int lane) {
  return vget_lane_s32(vqrdmulh_laneq_s32((int32x2_t){a}, v, lane), 0);
}

int32x4_t __builtin_mpl_vector_qdmlal_n_i32v4(int32x4_t a, int16x4_t b, int16x4_t c);
static inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmlal_n_s16(int32x4_t a, int16x4_t b, int16_t c) {
  return __builtin_mpl_vector_qdmlal_n_i32v4(a, b, (int16x4_t){c});
}

int64x2_t __builtin_mpl_vector_qdmlal_n_i64v2(int64x2_t a, int32x2_t b, int32x2_t c);
static inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmlal_n_s32(int64x2_t a, int32x2_t b, int32_t c) {
  return __builtin_mpl_vector_qdmlal_n_i64v2(a, b, (int32x2_t){c});
}

int32x4_t __builtin_mpl_vector_qdmlal_high_n_i32v4(int32x4_t a, int16x8_t b, int16x8_t c);
static inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmlal_high_n_s16(int32x4_t a, int16x8_t b, int16_t c) {
  return __builtin_mpl_vector_qdmlal_high_n_i32v4(a, b, (int16x8_t){c});
}

int64x2_t __builtin_mpl_vector_qdmlal_high_n_i64v2(int64x2_t a, int32x4_t b, int32x4_t c);
static inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmlal_high_n_s32(int64x2_t a, int32x4_t b, int32_t c) {
  return __builtin_mpl_vector_qdmlal_high_n_i64v2(a, b, (int32x4_t){c});
}

int32x4_t __builtin_mpl_vector_qdmlsl_n_i32v4(int32x4_t a, int16x4_t b, int16x4_t c);
static inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmlsl_n_s16(int32x4_t a, int16x4_t b, int16_t c) {
  return __builtin_mpl_vector_qdmlsl_n_i32v4(a, b, (int16x4_t){c});
}

int64x2_t __builtin_mpl_vector_qdmlsl_n_i64v2(int64x2_t a, int32x2_t b, int32x2_t c);
static inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmlsl_n_s32(int64x2_t a, int32x2_t b, int32_t c) {
  return __builtin_mpl_vector_qdmlsl_n_i64v2(a, b, (int32x2_t){c});
}

int32x4_t __builtin_mpl_vector_qdmlsl_high_n_i32v4(int32x4_t a, int16x8_t b, int16x8_t c);
static inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmlsl_high_n_s16(int32x4_t a, int16x8_t b, int16_t c) {
  return __builtin_mpl_vector_qdmlsl_high_n_i32v4(a, b, (int16x8_t){c});
}

int64x2_t __builtin_mpl_vector_qdmlsl_high_n_i64v2(int64x2_t a, int32x4_t b, int32x4_t c);
static inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__))
vqdmlsl_high_n_s32(int64x2_t a, int32x4_t b, int32_t c) {
  return __builtin_mpl_vector_qdmlsl_high_n_i64v2(a, b, (int32x4_t){c});
}

int8x8_t __builtin_mpl_vector_hsub_i8v8(int8x8_t a, int8x8_t b);
#define vhsub_s8(a, b)  __builtin_mpl_vector_hsub_i8v8(a, b)

int8x16_t __builtin_mpl_vector_hsubq_i8v16(int8x16_t a, int8x16_t b);
#define vhsubq_s8(a, b)  __builtin_mpl_vector_hsubq_i8v16(a, b)

int16x4_t __builtin_mpl_vector_hsub_i16v4(int16x4_t a, int16x4_t b);
#define vhsub_s16(a, b)  __builtin_mpl_vector_hsub_i16v4(a, b)

int16x8_t __builtin_mpl_vector_hsubq_i16v8(int16x8_t a, int16x8_t b);
#define vhsubq_s16(a, b)  __builtin_mpl_vector_hsubq_i16v8(a, b)

int32x2_t __builtin_mpl_vector_hsub_i32v2(int32x2_t a, int32x2_t b);
#define vhsub_s32(a, b)  __builtin_mpl_vector_hsub_i32v2(a, b)

int32x4_t __builtin_mpl_vector_hsubq_i32v4(int32x4_t a, int32x4_t b);
#define vhsubq_s32(a, b)  __builtin_mpl_vector_hsubq_i32v4(a, b)

uint8x8_t __builtin_mpl_vector_hsub_u8v8(uint8x8_t a, uint8x8_t b);
#define vhsub_u8(a, b)  __builtin_mpl_vector_hsub_u8v8(a, b)

uint8x16_t __builtin_mpl_vector_hsubq_u8v16(uint8x16_t a, uint8x16_t b);
#define vhsubq_u8(a, b)  __builtin_mpl_vector_hsubq_u8v16(a, b)

uint16x4_t __builtin_mpl_vector_hsub_u16v4(uint16x4_t a, uint16x4_t b);
#define vhsub_u16(a, b)  __builtin_mpl_vector_hsub_u16v4(a, b)

uint16x8_t __builtin_mpl_vector_hsubq_u16v8(uint16x8_t a, uint16x8_t b);
#define vhsubq_u16(a, b)  __builtin_mpl_vector_hsubq_u16v8(a, b)

uint32x2_t __builtin_mpl_vector_hsub_u32v2(uint32x2_t a, uint32x2_t b);
#define vhsub_u32(a, b)  __builtin_mpl_vector_hsub_u32v2(a, b)

uint32x4_t __builtin_mpl_vector_hsubq_u32v4(uint32x4_t a, uint32x4_t b);
#define vhsubq_u32(a, b)  __builtin_mpl_vector_hsubq_u32v4(a, b)

int8x8_t __builtin_mpl_vector_subhn_i8v8(int16x8_t a, int16x8_t b);
#define vsubhn_s16(a, b)  __builtin_mpl_vector_subhn_i8v8(a, b)

int16x4_t __builtin_mpl_vector_subhn_i16v4(int32x4_t a, int32x4_t b);
#define vsubhn_s32(a, b)  __builtin_mpl_vector_subhn_i16v4(a, b)

int32x2_t __builtin_mpl_vector_subhn_i32v2(int64x2_t a, int64x2_t b);
#define vsubhn_s64(a, b)  __builtin_mpl_vector_subhn_i32v2(a, b)

uint8x8_t __builtin_mpl_vector_subhn_u8v8(uint16x8_t a, uint16x8_t b);
#define vsubhn_u16(a, b)  __builtin_mpl_vector_subhn_u8v8(a, b)

uint16x4_t __builtin_mpl_vector_subhn_u16v4(uint32x4_t a, uint32x4_t b);
#define vsubhn_u32(a, b)  __builtin_mpl_vector_subhn_u16v4(a, b)

uint32x2_t __builtin_mpl_vector_subhn_u32v2(uint64x2_t a, uint64x2_t b);
#define vsubhn_u64(a, b)  __builtin_mpl_vector_subhn_u32v2(a, b)

int8x16_t __builtin_mpl_vector_subhn_high_i8v16(int8x8_t r, int16x8_t a, int16x8_t b);
#define vsubhn_high_s16(r, a, b)  __builtin_mpl_vector_subhn_high_i8v16(r, a, b)

int16x8_t __builtin_mpl_vector_subhn_high_i16v8(int16x4_t r, int32x4_t a, int32x4_t b);
#define vsubhn_high_s32(r, a, b)  __builtin_mpl_vector_subhn_high_i16v8(r, a, b)

int32x4_t __builtin_mpl_vector_subhn_high_i32v4(int32x2_t r, int64x2_t a, int64x2_t b);
#define vsubhn_high_s64(r, a, b)  __builtin_mpl_vector_subhn_high_i32v4(r, a, b)

uint8x16_t __builtin_mpl_vector_subhn_high_u8v16(uint8x8_t r, uint16x8_t a, uint16x8_t b);
#define vsubhn_high_u16(r, a, b)  __builtin_mpl_vector_subhn_high_u8v16(r, a, b)

uint16x8_t __builtin_mpl_vector_subhn_high_u16v8(uint16x4_t r, uint32x4_t a, uint32x4_t b);
#define vsubhn_high_u32(r, a, b)  __builtin_mpl_vector_subhn_high_u16v8(r, a, b)

uint32x4_t __builtin_mpl_vector_subhn_high_u32v4(uint32x2_t r, uint64x2_t a, uint64x2_t b);
#define vsubhn_high_u64(r, a, b)  __builtin_mpl_vector_subhn_high_u32v4(r, a, b)

int8x8_t __builtin_mpl_vector_rsubhn_i8v8(int16x8_t a, int16x8_t b);
#define vrsubhn_s16(a, b)  __builtin_mpl_vector_rsubhn_i8v8(a, b)

int16x4_t __builtin_mpl_vector_rsubhn_i16v4(int32x4_t a, int32x4_t b);
#define vrsubhn_s32(a, b)  __builtin_mpl_vector_rsubhn_i16v4(a, b)

int32x2_t __builtin_mpl_vector_rsubhn_i32v2(int64x2_t a, int64x2_t b);
#define vrsubhn_s64(a, b)  __builtin_mpl_vector_rsubhn_i32v2(a, b)

uint8x8_t __builtin_mpl_vector_rsubhn_u8v8(uint16x8_t a, uint16x8_t b);
#define vrsubhn_u16(a, b)  __builtin_mpl_vector_rsubhn_u8v8(a, b)

uint16x4_t __builtin_mpl_vector_rsubhn_u16v4(uint32x4_t a, uint32x4_t b);
#define vrsubhn_u32(a, b)  __builtin_mpl_vector_rsubhn_u16v4(a, b)

uint32x2_t __builtin_mpl_vector_rsubhn_u32v2(uint64x2_t a, uint64x2_t b);
#define vrsubhn_u64(a, b)  __builtin_mpl_vector_rsubhn_u32v2(a, b)

int8x16_t __builtin_mpl_vector_rsubhn_high_i8v16(int8x8_t r, int16x8_t a, int16x8_t b);
#define vrsubhn_high_s16(r, a, b)  __builtin_mpl_vector_rsubhn_high_i8v16(r, a, b)

int16x8_t __builtin_mpl_vector_rsubhn_high_i16v8(int16x4_t r, int32x4_t a, int32x4_t b);
#define vrsubhn_high_s32(r, a, b)  __builtin_mpl_vector_rsubhn_high_i16v8(r, a, b)

int32x4_t __builtin_mpl_vector_rsubhn_high_i32v4(int32x2_t r, int64x2_t a, int64x2_t b);
#define vrsubhn_high_s64(r, a, b)  __builtin_mpl_vector_rsubhn_high_i32v4(r, a, b)

uint8x16_t __builtin_mpl_vector_rsubhn_high_u8v16(uint8x8_t r, uint16x8_t a, uint16x8_t b);
#define vrsubhn_high_u16(r, a, b)  __builtin_mpl_vector_rsubhn_high_u8v16(r, a, b)

uint16x8_t __builtin_mpl_vector_rsubhn_high_u16v8(uint16x4_t r, uint32x4_t a, uint32x4_t b);
#define vrsubhn_high_u32(r, a, b)  __builtin_mpl_vector_rsubhn_high_u16v8(r, a, b)

uint32x4_t __builtin_mpl_vector_rsubhn_high_u32v4(uint32x2_t r, uint64x2_t a, uint64x2_t b);
#define vrsubhn_high_u64(r, a, b)  __builtin_mpl_vector_rsubhn_high_u32v4(r, a, b)

int8x8_t __builtin_mpl_vector_qsub_i8v8(int8x8_t a, int8x8_t b);
#define vqsub_s8(a, b)  __builtin_mpl_vector_qsub_i8v8(a, b)

int8x16_t __builtin_mpl_vector_qsubq_i8v16(int8x16_t a, int8x16_t b);
#define vqsubq_s8(a, b)  __builtin_mpl_vector_qsubq_i8v16(a, b)

int16x4_t __builtin_mpl_vector_qsub_i16v4(int16x4_t a, int16x4_t b);
#define vqsub_s16(a, b)  __builtin_mpl_vector_qsub_i16v4(a, b)

int16x8_t __builtin_mpl_vector_qsubq_i16v8(int16x8_t a, int16x8_t b);
#define vqsubq_s16(a, b)  __builtin_mpl_vector_qsubq_i16v8(a, b)

int32x2_t __builtin_mpl_vector_qsub_i32v2(int32x2_t a, int32x2_t b);
#define vqsub_s32(a, b)  __builtin_mpl_vector_qsub_i32v2(a, b)

int32x4_t __builtin_mpl_vector_qsubq_i32v4(int32x4_t a, int32x4_t b);
#define vqsubq_s32(a, b)  __builtin_mpl_vector_qsubq_i32v4(a, b)

int64x1_t __builtin_mpl_vector_qsub_i64v1(int64x1_t a, int64x1_t b);
#define vqsub_s64(a, b)  __builtin_mpl_vector_qsub_i64v1(a, b)

int64x2_t __builtin_mpl_vector_qsubq_i64v2(int64x2_t a, int64x2_t b);
#define vqsubq_s64(a, b)  __builtin_mpl_vector_qsubq_i64v2(a, b)

uint8x8_t __builtin_mpl_vector_qsub_u8v8(uint8x8_t a, uint8x8_t b);
#define vqsub_u8(a, b)  __builtin_mpl_vector_qsub_u8v8(a, b)

uint8x16_t __builtin_mpl_vector_qsubq_u8v16(uint8x16_t a, uint8x16_t b);
#define vqsubq_u8(a, b)  __builtin_mpl_vector_qsubq_u8v16(a, b)

uint16x4_t __builtin_mpl_vector_qsub_u16v4(uint16x4_t a, uint16x4_t b);
#define vqsub_u16(a, b)  __builtin_mpl_vector_qsub_u16v4(a, b)

uint16x8_t __builtin_mpl_vector_qsubq_u16v8(uint16x8_t a, uint16x8_t b);
#define vqsubq_u16(a, b)  __builtin_mpl_vector_qsubq_u16v8(a, b)

uint32x2_t __builtin_mpl_vector_qsub_u32v2(uint32x2_t a, uint32x2_t b);
#define vqsub_u32(a, b)  __builtin_mpl_vector_qsub_u32v2(a, b)

uint32x4_t __builtin_mpl_vector_qsubq_u32v4(uint32x4_t a, uint32x4_t b);
#define vqsubq_u32(a, b)  __builtin_mpl_vector_qsubq_u32v4(a, b)

uint64x1_t __builtin_mpl_vector_qsub_u64v1(uint64x1_t a, uint64x1_t b);
#define vqsub_u64(a, b)  __builtin_mpl_vector_qsub_u64v1(a, b)

uint64x2_t __builtin_mpl_vector_qsubq_u64v2(uint64x2_t a, uint64x2_t b);
#define vqsubq_u64(a, b)  __builtin_mpl_vector_qsubq_u64v2(a, b)

static inline int8_t __attribute__ ((__always_inline__, __gnu_inline__))
vqsubb_s8(int8_t a, int8_t b) {
  return vget_lane_s8(vqsub_s8((int8x8_t){a}, (int8x8_t){b}), 0);
}

static inline int16_t __attribute__ ((__always_inline__, __gnu_inline__))
vqsubh_s16(int16_t a, int16_t b) {
  return vget_lane_s16(vqsub_s16((int16x4_t){a}, (int16x4_t){b}), 0);
}

static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqsubs_s32(int32_t a, int32_t b) {
  return vget_lane_s32(vqsub_s32((int32x2_t){a}, (int32x2_t){b}), 0);
}

static inline int64_t __attribute__ ((__always_inline__, __gnu_inline__))
vqsubd_s64(int64_t a, int64_t b) {
  return vget_lane_s64(vqsub_s64((int64x1_t){a}, (int64x1_t){b}), 0);
}

static inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__))
vqsubb_u8(uint8_t a, uint8_t b) {
  return vget_lane_u8(vqsub_u8((uint8x8_t){a}, (uint8x8_t){b}), 0);
}

static inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__))
vqsubh_u16(uint16_t a, uint16_t b) {
  return vget_lane_u16(vqsub_u16((uint16x4_t){a}, (uint16x4_t){b}), 0);
}

static inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqsubs_u32(uint32_t a, uint32_t b) {
  return vget_lane_u32(vqsub_u32((uint32x2_t){a}, (uint32x2_t){b}), 0);
}

static inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__))
vqsubd_u64(uint64_t a, uint64_t b) {
  return vget_lane_u64(vqsub_u64((uint64x1_t){a}, (uint64x1_t){b}), 0);
}

int8x8_t __builtin_mpl_vector_aba_i8v8(int8x8_t a, int8x8_t b, int8x8_t c);
#define vaba_s8(a, b, c)  __builtin_mpl_vector_aba_i8v8(a, b, c)

int8x16_t __builtin_mpl_vector_abaq_i8v16(int8x16_t a, int8x16_t b, int8x16_t c);
#define vabaq_s8(a, b, c)  __builtin_mpl_vector_abaq_i8v16(a, b, c)

int16x4_t __builtin_mpl_vector_aba_i16v4(int16x4_t a, int16x4_t b, int16x4_t c);
#define vaba_s16(a, b, c)  __builtin_mpl_vector_aba_i16v4(a, b, c)

int16x8_t __builtin_mpl_vector_abaq_i16v8(int16x8_t a, int16x8_t b, int16x8_t c);
#define vabaq_s16(a, b, c)  __builtin_mpl_vector_abaq_i16v8(a, b, c)

int32x2_t __builtin_mpl_vector_aba_i32v2(int32x2_t a, int32x2_t b, int32x2_t c);
#define vaba_s32(a, b, c)  __builtin_mpl_vector_aba_i32v2(a, b, c)

int32x4_t __builtin_mpl_vector_abaq_i32v4(int32x4_t a, int32x4_t b, int32x4_t c);
#define vabaq_s32(a, b, c)  __builtin_mpl_vector_abaq_i32v4(a, b, c)

uint8x8_t __builtin_mpl_vector_aba_u8v8(uint8x8_t a, uint8x8_t b, uint8x8_t c);
#define vaba_u8(a, b, c)  __builtin_mpl_vector_aba_u8v8(a, b, c)

uint8x16_t __builtin_mpl_vector_abaq_u8v16(uint8x16_t a, uint8x16_t b, uint8x16_t c);
#define vabaq_u8(a, b, c)  __builtin_mpl_vector_abaq_u8v16(a, b, c)

uint16x4_t __builtin_mpl_vector_aba_u16v4(uint16x4_t a, uint16x4_t b, uint16x4_t c);
#define vaba_u16(a, b, c)  __builtin_mpl_vector_aba_u16v4(a, b, c)

uint16x8_t __builtin_mpl_vector_abaq_u16v8(uint16x8_t a, uint16x8_t b, uint16x8_t c);
#define vabaq_u16(a, b, c)  __builtin_mpl_vector_abaq_u16v8(a, b, c)

uint32x2_t __builtin_mpl_vector_aba_u32v2(uint32x2_t a, uint32x2_t b, uint32x2_t c);
#define vaba_u32(a, b, c)  __builtin_mpl_vector_aba_u32v2(a, b, c)

uint32x4_t __builtin_mpl_vector_abaq_u32v4(uint32x4_t a, uint32x4_t b, uint32x4_t c);
#define vabaq_u32(a, b, c)  __builtin_mpl_vector_abaq_u32v4(a, b, c)

int16x8_t __builtin_mpl_vector_abal_i16v8(int16x8_t a, int8x8_t b, int8x8_t c);
#define vabal_s8(a, b, c)  __builtin_mpl_vector_abal_i16v8(a, b, c)

int32x4_t __builtin_mpl_vector_abal_i32v4(int32x4_t a, int16x4_t b, int16x4_t c);
#define vabal_s16(a, b, c)  __builtin_mpl_vector_abal_i32v4(a, b, c)

int64x2_t __builtin_mpl_vector_abal_i64v2(int64x2_t a, int32x2_t b, int32x2_t c);
#define vabal_s32(a, b, c)  __builtin_mpl_vector_abal_i64v2(a, b, c)

uint16x8_t __builtin_mpl_vector_abal_u16v8(uint16x8_t a, uint8x8_t b, uint8x8_t c);
#define vabal_u8(a, b, c)  __builtin_mpl_vector_abal_u16v8(a, b, c)

uint32x4_t __builtin_mpl_vector_abal_u32v4(uint32x4_t a, uint16x4_t b, uint16x4_t c);
#define vabal_u16(a, b, c)  __builtin_mpl_vector_abal_u32v4(a, b, c)

uint64x2_t __builtin_mpl_vector_abal_u64v2(uint64x2_t a, uint32x2_t b, uint32x2_t c);
#define vabal_u32(a, b, c)  __builtin_mpl_vector_abal_u64v2(a, b, c)

int16x8_t __builtin_mpl_vector_abal_high_i16v8(int16x8_t a, int8x16_t b, int8x16_t c);
#define vabal_high_s8(a, b, c)  __builtin_mpl_vector_abal_high_i16v8(a, b, c)

int32x4_t __builtin_mpl_vector_abal_high_i32v4(int32x4_t a, int16x8_t b, int16x8_t c);
#define vabal_high_s16(a, b, c)  __builtin_mpl_vector_abal_high_i32v4(a, b, c)

int64x2_t __builtin_mpl_vector_abal_high_i64v2(int64x2_t a, int32x4_t b, int32x4_t c);
#define vabal_high_s32(a, b, c)  __builtin_mpl_vector_abal_high_i64v2(a, b, c)

uint16x8_t __builtin_mpl_vector_abal_high_u16v8(uint16x8_t a, uint8x16_t b, uint8x16_t c);
#define vabal_high_u8(a, b, c)  __builtin_mpl_vector_abal_high_u16v8(a, b, c)

uint32x4_t __builtin_mpl_vector_abal_high_u32v4(uint32x4_t a, uint16x8_t b, uint16x8_t c);
#define vabal_high_u16(a, b, c)  __builtin_mpl_vector_abal_high_u32v4(a, b, c)

uint64x2_t __builtin_mpl_vector_abal_high_u64v2(uint64x2_t a, uint32x4_t b, uint32x4_t c);
#define vabal_high_u32(a, b, c)  __builtin_mpl_vector_abal_high_u64v2(a, b, c)

int8x8_t __builtin_mpl_vector_qabs_i8v8(int8x8_t a);
#define vqabs_s8(a)  __builtin_mpl_vector_qabs_i8v8(a)

int8x16_t __builtin_mpl_vector_qabsq_i8v16(int8x16_t a);
#define vqabsq_s8(a)  __builtin_mpl_vector_qabsq_i8v16(a)

int16x4_t __builtin_mpl_vector_qabs_i16v4(int16x4_t a);
#define vqabs_s16(a)  __builtin_mpl_vector_qabs_i16v4(a)

int16x8_t __builtin_mpl_vector_qabsq_i16v8(int16x8_t a);
#define vqabsq_s16(a)  __builtin_mpl_vector_qabsq_i16v8(a)

int32x2_t __builtin_mpl_vector_qabs_i32v2(int32x2_t a);
#define vqabs_s32(a)  __builtin_mpl_vector_qabs_i32v2(a)

int32x4_t __builtin_mpl_vector_qabsq_i32v4(int32x4_t a);
#define vqabsq_s32(a)  __builtin_mpl_vector_qabsq_i32v4(a)

int64x1_t __builtin_mpl_vector_qabs_i64v1(int64x1_t a);
#define vqabs_s64(a)  __builtin_mpl_vector_qabs_i64v1(a)

int64x2_t __builtin_mpl_vector_qabsq_i64v2(int64x2_t a);
#define vqabsq_s64(a)  __builtin_mpl_vector_qabsq_i64v2(a)

static inline int8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqabsb_s8(int8_t a) {
  return vget_lane_s8(vqabs_s8((int8x8_t){a}), 0);
}

static inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqabsh_s16(int16_t a) {
  return vget_lane_s16(vqabs_s16((int16x4_t){a}), 0);
}

static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqabss_s32(int32_t a) {
  return vget_lane_s32(vqabs_s32((int32x2_t){a}), 0);
}

static inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vqabsd_s64(int64_t a) {
  return vget_lane_s64(vqabs_s64((int64x1_t){a}), 0);
}

uint32x2_t __builtin_mpl_vector_rsqrte_u32v2(uint32x2_t a);
#define vrsqrte_u32(a)  __builtin_mpl_vector_rsqrte_u32v2(a)

uint32x4_t __builtin_mpl_vector_rsqrteq_u32v4(uint32x4_t a);
#define vrsqrteq_u32(a)  __builtin_mpl_vector_rsqrteq_u32v4(a)

int16x4_t __builtin_mpl_vector_addlv_i8v8(int8x8_t a);
static inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddlv_s8(int8x8_t a) {
  return vget_lane_s16(__builtin_mpl_vector_addlv_i8v8(a), 0);
}

int16x4_t __builtin_mpl_vector_addlvq_i8v16(int8x16_t a);
static inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddlvq_s8(int8x16_t a) {
  return vget_lane_s16(__builtin_mpl_vector_addlvq_i8v16(a), 0);
}

int32x2_t __builtin_mpl_vector_addlv_i16v4(int16x4_t a);
static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddlv_s16(int16x4_t a) {
  return vget_lane_s32(__builtin_mpl_vector_addlv_i16v4(a), 0);
}

int32x2_t __builtin_mpl_vector_addlvq_i16v8(int16x8_t a);
static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddlvq_s16(int16x8_t a) {
  return vget_lane_s32(__builtin_mpl_vector_addlvq_i16v8(a), 0);
}

static inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddlv_s32(int32x2_t a) {
  return vget_lane_s64(vpaddl_s32(a), 0);
}

int64x1_t __builtin_mpl_vector_addlvq_i32v4(int32x4_t a);
static inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddlvq_s32(int32x4_t a) {
  return vget_lane_s64(__builtin_mpl_vector_addlvq_i32v4(a), 0);
}

uint16x4_t __builtin_mpl_vector_addlv_u8v8(uint8x8_t a);
static inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddlv_u8(uint8x8_t a) {
  return vget_lane_u16(__builtin_mpl_vector_addlv_u8v8(a), 0);
}

uint16x4_t __builtin_mpl_vector_addlvq_u8v16(uint8x16_t a);
static inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddlvq_u8(uint8x16_t a) {
  return vget_lane_u16(__builtin_mpl_vector_addlvq_u8v16(a), 0);
}

uint32x2_t __builtin_mpl_vector_addlv_u16v4(uint16x4_t a);
static inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddlv_u16(uint16x4_t a) {
  return vget_lane_u32(__builtin_mpl_vector_addlv_u16v4(a), 0);
}

uint32x2_t __builtin_mpl_vector_addlvq_u16v8(uint16x8_t a);
static inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddlvq_u16(uint16x8_t a) {
  return vget_lane_u32(__builtin_mpl_vector_addlvq_u16v8(a), 0);
}

static inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddlv_u32(uint32x2_t a) {
  return vget_lane_u64(vpaddl_u32(a), 0);
}

uint64x1_t __builtin_mpl_vector_addlvq_u32v4(uint32x4_t a);
static inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__)) vaddlvq_u32(uint32x4_t a) {
  return vget_lane_u64(__builtin_mpl_vector_addlvq_u32v4(a), 0);
}

int8x8_t __builtin_mpl_vector_qshl_i8v8(int8x8_t a, int8x8_t b);
#define vqshl_s8(a, b)  __builtin_mpl_vector_qshl_i8v8(a, b)

int8x16_t __builtin_mpl_vector_qshlq_i8v16(int8x16_t a, int8x16_t b);
#define vqshlq_s8(a, b)  __builtin_mpl_vector_qshlq_i8v16(a, b)

int16x4_t __builtin_mpl_vector_qshl_i16v4(int16x4_t a, int16x4_t b);
#define vqshl_s16(a, b)  __builtin_mpl_vector_qshl_i16v4(a, b)

int16x8_t __builtin_mpl_vector_qshlq_i16v8(int16x8_t a, int16x8_t b);
#define vqshlq_s16(a, b)  __builtin_mpl_vector_qshlq_i16v8(a, b)

int32x2_t __builtin_mpl_vector_qshl_i32v2(int32x2_t a, int32x2_t b);
#define vqshl_s32(a, b)  __builtin_mpl_vector_qshl_i32v2(a, b)

int32x4_t __builtin_mpl_vector_qshlq_i32v4(int32x4_t a, int32x4_t b);
#define vqshlq_s32(a, b)  __builtin_mpl_vector_qshlq_i32v4(a, b)

int64x1_t __builtin_mpl_vector_qshl_i64v1(int64x1_t a, int64x1_t b);
#define vqshl_s64(a, b)  __builtin_mpl_vector_qshl_i64v1(a, b)

int64x2_t __builtin_mpl_vector_qshlq_i64v2(int64x2_t a, int64x2_t b);
#define vqshlq_s64(a, b)  __builtin_mpl_vector_qshlq_i64v2(a, b)

uint8x8_t __builtin_mpl_vector_qshl_u8v8(uint8x8_t a, int8x8_t b);
#define vqshl_u8(a, b)  __builtin_mpl_vector_qshl_u8v8(a, b)

uint8x16_t __builtin_mpl_vector_qshlq_u8v16(uint8x16_t a, int8x16_t b);
#define vqshlq_u8(a, b)  __builtin_mpl_vector_qshlq_u8v16(a, b)

uint16x4_t __builtin_mpl_vector_qshl_u16v4(uint16x4_t a, int16x4_t b);
#define vqshl_u16(a, b)  __builtin_mpl_vector_qshl_u16v4(a, b)

uint16x8_t __builtin_mpl_vector_qshlq_u16v8(uint16x8_t a, int16x8_t b);
#define vqshlq_u16(a, b)  __builtin_mpl_vector_qshlq_u16v8(a, b)

uint32x2_t __builtin_mpl_vector_qshl_u32v2(uint32x2_t a, int32x2_t b);
#define vqshl_u32(a, b)  __builtin_mpl_vector_qshl_u32v2(a, b)

uint32x4_t __builtin_mpl_vector_qshlq_u32v4(uint32x4_t a, int32x4_t b);
#define vqshlq_u32(a, b)  __builtin_mpl_vector_qshlq_u32v4(a, b)

uint64x1_t __builtin_mpl_vector_qshl_u64v1(uint64x1_t a, int64x1_t b);
#define vqshl_u64(a, b)  __builtin_mpl_vector_qshl_u64v1(a, b)

uint64x2_t __builtin_mpl_vector_qshlq_u64v2(uint64x2_t a, int64x2_t b);
#define vqshlq_u64(a, b)  __builtin_mpl_vector_qshlq_u64v2(a, b)

static inline int8_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshlb_s8(int8_t a, int8_t b) {
  return vget_lane_s8(vqshl_s8((int8x8_t){a}, (int8x8_t){b}), 0);
}

static inline int16_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshlh_s16(int16_t a, int16_t b) {
  return vget_lane_s16(vqshl_s16((int16x4_t){a}, (int16x4_t){b}), 0);
}

static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshls_s32(int32_t a, int32_t b) {
  return vget_lane_s32(vqshl_s32((int32x2_t){a}, (int32x2_t){b}), 0);
}

static inline int64_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshld_s64(int64_t a, int64_t b) {
  return vget_lane_s64(vqshl_s64((int64x1_t){a}, (int64x1_t){b}), 0);
}

static inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshlb_u8(uint8_t a, uint8_t b) {
  return vget_lane_u8(vqshl_u8((uint8x8_t){a}, (uint8x8_t){b}), 0);
}

static inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshlh_u16(uint16_t a, uint16_t b) {
  return vget_lane_u16(vqshl_u16((uint16x4_t){a}, (uint16x4_t){b}), 0);
}

static inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshls_u32(uint32_t a, uint32_t b) {
  return vget_lane_u32(vqshl_u32((uint32x2_t){a}, (uint32x2_t){b}), 0);
}

static inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshld_u64(uint64_t a, uint64_t b) {
  return vget_lane_u64(vqshl_u64((uint64x1_t){a}, (uint64x1_t){b}), 0);
}

int8x8_t __builtin_mpl_vector_qshl_n_i8v8(int8x8_t a, const int n);
#define vqshl_n_s8(a, n)  __builtin_mpl_vector_qshl_n_i8v8(a, n)

int8x16_t __builtin_mpl_vector_qshlq_n_i8v16(int8x16_t a, const int n);
#define vqshlq_n_s8(a, n)  __builtin_mpl_vector_qshlq_n_i8v16(a, n)

int16x4_t __builtin_mpl_vector_qshl_n_i16v4(int16x4_t a, const int n);
#define vqshl_n_s16(a, n)  __builtin_mpl_vector_qshl_n_i16v4(a, n)

int16x8_t __builtin_mpl_vector_qshlq_n_i16v8(int16x8_t a, const int n);
#define vqshlq_n_s16(a, n)  __builtin_mpl_vector_qshlq_n_i16v8(a, n)

int32x2_t __builtin_mpl_vector_qshl_n_i32v2(int32x2_t a, const int n);
#define vqshl_n_s32(a, n)  __builtin_mpl_vector_qshl_n_i32v2(a, n)

int32x4_t __builtin_mpl_vector_qshlq_n_i32v4(int32x4_t a, const int n);
#define vqshlq_n_s32(a, n)  __builtin_mpl_vector_qshlq_n_i32v4(a, n)

int64x1_t __builtin_mpl_vector_qshl_n_i64v1(int64x1_t a, const int n);
#define vqshl_n_s64(a, n)  __builtin_mpl_vector_qshl_n_i64v1(a, n)

int64x2_t __builtin_mpl_vector_qshlq_n_i64v2(int64x2_t a, const int n);
#define vqshlq_n_s64(a, n)  __builtin_mpl_vector_qshlq_n_i64v2(a, n)

uint8x8_t __builtin_mpl_vector_qshl_n_u8v8(uint8x8_t a, const int n);
#define vqshl_n_u8(a, n)  __builtin_mpl_vector_qshl_n_u8v8(a, n)

uint8x16_t __builtin_mpl_vector_qshlq_n_u8v16(uint8x16_t a, const int n);
#define vqshlq_n_u8(a, n)  __builtin_mpl_vector_qshlq_n_u8v16(a, n)

uint16x4_t __builtin_mpl_vector_qshl_n_u16v4(uint16x4_t a, const int n);
#define vqshl_n_u16(a, n)  __builtin_mpl_vector_qshl_n_u16v4(a, n)

uint16x8_t __builtin_mpl_vector_qshlq_n_u16v8(uint16x8_t a, const int n);
#define vqshlq_n_u16(a, n)  __builtin_mpl_vector_qshlq_n_u16v8(a, n)

uint32x2_t __builtin_mpl_vector_qshl_n_u32v2(uint32x2_t a, const int n);
#define vqshl_n_u32(a, n)  __builtin_mpl_vector_qshl_n_u32v2(a, n)

uint32x4_t __builtin_mpl_vector_qshlq_n_u32v4(uint32x4_t a, const int n);
#define vqshlq_n_u32(a, n)  __builtin_mpl_vector_qshlq_n_u32v4(a, n)

uint64x1_t __builtin_mpl_vector_qshl_n_u64v1(uint64x1_t a, const int n);
#define vqshl_n_u64(a, n)  __builtin_mpl_vector_qshl_n_u64v1(a, n)

uint64x2_t __builtin_mpl_vector_qshlq_n_u64v2(uint64x2_t a, const int n);
#define vqshlq_n_u64(a, n)  __builtin_mpl_vector_qshlq_n_u64v2(a, n)

static inline int8_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshlb_n_s8(int8_t a, const int n) {
  return vget_lane_s64(vqshl_n_s8((int8x8_t){a}, n), 0);
}

static inline int16_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshlh_n_s16(int16_t a, const int n) {
  return vget_lane_s64(vqshl_n_s16((int16x4_t){a}, n), 0);
}

static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshls_n_s32(int32_t a, const int n) {
  return vget_lane_s64(vqshl_n_s32((int32x2_t){a}, n), 0);
}

static inline int64_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshld_n_s64(int64_t a, const int n) {
  return vget_lane_s64(vqshl_n_s64((int64x1_t){a}, n), 0);
}

static inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshlb_n_u8(uint8_t a, const int n) {
  return vget_lane_u64(vqshl_n_u8((uint8x8_t){a}, n), 0);
}

static inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshlh_n_u16(uint16_t a, const int n) {
  return vget_lane_u64(vqshl_n_u16((uint16x4_t){a}, n), 0);
}

static inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshls_n_u32(uint32_t a, const int n) {
  return vget_lane_u64(vqshl_n_u32((uint32x2_t){a}, n), 0);
}

static inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshld_n_u64(uint64_t a, const int n) {
  return vget_lane_u64(vqshl_n_u64((uint64x1_t){a}, n), 0);
}

uint8x8_t __builtin_mpl_vector_qshlu_n_u8v8(int8x8_t a, const int n);
#define vqshlu_n_s8(a, n)  __builtin_mpl_vector_qshlu_n_u8v8(a, n)

uint8x16_t __builtin_mpl_vector_qshluq_n_u8v16(int8x16_t a, const int n);
#define vqshluq_n_s8(a, n)  __builtin_mpl_vector_qshluq_n_u8v16(a, n)

uint16x4_t __builtin_mpl_vector_qshlu_n_u16v4(int16x4_t a, const int n);
#define vqshlu_n_s16(a, n)  __builtin_mpl_vector_qshlu_n_u16v4(a, n)

uint16x8_t __builtin_mpl_vector_qshluq_n_u16v8(int16x8_t a, const int n);
#define vqshluq_n_s16(a, n)  __builtin_mpl_vector_qshluq_n_u16v8(a, n)

uint32x2_t __builtin_mpl_vector_qshlu_n_u32v2(int32x2_t a, const int n);
#define vqshlu_n_s32(a, n)  __builtin_mpl_vector_qshlu_n_u32v2(a, n)

uint32x4_t __builtin_mpl_vector_qshluq_n_u32v4(int32x4_t a, const int n);
#define vqshluq_n_s32(a, n)  __builtin_mpl_vector_qshluq_n_u32v4(a, n)

uint64x1_t __builtin_mpl_vector_qshlu_n_u64v1(int64x1_t a, const int n);
#define vqshlu_n_s64(a, n)  __builtin_mpl_vector_qshlu_n_u64v1(a, n)

uint64x2_t __builtin_mpl_vector_qshluq_n_u64v2(int64x2_t a, const int n);
#define vqshluq_n_s64(a, n)  __builtin_mpl_vector_qshluq_n_u64v2(a, n)

static inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshlub_n_s8(int8_t a, const int n) {
  return vget_lane_u64(vqshlu_n_s8((int8x8_t){a}, n), 0);
}

static inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshluh_n_s16(int16_t a, const int n) {
  return vget_lane_u64(vqshlu_n_s16((int16x4_t){a}, n), 0);
}

static inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshlus_n_s32(int32_t a, const int n) {
  return vget_lane_u64(vqshlu_n_s32((int32x2_t){a}, n), 0);
}

static inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshlud_n_s64(int64_t a, const int n) {
  return vget_lane_u64(vqshlu_n_s64((int64x1_t){a}, n), 0);
}

int8x8_t __builtin_mpl_vector_rshl_i8v8(int8x8_t a, int8x8_t b);
#define vrshl_s8(a, b)  __builtin_mpl_vector_rshl_i8v8(a, b)

int8x16_t __builtin_mpl_vector_rshlq_i8v16(int8x16_t a, int8x16_t b);
#define vrshlq_s8(a, b)  __builtin_mpl_vector_rshlq_i8v16(a, b)

int16x4_t __builtin_mpl_vector_rshl_i16v4(int16x4_t a, int16x4_t b);
#define vrshl_s16(a, b)  __builtin_mpl_vector_rshl_i16v4(a, b)

int16x8_t __builtin_mpl_vector_rshlq_i16v8(int16x8_t a, int16x8_t b);
#define vrshlq_s16(a, b)  __builtin_mpl_vector_rshlq_i16v8(a, b)

int32x2_t __builtin_mpl_vector_rshl_i32v2(int32x2_t a, int32x2_t b);
#define vrshl_s32(a, b)  __builtin_mpl_vector_rshl_i32v2(a, b)

int32x4_t __builtin_mpl_vector_rshlq_i32v4(int32x4_t a, int32x4_t b);
#define vrshlq_s32(a, b)  __builtin_mpl_vector_rshlq_i32v4(a, b)

int64x1_t __builtin_mpl_vector_rshl_i64v1(int64x1_t a, int64x1_t b);
#define vrshl_s64(a, b)  __builtin_mpl_vector_rshl_i64v1(a, b)

int64x2_t __builtin_mpl_vector_rshlq_i64v2(int64x2_t a, int64x2_t b);
#define vrshlq_s64(a, b)  __builtin_mpl_vector_rshlq_i64v2(a, b)

uint8x8_t __builtin_mpl_vector_rshl_u8v8(uint8x8_t a, int8x8_t b);
#define vrshl_u8(a, b)  __builtin_mpl_vector_rshl_u8v8(a, b)

uint8x16_t __builtin_mpl_vector_rshlq_u8v16(uint8x16_t a, int8x16_t b);
#define vrshlq_u8(a, b)  __builtin_mpl_vector_rshlq_u8v16(a, b)

uint16x4_t __builtin_mpl_vector_rshl_u16v4(uint16x4_t a, int16x4_t b);
#define vrshl_u16(a, b)  __builtin_mpl_vector_rshl_u16v4(a, b)

uint16x8_t __builtin_mpl_vector_rshlq_u16v8(uint16x8_t a, int16x8_t b);
#define vrshlq_u16(a, b)  __builtin_mpl_vector_rshlq_u16v8(a, b)

uint32x2_t __builtin_mpl_vector_rshl_u32v2(uint32x2_t a, int32x2_t b);
#define vrshl_u32(a, b)  __builtin_mpl_vector_rshl_u32v2(a, b)

uint32x4_t __builtin_mpl_vector_rshlq_u32v4(uint32x4_t a, int32x4_t b);
#define vrshlq_u32(a, b)  __builtin_mpl_vector_rshlq_u32v4(a, b)

uint64x1_t __builtin_mpl_vector_rshl_u64v1(uint64x1_t a, int64x1_t b);
#define vrshl_u64(a, b)  __builtin_mpl_vector_rshl_u64v1(a, b)

uint64x2_t __builtin_mpl_vector_rshlq_u64v2(uint64x2_t a, int64x2_t b);
#define vrshlq_u64(a, b)  __builtin_mpl_vector_rshlq_u64v2(a, b)

static inline int64_t __attribute__ ((__always_inline__, __gnu_inline__))
vrshld_s64(int64_t a, int64_t b) {
  return vget_lane_s64(vrshl_s64((int64x1_t){a}, (int64x1_t){b}), 0);
}

static inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__))
vrshld_u64(uint64_t a, uint64_t b) {
  return vget_lane_u64(vrshl_u64((uint64x1_t){a}, (uint64x1_t){b}), 0);
}

int8x8_t __builtin_mpl_vector_qrshl_i8v8(int8x8_t a, int8x8_t b);
#define vqrshl_s8(a, b)  __builtin_mpl_vector_qrshl_i8v8(a, b)

int8x16_t __builtin_mpl_vector_qrshlq_i8v16(int8x16_t a, int8x16_t b);
#define vqrshlq_s8(a, b)  __builtin_mpl_vector_qrshlq_i8v16(a, b)

int16x4_t __builtin_mpl_vector_qrshl_i16v4(int16x4_t a, int16x4_t b);
#define vqrshl_s16(a, b)  __builtin_mpl_vector_qrshl_i16v4(a, b)

int16x8_t __builtin_mpl_vector_qrshlq_i16v8(int16x8_t a, int16x8_t b);
#define vqrshlq_s16(a, b)  __builtin_mpl_vector_qrshlq_i16v8(a, b)

int32x2_t __builtin_mpl_vector_qrshl_i32v2(int32x2_t a, int32x2_t b);
#define vqrshl_s32(a, b)  __builtin_mpl_vector_qrshl_i32v2(a, b)

int32x4_t __builtin_mpl_vector_qrshlq_i32v4(int32x4_t a, int32x4_t b);
#define vqrshlq_s32(a, b)  __builtin_mpl_vector_qrshlq_i32v4(a, b)

int64x1_t __builtin_mpl_vector_qrshl_i64v1(int64x1_t a, int64x1_t b);
#define vqrshl_s64(a, b)  __builtin_mpl_vector_qrshl_i64v1(a, b)

int64x2_t __builtin_mpl_vector_qrshlq_i64v2(int64x2_t a, int64x2_t b);
#define vqrshlq_s64(a, b)  __builtin_mpl_vector_qrshlq_i64v2(a, b)

uint8x8_t __builtin_mpl_vector_qrshl_u8v8(uint8x8_t a, int8x8_t b);
#define vqrshl_u8(a, b)  __builtin_mpl_vector_qrshl_u8v8(a, b)

uint8x16_t __builtin_mpl_vector_qrshlq_u8v16(uint8x16_t a, int8x16_t b);
#define vqrshlq_u8(a, b)  __builtin_mpl_vector_qrshlq_u8v16(a, b)

uint16x4_t __builtin_mpl_vector_qrshl_u16v4(uint16x4_t a, int16x4_t b);
#define vqrshl_u16(a, b)  __builtin_mpl_vector_qrshl_u16v4(a, b)

uint16x8_t __builtin_mpl_vector_qrshlq_u16v8(uint16x8_t a, int16x8_t b);
#define vqrshlq_u16(a, b)  __builtin_mpl_vector_qrshlq_u16v8(a, b)

uint32x2_t __builtin_mpl_vector_qrshl_u32v2(uint32x2_t a, int32x2_t b);
#define vqrshl_u32(a, b)  __builtin_mpl_vector_qrshl_u32v2(a, b)

uint32x4_t __builtin_mpl_vector_qrshlq_u32v4(uint32x4_t a, int32x4_t b);
#define vqrshlq_u32(a, b)  __builtin_mpl_vector_qrshlq_u32v4(a, b)

uint64x1_t __builtin_mpl_vector_qrshl_u64v1(uint64x1_t a, int64x1_t b);
#define vqrshl_u64(a, b)  __builtin_mpl_vector_qrshl_u64v1(a, b)

uint64x2_t __builtin_mpl_vector_qrshlq_u64v2(uint64x2_t a, int64x2_t b);
#define vqrshlq_u64(a, b)  __builtin_mpl_vector_qrshlq_u64v2(a, b)

static inline int8_t __attribute__ ((__always_inline__, __gnu_inline__))
vqrshlb_s8(int8_t a, int8_t b) {
  return vget_lane_s8(vqrshl_s8((int8x8_t){a}, (int8x8_t){b}), 0);
}

static inline int16_t __attribute__ ((__always_inline__, __gnu_inline__))
vqrshlh_s16(int16_t a, int16_t b) {
  return vget_lane_s16(vqrshl_s16((int16x4_t){a}, (int16x4_t){b}), 0);
}

static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqrshls_s32(int32_t a, int32_t b) {
  return vget_lane_s32(vqrshl_s32((int32x2_t){a}, (int32x2_t){b}), 0);
}

static inline int64_t __attribute__ ((__always_inline__, __gnu_inline__))
vqrshld_s64(int64_t a, int64_t b) {
  return vget_lane_s64(vqrshl_s64((int64x1_t){a}, (int64x1_t){b}), 0);
}

static inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__))
vqrshlb_u8(uint8_t a, int8_t b) {
  return vget_lane_u8(vqrshl_u8((uint8x8_t){a}, (int8x8_t){b}), 0);
}

static inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__))
vqrshlh_u16(uint16_t a, int16_t b) {
  return vget_lane_u16(vqrshl_u16((uint16x4_t){a}, (int16x4_t){b}), 0);
}

static inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqrshls_u32(uint32_t a, int32_t b) {
  return vget_lane_u32(vqrshl_u32((uint32x2_t){a}, (int32x2_t){b}), 0);
}

static inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__))
vqrshld_u64(uint64_t a, int64_t b) {
  return vget_lane_u64(vqrshl_u64((uint64x1_t){a}, (int64x1_t){b}), 0);
}

int16x8_t __builtin_mpl_vector_shll_n_i16v8(int8x8_t a, const int n);
#define vshll_n_s8(a, n)  __builtin_mpl_vector_shll_n_i16v8(a, n)

int32x4_t __builtin_mpl_vector_shll_n_i32v4(int16x4_t a, const int n);
#define vshll_n_s16(a, n)  __builtin_mpl_vector_shll_n_i32v4(a, n)

int64x2_t __builtin_mpl_vector_shll_n_i64v2(int32x2_t a, const int n);
#define vshll_n_s32(a, n)  __builtin_mpl_vector_shll_n_i64v2(a, n)

uint16x8_t __builtin_mpl_vector_shll_n_u16v8(uint8x8_t a, const int n);
#define vshll_n_u8(a, n)  __builtin_mpl_vector_shll_n_u16v8(a, n)

uint32x4_t __builtin_mpl_vector_shll_n_u32v4(uint16x4_t a, const int n);
#define vshll_n_u16(a, n)  __builtin_mpl_vector_shll_n_u32v4(a, n)

uint64x2_t __builtin_mpl_vector_shll_n_u64v2(uint32x2_t a, const int n);
#define vshll_n_u32(a, n)  __builtin_mpl_vector_shll_n_u64v2(a, n)

int16x8_t __builtin_mpl_vector_shll_high_n_i16v8(int8x16_t a, const int n);
#define vshll_high_n_s8(a, n)  __builtin_mpl_vector_shll_high_n_i16v8(a, n)

int32x4_t __builtin_mpl_vector_shll_high_n_i32v4(int16x8_t a, const int n);
#define vshll_high_n_s16(a, n)  __builtin_mpl_vector_shll_high_n_i32v4(a, n)

int64x2_t __builtin_mpl_vector_shll_high_n_i64v2(int32x4_t a, const int n);
#define vshll_high_n_s32(a, n)  __builtin_mpl_vector_shll_high_n_i64v2(a, n)

uint16x8_t __builtin_mpl_vector_shll_high_n_u16v8(uint8x16_t a, const int n);
#define vshll_high_n_u8(a, n)  __builtin_mpl_vector_shll_high_n_u16v8(a, n)

uint32x4_t __builtin_mpl_vector_shll_high_n_u32v4(uint16x8_t a, const int n);
#define vshll_high_n_u16(a, n)  __builtin_mpl_vector_shll_high_n_u32v4(a, n)

uint64x2_t __builtin_mpl_vector_shll_high_n_u64v2(uint32x4_t a, const int n);
#define vshll_high_n_u32(a, n)  __builtin_mpl_vector_shll_high_n_u64v2(a, n)

int8x8_t __builtin_mpl_vector_sli_n_i8v8(int8x8_t a, int8x8_t b, const int n);
#define vsli_n_s8(a, b, n)  __builtin_mpl_vector_sli_n_i8v8(a, b, n)

int8x16_t __builtin_mpl_vector_sliq_n_i8v16(int8x16_t a, int8x16_t b, const int n);
#define vsliq_n_s8(a, b, n)  __builtin_mpl_vector_sliq_n_i8v16(a, b, n)

int16x4_t __builtin_mpl_vector_sli_n_i16v4(int16x4_t a, int16x4_t b, const int n);
#define vsli_n_s16(a, b, n)  __builtin_mpl_vector_sli_n_i16v4(a, b, n)

int16x8_t __builtin_mpl_vector_sliq_n_i16v8(int16x8_t a, int16x8_t b, const int n);
#define vsliq_n_s16(a, b, n)  __builtin_mpl_vector_sliq_n_i16v8(a, b, n)

int32x2_t __builtin_mpl_vector_sli_n_i32v2(int32x2_t a, int32x2_t b, const int n);
#define vsli_n_s32(a, b, n)  __builtin_mpl_vector_sli_n_i32v2(a, b, n)

int32x4_t __builtin_mpl_vector_sliq_n_i32v4(int32x4_t a, int32x4_t b, const int n);
#define vsliq_n_s32(a, b, n)  __builtin_mpl_vector_sliq_n_i32v4(a, b, n)

int64x1_t __builtin_mpl_vector_sli_n_i64v1(int64x1_t a, int64x1_t b, const int n);
#define vsli_n_s64(a, b, n)  __builtin_mpl_vector_sli_n_i64v1(a, b, n)

int64x2_t __builtin_mpl_vector_sliq_n_i64v2(int64x2_t a, int64x2_t b, const int n);
#define vsliq_n_s64(a, b, n)  __builtin_mpl_vector_sliq_n_i64v2(a, b, n)

uint8x8_t __builtin_mpl_vector_sli_n_u8v8(uint8x8_t a, uint8x8_t b, const int n);
#define vsli_n_u8(a, b, n)  __builtin_mpl_vector_sli_n_u8v8(a, b, n)

uint8x16_t __builtin_mpl_vector_sliq_n_u8v16(uint8x16_t a, uint8x16_t b, const int n);
#define vsliq_n_u8(a, b, n)  __builtin_mpl_vector_sliq_n_u8v16(a, b, n)

uint16x4_t __builtin_mpl_vector_sli_n_u16v4(uint16x4_t a, uint16x4_t b, const int n);
#define vsli_n_u16(a, b, n)  __builtin_mpl_vector_sli_n_u16v4(a, b, n)

uint16x8_t __builtin_mpl_vector_sliq_n_u16v8(uint16x8_t a, uint16x8_t b, const int n);
#define vsliq_n_u16(a, b, n)  __builtin_mpl_vector_sliq_n_u16v8(a, b, n)

uint32x2_t __builtin_mpl_vector_sli_n_u32v2(uint32x2_t a, uint32x2_t b, const int n);
#define vsli_n_u32(a, b, n)  __builtin_mpl_vector_sli_n_u32v2(a, b, n)

uint32x4_t __builtin_mpl_vector_sliq_n_u32v4(uint32x4_t a, uint32x4_t b, const int n);
#define vsliq_n_u32(a, b, n)  __builtin_mpl_vector_sliq_n_u32v4(a, b, n)

uint64x1_t __builtin_mpl_vector_sli_n_u64v1(uint64x1_t a, uint64x1_t b, const int n);
#define vsli_n_u64(a, b, n)  __builtin_mpl_vector_sli_n_u64v1(a, b, n)

uint64x2_t __builtin_mpl_vector_sliq_n_u64v2(uint64x2_t a, uint64x2_t b, const int n);
#define vsliq_n_u64(a, b, n)  __builtin_mpl_vector_sliq_n_u64v2(a, b, n)

static inline int64_t __attribute__ ((__always_inline__, __gnu_inline__))
vslid_n_s64(int64_t a, int64_t b, const int n) {
  return vget_lane_s64(vsli_n_s64((int64x1_t){a}, (int64x1_t){b}, n), 0);
}

static inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__))
vslid_n_u64(uint64_t a, uint64_t b, const int n) {
  return vget_lane_u64(vsli_n_u64((uint64x1_t){a}, (uint64x1_t){b}, n), 0);
}

int8x8_t __builtin_mpl_vector_rshr_n_i8v8(int8x8_t a, const int n);
#define vrshr_n_s8(a, n)  __builtin_mpl_vector_rshr_n_i8v8(a, n)

int8x16_t __builtin_mpl_vector_rshrq_n_i8v16(int8x16_t a, const int n);
#define vrshrq_n_s8(a, n)  __builtin_mpl_vector_rshrq_n_i8v16(a, n)

int16x4_t __builtin_mpl_vector_rshr_n_i16v4(int16x4_t a, const int n);
#define vrshr_n_s16(a, n)  __builtin_mpl_vector_rshr_n_i16v4(a, n)

int16x8_t __builtin_mpl_vector_rshrq_n_i16v8(int16x8_t a, const int n);
#define vrshrq_n_s16(a, n)  __builtin_mpl_vector_rshrq_n_i16v8(a, n)

int32x2_t __builtin_mpl_vector_rshr_n_i32v2(int32x2_t a, const int n);
#define vrshr_n_s32(a, n)  __builtin_mpl_vector_rshr_n_i32v2(a, n)

int32x4_t __builtin_mpl_vector_rshrq_n_i32v4(int32x4_t a, const int n);
#define vrshrq_n_s32(a, n)  __builtin_mpl_vector_rshrq_n_i32v4(a, n)

int64x2_t __builtin_mpl_vector_rshrq_n_i64v2(int64x2_t a, const int n);
#define vrshrq_n_s64(a, n)  __builtin_mpl_vector_rshrq_n_i64v2(a, n)

uint8x8_t __builtin_mpl_vector_rshr_n_u8v8(uint8x8_t a, const int n);
#define vrshr_n_u8(a, n)  __builtin_mpl_vector_rshr_n_u8v8(a, n)

uint8x16_t __builtin_mpl_vector_rshrq_n_u8v16(uint8x16_t a, const int n);
#define vrshrq_n_u8(a, n)  __builtin_mpl_vector_rshrq_n_u8v16(a, n)

uint16x4_t __builtin_mpl_vector_rshr_n_u16v4(uint16x4_t a, const int n);
#define vrshr_n_u16(a, n)  __builtin_mpl_vector_rshr_n_u16v4(a, n)

uint16x8_t __builtin_mpl_vector_rshrq_n_u16v8(uint16x8_t a, const int n);
#define vrshrq_n_u16(a, n)  __builtin_mpl_vector_rshrq_n_u16v8(a, n)

uint32x2_t __builtin_mpl_vector_rshr_n_u32v2(uint32x2_t a, const int n);
#define vrshr_n_u32(a, n)  __builtin_mpl_vector_rshr_n_u32v2(a, n)

uint32x4_t __builtin_mpl_vector_rshrq_n_u32v4(uint32x4_t a, const int n);
#define vrshrq_n_u32(a, n)  __builtin_mpl_vector_rshrq_n_u32v4(a, n)

uint64x2_t __builtin_mpl_vector_rshrq_n_u64v2(uint64x2_t a, const int n);
#define vrshrq_n_u64(a, n)  __builtin_mpl_vector_rshrq_n_u64v2(a, n)

int64x1_t __builtin_mpl_vector_rshrd_n_i64(int64x1_t a, const int n);
static inline int64_t __attribute__ ((__always_inline__, __gnu_inline__))
vrshrd_n_s64(int64_t a, const int n) {
  return vget_lane_s64(__builtin_mpl_vector_rshrd_n_i64((int64x1_t){a}, n), 0);
}

uint64x1_t __builtin_mpl_vector_rshrd_n_u64(uint64x1_t a, const int n);
static inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__))
vrshrd_n_u64(uint64_t a, const int n) {
  return vget_lane_u64(__builtin_mpl_vector_rshrd_n_u64((uint64x1_t){a}, n), 0);
}

int8x8_t __builtin_mpl_vector_sra_n_i8v8(int8x8_t a, int8x8_t b, const int n);
#define vsra_n_s8(a, b, n)  __builtin_mpl_vector_sra_n_i8v8(a, b, n)

int8x16_t __builtin_mpl_vector_sraq_n_i8v16(int8x16_t a, int8x16_t b, const int n);
#define vsraq_n_s8(a, b, n)  __builtin_mpl_vector_sraq_n_i8v16(a, b, n)

int16x4_t __builtin_mpl_vector_sra_n_i16v4(int16x4_t a, int16x4_t b, const int n);
#define vsra_n_s16(a, b, n)  __builtin_mpl_vector_sra_n_i16v4(a, b, n)

int16x8_t __builtin_mpl_vector_sraq_n_i16v8(int16x8_t a, int16x8_t b, const int n);
#define vsraq_n_s16(a, b, n)  __builtin_mpl_vector_sraq_n_i16v8(a, b, n)

int32x2_t __builtin_mpl_vector_sra_n_i32v2(int32x2_t a, int32x2_t b, const int n);
#define vsra_n_s32(a, b, n)  __builtin_mpl_vector_sra_n_i32v2(a, b, n)

int32x4_t __builtin_mpl_vector_sraq_n_i32v4(int32x4_t a, int32x4_t b, const int n);
#define vsraq_n_s32(a, b, n)  __builtin_mpl_vector_sraq_n_i32v4(a, b, n)

int64x2_t __builtin_mpl_vector_sraq_n_i64v2(int64x2_t a, int64x2_t b, const int n);
#define vsraq_n_s64(a, b, n)  __builtin_mpl_vector_sraq_n_i64v2(a, b, n)

uint8x8_t __builtin_mpl_vector_sra_n_u8v8(uint8x8_t a, uint8x8_t b, const int n);
#define vsra_n_u8(a, b, n)  __builtin_mpl_vector_sra_n_u8v8(a, b, n)

uint8x16_t __builtin_mpl_vector_sraq_n_u8v16(uint8x16_t a, uint8x16_t b, const int n);
#define vsraq_n_u8(a, b, n)  __builtin_mpl_vector_sraq_n_u8v16(a, b, n)

uint16x4_t __builtin_mpl_vector_sra_n_u16v4(uint16x4_t a, uint16x4_t b, const int n);
#define vsra_n_u16(a, b, n)  __builtin_mpl_vector_sra_n_u16v4(a, b, n)

uint16x8_t __builtin_mpl_vector_sraq_n_u16v8(uint16x8_t a, uint16x8_t b, const int n);
#define vsraq_n_u16(a, b, n)  __builtin_mpl_vector_sraq_n_u16v8(a, b, n)

uint32x2_t __builtin_mpl_vector_sra_n_u32v2(uint32x2_t a, uint32x2_t b, const int n);
#define vsra_n_u32(a, b, n)  __builtin_mpl_vector_sra_n_u32v2(a, b, n)

uint32x4_t __builtin_mpl_vector_sraq_n_u32v4(uint32x4_t a, uint32x4_t b, const int n);
#define vsraq_n_u32(a, b, n)  __builtin_mpl_vector_sraq_n_u32v4(a, b, n)

uint64x2_t __builtin_mpl_vector_sraq_n_u64v2(uint64x2_t a, uint64x2_t b, const int n);
#define vsraq_n_u64(a, b, n)  __builtin_mpl_vector_sraq_n_u64v2(a, b, n)

int64x1_t __builtin_mpl_vector_srad_n_i64(int64x1_t a, int64x1_t b, const int n);
static inline int64_t __attribute__ ((__always_inline__, __gnu_inline__))
vsrad_n_s64(int64_t a, int64_t b, const int n) {
  return vget_lane_s64(__builtin_mpl_vector_srad_n_i64((int64x1_t){a}, (int64x1_t){b}, n), 0);
}

uint64x1_t __builtin_mpl_vector_srad_n_u64(uint64x1_t a, uint64x1_t b, const int n);
static inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__))
vsrad_n_u64(uint64_t a, uint64_t b, const int n) {
  return vget_lane_u64(__builtin_mpl_vector_srad_n_u64((uint64x1_t){a}, (uint64x1_t){b}, n), 0);
}

int8x8_t __builtin_mpl_vector_rsra_n_i8v8(int8x8_t a, int8x8_t b, const int n);
#define vrsra_n_s8(a, b, n)  __builtin_mpl_vector_rsra_n_i8v8(a, b, n)

int8x16_t __builtin_mpl_vector_rsraq_n_i8v16(int8x16_t a, int8x16_t b, const int n);
#define vrsraq_n_s8(a, b, n)  __builtin_mpl_vector_rsraq_n_i8v16(a, b, n)

int16x4_t __builtin_mpl_vector_rsra_n_i16v4(int16x4_t a, int16x4_t b, const int n);
#define vrsra_n_s16(a, b, n)  __builtin_mpl_vector_rsra_n_i16v4(a, b, n)

int16x8_t __builtin_mpl_vector_rsraq_n_i16v8(int16x8_t a, int16x8_t b, const int n);
#define vrsraq_n_s16(a, b, n)  __builtin_mpl_vector_rsraq_n_i16v8(a, b, n)

int32x2_t __builtin_mpl_vector_rsra_n_i32v2(int32x2_t a, int32x2_t b, const int n);
#define vrsra_n_s32(a, b, n)  __builtin_mpl_vector_rsra_n_i32v2(a, b, n)

int32x4_t __builtin_mpl_vector_rsraq_n_i32v4(int32x4_t a, int32x4_t b, const int n);
#define vrsraq_n_s32(a, b, n)  __builtin_mpl_vector_rsraq_n_i32v4(a, b, n)

int64x2_t __builtin_mpl_vector_rsraq_n_i64v2(int64x2_t a, int64x2_t b, const int n);
#define vrsraq_n_s64(a, b, n)  __builtin_mpl_vector_rsraq_n_i64v2(a, b, n)

uint8x8_t __builtin_mpl_vector_rsra_n_u8v8(uint8x8_t a, uint8x8_t b, const int n);
#define vrsra_n_u8(a, b, n)  __builtin_mpl_vector_rsra_n_u8v8(a, b, n)

uint8x16_t __builtin_mpl_vector_rsraq_n_u8v16(uint8x16_t a, uint8x16_t b, const int n);
#define vrsraq_n_u8(a, b, n)  __builtin_mpl_vector_rsraq_n_u8v16(a, b, n)

uint16x4_t __builtin_mpl_vector_rsra_n_u16v4(uint16x4_t a, uint16x4_t b, const int n);
#define vrsra_n_u16(a, b, n)  __builtin_mpl_vector_rsra_n_u16v4(a, b, n)

uint16x8_t __builtin_mpl_vector_rsraq_n_u16v8(uint16x8_t a, uint16x8_t b, const int n);
#define vrsraq_n_u16(a, b, n)  __builtin_mpl_vector_rsraq_n_u16v8(a, b, n)

uint32x2_t __builtin_mpl_vector_rsra_n_u32v2(uint32x2_t a, uint32x2_t b, const int n);
#define vrsra_n_u32(a, b, n)  __builtin_mpl_vector_rsra_n_u32v2(a, b, n)

uint32x4_t __builtin_mpl_vector_rsraq_n_u32v4(uint32x4_t a, uint32x4_t b, const int n);
#define vrsraq_n_u32(a, b, n)  __builtin_mpl_vector_rsraq_n_u32v4(a, b, n)

uint64x2_t __builtin_mpl_vector_rsraq_n_u64v2(uint64x2_t a, uint64x2_t b, const int n);
#define vrsraq_n_u64(a, b, n)  __builtin_mpl_vector_rsraq_n_u64v2(a, b, n)

int64x1_t __builtin_mpl_vector_rsrad_n_i64(int64x1_t a, int64x1_t b, const int n);
static inline int64_t __attribute__ ((__always_inline__, __gnu_inline__))
vrsrad_n_s64(int64_t a, int64_t b, const int n) {
  return vget_lane_s64(__builtin_mpl_vector_rsrad_n_i64((int64x1_t){a}, (int64x1_t){b}, n), 0);
}

uint64x1_t __builtin_mpl_vector_rsrad_n_u64(uint64x1_t a, uint64x1_t b, const int n);
static inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__))
vrsrad_n_u64(uint64_t a, uint64_t b, const int n) {
  return vget_lane_u64(__builtin_mpl_vector_rsrad_n_u64((uint64x1_t){a}, (uint64x1_t){b}, n), 0);
}

int8x8_t __builtin_mpl_vector_shrn_n_i8v8(int16x8_t a, const int n);
#define vshrn_n_s16(a, n)  __builtin_mpl_vector_shrn_n_i8v8(a, n)

int16x4_t __builtin_mpl_vector_shrn_n_i16v4(int32x4_t a, const int n);
#define vshrn_n_s32(a, n)  __builtin_mpl_vector_shrn_n_i16v4(a, n)

int32x2_t __builtin_mpl_vector_shrn_n_i32v2(int64x2_t a, const int n);
#define vshrn_n_s64(a, n)  __builtin_mpl_vector_shrn_n_i32v2(a, n)

uint8x8_t __builtin_mpl_vector_shrn_n_u8v8(uint16x8_t a, const int n);
#define vshrn_n_u16(a, n)  __builtin_mpl_vector_shrn_n_u8v8(a, n)

uint16x4_t __builtin_mpl_vector_shrn_n_u16v4(uint32x4_t a, const int n);
#define vshrn_n_u32(a, n)  __builtin_mpl_vector_shrn_n_u16v4(a, n)

uint32x2_t __builtin_mpl_vector_shrn_n_u32v2(uint64x2_t a, const int n);
#define vshrn_n_u64(a, n)  __builtin_mpl_vector_shrn_n_u32v2(a, n)

int8x16_t __builtin_mpl_vector_shrn_high_n_i8v16(int8x8_t r, int16x8_t a, const int n);
#define vshrn_high_n_s16(r, a, n)  __builtin_mpl_vector_shrn_high_n_i8v16(r, a, n)

int16x8_t __builtin_mpl_vector_shrn_high_n_i16v8(int16x4_t r, int32x4_t a, const int n);
#define vshrn_high_n_s32(r, a, n)  __builtin_mpl_vector_shrn_high_n_i16v8(r, a, n)

int32x4_t __builtin_mpl_vector_shrn_high_n_i32v4(int32x2_t r, int64x2_t a, const int n);
#define vshrn_high_n_s64(r, a, n)  __builtin_mpl_vector_shrn_high_n_i32v4(r, a, n)

uint8x16_t __builtin_mpl_vector_shrn_high_n_u8v16(uint8x8_t r, uint16x8_t a, const int n);
#define vshrn_high_n_u16(r, a, n)  __builtin_mpl_vector_shrn_high_n_u8v16(r, a, n)

uint16x8_t __builtin_mpl_vector_shrn_high_n_u16v8(uint16x4_t r, uint32x4_t a, const int n);
#define vshrn_high_n_u32(r, a, n)  __builtin_mpl_vector_shrn_high_n_u16v8(r, a, n)

uint32x4_t __builtin_mpl_vector_shrn_high_n_u32v4(uint32x2_t r, uint64x2_t a, const int n);
#define vshrn_high_n_u64(r, a, n)  __builtin_mpl_vector_shrn_high_n_u32v4(r, a, n)

uint8x8_t __builtin_mpl_vector_qshrun_n_u8v8(int16x8_t a, const int n);
#define vqshrun_n_s16(a, n)  __builtin_mpl_vector_qshrun_n_u8v8(a, n)

uint16x4_t __builtin_mpl_vector_qshrun_n_u16v4(int32x4_t a, const int n);
#define vqshrun_n_s32(a, n)  __builtin_mpl_vector_qshrun_n_u16v4(a, n)

uint32x2_t __builtin_mpl_vector_qshrun_n_u32v2(int64x2_t a, const int n);
#define vqshrun_n_s64(a, n)  __builtin_mpl_vector_qshrun_n_u32v2(a, n)

static inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshrunh_n_s16(int16_t a, const int n) {
  return vget_lane_u8(vqshrun_n_s16((int16x8_t){a}, n), 0);
}

static inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshruns_n_s32(int32_t a, const int n) {
  return vget_lane_u16(vqshrun_n_s32((int32x4_t){a}, n), 0);
}

static inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshrund_n_s64(int64_t a, const int n) {
  return vget_lane_u32(vqshrun_n_s64((int64x2_t){a}, n), 0);
}

uint8x16_t __builtin_mpl_vector_qshrun_high_n_u8v16(uint8x8_t r, int16x8_t a, const int n);
#define vqshrun_high_n_s16(r, a, n)  __builtin_mpl_vector_qshrun_high_n_u8v16(r, a, n)

uint16x8_t __builtin_mpl_vector_qshrun_high_n_u16v8(uint16x4_t r, int32x4_t a, const int n);
#define vqshrun_high_n_s32(r, a, n)  __builtin_mpl_vector_qshrun_high_n_u16v8(r, a, n)

uint32x4_t __builtin_mpl_vector_qshrun_high_n_u32v4(uint32x2_t r, int64x2_t a, const int n);
#define vqshrun_high_n_s64(r, a, n)  __builtin_mpl_vector_qshrun_high_n_u32v4(r, a, n)

int8x8_t __builtin_mpl_vector_qshrn_n_i8v8(int16x8_t a, const int n);
#define vqshrn_n_s16(a, n)  __builtin_mpl_vector_qshrn_n_i8v8(a, n)

int16x4_t __builtin_mpl_vector_qshrn_n_i16v4(int32x4_t a, const int n);
#define vqshrn_n_s32(a, n)  __builtin_mpl_vector_qshrn_n_i16v4(a, n)

int32x2_t __builtin_mpl_vector_qshrn_n_i32v2(int64x2_t a, const int n);
#define vqshrn_n_s64(a, n)  __builtin_mpl_vector_qshrn_n_i32v2(a, n)

uint8x8_t __builtin_mpl_vector_qshrn_n_u8v8(uint16x8_t a, const int n);
#define vqshrn_n_u16(a, n)  __builtin_mpl_vector_qshrn_n_u8v8(a, n)

uint16x4_t __builtin_mpl_vector_qshrn_n_u16v4(uint32x4_t a, const int n);
#define vqshrn_n_u32(a, n)  __builtin_mpl_vector_qshrn_n_u16v4(a, n)

uint32x2_t __builtin_mpl_vector_qshrn_n_u32v2(uint64x2_t a, const int n);
#define vqshrn_n_u64(a, n)  __builtin_mpl_vector_qshrn_n_u32v2(a, n)

static inline int8_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshrnh_n_s16(int16_t a, const int n) {
  return vget_lane_s8(vqshrn_n_s16((int16x8_t){a}, n), 0);
}

static inline int16_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshrns_n_s32(int32_t a, const int n) {
  return vget_lane_s16(vqshrn_n_s32((int32x4_t){a}, n), 0);
}

static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshrnd_n_s64(int64_t a, const int n) {
  return vget_lane_s32(vqshrn_n_s64((int64x2_t){a}, n), 0);
}

static inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshrnh_n_u16(uint16_t a, const int n) {
  return vget_lane_u8(vqshrn_n_u16((uint16x8_t){a}, n), 0);
}

static inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshrns_n_u32(uint32_t a, const int n) {
  return vget_lane_u16(vqshrn_n_u32((uint32x4_t){a}, n), 0);
}

static inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqshrnd_n_u64(uint64_t a, const int n) {
  return vget_lane_u32(vqshrn_n_u64((uint64x2_t){a}, n), 0);
}

int8x16_t __builtin_mpl_vector_qshrn_high_n_i8v16(int8x8_t r, int16x8_t a, const int n);
#define vqshrn_high_n_s16(r, a, n)  __builtin_mpl_vector_qshrn_high_n_i8v16(r, a, n)

int16x8_t __builtin_mpl_vector_qshrn_high_n_i16v8(int16x4_t r, int32x4_t a, const int n);
#define vqshrn_high_n_s32(r, a, n)  __builtin_mpl_vector_qshrn_high_n_i16v8(r, a, n)

int32x4_t __builtin_mpl_vector_qshrn_high_n_i32v4(int32x2_t r, int64x2_t a, const int n);
#define vqshrn_high_n_s64(r, a, n)  __builtin_mpl_vector_qshrn_high_n_i32v4(r, a, n)

uint8x16_t __builtin_mpl_vector_qshrn_high_n_u8v16(uint8x8_t r, uint16x8_t a, const int n);
#define vqshrn_high_n_u16(r, a, n)  __builtin_mpl_vector_qshrn_high_n_u8v16(r, a, n)

uint16x8_t __builtin_mpl_vector_qshrn_high_n_u16v8(uint16x4_t r, uint32x4_t a, const int n);
#define vqshrn_high_n_u32(r, a, n)  __builtin_mpl_vector_qshrn_high_n_u16v8(r, a, n)

uint32x4_t __builtin_mpl_vector_qshrn_high_n_u32v4(uint32x2_t r, uint64x2_t a, const int n);
#define vqshrn_high_n_u64(r, a, n)  __builtin_mpl_vector_qshrn_high_n_u32v4(r, a, n)

uint8x8_t __builtin_mpl_vector_qrshrun_n_u8v8(int16x8_t a, const int n);
#define vqrshrun_n_s16(a, n)  __builtin_mpl_vector_qrshrun_n_u8v8(a, n)

uint16x4_t __builtin_mpl_vector_qrshrun_n_u16v4(int32x4_t a, const int n);
#define vqrshrun_n_s32(a, n)  __builtin_mpl_vector_qrshrun_n_u16v4(a, n)

uint32x2_t __builtin_mpl_vector_qrshrun_n_u32v2(int64x2_t a, const int n);
#define vqrshrun_n_s64(a, n)  __builtin_mpl_vector_qrshrun_n_u32v2(a, n)

static inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__))
vqrshrunh_n_s16(int16_t a, const int n) {
  return vget_lane_u8(vqrshrun_n_s16((int16x8_t){a}, n), 0);
}

static inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__))
vqrshruns_n_s32(int32_t a, const int n) {
  return vget_lane_u16(vqrshrun_n_s32((int32x4_t){a}, n), 0);
}

static inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqrshrund_n_s64(int64_t a, const int n) {
  return vget_lane_u32(vqrshrun_n_s64((int64x2_t){a}, n), 0);
}

uint8x16_t __builtin_mpl_vector_qrshrun_high_n_u8v16(uint8x8_t r, int16x8_t a, const int n);
#define vqrshrun_high_n_s16(r, a, n)  __builtin_mpl_vector_qrshrun_high_n_u8v16(r, a, n)

uint16x8_t __builtin_mpl_vector_qrshrun_high_n_u16v8(uint16x4_t r, int32x4_t a, const int n);
#define vqrshrun_high_n_s32(r, a, n)  __builtin_mpl_vector_qrshrun_high_n_u16v8(r, a, n)

uint32x4_t __builtin_mpl_vector_qrshrun_high_n_u32v4(uint32x2_t r, int64x2_t a, const int n);
#define vqrshrun_high_n_s64(r, a, n)  __builtin_mpl_vector_qrshrun_high_n_u32v4(r, a, n)

int8x8_t __builtin_mpl_vector_qrshrn_n_i8v8(int16x8_t a, const int n);
#define vqrshrn_n_s16(a, n)  __builtin_mpl_vector_qrshrn_n_i8v8(a, n)

int16x4_t __builtin_mpl_vector_qrshrn_n_i16v4(int32x4_t a, const int n);
#define vqrshrn_n_s32(a, n)  __builtin_mpl_vector_qrshrn_n_i16v4(a, n)

int32x2_t __builtin_mpl_vector_qrshrn_n_i32v2(int64x2_t a, const int n);
#define vqrshrn_n_s64(a, n)  __builtin_mpl_vector_qrshrn_n_i32v2(a, n)

uint8x8_t __builtin_mpl_vector_qrshrn_n_u8v8(uint16x8_t a, const int n);
#define vqrshrn_n_u16(a, n)  __builtin_mpl_vector_qrshrn_n_u8v8(a, n)

uint16x4_t __builtin_mpl_vector_qrshrn_n_u16v4(uint32x4_t a, const int n);
#define vqrshrn_n_u32(a, n)  __builtin_mpl_vector_qrshrn_n_u16v4(a, n)

uint32x2_t __builtin_mpl_vector_qrshrn_n_u32v2(uint64x2_t a, const int n);
#define vqrshrn_n_u64(a, n)  __builtin_mpl_vector_qrshrn_n_u32v2(a, n)

static inline int8_t __attribute__ ((__always_inline__, __gnu_inline__))
vqrshrnh_n_s16(int16_t a, const int n) {
  return vget_lane_s8(vqrshrn_n_s16((int16x8_t){a}, n), 0);
}

static inline int16_t __attribute__ ((__always_inline__, __gnu_inline__))
vqrshrns_n_s32(int32_t a, const int n) {
  return vget_lane_s16(vqrshrn_n_s32((int32x4_t){a}, n), 0);
}

static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqrshrnd_n_s64(int64_t a, const int n) {
  return vget_lane_s32(vqrshrn_n_s64((int64x2_t){a}, n), 0);
}

static inline uint8_t __attribute__ ((__always_inline__, __gnu_inline__))
vqrshrnh_n_u16(uint16_t a, const int n) {
  return vget_lane_u8(vqrshrn_n_u16((uint16x8_t){a}, n), 0);
}

static inline uint16_t __attribute__ ((__always_inline__, __gnu_inline__))
vqrshrns_n_u32(uint32_t a, const int n) {
  return vget_lane_u16(vqrshrn_n_u32((uint32x4_t){a}, n), 0);
}

static inline uint32_t __attribute__ ((__always_inline__, __gnu_inline__))
vqrshrnd_n_u64(uint64_t a, const int n) {
  return vget_lane_u32(vqrshrn_n_u64((uint64x2_t){a}, n), 0);
}

int8x16_t __builtin_mpl_vector_qrshrn_high_n_i8v16(int8x8_t r, int16x8_t a, const int n);
#define vqrshrn_high_n_s16(r, a, n)  __builtin_mpl_vector_qrshrn_high_n_i8v16(r, a, n)

int16x8_t __builtin_mpl_vector_qrshrn_high_n_i16v8(int16x4_t r, int32x4_t a, const int n);
#define vqrshrn_high_n_s32(r, a, n)  __builtin_mpl_vector_qrshrn_high_n_i16v8(r, a, n)

int32x4_t __builtin_mpl_vector_qrshrn_high_n_i32v4(int32x2_t r, int64x2_t a, const int n);
#define vqrshrn_high_n_s64(r, a, n)  __builtin_mpl_vector_qrshrn_high_n_i32v4(r, a, n)

uint8x16_t __builtin_mpl_vector_qrshrn_high_n_u8v16(uint8x8_t r, uint16x8_t a, const int n);
#define vqrshrn_high_n_u16(r, a, n)  __builtin_mpl_vector_qrshrn_high_n_u8v16(r, a, n)

uint16x8_t __builtin_mpl_vector_qrshrn_high_n_u16v8(uint16x4_t r, uint32x4_t a, const int n);
#define vqrshrn_high_n_u32(r, a, n)  __builtin_mpl_vector_qrshrn_high_n_u16v8(r, a, n)

uint32x4_t __builtin_mpl_vector_qrshrn_high_n_u32v4(uint32x2_t r, uint64x2_t a, const int n);
#define vqrshrn_high_n_u64(r, a, n)  __builtin_mpl_vector_qrshrn_high_n_u32v4(r, a, n)

int8x8_t __builtin_mpl_vector_rshrn_n_i8v8(int16x8_t a, const int n);
#define vrshrn_n_s16(a, n)  __builtin_mpl_vector_rshrn_n_i8v8(a, n)

int16x4_t __builtin_mpl_vector_rshrn_n_i16v4(int32x4_t a, const int n);
#define vrshrn_n_s32(a, n)  __builtin_mpl_vector_rshrn_n_i16v4(a, n)

int32x2_t __builtin_mpl_vector_rshrn_n_i32v2(int64x2_t a, const int n);
#define vrshrn_n_s64(a, n)  __builtin_mpl_vector_rshrn_n_i32v2(a, n)

uint8x8_t __builtin_mpl_vector_rshrn_n_u8v8(uint16x8_t a, const int n);
#define vrshrn_n_u16(a, n)  __builtin_mpl_vector_rshrn_n_u8v8(a, n)

uint16x4_t __builtin_mpl_vector_rshrn_n_u16v4(uint32x4_t a, const int n);
#define vrshrn_n_u32(a, n)  __builtin_mpl_vector_rshrn_n_u16v4(a, n)

uint32x2_t __builtin_mpl_vector_rshrn_n_u32v2(uint64x2_t a, const int n);
#define vrshrn_n_u64(a, n)  __builtin_mpl_vector_rshrn_n_u32v2(a, n)

int8x16_t __builtin_mpl_vector_rshrn_high_n_i8v16(int8x8_t r, int16x8_t a, const int n);
#define vrshrn_high_n_s16(r, a, n)  __builtin_mpl_vector_rshrn_high_n_i8v16(r, a, n)

int16x8_t __builtin_mpl_vector_rshrn_high_n_i16v8(int16x4_t r, int32x4_t a, const int n);
#define vrshrn_high_n_s32(r, a, n)  __builtin_mpl_vector_rshrn_high_n_i16v8(r, a, n)

int32x4_t __builtin_mpl_vector_rshrn_high_n_i32v4(int32x2_t r, int64x2_t a, const int n);
#define vrshrn_high_n_s64(r, a, n)  __builtin_mpl_vector_rshrn_high_n_i32v4(r, a, n)

uint8x16_t __builtin_mpl_vector_rshrn_high_n_u8v16(uint8x8_t r, uint16x8_t a, const int n);
#define vrshrn_high_n_u16(r, a, n)  __builtin_mpl_vector_rshrn_high_n_u8v16(r, a, n)

uint16x8_t __builtin_mpl_vector_rshrn_high_n_u16v8(uint16x4_t r, uint32x4_t a, const int n);
#define vrshrn_high_n_u32(r, a, n)  __builtin_mpl_vector_rshrn_high_n_u16v8(r, a, n)

uint32x4_t __builtin_mpl_vector_rshrn_high_n_u32v4(uint32x2_t r, uint64x2_t a, const int n);
#define vrshrn_high_n_u64(r, a, n)  __builtin_mpl_vector_rshrn_high_n_u32v4(r, a, n)

int8x8_t __builtin_mpl_vector_sri_n_i8v8(int8x8_t a, int8x8_t b, const int n);
#define vsri_n_s8(a, b, n)  __builtin_mpl_vector_sri_n_i8v8(a, b, n)

int8x16_t __builtin_mpl_vector_sriq_n_i8v16(int8x16_t a, int8x16_t b, const int n);
#define vsriq_n_s8(a, b, n)  __builtin_mpl_vector_sriq_n_i8v16(a, b, n)

int16x4_t __builtin_mpl_vector_sri_n_i16v4(int16x4_t a, int16x4_t b, const int n);
#define vsri_n_s16(a, b, n)  __builtin_mpl_vector_sri_n_i16v4(a, b, n)

int16x8_t __builtin_mpl_vector_sriq_n_i16v8(int16x8_t a, int16x8_t b, const int n);
#define vsriq_n_s16(a, b, n)  __builtin_mpl_vector_sriq_n_i16v8(a, b, n)

int32x2_t __builtin_mpl_vector_sri_n_i32v2(int32x2_t a, int32x2_t b, const int n);
#define vsri_n_s32(a, b, n)  __builtin_mpl_vector_sri_n_i32v2(a, b, n)

int32x4_t __builtin_mpl_vector_sriq_n_i32v4(int32x4_t a, int32x4_t b, const int n);
#define vsriq_n_s32(a, b, n)  __builtin_mpl_vector_sriq_n_i32v4(a, b, n)

int64x2_t __builtin_mpl_vector_sriq_n_i64v2(int64x2_t a, int64x2_t b, const int n);
#define vsriq_n_s64(a, b, n)  __builtin_mpl_vector_sriq_n_i64v2(a, b, n)

uint8x8_t __builtin_mpl_vector_sri_n_u8v8(uint8x8_t a, uint8x8_t b, const int n);
#define vsri_n_u8(a, b, n)  __builtin_mpl_vector_sri_n_u8v8(a, b, n)

uint8x16_t __builtin_mpl_vector_sriq_n_u8v16(uint8x16_t a, uint8x16_t b, const int n);
#define vsriq_n_u8(a, b, n)  __builtin_mpl_vector_sriq_n_u8v16(a, b, n)

uint16x4_t __builtin_mpl_vector_sri_n_u16v4(uint16x4_t a, uint16x4_t b, const int n);
#define vsri_n_u16(a, b, n)  __builtin_mpl_vector_sri_n_u16v4(a, b, n)

uint16x8_t __builtin_mpl_vector_sriq_n_u16v8(uint16x8_t a, uint16x8_t b, const int n);
#define vsriq_n_u16(a, b, n)  __builtin_mpl_vector_sriq_n_u16v8(a, b, n)

uint32x2_t __builtin_mpl_vector_sri_n_u32v2(uint32x2_t a, uint32x2_t b, const int n);
#define vsri_n_u32(a, b, n)  __builtin_mpl_vector_sri_n_u32v2(a, b, n)

uint32x4_t __builtin_mpl_vector_sriq_n_u32v4(uint32x4_t a, uint32x4_t b, const int n);
#define vsriq_n_u32(a, b, n)  __builtin_mpl_vector_sriq_n_u32v4(a, b, n)

uint64x2_t __builtin_mpl_vector_sriq_n_u64v2(uint64x2_t a, uint64x2_t b, const int n);
#define vsriq_n_u64(a, b, n)  __builtin_mpl_vector_sriq_n_u64v2(a, b, n)

int64x1_t __builtin_mpl_vector_srid_n_i64(int64x1_t a, int64x1_t b, const int n);
static inline int64_t __attribute__ ((__always_inline__, __gnu_inline__))
vsrid_n_s64(int64_t a, int64_t b, const int n) {
  return vget_lane_s64(__builtin_mpl_vector_srid_n_i64((int64x1_t){a}, (int64x1_t){b}, n), 0);
}

uint64x1_t __builtin_mpl_vector_srid_n_u64(uint64x1_t a, uint64x1_t b, const int n);
static inline uint64_t __attribute__ ((__always_inline__, __gnu_inline__))
vsrid_n_u64(uint64_t a, uint64_t b, const int n) {
  return vget_lane_u64(__builtin_mpl_vector_srid_n_u64((uint64x1_t){a}, (uint64x1_t){b}, n), 0);
}

int16x4_t __builtin_mpl_vector_mla_lane_i16v4(int16x4_t a, int16x4_t b, int16x4_t v, const int lane);
#define vmla_lane_s16(a, b, v, lane)  __builtin_mpl_vector_mla_lane_i16v4(a, b, v, lane)

int16x8_t __builtin_mpl_vector_mlaq_lane_i16v8(int16x8_t a, int16x8_t b, int16x4_t v, const int lane);
#define vmlaq_lane_s16(a, b, v, lane)  __builtin_mpl_vector_mlaq_lane_i16v8(a, b, v, lane)

int32x2_t __builtin_mpl_vector_mla_lane_i32v2(int32x2_t a, int32x2_t b, int32x2_t v, const int lane);
#define vmla_lane_s32(a, b, v, lane)  __builtin_mpl_vector_mla_lane_i32v2(a, b, v, lane)

int32x4_t __builtin_mpl_vector_mlaq_lane_i32v4(int32x4_t a, int32x4_t b, int32x2_t v, const int lane);
#define vmlaq_lane_s32(a, b, v, lane)  __builtin_mpl_vector_mlaq_lane_i32v4(a, b, v, lane)

uint16x4_t __builtin_mpl_vector_mla_lane_u16v4(uint16x4_t a, uint16x4_t b, uint16x4_t v, const int lane);
#define vmla_lane_u16(a, b, v, lane)  __builtin_mpl_vector_mla_lane_u16v4(a, b, v, lane)

uint16x8_t __builtin_mpl_vector_mlaq_lane_u16v8(uint16x8_t a, uint16x8_t b, uint16x4_t v, const int lane);
#define vmlaq_lane_u16(a, b, v, lane)  __builtin_mpl_vector_mlaq_lane_u16v8(a, b, v, lane)

uint32x2_t __builtin_mpl_vector_mla_lane_u32v2(uint32x2_t a, uint32x2_t b, uint32x2_t v, const int lane);
#define vmla_lane_u32(a, b, v, lane)  __builtin_mpl_vector_mla_lane_u32v2(a, b, v, lane)

uint32x4_t __builtin_mpl_vector_mlaq_lane_u32v4(uint32x4_t a, uint32x4_t b, uint32x2_t v, const int lane);
#define vmlaq_lane_u32(a, b, v, lane)  __builtin_mpl_vector_mlaq_lane_u32v4(a, b, v, lane)

int16x4_t __builtin_mpl_vector_mla_laneq_i16v4(int16x4_t a, int16x4_t b, int16x8_t v, const int lane);
#define vmla_laneq_s16(a, b, v, lane)  __builtin_mpl_vector_mla_laneq_i16v4(a, b, v, lane)

int16x8_t __builtin_mpl_vector_mlaq_laneq_i16v8(int16x8_t a, int16x8_t b, int16x8_t v, const int lane);
#define vmlaq_laneq_s16(a, b, v, lane)  __builtin_mpl_vector_mlaq_laneq_i16v8(a, b, v, lane)

int32x2_t __builtin_mpl_vector_mla_laneq_i32v2(int32x2_t a, int32x2_t b, int32x4_t v, const int lane);
#define vmla_laneq_s32(a, b, v, lane)  __builtin_mpl_vector_mla_laneq_i32v2(a, b, v, lane)

int32x4_t __builtin_mpl_vector_mlaq_laneq_i32v4(int32x4_t a, int32x4_t b, int32x4_t v, const int lane);
#define vmlaq_laneq_s32(a, b, v, lane)  __builtin_mpl_vector_mlaq_laneq_i32v4(a, b, v, lane)

uint16x4_t __builtin_mpl_vector_mla_laneq_u16v4(uint16x4_t a, uint16x4_t b, uint16x8_t v, const int lane);
#define vmla_laneq_u16(a, b, v, lane)  __builtin_mpl_vector_mla_laneq_u16v4(a, b, v, lane)

uint16x8_t __builtin_mpl_vector_mlaq_laneq_u16v8(uint16x8_t a, uint16x8_t b, uint16x8_t v, const int lane);
#define vmlaq_laneq_u16(a, b, v, lane)  __builtin_mpl_vector_mlaq_laneq_u16v8(a, b, v, lane)

uint32x2_t __builtin_mpl_vector_mla_laneq_u32v2(uint32x2_t a, uint32x2_t b, uint32x4_t v, const int lane);
#define vmla_laneq_u32(a, b, v, lane)  __builtin_mpl_vector_mla_laneq_u32v2(a, b, v, lane)

uint32x4_t __builtin_mpl_vector_mlaq_laneq_u32v4(uint32x4_t a, uint32x4_t b, uint32x4_t v, const int lane);
#define vmlaq_laneq_u32(a, b, v, lane)  __builtin_mpl_vector_mlaq_laneq_u32v4(a, b, v, lane)

int32x4_t __builtin_mpl_vector_mlal_lane_i32v4(int32x4_t a, int16x4_t b, int16x4_t v, const int lane);
#define vmlal_lane_s16(a, b, v, lane)  __builtin_mpl_vector_mlal_lane_i32v4(a, b, v, lane)

int64x2_t __builtin_mpl_vector_mlal_lane_i64v2(int64x2_t a, int32x2_t b, int32x2_t v, const int lane);
#define vmlal_lane_s32(a, b, v, lane)  __builtin_mpl_vector_mlal_lane_i64v2(a, b, v, lane)

uint32x4_t __builtin_mpl_vector_mlal_lane_u32v4(uint32x4_t a, uint16x4_t b, uint16x4_t v, const int lane);
#define vmlal_lane_u16(a, b, v, lane)  __builtin_mpl_vector_mlal_lane_u32v4(a, b, v, lane)

uint64x2_t __builtin_mpl_vector_mlal_lane_u64v2(uint64x2_t a, uint32x2_t b, uint32x2_t v, const int lane);
#define vmlal_lane_u32(a, b, v, lane)  __builtin_mpl_vector_mlal_lane_u64v2(a, b, v, lane)

int32x4_t __builtin_mpl_vector_mlal_high_lane_i32v4(int32x4_t a, int16x8_t b, int16x4_t v, const int lane);
#define vmlal_high_lane_s16(a, b, v, lane)  __builtin_mpl_vector_mlal_high_lane_i32v4(a, b, v, lane)

int64x2_t __builtin_mpl_vector_mlal_high_lane_i64v2(int64x2_t a, int32x4_t b, int32x2_t v, const int lane);
#define vmlal_high_lane_s32(a, b, v, lane)  __builtin_mpl_vector_mlal_high_lane_i64v2(a, b, v, lane)

uint32x4_t __builtin_mpl_vector_mlal_high_lane_u32v4(uint32x4_t a, uint16x8_t b, uint16x4_t v, const int lane);
#define vmlal_high_lane_u16(a, b, v, lane)  __builtin_mpl_vector_mlal_high_lane_u32v4(a, b, v, lane)

uint64x2_t __builtin_mpl_vector_mlal_high_lane_u64v2(uint64x2_t a, uint32x4_t b, uint32x2_t v, const int lane);
#define vmlal_high_lane_u32(a, b, v, lane)  __builtin_mpl_vector_mlal_high_lane_u64v2(a, b, v, lane)

int32x4_t __builtin_mpl_vector_mlal_laneq_i32v4(int32x4_t a, int16x4_t b, int16x8_t v, const int lane);
#define vmlal_laneq_s16(a, b, v, lane)  __builtin_mpl_vector_mlal_laneq_i32v4(a, b, v, lane)

int64x2_t __builtin_mpl_vector_mlal_laneq_i64v2(int64x2_t a, int32x2_t b, int32x4_t v, const int lane);
#define vmlal_laneq_s32(a, b, v, lane)  __builtin_mpl_vector_mlal_laneq_i64v2(a, b, v, lane)

uint32x4_t __builtin_mpl_vector_mlal_laneq_u32v4(uint32x4_t a, uint16x4_t b, uint16x8_t v, const int lane);
#define vmlal_laneq_u16(a, b, v, lane)  __builtin_mpl_vector_mlal_laneq_u32v4(a, b, v, lane)

uint64x2_t __builtin_mpl_vector_mlal_laneq_u64v2(uint64x2_t a, uint32x2_t b, uint32x4_t v, const int lane);
#define vmlal_laneq_u32(a, b, v, lane)  __builtin_mpl_vector_mlal_laneq_u64v2(a, b, v, lane)

int32x4_t __builtin_mpl_vector_mlal_high_laneq_i32v4(int32x4_t a, int16x8_t b, int16x8_t v, const int lane);
#define vmlal_high_laneq_s16(a, b, v, lane)  __builtin_mpl_vector_mlal_high_laneq_i32v4(a, b, v, lane)

int64x2_t __builtin_mpl_vector_mlal_high_laneq_i64v2(int64x2_t a, int32x4_t b, int32x4_t v, const int lane);
#define vmlal_high_laneq_s32(a, b, v, lane)  __builtin_mpl_vector_mlal_high_laneq_i64v2(a, b, v, lane)

uint32x4_t __builtin_mpl_vector_mlal_high_laneq_u32v4(uint32x4_t a, uint16x8_t b, uint16x8_t v, const int lane);
#define vmlal_high_laneq_u16(a, b, v, lane)  __builtin_mpl_vector_mlal_high_laneq_u32v4(a, b, v, lane)

uint64x2_t __builtin_mpl_vector_mlal_high_laneq_u64v2(uint64x2_t a, uint32x4_t b, uint32x4_t v, const int lane);
#define vmlal_high_laneq_u32(a, b, v, lane)  __builtin_mpl_vector_mlal_high_laneq_u64v2(a, b, v, lane)

int16x4_t __builtin_mpl_vector_mla_n_i16v4(int16x4_t a, int16x4_t b, int16x4_t c);
static inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__))
vmla_n_s16(int16x4_t a, int16x4_t b, int16_t c) {
  return __builtin_mpl_vector_mla_n_i16v4(a, b, (int16x4_t){c});
}

int16x8_t __builtin_mpl_vector_mlaq_n_i16v8(int16x8_t a, int16x8_t b, int16x8_t c);
static inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__))
vmlaq_n_s16(int16x8_t a, int16x8_t b, int16_t c) {
  return __builtin_mpl_vector_mlaq_n_i16v8(a, b, (int16x8_t){c});
}

int32x2_t __builtin_mpl_vector_mla_n_i32v2(int32x2_t a, int32x2_t b, int32x2_t c);
static inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__))
vmla_n_s32(int32x2_t a, int32x2_t b, int32_t c) {
  return __builtin_mpl_vector_mla_n_i32v2(a, b, (int32x2_t){c});
}

int32x4_t __builtin_mpl_vector_mlaq_n_i32v4(int32x4_t a, int32x4_t b, int32x4_t c);
static inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__))
vmlaq_n_s32(int32x4_t a, int32x4_t b, int32_t c) {
  return __builtin_mpl_vector_mlaq_n_i32v4(a, b, (int32x4_t){c});
}

uint16x4_t __builtin_mpl_vector_mla_n_u16v4(uint16x4_t a, uint16x4_t b, uint16x4_t c);
static inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__))
vmla_n_u16(uint16x4_t a, uint16x4_t b, uint16_t c) {
  return __builtin_mpl_vector_mla_n_u16v4(a, b, (uint16x4_t){c});
}

uint16x8_t __builtin_mpl_vector_mlaq_n_u16v8(uint16x8_t a, uint16x8_t b, uint16x8_t c);
static inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__))
vmlaq_n_u16(uint16x8_t a, uint16x8_t b, uint16_t c) {
  return __builtin_mpl_vector_mlaq_n_u16v8(a, b, (uint16x8_t){c});
}

uint32x2_t __builtin_mpl_vector_mla_n_u32v2(uint32x2_t a, uint32x2_t b, uint32x2_t c);
static inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__))
vmla_n_u32(uint32x2_t a, uint32x2_t b, uint32_t c) {
  return __builtin_mpl_vector_mla_n_u32v2(a, b, (uint32x2_t){c});
}

uint32x4_t __builtin_mpl_vector_mlaq_n_u32v4(uint32x4_t a, uint32x4_t b, uint32x4_t c);
static inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__))
vmlaq_n_u32(uint32x4_t a, uint32x4_t b, uint32_t c) {
  return __builtin_mpl_vector_mlaq_n_u32v4(a, b, (uint32x4_t){c});
}

int16x4_t __builtin_mpl_vector_mls_lane_i16v4(int16x4_t a, int16x4_t b, int16x4_t v, const int lane);
#define vmls_lane_s16(a, b, v, lane)  __builtin_mpl_vector_mls_lane_i16v4(a, b, v, lane)

int16x8_t __builtin_mpl_vector_mlsq_lane_i16v8(int16x8_t a, int16x8_t b, int16x4_t v, const int lane);
#define vmlsq_lane_s16(a, b, v, lane)  __builtin_mpl_vector_mlsq_lane_i16v8(a, b, v, lane)

int32x2_t __builtin_mpl_vector_mls_lane_i32v2(int32x2_t a, int32x2_t b, int32x2_t v, const int lane);
#define vmls_lane_s32(a, b, v, lane)  __builtin_mpl_vector_mls_lane_i32v2(a, b, v, lane)

int32x4_t __builtin_mpl_vector_mlsq_lane_i32v4(int32x4_t a, int32x4_t b, int32x2_t v, const int lane);
#define vmlsq_lane_s32(a, b, v, lane)  __builtin_mpl_vector_mlsq_lane_i32v4(a, b, v, lane)

uint16x4_t __builtin_mpl_vector_mls_lane_u16v4(uint16x4_t a, uint16x4_t b, uint16x4_t v, const int lane);
#define vmls_lane_u16(a, b, v, lane)  __builtin_mpl_vector_mls_lane_u16v4(a, b, v, lane)

uint16x8_t __builtin_mpl_vector_mlsq_lane_u16v8(uint16x8_t a, uint16x8_t b, uint16x4_t v, const int lane);
#define vmlsq_lane_u16(a, b, v, lane)  __builtin_mpl_vector_mlsq_lane_u16v8(a, b, v, lane)

uint32x2_t __builtin_mpl_vector_mls_lane_u32v2(uint32x2_t a, uint32x2_t b, uint32x2_t v, const int lane);
#define vmls_lane_u32(a, b, v, lane)  __builtin_mpl_vector_mls_lane_u32v2(a, b, v, lane)

uint32x4_t __builtin_mpl_vector_mlsq_lane_u32v4(uint32x4_t a, uint32x4_t b, uint32x2_t v, const int lane);
#define vmlsq_lane_u32(a, b, v, lane)  __builtin_mpl_vector_mlsq_lane_u32v4(a, b, v, lane)

int16x4_t __builtin_mpl_vector_mls_laneq_i16v4(int16x4_t a, int16x4_t b, int16x8_t v, const int lane);
#define vmls_laneq_s16(a, b, v, lane)  __builtin_mpl_vector_mls_laneq_i16v4(a, b, v, lane)

int16x8_t __builtin_mpl_vector_mlsq_laneq_i16v8(int16x8_t a, int16x8_t b, int16x8_t v, const int lane);
#define vmlsq_laneq_s16(a, b, v, lane)  __builtin_mpl_vector_mlsq_laneq_i16v8(a, b, v, lane)

int32x2_t __builtin_mpl_vector_mls_laneq_i32v2(int32x2_t a, int32x2_t b, int32x4_t v, const int lane);
#define vmls_laneq_s32(a, b, v, lane)  __builtin_mpl_vector_mls_laneq_i32v2(a, b, v, lane)

int32x4_t __builtin_mpl_vector_mlsq_laneq_i32v4(int32x4_t a, int32x4_t b, int32x4_t v, const int lane);
#define vmlsq_laneq_s32(a, b, v, lane)  __builtin_mpl_vector_mlsq_laneq_i32v4(a, b, v, lane)

uint16x4_t __builtin_mpl_vector_mls_laneq_u16v4(uint16x4_t a, uint16x4_t b, uint16x8_t v, const int lane);
#define vmls_laneq_u16(a, b, v, lane)  __builtin_mpl_vector_mls_laneq_u16v4(a, b, v, lane)

uint16x8_t __builtin_mpl_vector_mlsq_laneq_u16v8(uint16x8_t a, uint16x8_t b, uint16x8_t v, const int lane);
#define vmlsq_laneq_u16(a, b, v, lane)  __builtin_mpl_vector_mlsq_laneq_u16v8(a, b, v, lane)

uint32x2_t __builtin_mpl_vector_mls_laneq_u32v2(uint32x2_t a, uint32x2_t b, uint32x4_t v, const int lane);
#define vmls_laneq_u32(a, b, v, lane)  __builtin_mpl_vector_mls_laneq_u32v2(a, b, v, lane)

uint32x4_t __builtin_mpl_vector_mlsq_laneq_u32v4(uint32x4_t a, uint32x4_t b, uint32x4_t v, const int lane);
#define vmlsq_laneq_u32(a, b, v, lane)  __builtin_mpl_vector_mlsq_laneq_u32v4(a, b, v, lane)

int32x4_t __builtin_mpl_vector_mlsl_lane_i32v4(int32x4_t a, int16x4_t b, int16x4_t v, const int lane);
#define vmlsl_lane_s16(a, b, v, lane)  __builtin_mpl_vector_mlsl_lane_i32v4(a, b, v, lane)

int64x2_t __builtin_mpl_vector_mlsl_lane_i64v2(int64x2_t a, int32x2_t b, int32x2_t v, const int lane);
#define vmlsl_lane_s32(a, b, v, lane)  __builtin_mpl_vector_mlsl_lane_i64v2(a, b, v, lane)

uint32x4_t __builtin_mpl_vector_mlsl_lane_u32v4(uint32x4_t a, uint16x4_t b, uint16x4_t v, const int lane);
#define vmlsl_lane_u16(a, b, v, lane)  __builtin_mpl_vector_mlsl_lane_u32v4(a, b, v, lane)

uint64x2_t __builtin_mpl_vector_mlsl_lane_u64v2(uint64x2_t a, uint32x2_t b, uint32x2_t v, const int lane);
#define vmlsl_lane_u32(a, b, v, lane)  __builtin_mpl_vector_mlsl_lane_u64v2(a, b, v, lane)

int32x4_t __builtin_mpl_vector_mlsl_high_lane_i32v4(int32x4_t a, int16x8_t b, int16x4_t v, const int lane);
#define vmlsl_high_lane_s16(a, b, v, lane)  __builtin_mpl_vector_mlsl_high_lane_i32v4(a, b, v, lane)

int64x2_t __builtin_mpl_vector_mlsl_high_lane_i64v2(int64x2_t a, int32x4_t b, int32x2_t v, const int lane);
#define vmlsl_high_lane_s32(a, b, v, lane)  __builtin_mpl_vector_mlsl_high_lane_i64v2(a, b, v, lane)

uint32x4_t __builtin_mpl_vector_mlsl_high_lane_u32v4(uint32x4_t a, uint16x8_t b, uint16x4_t v, const int lane);
#define vmlsl_high_lane_u16(a, b, v, lane)  __builtin_mpl_vector_mlsl_high_lane_u32v4(a, b, v, lane)

uint64x2_t __builtin_mpl_vector_mlsl_high_lane_u64v2(uint64x2_t a, uint32x4_t b, uint32x2_t v, const int lane);
#define vmlsl_high_lane_u32(a, b, v, lane)  __builtin_mpl_vector_mlsl_high_lane_u64v2(a, b, v, lane)

int32x4_t __builtin_mpl_vector_mlsl_laneq_i32v4(int32x4_t a, int16x4_t b, int16x8_t v, const int lane);
#define vmlsl_laneq_s16(a, b, v, lane)  __builtin_mpl_vector_mlsl_laneq_i32v4(a, b, v, lane)

int64x2_t __builtin_mpl_vector_mlsl_laneq_i64v2(int64x2_t a, int32x2_t b, int32x4_t v, const int lane);
#define vmlsl_laneq_s32(a, b, v, lane)  __builtin_mpl_vector_mlsl_laneq_i64v2(a, b, v, lane)

uint32x4_t __builtin_mpl_vector_mlsl_laneq_u32v4(uint32x4_t a, uint16x4_t b, uint16x8_t v, const int lane);
#define vmlsl_laneq_u16(a, b, v, lane)  __builtin_mpl_vector_mlsl_laneq_u32v4(a, b, v, lane)

uint64x2_t __builtin_mpl_vector_mlsl_laneq_u64v2(uint64x2_t a, uint32x2_t b, uint32x4_t v, const int lane);
#define vmlsl_laneq_u32(a, b, v, lane)  __builtin_mpl_vector_mlsl_laneq_u64v2(a, b, v, lane)

int32x4_t __builtin_mpl_vector_mlsl_high_laneq_i32v4(int32x4_t a, int16x8_t b, int16x8_t v, const int lane);
#define vmlsl_high_laneq_s16(a, b, v, lane)  __builtin_mpl_vector_mlsl_high_laneq_i32v4(a, b, v, lane)

int64x2_t __builtin_mpl_vector_mlsl_high_laneq_i64v2(int64x2_t a, int32x4_t b, int32x4_t v, const int lane);
#define vmlsl_high_laneq_s32(a, b, v, lane)  __builtin_mpl_vector_mlsl_high_laneq_i64v2(a, b, v, lane)

uint32x4_t __builtin_mpl_vector_mlsl_high_laneq_u32v4(uint32x4_t a, uint16x8_t b, uint16x8_t v, const int lane);
#define vmlsl_high_laneq_u16(a, b, v, lane)  __builtin_mpl_vector_mlsl_high_laneq_u32v4(a, b, v, lane)

uint64x2_t __builtin_mpl_vector_mlsl_high_laneq_u64v2(uint64x2_t a, uint32x4_t b, uint32x4_t v, const int lane);
#define vmlsl_high_laneq_u32(a, b, v, lane)  __builtin_mpl_vector_mlsl_high_laneq_u64v2(a, b, v, lane)

int32x4_t __builtin_mpl_vector_mlal_n_i32v4(int32x4_t a, int16x4_t b, int16x4_t c);
static inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__))
vmlal_n_s16(int32x4_t a, int16x4_t b, int16_t c) {
  return __builtin_mpl_vector_mlal_n_i32v4(a, b, (int16x4_t){c});
}

int64x2_t __builtin_mpl_vector_mlal_n_i64v2(int64x2_t a, int32x2_t b, int32x2_t c);
static inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__))
vmlal_n_s32(int64x2_t a, int32x2_t b, int32_t c) {
  return __builtin_mpl_vector_mlal_n_i64v2(a, b, (int32x2_t){c});
}

uint32x4_t __builtin_mpl_vector_mlal_n_u32v4(uint32x4_t a, uint16x4_t b, uint16x4_t c);
static inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__))
vmlal_n_u16(uint32x4_t a, uint16x4_t b, uint16_t c) {
  return __builtin_mpl_vector_mlal_n_u32v4(a, b, (uint16x4_t){c});
}

uint64x2_t __builtin_mpl_vector_mlal_n_u64v2(uint64x2_t a, uint32x2_t b, uint32x2_t c);
static inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__))
vmlal_n_u32(uint64x2_t a, uint32x2_t b, uint32_t c) {
  return __builtin_mpl_vector_mlal_n_u64v2(a, b, (uint32x2_t){c});
}

int32x4_t __builtin_mpl_vector_mlal_high_n_i32v4(int32x4_t a, int16x8_t b, int16x8_t c);
static inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__))
vmlal_high_n_s16(int32x4_t a, int16x8_t b, int16_t c) {
  return __builtin_mpl_vector_mlal_high_n_i32v4(a, b, (int16x8_t){c});
}

int64x2_t __builtin_mpl_vector_mlal_high_n_i64v2(int64x2_t a, int32x4_t b, int32x4_t c);
static inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__))
vmlal_high_n_s32(int64x2_t a, int32x4_t b, int32_t c) {
  return __builtin_mpl_vector_mlal_high_n_i64v2(a, b, (int32x4_t){c});
}

uint32x4_t __builtin_mpl_vector_mlal_high_n_u32v4(uint32x4_t a, uint16x8_t b, uint16x8_t c);
static inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__))
vmlal_high_n_u16(uint32x4_t a, uint16x8_t b, uint16_t c) {
  return __builtin_mpl_vector_mlal_high_n_u32v4(a, b, (uint16x8_t){c});
}

uint64x2_t __builtin_mpl_vector_mlal_high_n_u64v2(uint64x2_t a, uint32x4_t b, uint32x4_t c);
static inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__))
vmlal_high_n_u32(uint64x2_t a, uint32x4_t b, uint32_t c) {
  return __builtin_mpl_vector_mlal_high_n_u64v2(a, b, (uint32x4_t){c});
}

int16x4_t __builtin_mpl_vector_mls_n_i16v4(int16x4_t a, int16x4_t b, int16x4_t c);
static inline int16x4_t __attribute__ ((__always_inline__, __gnu_inline__))
vmls_n_s16(int16x4_t a, int16x4_t b, int16_t c) {
  return __builtin_mpl_vector_mls_n_i16v4(a, b, (int16x4_t){c});
}

int16x8_t __builtin_mpl_vector_mlsq_n_i16v8(int16x8_t a, int16x8_t b, int16x8_t c);
static inline int16x8_t __attribute__ ((__always_inline__, __gnu_inline__))
vmlsq_n_s16(int16x8_t a, int16x8_t b, int16_t c) {
  return __builtin_mpl_vector_mlsq_n_i16v8(a, b, (int16x8_t){c});
}

int32x2_t __builtin_mpl_vector_mls_n_i32v2(int32x2_t a, int32x2_t b, int32x2_t c);
static inline int32x2_t __attribute__ ((__always_inline__, __gnu_inline__))
vmls_n_s32(int32x2_t a, int32x2_t b, int32_t c) {
  return __builtin_mpl_vector_mls_n_i32v2(a, b, (int32x2_t){c});
}

int32x4_t __builtin_mpl_vector_mlsq_n_i32v4(int32x4_t a, int32x4_t b, int32x4_t c);
static inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__))
vmlsq_n_s32(int32x4_t a, int32x4_t b, int32_t c) {
  return __builtin_mpl_vector_mlsq_n_i32v4(a, b, (int32x4_t){c});
}

uint16x4_t __builtin_mpl_vector_mls_n_u16v4(uint16x4_t a, uint16x4_t b, uint16x4_t c);
static inline uint16x4_t __attribute__ ((__always_inline__, __gnu_inline__))
vmls_n_u16(uint16x4_t a, uint16x4_t b, uint16_t c) {
  return __builtin_mpl_vector_mls_n_u16v4(a, b, (uint16x4_t){c});
}

uint16x8_t __builtin_mpl_vector_mlsq_n_u16v8(uint16x8_t a, uint16x8_t b, uint16x8_t c);
static inline uint16x8_t __attribute__ ((__always_inline__, __gnu_inline__))
vmlsq_n_u16(uint16x8_t a, uint16x8_t b, uint16_t c) {
  return __builtin_mpl_vector_mlsq_n_u16v8(a, b, (uint16x8_t){c});
}

uint32x2_t __builtin_mpl_vector_mls_n_u32v2(uint32x2_t a, uint32x2_t b, uint32x2_t c);
static inline uint32x2_t __attribute__ ((__always_inline__, __gnu_inline__))
vmls_n_u32(uint32x2_t a, uint32x2_t b, uint32_t c) {
  return __builtin_mpl_vector_mls_n_u32v2(a, b, (uint32x2_t){c});
}

uint32x4_t __builtin_mpl_vector_mlsq_n_u32v4(uint32x4_t a, uint32x4_t b, uint32x4_t c);
static inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__))
vmlsq_n_u32(uint32x4_t a, uint32x4_t b, uint32_t c) {
  return __builtin_mpl_vector_mlsq_n_u32v4(a, b, (uint32x4_t){c});
}

int32x4_t __builtin_mpl_vector_mlsl_n_i32v4(int32x4_t a, int16x4_t b, int16x4_t c);
static inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__))
vmlsl_n_s16(int32x4_t a, int16x4_t b, int16_t c) {
  return __builtin_mpl_vector_mlsl_n_i32v4(a, b, (int16x4_t){c});
}

int64x2_t __builtin_mpl_vector_mlsl_n_i64v2(int64x2_t a, int32x2_t b, int32x2_t c);
static inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__))
vmlsl_n_s32(int64x2_t a, int32x2_t b, int32_t c) {
  return __builtin_mpl_vector_mlsl_n_i64v2(a, b, (int32x2_t){c});
}

uint32x4_t __builtin_mpl_vector_mlsl_n_u32v4(uint32x4_t a, uint16x4_t b, uint16x4_t c);
static inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__))
vmlsl_n_u16(uint32x4_t a, uint16x4_t b, uint16_t c) {
  return __builtin_mpl_vector_mlsl_n_u32v4(a, b, (uint16x4_t){c});
}

uint64x2_t __builtin_mpl_vector_mlsl_n_u64v2(uint64x2_t a, uint32x2_t b, uint32x2_t c);
static inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__))
vmlsl_n_u32(uint64x2_t a, uint32x2_t b, uint32_t c) {
  return __builtin_mpl_vector_mlsl_n_u64v2(a, b, (uint32x2_t){c});
}

int32x4_t __builtin_mpl_vector_mlsl_high_n_i32v4(int32x4_t a, int16x8_t b, int16x8_t c);
static inline int32x4_t __attribute__ ((__always_inline__, __gnu_inline__))
vmlsl_high_n_s16(int32x4_t a, int16x8_t b, int16_t c) {
  return __builtin_mpl_vector_mlsl_high_n_i32v4(a, b, (int16x8_t){c});
}

int64x2_t __builtin_mpl_vector_mlsl_high_n_i64v2(int64x2_t a, int32x4_t b, int32x4_t c);
static inline int64x2_t __attribute__ ((__always_inline__, __gnu_inline__))
vmlsl_high_n_s32(int64x2_t a, int32x4_t b, int32_t c) {
  return __builtin_mpl_vector_mlsl_high_n_i64v2(a, b, (int32x4_t){c});
}

uint32x4_t __builtin_mpl_vector_mlsl_high_n_u32v4(uint32x4_t a, uint16x8_t b, uint16x8_t c);
static inline uint32x4_t __attribute__ ((__always_inline__, __gnu_inline__))
vmlsl_high_n_u16(uint32x4_t a, uint16x8_t b, uint16_t c) {
  return __builtin_mpl_vector_mlsl_high_n_u32v4(a, b, (uint16x8_t){c});
}

uint64x2_t __builtin_mpl_vector_mlsl_high_n_u64v2(uint64x2_t a, uint32x4_t b, uint32x4_t c);
static inline uint64x2_t __attribute__ ((__always_inline__, __gnu_inline__))
vmlsl_high_n_u32(uint64x2_t a, uint32x4_t b, uint32_t c) {
  return __builtin_mpl_vector_mlsl_high_n_u64v2(a, b, (uint32x4_t){c});
}

int8x8_t __builtin_mpl_vector_qneg_i8v8(int8x8_t a);
#define vqneg_s8(a)  __builtin_mpl_vector_qneg_i8v8(a)

int8x16_t __builtin_mpl_vector_qnegq_i8v16(int8x16_t a);
#define vqnegq_s8(a)  __builtin_mpl_vector_qnegq_i8v16(a)

int16x4_t __builtin_mpl_vector_qneg_i16v4(int16x4_t a);
#define vqneg_s16(a)  __builtin_mpl_vector_qneg_i16v4(a)

int16x8_t __builtin_mpl_vector_qnegq_i16v8(int16x8_t a);
#define vqnegq_s16(a)  __builtin_mpl_vector_qnegq_i16v8(a)

int32x2_t __builtin_mpl_vector_qneg_i32v2(int32x2_t a);
#define vqneg_s32(a)  __builtin_mpl_vector_qneg_i32v2(a)

int32x4_t __builtin_mpl_vector_qnegq_i32v4(int32x4_t a);
#define vqnegq_s32(a)  __builtin_mpl_vector_qnegq_i32v4(a)

int64x1_t __builtin_mpl_vector_qneg_i64v1(int64x1_t a);
#define vqneg_s64(a)  __builtin_mpl_vector_qneg_i64v1(a)

int64x2_t __builtin_mpl_vector_qnegq_i64v2(int64x2_t a);
#define vqnegq_s64(a)  __builtin_mpl_vector_qnegq_i64v2(a)

static inline int8_t __attribute__ ((__always_inline__, __gnu_inline__)) vqnegb_s8(int8_t a) {
  return vget_lane_s8(vqneg_s8((int8x8_t){a}), 0);
}

static inline int16_t __attribute__ ((__always_inline__, __gnu_inline__)) vqnegh_s16(int16_t a) {
  return vget_lane_s16(vqneg_s16((int16x4_t){a}), 0);
}

static inline int32_t __attribute__ ((__always_inline__, __gnu_inline__)) vqnegs_s32(int32_t a) {
  return vget_lane_s32(vqneg_s32((int32x2_t){a}), 0);
}

static inline int64_t __attribute__ ((__always_inline__, __gnu_inline__)) vqnegd_s64(int64_t a) {
  return vget_lane_s64(vqneg_s64((int64x1_t){a}), 0);
}

#endif /* __ARM_NEON_H */
