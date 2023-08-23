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
#ifndef MAPLE_RUNTIME_MUTATOR_LIST_H
#define MAPLE_RUNTIME_MUTATOR_LIST_H

#include <mutex>
#include <list>
#include <cinttypes>
#include <memory>
#include <unordered_map>
#include "mm_config.h"
#include "collector/collector.h"
#include "panic.h"
#include "syscall.h"
#include "allocator/page_allocator.h"

#ifndef MRT_DEBUG_MUTATOR_LIST
#define MRT_DEBUG_MUTATOR_LIST __MRT_DEBUG_COND_FALSE
#endif

namespace maplert {
class MutatorList {
 public:
  MutatorList() : lockOwnerTid(0) {}
  ~MutatorList() = default;
  // Gets the singleton instance.
  static inline MutatorList &Instance() {
    return *instance;
  }

  // Replace the singleton instance with a new one.
  // This should be called after forked from parent process.
  static void reset();

  // Add a mutator to the list.
  void AddMutator(Mutator &mutator) {
    std::lock_guard<std::mutex> lock(mutatorListMutex);
    lockOwnerTid = static_cast<uint32_t>(maple::GetTid());
    mutator.active = kMutatorDebugTrue;
    mutator.Init();
    mutatorList.push_back(&mutator);
    lockOwnerTid = 0;
#if MRT_DEBUG_MUTATOR_LIST
    auto result = debugMap.insert({ &mutator, mutator.tid });
    if (UNLIKELY(!result.second)) {
      // insert failed, means there is a record leaved by an abnormlly exited mutator.
      LOG(FATAL) << "Found abnormally exited mutator!" <<
          " mutator: " << result.first->first <<
          " tid: " << result.first->second << maple::endl;
    }
#endif
  }

  // Remove a mutator from the list.
  template<typename Func>
  void RemoveMutator(Mutator &mutator, Func &&func) {
    std::lock_guard<std::mutex> lock(mutatorListMutex);
    lockOwnerTid = static_cast<uint32_t>(maple::GetTid());
    size_t sizeBeforeRemove __MRT_UNUSED = mutatorList.size();
    mutatorList.remove(&mutator);
#if __MRT_DEBUG
    size_t removedCount = sizeBeforeRemove - mutatorList.size();
    if (UNLIKELY(removedCount != 1)) {
      LOG(FATAL) << "RemoveMutator: invalid removed count: " << removedCount << maple::endl;
    }
#endif
    func(&mutator);
    mutator.active = kMutatorDebugFalse;
    lockOwnerTid = 0;
#if MRT_DEBUG_MUTATOR_LIST
    size_t removed = debugMap.erase(&mutator);
    if (UNLIKELY(removed != 1)) {
      LOG(FATAL) << "Remove an invalid mutator" << maple::endl;
    }
#endif
  }

  // Do things when lock is hold.
  template<typename Func>
  void LockGuard(Func &&func) {
    std::lock_guard<std::mutex> lock(mutatorListMutex);
    lockOwnerTid = static_cast<uint32_t>(maple::GetTid());
    func();
    lockOwnerTid = 0;
  }

  // Lock the mutex.
  void Lock() {
    mutatorListMutex.lock();
    lockOwnerTid = static_cast<uint32_t>(maple::GetTid());
  }

  // Unlock the mutex.
  bool TryLock() {
    if (mutatorListMutex.try_lock()) {
      lockOwnerTid = static_cast<uint32_t>(maple::GetTid());
      return true;
    }
    return false;
  }

  // Unlock the mutex.
  void Unlock() {
    lockOwnerTid = 0;
    mutatorListMutex.unlock();
  }

  // Number of mutators in the list.
  size_t Size() const {
    return mutatorList.size();
  }

  // Gets the mutator list.
  const std::list<Mutator*, StdContainerAllocator<Mutator*, kMutatorList>> &List() const {
    return mutatorList;
  }

  // Before using this interface, you need to call instance.Lock,
  // and after using mutator, you should call instance.Unlock.
  Mutator *GetMutator(uint32_t tid) {
    __MRT_ASSERT(MutatorList::Instance().IsLockedBySelf() == true, "mutator list is not locked");
    for (auto &mutator : mutatorList) {
      if (mutator->GetTid() == tid) {
        return mutator;
      }
    }
    return nullptr;
  }

  // Visit all mutators, should be called
  // in collector thread after the world stopped.
  // Func: void func(Mutator* mutator);
  template<typename Func>
  void VisitMutators(Func &&func) {
    for (auto &mutator : mutatorList) {
      func(mutator);
    }
  }

  // Get stack begin with given tid
  uintptr_t GetStackBeginByTid(uint32_t tid) {
    for (auto &mutator : mutatorList) {
      if (mutator->GetTid() == tid) {
        return mutator->GetStackBegin();
      }
    }
    return 0;
  }

  void DebugShowCurrentMutators();

  bool IsLockedBySelf() const {
    if (lockOwnerTid == 0) {
      return false;
    }
    return lockOwnerTid == static_cast<uint32_t>(maple::GetTid());
  }

  uint32_t LockOwner() const {
    return lockOwnerTid;
  }

 private:
  // mutex use to protect mutatorList.
  // we also use it to block all mutators when the world is stopped.
  std::mutex mutatorListMutex;

  // list of all mutators, protected by mutatorListMutex.
  std::list<Mutator*, StdContainerAllocator<Mutator*, kMutatorList>> mutatorList;

  // holder for this lock
  volatile uint32_t lockOwnerTid;

  // The Singleton instance.
  __attribute__((visibility("default"))) static ImmortalWrapper<MutatorList> instance;

#if MRT_DEBUG_MUTATOR_LIST
  // mutator --> tid map, for debug purpose.
  std::unordered_map<Mutator*, uint32_t> debugMap;
#endif
};
} // namespace maplert

#endif
