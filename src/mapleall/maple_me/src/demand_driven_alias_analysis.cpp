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
#include "demand_driven_alias_analysis.h"
#include "bb.h"
#include "dominance.h"
#include "mir_nodes.h"
#include "ssa_mir_nodes.h"
#include "mir_type.h"
#include "orig_symbol.h"
#include "me_cfg.h"
#include "alias_class.h"
#include "me_option.h"

namespace maple {
inline static bool IsAlloc(const BaseNode *x) {
  return x->op == OP_malloc || x->op == OP_gcmalloc || x->op == OP_gcmallocjarray || x->op == OP_alloca ||
         x->op == OP_stackmalloc || x->op == OP_stackmallocjarray;
}

void PEGNode::Dump() {
  LogInfo::MapleLogger() << "PEGNode: ";
  ost->Dump();
  if (attr[kAliasAttrNotAllDefsSeen]) {
    LogInfo::MapleLogger() << " NADS";
  }
  if (attr[kAliasAttrGlobal]) {
    LogInfo::MapleLogger() << " Global";
  }
  if (attr[kAliasAttrFormal]) {
    LogInfo::MapleLogger() << " Formal";
  }
  if (attr[kAliasAttrEscaped]) {
    LogInfo::MapleLogger() << " Escaped";
  }
  if (attr[kAliasAttrNextLevNotAllDefsSeen]) {
    LogInfo::MapleLogger() << " NextLevNADS";
  }
  LogInfo::MapleLogger() << std::endl;
}

PEGNode *ProgramExprGraph::GetOrCreateNodeOf(OriginalSt *ost) {
  uint32 nodeId = allNodes.size();
  const auto &itPair = idOfVal2IdOfNode.insert(std::make_pair(ost->GetIndex(), nodeId));
  // insert op fail, PEGNode of ost has created
  if (!itPair.second) {
    return allNodes[itPair.first->second];
  }

  auto *newNode = memPool->New<PEGNode>(ost);
  allNodes.emplace_back(newNode);
  if (ost->GetIndirectLev() == 0) {
    if (ost->IsFormal()) {
      newNode->attr[kAliasAttrFormal] = true;
    } else if (ost->IsSymbolOst() && ost->GetMIRSymbol()->IsGlobal()) {
      newNode->attr[kAliasAttrGlobal] = true;
    }
  }
  return newNode;
}

PEGNode *ProgramExprGraph::GetNodeOf(OriginalSt *ost) const {
  const auto &it = idOfVal2IdOfNode.find(ost->GetIndex());
  if (it == idOfVal2IdOfNode.end()) {
    return nullptr;
  }
  CHECK_FATAL(allNodes.size() > it->second, "out of range");
  return allNodes[it->second];
}

void ProgramExprGraph::Dump() const {
  LogInfo::MapleLogger() << ">>>>>>>>>>>>>>demand driven alias analysis: peg nodes<<<<<<<<<<<<<<<"
                         << std::endl;
  auto dumpOffset = [](OffsetType offset) {
    LogInfo::MapleLogger() << "{offset: ";
    if (offset.IsInvalid()) {
      LogInfo::MapleLogger() << "UN} ";
    } else {
      LogInfo::MapleLogger() << offset.val << "} ";
    }
  };

  for (auto &node : allNodes) {
    node->ost->Dump();
    if (node->attr[kAliasAttrNotAllDefsSeen]) {
      LogInfo::MapleLogger() << " NADS";
    }
    if (node->attr[kAliasAttrGlobal]) {
      LogInfo::MapleLogger() << " Global";
    }
    if (node->attr[kAliasAttrFormal]) {
      LogInfo::MapleLogger() << " Formal";
    }
    if (node->attr[kAliasAttrEscaped]) {
      LogInfo::MapleLogger() << " Escaped";
    }
    if (node->attr[kAliasAttrNextLevNotAllDefsSeen]) {
      LogInfo::MapleLogger() << " NextLevNADS";
    }

    LogInfo::MapleLogger() << std::endl << ">>>assign from : ";
    for (const auto &assignFrom : node->assignFrom) {
      assignFrom.pegNode->ost->Dump();
      dumpOffset(assignFrom.offset);
    }

    LogInfo::MapleLogger() << std::endl << ">>>assign to : ";
    for (const auto &assignTo : node->assignTo) {
      assignTo.pegNode->ost->Dump();
      dumpOffset(assignTo.offset);
    }

    LogInfo::MapleLogger() << std::endl << ">>>prevLevNode : ";
    if (node->prevLevNode != nullptr) {
      node->prevLevNode->ost->Dump();
      dumpOffset(node->ost->GetOffset());
    }

    LogInfo::MapleLogger() << std::endl << ">>>nextLevNodes : ";
    for (const auto *nextLevNode : node->nextLevNodes) {
      nextLevNode->ost->Dump();
      LogInfo::MapleLogger() << " ";
    }
    LogInfo::MapleLogger() << std::endl << std::endl;
  }
}

PEGBuilder::PtrValueRecorder PEGBuilder::BuildPEGNodeOfDread(const AddrofSSANode *dread) {
  auto *dreadNode = static_cast<const AddrofSSANode *>(dread);
  auto *ost = dreadNode->GetSSAVar()->GetOst();
  PEGNode *pegNode = peg->GetOrCreateNodeOf(ost);
  if (dreadNode->GetMIRSymbol().GetType()->IsStructType() && pegNode->prevLevNode == nullptr) {
    auto *prevLevOst = ost->GetPrevLevelOst();
    if (prevLevOst != nullptr) {
      auto pegNodeOfPrevLevOSt = peg->GetOrCreateNodeOf(prevLevOst);
      pegNodeOfPrevLevOSt->AddNextLevelNode(pegNode);
      pegNode->prevLevNode = pegNodeOfPrevLevOSt;
    }
  }
  return PtrValueRecorder(pegNode, 0, OffsetType(0));
}

PEGBuilder::PtrValueRecorder PEGBuilder::BuildPEGNodeOfRegread(const RegreadSSANode *regread) {
  auto *ost = regread->GetSSAVar()->GetOst();
  if (ost->IsSpecialPreg()) {
    return PtrValueRecorder(nullptr, 0, OffsetType(0));
  }
  return PtrValueRecorder(peg->GetOrCreateNodeOf(ost), 0, OffsetType(0));
}

PEGBuilder::PtrValueRecorder PEGBuilder::BuildPEGNodeOfIread(const IreadSSANode *iread) {
  const auto &ptrNode = BuildPEGNodeOfExpr(iread->Opnd(0));
  if (ptrNode.pegNode == nullptr) {
    return PtrValueRecorder(nullptr, 0, OffsetType(0));
  }

  MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iread->GetTyIdx());
  CHECK_FATAL(mirType->GetKind() == kTypePointer, "CreateAliasElemsExpr: ptr type expected in iread");
  auto *pointedType = static_cast<MIRPtrType*>(mirType)->GetPointedType();
  if (iread->GetFieldID() > 0) {
    pointedType = static_cast<MIRStructType *>(pointedType)->GetFieldType(iread->GetFieldID());
  }
  bool typeHasBeenCasted = pointedType->GetSize() != GetPrimTypeSize(iread->GetPrimType());
  OffsetType offset = typeHasBeenCasted ? OffsetType::InvalidOffset() : ptrNode.offset;
  auto *ostOfBase = ptrNode.pegNode->ost;
  auto *mayUsedOst =
      AliasClass::FindOrCreateExtraLevOst(ssaTab, ostOfBase, iread->GetTyIdx(), iread->GetFieldID(), offset);
  // build prevLev-nextLev relationship
  auto *pegNodeOfMayUsedOSt = peg->GetOrCreateNodeOf(mayUsedOst);

