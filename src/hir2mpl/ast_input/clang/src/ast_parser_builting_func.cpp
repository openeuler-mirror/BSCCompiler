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
#include "ast_expr.h"
#include "ast_decl.h"
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
#include "fe_macros.h"

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
                                                  MIRIntrinsicID argIntrinsicID, bool genTempVar) const {
  auto feTy = std::make_unique<FEIRTypeNative>(*mirType);
  std::vector<std::unique_ptr<FEIRExpr>> argOpnds;
  for (auto arg : args) {
    argOpnds.push_back(arg->Emit2FEExpr(stmts));
  }
  auto feExpr = std::make_unique<FEIRExprIntrinsicopForC>(std::move(feTy), argIntrinsicID, argOpnds);
  if (mirType->GetPrimType() == PTY_void) {
    std::list<UniqueFEIRExpr> feExprs;
    feExprs.emplace_back(std::move(feExpr));
    UniqueFEIRStmt evalStmt = std::make_unique<FEIRStmtNary>(OP_eval, std::move(feExprs));
    stmts.emplace_back(std::move(evalStmt));
    return nullptr;
  } else {
    if (!genTempVar) {
      return feExpr;
    }
    std::string tmpName = FEUtils::GetSequentialName("intrinsicop_var_");
    UniqueFEIRVar tmpVar = FEIRBuilder::CreateVarNameForC(tmpName, *mirType);
    UniqueFEIRStmt dAssign = std::make_unique<FEIRStmtDAssign>(tmpVar->Clone(), std::move(feExpr), 0);
    stmts.emplace_back(std::move(dAssign));
    auto dread = FEIRBuilder::CreateExprDRead(tmpVar->Clone());
    return dread;
  }
}

