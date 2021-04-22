/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "mir_lower.h"

#define DO_LT_0_CHECK 1

namespace maple {
LabelIdx MIRLower::CreateCondGotoStmt(Opcode op, BlockNode &blk, const IfStmtNode &ifStmt) {
  auto *brStmt = mirModule.CurFuncCodeMemPool()->New<CondGotoNode>(op);
  brStmt->SetOpnd(ifStmt.Opnd(), 0);
  brStmt->SetSrcPos(ifStmt.GetSrcPos());
  LabelIdx lableIdx = mirModule.CurFunction()->GetLabelTab()->CreateLabel();
  (void)mirModule.CurFunction()->GetLabelTab()->AddToStringLabelMap(lableIdx);
  brStmt->SetOffset(lableIdx);
  blk.AddStatement(brStmt);
  bool thenEmpty = (ifStmt.GetThenPart() == nullptr) || (ifStmt.GetThenPart()->GetFirst() == nullptr);
  if (thenEmpty) {
    blk.AppendStatementsFromBlock(*ifStmt.GetElsePart());
  } else {
    blk.AppendStatementsFromBlock(*ifStmt.GetThenPart());
  }
  return lableIdx;
}

void MIRLower::CreateBrFalseStmt(BlockNode &blk, const IfStmtNode &ifStmt) {
  LabelIdx labelIdx = CreateCondGotoStmt(OP_brfalse, blk, ifStmt);
  auto *lableStmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
  lableStmt->SetLabelIdx(labelIdx);
  blk.AddStatement(lableStmt);
}

void MIRLower::CreateBrTrueStmt(BlockNode &blk, const IfStmtNode &ifStmt) {
  LabelIdx labelIdx = CreateCondGotoStmt(OP_brtrue, blk, ifStmt);
  auto *lableStmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
  lableStmt->SetLabelIdx(labelIdx);
  blk.AddStatement(lableStmt);
}


void MIRLower::CreateBrFalseAndGotoStmt(BlockNode &blk, const IfStmtNode &ifStmt) {
  LabelIdx labelIdx = CreateCondGotoStmt(OP_brfalse, blk, ifStmt);
  bool fallThroughFromThen = !IfStmtNoFallThrough(ifStmt);
  LabelIdx gotoLableIdx = 0;
  if (fallThroughFromThen) {
    auto *gotoStmt = mirModule.CurFuncCodeMemPool()->New<GotoNode>(OP_goto);
    gotoLableIdx = mirModule.CurFunction()->GetLabelTab()->CreateLabel();
    (void)mirModule.CurFunction()->GetLabelTab()->AddToStringLabelMap(gotoLableIdx);
    gotoStmt->SetOffset(gotoLableIdx);
    blk.AddStatement(gotoStmt);
  }
  auto *lableStmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
  lableStmt->SetLabelIdx(labelIdx);
  blk.AddStatement(lableStmt);
  blk.AppendStatementsFromBlock(*ifStmt.GetElsePart());
  if (fallThroughFromThen) {
    lableStmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
    lableStmt->SetLabelIdx(gotoLableIdx);
    blk.AddStatement(lableStmt);
  }
}

BlockNode *MIRLower::LowerIfStmt(IfStmtNode &ifStmt, bool recursive) {
  bool thenEmpty = (ifStmt.GetThenPart() == nullptr) || (ifStmt.GetThenPart()->GetFirst() == nullptr);
  bool elseEmpty = (ifStmt.GetElsePart() == nullptr) || (ifStmt.GetElsePart()->GetFirst() == nullptr);
  if (recursive) {
    if (!thenEmpty) {
      ifStmt.SetThenPart(LowerBlock(*ifStmt.GetThenPart()));
    }
    if (!elseEmpty) {
      ifStmt.SetElsePart(LowerBlock(*ifStmt.GetElsePart()));
    }
  }
  auto *blk = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
  if (thenEmpty && elseEmpty) {
    // generate EVAL <cond> statement
    auto *evalStmt = mirModule.CurFuncCodeMemPool()->New<UnaryStmtNode>(OP_eval);
    evalStmt->SetOpnd(ifStmt.Opnd(), 0);
    evalStmt->SetSrcPos(ifStmt.GetSrcPos());
    blk->AddStatement(evalStmt);
  } else if (elseEmpty) {
    // brfalse <cond> <endlabel>
    // <thenPart>
    // label <endlabel>
    CreateBrFalseStmt(*blk, ifStmt);
  } else if (thenEmpty) {
    // brtrue <cond> <endlabel>
    // <elsePart>
    // label <endlabel>
    CreateBrTrueStmt(*blk, ifStmt);
  } else {
    // brfalse <cond> <elselabel>
    // <thenPart>
    // goto <endlabel>
    // label <elselabel>
    // <elsePart>
    // label <endlabel>
    CreateBrFalseAndGotoStmt(*blk, ifStmt);
  }
  return blk;
}

//     while <cond> <body>
// is lowered to:
//     brfalse <cond> <endlabel>
//   label <bodylabel>
//     <body>
//     brtrue <cond> <bodylabel>
//   label <endlabel>
BlockNode *MIRLower::LowerWhileStmt(WhileStmtNode &whileStmt) {
  ASSERT(whileStmt.GetBody() != nullptr, "nullptr check");
  whileStmt.SetBody(LowerBlock(*whileStmt.GetBody()));
  auto *blk = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
  auto *brFalseStmt = mirModule.CurFuncCodeMemPool()->New<CondGotoNode>(OP_brfalse);
  brFalseStmt->SetOpnd(whileStmt.Opnd(0), 0);
  brFalseStmt->SetSrcPos(whileStmt.GetSrcPos());
  LabelIdx lalbeIdx = mirModule.CurFunction()->GetLabelTab()->CreateLabel();
  (void)mirModule.CurFunction()->GetLabelTab()->AddToStringLabelMap(lalbeIdx);
  brFalseStmt->SetOffset(lalbeIdx);
  blk->AddStatement(brFalseStmt);
  LabelIdx bodyLableIdx = mirModule.CurFunction()->GetLabelTab()->CreateLabel();
  (void)mirModule.CurFunction()->GetLabelTab()->AddToStringLabelMap(bodyLableIdx);
  auto *lableStmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
  lableStmt->SetLabelIdx(bodyLableIdx);
  blk->AddStatement(lableStmt);
  blk->AppendStatementsFromBlock(*whileStmt.GetBody());
  auto *brTrueStmt = mirModule.CurFuncCodeMemPool()->New<CondGotoNode>(OP_brtrue);
  brTrueStmt->SetOpnd(whileStmt.Opnd(0)->CloneTree(mirModule.GetCurFuncCodeMPAllocator()), 0);
  brTrueStmt->SetOffset(bodyLableIdx);
  blk->AddStatement(brTrueStmt);
  lableStmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
  lableStmt->SetLabelIdx(lalbeIdx);
  blk->AddStatement(lableStmt);
  return blk;
}

//    doloop <do-var>(<start-expr>,<cont-expr>,<incr-amt>) {<body-stmts>}
// is lowered to:
//     dassign <do-var> (<start-expr>)
//     brfalse <cond-expr> <endlabel>
//   label <bodylabel>
//     <body-stmts>
//     dassign <do-var> (<incr-amt>)
//     brtrue <cond-expr>  <bodylabel>
//   label <endlabel>
BlockNode *MIRLower::LowerDoloopStmt(DoloopNode &doloop) {
  ASSERT(doloop.GetDoBody() != nullptr, "nullptr check");
  doloop.SetDoBody(LowerBlock(*doloop.GetDoBody()));
  auto *blk = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
  if (doloop.IsPreg()) {
    PregIdx regIdx = (PregIdx)doloop.GetDoVarStIdx().FullIdx();
    MIRPreg *mirPreg = mirModule.CurFunction()->GetPregTab()->PregFromPregIdx(regIdx);
    PrimType primType = mirPreg->GetPrimType();
    ASSERT(primType != kPtyInvalid, "runtime check error");
    auto *startRegassign = mirModule.CurFuncCodeMemPool()->New<RegassignNode>();
    startRegassign->SetRegIdx(regIdx);
    startRegassign->SetPrimType(primType);
    startRegassign->SetOpnd(doloop.GetStartExpr(), 0);
    startRegassign->SetSrcPos(doloop.GetSrcPos());
    blk->AddStatement(startRegassign);
  } else {
    auto *startDassign = mirModule.CurFuncCodeMemPool()->New<DassignNode>();
    startDassign->SetStIdx(doloop.GetDoVarStIdx());
    startDassign->SetRHS(doloop.GetStartExpr());
    startDassign->SetSrcPos(doloop.GetSrcPos());
    blk->AddStatement(startDassign);
  }
  auto *brFalseStmt = mirModule.CurFuncCodeMemPool()->New<CondGotoNode>(OP_brfalse);
  brFalseStmt->SetOpnd(doloop.GetCondExpr(), 0);
  LabelIdx lIdx = mirModule.CurFunction()->GetLabelTab()->CreateLabel();
  (void)mirModule.CurFunction()->GetLabelTab()->AddToStringLabelMap(lIdx);
  brFalseStmt->SetOffset(lIdx);
  blk->AddStatement(brFalseStmt);
  LabelIdx bodyLabelIdx = mirModule.CurFunction()->GetLabelTab()->CreateLabel();
  (void)mirModule.CurFunction()->GetLabelTab()->AddToStringLabelMap(bodyLabelIdx);
  auto *labelStmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
  labelStmt->SetLabelIdx(bodyLabelIdx);
  blk->AddStatement(labelStmt);
  blk->AppendStatementsFromBlock(*doloop.GetDoBody());
  if (doloop.IsPreg()) {
    PregIdx regIdx = (PregIdx)doloop.GetDoVarStIdx().FullIdx();
    MIRPreg *mirPreg = mirModule.CurFunction()->GetPregTab()->PregFromPregIdx(regIdx);
    PrimType doVarPType = mirPreg->GetPrimType();
    ASSERT(doVarPType != kPtyInvalid, "runtime check error");
    auto *readDoVar = mirModule.CurFuncCodeMemPool()->New<RegreadNode>();
    readDoVar->SetRegIdx(regIdx);
    readDoVar->SetPrimType(doVarPType);
    auto *add =
        mirModule.CurFuncCodeMemPool()->New<BinaryNode>(OP_add, doVarPType, doloop.GetIncrExpr(), readDoVar);
    auto *endRegassign = mirModule.CurFuncCodeMemPool()->New<RegassignNode>();
    endRegassign->SetRegIdx(regIdx);
    endRegassign->SetPrimType(doVarPType);
    endRegassign->SetOpnd(add, 0);
    blk->AddStatement(endRegassign);
  } else {
    const MIRSymbol *doVarSym = mirModule.CurFunction()->GetLocalOrGlobalSymbol(doloop.GetDoVarStIdx());
    PrimType doVarPType = doVarSym->GetType()->GetPrimType();
    auto *readDovar =
        mirModule.CurFuncCodeMemPool()->New<DreadNode>(OP_dread, doVarPType, doloop.GetDoVarStIdx(), 0);
    auto *add =
        mirModule.CurFuncCodeMemPool()->New<BinaryNode>(OP_add, doVarPType, doloop.GetIncrExpr(), readDovar);
    auto *endDassign = mirModule.CurFuncCodeMemPool()->New<DassignNode>();
    endDassign->SetStIdx(doloop.GetDoVarStIdx());
    endDassign->SetRHS(add);
    blk->AddStatement(endDassign);
  }
  auto *brTrueStmt = mirModule.CurFuncCodeMemPool()->New<CondGotoNode>(OP_brtrue);
  brTrueStmt->SetOpnd(doloop.GetCondExpr()->CloneTree(mirModule.GetCurFuncCodeMPAllocator()), 0);
  brTrueStmt->SetOffset(bodyLabelIdx);
  blk->AddStatement(brTrueStmt);
  labelStmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
  labelStmt->SetLabelIdx(lIdx);
  blk->AddStatement(labelStmt);
  return blk;
}

//     dowhile <body> <cond>
// is lowered to:
//   label <bodylabel>
//     <body>
//     brtrue <cond> <bodylabel>
BlockNode *MIRLower::LowerDowhileStmt(WhileStmtNode &doWhileStmt) {
  ASSERT(doWhileStmt.GetBody() != nullptr, "nullptr check");
  doWhileStmt.SetBody(LowerBlock(*doWhileStmt.GetBody()));
  auto *blk = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
  LabelIdx lIdx = mirModule.CurFunction()->GetLabelTab()->CreateLabel();
  (void)mirModule.CurFunction()->GetLabelTab()->AddToStringLabelMap(lIdx);
  auto *labelStmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
  labelStmt->SetLabelIdx(lIdx);
  blk->AddStatement(labelStmt);
  blk->AppendStatementsFromBlock(*doWhileStmt.GetBody());
  auto *brTrueStmt = mirModule.CurFuncCodeMemPool()->New<CondGotoNode>(OP_brtrue);
  brTrueStmt->SetOpnd(doWhileStmt.Opnd(0), 0);
  brTrueStmt->SetOffset(lIdx);
  blk->AddStatement(brTrueStmt);
  return blk;
}

BlockNode *MIRLower::LowerBlock(BlockNode &block) {
  auto *newBlock = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
  BlockNode *tmp = nullptr;
  if (block.GetFirst() == nullptr) {
    return newBlock;
  }
  StmtNode *nextStmt = block.GetFirst();
  ASSERT(nextStmt != nullptr, "nullptr check");
  do {
    StmtNode *stmt = nextStmt;
    nextStmt = stmt->GetNext();
    switch (stmt->GetOpCode()) {
      case OP_if:
        tmp = LowerIfStmt(static_cast<IfStmtNode&>(*stmt), true);
        newBlock->AppendStatementsFromBlock(*tmp);
        break;
      case OP_while:
        newBlock->AppendStatementsFromBlock(*LowerWhileStmt(static_cast<WhileStmtNode&>(*stmt)));
        break;
      case OP_dowhile:
        newBlock->AppendStatementsFromBlock(*LowerDowhileStmt(static_cast<WhileStmtNode&>(*stmt)));
        break;
      case OP_doloop:
        newBlock->AppendStatementsFromBlock(*LowerDoloopStmt(static_cast<DoloopNode&>(*stmt)));
        break;
      case OP_block:
        tmp = LowerBlock(static_cast<BlockNode&>(*stmt));
        newBlock->AppendStatementsFromBlock(*tmp);
        break;
      default:
        newBlock->AddStatement(stmt);
        break;
    }
  } while (nextStmt != nullptr);
  return newBlock;
}

// for lowering OP_cand and OP_cior that are top level operators in the
// condition operand of OP_brfalse and OP_brtrue
void MIRLower::LowerBrCondition(BlockNode &block) {
  if (block.GetFirst() == nullptr) {
    return;
  }
  StmtNode *nextStmt = block.GetFirst();
  do {
    StmtNode *stmt = nextStmt;
    nextStmt = stmt->GetNext();
    if (stmt->IsCondBr()) {
      auto *condGoto = static_cast<CondGotoNode*>(stmt);
      if (condGoto->Opnd(0)->GetOpCode() == OP_cand || condGoto->Opnd(0)->GetOpCode() == OP_cior) {
        auto *cond = static_cast<BinaryNode*>(condGoto->Opnd(0));
        if ((stmt->GetOpCode() == OP_brfalse && cond->GetOpCode() == OP_cand) ||
            (stmt->GetOpCode() == OP_brtrue && cond->GetOpCode() == OP_cior)) {
          // short-circuit target label is same as original condGoto stmt
          condGoto->SetOpnd(cond->GetBOpnd(0), 0);
          auto *newCondGoto = mirModule.CurFuncCodeMemPool()->New<CondGotoNode>(Opcode(stmt->GetOpCode()));
          newCondGoto->SetOpnd(cond->GetBOpnd(1), 0);
          newCondGoto->SetOffset(condGoto->GetOffset());
          block.InsertAfter(newCondGoto, condGoto);
          nextStmt = stmt;  // so it will be re-processed if another cand/cior
        } else {            // short-circuit target is next statement
          LabelIdx lIdx;
          LabelNode *labelStmt = nullptr;
          if (nextStmt->GetOpCode() == OP_label) {
            labelStmt = static_cast<LabelNode*>(nextStmt);
            lIdx = labelStmt->GetLabelIdx();
          } else {
            lIdx = mirModule.CurFunction()->GetLabelTab()->CreateLabel();
            (void)mirModule.CurFunction()->GetLabelTab()->AddToStringLabelMap(lIdx);
            labelStmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
            labelStmt->SetLabelIdx(lIdx);
            block.InsertAfter(condGoto, labelStmt);
          }
          auto *newCondGoto = mirModule.CurFuncCodeMemPool()->New<CondGotoNode>(
              stmt->GetOpCode() == OP_brfalse ? OP_brtrue : OP_brfalse);
          newCondGoto->SetOpnd(cond->GetBOpnd(0), 0);
          newCondGoto->SetOffset(lIdx);
          block.InsertBefore(condGoto, newCondGoto);
          condGoto->SetOpnd(cond->GetBOpnd(1), 0);
          nextStmt = newCondGoto;  // so it will be re-processed if another cand/cior
        }
      }
    }
  } while (nextStmt != nullptr);
}

void MIRLower::LowerFunc(MIRFunction &func) {
  mirModule.SetCurFunction(&func);
  if (IsLowerExpandArray()) {
    ExpandArrayMrt(func);
  }
  BlockNode *origBody = func.GetBody();
  ASSERT(origBody != nullptr, "nullptr check");
  BlockNode *newBody = LowerBlock(*origBody);
  ASSERT(newBody != nullptr, "nullptr check");
  LowerBrCondition(*newBody);
  func.SetBody(newBody);
}

IfStmtNode *MIRLower::ExpandArrayMrtIfBlock(IfStmtNode &node) {
  if (node.GetThenPart() != nullptr) {
    node.SetThenPart(ExpandArrayMrtBlock(*node.GetThenPart()));
  }
  if (node.GetElsePart() != nullptr) {
    node.SetElsePart(ExpandArrayMrtBlock(*node.GetElsePart()));
  }
  return &node;
}

WhileStmtNode *MIRLower::ExpandArrayMrtWhileBlock(WhileStmtNode &node) {
  if (node.GetBody() != nullptr) {
    node.SetBody(ExpandArrayMrtBlock(*node.GetBody()));
  }
  return &node;
}

DoloopNode *MIRLower::ExpandArrayMrtDoloopBlock(DoloopNode &node) {
  if (node.GetDoBody() != nullptr) {
    node.SetDoBody(ExpandArrayMrtBlock(*node.GetDoBody()));
  }
  return &node;
}

ForeachelemNode *MIRLower::ExpandArrayMrtForeachelemBlock(ForeachelemNode &node) {
  if (node.GetLoopBody() != nullptr) {
    node.SetLoopBody(ExpandArrayMrtBlock(*node.GetLoopBody()));
  }
  return &node;
}

void MIRLower::AddArrayMrtMpl(BaseNode &exp, BlockNode &newBlock) {
  MIRModule &mod = mirModule;
  MIRBuilder *builder = mod.GetMIRBuilder();
  for (size_t i = 0; i < exp.NumOpnds(); ++i) {
    ASSERT(exp.Opnd(i) != nullptr, "nullptr check");
    AddArrayMrtMpl(*exp.Opnd(i), newBlock);
  }
  if (exp.GetOpCode() == OP_array) {
    auto &arrayNode = static_cast<ArrayNode&>(exp);
    if (arrayNode.GetBoundsCheck()) {
      BaseNode *arrAddr = arrayNode.Opnd(0);
      BaseNode *index = arrayNode.Opnd(1);
      ASSERT(index != nullptr, "null ptr check");
      MIRType *indexType = GlobalTables::GetTypeTable().GetPrimType(index->GetPrimType());
      UnaryStmtNode *nullCheck = builder->CreateStmtUnary(OP_assertnonnull, arrAddr);
      newBlock.AddStatement(nullCheck);
#if DO_LT_0_CHECK
      ConstvalNode *indexZero = builder->GetConstUInt32(0);
      CompareNode *lessZero = builder->CreateExprCompare(OP_lt, *GlobalTables::GetTypeTable().GetUInt1(),
                                                         *GlobalTables::GetTypeTable().GetUInt32(), index, indexZero);
#endif
      MIRType *infoLenType = GlobalTables::GetTypeTable().GetInt32();
      MapleVector<BaseNode*> arguments(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
      arguments.push_back(arrAddr);
      BaseNode *arrLen = builder->CreateExprIntrinsicop(INTRN_JAVA_ARRAY_LENGTH, OP_intrinsicop,
                                                        *infoLenType, arguments);
      BaseNode *cpmIndex = index;
      if (arrLen->GetPrimType() != index->GetPrimType()) {
        cpmIndex = builder->CreateExprTypeCvt(OP_cvt, *infoLenType, *indexType, index);
      }
      CompareNode *largeLen = builder->CreateExprCompare(OP_ge, *GlobalTables::GetTypeTable().GetUInt1(),
                                                         *GlobalTables::GetTypeTable().GetUInt32(), cpmIndex, arrLen);
      // maybe should use cior
#if DO_LT_0_CHECK
      BinaryNode *indexCon =
          builder->CreateExprBinary(OP_lior, *GlobalTables::GetTypeTable().GetUInt1(), lessZero, largeLen);
#endif
      MapleVector<BaseNode*> args(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
#if DO_LT_0_CHECK
      args.push_back(indexCon);
      IntrinsiccallNode *boundaryTrinsicCall = builder->CreateStmtIntrinsicCall(INTRN_MPL_BOUNDARY_CHECK, args);
#else
      args.push_back(largeLen);
      IntrinsiccallNode *boundaryTrinsicCall = builder->CreateStmtIntrinsicCall(INTRN_MPL_BOUNDARY_CHECK, args);
#endif
      newBlock.AddStatement(boundaryTrinsicCall);
    }
  }
}

BlockNode *MIRLower::ExpandArrayMrtBlock(BlockNode &block) {
  auto *newBlock = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
  if (block.GetFirst() == nullptr) {
    return newBlock;
  }
  StmtNode *nextStmt = block.GetFirst();
  do {
    StmtNode *stmt = nextStmt;
    ASSERT(stmt != nullptr, "nullptr check");
    nextStmt = stmt->GetNext();
    switch (stmt->GetOpCode()) {
      case OP_if:
        newBlock->AddStatement(ExpandArrayMrtIfBlock(static_cast<IfStmtNode&>(*stmt)));
        break;
      case OP_while:
        newBlock->AddStatement(ExpandArrayMrtWhileBlock(static_cast<WhileStmtNode&>(*stmt)));
        break;
      case OP_dowhile:
        newBlock->AddStatement(ExpandArrayMrtWhileBlock(static_cast<WhileStmtNode&>(*stmt)));
        break;
      case OP_doloop:
        newBlock->AddStatement(ExpandArrayMrtDoloopBlock(static_cast<DoloopNode&>(*stmt)));
        break;
      case OP_foreachelem:
        newBlock->AddStatement(ExpandArrayMrtForeachelemBlock(static_cast<ForeachelemNode&>(*stmt)));
        break;
      case OP_block:
        newBlock->AddStatement(ExpandArrayMrtBlock(static_cast<BlockNode&>(*stmt)));
        break;
      default:
        AddArrayMrtMpl(*stmt, *newBlock);
        newBlock->AddStatement(stmt);
        break;
    }
  } while (nextStmt != nullptr);
  return newBlock;
}

void MIRLower::ExpandArrayMrt(MIRFunction &func) {
  if (ShouldOptArrayMrt(func)) {
    BlockNode *origBody = func.GetBody();
    ASSERT(origBody != nullptr, "nullptr check");
    BlockNode *newBody = ExpandArrayMrtBlock(*origBody);
    func.SetBody(newBody);
  }
}

const std::set<std::string> MIRLower::kSetArrayHotFunc = {};

bool MIRLower::ShouldOptArrayMrt(const MIRFunction &func) {
  return (MIRLower::kSetArrayHotFunc.find(func.GetName()) != MIRLower::kSetArrayHotFunc.end());
}
}  // namespace maple
