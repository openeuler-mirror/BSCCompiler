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
#include "call_graph.h"
#include "maple_phase.h"
#include "maple_phase.h"
#include <iostream>
#include <fstream>
#include <queue>
#include <unordered_set>
#include <algorithm>
#include "option.h"
#include "string_utils.h"
#include "mir_function.h"
#include "prop_return_null.h"
#include "me_dominance.h"

namespace maple {
static bool MaybeNull(const MeExpr &expr) {
  if (expr.GetMeOp() == kMeOpVar) {
    return static_cast<const VarMeExpr*>(&expr)->GetMaybeNull();
  } else if (expr.GetMeOp() == kMeOpIvar) {
    return static_cast<const IvarMeExpr*>(&expr)->GetMaybeNull();
  } else if (expr.GetOp() == OP_retype) {
    MeExpr *retypeRHS = (static_cast<const OpMeExpr*>(&expr))->GetOpnd(0);
    return MaybeNull(*retypeRHS);
  }
  return true;
}

TyIdx PropReturnAttr::GetInferredTyIdx(MeExpr &expr) const {
  if (expr.GetMeOp() == kMeOpVar) {
    auto *varMeExpr = static_cast<VarMeExpr*>(&expr);
    if (varMeExpr->GetInferredTyIdx() == 0u) {
      // If varMeExpr->inferredTyIdx has not been set, we can double check
      // if it is coming from a static final field
      const OriginalSt *ost = varMeExpr->GetOst();
      const MIRSymbol *mirSym = ost->GetMIRSymbol();
      if (mirSym->IsStatic() && mirSym->IsFinal() && mirSym->GetInferredTyIdx() != kInitTyIdx &&
          mirSym->GetInferredTyIdx() != kNoneTyIdx) {
        varMeExpr->SetInferredTyIdx(mirSym->GetInferredTyIdx());
      }
      if (mirSym->GetType()->GetKind() == kTypePointer) {
        MIRType *pointedType = (static_cast<MIRPtrType*>(mirSym->GetType()))->GetPointedType();
        if (pointedType->GetKind() == kTypeClass) {
          if ((static_cast<MIRClassType*>(pointedType))->IsFinal()) {
            varMeExpr->SetInferredTyIdx(pointedType->GetTypeIndex());
          }
        }
      }
    }
    return varMeExpr->GetInferredTyIdx();
  } else if (expr.GetMeOp() == kMeOpIvar) {
    return static_cast<IvarMeExpr*>(&expr)->GetInferredTyIdx();
  } else if (expr.GetOp() == OP_retype) {
    MeExpr *retypeRHS = (static_cast<OpMeExpr*>(&expr))->GetOpnd(0);
    return GetInferredTyIdx(*retypeRHS);
  }
  return TyIdx(0);
}

void PropReturnAttr::PropVarInferredType(VarMeExpr &varMeExpr) const {
  if (varMeExpr.GetDefBy() == kDefByStmt) {
    DassignMeStmt &defStmt = utils::ToRef(safe_cast<DassignMeStmt>(varMeExpr.GetDefStmt()));
    MeExpr *rhs = defStmt.GetRHS();
    if (rhs->GetOp() == OP_gcmalloc) {
      varMeExpr.SetInferredTyIdx(static_cast<GcmallocMeExpr*>(rhs)->GetTyIdx());
      varMeExpr.SetMaybeNull(false);
      if (PropReturnAttr::debug) {
        MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(varMeExpr.GetInferredTyIdx());
        LogInfo::MapleLogger() << "[PROP-RETURN-ATTR] [TYPE-INFERRING] mx" << varMeExpr.GetExprID() << " ";
        type->Dump(0, false);
        LogInfo::MapleLogger() << '\n';
      }
    } else if (rhs->GetOp() == OP_gcmallocjarray) {
      varMeExpr.SetInferredTyIdx(static_cast<OpMeExpr*>(rhs)->GetTyIdx());
      varMeExpr.SetMaybeNull(false);
      if (PropReturnAttr::debug) {
        MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(varMeExpr.GetInferredTyIdx());
        LogInfo::MapleLogger() << "[PROP-RETURN-ATTR] [TYPE-INFERRING] mx" << varMeExpr.GetExprID() << " ";
        type->Dump(0, false);
        LogInfo::MapleLogger() << '\n';
      }
    } else if (!MaybeNull(*rhs)) {
      varMeExpr.SetMaybeNull(false);
    } else {
      TyIdx tyIdx = GetInferredTyIdx(*rhs);
      if (tyIdx != 0u) {
        varMeExpr.SetInferredTyIdx(tyIdx);
        if (PropReturnAttr::debug) {
          MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(varMeExpr.GetInferredTyIdx());
          LogInfo::MapleLogger() << "[PROP-RETURN-ATTR] [TYPE-INFERRING] mx" << varMeExpr.GetExprID() << " ";
          type->Dump(0, false);
          LogInfo::MapleLogger() << '\n';
        }
      }
    }
  } else if (varMeExpr.GetDefBy() == kDefByPhi) {
    if (PropReturnAttr::debug) {
      LogInfo::MapleLogger() << "[PROP-RETURN-ATTR] [TYPE-INFERRING] " << "Def by phi " << '\n';
    }
  }
}

void PropReturnAttr::PropIvarInferredType(IvarMeExpr &ivar) const {
  IassignMeStmt *defStmt = ivar.GetDefStmt();
  if (defStmt == nullptr) {
    return;
  }
  MeExpr *rhs = defStmt->GetRHS();
  CHECK_NULL_FATAL(rhs);
  if (rhs->GetOp() == OP_gcmalloc) {
    ivar.GetInferredTyIdx() = static_cast<GcmallocMeExpr*>(rhs)->GetTyIdx();
    ivar.SetMaybeNull(false);
    if (PropReturnAttr::debug) {
      MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ivar.GetInferredTyIdx());
      LogInfo::MapleLogger() << "[PROP-RETURN-ATTR] [TYPE-INFERRING] mx" << ivar.GetExprID() << " ";
      type->Dump(0, false);
      LogInfo::MapleLogger() << '\n';
    }
  } else if (rhs->GetOp() == OP_gcmallocjarray) {
    ivar.GetInferredTyIdx() = static_cast<OpMeExpr*>(rhs)->GetTyIdx();
    ivar.SetMaybeNull(false);
  } else if (!MaybeNull(*rhs)) {
    ivar.SetMaybeNull(false);
  } else {
    TyIdx tyIdx = GetInferredTyIdx(*rhs);
    if (tyIdx != 0u) {
      ivar.SetInferredTyidx(tyIdx);
      if (PropReturnAttr::debug) {
        MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ivar.GetInferredTyIdx());
        LogInfo::MapleLogger() << "[PROP-RETURN-ATTR] [TYPE-INFERRING] mx" << ivar.GetExprID() << " ";
        type->Dump(0, false);
        LogInfo::MapleLogger() << '\n';
      }
    }
  }
}

