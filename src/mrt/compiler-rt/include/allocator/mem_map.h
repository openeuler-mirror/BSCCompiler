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
#ifndef MAPLE_RUNTIME_ALLOC_MEM_MAP_H
#define MAPLE_RUNTIME_ALLOC_MEM_MAP_H

#include <sys/mman.h>
#include "alloc_utils.h"

namespace maplert {
class MemMap {
 public:
  static const bool kEnableRange = true;
  static const bool kEnableRandomMemStart = true;
  static const int kDefaultMemFlags = MAP_PRIVATE | MAP_ANONYMOUS;
  static const int kDefaultMemProt = PROT_READ | PROT_WRITE;

  struct Option {          // optional args for mem map
    const char *tag;       // name to identify the mapped memory
    void *reqBase;         // a hint to mmap about start addr, not guaranteed
    int flags;             // mmap flags
    int prot;              // initial access flags
    bool protAll;          // applying prot to all pages in range
    bool reqRange;         // request mapping within the following range (guaranteed)
    address_t lowestAddr;  // lowest start addr (only used when reqRange == true)
    address_t highestAddr; // highest end addr (only used when reqRange == true)
    bool isRandom;         // randomise start addr (only used when reqRange == true)
  };
  // by default, it tries to map memory in low addr space, with a random start
  static constexpr Option kDefaultOptions = {
      "maple_unnamed", nullptr, kDefaultMemFlags, kDefaultMemProt, false,
      true, HEAP_START, HEAP_END, true
  };

  // the only way to get a MemMap
  static MemMap *MapMemory(size_t reqSize, size_t initSize, const Option &opt = kDefaultOptions);
  static MemMap *CreateMemMapAtExactAddress(void *addr, size_t size, const Option &opt = kDefaultOptions);

  // destroy a MemMap
  static void DestroyMemMap(MemMap *&memMap) noexcept {
    if (memMap != nullptr) {
      delete memMap;
      memMap = nullptr;
    }
  }

  void *GetBaseAddr() const {
    return memBaseAddr;
  }
  void *GetCurrEnd() const {
    return memCurrEndAddr;
  }
  void *GetMappedEndAddr() const {
    return memMappedEndAddr;
  }
  size_t GetCurrSize() const {
    return memCurrSize;
  }
  size_t GetMappedSize() const {
    return memMappedSize;
  }

  inline bool IsAddrInCurrentRange(address_t addr) const {
    return reinterpret_cast<size_t>(addr - reinterpret_cast<address_t>(memBaseAddr)) < memCurrSize;
  }

  // change the access flags of the memory in given range
  bool ProtectMem(address_t addr, size_t size, int prot) const;

  // grow the size of the usable memory
  bool Extend(size_t reqSize);

  // madvise(DONTNEED) memory in the range [releaseBeginAddr, releaseBeginAddr + size)
  bool ReleaseMem(address_t releaseBeginAddr, size_t size) const;

  // resize the memory map by releasing the pages at the end (munmap)
  bool Shrink(size_t newSize);

  ~MemMap();
  MemMap(const MemMap &that) = delete;
  MemMap(MemMap &&that) = delete;
  MemMap &operator=(const MemMap &that) = delete;
  MemMap &operator=(MemMap &&that) = delete;

 private:
  // Map in the specified range.
  // The return addr is guaranteed to be in [lowestAddr, highestAddr - reqSize) if successful.
  static void *MemMapInRange(address_t lowestAddr, address_t highestAddr,
                             size_t reqSize, int prot, int flags,
                             int fd = -1, int offset = 0);
  static void *MemMapInRangeInternal(address_t hint,
                                     address_t lowestAddr, address_t highestAddr,
                                     size_t reqSize, int prot, int flags,
                                     int fd, int offset);
  static bool ProtectMemInternal(void *addr, size_t size, int prot);

  void *memBaseAddr;      // start of the mapped memory
  void *memCurrEndAddr;   // end of the memory **in use**
  void *memMappedEndAddr; // end of the mapped memory, always >= currEndAddr
  size_t memCurrSize;     // size of the memory **in use**
  size_t memMappedSize;   // size of the mapped memory, always >= currSize
  bool protOnce;
  const int memProt;      // memory accessibility flags

  // MemMap is created via factory method
  MemMap(void *baseAddr, size_t initSize, size_t mappedSize, bool protAll, int prot);

  void UpdateCurrEndAddr();
}; // class MemMap
} // namespace maplert
#endif // MAPLE_RUNTIME_ALLOC_MEM_MAP_H
