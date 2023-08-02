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
#include "collector/rp_base.h"

namespace maplert {
address_t MRT_ReferenceGetReferent(address_t javaThis) {
  if (javaThis != 0) {
    SetReferenceActive(javaThis);
  }
  address_t referent = MRT_LOAD_JOBJECT_INC_REFERENT(javaThis, WellKnown::kReferenceReferentOffset);
  if (referent != 0) {
    // Apply barrier to ensure that concurrent marking sees the referent and
    // does not prematurely reclaim it.
    MRT_WeakRefGetBarrier(referent);
  }  else {
    ClearReferenceActive(javaThis);
  }
  return referent;
}

void MRT_ReferenceClearReferent(address_t javaThis) {
  bool clearResurrectWeak = false;
  if (javaThis != 0) {
    MClass *klass = reinterpret_cast<MObject*>(javaThis)->GetClass();
    uint32_t classFlag = klass->GetFlag();
    clearResurrectWeak = ((classFlag & (modifier::kClassCleaner | modifier::kClassPhantomReference)) == 0);
  }
  MRT_WRITE_REFERENT(javaThis, WellKnown::kReferenceReferentOffset, nullptr, clearResurrectWeak);
}


// Only one thread can invoke MRT_RunFinalization at same time
// 1. Only one runFinalizationFinalizers list in system.
// 2. Swap runFinalizationFinalizers and finalizers list two time, might cause same object finalize two times.
void MCC_RunFinalization() __attribute__((alias("MRT_RunFinalization")));
void MRT_RunFinalization() {
  ReferenceProcessor::Instance().RunFinalization();
}
} // namespace maplert
