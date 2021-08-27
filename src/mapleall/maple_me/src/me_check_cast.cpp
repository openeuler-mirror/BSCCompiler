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
#include "me_check_cast.h"
#include "class_hierarchy_phase.h"
#include "me_phase_manager.h"

namespace maple {
MIRStructType *GetClassTypeFromName(const std::string &className);
void FunctionGenericDump(MIRFunction &func);

std::string GetLabel(AnnotationType &aType) {
  std::string lable;
  switch (aType.GetKind()) {
    case kPrimeType:
    case kGenericType: {
      lable = aType.GetName();
      break;
    }
    case kGenericDeclare: {
      GenericDeclare *gDeclare = static_cast<GenericDeclare*>(&aType);
      lable = "(" + gDeclare->GetBelongToName() + ")" + ": T" + gDeclare->GetName();
      break;
    }
    case kGenericMatch: {
      lable = "*";
      break;
    }
    case kExtendType: {
      ExtendGeneric *extendGeneric = static_cast<ExtendGeneric*>(&aType);
      AnnotationType *contains = extendGeneric->GetContainsGeneric();
      if (extendGeneric->GetExtendInfo() == kHierarchyExtend) {
        lable = "(+)" + GetLabel(*contains);
      } else if (extendGeneric->GetExtendInfo() == kArrayType) {
        lable = "([)" + GetLabel(*contains);
      } else {
        lable = "(-)" + GetLabel(*contains);
      }
      break;
    }
    default:
      CHECK_FATAL(false, "must be");
  }
  return lable;
}

void CheckCast::DumpGenericNode(GenericNode &node, std::ostream &out) {
  std::string lable = GetLabel(*node.aType);
  out << node.aType->GetId() << " [label=\"" << lable << "\"];\n";
  if (node.next != nullptr) {
    out << node.aType->GetId() << "->" << node.next->aType->GetId() << ";\n";
  }
}

void CheckCast::DumpGenericGraph() {
  std::filebuf fileBuf;
  std::string outFile = func->GetName() + "-GenericGraph.dot";
  std::filebuf *fileBufPtr = fileBuf.open(outFile, std::ios::trunc | std::ios::out);
  CHECK_FATAL(fileBufPtr, "open file : %s failed!", outFile.c_str());
  std::ostream dotFile(&fileBuf);
  dotFile << "digraph InequalityGraph {\n";
  for (auto pair : created) {
    GenericNode *node = pair.second;
    DumpGenericNode(*node, dotFile);
  }
  dotFile << "}\n";
  fileBufPtr = fileBuf.close();
  CHECK_FATAL(fileBufPtr, "close file : %s failed", outFile.c_str());
}

GenericNode *CheckCast::GetOrCreateGenericNode(AnnotationType *annoType) {
  if (created.find(annoType) != created.end()) {
    return created[annoType];
  }
  GenericNode *newNode = graphTempMem->New<GenericNode>();
  newNode->aType = annoType;
  created[annoType] = newNode;
  return newNode;
}

void CheckCast::AddNextNode(GenericNode &from, GenericNode &to) const {
  if (from.multiOutput ||
      (from.next != nullptr && from.next != &to && from.next->aType->GetKind() != kGenericDeclare)) {
    from.multiOutput = true;
    from.next = nullptr;
    return;
  }
  from.next = &to;
}

void CheckCast::BuildGenericGraph(AnnotationType *annoType) {
  if (annoType == nullptr) {
    return;
  }
  switch (annoType->GetKind()) {
    case kGenericType: {
      GenericType *gType = static_cast<GenericType*>(annoType);
      for (auto pair : gType->GetGenericMap()) {
        GenericDeclare *gDeclare = pair.first;
        AnnotationType *realType = pair.second;
        GenericNode *gDeclareNode = GetOrCreateGenericNode(gDeclare);
        GenericNode *realTypeNode = GetOrCreateGenericNode(realType);
        AddNextNode(*gDeclareNode, *realTypeNode);
        BuildGenericGraph(realType);
      }
      break;
    }
    case kExtendType: {
      ExtendGeneric *extendGeneric = static_cast<ExtendGeneric*>(annoType);
      if (extendGeneric->GetExtendInfo() == kHierarchyExtend) {
        GenericNode *gExtendNode = GetOrCreateGenericNode(annoType);
        GenericNode *containsNode = GetOrCreateGenericNode(extendGeneric->GetContainsGeneric());
        AddNextNode(*gExtendNode, *containsNode);
        BuildGenericGraph(extendGeneric->GetContainsGeneric());
      }
      break;
    }
    case kGenericDeclare:
    case kGenericMatch:
    case kPrimeType:
      break;
    default:
      CHECK_FATAL(false, "must be");
  }
}

void CheckCast::TryToResolveFuncArg(MeExpr &expr, AnnotationType &at) {
  MIRType *mirType = nullptr;
  if (at.GetKind() == kGenericDeclare) {
    if (expr.GetMeOp() == kMeOpVar) {
      VarMeExpr *var = static_cast<VarMeExpr*>(&expr);
      const OriginalSt *symOst = func->GetIRMap()->GetSSATab().GetOriginalStFromID(var->GetOstIdx());
      const MIRSymbol *sym = symOst->GetMIRSymbol();
      mirType = sym->GetType();
    } else if (expr.GetMeOp() == kMeOpIvar) {
      IvarMeExpr *ivar = static_cast<IvarMeExpr*>(&expr);
      mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ivar->GetTyIdx());
      if (mirType->GetKind() == kTypePointer) {
        mirType = static_cast<MIRPtrType*>(mirType)->GetPointedType();
      }
      CHECK_FATAL(mirType->IsStructType(), "must be");
      mirType = static_cast<MIRStructType*>(mirType)->GetFieldType(ivar->GetFieldID());
    } else {
      return;
    }
    if (mirType->GetKind() == kTypePointer) {
      mirType = static_cast<MIRPtrType*>(mirType)->GetPointedType();
    }
    GenericType *realType = graphTempMem->New<GenericType>(mirType->GetNameStrIdx(),
                                                           mirType, graphTempAllocator);
    GenericNode *realNode = GetOrCreateGenericNode(realType);
    GenericNode *declareNode = GetOrCreateGenericNode(&at);
    AddNextNode(*declareNode, *realNode);
  } else if (at.GetKind() == kGenericType && expr.GetMeOp() == kMeOpVar) {
    VarMeExpr *var = static_cast<VarMeExpr*>(&expr);
    if (var->GetDefBy() != kDefByStmt) {
      return;
    }
    GenericType *gt = static_cast<GenericType*>(&at);
    MeStmt *defStmt = var->GetDefStmt();
    if (!(defStmt->GetOp() == OP_dassign && defStmt->GetRHS()->GetMeOp() == kMeOpNary)) {
      return;
    }
    NaryMeExpr *naryExpr = static_cast<NaryMeExpr*>(defStmt->GetRHS());
    if (naryExpr->GetIntrinsic() == INTRN_JAVA_CONST_CLASS) {
      mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(naryExpr->GetTyIdx());
      if (mirType->GetKind() == kTypePointer) {
        mirType = static_cast<MIRPtrType*>(mirType)->GetPointedType();
      }
      MapleMap<GenericDeclare*, AnnotationType*> &gtMap = gt->GetGenericMap();
      CHECK_FATAL(gtMap.size() == 1, "must be");
      AnnotationType *gd = gtMap.begin()->second;
      if (gd->GetKind() != kGenericDeclare) {
        return;
      }
      GenericType *realType = graphTempMem->New<GenericType>(mirType->GetNameStrIdx(), mirType,
                                                             graphTempAllocator);
      GenericNode *realNode = GetOrCreateGenericNode(realType);
      GenericNode *declareNode = GetOrCreateGenericNode(gd);
      AddNextNode(*declareNode, *realNode);
    }
  } else if(at.GetKind() == kExtendType && expr.GetMeOp() == kMeOpVar) {
    ExtendGeneric *extendG = static_cast<ExtendGeneric*>(&at);
    CHECK_FATAL(extendG->GetExtendInfo() == kArrayType, "must be");
    if (extendG->GetContainsGeneric()->GetKind() != kGenericDeclare) {
      return;
    }
    VarMeExpr *var = static_cast<VarMeExpr*>(&expr);
    const OriginalSt *symOst = func->GetIRMap()->GetSSATab().GetOriginalStFromID(var->GetOstIdx());
    const MIRSymbol *sym = symOst->GetMIRSymbol();
    mirType = sym->GetType();
    if (mirType->GetKind() == kTypePointer) {
      mirType = static_cast<MIRPtrType*>(mirType)->GetPointedType();
    }
    if (!mirType->IsMIRJarrayType()) {
      return;
    }
    GenericType *realType = graphTempMem->New<GenericType>(mirType->GetNameStrIdx(), mirType,
                                                           graphTempAllocator);
    GenericNode *realNode = GetOrCreateGenericNode(realType);
    GenericNode *declareNode = GetOrCreateGenericNode(extendG);
    AddNextNode(*declareNode, *realNode);
    mirType = static_cast<MIRJarrayType*>(mirType)->GetElemType();
    if (mirType->GetKind() == kTypePointer) {
      mirType = static_cast<MIRPtrType*>(mirType)->GetPointedType();
    }
    GenericType *subRealType = graphTempMem->New<GenericType>(mirType->GetNameStrIdx(), mirType,
                                                              graphTempAllocator);
    GenericNode *subRealNode = GetOrCreateGenericNode(subRealType);
    GenericNode *subDeclareNode = GetOrCreateGenericNode(extendG->GetContainsGeneric());
    AddNextNode(*subDeclareNode, *subRealNode);
  }
}

