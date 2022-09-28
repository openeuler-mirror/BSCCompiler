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
#include "ast_decl.h"
#include "ast_parser.h"
#include "global_tables.h"
#include "ast_stmt.h"
#include "feir_var_name.h"
#include "feir_builder.h"
#include "fe_manager.h"
#include "enhance_c_checker.h"
#include "conditional_operator.h"

namespace maple {
// ---------- ASTDecl ---------
const std::string ASTDecl::GetSrcFileName() const {
  return srcFileName.c_str() == nullptr ? "" : srcFileName.c_str();
}

const std::string ASTDecl::GetName() const {
  return name.c_str() == nullptr ? "" : name.c_str();
}

const MapleVector<MIRType*> &ASTDecl::GetTypeDesc() const {
  return typeDesc;
}

void ASTDecl::SetTypeDesc(const MapleVector<MIRType*> &typeVecIn) {
  typeDesc = typeVecIn;
}

MIRConst *ASTDecl::Translate2MIRConst() const {
  return Translate2MIRConstImpl();
}

std::string ASTDecl::GenerateUniqueVarName() const {
  // add `_line_column` suffix for avoiding local var name conflict
  if (isGlobalDecl || isParam) {
    return GetName();
  } else {
    std::stringstream os;
    os << GetName();
    if (isMacroID != 0) {
      // for macro expansion, variable names of same location need to be unique
      os << "_" << std::to_string(isMacroID);
    } else {
      os << "_" << std::to_string(loc.line) << "_" << std::to_string(loc.column);
    }
    return os.str();
  }
}

// ---------- ASTVar ----------
std::unique_ptr<FEIRVar> ASTVar::Translate2FEIRVar() const {
  CHECK_FATAL(typeDesc.size() == 1, "Invalid ASTVar");
  auto feirVar =
      std::make_unique<FEIRVarName>(GenerateUniqueVarName(), std::make_unique<FEIRTypeNative>(*(typeDesc[0])));
  feirVar->SetGlobal(isGlobalDecl);
  feirVar->SetAttrs(genAttrs);
  feirVar->SetSrcLoc(loc);
  feirVar->SetSectionAttr(sectionAttr);
  if (boundary.lenExpr != nullptr) {
    std::list<UniqueFEIRStmt> nullStmts;
    UniqueFEIRExpr lenExpr = boundary.lenExpr->Emit2FEExpr(nullStmts);
    feirVar->SetBoundaryLenExpr(std::move(lenExpr));
  }
  return feirVar;
}

MIRConst *ASTVar::Translate2MIRConstImpl() const {
  return initExpr->GenerateMIRConst();
}

void ASTVar::GenerateInitStmt4StringLiteral(const ASTExpr *initASTExpr, const UniqueFEIRVar &feirVar,
    const UniqueFEIRExpr &initFeirExpr, std::list<UniqueFEIRStmt> &stmts) const {
  if (!static_cast<const ASTStringLiteral*>(initASTExpr)->IsArrayToPointerDecay()) {
    std::unique_ptr<std::list<UniqueFEIRExpr>> argExprList = std::make_unique<std::list<UniqueFEIRExpr>>();
    UniqueFEIRExpr dstExpr = FEIRBuilder::CreateExprAddrofVar(feirVar->Clone());
    uint32 stringLiteralSize = static_cast<FEIRExprAddrofConstArray*>(initFeirExpr.get())->GetStringLiteralSize();
    auto uDstExpr = dstExpr->Clone();
    auto uSrcExpr = initFeirExpr->Clone();
    argExprList->emplace_back(std::move(uDstExpr));
    argExprList->emplace_back(std::move(uSrcExpr));
    MIRType *mirArrayType = feirVar->GetType()->GenerateMIRTypeAuto();
    UniqueFEIRExpr sizeExpr;
    if (mirArrayType->GetKind() == kTypeArray &&
      static_cast<MIRArrayType*>(mirArrayType)->GetElemType()->GetSize() != 1) {
      UniqueFEIRExpr leftExpr = FEIRBuilder::CreateExprConstI32(stringLiteralSize);
      size_t elemSizes = static_cast<MIRArrayType*>(mirArrayType)->GetElemType()->GetSize();
      CHECK_FATAL(elemSizes <= INT_MAX, "Too large elem size");
      UniqueFEIRExpr rightExpr = FEIRBuilder::CreateExprConstI32(static_cast<int32>(elemSizes));
      sizeExpr = FEIRBuilder::CreateExprBinary(OP_mul, std::move(leftExpr), std::move(rightExpr));
    } else {
      sizeExpr = FEIRBuilder::CreateExprConstI32(stringLiteralSize);
    }
    argExprList->emplace_back(sizeExpr->Clone());
    std::unique_ptr<FEIRStmtIntrinsicCallAssign> memcpyStmt = std::make_unique<FEIRStmtIntrinsicCallAssign>(
        INTRN_C_memcpy, nullptr, nullptr, std::move(argExprList));
    memcpyStmt->SetSrcLoc(initFeirExpr->GetLoc());
    stmts.emplace_back(std::move(memcpyStmt));
    if (mirArrayType->GetKind() != kTypeArray) {
      return;
    }
    auto allSize = static_cast<MIRArrayType*>(mirArrayType)->GetSize();
    auto elemSize = static_cast<MIRArrayType*>(mirArrayType)->GetElemType()->GetSize();
    CHECK_FATAL(elemSize != 0, "elemSize should not 0");
    auto allElemCnt = allSize / elemSize;
    uint32 needInitFurtherCnt = static_cast<uint32>(allElemCnt - stringLiteralSize);
    if (needInitFurtherCnt > 0) {
      argExprList = std::make_unique<std::list<UniqueFEIRExpr>>();
      auto addExpr = FEIRBuilder::CreateExprBinary(OP_add, std::move(dstExpr), sizeExpr->Clone());
      argExprList->emplace_back(std::move(addExpr));
      argExprList->emplace_back(FEIRBuilder::CreateExprConstI32(0));
      argExprList->emplace_back(FEIRBuilder::CreateExprConstI32(needInitFurtherCnt * elemSize));
      std::unique_ptr<FEIRStmtIntrinsicCallAssign> memsetStmt = std::make_unique<FEIRStmtIntrinsicCallAssign>(
          INTRN_C_memset, nullptr, nullptr, std::move(argExprList));
      memsetStmt->SetSrcLoc(initFeirExpr->GetLoc());
      stmts.emplace_back(std::move(memsetStmt));
    }
    return;
  }
}

void ASTVar::GenerateInitStmtImpl(std::list<UniqueFEIRStmt> &stmts) {
  MIRSymbol *sym = Translate2MIRSymbol();
  ENCChecker::CheckNonnullLocalVarInit(*sym, initExpr);
  UniqueFEIRVar feirVar = Translate2FEIRVar();
  if (FEOptions::GetInstance().IsDbgFriendly() && !hasAddedInMIRScope) {
    FEFunction &feFunction = FEManager::GetCurrentFEFunction();
    MIRScope *mirScope = feFunction.GetTopMIRScope();
    FEUtils::AddAliasInMIRScope(*mirScope, GetName(), *sym, sourceType);
    hasAddedInMIRScope = true;
  }
  if (variableArrayExpr != nullptr) {  // vla declaration point
    FEFunction &feFunction = FEManager::GetCurrentFEFunction();
    if (feFunction.GetTopFEIRScopePtr() != nullptr &&
        feFunction.GetTopFEIRScopePtr()->GetVLASavedStackVar() == nullptr) {
      // stack save
      MIRType *retType = GlobalTables::GetTypeTable().GetOrCreatePointerType(
          *GlobalTables::GetTypeTable().GetPrimType(PTY_void));
      auto stackVar = FEIRBuilder::CreateVarNameForC(FEUtils::GetSequentialName("saved_stack."), *retType, false);
      std::unique_ptr<std::list<UniqueFEIRExpr>> argExprList = std::make_unique<std::list<UniqueFEIRExpr>>();
      auto stackSaveStmt = std::make_unique<FEIRStmtIntrinsicCallAssign>(INTRN_C_stack_save, nullptr, stackVar->Clone(),
                                                                         std::move(argExprList));
      stackSaveStmt->SetSrcLoc(feirVar->GetSrcLoc());
      (void)stmts.emplace_back(std::move(stackSaveStmt));
      // push saved stack var into scope
      feFunction.GetTopFEIRScopePtr()->SetVLASavedStackVar(std::move(stackVar));
    }
    // alloca
    UniqueFEIRExpr variableArrayFEIRExpr = variableArrayExpr->Emit2FEExpr(stmts);
    MIRType *mirType = GlobalTables::GetTypeTable().GetPrimType(PTY_a64);
    UniqueFEIRType feType = std::make_unique<FEIRTypeNative>(*mirType);
    UniqueFEIRExpr allocaExpr = std::make_unique<FEIRExprUnary>(std::move(feType), OP_alloca,
                                                                std::move(variableArrayFEIRExpr));
    UniqueFEIRStmt allocaStmt = FEIRBuilder::CreateStmtDAssign(feirVar->Clone(), std::move(allocaExpr));
    allocaStmt->SetSrcLoc(feirVar->GetSrcLoc());
    (void)stmts.emplace_back(std::move(allocaStmt));
  }
  ENCChecker::InsertBoundaryVar(*this, stmts);
  if (initExpr == nullptr) {
    return;
  }
  if (genAttrs.GetAttr(GENATTR_static) && sym->GetKonst() != nullptr) {
    return;
  }
  UniqueFEIRExpr initFeirExpr = initExpr->Emit2FEExpr(stmts);
  if (initFeirExpr == nullptr) {
    return;
  }
  ENCChecker::CheckNonnullLocalVarInit(*sym, initFeirExpr, stmts);
  if (initExpr->GetASTOp() == kASTStringLiteral) { // init for StringLiteral
    return GenerateInitStmt4StringLiteral(initExpr, feirVar, initFeirExpr, stmts);
  }

  if (ConditionalOptimize::DeleteRedundantTmpVar(initFeirExpr, stmts, feirVar, feirVar->GetType()->GetPrimType())) {
    return;
  }

  PrimType srcPrimType = initFeirExpr->GetPrimType();
  UniqueFEIRStmt stmt;
  if (srcPrimType != feirVar->GetType()->GetPrimType() && srcPrimType != PTY_agg && srcPrimType != PTY_void) {
    auto castExpr = FEIRBuilder::CreateExprCastPrim(std::move(initFeirExpr), feirVar->GetType()->GetPrimType());
    stmt = FEIRBuilder::CreateStmtDAssign(feirVar->Clone(), std::move(castExpr));
  } else {
    stmt = FEIRBuilder::CreateStmtDAssign(feirVar->Clone(), std::move(initFeirExpr));
  }
  stmt->SetSrcLoc(feirVar->GetSrcLoc());
  stmts.emplace_back(std::move(stmt));
}

MIRSymbol *ASTVar::Translate2MIRSymbol() const {
  UniqueFEIRVar feirVar = Translate2FEIRVar();
  MIRSymbol *mirSymbol = feirVar->GenerateMIRSymbol(FEManager::GetMIRBuilder());
  if (initExpr != nullptr && genAttrs.GetAttr(GENATTR_static)) {
    MIRConst *cst = initExpr->GenerateMIRConst();
    if (cst != nullptr && cst->GetKind() != kConstInvalid) {
      mirSymbol->SetKonst(cst);
    }
  }
  if (!sectionAttr.empty()) {
    mirSymbol->sectionAttr = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(sectionAttr);
  }
  if (!asmAttr.empty()) {
    mirSymbol->SetAsmAttr(GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(asmAttr));
  }
  return mirSymbol;
}

// ---------- ASTEnumConstant ----------
void ASTEnumConstant::SetValue(const IntVal &val) {
  value = val;
}

const IntVal &ASTEnumConstant::GetValue() const {
  return value;
}

MIRConst *ASTEnumConstant::Translate2MIRConstImpl() const {
  return GlobalTables::GetIntConstTable().GetOrCreateIntConst(value, *typeDesc.front());
}

// ---------- ASTFunc ---------
void ASTFunc::SetCompoundStmt(ASTStmt *astCompoundStmt) {
  compound = astCompoundStmt;
}

void ASTFunc::InsertStmtsIntoCompoundStmtAtFront(const std::list<ASTStmt*> &stmts) const {
  static_cast<ASTCompoundStmt*>(compound)->InsertASTStmtsAtFront(stmts);
}

const ASTStmt *ASTFunc::GetCompoundStmt() const {
  return compound;
}

std::vector<std::unique_ptr<FEIRVar>> ASTFunc::GenArgVarList() const {
  std::vector<std::unique_ptr<FEIRVar>> args;
  return args;
}

std::list<UniqueFEIRStmt> ASTFunc::EmitASTStmtToFEIR() const {
  std::list<UniqueFEIRStmt> stmts;
  const ASTStmt *astStmt = GetCompoundStmt();
  if (astStmt == nullptr) {
    return stmts;
  }
  const ASTCompoundStmt *astCpdStmt = static_cast<const ASTCompoundStmt*>(astStmt);
  FEFunction &feFunction = FEManager::GetCurrentFEFunction();
  feFunction.PushFuncScope(FEUtils::CvtLoc2SrcPosition(astCpdStmt->GetSrcLoc()),
                           FEUtils::CvtLoc2SrcPosition(astCpdStmt->GetEndLoc()));
  const MapleList<ASTStmt*> &astStmtList = astCpdStmt->GetASTStmtList();
  for (auto stmtNode : astStmtList) {
    std::list<UniqueFEIRStmt> childStmts = stmtNode->Emit2FEStmt();
    for (auto &stmt : childStmts) {
      // Link jump stmt not implemented yet
      stmts.emplace_back(std::move(stmt));
    }
  }
  UniqueFEIRScope scope = feFunction.PopTopScope();
  // fix int main() no return 0 and void func() no return. there are multiple branches, insert return at the end.
  if (stmts.size() == 0 || stmts.back()->GetKind() != kStmtReturn) {
    if (scope->GetVLASavedStackVar() != nullptr) {
      auto stackRestoreStmt = scope->GenVLAStackRestoreStmt();
      stackRestoreStmt->SetSrcLoc(astCpdStmt->GetEndLoc());
      (void)stmts.emplace_back(std::move(stackRestoreStmt));
    }
    UniqueFEIRExpr retExpr = nullptr;
    PrimType retType = typeDesc[1]->GetPrimType();
    if (retType != PTY_void) {
      if (!typeDesc[1]->IsScalarType()) {
        retType = PTY_i32;
      }
      retExpr = FEIRBuilder::CreateExprConstAnyScalar(retType, static_cast<int64>(0));
    }
    UniqueFEIRStmt retStmt = std::make_unique<FEIRStmtReturn>(std::move(retExpr));
    Loc endLoc = astCpdStmt->GetEndLoc();
    endLoc.column = 0;
    retStmt->SetSrcLoc(endLoc);
    stmts.emplace_back(std::move(retStmt));
  }
  InsertBoundaryCheckingInRet(stmts);
  return stmts;
}

// ---------- ASTStruct ----------
std::string ASTStruct::GetStructName(bool mapled) const {
  return mapled ? namemangler::EncodeName(GetName()) : GetName();
}

void ASTStruct::GenerateInitStmtImpl(std::list<UniqueFEIRStmt> &stmts) {
  (void)stmts;
  if (!FEOptions::GetInstance().IsDbgFriendly()) {
    return;
  }
  FEFunction &feFunction = FEManager::GetCurrentFEFunction();
  MIRScope *mirScope = feFunction.GetTopMIRScope();
  mirScope->SetTypeAliasMap(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(GetName()),
                         typeDesc.front()->GetTypeIndex());
}

// ---------- ASTEnumDecl ----------
void ASTEnumDecl::GenerateInitStmtImpl(std::list<UniqueFEIRStmt> &stmts) {
  (void)stmts;
  if (!FEOptions::GetInstance().IsDbgFriendly()) {
    return;
  }
  FEFunction &feFunction = FEManager::GetCurrentFEFunction();
  MIRScope *mirScope = feFunction.GetTopMIRScope();
  MIRTypeByName *type = FEManager::GetTypeManager().GetOrCreateTypeByNameType(GenerateUniqueVarName());
  mirScope->SetTypeAliasMap(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(GetName()), type->GetTypeIndex());
}

// ---------- ASTTypedefDecl ----------
void ASTTypedefDecl::GenerateInitStmtImpl(std::list<UniqueFEIRStmt> &stmts) {
  (void)stmts;
  if (!FEOptions::GetInstance().IsDbgFriendly()) {
    return;
  }
  FEFunction &feFunction = FEManager::GetCurrentFEFunction();
  MIRScope *mirScope = feFunction.GetTopMIRScope();
  const ASTTypedefDecl *astTypedef = this;
  while (astTypedef != nullptr && !astTypedef->isGlobalDecl) {
    MIRTypeByName *type = FEManager::GetTypeManager().CreateTypedef(
        astTypedef->GenerateUniqueVarName(), *astTypedef->GetTypeDesc().front());
    mirScope->SetTypeAliasMap(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(
        astTypedef->GetName()), type->GetTypeIndex());
    astTypedef = astTypedef->GetSubTypedefDecl();
  }
}
}  // namespace maple
