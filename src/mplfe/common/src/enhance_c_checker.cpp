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
#include "enhance_c_checker.h"
#include "ast_expr.h"
#include "ast_stmt.h"
#include "feir_builder.h"

namespace maple {
UniqueFEIRExpr ENCChecker::FindBaseExprInPointerOperation(const UniqueFEIRExpr &expr) {
  if (expr == nullptr) {
    return nullptr;
  }
  UniqueFEIRExpr baseExpr = nullptr;
  if (expr->GetKind() == kExprBinary) {
    FEIRExprBinary *binExpr = static_cast<FEIRExprBinary*>(expr.get());
    if (binExpr->IsComparative()) {
      return nullptr;
    }
    baseExpr = FindBaseExprInPointerOperation(binExpr->GetOpnd0());
    if (baseExpr != nullptr) {
      return baseExpr;
    }
    baseExpr = FindBaseExprInPointerOperation(binExpr->GetOpnd1());
    if (baseExpr != nullptr) {
      return baseExpr;
    }
  }
  if (expr->GetKind() == kExprAddrofArray) {
    FEIRExprAddrofArray *arrExpr = static_cast<FEIRExprAddrofArray*>(expr.get());
    baseExpr = FindBaseExprInPointerOperation(arrExpr->GetExprArray());
    if (baseExpr != nullptr) {
      return baseExpr;
    }
  }
  if ((expr->GetKind() == kExprDRead && expr->GetPrimType() == PTY_ptr) ||
      (expr->GetKind() == kExprIRead && expr->GetPrimType() == PTY_ptr && expr->GetFieldID() != 0) ||
      IsAddrofArrayVar(expr) != nullptr) {
    baseExpr = expr->Clone();
  }
  return baseExpr;
}

MIRType *ENCChecker::IsAddrofArrayVar(const UniqueFEIRExpr &expr) {
  if (expr->GetKind() == kExprAddrofVar) {
    MIRType *type = expr->GetVarUses().front()->GetType()->GenerateMIRTypeAuto();
    if (expr->GetFieldID() == 0) {
      if (type->GetKind() == kTypeArray) {
        return type;
      }
    } else {
      CHECK_FATAL(type->IsStructType(), "basetype must be StructType");
      FieldID fieldID = expr->GetFieldID();
      FieldPair fieldPair = static_cast<MIRStructType*>(type)->TraverseToFieldRef(fieldID);
      MIRType *arrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldPair.second.first);
      if (arrType->GetKind() == kTypeArray) {
        return arrType;
      }
    }
  }
  if (expr->GetKind() == kExprIAddrof) {
    FEIRExprIAddrof *iaddrof = static_cast<FEIRExprIAddrof*>(expr.get());
    MIRType *pointerType = iaddrof->GetClonedPtrType()->GenerateMIRTypeAuto();
    CHECK_FATAL(pointerType->IsMIRPtrType(), "Must be ptr type!");
    MIRType *baseType = static_cast<MIRPtrType*>(pointerType)->GetPointedType();
    CHECK_FATAL(baseType->IsStructType(), "basetype must be StructType");
    FieldID fieldID = iaddrof->GetFieldID();
    FieldPair fieldPair = static_cast<MIRStructType*>(baseType)->TraverseToFieldRef(fieldID);
    MIRType *arrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldPair.second.first);
    if (arrType->GetKind() == kTypeArray) {
      return arrType;
    }
  }
  return nullptr;
}

