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
#ifndef MAPLEBE_INCLUDE_CG_LOOP_H
#define MAPLEBE_INCLUDE_CG_LOOP_H

#include "cg_phase.h"
#include "cgbb.h"
#include "insn.h"
#include "maple_phase.h"

namespace maplebe {
class LoopHierarchy {
 public:
  struct HeadIDCmp {
    bool operator()(const LoopHierarchy *loopHierarchy1, const LoopHierarchy *loopHierarchy2) const {
      CHECK_NULL_FATAL(loopHierarchy1);
      CHECK_NULL_FATAL(loopHierarchy2);
      return (loopHierarchy1->GetHeader()->GetId() < loopHierarchy2->GetHeader()->GetId());
    }
  };

  explicit LoopHierarchy(MemPool &memPool)
      : loopMp(memPool),
        loopAlloc(&memPool),
        otherLoopEntries(loopAlloc.Adapter()),
        loopMembers(loopAlloc.Adapter()),
        backedge(loopAlloc.Adapter()),
        backBBEdges(loopAlloc.Adapter()),
        exits(loopAlloc.Adapter()),
        innerLoops(loopAlloc.Adapter()) {}

  virtual ~LoopHierarchy() = default;

  BB *GetHeader() const {
    return header;
  }
  const MapleSet<BB*, BBIdCmp> &GetLoopMembers() const {
    return loopMembers;
  }
  const MapleSet<BB*, BBIdCmp> &GetBackedge() const {
    return backedge;
  }
  const MapleUnorderedMap<BB*, MapleSet<BB*>*> &GetBackBBEdges() const {
    return backBBEdges;
  }
  MapleSet<BB*, BBIdCmp> &GetBackedgeNonConst() {
    return backedge;
  }
  const MapleSet<BB*, BBIdCmp> &GetExits() const {
    return exits;
  }
  const MapleSet<LoopHierarchy*, HeadIDCmp> &GetInnerLoops() const {
    return innerLoops;
  }
  const LoopHierarchy *GetOuterLoop() const {
    return outerLoop;
  }
  LoopHierarchy *GetPrev() {
    return prev;
  }
  LoopHierarchy *GetNext() {
    return next;
  }

  MapleSet<BB*, BBIdCmp>::iterator EraseLoopMembers(MapleSet<BB*, BBIdCmp>::iterator it) {
    return loopMembers.erase(it);
  }
  void InsertLoopMembers(BB &bb) {
    (void)loopMembers.insert(&bb);
  }
  void InsertBackedge(BB &bb) {
    (void)backedge.insert(&bb);
  }
  void InsertBackBBEdge(BB &backBB, BB &headerBB) {
    auto backIt = backBBEdges.find(&backBB);
    if (backIt == backBBEdges.end()) {
      auto *headBBSPtr = loopMp.New<MapleSet<BB*>>(loopAlloc.Adapter());
      headBBSPtr->insert(&headerBB);
      backBBEdges.emplace(&backBB, headBBSPtr);
    } else {
      backIt->second->insert(&headerBB);
    }
  }
  void InsertExit(BB &bb) {
    (void)exits.insert(&bb);
  }
  void InsertInnerLoops(LoopHierarchy &loop) {
    (void)innerLoops.insert(&loop);
  }
  void SetHeader(BB &bb) {
    header = &bb;
  }
  void SetOuterLoop(LoopHierarchy &loop) {
    outerLoop = &loop;
  }
  void SetPrev(LoopHierarchy *loop) {
    prev = loop;
  }
  void SetNext(LoopHierarchy *loop) {
    next = loop;
  }
  void PrintLoops(const std::string &name) const;
  MapleSet<BB*, BBIdCmp> GetOtherLoopEntries() const {
    return otherLoopEntries;
  }

  void InsertBBToOtherLoopEntries(BB * const &insertBB) {
    (void)otherLoopEntries.insert(insertBB);
  }
  void EraseBBFromOtherLoopEntries(BB * const &eraseBB) {
    (void)otherLoopEntries.erase(eraseBB);
  }

 protected:
  LoopHierarchy *prev = nullptr;
  LoopHierarchy *next = nullptr;

 private:
  MemPool &loopMp;
  MapleAllocator loopAlloc;
  BB *header = nullptr;
  MapleSet<BB*, BBIdCmp> otherLoopEntries;
  MapleSet<BB*, BBIdCmp> loopMembers;
  MapleSet<BB*, BBIdCmp> backedge;
  MapleUnorderedMap<BB*, MapleSet<BB*>*> backBBEdges; // <backBB, headerBBs>
  MapleSet<BB*, BBIdCmp> exits;
  MapleSet<LoopHierarchy*, HeadIDCmp> innerLoops;
  LoopHierarchy *outerLoop = nullptr;
};

class LoopFinder : public AnalysisResult {
 public:
  LoopFinder(CGFunc &func, MemPool &mem)
      : AnalysisResult(&mem),
        cgFunc(&func),
        memPool(&mem),
        loopMemPool(memPool),
        visitedBBs(loopMemPool.Adapter()),
        sortedBBs(loopMemPool.Adapter()),
        dfsBBs(loopMemPool.Adapter()),
        onPathBBs(loopMemPool.Adapter()),
        recurseVisited(loopMemPool.Adapter())
        {}

  ~LoopFinder() override = default;

