/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
#ifndef MAPLE_LIB_CORE_MPL_FILEDS_H_
#define MAPLE_LIB_CORE_MPL_FILEDS_H_

#include <atomic>
#include <assert.h>

#include "jsan.h"
#include "gc_config.h"
#include "address.h"
#include "mrt_api_common.h"

#ifdef __cplusplus
namespace maplert {
extern "C" {
#endif

// Load and store operations for fields
MRT_EXPORT address_t MRT_LoadRefField(address_t obj, address_t *fieldAddr);
MRT_EXPORT void      MRT_WriteRefField(address_t obj, address_t *field, address_t value);
MRT_EXPORT address_t MRT_LoadVolatileField(address_t obj, address_t *fieldAddr);
MRT_EXPORT void      MRT_WriteVolatileField(address_t obj, address_t *objAddr, address_t value);
MRT_EXPORT void      MRT_WriteVolatileStaticField(address_t *fieldAddr, address_t value);
MRT_EXPORT void      MRT_WriteRefFieldStatic(address_t *field, address_t value);

// used for field with rc_weak_field annotation
MRT_EXPORT void      MRT_WriteWeakField(address_t obj, address_t *field, address_t value, bool isVolatile);
MRT_EXPORT address_t MRT_LoadWeakField(address_t obj, address_t *field, bool isVolatile);
MRT_EXPORT void      MRT_WriteReferentField(address_t obj, address_t *fieldAddr, address_t value,
                                            bool isResurrectWeak);
MRT_EXPORT address_t MRT_LoadReferentField(address_t obj, address_t *fieldAddr);

static inline bool MRT_LoadJBoolean(address_t obj, size_t offset) {
  JSAN_CHECK_OBJ(obj);
  return *reinterpret_cast<bool*>(obj + offset);
}

static inline bool MRT_LoadJBooleanVolatile(address_t obj, size_t offset) {
  JSAN_CHECK_OBJ(obj);
  return reinterpret_cast<std::atomic<bool>*>(obj + offset)->load(std::memory_order_seq_cst);
}

static inline int8_t MRT_LoadJByte(address_t obj, size_t offset) {
  JSAN_CHECK_OBJ(obj);
  return *reinterpret_cast<int8_t*>(obj + offset);
}

static inline int8_t MRT_LoadJByteVolatile(address_t obj, size_t offset) {
  JSAN_CHECK_OBJ(obj);
  return reinterpret_cast<std::atomic<int8_t>*>(obj + offset)->load(std::memory_order_seq_cst);
}

static inline int16_t MRT_LoadJShort(address_t obj, size_t offset) {
  JSAN_CHECK_OBJ(obj);
  return *reinterpret_cast<int16_t*>(obj + offset);
}

static inline int16_t MRT_LoadJShortVolatile(address_t obj, size_t offset) {
  JSAN_CHECK_OBJ(obj);
  return reinterpret_cast<std::atomic<int16_t>*>(obj + offset)->load(std::memory_order_seq_cst);
}

static inline uint16_t MRT_LoadJChar(address_t obj, size_t offset) {
  JSAN_CHECK_OBJ(obj);
  return *reinterpret_cast<uint16_t*>(obj + offset);
}

static inline uint16_t MRT_LoadJCharVolatile(address_t obj, size_t offset) {
  JSAN_CHECK_OBJ(obj);
  return reinterpret_cast<std::atomic<uint16_t>*>(obj + offset)->load(std::memory_order_seq_cst);
}

static inline int32_t MRT_LoadJInt(address_t obj, size_t offset) {
  JSAN_CHECK_OBJ(obj);
  return *reinterpret_cast<int32_t*>(obj + offset);
}

static inline int32_t MRT_LoadJIntVolatile(address_t obj, size_t offset) {
  JSAN_CHECK_OBJ(obj);
  return reinterpret_cast<std::atomic<int32_t>*>(obj + offset)->load(std::memory_order_seq_cst);
}

static inline int64_t MRT_LoadJLong(address_t obj, size_t offset) {
  JSAN_CHECK_OBJ(obj);
  return *reinterpret_cast<int64_t*>(obj + offset);
}

static inline int64_t MRT_LoadJLongVolatile(address_t obj, size_t offset) {
  JSAN_CHECK_OBJ(obj);
  return reinterpret_cast<std::atomic<int64_t>*>(obj + offset)->load(std::memory_order_seq_cst);
}

static inline float MRT_LoadJFloat(address_t obj, size_t offset) {
  JSAN_CHECK_OBJ(obj);
  return *reinterpret_cast<float*>(obj + offset);
}

static inline float MRT_LoadJFloatVolatile(address_t obj, size_t offset) {
  JSAN_CHECK_OBJ(obj);
  return reinterpret_cast<std::atomic<float>*>(obj + offset)->load(std::memory_order_seq_cst);
}

static inline double MRT_LoadJDouble(address_t obj, size_t offset) {
  JSAN_CHECK_OBJ(obj);
  return *reinterpret_cast<double*>(obj + offset);
}

static inline double MRT_LoadJDoubleVolatile(address_t obj, size_t offset) {
  JSAN_CHECK_OBJ(obj);
  return reinterpret_cast<std::atomic<double>*>(obj + offset)->load(std::memory_order_seq_cst);
}

static inline uintptr_t MRT_LoadJObject(address_t obj, size_t offset) {
  JSAN_CHECK_OBJ(obj);
  return reinterpret_cast<uintptr_t>(LoadRefField(obj, offset));
}

static inline uintptr_t MRT_LoadJObjectInc(address_t obj, size_t offset) {
  JSAN_CHECK_OBJ(obj);
  uintptr_t res = reinterpret_cast<uintptr_t>(MRT_LoadRefField(obj, reinterpret_cast<address_t*>(obj + offset)));
  return res;
}

static inline uintptr_t MRT_LoadJObjectIncVolatile(address_t obj, size_t offset) {
  JSAN_CHECK_OBJ(obj);
  uintptr_t res = reinterpret_cast<uintptr_t>(MRT_LoadVolatileField(obj, reinterpret_cast<address_t*>(obj + offset)));
  return res;
}

static inline uintptr_t MRT_LoadJObjectIncReferent(address_t obj, size_t offset) {
  JSAN_CHECK_OBJ(obj);
  uintptr_t res = reinterpret_cast<uintptr_t>(MRT_LoadReferentField(obj, reinterpret_cast<address_t*>(obj + offset)));
  return res;
}

static inline void MRT_StoreJBoolean(address_t obj, size_t offset, bool value) {
  JSAN_CHECK_OBJ(obj);
  *reinterpret_cast<bool*>(obj + offset) = value;
}

static inline void MRT_StoreJBooleanVolatile(address_t obj, size_t offset, bool value) {
  JSAN_CHECK_OBJ(obj);
  reinterpret_cast<std::atomic<bool>*>(obj + offset)->store(value, std::memory_order_seq_cst);
}

static inline void MRT_StoreJByte(address_t obj, size_t offset, int8_t value) {
  JSAN_CHECK_OBJ(obj);
  *reinterpret_cast<int8_t*>(obj + offset) = value;
}

static inline void MRT_StoreJByteVolatile(address_t obj, size_t offset, int8_t value) {
  JSAN_CHECK_OBJ(obj);
  reinterpret_cast<std::atomic<int8_t>*>(obj + offset)->store(value, std::memory_order_seq_cst);
}

static inline void MRT_StoreJShort(address_t obj, size_t offset, int16_t value) {
  JSAN_CHECK_OBJ(obj);
  *reinterpret_cast<int16_t*>(obj + offset) = value;
}

static inline void MRT_StoreJShortVolatile(address_t obj, size_t offset, int16_t value) {
  JSAN_CHECK_OBJ(obj);
  reinterpret_cast<std::atomic<int16_t>*>(obj + offset)->store(value, std::memory_order_seq_cst);
}

static inline void MRT_StoreJChar(address_t obj, size_t offset, uint16_t value) {
  JSAN_CHECK_OBJ(obj);
  *reinterpret_cast<uint16_t*>(obj + offset) = value;
}

static inline void MRT_StoreJCharVolatile(address_t obj, size_t offset, uint16_t value) {
  JSAN_CHECK_OBJ(obj);
  reinterpret_cast<std::atomic<uint16_t>*>(obj + offset)->store(value, std::memory_order_seq_cst);
}

static inline void MRT_StoreJInt(address_t obj, size_t offset, int32_t value) {
  JSAN_CHECK_OBJ(obj);
  *reinterpret_cast<int32_t*>(obj + offset) = value;
}

static inline void MRT_StoreJIntVolatile(address_t obj, size_t offset, int32_t value) {
  JSAN_CHECK_OBJ(obj);
  reinterpret_cast<std::atomic<int32_t>*>(obj + offset)->store(value, std::memory_order_seq_cst);
}

static inline void MRT_StoreOrderedJInt(address_t obj, size_t offset, int32_t value) {
  JSAN_CHECK_OBJ(obj);
  reinterpret_cast<std::atomic<int32_t>*>(obj + offset)->store(value, std::memory_order_release);
}

static inline void MRT_StoreJLong(address_t obj, size_t offset, int64_t value) {
  JSAN_CHECK_OBJ(obj);
  *reinterpret_cast<int64_t*>(obj + offset) = value;
}

static inline void MRT_StoreJLongVolatile(address_t obj, size_t offset, int64_t value) {
  JSAN_CHECK_OBJ(obj);
  reinterpret_cast<std::atomic<int64_t>*>(obj + offset)->store(value, std::memory_order_seq_cst);
}

static inline void MRT_StoreOrderedJLong(address_t obj, size_t offset, int64_t value) {
  JSAN_CHECK_OBJ(obj);
  reinterpret_cast<std::atomic<int64_t>*>(obj + offset)->store(value, std::memory_order_release);
}


static inline void MRT_StoreJFloat(address_t obj, size_t offset, float value) {
  JSAN_CHECK_OBJ(obj);
  *reinterpret_cast<float*>(obj + offset) = value;
}

static inline void MRT_StoreJFloatVolatile(address_t obj, size_t offset, float value) {
  JSAN_CHECK_OBJ(obj);
  reinterpret_cast<std::atomic<float>*>(obj + offset)->store(value, std::memory_order_seq_cst);
}

static inline void MRT_StoreJDouble(address_t obj, size_t offset, double value) {
  JSAN_CHECK_OBJ(obj);
  *reinterpret_cast<double*>(obj + offset) = value;
}

static inline void MRT_StoreJDoubleVolatile(address_t obj, size_t offset, double value) {
  JSAN_CHECK_OBJ(obj);
  reinterpret_cast<std::atomic<double>*>(obj + offset)->store(value, std::memory_order_seq_cst);
}

static inline void MRT_StoreJObject(address_t obj, size_t offset, uintptr_t value) {
  JSAN_CHECK_OBJ(obj);
  MRT_WriteRefField(obj, reinterpret_cast<address_t*>(obj + offset), reinterpret_cast<address_t>(value));
}

static inline void MRT_StoreJObjectStatic(address_t *addr, uintptr_t value) {
  JSAN_CHECK_OBJ(*addr);
  MRT_WriteRefFieldStatic(addr, reinterpret_cast<address_t>(value));
}

static inline void MRT_StoreMetaJobject(address_t obj, size_t offset, uintptr_t value) {
  JSAN_CHECK_OBJ(obj);
  StoreRefField(obj, offset, reinterpret_cast<address_t>(value));
}

static inline void MRT_StoreJObjectNoRc(address_t obj, size_t offset, uintptr_t value) {
  JSAN_CHECK_OBJ(obj);
  StoreRefField(obj, offset, reinterpret_cast<address_t>(value));
}

static inline void MRT_StoreJObjectVolatile(address_t obj, size_t offset, uintptr_t value) {
  JSAN_CHECK_OBJ(obj);
  MRT_WriteVolatileField(obj, reinterpret_cast<address_t*>(obj + offset), reinterpret_cast<address_t>(value));
}

static inline void MRT_StoreJObjectVolatileStatic(address_t *addr, uintptr_t value) {
  JSAN_CHECK_OBJ(*addr);
  MRT_WriteVolatileStaticField(addr, reinterpret_cast<address_t>(value));
}

static inline void MRT_WriteReferent(address_t obj, size_t offset, uintptr_t value, bool isResurrectWeak) {
  JSAN_CHECK_OBJ(obj);
  MRT_WriteReferentField(obj, reinterpret_cast<address_t*>(obj + offset),
                         reinterpret_cast<address_t>(value), isResurrectWeak);
}

#define MRT_LOAD_JBOOLEAN(obj, offset) MRT_LoadJBoolean(reinterpret_cast<address_t>(obj), offset)
#define MRT_LOAD_JBYTE(obj, offset) MRT_LoadJByte(reinterpret_cast<address_t>(obj), offset)
#define MRT_LOAD_JSHORT(obj, offset) MRT_LoadJShort(reinterpret_cast<address_t>(obj), offset)
#define MRT_LOAD_JCHAR(obj, offset) MRT_LoadJChar(reinterpret_cast<address_t>(obj), offset)
#define MRT_LOAD_JINT(obj, offset) MRT_LoadJInt(reinterpret_cast<address_t>(obj), offset)
#define MRT_LOAD_JLONG(obj, offset) MRT_LoadJLong(reinterpret_cast<address_t>(obj), offset)
#define MRT_LOAD_JFLOAT(obj, offset) MRT_LoadJFloat(reinterpret_cast<address_t>(obj), offset)
#define MRT_LOAD_JDOUBLE(obj, offset) MRT_LoadJDouble(reinterpret_cast<address_t>(obj), offset)

#define MRT_LOAD_JBOOLEAN_VOLATILE(obj, offset) MRT_LoadJBooleanVolatile(reinterpret_cast<address_t>(obj), offset)
#define MRT_LOAD_JBYTE_VOLATILE(obj, offset) MRT_LoadJByteVolatile(reinterpret_cast<address_t>(obj), offset)
#define MRT_LOAD_JSHORT_VOLATILE(obj, offset) MRT_LoadJShortVolatile(reinterpret_cast<address_t>(obj), offset)
#define MRT_LOAD_JCHAR_VOLATILE(obj, offset) MRT_LoadJCharVolatile(reinterpret_cast<address_t>(obj), offset)
#define MRT_LOAD_JINT_VOLATILE(obj, offset) MRT_LoadJIntVolatile(reinterpret_cast<address_t>(obj), offset)
#define MRT_LOAD_JLONG_VOLATILE(obj, offset) MRT_LoadJLongVolatile(reinterpret_cast<address_t>(obj), offset)
#define MRT_LOAD_JFLOAT_VOLATILE(obj, offset) MRT_LoadJFloatVolatile(reinterpret_cast<address_t>(obj), offset)
#define MRT_LOAD_JDOUBLE_VOLATILE(obj, offset) MRT_LoadJDoubleVolatile(reinterpret_cast<address_t>(obj), offset)

// not used in mrt, native(android-mrt) should use defines in
// maplert/public-headers/mrt_fields_api.h
// MRT_LOAD_JOBJECT_INC load object and IncRef
#define MRT_LOAD_JOBJECT(obj, offset) \
    MRT_LoadJObject(reinterpret_cast<address_t>(obj), offset) trigger_compiler_err  // should not use
#define MRT_LOAD_JOBJECT_INC(obj, offset) \
    MRT_LoadJObjectInc(reinterpret_cast<address_t>(obj), offset)  // used in runtime code (maplert)
#define MRT_LOAD_JOBJECT_INC_VOLATILE(obj, offset) \
    MRT_LoadJObjectIncVolatile(reinterpret_cast<address_t>(obj), offset)  // used in runtime code (maplert)
#define MRT_LOAD_JOBJECT_INC_REFERENT(obj, offset) \
    MRT_LoadJObjectIncReferent(reinterpret_cast<address_t>(obj), offset)  // used in runtime code (maplert)
#define MRT_LOAD_META_JOBJECT(obj, offset) \
    MRT_LoadJObject(reinterpret_cast<address_t>(obj), offset)  // used in runtime code (maplert)

// ONLY use in mrt_array.cpp for unsafe array copying.
#define __UNSAFE_MRT_LOAD_JOBJECT_NOINC(obj, offset) MRT_LoadJObject(reinterpret_cast<address_t>(obj), offset)
#define __UNSAFE_MRT_STORE_JOBJECT_NORC(obj, offset, value) \
    MRT_StoreJObjectNoRc(reinterpret_cast<address_t>(obj), offset, reinterpret_cast<uintptr_t>(value))

#define MRT_STORE_JBOOLEAN(obj, offset, value) MRT_StoreJBoolean(reinterpret_cast<address_t>(obj), offset, value)
#define MRT_STORE_JBYTE(obj, offset, value) MRT_StoreJByte(reinterpret_cast<address_t>(obj), offset, value)
#define MRT_STORE_JSHORT(obj, offset, value) MRT_StoreJShort(reinterpret_cast<address_t>(obj), offset, value)
#define MRT_STORE_JCHAR(obj, offset, value) MRT_StoreJChar(reinterpret_cast<address_t>(obj), offset, value)
#define MRT_STORE_JINT(obj, offset, value) MRT_StoreJInt(reinterpret_cast<address_t>(obj), offset, value)
#define MRT_STORE_JLONG(obj, offset, value) MRT_StoreJLong(reinterpret_cast<address_t>(obj), offset, value)
#define MRT_STORE_JFLOAT(obj, offset, value) MRT_StoreJFloat(reinterpret_cast<address_t>(obj), offset, value)
#define MRT_STORE_JDOUBLE(obj, offset, value) MRT_StoreJDouble(reinterpret_cast<address_t>(obj), offset, value)
#define MRT_STORE_ORDERED_JINT(obj, offset, value) \
    MRT_StoreOrderedJInt(reinterpret_cast<address_t>(obj), offset, value)
#define MRT_STORE_ORDERED_JLONG(obj, offset, value) \
    MRT_StoreOrderedJLong(reinterpret_cast<address_t>(obj), offset, value)

#define MRT_STORE_JBOOLEAN_VOLATILE(obj, offset, value) \
    MRT_StoreJBooleanVolatile(reinterpret_cast<address_t>(obj), offset, value)
#define MRT_STORE_JBYTE_VOLATILE(obj, offset, value) \
    MRT_StoreJByteVolatile(reinterpret_cast<address_t>(obj), offset, value)
#define MRT_STORE_JSHORT_VOLATILE(obj, offset, value) \
    MRT_StoreJShortVolatile(reinterpret_cast<address_t>(obj), offset, value)
#define MRT_STORE_JCHAR_VOLATILE(obj, offset, value) \
    MRT_StoreJCharVolatile(reinterpret_cast<address_t>(obj), offset, value)
#define MRT_STORE_JINT_VOLATILE(obj, offset, value) \
    MRT_StoreJIntVolatile(reinterpret_cast<address_t>(obj), offset, value)
#define MRT_STORE_JLONG_VOLATILE(obj, offset, value) \
    MRT_StoreJLongVolatile(reinterpret_cast<address_t>(obj), offset, value)
#define MRT_STORE_JFLOAT_VOLATILE(obj, offset, value) \
    MRT_StoreJFloatVolatile(reinterpret_cast<address_t>(obj), offset, value)
#define MRT_STORE_JDOUBLE_VOLATILE(obj, offset, value) \
    MRT_StoreJDoubleVolatile(reinterpret_cast<address_t>(obj), offset, value)

#define MRT_STORE_JOBJECT(obj, offset, value) \
    MRT_StoreJObject(reinterpret_cast<address_t>(obj), offset, reinterpret_cast<uintptr_t>(value))

#define MRT_STORE_JOBJECT_STATIC(addr, value) \
    MRT_StoreJObjectStatic(addr, reinterpret_cast<uintptr_t>(value))

#define MRT_STORE_META_JOBJECT(obj, offset, value) \
    MRT_StoreMetaJobject(reinterpret_cast<address_t>(obj), offset, reinterpret_cast<uintptr_t>(value))

#define MRT_STORE_JOBJECT_VOLATILE(obj, offset, value) \
    MRT_StoreJObjectVolatile(reinterpret_cast<address_t>(obj), offset, reinterpret_cast<uintptr_t>(value))

#define MRT_STORE_JOBJECT_VOLATILE_STATIC(addr, value) \
    MRT_StoreJObjectVolatileStatic(addr, reinterpret_cast<uintptr_t>(value))

#define MRT_WRITE_REFERENT(obj, offset, value, isResurrectWeak) \
    MRT_WriteReferent(reinterpret_cast<address_t>(obj), offset, reinterpret_cast<uintptr_t>(value), isResurrectWeak)

#ifdef __cplusplus
} // namespace maplert
} // extern "C"
#endif

#endif //MAPLE_LIB_CORE_MPL_FILEDS_H_

