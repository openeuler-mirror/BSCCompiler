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
#include "compiler_factory.h"
#include <regex>
#include "file_utils.h"
#include "string_utils.h"
#include "mpl_logging.h"

using namespace maple;

#define ADD_COMPILER(NAME, CLASSNAME)        \
  do {                                       \
    Insert((NAME), new (CLASSNAME)((NAME))); \
  } while (0);

CompilerFactory &CompilerFactory::GetInstance() {
  static CompilerFactory instance;
  return instance;
}

CompilerFactory::CompilerFactory() {
  // Supported compilers
  ADD_COMPILER("jbc2mpl", Jbc2MplCompiler)
  ADD_COMPILER("dex2mpl", Dex2MplCompiler)
  ADD_COMPILER("mplfe", Cpp2MplCompiler)
  ADD_COMPILER("clang", ClangCompiler)
  ADD_COMPILER("mplipa", IpaCompiler)
  ADD_COMPILER("me", MapleCombCompiler)
  ADD_COMPILER("mpl2mpl", MapleCombCompiler)
  ADD_COMPILER("maplecomb", MapleCombCompiler)
  ADD_COMPILER("maplecombwrp", MapleCombCompilerWrp)
  ADD_COMPILER("mplcg", MplcgCompiler)
  ADD_COMPILER("as", AsCompiler)
  ADD_COMPILER("ld", LdCompiler)
  compilerSelector = new CompilerSelectorImpl();
}

CompilerFactory::~CompilerFactory() {
  auto it = supportedCompilers.begin();
  while (it != supportedCompilers.end()) {
    if (it->second != nullptr) {
      delete it->second;
      it->second = nullptr;
    }
    ++it;
  }
  supportedCompilers.clear();
  if (compilerSelector != nullptr) {
    delete compilerSelector;
    compilerSelector = nullptr;
  }
}

void CompilerFactory::Insert(const std::string &name, Compiler *value) {
  (void)supportedCompilers.insert(make_pair(name, value));
}

ErrorCode CompilerFactory::DeleteTmpFiles(const MplOptions &mplOptions, const std::vector<std::string> &tempFiles,
                                          const std::unordered_set<std::string> &finalOutputs) const {
  int ret = 0;
  for (const std::string &tmpFile : tempFiles) {
    bool isSave = false;
    for (auto saveFile : mplOptions.GetSaveFiles()) {
      if (!saveFile.empty() && std::regex_match(tmpFile, std::regex(StringUtils::Replace(saveFile, "*", ".*?")))) {
        isSave = true;
        break;
      }
    }
    if (!isSave && mplOptions.GetInputFiles().find(tmpFile) == std::string::npos &&  // not input
        (finalOutputs.find(tmpFile) == finalOutputs.end())) {                   // not output
      ret = FileUtils::Remove(tmpFile);
    }
  }
  return ret == 0 ? kErrorNoError : kErrorFileNotFound;
}

ErrorCode CompilerFactory::Compile(MplOptions &mplOptions) {
  if (compileFinished) {
    LogInfo::MapleLogger() <<
        "Failed! Compilation has been completed in previous time and multi-instance compilation is not supported\n";
    return kErrorCompileFail;
  }

  /* Actions owner is MplOption, so while MplOption is alive we can use raw pointers here */
  std::vector<Action *> actions;
  if (compilerSelector == nullptr) {
    LogInfo::MapleLogger() << "Failed! Compiler is null." << "\n";
    return kErrorCompileFail;
  }
  ErrorCode ret = compilerSelector->Select(supportedCompilers, mplOptions, actions);
  if (ret != kErrorNoError) {
    return ret;
  }

  for (auto *action : actions) {
    if (action == nullptr) {
      LogInfo::MapleLogger() << "Failed! Compiler is null." << "\n";
      return kErrorCompileFail;
    }

    Compiler *compiler = action->GetCompiler();
    if (compiler == nullptr) {
      return kErrorToolNotFound;
    }

    ret = compiler->Compile(mplOptions, *action, this->theModule);
    if (ret != kErrorNoError) {
      return ret;
    }
  }
  if (mplOptions.HasSetDebugFlag()) {
    mplOptions.PrintDetailCommand(false);
  }
  // Compiler finished
  compileFinished = true;

  if (!mplOptions.HasSetSaveTmps() || !mplOptions.GetSaveFiles().empty()) {
    std::vector<std::string> tmpFiles;

    for (auto *action : actions) {
      action->GetCompiler()->GetTmpFilesToDelete(mplOptions, *action, tmpFiles);
    }

    Action *lastAction = actions.back();
    ret = DeleteTmpFiles(mplOptions, tmpFiles,
                         lastAction->GetCompiler()->GetFinalOutputs(mplOptions, *lastAction));
  }
  return ret;
}
