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
#include <fstream>
#include "mpl_logging.h"

namespace maple {
FEIRCFG::FEIRCFG(FEIRStmt *argStmtHead, FEIRStmt *argStmtTail)
    : stmtHead(argStmtHead), stmtTail(argStmtTail) {}

void FEIRCFG::Init() {
  bbHead = std::make_unique<FEIRBB>(kBBKindPesudoHead);
  bbTail = std::make_unique<FEIRBB>(kBBKindPesudoTail);
  bbHead->SetNext(bbTail.get());
  bbTail->SetPrev(bbHead.get());
}

void FEIRCFG::GenerateCFG() {
  if (isGeneratedCFG) {
    return;
  }
  Init();
  LabelStmtID();
  BuildBB();
  BuildCFG();
  LabelBBID();
}

void FEIRCFG::BuildBB() {
  FELinkListNode *nodeStmt = stmtHead->GetNext();
  FEIRBB *currBB = nullptr;
  while (nodeStmt != nullptr && nodeStmt != stmtTail) {
    FEIRStmt *stmt = static_cast<FEIRStmt*>(nodeStmt);
    if (!stmt->IsAux()) {
      // check start of BB
      if (currBB == nullptr || !stmt->GetExtraPreds().empty()) {
        currBB = NewBBAppend();
        bbTail->InsertBefore(currBB);
      }
      CHECK_FATAL(currBB != nullptr, "nullptr check of currBB");
      currBB->AppendStmt(stmt);
      // check end of BB
      if (!stmt->IsFallThru() || !stmt->GetExtraSuccs().empty()) {
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
    for (FEIRStmt *stmt : locStmtTail->GetExtraSuccs()) {
      auto itBB = mapTargetStmtBB.find(stmt);
      CHECK_FATAL(itBB != mapTargetStmtBB.end(), "Target BB is not found");
      FEIRBB *bbNext = itBB->second;
      bb->AddSuccBB(bbNext);
      bbNext->AddPredBB(bb);
    }
    nodeBB = nodeBB->GetNext();
  }
  isGeneratedCFG = true;
  return isGeneratedCFG;
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

void FEIRCFG::LabelStmtID() {
  FELinkListNode *nodeStmt = stmtHead;
  uint32 idx = 0;
  while (nodeStmt != nullptr) {
    FEIRStmt *stmt = static_cast<FEIRStmt*>(nodeStmt);
    stmt->SetID(idx);
    idx++;
    nodeStmt = nodeStmt->GetNext();
  }
}

void FEIRCFG::LabelBBID() {
  FELinkListNode *nodeBB = bbHead.get();
  uint32 idx = 0;
  while (nodeBB != nullptr) {
    FEIRBB *bb = static_cast<FEIRBB*>(nodeBB);
    bb->SetID(idx);
    idx++;
    nodeBB = nodeBB->GetNext();
  }
}

bool FEIRCFG::HasDeadBB() const {
  FELinkListNode *nodeBB = bbHead->GetNext();
  while (nodeBB != nullptr && nodeBB != bbTail.get()) {
    FEIRBB *bb = static_cast<FEIRBB*>(nodeBB);
    if (bb->IsDead()) {
      return true;
    }
    nodeBB = nodeBB->GetNext();
  }
  return false;
}

void FEIRCFG::DumpBBs() {
  FELinkListNode *nodeBB = bbHead->GetNext();
  while (nodeBB != nullptr && nodeBB != bbTail.get()) {
    FEIRBB *bb = static_cast<FEIRBB*>(nodeBB);
    bb->Dump();
    nodeBB = nodeBB->GetNext();
  }
}

void FEIRCFG::DumpCFGGraph(std::ofstream &file) {
  FELinkListNode *nodeBB = bbHead->GetNext();
  while (nodeBB != nullptr && nodeBB != bbTail.get()) {
    FEIRBB *bb = static_cast<FEIRBB*>(nodeBB);
    DumpCFGGraphForBB(file, *bb);
    nodeBB = nodeBB->GetNext();
  }
  DumpCFGGraphForEdge(file);
  file << "}" << std::endl;
}

void FEIRCFG::DumpCFGGraphForBB(std::ofstream &file, const FEIRBB &bb) {
  file << "  BB" << bb.GetID() << " [shape=record,label=\"{\n";
  const FELinkListNode *nodeStmt = bb.GetStmtHead();
  while (nodeStmt != nullptr) {
    const FEIRStmt *stmt = static_cast<const FEIRStmt*>(nodeStmt);
    file << "      " << stmt->DumpDotString();
    if (nodeStmt == bb.GetStmtTail()) {
      file << "\n";
      break;
    } else {
      file << " |\n";
    }
    nodeStmt = nodeStmt->GetNext();
  }
  file << "    }\"];\n";
}

void FEIRCFG::DumpCFGGraphForEdge(std::ofstream &file) {
  file << "  subgraph cfg_edges {\n";
  file << "    edge [color=\"#000000\",weight=0.3,len=3];\n";
  const FELinkListNode *nodeBB = bbHead->GetNext();
  while (nodeBB != nullptr && nodeBB != bbTail.get()) {
    const FEIRBB *bb = static_cast<const FEIRBB*>(nodeBB);
    const FEIRStmt *stmtS = bb->GetStmtTail();
    for (FEIRBB *bbNext : bb->GetSuccBBs()) {
      const FEIRStmt *stmtE = bbNext->GetStmtHead();
      file << "    BB" << bb->GetID() << ":stmt" << stmtS->GetID() << " -> ";
      file << "BB" << bbNext->GetID() << ":stmt" << stmtE->GetID() << "\n";
    }
    nodeBB = nodeBB->GetNext();
  }
  file << "  }\n";
}
}  // namespace maple
