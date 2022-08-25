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
#ifndef MAPLE_DRIVER_INCLUDE_COMPILER_H
#define MAPLE_DRIVER_INCLUDE_COMPILER_H
#include <map>
#include <unordered_set>
#include "error_code.h"
#include "mpl_options.h"
#include "cg_option.h"
#include "me_option.h"
#include "option.h"
#include "mir_module.h"
#include "mir_parser.h"
#include "driver_runner.h"
#include "bin_mplt.h"

namespace maple {
const std::string kBinNameNone = "";
const std::string kBinNameJbc2mpl = "jbc2mpl";
const std::string kBinNameCpp2mpl = "hir2mpl";
const std::string kBinNameClang = "clang";
const std::string kBinNameDex2mpl = "dex2mpl";
const std::string kBinNameMplipa = "mplipa";
const std::string kBinNameMe = "me";
const std::string kBinNameMpl2mpl = "mpl2mpl";
const std::string kBinNameMplcg = "mplcg";
const std::string kBinNameMapleComb = "maplecomb";
const std::string kBinNameMapleCombWrp = "maplecombwrp";
const std::string kMachine = "aarch64-";
const std::string kVendor = "unknown-";
const std::string kOperatingSystem = "linux-gnu-";
const std::string kLdFlag = "ld";
const std::string kGccFlag = "gcc";
const std::string kGppFlag = "g++";
const std::string kAsFlag = "as";
const std::string kInputPhase = "input";
const std::string kBinNameLd = kMachine + kOperatingSystem + kLdFlag;
const std::string kBinNameAs = kMachine + kOperatingSystem + kAsFlag;
const std::string kBinNameGcc = kMachine + kOperatingSystem + kGccFlag;
const std::string kBinNameGpp = kMachine + kOperatingSystem + kGppFlag;

constexpr char kGccBeIlp32SysrootPathEnv[] = "GCC_BIGEND_ILP32_SYSROOT_PATH";
constexpr char kGccBeSysrootPathEnv[] = "GCC_BIGEND_SYSROOT_PATH";
constexpr char kGccBePathEnv[] = "GCC_BIGEND_PATH";

class Compiler {
 public:
  explicit Compiler(const std::string &name) : name(name) {}

  virtual ~Compiler() = default;

  virtual ErrorCode Compile(MplOptions &options, const Action &action,
                            std::unique_ptr<MIRModule> &theModule);

  virtual void GetTmpFilesToDelete(const MplOptions &mplOptions [[maybe_unused]],
                                   const Action &action [[maybe_unused]],
                                   std::vector<std::string> &tempFiles [[maybe_unused]]) const {}

  virtual std::unordered_set<std::string> GetFinalOutputs(const MplOptions &mplOptions [[maybe_unused]],
                                                          const Action &action [[maybe_unused]]) const {
    return std::unordered_set<std::string>();
  }

  virtual void PrintCommand(const MplOptions&, const Action&) const {}

 protected:
  virtual std::string GetBinPath(const MplOptions &mplOptions) const;
  virtual const std::string &GetBinName() const {
    return kBinNameNone;
  }

  /* Default behaviour ToolName==BinName, But some tools have another behaviour:
   * AsCompiler: ToolName=kAsFlag, BinName=kMachine + kOperatingSystem + kAsFlag
   */
  virtual const std::string &GetTool() const {
    return GetBinName();
  }

  virtual std::string GetInputFileName(const MplOptions &options [[maybe_unused]], const Action &action) const {
    return action.GetInputFile();
  }

  virtual DefaultOption GetDefaultOptions(const MplOptions &options [[maybe_unused]],
                                          const Action &action [[maybe_unused]]) const {
    return DefaultOption();
  }

  virtual void AppendOutputOption(std::vector<MplOption> &, const std::string &) const {
    return;
  }

 private:
  const std::string name;
  std::vector<MplOption> MakeOption(const MplOptions &options,
                                    const Action &action) const;
  void AppendDefaultOptions(std::vector<MplOption> &finalOptions,
                            const std::vector<MplOption> &defaultOptions,
                            bool isDebug) const;
  void AppendExtraOptions(std::vector<MplOption> &finalOptions, const MplOptions &options,
                          bool isDebug, const Action &action) const;
  void AppendInputsAsOptions(std::vector<MplOption> &finalOptions,
                             const MplOptions &mplOptions, const Action &action) const;
  void ReplaceOrInsertOption(std::vector<MplOption> &finalOptions,
                             const std::string &key, const std::string &value) const;
  std::vector<MplOption> MakeDefaultOptions(const MplOptions &options,
                                            const Action &action) const;
  int Exe(const MplOptions &mplOptions, const std::vector<MplOption> &options) const;
  const std::string &GetName() const {
    return name;
  }
};

class Jbc2MplCompiler : public Compiler {
 public:
  explicit Jbc2MplCompiler(const std::string &name) : Compiler(name) {}

