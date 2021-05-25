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
#include "mpl_logging.h"
#include "feir_stmt.h"
#include "feir_builder.h"
#include "fe_utils_ast.h"
#include "feir_type_helper.h"
#include "fe_manager.h"
#include "ast_stmt.h"
#include "ast_util.h"

namespace maple {
// ---------- ASTValue ----------
MIRConst *ASTValue::Translate2MIRConst() const {
  switch (pty) {
    case PTY_i32: {
      return GlobalTables::GetIntConstTable().GetOrCreateIntConst(
          val.i32, *GlobalTables::GetTypeTable().GetPrimType(PTY_i32));
    }
    case PTY_i64: {
      return GlobalTables::GetIntConstTable().GetOrCreateIntConst(
          val.i64, *GlobalTables::GetTypeTable().GetPrimType(PTY_i64));
    }
    case PTY_f32: {
      return FEManager::GetModule().GetMemPool()->New<MIRFloatConst>(
          val.f32, *GlobalTables::GetTypeTable().GetPrimType(PTY_f32));
    }
    case PTY_f64: {
      return FEManager::GetModule().GetMemPool()->New<MIRDoubleConst>(
          val.f64, *GlobalTables::GetTypeTable().GetPrimType(PTY_f64));
    }
    case PTY_a64: {
      return FEManager::GetModule().GetMemPool()->New<MIRStrConst>(
          val.strIdx, *GlobalTables::GetTypeTable().GetPrimType(PTY_a64));
    }
    default: {
      CHECK_FATAL(false, "Unsupported Primitive type: %d", pty);
    }
  }
}

// ---------- ASTExpr ----------
UniqueFEIRExpr ASTExpr::Emit2FEExpr(std::list<UniqueFEIRStmt> &stmts) const {
  auto feirExpr = Emit2FEExprImpl(stmts);
  for (auto &stmt : stmts) {
    if (!stmt->HasSetLOCInfo()) {
      stmt->SetSrcFileInfo(srcFileIdx, srcFileLineNum);
    }
  }
  return feirExpr;
}

UniqueFEIRExpr ASTExpr::ImplicitInitFieldValue(MIRType *type, std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRExpr implicitInitFieldExpr;
  MIRTypeKind noInitExprKind = type->GetKind();
  if (noInitExprKind == kTypeStruct || noInitExprKind == kTypeUnion) {
    auto *structType = static_cast<MIRStructType*>(type);
    std::string tmpName = FEUtils::GetSequentialName("implicitInitStruct_");
    UniqueFEIRVar tmpVar = FEIRBuilder::CreateVarNameForC(tmpName, *type);
    for (size_t i = 0; i < structType->GetFieldsSize(); ++i) {
      uint32 fieldID = 0;
      FEManager::GetMIRBuilder().TraverseToNamedField(*structType, structType->GetElemStrIdx(i), fieldID);
      MIRType *fieldType = structType->GetFieldType(fieldID);
      UniqueFEIRExpr fieldExpr = ImplicitInitFieldValue(fieldType, stmts);
      UniqueFEIRStmt fieldStmt = std::make_unique<FEIRStmtDAssign>(tmpVar->Clone(), std::move(fieldExpr), fieldID);
      stmts.emplace_back(std::move(fieldStmt));
    }
    implicitInitFieldExpr = FEIRBuilder::CreateExprDRead(std::move(tmpVar));
  } else if (noInitExprKind == kTypeArray) {
    auto *arrayType = static_cast<MIRArrayType*>(type);
    size_t elemSize = arrayType->GetElemType()->GetSize();
    CHECK_FATAL(elemSize != 0, "elemSize is 0");
    size_t numElems = arrayType->GetSize() / elemSize;
    UniqueFEIRType typeNative = FEIRTypeHelper::CreateTypeNative(*type);
    std::string tmpName = FEUtils::GetSequentialName("implicitInitArray_");
    UniqueFEIRVar tmpVar = FEIRBuilder::CreateVarNameForC(tmpName, *type);
    UniqueFEIRExpr arrayExpr = FEIRBuilder::CreateExprDRead(tmpVar->Clone());
    for (size_t i = 0; i < numElems; ++i) {
      UniqueFEIRExpr exprIndex = FEIRBuilder::CreateExprConstI32(i);
      MIRType *fieldType = arrayType->GetElemType();
      UniqueFEIRExpr exprElem = ImplicitInitFieldValue(fieldType, stmts);
      UniqueFEIRType typeNativeTmp = typeNative->Clone();
      UniqueFEIRExpr arrayExprTmp = arrayExpr->Clone();
      auto stmt = FEIRBuilder::CreateStmtArrayStoreOneStmtForC(std::move(exprElem), std::move(arrayExprTmp),
                                                               std::move(exprIndex), std::move(typeNativeTmp),
                                                               tmpName);
      stmts.emplace_back(std::move(stmt));
    }
    implicitInitFieldExpr = FEIRBuilder::CreateExprDRead(std::move(tmpVar));
  } else if (noInitExprKind == kTypePointer) {
    implicitInitFieldExpr = std::make_unique<FEIRExprConst>(static_cast<int64>(0), PTY_ptr);
  } else {
    CHECK_FATAL(noInitExprKind == kTypeScalar, "noInitExprKind isn't kTypeScalar");
    implicitInitFieldExpr = FEIRBuilder::CreateExprConstAnyScalar(type->GetPrimType(), 0);
  }
  return implicitInitFieldExpr;
}

MIRConst *ASTExpr::GenerateMIRConstImpl() const {
  CHECK_FATAL(isConstantFolded && value != nullptr, "Unsupported for ASTExpr: %d", op);
  return value->Translate2MIRConst();
}

// ---------- ASTDeclRefExpr ---------
MIRConst *ASTDeclRefExpr::GenerateMIRConstImpl() const {
  MIRType *mirType = refedDecl->GetTypeDesc().front();
  if (mirType->GetKind() == kTypePointer &&
      static_cast<MIRPtrType*>(mirType)->GetPointedType()->GetKind() == kTypeFunction) {
    GStrIdx idx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(refedDecl->GetName());
#ifndef USE_OPS
    MIRSymbol *funcSymbol = SymbolBuilder::Instance().GetSymbolFromStrIdx(idx);
#else
    MIRSymbol *funcSymbol = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(idx);
#endif
    CHECK_FATAL(funcSymbol != nullptr, "Should process func decl before var decl");
    MIRFunction *mirFunc = funcSymbol->GetFunction();
    CHECK_FATAL(mirFunc != nullptr, "Same name symbol with function: %s", refedDecl->GetName().c_str());
    return FEManager::GetModule().GetMemPool()->New<MIRAddroffuncConst>(mirFunc->GetPuidx(), *mirType);
  } else {
    return GetConstantValue()->Translate2MIRConst();
  }
}

UniqueFEIRExpr ASTDeclRefExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  MIRType *mirType = refedDecl->GetTypeDesc().front();
  UniqueFEIRExpr feirRefExpr;
  auto attrs = refedDecl->GetGenericAttrs();
  if (mirType->GetKind() == kTypePointer &&
      static_cast<MIRPtrType*>(mirType)->GetPointedType()->GetKind() == kTypeFunction) {
    feirRefExpr = FEIRBuilder::CreateExprAddrofFunc(refedDecl->GetName());
  } else {
    UniqueFEIRVar feirVar =
        FEIRBuilder::CreateVarNameForC(refedDecl->GenerateUniqueVarName(), *mirType, refedDecl->IsGlobal(), false);
    if (attrs.GetAttr(GENATTR_static) && !refedDecl->IsGlobal()) {
      feirVar->SetAttrs(attrs);
      if (static_cast<ASTVar*>(refedDecl)->GetInitExpr() != nullptr) {
        feirVar->SetConst(refedDecl->Translate2MIRConst());
      }
    }
    if (mirType->GetKind() == kTypeArray) {
      feirRefExpr = FEIRBuilder::CreateExprAddrofVar(std::move(feirVar));
    } else {
      feirRefExpr = FEIRBuilder::CreateExprDRead(std::move(feirVar));
    }
  }
  return feirRefExpr;
}

// ---------- ASTCallExpr ----------
std::map<std::string, ASTCallExpr::FuncPtrBuiltinFunc> ASTCallExpr::funcPtrMap = ASTCallExpr::InitFuncPtrMap();

std::map<std::string, ASTCallExpr::FuncPtrBuiltinFunc> ASTCallExpr::InitFuncPtrMap() {
  std::map<std::string, FuncPtrBuiltinFunc> ans;
  return ans;
}

std::string ASTCallExpr::CvtBuiltInFuncName(std::string builtInName) const {
#define BUILTIN_FUNC(funcName) \
    {"__builtin_"#funcName, #funcName},
  static std::map<std::string, std::string> cvtMap = {
#include "ast_builtin_func.def"
#undef BUILTIN_FUNC
  };
  auto it = cvtMap.find(builtInName);
  if (it != cvtMap.end()) {
    return cvtMap.find(builtInName)->second;
  } else {
    return builtInName;
  }
}

UniqueFEIRExpr ASTCallExpr::Emit2FEExprCall(std::list<UniqueFEIRStmt> &stmts) const {
  // callassigned &funcName
  StructElemNameIdx *nameIdx = FEManager::GetManager().GetStructElemMempool()->New<StructElemNameIdx>(funcName);
  FEStructMethodInfo *info = static_cast<FEStructMethodInfo*>(
      FEManager::GetTypeManager().RegisterStructMethodInfo(*nameIdx, kSrcLangC, false));
  Opcode op;
  if (retType->GetPrimType() != PTY_void) {
    op = OP_callassigned;
  } else {
    op = OP_call;
  }
  std::unique_ptr<FEIRStmtCallAssign> callStmt = std::make_unique<FEIRStmtCallAssign>(*info, op, nullptr, false);
  // args
  for (int32 i = args.size() - 1; i >= 0; --i) {
    UniqueFEIRExpr expr = args[i]->Emit2FEExpr(stmts);
    callStmt->AddExprArgReverse(std::move(expr));
  }
  // attrs
  info->SetFuncAttrs(funcAttrs);
  // return
  FEIRTypeNative *retTypeInfo = FEManager::GetManager().GetModule().GetMemPool()->New<FEIRTypeNative>(*retType);
  info->SetReturnType(retTypeInfo);
  if (retType->GetPrimType() != PTY_void) {
    UniqueFEIRVar var = FEIRBuilder::CreateVarNameForC(varName, *retType, false, false);
    UniqueFEIRVar dreadVar = var->Clone();
    callStmt->SetVar(std::move(var));
    stmts.emplace_back(std::move(callStmt));
    return FEIRBuilder::CreateExprDRead(std::move(dreadVar));
  }
  stmts.emplace_back(std::move(callStmt));
  return nullptr;
}

UniqueFEIRExpr ASTCallExpr::Emit2FEExprICall(std::list<UniqueFEIRStmt> &stmts) const {
  std::unique_ptr<FEIRStmtICallAssign> icallStmt = std::make_unique<FEIRStmtICallAssign>();
  CHECK_NULL_FATAL(calleeExpr);
  // args
  for (int32 i = args.size() - 1; i >= 0; --i) {
    UniqueFEIRExpr exprArg = args[i]->Emit2FEExpr(stmts);
    icallStmt->AddExprArgReverse(std::move(exprArg));
  }
  UniqueFEIRExpr expr = calleeExpr->Emit2FEExpr(stmts);
  icallStmt->AddExprArgReverse(std::move(expr));
  // return
  if (retType->GetPrimType() != PTY_void) {
    UniqueFEIRVar var = FEIRBuilder::CreateVarNameForC(varName, *retType, false, false);
    UniqueFEIRVar dreadVar = var->Clone();
    icallStmt->SetVar(std::move(var));
    stmts.emplace_back(std::move(icallStmt));
    return FEIRBuilder::CreateExprDRead(std::move(dreadVar));
  }
  stmts.emplace_back(std::move(icallStmt));
  return nullptr;
}

UniqueFEIRExpr ASTCallExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  if (isIcall) {
    return Emit2FEExprICall(stmts);
  } else {
    if (calleeExpr != nullptr && calleeExpr->GetASTOp() == kASTOpCast &&
        static_cast<ASTImplicitCastExpr*>(calleeExpr)->IsBuilinFunc()) {
      auto ptrFunc = funcPtrMap.find(funcName);
      if (ptrFunc != funcPtrMap.end()) {
        return (this->*(ptrFunc->second))(stmts);
      }
    }
    return Emit2FEExprCall(stmts);
  }
}

// ---------- ASTImplicitCastExpr ----------
ASTValue *ASTImplicitCastExpr::GetConstantValueImpl() const {
  return child->GetConstantValue();
}

MIRConst *ASTImplicitCastExpr::GenerateMIRConstImpl() const {
  if (isArrayToPointerDecay && child->GetASTOp() == kASTStringLiteral) {
    return FEManager::GetModule().GetMemPool()->New<MIRStrConst>(
        GetConstantValue()->val.strIdx, *GlobalTables::GetTypeTable().GetPrimType(PTY_a64));
  } else if (isNeededCvt) {
    if (dst->GetPrimType() == PTY_f64) {
      return GenerateMIRDoubleConst();
    } else if (dst->GetPrimType() == PTY_f32) {
      return GenerateMIRFloatConst();
    } else {
      return GenerateMIRIntConst();
    }
  } else {
    return child->GenerateMIRConst();
  }
}

