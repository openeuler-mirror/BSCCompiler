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
#include "ast_decl.h"
#include "ast_parser.h"
#include "global_tables.h"
#include "ast_stmt.h"
#include "feir_var_name.h"
#include "feir_builder.h"

namespace maple {
// ---------- ASTDecl ---------
const std::string &ASTDecl::GetSrcFileName() const {
  return srcFileName;
}

const std::string &ASTDecl::GetName() const {
  return name;
}

const std::vector<MIRType*> &ASTDecl::GetTypeDesc() const {
  return typeDesc;
}

MIRConst *ASTDecl::Translate2MIRConst() const {
  return Translate2MIRConstImpl();
}

std::string ASTDecl::GenerateUniqueVarName() {
  // add `_line_column` suffix for avoiding local var name conflict
  if (isGlobalDecl || isParam) {
    return name;
  } else {
    return name + "_" + std::to_string(pos.first) + "_" + std::to_string(pos.second);
  }
}

// ---------- ASTVar ----------
std::unique_ptr<FEIRVar> ASTVar::Translate2FEIRVar() {
  CHECK_FATAL(typeDesc.size() == 1, "Invalid ASTVar");
  auto feirVar =
      std::make_unique<FEIRVarName>(GenerateUniqueVarName(), std::make_unique<FEIRTypeNative>(*(typeDesc[0])));
  feirVar->SetGlobal(isGlobalDecl);
  return feirVar;
}

MIRConst *ASTVar::Translate2MIRConstImpl() const {
  return initExpr->GenerateMIRConst();
}

void ASTVar::GenerateInitStmt4StringLiteral(ASTExpr *initASTExpr, UniqueFEIRVar feirVar, UniqueFEIRExpr initFeirExpr,
                                            std::list<UniqueFEIRStmt> &stmts) {
  if (!static_cast<ASTStringLiteral*>(initASTExpr)->IsArrayToPointerDecay()) {
    std::unique_ptr<std::list<UniqueFEIRExpr>> argExprList = std::make_unique<std::list<UniqueFEIRExpr>>();
    UniqueFEIRExpr dstExpr = FEIRBuilder::CreateExprAddrofVar(feirVar->Clone());
    uint32 stringLiteralSize = static_cast<FEIRExprAddrofConstArray*>(initFeirExpr.get())->GetStringLiteralSize();
    auto uDstExpr = dstExpr->Clone();
    auto uSrcExpr = initFeirExpr->Clone();
    argExprList->emplace_back(std::move(uDstExpr));
    argExprList->emplace_back(std::move(uSrcExpr));
    argExprList->emplace_back(FEIRBuilder::CreateExprConstI32(stringLiteralSize));
    std::unique_ptr<FEIRStmtIntrinsicCallAssign> memcpyStmt = std::make_unique<FEIRStmtIntrinsicCallAssign>(
        INTRN_C_memcpy, nullptr, nullptr, std::move(argExprList));
    stmts.emplace_back(std::move(memcpyStmt));

    MIRType *mirArrayType = feirVar->GetType()->GenerateMIRTypeAuto();
    if (mirArrayType->GetKind() != kTypeArray) {
      return;
    }
    auto allSize = static_cast<MIRArrayType*>(mirArrayType)->GetSize();
    auto elemSize = static_cast<MIRArrayType*>(mirArrayType)->GetElemType()->GetSize();
    CHECK_FATAL(elemSize != 0, "elemSize should not 0");
    auto allElemCnt = allSize / elemSize;
    uint32 needInitFurtherCnt = allElemCnt - stringLiteralSize;
    if (needInitFurtherCnt > 0) {
      std::unique_ptr<std::list<UniqueFEIRExpr>> argExprList = std::make_unique<std::list<UniqueFEIRExpr>>();
      auto addExpr = FEIRBuilder::CreateExprBinary(OP_add, std::move(dstExpr),
          FEIRBuilder::CreateExprConstI32(stringLiteralSize));
      argExprList->emplace_back(std::move(addExpr));
      argExprList->emplace_back(FEIRBuilder::CreateExprConstI32(0));
      argExprList->emplace_back(FEIRBuilder::CreateExprConstI32(needInitFurtherCnt));
      std::unique_ptr<FEIRStmtIntrinsicCallAssign> memsetStmt = std::make_unique<FEIRStmtIntrinsicCallAssign>(
          INTRN_C_memset, nullptr, nullptr, std::move(argExprList));
      stmts.emplace_back(std::move(memsetStmt));
    }
    return;
  }
}

void ASTVar::GenerateInitStmtImpl(std::list<UniqueFEIRStmt> &stmts) {
  if (genAttrs.GetAttr(GenericAttrKind::GENATTR_static)) {
    return;
  }
  ASTExpr *initASTExpr = GetInitExpr();
  if (initASTExpr == nullptr) {
    return;
  }
  UniqueFEIRVar feirVar = Translate2FEIRVar();
  UniqueFEIRExpr initFeirExpr = initASTExpr->Emit2FEExpr(stmts);
  if (initFeirExpr == nullptr) {
    return;
  }
  if (initASTExpr->GetASTOp() == kASTStringLiteral) { // init for StringLiteral
    return GenerateInitStmt4StringLiteral(initASTExpr, feirVar->Clone(), initFeirExpr->Clone(), stmts);
  }

  PrimType srcPrimType = initFeirExpr->GetPrimType();
  UniqueFEIRStmt stmt;
  if (srcPrimType != feirVar->GetType()->GetPrimType() && srcPrimType != PTY_agg && srcPrimType != PTY_void) {
    UniqueFEIRExpr cvtExpr = FEIRBuilder::CreateExprCvtPrim(std::move(initFeirExpr), feirVar->GetType()->GetPrimType());
    stmt = FEIRBuilder::CreateStmtDAssign(std::move(feirVar), std::move(cvtExpr));
  } else {
    stmt = FEIRBuilder::CreateStmtDAssign(std::move(feirVar), std::move(initFeirExpr));
  }
  stmts.emplace_back(std::move(stmt));
}

// ---------- ASTLocalEnumDecl ----------
void ASTLocalEnumDecl::GenerateInitStmtImpl(std::list<UniqueFEIRStmt> &stmts) {
  for (auto var : vars) {
    var->GenerateInitStmt(stmts);
  }
}

// ---------- ASTFunc ---------
void ASTFunc::SetCompoundStmt(ASTStmt *astCompoundStmt) {
  compound = astCompoundStmt;
}

void ASTFunc::InsertStmtsIntoCompoundStmtAtFront(const std::list<ASTStmt*> &stmts) {
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
  const std::list<ASTStmt*> &astStmtList = astCpdStmt->GetASTStmtList();
  for (auto stmtNode : astStmtList) {
    std::list<UniqueFEIRStmt> childStmts = stmtNode->Emit2FEStmt();
    for (auto &stmt : childStmts) {
      // Link jump stmt not implemented yet
      stmts.emplace_back(std::move(stmt));
    }
  }
  // fix int main() no return 0 and void func() no return. there are multiple branches, insert return at the end.
  if (stmts.size() == 0 || stmts.back()->GetKind() != kStmtReturn) {
    UniqueFEIRExpr retExpr = nullptr;
    PrimType retType = typeDesc[1]->GetPrimType();
    if (retType != PTY_void) {
      if (!typeDesc[1]->IsScalarType()) {
        retType = PTY_i32;
      }
      retExpr = FEIRBuilder::CreateExprConstAnyScalar(retType, static_cast<int64>(0));
    }
    UniqueFEIRStmt retStmt = std::make_unique<FEIRStmtReturn>(std::move(retExpr));
    stmts.emplace_back(std::move(retStmt));
  }
  return stmts;
}
// ---------- ASTStruct ----------
std::string ASTStruct::GetStructName(bool mapled) const {
  return mapled ? namemangler::EncodeName(name) : name;
}
}  // namespace maple