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
#include "mstring.h"
#include "mstring_inline.h"
#include "mrt_string.h"
#include "chelper.h"

namespace maplert {
std::string MString::GetChars() const {
  uint32_t length = GetLength();
  uint8_t *src = GetContentsPtr();

  // Prepare result string
  std::string res;
  res.reserve(length);

  if (IsCompress()) {
    // If it is compressed, just copy the content.
    for (uint32_t i = 0; i < length; ++i) {
      // IsCompress() implies each character is within 1 <= ch <= 0x7f
      res.push_back(static_cast<char>(src[i])); // safe to cast
    }
    __MRT_ASSERT(res.length() == length, "Length is different after copying");
  } else if (IsCompressChars<uint16_t>(reinterpret_cast<uint16_t*>(src), length)) {
    // If the string itself is not compressed, but every character is compressible, we compress on the fly.
    for (uint32_t i = 0; i < length; ++i) {
      // IsCompressChars() implies each character is within 1 <= ch <= 0x7f
      res.push_back(static_cast<char>(src[i * sizeof(uint16_t)])); // For little endian. Safe to cast.
    }
    __MRT_ASSERT(res.length() == length, "Length is different after conversion");
  } else {
    // It contains non-ASCII characters.  We need to handle it with care.
    std::u16string str16(reinterpret_cast<char16_t*>(src));
    // Note: The last parameter of UTF16ToUTF8 is called "isBigEndian", but it really means whether the endianness of
    // each str16 character needs to be swapped.  We do not need to swap endianness, thus we pass `true`.
    uint32_t ret = namemangler::UTF16ToUTF8(res, str16, 0, true);
    CHECK(ret <= str16.length()) << "namemangler::UTF16ToUTF8 in GetChars() fail" << maple::endl;
  }
  return res;
}

MString *MString::Intern() {
  return GetOrInsertStringPool(*this);
}

MString *MString::InternUtf8(const std::string &str) {
  uint32_t length = static_cast<uint32_t>(str.length());
  MString *strObj = NewStringObject<uint8_t, uint8_t>(reinterpret_cast<const uint8_t*>(str.c_str()), length);
  if (UNLIKELY(strObj == nullptr)) {
    return nullptr;
  }
  MString *internedString = strObj->Intern();
  RC_LOCAL_DEC_REF(strObj);
  return internedString;
}

MString *MString::InternUtf16(const std::string &str) {
  MString *strObj = NewStringObjectFromUtf16(str.c_str());
  if (UNLIKELY(strObj == nullptr)) {
    return nullptr;
  }
  MString *internedString = strObj->Intern();
  RC_LOCAL_DEC_REF(strObj);
  return internedString;
}

MString *MString::InternUtf(const std::string &str) {
  uint32_t length = static_cast<uint32_t>(str.length());
  bool isCompress = IsCompressChars<uint8_t>(reinterpret_cast<const uint8_t*>(str.c_str()), length);
  return isCompress ? InternUtf8(str) : InternUtf16(str);
}

MString *MString::NewStringObjectFromUtf8(const std::string &str) {
  uint32_t length = static_cast<uint32_t>(str.length());
  MString *strObj = NewStringObject<uint8_t, uint8_t>(reinterpret_cast<const uint8_t*>(str.c_str()), length);
  return strObj;
}

MString *MString::NewStringObjectFromUtf16(const std::string &str) {
  std::u16string str16;
  (void)namemangler::UTF8ToUTF16(str16, str, 0, true);
  MString *jStr = NewStringObject(
      reinterpret_cast<const uint16_t*>(str16.c_str()), static_cast<uint32_t>(str16.length()));
  return jStr;
}

MString *MString::NewStringObjectFromUtf(const std::string &str) {
  uint32_t length = static_cast<uint32_t>(str.length());
  bool isCompress = IsCompressChars<uint8_t>(reinterpret_cast<const uint8_t*>(str.c_str()), length);
  return isCompress ? NewStringObjectFromUtf8(str) : NewStringObjectFromUtf16(str);
}

MString *MString::NewConstEmptyStringObject() {
  static MString *constEmptyString = nullptr;
  if (constEmptyString == nullptr) {
    MString *emptyStr = NewEmptyStringObject();
    constEmptyString = emptyStr->Intern();
    RC_LOCAL_DEC_REF(emptyStr);
  }
  return constEmptyString;
}

bool MString::Equals(const MString &src) const {
  if (&src == this) {
    return true;
  }
  uint32_t lenA = this->count;
  uint32_t lenB = src.count;
  bool aIscompress = (lenA & 0x1u) == 0x1u;
  bool bIscompress = (lenB & 0x1u) == 0x1u;
  lenA = lenA >> 1;
  lenB = lenB >> 1;
  if (lenA != lenB) {
    return false;
  }
  // at here hash code should already been the same
  const uint8_t *srcA = this->content;
  const uint8_t *srcB = src.content;
  if (aIscompress && bIscompress) {
    return (memcmp(srcA, srcB, lenA) == 0);
  } else if (!aIscompress && !bIscompress) {
    return (memcmp(srcA, srcB, lenA * sizeof(uint16_t)) == 0);
  } else {
    const uint8_t *compressChars = nullptr;
    const uint16_t *uncompressChars = nullptr;
    if (aIscompress) {
      compressChars = srcA;
      uncompressChars = reinterpret_cast<const uint16_t*>(srcB);
    } else {
      compressChars = srcB;
      uncompressChars = reinterpret_cast<const uint16_t*>(srcA);
    }
    for (uint32_t i = 0; i < lenA; ++i) {
      if (compressChars[i] != uncompressChars[i]) {
        return false;
      }
    }
    return true;
  }
}
} // namespace maplert
