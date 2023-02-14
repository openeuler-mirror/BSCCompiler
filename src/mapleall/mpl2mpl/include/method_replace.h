/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEIPA_INCLUDE_METHODREPLACE_H
#define MAPLEIPA_INCLUDE_METHODREPLACE_H
#include "mir_module.h"
#include "mir_function.h"
#include "mir_builder.h"
#include "mempool.h"
#include "mempool_allocator.h"
#include "maple_phase.h"

namespace maple {
using StrStrMap = std::unordered_map<std::string, std::string>;
using ReplaceMethodMapType = std::unordered_map<std::string, StrStrMap>;

class MethodReplace : public AnalysisResult {
 public:
  MethodReplace(MIRModule *mod, MemPool *mp, MIRBuilder &builder)
      : AnalysisResult(mp), mirModule(mod), allocator(mp), mBuilder(builder) {}

  ~MethodReplace() override = default;

  void DoMethodReplace();
  void Init();

 private:
  MIRModule *mirModule;
  MapleAllocator allocator;
  MIRBuilder &mBuilder;
  ReplaceMethodMapType replaceMethodMap;

  void InsertRecord(const std::string &caller, const std::string &origCallee, const std::string &newCallee);
  void FindCalleeAndReplace(MIRFunction &callerFunc, StrStrMap &calleeMap) const;
};

MAPLE_MODULE_PHASE_DECLARE(M2MMethodReplace)
}  // namespace maple
#endif