  ptrNode.pegNode->AddNextLevelNode(pegNodeOfMayUsedOSt);
  pegNodeOfMayUsedOSt->SetPrevLevelNode(ptrNode.pegNode);

  return PtrValueRecorder(pegNodeOfMayUsedOSt, 0, OffsetType(0));
}

PEGBuilder::PtrValueRecorder PEGBuilder::BuildPEGNodeOfAddrof(const maple::AddrofSSANode *addrof) {
  auto *ost = addrof->AddrofSSANode::GetSSAVar()->GetOst();
  auto *addrofOst = ost->GetPrevLevelOst();
  if (addrofOst == nullptr) {
    auto *ostOfAgg = ssaTab->FindOrCreateSymbolOriginalSt(*ost->GetMIRSymbol(), ost->GetPuIdx(), 0);
    addrofOst = ssaTab->FindOrCreateAddrofSymbolOriginalSt(ostOfAgg);
    if (ost->GetFieldID() != 0) {
      ost->SetPrevLevelOst(addrofOst);
      addrofOst->AddNextLevelOst(ost);
    }
  }

  OffsetType offset(0);
  if (ost->GetFieldID() > 1) { // offset of field 0 and 1 both are zero
    offset.Set(ost->GetMIRSymbol()->GetType()->GetBitOffsetFromBaseAddr(ost->GetFieldID()));
  }

  auto *pegNodeOfAddrof = peg->GetOrCreateNodeOf(addrofOst);
  auto *pegNodeOfOst = peg->GetOrCreateNodeOf(ost);
  pegNodeOfAddrof->AddNextLevelNode(pegNodeOfOst);
  pegNodeOfOst->SetPrevLevelNode(pegNodeOfAddrof);

  return PtrValueRecorder(pegNodeOfAddrof, ost->GetFieldID(), offset);
}

PEGBuilder::PtrValueRecorder PEGBuilder::BuildPEGNodeOfIaddrof(const IreadNode *iaddrof) {
  const auto &ptrNode = BuildPEGNodeOfExpr(iaddrof->Opnd(0));
  if (ptrNode.pegNode == nullptr) {
    return PtrValueRecorder(nullptr, 0, OffsetType(0));
  }

  OffsetType offset = ptrNode.offset;
  if (iaddrof->GetFieldID() != 0) {
    auto mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iaddrof->GetTyIdx());
    auto pointeeType = static_cast<MIRPtrType*>(mirType)->GetPointedType();
    OffsetType offsetOfField(pointeeType->GetBitOffsetFromBaseAddr(iaddrof->GetFieldID()));
    offset = offset + offsetOfField;
  }

  return PtrValueRecorder(ptrNode.pegNode, iaddrof->GetFieldID() + ptrNode.fieldId, offset);
}

PEGBuilder::PtrValueRecorder PEGBuilder::BuildPEGNodeOfAdd(const BinaryNode *binaryNode) {
  const auto &ptrNode = BuildPEGNodeOfExpr(binaryNode->Opnd(0));
  if (IsAddress(binaryNode->Opnd(0)->GetPrimType())) {
    if (binaryNode->Opnd(1)->GetOpCode() != OP_constval) {
      return PtrValueRecorder(ptrNode.pegNode, 0, OffsetType::InvalidOffset());
    }

    auto *constVal = static_cast<ConstvalNode *>(binaryNode->Opnd(1))->GetConstVal();
    ASSERT(constVal->GetKind() == kConstInt, "pointer cannot add/sub a non-integer value");
    int64 offsetInByte = static_cast<uint64>(static_cast<MIRIntConst *>(constVal)->GetValue());
    int64 offsetInBit = kOffsetUnknown;
    if (offsetInByte < kOffsetMax && offsetInByte > kOffsetMin) {
      constexpr int kBitNumInOneByte = 8;
      offsetInBit = offsetInByte * kBitNumInOneByte;
    }

    OffsetType offset(kOffsetUnknown);
    if (binaryNode->GetOpCode() == OP_sub) {
      offset = ptrNode.offset + (-offsetInBit);
    } else if (binaryNode->GetOpCode() == OP_add) {
      offset = ptrNode.offset + offsetInBit;
    } else {
      CHECK_FATAL(false, "unsupported pointer arithmetic");
    }
    return PtrValueRecorder(ptrNode.pegNode, 0, offset);
  }
  return PtrValueRecorder(ptrNode.pegNode, 0, OffsetType::InvalidOffset());
}

