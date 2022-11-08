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

bool CRNode::IsEqual(const CRNode &crNode) const {
  if (crType != crNode.GetCRType()) {
    return false;
  }
  switch (crType) {
    case kCRConstNode: {
      return static_cast<const CRConstNode&>(crNode).GetConstValue() ==
             static_cast<const CRConstNode*>(this)->GetConstValue();
    }
    case kCRVarNode: {
      return expr == crNode.expr;
    }
    case kCRNode:
    case kCRAddNode:
    case kCRMulNode: {
      auto &opndsOfCRNode = crNode.GetOpnds();
      auto &opndsOfThis = this->GetOpnds();
      if (opndsOfCRNode.size() != opndsOfThis.size()) {
        return false;
      }
      for (size_t i = 0; i < opndsOfCRNode.size(); ++i) {
        if (!opndsOfCRNode.at(i)->IsEqual(*opndsOfThis.at(i))) {
          return false;
        }
      }
      return true;
    }
    case kCRDivNode: {
      return static_cast<const CRDivNode&>(crNode).GetLHS()->IsEqual(*static_cast<const CRDivNode*>(this)->GetLHS()) &&
             static_cast<const CRDivNode&>(crNode).GetRHS()->IsEqual(*static_cast<const CRDivNode*>(this)->GetRHS());
    }
    default:
      return false;
  }
}

// Put the sub nodes of crNode in vector for opt the boundary check, such as:
// crNode: ptr + length * 4 - 8
// =>
// curVector: { ptr, length * 4, -8 }
void CRNode::PutTheSubNode2Vector(std::vector<CRNode*> &curVector) {
  switch (crType) {
    case kCRConstNode:
    case kCRVarNode:
    case kCRDivNode:
    case kCRMulNode:
    case kCRNode:
    case kCRUnKnown:
      curVector.push_back(this);
      break;
    case kCRAddNode:
      auto *crAddNode = static_cast<CRAddNode*>(this);
      curVector.insert(curVector.cend(), crAddNode->GetOpnds().cbegin(), crAddNode->GetOpnds().cend());
      break;
  }
}

static bool IsConstantMultipliedByVariable(const CRMulNode &crMulNode) {
  return (crMulNode.GetOpndsSize() == kNumOpnds && crMulNode.GetOpnd(0)->GetCRType() == kCRConstNode);
}

// Get the byte size of array for simpify the crNodes.
uint8 LoopScalarAnalysisResult::GetByteSize(std::vector<CRNode*> &crNodeVector) {
  for (size_t i = 0; i < crNodeVector.size(); ++i) {
    if (crNodeVector[i]->GetCRType() != kCRMulNode) {
      continue;
    }
    auto *crMulNode = static_cast<CRMulNode*>(crNodeVector[i]);
    if (!IsConstantMultipliedByVariable(*crMulNode)) {
      return 0;
    }
    return static_cast<uint8>(static_cast<CRConstNode*>(crMulNode->GetOpnd(0))->GetConstValue());
  }
  return 0;
}

// Get the prim type of index or bound, if not found, return PTY_i64.
PrimType LoopScalarAnalysisResult::GetPrimType(std::vector<CRNode*> &crNodeVector) {
  for (size_t i = 0; i < crNodeVector.size(); ++i) {
    if (crNodeVector[i]->GetCRType() == kCRVarNode) {
      return crNodeVector[i]->GetExpr()->GetPrimType();
    }
  }
  return PTY_i64;
}