void CheckCast::TryToResolveFuncGeneric(MIRFunction &callee, const CallMeStmt &callMeStmt, size_t thisIdx) {
  size_t argStartPos = thisIdx;
  if (!callee.IsStatic()) {
    argStartPos += 1;
  }
  MapleVector<AnnotationType*> &argVec = callee.GetFuncGenericArg();
  CHECK_FATAL(callMeStmt.NumMeStmtOpnds() == argVec.size() + argStartPos, "must be");
  for (size_t i = 0; i < argVec.size(); ++i) {
    AnnotationType *at = argVec[i];
    MeExpr *expr = callMeStmt.GetOpnd(i + argStartPos);
    TryToResolveFuncArg(*expr, *at);
  }
}

void CheckCast::TryToResolveCall(MeStmt &meStmt) {
  CallMeStmt &callMeStmt = static_cast<CallMeStmt&>(meStmt);
  MIRFunction &callee = callMeStmt.GetTargetFunction();
  size_t thisIdx = 0;
  if (callMeStmt.GetOp() == OP_interfaceicallassigned || callMeStmt.GetOp() == OP_virtualicallassigned) {
    CHECK_FATAL(callee.GetParamSize() + 1 == callMeStmt.GetOpnds().size(), "must be");
    thisIdx = 1;
  } else if (callMeStmt.GetOp() == OP_callassigned) {
    thisIdx = 0;
  } else {
    CHECK_FATAL(false, "must be");
  }
  if (!callee.GetFuncGenericDeclare().empty()) {
    TryToResolveFuncGeneric(callee, callMeStmt, thisIdx);
  }
  if (callee.GetClassTyIdx() == 0 || callee.IsStatic()) {
    return;
  }
  MIRStructType *structType = static_cast<MIRStructType*>(
      GlobalTables::GetTypeTable().GetTypeFromTyIdx(callee.GetClassTyIdx()));
  if (structType->GetGenericDeclare().empty()) {
    return;
  }
  switch (callMeStmt.GetOpnd(thisIdx)->GetMeOp()) {
    case kMeOpVar: {
      (void)TryToResolveVar(*static_cast<VarMeExpr*>(callMeStmt.GetOpnd(thisIdx)), structType);
      break;
    }
    case kMeOpIvar: {
      TryToResolveIvar(*static_cast<IvarMeExpr*>(callMeStmt.GetOpnd(thisIdx)), structType);
      break;
    }
    default:
      break;
  }
  BuildGenericGraph(callee.GetFuncGenericRet());
  return;
}