PEGBuilder::PtrValueRecorder PEGBuilder::BuildPEGNodeOfArray(const ArrayNode *arrayNode) {
  for (uint32 id = 1; id < arrayNode->NumOpnds(); ++id) {
    (void) BuildPEGNodeOfExpr(arrayNode->Opnd(id));
  }

  const auto &ptrNode = BuildPEGNodeOfExpr(arrayNode->Opnd(0));
  OffsetType offset = AliasClass::OffsetInBitOfArrayElement(static_cast<const ArrayNode*>(arrayNode));
  return PtrValueRecorder(ptrNode.pegNode, 0, offset + ptrNode.offset);
}

PEGBuilder::PtrValueRecorder PEGBuilder::BuildPEGNodeOfSelect(const  TernaryNode* selectNode) {
  (void) BuildPEGNodeOfExpr(selectNode->Opnd(0));
  const auto &ptrNodeA = BuildPEGNodeOfExpr(selectNode->Opnd(1));
  const auto &ptrNodeB = BuildPEGNodeOfExpr(selectNode->Opnd(2));
  if (ptrNodeA.pegNode == nullptr) {
    return ptrNodeB;
  }
  return ptrNodeA;
}

PEGBuilder::PtrValueRecorder PEGBuilder::BuildPEGNodeOfIntrisic(const IntrinsicopNode* intrinsicNode) {
  if (intrinsicNode->GetIntrinsic() == INTRN_MPL_READ_OVTABLE_ENTRY ||
      (intrinsicNode->GetIntrinsic() == INTRN_JAVA_MERGE && intrinsicNode->NumOpnds() == 1 &&
          intrinsicNode->GetNopndAt(0)->GetOpCode() == OP_dread)) {
    return BuildPEGNodeOfExpr(intrinsicNode->GetNopndAt(0));
  }
  return PtrValueRecorder(nullptr, 0, OffsetType(0));
}

PEGBuilder::PtrValueRecorder PEGBuilder::BuildPEGNodeOfExpr(const BaseNode *expr) {
  switch (expr->GetOpCode()) {
    case OP_dread: {
      return BuildPEGNodeOfDread(static_cast<const AddrofSSANode *>(expr));
    }
    case OP_regread: {
      return BuildPEGNodeOfRegread(static_cast<const RegreadSSANode *>(expr));
    }
    case OP_iread: {
      return BuildPEGNodeOfIread(static_cast<const IreadSSANode *>(expr));
    }
    case OP_addrof: {
      return BuildPEGNodeOfAddrof(static_cast<const AddrofSSANode*>(expr));
    }
    case OP_iaddrof: {
      return BuildPEGNodeOfIaddrof(static_cast<const IreadNode *>(expr));
    }
    case OP_add:
    case OP_sub: {
      return BuildPEGNodeOfAdd(static_cast<const BinaryNode *>(expr));
    }
    case OP_array: {
      return BuildPEGNodeOfArray(static_cast<const ArrayNode*>(expr));
    }
    case OP_retype:
    case OP_cvt: {
      return BuildPEGNodeOfExpr(expr->Opnd(0));
    }
    case OP_select: {
      return BuildPEGNodeOfSelect(static_cast<const TernaryNode *>(expr));
    }
    case OP_intrinsicop: {
      return BuildPEGNodeOfIntrisic(static_cast<const IntrinsicopNode *>(expr));
    }
    default:
      return PtrValueRecorder(nullptr, 0, OffsetType(0));
  }

}

inline bool CallStmtOpndEscape(const StmtNode *stmt) {
  if (stmt->GetOpCode() != OP_call && stmt->GetOpCode() != OP_callassigned) {
    return true;
  }

  auto *mirFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(static_cast<const CallNode*>(stmt)->GetPUIdx());
  if (mirFunc->GetFuncAttrs().GetAttr(FUNCATTR_pure)) {
    return false;
  }
  return true;
}

inline static bool IsAddress(const TyIdx &tyIdx) {
  return IsAddress(GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx)->GetPrimType());
}

