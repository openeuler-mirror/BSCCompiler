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
#ifndef MAPLE_RUNTIME_BP_ALLOCATOR_H
#define MAPLE_RUNTIME_BP_ALLOCATOR_H

#include <cstdint>
#include <cstdlib>
#include <cinttypes>
#include <mutex>

#include "alloc_config.h"
#include "allocator/mem_map.h"

namespace maplert {
// A bump-pointer allocator for permanent space.
class BumpPointerAlloc {
 public:
  static const uint32_t kInitialSpaceSize;
  static const size_t kExtendedSize;

  // kClassInitializedState is defined according to kFireBreak and kSpaceAnchor,
  // so adjust kClassInitializedState if they are modified.
  //
  // kFireBreak also reserves some time for C2IBridge. We need to make sure the
  // C2I bridge does not exceed this amount.
  //
  // In the future, it is better to arrange the entire memory layout in a single
  // file in order to coordinate multiple modules that need to use the low 4GB
  // memory.
  static constexpr size_t kFireBreak = (256u << 20); // 256MB. Leave enough space for C2I Bridge.
  static constexpr address_t kSpaceAnchor = (3ul << 30); // 3GB

  BumpPointerAlloc(const string&, size_t);
  virtual ~BumpPointerAlloc();
  virtual address_t Alloc(size_t size);
  inline address_t AllocThrowExp(size_t size);
  bool Contains(address_t obj) const;
  void Dump(std::basic_ostream<char> &os);

 protected:
  MemMap *memMap;
  address_t startAddr;
  address_t currentAddr;
  address_t endAddr;
  mutex globalLock;
  const string showmapName;

  template <bool throwExp = false>
  inline address_t AllocInternal(const size_t &allocSize);
  virtual void DumpUsage(std::basic_ostream<char>&) {}

 private:
  void Init(size_t growthLimit);
}; // class BumpPointerAlloc

class MetaAllocator : public BumpPointerAlloc {
 public:
  MetaAllocator(const string &name, size_t maxSpaceSize) : BumpPointerAlloc(name, maxSpaceSize) {}
  ~MetaAllocator() = default;
  address_t Alloc(size_t size, MetaTag metaTag);
  address_t Alloc(size_t) override {
    BPALLOC_ASSERT(false, "must provide MetaTag in meta space");
    return 0U;
  }

 protected:
  void DumpUsage(std::basic_ostream<char> &os) override;

 private:
  uint32_t sizeUsed[kMetaTagNum] = { 0 };
};

class DecoupleAllocator : public BumpPointerAlloc {
 public:
  DecoupleAllocator(const string &name, size_t maxSpaceSize);
  ~DecoupleAllocator() = default;

  bool ForEachObj(function<void(address_t)> visitor);
  address_t Alloc(size_t size, DecoupleTag tag);
  address_t Alloc(size_t) override {
    BPALLOC_ASSERT(false, "must provide DecoupleTag in decouple space");
    return 0U;
  }

  static inline size_t GetAllocSize(size_t size) {
    return AllocUtilRndUp<address_t>(size + DecoupleAllocHeader::kHeaderSize, kBPAllocObjAlignment);
  }

 protected:
  void DumpUsage(std::basic_ostream<char> &os) override;
};

class ZterpStaticRootAllocator : public BumpPointerAlloc {
 public:
  static const size_t singleObjSize = kBPAllocObjAlignment;
  ZterpStaticRootAllocator(const string &name, size_t maxSpaceSize) : BumpPointerAlloc(name, maxSpaceSize) {};
  virtual ~ZterpStaticRootAllocator() = default;
  inline address_t GetStartAddr() const {
    return startAddr;
  }
  inline size_t GetObjNum() {
    // because the space of bump pointer allocator only increase linearly, and the obj won't be erased, so the race
    // only occurs when currentAddr is modified.
    std::lock_guard<std::mutex> lock(globalLock);
    __MRT_ASSERT(currentAddr >= startAddr, "currentAddr should not be lower than startAddr!");
    size_t usedSpace = currentAddr - startAddr;
    __MRT_ASSERT((usedSpace % singleObjSize) == 0, "used space should be devided by single size of root pointer!");
    return usedSpace / singleObjSize;
  }
  void VisitStaticRoots(const RefVisitor &visitor);
};
} // namespace maplert

#endif // MAPLE_RUNTIME_BPALLOCATOR_H
