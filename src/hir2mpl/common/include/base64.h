/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef HIR2MPL_INCLUDE_COMMON_BASE64_H
#define HIR2MPL_INCLUDE_COMMON_BASE64_H
#include <map>
#include <string>
#include "types_def.h"

namespace maple {
class Base64 {
 public:
  static std::string Encode(const uint8 *input, size_t length);
  static uint8 *Decode(const std::string &input, size_t &lengthRet);
  static std::map<char, char> InitEncodeMap();
  static std::map<char, uint8> InitDecodeMap();

 private:
  Base64() = default;
  ~Base64() = default;

  static std::map<char, char> encodeMap;
  static std::map<char, uint8> decodeMap;
  static size_t DecodeLength(const std::string &input);
};
}  // namespace maple
#endif