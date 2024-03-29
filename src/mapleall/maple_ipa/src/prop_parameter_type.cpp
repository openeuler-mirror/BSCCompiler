/*
 * Copyright (c) [2021-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "prop_parameter_type.h"
#include "call_graph.h"
#include "maple_phase.h"
#include "maple_phase.h"
#include "option.h"
#include "string_utils.h"
#include "mir_function.h"
#include "me_dominance.h"

namespace maple {
bool PropParamType::CheckOpndZero(const MeExpr *expr) const {
  if (expr->GetMeOp() == kMeOpConst &&
      static_cast<const ConstMeExpr*>(expr)->IsZero()) {
    return true;
  }
  return false;
}

bool PropParamType::CheckCondtionStmt(const MeStmt &meStmt) const {
  auto *node = meStmt.GetOpnd(0);
  auto subOpnd0 = node->GetOpnd(0);
  auto subOpnd1 = node->GetOpnd(1);
  return CheckOpndZero(subOpnd0) || CheckOpndZero(subOpnd1);
}

void PropParamType::ResolveIreadExpr(MeExpr &expr) {
  switch (expr.GetMeOp()) {
    case kMeOpIvar: {
      auto *ivarMeExpr = static_cast<IvarMeExpr*>(&expr);
      const MeExpr *base = ivarMeExpr->GetBase();
      if (base->GetMeOp() == kMeOpNary && base->GetOp() == OP_array) {
        base = base->GetOpnd(0);
      }
      if (base->GetMeOp() == kMeOpVar) {
        const VarMeExpr *varMeExpr = static_cast<const VarMeExpr*>(base);
        MIRSymbol *sym = varMeExpr->GetOst()->GetMIRSymbol();
        if (sym->IsFormal() && formalMapLocal[sym] != PointerAttr::kPointerNull) {
          formalMapLocal[sym] = PointerAttr::kPointerNoNull;
        }
      }
      break;
    }
    default: {
      for (uint32 i = 0; i < expr.GetNumOpnds(); ++i) {
        auto *subExpr = expr.GetOpnd(i);
        ResolveIreadExpr(*subExpr);
      }
    }
  }
}

void PropParamType::InsertNullCheck(CallMeStmt &callStmt, const std::string &funcName,
                                    uint32 index, MeExpr &receiver) {
  auto *irMap = curFunc->GetMeFunc()->GetIRMap();
  GStrIdx stridx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(funcName);
  CallAssertNonnullMeStmt *nullCheck = irMap->New<CallAssertNonnullMeStmt>(OP_callassertnonnull,
      stridx, index, builder.GetCurrentFunction()->GetNameStrIdx());
  nullCheck->SetBB(callStmt.GetBB());
  nullCheck->SetSrcPos(callStmt.GetSrcPosition());
  nullCheck->SetMeStmtOpndValue(&receiver);
  callStmt.GetBB()->InsertMeStmtBefore(&callStmt, nullCheck);
}

void PropParamType::ResolveCallStmt(MeStmt &meStmt) {
  auto *callMeStmt = static_cast<CallMeStmt*>(&meStmt);
  PUIdx puidx = callMeStmt->GetPUIdx();
  MIRFunction *calledFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puidx);

  // If the parameter is passed through the fucntion call, we think it maybe have a judge.
  for (auto &map : std::as_const(*meStmt.GetMuList())) {
    OStIdx idx = map.first;
    SSATab *ssaTab = static_cast<MESSATab*>(dataMap.GetVaildAnalysisPhase(curFunc->GetMeFunc()->GetUniqueID(),
                                                                          &MESSATab::id))->GetResult();
    OriginalSt *ostTemp = ssaTab->GetSymbolOriginalStFromID(idx);
    ASSERT(ostTemp != nullptr, "null ptr check");
    MIRSymbol *tempSymbol = ostTemp->GetMIRSymbol();
    if (tempSymbol != nullptr && tempSymbol->IsFormal() && curFunc->GetParamNonull(tempSymbol) !=
                                                           PointerAttr::kPointerNoNull) {
      formalMapLocal[tempSymbol] = PointerAttr::kPointerNull;
    }
  }
  // Insert the assertstmt && analysis curFunc parameter
  if (calledFunc->IsExtern() || calledFunc->IsEmpty()) {
    return;
  }
  for (uint32 i = 0; i < calledFunc->GetFormalCount(); i++) {
    MIRSymbol *formalSt = calledFunc->GetFormal(i);
    if (formalSt->GetType()->GetKind() == kTypePointer) {
      if (calledFunc->CheckParamNullType(formalSt) &&
          calledFunc->GetParamNonull(formalSt) == PointerAttr::kPointerNoNull) {
        InsertNullCheck(*callMeStmt, calledFunc->GetName(), i, *callMeStmt->GetOpnd(i));
        MIRSymbol *calledFuncFormalSt = calledFunc->GetFormal(i);
        if (calledFuncFormalSt->IsFormal() &&
            curFunc->GetParamNonull(calledFuncFormalSt) != PointerAttr::kPointerNull) {
          formalMapLocal[calledFuncFormalSt] = PointerAttr::kPointerNoNull;
        }
      }
    }
  }
}

void PropParamType::TraversalMeStmt(MeStmt &meStmt) {
  if (meStmt.GetOp() == OP_brfalse || meStmt.GetOp() == OP_brtrue) {
    auto *opnd = meStmt.GetOpnd(0);
    if (opnd->GetOp() != OP_eq && opnd->GetOp() != OP_ne && opnd->GetOp() != OP_gt && opnd->GetOp() != OP_ge) {
      for (uint32 i = 0; i < meStmt.NumMeStmtOpnds(); ++i) {
        auto *expr = meStmt.GetOpnd(i);
        ResolveIreadExpr(*expr);
      }
      return;
    }
    if (CheckCondtionStmt(meStmt)) {
      auto subOpnd0 = opnd->GetOpnd(0);
      auto subOpnd1 = opnd->GetOpnd(1);
      MeExpr *expr = CheckOpndZero(subOpnd0) ? subOpnd1 : subOpnd0;
      if (expr->GetOp() == OP_dread) {
        VarMeExpr *varExpr = static_cast<VarMeExpr *>(expr);
        MIRSymbol *sym = varExpr->GetOst()->GetMIRSymbol();
        if (sym->IsFormal()) {
          formalMapLocal[sym] = PointerAttr::kPointerNull;
          return;
        }
        if (meStmt.GetMuList() == nullptr) {
          return;
        }
        for (auto map : *meStmt.GetMuList()) {
          OStIdx idx = map.first;
          SSATab *ssaTab = static_cast<MESSATab*>(dataMap.GetVaildAnalysisPhase(curFunc->GetMeFunc()->GetUniqueID(),
                                                                                &MESSATab::id))->GetResult();
          ASSERT(ssaTab != nullptr, "null ptr check");
          OriginalSt *ostTemp = ssaTab->GetSymbolOriginalStFromID(idx);
          ASSERT(ostTemp != nullptr, "null ptr check");
          MIRSymbol *tempSymbol = ostTemp->GetMIRSymbol();
          if (tempSymbol->IsFormal()) {
            formalMapLocal[tempSymbol] = PointerAttr::kPointerNull;
            return;
          }
        }
      }
    }
  } else if (meStmt.GetOp() == OP_callassigned || meStmt.GetOp() == OP_call) {
    ResolveCallStmt(meStmt);
  } else {
    for (uint32 i = 0; i < meStmt.NumMeStmtOpnds(); ++i) {
      auto *expr = meStmt.GetOpnd(i);
      ResolveIreadExpr(*expr);
    }
  }
}

void PropParamType::RunOnScc(maple::SCCNode<CGNode> &scc) {
  for (auto *cgNode : scc.GetNodes()) {
    MIRFunction *func = cgNode->GetMIRFunction();
    if (func->IsEmpty()) {
      continue;
    }
    formalMapLocal.clear();
    curFunc = func;
    for (uint32 i = 0; i < func->GetFormalCount(); i++) {
      MIRSymbol *formalSt = func->GetFormal(i);
      if (formalSt->GetType()->GetKind() != kTypePointer) {
        continue;
      }
      if (formalSt->GetAttr(ATTR_nonnull)) {
        func->SetParamNonull(formalSt, PointerAttr::kPointerNoNull);
        formalMapLocal[formalSt] = PointerAttr::kPointerNoNull;
      }
    }
    Prop(*func);
    for (auto it = formalMapLocal.begin(); it != formalMapLocal.end(); ++it) {
      func->SetParamNonull(it->first, it->second);
      if (it->second == PointerAttr::kPointerNoNull) {
        static_cast<MIRSymbol*>(it->first)->SetAttr(ATTR_nonnull);
        uint32 index = func->GetFormalIndex(it->first);
        if (index != 0xffffffff) {
          FormalDef &formalDef = func->GetFormalDefAt(index);
          formalDef.formalAttrs.SetAttr(ATTR_nonnull);
        }
      }
    }
  }
}

void PropParamType::Prop(MIRFunction &func) {
  for (uint32 i = 0; i < func.GetFormalCount(); i++) {
    MIRSymbol *formalSt = func.GetFormal(i);
    if (formalSt->GetType()->GetKind() == kTypePointer) {
      formalMapLocal[formalSt] = PointerAttr::kPointerUndeiced;
    }
  }
  auto dom = static_cast<MEDominance*>(
      dataMap.GetVaildAnalysisPhase(curFunc->GetMeFunc()->GetUniqueID(), &MEDominance::id))->GetDomResult();
  CHECK_NULL_FATAL(dom);
  for (auto *node : dom->GetReversePostOrder()) {
    if (node == nullptr) {
      return;
    }
    auto bb = func.GetMeFunc()->GetCfg()->GetBBFromID(BBId(node->GetID()));
    // traversal on stmt
    for (auto &meStmt : bb->GetMeStmts()) {
      TraversalMeStmt(meStmt);
    }
  }
}

void SCCPropParamType::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<SCCPrepare>();
  aDep.SetPreservedAll();
}

bool SCCPropParamType::PhaseRun(maple::SCCNode<CGNode> &scc) {
  MIRModule *m = ((scc.GetNodes()[0])->GetMIRFunction())->GetModule();
  auto *memPool = GetPhaseMemPool();
  MapleAllocator alloc = MapleAllocator(memPool);
  MaplePhase *it = GetAnalysisInfoHook()->FindOverIRAnalyisData<M2MCallGraph, MIRModule>(*m);
  CallGraph *cg = static_cast<M2MCallGraph*>(it)->GetResult();
  CHECK_FATAL(cg != nullptr, "Expecting a valid CallGraph, found nullptr");
  AnalysisDataManager *dataMap = GET_ANALYSIS(SCCPrepare, scc);
  PropParamType prop(*memPool, alloc, *m, *cg, *dataMap);
  prop.RunOnScc(scc);
  return true;
}
}