void ENCChecker::AssignBoundaryVar(MIRBuilder &mirBuilder, const UniqueFEIRExpr &dstExpr, const UniqueFEIRExpr &srcExpr,
                                   std::list<StmtNode*> &ans) {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic() ||
      srcExpr->GetPrimType() != PTY_ptr || dstExpr->GetPrimType() != PTY_ptr) {
    return;
  }
  // Avoid inserting redundant boundary vars
  const std::string prefix = "_boundary.";
  if (dstExpr->GetKind() == kExprDRead &&
      static_cast<FEIRExprDRead*>(dstExpr.get())->GetVar()->GetNameRaw().compare(0, prefix.size(), prefix) == 0) {
    return;
  }
  MIRFunction *curFunction = mirBuilder.GetCurrentFunctionNotNull();
  // Check if the r-value has a boundary var or an array
  UniqueFEIRExpr baseExpr = ENCChecker::FindBaseExprInPointerOperation(srcExpr);
  if (baseExpr == nullptr) {
    return;
  }
  MIRType *arrType = ENCChecker::IsAddrofArrayVar(baseExpr);
  // Check if the l-value has a boundary var
  std::pair<StIdx, StIdx> lBoundaryVarStIdx = std::make_pair(StIdx(0), StIdx(0));
  auto lIt = curFunction->GetBoundaryMap().find(dstExpr->Hash());
  if (lIt != curFunction->GetBoundaryMap().end()) {  // The boundary var exists on the l-value
    lBoundaryVarStIdx = lIt->second;
  }
  std::pair<StIdx, StIdx> rBoundaryVarStIdx = std::make_pair(StIdx(0), StIdx(0));
  auto rIt = curFunction->GetBoundaryMap().find(baseExpr->Hash());
  if (rIt != curFunction->GetBoundaryMap().end()) {  // Assgin when the boundary var exists on the r-value
    rBoundaryVarStIdx = rIt->second;
  } else {
    if (lBoundaryVarStIdx.first != StIdx(0) && arrType == nullptr) {
      // Insert a empty r-value boundary var
      // when there is a l-value boundary var and r-value without a boundary var or with an array
      rBoundaryVarStIdx = ENCChecker::InsertBoundaryVar(mirBuilder, baseExpr);
    }
  }
  // insert L-value bounary and assign boundary var
  // when r-value with a boundary var or with an array
  if (lBoundaryVarStIdx.first == StIdx(0) &&
      (rBoundaryVarStIdx.first != StIdx(0) || arrType != nullptr)) {
    lBoundaryVarStIdx = ENCChecker::InsertBoundaryVar(mirBuilder, dstExpr);
  }
  if (lBoundaryVarStIdx.first != StIdx(0)) {
    MIRSymbol *lLowerSym = curFunction->GetLocalOrGlobalSymbol(lBoundaryVarStIdx.first);
    CHECK_NULL_FATAL(lLowerSym);
    MIRSymbol *lUpperSym = curFunction->GetLocalOrGlobalSymbol(lBoundaryVarStIdx.second);
    CHECK_NULL_FATAL(lUpperSym);
    if (rBoundaryVarStIdx.first != StIdx(0)) {
      MIRSymbol *rLowerSym = curFunction->GetLocalOrGlobalSymbol(rBoundaryVarStIdx.first);
      CHECK_NULL_FATAL(rLowerSym);
      StmtNode *lowerStmt = mirBuilder.CreateStmtDassign(*lLowerSym, 0, mirBuilder.CreateExprDread(*rLowerSym));
      ans.emplace_back(lowerStmt);

      MIRSymbol *rUpperSym = curFunction->GetLocalOrGlobalSymbol(rBoundaryVarStIdx.second);
      CHECK_NULL_FATAL(rUpperSym);
      StmtNode *upperStmt = mirBuilder.CreateStmtDassign(*lUpperSym, 0, mirBuilder.CreateExprDread(*rUpperSym));
      ans.emplace_back(upperStmt);
    } else if (arrType != nullptr) {
      StmtNode *lowerStmt = mirBuilder.CreateStmtDassign(*lLowerSym, 0, baseExpr->GenMIRNode(mirBuilder));
      ans.emplace_back(lowerStmt);
      UniqueFEIRExpr binExpr = FEIRBuilder::CreateExprBinary(
          OP_add, baseExpr->Clone(), std::make_unique<FEIRExprConst>(arrType->GetSize(), PTY_ptr));
      StmtNode *upperStmt = mirBuilder.CreateStmtDassign(*lUpperSym, 0, binExpr->GenMIRNode(mirBuilder));
      ans.emplace_back(upperStmt);
    }
  }
}

