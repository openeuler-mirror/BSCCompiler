/*
 * Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_DATA_DEP_BASE_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_DATA_DEP_BASE_H

#include "aarch64_operand.h"
#include "cgfunc.h"
#include "data_dep_base.h"

namespace maplebe {
class AArch64DataDepBase : public DataDepBase {
 public:
  AArch64DataDepBase(MemPool &mp, CGFunc &func, MAD &mad, bool isIntraAna) : DataDepBase(mp, func, mad, isIntraAna) {}
  ~AArch64DataDepBase() override = default;

  void InitCDGNodeDataInfo(MemPool &mp, MapleAllocator &alloc, CDGNode &cdgNode) override;
  void CombineClinit(DepNode &firstNode, DepNode &secondNode, bool isAcrossSeparator) override;
  void CombineMemoryAccessPair(DepNode &firstNode, DepNode &secondNode, bool useFirstOffset) override;
  bool IsFrameReg(const RegOperand &opnd) const override;
  void AnalysisAmbiInsns(BB &bb) override;
  void BuildDepsMemBar(Insn &insn) override;
  void BuildDepsUseMem(Insn &insn, MemOperand &aarchMemOpnd) override;
  void BuildDepsDefMem(Insn &insn, MemOperand &aarchMemOpnd) override;
  void BuildDepsAccessStImmMem(Insn &insn, bool isDest) override;
  void BuildCallerSavedDeps(Insn &insn) override;
  void BuildStackPassArgsDeps(Insn &insn) override;
  void BuildDepsDirtyStack(Insn &insn) override;
  void BuildDepsUseStack(Insn &insn) override;
  void BuildDepsDirtyHeap(Insn &insn) override;
  void BuildOpndDependency(Insn &insn) override;
  void BuildSpecialInsnDependency(Insn &insn, const MapleVector<DepNode*> &nodes) override;
  void BuildAsmInsnDependency(Insn &insn) override;
  void UpdateRegUseAndDef(Insn &insn, const DepNode &depNode, DepNode &sepNode) override;
  DepNode *BuildSeparatorNode() override;
  void BuildInterBlockMemDefUseDependency(DepNode &depNode, MemOperand &memOpnd,
                                          MemOperand *nextMemOpnd, bool isMemDef) override;
  void BuildPredPathMemDefDependencyDFS(BB &curBB, std::vector<bool> &visited, DepNode &depNode,
                                        MemOperand &memOpnd, MemOperand *nextMemOpnd) override;
  void BuildPredPathMemUseDependencyDFS(BB &curBB, std::vector<bool> &visited, DepNode &depNode,
                                        MemOperand &memOpnd, MemOperand *nextMemOpnd) override;
  void DumpNodeStyleInDot(std::ofstream &file, DepNode &depNode) override;

  void BuildAntiDepsDefStackMem(Insn &insn, MemOperand &memOpnd, const MemOperand *nextMemOpnd);
  bool NeedBuildDepsMem(const MemOperand &memOpnd, const MemOperand *nextMemOpnd, const Insn &memInsn) const;

 protected:
  MemOperand *GetNextMemOperand(const Insn &insn, const MemOperand &aarchMemOpnd) const;
  void BuildMemOpndDependency(Insn &insn, Operand &opnd, const OpndDesc &regProp);
  MemOperand *BuildNextMemOperandByByteSize(const MemOperand &aarchMemOpnd, uint32 byteSize) const;
  void ReplaceDepNodeWithNewInsn(DepNode &firstNode, DepNode &secondNode, Insn& newInsn, bool isFromClinit) const;
  void ClearDepNodeInfo(DepNode &depNode) const;
};
}

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_DATA_DEP_BASE_H */
