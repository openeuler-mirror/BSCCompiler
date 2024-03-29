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
  DUInsnInfo(Insn *cInsn, uint32 cIdx, MapleAllocator &alloc) : insn(cInsn), defUseInfo(alloc.Adapter()) {
    IncreaseDU(cIdx);
  }
  void IncreaseDU(uint32 idx) {
    if (defUseInfo.count(idx) == 0) {
      defUseInfo[idx] = 0;
    }
    defUseInfo[idx]++;
  }
  void DecreaseDU(uint32 idx) {
    ASSERT(defUseInfo[idx] > 0, "no def/use any more");
    defUseInfo[idx]--;
  }
  void ClearDU(uint32 idx) {
    ASSERT(defUseInfo.count(idx), "no def/use find");
    defUseInfo[idx] = 0;
  }
  bool HasNoDU() {
    for (auto &it : std::as_const(defUseInfo)) {
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
    return defUseInfo;
  }
 private:
  Insn *insn;
  /* operand idx --- count */
  MapleMap<uint32, uint32> defUseInfo;
};

class VRegVersion {
 public:
  VRegVersion(const MapleAllocator &alloc, RegOperand &vReg, uint32 vIdx, regno_t vregNO)
      : versionAlloc(alloc),
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
  RegOperand *GetSSAvRegOpnd(bool isDef = true) {
    if (!isDef) {
      return implicitCvtedRegOpnd;
    }
    return ssaRegOpnd;
  }
  uint32 GetVersionIdx() const {
    return versionIdx;
  }
  regno_t GetOriginalRegNO() const {
    return originalRegNO;
  }
  void AddUseInsn(CGSSAInfo &ssaInfo, Insn &useInsn, uint32 idx);
  /* elimate dead use */
  void CheckDeadUse(const Insn &useInsn);
  void RemoveUseInsn(const Insn &useInsn, uint32 idx);
  MapleUnorderedMap<uint32, DUInsnInfo*> &GetAllUseInsns() {
    return useInsnInfos;
  }
  void MarkDeleted() {
    deleted = true;
  }
  void MarkRecovery() {
    deleted = false;
  }
  bool IsDeleted() const {
    return deleted;
  }
  void SetImplicitCvt() {
    hasImplicitCvt = true;
  }
  bool HasImplicitCvt() const {
    return hasImplicitCvt;
  }

 private:
  MapleAllocator versionAlloc;
  /* if this version has implicit conversion, it refers to def reg */
  RegOperand *ssaRegOpnd;
  RegOperand *implicitCvtedRegOpnd = nullptr;
  uint32 versionIdx;
  regno_t originalRegNO;
  DUInsnInfo *defInsnInfo = nullptr;
  SSAOpndDefBy defType = kDefByNo;
  /* insn ID ->  insn* & operand Idx */
  // --> vector?
  MapleUnorderedMap<uint32, DUInsnInfo*> useInsnInfos;
  bool deleted = false;
  /*
   * def reg (size:64)  or       def reg (size:32)  -->
   * all use reg (size:32)       all use reg (size:64)
   * do not support single use which has implicit conversion yet
   * support single use in defUseInfo in future
   */
  bool hasImplicitCvt = false;
};

class CGSSAInfo {
 public:
  CGSSAInfo(CGFunc &f, DomAnalysis &da, MemPool &mp, MemPool &tmp)
      : cgFunc(&f),
        memPool(&mp),
        tempMp(&tmp),
        ssaAlloc(&mp),
        domInfo(&da),
        renamedBBs(ssaAlloc.Adapter()),
        vRegDefCount(ssaAlloc.Adapter()),
        vRegStk(ssaAlloc.Adapter()),
        allSSAOperands(ssaAlloc.Adapter()),
        noDefVRegs(ssaAlloc.Adapter()),
        reversePostOrder(ssaAlloc.Adapter()),
        safePropInsns(ssaAlloc.Adapter()) {}
  virtual ~CGSSAInfo() = default;
  void ConstructSSA();
  VRegVersion *FindSSAVersion(regno_t ssaRegNO); /* Get specific ssa info */
  Insn *GetDefInsn(const RegOperand &useReg);
  virtual void ReplaceInsn(Insn &oriInsn, Insn &newInsn) = 0; /* replace insn & update ssaInfo */
  virtual void ReplaceAllUse(VRegVersion *toBeReplaced, VRegVersion *newVersion) = 0;
  virtual void CreateNewInsnSSAInfo(Insn &newInsn) = 0;
  PhiOperand &CreatePhiOperand();

  DUInsnInfo *CreateDUInsnInfo(Insn *cInsn, uint32 idx) {
    return memPool->New<DUInsnInfo>(cInsn, idx, ssaAlloc);
  }
  const MapleUnorderedMap<regno_t, VRegVersion*> &GetAllSSAOperands() const {
    return allSSAOperands;
  }
  bool IsNoDefVReg(regno_t vRegNO) const {
    return noDefVRegs.find(vRegNO) != noDefVRegs.end();
  }
  uint32 GetVersionNOOfOriginalVreg(regno_t vRegNO) {
    if (vRegDefCount.count(vRegNO) > 0) {
      return vRegDefCount[vRegNO];
    }
    ASSERT(false, " original vreg is not existed");
    return 0;
  }
  MapleVector<uint32> &GetReversePostOrder() {
    return reversePostOrder;
  }
  void InsertSafePropInsn(uint32 insnId) {
    (void)safePropInsns.emplace_back(insnId);
  }
  MapleVector<uint32> &GetSafePropInsns() {
    return safePropInsns;
  }
  void DumpFuncCGIRinSSAForm() const;
  virtual void DumpInsnInSSAForm(const Insn &insn) const = 0;
  static uint32 ssaRegNObase;

