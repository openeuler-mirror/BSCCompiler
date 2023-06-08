/*
 * Copyright (c) [2022] Futurewei Technologies, Inc. All rights reverved.
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

#ifndef MAPLEBE_INCLUDE_CG_PROFUSE_H
#define MAPLEBE_INCLUDE_CG_PROFUSE_H

#include "cgfunc.h"

namespace maplebe {
class CgProfUse {
 public:

  struct Edge {
    BB *src;
    BB *dst;
    Edge *next = nullptr;  // the edge with the same src
    FreqType frequency = -1;
    bool status = false;   // True value indicates the edge's freq is determined
    Edge(BB *bb1, BB *bb2) : src(bb1), dst(bb2) {}
  };

  CgProfUse(CGFunc &f, MemPool &mp)
      : cgFunc(&f), memPool(&mp), alloc(&mp), allEdges(alloc.Adapter()),
        bb2InEdges(alloc.Adapter()),
        bb2OutEdges(alloc.Adapter()) {}

  virtual ~CgProfUse() {
    memPool = nullptr;
  }

  void SetupProf();

  MapleSet<Edge*> &GetAllEdges() {
    return allEdges;
  }

  Edge *CreateEdge(BB *src, BB *dst) {
    Edge *e = memPool->New<Edge>(src, dst);
    allEdges.insert(e);
    return e;
  }

  void SetupBB2Edges() {
    for (Edge *e : allEdges) {
      auto it = bb2InEdges.find(e->dst);
      if (it == bb2InEdges.end()) {
        MapleVector<Edge*> edgeVec(alloc.Adapter());
        edgeVec.push_back(e);
        bb2InEdges.emplace(e->dst, edgeVec);
      } else {
        MapleVector<Edge*> &edgeVec = it->second;
        edgeVec.push_back(e);
      }

      it = bb2OutEdges.find(e->src);
      if (it == bb2OutEdges.end()) {
        MapleVector<Edge*> edgeVec(alloc.Adapter());
        edgeVec.push_back(e);
        bb2OutEdges.emplace(e->src, edgeVec);
      } else {
        MapleVector<Edge*> &edgeVec = it->second;
        edgeVec.push_back(e);
      }
    }
  }

  void SetSuccsFreq() {
    for (Edge *e : allEdges) {
      e->src->SetEdgeProfFreq(e->dst, e->frequency);
    }
  }

  void InferEdgeFreq() {
    std::queue <BB*> bbQueue;
    FOR_ALL_BB(bb, cgFunc) {
      bbQueue.push(bb);
    }

    while (!bbQueue.empty()) {
      BB *bb = bbQueue.front();
      bbQueue.pop();

      // Type 1 inference
      uint32 knownEdges1 = 0;
      FreqType freqSum1 = 0;
      Edge *unknownEdge1 = nullptr;
      MapleMap<BB*, MapleVector<Edge*>>::iterator iit = bb2InEdges.find(bb);
      if ((iit != bb2InEdges.end()) && (iit->second.size() != 0)) {
        for (Edge *e : iit->second) {
          if (e->status) {
            knownEdges1++;
            freqSum1 += e->frequency;
          } else {
            unknownEdge1 = e;
          }
        }
        if ((knownEdges1 == iit->second.size() - 1) && (bb->GetProfFreq() != 0)) {
          if (bb->GetProfFreq() >= freqSum1) {
            CHECK_NULL_FATAL(unknownEdge1);
            unknownEdge1->status = true;
            unknownEdge1->frequency = bb->GetProfFreq() - freqSum1;
            bbQueue.push(unknownEdge1->src);
            bbQueue.push(unknownEdge1->dst);
          }
        }
      }

      // Type 2 inference
      uint32 knownEdges2 = 0;
      FreqType freqSum2 = 0;
      Edge* unknownEdge2 = nullptr;
      MapleMap<BB*, MapleVector<Edge*>>::iterator oit = bb2OutEdges.find(bb);
      if ((oit != bb2OutEdges.end()) && (oit->second.size() != 0)) {
        for (Edge *e : oit->second) {
          if (e->status) {
            knownEdges2++;
            freqSum2 += e->frequency;
          } else {
            unknownEdge2 = e;
          }
        }
        if ((knownEdges2 == oit->second.size() - 1) && (bb->GetProfFreq() != 0)) {
          if (bb->GetProfFreq() >= freqSum2) {
            unknownEdge2->status = true;
            unknownEdge2->frequency = bb->GetProfFreq() - freqSum2;
            bbQueue.push(unknownEdge2->src);
            bbQueue.push(unknownEdge2->dst);
          }
        }
      }

      // Type 3 inference
      if ((unknownEdge1 != nullptr) && (!unknownEdge1->status) &&
          (iit != bb2InEdges.end()) && (iit->second.size() != 0) &&
          (knownEdges1 == iit->second.size() - 1) &&
          (knownEdges2 > 0) && (unknownEdge2 == nullptr)) {
        if (freqSum2 >= freqSum1) {
          unknownEdge1->status = true;
          unknownEdge1->frequency = freqSum2 - freqSum1;
          bbQueue.push(unknownEdge1->src);
          bbQueue.push(unknownEdge1->dst);
        }
      }

      if ((unknownEdge2 != nullptr) && (!unknownEdge2->status) &&
          (oit != bb2InEdges.end()) && (oit->second.size() != 0) &&
          (knownEdges2 == oit->second.size() - 1) &&
          (knownEdges1 > 0) && (unknownEdge1 == nullptr)) {
        if (freqSum1 >= freqSum2) {
          unknownEdge2->status = true;
          unknownEdge2->frequency = freqSum1 - freqSum2;
          bbQueue.push(unknownEdge2->src);
          bbQueue.push(unknownEdge2->dst);
        }
      }
    }
  }

 protected:
  CGFunc *cgFunc;
  MemPool *memPool;
  MapleAllocator alloc;
  MapleSet<Edge*> allEdges;
  MapleMap<BB*, MapleVector<Edge*>> bb2InEdges;
  MapleMap<BB*, MapleVector<Edge*>> bb2OutEdges;
};

MAPLE_FUNC_PHASE_DECLARE(CGProfUse, maplebe::CGFunc)
}
#endif /* MAPLEBE_INCLUDE_CG_PROFUSE_H */
