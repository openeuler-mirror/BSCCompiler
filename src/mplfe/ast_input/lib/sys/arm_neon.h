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
int32_t __builtin_mpl_vector_sum_v2i32(int32x2_t);
int16_t __builtin_mpl_vector_sum_v4i16(int16x4_t);
int8_t __builtin_mpl_vector_sum_v8i8(int8x8_t);
uint32_t __builtin_mpl_vector_sum_v2u32(uint32x2_t);
uint16_t __builtin_mpl_vector_sum_v4u16(uint16x4_t);
uint8_t __builtin_mpl_vector_sum_v8u8(uint8x8_t);
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

// *************************
// Supported Neon Intrinsics
// *************************

// vaddv
#define vaddv_s8(a)  __builtin_mpl_vector_sum_v8i8(a)
#define vaddv_s16(a) __builtin_mpl_vector_sum_v4i16(a)
#define vaddv_s32(a) __builtin_mpl_vector_sum_v2i32(a)
#define vaddv_u8 (a) __builtin_mpl_vector_sum_v8u8(a)
#define vaddv_u16(a) __builtin_mpl_vector_sum_v4u16(a)
#define vaddv_u32(a) __builtin_mpl_vector_sum_v2u32(a)
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
#define vget_lane_s8(a, n)  __builtin_mpl_vector_get_element_v8i8(a, n)
#define vget_lane_s16(a, n) __builtin_mpl_vector_get_element_v4i16(a, n)
#define vget_lane_s32(a, n) __builtin_mpl_vector_get_element_v2i32(a, n)
#define vget_lane_s64(a, n) __builtin_mpl_vector_get_element_v1i64(a, n)
#define vget_lane_u8(a, n) __builtin_mpl_vector_get_element_v8u8(a, n)
#define vget_lane_u16(a, n) __builtin_mpl_vector_get_element_v4u16(a, n)
#define vget_lane_u32(a, n) __builtin_mpl_vector_get_element_v2u32(a, n)
#define vget_lane_u64(a, n) __builtin_mpl_vector_get_element_v1u64(a, n)
#define vget_lane_f16(a, n) __builtin_mpl_vector_get_element_v4f16(a, n)
#define vget_lane_f32(a, n) __builtin_mpl_vector_get_element_v2f32(a, n)
#define vget_lane_f64(a, n) __builtin_mpl_vector_get_element_v1f64(a, n)
#define vgetq_lane_s8(a, n) __builtin_mpl_vector_get_element_v16i8(a, n)
#define vgetq_lane_s16(a, n) __builtin_mpl_vector_get_element_v8i16(a, n)
#define vgetq_lane_s32(a, n) __builtin_mpl_vector_get_element_v4i32(a, n)
#define vgetq_lane_s64(a, n) __builtin_mpl_vector_get_element_v2i64(a, n)
#define vgetq_lane_u8(a, n) __builtin_mpl_vector_get_element_v16u8(a, n)
#define vgetq_lane_u16(a, n) __builtin_mpl_vector_get_element_v8u16(a, n)
#define vgetq_lane_u32(a, n) __builtin_mpl_vector_get_element_v4u32(a, n)
#define vgetq_lane_u64(a, n) __builtin_mpl_vector_get_element_v2u64(a, n)
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
#define vld1_s8(a)  __builtin_mpl_vector_load_v8i8(a)
#define vld1_s16(a) __builtin_mpl_vector_load_v4i16(a)
#define vld1_s32(a) __builtin_mpl_vector_load_v2i32(a)
#define vld1_s64(a) __builtin_mpl_vector_load_v1i64(a)
#define vld1_u8(a) __builtin_mpl_vector_load_v8u8(a)
#define vld1_u16(a) __builtin_mpl_vector_load_v4u16(a)
#define vld1_u32(a) __builtin_mpl_vector_load_v2u32(a)
#define vld1_u64(a) __builtin_mpl_vector_load_v1u64(a)
#define vld1_f16(a) __builtin_mpl_vector_load_v4f16(a)
#define vld1_f32(a) __builtin_mpl_vector_load_v2f32(a)
#define vld1_f64(a) __builtin_mpl_vector_load_v1f64(a)
#define vld1q_s8(a) __builtin_mpl_vector_load_v16i8(a)
#define vld1q_s16(a) __builtin_mpl_vector_load_v8i16(a)
#define vld1q_s32(a) __builtin_mpl_vector_load_v4i32(a)
#define vld1q_s64(a) __builtin_mpl_vector_load_v2i64(a)
#define vld1q_u8(a) __builtin_mpl_vector_load_v16u8(a)
#define vld1q_u16(a) __builtin_mpl_vector_load_v8u16(a)
#define vld1q_u32(a) __builtin_mpl_vector_load_v4u32(a)
#define vld1q_u64(a) __builtin_mpl_vector_load_v2u64(a)
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

