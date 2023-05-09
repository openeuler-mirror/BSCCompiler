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
#include "chain_layout.h"
namespace maplebe {
void RelinkBB(BB &prev, BB &next) {
  prev.SetNext(&next);
  next.SetPrev(&prev);
}

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
  if (!VerifyProfileData(iBBs, *bbInfo)) {
    if (!CGOptions::DoLiteProfVerify()) {
      LogInfo::MapleLogger() << "skip profile applying for " << f->GetName() << " due to out of date\n";
    } else {
      CHECK_FATAL_FALSE("Verify lite profile data Failed!");
    }
    return false;
  }

  for (size_t i = 0; i < iBBs.size(); ++i) {
    auto *bbUseInfo = GetOrCreateBBUseInfo(*iBBs[i]);
    bbUseInfo->SetCount(bbInfo->counter[i]);
  }

  InitBBEdgeInfo();
  ComputeEdgeFreq();

  ApplyOnBB();
  return true;
}

bool CGProfUse::VerifyProfileData(const std::vector<maplebe::BB *> &iBBs, LiteProfile::BBInfo &bbInfo) {
  /* check bb size */
  bbInfo.verified.first = true;
  if (bbInfo.counter.size() != iBBs.size()) {
    LogInfo::MapleLogger() << f->GetName() << " counter doesn't match profile counter :"
                           << bbInfo.counter.size() << " func real counter :" << iBBs.size() << '\n';
    bbInfo.verified.second = false;
    return false;
  }
  /* check cfg hash*/
  if (bbInfo.funcHash != f->GetTheCFG()->ComputeCFGHash()) {
    LogInfo::MapleLogger() << f->GetName() << " CFG hash doesn't match profile cfghash :"
                           << bbInfo.funcHash << " func cfghash :" << f->GetTheCFG()->ComputeCFGHash() << '\n';
    bbInfo.verified.second = false;
    return false;
  }
  bbInfo.verified.second = true;
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
  ChainLayout chainLayout(*f, *mp, debugChainLayout, *domInfo);
  // Init layout settings for CG
  chainLayout.SetHasRealProfile(true);
  chainLayout.SetConsiderBetterPred(true);
  chainLayout.BuildChainForFunc();
  NodeChain *mainChain = chainLayout.GetNode2Chain()[f->GetFirstBB()->GetID()];

  for (auto bbId : bbSplit) {
    BB *cbb = f->GetBBFromID(bbId);
    CHECK_FATAL(cbb, "get bb failed");
    f->GetTheCFG()->ReverseCriticalEdge(*cbb);
  }
  std::vector<BB*> coldSection;
  std::vector<uint32> layoutID;
  // clear next pointer for last non-split BB
  for (auto it = mainChain->rbegin(); it != mainChain->rend(); ++it) {
    auto *bb = static_cast<BB*>(*it);
    if (!bbSplit.count(bb->GetID())) {
      bb->SetNext(nullptr);
      break;
    }
  }
  for (auto it = mainChain->begin(); it != mainChain->end(); ++it) {
    auto *bb = static_cast<BB*>(*it);
    if (!bbSplit.count(bb->GetId())) {
      if (bb->IsInColdSection()) {
        coldSection.emplace_back(bb);
      } else {
        AddBBProf(*bb);
        layoutID.emplace_back(bb->GetId());
      }
    }
  }
  for (size_t i = 0; i < coldSection.size(); ++i) {
    AddBBProf(*coldSection[i]);
    layoutID.emplace_back(coldSection[i]->GetId());
  }
  // adjust the last BB if kind is fallthru or condtion BB
  BB *lastBB = layoutBBs.empty() ? nullptr : layoutBBs.back();
  if (lastBB != nullptr && !lastBB->IsEmptyOrCommentOnly()) {
    if (lastBB->GetKind() == BB::kBBFallthru) {
      CHECK_FATAL(lastBB->GetSuccs().size() == 1, "it is fallthru");
      BB *targetBB = *lastBB->GetSuccs().begin();
      BB *newBB = f->GetTheCFG()->GetInsnModifier()->CreateGotoBBAfterCondBB(*lastBB, *targetBB, targetBB == lastBB);
      RelinkBB(*lastBB, *newBB);
    } else if (lastBB->GetKind() == BB::kBBIf) {
      BB *targetBB = CGCFG::GetTargetSuc(*lastBB);
      BB *ftBB = nullptr;
      for (BB *sucBB : lastBB->GetSuccs()) {
        if (sucBB != targetBB) {
          ftBB = sucBB;
        }
      }
      BB *newBB = f->GetTheCFG()->GetInsnModifier()->CreateGotoBBAfterCondBB(*lastBB, *ftBB, targetBB == lastBB);
      RelinkBB(*lastBB, *newBB);
    }
  }
  if (debugChainLayout) {
    LogInfo::MapleLogger() << "Finish forming layout : ";
    for (auto it : layoutID) {
      LogInfo::MapleLogger() << it << " ";
    }
    LogInfo::MapleLogger() << "\n";
  }
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
    // Maintain head and tail BB, because emit phase still will access CFG
    if (count == 0) {
      f.SetFirstBB(*bb);
    }
    if (bb->GetNext() == nullptr) {
      f.SetLastBB(*bb);
    }
    count++;
    if (count > f.GetAllBBs().size()) {
      CHECK_FATAL(false, "infinte loop");
    }
  }
  LogInfo::MapleLogger() << std::endl;
  return false;
}

MAPLE_TRANSFORM_PHASE_REGISTER(CgPgoUse, cgpgouse)

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
      CHECK_FATAL(bbSplit.count(ftBB->GetId()) == 0, "check split bb");
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

void CGProfUse::ChangeToFallthruFromGoto(BB &bb) const {
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
