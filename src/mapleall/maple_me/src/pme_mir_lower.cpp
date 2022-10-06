/*
 * Copyright (c) [2021] Futurewei Technologies Co., Ltd. All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */

#include "mir_builder.h"
#include "pme_mir_lower.h"

using namespace maple;

// is lowered to :
// label <whilelabel>
// brfalse <cond> <endlabel>
// <body>
// goto <whilelabel>
// label <endlabel>
BlockNode *PreMeMIRLower::LowerWhileStmt(WhileStmtNode &whileStmt) {
  MIRBuilder *mirbuilder = mirModule.GetMIRBuilder();
  whileStmt.SetBody(LowerBlock(*whileStmt.GetBody()));
  BlockNode *blk = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
  LabelIdx whilelblidx = func->GetMirFunc()->GetLabelTab()->CreateLabelWithPrefix('w');
  mirModule.CurFunction()->GetLabelTab()->AddToStringLabelMap(whilelblidx);
  LabelNode *whilelblstmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
  whilelblstmt->SetLabelIdx(whilelblidx);
  PreMeWhileInfo *whileInfo = preMeFunc->pmemp->New<PreMeWhileInfo>();
  preMeFunc->SetWhileLabelCreatedByPreMe(whilelblidx);
  preMeFunc->label2WhileInfo.insert(std::make_pair(whilelblidx, whileInfo));
  blk->AddStatement(whilelblstmt);
  CondGotoNode *brfalsestmt = mirModule.CurFuncCodeMemPool()->New<CondGotoNode>(OP_brfalse);
  brfalsestmt->SetOpnd(whileStmt.Opnd(), 0);
  brfalsestmt->SetSrcPos(whileStmt.GetSrcPos());
  // add jump label target later
  blk->AddStatement(brfalsestmt);
  // update frequency
  if (GetFuncProfData()) {
    ASSERT(GetFuncProfData()->GetStmtFreq(whileStmt.GetStmtID()) >= 0, "while stmt should has freq");
    GetFuncProfData()->CopyStmtFreq(whilelblstmt->GetStmtID(), whileStmt.GetStmtID());
    GetFuncProfData()->CopyStmtFreq(brfalsestmt->GetStmtID(), whileStmt.GetStmtID());
  }
  // create body
  CHECK_FATAL(whileStmt.GetBody(), "null ptr check");
  blk->AppendStatementsFromBlock(*whileStmt.GetBody());
  GotoNode *whilegotonode = mirbuilder->CreateStmtGoto(OP_goto, whilelblidx);
  if (GetFuncProfData() && blk->GetLast()) {
    ASSERT(GetFuncProfData()->GetStmtFreq(blk->GetLast()->GetStmtID()) >= 0,
        "last stmt of while body should has freq");
    GetFuncProfData()->CopyStmtFreq(whilegotonode->GetStmtID(), blk->GetLast()->GetStmtID());
  }
  blk->AddStatement(whilegotonode);
  // create endlabel
  LabelIdx endlblidx = mirModule.CurFunction()->GetLabelTab()->CreateLabelWithPrefix('w');
  preMeFunc->SetWhileLabelCreatedByPreMe(endlblidx);
  mirModule.CurFunction()->GetLabelTab()->AddToStringLabelMap(endlblidx);
  LabelNode *endlblstmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
  endlblstmt->SetLabelIdx(endlblidx);
  brfalsestmt->SetOffset(endlblidx);
  SrcPosition pos = func->GetMirFunc()->GetScope()->GetScopeEndPos(whileStmt.GetBody()->GetSrcPos());
  if (!pos.IsValid()) {
    pos = func->GetMirFunc()->GetScope()->GetScopeEndPos(whileStmt.GetSrcPos());
  }
  endlblstmt->SetSrcPos(pos);
  blk->AddStatement(endlblstmt);
  if (GetFuncProfData()) {
    int64_t freq = GetFuncProfData()->GetStmtFreq(whileStmt.GetStmtID()) -
                   GetFuncProfData()->GetStmtFreq(whilegotonode->GetStmtID());
    GetFuncProfData()->SetStmtFreq(endlblstmt->GetStmtID(), freq > 0 ? freq : 0);
  }
  return blk;
}

static bool BlockHasLabel(BlockNode *blk) {
  for (StmtNode &stmt : blk->GetStmtNodes()) {
    if (stmt.GetOpCode() == OP_label) {
      return true;
    }
  }
  return false;
}

