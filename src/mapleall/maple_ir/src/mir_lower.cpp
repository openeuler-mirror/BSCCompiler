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
#include "constantfold.h"

#define DO_LT_0_CHECK 1

namespace maple {

static constexpr uint64 RoundUpConst(uint64 offset, uint32 align) {
  return (-align) & (offset + align - 1);
}

static inline uint64 RoundUp(uint64 offset, uint32 align) {
  if (align == 0) {
    return offset;
  }
  return RoundUpConst(offset, align);
}

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

// for lowering OP_cand and OP_cior embedded in the expression x which belongs
// to curstmt
BaseNode* MIRLower::LowerEmbeddedCandCior(BaseNode *x, StmtNode *curstmt, BlockNode *blk) {
  if (x->GetOpCode() == OP_cand || x->GetOpCode() == OP_cior) {
    MIRBuilder *builder = mirModule.GetMIRBuilder();
    BinaryNode *bnode = static_cast<BinaryNode *>(x);
    bnode->SetOpnd(LowerEmbeddedCandCior(bnode->Opnd(0), curstmt, blk), 0);
    PregIdx pregIdx = mirFunc->GetPregTab()->CreatePreg(PTY_u8);
    RegassignNode *regass = builder->CreateStmtRegassign(PTY_u8, pregIdx, bnode->Opnd(0));
    blk->InsertBefore(curstmt, regass);
    LabelIdx labIdx = mirFunc->GetLabelTab()->CreateLabel();
    (void)mirFunc->GetLabelTab()->AddToStringLabelMap(labIdx);
    BaseNode *cond = builder->CreateExprRegread(PTY_u8, pregIdx);
    CondGotoNode *cgoto = mirFunc->GetCodeMempool()->New<CondGotoNode>(x->GetOpCode() == OP_cior ? OP_brtrue : OP_brfalse);
    cgoto->SetOpnd(cond, 0);
    cgoto->SetOffset(labIdx);
    blk->InsertBefore(curstmt, cgoto);

    bnode->SetOpnd(LowerEmbeddedCandCior(bnode->Opnd(1), curstmt, blk), 1);
    regass = builder->CreateStmtRegassign(PTY_u8, pregIdx, bnode->Opnd(1));
    blk->InsertBefore(curstmt, regass);
    LabelNode *lbl = mirFunc->GetCodeMempool()->New<LabelNode>();
    lbl->SetLabelIdx(labIdx);
    blk->InsertBefore(curstmt, lbl);
    return builder->CreateExprRegread(PTY_u8, pregIdx);
  } else {
    for (size_t i = 0; i < x->GetNumOpnds(); i++) {
      x->SetOpnd(LowerEmbeddedCandCior(x->Opnd(i), curstmt, blk), i);
    }
    return x;
  }
}

// for lowering all appearances of OP_cand and OP_cior associated with condional
// branches in the block
void MIRLower::LowerCandCior(BlockNode &block) {
if (block.GetFirst() == nullptr) {
  return;
}
StmtNode *nextStmt = block.GetFirst();
do {
  StmtNode *stmt = nextStmt;
  nextStmt = stmt->GetNext();
  if (stmt->IsCondBr() &&
      (stmt->Opnd(0)->GetOpCode() == OP_cand || stmt->Opnd(0)->GetOpCode() == OP_cior)) {
    CondGotoNode *condGoto = static_cast<CondGotoNode*>(stmt);
    BinaryNode *cond = static_cast<BinaryNode*>(condGoto->Opnd(0));
    if ((stmt->GetOpCode() == OP_brfalse && cond->GetOpCode() == OP_cand) ||
        (stmt->GetOpCode() == OP_brtrue && cond->GetOpCode() == OP_cior)) {
      // short-circuit target label is same as original condGoto stmt
        condGoto->SetOpnd(cond->GetBOpnd(0), 0);
        auto *newCondGoto = mirModule.CurFuncCodeMemPool()->New<CondGotoNode>(Opcode(stmt->GetOpCode()));
        newCondGoto->SetOpnd(cond->GetBOpnd(1), 0);
        newCondGoto->SetOffset(condGoto->GetOffset());
        block.InsertAfter(condGoto, newCondGoto);
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
    } else {  // call LowerEmbeddedCandCior() for all the expression operands
      for (size_t i = 0; i < stmt->GetNumOpnds(); i++) {
        stmt->SetOpnd(LowerEmbeddedCandCior(stmt->Opnd(i), stmt, &block), i);
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
  if (!InLFO()) {
    LowerCandCior(*newBody);
  }
  func.SetBody(newBody);
}

BaseNode *MIRLower::LowerFarray(ArrayNode *array) {
  auto *farrayType = static_cast<MIRFarrayType*>(array->GetArrayType(GlobalTables::GetTypeTable()));
  size_t eSize = GlobalTables::GetTypeTable().GetTypeFromTyIdx(farrayType->GetElemTyIdx())->GetSize();
  MIRType &arrayType = *GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(array->GetPrimType()));
  /* how about multi-dimension array? */
  if (array->GetIndex(0)->GetOpCode() == OP_constval) {
    const ConstvalNode *constvalNode = static_cast<const ConstvalNode*>(array->GetIndex(0));
    if (constvalNode->GetConstVal()->GetKind() == kConstInt) {
      const MIRIntConst *pIntConst = static_cast<const MIRIntConst*>(constvalNode->GetConstVal());
      CHECK_FATAL(mirModule.IsJavaModule() || pIntConst->GetValue() >= 0, "Array index should >= 0.");
      int64 eleOffset = pIntConst->GetValue() * eSize;

      BaseNode *baseNode = array->GetBase();
      if (eleOffset == 0) {
        return baseNode;
      }

      MIRIntConst *eleConst =
          GlobalTables::GetIntConstTable().GetOrCreateIntConst(eleOffset, arrayType);
      BaseNode *offsetNode = mirModule.CurFuncCodeMemPool()->New<ConstvalNode>(eleConst);
      offsetNode->SetPrimType(array->GetPrimType());

      BaseNode *rAdd = mirModule.CurFuncCodeMemPool()->New<BinaryNode>(OP_add);
      rAdd->SetPrimType(array->GetPrimType());
      rAdd->SetOpnd(baseNode, 0);
      rAdd->SetOpnd(offsetNode, 1);
      return rAdd;
    }
  }

  BaseNode *rMul = nullptr;

  BaseNode *baseNode = array->GetBase();

  BaseNode *rAdd = mirModule.CurFuncCodeMemPool()->New<BinaryNode>(OP_add);
  rAdd->SetPrimType(array->GetPrimType());
  rAdd->SetOpnd(baseNode, 0);
  rAdd->SetOpnd(rMul, 1);
  auto *newAdd = ConstantFold(mirModule).Fold(rAdd);
  rAdd = (newAdd != nullptr ? newAdd : rAdd);
  return rAdd;
}

BaseNode *MIRLower::LowerCArray(ArrayNode *array) {
  MIRType *aType = array->GetArrayType(GlobalTables::GetTypeTable());
  if (aType->GetKind() == kTypeJArray) {
    return array;
  }
  if (aType->GetKind() == kTypeFArray) {
    return LowerFarray(array);
  }

  MIRArrayType *arrayType = static_cast<MIRArrayType *>(aType);
  /* There are two cases where dimension > 1.
   * 1) arrayType->dim > 1.  Process the current arrayType. (nestedArray = false)
   * 2) arrayType->dim == 1, but arraytype->eTyIdx is another array. (nestedArray = true)
   * Assume at this time 1) and 2) cannot mix.
   * Along with the array dimension, there is the array indexing.
   * It is allowed to index arrays less than the dimension.
   * This is dictated by the number of indexes.
   */
  bool nestedArray = false;
  int dim = arrayType->GetDim();
  MIRType *innerType = nullptr;
  MIRArrayType *innerArrayType = nullptr;
  uint64 elemSize = 0;
  if (dim == 1) {
    innerType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(arrayType->GetElemTyIdx());
    if (innerType->GetKind() == kTypeArray) {
      nestedArray = true;
      do {
        innerArrayType = static_cast<MIRArrayType *>(innerType);
        elemSize = RoundUp(innerArrayType->GetElemType()->GetSize(),
                           arrayType->GetElemType()->GetAlign());
        dim++;
        innerType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(innerArrayType->GetElemTyIdx());
      } while (innerType->GetKind() == kTypeArray);
    }
  }

  int32 numIndex = static_cast<int>(array->NumOpnds()) - 1;
  MIRArrayType *curArrayType = arrayType;
  BaseNode *resNode = array->GetIndex(0);
  if (dim > 1) {
    BaseNode *prevNode = nullptr;
    for (int i = 0; (i < dim) && (i < numIndex); i++) {
      uint32 mpyDim = 1;
      if (nestedArray) {
        CHECK_FATAL(arrayType->GetSizeArrayItem(0) > 0, "Zero size array dimension");
        innerType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(curArrayType->GetElemTyIdx());
        curArrayType = static_cast<MIRArrayType *>(innerType);
        while (innerType->GetKind() == kTypeArray) {
          innerArrayType = static_cast<MIRArrayType *>(innerType);
          mpyDim *= innerArrayType->GetSizeArrayItem(0);
          innerType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(innerArrayType->GetElemTyIdx());
        }
      } else {
        CHECK_FATAL(arrayType->GetSizeArrayItem(static_cast<uint32>(i)) > 0, "Zero size array dimension");
        for (int j = i + 1; j < dim; j++) {
          mpyDim *= arrayType->GetSizeArrayItem(static_cast<uint32>(j));
        }
      }

      BaseNode *index = static_cast<ConstvalNode *>(array->GetIndex(static_cast<size_t>(i)));
      bool isConst = false;
      int64 indexVal = 0;
      if (index->op == OP_constval) {
        ConstvalNode *constNode = static_cast<ConstvalNode *>(index);
        indexVal = (static_cast<MIRIntConst *>(constNode->GetConstVal()))->GetValue();
        isConst = true;
        MIRIntConst *newConstNode = mirModule.GetMemPool()->New<MIRIntConst>(
            indexVal * static_cast<int64>(mpyDim),
            *GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(array->GetPrimType())));
        BaseNode *newValNode = mirModule.CurFuncCodeMemPool()->New<ConstvalNode>(newConstNode);
        newValNode->SetPrimType(array->GetPrimType());
        if (i == 0) {
          prevNode = newValNode;
          continue;
        } else {
          resNode = newValNode;
        }
      }
      if (i > 0 && isConst == false) {
        resNode = array->GetIndex(static_cast<size_t>(i));
      }

      BaseNode *mpyNode;
      if (isConst) {
        MIRIntConst *mulConst = mirModule.GetMemPool()->New<MIRIntConst>(
            static_cast<int64>(mpyDim) * indexVal,
            *GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(array->GetPrimType())));
        BaseNode *mulSize = mirModule.CurFuncCodeMemPool()->New<ConstvalNode>(mulConst);
        mulSize->SetPrimType(array->GetPrimType());
        mpyNode = mulSize;
      } else if (mpyDim == 1 && prevNode) {
        mpyNode = prevNode;
        prevNode = resNode;
      } else {
        mpyNode = mirModule.CurFuncCodeMemPool()->New<BinaryNode>(OP_mul);
        mpyNode->SetPrimType(array->GetPrimType());
        MIRIntConst *mulConst = mirModule.GetMemPool()->New<MIRIntConst>(
            mpyDim, *GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(array->GetPrimType())));
        BaseNode *mulSize = mirModule.CurFuncCodeMemPool()->New<ConstvalNode>(mulConst);
        mulSize->SetPrimType(array->GetPrimType());
        mpyNode->SetOpnd(mulSize, 1);
        PrimType signedInt4AddressCompute = GetSignedPrimType(array->GetPrimType());
        if (!IsPrimitiveInteger(resNode->GetPrimType())) {
          resNode = mirModule.CurFuncCodeMemPool()->New<TypeCvtNode>(OP_cvt, signedInt4AddressCompute,
              resNode->GetPrimType(), resNode);
        } else if (GetPrimTypeSize(resNode->GetPrimType()) != GetPrimTypeSize(array->GetPrimType())) {
          if (IsSignedInteger(resNode->GetPrimType())) {
            resNode = mirModule.CurFuncCodeMemPool()->New<TypeCvtNode>(OP_cvt, signedInt4AddressCompute,
                GetRegPrimType(resNode->GetPrimType()), resNode);
          } else {
            resNode = mirModule.CurFuncCodeMemPool()->New<TypeCvtNode>(OP_cvt, array->GetPrimType(),
                GetSignedPrimType(GetRegPrimType(resNode->GetPrimType())), resNode);
          }
        }
        mpyNode->SetOpnd(resNode, 0);
      }
      if (i == 0) {
        prevNode = mpyNode;
        continue;
      }
      BaseNode *newResNode = mirModule.CurFuncCodeMemPool()->New<BinaryNode>(OP_add);
      newResNode->SetPrimType(array->GetPrimType());
      newResNode->SetOpnd(mpyNode, 0);
      if (prevNode->GetPrimType() != array->GetPrimType()) {
        prevNode = mirModule.CurFuncCodeMemPool()->New<TypeCvtNode>(OP_cvt, array->GetPrimType(),
            GetSignedPrimType(GetRegPrimType(prevNode->GetPrimType())), prevNode);
      }
      newResNode->SetOpnd(prevNode, 1);
      prevNode = newResNode;
    }
    resNode = prevNode;
  }

  BaseNode *rMul = nullptr;
  // esize is the size of the array element (eg. int = 4 long = 8)
  uint64 esize;
  if (nestedArray) {
    esize = elemSize;
  } else {
    esize = arrayType->GetElemType()->GetSize();
  }
  Opcode opadd = OP_add;
  MIRIntConst *econst = mirModule.GetMemPool()->New<MIRIntConst>(esize,
      *GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(array->GetPrimType())));
  BaseNode *eSize = mirModule.CurFuncCodeMemPool()->New<ConstvalNode>(econst);
  eSize->SetPrimType(array->GetPrimType());
  rMul = mirModule.CurFuncCodeMemPool()->New<BinaryNode>(OP_mul);
  PrimType signedInt4AddressCompute = GetSignedPrimType(array->GetPrimType());
  if (!IsPrimitiveInteger(resNode->GetPrimType())) {
    resNode = mirModule.CurFuncCodeMemPool()->New<TypeCvtNode>(OP_cvt, signedInt4AddressCompute,
        resNode->GetPrimType(), resNode);
  } else if (GetPrimTypeSize(resNode->GetPrimType()) != GetPrimTypeSize(array->GetPrimType())) {
    if (IsSignedInteger(resNode->GetPrimType())) {
      resNode = mirModule.CurFuncCodeMemPool()->New<TypeCvtNode>(OP_cvt, signedInt4AddressCompute,
          GetRegPrimType(resNode->GetPrimType()), resNode);
    } else {
      resNode = mirModule.CurFuncCodeMemPool()->New<TypeCvtNode>(OP_cvt, array->GetPrimType(),
          GetSignedPrimType(GetRegPrimType(resNode->GetPrimType())), resNode);
    }
  }
  rMul->SetPrimType(resNode->GetPrimType());
  rMul->SetOpnd(resNode, 0);
  rMul->SetOpnd(eSize, 1);
  BaseNode *baseNode = array->GetBase();
  BaseNode *rAdd = mirModule.CurFuncCodeMemPool()->New<BinaryNode>(opadd);
  rAdd->SetPrimType(array->GetPrimType());
  rAdd->SetOpnd(baseNode, 0);
  rAdd->SetOpnd(rMul, 1);
  auto *newAdd = ConstantFold(mirModule).Fold(rAdd);
  rAdd = (newAdd != nullptr ? newAdd : rAdd);
  return rAdd;
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
