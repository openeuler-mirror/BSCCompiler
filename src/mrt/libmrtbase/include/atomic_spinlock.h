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

#ifndef MAPLE_ATOMIC_SPINLOCK_H
#define MAPLE_ATOMIC_SPINLOCK_H
#include <atomic>
#include <base/macros.h>
#include "scoped_spinlock.h"

namespace maple {

class AtomicSpinLock {
 public:
  AtomicSpinLock();
  ~AtomicSpinLock() = default;

  void Lock();
  void Unlock()

 private:
  std::atomic_flag state = ATOMIC_FLAG_INIT;

  DISABLE_CLASS_COPY_AND_ASSIGN(AtomicSpinLock);
};

using ScopedAtomicSpinLock = ScopedSpinLock<AtomicSpinLock>;
} // maple namespace

#endif // MAPLE_ATOMIC_SPINLOCK_H
