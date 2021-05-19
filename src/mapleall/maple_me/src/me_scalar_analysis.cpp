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
#include "me_scalar_analysis.h"
#include <iostream>
#include <algorithm>
#include "me_loop_analysis.h"

namespace maple {
bool LoopScalarAnalysisResult::enableDebug = false;
constexpr int kNumOpnds = 2;

CRNode *CR::GetPolynomialsValueAtITerm(CRNode &iterCRNode, size_t num, LoopScalarAnalysisResult &scalarAnalysis) const {
  if (num == 1) {
    return &iterCRNode;
  }
  // BC(It, K) = (It * (It - 1) * ... * (It - K + 1)) / K!;
  size_t factorialOfNum = 1;
  // compute K!
  for (size_t i = 2; i <= num; ++i) {
    factorialOfNum *= i;
  }
  CRConstNode factorialOfNumCRNode = CRConstNode(nullptr, factorialOfNum);
  CRNode *result = &iterCRNode;
  for (size_t i = 1; i != num; ++i) {
    std::vector<CRNode*> crAddNodes;
    crAddNodes.push_back(&iterCRNode);
    CRConstNode constNode = CRConstNode(nullptr, i);
    CRNode *negetive2Mul = scalarAnalysis.ChangeNegative2MulCRNode(*static_cast<CRNode*>(&constNode));
    crAddNodes.push_back(negetive2Mul);
    CRNode *addResult = scalarAnalysis.GetCRAddNode(nullptr, crAddNodes);
    std::vector<CRNode*> crMulNodes;
    crMulNodes.push_back(result);
    crMulNodes.push_back(addResult);
    result = scalarAnalysis.GetCRMulNode(nullptr, crMulNodes);
  }
  return scalarAnalysis.GetOrCreateCRDivNode(nullptr, *result, *static_cast<CRNode*>(&factorialOfNumCRNode));
}

CRNode *CR::ComputeValueAtIteration(uint32 i, LoopScalarAnalysisResult &scalarAnalysis) const {
  CRConstNode iterCRNode(nullptr, i);
  CRNode *result = opnds[0];
  CRNode *subResult = nullptr;
  for (size_t j = 1; j < opnds.size(); ++j) {
    subResult = GetPolynomialsValueAtITerm(iterCRNode, j, scalarAnalysis);
    CHECK_NULL_FATAL(subResult);
    std::vector<CRNode*> crMulNodes;
    crMulNodes.push_back(opnds[j]);
    crMulNodes.push_back(subResult);
    CRNode *mulResult = scalarAnalysis.GetCRMulNode(nullptr, crMulNodes);
    std::vector<CRNode*> crAddNodes;
    crAddNodes.push_back(result);
    crAddNodes.push_back(mulResult);
    result = scalarAnalysis.GetCRAddNode(nullptr, crAddNodes);
  }
  return result;
}

MeExpr *LoopScalarAnalysisResult::TryToResolveVar(MeExpr &expr, std::set<MePhiNode*> &visitedPhi,
                                                  MeExpr &dummyExpr) {
  CHECK_FATAL(expr.GetMeOp() == kMeOpVar, "must be");
  auto *var = static_cast<VarMeExpr*>(&expr);
  if (var->GetDefBy() == kDefByStmt && !var->GetDefStmt()->GetRHS()->IsLeaf()) {
    return nullptr;
  }
  if (var->GetDefBy() == kDefByStmt && var->GetDefStmt()->GetRHS()->GetMeOp() == kMeOpConst) {
    return var->GetDefStmt()->GetRHS();
  }
  if (var->GetDefBy() == kDefByStmt) {
    CHECK_FATAL(var->GetDefStmt()->GetRHS()->GetMeOp() == kMeOpVar, "must be");
    return TryToResolveVar(*(var->GetDefStmt()->GetRHS()), visitedPhi, dummyExpr);
  }

  if (var->GetDefBy() == kDefByPhi) {
    MePhiNode *phi = &(var->GetDefPhi());
    if (visitedPhi.find(phi) != visitedPhi.end()) {
      return &dummyExpr;
    }
    visitedPhi.insert(phi);
    std::set<MeExpr*> constRes;
    for (auto *phiOpnd : phi->GetOpnds()) {
      MeExpr *tmp = TryToResolveVar(*phiOpnd, visitedPhi, dummyExpr);
      if (tmp == nullptr) {
        return nullptr;
      }
      if (tmp != &dummyExpr) {
        constRes.insert(tmp);
      }
    }
    if (constRes.size() == 1) {
      return *(constRes.begin());
    } else {
      return nullptr;
    }
  }
  return nullptr;
}

void LoopScalarAnalysisResult::VerifyCR(const CRNode &crNode) {
  constexpr uint32 verifyTimes = 10; // Compute top ten iterations result of crNode.
  for (uint32 i = 0; i < verifyTimes; ++i) {
    if (crNode.GetCRType() != kCRNode) {
      continue;
    }
    const CRNode *r = static_cast<const CR*>(&crNode)->ComputeValueAtIteration(i, *this);
    if (r->GetCRType() == kCRConstNode) {
      std::cout << static_cast<const CRConstNode*>(r)->GetConstValue() << std::endl;
    }
  }
}


void LoopScalarAnalysisResult::Dump(const CRNode &crNode) {
  switch (crNode.GetCRType()) {
    case kCRConstNode: {
      LogInfo::MapleLogger() << static_cast<const CRConstNode*>(&crNode)->GetConstValue();
      return;
    }
    case kCRVarNode: {
      CHECK_FATAL(crNode.GetExpr() != nullptr, "crNode must has expr");
      const MeExpr *meExpr = crNode.GetExpr();
      const VarMeExpr *varMeExpr = nullptr;
      std::string name;
      if (meExpr->GetMeOp() == kMeOpVar) {
        varMeExpr = static_cast<const VarMeExpr*>(meExpr);
      } else if (meExpr->GetMeOp() == kMeOpIvar) {
        const auto *ivarMeExpr = static_cast<const IvarMeExpr*>(meExpr);
        const MeExpr *base = ivarMeExpr->GetBase();
        if (base->GetMeOp() == kMeOpVar) {
          varMeExpr = static_cast<const VarMeExpr*>(base);
        } else {
          name = "ivar" + std::to_string(ivarMeExpr->GetExprID());
          LogInfo::MapleLogger() << name;
          return;
        }
      }
      MIRSymbol *sym = irMap->GetSSATab().GetMIRSymbolFromID(varMeExpr->GetOstIdx());
      name = sym->GetName() + "_mx" + std::to_string(meExpr->GetExprID());
      LogInfo::MapleLogger() << name;
      return;
    }
    case kCRAddNode: {
      const CRAddNode *crAddNode = static_cast<const CRAddNode*>(&crNode);
      CHECK_FATAL(crAddNode->GetOpndsSize() > 1, "crAddNode must has more than one opnd");
      Dump(*crAddNode->GetOpnd(0));
      for (size_t i = 1; i < crAddNode->GetOpndsSize(); ++i) {
        LogInfo::MapleLogger() << " + ";
        Dump(*crAddNode->GetOpnd(i));
      }
      return;
    }
    case kCRMulNode: {
      const CRMulNode *crMulNode = static_cast<const CRMulNode*>(&crNode);
      CHECK_FATAL(crMulNode->GetOpndsSize() > 1, "crMulNode must has more than one opnd");
      if (crMulNode->GetOpnd(0)->GetCRType() != kCRConstNode && crMulNode->GetOpnd(0)->GetCRType() != kCRVarNode) {
        LogInfo::MapleLogger() << "(";
        Dump(*crMulNode->GetOpnd(0));
        LogInfo::MapleLogger() << ")";
      } else {
        Dump(*crMulNode->GetOpnd(0));
      }

      for (size_t i = 1; i < crMulNode->GetOpndsSize(); ++i) {
        LogInfo::MapleLogger() << " * ";
        if (crMulNode->GetOpnd(i)->GetCRType() != kCRConstNode && crMulNode->GetOpnd(i)->GetCRType() != kCRVarNode) {
          LogInfo::MapleLogger() << "(";
          Dump(*crMulNode->GetOpnd(i));
          LogInfo::MapleLogger() << ")";
        } else {
          Dump(*crMulNode->GetOpnd(i));
        }
      }
      return;
    }
    case kCRDivNode: {
      const CRDivNode *crDivNode = static_cast<const CRDivNode*>(&crNode);
      if (crDivNode->GetLHS()->GetCRType() != kCRConstNode && crDivNode->GetLHS()->GetCRType() != kCRVarNode) {
        LogInfo::MapleLogger() << "(";
        Dump(*crDivNode->GetLHS());
        LogInfo::MapleLogger() << ")";
      } else {
        Dump(*crDivNode->GetLHS());
      }
      LogInfo::MapleLogger() << " / ";
      if (crDivNode->GetRHS()->GetCRType() != kCRConstNode && crDivNode->GetRHS()->GetCRType() != kCRVarNode) {
        LogInfo::MapleLogger() << "(";
        Dump(*crDivNode->GetRHS());
        LogInfo::MapleLogger() << ")";
      } else {
        Dump(*crDivNode->GetRHS());
      }
      return;
    }
    case kCRNode: {
      const CR *cr = static_cast<const CR*>(&crNode);
      CHECK_FATAL(cr->GetOpndsSize() > 1, "cr must has more than one opnd");
      LogInfo::MapleLogger() << "{";
      Dump(*cr->GetOpnd(0));
      for (size_t i = 1; i < cr->GetOpndsSize(); ++i) {
        LogInfo::MapleLogger() << ", + ,";
        Dump(*cr->GetOpnd(i));
      }
      LogInfo::MapleLogger() << "}";
      return;
    }
    default:
      LogInfo::MapleLogger() << crNode.GetExpr()->GetExprID() << "@@@" << crNode.GetCRType();
      CHECK_FATAL(false, "can not support");
  }
}

CRNode *LoopScalarAnalysisResult::GetOrCreateCRConstNode(MeExpr *expr, int32 value) {
  if (expr == nullptr) {
    std::unique_ptr<CRConstNode> constNode = std::make_unique<CRConstNode>(nullptr, value);
    CRConstNode *constPtr = constNode.get();
    allCRNodes.insert(std::move(constNode));
    return constPtr;
  }
  auto it = expr2CR.find(expr);
  if (it != expr2CR.end()) {
    return it->second;
  }

  std::unique_ptr<CRConstNode> constNode = std::make_unique<CRConstNode>(expr, value);
  CRConstNode *constPtr = constNode.get();
  allCRNodes.insert(std::move(constNode));
  InsertExpr2CR(*expr, constPtr);
  return constPtr;
}

CRNode *LoopScalarAnalysisResult::GetOrCreateCRVarNode(MeExpr &expr) {
  auto it = expr2CR.find(&expr);
  if (it != expr2CR.end()) {
    return it->second;
  }

  std::unique_ptr<CRVarNode> varNode = std::make_unique<CRVarNode>(&expr);
  CRVarNode *varPtr = varNode.get();
  allCRNodes.insert(std::move(varNode));
  InsertExpr2CR(expr, varPtr);
  return varPtr;
}

CRNode *LoopScalarAnalysisResult::GetOrCreateLoopInvariantCR(MeExpr &expr) {
  if (expr.GetMeOp() == kMeOpConst && static_cast<ConstMeExpr&>(expr).GetConstVal()->GetKind() == kConstInt) {
    return GetOrCreateCRConstNode(&expr, static_cast<int32>(static_cast<ConstMeExpr&>(expr).GetIntValue()));
  }
  if (expr.GetMeOp() == kMeOpVar) {
    // Try to resolve Var is assigned from Const
    MeExpr *varExpr = &expr;
    std::set<MePhiNode*> visitedPhi;
    ConstMeExpr dummyExpr(kInvalidExprID, nullptr, PTY_unknown);
    varExpr = TryToResolveVar(*varExpr, visitedPhi, dummyExpr);
    if (varExpr != nullptr && varExpr != &dummyExpr) {
      CHECK_FATAL(varExpr->GetMeOp() == kMeOpConst, "must be");
      return GetOrCreateCRConstNode(&expr, static_cast<int32>(static_cast<ConstMeExpr*>(varExpr)->GetIntValue()));
    }
    return GetOrCreateCRVarNode(expr);
  }
  if (expr.GetMeOp() == kMeOpIvar) {
    return GetOrCreateCRVarNode(expr);
  }
  CHECK_FATAL(false, "only support kMeOpConst, kMeOpVar, kMeOpIvar");
  return nullptr;
}

CRNode* LoopScalarAnalysisResult::GetOrCreateCRAddNode(MeExpr *expr, std::vector<CRNode*> &crAddNodes) {
  if (expr == nullptr) {
    std::unique_ptr<CRAddNode> crAdd = std::make_unique<CRAddNode>(nullptr);
    CRAddNode *crPtr = crAdd.get();
    allCRNodes.insert(std::move(crAdd));
    crPtr->SetOpnds(crAddNodes);
    return crPtr;
  }
  auto it = expr2CR.find(expr);
  if (it != expr2CR.end()) {
    return it->second;
  }
  std::unique_ptr<CRAddNode> crAdd = std::make_unique<CRAddNode>(expr);
  CRAddNode *crAddPtr = crAdd.get();
  allCRNodes.insert(std::move(crAdd));
  crAddPtr->SetOpnds(crAddNodes);
  // can not insert crAddNode to expr2CR map
  return crAddPtr;
}

CRNode *LoopScalarAnalysisResult::GetOrCreateCR(MeExpr &expr, CRNode &start, CRNode &stride) {
  auto it = expr2CR.find(&expr);
  if (it != expr2CR.end() && (it->second == nullptr || it->second->GetCRType() != kCRUnKnown)) {
    return it->second;
  }
  std::unique_ptr<CR> cr = std::make_unique<CR>(&expr);
  CR *crPtr = cr.get();
  allCRNodes.insert(std::move(cr));
  InsertExpr2CR(expr, crPtr);
  crPtr->PushOpnds(start);
  if (stride.GetCRType() == kCRNode) {
    crPtr->InsertOpnds(static_cast<CR*>(&stride)->GetOpnds().begin(), static_cast<CR*>(&stride)->GetOpnds().end());
    return crPtr;
  }
  crPtr->PushOpnds(stride);
  return crPtr;
}

CRNode *LoopScalarAnalysisResult::GetOrCreateCR(MeExpr *expr, std::vector<CRNode*> &crNodes) {
  auto it = expr2CR.find(expr);
  if (it != expr2CR.end()) {
    return it->second;
  }
  std::unique_ptr<CR> cr = std::make_unique<CR>(expr);
  CR *crPtr = cr.get();
  allCRNodes.insert(std::move(cr));
  crPtr->SetOpnds(crNodes);
  if (expr != nullptr) {
    InsertExpr2CR(*expr, crPtr);
  }
  return crPtr;
}

CRNode *LoopScalarAnalysisResult::GetCRAddNode(MeExpr *expr, std::vector<CRNode*> &crAddOpnds) {
  if (crAddOpnds.size() == 1) {
    return crAddOpnds[0];
  }
  std::sort(crAddOpnds.begin(), crAddOpnds.end(), CompareCRNode());
  // merge constCRNode
  // 1 + 2 + 3 + X -> 6 + X
  size_t index = 0;
  if (crAddOpnds[0]->GetCRType() == kCRConstNode) {
    CRConstNode *constLHS = static_cast<CRConstNode*>(crAddOpnds[0]);
    ++index;
    while (index < crAddOpnds.size() && crAddOpnds[index]->GetCRType() == kCRConstNode) {
      CRConstNode *constRHS = static_cast<CRConstNode*>(crAddOpnds[index]);
      crAddOpnds[0] = GetOrCreateCRConstNode(nullptr, constLHS->GetConstValue() + constRHS->GetConstValue());
      if (crAddOpnds.size() == kNumOpnds) {
        return crAddOpnds[0];
      }
      crAddOpnds.erase(crAddOpnds.begin() + 1);
      constLHS = static_cast<CRConstNode*>(crAddOpnds[0]);
    }
    // After merge constCRNode, if crAddOpnds[0] is 0, delete it from crAddOpnds vector
    // 0 + X -> X
    if (constLHS->GetConstValue() == 0) {
      crAddOpnds.erase(crAddOpnds.begin());
      --index;
    }
    if (crAddOpnds.size() == 1) {
      return crAddOpnds[0];
    }
  }

  // if the crType of operand is CRAddNode, unfold the operand
  // 1 + (X + Y) -> 1 + X + Y
  size_t addCRIndex = index;
  while (addCRIndex < crAddOpnds.size() && crAddOpnds[addCRIndex]->GetCRType() != kCRAddNode) {
    ++addCRIndex;
  }
  if (addCRIndex < crAddOpnds.size()) {
    while (crAddOpnds[addCRIndex]->GetCRType() == kCRAddNode) {
      crAddOpnds.insert(crAddOpnds.end(),
                        static_cast<CRAddNode*>(crAddOpnds[addCRIndex])->GetOpnds().begin(),
                        static_cast<CRAddNode*>(crAddOpnds[addCRIndex])->GetOpnds().end());
      crAddOpnds.erase(crAddOpnds.begin() + addCRIndex);
    }
    return GetCRAddNode(expr, crAddOpnds);
  }

  // merge cr
  size_t crIndex = index;
  while (crIndex < crAddOpnds.size() && crAddOpnds[crIndex]->GetCRType() != kCRNode) {
    ++crIndex;
  }
  if (crIndex < crAddOpnds.size()) {
    // X + { Y, + , Z } -> { X + Y, + , Z }
    if (crIndex != 0) {
      std::vector<CRNode*> startOpnds(crAddOpnds.begin(), crAddOpnds.begin() + crIndex);
      crAddOpnds.erase(crAddOpnds.begin(), crAddOpnds.begin() + crIndex);
      startOpnds.push_back(static_cast<CR*>(crAddOpnds[crIndex])->GetOpnd(0));
      CRNode *start = GetCRAddNode(nullptr, startOpnds);
      std::vector<CRNode*> newCROpnds{ start };
      newCROpnds.insert(newCROpnds.end(), static_cast<CR*>(crAddOpnds[crIndex])->GetOpnds().begin() + 1,
                        static_cast<CR*>(crAddOpnds[crIndex])->GetOpnds().end());
      CR *newCR = static_cast<CR*>(GetOrCreateCR(expr, newCROpnds));
      if (crAddOpnds.size() == 1) {
        return static_cast<CRNode*>(newCR);
      }
      crAddOpnds[0] = static_cast<CRNode*>(newCR);
      crIndex = 0;
    }
    // { X1 , + , Y1,  + , ... , + , Z1 } + { X2 , + , Y2, + , ... , + , Z2 }
    // ->
    // { X1 + X2, + , Y1 + Y2, + , ... , + , Z1 + Z2 }
    ++crIndex;
    CR *crLHS = static_cast<CR*>(crAddOpnds[0]);
    while (crIndex < crAddOpnds.size() && crAddOpnds[crIndex]->GetCRType() == kCRNode) {
      CR *crRHS = static_cast<CR*>(crAddOpnds[crIndex]);
      crAddOpnds[0] = static_cast<CRNode*>(AddCRWithCR(*crLHS, *crRHS));
      if (crAddOpnds.size() == kNumOpnds) {
        return crAddOpnds[0];
      }
      crAddOpnds.erase(crAddOpnds.begin() + 1);
      crLHS = static_cast<CR*>(crAddOpnds[0]);
    }
    if (crAddOpnds.size() == 1) {
      return crAddOpnds[0];
    }
  }
  return GetOrCreateCRAddNode(expr, crAddOpnds);
}

CRNode *LoopScalarAnalysisResult::GetCRMulNode(MeExpr *expr, std::vector<CRNode*> &crMulOpnds) {
  if (crMulOpnds.size() == 1) {
    return crMulOpnds[0];
  }
  std::sort(crMulOpnds.begin(), crMulOpnds.end(), CompareCRNode());
  // merge constCRNode
  // 1 * 2 * 3 * X -> 6 * X
  size_t index = 0;
  if (crMulOpnds[0]->GetCRType() == kCRConstNode) {
    CRConstNode *constLHS = static_cast<CRConstNode*>(crMulOpnds[0]);
    ++index;
    while (index < crMulOpnds.size() && crMulOpnds[index]->GetCRType() == kCRConstNode) {
      CRConstNode *constRHS = static_cast<CRConstNode*>(crMulOpnds[index]);
      crMulOpnds[0] = GetOrCreateCRConstNode(nullptr, constLHS->GetConstValue() * constRHS->GetConstValue());
      if (crMulOpnds.size() == kNumOpnds) {
        return crMulOpnds[0];
      }
      crMulOpnds.erase(crMulOpnds.begin() + 1);
      constLHS = static_cast<CRConstNode*>(crMulOpnds[0]);
    }
    // After merge constCRNode, if crMulOpnds[0] is 1, delete it from crMulOpnds vector
    // 1 * X -> X
    if (constLHS->GetConstValue() == 1) {
      crMulOpnds.erase(crMulOpnds.begin());
      --index;
    }
    // After merge constCRNode, if crMulOpnds[0] is 0, return crMulOpnds[0]
    // 0 * X -> 0
    if (constLHS->GetConstValue() == 0) {
      return crMulOpnds[0];
    }
    if (crMulOpnds.size() == 1) {
      return crMulOpnds[0];
    }
  }

  // if the crType of operand is CRAddNode, unfold the operand
  // 2 * (X * Y) -> 2 * X * Y
  size_t mulCRIndex = index;
  while (mulCRIndex < crMulOpnds.size() && crMulOpnds[mulCRIndex]->GetCRType() != kCRMulNode) {
    ++mulCRIndex;
  }
  if (mulCRIndex < crMulOpnds.size()) {
    while (crMulOpnds[mulCRIndex]->GetCRType() == kCRMulNode) {
      crMulOpnds.insert(crMulOpnds.end(),
                        static_cast<CRAddNode*>(crMulOpnds[mulCRIndex])->GetOpnds().begin(),
                        static_cast<CRAddNode*>(crMulOpnds[mulCRIndex])->GetOpnds().end());
      crMulOpnds.erase(crMulOpnds.begin() + mulCRIndex);
    }
    return GetCRMulNode(expr, crMulOpnds);
  }

  // merge cr
  size_t crIndex = index;
  while (crIndex < crMulOpnds.size() && crMulOpnds[crIndex]->GetCRType() != kCRNode) {
    ++crIndex;
  }
  if (crIndex < crMulOpnds.size()) {
    // X * { Y, + , Z } -> { X * Y, + , X * Z }
    if (crIndex != 0) {
      std::vector<CRNode*> newCROpnds;
      CR *currCR = static_cast<CR*>(crMulOpnds[crIndex]);
      std::vector<CRNode*> crOpnds(crMulOpnds.begin(), crMulOpnds.begin() + crIndex);
      crMulOpnds.erase(crMulOpnds.begin(), crMulOpnds.begin() + crIndex);
      for (size_t i = 0; i < currCR->GetOpndsSize(); ++i) {
        std::vector<CRNode*> currOpnds(crOpnds);
        currOpnds.push_back(currCR->GetOpnd(i));
        CRNode *start = GetCRMulNode(nullptr, currOpnds);
        newCROpnds.push_back(start);
      }
      CR *newCR = static_cast<CR*>(GetOrCreateCR(expr, newCROpnds));
      if (crMulOpnds.size() == 1) {
        return static_cast<CRNode*>(newCR);
      }
      crMulOpnds[0] = static_cast<CRNode*>(newCR);
      crIndex = 0;
    }
    // { X1 , + , Y1,  + , ... , + , Z1 } * { X2 , + , Y2, + , ... , + , Z2 }
    // ->
    // { X1 + X2, + , Y1 + Y2, + , ... , + , Z1 + Z2 }
    ++crIndex;
    CR *crLHS = static_cast<CR*>(crMulOpnds[0]);
    while (crIndex < crMulOpnds.size() && crMulOpnds[crIndex]->GetCRType() == kCRNode) {
      CR *crRHS = static_cast<CR*>(crMulOpnds[crIndex]);
      crMulOpnds[0] = static_cast<CRNode*>(MulCRWithCR(*crLHS, *crRHS));
      if (crMulOpnds.size() == kNumOpnds) {
        return crMulOpnds[0];
      }
      crMulOpnds.erase(crMulOpnds.begin() + 1);
      crLHS = static_cast<CR*>(crMulOpnds[0]);
    }
    if (crMulOpnds.size() == 1) {
      return crMulOpnds[0];
    }
  }
  return GetOrCreateCRMulNode(expr, crMulOpnds);
}

CRNode *LoopScalarAnalysisResult::GetOrCreateCRMulNode(MeExpr *expr, std::vector<CRNode*> &crMulNodes) {
  std::unique_ptr<CRMulNode> crMul = std::make_unique<CRMulNode>(expr);
  CRMulNode *crMulPtr = crMul.get();
  allCRNodes.insert(std::move(crMul));
  crMulPtr->SetOpnds(crMulNodes);
  return crMulPtr;
}

CRNode *LoopScalarAnalysisResult::GetOrCreateCRDivNode(MeExpr *expr, CRNode &lhsCRNode, CRNode &rhsCRNode) {
  if (lhsCRNode.GetCRType() == kCRConstNode && rhsCRNode.GetCRType() == kCRConstNode) {
    CRConstNode *lhsConst = static_cast<CRConstNode*>(&lhsCRNode);
    CRConstNode *rhsConst = static_cast<CRConstNode*>(&rhsCRNode);
    CHECK_FATAL(rhsConst->GetConstValue() != 0, "rhs is zero");
    if (lhsConst->GetConstValue() % rhsConst->GetConstValue() == 0) {
      std::unique_ptr<CRConstNode> constNode =
          std::make_unique<CRConstNode>(expr, lhsConst->GetConstValue() / rhsConst->GetConstValue());
      CRConstNode *constPtr = constNode.get();
      allCRNodes.insert(std::move(constNode));
      return constPtr;
    }
  }
  std::unique_ptr<CRDivNode> divNode = std::make_unique<CRDivNode>(expr, lhsCRNode, rhsCRNode);
  CRDivNode *divPtr = divNode.get();
  allCRNodes.insert(std::move(divNode));
  return divPtr;
}

// -expr => -1 * expr
CRNode *LoopScalarAnalysisResult::ChangeNegative2MulCRNode(CRNode &crNode) {
  std::unique_ptr<CRConstNode> constNode = std::make_unique<CRConstNode>(nullptr, -1);
  CRConstNode *constPtr = constNode.get();
  allCRNodes.insert(std::move(constNode));
  std::vector<CRNode*> crMulNodes;
  crMulNodes.push_back(constPtr);
  crMulNodes.push_back(&crNode);
  return GetCRMulNode(nullptr, crMulNodes);
}


CR *LoopScalarAnalysisResult::AddCRWithCR(CR &lhsCR, CR &rhsCR) {
  std::unique_ptr<CR> cr = std::make_unique<CR>(nullptr);
  CR *crPtr = cr.get();
  allCRNodes.insert(std::move(cr));
  size_t len = lhsCR.GetOpndsSize() < rhsCR.GetOpndsSize() ? lhsCR.GetOpndsSize() : rhsCR.GetOpndsSize();
  std::vector<CRNode*> crOpnds;
  size_t i = 0;
  for (; i < len; ++i) {
    std::vector<CRNode*> crAddOpnds{lhsCR.GetOpnd(i), rhsCR.GetOpnd(i)};
    crOpnds.push_back(GetCRAddNode(nullptr, crAddOpnds));
  }
  if (i < lhsCR.GetOpndsSize()) {
    crOpnds.insert(crOpnds.end(), lhsCR.GetOpnds().begin() + i, lhsCR.GetOpnds().end());
  } else if (i < rhsCR.GetOpndsSize()) {
    crOpnds.insert(crOpnds.end(), rhsCR.GetOpnds().begin() + i, rhsCR.GetOpnds().end());
  }
  crPtr->SetOpnds(crOpnds);
  return crPtr;
}

// support later
CR *LoopScalarAnalysisResult::MulCRWithCR(const CR &lhsCR, const CR &rhsCR) const {
  CHECK_FATAL(lhsCR.GetCRType() == kCRNode, "must be kCRNode");
  CHECK_FATAL(rhsCR.GetCRType() == kCRNode, "must be kCRNode");
  CHECK_FATAL(false, "NYI");
}

CRNode *LoopScalarAnalysisResult::ComputeCRNodeWithOperator(MeExpr &expr, CRNode &lhsCRNode,
                                                            CRNode &rhsCRNode, Opcode op) {
  switch (op) {
    case OP_add: {
      std::vector<CRNode*> crAddNodes;
      crAddNodes.push_back(&lhsCRNode);
      crAddNodes.push_back(&rhsCRNode);
      return GetCRAddNode(&expr, crAddNodes);
    }
    case OP_sub: {
      std::vector<CRNode*> crAddNodes;
      crAddNodes.push_back(&lhsCRNode);
      crAddNodes.push_back(ChangeNegative2MulCRNode(rhsCRNode));
      return GetCRAddNode(&expr, crAddNodes);
    }
    case OP_mul: {
      std::vector<CRNode*> crMulNodes;
      crMulNodes.push_back(&lhsCRNode);
      crMulNodes.push_back(&rhsCRNode);
      return GetCRMulNode(&expr, crMulNodes);
    }
    case OP_div: {
      return GetOrCreateCRDivNode(&expr, lhsCRNode, rhsCRNode);
    }
    default:
      return nullptr;
  }
}

bool LoopScalarAnalysisResult::HasUnknownCRNode(CRNode &crNode, CRNode *&result) {
  switch (crNode.GetCRType()) {
    case kCRConstNode: {
      return false;
    }
    case kCRVarNode: {
      return false;
    }
    case kCRAddNode: {
      CRAddNode *crAddNode = static_cast<CRAddNode*>(&crNode);
      CHECK_FATAL(crAddNode->GetOpndsSize() > 1, "crAddNode must has more than one opnd");
      for (size_t i = 0; i < crAddNode->GetOpndsSize(); ++i) {
        if (HasUnknownCRNode(*crAddNode->GetOpnd(i), result)) {
          return true;
        }
      }
      return false;
    }
    case kCRMulNode: {
      CRMulNode *crMulNode = static_cast<CRMulNode*>(&crNode);
      CHECK_FATAL(crMulNode->GetOpndsSize() > 1, "crMulNode must has more than one opnd");
      for (size_t i = 0; i < crMulNode->GetOpndsSize(); ++i) {
        if (HasUnknownCRNode(*crMulNode->GetOpnd(i), result)) {
          return true;
        }
      }
      return false;
    }
    case kCRDivNode: {
      CRDivNode *crDivNode = static_cast<CRDivNode*>(&crNode);
      return HasUnknownCRNode(*crDivNode->GetLHS(), result) || HasUnknownCRNode(*crDivNode->GetLHS(), result);
    }
    case kCRNode: {
      CR *cr = static_cast<CR*>(&crNode);
      CHECK_FATAL(cr->GetOpndsSize() > 1, "cr must has more than one opnd");
      for (size_t i = 0; i < cr->GetOpndsSize(); ++i) {
        if (HasUnknownCRNode(*cr->GetOpnd(i), result)) {
          return true;
        }
      }
      return false;
    }
    case kCRUnKnown:
      result = &crNode;
      return true;
    default:
      CHECK_FATAL(false, "impossible !");
  }
  return true;
}

// a = phi(b, c)
// c = a + d
// b and d is loopInvarirant
CRNode *LoopScalarAnalysisResult::CreateSimpleCRForPhi(MePhiNode &phiNode,
                                                       VarMeExpr &startExpr, const VarMeExpr &backEdgeExpr) {
  if (backEdgeExpr.GetDefBy() != kDefByStmt) {
    return nullptr;
  }
  MeExpr *rhs = backEdgeExpr.GetDefStmt()->GetRHS();
  if (rhs == nullptr) {
    return nullptr;
  }
  if (rhs->GetMeOp() == kMeOpConst && startExpr.GetDefBy() == kDefByStmt) {
    MeExpr *rhs2 = startExpr.GetDefStmt()->GetRHS();
    if (rhs2->GetMeOp() == kMeOpConst &&
        (static_cast<ConstMeExpr*>(rhs)->GetIntValue() == static_cast<ConstMeExpr*>(rhs2)->GetIntValue())) {
      return GetOrCreateLoopInvariantCR(*rhs);
    }
  }
  if (rhs->GetMeOp() != kMeOpOp || static_cast<OpMeExpr*>(rhs)->GetOp() != OP_add) {
    return nullptr;
  }
  OpMeExpr *opMeExpr = static_cast<OpMeExpr*>(rhs);
  MeExpr *opnd1 = opMeExpr->GetOpnd(0);
  MeExpr *opnd2 = opMeExpr->GetOpnd(1);
  CRNode *stride = nullptr;
  MeExpr *strideExpr = nullptr;
  if (opnd1->GetMeOp() == kMeOpVar && static_cast<VarMeExpr*>(opnd1)->GetDefBy() == kDefByPhi &&
      &(static_cast<VarMeExpr*>(opnd1)->GetDefPhi()) == &phiNode) {
    strideExpr = opnd2;
  } else if (opnd2->GetMeOp() == kMeOpVar && static_cast<VarMeExpr*>(opnd2)->GetDefBy() == kDefByPhi &&
             &(static_cast<VarMeExpr*>(opnd2)->GetDefPhi()) == &phiNode) {
    strideExpr = opnd1;
  }
  if (strideExpr == nullptr) {
    return nullptr;
  }
  switch (strideExpr->GetMeOp()) {
    case kMeOpConst: {
      stride = GetOrCreateLoopInvariantCR(*strideExpr);
      break;
    }
    case kMeOpVar: {
      if (static_cast<VarMeExpr*>(strideExpr)->DefByBB() != nullptr &&
          !loop->Has(*static_cast<VarMeExpr*>(strideExpr)->DefByBB())) {
        stride = GetOrCreateLoopInvariantCR(*strideExpr);
      }
      break;
    }
    case kMeOpIvar: {
      if (static_cast<IvarMeExpr*>(strideExpr)->GetDefStmt() != nullptr &&
          static_cast<IvarMeExpr*>(strideExpr)->GetDefStmt()->GetBB() != nullptr &&
          !loop->Has(*static_cast<IvarMeExpr*>(strideExpr)->GetDefStmt()->GetBB())) {
        stride = GetOrCreateLoopInvariantCR(*strideExpr);
      }
      break;
    }
    default: {
      return nullptr;
    }
  }
  if (stride == nullptr) {
    return nullptr;
  }
  CRNode *start = GetOrCreateLoopInvariantCR(startExpr);
  return GetOrCreateCR(*phiNode.GetLHS(), *start, *stride);
}

CRNode *LoopScalarAnalysisResult::CreateCRForPhi(MePhiNode &phiNode) {
  auto *opnd1 = static_cast<VarMeExpr*>(phiNode.GetOpnd(0));
  auto *opnd2 = static_cast<VarMeExpr*>(phiNode.GetOpnd(1));
  VarMeExpr *startExpr = nullptr;
  VarMeExpr *backEdgeExpr = nullptr;
  if (opnd1->DefByBB() == nullptr || opnd2->DefByBB() == nullptr) {
    return nullptr;
  }
  if (!loop->Has(*opnd1->DefByBB()) && loop->Has(*opnd2->DefByBB())) {
    startExpr = opnd1;
    backEdgeExpr = opnd2;
  } else if (loop->Has(*opnd1->DefByBB()) && !loop->Has(*opnd2->DefByBB())) {
    startExpr = opnd2;
    backEdgeExpr = opnd1;
  } else {
    return nullptr;
  }
  if (startExpr == nullptr || backEdgeExpr == nullptr) {
    return nullptr;
  }
  if (auto *cr = CreateSimpleCRForPhi(phiNode, *startExpr, *backEdgeExpr)) {
    return cr;
  }
  std::unique_ptr<CRUnKnownNode> phiUnKnown = std::make_unique<CRUnKnownNode>(static_cast<MeExpr*>(phiNode.GetLHS()));
  CRUnKnownNode *phiUnKnownPtr = phiUnKnown.get();
  allCRNodes.insert(std::move(phiUnKnown));
  InsertExpr2CR(*(static_cast<MeExpr*>(phiNode.GetLHS())), static_cast<CRNode*>(phiUnKnownPtr));
  CRNode *backEdgeCRNode = GetOrCreateCRNode(*backEdgeExpr);
  if (backEdgeCRNode == nullptr) {
    InsertExpr2CR(*(static_cast<MeExpr*>(phiNode.GetLHS())), nullptr);
    return nullptr;
  }
  if (backEdgeCRNode->GetCRType() == kCRAddNode) {
    size_t index = static_cast<CRAddNode*>(backEdgeCRNode)->GetOpndsSize() + 1;
    for (size_t i = 0; i < static_cast<CRAddNode*>(backEdgeCRNode)->GetOpndsSize(); ++i) {
      if (static_cast<CRAddNode*>(backEdgeCRNode)->GetOpnd(i) == phiUnKnownPtr) {
        index = i;
        break;
      }
    }
    if (index == (static_cast<CRAddNode*>(backEdgeCRNode)->GetOpndsSize() + 1)) {
      InsertExpr2CR(*(static_cast<MeExpr*>(phiNode.GetLHS())), nullptr);
      return nullptr;
    }
    std::vector<CRNode*> crNodes;
    for (size_t i = 0; i < static_cast<CRAddNode*>(backEdgeCRNode)->GetOpndsSize(); ++i) {
      if (i != index) {
        crNodes.push_back(static_cast<CRAddNode*>(backEdgeCRNode)->GetOpnd(i));
      }
    }
    CRNode *start = GetOrCreateLoopInvariantCR(*(static_cast<MeExpr*>(startExpr)));
    CRNode *stride = GetCRAddNode(nullptr, crNodes);
    if (stride == nullptr) {
      InsertExpr2CR(*(static_cast<MeExpr*>(phiNode.GetLHS())), nullptr);
      return nullptr;
    }
    CRNode *crNode = nullptr;
    if (HasUnknownCRNode(*stride, crNode)) {
      CHECK_NULL_FATAL(crNode);
      CHECK_FATAL(crNode->GetCRType() == kCRUnKnown, "must be kCRUnKnown!");
      InsertExpr2CR(*(static_cast<MeExpr*>(phiNode.GetLHS())), nullptr);
      return nullptr;
    }
    CR *cr = static_cast<CR*>(GetOrCreateCR(*phiNode.GetLHS(), *start, *stride));
    return cr;
  } else {
    InsertExpr2CR(*(static_cast<MeExpr*>(phiNode.GetLHS())), nullptr);
    return nullptr;
  }
}

CRNode *LoopScalarAnalysisResult::GetOrCreateCRNode(MeExpr &expr) {
  if (IsAnalysised(expr)) {
    return expr2CR[&expr];
  }
  // Only support expr is kMeOpConst, kMeOpVar, kMeOpIvar
  switch (expr.GetMeOp()) {
    case kMeOpConst: {
      return GetOrCreateLoopInvariantCR(expr);
    }
    case kMeOpVar: {
      VarMeExpr *varExpr = static_cast<VarMeExpr*>(&expr);
      if (varExpr->DefByBB() != nullptr && !loop->Has(*varExpr->DefByBB())) {
        return GetOrCreateLoopInvariantCR(expr);
      }
      switch (varExpr->GetDefBy()) {
        case kDefByStmt: {
          MeExpr *rhs = varExpr->GetDefStmt()->GetRHS();
          switch (rhs->GetMeOp()) {
            case kMeOpConst: {
              return GetOrCreateLoopInvariantCR(expr);
            }
            case kMeOpVar: {
              return GetOrCreateCRNode(*rhs);
            }
            case kMeOpOp: {
              OpMeExpr *opMeExpr = static_cast<OpMeExpr*>(rhs);
              switch (opMeExpr->GetOp()) {
                case OP_add:
                case OP_sub:
                case OP_mul:
                case OP_div: {
                  CHECK_FATAL(opMeExpr->GetNumOpnds() == kNumOpnds, "must be");
                  MeExpr *opnd1 = opMeExpr->GetOpnd(0);
                  MeExpr *opnd2 = opMeExpr->GetOpnd(1);
                  CRNode *lhsCR = GetOrCreateCRNode(*opnd1);
                  CRNode *rhsCR = GetOrCreateCRNode(*opnd2);
                  if (lhsCR == nullptr || rhsCR == nullptr) {
                    return nullptr;
                  }
                  return ComputeCRNodeWithOperator(expr, *lhsCR, *rhsCR, opMeExpr->GetOp());
                }
                default:
                  InsertExpr2CR(expr, nullptr);
                  return nullptr;
              }
            }
            default:
              InsertExpr2CR(expr, nullptr);
              return nullptr;
          }
        }
        case kDefByPhi: {
          MePhiNode *phiNode = &(varExpr->GetDefPhi());
          if (phiNode->GetOpnds().size() == kNumOpnds) {
            return CreateCRForPhi(*phiNode);
          } else {
            InsertExpr2CR(expr, nullptr);
            return nullptr;
          }
        }
        case kDefByChi: {
          ChiMeNode *chiNode = &(varExpr->GetDefChi());
          if (!loop->Has(*chiNode->GetBase()->GetBB())) {
            return GetOrCreateLoopInvariantCR(expr);
          } else {
            InsertExpr2CR(expr, nullptr);
            return nullptr;
          }
        }
        case kDefByNo: {
          return GetOrCreateLoopInvariantCR(expr);
        }
        default:
          InsertExpr2CR(expr, nullptr);
          return nullptr;
      }
    }
    case kMeOpIvar: {
      if (static_cast<IvarMeExpr*>(&expr)->GetDefStmt() != nullptr &&
          static_cast<IvarMeExpr*>(&expr)->GetDefStmt()->GetBB() != nullptr &&
          !loop->Has(*static_cast<IvarMeExpr*>(&expr)->GetDefStmt()->GetBB())) {
        return GetOrCreateLoopInvariantCR(expr);
      } else {
        InsertExpr2CR(expr, nullptr);
        return nullptr;
      }
    }
    default:
      InsertExpr2CR(expr, nullptr);
      return nullptr;
  }
}

bool IsLegal(MeStmt &meStmt) {
  CHECK_FATAL(meStmt.IsCondBr(), "must be");
  auto *brMeStmt = static_cast<CondGotoMeStmt*>(&meStmt);
  MeExpr *meCmp = brMeStmt->GetOpnd();
  if (meCmp->GetMeOp() != kMeOpOp) {
    return false;
  }
  auto *opMeExpr = static_cast<OpMeExpr*>(meCmp);
  CHECK_FATAL(opMeExpr->GetNumOpnds() == kNumOpnds, "must be");
  if (opMeExpr->GetOp() != OP_ge && opMeExpr->GetOp() != OP_le &&
      opMeExpr->GetOp() != OP_lt && opMeExpr->GetOp() != OP_gt &&
      opMeExpr->GetOp() != OP_eq) {
    return false;
  }
  MeExpr *opnd1 = opMeExpr->GetOpnd(0);
  MeExpr *opnd2 = opMeExpr->GetOpnd(1);
  if (!IsPrimitivePureScalar(opnd1->GetPrimType()) || !IsPrimitivePureScalar(opnd2->GetPrimType())) {
    return false;
  }
  return true;
}


// need consider min and max integer
uint32 LoopScalarAnalysisResult::ComputeTripCountWithCR(const CR &cr, const OpMeExpr &opMeExpr, int32 value) {
  uint32 tripCount = 0;
  for (uint32 i = 0; ; ++i) {
    CRNode *result = cr.ComputeValueAtIteration(i, *this);
    switch (opMeExpr.GetOp()) {
      case OP_ge: { // <
        if (static_cast<CRConstNode*>(result)->GetConstValue() < value) {
          ++tripCount;
        } else {
          return tripCount;
        }
        break;
      }
      case OP_le: {
        if (static_cast<CRConstNode*>(result)->GetConstValue() > value) {
          ++tripCount;
        } else {
          return tripCount;
        }
        break;
      }
      case OP_gt: { // <=
        if (static_cast<CRConstNode*>(result)->GetConstValue() <= value) {
          ++tripCount;
        } else {
          return tripCount;
        }
        break;
      }
      case OP_lt: {
        if (static_cast<CRConstNode*>(result)->GetConstValue() >= value) {
          ++tripCount;
        } else {
          return tripCount;
        }
        break;
      }
      case OP_eq: {
        if (static_cast<CRConstNode*>(result)->GetConstValue() != value) {
          ++tripCount;
        } else {
          return tripCount;
        }
        break;
      }
      default:
        CHECK_FATAL(false, "operator must be >=, <=, >, <, !=");
    }
  }
}

uint32 LoopScalarAnalysisResult::ComputeTripCountWithSimpleConstCR(const OpMeExpr &opMeExpr, int32 value,
                                                                   int32 start, int32 stride) const {
  constexpr int32 javaIntMinValue = -2147483648; // -8fffffff
  constexpr int32 javaIntMaxValue = 2147483647;  // 7fffffff
  CHECK_FATAL(stride != 0, "stride must not be zero");
  if (value == start) {
    return 0;
  }
  if (stride < 0) {
    stride = -stride;
  }
  int32 times = (value < start) ? ((start - value) / stride) : ((value - start) / stride);
  if (times <= 0) {
    return 0;
  }
  uint32 remainder = (value < start) ? ((start - value) % stride) : ((value - start) % stride);
  switch (opMeExpr.GetOp()) {
    case OP_ge: { // <
      if (start >= value || start - stride >= value) {
        return 0;
      }
      return (remainder == 0) ? times : (times + 1);
    }
    case OP_le: { // >
      if (start <= value || start - stride <= value) {
        return 0;
      }
      return (remainder == 0) ? times : (times + 1);
    }
    case OP_gt: { // <=
      if (start > value || start - stride > value || (start < value && value == javaIntMaxValue)) {
        return 0;
      }
      return times + 1;
    }
    case OP_lt: { // >=
      if (start < value || start - stride < value || (start > value && value == javaIntMinValue)) {
        return 0;
      }
      return times + 1;
    }
    case OP_eq: { // !=
      if (start == value || start - stride == value) {
        return 0;
      }
      return (remainder == 0) ? times : 0;
    }
    default:
      CHECK_FATAL(false, "operator must be >=, <=, >, <, !=");
  }
}

void LoopScalarAnalysisResult::DumpTripCount(const CR &cr, int32 value, const MeExpr *expr) {
  LogInfo::MapleLogger() << "==========Dump CR=========\n";
  Dump(cr);
  if (expr == nullptr) {
    LogInfo::MapleLogger() << "\n" << "value: " << value << "\n";
  } else {
    LogInfo::MapleLogger() << "\n" << "value: mx_" << expr->GetExprID() << "\n";
  }

  VerifyCR(cr);
  LogInfo::MapleLogger() << "==========Dump CR End=========\n";
}

TripCountType LoopScalarAnalysisResult::ComputeTripCount(MeFunction &func, uint32 &tripCountResult,
                                                         CRNode *&conditionCRNode, CR *&itCR) {
  enableDebug = false;
  BB *exitBB = func.GetBBFromID(loop->inloopBB2exitBBs.begin()->first);
  if (exitBB->GetKind() == kBBCondGoto && IsLegal(*(exitBB->GetLastMe()))) {
    auto *brMeStmt = static_cast<CondGotoMeStmt*>(exitBB->GetLastMe());
    BB *brTarget = exitBB->GetSucc(1);
    CHECK_FATAL(brMeStmt->GetOffset() == brTarget->GetBBLabel(), "must be");
    auto *opMeExpr = static_cast<OpMeExpr*>(brMeStmt->GetOpnd());
    MeExpr *opnd1 = opMeExpr->GetOpnd(0);
    MeExpr *opnd2 = opMeExpr->GetOpnd(1);
    if (opnd1->GetPrimType() != PTY_i32 || opnd2->GetPrimType() != PTY_i32) {
      return kCouldNotComputeCR;
    }
    CRNode *crNode1 = GetOrCreateCRNode(*opnd1);
    CRNode *crNode2 = GetOrCreateCRNode(*opnd2);
    CR *cr = nullptr;
    CRConstNode *constNode = nullptr;
    if (crNode1 != nullptr && crNode2 != nullptr) {
      if (crNode1->GetCRType() == kCRNode && crNode2->GetCRType() == kCRConstNode) {
        cr = static_cast<CR*>(crNode1);
        constNode = static_cast<CRConstNode*>(crNode2);
      } else if (crNode2->GetCRType() == kCRNode && crNode1->GetCRType() == kCRConstNode) {
        cr = static_cast<CR*>(crNode2);
        constNode = static_cast<CRConstNode*>(crNode1);
      } else if (crNode1->GetCRType() == kCRNode && crNode2->GetCRType() == kCRNode) {
        return kCouldNotUnroll; // can not compute tripcount
      } else if (crNode1->GetCRType() == kCRNode || crNode2->GetCRType() == kCRNode) {
        if (crNode1->GetCRType() == kCRNode) {
          conditionCRNode = crNode2;
          itCR = static_cast<CR*>(crNode1);
        } else if (crNode2->GetCRType() == kCRNode) {
          conditionCRNode = crNode1;
          itCR = static_cast<CR*>(crNode2);
        } else {
          CHECK_FATAL(false, "impossible");
        }
        if (enableDebug) {
          DumpTripCount(*itCR, 0, conditionCRNode->GetExpr());
        }
        return kVarCondition;
      } else {
        return kCouldNotComputeCR;
      }
    } else {
      return kCouldNotComputeCR;
    }
    if (enableDebug) {
      DumpTripCount(*cr, constNode->GetConstValue(), nullptr);
    }
    for (auto opnd : cr->GetOpnds()) {
      if (opnd->GetCRType() != kCRConstNode) {
        conditionCRNode = constNode;
        itCR = cr;
        return kVarCR;
      }
    }
    CHECK_FATAL(cr->GetOpndsSize() > 1, "impossible");
    if (cr->GetOpndsSize() == 2) { // cr has two opnds like {1, + 1}
      tripCountResult = ComputeTripCountWithSimpleConstCR(*opMeExpr, constNode->GetConstValue(),
                                                          static_cast<CRConstNode*>(cr->GetOpnd(0))->GetConstValue(),
                                                          static_cast<CRConstNode*>(cr->GetOpnd(1))->GetConstValue());
      return kConstCR;
    } else {
      CHECK_FATAL(false, "NYI");
      tripCountResult = ComputeTripCountWithCR(*cr, *opMeExpr, constNode->GetConstValue());
      return kConstCR;
    }
    return kConstCR;
  }
  return kCouldNotUnroll;
}
}  // namespace maple