 protected:
  VRegVersion *CreateNewVersion(RegOperand &virtualOpnd, Insn &defInsn, uint32 idx, bool isDefByPhi = false);
  virtual RegOperand *CreateSSAOperand(RegOperand &virtualOpnd) = 0;
  bool IncreaseSSAOperand(regno_t vRegNO, VRegVersion *vst);
  uint32 IncreaseVregCount(regno_t vRegNO);
  VRegVersion *GetVersion(const RegOperand &virtualOpnd);
  MapleUnorderedMap<regno_t, VRegVersion*> &GetPrivateAllSSAOperands() {
    return allSSAOperands;
  }
  void AddNoDefVReg(regno_t noDefVregNO) {
    noDefVRegs.emplace(noDefVregNO);
  }
  void MarkInsnsInSSA(Insn &insn);
  CGFunc *cgFunc  = nullptr;
  MemPool *memPool = nullptr;
  MemPool *tempMp = nullptr;
  MapleAllocator ssaAlloc;

 private:
  void InsertPhiInsn();
  void RenameVariablesForBB(uint32 bbID);
  void RenameBB(BB &bb);
  void RenamePhi(BB &bb);
  virtual void RenameInsn(Insn &insn) = 0;
  /* build ssa on virtual register only */
  virtual RegOperand *GetRenamedOperand(RegOperand &vRegOpnd, bool isDef, Insn &curInsn, uint32 idx) = 0;
  void RenameSuccPhiUse(const BB &bb);
  void PrunedPhiInsertion(const BB &bb, RegOperand &virtualOpnd);

  void AddRenamedBB(uint32 bbID) {
    ASSERT(!renamedBBs.count(bbID), "cgbb has been renamed already");
    renamedBBs.emplace(bbID);
  }
  bool IsBBRenamed(uint32 bbID) const {
    return renamedBBs.count(bbID);
  }
  void SetReversePostOrder();
  void PushToRenameStack(VRegVersion *newVersion);

  bool IsNewVersionPushed(regno_t vRegNo) const {
    return isNewVersionPushed->at(vRegNo);
  }

  void SetNewVersionPushed(regno_t vRegNo) {
    isNewVersionPushed->at(vRegNo) = true;
  }

  void ReplaceRenameStackTop(VRegVersion &newVersion) {
    auto &vstStack = vRegStk[newVersion.GetOriginalRegNO()];
    ASSERT(!vstStack.empty(), "must not be empty");
    vstStack.pop();
    vstStack.push(&newVersion);
  }

  DomAnalysis *domInfo = nullptr;
  MapleSet<uint32> renamedBBs;
  /* original regNO - number of definitions (start from 0) */
  MapleMap<regno_t, uint32> vRegDefCount;
  /* original regNO - ssa version stk */
  MapleVector<MapleStack<VRegVersion*>> vRegStk;
  /* ssa regNO - ssa virtual operand version */
  MapleUnorderedMap<regno_t, VRegVersion*> allSSAOperands;
  /* For virtual registers which do not have definition */
  MapleSet<regno_t> noDefVRegs;
  /* only save bb_id to reduce space */
  MapleVector<uint32> reversePostOrder;
  /* destSize < srcSize but can be propagated */
  MapleVector<uint32> safePropInsns;
  int32 insnCount = 0;
  std::vector<bool> *isNewVersionPushed = nullptr;
};

class SSAOperandVisitor : public OperandVisitorBase,
                          public OperandVisitors<RegOperand, ListOperand, MemOperand> {
 public:
  SSAOperandVisitor(Insn &cInsn, const OpndDesc &cDes, uint32 idx) : insn(&cInsn), opndDes(&cDes), idx(idx) {}
  SSAOperandVisitor() = default;
  ~SSAOperandVisitor() override = default;
  void SetInsnOpndInfo(Insn &cInsn, const OpndDesc &cDes, uint32 index) {
    insn = &cInsn;
    opndDes = &cDes;
    this->idx = index;
  }

 protected:
  Insn *insn = nullptr;
  const OpndDesc *opndDes = nullptr;
  uint32 idx = 0;
};

class SSAOperandDumpVisitor : public OperandVisitorBase,
                              public OperandVisitors<RegOperand, ListOperand, MemOperand>,
                              public OperandVisitor<PhiOperand> {
 public:
  explicit SSAOperandDumpVisitor(const MapleUnorderedMap<regno_t, VRegVersion*> &allssa) : allSSAOperands(allssa) {}
  ~SSAOperandDumpVisitor() override = default;
  void SetHasDumped() {
    hasDumped = true;
  }
  bool HasDumped() const {
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

#endif // MAPLEBE_CG_INCLUDE_CG_SSA_H
