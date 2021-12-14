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

namespace maplebe {
enum SSAOpndDefBy {
  kDefByNo,
  kDefByInsn,
  kDefByPhi
};

class VRegVersion {
 public:
  VRegVersion(MapleAllocator &allocr, RegOperand &vReg, uint32 vIdx, regno_t vregNO) :
      versionAlloc(allocr),
      ssaRegOpnd(&vReg),
      versionIdx(vIdx),
      originalRegNO(vregNO),
      useInsns(versionAlloc.Adapter()) {}
  void SetDefInsn(Insn *insn, SSAOpndDefBy defTy) {
    defInsn = insn;
    defType = defTy;
  }
  void SetNewSSAOpnd(RegOperand &regOpnd);
  Insn *GetDefInsn() const {
    return defInsn;
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
  void addUseInsn(Insn &insn) {
    useInsns.push_back(&insn);
  }

 private:
  MapleAllocator versionAlloc;
  RegOperand *ssaRegOpnd;
  uint32 versionIdx;
  regno_t originalRegNO;
  Insn *defInsn = nullptr;
  SSAOpndDefBy defType = kDefByNo;
  MapleVector<Insn*> useInsns;
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

  void DumpFuncCGIRinSSAForm() const;
  virtual void DumpInsnInSSAForm(const Insn &insn) const = 0;
  const MapleUnorderedMap<regno_t, VRegVersion*> &GetAllSSAOperands() const {
    return allSSAOperands;
  }
  bool IsNoDefVReg(regno_t vRegNO) const {
    return noDefVRegs.find(vRegNO) != noDefVRegs.end();
  }
  static uint32 SSARegNObase;

 protected:
  VRegVersion *CreateNewVersion(RegOperand &virtualOpnd, Insn &defInsn, bool isDefByPhi = false);
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
  virtual RegOperand *GetRenamedOperand(RegOperand &vRegOpnd, bool isDef, Insn &curInsn) = 0;
  void RenameSuccPhiUse(BB &bb);
  void PrunedPhiInsertion(BB &bb, RegOperand &virtualOpnd);

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
