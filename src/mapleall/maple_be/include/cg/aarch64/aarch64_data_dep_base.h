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
  bool IsFrameReg(const RegOperand &opnd) const override;
  bool IsMemOffsetOverlap(const Insn &memInsn1, const Insn &memInsn2) const;
  bool NeedBuildDepsForStackMem(const Insn &memInsn1, const Insn &memInsn2) const;
  bool NeedBuildDepsForHeapMem(const Insn &memInsn1, const Insn &memInsn2) const;

  void AnalysisAmbiInsns(BB &bb) override;

  void BuildDepsForMemDefCommon(Insn &insn, CDGNode &cdgNode);
  void BuildDepsForMemUseCommon(Insn &insn, CDGNode &cdgNode);
  void BuildDepsAccessStImmMem(Insn &insn) override;
  void BuildCallerSavedDeps(Insn &insn) override;
  void BuildDepsDirtyStack(Insn &insn) override;
  void BuildDepsUseStack(Insn &insn) override;
  void BuildDepsDirtyHeap(Insn &insn) override;
  void BuildDepsMemBar(Insn &insn) override;
  void BuildDepsUseMem(Insn &insn, MemOperand &memOpnd) override;
  void BuildDepsDefMem(Insn &insn, MemOperand &memOpnd) override;
  void BuildMemOpndDependency(Insn &insn, Operand &opnd, const OpndDesc &regProp);

  void BuildOpndDependency(Insn &insn) override;
  void BuildSpecialInsnDependency(Insn &insn, const MapleVector<DepNode*> &nodes) override;
  void BuildSpecialCallDeps(Insn &insn) override;
  void BuildAsmInsnDependency(Insn &insn) override;

  void BuildInterBlockMemDefUseDependency(DepNode &depNode, bool isMemDef) override;
  void BuildPredPathMemDefDependencyDFS(BB &curBB, std::vector<bool> &visited, DepNode &depNode) override;
  void BuildPredPathMemUseDependencyDFS(BB &curBB, std::vector<bool> &visited, DepNode &depNode) override;

  void DumpNodeStyleInDot(std::ofstream &file, DepNode &depNode) override;

 protected:
  void BuildSpecialBLDepsForJava(Insn &insn);
};
}

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_DATA_DEP_BASE_H */
