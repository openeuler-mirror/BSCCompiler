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
#include "me_analyze_rc.h"
#include "me_option.h"
#include "me_dominance.h"
#include "me_cond_based_rc.h"
#include "me_delegate_rc.h"
#include "me_subsum_rc.h"

// This phase analyzes the defs and uses of ref pointers in the function and
// performs the following modifications to the code:
//
// A. Insert a decref for the ref pointer before each of its definition.  If the
// ref pointer is local (a localrefvar) and the analysis shows that it is the
// first definition since entering the function, the decref will be omitted.
//
// B. At each statement that assigns a new value to a ref pointer, insert an
// incref after the assignment. In cases where the incref has already been
// performed when the assigned value is computed, it will not insert the incref.
//
// C. A localrefvar need to be cleaned up before function exit.  This clean-up
// corresponds to performing a decref.  Instead of inserting a decref to clean up
// each localrefvar, the cleanup for the localrefvars in the function is
// represented aggregate via a call to the intrinsic CLEANUP_LOCALREFVARS
// inserted before each return statement.  The localrefvars to be cleaned up
// for each return statement are indicated as actual parameters in the
// intrinsiccall statement. If a localrefvar's definition cannot reach a return
// statement, it will not be included in the actual parameters. If the number
// of localrefvar parameters in the intrinsiccall is more than
// `kCleanupLocalRefVarsLimit`, the intrinsiccall will be omitted, in which
// case the code generator will clean up all localrefvars.
//
// For C, analyzerc can try to do more optimization by inserting decrefs for
// individual localrefvars earlier than the return statements.  This is done
// when the placementRC flag is set to true. This optimization is performed by
// calling PlacementRC::ApplySSUPre for all localrefvars.
// Under placementRC, the CLEANUP_LOCALREFVARS intrinsiccall will still be
// inserted for the decrefs being inserted before the return statement.
//
// When a formal parameter of ref type is ever assigned inside the function,
// an incref for it needs to be inserted at function entry.  This is done
// by the placementRC phase. For such formal parameters, placementRC phase will
// also insert decref's to clean them up after their last use.
//
// If the return statement returns a ref pointer variable, an incref for it
// needs to be effected. This is not handled by analyzerc, but is handled by
// the rcLowering phase.
//
// This phase needs to be run before register promotion, because some alias
// information is lost after a pointer is promoted to preg. Because it is run
// after EPRE phase, there can be regassign's and regread's of ref type but
// they will not cause decref or incref insertion.
//
// if number of live localrefvars is more than this limit at a return, we will
// not insert the intrinsiccall to CLEANUP_LOCALREFVARS
namespace {
constexpr int kCleanupLocalRefVarsLimit = 200;
}  // namespace

