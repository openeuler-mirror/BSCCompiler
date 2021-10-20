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
#include "ast_parser.h"
#include "ast_expr.h"
#include "ast_stmt.h"
#include "ast_decl_builder.h"
#include "feir_builder.h"
#include "fe_manager.h"

namespace maple {
bool ENCChecker::HasNonnullAttrInExpr(MIRBuilder &mirBuilder, const UniqueFEIRExpr &expr) {
  if (expr->GetKind() == kExprDRead) {
    FEIRExprDRead *dread = static_cast<FEIRExprDRead*>(expr.get());
    MIRSymbol *symbol = dread->GetVar()->GenerateMIRSymbol(mirBuilder);
    if (expr->GetFieldID() == 0) {
      return symbol->GetAttr(ATTR_nonnull);
    } else {
      FieldID fieldID = expr->GetFieldID();
      CHECK_FATAL(symbol->GetType()->IsStructType(), "basetype must be StructType");
      FieldPair fieldPair = static_cast<MIRStructType*>(symbol->GetType())->TraverseToFieldRef(fieldID);
      return fieldPair.second.second.GetAttr(FLDATTR_nonnull);
    }
  } else if (expr->GetKind() == kExprIRead) {
    FEIRExprIRead *iread = static_cast<FEIRExprIRead*>(expr.get());
    FieldID fieldID = expr->GetFieldID();
    if (fieldID == 0) {  // multi-dimensional pointer
      return HasNonnullAttrInExpr(mirBuilder, iread->GetClonedOpnd());
    }
    MIRType *pointerType = iread->GetClonedPtrType()->GenerateMIRTypeAuto();
    CHECK_FATAL(pointerType->IsMIRPtrType(), "Must be ptr type!");
    MIRType *baseType = static_cast<MIRPtrType*>(pointerType)->GetPointedType();
    CHECK_FATAL(baseType->IsStructType(), "basetype must be StructType");
    FieldPair fieldPair = static_cast<MIRStructType*>(baseType)->TraverseToFieldRef(fieldID);
    return fieldPair.second.second.GetAttr(FLDATTR_nonnull);
  } else {
    WARN(kLncWarn, "invalid assertnonnull arg!");
    return true;
  }
}

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
      GetArrayTypeFromExpr(expr) != nullptr) {
    baseExpr = expr->Clone();
  }
  return baseExpr;
}

MIRType *ENCChecker::GetArrayTypeFromExpr(const UniqueFEIRExpr &expr) {
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
  MIRType *arrType = ENCChecker::GetArrayTypeFromExpr(baseExpr);
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
  // assign undef val to boundary var in func body head
  AssignUndefVal(mirBuilder, *upperSrcSym);
  AssignUndefVal(mirBuilder, *lowerSrcSym);
  // update BoundaryMap
  boundaryVarStIdx = std::make_pair(lowerSrcSym->GetStIdx(), upperSrcSym->GetStIdx());
  MIRFunction *curFunction = mirBuilder.GetCurrentFunctionNotNull();
  curFunction->SetBoundaryMap(expr->Hash(), boundaryVarStIdx);
  return boundaryVarStIdx;
}

void ENCChecker::AssignUndefVal(MIRBuilder &mirBuilder, const MIRSymbol &sym) {
  BaseNode *undef = mirBuilder.CreateIntConst(0xdeadbeef, PTY_ptr);
  StmtNode *assign = mirBuilder.CreateStmtDassign(sym, 0, undef);
  MIRFunction *curFunction = mirBuilder.GetCurrentFunctionNotNull();
  curFunction->GetBody()->InsertFirst(assign);
}