std::pair<StIdx, StIdx> ENCChecker::InsertBoundaryVar(MIRBuilder &mirBuilder, const UniqueFEIRExpr &expr) {
  std::pair<StIdx, StIdx> boundaryVarStIdx = std::make_pair(StIdx(0), StIdx(0));
  std::string boundaryName;
  if (expr != nullptr && expr->GetKind() == kExprDRead) {
    FEIRExprDRead *dread = static_cast<FEIRExprDRead*>(expr.get());
    // Naming format for var boundary: _boundary.[varname]_[fieldID].[exprHash].lower/upper
    boundaryName = "_boundary." + dread->GetVar()->GetNameRaw() + "." + std::to_string(expr->GetFieldID()) + "." +
                   std::to_string(expr->Hash());
  } else if (expr != nullptr && expr->GetKind() == kExprIRead) {
    FEIRExprIRead *iread = static_cast<FEIRExprIRead*>(expr.get());
    MIRType *pointerType = iread->GetClonedPtrType()->GenerateMIRTypeAuto();
    CHECK_FATAL(pointerType->IsMIRPtrType(), "Must be ptr type!");
    MIRType *structType = static_cast<MIRPtrType*>(pointerType)->GetPointedType();
    std::string structName = GlobalTables::GetStrTable().GetStringFromStrIdx(structType->GetNameStrIdx());
    FieldID fieldID = iread->GetFieldID();
    FieldPair rFieldPair = static_cast<MIRStructType*>(structType)->TraverseToFieldRef(fieldID);
    std::string fieldName = GlobalTables::GetStrTable().GetStringFromStrIdx(rFieldPair.first);
    // Naming format for field var boundary: _boundary.[sturctname]_[fieldname].[exprHash].lower/upper
    boundaryName = "_boundary." + structName + "_" + fieldName + "." + std::to_string(iread->Hash());
  } else {
    return boundaryVarStIdx;
  }
  MIRType *boundaryType = expr->GetType()->GenerateMIRTypeAuto();
  MIRSymbol *lowerSrcSym = mirBuilder.GetOrCreateLocalDecl(boundaryName + ".lower", *boundaryType);
  MIRSymbol *upperSrcSym = mirBuilder.GetOrCreateLocalDecl(boundaryName + ".upper", *boundaryType);
  boundaryVarStIdx = std::make_pair(lowerSrcSym->GetStIdx(), upperSrcSym->GetStIdx());
  // update BoundaryMap
  MIRFunction *curFunction = mirBuilder.GetCurrentFunctionNotNull();
  curFunction->SetBoundaryMap(expr->Hash(), boundaryVarStIdx);
  return boundaryVarStIdx;
}

UniqueFEIRExpr ENCChecker::CvtArray2PtrForm(const UniqueFEIRExpr &expr, bool &isConstantIdx) {
  FEIRExprAddrofArray *arrExpr = static_cast<FEIRExprAddrofArray*>(expr.get());
  UniqueFEIRExpr resExpr = nullptr;
  if (arrExpr->GetExprArray()->GetKind() == kExprAddrofArray) {
    resExpr = CvtArray2PtrForm(arrExpr->GetExprArray()->Clone(), isConstantIdx);
  }
  MIRType *type = arrExpr->GetTypeArray()->GenerateMIRTypeAuto();
  if (type->GetKind() != kTypeArray) {
    return nullptr;
  }
  MIRArrayType *arrType = static_cast<MIRArrayType*>(type);
  size_t offsetSize = arrType->GetElemType()->GetSize();
  for (size_t i = arrType->GetDim() - 1; i > 0; --i) {
    offsetSize *= arrType->GetSizeArrayItem(static_cast<uint32>(i));
  }
  CHECK_FATAL(arrExpr->GetExprIndexs().size() == 1, "idx num must be 1");
  if (arrExpr->GetExprIndexs().front()->GetKind() != kExprConst) {
    isConstantIdx = false;
  }
  UniqueFEIRExpr mulExpr = FEIRBuilder::CreateExprBinary(OP_mul,
      arrExpr->GetExprIndexs().front()->Clone(), FEIRBuilder::CreateExprConstI32(static_cast<int32>(offsetSize)));
  mulExpr = FEIRBuilder::CreateExprCvtPrim(std::move(mulExpr), PTY_ptr);
  if (arrExpr->GetExprArray()->GetKind() != kExprAddrofArray) {
    return FEIRBuilder::CreateExprBinary(OP_add, arrExpr->GetExprArray()->Clone(), std::move(mulExpr));
  } else if (resExpr != nullptr) {
    return FEIRBuilder::CreateExprBinary(OP_add, std::move(resExpr), std::move(mulExpr));
  } else {
    return nullptr;
  }
}

