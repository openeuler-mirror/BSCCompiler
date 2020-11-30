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
#include "allocator/mem_map.h"

#include <sys/mman.h>
#include <algorithm>
#include "sizes.h"
#include "syscall.h"
#include "libs.h"

namespace maplert {
using namespace std;

// not thread safe, do not call from multiple threads
MemMap *MemMap::MapMemory(size_t reqSize, size_t initSize, const Option &opt) {
  void *mappedAddr = MAP_FAILED;
  reqSize = AllocUtilRndUp<size_t>(reqSize, ALLOCUTIL_PAGE_SIZE);

  LOG2FILE(kLogTypeAllocator) << "MemMap::MapMemory size " << reqSize << std::endl;
  if (kEnableRange && opt.reqRange) {
    // repeatedly map and guarantee the result is in the specified range
    mappedAddr = MemMapInRange(opt.lowestAddr, opt.highestAddr, reqSize, PROT_NONE, opt.flags);
  } else {
    mappedAddr = mmap(opt.reqBase, reqSize, PROT_NONE, opt.flags, -1, 0);
  }

  bool failure = false;
  if (mappedAddr != MAP_FAILED) {
    MRT_PRCTL(mappedAddr, reqSize, opt.tag);
    // if protAll, all memory is protected at creation, and we never change it (save time)
    size_t protSize = opt.protAll ? reqSize : initSize;
    if (!ProtectMemInternal(mappedAddr, protSize, opt.prot)) {
      failure = true;
      LOG(ERROR) << "MemMap::MapMemory mprotect failed" << maple::endl;
      ALLOCUTIL_MEM_UNMAP(mappedAddr, reqSize);
    }
  } else {
    failure = true;
  }
  if (failure) {
    LOG(FATAL) << "MemMap::MapMemory failed reqSize: " << reqSize << " initSize: " << initSize <<
        " reqRange: " << opt.reqRange;
  }

  LOG2FILE(kLogTypeAllocator) << "MemMap::MapMemory size " << reqSize <<
      " successful at " << mappedAddr << std::endl;
  return new MemMap(mappedAddr, initSize, reqSize, opt.protAll, opt.prot);
}

MemMap *MemMap::CreateMemMapAtExactAddress(void *addr, size_t size, const Option &opt) {
  const size_t pageSize = static_cast<size_t>(sysconf(_SC_PAGESIZE));
  uintptr_t uaddr = reinterpret_cast<uintptr_t>(addr);
  if (uaddr % pageSize != 0) {
    LOG(FATAL) << "try to mmap at address " << std::hex << uaddr << std::dec << " which is not page-aligned" <<
        maple::endl;
  }

  void *mappedAddr = mmap(addr, size, opt.prot, opt.flags | MAP_FIXED, -1, 0);
  if (mappedAddr != MAP_FAILED && uaddr == reinterpret_cast<uintptr_t>(mappedAddr)) {
    MRT_PRCTL(mappedAddr, size, opt.tag);
    return new MemMap(mappedAddr, size, size, opt.protAll, opt.prot);
  } else {
    LOG(FATAL) << "MemMap::CreateMemMapAtExactAddress failed to map memory at 0x: " << std::hex << uaddr <<
        std::dec << " size: " << size << maple::endl;
  }

  return nullptr;
}

MemMap::MemMap(void *baseAddr, size_t initSize, size_t mappedSize, bool protAll, int prot)
    : memBaseAddr(baseAddr),
      memCurrSize(initSize),
      memMappedSize(mappedSize),
      protOnce(protAll),
      memProt(prot) {
  memCurrEndAddr = reinterpret_cast<void*>(reinterpret_cast<address_t>(memBaseAddr) + memCurrSize);
  memMappedEndAddr = reinterpret_cast<void*>(reinterpret_cast<address_t>(memBaseAddr) + memMappedSize);
}

bool MemMap::ProtectMemInternal(void *addr, size_t size, int prot) {
  LOG2FILE(kLogTypeAllocator) << "MemMap::ProtectMem " << addr << ", size " <<
      size << ", prot " << prot << std::endl;
  int ret = mprotect(addr, size, prot);
  return (ret == 0);
}

bool MemMap::ProtectMem(address_t addr, size_t size, int prot) const {
  if (addr >= reinterpret_cast<address_t>(memBaseAddr) &&
      addr + size <= reinterpret_cast<address_t>(memCurrEndAddr)) {
    return ProtectMemInternal(reinterpret_cast<void*>(addr), size, prot);
  }
  return false;
}

void MemMap::UpdateCurrEndAddr() {
  address_t startAddr = (reinterpret_cast<address_t>(memBaseAddr));
  memCurrEndAddr = reinterpret_cast<void*>(startAddr + memCurrSize);
}

bool MemMap::Extend(size_t reqSize) {
  if (memCurrSize >= memMappedSize) {
    LOG(ERROR) << "MemMap::Extend failed, curr size " << memCurrSize <<
        ", mapped size " << memMappedSize << maple::endl;
    return false;
  }
  size_t newCurrSize = memCurrSize + reqSize;
  if (newCurrSize > memMappedSize) {
    LOG(ERROR) << "MemMap::Extend invalid new size " << newCurrSize <<
        ", mapped size " << memMappedSize << maple::endl;
    return false;
  }
  if (!protOnce && !ProtectMemInternal(memCurrEndAddr, reqSize, memProt)) {
    LOG(ERROR) << "MemMap::Extend mprotect failed" << maple::endl;
    return false;
  }
  memCurrSize = newCurrSize;
  UpdateCurrEndAddr();
  LOG2FILE(kLogTypeAllocator) << "MemMap::Extend successful, curr size " <<
      memCurrSize << ", mapped size " << memMappedSize << std::endl;
  return true;
}

bool MemMap::ReleaseMem(address_t releaseBeginAddr, size_t size) const {
  address_t baseAddr = reinterpret_cast<address_t>(memBaseAddr);
  address_t currEndAddr = reinterpret_cast<address_t>(memCurrEndAddr);
  address_t releaseEndAddr = releaseBeginAddr + size;
  if (releaseBeginAddr < baseAddr || releaseEndAddr > currEndAddr) {
    return false;
  }
  ALLOCUTIL_MEM_MADVISE(releaseBeginAddr, size, MADV_DONTNEED);
  return true;
}

// resize the memory map by releasing the pages at the end (munmap)
bool MemMap::Shrink(size_t newSize) {
  if (newSize >= memMappedSize) {
    return false;
  }
  address_t newEndAddr = reinterpret_cast<address_t>(memBaseAddr) + newSize;
  memMappedEndAddr = reinterpret_cast<void*>(newEndAddr);
  ALLOCUTIL_MEM_UNMAP(memMappedEndAddr, memMappedSize - newSize);
  memMappedSize = newSize;
  memCurrSize = std::min(newSize, memCurrSize);
  return true;
}

void *MemMap::MemMapInRange(address_t lowestAddr, address_t highestAddr,
                            size_t reqSize, int prot, int flags, int fd, int offset) {
  lowestAddr = AllocUtilRndUp<address_t>(lowestAddr, ALLOCUTIL_PAGE_SIZE);
  highestAddr = AllocUtilRndDown<address_t>(highestAddr, ALLOCUTIL_PAGE_SIZE);
  if (lowestAddr >= highestAddr || highestAddr - lowestAddr < reqSize) {
    LOG(ERROR) << "MemMap::MemMapInRange illegal range [" << lowestAddr <<
        ", " << highestAddr << "]" << maple::endl;
    return MAP_FAILED;
  }
  void *mappedAddr = MAP_FAILED;
  highestAddr -= reqSize;
  AllocUtilRand<address_t> randAddr(lowestAddr, highestAddr);
  if (kEnableRandomMemStart) {
    // The hint doesn't guarantee the actually mapped address is in the specified range.
    // If the hint address is already occupied, mmap will try other arbitrary addresses.
    // We need to check the result, and repeat if fails.
    int repeat = 10;
    while (mappedAddr == MAP_FAILED && (--repeat) != 0) {
      address_t hint = AllocUtilRndUp(randAddr.next(), ALLOCUTIL_PAGE_SIZE);
      mappedAddr = MemMapInRangeInternal(hint, lowestAddr, highestAddr,
                                         reqSize, prot, flags | MAP_FIXED, fd, offset);
    }
  }
  if (mappedAddr == MAP_FAILED) {
    for (address_t addr = lowestAddr; addr <= highestAddr; addr += ALLOCUTIL_PAGE_SIZE) {
      mappedAddr = MemMapInRangeInternal(addr, lowestAddr, highestAddr,
                                         reqSize, prot, flags | MAP_FIXED, fd, offset);
      if (mappedAddr != MAP_FAILED) {
        break;
      }
    }
  }
  if (mappedAddr == MAP_FAILED) {
    LOG(ERROR) << "MemMap::MemMapInRange failed" << maple::endl;
  } else {
    LOG2FILE(kLogTypeAllocator) << "MemMap::MemMapInRange returns " << mappedAddr << std::endl;
  }
  return mappedAddr;
}

void *MemMap::MemMapInRangeInternal(address_t hint,
                                    address_t lowestAddr, address_t highestAddr,
                                    size_t reqSize, int prot, int flags,
                                    int fd, int offset) {
  void *mappedAddr = mmap(reinterpret_cast<void*>(hint), reqSize, prot, flags, fd, offset);
  if (mappedAddr == MAP_FAILED) {
    LOG2FILE(kLogTypeAllocator) <<
        FormatString("map memory at %p: hint %p, low %p, high %p, size %zu, prot %x, flags %x, fd %d, offset %d",
                     mappedAddr, hint, lowestAddr, highestAddr, reqSize, prot, flags, fd, offset);
  } else if (reinterpret_cast<address_t>(mappedAddr) < lowestAddr ||
             reinterpret_cast<address_t>(mappedAddr) > highestAddr) {
    LOG2FILE(kLogTypeAllocator) <<
        FormatString("map memory at %p: hint %p, low %p, high %p, size %zu, prot %x, flags %x, fd %d, offset %d",
                     mappedAddr, hint, lowestAddr, highestAddr, reqSize, prot, flags, fd, offset);
    ALLOCUTIL_MEM_UNMAP(mappedAddr, reqSize); // roll back
    mappedAddr = MAP_FAILED;
  }
  return mappedAddr;
}

MemMap::~MemMap() {
  memBaseAddr = nullptr;
  memCurrEndAddr = nullptr;
  memMappedEndAddr = nullptr;
}
} // namespace maplert
