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
#include "cg_pgo_use.h"
#include "cg_critical_edge.h"
#include "loop.h"
#include "optimize_common.h"
namespace maplebe {
bool CGProfUse::ApplyPGOData() {
  instrumenter.PrepareInstrumentInfo(f->GetFirstBB(), f->GetCommonExitBB());
  std::vector<maplebe::BB *> iBBs;
  instrumenter.GetInstrumentBBs(iBBs, f->GetFirstBB());

  /* skip large bb function currently due to offset in ldr/store */
  if (iBBs.size() > kMaxPimm8) {
    return false;
  }
  LiteProfile::BBInfo *bbInfo = f->GetFunction().GetModule()->GetLiteProfile().GetFuncBBProf(f->GetName());
  if (bbInfo == nullptr) {
    LogInfo::MapleLogger() << "find profile for " << f->GetName() << "Failed\n";
  }
  CHECK_FATAL(bbInfo != nullptr, "Get profile Failed");
  if (!VerifyProfiledata(iBBs, *bbInfo)) {
    CHECK_FATAL_FALSE("Verify lite profile data Failed!");
    return false;
  }

  for (size_t i = 0; i < iBBs.size(); ++i) {
    auto *bbUseInfo = GetOrCreateBBUseInfo(*iBBs[i]);
    bbUseInfo->SetCount(bbInfo->counter[i]);
  }

  InitBBEdgeInfo();
  ComputeEdgeFreq();

  ApplyOnBB();
  InitFrequencyReversePostOrderBBList();
#if 0
  f->GetTheCFG()->CheckCFGFreq();
#endif
  return true;
}

bool CGProfUse::VerifyProfiledata(const std::vector<maplebe::BB *> &iBBs, LiteProfile::BBInfo &bbInfo) {
  /* check bb size */
  if (bbInfo.counter.size() != iBBs.size()) {
    LogInfo::MapleLogger() << f->GetName() << " counter doesn't match profile counter :"
                           << bbInfo.counter.size() << " func real counter :" << iBBs.size() << '\n';
    return false;
  }
  /* check cfg hash*/
  if (bbInfo.funcHash != f->GetTheCFG()->ComputeCFGHash()) {
    LogInfo::MapleLogger() << f->GetName() << " CFG hash doesn't match profile cfghash :"
                           << bbInfo.funcHash << " func cfghash :" << f->GetTheCFG()->ComputeCFGHash() << '\n';
    return false;
  }
  return true;
}

void CGProfUse::InitBBEdgeInfo() {
  const MapleVector<maple::BBUseEdge<maplebe::BB> *> &allEdges = instrumenter.GetAllEdges();
  for (auto &e : allEdges) {
    BB *src = e->GetSrcBB();
    BB *dest = e->GetDestBB();
    BBUseInfo<BB> *srcUseInfo = GetOrCreateBBUseInfo(*src);
    srcUseInfo->AddOutEdge(e);
    BBUseInfo<BB> *destUseInfo = GetOrCreateBBUseInfo(*dest);
    destUseInfo->AddInEdge(e);
  }
  for (auto &e : allEdges) {
    if (e->IsInMST()) {
      continue;
    }
    BB *src = e->GetSrcBB();
    BBUseInfo<BB> *srcUseInfo = GetOrCreateBBUseInfo(*src, true);
    if (srcUseInfo->GetStatus() && srcUseInfo->GetOutEdgeSize() == 1) {
      SetEdgeCount(*e, srcUseInfo->GetCount());
    } else {
      BB *dest = e->GetDestBB();
      auto destUseInfo = GetOrCreateBBUseInfo(*dest, true);
      if (destUseInfo->GetStatus() && destUseInfo->GetInEdgeSize() == 1) {
        SetEdgeCount(*e, destUseInfo->GetCount());
      }
    }
    if (e->GetStatus()) {
      continue;
    }
    SetEdgeCount(*e, 0);
  }
}

void CGProfUse::ComputeEdgeFreq() {
  bool change = true;
  size_t times = 0;
  BB *commonEntry = f->GetFirstBB();
  while (change) {
    change = false;
    times++;
    CHECK_FATAL(times != UINT32_MAX, "parse all edges fail");
    for (BB *curbb = commonEntry; curbb != nullptr; curbb = curbb->GetNext()) {
      /* skip isolated bb */
      if (curbb->GetSuccs().empty() && curbb->GetPreds().empty()) {
        continue;
      }
      BBUseInfo<BB> *useInfo = GetOrCreateBBUseInfo(*curbb, true);
      if (!useInfo) {
        continue;
      }
      ComputeBBFreq(*useInfo, change);
      if (useInfo->GetStatus()) {
        if (useInfo->GetUnknownOutEdges() == 1) {
          uint64 total = 0;
          uint64 outCount = SumEdgesCount(useInfo->GetOutEdges());
          if (useInfo->GetCount() > outCount) {
            total = useInfo->GetCount() - outCount;
          }
          CHECK_FATAL(useInfo->GetCount() >= outCount, "find bad frequency");
          /* set the only unknown edge frequency */
          SetEdgeCount(*useInfo->GetOnlyUnknownOutEdges(), total);
          change = true;
        }
        if (useInfo->GetUnknownInEdges() == 1) {
          uint64 total = 0;
          uint64 inCount = SumEdgesCount(useInfo->GetInEdges());
          if (useInfo->GetCount() > inCount) {
            total = useInfo->GetCount() - inCount;
          }
          CHECK_FATAL(useInfo->GetCount() >= inCount, "find bad frequency");
          SetEdgeCount(*useInfo->GetOnlyUnknownInEdges(), total);
          change = true;
        }
      }
    }
  }
  if (debugChainLayout) {
    LogInfo::MapleLogger() << "parse all edges in " << times << " times" << '\n';
    LogInfo::MapleLogger() << f->GetName() << " succ compute all edges " << '\n';
  }
}

void CGProfUse::ApplyOnBB() {
  BB *commonEntry = f->GetFirstBB();
  for (BB *curbb = commonEntry; curbb != nullptr; curbb = curbb->GetNext()) {
    /* skip isolated bb */
    if (curbb->GetSuccs().empty() && curbb->GetPreds().empty()) {
      continue;
    }
    BBUseInfo<BB> *useInfo = GetOrCreateBBUseInfo(*curbb, true);
    if (!useInfo) {
      LogInfo::MapleLogger() << "find use info for bb " << curbb->GetId();
      CHECK_FATAL(false, "");
    }
    curbb->SetFrequency(static_cast<uint32>(useInfo->GetCount()));
    if (curbb == f->GetCommonExitBB()) {
      continue;
    }
    curbb->InitEdgeFreq();
    auto outEdges = useInfo->GetOutEdges();
    for (auto *e : outEdges) {
      auto *destBB = e->GetDestBB();
      if (destBB == f->GetCommonExitBB()) {
        continue;
      }
      curbb->SetEdgeFreq(*destBB, e->GetCount());
    }
  }
}

void CGProfUse::ComputeBBFreq(BBUseInfo<maplebe::BB> &bbInfo, bool &change) {
  uint64 count = 0;
  if (!bbInfo.GetStatus()) {
    if (bbInfo.GetUnknownOutEdges() == 0) {
      count = SumEdgesCount(bbInfo.GetOutEdges());
      bbInfo.SetCount(count);
      change = true;
    } else if (bbInfo.GetUnknownInEdges() == 0) {
      count = SumEdgesCount(bbInfo.GetInEdges());
      bbInfo.SetCount(count);
      change = true;
    }
  }
}

uint64 CGProfUse::SumEdgesCount(const MapleVector<BBUseEdge<maplebe::BB> *> &edges) const {
  uint64 count = 0;
  for (const auto &e : edges) {
    count += e->GetCount();
  }
  return count;
}

void CGProfUse::SetEdgeCount(maple::BBUseEdge<maplebe::BB> &e, size_t count) {
  if (!e.GetStatus()) {
    e.SetCount(count);
    BBUseInfo<BB> *srcUseInfo = GetOrCreateBBUseInfo(*(e.GetSrcBB()), true);
    BBUseInfo<BB> *destUseInfo = GetOrCreateBBUseInfo(*(e.GetDestBB()), true);
    srcUseInfo->DecreaseUnKnownOutEdges();
    destUseInfo->DecreaseUnKnownInEdges();
  }
}

BBUseInfo<maplebe::BB> *CGProfUse::GetOrCreateBBUseInfo(const maplebe::BB &bb, bool notCreate) {
  auto item = bbProfileInfo.find(bb.GetId());
  if (item != bbProfileInfo.end()) {
    return item->second;
  } else {
    CHECK_FATAL(!notCreate, "do not create new bb useinfo in this case");
    auto *newInfo = mp->New<BBUseInfo<maplebe::BB>>(*mp);
    (void)bbProfileInfo.emplace(std::make_pair(bb.GetId(), newInfo));
    return newInfo;
  }
}

void CGProfUse::LayoutBBwithProfile() {
  /* initialize */
  laidOut.resize(f->GetAllBBs().size(), false);
  /* BB chain layout */
  BuildChainForFunc();
  BBChain *mainChain = bb2chain[f->GetFirstBB()->GetId()];
  for (auto bbId : bbSplit) {
    BB *cbb = f->GetBBFromID(bbId);
    CHECK_FATAL(cbb, "get bb failed");
    f->GetTheCFG()->ReverseCriticalEdge(*cbb);
  }
  std::vector<BB*> coldSection;
  std::vector<uint32> layoutID;
  for (auto it = mainChain->begin(); it != mainChain->end(); ++it) {
    if (!bbSplit.count((*it)->GetId())) {
      if ((*it)->IsInColdSection()) {
        coldSection.emplace_back(*it);
      } else {
        AddBBProf(**it);
        layoutID.emplace_back((*it)->GetId());
      }
    }
  }
  for (size_t i = 0; i < coldSection.size(); ++i) {
    AddBBProf(*coldSection[i]);
    layoutID.emplace_back(coldSection[i]->GetId());
  }
  if (debugChainLayout) {
    LogInfo::MapleLogger() << "Finish forming layout : ";
    for (auto it : layoutID) {
      LogInfo::MapleLogger() << it << " ";
    }
    LogInfo::MapleLogger() << "\n";
  }
}

void CGProfUse::InitBBChains() {
  uint32 id = 0;
  bb2chain.resize(f->GetAllBBs().size(), nullptr);
  BB *commonEntry = f->GetFirstBB();
  for (BB *curbb = commonEntry; curbb != nullptr; curbb = curbb->GetNext()) {
    // BBChain constructor will update bb2chain
    // attention cleanup & unreachable
    (void)mp->New<BBChain>(puAlloc, bb2chain, *curbb, id++);
  }
}

void CGProfUse::BuildChainForFunc() {
  uint32 validBBNum = 0;
  BB *commonEntry = f->GetFirstBB();
  // attention cleanup & unreachable
  for (BB *curbb = commonEntry; curbb != nullptr; curbb = curbb->GetNext()) {
    if (curbb->IsUnreachable()) {
      ASSERT(false, "check unreachable bb");
      continue;
    }
    if (f->IsExitBB(*curbb)) {
      if (curbb->GetPrev() && curbb->GetPrev()->GetKind() == BB::kBBGoto &&
          curbb->GetPreds().empty() && curbb->GetSuccs().empty()) {
        continue;
      }
    }
    ++validBBNum;
  }
  // --validBBNum;  // exclude cleanup BB
  LogInfo::MapleLogger() << "\n[Chain layout] " << f->GetName() << ", valid bb num: " << validBBNum << std::endl;
  InitBBChains();
  BuildChainForLoops();
  // init ready chains for func
  for (BB *curbb = commonEntry; curbb != nullptr; curbb = curbb->GetNext()) {
    uint32 bbId = curbb->GetId();
    BBChain *chain = bb2chain[bbId];

    if (chain->IsReadyToLayout(nullptr)) {
      (void)readyToLayoutChains.insert(chain);
    }
  }
  BBChain *entryChain = bb2chain[commonEntry->GetId()];
  DoBuildChain(*commonEntry, *entryChain, nullptr);

  /* merge clean up */
  if (f->GetCleanupBB()) {
    BBChain *cleanup = bb2chain[f->GetCleanupBB()->GetId()];
    if (readyToLayoutChains.find(cleanup) == readyToLayoutChains.end()) {
      LogInfo::MapleLogger() << "clean up bb is not in ready layout ";
    }
    CHECK_FATAL(cleanup->GetHeader() == f->GetCleanupBB(), "more than one cleanup");
    if (CGOptions::DoEnableHotColdSplit()) {
      cleanup->GetHeader()->SetColdSection();
    }
    entryChain->MergeFrom(cleanup);
  }
  /* merge symbol label in C which is not in control flow */
  std::vector<BB*> labelBB;
  {
    for (BB *curbb = commonEntry; curbb != nullptr; curbb = curbb->GetNext()) {
      if (curbb->IsUnreachable()) {
        /* delete unreachable bb in cfgo */
        ASSERT(false, "check unreachable bb");
        CHECK_FATAL_FALSE("check unreachable bb");
        continue;
      }
      if (f->IsExitBB(*curbb)) {
        if (curbb->GetPrev() && curbb->GetPrev()->GetKind() == BB::kBBGoto &&
            curbb->GetPreds().empty() && curbb->GetSuccs().empty()) {
          continue;
        }
      }
      if (!entryChain->FindBB(*curbb)) {
        if (curbb->GetPreds().empty() && CGCFG::InSwitchTable(curbb->GetLabIdx(), *f)) {
          labelBB.push_back(curbb);
        // last bb which is not in control flow
        } else if (curbb->GetPreds().empty() && curbb->GetSuccs().empty() && f->GetLastBB() == curbb) {
          labelBB.push_back(curbb);
        } else {
          LogInfo::MapleLogger() << "In function " << f->GetName() << " bb " << curbb->GetId() << "  is no in chain\n";
        }
      }
    }

    for (auto bb : labelBB) {
      BBChain *labelchain = bb2chain[bb->GetId()];
      if (readyToLayoutChains.find(labelchain) == readyToLayoutChains.end()) {
        LogInfo::MapleLogger() << "label bb is not in ready layout ";
      }
      entryChain->MergeFrom(labelchain);
      if (CGOptions::DoEnableHotColdSplit()) {
        bb->SetColdSection();
      }
      bb->SetNext(nullptr);
      bb->SetPrev(nullptr);
    }
  }

  // To sure all of BBs have been laid out
  CHECK_FATAL(entryChain->size() == validBBNum, "has any BB not been laid out?");
}

void CGProfUse::BuildChainForLoops() {
  if (f->GetLoops().empty()) {
    return;
  }
  auto &loops = f->GetLoops();
  // sort loops from inner most to outer most
  std::stable_sort(loops.begin(), loops.end(), [](const CGFuncLoops *loop1, const CGFuncLoops *loop2) {
    return loop1->GetLoopLevel() > loop2->GetLoopLevel();
  });
  auto *context = mp->New<MapleVector<bool>>(f->GetAllBBs().size(), false, puAlloc.Adapter());
  for (auto *loop : loops) {
    BuildChainForLoop(*loop, context);
  }
}

void CGProfUse::BuildChainForLoop(CGFuncLoops &loop, MapleVector<bool> *context) {
  // init loop context
  std::fill(context->begin(), context->end(), false);
  for (auto *bbMember : loop.GetLoopMembers()) {
    CHECK_FATAL(bbMember->GetId() < context->size(), "index out of range");
    (*context)[bbMember->GetId()] = true;
  }
  // init ready chains for loop
  for (auto *bbMember : loop.GetLoopMembers()) {
    BBChain *chain = bb2chain[bbMember->GetId()];
    if (chain->IsReadyToLayout(context)) {
      (void)readyToLayoutChains.insert(chain);
    }
  }
  // find loop chain starting BB
  BB *startBB = FindBestStartBBForLoop(loop, context);
  if (startBB == nullptr) {
    return;  // all blocks in the loop have been laid out, just return
  }
  BBChain *startChain = bb2chain[startBB->GetId()];
  DoBuildChain(*startBB, *startChain, context);
  readyToLayoutChains.clear();
}

// Multiple loops may share the same header, we try to find the best unplaced BB in the loop
BB *CGProfUse::FindBestStartBBForLoop(CGFuncLoops &loop, const MapleVector<bool> *context) {
  auto *headerChain = bb2chain[loop.GetHeader()->GetId()];
  if (headerChain->size() == 1) {
    return loop.GetHeader();
  }
  // take inner loop chain tail BB as start BB
  if (headerChain->size() > 1 && IsBBInCurrContext(*headerChain->GetTail(), context)) {
    return headerChain->GetTail();
  }
  for (auto *bbMember : loop.GetLoopMembers()) {
    if (bb2chain[bbMember->GetId()]->size() == 1) {
      return f->GetBBFromID(bbMember->GetId());
    }
  }
  return nullptr;
}

bool CGProfUse::IsBBInCurrContext(const BB &bb, const MapleVector<bool> *context) const {
  if (context == nullptr) {
    return true;
  }
  return (*context)[bb.GetId()];
}

void CGProfUse::DoBuildChain(const BB &header, BBChain &chain, const MapleVector<bool> *context) {
  CHECK_FATAL(bb2chain[header.GetId()] == &chain, "bb2chain mis-match");
  BB *bb = chain.GetTail();
  BB *bestSucc = GetBestSucc(*bb, chain, context, true);
  while (bestSucc != nullptr) {
    BBChain *succChain = bb2chain[bestSucc->GetId()];
    succChain->UpdateSuccChainBeforeMerged(chain, context, readyToLayoutChains);
    chain.MergeFrom(succChain);
    (void)readyToLayoutChains.erase(succChain);
    bb = chain.GetTail();
    bestSucc = GetBestSucc(*bb, chain, context, true);
  }

  if (debugChainLayout) {
    bool inLoop = context != nullptr;
    LogInfo::MapleLogger() << "Finish forming " << (inLoop ? "loop" : "func") << " chain: ";
    chain.Dump();
  }
}

BB *CGProfUse::GetBestSucc(BB &bb, const BBChain &chain, const MapleVector<bool> *context, bool considerBetterPred) {
  // (1) search in succ
  CHECK_FATAL(bb2chain[bb.GetId()] == &chain, "bb2chain mis-match");
  uint64 bestEdgeFreq = 0;
  BB *bestSucc = nullptr;
  auto iterBB = bb.GetSuccsBegin();
  for (uint32 i = 0; i < bb.GetSuccs().size(); ++i, ++iterBB) {
    CHECK_FATAL(iterBB != bb.GetSuccsEnd(), "check unexpect BB");
    BB *succ = *iterBB;
    CHECK_FATAL(succ, "check Empty BB");
    if (!IsCandidateSucc(bb, *succ, context)) {
      continue;
    }
    if (considerBetterPred && HasBetterLayoutPred(bb, *succ)) {
      continue;
    }
    uint64 currEdgeFreq = bb.GetEdgeFreq(i);  // attention: entryBB->succFreq[i] is always 0
    if (bb.GetId() == 0) {  // special case for common entry BB
      CHECK_FATAL(bb.GetSuccs().size() == 1, "common entry BB should not have more than 1 succ");
      bestSucc = succ;
      break;
    }
    if (currEdgeFreq > bestEdgeFreq) {  // find max edge freq
      bestEdgeFreq = currEdgeFreq;
      bestSucc = succ;
    }
  }
  if (bestSucc != nullptr) {
    if (debugChainLayout) {
      LogInfo::MapleLogger() << "Select [range1 succ ]: ";
      LogInfo::MapleLogger() << bb.GetId() << " -> " << bestSucc->GetId() << std::endl;
    }
    return bestSucc;
  }

  // (2) search in readyToLayoutChains
  uint32 bestFreq = 0;
  for (auto it = readyToLayoutChains.begin(); it != readyToLayoutChains.end(); ++it) {
    BBChain *readyChain = *it;
    BB *header = readyChain->GetHeader();
    if (!IsCandidateSucc(bb, *header, context)) {
      continue;
    }
    bool useBBFreq = false;
    if (useBBFreq) { // use bb freq
      if (header->GetFrequency() > bestFreq) {  // find max bb freq
        bestFreq = header->GetFrequency();
        bestSucc = header;
      }
    } else { // use edge freq
      uint32 subBestFreq = 0;
      for (auto *pred : header->GetPreds()) {
        uint32 curFreq = static_cast<uint32>(pred->GetEdgeFreq(*header));
        if (curFreq > subBestFreq) {
          subBestFreq = curFreq;
        }
      }
      if (subBestFreq > bestFreq) {
        bestFreq = subBestFreq;
        bestSucc = header;
      } else if (subBestFreq == bestFreq && bestSucc != nullptr &&
                 bb2chain[header->GetId()]->GetId() < bb2chain[bestSucc->GetId()]->GetId()) {
        bestSucc = header;
      }
    }
  }
  if (bestSucc != nullptr) {
    (void)readyToLayoutChains.erase(bb2chain[bestSucc->GetId()]);
    if (debugChainLayout) {
      LogInfo::MapleLogger() << "Select [range2 ready]: ";
      LogInfo::MapleLogger() << bb.GetId() << " -> " << bestSucc->GetId() << std::endl;
    }
    return bestSucc;
  }

  // (3) search left part in context by profile
  for (auto freRpoEle : frequencyReversePostOrderBBList) {
    if (freRpoEle.frequency > 0) {
      BB *candBB = freRpoEle.bb;
      if (IsBBInCurrContext(*candBB, context) && bb2chain[candBB->GetId()] != &chain) {
        if (debugChainLayout) {
          LogInfo::MapleLogger() << "Select [range3 frequency ]: ";
          LogInfo::MapleLogger() << bb.GetId() << " -> " << candBB->GetId() << std::endl;
        }
        return candBB;
      }
    } else {
      break;
    }
  }

  // (4) search left part in context by topological sequence
  const auto &rpoVec = domInfo->GetReversePostOrder();
  bool searchedAgain = false;
  for (uint32 i = rpoSearchPos; i < rpoVec.size(); ++i) {
    BB *candBB = rpoVec[i];
    if (IsBBInCurrContext(*candBB, context) && bb2chain[candBB->GetId()] != &chain) {
      rpoSearchPos = i;
      if (debugChainLayout) {
        LogInfo::MapleLogger() << "Select [range4 rpot ]: ";
        LogInfo::MapleLogger() << bb.GetId() << " -> " << candBB->GetId() << std::endl;
      }
      if (CGOptions::DoEnableHotColdSplit()) {
        candBB->SetColdSection();
      }
      return candBB;
    }
    if (i == rpoVec.size() - 1 && !searchedAgain) {
      i = 0;
      searchedAgain = true;
    }
  }
  return nullptr;
}

void CGProfUse::InitFrequencyReversePostOrderBBList() {
  const auto &rpoVec = domInfo->GetReversePostOrder();
  for (uint32 i = 0; i < rpoVec.size(); ++i) {
    BB *cbb = rpoVec[i];
    BBOrderEle bbELe(cbb->GetFrequency(), i, cbb);
    frequencyReversePostOrderBBList.emplace(bbELe);
  }
}

bool CGProfUse::HasBetterLayoutPred(const BB &bb, const BB &succ) const {
  auto &predList = succ.GetPreds();
  // predList.size() may be 0 if bb is common entry BB
  if (predList.size() <= 1) {
    return false;
  }
  uint32 sumEdgeFreq = succ.GetFrequency();
  const double hotEdgeFreqPercent = 0.6;  // should further fine tuning
  uint64 hotEdgeFreq = static_cast<uint64>(sumEdgeFreq * hotEdgeFreqPercent);
  // if edge freq(bb->succ) contribute more than 60% to succ block freq, no better layout pred than bb
  for (auto predIt = predList.begin(); predIt != predList.end(); ++predIt) {
    if (*predIt == &bb) {
      continue;
    }
    uint64 edgeFreq = (*predIt)->GetEdgeFreq(succ);
    if (edgeFreq > (sumEdgeFreq - hotEdgeFreq)) {
      return true;
    }
  }
  return false;
}

bool CGProfUse::IsCandidateSucc(const BB &bb, const BB &succ, const MapleVector<bool> *context) {
  if (!IsBBInCurrContext(succ, context)) { // succ must be in the current context (current loop or current func)
    return false;
  }
  if (bb2chain[succ.GetId()] == bb2chain[bb.GetId()]) { // bb and succ should belong to different chains
    return false;
  }
  if (succ.GetId() == 1) { // special case, exclude common exit BB
    return false;
  }
  return true;
}

bool CgPgoUse::PhaseRun(maplebe::CGFunc &f) {
  CHECK_FATAL(f.NumBBs() < LiteProfile::GetBBNoThreshold(), "stop ! bb out of range!");
  if (!LiteProfile::IsInWhiteList(f.GetName())) {
    return false;
  }

  LiteProfile::BBInfo *bbInfo = f.GetFunction().GetModule()->GetLiteProfile().GetFuncBBProf(f.GetName());

  /* 
   * Currently, If all the counters of the function are 0, the bbInfo will not be recorded in pgo data.
   * skip this case. However, it cannot distinguish which is not genereated correct. Need to be improved */
  if (!bbInfo) {
    return false;
  }

  MemPool *memPool = GetPhaseMemPool();
  auto *split = memPool->New<CriticalEdge>(f, *memPool);
  f.GetTheCFG()->InitInsnVisitor(f);
  split->CollectCriticalEdges();
  split->SplitCriticalEdges();
  MapleSet<uint32> newbbinsplit = split->CopyNewBBInfo();

  MaplePhase *it = GetAnalysisInfoHook()->
      ForceRunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(&CgDomAnalysis::id, f);
  auto *domInfo = static_cast<CgDomAnalysis*>(it)->GetResult();

  (void)GetAnalysisInfoHook()->
      ForceRunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(&CgLoopAnalysis::id, f);

  CHECK_FATAL(domInfo, "find dom failed");
  CGProfUse pUse(f, *memPool, domInfo, newbbinsplit);
  if (pUse.ApplyPGOData()) {
    pUse.LayoutBBwithProfile();
  }
  uint64 count = 0;
  LogInfo::MapleLogger() << "Finial Layout : ";
  FOR_ALL_BB(bb, &f) {
    LogInfo::MapleLogger() << bb->GetId() << " ";
    count++;
    if (count > f.GetAllBBs().size()) {
      CHECK_FATAL(false, "infinte loop");
    }
  }
  LogInfo::MapleLogger() << std::endl;
  return false;
}

MAPLE_TRANSFORM_PHASE_REGISTER(CgPgoUse, cgpgouse)

void RelinkBB(BB &prev, BB &next) {
  prev.SetNext(&next);
  next.SetPrev(&prev);
}

void CGProfUse::AddBBProf(BB &bb) {
  if (layoutBBs.empty()) {
    AddBB(bb);
    return;
  }
  BB *curBB = layoutBBs.back();
  if ((curBB->GetKind() == BB::kBBFallthru || curBB->GetKind() == BB::kBBGoto) && !curBB->GetSuccs().empty()) {
    BB *targetBB = curBB->GetSuccs().front();
    CHECK_FATAL(!bbSplit.count(targetBB->GetId()), "check split bb");
    if (curBB->GetKind() == BB::kBBFallthru && (&bb != targetBB)) {
      ReTargetSuccBB(*curBB, *targetBB);
    } else if (curBB->GetKind() == BB::kBBGoto && (&bb == targetBB)) {
      // delete the goto
      ChangeToFallthruFromGoto(*curBB);
    }
  } else if (curBB->GetKind() == BB::kBBIf) {
    CHECK_FATAL(curBB->GetSuccs().size() <= kSuccSizeOfIfBB, " if bb have more than 2 succs");
    CHECK_FATAL(!curBB->GetSuccs().empty(), " if bb have no succs");
    BB *targetBB = CGCFG::GetTargetSuc(*curBB);
    BB *ftBB = nullptr;
    for (BB *sucBB : curBB->GetSuccs()) {
      if (sucBB != targetBB) {
        ftBB = sucBB;
      }
    }
    if (curBB->GetSuccs().size() == 1 && targetBB != curBB->GetNext()) {
      CHECK_FATAL(false, " only 1 succs for if bb and ft not equal to target");
    }
    if (curBB->GetSuccs().size() == 1 && targetBB == curBB->GetNext()) {
      ftBB = targetBB;
    }
    if (!ftBB && curBB->GetSuccs().size() == kSuccSizeOfIfBB) {
      auto bIt = curBB->GetSuccs().begin();
      BB *firstSucc = *bIt++;
      BB *secondSucc = *bIt;
      if (firstSucc == secondSucc) {
        ftBB = targetBB;
      }
    }
    CHECK_FATAL(ftBB, "find ft bb after ifBB failed");
    if (&bb == targetBB) {
      CHECK_FATAL(!bbSplit.count(ftBB->GetId()), "check split bb");
      LabelIdx fallthruLabel = GetOrCreateBBLabIdx(*ftBB);
      f->GetTheCFG()->GetInsnModifier()->FlipIfBB(*curBB, fallthruLabel);
    } else if (&bb != ftBB) {
      CHECK_FATAL(!bbSplit.count(targetBB->GetId()), "check split bb");
      BB *newBB = f->GetTheCFG()->GetInsnModifier()->CreateGotoBBAfterCondBB(*curBB, *ftBB, targetBB == ftBB);
      CHECK_FATAL(newBB, "create goto failed");
      if (curBB->IsInColdSection()) {
        newBB->SetColdSection();
      }
      if (curBB->GetFrequency() == 0 || ftBB->GetFrequency() == 0) {
        newBB->SetFrequency(0);
      } else {
        newBB->SetFrequency(ftBB->GetFrequency());
      }
      laidOut.push_back(false);
      RelinkBB(*curBB, *newBB);
      AddBB(*newBB);
      curBB = newBB;
    }
  } else if (curBB->GetKind() == BB::kBBIntrinsic) {
    CHECK_FATAL(false, "check intrinsic bb");
  }
  RelinkBB(*curBB, bb);
  AddBB(bb);
}

void CGProfUse::ReTargetSuccBB(BB &bb, BB &fallthru) {
  LabelIdx fallthruLabel = GetOrCreateBBLabIdx(fallthru);
  f->GetTheCFG()->GetInsnModifier()->ReTargetSuccBB(bb, fallthruLabel);
  bb.SetKind(BB::kBBGoto);
}

void CGProfUse::ChangeToFallthruFromGoto(BB &bb) {
  CHECK_FATAL(bb.GetLastMachineInsn(), "Get last insn in GOTO bb failed");
  bb.RemoveInsn(*bb.GetLastMachineInsn());
  bb.SetKind(BB::kBBFallthru);
}

LabelIdx CGProfUse::GetOrCreateBBLabIdx(BB &bb) const {
  LabelIdx bbLabel = bb.GetLabIdx();
  if (bbLabel == MIRLabelTable::GetDummyLabel()) {
    bbLabel = f->CreateLabel();
    bb.SetLabIdx(bbLabel);
    f->SetLab2BBMap(bbLabel, bb);
  }
  return bbLabel;
}

void CGProfUse::AddBB(BB &bb) {
  CHECK_FATAL(bb.GetId() < laidOut.size(), "index out of range in BBLayout::AddBB");
  CHECK_FATAL(!laidOut[bb.GetId()], "AddBB: bb already laid out");
  layoutBBs.push_back(&bb);
  laidOut[bb.GetId()] = true;

  if (bb.GetKind() == BB::kBBReturn) {
    CHECK_FATAL(bb.GetSuccs().empty(), " common entry?");
    bb.SetNext(nullptr);
  }

  // If the pred bb is goto bb and the target bb of goto bb is the current bb which is be added to layoutBBs, change the
  // goto bb to fallthru bb.
  if (layoutBBs.size() > 1) {
    BB *predBB = layoutBBs.at(layoutBBs.size() - 2); // Get the pred of bb.
    if (predBB->GetKind() != BB::kBBGoto) {
      return;
    }
    if (predBB->GetSuccs().front() != &bb) {
      return;
    }
    CHECK_FATAL(false, " implement ft bb to goto bb optimize ");
    ChangeToFallthruFromGoto(*predBB);
  }
}

void BBChain::MergeFrom(BBChain *srcChain) {
  CHECK_FATAL(this != srcChain, "merge same chain?");
  ASSERT_NOT_NULL(srcChain);
  if (srcChain->empty()) {
    return;
  }
  for (BB *bb : *srcChain) {
    bbVec.push_back(bb);
    bb2chain[bb->GetId()] = this;
  }
  srcChain->bbVec.clear();
  srcChain->unlaidPredCnt = 0;
  srcChain->isCacheValid = false;
  isCacheValid = false;  // is this necessary?
}

void BBChain::UpdateSuccChainBeforeMerged(const BBChain &destChain, const MapleVector<bool> *context,
                                          MapleSet<BBChain *> &readyToLayoutChains) {
  for (BB *bb : bbVec) {
    for (BB *succ : bb->GetSuccs()) {
      if (context != nullptr && !(*context)[succ->GetId()]) {
        continue;
      }
      if (bb2chain[succ->GetId()] == this || bb2chain[succ->GetId()] == &destChain) {
        continue;
      }
      BBChain *succChain = bb2chain[succ->GetId()];
      succChain->MayRecalculateUnlaidPredCnt(context);
      if (succChain->unlaidPredCnt != 0) {
        --succChain->unlaidPredCnt;
      }
      if (succChain->unlaidPredCnt == 0) {
        (void)readyToLayoutChains.insert(succChain);
      }
    }
  }
}

void BBChain::MayRecalculateUnlaidPredCnt(const MapleVector<bool> *context) {
  if (isCacheValid) {
    return;  // If cache is trustable, no need to recalculate
  }
  unlaidPredCnt = 0;
  for (BB *bb : bbVec) {
    for (BB *pred : bb->GetPreds()) {
      // exclude blocks out of context
      if (context != nullptr && !(*context)[pred->GetId()]) {
        continue;
      }
      // exclude blocks within the same chain
      if (bb2chain[pred->GetId()] == this) {
        continue;
      }
      ++unlaidPredCnt;
    }
  }
  isCacheValid = true;
}
}
