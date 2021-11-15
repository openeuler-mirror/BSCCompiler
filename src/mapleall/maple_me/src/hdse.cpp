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
#include "hdse.h"
#include <iostream>
#include "ssa_mir_nodes.h"
#include "ver_symbol.h"
#include "irmap.h"
#include "opcode_info.h"
#include "mir_preg.h"
#include "utils.h"

namespace maple {
using namespace utils;

void HDSE::DetermineUseCounts(MeExpr *x) {
  if (x->GetMeOp() == kMeOpVar) {
    VarMeExpr *varmeexpr = static_cast<VarMeExpr *>(x);
    verstUseCounts[varmeexpr->GetVstIdx()]++;
    return;
  }
  for (uint32 i = 0; i < x->GetNumOpnds(); ++i) {
    DetermineUseCounts(x->GetOpnd(i));
  }
}

void HDSE::CheckBackSubsCandidacy(DassignMeStmt *dass) {
  if (!dass->GetChiList()->empty()) {
    return;
  }
  if (dass->GetRHS()->GetMeOp() != kMeOpVar && dass->GetRHS()->GetMeOp() != kMeOpReg) {
    return;
  }
  ScalarMeExpr *lhsscalar = static_cast<ScalarMeExpr *>(dass->GetLHS());
  OriginalSt *ost = lhsscalar->GetOst();
  if (!ost->IsLocal()) {
    return;
  }
  MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ost->GetTyIdx());
  if (ty->GetPrimType() == PTY_agg && ty->GetSize() <= 16) {
    return;
  }
  ScalarMeExpr *rhsscalar = static_cast<ScalarMeExpr *>(dass->GetRHS());
  if (rhsscalar->GetDefBy() != kDefByMustDef) {
    return;
  }
  if (rhsscalar->DefByBB() != dass->GetBB()) {
    return;
  }
  // skip stmtpre candidate
  auto defStmt = rhsscalar->GetDefMustDef().GetBase();
  if (defStmt->GetOp() == OP_callassigned) {
    MIRFunction &callee = static_cast<CallMeStmt*>(defStmt)->GetTargetFunction();
    if (callee.IsPure() && callee.IsNoThrowException()) {
      return;
    }
  }
  backSubsCands.push_front(dass);
}

void HDSE::RemoveNotRequiredStmtsInBB(BB &bb) {
  MeStmt *mestmt = &bb.GetMeStmts().front();
  MeStmt *nextstmt = nullptr;
  while (mestmt) {
    nextstmt = mestmt->GetNext();
    if (!mestmt->GetIsLive()) {
      if (hdseDebug) {
        mirModule.GetOut() << "========== HSSA DSE is deleting this stmt: ";
        mestmt->Dump(&irMap);
      }
      if (mestmt->GetOp() != OP_dassign &&
          (mestmt->IsCondBr() || mestmt->GetOp() == OP_switch || mestmt->GetOp() == OP_igoto)) {
        // update CFG
        while (bb.GetSucc().size() != 1) {
          BB *succ = bb.GetSucc().back();
          succ->RemovePred(bb);
        }
        bb.SetKind(kBBFallthru);
      }
      // A ivar contained in stmt
      if (stmt2NotNullExpr.find(mestmt) != stmt2NotNullExpr.end()) {
        for (MeExpr *meExpr : stmt2NotNullExpr.at(mestmt)) {
          if (NeedNotNullCheck(*meExpr, bb)) {
            UnaryMeStmt *nullCheck = irMap.New<UnaryMeStmt>(OP_assertnonnull);
            nullCheck->SetBB(&bb);
            nullCheck->SetSrcPos(mestmt->GetSrcPosition());
            nullCheck->SetMeStmtOpndValue(meExpr);
            bb.InsertMeStmtBefore(mestmt, nullCheck);
            nullCheck->SetIsLive(true);
            notNullExpr2Stmt[meExpr].push_back(nullCheck);
          }
        }
      }
      bb.RemoveMeStmt(mestmt);
    } else {
      if (mestmt->IsCondBr()) { // see if foldable to unconditional branch
        CondGotoMeStmt *condbr = static_cast<CondGotoMeStmt *>(mestmt);
        if (!mirModule.IsJavaModule() && condbr->GetOpnd()->GetMeOp() == kMeOpConst) {
          CHECK_FATAL(IsPrimitiveInteger(condbr->GetOpnd()->GetPrimType()),
                      "MeHDSE::DseProcess: branch condition must be integer type");
          if ((condbr->GetOp() == OP_brtrue && condbr->GetOpnd()->IsZero()) ||
              (condbr->GetOp() == OP_brfalse && !condbr->GetOpnd()->IsZero())) {
            // delete the conditional branch
            BB *succbb = bb.GetSucc().back();
            succbb->RemoveBBFromPred(bb, false);
            bb.GetSucc().pop_back();
            bb.SetKind(kBBFallthru);
            bb.RemoveMeStmt(mestmt);
          } else {
            // change to unconditional branch
            BB *succbb = bb.GetSucc().front();
            succbb->RemoveBBFromPred(bb, false);
            (void)bb.GetSucc().erase(bb.GetSucc().begin());
            bb.SetKind(kBBGoto);
            GotoMeStmt *gotomestmt = irMap.New<GotoMeStmt>(condbr->GetOffset());
            bb.ReplaceMeStmt(condbr, gotomestmt);
          }
        } else {
          DetermineUseCounts(condbr->GetOpnd());
        }
      } else {
        for (uint32 i = 0; i < mestmt->NumMeStmtOpnds(); ++i) {
          DetermineUseCounts(mestmt->GetOpnd(i));
        }
        if (mestmt->GetOp() == OP_dassign) {
          CheckBackSubsCandidacy(static_cast<DassignMeStmt *>(mestmt));
        }
      }
    }
    mestmt = nextstmt;
  }
  // update verstUseCOunts for uses in phi operands
  for (std::pair<OStIdx, MePhiNode *> phipair : bb.GetMePhiList()) {
    if (phipair.second->GetIsLive()) {
      for (ScalarMeExpr *phiOpnd : phipair.second->GetOpnds()) {
        VarMeExpr *varx = dynamic_cast<VarMeExpr *>(phiOpnd);
        if (varx) {
          verstUseCounts[varx->GetVstIdx()]++;
        }
      }
    }
  }
}

