/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "fe_macros.h"

namespace maple {
void ASTParser::ProcessNonnullFuncPtrAttrs(MapleAllocator &allocator, const clang::ValueDecl &valueDecl,
                                           ASTDecl &astVar) {
  const MIRFuncType *funcType = FEUtils::GetFuncPtrType(*astVar.GetTypeDesc().front());
  if (funcType == nullptr) {
    return;
  }
  std::vector<TypeAttrs> attrsVec = funcType->GetParamAttrsList();
  TypeAttrs retAttr = funcType->GetRetAttrs();
  // nonnull with args in function type pointers need marking nonnull arg
  for (const auto *nonNull : valueDecl.specific_attrs<clang::NonNullAttr>()) {
    if (!nonNull->args_size()) {
      continue;
    }
    for (const clang::ParamIdx &paramIdx : nonNull->args()) {
      // The clang ensures that nonnull attribute only applies to pointer parameter
      unsigned int idx = paramIdx.getASTIndex();
      if (idx >= attrsVec.size()) {
        continue;
      }
      attrsVec[idx].SetAttr(ATTR_nonnull);
    }
  }
  if (valueDecl.hasAttr<clang::ReturnsNonNullAttr>()) {
    retAttr.SetAttr(ATTR_nonnull);
  }
  MIRType *newFuncType = GlobalTables::GetTypeTable().GetOrCreateFunctionType(
      funcType->GetRetTyIdx(), funcType->GetParamTypeList(), attrsVec, funcType->IsVarargs(), retAttr);
  astVar.SetTypeDesc(MapleVector<MIRType*>({GlobalTables::GetTypeTable().GetOrCreatePointerType(
      *GlobalTables::GetTypeTable().GetOrCreatePointerType(*newFuncType))}, allocator.Adapter()));
}

bool ENCChecker::HasNonnullAttrInExpr(MIRBuilder &mirBuilder, const UniqueFEIRExpr &expr, bool isNested) {
  if (expr->GetKind() == kExprDRead) {
    if (isNested) {  // skip multi-dimensional pointer
      return true;
    }
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
    if (fieldID == 0) {
      return HasNonnullAttrInExpr(mirBuilder, iread->GetClonedOpnd(), true);
    }
    MIRType *pointerType = iread->GetClonedPtrType()->GenerateMIRTypeAuto();
    CHECK_FATAL(pointerType->IsMIRPtrType(), "Must be ptr type!");
    MIRType *baseType = static_cast<MIRPtrType*>(pointerType)->GetPointedType();
    CHECK_FATAL(baseType->IsStructType(), "basetype must be StructType");
    FieldPair fieldPair = static_cast<MIRStructType*>(baseType)->TraverseToFieldRef(fieldID);
    return fieldPair.second.second.GetAttr(FLDATTR_nonnull);
  } else if (isNested && expr->GetKind() == kExprAddrofArray) {
    return false;
  } else {
    return true;
  }
}

bool ENCChecker::HasNullExpr(const UniqueFEIRExpr &expr) {
  if (expr == nullptr || expr->GetKind() != kExprUnary || expr->GetPrimType() != PTY_ptr) {
    return false;
  }
  const UniqueFEIRExpr &cstExpr = static_cast<FEIRExprUnary*>(expr.get())->GetOpnd();
  return FEIRBuilder::IsZeroConstExpr(cstExpr);
}

void ENCChecker::CheckNonnullGlobalVarInit(const MIRSymbol &sym, const MIRConst *cst) {
  if (!FEOptions::GetInstance().IsNpeCheckDynamic() || !sym.GetAttr(ATTR_nonnull)) {
    return;
  }
  if (cst == nullptr) {
    FE_ERR(kLncErr, "%s:%d error: nonnull parameter is uninitialized when defined",
           FEManager::GetModule().GetFileNameFromFileNum(sym.GetSrcPosition().FileNum()).c_str(),
           sym.GetSrcPosition().LineNum());
    return;
  }
  if (cst->IsZero()) {
    FE_ERR(kLncErr, "%s:%d error: null assignment of nonnull pointer",
           FEManager::GetModule().GetFileNameFromFileNum(sym.GetSrcPosition().FileNum()).c_str(),
           sym.GetSrcPosition().LineNum());
    return;
  }
}

void ENCChecker::CheckNullFieldInGlobalStruct(MIRType &type, MIRAggConst &cst, const MapleVector<ASTExpr*> &initExprs) {
  if (!FEOptions::GetInstance().IsNpeCheckDynamic() || !ENCChecker::HasNonnullFieldInStruct(type)) {
    return;
  }
  auto &structType = static_cast<MIRStructType&>(type);
  for (size_t i = 0; i < structType.GetFieldsSize(); ++i) {
    if (!structType.GetFieldsElemt(i).second.second.GetAttr(FLDATTR_nonnull)) {
      continue;
    }
    size_t idx = structType.GetKind() == kTypeUnion ? 0 : i;  // union only is one element
    if (idx < cst.GetConstVec().size() && cst.GetConstVecItem(idx) != nullptr && cst.GetConstVecItem(idx)->IsZero()) {
        FE_ERR(kLncErr, "%s:%d error: null assignment of nonnull field pointer. [field name: %s]",
               FEManager::GetModule().GetFileNameFromFileNum(initExprs[idx]->GetSrcFileIdx()).c_str(),
               initExprs[idx]->GetSrcFileLineNum(),
               GlobalTables::GetStrTable().GetStringFromStrIdx(structType.GetFieldsElemt(i).first).c_str());
    }
  }
}

void ENCChecker::CheckNonnullLocalVarInit(const MIRSymbol &sym, const ASTExpr *initExpr) {
  if (!FEOptions::GetInstance().IsNpeCheckDynamic() || !sym.GetAttr(ATTR_nonnull)) {
    return;
  }
  if (initExpr == nullptr) {
    FE_ERR(kLncErr, "%s:%d error: nonnull parameter is uninitialized when defined",
           FEManager::GetModule().GetFileNameFromFileNum(sym.GetSrcPosition().FileNum()).c_str(),
           sym.GetSrcPosition().LineNum());
    return;
  }
}

void ENCChecker::CheckNonnullLocalVarInit(const MIRSymbol &sym, const UniqueFEIRExpr &initFEExpr,
                                          std::list<UniqueFEIRStmt> &stmts) {
  if (!FEOptions::GetInstance().IsNpeCheckDynamic() || !sym.GetAttr(ATTR_nonnull)) {
    return;
  }
  if (HasNullExpr(initFEExpr)) {
    FE_ERR(kLncErr, "%s:%d error: null assignment of nonnull pointer",
           FEManager::GetModule().GetFileNameFromFileNum(sym.GetSrcPosition().FileNum()).c_str(),
           sym.GetSrcPosition().LineNum());
    return;
  }
  if (initFEExpr->GetPrimType() == PTY_ptr) {
    UniqueFEIRStmt stmt = std::make_unique<FEIRStmtAssertNonnull>(OP_assignassertnonnull, initFEExpr->Clone());
    Loc loc = {sym.GetSrcPosition().FileNum(), sym.GetSrcPosition().LineNum(), sym.GetSrcPosition().Column()};
    stmt->SetSrcLoc(loc);
    stmts.emplace_back(std::move(stmt));
  }
}

std::string ENCChecker::GetNthStr(size_t index) {
  switch (index) {
    case 0:
      return "1st";
    case 1:
      return "2nd";
    case 2:
      return "3rd";
    default: {
      std::ostringstream oss;
      oss << index + 1 << "th";
      return oss.str();
    }
  }
}

std::string ENCChecker::PrintParamIdx(const std::list<size_t> &idxs) {
  std::ostringstream os;
  for (auto iter = idxs.begin(); iter != idxs.end(); ++iter) {
    if (iter != idxs.begin()) {
      os << ", ";
    }
    os << GetNthStr(*iter);
  }
  return os.str();
}

void ENCChecker::CheckNonnullArgsAndRetForFuncPtr(const MIRType &dstType, const UniqueFEIRExpr &srcExpr,
                                                  const Loc &loc) {
  if (!FEOptions::GetInstance().IsNpeCheckDynamic()) {
    return;
  }
  const MIRFuncType *funcType = FEUtils::GetFuncPtrType(dstType);
  if (funcType == nullptr) {
    return;
  }
  if (srcExpr->GetKind() == kExprAddrofFunc) {  // check func ptr l-value and &func decl r-value
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(
        static_cast<FEIRExprAddrofFunc*>(srcExpr.get())->GetFuncAddr());
    MIRFunction *srcFunc = FEManager::GetTypeManager().GetMIRFunction(strIdx, false);
    CHECK_FATAL(srcFunc != nullptr, "can not get MIRFunction");
    std::list<size_t> errIdxs;
    for (size_t i = 0; i < srcFunc->GetParamSize() && i < funcType->GetParamAttrsList().size(); ++i) {
      if (srcFunc->GetNthParamAttr(i).GetAttr(ATTR_nonnull) != funcType->GetNthParamAttrs(i).GetAttr(ATTR_nonnull)) {
        errIdxs.emplace_back(i);
      }
    }
    if (!errIdxs.empty()) {
      FE_ERR(kLncErr, "%s:%d error: function pointer and assigned function %s are mismatched "
             "for the %s argument of nonnull attributes",
             FEManager::GetModule().GetFileNameFromFileNum(loc.fileIdx).c_str(), loc.line,
             srcFunc->GetName().c_str(), PrintParamIdx(errIdxs).c_str());
    }
    if (srcFunc->GetFuncAttrs().GetAttr(FUNCATTR_nonnull) != funcType->GetRetAttrs().GetAttr(ATTR_nonnull)) {
      FE_ERR(kLncErr, "%s:%d error: function pointer and target function's nonnull attributes are mismatched for"
             " the return value", FEManager::GetModule().GetFileNameFromFileNum(loc.fileIdx).c_str(), loc.line);
    }
  }
  const MIRFuncType *srcFuncType = FEUtils::GetFuncPtrType(*srcExpr->GetType()->GenerateMIRTypeAuto());
  if (srcFuncType != nullptr) {   // check func ptr l-value and func ptr r-value
    std::list<size_t> errIdxs;
    for (size_t i = 0; i < srcFuncType->GetParamAttrsList().size() && i < funcType->GetParamAttrsList().size(); ++i) {
      if (srcFuncType->GetNthParamAttrs(i).GetAttr(ATTR_nonnull) !=
          funcType->GetNthParamAttrs(i).GetAttr(ATTR_nonnull)) {
        errIdxs.emplace_back(i);
      }
    }
    if (!errIdxs.empty()) {
      FE_ERR(kLncErr, "%s:%d error: function pointer's nonnull attributes are mismatched for the %s argument",
             FEManager::GetModule().GetFileNameFromFileNum(loc.fileIdx).c_str(), loc.line,
             PrintParamIdx(errIdxs).c_str());
    }
    if (srcFuncType->GetRetAttrs().GetAttr(ATTR_nonnull) != funcType->GetRetAttrs().GetAttr(ATTR_nonnull)) {
      FE_ERR(kLncErr, "%s:%d error: function pointer's nonnull attributes are mismatched for the return value",
             FEManager::GetModule().GetFileNameFromFileNum(loc.fileIdx).c_str(), loc.line);
    }
  }
}

void FEIRStmtDAssign::CheckNonnullArgsAndRetForFuncPtr(const MIRBuilder &mirBuilder) const {
  if (!FEOptions::GetInstance().IsNpeCheckDynamic() || ENCChecker::IsUnsafeRegion(mirBuilder)) {
    return;
  }
  MIRType *baseType = var->GetType()->GenerateMIRTypeAuto();
  if (fieldID != 0) {
    baseType = FEUtils::GetStructFieldType(static_cast<MIRStructType*>(baseType), fieldID);
  }
  ENCChecker::CheckNonnullArgsAndRetForFuncPtr(*baseType, expr, loc);
}

void FEIRStmtIAssign::CheckNonnullArgsAndRetForFuncPtr(const MIRBuilder &mirBuilder, const MIRType &baseType) const {
  if (!FEOptions::GetInstance().IsNpeCheckDynamic() || ENCChecker::IsUnsafeRegion(mirBuilder)) {
    return;
  }
  MIRType *fieldType = FEUtils::GetStructFieldType(static_cast<const MIRStructType*>(&baseType), fieldID);
  ENCChecker::CheckNonnullArgsAndRetForFuncPtr(*fieldType, baseExpr, loc);
}

bool ENCChecker::HasNonnullFieldInStruct(const MIRType &mirType) {
  if (mirType.IsStructType() && FEManager::GetTypeManager().IsOwnedNonnullFieldStructSet(mirType.GetTypeIndex())) {
    return true;
  }
  return false;
}

bool ENCChecker::HasNonnullFieldInPtrStruct(const MIRType &mirType) {
  if (mirType.IsMIRPtrType()) {
    MIRType *structType = static_cast<const MIRPtrType&>(mirType).GetPointedType();
    if (structType != nullptr && HasNonnullFieldInStruct(*structType)) {
      return true;
    }
  }
  return false;
}

void ASTCallExpr::CheckNonnullFieldInStruct() const {
  if (!FEOptions::GetInstance().IsNpeCheckDynamic()) {
    return;
  }
  std::list<UniqueFEIRStmt> nullStmts;
  UniqueFEIRExpr baseExpr = nullptr;
  if (GetFuncName() == "bzero" && args.size() == 2) {
    baseExpr = args[0]->Emit2FEExpr(nullStmts);
  } else if (GetFuncName() == "memset" && args.size() == 3 &&
             FEIRBuilder::IsZeroConstExpr(args[1]->Emit2FEExpr(nullStmts))) {
    baseExpr = args[0]->Emit2FEExpr(nullStmts);
  }
  if (baseExpr == nullptr) {
    return;
  }
  MIRType *mirType = ENCChecker::GetTypeFromAddrExpr(baseExpr);  // check addrof or iaddrof
  if (mirType != nullptr) {
    mirType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*mirType);
  } else if ((baseExpr->GetKind() == kExprDRead || baseExpr->GetKind() == kExprIRead) &&
              baseExpr->GetType() != nullptr) {
    mirType = baseExpr->GetType()->GenerateMIRTypeAuto();
  }
  if (mirType != nullptr && ENCChecker::HasNonnullFieldInPtrStruct(*mirType)) {
      FE_ERR(kLncErr, "%s:%d error: null assignment of nonnull structure field pointer in %s",
             FEManager::GetModule().GetFileNameFromFileNum(loc.fileIdx).c_str(), loc.line, GetFuncName().c_str());
  }
}

