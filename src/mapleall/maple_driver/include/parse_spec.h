/*
 * Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_DRIVER_INCLUDE_PARSE_SPEC_H
#define MAPLE_DRIVER_INCLUDE_PARSE_SPEC_H
#include "compiler.h"

namespace maple {

class ParseSpec {
 public:
  ParseSpec() {}
  ~ParseSpec() = default;

  static std::vector<std::string> GetOpt(const std::vector<std::string> args);
  static ErrorCode GetOptFromSpecsByGcc(int argc, char **argv, const MplOptions &mplOptions);
};

}
#endif  // MAPLE_DRIVER_INCLUDE_PARSE_SPEC_H