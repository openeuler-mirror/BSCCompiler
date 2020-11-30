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
#include "collector/collector_rc.h"

#include "chosen.h"
#include "mutator_list.h"

namespace maplert {
#if RC_PROFILE
std::atomic<uint64_t> RCCollector::numLoadIncRef(0);
std::atomic<uint64_t> RCCollector::numWriteRefVar(0);
std::atomic<uint64_t> RCCollector::numWriteRefField(0);
std::atomic<uint64_t> RCCollector::numReleaseRefVar(0);
std::atomic<uint64_t> RCCollector::numNativeInc(0);
std::atomic<uint64_t> RCCollector::numNativeDec(0);
std::atomic<uint64_t> RCCollector::numIncRef(0);
std::atomic<uint64_t> RCCollector::numDecRef(0);
std::atomic<uint64_t> RCCollector::numIncNull(0);
std::atomic<uint64_t> RCCollector::numDecNull(0);

void RCCollector::PrintStats() {
  LOG2FILE(kLogtypeGc) << "[RCSTATS] RC statistics:" << std::endl;
  LOG2FILE(kLogtypeGc) << "[RCSTATS] # LoadIncRefVar   : " << RCCollector::numLoadIncRef.load() << std::endl;
  LOG2FILE(kLogtypeGc) << "[RCSTATS] # WriteRefVar   : " << RCCollector::numWriteRefVar.load() << std::endl;
  LOG2FILE(kLogtypeGc) << "[RCSTATS] # WriteRefField : " << RCCollector::numWriteRefField.load() << std::endl;
  LOG2FILE(kLogtypeGc) << "[RCSTATS] # ReleaseRefVar : " << RCCollector::numReleaseRefVar.load() << std::endl;
  LOG2FILE(kLogtypeGc) << "[RCSTATS] # IncRef  : " << RCCollector::numIncRef.load() << std::endl;
  LOG2FILE(kLogtypeGc) << "[RCSTATS] # IncNull : " << RCCollector::numIncNull.load() << std::endl;
  LOG2FILE(kLogtypeGc) << "[RCSTATS] # DecRef  : " << RCCollector::numDecRef.load() << std::endl;
  LOG2FILE(kLogtypeGc) << "[RCSTATS] # DecNull : " << RCCollector::numDecNull.load() << std::endl;
  LOG2FILE(kLogtypeGc) << "[RCSTATS] # NativeInc : " << RCCollector::numNativeInc.load() << std::endl;
  LOG2FILE(kLogtypeGc) << "[RCSTATS] # NativeDec : " << RCCollector::numNativeDec.load() << std::endl;
}

void RCCollector::ResetStats() {
  RCCollector::numLoadIncRef = 0;
  RCCollector::numWriteRefVar = 0;
  RCCollector::numWriteRefField = 0;
  RCCollector::numReleaseRefVar = 0;
  RCCollector::numIncRef = 0;
  RCCollector::numDecRef = 0;
  RCCollector::numIncNull = 0;
  RCCollector::numDecNull = 0;
  RCCollector::numNativeInc = 0;
  RCCollector::numNativeDec = 0;
}

#else

void RCCollector::PrintStats() {
  LOG2FILE(kLogtypeGc) << "[RCSTATS] RC statistics is disabled." << std::endl;
}

void RCCollector::ResetStats() {}

#endif // RC_PROFILE

void RCMutator::Fini() {
  Mutator::Fini();
}
}