// If the byteSize is 4, simpify the crNodes like this:
// before: length * 4 - var * 4 - 4
// =>
// after: lenght - var - 1
bool LoopScalarAnalysisResult::NormalizationWithByteCount(std::vector<CRNode*> &crNodeVector, uint8 byteSize) {
  if (byteSize <= 1) {
    return true;
  }
  for (size_t i = 0; i < crNodeVector.size(); ++i) {
    if (crNodeVector[i]->GetCRType() != kCRConstNode && crNodeVector[i]->GetCRType() != kCRMulNode) {
      return false;
    }
    if (crNodeVector[i]->GetCRType() == kCRConstNode) {
      auto value = static_cast<CRConstNode*>(crNodeVector[i])->GetConstValue();
      if (value % byteSize != 0) {
        return false;
      }
      value = value / byteSize;
      crNodeVector[i] = GetOrCreateCRConstNode(nullptr, value);
      continue;
    }
    if (crNodeVector[i]->GetCRType() == kCRMulNode) {
      auto *crMulNode = static_cast<CRMulNode*>(crNodeVector[i]);
      if (!IsConstantMultipliedByVariable(*crMulNode)) {
        return false;
      }
      auto value = static_cast<CRConstNode*>(crMulNode->GetOpnd(0))->GetConstValue();
      if (value % byteSize != 0) {
        return false;
      }
      value = value / byteSize;
      if (value == 1) {
        crNodeVector[i] = crMulNode->GetOpnd(1);
      } else {
        std::vector<CRNode*> crMulNodes { GetOrCreateCRConstNode(nullptr, value), crMulNode->GetOpnd(1) };
        crNodeVector[i] = GetOrCreateCRMulNode(nullptr, crMulNodes);
      }
      continue;
    }
  }
  return true;
}

// Return the unequal sub nodes of index crNode and bound crNode.
void CRNode::GetTheUnequalSubNodesOfIndexAndBound(
    CRNode &boundCRNode, std::vector<CRNode*> &indexVector, std::vector<CRNode*> &boundVector) {
  this->PutTheSubNode2Vector(indexVector);
  boundCRNode.PutTheSubNode2Vector(boundVector);
  for (auto it = indexVector.begin(); it != indexVector.end();) {
    bool findTheSameCR = false;
    for (auto itOfBoundVec = boundVector.begin(); itOfBoundVec != boundVector.end(); ++itOfBoundVec) {
      // The vectors are sorted by specified order like :
      // kCRConstNode < kCRVarNode < kCRAddNode < kCRMulNode < kCRDivNode < kCRNode < kCRUnKnown.
      if ((*itOfBoundVec)->GetCRType() > (*it)->GetCRType()) {
        break;
      }
      if ((*itOfBoundVec)->GetCRType() != (*it)->GetCRType()) {
        continue;
      }
      // If the crNodes are equal, erase them from their vector.
      if ((*it)->IsEqual(**itOfBoundVec)) {
        findTheSameCR = true;
        it = indexVector.erase(it);
        boundVector.erase(itOfBoundVec);
        break;
      }
    }
    if (!findTheSameCR) {
      ++it;
    }
  }
}

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

