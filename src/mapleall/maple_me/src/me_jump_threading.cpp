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
#include "me_jump_threading.h"
#include "me_value_range_prop.h"
#include "me_dominance.h"
#include "me_ssa_update.h"
#include "me_phase_manager.h"

namespace maple {
constexpr size_t kNumOperands = 2;
bool JumpThreading::isDebug = false;
size_t JumpThreading::pathSize = 0;
int64 JumpThreading::codeSizeOfCopy = 0;
constexpr int64 kCodeSizeLimit = 10; // Copy up to 10 statements per path.
constexpr size_t kLimitOfVisitedBB = 40; // A maximum of 40 bbs can be traversed when recursively finding a def point.
constexpr size_t kLimitOfPathLength = 12; // The maximum length of each path is 12 bbs.
constexpr size_t kLimitOfPathsSize = 20; // Each function can optimize up to 20 paths.
constexpr int64 kModuleCodeSizeLimit = 155;
constexpr size_t kLimitOfModulePathSize = 25;

#define DEBUG_LOG() if (JumpThreading::isDebug)     \
  LogInfo::MapleLogger()

void JumpThreading::Execute() {
  for (auto bIt = dom.GetReversePostOrder().begin(); bIt != dom.GetReversePostOrder().end(); ++bIt) {
    auto bb = func.GetCfg()->GetBBFromID(BBId((*bIt)->GetID()));
    switch (bb->GetKind()) {
      case kBBCondGoto: {
        DealWithCondGoto(*bb);
        break;
      }
      case kBBSwitch: {
        DealWithSwitch(*bb);
        break;
      }
      default:
        break;
    }
  }
  DEBUG_LOG() << "=========dump path begin=========\n";
  if (JumpThreading::isDebug) {
    for (auto &pathTmp : paths) {
      for (auto *bb : *pathTmp) {
        DEBUG_LOG() << bb->GetBBId() << " ";
      }
      DEBUG_LOG() << "\n";
    }
  }
  DEBUG_LOG() << "=========dump path end=========\n";
  if (paths.empty()) {
    return;
  }
  ExecuteJumpThreading();
}

// Insert the ost of phi opnds to their def bbs.
void JumpThreading::InsertOstOfPhi2Cands(BB &bb, size_t i) {
  for (auto &it : bb.GetMePhiList()) {
    if (i >= it.second->GetOpnds().size()) {
      break;
    }
    auto *opnd = it.second->GetOpnd(i);
    MeStmt *stmt = nullptr;
    auto *defBB = opnd->GetDefByBBMeStmt(*func.GetCfg(), stmt);
    // At the time of optimizing redundant branches, when the all preds of condgoto BB can jump directly to
    // the successors of condgoto BB and the conditional expr is only used for conditional stmt, after opt,
    // the ssa of expr not need to be updated, otherwise the ssa of epr still needs to be updated.
    MeSSAUpdate::InsertOstToSSACands(it.first, *defBB, &cands);
  }
}

void JumpThreading::PrepareForSSAUpdateWhenPredBBIsRemoved(const BB &pred, BB &bb) {
  int index = bb.GetPredIndex(pred);
  CHECK_FATAL(index != -1, "pred is not in preds of bb");
  InsertOstOfPhi2Cands(bb, static_cast<size_t>(static_cast<int64>(index)));
  MeSSAUpdate::InsertDefPointsOfBBToSSACands(bb, cands);
}

BB *JumpThreading::CopyBB(BB &bb, bool copyWithoutLastStmt) {
  // Each wont exit bb corresponds to an wont exit return bb,
  // so when the bb to be copied is an wont exit bb, copy the bb.
  bool isWontExitReturnBB = bb.GetAttributes(kBBAttrWontExit | kBBAttrIsExit) && bb.GetKind() == kBBReturn;
  if (!isWontExitReturnBB && bb.GetMeStmts().empty()) {
    return nullptr;
  }
  size_t numOfStmts = 0;
  for (auto &stmt : bb.GetMeStmts()) {
    if (stmt.GetOp() == OP_comment) {
      continue;
    }
    numOfStmts++;
  }
  // Need not copy the condition stmt which would be jumped after opt.
  if (!isWontExitReturnBB && (numOfStmts == 0 || (copyWithoutLastStmt && (numOfStmts == 1)))) {
    return nullptr;
  }
  BB *newBB = func.GetCfg()->NewBasicBlock();
  if (copyWithoutLastStmt) {
    newBB->SetKind(kBBFallthru);
  } else {
    newBB->SetKind(bb.GetKind());
  }
  newBB->SetAttributes(bb.GetAttributes());
  func.CloneBBMeStmts(bb, *newBB, &cands, copyWithoutLastStmt);
  return newBB;
}

void JumpThreading::SetNewOffsetOfLastMeStmtForNewBB(const BB &oldBB, BB &newBB, const BB &oldSucc, BB &newSucc) {
  switch (newBB.GetKind()) {
    case kBBGoto: {
      auto *oldLastMe = static_cast<const GotoMeStmt*>(oldBB.GetLastMe());
      auto *newLastMe = static_cast<GotoMeStmt*>(newBB.GetLastMe());
      if (oldLastMe->GetOffset() == oldSucc.GetBBLabel()) {
        ASSERT_NOT_NULL(newLastMe);
        newLastMe->SetOffset(func.GetOrCreateBBLabel(newSucc));
      }
      break;
    }
    case kBBCondGoto: {
      auto *oldLastMe = static_cast<const CondGotoMeStmt*>(oldBB.GetLastMe());
      auto *newLastMe = static_cast<CondGotoMeStmt*>(newBB.GetLastMe());
      if (oldLastMe->GetOffset() == oldSucc.GetBBLabel()) {
        newLastMe->SetOffset(func.GetOrCreateBBLabel(newSucc));
      }
      break;
    }
    case kBBSwitch: {
      auto *oldLastMe = static_cast<const SwitchMeStmt*>(oldBB.GetLastMe());
      auto *newLastMe = static_cast<SwitchMeStmt*>(newBB.GetLastMe());
      if (oldLastMe->GetDefaultLabel() == oldSucc.GetBBLabel()) {
        newLastMe->SetDefaultLabel(func.GetOrCreateBBLabel(newSucc));
      }
      for (size_t i = 0; i < oldLastMe->GetSwitchTable().size(); ++i) {
        LabelIdx labelIdx = oldLastMe->GetSwitchTable().at(i).second;
        if (labelIdx == oldSucc.GetBBLabel()) {
          newLastMe->SetCaseLabel(i, func.GetOrCreateBBLabel(newSucc));
        }
      }
      break;
    }
    default: {
      break;
    }
  }
}

void JumpThreading::ConnectNewPath(std::vector<BB*> &currPath, std::vector<std::pair<BB*, BB*>> &old2NewBB,
    size_t thePosOfSec2LastBB) {
  size_t idxOfCurrBB = 0;
  auto *currentBB = currPath[0];
  auto *currentSucc = currPath[1];
  for (size_t i = 1; i < currPath.size(); ++i) {
    auto *newSuccBB = old2NewBB[i].second;
    if (newSuccBB == nullptr) {
      continue;
    }
    if (idxOfCurrBB == thePosOfSec2LastBB) {
      old2NewBB[idxOfCurrBB].second->AddSucc(*newSuccBB);
      if (func.GetCfg()->UpdateCFGFreq()) {
        FreqType freqUsed = old2NewBB[idxOfCurrBB].second->GetFrequency();
        old2NewBB[idxOfCurrBB].second->PushBackSuccFreq(freqUsed);
        newSuccBB->SetFrequency(freqUsed);
      }
      break;
    }
    for (auto *temp : currentBB->GetSucc()) {
      auto *newTemp = temp;
      if (temp == currentSucc) {
        // If the succ of currentBB is in path, replace it with the new copied bb.
        // For example:
        // Current path has bb0, bb2, bb4, bb5, when currentBB is bb2, and temp bb is bb4,
        // need push the new copied bb of bb4 to the succs of bb2'(copied from bb2).
        //  bb0  bb1
        //    \ /
        //    bb2
        //    / \
        //  bb3 bb4
        //        \
        //        bb5
        newTemp = newSuccBB;
        if (currentBB == old2NewBB[idxOfCurrBB].second) {
          currentBB->ReplaceSucc(temp, newTemp, true);
          if (func.GetCfg()->UpdateCFGFreq()) {
            int idxInSucc = currentBB->GetSuccIndex(*newTemp);
            CHECK_FATAL(idxInSucc != -1, "idxInSucc can not be -1");
            newTemp->SetFrequency(currentBB->GetSuccFreq()[static_cast<uint32>(idxInSucc)]);
          }
        } else {
          old2NewBB[idxOfCurrBB].second->AddSucc(*newTemp);
          if (func.GetCfg()->UpdateCFGFreq()) {
            old2NewBB[idxOfCurrBB].second->PushBackSuccFreq(0);
            newTemp->SetFrequency(0);
          }
        }
      } else if (currentBB != old2NewBB[idxOfCurrBB].second) {
        // Deal with the case like the succ of currentBB is not in path.
        for (size_t idx = 0; idx < newTemp->GetPred().size(); ++idx) {
          InsertOstOfPhi2Cands(*newTemp, idx);
        }
        // Push the temp to the succ of new copied bb of currentBB.
        if (currentBB->GetKind() == kBBGoto && currentBB->GetAttributes(kBBAttrWontExit) &&
            currentBB->GetSuccIndex(*newTemp) == 1 && newTemp->GetAttributes(kBBAttrIsExit)) {
          BB *newWontExitBB = CopyBB(*newTemp);
          old2NewBB[idxOfCurrBB].second->AddSucc(*newWontExitBB);
          func.GetCfg()->GetCommonExitBB()->GetPred().push_back(newWontExitBB);
        } else {
          old2NewBB[idxOfCurrBB].second->AddSucc(*newTemp);
        }
        if (func.GetCfg()->UpdateCFGFreq()) {
          old2NewBB[idxOfCurrBB].second->PushBackSuccFreq(newTemp->GetFrequency());
        }
      }
      // Set label for new bb.
      SetNewOffsetOfLastMeStmtForNewBB(*currentBB, *old2NewBB[idxOfCurrBB].second, *temp, *newTemp);
    }
    idxOfCurrBB = i;
    currentBB = currPath[i];
    currentSucc = currPath[i + 1];
  }
}

void JumpThreading::CopyAndConnectPath(std::vector<BB*> &currPath) {
  auto length = currPath.size();
  size_t thePosOfSec2LastBB = length - 2; // The position of second to last bb.
  std::vector<std::pair<BB*, BB*>> old2NewBB;
  (void)old2NewBB.emplace_back(currPath[0], currPath[0]); // The first bb need not to be copied.
  DEBUG_LOG() << "old: " << currPath[0]->GetBBId() << " new: " << currPath[0]->GetBBId() << "\n";
  for (size_t i = 1; i < length; ++i) {
    PrepareForSSAUpdateWhenPredBBIsRemoved(*currPath[i - 1], *currPath[i]);
    if (i == length - 1) {
      // Need not copy the target bb.
      break;
    }
    auto *oldBB = currPath[i];
    auto *newBB = (i == thePosOfSec2LastBB) ? CopyBB(*oldBB, true) : CopyBB(*oldBB);
    old2NewBB.push_back(std::make_pair(oldBB, newBB));
    if (newBB == nullptr) {
      DEBUG_LOG() << "old: " << oldBB->GetBBId() << " new: nullptr\n";
    } else {
      DEBUG_LOG() << "old: " << oldBB->GetBBId() << " new: " << newBB->GetBBId() << "\n";
    }
  }
  (void)old2NewBB.emplace_back(currPath[length - 1], currPath[length - 1]); // The last bb need not to be copied.
  DEBUG_LOG() << "old: " << currPath[length - 1]->GetBBId() << " new: " << currPath[length - 1]->GetBBId() << "\n";
  CHECK_FATAL(old2NewBB.size() == currPath.size(), "must be equal");
  ConnectNewPath(currPath, old2NewBB, thePosOfSec2LastBB);
}

bool JumpThreading::TraverseBBsOfPath(std::vector<BB*> &currPath, int64 &currSize, bool &optSwitchBB,
                                      bool &multiBranchInPath, bool &pathThroughLatch) {
  auto thePositionOfThreadedBB = currPath.size() - 2; // The position of second to last bb.
  auto *loop = loops->GetBBLoopParent(currPath[0]->GetBBId());
  for (size_t i = 1; i < currPath.size() - 1; ++i) {
    if (currPath[i]->GetKind() == kBBSwitch) {
      if (i == thePositionOfThreadedBB) {
        optSwitchBB = true;
      } else {
        multiBranchInPath = true;
      }
    }
    if (i + 1 < currPath.size() && currPath[i]->GetSuccIndex(*currPath[i + 1]) == -1) {
      DEBUG_LOG() << "The path is not connected.\n";
      return false;
    }
    if (loop && loops->GetBBLoopParent(currPath[i]->GetBBId()) != loop) {
      DEBUG_LOG() << "The path crosses loops.\n";
      return false;
    }
    pathThroughLatch = (loop != nullptr && currPath[i] == loop->latch);
    for (auto &stmt : currPath[i]->GetMeStmts()) {
      if (i == thePositionOfThreadedBB && &stmt == currPath[i]->GetLastMe()) {
        // Need not copy the condition stmt of second to last bb.
        break;
      }
      if (!IsSupportedOpForCopyInPhasesLoopUnrollAndVRP(stmt.GetOp(), true)) {
        DEBUG_LOG() << "Is not supported op for copy.\n";
        return false;
      }
      if (stmt.GetOp() == OP_comment) {
        continue;
      }
      currSize += stmtCostAnalyzer.GetMeStmtCost(&stmt) / static_cast<int64>(kSizeScale);
      if (JumpThreading::isDebug) {
        stmt.Dump(func.GetIRMap());
      }
    }
  }
  return true;
}

// Check whether the path to be optimized is connected and whether the code size after opt is less than threshold.
bool JumpThreading::PreCopyAndConnectPath(std::vector<BB*> &currPath) {
  bool multiBranchInPath = false;
  bool optSwitchBB = false;
  int64 currSize = 0;
  bool pathThroughLatch = false;
  auto *loop = loops->GetBBLoopParent(currPath[0]->GetBBId());
  if (!TraverseBBsOfPath(currPath, currSize, optSwitchBB, multiBranchInPath, pathThroughLatch)) {
    return false;
  }
  if (multiBranchInPath && !optSwitchBB) {
    DEBUG_LOG() << "The threaded bb is not switch bb and the path has switch bb.\n";
    return false;
  }
  bool createsIrreducibleLoop = (pathThroughLatch && !dom.Dominate(*currPath.back(), *loop->latch));
  DEBUG_LOG() << "currStmtSize currPathSize: " << currSize << " " << currPath.size() << "\n";
  if (!optSwitchBB && createsIrreducibleLoop && (static_cast<uint64>(currSize) > currPath.size())) {
    DEBUG_LOG() << "In order to control the code size, each bb only can copy one stmt on average.\n";
    return false;
  }
  if (!(multiBranchInPath && optSwitchBB) && currSize > 10) {
    DEBUG_LOG() << "In order to control the code size of common thread jump, the limit of stmt size is 10.\n";
    return false;
  }
  auto codeSizeLimit = MeOption::optForSize ? 0 : kCodeSizeLimit;
  if (currSize > codeSizeLimit || static_cast<uint64>(currSize) > currPath.size() * kNumOperands ||
      codeSizeOfCopy + currSize > kModuleCodeSizeLimit) {
    DEBUG_LOG() << "Code size "<< currSize << "is greater than threshold\n";
    return false;
  }
  codeSizeOfCopy += currSize;
  DEBUG_LOG() << codeSizeOfCopy << "\n";
  return true;
}

void JumpThreading::ExecuteJumpThreading() {
  std::vector<bool> startBBs(func.GetCfg()->GetAllBBs().size());
  DEBUG_LOG() << "***************" << func.GetName() << " " << paths.size() << "***************" << "\n";
  for (size_t i = 0; i < paths.size(); ++i) {
    DEBUG_LOG() << "================" << i << "================" << "\n";
    auto *currPath = paths[i].get();
    if (startBBs[currPath->at(0)->GetBBId()]) {
      // If the start bb of current path has already jumped, do nothing.
      continue;
    }
    if (currPath->size() <= 2 || currPath->size() > kLimitOfPathLength) {
      // The current path must have more than one bb.
      // If current path only has two bbs, has been optimized in vrp phase now.
      continue;
    }
    if (i > kLimitOfPathsSize || pathSize > kLimitOfModulePathSize) {
      return;
    }
    if (!PreCopyAndConnectPath(*currPath)) {
      DEBUG_LOG() << "Can not jump thread with this path\n";
      continue;
    }
    isCFGChange = true;
    DEBUG_LOG() << "============path " << i << "begin\n";
    CopyAndConnectPath(*currPath);
    if (JumpThreading::isDebug) {
      func.GetCfg()->DumpToFile(
          "jump-threading-after" + std::to_string(func.jumpThreadingRuns) + "_" + std::to_string(i));
    }
    startBBs[currPath->at(0)->GetBBId()] = true;
    DEBUG_LOG() << "============path " << i << "end\n";
    ++pathSize;
  }
}

// Find sub path from the use of a1 in bb3 to the def of a1 in bb0, and push it to the path.
//     bb0 a1 = 1
//      |
// bb1  bb2
//  \  /
//  bb3 a2 = phi(a0, a1)
//   |
//  bb4 if a2 < 0
//  / \
// b5 b6
bool JumpThreading::FindSubPathFromUseToDefOfExpr(
    BB &defBB, BB &useBB, std::vector<BB*> &subPath, std::set<BB*> &visited) {
  auto *loop = loops->GetBBLoopParent(defBB.GetBBId());
  if (loop != nullptr) {
    if (currLoop != nullptr && loop != currLoop) {
      return false; // Only support walk through one loop.
    }
    currLoop = loop;
  }
  if (&defBB == &useBB) {
    return true;
  }
  if (visited.insert(&defBB).second) {
    for (auto &succ : defBB.GetSucc()) {
      if (FindSubPathFromUseToDefOfExpr(*succ, useBB, subPath, visited)) {
        subPath.push_back(&defBB);
        return true;
      }
    }
  }
  return false;
}

bool JumpThreading::PushSubPath2Path(BB &defBB, BB &useBB, size_t &lengthOfSubPath) {
  size_t countOfSubPath = 0;
  std::vector<BB*> subPath;
  for (auto &pred : useBB.GetPred()) {
    std::set<BB*> visited;
    subPath.push_back(&useBB);
    subPath.push_back(pred);
    if (FindSubPathFromUseToDefOfExpr(defBB, *pred, subPath, visited)) {
      ++countOfSubPath;
    }
    if (countOfSubPath > 1) {
      return false;
    }
  }
  if (countOfSubPath == 0) {
    return false;
  }
  if (!subPath.empty()) {
    subPath.pop_back();
  }
  (void)path->insert(path->end(), subPath.begin(), subPath.end());
  lengthOfSubPath = subPath.size();
  return true;
}

void JumpThreading::ResizePath(size_t lengthOfSubPath) {
  while (lengthOfSubPath != 0) {
    path->pop_back();
    lengthOfSubPath--;
  }
}

void JumpThreading::RegisterPath(BB &srcBB, BB &dstBB) {
  std::unique_ptr<std::vector<BB*>> pathTemp = std::make_unique<std::vector<BB*>>();
  pathTemp->push_back(&srcBB);
  (void)pathTemp->insert(pathTemp->end(), path->rbegin(), path->rend());

  pathTemp->push_back(&dstBB);
  paths.push_back(std::move(pathTemp));
}

// Judge whether the value range meets the conditions.
bool JumpThreading::CanJump2SuccOfCurrBB(
    BB &bb, ValueRange *vrOfOpnd0, ValueRange *vrOfOpnd1, BB &toBeJumpedBB, Opcode op) {
  if (vrOfOpnd0 == nullptr || vrOfOpnd1 == nullptr) {
    return false;
  }
  if (currBB->GetKind() != kBBCondGoto && currBB->GetKind() != kBBSwitch) {
    return false;
  }
  ASSERT_NOT_NULL(currBB->GetLastMe());
  auto pType = (currBB->GetKind() == kBBCondGoto && currBB->GetLastMe()->GetOpnd(0)->GetMeOp() == kMeOpOp) ?
      static_cast<OpMeExpr*>(currBB->GetLastMe()->GetOpnd(0))->GetOpndType() :
      currBB->GetLastMe()->GetOpnd(0)->GetPrimType();
  if (!valueRanges.BrStmtInRange(bb, *vrOfOpnd0, *vrOfOpnd1, op, pType)) {
    return false;
  }
  RegisterPath(bb, toBeJumpedBB);
  return true;
}

bool JumpThreading::CanJumpThreadingWithCondGoto(BB &bb, MeExpr *opnd, ValueRange *vrOfOpnd0) {
  auto vrpOfEqualZero = valueRanges.CreateTempValueRangeOfEqualZero(PTY_u1);
  auto *vrOfOpnd1 = (opnd == nullptr) ? &vrpOfEqualZero : valueRanges.FindValueRangeAndInitNumOfRecursion(bb, *opnd);
  ASSERT_NOT_NULL(currBB->GetLastMe());
  auto op = opnd == nullptr ? OP_ne : currBB->GetLastMe()->GetOpnd(0)->GetOp();
  if (vrOfOpnd0 != nullptr && vrOfOpnd1 != nullptr) {
    BB *trueBranch = nullptr;
    BB *falseBranch = nullptr;
    currBB->GetTrueAndFalseBranch(
        static_cast<CondGotoMeStmt*>(currBB->GetLastMe())->GetOp(), trueBranch, falseBranch);
    if (CanJump2SuccOfCurrBB(bb, vrOfOpnd0, vrOfOpnd1, *trueBranch, op)) {
      return true;
    } else if (CanJump2SuccOfCurrBB(bb, vrOfOpnd0, vrOfOpnd1, *falseBranch, GetReverseCmpOp(op))) {
      return true;
    }
  }
  return false;
}

bool JumpThreading::CanJumpThreadingWithSwitch(BB &bb, ValueRange *vrOfOpnd0) {
  if (vrOfOpnd0 == nullptr || currBB->GetLastMe()->GetOp() != OP_switch) {
    return false;
  }
  auto *switchMeStmt = static_cast<SwitchMeStmt*>(currBB->GetLastMe());
  ASSERT_NOT_NULL(switchMeStmt);
  auto *opnd = switchMeStmt->GetOpnd();
  auto *defalutBB = func.GetCfg()->GetLabelBBAt(switchMeStmt->GetDefaultLabel());
  for (auto &succ : currBB->GetSucc()) {
    if (succ == defalutBB) {
      continue;
    }
    auto *vrOfCurrCase = valueRanges.FindValueRangeAndInitNumOfRecursion(*succ, *opnd);
    if (CanJump2SuccOfCurrBB(bb, vrOfOpnd0, vrOfCurrCase, *succ, OP_eq)) {
      return true;
    }
  }
  if (defalutBB == nullptr) {
    return false;
  }
  if (vrOfOpnd0->IsConstantLowerAndUpper() && vrOfOpnd0->GetRangeType() == kEqual) {
    RegisterPath(bb, *defalutBB);
    return true;
  }
  return false;
}

// Judge whether the value range meets the conditions.
bool JumpThreading::CanJumpThreading(BB &bb, MeExpr &opnd0, MeExpr *opnd1) {
  if (currBB->GetKind() != kBBSwitch && currBB->GetKind() != kBBCondGoto) {
    return false;
  }
  auto *vrOfOpnd0 = valueRanges.FindValueRangeAndInitNumOfRecursion(bb, opnd0);
  std::unique_ptr<ValueRange> uniquePtrOfOpnd0 = nullptr;
  if (vrOfOpnd0 == nullptr && opnd0.GetMeOp() == kMeOpConst &&
      static_cast<ConstMeExpr&>(opnd0).GetConstVal()->GetKind() == kConstInt) {
    uniquePtrOfOpnd0 = std::make_unique<ValueRange>(Bound(static_cast<ConstMeExpr&>(opnd0).GetExtIntValue(),
        opnd0.GetPrimType()), kEqual);
    vrOfOpnd0 = uniquePtrOfOpnd0.get();
  }
  return (currBB->GetKind() == kBBCondGoto) ? CanJumpThreadingWithCondGoto(bb, opnd1, vrOfOpnd0) :
      CanJumpThreadingWithSwitch(bb, vrOfOpnd0);
}

void JumpThreading::FindPathWhenDefPointInCurrBB(BB &defBB, BB &predBB, MeExpr &opnd0, MeExpr *opnd1) {
  path->push_back(&defBB);
  auto cmpOpnds = std::make_pair(&opnd0, opnd1);
  FindPath(predBB, cmpOpnds);
  path->pop_back();
}

void JumpThreading::FindPathWhenDefByIsNotStmtAndPhi(BB &defBB, BB &predBB, CompareOpnds &cmpOpnds) {
  path->push_back(&predBB);
  FindPath(defBB, cmpOpnds);
  path->pop_back();
}

void JumpThreading::FindPathWhenDefPointIsNotInCurrBB(BB &defBB, BB &useBB, MeExpr &opnd0, MeExpr *opnd1) {
  size_t lengthOfSubPath = 0;
  if (!PushSubPath2Path(defBB, useBB, lengthOfSubPath)) {
    return;
  }
  auto cmpOpnds = std::make_pair(&opnd0, opnd1);
  FindPath(defBB, cmpOpnds);
  ResizePath(lengthOfSubPath);
}

void JumpThreading::FindPathWhenDefByStmt(BB &useBB, CompareOpnds &cmpOpnds, bool theFirstCmpOpndIsDefByPhi) {
  size_t lengthOfSubPath = 0;
  auto *opnd = theFirstCmpOpndIsDefByPhi ? cmpOpnds.first : cmpOpnds.second;
  if (!opnd->IsScalar() || static_cast<ScalarMeExpr*>(opnd)->GetDefBy() != kDefByStmt) {
    return;
  }
  auto *scalar = static_cast<ScalarMeExpr*>(opnd);
  auto *defBB = scalar->GetDefStmt()->GetBB();
  if (defBB == &useBB) {
    path->push_back(defBB);
    auto newCmpOpnds = theFirstCmpOpndIsDefByPhi ? std::make_pair(scalar->GetDefStmt()->GetRHS(), cmpOpnds.second) :
        std::make_pair(cmpOpnds.first, scalar->GetDefStmt()->GetRHS());
    FindPath(*defBB, newCmpOpnds);
    path->pop_back();
    return;
  }
  if (!PushSubPath2Path(*defBB, useBB, lengthOfSubPath)) {
    return;
  }
  auto newCmpOpnds = theFirstCmpOpndIsDefByPhi ? std::make_pair(scalar->GetDefStmt()->GetRHS(), cmpOpnds.second) :
      std::make_pair(cmpOpnds.first, scalar->GetDefStmt()->GetRHS());
  FindPath(*defBB, newCmpOpnds);
  ResizePath(lengthOfSubPath);
}

void JumpThreading::FindPathFromUse2DefWhenOneOpndIsDefByPhi(BB &bb, BB &pred, CompareOpnds &cmpOpnds,
    size_t i, bool theFirstCmpOpndIsDefByPhi) {
  ScalarMeExpr *scalarMeExpr = theFirstCmpOpndIsDefByPhi ? static_cast<ScalarMeExpr*>(cmpOpnds.first) :
      static_cast<ScalarMeExpr*>(cmpOpnds.second);
  auto *defPhi = &scalarMeExpr->GetDefPhi();
  auto *defBB = defPhi->GetDefBB();
  if (defBB != &bb) {
    FindPathWhenDefPointIsNotInCurrBB(*defBB, bb, *cmpOpnds.first, cmpOpnds.second);
    return;
  }
  auto *opnd0 = theFirstCmpOpndIsDefByPhi ? defPhi->GetOpnd(i) : cmpOpnds.first;
  if (theFirstCmpOpndIsDefByPhi && opnd0 == cmpOpnds.first) {
    return;
  }
  auto *opnd1 = theFirstCmpOpndIsDefByPhi ? cmpOpnds.second : defPhi->GetOpnd(i);
  if (!theFirstCmpOpndIsDefByPhi && opnd1->GetMeOp() != kMeOpConst && opnd1 == cmpOpnds.second) {
    return;
  }
  if (opnd0 != nullptr && opnd1 != nullptr && opnd0->IsScalar() && opnd1->IsScalar()) {
    MeStmt *defStmt0 = nullptr;
    MeStmt *defStmt1 = nullptr;
    auto *defBB0 = static_cast<ScalarMeExpr*>(opnd0)->GetDefByBBMeStmt(*func.GetCfg(), defStmt0);
    auto *defBB1 = static_cast<ScalarMeExpr*>(opnd1)->GetDefByBBMeStmt(*func.GetCfg(), defStmt1);
    if ((theFirstCmpOpndIsDefByPhi && !dom.Dominate(*defBB1, *defBB0)) ||
        (!theFirstCmpOpndIsDefByPhi && !dom.Dominate(*defBB0, *defBB1))) {
      // When recursively searching for the version of the value, the def points of two opnds must be dom each other.
      // For example, when searching for the definition point of a1 in bb1, the def point of B1 is bb3,
      // and bb3 does not dom bb1, so cannot continue to search upward.
      //      bb0  bb1
      //        \  /
      //    bb2  bb3  a2 = phi(a0, a1), b1 = 0
      //      \  /
      //     condgoto a5 = phi(a4, a3), b3 = phi(b2, b1)
      //       / \    if (a5 != b3)
      //     bb5 bb6
      return;
    }
  }
  FindPathWhenDefPointInCurrBB(*defBB, pred, *opnd0, opnd1);
}

void JumpThreading::FindPathWithTwoOperandsAreScalarMeExpr(BB &bb, BB &pred, CompareOpnds &cmpOpnds, size_t i) {
  auto *scalar0 = static_cast<ScalarMeExpr*>(cmpOpnds.first);
  auto *scalar1 = static_cast<ScalarMeExpr*>(cmpOpnds.second);
  MeStmt *defStmt0 = nullptr;
  MeStmt *defStmt1 = nullptr;
  auto *defBB0 = scalar0->GetDefByBBMeStmt(*func.GetCfg(), defStmt0);
  auto *defBB1 = scalar1->GetDefByBBMeStmt(*func.GetCfg(), defStmt1);
  if (scalar0->GetDefBy() == kDefByPhi && scalar1->GetDefBy() == kDefByPhi) {
    auto *defPhi0 = &scalar0->GetDefPhi();
    auto *defPhi1 = &scalar1->GetDefPhi();
    if (defBB0 == &bb && defBB1 == &bb) {
      FindPathWhenDefPointInCurrBB(bb, pred, *defPhi0->GetOpnd(i), defPhi1->GetOpnd(i));
    } else if (defBB0 == &bb) {
      FindPathWhenDefPointInCurrBB(bb, pred, *defPhi0->GetOpnd(i), cmpOpnds.second);
    } else if (defBB1 == &bb) {
      FindPathWhenDefPointInCurrBB(bb, pred, *cmpOpnds.first, defPhi1->GetOpnd(i));
    } else {
      if (dom.Dominate(*defBB0, *defBB1)) {
        FindPathWhenDefPointIsNotInCurrBB(*defBB1, bb, *cmpOpnds.first, cmpOpnds.second);
      } else if (dom.Dominate(*defBB1, *defBB0)) {
        FindPathWhenDefPointIsNotInCurrBB(*defBB0, bb, *cmpOpnds.first, cmpOpnds.second);
      }
    }
  } else if (scalar0->GetDefBy() == kDefByPhi) {
    FindPathFromUse2DefWhenOneOpndIsDefByPhi(bb, pred, cmpOpnds, i, true);
  } else if (scalar1->GetDefBy() == kDefByPhi) {
    FindPathFromUse2DefWhenOneOpndIsDefByPhi(bb, pred, cmpOpnds, i, false);
  } else if (scalar0->GetDefBy() == kDefByStmt && scalar1->GetDefBy() == kDefByStmt) {
    if (dom.Dominate(*defBB0, *defBB1)) {
      FindPathWhenDefByStmt(bb, cmpOpnds, false);
    } else if (dom.Dominate(*defBB1, *defBB0)) {
      FindPathWhenDefByStmt(bb, cmpOpnds, true);
    }
  } else if (static_cast<ScalarMeExpr*>(cmpOpnds.first)->GetDefBy() == kDefByStmt) {
    FindPathWhenDefByStmt(bb, cmpOpnds, true);
  } else if (static_cast<ScalarMeExpr*>(cmpOpnds.second)->GetDefBy() == kDefByStmt) {
    FindPathWhenDefByStmt(bb, cmpOpnds, false);
  }
}

void JumpThreading::FindPath(BB &bb, CompareOpnds &cmpOpnds) {
  auto *loop = loops->GetBBLoopParent(bb.GetBBId());
  if (loop != nullptr) {
    if (currLoop != nullptr && loop != currLoop) {
      return; // Only support walk through one loop.
    }
    currLoop = loop;
  }
  if (!visitedBBs.insert(&bb).second || bb.GetKind() == kBBIgoto ||
      visitedBBs.size() > kLimitOfVisitedBB) {
    return;
  }
  DEBUG_LOG() << bb.GetBBId() << " ";
  if ((bb.GetKind() == kBBFallthru || bb.GetKind() == kBBGoto) &&
      CanJumpThreading(bb, *cmpOpnds.first, cmpOpnds.second)) {
    return;
  }
  for (size_t i = 0; i < bb.GetPred().size(); ++i) {
    auto *pred = bb.GetPred(i);
    auto currPathsSize = paths.size();
    if (cmpOpnds.first->IsScalar() && cmpOpnds.second != nullptr && cmpOpnds.second->IsScalar()) {
      FindPathWithTwoOperandsAreScalarMeExpr(bb, *pred, cmpOpnds, i);
    } else if (cmpOpnds.first->IsScalar()) {
      if (static_cast<ScalarMeExpr*>(cmpOpnds.first)->GetDefBy() == kDefByPhi) {
        FindPathFromUse2DefWhenOneOpndIsDefByPhi(bb, *pred, cmpOpnds, i, true);
      } else if (static_cast<ScalarMeExpr*>(cmpOpnds.first)->GetDefBy() == kDefByStmt) {
        FindPathWhenDefByStmt(bb, cmpOpnds, true);
      }
    } else if (cmpOpnds.second != nullptr && cmpOpnds.second->IsScalar()) {
      if (static_cast<ScalarMeExpr*>(cmpOpnds.second)->GetDefBy() == kDefByPhi) {
        FindPathFromUse2DefWhenOneOpndIsDefByPhi(bb, *pred, cmpOpnds, i, false);
      } else if (static_cast<ScalarMeExpr*>(cmpOpnds.second)->GetDefBy() == kDefByStmt) {
        FindPathWhenDefByStmt(bb, cmpOpnds, false);
      }
    }
    if (currPathsSize + 1 == paths.size()) {
      continue; // This path has been optimized.
    }
    if (cmpOpnds.second != nullptr &&
        !valueRanges.TowCompareOperandsAreInSameIterOfLoop(*cmpOpnds.first, *cmpOpnds.second)) {
      continue;
    }
    // If currBB dom pred, can not do opt continue, because the value range of opnd in pred is come from currBB.
    // For example:
    // The loop has bb0, currBB, pred in the cfg:
    //       |
    //     currBB
    //     / ^ \
    //    v  |  v
    //  bb0  | bb1
    //     \ |
    //      pred
    if (!dom.Dominate(*currBB, *pred)) {
      FindPathWhenDefByIsNotStmtAndPhi(*pred, bb, cmpOpnds);
    }
  }
}

void JumpThreading::PreFindPath(BB &bb, MeExpr &opnd0, MeExpr *opnd1) {
  currLoop = nullptr;
  visitedBBs.clear();
  path->clear();
  currBB = &bb;
  auto compareOpnds = std::make_pair(&opnd0, opnd1);
  FindPath(bb, compareOpnds);
  currBB = nullptr;
  DEBUG_LOG() << "\n";
}

void JumpThreading::DealWithCondGoto(BB &bb) {
  ASSERT_NOT_NULL(bb.GetLastMe());
  if (!bb.GetLastMe()->IsCondBr()) {
    return;
  }
  auto *condGotoMeStmt = static_cast<CondGotoMeStmt*>(bb.GetLastMe());
  ASSERT_NOT_NULL(condGotoMeStmt);
  auto *opMeExpr = static_cast<OpMeExpr*>(condGotoMeStmt->GetOpnd());
  if (opMeExpr->GetNumOpnds() == 1) {
    PreFindPath(bb, *opMeExpr);
    return;
  }
  if (opMeExpr->GetNumOpnds() != kNumOperands) {
    return;
  }
  PreFindPath(bb, *opMeExpr->GetOpnd(0), opMeExpr->GetOpnd(1));
}

void JumpThreading::DealWithSwitch(BB &bb) {
  ASSERT_NOT_NULL(bb.GetLastMe());
  if (bb.GetLastMe()->GetOp() != OP_switch) {
    return;
  }
  auto *switchMeStmt = static_cast<SwitchMeStmt*>(bb.GetLastMe());
  PreFindPath(bb, *switchMeStmt->GetOpnd());
}

void MEJumpThreading::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEIRMapBuild>();
  aDep.AddRequired<MELoopCanon>();
  aDep.AddRequired<MEDominance>();
  aDep.AddRequired<MELoopAnalysis>();
  aDep.SetPreservedAll();
}

