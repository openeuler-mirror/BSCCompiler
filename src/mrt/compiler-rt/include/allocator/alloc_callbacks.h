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
#ifndef MAPLE_RUNTIME_ALLOC_CALLBACKS_H
#define MAPLE_RUNTIME_ALLOC_CALLBACKS_H

#include <fstream>
#include <string>
#include <dlfcn.h>
#include <unistd.h>

#include "allocator.h"
#include "chosen.h"
#include "exception/mrt_exception.h"
#include "mrt_reflection.h"
#include "mrt_object.h"
#include "cpphelper.h"
#include "chelper.h"
#include "mm_config.h"
#include "address.h"
#include "sizes.h"
#include "panic.h"
#include "collector/stats.h"
#include "jsan.h"

namespace maplert {
inline void Allocator::PreObjAlloc(address_t objAddress __attribute__((unused)),
                                   size_t objSize __attribute__((unused))) const {}

inline void Allocator::PostObjAlloc(address_t objAddress, size_t objSize, size_t internalSize) {
  JSAN_ADD_OBJ(objAddress, objSize);

  bool isLarge = internalSize > RosAllocImpl::kLargeObjSize;
  // non-atomic: per-mutator allocation statistics (currently not instrumented)
  // atomic: keep track of total bytes allocated in the heap
  account.AtAlloc(objSize, internalSize, isLarge);

  if (UNLIKELY(mAllocRecordingCallbackFunc != nullptr)) {
    mAllocRecordingCallbackFunc(objAddress, objSize);
  }
}

template<bool isFast>
inline size_t Allocator::PreObjFree(address_t objAddress) const {
  if (UNLIKELY(mAllocRecordingCallbackFunc != nullptr)) {
    mAllocRecordingCallbackFunc(objAddress, 0);
  }

  if (isFast) {
    return 0;
  } else {
    MObject *freeObj = MObject::Cast<MObject>(objAddress);
    return freeObj->GetSize();
  }
}

template<bool isFast>
inline void Allocator::PostObjFree(address_t, size_t objSize, size_t internalSize) {
#if ALLOC_USE_FAST_PATH
  static_cast<void>(objSize);
  if (!isFast) {
    FAST_ALLOC_ACCOUNT_SUB(internalSize);
  }
#else
  static_cast<void>(isFast);
  bool isLarge = internalSize > RosAllocImpl::kLargeObjSize;
  // non-atomic: per-mutator allocation statistics (currently not instrumented)
  // atomic: keep track of total bytes allocated in the heap
  account.AtFree(objSize, internalSize, isLarge);
#endif
}

#if LOG_ALLOC_TIMESTAT
#define PrintTimeStat(outs, op, typeInd, mut) \
  if (mut.GetTimerCnt(typeInd) != 0) {  \
    outs << "         " << op << "[Min] : " << mut.GetTimerMin(typeInd) << maple::endl;  \
    outs << "         " << op << "[Max] : " << mut.GetTimerMax(typeInd) << maple::endl;  \
    outs << "         " << op << "[Avg] : " << mut.GetTimerAvg(typeInd) <<  \
        " out of " << mut.GetTimerCnt(typeInd) << maple::endl ;  \
  }
#endif

void Allocator::PostMutatorFini(AllocMutator &mutator) {
#if LOG_ALLOC_TIMESTAT
  std::ostringstream os1;
  os1 << "[Mutator Allocation Time] : " << &mutator << maple::endl;
  PrintTimeStat(os1, "[local]", kTimeAllocLocal, mutator);
  PrintTimeStat(os1, "[global]", kTimeAllocGlobal, mutator);
  PrintTimeStat(os1, "[large]", kTimeAllocLarge, mutator);
  PrintTimeStat(os1, "[release]", kTimeReleaseObj, mutator);
  PrintTimeStat(os1, "[free local]", kTimeFreeLocal, mutator);
  PrintTimeStat(os1, "[free global]", kTimeFreeGlobal, mutator);
  PrintTimeStat(os1, "[free large]", kTimeFreeLarge, mutator);
  LOG(ERROR) << os1.str().c_str() << maple::endl;
#else
  static_cast<void>(mutator);
#endif
}
} // namespace maplert

#endif // MAPLE_RUNTIME_ALLOC_CALLBACKS_H
