/*
 * Copyright (c) [2019] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "func_emit.h"
#include "mir_function.h"
#include "ssa_mir_nodes.h"

namespace maple {
void FuncEmit::EmitLabelForBB(MIRFunction &func, BB &bb) const {
  ASSERT(bb.GetBBLabel() != 0, "Should have a label");
  // create labelnode
  LabelNode *label = func.GetCodeMempool()->New<LabelNode>();
  label->SetLabelIdx(bb.GetBBLabel());
  if (bb.IsEmpty()) {
    bb.SetFirst(label);
    bb.SetLast(label);
  } else {
    // Insert label before the first non-comment statement of bb.
    StmtNode *first = bb.GetStmtNodes().begin().d();
    StmtNode *firstPrev = nullptr;
    while (first != nullptr && first->GetOpCode() == OP_comment) {
      firstPrev = first;
      first = first->GetNext();
    }
    // "first" points to the first non-comment statement, or nullptr.
    if (first != nullptr) {
      label->InsertAfterThis(*first);
      if (first == bb.GetStmtNodes().begin().d()) {
        bb.SetFirst(label);
      }
    } else {
      ASSERT(firstPrev != nullptr, "check func's stmt");
      label->InsertBeforeThis(*firstPrev);
      if (firstPrev == bb.GetStmtNodes().rbegin().base().d()) {
        bb.SetLast(label);
      }
    }
  }
}

static BaseNode *ConvertSSANode(BaseNode *node) {
  if (node->IsSSANode()) {
    node = static_cast<SSANode *>(node)->GetNoSSANode();
  }
  for (uint32 opndId = 0; opndId < node->GetNumOpnds(); ++opndId) {
    ASSERT_NOT_NULL(node->Opnd(opndId));
    node->SetOpnd(ConvertSSANode(node->Opnd(opndId)), opndId);
  }
  return node;
}

static void ConvertStmt(BB &bb) {
  for (auto &stmt : bb.GetStmtNodes()) {
    ConvertSSANode(&stmt);
    if (stmt.GetOpCode() == OP_maydassign) {
      stmt.SetOpCode(OP_dassign);
    }
  }
}

// Inserting BBs in bblist into func's body.
void FuncEmit::EmitBeforeHSSA(MIRFunction &func, const MapleVector<BB*> &bbList) const {
  StmtNode *lastStmt = nullptr;       // last stmt of previous bb
  func.GetBody()->SetFirst(nullptr);  // reset body first stmt
  func.GetBody()->SetLast(nullptr);
  for (BB *bb : bbList) {
    if (bb == nullptr) {
      continue;
    }
    ConvertStmt(*bb);
    if (bb->GetBBLabel() != 0) {
      EmitLabelForBB(func, *bb);
    }
    if (!bb->IsEmpty()) {
      if (func.GetBody()->GetFirst() == nullptr) {
        func.GetBody()->SetFirst(bb->GetStmtNodes().begin().d());
      }
      if (lastStmt != nullptr) { // link basic blocks
        bb->GetFirst().SetPrev(lastStmt);
        lastStmt->SetNext(&bb->GetFirst());
      }
      lastStmt = bb->GetStmtNodes().rbegin().base().d();
    }
    if (bb->AddBackEndTry()) {
      // generate op_endtry andd added to next, it could be in an empty bb.
      StmtNode *endtry = func.GetCodeMempool()->New<StmtNode>(OP_endtry);
      CHECK_FATAL(lastStmt != nullptr, "EmitBeforeHSSA: shouldn't insert before a null stmt");
      endtry->InsertBeforeThis(*lastStmt);
      lastStmt = endtry;
    }
  }
  func.GetBody()->SetLast(lastStmt);
}
}  // namespace maple