MeExpr *LoopScalarAnalysisResult::TryToResolveScalar(MeExpr &expr, std::set<MePhiNode*> &visitedPhi,
                                                     MeExpr &dummyExpr) {
  if (expr.GetMeOp() != kMeOpVar && expr.GetMeOp() != kMeOpReg) {
    return nullptr;
  }
  auto *scalar = static_cast<ScalarMeExpr*>(&expr);
  if (scalar->GetDefBy() == kDefByStmt) {
    if (!scalar->GetDefStmt()->GetRHS()->IsLeaf()) {
      return nullptr;
    }
    if (scalar->GetDefStmt()->GetRHS()->GetMeOp() == kMeOpConst) {
      return scalar->GetDefStmt()->GetRHS();
    }
    return TryToResolveScalar(*(scalar->GetDefStmt()->GetRHS()), visitedPhi, dummyExpr);
  }

  if (scalar->GetDefBy() == kDefByPhi) {
    MePhiNode *phi = &(scalar->GetDefPhi());
    if (visitedPhi.find(phi) != visitedPhi.end()) {
      return &dummyExpr;
    }
    visitedPhi.insert(phi);
    std::set<MeExpr*> constRes;
    for (auto *phiOpnd : phi->GetOpnds()) {
      MeExpr *tmp = TryToResolveScalar(*phiOpnd, visitedPhi, dummyExpr);
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
      std::string name;
      OStIdx oStIdx;
      if (meExpr->GetMeOp() == kMeOpReg) {
        LogInfo::MapleLogger() << "mx" + std::to_string(meExpr->GetExprID());
        return;
      } else if (meExpr->GetMeOp() == kMeOpAddrof) {
        oStIdx = static_cast<const AddrofMeExpr*>(meExpr)->GetOstIdx();
      } else if (meExpr->GetMeOp() == kMeOpVar) {
        oStIdx = static_cast<const VarMeExpr*>(meExpr)->GetOstIdx();
      } else if (meExpr->GetMeOp() == kMeOpIvar) {
        const auto *ivarMeExpr = static_cast<const IvarMeExpr*>(meExpr);
        const MeExpr *base = ivarMeExpr->GetBase();
        if (base->GetMeOp() == kMeOpVar) {
          oStIdx = static_cast<const VarMeExpr*>(base)->GetOstIdx();
        } else {
          name = "ivar" + std::to_string(ivarMeExpr->GetExprID());
          LogInfo::MapleLogger() << name;
          return;
        }
      } else {
        LogInfo::MapleLogger() << "_mx" + std::to_string(meExpr->GetExprID());
        return;
      }
      MIRSymbol *sym = irMap->GetSSATab().GetMIRSymbolFromID(oStIdx);
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

CRNode *LoopScalarAnalysisResult::GetOrCreateCRConstNode(MeExpr *expr, int64 value) {
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
    return GetOrCreateCRConstNode(&expr, static_cast<ConstMeExpr&>(expr).GetExtIntValue());
  }
  if (expr.GetMeOp() == kMeOpVar || expr.GetMeOp() == kMeOpReg) {
    // Try to resolve Var is assigned from Const
    MeExpr *scalar = &expr;
    std::set<MePhiNode*> visitedPhi;
    ConstMeExpr dummyExpr(kInvalidExprID, nullptr, PTY_unknown);
    scalar = TryToResolveScalar(*scalar, visitedPhi, dummyExpr);
    if (scalar != nullptr && scalar != &dummyExpr) {
      CHECK_FATAL(scalar->GetMeOp() == kMeOpConst, "must be");
      return GetOrCreateCRConstNode(&expr, static_cast<ConstMeExpr*>(scalar)->GetExtIntValue());
    }
    return GetOrCreateCRVarNode(expr);
  }
  return GetOrCreateCRVarNode(expr);
}

CRNode* LoopScalarAnalysisResult::GetOrCreateCRAddNode(MeExpr *expr, const std::vector<CRNode*> &crAddNodes) {
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

CRNode *LoopScalarAnalysisResult::GetOrCreateCR(MeExpr *expr, const std::vector<CRNode*> &crNodes) {
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
  std::sort(crAddOpnds.begin(), crAddOpnds.end(), CompareCRNodeWithCRType());
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
      crAddOpnds.erase(crAddOpnds.cbegin() + 1);
      constLHS = static_cast<CRConstNode*>(crAddOpnds[0]);
    }
    // After merge constCRNode, if crAddOpnds[0] is 0, delete it from crAddOpnds vector
    // 0 + X -> X
    if (constLHS->GetConstValue() == 0) {
      crAddOpnds.erase(crAddOpnds.cbegin());
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
      crAddOpnds.insert(crAddOpnds.cend(),
                        static_cast<CRAddNode*>(crAddOpnds[addCRIndex])->GetOpnds().cbegin(),
                        static_cast<CRAddNode*>(crAddOpnds[addCRIndex])->GetOpnds().cend());
      crAddOpnds.erase(crAddOpnds.cbegin() + addCRIndex);
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
      crAddOpnds.erase(crAddOpnds.cbegin(), crAddOpnds.cbegin() + crIndex);
      startOpnds.push_back(static_cast<CR*>(crAddOpnds[crIndex])->GetOpnd(0));
      CRNode *start = GetCRAddNode(nullptr, startOpnds);
      std::vector<CRNode*> newCROpnds{ start };
      newCROpnds.insert(newCROpnds.cend(), static_cast<CR*>(crAddOpnds[crIndex])->GetOpnds().cbegin() + 1,
                        static_cast<CR*>(crAddOpnds[crIndex])->GetOpnds().cend());
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
        if (crAddOpnds[0]->GetCRType() == kCRNode && crAddOpnds[0]->GetOpnds().size() == 1) {
          return static_cast<CR*>(crAddOpnds[0])->GetOpnd(0);
        }
        return crAddOpnds[0];
      }
      crAddOpnds.erase(crAddOpnds.cbegin() + 1);
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
  std::sort(crMulOpnds.begin(), crMulOpnds.end(), CompareCRNodeWithCRType());
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
      crMulOpnds.erase(crMulOpnds.cbegin() + 1);
      constLHS = static_cast<CRConstNode*>(crMulOpnds[0]);
    }
    // After merge constCRNode, if crMulOpnds[0] is 1, delete it from crMulOpnds vector
    // 1 * X -> X
    if (constLHS->GetConstValue() == 1) {
      crMulOpnds.erase(crMulOpnds.cbegin());
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
      crMulOpnds.insert(crMulOpnds.cend(),
                        static_cast<CRAddNode*>(crMulOpnds[mulCRIndex])->GetOpnds().cbegin(),
                        static_cast<CRAddNode*>(crMulOpnds[mulCRIndex])->GetOpnds().cend());
      crMulOpnds.erase(crMulOpnds.cbegin() + mulCRIndex);
    }
    return GetCRMulNode(expr, crMulOpnds);
  }

  // if the crType of operand is CRAddNode, unfold the operand
  // 2 * (X + Y) -> 2 * X + 2 * Y
  if (irMap->GetMIRModule().IsCModule() &&
      crMulOpnds.size() == kNumOpnds && crMulOpnds[0]->GetCRType() == kCRConstNode &&
      crMulOpnds[1]->GetCRType() == kCRAddNode && static_cast<CRAddNode*>(crMulOpnds[1])->GetOpndsSize() == kNumOpnds) {
    auto *addCRNode = static_cast<CRAddNode*>(crMulOpnds[1]);
    std::vector<CRNode*> currOpndsLHSOfMulNode { crMulOpnds[0], addCRNode->GetOpnd(0) };
    std::vector<CRNode*> currOpndsRHSOfMulNode { crMulOpnds[0], addCRNode->GetOpnd(1) };
    CRNode *lhsOfMulNode = GetCRMulNode(nullptr, currOpndsLHSOfMulNode);
    CRNode *rhsOfMulNode = GetCRMulNode(nullptr, currOpndsRHSOfMulNode);
    std::vector<CRNode*> crAddOpnds({ lhsOfMulNode, rhsOfMulNode });
    return GetCRAddNode(expr, crAddOpnds);
  }

  // if the crType of operand is CRDivNode, simplify the operand
  // 8 * (X / 4) -> 2 * X
  if (crMulOpnds.size() > 1 && crMulOpnds[0]->GetCRType() == kCRConstNode) {
    size_t divCRIndex = index;
    while (divCRIndex < crMulOpnds.size() && crMulOpnds[divCRIndex]->GetCRType() != kCRDivNode) {
      ++divCRIndex;
    }
    if (divCRIndex < crMulOpnds.size()) {
      auto *divCRNode = static_cast<CRDivNode*>(crMulOpnds[divCRIndex]);
      if (divCRNode->GetRHS()->GetCRType() == kCRConstNode) {
        auto constantOfDivOperand = static_cast<CRConstNode*>(divCRNode->GetRHS())->GetConstValue();
        auto constantOfMulOperand = static_cast<CRConstNode*>(crMulOpnds[0])->GetConstValue();
        if (constantOfMulOperand == constantOfDivOperand) {
          // 8 * (X / 8) -> X
          crMulOpnds.insert(crMulOpnds.cend(), divCRNode->GetLHS());
          crMulOpnds.erase(crMulOpnds.cbegin() + divCRIndex);
          crMulOpnds.erase(crMulOpnds.cbegin());
          if (crMulOpnds.size() == 1) {
            return crMulOpnds[0];
          }
        } else if (constantOfMulOperand > constantOfDivOperand) {
          auto rem = constantOfMulOperand % constantOfDivOperand;
          if (rem == 0) {
            // 8 * (X / 4) -> X * 2
            auto res = constantOfMulOperand / constantOfDivOperand;
            crMulOpnds.insert(crMulOpnds.cend(), divCRNode->GetLHS());
            crMulOpnds.erase(crMulOpnds.cbegin() + divCRIndex);
            crMulOpnds.erase(crMulOpnds.cbegin());
            crMulOpnds.insert(crMulOpnds.cend(), GetOrCreateCRConstNode(nullptr, res));
          }
        } else {
          auto rem = constantOfDivOperand % constantOfMulOperand;
          if (rem == 0) {
            // 4 * (X / 8) -> X / 2
            auto res = constantOfDivOperand / constantOfMulOperand;
            crMulOpnds.erase(crMulOpnds.cbegin() + divCRIndex);
            crMulOpnds.erase(crMulOpnds.cbegin());
            crMulOpnds.insert(crMulOpnds.cend(),
                              GetOrCreateCRDivNode(nullptr,
                                                   *divCRNode->GetLHS(),
                                                   *GetOrCreateCRConstNode(nullptr, res)));
            if (crMulOpnds.size() == 1) {
              return crMulOpnds[0];
            }
          }
        }
      }
    }
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
      crMulOpnds.erase(crMulOpnds.cbegin(), crMulOpnds.cbegin() + crIndex);
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
    while (crIndex < crMulOpnds.size() && crMulOpnds[crIndex]->GetCRType() == kCRNode) {
      return GetOrCreateCRMulNode(expr, crMulOpnds);
    }
    if (crMulOpnds.size() == 1) {
      return crMulOpnds[0];
    }
  }
  return GetOrCreateCRMulNode(expr, crMulOpnds);
}

