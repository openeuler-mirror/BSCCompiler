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
#include "me_loop_analysis.h"
#include "me_dominance.h"
#include "cast_opt.h"

// This phase analyses the CFG and identify the loops. The implementation is
// based on the idea that, given two basic block a and b, if b is a's pred and
// a dominates b, then there is a loop from a to b. Loop identification is done
// in a preorder traversal of the dominator tree. In this order, outer loop is
// always detected before its nested loop(s). The building of the LoopDesc data
// structure takes advantage of this ordering.
namespace maple {
// solve: back edge rhs of loop header phi
// phiLhs: lhs of loop header phi
// return true if the ost is basic iv
bool LoopDesc::CheckBasicIV(MeExpr *solve, ScalarMeExpr *phiLhs, bool onlyStepOne) const {
  if (solve == phiLhs) {
    return false;
  }
  int meet = 0;
  if (!DoCheckBasicIV(solve, phiLhs, meet, false, onlyStepOne) || meet != 1) {
    return false;
  }
  return true;
}

bool LoopDesc::CheckStepOneIV(MeExpr *solve, ScalarMeExpr *phiLhs) const {
  return CheckBasicIV(solve, phiLhs, true);
}

bool LoopDesc::DoCheckBasicIV(MeExpr *solve, ScalarMeExpr *phiLhs, int &meet, bool tryProp, bool onlyStepOne) const {
  switch (solve->GetMeOp()) {
    case kMeOpVar:
    case kMeOpReg: {
      if (solve->IsVolatile()) {
        return false;
      }
      if (solve == phiLhs) {
        ++meet;
        return true;
      }
      auto *scalar = static_cast<ScalarMeExpr*>(solve);
      if (phiLhs == nullptr || scalar->GetOstIdx() != phiLhs->GetOstIdx()) {
        if (scalar->IsDefByNo()) {  // parameter
          return true;
        }
        if (!tryProp) {
          return loopBBs.count(scalar->DefByBB()->GetBBId()) == 0;
        }
      }
      if (scalar->GetDefBy() != kDefByStmt) {
        return false;
      }
      return DoCheckBasicIV(scalar->GetDefStmt()->GetRHS(), phiLhs, meet, true, onlyStepOne);
    }
    case kMeOpConst: {
      if (onlyStepOne) {
        auto *mirConst = static_cast<ConstMeExpr*>(solve)->GetConstVal();
        return mirConst->GetKind() == kConstInt && mirConst->IsOne();
      }
      return IsPrimitiveInteger(solve->GetPrimType());
    }
    case kMeOpOp: {
      if (solve->GetOp() == OP_add) {
        auto *opMeExpr = static_cast<OpMeExpr*>(solve);
        return DoCheckBasicIV(opMeExpr->GetOpnd(0), phiLhs, meet, tryProp, onlyStepOne) &&
               DoCheckBasicIV(opMeExpr->GetOpnd(1), phiLhs, meet, tryProp, onlyStepOne);
      }
      if (solve->GetOp() == OP_sub) {
        auto *opMeExpr = static_cast<OpMeExpr*>(solve);
        return DoCheckBasicIV(opMeExpr->GetOpnd(0), phiLhs, meet, tryProp, onlyStepOne) &&
               DoCheckBasicIV(opMeExpr->GetOpnd(1), nullptr, meet, tryProp, onlyStepOne);
      }
      return false;
    }
    default:
      break;
  }
  return false;
}

bool IsStepOneIV(MeExpr &expr, const LoopDesc &loop) {
  if (!expr.IsScalar()) {
    return false;
  }
  auto &scalarExpr = static_cast<ScalarMeExpr&>(expr);
  auto *loopHeader = loop.head;
  auto &phiList = loopHeader->GetMePhiList();
  const auto it = std::as_const(phiList).find(scalarExpr.GetOstIdx());
  if (it == phiList.cend()) {
    return false;
  }
  auto *phi = it->second;
  auto *phiLhs = phi->GetLHS();
  auto *backValue = phi->GetOpnd(1);
  if (loop.loopBBs.count(loopHeader->GetPred(0)->GetBBId()) != 0) {
    backValue = phi->GetOpnd(0);
  }
  return loop.CheckStepOneIV(backValue, phiLhs);
}

// loop must be a Canonical Loop
static bool IsExprIntConstOrDefOutOfLoop(MeExpr &expr, const LoopDesc &loop) {
  if (expr.GetOp() == OP_constval && static_cast<ConstMeExpr&>(expr).GetConstVal()->GetKind() == kConstInt) {
    return true;
  }
  if (!expr.IsScalar()) {
    return false;
  }
  auto &scalarExpr = static_cast<ScalarMeExpr&>(expr);
  auto *defStmt = scalarExpr.GetDefByMeStmt();
  auto &loopBBs = loop.loopBBs;
  if (defStmt != nullptr &&
      std::find(loopBBs.begin(), loopBBs.end(), defStmt->GetBB()->GetID()) == loop.loopBBs.end()) {
    return true;
  }
  if (scalarExpr.GetDefBy() == kDefByPhi) {
    auto *phi = &scalarExpr.GetDefPhi();
    auto *defBB = phi->GetDefBB();
    // defPhi is not in the loop
    if (std::find(loopBBs.begin(), loopBBs.end(), defBB->GetID()) == loop.loopBBs.end()) {
      return true;
    }
    if (phi->GetOpnds().size() == 1) {
      return false;
    }
    if (defBB == loop.head) {
      auto *phiLhs = phi->GetLHS();
      auto *backValue = phi->GetOpnd(1);
      if (loop.loopBBs.count(loop.head->GetPred(0)->GetBBId()) != 0) {
        backValue = phi->GetOpnd(0);
      }
      if (phiLhs == backValue) {
        return true;
      }
    }
  }
  return false;
}

// Return true if the loop must have bounded number of iterations.
bool LoopDesc::IsFiniteLoop() const {
  if (!IsCanonicalLoop()) {
    return false;
  }
  auto *lastStmt = head->GetLastMe();
  if (lastStmt == nullptr || !lastStmt->IsCondBr()) {
    return false;
  }
  auto *condExpr = static_cast<CondGotoMeStmt*>(lastStmt)->GetOpnd(0);
  if (!CastOpt::IsCompareOp(condExpr->GetOp())) {
    return false;
  }
  auto *opnd0 = condExpr->GetOpnd(0);
  auto *opnd1 = condExpr->GetOpnd(1);
  bool isIV0 = IsStepOneIV(*opnd0, *this);
  bool isIV1 = IsStepOneIV(*opnd1, *this);
  bool isConstOrDefOutOfLoop0 = IsExprIntConstOrDefOutOfLoop(*opnd0, *this);
  bool isConstOrDefOutOfLoop1 = IsExprIntConstOrDefOutOfLoop(*opnd1, *this);
  if (isConstOrDefOutOfLoop0 && isConstOrDefOutOfLoop1) {
    return true;
  }
  if ((isIV0 && isConstOrDefOutOfLoop1) || (isConstOrDefOutOfLoop0 && isIV1)) {
    return true;
  }
  return false;
}

LoopDesc *IdentifyLoops::CreateLoopDesc(BB &hd, BB &tail) {
  LoopDesc *newLoop = meLoopMemPool->New<LoopDesc>(meLoopAlloc, &hd, &tail);
  meLoops.push_back(newLoop);
  return newLoop;
}

void IdentifyLoops::SetLoopParent4BB(const BB &bb, LoopDesc &loopDesc) {
  if (bbLoopParent[bb.GetBBId()] != nullptr) {
    if (loopDesc.parent == nullptr) {
      loopDesc.parent = bbLoopParent[bb.GetBBId()];
      ASSERT_NOT_NULL(loopDesc.parent);
      loopDesc.nestDepth = loopDesc.parent->nestDepth + 1;
    }
  }
  bbLoopParent[bb.GetBBId()] = &loopDesc;
}

void IdentifyLoops::SetExitBB(LoopDesc &loop) const {
  for (auto bbId : loop.loopBBs) {
    auto *bb = cfg->GetBBFromID(bbId);
    for (auto *succ : bb->GetSucc()) {
      if (loop.loopBBs.count(succ->GetBBId()) == 0) {
        if (loop.exitBB && loop.exitBB != succ) {
          loop.exitBB = nullptr;
          return;
        }
        loop.exitBB = succ;
      }
    }
  }
}

// process each BB in preorder traversal of dominator tree
void IdentifyLoops::ProcessBB(BB *bb) {
  if (bb == nullptr || bb == cfg->GetCommonExitBB()) {
    return;
  }
  const MapleUnorderedSet<LabelIdx> &addrTakenLabels = func.GetMirFunc()->GetLabelTab()->GetAddrTakenLabels();
  for (BB *pred : bb->GetPred()) {
    if (dominance->Dominate(*bb, *pred)) {
      // create a loop with bb as loop head and pred as loop tail
      LoopDesc *loop = CreateLoopDesc(*bb, *pred);
      // check try...catch
      auto found = std::find_if(bb->GetPred().begin(), bb->GetPred().end(),
                                [](const BB *pre) { return pre->GetAttributes(kBBAttrIsTry); });
      if (found != bb->GetPred().end()) {
        loop->SetHasTryBB(true);
      }
      // check igoto
      if (addrTakenLabels.find(bb->GetBBLabel()) != addrTakenLabels.end()) {
        loop->SetHasIGotoBB(true);
      }
      std::list<BB*> bodyList;
      bodyList.push_back(pred);
      while (!bodyList.empty()) {
        BB *curr = bodyList.front();
        bodyList.pop_front();
        // skip bb or if it has already been dealt with
        if (curr == bb || loop->loopBBs.count(curr->GetBBId()) == 1) {
          continue;
        }
        (void)loop->loopBBs.insert(curr->GetBBId());
        curr->SetAttributes(kBBAttrIsInLoop);
        // check try...catch
        if (curr->GetAttributes(kBBAttrIsTry) || curr->GetAttributes(kBBAttrWontExit)) {
          loop->SetHasTryBB(true);
        }
        SetLoopParent4BB(*curr, *loop);
        for (BB *curPred : curr->GetPred()) {
          bodyList.push_back(curPred);
        }
      }
      (void)loop->loopBBs.insert(bb->GetBBId());
      bb->SetAttributes(kBBAttrIsInLoop);
      SetLoopParent4BB(*bb, *loop);
      if (!loop->HasTryBB() && !loop->HasIGotoBB()) {
        ProcessPreheaderAndLatch(*loop);
      }
      SetExitBB(*loop);
    }
  }
  // recursive call
  const auto &domChildren = dominance->GetDomChildren(bb->GetID());
  for (auto bbIt = domChildren.begin(); bbIt != domChildren.end(); ++bbIt) {
    ProcessBB(cfg->GetAllBBs().at(*bbIt));
  }
}

void IdentifyLoops::Dump() const {
  for (LoopDesc *meLoop : meLoops) {
    // loop
    LogInfo::MapleLogger() << "nest depth: " << meLoop->nestDepth
                           << " loop head BB: " << meLoop->head->GetBBId()
                           << " HasTryBB: " << meLoop->HasTryBB()
                           << " IsFiniteLoop: " << meLoop->IsFiniteLoop()
                           << " tail BB:" << meLoop->tail->GetBBId() << '\n';
    LogInfo::MapleLogger() << "loop body:";
    for (auto it = meLoop->loopBBs.begin(); it != meLoop->loopBBs.end(); ++it) {
      BBId bbId = *it;
      LogInfo::MapleLogger() << bbId << " ";
    }
    LogInfo::MapleLogger() << '\n';
  }
}

void IdentifyLoops::ProcessPreheaderAndLatch(LoopDesc &loop) const {
  // If predsize of head is one, it means that one is entry bb.
  if (loop.head->GetPred().size() == 1) {
    CHECK_FATAL(cfg->GetCommonEntryBB()->GetSucc(0) == loop.head, "succ of entry bb must be head");
    loop.preheader = cfg->GetCommonEntryBB();
    CHECK_FATAL(!loop.head->GetPred(0)->GetAttributes(kBBAttrIsTry), "must not be kBBAttrIsTry");
    loop.latch = loop.head->GetPred(0);
    return;
  }
  /* for example: GetInstance.java : 152
   * There are two loop in identifyLoops, and one has no try no catch.
   * In loop canon whould ont merge loops with latch bb.
   * for () {
   *   if () {
   *     do somthing
   *     continue
   *   }
   *   try {
   *     do somthing
   *   } catch (NoSuchAlgorithmException e) {
   *     do somthing
   *   }
   * }
   */
  if (loop.head->GetPred().size() != 2) { // Head must has two preds.
    loop.SetIsCanonicalLoop(false);
    loop.SetHasTryBB(true);
    return;
  }
  if (!loop.Has(*loop.head->GetPred(0))) {
    loop.preheader = loop.head->GetPred(0);
    // loop canon phase may not be called when identloop is used
    if ((loop.preheader->GetKind() != kBBFallthru) &&
        (loop.preheader->GetKind() != kBBGoto)) {
      loop.SetIsCanonicalLoop(false);
      return;
    }
    CHECK_FATAL(loop.preheader->GetKind() == kBBFallthru ||
                loop.preheader->GetKind() == kBBGoto,
                "must be kBBFallthru or kBBGoto");
    CHECK_FATAL(loop.Has(*loop.head->GetPred(1)), "must be latch bb");
    loop.latch = loop.head->GetPred(1);
    CHECK_FATAL(!loop.latch->GetAttributes(kBBAttrIsTry), "must not be kBBAttrIsTry");
  } else {
    loop.latch = loop.head->GetPred(0);
    CHECK_FATAL(!loop.latch->GetAttributes(kBBAttrIsTry), "must not be kBBAttrIsTry");
    CHECK_FATAL(!loop.Has(*loop.head->GetPred(1)), "must be latch preheader bb");
    loop.preheader = loop.head->GetPred(1);
  }
}

bool MELoopAnalysis::PhaseRun(maple::MeFunction &f) {
  auto *dom = EXEC_ANALYSIS(MEDominance, f)->GetDomResult();
  ASSERT_NOT_NULL(dom);
  identLoops = GetPhaseAllocator()->New<IdentifyLoops>(GetPhaseMemPool(), f, dom);
  identLoops->ProcessBB(f.GetCfg()->GetCommonEntryBB());
  if (DEBUGFUNC_NEWPM(f)) {
    identLoops->Dump();
  }
  return false;
}

void MELoopAnalysis::GetAnalysisDependence(AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>();
  aDep.SetPreservedAll();
}
}  // namespace maple