void ENCChecker::CheckNonnullFieldInStruct(const MIRType &src, const MIRType &dst, const Loc &loc) {
  if (!FEOptions::GetInstance().IsNpeCheckDynamic() ||
      !dst.IsMIRPtrType() || !src.IsMIRPtrType() ||
      dst.GetTypeIndex() == src.GetTypeIndex()) {
    return;
  }
  if (ENCChecker::HasNonnullFieldInPtrStruct(dst)) {
    FE_ERR(kLncErr, "%s:%d error: null assignment risk of nonnull field pointer",
           FEManager::GetModule().GetFileNameFromFileNum(loc.fileIdx).c_str(), loc.line);
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
  ProcessByteBoundaryFuncAttrs(allocator, funcDecl, astFunc);
  ProcessBoundaryFuncAttrsByIndex(allocator, funcDecl, astFunc);
}

void ASTParser::ProcessByteBoundaryFuncAttrs(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl,
                                             ASTFunc &astFunc) {
  for (const auto *returnsCountAttr : funcDecl.specific_attrs<clang::ReturnsByteCountAttr>()) {
    clang::Expr *expr = returnsCountAttr->getLenExpr();
    ASTExpr *lenExpr = ProcessExpr(allocator, expr);
    if (lenExpr == nullptr) {
      continue;
    }
    ProcessBoundaryLenExprInFunc(allocator, funcDecl, static_cast<unsigned int>(-1), astFunc, lenExpr, false);
  }
  for (const auto *countAttr : funcDecl.specific_attrs<clang::ByteCountAttr>()) {
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
          ProcessBoundaryLenExprInFunc(allocator, funcDecl, i, astFunc, lenExpr, false);
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
      ProcessBoundaryLenExprInFunc(allocator, funcDecl, idx, astFunc, lenExpr, false);
    }
  }
}

void ASTParser::ProcessBoundaryFuncAttrsByIndex(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl,
                                                ASTFunc &astFunc) {
  for (const auto *countIndexAttr : funcDecl.specific_attrs<clang::CountIndexAttr>()) {
    unsigned int lenIdx = countIndexAttr->getLenVarIndex().getASTIndex();
    for (const clang::ParamIdx &paramIdx : countIndexAttr->index()) {
      unsigned int idx = paramIdx.getASTIndex();
      if (idx >= astFunc.GetParamDecls().size()) {
        continue;
      }
      ProcessBoundaryLenExprInFunc(allocator, funcDecl, idx, astFunc, lenIdx, true);
    }
  }
  for (const auto *returnsCountIndexAttr : funcDecl.specific_attrs<clang::ReturnsCountIndexAttr>()) {
    unsigned int retLenIdx = returnsCountIndexAttr->getLenVarIndex().getASTIndex();
    ProcessBoundaryLenExprInFunc(allocator, funcDecl, static_cast<unsigned int>(-1), astFunc, retLenIdx, true);
  }
  for (const auto *byteCountIndexAttr : funcDecl.specific_attrs<clang::ByteCountIndexAttr>()) {
    unsigned int lenIdx = byteCountIndexAttr->getLenVarIndex().getASTIndex();
    for (const clang::ParamIdx &paramIdx : byteCountIndexAttr->index()) {
      unsigned int idx = paramIdx.getASTIndex();
      if (idx >= astFunc.GetParamDecls().size()) {
        continue;
      }
      ProcessBoundaryLenExprInFunc(allocator, funcDecl, idx, astFunc, lenIdx, false);
    }
  }
  for (const auto *returnsByteCountIndexAttr : funcDecl.specific_attrs<clang::ReturnsByteCountIndexAttr>()) {
    unsigned int retLenIdx = returnsByteCountIndexAttr->getLenVarIndex().getASTIndex();
    ProcessBoundaryLenExprInFunc(allocator, funcDecl, static_cast<unsigned int>(-1), astFunc, retLenIdx, false);
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
      if (countAttr->index_size()) {
        continue;  // boundary attrs with index args are only marked function pointers
      }
      ASTExpr *lenExpr = ProcessExpr(allocator, expr);
      if (lenExpr == nullptr) {
        continue;
      }
      ProcessBoundaryLenExprInFunc(allocator, funcDecl, i, astFunc, lenExpr, true);
    }
    for (const auto *byteCountAttr : parmDecl->specific_attrs<clang::ByteCountAttr>()) {
      clang::Expr *expr = byteCountAttr->getLenExpr();
      if (byteCountAttr->index_size()) {
        continue;  // boundary attrs with index args are only marked function pointers
      }
      ASTExpr *lenExpr = ProcessExpr(allocator, expr);
      if (lenExpr == nullptr) {
        continue;
      }
      ProcessBoundaryLenExprInFunc(allocator, funcDecl, i, astFunc, lenExpr, false);
    }
  }
  ProcessBoundaryParamAttrsByIndex(allocator, funcDecl, astFunc);
}

void ASTParser::ProcessBoundaryParamAttrsByIndex(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl,
                                                 ASTFunc &astFunc) {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic()) {
    return;
  }
  for (unsigned int i = 0; i < funcDecl.getNumParams(); ++i) {
    const clang::ParmVarDecl *parmDecl = funcDecl.getParamDecl(i);
    for (const auto *countIndexAttr : parmDecl->specific_attrs<clang::CountIndexAttr>()) {
      if (countIndexAttr->index_size()) {
        continue;  // boundary attrs with index args are only marked function pointers
      }
      unsigned int lenIdx = countIndexAttr->getLenVarIndex().getASTIndex();
      ProcessBoundaryLenExprInFunc(allocator, funcDecl, i, astFunc, lenIdx, true);
    }
    for (const auto *byteCountIndexAttr : parmDecl->specific_attrs<clang::ByteCountIndexAttr>()) {
      if (byteCountIndexAttr->index_size()) {
        continue;  // boundary attrs with index args are only marked function pointers
      }
      unsigned int lenIdx = byteCountIndexAttr->getLenVarIndex().getASTIndex();
      ProcessBoundaryLenExprInFunc(allocator, funcDecl, i, astFunc, lenIdx, false);
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
    if (countAttr->index_size()) {
      continue;  // boundary attrs with index args are only marked function pointers
    }
    ProcessBoundaryLenExprInVar(allocator, astVar, varDecl, lenExpr, true);
  }
  for (const auto *byteCountAttr : varDecl.specific_attrs<clang::ByteCountAttr>()) {
    clang::Expr *expr = byteCountAttr->getLenExpr();
    ASTExpr *lenExpr = ProcessExpr(allocator, expr);
    if (lenExpr == nullptr) {
      continue;
    }
    if (byteCountAttr->index_size()) {
      continue;  // boundary attrs with index args are only marked function pointers
    }
    ProcessBoundaryLenExprInVar(allocator, astVar, varDecl, lenExpr, false);
  }
}

void ASTParser::ProcessBoundaryFuncPtrAttrs(MapleAllocator &allocator, const clang::ValueDecl &valueDecl,
                                            ASTDecl &astDecl) {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic()) {
    return;
  }
  const MIRFuncType *funcType = FEUtils::GetFuncPtrType(*astDecl.GetTypeDesc().front());
  if (funcType == nullptr || !valueDecl.getType()->isFunctionPointerType()) {
    return;
  }
  clang::QualType qualType = valueDecl.getType()->getPointeeType();
  const clang::FunctionType *clangFuncType = qualType->getAs<clang::FunctionType>();
  if (clangFuncType == nullptr) {
    return;
  }
  const clang::FunctionProtoType *proto = llvm::dyn_cast<clang::FunctionProtoType>(clangFuncType);
  if (proto == nullptr) {
    return;
  }
  std::vector<TypeAttrs> attrsVec = funcType->GetParamAttrsList();
  TypeAttrs retAttr = funcType->GetRetAttrs();
  bool isUpdated = false;
  for (const auto *countAttr : valueDecl.specific_attrs<clang::CountAttr>()) {
    if (ProcessBoundaryFuncPtrAttrsForParams(countAttr, allocator, *funcType, *proto, attrsVec)) {
      isUpdated = true;
    }
  }
  for (const auto *byteCountAttr : valueDecl.specific_attrs<clang::ByteCountAttr>()) {
    if (ProcessBoundaryFuncPtrAttrsForParams(byteCountAttr, allocator, *funcType, *proto, attrsVec)) {
      isUpdated = true;
    }
  }
  for (const auto *returnsCountAttr : valueDecl.specific_attrs<clang::ReturnsCountAttr>()) {
    if (ProcessBoundaryFuncPtrAttrsForRet(returnsCountAttr, allocator, *funcType, *clangFuncType, retAttr)) {
      isUpdated = true;
    }
  }
  for (const auto *returnsByteCountAttr : valueDecl.specific_attrs<clang::ReturnsByteCountAttr>()) {
    if (ProcessBoundaryFuncPtrAttrsForRet(returnsByteCountAttr, allocator, *funcType, *clangFuncType, retAttr)) {
      isUpdated = true;
    }
  }
  if (isUpdated) {
    MIRType *newFuncType = GlobalTables::GetTypeTable().GetOrCreateFunctionType(
        funcType->GetRetTyIdx(), funcType->GetParamTypeList(), attrsVec, funcType->IsVarargs(), retAttr);
    astDecl.SetTypeDesc(MapleVector<MIRType*>({GlobalTables::GetTypeTable().GetOrCreatePointerType(
        *GlobalTables::GetTypeTable().GetOrCreatePointerType(*newFuncType))}, allocator.Adapter()));
  }
  ProcessBoundaryFuncPtrAttrsByIndex(allocator, valueDecl, astDecl, *funcType);
}