  void FormLoop(BB &headBB, BB &backBB);
  void SeekBackEdge(BB &bb, MapleList<BB*> succs);
  void SeekCycles();
  void MarkExtraEntryAndEncl();
  bool HasSameHeader(const LoopHierarchy &lp1, const LoopHierarchy &lp2) const;
  void MergeLoops() const;
  void SortLoops();
  void UpdateOuterForInnerLoop(BB *bb, LoopHierarchy *outer);
  void UpdateOuterLoop(const LoopHierarchy *loop);
  void CreateInnerLoop(LoopHierarchy &inner, LoopHierarchy &outer);
  void DetectInnerLoop();
  void UpdateCGFunc() const;
  void FormLoopHierarchy();

 private:
  CGFunc *cgFunc;
  MemPool *memPool;
  MapleAllocator loopMemPool;
  MapleVector<bool> visitedBBs;
  MapleVector<BB*> sortedBBs;
  MapleStack<BB*> dfsBBs;
  MapleVector<bool> onPathBBs;
  MapleVector<bool> recurseVisited;
  LoopHierarchy *loops = nullptr;
};

class CGFuncLoops {
 public:
  explicit CGFuncLoops(MemPool &memPool)
      : loopMemPool(&memPool),
        multiEntries(loopMemPool.Adapter()),
        loopMembers(loopMemPool.Adapter()),
        backedge(loopMemPool.Adapter()),
        backBBEdges(loopMemPool.Adapter()),
        exits(loopMemPool.Adapter()),
        innerLoops(loopMemPool.Adapter()) {}

  ~CGFuncLoops() = default;

  void CheckOverlappingInnerLoops(const MapleVector<CGFuncLoops*> &iLoops,
                                  const MapleVector<BB*> &loopMem) const;
  void CheckLoops() const;
  void PrintLoops(const CGFuncLoops &funcLoop) const;
  bool IsBBLoopMember(const BB *bb) const;
  bool IsBackEdge(const BB &fromBB, const BB &toBB) const {
    auto backIt = backBBEdges.find(fromBB.GetId());
    if (backIt == backBBEdges.end()) {
      return false;
    }
    for (auto headId : backIt->second) {
      if (toBB.GetId() == headId) {
        return true;
      }
    }
    return false;
  }

  const BB *GetHeader() const {
    return header;
  }
  BB *GetHeader() {
    return header;
  }
  const MapleVector<BB*> &GetMultiEntries() const {
    return multiEntries;
  }
  const MapleVector<BB*> &GetLoopMembers() const {
    return loopMembers;
  }
  const MapleVector<BB*> &GetBackedge() const {
    return backedge;
  }
  const MapleUnorderedMap<uint32, MapleSet<uint32>> &GetBackBBEdges() const {
    return backBBEdges;
  }
  const MapleVector<BB*> &GetExits() const {
    return exits;
  }
  const MapleVector<CGFuncLoops*> &GetInnerLoops() const {
    return innerLoops;
  }
  const CGFuncLoops *GetOuterLoop() const {
    return outerLoop;
  }
  uint32 GetLoopLevel() const {
    return loopLevel;
  }
  void AddMultiEntries(BB &bb) {
    multiEntries.emplace_back(&bb);
  }
  void AddLoopMembers(BB &bb) {
    loopMembers.emplace_back(&bb);
  }
  void AddBackedge(BB &bb) {
    backedge.emplace_back(&bb);
  }
  void AddBackBBEdge(const BB &backBB, const BB &headerBB) {
    auto backIt = backBBEdges.find(backBB.GetId());
    if (backIt == backBBEdges.end()) {
      MapleSet<uint32> toBBs(loopMemPool.Adapter());
      toBBs.insert(headerBB.GetId());
      backBBEdges.emplace(backBB.GetId(), toBBs);
    } else {
      backIt->second.insert(headerBB.GetId());
    }
  }
  void AddExit(BB &bb) {
    exits.emplace_back(&bb);
  }
  void AddInnerLoops(CGFuncLoops &loop) {
    innerLoops.emplace_back(&loop);
  }
  void SetHeader(BB &bb) {
    header = &bb;
  }
  void SetOuterLoop(CGFuncLoops &loop) {
    outerLoop = &loop;
  }
  void SetLoopLevel(uint32 val) {
    loopLevel = val;
  }

 private:
  MapleAllocator loopMemPool;
  BB *header = nullptr;
  MapleVector<BB*> multiEntries;
  MapleVector<BB*> loopMembers;
  MapleVector<BB*> backedge;
  MapleUnorderedMap<uint32, MapleSet<uint32>> backBBEdges; // <backBBId, headerBBIds> (<fromBBId, toBBIds>)
  MapleVector<BB*> exits;
  MapleVector<CGFuncLoops*> innerLoops;
  CGFuncLoops *outerLoop = nullptr;
  uint32 loopLevel = 0;
};

struct CGFuncLoopCmp {
  bool operator()(const CGFuncLoops *lhs, const CGFuncLoops *rhs) const {
    CHECK_NULL_FATAL(lhs);
    CHECK_NULL_FATAL(rhs);
    return lhs->GetHeader()->GetId() < rhs->GetHeader()->GetId();
  }
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgLoopAnalysis, maplebe::CGFunc);
MAPLE_FUNC_PHASE_DECLARE_END
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_LOOP_H */
