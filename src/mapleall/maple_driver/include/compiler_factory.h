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
#include "error_code.h"
#include "mir_module.h"
#include "mir_parser.h"
#include "triple.h"

namespace maple {

class Toolchain {
  using SupportedCompilers = std::unordered_map<std::string, std::unique_ptr<Compiler>>;
  SupportedCompilers compilers;

 public:
  Compiler *Find(const std::string &toolName) {
    auto it = compilers.find(toolName);
    if (it != compilers.end()) {
      return it->second.get();
    }
    return nullptr;
  }

  const SupportedCompilers &GetSupportedCompilers() const {
    return compilers;
  }

  virtual ~Toolchain() = default;

 protected:
  template<typename T>
  void AddCompiler(const std::string &toolName) {
    (void)compilers.emplace(std::make_pair(toolName, std::make_unique<T>(toolName)));
  }
};

class Aarch64Toolchain : public Toolchain {
 public:
  Aarch64Toolchain() {
    AddCompiler<Jbc2MplCompiler>("jbc2mpl");
    AddCompiler<Dex2MplCompiler>("dex2mpl");
    AddCompiler<Cpp2MplCompiler>("hir2mpl");
    AddCompiler<ClangCompiler>("clang");
    AddCompiler<IpaCompiler>("mplipa");
    AddCompiler<MapleCombCompiler>("me");
    AddCompiler<MapleCombCompiler>("mpl2mpl");
    AddCompiler<MplcgCompiler>("mplcg");
    AddCompiler<MapleCombCompiler>("maplecomb");
    AddCompiler<MapleCombCompilerWrp>("maplecombwrp");
    AddCompiler<AsCompiler>("as");
    AddCompiler<LdCompiler>("ld");
  }
};

class Aarch64BeILP32Toolchain : public Toolchain {
 public:
  Aarch64BeILP32Toolchain() {
    AddCompiler<Jbc2MplCompiler>("jbc2mpl");
    AddCompiler<Dex2MplCompiler>("dex2mpl");
    AddCompiler<Cpp2MplCompiler>("hir2mpl");
    AddCompiler<ClangCompilerBeILP32>("clang");
    AddCompiler<IpaCompiler>("mplipa");
    AddCompiler<MapleCombCompiler>("me");
    AddCompiler<MapleCombCompiler>("mpl2mpl");
    AddCompiler<MplcgCompiler>("mplcg");
    AddCompiler<MapleCombCompiler>("maplecomb");
    AddCompiler<MapleCombCompilerWrp>("maplecombwrp");
    AddCompiler<AsCompilerBeILP32>("as");
    AddCompiler<LdCompilerBeILP32>("ld");
  }
};

class CompilerFactory {
 public:
  static CompilerFactory &GetInstance();
  CompilerFactory(const CompilerFactory&) = delete;
  CompilerFactory(CompilerFactory&&) = delete;
  CompilerFactory &operator=(const CompilerFactory&) = delete;
  CompilerFactory &operator=(CompilerFactory&&) = delete;
  ~CompilerFactory() = default;

  ErrorCode Compile(MplOptions &mplOptions);
  Toolchain *GetToolChain();

 private:
  CompilerFactory() = default;

  ErrorCode Select(const MplOptions &mplOptions, std::vector<Action*> &selectedActions);
  ErrorCode Select(Action &action, std::vector<Action*> &selectedActions);
  ErrorCode DeleteTmpFiles(const MplOptions &mplOptions,
                           const std::vector<std::string> &tempFiles) const;

  bool compileFinished = false;
  std::unique_ptr<MIRModule> theModule;
  std::unique_ptr<Toolchain> toolchain;
};
}  // namespace maple
#endif  // MAPLE_DRIVER_INCLUDE_COMPILER_FACTORY_H