template <typename T>
bool ASTParser::ProcessBoundaryFuncPtrAttrsForParams(T *attr, MapleAllocator &allocator, const MIRFuncType &funcType,
                                                     const clang::FunctionProtoType &proto,
                                                     std::vector<TypeAttrs> &attrsVec) {
  bool isUpdated = false;
  clang::Expr *expr = attr->getLenExpr();
  ASTExpr *lenExpr = ProcessExpr(allocator, expr);
  if (!attr->index_size() || lenExpr == nullptr) {
    return isUpdated;
  }
  std::vector<TyIdx> typesVec = funcType.GetParamTypeList();
  for (const clang::ParamIdx &paramIdx : attr->index()) {
    unsigned int idx = paramIdx.getASTIndex();
    if (idx >= attrsVec.size() || idx >= typesVec.size()) {
      continue;
    }
    MIRType *ptrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(typesVec[idx]);
    ASTVar *tmpDecl = ASTDeclsBuilder::ASTVarBuilder(allocator, MapleString("", allocator.GetMemPool()), "tmpVar",
        MapleVector<MIRType*>({ptrType}, allocator.Adapter()), GenericAttrs());
    bool isByte = std::is_same<typename std::decay<T>::type, clang::ByteCountAttr>::value;
    ProcessBoundaryLenExprInVar(allocator, *tmpDecl, proto.getParamType(idx), lenExpr, !isByte);
    ENCChecker::InsertBoundaryInAtts(attrsVec[idx], tmpDecl->GetBoundaryInfo());
    isUpdated = true;
  }
  return isUpdated;
}

template <typename T>
bool ASTParser::ProcessBoundaryFuncPtrAttrsForRet(T *attr, MapleAllocator &allocator, const MIRFuncType &funcType,
                                                  const clang::FunctionType &clangFuncType, TypeAttrs &retAttr) {
  clang::Expr *expr = attr->getLenExpr();
  ASTExpr *lenExpr = ProcessExpr(allocator, expr);
  if (lenExpr == nullptr) {
    return false;
  }
  MIRType *ptrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(funcType.GetRetTyIdx());
  ASTVar *tmpRetDecl = ASTDeclsBuilder::ASTVarBuilder(allocator, MapleString("", allocator.GetMemPool()),
      "tmpRetVar", MapleVector<MIRType*>({ptrType}, allocator.Adapter()), GenericAttrs());
  bool isByte = std::is_same<typename std::decay<T>::type, clang::ReturnsByteCountAttr>::value;
  ProcessBoundaryLenExprInVar(allocator, *tmpRetDecl, clangFuncType.getReturnType(), lenExpr, !isByte);
  ENCChecker::InsertBoundaryInAtts(retAttr, tmpRetDecl->GetBoundaryInfo());
  return true;
}

void ASTParser::ProcessBoundaryFuncPtrAttrsByIndex(MapleAllocator &allocator, const clang::ValueDecl &valueDecl,
                                                   ASTDecl &astDecl, const MIRFuncType &funcType) {
  std::vector<TypeAttrs> attrsVec = funcType.GetParamAttrsList();
  TypeAttrs retAttr = funcType.GetRetAttrs();
  bool isUpdated = false;
  for (const auto *countAttr : valueDecl.specific_attrs<clang::CountIndexAttr>()) {
    if (ProcessBoundaryFuncPtrAttrsByIndexForParams(countAttr, astDecl, funcType, attrsVec)) {
      isUpdated = true;
    }
  }
  for (const auto *byteCountAttr : valueDecl.specific_attrs<clang::ByteCountIndexAttr>()) {
    if (ProcessBoundaryFuncPtrAttrsByIndexForParams(byteCountAttr, astDecl, funcType, attrsVec)) {
      isUpdated = true;
    }
  }
  for (const auto *returnsCountIndexAttr : valueDecl.specific_attrs<clang::ReturnsCountIndexAttr>()) {
    unsigned int lenIdx = returnsCountIndexAttr->getLenVarIndex().getASTIndex();
    retAttr.GetAttrBoundary().SetLenParamIdx(static_cast<int8>(lenIdx));
    isUpdated = true;
  }
  for (const auto *returnsByteCountIndexAttr : valueDecl.specific_attrs<clang::ReturnsByteCountIndexAttr>()) {
    unsigned int lenIdx = returnsByteCountIndexAttr->getLenVarIndex().getASTIndex();
    retAttr.GetAttrBoundary().SetLenParamIdx(static_cast<int8>(lenIdx));
    retAttr.GetAttrBoundary().SetIsBytedLen(true);
    isUpdated = true;
  }
  if (isUpdated) {
    MIRType *newFuncType = GlobalTables::GetTypeTable().GetOrCreateFunctionType(
        funcType.GetRetTyIdx(), funcType.GetParamTypeList(), attrsVec, funcType.IsVarargs(), retAttr);
    astDecl.SetTypeDesc(MapleVector<MIRType*>({GlobalTables::GetTypeTable().GetOrCreatePointerType(
        *GlobalTables::GetTypeTable().GetOrCreatePointerType(*newFuncType))}, allocator.Adapter()));
  }
}

template <typename T>
bool ASTParser::ProcessBoundaryFuncPtrAttrsByIndexForParams(T *attr, ASTDecl &astDecl, const MIRFuncType &funcType,
                                                            std::vector<TypeAttrs> &attrsVec) {
  bool isUpdated = false;
  std::vector<TyIdx> typesVec = funcType.GetParamTypeList();
  unsigned int lenIdx = attr->getLenVarIndex().getASTIndex();
  if (!attr->index_size() || lenIdx >= typesVec.size()) {
    return isUpdated;
  }
  MIRType *lenType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(typesVec[lenIdx]);
  if (lenType == nullptr || !FEUtils::IsInteger(lenType->GetPrimType())) {
    FE_ERR(kLncErr, "%s:%d error: The boundary length var by the %s argument is not an integer type",
           FEManager::GetModule().GetFileNameFromFileNum(astDecl.GetSrcFileIdx()).c_str(),
           astDecl.GetSrcFileLineNum(), ENCChecker::GetNthStr(lenIdx).c_str());
    return isUpdated;
  }
  for (const clang::ParamIdx &paramIdx : attr->index()) {
    unsigned int idx = paramIdx.getASTIndex();
    if (idx >= attrsVec.size() || idx >= typesVec.size()) {
      continue;
    }
    attrsVec[idx].GetAttrBoundary().SetLenParamIdx(static_cast<int8>(lenIdx));
    if (std::is_same<typename std::decay<T>::type, clang::ByteCountIndexAttr>::value) {
      attrsVec[idx].GetAttrBoundary().SetIsBytedLen(true);
    }
    isUpdated = true;
  }
  return isUpdated;
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
      if (!countAttr->index_size()) {
        ProcessBoundaryLenExprInField(allocator, *astField, structDecl, fieldDecl->getType(), lenExpr, true);
      }
    }
    for (const auto *byteCountAttr : fieldDecl->specific_attrs<clang::ByteCountAttr>()) {
      clang::Expr *expr = byteCountAttr->getLenExpr();
      ASTExpr *lenExpr = ProcessExpr(allocator, expr);
      if (lenExpr == nullptr) {
        continue;
      }
      if (!byteCountAttr->index_size()) {
        ProcessBoundaryLenExprInField(allocator, *astField, structDecl, fieldDecl->getType(), lenExpr, false);
      }
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
    FE_ERR(kLncErr, "%s:%d error: The variable modified by the boundary attribute should be a pointer type",
           FEManager::GetModule().GetFileNameFromFileNum(lenExpr->GetSrcFileIdx()).c_str(),
           lenExpr->GetSrcFileLineNum());
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
    FE_ERR(kLncErr, "%s:%d error: The boundary length expr is not an integer type",
           FEManager::GetModule().GetFileNameFromFileNum(lenExpr->GetSrcFileIdx()).c_str(),
           lenExpr->GetSrcFileLineNum());
    return;
  }
  if (isSize) {
    // The type size can only be obtained from ClangDecl instead of ASTDecl,
    // because the field of mir struct type has not yet been initialized at this time
    uint32 lenSize = GetSizeFromQualType(qualType->getPointeeType());
    MIRType *pointedType = static_cast<MIRPtrType*>(ptrDecl.GetTypeDesc().front())->GetPointedType();
    if (pointedType->GetPrimType() == PTY_f64) {
      lenSize = 8; // 8 is f64 byte num, because now f128 also cvt to f64
    }
    lenExpr = GetAddrShiftExpr(allocator, lenExpr, lenSize);
  }
  ptrDecl.SetBoundaryLenExpr(lenExpr);
  ptrDecl.SetIsBytedLen(!isSize);
}

void ENCChecker::CheckLenExpr(const ASTExpr &lenExpr, const std::list<UniqueFEIRStmt> &nullstmts) {
  for (const auto &stmt : nullstmts) {
    bool isAssertStmt = false;
    if (stmt->GetKind() == kStmtNary) {
      FEIRStmtNary *nary = static_cast<FEIRStmtNary*>(stmt.get());
      if (kOpcodeInfo.IsAssertBoundary(nary->GetOP()) || kOpcodeInfo.IsAssertNonnull(nary->GetOP())) {
        isAssertStmt = true;
      }
    }
    if (!isAssertStmt) {
      FE_ERR(kLncErr, "%s:%d error: The boundary length expr containing statement is invalid",
             FEManager::GetModule().GetFileNameFromFileNum(lenExpr.GetSrcFileIdx()).c_str(),
             lenExpr.GetSrcFileLineNum());
      break;
    }
  }
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
    FE_ERR(kLncErr, "%s:%d error: The parameter annotated boundary attr [the %s argument] is not found"
           "in the function [%s]", FEManager::GetModule().GetFileNameFromFileNum(lenExpr->GetSrcFileIdx()).c_str(),
           lenExpr->GetSrcFileLineNum(), ENCChecker::GetNthStr(idx).c_str(), astFunc.GetName().c_str());
    return;
  }
  // parameter stringLiteral -> real parameter decl
  auto getLenExprFromStringLiteral = [&]() -> ASTExpr* {
    ASTStringLiteral *strExpr = static_cast<ASTStringLiteral*>(lenExpr);
    std::string lenName(strExpr->GetCodeUnits().begin(), strExpr->GetCodeUnits().end());
    for (size_t i = 0; i < astFunc.GetParamDecls().size(); ++i) {
      if (astFunc.GetParamDecls()[i]->GetName() != lenName) {
        continue;
      }
      MIRType *lenType = astFunc.GetParamDecls()[i]->GetTypeDesc().front();
      if (lenType == nullptr || !FEUtils::IsInteger(lenType->GetPrimType())) {
        FE_ERR(kLncErr, "%s:%d error: The parameter [%s] specified as boundary length var is not an integer type "
               "in the function [%s]", FEManager::GetModule().GetFileNameFromFileNum(lenExpr->GetSrcFileIdx()).c_str(),
               lenExpr->GetSrcFileLineNum(), lenName.c_str(), astFunc.GetName().c_str());
        return nullptr;
      }
      ASTDeclRefExpr *lenRefExpr = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
      lenRefExpr->SetASTDecl(astFunc.GetParamDecls()[i]);
      lenRefExpr->SetType(astFunc.GetParamDecls()[i]->GetTypeDesc().front());
      ptrDecl->SetBoundaryLenParamIdx(static_cast<int8>(i));
      return lenRefExpr;
    }
    FE_ERR(kLncErr, "%s:%d error: The parameter [%s] specified as boundary length var is not found "
           "in the function [%s]", FEManager::GetModule().GetFileNameFromFileNum(lenExpr->GetSrcFileIdx()).c_str(),
           lenExpr->GetSrcFileLineNum(), lenName.c_str(), astFunc.GetName().c_str());
    return nullptr;
  };
  ProcessBoundaryLenExpr(allocator, *ptrDecl, qualType, getLenExprFromStringLiteral, lenExpr, isSize);
}

