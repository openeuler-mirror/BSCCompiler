/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "java_eh_lower.h"
#include "mir_function.h"
#include "mir_builder.h"
#include "global_tables.h"
#include "option.h"

namespace {
const std::string strDivOpnd = "__div_opnd1";
const std::string strDivRes = "__div_res";
const std::string strMCCThrowArrayIndexOutOfBoundsException = "MCC_ThrowArrayIndexOutOfBoundsException";
const std::string strMCCThrowNullPointerException = "MCC_ThrowNullPointerException";
} // namespace

// Do exception handling runtime insertion of runtime function call
// scan the entire function body once to lookup expression that
// could potentially raise exceptions such as division,
// for example:
// if we have x = a/b
// and we don't know the value of b during compile time
// then we will insert the test for exception:
// if b == 0,
//  call MCC_ThrowArithmeticException()
// x = a/b
namespace maple {
BaseNode *JavaEHLowerer::DoLowerDiv(BinaryNode &expr, BlockNode &blknode) {
  PrimType ptype = expr.GetPrimType();
  MIRBuilder *mirBuilder = GetMIRModule().GetMIRBuilder();
  MIRFunction *func = GetMIRModule().CurFunction();
  if (!IsPrimitiveInteger(ptype)) {
    return &expr;
  }

  // Store divopnd to a tmp st if not a leaf node.
  BaseNode *divOpnd = expr.Opnd(1);
  if (!divOpnd->IsLeaf()) {
    std::string opnd1name(strDivOpnd);
    opnd1name.append(std::to_string(divSTIndex));
    if (useRegTmp) {
      PregIdx pregIdx = func->GetPregTab()->CreatePreg(ptype);
      RegassignNode *regassDivnode = mirBuilder->CreateStmtRegassign(ptype, pregIdx, divOpnd);
      blknode.AddStatement(regassDivnode);
      divOpnd = mirBuilder->CreateExprRegread(ptype, pregIdx);
    } else {
      MIRSymbol *divOpndSymbol = mirBuilder->CreateSymbol(TyIdx(ptype), opnd1name, kStVar, kScAuto,
                                                          GetMIRModule().CurFunction(), kScopeLocal);
      DassignNode *dssDivNode = mirBuilder->CreateStmtDassign(*divOpndSymbol, 0, divOpnd);
      blknode.AddStatement(dssDivNode);
      divOpnd = mirBuilder->CreateExprDread(*divOpndSymbol);
    }
    expr.SetBOpnd(divOpnd, 1);
  }
  BaseNode *retExprNode = nullptr;
  StmtNode *divStmt = nullptr;
  if (useRegTmp) {
    PregIdx resPregIdx = func->GetPregTab()->CreatePreg(ptype);
    divStmt = mirBuilder->CreateStmtRegassign(ptype, resPregIdx, &expr);
    retExprNode = GetMIRModule().GetMIRBuilder()->CreateExprRegread(ptype, resPregIdx);
  } else {
    std::string resName(strDivRes);
    resName.append(std::to_string(divSTIndex++));
    MIRSymbol *divResSymbol = mirBuilder->CreateSymbol(TyIdx(ptype), resName, kStVar, kScAuto,
                                                       GetMIRModule().CurFunction(), kScopeLocal);
    // Put expr result to dssnode.
    divStmt = mirBuilder->CreateStmtDassign(*divResSymbol, 0, &expr);
    retExprNode = GetMIRModule().GetMIRBuilder()->CreateExprDread(*divResSymbol, 0);
  }
  // Check if the second operand of the div expression is 0.
  // Inser if statement for high level ir.
  CompareNode *cmpNode = mirBuilder->CreateExprCompare(OP_eq, *GlobalTables::GetTypeTable().GetInt32(),
                                                       *GlobalTables::GetTypeTable().GetTypeFromTyIdx((TyIdx)ptype),
                                                       divOpnd, mirBuilder->CreateIntConst(0, ptype));
  IfStmtNode *ifStmtNode = mirBuilder->CreateStmtIf(cmpNode);
  blknode.AddStatement(ifStmtNode);
  // Call the MCC_ThrowArithmeticException() that will never return.
  MapleVector<BaseNode*> args(GetMIRModule().GetMIRBuilder()->GetCurrentFuncCodeMpAllocator()->Adapter());
  IntrinsiccallNode *intrinCallNode = mirBuilder->CreateStmtIntrinsicCall(INTRN_JAVA_THROW_ARITHMETIC, args);
  ifStmtNode->GetThenPart()->AddStatement(intrinCallNode);
  blknode.AddStatement(divStmt);
  // Make dread from the divresst and return it as new expression for this function.
  return retExprNode;
}

BaseNode *JavaEHLowerer::DoLowerExpr(BaseNode &expr, BlockNode &curblk) {
  for (size_t i = 0; i < expr.NumOpnds(); ++i) {
    expr.SetOpnd(DoLowerExpr(*(expr.Opnd(i)), curblk), i);
  }
  switch (expr.GetOpCode()) {
    case OP_div: {
      return DoLowerDiv(static_cast<BinaryNode&>(expr), curblk);
    }
    case OP_rem: {
      return DoLowerRem(static_cast<BinaryNode&>(expr), curblk);
    }
    default:
      return &expr;
  }
}

void JavaEHLowerer::DoLowerBoundaryCheck(IntrinsiccallNode &intrincall, BlockNode &newblk) {
  const size_t intrincallNopndSize = intrincall.GetNopndSize();
  CHECK_FATAL(intrincallNopndSize > 0, "null ptr check");
  CondGotoNode *brFalseStmt = GetMIRModule().CurFuncCodeMemPool()->New<CondGotoNode>(OP_brfalse);
  brFalseStmt->SetOpnd(DoLowerExpr(*(intrincall.GetNopndAt(0)), newblk), 0);
  brFalseStmt->SetSrcPos(intrincall.GetSrcPos());
  LabelIdx lbidx = GetMIRModule().CurFunction()->GetLabelTab()->CreateLabel();
  GetMIRModule().CurFunction()->GetLabelTab()->AddToStringLabelMap(lbidx);
  brFalseStmt->SetOffset(lbidx);
  newblk.AddStatement(brFalseStmt);
  LabelNode *labStmt = GetMIRModule().CurFuncCodeMemPool()->New<LabelNode>();
  labStmt->SetLabelIdx(lbidx);
  MIRFunction *func =
    GetMIRModule().GetMIRBuilder()->GetOrCreateFunction(strMCCThrowArrayIndexOutOfBoundsException, TyIdx(PTY_void));
  MapleVector<BaseNode*> args(GetMIRModule().GetMIRBuilder()->GetCurrentFuncCodeMpAllocator()->Adapter());
  CallNode *callStmt = GetMIRModule().GetMIRBuilder()->CreateStmtCall(func->GetPuidx(), args);
  newblk.AddStatement(callStmt);
  newblk.AddStatement(labStmt);
}

BlockNode *JavaEHLowerer::DoLowerBlock(BlockNode &block) {
  BlockNode *newBlock = GetMIRModule().CurFuncCodeMemPool()->New<BlockNode>();
  StmtNode *nextStmt = block.GetFirst();
  if (nextStmt == nullptr) {
    return newBlock;
  }

  do {
    StmtNode *stmt = nextStmt;
    nextStmt = stmt->GetNext();
    stmt->SetNext(nullptr);

    switch (stmt->GetOpCode()) {
      case OP_switch: {
        auto *switchNode = static_cast<SwitchNode*>(stmt);
        switchNode->SetSwitchOpnd(DoLowerExpr(*(switchNode->GetSwitchOpnd()), *newBlock));
        newBlock->AddStatement(switchNode);
        break;
      }
      case OP_if: {
        auto *ifStmtNode = static_cast<IfStmtNode*>(stmt);
        BlockNode *thenPart = ifStmtNode->GetThenPart();
        BlockNode *elsePart = ifStmtNode->GetElsePart();
        ASSERT(ifStmtNode->Opnd() != nullptr, "null ptr check!");
        ifStmtNode->SetOpnd(DoLowerExpr(*(ifStmtNode->Opnd()), *newBlock), 0);
        ifStmtNode->SetThenPart(DoLowerBlock(*thenPart));
        if (elsePart != nullptr) {
          ifStmtNode->SetElsePart(DoLowerBlock(*elsePart));
        }
        newBlock->AddStatement(ifStmtNode);
        break;
      }
      case OP_while:
      case OP_dowhile: {
        auto *whileStmtNode = static_cast<WhileStmtNode*>(stmt);
        BaseNode *testOpnd = whileStmtNode->Opnd(0);
        whileStmtNode->SetOpnd(DoLowerExpr(*testOpnd, *newBlock), 0);
        whileStmtNode->SetBody(DoLowerBlock(*(whileStmtNode->GetBody())));
        newBlock->AddStatement(whileStmtNode);
        break;
      }
      case OP_doloop: {
        auto *doLoopNode = static_cast<DoloopNode*>(stmt);
        doLoopNode->SetStartExpr(DoLowerExpr(*(doLoopNode->GetStartExpr()), *newBlock));
        doLoopNode->SetContExpr(DoLowerExpr(*(doLoopNode->GetCondExpr()), *newBlock));
        doLoopNode->SetIncrExpr(DoLowerExpr(*(doLoopNode->GetIncrExpr()), *newBlock));
        doLoopNode->SetDoBody(DoLowerBlock(*(doLoopNode->GetDoBody())));
        newBlock->AddStatement(doLoopNode);
        break;
      }
      case OP_block: {
        auto *tmp = DoLowerBlock(*(static_cast<BlockNode*>(stmt)));
        CHECK_FATAL(tmp != nullptr, "null ptr check");
        newBlock->AddStatement(tmp);
        break;
      }
      case OP_throw: {
        auto *tstmt = static_cast<UnaryStmtNode*>(stmt);
        BaseNode *opnd0 = DoLowerExpr(*(tstmt->Opnd(0)), *newBlock);
        if (opnd0->GetOpCode() == OP_constval) {
          CHECK_FATAL(IsPrimitiveInteger(opnd0->GetPrimType()), "must be integer or something wrong");
          auto *intConst = safe_cast<MIRIntConst>(static_cast<ConstvalNode*>(opnd0)->GetConstVal());
          CHECK_FATAL(intConst->IsZero(), "can only be zero");
          MIRFunction *func =
            GetMIRModule().GetMIRBuilder()->GetOrCreateFunction(strMCCThrowNullPointerException, TyIdx(PTY_void));
          func->SetNoReturn();
          MapleVector<BaseNode*> args(GetMIRModule().GetMIRBuilder()->GetCurrentFuncCodeMpAllocator()->Adapter());
          CallNode *callStmt = GetMIRModule().GetMIRBuilder()->CreateStmtCall(func->GetPuidx(), args);
          newBlock->AddStatement(callStmt);
        } else {
          tstmt->SetOpnd(opnd0, 0);
          newBlock->AddStatement(tstmt);
        }
        break;
      }
      case OP_intrinsiccall: {
        auto *intrinCall = static_cast<IntrinsiccallNode*>(stmt);
        if (intrinCall->GetIntrinsic() == INTRN_MPL_BOUNDARY_CHECK) {
          DoLowerBoundaryCheck(*intrinCall, *newBlock);
          break;
        }
        // fallthrough;
      }
      [[clang::fallthrough]];
      default: {
        for (size_t i = 0; i < stmt->NumOpnds(); ++i) {
          stmt->SetOpnd(DoLowerExpr(*(stmt->Opnd(i)), *newBlock), i);
        }
        newBlock->AddStatement(stmt);
        break;
      }
    }
  } while (nextStmt != nullptr);
  return newBlock;
}

void JavaEHLowerer::ProcessFunc(MIRFunction *func) {
  GetMIRModule().SetCurFunction(func);
  if (func->GetBody() == nullptr) {
    return;
  }
  divSTIndex = 0;  // Init it to 0.
  BlockNode *newBody = DoLowerBlock(*(func->GetBody()));
  func->SetBody(newBody);
}

bool M2MJavaEHLowerer::PhaseRun(maple::MIRModule &m) {
  OPT_TEMPLATE_NEWPM(JavaEHLowerer, m);
  return true;
}

void M2MJavaEHLowerer::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<M2MKlassHierarchy>();
  aDep.SetPreservedAll();
}
}  // namespace maple
