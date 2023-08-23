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
#ifndef MAPLEBE_INCLUDE_CG_DATA_DEP_BASE_H
#define MAPLEBE_INCLUDE_CG_DATA_DEP_BASE_H

#include "deps.h"
#include "cgbb.h"
#include "cg_cdg.h"

namespace maplebe {
using namespace maple;
constexpr maple::uint32 kMaxDependenceNum = 200;
constexpr maple::uint32 kMaxInsnNum = 220;

class DataDepBase {
 public:
  DataDepBase(MemPool &memPool, CGFunc &func, MAD &mad, bool isIntraAna)
      : memPool(memPool), alloc(&memPool), cgFunc(func), mad(mad),
        beforeRA(!cgFunc.IsAfterRegAlloc()), isIntra(isIntraAna) {}
  virtual ~DataDepBase() {
    curRegion = nullptr;
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

  void SetCDGNode(CDGNode *cdgNode) {
    curCDGNode = cdgNode;
  }
  CDGNode *GetCDGNode() {
    return curCDGNode;
  }
  void SetCDGRegion(CDGRegion *region) {
    curRegion = region;
  }

  void ProcessNonMachineInsn(Insn &insn, MapleVector<Insn*> &comments, MapleVector<DepNode*> &dataNodes,
                             const Insn *&locInsn);

  void AddDependence4InsnInVectorByType(MapleVector<Insn*> &insns, Insn &insn, const DepType &type);
  void AddDependence4InsnInVectorByTypeAndCmp(MapleVector<Insn*> &insns, Insn &insn, const DepType &type);

  const std::string &GetDepTypeName(DepType depType) const;

  bool IfInAmbiRegs(regno_t regNO) const;
  void AddDependence(DepNode &fromNode, DepNode &toNode, DepType depType);
  DepNode *GenerateDepNode(Insn &insn, MapleVector<DepNode*> &nodes, uint32 &nodeSum, MapleVector<Insn*> &comments);
  void RemoveSelfDeps(Insn &insn);
  void BuildDepsUseReg(Insn &insn, regno_t regNO);
  void BuildDepsDefReg(Insn &insn, regno_t regNO);
  void BuildDepsAmbiInsn(Insn &insn);
  void BuildAmbiInsnDependency(Insn &insn);
  void BuildMayThrowInsnDependency(DepNode &depNode, Insn &insn, const Insn &locInsn);
  void BuildDepsControlAll(Insn &insn, const MapleVector<DepNode*> &nodes);
  void BuildDepsBetweenControlRegAndCall(Insn &insn, bool isDest);
  void BuildDepsLastCallInsn(Insn &insn);
  void BuildInterBlockDefUseDependency(DepNode &curDepNode, regno_t regNO, DepType depType, bool isDef);
  void BuildPredPathDefDependencyDFS(BB &curBB, std::vector<bool> &visited, DepNode &depNode,
                                     regno_t regNO, DepType depType);
  void BuildPredPathUseDependencyDFS(BB &curBB, std::vector<bool> &visited, DepNode &depNode,
                                     regno_t regNO, DepType depType);
  void BuildInterBlockSpecialDataInfoDependency(DepNode &curDepNode, bool needCmp, DepType depType,
                                                DataDepBase::DataFlowInfoType infoType);
  void BuildPredPathSpecialDataInfoDependencyDFS(BB &curBB, std::vector<bool> &visited, bool needCmp, DepNode &depNode,
                                                 DepType depType, DataDepBase::DataFlowInfoType infoType);

  virtual void InitCDGNodeDataInfo(MemPool &mp, MapleAllocator &alloc, CDGNode &cdgNode) = 0;
  virtual bool IsFrameReg(const RegOperand&) const = 0;
  virtual void AnalysisAmbiInsns(BB &bb) = 0;
  virtual void BuildDepsMemBar(Insn &insn) = 0;
  virtual void BuildDepsUseMem(Insn &insn, MemOperand &memOpnd) = 0;
  virtual void BuildDepsDefMem(Insn &insn, MemOperand &memOpnd) = 0;
  virtual void BuildDepsAccessStImmMem(Insn &insn) = 0;
  virtual void BuildCallerSavedDeps(Insn &insn) = 0;
  virtual void BuildDepsDirtyStack(Insn &insn) = 0;
  virtual void BuildDepsUseStack(Insn &insn) = 0;
  virtual void BuildDepsDirtyHeap(Insn &insn) = 0;
  virtual void BuildOpndDependency(Insn &insn) = 0;
  virtual void BuildSpecialInsnDependency(Insn &insn, const MapleVector<DepNode*> &nodes) = 0;
  virtual void BuildSpecialCallDeps(Insn &insn) = 0;
  virtual void BuildAsmInsnDependency(Insn &insn) = 0;
  virtual void BuildInterBlockMemDefUseDependency(DepNode &depNode, bool isMemDef) = 0;
  virtual void BuildPredPathMemDefDependencyDFS(BB &curBB, std::vector<bool> &visited, DepNode &depNode) = 0;
  virtual void BuildPredPathMemUseDependencyDFS(BB &curBB, std::vector<bool> &visited, DepNode &depNode) = 0;
  virtual void DumpNodeStyleInDot(std::ofstream &file, DepNode &depNode) = 0;

 protected:
  MemPool &memPool;
  MapleAllocator alloc;
  CGFunc &cgFunc;
  MAD &mad;
  bool beforeRA = false;
  bool isIntra = false;
  CDGNode *curCDGNode = nullptr;
  CDGRegion *curRegion = nullptr;
};
}

#endif  // MAPLEBE_INCLUDE_CG_DATA_DEP_BASE_H
