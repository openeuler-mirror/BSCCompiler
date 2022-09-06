/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "cl_option.h"
#include "driver_options.h"
#include "hir2mpl_ut_options.h"
#include <iostream>
#include "mpl_logging.h"
#include "parser_opt.h"
#include "triple.h"

namespace maple {

namespace opts::hir2mplut {

static maplecl::OptionCategory hir2mplUTCategory;

maplecl::Option<bool> help({"--help", "-h"},
                      "  -h, -help              : print usage and exit",
                      {hir2mplUTCategory});
maplecl::Option<std::string> genBase64({"--gen-base64", "-gen-base64"},
                                  "  -gen-base64 file.xx       : generate base64 string for file.xx",
                                  {hir2mplUTCategory});
maplecl::Option<std::string> mplt({"--mplt", "-mplt"},
                             "  -mplt lib1.mplt,lib2.mplt\n"
                             "                         : input mplt files",
                             {hir2mplUTCategory});

maplecl::Option<std::string> inClass({"--in-class", "-in-class"},
                                "  -in-class file1.jar,file2.jar\n"
                                "                         : input class files",
                                {hir2mplUTCategory});

maplecl::Option<std::string> inJar({"--in-jar", "-in-jar"},
                              "  -in-jar file1.jar,file2.jar\n"
                              "                         : input jar files",
                              {hir2mplUTCategory});
}

HIR2MPLUTOptions::HIR2MPLUTOptions()
    : runAll(false),
      runAllWithCore(false),
      genBase64(false),
      base64SrcFileName(""),
      coreMpltName("") {}

void HIR2MPLUTOptions::DumpUsage() const {
  std::cout << "========================================\n"
            << " Run gtest: hir2mplUT\n"
            << " Run gtest: hir2mplUT test [ options for gtest ]\n"
            << " Run ext mode: hir2mplUT ext [ options ]\n"
            << "========= options for ext mode =========\n";
  maplecl::CommandLine::GetCommandLine().HelpPrinter(opts::hir2mplut::hir2mplUTCategory);
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

  maplecl::CommandLine::GetCommandLine().Parse(argc, argv, opts::hir2mplut::hir2mplUTCategory);

  if (opts::hir2mplut::help) {
    DumpUsage();
    return false;
  }

  if (opts::hir2mplut::genBase64.IsEnabledByUser()) {
    base64SrcFileName = opts::hir2mplut::genBase64;
  }

  if (opts::hir2mplut::inClass.IsEnabledByUser()) {
    Split(opts::hir2mplut::inClass, ',', std::back_inserter(classFileList));
  }

  if (opts::hir2mplut::inJar.IsEnabledByUser()) {
    Split(opts::hir2mplut::inJar, ',', std::back_inserter(jarFileList));
  }

  if (opts::hir2mplut::mplt.IsEnabledByUser()) {
    Split(opts::hir2mplut::mplt, ',', std::back_inserter(mpltFileList));
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
