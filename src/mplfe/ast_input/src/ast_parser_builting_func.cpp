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
#include "ast_parser.h"
#include "mpl_logging.h"
#include "mir_module.h"
#include "mpl_logging.h"
#include "ast_decl_builder.h"
#include "ast_interface.h"
#include "ast_decl.h"
#include "ast_macros.h"
#include "ast_util.h"
#include "ast_input.h"
#include "fe_manager.h"

namespace maple {
std::map<std::string, ASTParser::ParseBuiltinFunc> ASTParser::InitFuncPtrMap() {
  std::map<std::string, ParseBuiltinFunc> ans;
  ans["__builtin_classify_type"] = &ASTParser::ParseBuiltingClassifyType;
  ans["__builtin_ctz"] = &ASTParser::ParseBuiltingCtz;
  ans["__builtin_clz"] = &ASTParser::ParseBuiltingClz;
  ans["__builtin_alloca"] = &ASTParser::ParseBuiltingAlloca;
  ans["__builtin_constant_p"] = &ASTParser::ParseBuiltingConstantP;
  ans["__builtin_expect"] = &ASTParser::ParseBuiltingExpect;
  ans["__builtin_signbit"] = &ASTParser::ParseBuiltingSignbit;
  ans["__builtin_isinf_sign"] = &ASTParser::ParseBuiltingIsinfSign;
  return ans;
}

ASTExpr *ASTParser::ParseBuiltingClassifyType(MapleAllocator &allocator, const clang::CallExpr &expr) const {
  clang::Expr::EvalResult res;
  bool success = expr.EvaluateAsInt(res, *(astFile->GetContext()));
  CHECK_FATAL(success, "Failed to evaluate __builtin_classify_type");
  llvm::APSInt apVal = res.Val.getInt();
  ASTIntegerLiteral *astIntegerLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
  astIntegerLiteral->SetVal(static_cast<uint64>(apVal.getExtValue()));
  astIntegerLiteral->SetType(PTY_i32);
  return astIntegerLiteral;
}

ASTExpr *ASTParser::ParseBuiltingCtz(MapleAllocator &allocator, const clang::CallExpr &expr) const {
  return nullptr;
}

ASTExpr *ASTParser::ParseBuiltingClz(MapleAllocator &allocator, const clang::CallExpr &expr) const {
  return nullptr;
}

ASTExpr *ASTParser::ParseBuiltingAlloca(MapleAllocator &allocator, const clang::CallExpr &expr) const {
  return nullptr;
}

ASTExpr *ASTParser::ParseBuiltingConstantP(MapleAllocator &allocator, const clang::CallExpr &expr) const {
  return nullptr;
}

ASTExpr *ASTParser::ParseBuiltingExpect(MapleAllocator &allocator, const clang::CallExpr &expr) const {
  return nullptr;
}

ASTExpr *ASTParser::ParseBuiltingSignbit(MapleAllocator &allocator, const clang::CallExpr &expr) const {
  return nullptr;
}

ASTExpr *ASTParser::ParseBuiltingIsinfSign(MapleAllocator &allocator, const clang::CallExpr &expr) const {
  return nullptr;
}
} // namespace maple