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
#include "cg_dominance.h"
#include <set>
#include "cg_option.h"
#include "cgfunc.h"

/*
 * This phase build dominance
 */
namespace maplebe {
constexpr uint32 kBBVectorInitialSize = 2;
void DomAnalysis::PostOrderWalk(const BB &bb, int32 &pid, MapleVector<bool> &visitedMap) {
  ASSERT(bb.GetId() < visitedMap.size(), "index out of range in Dominance::PostOrderWalk");
  if (visitedMap[bb.GetId()]) {
    return;
  }
  visitedMap[bb.GetId()] = true;
  for (const BB *suc : bb.GetSuccs()) {
    PostOrderWalk(*suc, pid, visitedMap);
  }
  ASSERT(bb.GetId() < postOrderIDVec.size(), "index out of range in Dominance::PostOrderWalk");
  postOrderIDVec[bb.GetId()] = pid++;
}

void DomAnalysis::GenPostOrderID() {
  ASSERT(!bbVec.empty(), "size to be allocated is 0");
  MapleVector<bool> visitedMap(bbVec.size() + 1, false, cgFunc.GetFuncScopeAllocator()->Adapter());
  int32 postOrderID = 0;
  PostOrderWalk(commonEntryBB, postOrderID, visitedMap);
  // initialize reversePostOrder
  int32 maxPostOrderID = postOrderID - 1;
  reversePostOrder.resize(maxPostOrderID + 1);
  for (size_t i = 0; i < postOrderIDVec.size(); ++i) {
    int32 postOrderNo = postOrderIDVec[i];
    if (postOrderNo == -1) {
      continue;
    }
    reversePostOrder[maxPostOrderID - postOrderNo] = bbVec[i];
  }
}

BB *DomAnalysis::Intersect(BB &bb1, const BB &bb2) {
  auto *ptrBB1 = &bb1;
  auto *ptrBB2 = &bb2;
  while (ptrBB1 != ptrBB2) {
    while (postOrderIDVec[ptrBB1->GetId()] < postOrderIDVec[ptrBB2->GetId()]) {
      ptrBB1 = GetDom(ptrBB1->GetId());
    }
    while (postOrderIDVec[ptrBB2->GetId()] < postOrderIDVec[ptrBB1->GetId()]) {
      ptrBB2 = GetDom(ptrBB2->GetId());
    }
  }
  return ptrBB1;
}

bool DominanceBase::CommonEntryBBIsPred(const BB &bb) const {
  for (const BB *suc : commonEntryBB.GetSuccs()) {
    if (suc == &bb) {
      return true;
    }
  }
  return false;
}

// Figure 3 in "A Simple, Fast Dominance Algorithm" by Keith Cooper et al.
void DomAnalysis::ComputeDominance() {
  SetDom(commonEntryBB.GetId(), &commonEntryBB);
  bool changed;
  do {
    changed = false;
    for (size_t i = 1; i < reversePostOrder.size(); ++i) {
      BB *bb = reversePostOrder[i];
      if (bb == nullptr) {
        continue;
      }
      BB *pre = nullptr;
      auto it = bb->GetPredsBegin();
      if (CommonEntryBBIsPred(*bb) || bb->GetPreds().empty()) {
        pre = &commonEntryBB;
      } else {
        pre = *it;
      }
      it++;
      while ((GetDom(pre->GetId()) == nullptr || pre == bb) && it != bb->GetPredsEnd()) {
        pre = *it;
        it++;
      }
      BB *newIDom = pre;
      for (; it != bb->GetPredsEnd(); it++) {
        pre = *it;
        if (GetDom(pre->GetId()) != nullptr && pre != bb) {
          newIDom = Intersect(*pre, *newIDom);
        }
      }
      if (GetDom(bb->GetId()) != newIDom) {
        SetDom(bb->GetId(), newIDom);
        changed = true;
      }
    }
  } while (changed);
}

// Figure 5 in "A Simple, Fast Dominance Algorithm" by Keith Cooper et al.
void DomAnalysis::ComputeDomFrontiers() {
  for (const BB *bb : bbVec) {
    if (bb == nullptr || bb == &commonExitBB) {
      continue;
    }
    if (bb->GetPreds().size() < kBBVectorInitialSize) {
      continue;
    }
    for (BB *pre : bb->GetPreds()) {
      BB *runner = pre;
      while (runner != nullptr && runner != GetDom(bb->GetId()) && runner != &commonEntryBB) {
        if (!HasDomFrontier(runner->GetId(), bb->GetId())) {
          domFrontier[runner->GetId()].push_back(bb->GetId());
        }
        runner = GetDom(runner->GetId());
      }
    }
  }
  // check entry bb's predBB, such as :
  // bb1 is commonEntryBB, bb2 is entryBB, bb2 is domFrontier of bb3 and bb7.
  //       1
  //       |
  //       2 <-
  //      /   |
  //     3    |
  //    / \   |
  //   4   7---
  //  / \  ^
  // |   | |
  // 5-->6--
  for (BB *succ : commonEntryBB.GetSuccs()) {
    if (succ->GetPreds().size() != 1) { // Only deal with one pred bb.
      continue;
    }
    for (BB *pre : succ->GetPreds()) {
      BB *runner = pre;
      while (runner != GetDom(succ->GetId()) && runner != &commonEntryBB && runner != succ) {
        if (!HasDomFrontier(runner->GetId(), succ->GetId())) {
          domFrontier[runner->GetId()].push_back(succ->GetId());
        }
        runner = GetDom(runner->GetId());
      }
    }
  }
}

void DomAnalysis::ComputeDomChildren() {
  for (auto *bb : reversePostOrder) {
    if (bb == nullptr || GetDom(bb->GetId()) == nullptr) {
      continue;
    }
    BB *parent = GetDom(bb->GetId());
    if (parent == bb) {
      continue;
    }
    domChildren[parent->GetId()].push_back(bb->GetId());
  }
}

// bbidMarker indicates that the iterDomFrontier results for bbid < bbidMarker
// have been computed
void DomAnalysis::GetIterDomFrontier(const BB *bb, MapleSet<uint32> *dfset, uint32 bbidMarker,
                                     std::vector<bool> &visitedMap) {
  if (visitedMap[bb->GetId()]) {
    return;
  }
  visitedMap[bb->GetId()] = true;
  for (uint32 frontierbbid : domFrontier[bb->GetId()]) {
    (void)dfset->insert(frontierbbid);
    if (frontierbbid < bbidMarker) {  // union with its computed result
      dfset->insert(iterDomFrontier[frontierbbid].begin(), iterDomFrontier[frontierbbid].end());
    } else {  // recursive call
      BB *frontierbb = bbVec[frontierbbid];
      GetIterDomFrontier(frontierbb, dfset, bbidMarker, visitedMap);
    }
  }
}

void DomAnalysis::ComputeIterDomFrontiers() {
  for (BB *bb : bbVec) {
    if (bb == nullptr || bb == &commonExitBB) {
      continue;
    }
    std::vector<bool> visitedMap(bbVec.size(), false);
    GetIterDomFrontier(bb, &iterDomFrontier[bb->GetId()], bb->GetId(), visitedMap);
  }
}


uint32 DomAnalysis::ComputeDtPreorder(const BB &bb, uint32 &num) {
  ASSERT(num < dtPreOrder.size(), "index out of range in Dominance::ComputeDtPreorder");
  dtPreOrder[num] = bb.GetId();
  dtDfn[bb.GetId()] = num;
  uint32 maxDtDfnOut = num;
  ++num;

  for (uint32 k : domChildren[bb.GetId()]) {
    maxDtDfnOut = ComputeDtPreorder(*bbVec[k], num);
  }

  dtDfnOut[bb.GetId()] = maxDtDfnOut;
  return maxDtDfnOut;
}

// true if b1 dominates b2
bool DomAnalysis::Dominate(const BB &bb1, const BB &bb2) {
  return dtDfn[bb1.GetId()] <= dtDfn[bb2.GetId()] && dtDfnOut[bb1.GetId()] >= dtDfnOut[bb2.GetId()];
}

void DomAnalysis::Compute() {
  GenPostOrderID();
  ComputeDominance();
  ComputeDomFrontiers();
  ComputeDomChildren();
  ComputeIterDomFrontiers();
  uint32 num = 0;
  (void)ComputeDtPreorder(*cgFunc.GetFirstBB(), num);
  GetDtPreOrder().resize(num);
}

void DomAnalysis::Dump() {
  for (BB *bb : reversePostOrder) {
    LogInfo::MapleLogger() << "postorder no " << postOrderIDVec[bb->GetId()];
    LogInfo::MapleLogger() << " is bb:" << bb->GetId();
    LogInfo::MapleLogger() << " im_dom is bb:" << GetDom(bb->GetId())->GetId();
    LogInfo::MapleLogger() << " domfrontier: [";
    for (uint32 id : domFrontier[bb->GetId()]) {
      LogInfo::MapleLogger() << id << " ";
    }
    LogInfo::MapleLogger() << "] domchildren: [";
    for (uint32 id : domChildren[bb->GetId()]) {
      LogInfo::MapleLogger() << id << " ";
    }
    LogInfo::MapleLogger() << "]\n";
  }
  LogInfo::MapleLogger() << "\npreorder traversal of dominator tree:";
  for (uint32 id : dtPreOrder) {
    LogInfo::MapleLogger() << id << " ";
  }
  LogInfo::MapleLogger() << "\n\n";
}

/* ================= for PostDominance ================= */
void PostDomAnalysis::PdomPostOrderWalk(const BB &bb, int32 &pid, MapleVector<bool> &visitedMap) {
  ASSERT(bb.GetId() < visitedMap.size(), "index out of range in  Dominance::PdomPostOrderWalk");
  if (bbVec[bb.GetId()] == nullptr) {
    return;
  }
  if (visitedMap[bb.GetId()]) {
    return;
  }
  visitedMap[bb.GetId()] = true;
  for (BB *pre : bb.GetPreds()) {
    PdomPostOrderWalk(*pre, pid, visitedMap);
  }
  ASSERT(bb.GetId() < pdomPostOrderIDVec.size(), "index out of range in  Dominance::PdomPostOrderWalk");
  pdomPostOrderIDVec[bb.GetId()] = pid++;
}

void PostDomAnalysis::PdomGenPostOrderID() {
  ASSERT(!bbVec.empty(), "call calloc failed in Dominance::PdomGenPostOrderID");
  MapleVector<bool> visitedMap(bbVec.size(), false, cgFunc.GetFuncScopeAllocator()->Adapter());
  int32 postOrderID = 0;
  PdomPostOrderWalk(commonExitBB, postOrderID, visitedMap);
  // initialize pdomReversePostOrder
  int32 maxPostOrderID = postOrderID - 1;
  pdomReversePostOrder.resize(maxPostOrderID + 1);
  for (size_t i = 0; i < pdomPostOrderIDVec.size(); ++i) {
    int32 postOrderNo = pdomPostOrderIDVec[i];
    if (postOrderNo == -1) {
      continue;
    }
    pdomReversePostOrder[maxPostOrderID - postOrderNo] = bbVec[i];
  }
}

BB *PostDomAnalysis::PdomIntersect(BB &bb1, const BB &bb2) {
  auto *ptrBB1 = &bb1;
  auto *ptrBB2 = &bb2;
  while (ptrBB1 != ptrBB2) {
    while (pdomPostOrderIDVec[ptrBB1->GetId()] < pdomPostOrderIDVec[ptrBB2->GetId()]) {
      ptrBB1 = GetPdom(ptrBB1->GetId());
    }
    while (pdomPostOrderIDVec[ptrBB2->GetId()] < pdomPostOrderIDVec[ptrBB1->GetId()]) {
      ptrBB2 = GetPdom(ptrBB2->GetId());
    }
  }
  return ptrBB1;
}

// Figure 3 in "A Simple, Fast Dominance Algorithm" by Keith Cooper et al.
void PostDomAnalysis::ComputePostDominance() {
  SetPdom(commonExitBB.GetId(), &commonExitBB);
  bool changed = false;
  do {
    changed = false;
    for (size_t i = 1; i < pdomReversePostOrder.size(); ++i) {
      BB *bb = pdomReversePostOrder[i];
      BB *suc = nullptr;
      auto it = bb->GetSuccsBegin();
      if (cgFunc.IsExitBB(*bb) || bb->GetSuccs().empty()) {
        suc = &commonExitBB;
      } else {
        suc = *it;
      }
      it++;
      while ((GetPdom(suc->GetId()) == nullptr || suc == bb) && it != bb->GetSuccsEnd()) {
        suc = *it;
        it++;
      }
      if (GetPdom(suc->GetId()) == nullptr) {
        suc = &commonExitBB;
      }
      BB *newIDom = suc;
      for (; it != bb->GetSuccsEnd(); it++) {
        suc = *it;
        if (GetPdom(suc->GetId()) != nullptr && suc != bb) {
          newIDom = PdomIntersect(*suc, *newIDom);
        }
      }
      if (GetPdom(bb->GetId()) != newIDom) {
        SetPdom(bb->GetId(), newIDom);
        ASSERT(GetPdom(newIDom->GetId()) != nullptr, "null ptr check");
        changed = true;
      }
    }
  } while (changed);
}

// Figure 5 in "A Simple, Fast Dominance Algorithm" by Keith Cooper et al.
void PostDomAnalysis::ComputePdomFrontiers() {
  for (const BB *bb : bbVec) {
    if (bb == nullptr || bb == &commonEntryBB) {
      continue;
    }
    if (bb->GetSuccs().size() < kBBVectorInitialSize) {
      continue;
    }
    for (BB *suc : bb->GetSuccs()) {
      BB *runner = suc;
      while (runner != GetPdom(bb->GetId()) && runner != &commonEntryBB) {
        if (!HasPdomFrontier(runner->GetId(), bb->GetId())) {
          pdomFrontier[runner->GetId()].push_back(bb->GetId());
        }
        ASSERT(GetPdom(runner->GetId()) != nullptr, "ComputePdomFrontiers: pdoms[] is nullptr");
        runner = GetPdom(runner->GetId());
      }
    }
  }
}

void PostDomAnalysis::ComputePdomChildren() {
  for (const BB *bb : bbVec) {
    if (bb == nullptr || GetPdom(bb->GetId()) == nullptr) {
      continue;
    }
    const BB *parent = GetPdom(bb->GetId());
    if (parent == bb) {
      continue;
    }
    pdomChildren[parent->GetId()].push_back(bb->GetId());
  }
}

uint32 PostDomAnalysis::ComputePdtPreorder(const BB &bb, uint32 &num) {
  ASSERT(num < pdtPreOrder.size(), "index out of range in Dominance::ComputePdtPreOrder");
  pdtPreOrder[num] = bb.GetId();
  pdtDfn[bb.GetId()] = num;
  uint32 maxDtDfnOut = num;
  ++num;

  for (uint32 k : pdomChildren[bb.GetId()]) {
    maxDtDfnOut = ComputePdtPreorder(*bbVec[k], num);
  }

  pdtDfnOut[bb.GetId()] = maxDtDfnOut;
  return maxDtDfnOut;
}

// true if b1 postdominates b2
bool PostDomAnalysis::PostDominate(const BB &bb1, const BB &bb2) {
  return pdtDfn[bb1.GetId()] <= pdtDfn[bb2.GetId()] && pdtDfnOut[bb1.GetId()] >= pdtDfnOut[bb2.GetId()];
}

void PostDomAnalysis::Dump() {
  for (BB *bb : pdomReversePostOrder) {
    LogInfo::MapleLogger() << "pdom_postorder no " << pdomPostOrderIDVec[bb->GetId()];
    LogInfo::MapleLogger() << " is bb:" << bb->GetId();
    LogInfo::MapleLogger() << " im_pdom is bb:" << GetPdom(bb->GetId())->GetId();
    LogInfo::MapleLogger() << " pdomfrontier: [";
    for (uint32 id : pdomFrontier[bb->GetId()]) {
      LogInfo::MapleLogger() << id << " ";
    }
    LogInfo::MapleLogger() << "] pdomchildren: [";
    for (uint32 id : pdomChildren[bb->GetId()]) {
      LogInfo::MapleLogger() << id << " ";
    }
    LogInfo::MapleLogger() << "]\n";
  }
  LogInfo::MapleLogger() << "\n";
  LogInfo::MapleLogger() << "preorder traversal of post-dominator tree:";
  for (uint32 id : pdtPreOrder) {
    LogInfo::MapleLogger() << id << " ";
  }
  LogInfo::MapleLogger() << "\n\n";
}

void PostDomAnalysis::Compute() {
  PdomGenPostOrderID();
  ComputePostDominance();
  ComputePdomFrontiers();
  ComputePdomChildren();
  uint32 num = 0;
  (void)ComputePdtPreorder(GetCommonExitBB(), num);
  ResizePdtPreOrder(num);
}

AnalysisResult *CgDoDomAnalysis::Run(CGFunc *cgFunc, CgFuncResultMgr *cgFuncResultMgr) {
  (void)cgFuncResultMgr;
  ASSERT(cgFunc != nullptr, "expect a cgFunc in CgDoDomAnalysis");
  MemPool *domMemPool = NewMemPool();
  DomAnalysis *domAnalysis = nullptr;
  domAnalysis = domMemPool->New<DomAnalysis>(*cgFunc, *domMemPool, *domMemPool, cgFunc->GetAllBBs(),
                                             *cgFunc->GetFirstBB(), *cgFunc->GetLastBB());
  domAnalysis->Compute();
  if (CG_DEBUG_FUNC(cgFunc)) {
    domAnalysis->Dump();
  }
  return domAnalysis;
}

AnalysisResult *CgDoPostDomAnalysis::Run(CGFunc *cgFunc, CgFuncResultMgr *cgFuncResultMgr) {
  (void)cgFuncResultMgr;
  ASSERT(cgFunc != nullptr, "expect a cgFunc in CgDoPostDomAnalysis");
  MemPool *domMemPool = NewMemPool();
  PostDomAnalysis *pdomAnalysis = nullptr;
  pdomAnalysis = domMemPool->New<PostDomAnalysis>(*cgFunc, *domMemPool, *domMemPool, cgFunc->GetAllBBs(),
                                                  *cgFunc->GetFirstBB(), *cgFunc->GetLastBB());
  pdomAnalysis->Compute();
  if (CG_DEBUG_FUNC(cgFunc)) {
    pdomAnalysis->Dump();
  }
  return pdomAnalysis;
}
}  /* namespace maplebe */