MIRConst *ASTImplicitCastExpr::GenerateMIRDoubleConst() const {
  switch (GetConstantValue()->pty) {
    case PTY_f32: {
      return FEManager::GetModule().GetMemPool()->New<MIRDoubleConst>(
          static_cast<double>(GetConstantValue()->val.f32), *GlobalTables::GetTypeTable().GetPrimType(PTY_f64));
    }
    case PTY_i64: {
      return FEManager::GetModule().GetMemPool()->New<MIRDoubleConst>(
          static_cast<double>(GetConstantValue()->val.i64), *GlobalTables::GetTypeTable().GetPrimType(PTY_f64));
    }
    case PTY_f64: {
      return FEManager::GetModule().GetMemPool()->New<MIRDoubleConst>(
          static_cast<double>(GetConstantValue()->val.f64), *GlobalTables::GetTypeTable().GetPrimType(PTY_f64));
    }
    default: {
      CHECK_FATAL(false, "Unsupported pty type: %d", GetConstantValue()->pty);
      return nullptr;
    }
  }
}

MIRConst *ASTImplicitCastExpr::GenerateMIRFloatConst() const {
  switch (GetConstantValue()->pty) {
    case PTY_f64: {
      return FEManager::GetModule().GetMemPool()->New<MIRFloatConst>(
          static_cast<float>(GetConstantValue()->val.f64), *GlobalTables::GetTypeTable().GetPrimType(PTY_f32));
    }
    case PTY_i64: {
      return FEManager::GetModule().GetMemPool()->New<MIRFloatConst>(
          static_cast<float>(GetConstantValue()->val.i64), *GlobalTables::GetTypeTable().GetPrimType(PTY_f32));
    }
    default: {
      CHECK_FATAL(false, "Unsupported pty type: %d", GetConstantValue()->pty);
      return nullptr;
    }
  }
}

MIRConst *ASTImplicitCastExpr::GenerateMIRIntConst() const {
  switch (GetConstantValue()->pty) {
    case PTY_f64: {
      return FEManager::GetModule().GetMemPool()->New<MIRIntConst>(
          static_cast<int64>(GetConstantValue()->val.f64), *GlobalTables::GetTypeTable().GetPrimType(PTY_i64));
    }
    case PTY_i64: {
      return FEManager::GetModule().GetMemPool()->New<MIRIntConst>(
          GetConstantValue()->val.i64, *GlobalTables::GetTypeTable().GetPrimType(PTY_i64));
    }
    default: {
      CHECK_FATAL(false, "Unsupported pty type: %d", GetConstantValue()->pty);
      return nullptr;
    }
  }
}

UniqueFEIRExpr ASTImplicitCastExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  const ASTExpr *childExpr = child;
  UniqueFEIRType srcType = std::make_unique<FEIRTypeNative>(*src);
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr subExpr = childExpr->Emit2FEExpr(stmts);
  if (complexType == nullptr) {
    if (IsNeededCvt(subExpr)) {
      if (IsPrimitiveFloat(subExpr->GetPrimType()) && IsPrimitiveInteger(dst->GetPrimType())) {
        return FEIRBuilder::CreateExprCvtPrim(OP_trunc, std::move(subExpr), dst->GetPrimType());
      }
      return FEIRBuilder::CreateExprCvtPrim(std::move(subExpr), dst->GetPrimType());
    }
  } else {
    std::string tmpName = FEUtils::GetSequentialName("Complex_");
    UniqueFEIRVar tmpVar = FEIRBuilder::CreateVarNameForC(tmpName, *complexType);
    UniqueFEIRExpr dreadAgg;
    if (imageZero) {
      UniqueFEIRStmt realStmtNode = std::make_unique<FEIRStmtDAssign>(tmpVar->Clone(),
          subExpr->Clone(), kComplexRealID);
      stmts.emplace_back(std::move(realStmtNode));
      UniqueFEIRExpr imagExpr = FEIRBuilder::CreateExprConstAnyScalar(src->GetPrimType(), 0);
      UniqueFEIRStmt imagStmtNode = std::make_unique<FEIRStmtDAssign>(tmpVar->Clone(),
          imagExpr->Clone(), kComplexImagID);
      stmts.emplace_back(std::move(imagStmtNode));
      dreadAgg = FEIRBuilder::CreateExprDRead(std::move(tmpVar));
      static_cast<FEIRExprDRead*>(dreadAgg.get())->SetFieldType(srcType->Clone());
    } else {
      UniqueFEIRExpr realExpr;
      UniqueFEIRExpr imagExpr;
      FEIRNodeKind subNodeKind = subExpr->GetKind();
      UniqueFEIRExpr cloneSubExpr = subExpr->Clone();
      if (subNodeKind == kExprIRead) {
        static_cast<FEIRExprIRead*>(subExpr.get())->SetFieldID(kComplexRealID);
        static_cast<FEIRExprIRead*>(cloneSubExpr.get())->SetFieldID(kComplexImagID);
      } else if (subNodeKind == kExprDRead) {
        static_cast<FEIRExprDRead*>(subExpr.get())->SetFieldID(kComplexRealID);
        static_cast<FEIRExprDRead*>(subExpr.get())->SetFieldType(srcType->Clone());
        static_cast<FEIRExprDRead*>(cloneSubExpr.get())->SetFieldID(kComplexImagID);
        static_cast<FEIRExprDRead*>(cloneSubExpr.get())->SetFieldType(srcType->Clone());
      }
      realExpr = FEIRBuilder::CreateExprCvtPrim(std::move(subExpr), dst->GetPrimType());
      imagExpr = FEIRBuilder::CreateExprCvtPrim(std::move(cloneSubExpr), dst->GetPrimType());
      UniqueFEIRStmt realStmt = std::make_unique<FEIRStmtDAssign>(tmpVar->Clone(), std::move(realExpr), kComplexRealID);
      stmts.emplace_back(std::move(realStmt));
      UniqueFEIRStmt imagStmt = std::make_unique<FEIRStmtDAssign>(tmpVar->Clone(), std::move(imagExpr), kComplexImagID);
      stmts.emplace_back(std::move(imagStmt));
      dreadAgg = FEIRBuilder::CreateExprDRead(std::move(tmpVar));
    }
    return dreadAgg;
  }
  return subExpr;
}

bool ASTImplicitCastExpr::IsNeededCvt(const UniqueFEIRExpr &expr) const {
  if (!isNeededCvt || expr == nullptr || dst == nullptr) {
    return false;
  }
  PrimType srcPrimType = expr->GetPrimType();
  return srcPrimType != dst->GetPrimType() && srcPrimType != PTY_agg && srcPrimType != PTY_void;
}

// ---------- ASTUnaryOperatorExpr ----------
void ASTUnaryOperatorExpr::SetUOExpr(ASTExpr *astExpr) {
  expr = astExpr;
}

void ASTUnaryOperatorExpr::SetSubType(MIRType *type) {
  subType = type;
}

UniqueFEIRExpr ASTUOMinusExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr childFEIRExpr = childExpr->Emit2FEExpr(stmts);
  PrimType dstType = uoType->GetPrimType();
  if (childFEIRExpr->GetPrimType() != dstType) {
    UniqueFEIRExpr minusExpr = std::make_unique<FEIRExprUnary>(OP_neg, subType, std::move(childFEIRExpr));
    return FEIRBuilder::CreateExprCvtPrim(std::move(minusExpr), dstType);
  }
  return std::make_unique<FEIRExprUnary>(OP_neg, subType, std::move(childFEIRExpr));
}

UniqueFEIRExpr ASTUOPlusExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr plusExpr = childExpr->Emit2FEExpr(stmts);
  return plusExpr;
}

UniqueFEIRExpr ASTUONotExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr childFEIRExpr = childExpr->Emit2FEExpr(stmts);
  PrimType dstType = uoType->GetPrimType();
  if (childFEIRExpr->GetPrimType() != dstType) {
    UniqueFEIRExpr notExpr = std::make_unique<FEIRExprUnary>(OP_bnot, subType, std::move(childFEIRExpr));
    return FEIRBuilder::CreateExprCvtPrim(std::move(notExpr), dstType);
  }
  return std::make_unique<FEIRExprUnary>(OP_bnot, subType, std::move(childFEIRExpr));
}

UniqueFEIRExpr ASTUOLNotExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr childFEIRExpr = childExpr->Emit2FEExpr(stmts);
  PrimType dstType = uoType->GetPrimType();
  if (childFEIRExpr->GetPrimType() != dstType) {
    UniqueFEIRExpr lnotExpr = std::make_unique<FEIRExprUnary>(OP_lnot, subType, std::move(childFEIRExpr));
    return FEIRBuilder::CreateExprCvtPrim(std::move(lnotExpr), dstType);
  }
  return std::make_unique<FEIRExprUnary>(OP_lnot, subType, std::move(childFEIRExpr));
}

UniqueFEIRExpr ASTUOPostIncExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr childFEIRExpr = childExpr->Emit2FEExpr(stmts);
  UniqueFEIRExpr readSelfExpr;
  UniqueFEIRVar selfVar;
  if (childExpr->GetASTOp() == kASTMemberExpr) {
    readSelfExpr = childFEIRExpr->Clone();
  } else {
    selfVar = FEIRBuilder::CreateVarNameForC(refedDecl->GenerateUniqueVarName(), *subType, isGlobal, false);
    readSelfExpr = FEIRBuilder::CreateExprDRead(selfVar->Clone());
  }
  UniqueFEIRVar tempVar = FEIRBuilder::CreateVarNameForC(tempVarName, *subType);
  UniqueFEIRStmt readSelfstmt = FEIRBuilder::CreateStmtDAssign(tempVar->Clone(), std::move(readSelfExpr));
  stmts.emplace_back(std::move(readSelfstmt));
  PrimType subPrimType = subType->GetPrimType();
  UniqueFEIRExpr incIecExpr = (subPrimType == PTY_ptr) ? std::make_unique<FEIRExprConst>(pointeeLen, PTY_i32) :
                                                         FEIRBuilder::CreateExprConstAnyScalar(subPrimType, 1);
  UniqueFEIRExpr selfAddExpr = FEIRBuilder::CreateExprMathBinary(OP_add, childFEIRExpr->Clone(),
                                                                 std::move(incIecExpr));
  UniqueFEIRStmt selfAddStmt;
  if (childExpr->GetASTOp() == kASTMemberExpr) {
    auto ireadFEExpr = static_cast<FEIRExprIRead*>(childFEIRExpr.get());
    selfAddStmt = std::make_unique<FEIRStmtIAssign>(ireadFEExpr->GetClonedPtrType(), ireadFEExpr->GetClonedOpnd(),
                                                    std::move(selfAddExpr), ireadFEExpr->GetFieldID());
  } else {
    selfAddStmt = FEIRBuilder::CreateStmtDAssign(std::move(selfVar), std::move(selfAddExpr));
  }
  stmts.emplace_back(std::move(selfAddStmt));
  UniqueFEIRExpr readTempExpr = FEIRBuilder::CreateExprDRead(std::move(tempVar));
  return readTempExpr;
}

UniqueFEIRExpr ASTUOPostDecExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr childFEIRExpr = childExpr->Emit2FEExpr(stmts);
  UniqueFEIRExpr readSelfExpr;
  UniqueFEIRVar selfVar;
  if (childExpr->GetASTOp() == kASTMemberExpr) {
    readSelfExpr = childFEIRExpr->Clone();
  } else {
    selfVar = FEIRBuilder::CreateVarNameForC(refedDecl->GenerateUniqueVarName(), *subType, isGlobal, false);
    readSelfExpr = FEIRBuilder::CreateExprDRead(selfVar->Clone());
  }
  UniqueFEIRVar tempVar = FEIRBuilder::CreateVarNameForC(tempVarName, *subType);
  UniqueFEIRVar tempMoveVar = tempVar->Clone();
  UniqueFEIRStmt readSelfstmt = FEIRBuilder::CreateStmtDAssign(std::move(tempMoveVar), std::move(readSelfExpr));
  stmts.emplace_back(std::move(readSelfstmt));
  PrimType subPrimType = subType->GetPrimType();
  UniqueFEIRExpr incDecExpr = (subPrimType == PTY_ptr) ? std::make_unique<FEIRExprConst>(pointeeLen, PTY_i32) :
                                                         FEIRBuilder::CreateExprConstAnyScalar(subPrimType, 1);
  UniqueFEIRExpr selfSubExpr = FEIRBuilder::CreateExprMathBinary(OP_sub, childFEIRExpr->Clone(),
                                                                 std::move(incDecExpr));
  UniqueFEIRStmt selfSubStmt;
  if (childExpr->GetASTOp() == kASTMemberExpr) {
    auto ireadFEExpr = static_cast<FEIRExprIRead*>(childFEIRExpr.get());
    selfSubStmt = std::make_unique<FEIRStmtIAssign>(ireadFEExpr->GetClonedPtrType(), ireadFEExpr->GetClonedOpnd(),
                                                    std::move(selfSubExpr), ireadFEExpr->GetFieldID());
  } else {
    selfSubStmt = FEIRBuilder::CreateStmtDAssign(std::move(selfVar), std::move(selfSubExpr));
  }
  stmts.emplace_back(std::move(selfSubStmt));
  UniqueFEIRExpr readTempExpr = FEIRBuilder::CreateExprDRead(std::move(tempVar));
  return readTempExpr;
}