void PEGBuilder::AddAssignEdge(const StmtNode *stmt, PEGNode *lhsNode, PEGNode *rhsNode, OffsetType offset) {
  if (lhsNode == nullptr && rhsNode == nullptr) {
    return;
  }
  if (lhsNode == rhsNode && offset.val == 0) {
    return;
  }
  if (lhsNode == nullptr) {
    bool rhsIsAddress = IsAddress(rhsNode->ost->GetTyIdx());
    if (rhsIsAddress) {
      rhsNode->attr[kAliasAttrEscaped] = true;
    }
    return;
  }

  bool lhsIsAddress = IsAddress(lhsNode->ost->GetTyIdx());
  if (rhsNode == nullptr) {
    if (!lhsIsAddress) {
      return;
    }
    if (IsAlloc(stmt->Opnd(0))) {
      return;
    }
    lhsNode->attr[kAliasAttrNextLevNotAllDefsSeen] = true;
    return;
  }

  bool rhsIsAddress = IsAddress(rhsNode->ost->GetTyIdx());
  if (lhsIsAddress) {
    if (lhsNode->ost->IsFormal()) {
      lhsNode->SetMultiDefined();
    }

    if (rhsIsAddress) {
      peg->AddAssignEdge(lhsNode, rhsNode, offset);
      lhsNode->attr |= rhsNode->attr;
    } else {
      lhsNode->attr[kAliasAttrNextLevNotAllDefsSeen] = true;
    }
  } else if (rhsIsAddress) {
    rhsNode->attr[kAliasAttrEscaped] = true;
  } else {
    auto lhsOst = lhsNode->ost;
    auto *lhsType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(lhsOst->GetTyIdx());
    auto rhsOst = rhsNode->ost;
    auto *rhsType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(rhsOst->GetTyIdx());

    if (lhsType != rhsType) {
      return;
    }

    if (lhsType->IsStructType()) {
      auto *preLevOfLHSOst = lhsOst->GetPrevLevelOst();
      if (preLevOfLHSOst == nullptr) {
        return;
      }
      auto *preLevOfRHSOst = rhsOst->GetPrevLevelOst();
      if (preLevOfRHSOst == nullptr) {
        return;
      }
      auto *structType = static_cast<MIRStructType *>(lhsType);
      for (size_t fieldId = 1; fieldId <= structType->NumberOfFieldIDs(); ++fieldId) {
        auto *fieldType = structType->GetFieldType(fieldId);
        if (!IsAddress(fieldType->GetPrimType())) {
          continue;
        }
        OffsetType offset(structType->GetBitOffsetFromBaseAddr(fieldId));

        const auto &nextLevOstsOfLHS = preLevOfLHSOst->GetNextLevelOsts();
        auto fieldOstLHS =
            ssaTab->GetOriginalStTable().FindExtraLevOriginalSt(nextLevOstsOfLHS, fieldType, fieldId, offset);

        const auto &nextLevOstsOfRHS = preLevOfRHSOst->GetNextLevelOsts();
        auto fieldOstRHS =
            ssaTab->GetOriginalStTable().FindExtraLevOriginalSt(nextLevOstsOfRHS, fieldType, fieldId, offset);

        if (fieldOstLHS == nullptr && fieldOstRHS == nullptr) {
          continue;
        }

        // the OriginalSt of at least one side has appearance in code
        if (fieldOstLHS == nullptr) {
          auto *ptrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(lhsOst->GetTyIdx());
          fieldOstLHS = ssaTab->GetOriginalStTable().FindOrCreateExtraLevOriginalSt(
              preLevOfLHSOst, ptrType->GetTypeIndex(), fieldId, offset);
        }

        if (fieldOstRHS == nullptr) {
          auto *ptrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(rhsOst->GetTyIdx());
          fieldOstRHS = ssaTab->GetOriginalStTable().FindOrCreateExtraLevOriginalSt(
              preLevOfRHSOst, ptrType->GetTypeIndex(), fieldId, offset);
        }

        auto pegNodeOfLhsField = peg->GetOrCreateNodeOf(fieldOstLHS);
        auto *prevLevNodeLhs = lhsNode->prevLevNode;
        if (prevLevNodeLhs != nullptr) {
          prevLevNodeLhs->AddNextLevelNode(pegNodeOfLhsField);
          pegNodeOfLhsField->SetPrevLevelNode(prevLevNodeLhs);
        }

        auto pegNodeOfRhsField = peg->GetOrCreateNodeOf(fieldOstRHS);
        auto *prevLevNodeRhs = rhsNode->prevLevNode;
        if (prevLevNodeRhs != nullptr) {
          prevLevNodeRhs->AddNextLevelNode(pegNodeOfRhsField);
          pegNodeOfRhsField->SetPrevLevelNode(prevLevNodeRhs);
        }

        peg->AddAssignEdge(pegNodeOfLhsField, pegNodeOfRhsField, offset);
      }
    }
  }
}

void PEGBuilder::BuildPEGNodeInAssign(const StmtNode *stmt) {
  OriginalSt *ost = ssaTab->GetStmtsSSAPart().SSAPartOf(*stmt)->GetSSAVar()->GetOst();
  PEGNode *lhsNode = peg->GetOrCreateNodeOf(ost);
  CHECK_FATAL(lhsNode != nullptr, "failed in create PEGNode");

  if (lhsNode->prevLevNode == nullptr && ost->IsSymbolOst() && ost->GetMIRSymbol()->GetType()->IsStructType()) {
    auto *prevLevOst = ost->GetPrevLevelOst();
    if (prevLevOst == nullptr) {
      auto *ostOfAgg = ssaTab->FindOrCreateSymbolOriginalSt(*ost->GetMIRSymbol(), ost->GetPuIdx(), 0);
      prevLevOst = ssaTab->FindOrCreateAddrofSymbolOriginalSt(ostOfAgg);
      if (ost->GetFieldID() != 0) {
        ost->SetPrevLevelOst(prevLevOst);
        prevLevOst->AddNextLevelOst(ost);
      }
    }
    auto pegNodeOfPrevLevOSt = peg->GetOrCreateNodeOf(prevLevOst);
    pegNodeOfPrevLevOSt->AddNextLevelNode(lhsNode);
    lhsNode->SetPrevLevelNode(pegNodeOfPrevLevOSt);
  }

  // get or create PEGNode of rhs
  if (stmt->Opnd(0)->GetOpCode() == OP_select) {
    (void) BuildPEGNodeOfExpr(stmt->Opnd(0)->Opnd(0)); // build PEGNode in condition expr
    const auto &rhsPtrValNodeA = BuildPEGNodeOfExpr(stmt->Opnd(0)->Opnd(1));
    AddAssignEdge(stmt, lhsNode, rhsPtrValNodeA.pegNode, rhsPtrValNodeA.offset);

    const auto &rhsPtrValNodeB = BuildPEGNodeOfExpr(stmt->Opnd(0)->Opnd(2));
    AddAssignEdge(stmt, lhsNode, rhsPtrValNodeB.pegNode, rhsPtrValNodeB.offset);
    return;
  } else if (stmt->Opnd(0)->GetOpCode() == OP_constval &&
             static_cast<ConstvalNode*>(stmt->Opnd(0))->GetConstVal()->IsZero()) {
    return;
  }
  const auto &rhsPtrValNode = BuildPEGNodeOfExpr(stmt->Opnd(0));
  AddAssignEdge(stmt, lhsNode, rhsPtrValNode.pegNode, rhsPtrValNode.offset);
}

