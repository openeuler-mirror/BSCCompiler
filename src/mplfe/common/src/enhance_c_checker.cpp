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

void ENCChecker::CheckNonnullGlobalVarInit(const MIRSymbol &sym, const MIRConst *cst) {
  if (!FEOptions::GetInstance().IsNpeCheckDynamic() || !sym.GetAttr(ATTR_nonnull)) {
    return;
  }
  if (cst == nullptr) {
    ERR(kLncErr, "%s:%d error: nonnull parameter is uninitialized when defined",
        FEManager::GetModule().GetFileNameFromFileNum(sym.GetSrcPosition().FileNum()).c_str(),
        sym.GetSrcPosition().LineNum());
    return;
  }
  if (cst->IsZero()) {
    ERR(kLncErr, "%s:%d error: nullable pointer assignment of nonnull pointer",
        FEManager::GetModule().GetFileNameFromFileNum(sym.GetSrcPosition().FileNum()).c_str(),
        sym.GetSrcPosition().LineNum());
    return;
  }
}

void ENCChecker::CheckNonnullLocalVarInit(const MIRSymbol &sym, const ASTExpr *initExpr,
                                          std::list<UniqueFEIRStmt> &stmts) {
  if (!FEOptions::GetInstance().IsNpeCheckDynamic() || !sym.GetAttr(ATTR_nonnull)) {
    return;
  }
  std::list<UniqueFEIRStmt> nullStmts;
  UniqueFEIRExpr initFeirExpr = nullptr;
  if (initExpr != nullptr) {
    initFeirExpr = initExpr->Emit2FEExpr(nullStmts);
  }
  if (initFeirExpr == nullptr) {
    ERR(kLncErr, "%s:%d error: nonnull parameter is uninitialized when defined",
        FEManager::GetModule().GetFileNameFromFileNum(sym.GetSrcPosition().FileNum()).c_str(),
        sym.GetSrcPosition().LineNum());
    return;
  }
  UniqueFEIRExpr tmpExpr = initFeirExpr->Clone();
  while (tmpExpr->GetKind() == kExprUnary) {
    FEIRExprUnary *cvtExpr = static_cast<FEIRExprUnary*>(tmpExpr.get());
    if (cvtExpr != nullptr) {
      tmpExpr = cvtExpr->GetOpnd()->Clone();
    }
  }
  if (tmpExpr->GetKind() == kExprConst && static_cast<FEIRExprConst*>(tmpExpr.get())->GetValue().u64 == 0) {
    ERR(kLncErr, "%s:%d error: nullable pointer assignment of nonnull pointer",
        FEManager::GetModule().GetFileNameFromFileNum(sym.GetSrcPosition().FileNum()).c_str(),
        sym.GetSrcPosition().LineNum());
    return;
  }
  if ((tmpExpr->GetKind() == kExprDRead || tmpExpr->GetKind() == kExprIRead) && tmpExpr->GetPrimType() == PTY_ptr) {
    UniqueFEIRStmt stmt = std::make_unique<FEIRStmtUseOnly>(OP_assignassertnonnull, std::move(tmpExpr));
    stmt->SetSrcFileInfo(sym.GetSrcPosition().FileNum(), sym.GetSrcPosition().LineNum());
    stmts.emplace_back(std::move(stmt));
  }
}

// ---------------------------
// process boundary attr
// ---------------------------
void ASTParser::ProcessBoundaryFuncAttrs(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl,
                                         ASTFunc &astFunc) {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic()) {
    return;
  }
  for (const auto *returnsCountAttr : funcDecl.specific_attrs<clang::ReturnsCountAttr>()) {
    clang::Expr *expr = returnsCountAttr->getLenExpr();
    ASTExpr *lenExpr = ProcessExpr(allocator, expr);
    if (lenExpr == nullptr) {
      continue;
    }
    ProcessBoundaryLenExprInFunc(allocator, funcDecl, static_cast<unsigned int>(-1), astFunc, lenExpr, true);
  }
  for (const auto *countAttr : funcDecl.specific_attrs<clang::CountAttr>()) {
    clang::Expr *expr = countAttr->getLenExpr();
    ASTExpr *lenExpr = ProcessExpr(allocator, expr);
    if (lenExpr == nullptr) {
      continue;
    }
    if (!countAttr->index_size()) {
      // Lack of attribute index parameters means that only one pointer parameter is
      // implicitly marked as boundary var in func.
      for (unsigned int i = 0; i < astFunc.GetParamDecls().size(); ++i) {
        if (astFunc.GetParamDecls()[i]->GetTypeDesc().front()->IsMIRPtrType()) {
          ProcessBoundaryLenExprInFunc(allocator, funcDecl, i, astFunc, lenExpr, true);
        }
      }
      break;
    }
    for (const clang::ParamIdx &paramIdx : countAttr->index()) {
      // The clang ensures that the attribute only applies to pointer parameter
      unsigned int idx = paramIdx.getASTIndex();
      if (idx >= astFunc.GetParamDecls().size()) {
        continue;
      }
      ProcessBoundaryLenExprInFunc(allocator, funcDecl, idx, astFunc, lenExpr, true);
    }
  }
}

void ASTParser::ProcessBoundaryParamAttrs(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl,
                                          ASTFunc &astFunc) {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic()) {
    return;
  }
  for (unsigned int i = 0; i < funcDecl.getNumParams(); ++i) {
    const clang::ParmVarDecl *parmDecl = funcDecl.getParamDecl(i);
    if (parmDecl->getKind() == clang::Decl::Function) {
      continue;
    }
    for (const auto *countAttr : parmDecl->specific_attrs<clang::CountAttr>()) {
      clang::Expr *expr = countAttr->getLenExpr();
      ASTExpr *lenExpr = ProcessExpr(allocator, expr);
      if (lenExpr == nullptr) {
        continue;
      }
      ProcessBoundaryLenExprInFunc(allocator, funcDecl, i, astFunc, lenExpr, true);
    }
  }
}

void ASTParser::ProcessBoundaryVarAttrs(MapleAllocator &allocator, const clang::VarDecl &varDecl, ASTVar &astVar) {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic()) {
    return;
  }
  for (const auto *countAttr : varDecl.specific_attrs<clang::CountAttr>()) {
    clang::Expr *expr = countAttr->getLenExpr();
    ASTExpr *lenExpr = ProcessExpr(allocator, expr);
    if (lenExpr == nullptr) {
      continue;
    }
    ProcessBoundaryLenExprInVar(allocator, astVar, varDecl.getType(), lenExpr, true);
  }
}

