/*
 * Copyright (c) [2020-2022] Huawei Technologies Co., Ltd. All rights reserved.
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
#include <climits>

#include "bin_mplt.h"
#include "mir_function.h"
#include "mir_parser.h"
#include "opcode_info.h"
#include "triple.h"

using namespace maple;

namespace maple {
enum TargetIdEnum { kArm32, kAarch64, kX86, kAmd64, kVm, kRiscV64, kLastTargetId };

typedef struct {
  const char *name;
  int len;
  TargetIdEnum id;
} target_descr_t;

target_descr_t targetDescrs[kLastTargetId] = {
  { "aarch64", 7, kAarch64 }, { "arm", 3, kArm32 }, { "vm", 2, kVm }, { "x86_64", 6, kAmd64 }, { "x86", 3, kX86 },
  {"riscv64", 7, kRiscV64 },
};

TargetIdEnum target;

MIRModule *theModule = nullptr;
bool dumpit;

bool VerifyModule(MIRModule *module) {
  bool res = true;
  MapleList<MIRFunction *> &funcList = module->GetFunctionList();
  for (MapleList<MIRFunction *>::iterator it = funcList.begin(); it != funcList.end(); it++) {
    MIRFunction *curfun = *it;
    BlockNode *block = curfun->GetBody();
    module->SetCurFunction(curfun);
    if (dumpit) {
      curfun->Dump(false);
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

}  // namespace maple

static void usage(const char *pgm) {
  INFO(kLncInfo, "usage: %s <maple-file> [--dump] [--target=<target>]\n", pgm);
  exit(1);
}

int main(int argc, const char *argv[]) {
  if (argc < 2) {
    usage(argv[0]);
  }

  target = kAarch64;  // default

  const char *mirInfile = nullptr;
  for (int i = 1; i < argc; ++i) {
    if (!strncmp(argv[i], "--dump", 6)) {
      dumpit = true;
    } else if (!strncmp(argv[i], "--target=", 9)) {
      target = kLastTargetId;
      for (int j = 0; j < kLastTargetId; ++j) {
        target_descr_t &jt = targetDescrs[j];
        if (!strncmp(argv[i] + 9, jt.name, jt.len)) {
          target = jt.id;
          break;
        }
      }
      if (target == kLastTargetId) {
        usage(argv[0]);
      }
    } else if (argv[i][0] != '-') {
      mirInfile = argv[i];
    }
  }

  if (!mirInfile) {
    usage(argv[0]);
  }
  if (strlen(mirInfile) > PATH_MAX) {
    CHECK_FATAL(false, "invalid arg ");
  }
  Triple::GetTriple().Init();
  std::string filename(mirInfile);
  std::string::size_type lastdot = filename.find_last_of(".");
  bool isbpl = filename.compare(lastdot, 5, ".bpl\0") == 0;

  theModule = new MIRModule(mirInfile);
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
  if (!VerifyModule(theModule)) {
    exit(1);
  }
  return 0;
}