void ASTParser::ProcessBoundaryLenExprInFunc(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl,
                                             unsigned int idx, ASTFunc &astFunc, unsigned int lenIdx, bool isSize) {
  if (lenIdx > astFunc.GetParamDecls().size()) {
    FE_ERR(kLncErr, "error: The %s parameter specified as boundary length var is not found "
           "in the function [%s]", ENCChecker::GetNthStr(lenIdx).c_str(), astFunc.GetName().c_str());
    return;
  }
  ASTDecl *lenDecl = astFunc.GetParamDecls()[lenIdx];
  MIRType *lenType = lenDecl->GetTypeDesc().front();
  if (lenType == nullptr || !FEUtils::IsInteger(lenType->GetPrimType())) {
    FE_ERR(kLncErr, "error: The %s parameter specified as boundary length var is not an integer type "
           "in the function [%s]", ENCChecker::GetNthStr(lenIdx).c_str(), astFunc.GetName().c_str());
    return;
  }
  ASTDeclRefExpr *lenRefExpr = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
  lenRefExpr->SetASTDecl(lenDecl);
  lenRefExpr->SetType(lenType);

  ASTDecl *ptrDecl = nullptr;
  if (idx == static_cast<unsigned int>(-1)) {  // return boundary attr
    ptrDecl = &astFunc;
  } else if (idx < astFunc.GetParamDecls().size()) {
    ptrDecl = astFunc.GetParamDecls()[idx];
  } else {
    FE_ERR(kLncErr, "error: The %s parameter annotated boundary attr is not found in the function [%s]",
           ENCChecker::GetNthStr(idx).c_str(), astFunc.GetName().c_str());
    return;
  }
  ptrDecl->SetBoundaryLenParamIdx(static_cast<int8>(lenIdx));
  ProcessBoundaryLenExprInFunc(allocator, funcDecl, idx, astFunc, lenRefExpr, isSize);
}

void ASTParser::ProcessBoundaryLenExprInVar(MapleAllocator &allocator, ASTDecl &ptrDecl,
                                            const clang::VarDecl &varDecl, ASTExpr *lenExpr, bool isSize) {
  if (!varDecl.isLocalVarDeclOrParm()) {
    ASTDecl *lenDecl = lenExpr->GetASTDecl();
    if (lenDecl != nullptr && FEUtils::IsInteger(lenDecl->GetTypeDesc().front()->GetPrimType())) {
      lenDecl->SetAttr(GENATTR_final_boundary_size);
    }
  }
  ProcessBoundaryLenExprInVar(allocator, ptrDecl, varDecl.getType(), lenExpr, isSize);
}

void ASTParser::ProcessBoundaryLenExprInVar(MapleAllocator &allocator, ASTDecl &ptrDecl,
                                            const clang::QualType &qualType, ASTExpr *lenExpr, bool isSize) {
  // The StringLiteral is not allowed to use as boundary length of var
  auto getLenExprFromStringLiteral = [&]() -> ASTExpr* {
    FE_ERR(kLncErr, "%s:%d error: The StringLiteral is not allowed to use as boundary length of var",
           FEManager::GetModule().GetFileNameFromFileNum(lenExpr->GetSrcFileIdx()).c_str(),
           lenExpr->GetSrcFileLineNum());
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
        FE_ERR(kLncErr, "%s:%d error: The field [%s] specified as boundary length var is not an integer type "
               "in the struct [%s]", FEManager::GetModule().GetFileNameFromFileNum(lenExpr->GetSrcFileIdx()).c_str(),
               lenExpr->GetSrcFileLineNum(), lenName.c_str(), structDecl.GetName().c_str());
        return nullptr;
      }
      fieldDecl->SetAttr(GENATTR_final_boundary_size);
      ASTDeclRefExpr *lenRefExpr = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
      lenRefExpr->SetASTDecl(fieldDecl);
      lenRefExpr->SetType(fieldDecl->GetTypeDesc().front());
      return lenRefExpr;
    }
    FE_ERR(kLncErr, "%s:%d error: The StringLiteral [%s] as boundary length var is not found "
           "in the struct [%s]", FEManager::GetModule().GetFileNameFromFileNum(lenExpr->GetSrcFileIdx()).c_str(),
           lenExpr->GetSrcFileLineNum(), lenName.c_str(), structDecl.GetName().c_str());
    return nullptr;
  };
  ProcessBoundaryLenExpr(allocator, ptrDecl, qualType, getLenExprFromStringLiteral, lenExpr, isSize);
}

// ---------------------------
// process boundary var
// ---------------------------
UniqueFEIRExpr ENCChecker::FindBaseExprInPointerOperation(const UniqueFEIRExpr &expr, bool isIncludingAddrof) {
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
    baseExpr = FindBaseExprInPointerOperation(cvtExpr->GetOpnd());
  } else if (expr->GetKind() == kExprAddrofArray) {
    FEIRExprAddrofArray *arrExpr = static_cast<FEIRExprAddrofArray*>(expr.get());
    baseExpr = FindBaseExprInPointerOperation(arrExpr->GetExprArray());
  } else if ((expr->GetKind() == kExprDRead && expr->GetPrimType() == PTY_ptr) ||
      (expr->GetKind() == kExprIRead && expr->GetPrimType() == PTY_ptr && expr->GetFieldID() != 0) ||
      GetArrayTypeFromExpr(expr) != nullptr) {
    baseExpr = expr->Clone();
  } else if (isIncludingAddrof && GetTypeFromAddrExpr(expr) != nullptr) {  // addrof as 1-sized array
    baseExpr = expr->Clone();
  }
  return baseExpr;
}

MIRType *ENCChecker::GetTypeFromAddrExpr(const UniqueFEIRExpr &expr) {
  if (expr->GetKind() == kExprAddrofVar) {
    MIRType *type = expr->GetVarUses().front()->GetType()->GenerateMIRTypeAuto();
    if (expr->GetFieldID() == 0) {
      return type;
    } else {
      CHECK_FATAL(type->IsStructType(), "basetype must be StructType");
      FieldID fieldID = expr->GetFieldID();
      FieldPair fieldPair = static_cast<MIRStructType*>(type)->TraverseToFieldRef(fieldID);
      return GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldPair.second.first);
    }
  } else if (expr->GetKind() == kExprIAddrof) {
    FEIRExprIAddrof *iaddrof = static_cast<FEIRExprIAddrof*>(expr.get());
    MIRType *pointerType = iaddrof->GetClonedPtrType()->GenerateMIRTypeAuto();
    CHECK_FATAL(pointerType->IsMIRPtrType(), "Must be ptr type!");
    MIRType *baseType = static_cast<MIRPtrType*>(pointerType)->GetPointedType();
    CHECK_FATAL(baseType->IsStructType(), "basetype must be StructType");
    FieldID fieldID = iaddrof->GetFieldID();
    FieldPair fieldPair = static_cast<MIRStructType*>(baseType)->TraverseToFieldRef(fieldID);
    return GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldPair.second.first);
  } else {
    return nullptr;
  }
}

MIRType *ENCChecker::GetArrayTypeFromExpr(const UniqueFEIRExpr &expr) {
  MIRType *arrType = GetTypeFromAddrExpr(expr);
  if (arrType != nullptr && arrType->GetKind() == kTypeArray &&
      !static_cast<MIRArrayType*>(arrType)->GetTypeAttrs().GetAttr(ATTR_incomplete_array)) {
    return arrType;
  }
  if (expr->GetKind() == kExprAddrof) {  // local char* value size
    auto *constArr = static_cast<FEIRExprAddrofConstArray*>(expr.get());
    return GlobalTables::GetTypeTable().GetOrCreateArrayType(
        *constArr->GetElemType(), constArr->GetStringLiteralSize() + 1);  // including the end character with string
  } else if (expr->GetKind() == kExprDRead || expr->GetKind() == kExprIRead) {  // global const char* value size
    MIRType *type = expr->GetType()->GenerateMIRTypeAuto();
    if (type->IsMIRPtrType() && static_cast<MIRPtrType*>(type)->GetPointedType()->GetPrimType() == PTY_u8) {
      MIRConst *cst = GetMIRConstFromExpr(expr);
      if (cst != nullptr && cst->GetKind() == kConstStrConst) {
        size_t size = GlobalTables::GetUStrTable().GetStringFromStrIdx(
            static_cast<MIRStrConst*>(cst)->GetValue()).size() + 1;  // including the end character with string
        return GlobalTables::GetTypeTable().GetOrCreateArrayType(
            *GlobalTables::GetTypeTable().GetUInt8(), static_cast<uint32>(size));
      }
    }
    return nullptr;
  }
  return nullptr;
}

MIRConst *ENCChecker::GetMIRConstFromExpr(const UniqueFEIRExpr &expr) {
  if (expr == nullptr || expr->GetVarUses().size() != 1) {
    return nullptr;
  }
  MIRSymbol *sym = expr->GetVarUses().front()->GenerateMIRSymbol(FEManager::GetMIRBuilder());
  if (!sym->IsGlobal() || !sym->GetAttr(ATTR_const) || sym->GetKonst() == nullptr) {  // const global
    return nullptr;
  }
  if (expr->GetKind() == kExprDRead || expr->GetKind() == kExprAddrofVar) {
    if (expr->GetFieldID() == 0) {
      return sym->GetKonst();
    } else if (expr->GetFieldID() != 0 && sym->GetKonst()->GetKind() == kConstAggConst) {
      MIRAggConst *aggConst = static_cast<MIRAggConst*>(sym->GetKonst());
      FieldID tmpID = expr->GetFieldID();
      MIRConst *cst = FEUtils::TraverseToMIRConst(aggConst, *static_cast<MIRStructType*>(sym->GetType()), tmpID);
      return cst;
    }
    return nullptr;
  } else if (expr->GetKind() == kExprIRead) {
    auto *iread = static_cast<FEIRExprIRead*>(expr.get());
    MIRConst *cst = GetMIRConstFromExpr(iread->GetClonedOpnd());
    if (expr->GetFieldID() != 0 && cst != nullptr && sym->GetKonst()->GetKind() == kConstAggConst) {
      MIRType *pointType = static_cast<MIRPtrType*>(iread->GetClonedPtrType()->GenerateMIRTypeAuto())->GetPointedType();
      FieldID tmpID = expr->GetFieldID();
      cst = FEUtils::TraverseToMIRConst(static_cast<MIRAggConst*>(cst), *static_cast<MIRStructType*>(pointType), tmpID);
    }
    return cst;
  } else if (expr->GetKind() == kExprAddrofArray) {
    FEIRExprAddrofArray *arrExpr = static_cast<FEIRExprAddrofArray*>(expr.get());
    MIRConst *cst = GetMIRConstFromExpr(arrExpr->GetExprArray());
    if (cst != nullptr && cst->GetKind() == kConstAggConst) {
      for (const auto & idxExpr : arrExpr->GetExprIndexs()) {
        MIRAggConst *aggConst = static_cast<MIRAggConst*>(cst);
        if (idxExpr->GetKind() != kExprConst) {
          return nullptr;
        }
        uint64 idx = static_cast<FEIRExprConst*>(idxExpr.get())->GetValue().u64;
        if (idx >= aggConst->GetConstVec().size()) {
          return nullptr;
        }
        cst = aggConst->GetConstVecItem(idx);
      }
      return cst;
    }
    return nullptr;
  }
  return nullptr;
}