void CheckCast::TryToResolveIvar(IvarMeExpr &ivar, MIRStructType *callStruct) {
  if (ivar.GetBase()->GetMeOp() == kMeOpIvar) {
    TryToResolveIvar(*static_cast<IvarMeExpr*>(ivar.GetBase()));
  } else if (ivar.GetBase()->GetMeOp() != kMeOpVar) {
    return;
  }
  MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ivar.GetTyIdx());
  if (type->GetKind() == kTypePointer) {
    type = static_cast<MIRPtrType*>(type)->GetPointedType();
  }
  if (!type->IsStructType()) {
    return;
  }
  MIRStructType *structType = static_cast<MIRStructType*>(type);
  GStrIdx gStrIdx = structType->GetFieldGStrIdx(ivar.GetFieldID());
  MIRType *ivarType = structType->GetFieldType(ivar.GetFieldID());
  if (ivarType->GetKind() == kTypePointer) {
    ivarType = static_cast<MIRPtrType*>(ivarType)->GetPointedType();
  }
  CHECK_FATAL(ivarType->IsStructType(), "must be");
  AnnotationType *annotationType = nullptr;
  MIRStructType *tmpStructType = structType;
  do {
    annotationType = tmpStructType->GetFieldGenericDeclare(gStrIdx);
    Klass *super = klassh->GetKlassFromTyIdx(tmpStructType->GetTypeIndex())->GetSuperKlass();
    if (super != nullptr) {
      tmpStructType = super->GetMIRStructType();
    } else {
      tmpStructType = nullptr;
    }
  } while (annotationType == nullptr && tmpStructType != nullptr);

  if (NeedChangeVarType(static_cast<MIRStructType*>(ivarType), callStruct)) {
    ivarType = callStruct;
    annotationType = CloneNewAnnotationType(annotationType, callStruct);
  }
  AddClassInheritanceInfo(*ivarType);
  if (annotationType == nullptr) {
    return;
  }
  if (annotationType->GetKind() == kGenericType) {
    BuildGenericGraph(annotationType);
    return;
  }
}