UniqueFEIRExpr ASTUOPreIncExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr childFEIRExpr = childExpr->Emit2FEExpr(stmts);
  PrimType subPrimType = subType->GetPrimType();
  UniqueFEIRExpr incIecExpr = (subPrimType == PTY_ptr) ? std::make_unique<FEIRExprConst>(pointeeLen, PTY_i32) :
                                                         FEIRBuilder::CreateExprConstAnyScalar(subPrimType, 1);
  UniqueFEIRExpr astUOPreIncExpr = FEIRBuilder::CreateExprMathBinary(OP_add, childFEIRExpr->Clone(),
                                                                     std::move(incIecExpr));
  UniqueFEIRStmt stmt;
  UniqueFEIRExpr retExpr;
  if (childExpr->GetASTOp() == kASTMemberExpr) {
    auto ireadFEExpr = static_cast<FEIRExprIRead*>(childFEIRExpr.get());
    stmt = std::make_unique<FEIRStmtIAssign>(ireadFEExpr->GetClonedPtrType(), ireadFEExpr->GetClonedOpnd(),
                                             std::move(astUOPreIncExpr), ireadFEExpr->GetFieldID());
    retExpr = std::move(childFEIRExpr);
  } else {
    UniqueFEIRVar selfVar = FEIRBuilder::CreateVarNameForC(refedDecl->GenerateUniqueVarName(),
                                                           *subType, isGlobal, false);
    UniqueFEIRVar selfMoveVar = selfVar->Clone();
    stmt = FEIRBuilder::CreateStmtDAssign(std::move(selfMoveVar), std::move(astUOPreIncExpr));
    retExpr = FEIRBuilder::CreateExprDRead(std::move(selfVar));
  }
  stmts.emplace_back(std::move(stmt));
  return retExpr;
}

UniqueFEIRExpr ASTUOPreDecExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr childFEIRExpr = childExpr->Emit2FEExpr(stmts);
  PrimType subPrimType = subType->GetPrimType();
  UniqueFEIRExpr incDecExpr = (subPrimType == PTY_ptr) ? std::make_unique<FEIRExprConst>(pointeeLen, PTY_i32) :
                                                         FEIRBuilder::CreateExprConstAnyScalar(subPrimType, 1);
  UniqueFEIRExpr astUOPreDecExpr = FEIRBuilder::CreateExprMathBinary(OP_sub, childFEIRExpr->Clone(),
                                                                     std::move(incDecExpr));
  UniqueFEIRStmt stmt;
  UniqueFEIRExpr retExpr;
  if (childExpr->GetASTOp() == kASTMemberExpr) {
    auto ireadFEExpr = static_cast<FEIRExprIRead*>(childFEIRExpr.get());
    stmt = std::make_unique<FEIRStmtIAssign>(ireadFEExpr->GetClonedPtrType(), ireadFEExpr->GetClonedOpnd(),
                                             std::move(astUOPreDecExpr), ireadFEExpr->GetFieldID());
    retExpr = std::move(childFEIRExpr);
  } else {
    UniqueFEIRVar selfVar = FEIRBuilder::CreateVarNameForC(refedDecl->GenerateUniqueVarName(), *subType,
                                                           isGlobal, false);
    UniqueFEIRVar selfMoveVar = selfVar->Clone();
    stmt = FEIRBuilder::CreateStmtDAssign(std::move(selfMoveVar), std::move(astUOPreDecExpr));
    retExpr = FEIRBuilder::CreateExprDRead(std::move(selfVar));
  }
  stmts.emplace_back(std::move(stmt));
  return retExpr;
}

MIRConst *ASTUOAddrOfExpr::GenerateMIRConstImpl() const {
  switch (expr->GetASTOp()) {
    case kASTOpRef: {
      ASTDecl *var = static_cast<ASTDeclRefExpr*>(expr)->GetASTDecl();
      MIRSymbol *mirSymbol = FEManager::GetMIRBuilder().GetOrCreateGlobalDecl(
          var->GenerateUniqueVarName(), *(var->GetTypeDesc().front()));
      MIRAddrofConst *konst = FEManager::GetModule().GetMemPool()->New<MIRAddrofConst>(
          mirSymbol->GetStIdx(), 0, *(var->GetTypeDesc().front()));
      return konst;
    }
    case kASTSubscriptExpr: {
#ifndef USE_OPS
      CHECK_FATAL(false, "Unsupported");
      return nullptr;
#else
      ASTDecl *var = static_cast<ASTArraySubscriptExpr*>(expr)->GetBaseExpr()->GetASTDecl();
      MIRSymbol *mirSymbol = FEManager::GetMIRBuilder().GetOrCreateGlobalDecl(
          var->GenerateUniqueVarName(), *(var->GetTypeDesc().front()));
      int32 offset = static_cast<ASTArraySubscriptExpr*>(expr)->TranslateArraySubscript2Offset();
      MIRAddrofConst *konst = FEManager::GetModule().GetMemPool()->New<MIRAddrofConst>(
          mirSymbol->GetStIdx(), 0, *(var->GetTypeDesc().front()), offset);
      return konst;
#endif
    }
    case kASTMemberExpr: {
      CHECK_FATAL(false, "MemberExpr and SubscriptExpr nested NIY");
      return nullptr;
    }
    default: {
      CHECK_FATAL(false, "lValue in expr: %d NIY", expr->GetASTOp());
    }
  }
  return nullptr;
}

UniqueFEIRExpr ASTUOAddrOfExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr addrOfExpr;
  auto astOp = childExpr->GetASTOp();
  if (astOp == kASTOpRef) {
    ASTDecl *var = static_cast<ASTDeclRefExpr*>(childExpr)->GetASTDecl();
    UniqueFEIRVar addrOfVar = FEIRBuilder::CreateVarNameForC(var->GenerateUniqueVarName(),
                                                             *(var->GetTypeDesc().front()),
                                                             var->IsGlobal(), false);
    addrOfExpr = FEIRBuilder::CreateExprAddrofVar(std::move(addrOfVar));
  } else if (astOp == kASTMemberExpr) {
    ASTMemberExpr *memberExpr = static_cast<ASTMemberExpr*>(childExpr);
    ASTExpr *baseExpr = memberExpr->GetBaseExpr();
    UniqueFEIRExpr baseFEExpr;
    std::string fieldName = memberExpr->GetMemberName();
    bool isArrow = memberExpr->GetIsArrow();
    MIRType *baseType = memberExpr->GetBaseType();
    if (baseExpr->GetASTOp() == kASTMemberExpr) {
      std::list<std::string> memberNameList;
      memberNameList.emplace_back(memberExpr->GetMemberName());
      ASTMemberExpr *finalMember = memberExpr->findFinalMember(static_cast<ASTMemberExpr*>(baseExpr), memberNameList);
      baseFEExpr = finalMember->GetBaseExpr()->Emit2FEExpr(stmts);
      isArrow = finalMember->GetIsArrow();
      baseType = finalMember->GetBaseType();
      fieldName = ASTUtil::Join(memberNameList, ".");
    } else {
      baseFEExpr = baseExpr->Emit2FEExpr(stmts);
    }

    FieldID fieldID = 0;
    if (isArrow) {
      CHECK_FATAL(baseType->IsMIRPtrType(), "Must be ptr type!");
      MIRPtrType *mirPtrType = static_cast<MIRPtrType*>(baseType);
      MIRType *pointedMirType = mirPtrType->GetPointedType();
      CHECK_FATAL(pointedMirType->IsStructType(), "pointedMirType must be StructType");
      MIRStructType *structType = static_cast<MIRStructType*>(pointedMirType);
      fieldID = FEUtils::GetStructFieldID(structType, fieldName);
      UniqueFEIRType baseFEType = std::make_unique<FEIRTypeNative>(*baseType);
      addrOfExpr = std::make_unique<FEIRExprIAddrof>(std::move(baseFEType), fieldID, std::move(baseFEExpr));
    } else {
       // other potential expr should concern
      CHECK_FATAL(baseFEExpr->GetKind() == kExprDRead, "baseFEExpr kind isn't DRead");
      auto dreadFEExpr = static_cast<FEIRExprDRead*>(baseFEExpr.get());
      UniqueFEIRVar tmpVar = dreadFEExpr->GetVar()->Clone();
      CHECK_FATAL(baseType->IsStructType(), "basetype must be StructType");
      MIRStructType *structType = static_cast<MIRStructType*>(baseType);
      fieldID = FEUtils::GetStructFieldID(structType, fieldName);
      addrOfExpr = FEIRBuilder::CreateExprAddrofVar(std::move(tmpVar));
      static_cast<FEIRExprAddrofVar*>(addrOfExpr.get())->SetFieldID(fieldID);
    }
  } else if (astOp == kASTSubscriptExpr) {
    ASTArraySubscriptExpr *arraySubExpr = static_cast<ASTArraySubscriptExpr*>(childExpr);
    arraySubExpr->SetAddrOfFlag(true);
    addrOfExpr = childExpr->Emit2FEExpr(stmts);
  } else { // other potential expr should concern
    UniqueFEIRExpr childFEIRExpr;
    childFEIRExpr = childExpr->Emit2FEExpr(stmts);
    if (childFEIRExpr->GetKind() == kExprDRead && childFEIRExpr->GetPrimType() == PTY_agg) {
      FEIRExprDRead *dread = static_cast<FEIRExprDRead*>(childFEIRExpr.get());
      UniqueFEIRVar dreadVar = dread->GetVar()->Clone();
      addrOfExpr = FEIRBuilder::CreateExprAddrofVar(std::move(dreadVar));
    } else {
      addrOfExpr = std::make_unique<FEIRExprUnary>(OP_addrof, uoType, std::move(childFEIRExpr));
    }
  }
  return addrOfExpr;
}

UniqueFEIRExpr ASTUODerefExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr childFEIRExpr = childExpr->Emit2FEExpr(stmts);
  UniqueFEIRType retType = std::make_unique<FEIRTypeNative>(*uoType);
  UniqueFEIRType ptrType = std::make_unique<FEIRTypeNative>(*subType);
  if (uoType->GetKind() == kTypePointer &&
      static_cast<MIRPtrType*>(uoType)->GetPointedType()->GetKind() == kTypeFunction) {
    return childFEIRExpr;
  }
  UniqueFEIRExpr derefExpr = FEIRBuilder::CreateExprIRead(std::move(retType), std::move(ptrType),
                                                          std::move(childFEIRExpr));
  return derefExpr;
}

UniqueFEIRExpr ASTUORealExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  ASTOp astOP = childExpr->GetASTOp();
  UniqueFEIRExpr subFEIRExpr;
  if (astOP == kASTStringLiteral || astOP == kASTIntegerLiteral || astOP == kASTFloatingLiteral ||
      astOP == kASTCharacterLiteral || astOP == kASTImaginaryLiteral) {
    subFEIRExpr = childExpr->Emit2FEExpr(stmts);
  } else {
    subFEIRExpr = childExpr->Emit2FEExpr(stmts);
    FEIRNodeKind subNodeKind = subFEIRExpr->GetKind();
    if (subNodeKind == kExprIRead) {
      static_cast<FEIRExprIRead*>(subFEIRExpr.get())->SetFieldID(kComplexRealID);
    } else if (subNodeKind == kExprDRead) {
      static_cast<FEIRExprDRead*>(subFEIRExpr.get())->SetFieldID(kComplexRealID);
      UniqueFEIRType elementFEType = std::make_unique<FEIRTypeNative>(*elementType);
      static_cast<FEIRExprDRead*>(subFEIRExpr.get())->SetFieldType(std::move(elementFEType));
    } else {
      CHECK_FATAL(false, "NIY");
    }
  }
  return subFEIRExpr;
}

UniqueFEIRExpr ASTUOImagExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  ASTOp astOP = childExpr->GetASTOp();
  UniqueFEIRExpr subFEIRExpr;
  if (astOP == kASTStringLiteral || astOP == kASTIntegerLiteral || astOP == kASTFloatingLiteral ||
      astOP == kASTCharacterLiteral || astOP == kASTImaginaryLiteral) {
    subFEIRExpr = childExpr->Emit2FEExpr(stmts);
  } else {
    subFEIRExpr = childExpr->Emit2FEExpr(stmts);
    FEIRNodeKind subNodeKind = subFEIRExpr->GetKind();
    if (subNodeKind == kExprIRead) {
      static_cast<FEIRExprIRead*>(subFEIRExpr.get())->SetFieldID(kComplexImagID);
    } else if (subNodeKind == kExprDRead) {
      static_cast<FEIRExprDRead*>(subFEIRExpr.get())->SetFieldID(kComplexImagID);
      UniqueFEIRType elementFEType = std::make_unique<FEIRTypeNative>(*elementType);
      static_cast<FEIRExprDRead*>(subFEIRExpr.get())->SetFieldType(std::move(elementFEType));
    } else {
      CHECK_FATAL(false, "NIY");
    }
  }
  return subFEIRExpr;
}

