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
#include "allocator/page_map.h"
#include "allocator/alloc_config.h"

// Definitions of page map
namespace maplert {
PageMap::PageMap()
    : spaceBeginAddr(0),
      spaceEndAddr(0),
      maxMapSize(0),
      pageMapSize(0),
      memMap(nullptr),
      map(nullptr) {
}

PageMap::~PageMap() {
  ROSIMPL_VERIFY_DUMP_PG_TABLE;
  spaceBeginAddr = 0;
  spaceEndAddr = 0;
  MemMap::DestroyMemMap(memMap);
  // memory will be released by memMap
  map = nullptr;
}

void PageMap::Init(address_t baseAddr, size_t maxSize, size_t size) {
  spaceBeginAddr = baseAddr;
  spaceEndAddr = baseAddr + size;
  maxMapSize = ALLOCUTIL_PAGE_BYTE2CNT(maxSize);
  pageMapSize = ALLOCUTIL_PAGE_BYTE2CNT(size);

  size_t reqSize = ALLOCUTIL_PAGE_RND_UP(maxMapSize);
  MemMap::Option opt = MemMap::kDefaultOptions;
  opt.tag = "maple_alloc_ros_pm";
  opt.reqBase = nullptr;
  opt.reqRange = false;
  memMap = MemMap::MapMemory(reqSize, reqSize, opt);
  map = static_cast<PageLabel*>(memMap->GetBaseAddr());
  if (UNLIKELY(map == nullptr)) {
    LOG(FATAL) << "page map initialisation failed";
  }
  SetRange(0, maxMapSize, kPReleased);

  // As for now, the assignment to maxMapSize in this function is the only
  // assignment.  This means the heap memory mapped to the address space never
  // exceeds this size.  We initialize finProf to create a table of this size,
  // so that the table is big enough even when the heap size is changed.
  finProf.Init(maxMapSize);

  LOG2FILE(kLogTypeAllocator) << "[PageMap] number of entries " << pageMapSize << ". address " << map << std::endl;
}

constexpr uint32_t kMaxNumPerLine = 10;

void PageMap::Dump() {
  LOG2FILE(kLogTypeAllocator) << "[PageMap] number of entries: " << pageMapSize << "\n";
  LOG2FILE(kLogTypeAllocator) << "          max number of entries: " << maxMapSize << "\n";
  LOG2FILE(kLogTypeAllocator) << "          spaceBeginAddr = 0x" << std::hex << spaceBeginAddr << "\n";
  LOG2FILE(kLogTypeAllocator) << "          spaceEndAddr = 0x" << std::hex << spaceEndAddr << "\n";
  std::stringstream ss;
  bool skipFreePages = true;
  if (map != nullptr) {
    for (IntType i = 0 ; i < GetMapSize(); ++i) {
      if (skipFreePages && static_cast<int>(map[i]) <= static_cast<int>(kPFree)) {
        continue;
      }
      if (i % kMaxNumPerLine == 0) {
        ss << "\n          [" << i << "]:" << static_cast<int>(map[i]);
        continue;
      }
      ss << "  [" << i << "]:" << static_cast<int>(map[i]);
    }
  }
  LOG2FILE(kLogTypeAllocator) << ss.str().c_str() << std::endl;
}

void PageMap::DumpFinalizableInfo(std::ostream &ost) {
  ost << "Dumping finalizable information for each page" << std::endl;

  IntType touched = 0;
  IntType hasFins = 0;

  for (IntType index = 0; index < maxMapSize; ++index) {
    FinalizableProf::PageEntry &entry = finProf.GetEntry(index);
#if MRT_RESURRECTION_PROFILE == 1
    size_t incs = entry.incs.load(std::memory_order_relaxed);
    size_t decs = entry.decs.load(std::memory_order_relaxed);
#endif // MRT_RESURRECTION_PROFILE
    uint16_t fins = entry.fins.load(std::memory_order_relaxed);
    if (
#if MRT_RESURRECTION_PROFILE == 1
      incs == 0 && decs == 0 &&
#endif // MRT_RESURRECTION_PROFILE
      fins == 0
    ) {
      continue;
    }

    ++touched;

    if (fins != 0) {
      ++hasFins;
    }

    ost << "  index: " << index <<
#if MRT_RESURRECTION_PROFILE == 1
        "  incs:" << incs <<
        "  decs:" << decs <<
#endif // MRT_RESURRECTION_PROFILE
        "  fins:" << fins <<
        std::endl;
  }

  ost << "maxMapSize: " << maxMapSize << std::endl;
  ost << "touched: " << touched << std::endl;
  ost << "hasFins: " << hasFins << std::endl;
  ost << "pageMapSize: " << pageMapSize << std::endl;
}
} // namespace maplert
