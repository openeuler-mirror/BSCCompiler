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
#ifndef MAPLE_RUNTIME_SIZES_H
#define MAPLE_RUNTIME_SIZES_H

#include <cstdlib>
#include <cstdint>
#include <cstddef>
#include <atomic>

#ifdef OBJSCAN_DEBUG
#include <cstdio>
#include <cinttypes>
#endif

#include "thread_offsets.h"
#include "mm_utils.h"
#include "address.h"
#include "chelper.h"
#include "exception/mrt_exception.h"

namespace maplert {
// Paranoid assertions. Please find clues in language specifications or ABIs
// that these predicates must hold.
#if defined(__aarch64__)
static_assert(sizeof(uint64_t) == sizeof(size_t),
    "size_t does not have the same size as uint64_t. We are probably not "
    "working on a 64-bit platform.");

static_assert(sizeof(int64_t) == sizeof(ssize_t),
    "ssize_t does not have the same size as int64_t. We are probably not "
    "working on a 64-bit platform.");
#elif defined(__arm__)
static_assert(sizeof(uint32_t) == sizeof(size_t),
    "size_t does not have the same size as uint32_t. We are probably not "
    "working on a 32-bit platform.");

static_assert(sizeof(int32_t) == sizeof(ssize_t),
    "ssize_t does not have the same size as int32_t. We are probably not "
    "working on a 64-bit platform.");
#endif

static_assert(sizeof(uintptr_t) == sizeof(size_t),
    "size_t does not have the same size as uintptr_t.");

static_assert(sizeof(intptr_t) == sizeof(ssize_t),
    "ssize_t does not have the same size as intptr_t.");

// AArch64-specific sizes.
const size_t kHWordBytes = 2;
const size_t kWordBytes = 4;
const size_t kDWordBytes = 8;
const size_t kQWordBytes = 16;

#if CONFIG_JSAN
const int kJsanHeaderSize = kDWordBytes;

const offset_t kOffsetJsanHeader = -(kDWordBytes + kDWordBytes);
const offset_t kOffsetObjStatus = -(kDWordBytes + kWordBytes);

static constexpr uint32_t kObjMagic = 0xabcabcab;
static constexpr uint32_t kObjStatusUnknown = 0x0;
static constexpr uint32_t kObjStatusAllocated = 0x1;
static constexpr uint32_t kObjStatusQuarantined = 0x2;

static inline void JSANSetAllocated(address_t objAddr) {
  *reinterpret_cast<uint32_t*>(objAddr + kOffsetJsanHeader) = kObjMagic;
  *reinterpret_cast<uint32_t*>(objAddr + kOffsetObjStatus) = kObjStatusAllocated;
}

static inline void JSANSetQuarantined(address_t objAddr) {
  *reinterpret_cast<uint32_t*>(objAddr + kOffsetObjStatus) = kObjStatusQuarantined;
}

static inline void JSANSetFreed(address_t objAddr) {
  *reinterpret_cast<uint32_t*>(objAddr + kOffsetJsanHeader) = 0;
  *reinterpret_cast<uint32_t*>(objAddr + kOffsetObjStatus) = kObjStatusUnknown;
}

static inline bool JSANIsValidObjAddress(address_t objAddr) {
  return *reinterpret_cast<uint32_t*>(objAddr + kOffsetJsanHeader) == kObjMagic;
}

static inline uint32_t JSANGetObjStatus(address_t objAddr) {
  if (!JSANIsValidObjAddress(objAddr)) {
    return kObjStatusUnknown;
  }
  return *reinterpret_cast<uint32_t*>(objAddr + kOffsetObjStatus);
}
#else
const int kJsanHeaderSize = 0;
#endif

#if RC_HOT_OBJECT_DATA_COLLECT
const size_t kTrackFrameNum = 4;
const size_t kRcHotHeaderSize = kDWordBytes + kDWordBytes * kTrackFrameNum;

// The offset of the RC Operation count field.
const offset_t kOffsetRcOperationCount = -(kWordBytes + kDWordBytes);

// The offset of the GC count field.
const offset_t kOffsetGcCountCount = -(kDWordBytes + kDWordBytes);

const offset_t kOffsetTrackPC = -(kDWordBytes + kDWordBytes + kDWordBytes * kTrackFrameNum);

static inline uint32_t RCOperationCount(address_t objAddr) {
  return (*reinterpret_cast<uint32_t*>(objAddr + kOffsetRcOperationCount));
}

static inline uint32_t GCCount(address_t objAddr) {
  return (*reinterpret_cast<uint32_t*>(objAddr + kOffsetGcCountCount));
}

static inline std::atomic<uint32_t> &RCOperationCountAtomicLVal(address_t objAddr) {
  return AddrToLValAtomic<uint32_t>(objAddr + kOffsetRcOperationCount);
}

static inline std::atomic<uint32_t> &GCCountAtomicLVal(address_t objAddr) {
  return AddrToLValAtomic<uint32_t>(objAddr + kOffsetGcCountCount);
}

extern std::atomic<uint64_t> totalSkipedRCOpCount;
extern std::atomic<uint64_t> totalRCOperationCount;

static inline bool IsRCSkiped(address_t obj);

static inline void StatsRCOperationCount(address_t obj) {
  totalRCOperationCount.fetch_add(1, memory_order_relaxed);
  if (IsRCSkiped(obj)) {
    totalSkipedRCOpCount.fetch_add(1, memory_order_relaxed);
  } else {
    if (RCOperationCount(obj) < std::numeric_limits<uint32_t>::max()) {
      RCOperationCountAtomicLVal(obj).fetch_add(1, memory_order_relaxed);
    }
  }
}

inline void StatsGCCount(address_t obj) {
  if (GCCount(obj) < std::numeric_limits<uint32_t>::max()) {
    GCCountAtomicLVal(obj).fetch_add(1, memory_order_relaxed);
  }
}
#else
const int kRcHotHeaderSize = 0;
#define StatsRCOperationCount(obj)
#endif

// Heap object header size.
const size_t kHeaderSize = kDWordBytes + kJsanHeaderSize + kRcHotHeaderSize;

// The offset of the reference count field.
const offset_t kOffsetRefCount = -kWordBytes;
// The offset of the GC header.
const offset_t kOffsetGCHeader = -kDWordBytes;

// Object Header bits
// Total 64 bit, split int two 32 bits word (RC Header/GC Header)
// RC Header: record reference counting
// GC Header: record misc informations: attributes, GCITB prototypes
//
// RC Header
// [15 -  0]  Strong RC
// [21 - 16]  Resurrectable Weak RC: soft/weak/global weak reference
// [28 - 22]  Weak RC: phantom reference/weak annotation
// [29 - 29]  Weak Collected
// [31 - 30]  RC Cycle Collect Color
//
// Strong RC: normal references like: stack reference\normal field
// reference\runtime reference(string table, classloader table, ..).
// In normals case object can be release when its Strong RC reach zero.
// If Weak RC is not zero, object release processing is differently
// according to Weak RC's type.
//
// Weak RC: If Object has Weak RC, it comes from phantom reference or
// @weak annotation. Object can be relased immedidately when Strong RC is zero
// while Weak RC not zero.
//
// Resurrectable Weak RC: special weak reference from soft/weak/weak global
// reference. Object can not be relased immedidately when Strong RC is zero
// while Resurrectable Weak RC not zero. Because these reference has cache
// semantic and need be resurrect with Reference.get(). Object with Resurrectable
// Weak RC is collected periodically in reference processor thread.
//
// Weak Collected: If set, weak reference to this object in invalidated.
// Reference.get returns null.
//
// RC Cycle Collect Color: Color used in Cycle pattern. Color is atmoiclly set
// and checked to avoiding racing condition in cycle pattern match and mutator
// operations.
//
static constexpr uint32_t kRCBits = 0x0000ffff;
static constexpr uint32_t kRCBitsMsb = 0x00008000;

static constexpr uint32_t kResurrectWeakRcBits = 0x003f0000;
static constexpr uint32_t kResurrectWeakOneBit = 0x00010000;
static constexpr uint32_t KResurrectWeakRcBitsShift = 16;
static constexpr uint32_t kMaxResurrectWeakRC = kResurrectWeakRcBits >> KResurrectWeakRcBitsShift;


static constexpr uint32_t kWeakRcBits = 0x1fc00000;
static constexpr uint32_t kWeakRCOneBit = 0x00400000;
static constexpr uint32_t kWeakRcBitsShift = 22;
static constexpr uint32_t kMaxWeakRC = kWeakRcBits >> kWeakRcBitsShift;

static constexpr uint32_t kWeakCollectedBit = 0x20000000;

static constexpr uint32_t kRCCycleColorMask = 0xc0000000;
static constexpr uint32_t kRCCycleColorBlack = 0x00000000;
static constexpr uint32_t kRCCycleColorGray = 0x40000000;
static constexpr uint32_t kRCCycleColorWhite = 0x80000000;
static constexpr uint32_t kRCCycleColorBrown = 0xc0000000; // color for decref, just for debug

// GC Header
// [0] Allocated:   Flag to tell if the object is allocated *by the allocator*
// [1] Dirty:       Tracing queued bit used in cocurrent marking
// [2] Release:     RC released object during backup tracing concurrent marking
// [3] RCTracing:   Object with this bit, all inc/dec will be traced, debug only
// [4] WrapWeakRef: Object need be put into a weak reference for periodlly dead check.
// [5] HasChildRef: Object has reference to other object.
// ...
// [20]      ReferenceActive: Reference is recently get, used in reference processor
// [21]      EnqueuedFinalizable: debug only, finalizalbe object is eqneued
// [22]      Array:       Object is array
// [23]      Reference:   Object is reference type
// [24]      Finalizable: Object is finalizable
// [27-25]   Cycle pattern RC min value
// [30-28]   Cycle pattern RC max value
// [31] Cycle pattern: Object with this bit is candiate for cycle collection
//
// The bottom three bits of GCHeader(obj) are used for color.
// NOTE: Only used by cycle detection, not mark-sweep.
//
// Bit for flags word (-8 offset)
static constexpr uint32_t kAllocatedBit = 0x1;
static constexpr uint32_t kDirtyBit = 0x2; // set if child refs modified during concurrent marking.
static constexpr uint32_t kReleasedBit = 0x4; // set if freed during concurrent marking.
static constexpr uint32_t kRCTracingBit = 0x8;
static constexpr uint32_t kHasChildRef = 0x40; // object has no child reference, need update compiler
static constexpr uint32_t kMygoteObjBit = 0x80; // object is mygote object, no free, likely immutable

static constexpr uint32_t kReferenceActiveBit = 0x00100000; // mark if reference is got recently and clear in rp
static constexpr uint32_t kEnqueuedFinalizableBit = 0x00200000;
static constexpr uint32_t kArrayBit = 0x00400000;
static constexpr uint32_t kReferenceBit = 0x00800000;
static constexpr uint32_t kFinalizableBit = 0x01000000;
static constexpr uint32_t kCycleRCMinMask = 0x0e000000;
static constexpr uint32_t kCycleRCMaxMask = 0x70000000;
static constexpr uint32_t kCyclePatternBit = 0x80000000;

static constexpr uint32_t kCycleRCMinShift = 25;
static constexpr uint32_t kCycleRCMaxShift = 28;
#define CYCLE_MIN_RC(header) (((static_cast<uint32_t>(header)) & kCycleRCMinMask) >> kCycleRCMinShift)
#define CYCLE_MAX_RC(header) (((static_cast<uint32_t>(header)) & kCycleRCMaxMask) >> kCycleRCMaxShift)

// ------------------------------------------------------------------
// About kHasChildRef and kArrayBit:
//                             kHasChildRef    kArrayBit
// object without child ref        0              0
// object with child ref           1              0
// primitive array                 0              1
// object array                    1              1
// ------------------------------------------------------------------
const size_t kJavaObjAlignment = kDWordBytes;

// Array content is 8 bytes align for long/double array type
const offset_t kJavaArrayLengthOffset = 12; // shadow + monitor + [padding], fixed.
const offset_t kJavaArrayContentOffset = 16; // fixed for 8B alignment

struct DecoupleAllocHeader {
  using FieldType = uint32_t;
  FieldType size;
  FieldType tag;
  // Object  header align to 8 , header align to 8 too
  static constexpr size_t kHeaderSize = 8;
  static constexpr size_t kSizeOffset = kHeaderSize;
  static constexpr size_t kTagOffset = kHeaderSize - sizeof(FieldType);

#if defined(__aarch64__)
  static constexpr size_t kMaxSize = (1UL << 32); // 4GB
#elif defined(__arm__)
  static constexpr size_t kMaxSize = (1UL << 31); // lower 2GB address space
#endif

