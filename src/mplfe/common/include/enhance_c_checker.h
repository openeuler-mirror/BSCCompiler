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
#ifndef MPLFE_INCLUDE_COMMON_ENCCHECKER_H
#define MPLFE_INCLUDE_COMMON_ENCCHECKER_H
#include "feir_var.h"
#include "feir_stmt.h"
#include "ast_expr.h"
#include "ast_decl.h"

namespace maple {
class ENCChecker {
 public:
  ENCChecker() = default;
  ~ENCChecker() = default;
  static bool HasNonnullAttrInExpr(MIRBuilder &mirBuilder, const UniqueFEIRExpr &expr);
  static void CheckNonnullGlobalVarInit(const MIRSymbol &sym, const MIRConst *cst);
  static void CheckNonnullLocalVarInit(const MIRSymbol &sym, const ASTExpr *initExpr, std::list<UniqueFEIRStmt> &stmts);
  static void CheckNonnullArgsAndRetForFuncPtr(const MIRType &dstType, const UniqueFEIRExpr &srcExpr,
                                               uint32 fileNum, uint32 fileLine);
  static bool IsSameBoundary(const AttrBoundary &arg1, const AttrBoundary &arg2);
  static void CheckBoundaryArgsAndRetForFuncPtr(const MIRType &dstType, const UniqueFEIRExpr &srcExpr,
                                                uint32 fileNum, uint32 fileLine);
  static UniqueFEIRExpr FindBaseExprInPointerOperation(const UniqueFEIRExpr &expr);
  static MIRType *GetArrayTypeFromExpr(const UniqueFEIRExpr &expr);
  static void AssignBoundaryVar(MIRBuilder &mirBuilder, const UniqueFEIRExpr &dstExpr, const UniqueFEIRExpr &srcExpr,
                                const UniqueFEIRExpr &lRealLenExpr, std::list<StmtNode*> &ans);
  static void AssignUndefVal(MIRBuilder &mirBuilder, MIRSymbol &sym);
  static std::string GetBoundaryName(const UniqueFEIRExpr &expr);
  static bool IsGlobalVarInExpr(const UniqueFEIRExpr &expr);
  static std::pair<StIdx, StIdx> InsertBoundaryVar(MIRBuilder &mirBuilder, const UniqueFEIRExpr &expr);
  static bool IsConstantIndex(const UniqueFEIRExpr &expr);
  static void PeelNestedBoundaryChecking(std::list<UniqueFEIRStmt> &stmts, const UniqueFEIRExpr &baseExpr);
  static UniqueFEIRExpr GetRealBoundaryLenExprInFunc(const UniqueFEIRExpr &lenExpr, const ASTFunc &astFunc,
                                                     const ASTCallExpr &astCallExpr);
  static UniqueFEIRExpr GetRealBoundaryLenExprInFuncByIndex(const TypeAttrs &typeAttrs, const MIRType &type,
                                                            const ASTCallExpr &astCallExpr);
  static UniqueFEIRExpr GetRealBoundaryLenExprInField(const UniqueFEIRExpr &lenExpr, MIRStructType &baseType,
                                                      const UniqueFEIRExpr &dstExpr);
  static void InitBoundaryVarFromASTDecl(MapleAllocator &allocator, ASTDecl *ptrDecl,
                                         ASTExpr *lenExpr, std::list<ASTStmt*> &stmts);
  static void InitBoundaryVar(MIRFunction &curFunction, const ASTDecl &ptrDecl,
                              UniqueFEIRExpr lenExpr, std::list<UniqueFEIRStmt> &stmts);
  static void InitBoundaryVar(MIRFunction &curFunction, const std::string &ptrName, MIRType &ptrType,
                              UniqueFEIRExpr lenExpr, std::list<UniqueFEIRStmt> &stmts);
  static std::pair<StIdx, StIdx> InitBoundaryVar(MIRFunction &curFunction, const UniqueFEIRExpr &ptrExpr,
                                                 UniqueFEIRExpr lenExpr, std::list<UniqueFEIRStmt> &stmts);
  static UniqueFEIRExpr GetGlobalOrFieldLenExprInExpr(MIRBuilder &mirBuilder, const UniqueFEIRExpr &expr);
  static void InsertBoundaryAssignChecking(MIRBuilder &mirBuilder, std::list<StmtNode*> &ans,
                                           const UniqueFEIRExpr &srcExpr, uint32 fileIdx, uint32 fileLine);
  static UniqueFEIRStmt InsertBoundaryLEChecking(UniqueFEIRExpr lenExpr, const UniqueFEIRExpr &srcExpr);
  static UniqueFEIRExpr GetBoundaryLenExprCache(uint32 hash);
  static UniqueFEIRExpr GetBoundaryLenExprCache(const TypeAttrs &attr);
  static UniqueFEIRExpr GetBoundaryLenExprCache(const FieldAttrs &attr);
  static void InsertBoundaryInAtts(TypeAttrs &attr, const BoundaryInfo &boundary);
  static void InsertBoundaryLenExprInAtts(TypeAttrs &attr, const UniqueFEIRExpr &expr);
  static void InsertBoundaryInAtts(FieldAttrs &attr, const BoundaryInfo &boundary);
  static void InsertBoundaryInAtts(FuncAttrs &attr, const BoundaryInfo &boundary);
};  // class ENCChecker
}  // namespace maple
#endif  // MPLFE_INCLUDE_COMMON_ENCCHECKER_H