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
#include "prop.h"
#include "me_irmap.h"
#include "dominance.h"

using namespace maple;

const int kPropTreeLevel = 15;  // tree height threshold to increase to

namespace maple {
Prop::Prop(IRMap &irMap, Dominance &dom, MemPool &memPool, std::vector<BB*> &&bbVec, BB &commonEntryBB,
           const PropConfig &config)
    : dom(dom),
      irMap(irMap),
      ssaTab(irMap.GetSSATab()),
      mirModule(irMap.GetSSATab().GetModule()),
      propMapAlloc(&memPool),
      bbVec(bbVec),
      commonEntryBB(commonEntryBB),
      vstLiveStackVec(),
      bbVisited(bbVec.size(), false),
      config(config) {
  const MapleVector<OriginalSt*> &originalStVec = ssaTab.GetOriginalStTable().GetOriginalStVector();
  vstLiveStackVec.resize(originalStVec.size());
  for (size_t i = 1; i < originalStVec.size(); ++i) {
    OriginalSt *ost = originalStVec[i];
    ASSERT(ost->GetIndex() == i, "inconsistent originalst_table index");
    MeExpr *expr = irMap.GetMeExpr(ost->GetZeroVersionIndex());
    if (expr != nullptr) {
      vstLiveStackVec[i].push(expr);
    }
  }
}

void Prop::PropUpdateDef(MeExpr &meExpr) {
  ASSERT(meExpr.GetMeOp() == kMeOpVar || meExpr.GetMeOp() == kMeOpReg, "meExpr error");
  OStIdx ostIdx;
  if (meExpr.GetMeOp() == kMeOpVar) {
    ostIdx = static_cast<VarMeExpr&>(meExpr).GetOStIdx();
  } else {
    auto &regExpr = static_cast<RegMeExpr&>(meExpr);
    if (!regExpr.IsNormalReg()) {
      return;
    }
    ostIdx = regExpr.GetOstIdx();
  }
  vstLiveStackVec.at(ostIdx).push(meExpr);
}

void Prop::PropUpdateChiListDef(const MapleMap<OStIdx, ChiMeNode*> &chiList) {
  for (auto it = chiList.begin(); it != chiList.end(); ++it) {
    PropUpdateDef(*static_cast<VarMeExpr*>(it->second->GetLHS()));
  }
}

void Prop::CollectSubVarMeExpr(const MeExpr &meExpr, std::vector<const MeExpr*> &varVec) const {
  switch (meExpr.GetMeOp()) {
    case kMeOpReg:
    case kMeOpVar:
      varVec.push_back(&meExpr);
      break;
    case kMeOpIvar: {
      auto &ivarMeExpr = static_cast<const IvarMeExpr&>(meExpr);
      if (ivarMeExpr.GetMu() != nullptr) {
        varVec.push_back(ivarMeExpr.GetMu());
      }
      break;
    }
    default:
      break;
  }
}

// check at the current statement, if the version symbol is consistent with its definition in the top of the stack
// for example:
// x1 <- a1 + b1;
// a2 <-
//  <-x1
// the version of progation of x1 is a1, but the top of the stack of symbol a is a2, so it's not consistent
// warning: I suppose the vector vervec is on the stack, otherwise would cause memory leak
bool Prop::IsVersionConsistent(const std::vector<const MeExpr*> &vstVec,
                               const std::vector<std::stack<SafeMeExprPtr>> &vstLiveStack) const {
  for (auto it = vstVec.begin(); it != vstVec.end(); ++it) {
    // iterate each cur defintion of related symbols of rhs, check the version
    const MeExpr *subExpr = *it;
    CHECK_FATAL(subExpr->GetMeOp() == kMeOpVar || subExpr->GetMeOp() == kMeOpReg, "error: sub expr error");
    uint32 stackIdx = 0;
    if (subExpr->GetMeOp() == kMeOpVar) {
      stackIdx = static_cast<const VarMeExpr*>(subExpr)->GetOStIdx();
    } else {
      stackIdx = static_cast<const RegMeExpr*>(subExpr)->GetOstIdx();
    }
    auto &pStack = vstLiveStack.at(stackIdx);
    if (pStack.empty()) {
      // no definition so far go ahead
      continue;
    }
    SafeMeExprPtr curDef = pStack.top();
    CHECK_FATAL(curDef->GetMeOp() == kMeOpVar || curDef->GetMeOp() == kMeOpReg, "error: cur def error");
    if (subExpr != curDef.get()) {
      return false;
    }
  }
  return true;
}

bool Prop::IvarIsFinalField(const IvarMeExpr &ivarMeExpr) const {
  if (!config.propagateFinalIloadRef) {
    return false;
  }
  if (ivarMeExpr.GetFieldID() == 0) {
    return false;
  }
  MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ivarMeExpr.GetTyIdx());
  ASSERT(ty->GetKind() == kTypePointer, "IvarIsFinalField: pointer type expected");
  MIRType *pointedType = static_cast<MIRPtrType*>(ty)->GetPointedType();
  auto *structType = static_cast<MIRStructType*>(pointedType);
  FieldID fieldID = ivarMeExpr.GetFieldID();
  return structType->IsFieldFinal(fieldID) && !structType->IsFieldRCUnownedRef(fieldID);
}