  static inline size_t GetSize(address_t objAddr) {
    return static_cast<size_t>(*reinterpret_cast<FieldType*>(objAddr - kSizeOffset));
  }

  static inline void SetSize(address_t objAddr, size_t size) {
    *reinterpret_cast<FieldType*>(objAddr - kSizeOffset) = static_cast<FieldType>(size);
  }

  static inline int GetTag(address_t objAddr) {
    return static_cast<int>(*reinterpret_cast<FieldType*>(objAddr - kTagOffset));
  }

  static inline void SetTag(address_t objAddr, int tag) {
    *reinterpret_cast<FieldType*>(objAddr - kTagOffset) = static_cast<FieldType>(tag);
  }

  static inline void SetHeader(address_t objAddr, int tag, size_t size) {
    SetTag(objAddr, tag);
    SetSize(objAddr, size);
  }
};

#define HEADER_CHECK_VALID_HEAP_OBJECT __MRT_DEBUG_COND_FALSE
#if HEADER_CHECK_VALID_HEAP_OBJECT
static inline void CheckValidHeapObject(address_t obj) {
  if (!IS_HEAP_OBJ(obj)) {
    LOG(FATAL) << "invalid heap obj addr: " << std::hex << obj << maple::endl;
  }
}
#define CHECK_HEADER_VALID_OBJ(obj) CheckValidHeapObject(obj)
#else
#define CHECK_HEADER_VALID_OBJ(obj)
#endif

// Accessors to Java array-specific fields
static inline uint32_t &ArrayLength(address_t objAddr) {
  return AddrToLVal<uint32_t>(objAddr + kJavaArrayLengthOffset);
}

// Accessors for gctib pointer from class metadata
static inline address_t GCTibPtr(address_t objAddr) {
  CHECK_HEADER_VALID_OBJ(objAddr);
  return reinterpret_cast<address_t>(MObject::Cast<MObject>(objAddr)->GetClass()->GetGctib());
}

static inline GCTibGCInfo &GCInfo(address_t objAddr) {
  CHECK_HEADER_VALID_OBJ(objAddr);
  return AddrToLVal<struct GCTibGCInfo>(GCTibPtr(objAddr));
}

static inline uint32_t GCHeader(address_t objAddr) {
  CHECK_HEADER_VALID_OBJ(objAddr);
  return *reinterpret_cast<uint32_t*>(objAddr + kOffsetGCHeader);
}

static inline uint32_t &GCHeaderLVal(address_t objAddr) {
  CHECK_HEADER_VALID_OBJ(objAddr);
  return AddrToLVal<uint32_t>(objAddr + kOffsetGCHeader);
}

static inline std::atomic<uint32_t> &GCHeaderAtomic(address_t objAddr) {
  CHECK_HEADER_VALID_OBJ(objAddr);
  return AddrToLValAtomic<uint32_t>(objAddr + kOffsetGCHeader);
}

static inline void SetGCHeader(address_t objAddr, uint32_t val) {
  CHECK_HEADER_VALID_OBJ(objAddr);
  uint32_t *gcHeaderAddr = reinterpret_cast<uint32_t*>(objAddr + kOffsetGCHeader);
  *gcHeaderAddr = val;
}

static inline uint32_t RCHeader(address_t objAddr) {
  CHECK_HEADER_VALID_OBJ(objAddr);
  return *reinterpret_cast<uint32_t*>(objAddr + kOffsetRefCount);
}

static inline uint32_t GetRCFromRCHeader(uint32_t rcHeader) {
  return rcHeader & kRCBits;
}

static inline uint32_t GetResurrectWeakRCFromRCHeader(uint32_t rcHeader) {
  return (rcHeader & kResurrectWeakRcBits) >> KResurrectWeakRcBitsShift;
}

static inline uint32_t GetWeakRCFromRCHeader(uint32_t rcHeader) {
  return (rcHeader & kWeakRcBits) >> kWeakRcBitsShift;
}

static inline uint32_t GetTotalWeakRCFromRCHeader(uint32_t rcHeader) {
  return (rcHeader & (kWeakRcBits | kResurrectWeakRcBits));
}

static inline uint32_t GetTotalRCFromRCHeader(uint32_t rcHeader) {
  return (rcHeader & (kWeakRcBits | kResurrectWeakRcBits | kRCBits));
}

static inline bool IsWeakCollectedFromRCHeader(uint32_t rcHeader) {
  return (rcHeader & kWeakCollectedBit) != 0;
}

static inline bool IsRCCollectableFromRCHeader(uint32_t rcHeader) {
  uint32_t totalRC = GetTotalRCFromRCHeader(rcHeader);
  return (totalRC == 0) || ((totalRC == kWeakRCOneBit) && !IsWeakCollectedFromRCHeader(rcHeader));
}

static inline uint32_t RefCount(address_t obj) {
  CHECK_HEADER_VALID_OBJ(obj);
  return GetRCFromRCHeader(RCHeader(obj));
}

static inline uint32_t ResurrectWeakRefCount(address_t obj) {
  CHECK_HEADER_VALID_OBJ(obj);
  return GetResurrectWeakRCFromRCHeader(RCHeader(obj));
}

static inline uint32_t WeakRefCount(address_t obj) {
  CHECK_HEADER_VALID_OBJ(obj);
  return GetWeakRCFromRCHeader(RCHeader(obj));
}

static inline uint32_t TotalRefCount(address_t obj) {
  CHECK_HEADER_VALID_OBJ(obj);
  return GetTotalRCFromRCHeader(RCHeader(obj));
}

static inline uint32_t TotalWeakRefCount(address_t obj) {
  CHECK_HEADER_VALID_OBJ(obj);
  return GetTotalWeakRCFromRCHeader(RCHeader(obj));
}

static inline bool IsRCCollectable(address_t obj) {
  CHECK_HEADER_VALID_OBJ(obj);
  return IsRCCollectableFromRCHeader(RCHeader(obj));
}

static inline bool IsWeakCollected(address_t obj) {
  CHECK_HEADER_VALID_OBJ(obj);
  return IsWeakCollectedFromRCHeader(RCHeader(obj));
}

static inline uint32_t& RefCountLVal(address_t objAddr) {
  CHECK_HEADER_VALID_OBJ(objAddr);
  return AddrToLVal<uint32_t>(objAddr + kOffsetRefCount);
}

static inline std::atomic<uint32_t>& RefCountAtomicLVal(address_t objAddr) {
  CHECK_HEADER_VALID_OBJ(objAddr);
  return AddrToLValAtomic<uint32_t>(objAddr + kOffsetRefCount);
}

// quickly check if need check cycle pattern for this object
// 1. has cycle pattern
// 2. rc in range
static inline bool IsValidForCyclePatterMatch(uint32_t rcFlags, uint32_t curRC) {
  return (rcFlags & kCyclePatternBit) && (curRC <= CYCLE_MAX_RC(rcFlags)) && (curRC >= CYCLE_MIN_RC(rcFlags));
}

static inline void ClearCyclePatternBit(address_t objAddr) {
  (void)GCHeaderAtomic(objAddr).fetch_and((~kCyclePatternBit), std::memory_order_release);
}

static inline uint32_t SetCycleMaxRC(uint32_t oldRcFlags, uint32_t max) {
  return (oldRcFlags & ~kCycleRCMaxMask) | (max << kCycleRCMaxShift);
}

static inline uint32_t SetCycleMinRC(uint32_t oldRcFlags, uint32_t min) {
  return (oldRcFlags & ~kCycleRCMinMask) | (min << kCycleRCMinShift);
}

static inline void InitWithAllocatedBit(address_t objAddr) {
  GCHeaderLVal(objAddr) = kAllocatedBit;
}

static inline void ClearAllocatedBit(address_t objAddr) {
  GCHeaderLVal(objAddr) &= ~kAllocatedBit;
}

static inline bool IsAllocatedByAllocator(address_t objAddr) {
  return (GCHeader(objAddr) & kAllocatedBit) != 0;
}

static inline bool IsDirty(address_t objAddr) {
  uint32_t header = GCHeaderAtomic(objAddr).load(std::memory_order_acquire);
  return (header & kDirtyBit) != 0;
}

static inline void SetDirty(address_t objAddr) {
  (void)GCHeaderAtomic(objAddr).fetch_or(kDirtyBit, std::memory_order_release);
}

static inline void ClearDirtyBit(address_t objAddr) {
  // this is called when the world is stopped, so no need atomic.
  GCHeaderLVal(objAddr) &= (~kDirtyBit);
}

static inline void SetReleasedBit(address_t objAddr) {
  (void)GCHeaderAtomic(objAddr).fetch_or(kReleasedBit, std::memory_order_release);
}

static inline bool HasReleasedBit(address_t objAddr) {
  return (GCHeader(objAddr) & kReleasedBit) != 0;
}

static inline void SetMygoteBit(address_t objAddr) {
  GCHeaderLVal(objAddr) |= kMygoteObjBit; // not clearable
}

static inline bool IsMygoteObj(address_t objAddr) {
  return (GCHeader(objAddr) & kMygoteObjBit) != 0;
}

static inline void SetReferenceActive(address_t objAddr) {
  if (!IsMygoteObj(objAddr)) {
    GCHeaderLVal(objAddr) |= kReferenceActiveBit;
  }
}

static inline void ClearReferenceActive(address_t objAddr) {
  if (!IsMygoteObj(objAddr)) {
    GCHeaderLVal(objAddr) &= ~(kReferenceActiveBit);
  }
}

static inline bool IsReferenceActive(address_t objAddr) {
  return (GCHeader(objAddr) & kReferenceActiveBit) != 0;
}

static inline void SetObjFinalizable(address_t objAddr) {
  // should be set once at creation time.
  GCHeaderLVal(objAddr) |= kFinalizableBit;
}

static inline void SetEnqueuedObjFinalizable(address_t objAddr) {
  (void)GCHeaderAtomic(objAddr).fetch_or(kEnqueuedFinalizableBit, std::memory_order_relaxed);
}

static inline void ClearObjFinalizable(address_t objAddr) {
  // should be cleared only by FinalizerThread
  (void)GCHeaderAtomic(objAddr).fetch_and(~(kFinalizableBit | kEnqueuedFinalizableBit), std::memory_order_relaxed);
}

static inline bool IsEnqueuedObjFinalizable(address_t objAddr) {
  return (GCHeader(objAddr) & kEnqueuedFinalizableBit) != 0;
}

static inline bool IsObjFinalizable(address_t objAddr) {
  return (GCHeader(objAddr) & kFinalizableBit) != 0;
}

static inline bool IsObjResurrectable(address_t objAddr) {
  return IsObjFinalizable(objAddr) && !IsEnqueuedObjFinalizable(objAddr);
}

// Trace Bit is settting for debugging and trace object life cycle
static inline void SetTraceBit(address_t objAddr) {
  (void)GCHeaderAtomic(objAddr).fetch_or(kRCTracingBit, std::memory_order_relaxed);
}

static inline void ClearTraceBit(address_t objAddr) {
  (void)GCHeaderAtomic(objAddr).fetch_and((~kRCTracingBit), std::memory_order_relaxed);
}

static inline bool IsTraceObj(address_t objAddr) {
  if (!IS_HEAP_OBJ(objAddr)) {
    return false;
  }
  if (TotalRefCount(objAddr) == 0) {
    return false;
  }
  return (GCHeader(objAddr) & kRCTracingBit) != 0;
}

static inline bool HasChildRef(address_t objAddr) {
  return (GCHeader(objAddr) & kHasChildRef) != 0;
}

static inline void SetObjReference(address_t objAddr) {
  GCHeaderLVal(objAddr) |= kReferenceBit;;
}

static inline bool IsObjReference(address_t objAddr) {
  return (GCHeader(objAddr) & kReferenceBit) != 0;
}

static inline bool IsArray(address_t objAddr) {
  return (GCHeader(objAddr) & kArrayBit) != 0;
}

static inline bool IsObjectArray(address_t objAddr) {
  return (GCHeader(objAddr) & (kArrayBit | kHasChildRef)) == (kArrayBit | kHasChildRef);
}

static inline bool SkipRC(uint32_t rcHeader) {
  return (rcHeader & kRCBitsMsb) != 0;
}

static inline bool IsRCOverflow(uint32_t rc) {
  return SkipRC(rc);
}

static inline bool IsRCSkiped(address_t obj) {
  CHECK_HEADER_VALID_OBJ(obj);
  return SkipRC(RCHeader(obj));
}

static inline void SetRCOverflow(address_t objAddr) {
  if (!IsRCSkiped(objAddr)) {
    RefCountLVal(objAddr) |= kRCBitsMsb;
  }
}

extern "C" {
extern void MRT_BuiltinAbortSaferegister(maple::address_t addr, const char *clsName);
}

#ifdef __ANDROID__
static void inline AbortWithHeader(address_t obj) {
  char *clsName = nullptr;
  static constexpr address_t lowerMetaBound  = 0x80000000;
  static constexpr address_t higherMetaBound = 0xdfffffff;
  if (IS_HEAP_OBJ(obj)) {
    __MRT_ASSERT(MObject::Cast<MObject>(obj) != nullptr, "AbortWithHeader: obj is a nullptr.");
    MClass *cls = MObject::Cast<MObject>(obj)->GetClass();
    address_t clsAddr = reinterpret_cast<address_t>(cls);
    if ((clsAddr >= lowerMetaBound) && (clsAddr < higherMetaBound) &&
        ((clsAddr % kJavaObjAlignment) == 0) && MRT_IsValidClass(*cls)) {
      clsName = cls->GetName();
    }
  }
  MRT_BuiltinAbortSaferegister(obj, clsName);
}
#else
static void inline AbortWithHeader(address_t obj) {
  __MRT_ASSERT(MObject::Cast<MObject>(obj) != nullptr, "AbortWithHeader: obj is a nullptr.");
  MClass *cls = MObject::Cast<MObject>(obj)->GetClass();
  std::cout << "Inc/Dec from 0 " << std::hex << obj << " " << GCHeader(obj) <<
      " " << RCHeader(obj) << " lock " << *(reinterpret_cast<uint32_t*>(obj + kLockWordOffset)) <<
      std::dec << cls->GetName() << std::endl;

  MRT_BuiltinAbortSaferegister(obj, nullptr);
}
#endif

#if MRT_DEBUG_DOUBLE_FREE && !ALLOC_USE_FAST_PATH
static inline void CheckDoubleFree(address_t objAddr) {
  if (UNLIKELY(!IsAllocatedByAllocator(objAddr))) {
    LOG(ERROR) << "double freeing " << objAddr;
    AbortWithHeader(objAddr);
  }
}
#else
#define CheckDoubleFree(x)
#endif

// Object reference field iteration utilities, used in
// Recursive RC update when release
// Backup tracing mark/sweep
static const uint64_t kNotRefBits = 0;
static const uint64_t kNormalRefBits = 1;
static const uint64_t kWeakRefBits = 2;
static const uint64_t kUnownedRefBits = 3;
static const uint64_t kRefBitsMask = 3;
static const uint32_t kBitsPerRefWord = 2;

constexpr uint32_t kBitsPerByte = 8;
static const uint32_t kRefWordPerMapWord = ((sizeof(uint64_t) * kBitsPerByte) / kBitsPerRefWord);

template<class RefFunc>
void ForEachRefFieldNonArrayObj(address_t objAddr, RefFunc &&refFunc) {
  struct GCTibGCInfo &gcInf = GCInfo(objAddr);

  // number of bitmap words from GCTIB.
  uint32_t bitmapWordsCount = gcInf.nBitmapWords;
  uint64_t *bitmapWords = gcInf.bitmapWords;
  bool allNormal = Collector::Instance().Type() == kMarkSweep;

  // start address of fields.
  address_t baseAddr = objAddr;
  // for each bitmap word.
  for (size_t i = 0; i < bitmapWordsCount; ++i) {
    uint64_t bitmapWord = bitmapWords[i];
    address_t fieldAddr = baseAddr;

    // for each bit in bitmap.
    while (LIKELY(bitmapWord != 0)) {
      uint64_t wordBits = bitmapWord & kRefBitsMask;
      if (wordBits != kNotRefBits) {
        if (allNormal) {
          wordBits = kNormalRefBits;
        }
        refFunc(AddrToLVal<reffield_t>(fieldAddr), wordBits);
      }
      // go next ref word.
      bitmapWord >>= kBitsPerRefWord;
      fieldAddr += sizeof(reffield_t);
    }

    // go next bitmap word.
    baseAddr += (sizeof(reffield_t) * kRefWordPerMapWord);
  }
}

// Call func on each element in an object array.
template<class UnaryFunction>
void ForEachObjectArrayElement(address_t objAddr, UnaryFunction &&func) {
  // we assume that both kHasChildRef & kArrayBit are set.
  // take array length and content.
  uint32_t arrayLengthVal = AddrToLVal<uint32_t>(objAddr + kJavaArrayLengthOffset);
  reffield_t *arrayContent = reinterpret_cast<reffield_t*>(objAddr + kJavaArrayContentOffset);

  // for each object in array.
  for (uint32_t i = 0; i < arrayLengthVal; ++i) {
    func(arrayContent[i], kNormalRefBits);
  }
}

// Use this macro to ensure that kHasChildRef & kArrayBit
// is checked before __ForEachXXXX() get called
// function should accept 'reffield_t&' as argument
// pattern for the call-back function:
// ForEachRefField(objAddr, [](reffield_t &field) { address_t ref = LoadRefField(field); }
// similar constraint applies to the other 2 macros.
#define ForEachRefField(obj, refFunc) do { \
  if (maplert::HasChildRef(obj)) { \
    if (UNLIKELY(IsObjectArray(obj))) { \
      ForEachObjectArrayElement(obj, refFunc); \
    } else { \
      ForEachRefFieldNonArrayObj(obj, refFunc); \
    } \
  } \
} while (0)

// Use this macro if kHasChildRef already checked.
#define DoForEachRefField(obj, refFunc) do { \
  if (UNLIKELY(IsObjectArray(obj))) { \
    ForEachObjectArrayElement(obj, refFunc); \
  } else { \
    ForEachRefFieldNonArrayObj(obj, refFunc); \
  } \
} while (0)
} // namespace maplert

#endif
