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
#ifndef HIR2MPL_UT_INCLUDE_HIR2MPL_UT_REGX_H
#define HIR2MPL_UT_INCLUDE_HIR2MPL_UT_REGX_H
#include <string>
#include <regex>
#include "types_def.h"

namespace maple {
class HIR2MPLUTRegx {
 public:
  static const uint32 kAnyNumber = 0xFFFFFFFF;
  HIR2MPLUTRegx() = default;
  ~HIR2MPLUTRegx() = default;
  static bool Match(const std::string &str, const std::string &pattern);
  static std::string RegName(uint32 regNum);
  static std::string RefIndex(uint32 typeIdx);
  static std::string AnyNum(uint32 typeIdx);
  static std::string Any() {
    return "(.|\r|\n)*";
  }
};
}  // namespace maple
#endif  // HIR2MPL_UT_INCLUDE_HIR2MPL_UT_REGX_H