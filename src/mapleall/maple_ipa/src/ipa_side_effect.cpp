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
#include "ipa_side_effect.h"
#include "func_desc.h"
namespace maple {
const std::map<std::string, FuncDesc> whiteList = {
#include "func_desc.def"
};

const FuncDesc &SideEffect::GetFuncDesc(MeFunction &f) {
  return SideEffect::GetFuncDesc(*f.GetMirFunc());
}

const FuncDesc &SideEffect::GetFuncDesc(MIRFunction &f) {
  auto it = whiteList.find(f.GetName());
  if (it != whiteList.end()) {
    return it->second;
  }
  return f.GetFuncDesc();
}

const std::map<std::string, FuncDesc> &SideEffect::GetWhiteList() {
  return whiteList;
}

void SideEffect::PropInfoFromCallee(const CallMeStmt &call, MIRFunction &callee) {
  const FuncDesc &desc = callee.GetFuncDesc();

  auto paramInfoUpdater = [&](size_t vstIdx, size_t calleeIdx) -> void {
    for (size_t callerIdx = 0; callerIdx < vstsValueAliasWithFormal.size(); ++callerIdx) {
      auto &formals = vstsValueAliasWithFormal[callerIdx];
      if (formals.find(vstIdx) != formals.end()) {
        curFuncDesc->SetParamInfoNoBetterThan(callerIdx, desc.GetParamInfo(calleeIdx));
      }
    }
  };

  for (size_t calleeIdx = 0; calleeIdx < callee.GetFormalCount(); ++calleeIdx) {
    MeExpr *opnd = call.GetOpnd(calleeIdx);
    OriginalSt *ost = nullptr;
    switch (opnd->GetMeOp()) {
      case kMeOpVar: {
        auto *dread = static_cast<ScalarMeExpr*>(opnd);
        paramInfoUpdater(dread->GetVstIdx(), calleeIdx);
        break;
      }
      case kMeOpAddrof: {
        AddrofMeExpr *addrofMeExpr = static_cast<AddrofMeExpr*>(opnd);
        // As in CollectFormalOst, this is conservative to make sure it's right.
        // For example:
        // void callee(int *p) : write memory that p points to.
        // call callee(&x) : this will modify x but we prop info of 'write memory' to x.
        ost = meFunc->GetMeSSATab()->GetOriginalStFromID(addrofMeExpr->GetOstIdx());
        for (auto vstIdx : ost->GetVersionsIndices()) {
          paramInfoUpdater(vstIdx, calleeIdx);
        }
        break;
      }
      default:
        break;
    }
  }
}

void SideEffect::DealWithStmt(MeStmt &stmt) {
  for (size_t i = 0; i < stmt.NumMeStmtOpnds(); ++i) {
    DealWithOperand(stmt.GetOpnd(i));
  }
  RetMeStmt *ret = safe_cast<RetMeStmt>(&stmt);
  if (ret != nullptr) {
    DealWithReturn(*ret);
  }
  CallMeStmt *call = safe_cast<CallMeStmt>(&stmt);
  if (call != nullptr) {
    MIRFunction *calleeFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(call->GetPUIdx());
    const FuncDesc &desc = calleeFunc->GetFuncDesc();
    if (!desc.IsPure() && !desc.IsConst()) {
      curFuncDesc->SetFuncInfoNoBetterThan(FI::kUnknown);
    }
    if (desc.IsPure()) {
      curFuncDesc->SetFuncInfoNoBetterThan(FI::kPure);
    }
    if (desc.IsConst()) {
      curFuncDesc->SetFuncInfoNoBetterThan(FI::kConst);
    }
    PropInfoFromCallee(*call, *calleeFunc);
  }
  if (stmt.GetMuList() == nullptr) {
    return;
  }
  // this may cause some kWriteMemoryOnly regard as kReadWriteMemory.
  // Example: {a.f = b} mulist in return stmt will regard param a as used.
  for (auto &mu : *stmt.GetMuList()) {
    DealWithOst(mu.first);
  }
}

void SideEffect::DealWithOst(OStIdx ostIdx) {
  OriginalSt *ost = meFunc->GetMeSSATab()->GetSymbolOriginalStFromID(ostIdx);
  DealWithOst(ost);
}

void SideEffect::DealWithOst(const OriginalSt *ost) {
  if (ost == nullptr) {
    return;
  }
  for (auto &pair : analysisLater) {
    if (pair.first == ost) {
      curFuncDesc->SetParamInfoNoBetterThan(pair.second, PI::kReadWriteMemory);
      return;
    }
  }
}

void SideEffect::DealWithOperand(MeExpr *expr) {
  if (expr == nullptr) {
    return;
  }
  for (uint32 i = 0; i < expr->GetNumOpnds(); ++i) {
    DealWithOperand(expr->GetOpnd(i));
  }
  switch (expr->GetMeOp()) {
    case kMeOpVar: {
      ScalarMeExpr *dread = static_cast<ScalarMeExpr*>(expr);
      OriginalSt *ost = dread->GetOst();
      DealWithOst(ost);
      break;
    }
    case kMeOpIvar: {
      auto *base = static_cast<IvarMeExpr*>(expr)->GetBase();
      if (base->GetMeOp() == kMeOpVar) {
        ScalarMeExpr *dread = static_cast<ScalarMeExpr*>(base);
        DealWithOst(dread->GetOst());
      }
      break;
    }
    default:
      break;
  }
  return;
}

void SideEffect::DealWithReturn(const RetMeStmt &retMeStmt) {
  if (retMeStmt.NumMeStmtOpnds() == 0) {
    return;
  }
  MeExpr *ret = retMeStmt.GetOpnd(0);
  if (ret->GetPrimType() == PTY_agg) {
    curFuncDesc->SetReturnInfo(RI::kUnknown);
    return;
  }
  if (!IsAddress(ret->GetPrimType())) {
    return;
  }
  if (ret->GetType() != nullptr && ret->GetType()->IsMIRPtrType()) {
    auto *ptrType = static_cast<MIRPtrType*>(ret->GetType());
    if (ptrType->GetPointedType()->GetPrimType() == PTY_agg) {
      curFuncDesc->SetReturnInfo(RI::kUnknown);
      return;
    }
  }
  OriginalSt *retOst = nullptr;
  size_t vstIdxOfRet = 0;
  if (ret->IsScalar()) {
    retOst = static_cast<ScalarMeExpr*>(ret)->GetOst();
    vstIdxOfRet = static_cast<ScalarMeExpr*>(ret)->GetVstIdx();
  } else if (ret->GetMeOp() == kMeOpIvar) {
    auto *base = static_cast<IvarMeExpr*>(ret)->GetBase();
    if (base->IsScalar()) {
      retOst = static_cast<ScalarMeExpr*>(base)->GetOst();
      vstIdxOfRet = static_cast<ScalarMeExpr*>(base)->GetVstIdx();
    }
  }
  if (retOst == nullptr) {
    return;
  }
  if (retOst->IsFormal()) {
    curFuncDesc->SetReturnInfo(RI::kUnknown);
    return;
  }
  std::set<size_t> result;
  alias->GetValueAliasSetOfVst(vstIdxOfRet, result);
  for (auto valueAliasVstIdx : result) {
    auto *meExpr = meFunc->GetIRMap()->GetVerst2MeExprTableItem(valueAliasVstIdx);
    // meExpr of valueAliasVstIdx not created in IRMap, it must not occured in hssa-mefunction
    if (meExpr == nullptr) {
      continue;
    }
    OriginalSt *aliasOst = nullptr;
    if (meExpr->GetMeOp() == kMeOpAddrof) {
      auto ostIdx = static_cast<AddrofMeExpr*>(meExpr)->GetOstIdx();
      aliasOst = meFunc->GetMeSSATab()->GetOriginalStFromID(ostIdx);
    } else if (meExpr->IsScalar()) {
      aliasOst = static_cast<ScalarMeExpr*>(meExpr)->GetOst();
    } else {
      CHECK_FATAL(false, "not supported meExpr");
    }
    if (aliasOst->IsFormal()) {
      curFuncDesc->SetReturnInfo(RI::kUnknown);
    }
  }
}

void SideEffect::SolveVarArgs(MeFunction &f) {
  MIRFunction *func = f.GetMirFunc();
  if (func->IsVarargs()) {
    for (size_t i = func->GetFormalCount(); i < kMaxParamCount; ++i) {
      curFuncDesc->SetParamInfoNoBetterThan(i, PI::kUnknown);
    }
    curFuncDesc->SetFuncInfoNoBetterThan(FI::kUnknown);
  }
}

void SideEffect::CollectAllLevelOst(size_t vstIdx, std::set<size_t> &result) {
  result.insert(vstIdx);
  auto *nextLevelOsts = meFunc->GetMeSSATab()->GetNextLevelOsts(vstIdx);
  if (nextLevelOsts == nullptr) {
    return;
  }
  for (auto *nlOst : *nextLevelOsts) {
    for (auto vstIdOfNextLevelOst : nlOst->GetVersionsIndices()) {
      CollectAllLevelOst(vstIdOfNextLevelOst, result);
    }
  }
}

void SideEffect::CollectFormalOst(MeFunction &f) {
  MIRFunction *func = f.GetMirFunc();
  for (auto *ost : f.GetMeSSATab()->GetOriginalStTable().GetOriginalStVector()) {
    if (ost == nullptr) {
      continue;
    }
    if (!ost->IsLocal()) {
      curFuncDesc->SetFuncInfoNoBetterThan(FI::kPure);
      if (ost->GetVersionsIndices().size() > 1) {
        curFuncDesc->SetFuncInfoNoBetterThan(FI::kUnknown);
      }
    }
    if (ost->IsFormal() && ost->GetIndirectLev() == 0) {
      auto idx = func->GetFormalIndex(ost->GetMIRSymbol());
      if (idx >= kMaxParamCount) {
        continue;
      }

      // Put level -1 ost into it, so we can get a conservative result.
      // Because when we solve all ostValueAliasWithFormal we regard every ost in it as lev 0.

      std::set<size_t> vstValueAliasFormal;
      if (ost->IsAddressTaken()) {
        CollectAllLevelOst(ost->GetPointerVstIdx(), vstsValueAliasWithFormal[idx]);
        alias->GetValueAliasSetOfVst(ost->GetPointerVstIdx(), vstValueAliasFormal);
      }
      CollectAllLevelOst(ost->GetZeroVersionIndex(), vstsValueAliasWithFormal[idx]);
      alias->GetValueAliasSetOfVst(ost->GetZeroVersionIndex(), vstValueAliasFormal);

      for (size_t vstIdx: vstValueAliasFormal) {
        auto *meExpr = meFunc->GetIRMap()->GetVerst2MeExprTableItem(vstIdx);
        if (meExpr == nullptr || meExpr->GetMeOp() == kMeOpAddrof) {
          // corresponding ScalarMeExpr has not created in irmap for vstIdx.
          CollectAllLevelOst(vstIdx, vstsValueAliasWithFormal[idx]);
          continue;
        }
        CHECK_FATAL(meExpr->IsScalar(), "not supported MeExpr type");
        CHECK_FATAL(static_cast<ScalarMeExpr*>(meExpr)->GetVstIdx() == vstIdx, "VersionSt index must be equal");
        auto *aliasOst = static_cast<ScalarMeExpr*>(meExpr)->GetOst();
        if (aliasOst != ost) {
          if (aliasOst->IsTopLevelOst()) {
            CollectAllLevelOst(vstIdx, vstsValueAliasWithFormal[idx]);
          } else {
            for (auto vstIdxOfAliasOst : aliasOst->GetVersionsIndices()) {
              CollectAllLevelOst(vstIdxOfAliasOst, vstsValueAliasWithFormal[idx]);
            }
          }
        }
      }
    }
  }
}

void SideEffect::AnalysisFormalOst() {
  for (size_t formalIndex = 0; formalIndex < vstsValueAliasWithFormal.size(); ++formalIndex) {
    for (size_t vstIdx : vstsValueAliasWithFormal[formalIndex]) {
      curFuncDesc->SetParamInfoNoBetterThan(formalIndex, PI::kReadSelfOnly);
      auto *meExpr = meFunc->GetIRMap()->GetVerst2MeExprTableItem(vstIdx);
      if (meExpr == nullptr) {
        continue;
      }
      if (meExpr->GetMeOp() == kMeOpAddrof) {
        curFuncDesc->SetParamInfoNoBetterThan(formalIndex, PI::kUnknown);
        curFuncDesc->SetFuncInfoNoBetterThan(FI::kUnknown);
        continue;
      }
      CHECK_FATAL(meExpr->IsScalar(), "must be me scalar");
      auto *ost = static_cast<ScalarMeExpr*>(meExpr)->GetOst();
      if (ost->GetIndirectLev() == 0 && ost->GetVersionsIndices().size() == 1) {
        curFuncDesc->SetParamInfoNoBetterThan(formalIndex, PI::kReadSelfOnly);
        continue;
      }
      if (ost->GetIndirectLev() == 1) {
        if (ost->GetVersionsIndices().size() == 1) {
          curFuncDesc->SetParamInfoNoBetterThan(formalIndex, PI::kReadMemoryOnly);
          curFuncDesc->SetFuncInfoNoBetterThan(FI::kPure);
        } else {
          analysisLater.insert(std::make_pair(ost, formalIndex));
          curFuncDesc->SetParamInfoNoBetterThan(formalIndex, PI::kWriteMemoryOnly);
          curFuncDesc->SetFuncInfoNoBetterThan(FI::kUnknown);
        }
        continue;
      }
      if (ost->GetIndirectLev() > 1) {
        if (ost->GetVersionsIndices().size() == 1) {
          curFuncDesc->SetParamInfoNoBetterThan(formalIndex, PI::kReadMemoryOnly);
          curFuncDesc->SetFuncInfoNoBetterThan(FI::kPure);
        } else {
          curFuncDesc->SetParamInfoNoBetterThan(formalIndex, PI::kUnknown);
          curFuncDesc->SetFuncInfoNoBetterThan(FI::kUnknown);
        }
      }
    }
  }
}

bool SideEffect::Perform(MeFunction &f) {
  MIRFunction *func = f.GetMirFunc();
  curFuncDesc = &func->GetFuncDesc();
  FuncDesc oldDesc = *curFuncDesc;

  SolveVarArgs(f);
  CollectFormalOst(f);
  AnalysisFormalOst();
  for (auto *bb : dom->GetReversePostOrder()) {
    for (auto &stmt : bb->GetMeStmts()) {
      DealWithStmt(stmt);
    }
  }
  return !curFuncDesc->Equals(oldDesc);
}

bool SCCSideEffect::PhaseRun(SCCNode<CGNode> &scc) {
  for (CGNode *node : scc.GetNodes()) {
    MIRFunction *func = node->GetMIRFunction();
    if (func != nullptr) {
      func->InitFuncDescToBest();
      func->GetFuncDesc().SetReturnInfo(RI::kUnknown);
      if (func->GetParamSize() > kMaxParamCount) {
        func->GetFuncDesc().SetFuncInfoNoBetterThan(FI::kUnknown);
      }
    }
  }
  bool changed = true;
  while (changed) {
    changed = false;
    auto *map = GET_ANALYSIS(SCCPrepare, scc);
    for (CGNode *node : scc.GetNodes()) {
      MIRFunction *func = node->GetMIRFunction();
      if (func == nullptr) {
        continue;
      }
      MeFunction *meFunc = func->GetMeFunc();
      if (meFunc == nullptr || meFunc->GetCfg()->NumBBs() == 0) {
        continue;
      }
      auto *phase = map->GetVaildAnalysisPhase(meFunc->GetUniqueID(), &MEDominance::id);
      Dominance *dom = static_cast<MEDominance*>(phase)->GetResult();
      phase = map->GetVaildAnalysisPhase(meFunc->GetUniqueID(), &MEAliasClass::id);
      AliasClass *alias = static_cast<MEAliasClass*>(phase)->GetResult();

      phase = map->GetVaildAnalysisPhase(meFunc->GetUniqueID(), &MESSATab::id);
      SSATab *meSSATab = static_cast<MESSATab*>(phase)->GetResult();
      CHECK_FATAL(meSSATab == meFunc->GetMeSSATab(), "IPA_PM may be wrong.");
      SideEffect se(meFunc, dom, alias);
      changed |= se.Perform(*meFunc);
    }
  }
  return false;
}

void SCCSideEffect::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<SCCPrepare>();
}
}  // namespace maple