// If a ivar's base not used as not null, should insert a not null stmt
// Only make sure throw NPE in same BB
// If must make sure throw at first stmt, much more not null stmt will be inserted
bool HDSE::NeedNotNullCheck(MeExpr &meExpr, const BB &bb) {
  if (theMIRModule->IsCModule()) {
    return false;
  }
  if (meExpr.GetOp() == OP_addrof) {
    return false;
  }
  if (meExpr.GetOp() == OP_iaddrof && static_cast<OpMeExpr &>(meExpr).GetFieldID() > 0) {
    return false;
  }

  for (MeStmt *stmt : notNullExpr2Stmt[&meExpr]) {
    if (!stmt->GetIsLive()) {
      continue;
    }
    if (postDom.Dominate(*(stmt->GetBB()), bb)) {
      return false;
    }
  }
  return true;
}

void HDSE::MarkMuListRequired(MapleMap<OStIdx, ScalarMeExpr*> &muList) {
  for (auto &pair : muList) {
    workList.push_front(pair.second);
  }
}

void HDSE::MarkChiNodeRequired(ChiMeNode &chiNode) {
  if (chiNode.GetIsLive()) {
    return;
  }
  chiNode.SetIsLive(true);
  workList.push_front(chiNode.GetRHS());
  MeStmt *meStmt = chiNode.GetBase();

  // set MustDefNode live, which defines the chiNode.
  auto *mustDefList = meStmt->GetMustDefList();
  if (mustDefList != nullptr) {
    for (auto &mustDef : *mustDefList) {
      if (aliasInfo->MayAlias(mustDef.GetLHS()->GetOst(), chiNode.GetLHS()->GetOst())) {
        mustDef.SetIsLive(true);
      }
    }
  }

  MarkStmtRequired(*meStmt);
}

template <class VarOrRegPhiNode>
void HDSE::MarkPhiRequired(VarOrRegPhiNode &mePhiNode) {
  if (mePhiNode.GetIsLive()) {
    return;
  }
  mePhiNode.SetIsLive(true);
  for (auto *meExpr : mePhiNode.GetOpnds()) {
    if (meExpr != nullptr) {
      MarkSingleUseLive(*meExpr);
    }
  }
  MarkControlDependenceLive(*mePhiNode.GetDefBB());
}

