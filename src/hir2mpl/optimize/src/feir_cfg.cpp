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
#include "feir_cfg.h"
#include "mpl_logging.h"

namespace maple {
FEIRCFG::FEIRCFG(FEIRStmt *argStmtHead, FEIRStmt *argStmtTail)
    : stmtHead(argStmtHead), stmtTail(argStmtTail) {
  (void)stmtTail;
}

void FEIRCFG::Init() {
  bbHead = std::make_unique<FEIRBB>(kBBKindPesudoHead);
  bbTail = std::make_unique<FEIRBB>(kBBKindPesudoTail);
  bbHead->SetNext(bbTail.get());
  bbTail->SetPrev(bbHead.get());
}

void FEIRCFG::BuildBB() {
  FELinkListNode *nodeStmt = stmtHead->GetNext();
  FEIRBB *currBB = nullptr;
  while (nodeStmt != nullptr) {
    FEIRStmt *stmt = static_cast<FEIRStmt*>(nodeStmt);
    if (!stmt->IsAux()) {
      // check start of BB
      if (currBB == nullptr) { // Additional conditions need to be added
        currBB = NewBBAppend();
        bbTail->InsertBefore(currBB);
      }
      CHECK_FATAL(currBB != nullptr, "nullptr check of currBB");
      currBB->AppendStmt(stmt);
      // check end of BB
      if (!stmt->IsFallThru()) { // Additional conditions need to be added
        currBB = nullptr;
      }
    }
    nodeStmt = nodeStmt->GetNext();
  }

  AppendAuxStmt();
}

void FEIRCFG::AppendAuxStmt() {
  FELinkListNode *nodeBB = bbHead->GetNext();
  while (nodeBB != nullptr && nodeBB != bbTail.get()) {
    FEIRBB *bb = static_cast<FEIRBB*>(nodeBB);
    // add pre
    FELinkListNode *nodeStmt = bb->GetStmtHead()->GetPrev();
    while (nodeStmt != nullptr) {
      FEIRStmt *stmt = static_cast<FEIRStmt*>(nodeStmt);
      if (stmt->IsAuxPre()) {
        bb->AddStmtAuxPre(stmt);
      } else {
        break;
      }
      nodeStmt = nodeStmt->GetPrev();
    }
    // add post
    nodeStmt = bb->GetStmtTail()->GetNext();
    while (nodeStmt != nullptr) {
      FEIRStmt *stmt = static_cast<FEIRStmt*>(nodeStmt);
      if (stmt->IsAuxPost()) {
        bb->AddStmtAuxPost(stmt);
      } else {
        break;
      }
      nodeStmt = nodeStmt->GetNext();
    }
    nodeBB = nodeBB->GetNext();
  }
}

FEIRBB *FEIRCFG::NewBBAppend() {
  std::unique_ptr<FEIRBB> bbNew = NewFEIRBB();
  ASSERT(bbNew != nullptr, "nullptr check for bbNew");
  listBB.push_back(std::move(bbNew));
  return listBB.back().get();
}

bool FEIRCFG::BuildCFG() {
  // build target map
  std::map<const FEIRStmt*, FEIRBB*> mapTargetStmtBB;
  FELinkListNode *nodeBB = bbHead->GetNext();
  while (nodeBB != nullptr && nodeBB != bbTail.get()) {
    FEIRBB *bb = static_cast<FEIRBB*>(nodeBB);
    const FEIRStmt *locStmtHead = bb->GetStmtNoAuxHead();
    if (locStmtHead != nullptr) { // Additional conditions need to be added
      mapTargetStmtBB[locStmtHead] = bb;
    }
    nodeBB = nodeBB->GetNext();
  }
  // link
  nodeBB = bbHead->GetNext();
  bool firstBB = true;
  while (nodeBB != nullptr && nodeBB != bbTail.get()) {
    FEIRBB *bb = static_cast<FEIRBB*>(nodeBB);
    if (firstBB) {
      bb->AddPredBB(bbHead.get());
      bbHead->AddSuccBB(bb);
      firstBB = false;
    }
    const FEIRStmt *locStmtTail = bb->GetStmtNoAuxTail();
    CHECK_FATAL(locStmtTail != nullptr, "stmt tail is nullptr");
    if (locStmtTail->IsFallThru()) {
      FELinkListNode *nodeBBNext = nodeBB->GetNext();
      if (nodeBBNext == nullptr || nodeBBNext == bbTail.get()) {
        ERR(kLncErr, "Method without return");
        return false;
      }
      FEIRBB *bbNext = static_cast<FEIRBB*>(nodeBBNext);
      bb->AddSuccBB(bbNext);
      bbNext->AddPredBB(bb);
    }
    for (FEIRStmt *stmt : locStmtTail->GetSuccs()) {
      auto itBB = mapTargetStmtBB.find(stmt);
      CHECK_FATAL(itBB != mapTargetStmtBB.end(), "Target BB is not found");
      FEIRBB *bbNext = itBB->second;
      bb->AddSuccBB(bbNext);
      bbNext->AddPredBB(bb);
    }
    nodeBB = nodeBB->GetNext();
  }
  return true;
}

const FEIRBB *FEIRCFG::GetHeadBB() {
  currBBNode = bbHead->GetNext();
  if (currBBNode == bbTail.get()) {
    return nullptr;
  }
  return static_cast<FEIRBB*>(currBBNode);
}

const FEIRBB *FEIRCFG::GetNextBB() {
  currBBNode = currBBNode->GetNext();
  if (currBBNode == bbTail.get()) {
    return nullptr;
  }
  return static_cast<FEIRBB*>(currBBNode);
}
}  // namespace maple