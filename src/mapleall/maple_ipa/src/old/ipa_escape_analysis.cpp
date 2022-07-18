/*
 * Copyright (c) [2019-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "ipa_escape_analysis.h"
#include "me_cfg.h"
#include <algorithm>

namespace maple {
constexpr maple::uint32 kInvalid = 0xffffffff;
static bool IsExprRefOrPtr(const MeExpr &expr) {
  return expr.GetPrimType() == PTY_ref || expr.GetPrimType() == PTY_ptr;
}

static bool IsTypeRefOrPtr(PrimType type) {
  return type == PTY_ref || type == PTY_ptr;
}

static bool IsGlobal(const SSATab &ssaTab, const VarMeExpr &expr) {
  const OriginalSt *symOst = ssaTab.GetOriginalStFromID(expr.GetOstIdx());
  ASSERT(symOst != nullptr, "null ptr check");
  if (symOst->GetMIRSymbol()->GetStIdx().IsGlobal()) {
    return true;
  }
  return false;
}

static bool IsGlobal(const SSATab &ssaTab, const AddrofMeExpr &expr) {
  const OriginalSt *symOst = ssaTab.GetOriginalStFromID(expr.GetOstIdx());
  ASSERT(symOst != nullptr, "null ptr check");
  if (symOst->GetMIRSymbol()->GetStIdx().IsGlobal()) {
    return true;
  }
  return false;
}

static bool IsZeroConst(const VarMeExpr *expr) {
  if (expr == nullptr) {
    return false;
  }
  if (expr->GetDefBy() != kDefByStmt) {
    return false;
  }
  MeStmt *stmt = expr->GetDefStmt();
  if (stmt->GetOp() != OP_dassign) {
    return false;
  }
  DassignMeStmt *dasgn = static_cast<DassignMeStmt*>(stmt);
  if (dasgn->GetRHS()->GetMeOp() != kMeOpConst) {
    return false;
  }
  ConstMeExpr *constExpr = static_cast<ConstMeExpr*>(dasgn->GetRHS());
  if (constExpr->GetConstVal()->GetKind() == kConstInt && constExpr->GetConstVal()->IsZero()) {
    return true;
  }
  return false;
}

static bool StartWith(const std::string &str, const std::string &head) {
  return str.compare(0, head.size(), head) == 0;
}

static bool IsVirtualVar(const SSATab &ssaTab, const VarMeExpr &expr) {
  const OriginalSt *ost = ssaTab.GetOriginalStFromID(expr.GetOstIdx());
  ASSERT(ost != nullptr, "null ptr check");
  return ost->GetIndirectLev() > 0;
}

static bool IsInWhiteList(const MIRFunction &func) {
  std::vector<std::string> whiteList = {
      "MCC_Reflect_Check_Casting_Array",
      "MCC_Reflect_Check_Casting_NoArray",
      "MCC_ThrowStringIndexOutOfBoundsException",
      "MCC_ArrayMap_String_Int_clear",
      "MCC_ArrayMap_String_Int_put",
      "MCC_ArrayMap_String_Int_getOrDefault",
      "MCC_ArrayMap_String_Int_size",
      "MCC_ThrowSecurityException",
      "MCC_String_Equals_NotallCompress",
      "memcmpMpl",
      "Native_java_lang_String_compareTo__Ljava_lang_String_2",
      "Native_java_lang_String_getCharsNoCheck__II_3CI",
      "Native_java_lang_String_toCharArray__",
      "Native_java_lang_System_arraycopyBooleanUnchecked___3ZI_3ZII",
      "Native_java_lang_System_arraycopyByteUnchecked___3BI_3BII",
      "Native_java_lang_System_arraycopyCharUnchecked___3CI_3CII",
      "Native_java_lang_System_arraycopyDoubleUnchecked___3DI_3DII",
      "Native_java_lang_System_arraycopyFloatUnchecked___3FI_3FII",
      "Native_java_lang_System_arraycopyIntUnchecked___3II_3III",
      "Native_java_lang_System_arraycopy__Ljava_lang_Object_2ILjava_lang_Object_2II",
      "Native_java_lang_System_arraycopyLongUnchecked___3JI_3JII",
      "Native_java_lang_System_arraycopyShortUnchecked___3SI_3SII",
      "getpriority",
      "setpriority"
  };
  for (std::string name : whiteList) {
    if (func.GetName() == name) {
      // close all the whitelist
      return false;
    }
  }
  return false;
}

static bool IsNoSideEffect(CallMeStmt &call) {
  CallMeStmt &callAssign = utils::ToRef(&call);
  MIRFunction &mirFunc = callAssign.GetTargetFunction();
  if (IsInWhiteList(mirFunc)) {
    return true;
  }
  // Non-nullptr means it has return value
  CHECK_FATAL(callAssign.GetMustDefList() != nullptr, "Impossible");
  if (callAssign.GetMustDefList()->size() == 1) {
    if (callAssign.GetMustDefListItem(0).GetLHS()->GetMeOp() != kMeOpVar &&
        callAssign.GetMustDefListItem(0).GetLHS()->GetMeOp() != kMeOpReg) {
      CHECK_FATAL(false, "NYI");
    }
    if (IsExprRefOrPtr(*callAssign.GetMustDefListItem(0).GetLHS())) {
      return false;
    }
  }

  const MapleVector<MeExpr*> &opnds = call.GetOpnds();
  const size_t size = opnds.size();
  for (size_t i = 0; i < size; ++i) {
    if (IsExprRefOrPtr(*opnds[i])) {
      return false;
    }
  }
  return true;
}

static bool IsRegAssignStmtForClassMeta(const AssignMeStmt &regAssign) {
  MeExpr &rhs = utils::ToRef(regAssign.GetRHS());
  if (rhs.GetOp() == OP_add) {
    return true;
  }

  if (instance_of<IvarMeExpr>(rhs)) {
    IvarMeExpr &ivar = static_cast<IvarMeExpr&>(rhs);
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ivar.GetTyIdx());
    MIRPtrType &ptrType = utils::ToRef(safe_cast<MIRPtrType>(type));
    if (ptrType.GetPointedType()->GetName() == "__class_meta__" ||
        (ptrType.GetPointedType()->GetName() == "Ljava_2Flang_2FObject_3B" && ivar.GetFieldID() == 1)) {
      return true;
    }
  }
  return false;
}

TyIdx IPAEscapeAnalysis::GetAggElemType(const MIRType &aggregate) const {
  switch (aggregate.GetKind()) {
    case kTypePointer: {
      const MIRPtrType *pointType = static_cast<const MIRPtrType*>(&aggregate);
      const MIRType *pointedType = pointType->GetPointedType();
      switch (pointedType->GetKind()) {
        case kTypeClass:
        case kTypeScalar:
          return pointedType->GetTypeIndex();
        case kTypePointer:
        case kTypeJArray:
          return GetAggElemType(*pointedType);
        default:
          return TyIdx(0);
      }
    }
    case kTypeJArray: {
      const MIRJarrayType *arrType = static_cast<const MIRJarrayType*>(&aggregate);
      const MIRType *elemType = arrType->GetElemType();
      CHECK_NULL_FATAL(elemType);
      switch (elemType->GetKind()) {
        case kTypeScalar:
          return elemType->GetTypeIndex();
        case kTypePointer:
        case kTypeJArray:
          return GetAggElemType(*elemType);
        default:  // Not sure what type is
          return TyIdx(0);
      }
    }
    default:
      CHECK_FATAL(false, "Should not reach here");
      return TyIdx(0);  // to eliminate compilation warning
  }
}

// check whether the newly allocated object implements Runnable, Throwable, extends Reference or has a finalizer
bool IPAEscapeAnalysis::IsSpecialEscapedObj(const MeExpr &alloc) const {
  if (alloc.GetOp() == OP_gcpermalloc || alloc.GetOp() == OP_gcpermallocjarray) {
    return true;
  }
  TyIdx tyIdx;
  const static TyIdx runnableInterface = kh->GetKlassFromLiteral("Ljava_2Flang_2FRunnable_3B")->GetTypeIdx();
  if (alloc.GetOp() == OP_gcmalloc) {
    tyIdx = static_cast<const GcmallocMeExpr*>(&alloc)->GetTyIdx();
  } else {
    CHECK_FATAL(alloc.GetOp() == OP_gcmallocjarray, "must be OP_gcmallocjarray");
    MIRType *arrType =
        GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<const OpMeExpr*>(&alloc)->GetTyIdx());
    tyIdx = GetAggElemType(*arrType);
    if (tyIdx == TyIdx(0)) {
      return true;  // deal as escape
    }
  }
  MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
  if (type->GetKind() == kTypeScalar) {
    return false;
  }
  Klass *klass = kh->GetKlassFromTyIdx(tyIdx);
  CHECK_FATAL(klass, "impossible");
  for (Klass *inter : klass->GetImplInterfaces()) {
    if (inter->GetTypeIdx() == runnableInterface) {
      return true;
    }
  }
  if (klass->HasFinalizer() || klass->IsExceptionKlass()) {
    return true;
  }

  // check subclass of Reference class, such as WeakReference, PhantomReference, SoftReference and Cleaner
  const static Klass *referenceKlass = kh->GetKlassFromLiteral("Ljava_2Flang_2Fref_2FReference_3B");
  if (kh->IsSuperKlass(referenceKlass, klass)) {
    return true;
  }
  return false;
}

EACGRefNode *IPAEscapeAnalysis::GetOrCreateCGRefNodeForReg(RegMeExpr &reg, bool createObjNode) {
  EACGBaseNode *node = eaCG->GetCGNodeFromExpr(&reg);
  EACGRefNode *refNode = nullptr;
  if (node == nullptr) {
    refNode = eaCG->CreateReferenceNode(&reg, kNoEscape, false);
    cgChangedInSCC = true;
  } else {
    refNode = static_cast<EACGRefNode*>(node);
  }
  if (node == nullptr && createObjNode) {
    EACGObjectNode *objNode = GetOrCreateCGObjNode(nullptr, nullptr, refNode->GetEAStatus());
    (void)refNode->AddOutNode(*objNode);
  }
  return refNode;
}

EACGRefNode *IPAEscapeAnalysis::GetOrCreateCGRefNodeForAddrof(AddrofMeExpr &var, bool createObjNode) {
  if (IsGlobal(*ssaTab, var)) {
    eaCG->UpdateExprOfGlobalRef(&var);
    return eaCG->GetGlobalReference();
  }
  EACGBaseNode *node = eaCG->GetCGNodeFromExpr(&var);
  EACGRefNode *refNode = nullptr;
  if (node == nullptr) {
    refNode = eaCG->CreateReferenceNode(&var, kNoEscape, false);
    cgChangedInSCC = true;
  } else {
    refNode = static_cast<EACGRefNode*>(node);
  }
  if (node == nullptr && createObjNode) {
    EACGObjectNode *objNode = GetOrCreateCGObjNode(nullptr, nullptr, refNode->GetEAStatus());
    (void)refNode->AddOutNode(*objNode);
  }
  return refNode;
}

EACGRefNode *IPAEscapeAnalysis::GetOrCreateCGRefNodeForVar(VarMeExpr &var, bool createObjNode) {
  if (IsGlobal(*ssaTab, var)) {
    eaCG->UpdateExprOfGlobalRef(&var);
    return eaCG->GetGlobalReference();
  }
  EACGBaseNode *node = eaCG->GetCGNodeFromExpr(&var);
  EACGRefNode *refNode = nullptr;
  if (node == nullptr) {
    refNode = eaCG->CreateReferenceNode(&var, kNoEscape, false);
    cgChangedInSCC = true;
  } else {
    refNode = static_cast<EACGRefNode*>(node);
  }
  if (node == nullptr && createObjNode) {
    EACGObjectNode *objNode = GetOrCreateCGObjNode(nullptr, nullptr, refNode->GetEAStatus());
    (void)refNode->AddOutNode(*objNode);
  }
  return refNode;
}

EACGRefNode *IPAEscapeAnalysis::GetOrCreateCGRefNodeForVarOrReg(MeExpr &var, bool createObjNode) {
  if (var.GetMeOp() == kMeOpVar) {
    return GetOrCreateCGRefNodeForVar(static_cast<VarMeExpr&>(var), createObjNode);
  } else if (var.GetMeOp() == kMeOpReg) {
    return GetOrCreateCGRefNodeForReg(static_cast<RegMeExpr&>(var), createObjNode);
  }
  CHECK_FATAL(false, "Impossible");
  return nullptr;
}

static FieldID GetBaseFieldId(const KlassHierarchy &kh, const TyIdx &tyIdx, FieldID fieldId) {
  FieldID ret = fieldId;
  Klass *klass = kh.GetKlassFromTyIdx(tyIdx);
  CHECK_FATAL(klass != nullptr, "Impossible");
  Klass *super = klass->GetSuperKlass();
  if (super == nullptr) {
    return ret;
  }
  MIRStructType *structType = super->GetMIRStructType();
  TyIdx typeIdx = structType->GetFieldTyIdx(fieldId - 1);
  while (typeIdx != 0u) {
    --ret;
    klass = super;
    super = klass->GetSuperKlass();
    if (super == nullptr) {
      return ret;
    }
    structType = super->GetMIRStructType();
    typeIdx = structType->GetFieldTyIdx(ret - 1);
  }
  return ret;
}

void IPAEscapeAnalysis::GetOrCreateCGFieldNodeForIvar(std::vector<EACGBaseNode*> &fieldNodes, IvarMeExpr &ivar,
                                                      MeStmt &stmt, bool createObjNode) {
  MeExpr *base = ivar.GetBase();
  FieldID fieldId = ivar.GetFieldID();
  std::vector<EACGBaseNode*> baseNodes;
  if (base->GetMeOp() == kMeOpReg && fieldId == 0) {
    GetArrayBaseNodeForReg(baseNodes, static_cast<RegMeExpr&>(*base), stmt);
  } else {
    GetCGNodeForMeExpr(baseNodes, *base, stmt, true);
  }
  bool ifHandled = (eaCG->GetCGNodeFromExpr(&ivar) != nullptr);
  if (ivar.GetFieldID() != 0) {
    MIRPtrType *ptrType = static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(ivar.GetTyIdx()));
    TyIdx tyIdx = ptrType->GetPointedTyIdx();
    fieldId = GetBaseFieldId(*kh, tyIdx, ivar.GetFieldID());
  }
  for (const auto &baseNode : baseNodes) {
    for (const auto &objNode : baseNode->GetPointsToSet()) {
      EACGFieldNode *fieldNode = objNode->GetFieldNodeFromIdx(fieldId);
      if (!ifHandled && fieldNode != nullptr) {
        eaCG->UpdateExprOfNode(*fieldNode, &ivar);
      } else if (!ifHandled && fieldNode == nullptr) {
        fieldNode = eaCG->CreateFieldNode(&ivar, objNode->GetEAStatus(), fieldId, objNode, false);
        cgChangedInSCC = true;
        if (createObjNode) {
          EACGObjectNode *phanObjNode = GetOrCreateCGObjNode(nullptr);
          (void)fieldNode->AddOutNode(*phanObjNode);
        }
      }
      if (fieldNode != nullptr) {
        fieldNodes.push_back(fieldNode);
      }
    }
  }
}

void IPAEscapeAnalysis::GetOrCreateCGFieldNodeForIAddrof(std::vector<EACGBaseNode*> &fieldNodes, OpMeExpr &expr,
                                                         MeStmt &stmt, bool createObjNode) {
  MeExpr *base = expr.GetOpnd(0);
  FieldID fieldId = expr.GetFieldID();
  std::vector<EACGBaseNode*> baseNodes;
  if (base->GetMeOp() == kMeOpReg && fieldId == 0) {
    GetArrayBaseNodeForReg(baseNodes, static_cast<RegMeExpr&>(*base), stmt);
  } else {
    GetCGNodeForMeExpr(baseNodes, *base, stmt, true);
  }
  bool ifHandled = (eaCG->GetCGNodeFromExpr(&expr) != nullptr);
  if (expr.GetFieldID() != 0) {
    MIRPtrType *ptrType = static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(expr.GetTyIdx()));
    TyIdx tyIdx = ptrType->GetPointedTyIdx();
    fieldId = GetBaseFieldId(*kh, tyIdx, expr.GetFieldID());
  }
  for (const auto &baseNode : baseNodes) {
    for (const auto &objNode : baseNode->GetPointsToSet()) {
      EACGFieldNode *fieldNode = objNode->GetFieldNodeFromIdx(fieldId);
      if (!ifHandled && fieldNode != nullptr) {
        eaCG->UpdateExprOfNode(*fieldNode, &expr);
      } else if (!ifHandled && fieldNode == nullptr) {
        fieldNode = eaCG->CreateFieldNode(&expr, objNode->GetEAStatus(), fieldId, objNode, false);
        cgChangedInSCC = true;
        if (createObjNode) {
          EACGObjectNode *phanObjNode = GetOrCreateCGObjNode(nullptr);
          (void)fieldNode->AddOutNode(*phanObjNode);
        }
      }
      if (fieldNode != nullptr) {
        fieldNodes.push_back(fieldNode);
      }
    }
  }
}

EACGObjectNode *IPAEscapeAnalysis::GetOrCreateCGObjNode(MeExpr *expr, const MeStmt *stmt, EAStatus easOfPhanObj) {
  EAStatus eas = kNoEscape;
  TyIdx tyIdx;
  Location *location = nullptr;
  bool isPhantom;
  if (expr != nullptr) {
    EACGBaseNode *cgNode = eaCG->GetCGNodeFromExpr(expr);
    if (cgNode != nullptr) {
      CHECK_FATAL(cgNode->IsObjectNode(), "should be object");
      EACGObjectNode *objNode = static_cast<EACGObjectNode*>(cgNode);
      return objNode;
    }
    if (expr->IsGcmalloc()) {
      CHECK_FATAL(stmt != nullptr, "Impossible");
      location = mirModule->GetMemPool()->New<Location>(mirModule->GetFileName(), stmt->GetSrcPosition().FileNum(),
                                                        stmt->GetSrcPosition().LineNum());
      isPhantom = false;
      if (IsSpecialEscapedObj(*expr)) {
        eas = kGlobalEscape;
      }
      if (expr->GetOp() == OP_gcmalloc || expr->GetOp() == OP_gcpermalloc) {
        tyIdx = static_cast<GcmallocMeExpr*>(expr)->GetTyIdx();
      } else {
        tyIdx = static_cast<OpMeExpr*>(expr)->GetTyIdx();
      }
    } else {
      isPhantom = true;
      eas = easOfPhanObj;
      tyIdx = kInitTyIdx;
    }
  } else {  // null alloc means creating phantom object
    isPhantom = true;
    eas = easOfPhanObj;
    tyIdx = kInitTyIdx;
  }
  if (eas == kGlobalEscape) {
    eaCG->UpdateExprOfNode(*eaCG->GetGlobalObject(), expr);
    return eaCG->GetGlobalObject();
  }
  cgChangedInSCC = true;
  EACGObjectNode *objectNode = eaCG->CreateObjectNode(expr, eas, isPhantom, tyIdx);
  if (location != nullptr) {
    objectNode->SetLocation(location);
  }
  return objectNode;
}

void IPAEscapeAnalysis::CollectDefStmtForReg(std::set<RegMeExpr*> &visited, std::set<AssignMeStmt*> &defStmts,
                                             RegMeExpr &regVar) {
  if (regVar.GetDefBy() == kDefByStmt) {
    AssignMeStmt *regAssignStmt = static_cast<AssignMeStmt*>(regVar.GetDefStmt());
    (void)defStmts.insert(regAssignStmt);
  } else if (regVar.GetDefBy() == kDefByPhi) {
    if (visited.find(&regVar) == visited.end()) {
      (void)visited.insert(&regVar);
      MePhiNode &regPhiNode = regVar.GetDefPhi();
      for (auto &reg : regPhiNode.GetOpnds()) {
        CollectDefStmtForReg(visited, defStmts, static_cast<RegMeExpr&>(*reg));
      }
    }
  } else {
    CHECK_FATAL(false, "not kDefByStmt or kDefByPhi");
  }
}

void IPAEscapeAnalysis::GetArrayBaseNodeForReg(std::vector<EACGBaseNode*> &nodes, RegMeExpr &regVar, MeStmt &stmt) {
  std::set<AssignMeStmt*> defStmts;
  std::set<RegMeExpr*> visited;
  CollectDefStmtForReg(visited, defStmts, regVar);
  for (auto &regAssignStmt : defStmts) {
    MeExpr *rhs = regAssignStmt->GetRHS();
    CHECK_FATAL(rhs != nullptr, "Impossible");
    CHECK_FATAL(rhs->GetOp() == OP_array, "Impossible, funcName: %s", func->GetName().c_str());
    NaryMeExpr *array = static_cast<NaryMeExpr*>(rhs);
    CHECK_FATAL(array->GetOpnds().size() > 0, "access array->GetOpnds() failed");
    MeExpr *base = array->GetOpnd(0);
    std::vector<EACGBaseNode*> baseNodes;
    GetCGNodeForMeExpr(baseNodes, *base, stmt, true);
    for (auto baseNode : baseNodes) {
      nodes.push_back(baseNode);
    }
  }
}

void IPAEscapeAnalysis::GetCGNodeForMeExpr(std::vector<EACGBaseNode*> &nodes, MeExpr &expr, MeStmt &stmt,
                                           bool createObjNode) {
  if (expr.GetMeOp() == kMeOpVar) {
    VarMeExpr *var = static_cast<VarMeExpr*>(&expr);
    EACGRefNode *refNode = GetOrCreateCGRefNodeForVar(*var, createObjNode);
    nodes.push_back(refNode);
  } else if (expr.GetMeOp() == kMeOpIvar) {
    IvarMeExpr *ivar = static_cast<IvarMeExpr*>(&expr);
    GetOrCreateCGFieldNodeForIvar(nodes, *ivar, stmt, createObjNode);
  } else if (expr.IsGcmalloc()) {
    EACGObjectNode *objNode = GetOrCreateCGObjNode(&expr, &stmt);
    nodes.push_back(objNode);
  } else if (expr.GetMeOp() == kMeOpReg) {
    RegMeExpr *regVar = static_cast<RegMeExpr*>(&expr);
    if (regVar->GetRegIdx() < 0) {
      eaCG->UpdateExprOfNode(*eaCG->GetGlobalObject(), &expr);
      EACGObjectNode *objNode = eaCG->GetGlobalObject();
      nodes.push_back(objNode);
    } else {
      if (regVar->GetDefBy() != kDefByStmt && regVar->GetDefBy() != kDefByMustDef) {
        CHECK_FATAL(false, "impossible");
      }
      EACGRefNode *refNode = GetOrCreateCGRefNodeForReg(*regVar, createObjNode);
      nodes.push_back(refNode);
    }
  } else if (expr.GetMeOp() == kMeOpOp && (expr.GetOp() == OP_retype || expr.GetOp() == OP_cvt)) {
    MeExpr *retypeRhs = (static_cast<OpMeExpr*>(&expr))->GetOpnd(0);
    if (IsExprRefOrPtr(*retypeRhs)) {
      GetCGNodeForMeExpr(nodes, *retypeRhs, stmt, createObjNode);
    } else {
      EACGObjectNode *objNode = nullptr;
      VarMeExpr *var = static_cast<VarMeExpr*>(retypeRhs);
      if (IsZeroConst(var)) {
        objNode = GetOrCreateCGObjNode(&expr, nullptr, kNoEscape);
      } else {
        eaCG->UpdateExprOfNode(*eaCG->GetGlobalObject(), &expr);
        objNode = eaCG->GetGlobalObject();
      }
      nodes.push_back(objNode);
    }
  } else if (expr.GetMeOp() == kMeOpOp && expr.GetOp() == OP_select) {
    OpMeExpr *opMeExpr = static_cast<OpMeExpr*>(&expr);
    EACGBaseNode *refNode = eaCG->GetCGNodeFromExpr(opMeExpr);
    if (refNode == nullptr) {
      refNode = eaCG->CreateReferenceNode(opMeExpr, kNoEscape, false);
      for (size_t i = 1; i < 3; ++i) { // OP_select expr has three operands.
        std::vector<EACGBaseNode*> opndNodes;
        GetCGNodeForMeExpr(opndNodes, *opMeExpr->GetOpnd(i), stmt, true);
        for (auto opndNode : opndNodes) {
          (void)refNode->AddOutNode(*opndNode);
        }
      }
    }
    nodes.push_back(refNode);
  } else if (expr.GetMeOp() == kMeOpAddrof && expr.GetOp() == OP_addrof) {
    AddrofMeExpr *var = static_cast<AddrofMeExpr*>(&expr);
    EACGRefNode *refNode = GetOrCreateCGRefNodeForAddrof(*var, createObjNode);
    nodes.push_back(refNode);
  } else if (expr.GetMeOp() == kMeOpOp && expr.GetOp() == OP_iaddrof) {
    OpMeExpr *opExpr = static_cast<OpMeExpr*>(&expr);
    GetOrCreateCGFieldNodeForIAddrof(nodes, *opExpr, stmt, createObjNode);
  } else if (expr.GetMeOp() == kMeOpNary &&
             (expr.GetOp() == OP_intrinsicopwithtype || expr.GetOp() == OP_intrinsicop)) {
    NaryMeExpr *naryMeExpr = static_cast<NaryMeExpr*>(&expr);
    if (naryMeExpr->GetIntrinsic() == INTRN_JAVA_CONST_CLASS) {
      // get some class's "Class", metadata
      eaCG->UpdateExprOfNode(*eaCG->GetGlobalObject(), &expr);
      EACGObjectNode *objNode = eaCG->GetGlobalObject();
      nodes.push_back(objNode);
    } else if (naryMeExpr->GetIntrinsic() == INTRN_JAVA_MERGE) {
      CHECK_FATAL(naryMeExpr->GetOpnds().size() == 1, "must have one opnd");
      MeExpr *opnd = naryMeExpr->GetOpnd(0);
      if (IsExprRefOrPtr(*opnd)) {
        GetCGNodeForMeExpr(nodes, *opnd, stmt, createObjNode);
      } else {
        eaCG->UpdateExprOfNode(*eaCG->GetGlobalObject(), &expr);
        EACGObjectNode *objNode = eaCG->GetGlobalObject();
        nodes.push_back(objNode);
      }
    } else {
      stmt.Dump(irMap);
      CHECK_FATAL(false, "NYI");
    }
  } else if (expr.GetMeOp() == kMeOpNary && expr.GetOp() == OP_array) {
    NaryMeExpr *array = static_cast<NaryMeExpr*>(&expr);
    CHECK_FATAL(array->GetOpnds().size() > 0, "access array->GetOpnds() failed");
    MeExpr *arrayBase = array->GetOpnd(0);
    GetCGNodeForMeExpr(nodes, *arrayBase, stmt, createObjNode);
  } else if (expr.GetMeOp() == kMeOpConst) {
    ConstMeExpr *constExpr = static_cast<ConstMeExpr*>(&expr);
    EACGObjectNode *objNode = nullptr;
    if (constExpr->GetConstVal()->GetKind() == kConstInt && constExpr->IsZero()) {
      objNode = GetOrCreateCGObjNode(&expr, nullptr, kNoEscape);
    } else {
      eaCG->UpdateExprOfNode(*eaCG->GetGlobalObject(), &expr);
      objNode = eaCG->GetGlobalObject();
    }
    nodes.push_back(objNode);
  } else if (expr.GetMeOp() == kMeOpConststr) {
    nodes.push_back(eaCG->GetGlobalReference());
    eaCG->UpdateExprOfGlobalRef(&expr);
  } else {
    stmt.Dump(irMap);
    CHECK_FATAL(false, "NYI funcName: %s", func->GetName().c_str());
  }
}

void IPAEscapeAnalysis::UpdateEscConnGraphWithStmt(MeStmt &stmt) {
  switch (stmt.GetOp()) {
    case OP_dassign: {
      DassignMeStmt *dasgn = static_cast<DassignMeStmt*>(&stmt);
      if (!IsExprRefOrPtr(*dasgn->GetLHS())) {
        break;
      }
      CHECK_FATAL(IsExprRefOrPtr(*dasgn->GetRHS()), "type mis-match");
      EACGRefNode *lhsNode = GetOrCreateCGRefNodeForVar(*static_cast<VarMeExpr*>(dasgn->GetVarLHS()), false);

      std::vector<EACGBaseNode*> rhsNodes;
      GetCGNodeForMeExpr(rhsNodes, *dasgn->GetRHS(), stmt, true);
      for (const auto &rhsNode : rhsNodes) {
        cgChangedInSCC = (lhsNode->AddOutNode(*rhsNode) ? true : cgChangedInSCC);
      }
      break;
    }
    case OP_iassign: {
      IassignMeStmt *iasgn = static_cast<IassignMeStmt*>(&stmt);
      if (!IsExprRefOrPtr(*iasgn->GetLHSVal())) {
        break;
      }
      CHECK_FATAL(IsExprRefOrPtr(*iasgn->GetRHS()), "type mis-match");
      // get or create field nodes for lhs (may need to create a phantom object node)
      std::vector<EACGBaseNode*> lhsNodes;
      GetOrCreateCGFieldNodeForIvar(lhsNodes, *iasgn->GetLHSVal(), stmt, false);
      std::vector<EACGBaseNode*> rhsNodes;
      GetCGNodeForMeExpr(rhsNodes, *iasgn->GetRHS(), stmt, true);
      for (const auto &lhsNode : lhsNodes) {
        for (const auto &rhsNode : rhsNodes) {
          cgChangedInSCC = (lhsNode->AddOutNode(*rhsNode) ? true : cgChangedInSCC);
        }
      }
      break;
    }
    case OP_maydassign: {
      MaydassignMeStmt *mdass = static_cast<MaydassignMeStmt*>(&stmt);
      CHECK_FATAL(mdass->GetChiList() != nullptr, "Impossible");
      if (mdass->GetChiList()->empty() || !IsExprRefOrPtr(*mdass->GetRHS())) {
        break;
      }
      for (std::pair<OStIdx, ChiMeNode*> it : *mdass->GetChiList()) {
        ChiMeNode *chi = it.second;
        CHECK_FATAL(IsExprRefOrPtr(*chi->GetLHS()), "type mis-match");
        EACGRefNode *lhsNode = GetOrCreateCGRefNodeForVar(*static_cast<VarMeExpr *>(chi->GetLHS()), false);
        std::vector<EACGBaseNode*> rhsNodes;
        GetCGNodeForMeExpr(rhsNodes, *mdass->GetRHS(), stmt, true);
        for (const auto &rhsNode : rhsNodes) {
          cgChangedInSCC = (lhsNode->AddOutNode(*rhsNode) ? true : cgChangedInSCC);
        }
      }
      break;
    }
    case OP_regassign: {
      AssignMeStmt *regasgn = static_cast<AssignMeStmt*>(&stmt);
      CHECK_FATAL(regasgn->GetLHS() != nullptr, "Impossible");
      CHECK_FATAL(regasgn->GetRHS() != nullptr, "Impossible");
      if (!IsExprRefOrPtr(*regasgn->GetLHS())) {
        break;
      }
      CHECK_FATAL(IsExprRefOrPtr(*regasgn->GetRHS()), "type mis-match");
      if (IsRegAssignStmtForClassMeta(*regasgn) || regasgn->GetRHS()->GetOp() == OP_array) {
        break;
      }
      EACGRefNode *lhsNode = GetOrCreateCGRefNodeForReg(*regasgn->GetLHS(), false);
      std::vector<EACGBaseNode*> rhsNodes;
      GetCGNodeForMeExpr(rhsNodes, *regasgn->GetRHS(), stmt, true);
      for (const auto &rhsNode : rhsNodes) {
        cgChangedInSCC = (lhsNode->AddOutNode(*rhsNode) ? true : cgChangedInSCC);
      }
      break;
    }
    case OP_throw: {
      ThrowMeStmt *throwStmt = static_cast<ThrowMeStmt*>(&stmt);
      std::vector<EACGBaseNode*> nodes;
      GetCGNodeForMeExpr(nodes, *throwStmt->GetOpnd(), stmt, true);
      for (const auto &node : nodes) {
        for (const auto &objNode : node->GetPointsToSet()) {
          if (objNode->GetEAStatus() != kGlobalEscape) {
            (void)objNode->UpdateEAStatus(kGlobalEscape);
            cgChangedInSCC = true;
          }
        }
      }
      break;
    }
    case OP_return: {
      RetMeStmt *retMeStmt = static_cast<RetMeStmt*>(&stmt);
      EACGActualNode *retNode = eaCG->GetReturnNode();
      MIRFunction *mirFunc = func->GetMirFunc();
      if (!IsTypeRefOrPtr(mirFunc->GetReturnType()->GetPrimType())) {
        break;
      }
      if (retNode == nullptr && retMeStmt->GetOpnds().size() > 0) {
        retNode = eaCG->CreateActualNode(kReturnEscape, true, true,
                                         static_cast<uint8>(mirFunc->GetFormalCount()), kInvalid);
        cgChangedInSCC = true;
      }
      for (const auto &expr : retMeStmt->GetOpnds()) {
        if (!IsExprRefOrPtr(*expr)) {
          continue;
        }
        if (expr->GetMeOp() != kMeOpVar && expr->GetMeOp() != kMeOpReg) {
          CHECK_FATAL(false, "should be");
        }
        EACGRefNode *refNode = GetOrCreateCGRefNodeForVarOrReg(*expr, true);
        cgChangedInSCC = (retNode->AddOutNode(*refNode) ? true : cgChangedInSCC);
      }
      break;
    }
    case OP_icall:
    case OP_customcall:
    case OP_polymorphiccall:
    case OP_virtualcall:
    case OP_virtualicall:
    case OP_superclasscall:
    case OP_interfacecall:
    case OP_interfaceicall:
    case OP_xintrinsiccall:
    case OP_icallassigned:
    case OP_customcallassigned:
    case OP_polymorphiccallassigned:
    case OP_xintrinsiccallassigned: {
      CHECK_FATAL(false, "NYI");
      break;
    }
    case OP_intrinsiccall: {
      IntrinsiccallMeStmt *intrn = static_cast<IntrinsiccallMeStmt*>(&stmt);
      if (intrn->GetIntrinsic() != INTRN_MPL_CLEANUP_LOCALREFVARS &&
          intrn->GetIntrinsic() != INTRN_MPL_CLEANUP_LOCALREFVARS_SKIP &&
          intrn->GetIntrinsic() != INTRN_MCCSetPermanent &&
          intrn->GetIntrinsic() != INTRN_MCCIncRef &&
          intrn->GetIntrinsic() != INTRN_MCCDecRef &&
          intrn->GetIntrinsic() != INTRN_MCCIncDecRef &&
          intrn->GetIntrinsic() != INTRN_MCCDecRefReset &&
          intrn->GetIntrinsic() != INTRN_MCCIncDecRefReset &&
          intrn->GetIntrinsic() != INTRN_MCCWrite &&
          intrn->GetIntrinsic() != INTRN_MCCWriteNoDec &&
          intrn->GetIntrinsic() != INTRN_MCCWriteNoInc &&
          intrn->GetIntrinsic() != INTRN_MCCWriteNoRC &&
          intrn->GetIntrinsic() != INTRN_MCCWriteReferent &&
          intrn->GetIntrinsic() != INTRN_MCCWriteS &&
          intrn->GetIntrinsic() != INTRN_MCCWriteSNoInc &&
          intrn->GetIntrinsic() != INTRN_MCCWriteSNoDec &&
          intrn->GetIntrinsic() != INTRN_MCCWriteSNoRC &&
          intrn->GetIntrinsic() != INTRN_MCCWriteSVol &&
          intrn->GetIntrinsic() != INTRN_MCCWriteSVolNoInc &&
          intrn->GetIntrinsic() != INTRN_MCCWriteSVolNoDec &&
          intrn->GetIntrinsic() != INTRN_MCCWriteSVolNoRC &&
          intrn->GetIntrinsic() != INTRN_MCCWriteVol &&
          intrn->GetIntrinsic() != INTRN_MCCWriteVolNoInc &&
          intrn->GetIntrinsic() != INTRN_MCCWriteVolNoDec &&
          intrn->GetIntrinsic() != INTRN_MCCWriteVolNoRC &&
          intrn->GetIntrinsic() != INTRN_MCCWriteVolWeak &&
          intrn->GetIntrinsic() != INTRN_MCCWriteWeak &&
          intrn->GetIntrinsic() != INTRN_MCCDecRefResetPair) {
        CHECK_FATAL(false, "intrnId: %d in function: %s", intrn->GetIntrinsic(), func->GetName().c_str());
      }

      if (intrn->GetIntrinsic() == INTRN_MPL_CLEANUP_LOCALREFVARS ||
          intrn->GetIntrinsic() == INTRN_MPL_CLEANUP_LOCALREFVARS_SKIP ||
          intrn->GetIntrinsic() == INTRN_MCCSetPermanent ||
          intrn->GetIntrinsic() == INTRN_MCCIncRef ||
          intrn->GetIntrinsic() == INTRN_MCCDecRef ||
          intrn->GetIntrinsic() == INTRN_MCCIncDecRef ||
          intrn->GetIntrinsic() == INTRN_MCCDecRefReset ||
          intrn->GetIntrinsic() == INTRN_MCCIncDecRefReset ||
          intrn->GetIntrinsic() == INTRN_MCCWriteReferent ||
          intrn->GetIntrinsic() == INTRN_MCCDecRefResetPair) {
        break;
      }

      CHECK_FATAL(intrn->GetOpnds().size() > 1, "must be");
      const size_t opndIdx = 2;
      MeExpr *lhs = intrn->GetOpnd(intrn->NumMeStmtOpnds() - opndIdx);
      MeExpr *rhs = intrn->GetOpnds().back();
      std::vector<EACGBaseNode*> lhsNodes;
      GetCGNodeForMeExpr(lhsNodes, *lhs, stmt, false);
      std::vector<EACGBaseNode*> rhsNodes;
      GetCGNodeForMeExpr(rhsNodes, *rhs, stmt, true);
      for (auto lhsNode : lhsNodes) {
        for (auto rhsNode : rhsNodes) {
          (void)lhsNode->AddOutNode(*rhsNode);
        }
      }
      break;
    }
    case OP_call:
    case OP_callassigned:
    case OP_superclasscallassigned:
    case OP_interfaceicallassigned:
    case OP_interfacecallassigned:
    case OP_virtualicallassigned:
    case OP_virtualcallassigned: {
      CallMeStmt *callMeStmt = static_cast<CallMeStmt*>(&stmt);
      MIRFunction &mirFunc = callMeStmt->GetTargetFunction();
      uint32 callInfo = callMeStmt->GetStmtID();
      if (callInfo == 0) {
        if (mirFunc.GetName() != "MCC_SetObjectPermanent" && mirFunc.GetName() != "MCC_DecRef_NaiveRCFast") {
          CHECK_FATAL(false, "funcName: %s", mirFunc.GetName().c_str());
        }
        break;
      }
      eaCG->TouchCallSite(callInfo);

      // If a function has no reference parameter or return value, then skip it.
      if (IsNoSideEffect(*callMeStmt)) {
        HandleParaAtCallSite(callInfo, *callMeStmt);
        break;
      }

      HandleParaAtCallSite(callInfo, *callMeStmt);
      if (stmt.GetOp() == OP_call || stmt.GetOp() == OP_callassigned || stmt.GetOp() == OP_superclasscallassigned) {
        if (IPAEscapeAnalysis::kDebug) {
          LogInfo::MapleLogger() << "[INVOKE] call func " << mirFunc.GetName() << "\n";
        }
        HandleSingleCallee(*callMeStmt);
      } else {
        if (IPAEscapeAnalysis::kDebug) {
          LogInfo::MapleLogger() << "[INVOKE] vcall func " << mirFunc.GetName() << "\n";
        }
        HandleMultiCallees(*callMeStmt);
      }
      break;
    }
    // mainly for JAVA_CLINIT_CHECK
    case OP_intrinsiccallwithtype: {
      IntrinsiccallMeStmt *intrnMestmt = static_cast<IntrinsiccallMeStmt*>(&stmt);
      if (intrnMestmt->GetIntrinsic() != INTRN_JAVA_CLINIT_CHECK &&
          intrnMestmt->GetIntrinsic() != INTRN_JAVA_CHECK_CAST) {
        CHECK_FATAL(false, "intrnId: %d in function: %s", intrnMestmt->GetIntrinsic(), func->GetName().c_str());
      }
      // 1. INTRN_JAVA_CLINIT_CHECK: Because all the operations in clinit are to initialize the static field
      //    of the Class, this will not affect the escape status of any reference or object node.
      // 2. INTRN_JAVA_CHECK_CAST: When mephase precheckcast is enabled, this will happen, we only hava to solve
      //    the next dassign stmt.
      break;
    }
    // mainly for JAVA_ARRAY_FILL and JAVA_POLYMORPHIC_CALL
    case OP_intrinsiccallassigned: {
      IntrinsiccallMeStmt *intrnMestmt = static_cast<IntrinsiccallMeStmt*>(&stmt);
      if (intrnMestmt->GetIntrinsic() == INTRN_JAVA_POLYMORPHIC_CALL) {
        // this intrinsiccall is MethodHandle.invoke, it is a native method.
        const MapleVector<MeExpr*> &opnds = intrnMestmt->GetOpnds();
        const size_t size = opnds.size();
        for (size_t i = 0; i < size; ++i) {
          MeExpr *var = opnds[i];
          // we only solve reference node.
          if (!IsExprRefOrPtr(*var)) {
            continue;
          }
          std::vector<EACGBaseNode*> nodes;
          GetCGNodeForMeExpr(nodes, *var, *intrnMestmt, true);
          for (auto realArgNode : nodes) {
            for (EACGObjectNode *obj : realArgNode->GetPointsToSet()) {
              (void)obj->UpdateEAStatus(kGlobalEscape);
            }
          }
        }
        CHECK_FATAL(intrnMestmt->GetMustDefList()->size() <= 1, "Impossible");
        if (intrnMestmt->GetMustDefList()->size() == 0) {
          break;
        }
        if (intrnMestmt->GetMustDefListItem(0).GetLHS()->GetMeOp() != kMeOpVar &&
            intrnMestmt->GetMustDefListItem(0).GetLHS()->GetMeOp() != kMeOpReg) {
          CHECK_FATAL(false, "impossible");
        }
        if (!IsExprRefOrPtr(*intrnMestmt->GetMustDefListItem(0).GetLHS())) {
          break;
        }
        EACGRefNode *realRetNode = GetOrCreateCGRefNodeForVarOrReg(*intrnMestmt->GetMustDefListItem(0).GetLHS(), true);
        for (EACGObjectNode *obj : realRetNode->GetPointsToSet()) {
          (void)obj->UpdateEAStatus(kGlobalEscape);
        }
        break;
      } else if (intrnMestmt->GetIntrinsic() == INTRN_JAVA_ARRAY_FILL) {
        // JAVA_ARRAY_FILL can be skipped.
        break;
      } else {
        if (intrnMestmt->GetIntrinsic() != INTRN_MCCIncRef &&
            intrnMestmt->GetIntrinsic() != INTRN_MCCLoadRef &&
            intrnMestmt->GetIntrinsic() != INTRN_MCCLoadRefS &&
            intrnMestmt->GetIntrinsic() != INTRN_MCCLoadRefSVol &&
            intrnMestmt->GetIntrinsic() != INTRN_MCCLoadRefVol &&
            intrnMestmt->GetIntrinsic() != INTRN_MCCLoadWeak &&
            intrnMestmt->GetIntrinsic() != INTRN_MCCLoadWeakVol) {
          CHECK_FATAL(false, "intrnId: %d in function: %s", intrnMestmt->GetIntrinsic(), func->GetName().c_str());
        }

        CHECK_FATAL(intrnMestmt->GetMustDefList()->size() == 1, "Impossible");
        if (intrnMestmt->GetMustDefListItem(0).GetLHS()->GetMeOp() != kMeOpVar &&
            intrnMestmt->GetMustDefListItem(0).GetLHS()->GetMeOp() != kMeOpReg) {
          CHECK_FATAL(false, "impossible");
        }

        if (!IsExprRefOrPtr(*intrnMestmt->GetMustDefListItem(0).GetLHS())) {
          break;
        }
        EACGRefNode *retNode = GetOrCreateCGRefNodeForVarOrReg(*intrnMestmt->GetMustDefListItem(0).GetLHS(), false);
        MeExpr *rhs = intrnMestmt->GetOpnds().back();
        std::vector<EACGBaseNode*> rhsNodes;
        GetCGNodeForMeExpr(rhsNodes, *rhs, stmt, true);
        for (auto rhsNode : rhsNodes) {
          (void)retNode->AddOutNode(*rhsNode);
        }
        break;
      }
    }
    // mainly for JAVA_CHECK_CAST and JAVA_FILL_NEW_ARRAY
    case OP_intrinsiccallwithtypeassigned: {
      IntrinsiccallMeStmt *intrnMestmt = static_cast<IntrinsiccallMeStmt*>(&stmt);
      if (intrnMestmt->GetIntrinsic() == INTRN_JAVA_CHECK_CAST) {
        // We regard this as dassign
        CHECK_FATAL(intrnMestmt->GetMustDefList()->size() <= 1, "Impossible");
        if (intrnMestmt->GetMustDefList()->size() == 0) {
          break;
        }
        CHECK_FATAL(intrnMestmt->GetMustDefListItem(0).GetLHS()->GetMeOp() == kMeOpVar, "must be kMeOpVar");
        VarMeExpr *lhs = static_cast<VarMeExpr*>(intrnMestmt->GetMustDefListItem(0).GetLHS());
        if (!IsExprRefOrPtr(*lhs)) {
          break;
        }
        CHECK_FATAL(intrnMestmt->GetOpnds().size() == 1, "Impossible");
        CHECK_FATAL(intrnMestmt->GetOpnd(0)->GetMeOp() == kMeOpVar, "must be kMeOpVar");
        VarMeExpr *rhs = static_cast<VarMeExpr*>(intrnMestmt->GetOpnd(0));
        CHECK_FATAL(IsExprRefOrPtr(*rhs), "type mis-match");
        EACGRefNode *lhsNode = GetOrCreateCGRefNodeForVar(*lhs, false);
        EACGRefNode *rhsNode = GetOrCreateCGRefNodeForVar(*rhs, true);
        (void)lhsNode->AddOutNode(*rhsNode);
      } else if (intrnMestmt->GetIntrinsic() == INTRN_JAVA_FILL_NEW_ARRAY) {
        CHECK_FATAL(intrnMestmt->GetMustDefList()->size() == 1, "Impossible");
        CHECK_FATAL(intrnMestmt->GetMustDefListItem(0).GetLHS()->GetMeOp() == kMeOpVar, "must be kMeOpVar");
        VarMeExpr *lhs = static_cast<VarMeExpr*>(intrnMestmt->GetMustDefListItem(0).GetLHS());
        if (!IsExprRefOrPtr(*lhs)) {
          break;
        }
        EACGRefNode *lhsNode = GetOrCreateCGRefNodeForVar(*lhs, true);
        CHECK_FATAL(intrnMestmt->GetOpnds().size() >= 1, "Impossible");
        for (MeExpr *expr : intrnMestmt->GetOpnds()) {
          CHECK_FATAL(expr->GetMeOp() == kMeOpVar, "Impossible");
          VarMeExpr *rhs = static_cast<VarMeExpr*>(expr);
          if (!IsExprRefOrPtr(*rhs)) {
            continue;
          }
          EACGRefNode *rhsNode = GetOrCreateCGRefNodeForVar(*rhs, true);
          for (const auto &objNode : lhsNode->GetPointsToSet()) {
            // for array case, only one field node represents all elements
            EACGFieldNode *fieldNode = objNode->GetFieldNodeFromIdx(0);
            if (fieldNode == nullptr) {
              fieldNode = eaCG->CreateFieldNode(nullptr, objNode->GetEAStatus(), 0, objNode, true);
            }
            (void)fieldNode->AddOutNode(*rhsNode);
          }
        }
      } else {
        CHECK_FATAL(false, "intrnId: %d in function: %s", intrnMestmt->GetIntrinsic(), func->GetName().c_str());
      }
      break;
    }

    default:
      break;
  }
}

EAConnectionGraph *IPAEscapeAnalysis::GetEAConnectionGraph(MIRFunction &function) const {
  if (function.GetEACG() != nullptr) {
    return function.GetEACG();
  }
  const std::map<GStrIdx, EAConnectionGraph*> &summaryMap = mirModule->GetEASummary();
  GStrIdx nameStrIdx = function.GetNameStrIdx();
  auto it = summaryMap.find(nameStrIdx);
  if (it != summaryMap.end() && it->second != nullptr) {
    return it->second;
  }
  return nullptr;
}

void IPAEscapeAnalysis::HandleParaAtCallSite(uint32 callInfo, CallMeStmt &call) {
  MapleVector<EACGBaseNode*> *argVector = eaCG->GetCallSiteArgNodeVector(callInfo);
  if (argVector != nullptr && argVector->size() > 0) {
    // We have handled this callsite before, skip it.
    return;
  }
  const MapleVector<MeExpr*> &opnds = call.GetOpnds();
  const uint32 size = static_cast<uint32>(opnds.size());

  bool isOptIcall = (call.GetOp() == OP_interfaceicallassigned || call.GetOp() == OP_virtualicallassigned);
  uint32 firstParmIdx = (isOptIcall ? 1 : 0);

  for (uint32 i = firstParmIdx; i < size; ++i) {
    MeExpr *var = opnds[i];
    // we only solve reference node.
    if (!IsExprRefOrPtr(*var) || var->GetOp() == OP_add) {
      continue;
    }
    // for func(u, v), we assume that there exists assignment: a1 = u; a2 = v;
    // a1, a2 are phantomArgNode and u, v are realArgNode, we add edge from a1 to u, etc.
    EACGActualNode *phantomArgNode =
        eaCG->CreateActualNode(kNoEscape, false, true, static_cast<uint8>(i), callInfo);
    // node for u, v.
    std::vector<EACGBaseNode*> nodes;
    GetCGNodeForMeExpr(nodes, *var, call, true);
    for (auto realArgNode : nodes) {
      (void)phantomArgNode->AddOutNode(*realArgNode);
    }
  }
  // Non-nullptr means it has return value
  CHECK_FATAL(call.GetMustDefList() != nullptr, "Impossible");
  if (call.GetMustDefList()->size() == 1) {
    if (call.GetMustDefListItem(0).GetLHS()->GetMeOp() != kMeOpVar &&
        call.GetMustDefListItem(0).GetLHS()->GetMeOp() != kMeOpReg) {
      CHECK_FATAL(false, "NYI");
    }
    if (!IsExprRefOrPtr(*call.GetMustDefListItem(0).GetLHS())) {
      return;
    }
    // for x = func(u, v), we assume that there exists assignment: r = x;
    // r is a phantom return node, and x is the real return node, we add edge from r to x.
    EACGActualNode *phantomRetNode =
        eaCG->CreateActualNode(kNoEscape, true, true, static_cast<uint8>(size), callInfo);
    // node for x
    EACGRefNode *realRetNode = GetOrCreateCGRefNodeForVarOrReg(*call.GetMustDefListItem(0).GetLHS(), true);
    (void)phantomRetNode->AddOutNode(*realRetNode);
  }
}

void IPAEscapeAnalysis::HandleSingleCallee(CallMeStmt &callMeStmt) {
  uint32 callInfoId = callMeStmt.GetStmtID();
  MIRFunction &calleeCandidate = callMeStmt.GetTargetFunction();
  if (IPAEscapeAnalysis::kDebug) {
    LogInfo::MapleLogger() << "[MERGECG] ready to merge func " << calleeCandidate.GetName() << "\n";
  }
  if (calleeCandidate.IsAbstract()) {
    CHECK_FATAL(false, "Impossible");
    if (IPAEscapeAnalysis::kDebug) {
      LogInfo::MapleLogger() << "[MERGECG] skip to merge func because it is abstract." << "\n";
    }
    return;
  }

  EAConnectionGraph *calleeSummary = GetEAConnectionGraph(calleeCandidate);

  if (!mirModule->IsInIPA()) {
    // This phase is in maplecomb, we need handle single callee differently
    if (calleeSummary == nullptr) {
      if (!calleeCandidate.IsNative() && !calleeCandidate.IsEmpty()) {
        CHECK_FATAL(false, "Impossible");
      }
      bool changedAfterMerge = eaCG->MergeCG(*eaCG->GetCallSiteArgNodeVector(callInfoId), nullptr);
      if (changedAfterMerge) {
        cgChangedInSCC = true;
      }
    } else {
      MapleVector<EACGBaseNode*> *caller = eaCG->GetCallSiteArgNodeVector(callInfoId);
      const MapleVector<EACGBaseNode*> *callee = calleeSummary->GetFuncArgNodeVector();
      bool changedAfterMerge = eaCG->MergeCG(*caller, callee);
      if (changedAfterMerge) {
        cgChangedInSCC = true;
      }
    }
    return;
  }

  CGNode *callerNode = pcg->GetCGNode(func->GetMirFunc());
  CHECK_FATAL(callerNode != nullptr, "Impossible, funcName: %s", func->GetName().c_str());
  CGNode *calleeNode = pcg->GetCGNode(&calleeCandidate);
  CHECK_FATAL(calleeNode != nullptr, "Impossible, funcName: %s", calleeCandidate.GetName().c_str());
  if (calleeNode->GetSCCNode() == callerNode->GetSCCNode() &&
      (eaCG->GetNeedConservation() ||
       callerNode->GetSCCNode()->GetNodes().size() > IPAEscapeAnalysis::kFuncInSCCLimit)) {
    bool changedAfterMerge = eaCG->MergeCG(*eaCG->GetCallSiteArgNodeVector(callInfoId), nullptr);
    if (changedAfterMerge) {
      cgChangedInSCC = true;
    }
    if (IPAEscapeAnalysis::kDebug) {
      LogInfo::MapleLogger() << "[MERGECG] skip to merge func because NeedConservation." << "\n";
    }
    return;
  }
  if (calleeSummary == nullptr && calleeCandidate.GetBody() != nullptr && !calleeCandidate.IsNative()) {
    if (IPAEscapeAnalysis::kDebug) {
      LogInfo::MapleLogger() << "[MERGECG] skip to merge func because this is first loop in scc." << "\n";
    }
    return;
  }
  if (calleeSummary == nullptr) {
    bool changedAfterMerge = eaCG->MergeCG(*eaCG->GetCallSiteArgNodeVector(callInfoId), nullptr);
    if (changedAfterMerge) {
      cgChangedInSCC = true;
    }
  } else {
    MapleVector<EACGBaseNode*> *caller = eaCG->GetCallSiteArgNodeVector(callInfoId);
    const MapleVector<EACGBaseNode*> *callee = calleeSummary->GetFuncArgNodeVector();
    bool changedAfterMerge = eaCG->MergeCG(*caller, callee);
    if (changedAfterMerge) {
      cgChangedInSCC = true;
    }
  }
}

void IPAEscapeAnalysis::HandleMultiCallees(const CallMeStmt &callMeStmt) {
  uint32 callInfoId = callMeStmt.GetStmtID();
  bool changedAfterMerge = eaCG->MergeCG(*eaCG->GetCallSiteArgNodeVector(callInfoId), nullptr);
  if (changedAfterMerge) {
    cgChangedInSCC = true;
  }
}

void IPAEscapeAnalysis::UpdateEscConnGraphWithPhi(const BB &bb) {
  const MapleMap<OStIdx, MePhiNode*> &mePhiList = bb.GetMePhiList();
  for (auto it = mePhiList.begin(); it != mePhiList.end(); ++it) {
    MePhiNode *phiNode = it->second;
    auto *lhs = phiNode->GetLHS();
    if (lhs->GetMeOp() != kMeOpVar) {
      continue;
    }
    if (!IsExprRefOrPtr(*lhs) || phiNode->GetOpnds().empty() ||
        IsVirtualVar(*ssaTab, static_cast<VarMeExpr&>(*lhs))) {
      continue;
    }
    EACGRefNode *lhsNode = GetOrCreateCGRefNodeForVar(static_cast<VarMeExpr&>(*lhs), false);
    for (auto itt = phiNode->GetOpnds().begin(); itt != phiNode->GetOpnds().end(); ++itt) {
      auto *var = static_cast<VarMeExpr*>(*itt);
      EACGRefNode *rhsNode = GetOrCreateCGRefNodeForVar(*var, true);
      cgChangedInSCC = (lhsNode->AddOutNode(*rhsNode) ? true : cgChangedInSCC);
    }
  }
}

void IPAEscapeAnalysis::HandleParaAtFuncEntry() {
  if (!mirModule->IsInIPA()) {
    CHECK_FATAL(eaCG == nullptr, "Impossible");
  }

  if (eaCG != nullptr) {
    return;
  }
  MIRFunction *mirFunc = func->GetMirFunc();
  eaCG = mirModule->GetMemPool()->New<EAConnectionGraph>(
      mirModule, &mirModule->GetMPAllocator(), mirFunc->GetNameStrIdx());
  eaCG->InitGlobalNode();
  OriginalStTable &ostTab = ssaTab->GetOriginalStTable();
  // create actual node for formal parameter
  for (size_t i = 0; i < mirFunc->GetFormalCount(); ++i) {
    MIRSymbol *mirSt = mirFunc->GetFormal(i);
    OriginalSt *ost = ostTab.FindOrCreateSymbolOriginalSt(*mirSt, mirFunc->GetPuidx(), 0);
    VarMeExpr *formal = irMap->GetOrCreateZeroVersionVarMeExpr(*ost);
    if (IsExprRefOrPtr(*formal)) {
      EACGActualNode *actualNode =
          eaCG->CreateActualNode(kArgumentEscape, false, true, static_cast<uint8>(i), kInvalid);
      EACGObjectNode *objNode = eaCG->CreateObjectNode(nullptr, kNoEscape, true, kInitTyIdx);
      (void)actualNode->AddOutNode(*objNode);
      EACGRefNode *formalNode = eaCG->CreateReferenceNode(formal, kNoEscape, false);
      (void)formalNode->AddOutNode(*actualNode);
    }
  }
}

void IPAEscapeAnalysis::ConstructConnGraph() {
  HandleParaAtFuncEntry();
  auto cfg = func->GetCfg();
  cfg->BuildSCC();
  const MapleVector<SCCOfBBs*> &sccTopologicalVec = cfg->GetSccTopologicalVec();
  for (size_t i = 0; i < sccTopologicalVec.size(); ++i) {
    SCCOfBBs *scc = sccTopologicalVec[i];
    CHECK_FATAL(scc != nullptr, "nullptr check");
    if (scc->GetBBs().size() > 1) {
      cfg->BBTopologicalSort(*scc);
    }
    cgChangedInSCC = true;
    bool analyzeAgain = true;
    while (analyzeAgain) {
      analyzeAgain = false;
      cgChangedInSCC = false;
      for (BB *bb : scc->GetBBs()) {
        if (bb == cfg->GetCommonEntryBB() || bb == cfg->GetCommonExitBB()) {
          continue;
        }
        UpdateEscConnGraphWithPhi(*bb);
        for (MeStmt *stmt = to_ptr(bb->GetMeStmts().begin()); stmt != nullptr; stmt = stmt->GetNextMeStmt()) {
          UpdateEscConnGraphWithStmt(*stmt);
        }
      }
      if (scc->HasCycle() && cgChangedInSCC) {
        analyzeAgain = true;
      }
    }
  }
  eaCG->PropogateEAStatus();
  func->GetMirFunc()->SetEACG(eaCG);
}

void IPAEscapeAnalysis::DoOptimization() {
  CountObjRCOperations();
  ProcessNoAndRetEscObj();
  ProcessRetStmt();
  DeleteRedundantRC();
}

VarMeExpr *IPAEscapeAnalysis::CreateEATempVarWithName(const std::string &name) const {
  const auto &strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(name);
  VarMeExpr *var = irMap->CreateNewVar(strIdx, PTY_ref, false);
  return var;
}

OriginalSt *IPAEscapeAnalysis::CreateEATempOst() {
  std::string name = std::string("__EATemp__").append(std::to_string(++tempCount));
  return CreateEATempOstWithName(name);
}

OriginalSt *IPAEscapeAnalysis::CreateEARetTempOst() {
  std::string name = std::string("__EARetTemp__");
  return CreateEATempOstWithName(name);
}

OriginalSt *IPAEscapeAnalysis::CreateEATempOstWithName(const std::string &name) const {
  MIRSymbol *symbol = func->GetMIRModule().GetMIRBuilder()->CreateLocalDecl(name,
                                                                            *GlobalTables::GetTypeTable().GetRef());
  OriginalSt *ost = ssaTab->CreateSymbolOriginalSt(*symbol, func->GetMirFunc()->GetPuidx(), 0);
  ost->SetZeroVersionIndex(irMap->GetVerst2MeExprTable().size());
  irMap->GetVerst2MeExprTable().push_back(nullptr);
  ost->PushbackVersionsIndices(ost->GetZeroVersionIndex());
  return ost;
}

VarMeExpr *IPAEscapeAnalysis::CreateEATempVarMeExpr(OriginalSt &ost) const {
  VarMeExpr *var = irMap->CreateVarMeExprVersion(&ost);
  return var;
}

VarMeExpr *IPAEscapeAnalysis::GetOrCreateEARetTempVarMeExpr(OriginalSt &ost) {
  if (retVar != nullptr) {
    return retVar;
  }
  retVar = CreateEATempVarMeExpr(ost);
  return retVar;
}

VarMeExpr *IPAEscapeAnalysis::CreateEATempVar() {
  std::string name = std::string("__EATemp__").append(std::to_string(++tempCount));
  return CreateEATempVarWithName(name);
}

VarMeExpr *IPAEscapeAnalysis::GetOrCreateEARetTempVar() {
  if (retVar != nullptr) {
    return retVar;
  }
  std::string name = std::string("__EARetTemp__");
  retVar = CreateEATempVarWithName(name);
  return retVar;
}

void IPAEscapeAnalysis::ProcessNoAndRetEscObj() {
  MeCFG *cfg = func->GetCfg();
  for (BB *bb : cfg->GetAllBBs()) {
    if (bb == cfg->GetCommonEntryBB() || bb == cfg->GetCommonExitBB() || bb == nullptr ||
        bb->GetAttributes(kBBAttrIsInLoopForEA)) {
      continue;
    }

    for (MeStmt *stmt = to_ptr(bb->GetMeStmts().begin()); stmt != nullptr; stmt = stmt->GetNextMeStmt()) {
      if (stmt->GetOp() == OP_dassign || stmt->GetOp() == OP_regassign || stmt->GetOp() == OP_iassign ||
          stmt->GetOp() == OP_maydassign) {
        MeExpr *rhs = stmt->GetRHS();
        CHECK_FATAL(rhs != nullptr, "nullptr check");
        if (!rhs->IsGcmalloc()) {
          continue;
        }
        CHECK_FATAL(func->GetMirFunc()->GetEACG() != nullptr, "Impossible");
        EACGBaseNode *node = func->GetMirFunc()->GetEACG()->GetCGNodeFromExpr(rhs);
        CHECK_FATAL(node != nullptr, "nullptr check");
        CHECK_FATAL(node->IsObjectNode(), "impossible");
        EAStatus eaStatus = node->GetEAStatus();
        if ((eaStatus == kNoEscape) && (!static_cast<EACGObjectNode*>(node)->IsPointedByFieldNode()) &&
            (static_cast<EACGObjectNode*>(node)->GetRCOperations() >= kRCOperLB)) {
          static_cast<EACGObjectNode*>(node)->SetIgnorRC(true);
          gcStmts.push_back(stmt);
          OriginalSt *ost = CreateEATempOst();
          noAndRetEscOst.push_back(ost);
        }
      }
    }
  }
  if (noAndRetEscOst.size() == 0) {
    return;
  }
  BB *firstBB = cfg->GetFirstBB();
  CHECK_FATAL(firstBB != nullptr, "Impossible");
  for (size_t i = 0; i < noAndRetEscOst.size(); ++i) {
    OriginalSt *ost = noAndRetEscOst[i];
    MeStmt *stmt = gcStmts.at(i);
    BB *curBB = stmt->GetBB();
    VarMeExpr *initVar = CreateEATempVarMeExpr(*ost);
    MeExpr *zeroExpr = irMap->CreateIntConstMeExpr(0, PTY_ref);
    DassignMeStmt *initStmt = static_cast<DassignMeStmt *>(irMap->CreateAssignMeStmt(*initVar, *zeroExpr, *firstBB));
    firstBB->AddMeStmtFirst(initStmt);

    VarMeExpr *var = CreateEATempVarMeExpr(*ost);
    noAndRetEscObj.push_back(var);
    ScalarMeExpr *lhs = stmt->GetLHS();
    CHECK_FATAL(lhs != nullptr, "nullptr check");
    DassignMeStmt *newStmt = static_cast<DassignMeStmt *>(irMap->CreateAssignMeStmt(*var, *lhs, *curBB));
    curBB->InsertMeStmtAfter(stmt, newStmt);
    IntrinsiccallMeStmt *meStmt = irMap->NewInPool<IntrinsiccallMeStmt>(OP_intrinsiccall, INTRN_MCCSetObjectPermanent);
    meStmt->PushBackOpnd(var);
    curBB->InsertMeStmtAfter(newStmt, meStmt);
  }
}

void IPAEscapeAnalysis::ProcessRetStmt() {
  if (noAndRetEscObj.size() == 0) {
    return;
  }
  MeCFG *cfg = func->GetCfg();
  BB *firstBB = cfg->GetFirstBB();
  OriginalSt *ost = CreateEARetTempOst();
  VarMeExpr *initVar = CreateEATempVarMeExpr(*ost);
  MeExpr *zeroExpr = irMap->CreateIntConstMeExpr(0, PTY_ref);
  DassignMeStmt *newStmt = static_cast<DassignMeStmt *>(irMap->CreateAssignMeStmt(*initVar, *zeroExpr, *firstBB));
  ASSERT(firstBB != nullptr, "null ptr check");
  firstBB->AddMeStmtFirst(newStmt);

  for (BB *bb : cfg->GetAllBBs()) {
    if (bb == cfg->GetCommonEntryBB() || bb == cfg->GetCommonExitBB() || bb == nullptr) {
      continue;
    }
    for (MeStmt *stmt = to_ptr(bb->GetMeStmts().begin()); stmt != nullptr; stmt = stmt->GetNextMeStmt()) {
      if (stmt->GetOp() == OP_return) {
        RetMeStmt *retMeStmt = static_cast<RetMeStmt *>(stmt);
        CHECK_FATAL(retMeStmt->GetOpnds().size() <= 1, "must less than one");
        VarMeExpr *var = GetOrCreateEARetTempVarMeExpr(*ost);
        for (const auto &expr : retMeStmt->GetOpnds()) {
          if (IsExprRefOrPtr(*expr)) {
            DassignMeStmt *newStmtTmp = static_cast<DassignMeStmt *>(irMap->CreateAssignMeStmt(*var, *expr, *bb));
            bb->InsertMeStmtBefore(stmt, newStmtTmp);
          }
        }
        IntrinsiccallMeStmt *meStmt = irMap->NewInPool<IntrinsiccallMeStmt>(
            OP_intrinsiccall, INTRN_MPL_CLEANUP_NORETESCOBJS);
        meStmt->PushBackOpnd(var);
        for (auto opnd : noAndRetEscObj) {
          meStmt->PushBackOpnd(opnd);
        }
        bb->InsertMeStmtBefore(stmt, meStmt);
      }
    }
  }
}

void IPAEscapeAnalysis::CountObjRCOperations() {
  MeCFG *cfg = func->GetCfg();
  for (BB *bb : cfg->GetAllBBs()) {
    if (bb == cfg->GetCommonEntryBB() || bb == cfg->GetCommonExitBB() || bb == nullptr) {
      continue;
    }
    for (MeStmt *stmt = to_ptr(bb->GetMeStmts().begin()); stmt != nullptr; stmt = stmt->GetNextMeStmt()) {
      switch (stmt->GetOp()) {
        case OP_intrinsiccall: {
          IntrinsiccallMeStmt *intrn = static_cast<IntrinsiccallMeStmt*>(stmt);
          switch (intrn->GetIntrinsic()) {
            case INTRN_MCCIncRef:
            case INTRN_MCCIncDecRef:
            case INTRN_MCCIncDecRefReset: {
              CHECK_FATAL(eaCG->GetCGNodeFromExpr(intrn->GetOpnd(0)) != nullptr, "nullptr check");
              std::vector<EACGBaseNode*> nodes;
              GetCGNodeForMeExpr(nodes, *intrn->GetOpnd(0), *intrn, false);
              for (auto refNode : nodes) {
                for (auto objNode : refNode->GetPointsToSet()) {
                  objNode->IncresRCOperations();
                }
              }
              break;
            }
            case INTRN_MCCWrite:
            case INTRN_MCCWriteNoDec:
            case INTRN_MCCWriteS:
            case INTRN_MCCWriteSNoDec:
            case INTRN_MCCWriteSVol:
            case INTRN_MCCWriteSVolNoDec:
            case INTRN_MCCWriteVol:
            case INTRN_MCCWriteVolNoDec:
            case INTRN_MCCWriteVolWeak:
            case INTRN_MCCWriteWeak: {
              CHECK_FATAL(eaCG->GetCGNodeFromExpr(intrn->GetOpnds().back()) != nullptr, "nullptr check");
              std::vector<EACGBaseNode*> nodes;
              GetCGNodeForMeExpr(nodes, *intrn->GetOpnds().back(), *intrn, false);
              for (auto refNode : nodes) {
                for (auto objNode : refNode->GetPointsToSet()) {
                  objNode->IncresRCOperations();
                }
              }
              break;
            }
            default:
              break;
          }
          break;
        }
        case OP_intrinsiccallassigned: {
          IntrinsiccallMeStmt *intrn = static_cast<IntrinsiccallMeStmt*>(stmt);
          switch (intrn->GetIntrinsic()) {
            case INTRN_MCCIncRef:
            case INTRN_MCCLoadRef:
            case INTRN_MCCLoadRefS:
            case INTRN_MCCLoadRefSVol:
            case INTRN_MCCLoadRefVol:
            case INTRN_MCCLoadWeak:
            case INTRN_MCCLoadWeakVol: {
              CHECK_FATAL(intrn->GetMustDefList()->size() == 1, "Impossible");
              if (intrn->GetMustDefListItem(0).GetLHS()->GetMeOp() != kMeOpVar &&
                  intrn->GetMustDefListItem(0).GetLHS()->GetMeOp() != kMeOpReg) {
                CHECK_FATAL(false, "must be kMeOpVar or kMeOpReg");
              }

              if (!IsExprRefOrPtr(*intrn->GetMustDefListItem(0).GetLHS())) {
                break;
              }
              EACGBaseNode *refNode = eaCG->GetCGNodeFromExpr(intrn->GetMustDefListItem(0).GetLHS());
              CHECK_FATAL(refNode != nullptr, "nullptr check");
              for (auto objNode : refNode->GetPointsToSet()) {
                objNode->IncresRCOperations();
              }
              break;
            }
            default:
              break;
          }
          break;
        }
        case OP_call:
        case OP_callassigned:
        case OP_superclasscallassigned: {
          CallMeStmt *callMeStmt = static_cast<CallMeStmt*>(stmt);

          // If a function has no reference parameter or return value, then skip it.
          if (IsNoSideEffect(*callMeStmt)) {
            break;
          }
          MIRFunction &calleeCandidate = callMeStmt->GetTargetFunction();
          std::string fName = calleeCandidate.GetName();
          if (fName == "MCC_GetOrInsertLiteral" ||
              fName == "MCC_GetCurrentClassLoader" ||
              fName == "Native_Thread_currentThread" ||
              fName == "Native_java_lang_StringFactory_newStringFromBytes___3BIII" ||
              fName == "Native_java_lang_StringFactory_newStringFromChars__II_3C" ||
              fName == "Native_java_lang_StringFactory_newStringFromString__Ljava_lang_String_2" ||
              fName == "Native_java_lang_String_intern__" ||
              fName == "MCC_StringAppend" ||
              fName == "MCC_StringAppend_StringInt" ||
              fName == "MCC_StringAppend_StringJcharString" ||
              fName == "MCC_StringAppend_StringString") {
            break;
          }
          const MapleVector<MeExpr*> &opnds = callMeStmt->GetOpnds();
          const size_t size = opnds.size();

          bool isOptIcall =
              (callMeStmt->GetOp() == OP_interfaceicallassigned || callMeStmt->GetOp() == OP_virtualicallassigned);
          size_t firstParmIdx = (isOptIcall ? 1 : 0);

          bool isSpecialCall = false;
          if (fName == "Native_java_lang_Object_clone_Ljava_lang_Object__" ||
              fName == "Native_java_lang_String_concat__Ljava_lang_String_2" ||
              fName ==
                  "Ljava_2Flang_2FAbstractStringBuilder_3B_7CappendCLONEDignoreret_7C_28Ljava_2Flang_2FString_3B_29V" ||
              StartWith(fName, "Ljava_2Flang_2FAbstractStringBuilder_3B_7Cappend_7C") ||
              StartWith(fName, "Ljava_2Flang_2FStringBuilder_3B_7Cappend_7C")) {
            CallMeStmt *call = static_cast<CallMeStmt*>(callMeStmt);
            CHECK_FATAL(call->GetMustDefList() != nullptr, "funcName: %s", fName.c_str());
            CHECK_FATAL(call->GetMustDefList()->size() <= 1, "funcName: %s", fName.c_str());
            if (call->GetMustDefList() != nullptr && call->GetMustDefList()->size() == 0) {
              break;
            }
            isSpecialCall = true;
          }

          for (size_t i = firstParmIdx; i < size; ++i) {
            MeExpr *var = opnds[i];
            // we only solve reference node.
            if (!IsExprRefOrPtr(*var) || var->GetOp() == OP_add) {
              continue;
            }
            CHECK_NULL_FATAL(eaCG->GetCGNodeFromExpr(var));
            std::vector<EACGBaseNode*> nodes;
            GetCGNodeForMeExpr(nodes, *var, *callMeStmt, false);
            CHECK_FATAL(nodes.size() > 0, "the size must not be zero");
            for (EACGBaseNode *refNode : nodes) {
              for (auto objNode : refNode->GetPointsToSet()) {
                objNode->IncresRCOperations();
              }
            }
            if (isSpecialCall) {
              break;
            }
          }
          break;
        }
        case OP_intrinsiccallwithtypeassigned: {
          CHECK_FATAL(false, "must not be OP_intrinsiccallwithtypeassigned");
          break;
        }
        default:
          break;
      }
    }
  }

  for (EACGBaseNode *node : eaCG->GetNodes()) {
    if (node == nullptr || !node->IsObjectNode()) {
      continue;
    }
    EACGObjectNode *obj = static_cast<EACGObjectNode*>(node);
    if (obj->IsPhantom()) {
      continue;
    }
    if (obj->IsPointedByFieldNode()) {
      obj->IncresRCOperations(kRCOperLB);
    }
  }
}

void IPAEscapeAnalysis::DeleteRedundantRC() {
  MeCFG *cfg = func->GetCfg();
  for (BB *bb : cfg->GetAllBBs()) {
    if (bb == cfg->GetCommonEntryBB() || bb == cfg->GetCommonExitBB() || bb == nullptr) {
      continue;
    }
    for (MeStmt *stmt = to_ptr(bb->GetMeStmts().begin()); stmt != nullptr; stmt = stmt->GetNextMeStmt()) {
      if (stmt->GetOp() == OP_intrinsiccall) {
        IntrinsiccallMeStmt *intrn = static_cast<IntrinsiccallMeStmt*>(stmt);
        switch (intrn->GetIntrinsic()) {
          case INTRN_MCCIncRef:
          case INTRN_MCCDecRef:
          case INTRN_MCCIncDecRef: {
            bool canRemoveStmt = true;
            for (auto expr : intrn->GetOpnds()) {
              if (eaCG->GetCGNodeFromExpr(expr) == nullptr) {
                canRemoveStmt = false;
                break;
              }
              std::vector<EACGBaseNode*> nodes;
              GetCGNodeForMeExpr(nodes, *expr, *intrn, false);
              for (auto node : nodes) {
                if (!node->CanIgnoreRC()) {
                  canRemoveStmt = false;
                  break;
                }
              }
              if (!canRemoveStmt) {
                break;
              }
            }
            if (canRemoveStmt) {
              bb->RemoveMeStmt(stmt);
            }
            break;
          }
          case INTRN_MCCIncDecRefReset:
          case INTRN_MCCDecRefReset: {
            bool canRemoveStmt = true;
            for (auto expr : intrn->GetOpnds()) {
              if (expr->GetMeOp() != kMeOpAddrof) {
                if (eaCG->GetCGNodeFromExpr(expr) == nullptr) {
                  canRemoveStmt = false;
                  break;
                }
                std::vector<EACGBaseNode*> nodes;
                GetCGNodeForMeExpr(nodes, *expr, *intrn, false);
                for (auto node : nodes) {
                  if (!node->CanIgnoreRC()) {
                    canRemoveStmt = false;
                    break;
                  }
                }
                if (!canRemoveStmt) {
                  break;
                }
              } else {
                AddrofMeExpr *addrof = static_cast<AddrofMeExpr*>(expr);
                const OriginalSt *ost = addrof->GetOst();
                ASSERT(ost != nullptr, "null ptr check");
                for (auto index : ost->GetVersionsIndices()) {
                  if (ost->IsFormal()) {
                    canRemoveStmt = false;
                    break;
                  }
                  if (index == ost->GetZeroVersionIndex()) {
                    continue;
                  }
                  MeExpr *var = irMap->GetMeExprByVerID(static_cast<uint32>(index));
                  if (var == nullptr) {
                    continue;
                  }
                  if (eaCG->GetCGNodeFromExpr(var) == nullptr) {
                    canRemoveStmt = false;
                    break;
                  }
                  std::vector<EACGBaseNode*> nodes;
                  GetCGNodeForMeExpr(nodes, *var, *intrn, false);
                  for (auto node : nodes) {
                    if (!node->CanIgnoreRC()) {
                      canRemoveStmt = false;
                      break;
                    }
                  }
                }
                if (!canRemoveStmt) {
                  break;
                }
              }
            }
            if (canRemoveStmt) {
              bb->RemoveMeStmt(stmt);
            }
            break;
          }
          default:
            break;
        }
      }
    }
  }
}
}