UniqueFEIRExpr ASTCallExpr::CreateIntrinsicCallAssignedForC(std::list<UniqueFEIRStmt> &stmts,
                                                            MIRIntrinsicID argIntrinsicID) const {
  std::unique_ptr<std::list<UniqueFEIRExpr>> argExprList = std::make_unique<std::list<UniqueFEIRExpr>>();
  for (auto arg : args) {
    (void)argExprList->emplace_back(arg->Emit2FEExpr(stmts));
  }
  if (!IsNeedRetExpr()) {
    auto stmt = std::make_unique<FEIRStmtIntrinsicCallAssign>(argIntrinsicID, nullptr, nullptr,
                                                              std::move(argExprList));
    stmts.emplace_back(std::move(stmt));
    return nullptr;
  }
  UniqueFEIRVar retVar = FEIRBuilder::CreateVarNameForC(GetRetVarName(), *mirType, false);
  auto stmt = std::make_unique<FEIRStmtIntrinsicCallAssign>(argIntrinsicID, nullptr, retVar->Clone(),
                                                            std::move(argExprList));
  stmts.emplace_back(std::move(stmt));
  UniqueFEIRExpr dread = FEIRBuilder::CreateExprDRead(std::move(retVar));
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
  if (GetFuncName().compare(0, prefix.size(), prefix) == 0) {
    return EmitBuiltinVectorLoad(stmts, isFinish);
  }
  prefix = "__builtin_mpl_vector_store";
  if (GetFuncName().compare(0, prefix.size(), prefix) == 0) {
    return EmitBuiltinVectorStore(stmts, isFinish);
  }
  prefix = "__builtin_mpl_vector_zip";
  if (GetFuncName().compare(0, prefix.size(), prefix) == 0) {
    return EmitBuiltinVectorZip(stmts, isFinish);
  }
  prefix = "__builtin_mpl_vector_shli";
  if (GetFuncName().compare(0, prefix.size(), prefix) == 0) {
    return EmitBuiltinVectorShli(stmts, isFinish);
  }
  prefix = "__builtin_mpl_vector_shri";
  if (GetFuncName().compare(0, prefix.size(), prefix) == 0) {
    return EmitBuiltinVectorShri(stmts, isFinish);
  }
  prefix = "__builtin_mpl_vector_shru";
  if (GetFuncName().compare(0, prefix.size(), prefix) == 0) {
    return EmitBuiltinVectorShru(stmts, isFinish);
  }
  // process a single builtinFunc
  auto ptrFunc = builtingFuncPtrMap.find(GetFuncName());
  if (ptrFunc != builtingFuncPtrMap.end()) {
    isFinish = true;
    return EmitBuiltinFunc(stmts);
  }
  isFinish = false;
  if (FEOptions::GetInstance().GetDumpLevel() >= FEOptions::kDumpLevelInfo) {
    prefix = "__builtin";
    if (GetFuncName().compare(0, prefix.size(), prefix) == 0) {
      FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "%s:%d BuiltinFunc (%s) has not been implemented",
                    FEManager::GetModule().GetFileNameFromFileNum(GetSrcFileIdx()).c_str(), GetSrcFileLineNum(),
                    GetFuncName().c_str());
    }
  }
  return nullptr;
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinFunc(std::list<UniqueFEIRStmt> &stmts) const {
  return (this->*(builtingFuncPtrMap[GetFuncName()]))(stmts);
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

UniqueFEIRExpr ASTCallExpr::EmitBuiltinVectorLoad(std::list<UniqueFEIRStmt> &stmts, bool &isFinish) const {
  auto argExpr = args[0]->Emit2FEExpr(stmts);
  UniqueFEIRType type = FEIRTypeHelper::CreateTypeNative(*mirType);
  UniqueFEIRType ptrType = FEIRTypeHelper::CreateTypeNative(
      *GlobalTables::GetTypeTable().GetOrCreatePointerType(*mirType));
  isFinish = true;
  return FEIRBuilder::CreateExprIRead(std::move(type), std::move(ptrType), std::move(argExpr));
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinVectorStore(std::list<UniqueFEIRStmt> &stmts, bool &isFinish) const {
  auto arg1Expr = args[0]->Emit2FEExpr(stmts);
  auto arg2Expr = args[1]->Emit2FEExpr(stmts);
  UniqueFEIRType type = FEIRTypeHelper::CreateTypeNative(
      *GlobalTables::GetTypeTable().GetOrCreatePointerType(*args[1]->GetType()));
  auto stmt = FEIRBuilder::CreateStmtIAssign(std::move(type), std::move(arg1Expr), std::move(arg2Expr));
  (void)stmts.emplace_back(std::move(stmt));
  isFinish = true;
  return nullptr;
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinVectorShli(std::list<UniqueFEIRStmt> &stmts, bool &isFinish) const {
  isFinish = true;
  UniqueFEIRType type = FEIRTypeHelper::CreateTypeNative(*args[0]->GetType());
  auto arg1Expr = args[0]->Emit2FEExpr(stmts);
  auto arg2Expr = args[1]->Emit2FEExpr(stmts);
  return FEIRBuilder::CreateExprBinary(std::move(type), OP_shl, std::move(arg1Expr), std::move(arg2Expr));
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinVectorShri(std::list<UniqueFEIRStmt> &stmts, bool &isFinish) const {
  isFinish = true;
  UniqueFEIRType type = FEIRTypeHelper::CreateTypeNative(*args[0]->GetType());
  auto arg1Expr = args[0]->Emit2FEExpr(stmts);
  auto arg2Expr = args[1]->Emit2FEExpr(stmts);
  return FEIRBuilder::CreateExprBinary(std::move(type), OP_ashr, std::move(arg1Expr), std::move(arg2Expr));
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinVectorShru(std::list<UniqueFEIRStmt> &stmts, bool &isFinish) const {
  isFinish = true;
  UniqueFEIRType type = FEIRTypeHelper::CreateTypeNative(*args[0]->GetType());
  auto arg1Expr = args[0]->Emit2FEExpr(stmts);
  auto arg2Expr = args[1]->Emit2FEExpr(stmts);
  return FEIRBuilder::CreateExprBinary(std::move(type), OP_lshr, std::move(arg1Expr), std::move(arg2Expr));
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinVectorZip(std::list<UniqueFEIRStmt> &stmts, bool &isFinish) const {
  std::unique_ptr<std::list<UniqueFEIRExpr>> argExprList = std::make_unique<std::list<UniqueFEIRExpr>>();
  for (auto arg : args) {
    UniqueFEIRExpr expr = arg->Emit2FEExpr(stmts);
    argExprList->emplace_back(std::move(expr));
  }
  CHECK_NULL_FATAL(mirType);
  std::string retName = FEUtils::GetSequentialName("vector_zip_retvar_");
  UniqueFEIRVar retVar = FEIRBuilder::CreateVarNameForC(retName, *mirType);

#define VECTOR_INTRINSICCALL_TYPE(OP_NAME, VECTY)                                                \
  if (FEUtils::EndsWith(GetFuncName(), #VECTY)) {                                                \
    stmt = std::make_unique<FEIRStmtIntrinsicCallAssign>(                                        \
        INTRN_vector_##OP_NAME##_##VECTY, nullptr, retVar->Clone(), std::move(argExprList));     \
  }
  UniqueFEIRStmt stmt;

  VECTOR_INTRINSICCALL_TYPE(zip, v2i32)
  else VECTOR_INTRINSICCALL_TYPE(zip, v4i16)
  else VECTOR_INTRINSICCALL_TYPE(zip, v8i8)
  else VECTOR_INTRINSICCALL_TYPE(zip, v2u32)
  else VECTOR_INTRINSICCALL_TYPE(zip, v4u16)
  else VECTOR_INTRINSICCALL_TYPE(zip, v8u8)
  else VECTOR_INTRINSICCALL_TYPE(zip, v2f32)

  stmts.emplace_back(std::move(stmt));
  isFinish = true;
  return FEIRBuilder::CreateExprDRead(std::move(retVar));
}

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
  size_t elemSizes = vaListType->GenerateMIRTypeAuto()->GetSize();
  CHECK_FATAL(elemSizes <= INT_MAX, "Too large elem size");
  UniqueFEIRExpr sizeExpr = FEIRBuilder::CreateExprConstI32(static_cast<int32>(elemSizes));
  exprArgList->emplace_back(std::move(sizeExpr));
  std::unique_ptr<FEIRStmtIntrinsicCallAssign> stmt = std::make_unique<FEIRStmtIntrinsicCallAssign>(
      INTRN_C_memcpy, nullptr /* type */, nullptr /* retVar */, std::move(exprArgList));
  stmts.emplace_back(std::move(stmt));
  return nullptr;
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinPrefetch(std::list<UniqueFEIRStmt> &stmts) const {
  // __builtin_prefetch is not supported, only parsing args including stmts
  for (size_t i = 0; i <= args.size() - 1; ++i) {
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

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncAddAndFetch8(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_add_and_fetch_8);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncAddAndFetch4(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_add_and_fetch_4);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncAddAndFetch2(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_add_and_fetch_2);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncAddAndFetch1(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_add_and_fetch_1);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncSubAndFetch8(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_sub_and_fetch_8);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncSubAndFetch4(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_sub_and_fetch_4);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncSubAndFetch2(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_sub_and_fetch_2);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncSubAndFetch1(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_sub_and_fetch_1);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncFetchAndSub8(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_fetch_and_sub_8);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncFetchAndSub4(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_fetch_and_sub_4);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncFetchAndSub2(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_fetch_and_sub_2);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncFetchAndSub1(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_fetch_and_sub_1);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncFetchAndAdd8(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_fetch_and_add_8);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncFetchAndAdd4(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_fetch_and_add_4);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncFetchAndAdd2(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_fetch_and_add_2);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncFetchAndAdd1(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_fetch_and_add_1);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncValCompareAndSwap8(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_val_compare_and_swap_8);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncValCompareAndSwap4(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_val_compare_and_swap_4);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncValCompareAndSwap2(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_val_compare_and_swap_2);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncValCompareAndSwap1(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_val_compare_and_swap_1);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncLockRelease8(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_lock_release_8);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncLockRelease4(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_lock_release_4);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncLockRelease2(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_lock_release_2);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncLockRelease1(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_lock_release_1);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncBoolCompareAndSwap8(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_bool_compare_and_swap_8);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncBoolCompareAndSwap4(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_bool_compare_and_swap_4);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncBoolCompareAndSwap2(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_bool_compare_and_swap_2);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncBoolCompareAndSwap1(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_bool_compare_and_swap_1);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncLockTestAndSet8(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_lock_test_and_set_8);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncLockTestAndSet4(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_lock_test_and_set_4);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncLockTestAndSet2(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_lock_test_and_set_2);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncLockTestAndSet1(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_lock_test_and_set_1);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncFetchAndAnd1(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_fetch_and_and_1);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncFetchAndAnd2(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_fetch_and_and_2);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncFetchAndAnd4(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_fetch_and_and_4);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncFetchAndAnd8(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_fetch_and_and_8);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncFetchAndOr1(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_fetch_and_or_1);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncFetchAndOr2(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_fetch_and_or_2);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncFetchAndOr4(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_fetch_and_or_4);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncFetchAndOr8(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_fetch_and_or_8);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncFetchAndXor1(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_fetch_and_xor_1);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncFetchAndXor2(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_fetch_and_xor_2);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncFetchAndXor4(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_fetch_and_xor_4);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncFetchAndXor8(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_fetch_and_xor_8);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncFetchAndNand1(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_fetch_and_nand_1);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncFetchAndNand2(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_fetch_and_nand_2);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncFetchAndNand4(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_fetch_and_nand_4);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncFetchAndNand8(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_fetch_and_nand_8);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncAndAndFetch1(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_and_and_fetch_1);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncAndAndFetch2(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_and_and_fetch_2);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncAndAndFetch4(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_and_and_fetch_4);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncAndAndFetch8(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_and_and_fetch_8);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncOrAndFetch1(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_or_and_fetch_1);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncOrAndFetch2(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_or_and_fetch_2);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncOrAndFetch4(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_or_and_fetch_4);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncOrAndFetch8(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_or_and_fetch_8);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncXorAndFetch1(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_xor_and_fetch_1);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncXorAndFetch2(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_xor_and_fetch_2);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncXorAndFetch4(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_xor_and_fetch_4);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncXorAndFetch8(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_xor_and_fetch_8);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncNandAndFetch1(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_nand_and_fetch_1);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncNandAndFetch2(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_nand_and_fetch_2);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncNandAndFetch4(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_nand_and_fetch_4);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncNandAndFetch8(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_nand_and_fetch_8);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinSyncSynchronize(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___sync_synchronize);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinAtomicExchangeN(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___atomic_exchange_n);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinObjectSize(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicCallAssignedForC(stmts, INTRN_C___builtin_object_size);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinReturnAddress(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C__builtin_return_address);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinExtractReturnAddr(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C__builtin_extract_return_addr);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinAlloca(std::list<UniqueFEIRStmt> &stmts) const {
  auto arg = args[0]->Emit2FEExpr(stmts);
  CHECK_NULL_FATAL(mirType);
  auto alloca = std::make_unique<FEIRExprUnary>(FEIRTypeHelper::CreateTypeNative(*mirType), OP_alloca, std::move(arg));
  return alloca;
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinExpect(std::list<UniqueFEIRStmt> &stmts) const {
  ASSERT(args.size() == 2, "__builtin_expect requires two arguments");
  std::list<UniqueFEIRStmt> subStmts;
  UniqueFEIRExpr feExpr = CreateIntrinsicopForC(subStmts, INTRN_C___builtin_expect, false);
  bool isOptimized = false;
  for (auto &stmt : std::as_const(subStmts)) {
    // If there are mutiple conditions combined with logical AND '&&' or logical OR '||' in __builtin_expect, generate
    // a __builtin_expect intrinsicop for each one condition in mpl
    if (stmt->GetKind() == FEIRNodeKind::kStmtCondGoto) {
      isOptimized = true;
      auto *condGotoStmt = static_cast<FEIRStmtCondGotoForC*>(stmt.get());
      // skip if condition label name not starts with <kCondGoToStmtLabelNamePrefix>
      if (condGotoStmt->GetLabelName().rfind(FEUtils::kCondGoToStmtLabelNamePrefix, 0) != 0) {
        continue;
      }
      const auto &conditionExpr = condGotoStmt->GetConditionExpr();
      // skip if __builtin_expect intrinsicop has been generated for condition expr
      if (conditionExpr->GetKind() == kExprBinary) {
        auto &opnd0 = static_cast<FEIRExprBinary*>(conditionExpr.get())->GetOpnd0();
        if (opnd0->GetKind() == FEIRNodeKind::kExprIntrinsicop &&
            static_cast<FEIRExprIntrinsicopForC*>(opnd0.get())->GetIntrinsicID() == INTRN_C___builtin_expect) {
          continue;
        }
      }
      std::vector<std::unique_ptr<FEIRExpr>> argOpnds;
      auto &builtInExpectArgs = static_cast<FEIRExprIntrinsicopForC*>(feExpr.get())->GetOpnds();
      auto cvtFeExpr = FEIRBuilder::CreateExprCvtPrim(conditionExpr->Clone(), builtInExpectArgs.front()->GetPrimType());
      argOpnds.push_back(std::move(cvtFeExpr));
      argOpnds.push_back(builtInExpectArgs.back()->Clone());
      auto returnType = std::make_unique<FEIRTypeNative>(*mirType);
      auto builtinExpectExpr = std::make_unique<FEIRExprIntrinsicopForC>(std::move(returnType),
                                                                         INTRN_C___builtin_expect, argOpnds);
      auto newConditionExpr = FEIRBuilder::CreateExprZeroCompare(OP_ne, std::move(builtinExpectExpr));
      condGotoStmt->SetCondtionExpr(newConditionExpr);
    }
  }
  stmts.splice(stmts.end(), subStmts);
  return isOptimized ? static_cast<FEIRExprIntrinsicopForC*>(feExpr.get())->GetOpnds().front()->Clone()
                     : feExpr->Clone();
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinAbs(std::list<UniqueFEIRStmt> &stmts) const {
  auto arg = args[0]->Emit2FEExpr(stmts);
  CHECK_NULL_FATAL(mirType);
  auto abs = std::make_unique<FEIRExprUnary>(FEIRTypeHelper::CreateTypeNative(*mirType), OP_abs, std::move(arg));
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

UniqueFEIRExpr ASTCallExpr::EmitBuiltinBswap64(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_bswap64);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinBswap32(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_bswap32);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinBswap16(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateIntrinsicopForC(stmts, INTRN_C_bswap16);
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

UniqueFEIRExpr ASTCallExpr::EmitBuiltinIsgreater(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateBinaryExpr(stmts, OP_gt);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinIsgreaterequal(std::list<UniqueFEIRStmt> &stmts) const {
  return CreateBinaryExpr(stmts, OP_ge);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinIslessgreater(std::list<UniqueFEIRStmt> &stmts) const {
  auto feTy = std::make_unique<FEIRTypeNative>(*mirType);
  auto arg1 = args[0]->Emit2FEExpr(stmts);
  auto arg2 = args[1]->Emit2FEExpr(stmts);
  auto cond1 = std::make_unique<FEIRExprBinary>(feTy->Clone(), OP_lt, arg1->Clone(), arg2->Clone());
  auto cond2 = std::make_unique<FEIRExprBinary>(feTy->Clone(), OP_gt, arg1->Clone(), arg2->Clone());
  auto res = std::make_unique<FEIRExprBinary>(feTy->Clone(), OP_lior, std::move(cond1), std::move(cond2));
  return res;
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinWarnMemsetZeroLen(std::list<UniqueFEIRStmt> &stmts) const {
  (void)stmts;
  return nullptr;
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinRotateLeft8(std::list<UniqueFEIRStmt> &stmts) const {
  return EmitBuiltinRotate(stmts, PTY_u8, true);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinRotateLeft16(std::list<UniqueFEIRStmt> &stmts) const {
  return EmitBuiltinRotate(stmts, PTY_u16, true);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinRotateLeft32(std::list<UniqueFEIRStmt> &stmts) const {
  return EmitBuiltinRotate(stmts, PTY_u32, true);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinRotateLeft64(std::list<UniqueFEIRStmt> &stmts) const {
  return EmitBuiltinRotate(stmts, PTY_u64, true);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinRotateRight8(std::list<UniqueFEIRStmt> &stmts) const {
  return EmitBuiltinRotate(stmts, PTY_u8, false);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinRotateRight16(std::list<UniqueFEIRStmt> &stmts) const {
  return EmitBuiltinRotate(stmts, PTY_u16, false);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinRotateRight32(std::list<UniqueFEIRStmt> &stmts) const {
  return EmitBuiltinRotate(stmts, PTY_u32, false);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinRotateRight64(std::list<UniqueFEIRStmt> &stmts) const {
  return EmitBuiltinRotate(stmts, PTY_u64, false);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinRotate(std::list<UniqueFEIRStmt> &stmts, PrimType rotType, bool isLeft) const {
  const int mask = FEUtils::GetWidth(rotType) - 1;
  UniqueFEIRExpr maskExpr = FEIRBuilder::CreateExprConstAnyScalar(rotType, mask);
  UniqueFEIRExpr valExpr = args[0]->Emit2FEExpr(stmts);
  UniqueFEIRExpr bitExpr = args[1]->Emit2FEExpr(stmts);
  bitExpr = FEIRBuilder::CreateExprBinary(OP_band, bitExpr->Clone(), maskExpr->Clone());
  // RotateLeft: (val >> bit) | (val << ((-bit) & mask))
  // RotateRight: (val << bit) | (val >> ((-bit) & mask))
  return FEIRBuilder::CreateExprBinary(OP_bior,
      FEIRBuilder::CreateExprBinary((isLeft ? OP_shl : OP_lshr), valExpr->Clone(), bitExpr->Clone()),
      FEIRBuilder::CreateExprBinary((isLeft ? OP_lshr : OP_shl),
          valExpr->Clone(),
          FEIRBuilder::CreateExprBinary(OP_band,
              FEIRBuilder::CreateExprMathUnary(OP_neg, bitExpr->Clone()),
              maskExpr->Clone())));
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
                                             std::stringstream &ss, const std::string &name) const {
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
  astIntegerLiteral->SetVal(apVal.getExtValue());
  astIntegerLiteral->SetType(GlobalTables::GetTypeTable().GetInt32());
  return astIntegerLiteral;
}

ASTExpr *ASTParser::ParseBuiltinConstantP(MapleAllocator &allocator, const clang::CallExpr &expr,
                                          std::stringstream &ss) const {
  (void)ss;
  int64 constP = expr.getArg(0)->isConstantInitializer(*astFile->GetNonConstAstContext(), false) ? 1 : 0;
  // Pointers are not considered constant
  if (expr.getArg(0)->getType()->isPointerType() &&
      !llvm::isa<clang::StringLiteral>(expr.getArg(0)->IgnoreParenCasts())) {
    constP = 0;
  }
  ASTIntegerLiteral *astIntegerLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
  astIntegerLiteral->SetVal(constP);
  astIntegerLiteral->SetType(astFile->CvtType(expr.getType()));
  return astIntegerLiteral;
}

ASTExpr *ASTParser::ParseBuiltinSignbit(MapleAllocator &allocator, const clang::CallExpr &expr,
                                        std::stringstream &ss) const {
  return ProcessBuiltinFuncByName(allocator, expr, ss, "__signbit");
}

ASTExpr *ASTParser::ParseBuiltinIsinfsign(MapleAllocator &allocator, const clang::CallExpr &expr,
                                          std::stringstream &ss) const {
  (void)allocator;
  ss.clear();
  ss.str(std::string());
  MIRType *mirType = astFile->CvtType(expr.getArg(0)->getType());
  if (mirType != nullptr) {
    PrimType type = mirType->GetPrimType();
    if (type == PTY_f64) {
      ss << "__isinf";
    } else if (type == PTY_f32) {
      ss << "__isinff";
    } else {
      ASSERT(false, "Unsupported type passed to isinf");
    }
  }
  return nullptr;
}

ASTExpr *ASTParser::ParseBuiltinHugeVal(MapleAllocator &allocator, const clang::CallExpr &expr,
                                        std::stringstream &ss) const {
  (void)expr;
  (void)ss;
  ASTFloatingLiteral *astFloatingLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTFloatingLiteral>(allocator);
  astFloatingLiteral->SetKind(FloatKind::F64);
  astFloatingLiteral->SetVal(std::numeric_limits<double>::infinity());
  return astFloatingLiteral;
}

ASTExpr *ASTParser::ParseBuiltinHugeValf(MapleAllocator &allocator, const clang::CallExpr &expr,
                                         std::stringstream &ss) const {
  (void)expr;
  (void)ss;
  ASTFloatingLiteral *astFloatingLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTFloatingLiteral>(allocator);
  astFloatingLiteral->SetKind(FloatKind::F32);
  astFloatingLiteral->SetVal(std::numeric_limits<float>::infinity());
  return astFloatingLiteral;
}

ASTExpr *ASTParser::ParseBuiltinInf(MapleAllocator &allocator, const clang::CallExpr &expr,
                                    std::stringstream &ss) const {
  (void)expr;
  (void)ss;
  ASTFloatingLiteral *astFloatingLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTFloatingLiteral>(allocator);
  astFloatingLiteral->SetKind(FloatKind::F64);
  astFloatingLiteral->SetVal(std::numeric_limits<float>::infinity());
  return astFloatingLiteral;
}

ASTExpr *ASTParser::ParseBuiltinInff(MapleAllocator &allocator, const clang::CallExpr &expr,
                                     std::stringstream &ss) const {
  (void)expr;
  (void)ss;
  ASTFloatingLiteral *astFloatingLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTFloatingLiteral>(allocator);
  astFloatingLiteral->SetKind(FloatKind::F32);
  astFloatingLiteral->SetVal(std::numeric_limits<float>::infinity());
  return astFloatingLiteral;
}

ASTExpr *ASTParser::ParseBuiltinNan(MapleAllocator &allocator, const clang::CallExpr &expr,
                                    std::stringstream &ss) const {
  (void)expr;
  (void)ss;
  ASTFloatingLiteral *astFloatingLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTFloatingLiteral>(allocator);
  astFloatingLiteral->SetKind(FloatKind::F64);
  astFloatingLiteral->SetVal(nan(""));
  return astFloatingLiteral;
}

ASTExpr *ASTParser::ParseBuiltinNanf(MapleAllocator &allocator, const clang::CallExpr &expr,
                                     std::stringstream &ss) const {
  (void)expr;
  (void)ss;
  ASTFloatingLiteral *astFloatingLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTFloatingLiteral>(allocator);
  astFloatingLiteral->SetKind(FloatKind::F32);
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
} // namespace maple
