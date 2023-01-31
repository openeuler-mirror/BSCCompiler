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
#include "mpl_scheduler.h"
#include "mpl_timer.h"
#include "mpl_logging.h"
namespace maple {
constexpr double kAlternateUnits = 1000.0;
// ========== MplScheduler ==========
MplScheduler::MplScheduler(const std::string &name)
    : schedulerName(name),
      taskIdForAdd(0),
      taskIdToRun(0),
      taskIdExpected(0),
      numberTasks(0),
      numberTasksFinish(0),
      isSchedulerSeq(false),
      dumpTime(false),
      statusFinish(kThreadRun) {}

void MplScheduler::Init() {
  char *envStr = getenv("MP_DUMPTIME");
  if (envStr != nullptr && std::stoi(envStr) == 1) {
    dumpTime = true;
  }
  int ret = pthread_mutex_init(&mutexTaskIdsToRun, nullptr);
  CHECK_FATAL(ret == 0, "pthread_mutex_init failed");
  ret = pthread_mutex_init(&mutexTaskIdsToFinish, nullptr);
  CHECK_FATAL(ret == 0, "pthread_mutex_init failed");
  ret = pthread_mutex_init(&mutexGlobal, nullptr);
  CHECK_FATAL(ret == 0, "pthread_mutex_init failed");
  mutexTaskFinishProcess = PTHREAD_MUTEX_INITIALIZER;
  conditionFinishProcess = PTHREAD_COND_INITIALIZER;
}

void MplScheduler::AddTask(MplTask &task) {
  task.SetTaskId(taskIdForAdd);
  tbTasks.push_back(&task);
  ++taskIdForAdd;
  ++numberTasks;
}

MplTask *MplScheduler::GetTaskToRun() {
  MplTask *task = nullptr;
  int ret = pthread_mutex_lock(&mutexTaskIdsToRun);
  CHECK_FATAL(ret == 0, "pthread_mutex_lock failed");
  if (taskIdToRun < numberTasks) {
    task = tbTasks[taskIdToRun++];
  }
  ret = pthread_mutex_unlock(&mutexTaskIdsToRun);
  CHECK_FATAL(ret == 0, "pthread_mutex_unlock failed");
  return task;
}

size_t MplScheduler::GetTaskIdsFinishSize() {
  int ret = pthread_mutex_lock(&mutexTaskIdsToFinish);
  CHECK_FATAL(ret == 0, "pthread_mutex_lock failed");
  size_t size = tbTaskIdsToFinish.size();
  ret = pthread_mutex_unlock(&mutexTaskIdsToFinish);
  CHECK_FATAL(ret == 0, "pthread_mutex_unlock failed");
  return size;
}

MplTask *MplScheduler::GetTaskFinishFirst() {
  MplTask *task = nullptr;
  int ret = pthread_mutex_lock(&mutexTaskIdsToFinish);
  CHECK_FATAL(ret == 0, "pthread_mutex_lock failed");
  if (!tbTaskIdsToFinish.empty()) {
    task = tbTasks[*(tbTaskIdsToFinish.begin())];
  }
  ret = pthread_mutex_unlock(&mutexTaskIdsToFinish);
  CHECK_FATAL(ret == 0, "pthread_mutex_unlock failed");
  return task;
}

void MplScheduler::RemoveTaskFinish(uint32 id) {
  int ret = pthread_mutex_lock(&mutexTaskIdsToFinish);
  CHECK_FATAL(ret == 0, "pthread_mutex_lock failed");
  tbTaskIdsToFinish.erase(id);
  ret = pthread_mutex_unlock(&mutexTaskIdsToFinish);
  CHECK_FATAL(ret == 0, "pthread_mutex_unlock failed");
}

void MplScheduler::TaskIdFinish(uint32 id) {
  int ret = pthread_mutex_lock(&mutexTaskIdsToFinish);
  CHECK_FATAL(ret == 0, "pthread_mutex_lock failed");
  tbTaskIdsToFinish.insert(id);
  ret = pthread_mutex_unlock(&mutexTaskIdsToFinish);
  CHECK_FATAL(ret == 0, "pthread_mutex_unlock failed");
}

int MplScheduler::RunTask(uint32 threadsNum, bool seq) {
  isSchedulerSeq = seq;
  if (threadsNum > 0) {
    taskIdExpected = 0;
    std::thread threads[threadsNum];
    std::thread threadFinish;
    for (uint32 i = 0; i < threadsNum; ++i) {
      threads[i] = std::thread(&MplScheduler::ThreadMain, this, i, EncodeThreadMainEnvironment(i));
    }
    if (isSchedulerSeq) {
      threadFinish = std::thread(&MplScheduler::ThreadFinish, this, EncodeThreadFinishEnvironment());
    }
    for (uint32 i = 0; i < threadsNum; ++i) {
      threads[i].join();
    }
    if (isSchedulerSeq) {
      threadFinish.join();
    }
  }
  return 0;
}

int MplScheduler::FinishTask(const MplTask &task) {
  TaskIdFinish(task.GetTaskId());
  return 0;
}

void MplScheduler::Reset() {
  tbTasks.clear();
  tbTaskIdsToFinish.clear();
  taskIdForAdd = 0;
  taskIdToRun = 0;
  taskIdExpected = 0;
  numberTasks = 0;
  numberTasksFinish = 0;
  isSchedulerSeq = false;
}

void MplScheduler::ThreadMain(uint32 threadID, MplSchedulerParam *env) {
  MPLTimer timerTotal;
  MPLTimer timerRun;
  MPLTimer timerToRun;
  MPLTimer timerFinish;
  double timeRun = 0.0;
  double timeToRun = 0.0;
  double timeFinish = 0.0;
  if (dumpTime) {
    timerTotal.Start();
  }
  DecodeThreadMainEnvironment(env);
  CallbackThreadMainStart();
  if (dumpTime) {
    timerToRun.Start();
  }
  MplTask *task = GetTaskToRun();
  if (dumpTime) {
    timerToRun.Stop();
    timeToRun += timerRun.ElapsedMicroseconds();
  }
  int ret = 0;
  while (task != nullptr) {
    if (dumpTime) {
      timerRun.Start();
    }
    MplTaskParam *paramRun = CallbackGetTaskRunParam();
    (void)task->Run(paramRun);
    if (dumpTime) {
      timerRun.Stop();
      timeRun += timerRun.ElapsedMicroseconds();
      timerFinish.Start();
    }
    if (isSchedulerSeq) {
      ret = FinishTask(*task);
      CHECK_FATAL(ret == 0, "task finish failed");
    } else {
      MplTaskParam *paramFinish = CallbackGetTaskFinishParam();
      ret = task->Finish(paramFinish);
      CHECK_FATAL(ret == 0, "task finish failed");
    }
    if (dumpTime) {
      timerFinish.Stop();
      timeFinish += timerFinish.ElapsedMicroseconds();
      timerToRun.Start();
    }
    task = GetTaskToRun();
    if (dumpTime) {
      timerToRun.Stop();
      timeToRun += timerToRun.ElapsedMicroseconds();
    }
  }
  CallbackThreadMainEnd();
  if (dumpTime) {
    timerTotal.Stop();
    GlobalLock();
    LogInfo::MapleLogger() << "MP TimeDump(" << schedulerName << ")::Thread" << threadID << "::ThreadMain " <<
        (timerTotal.ElapsedMicroseconds() / kAlternateUnits) << "ms" << std::endl;
    LogInfo::MapleLogger() << "MP TimeDump(" << schedulerName << ")::Thread" << threadID <<
        "::ThreadMain::Task::Run " << (timeRun / kAlternateUnits) << "ms" << std::endl;
    LogInfo::MapleLogger() << "MP TimeDump(" << schedulerName << ")::Thread" << threadID <<
        "::ThreadMain::Task::ToRun " << (timeToRun / kAlternateUnits) << "ms" << std::endl;
    LogInfo::MapleLogger() << "MP TimeDump(" << schedulerName << ")::Thread" << threadID <<
        "::ThreadMain::Task::Finish " << (timeFinish / kAlternateUnits) << "ms" << std::endl;
    GlobalUnlock();
  }
}

void MplScheduler::ThreadFinish(MplSchedulerParam *env) {
  statusFinish = kThreadRun;
  DecodeThreadFinishEnvironment(env);
  CallbackThreadFinishStart();
  MplTask *task = nullptr;
  int ret = 0;
  uint32 taskId;
  while (numberTasksFinish < numberTasks) {
    while (true) {
      if (GetTaskIdsFinishSize() == 0) {
        break;
      }
      task = GetTaskFinishFirst();
      CHECK_FATAL(task != nullptr, "null ptr check");
      taskId = task->GetTaskId();
      if (isSchedulerSeq) {
        if (taskId != taskIdExpected) {
          break;
        }
      }
      MplTaskParam *paramFinish = CallbackGetTaskFinishParam();
      ret = task->Finish(paramFinish);
      CHECK_FATAL(ret == 0, "task finish failed");
      ++numberTasksFinish;
      if (isSchedulerSeq) {
        ++taskIdExpected;
      }
      RemoveTaskFinish(task->GetTaskId());
    }
  }
  CallbackThreadFinishEnd();
}
}  // namespace maple