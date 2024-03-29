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
  std::stack<const BB *> s;
  s.push(&bb);
  visitedMap[bb.GetID()] = true;
  while (!s.empty()) {
    auto node = s.top();
    auto nodeId = node->GetID();
    ASSERT(nodeId < visitedMap.size() && nodeId < postOrderIDVec.size(), "index out of range");
    bool tail = true;
    for (auto succ : node->GetAllSuccs()) {
      if (!visitedMap[succ->GetID()]) {
        tail = false;
        visitedMap[succ->GetID()] = true;
        s.push(succ);
        break;
      }
    }
    if (tail) {
      s.pop();
      postOrderIDVec[nodeId] = pid++;
    }
  }
}

void DomAnalysis::GenPostOrderID() {
  ASSERT(!bbVec.empty(), "size to be allocated is 0");
  MapleVector<bool> visitedMap(bbVec.size() + 1, false, cgFunc.GetFuncScopeAllocator()->Adapter());
  int32 postOrderID = 0;
  PostOrderWalk(commonEntryBB, postOrderID, visitedMap);
  // initialize reversePostOrder
  int32 maxPostOrderID = postOrderID - 1;
  reversePostOrder.resize(static_cast<uint32>(maxPostOrderID + 1));
  for (size_t i = 0; i < postOrderIDVec.size(); ++i) {
    int32 postOrderNo = postOrderIDVec[i];
    if (postOrderNo == -1) {
      continue;
    }
    reversePostOrder[static_cast<uint32>(maxPostOrderID - postOrderNo)] = bbVec[i];
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
  for (const auto *suc : commonEntryBB.GetAllSuccs()) {
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
      auto preds = bb->GetAllPreds();
      if (CommonEntryBBIsPred(*bb) || preds.empty()) {
        pre = &commonEntryBB;
      } else {
        pre = preds[0];
      }
      size_t j = 1;
      while ((GetDom(pre->GetId()) == nullptr || pre == bb) && j < preds.size()) {
        pre = preds[j];
        ++j;
      }
      BB *newIDom = pre;
      for (; j < preds.size(); ++j) {
        pre = preds[j];
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
    auto preds = bb->GetAllPreds();
    if (preds.size() < kBBVectorInitialSize) {
      continue;
    }
    for (const auto *pre : preds) {
      const auto *runner = pre;
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
  for (const auto *succ : commonEntryBB.GetAllSuccs()) {
    auto preds = succ->GetAllPreds();
    if (preds.size() != 1) { // Only deal with one pred bb.
      continue;
    }
    for (const auto  *pre : preds) {
      const auto *runner = pre;
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
  std::stack<const BB *> s;
  s.push(&bb);
  visitedMap[bb.GetID()] = true;
  while (!s.empty()) {
    auto node = s.top();
    auto nodeId = node->GetID();
    if (bbVec[nodeId] == nullptr) {
      s.pop();
      continue;
    }
    ASSERT(nodeId < visitedMap.size() && nodeId < pdomPostOrderIDVec.size(), "index out of range");
    bool tail = true;
    for (auto pred : node->GetAllPreds()) {
      if (!visitedMap[pred->GetID()]) {
        tail = false;
        visitedMap[pred->GetID()] = true;
        s.push(pred);
        break;
      }
    }
    if (tail) {
      s.pop();
      pdomPostOrderIDVec[nodeId] = pid++;
    }
  }
}

void PostDomAnalysis::PdomGenPostOrderID() {
  ASSERT(!bbVec.empty(), "call calloc failed in Dominance::PdomGenPostOrderID");
  MapleVector<bool> visitedMap(bbVec.size(), false, cgFunc.GetFuncScopeAllocator()->Adapter());
  int32 postOrderID = 0;
  PdomPostOrderWalk(commonExitBB, postOrderID, visitedMap);
  // initialize pdomReversePostOrder
  int32 maxPostOrderID = postOrderID - 1;
  pdomReversePostOrder.resize(static_cast<uint32>(maxPostOrderID + 1));
  for (size_t i = 0; i < pdomPostOrderIDVec.size(); ++i) {
    int32 postOrderNo = pdomPostOrderIDVec[i];
    if (postOrderNo == -1) {
      continue;
    }
    pdomReversePostOrder[static_cast<uint32>(maxPostOrderID - postOrderNo)] = bbVec[i];
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
      auto succs = bb->GetAllSuccs();
      if (cgFunc.IsExitBB(*bb) || succs.empty() || (bb->IsWontExit() && bb->GetKind() == BB::kBBGoto)) {
        suc = &commonExitBB;
      } else {
        suc = succs[0];
      }
      size_t j = 1;
      while ((GetPdom(suc->GetId()) == nullptr || suc == bb) && j < succs.size()) {
        suc = succs[j];
        ++j;
      }
      if (GetPdom(suc->GetId()) == nullptr) {
        suc = &commonExitBB;
      }
      BB *newIDom = suc;
      for (; j < succs.size(); ++j) {
        suc = succs[j];
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
    auto succs = bb->GetAllSuccs();
    if (succs.size() < kBBVectorInitialSize) {
      continue;
    }
    for (const auto *suc : succs) {
      const auto *runner = suc;
      while (runner != GetPdom(bb->GetId()) && runner != &commonEntryBB &&
          GetPdom(runner->GetId()) != nullptr) {   // add infinite loop code limit
        if (!HasPdomFrontier(runner->GetId(), bb->GetId())) {
          pdomFrontier[runner->GetId()].push_back(bb->GetId());
        }
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

// bbidMarker indicates that the iterPdomFrontier results for bbid < bbidMarker
// have been computed
void PostDomAnalysis::GetIterPdomFrontier(const BB *bb, MapleSet<uint32> *dfset, uint32 bbidMarker,
                                          std::vector<bool> &visitedMap) {
  if (visitedMap[bb->GetId()]) {
    return;
  }
  visitedMap[bb->GetId()] = true;
  for (uint32 frontierbbid : pdomFrontier[bb->GetId()]) {
    (void)dfset->insert(frontierbbid);
    if (frontierbbid < bbidMarker) {  // union with its computed result
      dfset->insert(iterPdomFrontier[frontierbbid].begin(), iterPdomFrontier[frontierbbid].end());
    } else {  // recursive call
      BB *frontierbb = bbVec[frontierbbid];
      GetIterPdomFrontier(frontierbb, dfset, bbidMarker, visitedMap);
    }
  }
}

void PostDomAnalysis::ComputeIterPdomFrontiers() {
  for (BB *bb : bbVec) {
    if (bb == nullptr || bb == &commonEntryBB) {
      continue;
    }
    std::vector<bool> visitedMap(bbVec.size(), false);
    GetIterPdomFrontier(bb, &iterPdomFrontier[bb->GetId()], bb->GetId(), visitedMap);
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

void PostDomAnalysis::GeneratePdomTreeDot() {
  std::streambuf *coutBuf = std::cout.rdbuf();
  std::ofstream pdomFile;
  std::streambuf *fileBuf = pdomFile.rdbuf();
  (void)std::cout.rdbuf(fileBuf);

  std::string fileName;
  (void)fileName.append("pdom_tree_");
  (void)fileName.append(cgFunc.GetName());
  (void)fileName.append(".dot");

  pdomFile.open(fileName.c_str(), std::ios::trunc);
  if (!pdomFile.is_open()) {
    LogInfo::MapleLogger(kLlWarn) << "fileName:" << fileName << " open failed.\n";
    return;
  }
  pdomFile << "digraph Pdom_" << cgFunc.GetName() << " {\n\n";
  pdomFile << "  node [shape=box];\n\n";

  FOR_ALL_BB_CONST(bb, &cgFunc) {
    if (bb->IsUnreachable()) {
      continue;
    }
    pdomFile << "  BB_" << bb->GetId();
    pdomFile << "[label= \"";
    if (bb == cgFunc.GetCommonEntryBB()) {
      pdomFile << "ENTRY\n";
    }
    pdomFile << "BB_" << bb->GetId() << "\"];\n";
  }
  BB *exitBB = cgFunc.GetCommonExitBB();
  pdomFile << "  BB_" << exitBB->GetId();
  pdomFile << "[label= \"EXIT\n";
  pdomFile << "BB_" << exitBB->GetId() << "\"];\n";
  pdomFile << "\n";

  for (uint32 bbId = 0; bbId < pdomChildren.size(); ++bbId) {
    if (pdomChildren[bbId].empty()) {
      continue;
    }
    BB *parent = cgFunc.GetBBFromID(bbId);
    CHECK_FATAL(parent != nullptr, "get pdom parent-node failed");
    for (auto childId : pdomChildren[bbId]) {
      BB *child = cgFunc.GetBBFromID(childId);
      CHECK_FATAL(child != nullptr, "get pdom child-node failed");
      pdomFile << "  BB_" << parent->GetId() << " -> " << "BB_" << child->GetId();
      pdomFile << " [dir=none]" << ";\n";
    }
  }
  pdomFile << "\n";

  pdomFile << "}\n";
  (void)pdomFile.flush();
  pdomFile.close();
  (void)std::cout.rdbuf(coutBuf);
}

void PostDomAnalysis::Compute() {
  PdomGenPostOrderID();
  ComputePostDominance();
  ComputePdomFrontiers();
  ComputePdomChildren();
  ComputeIterPdomFrontiers();
  uint32 num = 0;
  (void)ComputePdtPreorder(GetCommonExitBB(), num);
  ResizePdtPreOrder(num);
}

bool CgDomAnalysis::PhaseRun(maplebe::CGFunc &f) {
  MemPool *domMemPool = GetPhaseMemPool();
  domAnalysis = domMemPool->New<DomAnalysis>(f, *domMemPool, *domMemPool, f.GetAllBBs(),
                                             *f.GetFirstBB(), *f.GetCommonExitBB());
  domAnalysis->Compute();
  if (CG_DEBUG_FUNC(f)) {
    domAnalysis->Dump();
  }
  return false;
}
MAPLE_ANALYSIS_PHASE_REGISTER(CgDomAnalysis, domanalysis)

bool CgPostDomAnalysis::PhaseRun(maplebe::CGFunc &f) {
  MemPool *pdomMemPool = GetPhaseMemPool();
  /* Currently, using the dummyBB which is created at the beginning of constructing CGFunc, as the commonEntryBB */
  f.GetCommonEntryBB()->PushBackSuccs(*f.GetFirstBB());
  f.GetFirstBB()->PushBackPreds(*f.GetCommonEntryBB());
  pdomAnalysis = pdomMemPool->New<PostDomAnalysis>(f, *pdomMemPool, *pdomMemPool, f.GetAllBBs(),
                                                   *f.GetCommonEntryBB(), *f.GetCommonExitBB());
  pdomAnalysis->Compute();
  if (CG_DEBUG_FUNC(f)) {
    pdomAnalysis->Dump();
  }
  f.GetCommonEntryBB()->ClearSuccs();
  f.GetFirstBB()->ClearPreds();
  return false;
}
MAPLE_ANALYSIS_PHASE_REGISTER(CgPostDomAnalysis, pdomanalysis)
}  /* namespace maplebe */
