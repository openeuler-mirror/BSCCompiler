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
#ifndef MAPLE_PGO_INCLUDE_CFG_MST_H
#define MAPLE_PGO_INCLUDE_CFG_MST_H

#include "types_def.h"
#include "mempool_allocator.h"

namespace maple {
template <class Edge, class BB>
class CFGMST {
 public:
  explicit CFGMST(MemPool &mp) : mp(&mp), alloc(&mp), allEdges(alloc.Adapter()), bbGroups(alloc.Adapter()) {}
  virtual ~CFGMST() = default;
  void ComputeMST(BB *commonEntry, BB *commonExit);
  void BuildEdges(BB *commonEntry, BB *commonExit);
  void SortEdges();
  void AddEdge(BB *src, BB *dest, uint64 w, bool isCritical = false, bool isFake = false);
  bool IsCritialEdge(const BB *src, const BB *dest) const {
    return src->GetSuccs().size() > 1 && dest->GetPreds().size() > 1;
  }
  const MapleVector<Edge*> &GetAllEdges() const {
    return allEdges;
  }

  size_t GetAllEdgesSize() const {
    return allEdges.size();
  }

  uint32 GetAllBBs() const {
    return totalBB;
  }

  void GetInstrumentEdges(std::vector<Edge*> &instrumentEdges) const {
    for (const auto &e : allEdges) {
      if (!e->IsInMST()) {
        instrumentEdges.push_back(e);
      }
    }
  }
  void DumpEdgesInfo() const;
 private:
  uint32 FindGroup(uint32 bbId);
  bool UnionGroups(uint32 srcId, uint32 destId);
  static constexpr int normalEdgeWeight = 2;
  static constexpr int exitEdgeWeight = 3;
  static constexpr int fakeExitEdgeWeight = 4;
  static constexpr int criticalEdgeWeight = 4;
  MemPool *mp;
  MapleAllocator alloc;
  MapleVector<Edge*> allEdges;
  MapleMap<uint32, uint32> bbGroups; // bbId - gourpId
  uint32 totalBB = 0;
};
} /* namespace maple */
#endif // MAPLE_PGO_INCLUDE_CFG_MST_H
