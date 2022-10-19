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
      : cgFunc(f), interAlloc(&memPool), ddb(dataDepBase),
        readyNodes(interAlloc.Adapter()), restNodes(interAlloc.Adapter()) {}
  virtual ~InterDataDepAnalysis() = default;

  void AddReadyNode(CDGNode *node) {
    (void)readyNodes.emplace_back(node);
  }
  void RemoveReadyNode(CDGNode *node) {
    auto it = std::find(readyNodes.begin(), readyNodes.end(), node);
    if (it != readyNodes.end()) {
      (void)readyNodes.erase(it);
    }
  }
  void InitRestNodes(MapleVector<CDGNode*> &nodes) {
    restNodes = nodes;
  }
  void RemoveRestNode(CDGNode *node) {
    auto it = std::find(restNodes.begin(), restNodes.end(), node);
    if (it != restNodes.end()) {
      (void)restNodes.erase(it);
    }
  }

  void Run(CDGRegion &region, MapleVector<DepNode*> &dataNodes);
  void GlobalInit(MapleVector<DepNode*> &dataNodes);
  void LocalInit(BB &bb, CDGNode &cdgNode, MapleVector<DepNode*> &dataNodes, std::size_t idx);
  void GenerateInterDDGDot(MapleVector<DepNode*> &dataNodes);

 protected:
  void ComputeTopologicalOrderInRegion(CDGRegion &region);

 private:
  CGFunc &cgFunc;
  MapleAllocator interAlloc;
  DataDepBase &ddb;
  MapleVector<CDGNode*> readyNodes;
  MapleVector<CDGNode*> restNodes;
};
} /* namespace maplebe */

#endif  // MAPLEBE_INCLUDE_CG_DATA_DEP_ANALYSIS_H
