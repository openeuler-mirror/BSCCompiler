#include "neon.h"

int main() {
  // CHECK: #inlining begin: FUNC vsetq_lane_s64
  // CHECK: dassign %{{.*}} 0 (intrinsicop v2i64 vector_set_element_v2i64 (dread i64 %{{.*}}, dread v2i64 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vsetq_lane_s64
  vsetq_lane_s64(set_int64_t(), set_int64x2_t(), 1);

  // CHECK: #inlining begin: FUNC vqdmlals_lane_s32
  // CHECK: #inlining begin: FUNC vqdmlal_lane_s32
  // CHECK: dassign %{{.*}} 0 (intrinsicop v2i64 vector_qdmlal_lane_v2i64 (dread v2i64 %{{.*}}, dread v2i32 %{{.*}}, dread v2i32 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqdmlal_lane_s32
  // CHECK-NEXT: #inlining begin: FUNC vgetq_lane_s64
  // CHECK: dassign %{{.*}} 0 (intrinsicop i64 vector_getq_lane_v2i64 (dread v2i64 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vgetq_lane_s64
  // CHECK: dassign %{{.*}} 0 (dread i64 %_{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqdmlals_lane_s32
  vqdmlals_lane_s32(set_int64_t(), set_int32_t(), set_int32x2_t(), 1);

  // CHECK: #inlining begin: FUNC vqdmlalh_laneq_s16
  // CHECK: #inlining begin: FUNC vqdmlal_laneq_s16
  // CHECK: dassign %{{.*}} 0 (intrinsicop v4i32 vector_qdmlal_laneq_v4i32 (dread v4i32 %{{.*}}, dread v4i16 %{{.*}}, dread v8i16 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqdmlal_laneq_s16
  // CHECK-NEXT: #inlining begin: FUNC vgetq_lane_s32
  // CHECK: dassign %{{.*}} 0 (intrinsicop i32 vector_getq_lane_v4i32 (dread v4i32 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vgetq_lane_s32
  // CHECK: dassign %{{.*}} 0 (dread i32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqdmlalh_laneq_s16
  vqdmlalh_laneq_s16(set_int32_t(), set_int16_t(), set_int16x8_t(), 1);

  // CHECK: #inlining begin: FUNC vqdmlals_laneq_s32
  // CHECK: #inlining begin: FUNC vqdmlal_laneq_s32
  // CHECK: dassign %{{.*}} 0 (intrinsicop v2i64 vector_qdmlal_laneq_v2i64 (dread v2i64 %{{.*}}, dread v2i32 %{{.*}}, dread v4i32 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqdmlal_laneq_s32
  // CHECK-NEXT: #inlining begin: FUNC vgetq_lane_s64
  // CHECK: dassign %{{.*}} 0 (intrinsicop i64 vector_getq_lane_v2i64 (dread v2i64 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vgetq_lane_s64
  // CHECK: dassign %{{.*}} 0 (dread i64 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqdmlals_laneq_s32
  vqdmlals_laneq_s32(set_int64_t(), set_int32_t(), set_int32x4_t(), 1);

  // CHECK: #inlining begin: FUNC vqdmlslh_lane_s16
  // CHECK: #inlining begin: FUNC vqdmlsl_lane_s16
  // CHECK: dassign %{{.*}} 0 (intrinsicop v4i32 vector_qdmlsl_lane_v4i32 (dread v4i32 %{{.*}}, dread v4i16 %{{.*}}, dread v4i16 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqdmlsl_lane_s16
  // CHECK-NEXT: #inlining begin: FUNC vgetq_lane_s32
  // CHECK: dassign %{{.*}} 0 (intrinsicop i32 vector_getq_lane_v4i32 (dread v4i32 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vgetq_lane_s32
  // CHECK: dassign %{{.*}} 0 (dread i32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqdmlslh_lane_s16
  vqdmlslh_lane_s16(set_int32_t(), set_int16_t(), set_int16x4_t(), 1);

  // CHECK: #inlining begin: FUNC vqdmlsls_lane_s32
  // CHECK: #inlining begin: FUNC vqdmlsl_lane_s32
  // CHECK: dassign %{{.*}} 0 (intrinsicop v2i64 vector_qdmlsl_lane_v2i64 (dread v2i64 %{{.*}}, dread v2i32 %{{.*}}, dread v2i32 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqdmlsl_lane_s32
  // CHECK-NEXT: #inlining begin: FUNC vgetq_lane_s64
  // CHECK: dassign %{{.*}} 0 (intrinsicop i64 vector_getq_lane_v2i64 (dread v2i64 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vgetq_lane_s64
  // CHECK: dassign %{{.*}} 0 (dread i64 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqdmlsls_lane_s32
  vqdmlsls_lane_s32(set_int64_t(), set_int32_t(), set_int32x2_t(), 1);

  // CHECK: #inlining begin: FUNC vqdmlslh_laneq_s16
  // CHECK: #inlining begin: FUNC vqdmlsl_laneq_s16
  // CHECK: dassign %{{.*}}_0 0 (intrinsicop v4i32 vector_qdmlsl_laneq_v4i32 (dread v4i32 %{{.*}}, dread v4i16 %{{.*}}, dread v8i16 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqdmlsl_laneq_s16
  // CHECK-NEXT: #inlining begin: FUNC vgetq_lane_s32
  // CHECK: dassign %{{.*}}_0 0 (intrinsicop i32 vector_getq_lane_v4i32 (dread v4i32 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vgetq_lane_s32
  // CHECK: dassign %{{.*}} 0 (dread i32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqdmlslh_laneq_s16
  vqdmlslh_laneq_s16(set_int32_t(), set_int16_t(), set_int16x8_t(), 1);

  // CHECK: #inlining begin: FUNC vqdmlsls_laneq_s32
  // CHECK: #inlining begin: FUNC vqdmlsl_laneq_s32
  // CHECK: dassign %{{.*}} 0 (intrinsicop v2i64 vector_qdmlsl_laneq_v2i64 (dread v2i64 %{{.*}}, dread v2i32 %{{.*}}, dread v4i32 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqdmlsl_laneq_s32
  // CHECK-NEXT: #inlining begin: FUNC vgetq_lane_s64
  // CHECK: dassign %{{.*}} 0 (intrinsicop i64 vector_getq_lane_v2i64 (dread v2i64 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vgetq_lane_s64
  // CHECK: dassign %{{.*}} 0 (dread i64 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqdmlsls_laneq_s32
  vqdmlsls_laneq_s32(set_int64_t(), set_int32_t(), set_int32x4_t(), 1);

  // CHECK: #inlining begin: FUNC vqdmullh_lane_s16
  // CHECK: #inlining begin: FUNC vqdmull_lane_s16
  // CHECK: dassign %{{.*}} 0 (intrinsicop v4i32 vector_qdmull_lane_v4i32 (dread v4i16 %{{.*}}, dread v4i16 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqdmull_lane_s16
  // CHECK-NEXT: #inlining begin: FUNC vgetq_lane_s32
  // CHECK: dassign %{{.*}} 0 (intrinsicop i32 vector_getq_lane_v4i32 (dread v4i32 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vgetq_lane_s32
  // CHECK: dassign %{{.*}} 0 (dread i32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqdmullh_lane_s16
  vqdmullh_lane_s16(set_int16_t(), set_int16x4_t(), 1);

  // CHECK: #inlining begin: FUNC vqdmulls_lane_s32
  // CHECK: #inlining begin: FUNC vqdmull_lane_s32
  // CHECK: dassign %{{.*}} 0 (intrinsicop v2i64 vector_qdmull_lane_v2i64 (dread v2i32 %{{.*}}, dread v2i32 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqdmull_lane_s32
  // CHECK-NEXT: #inlining begin: FUNC vgetq_lane_s64
  // CHECK: dassign %{{.*}} 0 (intrinsicop i64 vector_getq_lane_v2i64 (dread v2i64 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vgetq_lane_s64
  // CHECK: dassign %{{.*}} 0 (dread i64 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqdmulls_lane_s32
  vqdmulls_lane_s32(set_int32_t(), set_int32x2_t(), 1);

  // CHECK: #inlining begin: FUNC vqdmullh_laneq_s16
  // CHECK: #inlining begin: FUNC vqdmull_laneq_s16
  // CHECK: dassign %{{.*}} 0 (intrinsicop v4i32 vector_qdmull_laneq_v4i32 (dread v4i16 %{{.*}}, dread v8i16 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqdmull_laneq_s16
  // CHECK-NEXT: #inlining begin: FUNC vgetq_lane_s32
  // CHECK: dassign %{{.*}} 0 (intrinsicop i32 vector_getq_lane_v4i32 (dread v4i32 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vgetq_lane_s32
  // CHECK: dassign %{{.*}} 0 (dread i32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqdmullh_laneq_s16
  vqdmullh_laneq_s16(set_int16_t(), set_int16x8_t(), 1);

  // CHECK: #inlining begin: FUNC vqdmulls_laneq_s32
  // CHECK: #inlining begin: FUNC vqdmull_laneq_s32
  // CHECK: dassign %{{.*}} 0 (intrinsicop v2i64 vector_qdmull_laneq_v2i64 (dread v2i32 %{{.*}}, dread v4i32 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqdmull_laneq_s32
  // CHECK-NEXT: #inlining begin: FUNC vgetq_lane_s64
  // CHECK: dassign %{{.*}} 0 (intrinsicop i64 vector_getq_lane_v2i64 (dread v2i64 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vgetq_lane_s64
  // CHECK: dassign %{{.*}} 0 (dread i64 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqdmulls_laneq_s32
  vqdmulls_laneq_s32(set_int32_t(), set_int32x4_t(), 1);

  // CHECK: #inlining begin: FUNC vqdmulhh_lane_s16
  // CHECK: #inlining begin: FUNC vqdmulh_lane_s16
  // CHECK: dassign %{{.*}} 0 (intrinsicop v4i16 vector_qdmulh_lane_v4i16 (dread v4i16 %{{.*}}, dread v4i16 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqdmulh_lane_s16
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_s16
  // CHECK: dassign %{{.*}} 0 (intrinsicop i16 vector_get_lane_v4i16 (dread v4i16 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_s16
  // CHECK: dassign %{{.*}} 0 (dread i32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqdmulhh_lane_s16
  vqdmulhh_lane_s16(set_int16_t(), set_int16x4_t(), 1);

  // CHECK: #inlining begin: FUNC vqdmulhs_lane_s32
  // CHECK: #inlining begin: FUNC vqdmulh_lane_s32
  // CHECK: dassign %{{.*}} 0 (intrinsicop v2i32 vector_qdmulh_lane_v2i32 (dread v2i32 %{{.*}}, dread v2i32 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqdmulh_lane_s32
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_s32
  // CHECK: dassign %{{.*}} 0 (intrinsicop i32 vector_get_lane_v2i32 (dread v2i32 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_s32
  // CHECK: dassign %{{.*}} 0 (dread i32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqdmulhs_lane_s32
  vqdmulhs_lane_s32(set_int32_t(), set_int32x2_t(), 1);

  // CHECK: #inlining begin: FUNC vqdmulhh_laneq_s16
  // CHECK: #inlining begin: FUNC vqdmulh_laneq_s16
  // CHECK: dassign %{{.*}} 0 (intrinsicop v4i16 vector_qdmulh_laneq_v4i16 (dread v4i16 %{{.*}}, dread v8i16 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqdmulh_laneq_s16
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_s16
  // CHECK: dassign %{{.*}} 0 (intrinsicop i16 vector_get_lane_v4i16 (dread v4i16 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_s16
  // CHECK: dassign %{{.*}} 0 (dread i32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqdmulhh_laneq_s16
  vqdmulhh_laneq_s16(set_int16_t(), set_int16x8_t(), 1);

  // CHECK: #inlining begin: FUNC vqdmulhs_laneq_s32
  // CHECK: #inlining begin: FUNC vqrdmulh_lane_s16
  // CHECK: dassign %{{.*}} 0 (intrinsicop v4i16 vector_qrdmulh_lane_v4i16 (dread v4i16 %{{.*}}, dread v4i16 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqrdmulh_lane_s16
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_s16
  // CHECK: dassign %{{.*}} 0 (intrinsicop i16 vector_get_lane_v4i16 (dread v4i16 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_s16
  // CHECK: dassign %{{.*}} 0 (dread i32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqrdmulhh_lane_s16
  vqdmulhs_laneq_s32(set_int32_t(), set_int32x4_t(), 1);
  vqrdmulhh_lane_s16(set_int16_t(), set_int16x4_t(), 1);

  // CHECK: #inlining begin: FUNC vqrdmulhs_lane_s32
  // CHECK: #inlining begin: FUNC vqrdmulh_lane_s32
  // CHECK: dassign %{{.*}} 0 (intrinsicop v2i32 vector_qrdmulh_lane_v2i32 (dread v2i32 %{{.*}}, dread v2i32 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqrdmulh_lane_s32
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_s32
  // CHECK: dassign %{{.*}} 0 (intrinsicop i32 vector_get_lane_v2i32 (dread v2i32 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_s32
  // CHECK: dassign %{{.*}} 0 (dread i32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqrdmulhs_lane_s32
  vqrdmulhs_lane_s32(set_int32_t(), set_int32x2_t(), 1);

  // CHECK: #inlining begin: FUNC vqrdmulhh_laneq_s16
  // CHECK: #inlining begin: FUNC vqrdmulh_laneq_s16
  // CHECK: dassign %{{.*}} 0 (intrinsicop v4i16 vector_qrdmulh_laneq_v4i16 (dread v4i16 %{{.*}}, dread v8i16 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqrdmulh_laneq_s16
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_s16
  // CHECK: dassign %{{.*}} 0 (intrinsicop i16 vector_get_lane_v4i16 (dread v4i16 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_s16
  // CHECK: dassign %{{.*}} 0 (dread i32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqrdmulhh_laneq_s16 
  vqrdmulhh_laneq_s16(set_int16_t(), set_int16x8_t(), 1);

  // CHECK: #inlining begin: FUNC vqrdmulhs_laneq_s32
  // CHECK: #inlining begin: FUNC vqrdmulh_laneq_s32
  // CHECK: dassign %{{.*}} 0 (intrinsicop v2i32 vector_qrdmulh_laneq_v2i32 (dread v2i32 %{{.*}}, dread v4i32 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqrdmulh_laneq_s32
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_s32
  // CHECK: dassign %{{.*}} 0 (intrinsicop i32 vector_get_lane_v2i32 (dread v2i32 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_s32
  // CHECK: dassign %{{.*}} 0 (dread i32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqrdmulhs_laneq_s32
  vqrdmulhs_laneq_s32(set_int32_t(), set_int32x4_t(), 1);

  // CHECK: #inlining begin: FUNC vqshlb_n_s8
  // CHECK: #inlining begin: FUNC vqshl_n_s8
  // CHECK: dassign %{{.*}} 0 (intrinsicop v8i8 vector_qshl_n_v8i8 (dread v8i8 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqshl_n_s8
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_s64
  // CHECK: dassign %{{.*}} 0 (intrinsicop i64 vector_get_lane_v1i64 (dread v1i64 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_s64
  // CHECK: dassign %{{.*}} 0 (sext i32 8 (dread i64 %{{.*}}))
  // CHECK-NEXT: #inlining end: FUNC vqshlb_n_s8
  vqshlb_n_s8(set_int8_t(), 1);

  // CHECK: #inlining begin: FUNC vqshlh_n_s16
  // CHECK: #inlining begin: FUNC vqshl_n_s16
  // CHECK: dassign %{{.*}} 0 (intrinsicop v4i16 vector_qshl_n_v4i16 (dread v4i16 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqshl_n_s16
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_s64
  // CHECK: dassign %{{.*}} 0 (intrinsicop i64 vector_get_lane_v1i64 (dread v1i64 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_s64
  // CHECK: dassign %{{.*}} 0 (sext i32 16 (dread i64 %{{.*}}))
  // CHECK-NEXT: #inlining end: FUNC vqshlh_n_s16
  vqshlh_n_s16(set_int16_t(), 1);

  // CHECK: #inlining begin: FUNC vqshls_n_s32
  // CHECK: #inlining begin: FUNC vqshl_n_s32
  // CHECK: dassign %{{.*}} 0 (intrinsicop v2i32 vector_qshl_n_v2i32 (dread v2i32 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqshl_n_s32
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_s64
  // CHECK: dassign %{{.*}} 0 (intrinsicop i64 vector_get_lane_v1i64 (dread v1i64 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_s64
  // CHECK: dassign %{{.*}} 0 (cvt i32 i64 (dread i64 %{{.*}}))
  // CHECK-NEXT: #inlining end: FUNC vqshls_n_s32
  vqshls_n_s32(set_int32_t(), 1);

  // CHECK: #inlining begin: FUNC vqshld_n_s64
  // CHECK: #inlining begin: FUNC vqshl_n_s64
  // CHECK: dassign %{{.*}} 0 (intrinsicop v1i64 vector_qshl_n_v1i64 (dread v1i64 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqshl_n_s64
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_s64
  // CHECK: dassign %{{.*}} 0 (intrinsicop i64 vector_get_lane_v1i64 (dread v1i64 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_s64
  // CHECK: dassign %{{.*}} 0 (dread i64 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqshld_n_s64
  vqshld_n_s64(set_int64_t(), 1);

  // CHECK: #inlining begin: FUNC vqshlb_n_u8
  // CHECK: #inlining begin: FUNC vqshl_n_u8
  // CHECK: dassign %{{.*}} 0 (intrinsicop v8u8 vector_qshl_n_v8u8 (dread v8u8 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqshl_n_u8
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_u64
  // CHECK: dassign %{{.*}} 0 (intrinsicop u64 vector_get_lane_v1u64 (dread v1u64 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_u64
  // CHECK: dassign %{{.*}} 0 (zext u32 8 (dread u64 %{{.*}}))
  // CHECK-NEXT: #inlining end: FUNC vqshlb_n_u8
  vqshlb_n_u8(set_uint8_t(), 1);

  // CHECK: #inlining begin: FUNC vqshlh_n_u16
  // CHECK: #inlining begin: FUNC vqshl_n_u16
  // CHECK: dassign %{{.*}} 0 (intrinsicop v4u16 vector_qshl_n_v4u16 (dread v4u16 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqshl_n_u16
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_u64
  // CHECK: dassign %{{.*}} 0 (intrinsicop u64 vector_get_lane_v1u64 (dread v1u64 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_u64
  // CHECK: dassign %{{.*}} 0 (zext u32 16 (dread u64 %{{.*}}))
  // CHECK-NEXT: #inlining end: FUNC vqshlh_n_u16
  vqshlh_n_u16(set_uint16_t(), 1);

  // CHECK: #inlining begin: FUNC vqshls_n_u32
  // CHECK: #inlining begin: FUNC vqshl_n_u32
  // CHECK: dassign %{{.*}} 0 (intrinsicop v2u32 vector_qshl_n_v2u32 (dread v2u32 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqshl_n_u32
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_u64
  // CHECK: dassign %{{.*}} 0 (intrinsicop u64 vector_get_lane_v1u64 (dread v1u64 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_u64
  // CHECK: dassign %{{.*}} 0 (cvt u32 u64 (dread u64 %{{.*}}))
  // CHECK-NEXT: #inlining end: FUNC vqshls_n_u32
  vqshls_n_u32(set_uint32_t(), 1);

  // CHECK: #inlining begin: FUNC vqshld_n_u64
  // CHECK: #inlining begin: FUNC vqshl_n_u64
  // CHECK: dassign %{{.*}} 0 (intrinsicop v1u64 vector_qshl_n_v1u64 (dread v1u64 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqshl_n_u64
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_u64
  // CHECK: dassign %{{.*}} 0 (intrinsicop u64 vector_get_lane_v1u64 (dread v1u64 %{{.*}}, constval i32 0)
  // CHECK-NEXT: #inlining end: FUNC vget_lane_u64
  // CHECK: dassign %{{.*}} 0 (dread u64 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqshld_n_u64
  vqshld_n_u64(set_uint64_t(), 1);

  // CHECK: #inlining begin: FUNC vqshlub_n_s8
  // CHECK: #inlining begin: FUNC vqshlu_n_s8
  // CHECK: dassign %{{.*}} 0 (intrinsicop v8u8 vector_qshlu_n_v8u8 (dread v8i8 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqshlu_n_s8
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_u64
  // CHECK: dassign %{{.*}} 0 (intrinsicop u64 vector_get_lane_v1u64 (dread v1u64 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_u64
  // CHECK: dassign %{{.*}} 0 (zext u32 8 (dread u64 %{{.*}}))
  // CHECK-NEXT: #inlining end: FUNC vqshlub_n_s8
  vqshlub_n_s8(set_int8_t(), 1);

  // CHECK: #inlining begin: FUNC vqshluh_n_s16
  // CHECK: #inlining begin: FUNC vqshlu_n_s16
  // CHECK: dassign %{{.*}} 0 (intrinsicop v4u16 vector_qshlu_n_v4u16 (dread v4i16 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqshlu_n_s16
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_u64
  // CHECK: dassign %{{.*}} 0 (intrinsicop u64 vector_get_lane_v1u64 (dread v1u64 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_u64
  // CHECK: dassign %{{.*}} 0 (zext u32 16 (dread u64 %{{.*}}))
  // CHECK-NEXT: #inlining end: FUNC vqshluh_n_s16
  vqshluh_n_s16(set_int16_t(), 1);

  // CHECK: #inlining begin: FUNC vqshlus_n_s32
  // CHECK: #inlining begin: FUNC vqshlu_n_s32
  // CHECK: dassign %{{.*}} 0 (intrinsicop v2u32 vector_qshlu_n_v2u32 (dread v2i32 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqshlu_n_s32
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_u64
  // CHECK: dassign %{{.*}} 0 (intrinsicop u64 vector_get_lane_v1u64 (dread v1u64 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_u64
  // CHECK: dassign %{{.*}} 0 (cvt u32 u64 (dread u64 %{{.*}}))
  // CHECK-NEXT: #inlining end: FUNC vqshlus_n_s32
  vqshlus_n_s32(set_int32_t(), 1);

  // CHECK: #inlining begin: FUNC vqshlud_n_s64
  // CHECK: #inlining begin: FUNC vqshlu_n_s64
  // CHECK: dassign %{{.*}} 0 (intrinsicop v1u64 vector_qshlu_n_v1u64 (dread v1i64 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqshlu_n_s64
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_u64
  // CHECK: dassign %{{.*}} 0 (intrinsicop u64 vector_get_lane_v1u64 (dread v1u64 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_u64
  // CHECK: dassign %{{.*}} 0 (dread u64 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqshlud_n_s64
  vqshlud_n_s64(set_int64_t(), 1);

  // CHECK: #inlining begin: FUNC vslid_n_s64
  // CHECK: #inlining begin: FUNC vsli_n_s64
  // CHECK: dassign %{{.*}} 0 (intrinsicop v1i64 vector_sli_n_v1i64 (dread v1i64 %{{.*}}, dread v1i64 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vsli_n_s64
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_s64
  // CHECK: dassign %{{.*}} 0 (intrinsicop i64 vector_get_lane_v1i64 (dread v1i64 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_s64
  // CHECK: dassign %{{.*}} 0 (dread i64 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vslid_n_s64
  vslid_n_s64(set_int64_t(), set_int64_t(), 1);

  // CHECK: #inlining begin: FUNC vslid_n_u64
  // CHECK: #inlining begin: FUNC vsli_n_u64
  // CHECK: dassign %{{.*}} 0 (intrinsicop v1u64 vector_sli_n_v1u64 (dread v1u64 %{{.*}}, dread v1u64 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vsli_n_u64
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_u64
  // CHECK: dassign %{{.*}} 0 (intrinsicop u64 vector_get_lane_v1u64 (dread v1u64 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_u64
  // CHECK: dassign %{{.*}} 0 (dread u64 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vslid_n_u64
  vslid_n_u64(set_uint64_t(), set_uint64_t(), 1);

  // CHECK: #inlining begin: FUNC vrshrd_n_s64
  // CHECK: #inlining begin: FUNC vget_lane_s64
  // CHECK: dassign %{{.*}} 0 (intrinsicop i64 vector_get_lane_v1i64 (dread v1i64 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_s64
  // CHECK: dassign %{{.*}} 0 (dread i64 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vrshrd_n_s64
  vrshrd_n_s64(set_int64_t(), 1);

  // CHECK: #inlining begin: FUNC vrshrd_n_u64
  // CHECK: #inlining begin: FUNC vget_lane_u64
  // CHECK: dassign %{{.*}} 0 (intrinsicop u64 vector_get_lane_v1u64 (dread v1u64 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_u64
  // CHECK: dassign %{{.*}} 0 (dread u64 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vrshrd_n_u64
  vrshrd_n_u64(set_uint64_t(), 1);

  // CHECK: #inlining begin: FUNC vsrad_n_s64
  // CHECK: #inlining begin: FUNC vget_lane_s64
  // CHECK: dassign %{{.*}} 0 (intrinsicop i64 vector_get_lane_v1i64 (dread v1i64 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_s64
  // CHECK: dassign %{{.*}} 0 (dread i64 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vsrad_n_s64
  vsrad_n_s64(set_int64_t(), set_int64_t(), 1);

  // CHECK: #inlining begin: FUNC vsrad_n_u64
  // CHECK: #inlining begin: FUNC vget_lane_u64
  // CHECK: dassign %{{.*}} 0 (intrinsicop u64 vector_get_lane_v1u64 (dread v1u64 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_u64
  // CHECK: dassign %{{.*}} 0 (dread u64 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vsrad_n_u64
  vsrad_n_u64(set_uint64_t(), set_uint64_t(), 1);

  // CHECK: #inlining begin: FUNC vrsrad_n_s64
  // CHECK: #inlining begin: FUNC vget_lane_s64
  // CHECK: dassign %{{.*}} 0 (intrinsicop i64 vector_get_lane_v1i64 (dread v1i64 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_s64
  // CHECK: dassign %{{.*}} 0 (dread i64 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vrsrad_n_s64
  vrsrad_n_s64(set_int64_t(), set_int64_t(), 1);

  // CHECK: #inlining begin: FUNC vrsrad_n_u64
  // CHECK: #inlining begin: FUNC vget_lane_u64
  // CHECK: dassign %{{.*}} 0 (intrinsicop u64 vector_get_lane_v1u64 (dread v1u64 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_u64
  // CHECK: dassign %{{.*}} 0 (dread u64 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vrsrad_n_u64
  vrsrad_n_u64(set_uint64_t(), set_uint64_t(), 1);

  // CHECK: #inlining begin: FUNC vqshrunh_n_s16
  // CHECK: #inlining begin: FUNC vget_lane_u8
  // CHECK: dassign %{{.*}} 0 (intrinsicop u8 vector_get_lane_v8u8 (dread v8u8 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_u8
  // CHECK: dassign %{{.*}} 0 (dread u32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqshrunh_n_s16
  vqshrunh_n_s16(set_int16_t(), 1);

  // CHECK: #inlining begin: FUNC vqshruns_n_s32
  // CHECK: #inlining begin: FUNC vqshrun_n_s32
  // CHECK: dassign %{{.*}} 0 (intrinsicop v4u16 vector_qshrun_n_v4u16 (dread v4i32 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqshrun_n_s32
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_u16
  // CHECK: dassign %{{.*}} 0 (intrinsicop u16 vector_get_lane_v4u16 (dread v4u16 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_u16
  // CHECK: dassign %{{.*}} 0 (dread u32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqshruns_n_s32
  vqshruns_n_s32(set_int32_t(), 1);

  // CHECK: #inlining begin: FUNC vqshrund_n_s64
  // CHECK: #inlining begin: FUNC vqshrun_n_s64
  // CHECK: dassign %{{.*}} 0 (intrinsicop v2u32 vector_qshrun_n_v2u32 (dread v2i64 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqshrun_n_s64
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_u32
  // CHECK: dassign %{{.*}} 0 (intrinsicop u32 vector_get_lane_v2u32 (dread v2u32 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_u32
  // CHECK: dassign %{{.*}} 0 (dread u32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqshrund_n_s64
  vqshrund_n_s64(set_int64_t(), 1);

  // CHECK: #inlining begin: FUNC vqshrnh_n_s16
  // CHECK: #inlining begin: FUNC vqshrn_n_s16
  // CHECK: dassign %{{.*}} 0 (intrinsicop v8i8 vector_qshrn_n_v8i8 (dread v8i16 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqshrn_n_s16
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_s8
  // CHECK: dassign %{{.*}} 0 (intrinsicop i8 vector_get_lane_v8i8 (dread v8i8 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_s8
  // CHECK: dassign %{{.*}} 0 (dread i32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqshrnh_n_s16
  vqshrnh_n_s16(set_int16_t(), 1);

  // CHECK: #inlining begin: FUNC vqshrns_n_s32
  // CHECK: #inlining begin: FUNC vqshrn_n_s32
  // CHECK: dassign %{{.*}} 0 (intrinsicop v4i16 vector_qshrn_n_v4i16 (dread v4i32 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqshrn_n_s32
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_s16
  // CHECK: dassign %{{.*}} 0 (intrinsicop i16 vector_get_lane_v4i16 (dread v4i16 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_s16
  // CHECK: dassign %{{.*}} 0 (dread i32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqshrns_n_s32
  vqshrns_n_s32(set_int32_t(), 1);

  // CHECK: #inlining begin: FUNC vqshrnd_n_s64
  // CHECK: #inlining begin: FUNC vqshrn_n_s64
  // CHECK: dassign %{{.*}} 0 (intrinsicop v2i32 vector_qshrn_n_v2i32 (dread v2i64 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqshrn_n_s64
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_s32
  // CHECK: dassign %{{.*}} 0 (intrinsicop i32 vector_get_lane_v2i32 (dread v2i32 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_s32
  // CHECK: dassign %{{.*}} 0 (dread i32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqshrnd_n_s64
  vqshrnd_n_s64(set_int64_t(), 1);

  // CHECK: #inlining begin: FUNC vqshrnh_n_u16
  // CHECK: #inlining begin: FUNC vqshrn_n_u16
  // CHECK: dassign %{{.*}} 0 (intrinsicop v8u8 vector_qshrn_n_v8u8 (dread v8u16 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqshrn_n_u16
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_u8
  // CHECK: dassign %{{.*}} 0 (intrinsicop u8 vector_get_lane_v8u8 (dread v8u8 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_u8
  // CHECK: dassign %{{.*}} 0 (dread u32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqshrnh_n_u16
  vqshrnh_n_u16(set_uint16_t(), 1);

  // CHECK: #inlining begin: FUNC vqshrns_n_u32
  // CHECK: #inlining begin: FUNC vqshrn_n_u32
  // CHECK: dassign %{{.*}} 0 (intrinsicop v4u16 vector_qshrn_n_v4u16 (dread v4u32 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqshrn_n_u32
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_u16
  // CHECK: dassign %{{.*}} 0 (intrinsicop u16 vector_get_lane_v4u16 (dread v4u16 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_u16
  // CHECK: dassign %{{.*}} 0 (dread u32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqshrns_n_u32
  vqshrns_n_u32(set_uint32_t(), 1);

  // CHECK: #inlining begin: FUNC vqshrnd_n_u64
  // CHECK: #inlining begin: FUNC vqshrn_n_u64
  // CHECK: dassign %{{.*}} 0 (intrinsicop v2u32 vector_qshrn_n_v2u32 (dread v2u64 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqshrn_n_u64
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_u32
  // CHECK: dassign %{{.*}} 0 (intrinsicop u32 vector_get_lane_v2u32 (dread v2u32 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_u32
  // CHECK: dassign %{{.*}} 0 (dread u32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqshrnd_n_u64
  vqshrnd_n_u64(set_uint64_t(), 1);

  // CHECK: #inlining begin: FUNC vqrshrunh_n_s16
  // CHECK: #inlining begin: FUNC vqrshrun_n_s16
  // CHECK: dassign %{{.*}} 0 (intrinsicop v8u8 vector_qrshrun_n_v8u8 (dread v8i16 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqrshrun_n_s16
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_u8
  // CHECK: dassign %{{.*}} 0 (intrinsicop u8 vector_get_lane_v8u8 (dread v8u8 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_u8
  // CHECK: dassign %{{.*}} 0 (dread u32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqrshrunh_n_s16
  vqrshrunh_n_s16(set_int16_t(), 1);

  // CHECK: #inlining begin: FUNC vqrshruns_n_s32
  // CHECK: #inlining begin: FUNC vqrshrun_n_s32
  // CHECK: dassign %{{.*}} 0 (intrinsicop v4u16 vector_qrshrun_n_v4u16 (dread v4i32 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqrshrun_n_s32
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_u16
  // CHECK: dassign %{{.*}} 0 (intrinsicop u16 vector_get_lane_v4u16 (dread v4u16 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_u16
  // CHECK: dassign %{{.*}} 0 (dread u32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqrshruns_n_s32
  vqrshruns_n_s32(set_int32_t(), 1);

  // CHECK: #inlining begin: FUNC vqrshrund_n_s64
  // CHECK: #inlining begin: FUNC vqrshrun_n_s64
  // CHECK: dassign %{{.*}} 0 (intrinsicop v2u32 vector_qrshrun_n_v2u32 (dread v2i64 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqrshrun_n_s64
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_u32
  // CHECK: dassign %{{.*}} 0 (intrinsicop u32 vector_get_lane_v2u32 (dread v2u32 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_u32
  // CHECK: dassign %{{.*}} 0 (dread u32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqrshrund_n_s64
  vqrshrund_n_s64(set_int64_t(),  1);

  // CHECK: #inlining begin: FUNC vqrshrnh_n_s16
  // CHECK: #inlining begin: FUNC vqrshrn_n_s16
  // CHECK: dassign %{{.*}} 0 (intrinsicop v8i8 vector_qrshrn_n_v8i8 (dread v8i16 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqrshrn_n_s16
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_s8
  // CHECK: dassign %{{.*}} 0 (intrinsicop i8 vector_get_lane_v8i8 (dread v8i8 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_s8
  // CHECK: dassign %{{.*}} 0 (dread i32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqrshrnh_n_s16
  vqrshrnh_n_s16(set_int16_t(), 1);

  // CHECK: #inlining begin: FUNC vqrshrns_n_s32
  // CHECK: #inlining begin: FUNC vqrshrn_n_s32
  // CHECK: dassign %{{.*}} 0 (intrinsicop v4i16 vector_qrshrn_n_v4i16 (dread v4i32 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqrshrn_n_s32
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_s16
  // CHECK: dassign %{{.*}} 0 (intrinsicop i16 vector_get_lane_v4i16 (dread v4i16 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_s16
  // CHECK: dassign %{{.*}} 0 (dread i32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqrshrns_n_s32
  vqrshrns_n_s32(set_int32_t(), 1);

  // CHECK: #inlining begin: FUNC vqrshrnd_n_s64
  // CHECK: #inlining begin: FUNC vqrshrn_n_s64
  // CHECK: dassign %{{.*}} 0 (intrinsicop v2i32 vector_qrshrn_n_v2i32 (dread v2i64 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqrshrn_n_s64
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_s32
  // CHECK: dassign %{{.*}} 0 (intrinsicop i32 vector_get_lane_v2i32 (dread v2i32 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_s32
  // CHECK: dassign %{{.*}} 0 (dread i32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqrshrnd_n_s64
  vqrshrnd_n_s64(set_int64_t(), 1);

  // CHECK: #inlining begin: FUNC vqrshrnh_n_u16
  // CHECK: #inlining begin: FUNC vqrshrn_n_u16
  // CHECK: dassign %{{.*}} 0 (intrinsicop v8u8 vector_qrshrn_n_v8u8 (dread v8u16 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqrshrn_n_u16
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_u8
  // CHECK: dassign %{{.*}} 0 (intrinsicop u8 vector_get_lane_v8u8 (dread v8u8 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_u8
  // CHECK: dassign %{{.*}} 0 (dread u32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqrshrnh_n_u16
  vqrshrnh_n_u16(set_uint16_t(), 1);

  // CHECK: #inlining begin: FUNC vqrshrns_n_u32
  // CHECK: #inlining begin: FUNC vqrshrn_n_u32
  // CHECK: dassign %{{.*}} 0 (intrinsicop v4u16 vector_qrshrn_n_v4u16 (dread v4u32 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqrshrn_n_u32
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_u16
  // CHECK: dassign %{{.*}} 0 (intrinsicop u16 vector_get_lane_v4u16 (dread v4u16 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_u16
  // CHECK: dassign %{{.*}} 0 (dread u32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqrshrns_n_u32
  vqrshrns_n_u32(set_uint32_t(), 1);

  // CHECK: #inlining begin: FUNC vqrshrnd_n_u64
  // CHECK: #inlining begin: FUNC vqrshrn_n_u64
  // CHECK: dassign %{{.*}} 0 (intrinsicop v2u32 vector_qrshrn_n_v2u32 (dread v2u64 %{{.*}}, constval i32 1))
  // CHECK-NEXT: #inlining end: FUNC vqrshrn_n_u64
  // CHECK-NEXT: #inlining begin: FUNC vget_lane_u32
  // CHECK: dassign %{{.*}} 0 (intrinsicop u32 vector_get_lane_v2u32 (dread v2u32 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_u32
  // CHECK: dassign %{{.*}} 0 (dread u32 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vqrshrnd_n_u64
  vqrshrnd_n_u64(set_uint64_t(), 1);

  // CHECK: #inlining begin: FUNC vsrid_n_s64
  // CHECK: #inlining begin: FUNC vget_lane_s64
  // CHECK: dassign %{{.*}} 0 (intrinsicop i64 vector_get_lane_v1i64 (dread v1i64 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_s64
  // CHECK: dassign %{{.*}} 0 (dread i64 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vsrid_n_s64
  vsrid_n_s64(set_int64_t(), set_int64_t(), 1);

  // CHECK: #inlining begin: FUNC vsrid_n_u64
  // CHECK: #inlining begin: FUNC vget_lane_u64
  // CHECK: dassign %{{.*}} 0 (intrinsicop u64 vector_get_lane_v1u64 (dread v1u64 %{{.*}}, constval i32 0))
  // CHECK-NEXT: #inlining end: FUNC vget_lane_u64
  // CHECK: dassign %{{.*}} 0 (dread u64 %{{.*}})
  // CHECK-NEXT: #inlining end: FUNC vsrid_n_u64
  vsrid_n_u64(set_uint64_t(), set_uint64_t(), 1);

  return 0;
}