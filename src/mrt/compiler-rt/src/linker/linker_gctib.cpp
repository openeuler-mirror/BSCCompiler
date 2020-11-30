/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "linker/linker_gctib.h"

#include <dirent.h>
#include <sys/mman.h>
#include "base/logging.h"
#include "collector/cp_generator.h"

namespace maplert {
// Get all offset of reference field.
void RefCal(const MClass &klass, vector<uint64_t> &refOffsets, vector<uint64_t> &weakOffsets,
    vector<uint64_t> &unownedOffsets, uint64_t &maxRefOffset) {
  if (&klass == WellKnown::GetMClassObject()) {
    return;
  }
  FieldMeta *fields = klass.GetFieldMetas();
  uint32_t numOfField = klass.GetNumOfFields();
  for (uint32_t i = 0; i < numOfField; ++i) {
    FieldMeta *field = &fields[i];
    if (field->IsPrimitive() || field->IsStatic()) {
      continue;
    }
    uint64_t fieldOffset = field->GetOffset();
    if (fieldOffset == 0) {
      // skip first meta
      continue;
    }
    if (fieldOffset > maxRefOffset) {
      maxRefOffset = fieldOffset;
    }
    size_t fieldModifier = field->GetMod();
    if (modifier::IsUnowned(fieldModifier)) {
      unownedOffsets.push_back(fieldOffset);
    } else if (modifier::IsWeakRef(fieldModifier)) {
      weakOffsets.push_back(fieldOffset);
    } else {
      refOffsets.push_back(fieldOffset);
    }
  }

  if (klass.GetNumOfSuperClasses() == 0) {
    return;
  }

  if (klass.IsColdClass() && !klass.IsLazyBinding()) {
    LinkerAPI::Instance().ResolveColdClassSymbol(klass.AsJclass());
  }

  MClass **superclassArray = klass.GetSuperClassArrayPtr();
  MClass *superClass = superclassArray[0];
  if (superClass != WellKnown::GetMClassObject() && !superClass->IsInterface()) {
    RefCal(*superClass, refOffsets, weakOffsets, unownedOffsets, maxRefOffset);
  }
  return;
}

void DumpGctib(const struct GCTibGCInfo &gcTib) {
  ostringstream oss;
  for (size_t i = 0; i != gcTib.nBitmapWords; ++i) {
    oss << std::hex << gcTib.bitmapWords[i] << '\t';
  }
  LOG(WARNING) << "[Gctib]headerProto:" << gcTib.headerProto <<
      ", nBitmapWords:" << gcTib.nBitmapWords <<
      ", bitmapWords:" << oss.str() << endl;
}

static inline uint64_t OffsetToMapWordIndex(uint64_t offset) {
  return (offset / sizeof(reffield_t)) / kRefWordPerMapWord;
}

static inline uint64_t OffsetToInMapWordOffset(uint64_t offset) {
  return ((offset / sizeof(reffield_t)) % kRefWordPerMapWord) * kBitsPerRefWord;
}

static void FillBitmapWords(vector<uint64_t> refOffsets, struct GCTibGCInfo *newGctibPtr, uint64_t refType) {
  for (auto refOffset : refOffsets) {
    uint64_t index = OffsetToMapWordIndex(refOffset);
    uint64_t shift = OffsetToInMapWordOffset(refOffset);
    newGctibPtr->bitmapWords[index] |= (refType << shift);
  }
}

bool ReGenGctib(ClassMetadata *classInfo, bool forceUpdate) {
  vector<uint64_t> refOffsets;
  vector<uint64_t> weakOffsets;
  vector<uint64_t> unownedOffsets;
  uint64_t maxRefOffset = 0;
  MClass *cls = reinterpret_cast<MClass*>(classInfo);
  if (cls->IsInterface()) {
    return false;
  }
  RefCal(*cls, refOffsets, weakOffsets, unownedOffsets, maxRefOffset);

  struct GCTibGCInfo *gcInfo = reinterpret_cast<struct GCTibGCInfo*>(cls->GetGctib());
  // construct new GCTIB
  uint64_t nBitmapWords = 0;
  if (maxRefOffset > 0) {
    nBitmapWords = OffsetToMapWordIndex(maxRefOffset) + 1;
  }
  uint32_t gctibByteSize = static_cast<uint32_t>((nBitmapWords + 1) * sizeof(uint64_t));
  struct GCTibGCInfo *newGctibPtr = reinterpret_cast<struct GCTibGCInfo*>(calloc(gctibByteSize, sizeof(char)));
  if (newGctibPtr == nullptr) {
    LOG(FATAL) << "malloc gctib failed" << endl;
    return false;
  }

  // fill bitmap
  FillBitmapWords(refOffsets, newGctibPtr, kNormalRefBits);
  FillBitmapWords(weakOffsets, newGctibPtr, kWeakRefBits);
  FillBitmapWords(unownedOffsets, newGctibPtr, kUnownedRefBits);
  newGctibPtr->nBitmapWords = static_cast<uint32_t>(nBitmapWords);
  if (gcInfo != nullptr) {
    if (maxRefOffset > 0) {
      newGctibPtr->headerProto = gcInfo->headerProto | kHasChildRef;
    } else {
      newGctibPtr->headerProto = gcInfo->headerProto & ~(kHasChildRef);
    }
    newGctibPtr->headerProto = newGctibPtr->headerProto & ~(kCyclePatternBit);
    if ((!forceUpdate || VLOG_IS_ON(mpllinker)) && (gcInfo->nBitmapWords == nBitmapWords) &&
        (memcmp(gcInfo, newGctibPtr, gctibByteSize) == 0)) {
      free(newGctibPtr);
      return false;
    } else if (VLOG_IS_ON(mpllinker)) {
      LOG(WARNING) << "[Gctib] ResolveClassGctib:" << cls->GetName() << " mismatch" << endl;
      DumpGctib(*gcInfo);
      DumpGctib(*newGctibPtr);
    }
  }
  classInfo->gctib.SetGctibRef(newGctibPtr);
  if (gcInfo != nullptr && Collector::Instance().Type() == kNaiveRC && ClassCycleManager::HasDynamicLoadPattern(cls)) {
    free(gcInfo);
    // delete the cls from dynamic_loaded_cycle_classes_
    ClassCycleManager::DeleteDynamicLoadPattern(cls);
  }
  return true;
}
}