void HDSE::MarkVarDefByStmt(VarMeExpr &varExpr) {
  switch (varExpr.GetDefBy()) {
    case kDefByNo:
      break;
    case kDefByStmt: {
      auto *defStmt = varExpr.GetDefStmt();
      if (defStmt != nullptr) {
        MarkStmtRequired(*defStmt);
      }
      break;
    }
    case kDefByPhi: {
      MarkPhiRequired(varExpr.GetDefPhi());
      break;
    }
    case kDefByChi: {
      auto *defChi = &varExpr.GetDefChi();
      if (defChi != nullptr) {
        MarkChiNodeRequired(*defChi);
      }
      break;
    }
    case kDefByMustDef: {
      auto *mustDef = &varExpr.GetDefMustDef();
      if (!mustDef->GetIsLive()) {
        mustDef->SetIsLive(true);
        MarkStmtRequired(*mustDef->GetBase());
      }
      break;
    }
    default:
      ASSERT(false, "var defined wrong");
      break;
  }
}

void HDSE::MarkRegDefByStmt(RegMeExpr &regMeExpr) {
  PregIdx regIdx = regMeExpr.GetRegIdx();
  if (regIdx == -kSregRetval0) {
    if (regMeExpr.GetDefStmt()) {
      MarkStmtRequired(*regMeExpr.GetDefStmt());
    }
    return;
  }
  switch (regMeExpr.GetDefBy()) {
    case kDefByNo:
      break;
    case kDefByStmt: {
      auto *defStmt = regMeExpr.GetDefStmt();
      if (defStmt != nullptr) {
        MarkStmtRequired(*defStmt);
      }
      break;
    }
    case kDefByPhi:
      MarkPhiRequired(regMeExpr.GetDefPhi());
      break;
    case kDefByChi: {
      ASSERT(regMeExpr.GetOst()->GetIndirectLev() > 0,
             "MarkRegDefByStmt: preg cannot be defined by chi");
      auto *defChi = &regMeExpr.GetDefChi();
      if (defChi != nullptr) {
        MarkChiNodeRequired(*defChi);
      }
      break;
    }
    case kDefByMustDef: {
      MustDefMeNode *mustDef = &regMeExpr.GetDefMustDef();
      if (!mustDef->GetIsLive()) {
        mustDef->SetIsLive(true);
        MarkStmtRequired(*mustDef->GetBase());
      }
      break;
    }
    default:
      ASSERT(false, "MarkRegDefByStmt unexpected defBy value");
      break;
  }
}

// Find all stmt contains ivar and save to stmt2NotNullExpr
// Find all not null expr used as ivar's base、OP_array's or OP_assertnonnull's opnd
// And save to notNullExpr2Stmt
void HDSE::CollectNotNullExpr(MeStmt &stmt) {
  size_t opndNum = stmt.NumMeStmtOpnds();
  uint8 exprType = kExprTypeNormal;
  for (size_t i = 0; i < opndNum; ++i) {
    MeExpr *opnd = stmt.GetOpnd(i);
    if (i == 0 && instance_of<CallMeStmt>(stmt)) {
      // A non-static call's first opnd is this, should be not null
      CallMeStmt &callStmt = static_cast<CallMeStmt&>(stmt);
      exprType = callStmt.GetTargetFunction().IsStatic() ? kExprTypeNormal : kExprTypeNotNull;
    } else {
      // A normal opnd not sure
      MeExprOp meOp = opnd->GetMeOp();
      if (meOp == kMeOpVar || meOp == kMeOpReg) {
        continue;
      }
      exprType = kExprTypeNormal;
    }
    CollectNotNullExpr(stmt, ToRef(opnd), exprType);
  }
}

