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
#ifndef MAPLE_RUNTIME_SPACE_H
#define MAPLE_RUNTIME_SPACE_H

#include <cstdint>
#include <atomic>
#include <memory>
#include <sys/mman.h>
#include "page_map.h"
#include "address.h"
#include "panic.h"
#include "utils/time_utils.h"
#include "alloc_config.h"
#include "mem_map.h"
#include "cartesian_tree.h"

namespace maplert {
class Space;
class SpacePageManager {
 public:
  SpacePageManager(Space &allocSpaceVal, uintptr_t begin) : allocSpace(allocSpaceVal), baseAddress(begin) {}
  ~SpacePageManager() = default;
  void SetBaseAddress(uintptr_t baseAddr) {
    baseAddress = baseAddr;
  }

  // given size in bytes we return the size normalized to pages (4k)
  uint32_t CalcChunkSize(size_t regionSize) const;

  // given index of a region, calculate the page address
  uintptr_t GetRegAddrFromIdx(uint32_t idx) const;

  // given the first page and the size of memory region in bytes, we add
  // it to the set of free chunks
  void AddRegion(uintptr_t regionAddr, size_t regionSize);
  // add contiguous pages of count page_count starting with first_page
  void AddPages(uintptr_t firstPage, uint32_t pageCount);

  // release all free pages back to the system
  void ReleaseAllFreePages(size_t freeBytes, const MemMap &memMap, bool isAggressive);

  // Given a request size, we split the first chunk and return the
  // address to the first page
  uintptr_t GetChunk(size_t reqSize);

  uint32_t CalcPageIdx(uintptr_t pageAddr) const;

  size_t GetLargestChunkSize();
 private:
  friend Space;
  using TreeType = CartesianTree<uint32_t, uint32_t>;
  Space &allocSpace;
  uintptr_t baseAddress;
  TreeType pageCTree;
}; // end of SpacePageManager

class Space {
 public:
  // These parameters can be set via static methods Space::SetXXX().
  // Currently they are called before Space::Init(). They take
  // the values from the configuration of the virtual machine.
  // Default configuration (please check the corresponding .prop files to confirm):
  // kReleasePageAtFree/kReleasePageAtTrim: set in alloc_config.h
  // heap start size: 8m
  // heap size: 512m
  // heap growth limit: 384m
  // heap min free: 2m
  // heap max free: 8m
  // heap target utilization: 0.75
  // ignore max footprint: see runtime.cc
  //
  // they are not constant..
  // whether to release pages immediately after we free them
  static bool kReleasePageAtFree;
  // whether to release free pages all together at a separate phase we call "trim"
  static bool kReleasePageAtTrim;
  // the initial size of the heap
  static size_t kHeapStartSize;
  // the maximum heap size
  static size_t kHeapSize;
  // the default heap size
  // In ART, kHeapGrowthLimit is used as the heap size for all processes,
  // UNLESS the manifest file set the largeHeap property to true,
  // in which case the heap size is kHeapSize.
  // In our Maple, we ignore this and use the kHeapSize directly (for now),
  // see RosAllocImpl::Init()
  static size_t kHeapGrowthLimit;
  // the minimum size of free pages, unused
  static size_t kHeapMinFree;
  // the maximum size of free pages, used to trigger trim
  static size_t kHeapMaxFree;
  // the following is unused
  // target utilization rate of the heap
  static float kHeapTargetUtilization;
  // unused
  static bool kIgnoreMaxFootprint;
  // 1s interval for trim, to prevent impeding performance
  static const uint64_t kMinTrimInterval = 1000; // ms
  // non-aggressive trim max time
  static const uint64_t kMaxTrimTime = 10000; // us

  // this controls how many free pages we don't release (return to the os)
  static inline size_t TargetFreePageSize() {
    return Space::kHeapMaxFree >> 1;
  }

  static inline void SetHeapStartSize(size_t heapStartSize) {
    Space::kHeapStartSize = heapStartSize;
  }
  static inline void SetHeapSize(size_t heapSize) {
    Space::kHeapSize = heapSize;
  }
  static inline void SetHeapGrowthLimit(size_t heapGrowthLimit) {
    Space::kHeapGrowthLimit = heapGrowthLimit;
  }
  static inline void SetHeapMinFree(size_t heapMinFree) {
    Space::kHeapMinFree = heapMinFree;
  }
  static inline void SetHeapMaxFree(size_t heapMaxFree) {
    Space::kHeapMaxFree = heapMaxFree;
  }
  static inline void SetHeapTargetUtilization(float heapTargetUtilization) {
    Space::kHeapTargetUtilization = heapTargetUtilization;
  }
  static inline void SetIgnoreMaxFootprint(bool ignoreMaxFootprint) {
    Space::kIgnoreMaxFootprint = ignoreMaxFootprint;
  }