void PropReturnAttr::VisitVarPhiNode(MePhiNode &varPhi) {
  MapleVector<ScalarMeExpr*> opnds = varPhi.GetOpnds();
  auto *lhs = varPhi.GetLHS();
  // RegPhiNode cases NYI
  if (lhs == nullptr || lhs->GetMeOp() != kMeOpVar) {
    return;
  }
  VarMeExpr *lhsVar = static_cast<VarMeExpr*>(varPhi.GetLHS());
  for (size_t i = 0; i < opnds.size(); ++i) {
    VarMeExpr *opnd = static_cast<VarMeExpr *>(opnds[i]);
    PropVarInferredType(*opnd);
    if (MaybeNull(*opnd)) {
      return;
    }
  }
  lhsVar->SetMaybeNull(false);
  return;
}

void PropReturnAttr::VisitMeExpr(MeExpr *meExpr) const {
  if (meExpr == nullptr) {
    return;
  }
  MeExprOp meOp = meExpr->GetMeOp();
  switch (meOp) {
    case kMeOpVar: {
      auto *varExpr = static_cast<VarMeExpr*>(meExpr);
      PropVarInferredType(*varExpr);
      break;
    }
    case kMeOpReg:
      break;
    case kMeOpIvar: {
      auto *iVar = static_cast<IvarMeExpr*>(meExpr);
      PropIvarInferredType(*iVar);
      break;
    }
    case kMeOpOp: {
      auto *meOpExpr = static_cast<OpMeExpr*>(meExpr);
      for (uint32 i = 0; i < kOperandNumTernary; ++i) {
        VisitMeExpr(meOpExpr->GetOpnd(i));
      }
      break;
    }
    case kMeOpNary: {
      auto *naryMeExpr = static_cast<NaryMeExpr*>(meExpr);
      for (MeExpr *opnd : naryMeExpr->GetOpnds()) {
        VisitMeExpr(opnd);
      }
      break;
    }
    case kMeOpAddrof:
    case kMeOpAddroffunc:
    case kMeOpAddroflabel:
    case kMeOpGcmalloc:
    case kMeOpConst:
    case kMeOpConststr:
    case kMeOpConststr16:
    case kMeOpSizeoftype:
    case kMeOpFieldsDist:
      break;
    default:
      CHECK_FATAL(false, "MeOP NIY");
      break;
  }
}