void HDSE::CollectNotNullExpr(MeStmt &stmt, MeExpr &meExpr, uint8 exprType) {
  MeExprOp meOp = meExpr.GetMeOp();
  switch (meOp) {
    case kMeOpVar:
    case kMeOpReg:
    case kMeOpConst: {
      PrimType type = meExpr.GetPrimType();
      // Ref expr used in ivar、array or assertnotnull
      if (exprType != kExprTypeNormal && (type == PTY_ref || type == PTY_ptr)) {
        notNullExpr2Stmt[&meExpr].push_back(&stmt);
      }
      break;
    }
    case kMeOpIvar: {
      MeExpr *base = static_cast<IvarMeExpr&>(meExpr).GetBase();
      if (exprType != kExprTypeIvar) {
        stmt2NotNullExpr[&stmt].push_back(base);
        MarkSingleUseLive(meExpr);
      }
      notNullExpr2Stmt[base].push_back(&stmt);
      CollectNotNullExpr(stmt, ToRef(base), kExprTypeIvar);
      break;
    }
    default: {
      if (exprType != kExprTypeNormal) {
        // Ref expr used in ivar、array or assertnotnull
        PrimType type = meExpr.GetPrimType();
        if (type == PTY_ref || type == PTY_ptr) {
          notNullExpr2Stmt[&meExpr].push_back(&stmt);
        }
      } else {
        // Ref expr used array or assertnotnull
        Opcode op = meExpr.GetOp();
        bool notNull = op == OP_array || kOpcodeInfo.IsAssertNonnull(op);
        exprType = notNull ? kExprTypeNotNull : kExprTypeNormal;
      }
      for (size_t i = 0; i < meExpr.GetNumOpnds(); ++i) {
        CollectNotNullExpr(stmt, ToRef(meExpr.GetOpnd(i)), exprType);
      }
      break;
    }
  }
}

void HDSE::PropagateUseLive(MeExpr &meExpr) {
  switch (meExpr.GetMeOp()) {
    case kMeOpVar: {
      auto &varMeExpr = static_cast<VarMeExpr&>(meExpr);
      MarkVarDefByStmt(varMeExpr);
      return;
    }
    case kMeOpReg: {
      auto &regMeExpr = static_cast<RegMeExpr&>(meExpr);
      MarkRegDefByStmt(regMeExpr);
      return;
    }
    default: {
      ASSERT(false, "MeOp ERROR");
      return;
    }
  }
}

bool HDSE::ExprHasSideEffect(const MeExpr &meExpr) const {
  Opcode op = meExpr.GetOp();
  // in c language, OP_array and OP_div has no side-effect
  if (mirModule.IsCModule() && (op == OP_array || op == OP_div || op == OP_rem)) {
    return false;
  }
  if (kOpcodeInfo.HasSideEffect(op)) {
    return true;
  }
  // may throw exception
  if (op == OP_gcmallocjarray || op == OP_gcpermallocjarray) {
    return true;
  }
  // create a instance of interface
  if (op == OP_gcmalloc || op == OP_gcpermalloc) {
    auto &gcMallocMeExpr = static_cast<const GcmallocMeExpr&>(meExpr);
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(gcMallocMeExpr.GetTyIdx());
    return type->GetKind() == kTypeInterface;
  }
  return false;
}

bool HDSE::ExprNonDeletable(const MeExpr &meExpr) const {
  if (ExprHasSideEffect(meExpr)) {
    return true;
  }

  switch (meExpr.GetMeOp()) {
    case kMeOpReg: {
      auto &regMeExpr = static_cast<const RegMeExpr&>(meExpr);
      return (regMeExpr.GetRegIdx() == -kSregThrownval);
    }
    case kMeOpVar: {
      auto &varMeExpr = static_cast<const VarMeExpr&>(meExpr);
      return varMeExpr.IsVolatile() ||
             (decoupleStatic && varMeExpr.GetOst()->GetMIRSymbol()->IsGlobal());
    }
    case kMeOpIvar: {
      auto &opIvar = static_cast<const IvarMeExpr&>(meExpr);
      return opIvar.IsVolatile() || ExprNonDeletable(*opIvar.GetBase());
    }
    case kMeOpNary: {
      auto &opNary = static_cast<const NaryMeExpr&>(meExpr);
      if (meExpr.GetOp() == OP_intrinsicop) {
        IntrinDesc *intrinDesc = &IntrinDesc::intrinTable[opNary.GetIntrinsic()];
        return (!intrinDesc->HasNoSideEffect());
      }
      break;
    }
    default:
      break;
  }
  for (size_t i = 0; i != meExpr.GetNumOpnds(); ++i) {
    if (ExprNonDeletable(*meExpr.GetOpnd(i))) {
      return true;
    }
  }
  return false;
}

