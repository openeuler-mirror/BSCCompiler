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
#ifndef MAPLE_RUNTIME_ALLOCATOR_H
#define MAPLE_RUNTIME_ALLOCATOR_H

#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>

#include "mm_config.h"
#include "address.h"
#include "heap_stats.h"
#include "mrt_object.h"
#include "panic.h"
#include "thread_api.h"
#include "collector/mrt_bitmap.h"
#include "collector/mpl_thread_pool.h"
#include "mrt_reference_api.h"
#if LOG_ALLOC_TIMESTAT
#include "utils/time_utils.h"
#endif

#define ALLOC_ENABLE_LOCK_CONTENTION_STATS __MRT_DEBUG_COND_FALSE

#if ALLOC_ENABLE_LOCK_CONTENTION_STATS
#define ALLOC_MUTEX_TYPE maple::Mutex
#define ALLOC_LOCK_TYPE maple::MutexLock
#define ALLOC_CURRENT_THREAD maple::IThread::Current(),
#else
#define ALLOC_MUTEX_TYPE std::mutex
#define ALLOC_LOCK_TYPE std::lock_guard<mutex>
#define ALLOC_CURRENT_THREAD
#endif

namespace maplert {
// The allocator collects five kinds of data from the heap:
// 1. Fragmentation related: the utilisation of the heap
// 2. Per-mutator allocation data: this is non-atomic
// 3. Global allocation data: this is atomic
// 4. Allocation time statistics
// 5. Lock contention statistics
// These are written in the contract and every allocator implementation should support them.
// Additional stats can be supported by adding callbacks to the callback interface.
class FragmentationRecord {
 public:
  // this is allocator-specific, move into allocator implementation header
  size_t GetExternalFragmentation() const {
    return runFreeSlots + runOverhead + freePages;
  }
  size_t GetInternalFragmentation() const {
    return internal;
  }
  size_t GetRCHeader()const {
    return rcOverhead;
  }
  size_t GetBytesInFreePages() const {
    return freePages;
  }
  size_t GetBytesInPages() const {
    return totalPages;
  }
  size_t GetFreeSlots() const {
    return runFreeSlots;
  }
  size_t GetSlots() const {
    return runTotalSlots;
  }
  size_t GetRunCacheVolume() const {
    return runCacheVolume;
  }
  size_t GetRunOverhead() const {
    return runOverhead;
  }
  size_t GetRun(bool isFree, bool isLocal) const {
    if (isFree) {
      if (isLocal) {
        return freeLocalRun;
      } else {
        return freeRun;
      }
    } else {
      if (isLocal) {
        return totalLocalRun;
      } else {
        return totalRun;
      }
    }
  }

  void RecordInternalFrag(size_t totalBytes, size_t requestedBytes) {
    internal = totalBytes - requestedBytes;
  }

  void IncFreePages(size_t bytes) {
    freePages += bytes;
  }
  void IncPages(size_t bytes) {
    totalPages += bytes;
  }
  void IncFreeSlots(size_t bytes) {
    runFreeSlots += bytes;
  }
  void IncSlots(size_t bytes) {
    runTotalSlots += bytes;
  }
  void IncRunOverhead(size_t bytes) {
    runOverhead += bytes;
  }
  void IncRunCacheVolume(size_t bytes) {
    runCacheVolume += bytes;
  }
  void IncRun(size_t bytes, bool isFree, bool isLocal) {
    if (isFree) {
      freeRun += bytes;
      if (isLocal) {
        freeLocalRun += bytes;
      }
    }
    totalRun += bytes;
    if (isLocal) {
      totalLocalRun += bytes;
    }
  }

  FragmentationRecord()
      : internal(0),
        freePages(0),
        totalPages(0),
        runFreeSlots(0),
        runTotalSlots(0),
        runOverhead(0),
        rcOverhead(0),
        runCacheVolume(0),
        freeRun(0),
        totalRun(0),
        freeLocalRun(0),
        totalLocalRun(0) {}
  ~FragmentationRecord() = default;

  void Reset() {
    internal = 0;
    freePages = 0;
    totalPages = 0;
    runFreeSlots = 0;
    runTotalSlots = 0;
    runOverhead = 0;
    rcOverhead = 0;
    runCacheVolume = 0;
    freeRun = 0;
    totalRun = 0;
    freeLocalRun = 0;
    totalLocalRun = 0;
  }

