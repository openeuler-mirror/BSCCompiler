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
#ifndef MAPLE_UTIL_INCLUDE_MPLSCHEDULER_H
#define MAPLE_UTIL_INCLUDE_MPLSCHEDULER_H

#include <vector>
#include <set>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <pthread.h>
#include <cinttypes>
#include <iostream>
#include <string>
#include "types_def.h"

namespace maple {
class MplTaskParam {
 public:
  MplTaskParam() = default;
  virtual ~MplTaskParam() = default;
};

class MplTask {
 public:
  MplTask() : taskId(0) {}

  virtual ~MplTask() = default;

  void SetTaskId(uint32 id) {
    taskId = id;
  }

  uint32 GetTaskId() const {
    return taskId;
  }

  int Run(MplTaskParam *param = nullptr) {
    return RunImpl(param);
  }

  int Finish(MplTaskParam *param = nullptr) {
    return FinishImpl(param);
  }

 protected:
  virtual int RunImpl(MplTaskParam*) {
    return 0;
  }

  virtual int FinishImpl(MplTaskParam*) {
    return 0;
  }

  uint32 taskId;
};

class MplSchedulerParam {
 public:
  MplSchedulerParam() = default;
  ~MplSchedulerParam() = default;
};

class MplScheduler {
 public:
  explicit MplScheduler(const std::string &name);
  virtual ~MplScheduler() = default;

  void Init();
  virtual void AddTask(MplTask &task);
  virtual int RunTask(uint32 threadsNum, bool seq = false);
  virtual MplSchedulerParam *EncodeThreadMainEnvironment(uint32) {
    return nullptr;
  }

  virtual void DecodeThreadMainEnvironment(MplSchedulerParam*) {}

  virtual MplSchedulerParam *EncodeThreadFinishEnvironment() {
    return nullptr;
  }

  virtual void DecodeThreadFinishEnvironment(MplSchedulerParam*) {}

  void GlobalLock() {
    pthread_mutex_lock(&mutexGlobal);
  }

  void GlobalUnlock() {
    pthread_mutex_unlock(&mutexGlobal);
  }

  void Reset();

 protected:
  std::string schedulerName;
  std::vector<MplTask*> tbTasks;
  std::set<uint32> tbTaskIdsToFinish;
  uint32 taskIdForAdd;
  uint32 taskIdToRun;
  uint32 taskIdExpected;
  uint32 numberTasks;
  uint32 numberTasksFinish;
  pthread_mutex_t mutexTaskIdsToRun;
  pthread_mutex_t mutexTaskIdsToFinish;
  pthread_mutex_t mutexTaskFinishProcess;
  pthread_mutex_t mutexGlobal;
  pthread_cond_t conditionFinishProcess;
  bool isSchedulerSeq;
  bool dumpTime;

  enum ThreadStatus {
    kThreadStop,
    kThreadRun,
    kThreadPause
  };

  ThreadStatus statusFinish;
  virtual int FinishTask(const MplTask &task);
  virtual MplTask *GetTaskToRun();
  virtual size_t GetTaskIdsFinishSize();
  virtual MplTask *GetTaskFinishFirst();
  virtual void RemoveTaskFinish(uint32 id);
  virtual void TaskIdFinish(uint32 id);
  void ThreadMain(uint32 threadID, MplSchedulerParam *env);
  void ThreadFinishNoSequence(MplSchedulerParam *env);
  void ThreadFinishSequence(MplSchedulerParam *env);
  void ThreadFinish(MplSchedulerParam *env);
  // Callback Function
  virtual void CallbackThreadMainStart() {}

  virtual void CallbackThreadMainEnd() {}

  virtual void CallbackThreadFinishStart() {}

  virtual void CallbackThreadFinishEnd() {}

  virtual MplTaskParam *CallbackGetTaskRunParam() const {
    return nullptr;
  }

  virtual MplTaskParam *CallbackGetTaskFinishParam() const {
    return nullptr;
  }
};
}  // namespace maple
#endif  // MAPLE_UTIL_INCLUDE_MPLSCHEDULER_H