UniqueFEIRExpr ASTUOExtensionExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  return expr->Emit2FEExpr(stmts);
}

UniqueFEIRExpr ASTUOCoawaitExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "C++ feature");
  return nullptr;
}

// ---------- ASTPredefinedExpr ----------
UniqueFEIRExpr ASTPredefinedExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  return child->Emit2FEExpr(stmts);
}

void ASTPredefinedExpr::SetASTExpr(ASTExpr *astExpr) {
  child = astExpr;
}

// ---------- ASTOpaqueValueExpr ----------
UniqueFEIRExpr ASTOpaqueValueExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  return child->Emit2FEExpr(stmts);
}

void ASTOpaqueValueExpr::SetASTExpr(ASTExpr *astExpr) {
  child = astExpr;
}

// ---------- ASTBinaryConditionalOperator ----------
UniqueFEIRExpr ASTBinaryConditionalOperator::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRExpr condFEIRExpr = condExpr->Emit2FEExpr(stmts);
  UniqueFEIRExpr trueFEIRExpr;
  CHECK_NULL_FATAL(mirType);
  // if a conditional expr is noncomparative, e.g., b = a ?: c
  // the conditional expr will be use for trueExpr before it will be converted to comparative expr
  if (!(condFEIRExpr->GetKind() == kExprBinary && static_cast<FEIRExprBinary*>(condFEIRExpr.get())->IsComparative())) {
    trueFEIRExpr = condFEIRExpr->Clone();
    UniqueFEIRExpr zeroConstExpr = (condFEIRExpr->GetPrimType() == PTY_ptr) ?
        FEIRBuilder::CreateExprConstPtrNull() :
        FEIRBuilder::CreateExprConstAnyScalar(condFEIRExpr->GetPrimType(), 0);
    condFEIRExpr = FEIRBuilder::CreateExprMathBinary(OP_ne, std::move(condFEIRExpr), std::move(zeroConstExpr));
  } else {
    // if a conditional expr already is comparative (only return u1 val 0 or 1), e.g., b = (a < 0) ?: c
    // the conditional expr will be assigned var used for comparative expr and true expr meanwhile
    MIRType *condType = condFEIRExpr->GetType()->GenerateMIRTypeAuto();
    ASSERT_NOT_NULL(condType);
    UniqueFEIRVar condVar = FEIRBuilder::CreateVarNameForC(FEUtils::GetSequentialName("condVal_"), *condType);
    UniqueFEIRVar condVarCloned = condVar->Clone();
    UniqueFEIRVar condVarCloned2 = condVar->Clone();
    UniqueFEIRStmt condStmt = FEIRBuilder::CreateStmtDAssign(std::move(condVar), std::move(condFEIRExpr));
    stmts.emplace_back(std::move(condStmt));
    condFEIRExpr = FEIRBuilder::CreateExprDRead(std::move(condVarCloned));
    if (condType->GetPrimType() != mirType->GetPrimType()) {
      trueFEIRExpr = FEIRBuilder::CreateExprCvtPrim(std::move(condVarCloned2), mirType->GetPrimType());
    } else {
      trueFEIRExpr = FEIRBuilder::CreateExprDRead(std::move(condVarCloned2));
    }
  }
  std::list<UniqueFEIRStmt> falseStmts;
  UniqueFEIRExpr falseFEIRExpr = falseExpr->Emit2FEExpr(falseStmts);
  // There are no extra nested statements in false expressions, (e.g., a < 1 ?: 2), use ternary FEIRExpr
  if (falseStmts.empty()) {
    UniqueFEIRType type = std::make_unique<FEIRTypeNative>(*mirType);
    return FEIRBuilder::CreateExprTernary(OP_select, std::move(type), std::move(condFEIRExpr),
                                          std::move(trueFEIRExpr), std::move(falseFEIRExpr));
  }
  // Otherwise, (e.g., a < 1 ?: a++) create a temporary var to hold the return trueExpr or falseExpr value
  CHECK_FATAL(falseFEIRExpr->GetPrimType() == mirType->GetPrimType(), "The type of falseFEIRExpr are inconsistent");
  UniqueFEIRVar tempVar = FEIRBuilder::CreateVarNameForC(FEUtils::GetSequentialName("levVar_"), *mirType);
  UniqueFEIRVar tempVarCloned1 = tempVar->Clone();
  UniqueFEIRVar tempVarCloned2 = tempVar->Clone();
  UniqueFEIRStmt retTrueStmt = FEIRBuilder::CreateStmtDAssign(std::move(tempVar), std::move(trueFEIRExpr));
  std::list<UniqueFEIRStmt> trueStmts;
  trueStmts.emplace_back(std::move(retTrueStmt));
  UniqueFEIRStmt retFalseStmt = FEIRBuilder::CreateStmtDAssign(std::move(tempVarCloned1), std::move(falseFEIRExpr));
  falseStmts.emplace_back(std::move(retFalseStmt));
  UniqueFEIRStmt stmtIf = FEIRBuilder::CreateStmtIf(std::move(condFEIRExpr), trueStmts, falseStmts);
  stmts.emplace_back(std::move(stmtIf));
  return FEIRBuilder::CreateExprDRead(std::move(tempVarCloned2));
}

void ASTBinaryConditionalOperator::SetCondExpr(ASTExpr *expr) {
  condExpr = expr;
}

void ASTBinaryConditionalOperator::SetFalseExpr(ASTExpr *expr) {
  falseExpr = expr;
}

// ---------- ASTNoInitExpr ----------
UniqueFEIRExpr ASTNoInitExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  return ImplicitInitFieldValue(noInitType, stmts);
}

void ASTNoInitExpr::SetNoInitType(MIRType *type) {
  noInitType = type;
}

// ---------- ASTCompoundLiteralExpr ----------
UniqueFEIRExpr ASTCompoundLiteralExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRExpr feirExpr;
  if (child->GetASTOp() == kASTOpInitListExpr) { // other potential expr should concern
    std::string tmpName = FEUtils::GetSequentialName("clvar_");
    static_cast<ASTInitListExpr*>(child)->SetInitListVarName(tmpName);
    child->Emit2FEExpr(stmts);
    UniqueFEIRVar tmpVar = FEIRBuilder::CreateVarNameForC(tmpName, *compoundLiteralType);
    feirExpr = FEIRBuilder::CreateExprDRead(std::move(tmpVar));
  } else {
    feirExpr = child->Emit2FEExpr(stmts);
  }
  return feirExpr;
}

void ASTCompoundLiteralExpr::SetCompoundLiteralType(MIRType *clType) {
  compoundLiteralType = clType;
}

void ASTCompoundLiteralExpr::SetASTExpr(ASTExpr *astExpr) {
  child = astExpr;
}

// ---------- ASTOffsetOfExpr ----------
void ASTOffsetOfExpr::SetStructType(MIRType *stype) {
  structType = stype;
}

void ASTOffsetOfExpr::SetFieldName(std::string fName){
  fieldName = fName;
}

UniqueFEIRExpr ASTOffsetOfExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  return FEIRBuilder::CreateExprConstU64(static_cast<uint64>(offset));
}

// ---------- ASTInitListExpr ----------
MIRConst *ASTInitListExpr::GenerateMIRConstImpl() const {
  if (!initListType->IsStructType()) {
    return GenerateMIRConstForArray();
  } else {
    return GenerateMIRConstForStruct();
  }
}

MIRConst *ASTInitListExpr::GenerateMIRConstForArray() const {
  if (fillers.size() == 1 && fillers[0]->GetASTOp() == kASTStringLiteral) {
    return fillers[0]->GenerateMIRConst();
  }
  MIRAggConst *aggConst = FEManager::GetModule().GetMemPool()->New<MIRAggConst>(FEManager::GetModule(), *initListType);
#ifndef USE_OPS
  CHECK_FATAL(false, "Not support");
#else
  if (HasArrayFiller()) {
    for (size_t i = 0; i < fillers.size(); ++i) {
      if (fillers[i]->GetASTOp() == kASTImplicitValueInitExpr) {
        continue;
      }
      if (fillers[i]->GetASTOp() == kASTOpInitListExpr) {
        for (auto subFiller : static_cast<ASTInitListExpr*>(fillers[i])->fillers) {
          aggConst->AddItem(subFiller->GenerateMIRConst(), 0);
        }
      } else {
        aggConst->AddItem(fillers[i]->GenerateMIRConst(), 0);
      }
    }
  } else {
    for (size_t i = 0; i < fillers.size(); ++i) {
      if (fillers[i]->GetASTOp() == kASTImplicitValueInitExpr) {
        continue;
      }
      aggConst->AddItem(fillers[i]->GenerateMIRConst(), 0);
    }
  }
#endif
  return aggConst;
}

MIRConst *ASTInitListExpr::GenerateMIRConstForStruct() const {
  if (fillers.empty()) {
    return nullptr; // No var constant generation
  }
  bool hasFiller = false;
  for (auto e : fillers) {
    if (e != nullptr) {
      hasFiller = true;
      break;
    }
  }
  if (!hasFiller) {
    return nullptr;
  }
  MIRAggConst *aggConst = FEManager::GetModule().GetMemPool()->New<MIRAggConst>(FEManager::GetModule(), *initListType);
#ifndef USE_OPS
  CHECK_FATAL(false, "Not support");
#else
  for (size_t i = 0; i < fillers.size(); ++i) {
    if (fillers[i] == nullptr || fillers[i]->GetASTOp() == kASTImplicitValueInitExpr) {
      continue;
    }
    aggConst->AddItem(fillers[i]->GenerateMIRConst(), i + 1);
  }
#endif
  return aggConst;
}

UniqueFEIRExpr ASTInitListExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  if (!initListType->IsStructType()) {
    Emit2FEExprForArray(stmts);
  } else {
    Emit2FEExprForStruct(stmts);
  }
  return nullptr;
}

void ASTInitListExpr::Emit2FEExprForArrayForNest(UniqueFEIRType typeNative, UniqueFEIRExpr arrayExpr,
                                                 std::list<UniqueFEIRStmt> &stmts) const {
  for (int i = 0; i < fillers.size(); ++i) {
    if (static_cast<ASTInitListExpr*>(fillers[i])->initListType->IsStructType()) { // array elem is struct
      Emit2FEExprForStruct(stmts);
      return;
    }
    MIRType *mirType = static_cast<ASTInitListExpr*>(fillers[i])->initListType;
    std::string tmpName = FEUtils::GetSequentialName("subArray_");
    UniqueFEIRVar tmpVar = FEIRBuilder::CreateVarNameForC(tmpName, *mirType);
    static_cast<ASTInitListExpr*>(fillers[i])->SetInitListVarName(tmpName);
    (void)(fillers[i])->Emit2FEExpr(stmts);
    UniqueFEIRType elemType = tmpVar->GetType()->Clone();
    UniqueFEIRExpr dreadAgg = FEIRBuilder::CreateExprDRead(std::move(tmpVar));
    UniqueFEIRExpr arrayExprTmp = arrayExpr->Clone();
    UniqueFEIRExpr exprIndex = FEIRBuilder::CreateExprConstI32(i);
    UniqueFEIRType typeNativeTmp = typeNative->Clone();
    auto fieldStmt = FEIRBuilder::CreateStmtArrayStoreOneStmtForC(std::move(dreadAgg), std::move(arrayExprTmp),
                                                                  std::move(exprIndex), std::move(typeNativeTmp),
                                                                  std::move(elemType),
                                                                  varName);
    stmts.emplace_back(std::move(fieldStmt));
  }
}

