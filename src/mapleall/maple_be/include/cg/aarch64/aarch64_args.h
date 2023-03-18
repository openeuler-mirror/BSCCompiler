/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ARGS_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ARGS_H

#include "args.h"
#include "aarch64_cgfunc.h"

namespace maplebe {
using namespace maple;

class AArch64MoveRegArgs : public MoveRegArgs {
 public:
  explicit AArch64MoveRegArgs(CGFunc &func) : MoveRegArgs(func) {
    aarFunc = static_cast<AArch64CGFunc*>(&func);
  }
  ~AArch64MoveRegArgs() override {
    aarFunc = nullptr;
  }
  void Run() override;

 private:
  // gen param to stack
  // call foo(var $a) -> str X0, [memOpnd]
  void MoveRegisterArgs();

  // gen param to preg
  // call foo(%1) -> mov V201, X0
  void MoveVRegisterArgs() const;
  void MoveLocalRefVarToRefLocals(MIRSymbol &mirSym) const;
  void LoadStackArgsToVReg(MIRSymbol &mirSym) const;
  void MoveArgsToVReg(const CCLocInfo &ploc, MIRSymbol &mirSym) const;
  Insn &CreateMoveArgsToVRegInsn(MOperator mOp, RegOperand &destOpnd, RegOperand &srcOpnd, PrimType primType) const;

  AArch64CGFunc *aarFunc = nullptr;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ARGS_H */
