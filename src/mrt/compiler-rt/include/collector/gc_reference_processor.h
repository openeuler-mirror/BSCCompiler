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
#ifndef MAPLE_COMPILER_RT_GC_REFERENCE_PROCESSOR_H
#define MAPLE_COMPILER_RT_GC_REFERENCE_PROCESSOR_H

#include <mutex>
#include "address.h"
#include "sizes.h"
#include "rp_base.h"

namespace maplert {
struct GCRefContext {
  std::mutex discoverLock;
  address_t discoverRefs; // Discover list
  address_t enqueueRefs; // enqueue list
  uint32_t discoverCount;
  uint32_t enqueueCount;
};

class GCReferenceProcessor : public ReferenceProcessor {
 public:
  static inline GCReferenceProcessor &Instance() {
    return static_cast<GCReferenceProcessor&>(ReferenceProcessor::Instance());
  }
  GCReferenceProcessor();
  ~GCReferenceProcessor() = default;
  void DiscoverReference(address_t reference);
  void ProcessDiscoveredReference(uint32_t flags);
  void VisitGCRoots(RefVisitor &visitor) override;
  void ConcurrentProcessDisovered();
  void InitEnqueueAtFork(uint32_t type, address_t refs);
 protected:
  bool ShouldStartIteration() override;
  void LogRefProcessorBegin() override;
  void LogRefProcessorEnd() override;
 private:
  void PreAddFinalizables(address_t obj[], uint32_t count, bool needLock) override;
  void PostAddFinalizables(address_t obj[], uint32_t count, bool needLock) override;
  void EnqeueReferences() override;
  GCRefContext refContext[kRPPhantomRef + 1]; // weak/soft/phantom has index 0,1,2
};
}
#endif // MAPLE_COMPILER_RT_REFERENCE_PROCESSOR_H
