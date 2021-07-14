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
std::unordered_map<std::string, ASTCallExpr::FuncPtrBuiltinFunc> ASTCallExpr::InitBuiltinFuncPtrMap() {
  std::unordered_map<std::string, FuncPtrBuiltinFunc> ans;
#define BUILTIN_FUNC_EMIT(funcName, FuncPtrBuiltinFunc) \
  ans[funcName] = FuncPtrBuiltinFunc;
#include "builtin_func_emit.def"
#undef BUILTIN_FUNC_EMIT
  // vector builtinfunc
#define DEF_MIR_INTRINSIC(STR, NAME, INTRN_CLASS, RETURN_TYPE, ...)                                \
  ans["__builtin_mpl_"#STR] = &ASTCallExpr::EmitBuiltin##STR;
#include "intrinsic_vector.def"
#undef DEF_MIR_INTRINSIC

  return ans;
}

UniqueFEIRExpr ASTCallExpr::CreateIntrinsicopForC(std::list<UniqueFEIRStmt> &stmts,
                                                  MIRIntrinsicID argIntrinsicID) const {
  auto feTy = std::make_unique<FEIRTypeNative>(*mirType);
  std::vector<std::unique_ptr<FEIRExpr>> argOpnds;
  for (auto arg : args) {
    argOpnds.push_back(arg->Emit2FEExpr(stmts));
  }
  auto feExpr = std::make_unique<FEIRExprIntrinsicopForC>(std::move(feTy), argIntrinsicID, argOpnds);
  std::string tmpName = FEUtils::GetSequentialName("intrinsicop_var_");
  UniqueFEIRVar tmpVar = FEIRBuilder::CreateVarNameForC(tmpName, *mirType);
  UniqueFEIRStmt dAssign = std::make_unique<FEIRStmtDAssign>(tmpVar->Clone(), std::move(feExpr), 0);
  dAssign->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
  stmts.emplace_back(std::move(dAssign));
  auto dread = FEIRBuilder::CreateExprDRead(tmpVar->Clone());
  return dread;
}

UniqueFEIRExpr ASTCallExpr::CreateBinaryExpr(std::list<UniqueFEIRStmt> &stmts, Opcode op) const {
  auto feTy = std::make_unique<FEIRTypeNative>(*mirType);
  auto arg1 = args[0]->Emit2FEExpr(stmts);
  auto arg2 = args[1]->Emit2FEExpr(stmts);
  return std::make_unique<FEIRExprBinary>(std::move(feTy), op, std::move(arg1), std::move(arg2));
}

UniqueFEIRExpr ASTCallExpr::ProcessBuiltinFunc(std::list<UniqueFEIRStmt> &stmts, bool &isFinish) const {
  // process a kind of builtinFunc
  std::string prefix = "__builtin_mpl_vector_load";
  if (funcName.compare(0, prefix.size(), prefix) == 0) {
    auto argExpr = args[0]->Emit2FEExpr(stmts);
    UniqueFEIRType type = FEIRTypeHelper::CreateTypeNative(*mirType);
    UniqueFEIRType ptrType = FEIRTypeHelper::CreateTypeNative(*args[0]->GetType());
    isFinish = true;
    return FEIRBuilder::CreateExprIRead(std::move(type), std::move(ptrType), std::move(argExpr));
  }
  prefix = "__builtin_mpl_vector_store";
  if (funcName.compare(0, prefix.size(), prefix) == 0) {
    auto arg1Expr = args[0]->Emit2FEExpr(stmts);
    auto arg2Expr = args[1]->Emit2FEExpr(stmts);
    UniqueFEIRType type = FEIRTypeHelper::CreateTypeNative(*args[0]->GetType());
    auto stmt = FEIRBuilder::CreateStmtIAssign(std::move(type), std::move(arg1Expr), std::move(arg2Expr));
    stmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
    stmts.emplace_back(std::move(stmt));
    isFinish = true;
    return nullptr;
  }
  // process a single builtinFunc
  auto ptrFunc = builtingFuncPtrMap.find(funcName);
  if (ptrFunc != builtingFuncPtrMap.end()) {
    isFinish = true;
    return EmitBuiltinFunc(stmts);
  }
  isFinish = false;
  prefix = "__builtin";
  if (funcName.compare(0, prefix.size(), prefix) == 0) {
    WARN(kLncWarn, "BuiltinFunc (%s) has not been implemented, line: %d", funcName.c_str(), GetSrcFileLineNum());
  }
  return nullptr;
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinFunc(std::list<UniqueFEIRStmt> &stmts) const {
  return (this->*(builtingFuncPtrMap[funcName]))(stmts);
}

#define DEF_MIR_INTRINSIC(STR, NAME, INTRN_CLASS, RETURN_TYPE, ...)                                \
UniqueFEIRExpr ASTCallExpr::EmitBuiltin##STR(std::list<UniqueFEIRStmt> &stmts) const {             \
  auto feType = FEIRTypeHelper::CreateTypeNative(*mirType);                                        \
  std::vector<std::unique_ptr<FEIRExpr>> argOpnds;                                                 \
  for (auto arg : args) {                                                                          \
    argOpnds.push_back(arg->Emit2FEExpr(stmts));                                                   \
  }                                                                                                \
  return std::make_unique<FEIRExprIntrinsicopForC>(std::move(feType), INTRN_##STR, argOpnds);      \
}
#include "intrinsic_vector.def"
#undef DEF_MIR_INTRINSIC

UniqueFEIRExpr ASTCallExpr::EmitBuiltinVaStart(std::list<UniqueFEIRStmt> &stmts) const {
  // args
  auto exprArgList = std::make_unique<std::list<UniqueFEIRExpr>>();
  for (auto arg : args) {
    UniqueFEIRExpr expr = arg->Emit2FEExpr(stmts);
    exprArgList->emplace_back(std::move(expr));
  }
  // addrof va_list instead of dread va_list
  exprArgList->front()->SetAddrof(true);
  std::unique_ptr<FEIRStmtIntrinsicCallAssign> stmt = std::make_unique<FEIRStmtIntrinsicCallAssign>(
      INTRN_C_va_start, nullptr /* type */, nullptr /* retVar */, std::move(exprArgList));
  stmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
  stmts.emplace_back(std::move(stmt));
  return nullptr;
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinVaEnd(std::list<UniqueFEIRStmt> &stmts) const {
  // args
  ASSERT(args.size() == 1, "va_end expects 1 arguments");
  std::list<UniqueFEIRExpr> exprArgList;
  for (auto arg : args) {
    UniqueFEIRExpr expr = arg->Emit2FEExpr(stmts);
    // addrof va_list instead of dread va_list
    expr->SetAddrof(true);
    exprArgList.emplace_back(std::move(expr));
  }
  auto stmt = std::make_unique<FEIRStmtNary>(OP_eval, std::move(exprArgList));
  stmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
  stmts.emplace_back(std::move(stmt));
  return nullptr;
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinVaCopy(std::list<UniqueFEIRStmt> &stmts) const {
  // args
  auto exprArgList = std::make_unique<std::list<UniqueFEIRExpr>>();
  UniqueFEIRType vaListType;
  for (auto arg : args) {
    UniqueFEIRExpr expr = arg->Emit2FEExpr(stmts);
    // addrof va_list instead of dread va_list
    expr->SetAddrof(true);
    vaListType = expr->GetType()->Clone();
    exprArgList->emplace_back(std::move(expr));
  }
  // Add the size of the va_list structure as the size to memcpy.
  UniqueFEIRExpr sizeExpr = FEIRBuilder::CreateExprConstI32(vaListType->GenerateMIRTypeAuto()->GetSize());
  exprArgList->emplace_back(std::move(sizeExpr));
  std::unique_ptr<FEIRStmtIntrinsicCallAssign> stmt = std::make_unique<FEIRStmtIntrinsicCallAssign>(
      INTRN_C_memcpy, nullptr /* type */, nullptr /* retVar */, std::move(exprArgList));
  stmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
  stmts.emplace_back(std::move(stmt));
  return nullptr;
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinPrefetch(std::list<UniqueFEIRStmt> &stmts) const {
  // __builtin_prefetch is not supported, only parsing args including stmts
  for (int32 i = 0; i <= args.size() - 1; ++i) {
    (void)args[i]->Emit2FEExpr(stmts);
  }
  return nullptr;
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinCtz(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_ctz32);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinCtzl(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_ctz64);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinClz(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_clz32);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinClzl(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_clz64);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinPopcount(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_popcount32);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinPopcountl(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_popcount64);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinPopcountll(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_popcount64);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinParity(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_parity32);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinParityl(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_parity64);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinParityll(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_parity64);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinClrsb(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_clrsb32);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinClrsbl(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_clrsb64);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinClrsbll(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_clrsb64);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinFfs(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_ffs);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinFfsl(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_ffs);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinFfsll(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_ffs);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinIsAligned(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_isaligned);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinAlignUp(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_alignup);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinAlignDown(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_aligndown);
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
  stmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
  stmts.emplace_back(std::move(stmt));
  return args[0]->Emit2FEExpr(stmts);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinUnreachable(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRExpr feExpr = nullptr;
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtReturn>(std::move(feExpr));
  stmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
  stmts.emplace_back(std::move(stmt));
  return nullptr;
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinAbs(std::list<UniqueFEIRStmt> &stmts) const {
  auto arg = args[0]->Emit2FEExpr(stmts);
  auto abs = std::make_unique<FEIRExprUnary>(OP_abs, mirType, std::move(arg));
  auto feType = std::make_unique<FEIRTypeNative>(*mirType);
  abs->SetType(std::move(feType));
  return abs;
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinACos(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_acos);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinACosf(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_acosf);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinASin(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_asin);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinASinf(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_asinf);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinATan(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_atan);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinATanf(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_atanf);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinCos(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_cos);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinCosf(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_cosf);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinCosh(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_cosh);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinCoshf(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_coshf);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSin(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_sin);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSinf(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_sinf);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSinh(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_sinh);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSinhf(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_sinhf);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinExp(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_exp);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinExpf(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_expf);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinFmax(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateBinaryExpr(stmts, OP_max);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinFmin(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateBinaryExpr(stmts, OP_min);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinLog(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_log);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinLogf(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_logf);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinLog10(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_log10);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinLog10f(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_log10f);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinIsunordered(std::list<UniqueFEIRStmt> &stmts) const {
  auto feTy = std::make_unique<FEIRTypeNative>(*mirType);
  auto arg1 = args[0]->Emit2FEExpr(stmts);
  auto arg2 = args[1]->Emit2FEExpr(stmts);
  auto nan1 = std::make_unique<FEIRExprBinary>(feTy->Clone(), OP_ne, arg1->Clone(), arg1->Clone());
  auto nan2 = std::make_unique<FEIRExprBinary>(feTy->Clone(), OP_ne, arg2->Clone(), arg2->Clone());
  auto res = std::make_unique<FEIRExprBinary>(feTy->Clone(), OP_lior, std::move(nan1), std::move(nan2));
  return res;
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinIsless(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateBinaryExpr(stmts, OP_lt);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinIslessequal(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateBinaryExpr(stmts, OP_le);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinIsgreater (std::list<UniqueFEIRStmt> &stmts) const {
  return CreateBinaryExpr(stmts, OP_gt);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinIsgreaterequal(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateBinaryExpr(stmts, OP_ge);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinIslessgreater (std::list<UniqueFEIRStmt> &stmts) const {
  auto feTy = std::make_unique<FEIRTypeNative>(*mirType);
  auto arg1 = args[0]->Emit2FEExpr(stmts);
  auto arg2 = args[1]->Emit2FEExpr(stmts);
  auto cond1 = std::make_unique<FEIRExprBinary>(feTy->Clone(), OP_lt, arg1->Clone(), arg2->Clone());
  auto cond2 = std::make_unique<FEIRExprBinary>(feTy->Clone(), OP_gt, arg1->Clone(), arg2->Clone());
  auto res = std::make_unique<FEIRExprBinary>(feTy->Clone(), OP_lior, std::move(cond1), std::move(cond2));
  return res;
}

std::map<std::string, ASTParser::FuncPtrBuiltinFunc> ASTParser::InitBuiltinFuncPtrMap() {
  std::map<std::string, FuncPtrBuiltinFunc> ans;
#define BUILTIN_FUNC_PARSE(funcName, FuncPtrBuiltinFunc) \
  ans[funcName] = FuncPtrBuiltinFunc;
#include "builtin_func_parse.def"
#undef BUILTIN_FUNC_PARSE
  return ans;
}

ASTExpr *ASTParser::ParseBuiltinFunc(MapleAllocator &allocator, const clang::CallExpr &expr,
                                     std::stringstream &ss) const {
  return (this->*(builtingFuncPtrMap[ss.str()]))(allocator, expr, ss);
}

ASTExpr *ASTParser::ProcessBuiltinFuncByName(MapleAllocator &allocator, const clang::CallExpr &expr,
                                             std::stringstream &ss, std::string name) const {
  (void)allocator;
  (void)expr;
  ss.clear();
  ss.str(std::string());
  ss << name;
  return nullptr;
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
   return ProcessBuiltinFuncByName(allocator, expr, ss, "__signbit");
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

ASTExpr *ASTParser::ParseBuiltinHugeVal(MapleAllocator &allocator, const clang::CallExpr &expr,
                                        std::stringstream &ss) const {
  ASTFloatingLiteral *astFloatingLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTFloatingLiteral>(allocator);
  astFloatingLiteral->SetKind(F64);
  astFloatingLiteral->SetVal(std::numeric_limits<double>::infinity());
  return astFloatingLiteral;
}

ASTExpr *ASTParser::ParseBuiltinHugeValf(MapleAllocator &allocator, const clang::CallExpr &expr,
                                         std::stringstream &ss) const {
  ASTFloatingLiteral *astFloatingLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTFloatingLiteral>(allocator);
  astFloatingLiteral->SetKind(F32);
  astFloatingLiteral->SetVal(std::numeric_limits<float>::infinity());
  return astFloatingLiteral;
}

ASTExpr *ASTParser::ParseBuiltinInf(MapleAllocator &allocator, const clang::CallExpr &expr,
                                    std::stringstream &ss) const {
  ASTFloatingLiteral *astFloatingLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTFloatingLiteral>(allocator);
  astFloatingLiteral->SetKind(F64);
  astFloatingLiteral->SetVal(std::numeric_limits<float>::infinity());
  return astFloatingLiteral;
}

ASTExpr *ASTParser::ParseBuiltinInff(MapleAllocator &allocator, const clang::CallExpr &expr,
                                    std::stringstream &ss) const {
  ASTFloatingLiteral *astFloatingLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTFloatingLiteral>(allocator);
  astFloatingLiteral->SetKind(F32);
  astFloatingLiteral->SetVal(std::numeric_limits<float>::infinity());
  return astFloatingLiteral;
}

ASTExpr *ASTParser::ParseBuiltinNan(MapleAllocator &allocator, const clang::CallExpr &expr,
                                    std::stringstream &ss) const {
  ASTFloatingLiteral *astFloatingLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTFloatingLiteral>(allocator);
  astFloatingLiteral->SetKind(F64);
  astFloatingLiteral->SetVal(nan(""));
  return astFloatingLiteral;
}

ASTExpr *ASTParser::ParseBuiltinNanf(MapleAllocator &allocator, const clang::CallExpr &expr,
                                     std::stringstream &ss) const {
  ASTFloatingLiteral *astFloatingLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTFloatingLiteral>(allocator);
  astFloatingLiteral->SetKind(F32);
  astFloatingLiteral->SetVal(nanf(""));
  return astFloatingLiteral;
}

ASTExpr *ASTParser::ParseBuiltinSignBitf(MapleAllocator &allocator, const clang::CallExpr &expr,
                                         std::stringstream &ss) const {
   return ProcessBuiltinFuncByName(allocator, expr, ss, "__signbitf");
}

ASTExpr *ASTParser::ParseBuiltinSignBitl(MapleAllocator &allocator, const clang::CallExpr &expr,
                                         std::stringstream &ss) const {
  return ProcessBuiltinFuncByName(allocator, expr, ss, "__signbitl");
}

ASTExpr *ASTParser::ParseBuiltinTrap(MapleAllocator &allocator, const clang::CallExpr &expr,
                                     std::stringstream &ss) const {
  return ProcessBuiltinFuncByName(allocator, expr, ss, "abort");
}

ASTExpr *ASTParser::ParseBuiltinCopysignf(MapleAllocator &allocator, const clang::CallExpr &expr,
                                          std::stringstream &ss) const {
  return ProcessBuiltinFuncByName(allocator, expr, ss, "copysignf");
}

ASTExpr *ASTParser::ParseBuiltinCopysign(MapleAllocator &allocator, const clang::CallExpr &expr,
                                          std::stringstream &ss) const {
  return ProcessBuiltinFuncByName(allocator, expr, ss, "copysign");
}

ASTExpr *ASTParser::ParseBuiltinCopysignl(MapleAllocator &allocator, const clang::CallExpr &expr,
                                          std::stringstream &ss) const {
  return ProcessBuiltinFuncByName(allocator, expr, ss, "copysignl");
}

ASTExpr *ASTParser::ParseBuiltinObjectsize(MapleAllocator &allocator, const clang::CallExpr &expr,
                                           std::stringstream &ss) const {
  uint32 objSizeType = expr.getArg(1)->EvaluateKnownConstInt(*astFile->GetContext()).getZExtValue();
  // GCC size_t __builtin_object_size(void *ptr, int type) type range is 0 ~ 3
  ASSERT(objSizeType <= 3, "unexpected type");
  uint64 objSize;
  bool canEval = expr.getArg(0)->tryEvaluateObjectSize(objSize, *astFile->GetNonConstAstContext(), objSizeType);
  if (!canEval) {
    // type 0 and 1 need return -1, type 2 and 3 need return 0
    objSize = objSizeType & 2 ? 0 : -1;
  }
  ASTIntegerLiteral *astIntegerLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
  astIntegerLiteral->SetVal(static_cast<uint64>(objSize));
  astIntegerLiteral->SetType(astFile->CvtType(expr.getType())->GetPrimType());
  return astIntegerLiteral;
}
} // namespace maple