 public:
  string name; // full name of the space
  string tag; // tag to identify the memory region in smaps
  bool isManaged; // whether the space is under the management of RC/GC
  // maximum memory allowed for the space
  size_t maxCapacity;
  // mem map is a wrapper of system calls like mmap(), madvise(), mprotect()
  MemMap *memMap;
  // page map labels each page with a type, e.g., run type, large obj type
  PageMap &pageMap;
  // The beginning of the storage for fast access.
  uint8_t *begin;
  // current end of the space.
  atomic<uint8_t*> end;
  // we do out-of-lock reads to the following fields for gc trigger heuristics
  // as long as this fields are aligned, we don't have to worry about atomicity problems
  // total size of pages allocated
  size_t allocatedPageSize;
  // total size of pages fetched from the kernel
  // 'release' is the action of returning physical memory pages back to the kernel
  // if a page is 'non-released', it means we still occupy the corresponding physical memory
  size_t nonReleasedPageSize;

  friend class SpacePageManager;
  // the page manager manages all of the free pages (including released pages)
  SpacePageManager pageManager;

  // records the last time we trimed
  uint64_t lastTrimTime;
  inline bool IsTrimAllowedAtTheMoment() {
    uint64_t currTime = timeutils::MilliSeconds();
    if (currTime - lastTrimTime < kMinTrimInterval) {
      return false;
    } else {
      return true;
    }
  }

 public:
  static const int kPagesOneTime = kRosimplDefaultPageOneTime;
  Space(const string &nameVal, const string &tagVal, bool managed, PageMap &pageMapVal);
  virtual ~Space();
  Space(const Space &that) = delete;
  Space(Space &&that) = delete;
  Space &operator=(const Space &that) = delete;
  Space &operator=(Space &&that) = delete;

  void Init(uint8_t *reqStart = nullptr);

  // Name of the space. May vary, for example before/after the Zygote fork.
  const char *GetName() const {
    return name.c_str();
  }

  inline uint8_t *GetEnd() const {
    return end.load(std::memory_order_acquire);
  }
  inline uint8_t *GetEndRelaxed() const {
    return end.load(std::memory_order_relaxed);
  }

  size_t GetMaxCapacity() const {
    return maxCapacity;
  }

  inline uint8_t *GetBegin() const {
    return begin;
  }

  size_t GetSize() const {
    return static_cast<size_t>(GetEnd() - GetBegin());
  }
  size_t GetSizeRelaxed() const {
    return static_cast<size_t>(GetEndRelaxed() - GetBegin());
  }

  // Get the size of the allocated pages
  inline size_t GetAllocatedPageSize() const {
    return allocatedPageSize;
  }
  // Get the size of the non-released pages
  inline size_t GetNonReleasedPageSize() const {
    return nonReleasedPageSize;
  }
  // Get the size of the free pages
  inline size_t GetFreePageSize() const {
    return nonReleasedPageSize - allocatedPageSize;
  }

  inline size_t HeapMaxFreeByUtilization() const {
    // maximum is twice the ideal
    // cap this for the same reason
    return std::min(TargetFreePageSizeByUtilization() << 1, kHeapMaxFree);
  }

  size_t TargetFreePageSizeByUtilization() const;

  void SetEnd(uint8_t *endVal) {
    // This should be visible by other thread
    // 1. expand happens under global_lock, new_expand space obj
    //    must be visible after new "end_" visible to other thread.
    // 2. heapCurSize update before end_, and end_ store with release order
    //
    // when other thread see newly expand space obj, it must see new heapCurSize
    if (isManaged) {
      HeapStats::OnHeapExtended(static_cast<size_t>(endVal - GetBegin()));
    }
    end.store(endVal, memory_order_release);
  }

  void SetBegin(uint8_t *beginVal) {
    begin = beginVal;
    if (isManaged) {
      HeapStats::OnHeapCreated(reinterpret_cast<address_t>(beginVal));
    }
    // assume begin only set once in Space, otherwise need update heapCurSize
  }

  inline void RecordReleasedToNonReleased(size_t pageCount) {
    nonReleasedPageSize += ALLOCUTIL_PAGE_CNT2BYTE(pageCount);
  }

  // returns the number of pages free within the reserved memry space
  size_t GetAvailPageCount() const;

  uintptr_t GetChunk(size_t reqSize);
  size_t GetLargestChunkSize();

  inline bool HasAddress(address_t addr) {
    const uint8_t *objPtr = reinterpret_cast<const uint8_t*>(addr);
    return objPtr >= GetBegin() && objPtr < GetEnd();
  }

  void Extend(size_t deltaSize);
  address_t Alloc(size_t reqSize, bool allowExtension);
  size_t ReleaseFreePages(bool aggressive = false);
  void FreeRegion(address_t addr, size_t pgCnt);
}; // class Space
} // namespace maplert

#endif // MAPLE_RUNTIME_SPACE_H
