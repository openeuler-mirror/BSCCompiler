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
#ifndef MAPLEBE_INCLUDE_CG_X64_X64_ARGS_H
#define MAPLEBE_INCLUDE_CG_X64_X64_ARGS_H

#include "args.h"
#include "x64_isa.h"
#include "x64_cgfunc.h"
#include "x64_call_conv.h"

namespace maplebe {
using namespace maple;
using namespace x64;

struct ArgInfo {
  X64reg reg;
  MIRType *mirTy;
  uint32 symSize;
  uint32 stkSize;
  RegType regType;
  MIRSymbol *sym;
  const X64SymbolAlloc *symLoc;
  uint8 memPairSecondRegSize;  /* struct arg requiring two regs, size of 2nd reg */
  bool doMemPairOpt;
  bool createTwoStores;
  bool isTwoRegParm;
};

class X64MoveRegArgs : public MoveRegArgs {
 public:
  explicit X64MoveRegArgs(CGFunc &func) : MoveRegArgs(func) {}
  ~X64MoveRegArgs() override = default;
  void Run() override;

 private:
  void CollectRegisterArgs(std::map<uint32, X64reg> &argsList, std::vector<uint32> &indexList,
                           std::map<uint32, X64reg> &pairReg, std::vector<uint32> &numFpRegs,
                           std::vector<uint32> &fpSize) const;
  ArgInfo GetArgInfo(std::map<uint32, X64reg> &argsList, uint32 argIndex,
      std::vector<uint32> &numFpRegs, std::vector<uint32> &fpSize) const;
  void GenerateMovInsn(ArgInfo &argInfo, X64reg reg2);
  void MoveRegisterArgs();
  void MoveVRegisterArgs();
  void LoadStackArgsToVReg(MIRSymbol &mirSym);
  void MoveArgsToVReg(const CCLocInfo &ploc, MIRSymbol &mirSym);
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_X64_X64_ARGS_H */
