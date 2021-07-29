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
#ifndef MAPLE_DRIVER_INCLUDE_COMPILER_FACTORY_H
#define MAPLE_DRIVER_INCLUDE_COMPILER_FACTORY_H
#include <unordered_set>
#include "compiler.h"
#include "compiler_selector.h"
#include "error_code.h"
#include "mir_module.h"
#include "mir_parser.h"

namespace maple {
class CompilerFactory {
 public:
  static CompilerFactory &GetInstance();
  CompilerFactory(const CompilerFactory&) = delete;
  CompilerFactory(CompilerFactory&&) = delete;
  CompilerFactory &operator=(const CompilerFactory&) = delete;
  CompilerFactory &operator=(CompilerFactory&&) = delete;
  ~CompilerFactory();
  ErrorCode Compile(MplOptions &mplOptions);

 private:
  bool compileFinished = false;
  CompilerFactory();
  void Insert(const std::string &name, Compiler *value);
  ErrorCode DeleteTmpFiles(const MplOptions &mplOptions, const std::vector<std::string> &tempFiles,
                           const std::unordered_set<std::string> &finalOutputs) const;
  // Code_exp: it is from compiler_selector.h: using SupportedCompilers = std::unordered_map<std::string, Compiler*>;
  SupportedCompilers supportedCompilers;
  CompilerSelector *compilerSelector;
  std::unique_ptr<MIRModule> theModule;
};
}  // namespace maple
#endif  // MAPLE_DRIVER_INCLUDE_COMPILER_FACTORY_H