void ENCChecker::PeelNestedBoundaryChecking(std::list<UniqueFEIRStmt> &stmts, const UniqueFEIRExpr &baseExpr) {
  std::list<UniqueFEIRStmt>::iterator i = stmts.begin();
  while (i != stmts.end()) {
    bool flag = ((*i)->GetKind() == kFEIRStmtNary);
    if (flag) {
      FEIRStmtNary *nary = static_cast<FEIRStmtNary*>((*i).get());
      flag = (nary->GetOP() == OP_assertge || nary->GetOP() == OP_assertlt) &&
             nary->GetArgExprs().back()->Hash() == baseExpr->Hash();
    }
    if (flag) {
      i = stmts.erase(i);
    } else {
      ++i;
    }
  }
}

void FEIRStmtDAssign::AssignBoundaryVar(MIRBuilder &mirBuilder, std::list<StmtNode*> &ans) const {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic()) {
    return;
  }
  UniqueFEIRExpr dstExpr;
  if (fieldID == 0) {
    dstExpr = FEIRBuilder::CreateExprDRead(var->Clone());
  } else {
    FieldID tmpID = fieldID;
    FieldPair fieldPair = static_cast<MIRStructType*>(var->GetType()->GenerateMIRTypeAuto())->TraverseToFieldRef(tmpID);
    UniqueFEIRType fieldType = FEIRTypeHelper::CreateTypeNative(
        *GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldPair.second.first));
    dstExpr = FEIRBuilder::CreateExprDReadAggField(var->Clone(), fieldID, std::move(fieldType));
  }
  ENCChecker::AssignBoundaryVar(mirBuilder, dstExpr, expr, ans);
}

void FEIRStmtIAssign::AssignBoundaryVarAndChecking(MIRBuilder &mirBuilder, std::list<StmtNode*> &ans) const {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic() || fieldID == 0) {
    return;
  }
  MIRType *baseType = static_cast<MIRPtrType*>(addrType->GenerateMIRTypeAuto())->GetPointedType();
  FieldID tmpID = fieldID;
  FieldPair fieldPair = static_cast<MIRStructType*>(baseType)->TraverseToFieldRef(tmpID);
  MIRType *dstType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldPair.second.first);
  UniqueFEIRExpr dstExpr = FEIRBuilder::CreateExprIRead(
      FEIRTypeHelper::CreateTypeNative(*dstType), addrType->Clone(), addrExpr->Clone(), fieldID);
  ENCChecker::AssignBoundaryVar(mirBuilder, dstExpr, baseExpr, ans);
  InsertBoundaryChecking(mirBuilder, ans, dstType);
}

