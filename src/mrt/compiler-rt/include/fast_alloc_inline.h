/*
 * Copyright (c) [2020] Huawei Technologies Co., Ltd. All rights reserved.
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
#ifndef MAPLE_RUNTIME_FAST_ALLOC_INLINE_H
#define MAPLE_RUNTIME_FAST_ALLOC_INLINE_H

#include "chosen.h"
#include "sizes.h"

namespace maplert {
#if ALLOC_USE_FAST_PATH
// common fast allocation routine, it assumes the size is aligned, and
// it shouldn't be too big (bigger than large obj size)
__attribute__ ((always_inline))
static address_t MRT_TryNew(size_t alignedObjSize, address_t klass) {
  address_t objAddr = (*theAllocator).FastNewObj(alignedObjSize);
  if (LIKELY(objAddr != 0)) {
    StoreRefField(objAddr, kMrtKlassOffset, klass);
    // in rc mode, we need to set cycle bits and rc bits
    // this produces very long code, need to optimise
    GCTibGCInfo *gcInfo = reinterpret_cast<GCTibGCInfo*>(reinterpret_cast<MClass*>(klass)->GetGctib());
    CHECK(gcInfo != nullptr) << "MRT_TryNew : gcInfo is nullptr";
    GCHeaderLVal(objAddr) = (kAllocatedBit | gcInfo->headerProto);
    RefCountLVal(objAddr) = 1 + kWeakRCOneBit;
    if (UNLIKELY(FastAllocData::data.isConcurrentMarking)) {
      FastAllocData::data.bm->MarkObject(objAddr);
    }
  }
  return objAddr;
}

// try new object: it takes fast path if possible, otherwise returns 0
// see slow path code for more info
__attribute__ ((always_inline, used))
static address_t MRT_TryNewObject(address_t klass) {
  if (UNLIKELY(!modifier::IsFastAllocClass(reinterpret_cast<ClassMetadata*>(klass)->flag))) {
    return 0;
  }
  MClass *cls = MObject::Cast<MClass>(klass);
  size_t objSize = cls->GetObjectSize(); // must have been aligned
  address_t objAddr = MRT_TryNew(objSize, klass);
  if (UNLIKELY(objAddr == 0)) {
    return 0;
  }

  return objAddr;
}

// try new object: it takes fast path if possible, otherwise returns 0
// it also accepts an explicit size instead of taking the size from the klass
// useful for allocation of strings (where the klass's size is not accurate)
// see slow path code for more info
__attribute__ ((always_inline, used))
static address_t MRT_TryNewObject(address_t klass, size_t explicitSize) {
  if (UNLIKELY(explicitSize > kFastAllocMaxSize)) {
    return 0;
  }
  // we need to make sure the string class passes the following condition
  // somehow it does, coincidently
  if (UNLIKELY(!modifier::IsFastAllocClass(reinterpret_cast<ClassMetadata*>(klass)->flag))) {
    return 0;
  }
  // we additionally need to align the size, because the explicit size can be any number
  // this costs additional time
  address_t alignedSize = AllocUtilRndUp(explicitSize, kAllocAlign);
  address_t objAddr = MRT_TryNew(alignedSize, klass);
  if (UNLIKELY(objAddr == 0)) {
    return 0;
  }

  return objAddr;
}

// try new array: it takes fast path if possible, otherwise returns 0
// see slow path code for more info
// assumption: class initialised
template<ArrayElemSize elemSizeExp>
__attribute__ ((always_inline, used))
static address_t MRT_TryNewArray(size_t nElems, address_t klass) {
  if (UNLIKELY(nElems > (kFastAllocArrayMaxSize >> static_cast<size_t>(elemSizeExp)))) {
    return 0;
  }
  size_t contentSize = nElems << static_cast<size_t>(elemSizeExp);
  // this size must be aligned because the allocator fast path takes it as an assumption
  size_t objSize = kJavaArrayContentOffset + AllocUtilRndUp(contentSize, kAllocAlign);
  // potentially optimise this given the constraint that we are an array class?
  address_t objAddr = MRT_TryNew(objSize, klass);
  if (LIKELY(objAddr != 0)) {
    ArrayLength(objAddr) = static_cast<uint32_t>(nElems);
    // not sure if we want this fence
    std::atomic_thread_fence(std::memory_order_release);
  }
  return objAddr;
}

// try new array: it takes fast path if possible, otherwise returns 0
// this function also accepts an elem size variable, in case the elem size is not
// immediately available
// the previous function is preferred over this one
// see slow path code for more info
// assumption: class initialised
__attribute__ ((always_inline, used))
static address_t MRT_TryNewArray(size_t elemSize, size_t nElems, address_t klass) {
  // multiplication cost more cpu clocks, we don't like it, prefer shifting
  if (UNLIKELY(nElems > (kFastAllocArrayMaxSize / elemSize))) {
    return 0;
  }
  size_t contentSize = elemSize * nElems;
  // this size must be aligned because the allocator fast path takes it as an assumption
  size_t objSize = kJavaArrayContentOffset + AllocUtilRndUp(contentSize, kAllocAlign);
  // potentially optimise this given the constraint that we are an array class?
  address_t objAddr = MRT_TryNew(objSize, klass);
  if (LIKELY(objAddr != 0)) {
    ArrayLength(objAddr) = static_cast<uint32_t>(nElems);
    // not sure if we want this fence
    std::atomic_thread_fence(std::memory_order_release);
  }
  return objAddr;
}
#else
inline address_t MRT_TryNewObject(address_t) {
  return 0;
}
inline address_t MRT_TryNewObject(address_t, size_t) {
  return 0;
}
template<ArrayElemSize elemSizeExp>
__attribute__ ((always_inline, used))
static address_t MRT_TryNewArray(size_t, address_t) {
  return 0;
}
__attribute__ ((always_inline, used))
static address_t MRT_TryNewArray(size_t, size_t, address_t) {
  return 0;
}
#endif
}

#endif
