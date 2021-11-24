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
#include "compiler.h"
#include "file_utils.h"
#include "default_options.def"

namespace maple {
std::string LdCompiler::GetBinPath(const MplOptions&) const {
#ifdef ANDROID
  return "prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/";
#else
  return FileUtils::SafeGetenv(kMapleRoot) + "/tools/bin/";
#endif
}

// TODO: Required to use ld instead of gcc; ld will be implemented later
const std::string &LdCompiler::GetBinName() const {
  return kBinNameGcc;
}

/* the tool name must be the same as exeName field in Descriptor structure */
const std::string &LdCompiler::GetTool() const {
  return kLdFlag;
}

DefaultOption LdCompiler::GetDefaultOptions(const MplOptions &options, const Action&) const {
  uint32_t len = sizeof(kLdDefaultOptions) / sizeof(MplOption);
  DefaultOption defaultOptions = { std::make_unique<MplOption[]>(len), len };

  for (uint32_t i = 0; i < len; ++i) {
    defaultOptions.mplOptions[i] = kLdDefaultOptions[i];
  }

  for (uint32_t i = 0; i < defaultOptions.length; ++i) {
    defaultOptions.mplOptions[i].SetValue(
        FileUtils::AppendMapleRootIfNeeded(defaultOptions.mplOptions[i].GetNeedRootPath(),
                                           defaultOptions.mplOptions[i].GetValue(),
                                           options.GetExeFolder()));
  }
  return defaultOptions;
}

std::string LdCompiler::GetInputFileName(const MplOptions&, const Action &action) const {
    std::string files;

    bool isFirstEntry = true;
    for (const auto &file : action.GetLinkFiles()) {
      /* Split Input files with " "; (except first entry) */
      if (isFirstEntry == true) {
        isFirstEntry = false;
      } else {
        files += " ";
      }

      files += StringUtils::GetStrBeforeLast(file, ".") + ".o";
    }
    return files;
}
}  // namespace maple
