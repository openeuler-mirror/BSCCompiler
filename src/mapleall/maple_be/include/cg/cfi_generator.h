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
#ifndef MAPLEBE_INCLUDE_CG_CFI_GENERATOR_H
#define MAPLEBE_INCLUDE_CG_CFI_GENERATOR_H
#include "cg_phase.h"
#include "cfi.h"
#include "cgfunc.h"
#include "insn.h"
#include "aarch64_insn.h"
#include "aarch64_operand.h"

namespace maplebe {
class GenCfi {
 public:
  explicit GenCfi(CGFunc &func) : cg(*func.GetCG()), cgFunc(func) {}
  virtual ~GenCfi() = default;

  void Run();

 protected:
  void InsertCFIDefCfaOffset(BB &bb, Insn &insn, int32 &cfiOffset); /* cfiOffset in-out */
  Insn &FindStackDefNextInsn(BB &bb) const;
  Insn &FindReturnInsn(BB &bb) const;

  /* CFI related routines */
  int64 GetOffsetFromCFA() const {
    return offsetFromCfa;
  }

  /* add increment (can be negative) and return the new value */
  int64 AddtoOffsetFromCFA(int64 delta) {
    offsetFromCfa += delta;
    return offsetFromCfa;
  }

  CG &cg;
  CGFunc &cgFunc;
  /* SP offset from Call Frame Address */
  int64 offsetFromCfa = 0;
  bool useFP = true;

  static constexpr const int32 kOffset8MemPos = 8;

 private:
  void GenerateStartDirective(BB &bb);
  void GenerateEndDirective(BB &bb);
  void GenerateRegisterStateDirective(BB &bb);
  virtual void GenerateRegisterSaveDirective(BB &bb) {}
  virtual void GenerateRegisterRestoreDirective(BB &bb) {}

  /* It is do insert a start location information for each function in debugging mode */
  void InsertFirstLocation(BB &bb);
};
}  /* namespace maplebe */
#endif  /* MAPLEBE_INCLUDE_CG_CFI_GENERATOR_H */
