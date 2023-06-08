/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "ipa_option.h"
#include "driver_options.h"
#include "file_utils.h"
#include "mpl_logging.h"
#include "triple.h"

namespace maple {

namespace opts::ipa {
  const maplecl::Option<bool> help({"--help", "-h"},
                        "  -h --help                   \tPrint usage and exit.Available command names:\n",
                        {ipaCategory});

  maplecl::Option<bool> o1({"--O1", "-O1"},
                      "  --O1                        \tEnable basic inlining\n",
                      {ipaCategory});

  maplecl::Option<bool> o2({"--O2", "-O2"},
                      "  --O2                        \tEnable greedy inlining\n",
                      {ipaCategory});

  maplecl::Option<bool> effectipa({"--effectipa", "-effectipa"},
                             "  --effectipa                 \tEnable method side effect for ipa\n",
                             {ipaCategory});

  const maplecl::Option<std::string> inlinefunclist({"--inlinefunclist", "-inlinefunclist"},
                                         "  --inlinefunclist=           \tInlining related configuration\n",
                                         {ipaCategory});

  const maplecl::Option<bool> quiet({"--quiet", "-quiet"},
                         "  --quiet                     \tDisable out debug info\n",
                         {ipaCategory});
}

IpaOption &IpaOption::GetInstance() {
  static IpaOption instance;
  return instance;
}

bool IpaOption::SolveOptions() const {
  if (::opts::target.IsEnabledByUser()) {
    Triple::GetTriple().Init(::opts::target.GetValue());
  } else {
    Triple::GetTriple().Init();
  }

  if (opts::ipa::help.IsEnabledByUser()) {
    maplecl::CommandLine::GetCommandLine().HelpPrinter(ipaCategory);
    return false;
  }

  if (opts::ipa::quiet.IsEnabledByUser()) {
    MeOption::quiet = true;
    Options::quiet = true;
  }

  maplecl::CopyIfEnabled(MeOption::inlineFuncList, opts::ipa::inlinefunclist);

  return true;
}

bool IpaOption::ParseCmdline(int argc, char **argv, std::vector<std::string> &fileNames) const {
  // Default value
  MeOption::inlineFuncList = "";

  (void)maplecl::CommandLine::GetCommandLine().Parse(argc, static_cast<char **>(argv), ipaCategory);
  bool result = SolveOptions();
  if (!result) {
    return false;
  }

  auto &badArgs = maplecl::CommandLine::GetCommandLine().badCLArgs;
  for (auto &arg : badArgs) {
    if (FileUtils::IsFileExists(arg.first)) {
      fileNames.push_back(arg.first);
    } else {
      return false;
    }
  }

  return true;
}
}  // namespace maple

