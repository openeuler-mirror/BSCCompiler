/*
 * Copyright (c) [2022] Futurewei Technologies, Inc. All rights reserved.
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

#ifndef MAPLEBE_INCLUDE_AARCH64_STANDARDIZE_H
#define MAPLEBE_INCLUDE_AARCH64_STANDARDIZE_H

#include "standardize.h"

namespace maplebe {

enum TargetOperandAction : uint8 {
  kAbtractReg,
  kAbtractMem,
  kAbtractImm,
  kAbtractNone,
};

struct TargetMopGen {
  AArch64MOP_t targetMop;
  std::vector<TargetOperandAction> targetOpndAction;
  std::vector<uint8> mappingOrder;
};

class AbstractIR2Target {
 public:
  abstract::AbstractMOP_t abstractMop;
  std::vector<TargetMopGen> targetMap;
};

class AArch64Standardize : public Standardize {
 public:
  explicit AArch64Standardize(CGFunc &f) : Standardize(f) {
    SetAddressMapping(false);
  }

  ~AArch64Standardize() override = default;

 private:
  void Legalize() override;
  void StdzMov(Insn &insn) override;
  void StdzStrLdr(Insn &insn) override;
  void StdzBasicOp(Insn &insn) override;
  void StdzUnaryOp(Insn &insn) override;
  void StdzCvtOp(Insn &insn) override;
  void StdzShiftOp(Insn &insn) override;
  void StdzCommentOp(Insn &insn) override;

  Operand *UpdateRegister(Operand &opnd, std::map<regno_t, regno_t> &regMap, bool allocate);
  void TraverseOperands(Insn *insn, std::map<regno_t, regno_t> &regMap, bool allocate);
  Operand *GetInsnResult(Insn *insn);
  Insn *HandleTargetImm(Insn *insn, Insn *newInsn, uint32 idx, MOperator targetMop, uint8 order);
  void SelectTargetInsn(Insn *insn);
};
}
#endif  /* MAPLEBE_INCLUDE_AARCH64_STANDARDIZE_H */
