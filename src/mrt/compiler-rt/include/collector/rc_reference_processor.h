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
#ifndef MAPLE_COMPILER_RT_RC_REFERENCE_PROCESSOR_H
#define MAPLE_COMPILER_RT_RC_REFERENCE_PROCESSOR_H

#include <vector>
#include <mutex>
#include "mm_config.h"
#include "address.h"
#include "sizes.h"
#include "mrt_reference_api.h"
#include "mrt_well_known.h"
#include "rp_base.h"

namespace maplert {
class RCReferenceProcessor : public ReferenceProcessor {
 public:
  static inline RCReferenceProcessor &Instance() {
    return static_cast<RCReferenceProcessor&>(ReferenceProcessor::Instance());
  }
  RCReferenceProcessor();
  ~RCReferenceProcessor() = default;
  void Init() override;
  void Fini() override;
  void VisitFinalizers(RefVisitor &visitor) override;

  // async release queue
  void AddAsyncReleaseObj(address_t obj, bool isMutator);
  void ClearAsyncReleaseObjs();
  void ProcessAsyncReleaseObjs();
  void VisitAsyncReleaseObjs(const RefVisitor &vistor);
  // stats
  inline void CountRecent(uint32_t type) {
    ++(recentCount[type]);
  }
  inline uint32_t RecentCount(uint32_t type) {
    return recentCount[type];
  }
  inline void AddAgedProcessedCount() {
    ++(numProcessedAgedRefs[curProcessingRef]);
  }
  bool CheckAndSetReferenceFlags();
  bool CheckAndUpdateAgedReference(uint32_t type); // check if perfrom aged reference process after young
  bool CheckAndAddFinalizable(address_t obj);
  void TransferFinalizblesOnFork(ManagedList<address_t> &toFinalizables);
  address_t TransferEnquenenableReferenceOnFork(uint32_t type);
 protected:
  bool ShouldStartIteration() override;
  void LogRefProcessorBegin() override;
  void LogRefProcessorEnd() override;
 private:
  void PreIteration() override;
  void PostIteration() override;
  void PreExitDoFinalize() override;
  void PostProcessFinalizables() override;
  void PostFinalizable(address_t obj) override;
  void PreAddFinalizables(address_t obj[], uint32_t count, bool needLock) override;
  void PostAddFinalizables(address_t obj[], uint32_t count, bool needLock) override;
  bool SpecializedAddFinalizable(address_t obj) override;
  void EnqeueReferences() override;
  // RP check and start
  inline bool HasReferenceFlag(uint32_t type) {
    return referenceFlags & RPMask(type);
  }

  // finalization
  ManagedList<address_t> pendingFinalizables; // pending list for RC, workaround for too fast finalize
  ManagedList<address_t> pendingFinalizablesPrev; // Prev to record next list for processing
  // release queue
  std::mutex releaseQueueLock;
  ManagedDeque<address_t> asyncReleaseQueue; // release queue for async release
  ManagedDeque<address_t> workingAsyncReleaseQueue; // working async release queue
  // reference processing flags
  uint32_t referenceFlags; // which references to process
  uint32_t agedReferenceFlags; // which aged references to process
  uint32_t numProcessedAgedRefs[kRPTypeNum]; // aged reference processed from begining
  uint32_t numLastProcessedAgedRefs[kRPTypeNum];
  volatile uint32_t recentCount[kRPTypeNum]; // recent reference count added to list
  uint32_t agedReferenceCount[kRPTypeNum]; // how many iterations aged is skipped
  uint32_t hungryCount; // how many iteration not process RP
  uint32_t agedHungryCount; // how many iteration not process aged RP
};


// add a (newly created) reference object for later processing
void AddNewReference(address_t obj, uint32_t classFlag);
void MrtVisitReferenceRoots(AddressVisitor visitor, uint32_t flags);

// used by gc thread for visit and process references in non-parallel or parallel mode.
void MRT_GCVisitReferenceRoots(RefVisitor &visitor, uint32_t flags);
void MRT_ParallelVisitReferenceRoots(MplThreadPool &threadPool, RefVisitor &visitor, uint32_t flags);

uint32_t MrtRpEpochIntervalMs();
}
#endif // MAPLE_COMPILER_RT_REFERENCE_PROCESSOR_H