bool CheckCast::TryToResolveStaticVar(const VarMeExpr &var) {
  const OriginalSt *symOst = func->GetIRMap()->GetSSATab().GetOriginalStFromID(var.GetOstIdx());
  const MIRSymbol *sym = symOst->GetMIRSymbol();
  if (!sym->IsStatic()) {
    return false;
  }
  std::string varTotalName = sym->GetName();
  size_t posOfClass = varTotalName.find("_7C");
  std::string className = varTotalName.substr(0, posOfClass);
  std::string varName = varTotalName.substr(posOfClass + strlen(namemangler::kNameSplitterStr));
  MIRStructType *structType = GetClassTypeFromName(className);
  GStrIdx gStrIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(varName);
  AnnotationType *aType = structType->GetFieldGenericDeclare(gStrIdx);
  if (aType == nullptr) {
    return true;
  }
  if (aType->GetKind() == kGenericType) {
    BuildGenericGraph(aType);
  } else {
    CHECK_FATAL(false, "must be");
  }
  return true;
}

void CheckCast::TryToResolveDassign(MeStmt &meStmt) {
  CHECK_FATAL(meStmt.GetOp() == OP_dassign, "must be");
  DassignMeStmt &dassignMeStmt = static_cast<DassignMeStmt&>(meStmt);
  if (dassignMeStmt.GetRHS()->GetMeOp() == kMeOpIvar) {
    IvarMeExpr *ivar = static_cast<IvarMeExpr*>(dassignMeStmt.GetRHS());
    TryToResolveIvar(*ivar);
  } else if (dassignMeStmt.GetRHS()->GetMeOp() == kMeOpVar) {
    bool tmpResult = TryToResolveStaticVar(*static_cast<VarMeExpr*>(dassignMeStmt.GetRHS()));
    if (!tmpResult) {
      (void)TryToResolveVar(static_cast<VarMeExpr&>(*dassignMeStmt.GetRHS()));
    }
  }
}

