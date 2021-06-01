/*
 * Copyright (c) [2021] Futurewei Technologies Co., Ltd. All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */

#ifndef MAPLEBE_INCLUDE_CG_AARCH64RAOPT_H
#define MAPLEBE_INCLUDE_CG_AARCH64RAOPT_H

#include "cg.h"
#include "ra_opt.h"
#include "aarch64_cg.h"
#include "aarch64_insn.h"
#include "aarch64_operand.h"

namespace maplebe {
class X0OptInfo {
 public:
  X0OptInfo() : movSrc(nullptr), replaceReg(0), renameInsn(nullptr), renameOpnd(nullptr), renameReg(0) {}

  inline RegOperand *GetMovSrc() const {
    return movSrc;
  }

  inline regno_t GetReplaceReg() const {
    return replaceReg;
  }

  inline Insn *GetRenameInsn() const {
    return renameInsn;
  }

  inline Operand *GetRenameOpnd() const {
    return renameOpnd;
  }

  inline regno_t GetRenameReg() const {
    return renameReg;
  }

  inline void SetMovSrc(RegOperand *srcReg) {
    movSrc = srcReg;
  }

  inline void SetReplaceReg(regno_t regno) {
    replaceReg = regno;
  }

  inline void SetRenameInsn(Insn *insn) {
    renameInsn = insn;
  }

  inline void ResetRenameInsn() {
    renameInsn = nullptr;
  }

  inline void SetRenameOpnd(Operand *opnd) {
    renameOpnd = opnd;
  }

  inline void SetRenameReg(regno_t regno) {
    renameReg = regno;
  }

 private:
  RegOperand *movSrc;
  regno_t replaceReg;
  Insn *renameInsn;
  Operand *renameOpnd;
  regno_t renameReg;
};

class RaX0Opt {
 public:
  explicit RaX0Opt(CGFunc* func) : cgFunc(func)  {}

  bool PropagateX0CanReplace(Operand *opnd, regno_t replaceReg) const;
  bool PropagateRenameReg(Insn *insn, const X0OptInfo &optVal);
  bool PropagateX0DetectX0(const Insn *insn, X0OptInfo &optVal);
  bool PropagateX0DetectRedefine(const AArch64MD *md, const Insn *ninsn, const X0OptInfo &optVal, uint32 index);
  bool PropagateX0Optimize(const BB *bb, const Insn *insn, X0OptInfo &optVal);
  bool PropagateX0ForCurrBb(BB *bb, const X0OptInfo &optVal);
  void PropagateX0ForNextBb(BB *nextBb, const X0OptInfo &optVal);
  void PropagateX0();

 private:
  CGFunc *cgFunc;
};

class AArch64RaOpt : public RaOpt {
 public:
  explicit AArch64RaOpt(CGFunc &func) : RaOpt(func) {}
  ~AArch64RaOpt() override = default;
  void Run() override;

 private:
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64RAOPT_H */
