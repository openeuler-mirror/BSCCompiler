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
#include "phase_driver.h"
#include "mpl_timer.h"

namespace maple {
constexpr long kAlternateUnits = 1000;
thread_local PhaseDriverImpl *PhaseDriver::phaseImplLocal = nullptr;
PhaseDriver::PhaseDriver(const std::string &phaseName)
    : MplScheduler(phaseName), module(nullptr), phaseImpl(nullptr), phaseName(phaseName) {}

void PhaseDriver::RunAll(MIRModule *currModule, int thread, bool bSeq) {
  module = currModule;
  phaseImpl = NewPhase();
  CHECK_FATAL(phaseImpl != nullptr, "null ptr check");
  phaseImpl->GlobalInit();
  if (thread == 1) {
    RunSerial();
  } else {
    RunParallel(thread, bSeq);
  }
  delete phaseImpl;
  phaseImpl = nullptr;
}

void PhaseDriver::RunSerial() {
  phaseImplLocal = NewPhase();
  CHECK_FATAL(phaseImplLocal != nullptr, "null ptr check");
  phaseImplLocal->LocalInit();
  MPLTimer timer;
  if (dumpTime) {
    timer.Start();
  }
  RegisterTasks();
  if (dumpTime) {
    timer.Stop();
    INFO(kLncInfo, "PhaseDriver::RegisterTasks (%s): %lf ms",
         phaseName.c_str(), timer.ElapsedMicroseconds() / kAlternateUnits);
    timer.Start();
  }
  MplTask *task = GetTaskToRun();
  while (task != nullptr) {
    MplTaskParam *paramRun = CallbackGetTaskRunParam();
    (void)task->Run(paramRun);
    MplTaskParam *paramFinish = CallbackGetTaskFinishParam();
    (void)task->Run(paramFinish);
  }
  if (dumpTime) {
    timer.Stop();
    INFO(kLncInfo, "PhaseDriver::RunTask (%s): %lf ms",
         phaseName.c_str(), timer.ElapsedMicroseconds() / kAlternateUnits);
  }
}

void PhaseDriver::RunParallel(int thread, bool bSeq) {
  MPLTimer timer;
  if (dumpTime) {
    timer.Start();
  }
  RegisterTasks();
  if (dumpTime) {
    timer.Stop();
    INFO(kLncInfo, "PhaseDriver::RegisterTasks (%s): %lf ms",
         phaseName.c_str(), timer.ElapsedMicroseconds() / kAlternateUnits);
  }
  if (dumpTime) {
    timer.Start();
  }
  int ret = RunTask(static_cast<uint32>(thread), bSeq);
  CHECK_FATAL(ret == 0, "RunTask failed");
  if (dumpTime) {
    timer.Stop();
    INFO(kLncInfo, "PhaseDriver::RunTask (%s): %lf ms",
         phaseName.c_str(), timer.ElapsedMicroseconds() / kAlternateUnits);
  }
}

void PhaseDriver::CallbackThreadMainStart() {
  phaseImplLocal = NewPhase();
  CHECK_FATAL(phaseImplLocal != nullptr, "null ptr check");
  phaseImplLocal->LocalInit();
}

void PhaseDriver::CallbackThreadMainEnd() {
  delete phaseImplLocal;
  phaseImplLocal = nullptr;
}
}  // namespace maple
