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
#ifndef MAPLE_UTIL_INCLUDE_SET_SPEC_H
#define MAPLE_UTIL_INCLUDE_SET_SPEC_H

#include <deque>
#include <string>
#include <vector>

#include "file_utils.h"

namespace maple {

struct SpecList {
  const char *name; /* name of the spec. */
  int nameLen; /* length of the name */
  const char* value; /* spec value. */
};

class SetSpec {
 public:
  static std::vector<SpecList *> specs;
  static std::deque<std::string> args;
  static void InitSpec();
  static void ReadSpec(const std::string &specsFile);
  static void DoSpecs(const char *spec);
  static void SetUpSpecs(std::deque<std::string_view> &arg, const std::string &specsFile);
  static void SetSpecs(const char *name, const char *spec);
  static void EndGoingArg(const std::string tmpArg);
  static std::string FindPath(const std::string tmpArg);
};
}  // namespace maple

#endif /* MAPLE_UTIL_INCLUDE_SET_SPEC_H */