// vmull
#define vmull_s8(a, b)  __builtin_mpl_vector_mul_v8i8(a, b)
#define vmull_s16(a, b) __builtin_mpl_vector_mul_v4i16(a, b)
#define vmull_s32(a, b) __builtin_mpl_vector_mul_v2i32(a, b)
#define vmull_u8(a, b) __builtin_mpl_vector_mul_v8u8(a, b)
#define vmull_u16(a, b) __builtin_mpl_vector_mul_v4u16(a, b)
#define vmull_u32(a, b) __builtin_mpl_vector_mul_v2u32(a, b)

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

// vqtbl1
#define vqtbl1_s8(a, b) __builtin_mpl_vector_table_lookup_v8i8(a, b)
#define vqtbl1_u8(a, b) __builtin_mpl_vector_table_lookup_v8u8(a, b)
#define vqtbl1q_s8(a, b) __builtin_mpl_vector_table_lookup_v16i8(a, b)
#define vqtbl1q_u8(a, b) __builtin_mpl_vector_table_lookup_v16u8(a, b)

// vreinterpret 8
#define vreinterpret_s16_s8(a) ((int16x4_t)a)
#define vreinterpret_s32_s8(a) ((int32x2_t)a)
#define vreinterpret_s64_s8(a) ((int64x1_t)a)
#define vreinterpret_u16_u8(a) ((uint16x4_t)a)
#define vreinterpret_u32_u8(a) ((uint32x2_t)a)
#define vreinterpret_u64_u8(a) ((uint64x1_t)a)
#define vreinterpret_f16_s8(a) ((float16x4_t)a)
#define vreinterpret_f32_s8(a) ((float32x2_t)a)
#define vreinterpret_f64_s8(a) ((float64x1_t)a)
#define vreinterpret_f16_u8(a) ((float16x4_t)a)
#define vreinterpret_f32_u8(a) ((float32x2_t)a)
#define vreinterpret_f64_u8(a) ((float64x1_t)a)
#define vreinterpretq_s16_s8(a) ((int16x8_t)a)
#define vreinterpretq_s32_s8(a) ((int32x4_t)a)
#define vreinterpretq_s64_s8(a) ((int64x2_t)a)
#define vreinterpretq_u16_u8(a) ((uint16x8_t)a)
#define vreinterpretq_u32_u8(a) ((uint32x4_t)a)
#define vreinterpretq_u64_u8(a) ((uint64x2_t)a)
#define vreinterpretq_f16_s8(a) ((float16x8_t)a)
#define vreinterpretq_f32_s8(a) ((float32x4_t)a)
#define vreinterpretq_f64_s8(a) ((float64x2_t)a)
#define vreinterpretq_f16_u8(a) ((float16x8_t)a)
#define vreinterpretq_f32_u8(a) ((float32x4_t)a)
#define vreinterpretq_f64_u8(a) ((float64x2_t)a)

// vreinterpret 16
#define vreinterpret_s8_s16(a) ((int8x8_t)a)
#define vreinterpret_s32_s16(a) ((int32x2_t)a)
#define vreinterpret_s64_s16(a) ((int64x1_t)a)
#define vreinterpret_u8_u16(a) ((uint8x8_t)a)
#define vreinterpret_u32_u16(a) ((uint32x2_t)a)
#define vreinterpret_u64_u16(a) ((uint64x1_t)a)
#define vreinterpret_f16_s16(a) ((float16x4_t)a)
#define vreinterpret_f32_s16(a) ((float32x2_t)a)
#define vreinterpret_f64_s16(a) ((float64x1_t)a)
#define vreinterpret_f16_u16(a) ((float16x4_t)a)
#define vreinterpret_f32_u16(a) ((float32x2_t)a)
#define vreinterpret_f64_u16(a) ((float64x1_t)a)
#define vreinterpretq_s8_s16(a) ((int16x8_t)a)
#define vreinterpretq_s32_s16(a) ((int32x4_t)a)
#define vreinterpretq_s64_s16(a) ((int64x2_t)a)
#define vreinterpretq_u8_u16(a) ((uint16x8_t)a)
#define vreinterpretq_u32_u16(a) ((uint32x4_t)a)
#define vreinterpretq_u64_u16(a) ((uint64x2_t)a)
#define vreinterpretq_f16_s16(a) ((float16x8_t)a)
#define vreinterpretq_f32_s16(a) ((float32x4_t)a)
#define vreinterpretq_f64_s16(a) ((float64x2_t)a)
#define vreinterpretq_f16_u16(a) ((float16x8_t)a)
#define vreinterpretq_f32_u16(a) ((float32x4_t)a)
#define vreinterpretq_f64_u16(a) ((float64x2_t)a)

