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
  ~X0OptInfo() = default;

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
  ~RaX0Opt() = default;

  bool PropagateX0CanReplace(Operand *opnd, regno_t replaceReg) const;
  bool PropagateRenameReg(Insn *nInsn, const X0OptInfo &optVal) const;
  bool PropagateX0DetectX0(const Insn *insn, X0OptInfo &optVal) const;
  bool PropagateX0DetectRedefine(const AArch64MD *md, const Insn *ninsn, const X0OptInfo &optVal, uint32 index) const;
  bool PropagateX0Optimize(const BB *bb, const Insn *insn, X0OptInfo &optVal) const;
  bool PropagateX0ForCurrBb(BB *bb, const X0OptInfo &optVal) const;
  void PropagateX0ForNextBb(BB *nextBb, const X0OptInfo &optVal) const;
  void PropagateX0();

 private:
  CGFunc *cgFunc;
};

class ParamRegOpt {
 public:
  ParamRegOpt(CGFunc *func, DomAnalysis *dom) : cgFunc(func), domInfo(dom) {}
  ~ParamRegOpt() = default;

  void HandleParamReg();
  void CollectRefBBs(RegOperand &movDest, std::set<uint32> &refBBs);
  void TryToSplitParamReg(RegOperand &movDest, Insn &posInsn);
  BB* GetCommondDom(std::set<uint32> &refBBs);
  bool DominatorAll(uint32 domBB, std::set<uint32> &refBBs);
  void SplitAtDomBB(RegOperand &movDest, BB &domBB, Insn &posInsn);
  void SetDumpInfo(bool val) {
    dumpInfo = val;
  }

 private:
  CGFunc *cgFunc;
  DomAnalysis *domInfo;
  bool dumpInfo = false;
};

class VregRenameInfo {
 public:
  VregRenameInfo() = default;

  ~VregRenameInfo() = default;

  BB *firstBBLevelSeen = nullptr;
  BB *lastBBLevelSeen = nullptr;
  uint32 numDefs = 0;
  uint32 numUses = 0;
  uint32 numInnerDefs = 0;
  uint32 numInnerUses = 0;
  uint32 largestUnusedDistance = 0;
  uint8 innerMostloopLevelSeen = 0;
};

class VregRename {
 public:
  VregRename(CGFunc *func, MemPool *pool) : cgFunc(func), memPool(pool), alloc(pool), renameInfo(alloc.Adapter()) {
    renameInfo.resize(cgFunc->GetMaxRegNum());
    ccRegno = static_cast<RegOperand *>(&cgFunc->GetOrCreateRflag())->GetRegisterNumber();
  }
  ~VregRename() = default;

  void PrintRenameInfo(regno_t regno) const;
  void PrintAllRenameInfo() const;

  void RenameFindLoopVregs(const CGFuncLoops *loop);
  void RenameFindVregsToRename(const CGFuncLoops *loop);
  bool IsProfitableToRename(const VregRenameInfo *info) const;
  void RenameProfitableVreg(RegOperand *ropnd, const CGFuncLoops *loop);
  void RenameGetFuncVregInfo();
  void UpdateVregInfo(regno_t vreg, BB *bb, bool isInner, bool isDef);
  void VregLongLiveRename();

  CGFunc *cgFunc = nullptr;
  MemPool *memPool = nullptr;
  MapleAllocator alloc;
  Bfs *bfs = nullptr;
  MapleVector<VregRenameInfo*> renameInfo;
  uint32 maxRegnoSeen = 0;
  regno_t ccRegno = 0;
};

class AArch64RaOpt : public RaOpt {
 public:
  AArch64RaOpt(CGFunc &func, MemPool &pool) : RaOpt(func, pool) {}
  ~AArch64RaOpt() override = default;
  void Run() override;

 private:
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64RAOPT_H */
