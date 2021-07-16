/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEBE_INCLUDE_CG_DOM_H
#define MAPLEBE_INCLUDE_CG_DOM_H

#include "cg_phase.h"
#include "insn.h"
#include "cgbb.h"
#include "datainfo.h"

namespace maplebe {
class DominanceBase : public AnalysisResult {
 public:
  DominanceBase(CGFunc &func, MemPool &memPool, MemPool &tmpPool,  MapleVector<BB *> &bbVec, BB &commonEntryBB,
                BB &commonExitBB)
      : AnalysisResult(&memPool),
        domAllocator(&memPool),
        tmpAllocator(&tmpPool),
        bbVec(bbVec),
        cgFunc(func),
        commonEntryBB(commonEntryBB),
        commonExitBB(commonExitBB) {}

  ~DominanceBase() override = default;

  BB &GetCommonEntryBB() const {
    return commonEntryBB;
  }

  BB &GetCommonExitBB() const {
    return commonExitBB;
  }

protected:
  bool CommonEntryBBIsPred(const BB &bb) const;
  MapleAllocator domAllocator;  // stores the analysis results
  MapleAllocator tmpAllocator;  // can be freed after dominator computation
  MapleVector<BB *> &bbVec;
  CGFunc &cgFunc;
  BB &commonEntryBB;
  BB &commonExitBB;
};

class DomAnalysis : public DominanceBase {
 public:
  DomAnalysis(CGFunc &func, MemPool &memPool, MemPool &tmpPool,  MapleVector<BB *> &bbVec, BB &commonEntryBB,
              BB &commonExitBB)
      : DominanceBase(func, memPool, tmpPool, bbVec, commonEntryBB, commonExitBB),
        postOrderIDVec(bbVec.size() + 1, -1, tmpAllocator.Adapter()),
        reversePostOrder(tmpAllocator.Adapter()),
        doms(bbVec.size() + 1, nullptr, domAllocator.Adapter()),
        domFrontier(bbVec.size() + 1, MapleVector<uint32>(domAllocator.Adapter()), domAllocator.Adapter()),
        domChildren(bbVec.size() + 1, MapleVector<uint32>(domAllocator.Adapter()), domAllocator.Adapter()),
        iterDomFrontier(bbVec.size() + 1, MapleSet<uint32>(domAllocator.Adapter()), domAllocator.Adapter()),
        dtPreOrder(bbVec.size() + 1, 0, domAllocator.Adapter()),
        dtDfn(bbVec.size() + 1, -1, domAllocator.Adapter()),
        dtDfnOut(bbVec.size() + 1, -1, domAllocator.Adapter()) {}
  ~DomAnalysis() override = default;

  void Compute();
  void Dump();

  void GenPostOrderID();
  void ComputeDominance();
  void ComputeDomFrontiers();
  void ComputeDomChildren();
  void GetIterDomFrontier(const BB *bb, MapleSet<uint32> *dfset, uint32 bbidMarker, std::vector<bool> &visitedMap);
  void ComputeIterDomFrontiers();
  uint32 ComputeDtPreorder(const BB &bb, uint32 &num);
  bool Dominate(const BB &bb1, const BB &bb2);  // true if bb1 dominates bb2

  MapleVector<BB *> &GetReversePostOrder() {
    return reversePostOrder;
  }

  MapleVector<uint32> &GetDtPreOrder() {
    return dtPreOrder;
  }

  uint32 GetDtPreOrderItem(size_t idx) const {
    return dtPreOrder[idx];
  }

  size_t GetDtPreOrderSize() const {
    return dtPreOrder.size();
  }

  uint32 GetDtDfnItem(size_t idx) const {
    return dtDfn[idx];
  }

  size_t GetDtDfnSize() const {
    return dtDfn.size();
  }

  BB *GetDom(uint32 id) {
    ASSERT(id < doms.size(), "bbid out of range");
    return doms[id];
  }
  void SetDom(uint32 id, BB *bb) {
    ASSERT(id < doms.size(), "bbid out of range");
    doms[id] = bb;
  }
  size_t GetDomsSize() const {
    return doms.size();
  }

  auto &GetDomFrontier(size_t idx) {
    return domFrontier[idx];
  }
  bool HasDomFrontier(uint32 id, uint32 frontier) const {
    return std::find(domFrontier[id].begin(), domFrontier[id].end(), frontier) != domFrontier[id].end();
  }

  size_t GetDomFrontierSize() const {
    return domFrontier.size();
  }

  auto &GetDomChildren() {
    return domChildren;
  }

  auto &GetDomChildren(size_t idx) {
    return domChildren[idx];
  }

  auto &GetIdomFrontier(uint32 idx) {
    return iterDomFrontier[idx];
  }

  size_t GetDomChildrenSize() const {
    return domChildren.size();
  }

 private:
  void PostOrderWalk(const BB &bb, int32 &pid, MapleVector<bool> &visitedMap);
  BB *Intersect(BB &bb1, const BB &bb2);