void ASTParser::ProcessBoundaryFieldAttrs(MapleAllocator &allocator, const ASTStruct &structDecl,
                                          const clang::RecordDecl &recDecl) {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic()) {
    return;
  }
  const auto *declContext = llvm::dyn_cast<clang::DeclContext>(&recDecl);
  if (declContext == nullptr) {
    return;
  }
  for (auto *loadDecl : declContext->decls()) {
    if (loadDecl == nullptr) {
      continue;
    }
    auto *fieldDecl = llvm::dyn_cast<clang::FieldDecl>(loadDecl);
    if (fieldDecl == nullptr) {
      continue;
    }
    ASTDecl *astField = ASTDeclsBuilder::GetASTDecl(fieldDecl->getID());
    if (astField == nullptr) {
      continue;
    }
    for (const auto *countAttr : fieldDecl->specific_attrs<clang::CountAttr>()) {
      clang::Expr *expr = countAttr->getLenExpr();
      ASTExpr *lenExpr = ProcessExpr(allocator, expr);
      if (lenExpr == nullptr) {
        continue;
      }
      ProcessBoundaryLenExprInField(allocator, *astField, structDecl, fieldDecl->getType(), lenExpr, true);
    }
  }
}

// ---------------------------
// process boundary length expr in attr
// ---------------------------
void ASTParser::ProcessBoundaryLenExpr(MapleAllocator &allocator, ASTDecl &ptrDecl, const clang::QualType &qualType,
                                       const std::function<ASTExpr* ()> &getLenExprFromStringLiteral,
                                       ASTExpr *lenExpr, bool isSize) {
  if (!qualType->isPointerType()) {
    ERR(kLncErr, "%s:%d EnhanceC error: The variable modified by the boundary attribute should be a pointer type",
        FEManager::GetModule().GetFileNameFromFileNum(lenExpr->GetSrcFileIdx()).c_str(), lenExpr->GetSrcFileLineNum());
    return;
  }
  // Check lenExpr kind from: length stringLiteral or constant value/var expression
  if (lenExpr->GetASTOp() == kASTStringLiteral) {
    // boundary length stringLiteral -> real length decl expr
    lenExpr = getLenExprFromStringLiteral();
    if (lenExpr == nullptr) {
      return;
    }
  } else if (lenExpr->GetType() == nullptr || !FEUtils::IsInteger(lenExpr->GetType()->GetPrimType())) {
    ERR(kLncErr, "%s:%d EnhanceC error: The boundary length expr is not an integer type",
        FEManager::GetModule().GetFileNameFromFileNum(lenExpr->GetSrcFileIdx()).c_str(), lenExpr->GetSrcFileLineNum());
    return;
  }
  if (isSize) {
    // The type size can only be obtained from ClangDecl instead of ASTDecl,
    // because the field of mir struct type has not yet been initialized at this time
    uint64 lenSize = GetSizeFromQualType(qualType->getPointeeType());
    MIRType *pointedType = static_cast<MIRPtrType*>(ptrDecl.GetTypeDesc().front())->GetPointedType();
    if (pointedType->GetPrimType() == PTY_f64) {
      lenSize = 8; // 8 is f64 byte num, because now f128 also cvt to f64
    }
    lenExpr = GetAddrShiftExpr(allocator, lenExpr, lenSize);
  }
  ptrDecl.SetBoundaryLenExpr(lenExpr);
}

void ASTParser::ProcessBoundaryLenExprInFunc(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl,
                                             unsigned int idx, ASTFunc &astFunc, ASTExpr *lenExpr, bool isSize) {
  ASTDecl *ptrDecl = nullptr;
  clang::QualType qualType;
  if (idx == static_cast<unsigned int>(-1)) {  // return boundary attr
    ptrDecl = &astFunc;
    qualType = funcDecl.getReturnType();
  } else if (idx < astFunc.GetParamDecls().size()) {
    ptrDecl = astFunc.GetParamDecls()[idx];
    qualType = funcDecl.getParamDecl(idx)->getType();
  } else {
    ERR(kLncErr, "%s:%d EnhanceC error: The parameter annotated boundary attr [idx:%d] is not found"
        "in the function [%s]", FEManager::GetModule().GetFileNameFromFileNum(lenExpr->GetSrcFileIdx()).c_str(),
        lenExpr->GetSrcFileLineNum(), idx, astFunc.GetName().c_str());
    return;
  }
  // parameter stringLiteral -> real parameter decl
  auto getLenExprFromStringLiteral = [&]() -> ASTExpr* {
    ASTStringLiteral *strExpr = static_cast<ASTStringLiteral*>(lenExpr);
    std::string lenName(strExpr->GetCodeUnits().begin(), strExpr->GetCodeUnits().end());
    for (ASTDecl *lenDecl : astFunc.GetParamDecls()) {
      if (lenDecl->GetName() != lenName) {
        continue;
      }
      MIRType *lenType = lenDecl->GetTypeDesc().front();
      if (lenType == nullptr || !FEUtils::IsInteger(lenType->GetPrimType())) {
        ERR(kLncErr, "%s:%d EnhanceC error: The parameter [%s] specified as boundary length var is not an integer type "
            "in the function [%s]", FEManager::GetModule().GetFileNameFromFileNum(lenExpr->GetSrcFileIdx()).c_str(),
            lenExpr->GetSrcFileLineNum(), lenName.c_str(), astFunc.GetName().c_str());
        return nullptr;
      }
      ASTDeclRefExpr *lenRefExpr = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
      lenRefExpr->SetASTDecl(lenDecl);
      lenRefExpr->SetType(lenDecl->GetTypeDesc().front());
      return lenRefExpr;
    }
    ERR(kLncErr, "%s:%d EnhanceC error: The parameter [%s] specified as boundary length var is not found "
        "in the function [%s]", FEManager::GetModule().GetFileNameFromFileNum(lenExpr->GetSrcFileIdx()).c_str(),
        lenExpr->GetSrcFileLineNum(), lenName.c_str(), astFunc.GetName().c_str());
    return nullptr;
  };
  ProcessBoundaryLenExpr(allocator, *ptrDecl, qualType, getLenExprFromStringLiteral, lenExpr, isSize);
}

void ASTParser::ProcessBoundaryLenExprInVar(MapleAllocator &allocator, ASTDecl &ptrDecl,
                                            const clang::QualType &qualType, ASTExpr *lenExpr, bool isSize) {
  // The StringLiteral is not allowed to use as boundary length of var
  auto getLenExprFromStringLiteral = [&]() -> ASTExpr* {
    ERR(kLncErr, "%s:%d EnhanceC error: The StringLiteral is not allowed to use as boundary length of var",
        FEManager::GetModule().GetFileNameFromFileNum(lenExpr->GetSrcFileIdx()).c_str(), lenExpr->GetSrcFileLineNum());
    return nullptr;
  };
  ProcessBoundaryLenExpr(allocator, ptrDecl, qualType, getLenExprFromStringLiteral, lenExpr, isSize);
}

