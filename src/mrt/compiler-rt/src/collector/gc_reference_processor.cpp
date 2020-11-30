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
#include "collector/gc_reference_processor.h"
#include "yieldpoint.h"
#include "chosen.h"

namespace maplert {
GCReferenceProcessor::GCReferenceProcessor() : ReferenceProcessor() {
  for (uint32_t type = kRPWeakRef; type <= kRPPhantomRef; ++type) {
    refContext[type].discoverRefs = 0;
    refContext[type].enqueueRefs = 0;
    refContext[type].discoverCount = 0;
    refContext[type].enqueueCount = 0;
  }
}

bool GCReferenceProcessor::ShouldStartIteration() {
  if (processAllRefs.load()) {
    processAllRefs.store(false);
    return true;
  }
  if (hasBackgroundGC.load()) {
    return true;
  }
  for (uint32_t type = kRPWeakRef; type <= kRPPhantomRef; ++type) {
    if (refContext[type].enqueueRefs != 0) {
      return true;
    }
  }
  return false;
}

void GCReferenceProcessor::VisitGCRoots(RefVisitor &visitor) {
  ReferenceProcessor::VisitGCRoots(visitor);
  for (uint32_t type = kRPWeakRef; type <= kRPPhantomRef; ++type) {
    GCRefContext &ctx = refContext[type];
    if (ctx.enqueueRefs != 0) {
      visitor(ctx.enqueueRefs);
    }
  }
}

inline void SetNextReference(address_t next, address_t reference) {
  if (next == 0) {
    ReferenceSetPendingNext(reference, reference);
  } else {
    ReferenceSetPendingNext(reference, next);
  }
}

void GCReferenceProcessor::DiscoverReference(address_t reference) {
  MClass *klass = reinterpret_cast<MObject*>(reference)->GetClass();
  uint32_t classFlag = klass->GetFlag();
  uint32_t type = ReferenceProcessor::GetRPTypeByClassFlag(classFlag);
  GCRefContext &ctx = refContext[type];
  {
    LockGuard guard(ctx.discoverLock);
    if (ReferenceGetPendingNext(reference) != 0) { // avoid multiple threads racing discover
      return;
    }
    address_t head = ctx.discoverRefs;
    SetNextReference(head, reference);
    ctx.discoverRefs = reference;
    ++(ctx.discoverCount);
  }
}

// Concurrently remove reference with null or marked referent, save STW processing time
// 1. Lock discover ref and retrive whole list
// 2. start with prev and cur reference, always record the head and tail
// 3. Iterate and remove reference whose referent is null or marked
// 4. Merge discovered list back to context
void GCReferenceProcessor::ConcurrentProcessDisovered() {
  Collector &collector = Collector::Instance();
  __MRT_ASSERT(collector.IsConcurrentMarkRunning(), "Not In Concurrent marking");
  for (uint32_t type = kRPWeakRef; type <= kRPPhantomRef; ++type) {
    GCRefContext &ctx = refContext[type];
    address_t prev = 0;
    address_t cur;
    address_t head;
    uint32_t removedCount = 0;
    {
      LockGuard guard(ctx.discoverLock);
      head = ctx.discoverRefs;
      cur = ctx.discoverRefs;
      ctx.discoverRefs = 0;
    }
    while (cur != 0) {
      address_t referent = ReferenceGetReferent(cur);
      address_t next = ReferenceGetPendingNext(cur);
      if (next == cur) {
        next = 0;
      }
      if (referent == 0 || !collector.IsGarbage(referent)) {
        ++removedCount;
        if (prev == 0) {
          head = next;
        } else {
          SetNextReference(next, prev);
        }
        ReferenceSetPendingNext(cur, 0);
      } else {
        prev = cur;
      }
      if (next == 0) {
        break;
      }
      cur = next;
    }
    if (head != 0) {
      LockGuard guard(ctx.discoverLock);
      address_t curHead = ctx.discoverRefs;
      ctx.discoverRefs = head;
      if (curHead != 0) {
        ReferenceSetPendingNext(cur, curHead);
      }
    }
    if (removedCount > 0) {
      LOG2FILE(kLogtypeGc) << "ConcurrentProcessDisovered clear " << type << " " << removedCount << std::endl;
    }
  }
}

void GCReferenceProcessor::InitEnqueueAtFork(uint32_t type, address_t refs) {
  __MRT_ASSERT(type <= kRPPhantomRef, "Invalid type");
  GCRefContext &ctx = refContext[type];
  __MRT_ASSERT(ctx.enqueueRefs == 0, "not zero at fork");
  ctx.enqueueRefs = refs;
}

// Iterate discoverd Reference:
// 1. check if it can be enqueued, referent is not null and dead
// 2. If enqueuable, add into enqueueRefs
// 3. Otherwise, clear pending next
// 4. clear discoveredref
void GCReferenceProcessor::ProcessDiscoveredReference(uint32_t flags) {
  __MRT_ASSERT(WorldStopped(), "Not In STW");
  for (uint32_t type = kRPWeakRef; type <= kRPPhantomRef; type++) {
    if (!(flags & RPMask(type))) {
      continue;
    }
    GCRefContext &ctx = refContext[type];
    address_t reference = ctx.discoverRefs;
    while (reference != 0) {
      address_t referent = ReferenceGetReferent(reference);
      address_t next = ReferenceGetPendingNext(reference);
      if (next == reference) {
        next = 0;
      }
      // must be in heap or cleared during concurrent mark
      if (referent != 0 && Collector::Instance().IsGarbage(referent)) {
        ReferenceClearReferent(reference);
        SetNextReference(ctx.enqueueRefs, reference);
        ctx.enqueueRefs = reference;
        ++(ctx.enqueueCount);
      } else {
        ReferenceSetPendingNext(reference, 0);
      }
      reference = next;
    }
    ctx.discoverRefs = 0;
    ctx.discoverCount = 0;
  }
}

// Iterate enqueued Reference:
// 1. Leave Safe Region, avoid GC STW modify enqueueRefs
// 2. Check if available enqueueRefs, break if empty
// 3. Get Reference to proceess and update enqueueRefs
// 4. Invoke Enqueu method
void GCReferenceProcessor::EnqeueReferences() {
  for (uint32_t type = kRPWeakRef; type <= kRPPhantomRef; ++type) {
    GCRefContext &ctx = refContext[type];
    // iterate all reference and invoke enqueue method, might racing with GC STW phase
    while (true) {
      ScopedObjectAccess soa; // leave safe region to sync with GC
      if (ctx.enqueueRefs == 0) {
        break;
      }
      ScopedHandles sHandles;
      ObjHandle<MObject, false> reference(ctx.enqueueRefs);
      address_t next = ReferenceGetPendingNext(reference.AsRaw());
      __MRT_ASSERT(next != 0 && ReferenceGetReferent(reference.AsRaw()) == 0, "Invalid pending enqueue reference");
      if (next == reference.AsRaw()) {
        next = 0;
      } else {
        TLMutator().SatbWriteBarrier(next);
      }
      ReferenceSetPendingNext(reference.AsRaw(), reference.AsRaw());
      --(ctx.enqueueCount);
      ctx.enqueueRefs = next;
      Enqueue(reference.AsRaw());
    }
  }
}

void GCReferenceProcessor::LogRefProcessorBegin() {
  if (!GCLog().IsWriteToFile(kLogtypeRp)) {
    return;
  }
  timeCurrentRefProcessBegin = timeutils::MicroSeconds();
  LOG2FILE(kLogtypeRp) << "[RefProcessor] Begin (" << timeutils::GetDigitDate() << ") Soft: " <<
      refContext[kRPSoftRef].enqueueCount <<
      " Weak " << refContext[kRPWeakRef].enqueueCount <<
      " Phantom " << refContext[kRPPhantomRef].enqueueCount << std::endl;
}

void GCReferenceProcessor::LogRefProcessorEnd() {
  if (!GCLog().IsWriteToFile(kLogtypeRp)) {
    return;
  }
  uint64_t timeNow = timeutils::MicroSeconds();
  uint64_t timeConsumed = timeNow - timeCurrentRefProcessBegin;
  uint64_t totalTimePassed = timeNow - timeRefProcessorBegin;
  timeRefProcessUsed += timeConsumed;
  float percentage = ((maple::kTimeFactor * timeRefProcessUsed) / totalTimePassed) / kPercentageDivend;
  LOG2FILE(kLogtypeRp) << "[RefProcessor] End " << " (" << timeConsumed << "us" <<
      " [" << timeRefProcessUsed << "us]" <<
      " [ " << percentage << "%]" << std::endl;
}

void GCReferenceProcessor::PreAddFinalizables(address_t[], uint32_t, bool needLock) {
  if (needLock) {
    finalizersLock.lock();
  }
}

void GCReferenceProcessor::PostAddFinalizables(address_t[], uint32_t, bool needLock) {
  if (needLock) {
    finalizersLock.unlock();
  }
}
}
