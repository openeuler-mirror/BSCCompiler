/*
 * Copyright (c) [2021] Futurewei Technologies Co., Ltd. All rights reserved.
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
#include "ext_constantfold.h"
#include <climits>

namespace maple {
// This class is designed to further identify simplification
// patterns that have not been covered in ConstantFold.

StmtNode *ExtConstantFold::ExtSimplify(StmtNode *node) {
  CHECK_NULL_FATAL(node);
  switch (node->GetOpCode()) {
    case OP_block:
      return ExtSimplifyBlock(static_cast<BlockNode*>(node));
    case OP_if:
      return ExtSimplifyIf(static_cast<IfStmtNode*>(node));
    case OP_dassign:
      return ExtSimplifyDassign(static_cast<DassignNode*>(node));
    case OP_iassign:
      return ExtSimplifyIassign(static_cast<IassignNode*>(node));
    case OP_dowhile:
    case OP_while:
      return ExtSimplifyWhile(static_cast<WhileStmtNode*>(node));
    default:
      return node;
  }
}

BaseNode* ExtConstantFold::DispatchFold(BaseNode *node) {
  CHECK_NULL_FATAL(node);

  switch (node->GetOpCode()) {
    case OP_cior:
    case OP_lior:
      return FoldIor(static_cast<BinaryNode*>(node));
    default:
      return node;
  }
}

BaseNode *ExtConstantFold::Fold(BaseNode *node) {
  if (node == nullptr || kOpcodeInfo.IsStmt(node->GetOpCode())) {
    return nullptr;
  }
  return DispatchFold(node);
}

BaseNode *ExtConstantFold::FoldIor(BinaryNode *node) {
  CHECK_NULL_FATAL(node);
  // The target pattern (Cior, Lior):
  // x == c || x == c+1 || ... || x == c+k
  // ==> le (x - c), k
  // where c is int. i
  // Leave other cases for future including extended simplification of partial expressions
  std::queue<BaseNode *> operands;
  std::vector<int64> uniqOperands;
  operands.push(node);
  int64 minVal = LLONG_MAX;
  bool isWorkable = true;
  BaseNode* lNode = nullptr;

  while (!operands.empty()) {
    BaseNode *operand = operands.front();
    operands.pop();
    Opcode op = operand->GetOpCode();
    if (op == OP_cior || op == OP_lior) {
      operands.push(static_cast<BinaryNode *>(operand)->GetBOpnd(0));
      operands.push(static_cast<BinaryNode *>(operand)->GetBOpnd(1));
    } else if (op == OP_eq) {
      BinaryNode *bNode = static_cast<BinaryNode *>(operand);
      if (lNode == nullptr) {
        if (bNode->Opnd(0)->GetOpCode() == OP_dread ||
            bNode->Opnd(0)->GetOpCode() == OP_iread) {
          lNode = bNode->Opnd(0);
        } else {
          // Consider other cases in future
          isWorkable = false;
          break;
        }
      }

      if ((lNode->IsSameContent(bNode->Opnd(0))) &&
          (bNode->Opnd(1)->GetOpCode() == OP_constval) &&
          (IsPrimitiveInteger(bNode->Opnd(1)->GetPrimType()))) {
        MIRConst *rConstVal = safe_cast<ConstvalNode>(bNode->Opnd(1))->GetConstVal();
        int64 rVal = static_cast<MIRIntConst*>(rConstVal)->GetValue();
        minVal = std::min(minVal, rVal);
        uniqOperands.push_back(rVal);
      } else {
        isWorkable = false;
        break;
      }
    } else {
      isWorkable = false;
      break;
    }
  }

  if (isWorkable) {
    std::sort(uniqOperands.begin(), uniqOperands.end());
    uniqOperands.erase(std::unique(uniqOperands.begin(), uniqOperands.end()), uniqOperands.end());
    if ((uniqOperands.size() >= 2) &&
        (uniqOperands[uniqOperands.size() - 1] == uniqOperands[0] + uniqOperands.size() - 1)) {
      PrimType nPrimType = node->GetPrimType();
      BaseNode* diffVal;
      ConstvalNode *lowVal = mirModule->GetMIRBuilder()->CreateIntConst(minVal, nPrimType);
      diffVal = mirModule->CurFuncCodeMemPool()->New<BinaryNode>(OP_sub, nPrimType, lNode, lowVal);
      PrimType cmpPrimType = (nPrimType == PTY_i64 || nPrimType == PTY_u64) ? PTY_u64 : PTY_u32;
      MIRType *cmpMirType = (nPrimType == PTY_i64 || nPrimType == PTY_u64) ?
                            GlobalTables::GetTypeTable().GetUInt64() :
                            GlobalTables::GetTypeTable().GetUInt32();
      ConstvalNode *deltaVal = mirModule->GetMIRBuilder()->CreateIntConst(uniqOperands.size() - 1, cmpPrimType);
      CompareNode *result;
      result = mirModule->GetMIRBuilder()->CreateExprCompare(OP_le, *cmpMirType, *cmpMirType, diffVal, deltaVal);
      return result;
    } else {
      return node;
    }
  } else {
    return node;
  }
}

StmtNode *ExtConstantFold::ExtSimplifyBlock(BlockNode *node) {
  CHECK_NULL_FATAL(node);
  if (node->GetFirst() == nullptr) {
    return node;
  }
  StmtNode *s = node->GetFirst();
  do {
    (void)ExtSimplify(s);
    s = s->GetNext();;
  } while (s != nullptr);
  return node;
}

StmtNode *ExtConstantFold::ExtSimplifyIf(IfStmtNode *node) {
  CHECK_NULL_FATAL(node);
  (void)ExtSimplify(node->GetThenPart());
  if (node->GetElsePart()) {
    (void)ExtSimplify(node->GetElsePart());
  }
  BaseNode *origTest = node->Opnd();
  BaseNode *returnValue = Fold(node->Opnd());
  if (returnValue != origTest) {
    node->SetOpnd(returnValue, 0);
  }
  return node;
}

StmtNode *ExtConstantFold::ExtSimplifyDassign(DassignNode *node) {
  CHECK_NULL_FATAL(node);
  BaseNode *returnValue;
  returnValue = Fold(node->GetRHS());
  if (returnValue != node->GetRHS()) {
    node->SetRHS(returnValue);
  }
  return node;
}

StmtNode *ExtConstantFold::ExtSimplifyIassign(IassignNode *node) {
  CHECK_NULL_FATAL(node);
  BaseNode *returnValue;
  returnValue = Fold(node->GetRHS());
  if (returnValue != node->GetRHS()) {
    node->SetRHS(returnValue);
  }
  return node;
}

StmtNode *ExtConstantFold::ExtSimplifyWhile(WhileStmtNode *node) {
  CHECK_NULL_FATAL(node);
  BaseNode *returnValue;
  if (node->Opnd(0) == nullptr) {
    return node;
  }
  returnValue = Fold(node->Opnd(0));
  if (returnValue != node->Opnd(0)) {
    node->SetOpnd(returnValue, 0);
  }
  return node;
}
}  // namespace maple
