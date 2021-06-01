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
#include "ast_expr.h"
#include "ast_decl.h"
#include "ast_macros.h"
#include "ast_decl_builder.h"
#include "ast_interface.h"
#include "ast_util.h"
#include "ast_input.h"
#include "ast_stmt.h"
#include "ast_parser.h"
#include "feir_stmt.h"
#include "feir_builder.h"
#include "fe_utils_ast.h"
#include "feir_type_helper.h"
#include "fe_manager.h"
#include "mir_module.h"
#include "mpl_logging.h"

namespace maple {
std::map<std::string, ASTCallExpr::FuncPtrBuiltinFunc> ASTCallExpr::InitBuiltinFuncPtrMap() {
  std::map<std::string, FuncPtrBuiltinFunc> ans;
  ans["alloca"] = &ASTCallExpr::EmitBuiltinAlloca;
  ans["__builtin_ctz"] = &ASTCallExpr::EmitBuiltinCtz;
  ans["__builtin_clz"] = &ASTCallExpr::EmitBuiltinClz;
  ans["__builtin_alloca"] = &ASTCallExpr::EmitBuiltinAlloca;
  ans["__builtin_expect"] = &ASTCallExpr::EmitBuiltinExpect;
  return ans;
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinFunc(std::list<UniqueFEIRStmt> &stmts) const {
    return (this->*(builtingFuncPtrMap[funcName]))(stmts);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinCtz(std::list<UniqueFEIRStmt> &stmts) const {
  auto feTy = std::make_unique<FEIRTypeNative>(*mirType);
  std::vector<std::unique_ptr<FEIRExpr>> argOpnds;
  for (auto arg : args) {
    argOpnds.push_back(arg->Emit2FEExpr(stmts));
  }
#ifndef USE_OPS
  CHECK_FATAL(false, "implemention in ops branch");
  return nullptr;
#else
  if (mirType->GetSize() == 4) {
    // 32 bit
    return std::make_unique<FEIRExprIntrinsicopForC>(std::move(feTy), INTRN_C_ctz32, argOpnds);
  }
  return std::make_unique<FEIRExprIntrinsicopForC>(std::move(feTy), INTRN_C_ctz32, argOpnds);
#endif
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinClz(std::list<UniqueFEIRStmt> &stmts) const {
  auto feTy = std::make_unique<FEIRTypeNative>(*mirType);
  std::vector<std::unique_ptr<FEIRExpr>> argOpnds;
  for (auto arg : args) {
    argOpnds.push_back(arg->Emit2FEExpr(stmts));
  }
#ifndef USE_OPS
  CHECK_FATAL(false, "implemention in ops branch");
  return nullptr;
#else
  if (mirType->GetSize() == 4) {
    // 32 bit
    return std::make_unique<FEIRExprIntrinsicopForC>(std::move(feTy), INTRN_C_clz32, argOpnds);
  }
  return std::make_unique<FEIRExprIntrinsicopForC>(std::move(feTy), INTRN_C_clz64, argOpnds);
#endif
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinAlloca(std::list<UniqueFEIRStmt> &stmts) const {
  auto arg = args[0]->Emit2FEExpr(stmts);
  auto alloca = std::make_unique<FEIRExprUnary>(OP_alloca, mirType, std::move(arg));
  return alloca;
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinExpect(std::list<UniqueFEIRStmt> &stmts) const {
  ASSERT(args.size() == 2, "__builtin_expect requires two arguments");
  auto arg1Expr = args[1]->Emit2FEExpr(stmts);
  std::list<std::unique_ptr<FEIRExpr>> argExprsIn;
  argExprsIn.push_back(std::move(arg1Expr));
  auto stmt = std::make_unique<FEIRStmtNary>(OP_eval, std::move(argExprsIn));
  return args[0]->Emit2FEExpr(stmts);
}

std::map<std::string, ASTParser::FuncPtrBuiltinFunc> ASTParser::InitBuiltinFuncPtrMap() {
  std::map<std::string, FuncPtrBuiltinFunc> ans;
  ans["__builtin_classify_type"] = &ASTParser::ParseBuiltinClassifyType;
  ans["__builtin_constant_p"] = &ASTParser::ParseBuiltinConstantP;
  ans["__builtin_signbit"] = &ASTParser::ParseBuiltinSignbit;
  ans["__builtin_isinf_sign"] = &ASTParser::ParseBuiltinIsinfsign;
  return ans;
}

ASTExpr *ASTParser::ParseBuiltinFunc(MapleAllocator &allocator, const clang::CallExpr &expr,
                                     std::stringstream &ss) const {
  return (this->*(builtingFuncPtrMap[ss.str()]))(allocator, expr, ss);
}

ASTExpr *ASTParser::ParseBuiltinClassifyType(MapleAllocator &allocator, const clang::CallExpr &expr,
                                             std::stringstream &ss) const {
  (void)ss;
  clang::Expr::EvalResult res;
  bool success = expr.EvaluateAsInt(res, *(astFile->GetContext()));
  CHECK_FATAL(success, "Failed to evaluate __builtin_classify_type");
  llvm::APSInt apVal = res.Val.getInt();
  ASTIntegerLiteral *astIntegerLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
  astIntegerLiteral->SetVal(static_cast<uint64>(apVal.getExtValue()));
  astIntegerLiteral->SetType(PTY_i32);
  return astIntegerLiteral;
}

ASTExpr *ASTParser::ParseBuiltinConstantP(MapleAllocator &allocator, const clang::CallExpr &expr,
                                          std::stringstream &ss) const {
  (void)ss;
  int constP = expr.getArg(0)->isConstantInitializer(*astFile->GetNonConstAstContext(), false) ? 1 : 0;
  // Pointers are not considered constant
  if (expr.getArg(0)->getType()->isPointerType() &&
      !llvm::isa<clang::StringLiteral>(expr.getArg(0)->IgnoreParenCasts())) {
    constP = 0;
  }
  ASTIntegerLiteral *astIntegerLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
  astIntegerLiteral->SetVal(static_cast<uint64>(constP));
  astIntegerLiteral->SetType(astFile->CvtType(expr.getType())->GetPrimType());
  return astIntegerLiteral;
}

ASTExpr *ASTParser::ParseBuiltinSignbit(MapleAllocator &allocator, const clang::CallExpr &expr,
                                        std::stringstream &ss) const {
  (void)allocator;
  (void)expr;
  ss.clear();
  ss.str(std::string());
  ss << "__signbit";
  return nullptr;
}

ASTExpr *ASTParser::ParseBuiltinIsinfsign(MapleAllocator &allocator, const clang::CallExpr &expr,
                                          std::stringstream &ss) const {
  (void)allocator;
  (void)expr;
  ss.clear();
  ss.str(std::string());
  if (astFile->CvtType(expr.getArg(0)->getType())->GetPrimType() == PTY_f64) {
    ss << "__isinf";
  } else if (astFile->CvtType(expr.getArg(0)->getType())->GetPrimType() == PTY_f32) {
    ss << "__isinff";
  } else {
    ASSERT(false, "Unsupported type passed to isinf");
  }
  return nullptr;
}
} // namespace maple