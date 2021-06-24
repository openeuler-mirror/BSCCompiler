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

typedef struct int32x2x2_t {
  int32x2_t val[2];
} int32x2x2_t;

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

// vecTy2 vector_mul(vecTy1 src1, vecTy1 src2)
//      Multiply the elements of src1 and src2. Elements of vecTy2 are twice as
//      long as elements of vecTy1.
int64x2_t __builtin_mpl_vector_mul_v2i32(int32x2_t, int32x2_t);
int32x4_t __builtin_mpl_vector_mul_v4i16(int16x4_t, int16x4_t);
int16x8_t __builtin_mpl_vector_mul_v8i8(int8x8_t, int8x8_t);
uint64x2_t __builtin_mpl_vector_mul_v2u32(uint32x2_t, uint32x2_t);
uint32x4_t __builtin_mpl_vector_mul_v4u16(uint16x4_t, uint16x4_t);
uint16x8_t __builtin_mpl_vector_mul_v8u8(uint8x8_t, uint8x8_t);

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

// vecTy2 vector_get_low(vecTy1 src)
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
int64_t __builtin_mpl_vector_get_element_v2i64(int64x2_t, int32_t);
int32_t __builtin_mpl_vector_get_element_v4i32(int32x4_t, int32_t);
int16_t __builtin_mpl_vector_get_element_v8i16(int16x8_t, int32_t);
int8_t __builtin_mpl_vector_get_element_v16i8(int8x16_t, int32_t);
uint64_t __builtin_mpl_vector_get_element_v2u64(uint64x2_t, int32_t);
uint32_t __builtin_mpl_vector_get_element_v4u32(uint32x4_t, int32_t);
uint16_t __builtin_mpl_vector_get_element_v8u16(uint16x8_t, int32_t);
uint8_t __builtin_mpl_vector_get_element_v16u8(uint8x16_t, int32_t);
float64_t __builtin_mpl_vector_get_element_v2f64(float64x2_t, int32_t);
float32_t __builtin_mpl_vector_get_element_v4f32(float32x4_t, int32_t);
int64_t __builtin_mpl_vector_get_element_v1i64(int64x1_t, int32_t);
int32_t __builtin_mpl_vector_get_element_v2i32(int32x2_t, int32_t);
int16_t __builtin_mpl_vector_get_element_v4i16(int16x4_t, int32_t);
int8_t __builtin_mpl_vector_get_element_v8i8(int8x8_t, int32_t);
uint64_t __builtin_mpl_vector_get_element_v1u64(uint64x1_t, int32_t);
uint32_t __builtin_mpl_vector_get_element_v2u32(uint32x2_t, int32_t);
uint16_t __builtin_mpl_vector_get_element_v4u16(uint16x4_t, int32_t);
uint8_t __builtin_mpl_vector_get_element_v8u8(uint8x8_t, int32_t);
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
int64_t __builtin_mpl_vector_sum_v1i64(int64x1_t);
int32_t __builtin_mpl_vector_sum_v2i32(int32x2_t);
int16_t __builtin_mpl_vector_sum_v4i16(int16x4_t);
int8_t __builtin_mpl_vector_sum_v8i8(int8x8_t);
uint64_t __builtin_mpl_vector_sum_v1u64(uint64x1_t);
uint32_t __builtin_mpl_vector_sum_v2u32(uint32x2_t);
uint16_t __builtin_mpl_vector_sum_v4u16(uint16x4_t);
uint8_t __builtin_mpl_vector_sum_v8u8(uint8x8_t);
float64_t __builtin_mpl_vector_sum_v1f64(float64x1_t);
float32_t __builtin_mpl_vector_sum_v2f32(float32x2_t);

