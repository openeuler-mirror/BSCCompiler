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
#include "code_factoring.h"
#include "me_cfg.h"
#include "me_dominance.h"

// This phase aims to move identical stmts into common block to reduce code size
// Two methods are used in this phase:
// 1. local factoring: hoist(TODO)/sink identical stmts to it's common predecessor/successor
// 2. sequence extract: extract a series of stmts into one common block and merge
//                      all predecessors and successors to this block (TODO)

namespace maple {
struct cmpByBBID {
  bool operator()(BB* bb1, BB* bb2) const {
    return bb1->GetBBId() < bb2->GetBBId();
  }
};

class FactoringOptimizer {
 public:
  FactoringOptimizer(MeFunction &f, bool enableDebug)
      : func(f),
        cfg(f.GetCfg()),
        codeMP(f.GetMirFunc()->GetCodeMemPool()),
        codeMPAlloc(&f.GetMirFunc()->GetCodeMemPoolAllocator()),
        mirBuilder(f.GetMIRModule().GetMIRBuilder()),
        dumpDetail(enableDebug) {}
  ~FactoringOptimizer() = default;
  bool IsCFGChanged() const {
    return isCFGChanged;
  }
  bool IsIdenticalStmt(StmtNode *stmt1, StmtNode *stmt2);
  void SinkStmtsInCands(std::map<BB*, StmtNode*, cmpByBBID> &cands);
  void DoSink(BB *bb);
  void RunLocalFactoring();