void ENCChecker::InitBoundaryVarFromASTDecl(MapleAllocator &allocator, ASTDecl *ptrDecl, ASTExpr *lenExpr,
                                            std::list<ASTStmt*> &stmts) {
  MIRType *ptrType = ptrDecl->GetTypeDesc().front();
  // insert lower boundary stmt
  ASTDeclRefExpr *lowerRefExpr = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
  lowerRefExpr->SetASTDecl(ptrDecl);
  std::string lowerVarName = "_boundary." + ptrDecl->GetName() + ".lower";
  ASTVar *lowerDecl = ASTDeclsBuilder::ASTVarBuilder(
      allocator, "", lowerVarName, std::vector<MIRType*>{ptrType}, GenericAttrs());
  lowerDecl->SetIsParam(true);
  lowerDecl->SetInitExpr(lowerRefExpr);
  ASTDeclStmt *lowerStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTDeclStmt>(allocator);
  lowerStmt->SetSubDecl(lowerDecl);
  // insert upper boundary stmt
  ASTDeclRefExpr *upperRefExpr = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
  upperRefExpr->SetASTDecl(ptrDecl);
  ASTBinaryOperatorExpr *upperBinExpr = ASTDeclsBuilder::ASTExprBuilder<ASTBinaryOperatorExpr>(allocator);
  upperBinExpr->SetLeftExpr(upperRefExpr);
  upperBinExpr->SetRightExpr(lenExpr);
  upperBinExpr->SetOpcode(OP_add);
  upperBinExpr->SetRetType(ptrType);
  upperBinExpr->SetCvtNeeded(true);
  std::string upperVarName = "_boundary." + ptrDecl->GetName() + ".upper";
  ASTVar *upperDecl = ASTDeclsBuilder::ASTVarBuilder(
      allocator, "", upperVarName, std::vector<MIRType*>{ptrType}, GenericAttrs());
  upperDecl->SetIsParam(true);
  upperDecl->SetInitExpr(upperBinExpr);
  ASTDeclStmt *upperStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTDeclStmt>(allocator);
  upperStmt->SetSubDecl(upperDecl);
  stmts.emplace_back(lowerStmt);
  stmts.emplace_back(upperStmt);
}

void ENCChecker::InitBoundaryVar(MIRFunction &curFunction, const std::string &ptrName, MIRType &ptrType,
                                 UniqueFEIRExpr lenExpr, std::list<UniqueFEIRStmt> &stmts) {
  UniqueFEIRExpr ptrExpr = FEIRBuilder::CreateExprDRead(FEIRBuilder::CreateVarNameForC(ptrName, ptrType));
  // insert lower boundary stmt
  std::string lowerVarName = "_boundary." + ptrName + ".lower";
  UniqueFEIRVar lowerVar = FEIRBuilder::CreateVarNameForC(lowerVarName, ptrType);
  UniqueFEIRStmt lowerStmt = FEIRBuilder::CreateStmtDAssign(std::move(lowerVar), ptrExpr->Clone());
  stmts.emplace_back(std::move(lowerStmt));
  // insert upper boundary stmt
  UniqueFEIRExpr binExpr = FEIRBuilder::CreateExprBinary(OP_add, ptrExpr->Clone(), std::move(lenExpr));
  std::string upperVarName = "_boundary." + ptrName + ".upper";
  UniqueFEIRVar upperVar = FEIRBuilder::CreateVarNameForC(upperVarName, ptrType);
  UniqueFEIRStmt upperStmt = FEIRBuilder::CreateStmtDAssign(std::move(upperVar), std::move(binExpr));
  stmts.emplace_back(std::move(upperStmt));
  // update BoundaryMap
  MIRSymbol *lowerSym = FEManager::GetMIRBuilder().GetOrCreateDeclInFunc(lowerVarName, ptrType, curFunction);
  MIRSymbol *upperSym = FEManager::GetMIRBuilder().GetOrCreateDeclInFunc(upperVarName, ptrType, curFunction);
  curFunction.SetBoundaryMap(ptrExpr->Hash(), std::make_pair(lowerSym->GetStIdx(), upperSym->GetStIdx()));
}

