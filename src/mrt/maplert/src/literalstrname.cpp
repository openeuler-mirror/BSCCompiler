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
#include "literalstrname.h"

// literal string name is shared between maple compiler and runtime, thus not in namespace maplert
// note there is a macor kConstString "_C_STR_" in literalstrname.h
// which need to match
static const std::string mplConstStr("_C_STR_00000000000000000000000000000000");

namespace {
const uint32_t kMaxBytesLength = 15;
const char *kMplDigits = "0123456789abcdef";
}

// Return the hex string of bytes. The result is the combination of prefix "_C_STR_" and hex string of bytes.
// The upper 4 bits and lower 4 bits of bytes[i] are transformed to hex form and restored separately in hex string.
std::string LiteralStrName::GetHexStr(const uint8_t *bytes, uint32_t len) {
  if (bytes == nullptr) {
    return std::string();
  }
  std::string str(mplConstStr, 0, (len << 1) + kConstStringLen);
  for (unsigned i = 0; i < len; ++i) {
    str[2 * i + kConstStringLen] = kMplDigits[(bytes[i] & 0xf0) >> 4]; // get the hex value of upper 4 bits of bytes[i]
    str[2 * i + kConstStringLen + 1] = kMplDigits[bytes[i] & 0x0f]; // get the hex value of lower 4 bits of bytes[i]
  }
  return str;
}

// Return the hash code of data. The hash code is computed as
// s[0] * 31 ^ (len - 1) + s[1] * 31 ^ (len - 2) + ... + s[len - 1],
// where s[i] is the value of swapping the upper 8 bits and lower 8 bits of data[i].
int32_t LiteralStrName::CalculateHashSwapByte(const char16_t *data, uint32_t len) {
  uint32_t hash = 0;
  const char16_t *end = data + len;
  while (data < end) {
    hash = (hash << 5) - hash;
    char16_t val = *data++;
    hash += (((val << 8) & 0xff00) | ((val >> 8) & 0xff));
  }
  return static_cast<int32_t>(hash);
}

std::string LiteralStrName::GetLiteralStrName(const uint8_t *bytes, uint32_t len) {
  if (len <= kMaxBytesLength) {
    return GetHexStr(bytes, len);
  }
  return ComputeMuid(bytes, len);
}

std::string LiteralStrName::ComputeMuid(const uint8_t *bytes, uint32_t len) {
  DigestHash digestHash = GetDigestHash(*bytes, len);
  return GetHexStr(digestHash.bytes, kDigestHashLength);
}