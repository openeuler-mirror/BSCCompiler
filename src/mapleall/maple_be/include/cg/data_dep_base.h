/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEBE_INCLUDE_CG_DATA_DEP_BASE_H
#define MAPLEBE_INCLUDE_CG_DATA_DEP_BASE_H

#include "deps.h"
#include "cgbb.h"
#include "cg_cdg.h"

namespace maplebe {
using namespace maple;
constexpr maple::uint32 kMaxDependenceNum = 200;

class DataDepBase {
 public:
  DataDepBase(MemPool &memPool, CGFunc &func, MAD &mad)
      : memPool(memPool), alloc(&memPool), cgFunc(func), mad(mad), beforeRA(!cgFunc.IsAfterRegAlloc()) {}
  virtual ~DataDepBase() {
    curCDGNode = nullptr;
  }

  enum DataFlowInfoType : uint8 {
    kDataFlowUndef,
    kMembar,
    kLastCall,
    kLastFrameDef,
    kStackUses,
    kStackDefs,
    kHeapUses,
    kHeapDefs,
    kMayThrows,
    kAmbiguous,
  };

  uint32 GetSeparatorIndex() const {
    return separatorIndex;
  }
  void SetSeparatorIndex(uint32 index) {
    separatorIndex = index;
  }
  void SetCDGNode(CDGNode *cdgNode) {
    curCDGNode = cdgNode;
  }
  CDGNode *GetCDGNode() {
    return curCDGNode;
  }
  void SetLastFrameDefInsn(Insn *insn) const {
    curCDGNode->SetLastFrameDefInsn(insn);
  }
  void CopyAndClearComments(MapleVector<Insn*> &comments) const {
    curCDGNode->CopyAndClearComments(comments);
  }
  MapleVector<Insn*> &GetLastComments() const {
    return curCDGNode->GetLastComments();
  }
  void ClearLastComments() const {
    curCDGNode->ClearLastComments();
  }

  void ProcessNonMachineInsn(Insn &insn, MapleVector<Insn*> &comments, MapleVector<DepNode*> &dataNodes,
                             const Insn *&locInsn);

  void AddDependence4InsnInVectorByType(MapleVector<Insn*> &insns, Insn &insn, const DepType &type);
  void AddDependence4InsnInVectorByTypeAndCmp(MapleVector<Insn*> &insns, Insn &insn, const DepType &type);

  void DumpDepNode(const DepNode &node) const;
  void DumpDepLink(const DepLink &link, const DepNode *node) const;
  const std::string &GetDepTypeName(DepType depType) const;
  void CombineDependence(DepNode &firstNode, const DepNode &secondNode, bool isAcrossSeparator,
                         bool isMemCombine = false);

  bool IsIntraBlockAnalysis() const;
  bool IfInAmbiRegs(regno_t regNO) const;
  void AddDependence(DepNode &fromNode, DepNode &toNode, DepType depType);
  void RemoveSelfDeps(Insn &insn);
  void BuildDepsUseReg(Insn &insn, regno_t regNO);
  void BuildDepsDefReg(Insn &insn, regno_t regNO);
  void BuildDepsAmbiInsn(Insn &insn);
  void BuildAmbiInsnDependency(Insn &insn);
  void BuildDepsMayThrowInsn(Insn &insn);
  void BuildMayThrowInsnDependency(Insn &insn);
  void BuildDepsSeparator(DepNode &newSepNode, MapleVector<DepNode*> &nodes);
  void BuildDepsControlAll(Insn &insn, const MapleVector<DepNode*> &nodes);
  void BuildDepsBetweenControlRegAndCall(Insn &insn, bool isDest);
  void BuildDepsLastCallInsn(Insn &insn);
  void SeparateDependenceGraph(MapleVector<DepNode*> &nodes, uint32 &nodeSum);
  DepNode *GenerateDepNode(Insn &insn, MapleVector<DepNode*> &nodes, uint32 &nodeSum, MapleVector<Insn*> &comments);
  void UpdateStackAndHeapDependency(DepNode &depNode, Insn &insn, const Insn &locInsn);
  void BuildSeparatorNodeDependency(MapleVector<DepNode*> &dataNodes, Insn &insn);
  void BuildInterBlockDefUseDependency(DepNode &curDepNode, regno_t regNO, DepType depType, bool isDef);
  void BuildPredPathDefDependencyDFS(BB &curBB, std::vector<bool> &visited, DepNode &depNode,
                                     regno_t regNO, DepType depType);
  void BuildPredPathUseDependencyDFS(BB &curBB, std::vector<bool> &visited, DepNode &depNode,
                                     regno_t regNO, DepType depType);
  void BuildInterBlockSpecialDataInfoDependency(DepNode &curDepNode, bool needCmp, DepType depType,
                                                DataDepBase::DataFlowInfoType infoType);
  void BuildPredPathSpecialDataInfoDependencyDFS(BB &curBB, std::vector<bool> &visited, bool needCmp, DepNode &depNode,
                                                 DepType depType, DataDepBase::DataFlowInfoType infoType);

  virtual void CombineClinit(DepNode &firstNode, DepNode &secondNode, bool isAcrossSeparator) = 0;
  virtual void CombineMemoryAccessPair(DepNode &firstNode, DepNode &secondNode, bool useFirstOffset) = 0;
  virtual bool IsFrameReg(const RegOperand&) const = 0;
  virtual void AnalysisAmbiInsns(BB &bb) = 0;
  virtual void BuildDepsMemBar(Insn &insn) = 0;
  virtual void BuildDepsUseMem(Insn &insn, MemOperand &memOpnd) = 0;
  virtual void BuildDepsDefMem(Insn &insn, MemOperand &memOpnd) = 0;
  virtual void BuildDepsAccessStImmMem(Insn &insn, bool isDest) = 0;
  virtual void BuildCallerSavedDeps(Insn &insn) = 0;
  virtual void BuildStackPassArgsDeps(Insn &insn) = 0;
  virtual void BuildDepsDirtyStack(Insn &insn) = 0;
  virtual void BuildDepsUseStack(Insn &insn) = 0;
  virtual void BuildDepsDirtyHeap(Insn &insn) = 0;
  virtual void BuildOpndDependency(Insn &insn) = 0;
  virtual void BuildSpecialInsnDependency(Insn &insn, const MapleVector<DepNode*> &nodes) = 0;
  virtual void UpdateRegUseAndDef(Insn &insn, const DepNode &depNode, MapleVector<DepNode*> &nodes) = 0;
  virtual DepNode *BuildSeparatorNode() = 0;
  virtual void BuildInterBlockMemDefUseDependency(DepNode &depNode, MemOperand &memOpnd,
                                                  MemOperand *nextMemOpnd, bool isMemDef) = 0;
  virtual void BuildPredPathMemDefDependencyDFS(BB &curBB, std::vector<bool> &visited, DepNode &depNode,
                                                MemOperand &memOpnd, MemOperand *nextMemOpnd) = 0;
  virtual void BuildPredPathMemUseDependencyDFS(BB &curBB, std::vector<bool> &visited, DepNode &depNode,
                                                MemOperand &memOpnd, MemOperand *nextMemOpnd) = 0;

 protected:
  MemPool &memPool;
  MapleAllocator alloc;
  CGFunc &cgFunc;
  MAD &mad;
  CDGNode *curCDGNode = nullptr;
  bool beforeRA;
  uint32 separatorIndex = 0;
};
}

#endif  /* MAPLEBE_INCLUDE_CG_DATA_DEP_BASE_H */
