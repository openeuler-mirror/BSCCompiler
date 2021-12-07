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
#include "hir2mpl_ut_options.h"
#include <iostream>
#include "mpl_logging.h"
#include "option_parser.h"
#include "parser_opt.h"

namespace maple {
using namespace mapleOption;

enum OptionIndex {
  kHir2mplUTHelp = kCommonOptionEnd + 1,   // -h
  kGenBase64,                            // -genBase64
  kInClass,                              // -in-class
  kInJar,                                // -in-jar
  kMplt,                                 // -mplt
};

const Descriptor kUsage[] = {
  { kUnknown, 0, "", "", kBuildTypeAll, kArgCheckPolicyUnknown,
    "========================================\n"
    " Run gtest: hir2mplUT\n"
    " Run gtest: hir2mplUT test [ options for gtest ]\n"
    " Run ext mode: hir2mplUT ext [ options ]\n"
    "========= options for ext mode =========" },
  { kHelp, 0, "h", "help", kBuildTypeAll, kArgCheckPolicyNone,
    "  -h, --help                : print usage and exit", "hir2mplUT", {} },
  { kGenBase64, 0, "", "gen-base64", kBuildTypeAll, kArgCheckPolicyRequired,
    "  -gen-base64 file.xx       : generate base64 string for file.xx", "hir2mplUT", {} },
  { kInClass, 0, "", "in-class", kBuildTypeAll, kArgCheckPolicyRequired,
    "  -in-class file1.class,file2.class\n"
    "                            : input class files", "hir2mplUT", {} },
  { kInJar, 0, "", "in-jar", kBuildTypeAll, kArgCheckPolicyRequired,
    "  -in-jar file1.jar,file2.jar\n"
    "                            : input jar files", "hir2mplUT", {} },
  { kMplt, 0, "", "mplt", kBuildTypeAll, kArgCheckPolicyRequired,
    "  -mplt lib1.mplt,lib2.mplt\n"
    "                            : input mplt files", "hir2mplUT", {} },
  { kUnknown, 0, "", "", kBuildTypeAll, kArgCheckPolicyNone, "", "hir2mplUT", {} }
};

HIR2MPLUTOptions::HIR2MPLUTOptions()
    : runAll(false),
      runAllWithCore(false),
      genBase64(false),
      base64SrcFileName(""),
      coreMpltName("") {
        CreateUsages(kUsage);
      }

void HIR2MPLUTOptions::DumpUsage() const {
  for (unsigned int i = 0; !kUsage[i].help.empty(); i++) {
    std::cout << kUsage[i].help << std::endl;
  }
  exit(1);
}

bool HIR2MPLUTOptions::SolveArgs(int argc, char **argv) {
  if (argc == 1) {
    runAll = true;
    return true;
  }
  if (std::string(argv[1]).compare("test") == 0) {
    runAll = true;
    return true;
  }
  if (std::string(argv[1]).compare("testWithMplt") == 0) {
    runAllWithCore = true;
    CHECK_FATAL(argc > 2, "In TestWithMplt mode, core.mplt must be specified");
    coreMpltName = argv[2];
    return true;
  }
  if (std::string(argv[1]).compare("ext") != 0) {
    FATAL(kLncFatal, "Undefined mode");
    return false;
  }
  runAll = false;
  OptionParser optionParser;
  optionParser.RegisteUsages(DriverOptionCommon::GetInstance());
  optionParser.RegisteUsages(HIR2MPLUTOptions::GetInstance());

  ErrorCode ret = optionParser.Parse(argc, argv, "hir2mplUT");
  if (ret != ErrorCode::kErrorNoError) {
    DumpUsage();
    return false;
  }

  for (auto opt : optionParser.GetOptions()) {
    switch (opt.Index()) {
      case kHir2mplUTHelp:
        DumpUsage();
        return false;
      case kGenBase64:
        base64SrcFileName = opt.Args();
        genBase64 = true;
        break;
      case kInClass:
        Split(opt.Args(), ',', std::back_inserter(classFileList));
        break;
      case kInJar:
        Split(opt.Args(), ',', std::back_inserter(jarFileList));
        break;
      case kMplt:
        Split(opt.Args(), ',', std::back_inserter(mpltFileList));
        break;
      default:
        FATAL(kLncFatal, "Unsupport option %s", opt.OptionKey().c_str());
        DumpUsage();
        return false;
    }
  }
  return true;
}

template <typename Out>
void HIR2MPLUTOptions::Split(const std::string &s, char delim, Out result) {
  std::stringstream ss;
  ss.str(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    *(result++) = item;
  }
}
}  // namespace maple