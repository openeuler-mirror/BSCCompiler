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
#include "me_gc_write_barrier_opt.h"
#include "dominance.h"

// GCWriteBarrierOpt phase generate GC intrinsic for reference assignment
// based on previous analyze results. GC intrinsic will later be lowered
// in Code Generation
namespace maple {
#ifdef NOT_USED
AnalysisResult *MeDoGCWriteBarrierOpt::Run(MeFunction *func, MeFuncResultMgr *funcResMgr, ModuleResultMgr*) {
  Dominance *dom = static_cast<Dominance*>(funcResMgr->GetAnalysisResult(MeFuncPhase_DOMINANCE, func));
  CHECK_FATAL(dom != nullptr, "dominance phase has problem");

  GCWriteBarrierOpt gcWriteBarrierOpt(*func, *dom, DEBUGFUNC(func));
  gcWriteBarrierOpt.Prepare();
  std::map<OStIdx, std::vector<MeStmt*>> writeBarrierMap;
  BB *entryBB = func->GetCfg()->GetCommonEntryBB();
  gcWriteBarrierOpt.GCLower(*entryBB, writeBarrierMap);
  gcWriteBarrierOpt.Finish();
  return nullptr;
}
#endif

void GCWriteBarrierOpt::Prepare() {
  callBBs.resize(func.GetCfg()->NumBBs());
  visited.resize(func.GetCfg()->NumBBs());
  if (enabledDebug) {
    LogInfo::MapleLogger() << "\n============== Before GC WRITE BARRIER OPT =============" << '\n';
    func.Dump(false);
  }
}

void GCWriteBarrierOpt::GCLower(BB &bb, std::map<OStIdx, std::vector<MeStmt*>> &writeBarrierMap) {
  // to record stack size
  std::map<OStIdx, size_t> savedStacksize;
  for (const auto &item : std::as_const(writeBarrierMap)) {
    savedStacksize[item.first] = item.second.size();
  }
  for (auto &stmt : bb.GetMeStmts()) {
    if (!visited[bb.GetBBId()] && IsCall(stmt)) {
      callBBs[bb.GetBBId()] = true;
      visited[bb.GetBBId()] = true;
    }
    if (IsWriteBarrier(stmt)) {
      IntrinsiccallMeStmt &intrinsicCall = static_cast<IntrinsiccallMeStmt&>(stmt);
      MeExpr *opnd = intrinsicCall.GetOpnd(0);
      OStIdx ostIdx = GetOStIdx(*opnd);
      CHECK_FATAL(ostIdx != 0u, "ostIdx is not expected.");
      std::vector<MeStmt*> &stmtVec = writeBarrierMap[ostIdx];
      if (stmtVec.empty()) {
        stmtVec.push_back(&stmt);
        callBBs[bb.GetBBId()] = true;
      } else {
        MeStmt *start = stmtVec.back();
        if (!HasYieldPoint(*start, stmt)) {
          ResetMeStmt(intrinsicCall);
        } else {
          stmtVec.push_back(&stmt);
          callBBs[bb.GetBBId()] = true;
        }
      }
    }
  }
  visited[bb.GetBBId()] = true;
  const auto &domChildren = dominance.GetDomChildren(bb.GetID());
  for (const auto &childBBId : domChildren) {
    BB *child = func.GetCfg()->GetBBFromID(BBId(childBBId));
    if (child == nullptr) {
      continue;
    }
    GCLower(*child, writeBarrierMap);
  }
  // restore the stacks to their size at entry to this function invocation
  for (auto &item : writeBarrierMap) {
    size_t lastSize = savedStacksize[item.first];
    while (item.second.size() > lastSize) {
      item.second.pop_back();
    }
  }
}

bool GCWriteBarrierOpt::IsWriteBarrier(const MeStmt &stmt) const {
  if (stmt.GetOp() != OP_intrinsiccall) {
    return false;
  }
  const IntrinsiccallMeStmt &intrinsicCall = static_cast<const IntrinsiccallMeStmt&>(stmt);
  MIRIntrinsicID id = intrinsicCall.GetIntrinsic();
  return id == INTRN_MCCWriteS || id == INTRN_MCCWrite;
}

void GCWriteBarrierOpt::ResetMeStmt(IntrinsiccallMeStmt &stmt) {
  MeStmt *meStmt = nullptr;
  MeExpr *rhs = stmt.GetOpnds().back();
  if (stmt.GetIntrinsic() == INTRN_MCCWrite) {
    OpMeExpr *opnd = static_cast<OpMeExpr*>(stmt.GetOpnd(1));
    IvarMeExpr *ivar = irMap.BuildLHSIvar(*opnd->GetOpnd(0), opnd->GetPrimType(),
                                          opnd->GetTyIdx(), opnd->GetFieldID());
    MapleMap<OStIdx, ChiMeNode*> *chiList = stmt.GetChiList();
    meStmt = irMap.CreateIassignMeStmt(opnd->GetTyIdx(), *ivar, *rhs, *chiList);
  } else {
    ScalarMeExpr *lhs = static_cast<ScalarMeExpr *>(stmt.GetOpnds().front());
    if (lhs->GetMeOp() == kMeOpReg) {
      meStmt = irMap.CreateAssignMeStmt(*lhs, *rhs, *stmt.GetBB());
    } else {
      CHECK_FATAL((lhs->GetMeOp() == kMeOpVar && stmt.GetIntrinsic() == INTRN_MCCWriteS), "Not Expected.");
      auto *ost = ssaTab.GetOriginalStFromID(GetOStIdx(*lhs));
      VarMeExpr *lhsVar = irMap.GetOrCreateZeroVersionVarMeExpr(*ost);
      meStmt = irMap.CreateAssignMeStmt(*lhsVar, *rhs, *stmt.GetBB());
    }
  }
  stmt.GetBB()->ReplaceMeStmt(&stmt, meStmt);
}

OStIdx GCWriteBarrierOpt::GetOStIdx(MeExpr &meExpr) {
  MeExprOp meOp = meExpr.GetMeOp();
  switch (meOp) {
    case kMeOpAddrof:
      return (static_cast<AddrofMeExpr&>(meExpr)).GetOstIdx();
    case kMeOpVar:
      return (static_cast<VarMeExpr &>(meExpr)).GetOstIdx();
    case kMeOpReg:
      return (static_cast<RegMeExpr &>(meExpr)).GetOstIdx();
    case kMeOpIvar:
      return GetOStIdx(*((static_cast<IvarMeExpr&>(meExpr)).GetBase()));
    default:
      CHECK_FATAL(meOp == kMeOpReg, "MeOp is not expected.");
      break;
  }
  return OStIdx(0);
}

bool GCWriteBarrierOpt::IsCall(const MeStmt &stmt) const {
  return kOpcodeInfo.IsCall(stmt.GetOp()) && !IsWriteBarrier(stmt);
}

bool GCWriteBarrierOpt::HasYieldPoint(const MeStmt &start, const MeStmt &end) {
  const BB *startBB = start.GetBB();
  const BB *endBB = end.GetBB();
  if (startBB == endBB) {
    return HasCallBetweenStmt(start, end);
  }
  if (!dominance.Dominate(*startBB, *endBB)) {
    return true;
  }
  if (HasCallAfterStmt(start) || HasCallBeforeStmt(end) || IsBackEdgeDest(*endBB)) {
    return true;
  }
  const auto &domChildren = dominance.GetDomChildren(startBB->GetID());
  const auto pdomChildren = postDominance.GetDomChildren(endBB->GetID());
  const MapleVector<BB*> bbVec = func.GetCfg()->GetAllBBs();
  for (const auto &childBBId : domChildren) {
    if (std::find(pdomChildren.begin(), pdomChildren.end(), childBBId) == pdomChildren.end()) {
      continue;
    }
    BB *bb = bbVec.at(childBBId);
    if (bb == nullptr) {
      continue;
    }
    if (IsBackEdgeDest(*bb)) {
      return true;
    }
    if (HasCallInBB(*bb)) {
      return true;
    }
  }
  return false;
}

bool GCWriteBarrierOpt::HasCallAfterStmt(const MeStmt &stmt) const {
  const MeStmt &lastMeStmt = stmt.GetBB()->GetMeStmts().back();
  return HasCallBetweenStmt(stmt, lastMeStmt);
}

bool GCWriteBarrierOpt::HasCallBeforeStmt(const MeStmt &stmt) const {
  const MeStmt &firstMeStmt = stmt.GetBB()->GetMeStmts().front();
  return HasCallBetweenStmt(firstMeStmt, stmt);
}

bool GCWriteBarrierOpt::HasCallBetweenStmt(const MeStmt &start, const MeStmt &end) const {
  CHECK_FATAL(start.GetBB() == end.GetBB(), "NYI.");
  for (const MeStmt *meStmt = &start; meStmt != &end; meStmt = meStmt->GetNext()) {
    if (IsCall(*meStmt)) {
      return true;
    }
  }
  return IsCall(end);
}

bool GCWriteBarrierOpt::IsBackEdgeDest(const BB &bb) {
  for (BB *pred : bb.GetPred()) {
    if (dominance.Dominate(bb, *pred)) {
      return true;
    }
  }
  return false;
}

bool GCWriteBarrierOpt::HasCallInBB(const BB &bb) {
  BBId id = bb.GetBBId();
  if (callBBs.at(id)) {
    return true;
  } else if (visited.at(id)) {
    return false;
  }
  for (auto &stmt : bb.GetMeStmts()) {
    if (IsCall(stmt)) {
      callBBs[id] = true;
      visited[id] = true;
      return true;
    }
  }
  visited[id] = true;
  return false;
}

void GCWriteBarrierOpt::Finish() {
  if (enabledDebug) {
    LogInfo::MapleLogger() << "\n============== After GC WRITE BARRIER OPT =============" << '\n';
    func.Dump(false);
  }
}
}  // namespace maple
