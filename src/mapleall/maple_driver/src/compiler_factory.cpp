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
#include "driver_options.h"
#include "file_utils.h"
#include "string_utils.h"
#include "mpl_logging.h"

using namespace maple;

CompilerFactory &CompilerFactory::GetInstance() {
  static CompilerFactory instance;
  return instance;
}

ErrorCode CompilerFactory::DeleteTmpFiles(const MplOptions &mplOptions,
                                          const std::vector<std::string> &tempFiles) const {
  int ret = 0;
  for (const std::string &tmpFile : tempFiles) {
    bool isSave = false;
    for (auto saveFile : mplOptions.GetSaveFiles()) {
      if (!saveFile.empty() && std::regex_match(tmpFile, std::regex(StringUtils::Replace(saveFile, "*", ".*?")))) {
        isSave = true;
        break;
      }
    }

    auto &inputs = mplOptions.GetInputFiles();
    if (!isSave && (std::find(inputs.begin(), inputs.end(), tmpFile) == inputs.end())) {
      bool isNeedRemove = true;
      /* If we compile several files we can have several last Actions,
       * so we need to NOT remove output files for each last Action.
       */
      for (auto &lastAction : mplOptions.GetActions()) {
        auto finalOutputs = lastAction->GetCompiler()->GetFinalOutputs(mplOptions, *lastAction);
        /* do not remove output files */
        if (finalOutputs.find(tmpFile) != finalOutputs.end()) {
          isNeedRemove = false;
        }
      }

      if (isNeedRemove) {
        (void)FileUtils::Remove(tmpFile);
      }
    }
  }
  return ret == 0 ? kErrorNoError : kErrorFileNotFound;
}

Toolchain *CompilerFactory::GetToolChain() {
  if (toolchain == nullptr) {
    if (maple::Triple::GetTriple().GetArch() == Triple::ArchType::aarch64_be) {
      toolchain = std::make_unique<Aarch64BeILP32Toolchain>();
    } else {
      toolchain = std::make_unique<Aarch64Toolchain>();
    }
  }

  return toolchain.get();
}

ErrorCode CompilerFactory::Select(Action &action, std::vector<Action*> &selectedActions) {
  ErrorCode ret = kErrorNoError;

  /* Traverse Action tree recursively and select compilers in
   * "from leaf(clang) to root(ld)" order */
  for (const std::unique_ptr<Action> &a : action.GetInputActions()) {
    if (a == nullptr) {
      LogInfo::MapleLogger(kLlErr) << "Action is not Initialized\n";
      return kErrorToolNotFound;
    }

    ret = Select(*a, selectedActions);
    if (ret != kErrorNoError) {
      return ret;
    }
  }

  Toolchain *toolChain = GetToolChain();
  if (toolChain == nullptr) {
    LogInfo::MapleLogger(kLlErr) << "Wrong ToolChain\n";
    return kErrorToolNotFound;
  }
  Compiler *compiler = toolChain->Find(action.GetTool());

  if (compiler == nullptr) {
    if (action.GetTool() != "input") {
      LogInfo::MapleLogger(kLlErr) << "Fatal error: " <<  action.GetTool()
                                   << " tool is not supported" << "\n";
      LogInfo::MapleLogger(kLlErr) << "Supported Tool: ";

      auto print = [](const auto &supportedComp) { std::cout << " " << supportedComp.first; };
      (void)std::for_each(toolChain->GetSupportedCompilers().begin(),
                          toolChain->GetSupportedCompilers().end(), print);
      LogInfo::MapleLogger(kLlErr) << "\n";

      return kErrorToolNotFound;
    }
  } else {
    action.SetCompiler(compiler);
    selectedActions.push_back(&action);
  }

  return ret;
}

ErrorCode CompilerFactory::Select(const MplOptions &mplOptions, std::vector<Action*> &selectedActions) {
  for (const std::unique_ptr<Action> &action : mplOptions.GetActions()) {
    if (action == nullptr) {
      LogInfo::MapleLogger(kLlErr) << "Action is not Initialized\n";
      return kErrorToolNotFound;
    }
    ErrorCode ret = Select(*action, selectedActions);
    if (ret != kErrorNoError) {
      return ret;
    }
  }

  return selectedActions.empty() ? kErrorToolNotFound : kErrorNoError;
}

ErrorCode CompilerFactory::Compile(MplOptions &mplOptions) {
  if (compileFinished) {
    LogInfo::MapleLogger() <<
        "Failed! Compilation has been completed in previous time and multi-instance compilation is not supported\n";
    return kErrorCompileFail;
  }

  /* Actions owner is MplOption, so while MplOption is alive we can use raw pointers here */
  std::vector<Action*> actions;
  ErrorCode ret = Select(mplOptions, actions);
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
  if (opts::debug) {
    mplOptions.PrintDetailCommand(false);
  }
  // Compiler finished
  compileFinished = true;

  if (!FileUtils::DelTmpDir()) {
    LogInfo::MapleLogger() << "Failed! Failed to delete tmpdir with command " << FileUtils::tmpFolderPath << "\n";
  }

  return ret;
}