void ENCChecker::AssignBoundaryVar(MIRBuilder &mirBuilder, const UniqueFEIRExpr &dstExpr, const UniqueFEIRExpr &srcExpr,
                                   const UniqueFEIRExpr &lRealLenExpr, std::list<StmtNode*> &ans) {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic() ||
      srcExpr->GetPrimType() != PTY_ptr || dstExpr->GetPrimType() != PTY_ptr) {
    return;
  }
  if (lRealLenExpr == nullptr && IsGlobalVarInExpr(dstExpr)) {
    return;  // skip boundary assignment for global var whithout boundary attr
  }
  // Avoid inserting redundant boundary vars
  const std::string prefix = "_boundary.";
  if (dstExpr->GetKind() == kExprDRead &&
      static_cast<FEIRExprDRead*>(dstExpr.get())->GetVar()->GetNameRaw().compare(0, prefix.size(), prefix) == 0) {
    return;
  }
  MIRFunction *curFunction = mirBuilder.GetCurrentFunctionNotNull();
  FEFunction &curFEFunction = FEManager::GetCurrentFEFunction();
  UniqueFEIRExpr baseExpr = FindBaseExprInPointerOperation(srcExpr, true);
  if (baseExpr == nullptr) {
    return;
  }
  if (dstExpr->Hash() == baseExpr->Hash()) {  // skip self-assignment, e.g. p++;
    return;
  }
  // Check if the l-value has a boundary var
  std::pair<StIdx, StIdx> lBoundaryVarStIdx = std::make_pair(StIdx(0), StIdx(0));
  auto lIt = curFEFunction.GetBoundaryMap().find(dstExpr->Hash());
  if (lIt != curFEFunction.GetBoundaryMap().end()) {  // The boundary var exists on the l-value
    lBoundaryVarStIdx = lIt->second;
  } else {
    if (lRealLenExpr != nullptr) {  // init boundary in func body if a field/global var l-value with boundary attr
      std::list<UniqueFEIRStmt> stmts;
      lBoundaryVarStIdx = InitBoundaryVar(*curFunction, dstExpr, lRealLenExpr->Clone(), stmts);
      for (const auto &stmt : stmts) {
        std::list<StmtNode*> stmtNodes = stmt->GenMIRStmts(mirBuilder);
        for (auto stmtNode : stmtNodes) {
          curFunction->GetBody()->InsertFirst(stmtNode);
        }
      }
    }
  }
  // Check if the r-value has a boundary var or an array or a global var/field with boundary attr
  std::pair<StIdx, StIdx> rBoundaryVarStIdx = std::make_pair(StIdx(0), StIdx(0));
  MIRType *arrType = nullptr;
  UniqueFEIRExpr rRealLenExpr = nullptr;
  auto rIt = curFEFunction.GetBoundaryMap().find(baseExpr->Hash());
  if (rIt != curFEFunction.GetBoundaryMap().end()) {  // Assgin when the boundary var exists on the r-value
    rBoundaryVarStIdx = rIt->second;
  } else {
    arrType = GetArrayTypeFromExpr(baseExpr);
    if (arrType == nullptr) {
      rRealLenExpr = GetGlobalOrFieldLenExprInExpr(mirBuilder, baseExpr);
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
    } else {
      MIRType *addrofType = GetTypeFromAddrExpr(baseExpr);
      if (addrofType != nullptr) {  // addrof as 1-sized array, if l-value with boundary and r-value is addrof non-array
        lowerStmt = mirBuilder.CreateStmtDassign(*lLowerSym, 0, baseExpr->GenMIRNode(mirBuilder));
        UniqueFEIRExpr binExpr = FEIRBuilder::CreateExprBinary(
            OP_add, baseExpr->Clone(), std::make_unique<FEIRExprConst>(addrofType->GetSize(), PTY_ptr));
        upperStmt = mirBuilder.CreateStmtDassign(*lUpperSym, 0, binExpr->GenMIRNode(mirBuilder));
      } else {
        // Insert a undef boundary r-value
        // when there is a l-value boundary var and r-value without a boundary var
        lowerStmt = mirBuilder.CreateStmtDassign(*lLowerSym, 0, baseExpr->GenMIRNode(mirBuilder));
        UniqueFEIRExpr undef = std::make_unique<FEIRExprConst>(kUndefValue, PTY_ptr);
        upperStmt = mirBuilder.CreateStmtDassign(*lUpperSym, 0, undef->GenMIRNode(mirBuilder));
      }
    }
    if (lowerStmt == nullptr || upperStmt == nullptr) {
      return;
    }
    if (lRealLenExpr != nullptr) {  // use l-vaule own boundary(use r-base + offset) when l-value has a boundary attr
      BaseNode *binExpr = mirBuilder.CreateExprBinary(
          OP_add, *lLowerSym->GetType(), srcExpr->GenMIRNode(mirBuilder), lRealLenExpr->GenMIRNode(mirBuilder));
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
  MIRType *boundaryType = expr->GetType()->GenerateMIRTypeAuto();
  MIRSymbol *lowerSrcSym = mirBuilder.GetOrCreateLocalDecl(boundaryName + ".lower", *boundaryType);
  MIRSymbol *upperSrcSym = mirBuilder.GetOrCreateLocalDecl(boundaryName + ".upper", *boundaryType);
  // assign undef val to boundary var in func body head
  AssignUndefVal(mirBuilder, *upperSrcSym);
  AssignUndefVal(mirBuilder, *lowerSrcSym);
  // update BoundaryMap
  boundaryVarStIdx = std::make_pair(lowerSrcSym->GetStIdx(), upperSrcSym->GetStIdx());
  FEManager::GetCurrentFEFunction().SetBoundaryMap(expr->Hash(), boundaryVarStIdx);
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
        kUndefValue, *GlobalTables::GetTypeTable().GetPrimType(PTY_ptr));
    sym.SetKonst(cst);
  } else {
    BaseNode *undef = mirBuilder.CreateIntConst(kUndefValue, PTY_ptr);
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
  ASTVar *lowerDecl = ASTDeclsBuilder::ASTVarBuilder(allocator, MapleString("", allocator.GetMemPool()),
      lowerVarName, MapleVector<MIRType*>({ptrType}, allocator.Adapter()), GenericAttrs());
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
  ASTVar *upperDecl = ASTDeclsBuilder::ASTVarBuilder(allocator, MapleString("", allocator.GetMemPool()),
      upperVarName, MapleVector<MIRType*>({ptrType}, allocator.Adapter()), GenericAttrs());
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
    Loc loc = {ptrDecl.GetSrcFileIdx(), ptrDecl.GetSrcFileLineNum(), ptrDecl.GetSrcFileColumn()};
    lowerStmt->SetSrcLoc(loc);
    upperStmt->SetSrcLoc(loc);
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
  FEManager::GetCurrentFEFunction().SetBoundaryMap(
      ptrExpr->Hash(), std::make_pair(lowerSym->GetStIdx(), upperSym->GetStIdx()));
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
  FEManager::GetCurrentFEFunction().SetBoundaryMap(
      ptrExpr->Hash(), std::make_pair(lowerSym->GetStIdx(), upperSym->GetStIdx()));
}

std::pair<StIdx, StIdx> ENCChecker::InitBoundaryVar(MIRFunction &curFunction, const UniqueFEIRExpr &ptrExpr,
                                                    UniqueFEIRExpr lenExpr, std::list<UniqueFEIRStmt> &stmts) {
  std::string ptrName = GetBoundaryName(ptrExpr);
  if (ptrName.empty()) {
    return std::make_pair(StIdx(0), StIdx(0));
  }
  MIRType *ptrType = ptrExpr->GetType()->GenerateMIRTypeAuto();
  // insert lower boundary stmt
  std::string lowerVarName = ptrName + ".lower";
  UniqueFEIRVar lowerVar = FEIRBuilder::CreateVarNameForC(lowerVarName, *ptrType);
  UniqueFEIRStmt lowerStmt = FEIRBuilder::CreateStmtDAssign(std::move(lowerVar), ptrExpr->Clone());
  stmts.emplace_back(std::move(lowerStmt));
  // insert upper boundary stmt
  UniqueFEIRExpr binExpr = FEIRBuilder::CreateExprBinary(OP_add, ptrExpr->Clone(), std::move(lenExpr));
  std::string upperVarName = ptrName + ".upper";
  UniqueFEIRVar upperVar = FEIRBuilder::CreateVarNameForC(upperVarName, *ptrType);
  UniqueFEIRStmt upperStmt = FEIRBuilder::CreateStmtDAssign(std::move(upperVar), std::move(binExpr));
  stmts.emplace_back(std::move(upperStmt));
  // update BoundaryMap
  MIRSymbol *lowerSym = FEManager::GetMIRBuilder().GetOrCreateDeclInFunc(lowerVarName, *ptrType, curFunction);
  MIRSymbol *upperSym = FEManager::GetMIRBuilder().GetOrCreateDeclInFunc(upperVarName, *ptrType, curFunction);
  auto stIdxs = std::make_pair(lowerSym->GetStIdx(), upperSym->GetStIdx());
  FEManager::GetCurrentFEFunction().SetBoundaryMap(ptrExpr->Hash(), stIdxs);
  return stIdxs;
}