namespace maple {
void RCItem::Dump() {
  ost.Dump();
  if (!noAlias) {
    LogInfo::Info() << " aliased";
  }
  if (nonLocal) {
    LogInfo::Info() << " nonLocal";
  }
  if (isFormal) {
    LogInfo::Info() << " isFormal";
  }
  for (BBId bbId : occurBBs) {
    LogInfo::Info() << " " << bbId;
  }
  LogInfo::Info() << '\n';
}

RCItem *AnalyzeRC::FindOrCreateRCItem(OriginalSt &ost) {
  auto mapIt = rcItemsMap.find(ost.GetIndex());
  if (mapIt != rcItemsMap.end()) {
    return mapIt->second;
  }
  RCItem *rcItem = analyzeRCMp->New<RCItem>(ost, analyzeRCAllocator);
  rcItemsMap[ost.GetIndex()] = rcItem;
  if (ost.GetIndex() >= aliasClass.GetAliasElemCount()) {
    rcItem->noAlias = true;
  } else {
    AliasElem *ae = aliasClass.FindAliasElem(ost);
    rcItem->noAlias = ae->GetClassSet() == nullptr;
  }
  rcItem->nonLocal = ost.GetIndirectLev() > 0 || ost.GetMIRSymbol()->IsGlobal();
  if (!rcItem->nonLocal) {
    rcItem->isFormal = ost.GetMIRSymbol()->GetStorageClass() == kScFormal;
  }
  return rcItem;
}

OriginalSt *AnalyzeRC::GetOriginalSt(const MeExpr &refLHS) const {
  if (refLHS.GetMeOp() == kMeOpVar) {
    auto &varMeExpr = static_cast<const VarMeExpr&>(refLHS);
    return varMeExpr.GetOst();
  }
  ASSERT(refLHS.GetMeOp() == kMeOpIvar, "GetOriginalSt: unexpected node type");
  auto &ivarMeExpr = static_cast<const IvarMeExpr&>(refLHS);
  if (ivarMeExpr.GetMu() != nullptr) {
    return ivarMeExpr.GetMu()->GetOst();
  }
  ASSERT(ivarMeExpr.GetDefStmt() != nullptr, "GetOriginalSt: ivar with mu==nullptr has no defStmt");
  IassignMeStmt *iass = ivarMeExpr.GetDefStmt();
  CHECK_FATAL(!iass->GetChiList()->empty(), "GetOriginalSt: ivar with mu==nullptr has empty chiList at its def");
  return iass->GetChiList()->begin()->second->GetLHS()->GetOst();
}

VarMeExpr *AnalyzeRC::GetZeroVersionVarMeExpr(const VarMeExpr &var) {
  OriginalSt *ost = var.GetOst();
  return irMap.GetOrCreateZeroVersionVarMeExpr(*ost);
}

// check if incref needs to be inserted after this ref pointer assignment;
// if it is callassigned, the incref has already been done in the callee;
// if rhs is gcmalloc/gcmallocjarray, the refcount is already 1;
// if rhs is neither dread or iread, it cannot be a pointer, so incref not needed
bool AnalyzeRC::NeedIncref(const MeStmt &stmt) const {
  if (kOpcodeInfo.IsCallAssigned(stmt.GetOp())) {
    return false;
  }
  MeExpr *rhs = stmt.GetRHS();
  CHECK_NULL_FATAL(rhs);
  return rhs->PointsToSomethingThatNeedsIncRef();
}

// identify assignments to ref pointers and insert decref before it and incref
// after it
void AnalyzeRC::IdentifyRCStmts() {
  auto eIt = cfg->valid_end();
  for (auto bIt = cfg->valid_begin(); bIt != eIt; ++bIt) {
    auto &bb = **bIt;
    for (auto &stmt : bb.GetMeStmts()) {
      MeExpr *lhsRef = stmt.GetLHSRef(skipLocalRefVars);
      if (lhsRef != nullptr) {
        OriginalSt *ost = GetOriginalSt(*lhsRef);
        ASSERT(ost != nullptr, "IdentifyRCStmts: cannot get SymbolOriginalSt");
        (void)FindOrCreateRCItem(*ost);
        // this part for inserting decref
        if (lhsRef->GetMeOp() == kMeOpVar) {
          // insert a decref statement
          UnaryMeStmt *decrefStmt = irMap.CreateUnaryMeStmt(
              OP_decref, GetZeroVersionVarMeExpr(static_cast<const VarMeExpr&>(*lhsRef)), &bb, &stmt.GetSrcPosition());
          // insertion position is before stmt
          bb.InsertMeStmtBefore(&stmt, decrefStmt);
        } else {
          auto *lhsIvar = static_cast<IvarMeExpr*>(lhsRef);
          {
            // insert a decref statement
            IvarMeExpr ivarMeExpr(-1, lhsIvar->GetPrimType(), lhsIvar->GetTyIdx(), lhsIvar->GetFieldID());
            ivarMeExpr.SetBase(lhsIvar->GetBase());
            // form mu from chiList
            auto &iass = static_cast<IassignMeStmt&>(stmt);
            MapleMap<OStIdx, ChiMeNode*>::iterator xit = iass.GetChiList()->begin();
            for (; xit != iass.GetChiList()->end(); ++xit) {
              ChiMeNode *chi = xit->second;
              if (chi->GetRHS()->GetOst() == ost) {
                ivarMeExpr.SetMuVal(chi->GetRHS());
                break;
              }
            }
            ASSERT(xit != iass.GetChiList()->end(), "IdentifyRCStmts: failed to find corresponding chi node");
            UnaryMeStmt *decrefStmt = irMap.CreateUnaryMeStmt(
                OP_decref, irMap.HashMeExpr(ivarMeExpr), &bb, &stmt.GetSrcPosition());
            // insertion position is before stmt
            bb.InsertMeStmtBefore(&stmt, decrefStmt);
            ost = GetOriginalSt(*decrefStmt->GetOpnd());
            ASSERT(ost != nullptr, "IdentifyRCStmts: cannot get SymbolOriginalSt");
            (void)FindOrCreateRCItem(*ost);
          }
        }
        // this part for inserting incref
        if (NeedIncref(stmt)) {
          stmt.EnableNeedIncref();
        }
      }
    }  // end of stmt iteration
  }
}

void AnalyzeRC::CreateCleanupIntrinsics() {
  for (BB *bb : cfg->GetCommonExitBB()->GetPred()) {
    auto &meStmts = bb->GetMeStmts();
    if (meStmts.empty() || meStmts.back().GetOp() != OP_return) {
      continue;
    }
    std::vector<MeExpr*> opnds;
    for (const auto &mapItem : rcItemsMap) {
      RCItem *rcItem = mapItem.second;
      if (rcItem->nonLocal || rcItem->isFormal) {
        continue;
      }
      opnds.push_back(irMap.GetOrCreateZeroVersionVarMeExpr(rcItem->ost));
    }
    IntrinsiccallMeStmt *intrn = irMap.CreateIntrinsicCallMeStmt(INTRN_MPL_CLEANUP_LOCALREFVARS, opnds);
    bb->InsertMeStmtBefore(&(meStmts.back()), intrn);
  }
}

void AnalyzeRC::TraverseStmt(BB &bb) {
  if (bb.GetMeStmts().empty()) {
    return;
  }
  for (auto &meStmt : bb.GetMeStmts()) {
    if (meStmt.GetOp() == OP_decref || (meStmt.GetOp() == OP_intrinsiccall &&
        static_cast<IntrinsiccallMeStmt&>(meStmt).GetIntrinsic() == INTRN_MPL_CLEANUP_LOCALREFVARS)) {
      RenameUses(meStmt);
    } else {
      MeExpr *lhsRef = meStmt.GetLHSRef(skipLocalRefVars);
      if (lhsRef == nullptr) {
        continue;
      }
      const OriginalSt *ost = GetOriginalSt(*lhsRef);
      RCItem *rcItem = rcItemsMap[ost->GetIndex()];
      if (!rcItem->nonLocal) {
        rcItem->versionStack.push(lhsRef);
      }
    }
  }
}

void AnalyzeRC::RenameRefPtrs(BB *bb) {
  if (skipLocalRefVars || bb == nullptr) {
    return;
  }
  std::map<RCItem*, size_t> savedStacksize;  // to record stack size
  // in each RCItem for stack pop-ups
  for (const auto &mapItem : rcItemsMap) {
    RCItem *rcItem = mapItem.second;
    if (rcItem->nonLocal) {
      continue;
    }
    // record stack size
    savedStacksize[rcItem] = rcItem->versionStack.size();
    // if there is a phi, push stack
    auto phiIt = bb->GetMePhiList().find(mapItem.second->ost.GetIndex());
    if (phiIt != bb->GetMePhiList().end()) {
      rcItem->versionStack.push((*phiIt).second->GetLHS());
    }
  }
  // traverse the BB stmts
  TraverseStmt(*bb);
  // recursive call in preorder traversal of dominator tree
  ASSERT(bb->GetBBId() < dominance.GetDomChildrenSize(), "index out of range in AnalyzeRC::RenameRefPtrs");
  const MapleSet<BBId> &domChildren = dominance.GetDomChildren(bb->GetBBId());
  for (const auto &childBBId : domChildren) {
    RenameRefPtrs(cfg->GetAllBBs().at(childBBId));
  }
  // restore the stacks to their size at entry to this function invocation
  for (const auto &mapItem : rcItemsMap) {
    RCItem *rcItem = mapItem.second;
    if (rcItem->nonLocal) {
      continue;
    }
    size_t lastSize = savedStacksize[rcItem];
    while (rcItem->versionStack.size() > lastSize) {
      rcItem->versionStack.pop();
    }
  }
}

void AnalyzeRC::RenameUses(MeStmt &meStmt) {
  for (size_t i = 0; i < meStmt.NumMeStmtOpnds(); ++i) {
    const OriginalSt *ost = GetOriginalSt(*meStmt.GetOpnd(i));
    RCItem *rcItem = rcItemsMap[ost->GetIndex()];
    if (meStmt.GetOp() == OP_intrinsiccall) {
      ASSERT(!rcItem->nonLocal, "cleanupLocalrefvars only takes locals");
    }
    if (!rcItem->nonLocal && !rcItem->versionStack.empty()) {
      meStmt.SetOpnd(i, rcItem->versionStack.top());
    }
  }
}

DassignMeStmt *AnalyzeRC::CreateDassignInit(OriginalSt &ost, BB &bb) {
  VarMeExpr *lhs = irMap.CreateVarMeExprVersion(&ost);
  MeExpr *rhs = irMap.CreateIntConstMeExpr(0, PTY_ref);
  return static_cast<DassignMeStmt *>(irMap.CreateAssignMeStmt(utils::ToRef(lhs), utils::ToRef(rhs), bb));
}

UnaryMeStmt *AnalyzeRC::CreateIncrefZeroVersion(OriginalSt &ost) {
  return irMap.CreateUnaryMeStmt(OP_incref, irMap.GetOrCreateZeroVersionVarMeExpr(ost));
}

void AnalyzeRC::OptimizeRC() {
  auto eIt = cfg->valid_end();
  for (auto bIt = cfg->valid_begin(); bIt != eIt; ++bIt) {
    auto *bb = *bIt;
    for (auto itStmt = bb->GetMeStmts().begin(); itStmt != bb->GetMeStmts().end(); ++itStmt) {
      MeStmt *stmt = to_ptr(itStmt);
      if (itStmt->GetOp() != OP_decref) {
        continue;
      }
      auto *decref = static_cast<UnaryMeStmt*>(stmt);
      MeStmt *refAssign = stmt->GetNext();
      MeExpr *opnd = decref->GetOpnd();
      OriginalSt *ost = GetOriginalSt(*opnd);
      if (opnd->GetMeOp() == kMeOpVar && ost->IsLocal() && skipLocalRefVars) {
        continue;
      }
      RCItem *rcItem = rcItemsMap[ost->GetIndex()];
      if (!rcItem->isFormal && opnd->GetMeOp() == kMeOpVar) {
        // !nonLocal && noAlias
        rcItem->occurBBs.insert(bb->GetBBId());
        rcItem->needSomeRC = true;
      }
      if (NeedDecRef(*rcItem, *opnd)) {
        refAssign->EnableNeedDecref();
      } else {
        bb->RemoveMeStmt(stmt);  // delete the decref
      }
      ++itStmt;  // next iteration will process the stmt after refAssign
    }
  }
}

bool AnalyzeRC::NeedDecRef(const RCItem &rcItem, MeExpr &expr) const {
  CHECK_FATAL((rcItem.nonLocal || rcItem.noAlias), "OptimizeRC: local pointers cannot have alias");
  // see if the decref can be optimized away
  if (rcItem.nonLocal) {
    // the decref can be avoided for iassign thru this in constructor funcs
    if (rcItem.ost.GetIndirectLev() == 1 && func.GetMirFunc()->IsConstructor()) {
      auto &ivarMeExpr = static_cast<IvarMeExpr&>(expr);
      return NeedDecRef(ivarMeExpr);
    }
    return true;
  }
  if (rcItem.isFormal || expr.GetMeOp() != kMeOpVar) {
    return true;
  }
  auto &varMeExpr = static_cast<VarMeExpr&>(expr);
  return NeedDecRef(varMeExpr);
}

bool AnalyzeRC::NeedDecRef(IvarMeExpr &ivar) const {
  auto *base = ivar.GetBase();
  if (base->GetMeOp() != kMeOpVar) {
    return true;
  }
  auto *baseVar = static_cast<VarMeExpr*>(base);
  const MIRSymbol *sym = baseVar->GetOst()->GetMIRSymbol();
  if (sym->GetStorageClass() != kScFormal || sym != func.GetMirFunc()->GetFormal(0)) {
    return true;
  }
  MIRType *baseType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ivar.GetTyIdx());
  MIRType *type = static_cast<MIRPtrType*>(baseType)->GetPointedType();
  if (!instance_of<MIRClassType>(type)) {
    return true;
  }
  auto *classType = static_cast<MIRClassType*>(type);
  // check ivarMeExpr->fieldID is not from base classes
  if (!classType->IsOwnField(ivar.GetFieldID())) {
    return true;
  }
  // check the ivar's mu is zero version
  OriginalSt *ost = GetOriginalSt(ivar);
  if (ivar.GetMu() == nullptr) {
    return false;
  }
  return ivar.GetMu()->GetVstIdx() != ost->GetZeroVersionIndex() && ivar.GetMu()->GetDefBy() != kDefByNo;
}

