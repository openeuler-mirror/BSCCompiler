/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_loop_unrolling.h"
#include <iostream>
#include <algorithm>
#include "me_cfg.h"
#include "me_option.h"
#include "mir_module.h"
#include "mir_builder.h"
#include "me_phase_manager.h"

namespace maple {
bool LoopUnrollingExecutor::enableDebug = false;
bool LoopUnrollingExecutor::enableDump = false;
constexpr uint32 kOneInsn = 1;
constexpr uint32 kTwoInsn = 2;
constexpr uint32 kThreeInsn = 3;
constexpr uint32 kFiveInsn = 5;
constexpr uint32 kMaxIndex = 2;
constexpr uint32 kLoopBodyNum = 1;
constexpr uint32 kOperandNum = 2;

bool ProfileCheck(const maple::MeFunction &f) {
  auto &profile = f.GetMIRModule().GetProfile();
  if (!profile.IsValid()) {
    if (LoopUnrollingExecutor::enableDebug) {
      LogInfo::MapleLogger() << "DeCompress failed in loopUnrolling" << "\n";
    }
    return false;
  }
  if (!profile.CheckFuncHot(f.GetName())) {
    if (LoopUnrollingExecutor::enableDebug) {
      LogInfo::MapleLogger() << "func is not hot" << "\n";
    }
    return false;
  }
  return true;
}

void LoopUnrolling::ComputeCodeSize(const MeStmt &meStmt, uint32 &cost) {
  switch (meStmt.GetOp()) {
    case OP_igoto:
    case OP_switch: {
      canUnroll = false;
      break;
    }
    case OP_comment: {
      break;
    }
    case OP_goto:
    case OP_dassign:
    case OP_regassign:
    case OP_membarrelease: {
      cost += kOneInsn;
      break;
    }
    case OP_brfalse:
    case OP_brtrue:
    case OP_maydassign:
    CASE_OP_ASSERT_BOUNDARY {
      cost += kTwoInsn;
      break;
    }
    case OP_iassign:
    CASE_OP_ASSERT_NONNULL
    case OP_membaracquire: {
      cost += kThreeInsn;
      break;
    }
    case OP_call:
    case OP_callassigned:
    case OP_virtualcallassigned:
    case OP_virtualicallassigned:
    case OP_interfaceicallassigned:
    case OP_intrinsiccall:
    case OP_intrinsiccallassigned:
    case OP_intrinsiccallwithtype:
    case OP_membarstorestore:
    case OP_membarstoreload: {
      cost += kFiveInsn;
      break;
    }
    case OP_asm: {
      // currently skip unrolling loop with asm stmt
      cost += kMaxCost;
      break;
    }
    default:
      if (LoopUnrollingExecutor::enableDebug) {
        LogInfo::MapleLogger() << "consider this op :"<< meStmt.GetOp() << "\n";
      }
      canUnroll = false;
      break;
  }
}

BB *LoopUnrolling::CopyBB(BB &bb, bool isInLoop) {
  BB *newBB = cfg->NewBasicBlock();
  if (isInLoop) {
    newBB->SetAttributes(kBBAttrIsInLoop);
  }
  newBB->SetKind(bb.GetKind());
  func->CloneBBMeStmts(bb, *newBB, &cands);
  return newBB;
}

void LoopUnrolling::SetLabelWithCondGotoOrGotoBB(BB &bb, std::unordered_map<BB*, BB*> &old2NewBB, const BB &exitBB,
                                                 LabelIdx oldlabIdx) {
  CHECK_FATAL(!bb.GetMeStmts().empty(), "must not be empty");
  for (auto succ : bb.GetSucc()) {
    // process in replace preds of exit bb
    if (succ == &exitBB || old2NewBB.find(succ) == old2NewBB.end()) {
      continue;
    }
    bool inLastNew2OldBB = false;
    if (old2NewBB.find(succ) == old2NewBB.end()) {
      for (auto it : lastNew2OldBB) {
        if (it.second == succ) {
          inLastNew2OldBB = true;
          break;
        }
      }
      if (inLastNew2OldBB) {
        continue;
      } else {
        CHECK_FATAL(false, "impossible");
      }
    }
    if (oldlabIdx == succ->GetBBLabel()) {
      LabelIdx label = old2NewBB[succ]->GetBBLabel();
      CHECK_FATAL(label != 0, "label must not be zero");
      if (bb.GetKind() == kBBCondGoto) {
        static_cast<CondGotoMeStmt&>(old2NewBB[&bb]->GetMeStmts().back()).SetOffset(label);
      } else {
        CHECK_FATAL(bb.GetKind() == kBBGoto, "must be kBBGoto");
        static_cast<GotoMeStmt&>(old2NewBB[&bb]->GetMeStmts().back()).SetOffset(label);
      }
    }
  }
}

// When loop unroll times is two, use this function to update the preds and succs of the duplicate loopbody.
void LoopUnrolling::ResetOldLabel2NewLabel(std::unordered_map<BB*, BB*> &old2NewBB, BB &bb,
                                           const BB &exitBB, BB &newHeadBB) {
  if (bb.GetKind() == kBBCondGoto) {
    CHECK_FATAL(!bb.GetMeStmts().empty(), "must not be empty");
    CondGotoMeStmt &condGotoNode = static_cast<CondGotoMeStmt&>(bb.GetMeStmts().back());
    LabelIdx oldlabIdx = condGotoNode.GetOffset();
    CHECK_FATAL(oldlabIdx == exitBB.GetBBLabel(), "must equal");
    LabelIdx label = func->GetOrCreateBBLabel(newHeadBB);
    condGotoNode.SetOffset(label);
    static_cast<CondGotoMeStmt&>(newHeadBB.GetMeStmts().back()).SetOffset(oldlabIdx);
  } else if (bb.GetKind() == kBBGoto) {
    CHECK_FATAL(!bb.GetMeStmts().empty(), "must not be empty");
    GotoMeStmt &gotoStmt = static_cast<GotoMeStmt&>(bb.GetMeStmts().back());
    LabelIdx oldlabIdx = gotoStmt.GetOffset();
    CHECK_FATAL(oldlabIdx == exitBB.GetBBLabel(), "must equal");
    LabelIdx label = func->GetOrCreateBBLabel(newHeadBB);
    gotoStmt.SetOffset(label);
    static_cast<GotoMeStmt&>(old2NewBB[&bb]->GetMeStmts().back()).SetOffset(oldlabIdx);
  }
}

// When loop unroll times more than two, use this function to update the preds and succs of duplicate loopbodys.
void LoopUnrolling::ResetOldLabel2NewLabel2(std::unordered_map<BB*, BB*> &old2NewBB, BB &bb,
                                            const BB &exitBB, BB &newHeadBB) {
  if (bb.GetKind() == kBBCondGoto) {
    CHECK_FATAL(!bb.GetMeStmts().empty(), "must not be empty");
    CondGotoMeStmt &condGotoNode = static_cast<CondGotoMeStmt&>(bb.GetMeStmts().back());
    LabelIdx oldlabIdx = condGotoNode.GetOffset();
    CHECK_FATAL(oldlabIdx == exitBB.GetBBLabel(), "must equal");
    LabelIdx label = func->GetOrCreateBBLabel(newHeadBB);
    condGotoNode.SetOffset(label);
    static_cast<CondGotoMeStmt&>(newHeadBB.GetMeStmts().back()).SetOffset(oldlabIdx);
  } else if (bb.GetKind() == kBBGoto) {
    CHECK_FATAL(!bb.GetMeStmts().empty(), "must not be empty");
    GotoMeStmt &gotoStmt = static_cast<GotoMeStmt&>(bb.GetMeStmts().back());
    LabelIdx oldlabIdx = gotoStmt.GetOffset();
    LabelIdx label = func->GetOrCreateBBLabel(newHeadBB);
    gotoStmt.SetOffset(label);
    static_cast<GotoMeStmt&>(old2NewBB[lastNew2OldBB[&bb]]->GetMeStmts().back()).SetOffset(oldlabIdx);
  }
}

void LoopUnrolling::ResetFrequency(const BB &curBB, const BB &succ, const BB &exitBB,
                                   BB &curCopyBB, bool copyAllLoop) {
  if (resetFreqForUnrollWithVar && copyAllLoop) {
    if (&curBB == &exitBB && &succ == *loop->inloopBB2exitBBs.begin()->second->begin()) {
      curCopyBB.PushBackSuccFreq(loop->head->GetFrequency() % replicatedLoopNum == 0 ? 0 : 1);
    }
    if ((&curBB == loop->latch && &succ == loop->head) || (&curBB == &exitBB && &succ == loop->latch)) {
      curCopyBB.PushBackSuccFreq(loop->head->GetFrequency() % replicatedLoopNum == 0 ? 0 :
          loop->head->GetFrequency() % replicatedLoopNum - 1);
    } else {
      curCopyBB.PushBackSuccFreq(curBB.GetEdgeFreq(&succ) % replicatedLoopNum);
    }
  } else {
    profValid && resetFreqForAfterInsertGoto ?
        curCopyBB.PushBackSuccFreq(curBB.GetEdgeFreq(&succ) - 1 == 0 ? curBB.GetEdgeFreq(&succ) :
            curBB.GetEdgeFreq(&succ) - 1) :
        curCopyBB.PushBackSuccFreq(curBB.GetEdgeFreq(&succ));
  }
}

void LoopUnrolling::CreateLableAndInsertLabelBB(BB &newHeadBB, std::set<BB*> &labelBBs) {
  if (loop->head->GetBBLabel() != 0) {
    (void)func->GetOrCreateBBLabel(newHeadBB);
  }
  if (newHeadBB.GetKind() == kBBCondGoto || newHeadBB.GetKind() == kBBGoto) {
    labelBBs.insert(loop->head);
  }
}

void LoopUnrolling::CopyLoopBodyForProfile(BB &newHeadBB, std::unordered_map<BB*, BB*> &old2NewBB,
                                           std::set<BB*> &labelBBs, const BB &exitBB, bool copyAllLoop) {
  CreateLableAndInsertLabelBB(newHeadBB, labelBBs);
  std::queue<BB*> bbQue;
  bbQue.push(loop->head);
  while (!bbQue.empty()) {
    BB *curBB = bbQue.front();
    bbQue.pop();
    BB *curCopyBB = old2NewBB[curBB];
    for (BB *succ : curBB->GetSucc()) {
      if (loop->loopBBs.find(succ->GetBBId()) == loop->loopBBs.end() ||
          (!copyAllLoop && (succ == &exitBB || succ == loop->latch))) {
        continue;
      }
      if (old2NewBB.find(succ) != old2NewBB.end()) {
        ResetFrequency(*curBB, *succ, exitBB, *curCopyBB, copyAllLoop);
        curCopyBB->AddSucc(*old2NewBB[succ]);
      } else {
        if (needUpdateInitLoopFreq) {
          ResetFrequency(*succ);
        }
        BB *newBB = CopyBB(*succ, true);
        if (resetFreqForUnrollWithVar) {
          if (succ == loop->latch) {
            newBB->SetFrequency(loop->head->GetFrequency() % replicatedLoopNum - 1);
          } else {
            newBB->SetFrequency(succ->GetFrequency() % replicatedLoopNum);
          }
        } else {
          resetFreqForAfterInsertGoto ?
              newBB->SetFrequency(succ->GetFrequency() - 1 == 0 ? succ->GetFrequency() :
                                                                  succ->GetFrequency() - 1) :
              newBB->SetFrequency(succ->GetFrequency());
        }
        curCopyBB->AddSucc(*newBB);
        ResetFrequency(*curBB, *succ, exitBB, *curCopyBB, copyAllLoop);
        (void)old2NewBB.emplace(succ, newBB);
        bbQue.push(succ);
        if (succ->GetBBLabel() != 0) {
          (void)func->GetOrCreateBBLabel(*newBB);
        }
        if (succ->GetKind() == kBBCondGoto || succ->GetKind() == kBBGoto) {
          labelBBs.insert(succ);
        }
      }
    }
  }
}

void LoopUnrolling::CopyLoopBody(BB &newHeadBB, std::unordered_map<BB*, BB*> &old2NewBB,
                                 std::set<BB*> &labelBBs, const BB &exitBB, bool copyAllLoop) {
  CreateLableAndInsertLabelBB(newHeadBB, labelBBs);
  std::queue<BB*> bbQue;
  bbQue.push(loop->head);
  while (!bbQue.empty()) {
    BB *curBB = bbQue.front();
    bbQue.pop();
    BB *curCopyBB = old2NewBB[curBB];
    for (BB *succ : curBB->GetSucc()) {
      if (loop->loopBBs.find(succ->GetBBId()) == loop->loopBBs.end() ||
          (!copyAllLoop && (succ == &exitBB || succ == loop->latch))) {
        continue;
      }
      if (old2NewBB.find(succ) != old2NewBB.end()) {
        curCopyBB->AddSucc(*old2NewBB[succ]);
      } else {
        BB *newBB = CopyBB(*succ, true);
        curCopyBB->AddSucc(*newBB);
        CHECK_NULL_FATAL(newBB);
        (void)old2NewBB.emplace(succ, newBB);
        bbQue.push(succ);
        if (succ->GetBBLabel() != 0) {
          (void)func->GetOrCreateBBLabel(*newBB);
        }
        if (succ->GetKind() == kBBCondGoto || succ->GetKind() == kBBGoto) {
          labelBBs.insert(succ);
        }
      }
    }
  }
}

// Update frequency of old BB.
void LoopUnrolling::ResetFrequency(BB &bb) {
  auto freq = bb.GetFrequency() / replicatedLoopNum;
  if (freq == 0 && partialCount == 0 && bb.GetFrequency() != 0) {
    freq = 1;
  }
  bb.SetFrequency(static_cast<uint32>(freq + partialCount));
  for (size_t i = 0; i < bb.GetSucc().size(); ++i) {
    auto currFreq = bb.GetEdgeFreq(i) / replicatedLoopNum;
    if (currFreq == 0 && partialCount == 0 && bb.GetFrequency() != 0) {
      currFreq = 1;
    }
    bb.SetEdgeFreq(bb.GetSucc(i), currFreq + partialCount);
  }
}

// Update frequency of old exiting BB and latch BB.
void LoopUnrolling::ResetFrequency() {
  auto exitBB = cfg->GetBBFromID(loop->inloopBB2exitBBs.begin()->first);
  auto latchBB = loop->latch;
  if (isUnrollWithVar) {
    auto latchFreq = loop->head->GetFrequency() % replicatedLoopNum - loop->preheader->GetFrequency();
    exitBB->SetFrequency(static_cast<uint32>(loop->head->GetFrequency() % replicatedLoopNum - latchFreq));
    exitBB->SetEdgeFreq(latchBB, latchFreq);
    latchBB->SetFrequency(static_cast<uint32>(latchFreq));
    latchBB->SetEdgeFreq(loop->head, latchFreq);
  } else {
    auto exitFreq = exitBB->GetFrequency() / replicatedLoopNum;
    if (exitFreq == 0 && exitBB->GetFrequency() != 0) {
      exitFreq = 1;
    }
    exitBB->SetFrequency(static_cast<uint32>(exitFreq));
    auto exitEdgeFreq = exitBB->GetEdgeFreq(latchBB) / replicatedLoopNum;
    if (exitEdgeFreq == 0 && exitBB->GetEdgeFreq(latchBB) != 0) {
      exitEdgeFreq = 1;
    }
    exitBB->SetEdgeFreq(latchBB, exitEdgeFreq);
    auto latchFreq = latchBB->GetFrequency() / replicatedLoopNum;
    if (latchFreq == 0 && latchBB->GetFrequency() != 0) {
      latchFreq = 1;
    }
    latchBB->SetFrequency(static_cast<uint32>(latchFreq));
    latchBB->SetEdgeFreq(loop->head, latchFreq);
  }
}

// When copying the loop for the first time, insert the loop header behind the tail of the original loop.
void LoopUnrolling::AddEdgeForExitBBLastNew2OldBBEmpty(BB &exitBB, std::unordered_map<BB*, BB*> &old2NewBB,
                                                       BB &newHeadBB) {
  if (!exitBB.GetPred().empty()) {
    for (auto &it : exitBB.GetMePhiList()) {
      MeSSAUpdate::InsertOstToSSACands(it.first, *exitBB.GetPred(0), &cands);
    }
  }
  for (size_t idx = 0; idx < exitBB.GetPred().size(); ++idx) {
    auto *bb = exitBB.GetPred(idx);
    auto it = old2NewBB.find(bb);
    CHECK_FATAL(it != old2NewBB.end(), "Can not find bb in old2NewBB");
    bb->ReplaceSucc(&exitBB, &newHeadBB);
    exitBB.AddPred(*old2NewBB[bb], idx);
    if (profValid) {
      resetFreqForAfterInsertGoto ?
          (bb->GetEdgeFreq(idx) - 1 == 0 ? old2NewBB[bb]->PushBackSuccFreq(bb->GetEdgeFreq(idx)) :
              old2NewBB[bb]->PushBackSuccFreq(bb->GetEdgeFreq(idx) - 1)) :
          old2NewBB[bb]->PushBackSuccFreq(bb->GetEdgeFreq(idx));
    }
    ResetOldLabel2NewLabel(old2NewBB, *bb, exitBB, newHeadBB);
  }
}

void LoopUnrolling::AddEdgeForExitBB(BB &exitBB, std::unordered_map<BB*, BB*> &old2NewBB, BB &newHeadBB) {
  for (size_t idx = 0; idx < exitBB.GetPred().size(); ++idx) {
    auto *bb = exitBB.GetPred(idx);
    auto it = old2NewBB.find(lastNew2OldBB[bb]);
    CHECK_FATAL(it != old2NewBB.end(), "Can not find bb in lastNew2OldBB");
    bb->ReplaceSucc(&exitBB, &newHeadBB);
    exitBB.AddPred(*old2NewBB[lastNew2OldBB[bb]], idx);
    if (profValid) {
      (resetFreqForAfterInsertGoto && firstResetForAfterInsertGoto) ?
          (bb->GetEdgeFreq(idx) - 1 == 0 ? old2NewBB[lastNew2OldBB[bb]]->PushBackSuccFreq(bb->GetEdgeFreq(idx)) :
              old2NewBB[lastNew2OldBB[bb]]->PushBackSuccFreq(bb->GetEdgeFreq(idx) - 1)) :
              old2NewBB[lastNew2OldBB[bb]]->PushBackSuccFreq(bb->GetEdgeFreq(idx));
      firstResetForAfterInsertGoto = false;
    }
    ResetOldLabel2NewLabel2(old2NewBB, *bb, exitBB, newHeadBB);
  }
}

// Copy loop except exiting BB and latch BB.
void LoopUnrolling::CopyAndInsertBB(bool isPartial) {
  auto exitBB = cfg->GetBBFromID(loop->inloopBB2exitBBs.begin()->first);
  CHECK_FATAL(exitBB->GetKind() == kBBCondGoto, "exiting bb must be kBBCondGoto");
  std::unordered_map<BB*, BB*> old2NewBB;
  BB *newHeadBB = CopyBB(*loop->head, true);
  if (profValid && needUpdateInitLoopFreq) {
    ResetFrequency(*loop->head);
    ResetFrequency();
  }
  profValid && resetFreqForAfterInsertGoto ?
      (loop->head->GetFrequency() - 1 == 0 ? newHeadBB->SetFrequency(loop->head->GetFrequency()) :
          newHeadBB->SetFrequency(loop->head->GetFrequency() - 1)) :
          newHeadBB->SetFrequency(loop->head->GetFrequency());
  (void)old2NewBB.emplace(loop->head, newHeadBB);
  std::set<BB*> labelBBs;
  profValid ? CopyLoopBodyForProfile(*newHeadBB, old2NewBB, labelBBs, *exitBB, false) :
              CopyLoopBody(*newHeadBB, old2NewBB, labelBBs, *exitBB, false);
  if (isPartial) {
    partialSuccHead = newHeadBB;
  }
  for (auto bb : labelBBs) {
    if (bb->GetKind() == kBBCondGoto) {
      CHECK_FATAL(!bb->GetMeStmts().empty(), "must not be empty");
      LabelIdx oldlabIdx = static_cast<CondGotoMeStmt&>(bb->GetMeStmts().back()).GetOffset();
      SetLabelWithCondGotoOrGotoBB(*bb, old2NewBB, *exitBB, oldlabIdx);
    } else if (bb->GetKind() == kBBGoto) {
      CHECK_FATAL(!bb->GetMeStmts().empty(), "must not be empty");
      LabelIdx oldlabIdx = static_cast<GotoMeStmt&>(bb->GetMeStmts().back()).GetOffset();
      SetLabelWithCondGotoOrGotoBB(*bb, old2NewBB, *exitBB, oldlabIdx);
    } else {
      CHECK_FATAL(false, "must be kBBCondGoto or kBBGoto");
    }
  }
  if (lastNew2OldBB.empty()) {
    AddEdgeForExitBBLastNew2OldBBEmpty(*exitBB, old2NewBB, *newHeadBB);
  } else {
    AddEdgeForExitBB(*exitBB, old2NewBB, *newHeadBB);
  }
  lastNew2OldBB.clear();
  for (auto it : old2NewBB) {
    lastNew2OldBB[it.second] = it.first;
  }
}

void LoopUnrolling::RemoveCondGoto() {
  BB *exitingBB = cfg->GetBBFromID(loop->inloopBB2exitBBs.begin()->first);
  CHECK_FATAL(exitingBB->GetSucc().size() == 2, "must has two succ bb");
  if (exitingBB->GetSucc(0) != loop->latch && exitingBB->GetSucc(1) != loop->latch) {
    CHECK_FATAL(false, "latch must be pred of exiting bb");
  }
  exitingBB->RemoveSucc(*loop->latch);
  exitingBB->RemoveMeStmt(exitingBB->GetLastMe());
  exitingBB->SetKind(kBBFallthru);
  if (profValid) {
    exitingBB->SetEdgeFreq(exitingBB->GetSucc(0), exitingBB->GetFrequency());
  }
  loop->head->RemovePred(*loop->latch);
  cfg->DeleteBasicBlock(*loop->latch);
}

bool LoopUnrolling::SplitCondGotoBB() {
  auto *exitBB = func->GetCfg()->GetBBFromID(loop->inloopBB2exitBBs.begin()->first);
  auto *exitedBB = *(loop->inloopBB2exitBBs.begin()->second->begin());
  MeStmt *lastStmt = exitBB->GetLastMe();
  if (lastStmt->GetOp() == OP_igoto || lastStmt->GetOp() == OP_switch) {
    return false;
  }
  if (lastStmt->GetOp() != OP_brfalse && lastStmt->GetOp() != OP_brtrue) {
    return false;
  }
  bool notOnlyHasBrStmt = true;
  for (auto &stmt : exitBB->GetMeStmts()) {
    if (&stmt != exitBB->GetLastMe()) {
      notOnlyHasBrStmt = false;
      break;
    }
  }
  if (notOnlyHasBrStmt) {
    return true;
  }
  BB *newCondGotoBB = func->GetCfg()->NewBasicBlock();
  newCondGotoBB->SetKind(kBBCondGoto);
  newCondGotoBB->SetAttributes(kBBAttrIsInLoop);
  exitBB->SetKind(kBBFallthru);
  loop->loopBBs.insert(newCondGotoBB->GetBBId());
  loop->InsertInloopBB2exitBBs(*newCondGotoBB, *exitedBB);
  loop->inloopBB2exitBBs.erase(exitBB->GetBBId());

  for (auto *bb : exitBB->GetSucc()) {
    bb->ReplacePred(exitBB, newCondGotoBB);
  }

  exitBB->AddSucc(*newCondGotoBB);
  exitBB->RemoveMeStmt(lastStmt);
  newCondGotoBB->InsertMeStmtLastBr(lastStmt);
  return true;
}

LoopUnrolling::ReturnKindOfFullyUnroll LoopUnrolling::LoopFullyUnroll(int64 tripCount) {
  if (tripCount > kMaxCost) {
    // quickly return if iteration is large enough
    return kCanNotFullyUnroll;
  }
  if (MeOption::optForSize) {
    return kCanNotFullyUnroll;
  }
  uint32 costResult = 0;
  for (auto bbId : loop->loopBBs) {
    BB *bb = cfg->GetBBFromID(bbId);
    for (auto &meStmt : bb->GetMeStmts()) {
      uint32 cost = 0;
      ComputeCodeSize(meStmt, cost);
      if (!canUnroll) {
        return kCanNotFullyUnroll;
      }
      costResult += cost;
      if (costResult * tripCount > kMaxCost) {
        return kCanNotFullyUnroll;
      }
    }
  }
  if (!SplitCondGotoBB()) {
    return kCanNotSplitCondGoto;
  }
  replicatedLoopNum = static_cast<uint64>(tripCount);
  for (int64 i = 0; i < tripCount; ++i) {
    if (i > 0) {
      needUpdateInitLoopFreq = false;
    }
    CopyAndInsertBB(false);
  }
  RemoveCondGoto();
  return kCanFullyUnroll;
}

void LoopUnrolling::ResetFrequency(BB &newCondGotoBB, BB &exitingBB, const BB &exitedBB, uint32 headFreq) {
  if (profValid) {
    newCondGotoBB.SetFrequency(headFreq);
    exitingBB.SetEdgeFreq(&exitedBB, 0);
  }
}

void LoopUnrolling::InsertCondGotoBB() {
  CHECK_NULL_FATAL(partialSuccHead);
  BB *exitingBB = cfg->GetBBFromID(loop->inloopBB2exitBBs.begin()->first);
  BB *exitedBB = *(loop->inloopBB2exitBBs.begin()->second->begin());
  BB *newCondGotoBB = CopyBB(*exitingBB, true);
  auto headFreq = loop->head->GetFrequency();
  ResetFrequency(*newCondGotoBB, *exitingBB, *exitedBB, headFreq);
  MeStmt *lastMeStmt = newCondGotoBB->GetLastMe();
  CHECK_FATAL(lastMeStmt != nullptr, "last meStmt must not be nullptr");
  CHECK_FATAL(partialSuccHead->GetBBLabel() != 0, "label must not be zero");
  static_cast<CondGotoMeStmt*>(lastMeStmt)->SetOffset(partialSuccHead->GetBBLabel());
  bool addExitedSucc = false;
  if (exitingBB->GetSucc().front() == exitedBB) {
    addExitedSucc = true;
    newCondGotoBB->AddSucc(*exitedBB);
    if (profValid) {
      newCondGotoBB->PushBackSuccFreq(1);
    }
  }
  for (size_t idx = 0; idx < partialSuccHead->GetPred().size(); ++idx) {
    auto *pred = partialSuccHead->GetPred(idx);
    pred->ReplaceSucc(partialSuccHead, newCondGotoBB);
    partialSuccHead->AddPred(*newCondGotoBB, idx);
    if (profValid) {
      newCondGotoBB->PushBackSuccFreq(headFreq - 1 == 0 ? headFreq : headFreq - 1);
    }
    if (pred->GetKind() == kBBCondGoto) {
      CHECK_FATAL(!pred->GetMeStmts().empty(), "must not be empty");
      CondGotoMeStmt &condGotoNode = static_cast<CondGotoMeStmt&>(pred->GetMeStmts().back());
      LabelIdx oldlabIdx = condGotoNode.GetOffset();
      CHECK_FATAL(oldlabIdx == partialSuccHead->GetBBLabel(), "must equal");
      LabelIdx label = func->GetOrCreateBBLabel(*newCondGotoBB);
      condGotoNode.SetOffset(label);
    } else if (pred->GetKind() == kBBGoto) {
      CHECK_FATAL(!pred->GetMeStmts().empty(), "must not be empty");
      GotoMeStmt &gotoStmt = static_cast<GotoMeStmt&>(pred->GetMeStmts().back());
      LabelIdx oldlabIdx = gotoStmt.GetOffset();
      CHECK_FATAL(oldlabIdx == partialSuccHead->GetBBLabel(), "must equal");
      LabelIdx label = func->GetOrCreateBBLabel(*newCondGotoBB);
      gotoStmt.SetOffset(label);
    }
  }
  if (!addExitedSucc) {
    newCondGotoBB->AddSucc(*exitedBB);
    if (profValid) {
      newCondGotoBB->PushBackSuccFreq(headFreq - 1 == 0 ? headFreq : headFreq - 1);
    }
  }
}

bool LoopUnrolling::DetermineUnrollTimes(uint32 &index, bool isConst) {
  uint32 costResult = 0;
  uint32 unrollTime = unrollTimes[index];
  for (auto bbId : loop->loopBBs) {
    BB *bb = cfg->GetBBFromID(bbId);
    auto exitBB = cfg->GetBBFromID(loop->inloopBB2exitBBs.begin()->first);
    if (bb == exitBB) {
      continue;
    }
    for (auto &meStmt : bb->GetMeStmts()) {
      uint32 cost = 0;
      ComputeCodeSize(meStmt, cost);
      if (!canUnroll) {
        return false;
      }
      costResult += cost;
      if ((isConst && costResult * (unrollTime - 1) > kMaxCost) || (!isConst && costResult * unrollTime > kMaxCost)) {
        if (index < kMaxIndex) {
          ++index;
          return DetermineUnrollTimes(index, isConst);
        } else {
          return false;
        }
      }
    }
  }
  if (LoopUnrollingExecutor::enableDebug) {
    LogInfo::MapleLogger() << "CodeSize: " << costResult << "\n";
  }
  return true;
}

void LoopUnrolling::AddPreHeader(BB *oldPreHeader, BB *head) {
  CHECK_FATAL(oldPreHeader->GetKind() == kBBCondGoto, "must be kBBCondGoto");
  auto *preheader = cfg->NewBasicBlock();
  preheader->SetAttributes(kBBAttrArtificial);
  preheader->SetKind(kBBFallthru);
  uint64 preheaderFreq = 0;
  if (profValid) {
    preheaderFreq = oldPreHeader->GetEdgeFreq(head);
  }
  size_t index = head->GetPred().size();
  while (index > 0) {
    if (head->GetPred(index - 1) == oldPreHeader) {
      break;
    }
    --index;
  }
  oldPreHeader->ReplaceSucc(head, preheader);
  --index;
  head->AddPred(*preheader, index);
  if (profValid) {
    preheader->PushBackSuccFreq(preheaderFreq);
    preheader->SetFrequency(static_cast<uint32>(preheaderFreq));
  }
  CondGotoMeStmt *condGotoStmt = static_cast<CondGotoMeStmt*>(oldPreHeader->GetLastMe());
  LabelIdx oldlabIdx = condGotoStmt->GetOffset();
  if (oldlabIdx == head->GetBBLabel()) {
    LabelIdx label = func->GetOrCreateBBLabel(*preheader);
    condGotoStmt->SetOffset(label);
  }
}

bool LoopUnrolling::LoopPartialUnrollWithConst(uint64 tripCount) {
  uint32 index = 0;
  if (!DetermineUnrollTimes(index, true)) {
    if (LoopUnrollingExecutor::enableDebug) {
      LogInfo::MapleLogger() << "CodeSize is too large" << "\n";
    }
    return false;
  }
  uint32 unrollTime = unrollTimes[index];
  if (tripCount / unrollTime < 1) {
    return false;
  }
  uint32 remainder = (static_cast<uint32>(tripCount) + kLoopBodyNum) % unrollTime;
  if (!SplitCondGotoBB()) {
    return false;
  }
  if (LoopUnrollingExecutor::enableDebug) {
    LogInfo::MapleLogger() << "Unrolltime: " << unrollTime << "\n";
    LogInfo::MapleLogger() << "Remainder: " << remainder << "\n";
  }
  if (remainder == 0) {
    for (int64 i = 1; i < unrollTime; ++i) {
      if (i > 1) {
        needUpdateInitLoopFreq = false;
      }
      replicatedLoopNum = unrollTime;
      CopyAndInsertBB(false);
    }
  } else {
    partialCount = 1;
    for (int64 i = 1; i < unrollTime; ++i) {
      if (i > 1) {
        needUpdateInitLoopFreq = false;
      }
      resetFreqForAfterInsertGoto = i < remainder ? false : true;
      replicatedLoopNum = unrollTime;
      if (i == remainder) {
        CopyAndInsertBB(true);
      } else {
        CopyAndInsertBB(false);
      }
    }
    replicatedLoopNum = unrollTime;
    InsertCondGotoBB();
  }
  return true;
}

void LoopUnrolling::CopyLoopForPartialAndPre(BB *&newHead, BB *&newExiting) {
  needUpdateInitLoopFreq = false;
  auto exitBB = cfg->GetBBFromID(loop->inloopBB2exitBBs.begin()->first);
  CHECK_FATAL(exitBB->GetKind() == kBBCondGoto, "exiting bb must be kBBCondGoto");
  std::unordered_map<BB*, BB*> old2NewBB;
  BB *newHeadBB = CopyBB(*loop->head, true);
  if (profValid) {
    newHeadBB->SetFrequency(loop->head->GetFrequency() % replicatedLoopNum);
  }
  (void)old2NewBB.emplace(loop->head, newHeadBB);
  std::set<BB*> labelBBs;
  resetFreqForUnrollWithVar = true;
  profValid ? CopyLoopBodyForProfile(*newHeadBB, old2NewBB, labelBBs, *exitBB, true) :
              CopyLoopBody(*newHeadBB, old2NewBB, labelBBs, *exitBB, true);
  resetFreqForUnrollWithVar = false;
  for (auto bb : labelBBs) {
    if (bb->GetKind() == kBBCondGoto) {
      CHECK_FATAL(!bb->GetMeStmts().empty(), "must not be empty");
      LabelIdx oldlabIdx = static_cast<CondGotoMeStmt&>(bb->GetMeStmts().back()).GetOffset();
      SetLabelWithCondGotoOrGotoBB(*bb, old2NewBB, *exitBB, oldlabIdx);
    } else if (bb->GetKind() == kBBGoto) {
      CHECK_FATAL(!bb->GetMeStmts().empty(), "must not be empty");
      LabelIdx oldlabIdx = static_cast<GotoMeStmt&>(bb->GetMeStmts().back()).GetOffset();
      SetLabelWithCondGotoOrGotoBB(*bb, old2NewBB, *exitBB, oldlabIdx);
    } else {
      CHECK_FATAL(false, "must be kBBCondGoto or kBBGoto");
    }
  }
  newHead = newHeadBB;
  newExiting = old2NewBB[exitBB];
}

VarMeExpr *LoopUnrolling::CreateIndVarOrTripCountWithName(const std::string &name) {
  MIRSymbol *symbol =
      func->GetMIRModule().GetMIRBuilder()->CreateLocalDecl(name, *GlobalTables::GetTypeTable().GetInt32());
  OriginalSt *ost = irMap->GetSSATab().CreateSymbolOriginalSt(*symbol, func->GetMirFunc()->GetPuidx(), 0);
  ost->SetZeroVersionIndex(irMap->GetVerst2MeExprTable().size());
  irMap->GetVerst2MeExprTable().push_back(nullptr);
  ost->PushbackVersionsIndices(ost->GetZeroVersionIndex());
  VarMeExpr *var = irMap->CreateVarMeExprVersion(ost);
  return var;
}

// i < tripcount / unrollTime
void LoopUnrolling::UpdateCondGotoStmt(BB &bb, VarMeExpr &indVar, MeExpr &tripCount,
                                       MeExpr &unrollTimeExpr, uint32 offset) {
  for (auto &stmt : bb.GetMeStmts()) {
    bb.RemoveMeStmt(&stmt);
  }
  MeExpr *divMeExpr = irMap->CreateMeExprBinary(OP_div, PTY_i32, tripCount, unrollTimeExpr);
  MeExpr *opMeExpr = irMap->CreateMeExprCompare(OP_ge, PTY_u1, PTY_i32, static_cast<MeExpr&>(indVar), *divMeExpr);
  UnaryMeStmt *unaryMeStmt = irMap->New<UnaryMeStmt>(OP_brfalse);
  unaryMeStmt->SetOpnd(0, opMeExpr);
  CondGotoMeStmt *newCondGotoStmt = irMap->New<CondGotoMeStmt>(*unaryMeStmt, offset);
  bb.AddMeStmtLast(newCondGotoStmt);
}

// i++
// i < tripcount / unrollTime
void LoopUnrolling::UpdateCondGotoBB(BB &bb, VarMeExpr &indVar, MeExpr &tripCount, MeExpr &unrollTimeExpr) {
  uint32 offset = static_cast<CondGotoMeStmt*>(bb.GetLastMe())->GetOffset();
  for (auto &stmt : bb.GetMeStmts()) {
    bb.RemoveMeStmt(&stmt);
  }
  VarMeExpr *newVarLHS = irMap->CreateVarMeExprVersion(indVar.GetOst());
  MeSSAUpdate::InsertOstToSSACands(newVarLHS->GetOstIdx(), bb, &cands);
  MeExpr *constMeExprForOne = irMap->CreateIntConstMeExpr(1, PTY_i32);
  MeExpr *addMeExpr = irMap->CreateMeExprBinary(OP_add, PTY_i32, indVar, *constMeExprForOne);
  UpdateCondGotoStmt(bb, indVar, tripCount, unrollTimeExpr, offset);
  bb.AddMeStmtFirst(irMap->CreateAssignMeStmt(*newVarLHS, *addMeExpr, bb));
}

void LoopUnrolling::ExchangeSucc(BB &partialExit) {
  BB *succ0 = partialExit.GetSucc(0);
  partialExit.SetSucc(0, partialExit.GetSucc(1));
  partialExit.SetSucc(1, succ0);
}

// copy loop for partial
void LoopUnrolling::CopyLoopForPartial(BB &partialCondGoto, BB &exitedBB, BB &exitingBB) {
  BB *partialHead = nullptr;
  BB *partialExit = nullptr;
  CopyLoopForPartialAndPre(partialHead, partialExit);
  (void)func->GetOrCreateBBLabel(partialCondGoto);
  CHECK_FATAL(partialHead->GetBBLabel() != 0, "must not be zero");
  CHECK_FATAL(!partialCondGoto.GetMeStmts().empty(), "must not be empty");
  CondGotoMeStmt &condGotoStmt = static_cast<CondGotoMeStmt&>(partialCondGoto.GetMeStmts().back());
  condGotoStmt.SetOffset(partialHead->GetBBLabel());
  size_t index = exitedBB.GetPred().size();
  while (index > 0) {
    if (exitedBB.GetPred(index - 1) == &exitingBB) {
      break;
    }
    --index;
  }
  exitingBB.ReplaceSucc(&exitedBB, &partialCondGoto);
  --index;
  exitedBB.AddPred(partialCondGoto, index);
  if (profValid) {
    partialCondGoto.PushBackSuccFreq(exitedBB.GetFrequency() - (loop->head->GetFrequency() % replicatedLoopNum));
  }
  partialExit->AddSucc(exitedBB);
  if (profValid) {
    partialExit->PushBackSuccFreq(loop->head->GetFrequency() % replicatedLoopNum);
  }
  CHECK_FATAL(partialExit->GetKind() == kBBCondGoto, "must be kBBCondGoto");
  if (static_cast<CondGotoMeStmt*>(partialExit->GetLastMe())->GetOffset() != partialExit->GetSucc(1)->GetBBLabel()) {
    ExchangeSucc(*partialExit);
    if (profValid) {
      auto tempFreq = partialExit->GetEdgeFreq(partialExit->GetSucc(0));
      partialExit->SetEdgeFreq(partialExit->GetSucc(0), partialExit->GetEdgeFreq(partialExit->GetSucc(1)));
      partialExit->SetEdgeFreq(partialExit->GetSucc(1), tempFreq);
    }
  }
  partialCondGoto.AddSucc(*partialHead);
  if (profValid) {
    partialCondGoto.PushBackSuccFreq(loop->head->GetFrequency() % replicatedLoopNum);
    partialCondGoto.SetFrequency(static_cast<uint32>(partialCondGoto.GetEdgeFreq(static_cast<size_t>(0)) +
        partialCondGoto.GetEdgeFreq(1)));
  }
  CHECK_FATAL(partialCondGoto.GetKind() == kBBCondGoto, "must be partialCondGoto");
  CHECK_FATAL(!partialCondGoto.GetMeStmts().empty(), "must not be empty");
  if (static_cast<CondGotoMeStmt*>(partialCondGoto.GetLastMe())->GetOffset() !=
      partialCondGoto.GetSucc(1)->GetBBLabel()) {
    ExchangeSucc(partialCondGoto);
  }
  AddPreHeader(&partialCondGoto, partialHead);
}

MeExpr *LoopUnrolling::CreateExprWithCRNode(CRNode &crNode) {
  switch (crNode.GetCRType()) {
    case kCRConstNode: {
      return irMap->CreateIntConstMeExpr(static_cast<const CRConstNode&>(crNode).GetConstValue(), PTY_i32);
    }
    case kCRVarNode: {
      CHECK_FATAL(crNode.GetExpr() != nullptr, "expr must not be nullptr");
      return crNode.GetExpr();
    }
    case kCRAddNode: {
      auto addCRNode = static_cast<const CRAddNode&>(crNode);
      auto addOpnd1 = CreateExprWithCRNode(*addCRNode.GetOpnd(0));
      auto addOpnd2 = CreateExprWithCRNode(*addCRNode.GetOpnd(1));
      MeExpr *addOpnds = irMap->CreateMeExprBinary(OP_add, PTY_i32, *addOpnd1, *addOpnd2);
      for (size_t i = 2; i < addCRNode.GetOpndsSize(); ++i) {
        auto addOpnd = CreateExprWithCRNode(*addCRNode.GetOpnd(i));
        addOpnds = irMap->CreateMeExprBinary(OP_add, PTY_i32, *addOpnds, *addOpnd);
      }
      return addOpnds;
    }
    case kCRMulNode: {
      auto mulCRNode = static_cast<const CRMulNode&>(crNode);
      auto mulOpnd1 = CreateExprWithCRNode(*mulCRNode.GetOpnd(0));
      auto mulOpnd2 = CreateExprWithCRNode(*mulCRNode.GetOpnd(1));
      MeExpr *mulOpnds = irMap->CreateMeExprBinary(OP_mul, PTY_i32, *mulOpnd1, *mulOpnd2);
      for (size_t i = 2; i < mulCRNode.GetOpndsSize(); ++i) {
        auto mulOpnd = CreateExprWithCRNode(*mulCRNode.GetOpnd(i));
        mulOpnds = irMap->CreateMeExprBinary(OP_mul, PTY_i32, *mulOpnds, *mulOpnd);
      }
      return mulOpnds;
    }
    case kCRDivNode: {
      auto divCRNode = static_cast<CRDivNode&>(crNode);
      auto opnd1 = CreateExprWithCRNode(*divCRNode.GetLHS());
      auto opnd2 = CreateExprWithCRNode(*divCRNode.GetRHS());
      return irMap->CreateMeExprBinary(OP_div, PTY_i32, *opnd1, *opnd2);
    }
    default: {
      CHECK_FATAL(false, "impossible");
    }
  }
}

void LoopUnrolling::CreateIndVarAndCondGotoStmt(CR &cr, CRNode &varNode, BB &preCondGoto, uint32 unrollTime, uint32 i) {
  // create stmt : int i = 0.
  BB *indVarAndTripCountDefBB = cfg->NewBasicBlock();
  std::string indVarName = std::string("__LoopUnrolllIndvar__") + std::to_string(i);
  VarMeExpr *indVar = CreateIndVarOrTripCountWithName(indVarName);
  indVarAndTripCountDefBB->SetKind(kBBFallthru);
  MeExpr *constMeExprForZero = irMap->CreateIntConstMeExpr(0, PTY_i32);
  indVarAndTripCountDefBB->AddMeStmtLast(
      irMap->CreateAssignMeStmt(*indVar, *constMeExprForZero, *indVarAndTripCountDefBB));
  MeSSAUpdate::InsertOstToSSACands(indVar->GetOstIdx(), *indVarAndTripCountDefBB, &cands);

  // create stmt : tripCount = (n - start) / stride.
  BB *exitingBB = cfg->GetBBFromID(loop->inloopBB2exitBBs.begin()->first);
  auto opnd0 = CreateExprWithCRNode(*cr.GetOpnd(0));
  auto opnd1 = CreateExprWithCRNode(*cr.GetOpnd(1));
  MeExpr *conditionExpr = CreateExprWithCRNode(varNode);

  MeExpr *subMeExpr = irMap->CreateMeExprBinary(OP_sub, PTY_i32, *conditionExpr, *opnd0);
  MeExpr *divMeExpr = irMap->CreateMeExprBinary(OP_div, PTY_i32, *subMeExpr, *opnd1);
  std::string tripConutName = std::string("__LoopUnrolllTripCount__") + std::to_string(i);
  VarMeExpr *tripCountExpr = CreateIndVarOrTripCountWithName(tripConutName);
  indVarAndTripCountDefBB->AddMeStmtLast(
      irMap->CreateAssignMeStmt(*tripCountExpr, *divMeExpr, *indVarAndTripCountDefBB));
  MeSSAUpdate::InsertOstToSSACands(tripCountExpr->GetOstIdx(), *indVarAndTripCountDefBB, &cands);
  for (size_t idx = 0; idx < preCondGoto.GetPred().size(); ++idx) {
    auto *bb = preCondGoto.GetPred(idx);
    bb->ReplaceSucc(&preCondGoto, indVarAndTripCountDefBB);
    preCondGoto.AddPred(*indVarAndTripCountDefBB, idx);
    if (profValid) {
      indVarAndTripCountDefBB->PushBackSuccFreq(preCondGoto.GetEdgeFreq(idx));
    }
    switch (bb->GetKind()) {
      case kBBFallthru: {
        break;
      }
      case kBBGoto: {
        auto gotoStmt = static_cast<GotoMeStmt*>(bb->GetLastMe());
        if (preCondGoto.GetBBLabel() == gotoStmt->GetOffset()) {
          LabelIdx label = func->GetOrCreateBBLabel(*indVarAndTripCountDefBB);
          gotoStmt->SetOffset(label);
        }
        break;
      }
      case kBBCondGoto: {
        auto condGotoStmt = static_cast<CondGotoMeStmt*>(bb->GetLastMe());
        if (preCondGoto.GetBBLabel() == condGotoStmt->GetOffset()) {
          LabelIdx label = func->GetOrCreateBBLabel(*indVarAndTripCountDefBB);
          condGotoStmt->SetOffset(label);
        }
        break;
      }
      default: {
        CHECK_FATAL(false, "NYI");
      }
    }
  }
  MeExpr *unrollTimeExpr = irMap->CreateIntConstMeExpr(unrollTime, PTY_i32);
  UpdateCondGotoBB(*exitingBB, *indVar, *tripCountExpr, *unrollTimeExpr);
  UpdateCondGotoStmt(preCondGoto, *indVar, *tripCountExpr, *unrollTimeExpr, loop->head->GetBBLabel());
}

void LoopUnrolling::CopyLoopForPartial(CR &cr, CRNode &varNode, uint32 j, uint32 unrollTime) {
  BB *exitingBB = cfg->GetBBFromID(loop->inloopBB2exitBBs.begin()->first);
  BB *exitedBB = *loop->inloopBB2exitBBs.begin()->second->begin();
  BB *partialCondGoto = CopyBB(*exitingBB, false);
  replicatedLoopNum = unrollTime;
  CopyLoopForPartial(*partialCondGoto, *exitedBB, *exitingBB);
  // create preCondGoto bb
  BB *preCondGoto = cfg->NewBasicBlock();
  if (profValid) {
    preCondGoto->SetFrequency(loop->preheader->GetFrequency());
  }
  size_t idx = loop->head->GetPred().size();
  while (idx > 0) {
    if (loop->head->GetPred(idx - 1) == loop->preheader) {
      break;
    }
    --idx;
  }
  loop->preheader->ReplaceSucc(loop->head, preCondGoto);
  --idx;
  loop->head->AddPred(*preCondGoto, idx);
  preCondGoto->AddSucc(*partialCondGoto);
  preCondGoto->GetSuccFreq().resize(kOperandNum);
  if (profValid) {
    preCondGoto->SetEdgeFreq(partialCondGoto, loop->head->GetFrequency() >= unrollTime ? 0 : 1);
    preCondGoto->SetEdgeFreq(loop->head, loop->head->GetFrequency() / unrollTime);
  }
  preCondGoto->SetKind(kBBCondGoto);
  CreateIndVarAndCondGotoStmt(cr, varNode, *preCondGoto, unrollTime, j);
  AddPreHeader(preCondGoto, loop->head);
  CHECK_FATAL(preCondGoto->GetKind() == kBBCondGoto, "must be kBBCondGoto");
  if (static_cast<CondGotoMeStmt*>(preCondGoto->GetLastMe())->GetOffset() != preCondGoto->GetSucc(1)->GetBBLabel()) {
    ExchangeSucc(*preCondGoto);
    auto tempFreq = preCondGoto->GetEdgeFreq(preCondGoto->GetSucc(0));
    if (profValid) {
      preCondGoto->SetEdgeFreq(preCondGoto->GetSucc(0), preCondGoto->GetEdgeFreq(preCondGoto->GetSucc(1)));
      preCondGoto->SetEdgeFreq(preCondGoto->GetSucc(1), tempFreq);
    }
  }
}

bool LoopUnrolling::LoopPartialUnrollWithVar(CR &cr, CRNode &varNode, uint32 j) {
  uint32 index = 0;
  if (!DetermineUnrollTimes(index, false)) {
    if (LoopUnrollingExecutor::enableDebug) {
      LogInfo::MapleLogger() << "codesize is too large" << "\n";
    }
    return false;
  }
  if (LoopUnrollingExecutor::enableDebug) {
    LogInfo::MapleLogger() << "partial unrolling with var" << "\n";
  }
  if (LoopUnrollingExecutor::enableDump) {
    irMap->Dump();
    profValid ? cfg->DumpToFile("cfgIncludeFreqInfobeforeLoopPartialWithVarUnrolling", false, true) :
                cfg->DumpToFile("cfgbeforeLoopPartialWithVarUnrolling");
  }
  if (!SplitCondGotoBB()) {
    return false;
  }
  uint32 unrollTime = unrollTimes[index];
  if (LoopUnrollingExecutor::enableDebug) {
    LogInfo::MapleLogger() << "unrolltime: " << unrollTime << "\n";
  }
  CopyLoopForPartial(cr, varNode, j, unrollTime);
  replicatedLoopNum = unrollTime;
  needUpdateInitLoopFreq = true;
  isUnrollWithVar = true;
  for (int64 i = 1; i < unrollTime; ++i) {
    if (i > 1) {
      needUpdateInitLoopFreq = false;
    }
    CopyAndInsertBB(false);
  }
  if (LoopUnrollingExecutor::enableDump) {
    irMap->Dump();
    profValid ? cfg->DumpToFile("cfgIncludeFreqInfoafterLoopPartialWithVarUnrolling", false, true) :
                cfg->DumpToFile("cfgafterLoopPartialWithVarUnrolling");
  }
  return true;
}

void LoopUnrollingExecutor::SetNestedLoop(const IdentifyLoops &meLoop,
                                          std::unordered_map<LoopDesc*, std::set<LoopDesc*>> &parentLoop) const {
  for (auto loop : meLoop.GetMeLoops()) {
    if (loop->nestDepth == 0) {
      continue;
    }
    CHECK_NULL_FATAL(loop->parent);
    auto it = parentLoop.find(loop->parent);
    if (it == parentLoop.end()) {
      parentLoop[loop->parent] = { loop };
    } else {
      parentLoop[loop->parent].insert(loop);
    }
  }
}

bool LoopUnrollingExecutor::IsDoWhileLoop(const MeFunction &func, LoopDesc &loop) const {
  if (loop.inloopBB2exitBBs.find(loop.head->GetBBId()) != loop.inloopBB2exitBBs.end()) {
    // if head and exit are the same bb, this must be a do-while loop
    return true;
  }
  for (auto *succ : loop.head->GetSucc()) {
    if (!loop.Has(*succ)) {
      return false;
    }
  }
  auto exitBB = func.GetCfg()->GetBBFromID(loop.inloopBB2exitBBs.begin()->first);
  for (auto pred : exitBB->GetPred()) {
    if (!loop.Has(*pred)) {
      return false;
    }
  }
  return true;
}

bool LoopUnrollingExecutor::PredIsOutOfLoopBB(const MeFunction &func, LoopDesc &loop) const {
  MeCFG *cfg = func.GetCfg();
  for (auto bbID : loop.loopBBs) {
    auto bb = cfg->GetBBFromID(bbID);
    if (bb == loop.head) {
      continue;
    }
    for (auto pred : bb->GetPred()) {
      if (!loop.Has(*pred)) {
        CHECK_FATAL(false, "pred is out of loop bb");
        return true;
      }
    }
  }
  return false;
}

bool LoopUnrollingExecutor::IsCanonicalAndOnlyOneExitLoop(const MeFunction &func, LoopDesc &loop,
    const std::unordered_map<LoopDesc*, std::set<LoopDesc*>> &parentLoop) const {
  // Only handle one nested loop.
  if (parentLoop.find(&loop) != parentLoop.end() && ProfileCheck(func)) {
    return false;
  }
  // Must be canonical loop and has one exit bb.
  if (loop.inloopBB2exitBBs.size() != 1 || loop.inloopBB2exitBBs.begin()->second->size() != 1 ||
      !loop.IsCanonicalLoop()) {
    return false;
  }
  CHECK_NULL_FATAL(loop.preheader);
  CHECK_NULL_FATAL(loop.latch);
  auto headBB = loop.head;
  auto exitBB = func.GetCfg()->GetBBFromID(loop.inloopBB2exitBBs.begin()->first);
  CHECK_FATAL(headBB->GetPred().size() == 2, "head must has two preds");
  if (!IsDoWhileLoop(func, loop)) {
    if (enableDebug) {
      LogInfo::MapleLogger() << "While-do loop" << "\n";
    }
    return false;
  }
  if (PredIsOutOfLoopBB(func, loop)) {
    return false;
  }
  // latch bb only has one pred bb
  if (loop.latch->GetPred().size() != 1 || loop.latch->GetPred(0) != exitBB) {
    return false;
  }
  return true;
}

bool LoopUnrolling::LoopUnrollingWithConst(uint64 tripCount, bool onlyFully) {
  if (tripCount == kInvalidTripCount) {
    if (LoopUnrollingExecutor::enableDebug) {
      LogInfo::MapleLogger() << "tripCount is invalid" << "\n";
    }
    return false;
  }
  if (LoopUnrollingExecutor::enableDebug) {
    LogInfo::MapleLogger() << "start unrolling with const" << "\n";
    LogInfo::MapleLogger() << "tripCount: " << tripCount << "\n";
  }
  if (LoopUnrollingExecutor::enableDebug) {
    irMap->Dump();
    func->IsIRProfValid() ? func->GetCfg()->DumpToFile("cfgIncludeFreqInfobeforLoopUnrolling", false, true) :
                            func->GetCfg()->DumpToFile("cfgbeforLoopUnrolling" +
                                                       std::to_string(loop->head->GetBBId()));
  }
  // fully unroll
  ReturnKindOfFullyUnroll returnKind = LoopFullyUnroll(static_cast<int64>(tripCount));
  if (returnKind == LoopUnrolling::kCanNotSplitCondGoto) {
    return false;
  }
  if (returnKind == LoopUnrolling::kCanFullyUnroll) {
    if (LoopUnrollingExecutor::enableDebug) {
      LogInfo::MapleLogger() << "fully unrolling" << "\n";
    }
    if (LoopUnrollingExecutor::enableDebug) {
      irMap->Dump();
      func->IsIRProfValid() ? func->GetCfg()->DumpToFile("cfgIncludeFreqInfoafterLoopFullyUnrolling", false, true) :
                              func->GetCfg()->DumpToFile("cfgafterLoopFullyUnrolling" +
                                                         std::to_string(loop->head->GetBBId()));
    }
    return true;
  }
  if (onlyFully) {
    return false;
  }
  // partial unroll with const
  if (LoopPartialUnrollWithConst(tripCount)) {
    if (LoopUnrollingExecutor::enableDebug) {
      LogInfo::MapleLogger() << "partial unrolling with const" << "\n";
    }
    if (LoopUnrollingExecutor::enableDebug) {
      irMap->Dump();
      func->IsIRProfValid() ? func->GetCfg()->DumpToFile("cfgIncludeFreqInfoafterLoopPartialWithConst", false, true) :
                              func->GetCfg()->DumpToFile("cfgafterLoopPartialWithConstUnrolling");
    }
    return true;
  }
  return false;
}

void LoopUnrollingExecutor::ExecuteLoopUnrolling(MeFunction &func, MeIRMap &irMap,
    std::map<OStIdx, std::unique_ptr<std::set<BBId>>> &cands, IdentifyLoops &meLoop, const MapleAllocator &alloc) {
  if (enableDebug) {
    LogInfo::MapleLogger() << func.GetName() << "\n";
  }
  std::unordered_map<LoopDesc*, std::set<LoopDesc*>> parentLoop;
  // Do loop unrolling only when the function is hot in profile,
  // or we only apply fully unrolling on those small loops.
  if (!ProfileCheck(func)) {
    if (func.GetMIRModule().IsJavaModule()) {
      // not apply on java
      return;
    }
    for (auto loop : meLoop.GetMeLoops()) {
      if (!IsCanonicalAndOnlyOneExitLoop(func, *loop, parentLoop)) {
        continue;
      }
      LoopScalarAnalysisResult sa(irMap, loop);
      LoopUnrolling loopUnrolling(func, *loop, irMap, alloc, cands);
      uint64 tripCount = 0;
      CRNode *conditionCRNode = nullptr;
      CR *itCR = nullptr;
      TripCountType type = sa.ComputeTripCount(func, tripCount, conditionCRNode, itCR);
      if (type == kConstCR) {
        if (loopUnrolling.LoopUnrollingWithConst(tripCount, true)) {
          isCFGChange = true;
        }
      }
    }
    return;
  }
  SetNestedLoop(meLoop, parentLoop);
  uint32 i = 0;
  for (auto loop : meLoop.GetMeLoops()) {
    if (!IsCanonicalAndOnlyOneExitLoop(func, *loop, parentLoop)) {
      continue;
    }
    LoopScalarAnalysisResult sa(irMap, loop);
    LoopUnrolling loopUnrolling(func, *loop, irMap, alloc, cands);
    uint64 tripCount = 0;
    CRNode *conditionCRNode = nullptr;
    CR *itCR = nullptr;
    TripCountType type = sa.ComputeTripCount(func, tripCount, conditionCRNode, itCR);
    if (type == kConstCR) {
      if (loopUnrolling.LoopUnrollingWithConst(tripCount)) {
        isCFGChange = true;
      }
    } else if ((type == kVarCR || type == kVarCondition) && itCR->GetOpndsSize() == kOperandNum &&
               // Will create stmt : tripCount = (n - start) / stride, so the stride must not be zero.
               itCR->GetOpnd(1)->GetCRType() == kCRConstNode &&
               static_cast<CRConstNode*>(itCR->GetOpnd(1))->GetConstValue() != 0) {
      if (loopUnrolling.LoopPartialUnrollWithVar(*itCR, *conditionCRNode, i)) {
        isCFGChange = true;
      }
      ++i;
    }
  }
}

void MELoopUnrolling::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MELoopAnalysis>();
  aDep.AddRequired<MEIRMapBuild>();
  aDep.AddRequired<MEDominance>();
  aDep.SetPreservedAll();
}