bool HDSE::HasNonDeletableExpr(const MeStmt &meStmt) const {
  Opcode op = meStmt.GetOp();
  switch (op) {
    case OP_dassign: {
      auto &dasgn = static_cast<const DassignMeStmt&>(meStmt);
      VarMeExpr *varMeExpr = static_cast<VarMeExpr *>(dasgn.GetVarLHS());
      return (varMeExpr != nullptr && varMeExpr->IsVolatile()) || ExprNonDeletable(*dasgn.GetRHS()) ||
             (hdseKeepRef && dasgn.Propagated()) || dasgn.GetWasMayDassign() ||
             (decoupleStatic && varMeExpr != nullptr && varMeExpr->GetOst()->GetMIRSymbol()->IsGlobal());
    }
    case OP_regassign: {
      auto &rasgn = static_cast<const AssignMeStmt&>(meStmt);
      return ExprNonDeletable(*rasgn.GetRHS());
    }
    case OP_maydassign:
      return true;
    case OP_iassign: {
      auto &iasgn = static_cast<const IassignMeStmt&>(meStmt);
      auto &ivarMeExpr = static_cast<IvarMeExpr&>(*iasgn.GetLHSVal());
      return ivarMeExpr.IsVolatile() || ivarMeExpr.IsFinal() ||
          ExprNonDeletable(*iasgn.GetLHSVal()->GetBase()) || ExprNonDeletable(*iasgn.GetRHS());
    }
    default:
      return false;
  }
}

void HDSE::MarkLastBranchStmtInPredBBRequired(const BB &bb) {
  for (auto predIt = bb.GetPred().begin(); predIt != bb.GetPred().end(); ++predIt) {
    BB *predBB = *predIt;
    if (predBB == &bb || predBB->GetMeStmts().empty()) {
      continue;
    }
    auto &lastStmt = predBB->GetMeStmts().back();
    if (!lastStmt.GetIsLive() && IsBranch(lastStmt.GetOp())) {
      MarkStmtRequired(lastStmt);
    }
  }
}

void HDSE::MarkLastStmtInPDomBBRequired(const BB &bb) {
  CHECK(bb.GetBBId() < postDom.GetPdomFrontierSize(), "index out of range in HDSE::MarkLastStmtInPDomBBRequired");
  for (BBId cdBBId : postDom.GetPdomFrontierItem(bb.GetBBId())) {
    BB *cdBB = bbVec[cdBBId];
    CHECK_FATAL(cdBB != nullptr, "cdBB is null in HDSE::MarkLastStmtInPDomBBRequired");
    if (cdBB == &bb) {
      continue;
    }
    if (cdBB->IsMeStmtEmpty()) {
      CHECK_FATAL(cdBB->GetAttributes(kBBAttrIsTry), "empty bb in pdom frontier must have try attributes");
      MarkLastStmtInPDomBBRequired(*cdBB);
      continue;
    }
    auto &lastStmt = cdBB->GetMeStmts().back();
    Opcode op = lastStmt.GetOp();
    CHECK_FATAL((lastStmt.IsCondBr() || op == OP_switch || op == OP_igoto || op == OP_retsub || op == OP_throw ||
                 cdBB->GetAttributes(kBBAttrIsTry) || cdBB->GetAttributes(kBBAttrWontExit)),
                "HDSE::MarkLastStmtInPDomBBRequired: control dependent on unexpected statement");
    if ((IsBranch(op) || op == OP_retsub || op == OP_throw)) {
      MarkStmtRequired(lastStmt);
    }
  }
}

void HDSE::MarkLastBranchStmtInBBRequired(BB &bb) {
  auto &meStmts = bb.GetMeStmts();
  if (!meStmts.empty()) {
    auto &lastStmt = meStmts.back();
    Opcode op = lastStmt.GetOp();
    if (IsBranch(op)) {
      MarkStmtRequired(lastStmt);
    }
  }
}

void HDSE::MarkControlDependenceLive(BB &bb) {
  if (bbRequired[bb.GetBBId()]) {
    return;
  }
  bbRequired[bb.GetBBId()] = true;

  MarkLastBranchStmtInBBRequired(bb);
  MarkLastStmtInPDomBBRequired(bb);
  MarkLastBranchStmtInPredBBRequired(bb);
}

void HDSE::MarkSingleUseLive(MeExpr &meExpr) {
  if (IsExprNeeded(meExpr)) {
    return;
  }
  SetExprNeeded(meExpr);
  MeExprOp meOp = meExpr.GetMeOp();
  switch (meOp) {
    case kMeOpVar:
    case kMeOpReg: {
      workList.push_front(&meExpr);
      break;
    }
    case kMeOpIvar: {
      auto *base = static_cast<IvarMeExpr&>(meExpr).GetBase();
      MarkSingleUseLive(*base);
      ScalarMeExpr *mu = static_cast<IvarMeExpr&>(meExpr).GetMu();
      workList.push_front(mu);
      break;
    }
    default:
      break;
  }

  for (size_t i = 0; i != meExpr.GetNumOpnds(); ++i) {
    MeExpr *operand = meExpr.GetOpnd(i);
    if (operand != nullptr) {
      MarkSingleUseLive(*operand);
    }
  }
}

