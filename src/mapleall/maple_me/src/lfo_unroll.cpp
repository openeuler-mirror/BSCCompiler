/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "lfo_unroll.h"
#include "me_loop_analysis.h"

namespace maple {
uint32 LfoUnrollOneLoop::countOfLoopsUnrolled = 0;

constexpr size_t unrolledSizeLimit = 12;  // unrolled loop body size to be < this value
constexpr size_t unrollMax = 8;           // times to unroll never more than this

BaseNode *LfoUnrollOneLoop::CloneIVNode() {
  if (doloop->IsPreg()) {
    return mirBuilder->CreateExprRegread(ivPrimType, doloop->GetDoVarPregIdx());
  } else {
    return codeMP->New<AddrofNode>(OP_dread, ivPrimType, doloop->GetDoVarStIdx(), 0);
  }
}

bool LfoUnrollOneLoop::IsIVNode(BaseNode *x) const {
  if (doloop->IsPreg()) {
    if (x->GetOpCode() != OP_regread) {
      return false;
    }
    return static_cast<RegreadNode *>(x)->GetRegIdx() == doloop->GetDoVarPregIdx();
  } else {
    if (x->GetOpCode() != OP_dread) {
      return false;
    }
    return static_cast<AddrofNode *>(x)->GetStIdx() == doloop->GetDoVarStIdx();
  }
}

// replace any occurrence of the IV in x with a copy of repNode
void LfoUnrollOneLoop::ReplaceIV(BaseNode *x, BaseNode *repNode) {
  if (x->GetOpCode() == OP_block) {
    BlockNode *blk = static_cast<BlockNode *>(x);
    StmtNode *stmt = blk->GetFirst();
    while (stmt != nullptr) {
      ReplaceIV(stmt, repNode);
      stmt = stmt->GetNext();
    }
    return;
  }
  for (size_t i = 0; i < x->NumOpnds(); i++) {
    if (IsIVNode(x->Opnd(i))) {
      x->SetOpnd(repNode, i);
    } else {
      ReplaceIV(x->Opnd(i), repNode);
    }
  }
}

BlockNode *LfoUnrollOneLoop::DoFullUnroll(size_t tripCount) {
  BlockNode *unrolledBlk = doloop->GetDoBody()->CloneTreeWithSrcPosition(*mirModule);
  ReplaceIV(unrolledBlk, doloop->GetStartExpr());
  BlockNode *nextIterBlk = nullptr;
  tripCount--;
  uint32 i = 0;
  while (tripCount > 0) {
    i++;
    nextIterBlk = doloop->GetDoBody()->CloneTreeWithSrcPosition(*mirModule);
    BaseNode *adjExpr = mirBuilder->CreateIntConst(stepAmount * i, ivPrimType);
    BaseNode *repExpr = codeMP->New<BinaryNode>(OP_add, ivPrimType, doloop->GetStartExpr(), adjExpr);
    ReplaceIV(nextIterBlk, repExpr);
    unrolledBlk->InsertBlockAfter(*nextIterBlk, unrolledBlk->GetLast());
    tripCount--;
  }
  return unrolledBlk;
}

// only handling constant trip count for now
BlockNode *LfoUnrollOneLoop::DoUnroll(size_t times, size_t tripCount) {
  BlockNode *unrolledBlk = nullptr;
  // form the remainder loop before the unrolled loop
  size_t remainderTripCount = tripCount % times; // used when constant tripcount
  PregIdx regIdx = 0;   // used when unknown tripcount
  if (tripCount != 0) {
    if (remainderTripCount == 0) {
      unrolledBlk = codeMP->New<BlockNode>();
    } else if (remainderTripCount == 1) {
      unrolledBlk = doloop->GetDoBody()->CloneTreeWithSrcPosition(*mirModule);
      ReplaceIV(unrolledBlk, doloop->GetStartExpr());
    } else {
      DoloopNode *remDoloop = doloop->CloneTree(*preEmit->GetCodeMPAlloc());
      // generate remDoloop's termination
      BaseNode *terminationRHS = codeMP->New<BinaryNode>(OP_add, ivPrimType,
          doloop->GetStartExpr()->CloneTree(*preEmit->GetCodeMPAlloc()),
          mirBuilder->CreateIntConst(static_cast<int64>(remainderTripCount), ivPrimType));
      remDoloop->SetContExpr(codeMP->New<CompareNode>(OP_lt, PTY_i32, ivPrimType, CloneIVNode(), terminationRHS));
      unrolledBlk = codeMP->New<BlockNode>();
      unrolledBlk->AddStatement(remDoloop);
    }
  } else {
    // generate code to calculate the remainder loop trip count which is
    // (endexpr - startexpr) % times (increment assumed to be 1)
    BaseNode *startExpr = doloop->GetStartExpr();
    BaseNode *condExpr = doloop->GetCondExpr();
    BaseNode *endExpr = condExpr->Opnd(1);
    BaseNode *tripsExpr = codeMP->New<BinaryNode>(OP_sub, ivPrimType,
        endExpr->CloneTree(*preEmit->GetCodeMPAlloc()),
        startExpr->CloneTree(*preEmit->GetCodeMPAlloc()));
    if (condExpr->GetOpCode() == OP_ge || condExpr->GetOpCode() == OP_le) {
      tripsExpr = codeMP->New<BinaryNode>(OP_add, ivPrimType, tripsExpr,
          mirBuilder->CreateIntConst(1, ivPrimType));
    }
    tripsExpr = codeMP->New<BinaryNode>(OP_rem, ivPrimType, tripsExpr,
        mirBuilder->CreateIntConst(static_cast<int64>(times), ivPrimType));
    BaseNode *remLoopEndExpr = codeMP->New<BinaryNode>(OP_add, ivPrimType,
        startExpr->CloneTree(*preEmit->GetCodeMPAlloc()), tripsExpr);
    // store in a preg
    regIdx = preMeFunc->meFunc->GetMirFunc()->GetPregTab()->CreatePreg(ivPrimType);
    RegassignNode *rass = mirBuilder->CreateStmtRegassign(ivPrimType, regIdx, remLoopEndExpr);
    unrolledBlk = codeMP->New<BlockNode>();
    unrolledBlk->AddStatement(rass);

    DoloopNode *remDoloop = doloop->CloneTree(*preEmit->GetCodeMPAlloc());
    // generate remDoloop's termination
    BaseNode *terminationRHS = mirBuilder->CreateExprRegread(ivPrimType, regIdx);
    remDoloop->SetContExpr(codeMP->New<CompareNode>(OP_lt, PTY_i32, ivPrimType, CloneIVNode(), terminationRHS));
    unrolledBlk->AddStatement(remDoloop);
  }

  // form the unrolled loop
  DoloopNode *unrolledDoloop = doloop->CloneTree(*preEmit->GetCodeMPAlloc());
  uint32 i = 1;
  BlockNode *nextIterBlk = nullptr;
  do {
    nextIterBlk = doloop->GetDoBody()->CloneTreeWithSrcPosition(*mirModule);
    BaseNode *adjExpr = mirBuilder->CreateIntConst(stepAmount * i, ivPrimType);
    BaseNode *repExpr = codeMP->New<BinaryNode>(OP_add, ivPrimType, CloneIVNode(), adjExpr);
    ReplaceIV(nextIterBlk, repExpr);
    unrolledDoloop->GetDoBody()->InsertBlockAfter(*nextIterBlk, unrolledDoloop->GetDoBody()->GetLast());
    i++;
  } while (i != times);
  // update startExpr
  if (tripCount != 0) {
    if (remainderTripCount != 0) {
      BaseNode *newStartExpr = codeMP->New<BinaryNode>(OP_add, ivPrimType, unrolledDoloop->GetStartExpr(),
          mirBuilder->CreateIntConst(static_cast<int64>(remainderTripCount), ivPrimType));
      unrolledDoloop->SetStartExpr(newStartExpr);
    }
  } else {
    BaseNode *newStartExpr = mirBuilder->CreateExprRegread(ivPrimType, regIdx);
    unrolledDoloop->SetStartExpr(newStartExpr);
  }
  // update incrExpr
  ConstvalNode *stepNode = static_cast<ConstvalNode *>(unrolledDoloop->GetIncrExpr());
  int64 origIncr = static_cast<MIRIntConst *>(stepNode->GetConstVal())->GetValue();
  unrolledDoloop->SetIncrExpr(mirBuilder->CreateIntConst(static_cast<int64>(origIncr * times), ivPrimType));
  unrolledBlk->AddStatement(unrolledDoloop);
  return unrolledBlk;
}

static size_t CountBlockStmts(BlockNode *blk) {
  if (blk == nullptr) {
    return 0;
  }
  size_t stmtCount = 0;
  StmtNode *stmt = blk->GetFirst();
  while (stmt != nullptr) {
    if (stmt->GetOpCode() == OP_block) {
      stmtCount += CountBlockStmts(static_cast<BlockNode *>(stmt));
    } else if (stmt->GetOpCode() != OP_label && stmt->GetOpCode() != OP_comment) {
      stmtCount++;
      if (stmt->GetOpCode() == OP_if) {
        stmtCount += 3 + CountBlockStmts(static_cast<IfStmtNode *>(stmt)->GetThenPart());
        stmtCount += 3 + CountBlockStmts(static_cast<IfStmtNode *>(stmt)->GetElsePart());
      }
    }
    ASSERT(stmt->GetOpCode() != OP_switch && stmt->GetOpCode() != OP_while &&
           stmt->GetOpCode() != OP_dowhile && stmt->GetOpCode() != OP_doloop,
           "CountBlockStmts: unexpected statement type");
    stmt = stmt->GetNext();
  }
  return stmtCount;
}

void LfoUnrollOneLoop::Process() {
  if (doloopInfo->hasOtherCtrlFlow || doloopInfo->hasBeenVectorized || doloopInfo->hasLabels) {
    return;
  }
  if (!doloop->GetIncrExpr()->IsConstval()) {
    return;
  }
  ivPrimType = doloop->GetStartExpr()->GetPrimType();
  ConstvalNode *cvalnode = static_cast<ConstvalNode *>(doloop->GetIncrExpr());
  if (!cvalnode->GetConstVal()->IsOne()) {
    return;
  }
  size_t stmtCount = CountBlockStmts(doloop->GetDoBody());
  if (stmtCount == 0 || stmtCount > 16) {
    return;
  }

  // screen doloop condExpr
  BaseNode *condExpr = doloop->GetCondExpr();
  if (!kOpcodeInfo.IsCompare(condExpr->GetOpCode())) {
    return;
  }
  if (condExpr->GetOpCode() == OP_eq) {
    return;
  }
  if (!IsIVNode(condExpr->Opnd(0))) {
    return;
  }
  BaseNode *endExpr = condExpr->Opnd(1);

  // screen doloop incrExpr
  if (!doloop->GetIncrExpr()->IsConstval()) {
    return;
  }
  ConstvalNode *stepNode = static_cast<ConstvalNode *>(doloop->GetIncrExpr());
  MIRIntConst *stepConst = static_cast<MIRIntConst *>(stepNode->GetConstVal());
  stepAmount = stepConst->GetValue();

  size_t tripCount = 0;         // 0 if not constant trip count
  if (doloop->GetStartExpr()->IsConstval() && endExpr->IsConstval()) {
    ConstvalNode *startNode = static_cast<ConstvalNode *>(doloop->GetStartExpr());
    MIRIntConst *startConst = static_cast<MIRIntConst *>(startNode->GetConstVal());
    ConstvalNode *endNode = static_cast<ConstvalNode *>(endExpr);
    MIRIntConst *endConst = static_cast<MIRIntConst *>(endNode->GetConstVal());
    tripCount = (endConst->GetValue() - startConst->GetValue()) / stepAmount;
    if (condExpr->GetOpCode() == OP_ge || condExpr->GetOpCode() == OP_le) {
      tripCount++;
    }
  }
  size_t unrollTimes = 1;
  size_t unrolledStmtCount = stmtCount;
  while (unrolledStmtCount < unrolledSizeLimit && unrollTimes < unrollMax) {
    unrollTimes++;
    unrolledStmtCount += stmtCount;
  }
  bool fullUnroll = tripCount != 0 && tripCount < (unrollTimes * 2);
  BlockNode *unrolledBlk = nullptr;
  if (fullUnroll) {
    unrolledBlk = DoFullUnroll(tripCount);
  } else {
    if (unrollTimes == 1) {
      return;
    }
    unrolledBlk = DoUnroll(unrollTimes, tripCount);
  }

  // replace doloop by the statements in unrolledBlk
  PreMeMIRExtension *lfopart = (*preEmit->GetPreMeStmtExtensionMap())[doloop->GetStmtID()];
  BaseNode *parent = lfopart->GetParent();
  ASSERT(parent && (parent->GetOpCode() == OP_block), "LfoUnroll: parent of doloop is not OP_block");
  BlockNode *pblock = static_cast<BlockNode *>(parent);
  pblock->ReplaceStmtWithBlock(*doloop, *unrolledBlk);

  // update counter
  countOfLoopsUnrolled++;
};

bool MELfoUnroll::PhaseRun(MeFunction &f) {
  PreMeEmitter *preEmit = GET_ANALYSIS(MEPreMeEmission, f);
  ASSERT(preEmit != nullptr, "lfo preemit phase has problem");
  LfoDepInfo *lfoDepInfo = GET_ANALYSIS(MELfoDepTest, f);
  ASSERT(lfoDepInfo != nullptr, "lfo dep test phase has problem");
  PreMeFunction *preMeFunc = f.GetPreMeFunc();
  uint32 savedCountOfLoopsUnrolled = LfoUnrollOneLoop::countOfLoopsUnrolled;

  MapleMap<DoloopNode *, DoloopInfo *>::iterator mapit = lfoDepInfo->doloopInfoMap.begin();
  for (; mapit != lfoDepInfo->doloopInfoMap.end(); ++mapit) {
    if (!mapit->second->children.empty() || mapit->second->hasBeenVectorized) {
      continue;
    }
    LfoUnrollOneLoop unroll(preMeFunc, preEmit, mapit->second);
    unroll.Process();
  }
  if (DEBUGFUNC_NEWPM(f) && savedCountOfLoopsUnrolled != LfoUnrollOneLoop::countOfLoopsUnrolled) {
    LogInfo::MapleLogger() << "\n**** After lfo loop unrolling ****\n";
    f.GetMirFunc()->Dump();
  }
  // cfg is invalid for now
  f.SetTheCfg(nullptr);
  return false;
}

void MELfoUnroll::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEPreMeEmission>();
  aDep.AddRequired<MELfoDepTest>();
  aDep.PreservedAllExcept<MEMeCfg>();
  aDep.PreservedAllExcept<MEDominance>();
  aDep.PreservedAllExcept<MELoopAnalysis>();
}
}  // namespace maple
