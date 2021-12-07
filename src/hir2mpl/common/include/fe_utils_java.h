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
#ifndef HIR2MPL_INCLUDE_FE_UTILS_JAVA_H
#define HIR2MPL_INCLUDE_FE_UTILS_JAVA_H
#include <string>
#include <vector>
#include "feir_type.h"

namespace maple {
class FEUtilJava {
 public:
  static std::vector<std::string> SolveMethodSignature(const std::string &signature);
  static std::string SolveParamNameInJavaFormat(const std::string &signature);

 private:
  FEUtilJava() = default;
  ~FEUtilJava() = default;
};
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_FE_UTILS_JAVA_H