void HDSE::MarkStmtUseLive(MeStmt &meStmt) {
  // mark single use
  for (size_t i = 0; i < meStmt.NumMeStmtOpnds(); ++i) {
    auto *operand = meStmt.GetOpnd(i);
    if (operand != nullptr) {
      MarkSingleUseLive(*operand);
    }
  }

  // mark MuList
  auto *muList = meStmt.GetMuList();
  if (muList != nullptr) {
    MarkMuListRequired(*muList);
  }
}

void HDSE::MarkStmtRequired(MeStmt &meStmt) {
  if (meStmt.GetIsLive()) {
    return;
  }
  meStmt.SetIsLive(true);

  if (meStmt.GetOp() == OP_comment) {
    return;
  }

  // mark use
  MarkStmtUseLive(meStmt);

  // markBB
  MarkControlDependenceLive(*meStmt.GetBB());
}

bool HDSE::StmtMustRequired(const MeStmt &meStmt, const BB &bb) const {
  Opcode op = meStmt.GetOp();
  // special opcode cannot be eliminated
  if (IsStmtMustRequire(op) || op == OP_comment) {
    return true;
  }
  // control flow in an infinite loop cannot be eliminated
  if (ControlFlowInInfiniteLoop(bb, op)) {
    return true;
  }
  // if stmt has not deletable expr
  if (HasNonDeletableExpr(meStmt)) {
    return true;
  }
  return false;
}

void HDSE::MarkSpecialStmtRequired() {
  for (auto *bb : bbVec) {
    if (bb == nullptr) {
      continue;
    }
    auto &meStmtNodes = bb->GetMeStmts();
    for (auto itStmt = meStmtNodes.rbegin(); itStmt != meStmtNodes.rend(); ++itStmt) {
      MeStmt *pStmt = to_ptr(itStmt);
      CollectNotNullExpr(*pStmt);
      if (pStmt->GetIsLive()) {
        continue;
      }
      if (StmtMustRequired(*pStmt, *bb)) {
        MarkStmtRequired(*pStmt);
      }
    }
  }
  if (IsLfo()) {
    ProcessWhileInfos();
  }
}

void HDSE::DseInit() {
  // Init bb's required flag
  bbRequired[commonEntryBB.GetBBId()] = true;
  bbRequired[commonExitBB.GetBBId()] = true;

  // Init all MeExpr to be dead;
  exprLive.resize(irMap.GetExprID(), false);
  // Init all use counts to be 0
  verstUseCounts.resize(0);
  verstUseCounts.resize(irMap.GetVerst2MeExprTable().size(), 0);

  for (auto *bb : bbVec) {
    if (bb == nullptr) {
      continue;
    }
    // mark phi nodes dead
    for (const auto &phiPair : bb->GetMePhiList()) {
      phiPair.second->SetIsLive(false);
    }

    for (auto &stmt : bb->GetMeStmts()) {
      // mark stmt dead
      stmt.SetIsLive(false);
      // mark chi nodes dead
      MapleMap<OStIdx, ChiMeNode*> *chiList = stmt.GetChiList();
      if (chiList != nullptr) {
        for (std::pair<OStIdx, ChiMeNode*> pair : *chiList) {
          pair.second->SetIsLive(false);
        }
      }
      // mark mustDef nodes dead
      if (kOpcodeInfo.IsCallAssigned(stmt.GetOp()) && stmt.GetOp() != OP_asm) {
        MapleVector<MustDefMeNode> *mustDefList = stmt.GetMustDefList();
        for (MustDefMeNode &mustDef : *mustDefList) {
          mustDef.SetIsLive(false);
        }
      }
    }
  }
}

void HDSE::InvokeHDSEUpdateLive() {
  DseInit();
  MarkSpecialStmtRequired();
  PropagateLive();
}

void HDSE::DoHDSE() {
  DseInit();
  MarkSpecialStmtRequired();
  PropagateLive();
  RemoveNotRequiredStmts();
}
} // namespace maple
