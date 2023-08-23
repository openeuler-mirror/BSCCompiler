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
#include "mrt_naming_api.h"
#include "utils/string_utils.h"

namespace namemanglerapi {
static bool ConvertSpeicalChar(uint32_t code, std::string &str) {
  switch (code) {
    case '.':
    case '/':
      str = "_";
      return true;
    case '_':
      str = "_1";
      return true;
    case ';':
      str = "_2";
      return true;
    case '[':
      str = "_3";
      return true;
    default:
      return false;
  }
}

static inline void AppendUint(std::string &s, uint32_t code) {
  const uint16_t low = static_cast<uint16_t>(code & 0xFFFF);
  const uint16_t high = static_cast<uint16_t>(code >> 16); // get the high 16 bits

  stringutils::AppendFormat(s, "_0%04x", low);
  if (high != 0) {
    stringutils::AppendFormat(s, "_0%04x", high);
  }
}

static uint32_t DecodeUtf8(const char *&data) {
  // one~three utf8 bytes translate to utf16 in 'Basic Multilingual Plane'
  const uint8_t byte0 = *data++;
  // utf8 one byte: 0xxxxxxx
  if (byte0 < 0x80) {
    return byte0;
  }

  // utf8 two bytes: 110xxxxx 10xxxxxx
  uint32_t rc = 0;
  const uint8_t byte1 = *data++;
  if (byte0 < 0xe0) {
    rc = (static_cast<uint32_t>(byte0 & 0x1f) << 6) | (static_cast<uint32_t>(byte1 & 0x3f));
    return rc;
  }

  // utf8 three bytes: 1110xxxx 10xxxxxx 10xxxxxx
  const uint8_t byte2 = *data++;
  if (byte0 < 0xf0) {
    rc = (static_cast<uint32_t>(byte0 & 0x0f) << 12) |
         (static_cast<uint32_t>(byte1 & 0x3f) << 06) |
         (static_cast<uint32_t>(byte2 & 0x3f));
    return rc;
  }

  // Four utf8 bytes translate to utf16 in 'Supplementary Planes'.
  // In Supplementary Planes Utf16 is encoded as a 'Surrogate Pair'.
  // About utf16 you can refer to: https://zh.wikipedia.org/wiki/UTF-16
  //
  // utf8 four bytes: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
  const uint8_t byte3 = *data++;
  uint32_t code;
  code = (static_cast<uint32_t>(byte0 & 0x07) << 18) |
         (static_cast<uint32_t>(byte1 & 0x3f) << 12) |
         (static_cast<uint32_t>(byte2 & 0x3f) << 06) |
         (static_cast<uint32_t>(byte3 & 0x3f));

  // Convert to Surrogate Pair, use three steps:
  code -= 0x10000;
  uint32_t lead = (code >> 10) | 0xD800;
  uint32_t tail = (code & 0x3FF) | 0xDC00;
  rc = lead + (tail << 16);
  return rc;
}

static inline bool IsAlnum(uint32_t code) {
  if ((code >= 'a' && code <= 'z') || (code >= 'A' && code <= 'Z') || (code >= '0' && code <= '9')) {
    return true;
  }
  return false;
}

static std::string MangleInternal(const std::string &s, const bool isDex) {
  std::string output = "";
  const char *cp = &s[0];
  const char *end = cp + s.length();
  while (cp < end) {
    uint32_t code = DecodeUtf8(cp);
    if (isDex) {
      if (code == '(') {
        output.append("__");
        continue;
      }
      if (code == ')') {
        break;
      }
    }

    std::string str;
    if (IsAlnum(code)) {
      output.append(1, static_cast<char>(code));
    } else if (ConvertSpeicalChar(code, str)) {
      output.append(str);
    } else {
      AppendUint(output, code);
    }
  }

  return output;
}

std::string MangleForJniDex(const std::string &s) {
  return MangleInternal(s, true);
}

std::string MangleForJni(const std::string &s) {
  return MangleInternal(s, false);
}
}