  void Print(std::basic_ostream<char> &os, std::string tag) {
    os << "External fragment [" << tag << "]: " << GetExternalFragmentation() << "\n";
    os << "Internal fragment [" << tag << "]: " << GetInternalFragmentation() << "\n";
    os << "Free page bytes [" << tag << "]: " << GetBytesInFreePages() << "\n";
    os << "Total page bytes [" << tag << "]: " << GetBytesInPages() << "\n";
    os << "Free run bytes (incl cache) [" << tag << "]: " << GetRun(true, false) << "\n";
    os << "Total run bytes (incl cache) [" << tag << "]: " << GetRun(false, false) << "\n";
    os << "Free slot bytes (incl cache) [" << tag << "]: " << GetFreeSlots() << "\n";
    os << "Run overhead [" << tag << "]: " << GetRunOverhead() << "\n";
    // only use when not considering the gc header
    os << "Total slot bytes (incl cache) [" << tag << "]: " << GetSlots() << "\n";
    os << "Free local run bytes [" << tag << "]: " << GetRun(true, true) << "\n";
    os << "Total local run bytes [" << tag << "]: " << GetRun(false, true) << "\n";
    os << "Cached run bytes [" << tag << "]: " << GetRunCacheVolume() << "\n";
  }
 private:
  // fragmentation fields:
  size_t internal;       // internal frag. caused by rc/gc header and alignment
  size_t freePages;      // external frag. free pages in bytes not yet allocated
  size_t totalPages;
  size_t runFreeSlots;   // external frag. total bytes of the free slots
  size_t runTotalSlots;
  size_t runOverhead;    // external frag. total bytes of the run header and the trailing space
  // extra fields:
  size_t rcOverhead;     // total bytes used by RC header
  size_t runCacheVolume; // bytes consumed by the cached runs
  size_t freeRun;
  size_t totalRun;
  size_t freeLocalRun;
  size_t totalLocalRun;
}; // class FragmentationRecord

class AccUnSynchedSizeField {
 public:
  inline size_t LoadSize() const {
    return val;
  }
  inline void Inc(size_t param) {
    val += param;
  }
  inline void Dec(size_t param) {
    val -= param;
  }
  inline void Init() {
    val = 0;
  }
  AccUnSynchedSizeField() : val(0) {}
  ~AccUnSynchedSizeField() = default;
 private:
  size_t val;
};

class AccSynchedSizeField {
 public:
  inline size_t LoadSize() const {
    return val.load(std::memory_order_relaxed);
  }
  inline void Inc(size_t param) {
    (void)val.fetch_add(param, std::memory_order_relaxed);
  }
  inline void Sub(size_t param) {
    (void)val.fetch_sub(param, std::memory_order_relaxed);
  }
  inline void Dec(size_t param) {
    (void)val.fetch_sub(param, std::memory_order_relaxed);
  }
  inline void Init() {
    val.store(0, std::memory_order_seq_cst);
  }
  AccSynchedSizeField() : val(0) {}
  ~AccSynchedSizeField() = default;
 private:
  std::atomic<size_t> val;
};

template<class SizeField>
class AllocAccounting {
 public:
  inline size_t GetNetBytes() const {
    return netBytes.LoadSize();
  }
  inline size_t GetNetObjs() const {
    return netObjs.LoadSize();
  }
  inline size_t GetNetObjBytes() const {
    return netObjBytes.LoadSize();
  }
  inline size_t GetNetLargeObjBytes() const {
    return netLargeObjBytes.LoadSize();
  }

  AllocAccounting() {
    netBytes.Init();
    netObjs.Init();
    netObjBytes.Init();
    netLargeObjBytes.Init();
    totalFreedBytes.Init();
    totalFreedObjBytes.Init();
    totalFreedObjs.Init();
    totalAllocdBytes.Init();
    totalAllocdObjs.Init();
    totalAllocdObjBytes.Init();
  }
  ~AllocAccounting() = default;
  inline size_t TotalAllocdBytes() {
    return totalAllocdBytes.LoadSize();
  }
  inline size_t TotalFreedBytes() {
    return totalFreedBytes.LoadSize();
  }
  inline size_t TotalAllocdObjs() {
    return totalAllocdObjs.LoadSize();
  }
  inline size_t TotalFreedObjs() {
    return totalFreedObjs.LoadSize();
  }
  inline void AtAlloc(size_t objSize, size_t internalSize, bool isLarge) {
    netBytes.Inc(internalSize);
    netObjBytes.Inc(objSize);
    netObjs.Inc(1);
    if (UNLIKELY(isLarge)) {
      netLargeObjBytes.Inc(objSize);
    }
#if ENABLE_HPROF
    if (UNLIKELY(IsHeapStatsEnabled())) {
      // this collects total alloc stats from a time window, instead of net stats
      totalAllocdBytes.Inc(internalSize);
      totalAllocdObjs.Inc(1);
    }
#endif
  }
  inline void AtFree(size_t objSize, size_t internalSize, bool isLarge) {
    netBytes.Dec(internalSize);
    netObjBytes.Dec(objSize);
    netObjs.Dec(1);
    if (UNLIKELY(isLarge)) {
      netLargeObjBytes.Dec(objSize);
    }
#if ENABLE_HPROF
    if (UNLIKELY(IsHeapStatsEnabled())) {
      // this collects total free stats from a time window, instead of net stats
      totalFreedBytes.Inc(internalSize);
      totalFreedObjs.Inc(1);
    }
#endif
  }
  inline void ResetWindowTotal() {
    totalFreedBytes.Init();
    totalFreedObjBytes.Init();
    totalFreedObjs.Init();
    totalAllocdBytes.Init();
    totalAllocdObjs.Init();
    totalAllocdObjBytes.Init();
  }

