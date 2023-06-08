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

FEIRStmt *FEIRLower::RegisterAndInsertFEIRStmt(UniqueFEIRStmt stmt, FEIRStmt *ptrTail, const Loc loc) {
  stmt->SetSrcLoc(loc);
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
      case kStmtDoWhile:
        ProcessLoopStmt(*static_cast<FEIRStmtDoWhile*>(stmt), ptrTail);
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
        std::make_unique<FEIRStmtNary>(OP_eval, std::move(feExprs)), ptrTail, ifStmt.GetSrcLoc());
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
    auto condStmt = RegisterAndInsertFEIRStmt(std::move(condFEStmt), ptrTail, ifStmt.GetSrcLoc());
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

// for/dowhile/while stmts
void FEIRLower::ProcessLoopStmt(const FEIRStmtDoWhile &stmt, FEIRStmt *ptrTail) {
  FEIRStmt *bodyHead = nullptr;
  FEIRStmt *bodyTail = nullptr;
  if (!stmt.GetBodyStmts().empty()) {
    bodyHead = CreateHeadAndTail();
    bodyTail = static_cast<FEIRStmt*>(bodyHead->GetNext());
    LowerStmt(stmt.GetBodyStmts(), bodyTail);
  }
  // for/while
  if (stmt.GetOpcode() == OP_while) {
    return LowerWhileStmt(stmt, bodyHead, bodyTail, ptrTail);
  }
  // dowhile
  if (stmt.GetOpcode() == OP_dowhile) {
    return LowerDoWhileStmt(stmt, bodyHead, bodyTail, ptrTail);
  }
}

/*
 * while <cond> <body>
 * is lowered to :
 * label <whilelabel>
 * brfalse <cond> <endlabel>
 * <body>
 * goto <whilelabel>
 * label <endlabel>
 */
void FEIRLower::LowerWhileStmt(const FEIRStmtDoWhile &whileStmt, FEIRStmt *bodyHead,
                               FEIRStmt *bodyTail, FEIRStmt *ptrTail) {
  std::string whileLabelName = FEUtils::CreateLabelName();
  // label <whilelabel>
  auto whileLabelStmt = RegisterAndInsertFEIRStmt(std::make_unique<FEIRStmtLabel>(whileLabelName), ptrTail);
  std::string endLabelName = FEUtils::CreateLabelName();
  // brfalse <cond> <endlabel>
  UniqueFEIRStmt condFEStmt = std::make_unique<FEIRStmtCondGotoForC>(
      whileStmt.GetCondExpr()->Clone(), OP_brfalse, endLabelName);
  auto condStmt = RegisterAndInsertFEIRStmt(std::move(condFEStmt), ptrTail, whileStmt.GetSrcLoc());
  if (bodyHead != nullptr && bodyTail != nullptr) {
    // <body>
    FELinkListNode::SpliceNodes(bodyHead, bodyTail, ptrTail);
  }
  // goto <whilelabel>
  auto gotoStmt = RegisterAndInsertFEIRStmt(FEIRBuilder::CreateStmtGoto(whileLabelName), ptrTail);
  // label <endlabel>
  auto endLabelStmt = RegisterAndInsertFEIRStmt(std::make_unique<FEIRStmtLabel>(endLabelName), ptrTail);
  // link bb
  condStmt->AddExtraSucc(*endLabelStmt);
  endLabelStmt->AddExtraPred(*condStmt);
  gotoStmt->AddExtraSucc(*whileLabelStmt);
  whileLabelStmt->AddExtraPred(*gotoStmt);
}

/*
 * dowhile <body> <cond>
 * is lowered to:
 * label <bodylabel>
 * <body>
 * brtrue <cond> <bodylabel>
 */
void FEIRLower::LowerDoWhileStmt(const FEIRStmtDoWhile &doWhileStmt, FEIRStmt *bodyHead,
                                 FEIRStmt *bodyTail, FEIRStmt *ptrTail) {
  std::string bodyLabelName = FEUtils::CreateLabelName();
  // label <bodylabel>
  auto bodyLabelStmt = RegisterAndInsertFEIRStmt(std::make_unique<FEIRStmtLabel>(bodyLabelName), ptrTail);
  if (bodyHead != nullptr && bodyTail != nullptr) {
    // <body>
    FELinkListNode::SpliceNodes(bodyHead, bodyTail, ptrTail);
  }
  // brtrue <cond> <bodylabel>
  UniqueFEIRStmt condFEStmt = std::make_unique<FEIRStmtCondGotoForC>(
      doWhileStmt.GetCondExpr()->Clone(), OP_brtrue, bodyLabelName);
  auto condStmt = RegisterAndInsertFEIRStmt(std::move(condFEStmt), ptrTail, doWhileStmt.GetSrcLoc());
  // link bb
  condStmt->AddExtraSucc(*bodyLabelStmt);
  bodyLabelStmt->AddExtraPred(*condStmt);
}

void FEIRLower::CreateAndInsertCondStmt(Opcode op, const FEIRStmtIf &ifStmt,
                                        FEIRStmt *head, FEIRStmt *tail, FEIRStmt *ptrTail) {
  std::string labelName = FEUtils::CreateLabelName();
  UniqueFEIRStmt condFEStmt = std::make_unique<FEIRStmtCondGotoForC>(ifStmt.GetCondExpr()->Clone(), op, labelName);
  FEIRStmt *condStmt = RegisterAndInsertFEIRStmt(std::move(condFEStmt), ptrTail, ifStmt.GetSrcLoc());
  FELinkListNode::SpliceNodes(head, tail, ptrTail);
  FEIRStmt *labelStmt = RegisterAndInsertFEIRStmt(std::make_unique<FEIRStmtLabel>(labelName), ptrTail);
  // link bb
  condStmt->AddExtraSucc(*labelStmt);
  labelStmt->AddExtraPred(*condStmt);
}
}  // namespace maple