// check if the expression x can be legally forward-substitute the variable that
// it was assigned to; x is from bb
bool Prop::Propagatable(const MeExpr &expr, const BB &fromBB, bool atParm) const {
  MeExprOp meOp = expr.GetMeOp();
  switch (meOp) {
    case kMeOpGcmalloc:
      return false;
    case kMeOpNary: {
      if (expr.GetOp() == OP_intrinsicop || expr.GetOp() == OP_intrinsicopwithtype) {
        return false;
      }
      break;
    }
    case kMeOpReg: {
      auto &regRead = static_cast<const RegMeExpr&>(expr);
      if (regRead.GetRegIdx() < 0) {
        return false;
      }
      // get the current definition version
      std::vector<const MeExpr*> regReadVec;
      CollectSubVarMeExpr(expr, regReadVec);
      if (!IsVersionConsistent(regReadVec, vstLiveStackVec)) {
        return false;
      }
      break;
    }
    case kMeOpVar: {
      auto &varMeExpr = static_cast<const VarMeExpr&>(expr);
      if (varMeExpr.IsVolatile(ssaTab)) {
        return false;
      }
      const MIRSymbol *st = ssaTab.GetMIRSymbolFromID(varMeExpr.GetOStIdx());
      if (!config.propagateGlobalRef && st->IsGlobal() && !st->IsFinal() && !st->IgnoreRC()) {
        return false;
      }
      if (LocalToDifferentPU(st->GetStIdx(), fromBB)) {
        return false;
      }
      // get the current definition version
      std::vector<const MeExpr*> varMeExprVec;
      CollectSubVarMeExpr(expr, varMeExprVec);
      if (!IsVersionConsistent(varMeExprVec, vstLiveStackVec)) {
        return false;
      }
      break;
    }
    case kMeOpIvar: {
      auto &ivarMeExpr = static_cast<const IvarMeExpr&>(expr);
      if (!IvarIsFinalField(ivarMeExpr) &&
          !GetTypeFromTyIdx(ivarMeExpr.GetTyIdx()).PointsToConstString()) {
        if ((!config.propagateIloadRef || (config.propagateIloadRefNonParm && atParm)) &&
            ivarMeExpr.GetPrimType() == PTY_ref) {
          return false;
        }
      }
      ASSERT_NOT_NULL(curBB);
      if (fromBB.GetAttributes(kBBAttrIsTry) && !curBB->GetAttributes(kBBAttrIsTry)) {
        return false;
      }
      if (ivarMeExpr.IsVolatile() || ivarMeExpr.IsRCWeak()) {
        return false;
      }
      // get the current definition version
      std::vector<const MeExpr*> varMeExprVec;
      CollectSubVarMeExpr(expr, varMeExprVec);
      if (!IsVersionConsistent(varMeExprVec, vstLiveStackVec)) {
        return false;
      }
      break;
    }
    case kMeOpOp: {
      if (kOpcodeInfo.NotPure(expr.GetOp())) {
        return false;
      }
      break;
    }
    default:
      break;
  }

  for (size_t i = 0; i < expr.GetNumOpnds(); ++i) {
    if (!Propagatable(utils::ToRef(expr.GetOpnd(i)), fromBB, false)) {
      return false;
    }
  }

  return true;
}