void PEGBuilder::BuildPEGNodeInIassign(const IassignNode *iassign) {
  const auto &baseAddrValNode = BuildPEGNodeOfExpr(iassign->IassignNode::Opnd(0));
  const auto &rhsPtrNode = BuildPEGNodeOfExpr(iassign->Opnd(1));
  if (baseAddrValNode.pegNode == nullptr) {
    AddAssignEdge(iassign, nullptr, rhsPtrNode.pegNode, rhsPtrNode.offset);
    return;
  }

  auto *ostOfBase = baseAddrValNode.pegNode->ost;
  OriginalSt *defedOst = AliasClass::FindOrCreateExtraLevOst(
      ssaTab, ostOfBase, iassign->GetTyIdx(), iassign->GetFieldID(), baseAddrValNode.offset);
  PEGNode *lhsNode = peg->GetOrCreateNodeOf(defedOst);

  // build prevLev-nextLev relation
  baseAddrValNode.pegNode->AddNextLevelNode(lhsNode);
  lhsNode->SetPrevLevelNode(baseAddrValNode.pegNode);

  AddAssignEdge(iassign, lhsNode, rhsPtrNode.pegNode, rhsPtrNode.offset);
}

void PEGBuilder::BuildPEGNodeInDirectCall(const CallNode *call) {
  bool opndEscape = CallStmtOpndEscape(call);
  if (!opndEscape) {
    return;
  }
  for (uint32 opndId = 0; opndId < call->NumOpnds(); ++opndId) {
    const auto &opndNode = BuildPEGNodeOfExpr(call->Opnd(opndId));
    if (opndNode.pegNode != nullptr) {
      opndNode.pegNode->attr[kAliasAttrEscaped] = true;
    }
  }
}

void PEGBuilder::BuildPEGNodeInIcall(const IcallNode *icall) {
  BuildPEGNodeOfExpr(icall->IcallNode::Opnd(0));
  for (uint32 opndId = 1; opndId < icall->NumOpnds(); ++opndId) {
    const auto &opndPtrNode = BuildPEGNodeOfExpr(icall->IcallNode::Opnd(opndId));
    auto *pegNode = opndPtrNode.pegNode;
    if (pegNode != nullptr) {
      pegNode->attr[kAliasAttrEscaped] = true;
    }
  }
}

void PEGBuilder::BuildPEGNodeInVirtualcall(const NaryStmtNode *virtualCall) {
  for (uint32 opndId = 0; opndId < virtualCall->NumOpnds(); ++opndId) {
    const auto &opndPtrNode = BuildPEGNodeOfExpr(virtualCall->Opnd(opndId));
    auto *pegNode = opndPtrNode.pegNode;
    if (pegNode != nullptr) {
      pegNode->attr[kAliasAttrEscaped] = true;
    }
  }
}

void PEGBuilder::BuildPEGNodeInIntrinsicCall(const IntrinsiccallNode *intrinsiccall) {
  uint32 opndId = 0;
  if (intrinsiccall->GetIntrinsic() == INTRN_JAVA_POLYMORPHIC_CALL) {
    BuildPEGNodeOfExpr(intrinsiccall->Opnd(opndId));
    ++opndId;
    for (; opndId < intrinsiccall->NumOpnds(); ++opndId) {
      const auto &opndPtrNode = BuildPEGNodeOfExpr(intrinsiccall->Opnd(opndId));
      auto *pegNode = opndPtrNode.pegNode;
      if (pegNode != nullptr) {
        pegNode->attr[kAliasAttrNextLevNotAllDefsSeen] = true;
      }
    }
  } else if (intrinsiccall->GetIntrinsic() == INTRN_C_va_start) {
    // opndId == 0, the first arg is set to points to formal
    const auto &opndPtrNode = BuildPEGNodeOfExpr(intrinsiccall->Opnd(opndId));
    opndPtrNode.pegNode->attr[kAliasAttrNextLevNotAllDefsSeen] = true;
  }
  for (; opndId < intrinsiccall->NumOpnds(); ++opndId) {
    BuildPEGNodeOfExpr(intrinsiccall->Opnd(opndId));
  }
}

