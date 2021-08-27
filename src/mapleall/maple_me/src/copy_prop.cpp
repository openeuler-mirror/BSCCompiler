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
static bool PropagatableByCopyProp(const MeExpr *newExpr) {
  return newExpr->GetMeOp() == kMeOpReg || newExpr->GetMeOp() == kMeOpConst;
}

static bool PropagatableBaseOfIvar(const IvarMeExpr *ivar, const MeExpr *newExpr) {
  if (PropagatableByCopyProp(newExpr)) {
    return true;
  }
  if ((ivar->GetFieldID() != 0 && ivar->GetFieldID() != 1) || ivar->GetOffset() != 0) {
    return false;
  }

#if TARGX86_64 || TARGX86 || TARGVM || TARGARM32
  return false;
#endif

  if (newExpr->GetOp() == OP_add) {
    auto opndA = newExpr->GetOpnd(0);
    if (!PropagatableByCopyProp(opndA)) {
      return false;
    }

    auto opndB = newExpr->GetOpnd(1);
    if (PropagatableByCopyProp(opndB)) {
      return true;
    }

    if (opndB->GetOp() == OP_cvt || opndB->GetOp() == OP_retype) {
      if (PropagatableByCopyProp(opndB->GetOpnd(0))) {
        return true;
      }
    } else if (opndB->GetOp() == OP_mul) {
      auto opndC = opndB->GetOpnd(0);
      if (!PropagatableByCopyProp(opndC)) {
        return false;
      }

      auto opndD = opndB->GetOpnd(1);
      if (opndD->GetMeOp() != kMeOpConst) {
        return false;
      }
      auto constVal = static_cast<ConstMeExpr *>(opndD)->GetConstVal();
      if (constVal->GetKind() != kConstInt) {
        return false;
      }
      int64 val = static_cast<MIRIntConst*>(constVal)->GetValue();
      if (val == 0 || val == GetPrimTypeSize(PTY_ptr)) {
        return true;
      }
    }
  }
  return false;
}

static bool PropagatableOpndOfOperator(const MeExpr *meExpr, Opcode op, uint32 opndId) {
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
      MeExpr &propMeExpr = PropReg(regExpr, atParm);
      if (&propMeExpr != &regExpr) {
        isproped = true;
      }
      return propMeExpr;
    }
    case kMeOpIvar: {
      auto *ivarMeExpr = static_cast<IvarMeExpr*>(&meExpr);
      ASSERT(ivarMeExpr->GetMu() != nullptr, "PropMeExpr: ivar has mu == nullptr");
      auto *base = ivarMeExpr->GetBase();
      MeExpr *propedExpr = &PropMeExpr(utils::ToRef(base), subProped, false);
      if (propedExpr == base) {
        return meExpr;
      }

      if ((base->GetMeOp() == kMeOpVar || base->GetMeOp() == kMeOpReg) &&
          !PropagatableBaseOfIvar(ivarMeExpr, propedExpr)) {
        return meExpr;
      }

      isproped = true;
      IvarMeExpr newMeExpr(-1, *ivarMeExpr);
      newMeExpr.SetBase(propedExpr);
      newMeExpr.SetDefStmt(nullptr);
      ivarMeExpr = static_cast<IvarMeExpr*>(irMap.HashMeExpr(newMeExpr));

      auto &propedIvar = PropIvar(*ivarMeExpr);
      if (PropagatableByCopyProp(&propedIvar)) {
        return propedIvar;
      }
      return *ivarMeExpr;
    }
    case kMeOpOp: {
      auto &meOpExpr = static_cast<OpMeExpr&>(meExpr);
      OpMeExpr newMeExpr(meOpExpr, -1);

      for (size_t i = 0; i < newMeExpr.GetNumOpnds(); ++i) {
        auto opnd = meOpExpr.GetOpnd(i);
        auto &propedExpr = PropMeExpr(utils::ToRef(opnd), subProped, false);
        if ((opnd->GetMeOp() == kMeOpVar || opnd->GetMeOp() == kMeOpReg) &&
            !PropagatableOpndOfOperator(&propedExpr, meExpr.GetOp(), i)) {
          continue;
        }

        newMeExpr.SetOpnd(i, &propedExpr);
        isproped = true;
      }
      MeExpr *simplifyExpr = irMap.SimplifyOpMeExpr(&newMeExpr);
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
        if (&propedExpr != &meExpr) {
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
      if (propedExpr != baseOfIvar && PropagatableBaseOfIvar(lhs, propedExpr)) {
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
        if (PropagatableByCopyProp(&propedRHS)) {
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
  aDep.SetPreservedAll();
}

bool MECopyProp::PhaseRun(maple::MeFunction &f) {
  auto *dom = GET_ANALYSIS(MEDominance, f);
  auto *hMap = GET_ANALYSIS(MEIRMapBuild, f);

  CopyProp copyProp(&f, *hMap, *dom, *GetPhaseMemPool(), f.GetCfg()->NumBBs(),
      Prop::PropConfig { MeOption::propBase, true, MeOption::propGlobalRef, MeOption::propFinaliLoadRef,
      MeOption::propIloadRefNonParm, MeOption::propAtPhi, MeOption::propWithInverse || f.IsLfo() });
  copyProp.TraversalBB(*f.GetCfg()->GetCommonEntryBB());
  if (DEBUGFUNC_NEWPM(f)) {
    LogInfo::MapleLogger() << "\n============== After Copy Propagation  =============" << '\n';
    f.Dump(false);
    f.GetCfg()->DumpToFile("afterCopyProp-");
  }
  return false;
}
} // namespace maple
