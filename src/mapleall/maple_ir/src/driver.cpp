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
#include <cstdlib>
#include <fstream>
#include <iostream>

#include "bin_mplt.h"
#include "constantfold.h"
#include "mir_function.h"
#include "mir_parser.h"
#include "mir_type.h"
#include "mpl_sighandler.h"
#include "opcode_info.h"

using namespace maple;

std::unordered_set<std::string> dumpFuncSet = {};

#if MIR_FEATURE_FULL

int main(int argc, char **argv) {
  SigHandler::Enable();

  constexpr int judgeNumber = 2;
  constexpr uint32 k2Argv = 2;
  constexpr uint32 k10Argv = 10;
  constexpr uint32 kNlSize = 5;
  if (argc < judgeNumber) {
    (void)MIR_PRINTF(
        "usage: ./irbuild [-b] [-dumpfunc=<string>] [-srclang=<string>] <any number of .mplt, .mpl, .bpl, .mbc, "
        ".lmbc or .tmpl files>\n"
        "    By default, the files are converted to corresponding ascii format.\n"
        "    If -b is specified, output is binary format instead.\n"
        "    If -dumpfunc= is specified, only functions with name containing the string is output.\n"
        "    -dumpfunc= can be specified multiple times to give multiple strings.\n"
        "    -srclang specifies the source language that produces the mpl file. \n"
        "    Each output file has .irb added after its file stem.\n");
    exit(1);
  }

  std::vector<maple::MIRModule *> themodule(argc, nullptr);
  bool useBinary = false;
  MIRSrcLang srcLang = kSrcLangUnknown;
  // process the options which must come first
  maple::uint32 i = 1;
  while (argv[i][0] == '-') {
    if (argv[i][1] == 'b' && argv[i][k2Argv] == '\0') {
      useBinary = true;
    } else if (strncmp(argv[i], "-dumpfunc=", k10Argv) == 0 && strlen(argv[i]) > k10Argv) {
      std::string funcName(&argv[i][k10Argv]);
      dumpFuncSet.insert(funcName);
    } else if (strcmp(argv[i], "-srclang=java") == 0) {
      srcLang = kSrcLangJava;
    } else if (strcmp(argv[i], "-srclang=c") == 0) {
      srcLang = kSrcLangC;
    } else if (strcmp(argv[i], "-srclang=c++") == 0) {
      srcLang = kSrcLangCPlusPlus;
    } else {
      ERR(kLncErr, "irbuild: unrecognized command line option");
      return 1;
    }
    ++i;
  }
  // process the input files
  while (i < static_cast<uint32>(argc)) {
    themodule[i] = new maple::MIRModule(argv[i]);
    themodule[i]->SetSrcLang(srcLang);
    std::string::size_type lastdot = themodule[i]->GetFileName().find_last_of(".");
    bool ismplt = themodule[i]->GetFileName().compare(lastdot, kNlSize, ".mplt") == 0;
    bool istmpl = themodule[i]->GetFileName().compare(lastdot, kNlSize, ".tmpl") == 0;
    bool ismpl = themodule[i]->GetFileName().compare(lastdot, kNlSize, ".mpl") == 0;
    bool isbpl = themodule[i]->GetFileName().compare(lastdot, kNlSize, ".bpl") == 0;
    bool ismbc = themodule[i]->GetFileName().compare(lastdot, kNlSize, ".mbc") == 0;
    bool islmbc = themodule[i]->GetFileName().compare(lastdot, kNlSize, ".lmbc") == 0;
    if (!ismplt && !istmpl && !ismpl && !isbpl && !ismbc && !islmbc) {
      ERR(kLncErr, "irbuild: input must be .mplt or .mpl or .bpl or .mbc or .lmbc or .tmpl file");
      return 1;
    }
    // input the file
    if (ismpl || istmpl) {
      maple::MIRParser theparser(*themodule[i]);
      if (!theparser.ParseMIR()) {
        theparser.EmitError(themodule[i]->GetFileName().c_str());
        return 1;
      }
    } else {
      BinaryMplImport binMplt(*themodule[i]);
      binMplt.SetImported(false);
      std::string modid = themodule[i]->GetFileName();
      if (!binMplt.Import(modid, true)) {
        ERR(kLncErr, "irbuild: cannot open .mplt or .bpl or .mbc or .lmbc file: %s", modid.c_str());
        return 1;
      }
    }

    // output the file
    if (!useBinary) {
      themodule[i]->OutputAsciiMpl(
          ".irb", (ismpl || isbpl || ismbc || islmbc) ? ".mpl" : ".tmpl", &dumpFuncSet, true, false);
    } else {
      BinaryMplt binMplt(*themodule[i]);
      std::string modid = themodule[i]->GetFileName();
      binMplt.GetBinExport().not2mplt = ismpl || isbpl || ismbc || islmbc;
      std::string filestem = modid.substr(0, lastdot);
      binMplt.Export(filestem + ((ismpl || isbpl || ismbc || islmbc) ? ".irb.bpl" : ".irb.mplt"), &dumpFuncSet);
    }
    ++i;
  }
  return 0;
}
#else
#warning "this module is compiled without MIR_FEATURE_FULL=1 defined"
#endif  // MIR_FEATURE_FULL