MIRType *CheckCast::TryToResolveType(GenericNode &retNode) {
  (void)visited.insert(&retNode);
  if (retNode.next == nullptr || visited.find(retNode.next) != visited.end()) {
    AnnotationType *at = retNode.aType;
    if (at->GetKind() == kGenericType) {
      return static_cast<GenericType*>(at)->GetMIRType();
    } else if (at->GetKind() == kGenericDeclare) {
      AnnotationType *defaultType = static_cast<GenericDeclare*>(at)->GetDefaultType();
      while (defaultType->GetKind() == kGenericDeclare) {
        defaultType = static_cast<GenericDeclare*>(defaultType)->GetDefaultType();
      }
      if (defaultType->GetKind() == kGenericType) {
        return static_cast<GenericType*>(defaultType)->GetMIRStructType();
      }
      CHECK_FATAL(false, "must be");
    } else {
      return nullptr;
    }
  }
  return TryToResolveType(*retNode.next);
}

void CheckCast::AddClassInheritanceInfo(MIRType &mirType) {
  if (mirType.GetKind() == kTypePointer) {
    mirType = *static_cast<MIRPtrType&>(mirType).GetPointedType();
  }
  if (!mirType.IsStructType()) {
    return;
  }
  MIRStructType *structType = static_cast<MIRStructType*>(&mirType);
  for (GenericType *gt : structType->GetInheritanceGeneric()) {
    BuildGenericGraph(gt);
    AddClassInheritanceInfo(*gt->GetMIRStructType());
  }
}

// varStruct is parent, callStruct is child
bool CheckCast::ExactlyMatch(MIRStructType &varStruct, MIRStructType &callStruct) {
  if (varStruct.GetGenericDeclare().size() == 0 || callStruct.GetGenericDeclare().size() == 0) {
    return false;
  }
  if (varStruct.GetGenericDeclare().size() != callStruct.GetGenericDeclare().size()) {
    return false;
  }
  GenericType *gTmp = nullptr;
  for (GenericType *gt : callStruct.GetInheritanceGeneric()) {
    if (gt->GetMIRStructType() == &varStruct) {
      gTmp = gt;
      break;
    }
  }
  if (gTmp == nullptr) {
    return false;
  }

  for (size_t i = 0; i < gTmp->GetGenericArg().size(); ++i) {
    if (gTmp->GetGenericArg()[i] != callStruct.GetGenericDeclare()[i]) {
      return false;
    }
  }
  return true;
}

bool CheckCast::NeedChangeVarType(MIRStructType *varStruct, MIRStructType *callStruct) {
  if (callStruct == nullptr) {
    return false;
  }
  if (varStruct == callStruct) {
    return false;
  }
  Klass *varKlass = klassh->GetKlassFromTyIdx(varStruct->GetTypeIndex());
  Klass *callKlass = klassh->GetKlassFromTyIdx(callStruct->GetTypeIndex());
  if (klassh->IsSuperKlass(varKlass, callKlass) ||
      klassh->IsSuperKlassForInterface(varKlass, callKlass) || klassh->IsInterfaceImplemented(varKlass, callKlass)) {
    if (ExactlyMatch(*varStruct, *callStruct)) {
      return true;
    }
  }
  return false;
}