 private:
  SizeField netBytes;
  SizeField netObjBytes;
  SizeField netObjs;
  SizeField netLargeObjBytes;    // net bytes excluding overhead

  // these are for total numbers in a time window, caution there is no overflow protection
  SizeField totalAllocdBytes;    // total bytes including overhead
  SizeField totalAllocdObjBytes; // total bytes excluding overhead
  SizeField totalAllocdObjs;     // total number of objects allocated
  SizeField totalFreedBytes;     // total bytes including overhead
  SizeField totalFreedObjBytes;  // total bytes excluding overhead
  SizeField totalFreedObjs;      // total number of objects freed
};

// the allocator uses a version that does requires atomic instructions, because multiple
// threads could do the same operation concurrently
using SynchedAllocAccounting = AllocAccounting<AccSynchedSizeField>;
// the mutator uses a light weight version that does not require atomic instructions
using UnsyncAllocAccounting = AllocAccounting<AccUnSynchedSizeField>;

#if LOG_ALLOC_TIMESTAT
// Time statistic obj for measured functions such as allocator obj alloc/release
class TimeStat {
 public:
  uint64_t tmMin;  // min time for an ocurrance of the function
  uint64_t tmMax;  // max time for an ocurrance of the function
  uint64_t tmSum;  // sum total time of all ocurrances of the function
  uint64_t tmCnt;  // count of ourrances of the function

  TimeStat() : tmMin(std::numeric_limits<uint64_t>::max()), tmMax(0), tmSum(0), tmCnt(0) {}

  void Reset() {
    tmMin = std::numeric_limits<uint64_t>::max();
    tmMax = 0;
    tmSum = 0;
    tmCnt = 0;
  }

  // Update time stats with time taken from an ocurrance of the function
  void Update(uint64_t val) {
    tmMin = std::min(val, tmMin);
    tmMax = std::max(val, tmMax);
    tmSum += val;
    ++tmCnt;
  }

  // True if there is recorded timestat data for the function
  bool HasStat() {
    return (tmCnt != 0);
  }

  uint64_t GetMin() {
    return tmMin;
  }
  uint64_t GetMax() {
    return tmMax;
  }
  uint64_t GetSum() {
    return tmSum;
  }
  uint64_t GetCnt() {
    return tmCnt;
  }
  uint64_t GetAvg() {
    return tmSum / tmCnt;
  }
};

// TypeInd list of timed functions
enum {
  kTimeAllocLocal,
  kTimeAllocGlobal,
  kTimeAllocLarge,
  kTimeFreeLocal,
  kTimeFreeGlobal,
  kTimeFreeLarge,
  kTimeReleaseObj,
  kTimeMax
};
#endif

class AllocMutator {
 public:
  virtual void Init() = 0;
  virtual void Fini() = 0;
  virtual void VisitGCRoots(std::function<void(address_t)> visitor) = 0;
  AllocMutator() = default;

#if LOG_ALLOC_TIMESTAT
  void StartTimer() {
    timeStamp = timeutils::NanoSeconds();
  }

  void StopTimer(int typeInd) {
    uint64_t stopTime = timeutils::NanoSeconds();
    timeStat[typeInd].Update(stopTime - timeStamp);
  }

  // reset timestat for all tracked functions
  void ResetTimers() {
    for (int typeInd = 0; typeInd < kTimeMax; ++typeInd) {
      timeStat[typeInd].Reset();
    }
  }

  uint64_t GetTimerMin(int typeInd) {
    return timeStat[typeInd].GetMin();
  }
  uint64_t GetTimerMax(int typeInd) {
    return timeStat[typeInd].GetMax();
  }
  uint64_t GetTimerSum(int typeInd) {
    return timeStat[typeInd].GetSum();
  }
  uint64_t GetTimerCnt(int typeInd) {
    return timeStat[typeInd].GetCnt();
  }
  uint64_t GetTimerAvg(int typeInd) {
    return timeStat[typeInd].GetAvg();
  }

  void SuspendFreeObjTimeStat() {
    statOn = false;
  }
  void ResumeFreeObjTimeStat() {
    statOn = true;
  }
  bool DoFreeObjTimeStat() {
    return statOn;
  }
#endif

