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
#ifndef METADATA_INLINE_H
#define METADATA_INLINE_H

#include "metadata_layout.h"

template<typename T>
inline T DataRef32::GetDataRef() const {
  uint32_t format = refVal & kDataRefBitMask;
  switch (format) {
    case kDataRefIsDirect: {
      // Note if *refVal* is a memory address, it should not be sign-extended
      return reinterpret_cast<T>(static_cast<uintptr_t>(refVal));
    }
    case kDataRefIsOffset: {
      // Note if *refVal* is an offset, it is a signed integer and should be sign-extended
      int32_t offset = static_cast<int32_t>(refVal & ~static_cast<uint32_t>(kDataRefBitMask));
      intptr_t addr = reinterpret_cast<intptr_t>(this) + static_cast<intptr_t>(offset);
      return reinterpret_cast<T>(addr);
    }
    case kDataRefIsIndirect: {
      // Note if *refVal* is an offset, it is a signed integer and should be sign-extended
      int32_t offset = static_cast<int32_t>(refVal & ~static_cast<uint32_t>(kDataRefBitMask));
      intptr_t addr = reinterpret_cast<intptr_t>(this) + static_cast<intptr_t>(offset);
      uintptr_t *pRef = reinterpret_cast<uintptr_t*>(addr);
      return reinterpret_cast<T>(*pRef);
    }
    default: {
      std::abort();
    }
  }
}

template<typename T>
inline void DataRef32::SetDataRef(T ref, DataRefFormat format) {
  static_assert(sizeof(T) == sizeof(uintptr_t), "wrong type.");
  switch (format) {
    case kDataRefIsDirect: {
      // Note if *refVal* is a memory address, it should be an unsigned integer.
      uintptr_t val = reinterpret_cast<uintptr_t>(ref);
      if ((val & kDataRefBitMask) == 0 && val <= UINT32_MAX) {
        refVal = static_cast<uint32_t>(val);
        return;
      } else {
        std::abort();
      }
    }
    case kDataRefIsOffset: {
      intptr_t offset = static_cast<intptr_t>(reinterpret_cast<uintptr_t>(ref)) - reinterpret_cast<intptr_t>(this);
      if ((offset & kDataRefBitMask) == 0 && INT32_MIN <= offset && offset <= INT32_MAX) {
        refVal = static_cast<uint32_t>(offset | kDataRefIsOffset);
        return;
      } else {
        std::abort();
      }
    }
    default: {
      std::abort();
    }
  }
}

template<typename T>
inline T DataRef32::GetRawValue() const {
  return reinterpret_cast<T>(refVal);
}

template<typename T>
inline T DataRef::GetDataRef() const {
  intptr_t ref = static_cast<intptr_t>(refVal);
  intptr_t format = ref & kDataRefBitMask;
  switch (format) {
    case kDataRefIsDirect: return reinterpret_cast<T>(refVal);
    case kDataRefIsOffset: {
      ref &= ~static_cast<intptr_t>(kDataRefBitMask);
      ref += reinterpret_cast<intptr_t>(this);
      return reinterpret_cast<T>(ref);
    }
    case kDataRefIsIndirect: {
      ref &= ~static_cast<intptr_t>(kDataRefBitMask);
      ref += reinterpret_cast<intptr_t>(this);
      uintptr_t *pRef = reinterpret_cast<uintptr_t*>(ref);
      return reinterpret_cast<T>(*pRef);
    }
    default: {
      std::abort();
    }
  }
}

template<typename T>
inline void DataRef::SetDataRef(const T ref, const DataRefFormat format) {
  static_assert(sizeof(T) == sizeof(uintptr_t), "wrong type.");
  uintptr_t uRef = reinterpret_cast<uintptr_t>(ref);
  switch (format) {
    case kDataRefIsDirect: {
      refVal = reinterpret_cast<uintptr_t>(uRef);
      return;
    }
    case kDataRefIsOffset: {
      intptr_t offset = static_cast<intptr_t>(uRef) - reinterpret_cast<intptr_t>(this);
      if ((offset & kDataRefBitMask) == 0) {
        refVal = static_cast<uintptr_t>(offset | kDataRefIsOffset);
        return;
      } else {
        std::abort();
      }
    }
    default: {
      std::abort();
    }
  }
}

template<typename T>
inline T DataRef::GetRawValue() const {
  return reinterpret_cast<T>(refVal);
}

