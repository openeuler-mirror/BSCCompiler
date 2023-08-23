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
#include "collector/collector.h"
#include "collector/rp_base.h"
#include "chosen.h"
#include "mutator_list.h"

namespace maplert {
namespace {
const std::string kCollectorName[] = {
    "NoCollector", "NaiveRC", "MarkSweep", "NaiveRCMarkSweep"
};
}

Collector *Collector::instance = nullptr;
RegisteredCollectorTypeCheckAddrs *RegisteredCollectorTypeCheckAddrs::instance =
    new (std::nothrow) RegisteredCollectorTypeCheckAddrs();

void RegisteredCollectorTypeCheckAddrs::Register(uint64_t *addr) {
  if (addr == nullptr) {
    LOG(FATAL) << "Register get a null pointer as parameter" << maple::endl;
  }
  std::lock_guard<std::mutex> lock(registerLock);
  addrs.push_back(addr);
  if (Collector::InstancePtr() != nullptr) {
    bool isGCOnly = Collector::Instance().Type() != kNaiveRC;
    if (isGCOnly) {
      __MRT_ASSERT(*(addr + 1) == kRegistedMagicNumber, "Invalid number");
      *addr = 1;
    }
  }
}

void RegisteredCollectorTypeCheckAddrs::PostCollectorCreate() {
  std::lock_guard<std::mutex> lock(registerLock);
  bool isGCOnly = Collector::Instance().Type() != kNaiveRC;
  if (isGCOnly) {
    LOG(INFO) << "RegisterRCCheckAddr switch to GC " << addrs.size();
    for (uint64_t *addr : addrs) {
      __MRT_ASSERT(*(addr + 1) == kRegistedMagicNumber, "Invalid number");
      *addr = 1;
    }
  }
}

Collector::Collector()
    : processState(kProcessStateJankPerceptible) {}

void Collector::Create(bool gcOnly) {
  bool createNaivRCCollector = true;
#if ((MRT_COLLECTOR == MRT_COLLECTOR_NONE) || (MRT_COLLECTOR == MRT_COLLECTOR_NAIVERC))
  createNaivRCCollector = !VLOG_IS_ON(gconly) && !gcOnly;
#elif (MRT_COLLECTOR == MRT_COLLECTOR_MS)
  UNUSED(gcOnly);
  createNaivRCCollector = false;
#else
#error "Invalid MRT_COLLECTOR"
#endif
  if (createNaivRCCollector) {
    instance = new (std::nothrow) NaiveRCCollector();
    FastAllocData::data.isGCOnly = false;
  } else {
    instance = new (std::nothrow) MarkSweepCollector();
    FastAllocData::data.isGCOnly = true;
  }
  if (UNLIKELY(instance == nullptr)) {
    LOG(FATAL) << "Create Collector instance failed!" << maple::endl;
  }
  RegisteredCollectorTypeCheckAddrs::Instance().PostCollectorCreate();
}

void Collector::SwitchToGCOnFork() {
  __MRT_ASSERT(instance != nullptr, "null collector");
  __MRT_ASSERT(instance->Type() == kNaiveRC, "not RC collector");
  // update all object's RC to overflow, avoid RC check
  __MRT_ASSERT((static_cast<NaiveRCCollector*>(instance)->HasWeakRelease()) == false, "has weak field");
  // switch reference processor
  ReferenceProcessor::SwitchToGCOnFork();
  // switch collector
  __MRT_ASSERT(instance->IsSystem() == false, "switch to gc for SS");
  instance->Fini();
  delete instance;
  instance = new (std::nothrow) MarkSweepCollector();
  FastAllocData::data.isGCOnly = true;
  if (instance == nullptr) {
    LOG(FATAL) << "create new collector fail" << maple::endl;
  }
  RegisteredCollectorTypeCheckAddrs::Instance().PostCollectorCreate();
  instance->SetIsSystem(false);
  instance->InitAfterFork();
  MRT_SendBackgroundGcJob(true);
  // switch mutator
  __MRT_ASSERT(MutatorList::Instance().Size() == 1, "only main thread allowed");
  NaiveRCMutator &rcMutator = NRCMutator();
  Mutator *gcMutator = new (std::nothrow) MarkSweepMutator(*instance);
  if (gcMutator == nullptr) {
    LOG(FATAL) << "create new mutator fail" << maple::endl;
  }
  gcMutator->CopyStateOnFork(rcMutator);
  gcMutator->InitAfterFork();
  // close rc mutator
  MutatorList::Instance().RemoveMutator(rcMutator, [](Mutator *mut) {
    if (mut == nullptr) {
      LOG(FATAL) << "Wrong mutator pointer" << maple::endl;
    }
    mut->Fini();
  });
  delete &rcMutator;
  // add new gc mutator
  MutatorList::Instance().AddMutator(*gcMutator);
  maple::tls::StoreTLS(gcMutator, maple::tls::kSlotMutator);
  std::atomic_thread_fence(std::memory_order_release);
}

void Collector::UpdateProcessState(ProcessState processStateVal, bool isSystemServer) {
  // ensure the new state is different from the old state
  if (processStateVal == processState) {
    return;
  }
  if ((processStateVal == kProcessStateJankImperceptible) && !isSystemServer) {
    LOG(INFO) << "UpdateProcessState to JankImperceptible triggers GC" << maple::endl;
    MRT_SendBackgroundGcJob(false);
  }
  processState = processStateVal;
}

const std::string &Collector::GetName() {
  return kCollectorName[type];
}

void Collector::Init() {
  ReferenceProcessor::Create(type);
  stats::gcStats->OnCollectorInit();
}

void Mutator::InitTid() {
  tid = static_cast<uint32_t>(maple::GetTid());
  if (UNLIKELY(tid == 0)) {
    LOG(FATAL) << "Mutator::InitTid(): invalid tid = 0" << maple::endl;
  }
}

void Mutator::DebugShow() const {
  fprintf(stderr, "Mutator: %p", this);
  fprintf(stderr, "  tid = %" PRIu32, GetTid());
  fprintf(stderr, "  active = %s", IsActive() ? "true" : "false");
  fprintf(stderr, "  in_saferegion = %s", InSaferegion() ? "true" : "false");
  fprintf(stderr, "  stack: %p to %p", reinterpret_cast<void*>(stackBegin), reinterpret_cast<void*>(stackEnd));
}

// Copy state: active, in safe region and stack begin
// Copy last java context
// Copy LocalValueArena and force current LocalValueArena skip desctructor
void Mutator::CopyStateOnFork(Mutator &orig) {
  __MRT_ASSERT(orig.active == kMutatorDebugTrue, "Base not active");
  active = orig.active;
  __MRT_ASSERT(orig.inSaferegion == kMutatorDebugTrue, "Base in saferegion");
  inSaferegion = orig.inSaferegion;
  stackBegin = orig.stackBegin;
  initialUnwindContext.CopyFrom(orig.initialUnwindContext);
  arena = orig.arena;
  orig.arena.Clear();
}

void Mutator::SatbWriteBarrier(address_t obj, const reffield_t &field) {
  if (!concurrentMarking) {
    return;
  }
  reffield_t ref = field;
  if (UNLIKELY((ref & LOAD_INC_RC_MASK) != 0)) {
    ref = Collector::Instance().RefFieldLoadBarrier(obj, field);
  }
  PushIntoSatbBuffer(ref);
}

void Mutator::PushChildrenToSatbBuffer(address_t obj) {
  if (!IS_HEAP_ADDR(obj)) {
    return;
  }
  auto func = [&](const reffield_t &field, uint64_t) {
    reffield_t ref = field;
    if (UNLIKELY((ref & LOAD_INC_RC_MASK) != 0)) {
      ref = Collector::Instance().RefFieldLoadBarrier(obj, field);
    }
    PushIntoSatbBuffer(ref);
  };
  DoForEachRefField(obj, func);
}

// satb buffer
SatbBuffer &SatbBuffer::Instance() {
  static ImmortalWrapper<SatbBuffer> instance;
  return *instance;
}

struct GCTibGCInfo MCC_GCTIB___EmptyObject = { .headerProto = 0, .nBitmapWords = 0 };
struct GCTibGCInfo MCC_GCTIB___ArrayOfObject = { .headerProto = kHasChildRef | kArrayBit, .nBitmapWords = 0 };
struct GCTibGCInfo MCC_GCTIB___ArrayOfPrimitive = { .headerProto = kArrayBit, .nBitmapWords = 0 };
} // namespace maplert.