// vecTy table_lookup(vecTy tbl, vecTy idx)
//     Performs a table vector lookup.
int64x2_t __builtin_mpl_vector_table_lookup_v2i64(int64x2_t, int64x2_t);
int32x4_t __builtin_mpl_vector_table_lookup_v4i32(int32x4_t, int32x4_t);
int16x8_t __builtin_mpl_vector_table_lookup_v8i16(int16x8_t, int16x8_t);
int8x16_t __builtin_mpl_vector_table_lookup_v16i8(int8x16_t, int8x16_t);
uint64x2_t __builtin_mpl_vector_table_lookup_v2u64(uint64x2_t, uint64x2_t);
uint32x4_t __builtin_mpl_vector_table_lookup_v4u32(uint32x4_t, uint32x4_t);
uint16x8_t __builtin_mpl_vector_table_lookup_v8u16(uint16x8_t, uint16x8_t);
uint8x16_t __builtin_mpl_vector_table_lookup_v16u8(uint8x16_t, uint8x16_t);
float64x2_t __builtin_mpl_vector_table_lookup_v2f64(float64x2_t, float64x2_t);
float32x4_t __builtin_mpl_vector_table_lookup_v4f32(float32x4_t, float32x4_t);
int64x1_t __builtin_mpl_vector_table_lookup_v1i64(int64x1_t, int64x1_t);
int32x2_t __builtin_mpl_vector_table_lookup_v2i32(int32x2_t, int32x2_t);
int16x4_t __builtin_mpl_vector_table_lookup_v4i16(int16x4_t, int16x4_t);
int8x8_t __builtin_mpl_vector_table_lookup_v8i8(int8x8_t, int8x8_t);
uint64x1_t __builtin_mpl_vector_table_lookup_v1u64(uint64x1_t, uint64x1_t);
uint32x2_t __builtin_mpl_vector_table_lookup_v2u32(uint32x2_t, uint32x2_t);
uint16x4_t __builtin_mpl_vector_table_lookup_v4u16(uint16x4_t, uint16x4_t);
uint8x8_t __builtin_mpl_vector_table_lookup_v8u8(uint8x8_t, uint8x8_t);
float64x1_t __builtin_mpl_vector_table_lookup_v1f64(float64x1_t, float64x1_t);
float32x2_t __builtin_mpl_vector_table_lookup_v2f32(float32x2_t, float32x2_t);

// vecTy vector_load(scalarTy *ptr)
//     Load the elements pointed to by ptr into a vector.
int64x2_t __builtin_mpl_vector_load_v2i64(int64_t *);
int32x4_t __builtin_mpl_vector_load_v4i32(int32_t *);
int16x8_t __builtin_mpl_vector_load_v8i16(int16_t *);
int8x16_t __builtin_mpl_vector_load_v16i8(int8_t *);
uint64x2_t __builtin_mpl_vector_load_v2u64(uint64_t *);
uint32x4_t __builtin_mpl_vector_load_v4u32(uint32_t *);
uint16x8_t __builtin_mpl_vector_load_v8u16(uint16_t *);
uint8x16_t __builtin_mpl_vector_load_v16u8(uint8_t *);
float64x2_t __builtin_mpl_vector_load_v2f64(float64_t *);
float32x4_t __builtin_mpl_vector_load_v4f32(float32_t *);
int64x1_t __builtin_mpl_vector_load_v1i64(int64_t *);
int32x2_t __builtin_mpl_vector_load_v2i32(int32_t *);
int16x4_t __builtin_mpl_vector_load_v4i16(int16_t *);
int8x8_t __builtin_mpl_vector_load_v8i8(int8_t *);
uint64x1_t __builtin_mpl_vector_load_v1u64(uint64_t *);
uint32x2_t __builtin_mpl_vector_load_v2u32(uint32_t *);
uint16x4_t __builtin_mpl_vector_load_v4u16(uint16_t *);
uint8x8_t __builtin_mpl_vector_load_v8u8(uint8_t *);
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

// Temporary builtins that should be replaced by standard ops.
uint16x8_t __builtin_mpl_vector_and_v8u16(uint16x8_t, uint16x8_t);
int32x4_t __builtin_mpl_vector_and_v4i32(int32x4_t, int32x4_t);
uint16x8_t __builtin_mpl_vector_eq_v8u16(uint16x8_t, uint16x8_t);
uint16x8_t __builtin_mpl_vector_shl_v8u16(uint16x8_t, int16x8_t);
uint64x2_t __builtin_mpl_vector_shli_v2u64(uint64x2_t, const int);
uint64x2_t __builtin_mpl_vector_shri_v2u64(uint64x2_t, const int);
uint32x4_t __builtin_mpl_vector_xor_v4u32(uint32x4_t, uint32x4_t);
uint64x2_t __builtin_mpl_vector_xor_v2u64(uint64x2_t, uint64x2_t);