UniqueFEIRExpr ENCChecker::GetGlobalOrFieldLenExprInExpr(MIRBuilder &mirBuilder, const UniqueFEIRExpr &expr) {
  UniqueFEIRExpr lenExpr = nullptr;
  if (expr->GetKind() == kExprDRead && expr->GetFieldID() == 0) {
    FEIRVar *var = expr->GetVarUses().front();
    MIRSymbol *symbol = var->GenerateMIRSymbol(mirBuilder);
    if (!symbol->IsGlobal()) {
      return nullptr;
    }
    // Get the boundary attr(i.e. boundary length expr cache)
    lenExpr = GetBoundaryLenExprCache(symbol->GetAttrs());
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
    FieldPair fieldPair = structType->TraverseToFieldRef(tmpID);
    // Get the boundary attr(i.e. boundary length expr cache) of field
    lenExpr = GetBoundaryLenExprCache(fieldPair.second.second);
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
    bool flag = ((*i)->GetKind() == kStmtNary);
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

UniqueFEIRExpr ENCChecker::GetRealBoundaryLenExprInFuncByIndex(const TypeAttrs &typeAttrs, const MIRType &type,
                                                               const ASTCallExpr &astCallExpr) {
  int8 idx = typeAttrs.GetAttrBoundary().GetLenParamIdx();
  if (idx >= 0) {
    ASTExpr *astLenExpr = astCallExpr.GetArgsExpr()[idx];
    if (!typeAttrs.GetAttrBoundary().IsBytedLen()) {
      CHECK_FATAL(type.IsMIRPtrType(), "Must be ptr type!");
      size_t lenSize = static_cast<const MIRPtrType&>(type).GetPointedType()->GetSize();
      MapleAllocator &allocator = FEManager::GetModule().GetMPAllocator();
      astLenExpr = ASTParser::GetAddrShiftExpr(allocator, astCallExpr.GetArgsExpr()[idx], static_cast<uint32>(lenSize));
    }
    std::list<UniqueFEIRStmt> nullStmts;
    return astLenExpr->Emit2FEExpr(nullStmts);
  } else if (typeAttrs.GetAttrBoundary().GetLenExprHash() != 0) {
    return ENCChecker::GetBoundaryLenExprCache(typeAttrs);
  }
  return nullptr;
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
  if (lenExpr->GetKind() == kExprIRead) {
    FEIRExprIRead *ireadExpr = static_cast<FEIRExprIRead*>(lenExpr.get());
    UniqueFEIRExpr subExpr = GetRealBoundaryLenExprInFunc(ireadExpr->GetClonedOpnd(), astFunc, astCallExpr);
    if (subExpr != nullptr) {
      ireadExpr->SetClonedOpnd(std::move(subExpr));
    } else {
      return nullptr;
    }
  }
  // formal parameter length expr -> actual parameter expr
  std::list<UniqueFEIRStmt> nullStmts;
  if (lenExpr->GetKind() == kExprDRead) {
    std::string lenName = lenExpr->GetVarUses().front()->GetNameRaw();
    for (size_t i = 0; i < astFunc.GetParamDecls().size(); ++i) {
      if (lenName == astFunc.GetParamDecls()[i]->GetName()) {
        return astCallExpr.GetArgsExpr()[i]->Emit2FEExpr(nullStmts);
      }
    }
    // the var may be a global var
    FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "%s:%d EnhanceC warning: The var [%s] as boundary length var "
        "is not found in the caller func [%s], check the caller whether the var is the actual parameter",
        FEManager::GetModule().GetFileNameFromFileNum(astCallExpr.GetSrcFileIdx()).c_str(),
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
    MIRType *reType = FEUtils::GetStructFieldType(&baseType, static_cast<FieldID>(fieldID));
    UniqueFEIRType reFEType = FEIRTypeHelper::CreateTypeNative(*reType);
    if (dstExpr->GetKind() == kExprDRead) {
      return FEIRBuilder::CreateExprDReadAggField(
          dstExpr->GetVarUses().front()->Clone(), static_cast<FieldID>(fieldID), std::move(reFEType));
    } else if (dstExpr->GetKind() == kExprIRead) {
      FEIRExprIRead *iread = static_cast<FEIRExprIRead*>(dstExpr.get());
      return FEIRBuilder::CreateExprIRead(
          std::move(reFEType), iread->GetClonedPtrType(), iread->GetClonedOpnd(), static_cast<FieldID>(fieldID));
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
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic() || boundary.lenExpr == nullptr ||
      stmts.size() == 0 || stmts.back()->GetKind() != kStmtReturn) {
    return;
  }
  std::list<UniqueFEIRStmt> nullStmts;
  const UniqueFEIRExpr &retExpr = static_cast<FEIRStmtReturn*>(stmts.back().get())->GetExpr();
  UniqueFEIRExpr baseExpr = ENCChecker::FindBaseExprInPointerOperation(retExpr);
  if (baseExpr == nullptr) {
    return;
  }
  std::list<UniqueFEIRExpr> exprs;
  UniqueFEIRExpr lenExpr = boundary.lenExpr->Emit2FEExpr(nullStmts);
  if (boundary.lenParamIdx != -1) {  // backup return boundary size in head func body
    UniqueFEIRVar retSizeVar = FEIRBuilder::CreateVarNameForC("_boundary.return.size", lenExpr->GetType()->Clone());
    UniqueFEIRStmt lenStmt = FEIRBuilder::CreateStmtDAssign(retSizeVar->Clone(), lenExpr->Clone());
    stmts.emplace_front(std::move(lenStmt));
    lenExpr = FEIRBuilder::CreateExprDRead(std::move(retSizeVar));
  }
  lenExpr = FEIRBuilder::CreateExprBinary(OP_add, retExpr->Clone(), std::move(lenExpr));
  exprs.emplace_back(std::move(lenExpr));
  exprs.emplace_back(std::move(baseExpr));
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtAssertBoundary>(OP_returnassertle, std::move(exprs));
  stmt->SetSrcLoc(stmts.back()->GetSrcLoc());
  stmts.insert(--stmts.end(), std::move(stmt));
}

void ENCChecker::InsertBoundaryAssignChecking(MIRBuilder &mirBuilder, std::list<StmtNode*> &ans,
                                              const UniqueFEIRExpr &srcExpr, const Loc &loc) {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic() || srcExpr == nullptr || srcExpr->GetPrimType() != PTY_ptr ||
      srcExpr->GetKind() != kExprBinary) {  // pointer computed assignment
    return;
  }
  if (srcExpr->IsBoundaryChecking()) {  // skip if boundary checking has been generated
    return;
  }
  UniqueFEIRExpr baseExpr = FindBaseExprInPointerOperation(srcExpr);
  if (baseExpr == nullptr) {
    return;
  }
  srcExpr->SetIsBoundaryChecking(true);
  // insert l-value lower boundary chencking
  std::list<UniqueFEIRExpr> lowerExprs;
  lowerExprs.emplace_back(srcExpr->Clone());
  lowerExprs.emplace_back(baseExpr->Clone());
  UniqueFEIRStmt lowerStmt = std::make_unique<FEIRStmtAssertBoundary>(OP_calcassertge, std::move(lowerExprs));
  lowerStmt->SetSrcLoc(loc);
  std::list<StmtNode*> lowerStmts = lowerStmt->GenMIRStmts(mirBuilder);
  ans.splice(ans.end(), lowerStmts);
  // insert l-value upper boundary chencking
  std::list<UniqueFEIRExpr> upperExprs;
  upperExprs.emplace_back(srcExpr->Clone());
  upperExprs.emplace_back(baseExpr->Clone());
  UniqueFEIRStmt upperStmt = std::make_unique<FEIRStmtAssertBoundary>(OP_calcassertlt, std::move(upperExprs));
  upperStmt->SetSrcLoc(loc);
  std::list<StmtNode*> upperStmts = upperStmt->GenMIRStmts(mirBuilder);
  ans.splice(ans.end(), upperStmts);
}

UniqueFEIRStmt ENCChecker::InsertBoundaryLEChecking(UniqueFEIRExpr lenExpr, const UniqueFEIRExpr &srcExpr,
                                                    const UniqueFEIRExpr &dstExpr) {
  UniqueFEIRExpr baseExpr = ENCChecker::FindBaseExprInPointerOperation(srcExpr);
  if (baseExpr == nullptr) {
    return nullptr;
  }
  if (dstExpr->Hash() == baseExpr->Hash()) {  // skip self-assignment, e.g. p++;
    return nullptr;
  }
  UniqueFEIRExpr boundaryExpr = FEIRBuilder::CreateExprBinary(  // use r-value base + offset
      OP_add, srcExpr->Clone(), std::move(lenExpr));
  std::list<UniqueFEIRExpr> exprs;
  exprs.emplace_back(std::move(boundaryExpr));
  exprs.emplace_back(std::move(baseExpr));
  return std::make_unique<FEIRStmtAssertBoundary>(OP_assignassertle, std::move(exprs));
}

UniqueFEIRExpr ENCChecker::GetBoundaryLenExprCache(uint32 hash) {
  if (hash == 0) {
    return nullptr;
  } else {
    return FEManager::GetTypeManager().GetBoundaryLenExprFromMap(hash);
  }
}

UniqueFEIRExpr ENCChecker::GetBoundaryLenExprCache(const TypeAttrs &attr) {
  return GetBoundaryLenExprCache(attr.GetAttrBoundary().GetLenExprHash());
}

UniqueFEIRExpr ENCChecker::GetBoundaryLenExprCache(const FieldAttrs &attr) {
  return GetBoundaryLenExprCache(attr.GetAttrBoundary().GetLenExprHash());
}

void ENCChecker::InsertBoundaryInAtts(TypeAttrs &attr, const BoundaryInfo &boundary) {
  attr.GetAttrBoundary().SetIsBytedLen(boundary.isBytedLen);
  if (boundary.lenParamIdx != -1) {
    attr.GetAttrBoundary().SetLenParamIdx(boundary.lenParamIdx);
  }
  if (boundary.lenExpr == nullptr) {
    return;
  }
  std::list<UniqueFEIRStmt> nullStmts;
  UniqueFEIRExpr lenExpr = boundary.lenExpr->Emit2FEExpr(nullStmts);
  CheckLenExpr(*boundary.lenExpr, nullStmts);
  InsertBoundaryLenExprInAtts(attr, lenExpr);
}

void ENCChecker::InsertBoundaryLenExprInAtts(TypeAttrs &attr, const UniqueFEIRExpr &expr) {
  if (expr == nullptr) {
    return;
  }
  uint32 hash = expr->Hash();
  FEManager::GetTypeManager().InsertBoundaryLenExprHashMap(hash, expr->Clone());  // save expr cache
  attr.GetAttrBoundary().SetLenExprHash(hash);
}

void ENCChecker::InsertBoundaryInAtts(FieldAttrs &attr, const BoundaryInfo &boundary) {
  attr.GetAttrBoundary().SetIsBytedLen(boundary.isBytedLen);
  if (boundary.lenParamIdx != -1) {
    attr.GetAttrBoundary().SetLenParamIdx(boundary.lenParamIdx);
  }
  if (boundary.lenExpr == nullptr) {
    return;
  }
  std::list<UniqueFEIRStmt> nullStmts;
  UniqueFEIRExpr lenExpr = boundary.lenExpr->Emit2FEExpr(nullStmts);
  uint32 hash = lenExpr->Hash();
  FEManager::GetTypeManager().InsertBoundaryLenExprHashMap(hash, std::move(lenExpr));  // save expr cache
  attr.GetAttrBoundary().SetLenExprHash(hash);
}

void ENCChecker::InsertBoundaryInAtts(FuncAttrs &attr, const BoundaryInfo &boundary) {
  attr.GetAttrBoundary().SetIsBytedLen(boundary.isBytedLen);
  if (boundary.lenParamIdx != -1) {
    attr.GetAttrBoundary().SetLenParamIdx(boundary.lenParamIdx);
  }
  if (boundary.lenExpr == nullptr) {
    return;
  }
  std::list<UniqueFEIRStmt> nullStmts;
  UniqueFEIRExpr lenExpr = boundary.lenExpr->Emit2FEExpr(nullStmts);
  uint32 hash = lenExpr->Hash();
  FEManager::GetTypeManager().InsertBoundaryLenExprHashMap(hash, std::move(lenExpr));  // save expr cache
  attr.GetAttrBoundary().SetLenExprHash(hash);
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
  ENCChecker::InsertBoundaryAssignChecking(mirBuilder, ans, expr, loc);

  UniqueFEIRExpr dstExpr = nullptr;
  UniqueFEIRExpr lenExpr = nullptr;
  if (fieldID == 0) {
    dstExpr = FEIRBuilder::CreateExprDRead(var->Clone());
    MIRSymbol *dstSym = var->GenerateMIRSymbol(mirBuilder);
    // Get the boundary attr(i.e. boundary length expr cache) of var
    lenExpr = ENCChecker::GetBoundaryLenExprCache(dstSym->GetAttrs());
  } else {
    FieldID tmpID = fieldID;
    MIRStructType *structType = static_cast<MIRStructType*>(var->GetType()->GenerateMIRTypeAuto());
    FieldPair fieldPair = structType->TraverseToFieldRef(tmpID);
    UniqueFEIRType fieldType = FEIRTypeHelper::CreateTypeNative(
        *GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldPair.second.first));
    dstExpr = FEIRBuilder::CreateExprDReadAggField(var->Clone(), fieldID, std::move(fieldType));
    // Get the boundary attr(i.e. boundary length expr cache) of field
    lenExpr = ENCChecker::GetBoundaryLenExprCache(fieldPair.second.second);
    if (lenExpr != nullptr) {
      UniqueFEIRExpr realLenExpr = ENCChecker::GetRealBoundaryLenExprInField(
          lenExpr->Clone(), *structType, dstExpr); // lenExpr needs to be cloned
      if (realLenExpr != nullptr) {
        lenExpr = std::move(realLenExpr);
      }
    }
  }
  if (lenExpr != nullptr) {
    UniqueFEIRStmt stmt = ENCChecker::InsertBoundaryLEChecking(lenExpr->Clone(), expr, dstExpr);
    if (stmt != nullptr) {
      stmt->SetSrcLoc(loc);
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
  ENCChecker::InsertBoundaryAssignChecking(mirBuilder, ans, baseExpr, loc);

  MIRType *baseType = static_cast<MIRPtrType*>(addrType->GenerateMIRTypeAuto())->GetPointedType();
  FieldID tmpID = fieldID;
  FieldPair fieldPair = static_cast<MIRStructType*>(baseType)->TraverseToFieldRef(tmpID);
  MIRType *dstType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldPair.second.first);
  UniqueFEIRExpr dstExpr = FEIRBuilder::CreateExprIRead(
      FEIRTypeHelper::CreateTypeNative(*dstType), addrType->Clone(), addrExpr->Clone(), fieldID);
  // Get the boundary attr (i.e. boundary length expr cache) of field
  UniqueFEIRExpr lenExpr = ENCChecker::GetBoundaryLenExprCache(fieldPair.second.second);
  if (lenExpr != nullptr) {
    UniqueFEIRExpr realLenExpr = ENCChecker::GetRealBoundaryLenExprInField(
        lenExpr->Clone(), *static_cast<MIRStructType*>(baseType), dstExpr);  // lenExpr needs to be cloned
    if (realLenExpr != nullptr) {
      lenExpr = std::move(realLenExpr);
    }
    UniqueFEIRStmt stmt = ENCChecker::InsertBoundaryLEChecking(lenExpr->Clone(), baseExpr, dstExpr);
    if (stmt != nullptr) {
      stmt->SetSrcLoc(loc);
      std::list<StmtNode*> stmtnodes = stmt->GenMIRStmts(mirBuilder);
      ans.insert(ans.end(), stmtnodes.begin(), stmtnodes.end());
    }
  }
  ENCChecker::AssignBoundaryVar(mirBuilder, dstExpr, baseExpr, lenExpr, ans);
}

void ENCChecker::CheckBoundaryLenFinalAssign(MIRBuilder &mirBuilder, const UniqueFEIRVar &var, FieldID fieldID,
                                             const Loc &loc) {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic() || !FEOptions::GetInstance().IsEnableSafeRegion()) {
    return;
  }
  bool isUnsafe = mirBuilder.GetCurrentFunctionNotNull()->GetAttr(FUNCATTR_unsafed);
  if (!FEManager::GetCurrentFEFunction().GetSafeRegionFlag().empty()) {
    isUnsafe = !FEManager::GetCurrentFEFunction().GetSafeRegionFlag().top();
  }
  if (isUnsafe) {  // not warning in unsafe region
    return;
  }
  if (fieldID == 0) {
    MIRSymbol *dstSym = var->GenerateMIRSymbol(mirBuilder);
    if (dstSym->GetAttr(ATTR_final_boundary_size)) {
      WARN(kLncWarn, "%s:%d warning: this var specified as the global or field boundary length is "
           "assigned or token address. [Use __Unsafe__ to eliminate the warning]",
           FEManager::GetModule().GetFileNameFromFileNum(loc.fileIdx).c_str(), loc.line);
    }
  } else {
    FieldID tmpID = fieldID;
    MIRStructType *structType = static_cast<MIRStructType*>(var->GetType()->GenerateMIRTypeAuto());
    FieldPair fieldPair = structType->TraverseToFieldRef(tmpID);
    if (fieldPair.second.second.GetAttr(FLDATTR_final_boundary_size)) {
      WARN(kLncWarn, "%s:%d warning: this field specified as the global or field boundary length is "
           "assigned or token address. [Use __Unsafe__ to eliminate the warning]",
           FEManager::GetModule().GetFileNameFromFileNum(loc.fileIdx).c_str(), loc.line);
    }
  }
}

void ENCChecker::CheckBoundaryLenFinalAssign(MIRBuilder &mirBuilder, const UniqueFEIRType &addrType, FieldID fieldID,
                                             const Loc &loc) {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic() || !FEOptions::GetInstance().IsEnableSafeRegion() ||
      fieldID == 0) {
    return;
  }
  bool isUnsafe = mirBuilder.GetCurrentFunctionNotNull()->GetAttr(FUNCATTR_unsafed);
  if (!FEManager::GetCurrentFEFunction().GetSafeRegionFlag().empty()) {
    isUnsafe = !FEManager::GetCurrentFEFunction().GetSafeRegionFlag().top();
  }
  if (isUnsafe) {  // not warning in unsafe region
    return;
  }
  MIRType *baseType = static_cast<MIRPtrType*>(addrType->GenerateMIRTypeAuto())->GetPointedType();
  FieldID tmpID = fieldID;
  FieldPair fieldPair = static_cast<MIRStructType*>(baseType)->TraverseToFieldRef(tmpID);
  if (fieldPair.second.second.GetAttr(FLDATTR_final_boundary_size)) {
    WARN(kLncWarn, "%s:%d warning: this field specified as the global or field boundary length is "
         "assigned or token address. [Use __Unsafe__ to eliminate the warning]",
         FEManager::GetModule().GetFileNameFromFileNum(loc.fileIdx).c_str(), loc.line);
  }
}