BlockNode *PreMeMIRLower::LowerIfStmt(IfStmtNode &ifstmt, bool recursive) {
  bool thenempty = ifstmt.GetThenPart() == nullptr || ifstmt.GetThenPart()->GetFirst() == nullptr;
  bool elseempty = ifstmt.GetElsePart() == nullptr || ifstmt.GetElsePart()->GetFirst() == nullptr;
  bool canRaiseBack = true;

  if (recursive) {
    if (!thenempty) {
      if (BlockHasLabel(ifstmt.GetThenPart())) {
        canRaiseBack = false;
      }
      ifstmt.SetThenPart(LowerBlock(*ifstmt.GetThenPart()));
    }
    if (!elseempty) {
      if (BlockHasLabel(ifstmt.GetElsePart())) {
        canRaiseBack = false;
      }
      ifstmt.SetElsePart(LowerBlock(*ifstmt.GetElsePart()));
    }
  }

  BlockNode *blk = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
  MIRFunction *mirFunc = func->GetMirFunc();
  MIRBuilder *mirbuilder = mirModule.GetMIRBuilder();

  if (thenempty && elseempty) {
    // generate EVAL <cond> statement
    UnaryStmtNode *evalstmt = mirModule.CurFuncCodeMemPool()->New<UnaryStmtNode>(OP_eval);
    evalstmt->SetOpnd(ifstmt.Opnd(), 0);
    evalstmt->SetSrcPos(ifstmt.GetSrcPos());
    blk->AddStatement(evalstmt);
    if (GetFuncProfData()) {
      GetFuncProfData()->CopyStmtFreq(evalstmt->GetStmtID(), ifstmt.GetStmtID());
    }
  } else {
    // brfalse <cond> <elselabel>
    // <thenPart>
    // goto <endlabel>
    // label <elselabel>
    // <elsePart>
    // label <endlabel>
    CondGotoNode *brfalsestmt = mirModule.CurFuncCodeMemPool()->New<CondGotoNode>(OP_brfalse);
    brfalsestmt->SetOpnd(ifstmt.Opnd(), 0);
    brfalsestmt->SetSrcPos(ifstmt.GetSrcPos());
   // set stmtfreqs
    if (GetFuncProfData()) {
      GetFuncProfData()->CopyStmtFreq(brfalsestmt->GetStmtID(), ifstmt.GetStmtID());
    }
    LabelIdx elselabelidx = mirFunc->GetLabelTab()->CreateLabelWithPrefix('s');
    mirFunc->GetLabelTab()->AddToStringLabelMap(elselabelidx);
    if (canRaiseBack) {
      preMeFunc->SetIfLabelCreatedByPreMe(elselabelidx);
    }
    PreMeIfInfo *ifInfo = preMeFunc->pmemp->New<PreMeIfInfo>();
    brfalsestmt->SetOffset(elselabelidx);
    blk->AddStatement(brfalsestmt);
    ifInfo->elseLabel = elselabelidx;
    if (canRaiseBack) {
      preMeFunc->label2IfInfo.insert(std::make_pair(elselabelidx, ifInfo));
    }

    bool fallthru_from_then = true;
    if (!thenempty) {
      blk->AppendStatementsFromBlock(*ifstmt.GetThenPart());
      fallthru_from_then = !OpCodeNoFallThrough(ifstmt.GetThenPart()->GetLast()->GetOpCode());
    }
    LabelIdx endlabelidx = 0;

    if (fallthru_from_then) {
      GotoNode *gotostmt = mirModule.CurFuncCodeMemPool()->New<GotoNode>(OP_goto);
      endlabelidx = mirFunc->GetLabelTab()->CreateLabelWithPrefix('e');
      mirFunc->GetLabelTab()->AddToStringLabelMap(endlabelidx);
      if (canRaiseBack) {
        preMeFunc->SetIfLabelCreatedByPreMe(endlabelidx);
      }
      gotostmt->SetOffset(endlabelidx);
      blk->AddStatement(gotostmt);
      // set stmtfreqs
      if (GetFuncProfData()) {
        if (!thenempty) {
          GetFuncProfData()->CopyStmtFreq(gotostmt->GetStmtID(), ifstmt.GetThenPart()->GetStmtID());
        } else {
          GetFuncProfData()->SetStmtFreq(gotostmt->GetStmtID(), 0);
        }
      }
    }

    LabelNode *labstmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
    labstmt->SetLabelIdx(elselabelidx);
    SrcPosition pos = func->GetMirFunc()->GetScope()->GetScopeEndPos(ifstmt.GetThenPart()->GetSrcPos());
    labstmt->SetSrcPos(pos);
    blk->AddStatement(labstmt);

    if (!elseempty) {
      blk->AppendStatementsFromBlock(*ifstmt.GetElsePart());
    }

    if (fallthru_from_then) {
      labstmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
      labstmt->SetLabelIdx(endlabelidx);
      if (!elseempty) {
        SrcPosition position = func->GetMirFunc()->GetScope()->GetScopeEndPos(ifstmt.GetElsePart()->GetSrcPos());
        labstmt->SetSrcPos(position);
      }
      blk->AddStatement(labstmt);
      // set stmtfreqs
      if (!elseempty && GetFuncProfData()) {
        GetFuncProfData()->CopyStmtFreq(labstmt->GetStmtID(), ifstmt.GetStmtID());
      }
    }
    if (endlabelidx == 0) {  // create end label
      endlabelidx = mirFunc->GetLabelTab()->CreateLabelWithPrefix('e');
      if (canRaiseBack) {
        preMeFunc->SetIfLabelCreatedByPreMe(endlabelidx);
      }
      LabelNode *endlabelnode = mirbuilder->CreateStmtLabel(endlabelidx);
      SrcPosition position = func->GetMirFunc()->GetScope()->GetScopeEndPos(ifstmt.GetSrcPos());
      endlabelnode->SetSrcPos(position);
      blk->AddStatement(endlabelnode);
      // set stmtfreqs
      if (GetFuncProfData()) {
        GetFuncProfData()->CopyStmtFreq(endlabelnode->GetStmtID(), ifstmt.GetStmtID());
      }
    }
    ifInfo->endLabel = endlabelidx;
  }
  // generate extra label to avoid critical edge
  LabelIdx extraLabelIdx = mirFunc->GetLabelTab()->CreateLabelWithPrefix('x');
  preMeFunc->SetIfLabelCreatedByPreMe(extraLabelIdx);
  LabelNode *extraLabelNode = mirbuilder->CreateStmtLabel(extraLabelIdx);
  blk->AddStatement(extraLabelNode);
  // set stmtfreqs
  if (GetFuncProfData()) {
    GetFuncProfData()->CopyStmtFreq(extraLabelNode->GetStmtID(), ifstmt.GetStmtID());
  }
  return blk;
}
