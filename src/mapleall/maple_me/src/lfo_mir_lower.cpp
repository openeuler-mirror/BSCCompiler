/*
 * Copyright (c) [2021] Huawei Technologies Co., Ltd. All rights reserved.
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
#include "lfo_mir_lower.h"

using namespace maple;

// is lowered to :
// label <whilelabel>
// brfalse <cond> <endlabel>
// <body>
// goto <whilelabel>
// label <endlabel>
BlockNode *LFOMIRLower::LowerWhileStmt(WhileStmtNode &whilestmt) {
  MIRBuilder *mirbuilder = mirModule.GetMIRBuilder();
  whilestmt.SetBody(LowerBlock(*whilestmt.GetBody()));
  BlockNode *blk = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
  LabelIdx whilelblidx = func->GetMirFunc()->GetLabelTab()->CreateLabelWithPrefix('w');
  mirModule.CurFunction()->GetLabelTab()->AddToStringLabelMap(whilelblidx);
  LabelNode *whilelblstmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
  whilelblstmt->SetLabelIdx(whilelblidx);
  LfoWhileInfo *whileInfo = lfoFunc->lfomp->New<LfoWhileInfo>();
  lfoFunc->SetWhileLabelCreatedByLfo(whilelblidx);
  lfoFunc->label2WhileInfo.insert(std::make_pair(whilelblidx, whileInfo));
  blk->AddStatement(whilelblstmt);
  CondGotoNode *brfalsestmt = mirModule.CurFuncCodeMemPool()->New<CondGotoNode>(OP_brfalse);
  brfalsestmt->SetOpnd(whilestmt.Opnd(), 0);
  brfalsestmt->SetSrcPos(whilestmt.GetSrcPos());
  // add jump label target later
  blk->AddStatement(brfalsestmt);

  // create body
  CHECK_FATAL(whilestmt.GetBody(), "null ptr check");
  blk->AppendStatementsFromBlock(*whilestmt.GetBody());
  GotoNode *whilegotonode = mirbuilder->CreateStmtGoto(OP_goto, whilelblidx);
  blk->AddStatement(whilegotonode);
  // create endlabel
  LabelIdx endlblidx = mirModule.CurFunction()->GetLabelTab()->CreateLabelWithPrefix('w');
  lfoFunc->SetWhileLabelCreatedByLfo(endlblidx);
  mirModule.CurFunction()->GetLabelTab()->AddToStringLabelMap(endlblidx);
  LabelNode *endlblstmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
  endlblstmt->SetLabelIdx(endlblidx);
  brfalsestmt->SetOffset(endlblidx);
  blk->AddStatement(endlblstmt);
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

BlockNode *LFOMIRLower::LowerIfStmt(IfStmtNode &ifstmt, bool recursive) {
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
  } else if (elseempty) {
    // brfalse <cond> <endlabel>
    // <thenPart>
    // label <endlabel>
    CondGotoNode *brfalsestmt = mirModule.CurFuncCodeMemPool()->New<CondGotoNode>(OP_brfalse);
    brfalsestmt->SetOpnd(ifstmt.Opnd(), 0);
    brfalsestmt->SetSrcPos(ifstmt.GetSrcPos());
    LabelIdx endlabelidx = mirFunc->GetLabelTab()->CreateLabelWithPrefix('e');
    mirFunc->GetLabelTab()->AddToStringLabelMap(endlabelidx);
    if (canRaiseBack) {
      lfoFunc->SetIfLabelCreatedByLfo(endlabelidx);
    }
    LfoIfInfo *ifInfo = lfoFunc->lfomp->New<LfoIfInfo>();
    brfalsestmt->SetOffset(endlabelidx);
    blk->AddStatement(brfalsestmt);

    blk->AppendStatementsFromBlock(*ifstmt.GetThenPart());

    LabelNode *labstmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
    labstmt->SetLabelIdx(endlabelidx);
    ifInfo->endLabel = endlabelidx;
    if (canRaiseBack) {
      lfoFunc->label2IfInfo.insert(std::make_pair(endlabelidx, ifInfo));
    }
    blk->AddStatement(labstmt);
  } else if (thenempty) {
    // brtrue <cond> <endlabel>
    // <elsePart>
    // label <endlabel>
    CondGotoNode *brtruestmt = mirModule.CurFuncCodeMemPool()->New<CondGotoNode>(OP_brtrue);
    brtruestmt->SetOpnd(ifstmt.Opnd(), 0);
    brtruestmt->SetSrcPos(ifstmt.GetSrcPos());
    LabelIdx endlabelidx = mirFunc->GetLabelTab()->CreateLabelWithPrefix('e');
    if (canRaiseBack) {
      lfoFunc->SetIfLabelCreatedByLfo(endlabelidx);
    }
    LfoIfInfo *ifInfo = lfoFunc->lfomp->New<LfoIfInfo>();
    mirFunc->GetLabelTab()->AddToStringLabelMap(endlabelidx);
    brtruestmt->SetOffset(endlabelidx);
    blk->AddStatement(brtruestmt);

    blk->AppendStatementsFromBlock(*ifstmt.GetElsePart());
    LabelNode *labstmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
    labstmt->SetLabelIdx(endlabelidx);
    ifInfo->endLabel = endlabelidx;
    if (canRaiseBack) {
      lfoFunc->label2IfInfo.insert(std::make_pair(endlabelidx, ifInfo));
    }
    blk->AddStatement(labstmt);
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
    LabelIdx elselabelidx = mirFunc->GetLabelTab()->CreateLabelWithPrefix('s');
    mirFunc->GetLabelTab()->AddToStringLabelMap(elselabelidx);
    if (canRaiseBack) {
      lfoFunc->SetIfLabelCreatedByLfo(elselabelidx);
    }
    LfoIfInfo *ifInfo = lfoFunc->lfomp->New<LfoIfInfo>();
    brfalsestmt->SetOffset(elselabelidx);
    blk->AddStatement(brfalsestmt);
    ifInfo->elseLabel = elselabelidx;
    if (canRaiseBack) {
      lfoFunc->label2IfInfo.insert(std::make_pair(elselabelidx, ifInfo));
    }

    blk->AppendStatementsFromBlock(*ifstmt.GetThenPart());
    bool fallthru_from_then = !OpCodeNoFallThrough(ifstmt.GetThenPart()->GetLast()->GetOpCode());
    LabelIdx endlabelidx = 0;

    if (fallthru_from_then) {
      GotoNode *gotostmt = mirModule.CurFuncCodeMemPool()->New<GotoNode>(OP_goto);
      endlabelidx = mirFunc->GetLabelTab()->CreateLabelWithPrefix('e');
      mirFunc->GetLabelTab()->AddToStringLabelMap(endlabelidx);
      if (canRaiseBack) {
        lfoFunc->SetIfLabelCreatedByLfo(endlabelidx);
      }
      gotostmt->SetOffset(endlabelidx);
      blk->AddStatement(gotostmt);
    }

    LabelNode *labstmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
    labstmt->SetLabelIdx(elselabelidx);
    blk->AddStatement(labstmt);

    blk->AppendStatementsFromBlock(*ifstmt.GetElsePart());

    if (fallthru_from_then) {
      labstmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
      labstmt->SetLabelIdx(endlabelidx);
      blk->AddStatement(labstmt);
    }
    if (endlabelidx == 0) {  // create end label
      endlabelidx = mirFunc->GetLabelTab()->CreateLabelWithPrefix('e');
      if (canRaiseBack) {
        lfoFunc->SetIfLabelCreatedByLfo(endlabelidx);
      }
      LabelNode *endlabelnode = mirbuilder->CreateStmtLabel(endlabelidx);
      blk->AddStatement(endlabelnode);
    }
    ifInfo->endLabel = endlabelidx;
  }
  return blk;
}
