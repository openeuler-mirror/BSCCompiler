/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_ME_INCLUDE_ME_BB_LAYOUT_H
#define MAPLE_ME_INCLUDE_ME_BB_LAYOUT_H
#include "me_function.h"
#include "me_pgo_instrument.h"
#include "maple_phase_manager.h"
#include "me_dominance.h"
#include "me_loop_analysis.h"

namespace maple {
class BBChain {
public:
  using iterator = MapleVector<BB*>::iterator;
  using const_iterator = MapleVector<BB*>::const_iterator;
  BBChain(MapleAllocator &alloc, MapleVector<BBChain*> &bb2chain, BB *bb)
      : bbVec(1, bb, alloc.Adapter()), bb2chain(bb2chain) {
    bb2chain[bb->GetBBId()] = this;
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

  BB *GetHeader() {
    CHECK_FATAL(!bbVec.empty(), "cannot get header from a empty bb chain");
    return bbVec.front();
  }
  BB *GetTail() {
    CHECK_FATAL(!bbVec.empty(), "cannot get tail from a empty bb chain");
    return bbVec.back();
  }

  // update unlaidPredCnt if needed. The chain is ready to layout only if unlaidPredCnt == 0
  bool IsReadyToLayout(const MapleVector<bool> *context) {
    MayRecalculateUnlaidPredCnt(context);
    return (unlaidPredCnt == 0);
  }

  // Merge src chain to this one
  void MergeFrom(BBChain *srcChain) {
    CHECK_FATAL(this != srcChain, "merge same chain?");
    ASSERT_NOT_NULL(srcChain);
    if (srcChain->empty()) {
      return;
    }
    for (BB *bb : *srcChain) {
      bbVec.push_back(bb);
      bb2chain[bb->GetBBId()] = this;
    }
    srcChain->bbVec.clear();
    srcChain->unlaidPredCnt = 0;
    srcChain->isCacheValid = false;
    isCacheValid = false;  // is this necessary?
  }

  void UpdateSuccChainBeforeMerged(const BBChain &destChain, const MapleVector<bool> *context,
                                   MapleSet<BBChain*> &readyToLayoutChains) {
    for (BB *bb : bbVec) {
      for (BB *succ : bb->GetSucc()) {
        if (context != nullptr && !(*context)[succ->GetBBId()]) {
          continue;
        }
        if (bb2chain[succ->GetBBId()] == this || bb2chain[succ->GetBBId()] == &destChain) {
          continue;
        }
        BBChain *succChain = bb2chain[succ->GetBBId()];
        succChain->MayRecalculateUnlaidPredCnt(context);
        if (succChain->unlaidPredCnt != 0) {
          --succChain->unlaidPredCnt;
        }
        if (succChain->unlaidPredCnt == 0) {
          readyToLayoutChains.insert(succChain);
        }
      }
    }
  }

  void Dump() const {
    LogInfo::MapleLogger() << "bb chain with " << bbVec.size() << " blocks: ";
    for (BB *bb : bbVec) {
      LogInfo::MapleLogger() << bb->GetBBId() << " ";
    }
    LogInfo::MapleLogger() << std::endl;
  }

  void DumpOneLine() const {
    for (BB *bb : bbVec) {
      LogInfo::MapleLogger() << bb->GetBBId() << " ";
    }
  }

private:
  void MayRecalculateUnlaidPredCnt(const MapleVector<bool> *context) {
    if (isCacheValid) {
      return;  // If cache is trustable, no need to recalculate
    }
    unlaidPredCnt = 0;
    for (BB *bb : bbVec) {
      for (BB *pred : bb->GetPred()) {
        // exclude blocks out of context
        if (context != nullptr && !(*context)[pred->GetBBId()]) {
          continue;
        }
        // exclude blocks within the same chain
        if (bb2chain[pred->GetBBId()] == this) {
          continue;
        }
        ++unlaidPredCnt;
      }
    }
    isCacheValid = true;
  }

  MapleVector<BB*> bbVec;
  MapleVector<BBChain*> &bb2chain;
  uint32 unlaidPredCnt = 0;   // how many predecessors are not laid out
  bool isCacheValid = false;  // whether unlaidPredCnt is trustable
};

class BBLayout {
 public:
  BBLayout(MemPool &memPool, MeFunction &f, bool enabledDebug, MaplePhase *phase)
      : func(f),
        layoutAlloc(&memPool),
        layoutBBs(layoutAlloc.Adapter()),
        startTryBBVec(func.GetCfg()->GetAllBBs().size(), false, layoutAlloc.Adapter()),
        bbVisited(func.GetCfg()->GetAllBBs().size(), false, layoutAlloc.Adapter()),
        allEdges(layoutAlloc.Adapter()),
        bb2chain(layoutAlloc.Adapter()),
        readyToLayoutChains(layoutAlloc.Adapter()),
        laidOut(func.GetCfg()->GetAllBBs().size(), false, layoutAlloc.Adapter()),
        enabledDebug(enabledDebug),
        profValid(func.IsIRProfValid()),
        cfg(f.GetCfg()),
        phase(phase) {
    laidOut[func.GetCfg()->GetCommonEntryBB()->GetBBId()] = true;
    laidOut[func.GetCfg()->GetCommonExitBB()->GetBBId()] = true;
  }