 private:
  MeFunction &func;
  MeCFG *cfg;
  MemPool *codeMP;
  MapleAllocator *codeMPAlloc;
  MIRBuilder *mirBuilder;
  bool dumpDetail = false;
  bool isCFGChanged = false;
  uint32 sinkCount = 0;
};

static bool IsConstOp(Opcode op) {
  return op == OP_constval || op == OP_conststr || op == OP_conststr;
}

bool FactoringOptimizer::IsIdenticalStmt(StmtNode *stmt1, StmtNode *stmt2) {
  auto op = stmt1->GetOpCode();
  if (op != stmt2->GetOpCode()) {
    return false;
  }
  bool isCall = false;
  switch (op) {
    case OP_dassign: {
      auto *dass1 = static_cast<DassignNode*>(stmt1);
      auto *dass2 = static_cast<DassignNode*>(stmt2);
      if (dass1->GetStIdx() != dass2->GetStIdx() ||
          dass1->GetFieldID() != dass2->GetFieldID()) {
        return false;
      }
      break;
    }
    case OP_dassignoff: {
      auto *dassoff1 = static_cast<DassignoffNode*>(stmt1);
      auto *dassoff2 = static_cast<DassignoffNode*>(stmt2);
      if (dassoff1->stIdx != dassoff2->stIdx ||
          dassoff1->offset != dassoff2->offset) {
        return false;
      }
      break;
    }
    case OP_iassign: {
      auto *iass1 = static_cast<IassignNode*>(stmt1);
      auto *iass2 = static_cast<IassignNode*>(stmt2);
      if (iass1->GetTyIdx() != iass2->GetTyIdx() ||
          iass1->GetFieldID() != iass2->GetFieldID()) {
        return false;
      }
      break;
    }
    case OP_iassignoff: {
      auto *iassoff1 = static_cast<IassignoffNode*>(stmt1);
      auto *iassoff2 = static_cast<IassignoffNode*>(stmt2);
      if (iassoff1->GetOffset() != iassoff2->GetOffset()) {
        return false;
      }
      break;
    }
    case OP_call:
    case OP_callassigned: {
      auto *call1 = static_cast<CallNode*>(stmt1);
      auto *call2 = static_cast<CallNode*>(stmt2);
      if (call1->GetPUIdx() != call2->GetPUIdx() ||
          call1->GetTyIdx() != call2->GetTyIdx() ||
          call1->GetCallReturnVector()->size() != call2->GetReturnVec().size()) {
        return false;
      }
      for (size_t i = 0; i < call1->GetCallReturnVector()->size(); ++i) {
        auto retPair1 = call1->GetReturnPair(i);
        auto retPair2 = call2->GetReturnPair(i);
        if (retPair1.first != retPair2.first ||
            retPair1.second.GetPregIdx() != retPair2.second.GetPregIdx() ||
            retPair1.second.GetFieldID() != retPair2.second.GetFieldID()) {
          return false;
        }
      }
      isCall = true;
      break;
    }
    case OP_icall:
    case OP_icallassigned:
    case OP_icallproto:
    case OP_icallprotoassigned: {
      auto *icall1 = static_cast<IcallNode*>(stmt1);
      auto *icall2 = static_cast<IcallNode*>(stmt2);
      if (icall1->GetRetTyIdx() != icall2->GetRetTyIdx() ||
          icall1->GetCallReturnVector()->size() != icall2->GetCallReturnVector()->size()) {
        return false;
      }
      if (icall1->GetCallReturnVector()->size() > 0) {
        auto retPair1 = icall1->GetCallReturnVector()->at(0);
        auto retPair2 = icall2->GetCallReturnVector()->at(0);
        if (retPair1.first != retPair2.first ||
            retPair1.second.GetPregIdx() != retPair2.second.GetPregIdx() ||
            retPair1.second.GetFieldID() != retPair2.second.GetFieldID()) {
          return false;
        }
      }
      isCall = true;
      break;
    }
    case OP_intrinsiccall:
    case OP_intrinsiccallassigned: {
      auto *call1 = static_cast<IntrinsiccallNode*>(stmt1);
      auto *call2 = static_cast<IntrinsiccallNode*>(stmt2);
      if (call1->GetIntrinsic() != call2->GetIntrinsic() ||
          call1->GetTyIdx() != call2->GetTyIdx() ||
          call1->GetCallReturnVector()->size() != call2->GetReturnVec().size()) {
        return false;
      }
      for (size_t i = 0; i < call1->GetCallReturnVector()->size(); ++i) {
        auto retPair1 = call1->GetCallReturnPair(i);
        auto retPair2 = call2->GetCallReturnPair(i);
        if (retPair1.first != retPair2.first ||
            retPair1.second.GetPregIdx() != retPair2.second.GetPregIdx() ||
            retPair1.second.GetFieldID() != retPair2.second.GetFieldID()) {
          return false;
        }
      }
      break;
    }
    default:
      return false;
  }

  // check number of operands
  if (stmt1->NumOpnds() != stmt2->NumOpnds()) {
    return false;
  }

  // check the operands
  for (size_t i = 0; i < stmt1->NumOpnds(); ++i) {
    auto *opnd1 = stmt1->Opnd(i);
    auto *opnd2 = stmt2->Opnd(i);
    if (isCall && IsConstOp(opnd1->GetOpCode())) {
      if (opnd1->GetOpCode() == opnd2->GetOpCode()) {
        continue;
      }
      return false;
    }
    if (!opnd1->IsSameContent(opnd2)) {
      return false;
    }
  }
  return true;
}

void FactoringOptimizer::SinkStmtsInCands(std::map<BB *, StmtNode *, cmpByBBID> &cands) {
  BB *succ = cands.begin()->first->GetSucc(0);
  BB *sinkBB = nullptr;
  if (succ->GetPred().size() == cands.size()) {
    sinkBB = succ;
  } else {
    sinkBB = cfg->NewBasicBlock();
    sinkBB->SetKind(kBBFallthru);
    sinkBB->SetAttributes(kBBAttrArtificial);
    for (auto &candPair : cands) {
      auto *pred = candPair.first;
      pred->ReplaceSucc(succ, sinkBB);
    }
    sinkBB->AddSucc(*succ);
    isCFGChanged = true;
  }
  auto *identifyStmt = cands.begin()->second;
  auto *sinkedStmt = identifyStmt->CloneTree(*codeMPAlloc);
  if (kOpcodeInfo.IsCall(identifyStmt->GetOpCode())) {
    for (size_t i = 0; i < identifyStmt->NumOpnds(); ++i) {
      auto *param = identifyStmt->Opnd(i);
      if (!IsConstOp(param->GetOpCode())) {
        continue;
      }
      // for different const parameter, use tmp reg to unify them
      bool diff = false;
      auto *identifyConst = param;
      for (auto &pair : cands) {
        auto *curNode = pair.second->Opnd(i);
        if (!curNode->IsSameContent(identifyConst)) {
          diff = true;
          break;
        }
      }
      if (!diff) {
        continue;
      }
      // create tmp param reg
      auto ptyp = param->GetPrimType();
      auto regIdx = func.GetMirFunc()->GetPregTab()->CreatePreg(ptyp);
      for (auto &pair : cands) {
        auto *origOpnd = pair.second->Opnd(i);
        auto *regass =
            mirBuilder->CreateStmtRegassign(ptyp, regIdx, origOpnd->CloneTree(*codeMPAlloc));
        // add const mov to bb's first so that we can continue to sink the new last stmt
        pair.first->PrependStmtNode(regass);
      }
      sinkedStmt->SetOpnd(mirBuilder->CreateExprRegread(ptyp, regIdx) ,i);
    }
  }
  if (dumpDetail) {
    LogInfo::MapleLogger() << "\nSink process" << sinkCount << ":\n";
    ++sinkCount;
  }
  for (auto &pair : cands) {
    auto *pred = pair.first;
    if (dumpDetail) {
      LogInfo::MapleLogger() << "\nstmt: \n";
      pair.second->Dump();
      LogInfo::MapleLogger() << "in BB:" << pred->GetBBId() << " removed.\n";
    }
    if (pred->IsGoto()) {
      pred->RemoveLastStmt();
      pred->SetKind(kBBFallthru);
    }
    pred->RemoveStmtNode(pair.second);
  }
  if (dumpDetail) {
    LogInfo::MapleLogger() << "\nnew stmt: \n";
    sinkedStmt->Dump();
    LogInfo::MapleLogger() << "added to BB:" << sinkBB->GetBBId() << ".\n";
  }
  sinkBB->PrependStmtNode(sinkedStmt);
}

// try to sink BB's last stmt to its succ, check if other preds have identical last stmt
void FactoringOptimizer::DoSink(BB *bb) {
  if (!bb || bb->IsEmpty() || bb->GetSucc().size() != 1) {
    return;
  }
  auto *identifyStmt = &bb->GetLast();
  while (identifyStmt &&
         (identifyStmt->GetOpCode() == OP_comment || identifyStmt->GetOpCode() == OP_goto)) {
    identifyStmt = identifyStmt->GetPrev();
  }
  if (!identifyStmt) {
    return;
  }
  auto *succ = bb->GetSucc(0);
  std::map<BB*, StmtNode*, cmpByBBID> cands;
  cands.emplace(bb, identifyStmt);
  // check other pred's last stmt
  for (auto *pred : succ->GetPred()) {
    if (pred == bb || pred->GetSucc().size() != 1 || pred->IsEmpty()) {
      continue;
    }
    auto *lastStmt = &pred->GetLast();
    while (lastStmt &&
           (lastStmt->GetOpCode() == OP_comment || lastStmt->GetOpCode() == OP_goto)) {
      lastStmt = lastStmt->GetPrev();
    }
    if (!lastStmt) {
      continue;
    }
    if (IsIdenticalStmt(lastStmt, identifyStmt)) {
      cands.emplace(pred, lastStmt);
    }
  }
  if (cands.size() < 2) {
    return;
  }
  SinkStmtsInCands(cands);
  // continue to sink new last stmt
  DoSink(bb);
}

void FactoringOptimizer::RunLocalFactoring() {
  if (func.IsEmpty()) {
    return;
  }
  for (auto *bb : cfg->GetAllBBs()) {
    DoSink(bb);
  }
}

void MECodeFactoring::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEMeCfg>();
  aDep.SetPreservedAll();
}

bool MECodeFactoring::PhaseRun(MeFunction &f) {
  auto *optimizer = GetPhaseAllocator()->New<FactoringOptimizer>(f, DEBUGFUNC_NEWPM(f));
  optimizer->RunLocalFactoring();
  if (optimizer->IsCFGChanged()) {
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &MEDominance::id);
  }
  return true;
}
}  // namespace maple