  virtual ~AllocMutator() = default;
  AllocMutator(AllocMutator const&) = delete;
  void operator=(AllocMutator const &x) = delete;
  inline UnsyncAllocAccounting &GetAllocAccount() {
    return account;
  }

 public:
  // the mutator uses a light weight version that does not require atomic instructions
  // this is currently not properly instrumented, use the global/atomic version instead
  UnsyncAllocAccounting account;
#if LOG_ALLOC_TIMESTAT
  // Note: timestat for allocator FreeObj is not collected in following cases:
  // 1. objects freed by MS collector via RosAllocImpl::FreeAllIf
  // 2. objects freed by non alloc-mutator threads calling the free function
  // 3. objects freed by naive RC collector via ReleaseObj call are not individually timestat'd
  //    again since they are already timestat'd in the ReleaseObj operation.
  TimeStat timeStat[kTimeMax];
  uint64_t timeStamp;
  bool statOn = true;
#endif
}; // class AllocMutator

// Globals used by fast allocation containing context information.
// Putting all the information together helps avoid virtual calls (Collector::Instance()).
// It also reduces adrp usage in the generated code, saving a few lines of code.
struct FastAllocData {
  static FastAllocData data;
  std::atomic<bool> isConcurrentSweeping = { false };
  bool isConcurrentMarking = false;
  bool isGCOnly = false;
  std::atomic<size_t> allocatedInternalSize = { 0 };
  MrtBitmap *bm;
};

#if ALLOC_USE_FAST_PATH
#define FAST_ALLOC_ACCOUNT_ADD(s) \
    (void) FastAllocData::data.allocatedInternalSize.fetch_add((s), std::memory_order_relaxed)
#define FAST_ALLOC_ACCOUNT_SUB(s) \
    (void) FastAllocData::data.allocatedInternalSize.fetch_sub((s), std::memory_order_relaxed)
#else
#define FAST_ALLOC_ACCOUNT_ADD(s)
#define FAST_ALLOC_ACCOUNT_SUB(s)
#endif

// Allocator abstract class
class Allocator {
 public:
  // For methods that visit objects via callback functions.
  using AddressVisitor = std::function<void(address_t)>;
  using VisitorFactory = std::function<AddressVisitor()>;

  // returns the total number of objs allocated
  inline size_t AllocatedObjs() const {
    return account.GetNetObjs();
  }

  // returns the total bytes that has been occupied
  // to be specific, this is the sum of internal size of all allocated objects
  inline size_t AllocatedMemory() const {
#if ALLOC_USE_FAST_PATH
    return FastAllocData::data.allocatedInternalSize.load(std::memory_order_relaxed);
#else
    return account.GetNetBytes();
#endif
  }

  // returns the total size of the objects allocated
  // the difference between requested memory and allocated memory, is that
  // allocated memory sums internal size, whereas requested memory sums raw obj size (no header)
  inline size_t RequestedMemory() const {
    return account.GetNetObjBytes();
  }

  inline SynchedAllocAccounting &GetAllocAccount() {
    return account;
  }

  // callback for mutator finalisation
  inline void PostMutatorFini(AllocMutator &mutator);

  // callback before allocation
  __attribute__ ((always_inline))
  void PreObjAlloc(address_t objAddress, size_t objSize) const;

  // callback after allocation
  // objSize is the requested object size
  // internalSize is the size including overhead
  __attribute__ ((always_inline))
  void PostObjAlloc(address_t objAddress, size_t objSize, size_t internalSize);

  // callback before free. returns the object size
  template<bool isFast = false>
  __attribute__ ((always_inline))
  size_t PreObjFree(address_t objAddress) const;

  // callback after free
// internalSize is the bytes occupied by the object including overhead
  template<bool isFast = false>
  __attribute__ ((always_inline))
  void PostObjFree(address_t objAddress, size_t objSize, size_t internalSize);

  virtual void Init(const VMHeapParam&) = 0;

  // Allocate space for a new object of the requested size
  virtual address_t NewObj(size_t size) = 0;

  static void ReleaseResource(address_t obj);

  virtual ~Allocator() {
    oome = nullptr;
  }
  Allocator();
  void NewOOMException();

  // Allocation tracking support, set callback
  void SetAllocRecordingCallback(std::function<void(address_t, size_t)> allocRecordingCallbackFunc) {
    mAllocRecordingCallbackFunc = allocRecordingCallbackFunc;
  }

 public:
  MObject *oome; // this doesn't belong here!
  SynchedAllocAccounting account;
 protected:
  std::atomic<bool> oomeCreated;
  ALLOC_MUTEX_TYPE globalLock;
  // Allocation tracking support, recorder callback
  std::function<void(address_t, size_t)> mAllocRecordingCallbackFunc = nullptr;
};
}

#endif