AnnotationType *CheckCast::CloneNewAnnotationType(AnnotationType *at, MIRStructType *callStruct) {
  if (at == nullptr || at->GetKind() != kGenericType) {
    return at;
  }
  GenericType *newGT = graphTempMem->New<GenericType>(callStruct->GetNameStrIdx(), callStruct, graphTempAllocator);
  MapleVector<AnnotationType*> &argVec = static_cast<GenericType*>(at)->GetGenericArg();
  for (size_t i = 0; i < argVec.size(); i++) {
    newGT->AddGenericPair(callStruct->GetGenericDeclare()[i], argVec[i]);
  }
  return newGT;
}

bool CheckCast::RetIsGenericRelative(MIRFunction &callee) {
  if (callee.GetFuncGenericRet() == nullptr) {
    return false;
  }
  AnnotationType *ret = callee.GetFuncGenericRet();
  if (ret->GetKind() == kGenericDeclare) {
    return true;
  }
  if (ret->GetKind() == kExtendType) {
    ExtendGeneric *extendRet = static_cast<ExtendGeneric*>(ret);
    if (extendRet->GetExtendInfo() != kArrayType && extendRet->GetExtendInfo() != kHierarchyExtend) {
      return false;
    }
    if (extendRet->GetContainsGeneric()->GetKind() == kGenericDeclare) {
      return true;
    }
  }
  return false;
}

bool CheckCast::TryToResolveVar(VarMeExpr &var, MIRStructType *callStruct, bool checkFirst) {
  const OriginalSt *symOst = func->GetIRMap()->GetSSATab().GetOriginalStFromID(var.GetOstIdx());
  const MIRSymbol *sym = symOst->GetMIRSymbol();
  AnnotationType *at = func->GetMirFunc()->GetFuncLocalGenericVar(sym->GetNameStrIdx());

  MIRType *varType = sym->GetType();
  if (at != nullptr && at->GetKind() == kGenericType) {
    varType = static_cast<GenericType*>(at)->GetMIRStructType();
  }
  if (varType->GetKind() == kTypePointer) {
    varType = static_cast<MIRPtrType*>(varType)->GetPointedType();
  }
  if (varType->IsStructType() && NeedChangeVarType(static_cast<MIRStructType*>(varType), callStruct)) {
    varType = callStruct;
    at = CloneNewAnnotationType(at, callStruct);
  }
  if (at != nullptr && at->GetKind() == kGenericType) {
    BuildGenericGraph(at);
  }
  AddClassInheritanceInfo(*varType);

  MeDefBy defBy = var.GetDefBy();
  switch (defBy) {
    case kDefByStmt: {
      if (checkFirst) {
        break;
      }
      CHECK_FATAL(var.GetDefStmt()->GetOp() == OP_dassign, "must be");
      TryToResolveDassign(*var.GetDefStmt());
      break;
    }
    case kDefByPhi:
      break;
    case kDefByChi: {
      if (checkFirst) {
        break;
      }
      (void)TryToResolveStaticVar(var);
      break;
    }
    case kDefByMustDef: {
      CallMeStmt *callMeStmt = safe_cast<CallMeStmt>(var.GetDefMustDef().GetBase());
      if (callMeStmt == nullptr) {
        break;
      }
      MIRFunction &callee = callMeStmt->GetTargetFunction();
      if (checkFirst) {
        if (!RetIsGenericRelative(callee)) {
          break;
        }
      }
      TryToResolveCall(*var.GetDefMustDef().GetBase());
      if (checkFirst) {
        GenericNode *retNode = GetOrCreateGenericNode(callee.GetFuncGenericRet());
        MIRType *resultType = TryToResolveType(*retNode);
        if (curCheckCastType == resultType) {
          return true;
        }
      }
      break;
    }
    default:
      break;
  }
  return false;
}