// return varMeExpr itself if no propagation opportunity
MeExpr &Prop::PropVar(VarMeExpr &varMeExpr, bool atParm, bool checkPhi) const {
  const MIRSymbol *st = ssaTab.GetMIRSymbolFromID(varMeExpr.GetOStIdx());
  if (st->IsInstrumented() || varMeExpr.IsVolatile(ssaTab)) {
    return varMeExpr;
  }

  if (varMeExpr.GetDefBy() == kDefByStmt) {
    DassignMeStmt *defStmt = static_cast<DassignMeStmt*>(varMeExpr.GetDefStmt());
    ASSERT(defStmt != nullptr, "dynamic cast result is nullptr");
    MeExpr *rhs = defStmt->GetRHS();
    if (rhs->GetDepth() <= kPropTreeLevel &&
        Propagatable(utils::ToRef(rhs), utils::ToRef(defStmt->GetBB()), atParm)) {
      // mark propagated for iread ref
      if (rhs->GetMeOp() == kMeOpIvar && rhs->GetPrimType() == PTY_ref) {
        defStmt->SetPropagated(true);
      }
      return utils::ToRef(rhs);
    } else {
      return varMeExpr;
    }
  } else if (checkPhi && varMeExpr.GetDefBy() == kDefByPhi && config.propagateAtPhi) {
    MePhiNode &defPhi = varMeExpr.GetDefPhi();
    VarMeExpr* phiOpndLast = static_cast<VarMeExpr*>(defPhi.GetOpnds().back());
    MeExpr *opndLastProp = &PropVar(utils::ToRef(phiOpndLast), atParm, false);
    if (opndLastProp != &varMeExpr && opndLastProp != phiOpndLast && opndLastProp->GetMeOp() == kMeOpVar) {
      // one more call
      opndLastProp = &PropVar(static_cast<VarMeExpr&>(*opndLastProp), atParm, false);
    }
    if (opndLastProp == &varMeExpr) {
      return varMeExpr;
    }
    MapleVector<ScalarMeExpr *> opndsVec = defPhi.GetOpnds();
    for (auto it = opndsVec.rbegin() + 1; it != opndsVec.rend(); ++it) {
      VarMeExpr *phiOpnd = static_cast<VarMeExpr*>(*it);
      MeExpr &opndProp = PropVar(utils::ToRef(phiOpnd), atParm, false);
      if (&opndProp != opndLastProp) {
        return varMeExpr;
      }
    }
    return *opndLastProp;
  }
  return varMeExpr;
}

MeExpr &Prop::PropReg(RegMeExpr &regMeExpr, bool atParm) const {
  if (regMeExpr.GetDefBy() == kDefByStmt) {
    RegassignMeStmt *defStmt = static_cast<RegassignMeStmt*>(regMeExpr.GetDefStmt());
    MeExpr &rhs = utils::ToRef(defStmt->GetRHS());
    if (rhs.GetDepth() <= kPropTreeLevel && Propagatable(rhs, utils::ToRef(defStmt->GetBB()), atParm)) {
      return rhs;
    }
  }
  return regMeExpr;
}

MeExpr &Prop::PropIvar(IvarMeExpr &ivarMeExpr) const {
  IassignMeStmt *defStmt = ivarMeExpr.GetDefStmt();
  if (defStmt == nullptr || ivarMeExpr.IsVolatile()) {
    return ivarMeExpr;
  }
  MeExpr &rhs = utils::ToRef(defStmt->GetRHS());
  if (rhs.GetDepth() <= kPropTreeLevel && Propagatable(rhs, utils::ToRef(defStmt->GetBB()), false)) {
    return rhs;
  }
  return ivarMeExpr;
}

