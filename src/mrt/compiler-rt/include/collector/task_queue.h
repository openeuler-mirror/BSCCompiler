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

#ifndef MAPLE_RUNTIME_TASK_QUEUE_H
#define MAPLE_RUNTIME_TASK_QUEUE_H

#include "allocator/page_allocator.h"

// task queue implementation
namespace maplert {
class ScheduleTaskBase {
 public:
  enum ScheduleTaskType : int {
    kInvalidScheduleType = -1,
    kScheduleTaskTypeInvokeGC = 0, // invoke gc task
    kScheduleTaskTypeTimeout = 1, // timeout task
    kScheduleTaskTypeTerminate = 2, // terminate task
  };
  static constexpr uint64_t kIndexExit = 0;
  static constexpr uint64_t kSyncIndexStartValue = 1;
  static constexpr uint64_t kAsyncIndex = ULONG_MAX;

  ScheduleTaskBase(const ScheduleTaskBase &task) = default;
  ScheduleTaskBase(ScheduleTaskType type) : taskType(type), syncIndex(kAsyncIndex) {}
  virtual ~ScheduleTaskBase() = default;
  ScheduleTaskBase &operator=(const ScheduleTaskBase&) = default;

  ScheduleTaskType GetType() const {
    return taskType;
  }

  void SetType(ScheduleTaskType type) {
    taskType = type;
  }

  uint64_t GetSyncIndex() const {
    return syncIndex;
  }

  void SetSyncIndex(uint64_t index) {
    syncIndex = index;
  }

  virtual bool NeedFilter() const {
    return false;
  }

  virtual bool Execute(void *owner) = 0;

  virtual std::string ToString() const {
    std::stringstream ss;
    ss << ScheduleTaskTypeToString(taskType) << " index=" << syncIndex;
    return ss.str();
  }

 protected:
  ScheduleTaskType taskType;
  uint64_t syncIndex;

 private:
  const char *ScheduleTaskTypeToString(ScheduleTaskType type) const {
    switch (type) {
      case ScheduleTaskType::kScheduleTaskTypeInvokeGC:
        return "InvokeGC";
      case ScheduleTaskType::kScheduleTaskTypeTimeout:
        return "Timeout";
      case ScheduleTaskType::kScheduleTaskTypeTerminate:
        return "Terminate";
      default:
        return "Wrong Type";
    }
  }
};

template<typename T>
class LocklessTaskQueue {
 public:
  // this queue manages a list of deduplicated tasks of *a fixed number of* kinds
  // each task has a priority directly associated with the kind
  // smaller kind (that is, smaller integer) has a higher priority
  // high priority task's enqueue might erase all lower-priority tasks
  // the intuition is that, (when managing concurrent gc tasks, which are merely
  // to lower memory level from the background), maybe we only need the strongest
  // one in the queue
  // this queue is lockless
  void Push(const T &task) {
    bool overriding = task.IsOverriding();
    uint32_t taskMask = (1U << task.GetPrio());
    uint32_t oldWord = tqWord.load(std::memory_order_relaxed);
    uint32_t newWord = 0;
    do {
      if (overriding) {
        newWord = taskMask | ((taskMask - 1) & oldWord);
      } else {
        newWord = taskMask | oldWord;
      }
    } while (!tqWord.compare_exchange_weak(oldWord, newWord, std::memory_order_relaxed));
  }
  T Pop() {
    uint32_t oldWord = tqWord.load(std::memory_order_relaxed);
    uint32_t newWord = 0;
    uint32_t dequeued = oldWord;
    do {
      newWord = oldWord & (oldWord - 1);
      dequeued = oldWord;
    } while (!tqWord.compare_exchange_weak(oldWord, newWord, std::memory_order_relaxed));
    if (oldWord == 0) {
      return T::DoNothing();
    }
    // count the number of trailing zeros
    return T::FromPrio(__builtin_ctz(dequeued));
  }
  void Clear() {
    tqWord.store(0, std::memory_order_relaxed);
  }
 private:
  std::atomic<uint32_t> tqWord = {};
};

template<typename T, uint64_t minIntervalNs = 1000UL * 1000 * 1000>
class TaskQueue {
  static_assert(std::is_base_of<ScheduleTaskBase, T>::value, "T is not a subclass of maplert::ScheduleTaskBase");

 public:
  using TaskFilter = std::function<bool(T &oldTask, T &newTask)>;
  using TaskQueueType = std::list<T, StdContainerAllocator<T, kGCTaskQueue>>;

  void Init() {
    queueSyncIndex = ScheduleTaskBase::kSyncIndexStartValue;
  }

  void DeInit() {
    std::lock_guard<std::mutex> lock(taskQueueLock);
    taskQueue.Clear();
    syncTaskQueue.clear();
    LOG(INFO) << "[GC] DeInit task Q done" << maple::endl;
  }

  template<bool sync = false>
  uint64_t Enqueue(T &task, TaskFilter &filter) {
    if (!sync) {
      EnqueueAsync(task);
      return ScheduleTaskBase::kAsyncIndex;
    }
    std::unique_lock<std::mutex> lock(taskQueueLock);
    TaskQueueType &queue = syncTaskQueue;

    if (!queue.empty() && task.NeedFilter()) {
      for (auto iter = queue.rbegin(); iter != queue.rend(); ++iter) {
        if (filter(*iter, task)) {
          return (*iter).GetSyncIndex();
        }
      }
    }
    task.SetSyncIndex(++queueSyncIndex);
    queue.push_back(task);
    taskQueueCondVar.notify_all();
    return task.GetSyncIndex();
  }

  void EnqueueAsync(const T &task) {
    taskQueue.Push(task);
    taskQueueCondVar.notify_all();
  }

  T Dequeue() {
    std::cv_status cvResult = std::cv_status::no_timeout;
    std::chrono::nanoseconds waitTime(kDefaultTheadTimeoutNs);
    while (true) {
      std::unique_lock<std::mutex> qLock(taskQueueLock);
      // check sync queue firstly
      if (!syncTaskQueue.empty()) {
        T curTask(syncTaskQueue.front());
        syncTaskQueue.pop_front();
        return curTask;
      }

      if (cvResult == std::cv_status::timeout) {
        taskQueue.Push(T(ScheduleTaskBase::ScheduleTaskType::kScheduleTaskTypeTimeout));
      }

      T task = taskQueue.Pop();
      if (!task.IsNothing()) {
        return task;
      }
      cvResult = taskQueueCondVar.wait_for(qLock, waitTime);
    }
  }

  void LoopDrainTaskQueue(void *owner) {
    while (true) {
      T task = Dequeue();
      if (!task.Execute(owner)) {
        DeInit();
        break;
      }
    }
  }

 private:
#if LOG_ALLOC_TIMESTAT
  static constexpr uint64_t kDefaultTheadTimeoutNs = 300000L * 1000 * 1000; // default 300s
#else
  static constexpr uint64_t kDefaultTheadTimeoutNs = 1000L * 1000 * 1000; // default 1s
#endif
  std::mutex taskQueueLock;
  uint64_t queueSyncIndex;
  TaskQueueType syncTaskQueue;
  LocklessTaskQueue<T> taskQueue;
  std::condition_variable taskQueueCondVar;
};
}
#endif
