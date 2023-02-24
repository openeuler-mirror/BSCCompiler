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

/*
 * Build intra/inter block data dependence graph.
 * 1: Build data dependence nodes
 * 2: Build edges between dependence nodes. Edges are:
 *   2.1) True dependence
 *   2.2) Anti dependence
 *   2.3) Output dependence
 *   2.4) Barrier dependence
 */
#ifndef MAPLEBE_INCLUDE_CG_DATA_DEP_ANALYSIS_H
#define MAPLEBE_INCLUDE_CG_DATA_DEP_ANALYSIS_H

#include "data_dep_base.h"
#include "mempool.h"
#include "cg_cdg.h"

namespace maplebe {
constexpr uint32 kMaxDumpRegionNodeNum = 6;

/* Analyze IntraBlock Data Dependence */
class IntraDataDepAnalysis {
 public:
  IntraDataDepAnalysis(MemPool &mp, CGFunc &f, DataDepBase &dataDepBase)
      : intraMp(mp), intraAlloc(&mp), cgFunc(f), ddb(dataDepBase) {}
  virtual ~IntraDataDepAnalysis() = default;

  void Run(BB &bb, MapleVector<DepNode*> &dataNodes);
  void InitCurNodeInfo(MemPool &tmpMp, MapleAllocator &tmpAlloc, BB &bb, MapleVector<DepNode*> &dataNodes);
  void ClearCurNodeInfo(MemPool *tmpMp, MapleAllocator *tmpAlloc);
  void AddEndSeparatorNode(BB &bb, MapleVector<DepNode*> &nodes);

 private:
  MemPool &intraMp;
  MapleAllocator intraAlloc;
  CGFunc &cgFunc;
  DataDepBase &ddb;
};

/* Analyze InterBlock Data Dependence */
class InterDataDepAnalysis {
 public:
  InterDataDepAnalysis(CGFunc &f, MemPool &memPool, DataDepBase &dataDepBase)
      : cgFunc(f), interMp(memPool), interAlloc(&memPool), ddb(dataDepBase) {}
  virtual ~InterDataDepAnalysis() = default;

  void Run(CDGRegion &region);
  void GenerateDataDepGraphDotOfRegion(CDGRegion &region);

 protected:
  void InitInfoInRegion(MemPool &regionMp, MapleAllocator &regionAlloc, CDGRegion &region);
  void InitInfoInCDGNode(MemPool &regionMp, MapleAllocator &regionAlloc, BB &bb, CDGNode &cdgNode);
  void ClearInfoInRegion(MemPool *regionMp, MapleAllocator *regionAlloc, CDGRegion &region);
  void AddBeginSeparatorNode(CDGNode *rootNode);
  void SeparateDependenceGraph(CDGRegion &region, CDGNode &cdgNode);
  void BuildDepsForNewSeparator(CDGRegion &region, CDGNode &cdgNode, DepNode &newSepNode);
  void BuildDepsForPrevSeparator(CDGNode &cdgNode, DepNode &depNode, CDGRegion &curRegion);
  void BuildSpecialInsnDependency(Insn &insn, CDGNode &cdgNode, CDGRegion &region, MapleAllocator &alloc);
  void UpdateRegUseAndDef(Insn &insn, const DepNode &depNode, CDGNode &cdgNode);
  void AddEndSeparatorNode(CDGRegion &region, CDGNode &cdgNode);
  void UpdateReadyNodesInfo(MapleAllocator &regionAlloc, CDGRegion &region, CDGNode &cdgNode, const CDGNode &root);

 private:
  CGFunc &cgFunc;
  MemPool &interMp;
  MapleAllocator interAlloc;
  DataDepBase &ddb;
};
} /* namespace maplebe */

#endif  // MAPLEBE_INCLUDE_CG_DATA_DEP_ANALYSIS_H
