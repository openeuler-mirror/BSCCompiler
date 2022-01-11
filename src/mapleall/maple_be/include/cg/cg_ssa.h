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
#ifndef MAPLEBE_CG_INCLUDE_CG_SSA_H
#define MAPLEBE_CG_INCLUDE_CG_SSA_H

#include "cgfunc.h"
#include "cg_dominance.h"
#include "live.h"
#include "operand.h"
#include "visitor_common.h"

namespace maplebe {
class CGSSAInfo;
enum SSAOpndDefBy {
  kDefByNo,
  kDefByInsn,
  kDefByPhi
};

/* precise def/use info in machine instrcution */
class DUInsnInfo {
 public:
  DUInsnInfo(Insn *cInsn, uint32 cIdx, MapleAllocator &alloc) : insn(cInsn), DUInfo(alloc.Adapter()) {
    IncreaseDU(cIdx);
  }
  void IncreaseDU(uint32 idx) {
    if (!DUInfo.count(idx)) {
      DUInfo[idx] = 0;
    }
    DUInfo[idx]++;
  }
  void DecreaseDU(uint32 idx) {
    ASSERT(DUInfo[idx] > 0, "no def/use any more");
    DUInfo[idx]--;
  }
  void ClearDU(uint32 idx) {
    ASSERT(DUInfo.count(idx), "no def/use find");
    DUInfo[idx] = 0;
  }
  bool HasNoDU() {
    for(auto it : DUInfo) {
      if (it.second != 0) {
        return false;
      }
    }
    return true;
  }
  Insn *GetInsn() {
    return insn;
  }
  MapleMap<uint32, uint32>& GetOperands() {
    return DUInfo;
  }
 private:
  Insn *insn;
  /* operand idx --- count */
  MapleMap<uint32, uint32> DUInfo;
};

class VRegVersion {
 public:
  VRegVersion(MapleAllocator &alloc, RegOperand &vReg, uint32 vIdx, regno_t vregNO) :
      versionAlloc(alloc),
      ssaRegOpnd(&vReg),
      versionIdx(vIdx),
      originalRegNO(vregNO),
      useInsnInfos(versionAlloc.Adapter()) {}
  void SetDefInsn(DUInsnInfo *duInfo, SSAOpndDefBy defTy) {
    defInsnInfo = duInfo;
    defType = defTy;
  }
  DUInsnInfo *GetDefInsnInfo() const {
    return defInsnInfo;
  }
  SSAOpndDefBy GetDefType() const {
    return defType;
  }
  RegOperand *GetSSAvRegOpnd() {
    return ssaRegOpnd;
  }
  uint32 GetVersionIdx() const {
    return versionIdx;
  }
  regno_t GetOriginalRegNO() const {
    return originalRegNO;
  }
  void AddUseInsn(CGSSAInfo &ssaInfo, Insn &useInsn, uint32 idx);
  void RemoveUseInsn(Insn &useInsn, uint32 idx);
  MapleUnorderedMap<uint32, DUInsnInfo*> &GetAllUseInsns() {
    return useInsnInfos;
  }
  void MarkDeleted() {
    deleted = true;
  }
  void MarkRecovery() {
    deleted = false;
  }
  bool IsDeleted() {
    return deleted;
  }

 private:
  MapleAllocator versionAlloc;
  RegOperand *ssaRegOpnd;
  uint32 versionIdx;
  regno_t originalRegNO;
  DUInsnInfo *defInsnInfo = nullptr;
  SSAOpndDefBy defType = kDefByNo;
  /* insn ID ->  insn* & operand Idx */
  // --> vector?
  MapleUnorderedMap<uint32, DUInsnInfo*> useInsnInfos;
  bool deleted = false;
};

class CGSSAInfo {
 public:
  CGSSAInfo(CGFunc &f, DomAnalysis &da, MemPool &mp, MemPool &tmp) :
      cgFunc(&f),
      memPool(&mp),
      tempMp(&tmp),
      domInfo(&da),
      ssaAlloc(&mp),
      renamedBBs(ssaAlloc.Adapter()),
      vRegDefCount(ssaAlloc.Adapter()),
      vRegStk(ssaAlloc.Adapter()),
      allSSAOperands(ssaAlloc.Adapter()),
      noDefVRegs(ssaAlloc.Adapter()) {}
  virtual ~CGSSAInfo() = default;
  void ConstructSSA();
  VRegVersion *FindSSAVersion(regno_t ssaRegNO); /* Get specific ssa info */
  /* replace insn & update ssaInfo */
  virtual void ReplaceInsn(Insn &oriInsn, Insn &newInsn) = 0;
  /* only used in cg peep */
  virtual void AddInsn(Insn &newInsn) = 0;

  DUInsnInfo *CreateDUInsnInfo(Insn *cInsn, uint32 idx) {
    return memPool->New<DUInsnInfo>(cInsn, idx, ssaAlloc);
  }