void ENCChecker::CheckBoundaryLenFinalAddr(MIRBuilder &mirBuilder, const UniqueFEIRExpr &expr, const Loc &loc) {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic() || !FEOptions::GetInstance().IsEnableSafeRegion()) {
    return;
  }
  if (expr->GetKind() == kExprAddrofVar) {
    UniqueFEIRVar var = expr->GetVarUses().front()->Clone();
    CheckBoundaryLenFinalAssign(mirBuilder, var, expr->GetFieldID(), loc);
  } else if (expr->GetKind() == kExprIAddrof) {
    auto *iaddrof = static_cast<FEIRExprIAddrof*>(expr.get());
    CheckBoundaryLenFinalAssign(mirBuilder, iaddrof->GetClonedPtrType(), expr->GetFieldID(), loc);
  }
}

MapleVector<BaseNode*> ENCChecker::ReplaceBoundaryChecking(MIRBuilder &mirBuilder, const FEIRStmtNary *stmt) {
  MIRFunction *curFunction = mirBuilder.GetCurrentFunctionNotNull();
  UniqueFEIRExpr leftExpr = stmt->GetArgExprs().front()->Clone();
  UniqueFEIRExpr rightExpr = stmt->GetArgExprs().back()->Clone();
  BaseNode *leftNode = nullptr;
  BaseNode *rightNode = nullptr;
  MIRType *arrType = ENCChecker::GetArrayTypeFromExpr(stmt->GetArgExprs().back());
  MapleVector<BaseNode*> args(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
  Opcode op = stmt->GetOP();
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
    auto it = FEManager::GetCurrentFEFunction().GetBoundaryMap().find(rightExpr->Hash());
    UniqueFEIRExpr lenExpr = ENCChecker::GetGlobalOrFieldLenExprInExpr(mirBuilder, rightExpr);
    if (it != FEManager::GetCurrentFEFunction().GetBoundaryMap().end()) {
      if (lenExpr == nullptr && IsGlobalVarInExpr(rightExpr)) {
        return args;  // skip boundary checking for global var whithout boundary attr,
      }               // because the lack of global variable boundary analysis capabilities in mapleall
      StIdx boundaryStIdx = kOpcodeInfo.IsAssertLowerBoundary(op) ? it->second.first : it->second.second;
      MIRSymbol *boundarySym = curFunction->GetLocalOrGlobalSymbol(boundaryStIdx);
      CHECK_NULL_FATAL(boundarySym);
      rightNode = mirBuilder.CreateExprDread(*boundarySym);
    } else {
      if (lenExpr != nullptr) {  // a global var or field with boundary attr
        if (kOpcodeInfo.IsAssertUpperBoundary(op)) {
          rightExpr = FEIRBuilder::CreateExprBinary(OP_add, std::move(rightExpr), std::move(lenExpr));
        }
        rightNode = rightExpr->GenMIRNode(mirBuilder);
      } else {
        if (ENCChecker::IsUnsafeRegion(mirBuilder)) {
          return args;
        }
        if (op == OP_callassertle) {
          auto callAssert = static_cast<const FEIRStmtCallAssertBoundary*>(stmt);
          FE_ERR(kLncErr, "%s:%d error: boundaryless pointer passed to %s that requires a boundary pointer for the %s"
                 " argument", FEManager::GetModule().GetFileNameFromFileNum(stmt->GetSrcFileIdx()).c_str(),
                 stmt->GetSrcFileLineNum(), callAssert->GetFuncName().c_str(),
                 ENCChecker::GetNthStr(callAssert->GetParamIndex()).c_str());
        } else if (op == OP_returnassertle) {
          if (curFunction->GetName().compare(kBoundsBuiltFunc) != 0) {
            FE_ERR(kLncErr, "%s:%d error: boundaryless pointer returned from %s that requires a boundary pointer",
                   FEManager::GetModule().GetFileNameFromFileNum(stmt->GetSrcFileIdx()).c_str(),
                   stmt->GetSrcFileLineNum(), curFunction->GetName().c_str());
          }
        } else if (op == OP_assignassertle) {
          FE_ERR(kLncErr, "%s:%d error: r-value requires a boundary pointer",
              FEManager::GetModule().GetFileNameFromFileNum(stmt->GetSrcFileIdx()).c_str(), stmt->GetSrcFileLineNum());
        } else if (ENCChecker::IsSafeRegion(mirBuilder) &&
                   (op == OP_calcassertge ||
                    (op == OP_assertge && static_cast<const FEIRStmtAssertBoundary*>(stmt)->IsComputable()))) {
          FE_ERR(kLncErr, "%s:%d error: calculation with pointer requires bounds in safe region",
              FEManager::GetModule().GetFileNameFromFileNum(stmt->GetSrcFileIdx()).c_str(), stmt->GetSrcFileLineNum());
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

bool ASTArraySubscriptExpr::InsertBoundaryChecking(std::list<UniqueFEIRStmt> &stmts,
                                                   UniqueFEIRExpr indexExpr, UniqueFEIRExpr baseAddrFEExpr) const {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic()) {
    return false;
  }
  if (arrayType->GetKind() == MIRTypeKind::kTypeArray) {
    // checking all type of indexes, including constant indexes
    while (baseAddrFEExpr != nullptr && baseAddrFEExpr->GetKind() == kExprAddrofArray) {
      baseAddrFEExpr = static_cast<FEIRExprAddrofArray*>(baseAddrFEExpr.get())->GetExprArray()->Clone();
    }
  } else {
    baseAddrFEExpr = ENCChecker::FindBaseExprInPointerOperation(baseAddrFEExpr);
    if (baseAddrFEExpr == nullptr) {
      return false;
    }
  }
  // peel nested boundary checking in a multi-dimensional array
  ENCChecker::PeelNestedBoundaryChecking(stmts, baseAddrFEExpr);
  // insert lower boundary chencking, baseExpr will be replace by lower boundary var when FEIRStmtNary GenMIRStmts
  std::list<UniqueFEIRExpr> lowerExprs;
  lowerExprs.emplace_back(indexExpr->Clone());
  lowerExprs.emplace_back(baseAddrFEExpr->Clone());
  auto lowerStmt = std::make_unique<FEIRStmtAssertBoundary>(OP_assertge, std::move(lowerExprs));
  lowerStmt->SetIsComputable(true);
  lowerStmt->SetSrcLoc(loc);
  stmts.emplace_back(std::move(lowerStmt));
  // insert upper boundary chencking, baseExpr will be replace by upper boundary var when FEIRStmtNary GenMIRStmts
  std::list<UniqueFEIRExpr> upperExprs;
  upperExprs.emplace_back(std::move(indexExpr));
  upperExprs.emplace_back(std::move(baseAddrFEExpr));
  auto upperStmt = std::make_unique<FEIRStmtAssertBoundary>(OP_assertlt, std::move(upperExprs));
  upperStmt->SetIsComputable(true);
  upperStmt->SetSrcLoc(loc);
  stmts.emplace_back(std::move(upperStmt));
  return true;
}

bool ASTUODerefExpr::InsertBoundaryChecking(std::list<UniqueFEIRStmt> &stmts, UniqueFEIRExpr expr) const {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic()) {
    return false;
  }
  UniqueFEIRExpr baseExpr = ENCChecker::FindBaseExprInPointerOperation(expr);
  if (baseExpr == nullptr) {
    return false;
  }
  // peel nested boundary checking in a multi-dereference
  ENCChecker::PeelNestedBoundaryChecking(stmts, baseExpr);
  // insert lower boundary chencking, baseExpr will be replace by lower boundary var when FEIRStmtNary GenMIRStmts
  std::list<UniqueFEIRExpr> lowerExprs;
  lowerExprs.emplace_back(expr->Clone());
  lowerExprs.emplace_back(baseExpr->Clone());
  auto lowerStmt = std::make_unique<FEIRStmtAssertBoundary>(OP_assertge, std::move(lowerExprs));
  lowerStmt->SetIsComputable(expr->GetKind() == kExprBinary);
  lowerStmt->SetSrcLoc(loc);
  stmts.emplace_back(std::move(lowerStmt));
  // insert upper boundary chencking, baseExpr will be replace by upper boundary var when FEIRStmtNary GenMIRStmts
  std::list<UniqueFEIRExpr> upperExprs;
  upperExprs.emplace_back(expr->Clone());
  upperExprs.emplace_back(std::move(baseExpr));
  auto upperStmt = std::make_unique<FEIRStmtAssertBoundary>(OP_assertlt, std::move(upperExprs));
  upperStmt->SetIsComputable(expr->GetKind() == kExprBinary);
  upperStmt->SetSrcLoc(loc);
  stmts.emplace_back(std::move(upperStmt));
  return true;
}

void ENCChecker::ReduceBoundaryChecking(std::list<UniqueFEIRStmt> &stmts, const UniqueFEIRExpr &expr) {
  // assert* --> calcassert*, when addrof the dereference, e.g. &arr[i]
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic()) {
    return;
  }
  std::list<UniqueFEIRStmt>::iterator iter = stmts.begin();
  for (; iter != stmts.end(); ++iter) {
    if ((*iter)->GetKind() != kStmtNary) {
      continue;
    }
    FEIRStmtNary *nary = static_cast<FEIRStmtNary*>((*iter).get());
    if (nary->GetOP() != OP_assertge ||
        nary->GetArgExprs().front()->Hash() != expr->Hash()) {  // addrof expr and index expr of checking are consistent
      continue;
    }
    nary->SetOP(OP_calcassertge);
    std::list<UniqueFEIRStmt>::iterator nextedIter = std::next(iter, 1);
    if (nextedIter != stmts.end() && (*nextedIter)->GetKind() == kStmtNary) {
      FEIRStmtNary *nextedNary = static_cast<FEIRStmtNary*>((*nextedIter).get());
      if (nextedNary->GetOP() == OP_assertlt) {
        nextedNary->SetOP(OP_calcassertlt);
      }
    }
    break;
  }
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
    realLenExpr = FEIRBuilder::CreateExprBinary(OP_add, std::move(argExpr), std::move(realLenExpr));
    exprs.emplace_back(std::move(realLenExpr));
    exprs.emplace_back(std::move(baseExpr));
    UniqueFEIRStmt stmt = std::make_unique<FEIRStmtCallAssertBoundary>(OP_callassertle, std::move(exprs),
                                                                       GetFuncName(), i);
    stmt->SetSrcLoc(loc);
    stmts.emplace_back(std::move(stmt));
  }
}

void ASTCallExpr::InsertBoundaryCheckingInArgsForICall(std::list<UniqueFEIRStmt> &stmts,
                                                       const UniqueFEIRExpr &calleeFEExpr) const {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic() || calleeFEExpr == nullptr) {
    return;
  }
  const MIRFuncType *funcType = FEUtils::GetFuncPtrType(*calleeFEExpr->GetType()->GenerateMIRType());
  if (funcType == nullptr) {
    return;
  }
  const std::vector<TypeAttrs> &attrsVec = funcType->GetParamAttrsList();
  const std::vector<TyIdx> typesVec = funcType->GetParamTypeList();
  std::list<UniqueFEIRStmt> nullStmts;
  for (size_t i = 0; i < attrsVec.size() && i < typesVec.size() && i < args.size(); ++i) {
    MIRType *ptrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(typesVec[i]);
    UniqueFEIRExpr lenExpr = ENCChecker::GetRealBoundaryLenExprInFuncByIndex(attrsVec[i], *ptrType, *this);
    if (lenExpr == nullptr) {
      continue;
    }
    UniqueFEIRExpr argExpr = args[i]->Emit2FEExpr(nullStmts);
    UniqueFEIRExpr baseExpr = ENCChecker::FindBaseExprInPointerOperation(argExpr);
    if (baseExpr == nullptr) {
      continue;
    }
    std::list<UniqueFEIRExpr> exprs;
    lenExpr = FEIRBuilder::CreateExprBinary(OP_add, std::move(argExpr), std::move(lenExpr));
    exprs.emplace_back(std::move(lenExpr));
    exprs.emplace_back(std::move(baseExpr));
    UniqueFEIRStmt stmt = std::make_unique<FEIRStmtCallAssertBoundary>(
        OP_callassertle, std::move(exprs), "function_pointer", i);
    stmt->SetSrcLoc(loc);
    stmts.emplace_back(std::move(stmt));
  }
}