void ASTInitListExpr::Emit2FEExprForArray(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRVar feirVar = FEIRBuilder::CreateVarNameForC(varName, *initListType);
  UniqueFEIRVar feirVarTmp = feirVar->Clone();
  UniqueFEIRType typeNative = FEIRTypeHelper::CreateTypeNative(*initListType);
  UniqueFEIRExpr arrayExpr = FEIRBuilder::CreateExprAddrofVar(std::move(feirVarTmp));
  if (fillers[0]->GetASTOp() == kASTOpInitListExpr) {
    Emit2FEExprForArrayForNest(typeNative->Clone(), arrayExpr->Clone(), stmts);
  } else {
    for (int i = 0; i < fillers.size(); ++i) {
      UniqueFEIRExpr exprIndex = FEIRBuilder::CreateExprConstI32(i);
      UniqueFEIRExpr exprElem = fillers[i]->Emit2FEExpr(stmts);
      UniqueFEIRType typeNativeTmp = typeNative->Clone();
      UniqueFEIRExpr arrayExprTmp = arrayExpr->Clone();
      auto stmt = FEIRBuilder::CreateStmtArrayStoreOneStmtForC(std::move(exprElem), std::move(arrayExprTmp),
                                                               std::move(exprIndex), std::move(typeNativeTmp),
                                                               varName);
      stmts.emplace_back(std::move(stmt));
    }

    MIRType *ptrMIRArrayType = typeNative->GenerateMIRTypeAuto();
    auto allSize = static_cast<MIRArrayType*>(ptrMIRArrayType)->GetSize();
    auto elemSize = static_cast<MIRArrayType*>(ptrMIRArrayType)->GetElemType()->GetSize();
    CHECK_FATAL(elemSize != 0, "elemSize should not 0");
    auto allElemCnt = allSize / elemSize;
    uint32 needInitFurtherCnt = allElemCnt - fillers.size();
    PrimType elemPrimType = static_cast<MIRArrayType*>(ptrMIRArrayType)->GetElemType()->GetPrimType();
    for (int i = 0; i < needInitFurtherCnt; ++i) {
      UniqueFEIRExpr exprIndex = FEIRBuilder::CreateExprConstI32(fillers.size() + i);
      UniqueFEIRType typeNativeTmp = typeNative->Clone();
      UniqueFEIRExpr arrayExprTmp = arrayExpr->Clone();

      UniqueFEIRExpr exprElemOther = std::make_unique<FEIRExprConst>(static_cast<int64>(0), elemPrimType);
      auto stmt = FEIRBuilder::CreateStmtArrayStoreOneStmtForC(std::move(exprElemOther), std::move(arrayExprTmp),
                                                               std::move(exprIndex),
                                                               std::move(typeNativeTmp),
                                                               varName);
      stmts.emplace_back(std::move(stmt));
    }
  }
}

void ASTInitListExpr::Emit2FEExprForStruct4ArrayElemIsStruct(uint32 i, std::list<UniqueFEIRStmt> &stmts) const {
  MIRType *ptrMIRElemType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*initListType, PTY_ptr);
  UniqueFEIRVar tmpVar = FEIRBuilder::CreateVarNameForC(varName, *ptrMIRElemType);
  UniqueFEIRExpr dreadAddr = FEIRBuilder::CreateExprDRead(std::move(tmpVar));
  MIRArrayType* arrayType = static_cast<MIRArrayType*>(initListType);
  UniqueFEIRType typeArray = std::make_unique<FEIRTypeNative>(*static_cast<MIRArrayType*>(initListType));
  UniqueFEIRType uTypeArray = typeArray->Clone();
  std::list<UniqueFEIRExpr> indexsExprs;
  UniqueFEIRExpr indexExpr = FEIRBuilder::CreateExprConstAnyScalar(PTY_i32, i);
  indexsExprs.push_back(std::move(indexExpr));
  UniqueFEIRExpr srcExpr = FEIRBuilder::CreateExprAddrofArray(std::move(uTypeArray), std::move(dreadAddr),
                                                              varName, indexsExprs);
  MIRType *ptrElemType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*arrayType->GetElemType(), PTY_ptr);
  auto tmpArrayName = FEUtils::GetSequentialName("struct_tmpvar_");
  UniqueFEIRVar tmpArrayVar = FEIRBuilder::CreateVarNameForC(tmpArrayName, *ptrElemType);
  UniqueFEIRStmt stmtDAssign = FEIRBuilder::CreateStmtDAssign(std::move(tmpArrayVar), std::move(srcExpr));
  stmts.emplace_back(std::move(stmtDAssign));
  static_cast<ASTInitListExpr*>(fillers[i])->SetInitListVarName(tmpArrayName);
  static_cast<ASTInitListExpr*>(fillers[i])->SetParentFlag(kArrayParent);
  (void)(fillers[i])->Emit2FEExpr(stmts);
}

void ASTInitListExpr::Emit2FEExprForStruct(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRVar feirVar = FEIRBuilder::CreateVarNameForC(varName, *initListType);
  if (IsUnionInitListExpr()) {
#ifndef USE_OPS
    CHECK_FATAL(false, "Unsupported INTRN_C_memset");
#else
    std::unique_ptr<std::list<UniqueFEIRExpr>> argExprList = std::make_unique<std::list<UniqueFEIRExpr>>();
    UniqueFEIRExpr addrOfExpr = FEIRBuilder::CreateExprAddrofVar(feirVar->Clone());
    argExprList->emplace_back(std::move(addrOfExpr));
    argExprList->emplace_back(FEIRBuilder::CreateExprConstU32(0));
    argExprList->emplace_back(FEIRBuilder::CreateExprSizeOfType(std::make_unique<FEIRTypeNative>(*initListType)));
    std::unique_ptr<FEIRStmtIntrinsicCallAssign> stmt = std::make_unique<FEIRStmtIntrinsicCallAssign>(
        INTRN_C_memset, nullptr, nullptr, std::move(argExprList));
    stmts.emplace_back(std::move(stmt));
#endif
  }
  MIRStructType *structType = static_cast<MIRStructType*>(initListType);
  for (uint32 i = 0; i < fillers.size(); i++) {
    if (fillers[i] == nullptr) {
      continue; // skip anonymous field
    }
    uint32 fieldID = 0;
    if (fillers[i]->GetASTOp() == kASTOpInitListExpr) {
      if (initListType->GetKind() == kTypeStruct) {
        MIRType *mirType = static_cast<ASTInitListExpr*>(fillers[i])->GetInitListType();
        std::string tmpName = FEUtils::GetSequentialName("subInitListVar_");
        UniqueFEIRVar tmpVar = FEIRBuilder::CreateVarNameForC(tmpName, *mirType);
        static_cast<ASTInitListExpr*>(fillers[i])->SetInitListVarName(tmpName);
        static_cast<ASTInitListExpr*>(fillers[i])->SetParentFlag(kStructParent);
        (void)(fillers[i])->Emit2FEExpr(stmts);
        UniqueFEIRExpr dreadAgg = FEIRBuilder::CreateExprDRead(std::move(tmpVar));
        FEManager::GetMIRBuilder().TraverseToNamedField(*structType, structType->GetElemStrIdx(i), fieldID);
        UniqueFEIRStmt fieldStmt = std::make_unique<FEIRStmtDAssign>(feirVar->Clone(), std::move(dreadAgg), fieldID);
        stmts.emplace_back(std::move(fieldStmt));
      } else if (initListType->GetKind() == kTypeArray) { // array elem is struct
        Emit2FEExprForStruct4ArrayElemIsStruct(i, stmts);
      }
    } else if (fillers[i]->GetASTOp() == kASTASTDesignatedInitUpdateExpr) {
      MIRType *mirType = static_cast<ASTDesignatedInitUpdateExpr*>(fillers[i])->GetInitListType();
      std::string tmpName = FEUtils::GetSequentialName("subVarToBeUpdate_");
      UniqueFEIRVar tmpVar = FEIRBuilder::CreateVarNameForC(tmpName, *mirType);
      static_cast<ASTDesignatedInitUpdateExpr*>(fillers[i])->SetInitListVarName(tmpName);
      (void)(fillers[i])->Emit2FEExpr(stmts);
      UniqueFEIRExpr dreadAgg = FEIRBuilder::CreateExprDRead(std::move(tmpVar));
      FEManager::GetMIRBuilder().TraverseToNamedField(*structType, structType->GetElemStrIdx(i), fieldID);
      UniqueFEIRStmt fieldStmt = std::make_unique<FEIRStmtDAssign>(feirVar->Clone(), std::move(dreadAgg), fieldID);
      stmts.emplace_back(std::move(fieldStmt));
    } else {
      if (parentFlag == kStructParent || (parentFlag == kNoParent)) {
        FEManager::GetMIRBuilder().TraverseToNamedField(*structType, structType->GetElemStrIdx(i), fieldID);
        UniqueFEIRExpr fieldFEExpr = fillers[i]->Emit2FEExpr(stmts);
        UniqueFEIRStmt fieldStmt = std::make_unique<FEIRStmtDAssign>(feirVar->Clone(), fieldFEExpr->Clone(), fieldID);
        stmts.emplace_back(std::move(fieldStmt));
      } else if (parentFlag == kArrayParent) { // array elem is struct / no nest case
        UniqueFEIRExpr exprIndex = FEIRBuilder::CreateExprConstI32(i);
        UniqueFEIRExpr fieldFEExpr = fillers[i]->Emit2FEExpr(stmts);
        MIRType *ptrBaseType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*initListType, PTY_ptr);
        UniqueFEIRVar tmpVar = FEIRBuilder::CreateVarNameForC(varName, *ptrBaseType);
        UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtFieldStoreForC(std::move(tmpVar), std::move(fieldFEExpr),
                                                                    structType, i + 1);
        stmts.emplace_back(std::move(stmt));
      }
    }
  }
}

void ASTInitListExpr::SetFillerExprs(ASTExpr *astExpr) {
  fillers.emplace_back(astExpr);
}

void ASTInitListExpr::SetInitListType(MIRType *type) {
  initListType = type;
}

// ---------- ASTImplicitValueInitExpr ----------
MIRConst *ASTImplicitValueInitExpr::GenerateMIRConstImpl() const {
  CHECK_FATAL(false, "Should not generate const val for ImplicitValueInitExpr");
  return nullptr;
}

UniqueFEIRExpr ASTImplicitValueInitExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  return ImplicitInitFieldValue(type, stmts);
}

MIRConst *ASTStringLiteral::GenerateMIRConstImpl() const {
  MIRType *elemType = GlobalTables::GetTypeTable().GetPrimType(PTY_i8);
  MIRType *arrayTypeWithSize = GlobalTables::GetTypeTable().GetOrCreateArrayType(*elemType, codeUnits.size());
  MIRAggConst *val = FEManager::GetModule().GetMemPool()->New<MIRAggConst>(FEManager::GetModule(), *arrayTypeWithSize);
  for (uint32 i = 0; i < codeUnits.size(); ++i) {
    MIRConst *cst = FEManager::GetModule().GetMemPool()->New<MIRIntConst>(codeUnits[i], *elemType);
    val->PushBack(cst);
  }
  return val;
}

UniqueFEIRExpr ASTStringLiteral::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRExpr expr = std::make_unique<FEIRExprAddrof>(codeUnits);
  CHECK_NULL_FATAL(expr);
  return expr;
}

// ---------- ASTArraySubscriptExpr ----------
int32 ASTArraySubscriptExpr::TranslateArraySubscript2Offset() const {
  int32 offset = 0;
  for (std::size_t i = 0; i < idxExprs.size(); ++i) {
    CHECK_FATAL(idxExprs[i]->GetConstantValue() != nullptr, "Not constant value for constant initializer");
    offset += baseExprTypes[i]->GetSize() * idxExprs[i]->GetConstantValue()->val.i64;
  }
  return offset;
}

UniqueFEIRExpr ASTArraySubscriptExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  std::list<UniqueFEIRExpr> indexExprs;
  UniqueFEIRExpr arrayStoreForCExpr;
  UniqueFEIRType typeNative = FEIRTypeHelper::CreateTypeNative(*baseExpr->GetType());
  UniqueFEIRExpr baseFEExpr;
  if (baseExpr->GetASTOp() == kASTStringLiteral) {
    UniqueFEIRVar unamedVar = FEIRBuilder::CreateVarNameForC(FEUtils::GetSequentialName("unamedstr_"),
                                                             *baseExpr->GetType(), true, false);
    baseFEExpr = FEIRBuilder::CreateExprAddrofVar(std::move(unamedVar));
    FEIRExprAddrofVar *addrofVarExpr = static_cast<FEIRExprAddrofVar*>(baseFEExpr.get());
    addrofVarExpr->SetVarValue(baseExpr->GenerateMIRConst());
  } else {
    baseFEExpr = baseExpr->Emit2FEExpr(stmts);
  }
  for (auto expr : idxExprs) {
    UniqueFEIRExpr feirExpr = expr->Emit2FEExpr(stmts);
    indexExprs.emplace_front(std::move(feirExpr));
  }
  if (memberExpr != nullptr) {
    auto memExpr = static_cast<ASTMemberExpr*>(memberExpr)->Emit2FEExpr(stmts);
    auto memType = FEIRTypeHelper::CreateTypeNative(*static_cast<ASTMemberExpr*>(memberExpr)->GetMemberType());
    CHECK_FATAL(memType, "memType should not be nullptr");
    arrayStoreForCExpr = FEIRBuilder::CreateExprArrayStoreForC(std::move(memExpr), indexExprs, std::move(memType),
                                                               std::move(baseFEExpr), std::move(typeNative),
                                                               baseExprVarName);
  } else {
    if (baseFEExpr->GetKind() == kExprCStyleCast) {
      FEIRExprCStyleCast *expr = static_cast<FEIRExprCStyleCast*>(baseFEExpr.get());
      baseFEExpr = expr->GetSubExpr()->Clone();
      typeNative = FEIRTypeHelper::CreateTypeNative(*expr->GetMIRType());
    }
    arrayStoreForCExpr = FEIRBuilder::CreateExprArrayStoreForC(std::move(baseFEExpr), indexExprs,
                                                               std::move(typeNative), baseExprVarName);
  }
  static_cast<FEIRExprArrayStoreForC*>(arrayStoreForCExpr.get())->SetAddrOfFlag(isAddrOf);
  return arrayStoreForCExpr;
}

