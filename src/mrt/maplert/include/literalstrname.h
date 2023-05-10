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
#ifndef MRT_INCLUDE_LITERALSTRNAME_H
#define MRT_INCLUDE_LITERALSTRNAME_H
#include <map>
#include "muid.h"

// literal string naming is shared between maple compiler and runtime, thus not in namespace maplert
const std::string kConstString = "_C_STR_";
const std::string kConstStringPtr = "_PTR_C_STR_";
const std::string kLocalStringPrefix = "L_STR_";

class LiteralStrName {
 public:
  static int32_t CalculateHashSwapByte(const char16_t *data, uint32_t len);
  static uint32_t CalculateHash(const char16_t *data, uint32_t len, bool dataIsCompress) {
    uint32_t hash = 0;
    if (dataIsCompress) {
      const char *dataStart = reinterpret_cast<const char*>(data);
      const char *end = dataStart + len;
      while (dataStart < end) {
        hash = (hash << 5) - hash + static_cast<uint32_t>(*dataStart++); // calculate the hash code of data
      }
    } else {
      const char16_t *end = data + len;
      while (data < end) {
        // calculate the hash code of data
        hash = (static_cast<unsigned int>(hash) << 5) - hash + static_cast<uint32_t>(*data++);
      }
    }
    return hash;
  }

  static std::string GetHexStr(const uint8_t *bytes, uint32_t len);
  static std::string GetLiteralStrName(const uint8_t *bytes, uint32_t len);
  static std::string ComputeMuid(const uint8_t *bytes, uint32_t len);
  static constexpr int kConstStringLen = 7;
};

#endif