bool ENCChecker::IsConstantIndex(const UniqueFEIRExpr &expr) {
  if (expr->GetKind() != kExprAddrofArray) {
    return false;
  }
  FEIRExprAddrofArray *arrExpr = static_cast<FEIRExprAddrofArray*>(expr.get());
  // nested array
  if (arrExpr->GetExprArray()->GetKind() == kExprAddrofArray) {
    if (!IsConstantIndex(arrExpr->GetExprArray()->Clone())) {
      return false;
    }
  }
  for (const auto &idxExpr : arrExpr->GetExprIndexs()) {
    if (idxExpr->GetKind() != kExprConst) {
      return false;
    }
  }
  return true;
}

void ENCChecker::PeelNestedBoundaryChecking(std::list<UniqueFEIRStmt> &stmts, const UniqueFEIRExpr &baseExpr) {
  std::list<UniqueFEIRStmt>::iterator i = stmts.begin();
  while (i != stmts.end()) {
    bool flag = ((*i)->GetKind() == kFEIRStmtNary);
    if (flag) {
      FEIRStmtNary *nary = static_cast<FEIRStmtNary*>((*i).get());
      flag = kOpcodeInfo.IsAssertBoundary(nary->GetOP()) &&
             nary->GetArgExprs().back()->Hash() == baseExpr->Hash();
    }
    if (flag) {
      i = stmts.erase(i);
    } else {
      ++i;
    }
  }
}

UniqueFEIRExpr ENCChecker::GetRealBoundaryLenExpr(const UniqueFEIRExpr &lenExpr, const ASTFunc &astFunc,
                                                  const ASTCallExpr &astCallExpr) {
  if (lenExpr == nullptr) {
    return nullptr;
  }
  if (lenExpr->GetKind() == kExprBinary) {
    FEIRExprBinary *mulExpr = static_cast<FEIRExprBinary*>(lenExpr.get());
    UniqueFEIRExpr leftExpr = GetRealBoundaryLenExpr(mulExpr->GetOpnd0(), astFunc, astCallExpr);
    if (leftExpr != nullptr) {
      mulExpr->SetOpnd0(std::move(leftExpr));
    } else {
      return nullptr;
    }
    UniqueFEIRExpr rightExpr = GetRealBoundaryLenExpr(mulExpr->GetOpnd1(), astFunc, astCallExpr);
    if (rightExpr != nullptr) {
      mulExpr->SetOpnd1(std::move(rightExpr));
    } else {
      return nullptr;
    }
  }
  if (lenExpr->GetKind() == kExprUnary) {
    FEIRExprUnary *cvtExpr = static_cast<FEIRExprUnary*>(lenExpr.get());
    UniqueFEIRExpr subExpr = GetRealBoundaryLenExpr(cvtExpr->GetOpnd(), astFunc, astCallExpr);
    if (subExpr != nullptr) {
      cvtExpr->SetOpnd(std::move(subExpr));
    } else {
      return nullptr;
    }
  }
  // formal parameter length expr -> actual parameter expr
  std::list<UniqueFEIRStmt> nullStmts;
  if (lenExpr->GetKind() == kExprDRead) {
    std::string lenName = lenExpr->GetVarUses().front()->GetNameRaw();
    for (size_t i = 0; astFunc.GetParamDecls().size(); ++i) {
      if (lenName == astFunc.GetParamDecls()[i]->GetName()) {
        return astCallExpr.GetArgsExpr()[i]->Emit2FEExpr(nullStmts);
      }
    }
    WARN(kLncWarn, "%s:%d EnhanceC warning: The actual parameter [%s] as boundary length var is not found"
         "in the caller [%s]", FEManager::GetModule().GetFileNameFromFileNum(astCallExpr.GetSrcFileIdx()).c_str(),
         astCallExpr.GetSrcFileLineNum(), lenName.c_str(), astFunc.GetName().c_str());
  }
  return lenExpr->Clone();
}

/*
 * process boundary in ast func
 */
