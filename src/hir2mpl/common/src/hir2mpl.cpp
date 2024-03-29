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
#include "fe_utils.h"
#include "hir2mpl_compiler.h"
#include "mpl_sighandler.h"
using namespace maple;

int main(int argc, char **argv) {
  SigHandler::Enable();

  MPLTimer timer;
  timer.Start();
  HIR2MPLOptions &options = HIR2MPLOptions::GetInstance();
  if (!options.SolveArgs(argc, argv)) {
    return static_cast<int>(FEErrno::kCmdParseError);
  }
  HIR2MPLEnv::GetInstance().Init();
  MIRModule module;
  HIR2MPLCompiler compiler(module);
  int res = compiler.Run();
  timer.Stop();
  if (FEOptions::GetInstance().IsDumpTime()) {
    INFO(kLncInfo, "hir2mpl time: %.2lfms", timer.ElapsedMilliseconds() / 1.0);
  }
  return res;
}