bool MELoopUnrolling::PhaseRun(maple::MeFunction &f) {
  IdentifyLoops *meLoop = GET_ANALYSIS(MELoopAnalysis, f);
  if (meLoop == nullptr) {
    return false;
  }
  auto *loopunrollMemPool = GetPhaseMemPool();
  MapleAllocator loopUnrollingAlloc = MapleAllocator(loopunrollMemPool);
  std::map<OStIdx, std::unique_ptr<std::set<BBId>>> cands((std::less<OStIdx>()));
  auto *irMap = GET_ANALYSIS(MEIRMapBuild, f);
  CHECK_NULL_FATAL(irMap);
  if (DEBUGFUNC_NEWPM(f)) {
    f.GetCfg()->DumpToFile("beforeloopunrolling", false);
  }
  LoopUnrollingExecutor loopUnrollingExe = LoopUnrollingExecutor(*loopunrollMemPool);
  loopUnrollingExe.ExecuteLoopUnrolling(f, *irMap, cands, *meLoop, loopUnrollingAlloc);
  if (loopUnrollingExe.IsCFGChange()) {
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &MEDominance::id);
    auto dom = FORCE_GET(MEDominance);
    MeSSAUpdate ssaUpdate(f, *f.GetMeSSATab(), *dom, cands);
    ssaUpdate.Run();
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &MELoopAnalysis::id);
  }
  if (DEBUGFUNC_NEWPM(f)) {
    f.GetCfg()->DumpToFile("afterloopunrolling", false);
  }
  return true;
}
}  // namespace maple