void ASTParser::ProcessBoundaryLenExprInField(MapleAllocator &allocator, ASTDecl &ptrDecl, const ASTStruct &structDecl,
                                              const clang::QualType &qualType, ASTExpr *lenExpr, bool isSize) {
  // boundary length stringLiteral in field -> real field decl
  auto getLenExprFromStringLiteral = [&]() -> ASTExpr* {
    ASTStringLiteral *strExpr = static_cast<ASTStringLiteral*>(lenExpr);
    std::string lenName(strExpr->GetCodeUnits().begin(), strExpr->GetCodeUnits().end());
    for (ASTField *fieldDecl: structDecl.GetFields()) {
      if (lenName != fieldDecl->GetName()) {
        continue;
      }
      MIRType *lenType = fieldDecl->GetTypeDesc().front();
      if (lenType == nullptr || !FEUtils::IsInteger(lenType->GetPrimType())) {
        ERR(kLncErr, "%s:%d EnhanceC error: The field [%s] specified as boundary length var is not an integer type "
            "in the struct [%s]", FEManager::GetModule().GetFileNameFromFileNum(lenExpr->GetSrcFileIdx()).c_str(),
            lenExpr->GetSrcFileLineNum(), lenName.c_str(), structDecl.GetName().c_str());
        return nullptr;
      }
      ASTDeclRefExpr *lenRefExpr = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
      lenRefExpr->SetASTDecl(fieldDecl);
      lenRefExpr->SetType(fieldDecl->GetTypeDesc().front());
      return lenRefExpr;
    }
    ERR(kLncErr, "%s:%d EnhanceC error: The StringLiteral [%s] as boundary length var is not found "
        "in the struct [%s]", FEManager::GetModule().GetFileNameFromFileNum(lenExpr->GetSrcFileIdx()).c_str(),
        lenExpr->GetSrcFileLineNum(), lenName.c_str(), structDecl.GetName().c_str());
    return nullptr;
  };
  ProcessBoundaryLenExpr(allocator, ptrDecl, qualType, getLenExprFromStringLiteral, lenExpr, isSize);
}

// ---------------------------
// process boundary var
// ---------------------------
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
  if (expr->GetKind() == kExprUnary) {
    FEIRExprUnary *cvtExpr = static_cast<FEIRExprUnary*>(expr.get());
    UniqueFEIRExpr baseExpr = FindBaseExprInPointerOperation(cvtExpr->GetOpnd());
  } else if (expr->GetKind() == kExprAddrofArray) {
    FEIRExprAddrofArray *arrExpr = static_cast<FEIRExprAddrofArray*>(expr.get());
    baseExpr = FindBaseExprInPointerOperation(arrExpr->GetExprArray());
  } else if ((expr->GetKind() == kExprDRead && expr->GetPrimType() == PTY_ptr) ||
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
                                   const UniqueFEIRExpr &lRealLenExpr, std::list<StmtNode*> &ans) {
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
  UniqueFEIRExpr baseExpr = FindBaseExprInPointerOperation(srcExpr);
  if (baseExpr == nullptr) {
    return;
  }
  // Check if the l-value has a boundary var
  std::pair<StIdx, StIdx> lBoundaryVarStIdx = std::make_pair(StIdx(0), StIdx(0));
  auto lIt = curFunction->GetBoundaryMap().find(dstExpr->Hash());
  if (lIt != curFunction->GetBoundaryMap().end()) {  // The boundary var exists on the l-value
    lBoundaryVarStIdx = lIt->second;
  } else {
    if (lRealLenExpr != nullptr) {  // init boundary in func body if a field/global var l-value with boundary attr
      std::list<UniqueFEIRStmt> stmts;
      std::list<StmtNode*> stmtNodes;
      lBoundaryVarStIdx = InitBoundaryVar(*curFunction, dstExpr, lRealLenExpr->Clone(), stmts);
      for (const auto &stmt : stmts) {
        std::list<StmtNode*> res = stmt->GenMIRStmts(mirBuilder);
        stmtNodes.splice(stmtNodes.end(), res);
      }
      for (auto stmtNode : stmtNodes) {
        MIRFunction *curFunction = mirBuilder.GetCurrentFunctionNotNull();
        curFunction->GetBody()->InsertFirst(stmtNode);
      }
    }
  }
  // Check if the r-value has a boundary var or an array or a global var/field with boundary attr
  std::pair<StIdx, StIdx> rBoundaryVarStIdx = std::make_pair(StIdx(0), StIdx(0));
  MIRType *arrType = nullptr;
  UniqueFEIRExpr rRealLenExpr = nullptr;
  auto rIt = curFunction->GetBoundaryMap().find(baseExpr->Hash());
  if (rIt != curFunction->GetBoundaryMap().end()) {  // Assgin when the boundary var exists on the r-value
    rBoundaryVarStIdx = rIt->second;
  } else {
    arrType = GetArrayTypeFromExpr(baseExpr);
    if (arrType == nullptr) {
      rRealLenExpr = GetGlobalOrFieldLenExprInExpr(mirBuilder, baseExpr);
      if (rRealLenExpr == nullptr && lBoundaryVarStIdx.first != StIdx(0)) {
        // Insert a empty r-value boundary var
        // when there is a l-value boundary var and r-value without a boundary var, or an array,
        // or a global var/field r-value with boundary attr
        rBoundaryVarStIdx = ENCChecker::InsertBoundaryVar(mirBuilder, baseExpr);
      }
    }
  }
  // insert L-value bounary and assign boundary var
  // when r-value with a boundary var, or with a sized array, a global var/field r-value with boundary attr
  bool isSizedArray = (arrType != nullptr && arrType->GetSize() > 0);
  if (lBoundaryVarStIdx.first == StIdx(0) &&
      (rBoundaryVarStIdx.first != StIdx(0) || isSizedArray || rRealLenExpr != nullptr)) {
    lBoundaryVarStIdx = ENCChecker::InsertBoundaryVar(mirBuilder, dstExpr);
  }
  if (lBoundaryVarStIdx.first != StIdx(0)) {
    MIRSymbol *lLowerSym = curFunction->GetLocalOrGlobalSymbol(lBoundaryVarStIdx.first);
    CHECK_NULL_FATAL(lLowerSym);
    MIRSymbol *lUpperSym = curFunction->GetLocalOrGlobalSymbol(lBoundaryVarStIdx.second);
    CHECK_NULL_FATAL(lUpperSym);
    StmtNode *lowerStmt = nullptr;
    StmtNode *upperStmt = nullptr;
    if (rBoundaryVarStIdx.first != StIdx(0)) {
      MIRSymbol *rLowerSym = curFunction->GetLocalOrGlobalSymbol(rBoundaryVarStIdx.first);
      CHECK_NULL_FATAL(rLowerSym);
      lowerStmt = mirBuilder.CreateStmtDassign(*lLowerSym, 0, mirBuilder.CreateExprDread(*rLowerSym));

      MIRSymbol *rUpperSym = curFunction->GetLocalOrGlobalSymbol(rBoundaryVarStIdx.second);
      CHECK_NULL_FATAL(rUpperSym);
      upperStmt = mirBuilder.CreateStmtDassign(*lUpperSym, 0, mirBuilder.CreateExprDread(*rUpperSym));
    } else if (isSizedArray) {
      lowerStmt = mirBuilder.CreateStmtDassign(*lLowerSym, 0, baseExpr->GenMIRNode(mirBuilder));
      UniqueFEIRExpr binExpr = FEIRBuilder::CreateExprBinary(
          OP_add, baseExpr->Clone(), std::make_unique<FEIRExprConst>(arrType->GetSize(), PTY_ptr));
      upperStmt = mirBuilder.CreateStmtDassign(*lUpperSym, 0, binExpr->GenMIRNode(mirBuilder));
    } else if (rRealLenExpr != nullptr) {
      lowerStmt = mirBuilder.CreateStmtDassign(*lLowerSym, 0, baseExpr->GenMIRNode(mirBuilder));
      UniqueFEIRExpr binExpr = FEIRBuilder::CreateExprBinary(OP_add, baseExpr->Clone(), rRealLenExpr->Clone());
      upperStmt = mirBuilder.CreateStmtDassign(*lUpperSym, 0, binExpr->GenMIRNode(mirBuilder));
    }
    if (lowerStmt == nullptr || upperStmt == nullptr) {
      return;
    }
    if (lRealLenExpr != nullptr) {  // use l-vaule own boundary(use r-value base) when l-value has a boundary attr
      BaseNode *binExpr = mirBuilder.CreateExprBinary(
          OP_add, *lLowerSym->GetType(), mirBuilder.CreateExprDread(*lLowerSym), lRealLenExpr->GenMIRNode(mirBuilder));
      upperStmt = mirBuilder.CreateStmtDassign(*lUpperSym, 0, binExpr);
    }
    ans.emplace_back(lowerStmt);
    ans.emplace_back(upperStmt);
  }
}