  const MapleUnorderedMap<regno_t, VRegVersion*> &GetAllSSAOperands() const {
    return allSSAOperands;
  }
  bool IsNoDefVReg(regno_t vRegNO) const {
    return noDefVRegs.find(vRegNO) != noDefVRegs.end();
  }
  int32 GetVersionNOOfOriginalVreg(regno_t vRegNO) {
    if (vRegDefCount.count(vRegNO)) {
      return vRegDefCount[vRegNO];
    }
    ASSERT(false, " original vreg is not existed");
    return 0;
  }
  void DumpFuncCGIRinSSAForm() const;
  static uint32 SSARegNObase;

 protected:
  VRegVersion *CreateNewVersion(RegOperand &virtualOpnd, Insn &defInsn, uint32 idx, bool isDefByPhi = false);
  virtual RegOperand *CreateSSAOperand(RegOperand &virtualOpnd) = 0;
  VRegVersion *GetVersion(RegOperand &virtualOpnd);
  MapleUnorderedMap<regno_t, VRegVersion*> &GetPrivateAllSSAOperands() {
    return allSSAOperands;
  }
  bool IncreaseSSAOperand(regno_t vRegNO, VRegVersion *vst);
  void AddNoDefVReg(regno_t noDefVregNO) {
    ASSERT(!noDefVRegs.count(noDefVregNO), "duplicate no def Reg, please check");
    noDefVRegs.emplace(noDefVregNO);
  }
  void MarkInsnsInSSA(Insn &insn);
  CGFunc *cgFunc  = nullptr;
  MemPool *memPool = nullptr;
  MemPool *tempMp = nullptr;
 private:
  void InsertPhiInsn();
  void RenameVariablesForBB(uint32 bbID);
  void RenameBB(BB &bb);
  void RenamePhi(BB &bb);
  virtual void RenameInsn(Insn &insn) = 0;
  /* build ssa on virtual register only */
  virtual RegOperand *GetRenamedOperand(RegOperand &vRegOpnd, bool isDef, Insn &curInsn, uint32 idx) = 0;
  void RenameSuccPhiUse(BB &bb);
  void PrunedPhiInsertion(BB &bb, RegOperand &virtualOpnd);
  virtual void DumpInsnInSSAForm(const Insn &insn) const = 0;

  void AddRenamedBB(uint32 bbID) {
    ASSERT(!renamedBBs.count(bbID), "cgbb has been renamed already");
    renamedBBs.emplace(bbID);
  }
  bool IsBBRenamed(uint32 bbID) {
    return renamedBBs.count(bbID);
  }
  uint32 IncreaseVregCount(regno_t vRegNO);

  DomAnalysis *domInfo = nullptr;
  MapleAllocator ssaAlloc;
  MapleSet<uint32> renamedBBs;
  /* original regNO - number of definitions (start from 0) */
  MapleMap<regno_t, uint32> vRegDefCount;
  /* original regNO - ssa version stk */
  MapleMap<regno_t, MapleStack<VRegVersion*>> vRegStk;
  /* ssa regNO - ssa virtual operand version */
  MapleUnorderedMap<regno_t, VRegVersion*> allSSAOperands;
  /* For virtual registers which do not have definition */
  MapleSet<regno_t> noDefVRegs;
  int32 insnCount = 0;
};

class SSAOperandVisitor : public OperandVisitorBase,
                          public OperandVisitors<RegOperand, ListOperand, MemOperand> {
 public:
  SSAOperandVisitor(Insn &cInsn, OpndProp &cProp, uint32 idx) : insn(&cInsn), opndProp(&cProp), idx(idx) {}
  SSAOperandVisitor() = default;
  virtual ~SSAOperandVisitor() = default;
  void SetInsnOpndInfo(Insn &cInsn, OpndProp &cProp, uint32 idx) {
    insn = &cInsn;
    opndProp = &cProp;
    this->idx = idx;
  }

 protected:
  Insn *insn = nullptr;
  OpndProp *opndProp = nullptr;
  uint32 idx = 0;
};

class SSAOperandDumpVisitor : public OperandVisitorBase,
                              public OperandVisitors<RegOperand, ListOperand, MemOperand>,
                              public OperandVisitor<PhiOperand> {
 public:
  explicit SSAOperandDumpVisitor(const MapleUnorderedMap<regno_t, VRegVersion*> &allssa) : allSSAOperands(allssa) {}
  virtual ~SSAOperandDumpVisitor() = default;
  void SetHasDumped() {
    hasDumped = true;
  }
  bool HasDumped() {
    return hasDumped;
  }
  bool hasDumped = false;
 protected:
  const MapleUnorderedMap<regno_t, VRegVersion*> &allSSAOperands;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgSSAConstruct, maplebe::CGFunc);
CGSSAInfo *GetResult() {
  return ssaInfo;
}
CGSSAInfo *ssaInfo = nullptr;
 private:
  void GetAnalysisDependence(maple::AnalysisDep &aDep) const override;
MAPLE_FUNC_PHASE_DECLARE_END
}

#endif //MAPLEBE_CG_INCLUDE_CG_SSA_H
