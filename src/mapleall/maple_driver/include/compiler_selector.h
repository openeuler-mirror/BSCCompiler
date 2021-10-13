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
#ifndef MAPLE_DRIVER_INCLUDE_COMPILER_SELECTOR_H
#define MAPLE_DRIVER_INCLUDE_COMPILER_SELECTOR_H
#include <unordered_map>
#include <vector>
#include "compiler.h"
#include "error_code.h"

namespace maple {
using SupportedCompilers = std::unordered_map<std::string, Compiler*>;
class CompilerSelector {
 public:
  CompilerSelector() = default;

  virtual ~CompilerSelector() = default;

  virtual ErrorCode Select(const SupportedCompilers&, const MplOptions&,
                           std::vector<Action *>&) const {
    return kErrorToolNotFound;
  }
};

class CompilerSelectorImpl : public CompilerSelector {
 public:
  CompilerSelectorImpl() = default;

  ~CompilerSelectorImpl() = default;

  ErrorCode Select(const SupportedCompilers &supportedCompilers, const MplOptions &mplOptions,
                   std::vector<Action *> &selectedActions) const override;

 private:
  Compiler *FindCompiler(const SupportedCompilers &compilers, const std::string &name) const;
  ErrorCode InsertCompilerIfNeeded(std::vector<Compiler*> &selected, const SupportedCompilers &compilers,
                                   const std::string &name) const;
  ErrorCode Select(const SupportedCompilers &supportedCompilers, const MplOptions &mplOptions,
                   Action &action, std::vector<Action *> &selectedActions) const;
};
}  // namespace maple
#endif  // MAPLE_DRIVER_INCLUDE_COMPILER_SELECTOR_H
