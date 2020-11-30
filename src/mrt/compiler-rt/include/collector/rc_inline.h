/*
 * Copyright (c) [2020] Huawei Technologies Co., Ltd. All rights reserved.
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
#ifndef MAPLE_RUNTIME_RC_INLINE_H
#define MAPLE_RUNTIME_RC_INLINE_H

#include "sizes.h"

namespace maplert {
static void inline HandleRCError(address_t obj) {
  AbortWithHeader(obj);
}

template<int32_t strongRCDelta, bool updateColor>
static inline uint32_t GetNewRCHeaderStrong(uint32_t header) {
  static_assert(strongRCDelta == 1 || strongRCDelta == -1 || strongRCDelta == 0, "Invalid strong rc delta");
  if (strongRCDelta == 0) {
    return header;
  }
  uint32_t newHeader = static_cast<uint32_t>((static_cast<int32_t>(header)) + strongRCDelta);
  if (updateColor) {
    uint32_t color = (strongRCDelta > 0) ? kRCCycleColorBlack : kRCCycleColorBrown;
    newHeader = (newHeader & ~kRCCycleColorMask) | color;
  }
  return newHeader;
}

template<int32_t weakRCDelta>
static inline uint32_t GetNewRCHeaderWeak(uint32_t header) {
  static_assert(weakRCDelta == 1 || weakRCDelta == -1 || weakRCDelta == 0, "Invalid weak rc delta");
  if (weakRCDelta == 0) {
    return header;
  }
  uint32_t weakRC = GetWeakRCFromRCHeader(header);
  if (weakRC == kMaxWeakRC) {
    return header;
  }

  weakRC = static_cast<uint32_t>((static_cast<int32_t>(weakRC)) + weakRCDelta);
  uint32_t newHeader = header & ~kWeakRcBits;
  newHeader |= ((weakRC << kWeakRcBitsShift) & kWeakRcBits);
  return newHeader;
}

template<int32_t resurrectWeakRCDelta>
static inline uint32_t GetNewRCHeaderResurrectWeak(uint32_t header) {
  static_assert(resurrectWeakRCDelta == 1 || resurrectWeakRCDelta == -1 || resurrectWeakRCDelta == 0,
                "Invalid weak rc delta");
  if (resurrectWeakRCDelta == 0) {
    return header;
  }
  uint32_t resurrectWeakRC = GetResurrectWeakRCFromRCHeader(header);
  if (resurrectWeakRC == kMaxResurrectWeakRC) {
    return header;
  }

  resurrectWeakRC =
      static_cast<uint32_t>((static_cast<int32_t>(resurrectWeakRC)) + resurrectWeakRCDelta);
  uint32_t newHeader = header & ~kResurrectWeakRcBits;
  newHeader |= ((resurrectWeakRC << KResurrectWeakRcBitsShift) & kResurrectWeakRcBits);
  return newHeader;
}

// Update reference count non-atomically:
// 1. During backup tracing STW time
// 2. Or for non-escaped objects
// No color update needed, because no racing with cycle pattern in above scenario
template<int32_t strongRCDelta, int32_t weakRCDelta, int32_t resurrectWeakRCDelta>
static inline uint32_t UpdateRC(address_t objAddr) {
  uint32_t oldHeader = RCHeader(objAddr);
  StatsRCOperationCount(objAddr);
  if (UNLIKELY(SkipRC(oldHeader))) {
    return oldHeader;
  }
  uint32_t newHeader = GetNewRCHeaderStrong<strongRCDelta, false>(oldHeader);
  newHeader = GetNewRCHeaderWeak<weakRCDelta>(newHeader);
  newHeader = GetNewRCHeaderResurrectWeak<resurrectWeakRCDelta>(newHeader);
  RefCountLVal(objAddr) = newHeader;
  return oldHeader;
}

// Update reference count atomically:
// 1. Mutator update RC
// 2. Backup tracing: parallel/concurrent sweep
template<int32_t strongRCDelta, int32_t weakRCDelta, int32_t resurrectWeakRCDelta>
static inline uint32_t AtomicUpdateRC(address_t objAddr) {
  std::atomic<uint32_t> &headerAtomic = RefCountAtomicLVal(objAddr);
  uint32_t oldHeader = headerAtomic.load();
  uint32_t newHeader = 0;

  do {
    StatsRCOperationCount(objAddr);
    if (UNLIKELY(SkipRC(oldHeader))) {
      return oldHeader;
    }
    newHeader = GetNewRCHeaderStrong<strongRCDelta, true>(oldHeader);
    newHeader = GetNewRCHeaderWeak<weakRCDelta>(newHeader);
    newHeader = GetNewRCHeaderResurrectWeak<resurrectWeakRCDelta>(newHeader);
  } while (!headerAtomic.compare_exchange_weak(oldHeader, newHeader));
  return oldHeader;
}

// Check what to do next after Dec
// 1. Release object if:
//    1.1 weak collected bit is set, and all rc is zero
//    1.2 weak collected bit is not set and only weak rc is 1, strong rc and resurrect weak rc is 0
// 2. Collected object weak rc if:
//    2.1 weak collected bit is not set and strong rc and resurrect rc is 0
static constexpr uint32_t kReleaseObject = 0x0;
static constexpr uint32_t kCollectedWeak = 0x1;
static constexpr uint32_t kNotRelease = 0x2;
template<int32_t strongRCDelta, int32_t weakRCDelta, int32_t resurrectWeakRCDelta>
static inline uint32_t CanReleaseObj(uint32_t oldHeader) {
  static_assert(strongRCDelta + weakRCDelta + resurrectWeakRCDelta == -1, "only one with -1");
  static_assert(strongRCDelta * weakRCDelta * resurrectWeakRCDelta == 0, "only one with -1");
  uint32_t oldRC = oldHeader & (kWeakCollectedBit | kWeakRcBits | kResurrectWeakRcBits | kRCBits);
  if (strongRCDelta != 0) {
    // old header is strong rc = 1, resurrect weak rc = 0, weak rc = 0 and collected bit set
    if ((oldRC == (1 + kWeakRCOneBit)) || (oldRC == (1 + kWeakCollectedBit))) {
      return kReleaseObject;
    } else if ((oldHeader & (kWeakCollectedBit | kResurrectWeakRcBits | kRCBits)) == 1) {
      return kCollectedWeak;
    }
  } else if (weakRCDelta != 0) {
    if ((oldRC == (kWeakRCOneBit + kWeakRCOneBit)) || (oldRC == (kWeakRCOneBit + kWeakCollectedBit))) {
      return kReleaseObject;
    }
  } else {
    // Dec resurrect weak rc
    if (oldRC == (kResurrectWeakOneBit + kWeakRCOneBit) || oldRC == (kResurrectWeakOneBit + kWeakCollectedBit)) {
      return kReleaseObject;
    } else if ((oldHeader & (kWeakCollectedBit | kResurrectWeakRcBits | kRCBits)) == kResurrectWeakOneBit) {
      return kCollectedWeak;
    }
  }
  return kNotRelease;
}

template<int32_t strongRCDelta, int32_t weakRCDelta, int32_t resurrectWeakRCDelta, bool checkWeakCollected = true>
static inline uint32_t AtomicDecRCAndCheckRelease(address_t objAddr, uint32_t &releaseState) {
  static_assert(strongRCDelta == -1 || strongRCDelta == 0, "Invalid dec rc delta");
  static_assert(weakRCDelta == -1 || weakRCDelta == 0, "Invalid dec rc delta");
  static_assert(resurrectWeakRCDelta == -1 || resurrectWeakRCDelta == 0, "Invalid dec rc delta");
  static_assert(strongRCDelta == -1 || weakRCDelta == -1 || resurrectWeakRCDelta == -1, "must have one -1");
  std::atomic<uint32_t> &headerAtomic = RefCountAtomicLVal(objAddr);
  uint32_t oldHeader = headerAtomic.load();
  uint32_t newHeader = 0;

  do {
    StatsRCOperationCount(objAddr);
    releaseState = kNotRelease;
    if (UNLIKELY(SkipRC(oldHeader))) {
      return oldHeader;
    }
    newHeader = GetNewRCHeaderStrong<strongRCDelta, true>(oldHeader);
    newHeader = GetNewRCHeaderWeak<weakRCDelta>(newHeader);
    newHeader = GetNewRCHeaderResurrectWeak<resurrectWeakRCDelta>(newHeader);
    // update release state
    // release: weak rc is 1, resurrect and strong rc and weak collected bit not set
    //          weak rc is 0, resurrect and strong rc and weak collected bit set
    if (IsWeakCollectedFromRCHeader(newHeader)) {
      if (GetTotalRCFromRCHeader(newHeader) == 0) {
        releaseState = kReleaseObject;
      }
    } else {
      if (GetTotalRCFromRCHeader(newHeader) == kWeakRCOneBit) {
        releaseState = kReleaseObject;
      } else if (checkWeakCollected && (newHeader & (kRCBits | kResurrectWeakRcBits)) == 0) {
        releaseState = kCollectedWeak;
        newHeader = newHeader | kWeakCollectedBit;
      }
    }
  } while (!headerAtomic.compare_exchange_weak(oldHeader, newHeader));
  return oldHeader;
}

template<int32_t strongRCDelta, int32_t weakRCDelta, int32_t resurrectWeakRCDelta>
static inline bool IsInvalidDec(uint32_t oldHeader) {
  static_assert(strongRCDelta + weakRCDelta + resurrectWeakRCDelta == -1, "only one with -1");
  static_assert(strongRCDelta * weakRCDelta * resurrectWeakRCDelta == 0, "only one with -1");
  if (IsRCOverflow(oldHeader)) {
    return false;
  }
  if (strongRCDelta != 0) {
    return (oldHeader & kRCBits) == 0;
  } else if (weakRCDelta != 0) {
    return (oldHeader & kWeakRcBits) == 0;
  }
  return (oldHeader & kResurrectWeakRcBits) == 0;
}

// Update color atomically:
// 1. Cycle pattern match
static inline uint32_t AtomicUpdateColor(address_t objAddr, uint32_t color) {
  std::atomic<uint32_t> &headerAtomic = RefCountAtomicLVal(objAddr);
  uint32_t oldHeader = headerAtomic.load();
  uint32_t newHeader = 0;

  do {
    StatsRCOperationCount(objAddr);
    if (UNLIKELY(SkipRC(oldHeader))) {
      return oldHeader;
    }
    newHeader = (oldHeader & ~kRCCycleColorMask) | color;
  } while (!headerAtomic.compare_exchange_weak(oldHeader, newHeader));
  return oldHeader;
}

static inline bool TryAtomicIncStrongRC(address_t objAddr) {
  std::atomic<uint32_t> &headerAtomic = RefCountAtomicLVal(objAddr);
  uint32_t oldHeader = headerAtomic.load();
  uint32_t newHeader = 0;

  do {
    StatsRCOperationCount(objAddr);
    if (UNLIKELY(SkipRC(oldHeader))) {
      return true;
    }
    if (UNLIKELY(GetRCFromRCHeader(oldHeader) == 0)) {
      return false;
    }
    newHeader = GetNewRCHeaderStrong<1, true>(oldHeader);
  } while (!headerAtomic.compare_exchange_weak(oldHeader, newHeader));
  return true;
}

// RC Java Reference and Weak Global processing
// Operatoins
// 1. Collect:
//    1.1. weak rc == strong rc is equal, set weak collected bit
//    1.2. weak rc == (strong rc - matched cycle rc), set weak collected bit
//    1.3. If weak collected bit is set, perform clearAndEnqueue/clear WGRT
// 2. Process finalizable
//    2.1 add finalizable queue
//    2.2 clear weak collected bit: object is resurrected
// 3. Get
//    If weak collected bit is on, Reference.get and Weak Global.get return null
//
// delta is used in cycle collect for referent
// 1. when process reference: check cycle with rc (rc-weakrc)
// 2. if match, invoke AtomicCheckWeakCollectable(obj, rc-weakrc)
// no overflow/skipRC check! should be handled by caller!
template<bool checkResurrectWeak>
static inline bool AtomicCheckWeakCollectable(address_t objAddr, uint32_t delta) {
  std::atomic<uint32_t> &headerAtomic = RefCountAtomicLVal(objAddr);
  uint32_t oldHeader = headerAtomic.load();
  uint32_t newHeader = 0;

  do {
    if (IsWeakCollectedFromRCHeader(oldHeader)) {
      return false;
    }
    uint32_t strongRC = GetRCFromRCHeader(oldHeader);
    if (strongRC != delta) {
      return false;
    }
    if (checkResurrectWeak) {
      if (oldHeader & kResurrectWeakRcBits) {
        return false;
      }
    }
    newHeader = oldHeader | kWeakCollectedBit;
  } while (!headerAtomic.compare_exchange_weak(oldHeader, newHeader));
  return true;
}

// invoked in reference processor thread or muator thread
// 1. reference process clear kWeakCollectedBit for finalizable
// 2. mutator set obj as referent again and need clear corrsponding bit
// no overflow/skipRC check! should be handled by caller!
static inline void AtomicClearWeakCollectable(address_t objAddr) {
  std::atomic<uint32_t> &headerAtomic = RefCountAtomicLVal(objAddr);
  uint32_t oldHeader = headerAtomic.load();
  uint32_t newHeader = 0;
  do {
    if (!(oldHeader & kWeakCollectedBit)) {
      return;
    }
    newHeader = oldHeader & (~kWeakCollectedBit);
  } while (!headerAtomic.compare_exchange_weak(oldHeader, newHeader));
  return;
}

// Load and inc Referent, load can from
// 1. Weak Global.get
// 2. SoftReference.get
// 3. WeakReference.get
//
// If object 1) has no weak rc 2) weak collected bit set return 0
// Otherwise, try inc weak rc
template<bool incWeak>
static inline address_t AtomicIncLoadWeak(address_t objAddr) {
  std::atomic<uint32_t> &headerAtomic = RefCountAtomicLVal(objAddr);
  uint32_t oldHeader = headerAtomic.load();
  uint32_t newHeader = 0;
  do {
    if (oldHeader & kWeakCollectedBit) {
      return 0;
    }
    StatsRCOperationCount(objAddr);
    if (UNLIKELY(SkipRC(oldHeader))) {
      return objAddr;
    }
    if (incWeak) {
      // weak collected bit not set, weak rc is 1 means no other weak reference
      if (GetWeakRCFromRCHeader(oldHeader) == 1) {
        return 0;
      }
      newHeader = GetNewRCHeaderWeak<1>(oldHeader);
    } else {
      newHeader = GetNewRCHeaderStrong<1, true>(oldHeader);
    }
  } while (!headerAtomic.compare_exchange_weak(oldHeader, newHeader));
  return objAddr;
}
} // namespace maplert

#endif // MAPLE_RUNTIME_RC_INLINE_H