StmtNode *FEIRStmtNary::ReplaceBoundaryVar(MIRBuilder &mirBuilder) const {
  MIRFunction *curFunction = mirBuilder.GetCurrentFunctionNotNull();
  BaseNode *rightNode = nullptr;
  MIRType *arrType = ENCChecker::IsAddrofArrayVar(argExprs.back());
  if (arrType != nullptr) {  // var must be array type by the previous checking
    UniqueFEIRExpr rightExpr = argExprs.back()->Clone();
    if (op == OP_assertlt) {
      rightExpr = FEIRBuilder::CreateExprBinary(
          OP_add, std::move(rightExpr), std::make_unique<FEIRExprConst>(arrType->GetSize(), PTY_ptr));
    }
    rightNode = rightExpr->GenMIRNode(mirBuilder);
  } else {
    auto it = curFunction->GetBoundaryMap().find(argExprs.back()->Hash());
    if (it == curFunction->GetBoundaryMap().end()) {
      return nullptr;
    }
    StIdx boundaryStIdx = (op == OP_assertge) ? it->second.first : it->second.second;
    MIRSymbol *boundarySym = curFunction->GetLocalOrGlobalSymbol(boundaryStIdx);
    CHECK_NULL_FATAL(boundarySym);
    rightNode = mirBuilder.CreateExprDread(*boundarySym);
  }
  MapleVector<BaseNode*> args(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
  BaseNode *leftNode = argExprs.front()->GenMIRNode(mirBuilder);
  args.emplace_back(leftNode);
  args.emplace_back(rightNode);
  return mirBuilder.CreateStmtNary(op, std::move(args));
}

void ASTArraySubscriptExpr::InsertBoundaryChecking(std::list<UniqueFEIRStmt> &stmts,
                                                   UniqueFEIRExpr idxExpr, UniqueFEIRExpr baseAddrFEExpr) const {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic()) {
    return;
  }
  if (arrayType->GetKind() == MIRTypeKind::kTypeArray) {
    bool isConstantIdx = true;
    idxExpr = ENCChecker::CvtArray2PtrForm(idxExpr, isConstantIdx);
    if (isConstantIdx) {
      return;  // skip checking when all indexes are constants
    }
    while (baseAddrFEExpr != nullptr && baseAddrFEExpr->GetKind() == kExprAddrofArray) {
      baseAddrFEExpr = static_cast<FEIRExprAddrofArray*>(baseAddrFEExpr.get())->GetExprArray()->Clone();
    }
  }
  // peel nested boundary checking in a multi-dimensional array
  ENCChecker::PeelNestedBoundaryChecking(stmts, baseAddrFEExpr);
  // insert lower boundary chencking, baseExpr will be replace by lower boundary var when FEIRStmtNary GenMIRStmts
  std::list<UniqueFEIRExpr> lowerExprs;
  lowerExprs.emplace_back(idxExpr->Clone());
  lowerExprs.emplace_back(baseAddrFEExpr->Clone());
  UniqueFEIRStmt lowerStmt = std::make_unique<FEIRStmtNary>(OP_assertge, std::move(lowerExprs));
  stmts.emplace_back(std::move(lowerStmt));
  // insert upper boundary chencking, baseExpr will be replace by upper boundary var when FEIRStmtNary GenMIRStmts
  std::list<UniqueFEIRExpr> upperExprs;
  upperExprs.emplace_back(std::move(idxExpr));
  upperExprs.emplace_back(std::move(baseAddrFEExpr));
  UniqueFEIRStmt upperStmt = std::make_unique<FEIRStmtNary>(OP_assertlt, std::move(upperExprs));
  stmts.emplace_back(std::move(upperStmt));
}

void ASTUODerefExpr::InsertBoundaryChecking(std::list<UniqueFEIRStmt> &stmts, UniqueFEIRExpr expr) const {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic()) {
    return;
  }
  UniqueFEIRExpr baseExpr = ENCChecker::FindBaseExprInPointerOperation(expr);
  if (baseExpr == nullptr) {
    return;
  }
  // peel nested boundary checking in a multi-dimensional array
  ENCChecker::PeelNestedBoundaryChecking(stmts, baseExpr);
  // insert lower boundary chencking, baseExpr will be replace by lower boundary var when FEIRStmtNary GenMIRStmts
  std::list<UniqueFEIRExpr> lowerExprs;
  lowerExprs.emplace_back(expr->Clone());
  lowerExprs.emplace_back(baseExpr->Clone());
  UniqueFEIRStmt lowerStmt = std::make_unique<FEIRStmtNary>(OP_assertge, std::move(lowerExprs));
  stmts.emplace_back(std::move(lowerStmt));
  // insert upper boundary chencking, baseExpr will be replace by upper boundary var when FEIRStmtNary GenMIRStmts
  std::list<UniqueFEIRExpr> upperExprs;
  upperExprs.emplace_back(std::move(expr));
  upperExprs.emplace_back(std::move(baseExpr));
  UniqueFEIRStmt upperStmt = std::make_unique<FEIRStmtNary>(OP_assertlt, std::move(upperExprs));
  stmts.emplace_back(std::move(upperStmt));
}