std::list<UniqueFEIRStmt> ASTFunc::InitArgsBoundaryVar(MIRFunction &mirFunc) const {
  std::list<UniqueFEIRStmt> stmts;
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic() || compound == nullptr) {
    return stmts;
  }
  for (auto paramDecl : paramDecls) {
    ASTExpr *lenExpr = paramDecl->GetBoundaryLenExpr();
    if (lenExpr == nullptr) {
      continue;
    }
    UniqueFEIRExpr lenFEExpr = lenExpr->Emit2FEExpr(stmts);
    ENCChecker::InitBoundaryVar(
        mirFunc, paramDecl->GetName(), *paramDecl->GetTypeDesc().front(), std::move(lenFEExpr), stmts);
  }
  return stmts;
}

void ASTFunc::InsertBoundaryCheckingInRet(std::list<UniqueFEIRStmt> &stmts) const {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic() || boundaryLenExpr == nullptr ||
      stmts.size() == 0 || stmts.back()->GetKind() != kStmtReturn) {
    return;
  }
  std::list<UniqueFEIRStmt> nullStmts;
  static_cast<FEIRStmtReturn*>(stmts.back().get())->GetExpr();
  UniqueFEIRExpr retExpr = static_cast<FEIRStmtReturn*>(stmts.back().get())->GetExpr()->Clone();
  UniqueFEIRExpr baseExpr = ENCChecker::FindBaseExprInPointerOperation(retExpr);
  if (baseExpr == nullptr) {
    return;
  }
  std::list<UniqueFEIRExpr> exprs;
  UniqueFEIRExpr lenExpr = boundaryLenExpr->Emit2FEExpr(nullStmts);
  lenExpr = FEIRBuilder::CreateExprBinary(OP_add, std::move(retExpr), std::move(lenExpr));
  exprs.emplace_back(std::move(lenExpr));
  exprs.emplace_back(std::move(baseExpr));
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtNary>(OP_returnassertle, std::move(exprs));
  stmt->SetSrcFileInfo(stmts.back()->GetSrcFileIdx(), stmts.back()->GetSrcFileLineNum());
  stmts.insert(--stmts.end(), std::move(stmt));
}

/*
 * process boundary in stmt of ast func
 */
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