bool ENCChecker::IsGlobalVarInExpr(const UniqueFEIRExpr &expr) {
  bool isGlobal = false;
  auto vars = expr->GetVarUses();
  if (!vars.empty() && vars.front() != nullptr) {
    isGlobal = vars.front()->IsGlobal();
  }
  return isGlobal;
}

std::pair<StIdx, StIdx> ENCChecker::InsertBoundaryVar(MIRBuilder &mirBuilder, const UniqueFEIRExpr &expr) {
  std::pair<StIdx, StIdx> boundaryVarStIdx = std::make_pair(StIdx(0), StIdx(0));
  std::string boundaryName = GetBoundaryName(expr);
  if (boundaryName.empty()) {
    return boundaryVarStIdx;
  }
  MIRSymbol *lowerSrcSym = nullptr;
  MIRSymbol *upperSrcSym = nullptr;
  MIRType *boundaryType = expr->GetType()->GenerateMIRTypeAuto();
  if (IsGlobalVarInExpr(expr)) {
    lowerSrcSym = mirBuilder.GetOrCreateGlobalDecl(boundaryName + ".lower", *boundaryType);
    upperSrcSym = mirBuilder.GetOrCreateGlobalDecl(boundaryName + ".upper", *boundaryType);
  } else {
    lowerSrcSym = mirBuilder.GetOrCreateLocalDecl(boundaryName + ".lower", *boundaryType);
    upperSrcSym = mirBuilder.GetOrCreateLocalDecl(boundaryName + ".upper", *boundaryType);
  }
  // assign undef val to boundary var in func body head
  AssignUndefVal(mirBuilder, *upperSrcSym);
  AssignUndefVal(mirBuilder, *lowerSrcSym);
  // update BoundaryMap
  boundaryVarStIdx = std::make_pair(lowerSrcSym->GetStIdx(), upperSrcSym->GetStIdx());
  MIRFunction *curFunction = mirBuilder.GetCurrentFunctionNotNull();
  curFunction->SetBoundaryMap(expr->Hash(), boundaryVarStIdx);
  return boundaryVarStIdx;
}

std::string ENCChecker::GetBoundaryName(const UniqueFEIRExpr &expr) {
  std::string boundaryName;
  if (expr == nullptr) {
    return boundaryName;
  }
  if (expr->GetKind() == kExprDRead) {
    const FEIRExprDRead *dread = static_cast<FEIRExprDRead*>(expr.get());
    // Naming format for var boundary: _boundary.[varname]_[fieldID].[exprHash].lower/upper
    boundaryName = "_boundary." + dread->GetVar()->GetNameRaw() + "." + std::to_string(expr->GetFieldID()) + "." +
                   std::to_string(expr->Hash());
  } else if (expr->GetKind() == kExprIRead) {
    const FEIRExprIRead *iread = static_cast<FEIRExprIRead*>(expr.get());
    MIRType *pointerType = iread->GetClonedPtrType()->GenerateMIRTypeAuto();
    CHECK_FATAL(pointerType->IsMIRPtrType(), "Must be ptr type!");
    MIRType *structType = static_cast<MIRPtrType*>(pointerType)->GetPointedType();
    std::string structName = GlobalTables::GetStrTable().GetStringFromStrIdx(structType->GetNameStrIdx());
    FieldID fieldID = iread->GetFieldID();
    FieldPair rFieldPair = static_cast<MIRStructType*>(structType)->TraverseToFieldRef(fieldID);
    std::string fieldName = GlobalTables::GetStrTable().GetStringFromStrIdx(rFieldPair.first);
    // Naming format for field var boundary: _boundary.[sturctname]_[fieldname].[exprHash].lower/upper
    boundaryName = "_boundary." + structName + "_" + fieldName + "." + std::to_string(iread->Hash());
  }
  return boundaryName;
}