void PEGBuilder::BuildPEGNodeInStmt(const StmtNode *stmt) {
  switch (stmt->GetOpCode()) {
    case OP_maydassign:
    case OP_regassign:
    case OP_dassign: {
      BuildPEGNodeInAssign(stmt);
      return;
    }
    case OP_iassign: {
      BuildPEGNodeInIassign(static_cast<const IassignNode*>(stmt));
      return;
    }
    case OP_throw: {
      auto *pegNode = BuildPEGNodeOfExpr(stmt->Opnd(0)).pegNode;
      if (IsAddress(stmt->Opnd(0)->GetPrimType())) {
        pegNode->attr[kAliasAttrEscaped] = true;
      }
      return;
    }
    case OP_call:
    case OP_callassigned: {
      BuildPEGNodeInDirectCall(static_cast<const CallNode *>(stmt));
      break;
    }
    case OP_asm:
    case OP_virtualcall:
    case OP_virtualicall:
    case OP_superclasscall:
    case OP_interfacecall:
    case OP_interfaceicall:
    case OP_customcall:
    case OP_polymorphiccall:
    case OP_virtualcallassigned:
    case OP_virtualicallassigned:
    case OP_superclasscallassigned:
    case OP_interfacecallassigned:
    case OP_interfaceicallassigned:
    case OP_customcallassigned:
    case OP_polymorphiccallassigned: {
      BuildPEGNodeInVirtualcall(static_cast<const NaryStmtNode *>(stmt));
      break;
    }
    case OP_icall:
    case OP_icallassigned: {
      BuildPEGNodeInIcall(static_cast<const IcallNode *>(stmt));
      break;
    }
    case OP_intrinsiccall:
    case OP_intrinsiccallassigned: {
      BuildPEGNodeInIntrinsicCall(static_cast<const IntrinsiccallNode*>(stmt));
      break;
    }
    default:
      break;
  }

  if (kOpcodeInfo.IsCallAssigned(stmt->GetOpCode())) {
    auto &mustDefNodes = ssaTab->GetStmtsSSAPart().SSAPartOf(*stmt)->GetMustDefNodes();
    if (!mustDefNodes.empty()) {
      auto *mustDefedOst = mustDefNodes.front().GetResult()->GetOst();
      auto *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(mustDefedOst->GetTyIdx());
      if (IsAddress(mirType->GetPrimType())) {
        peg->GetOrCreateNodeOf(mustDefedOst)->attr[kAliasAttrNextLevNotAllDefsSeen] = true;
      }
    }
  }
}

void ResetNextLevelOffsetResultingFromMultiDefOfPtr(PEGNode *node, std::set<PEGNode*> updatedPEGNode) {
  const auto &it = updatedPEGNode.insert(node);
  // insert failed, the node has been updated
  if (!it.second) {
    return;
  }

  node->SetMultiDefined();
  for (auto *nextLevNode : node->nextLevNodes) {
    nextLevNode->attr[kAliasAttrNextLevNotAllDefsSeen] = true;
  }

  for (const auto &assignTo : node->assignTo) {
    ResetNextLevelOffsetResultingFromMultiDefOfPtr(assignTo.pegNode, updatedPEGNode);
  }
}

void PropGlobalAndFormalAttr(PEGNode *node, AliasAttribute attr, int32 indirectLev, std::set<PEGNode*> &processed) {
  if (node->attr[attr] || indirectLev > 0) {
    return;
  }

  const auto &nodePair = processed.insert(node);
  // insert failed, the node has been processed
  if (!nodePair.second) {
    return;
  }

  if (indirectLev == 0) {
    node->attr[attr] = true;
  }

  for (const auto &assignToNode : node->assignTo) {
    PropGlobalAndFormalAttr(assignToNode.pegNode, attr, indirectLev, processed);
  }

  auto prevLevNode = node->prevLevNode;
  if (prevLevNode != nullptr) {
    PropGlobalAndFormalAttr(prevLevNode, attr, indirectLev - 1, processed);
  }

  for (auto *nextLevNode : node->nextLevNodes) {
    PropGlobalAndFormalAttr(nextLevNode, attr, indirectLev + 1, processed);
  }
}

void PEGBuilder::UpdateAttributes() {
  // multi-assign of ost results in uncertainty of value of the ost. For example:
  // L1: int *ptr = array;
  // L2: def/use *(ptr + 1) = 1;
  // L3: ptr = array + 1;
  // L4: use *ptr; <=== *ptr alias with L2: *(ptr+1)
  std::set<PEGNode*> updatedPEGNode;
  for (auto *node : peg->GetAllNodes()) {
    if (node->ost->GetIndirectLev() <= 0) {
      if (node->ost->IsSymbolOst() && node->ost->GetMIRSymbol()->IsGlobal()) {
        std::set<PEGNode*> processed;
        PropGlobalAndFormalAttr(node, kAliasAttrGlobal, node->ost->GetIndirectLev(), processed);
        continue;
      }
      if (node->ost->IsFormal()) {
        std::set<PEGNode*> processed;
        PropGlobalAndFormalAttr(node, kAliasAttrFormal, node->ost->GetIndirectLev(), processed);
      }
    }

    if (node->ost->GetIndirectLev() > 0 || node->assignFrom.size() > 1 ||
        (node->ost->IsFormal() && node->assignFrom.size() > 0) ||
        (node->ost->IsSymbolOst() && node->ost->GetMIRSymbol()->IsStatic())) {
      ResetNextLevelOffsetResultingFromMultiDefOfPtr(node, updatedPEGNode);
    }
  }
  updatedPEGNode.clear();

  // update alias attributes
  bool changed = true;
  while (changed) {
    changed = false;
    for (auto *node : peg->GetAllNodes()) {
      if (node->attr[kAliasAttrNotAllDefsSeen]) {
        for (auto *nextLevNode : node->nextLevNodes) {
          auto &attr = nextLevNode->attr;
          if (!attr[kAliasAttrNotAllDefsSeen]) {
            attr[kAliasAttrNotAllDefsSeen] = true;
            changed = true;
          }
        }
        for (const auto &assignFrom : node->assignFrom) {
          auto &attr = assignFrom.pegNode->attr;
          if (!attr[kAliasAttrEscaped]) {
            attr[kAliasAttrEscaped] = true;
            changed = true;
          }
        }
        continue;
      }

      if (node->attr[kAliasAttrNextLevNotAllDefsSeen]) {
        for (auto *nextLevNode : node->nextLevNodes) {
          auto &attr = nextLevNode->attr;
          if (!attr[kAliasAttrNotAllDefsSeen]) {
            attr[kAliasAttrNotAllDefsSeen] = true;
            changed = true;
          }
        }
      }

      if (node->attr[kAliasAttrGlobal]) {
        for (auto *nextLevNode : node->nextLevNodes) {
          auto &attr = nextLevNode->attr;
          if (!attr[kAliasAttrNotAllDefsSeen]) {
            attr[kAliasAttrNotAllDefsSeen] = true;
            changed = true;
          }
        }
      }

      if (node->attr[kAliasAttrFormal]) {
        for (auto *nextLevNode : node->nextLevNodes) {
          auto &attr = nextLevNode->attr;
          if (!attr[kAliasAttrNotAllDefsSeen]) {
            attr[kAliasAttrNotAllDefsSeen] = true;
            changed = true;
          }
        }
      }

      // update alias attribute of next-level nodes
      if (node->attr[kAliasAttrEscaped]) {
        for (auto *nextLevNode : node->nextLevNodes) {
          auto &attr = nextLevNode->attr;
          if (!attr[kAliasAttrNextLevNotAllDefsSeen]) {
            attr[kAliasAttrNextLevNotAllDefsSeen] = true;
            changed = true;
          }
        }
        for (auto &assignFrom : node->assignFrom) {
          auto &attr = assignFrom.pegNode->attr;
          if (!attr[kAliasAttrEscaped]) {
            attr[kAliasAttrEscaped] = true;
            changed = true;
          }
        }
      }

      // update alias attribute of assign-to nodes
      for (const auto &assignTo : node->assignTo) {
        auto newAttr = assignTo.pegNode->attr | node->attr;
        if (newAttr != assignTo.pegNode->attr) {
          assignTo.pegNode->attr = newAttr;
          changed = true;
        }
      }
    }
  }
}

