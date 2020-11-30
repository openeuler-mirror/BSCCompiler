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
#ifndef MAPLE_RUNTIME_CHOSEN_H
#define MAPLE_RUNTIME_CHOSEN_H

#include "mm_config.h"
#include "deps.h"
// This module chooses the desired allocator and collector instances according
// to the GC strategy configured using preprocessor macros.
// The chosen strategy class inherits from the implementation, not the other way around.
// This avoids virtual dispatch, and is what's done in MMTk.
#if MRT_ALLOCATOR == MRT_ALLOCATOR_ROS
namespace maplert {
class RosAllocImpl;
template<uint32_t kLocalRuns>
class ROSAllocMutator;
// this number equals kROSAllocLocalRuns; because of forward declaration, we need to explicitly
// write it here; get rid of forward declaration in the future
using RosBasedMutator = ROSAllocMutator<12>;
using TheAllocator = RosAllocImpl;
using TheAllocMutator = RosBasedMutator;
}
#else
#error "Invalid MRT_ALLOCATOR"
#endif

// We don't provide an empty collect/mutator. So when MRT_COLLECTOR_NONE is defined,
// using NaiveRC as the default ones, but disable its effect in runtime
#if ((MRT_COLLECTOR == MRT_COLLECTOR_NONE) || (MRT_COLLECTOR == MRT_COLLECTOR_NAIVERC))
namespace maplert {
class NaiveRCCollector;
class NaiveRCMutator;
using TheCollector = NaiveRCCollector;
using TheMutator = NaiveRCMutator;
}
#elif MRT_COLLECTOR == MRT_COLLECTOR_MS
namespace maplert {
class MarkSweepCollector;
class MarkSweepMutator;
using TheCollector = MarkSweepCollector;
using TheMutator = MarkSweepMutator;
}
#else
#error "Invalid MRT_COLLECTOR"
#endif

#include "tls_store.h"
namespace maplert {
class BumpPointerAlloc;
class DecoupleAllocator;
class MetaAllocator;
class ZterpStaticRootAllocator;
extern ImmortalWrapper<BumpPointerAlloc> permAllocator;
extern ImmortalWrapper<BumpPointerAlloc> zterpMetaAllocator;
extern ImmortalWrapper<MetaAllocator> metaAllocator;
extern ImmortalWrapper<DecoupleAllocator> decoupleAllocator;
extern ImmortalWrapper<ZterpStaticRootAllocator> zterpStaticRootAllocator;
extern ImmortalWrapper<TheAllocator> theAllocator;

class Mutator;

static inline Mutator &TLMutator(void) noexcept {
  // tl_the_mutator is added to thread context in GCInitThreadLocal()
  return *reinterpret_cast<Mutator*>(maple::tls::GetTLS(maple::tls::kSlotMutator));
}

static inline Mutator *TLMutatorPtr(void) {
  if (maple::tls::HasTLS()) {
    // tl_the_mutator is added to thread context in GCInitThreadLocal()
    return reinterpret_cast<Mutator*>(maple::tls::GetTLS(maple::tls::kSlotMutator));
  }
  // CreateTLS() not called on current thread.
  return nullptr;
}

static inline TheAllocMutator &TLAllocMutator(void) {
  // tl_alloc_mutator is added to thread context in GCInitThreadLocal()
  return *reinterpret_cast<TheAllocMutator*>(maple::tls::GetTLS(maple::tls::kSlotAllocMutator));
}

static inline TheAllocMutator *TLAllocMutatorPtr(void) {
  if (maple::tls::HasTLS()) {
    // tl_alloc_mutator is added to thread context in GCInitThreadLocal()
    return reinterpret_cast<TheAllocMutator*>(maple::tls::GetTLS(maple::tls::kSlotAllocMutator));
  }
  // CreateTLS() not called on current thread.
  return nullptr;
}
} // namespace maplert

// we forward declared the classes above, then include the headers these classes depend on,
// so that the code (especially inline functions) in these headers can at least enjoy the benefits
// of knowing what the chosen collector/allocator/mutator are, as well as the mutator getters.
//
// if any of these headers uses the above information, it needs to include chosen.h to be
// self-contained. but we won't do that because usually chosen.h comes at the very beginning of a
// compilation unit, and it's intended to be used in place of collector/allocator headers.
#include "allocator/bp_allocator.h"
#if MRT_ALLOCATOR == MRT_ALLOCATOR_ROS
#include "allocator/ros_allocator.h"
#endif
#include "collector/collector_naiverc.h"
#if ((MRT_COLLECTOR == MRT_COLLECTOR_NONE) || (MRT_COLLECTOR == MRT_COLLECTOR_NAIVERC))
#elif MRT_COLLECTOR == MRT_COLLECTOR_MS
#include "collector/collector_ms.h"
#endif
#endif // MAPLE_RUNTIME_CHOSEN_H