template<typename T>
inline T GctibRef32::GetGctibRef() const {
  uint32_t format = refVal & kGctibRefBitMask;
  switch (format) {
    case kGctibRefIsOffset: {
      // Note if *refVal* is an offset, it is a signed integer and should be sign-extended
      int32_t offset = static_cast<int32_t>(refVal);
      intptr_t addr = reinterpret_cast<intptr_t>(this) + static_cast<intptr_t>(offset);
      return reinterpret_cast<T>(addr);
    }
    case kGctibRefIsIndirect: {
      // Note if *refVal* is an offset, it is a signed integer and should be sign-extended
      int32_t offset = static_cast<int32_t>(refVal & ~static_cast<uint32_t>(kGctibRefBitMask));
      intptr_t addr = reinterpret_cast<intptr_t>(this) + static_cast<intptr_t>(offset);
      uintptr_t *pRef = reinterpret_cast<uintptr_t*>(addr);
      return reinterpret_cast<T>(*pRef);
    }
    default: {
      std::abort();
    }
  }
}

template<typename T>
inline void GctibRef32::SetGctibRef(T ref, GctibRefFormat format) {
  static_assert(sizeof(T) == sizeof(uintptr_t), "wrong type.");
  switch (format) {
    case kGctibRefIsOffset:  {
      intptr_t offset = reinterpret_cast<intptr_t>(ref) - reinterpret_cast<intptr_t>(this);
      if ((offset & kGctibRefBitMask) == 0 && INT32_MIN <= offset && offset <= INT32_MAX) {
        refVal = static_cast<uint32_t>(offset);
        return;
      } else {
        std::abort();
      }
    }
    default: {
      std::abort();
    }
  }
}

template<typename T>
inline T GctibRef::GetGctibRef() const {
  intptr_t ref = static_cast<intptr_t>(refVal);
  intptr_t format = ref & kGctibRefBitMask;
  switch (format) {
    case kGctibRefIsOffset: {
      ref += reinterpret_cast<intptr_t>(this);
      return reinterpret_cast<T>(ref);
    }
    case kGctibRefIsIndirect: {
      ref &= ~static_cast<intptr_t>(kGctibRefBitMask);
      ref += reinterpret_cast<intptr_t>(this);
      uintptr_t *pRef = reinterpret_cast<uintptr_t*>(ref);
      return reinterpret_cast<T>(*pRef);
    }
    default: {
      std::abort();
    }
  }
}

template<typename T>
inline void GctibRef::SetGctibRef(const T ref, const GctibRefFormat format) {
  static_assert(sizeof(T) == sizeof(uintptr_t), "wrong type.");
  switch (format) {
    case kGctibRefIsOffset: {
      intptr_t offset = static_cast<intptr_t>(reinterpret_cast<uintptr_t>(ref)) - reinterpret_cast<intptr_t>(this);
      if ((offset & kDataRefBitMask) == 0) {
        refVal = static_cast<uintptr_t>(offset);
        return;
      } else {
        std::abort();
      }
    }
    default: {
      std::abort();
    }
  }
}

#if defined(__aarch64__)
inline bool MByteRef::IsOffset() const {
  uintptr_t offset = reinterpret_cast<uintptr_t>(refVal);
  return (kEncodedPosOffsetMin < offset) && (offset < kEncodedPosOffsetMax);
}

template<typename T>
inline T MByteRef::GetRef() const {
  if (IsOffset()) {
    uint32_t offset = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(refVal) & ~kPositiveOffsetBias);
    intptr_t ref = reinterpret_cast<intptr_t>(this) + static_cast<intptr_t>(offset);
    return reinterpret_cast<T>(ref);
  }
  uintptr_t ref = reinterpret_cast<uintptr_t>(refVal);
  return reinterpret_cast<T>(ref);
}

template<typename T>
inline void MByteRef::SetRef(const T ref) {
  static_assert(sizeof(T) == sizeof(uintptr_t), "wrong type.");
  refVal = reinterpret_cast<uintptr_t>(ref);
  if (IsOffset()) {
    std::abort();
  }
}
#elif defined(__arm__)
inline bool MByteRef::IsOffset() const {
  intptr_t offset = static_cast<intptr_t>(refVal);
  return (kEncodedOffsetMin < offset) && (offset < kEncodedOffsetMax);
}

template<typename T>
inline T MByteRef::GetRef() const {
  if (IsOffset()) {
    int32_t offset = static_cast<int32_t>(static_cast<intptr_t>(refVal) - kPositiveOffsetBias);
    intptr_t ref = reinterpret_cast<intptr_t>(this) + static_cast<intptr_t>(offset);
    return reinterpret_cast<T>(ref);
  }
  uintptr_t ref = reinterpret_cast<uintptr_t>(refVal);
  return reinterpret_cast<T>(ref);
}

template<typename T>
inline void MByteRef::SetRef(const T ref) {
  static_assert(sizeof(T) == sizeof(uintptr_t), "wrong type.");
  refVal = reinterpret_cast<uintptr_t>(ref);
  if (IsOffset()) {
    std::abort();
  }
}
#endif

inline bool MByteRef32::IsPositiveOffset() const {
  return (kEncodedPosOffsetMin < refVal) && (refVal < kEncodedPosOffsetMax);
}

inline bool MByteRef32::IsNegativeOffset() const {
  int32_t offset = static_cast<int32_t>(refVal);
  return (kNegativeOffsetMin <= offset) && (offset < kNegativeOffsetMax);
}

