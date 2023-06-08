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
#ifndef MAPLE_ME_INCLUDE_ME_FUNC_OPT_H
#define MAPLE_ME_INCLUDE_ME_FUNC_OPT_H
#include <memory>
#include "me_phase_manager.h"

namespace maple {
#ifdef NOT_USED
class MeFuncOptTask : public MplTask {
 public:
  MeFuncOptTask(MIRModule &mod, MIRFunction &func, size_t rangeNum, const std::string &meInput)
      : module(mod), mirFunc(func), rangeNum(rangeNum), meInput(meInput) {}
  ~MeFuncOptTask() override = default;
  int RunImpl(MplTaskParam *param) override;

 private:
  MIRModule &module;
  MIRFunction &mirFunc;
  size_t rangeNum;
  const std::string &meInput;
};

class MeFuncOptExecutor : public MplTaskParam {
 public:
  MeFuncOptExecutor(std::unique_ptr<MemPoolCtrler> mpCtrler, MeFuncPhaseManager &fpm)
      : mpCtrler(std::move(mpCtrler)), fpm(fpm) {}

  ~MeFuncOptExecutor() = default;

  std::unique_ptr<MeFuncOptExecutor> Clone() const;
  void ProcessRun(MIRFunction &mirFunc, size_t rangeNum, const std::string &meInput);

 private:
  std::unique_ptr<MemPoolCtrler> mpCtrler;
  MeFuncPhaseManager &fpm;
};

class MeFuncOptScheduler : public MplScheduler {
 public:
  MeFuncOptScheduler(const std::string &name, std::unique_ptr<MeFuncOptExecutor> funcOpt)
      : MplScheduler(name), funcOpt(std::move(funcOpt)) {}

  ~MeFuncOptScheduler() override {}

  void CallbackThreadMainStart() override;
  void CallbackThreadMainEnd() override;
  void AddFuncOptTask(MIRModule &module, MIRFunction &mirFunc, size_t rangeNum, const std::string &meInput);

 protected:
  MplTaskParam *CallbackGetTaskRunParam() const override;
  MplTaskParam *CallbackGetTaskFinishParam() const override;

 private:
  thread_local static std::unique_ptr<MeFuncOptExecutor> funcOptLocal;
  std::unique_ptr<MeFuncOptExecutor> funcOpt;
  std::vector<std::unique_ptr<MplTask>> tasksUniquePtr;
};
#endif
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_FUNC_OPT_H
