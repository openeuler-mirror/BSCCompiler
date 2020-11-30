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
#include "allocator/bp_allocator.h"
#include "allocator/bp_allocator_inlined.h"
#include "collector/collector_tracing.h"

namespace maplert {
// vaddr starts from (kSpaceAnchor - kMaxSpaceSize)
const uint32_t BumpPointerAlloc::kInitialSpaceSize = (1u << 18); // 256KB initially
const size_t BumpPointerAlloc::kExtendedSize = (1u << 16); // extend 64KB each time

// The reason with this mem map is that we use multiple bp allocators, each
// having a mem map. If we let them map random addresses one by one, there
// is a good chance the attempted mmap() clobbers one another, and fails
// constantly, thus impeding performance.
// The solution is to reserve enough space for all of them, then each of
// them uses another mmap (with flag MAP_FIXED) to place their mappings
// in the reserved space in order.
// See the usage of flag MAP_FIXED:
//   http://man7.org/linux/man-pages/man2/mmap.2.html
// Alternatively, we can make changes to MemMap so it coordinates
// multiple random mappings better, but by intuition that's more expensive.
static MemMap *reservedAddrSpace = nullptr;
static address_t reservedAddr = 0;
static address_t reservedEnd = 0;

static inline void ReserveAddrSpace() {
  if (reservedAddr != 0) {
    return;
  }
  MemMap::Option opt = MemMap::kDefaultOptions;
  opt.reqRange = true;
  opt.lowestAddr = PERM_BEGIN;
  opt.highestAddr = PERM_END - BumpPointerAlloc::kFireBreak;
  opt.tag = "maple_alloc_reserved";
  // this must succeed otherwise it won't return
  reservedAddrSpace = MemMap::MapMemory(kOffHeapSpaceSize, 0, opt);
  reservedAddr = reinterpret_cast<address_t>(reservedAddrSpace->GetBaseAddr());
  reservedEnd = reinterpret_cast<address_t>(reservedAddrSpace->GetMappedEndAddr());
}

BumpPointerAlloc::BumpPointerAlloc(const string &name, size_t maxSize)
    : memMap(nullptr), startAddr(0), currentAddr(0), endAddr(0), showmapName(name) {
  // this should be called at a separate time!
  Init(maxSize);
}

BumpPointerAlloc::~BumpPointerAlloc() {
  LOG2FILE(kLogTypeAllocator) << "Destructing BumpPointerAlloc" << std::endl;
  MemMap::DestroyMemMap(memMap);
}

void BumpPointerAlloc::Init(size_t growthLimit) {
  ReserveAddrSpace();
  if (reservedAddr == 0 || reservedAddr > reservedEnd - growthLimit) {
    LOG(FATAL) << "space size inconsistency: reserved from " << std::hex << reservedAddr <<
                  " to " << reservedEnd << ", size requested " << growthLimit;
  }
  MemMap::Option opt = MemMap::kDefaultOptions;
  opt.reqRange = false;
  opt.tag = showmapName.c_str();
  opt.reqBase = reinterpret_cast<void*>(reservedAddr);
  opt.flags |= MAP_FIXED;
  reservedAddr += growthLimit + kOffHeapSpaceGap;
  if (reservedAddr == reservedEnd + kOffHeapSpaceGap) {
    MemMap::DestroyMemMap(reservedAddrSpace);
  }
  memMap = MemMap::MapMemory(growthLimit, kInitialSpaceSize, opt);
  startAddr = reinterpret_cast<address_t>(memMap->GetBaseAddr());
  currentAddr = startAddr;
  endAddr = reinterpret_cast<address_t>(memMap->GetCurrEnd());
}

void BumpPointerAlloc::Dump(std::basic_ostream<char> &os) {
  if (memMap == nullptr) {
    os << "Fail to dump permanent space because memMap is null!" << "\n";
    return;
  }
  // invoked in stw
  size_t usedSize = currentAddr - startAddr;
  os << showmapName << " space used: " << usedSize << ", left: " << ((memMap->GetMappedSize()) - usedSize) << "\n";

#if BPALLOC_DEBUG
  DumpUsage(os);
#endif
}

address_t BumpPointerAlloc::Alloc(size_t size) {
  size_t allocSize = AllocUtilRndUp<address_t>(size, kBPAllocObjAlignment);
  address_t resultAddr = AllocInternal(allocSize);
  return resultAddr;
}

bool BumpPointerAlloc::Contains(address_t obj) const {
  if ((obj < currentAddr) && (obj >= startAddr)) {
    return true;
  } else {
    return false;
  }
}

DecoupleAllocator::DecoupleAllocator(const string &name, size_t maxSize) : BumpPointerAlloc(name, maxSize) {}

address_t DecoupleAllocator::Alloc(size_t size, DecoupleTag tag) {
  address_t resultAddr = 0U;
  if (UNLIKELY(size >= DecoupleAllocHeader::kMaxSize)) {
    return resultAddr;
  }
  size_t allocSize = GetAllocSize(size);
  resultAddr = AllocInternal(allocSize);
  address_t allocAddr = BPALLOC_GET_OBJ_FROM_ADDR(resultAddr);
  DecoupleAllocHeader::SetHeader(allocAddr, static_cast<int>(tag), size);
  return allocAddr;
}

bool DecoupleAllocator::ForEachObj(function<void(address_t)> visitor) {
  address_t objAddr = startAddr + DecoupleAllocHeader::kHeaderSize;
  while (objAddr < currentAddr) {
    size_t size = DecoupleAllocHeader::GetSize(objAddr);
    if (size >= DecoupleAllocHeader::kMaxSize) {
      // size broken! heap corruption detected.
      return false;
    }
    visitor(objAddr);
    size_t allocSize = GetAllocSize(size);
    if (currentAddr - objAddr <= allocSize) {
      break;
    }
    objAddr += allocSize;
  }
  return true;
}

constexpr int kDecoupleTagNamesAlign = 20;

void DecoupleAllocator::DumpUsage(std::basic_ostream<char> &os) {
  std::vector<std::pair<uint64_t, size_t>> profile;
  for (int i = 0; i < static_cast<int>(DecoupleTag::kTagMax); ++i) {
    profile.push_back(std::make_pair(0, 0));
  }
  address_t objAddr = startAddr + DecoupleAllocHeader::kHeaderSize;
  while (objAddr < currentAddr) {
    int tag = DecoupleAllocHeader::GetTag(objAddr);
    if (tag < static_cast<int>(DecoupleTag::kTagMax)) {
      profile[tag].first++;
    } else {
      os << "tag broken at " << objAddr << ", tag " << tag << "\n";
      break;
    }
    size_t size = DecoupleAllocHeader::GetSize(objAddr);
    if (size < DecoupleAllocHeader::kMaxSize) {
      profile[tag].second += size;
    } else {
      os << "size broken at " << objAddr << ", tag " << tag << ", size " << size << "\n";
      break;
    }
    size_t allocSize = GetAllocSize(size);
    if (currentAddr - objAddr <= allocSize) {
      break;
    }
    objAddr += allocSize;
  }
  os << showmapName << " space usage:\n";
  for (uint16_t i = 0; i < DecoupleTag::kTagMax; ++i) {
    os << " " << std::setw(kDecoupleTagNamesAlign) << kDecoupleTagNames[i] << ": count " << profile[i].first <<
        ", total size " << profile[i].second << "\n";
  }
}

address_t MetaAllocator::Alloc(size_t size, MetaTag metaTag) {
  size_t allocSize = AllocUtilRndUp<address_t>(size, kBPAllocObjAlignment);
  address_t resultAddr = AllocInternal(allocSize);
  sizeUsed[metaTag] += allocSize;
  return resultAddr;
}

void MetaAllocator::DumpUsage(std::basic_ostream<char> &os) {
  os << "Class meta: " << sizeUsed[kClassMetaData] << "\n";
  os << "Field meta: " << sizeUsed[kFieldMetaData] << "\n";
  os << "Method meta: " << sizeUsed[kMethodMetaData] << "\n";
  os << "VTable meta: " << sizeUsed[kITabMetaData] << "\n";
  os << "Native string: " << sizeUsed[kNativeStringData] << "\n";
}

void ZterpStaticRootAllocator::VisitStaticRoots(const RefVisitor &visitor) {
  std::lock_guard<std::mutex> lock(globalLock);
  size_t spaceAddr = startAddr;
  while (spaceAddr < currentAddr) {
    address_t *rootAddr = *(reinterpret_cast<address_t**>(spaceAddr));
    LinkerRef ref(rootAddr);
    if (!ref.IsIndex()) {
      visitor(*rootAddr);
    }
    spaceAddr += singleObjSize;
  }
}
} // namespace maplert