UniqueFEIRExpr ASTExprUnaryExprOrTypeTraitExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

ASTMemberExpr *ASTMemberExpr::findFinalMember(ASTMemberExpr *startExpr, std::list<std::string> &memberNames) const {
  memberNames.emplace_back(startExpr->memberName);
  if (startExpr->isArrow || startExpr->baseExpr->GetASTOp() != kASTMemberExpr) {
    return startExpr;
  }
  return findFinalMember(static_cast<ASTMemberExpr*>(startExpr->baseExpr), memberNames);
}

void ASTMemberExpr::Emit2FEExprImplForArrayElemIsStruct(UniqueFEIRExpr baseFEExpr, std::string &tmpStructName,
                                                        std::list<UniqueFEIRStmt> &stmts) const {
  if (baseExpr->GetASTOp() == kASTSubscriptExpr) {
    auto arrayStoreForC = static_cast<FEIRExprArrayStoreForC*>(baseFEExpr.get());
    FEIRExpr &exprArray = arrayStoreForC->GetExprArray();
    std::list<UniqueFEIRExpr> &indexsExpr = arrayStoreForC->GetExprIndexs();
    UniqueFEIRType typeArray = arrayStoreForC->GetTypeArray().Clone();
    UniqueFEIRExpr uExprArray = exprArray.Clone();
    const std::string &arrayName = static_cast<ASTArraySubscriptExpr*>(baseExpr)->GetBaseExprVarName();
    UniqueFEIRExpr srcExpr = FEIRBuilder::CreateExprAddrofArray(std::move(typeArray), std::move(baseFEExpr),
                                                                arrayName, indexsExpr);
    MIRType *ptrBaseType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*baseType, PTY_ptr);
    tmpStructName = FEUtils::GetSequentialName("struct_tmpvar_");
    UniqueFEIRVar tmpVar = FEIRBuilder::CreateVarNameForC(tmpStructName, *ptrBaseType);
    auto uTmpVar = tmpVar->Clone();
    UniqueFEIRStmt readStmt1 = FEIRBuilder::CreateStmtDAssign(uTmpVar->Clone(), std::move(srcExpr));
    stmts.emplace_back(std::move(readStmt1));
  }
}

UniqueFEIRExpr ASTMemberExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRExpr baseFEExpr;
  std::string fieldName = memberName;
  bool isArrow = this->isArrow;
  MIRType *baseType = this->baseType;
  std::string tmpStructName;
  if (baseExpr->GetASTOp() == kASTMemberExpr) {
    std::list<std::string> memberNameList;
    memberNameList.emplace_back(memberName);
    ASTMemberExpr *finalMember = findFinalMember(static_cast<ASTMemberExpr*>(baseExpr), memberNameList);
    baseFEExpr = finalMember->baseExpr->Emit2FEExpr(stmts);
    isArrow = finalMember->isArrow;
    baseType = finalMember->baseType;
    fieldName = ASTUtil::Join(memberNameList, ".");
  } else {
    baseFEExpr = baseExpr->Emit2FEExpr(stmts);
    Emit2FEExprImplForArrayElemIsStruct(baseFEExpr->Clone(), tmpStructName, stmts);
  }
  UniqueFEIRType baseFEType = std::make_unique<FEIRTypeNative>(*baseType);
  if (isArrow) {
    CHECK_FATAL(baseType->IsMIRPtrType(), "Must be ptr type!");
    MIRPtrType *mirPtrType = static_cast<MIRPtrType*>(baseType);
    MIRType *pointedMirType = mirPtrType->GetPointedType();
    CHECK_FATAL(pointedMirType->IsStructType(), "pointedMirType must be StructType");
    MIRStructType *structType = static_cast<MIRStructType*>(pointedMirType);
    FieldID fieldID = FEUtils::GetStructFieldID(structType, fieldName);
    MIRType *reType = FEUtils::GetStructFieldType(structType, fieldID);
    UniqueFEIRType retFEType = std::make_unique<FEIRTypeNative>(*reType);
    if (retFEType->IsArray()) {
      return std::make_unique<FEIRExprIAddrof>(std::move(baseFEType), fieldID, std::move(baseFEExpr));
    } else {
      return FEIRBuilder::CreateExprIRead(std::move(retFEType), std::move(baseFEType), std::move(baseFEExpr), fieldID);
    }
  } else {
    if (baseExpr->GetASTOp() != kASTSubscriptExpr) {
      CHECK_FATAL(baseType->IsStructType(), "basetype must be StructType");
      UniqueFEIRVar tmpVar = static_cast<FEIRExprDRead*>(baseFEExpr.get())->GetVar()->Clone();
      MIRStructType *structType = static_cast<MIRStructType*>(baseType);
      FieldID fieldID = FEUtils::GetStructFieldID(structType, fieldName);
      UniqueFEIRType memberFEType = std::make_unique<FEIRTypeNative>(*memberType);
      if (memberFEType->IsArray()) {
        auto addrofExpr = std::make_unique<FEIRExprAddrofVar>(std::move(tmpVar));
        addrofExpr->SetFieldID(fieldID);
        return addrofExpr;
      } else {
        return FEIRBuilder::CreateExprDReadAggField(std::move(tmpVar), fieldID, std::move(memberFEType));
      }
    } else { // GetASTOp() is kASTSubscriptExpr
      uint32 fieldID = 0;
      GStrIdx fieldNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(memberName);
      auto ptrMIRStructType = std::make_unique<FEIRTypeNative>(*baseType);
      MIRStructType* mirStructType = static_cast<MIRStructType*>(ptrMIRStructType->GenerateMIRType());
      FEManager::GetMIRBuilder().TraverseToNamedField(*mirStructType, fieldNameIdx, fieldID);
      auto fieldVar = FEIRBuilder::CreateVarNameForC(memberName, *memberType);
      MIRType *ptrBaseType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*baseType, PTY_ptr);
      UniqueFEIRVar tmpVar = FEIRBuilder::CreateVarNameForC(tmpStructName, *ptrBaseType);
      return FEIRBuilder::CreateExprFieldLoadForC(std::move(tmpVar), std::move(fieldVar),
                                                  mirStructType, fieldID);
    }
  }
  return nullptr;
}

// ---------- ASTDesignatedInitUpdateExpr ----------
MIRConst *ASTDesignatedInitUpdateExpr::GenerateMIRConstImpl() const {
  return FEManager::GetModule().GetMemPool()->New<MIRAggConst>(FEManager::GetModule(), *initListType);
}

UniqueFEIRExpr ASTDesignatedInitUpdateExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRVar feirVar = FEIRBuilder::CreateVarNameForC(initListVarName, *initListType);
  UniqueFEIRExpr baseFEIRExpr = baseExpr->Emit2FEExpr(stmts);
  UniqueFEIRStmt baseFEIRStmt = std::make_unique<FEIRStmtDAssign>(std::move(feirVar), std::move(baseFEIRExpr), 0);
  stmts.emplace_back(std::move(baseFEIRStmt));
  static_cast<ASTInitListExpr*>(updaterExpr)->SetInitListVarName(initListVarName);
  updaterExpr->Emit2FEExpr(stmts);
  return nullptr;
}

UniqueFEIRExpr ASTBinaryOperatorExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  if (complexElementType != nullptr) {
    UniqueFEIRVar tempVar = FEIRBuilder::CreateVarNameForC(FEUtils::GetSequentialName("Complex_"), *retType);
    if (opcode == OP_add || opcode == OP_sub) {
      auto complexElementFEType = std::make_unique<FEIRTypeNative>(*complexElementType);
      UniqueFEIRExpr realFEExpr = FEIRBuilder::CreateExprBinary(complexElementFEType->Clone(), opcode,
                                                                leftRealExpr->Emit2FEExpr(stmts),
                                                                rightRealExpr->Emit2FEExpr(stmts));
      UniqueFEIRExpr imagFEExpr = FEIRBuilder::CreateExprBinary(complexElementFEType->Clone(), opcode,
                                                                leftImagExpr->Emit2FEExpr(stmts),
                                                                rightImagExpr->Emit2FEExpr(stmts));
      auto realStmt = FEIRBuilder::CreateStmtDAssignAggField(tempVar->Clone(), std::move(realFEExpr), kComplexRealID);
      auto imagStmt = FEIRBuilder::CreateStmtDAssignAggField(tempVar->Clone(), std::move(imagFEExpr), kComplexImagID);
      stmts.emplace_back(std::move(realStmt));
      stmts.emplace_back(std::move(imagStmt));
      auto dread = FEIRBuilder::CreateExprDRead(std::move(tempVar));
      static_cast<FEIRExprDRead*>(dread.get())->SetFieldType(std::move(complexElementFEType));
      return dread;
    } else if (opcode == OP_eq || opcode == OP_ne) {
      auto boolFEType = std::make_unique<FEIRTypeNative>(*GlobalTables::GetTypeTable().GetPrimType(PTY_i32));
      UniqueFEIRExpr realFEExpr = FEIRBuilder::CreateExprBinary(boolFEType->Clone(), opcode,
                                                                leftRealExpr->Emit2FEExpr(stmts),
                                                                rightRealExpr->Emit2FEExpr(stmts));
      UniqueFEIRExpr imagFEExpr = FEIRBuilder::CreateExprBinary(boolFEType->Clone(), opcode,
                                                                leftImagExpr->Emit2FEExpr(stmts),
                                                                rightImagExpr->Emit2FEExpr(stmts));
      UniqueFEIRExpr finalExpr;
      if (opcode == OP_eq) {
        finalExpr = FEIRBuilder::CreateExprBinary(boolFEType->Clone(), OP_land, std::move(realFEExpr),
                                                  std::move(imagFEExpr));
      } else {
        finalExpr = FEIRBuilder::CreateExprBinary(boolFEType->Clone(), OP_lior, std::move(realFEExpr),
                                                  std::move(imagFEExpr));
      }
      return finalExpr;
    } else {
      CHECK_FATAL(false, "NIY");
    }
  } else {
    if (opcode == OP_lior || opcode == OP_land) {
      Opcode op = opcode == OP_lior ? OP_brtrue : OP_brfalse;
      MIRType *tempVarType = GlobalTables::GetTypeTable().GetInt32();
      UniqueFEIRType tempFeirType = std::make_unique<FEIRTypeNative>(*tempVarType);
      UniqueFEIRVar shortCircuit = FEIRBuilder::CreateVarNameForC(varName, *tempVarType);
      std::string labelName = FEUtils::GetSequentialName("shortCircuit_label_");

      auto leftFEExpr = leftExpr->Emit2FEExpr(stmts);
      auto leftStmt = std::make_unique<FEIRStmtDAssign>(shortCircuit->Clone(), leftFEExpr->Clone(), 0);
      stmts.emplace_back(std::move(leftStmt));

      auto dreadExpr = FEIRBuilder::CreateExprDRead(shortCircuit->Clone());
      UniqueFEIRStmt condGoToExpr = std::make_unique<FEIRStmtCondGotoForC>(dreadExpr->Clone(), op, labelName);
      stmts.emplace_back(std::move(condGoToExpr));

      auto rightFEExpr = rightExpr->Emit2FEExpr(stmts);
      auto rightStmt = std::make_unique<FEIRStmtDAssign>(shortCircuit->Clone(), rightFEExpr->Clone(), 0);
      stmts.emplace_back(std::move(rightStmt));

      auto labelStmt = std::make_unique<FEIRStmtLabel>(labelName);
      labelStmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
      stmts.emplace_back(std::move(labelStmt));

      UniqueFEIRExpr zeroConstExpr = FEIRBuilder::CreateExprConstAnyScalar(PTY_i32, 0);
      return FEIRBuilder::CreateExprBinary(std::move(tempFeirType), OP_ne,
          dreadExpr->Clone(), std::move(zeroConstExpr));
    } else {
      auto leftFEExpr = leftExpr->Emit2FEExpr(stmts);
      auto rightFEExpr = rightExpr->Emit2FEExpr(stmts);
      UniqueFEIRType feirType = std::make_unique<FEIRTypeNative>(*retType);
      if (cvtNeeded) {
        if (leftFEExpr->GetPrimType() != feirType->GetPrimType()) {
          leftFEExpr = FEIRBuilder::CreateExprCvtPrim(std::move(leftFEExpr), feirType->GetPrimType());
        }
        if (rightFEExpr->GetPrimType() != feirType->GetPrimType()) {
          rightFEExpr = FEIRBuilder::CreateExprCvtPrim(std::move(rightFEExpr), feirType->GetPrimType());
        }
      }
      return FEIRBuilder::CreateExprBinary(std::move(feirType), opcode, std::move(leftFEExpr), std::move(rightFEExpr));
    }
  }
}

