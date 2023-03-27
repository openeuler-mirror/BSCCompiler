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
#include "copy_prop.h"
#include "me_cfg.h"

namespace maple {
static constexpr uint kMaxDepth = 5;
static bool PropagatableByCopyProp(const MeExpr *newExpr) {
  ASSERT_NOT_NULL(newExpr);
  return newExpr->GetMeOp() == kMeOpReg || newExpr->GetMeOp() == kMeOpConst;
}

static bool PropagatableOpndOfOperator(const MeExpr *meExpr, Opcode op, size_t opndId) {
  if (PropagatableByCopyProp(meExpr)) {
    return true;
  }

#if TARGX86_64 || TARGX86 || TARGVM || TARGARM32
  return false;
#endif

  if (op != OP_add && op != OP_sub) {
    return false;
  }
  if (opndId != 1) {
    return false;
  }

  if (meExpr->GetOp() != OP_cvt && meExpr->GetOp() != OP_retype) {
    return false;
  }
  if (!PropagatableByCopyProp(meExpr->GetOpnd(0))) {
    return false;
  }
  return true;
}

static bool ExpectedPropedExpr(const MeExpr &expr) {
  auto meop = expr.GetMeOp();
  if (meop == kMeOpReg || meop == kMeOpConst) {
    return true;
  }
  if (meop == kMeOpOp) {
    for (size_t i = 0; i < expr.GetNumOpnds(); ++i) {
      if (!ExpectedPropedExpr(*expr.GetOpnd(i))) {
        return false;
      }
    }
    return true;
  }
  return false;
}

static bool IsSimpleExprOfOst(MeExpr &rhs, const OStIdx &idx, MeExpr *&preVersion) {
  if (rhs.GetDepth() > kMaxDepth) {
    return false;
  }
  MeExprOp meOp = rhs.GetMeOp();
  switch (meOp) {
    case kMeOpReg: {
      auto &regRead = static_cast<RegMeExpr&>(rhs);
      if (regRead.GetRegIdx() < 0) {
        return false;
      }
      if (regRead.GetOstIdx() != idx) {
        return false;
      }
      preVersion = &regRead;
      break;
    }
    case kMeOpOp: {
      if (kOpcodeInfo.NotPure(rhs.GetOp())) {
        return false;
      }
      auto &meopexpr = static_cast<const OpMeExpr &>(rhs);
      for (uint8 i = 0; i < meopexpr.GetNumOpnds(); ++i) {
        if (!meopexpr.GetOpnd(i)) {
          continue;
        }
        if (!IsSimpleExprOfOst(*meopexpr.GetOpnd(i), idx, preVersion)) {
          return false;
        }
      }
      break;
    }
    case kMeOpConst:
      return true;
    default:
      return false;
  }
  return true;
}

bool CopyProp::IsSingleUse(const MeExpr *expr) {
  if (!expr || expr->GetMeOp() != kMeOpReg) {
    return false;
  }
  auto regMeExpr = static_cast<const RegMeExpr *>(expr);
  if (regMeExpr->GetRegIdx() < 0) {  // special register
    return false;
  }
  if (!regMeExpr->IsDefByStmt()) {
    return false;
  }
  auto *useSites = useInfo.GetUseSitesOfExpr(expr);
  CHECK_NULL_FATAL(useSites);
  if (useSites->size() != 1) {
    return false;
  }
  auto useItem = useSites->front();
  if (useItem.GetRef() != 1) {
    return false;
  }
  if (useItem.IsUseByPhi() || useItem.GetStmt()->GetOp() == OP_asm || kOpcodeInfo.IsCall(useItem.GetStmt()->GetOp())) {
    return false;
  }
  return true;
}

static bool FoldableOp(Opcode op) {
  return op == OP_add || op == OP_sub || op == OP_mul || op == OP_bior || op == OP_band;
}

// we hope that prop the expr will not make the live-range more complicated
void CopyProp::CheckLiveRange(const MeExpr &expr, int32 &badPropCnt) {
  if (static_cast<uint32>(expr.GetDepth()) > kMaxDepth) {
    badPropCnt += static_cast<int32>(kMaxDepth);
    return;
  }
  switch (expr.GetMeOp()) {
    case kMeOpReg: {
      auto &scalar = static_cast<const ScalarMeExpr&>(expr);
      bool findLongerUse = false;
      auto *useSites = useInfo.GetUseSitesOfExpr(&scalar);
      CHECK_NULL_FATAL(useSites);
      for (auto &ui : *useSites) {
        const BB *useBB = ui.GetUseBB();
        if (dom.Dominate(*curBB, *useBB) || pdom.Dominate(*useBB, *curBB)) {
          findLongerUse = true;
          break;
        }
      }
      badPropCnt += (findLongerUse ? 0 : 1);
      return;
    }
    default: {
      for (size_t i = 0; i < expr.GetNumOpnds(); ++i) {
        CheckLiveRange(*expr.GetOpnd(i), badPropCnt);
      }
    }
  }
}

bool CopyProp::CanPropSingleUse(const ScalarMeExpr &lhs, const MeExpr &rhs) {
  if (!FoldableOp(rhs.GetOp()) && !rhs.IsScalar()) {
    return false;
  }
  if (!IsSingleUse(&lhs)) {
    return false;
  }
  BB *defBB = lhs.GetDefStmt()->GetBB();
  if (curBB == defBB) {
    return true;
  }
  if (loopInfo->GetBBLoopParent(curBB->GetBBId()) != loopInfo->GetBBLoopParent(defBB->GetBBId())) {
    return false;
  }
  int32 badPropCnt = 0;
  CheckLiveRange(rhs, badPropCnt);
  static const int32 allowedBPC = 2;
  return badPropCnt <= allowedBPC;
}

void CopyProp::ReplaceSelfAssign() {
  for (auto &useSitesPair : useInfo.GetUseSites()) {
    auto expr = useSitesPair.first;
    if (!IsSingleUse(expr)) {
      continue;
    }
    auto regMeExpr = static_cast<RegMeExpr *>(expr);
    auto *defStmt = regMeExpr->GetDefStmt();
    CHECK_NULL_FATAL(defStmt);
    auto rhs = defStmt->GetRHS();
    MeExpr* preVersionExpr = nullptr;
    if (!IsSimpleExprOfOst(*rhs, regMeExpr->GetOstIdx(), preVersionExpr)) {
      continue;
    }
    auto stmt = useSitesPair.second->front().GetStmt();
    if (irMap.ReplaceMeExprStmt(*stmt, *regMeExpr, *rhs)) {
      defStmt->GetBB()->RemoveMeStmt(defStmt);
      if (preVersionExpr) {
        useInfo.AddUseSiteOfExpr(preVersionExpr, stmt);
      }
    }
  }
}

MeExpr &CopyProp::PropMeExpr(MeExpr &meExpr, bool &isproped, bool atParm) {
  MeExprOp meOp = meExpr.GetMeOp();

  bool subProped = false;
  switch (meOp) {
    case kMeOpVar: {
      auto &varExpr = static_cast<VarMeExpr&>(meExpr);
      MeExpr *propMeExpr = &meExpr;
      MIRSymbol *symbol = varExpr.GetOst()->GetMIRSymbol();
      if (mirModule.IsCModule() && CanBeReplacedByConst(*symbol) && symbol->GetKonst() != nullptr) {
        propMeExpr = irMap.CreateConstMeExpr(varExpr.GetPrimType(), *symbol->GetKonst());
      } else {
        propMeExpr = &PropVar(varExpr, atParm, true);
      }
      if (propMeExpr != &varExpr) {
        isproped = true;
      }
      return *propMeExpr;
    }
    case kMeOpReg: {
      auto &regExpr = static_cast<RegMeExpr&>(meExpr);
      if (regExpr.GetRegIdx() < 0) {
        return meExpr;
      }
      MeExpr &propMeExpr = PropReg(regExpr, atParm, true);
      if (&propMeExpr != &regExpr) {
        isproped = true;
      }
      return propMeExpr;
    }
    case kMeOpIvar: {
      auto *ivarMeExpr = static_cast<IvarMeExpr*>(&meExpr);
      CHECK_NULL_FATAL(ivarMeExpr);
      if (ivarMeExpr->HasMultipleMu()) {
        return meExpr;
      }
      ASSERT(ivarMeExpr->GetUniqueMu() != nullptr, "PropMeExpr: ivar has mu == nullptr");
      auto *base = ivarMeExpr->GetBase();
      MeExpr *propedExpr = &PropMeExpr(utils::ToRef(base), subProped, false);
      if (propedExpr == base) {
        return meExpr;
      }

      if (!(base->GetMeOp() == kMeOpVar || base->GetMeOp() == kMeOpReg) || PropagatableByCopyProp(propedExpr)) {
        isproped = true;
        IvarMeExpr newMeExpr(&irMap.GetIRMapAlloc(), -1, *ivarMeExpr);
        newMeExpr.SetBase(propedExpr);
        newMeExpr.SetDefStmt(nullptr);
        ivarMeExpr = static_cast<IvarMeExpr*>(irMap.HashMeExpr(newMeExpr));
        auto *simplified = irMap.SimplifyIvar(ivarMeExpr, false);
        ivarMeExpr = (simplified != nullptr && simplified->GetMeOp() == kMeOpIvar) ?
            static_cast<IvarMeExpr*>(simplified) : ivarMeExpr;
      }
      CHECK_FATAL(ivarMeExpr != nullptr, "ivarMeExpr should not be nullptr");
      auto &propedIvar = PropIvar(*ivarMeExpr);
      if (propedIvar.IsScalar() && !useInfo.GetUseSitesOfExpr(&propedIvar)) {
        useInfo.AddUseSiteOfExpr(&propedIvar, static_cast<MeStmt *>(ivarMeExpr->GetDefStmt()));
      }
      if (PropagatableByCopyProp(&propedIvar)) {
        return propedIvar;
      }
      return *ivarMeExpr;
    }
    case kMeOpOp: {
      auto &meOpExpr = static_cast<OpMeExpr&>(meExpr);
      OpMeExpr newMeExpr(meOpExpr, -1);
      MeExpr *simplifyExpr = nullptr;

      for (size_t i = 0; i < newMeExpr.GetNumOpnds(); ++i) {
        auto opnd = meOpExpr.GetOpnd(i);
        auto &propedExpr = PropMeExpr(utils::ToRef(opnd), subProped, false);
        if ((opnd->GetMeOp() == kMeOpVar || opnd->GetMeOp() == kMeOpReg) &&
            !PropagatableOpndOfOperator(&propedExpr, meExpr.GetOp(), i) &&
            // force prop into foldableop if is single use
            (!FoldableOp(meExpr.GetOp()) || !CanPropSingleUse(static_cast<ScalarMeExpr&>(*opnd), propedExpr))) {
          if (!ExpectedPropedExpr(propedExpr)) {
            continue;
          }
          newMeExpr.SetOpnd(i, &propedExpr);
          simplifyExpr = irMap.SimplifyOpMeExpr(&newMeExpr);
          if (!simplifyExpr || simplifyExpr->GetDepth() >= newMeExpr.GetDepth()) {
            newMeExpr.SetOpnd(i, opnd);
            continue;
          }
        }

        newMeExpr.SetOpnd(i, &propedExpr);
        isproped = true;
      }
      simplifyExpr = irMap.SimplifyOpMeExpr(&newMeExpr);
      return simplifyExpr != nullptr ? *simplifyExpr : utils::ToRef(irMap.HashMeExpr(newMeExpr));
    }
    case kMeOpNary: {
      auto &naryMeExpr = static_cast<NaryMeExpr&>(meExpr);
      NaryMeExpr newMeExpr(&propMapAlloc, -1, naryMeExpr);

      bool needRehash = false;
      for (size_t i = 0; i < naryMeExpr.GetOpnds().size(); ++i) {
        if (i == 0 && naryMeExpr.GetOp() == OP_array && !config.propagateBase) {
          continue;
        }
        auto *opnd = naryMeExpr.GetOpnd(i);
        auto &propedExpr = PropMeExpr(utils::ToRef(opnd), subProped, false);
        if (&propedExpr != opnd) {
          if ((opnd->GetMeOp() == kMeOpVar || opnd->GetMeOp() == kMeOpReg) && !PropagatableByCopyProp(&propedExpr)) {
            continue;
          }
          newMeExpr.SetOpnd(i, &propedExpr);
          needRehash = true;
        }
      }

      if (needRehash) {
        isproped = true;
        return utils::ToRef(irMap.HashMeExpr(newMeExpr));
      } else {
        return naryMeExpr;
      }
    }
    default:
      return meExpr;
  }
}

void CopyProp::TraversalMeStmt(MeStmt &meStmt) {
  ++cntOfPropedStmt;
  if (cntOfPropedStmt > MeOption::copyPropLimit) {
    return;
  }
  Opcode op = meStmt.GetOp();

  bool subProped = false;
  // prop operand
  switch (op) {
    case OP_iassign: {
      auto &ivarStmt = static_cast<IassignMeStmt&>(meStmt);
      auto *rhs = ivarStmt.GetRHS();
      auto &propedRHS = PropMeExpr(utils::ToRef(rhs), subProped, false);
      if (rhs->GetMeOp() == kMeOpVar || rhs->GetMeOp() == kMeOpReg) {
        if (PropagatableByCopyProp(&propedRHS)) {
          ivarStmt.SetRHS(&propedRHS);
        }
      } else {
        ivarStmt.SetRHS(&propedRHS);
      }

      auto *lhs = ivarStmt.GetLHSVal();
      auto *baseOfIvar = lhs->GetBase();
      MeExpr *propedExpr = &PropMeExpr(utils::ToRef(baseOfIvar), subProped, false);
      if (propedExpr != baseOfIvar &&
          (PropagatableByCopyProp(propedExpr) ||
           (propedExpr->GetDepth() == baseOfIvar->GetDepth() && !propedExpr->IsLeaf()))) {
        ivarStmt.GetLHSVal()->SetBase(propedExpr);
        ivarStmt.SetLHSVal(irMap.BuildLHSIvarFromIassMeStmt(ivarStmt));
      }
      break;
    }
    case OP_dassign:
    case OP_regassign: {
      AssignMeStmt &assignStmt = static_cast<AssignMeStmt &>(meStmt);
      auto *rhs = assignStmt.GetRHS();
      auto &propedRHS = PropMeExpr(utils::ToRef(rhs), subProped, false);
      if (rhs->GetMeOp() == kMeOpVar || rhs->GetMeOp() == kMeOpReg) {
        if (PropagatableByCopyProp(&propedRHS) || CanPropSingleUse(static_cast<ScalarMeExpr&>(*rhs), propedRHS)) {
          assignStmt.SetRHS(&propedRHS);
        }
      } else {
        assignStmt.SetRHS(&propedRHS);
      }

      PropUpdateDef(*assignStmt.GetLHS());
      break;
    }
    case OP_asm: break;
    case OP_brtrue:
    case OP_brfalse: {
      PropConditionBranchStmt(&meStmt);
      break;
    }
    default:{
      for (size_t i = 0; i != meStmt.NumMeStmtOpnds(); ++i) {
        auto opnd = meStmt.GetOpnd(i);
        MeExpr &propedExpr = PropMeExpr(utils::ToRef(opnd), subProped, kOpcodeInfo.IsCall(op));
        if (opnd->GetMeOp() == kMeOpVar || opnd->GetMeOp() == kMeOpReg) {
          if (PropagatableByCopyProp(&propedExpr)) {
            meStmt.SetOpnd(i, &propedExpr);
          }
        } else {
          meStmt.SetOpnd(i, &propedExpr);
        }
      }
      break;
    }
  }

  // update chi
  auto *chiList = meStmt.GetChiList();
  if (chiList != nullptr) {
    switch (op) {
      case OP_syncenter:
      case OP_syncexit: {
        break;
      }
      default:
        PropUpdateChiListDef(*chiList);
        break;
    }
  }

  // update must def
  if (kOpcodeInfo.IsCallAssigned(op)) {
    MapleVector<MustDefMeNode> *mustDefList = meStmt.GetMustDefList();
    for (auto &node : utils::ToRef(mustDefList)) {
      MeExpr *meLhs = node.GetLHS();
      PropUpdateDef(utils::ToRef(static_cast<VarMeExpr*>(meLhs)));
    }
  }
}

void MECopyProp::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>();
  aDep.AddRequired<MEIRMapBuild>();
  aDep.AddRequired<MELoopAnalysis>();
  aDep.SetPreservedAll();
}

