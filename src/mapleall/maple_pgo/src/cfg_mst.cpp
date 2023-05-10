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

#include "cfg_mst.h"
#include "cgbb.h"
#include "instrument.h"
namespace maple {
template <class Edge, class BB>
void CFGMST<Edge, BB>::BuildEdges(BB *commonEntry, BB *commonExit) {
  for (auto *curbb = commonEntry; curbb != nullptr; curbb = curbb->GetNext()) {
    bbGroups[curbb->GetId()] = curbb->GetId();
    if (curbb == commonExit) {
      continue;
    }
    totalBB++;
    for (auto *succBB : curbb->GetSuccs()) {
      // exitBB incoming edge allocate high weight
      if (succBB->GetKind() == BB::BBKind::kBBReturn) {
        AddEdge(curbb, succBB, exitEdgeWeight);
        continue;
      }
      if (IsCritialEdge(curbb, succBB)) {
        AddEdge(curbb, succBB, criticalEdgeWeight, true);
        continue;
      }
      AddEdge(curbb, succBB, normalEdgeWeight);
    }
  }

  for (BB *bb : commonExit->GetPreds()) {
    AddEdge(bb, commonExit, fakeExitEdgeWeight, false, true);
  }
  bbGroups[commonExit->GetId()] = commonExit->GetId();
  // insert fake edge to keep consistent
  AddEdge(commonExit, commonEntry, UINT64_MAX, false, true);
}

template <class Edge, class BB>
void CFGMST<Edge, BB>::ComputeMST(BB *commonEntry, BB *commonExit) {
  BuildEdges(commonEntry, commonExit);
  SortEdges();
  /* only one edge means only one bb */
  if (allEdges.size() == 1) {
    LogInfo::MapleLogger() << "only one edge find " << std::endl;
    return;
  }
  // first,put all critial edge,with destBB is eh-handle
  // in mst,because current doesn't support split that edge
  for (auto &e : allEdges) {
    if (UnionGroups(e->GetSrcBB()->GetId(), e->GetDestBB()->GetId())) {
      e->SetInMST();
    }
  }
}

template <class Edge, class BB>
void CFGMST<Edge, BB>::AddEdge(BB *src, BB *dest, uint64 w, bool isCritical, bool isFake) {
  if (src == nullptr || dest == nullptr) {
    return;
  }
  bool found = false;
  for (auto &edge : allEdges) {
    if (edge->GetSrcBB() == src && edge->GetDestBB() == dest) {
      uint64 weight = edge->GetWeight();
      weight++;
      edge->SetWeight(weight);
      found = true;
    }
  }
  if (!found) {
    (void)allEdges.emplace_back(mp->New<Edge>(src, dest, w, isCritical, isFake));
  }
}

template <class Edge, class BB>
void CFGMST<Edge, BB>::SortEdges() {
  std::stable_sort(allEdges.begin(), allEdges.end(),
                   [](const Edge *edge1, const Edge *edge2) { return edge1->GetWeight() > edge2->GetWeight(); });
}

template <class Edge, class BB>
uint32 CFGMST<Edge, BB>::FindGroup(uint32 bbId) {
  CHECK_FATAL(bbGroups.count(bbId) != 0, "unRegister bb");
  if (bbGroups[bbId] != bbId) {
    bbGroups[bbId] = FindGroup(bbGroups[bbId]);
  }
  return bbGroups[bbId];
}

template <class Edge, class BB>
bool CFGMST<Edge, BB>::UnionGroups(uint32 srcId, uint32 destId) {
  uint32 srcGroupId = FindGroup(srcId);
  uint32 destGroupId = FindGroup(destId);
  if (srcGroupId == destGroupId) {
    return false;
  }
  bbGroups[srcGroupId] = destGroupId;
  return true;
}
template <class Edge, class BB>
void CFGMST<Edge, BB>::DumpEdgesInfo() const {
  for (auto &edge : allEdges) {
    BB *src = edge->GetSrcBB();
    BB *dest = edge->GetDestBB();
    LogInfo::MapleLogger() << "BB" << src->GetId() << "->"
                           << "BB" << dest->GetId() << " weight " << edge->GetWeight();
    if (edge->IsInMST()) {
      LogInfo::MapleLogger() << " in Mst\n";
    } else {
      LogInfo::MapleLogger() << " not in  Mst\n";
    }
  }
}

template class CFGMST<maple::BBEdge<maplebe::BB>, maplebe::BB>;
template class CFGMST<maple::BBUseEdge<maplebe::BB>, maplebe::BB>;
}
