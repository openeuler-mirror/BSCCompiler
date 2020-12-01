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
#ifndef MRT_MAPLERT_INCLUDE_MSTRING_H_
#define MRT_MAPLERT_INCLUDE_MSTRING_H_
#include "mobject.h"
#include <string>
namespace maplert {
class PACKED(4) MString : public MObject {
 public:
  uint32_t GetCountValue() const;
  inline void SetCountValue(uint32_t value);
  inline uint32_t GetLength() const;
  uint32_t GetHash() const;
  inline void SetHash(uint32_t hash);
  uint8_t *GetContentsPtr() const;
  bool IsCompress() const;
  bool IsLiteral() const;
  inline uint16_t CharAt(uint32_t index) const;
  uint32_t GetStringSize() const;
  inline bool Cmp(const std::string &src) const;
  std::string GetChars() const;
  MString *Intern();
  bool Equals(const MString &src) const;
  inline void SetStringClass();
  static MString *InternUtf8(const std::string &str);
  static MString *InternUtf16(const std::string &str);
  static MString *InternUtf(const std::string &str);
  static uint32_t GetStringBaseSize();
  static uint32_t GetCountOffset();
  static  bool GetCompressFlag();
  template<typename Type>
  static inline bool IsCompressChars(const Type *chars, uint32_t length);
  static bool IsCompressChar(const uint16_t c);
  static bool IsCompressedFromCount(const uint32_t countValue);
  static inline bool IsCompressCharsExcept(const uint16_t *chars, uint32_t length, uint16_t except);
  template<typename srcType, typename Func>
  static inline MString *NewStringObject(uint32_t stringLen, const Func &fillContents, bool isJNI = false);
  template<typename srcType>
  static inline MString *NewStringObject(const srcType *src, uint32_t stringLen, bool isJNI = false);
  template<typename srcType, typename dstType>
  static inline MString *NewStringObject(const srcType *src, uint32_t stringLen, bool isJNI = false);
  static inline MString *NewStringObject(const uint16_t *src, uint32_t stringLen, bool isJNI = false);
  static MString *NewEmptyStringObject();
  static MString *NewConstEmptyStringObject();
  static MString *NewStringObjectFromUtf8(const std::string &str);
  static MString *NewStringObjectFromUtf16(const std::string &str);
  static MString *NewStringObjectFromUtf(const std::string &str);

  template<typename T>
  static inline MString *JniCast(T s);
  template<typename T>
  static inline MString *JniCastNonNull(T s);
  jstring inline AsJstring() const;

 private:
  uint32_t count;
  uint32_t hash;
  uint8_t content[];
  static constexpr bool enableStringCompression = true;
};
} // namespace maplert
#endif // MRT_MAPLERT_INCLUDE_MSTRING_H_