  ~Jbc2MplCompiler() = default;

 private:
  const std::string &GetBinName() const override;
  DefaultOption GetDefaultOptions(const MplOptions &options, const Action &action) const override;
  void GetTmpFilesToDelete(const MplOptions &mplOptions, const Action &action,
                           std::vector<std::string> &tempFiles) const override;
  std::unordered_set<std::string> GetFinalOutputs(const MplOptions &mplOptions,
                                                  const Action &action) const override;
};

class ClangCompiler : public Compiler {
 public:
  explicit ClangCompiler(const std::string &name) : Compiler(name) {}

  ~ClangCompiler() = default;

 private:
  const std::string &GetBinName() const override;
  std::string GetBinPath(const MplOptions &mplOptions) const override;
  DefaultOption GetDefaultOptions(const MplOptions &options, const Action &action) const override;
  void GetTmpFilesToDelete(const MplOptions &mplOptions, const Action &action,
                           std::vector<std::string> &tempFiles) const override;
  std::unordered_set<std::string> GetFinalOutputs(const MplOptions &mplOptions,
                                                  const Action &action) const override ;
  void AppendOutputOption(std::vector<MplOption> &finalOptions, const std::string &name) const override;
};

class ClangCompilerBeILP32 : public ClangCompiler {
 public:
  explicit ClangCompilerBeILP32(const std::string &name) : ClangCompiler(name) {}
 private:
  DefaultOption GetDefaultOptions(const MplOptions &options, const Action &action) const override;
};

class Cpp2MplCompiler : public Compiler {
 public:
  explicit Cpp2MplCompiler(const std::string &name) : Compiler(name) {}

  ~Cpp2MplCompiler() = default;

 private:
  std::string GetBinPath(const MplOptions &mplOptions) const override;
  const std::string &GetBinName() const override;
  std::string GetInputFileName(const MplOptions &options, const Action &action) const override;
  DefaultOption GetDefaultOptions(const MplOptions &options, const Action &action) const override;
  void GetTmpFilesToDelete(const MplOptions &mplOptions, const Action &action,
                           std::vector<std::string> &tempFiles) const override;
  std::unordered_set<std::string> GetFinalOutputs(const MplOptions &mplOptions,
                                                  const Action &action) const override;
  void AppendOutputOption(std::vector<MplOption> &finalOptions, const std::string &name) const override;
};

class Dex2MplCompiler : public Compiler {
 public:
  explicit Dex2MplCompiler(const std::string &name) : Compiler(name) {}

  ~Dex2MplCompiler() = default;
#ifdef INTERGRATE_DRIVER
  ErrorCode Compile(MplOptions &options, const Action &action,
                    std::unique_ptr<MIRModule> &theModule) override;
#endif

  void PrintCommand(const MplOptions &options, const Action &action) const override;

 private:
  const std::string &GetBinName() const override;
  DefaultOption GetDefaultOptions(const MplOptions &options, const Action &action) const override;
  void GetTmpFilesToDelete(const MplOptions &mplOptions, const Action &action,
                           std::vector<std::string> &tempFiles) const override;
  std::unordered_set<std::string> GetFinalOutputs(const MplOptions &mplOptions,
                                                  const Action &action) const override;
#ifdef INTERGRATE_DRIVER
  void PostDex2Mpl(std::unique_ptr<MIRModule> &theModule) const;
  bool MakeDex2mplOptions(const MplOptions &options);
#endif
};

class IpaCompiler : public Compiler {
 public:
  explicit IpaCompiler(const std::string &name) : Compiler(name) {}

  ~IpaCompiler() = default;

 private:
  const std::string &GetBinName() const override;
  DefaultOption GetDefaultOptions(const MplOptions &options, const Action &action) const override;
  std::string GetInputFileName(const MplOptions &options, const Action &action) const override;
};

class MapleCombCompiler : public Compiler {
 public:
  explicit MapleCombCompiler(const std::string &name) : Compiler(name) {}

  ~MapleCombCompiler() = default;

  ErrorCode Compile(MplOptions &options, const Action &action,
                    std::unique_ptr<MIRModule> &theModule) override;
  void PrintCommand(const MplOptions &options, const Action &action) const override;
  std::string GetInputFileName(const MplOptions &options, const Action &action) const override;