MapleVector<BaseNode*> FEIRStmtNary::ReplaceBoundaryChecking(MIRBuilder &mirBuilder) const {
  MIRFunction *curFunction = mirBuilder.GetCurrentFunctionNotNull();
  UniqueFEIRExpr leftExpr = argExprs.front()->Clone();
  UniqueFEIRExpr rightExpr = argExprs.back()->Clone();
  BaseNode *leftNode = nullptr;
  BaseNode *rightNode = nullptr;
  MIRType *arrType = ENCChecker::GetArrayTypeFromExpr(argExprs.back());
  MapleVector<BaseNode*> args(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
  if (arrType != nullptr) {  // var must be array type by the previous checking
    // Convert to the following node for array:
    // assertge/assertlt lnode: index; assertle lnode: (attributed upper boundary) index + len expr
    // assertge rnode: addrof array; assertlt/assertle rnode: addrof array + sizeof expr
    if (kOpcodeInfo.IsAssertUpperBoundary(op)) {
      rightExpr = FEIRBuilder::CreateExprBinary(
          OP_add, std::move(rightExpr), std::make_unique<FEIRExprConst>(arrType->GetSize(), PTY_ptr));
    }
    leftNode = leftExpr->GenMIRNode(mirBuilder);
    rightNode = rightExpr->GenMIRNode(mirBuilder);
  } else {
    // Convert to the following node for expr with boundary var:
    // assertge/assertlt lnode: index; assertle lnode: (attributed upper boundary) index + len expr
    // assertge rnode: lower boundary; assertlt/assertle rnode: upper boundary
    auto it = curFunction->GetBoundaryMap().find(argExprs.back()->Hash());
    if (it == curFunction->GetBoundaryMap().end()) {
      if (op == OP_returnassertle || op == OP_callassertle) {
        WARN(kLncWarn, "%s:%d EnhanceC waring: boundaryless pointer passed to callee that requires a boundary pointer"
             "argument", FEManager::GetModule().GetFileNameFromFileNum(srcFileIndex).c_str(), srcFileLineNum);
      }
      return args;
    }
    StIdx boundaryStIdx = kOpcodeInfo.IsAssertLowerBoundary(op) ? it->second.first : it->second.second;
    MIRSymbol *boundarySym = curFunction->GetLocalOrGlobalSymbol(boundaryStIdx);
    CHECK_NULL_FATAL(boundarySym);
    leftNode = leftExpr->GenMIRNode(mirBuilder);
    rightNode = mirBuilder.CreateExprDread(*boundarySym);
  }
  args.emplace_back(leftNode);
  args.emplace_back(rightNode);
  return args;
}

void ASTArraySubscriptExpr::InsertBoundaryChecking(std::list<UniqueFEIRStmt> &stmts,
                                                   UniqueFEIRExpr idxExpr, UniqueFEIRExpr baseAddrFEExpr) const {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic()) {
    return;
  }
  if (arrayType->GetKind() == MIRTypeKind::kTypeArray) {
    if (ENCChecker::IsConstantIndex(idxExpr)) {
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
  lowerStmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
  stmts.emplace_back(std::move(lowerStmt));
  // insert upper boundary chencking, baseExpr will be replace by upper boundary var when FEIRStmtNary GenMIRStmts
  std::list<UniqueFEIRExpr> upperExprs;
  upperExprs.emplace_back(std::move(idxExpr));
  upperExprs.emplace_back(std::move(baseAddrFEExpr));
  UniqueFEIRStmt upperStmt = std::make_unique<FEIRStmtNary>(OP_assertlt, std::move(upperExprs));
  upperStmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
  stmts.emplace_back(std::move(upperStmt));
}

void ASTUODerefExpr::InsertBoundaryChecking(std::list<UniqueFEIRStmt> &stmts, UniqueFEIRExpr expr) const {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic() ||
      expr->GetKind() != kExprBinary) {  // only computed pointer
    return;
  }
  UniqueFEIRExpr baseExpr = ENCChecker::FindBaseExprInPointerOperation(expr);
  if (baseExpr == nullptr) {
    return;
  }
  // peel nested boundary checking in a multi-dereference
  ENCChecker::PeelNestedBoundaryChecking(stmts, baseExpr);
  // insert lower boundary chencking, baseExpr will be replace by lower boundary var when FEIRStmtNary GenMIRStmts
  std::list<UniqueFEIRExpr> lowerExprs;
  lowerExprs.emplace_back(expr->Clone());
  lowerExprs.emplace_back(baseExpr->Clone());
  UniqueFEIRStmt lowerStmt = std::make_unique<FEIRStmtNary>(OP_assertge, std::move(lowerExprs));
  lowerStmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
  stmts.emplace_back(std::move(lowerStmt));
  // insert upper boundary chencking, baseExpr will be replace by upper boundary var when FEIRStmtNary GenMIRStmts
  std::list<UniqueFEIRExpr> upperExprs;
  upperExprs.emplace_back(std::move(expr));
  upperExprs.emplace_back(std::move(baseExpr));
  UniqueFEIRStmt upperStmt = std::make_unique<FEIRStmtNary>(OP_assertlt, std::move(upperExprs));
  upperStmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
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
  UniqueFEIRStmt lowerStmt = std::make_unique<FEIRStmtNary>(OP_assignassertge, std::move(lowerExprs));
  lowerStmt->SetSrcFileInfo(srcFileIndex, srcFileLineNum);
  std::list<StmtNode*> lowerStmts = lowerStmt->GenMIRStmts(mirBuilder);
  ans.splice(ans.end(), lowerStmts);
  // insert l-value upper boundary chencking
  std::list<UniqueFEIRExpr> upperExprs;
  upperExprs.emplace_back(idxExpr->Clone());
  upperExprs.emplace_back(idxExpr->Clone());
  UniqueFEIRStmt upperStmt = std::make_unique<FEIRStmtNary>(OP_assignassertlt, std::move(upperExprs));
  upperStmt->SetSrcFileInfo(srcFileIndex, srcFileLineNum);
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
  UniqueFEIRStmt lowerStmt = std::make_unique<FEIRStmtNary>(OP_assignassertge, std::move(lowerExprs));
  lowerStmt->SetSrcFileInfo(srcFileIndex, srcFileLineNum);
  std::list<StmtNode*> lowerStmts = lowerStmt->GenMIRStmts(mirBuilder);
  ans.splice(ans.end(), lowerStmts);
  // insert l-value upper boundary chencking
  std::list<UniqueFEIRExpr> upperExprs;
  upperExprs.emplace_back(idxExpr->Clone());
  upperExprs.emplace_back(idxExpr->Clone());
  UniqueFEIRStmt upperStmt = std::make_unique<FEIRStmtNary>(OP_assignassertlt, std::move(upperExprs));
  upperStmt->SetSrcFileInfo(srcFileIndex, srcFileLineNum);
  std::list<StmtNode*> upperStmts = upperStmt->GenMIRStmts(mirBuilder);
  ans.splice(ans.end(), upperStmts);
}

/*
 * caller boundary inserting and checking
 */
void ASTCallExpr::InsertBoundaryCheckingInArgs(std::list<UniqueFEIRStmt> &stmts) const {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic() || funcDecl == nullptr) {
    return;
  }
  std::list<UniqueFEIRStmt> nullStmts;
  for (size_t i = 0; i < funcDecl->GetParamDecls().size(); ++i) {
    ASTDecl *paramDecl = funcDecl->GetParamDecls()[i];
    ASSERT_NOT_NULL(paramDecl);
    if (paramDecl->GetBoundaryLenExpr() == nullptr) {
      continue;
    }
    UniqueFEIRExpr realLenExpr = ENCChecker::GetRealBoundaryLenExpr(
        paramDecl->GetBoundaryLenExpr()->Emit2FEExpr(stmts), *funcDecl, *this);
    if (realLenExpr == nullptr) {
      continue;
    }
    UniqueFEIRExpr argExpr = args[i]->Emit2FEExpr(nullStmts);
    UniqueFEIRExpr baseExpr = ENCChecker::FindBaseExprInPointerOperation(argExpr);
    if (baseExpr == nullptr) {
      continue;
    }
    std::list<UniqueFEIRExpr> exprs;
    realLenExpr = FEIRBuilder::CreateExprBinary(OP_add, std::move(argExpr), std::move(realLenExpr));
    exprs.emplace_back(std::move(realLenExpr));
    exprs.emplace_back(std::move(baseExpr));
    UniqueFEIRStmt stmt = std::make_unique<FEIRStmtCallAssertBoundary>(OP_callassertle, std::move(exprs),
                                                                       GetFuncName(), i);
    stmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
    stmts.emplace_back(std::move(stmt));
  }
}

void ASTCallExpr::InsertBoundaryVarInRet(std::list<UniqueFEIRStmt> &stmts) const {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic() || !IsNeedRetExpr() ||
      funcDecl == nullptr || funcDecl->GetBoundaryLenExpr() == nullptr) {
    return;
  }
  UniqueFEIRExpr realLenExpr = ENCChecker::GetRealBoundaryLenExpr(
      funcDecl->GetBoundaryLenExpr()->Emit2FEExpr(stmts), *funcDecl, *this);
  if (realLenExpr == nullptr) {
    return;
  }
  // GetCurrentFunction need to be optimized when parallel features
  MIRFunction *curFunction = FEManager::GetMIRBuilder().GetCurrentFunctionNotNull();
  ENCChecker::InitBoundaryVar(*curFunction, varName, *retType, std::move(realLenExpr), stmts);
}
}