void FEIRStmtDAssign::InsertBoundaryChecking(MIRBuilder &mirBuilder, std::list<StmtNode*> &ans) const {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic() ||
      var->GetType()->GetPrimType() != PTY_ptr || expr->GetPrimType() != PTY_ptr ||
      expr->GetKind() != kExprBinary) {  // pointer computed assignment
    return;
  }
  UniqueFEIRExpr idxExpr = FEIRBuilder::CreateExprDRead(var->Clone());
  // insert l-value lower boundary chencking
  std::list<UniqueFEIRExpr> lowerExprs;
  lowerExprs.emplace_back(idxExpr->Clone());
  lowerExprs.emplace_back(idxExpr->Clone());
  UniqueFEIRStmt lowerStmt = std::make_unique<FEIRStmtNary>(OP_assertge, std::move(lowerExprs));
  std::list<StmtNode*> lowerStmts = lowerStmt->GenMIRStmts(mirBuilder);
  ans.splice(ans.end(), lowerStmts);
  // insert l-value upper boundary chencking
  std::list<UniqueFEIRExpr> upperExprs;
  upperExprs.emplace_back(idxExpr->Clone());
  upperExprs.emplace_back(idxExpr->Clone());
  UniqueFEIRStmt upperStmt = std::make_unique<FEIRStmtNary>(OP_assertlt, std::move(upperExprs));
  std::list<StmtNode*> upperStmts = upperStmt->GenMIRStmts(mirBuilder);
  ans.splice(ans.end(), upperStmts);
}

void FEIRStmtIAssign::InsertBoundaryChecking(MIRBuilder &mirBuilder, std::list<StmtNode*> &ans,
                                             MIRType *dstType) const {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic() ||
      !dstType->IsMIRPtrType() || baseExpr->GetPrimType() != PTY_ptr ||
      baseExpr->GetKind() != kExprBinary) {  // pointer computed assignment
    return;
  }
  UniqueFEIRExpr idxExpr = FEIRBuilder::CreateExprIRead(
      FEIRTypeHelper::CreateTypeNative(*dstType), addrType->Clone(), addrExpr->Clone(), fieldID);
  // insert l-value lower boundary chencking
  std::list<UniqueFEIRExpr> lowerExprs;
  lowerExprs.emplace_back(idxExpr->Clone());
  lowerExprs.emplace_back(idxExpr->Clone());
  UniqueFEIRStmt lowerStmt = std::make_unique<FEIRStmtNary>(OP_assertge, std::move(lowerExprs));
  std::list<StmtNode*> lowerStmts = lowerStmt->GenMIRStmts(mirBuilder);
  ans.splice(ans.end(), lowerStmts);
  // insert l-value upper boundary chencking
  std::list<UniqueFEIRExpr> upperExprs;
  upperExprs.emplace_back(idxExpr->Clone());
  upperExprs.emplace_back(idxExpr->Clone());
  UniqueFEIRStmt upperStmt = std::make_unique<FEIRStmtNary>(OP_assertlt, std::move(upperExprs));
  std::list<StmtNode*> upperStmts = upperStmt->GenMIRStmts(mirBuilder);
  ans.splice(ans.end(), upperStmts);
}

void ASTCallExpr::InsertBoudaryVarInRet(std::list<UniqueFEIRStmt> &stmts) const {
  for (ASTStmt *bs : boundaryStmts) {
    for (UniqueFEIRStmt &feStmt : bs->Emit2FEStmt()) {
      stmts.emplace_back(std::move(feStmt));
    }
  }
}
}