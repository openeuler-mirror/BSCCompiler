/*
 * Copyright (c) [2019] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "compiler_selector.h"
#include <algorithm>
#include "mpl_logging.h"

namespace maple {
Compiler *CompilerSelectorImpl::FindCompiler(const SupportedCompilers &compilers, const std::string &name) const {
  auto compiler = compilers.find(name);
  if (compiler != compilers.end()) {
    return compiler->second;
  }
  return nullptr;
}

ErrorCode CompilerSelectorImpl::InsertCompilerIfNeeded(std::vector<Compiler*> &selected,
                                                       const SupportedCompilers &compilers,
                                                       const std::string &name) const {
  Compiler *compiler = FindCompiler(compilers, name);
  if (compiler != nullptr) {
    if (std::find(selected.cbegin(), selected.cend(), compiler) == selected.cend()) {
      selected.push_back(compiler);
    }
    return kErrorNoError;
  }

  LogInfo::MapleLogger(kLlErr) << name << " not found!!!" << '\n';
  return kErrorToolNotFound;
}

ErrorCode CompilerSelectorImpl::Select(const SupportedCompilers &supportedCompilers,
                                       const MplOptions &mplOptions,
                                       Action &action,
                                       std::vector<Action*> &selectedActions) const {
  ErrorCode ret = kErrorNoError;

  /* Traverse Action tree recursively and select compilers in
   * "from leaf(clang) to root(ld)" order */
  for (const std::unique_ptr<Action> &a : action.GetInputActions()) {
    ret = Select(supportedCompilers, mplOptions, *a, selectedActions);
    if (ret != kErrorNoError) {
      return ret;
    }
  }

  Compiler *compiler = FindCompiler(supportedCompilers, action.GetTool());
  if (compiler == nullptr) {
    if (action.GetTool() != "input") {
      LogInfo::MapleLogger(kLlErr) << "Fatal error: " <<  action.GetTool()
                                   << " tool is not supported" << "\n";
      LogInfo::MapleLogger(kLlErr) << "Supported Tool: ";

      auto print = [](auto supportedComp) { std::cout << " " << supportedComp.first; };
      std::for_each(supportedCompilers.begin(), supportedCompilers.end(), print);
      LogInfo::MapleLogger(kLlErr) << "\n";

      return kErrorToolNotFound;
    }
  } else {
    action.SetCompiler(compiler);
    selectedActions.push_back(&action);
  }

  return ret;
}

ErrorCode CompilerSelectorImpl::Select(const SupportedCompilers &supportedCompilers,
                                       const MplOptions &mplOptions,
                                       std::vector<Action*> &selectedActions) const {
  ErrorCode ret;

  for (const std::unique_ptr<Action> &action : mplOptions.GetActions()) {
    ret = Select(supportedCompilers, mplOptions, *action, selectedActions);
    if (ret != kErrorNoError) {
      return ret;
    }
  }

  return selectedActions.empty() ? kErrorToolNotFound : kErrorNoError;
}
}  // namespace maple
