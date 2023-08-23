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
#include "loader/hash_pool.h"
#include "cinterface.h"
#include "utils/time_utils.h"

using namespace std;
namespace maplert {
// special case 0, handle next_prime(i) for i in [1, 210)
const unsigned kSmallPrimes[] = {
    0, 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107,
    109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211
};

// potential primes = 210*k + kIndices[i], k >= 1 these numbers are not divisible
//     by 2, 3, 5 or 7 (or any integer 2 <= j <= 10 for that matter).
const unsigned kIndices[] = {
    1, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113,
    121, 127, 131, 137, 139, 143, 149, 151, 157, 163, 167, 169, 173, 179, 181, 187, 191, 193, 197, 199, 209
};

const unsigned kFactor[] = {
    10, 2, 4, 2, 4, 6, 2, 6, 4, 2, 4, 6, 6, 2, 6, 4, 2, 6, 4, 6, 8, 4, 2, 4,
    2, 4, 8, 6, 4, 6, 2, 4, 6, 2, 6, 6, 4, 2, 4, 6, 2, 6, 4, 2, 4, 2, 10
};
const size_t kMaxIndices = 210;
const size_t kSmallPrimesNum = sizeof(kSmallPrimes) / sizeof(kSmallPrimes[0]);
const size_t kFactNum = sizeof(kFactor) / sizeof(kFactor[0]);
const size_t kIndicesNum = sizeof(kIndices) / sizeof(kIndices[0]);
size_t MClassHashPool::FindNextPrime(size_t nr) const {
  // Else nr > largest kSmallPrimes
  bool flag = false;
  // Start searching list of potential primes: L * k0 + kIndices[in]
  // Select first potential prime >= nr
  //   Known a-priori nr >= L
  size_t k0 = nr / kMaxIndices;
  size_t in = static_cast<size_t>(std::lower_bound(kIndices, kIndices + kIndicesNum, nr - k0 * kMaxIndices) - kIndices);
  nr = kMaxIndices * k0 + kIndices[in];
  for (;;) {
    // It is known a-priori that nr is not divisible by 2, 3, 5 or 7, so don't test those (j == 5 -> divide by 11 first)
    //   And the potential primes start with 211, so don't test against the last small prime.
    // Divide nr by all primes or potential primes(i) until:
    //   1.The division is even, so try next potential prime.  2.The i > sqrt(nr), in which case nr is prime.
    size_t ix = 211;
    for (size_t j = 5; j < kSmallPrimesNum - 1; ++j) {
      const std::size_t qr = nr / kSmallPrimes[j];
      if (qr < kSmallPrimes[j]) {
        return nr;
      }
      if (nr == qr * kSmallPrimes[j]) {
        goto NEXT;
      }
    }
    // nr cann't divisible by small primes, try potential primes
    for (;;) {
      std::size_t qr = nr / ix;
      if (qr < ix) {
        return nr;
      }
      if (nr == qr * ix) {
        break;
      }
      for (size_t factorIndex = 0; factorIndex < kFactNum; ++factorIndex) {
        ix += kFactor[factorIndex];
        qr = nr / ix;
        if (qr < ix) {
          return nr;
        }
        if (nr == qr * ix) {
          flag = true;
          break;
        }
      }
      if (flag) {
        flag = false;
        break;
      }
      ix += 2; // This will loop i to the next "plane" of potential primes, 2:Even numbers can not be primes.
    }
NEXT:
    // nr is not prime.  Increment nr to next potential prime.
    if (++in == kIndicesNum) {
      ++k0;
      in = 0;
    }
    nr = kMaxIndices * k0 + kIndices[in];
  }
}

inline uint32_t MClassHashPool::NextPrime(size_t n) const {
  size_t nr = n * kMaxExtended; // to increase the number of nbuckets in advance, which can reduce the conflict rate.
  // If nr is small enough, search in kSmallPrimes
  if (nr <= kSmallPrimes[kSmallPrimesNum - 1]) {
    return *std::lower_bound(kSmallPrimes, kSmallPrimes + kSmallPrimesNum, nr);
  }
  return static_cast<uint32_t>(FindNextPrime(nr));
}

MClassHashPool::MClassHashPool() : mBucketCount(0), mClassCount(0), mConflictBucketCount(0), mFillBucketCount(0) {}

MClassHashPool::~MClassHashPool() {
  bucketData.clear();
  bitBuckets.clear();
  bitCounts.clear();
}

void MClassHashPool::Create(uint32_t classCount) {
  mClassCount = classCount;
  if (classCount != 0) {
    mBucketCount = NextPrime(classCount);
  }

  CL_VLOG(classloader) << "Create(), extended bucket count: " << classCount << "-->" << mBucketCount << maple::endl;
  bitBuckets.resize(mBucketCount / kBucketBitNum + 1, 0);
  bitCounts.resize(mBucketCount / kBucketBitNum + 1, 0);
}

void MClassHashPool::Destroy() {
  bucketData.clear();
  bitBuckets.clear();
  bitCounts.clear();
}

inline uint32_t MClassHashPool::BitCount(uint32_t n) const {
  return __builtin_popcountl(n);
}

inline uint32_t MClassHashPool::BitCount(uint64_t n) const {
  return __builtin_popcountll(n);
}

void MClassHashPool::InitClass(uint32_t hashIndex) {
  uint32_t bucketPos = hashIndex % mBucketCount;
  uint32_t index = bucketPos / kBucketBitNum;
  uint32_t offset = bucketPos % kBucketBitNum;
  BucketType bucketVal = bitBuckets[index];
  BucketType targetBit = 1ul << (kBucketBitNum - offset - 1);
  if ((bucketVal & targetBit) == 0) { // no conflict
    bitBuckets[index] = bucketVal | targetBit;
    ++mFillBucketCount;
  } else {
    ++mConflictBucketCount;
  }
}
void MClassHashPool::Collect() {
  bucketData.resize(mFillBucketCount, 0);
  uint32_t total = 0;
  for (uint32_t i = 0; i < bitCounts.size(); ++i) {
    bitCounts[i] = total;
    total += BitCount(bitBuckets[i]);
  }
}

void MClassHashPool::Set(uint32_t hashIndex, ClassPtr targetKlass) {
  uint32_t bucketPos = hashIndex % mBucketCount;
  uint32_t index = bucketPos / kBucketBitNum;
  uint32_t offset = bucketPos % kBucketBitNum;
  BucketType targetBit = 1ul << (kBucketBitNum - offset - 1);
  if ((bitBuckets[index] & targetBit) == 0) { // Not in pool
    CL_LOG(FATAL) << "Fatal Error, Set HashPool Empty Bucket, this=" << this << ", hashIndex=" << hashIndex <<
        ", mBucketCount=" << mBucketCount << ", bitBuckets[index]" << bitBuckets[index] << maple::endl;
    return;
  }
  BucketType shift = (offset == 0) ? 0 : bitBuckets[index] >> (kBucketBitNum - offset);
  uint32_t dataPos = bitCounts[index] + BitCount(shift);
  if (dataPos >= mFillBucketCount) {
    CL_LOG(FATAL) << "Fatal Error, Set HashPool Index overflow, this=" << this << ", index=" << index << ", offset=" <<
        offset << ", dataPos=" << dataPos << ", mFillBucketCount=" << mFillBucketCount << ", bitCounts[index]=" <<
        bitCounts[index] << ", BitCount(shift)=" << BitCount(shift) << ", bitBuckets[index]=" << bitBuckets[index] <<
        ", mBucketCount=" << mBucketCount << ", bitBuckets.size()=" << bitBuckets.size() << ", hashIndex=" <<
        hashIndex << maple::endl;
    return;
  }
  ClassPtr data = bucketData[dataPos];
  if (data == 0) { // no conflict
    bucketData[dataPos] = targetKlass;
  } else { // conflict
    if ((data & kIsConflict) == 0) { // first conflict
      ConflictClass *obj = reinterpret_cast<ConflictClass*>(maplert::MRT_AllocFromMeta(sizeof(ConflictClass),
                                                                                       kClassMetaData));
      obj->conflictData.push_back(data);
      obj->conflictData.push_back(targetKlass);
      data = static_cast<ClassPtr>(reinterpret_cast<uintptr_t>(obj));
      data |= kIsConflict;
      bucketData[dataPos] = data;
    } else { // no first conflict
      ConflictClass *obj = reinterpret_cast<ConflictClass*>(data & ~kMClassAddressMask);
      obj->conflictData.push_back(targetKlass);
    }
  }
}

MClass *MClassHashPool::Get(const std::string &name) const {
  auto compare = [&](const ClassPtr data, const std::string &name)->bool {
    if ((data & kIsDexClassOffset) != 0) {
      return false;
    }
    MClass *klass = reinterpret_cast<MClass*>(data & ~kMClassAddressMask);
    if (name == klass->GetName()) {
      return true;
    }
    return false;
  };
  return reinterpret_cast<MClass*>(Get(name, compare));
}

ClassPtr MClassHashPool::Get(const std::string &name, const NameCompare compare) const {
  if (mClassCount == 0) {
    return 0;
  }
  uint32_t hashIndex = GetHashIndex32(name);
  uint32_t bucketPos = hashIndex % mBucketCount;
  uint32_t index = bucketPos / kBucketBitNum;
  uint32_t offset = bucketPos % kBucketBitNum;
  BucketType targetBit = 1ul << (kBucketBitNum - offset - 1);
  if ((bitBuckets[index] & targetBit) == 0) { // Not in pool
    return 0;
  }
  BucketType shift = (offset == 0) ? 0 : bitBuckets[index] >> (kBucketBitNum - offset);
  uint32_t dataPos = bitCounts[index] + BitCount(shift);
  if (dataPos >= mFillBucketCount) {
    CL_LOG(FATAL) << "Fatal Error, Get HashPool Index overflow, this=" << this << ", index=" << index << ", offset=" <<
        offset << ", dataPos=" << dataPos << ", mFillBucketCount=" << mFillBucketCount << ", bitCounts[index]=" <<
        bitCounts[index] << ", BitCount(shift)=" << BitCount(shift) << ", bitBuckets[index]=" << bitBuckets[index] <<
        ", mBucketCount=" << mBucketCount << ", bitBuckets.size()=" << bitBuckets.size() << ", hashIndex=" <<
        hashIndex << maple::endl;
    return 0;
  }
  ClassPtr data = bucketData[dataPos];
  if (data == 0) {
    return 0;
  }
  if ((data & kIsConflict) == 0) { // no conflict
    if (compare(data, name)) {
      return data;
    }
  } else { // conflict
    ConflictClass *obj = reinterpret_cast<ConflictClass*>(data & ~kMClassAddressMask);
    for (ClassPtr conflict : obj->conflictData) {
      if (compare(conflict, name)) {
        return conflict;
      }
    }
  }
  return 0;
}

double MClassHashPool::GetHashConflictRate() const {
  return static_cast<double>(mConflictBucketCount) / static_cast<double>(mBucketCount);
}

double MClassHashPool::GetClassConflictRate() const {
  return static_cast<double>(mClassCount - mFillBucketCount) / static_cast<double>(mClassCount);
}

size_t MClassHashPool::CalcMemoryCost() const {
  size_t memory = 0;
  memory += sizeof(mBucketCount) + sizeof(mClassCount) + sizeof(mFillBucketCount) + sizeof(mConflictBucketCount) + 1;
  memory += bitBuckets.size() * sizeof(BucketType);
  memory += bitCounts.size() * sizeof(uint16_t);
  memory += bucketData.size() * sizeof(ClassPtr);
  for (ClassPtr ptr : bucketData) {
    if ((ptr & kIsConflict) != 0) { // conflict calc
      ConflictClass *obj = reinterpret_cast<ConflictClass*>(ptr & ~kIsConflict);
      memory += sizeof(ConflictClass) + obj->conflictData.size() * sizeof(ClassPtr);
    }
  }
  return memory;
}

void MClassHashPool::VisitClasses(const maple::rootObjectFunc &func) {
  for (ClassPtr ptr : bucketData) {
    if ((ptr & kIsConflict) != 0) { // conflict calc
      ConflictClass *obj = reinterpret_cast<ConflictClass*>(ptr & ~kIsConflict);
      for (ClassPtr conflict : obj->conflictData) {
        MClass *classInfo = reinterpret_cast<MClass*>(conflict);
        func(reinterpret_cast<maple::address_t>(classInfo));
      }
    } else { // not conflict
      MClass *classInfo = reinterpret_cast<MClass*>(ptr);
      func(reinterpret_cast<maple::address_t>(classInfo));
    }
  }
}
} // namespace maplert