bool MECopyProp::PhaseRun(maple::MeFunction &f) {
  auto *dominancePhase = EXEC_ANALYSIS(MEDominance, f);
  auto dom = dominancePhase->GetDomResult();
  CHECK_NULL_FATAL(dom);
  auto pdom = dominancePhase->GetPdomResult();
  CHECK_NULL_FATAL(pdom);
  auto *hMap = GET_ANALYSIS(MEIRMapBuild, f);
  auto *loops = GET_ANALYSIS(MELoopAnalysis, f);

  auto propConfig = Prop::PropConfig {
      MeOption::propBase, true, MeOption::propGlobalRef, MeOption::propFinaliLoadRef, MeOption::propIloadRefNonParm,
      MeOption::propAtPhi, MeOption::propWithInverse || f.IsLfo()
  };
  auto &useInfo = hMap->GetExprUseInfo();
  if (useInfo.IsInvalid()) {
    useInfo.CollectUseInfoInFunc(hMap, dom, kUseInfoOfScalar);
  }
  CopyProp copyProp(&f, useInfo, loops, *hMap, *dom, *pdom, *ApplyTempMemPool(), f.GetCfg()->NumBBs(), propConfig);
  copyProp.ReplaceSelfAssign();
  copyProp.TraversalBB(*f.GetCfg()->GetCommonEntryBB());
  useInfo.InvalidUseInfo();
  if (DEBUGFUNC_NEWPM(f)) {
    LogInfo::MapleLogger() << "\n============== After Copy Propagation  =============" << '\n';
    f.Dump(false);
    f.GetCfg()->DumpToFile("afterCopyProp-");
  }
  return false;
}
} // namespace maple
