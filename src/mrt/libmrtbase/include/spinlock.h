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

#ifndef MAPLE_SPINLOCK_H
#define MAPLE_SPINLOCK_H
#include <pthread.h>
#include "scoped_spinlock.h"

namespace maple {

class SpinLock {
 public:
  // Create a Mutex that is not held by anybody.
  inline SpinLock();

  // Destructor
  inline ~SpinLock();

  inline void Lock();
  inline void Unlock();
  inline bool TryLock();

 private:
  pthread_spinlock_t spinlock;
  DISABLE_CLASS_COPY_AND_ASSIGN(SpinLock);
};

inline SpinLock::SpinLock() {
  pthread_spin_init(&spinlock, 0);
}

inline SpinLock::~SpinLock() {
  pthread_spin_destroy(&spinlock);
}

inline void SpinLock::Lock() {
  pthread_spin_lock(&spinlock);
}

inline void SpinLock::Unlock() {
  pthread_spin_unlock(&spinlock);
}

inline bool SpinLock::TryLock() {
  return pthread_spin_trylock(&spinlock) == 0;
}

using SpinAutoLock = ScopedSpinLock<SpinLock>;

}  // namespace maple
#endif  // MAPLE_SPINLOCK_H
