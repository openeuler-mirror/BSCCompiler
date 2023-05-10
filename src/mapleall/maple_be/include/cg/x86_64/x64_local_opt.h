/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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

#ifndef MAPLEBE_INCLUDE_X64_LOCALO_H
#define MAPLEBE_INCLUDE_X64_LOCALO_H

#include "local_opt.h"
namespace maplebe {
class X64LocalOpt : public LocalOpt {
 public:
  X64LocalOpt(MemPool &memPool, CGFunc &func, ReachingDefinition &rd)
      : LocalOpt(memPool, func, rd) {}
  ~X64LocalOpt() = default;
 private:
  void DoLocalCopyProp() override;
};

class CopyRegProp : public LocalPropOptimizePattern {
 public:
  CopyRegProp(CGFunc &cgFunc, ReachingDefinition &rd) : LocalPropOptimizePattern(cgFunc, rd) {}
  ~CopyRegProp() override = default;
  bool CheckCondition(Insn &insn) final;
  void Optimize(BB &bb, Insn &insn) final;
 private:
  bool propagateOperand(Insn &insn, RegOperand& oldOpnd, RegOperand& replaceOpnd);
};

class X64RedundantDefRemove : public RedundantDefRemove {
 public:
  X64RedundantDefRemove(CGFunc &cgFunc, ReachingDefinition &rd) : RedundantDefRemove(cgFunc, rd) {}
  ~X64RedundantDefRemove() override = default;
  void Optimize(BB &bb, Insn &insn) final;
};

}

#endif /* MAPLEBE_INCLUDE_X64_LOCALO_H */