bool CheckCast::ProvedByAnnotationInfo(const IntrinsiccallMeStmt &callNode) {
  MeExpr *expr = callNode.GetOpnd(0);
  bool result = false;
  switch (expr->GetMeOp()) {
    case kMeOpVar: {
      VarMeExpr *var = static_cast<VarMeExpr*>(expr);
      result = TryToResolveVar(*var, nullptr, true);
      break;
    }
    case kMeOpConst:
      CHECK_FATAL(static_cast<ConstMeExpr*>(expr)->IsZero(), "must be");
      break;
    case kMeOpIvar: {
      break;
    }
    default:
      break;
  }
  created.clear();
  visited.clear();
  return result;
}

void CheckCast::RemoveRedundantCheckCast(MeStmt &stmt, BB &bb) {
  if (stmt.GetOp() == OP_intrinsiccallwithtypeassigned) {
    auto *callAssign = static_cast<IntrinsiccallMeStmt*>(&stmt);
    ScalarMeExpr *lhs = callAssign->GetAssignedLHS();
    AssignMeStmt *newDass = func->GetIRMap()->CreateAssignMeStmt(*lhs, *(callAssign->GetOpnd(0)), bb);
    newDass->SetSrcPos(stmt.GetSrcPosition());
    lhs->SetDefByStmt(*newDass);
    bb.InsertMeStmtBefore(&stmt, newDass);
  }
  bb.RemoveMeStmt(&stmt);
}

ProveRes CheckCast::TraverseBackProve(MeExpr &expr, MIRType &targetType, std::set<MePhiNode*> &visitedPhi) {
  if (expr.GetMeOp() == kMeOpConst) {
    CHECK_FATAL(static_cast<ConstMeExpr*>(&expr)->IsZero(), "must be");
    return ProveRes::kT;
  }
  if (expr.GetMeOp() == kMeOpIvar) {
    return TraverseBackProve(*(static_cast<IvarMeExpr&>(expr).GetMu()), targetType, visitedPhi);
  }
  if (expr.GetMeOp() != kMeOpVar) {
    return ProveRes::kF;
  }
  if (meSSI->GetInferredType(&expr) == &targetType) {
    return ProveRes::kT;
  }
  VarMeExpr *var = static_cast<VarMeExpr*>(&expr);
  switch (var->GetDefBy()) {
    case kDefByStmt: {
      MeStmt *meStmt = var->GetDefStmt();
      if (meStmt->IsAssign()) {
        return TraverseBackProve(*(meStmt->GetRHS()), targetType, visitedPhi);
      }
      break;
    }
    case kDefByPhi: {
      MePhiNode &phi = var->GetDefPhi();
      if (visitedPhi.find(&phi) == visitedPhi.end()) {
        return ProveRes::kIgnore;
      }
      visitedPhi.insert(&phi);
      ProveRes res = ProveRes::kF;
      for (auto *scalar : phi.GetOpnds()) {
        ProveRes tmp = TraverseBackProve(*scalar, targetType, visitedPhi);
        if (tmp == ProveRes::kF) {
          return ProveRes::kF;
        } else if (tmp == ProveRes::kT) {
          res = ProveRes::kT;
        }
      }
      return res;
      break;
    }
    default:
    break;
  }
  return ProveRes::kF;
}

bool CheckCast::ProvedBySSI(const IntrinsiccallMeStmt &callNode) {
  std::set<MePhiNode*> visitedPhi;
  MeExpr *expr = callNode.GetOpnd(0);
  MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(callNode.GetTyIdx());
  return TraverseBackProve(*expr, *mirType, visitedPhi) == ProveRes::kT;
}