 private:
  std::unordered_set<std::string> GetFinalOutputs(const MplOptions &mplOptions,
                                                  const Action &action) const override;
  void GetTmpFilesToDelete(const MplOptions &mplOptions, const Action &action,
                           std::vector<std::string> &tempFiles) const override;
  ErrorCode MakeMeOptions(const MplOptions &options, DriverRunner &runner) const;
  ErrorCode MakeMpl2MplOptions(const MplOptions &options, DriverRunner &runner) const;
  std::string DecideOutExe(const MplOptions &options) const;
  std::string GetStringOfSafetyOption() const;
};

class MplcgCompiler : public Compiler {
 public:
  explicit MplcgCompiler(const std::string &name) : Compiler(name) {}

  ~MplcgCompiler() = default;
  ErrorCode Compile(MplOptions &options, const Action &action,
                    std::unique_ptr<MIRModule> &theModule) override;
  void PrintMplcgCommand(const MplOptions &options, const Action &action, const MIRModule &md) const;
  void SetOutputFileName(const MplOptions &options, const Action &action, const MIRModule &md);
  std::string GetInputFile(const MplOptions &options, const Action &action, const MIRModule *md) const;
 private:
  DefaultOption GetDefaultOptions(const MplOptions &options, const Action &action) const override;
  ErrorCode GetMplcgOptions(MplOptions &options, const Action &action, const MIRModule *theModule) const;
  ErrorCode MakeCGOptions(const MplOptions &options) const;
  const std::string &GetBinName() const override;
  std::string baseName;
  std::string outputFile;
};

class MapleCombCompilerWrp : public Compiler {
 public:
  explicit MapleCombCompilerWrp(const std::string &name) : Compiler(name) {}
  ~MapleCombCompilerWrp() = default;

  std::string GetInputFileName(const MplOptions &options, const Action &action) const override;

 private:
  std::string GetBinPath(const MplOptions &mplOptions) const override;
  const std::string &GetBinName() const override;
  DefaultOption GetDefaultOptions(const MplOptions &options, const Action &action) const override;
  void GetTmpFilesToDelete(const MplOptions &mplOptions, const Action &action,
                           std::vector<std::string> &tempFiles) const override;
  std::unordered_set<std::string> GetFinalOutputs(const MplOptions &mplOptions,
                                                  const Action &action) const override;
};

// Build .s to .o
class AsCompiler : public Compiler {
 public:
  explicit AsCompiler(const std::string &name) : Compiler(name) {}

  ~AsCompiler() = default;

 private:
  std::string GetBinPath(const MplOptions &mplOptions) const override;
  const std::string &GetBinName() const override;
  const std::string &GetTool() const override;
  DefaultOption GetDefaultOptions(const MplOptions &options, const Action &action) const override;
  std::string GetInputFileName(const MplOptions &options, const Action &action) const override;
  void GetTmpFilesToDelete(const MplOptions &mplOptions, const Action &action,
                           std::vector<std::string> &tempFiles) const override;
  std::unordered_set<std::string> GetFinalOutputs(const MplOptions &mplOptions,
                                                  const Action &action) const override;
  void AppendOutputOption(std::vector<MplOption> &finalOptions, const std::string &name) const override;
};

class AsCompilerBeILP32 : public AsCompiler {
 public:
  explicit AsCompilerBeILP32(const std::string &name) : AsCompiler(name) {}
 private:
  std::string GetBinPath(const MplOptions &mplOptions) const override;
  const std::string &GetBinName() const override;
  DefaultOption GetDefaultOptions(const MplOptions &options, const Action &action) const override;
};

// Build .o to .so
class LdCompiler : public Compiler {
 public:
  explicit LdCompiler(const std::string &name) : Compiler(name) {}

  ~LdCompiler() = default;

 private:
  std::string GetBinPath(const MplOptions &mplOptions) const override;
  const std::string &GetBinName() const override;
  const std::string &GetTool() const override;
  DefaultOption GetDefaultOptions(const MplOptions &options, const Action &action) const override;
  std::string GetInputFileName(const MplOptions &options, const Action &action) const override;
  void AppendOutputOption(std::vector<MplOption> &finalOptions, const std::string &name) const override;
};

class LdCompilerBeILP32 : public LdCompiler {
 public:
  explicit LdCompilerBeILP32(const std::string &name) : LdCompiler(name) {}
 private:
  std::string GetBinPath(const MplOptions &mplOptions) const override;
  const std::string &GetBinName() const override;
};

}  // namespace maple
#endif  // MAPLE_DRIVER_INCLUDE_COMPILER_H
