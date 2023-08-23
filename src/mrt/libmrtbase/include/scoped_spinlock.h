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

#ifndef MAPLE_SCOPED_SPINLOCK_H
#define MAPLE_SCOPED_SPINLOCK_H

#include "base/macros.h"

namespace maple {

template <typename LockType>
class ScopedSpinLock {
 public:
  explicit ScopedSpinLock(LockType &lock) : sl(lock) {
    sl.Lock();
  }

  ~ScopedSpinLock() {
    sl.Unlock();
  }

 private:
  LockType &sl;
  DISABLE_CLASS_COPY_AND_ASSIGN(ScopedSpinLock);
};

} // namespace maple

#endif // MAPLE_SCOPED_SPINLOCK_H
