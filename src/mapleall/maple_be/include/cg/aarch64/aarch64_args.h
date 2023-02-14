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

struct ArgInfo {
  AArch64reg reg;
  MIRType *mirTy;
  uint32 symSize;
  uint32 stkSize;
  RegType regType;
  MIRSymbol *sym;
  const AArch64SymbolAlloc *symLoc;
  uint8 memPairSecondRegSize;  /* struct arg requiring two regs, size of 2nd reg */
  bool doMemPairOpt;
  bool createTwoStores;
  bool isTwoRegParm;
};

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
  RegOperand *baseReg = nullptr;
  const MemSegment *lastSegment = nullptr;
  void CollectRegisterArgs(std::map<uint32, AArch64reg> &argsList, std::vector<uint32> &indexList,
                           std::map<uint32, AArch64reg> &pairReg, std::vector<uint32> &numFpRegs,
                           std::vector<uint32> &fpSize) const;
  ArgInfo GetArgInfo(std::map<uint32, AArch64reg> &argsList, std::vector<uint32> &numFpRegs,
                     std::vector<uint32> &fpSize, uint32 argIndex) const;
  bool IsInSameSegment(const ArgInfo &firstArgInfo, const ArgInfo &secondArgInfo) const;
  void GenOneInsn(const ArgInfo &argInfo, RegOperand &baseOpnd, uint32 stBitSize, AArch64reg dest, int32 offset) const;
  void GenerateStpInsn(const ArgInfo &firstArgInfo, const ArgInfo &secondArgInfo);
  void GenerateStrInsn(const ArgInfo &argInfo, AArch64reg reg2, uint32 numFpRegs, uint32 fpSize);
  void MoveRegisterArgs();
  void MoveVRegisterArgs() const;
  void MoveLocalRefVarToRefLocals(MIRSymbol &mirSym) const;
  void LoadStackArgsToVReg(MIRSymbol &mirSym) const;
  void MoveArgsToVReg(const CCLocInfo &ploc, MIRSymbol &mirSym) const;
  Insn &CreateMoveArgsToVRegInsn(MOperator mOp, RegOperand &destOpnd, RegOperand &srcOpnd, PrimType primType) const;

  AArch64CGFunc *aarFunc;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ARGS_H */