MeExpr &Prop::PropMeExpr(MeExpr &meExpr, bool &isProped, bool atParm) {
  MeExprOp meOp = meExpr.GetMeOp();

  bool subProped = false;
  switch (meOp) {
    case kMeOpVar: {
      auto &varExpr = static_cast<VarMeExpr&>(meExpr);
      MeExpr &propMeExpr = PropVar(varExpr, atParm, true);
      if (&propMeExpr != &varExpr) {
        isProped = true;
      }
      return propMeExpr;
    }
    case kMeOpReg: {
      auto &regExpr = static_cast<RegMeExpr&>(meExpr);
      if (regExpr.GetRegIdx() < 0) {
        return meExpr;
      }
      MeExpr &propMeExpr = PropReg(regExpr, atParm);
      if (&propMeExpr != &regExpr) {
        isProped = true;
      }
      return propMeExpr;
    }
    case kMeOpIvar: {
      auto *ivarMeExpr = static_cast<IvarMeExpr*>(&meExpr);
      ASSERT(ivarMeExpr->GetMu() != nullptr, "PropMeExpr: ivar has mu == nullptr");
      bool baseProped = false;
      MeExpr *base = nullptr;
      if (ivarMeExpr->GetBase()->GetMeOp() != kMeOpVar || config.propagateBase) {
        base = &PropMeExpr(utils::ToRef(ivarMeExpr->GetBase()), baseProped, false);
      }
      if (baseProped) {
        isProped = true;
        IvarMeExpr newMeExpr(-1, *ivarMeExpr);
        newMeExpr.SetBase(base);
        newMeExpr.SetDefStmt(nullptr);
        ivarMeExpr = static_cast<IvarMeExpr*>(irMap.HashMeExpr(newMeExpr));
      }
      MeExpr &propIvarExpr = PropIvar(utils::ToRef(ivarMeExpr));
      if (&propIvarExpr != ivarMeExpr) {
        isProped = true;
      }
      return propIvarExpr;
    }
    case kMeOpOp: {
      auto &meOpExpr = static_cast<OpMeExpr&>(meExpr);
      OpMeExpr newMeExpr(-1, meOpExpr.GetOp(), meOpExpr.GetPrimType(), meOpExpr.GetNumOpnds());

      for (size_t i = 0; i < newMeExpr.GetNumOpnds(); ++i) {
        newMeExpr.SetOpnd(i, &PropMeExpr(utils::ToRef(meOpExpr.GetOpnd(i)), subProped, false));
      }

      if (subProped) {
        isProped = true;
        newMeExpr.SetOpndType(meOpExpr.GetOpndType());
        newMeExpr.SetBitsOffSet(meOpExpr.GetBitsOffSet());
        newMeExpr.SetBitsSize(meOpExpr.GetBitsSize());
        newMeExpr.SetTyIdx(meOpExpr.GetTyIdx());
        newMeExpr.SetFieldID(meOpExpr.GetFieldID());
        MeExpr *simplifyExpr = irMap.SimplifyOpMeExpr(&newMeExpr);
        return simplifyExpr != nullptr ? *simplifyExpr : utils::ToRef(irMap.HashMeExpr(newMeExpr));
      } else {
        return meOpExpr;
      }
    }
    case kMeOpNary: {
      auto &naryMeExpr = static_cast<NaryMeExpr&>(meExpr);
      NaryMeExpr newMeExpr(&propMapAlloc, -1, naryMeExpr);

      for (size_t i = 0; i < naryMeExpr.GetOpnds().size(); ++i) {
        if (i == 0 && naryMeExpr.GetOp() == OP_array && !config.propagateBase) {
          continue;
        }
        newMeExpr.SetOpnd(i, &PropMeExpr(utils::ToRef(naryMeExpr.GetOpnd(i)), subProped, false));
      }

      if (subProped) {
        isProped = true;
        return utils::ToRef(irMap.HashMeExpr(newMeExpr));
      } else {
        return naryMeExpr;
      }
    }
    default:
      return meExpr;
  }
}

