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
#ifndef MAPLE_PHASE_INCLUDE_PHASEDRIVER_H
#define MAPLE_PHASE_INCLUDE_PHASEDRIVER_H
#include "mir_module.h"
#include "mir_function.h"
#include "mpl_scheduler.h"

namespace maple {
class PhaseDriverImpl : public MplTaskParam {
 public:
  PhaseDriverImpl() = default;
  virtual ~PhaseDriverImpl() = default;

  virtual void GlobalInit() {}

  virtual void LocalInit() {}

  virtual void ProcessRun(uint32, void*, void*) {}

  virtual void ProcessFinish(uint32, void*, void*) {}
};

class PhaseDriver : public MplScheduler {
 public:
  class Task : public MplTask {
   public:
    explicit Task(void *currTarget, void *currParamEx = nullptr) : target(currTarget), paramException(currParamEx) {}

    ~Task() = default;

   protected:
    int RunImpl(MplTaskParam *param) override {
      CHECK_NULL_FATAL(param);
      static_cast<PhaseDriverImpl*>(param)->ProcessRun(taskId, target, paramException);
      return 0;
    }

    int FinishImpl(MplTaskParam *param) override {
      CHECK_NULL_FATAL(param);
      static_cast<PhaseDriverImpl*>(param)->ProcessFinish(taskId, target, paramException);
      return 0;
    }
    void *target;
    void *paramException;
  };

  explicit PhaseDriver(const std::string &phaseName);
  virtual ~PhaseDriver() = default;

  virtual void RunAll(MIRModule *currModule, int thread, bool bSeq = false);
  virtual void RunSerial();
  virtual void RunParallel(int thread, bool bSeq = false);
  virtual PhaseDriverImpl *NewPhase() = 0;
  virtual void RegisterTasks() = 0;

 protected:
  virtual void CallbackThreadMainStart();
  virtual void CallbackThreadMainEnd();
  virtual MplTaskParam *CallbackGetTaskRunParam() const {
    return phaseImplLocal;
  }

  virtual MplTaskParam *CallbackGetTaskFinishParam() const {
    return phaseImplLocal;
  }

  MIRModule *module;
  PhaseDriverImpl *phaseImpl;
  thread_local static PhaseDriverImpl *phaseImplLocal;
  std::string phaseName;
};
}  // namespace maple
#endif  // MAPLE_PHASE_INCLUDE_PHASEDRIVER_H