void ASTAssignExpr::ProcessAssign4ExprArrayStoreForC(std::list<UniqueFEIRStmt> &stmts, UniqueFEIRExpr leftFEExpr,
                                                     UniqueFEIRExpr rightFEExpr) const {
  auto arrayStoreForC = static_cast<FEIRExprArrayStoreForC*>(leftFEExpr.get());
  FEIRExpr &exprArray = arrayStoreForC->GetExprArray();
  std::list<UniqueFEIRExpr> &indexsExpr = arrayStoreForC->GetExprIndexs();
  FEIRType &typeArray = arrayStoreForC->GetTypeArray();
  UniqueFEIRExpr uExprArray = exprArray.Clone();
  UniqueFEIRType uTypeArray = typeArray.Clone();
  auto arrayName = arrayStoreForC->GetArrayName();
  if (arrayStoreForC->IsMember()) {
    FEIRExpr &exprStruct = arrayStoreForC->GetExprStruct();
    FEIRType &typeStruct = arrayStoreForC->GetTypeSruct();
    UniqueFEIRExpr uExprStruct = exprStruct.Clone();
    UniqueFEIRType uTypeStruct = typeStruct.Clone();
    UniqueFEIRStmt stmt = std::make_unique<FEIRStmtArrayStore>(std::move(rightFEExpr),
                                                               std::move(uExprArray),
                                                               indexsExpr,
                                                               std::move(uTypeArray),
                                                               std::move(uExprStruct),
                                                               std::move(uTypeStruct),
                                                               arrayName);
    stmts.emplace_back(std::move(stmt));
  } else {
    UniqueFEIRStmt stmt = std::make_unique<FEIRStmtArrayStore>(std::move(rightFEExpr),
                                                               std::move(uExprArray),
                                                               indexsExpr,
                                                               std::move(uTypeArray),
                                                               arrayName);
    stmts.emplace_back(std::move(stmt));
  }
}

UniqueFEIRExpr ASTAssignExpr::ProcessAssign(std::list<UniqueFEIRStmt> &stmts, UniqueFEIRExpr leftFEExpr,
                                            UniqueFEIRExpr rightFEExpr) const {
  // C89 does not support lvalue casting, but cxx support, needs to improve here
  if (leftFEExpr->GetKind() == FEIRNodeKind::kExprDRead && !leftFEExpr->GetType()->IsArray()) {
    auto dreadFEExpr = static_cast<FEIRExprDRead*>(leftFEExpr.get());
    FieldID fieldID = dreadFEExpr->GetFieldID();
    UniqueFEIRVar var = dreadFEExpr->GetVar()->Clone();
    auto preStmt = std::make_unique<FEIRStmtDAssign>(std::move(var), std::move(rightFEExpr), fieldID);
    stmts.emplace_back(std::move(preStmt));
    return leftFEExpr;
  } else if (leftFEExpr->GetKind() == FEIRNodeKind::kExprIRead) {
    auto ireadFEExpr = static_cast<FEIRExprIRead*>(leftFEExpr.get());
    FieldID fieldID = ireadFEExpr->GetFieldID();
    auto preStmt = std::make_unique<FEIRStmtIAssign>(ireadFEExpr->GetClonedPtrType(), ireadFEExpr->GetClonedOpnd(),
                                                     std::move(rightFEExpr), fieldID);
    stmts.emplace_back(std::move(preStmt));
    return leftFEExpr;
  } else if (leftFEExpr->GetKind() == FEIRNodeKind::kExprArrayStoreForC) {
    ProcessAssign4ExprArrayStoreForC(stmts, leftFEExpr->Clone(), rightFEExpr->Clone());
    return leftFEExpr;
  } else if (leftFEExpr->GetKind() == FEIRNodeKind::kExprFieldLoadForC) {
    auto exprFieldLoadForC = static_cast<FEIRExprFieldLoadForC*>(leftFEExpr.get());
    UniqueFEIRVar varStruct = exprFieldLoadForC->GetStructVar().Clone();
    MIRStructType *structType = exprFieldLoadForC->GetMIRStructType();
    uint32 fieldID = exprFieldLoadForC->GetFieldID();
    UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtFieldStoreForC(std::move(varStruct), std::move(rightFEExpr),
                                                                structType, fieldID);
    stmts.emplace_back(std::move(stmt));
    return leftFEExpr;
  }
  return nullptr;
}

UniqueFEIRExpr ASTAssignExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRExpr leftFEExpr;
  if (isCompoundAssign) {
    std::list<UniqueFEIRStmt> dummyStmts;
    leftFEExpr = leftExpr->Emit2FEExpr(dummyStmts);
  } else {
    leftFEExpr = leftExpr->Emit2FEExpr(stmts);
  }
  UniqueFEIRExpr rightFEExpr = rightExpr->Emit2FEExpr(stmts);
  return ProcessAssign(stmts, std::move(leftFEExpr), std::move(rightFEExpr));
}

UniqueFEIRExpr ASTBOComma::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  auto leftFEExpr = leftExpr->Emit2FEExpr(stmts);
  std::list<UniqueFEIRExpr> exprs;
  exprs.emplace_back(std::move(leftFEExpr));
  auto leftStmt = std::make_unique<FEIRStmtNary>(OP_eval, std::move(exprs));
  stmts.emplace_back(std::move(leftStmt));
  return rightExpr->Emit2FEExpr(stmts);
}

UniqueFEIRExpr ASTBOPtrMemExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

// ---------- ASTParenExpr ----------
UniqueFEIRExpr ASTParenExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = child;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr childFEIRExpr = childExpr->Emit2FEExpr(stmts);
  return childFEIRExpr;
}

// ---------- ASTIntegerLiteral ----------
UniqueFEIRExpr ASTIntegerLiteral::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRExpr constExpr = std::make_unique<FEIRExprConst>(val, type);
  return constExpr;
}

// ---------- ASTFloatingLiteral ----------
UniqueFEIRExpr ASTFloatingLiteral::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRExpr expr;
  if (kind == F32) {
    expr = FEIRBuilder::CreateExprConstF32(static_cast<float>(val));
  } else {
    expr = FEIRBuilder::CreateExprConstF64(val);
  }
  CHECK_NULL_FATAL(expr);
  return expr;
}

// ---------- ASTCharacterLiteral ----------
UniqueFEIRExpr ASTCharacterLiteral::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRExpr constExpr = FEIRBuilder::CreateExprConstAnyScalar(type, val);
  return constExpr;
}

// ---------- ASTConditionalOperator ----------
UniqueFEIRExpr ASTConditionalOperator::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRExpr condFEIRExpr = condExpr->Emit2FEExpr(stmts);
  // a noncomparative conditional expr need to be converted a comparative conditional expr
  if (!(condFEIRExpr->GetKind() == kExprBinary && static_cast<FEIRExprBinary*>(condFEIRExpr.get())->IsComparative())) {
    UniqueFEIRExpr zeroConstExpr = (condFEIRExpr->GetPrimType() == PTY_ptr) ?
        FEIRBuilder::CreateExprConstPtrNull() :
        FEIRBuilder::CreateExprConstAnyScalar(condFEIRExpr->GetPrimType(), 0);
    condFEIRExpr = FEIRBuilder::CreateExprMathBinary(OP_ne, std::move(condFEIRExpr), std::move(zeroConstExpr));
  }
  std::list<UniqueFEIRStmt> trueStmts;
  UniqueFEIRExpr trueFEIRExpr = trueExpr->Emit2FEExpr(trueStmts);
  std::list<UniqueFEIRStmt> falseStmts;
  UniqueFEIRExpr falseFEIRExpr = falseExpr->Emit2FEExpr(falseStmts);
  // There are no extra nested statements in the expressions, (e.g., a < 1 ? 1 : 2), use ternary FEIRExpr
  if (trueStmts.empty() && falseStmts.empty()) {
    CHECK_NULL_FATAL(mirType);
    UniqueFEIRType type = std::make_unique<FEIRTypeNative>(*mirType);
    return FEIRBuilder::CreateExprTernary(OP_select, std::move(type), std::move(condFEIRExpr),
                                          std::move(trueFEIRExpr), std::move(falseFEIRExpr));
  }
  if (trueFEIRExpr == nullptr || falseFEIRExpr == nullptr) {
    UniqueFEIRStmt stmtIf = FEIRBuilder::CreateStmtIf(std::move(condFEIRExpr), trueStmts, falseStmts);
    stmts.emplace_back(std::move(stmtIf));
    return nullptr;
  }
  // Otherwise, (e.g., a < 1 ? 1 : a++) create a temporary var to hold the return trueExpr or falseExpr value
  CHECK_FATAL(trueFEIRExpr->GetPrimType() == falseFEIRExpr->GetPrimType(),
              "The types of trueFEIRExpr and falseFEIRExpr are inconsistent");
  MIRType *retType = trueFEIRExpr->GetType()->GenerateMIRTypeAuto();
  UniqueFEIRVar tempVar = FEIRBuilder::CreateVarNameForC(FEUtils::GetSequentialName("levVar_"), *retType);
  UniqueFEIRVar tempVarCloned1 = tempVar->Clone();
  UniqueFEIRVar tempVarCloned2 = tempVar->Clone();
  UniqueFEIRStmt retTrueStmt = FEIRBuilder::CreateStmtDAssign(std::move(tempVar), std::move(trueFEIRExpr));
  trueStmts.emplace_back(std::move(retTrueStmt));
  UniqueFEIRStmt retFalseStmt = FEIRBuilder::CreateStmtDAssign(std::move(tempVarCloned1), std::move(falseFEIRExpr));
  falseStmts.emplace_back(std::move(retFalseStmt));
  UniqueFEIRStmt stmtIf = FEIRBuilder::CreateStmtIf(std::move(condFEIRExpr), trueStmts, falseStmts);
  stmts.emplace_back(std::move(stmtIf));
  return FEIRBuilder::CreateExprDRead(std::move(tempVarCloned2));
}

// ---------- ASTConstantExpr ----------
UniqueFEIRExpr ASTConstantExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTImaginaryLiteral ----------
UniqueFEIRExpr ASTImaginaryLiteral::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_NULL_FATAL(complexType);
  GStrIdx nameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(FEUtils::GetSequentialName("Complex_"));
  UniqueFEIRVar complexVar = FEIRBuilder::CreateVarNameForC(nameIdx, *complexType);
  UniqueFEIRVar clonedComplexVar = complexVar->Clone();
  UniqueFEIRVar clonedComplexVar2 = complexVar->Clone();
  // real number
  UniqueFEIRExpr zeroConstExpr = FEIRBuilder::CreateExprConstAnyScalar(elemType->GetPrimType(), 0);
  UniqueFEIRStmt realStmt = std::make_unique<FEIRStmtDAssign>(std::move(complexVar), std::move(zeroConstExpr), 1);
  stmts.emplace_back(std::move(realStmt));
  // imaginary number
  CHECK_FATAL(child != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr childFEIRExpr = child->Emit2FEExpr(stmts);
  UniqueFEIRStmt imagStmt = std::make_unique<FEIRStmtDAssign>(std::move(clonedComplexVar), std::move(childFEIRExpr), 2);
  stmts.emplace_back(std::move(imagStmt));
  // return expr to parent operation
  UniqueFEIRExpr expr = FEIRBuilder::CreateExprDRead(std::move(clonedComplexVar2));
  return expr;
}