CRNode *LoopScalarAnalysisResult::GetOrCreateCRMulNode(MeExpr *expr, const std::vector<CRNode*> &crMulNodes) {
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
    auto *tempCRNode = GetCRAddNode(nullptr, crAddOpnds);
    if (tempCRNode != nullptr && tempCRNode->GetCRType() == kCRConstNode &&
        static_cast<CRConstNode*>(tempCRNode)->GetConstValue() == 0) {
      continue;
    }
    crOpnds.push_back(tempCRNode);
  }
  if (i < lhsCR.GetOpndsSize()) {
    crOpnds.insert(crOpnds.cend(), lhsCR.GetOpnds().begin() + i, lhsCR.GetOpnds().end());
  } else if (i < rhsCR.GetOpndsSize()) {
    crOpnds.insert(crOpnds.cend(), rhsCR.GetOpnds().begin() + i, rhsCR.GetOpnds().end());
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
  if (loop == nullptr) {
    return nullptr;
  }
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
        (static_cast<ConstMeExpr*>(rhs)->GetExtIntValue() == static_cast<ConstMeExpr*>(rhs2)->GetExtIntValue())) {
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
  if (loop == nullptr) {
    return nullptr;
  }
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

CRNode *LoopScalarAnalysisResult::DealWithMeOpOp(MeExpr &currOpMeExpr, MeExpr &expr) {
  OpMeExpr &opMeExpr = static_cast<OpMeExpr&>(currOpMeExpr);
  switch (opMeExpr.GetOp()) {
    case OP_add:
    case OP_sub:
    case OP_mul:
    case OP_div: {
      CHECK_FATAL(opMeExpr.GetNumOpnds() == kNumOpnds, "must be");
      MeExpr *opnd1 = opMeExpr.GetOpnd(0);
      MeExpr *opnd2 = opMeExpr.GetOpnd(1);
      CRNode *lhsCR = GetOrCreateCRNode(*opnd1);
      CRNode *rhsCR = GetOrCreateCRNode(*opnd2);
      if (lhsCR == nullptr || rhsCR == nullptr) {
        return nullptr;
      }
      return ComputeCRNodeWithOperator(expr, *lhsCR, *rhsCR, opMeExpr.GetOp());
    }
    case OP_cvt: {
      if (!computeTripCountForLoopUnroll) {
        return GetOrCreateCRNode(*opMeExpr.GetOpnd(0));
      }
      auto toType = opMeExpr.GetPrimType();
      auto fromType = opMeExpr.GetOpndType();
      if (GetPrimTypeBitSize(toType) > GetPrimTypeBitSize(fromType) && IsUnsignedInteger(fromType)) {
        return nullptr;
      }
      return GetOrCreateCRNode(*opMeExpr.GetOpnd(0));
    }
    case OP_zext: {
      if (!computeTripCountForLoopUnroll) {
        return GetOrCreateCRNode(*opMeExpr.GetOpnd(0));
      }
      InsertExpr2CR(expr, nullptr);
      return nullptr;
    }
    case OP_iaddrof: {
      return GetOrCreateCRNode(*opMeExpr.GetOpnd(0));
    }
    case OP_rem: {
      return GetOrCreateCRVarNode(expr);
    }
    default:
      InsertExpr2CR(expr, nullptr);
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
    case kMeOpVar:
    case kMeOpReg: {
      ScalarMeExpr *scalar = static_cast<ScalarMeExpr*>(&expr);
      if (scalar->IsVolatile()) {
        // volatile variable can not find real def pos, skip it
        InsertExpr2CR(expr, nullptr);
        return nullptr;
      }
      if (scalar->DefByBB() != nullptr && loop != nullptr && !loop->Has(*scalar->DefByBB())) {
        return GetOrCreateLoopInvariantCR(expr);
      }
      switch (scalar->GetDefBy()) {
        case kDefByStmt: {
          MeExpr *rhs = scalar->GetDefStmt()->GetRHS();
          switch (rhs->GetMeOp()) {
            case kMeOpVar: {
              return GetOrCreateCRNode(*rhs);
            }
            case kMeOpOp: {
              return DealWithMeOpOp(*rhs, expr);
            }
            default:
              return GetOrCreateLoopInvariantCR(*rhs);
          }
        }
        case kDefByPhi: {
          if (!computeTripCountForLoopUnroll) {
            return GetOrCreateCRVarNode(expr);
          }
          MePhiNode *phiNode = &(scalar->GetDefPhi());
          if (phiNode->GetOpnds().size() == kNumOpnds) {
            return CreateCRForPhi(*phiNode);
          } else {
            InsertExpr2CR(expr, nullptr);
            return nullptr;
          }
        }
        default:
          return GetOrCreateLoopInvariantCR(expr);
      }
    }
    case kMeOpOp: {
      return DealWithMeOpOp(expr, expr);
    }
    default:
      return GetOrCreateLoopInvariantCR(expr);
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
  if (opMeExpr->GetNumOpnds() != kNumOpnds) {
    return false;
  }
  if (opMeExpr->GetOp() != OP_ge && opMeExpr->GetOp() != OP_le &&
      opMeExpr->GetOp() != OP_lt && opMeExpr->GetOp() != OP_gt &&
      opMeExpr->GetOp() != OP_eq && opMeExpr->GetOp() != OP_ne) {
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

uint64 LoopScalarAnalysisResult::ComputeTripCountWithSimpleConstCR(Opcode op, bool isSigned, int64 value,
                                                                   int64 start, int64 stride) const {
  CHECK_FATAL(stride != 0, "stride must not be zero");
  int64 times = (value - start) / stride;
  if (times < 0) {
    return kInvalidTripCount;
  }
  uint64 remainder = static_cast<uint64>((value - start) % stride);
  switch (op) {
    case OP_ge: {
      if (isSigned && start < value) { return 0; }
      if (!isSigned && static_cast<uint64>(start) < static_cast<uint64>(value)) { return 0; }
      // consider if there's overflow
      if (stride > 0) {
        if (isSigned ||  // undefined overflow
            value == 0 || value < stride ||  // infinite loop
            remainder != 0) {  // not common case, just skip
          return kInvalidTripCount;
        }
        // change to "i <= umax" to compute
        return ComputeTripCountWithSimpleConstCR(OP_le, false, -1, start, stride);
      }
      return static_cast<uint64>(times + 1);
    }
    case OP_gt: {
      if (isSigned && start <= value) { return 0; }
      if (!isSigned && static_cast<uint64>(start) <= static_cast<uint64>(value)) { return 0; }
      // consider if there's overflow
      if (stride > 0) {
        if (isSigned ||  // undefined overflow
            remainder != 0) {  // not common case, just skip
          return kInvalidTripCount;
        }
        // change to "i <= umax" to compute
        return ComputeTripCountWithSimpleConstCR(OP_le, false, -1, start, stride);
      }
      return static_cast<uint64>(times + (remainder != 0));
    }
    case OP_le: {
      if (isSigned && start > value) { return 0; }
      if (!isSigned && static_cast<uint64>(start) > static_cast<uint64>(value)) { return 0; }
      // consider if there's underflow
      if (stride < 0) {
        if (isSigned ||  // undefined underflow
            value == -1 || static_cast<uint64>(value) >= static_cast<uint64>(stride) ||  // infinite loop
            remainder != 0) {  // not common case, just skip
          return kInvalidTripCount;
        }
        // change to "i >= 0" to compute
        return ComputeTripCountWithSimpleConstCR(OP_ge, false, 0, start, stride);
      }
      return static_cast<uint64>(times + 1);
    }
    case OP_lt: {
      if (isSigned && start >= value) { return 0; }
      if (!isSigned && static_cast<uint64>(start) >= static_cast<uint64>(value)) { return 0; }
      // consider if there's underflow
      if (stride < 0) {
        if (isSigned ||  // undefined underflow
            remainder != 0) {  // not common case, just skip
          return kInvalidTripCount;
        }
        // change to "i >= 0" to compute
        return ComputeTripCountWithSimpleConstCR(OP_ge, false, 0, start, stride);
      }
      return static_cast<uint64>(times + static_cast<uint>(remainder != 0));
    }
    case OP_eq: {
      if (start != value) { return 0; }
      return 1;
    }
    case OP_ne: {
      if (start == value) { return 0; }
      if (remainder != 0) { return kInvalidTripCount; }  // infinite loop
      if (stride < 0) {
        // consider if there's underflow
        if (isSigned && start < value) { return kInvalidTripCount; }  // undefined underflow
      } else {
        // consider if there's overflow
        if (isSigned && start > value) { return kInvalidTripCount; }  // undefined overflow
      }
      return static_cast<uint64>(times);
    }
    default:
      CHECK_FATAL(false, "operator must be >=, <=, >, <, !=, ==");
  }
}

void LoopScalarAnalysisResult::SortOperatorCRNode(std::vector<CRNode*> &crNodeOperands, MeExpr &addrExpr) {
  for (auto &opnd : crNodeOperands) {
    (void)SortCROperand(*opnd, addrExpr);
  }
}

void LoopScalarAnalysisResult::PutTheAddrExprAtTheFirstOfVector(
    std::vector<CRNode*> &crNodeOperands, const MeExpr &addrExpr) {
  if (crNodeOperands.size() >= 1 && crNodeOperands[0]->GetExpr() == &addrExpr) {
    return;
  }
  bool find = false;
  CRNode *crNode = nullptr;
  for (auto it = crNodeOperands.begin(); it != crNodeOperands.end();) {
    if ((*it)->GetCRType() != kCRVarNode) {
      ++it;
      continue;
    }
    if ((*it)->GetExpr() == &addrExpr) {
      find = true;
      crNode = *it;
      it = crNodeOperands.erase(it);
      break;
    }
    ++it;
  }
  if (find) {
    crNodeOperands.insert(crNodeOperands.cbegin(), crNode);
  }
}

CRNode &LoopScalarAnalysisResult::SortCROperand(CRNode &crNode, MeExpr &addrExpr) {
  switch (crNode.GetCRType()) {
    case kCRAddNode: {
      auto &crNodeOperands = static_cast<CRAddNode&>(crNode).GetOpnds();
      std::sort(crNodeOperands.begin(), crNodeOperands.end(), CompareCRNodeWithReverseOrderOfCRType());
      SortOperatorCRNode(crNodeOperands, addrExpr);
      PutTheAddrExprAtTheFirstOfVector(crNodeOperands, addrExpr);
      break;
    }
    case kCRMulNode: {
      auto &crNodeOperands = static_cast<CRAddNode&>(crNode).GetOpnds();
      std::sort(crNodeOperands.begin(), crNodeOperands.end(), CompareCRNodeWithReverseOrderOfCRType());
      SortOperatorCRNode(crNodeOperands, addrExpr);
      break;
    }
    default:
      break;
  }
  return crNode;
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

TripCountType LoopScalarAnalysisResult::ComputeTripCount(const MeFunction &func, uint64 &tripCountResult,
                                                         CRNode *&conditionCRNode, CR *&itCR) {
  if (loop == nullptr) {
    return kCouldNotComputeCR;
  }
  enableDebug = false;
  BB *exitBB = func.GetCfg()->GetBBFromID(loop->inloopBB2exitBBs.begin()->first);
  if (exitBB->GetKind() == kBBCondGoto && IsLegal(*(exitBB->GetLastMe()))) {
    auto *brMeStmt = static_cast<CondGotoMeStmt*>(exitBB->GetLastMe());
    BB *brTarget = exitBB->GetSucc(1);
    CHECK_FATAL(brMeStmt->GetOffset() == brTarget->GetBBLabel(), "must be");
    auto *opMeExpr = static_cast<OpMeExpr*>(brMeStmt->GetOpnd());
    Opcode op = OP_undef;
    if (loop->Has(*exitBB->GetSucc(0)) && !loop->Has(*exitBB->GetSucc(1))) {
      op = (brMeStmt->GetOp() == OP_brtrue) ? GetReverseCmpOp(opMeExpr->GetOp()) : opMeExpr->GetOp();
    } else if (loop->Has(*exitBB->GetSucc(1)) && !loop->Has(*exitBB->GetSucc(0))) {
      op = (brMeStmt->GetOp() == OP_brtrue) ? opMeExpr->GetOp() : GetReverseCmpOp(opMeExpr->GetOp());
    } else {
      return kCouldNotComputeCR;
    }
    MeExpr *opnd1 = opMeExpr->GetOpnd(0);
    MeExpr *opnd2 = opMeExpr->GetOpnd(1);
    if (!IsPrimitiveInteger(opMeExpr->GetOpndType())) {
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
      DumpTripCount(*cr, static_cast<int32>(constNode->GetConstValue()), nullptr);
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
      if (crNode1->GetCRType() == kCRConstNode) {
        // swap comparison to make const always being right
        op = op == OP_ge ? OP_le
                         : op == OP_le ? OP_ge
                                       : op == OP_lt ? OP_gt
                                                     : op == OP_gt ? OP_lt
                                                                   : op;
      }
      tripCountResult = ComputeTripCountWithSimpleConstCR(op, IsSignedInteger(opMeExpr->GetOpndType()),
                                                          constNode->GetConstValue(),
                                                          static_cast<CRConstNode*>(cr->GetOpnd(0))->GetConstValue(),
                                                          static_cast<CRConstNode*>(cr->GetOpnd(1))->GetConstValue());
      return (tripCountResult == 0) ? kCouldNotUnroll : kConstCR;
    } else {
      return kCouldNotComputeCR;
    }
  }
  return kCouldNotUnroll;
}
}  // namespace maple
