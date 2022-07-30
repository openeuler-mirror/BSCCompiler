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
#ifndef HIR2MPL_INCLUDE_JBC_UTIL_H
#define HIR2MPL_INCLUDE_JBC_UTIL_H
#include <string>
#include <vector>
#include "jbc_opcode.h"

namespace maple {
namespace jbc {
class JBCUtil {
 public:
  static std::string ClassInternalNameToFullName(const std::string &name);
  static JBCPrimType GetPrimTypeForName(const std::string &name);

 private:
  JBCUtil() = default;
  ~JBCUtil() = default;
};
}  // namespace jbc
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_JBC_UTIL_H