void PEGBuilder::BuildPEG() {
  for (const auto bb : func->GetCfg()->GetAllBBs()) {
    if (bb == nullptr || bb->IsEmpty()) {
      continue;
    }
    for (const auto &stmt : bb->GetStmtNodes()) {
      BuildPEGNodeInStmt(&stmt);
    }
  }

  UpdateAttributes();
}

void DemandDrivenAliasAnalysis::Propagate(WorkListType &workList, PEGNode *to, PEGNode *src, ReachState state,
                                          OffsetType offset) {
  if (to == nullptr || src == nullptr) {
    return;
  }
  bool insertNewNode = AddReachNode(to, src, state, offset);
  if (insertNewNode) {
    workList.push_back(WorkListItem(to, src, state, offset));
    to->attr |= src->attr;
    if (enableDebug) {
      LogInfo::MapleLogger() << "===New candidate: ";
      src->ost->Dump();
      LogInfo::MapleLogger() << " + " << offset.val << " => ";
      to->ost->Dump();
      LogInfo::MapleLogger() << std::endl << std::endl;
    }
  }
}

bool DemandDrivenAliasAnalysis::AliasBasedOnAliasAttr(PEGNode *to, PEGNode *src) const {
  if (to->attr[kAliasAttrNotAllDefsSeen]) {
    if (src->attr[kAliasAttrNotAllDefsSeen]) {
      return true;
    }
    if (src->attr[kAliasAttrGlobal]) {
      return true;
    }
    auto *prevLevNodeOfSrc = GetPrevLevPEGNode(src);
    if (prevLevNodeOfSrc != nullptr && prevLevNodeOfSrc->attr[kAliasAttrEscaped]) {
      return true;
    }
  }

  if (to->attr[kAliasAttrGlobal]) {
    if (src->attr[kAliasAttrNotAllDefsSeen]) {
      return true;
    }
  }

  auto *prevLevNodeOfTo = GetPrevLevPEGNode(to);
  if (prevLevNodeOfTo != nullptr && prevLevNodeOfTo->attr[kAliasAttrEscaped]) {
    if (src->attr[kAliasAttrNotAllDefsSeen]) {
      return true;
    }
  }

  return false;
}

inline static bool MemOverlapAccordingOffset(OffsetType startA, OffsetType endA, OffsetType startB, OffsetType endB) {
  if (startA == startB) {
    return true;
  }
  if (startA.IsInvalid() || endA.IsInvalid() || startB.IsInvalid() || endB.IsInvalid()) {
    return true;
  }
  return startA < endB && startB < endA;
}

inline static OffsetType OffsetFromPrevLevNode(const PEGNode *pegNode) {
  if (pegNode->prevLevNode->multiDefed) {
    return OffsetType::InvalidOffset();
  }

  return pegNode->ost->GetOffset();
}

