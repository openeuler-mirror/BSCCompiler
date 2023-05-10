/*
 * Copyright (c) [2022] Huawei Technologies Co., Ltd. All rights reserved.
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

#include "bin_mplt.h"
#include "mir_function.h"
#include "mir_parser.h"
#include "opcode_info.h"
#include "triple.h"

using namespace maple;

static bool dumpit;

static bool VerifyModule(MIRModule &module) {
  bool res = true;
  for (MIRFunction *curfunc : module.GetFunctionList()) {
    BlockNode *block = curfunc->GetBody();
    module.SetCurFunction(curfunc);
    if (dumpit) {
      curfunc->Dump(false);
    }
    if (!block) {
      continue;
    }
    if (!block->Verify()) {
      res = false;
    }
  }
  return res;
}

static void Usage(const char *pgm) {
  INFO(kLncInfo, "usage: %s <maple-file> [--dump]\n", pgm);
}

int main(int argc, const char *argv[]) {
  constexpr int32 minArgCnt = 2;
  constexpr int32 dumpNum = 6;
  if (argc < minArgCnt) {
    Usage(argv[0]);
    return 1;
  }

  const char *mirInfile = nullptr;
  for (int i = 1; i < argc; ++i) {
    if (strncmp(argv[i], "--dump", dumpNum) == 0) {
      dumpit = true;
    } else if (argv[i][0] != '-') {
      mirInfile = argv[i];
    }
  }

  if (!mirInfile) {
    Usage(argv[0]);
    return 1;
  }
  Triple::GetTriple().Init();
  std::string filename(mirInfile);
  std::string::size_type lastdot = filename.find_last_of(".");
  bool isbpl = filename.compare(lastdot, std::string::npos, ".bpl") == 0;

  MIRModule *theModule = new MIRModule(mirInfile);
  theMIRModule = theModule;
  if (!isbpl) {
    maple::MIRParser theparser(*theModule);
    if (!theparser.ParseMIR()) {
      theparser.EmitError(filename);
      return 1;
    }
  }
  else {
    BinaryMplImport binMplt(*theModule);
    binMplt.SetImported(false);
    if (!binMplt.Import(filename, true)) {
      FATAL(kLncFatal, "cannot open .bpl file: %s", mirInfile);
      return 1;
    }
  }
  if (!VerifyModule(*theModule)) {
    return 1;
  }
  return 0;
}