bool AnalyzeRC::NeedDecRef(const VarMeExpr &var) const {
  OriginalSt *ost = GetOriginalSt(var);
  return var.GetVstIdx() != ost->GetZeroVersionIndex() && var.GetDefBy() != kDefByNo;
}

// among the arguments in the intrinsiccall to INTRN_CLEANUP_LOCALREFVARS, those
// that are zero version are not live, and can be deleted; if the number of
// arguments left are > `kCleanupLocalRefVarsLimit`, delete the intrinsiccall.
void AnalyzeRC::RemoveUnneededCleanups() {
  for (BB *bb : cfg->GetCommonExitBB()->GetPred()) {
    auto &meStmts = bb->GetMeStmts();
    if (meStmts.empty() || meStmts.back().GetOp() != OP_return) {
      continue;
    }
    MeStmt *meStmt = meStmts.back().GetPrev();
    CHECK_NULL_FATAL(meStmt);
    ASSERT(meStmt->GetOp() == OP_intrinsiccall, "RemoveUnneededCleanups: cannot find cleanup intrinsic stmt");
    auto intrn = static_cast<IntrinsiccallMeStmt*>(meStmt);
    ASSERT(intrn->GetIntrinsic() == INTRN_MPL_CLEANUP_LOCALREFVARS,
           "RemoveUnneededCleanups: cannot find cleanup intrinsic stmt");
    size_t nextPos = 0;
    size_t i = 0;
    for (; i < intrn->NumMeStmtOpnds(); ++i) {
      auto varMeExpr = static_cast<VarMeExpr*>(intrn->GetOpnd(i));
      if (varMeExpr->IsZeroVersion()) {
        continue;
      }
      if (nextPos != i) {
        intrn->SetOpnd(nextPos, varMeExpr);
      }
      ++nextPos;
    }
    while (nextPos < i) {
      intrn->PopBackOpnd();
      --i;
    }
    if (intrn->NumMeStmtOpnds() > kCleanupLocalRefVarsLimit) {
      bb->RemoveMeStmt(intrn);  // delete the intrinsiccall stmt
    }
  }
}