#define vaddvq_u16(a) __builtin_mpl_vector_sum_v8u16(a)
#define vandq_u16(a, b) __builtin_mpl_vector_and_v8u16(a, b) // (a & b)
#define vandq_s32(a, b) __builtin_mpl_vector_and_v4i32(a, b) // (a & b)
#define vdupq_n_s32(value) __builtin_mpl_vector_from_scalar_v4i32(value)
#define vdupq_n_u16(value) __builtin_mpl_vector_from_scalar_v8u16(value)
#define vdupq_n_u8(value) __builtin_mpl_vector_from_scalar_v16u8(value)
#define vdup_n_u32(value) __builtin_mpl_vector_from_scalar_v2u32(value)
#define vceqq_u16(a, b) __builtin_mpl_vector_eq_v8u16(a, b)  // (a == b)
#define veorq_u32(a, b) __builtin_mpl_vector_xor_v4u32(a, b) // (a ^ b)
#define veorq_u64(a, b) __builtin_mpl_vector_xor_v2u64(a, b) // (a ^ b)
#define vextq_u8(a, b, n) __builtin_mpl_vector_merge_v16u8(a, b, n)
#define vextq_u16(a, b, n) __builtin_mpl_vector_merge_v8u16(a, b, n)
#define vget_high_u64(a) __builtin_mpl_vector_get_high_v2u64(a)
#define vget_low_u64(a) __builtin_mpl_vector_get_low_v2u64(a)
#define vget_lane_u32(vec, lane)                                               \
  __builtin_mpl_vector_get_element_v2u32(vec, lane)
#define vgetq_lane_u32(vec, lane)                                              \
  __builtin_mpl_vector_get_element_v4u32(vec, lane)
#define vgetq_lane_u16(vec, lane)                                              \
  __builtin_mpl_vector_get_element_v8u16(vec, lane)
#define vld1q_u8(ptr) __builtin_mpl_vector_load_v16u8(ptr)
#define vld1q_u16(ptr) __builtin_mpl_vector_load_v8u16(ptr)
#define vld1q_s32(ptr) __builtin_mpl_vector_load_v4i32(ptr)
#define vld1q_u32(ptr) __builtin_mpl_vector_load_v4u32(ptr)
#define vmlal_u32(accum, s1, s2) __builtin_mpl_vector_madd_v2u32(accum, s1, s2)
#define vmull_u32(a, b) __builtin_mpl_vector_mul_v2u32(a, b)
#define vpaddlq_u16(a) __builtin_mpl_vector_pairwise_add_v8u16(a)
#define vpaddlq_u32(a) __builtin_mpl_vector_pairwise_add_v4u32(a)
#define vqtbl1q_u8(t, idx) __builtin_mpl_vector_table_lookup_v16u8(t, idx)
#define vreinterpretq_u64_u8(a) ((uint64x2_t)a)
#define vreinterpretq_u32_u64(a) ((uint32x4_t)a)
#define vrev32q_u8(vec) __builtin_mpl_vector_reverse_v16u8(vec)
#define vsetq_lane_u32(value, vec, lane)                                       \
  __builtin_mpl_vector_set_element_v4u32(value, vec, lane)
#define vsetq_lane_u16(value, vec, lane)                                       \
  __builtin_mpl_vector_set_element_v8u16(value, vec, lane)
#define vshlq_u16(a, b) __builtin_mpl_vector_shl_v8u16(a, b)   // (a << b)
#define vshlq_n_u64(a, n) __builtin_mpl_vector_shli_v2u64(a, n) // (a << n)
#define vshrq_n_u64(a, n) __builtin_mpl_vector_shri_v2u64(a, n) // (a >> n)
#define vst1q_s32(ptr, val) __builtin_mpl_vector_store_v4i32(ptr, val)
#define vst1q_u8(ptr, val) __builtin_mpl_vector_store_v16u8(ptr, val)

#endif /* __ARM_NEON_H */