void ASTCallExpr::InsertBoundaryVarInRet(std::list<UniqueFEIRStmt> &stmts) const {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic() || !IsNeedRetExpr()) {
    return;
  }
  std::list<UniqueFEIRStmt> nullStmts;
  UniqueFEIRExpr realLenExpr = nullptr;
  if (funcDecl != nullptr && funcDecl->GetBoundaryLenExpr() != nullptr) {  // call
    realLenExpr = ENCChecker::GetRealBoundaryLenExprInFunc(
        funcDecl->GetBoundaryLenExpr()->Emit2FEExpr(stmts), *funcDecl, *this);
  } else if (isIcall && calleeExpr != nullptr) {  // icall
    const MIRFuncType *funcType = FEUtils::GetFuncPtrType(
        *calleeExpr->Emit2FEExpr(nullStmts)->GetType()->GenerateMIRType());
    if (funcType != nullptr) {
      MIRType *ptrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(funcType->GetRetTyIdx());
      if (ptrType != nullptr) {
        realLenExpr = ENCChecker::GetRealBoundaryLenExprInFuncByIndex(funcType->GetRetAttrs(), *ptrType, *this);
      }
    }
  }
  if (realLenExpr == nullptr) {
    return;
  }
  // GetCurrentFunction need to be optimized when parallel features
  MIRFunction *curFunction = FEManager::GetMIRBuilder().GetCurrentFunctionNotNull();
  ENCChecker::InitBoundaryVar(*curFunction, GetRetVarName(), *retType, std::move(realLenExpr), stmts);
}

bool ENCChecker::IsSameBoundary(const AttrBoundary &arg1, const AttrBoundary &arg2) {
  if (arg1.IsBytedLen() != arg2.IsBytedLen()) {
    return false;
  }
  if (arg1.GetLenExprHash() != 0 && arg1.GetLenExprHash() == arg2.GetLenExprHash()) {
    return true;
  }
  if (arg1.GetLenParamIdx() != -1 && arg1.GetLenParamIdx() == arg2.GetLenParamIdx()) {
    return true;
  }
  if (arg1.GetLenExprHash() == arg2.GetLenExprHash() && arg1.GetLenParamIdx() == arg2.GetLenParamIdx()) {
    return true;
  }
  return false;
}

void ENCChecker::CheckBoundaryArgsAndRetForFuncPtr(const MIRType &dstType, const UniqueFEIRExpr &srcExpr,
                                                   const Loc &loc) {
  const MIRFuncType *funcType = FEUtils::GetFuncPtrType(dstType);
  if (funcType == nullptr) {
    return;
  }
  if (srcExpr->GetKind() == kExprAddrofFunc) {  // check func ptr l-value and &func decl r-value
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(
        static_cast<FEIRExprAddrofFunc*>(srcExpr.get())->GetFuncAddr());
    MIRFunction *srcFunc = FEManager::GetTypeManager().GetMIRFunction(strIdx, false);
    CHECK_FATAL(srcFunc != nullptr, "can not get MIRFunction");
    std::list<size_t> errIdxs;
    for (size_t i = 0; i < srcFunc->GetParamSize() && i < funcType->GetParamAttrsList().size(); ++i) {
      if (!IsSameBoundary(
          srcFunc->GetNthParamAttr(i).GetAttrBoundary(), funcType->GetNthParamAttrs(i).GetAttrBoundary())) {
        errIdxs.emplace_back(i);
      }
    }
    if (!errIdxs.empty()) {
      FE_ERR(kLncErr, "%s:%d error: function pointer and target function's boundary attributes are mismatched "
             "for the %s argument", FEManager::GetModule().GetFileNameFromFileNum(loc.fileIdx).c_str(), loc.line,
             PrintParamIdx(errIdxs).c_str());
    }
    if (!IsSameBoundary(srcFunc->GetFuncAttrs().GetAttrBoundary(), funcType->GetRetAttrs().GetAttrBoundary())) {
      FE_ERR(kLncErr, "%s:%d error: function pointer and target function's boundary attributes are mismatched "
             "for the return value", FEManager::GetModule().GetFileNameFromFileNum(loc.fileIdx).c_str(), loc.line);
    }
  }
  const MIRFuncType *srcFuncType = FEUtils::GetFuncPtrType(*srcExpr->GetType()->GenerateMIRTypeAuto());
  if (srcFuncType != nullptr) {  // check func ptr l-value and func ptr r-value
    std::list<size_t> errIdxs;
    for (size_t i = 0; i < srcFuncType->GetParamAttrsList().size() && i < funcType->GetParamAttrsList().size(); ++i) {
      if (!IsSameBoundary(
          srcFuncType->GetNthParamAttrs(i).GetAttrBoundary(), funcType->GetNthParamAttrs(i).GetAttrBoundary())) {
        errIdxs.emplace_back(i);
      }
    }
    if (!errIdxs.empty()) {
      FE_ERR(kLncErr, "%s:%d error: function pointer's boundary attributes are mismatched for the %s argument",
             FEManager::GetModule().GetFileNameFromFileNum(loc.fileIdx).c_str(), loc.line,
             PrintParamIdx(errIdxs).c_str());
    }
    if (!IsSameBoundary(srcFuncType->GetRetAttrs().GetAttrBoundary(), funcType->GetRetAttrs().GetAttrBoundary())) {
      FE_ERR(kLncErr, "%s:%d error: function pointer's boundary attributes are mismatched for the return value",
             FEManager::GetModule().GetFileNameFromFileNum(loc.fileIdx).c_str(), loc.line);
    }
  }
}

void FEIRStmtDAssign::CheckBoundaryArgsAndRetForFuncPtr(const MIRBuilder &mirBuilder) const {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic() || ENCChecker::IsUnsafeRegion(mirBuilder)) {
    return;
  }
  MIRType *baseType = var->GetType()->GenerateMIRTypeAuto();
  if (fieldID != 0) {
    baseType = FEUtils::GetStructFieldType(static_cast<MIRStructType*>(baseType), fieldID);
  }
  ENCChecker::CheckBoundaryArgsAndRetForFuncPtr(*baseType, expr, loc);
}

void FEIRStmtIAssign::CheckBoundaryArgsAndRetForFuncPtr(const MIRBuilder &mirBuilder, const MIRType &baseType) const {
  if (!FEOptions::GetInstance().IsBoundaryCheckDynamic() || ENCChecker::IsUnsafeRegion(mirBuilder)) {
    return;
  }
  MIRType *fieldType = FEUtils::GetStructFieldType(static_cast<const MIRStructType*>(&baseType), fieldID);
  ENCChecker::CheckBoundaryArgsAndRetForFuncPtr(*fieldType, baseExpr, loc);
}

// ---------------------------
// process safe region
// ---------------------------
bool ENCChecker::IsSafeRegion(const MIRBuilder &mirBuilder) {
  bool isSafe = false;
  if (FEOptions::GetInstance().IsEnableSafeRegion()) {
    isSafe = mirBuilder.GetCurrentFunctionNotNull()->GetAttr(FUNCATTR_safed);
    if (!FEManager::GetCurrentFEFunction().GetSafeRegionFlag().empty()) {
      isSafe = FEManager::GetCurrentFEFunction().GetSafeRegionFlag().top();
    }
  }
  return isSafe;
}

bool ENCChecker::IsUnsafeRegion(const MIRBuilder &mirBuilder) {
  bool isUnsafe = false;
  if (FEOptions::GetInstance().IsEnableSafeRegion()) {
    isUnsafe = mirBuilder.GetCurrentFunctionNotNull()->GetAttr(FUNCATTR_unsafed);
    if (!FEManager::GetCurrentFEFunction().GetSafeRegionFlag().empty()) {
      isUnsafe = !FEManager::GetCurrentFEFunction().GetSafeRegionFlag().top();
    }
  }
  return isUnsafe;
}
}
