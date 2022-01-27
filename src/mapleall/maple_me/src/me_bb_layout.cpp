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
#include "me_bb_layout.h"
#include "me_cfg.h"
#include "bb.h"
#include "me_irmap.h"
#include "me_option.h"
#include "me_predict.h"
#include "maple_phase.h"

// This BB layout strategy strictly obeys source ordering when inside try blocks.
// This Optimization will reorder the bb layout. it start from the first bb of func.
// All bbs will be put into layoutBBs and it gives the determined layout order.
// The entry of the optimization is MeDoBBLayout::Run. If func's IR profile is
// valid, use the either chain layout or Pettis & Hansen intra func bb layout
// according to useChainLayout argument.
// the PH layout refer the following paper:
// Profile Guided Code Positioning
// The idea is when there's a branch, put the most probility target next to
// the branch to get the minimum jump distance.
// If the profile is invalid, do the normal bb layout:
// It starts from the first bb.
// 1. If curBB is condtion goto or goto kind, do OptimizeBranchTarget for bb.
// 2. Find curBB's next bb nextBB, and based on nextBB do the following:
// 3. (1) For fallthru/catch/finally, fix curBB's fallthru
//    (2) For condtion goto curBB:
//        i) If the target bb can be moved, then put it as currBB's next
//           and retarget curBB to it's fallthru bb. add targetBB as next.
//        ii) If curBB's fallthru is not its next bb add fallthru as its next if
//            fallthru can be moved else create a new fallthru contains a goto to
//            the original fallthru
//    (3) For goto curBB see if goto target can be placed as next.
// 5. do step 3 for nextBB until all bbs are laid out
namespace maple {
static constexpr uint32 kMaxNumBBToPredict = 7500;

static void CreateGoto(BB &bb, MeFunction &func, BB &fallthru) {
  LabelIdx label = func.GetOrCreateBBLabel(fallthru);
  if (func.GetIRMap() != nullptr) {
    GotoNode stmt(OP_goto);
    auto *newGoto = func.GetIRMap()->New<GotoMeStmt>(&stmt);
    newGoto->SetOffset(label);
    bb.AddMeStmtLast(newGoto);
  } else {
    auto *newGoto = func.GetMirFunc()->GetCodeMempool()->New<GotoNode>(OP_goto);
    newGoto->SetOffset(label);
    bb.AddStmtNode(newGoto);
  }
  bb.SetKind(kBBGoto);
}

bool BBLayout::IsBBInCurrContext(const BB &bb, const MapleVector<bool> *context) const {
  if (context == nullptr) {
    return true;
  }
  return (*context)[bb.GetBBId()];
}

// Create chains for each BB
void BBLayout::InitBBChains() {
  bb2chain.resize(cfg->NumBBs(), nullptr);
  for (auto it = cfg->valid_begin(); it != cfg->valid_end(); ++it) {
    // BBChain constructor will update bb2chain
    (void)layoutAlloc.GetMemPool()->New<BBChain>(layoutAlloc, bb2chain, *it);
  }
}

void BBLayout::BuildChainForFunc() {
  debugChainLayout = enabledDebug;
  uint32 validBBNum = 0;
  for (auto it = cfg->valid_begin(); it != cfg->valid_end(); ++it) {
    ++validBBNum;
  }
  --validBBNum;  // exclude common entry BB
  if (debugChainLayout) {
    LogInfo::MapleLogger() << "\n[Chain layout] " << func.GetName() << ", valid bb num: " << validBBNum << std::endl;
  }
  InitBBChains();
  BuildChainForLoops();
  // init ready chains for func
  for (auto it = cfg->valid_end(); it != cfg->valid_end(); ++it) {
    BB *bb = *it;
    BBId bbId = bb->GetBBId();
    BBChain *chain = bb2chain[bbId];
    if (chain->IsReadyToLayout(nullptr)) {
      readyToLayoutChains.insert(chain);
    }
  }
  BB *entryBB = func.GetCfg()->GetCommonEntryBB();
  BBChain *entryChain = bb2chain[entryBB->GetBBId()];
  DoBuildChain(*entryBB, *entryChain, nullptr);
  // To sure all of BBs have been laid out
  CHECK_FATAL(entryChain->size() == validBBNum, "has any BB not been laid out?");
}

void BBLayout::BuildChainForLoops() {
  if (meLoop == nullptr || meLoop->GetMeLoops().empty()) {
    return;
  }
  auto &loops = meLoop->GetMeLoops();
  // sort loops from inner most to outer most
  // need use the same sort rules as prediction?
  std::stable_sort(loops.begin(), loops.end(),[](const LoopDesc *loop1, const LoopDesc *loop2) {
    return loop1->nestDepth > loop2->nestDepth;
  });
  // build chain for loops one by one
  auto *context =
      layoutAlloc.GetMemPool()->New<MapleVector<bool>>(cfg->NumBBs(), false, layoutAlloc.Adapter());
  for (auto *loop : loops) {
    BuildChainForLoop(loop, context);
  }
}

void BBLayout::BuildChainForLoop(LoopDesc *loop, MapleVector<bool> *context) {
  // init loop context
  std::fill(context->begin(), context->end(), false);
  for (BBId bbId : loop->loopBBs) {
    (*context)[bbId] = true;
  }
  // init ready chains for loop
  for (BBId bbId : loop->loopBBs) {
    BBChain *chain = bb2chain[bbId];
    if (chain->IsReadyToLayout(context)) {
      readyToLayoutChains.insert(chain);
    }
  }
  // find loop chain starting BB
  BB *startBB = FindBestStartBBForLoop(loop, context);
  if (startBB == nullptr) {
    return;  // all blocks in the loop have been laid out, just return
  }
  BBChain *startChain = bb2chain[startBB->GetBBId()];
  DoBuildChain(*startBB, *startChain, context);
  readyToLayoutChains.clear();
}

// Multiple loops may share the same header, we try to find the best unplaced BB in the loop
// This function can be improved
BB *BBLayout::FindBestStartBBForLoop(LoopDesc *loop, MapleVector<bool> *context) {
  // If the loop header has not been placed, take it as start BB of the loop chain
  auto *headerChain = bb2chain[loop->head->GetBBId()];
  if (headerChain->size() == 1) {
    return loop->head;
  }
  // take inner loop chain tail BB as start BB
  if (headerChain->size() > 1 && IsBBInCurrContext(*headerChain->GetTail(), context)) {
    return headerChain->GetTail();
  }
  for (BBId bbId : loop->loopBBs) {
    if (bb2chain[bbId]->size() == 1) {
      return cfg->GetBBFromID(bbId);
    }
  }
  return nullptr;
}

void BBLayout::DoBuildChain(const BB &header, BBChain &chain, const MapleVector<bool> *context) {
  CHECK_FATAL(bb2chain[header.GetBBId()] == &chain, "bb2chain mis-match");
  BB *bb = chain.GetTail();
  BB *bestSucc = GetBestSucc(*bb, chain, context, false);
  while (bestSucc != nullptr) {
    BBChain *succChain = bb2chain[bestSucc->GetBBId()];
    succChain->UpdateSuccChainBeforeMerged(chain, context, readyToLayoutChains);
    chain.MergeFrom(succChain);
    readyToLayoutChains.erase(succChain);
    bb = chain.GetTail();
    bestSucc = GetBestSucc(*bb, chain, context, false);
  }
  if (debugChainLayout) {
    bool inLoop = context != nullptr;
    LogInfo::MapleLogger() << "Finish forming " << (inLoop ? "loop" : "func") << " chain: ";
    chain.Dump();
  }
}

bool BBLayout::IsCandidateSucc(const BB &bb, BB &succ, const MapleVector<bool> *context) {
  if (!IsBBInCurrContext(succ, context)) { // succ must be in the current context (current loop or current func)
    return false;
  }
  if (bb2chain[succ.GetBBId()] == bb2chain[bb.GetBBId()]) { // bb and succ should belong to different chains
    return false;
  }
  if (succ.GetBBId() == 1) { // special case, exclude common exit BB
    return false;
  }
  return true;
}

// Whether succ has a better layout pred than bb
bool BBLayout::HasBetterLayoutPred(const BB &bb, BB &succ) {
  auto &predList = succ.GetPred();
  // predList.size() may be 0 if bb is common entry BB
  if (predList.size() <= 1) {
    return false;
  }
  uint32 sumEdgeFreq = succ.GetFrequency();
  const double hotEdgeFreqPercent = 0.8;  // should further fine tuning
  uint32 hotEdgeFreq = sumEdgeFreq * hotEdgeFreqPercent;
  // if edge freq(bb->succ) contribute more than 80% to succ block freq, no better layout pred than bb
  for (uint32 i = 0; i < predList.size(); ++i) {
    if (predList[i] == &bb) {
      continue;
    }
    uint32 edgeFreq = static_cast<uint32>(predList[i]->GetEdgeFreq(&succ));
    if (edgeFreq > (sumEdgeFreq - hotEdgeFreq)) {
      return true;
    }
  }
  return false;
}

// considerBetterPredForSucc: whether consider better layout pred for succ, we found better
// performance when this argument is disabled
BB *BBLayout::GetBestSucc(BB &bb, const BBChain &chain, const MapleVector<bool> *context,
                          bool considerBetterPredForSucc) {
  // (1) search in succ
  CHECK_FATAL(bb2chain[bb.GetBBId()] == &chain, "bb2chain mis-match");
  uint32 bestEdgeFreq = 0;
  BB *bestSucc = nullptr;
  for (uint32 i = 0; i < bb.GetSucc().size(); ++i) {
    BB *succ = bb.GetSucc(i);
    if (!IsCandidateSucc(bb, *succ, context)) {
      continue;
    }
    if (considerBetterPredForSucc && HasBetterLayoutPred(bb, *succ)) {
      continue;
    }
    uint32 currEdgeFreq = static_cast<uint32>(bb.GetEdgeFreq(i));  // attention: entryBB->succFreq[i] is always 0
    if (bb.GetBBId() == 0) {  // special case for common entry BB
      CHECK_FATAL(bb.GetSucc().size() == 1, "common entry BB should not have more than 1 succ");
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
      LogInfo::MapleLogger() << bb.GetBBId() <<  " -> " << bestSucc->GetBBId() << std::endl;
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
      for (auto *pred : header->GetPred()) {
        uint32 curFreq = static_cast<uint32>(pred->GetEdgeFreq(header));
        if (curFreq > subBestFreq) {
          subBestFreq = curFreq;
        }
      }
      if (subBestFreq > bestFreq) {
        bestFreq = subBestFreq;
        bestSucc = header;
      }
    }
  }
  if (bestSucc != nullptr) {
    readyToLayoutChains.erase(bb2chain[bestSucc->GetBBId()]);
    if (debugChainLayout) {
      LogInfo::MapleLogger() << "Select [range2 ready]: ";
      LogInfo::MapleLogger() << bb.GetBBId() << " -> " << bestSucc->GetBBId() << std::endl;
    }
    return bestSucc;
  }

  // (3) search left part in context by topological sequence
  const auto &rpoVec = dom->GetReversePostOrder();
  bool searchedAgain = false;
  for (uint32 i = rpoSearchPos; i < rpoVec.size(); ++i) {
    BB *candBB = rpoVec[i];
    if (IsBBInCurrContext(*candBB, context) && bb2chain[candBB->GetBBId()] != &chain) {
      rpoSearchPos = i;
      if (debugChainLayout) {
        LogInfo::MapleLogger() << "Select [range3 rpot ]: ";
        LogInfo::MapleLogger() << bb.GetBBId() << " -> " << candBB->GetBBId() << std::endl;
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

// return true if bb is empty and its kind is fallthru.
bool BBLayout::BBEmptyAndFallthru(const BB &bb) {
  if (bb.GetAttributes(kBBAttrIsTryEnd)) {
    return false;
  }
  if (bb.GetKind() == kBBFallthru) {
    if (func.GetIRMap() != nullptr) {
      return bb.IsMeStmtEmpty();
    }
    return bb.IsEmpty();
  }
  return false;
}

// Return true if bb only has conditonal branch stmt except comment
bool BBLayout::BBContainsOnlyCondGoto(const BB &bb) const {
  if (bb.GetKind() != kBBCondGoto || bb.GetAttributes(kBBAttrIsTryEnd)) {
    return false;
  }

  if (func.GetIRMap() != nullptr) {
    auto &meStmts = bb.GetMeStmts();
    if (meStmts.empty()) {
      return false;
    }
    for (auto itMeStmt = meStmts.begin(); itMeStmt != meStmts.rbegin().base(); ++itMeStmt) {
      if (!itMeStmt->IsCondBr() && itMeStmt->GetOp() != OP_comment) {
        return false;
      }
    }
    return meStmts.back().IsCondBr();
  }
  auto &stmtNodes = bb.GetStmtNodes();
  if (stmtNodes.empty()) {
    return false;
  }
  for (auto itStmt = stmtNodes.begin(); itStmt != stmtNodes.rbegin().base(); ++itStmt) {
    if (!itStmt->IsCondBr() && itStmt->GetOpCode() != OP_comment) {
      return false;
    }
  }
  return bb.GetStmtNodes().back().IsCondBr();
}

bool BBLayout::ChooseTargetAsFallthru(const BB &bb, const BB &targetBB, const BB &oldFallThru,
                                      const BB &fallthru) const {
  if (&targetBB == &fallthru) {
    return false;
  }
  if (profValid) {
    uint64 freqToTargetBB = bb.GetEdgeFreq(&targetBB);
    uint64 freqToFallthru = bb.GetEdgeFreq(&fallthru);
    if (enabledDebug) {
      LogInfo::MapleLogger() << func.GetName() << " " << bb.GetBBId() << "->" << targetBB.GetBBId() << " freq "
                             << freqToTargetBB << " " << bb.GetBBId() << "->" << fallthru.GetBBId() << " freq "
                             << freqToFallthru << '\n';
    }
    if ((freqToTargetBB > freqToFallthru) && BBCanBeMovedBasedProf(targetBB, bb)) {
      if (enabledDebug) {
        LogInfo::MapleLogger() << func.GetName() << bb.GetBBId() << " move targeBB " << targetBB.GetBBId()
                               << " to fallthru" << '\n';
      }
      return true;
    }
  } else {
    if ((&oldFallThru != &fallthru || fallthru.GetPred().size() > 1)
        && BBCanBeMoved(targetBB, bb)) {
      return true;
    }
  }
  return false;
}
// Return the opposite opcode for condition/compare opcode.
static Opcode GetOppositeOp(Opcode opcInput) {
  Opcode opc = OP_undef;
  switch (opcInput) {
    case OP_brtrue:
      opc = OP_brfalse;
      break;
    case OP_brfalse:
      opc = OP_brtrue;
      break;
    case OP_ne:
      opc = OP_eq;
      break;
    case OP_eq:
      opc = OP_ne;
      break;
    case OP_gt:
      opc = OP_le;
      break;
    case OP_le:
      opc = OP_gt;
      break;
    case OP_lt:
      opc = OP_ge;
      break;
    case OP_ge:
      opc = OP_lt;
      break;
    default:
      break;
  }
  return opc;
}

bool BBLayout::BBContainsOnlyGoto(const BB &bb) const {
  if (bb.GetKind() != kBBGoto || bb.GetAttributes(kBBAttrIsTryEnd)) {
    return false;
  }

  if (func.GetIRMap() != nullptr) {
    auto &meStmts = bb.GetMeStmts();
    if (meStmts.empty()) {
      return false;
    }
    for (auto itMeStmt = meStmts.begin(); itMeStmt != meStmts.rbegin().base(); ++itMeStmt) {
      if (itMeStmt->GetOp() != OP_goto && itMeStmt->GetOp() != OP_comment) {
        return false;
      }
    }
    return meStmts.back().GetOp() == OP_goto;
  }
  auto &stmtNodes = bb.GetStmtNodes();
  if (stmtNodes.empty()) {
    return false;
  }
  for (auto itStmt = stmtNodes.begin(); itStmt != stmtNodes.rbegin().base(); ++itStmt) {
    if (itStmt->GetOpCode() != OP_goto && itStmt->GetOpCode() != OP_comment) {
      return false;
    }
  }
  return bb.GetStmtNodes().back().GetOpCode() == OP_goto;
}

// Return true if all the following are satisfied:
// 1.fromBB only has one predecessor
// 2.fromBB has not been laid out.
// 3.fromBB has only one succor when fromBB is artifical or fromBB and
//   toafter_bb are both not in try block.
// The other case is fromBB has one predecessor and one successor and
// contains only goto stmt.
bool BBLayout::BBCanBeMoved(const BB &fromBB, const BB &toAfterBB) const {
  if (laidOut[fromBB.GetBBId()]) {
    return false;
  }
  if (fromBB.GetPred().size() > 1) {
    // additional condition: if all preds have been laidout, fromBB can be freely moved
    for (auto *pred : fromBB.GetPred()) {
      if (!laidOut[pred->GetBBId()]) {
        return false;
      }
    }
  }

  if (fromBB.GetAttributes(kBBAttrArtificial) ||
      (!fromBB.GetAttributes(kBBAttrIsTry) && !toAfterBB.GetAttributes(kBBAttrIsTry))) {
    return fromBB.GetSucc().size() == 1;
  }
  return BBContainsOnlyGoto(fromBB);
}

// Return true if all the following are satisfied:
// 1.fromBB has not been laid out.
// 2.fromBB has only one succor when fromBB is artifical or fromBB and
//   toafter_bb are both not in try block.
// The other case is fromBB contains only goto stmt.
bool BBLayout::BBCanBeMovedBasedProf(const BB &fromBB, const BB &toAfterBB) const {
  if (laidOut[fromBB.GetBBId()]) {
    return false;
  }
  if (fromBB.GetAttributes(kBBAttrArtificial) ||
      (!fromBB.GetAttributes(kBBAttrIsTry) && !toAfterBB.GetAttributes(kBBAttrIsTry))) {
    return fromBB.GetSucc().size() <= 1;
  }
  return BBContainsOnlyGoto(fromBB);
}

// Return true if bb1 and bb2 has the branch conditon.such as
// bb1 : brfalse (a > 3)  bb2: brfalse (a > 3)/ brtrue (a <= 3)
bool BBLayout::HasSameBranchCond(BB &bb1, BB &bb2) const {
  if (func.GetIRMap() == nullptr) {
    return false;
  }
  auto &meStmt1 = static_cast<CondGotoMeStmt&>(bb1.GetMeStmts().back());
  auto &meStmt2 = static_cast<CondGotoMeStmt&>(bb2.GetMeStmts().back());
  MeExpr *expr1 = meStmt1.GetOpnd();
  MeExpr *expr2 = meStmt2.GetOpnd();
  // Compare the opcode:  brtrue/brfalse
  if (!(meStmt1.GetOp() == meStmt2.GetOp() && expr1->GetOp() == expr2->GetOp()) &&
      !(meStmt1.GetOp() == GetOppositeOp(meStmt2.GetOp()) && expr1->GetOp() == GetOppositeOp(expr2->GetOp()))) {
    return false;
  }
  if (!(expr1->GetMeOp() == expr2->GetMeOp() && expr1->GetMeOp() == kMeOpOp)) {
    return false;
  }
  auto *opMeExpr1 = static_cast<OpMeExpr*>(expr1);
  auto *opMeExpr2 = static_cast<OpMeExpr*>(expr2);
  // Compare the two operands to make sure they are both equal.
  if (opMeExpr1->GetOpnd(0) != opMeExpr2->GetOpnd(0)) {
    return false;
  }
  // If one side is const, assume it is always the rhs.
  if ((opMeExpr1->GetOpnd(1) != opMeExpr2->GetOpnd(1)) &&
      !(opMeExpr1->GetOpnd(1)->IsZero() && opMeExpr2->GetOpnd(1)->IsZero())) {
    return false;
  }
  return true;
}

bool BBLayout::BBIsEmpty(BB *bb) {
  if (func.GetIRMap() != nullptr) {
    return bb->IsMeStmtEmpty();
  }
  return bb->IsEmpty();
}

void BBLayout::OptimizeCaseTargets(BB *switchBB, CaseVector *swTable) {
  CaseVector::iterator caseIt = swTable->begin();
  for (; caseIt != swTable->end(); ++caseIt) {
    LabelIdx lidx = caseIt->second;
    BB *brTargetBB = func.GetCfg()->GetLabelBBAt(lidx);
    if (brTargetBB->GetSucc().size() != 0) {
      OptimizeBranchTarget(*brTargetBB);
    }
    if (brTargetBB->GetSucc().size() != 1) {
      continue;
    }
    if (!(BBContainsOnlyGoto(*brTargetBB) || BBIsEmpty(brTargetBB))) {
      continue;
    }
    BB *newTargetBB = brTargetBB->GetSucc().front();
    if (newTargetBB == brTargetBB) {
      continue;
    }
    LabelIdx newTargetLabel = func.GetOrCreateBBLabel(*newTargetBB);
    caseIt->second = newTargetLabel;
    if (!newTargetBB->IsSuccBB(*switchBB)) {
      switchBB->AddSucc(*newTargetBB);
    }
  }
}

// (1) bb's last statement is a conditional or unconditional branch; if the branch
// target is a BB with only a single goto statement, optimize the branch target
// to the eventual target
// (2) bb's last statement is a conditonal branch, if the branch target is a BB with a single
// conditional branch statement and has the same condition as bb's last statement, optimize the
// branch target to the eventual target.
void BBLayout::OptimizeBranchTarget(BB &bb) {
  if (bbVisited[bb.GetBBId().GetIdx()]) {
    return;
  }
  if (func.GetIRMap() != nullptr) {
    auto &meStmts = bb.GetMeStmts();
    if (meStmts.empty()) {
      return;
    }
    if (meStmts.back().GetOp() != OP_goto && !meStmts.back().IsCondBr()) {
      return;
    }
  } else {
    auto &stmtNodes = bb.GetStmtNodes();
    if (stmtNodes.empty()) {
      return;
    }
    if (stmtNodes.back().GetOpCode() != OP_goto && !stmtNodes.back().IsCondBr()) {
      return;
    }
  }
  do {
    ASSERT(!bb.GetSucc().empty(), "container check");
    BB *brTargetBB = bb.GetKind() == kBBCondGoto ? bb.GetSucc().back() : bb.GetSucc().front();
    CHECK_FATAL((bb.GetKind() != kBBCondGoto || bb.GetSucc().back() != bb.GetSucc().front()),
                "target is same as fallthru");
    if (brTargetBB->GetAttributes(kBBAttrWontExit)) {
      return;
    }
    if (!BBContainsOnlyGoto(*brTargetBB) && !BBEmptyAndFallthru(*brTargetBB) &&
        !(bb.GetKind() == kBBCondGoto && brTargetBB->GetKind() == kBBCondGoto && &bb != brTargetBB &&
          BBContainsOnlyCondGoto(*brTargetBB) && HasSameBranchCond(bb, *brTargetBB))) {
      return;
    }
    bbVisited[bb.GetBBId().GetIdx()] = true;
    OptimizeBranchTarget(*brTargetBB);
    // optimize stmt
    BB *newTargetBB =
        (brTargetBB->GetKind() == kBBCondGoto) ? brTargetBB->GetSucc().back() : brTargetBB->GetSucc().front();
    if (newTargetBB == brTargetBB) {
      return;
    }
    LabelIdx newTargetLabel = func.GetOrCreateBBLabel(*newTargetBB);
    if (func.GetIRMap() != nullptr) {
      auto &lastStmt = bb.GetMeStmts().back();
      if (lastStmt.GetOp() == OP_goto) {
        auto &gotoMeStmt = static_cast<GotoMeStmt&>(lastStmt);
        ASSERT(brTargetBB->GetBBLabel() == gotoMeStmt.GetOffset(), "OptimizeBranchTarget: wrong branch target BB");
        gotoMeStmt.SetOffset(newTargetLabel);
      } else {
        auto &gotoMeStmt = static_cast<CondGotoMeStmt&>(lastStmt);
        ASSERT(brTargetBB->GetBBLabel() == gotoMeStmt.GetOffset(), "OptimizeBranchTarget: wrong branch target BB");
        gotoMeStmt.SetOffset(newTargetLabel);
      }
    } else {
      StmtNode &lastStmt = bb.GetStmtNodes().back();
      if (lastStmt.GetOpCode() == OP_goto) {
        auto &gotoNode = static_cast<GotoNode&>(lastStmt);
        ASSERT(brTargetBB->GetBBLabel() == gotoNode.GetOffset(), "OptimizeBranchTarget: wrong branch target BB");
        gotoNode.SetOffset(newTargetLabel);
      } else {
        auto &gotoNode = static_cast<CondGotoNode&>(lastStmt);
        ASSERT(brTargetBB->GetBBLabel() == gotoNode.GetOffset(), "OptimizeBranchTarget: wrong branch target BB");
        gotoNode.SetOffset(newTargetLabel);
      }
    }
    // update CFG
    bb.ReplaceSucc(brTargetBB, newTargetBB);
    if (brTargetBB->GetPred().empty()) {
      laidOut[brTargetBB->GetBBId()] = true;
      RemoveUnreachable(*brTargetBB);
      if (needDealWithTryBB) {
        DealWithStartTryBB();
      }
    }
  } while (true);
}

void BBLayout::AddBB(BB &bb) {
  CHECK_FATAL(bb.GetBBId() < laidOut.size(), "index out of range in BBLayout::AddBB");
  ASSERT(!laidOut[bb.GetBBId()], "AddBB: bb already laid out");
  layoutBBs.push_back(&bb);
  laidOut[bb.GetBBId()] = true;
  if (enabledDebug) {
    LogInfo::MapleLogger() << "bb id " << bb.GetBBId() << " kind is " << bb.StrAttribute();
  }
  bool isTry = false;
  if (func.GetIRMap() != nullptr) {
    isTry = !bb.GetMeStmts().empty() && bb.GetMeStmts().front().GetOp() == OP_try;
  } else {
    isTry = !bb.GetStmtNodes().empty() && bb.GetStmtNodes().front().GetOpCode() == OP_try;
  }
  if (isTry) {
    ASSERT(!tryOutstanding, "BBLayout::AddBB: cannot lay out another try without ending the last one");
    tryOutstanding = true;
    if (enabledDebug) {
      LogInfo::MapleLogger() << " try";
    }
  }
  if (bb.GetAttributes(kBBAttrIsTryEnd) && func.GetMIRModule().IsJavaModule()) {
    tryOutstanding = false;
    if (enabledDebug) {
      LogInfo::MapleLogger() << " endtry";
    }
  }
  if (enabledDebug) {
    LogInfo::MapleLogger() << '\n';
  }
  // If the pred bb is goto bb and the target bb of goto bb is the current bb which is be added to layoutBBs, change the
  // goto bb to fallthru bb.
  if (layoutBBs.size() > 1) {
    BB *predBB = layoutBBs.at(layoutBBs.size() - 2); // Get the pred of bb.
    if (predBB->GetKind() != kBBGoto) {
      return;
    }
    if (func.GetIRMap() != nullptr) {
      if (predBB->GetLastMe()->GetOp() == OP_throw) {
        return;
      }
    } else {
      if (predBB->GetLast().GetOpCode() == OP_throw) {
        return;
      }
    }
    if (predBB->GetSucc().front() != &bb) {
      return;
    }
    ChangeToFallthruFromGoto(*predBB);
  }
}

BB *BBLayout::GetFallThruBBSkippingEmpty(BB &bb) {
  ASSERT(bb.GetKind() == kBBFallthru || bb.GetKind() == kBBCondGoto, "GetFallThruSkippingEmpty: unexpected BB kind");
  ASSERT(!bb.GetSucc().empty(), "container check");
  BB *fallthru = bb.GetSucc().front();
  do {
    if (fallthru->GetPred().size() > 1 || fallthru->GetAttributes(kBBAttrIsTryEnd)) {
      return fallthru;
    }
    if (func.GetIRMap() != nullptr) {
      if (!fallthru->IsMeStmtEmpty() && !BBContainsOnlyGoto(*fallthru)) {
        return fallthru;
      }
    } else {
      if (!fallthru->IsEmpty()) {
        return fallthru;
      }
    }
    // in case of duplicate succ
    if (fallthru->GetSucc().front() == bb.GetSucc().back()) {
      return fallthru;
    }
    laidOut[fallthru->GetBBId()] = true;
    BB *oldFallThru = fallthru;
    fallthru = fallthru->GetSucc().front();
    bb.ReplaceSucc(oldFallThru, fallthru);
    if (oldFallThru->GetPred().empty()) {
      RemoveUnreachable(*oldFallThru);
      if (needDealWithTryBB) {
        DealWithStartTryBB();
      }
    }
  } while (true);
}

// bb end with a goto statement; remove the goto stmt if its target
// is its fallthru nextBB.
void BBLayout::ChangeToFallthruFromGoto(BB &bb) {
  ASSERT(bb.GetKind() == kBBGoto, "ChangeToFallthruFromGoto: unexpected BB kind");
  if (func.GetIRMap() != nullptr) {
    bb.RemoveMeStmt(to_ptr(bb.GetMeStmts().rbegin()));
  } else {
    bb.RemoveLastStmt();
  }
  bb.SetKind(kBBFallthru);
}

void BBLayout::ChangeToFallthruFromCondGoto(BB &bb) {
  ASSERT(bb.GetKind() == kBBCondGoto, "ChangeToFallthruFromCondGoto: unexpected BB kind");
  if (func.GetIRMap() != nullptr) {
    bb.RemoveMeStmt(to_ptr(bb.GetMeStmts().rbegin()));
  } else {
    bb.RemoveLastStmt();
  }
  bb.SetKind(kBBFallthru);
}

// bb does not end with a branch statement; if its fallthru is not nextBB,
// perform the fix by either laying out the fallthru immediately or adding a goto
void BBLayout::ResolveUnconditionalFallThru(BB &bb, BB &nextBB) {
  if (bb.GetKind() != kBBFallthru) {
    return;
  }
  ASSERT(bb.GetAttributes(kBBAttrIsTry) || bb.GetAttributes(kBBAttrWontExit) || bb.GetSucc().size() == 1,
         "runtime check error");
  BB *fallthru = GetFallThruBBSkippingEmpty(bb);
  if (fallthru != &nextBB) {
    if (BBCanBeMoved(*fallthru, bb)) {
      AddBB(*fallthru);
      SetAttrTryForTheCanBeMovedBB(bb, *fallthru);
      ResolveUnconditionalFallThru(*fallthru, nextBB);
      OptimizeBranchTarget(*fallthru);
    } else {
      CreateGoto(bb, func, *fallthru);
      OptimizeBranchTarget(bb);
    }
  }
}

void BBLayout::FixEndTryBB(BB &bb) {
  BBId prevID = bb.GetBBId() - 1UL;
  for (BBId id = prevID; id != 0; --id) {
    auto prevBB = func.GetCfg()->GetBBFromID(id);
    if (prevBB != nullptr) {
      if (prevBB->GetAttributes(kBBAttrIsTry) && !prevBB->GetAttributes(kBBAttrIsTryEnd)) {
        prevBB->SetAttributes(kBBAttrIsTryEnd);
        cfg->SetTryBBByOtherEndTryBB(prevBB, &bb);
      }
      break;
    }
  }
}

void BBLayout::FixTryBB(BB &startTryBB, BB &nextBB) {
  startTryBB.RemoveAllPred();
  for (size_t i = 0; i < nextBB.GetPred().size(); ++i) {
    nextBB.GetPred(i)->ReplaceSucc(&nextBB, &startTryBB);
  }
  nextBB.RemoveAllPred();
  ASSERT(startTryBB.GetSucc().empty(), "succ of try should have been removed");
  startTryBB.AddSucc(nextBB);
}

void BBLayout::DealWithStartTryBB() {
  size_t size = startTryBBVec.size();
  for (size_t i = 0; i < size; ++i) {
    if (!startTryBBVec[i]) {
      continue;
    }
    auto curBB = cfg->GetBBFromID(BBId(i));
    for (size_t j = i + 1; j < size && !startTryBBVec[j]; ++j) {
      auto nextBB = cfg->GetBBFromID(BBId(j));
      if (nextBB != nullptr) {
        if (nextBB->GetAttributes(kBBAttrIsTry)) {
          FixTryBB(*curBB, *nextBB);
        } else {
          curBB->RemoveAllSucc();
          cfg->NullifyBBByID(curBB->GetBBId());
          for (auto it = layoutBBs.begin(); it != layoutBBs.end(); ++it) {
            if (*it == curBB) {
              layoutBBs.erase(it);
              break;
            }
          }
        }
        break;
      } else if (j == size - 1) {
        curBB->RemoveAllSucc();
        cfg->NullifyBBByID(curBB->GetBBId());
        for (auto it = layoutBBs.begin(); it != layoutBBs.end(); ++it) {
          if (*it == curBB) {
            layoutBBs.erase(it);
            break;
          }
        }
      }
    }
    startTryBBVec[i] = false;
  }
  needDealWithTryBB = false;
}

// remove unnessary bb whose pred size is zero
// keep cfg correct to rebuild dominance
void BBLayout::RemoveUnreachable(BB &bb) {
  if (bb.GetAttributes(kBBAttrIsEntry)) {
    return;
  }

  while (!bb.GetSucc().empty()) {
    BB *succ = bb.GetSucc(0);
    succ->RemovePred(bb, false);
    if (succ->GetPred().empty()) {
      RemoveUnreachable(*succ);
    }
  }

  if (bb.GetAttributes(kBBAttrIsTry) && !bb.GetAttributes(kBBAttrIsTryEnd)) {
    // identify if try bb is the start try bb
    if (!bb.GetMeStmts().empty() && bb.GetMeStmts().front().GetOp() == OP_try) {
      startTryBBVec[bb.GetBBId()] = true;
      needDealWithTryBB = true;
      return;
    }
  }
  if (bb.GetAttributes(kBBAttrIsTryEnd)) {
    FixEndTryBB(bb);
  }
  bb.RemoveAllSucc();
  cfg->NullifyBBByID(bb.GetBBId());
  for (auto it = layoutBBs.begin(); it != layoutBBs.end(); ++it) {
    if (*it == &bb) {
      layoutBBs.erase(it);
      break;
    }
  }
}


void BBLayout::UpdateNewBBWithAttrTry(const BB &bb, BB &fallthru) const {
  auto tryBB = &bb;
  fallthru.SetAttributes(kBBAttrIsTry);
  if (bb.IsReturnBB()) {
    int i = 1;
    tryBB = cfg->GetBBFromID(bb.GetBBId() - i);
    while (tryBB == nullptr || tryBB->IsReturnBB()) {
      ++i;
      tryBB = cfg->GetBBFromID(bb.GetBBId() - i);
    }
    ASSERT_NOT_NULL(tryBB);
    ASSERT(tryBB->GetAttributes(kBBAttrIsTry), "must be try");
  }
  bool setEHEdge = false;
  for (auto *candCatch : tryBB->GetSucc()) {
    if (candCatch != nullptr && candCatch->GetAttributes(kBBAttrIsCatch)) {
      setEHEdge = true;
      fallthru.AddSucc(*candCatch);
    }
  }
  ASSERT(setEHEdge, "must set eh edge");
}

// create a new fallthru that contains a goto to the original fallthru
// bb              bb
//  |     ====>    |
//  |              [NewCreated]
//  |              |
// fallthru       fallthru
BB *BBLayout::CreateGotoBBAfterCondBB(BB &bb, BB &fallthru) {
  ASSERT(bb.GetKind() == kBBCondGoto, "CreateGotoBBAfterCondBB: unexpected BB kind");
  BB *newFallthru = cfg->NewBasicBlock();
  bbVisited.push_back(false);
  newFallthru->SetAttributes(kBBAttrArtificial);
  AddLaidOut(false);
  newFallthru->SetKind(kBBGoto);
  SetNewBBInLayout();
  LabelIdx fallthruLabel = func.GetOrCreateBBLabel(fallthru);
  if (func.GetIRMap() != nullptr) {
    GotoNode stmt(OP_goto);
    auto *newGoto = func.GetIRMap()->New<GotoMeStmt>(&stmt);
    newGoto->SetOffset(fallthruLabel);
    newFallthru->SetFirstMe(newGoto);
    newFallthru->SetLastMe(to_ptr(newFallthru->GetMeStmts().begin()));
  } else {
    auto *newGoto = func.GetMirFunc()->GetCodeMempool()->New<GotoNode>(OP_goto);
    newGoto->SetOffset(fallthruLabel);
    newFallthru->SetFirst(newGoto);
    newFallthru->SetLast(newFallthru->GetStmtNodes().begin().d());
  }
  // replace pred and succ
  size_t index = fallthru.GetPred().size();
  while (index > 0) {
    if (fallthru.GetPred(index - 1) == &bb) {
      break;
    }
    index--;
  }
  bb.ReplaceSucc(&fallthru, newFallthru);
  // pred has been remove for pred vector of succ
  // means size reduced, so index reduced
  index--;
  fallthru.AddPred(*newFallthru, index);
  newFallthru->SetFrequency(fallthru.GetFrequency());
  if (enabledDebug) {
    LogInfo::MapleLogger() << "Created fallthru and goto original fallthru" << '\n';
  }
  AddBB(*newFallthru);
  SetAttrTryForTheCanBeMovedBB(bb, *newFallthru);
  return newFallthru;
}

void BBLayout::OptimizeEmptyFallThruBB(BB &bb) {
  if (needDealWithTryBB) { return; }
  auto *fallthru = bb.GetSucc().front();
  if (fallthru && fallthru->GetBBLabel() == 0 &&
      (BBEmptyAndFallthru(*fallthru) || BBContainsOnlyGoto(*fallthru))) {
    if (fallthru->GetSucc().front() == bb.GetSucc().back()) {
      bb.ReplaceSucc(fallthru, bb.GetSucc().back());
      ASSERT(fallthru->GetPred().empty(), "fallthru should not has other pred");
      ChangeToFallthruFromCondGoto(bb);
      bb.GetSucc().resize(1); // resize succ to 1
      laidOut[fallthru->GetBBId()] = true;
      RemoveUnreachable(*fallthru);
    }
  }
}

void BBLayout::DumpBBPhyOrder() const {
  LogInfo::MapleLogger() << func.GetName() << " final BB order " <<  '\n';
  for (auto bb : layoutBBs) {
    LogInfo::MapleLogger() << bb->GetBBId();
    if (bb != layoutBBs.back()) {
      LogInfo::MapleLogger() << "-->";
    }
  }
  LogInfo::MapleLogger() << '\n';
}

void BBLayout::OptimiseCFG() {
  auto eIt = cfg->valid_end();
  for (auto bIt = cfg->valid_begin(); bIt != eIt; ++bIt) {
    if (bIt == cfg->common_entry() || bIt == cfg->common_exit()) {
      continue;
    }
    auto *bb = *bIt;
    if (bb->GetKind() == kBBCondGoto || bb->GetKind() == kBBGoto) {
      OptimizeBranchTarget(*bb);
      // check fallthru is empty without label, delete fallthru and
      // make fallthrSucc as the only succ of bb
      //      bb
      //      /      \
      //   fallthru  |
      //      \      /
      //      fallthruSucc
      if (bb->GetKind() == kBBCondGoto) {
        OptimizeEmptyFallThruBB(*bb);
      }
    }
  }
  (void)cfg->UnreachCodeAnalysis(false);
}

void BBLayout::SetAttrTryForTheCanBeMovedBB(BB &bb, BB &canBeMovedBB) const {
  if (bb.GetAttributes(kBBAttrIsTryEnd)) {
    bb.ClearAttributes(kBBAttrIsTryEnd);
    canBeMovedBB.SetAttributes(kBBAttrIsTryEnd);
    cfg->SetTryBBByOtherEndTryBB(&canBeMovedBB, &bb);
  }
  if (bb.GetAttributes(kBBAttrIsTry) && !canBeMovedBB.GetAttributes(kBBAttrIsTry)) {
    UpdateNewBBWithAttrTry(bb, canBeMovedBB);
  }
}

void BBLayout::LayoutWithoutProf() {
  RebuildFreq();
  BB *bb = cfg->GetFirstBB();
  while (bb != nullptr) {
    AddBB(*bb);
    if (bb->GetKind() == kBBCondGoto || bb->GetKind() == kBBGoto) {
      OptimizeBranchTarget(*bb);
    }
    BB *nextBB = NextBB();
    if (nextBB != nullptr) {
      // check try-endtry correspondence
      bool isTry = false;
      if (func.GetIRMap() != nullptr) {
        isTry = !nextBB->GetMeStmts().empty() && nextBB->GetMeStmts().front().GetOp() == OP_try;
      } else {
        auto &stmtNodes = nextBB->GetStmtNodes();
        isTry = !stmtNodes.empty() && stmtNodes.front().GetOpCode() == OP_try;
      }
      ASSERT(!(isTry && GetTryOutstanding()), "cannot emit another try if last try has not been ended");
      if (nextBB->GetAttributes(kBBAttrIsTryEnd)) {
        ASSERT(cfg->GetTryBBFromEndTryBB(nextBB) == nextBB ||
               IsBBLaidOut(cfg->GetTryBBFromEndTryBB(nextBB)->GetBBId()),
               "cannot emit endtry bb before its corresponding try bb");
      }
    }
    // based on nextBB, may need to fix current bb's fall-thru
    if (bb->GetKind() == kBBFallthru) {
      ResolveUnconditionalFallThru(*bb, *nextBB);
    } else if (bb->GetKind() == kBBCondGoto) {
      BB *oldFallThru = bb->GetSucc(0);
      BB *fallthru = GetFallThruBBSkippingEmpty(*bb);
      BB *brTargetBB = bb->GetSucc(1);
      if (ChooseTargetAsFallthru(*bb, *brTargetBB, *oldFallThru, *fallthru)) {
        // flip the sense of the condgoto and lay out brTargetBB right here
        LabelIdx fallthruLabel = func.GetOrCreateBBLabel(*fallthru);
        if (func.GetIRMap() != nullptr) {
          auto &condGotoMeStmt = static_cast<CondGotoMeStmt&>(bb->GetMeStmts().back());
          ASSERT(brTargetBB->GetBBLabel() == condGotoMeStmt.GetOffset(), "bbLayout: wrong branch target BB");
          condGotoMeStmt.SetOffset(fallthruLabel);
          condGotoMeStmt.SetOp((condGotoMeStmt.GetOp() == OP_brtrue) ? OP_brfalse : OP_brtrue);
        } else {
          auto &condGotoNode = static_cast<CondGotoNode&>(bb->GetStmtNodes().back());
          ASSERT(brTargetBB->GetBBLabel() == condGotoNode.GetOffset(), "bbLayout: wrong branch target BB");
          condGotoNode.SetOffset(fallthruLabel);
          condGotoNode.SetOpCode((condGotoNode.GetOpCode() == OP_brtrue) ? OP_brfalse : OP_brtrue);
        }
        // The offset of condgotostmt must be the label of second succ bb.
        ASSERT(bb->GetSucc(0)->GetBBId() == fallthru->GetBBId(), "must be the fallthru");
        ASSERT(bb->GetSucc(1)->GetBBId() == brTargetBB->GetBBId(), "must be the brTargetBB");
        bb->SetSucc(0, brTargetBB);
        bb->SetSucc(1, fallthru);
        if (func.GetIRMap() != nullptr) {
          ASSERT(bb->GetSucc(1)->GetBBLabel() == static_cast<CondGotoMeStmt&>(bb->GetMeStmts().back()).GetOffset(),
                 "bbLayout: wrong branch target BB");
        } else {
          ASSERT(bb->GetSucc(1)->GetBBLabel() == static_cast<CondGotoNode&>(bb->GetStmtNodes().back()).GetOffset(),
                 "bbLayout: wrong branch target BB");
        }
        AddBB(*brTargetBB);
        SetAttrTryForTheCanBeMovedBB(*bb, *brTargetBB);
        ResolveUnconditionalFallThru(*brTargetBB, *nextBB);
        OptimizeBranchTarget(*brTargetBB);
      } else if (fallthru != nextBB) {
        if (BBCanBeMoved(*fallthru, *bb)) {
          AddBB(*fallthru);
          SetAttrTryForTheCanBeMovedBB(*bb, *fallthru);
          ResolveUnconditionalFallThru(*fallthru, *nextBB);
          OptimizeBranchTarget(*fallthru);
        } else {
          BB *newFallthru = CreateGotoBBAfterCondBB(*bb, *fallthru);
          OptimizeBranchTarget(*newFallthru);
        }
      }
    } else if (bb->GetKind() == kBBSwitch) {
      if (func.GetIRMap() != nullptr) {
        SwitchMeStmt *swStmt = &static_cast<SwitchMeStmt&>(bb->GetMeStmts().back());
        OptimizeCaseTargets(bb, &swStmt->GetSwitchTable());
      } else {
        SwitchNode *swStmt = &static_cast<SwitchNode&>(bb->GetStmtNodes().back());
        OptimizeCaseTargets(bb, &swStmt->GetSwitchTable());
      }
    }
    if (bb->GetKind() == kBBGoto) {
      // see if goto target can be placed here
      BB *gotoTarget = bb->GetSucc().front();
      CHECK_FATAL(gotoTarget != nullptr, "null ptr check");

      if (gotoTarget != nextBB && BBCanBeMoved(*gotoTarget, *bb)) {
        SetAttrTryForTheCanBeMovedBB(*bb, *gotoTarget);
        ChangeToFallthruFromGoto(*bb);
        AddBB(*gotoTarget);
        ResolveUnconditionalFallThru(*gotoTarget, *nextBB);
        OptimizeBranchTarget(*gotoTarget);
      } else if (gotoTarget->GetKind() == kBBCondGoto && gotoTarget->GetPred().size() == 1) {
        BB *targetNext = gotoTarget->GetSucc().front();
        if (targetNext != nextBB && BBCanBeMoved(*targetNext, *bb)) {
          SetAttrTryForTheCanBeMovedBB(*bb, *gotoTarget);
          ChangeToFallthruFromGoto(*bb);
          AddBB(*gotoTarget);
          OptimizeBranchTarget(*gotoTarget);
          AddBB(*targetNext);
          SetAttrTryForTheCanBeMovedBB(*bb, *targetNext);
          ResolveUnconditionalFallThru(*targetNext, *nextBB);
          OptimizeBranchTarget(*targetNext);
        }
      }
    }
    if (nextBB != nullptr && IsBBLaidOut(nextBB->GetBBId())) {
      nextBB = NextBB();
    }
    bb = nextBB;
  }
}

void BBLayout::AddBBProf(BB &bb) {
  if (layoutBBs.empty()) {
    AddBB(bb);
    return;
  }
  BB *curBB = layoutBBs.back();
  if ((curBB->GetKind() == kBBFallthru || curBB->GetKind() == kBBGoto) && !curBB->GetSucc().empty()) {
    BB *targetBB = curBB->GetSucc().front();
    if (curBB->GetKind() == kBBFallthru && (&bb != targetBB)) {
      CreateGoto(*curBB, func, *targetBB);
    } else if (curBB->GetKind() == kBBGoto && (&bb == targetBB)) {
      // delete the goto stmt
      ChangeToFallthruFromGoto(*curBB);
    }
  } else if (curBB->GetKind() == kBBCondGoto) {
    BB *fallthru = curBB->GetSucc(0);
    BB *targetBB = curBB->GetSucc(1);
    CHECK_FATAL(targetBB != fallthru, "condbb targetBB is same as fallthru");
    if (targetBB == &bb) {
      LabelIdx fallthruLabel = func.GetOrCreateBBLabel(*fallthru);
      if (func.GetIRMap() != nullptr) {
        auto &condGotoMeStmt = static_cast<CondGotoMeStmt&>(curBB->GetMeStmts().back());
        ASSERT(targetBB->GetBBLabel() == condGotoMeStmt.GetOffset(), "bbLayout: wrong branch target BB");
        condGotoMeStmt.SetOffset(fallthruLabel);
        condGotoMeStmt.SetOp((condGotoMeStmt.GetOp() == OP_brtrue) ? OP_brfalse : OP_brtrue);
      } else {
        auto &condGotoNode = static_cast<CondGotoNode&>(curBB->GetStmtNodes().back());
        ASSERT(targetBB->GetBBLabel() == condGotoNode.GetOffset(), "bbLayout: wrong branch target BB");
        condGotoNode.SetOffset(fallthruLabel);
        condGotoNode.SetOpCode((condGotoNode.GetOpCode() == OP_brtrue) ? OP_brfalse : OP_brtrue);
      }
    } else if (&bb != fallthru) {
      (void)CreateGotoBBAfterCondBB(*curBB, *fallthru);
    }
  }
  AddBB(bb);
}

void BBLayout::BuildEdges() {
  auto eIt = cfg->valid_end();
  for (auto bIt = cfg->valid_begin(); bIt != eIt; ++bIt) {
    if (bIt == cfg->common_entry() || bIt == cfg->common_exit()) {
      continue;
    }
    auto *bb = *bIt;
    for (size_t i = 0; i < bb->GetSucc().size(); ++i) {
      BB *dest = bb->GetSucc(i);
      uint64 w = bb->GetEdgeFreq(i);
      allEdges.emplace_back(layoutAlloc.GetMemPool()->New<BBEdge>(bb, dest, w));
    }
  }
  std::stable_sort(allEdges.begin(), allEdges.end(), [](const BBEdge *edge1, const BBEdge *edge2) {
      return edge1->GetWeight() > edge2->GetWeight(); });
}

BB *BBLayout::GetBBFromEdges() {
  while (edgeIdx < allEdges.size()) {
    BBEdge *edge = allEdges[edgeIdx];
    BB *srcBB = edge->GetSrcBB();
    BB *destBB = edge->GetDestBB();
    if (enabledDebug) {
      LogInfo::MapleLogger() << srcBB->GetBBId() << "->" << destBB->GetBBId() << " freq "
                             << srcBB->GetEdgeFreq(destBB) << '\n';
    }

    if (!laidOut[srcBB->GetBBId()]) {
      if (enabledDebug) {
        LogInfo::MapleLogger() << "choose srcBB " << srcBB->GetBBId() << '\n';
      }
      return srcBB;
    }
    if (!laidOut[destBB->GetBBId()]) {
      if (enabledDebug) {
        LogInfo::MapleLogger() << "choose destBB " << destBB->GetBBId() << '\n';
      }
      return destBB;
    }
    if (enabledDebug) {
      LogInfo::MapleLogger() << "skip this edge " << '\n';
    }
    edgeIdx++;
  }
  return nullptr;
}
// find the most probility succ,if bb's succ more then one
// if no succ,choose the most weight edge in the edges
BB *BBLayout::NextBBProf(BB &bb) {
  if (bb.GetSucc().size() == 0) {
    return GetBBFromEdges();
  }

  if (bb.GetSucc().size() == 1) {
    BB *succBB = bb.GetSucc(0);
    if (!laidOut[succBB->GetBBId()]) {
      return succBB;
    }
    return NextBBProf(*succBB);
  }
  // max freq intial
  uint64 maxFreq  = 0;
  size_t idx = 0;
  bool found = false;
  for (size_t i = 0; i < bb.GetSucc().size(); ++i) {
    BB *succBB = bb.GetSucc(i);
    if (!laidOut[succBB->GetBBId()]) {
      uint64 edgeFreqFromBB = bb.GetEdgeFreq(i);
      // if bb isn't executed, choose the first valid bb
      if (bb.GetFrequency() == 0) {
        idx = i;
        found = true;
        break;
      }
      if (edgeFreqFromBB > maxFreq) {
        maxFreq = edgeFreqFromBB;
        idx = i;
        found = true;
      }
    }
  }
  if (found) {
    return bb.GetSucc(idx);
  }
  return GetBBFromEdges();
}

void BBLayout::RebuildFreq() {
  auto *hook = phase->GetAnalysisInfoHook();
  hook->ForceEraseAnalysisPhase(func.GetUniqueID(), &MEDominance::id);
  hook->ForceEraseAnalysisPhase(func.GetUniqueID(), &MELoopAnalysis::id);
  dom = static_cast<MEDominance*>(
      hook->ForceRunAnalysisPhase<MapleFunctionPhase<MeFunction>>(&MEDominance::id, func))->GetResult();
  meLoop = static_cast<MELoopAnalysis*>(
      hook->ForceRunAnalysisPhase<MapleFunctionPhase<MeFunction>>(&MELoopAnalysis::id, func))->GetResult();
  MePrediction::RebuildFreq(func, *dom, *meLoop);
}

void BBLayout::LayoutWithProf(bool useChainLayout) {
  OptimiseCFG();
  // We rebuild freq after OptimiseCFG
  RebuildFreq();
  if (enabledDebug) {
    cfg->DumpToFile("cfgopt", false, true);
  }

  BB *bb = cfg->GetFirstBB();
  if (useChainLayout) {  // chain BB layout
    BuildChainForFunc();
    BBChain *mainChain = bb2chain[bb->GetBBId()];
    for (auto it = mainChain->begin(); it != mainChain->end(); ++it) {
      if (it == mainChain->begin()) {
        continue;  // skip common entry BB
      }
      AddBBProf(**it);
    }
  } else {  // PH BB layout
    BuildEdges();
    while (bb != nullptr) {
      AddBBProf(*bb);
      bb = NextBBProf(*bb);
    }
  }

  // adjust the last BB if kind is fallthru or condtion BB
  BB *lastBB = layoutBBs.empty() ? nullptr : layoutBBs.back();
  if (lastBB != nullptr) {
    if (lastBB->GetKind() == kBBFallthru) {
      if (!lastBB->IsPredBB(*cfg->GetCommonExitBB())) {
        BB *targetBB = lastBB->GetSucc().front();
        CreateGoto(*lastBB, func, *targetBB);
      }
    } else if (lastBB->GetKind() == kBBCondGoto) {
      BB *fallthru = lastBB->GetSucc(0);
      (void)CreateGotoBBAfterCondBB(*lastBB, *fallthru);
    }
  }
}

void BBLayout::RunLayout() {
  // If the func is too large, won't run prediction
  if (MeOption::layoutWithPredict && func.GetMIRModule().IsCModule() && cfg->GetAllBBs().size() <= kMaxNumBBToPredict) {
    LayoutWithProf(true);
  } else {
    LayoutWithoutProf();
  }
}

void MEBBLayout::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEMeCfg>();
  aDep.AddRequired<MELoopAnalysis>();
  aDep.SetPreservedAll();
}

bool MEBBLayout::PhaseRun(maple::MeFunction &f) {
  MeCFG *cfg = GET_ANALYSIS(MEMeCfg, f);
  auto *bbLayout = GetPhaseAllocator()->New<BBLayout>(*GetPhaseMemPool(), f, DEBUGFUNC_NEWPM(f), this);
  // assume common_entry_bb is always bb 0
  ASSERT(cfg->front() == cfg->GetCommonEntryBB(), "assume bb[0] is the commont entry bb");
  if (DEBUGFUNC_NEWPM(f)) {
    cfg->DumpToFile("beforeBBLayout", false);
  }
  bbLayout->RunLayout();
  f.SetLaidOutBBs(bbLayout->GetBBs());
  if (DEBUGFUNC_NEWPM(f)) {
    bbLayout->DumpBBPhyOrder();
    cfg->DumpToFile("afterBBLayout", false);
  }
  return true;
}
}  // namespace maple