bool MEJumpThreading::PhaseRun(maple::MeFunction &f) {
  f.jumpThreadingRuns++;
  JumpThreading::isDebug = DEBUGFUNC_NEWPM(f);
  auto *irMap = GET_ANALYSIS(MEIRMapBuild, f);
  CHECK_FATAL(irMap != nullptr, "irMap phase has problem");
  auto *dom = EXEC_ANALYSIS(MEDominance, f)->GetDomResult();
  CHECK_FATAL(dom != nullptr, "dominance phase has problem");
  auto *meLoop = GET_ANALYSIS(MELoopAnalysis, f);
  if (JumpThreading::isDebug) {
    LogInfo::MapleLogger() << f.GetName() << "\n";
    f.Dump(false);
    f.GetCfg()->DumpToFile("jump-threading-before" + std::to_string(f.jumpThreadingRuns));
  }
  std::map<OStIdx, std::unique_ptr<std::set<BBId>>> cands((std::less<OStIdx>()));
  LoopScalarAnalysisResult sa(*irMap, nullptr);
  sa.SetComputeTripCountForLoopUnroll(false);
  auto *memPool = GetPhaseMemPool();
  StmtCostAnalyzer stmtCostAnalyzer(memPool, f.GetMirFunc());
  ValueRangePropagation valueRangePropagation(f, *irMap, *dom, meLoop, *memPool, cands, sa, false, true);
  JumpThreading jumpThreading(f, *dom, meLoop, valueRangePropagation, stmtCostAnalyzer, cands);
  valueRangePropagation.Execute();
  jumpThreading.Execute();
  if (jumpThreading.IsCFGChange()) {
    (void)f.GetCfg()->UnreachCodeAnalysis(true);
    f.GetCfg()->WontExitAnalysis();
    (void)MeSplitCEdge(false).SplitCriticalEdgeForMeFunc(f);
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &MEDominance::id);
    dom = FORCE_EXEC(MEDominance)->GetDomResult();
    MeSSAUpdate ssaUpdate(f, *f.GetMeSSATab(), *dom, cands);
    ssaUpdate.Run();
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &MELoopAnalysis::id);
    GetAnalysisInfoHook()->ForceRunTransFormPhase<MeFuncOptTy, MeFunction>(&MELoopCanon::id, f);
  }
  if (JumpThreading::isDebug) {
    LogInfo::MapleLogger() << f.GetName() << "\n";
    f.Dump(false);
    f.GetCfg()->DumpToFile("jump-threading-after" + std::to_string(f.jumpThreadingRuns));
  }
  return false;
}
}  // namespace maple
