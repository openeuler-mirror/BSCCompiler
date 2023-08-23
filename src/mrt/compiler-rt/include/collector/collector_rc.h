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
#ifndef MAPLE_RUNTIME_RCCOLLECTOR_H
#define MAPLE_RUNTIME_RCCOLLECTOR_H

#include "collector.h"

namespace maplert {
class RCCollector : public Collector {
 public:
  static void PrintStats();
  static void ResetStats();
  RCCollector() : Collector() {}
  virtual ~RCCollector() = default;
#if RC_PROFILE
  static std::atomic<uint64_t> numNativeInc;
  static std::atomic<uint64_t> numNativeDec;
  static std::atomic<uint64_t> numLoadIncRef;
  static std::atomic<uint64_t> numWriteRefVar;
  static std::atomic<uint64_t> numWriteRefField;
  static std::atomic<uint64_t> numReleaseRefVar;
  static std::atomic<uint64_t> numIncRef;
  static std::atomic<uint64_t> numDecRef;
  static std::atomic<uint64_t> numIncNull;
  static std::atomic<uint64_t> numDecNull;
#endif
};

class RCMutator : public Mutator {
 public:
  RCMutator() = default;
  virtual ~RCMutator() = default;
  void Fini() override;
  virtual void ReleaseObj(address_t obj) = 0; // Recurisvely dec and free objects
};

#if RC_HOT_OBJECT_DATA_COLLECT
void StatsFreeObject(address_t obj);
void DumpHotObj();
#else
#define StatsFreeObject(obj)
#define DumpHotObj()
#endif
} // namespace maplert

#endif
