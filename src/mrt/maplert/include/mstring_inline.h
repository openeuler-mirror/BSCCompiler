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
#ifndef MRT_MAPLERT_INCLUDE_MSTRING_INLINE_H_
#define MRT_MAPLERT_INCLUDE_MSTRING_INLINE_H_

#include "mstring.h"
#include "heap_stats.h"
#include "mobject_inline.h"
#include "mrt_well_known.h"
namespace maplert {
inline uint32_t MString::GetCountValue() const {
  return count;
}

inline void MString::SetCountValue(uint32_t countValue) {
  count = countValue;
}

inline uint32_t MString::GetLength() const {
  return GetCountValue() >> 1;
}

inline uint32_t MString::GetHash() const {
  return hash;
}

inline void MString::SetHash(uint32_t hashCode) {
  hash = hashCode;
}

inline uint8_t *MString::GetContentsPtr() const {
  return reinterpret_cast<uint8_t*>(AsUintptr() + GetStringBaseSize());
}

inline uint32_t MString::GetCountOffset() {
  return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&static_cast<MString*>(0)->count));
}

inline bool MString::GetCompressFlag() {
  return enableStringCompression;
}

inline bool MString::IsCompress() const{
  uint32_t countValue = GetCountValue();
  return static_cast<unsigned int>(countValue) & 0x01;
}

inline bool MString::IsCompressedFromCount(const uint32_t countValue) {
  return countValue & 0x01;
}

inline bool MString::IsLiteral() const {
  // literal is off-heap java-string or string in PERM-space
  return IsOffHeap();
}

inline uint16_t MString::CharAt(uint32_t index) const {
  bool isCompress = IsCompress();
  uint8_t *strContent = GetContentsPtr();
  return isCompress ? *(strContent + index) : *(reinterpret_cast<uint16_t*>(strContent) + index);
}

inline bool MString::IsCompressChar(const uint16_t c) {
  return (c - 1u) < 0x7fu;
}

template<typename Type>
inline bool MString::IsCompressChars(const Type *chars, uint32_t length) {
  if (!enableStringCompression) {
    return false;
  }
  DCHECK(chars != nullptr) << "MString::IsCompressChars: chars is nullptr." << maple::endl;
  for (uint32_t i = 0; i < length; ++i) {
    if (!IsCompressChar(chars[i])) {
      return false;
    }
  }
  return true;
}

inline bool MString::IsCompressCharsExcept(const uint16_t *chars, uint32_t length, uint16_t except) {
  DCHECK(chars != nullptr) << "MString::IsCompressChars: chars is nullptr." << maple::endl;
  for (uint32_t i = 0; i < length; ++i) {
    uint16_t c = chars[i];
    if (!IsCompressChar(c) && c != except) {
      return false;
    }
  }
  return true;
}

inline uint32_t MString::GetStringSize() const {
  return IsCompress() ? GetStringBaseSize() + GetLength() : GetStringBaseSize() + GetCountValue();
}

inline uint32_t MString::GetStringBaseSize() {
  return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&(static_cast<MString*>(0))->content));
}

inline bool MString::Cmp(const std::string &src) const {
  bool isCompress = IsCompress();
  bool res = true;
  if (isCompress) {
    size_t dstLen = GetLength(); // extended to size_t
    size_t srcLen = src.length(); // may be > INT32_MAX
    uint8_t *dstName = GetContentsPtr();
    if (srcLen == dstLen) { // if srcLen > INT32_MAX, this must not be equal.
      for (size_t i = 0; i < dstLen; ++i) {
        if (src[i] != dstName[i]) {
          res = false;
          break;
        }
      }
    } else {
      res = false;
    }
  } else {
    std::string charStr = GetChars();
    res = (src == charStr);
  }
  return res;
}

template<typename srcType, typename Func>
inline MString *MString::NewStringObject(uint32_t stringLen, const Func &fillContents, bool isJNI) {
  constexpr bool compressible = (sizeof(uint8_t) == sizeof(srcType));
  DCHECK(stringLen <= ((UINT32_MAX - GetStringBaseSize()) / sizeof(srcType))) << "stringLen must valid" << maple::endl;
  size_t memLen = GetStringBaseSize() + stringLen * sizeof(srcType);
  uint32_t stringCount = (stringLen != 0) ? (((static_cast<unsigned int>(stringLen) << 1)) | compressible) : 0;
  MString *stringObj = static_cast<MString*>(MObject::NewObject(*WellKnown::GetMClassString(), memLen, isJNI));
  if (UNLIKELY(stringObj == nullptr)) {
    return nullptr;
  }
  stringObj->SetCountValue(stringCount);
  fillContents(*stringObj);
  // add memory order release
  std::atomic_thread_fence(std::memory_order_release);
  return stringObj;
}

template<typename srcType>
inline MString *MString::NewStringObject(const srcType *src, uint32_t stringLen, bool isJNI) {
  DCHECK(src != nullptr) << "MString::NewStringObject: src is nullptr." << maple::endl;
  DCHECK(stringLen <= UINT32_MAX / sizeof(srcType)) << "stringLen must valid" << maple::endl;
  size_t memLen = stringLen * sizeof(srcType);
  MString *res = NewStringObject<srcType>(stringLen, [&](MString &stringObj) {
    srcType *resDst = reinterpret_cast<srcType*>(stringObj.GetContentsPtr());
    if (memLen != 0) {
      if (memcpy_s(resDst, memLen, src, memLen) != EOK) {
        LOG(ERROR) << "newStringObject memcpy_s() not return EOK" << maple::endl;
      }
    }
  }, isJNI);
  return res;
}

template<typename srcType, typename dstType>
inline MString *MString::NewStringObject(const srcType *src, uint32_t stringLen, bool isJNI) {
  DCHECK(src != nullptr) << "MString::NewStringObject: src is nullptr." << maple::endl;
  MString *res = NewStringObject<dstType>(stringLen, [&](MString &stringObj) {
    dstType *resDst = reinterpret_cast<dstType*>(stringObj.GetContentsPtr());
    for (uint32_t i = 0; i < stringLen; ++i) {
      resDst[i] = static_cast<dstType>(src[i]);
    }
  }, isJNI);
  return res;
}

inline MString *MString::NewStringObject(const uint16_t *src, uint32_t stringLen, bool isJNI) {
  bool isCompress = IsCompressChars<uint16_t>(src, stringLen);
  return isCompress ? NewStringObject<uint16_t, uint8_t>(src, stringLen, isJNI) :
                      NewStringObject<uint16_t>(src, stringLen, isJNI);
}

inline MString *MString::NewEmptyStringObject() {
  return NewStringObject<uint8_t>(0, [](MString&) { });
}

inline void MString::SetStringClass() {
  // only use to set literal string
  if (GetClass() == nullptr) {
    StoreObjectOffHeap(0, WellKnown::GetMClassString());
    std::atomic_thread_fence(std::memory_order_release);
  }
}

template<typename T>
inline MString *MString::JniCast(T s) {
  static_assert(std::is_same<T, jstring>::value || std::is_same<T, jobject>::value, "wrong type");
  return reinterpret_cast<MString*>(s);
}

template<typename T>
inline MString *MString::JniCastNonNull(T s) {
  DCHECK(s != nullptr);
  return JniCast(s);
}

inline jstring MString::AsJstring() const {
  return reinterpret_cast<jstring>(const_cast<MString*>(this));
}
} // namespace maplert
#endif // MRT_MAPLERT_INCLUDE_MSTRING_INLINE_H_