  ~BBLayout() = default;
  BB *NextBB() {
    // return the next BB following strictly program input order
    ++curBBId;
    while (curBBId < func.GetCfg()->GetAllBBs().size()) {
      BB *nextBB = func.GetCfg()->GetBBFromID(curBBId);
      if (nextBB != nullptr && !laidOut[nextBB->GetBBId()]) {
        return nextBB;
      }
      ++curBBId;
    }
    return nullptr;
  }

  bool BBIsEmpty(BB *bb);
  void OptimizeCaseTargets(BB *switchBB, CaseVector *swTable);
  void OptimizeBranchTarget(BB &bb);
  bool BBEmptyAndFallthru(const BB &bb);
  bool BBContainsOnlyGoto(const BB &bb) const;
  bool BBContainsOnlyCondGoto(const BB &bb) const;
  bool HasSameBranchCond(BB &bb1, BB &bb2) const;
  bool BBCanBeMoved(const BB &fromBB, const BB &toAfterBB) const;
  bool BBCanBeMovedBasedProf(const BB &fromBB, const BB &toAfterBB) const;
  void AddBB(BB &bb);
  void AddBBProf(BB &bb);
  void BuildEdges();
  BB *GetFallThruBBSkippingEmpty(BB &bb);
  void ResolveUnconditionalFallThru(BB &bb, BB &nextBB);
  void ChangeToFallthruFromGoto(BB &bb);
  void RemoveUnreachable(BB &bb);
  BB *CreateGotoBBAfterCondBB(BB &bb, BB &fallthru);
  bool ChooseTargetAsFallthru(const BB &bb, const BB &targetBB, const BB &oldFallThru,
                              const BB &fallThru) const;
  void ChangeToFallthruFromCondGoto(BB &bb);
  void OptimizeEmptyFallThruBB(BB &bb);
  const MapleVector<BB*> &GetBBs() const {
    return layoutBBs;
  }

  bool IsNewBBInLayout() const {
    return bbCreated;
  }

  void SetNewBBInLayout() {
    bbCreated = true;
  }

  bool GetTryOutstanding() const {
    return tryOutstanding;
  }

  void SetTryOutstanding(bool val) {
    tryOutstanding = val;
  }

  bool IsBBLaidOut(BBId bbid) const {
    return laidOut.at(bbid);
  }

  void AddLaidOut(bool val) {
    return laidOut.push_back(val);
  }
  void OptimiseCFG();
  BB *NextBBProf(BB &bb);
  BB *GetBBFromEdges();
  void LayoutWithProf(bool useChainLayout);
  void LayoutWithoutProf();
  void RunLayout();
  void DumpBBPhyOrder() const;
  void VerifyBB();

 private:
  void FixEndTryBB(BB &bb);
  void FixTryBB(BB &startTryBB, BB &nextBB);
  void DealWithStartTryBB();
  void UpdateNewBBWithAttrTry(const BB &bb, BB &fallthru) const;
  void SetAttrTryForTheCanBeMovedBB(BB &bb, BB &fallthru) const;
  void RebuildFreq();
  bool IsBBInCurrContext(const BB &bb, const MapleVector<bool> *context) const;
  void InitBBChains();
  void BuildChainForFunc();
  void BuildChainForLoops();
  void BuildChainForLoop(LoopDesc *loop, MapleVector<bool> *context);
  BB *FindBestStartBBForLoop(LoopDesc *loop, MapleVector<bool> *context);
  void DoBuildChain(const BB &header, BBChain &chain, const MapleVector<bool> *context);
  BB *GetBestSucc(BB &bb, const BBChain &chain, const MapleVector<bool> *context, bool considerBetterPredForSucc);
  bool IsCandidateSucc(const BB &bb, BB &succ, const MapleVector<bool> *context);
  bool HasBetterLayoutPred(const BB &bb, BB &succ);

  MeFunction &func;
  MapleAllocator layoutAlloc;
  MapleVector<BB*> layoutBBs;  // gives the determined layout order
  MapleVector<bool> startTryBBVec;  // record the try BB to fix the try&endtry map
  MapleVector<bool> bbVisited; // mark the bb as visited when accessed
  MapleVector<BBEdge*> allEdges;
  MapleVector<BBChain*> bb2chain;   // mapping bb id to the chain that bb belongs to
  MapleSet<BBChain*> readyToLayoutChains;
  IdentifyLoops *meLoop = nullptr;
  Dominance *dom = nullptr;
  uint32 rpoSearchPos = 0;   // reverse post order search beginning position
  bool debugChainLayout = false;
  bool needDealWithTryBB = false;
  BBId curBBId { 0 };          // to index into func.bb_vec_ to return the next BB
  bool bbCreated = false;      // new create bb will change mefunction::bb_vec_ and
  // related analysis result
  MapleVector<bool> laidOut;   // indexed by bbid to tell if has been laid out
  bool tryOutstanding = false; // true if a try laid out but not its endtry
  bool enabledDebug;
  bool profValid = false;
  size_t edgeIdx = 0;
  MeCFG *cfg;
  MaplePhase *phase;
};

MAPLE_FUNC_PHASE_DECLARE(MEBBLayout, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_BB_LAYOUT_H