void CheckCast::DoCheckCastOpt() {
  auto cfg = func->GetCfg();
  for (BB *bb : cfg->GetAllBBs()) {
    if (bb == nullptr) {
      continue;
    }
    MeStmt *nextStmt = nullptr;
    auto &meStmts = bb->GetMeStmts();
    MeStmt *stmt = to_ptr(meStmts.begin());
    for (; stmt != nullptr; stmt = nextStmt) {
      nextStmt = stmt->GetNext();
      if (stmt->GetOp() != OP_intrinsiccallwithtypeassigned && stmt->GetOp() != OP_intrinsiccallwithtype) {
        continue;
      }
      auto *callNode = safe_cast<IntrinsiccallMeStmt>(stmt);
      if (callNode == nullptr || callNode->GetIntrinsic() != INTRN_JAVA_CHECK_CAST) {
        continue;
      }
      MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(callNode->GetTyIdx());
      if (mirType->GetKind() == kTypePointer) {
        mirType = static_cast<MIRPtrType*>(mirType)->GetPointedType();
      }
      if (!mirType->IsInstanceOfMIRStructType()) {
        if (!mirType->IsMIRJarrayType()) {
          continue;
        }
        MIRType *tmpType = static_cast<MIRJarrayType*>(mirType)->GetElemType();
        if (tmpType->GetKind() == kTypePointer) {
          tmpType = static_cast<MIRPtrType*>(tmpType)->GetPointedType();
        }
        if (!tmpType->IsInstanceOfMIRStructType()) {
          continue;
        }
      }
      curCheckCastType = mirType;

      if (ProvedByAnnotationInfo(*callNode)) {
        RemoveRedundantCheckCast(*stmt, *bb);
        continue;
      }
    }
  }
  ReleaseMemory();
}

void CheckCast::FindRedundantChecks() {
  auto cfg = func->GetCfg();
  for (BB *bb : cfg->GetAllBBs()) {
    if (bb == nullptr) {
      continue;
    }
    MeStmt *nextStmt = nullptr;
    auto &meStmts = bb->GetMeStmts();
    MeStmt *stmt = to_ptr(meStmts.begin());
    for (; stmt != nullptr; stmt = nextStmt) {
      nextStmt = stmt->GetNext();
      if (stmt->GetOp() != OP_intrinsiccallwithtypeassigned && stmt->GetOp() != OP_intrinsiccallwithtype) {
        continue;
      }
      auto *callNode = safe_cast<IntrinsiccallMeStmt>(stmt);
      if (callNode == nullptr || callNode->GetIntrinsic() != INTRN_JAVA_CHECK_CAST) {
        continue;
      }
      if (ProvedBySSI(*callNode)) {
        redundantChecks.push_back(stmt);
      }
    }
  }
}

void CheckCast::DeleteRedundants() {
  for (MeStmt *meStmt : redundantChecks) {
    RemoveRedundantCheckCast(*meStmt, *(meStmt->GetBB()));
  }
}

void MECheckCastOpt::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEIRMapBuild>();
  aDep.AddRequired<MEDominance>();
  aDep.SetPreservedAll();
}

bool MECheckCastOpt::PhaseRun(maple::MeFunction &f) {
  auto *dom = GET_ANALYSIS(MEDominance, f);
  MaplePhase *it = GetAnalysisInfoHook()->GetOverIRAnalyisData<MeFuncPM, M2MKlassHierarchy,
                                                               MIRModule>(f.GetMIRModule());
  auto *kh = static_cast<M2MKlassHierarchy*>(it)->GetResult();
  ASSERT_NOT_NULL(dom);

  MeSSI meSSI(f, *dom, *f.GetIRMap(), *GetPhaseMemPool());
  CheckCast checkCast(f, *kh, meSSI);
  meSSI.SetSSIType(kCheckCastOpt);
  meSSI.ConvertToSSI();
  checkCast.FindRedundantChecks();
  meSSI.ConvertToSSA();
  checkCast.DeleteRedundants();
  if (MeOption::checkCastOpt) {
    checkCast.DoCheckCastOpt();
  }
  return true;
}
}  // namespace maple
