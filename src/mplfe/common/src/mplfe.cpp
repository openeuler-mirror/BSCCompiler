/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "mplfe_compiler.h"
using namespace maple;

int main(int argc, char **argv) {
  MPLTimer timer;
  timer.Start();
  MPLFEOptions &options = MPLFEOptions::GetInstance();
  if (options.SolveArgs(argc, argv) == false) {
    return static_cast<int>(FEErrno::kCmdParseError);
  }
  MPLFEEnv::GetInstance().Init();
  MIRModule module;
  MPLFECompiler compiler(module);
  compiler.Run();
  timer.Stop();
  if (FEOptions::GetInstance().IsDumpTime()) {
    INFO(kLncInfo, "mplfe time: %.2lfms", timer.ElapsedMilliseconds() / 1.0);
  }
  return static_cast<int>(FEErrno::kNoError);
}
