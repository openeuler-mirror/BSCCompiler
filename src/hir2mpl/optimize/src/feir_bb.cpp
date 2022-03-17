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
#include "feir_bb.h"

namespace maple {
FEIRBB::FEIRBB(uint8 argKind)
    : kind(argKind),
      id(0),
      stmtHead(nullptr),
      stmtTail(nullptr),
      stmtNoAuxHead(nullptr),
      stmtNoAuxTail(nullptr) {}

FEIRBB::FEIRBB()
    : FEIRBB(FEIRBBKind::kBBKindDefault) {}

FEIRBB::~FEIRBB() {
  stmtHead = nullptr;
  stmtTail = nullptr;
  stmtNoAuxHead = nullptr;
  stmtNoAuxTail = nullptr;
}

void FEIRBB::AppendStmt(FEIRStmt *stmt) {
  if (stmtHead == nullptr) {
    stmtHead = stmt;
  }
  stmtTail = stmt;
  if (!stmt->IsAux()) {
    if (stmtNoAuxHead == nullptr) {
      stmtNoAuxHead = stmt;
    }
    stmtNoAuxTail = stmt;
  }
}

void FEIRBB::AddStmtAuxPre(FEIRStmt *stmt) {
  if (!stmt->IsAuxPre()) {
    return;
  }
  stmtHead = stmt;
}

void FEIRBB::AddStmtAuxPost(FEIRStmt *stmt) {
  if (!stmt->IsAuxPost()) {
    return;
  }
  stmtTail = stmt;
}

bool FEIRBB::IsPredBB(uint32 bbID) {
  for (FEIRBB *bb : predBBs) {
    if (bb->GetID() == bbID) {
      return true;
    }
  }
  return false;
}

bool FEIRBB::IsSuccBB(uint32 bbID) {
  for (FEIRBB *bb : succBBs) {
    if (bb->GetID() == bbID) {
      return true;
    }
  }
  return false;
}

void FEIRBB::Dump() const {
  std::cout << "FEIRBB (id=" << id << ", kind=" << GetBBKindName() <<
               ", preds={";
  for (FEIRBB *bb : predBBs) {
    std::cout << bb->GetID() << " ";
  }
  std::cout << "}, succs={";
  for (FEIRBB *bb : succBBs) {
    std::cout << bb->GetID() << " ";
  }
  std::cout << "})" << std::endl;
  FELinkListNode *nodeStmt = stmtHead;
  while (nodeStmt != nullptr) {
    FEIRStmt *stmt = static_cast<FEIRStmt*>(nodeStmt);
    stmt->Dump("    ");
    if (nodeStmt == stmtTail) {
      return;
    }
    nodeStmt = nodeStmt->GetNext();
  }
}

std::string FEIRBB::GetBBKindName() const {
  switch (kind) {
    case kBBKindDefault:
      return "Default";
    case kBBKindPesudoHead:
      return "PesudoHead";
    case kBBKindPesudoTail:
      return "PesudoTail";
    default:
      return "unknown";
  }
}
}  // namespace maple