void AnalyzeRC::Run() {
  if (func.GetHints() & kPlacementRCed) {
    skipLocalRefVars = true;
  } else {
    func.SetHints(func.GetHints() | kAnalyzeRCed);
  }
  IdentifyRCStmts();
  if (!skipLocalRefVars) {
    CreateCleanupIntrinsics();
  }
  RenameRefPtrs(cfg->GetCommonEntryBB());
  if (MeOption::optLevel > 0 && !skipLocalRefVars) {
    RemoveUnneededCleanups();
  }
  OptimizeRC();
}

void MEAnalyzeRC::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>();
  aDep.AddRequired<MEAliasClass>();
  aDep.AddRequired<MEIRMapBuild>();
  aDep.SetPreservedAll();
}

bool MEAnalyzeRC::PhaseRun(maple::MeFunction &f) {
  auto *dom = GET_ANALYSIS(MEDominance, f);
  ASSERT(dom != nullptr, "dominance phase has problem");
  auto *aliasClass = GET_ANALYSIS(MEAliasClass, f);
  ASSERT(aliasClass != nullptr, "aliasClass phase has problem");
  ASSERT_NOT_NULL((GET_ANALYSIS(MEIRMapBuild, f)));
  if (DEBUGFUNC_NEWPM(f)) {
    LogInfo::Info() << " Processing " << f.GetMirFunc()->GetName() << '\n';
  }
  // add extra scope so destructor for analyzerc will be invoked earlier
  AnalyzeRC analyzerc(f, *dom, *aliasClass, GetPhaseMemPool());
  analyzerc.Run();
  if (DEBUGFUNC_NEWPM(f)) {
    LogInfo::Info() << "\n============== After ANALYZE RC =============" << '\n';
    f.Dump(false);
  }
  if (MeOption::subsumRC && MeOption::rcLowering && MeOption::optLevel > 0) {
    GetAnalysisInfoHook()->ForceRunTransFormPhase<meFuncOptTy, MeFunction>(&MESubsumRC::id, f);
  }
  if (!MeOption::noDelegateRC && MeOption::rcLowering && MeOption::optLevel > 0) {
    GetAnalysisInfoHook()->ForceRunTransFormPhase<meFuncOptTy, MeFunction>(&MEDelegateRC::id, f);
  }
  if (!MeOption::noCondBasedRC && !(f.GetHints() & kPlacementRCed) &&
      MeOption::rcLowering && MeOption::optLevel > 0) {
    GetAnalysisInfoHook()->ForceRunTransFormPhase<meFuncOptTy, MeFunction>(&MECondBasedRC::id, f);
  }
  if (DEBUGFUNC_NEWPM(f)) {
    LogInfo::Info() << "\n============== After delegate RC and condbased RC =============" << '\n';
    f.Dump(false);
  }
  return true;
}
}  // namespace maple
