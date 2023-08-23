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
#ifndef COLLIE_H
#define COLLIE_H

#ifdef __ANDROID__
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cinttypes>
#include <vector>
#include <deque>
#include <atomic>
#include <mutex>
#include <chrono>
#include <sys/types.h>
#include <sys/syscall.h>
#include <time.h>
#include "mrt_api_common.h"
#include "panic.h"
#include "mutator_list.h"

namespace maplert {
constexpr int kMplWaitCheckInterval = 5;
const int kMaxDelayCount = 3;
constexpr unsigned int kTimerRingCheckInterval = 1;
const int kMplCollieCallbackHistoryMax = 5;
const int kMplCollieCallbackTimewinMax = 60;
const int kInvalidId = -1;

constexpr int kMplWaitHeavyGcTimeout = 90; // other threads wait for gc finish
constexpr int kMplWaitGcTimeout = 60; // normal gc timeout
constexpr int kMplFinalizerTimeout = 30;
constexpr int kThreadDumpTimeout = 40;
constexpr int kMplCollieMaxRecordVal = 10;

#define MPLCOLLIE_FLAG_ABORT (1 << 0)
#define MPLCOLLIE_FLAG_PROMOTE_PRIORITY (1 << 7)
#define MPLCOLLIE_FLAG_IN_STW (1 << 8)
#define MPLCOLLIE_FLAG_FOR_STW (1 << 9)
#define MPLCOLLIE_FLAG_IS_STW(flag) (((flag) & MPLCOLLIE_FLAG_IN_STW) || ((flag) & MPLCOLLIE_FLAG_FOR_STW))

enum CollieType {
  kGCCollie = 0, // wait for GC finish
  kProcessFinalizeCollie, // reference processor trigger
  kSTWCollie, // wait for stop the world finish
  kThreadDumpCollie,
  kCollieTypeMax
};

struct CollieNode {
  CollieType type;
  bool isUsed;
  std::string name;
  time_t startTime;
  int timeout;
  int promoteTimes;
  pid_t tid;
  void *arg;
  uint32_t flag;
  void (*callback)(void *args);
  CollieNode *next = nullptr;
  void Reset() {
    next = nullptr;
  }
};

class CollieList {
 public:
  CollieList() : head(nullptr) {}

  void Init() {
    head = nullptr;
  }

  void Insert(CollieNode &target) {
    if (head == nullptr) {
      head = &target;
      target.Reset();
      return;
    }
    CollieNode *prev = nullptr;
    CollieNode *cur = head;
    while (cur != nullptr) {
      if (target.timeout >= cur->timeout) {
        prev = cur;
        cur = cur->next;
        continue;
      }
      if (prev == nullptr) {
        head = &target;
        head->next = cur;
      } else {
        prev->next = &target;
        target.next = cur;
      }
      return;
    }
    prev->next = &target;
    target.Reset();
  }

  void Remove(CollieNode &target) {
    __MRT_ASSERT(head != nullptr, "remove a node from a empty list");
    CollieNode *prev = nullptr;
    CollieNode *cur = head;
    while (cur != nullptr) {
      if (cur != &target) {
        prev = cur;
        cur = cur->next;
        continue;
      }
      if (prev == nullptr) {
        head = cur->next;
        target.Reset();
      } else {
        prev->next = cur->next;
        target.Reset();
      }
      return;
    }
    __MRT_ASSERT(cur != nullptr, "collie list can not find a target node");
  }

  CollieNode *GetHead() const {
    return head;
  }

  bool IsEmpty() {
    return head == nullptr;
  }
 private:
  CollieNode *head;
};

struct CollieTimerRing {
  CollieList cl;
  int timer;
};

class MplCollie {
 public:
  void Init();
  void Fini();

  MRT_EXPORT int Start(CollieType type, int flag, pid_t tid = maple::GetTid(),
                       void (*func)(void*) = nullptr, void *arg = nullptr);
  MRT_EXPORT void End(int type);

  void StartSTW() {
    Start(kSTWCollie, MPLCOLLIE_FLAG_ABORT | MPLCOLLIE_FLAG_FOR_STW);
  }

  void EndSTW() {
    End(kSTWCollie);
  }

  void JoinThread();
  MRT_EXPORT void SetSTWPanic(bool enable);
  MRT_EXPORT bool GetSTWPanic(void);

  MRT_EXPORT void FatalPanic(std::string &s, int tid);
  void FatalPanicStopTheWorld(std::string &msg);

 private:
  int CallbackShouldLimit(int flag);
  void CheckTimerRing(CollieNode callbackList[], CollieTimerRing &r, int &count);
  static void *CollieThreadEntry(void *arg);
  void *CollieThreadHandle();
  void CollieTrySleep(void);
  void CollieTryWake(void);
  void FatalPanicLocked(std::string &msg);
  void InitImp();
  void ResetNode(CollieNode &node, int seq);
  void RunCallback(CollieNode &cb);
  void TimerRingTimeout();
  void ForceEnd();
  pthread_t collieThread;

  int targetTid = kInvalidId;

  int stwID = kInvalidId;
  bool runnable = false; // false means the collie thread should stop.
  std::mutex initMutex;
  std::mutex listMutex;
  std::mutex panicMutex;
  // used for timing.
  std::mutex timerMutex;
  std::condition_variable timerCond;
  // call back count
  unsigned int nrCallback = 0;

  // call back time stamp
  time_t timeCallback = 0;

  // to store the time
  CollieTimerRing collieTimerRingSTW;
  CollieTimerRing collieTimerRingNonSTW;

  // save all the monitor info
  CollieNode collieNodesArray[kCollieTypeMax];

  struct TimerRingSleepControl {
    bool threadInSleep;
    std::atomic<unsigned int> delayCount;
    std::unique_ptr<std::mutex> sleepMutexPtr;
    std::unique_ptr<std::condition_variable> sleepCond;
  } trCtl;
};
#ifdef __ANDROID__
extern MRT_EXPORT MplCollie mplCollie;
#endif

class MplCollieScope {
 public:
  MplCollieScope(CollieType type, int flag, pid_t tid = maple::GetTid(), void (*func)(void*) = nullptr,
                 void *arg = nullptr) {
    mplCollieType = mplCollie.Start(type, flag, tid, func, arg);
  }
  ~MplCollieScope() {
    mplCollie.End(mplCollieType);
  }
 private:
  int mplCollieType = kInvalidId;
};
}
#endif // __ANDROID__
#endif // COLLIE_H
