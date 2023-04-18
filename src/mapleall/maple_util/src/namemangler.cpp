/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "namemangler.h"
#include <cassert>
#include <fstream>
#include <map>
#include <regex>

namespace namemangler {
#ifdef __MRT_DEBUG
#define MRT_ASSERT(f) assert(f)
#else
#define MRT_ASSERT(f) ((void)0)
#endif

const int kLocalCodebufSize = 1024;
const int kMaxCodecbufSize = (1 << 16); // Java spec support a max name length of 64K.

#define GETHEXCHAR(n) static_cast<char>((n) < 10 ? static_cast<int32_t>(n) + kZeroAsciiNum : (n) - 10 + kaAsciiNum)
#define GETHEXCHARU(n) static_cast<char>((n) < 10 ? static_cast<int32_t>(n) + kZeroAsciiNum : (n) - 10 + kAAsciiNum)

bool doCompression = false;

// Store a mapping between full string and its compressed version
// More frequent and specific strings go before  general ones,
// e.g. Ljava_2Flang_2FObject_3B goes before Ljava_2Flang_2F
//
using StringMap = std::map<const std::string, const std::string>;

const StringMap kInternalMangleTable = {
    { "Ljava_2Flang_2FObject_3B", "L0_3B" },
    { "Ljava_2Flang_2FClass_3B",  "L1_3B" },
    { "Ljava_2Flang_2FString_3B", "L2_3B" }
};

// This mapping is mainly used for compressing annotation strings
const StringMap kOriginalMangleTable = {
    { "Ljava/lang/Object", "L0" },
    { "Ljava/lang/Class",  "L1" },
    { "Ljava/lang/String", "L2" }
};

// The returned buffer needs to be explicitly freed
static inline char *AllocCodecBuf(size_t maxLen) {
  if (maxLen == 0) {
    return nullptr;
  }
  // each char may have 2 more char, so give out the max space buffer
  return reinterpret_cast<char*>(malloc((maxLen <= kLocalCodebufSize) ? 3 * maxLen : 3 * kMaxCodecbufSize));
}

static inline void FreeCodecBuf(char *buf) {
  free(buf);
}

static std::string CompressName(std::string &name, const StringMap &mapping = kInternalMangleTable) {
  for (auto &entry : mapping) {
    if (name.find(entry.first) != std::string::npos) {
      name = std::regex_replace(name, std::regex(entry.first), entry.second);
    }
  }
  return name;
}

static std::string DecompressName(std::string &name, const StringMap &mapping = kInternalMangleTable) {
  for (auto &entry : mapping) {
    if (name.find(entry.second) != std::string::npos) {
      name = std::regex_replace(name, std::regex(entry.second), entry.first);
    }
  }
  return name;
}

std::string GetInternalNameLiteral(std::string name) {
  return (doCompression ? CompressName(name) : name);
}

std::string GetOriginalNameLiteral(std::string name) {
  return (doCompression ? CompressName(name, kOriginalMangleTable) : name);
}

std::string EncodeName(const std::string &name) {
  // name is guaranteed to be null-terminated
  size_t nameLen = name.length();
  nameLen = nameLen > kMaxCodecbufSize ? kMaxCodecbufSize : nameLen;
  char *buf = AllocCodecBuf(nameLen);
  if (buf == nullptr) {
    return std::string(name);
  }

  size_t pos = 0;
  size_t i = 0;
  std::string str(name);
  std::u16string str16;
  while (i < nameLen) {
    unsigned char c = static_cast<unsigned char>(name[i]);
    if (c == '_') {
      buf[pos++] = '_';
      buf[pos++] = '_';
    } else if (c == '[') {
      buf[pos++] = 'A';
    } else if (isalnum(c)) {
      buf[pos++] = static_cast<char>(c);
    } else if (c <= 0x7F) {
      // _XX: '_' followed by ascii code in hex
      if (c == '.') {
        c = '/'; // use / in package name
      }
      buf[pos++] = '_';
      unsigned char n = c >> 4; // get the high 4 bit and calculate
      buf[pos++] = GETHEXCHARU(n);
      n = static_cast<unsigned char>(c - static_cast<unsigned char>(n << 4)); // revert
      buf[pos++] = GETHEXCHARU(n);
    } else {
      str16.clear();
      // process one 16-bit char at a time
      unsigned int n = UTF8ToUTF16(str16, str.substr(i), 1, false);
      buf[pos++] = '_';
      if ((n >> 16) == 1) { // if n is 16-bit
        unsigned short m = str16[0];
        buf[pos++] = 'u';
        buf[pos++] = GETHEXCHAR((m & 0xF000) >> 12);
        buf[pos++] = GETHEXCHAR((m & 0x0F00) >> 8);
        buf[pos++] = GETHEXCHAR((m & 0x00F0) >> 4);
        buf[pos++] = GETHEXCHAR(m & 0x000F);
      } else {
        unsigned short m = str16[0];
        buf[pos++] = 'U';
        buf[pos++] = GETHEXCHAR((m & 0xF000) >> 12);
        buf[pos++] = GETHEXCHAR((m & 0x0F00) >> 8);
        buf[pos++] = GETHEXCHAR((m & 0x00F0) >> 4);
        buf[pos++] = GETHEXCHAR(m & 0x000F);
        m = str16[1];
        buf[pos++] = GETHEXCHAR((m & 0xF000) >> 12);
        buf[pos++] = GETHEXCHAR((m & 0x0F00) >> 8);
        buf[pos++] = GETHEXCHAR((m & 0x00F0) >> 4);
        buf[pos++] = GETHEXCHAR(m & 0x000F);
      }
      i += static_cast<size_t>(int32_t(n & 0xFFFF) - 1);
    }
    i++;
  }

  buf[pos] = '\0';
  std::string newName = std::string(buf, pos);
  FreeCodecBuf(buf);
  if (doCompression) {
    newName = CompressName(newName);
  }
  return newName;
}

static inline bool UpdatePrimType(bool primType, int splitNo, uint32_t ch) {
  if (ch == 'L') {
    return false;
  }

  if (((ch == ';') || (ch == '(') || (ch == ')')) && (splitNo > 1)) {
    return true;
  }

  return primType;
}

const int kNumLimit = 10;
const int kCodeOffset3 = 12;
const int kCodeOffset2 = 8;
const int kCodeOffset = 4;
const int kJavaStrLength = 5;

std::string DecodeName(const std::string &name) {
  if (name.find(';') != std::string::npos) { // no need Decoding a non-encoded string
    return name;
  }
  std::string decompressedName;
  const char *namePtr = nullptr;
  size_t nameLen;

  if (doCompression) {
    decompressedName = name;
    decompressedName = DecompressName(decompressedName);
    namePtr = decompressedName.c_str();
    nameLen = decompressedName.length();
  }
  else {
    namePtr = name.c_str();
    nameLen = name.length();
  }

  // Demangled name is supposed to be shorter. No buffer overflow issue here.
  std::string newName(nameLen, '\0');

  bool primType = true;
  int splitNo = 0; // split: class 0 | method 1 | signature 2
  size_t pos = 0;
  std::string str;
  std::u16string str16;
  for (size_t i = 0; i < nameLen;) {
    unsigned char c = static_cast<unsigned char>(namePtr[i]);
    ++i;
    if (c == '_') { // _XX: '_' followed by ascii code in hex
      if (i >= nameLen) {
        break;
      }
      if (namePtr[i] == '_') {
        newName[pos++] = namePtr[i++];
      } else if (namePtr[i] == 'u') {
        str.clear();
        str16.clear();
        i++;
        c = static_cast<unsigned char>(namePtr[i++]);
        uint8_t b1 = (c <= '9') ? c - kZeroAsciiNum : c - kaAsciiNum + kNumLimit;
        c = static_cast<unsigned char>(namePtr[i++]);
        uint8_t b2 = (c <= '9') ? c - kZeroAsciiNum : c - kaAsciiNum + kNumLimit;
        c = static_cast<unsigned char>(namePtr[i++]);
        uint8_t b3 = (c <= '9') ? c - kZeroAsciiNum : c - kaAsciiNum + kNumLimit;
        c = static_cast<unsigned char>(namePtr[i++]);
        uint8_t b4 = (c <= '9') ? static_cast<uint8_t>(c - kZeroAsciiNum) :
                                  static_cast<uint8_t>(c - kaAsciiNum + kNumLimit);
        uint32_t codepoint = (b1 << kCodeOffset3) | (b2 << kCodeOffset2) | (b3 << kCodeOffset) | b4;
        str16 += static_cast<char16_t>(codepoint);
        unsigned int n = UTF16ToUTF8(str, str16, 1, false);
        if ((n >> 16) == 2) { // the count of str equal 2 to 4, use array to save the utf8 results
          newName[pos++] = str[0];
          newName[pos++] = str[1];
        } else if ((n >> 16) == 3) {
          newName[pos++] = str[0];
          newName[pos++] = str[1];
          newName[pos++] = str[2];
        } else if ((n >> 16) == 4) {
          newName[pos++] = str[0];
          newName[pos++] = str[1];
          newName[pos++] = str[2];
          newName[pos++] = str[3];
        }
      } else {
        c = static_cast<unsigned char>(namePtr[i++]);
        unsigned int v = (c <= '9') ? c - kZeroAsciiNum : c - kAAsciiNum + kNumLimit;
        unsigned int asc = v << kCodeOffset;
        if (i >= nameLen) {
          break;
        }
        c = static_cast<unsigned char>(namePtr[i++]);
        v = (c <= '9') ? c - kZeroAsciiNum : c - kAAsciiNum + kNumLimit;
        asc += v;

        newName[pos++] = static_cast<char>(asc);

        if (asc == '|') {
          splitNo++;
        }

        primType = UpdatePrimType(primType, splitNo, asc);
      }
    } else {
      if (splitNo < 2) { // split: class 0 | method 1 | signature 2
        newName[pos++] = static_cast<char>(c);
        continue;
      }

      primType = UpdatePrimType(primType, splitNo, c);
      if (primType) {
        newName[pos++] = (c == 'A') ? '[' : static_cast<char>(c);
      } else {
        newName[pos++] = static_cast<char>(c);
      }
    }
  }

  newName.resize(pos);
  return newName;
}

// input: maple name
// output: Ljava/lang/Object;  [Ljava/lang/Object;
void DecodeMapleNameToJavaDescriptor(const std::string &nameIn, std::string &nameOut) {
  nameOut = DecodeName(nameIn);
  if (nameOut[0] == 'A') {
    size_t i = 0;
    while (nameOut[i] == 'A') {
      nameOut[i++] = '[';
    }
  }
}

// convert maple name to java name
// http://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/design.html#resolving_native_method_names
std::string NativeJavaName(const std::string &name, bool overLoaded) {
  // Decompress name first because the generated native function name needs
  // to follow certain spec, not something maple can control.
  std::string decompressedName(name);
  if (doCompression) {
    decompressedName = DecompressName(decompressedName);
  }

  unsigned int nameLen = static_cast<unsigned int>(decompressedName.length()) + kJavaStrLength;
  std::string newName = "Java_";
  unsigned int i = 0;

  // leading A's are array
  while (i < nameLen && name[i] == 'A') {
    newName += "_3";
    i++;
  }

  bool isProto = false; // class names in prototype have 'L' and ';'
  bool isFuncname = false;
  bool isTypename = false;
  while (i < nameLen) {
    char c = decompressedName[i];
    if (c == '_') {
      i++;
      // UTF16 unicode
      if (decompressedName[i] == 'u') {
        newName += "_0";
        i++;
      } else if (decompressedName[i] == '_') {
        newName += "_1";
        i++;
      } else {
        // _XX: '_' followed by ascii code in hex
        c = decompressedName[i++];
        unsigned char v =
            (c <= '9') ? static_cast<unsigned char>(c - kZeroAsciiNum) :
                         static_cast<unsigned char>((c - kAAsciiNum) + kNumLimit);
        unsigned char asc = v << kCodeOffset;
        c = decompressedName[i++];
        v = (c <= '9') ? static_cast<unsigned char>(c - kZeroAsciiNum) :
                         static_cast<unsigned char>((c - kAAsciiNum) + kNumLimit);
        asc += v;
        if (asc == '/') {
          newName += "_";
        } else if (asc == '|' && !isFuncname) {
          newName += "_";
          isFuncname = true;
        } else if (asc == '|' && isFuncname) {
          if (!overLoaded) {
            break;
          }
          newName += "_";
          isFuncname = false;
        } else if (asc == '(') {
          newName += "_";
          isProto = true;
        } else if (asc == ')') {
          break;
        } else if (asc == ';' && !isFuncname) {
          if (isProto) {
            newName += "_2";
          }
          isTypename = false;
        } else if (asc == '$') {
          newName += "_00024";
        } else if (asc == '-') {
          newName += "_0002d";
        } else {
          printf("name = %s\n", decompressedName.c_str());
          printf("c = %c\n", asc);
          MRT_ASSERT(false && "more cases in NativeJavaName");
        }
      }
    } else {
      if (c == 'L' && !isFuncname && !isTypename) {
        if (isProto) {
          newName += c;
        }
        isTypename = true;
        i++;
      } else if (c == 'A' && !isTypename && !isFuncname) {
        while (name[i] == 'A') {
          newName += "_3";
          i++;
        }
      } else {
        newName += c;
        i++;
      }
    }
  }
  return newName;
}

static uint16_t ChangeEndian16(uint16_t u16) {
  return ((u16 & 0xFF00) >> kCodeOffset2) | ((u16 & 0xFF) << kCodeOffset2);
}

/* UTF8
 * U+0000 - U+007F   0xxxxxxx
 * U+0080 - U+07FF   110xxxxx 10xxxxxx
 * U+0800 - U+FFFF   1110xxxx 10xxxxxx 10xxxxxx
 * U+10000- U+10FFFF 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 *
 * UTF16
 * U+0000 - U+D7FF   codePoint
 * U+E000 - U+FFFF   codePoint
 * U+10000- U+10FFFF XXXX YYYY
 *   code = codePoint - 0x010000, ie, 20-bit number in the range 0x000000..0x0FFFFF
 *   XXXX: top 10 bits of code + 0xD800: 0xD800..0xDBFF
 *   YYYY: low 10 bits of code + 0xDC00: 0xDC00..0xDFFF
 *
 * convert upto num UTF8 elements
 * return two 16-bit values: return_number_of_elements | consumed_input_number_of_elements
 */
const int kCodepointOffset1 = 6; // U+0080 - U+07FF   110xxxxx 10xxxxxx
const int kCodepointOffset2 = 12; // U+0800 - U+FFFF   1110xxxx 10xxxxxx 10xxxxxx
const int kCodepointOffset3 = 18; // U+10000- U+10FFFF 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
const int kCountOffset = 16;
const int kCodeAfterMinusOffset = 10; // codePoint equals itself minus 0x10000

unsigned UTF16ToUTF8(std::string &str, const std::u16string &str16, unsigned short num, bool isBigEndian) {
  uint32_t codePoint = 0;
  uint32_t i = 0;
  unsigned short count = 0;
  unsigned short retNum = 0;
  while (i < str16.length()) {
    if (isBigEndian || num == 1) {
      codePoint = str16[i++];
    } else {
      codePoint = ChangeEndian16(str16[i++]);
    }
    if (codePoint > 0xFFFF) {
      codePoint &= 0x3FF;
      codePoint <<= kNumLimit;
      if (isBigEndian) {
        codePoint += str16[i++] & 0x3FF;
      } else {
        codePoint += ChangeEndian16(str16[i++]) & 0x3FF;
      }
    }
    if (codePoint <= 0x7F) {
      str += static_cast<char>(codePoint);
      retNum += 1; // one UTF8 char
    } else if (codePoint <= 0x7FF) {
      str += static_cast<char>(0xC0 + (codePoint >> kCodepointOffset1));
      str += static_cast<char>(0x80 + (codePoint & 0x3F));
      retNum += 2; // two UTF8 chars
    } else if (codePoint <= 0xFFFF) {
      str += static_cast<char>(0xE0 + ((codePoint >> kCodepointOffset2) & 0xF));
      str += static_cast<char>(0x80 + ((codePoint >> kCodepointOffset1) & 0x3F));
      str += static_cast<char>(0x80 + (codePoint & 0x3F));
      retNum += 3; // three UTF8 chars
    } else {
      str += static_cast<char>(0xF0 + ((codePoint >> kCodepointOffset3) & 0x7));
      str += static_cast<char>(0x80 + ((codePoint >> kCodepointOffset2) & 0x3F));
      str += static_cast<char>(0x80 + ((codePoint >> kCodepointOffset1) & 0x3F));
      str += static_cast<char>(0x80 + (codePoint & 0x3F));
      retNum += 4; // four UTF8 chars
    }
    count++;
    if (num == count) {
      return ((static_cast<unsigned>(retNum)) << kCountOffset) | static_cast<unsigned>(i);
    }
  }
  return i;
}

bool NeedConvertUTF16(const std::string &str8) {
  uint32_t a = 0;
  size_t i = 0;
  size_t size = str8.length();
  while (i < size) {
    a = static_cast<uint8_t>(str8[i++]);
    constexpr uint8_t maxValidAscii = 0x7F;
    if (a > maxValidAscii) {
      return true;
    }
  }
  return false;
}

uint32_t GetCodePoint(const std::string &str8, uint32_t &i) {
  uint32_t b;
  uint32_t c;
  uint32_t d;
  uint32_t codePoint = 0;
  uint32_t a = static_cast<uint8_t>(str8[i++]);
  if (a <= 0x7F) { // 0...
    codePoint = a;
  } else if (a >= 0xF0) { // 11110...
    b = static_cast<uint32_t>(str8[i++]);
    c = static_cast<uint32_t>(str8[i++]);
    d = static_cast<uint32_t>(str8[i++]);
    codePoint = ((a & 0x7) << kCodepointOffset3) | ((b & 0x3F) << kCodepointOffset2) |
                ((c & 0x3F) << kCodepointOffset1) | (d & 0x3F);
  } else if (a >= 0xE0) { // 1110...
    b = static_cast<uint32_t>(str8[i++]);
    c = static_cast<uint32_t>(str8[i++]);
    codePoint = ((a & 0xF) << kCodepointOffset2) | ((b & 0x3F) << kCodepointOffset1) | (c & 0x3F);
  } else if (a >= 0xC0) { // 110...
    b = static_cast<uint32_t>(str8[i++]);
    codePoint = ((a & 0x1F) << kCodepointOffset1) | (b & 0x3F);
  } else {
    MRT_ASSERT(false && "invalid UTF-8");
  }
  return codePoint;
}

// convert upto num UTF16 elements
// two 16-bit values: return_number_of_elements | consumed_input_number_of_elements
unsigned UTF8ToUTF16(std::u16string &str16, const std::string &str8, unsigned short num, bool isBigEndian) {
  uint32_t i = 0;
  unsigned short count = 0;
  unsigned short retNum = 0;
  while (i < str8.length()) {
    uint32_t codePoint = GetCodePoint(str8, i);
    if (codePoint <= 0xFFFF) {
      if (isBigEndian || num == 1) {
        str16 += static_cast<char16_t>(codePoint);
      } else {
        str16 += static_cast<char16_t>(ChangeEndian16(static_cast<uint16_t>(codePoint)));
      }
      retNum += 1; // one utf16
    } else {
      codePoint -= 0x10000;
      if (isBigEndian || num == 1) {
        str16 += static_cast<char16_t>((codePoint >> kCodeAfterMinusOffset) | 0xD800);
        str16 += static_cast<char16_t>((codePoint & 0x3FF) | 0xDC00);
      } else {
        str16 += static_cast<char16_t>(
            ChangeEndian16(static_cast<uint16_t>((codePoint >> kCodeAfterMinusOffset) | 0xD800)));
        str16 += static_cast<char16_t>(ChangeEndian16((codePoint & 0x3FF) | 0xDC00));
      }
      retNum += 2; // two utf16
    }
    count++;
    // only convert num elmements
    if (num == count) {
      return (static_cast<uint32_t>(retNum) << kCountOffset) | i;
    }
  }
  return i;
}

const uint32_t kGreybackOffset = 7;
void GetUnsignedLeb128Encode(std::vector<uint8_t> &dest, uint32_t value) {
  bool done = false;
  do {
    uint8_t byte = value & 0x7f;
    value >>= kGreybackOffset;
    done = (value == 0);
    if (!done) {
      byte |= 0x80;
    }
    dest.push_back(byte);
  } while (!done);
}

uint32_t GetUnsignedLeb128Decode(const uint8_t **data) {
  MRT_ASSERT(data != nullptr && "data in GetUnsignedLeb128Decode() is nullptr");
  const uint8_t *ptr = *data;
  uint32_t result = 0;
  uint32_t shift = 0;
  uint8_t byte = 0;
  while (true) {
    byte = *(ptr++);
    result |= static_cast<uint8_t>(byte & 0x7f) << shift;
    if ((byte & 0x80) == 0) {
      break;
    }
    shift += kGreybackOffset;
  }
  *data = ptr;
  return result;
}

size_t GetUleb128Size(uint64_t v) {
  MRT_ASSERT(v && "if v == 0, __builtin_clzll(v) is not defined");
  size_t clz = static_cast<size_t>(__builtin_clzll(v));
  // num of 7-bit groups
  return size_t((64 - clz + 6) / 7);
}

size_t GetSleb128Size(int32_t v) {
  size_t size = 0;

  // intended signed shift: block codedex here
  constexpr uint32_t oneByte = 8;
  uint32_t vShift = sizeof(v) * oneByte - kGreybackOffset;
  uint32_t maskRem = static_cast<uint32_t>(v) >> kGreybackOffset;
  int32_t rem = (v < 0) ? static_cast<int32_t>(maskRem | (UINT32_MAX << vShift)) : static_cast<int32_t>(maskRem);

  bool hasMore = true;
  int32_t end = ((v >= 0) ? 0 : -1);

  uint32_t remShift = sizeof(rem) * oneByte - kGreybackOffset;

  while (hasMore) {
    // judege whether has more valid rem
    hasMore = (rem != end) || ((static_cast<uint32_t>(rem) & 1) !=
                               (static_cast<uint>((static_cast<uint32_t>(v) >> 6)) & 1));
    size++;
    v = rem;
    // intended signed shift: block codedex here
    uint32_t blockRem = static_cast<uint32_t>(rem) >> kGreybackOffset;
    rem = (rem < 0) ? static_cast<int32_t>(blockRem | (UINT32_MAX << remShift)) : static_cast<int32_t>(blockRem);
  }
  return size;
}

// encode signed to output stream
uint32_t EncodeSLEB128(uint64_t value, std::ofstream &out) {
  bool more;
  uint32_t count = 0;
  do {
    uint8_t byte = value & 0x7f;
    // NOTE: this assumes that this signed shift is an arithmetic right shift.
    value >>= kGreybackOffset;
    more = !((((value == 0) && ((byte & 0x40) == 0)) ||
             ((value == static_cast<uint64_t>(-1)) && ((byte & 0x40) != 0))));
    count++;
    if (more) {
      byte |= 0x80; // Mark this byte to show that more bytes will follow.
    }
    out << static_cast<char>(byte);
  } while (more);
  return count;
}

uint32_t EncodeULEB128(uint64_t value, std::ofstream &out) {
  uint32_t count = 0;
  do {
    uint8_t byte = value & 0x7f;
    value >>= kGreybackOffset;
    count++;
    if (value != 0) {
      byte |= 0x80; // Mark this byte to show that more bytes will follow.
    }
    out << char(byte);
  } while (value != 0);
  return count;
}

// decode a ULEB128 value.
uint64_t DecodeULEB128(const uint8_t *p, unsigned *n, const uint8_t *end) {
  const uint8_t *origP = p;
  uint64_t value = 0;
  unsigned shift = 0;
  enum Literals {
    kOneHundredTwentyEight = 128,
    kSixtyFour = 64
  };
  do {
    if (p == end) {
      if (n) {
        *n = static_cast<unsigned>(p - origP);
      }
      return 0;
    }
    uint64_t slice = *p & 0x7f;
    if ((shift >= kSixtyFour && slice != 0) || ((slice << shift) >> shift) != slice) {
      if (n) {
        *n = static_cast<unsigned>(p - origP);
      }
      return 0;
    }
    value += slice << shift;
    shift += kGreybackOffset;
  } while (*p++ >= kOneHundredTwentyEight);
  if (n) {
    *n = static_cast<unsigned>(p - origP);
  }
  return value;
}

// decode a SLEB128 value.
int64_t DecodeSLEB128(const uint8_t *p, unsigned *n, const uint8_t *end) {
  const uint8_t *origP = p;
  int64_t value = 0;
  unsigned shift = 0;
  uint8_t byte;
  enum Literals {
    kOneHundredTwentyEight = 128,
    kSixtyFour = 64,
    kSixtyThree = 63
  };
  do {
    if (p == end) {
      if (n) {
        *n = static_cast<unsigned>(p - origP);
      }
      return 0;
    }
    byte = *p;
    uint64_t slice = byte & 0x7f;
    if ((shift >= kSixtyFour && slice != (value < static_cast<int64_t>(0) ? 0x7f : 0x00)) ||
        (shift == kSixtyThree && slice != 0 && slice != 0x7f)) {
      if (n) {
        *n = static_cast<unsigned>(p - origP);
      }
      return 0;
    }
    value = static_cast<int64_t>(static_cast<uint64_t>(value) | (slice << shift));
    shift += kGreybackOffset;
    ++p;
  } while (byte >= kOneHundredTwentyEight);
  // Sign extend negative numbers if needed.
  if (shift < kSixtyFour && (byte & 0x40)) {
    value = static_cast<int64_t>(static_cast<uint64_t>(value) | static_cast<uint64_t>(0xffffffffffffffff << shift));
  }
  if (n) {
    *n = static_cast<unsigned>(p - origP);
  }
  return value;
}

} // namespace namemangler
