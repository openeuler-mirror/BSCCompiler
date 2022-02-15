/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "bb.h"
#include "mempool_allocator.h"
#include "ver_symbol.h"
#include "me_ssa.h"
#include "me_ir.h"

namespace maple {
std::string BB::StrAttribute() const {
  switch (kind) {
    case kBBUnknown:
      return "unknown";
    case kBBCondGoto:
      return "condgoto";
    case kBBGoto:
      return "goto";
    case kBBFallthru:
      return "fallthru";
    case kBBReturn:
      return "return";
    case kBBAfterGosub:
      return "aftergosub";
    case kBBSwitch:
      return "switch";
    case kBBIgoto:
      return "igoto";
    case kBBInvalid:
      return "invalid";
    default:
      CHECK_FATAL(false, "not yet implemented");
  }
  return "NYI";
}

void BB::DumpBBAttribute(const MIRModule *mod) const {
  if (GetAttributes(kBBAttrIsEntry)) {
    mod->GetOut() << " Entry ";
  }
  if (GetAttributes(kBBAttrIsExit)) {
    mod->GetOut() << " Exit ";
  }
  if (GetAttributes(kBBAttrWontExit)) {
    mod->GetOut() << " WontExit ";
  }
  if (GetAttributes(kBBAttrIsTry)) {
    mod->GetOut() << " Try ";
  }
  if (GetAttributes(kBBAttrIsTryEnd)) {
    mod->GetOut() << " Tryend ";
  }
  if (GetAttributes(kBBAttrIsJSCatch)) {
    mod->GetOut() << " JSCatch ";
  }
  if (GetAttributes(kBBAttrIsJSFinally)) {
    mod->GetOut() << " JSFinally ";
  }
  if (GetAttributes(kBBAttrIsJavaFinally)) {
    mod->GetOut() << " Catch::Finally ";
  } else if (GetAttributes(kBBAttrIsCatch)) {
    mod->GetOut() << " Catch ";
  }
  if (GetAttributes(kBBAttrIsInLoop)) {
    mod->GetOut() << " InLoop ";
  }
}

void BB::DumpHeader(const MIRModule *mod) const {
  mod->GetOut() << "============BB id:" << GetBBId() << " " << StrAttribute() << " [";
  DumpBBAttribute(mod);
  mod->GetOut() << "]===============\n";
  mod->GetOut() << "preds: ";
  for (const auto &predElement : pred) {
    mod->GetOut() << predElement->GetBBId() << " ";
  }
  mod->GetOut() << "\nsuccs: ";
  for (const auto &succElement : succ) {
    mod->GetOut() << succElement->GetBBId() << " ";
  }
  mod->GetOut() << '\n';
  if (bbLabel != 0) {
    LabelNode lblNode;
    lblNode.SetLabelIdx(bbLabel);
    lblNode.Dump(0);
    mod->GetOut() << '\n';
  }
}

void BB::Dump(const MIRModule *mod) {
  DumpHeader(mod);
  DumpPhi();
  for (auto &stmt : stmtNodeList) {
    stmt.Dump(1);
  }
}

void BB::DumpPhi() {
  for (auto &phi : phiList) {
    phi.second.Dump();
  }
}

const PhiNode *BB::PhiofVerStInserted(const VersionSt &versionSt) const {
  auto phiIt = phiList.find(versionSt.GetOst()->GetIndex());
  return (phiIt != phiList.end()) ? &(phiIt->second) : nullptr;
}

void BB::InsertPhi(MapleAllocator *alloc, VersionSt *versionSt) {
  PhiNode phiNode(*alloc, *versionSt);
  for (auto prevIt = pred.begin(); prevIt != pred.end(); ++prevIt) {
    phiNode.GetPhiOpnds().push_back(versionSt);
  }
  (void)phiList.insert(std::make_pair(versionSt->GetOst()->GetIndex(), phiNode));
}

bool BB::IsInList(const MapleVector<BB*> &bbList) const {
  for (const auto &bb : bbList) {
    if (bb == this) {
      return true;
    }
  }
  return false;
}

// remove the given bb from the BB vector bbVec; nothing done if not found
int BB::RemoveBBFromVector(MapleVector<BB*> &bbVec) const {
  size_t i = 0;
  while (i < bbVec.size()) {
    if (bbVec[i] == this) {
      break;
    }
    ++i;
  }
  if (i == bbVec.size()) {
    // bb not in the vector
    return -1;
  }
  bbVec.erase(bbVec.cbegin() + i);
  return i;
}

void BB::RemovePhiOpnd(int index) {
  for (auto &phi : phiList) {
    ASSERT(phi.second.GetPhiOpnds().size() > index, "index out of range in BB::RemovePhiOpnd");
    phi.second.GetPhiOpnds().erase(phi.second.GetPhiOpnds().cbegin() + index);
  }
  for (auto &phi : mePhiList) {
    ASSERT(phi.second->GetOpnds().size() > index, "index out of range in BB::RemovePhiOpnd");
    phi.second->GetOpnds().erase(phi.second->GetOpnds().cbegin() + index);
  }
}

void BB::RemoveBBFromPred(const BB &bb, bool updatePhi) {
  int index = bb.RemoveBBFromVector(pred);
  ASSERT(index != -1, "-1 is a very large number in BB::RemoveBBFromPred");
  if (updatePhi) {
    RemovePhiOpnd(index);
  }
}

void BB::RemoveBBFromSucc(const BB &bb) {
  int ret = bb.RemoveBBFromVector(succ);
  if (ret != -1 && frequency != 0 && !succFreq.empty()) {
    succFreq.erase(succFreq.cbegin() + ret);
  }
}

int BB::GetPredIndex(const BB &predBB) const {
  size_t i = 0;
  while (i < pred.size()) {
    if (pred[i] == &predBB) {
      return static_cast<int>(i);
    }
    ++i;
  }
  return -1;
}

int BB::GetSuccIndex(const BB &succBB) const {
  size_t i = 0;
  while (i < succ.size()) {
    if (succ[i] == &succBB) {
      return static_cast<int>(i);
    }
    ++i;
  }
  // bb not in the vector
  return -1;
}

// add stmtnode to bb and update first_stmt_ and last_stmt_
void BB::AddStmtNode(StmtNode *stmt) {
  CHECK_FATAL(stmt != nullptr, "null ptr check");
  stmtNodeList.push_back(stmt);
}

// add stmtnode at beginning of bb and update first_stmt_ and last_stmt_
void BB::PrependStmtNode(StmtNode *stmt) {
  stmtNodeList.push_front(stmt);
}

void BB::PrependMeStmt(MeStmt *meStmt) {
  meStmtList.push_front(meStmt);
  meStmt->SetBB(this);
}

// if the bb contains only one stmt other than comment, return that stmt
// otherwise return nullptr
StmtNode *BB::GetTheOnlyStmtNode() {
  StmtNode *onlyStmtNode = nullptr;
  for (auto &stmtNode : stmtNodeList) {
    if (stmtNode.GetOpCode() == OP_comment) {
      continue;
    }
    if (onlyStmtNode != nullptr) {
      return nullptr;
    }
    onlyStmtNode = &stmtNode;
  }
  return onlyStmtNode;
}

// warning: if stmt is not in the bb, this will cause undefined behavior
void BB::RemoveStmtNode(StmtNode *stmt) {
  CHECK_FATAL(stmt != nullptr, "null ptr check");
  stmtNodeList.erase(StmtNodes::iterator(stmt));
}

void BB::InsertStmtBefore(const StmtNode *stmt, StmtNode *newStmt) {
  CHECK_FATAL(newStmt != nullptr, "null ptr check");
  CHECK_FATAL(stmt != nullptr, "null ptr check");
  stmtNodeList.insert(stmt, newStmt);
}

void BB::ReplaceStmt(StmtNode *stmt, StmtNode *newStmt) {
  InsertStmtBefore(stmt, newStmt);
  RemoveStmtNode(stmt);
}

// delete last_stmt_ and update
void BB::RemoveLastStmt() {
  stmtNodeList.pop_back();
}

// replace pred with newbb to keep position and add this as succ of newpred
// and remove itself from succ of old
void BB::ReplacePred(const BB *old, BB *newPred) {
  ASSERT((old != nullptr && newPred != nullptr), "Nullptr check.");
  ASSERT((old->IsInList(pred) && IsInList(old->succ)), "Nullptr check.");
  for (auto &predElement : pred) {
    if (predElement == old) {
      predElement->RemoveBBFromSucc(*this);
      if (IsInList(newPred->succ)){
        RemoveBBFromPred(*predElement, true);
      } else {
        newPred->succ.push_back(this);
        predElement = newPred;
      }
      break;
    }
  }
}

// replace myself with a new succ in all my predecessors
void BB::MoveAllPredToSucc(BB *newSucc, BB *commonEntry) {
  ASSERT_NOT_NULL(newSucc);
  if (GetAttributes(kBBAttrIsEntry)) {
    ASSERT(IsSuccBB(*commonEntry), "BB is not in commonEntry's successors, but it is set kBBAttrIsEntry");
    commonEntry->RemoveEntry(*this);
    commonEntry->AddEntry(*newSucc);
    newSucc->SetAttributes(kBBAttrIsEntry);
  } else {
    while (!GetPred().empty()) {
      BB *firstPred = GetPred(0);
      if (IsSuccBB(*firstPred)) {                    // avoid replacing twice
        firstPred->ReplaceSucc(this, newSucc, true); // firstPred will be removed from this->pred
      }
    }
  }
}

// replace succ in current position with newsucc and add this as pred of newsucc
// and remove itself from pred of old
void BB::ReplaceSucc(const BB *old, BB *newSucc, bool updatePhi) {
  ASSERT((old != nullptr && newSucc != nullptr), "Nullptr check.");
  ASSERT((old->IsInList(succ) && IsInList(old->pred)), "Nullptr check.");
  for (auto &succElement : succ) {
    if (succElement == old) {
      succElement->RemoveBBFromPred(*this, updatePhi);
      if (IsInList(newSucc->pred)) {
        RemoveBBFromSucc(*succElement);
      } else {
        newSucc->pred.push_back(this);
        succElement = newSucc;
      }
      break;
    }
  }
}

// replace myself with a new pred in all my successors
void BB::MoveAllSuccToPred(BB *newPred, BB *commonExit) {
  ASSERT_NOT_NULL(newPred);
  if (GetAttributes(kBBAttrIsExit)) {
    ASSERT(IsPredBB(*commonExit), "BB is not in commonExit's predecessors, but it is set kBBAttrIsExit");
    commonExit->RemoveExit(*this);
    commonExit->AddExit(*newPred);
    newPred->SetAttributes(kBBAttrIsExit);
  } else {
    while (!GetSucc().empty()) {
      BB *firstSucc = GetSucc(0);
      if (IsPredBB(*firstSucc)) {              // avoid replacing twice
        firstSucc->ReplacePred(this, newPred); // firstSucc will be removed from this->succ
      }
    }
  }
}

void BB::FindReachableBBs(std::vector<bool> &visitedBBs) const {
  CHECK_FATAL(GetBBId() < visitedBBs.size(), "out of range in BB::FindReachableBBs");
  if (visitedBBs[GetBBId()]) {
    return;
  }
  visitedBBs[GetBBId()] = true;
  for (const BB *bb : succ) {
    bb->FindReachableBBs(visitedBBs);
  }
}

void BB::FindWillExitBBs(std::vector<bool> &visitedBBs) const {
  CHECK_FATAL(GetBBId() < visitedBBs.size(), "out of range in BB::FindReachableBBs");
  if (visitedBBs[GetBBId()]) {
    return;
  }
  visitedBBs[GetBBId()] = true;
  for (const BB *bb : pred) {
    bb->FindWillExitBBs(visitedBBs);
  }
}

void BB::SetFirstMe(MeStmt *stmt) {
  meStmtList.update_front(stmt);
}

void BB::SetLastMe(MeStmt *stmt) {
  meStmtList.update_back(stmt);
}

MeStmt *BB::GetFirstMe() {
  return meStmtList.empty() ? nullptr : &meStmtList.front();
}

const MeStmt *BB::GetFirstMe() const {
  return meStmtList.empty() ? nullptr : &meStmtList.front();
}

MeStmt *BB::GetLastMe() {
  return meStmtList.empty() ? nullptr : &meStmtList.back();
}

const MeStmt *BB::GetLastMe() const {
  return meStmtList.empty() ? nullptr : &meStmtList.back();
}

void BB::RemoveLastMeStmt() {
  meStmtList.pop_back();
}

void BB::RemoveMeStmt(MeStmt *meStmt) {
  CHECK_FATAL(meStmt != nullptr, "null ptr check");
  meStmtList.erase(meStmt);
}

void BB::AddMeStmtFirst(MeStmt *meStmt) {
  CHECK_FATAL(meStmt != nullptr, "null ptr check");
  meStmtList.push_front(meStmt);
  meStmt->SetBB(this);
}

void BB::AddMeStmtLast(MeStmt *meStmt) {
  CHECK_FATAL(meStmt != nullptr, "null ptr check");
  meStmtList.push_back(meStmt);
  meStmt->SetBB(this);
}

void BB::InsertMeStmtBefore(const MeStmt *meStmt, MeStmt *inStmt) {
  CHECK_FATAL(meStmt != nullptr, "null ptr check");
  CHECK_FATAL(inStmt != nullptr, "null ptr check");
  meStmtList.insert(meStmt, inStmt);
  inStmt->SetBB(this);
}

void BB::InsertMeStmtAfter(const MeStmt *meStmt, MeStmt *inStmt) {
  meStmtList.insertAfter(meStmt, inStmt);
  inStmt->SetBB(this);
}

// insert instmt before br to the last of bb
void BB::InsertMeStmtLastBr(MeStmt *inStmt) {
  if (IsMeStmtEmpty()) {
    AddMeStmtLast(inStmt);
    return;
  }
  MeStmt *brStmt = meStmtList.rbegin().base().d();
  Opcode op = brStmt->GetOp();
  if (brStmt->IsCondBr() || op == OP_goto || op == OP_switch || op == OP_throw || op == OP_return || op == OP_gosub ||
      op == OP_retsub || op == OP_igoto) {
    InsertMeStmtBefore(brStmt, inStmt);
  } else {
    AddMeStmtLast(inStmt);
  }
}

void BB::ReplaceMeStmt(MeStmt *stmt, MeStmt *newStmt) {
  InsertMeStmtBefore(stmt, newStmt);
  RemoveMeStmt(stmt);
}

void BB::DumpMeBB(const IRMap &irMap) {
  for (MeStmt &meStmt : GetMeStmts()) {
    meStmt.Dump(&irMap);
  }
}

void BB::DumpMeVarPiList(const IRMap *irMap) {
  if (meVarPiList.empty()) {
    return;
  }
  std::cout << "<<<<<<<<<<<<<<  PI Node Start >>>>>>>>>>>>>>>>>>\n";
  for (const auto &pair : meVarPiList) {
    BB *bb = pair.first;
    std::cout << "Frome BB : " << bb->GetBBId() << '\n';
    for (const auto *stmt : pair.second) {
      stmt->Dump(irMap);
    }
  }
  std::cout << "<<<<<<<<<<<<<<  PI Node End >>>>>>>>>>>>>>>>>>>>\n";
}

void BB::DumpMePhiList(const IRMap *irMap) {
  int count = 0;
  for (const auto &phi : mePhiList) {
    phi.second->Dump(irMap);
    int dumpVsyNum = DumpOptions::GetDumpVsyNum();
    if (dumpVsyNum > 0 && ++count >= dumpVsyNum) {
      break;
    }
    ASSERT(count >= 0, "mePhiList too large");
  }
}

void SCCOfBBs::Dump() {
  std::cout << "SCC " << id << " contains\n";
  for (BB *bb : bbs) {
    std::cout << "bb(" << bb->UintID() << ")  ";
  }
  std::cout << '\n';
}

bool SCCOfBBs::HasCycle() const {
  CHECK_FATAL(!bbs.empty(), "should have bbs in the scc");
  if (bbs.size() > 1) {
    return true;
  }
  BB *bb = bbs[0];
  for (const BB *succ : bb->GetSucc()) {
    if (succ == bb) {
      return true;
    }
  }
  return false;
}

void SCCOfBBs::Verify(MapleVector<SCCOfBBs*> &sccOfBB) {
  CHECK_FATAL(!bbs.empty(), "should have bbs in the scc");
  for (const BB *bb : bbs) {
    SCCOfBBs *scc = sccOfBB.at(bb->UintID());
    CHECK_FATAL(scc == this, "");
  }
}

void SCCOfBBs::SetUp(MapleVector<SCCOfBBs*> &sccOfBB) {
  for (BB *bb : bbs) {
    for (BB *succ : bb->GetSucc()) {
      if (succ == nullptr || sccOfBB.at(succ->UintID()) == this) {
        continue;
      }
      succSCC.insert(sccOfBB[succ->UintID()]);
    }

    for (BB *pred : bb->GetPred()) {
      if (pred == nullptr || sccOfBB.at(pred->UintID()) == this) {
        continue;
      }
      predSCC.insert(sccOfBB[pred->UintID()]);
    }
  }
}

bool ControlFlowInInfiniteLoop(const BB &bb, Opcode opcode) {
  switch (opcode) {
    // goto always return true
    case OP_goto:
    case OP_igoto:
      return true;
    case OP_brtrue:
    case OP_brfalse:
    case OP_switch:
      return bb.GetAttributes(kBBAttrWontExit);
    default:
      return false;
  }
}
}  // namespace maple