// vreinterpret 32
#define vreinterpret_s8_s32(a) ((int8x8_t)a)
#define vreinterpret_s16_s32(a) ((int16x4_t)a)
#define vreinterpret_s64_s32(a) ((int64x1_t)a)
#define vreinterpret_u8_u32(a) ((uint8x8_t)a)
#define vreinterpret_u16_u32(a) ((uint16x4_t)a)
#define vreinterpret_u64_u32(a) ((uint64x1_t)a)
#define vreinterpret_f16_s32(a) ((float16x4_t)a)
#define vreinterpret_f32_s32(a) ((float32x2_t)a)
#define vreinterpret_f64_s32(a) ((float64x1_t)a)
#define vreinterpret_f16_u32(a) ((float16x4_t)a)
#define vreinterpret_f32_u32(a) ((float32x2_t)a)
#define vreinterpret_f64_u32(a) ((float64x1_t)a)
#define vreinterpretq_s8_s32(a) ((int16x8_t)a)
#define vreinterpretq_s16_s32(a) ((int16x8_t)a)
#define vreinterpretq_s64_s32(a) ((int64x2_t)a)
#define vreinterpretq_u8_u32(a) ((uint16x8_t)a)
#define vreinterpretq_u16_u32(a) ((uint16x8_t)a)
#define vreinterpretq_u64_u32(a) ((uint64x2_t)a)
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
#define vreinterpret_u8_u64(a) ((uint8x8_t)a)
#define vreinterpret_u16_u64(a) ((uint16x4_t)a)
#define vreinterpret_u32_u64(a) ((uint32x2_t)a)
#define vreinterpret_f16_s64(a) ((float16x4_t)a)
#define vreinterpret_f32_s64(a) ((float32x2_t)a)
#define vreinterpret_f64_s64(a) ((float64x1_t)a)
#define vreinterpret_f16_u64(a) ((float16x4_t)a)
#define vreinterpret_f32_u64(a) ((float32x2_t)a)
#define vreinterpret_f64_u64(a) ((float64x1_t)a)
#define vreinterpretq_s8_s64(a) ((int8x16_t)a)
#define vreinterpretq_s16_s64(a) ((int16x8_t)a)
#define vreinterpretq_s32_s64(a) ((int32x4_t)a)
#define vreinterpretq_u8_u64(a) ((uint8x16_t)a)
#define vreinterpretq_u16_u64(a) ((uint16x8_t)a)
#define vreinterpretq_u32_u64(a) ((uint32x4_t)a)
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

// vshl_n
#define vshl_n_s8(a, n) (a << n)
#define vshl_n_s16(a, n) (a << n)
#define vshl_n_s32(a, n) (a << n)
#define vshl_n_s64(a, n) (a << n)
#define vshl_n_u8(a, n) (a << n)
#define vshl_n_u16(a, n) (a << n)
#define vshl_n_u32(a, n) (a << n)
#define vshl_n_u64(a, n) (a << n)
#define vshlq_n_s8(a, n) (a << n)
#define vshlq_n_s16(a, n) (a << n)
#define vshlq_n_s32(a, n) (a << n)
#define vshlq_n_s64(a, n) (a << n)
#define vshlq_n_u8(a, n) (a << n)
#define vshlq_n_u16(a, n) (a << n)
#define vshlq_n_u32(a, n) (a << n)
#define vshlq_n_u64(a, n) (a << n)

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

// vshr_n
#define vshr_n_s8(a, n) (a >> n)
#define vshr_n_s16(a, n) (a >> n)
#define vshr_n_s32(a, n) (a >> n)
#define vshr_n_s64(a, n) (a >> n)
#define vshr_n_u8(a, n) (a >> n)
#define vshr_n_u16(a, n) (a >> n)
#define vshr_n_u32(a, n) (a >> n)
#define vshr_n_u64(a, n) (a >> n)
#define vshrq_n_s8(a, n) (a >> n)
#define vshrq_n_s16(a, n) (a >> n)
#define vshrq_n_s32(a, n) (a >> n)
#define vshrq_n_s64(a, n) (a >> n)
#define vshrq_n_u8(a, n) (a >> n)
#define vshrq_n_u16(a, n) (a >> n)
#define vshrq_n_u32(a, n) (a >> n)
#define vshrq_n_u64(a, n) (a >> n)

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

#endif /* __ARM_NEON_H */
