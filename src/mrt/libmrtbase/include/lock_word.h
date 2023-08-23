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

#ifndef MAPLE_OBJECT_LOCK_WORD_H_
#define MAPLE_OBJECT_LOCK_WORD_H_

#include "monitor.h"
#include "monitor_pool.h"

namespace maple {

/*
  format:
  ThinLock:
  |10|98|765432109876|5432109876543210|
  |00|xx|lock count  |thread id       |

  FatLock:
  |10|98|7654321098765432109876543210|
  |01|xx| MonitorId                  |

  HashCode:
  |10|98|7654321098765432109876543210|
  |10|xx| HashCode                   |
*/
class LockWord {
 public:
  enum SizeShiftsAndMasks {
    kStateSize = 2,
    kReadBarrierStateSize = 2,

    // ThinLock
    kThinLockOwnerSize = 16,
    kThinLockCountSize = 32 - kThinLockOwnerSize - kStateSize - kReadBarrierStateSize,
    // ThinLock owner
    kThinLockOwnerShift = 0,
    kThinLockOwnerMask = (1 << kThinLockOwnerSize) - 1,
    kThinLockMaxOwner = kThinLockOwnerMask,
    // ThinLock Count
    kThinLockCountShift = kThinLockOwnerSize + kThinLockOwnerShift,
    kThinLockCountMask = (1 << kThinLockCountSize) - 1,
    kThinLockMaxCount = kThinLockCountMask,

    // FatLock
    kMonitorIdShift = 0,
    kMonitorIdSize = 32 - kStateSize - kReadBarrierStateSize,
    kMonitorIdMask = (1 << kMonitorIdSize) - 1,
    kMaxMonitorId = kMonitorIdMask,

    // HashCode
    kHashShift = 0,
    kHashSize = 32 - kStateSize - kReadBarrierStateSize,
    kHashMask = (1 << kHashSize) - 1,
    kMaxHash = kHashMask,

    // Hash Cached flag
    kHashFlagShift = kThinLockCountSize + kThinLockCountShift,
    kHashFlagMaskShifted  = 1 << kHashFlagShift,

    // Lock Status
    kStateThinOrUnlocked = 0,
    kStateFat = 1,
    kStateHash = 2,

    // State in the highest bits.
    kStateShift = kReadBarrierStateSize + kThinLockCountSize + kThinLockCountShift,
    kStateMask = (1 << kStateSize) - 1,
    kStateMaskShifted = kStateMask << kStateShift,
  };

  enum LockState {
    kUnlocked = 0,
    kThinLocked,
    kFatLocked,
    kHashCode,
    kUndefined,
  };

  LockWord();
  LockWord(uint32_t val);
  LockWord(Monitor* mon);

  static LockWord FromThinLockId(uint32_t threadId, uint32_t count, uint32_t ATTR_UNUSED) {
    // CHECK(threadId <= 0xffff) << "thread id out of maxsize" << maple::endl;
    return LockWord((threadId << kThinLockOwnerShift) | (count << kThinLockCountShift) |
                    (kStateThinOrUnlocked << kStateShift));
  }

  static LockWord FromHashCode(uint32_t hashCode, uint32_t unused ATTR_UNUSED) {
    CHECK_LE(hashCode, static_cast<uint32_t>(kMaxHash));
    return LockWord((hashCode << kHashShift) |
                    (kStateHash << kStateShift));
  }

  LockState GetState() const;

  uint32_t ThinLockOwner() const;

  uint32_t ThinLockCount() const;

  Monitor* FatLockMonitor() const;

  int32_t GetHashCode();

  uint32_t GetHashFlag() const;

  uint32_t GetValue() const;

 private:
  uint32_t value;
};

} // namespace maple
#endif // MAPLE_OBJECT_LOCK_WORD_H_
