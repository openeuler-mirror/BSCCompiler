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
#include "me_func_opt.h"

namespace maple {
#ifdef NOT_USED
thread_local std::unique_ptr<MeFuncOptExecutor> MeFuncOptScheduler::funcOptLocal;

int MeFuncOptTask::RunImpl(MplTaskParam *param) {
  module.SetCurFunction(&mirFunc);
  auto *optExe = static_cast<MeFuncOptExecutor*>(param);
  CHECK_NULL_FATAL(optExe);
  optExe->ProcessRun(mirFunc, rangeNum, meInput);
  return 0;
}

std::unique_ptr<MeFuncOptExecutor> MeFuncOptExecutor::Clone() const {
  // thread local memPoolCtrler and fpm
  auto threadLocalCtrler = std::make_unique<MemPoolCtrler>();
  auto *memPool = threadLocalCtrler->NewMemPool("thread local func opt mempool", true /* isLcalPool */);
  MeFuncPhaseManager &fpmCopy = fpm.Clone(*memPool, *threadLocalCtrler);
  auto copy = std::make_unique<MeFuncOptExecutor>(std::move(threadLocalCtrler), fpmCopy);
  return copy;
}

void MeFuncOptExecutor::ProcessRun(MIRFunction &mirFunc, size_t rangeNum, const std::string &meInput) {
  fpm.Run(&mirFunc, rangeNum, meInput, *mpCtrler);
}

void MeFuncOptScheduler::CallbackThreadMainStart() {
  ThreadEnv::InitThreadIndex(std::this_thread::get_id());
  funcOptLocal = funcOpt->Clone();
}

void MeFuncOptScheduler::CallbackThreadMainEnd() {
  funcOptLocal = nullptr;
}

MplTaskParam *MeFuncOptScheduler::CallbackGetTaskRunParam() const {
  return funcOptLocal.get();
}

MplTaskParam *MeFuncOptScheduler::CallbackGetTaskFinishParam() const {
  return funcOptLocal.get();
}

void MeFuncOptScheduler::AddFuncOptTask(MIRModule &module, MIRFunction &mirFunc,
                                        size_t rangeNum, const std::string &meInput) {
  std::unique_ptr<MeFuncOptTask> task = std::make_unique<MeFuncOptTask>(module, mirFunc, rangeNum, meInput);
  AddTask(task.get());
  tasksUniquePtr.emplace_back(std::move(task));
}
#endif
}  // namespace maple
