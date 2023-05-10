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
#ifndef MAPLEBE_CG_INCLUDE_CG_PGO_USE_H
#define MAPLEBE_CG_INCLUDE_CG_PGO_USE_H
#include "cgfunc.h"
#include "instrument.h"
#include "cg_dominance.h"
namespace maplebe {
class BBChain {
 public:
  using iterator = MapleVector<BB*>::iterator;
  using const_iterator = MapleVector<BB*>::const_iterator;
  BBChain(MapleAllocator &alloc, MapleVector<BBChain*> &bb2chain, BB &bb, uint32 inputId)
      : id(inputId),
        bbVec(1, &bb, alloc.Adapter()),
        bb2chain(bb2chain) {
    bb2chain[bb.GetId()] = this;
  }

  iterator begin() {
    return bbVec.begin();
  }
  const_iterator begin() const {
    return bbVec.begin();
  }
  iterator end() {
    return bbVec.end();
  }
  const_iterator end() const {
    return bbVec.end();
  }

  bool empty() const {
    return bbVec.empty();
  }

  size_t size() const {
    return bbVec.size();
  }

  uint32 GetId() const {
    return id;
  }

  BB *GetHeader() {
    CHECK_FATAL(!bbVec.empty(), "cannot get header from a empty bb chain");
    return bbVec.front();
  }
  BB *GetTail() {
    CHECK_FATAL(!bbVec.empty(), "cannot get tail from a empty bb chain");
    return bbVec.back();
  }

  bool FindBB(BB &bb) {
    auto fIt = std::find(bbVec.begin(), bbVec.end(), &bb);
    return fIt != bbVec.end();
  }

  // update unlaidPredCnt if needed. The chain is ready to layout only if unlaidPredCnt == 0
  bool IsReadyToLayout(const MapleVector<bool> *context) {
    MayRecalculateUnlaidPredCnt(context);
    return (unlaidPredCnt == 0);
  }

  // Merge src chain to this one
  void MergeFrom(BBChain *srcChain);

  void UpdateSuccChainBeforeMerged(const BBChain &destChain, const MapleVector<bool> *context,
                                   MapleSet<BBChain*> &readyToLayoutChains);

  void Dump() const {
    LogInfo::MapleLogger() << "bb chain with " << bbVec.size() << " blocks: ";
    for (BB *bb : bbVec) {
      LogInfo::MapleLogger() << bb->GetId() << " ";
    }
    LogInfo::MapleLogger() << std::endl;
  }

  void DumpOneLine() const {
    for (BB *bb : bbVec) {
      LogInfo::MapleLogger() << bb->GetId() << " ";
    }
  }

 private:
  void MayRecalculateUnlaidPredCnt(const MapleVector<bool> *context);

  uint32 id = 0;
  MapleVector<BB*> bbVec;
  MapleVector<BBChain*> &bb2chain;
  uint32 unlaidPredCnt = 0;   // how many predecessors are not laid out
  bool isCacheValid = false;  // whether unlaidPredCnt is trustable
};

class CGProfUse {
 public:
  CGProfUse(CGFunc &curF, MemPool &mp, DomAnalysis *dom, const MapleSet<uint32> &newbbinsplit)
      : f(&curF),
        mp(&mp),
        puAlloc(&mp),
        domInfo(dom),
        bbSplit(newbbinsplit),
        instrumenter(mp),
        layoutBBs(puAlloc.Adapter()),
        laidOut(puAlloc.Adapter()) {}

  bool ApplyPGOData();
  void LayoutBBwithProfile();
 protected:
  CGFunc *f;
  MemPool *mp;
  MapleAllocator puAlloc;
  DomAnalysis *domInfo = nullptr;
  MapleSet<uint32> bbSplit;
 private:
  PGOInstrumentTemplate<maplebe::BB, maple::BBUseEdge<maplebe::BB>> instrumenter;
  std::unordered_map<uint32, BBUseInfo<maplebe::BB>*> bbProfileInfo;

  void ApplyOnBB();
  // verify profile data according to 1. cfg hash 2. bb counter number
  bool VerifyProfileData(const std::vector<maplebe::BB *> &iBBs, LiteProfile::BBInfo &bbInfo);
  void InitBBEdgeInfo();
  /* compute all edge freq in the cfg without consider exception */
  void ComputeEdgeFreq();
  /* If all input edges or output edges determined, caculate BB freq */
  void ComputeBBFreq(BBUseInfo<maplebe::BB> &bbInfo, bool &change);
  uint64 SumEdgesCount(const MapleVector<BBUseEdge<maplebe::BB>*> &edges) const;
  BBUseInfo<maplebe::BB> *GetOrCreateBBUseInfo(const maplebe::BB &bb, bool notCreate = false);
  void SetEdgeCount(maple::BBUseEdge<maplebe::BB> &e, size_t count);

  void AddBBProf(BB &bb);
  void AddBB(BB &bb);
  void ReTargetSuccBB(BB &bb, BB &fallthru);
  void ChangeToFallthruFromGoto(BB &bb) const;
  LabelIdx GetOrCreateBBLabIdx(BB &bb) const;

  bool debugChainLayout = false;
  MapleVector<BB*> layoutBBs;  // gives the determined layout order
  MapleVector<bool> laidOut;   // indexed by bbid to tell if has been laid out
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgPgoUse, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
}
#endif // MAPLEBE_CG_INCLUDE_CG_PGO_USE_H