void PropReturnAttr::ReturnTyIdxInferring(const RetMeStmt &retMeStmt) {
  const MapleVector<MeExpr*> &opnds = retMeStmt.GetOpnds();
  CHECK_FATAL(opnds.size() <= 1, "Assume at most one return value for now");
  for (size_t i = 0; i < opnds.size(); ++i) {
    MeExpr *opnd = opnds[i];
    TyIdx tyIdx = GetInferredTyIdx(*opnd);
    if (retTy == kNotSeen) {
      // seen the first return stmt
      retTy = kSeen;
      inferredRetTyIdx = tyIdx;
      if (!MaybeNull(*opnd)) {
        maybeNull = false;
      }
    } else if (retTy == kSeen) {
      // has seen an nonull before, check if they agreed
      if (inferredRetTyIdx != tyIdx) {
        retTy = kFailed;
        inferredRetTyIdx = TyIdx(0);  // not agreed, cleared.
      }
      if (MaybeNull(*opnd) || maybeNull) {
        maybeNull = true;  // not agreed, cleared.
      }
    }
  }
}

void PropReturnAttr::TraversalMeStmt(MeStmt &meStmt) {
  Opcode op = meStmt.GetOp();
  switch (op) {
    case OP_dassign: {
      auto *varMeStmt = static_cast<DassignMeStmt*>(&meStmt);
      VisitMeExpr(varMeStmt->GetRHS());
      break;
    }
    case OP_regassign: {
      auto *regMeStmt = static_cast<AssignMeStmt*>(&meStmt);
      VisitMeExpr(regMeStmt->GetRHS());
      break;
    }
    case OP_maydassign: {
      auto *maydStmt = static_cast<MaydassignMeStmt*>(&meStmt);
      VisitMeExpr(maydStmt->GetRHS());
      break;
    }
    case OP_iassign: {
      auto *ivarStmt = static_cast<IassignMeStmt*>(&meStmt);
      VisitMeExpr(ivarStmt->GetRHS());
      break;
    }
    case OP_syncenter:
    case OP_syncexit: {
      auto *syncMeStmt = static_cast<SyncMeStmt*>(&meStmt);
      const MapleVector<MeExpr*> &opnds = syncMeStmt->GetOpnds();
      for (size_t i = 0; i < opnds.size(); ++i) {
        MeExpr *opnd = opnds[i];
        VisitMeExpr(opnd);
      }
      break;
    }
    case OP_throw: {
      auto *thrMeStmt = static_cast<ThrowMeStmt*>(&meStmt);
      VisitMeExpr(thrMeStmt->GetOpnd());
      break;
    }
    case OP_assertnonnull:
    case OP_eval:
    case OP_igoto:
    case OP_free: {
      auto *unaryStmt = static_cast<UnaryMeStmt*>(&meStmt);
      VisitMeExpr(unaryStmt->GetOpnd());
      break;
    }
    case OP_asm:
    case OP_call:
    case OP_virtualcall:
    case OP_virtualicall:
    case OP_superclasscall:
    case OP_interfacecall:
    case OP_interfaceicall:
    case OP_customcall:
    case OP_polymorphiccall:
    case OP_callassigned:
    case OP_virtualcallassigned:
    case OP_virtualicallassigned:
    case OP_superclasscallassigned:
    case OP_interfacecallassigned:
    case OP_interfaceicallassigned:
    case OP_customcallassigned:
    case OP_polymorphiccallassigned: {
      auto *callMeStmt = static_cast<CallMeStmt*>(&meStmt);
      const MapleVector<MeExpr*> &opnds = callMeStmt->GetOpnds();
      for (size_t i = 0; i < opnds.size(); ++i) {
        MeExpr *opnd = opnds[i];
        VisitMeExpr(opnd);
      }
      break;
    }
    case OP_icall:
    case OP_icallassigned: {
      auto *icallMeStmt = static_cast<IcallMeStmt*>(&meStmt);
      const MapleVector<MeExpr*> &opnds = icallMeStmt->GetOpnds();
      for (size_t i = 0; i < opnds.size(); ++i) {
        MeExpr *opnd = opnds[i];
        VisitMeExpr(opnd);
      }
      break;
    }
    case OP_intrinsiccallwithtype:
    case OP_intrinsiccall:
    case OP_xintrinsiccall:
    case OP_intrinsiccallwithtypeassigned:
    case OP_intrinsiccallassigned:
    case OP_xintrinsiccallassigned: {
      auto *intrinCallStmt = static_cast<IntrinsiccallMeStmt*>(&meStmt);
      const MapleVector<MeExpr*> &opnds = intrinCallStmt->GetOpnds();
      for (size_t i = 0; i < opnds.size(); ++i) {
        MeExpr *opnd = opnds[i];
        VisitMeExpr(opnd);
      }
      break;
    }
    case OP_brtrue:
    case OP_brfalse: {
      auto *condGotoStmt = static_cast<CondGotoMeStmt*>(&meStmt);
      VisitMeExpr(condGotoStmt->GetOpnd());
      break;
    }
    case OP_switch: {
      auto *switchStmt = static_cast<SwitchMeStmt*>(&meStmt);
      VisitMeExpr(switchStmt->GetOpnd());
      break;
    }
    case OP_return: {
      auto *retMeStmt = static_cast<RetMeStmt*>(&meStmt);
      const MapleVector<MeExpr*> &opnds = retMeStmt->GetOpnds();
      for (size_t i = 0; i < opnds.size(); ++i) {
        MeExpr *opnd = opnds[i];
        VisitMeExpr(opnd);
      }
      ReturnTyIdxInferring(*retMeStmt);
      break;
    }
    CASE_OP_ASSERT_BOUNDARY {
      auto *assMeStmt = static_cast<AssertMeStmt*>(&meStmt);
      VisitMeExpr(assMeStmt->GetOpnd(0));
      VisitMeExpr(assMeStmt->GetOpnd(1));
      break;
    }
    case OP_jstry:
    case OP_jscatch:
    case OP_finally:
    case OP_endtry:
    case OP_cleanuptry:
    case OP_try:
    case OP_catch:
    case OP_goto:
    case OP_gosub:
    case OP_retsub:
    case OP_comment:
    case OP_membaracquire:
    case OP_membarrelease:
    case OP_membarstoreload:
    case OP_membarstorestore:
      break;
    default:
      CHECK_FATAL(false, "unexpected stmt or NYI");
  }
  if (meStmt.GetOp() != OP_callassigned) {
    return;
  }
  MapleVector<MustDefMeNode> *mustDefList = meStmt.GetMustDefList();
  if (mustDefList->empty()) {
    return;
  }
  MeExpr *meLHS = mustDefList->front().GetLHS();
  if (meLHS->GetMeOp() != kMeOpVar) {
    return;
  }
  auto *lhsVar = static_cast<VarMeExpr*>(meLHS);
  auto *callMeStmt = static_cast<CallMeStmt*>(&meStmt);
  MIRFunction &called = callMeStmt->GetTargetFunction();
  if (called.GetInferredReturnTyIdx() != 0u) {
    lhsVar->SetInferredTyIdx(called.GetInferredReturnTyIdx());
    if (called.GetRetrunAttrKind() == kRetrunNoNull) {
      lhsVar->SetMaybeNull(false);
    }
    if (PropReturnAttr::debug) {
      MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(lhsVar->GetInferredTyIdx());
      LogInfo::MapleLogger() << "[PROP-RETURN-ATTR] [TYPE-INFERRING] mx" << lhsVar->GetExprID() << " ";
      type->Dump(0, false);
      LogInfo::MapleLogger() << '\n';
    }
  } else if (called.GetRetrunAttrKind() == kRetrunNoNull || called.GetName() == "malloc") {
    lhsVar->SetMaybeNull(false);
  }
}