void Prop::TraversalMeStmt(MeStmt &meStmt) {
  Opcode op = meStmt.GetOp();

  bool subProped = false;
  // prop operand
  switch (op) {
    case OP_iassign: {
      auto &ivarStmt = static_cast<IassignMeStmt&>(meStmt);
      ivarStmt.SetRHS(&PropMeExpr(utils::ToRef(ivarStmt.GetRHS()), subProped, false));
      if (ivarStmt.GetLHSVal()->GetBase()->GetMeOp() != kMeOpVar || config.propagateBase) {
        MeExpr *propedExpr = &PropMeExpr(utils::ToRef(ivarStmt.GetLHSVal()->GetBase()), subProped, false);
        if (propedExpr->GetOp() == OP_constval) {
          subProped = false;
        } else {
          ivarStmt.GetLHSVal()->SetBase(propedExpr);
        }
      }
      if (subProped) {
        ivarStmt.SetLHSVal(irMap.BuildLHSIvarFromIassMeStmt(ivarStmt));
      }
      break;
    }
    case OP_return: {
      auto &retMeStmt = static_cast<RetMeStmt&>(meStmt);
      const MapleVector<MeExpr*> &opnds = retMeStmt.GetOpnds();
      for (size_t i = 0; i < opnds.size(); ++i) {
        MeExpr *opnd = opnds[i];
        auto &propedExpr = PropMeExpr(utils::ToRef(opnd), subProped, false);
        if (propedExpr.GetMeOp() == kMeOpVar) {
          retMeStmt.SetOpnd(i, &propedExpr);
        }
      }
      break;
    }
    default:
      for (size_t i = 0; i != meStmt.NumMeStmtOpnds(); ++i) {
        MeExpr &expr = PropMeExpr(utils::ToRef(meStmt.GetOpnd(i)), subProped, kOpcodeInfo.IsCall(op));
        meStmt.SetOpnd(i, &expr);
      }
      break;
  }

  // update lhs
  switch (op) {
    case OP_dassign: {
      auto &varMeStmt = static_cast<DassignMeStmt&>(meStmt);
      PropUpdateDef(static_cast<VarMeExpr&>(utils::ToRef(varMeStmt.GetLHS())));
      break;
    }
    case OP_regassign: {
      auto &regMeStmt = static_cast<RegassignMeStmt&>(meStmt);
      PropUpdateDef(static_cast<RegMeExpr&>(utils::ToRef(regMeStmt.GetRegLHS())));
      break;
    }
    default:
      break;
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
      CHECK_FATAL(meLhs->GetMeOp() == kMeOpVar, "error: lhs is not var");
      PropUpdateDef(utils::ToRef(static_cast<VarMeExpr*>(meLhs)));
    }
  }
}

void Prop::TraversalBB(BB &bb) {
  if (bbVisited[bb.GetBBId()]) {
    return;
  }
  bbVisited[bb.GetBBId()] = true;
  curBB = &bb;

  // record stack size for variable versions before processing rename. It is used for stack pop up.
  std::vector<size_t> curStackSizeVec(vstLiveStackVec.size());
  for (size_t i = 1; i < vstLiveStackVec.size(); ++i) {
    curStackSizeVec[i] = vstLiveStackVec[i].size();
  }

  // update var phi nodes
  for (auto it = bb.GetMePhiList().begin(); it != bb.GetMePhiList().end(); ++it) {
    PropUpdateDef(utils::ToRef(it->second->GetLHS()));
  }

  // traversal on stmt
  for (auto &meStmt : bb.GetMeStmts()) {
    TraversalMeStmt(meStmt);
  }

  auto &domChildren = dom.GetDomChildren(bb.GetBBId());
  for (auto it = domChildren.begin(); it != domChildren.end(); ++it) {
    TraversalBB(utils::ToRef(bbVec[*it]));
  }

  for (size_t i = 1; i < vstLiveStackVec.size(); ++i) {
    auto &liveStack = vstLiveStackVec[i];
    size_t curSize = curStackSizeVec[i];
    while (liveStack.size() > curSize) {
      liveStack.pop();
    }
  }
}

void Prop::DoProp() {
  TraversalBB(commonEntryBB);
}
}  // namespace maple