void ENCChecker::AssignUndefVal(MIRBuilder &mirBuilder, MIRSymbol &sym) {
  if (sym.IsGlobal()) {
    MIRIntConst *cst = FEManager::GetModule().GetMemPool()->New<MIRIntConst>(
        0xdeadbeef, *GlobalTables::GetTypeTable().GetPrimType(PTY_ptr));
    sym.SetKonst(cst);
  } else {
    BaseNode *undef = mirBuilder.CreateIntConst(0xdeadbeef, PTY_ptr);
    StmtNode *assign = mirBuilder.CreateStmtDassign(sym, 0, undef);
    MIRFunction *curFunction = mirBuilder.GetCurrentFunctionNotNull();
    curFunction->GetBody()->InsertFirst(assign);
  }
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

void ENCChecker::InitBoundaryVar(MIRFunction &curFunction, const ASTDecl &ptrDecl,
                                 UniqueFEIRExpr lenExpr, std::list<UniqueFEIRStmt> &stmts) {
  std::string ptrName = ptrDecl.GenerateUniqueVarName();
  MIRType *ptrType = ptrDecl.GetTypeDesc().front();
  UniqueFEIRExpr ptrExpr = FEIRBuilder::CreateExprDRead(FEIRBuilder::CreateVarNameForC(ptrName, *ptrType));
  // insert lower boundary stmt
  std::string lowerVarName = "_boundary." + ptrName + ".lower";
  UniqueFEIRVar lowerVar = FEIRBuilder::CreateVarNameForC(lowerVarName, *ptrType);
  UniqueFEIRStmt lowerStmt = FEIRBuilder::CreateStmtDAssign(std::move(lowerVar), ptrExpr->Clone());
  // insert upper boundary stmt
  UniqueFEIRExpr binExpr = FEIRBuilder::CreateExprBinary(OP_add, ptrExpr->Clone(), std::move(lenExpr));
  std::string upperVarName = "_boundary." + ptrName + ".upper";
  UniqueFEIRVar upperVar = FEIRBuilder::CreateVarNameForC(upperVarName, *ptrType);
  UniqueFEIRStmt upperStmt = FEIRBuilder::CreateStmtDAssign(std::move(upperVar), std::move(binExpr));
  if (ptrDecl.GetSrcFileLineNum() != 0) {
    lowerStmt->SetSrcFileInfo(ptrDecl.GetSrcFileIdx(), ptrDecl.GetSrcFileLineNum());
    upperStmt->SetSrcFileInfo(ptrDecl.GetSrcFileIdx(), ptrDecl.GetSrcFileLineNum());
  }
  stmts.emplace_back(std::move(lowerStmt));
  stmts.emplace_back(std::move(upperStmt));
  // update BoundaryMap
  MIRSymbol *lowerSym = nullptr;
  MIRSymbol *upperSym = nullptr;
  if (ptrDecl.IsGlobal()) {
    lowerSym = FEManager::GetMIRBuilder().GetOrCreateGlobalDecl(lowerVarName, *ptrType);
    upperSym = FEManager::GetMIRBuilder().GetOrCreateGlobalDecl(upperVarName, *ptrType);
  } else {
    lowerSym = FEManager::GetMIRBuilder().GetOrCreateDeclInFunc(lowerVarName, *ptrType, curFunction);
    upperSym = FEManager::GetMIRBuilder().GetOrCreateDeclInFunc(upperVarName, *ptrType, curFunction);
  }
  curFunction.SetBoundaryMap(ptrExpr->Hash(), std::make_pair(lowerSym->GetStIdx(), upperSym->GetStIdx()));
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

std::pair<StIdx, StIdx> ENCChecker::InitBoundaryVar(MIRFunction &curFunction, const UniqueFEIRExpr &ptrExpr,
                                                    UniqueFEIRExpr lenExpr, std::list<UniqueFEIRStmt> &stmts) {
  std::string ptrName = GetBoundaryName(ptrExpr);
  if (ptrName.empty()) {
    return std::make_pair(StIdx(0), StIdx(0));
  }
  MIRType *ptrType = ptrExpr->GetType()->GenerateMIRTypeAuto();
  bool isGlobal = IsGlobalVarInExpr(ptrExpr);
  // insert lower boundary stmt
  std::string lowerVarName = ptrName + ".lower";
  UniqueFEIRVar lowerVar = FEIRBuilder::CreateVarNameForC(lowerVarName, *ptrType, isGlobal, false);
  UniqueFEIRStmt lowerStmt = FEIRBuilder::CreateStmtDAssign(std::move(lowerVar), ptrExpr->Clone());
  stmts.emplace_back(std::move(lowerStmt));
  // insert upper boundary stmt
  UniqueFEIRExpr binExpr = FEIRBuilder::CreateExprBinary(OP_add, ptrExpr->Clone(), std::move(lenExpr));
  std::string upperVarName = ptrName + ".upper";
  UniqueFEIRVar upperVar = FEIRBuilder::CreateVarNameForC(upperVarName, *ptrType, isGlobal, false);
  UniqueFEIRStmt upperStmt = FEIRBuilder::CreateStmtDAssign(std::move(upperVar), std::move(binExpr));
  stmts.emplace_back(std::move(upperStmt));
  // update BoundaryMap
  MIRSymbol *lowerSym = nullptr;
  MIRSymbol *upperSym = nullptr;
  if (isGlobal) {
    lowerSym = FEManager::GetMIRBuilder().GetOrCreateGlobalDecl(lowerVarName, *ptrType);
    upperSym = FEManager::GetMIRBuilder().GetOrCreateGlobalDecl(upperVarName, *ptrType);
  } else {
    lowerSym = FEManager::GetMIRBuilder().GetOrCreateDeclInFunc(lowerVarName, *ptrType, curFunction);
    upperSym = FEManager::GetMIRBuilder().GetOrCreateDeclInFunc(upperVarName, *ptrType, curFunction);
  }
  auto stIdxs = std::make_pair(lowerSym->GetStIdx(), upperSym->GetStIdx());
  curFunction.SetBoundaryMap(ptrExpr->Hash(), stIdxs);
  return stIdxs;
}


UniqueFEIRExpr ENCChecker::GetGlobalOrFieldLenExprInExpr(MIRBuilder &mirBuilder, const UniqueFEIRExpr &expr) {
  MIRFunction *curFunction = mirBuilder.GetCurrentFunctionNotNull();
  UniqueFEIRExpr lenExpr = nullptr;
  if (expr->GetKind() == kExprDRead && expr->GetFieldID() == 0) {
    FEIRVar *var = expr->GetVarUses().front();
    MIRSymbol *symbol = var->GenerateMIRSymbol(mirBuilder);
    if (!symbol->IsGlobal()) {
      return nullptr;
    }
    // Get the boundary attr(i.e. boundary length expr cache)
    lenExpr = GetBoundaryLenExprCache(*curFunction->GetFuncSymbol(), *symbol);
  } else if ((expr->GetKind() == kExprDRead || expr->GetKind() == kExprIRead) && expr->GetFieldID() != 0) {
    MIRStructType *structType = nullptr;
    if (expr->GetKind() == kExprDRead) {
      structType = static_cast<MIRStructType*>(expr->GetVarUses().front()->GetType()->GenerateMIRTypeAuto());
    } else {
      FEIRExprIRead *iread = static_cast<FEIRExprIRead*>(expr.get());
      MIRType *baseType = static_cast<MIRPtrType*>(iread->GetClonedPtrType()->GenerateMIRTypeAuto())->GetPointedType();
      structType = static_cast<MIRStructType*>(baseType);
    }
    FieldID tmpID = expr->GetFieldID();
    MIRStructType *lastestStruct = nullptr;
    FieldPair fieldPair = FEUtils::GetLastestStructTypeAndField(*structType, lastestStruct, tmpID);
    // Get the boundary attr(i.e. boundary length expr cache) of field
    lenExpr = GetBoundaryLenExprCache(*lastestStruct, fieldPair);
    if (lenExpr == nullptr) {
      return nullptr;
    }
    UniqueFEIRExpr realExpr = ENCChecker::GetRealBoundaryLenExprInField(
        lenExpr->Clone(), *structType, expr);  // lenExpr needs to be cloned
    if (realExpr != nullptr) {
      lenExpr = std::move(realExpr);
    }
  } else {
    return nullptr;
  }
  return lenExpr;
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

UniqueFEIRExpr ENCChecker::GetRealBoundaryLenExprInFunc(const UniqueFEIRExpr &lenExpr, const ASTFunc &astFunc,
                                                        const ASTCallExpr &astCallExpr) {
  if (lenExpr == nullptr) {
    return nullptr;
  }
  if (lenExpr->GetKind() == kExprBinary) {
    FEIRExprBinary *mulExpr = static_cast<FEIRExprBinary*>(lenExpr.get());
    UniqueFEIRExpr leftExpr = GetRealBoundaryLenExprInFunc(mulExpr->GetOpnd0(), astFunc, astCallExpr);
    if (leftExpr != nullptr) {
      mulExpr->SetOpnd0(std::move(leftExpr));
    } else {
      return nullptr;
    }
    UniqueFEIRExpr rightExpr = GetRealBoundaryLenExprInFunc(mulExpr->GetOpnd1(), astFunc, astCallExpr);
    if (rightExpr != nullptr) {
      mulExpr->SetOpnd1(std::move(rightExpr));
    } else {
      return nullptr;
    }
  }
  if (lenExpr->GetKind() == kExprUnary) {
    FEIRExprUnary *cvtExpr = static_cast<FEIRExprUnary*>(lenExpr.get());
    UniqueFEIRExpr subExpr = GetRealBoundaryLenExprInFunc(cvtExpr->GetOpnd(), astFunc, astCallExpr);
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

UniqueFEIRExpr ENCChecker::GetRealBoundaryLenExprInField(const UniqueFEIRExpr &lenExpr, MIRStructType &baseType,
                                                         const UniqueFEIRExpr &dstExpr) {
  if (lenExpr == nullptr) {
    return nullptr;
  }
  if (lenExpr->GetKind() == kExprBinary) {
    FEIRExprBinary *mulExpr = static_cast<FEIRExprBinary*>(lenExpr.get());
    UniqueFEIRExpr leftExpr = GetRealBoundaryLenExprInField(mulExpr->GetOpnd0(), baseType, dstExpr);
    if (leftExpr != nullptr) {
      mulExpr->SetOpnd0(std::move(leftExpr));
    } else {
      return nullptr;
    }
    UniqueFEIRExpr rightExpr = GetRealBoundaryLenExprInField(mulExpr->GetOpnd1(), baseType, dstExpr);
    if (rightExpr != nullptr) {
      mulExpr->SetOpnd1(std::move(rightExpr));
    } else {
      return nullptr;
    }
  }
  if (lenExpr->GetKind() == kExprUnary) {
    FEIRExprUnary *cvtExpr = static_cast<FEIRExprUnary*>(lenExpr.get());
    UniqueFEIRExpr subExpr = GetRealBoundaryLenExprInField(cvtExpr->GetOpnd(), baseType, dstExpr);
    if (subExpr != nullptr) {
      cvtExpr->SetOpnd(std::move(subExpr));
    } else {
      return nullptr;
    }
  }
  // boundary length expr -> actual dread/iread length field expr
  if (lenExpr->GetKind() == kExprDRead) {
    std::string lenName = lenExpr->GetVarUses().front()->GetNameRaw();
    uint32 fieldID = 0;
    bool flag = FEManager::GetMIRBuilder().TraverseToNamedField(
        baseType, GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(lenName), fieldID);
    if (!flag) {
      return nullptr;
    }
    MIRType *reType = FEUtils::GetStructFieldType(&baseType, fieldID);
    UniqueFEIRType reFEType = FEIRTypeHelper::CreateTypeNative(*reType);
    if (dstExpr->GetKind() == kExprDRead) {
      return FEIRBuilder::CreateExprDReadAggField(dstExpr->GetVarUses().front()->Clone(), fieldID, std::move(reFEType));
    } else if (dstExpr->GetKind() == kExprIRead) {
      FEIRExprIRead *iread = static_cast<FEIRExprIRead*>(dstExpr.get());
      return FEIRBuilder::CreateExprIRead(
          std::move(reFEType), iread->GetClonedPtrType(), iread->GetClonedOpnd(), fieldID);
    } else {
      return nullptr;
    }
  }
  return lenExpr->Clone();
}

// ---------------------------
// process boundary var and checking for ast func arg and return
// ---------------------------
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
    ENCChecker::InitBoundaryVar(mirFunc, *paramDecl, std::move(lenFEExpr), stmts);
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
  lenExpr = FEIRBuilder::CreateExprBinary(OP_add, baseExpr->Clone(), std::move(lenExpr));
  exprs.emplace_back(std::move(lenExpr));
  exprs.emplace_back(std::move(baseExpr));
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtNary>(OP_returnassertle, std::move(exprs));
  stmt->SetSrcFileInfo(stmts.back()->GetSrcFileIdx(), stmts.back()->GetSrcFileLineNum());
  stmts.insert(--stmts.end(), std::move(stmt));
}

void ENCChecker::InsertBoundaryAssignChecking(MIRBuilder &mirBuilder, std::list<StmtNode*> &ans,
                                              const UniqueFEIRExpr &srcExpr, uint32 fileIdx, uint32 fileLine) {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic() || srcExpr->GetPrimType() != PTY_ptr ||
      srcExpr->GetKind() != kExprBinary) {  // pointer computed assignment
    return;
  }
  UniqueFEIRExpr baseExpr = FindBaseExprInPointerOperation(srcExpr);
  if (baseExpr == nullptr) {
    return;
  }
  // insert l-value lower boundary chencking
  std::list<UniqueFEIRExpr> lowerExprs;
  lowerExprs.emplace_back(srcExpr->Clone());
  lowerExprs.emplace_back(baseExpr->Clone());
  UniqueFEIRStmt lowerStmt = std::make_unique<FEIRStmtNary>(OP_assertge, std::move(lowerExprs));
  lowerStmt->SetSrcFileInfo(fileIdx, fileLine);
  std::list<StmtNode*> lowerStmts = lowerStmt->GenMIRStmts(mirBuilder);
  ans.splice(ans.end(), lowerStmts);
  // insert l-value upper boundary chencking
  std::list<UniqueFEIRExpr> upperExprs;
  upperExprs.emplace_back(srcExpr->Clone());
  upperExprs.emplace_back(baseExpr->Clone());
  UniqueFEIRStmt upperStmt = std::make_unique<FEIRStmtNary>(OP_assertlt, std::move(upperExprs));
  upperStmt->SetSrcFileInfo(fileIdx, fileLine);
  std::list<StmtNode*> upperStmts = upperStmt->GenMIRStmts(mirBuilder);
  ans.splice(ans.end(), upperStmts);
}

UniqueFEIRStmt ENCChecker::InsertBoundaryLEChecking(UniqueFEIRExpr lenExpr, const UniqueFEIRExpr &srcExpr) {
  UniqueFEIRExpr baseExpr = ENCChecker::FindBaseExprInPointerOperation(srcExpr);
  if (baseExpr == nullptr) {
    return nullptr;
  }
  UniqueFEIRExpr boundaryExpr = FEIRBuilder::CreateExprBinary(  // use r-value base
      OP_add, baseExpr->Clone(), std::move(lenExpr));
  std::list<UniqueFEIRExpr> exprs;
  exprs.emplace_back(std::move(boundaryExpr));
  exprs.emplace_back(std::move(baseExpr));
  return std::make_unique<FEIRStmtNary>(OP_assignassertle, std::move(exprs));
}

void ENCChecker::SaveBoundaryLenExprCache(const std::pair<StIdx, StIdx> &key, ASTExpr *astLenExpr) {
  if (astLenExpr == nullptr) {
    return;
  }
  std::list<UniqueFEIRStmt> nullStmts;
  UniqueFEIRExpr lenExpr = astLenExpr->Emit2FEExpr(nullStmts);
  FEManager::GetTypeManager().InsertVarBoundaryLenExprMap(key, std::move(lenExpr));  // save var lenExpr cache
}

void ENCChecker::SaveBoundaryLenExprCache(const std::pair<TyIdx, FieldPair> &key, ASTExpr *astLenExpr) {
  if (astLenExpr == nullptr) {
    return;
  }
  std::list<UniqueFEIRStmt> nullStmts;
  UniqueFEIRExpr lenExpr = astLenExpr->Emit2FEExpr(nullStmts);
  FEManager::GetTypeManager().InsertFieldBoundaryLenExprMap(key, std::move(lenExpr));  // save field lenExpr cache
}

void ENCChecker::SaveBoundaryLenExprCache(const std::pair<StIdx, StIdx> &key, const UniqueFEIRExpr &expr) {
  if (expr == nullptr) {
    return;
  }
  FEManager::GetTypeManager().InsertVarBoundaryLenExprMap(key, expr->Clone());
}


UniqueFEIRExpr ENCChecker::GetBoundaryLenExprCache(const MIRSymbol &funcSym, const MIRSymbol &varSym) {
  if (varSym.IsGlobal()) {
    return FEManager::GetTypeManager().GetVarBoundaryLenExprFromMap(std::make_pair(StIdx(0), varSym.GetStIdx()));
  } else {
    return FEManager::GetTypeManager().GetVarBoundaryLenExprFromMap(
        std::make_pair(funcSym.GetStIdx(), varSym.GetStIdx()));
  }
}

UniqueFEIRExpr ENCChecker::GetBoundaryLenExprCache(const MIRStructType &type, const FieldPair &fieldPair) {
  return FEManager::GetTypeManager().GetFieldBoundaryLenExprFromMap(std::make_pair(type.GetTypeIndex(), fieldPair));
}

// ---------------------------
// process boundary var and checking in stmt of ast func
// ---------------------------
void FEIRStmtDAssign::AssignBoundaryVarAndChecking(MIRBuilder &mirBuilder, std::list<StmtNode*> &ans) const {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic() || expr->GetPrimType() != PTY_ptr) {
    return;
  }
  const std::string prefix = "_boundary.";
  if (var->GetNameRaw().compare(0, prefix.size(), prefix) == 0) {
    return;
  }
  // insert assign boundary checking for computed r-value
  ENCChecker::InsertBoundaryAssignChecking(mirBuilder, ans, expr, srcFileIndex, srcFileLineNum);

  UniqueFEIRExpr dstExpr = nullptr;
  UniqueFEIRExpr lenExpr = nullptr;
  if (fieldID == 0) {
    dstExpr = FEIRBuilder::CreateExprDRead(var->Clone());
    MIRSymbol *dstSym = var->GenerateMIRSymbol(mirBuilder);
    MIRSymbol *funcSym = mirBuilder.GetCurrentFunctionNotNull()->GetFuncSymbol();
    // Get the boundary attr(i.e. boundary length expr cache) of var
    lenExpr = ENCChecker::GetBoundaryLenExprCache(*funcSym, *dstSym);
  } else {
    FieldID tmpID = fieldID;
    MIRStructType *structType = static_cast<MIRStructType*>(var->GetType()->GenerateMIRTypeAuto());
    MIRStructType *lastestStruct = nullptr;
    FieldPair fieldPair = FEUtils::GetLastestStructTypeAndField(*structType, lastestStruct, tmpID);
    UniqueFEIRType fieldType = FEIRTypeHelper::CreateTypeNative(
        *GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldPair.second.first));
    dstExpr = FEIRBuilder::CreateExprDReadAggField(var->Clone(), fieldID, std::move(fieldType));
    // Get the boundary attr(i.e. boundary length expr cache) of field
    lenExpr = ENCChecker::GetBoundaryLenExprCache(*lastestStruct, fieldPair);
    if (lenExpr != nullptr) {
      UniqueFEIRExpr realLenExpr = ENCChecker::GetRealBoundaryLenExprInField(
          lenExpr->Clone(), *structType, dstExpr); // lenExpr needs to be cloned
      if (realLenExpr != nullptr) {
        lenExpr = std::move(realLenExpr);
      }
    }
  }
  if (lenExpr != nullptr) {
    UniqueFEIRStmt stmt = ENCChecker::InsertBoundaryLEChecking(lenExpr->Clone(), expr);
    if (stmt != nullptr) {
      stmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
      std::list<StmtNode*> stmtnodes = stmt->GenMIRStmts(mirBuilder);
      ans.insert(ans.end(), stmtnodes.begin(), stmtnodes.end());
    }
  }
  ENCChecker::AssignBoundaryVar(mirBuilder, dstExpr, expr, lenExpr, ans);
}

void FEIRStmtIAssign::AssignBoundaryVarAndChecking(MIRBuilder &mirBuilder, std::list<StmtNode*> &ans) const {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic() || fieldID == 0 || baseExpr->GetPrimType() != PTY_ptr) {
    return;
  }
  // insert assign boundary checking for computed r-value
  ENCChecker::InsertBoundaryAssignChecking(mirBuilder, ans, baseExpr, srcFileIndex, srcFileLineNum);

  MIRType *baseType = static_cast<MIRPtrType*>(addrType->GenerateMIRTypeAuto())->GetPointedType();
  FieldID tmpID = fieldID;
  MIRStructType *lastestStruct = nullptr;
  FieldPair fieldPair = FEUtils::GetLastestStructTypeAndField(
      *static_cast<MIRStructType*>(baseType), lastestStruct, tmpID);
  MIRType *dstType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldPair.second.first);
  UniqueFEIRExpr dstExpr = FEIRBuilder::CreateExprIRead(
      FEIRTypeHelper::CreateTypeNative(*dstType), addrType->Clone(), addrExpr->Clone(), fieldID);
  // Get the boundary attr (i.e. boundary length expr cache) of field
  UniqueFEIRExpr lenExpr = ENCChecker::GetBoundaryLenExprCache(*lastestStruct, fieldPair);
  if (lenExpr != nullptr) {
    UniqueFEIRExpr realLenExpr = ENCChecker::GetRealBoundaryLenExprInField(
        lenExpr->Clone(), *static_cast<MIRStructType*>(baseType), dstExpr);  // lenExpr needs to be cloned
    if (realLenExpr != nullptr) {
      lenExpr = std::move(realLenExpr);
    }
    UniqueFEIRStmt stmt = ENCChecker::InsertBoundaryLEChecking(lenExpr->Clone(), baseExpr);
    if (stmt != nullptr) {
      stmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
      std::list<StmtNode*> stmtnodes = stmt->GenMIRStmts(mirBuilder);
      ans.insert(ans.end(), stmtnodes.begin(), stmtnodes.end());
    }
  }
  ENCChecker::AssignBoundaryVar(mirBuilder, dstExpr, baseExpr, lenExpr, ans);
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
    if (arrType->GetSize() == 0) {  // unsized array
      return args;
    }
    // Convert to the following node for array:
    // assertge/assertlt lnode: addrofarray + index; assertle lnode: (attributed upper boundary) addrofarray + len expr
    // assertge rnode: addrof array; assertlt/assertle rnode: addrof array + sizeof expr
    if (kOpcodeInfo.IsAssertUpperBoundary(op)) {
      rightExpr = FEIRBuilder::CreateExprBinary(
          OP_add, std::move(rightExpr), std::make_unique<FEIRExprConst>(arrType->GetSize(), PTY_ptr));
    }
    leftNode = leftExpr->GenMIRNode(mirBuilder);
    rightNode = rightExpr->GenMIRNode(mirBuilder);
  } else {
    // Convert to the following node for expr with boundary var:
    // assertge/assertlt lnode: addrof base + index; assertle lnode: (attributed upper boundary) addrof base + len expr
    // assertge rnode: lower boundary; assertlt/assertle rnode: upper boundary
    auto it = curFunction->GetBoundaryMap().find(rightExpr->Hash());
    if (it != curFunction->GetBoundaryMap().end()) {
      StIdx boundaryStIdx = kOpcodeInfo.IsAssertLowerBoundary(op) ? it->second.first : it->second.second;
      MIRSymbol *boundarySym = curFunction->GetLocalOrGlobalSymbol(boundaryStIdx);
      CHECK_NULL_FATAL(boundarySym);
      rightNode = mirBuilder.CreateExprDread(*boundarySym);
    } else {
      UniqueFEIRExpr lenExpr = ENCChecker::GetGlobalOrFieldLenExprInExpr(mirBuilder, rightExpr);
      if (lenExpr != nullptr) {  // a global var or field with boundary attr
        if (kOpcodeInfo.IsAssertUpperBoundary(op)) {
          rightExpr = FEIRBuilder::CreateExprBinary(OP_add, std::move(rightExpr), std::move(lenExpr));
        }
        rightNode = rightExpr->GenMIRNode(mirBuilder);
      } else {
        if (op == OP_callassertle) {
          WARN(kLncWarn, "%s:%d EnhanceC waring: boundaryless pointer passed to callee that requires a boundary pointer"
               " argument", FEManager::GetModule().GetFileNameFromFileNum(srcFileIndex).c_str(), srcFileLineNum);
       }
        if (op == OP_returnassertle) {
          WARN(kLncWarn, "%s:%d EnhanceC waring: returned pointer's boundary and the functions requirement are "
               "mismatched", FEManager::GetModule().GetFileNameFromFileNum(srcFileIndex).c_str(), srcFileLineNum);
       }
        if (op == OP_assignassertle) {
          WARN(kLncWarn, "%s:%d EnhanceC waring: r-value requires a boundary pointer",
               FEManager::GetModule().GetFileNameFromFileNum(srcFileIndex).c_str(), srcFileLineNum);
        }
        return args;
      }
    }
    leftNode = leftExpr->GenMIRNode(mirBuilder);
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
  } else {
    baseAddrFEExpr = ENCChecker::FindBaseExprInPointerOperation(baseAddrFEExpr);
    if (baseAddrFEExpr == nullptr) {
      return;
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

// ---------------------------
// caller boundary inserting and checking
// ---------------------------
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
    UniqueFEIRExpr realLenExpr = ENCChecker::GetRealBoundaryLenExprInFunc(
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
    realLenExpr = FEIRBuilder::CreateExprBinary(OP_add, baseExpr->Clone(), std::move(realLenExpr));
    exprs.emplace_back(std::move(realLenExpr));
    exprs.emplace_back(std::move(baseExpr));
    UniqueFEIRStmt stmt = std::make_unique<FEIRStmtCallAssertBoundary>(OP_callassertle, std::move(exprs),
                                                                       GetFuncName(), i);
    stmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
    stmts.emplace_back(std::move(stmt));
  }
}

void ASTCallExpr::InsertBoundaryVarInRet(std::list<UniqueFEIRStmt> &stmts) const {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic() || !IsNeedRetExpr()) {
    return;
  }
  std::list<UniqueFEIRStmt> nullStmts;
  UniqueFEIRExpr realLenExpr = nullptr;
  if (funcName == "malloc" && args.size() == 1) {
    realLenExpr = args[0]->Emit2FEExpr(nullStmts);
  }
  if (funcDecl != nullptr && funcDecl->GetBoundaryLenExpr() != nullptr) {
    realLenExpr = ENCChecker::GetRealBoundaryLenExprInFunc(
        funcDecl->GetBoundaryLenExpr()->Emit2FEExpr(stmts), *funcDecl, *this);
  }
  if (realLenExpr == nullptr) {
    return;
  }
  // GetCurrentFunction need to be optimized when parallel features
  MIRFunction *curFunction = FEManager::GetMIRBuilder().GetCurrentFunctionNotNull();
  ENCChecker::InitBoundaryVar(*curFunction, varName, *retType, std::move(realLenExpr), stmts);
}
}