void PropReturnAttr::TraversalBB(BB *bb) {
  if (bb == nullptr) {
    return;
  }
  // traversal var phi nodes
  MapleMap<OStIdx, MePhiNode*> &mePhiList = bb->GetMePhiList();
  for (auto it = mePhiList.begin(); it != mePhiList.end(); ++it) {
    MePhiNode *phiMeNode = it->second;
    if (phiMeNode == nullptr || phiMeNode->GetLHS()->GetMeOp() != kMeOpVar) {
      continue;
    }
    VisitVarPhiNode(*phiMeNode);
  }
  // traversal reg phi nodes (NYI)
  // traversal on stmt
  for (auto &meStmt : bb->GetMeStmts()) {
    TraversalMeStmt(meStmt);
  }
}

void PropReturnAttr::Perform(MeFunction &func) {
  // Pre-order traverse the cominance tree, so that each def is traversed
  // before its use
  std::vector<bool> bbVisited(func.GetCfg()->GetAllBBs().size(), false);
  Dominance *dom = static_cast<MEDominance*>(dataMap.GetVaildAnalysisPhase(func.GetUniqueID(),
                                                                           &MEDominance::id))->GetResult();
  for (auto *bb : dom->GetReversePostOrder()) {
    TraversalBB(bb);
  }
  MIRFunction *mirFunc = func.GetMirFunc();
  if (mirFunc == nullptr) {
    return;
  }
  if (retTy == kSeen && !maybeNull) {
    mirFunc->SetRetrunAttrKind(kRetrunNoNull);
  }
}

