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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ISOLATE_FASTPATH_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ISOLATE_FASTPATH_H

#include "isolate_fastpath.h"
#include "aarch64_cgfunc.h"
#include "aarch64_operand.h"
#include "aarch64_insn.h"

namespace maplebe {
using namespace maple;

class AArch64IsolateFastPath : public IsolateFastPath {
 public:
  explicit AArch64IsolateFastPath(CGFunc &func)
      : IsolateFastPath(func) {}
  ~AArch64IsolateFastPath() override = default;

  void Run() override;

 private:
  bool InsertOpndRegs(Operand &op, std::set<regno_t> &vecRegs) const;
  bool InsertInsnRegs(Insn &insn, bool insertSource, std::set<regno_t> &vecSourceRegs,
                      bool insertTarget, std::set<regno_t> &vecTargetRegs) const;
  bool FindRegs(Operand &op, std::set<regno_t> &vecRegs) const;
  bool BackwardFindDependency(BB &ifbb, std::set<regno_t> &vecReturnSourceRegs, std::list<Insn*> &existingInsns,
                              std::list<Insn*> &moveInsns) const;
  void IsolateFastPathOpt();

  void SetFastPathReturnBB(BB *bb) {
    bb->SetFastPathReturn(true);
    fastPathReturnBB = bb;
  }
  BB *GetFastPathReturnBB() {
    return fastPathReturnBB;
  }

  BB *fastPathReturnBB = nullptr;
};
}  /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ISOLATE_FASTPATH_H */