inline bool MByteRef32::IsOffset() const {
  return IsNegativeOffset() || IsPositiveOffset();
}

template<typename T>
inline T MByteRef32::GetRef() const {
  if (IsPositiveOffset()) {
    uint32_t offset = refVal & ~kPositiveOffsetBias;
    intptr_t ref = reinterpret_cast<intptr_t>(this) + static_cast<intptr_t>(offset);
    return reinterpret_cast<T>(ref);
  } else if (IsNegativeOffset()) {
    int32_t offset = static_cast<int32_t>(refVal);
    intptr_t ref = reinterpret_cast<intptr_t>(this) + static_cast<intptr_t>(offset);
    return reinterpret_cast<T>(ref);
  }

  uintptr_t ref = static_cast<uintptr_t>(refVal);
  return reinterpret_cast<T>(ref);
}

template<typename T>
inline void MByteRef32::SetRef(T ref) {
  static_assert(sizeof(T) == sizeof(uintptr_t), "wrong type.");
  uintptr_t addr = reinterpret_cast<uintptr_t>(ref);
  refVal = static_cast<uint32_t>(addr);
#if defined(__aarch64__)
  if (IsOffset()) {
    std::abort();
  }
#endif
}

template<typename T>
inline T DataRefOffset32::GetDataRef() const {
  if (refOffset == 0) {
    return 0;
  }
  intptr_t ref = static_cast<intptr_t>(refOffset);
  ref += reinterpret_cast<intptr_t>(this);
  return reinterpret_cast<T>(static_cast<uintptr_t>(ref));
}

inline int32_t DataRefOffset32::GetRawValue() const {
  return refOffset;
}

inline void DataRefOffset32::SetRawValue(int32_t value) {
  refOffset = value;
}

// specialized for int32_t, check for out-of-boundary
template<typename T>
inline void DataRefOffset32::SetDataRef(T ref) {
  static_assert(sizeof(T) == sizeof(uintptr_t), "wrong type.");
  if (ref == 0) {
    refOffset = 0;
    return;
  }
  uintptr_t uRef = reinterpret_cast<uintptr_t>(ref);
  intptr_t offset = static_cast<intptr_t>(uRef) - reinterpret_cast<intptr_t>(this);
  if (INT32_MIN <= offset && offset <= INT32_MAX) {
    refOffset = static_cast<int32_t>(offset);
  } else {
    std::abort();
  }
}

template<typename T>
inline T DataRefOffsetPtr::GetDataRef() const {
  if (refOffset == 0) {
    return 0;
  }
  intptr_t ref = static_cast<intptr_t>(refOffset);
  ref += reinterpret_cast<intptr_t>(this);
  return reinterpret_cast<T>(static_cast<uintptr_t>(ref));
}

template<typename T>
inline void DataRefOffsetPtr::SetDataRef(T ref) {
  static_assert(sizeof(T) == sizeof(uintptr_t), "wrong type.");
  uintptr_t uRef = reinterpret_cast<uintptr_t>(ref);
  refOffset = static_cast<intptr_t>(uRef) - reinterpret_cast<intptr_t>(this);
}

inline intptr_t DataRefOffsetPtr::GetRawValue() const {
  return refOffset;
}

inline void DataRefOffsetPtr::SetRawValue(intptr_t value) {
  refOffset = value;
}

template<typename T>
inline T DataRefOffset::GetDataRef() const {
  return refOffset.GetDataRef<T>();
}

template<typename T>
inline void DataRefOffset::SetDataRef(T ref) {
  refOffset.SetDataRef(ref);
}

inline intptr_t DataRefOffset::GetRawValue() const {
  return refOffset.GetRawValue();
}

inline void DataRefOffset::SetRawValue(intptr_t value) {
  refOffset.SetRawValue(value);
}

template<typename T>
inline T MethodFieldRef::GetDataRef() const {
  return refOffset.GetDataRef<T>();
}

template<typename T>
inline void MethodFieldRef::SetDataRef(T ref) {
  refOffset.SetDataRef(ref);
}

inline intptr_t MethodFieldRef::GetRawValue() const {
  return refOffset.GetRawValue();
}

inline void MethodFieldRef::SetRawValue(intptr_t value) {
  refOffset.SetRawValue(value);
}

inline bool MethodFieldRef::IsCompact() const {
  return (static_cast<uint32_t>(refOffset.GetRawValue()) & kMethodFieldRefIsCompact) == kMethodFieldRefIsCompact;
}

template<typename T>
inline T MethodFieldRef::GetCompactData() const {
  uint8_t *ref = GetDataRef<uint8_t*>();
  uintptr_t isCompact = kMethodFieldRefIsCompact;
  return reinterpret_cast<T>(reinterpret_cast<uintptr_t>(ref) & (~isCompact));
}
#endif // METADATA_INLINE_H
