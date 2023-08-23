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
#include "allocator/space.h"

#include <atomic>
#include <cinttypes>
#include <sys/types.h>
#include <sys/syscall.h>
#include "chosen.h"
#include "collector/stats.h"
#include "yieldpoint.h"

namespace maplert {
using namespace std;

bool Space::kReleasePageAtFree = kRosimplReleasePageAtFree;
bool Space::kReleasePageAtTrim = kRosimplReleasePageAtTrim;
// this duplicates with heap initial size
size_t Space::kHeapStartSize = ROSIMPL_DEFAULT_HEAP_START_SIZE;
size_t Space::kHeapSize = ROSIMPL_DEFAULT_HEAP_SIZE;
size_t Space::kHeapGrowthLimit = ROSIMPL_DEFAULT_HEAP_GROWTH_LIMIT;
size_t Space::kHeapMinFree = ROSIMPL_DEFAULT_HEAP_MIN_FREE;
size_t Space::kHeapMaxFree = ROSIMPL_DEFAULT_HEAP_MAX_FREE;
// the following is unused
float Space::kHeapTargetUtilization = kRosimplDefaultHeapTargetUtilization;
bool Space::kIgnoreMaxFootprint = kRosimplDefaultIgnoreMaxFootprint;

//  SpacePageManager
uint32_t SpacePageManager::CalcPageIdx(uintptr_t pageAddr) const {
  return static_cast<uint32_t>(ALLOCUTIL_PAGE_BYTE2CNT(pageAddr - baseAddress));
}

// given the first page and the size of region in bytes, we add it
// to the set
void SpacePageManager::AddRegion(uintptr_t regionAddr, size_t regionSize) {
  uint32_t pageCount = CalcChunkSize(regionSize);
  AddPages(regionAddr, pageCount);
}

// add contiguous pages of count page_count starting with first_page
void SpacePageManager::AddPages(uintptr_t firstPage, uint32_t pageCount) {
  // we should check that the page is within the limit in future
  uint32_t pageIdx = CalcPageIdx(firstPage);
  // add the new entry to the set
  if (UNLIKELY(!pageCTree.MergeInsert(pageIdx, pageCount))) {
    LOG(ERROR) << "free page insertion failed, inconsistency, " << pageIdx << ", " << pageCount << maple::endl;
  }
}

void SpacePageManager::ReleaseAllFreePages(size_t freeBytes, const MemMap &memMap, bool isAggressive) {
  if (pageCTree.Empty()) {
    return;
  }
  uint64_t beginTime = timeutils::MicroSeconds();
  TreeType::Iterator it(pageCTree);
  auto next = it.Next();
  size_t totalReleasedBytes = 0U;
  size_t targetFreeBytes = allocSpace.TargetFreePageSizeByUtilization();
  size_t targetReleaseBytes = freeBytes > targetFreeBytes ? freeBytes - targetFreeBytes : 0U;
  while (next != nullptr) {
    // control the size of unreleased pages
    uintptr_t releaseBeginAddr = GetRegAddrFromIdx(next->Idx());
    size_t releaseBytes = ALLOCUTIL_PAGE_CNT2BYTE(next->Cnt());
    if (UNLIKELY(!isAggressive && // aggressive trim ignores these conditions
                 (timeutils::MicroSeconds() - beginTime > Space::kMaxTrimTime ||
                  totalReleasedBytes >= targetReleaseBytes))) {
      // after we release enough many pages, we stop (reserve a certain number of free pages)
      // also, 10ms timeout
      break;
    }

    // release
    uint32_t relEndIdx = next->Idx() + next->Cnt();
    size_t c = allocSpace.pageMap.SetRangeAndCount(next->Idx(), relEndIdx, kPReleased);
    if (c) {
      (void)memMap.ReleaseMem(releaseBeginAddr, releaseBytes);
      totalReleasedBytes += ALLOCUTIL_PAGE_CNT2BYTE(c);
    }
    next = it.Next();
  }
  allocSpace.nonReleasedPageSize -= totalReleasedBytes;
  size_t allocated = stats::gcStats->CurAllocBytes();
  float utilization = static_cast<float>(allocated) / allocSpace.nonReleasedPageSize;
  LOG2FILE(kLogTypeAllocFrag) <<
      (isAggressive ? "    A" : "Non-a") << "ggressive trim time " <<
      (timeutils::MicroSeconds() - beginTime) << "us, released " << totalReleasedBytes <<
      ", post-trim utilization (" << allocated <<
      "/" << allocSpace.allocatedPageSize <<
      "/" << allocSpace.nonReleasedPageSize << ") " <<
      std::setprecision(2) << std::fixed << utilization <<
      " (target " << Space::kHeapTargetUtilization << ")\n";
}

// Given a request size in bytes, we split the first chunk and return the
// address to the first page
uintptr_t SpacePageManager::GetChunk(size_t reqSize) {
  uint32_t idx = 0U;
  return pageCTree.Find(idx, CalcChunkSize(reqSize)) ? GetRegAddrFromIdx(idx) : 0U;
}

// given size in bytes we return the size normalized to pages (4k)
uint32_t SpacePageManager::CalcChunkSize(size_t regionSize) const {
  return static_cast<uint32_t>(ALLOCUTIL_PAGE_BYTE2CNT(ALLOCUTIL_PAGE_RND_UP(regionSize)));
}

// given index of a region, calculate the page address
uintptr_t SpacePageManager::GetRegAddrFromIdx(uint32_t idx) const {
  size_t offset = ALLOCUTIL_PAGE_CNT2BYTE(idx);
  return offset + baseAddress;
}

// Returns the size of largest chunk currently in the page set.
size_t SpacePageManager::GetLargestChunkSize() {
  return ALLOCUTIL_PAGE_CNT2BYTE(pageCTree.Top());
}

// Space
Space::Space(const std::string &fullname, const std::string &tagVal, bool managed, PageMap &pageMapVal)
    : name(fullname),
      tag(tagVal),
      isManaged(managed),
      maxCapacity(0),
      memMap(nullptr),
      pageMap(pageMapVal),
      begin(nullptr),
      end(0),
      allocatedPageSize(0),
      nonReleasedPageSize(0),
      pageManager(*this, 0U),
      lastTrimTime(0) {
}

void Space::Init(uint8_t *reqBegin) {
  LOG2FILE(kLogTypeAllocator) << "Initializing the heap.." << std::endl;
  LOG2FILE(kLogTypeAllocator) << "  heap start size: " << Space::kHeapStartSize << std::endl;
  LOG2FILE(kLogTypeAllocator) << "  heap maximum size: " << Space::kHeapSize << std::endl;
  LOG2FILE(kLogTypeAllocator) << "  heap growth limit: " << Space::kHeapGrowthLimit << std::endl;
  LOG2FILE(kLogTypeAllocator) << "  heap minimum free size: " << Space::kHeapMinFree << std::endl;
  LOG2FILE(kLogTypeAllocator) << "  heap maximum free size: " << Space::kHeapMaxFree << std::endl;
  LOG2FILE(kLogTypeAllocator) << "  heap target utilization rate: " << Space::kHeapTargetUtilization << std::endl;
  LOG2FILE(kLogTypeAllocator) << "  is ignoring max footprint: " << Space::kIgnoreMaxFootprint << std::endl;
  maxCapacity = Space::kHeapGrowthLimit;
  MemMap::Option opt = MemMap::kDefaultOptions;
  opt.tag = tag.c_str();
  opt.reqBase = reinterpret_cast<void*>(reqBegin);
#if !ROSIMPL_ENABLE_ASSERTS
  opt.protAll = true;
#endif
  memMap = MemMap::MapMemory(maxCapacity, Space::kHeapStartSize, opt);
  begin = reqBegin;

  uint8_t *baseAddress = reinterpret_cast<uint8_t*>(memMap->GetBaseAddr());
  SetBegin(baseAddress);
  SetEnd(reinterpret_cast<uint8_t*>(memMap->GetCurrEnd()));
  uintptr_t startAddress = reinterpret_cast<uintptr_t>(baseAddress);
  allocatedPageSize = 0U;
  nonReleasedPageSize = 0U;
  pageManager.pageCTree.Init(maxCapacity);
  pageManager.SetBaseAddress(startAddress);
  pageManager.AddRegion(startAddress, GetSize());
}


// returns the number of pages free within the reserved memory space
size_t Space::GetAvailPageCount() const {
  return ALLOCUTIL_PAGE_BYTE2CNT(GetSize() - allocatedPageSize);
}

uintptr_t Space::GetChunk(size_t reqSize) {
  uintptr_t addr = pageManager.GetChunk(reqSize);
  if (addr != 0) {
#if DEBUG_RUNLOCK_OWNER
    if (UNLIKELY(!memMap->ProtectMem(addr, reqSize, PROT_READ | PROT_WRITE))) {
      LOG(FATAL) << "failed to mprotect " << std::hex << addr << " of size " << std::dec << reqSize;
    }
#endif
    allocatedPageSize += reqSize;
  }
  return addr;
}

size_t Space::GetLargestChunkSize() {
  return pageManager.GetLargestChunkSize();
}

size_t Space::TargetFreePageSizeByUtilization() const {
  size_t allocated = stats::gcStats->CurAllocBytes();
  // this should be guaranteed by the option parser
  __MRT_ASSERT(kHeapTargetUtilization > 0, "invalid utilization");
  __MRT_ASSERT(kHeapTargetUtilization < 1, "invalid utilization");
  size_t targetTotal = static_cast<size_t>(allocated / kHeapTargetUtilization);
  size_t targetFree = std::max(targetTotal, allocatedPageSize) - allocatedPageSize;
  // we want to cap this: for larger heap, assuming usage do not grow that much faster
  return std::min(targetFree, TargetFreePageSize());
}

void Space::Extend(size_t deltaSize) {
  LOG2FILE(kLogTypeAllocator) << "extending space size delta " << deltaSize << std::endl;
  size_t reqSize = ALLOCUTIL_PAGE_RND_UP(deltaSize);
  size_t currentSize = GetSize();
  if (currentSize >= maxCapacity || reqSize > maxCapacity) {
    LOG(ERROR) << "space exceeded maximum capacity." << maple::endl;
    return;
  }
  size_t oneTimeReq = ALLOCUTIL_PAGE_CNT2BYTE(kPagesOneTime);
  size_t remainingSize = maxCapacity - currentSize;

  size_t expandValue = std::min(remainingSize, std::max(reqSize, oneTimeReq));
  LOG2FILE(kLogTypeAllocator) << "    requesting " << expandValue << std::endl;
  bool extendResult = memMap->Extend(expandValue);
  if (!extendResult) {
    LOG(ERROR) << "allocator: extending space failed." << maple::endl;
    return;
  }
  address_t oldEndAddr = reinterpret_cast<address_t>(GetEndRelaxed());
  SetEnd(reinterpret_cast<uint8_t*>(memMap->GetCurrEnd()));
  LOG2FILE(kLogTypeAllocator) << "    setting new end address 0x" <<
      std::hex << reinterpret_cast<address_t>(GetEndRelaxed()) << std::endl;
  // we add the new chunk to the page manager so that it can coalesce with
  // any existing chunk at the end of the space
  pageManager.AddRegion(oldEndAddr, expandValue);
}

address_t Space::Alloc(size_t reqSize, bool allowExtension) {
  address_t retAddress = 0U;
  if (GetSize() - allocatedPageSize >= reqSize) {
    retAddress = GetChunk(reqSize);
    if (retAddress != 0) {
      return retAddress;
    }
  }

  if (allowExtension) {
    Extend(reqSize);
    retAddress = GetChunk(reqSize);
  }
  return retAddress;
}

// release the physical memory of free pages, using madvise()
size_t Space::ReleaseFreePages(bool aggressive) {
  // don't release when gc running, too slow
  if (Collector::Instance().InStartupPhase() ||
      Collector::Instance().IsGcRunning() || WorldStopped()) {
    return 0;
  }
  size_t free_bytes = GetFreePageSize();
  // if aggressive, it ignores the threshold and the timer
  if (aggressive || (IsTrimAllowedAtTheMoment() && free_bytes > HeapMaxFreeByUtilization())) {
    pageManager.ReleaseAllFreePages(free_bytes, *memMap, aggressive);
    lastTrimTime = timeutils::MilliSeconds();
  }
  // if any page was released, the size must have decreased
  return free_bytes - GetFreePageSize();
}

void Space::FreeRegion(address_t addr, size_t pgCnt) {
  pageManager.AddPages(addr, static_cast<uint32_t>(pgCnt));
  size_t memSize = ALLOCUTIL_PAGE_CNT2BYTE(pgCnt);
#if DEBUG_RUNLOCK_OWNER
  if (UNLIKELY(!memMap->ProtectMem(addr, memSize, PROT_NONE))) {
    LOG(FATAL) << "failed to mprotect " << std::hex << addr << " of size " << std::dec << memSize;
  }
#endif
  allocatedPageSize -= memSize;
  if (Space::kReleasePageAtFree) {
    static_cast<void>(ReleaseFreePages(false));
  }
}

Space::~Space() {
  LOG2FILE(kLogTypeAllocator) << "Destructing RoSSpace" << std::endl;
  if (memMap != nullptr) {
    delete(memMap);
    memMap = nullptr;
  }
  begin = nullptr;
  LOG2FILE(kLogTypeAllocator) << "Done Destructing RoSSpace" << std::endl;
}
} // namesapce maplert