void DemandDrivenAliasAnalysis::UpdateAliasInfoOfPegNode(PEGNode *pegNode) {
  WorkListType workList{WorkListItem(pegNode, pegNode, S1, OffsetType(0))};

  while (!workList.empty()) {
    const auto &item = workList.front();
    auto *toNode = item.to;
    auto *srcNode = item.srcItem.src;
    auto state = item.srcItem.state;
    auto offset = item.srcItem.offset;
    workList.pop_front();

    // toNode->ost value alias with srcNode->ost, the nextLevOsts may memory alias with each other
    for (auto *nextLevNodeOfTo : toNode->nextLevNodes) {
      constexpr int kBitNumInOneByte = 8;
      OffsetType offsetStartA = OffsetFromPrevLevNode(nextLevNodeOfTo) + offset;
      OffsetType offsetEndA = offsetStartA + nextLevNodeOfTo->ost->GetType()->GetSize() * kBitNumInOneByte;
      for (auto *nextLevNodeOfSrc : srcNode->nextLevNodes) {
        OffsetType offsetStartB = OffsetFromPrevLevNode(nextLevNodeOfSrc);
        OffsetType offsetEndB = offsetStartB + nextLevNodeOfSrc->ost->GetType()->GetSize() * kBitNumInOneByte;

        if (MemOverlapAccordingOffset(offsetStartA, offsetEndA, offsetStartB, offsetEndB)) {
          AddAlias(nextLevNodeOfTo->ost, nextLevNodeOfSrc->ost);
          for (const auto &reachItem : *ReachSetOf(nextLevNodeOfSrc)) {
            switch (reachItem->state) {
              case S1: {
                Propagate(workList, nextLevNodeOfTo, reachItem->src, S2, reachItem->offset);
                break;
              }
              case S3: {
                Propagate(workList, nextLevNodeOfTo, reachItem->src, S4, reachItem->offset);
                break;
              }
              default:
                break;
            }
          }
        }
      }
    }

    switch (state) {
      case S1: {
        for (const auto &readNode : toNode->assignFrom) {
          Propagate(workList, readNode.pegNode, srcNode, S1, offset + readNode.offset);
        }
        for (auto *aliasOst : *AliasSetOf(toNode->ost)) {
          auto *aliasNode = peg.GetNodeOf(aliasOst);
          Propagate(workList, aliasNode, srcNode, S2, offset);
        }
        for (const auto &writeNode : toNode->assignTo) {
          Propagate(workList, writeNode.pegNode, srcNode, S3, writeNode.offset + (-offset));
        }
        break;
      }
      case S2: {
        for (const auto &readNode : toNode->assignFrom) {
          Propagate(workList, readNode.pegNode, srcNode, S1, offset +  readNode.offset);
        }
        for (const auto &writeNode : toNode->assignTo) {
          Propagate(workList, writeNode.pegNode, srcNode, S3, writeNode.offset + (-offset));
        }
        break;
      }
      case S3: {
        for (const auto &writeNode : toNode->assignTo) {
          Propagate(workList, writeNode.pegNode, srcNode, S3, writeNode.offset + offset);
        }
        for (auto *aliasOst : *AliasSetOf(toNode->ost)) {
          auto *aliasNode = peg.GetNodeOf(aliasOst);
          Propagate(workList, aliasNode, srcNode, S4, offset);
        }
        break;
      }
      case S4: {
        for (const auto &writeNode : toNode->assignTo) {
          Propagate(workList, writeNode.pegNode, srcNode, S3, writeNode.offset + offset);
        }
        break;
      }
      default: {
        CHECK_FATAL(false, "Not supported state");
        break;
      }
    }

    // propagate downward
    if (state != S1 && state != S3) {
      continue;
    }
    auto pegNodeOfPrevLevOst = toNode->prevLevNode;
    if (pegNodeOfPrevLevOst != nullptr) {
      Propagate(workList, pegNodeOfPrevLevOst, pegNodeOfPrevLevOst, S1);
    }
  }
}

bool DemandDrivenAliasAnalysis::MayAlias(PEGNode *to, PEGNode *src) {
  if (to == src) {
    return true;
  }
  auto *ostOfTo = to->ost;
  auto *ostOfSrc = src->ost;
  // alias based on AliasAttr
  auto *nodeOfPrevLevOfTo = to->prevLevNode;
  auto *nodeOfPrevLevOfSrc = src->prevLevNode;
  if (nodeOfPrevLevOfTo != nodeOfPrevLevOfSrc || nodeOfPrevLevOfTo->multiDefed) {
    if (AliasBasedOnAliasAttr(to, src)) {
      AddAlias(ostOfTo, ostOfSrc);
      return true;
    }
  }

  if (nodeOfPrevLevOfTo == nullptr || nodeOfPrevLevOfSrc == nullptr) {
    return false;
  }

  if (nodeOfPrevLevOfTo->processed || nodeOfPrevLevOfSrc->processed) {
    auto *aliasSetOfToOst = AliasSetOf(ostOfTo);
    return (aliasSetOfToOst->find(ostOfSrc) != aliasSetOfToOst->end());
  }

  UpdateAliasInfoOfPegNode(nodeOfPrevLevOfTo);
  nodeOfPrevLevOfTo->processed = true;

  auto *aliasSetOfToOst = AliasSetOf(ostOfTo);
  if (aliasSetOfToOst->find(ostOfSrc) != aliasSetOfToOst->end()) {
    return true;
  }

  if (nodeOfPrevLevOfTo != nodeOfPrevLevOfSrc) {
    bool aliasAccordingAttr = AliasBasedOnAliasAttr(to, src);
    if (aliasAccordingAttr) {
      AddAlias(to->ost, src->ost);
    }
    return aliasAccordingAttr;
  }
  return false;
}

bool DemandDrivenAliasAnalysis::MayAlias(OriginalSt *ostA, OriginalSt *ostB) {
  auto *aliasSet = AliasSetOf(ostA);
  if (aliasSet->find(ostB) != aliasSet->end()) {
    return true;
  }

  bool mayAliasAccordingBasicAA = AliasClass::MayAliasBasicAA(ostA, ostB);
  if (!mayAliasAccordingBasicAA) {
    return false;
  }
  if (ostA->GetIndirectLev() <= 0 && ostB->GetIndirectLev() <= 0) {
    return mayAliasAccordingBasicAA;
  }

  auto *to = peg.GetNodeOf(ostA);
  auto *src = peg.GetNodeOf(ostB);
  if (to == nullptr) {
    return true;
  }
  if (src == nullptr) {
    return true;
  }

  bool aliasAccordingDDAA = MayAlias(to, src);
  if (enableDebug) {
    LogInfo::MapleLogger() << "Demand Driven Alias Aanlysis: ";
    to->ost->Dump();
    LogInfo::MapleLogger() << " and ";
    src->ost->Dump();
    if (!aliasAccordingDDAA) {
      LogInfo::MapleLogger() << " not ";
    }
    LogInfo::MapleLogger() << " alias." << std::endl;
  }
  return aliasAccordingDDAA;
}
} // namespace maple
