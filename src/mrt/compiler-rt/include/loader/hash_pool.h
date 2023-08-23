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
#ifndef __MAPLE_LOADER_HASH_POOL__
#define __MAPLE_LOADER_HASH_POOL__

#include <vector>

#include "gc_roots.h"
#include "mrt_object.h"
#include "mrt_reflection_class.h"
#include "allocator/page_allocator.h"
#include "mclass_inline.h"
#include <bitset>
namespace maplert {
#ifndef __ANDROID__
using ClassPtr = uintptr_t;
#else
using ClassPtr = uint32_t;
#endif

#if defined(__arm__)
using BucketType = uint32_t;
#else
using BucketType = uint64_t;
#endif
struct ConflictClass {
  std::vector<ClassPtr> conflictData;
};
template<class Key>
using HashPoolVector = std::vector<Key, StdContainerAllocator<Key, kClassLoaderAllocator>>;
using NameCompare = std::function<bool(const ClassPtr data, const std::string &name)>;
enum MClassFlag {
  kIsConflict = 1ul, // low 1 bit as conflict flag
  kIsDexClassOffset = 2ul, // low 2 bit as dexClassDef flag
};

class MClassHashPool {
 public:
  MClassHashPool();
  ~MClassHashPool();

  void Create(uint32_t bucketCount);
  void Destroy();
  size_t CalcMemoryCost() const;
  double GetHashConflictRate() const;
  double GetClassConflictRate() const;
  void InitClass(uint32_t hashIndex);
  void Collect();
  void Set(uint32_t hashIndex, ClassPtr classInfo);
  void VisitClasses(const maple::rootObjectFunc &func);
  MClass *Get(const std::string &name) const;
  ClassPtr Get(const std::string &name, NameCompare cmp) const;
  inline void Set(const std::string &name, ClassPtr classInfo) {
    uint32_t hashIndex = GetHashIndex32(name);
    Set(hashIndex, classInfo);
  }
  inline void Set(uint32_t hashIndex, const MClass &klass) {
    Set(hashIndex, static_cast<ClassPtr>(reinterpret_cast<uintptr_t>(&klass)));
  }
  inline uint16_t GetHashIndex16(const std::string &name) const {
    // 211 is a proper prime, which can reduce the conflict rate.
    return static_cast<uint16_t>(0xFFFF & BKDRHash(name, 211));
  }
  inline uint32_t GetHashIndex32(const std::string &name) const {
    // 211 is a proper prime, which can reduce the conflict rate.
    return BKDRHash(name, 211);
  }
 protected:
  inline uint32_t BitCount(uint32_t n) const;
  inline uint32_t BitCount(uint64_t n) const;
  size_t FindNextPrime(size_t nr) const;
  inline uint32_t NextPrime(size_t n) const;
  inline uint32_t BKDRHash(const std::string &key, uint32_t seed) const {
    size_t len = key.length();
    size_t idx = 0;
    uint32_t hash = 0;
    while (idx < len) {
      hash = hash * seed + static_cast<uint32_t>(key[idx++]);
    }
    return hash;
  }

 private:
  static constexpr uint32_t kBucketBitNum = sizeof(BucketType) << 3; // calculate the number of bits
  static constexpr uint32_t kMaxExtended = 20;
  static constexpr ClassPtr kMClassAddressMask = 0x3;
  uint32_t mBucketCount = 0;
  uint32_t mClassCount = 0;
  uint32_t mConflictBucketCount = 0;
  uint32_t mFillBucketCount = 0;
  HashPoolVector<uint32_t> bitCounts;
  HashPoolVector<BucketType> bitBuckets;
  HashPoolVector<ClassPtr> bucketData; // 2-x : Ptr to Data, 1: DexClassOffset flag, 0: conflict Flag
};
} // namespace maplert
#endif // __MAPLE_LOADER_HASH_POOL__