void PropReturnAttr::Initialize(maple::SCCNode &scc) {
  for (auto *cgNode : scc.GetCGNodes()) {
    MIRFunction *func = cgNode->GetMIRFunction();
    if (func->IsEmpty()) {
      continue;
    }
    if (func->GetAttr(FUNCATTR_nonnull)) {
      func->SetRetrunAttrKind(kRetrunNoNull);
    }
  }
}

void PropReturnAttr::Prop(maple::SCCNode &scc) {
  for (auto *cgNode : scc.GetCGNodes()) {
    retTy = kNotSeen;
    maybeNull = true;
    MIRFunction *func = cgNode->GetMIRFunction();
    if (func->IsEmpty() || func->GetReturnType()->GetKind() != kTypePointer) {
      continue;
    }
    if (func->GetRetrunAttrKind() == FuncReturnAttr::kRetrunNoNull) {
      continue;
    }
    MeFunction *meFunc = func->GetMeFunc();
    Perform(*meFunc);
  }
}

void SCCPropReturnAttr::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<SCCPrepare>();
}

bool SCCPropReturnAttr::PhaseRun(maple::SCCNode &scc) {
  MIRModule *m = ((scc.GetCGNodes()[0])->GetMIRFunction())->GetModule();
  auto *memPool = GetPhaseMemPool();
  MapleAllocator alloc = MapleAllocator(memPool);
  MaplePhase *it = GetAnalysisInfoHook()->GetTopLevelAnalyisData<M2MCallGraph, MIRModule>(*m);
  CallGraph *cg = static_cast<M2MCallGraph*>(it)->GetResult();
  CHECK_FATAL(cg != nullptr, "Expecting a valid CallGraph, found nullptr");
  AnalysisDataManager *dataMap = GET_ANALYSIS(SCCPrepare, scc);
  PropReturnAttr prop(*memPool, alloc, *m, *cg, *dataMap);
  prop.Initialize(scc);
  prop.Prop(scc);
  return true;
}
}