  MapleVector<int32> postOrderIDVec;   // index is bb id
  MapleVector<BB *> reversePostOrder;  // an ordering of the BB in reverse postorder
  MapleVector<BB *> doms;              // index is bb id; immediate dominator for each BB
  MapleVector<MapleVector<uint32>> domFrontier;   // index is bb id
  MapleVector<MapleVector<uint32>> domChildren;   // index is bb id; for dom tree
  MapleVector<MapleSet<uint32>> iterDomFrontier;
  MapleVector<uint32> dtPreOrder;              // ordering of the BBs in a preorder traversal of the dominator tree
  MapleVector<uint32> dtDfn;                 // gives position of each BB in dt_preorder
  MapleVector<uint32> dtDfnOut;                 // max position of all nodes in the sub tree of each BB in dt_preorder
};

class PostDomAnalysis : public DominanceBase {
public:
  PostDomAnalysis(CGFunc &func, MemPool &memPool, MemPool &tmpPool,  MapleVector<BB *> &bbVec, BB &commonEntryBB,
                  BB &commonExitBB)
      : DominanceBase(func, memPool, tmpPool, bbVec, commonEntryBB, commonExitBB),
        pdomPostOrderIDVec(bbVec.size() + 1, -1, tmpAllocator.Adapter()),
        pdomReversePostOrder(tmpAllocator.Adapter()),
        pdoms(bbVec.size() + 1, nullptr, domAllocator.Adapter()),
        pdomFrontier(bbVec.size() + 1, MapleVector<uint32>(domAllocator.Adapter()), domAllocator.Adapter()),
        pdomChildren(bbVec.size() + 1, MapleVector<uint32>(domAllocator.Adapter()), domAllocator.Adapter()),
        pdtPreOrder(bbVec.size() + 1, 0, domAllocator.Adapter()),
        pdtDfn(bbVec.size() + 1, -1, domAllocator.Adapter()),
        pdtDfnOut(bbVec.size() + 1, -1, domAllocator.Adapter()) {}

  ~PostDomAnalysis() override = default;
  void Compute();
  void PdomGenPostOrderID();
  void ComputePostDominance();
  void ComputePdomFrontiers();
  void ComputePdomChildren();
  uint32 ComputePdtPreorder(const BB &bb, uint32 &num);
  bool PostDominate(const BB &bb1, const BB &bb2);  // true if bb1 postdominates bb2
  void Dump();

  auto &GetPdomFrontierItem(size_t idx) {
    return pdomFrontier[idx];
  }

  size_t GetPdomFrontierSize() const {
    return pdomFrontier.size();
  }

  auto &GetPdomChildrenItem(size_t idx) {
    return pdomChildren[idx];
  }

  void ResizePdtPreOrder(size_t n) {
    pdtPreOrder.resize(n);
  }

  uint32 GetPdtPreOrderItem(size_t idx) const {
    return pdtPreOrder[idx];
  }

  uint32 GetPdtDfnItem(size_t idx) const {
    return pdtDfn[idx];
  }

  int32 GetPdomPostOrderIDVec(size_t idx) const {
    return pdomPostOrderIDVec[idx];
  }

  BB *GetPdomReversePostOrder(size_t idx) {
    return pdomReversePostOrder[idx];
  }

  MapleVector<BB *> &GetPdomReversePostOrder() {
    return pdomReversePostOrder;
  }

  size_t GetPdomReversePostOrderSize() const {
    return pdomReversePostOrder.size();
  }

  bool HasPdomFrontier(uint32 id, uint32 frontier) const {
    return std::find(pdomFrontier[id].begin(), pdomFrontier[id].end(), frontier) != pdomFrontier[id].end();
  }

  BB *GetPdom(uint32 id) {
    ASSERT(id < pdoms.size(), "bbid out of range");
    return pdoms[id];
  }
  void SetPdom(uint32 id, BB *bb) {
    ASSERT(id < pdoms.size(), "bbid out of range");
    pdoms[id] = bb;
  }

private:
  void PdomPostOrderWalk(const BB &bb, int32 &pid, MapleVector<bool> &visitedMap);
  BB *PdomIntersect(BB &bb1, const BB &bb2);

  MapleVector<int32> pdomPostOrderIDVec;     // index is bb id
  MapleVector<BB *> pdomReversePostOrder;    // an ordering of the BB in reverse postorder
  MapleVector<BB *> pdoms;                   // index is bb id; immediate dominator for each BB
  MapleVector<MapleVector<uint32>> pdomFrontier;  // index is bb id
  MapleVector<MapleVector<uint32>> pdomChildren;  // index is bb id; for pdom tree
  MapleVector<uint32> pdtPreOrder;             // ordering of the BBs in a preorder traversal of the post-dominator tree
  MapleVector<uint32> pdtDfn;                // gives position of each BB in pdt_preorder
  MapleVector<uint32> pdtDfnOut;                 // max position of all nodes in the sub tree of each BB in pdt_preorder
};

CGFUNCPHASE(CgDoDomAnalysis, "domanalysis")
CGFUNCPHASE(CgDoPostDomAnalysis, "postdomanalysis")
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_DOM_H */