// ---------- ASTVAArgExpr ----------
UniqueFEIRExpr ASTVAArgExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_NULL_FATAL(mirType);
  VaArgInfo info = ProcessValistArgInfo(*mirType);
  UniqueFEIRExpr dreadVaList = child->Emit2FEExpr(stmts);
  CHECK_FATAL(dreadVaList->GetKind() == kExprDRead, "expr must be kExprDRead, %u", dreadVaList->GetKind());
  UniqueFEIRVar vaListVar = static_cast<FEIRExprDRead*>(dreadVaList.get())->GetVar()->Clone();
  // The va_arg_offset temp var is created and assigned from __gr_offs or __vr_offs of va_list
  MIRType *int32Type = GlobalTables::GetTypeTable().GetInt32();
  UniqueFEIRType int32FETRType = std::make_unique<FEIRTypeNative>(*int32Type);
  UniqueFEIRVar offsetVar = FEIRBuilder::CreateVarNameForC(FEUtils::GetSequentialName("va_arg_offs_"), *int32Type);
  UniqueFEIRExpr dreadVaListOffset = FEIRBuilder::CreateExprDReadAggField(
      vaListVar->Clone(), info.isGPReg ? 4 : 5, int32FETRType->Clone());
  UniqueFEIRStmt dassignOffsetVar = FEIRBuilder::CreateStmtDAssign(offsetVar->Clone(), dreadVaListOffset->Clone());
  stmts.emplace_back(std::move(dassignOffsetVar));
  UniqueFEIRExpr dreadOffsetVar = FEIRBuilder::CreateExprDRead(offsetVar->Clone());
  // The va_arg temp var is created and assigned in tow following way:
  // If the va_arg_offs is vaild, i.e., its value should be (0 - (8 - named_arg) * 8)  ~  0,
  // the va_arg will be got from GP or FP/SIMD arg reg, otherwise va_arg will be got from stack.
  // See https://developer.arm.com/documentation/ihi0055/latest/ for more info about the va_list type
  MIRType *uint64Type = GlobalTables::GetTypeTable().GetUInt64();
  UniqueFEIRType uint64FEIRType = std::make_unique<FEIRTypeNative>(*uint64Type);
  MIRType *ptrType = GlobalTables::GetTypeTable().GetOrCreatePointerType((!info.isCopyedMem ? *mirType : *uint64Type));
  UniqueFEIRVar vaArgVar = FEIRBuilder::CreateVarNameForC(FEUtils::GetSequentialName("va_arg"), *ptrType);
  UniqueFEIRExpr dreadVaArgTop = FEIRBuilder::CreateExprDReadAggField(
      vaListVar->Clone(), info.isGPReg ? 2 : 3, uint64FEIRType->Clone());
  UniqueFEIRExpr cvtOffset = FEIRBuilder::CreateExprCvtPrim(dreadOffsetVar->Clone(), PTY_u64);
  UniqueFEIRExpr addTopAndOffs = FEIRBuilder::CreateExprBinary(OP_add, std::move(dreadVaArgTop), std::move(cvtOffset));
  UniqueFEIRStmt dassignVaArgFromReg = FEIRBuilder::CreateStmtDAssign(vaArgVar->Clone(), std::move(addTopAndOffs));
  UniqueFEIRExpr condLower = FEIRBuilder::CreateExprBinary(  // checking validity: regOffset < 0
      OP_lt, dreadOffsetVar->Clone(), FEIRBuilder::CreateExprConstI32(0));
  UniqueFEIRExpr condUpper = FEIRBuilder::CreateExprBinary(  // checking validity: regOffset + next offset <= 0
      OP_le, dreadVaListOffset->Clone(), FEIRBuilder::CreateExprConstI32(0));
  UniqueFEIRExpr ArgAUnitOffs = FEIRBuilder::CreateExprBinary(
      OP_add, dreadOffsetVar->Clone(), FEIRBuilder::CreateExprConstI32(info.regOffset));
  std::list<UniqueFEIRStmt> trueStmtsInLower;
  std::list<UniqueFEIRStmt> trueStmtsInUpper;
  std::list<UniqueFEIRStmt> falseStmtsInLower;
  // The va_arg will be got from GP or FP/SIMD arg reg and set next reg setoff
  UniqueFEIRStmt dassignArgNextOffs = FEIRBuilder::CreateStmtDAssignAggField(
      vaListVar->Clone(), std::move(ArgAUnitOffs), info.isGPReg ? 4 : 5);
  trueStmtsInUpper.emplace_back(std::move(dassignVaArgFromReg));
  if (info.isHFA && !info.isCopyedMem) {
    UniqueFEIRVar copyedVar = FEIRBuilder::CreateVarNameForC(FEUtils::GetSequentialName("va_arg_struct"), *mirType);
    CvtHFA2Struct(*static_cast<MIRStructType*>(mirType), vaArgVar->Clone(), std::move(copyedVar), trueStmtsInUpper);
  }
  UniqueFEIRStmt stmtIfCondUpper = FEIRBuilder::CreateStmtIfWithoutElse(std::move(condUpper), trueStmtsInUpper);
  trueStmtsInLower.emplace_back(std::move(dassignArgNextOffs));
  trueStmtsInLower.emplace_back(std::move(stmtIfCondUpper));
  // Otherwise, the va_arg will be got from stack and set next stack setoff
  UniqueFEIRExpr dreadStackTop = FEIRBuilder::CreateExprDReadAggField(vaListVar->Clone(), 1, uint64FEIRType->Clone());
  UniqueFEIRStmt dassignVaArgFromStack = FEIRBuilder::CreateStmtDAssign(vaArgVar->Clone(), dreadStackTop->Clone());
  UniqueFEIRExpr stackAUnitOffs = FEIRBuilder::CreateExprBinary(
      OP_add, dreadStackTop->Clone(), FEIRBuilder::CreateExprConstU64(info.stackOffset));
  UniqueFEIRStmt dassignStackNextOffs = FEIRBuilder::CreateStmtDAssignAggField(
      vaListVar->Clone(), std::move(stackAUnitOffs), 1);
  falseStmtsInLower.emplace_back(std::move(dassignVaArgFromStack));
  falseStmtsInLower.emplace_back(std::move(dassignStackNextOffs));
  UniqueFEIRStmt stmtIfCondLower = FEIRBuilder::CreateStmtIf(std::move(condLower), trueStmtsInLower, falseStmtsInLower);
  stmts.emplace_back(std::move(stmtIfCondLower));
  // return va_arg
  UniqueFEIRExpr dreadRetVar = FEIRBuilder::CreateExprDRead(vaArgVar->Clone());
  if (info.isCopyedMem) {
    UniqueFEIRType ptrPtrFEIRType = std::make_unique<FEIRTypeNative>(
        *GlobalTables::GetTypeTable().GetOrCreatePointerType(*ptrType));
    dreadRetVar = FEIRBuilder::CreateExprIRead(
        std::move(uint64FEIRType), std::move(ptrPtrFEIRType), std::move(dreadRetVar));
  }
  UniqueFEIRType baseFEIRType = std::make_unique<FEIRTypeNative>(*mirType);
  UniqueFEIRType ptrFEIRType = std::make_unique<FEIRTypeNative>(*ptrType);
  UniqueFEIRExpr retExpr = FEIRBuilder::CreateExprIRead(
      std::move(baseFEIRType), std::move(ptrFEIRType), std::move(dreadRetVar));
  return retExpr;
}

VaArgInfo ASTVAArgExpr::ProcessValistArgInfo(MIRType &type) const {
  VaArgInfo info;
  if (type.IsScalarType()) {
    switch (type.GetPrimType()) {
      case PTY_f32:  // float is automatically promoted to double when passed to va_arg
        WARN(kLncWarn, "error: float is promoted to double when passed to va_arg");
      case PTY_f64:  // double
        info = { false, 16, 8, false, false };
        break;
      case PTY_i32:
      case PTY_i64:
        info = { true, 8, 8, false, false };
        break;
      default:  // bool, char, short, and unscoped enumerations are converted to int or wider integer types
        WARN(kLncWarn, "error: bool, char, short, and unscoped enumerations are promoted to int or "\
                       "wider integer types when passed to va_arg");
        info = { true, 8, 8, false, false };
        break;
    }
  } else if (type.IsMIRPtrType()) {
    info = { true, 8, 8, false, false };
  } else if (type.IsStructType()) {
    MIRStructType structType = static_cast<MIRStructType&>(type);
    size_t size = structType.GetSize();
    size = (size + 7) & -8;  // size round up 8
    if (size > 16) {
      info = { true, 8, 8, true, false };
    } else if (IsHFAType(structType)) {
      int fieldsSize = static_cast<int>(structType.GetFieldsSize());
      info = { false, fieldsSize * 16, static_cast<int>(size), false, true };
    } else {
      info = { true, static_cast<int>(size), static_cast<int>(size), false, false };
    }
  } else {
    CHECK_FATAL(false, "unsupport mirtype");
  }
  return info;
}

// Homogeneous Floating-point Aggregate:
// A data type with 2 to 4 identical floating-point members, either floats or doubles. (including 1 members here)
bool ASTVAArgExpr::IsHFAType(MIRStructType &type) const {
  size_t size = type.GetFieldsSize();
  if (size < 1 || size > 4) {
    return false;
  }
  PrimType firstFieldPrimType;
  for (size_t i = 0; i < size; ++i) {
    MIRType *fieldType = type.GetElemType(i);
    if (fieldType->GetPrimType() != PTY_f32 && fieldType->GetPrimType() != PTY_f64) {
      return false;
    }
    if (i == 0) {
      firstFieldPrimType = fieldType->GetPrimType();
    } else {
      if (fieldType->GetPrimType() != firstFieldPrimType) {
        return false;
      }
    }
  }
  return true;
}

// When va_arg is HFA struct,
// if it is passed as parameter in register then each uniquely addressable field goes in its own register.
// So its fields in FP/SIMD arg reg are still 128 bit and should be converted float or double type fields.
void ASTVAArgExpr::CvtHFA2Struct(MIRStructType &structType, UniqueFEIRVar vaArgVar, UniqueFEIRVar copyedVar,
                                 std::list<UniqueFEIRStmt> &stmts) const {
  int size = static_cast<int>(structType.GetFieldsSize());
  MIRType *fieldType = structType.GetElemType(0);
  MIRType *ptrMirType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*fieldType);
  UniqueFEIRType baseType = std::make_unique<FEIRTypeNative>(*fieldType);
  UniqueFEIRType ptrType = std::make_unique<FEIRTypeNative>(*ptrMirType);
  for (int i = 0; i < size; ++i) {
    UniqueFEIRExpr dreadVaArg = FEIRBuilder::CreateExprDRead(vaArgVar->Clone());
    if (i != 0) {
      dreadVaArg = FEIRBuilder::CreateExprBinary(
          OP_add, std::move(dreadVaArg), FEIRBuilder::CreateExprConstU64(16 * i));
    }
    UniqueFEIRExpr ireadVaArg = FEIRBuilder::CreateExprIRead(baseType->Clone(), ptrType->Clone(), dreadVaArg->Clone());
    UniqueFEIRStmt dassignCopyedVar = FEIRBuilder::CreateStmtDAssignAggField(
        copyedVar->Clone(), std::move(ireadVaArg), (i + 1));
    stmts.emplace_back(std::move(dassignCopyedVar));
  }
  UniqueFEIRExpr addrofCopyedVar = FEIRBuilder::CreateExprAddrofVar(copyedVar->Clone());
  UniqueFEIRStmt assignVar = FEIRBuilder::CreateStmtDAssign(vaArgVar->Clone(), std::move(addrofCopyedVar));
  stmts.emplace_back(std::move(assignVar));
}

// ---------- ASTCStyleCastExpr ----------
UniqueFEIRExpr ASTCStyleCastExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  if (destType->GetPrimType() == PTY_void) {
    std::list<UniqueFEIRExpr> feExprs;
    feExprs.emplace_back(child->Emit2FEExpr(stmts));
    UniqueFEIRStmt stmt = std::make_unique<FEIRStmtNary>(OP_eval, std::move(feExprs));
    stmts.emplace_back(std::move(stmt));
    return nullptr;
  }
  MIRType *src = canCastArray ? decl->GetTypeDesc().front() : srcType;
  auto feirCStyleCastExpr = std::make_unique<FEIRExprCStyleCast>(src, destType,
                                                                 child->Emit2FEExpr(stmts),
                                                                 canCastArray);
  if (decl != nullptr) {
    feirCStyleCastExpr->SetRefName(decl->GenerateUniqueVarName());
  }
  return feirCStyleCastExpr;
}

// ---------- ASTArrayInitLoopExpr ----------
UniqueFEIRExpr ASTArrayInitLoopExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTArrayInitIndexExpr ----------
UniqueFEIRExpr ASTArrayInitIndexExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTExprWithCleanups ----------
UniqueFEIRExpr ASTExprWithCleanups::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTMaterializeTemporaryExpr ----------
UniqueFEIRExpr ASTMaterializeTemporaryExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTSubstNonTypeTemplateParmExpr ----------
UniqueFEIRExpr ASTSubstNonTypeTemplateParmExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTDependentScopeDeclRefExpr ----------
UniqueFEIRExpr ASTDependentScopeDeclRefExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTAtomicExpr ----------
UniqueFEIRExpr ASTAtomicExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  auto atomicExpr = std::make_unique<FEIRExprAtomic>(type, refType, objExpr->Emit2FEExpr(stmts), atomicOp);
  if (atomicOp != kAtomicOpLoad) {
    static_cast<FEIRExprAtomic*>(atomicExpr.get())->SetVal1Expr(valExpr1->Emit2FEExpr(stmts));
    static_cast<FEIRExprAtomic*>(atomicExpr.get())->SetVal1Type(val1Type);
  }

  if (atomicOp == kAtomicOpCompareExchange) {
    static_cast<FEIRExprAtomic*>(atomicExpr.get())->SetVal2Expr(valExpr2->Emit2FEExpr(stmts));
    static_cast<FEIRExprAtomic*>(atomicExpr.get())->SetVal2Type(val2Type);
  }
  auto lock = FEIRBuilder::CreateVarNameForC(FEUtils::GetSequentialName("lockVar"), *type, false, false);
  auto var = FEIRBuilder::CreateVarNameForC(FEUtils::GetSequentialName("valueVar"), *refType, false, false);
  atomicExpr->SetLockVar(lock->Clone());
  atomicExpr->SetValVar(var->Clone());
  if (!isFromStmt) {
    auto stmt = std::make_unique<FEIRStmtAtomic>(std::move(atomicExpr));
    stmts.emplace_back(std::move(stmt));
    return FEIRBuilder::CreateExprDRead(var->Clone());
  }
  return atomicExpr;
}

// ---------- ASTExprStmtExpr ----------
UniqueFEIRExpr ASTExprStmtExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  std::list<UniqueFEIRStmt> stmts0 = cpdStmt->Emit2FEStmt();
  for (auto &stmt : stmts0) {
    stmts.emplace_back(std::move(stmt));
  }
  CHECK_FATAL(cpdStmt->GetASTStmtOp() == kASTStmtCompound, "Invalid in ASTExprStmtExpr");
  stmts0.clear();
  return static_cast<ASTCompoundStmt*>(cpdStmt)->GetASTStmtList().back()->GetExprs().back()->Emit2FEExpr(stmts);
}
}
