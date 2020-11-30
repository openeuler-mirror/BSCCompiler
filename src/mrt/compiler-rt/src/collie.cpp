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
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifdef __ANDROID__
#include "collie.h"

#include <cstdio>
#include <pthread.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <semaphore.h>
#include <cerrno>
#include <cstring>
#include <string>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <android/set_abort_message.h>

#undef LOG_TAG
#define LOG_TAG "MAPLECOLLIE"
#include <log/log.h>

#define USE_LIBC_SEC
#ifdef USE_LIBC_SEC
#include "securec.h"
#endif
#include "chelper.h"

#define ID_IS_INVALID(x) (UNLIKELY(((x) >= kCollieTypeMax) || ((x) < 0)))

namespace maplert {
MplCollie mplCollie;

// let monitor thread try to sleep
// it will sleep if count to kMaxDelayCount and no request from the watched threads
void MplCollie::CollieTrySleep(void) {
  if (trCtl.delayCount.load() >= kMaxDelayCount) {
    std::unique_lock<std::mutex> lock(*trCtl.sleepMutexPtr.get());
    trCtl.threadInSleep = true;
    trCtl.sleepCond->wait(lock, [this] { return !(trCtl.threadInSleep && runnable); });
    trCtl.delayCount = 0;
    collieTimerRingSTW.timer = 0;
    collieTimerRingNonSTW.timer = 0;
  }
}

// initialize a monitor node
void MplCollie::ResetNode(CollieNode &node, int type) {
  int savedType = node.type;
  if (memset_s(&node, sizeof(CollieNode), 0, sizeof(CollieNode)) != EOK) {
    LOG(FATAL) << "ResetNode memset_s not return 0" << maple::endl;
  }
  node.type = CollieType(type);
  node.isUsed = false;
  switch (type) {
    case kGCCollie: {
      node.name = "WaitForGCFinish Collie";
      node.timeout = kMplWaitCheckInterval;
      break;
    }
    case kProcessFinalizeCollie: {
      node.name = "Finalizer Collie";
      node.timeout = kMplFinalizerTimeout;
      break;
    }
    case kSTWCollie: {
      node.name = "StopTheWorld Collie";
      node.timeout = kMplCollieMaxRecordVal;
      break;
    }
    case kThreadDumpCollie: {
      node.name = "ThreadDump Collie";
      node.timeout = kThreadDumpTimeout;
      break;
    }
    default: {
      node.type = CollieType(savedType);
    }
  }
  int maxPromoteTimeout = ((VLOG_IS_ON(dumpgarbage) || VLOG_IS_ON(dumpheapbeforegc) || VLOG_IS_ON(dumpheapaftergc) ||
                            VLOG_IS_ON(rcverify))) ? kMplWaitHeavyGcTimeout : kMplWaitGcTimeout;
  node.promoteTimes = maxPromoteTimeout / kMplWaitCheckInterval;
  node.Reset();
}

int MplCollie::CallbackShouldLimit(int flag ATTR_UNUSED) {
  int ret = 0;
  time_t now;

  now = time(nullptr);
  if (timeCallback + kMplCollieCallbackTimewinMax < now) {
    timeCallback = now;
  } else {
    if (++nrCallback > kMplCollieCallbackHistoryMax) {
      ret = 1;
    }
  }

  return ret;
}

void MplCollie::FatalPanicLocked(std::string &msg) {
  if (VLOG_IS_ON(dumpgarbage) || VLOG_IS_ON(dumpheapbeforegc) || VLOG_IS_ON(dumpheapaftergc) || VLOG_IS_ON(rcverify)) {
    LOG(ERROR) << "dump heap switch or verfiy rc is on, skip maple collie panic";
    return;
  }
#if CONFIG_JSAN
  LOG(ERROR) << msg << maple::endl;
  LOG(ERROR) << "== Skip abort in JSAN version ==";
#else
  android_set_abort_message(msg.c_str());
  sleep(2);
  abort();
#endif
}

void MplCollie::FatalPanicStopTheWorld(std::string &msg) {
  std::unique_lock<std::mutex> lock(panicMutex);

  std::string dumpMsg;
  if (!ID_IS_INVALID(stwID)) {
    targetTid = collieNodesArray[stwID].tid;
  } else {
    targetTid = static_cast<int>(maple::GetTid());
    dumpMsg = "cannot find stw id\n";
  }
  maplert::MutatorList::Instance().VisitMutators([this, &dumpMsg](Mutator *mutator) {
    if (!mutator->InSaferegion()) {
      targetTid = static_cast<int>(mutator->GetTid());
      dumpMsg = dumpMsg + " not in saferegion : " + std::to_string(targetTid);
    }
  });
  dumpMsg = "tid: " + std::to_string(targetTid) + "\n" + msg + dumpMsg;
  FatalPanicLocked(dumpMsg);
}

static void PromoteThreadPriority(pid_t tid) {
  errno = 0;
  int32_t priority = getpriority(static_cast<int>(PRIO_PROCESS), tid);
  if (UNLIKELY(errno != 0)) {
    char errMsg[maple::kMaxStrErrorBufLen];
    (void)strerror_r(errno, errMsg, sizeof(errMsg));
    LOG(ERROR) << "getpriority() in failed with errno " << errno << ": " << errMsg;
    return;
  }
  if (priority + maple::kPriorityPromoteStep > maple::kGCThreadStwPriority) {
    priority += maple::kPriorityPromoteStep;
  } else if (priority > maple::kGCThreadStwPriority) {
    priority = maple::kGCThreadStwPriority;
  } else {
    return;
  }
  MRT_SetThreadPriority(tid, priority);
}

void MplCollie::FatalPanic(std::string &msg, int tid) {
  std::unique_lock<std::mutex> lock(panicMutex);
  msg = "tid: " + std::to_string(tid) + "\n" + msg;
  FatalPanicLocked(msg);
}

// trigger callback and caller-defined callback
void MplCollie::RunCallback(CollieNode &cb) {
  if (CallbackShouldLimit(cb.flag)) {
    LOG(ERROR) << "Too many callback triggerd in a short time!" << maple::endl;
    return;
  }

  if (cb.callback) {
    cb.callback(cb.arg);
  }

  if (static_cast<uint32_t>(cb.flag) & MPLCOLLIE_FLAG_ABORT) {
    std::string msg = cb.name;
    msg += " took too long: from ";
    msg += std::to_string(cb.startTime);
    msg += " to ";
    msg += std::to_string(time(nullptr));
    if (MPLCOLLIE_FLAG_IS_STW(static_cast<uint32_t>(cb.flag))) {
      FatalPanicStopTheWorld(msg);
    } else {
      FatalPanic(msg, cb.tid);
    }
  }
}

void MplCollie::CheckTimerRing(CollieNode callbackList[], CollieTimerRing &r, int &count) {
  r.timer += kTimerRingCheckInterval;
  CollieNode *cur = r.cl.GetHead();
  while (cur != nullptr && r.timer > cur->timeout) {
    CollieNode *node = cur;
    cur = cur->next;
    r.cl.Remove(*node);
    if ((static_cast<uint32_t>(node->flag) & MPLCOLLIE_FLAG_PROMOTE_PRIORITY) && node->promoteTimes > 0) {
      PromoteThreadPriority(node->tid);
      node->timeout = r.timer + kMplWaitCheckInterval;
      --(node->promoteTimes);
      r.cl.Insert(*node);
    } else {
      if (stwID == node->type) {
        stwID = kInvalidId;
      } else {
        callbackList[count] = *node;
        ++count;
      }
      int type = node->type;
      ResetNode(*node, type);
    }
  }
}

// check the current timer ring position and run all the callbacks
void MplCollie::TimerRingTimeout() {
  CollieNode callbackList[kCollieTypeMax];

  int count = 0;
  {
    std::unique_lock<std::mutex> lock(listMutex);
    bool isEmpty = true;
    // point to the current list timeout in timer ring
    if (!collieTimerRingSTW.cl.IsEmpty()) {
      CheckTimerRing(callbackList, collieTimerRingSTW, count);
      isEmpty = false;
    }
    // if in stop the world, then stop the nonstw timer walking
    if (ID_IS_INVALID(stwID)) {
      if (!collieTimerRingNonSTW.cl.IsEmpty()) {
        CheckTimerRing(callbackList, collieTimerRingNonSTW, count);
        isEmpty = false;
      }
    }
    if (isEmpty) {
      ++(trCtl.delayCount);
    } else {
      trCtl.delayCount = 0;
    }
  }
  // run timeout callback
  for (int i = 0; i < count; ++i) {
    LOG(ERROR) << "Trigger " << callbackList[i].name << " Callback Function (start time: " <<
        std::to_string(callbackList[i].startTime) << ")" << maple::endl;
    RunCallback(callbackList[i]);
  }
}

// monitor thread main loop
void *MplCollie::CollieThreadHandle() {
  pthread_setname_np(pthread_self(), "MapleCollie");

  while (runnable) {
    {
      std::unique_lock<std::mutex> lock(timerMutex);
      timerCond.wait_for(lock, std::chrono::seconds(kTimerRingCheckInterval), [this] { return !runnable; });
    }
    // timeout process
    // step 1: get timeout info and run callback
    TimerRingTimeout();

    // step 2: goto sleep if needed
    // if wake up from sleep state, time should be re-calc
    CollieTrySleep();
  }
  return nullptr;
}

// force collie thread to end.
void MplCollie::ForceEnd() {
  std::unique_lock<std::mutex> lock(timerMutex);
  runnable = false;
  timerCond.notify_one();
}

void MplCollie::JoinThread() {
  int ret = ::pthread_join(collieThread, nullptr);
  if (UNLIKELY(ret != 0)) {
    LOG(FATAL) << "failed to join maple collie thread!" << maple::endl;
  }
}

void* MplCollie::CollieThreadEntry(void *arg) {
  if (arg != nullptr) {
    MplCollie *self = reinterpret_cast<MplCollie*>(arg);
    self->CollieThreadHandle();
  }

  return nullptr;
}

// initializing the data struct
void MplCollie::InitImp(void) {
  trCtl.threadInSleep = false;
  trCtl.delayCount = 0;
  trCtl.sleepMutexPtr = std::make_unique<std::mutex>();
  trCtl.sleepCond = std::make_unique<std::condition_variable>();

  for (int i = 0; i < kCollieTypeMax; ++i) {
    ResetNode(collieNodesArray[i], i);
  }

  collieTimerRingSTW.timer = 0;
  collieTimerRingSTW.cl.Init();

  collieTimerRingNonSTW.timer = 0;
  collieTimerRingNonSTW.cl.Init();

  stwID = kInvalidId;
  // wait until last collie thread exits successfully.
  runnable = true;
  int ret = pthread_create(&collieThread, nullptr, MplCollie::CollieThreadEntry, this);
  if (ret != EOK) {
    LOG(ERROR) << "pthread_create return fail, MplCollie::InitImp" << maple::endl;
  }
}

// try wake up the monitor thread if it is sleeping
void MplCollie::CollieTryWake(void) {
  std::unique_lock<std::mutex> lock(*trCtl.sleepMutexPtr.get());
  if (trCtl.threadInSleep) {
    trCtl.threadInSleep = false;
    trCtl.sleepCond->notify_one();
  }
}

void MplCollie::Init(void) {
  std::unique_lock<std::mutex> lock(initMutex);
  if (UNLIKELY(!runnable)) {
    InitImp();
  }
}

void MplCollie::Fini() {
  ForceEnd();
  {
    std::unique_lock<std::mutex> lock(listMutex);
    collieTimerRingSTW.cl.Init();
    collieTimerRingNonSTW.cl.Init();
  }
  CollieTryWake();
}

bool MplCollie::GetSTWPanic(void) {
  if (ID_IS_INVALID(stwID)) {
    return true;
  }
  CollieNode *node = &collieNodesArray[stwID];
  return ((static_cast<uint32_t>(node->flag) & MPLCOLLIE_FLAG_ABORT) != 0);
}

void MplCollie::SetSTWPanic(bool enable) {
  if (ID_IS_INVALID(stwID)) {
    return;
  }
  LOG(ERROR) << "Set STW Panic: " << (enable ? "true" : "false") << maple::endl;
  CollieNode *node = &collieNodesArray[stwID];
  if (enable) {
    node->flag = static_cast<int>(static_cast<uint32_t>(node->flag) | MPLCOLLIE_FLAG_ABORT); // set abort flag
  } else {
    node->flag = static_cast<int>(static_cast<uint32_t>(node->flag) & (~MPLCOLLIE_FLAG_ABORT)); // clear abort flag
  }
}

// fill the collie node, and put it into the timer ring
// @param flag used to control whether to call the FatalPanicLocked or PromoteThreadPriority function after timeout
// @param tid thread id being monitored
// @param func callback called after timeout
// @param arg  args for callback
int MplCollie::Start(CollieType type, int flag, pid_t tid, void (*func)(void*), void *arg) {
  if (UNLIKELY(!runnable)) {
    return kInvalidId;
  }
  std::unique_lock<std::mutex> lock(listMutex);

  CollieNode *node = &(collieNodesArray[type]);
  if (node->isUsed) {
    LOG(ERROR) << node->name << "node is in used" << maple::endl;
    return kInvalidId;
  }
  node->Reset();

  // fill node info
  node->startTime = time(nullptr);
  node->tid = tid;
  node->arg = arg;
  node->flag = flag;
  node->callback = func;

  // add to timer ring
  CollieTimerRing *r = MPLCOLLIE_FLAG_IS_STW(static_cast<uint32_t>(node->flag)) ? &collieTimerRingSTW
                                                                                : &collieTimerRingNonSTW;
  node->timeout += r->timer;
  r->cl.Insert(*node);
  node->isUsed = true;

  if (static_cast<uint32_t>(flag) & MPLCOLLIE_FLAG_FOR_STW) {
    stwID = node->type;
  }

  CollieTryWake(); // Wake up the collie monitor thread when inserting at the front.
  trCtl.delayCount = 0;
  return node->type;
}

void MplCollie::End(int type) {
  if (UNLIKELY(!runnable)) {
    return;
  }

  if (ID_IS_INVALID(type)) {
    LOG(ERROR) << "MplCollie::End : not valid type: " << type << maple::endl;
    return;
  }

  // get node from timer ring, add to free list
  std::unique_lock<std::mutex> lock(listMutex);

  if (!collieNodesArray[type].isUsed) {
    LOG(ERROR) << "MplCollie::End : already release type: " << type << maple::endl;
    return;
  }
  CollieNode *node = &(collieNodesArray[type]);

  if (static_cast<uint32_t>(node->flag) & MPLCOLLIE_FLAG_FOR_STW) {
    stwID = kInvalidId;
  }

  // remove from timer ring
  CollieTimerRing *r = MPLCOLLIE_FLAG_IS_STW(static_cast<uint32_t>(node->flag)) ? &collieTimerRingSTW
                                                                                : &collieTimerRingNonSTW;
  r->cl.Remove(*node);

  ResetNode(*node, type);
}
}
#endif
