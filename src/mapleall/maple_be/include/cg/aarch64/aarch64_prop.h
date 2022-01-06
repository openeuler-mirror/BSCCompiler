/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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

#ifndef MAPLEBE_INCLUDE_AARCH64_PROP_H
#define MAPLEBE_INCLUDE_AARCH64_PROP_H

#include "cg_prop.h"
#include "aarch64_cgfunc.h"
#include "aarch64_strldr.h"
namespace maplebe{
class AArch64Prop : public CGProp {
 public:
  AArch64Prop(MemPool &mp, CGFunc &f, CGSSAInfo &sInfo)
      : CGProp(mp, f, sInfo){}

 private:
  void CopyProp(Insn &insn) override;
  /*
   * for aarch64
   * 1. extended register prop
   * 2. shift register prop
   * 3. add/ext/shf prop -> str/ldr
   */
  void TargetProp(Insn &insn) override;

  void ReplaceAllUse(VRegVersion *toBeReplaced, VRegVersion *newVersion);
};

class A64StrLdrProp {
 public:
  A64StrLdrProp(MemPool &mp, CGFunc &f, CGSSAInfo &sInfo, Insn &insn)
      : cgFunc(&f),
        ssaInfo(&sInfo),
        curInsn(&insn),
        a64StrLdrAlloc(&mp),
        replaceVersions(a64StrLdrAlloc.Adapter()) {}
  void DoOpt();
 private:
  AArch64MemOperand *StrLdrPropPreCheck(Insn &insn, MemPropMode prevMod = kUndef);
  static MemPropMode SelectStrLdrPropMode(AArch64MemOperand &currMemOpnd);
  bool ReplaceMemOpnd(AArch64MemOperand &currMemOpnd, Insn *defInsn);
  AArch64MemOperand *SelectReplaceMem(Insn &defInsn, AArch64MemOperand &currMemOpnd);
  AArch64RegOperand *GetReplaceReg(AArch64RegOperand &a64Reg);
  AArch64MemOperand *HandleArithImmDef(AArch64RegOperand &replace, Operand *oldOffset, int64 defVal);
  AArch64MemOperand *SelectReplaceExt(Insn &defInsn, RegOperand &base, uint32 amount, bool isSigned);
  bool CheckNewMemOffset(Insn &insn, AArch64MemOperand *newMemOpnd, uint32 opndIdx);
  void DoMemReplace(RegOperand &replacedReg, AArch64MemOperand &newMem, Insn &useInsn);
  uint32 GetMemOpndIdx(AArch64MemOperand *newMemOpnd, Insn &insn);

  bool CheckSameReplace(RegOperand &replacedReg, AArch64MemOperand *memOpnd);

  CGFunc *cgFunc;
  CGSSAInfo *ssaInfo;
  Insn *curInsn;
  MapleAllocator a64StrLdrAlloc;
  MapleMap<regno_t, VRegVersion*> replaceVersions;
  MemPropMode memPropMode = kUndef;
};

class A64ReplaceRegOpndVisitor : public ReplaceRegOpndVisitor {
 public:
  A64ReplaceRegOpndVisitor(CGFunc &f, Insn &cInsn, uint32 cIdx, RegOperand &oldRegister ,RegOperand &newRegister)
      : ReplaceRegOpndVisitor(f, cInsn, cIdx, oldRegister, newRegister) {}
 private:
  void Visit(RegOperand *v) final;
  void Visit(ListOperand *v) final;
  void Visit(MemOperand *v) final;
  void Visit(PhiOperand *v) final;
};
}
#endif /* MAPLEBE_INCLUDE_AARCH64_PROP_H */
