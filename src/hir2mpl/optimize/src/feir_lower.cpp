/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "feir_lower.h"
#include "fe_function.h"
#include "feir_builder.h"

namespace maple {
FEIRLower::FEIRLower(FEFunction &funcIn) : func(funcIn) {
  Init();
}

void FEIRLower::Init() {
  lowerStmtHead = func.RegisterFEIRStmt(std::make_unique<FEIRStmt>(FEIRNodeKind::kStmtPesudoFuncStart));
  lowerStmtTail = func.RegisterFEIRStmt(std::make_unique<FEIRStmt>(FEIRNodeKind::kStmtPesudoFuncEnd));
  lowerStmtHead->SetNext(lowerStmtTail);
  lowerStmtTail->SetPrev(lowerStmtHead);
}

void FEIRLower::Clear() {
  auxFEIRStmtList.clear();
}

FEIRStmt *FEIRLower::RegisterAuxFEIRStmt(UniqueFEIRStmt stmt) {
  auxFEIRStmtList.push_back(std::move(stmt));
  return auxFEIRStmtList.back().get();
}

FEIRStmt *FEIRLower::CreateHeadAndTail() {
  FEIRStmt *head = RegisterAuxFEIRStmt(std::make_unique<FEIRStmt>(FEIRNodeKind::kStmtPesudoHead));
  FEIRStmt *tail = RegisterAuxFEIRStmt(std::make_unique<FEIRStmt>(FEIRNodeKind::kStmtPesudoTail));
  head->SetNext(tail);
  tail->SetPrev(head);
  return head;
}

FEIRStmt *FEIRLower::RegisterAndInsertFEIRStmt(UniqueFEIRStmt stmt, FEIRStmt *ptrTail,
                                               uint32 fileIdx, uint32 fileLine) {
  stmt->SetSrcFileInfo(fileIdx, fileLine);
  FEIRStmt *prtStmt = func.RegisterFEIRStmt(std::move(stmt));
  ptrTail->InsertBefore(prtStmt);
  return prtStmt;
}

void FEIRLower::LowerFunc() {
  FELinkListNode *nodeStmt = func.GetFEIRStmtHead()->GetNext();
  if (nodeStmt != func.GetFEIRStmtTail()) {
    LowerStmt(static_cast<FEIRStmt*>(nodeStmt), lowerStmtTail);
  }
  Clear();
}

void FEIRLower::LowerStmt(const std::list<UniqueFEIRStmt> &stmts, FEIRStmt *ptrTail) {
  FEIRStmt *tmpHead = CreateHeadAndTail();
  FEIRStmt *tmpTail = static_cast<FEIRStmt*>(tmpHead->GetNext());
  for (auto &stmt : stmts) {
    tmpTail->InsertBefore(stmt.get());
  }
  LowerStmt(static_cast<FEIRStmt*>(tmpHead->GetNext()), ptrTail);
}

void FEIRLower::LowerStmt(FEIRStmt *stmt, FEIRStmt *ptrTail) {
  FEIRStmt *nextStmt = stmt;
  do {
    stmt = nextStmt;
    nextStmt = static_cast<FEIRStmt*>(stmt->GetNext());
    switch (stmt->GetKind()) {
      case kStmtIf:
        LowerIfStmt(*static_cast<FEIRStmtIf*>(stmt), ptrTail);
        break;
      case kStmtPesudoTail:
      case kStmtPesudoFuncEnd:
        return;
      default:
        ptrTail->InsertBefore(stmt);
        break;
    }
  } while (nextStmt != nullptr);
}

void FEIRLower::LowerIfStmt(FEIRStmtIf &ifStmt, FEIRStmt *ptrTail) {
  FEIRStmt *thenHead = nullptr;
  FEIRStmt *thenTail = nullptr;
  FEIRStmt *elseHead = nullptr;
  FEIRStmt *elseTail = nullptr;
  if (!ifStmt.GetThenStmt().empty()) {
    thenHead = CreateHeadAndTail();
    thenTail = static_cast<FEIRStmt*>(thenHead->GetNext());
    LowerStmt(ifStmt.GetThenStmt(), thenTail);
  }
  if (!ifStmt.GetElseStmt().empty()) {
    elseHead = CreateHeadAndTail();
    elseTail = static_cast<FEIRStmt*>(elseHead->GetNext());
    LowerStmt(ifStmt.GetElseStmt(), elseTail);
  }
  if (ifStmt.GetThenStmt().empty() && ifStmt.GetElseStmt().empty()) {
    // eval <cond> statement
    std::list<UniqueFEIRExpr> feExprs;
    feExprs.emplace_back(ifStmt.GetCondExpr()->Clone());
    (void)RegisterAndInsertFEIRStmt(
        std::make_unique<FEIRStmtNary>(OP_eval, std::move(feExprs)),
        ptrTail, ifStmt.GetSrcFileIdx(), ifStmt.GetSrcFileLineNum());
  } else if (ifStmt.GetElseStmt().empty()) {
    // brfalse <cond> <endlabel>
    // <thenPart>
    // label <endlabel>
    CreateAndInsertCondStmt(OP_brfalse, ifStmt, thenHead, thenTail, ptrTail);
  } else if (ifStmt.GetThenStmt().empty()) {
    // brtrue <cond> <endlabel>
    // <elsePart>
    // label <endlabel>
    CreateAndInsertCondStmt(OP_brtrue, ifStmt, elseHead, elseTail, ptrTail);
  } else {
    // brfalse <cond> <elselabel>
    // <thenPart>
    // goto <endlabel>
    // label <elselabel>
    // <elsePart>
    // label <endlabel>
    std::string elseName = FEUtils::CreateLabelName();
    UniqueFEIRStmt condFEStmt = std::make_unique<FEIRStmtCondGotoForC>(
        ifStmt.GetCondExpr()->Clone(), OP_brfalse, elseName);
    auto condStmt = RegisterAndInsertFEIRStmt(
        std::move(condFEStmt), ptrTail, ifStmt.GetSrcFileIdx(), ifStmt.GetSrcFileLineNum());
    // <thenPart>
    FELinkListNode::SpliceNodes(thenHead, thenTail, ptrTail);
    // goto <endlabel>
    std::string endName = FEUtils::CreateLabelName();
    auto gotoStmt = RegisterAndInsertFEIRStmt(FEIRBuilder::CreateStmtGoto(endName), ptrTail);
    // label <elselabel>
    auto elseLabelStmt = RegisterAndInsertFEIRStmt(std::make_unique<FEIRStmtLabel>(elseName), ptrTail);
    // <elsePart>
    FELinkListNode::SpliceNodes(elseHead, elseTail, ptrTail);
    // label <endlabel>
    auto endLabelStmt = RegisterAndInsertFEIRStmt(std::make_unique<FEIRStmtLabel>(endName), ptrTail);
    // link bb
    condStmt->AddExtraSucc(*elseLabelStmt);
    elseLabelStmt->AddExtraPred(*condStmt);
    gotoStmt->AddExtraSucc(*endLabelStmt);
    endLabelStmt->AddExtraPred(*gotoStmt);
  }
}

void FEIRLower::CreateAndInsertCondStmt(Opcode op, FEIRStmtIf &ifStmt,
                                        FEIRStmt *head, FEIRStmt *tail, FEIRStmt *ptrTail) {
  std::string labelName = FEUtils::CreateLabelName();
  UniqueFEIRStmt condFEStmt = std::make_unique<FEIRStmtCondGotoForC>(ifStmt.GetCondExpr()->Clone(), op, labelName);
  FEIRStmt *condStmt = RegisterAndInsertFEIRStmt(
      std::move(condFEStmt), ptrTail, ifStmt.GetSrcFileIdx(), ifStmt.GetSrcFileLineNum());
  FELinkListNode::SpliceNodes(head, tail, ptrTail);
  FEIRStmt *labelStmt = RegisterAndInsertFEIRStmt(std::make_unique<FEIRStmtLabel>(labelName), ptrTail);
  // link bb
  condStmt->AddExtraSucc(*labelStmt);
  labelStmt->AddExtraPred(*condStmt);
}
}  // namespace maple