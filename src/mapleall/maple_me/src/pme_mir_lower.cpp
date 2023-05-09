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
  if (GetFuncProfData()) {
    // The last stmt in body is not reliable as it could be an end stmt of an inner loop
    ASSERT(GetFuncProfData()->GetStmtFreq(whileStmt.GetBody()->GetStmtID()) >= 0,
        "while body should have freq set");
    GetFuncProfData()->CopyStmtFreq(whilegotonode->GetStmtID(), whileStmt.GetBody()->GetStmtID());
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
    CHECK_FATAL(GetFuncProfData()->GetStmtFreq(whileStmt.GetStmtID()) >= 0, "while stmt's freq is not set");
    FreqType freq = GetFuncProfData()->GetStmtFreq(whileStmt.GetStmtID());
    GetFuncProfData()->SetStmtFreq(endlblstmt->GetStmtID(), freq);
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
  } else if (elseempty && !Options::profileUse && !Options::profileGen) {
    // brfalse <cond> <endlabel>
    // <thenPart>
    // label <endlabel>
    CondGotoNode *brfalsestmt = mirModule.CurFuncCodeMemPool()->New<CondGotoNode>(OP_brfalse);
    brfalsestmt->SetOpnd(ifstmt.Opnd(), 0);
    brfalsestmt->SetSrcPos(ifstmt.GetSrcPos());
    LabelIdx endlabelidx = mirFunc->GetLabelTab()->CreateLabelWithPrefix('e');
    mirFunc->GetLabelTab()->AddToStringLabelMap(endlabelidx);
    if (canRaiseBack) {
      preMeFunc->SetIfLabelCreatedByPreMe(endlabelidx);
    }
    PreMeIfInfo *ifInfo = preMeFunc->pmemp->New<PreMeIfInfo>();
    brfalsestmt->SetOffset(endlabelidx);
    blk->AddStatement(brfalsestmt);
    // set stmtfreqs
    if (GetFuncProfData()) {
      GetFuncProfData()->CopyStmtFreq(brfalsestmt->GetStmtID(), ifstmt.GetStmtID());
    }
    blk->AppendStatementsFromBlock(*ifstmt.GetThenPart());

    LabelNode *labstmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
    labstmt->SetLabelIdx(endlabelidx);
    ifInfo->endLabel = endlabelidx;
    if (canRaiseBack) {
      preMeFunc->label2IfInfo.insert(std::make_pair(endlabelidx, ifInfo));
    }
    SrcPosition pos = func->GetMirFunc()->GetScope()->GetScopeEndPos(ifstmt.GetThenPart()->GetSrcPos());
    labstmt->SetSrcPos(pos);
    blk->AddStatement(labstmt);
    // set stmtfreqs
    if (GetFuncProfData()) {
      ASSERT(GetFuncProfData()->GetStmtFreq(ifstmt.GetThenPart()->GetStmtID()) >= 0, "sanity check");
      FreqType freq = GetFuncProfData()->GetStmtFreq(ifstmt.GetStmtID()) -
                      GetFuncProfData()->GetStmtFreq(ifstmt.GetThenPart()->GetStmtID());
      GetFuncProfData()->SetStmtFreq(labstmt->GetStmtID(), freq);
    }
  } else if (thenempty && !Options::profileUse && !Options::profileGen) {
    // brtrue <cond> <endlabel>
    // <elsePart>
    // label <endlabel>
    CondGotoNode *brtruestmt = mirModule.CurFuncCodeMemPool()->New<CondGotoNode>(OP_brtrue);
    brtruestmt->SetOpnd(ifstmt.Opnd(), 0);
    brtruestmt->SetSrcPos(ifstmt.GetSrcPos());
    LabelIdx endlabelidx = mirFunc->GetLabelTab()->CreateLabelWithPrefix('e');
    if (canRaiseBack) {
      preMeFunc->SetIfLabelCreatedByPreMe(endlabelidx);
    }
    PreMeIfInfo *ifInfo = preMeFunc->pmemp->New<PreMeIfInfo>();
    mirFunc->GetLabelTab()->AddToStringLabelMap(endlabelidx);
    brtruestmt->SetOffset(endlabelidx);
    blk->AddStatement(brtruestmt);

    // set stmtfreqs
    if (GetFuncProfData()) {
      GetFuncProfData()->CopyStmtFreq(brtruestmt->GetStmtID(), ifstmt.GetStmtID());
    }
    blk->AppendStatementsFromBlock(*ifstmt.GetElsePart());
    LabelNode *labstmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
    labstmt->SetLabelIdx(endlabelidx);
    ifInfo->endLabel = endlabelidx;
    if (canRaiseBack) {
      preMeFunc->label2IfInfo.insert(std::make_pair(endlabelidx, ifInfo));
    }
    SrcPosition pos = func->GetMirFunc()->GetScope()->GetScopeEndPos(ifstmt.GetElsePart()->GetSrcPos());
    labstmt->SetSrcPos(pos);
    blk->AddStatement(labstmt);
    // set stmtfreqs
    if (GetFuncProfData()) {
      ASSERT(GetFuncProfData()->GetStmtFreq(ifstmt.GetElsePart()->GetStmtID()) > 0, "sanity check");
      FreqType freq = GetFuncProfData()->GetStmtFreq(ifstmt.GetStmtID()) -
                         GetFuncProfData()->GetStmtFreq(ifstmt.GetElsePart()->GetStmtID());
      GetFuncProfData()->SetStmtFreq(labstmt->GetStmtID(), freq);
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

    bool fallthruFromThen = true;
    if (!thenempty) {
      blk->AppendStatementsFromBlock(*ifstmt.GetThenPart());
      fallthruFromThen = !OpCodeNoFallThrough(ifstmt.GetThenPart()->GetLast()->GetOpCode());
    }
    LabelIdx endlabelidx = 0;

    if (fallthruFromThen) {
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
          if (ifstmt.GetThenPart()->GetLast()->IsCondBr()) {
            // Estimate a freq within [0, ifstmt-freq] without going after further
            FreqType ifFreq = GetFuncProfData()->GetStmtFreq(ifstmt.GetStmtID());
            FreqType ifElseFreq = GetFuncProfData()->GetStmtFreq(ifstmt.GetElsePart()->GetStmtID());
            FreqType freqDiff = (ifFreq >= ifElseFreq) ? (ifFreq - ifElseFreq) : 0;
            GetFuncProfData()->SetStmtFreq(gotostmt->GetStmtID(), freqDiff);
          } else {
            GetFuncProfData()->CopyStmtFreq(gotostmt->GetStmtID(), ifstmt.GetThenPart()->GetStmtID());
          }
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
    if (GetFuncProfData()) {
      GetFuncProfData()->CopyStmtFreq(labstmt->GetStmtID(), ifstmt.GetElsePart()->GetStmtID());
    }
    if (!elseempty) {
      blk->AppendStatementsFromBlock(*ifstmt.GetElsePart());
    }

    if (fallthruFromThen) {
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
  if (Options::profileUse || Options::profileGen) {
    // generate extra label to avoid critical edge
    LabelIdx extraLabelIdx = mirFunc->GetLabelTab()->CreateLabelWithPrefix('x');
    preMeFunc->SetIfLabelCreatedByPreMe(extraLabelIdx);
    LabelNode *extraLabelNode = mirbuilder->CreateStmtLabel(extraLabelIdx);
    blk->AddStatement(extraLabelNode);
    if (GetFuncProfData()) {
      // set stmtfreqs
      GetFuncProfData()->CopyStmtFreq(extraLabelNode->GetStmtID(), ifstmt.GetStmtID());
    }
  }
  return blk;
}
