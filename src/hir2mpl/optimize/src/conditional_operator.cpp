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
#include "conditional_operator.h"
#include "feir_builder.h"

namespace maple {
bool ConditionalOptimize::IsCompletedConditional(const UniqueFEIRExpr &expr, std::list<UniqueFEIRStmt> &stmts) {
  if (FEOptions::GetInstance().IsNpeCheckDynamic() || FEOptions::GetInstance().IsBoundaryCheckDynamic()) {
    return false;
  }

  if (expr == nullptr || expr->GetKind() != kExprTernary) {
    return false;
  }

  if (stmts.empty() || stmts.back()->GetKind() != kStmtIf) {
    return false;
  }

  FEIRStmtIf *ifStmt = static_cast<FEIRStmtIf*>(stmts.back().get());
  if (ifStmt->GetThenStmt().empty() || ifStmt->GetElseStmt().empty()) {
    return false;
  }

  return true;
}

bool ConditionalOptimize::DeleteRedundantTmpVar(const UniqueFEIRExpr &expr, std::list<UniqueFEIRStmt> &stmts,
                                                const UniqueFEIRVar &var, PrimType dstPty, FieldID fieldID) {
  if (!IsCompletedConditional(expr, stmts)) {
    return false;
  }

  auto ReplaceBackStmt = [&](std::list<UniqueFEIRStmt> &stmts, const FEIRStmt &srcStmt) {
    auto dassignStmt = static_cast<FEIRStmtDAssign*>(stmts.back().get());
    UniqueFEIRExpr srcExpr = dassignStmt->GetExpr()->Clone();
    PrimType srcPty = srcExpr->GetPrimType();
    if (srcPty != dstPty && srcPty != PTY_agg && srcPty != PTY_void) {
      if (srcPty == PTY_f32 || srcPty == PTY_f64) {
        if (dstPty == PTY_u8 || dstPty == PTY_u16) {
          dstPty = PTY_u32;
        }
        if (dstPty == PTY_i8 || dstPty == PTY_i16) {
          dstPty = PTY_i32;
        }
      }
      srcExpr = FEIRBuilder::CreateExprCastPrim(std::move(srcExpr), dstPty);
    }

    auto stmt = std::make_unique<FEIRStmtDAssign>(var->Clone(), srcExpr->Clone(), fieldID);
    stmt->SetSrcFileInfo(srcStmt.GetSrcFileIdx(), srcStmt.GetSrcFileLineNum());
    stmts.pop_back();
    stmts.emplace_back(std::move(stmt));
  };

  FEIRStmtIf *ifStmt = static_cast<FEIRStmtIf*>(stmts.back().get());
  ReplaceBackStmt(ifStmt->GetThenStmt(), *ifStmt);
  ReplaceBackStmt(ifStmt->GetElseStmt(), *ifStmt);
  return true;
}

bool ConditionalOptimize::DeleteRedundantTmpVar(const UniqueFEIRExpr &expr, std::list<UniqueFEIRStmt> &stmts) {
  if (!IsCompletedConditional(expr, stmts)) {
    return false;
  }

  auto ReplaceBackStmt = [&](std::list<UniqueFEIRStmt> &stmts, const FEIRStmt &srcStmt) {
    auto dassignStmt = static_cast<FEIRStmtDAssign*>(stmts.back().get());
    auto stmt = std::make_unique<FEIRStmtReturn>(dassignStmt->GetExpr()->Clone());
    stmt->SetSrcFileInfo(srcStmt.GetSrcFileIdx(), srcStmt.GetSrcFileLineNum());
    stmts.pop_back();
    stmts.emplace_back(std::move(stmt));
  };

  FEIRStmtIf *ifStmt = static_cast<FEIRStmtIf*>(stmts.back().get());
  ReplaceBackStmt(ifStmt->GetThenStmt(), *ifStmt);
  ReplaceBackStmt(ifStmt->GetElseStmt(), *ifStmt);
  return true;
}
}