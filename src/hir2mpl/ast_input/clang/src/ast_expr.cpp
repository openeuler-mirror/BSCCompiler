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
#include "ast_macros.h"
#include "mpl_logging.h"
#include "feir_stmt.h"
#include "feir_builder.h"
#include "fe_utils_ast.h"
#include "feir_type_helper.h"
#include "fe_manager.h"
#include "ast_stmt.h"
#include "ast_util.h"
#include "enhance_c_checker.h"
#include "ror.h"
#include "conditional_operator.h"

namespace maple {
const uint32 kOneByte = 8;
// ---------- ASTValue ----------
MIRConst *ASTValue::Translate2MIRConst() const {
  switch (pty) {
    case PTY_u1: {
      return GlobalTables::GetIntConstTable().GetOrCreateIntConst(
          val.u8, *GlobalTables::GetTypeTable().GetPrimType(PTY_u1));
    }
    case PTY_u8: {
      return GlobalTables::GetIntConstTable().GetOrCreateIntConst(
          val.u8, *GlobalTables::GetTypeTable().GetPrimType(PTY_u8));
    }
    case PTY_u16: {
      return GlobalTables::GetIntConstTable().GetOrCreateIntConst(
          val.u16, *GlobalTables::GetTypeTable().GetPrimType(PTY_u16));
    }
    case PTY_u32: {
      return GlobalTables::GetIntConstTable().GetOrCreateIntConst(
          val.u32, *GlobalTables::GetTypeTable().GetPrimType(PTY_u32));
    }
    case PTY_u64: {
      return GlobalTables::GetIntConstTable().GetOrCreateIntConst(
          val.u64, *GlobalTables::GetTypeTable().GetPrimType(PTY_u64));
    }
    case PTY_i8: {
      return GlobalTables::GetIntConstTable().GetOrCreateIntConst(
          val.i8, *GlobalTables::GetTypeTable().GetPrimType(PTY_i8));
    }
    case PTY_i16: {
      return GlobalTables::GetIntConstTable().GetOrCreateIntConst(
          val.i16, *GlobalTables::GetTypeTable().GetPrimType(PTY_i16));
    }
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
      FieldID fieldID = 0;
      FEUtils::TraverseToNamedField(*structType, structType->GetElemStrIdx(i), fieldID);
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
    for (uint32 i = 0; i < numElems; ++i) {
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
    MIRSymbol *funcSymbol = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(idx);
    CHECK_FATAL(funcSymbol != nullptr, "Should process func decl before var decl");
    MIRFunction *mirFunc = funcSymbol->GetFunction();
    CHECK_FATAL(mirFunc != nullptr, "Same name symbol with function: %s", refedDecl->GetName().c_str());
    return FEManager::GetModule().GetMemPool()->New<MIRAddroffuncConst>(mirFunc->GetPuidx(), *mirType);
  } else if (!isConstantFolded) {
    ASTDecl *var = refedDecl;
    MIRSymbol *mirSymbol;
    if (var->IsGlobal()) {
      mirSymbol = FEManager::GetMIRBuilder().GetOrCreateGlobalDecl(
          var->GenerateUniqueVarName(), *(var->GetTypeDesc().front()));
    } else {
      mirSymbol = FEManager::GetMIRBuilder().GetOrCreateLocalDecl(
          var->GenerateUniqueVarName(), *(var->GetTypeDesc().front()));
    }
    return FEManager::GetModule().GetMemPool()->New<MIRAddrofConst>(
        mirSymbol->GetStIdx(), 0, *(var->GetTypeDesc().front()));
  } else {
    return GetConstantValue()->Translate2MIRConst();
  }
}

UniqueFEIRExpr ASTDeclRefExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  (void)stmts;
  MIRType *mirType = refedDecl->GetTypeDesc().front();
  UniqueFEIRExpr feirRefExpr;
  auto attrs = refedDecl->GetGenericAttrs();
  if (mirType->GetKind() == kTypePointer &&
      static_cast<MIRPtrType*>(mirType)->GetPointedType()->GetKind() == kTypeFunction) {
    feirRefExpr = FEIRBuilder::CreateExprAddrofFunc(refedDecl->GetName());
  } else {
    if (refedDecl->GetDeclKind() == kASTEnumConstant) {
      return FEIRBuilder::CreateExprConstAnyScalar(refedDecl->GetTypeDesc().front()->GetPrimType(),
          static_cast<ASTEnumConstant*>(refedDecl)->GetValue());
    }
    UniqueFEIRVar feirVar =
        FEIRBuilder::CreateVarNameForC(refedDecl->GenerateUniqueVarName(), *mirType, refedDecl->IsGlobal(), false);
    feirVar->SetAttrs(attrs);
    if (mirType->GetKind() == kTypeArray) {
      feirRefExpr = FEIRBuilder::CreateExprAddrofVar(std::move(feirVar));
    } else {
      feirRefExpr = FEIRBuilder::CreateExprDRead(std::move(feirVar));
    }
  }
  return feirRefExpr;
}

// ---------- ASTCallExpr ----------
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

std::unordered_map<std::string, ASTCallExpr::FuncPtrBuiltinFunc> ASTCallExpr::builtingFuncPtrMap =
     ASTCallExpr::InitBuiltinFuncPtrMap();

void ASTCallExpr::AddArgsExpr(const std::unique_ptr<FEIRStmtAssign> &callStmt, std::list<UniqueFEIRStmt> &stmts) const {
  for (int32 i = (static_cast<int32>(args.size()) - 1); i >= 0; --i) {
    UniqueFEIRExpr expr = args[i]->Emit2FEExpr(stmts);
    callStmt->AddExprArgReverse(std::move(expr));
  }
  if (IsFirstArgRet()) {
    UniqueFEIRVar var = FEIRBuilder::CreateVarNameForC(varName, *retType, false, false);
    UniqueFEIRExpr expr = FEIRBuilder::CreateExprAddrofVar(var->Clone());
    callStmt->AddExprArgReverse(std::move(expr));
  }
  if (isIcall) {
    UniqueFEIRExpr expr = calleeExpr->Emit2FEExpr(stmts);
    InsertNonnullCheckingForIcall(expr, stmts);
    InsertBoundaryCheckingInArgsForICall(stmts, expr);
    callStmt->AddExprArgReverse(std::move(expr));
  }
  InsertBoundaryCheckingInArgs(stmts);
  CheckNonnullFieldInStruct();
}

void ASTCallExpr::InsertNonnullCheckingForIcall(const UniqueFEIRExpr &expr, std::list<UniqueFEIRStmt> &stmts) const {
  if (!FEOptions::GetInstance().IsNpeCheckDynamic() || expr->GetPrimType() != PTY_ptr) {
    return;
  }
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtAssertNonnull>(OP_assertnonnull, expr->Clone());
  stmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
  stmts.emplace_back(std::move(stmt));
}

UniqueFEIRExpr ASTCallExpr::AddRetExpr(const std::unique_ptr<FEIRStmtAssign> &callStmt) const {
  UniqueFEIRVar var = FEIRBuilder::CreateVarNameForC(varName, *retType, false, false);
  UniqueFEIRVar dreadVar = var->Clone();
  if (!IsFirstArgRet()) {
    callStmt->SetVar(var->Clone());
  }
  return FEIRBuilder::CreateExprDRead(dreadVar->Clone());
}

std::unique_ptr<FEIRStmtAssign> ASTCallExpr::GenCallStmt() const {
  MemPool *mp = FEManager::GetManager().GetStructElemMempool();
  std::unique_ptr<FEIRStmtAssign> callStmt;
  if (isIcall) {
    callStmt = std::make_unique<FEIRStmtICallAssign>();
  } else {
    StructElemNameIdx *nameIdx = mp->New<StructElemNameIdx>(funcName);
    FEStructMethodInfo *info = static_cast<FEStructMethodInfo*>(
        FEManager::GetTypeManager().RegisterStructMethodInfo(*nameIdx, kSrcLangC, false));
    info->SetFuncAttrs(funcAttrs);
    FEIRTypeNative *retTypeInfo = nullptr;
    if (IsFirstArgRet()) {
      retTypeInfo = mp->New<FEIRTypeNative>(*GlobalTables::GetTypeTable().GetPrimType(PTY_void));
    } else {
      retTypeInfo = mp->New<FEIRTypeNative>(*retType);
    }
    info->SetReturnType(retTypeInfo);
    Opcode op;
    if (retTypeInfo->GetPrimType() != PTY_void) {
      op = OP_callassigned;
    } else {
      op = OP_call;
    }
    callStmt = std::make_unique<FEIRStmtCallAssign>(*info, op, nullptr, false);
  }
  callStmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
  return callStmt;
}

UniqueFEIRExpr ASTCallExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  if (!isIcall) {
    bool isFinish = false;
    UniqueFEIRExpr buitinExpr = ProcessBuiltinFunc(stmts, isFinish);
    if (isFinish) {
      return buitinExpr;
    }
  }
  std::unique_ptr<FEIRStmtAssign> callStmt = GenCallStmt();
  AddArgsExpr(callStmt, stmts);
  UniqueFEIRExpr retExpr = nullptr;
  if (IsNeedRetExpr()) {
    retExpr = AddRetExpr(callStmt);
  }
  stmts.emplace_back(std::move(callStmt));
  InsertBoundaryVarInRet(stmts);
  return retExpr;
}

// ---------- ASTCastExpr ----------
MIRConst *ASTCastExpr::GenerateMIRConstImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  auto feExpr = child->Emit2FEExpr(stmts);
  if (isArrayToPointerDecay && feExpr->GetKind() == FEIRNodeKind::kExprAddrof) {
    return FEManager::GetModule().GetMemPool()->New<MIRStrConst>(
        GetConstantValue()->val.strIdx, *GlobalTables::GetTypeTable().GetPrimType(PTY_a64));
  } else if (isArrayToPointerDecay && child->GetASTOp() == kASTOpRef) {
    ASTDecl *astDecl = static_cast<ASTDeclRefExpr*>(child)->GetASTDecl();
    CHECK_FATAL(astDecl->GetDeclKind() == kASTVar, "Invalid");
    MIRSymbol *mirSymbol = static_cast<ASTVar*>(astDecl)->Translate2MIRSymbol();
    return FEManager::GetModule().GetMemPool()->New<MIRAddrofConst>(mirSymbol->GetStIdx(), 0,
                                                                    *(astDecl->GetTypeDesc().front()));
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

MIRConst *ASTCastExpr::GenerateMIRDoubleConst() const {
  MIRConst *childConst = child->GenerateMIRConst();
  switch (childConst->GetKind()) {
    case kConstFloatConst: {
      return FEManager::GetModule().GetMemPool()->New<MIRDoubleConst>(
          static_cast<double>(static_cast<MIRFloatConst*>(childConst)->GetValue()),
          *GlobalTables::GetTypeTable().GetPrimType(PTY_f64));
    }
    case kConstInt: {
      return FEManager::GetModule().GetMemPool()->New<MIRDoubleConst>(
          static_cast<double>(static_cast<MIRIntConst*>(childConst)->GetValue()),
          *GlobalTables::GetTypeTable().GetPrimType(PTY_f64));
    }
    case kConstDoubleConst: {
      return FEManager::GetModule().GetMemPool()->New<MIRDoubleConst>(
          static_cast<double>(static_cast<MIRDoubleConst*>(childConst)->GetValue()),
          *GlobalTables::GetTypeTable().GetPrimType(PTY_f64));
    }
    default: {
      CHECK_FATAL(false, "Unsupported pty type: %d", GetConstantValue()->pty);
      return nullptr;
    }
  }
}

MIRConst *ASTCastExpr::GenerateMIRFloatConst() const {
  MIRConst *childConst = child->GenerateMIRConst();
  switch (childConst->GetKind()) {
    case kConstDoubleConst: {
      return FEManager::GetModule().GetMemPool()->New<MIRFloatConst>(
          static_cast<float>(static_cast<MIRDoubleConst*>(childConst)->GetValue()),
          *GlobalTables::GetTypeTable().GetPrimType(PTY_f32));
    }
    case kConstInt: {
      return FEManager::GetModule().GetMemPool()->New<MIRFloatConst>(
          static_cast<float>(static_cast<MIRIntConst*>(childConst)->GetValue()),
          *GlobalTables::GetTypeTable().GetPrimType(PTY_f32));
    }
    default: {
      CHECK_FATAL(false, "Unsupported pty type: %d", GetConstantValue()->pty);
      return nullptr;
    }
  }
}

MIRConst *ASTCastExpr::GenerateMIRIntConst() const {
  MIRConst *childConst = child->GenerateMIRConst();
  switch (childConst->GetKind()) {
    case kConstDoubleConst:
    case kConstInt: {
      int64 val = childConst->GetKind() == kConstDoubleConst ?
          static_cast<int64>(static_cast<MIRDoubleConst*>(childConst)->GetValue()) :
          static_cast<MIRIntConst*>(childConst)->GetValue();

      PrimType destPrimType = mirType->GetPrimType();
      switch (destPrimType) {
        case PTY_i8:
          val = static_cast<int8>(val);
          break;
        case PTY_i16:
          val = static_cast<int16>(val);
          break;
        case PTY_i32:
          val = static_cast<int32>(val);
          break;
        case PTY_i64:
          val = static_cast<int64>(val);
          break;
        case PTY_u8:
          val = static_cast<uint8>(val);
          break;
        case PTY_u16:
          val = static_cast<uint16>(val);
          break;
        case PTY_u32:
          val = static_cast<uint32>(val);
          break;
        case PTY_u64:
          val = static_cast<uint64>(val);
          break;
        default:
          break;
      }
      return FEManager::GetModule().GetMemPool()->New<MIRIntConst>(
          val, *GlobalTables::GetTypeTable().GetPrimType(PTY_i64));
    }
    case kConstStrConst: {
      return FEManager::GetModule().GetMemPool()->New<MIRIntConst>(
          static_cast<int64>(static_cast<MIRStrConst*>(childConst)->GetValue()),
          *GlobalTables::GetTypeTable().GetPrimType(PTY_a64));
    }
    case kConstAddrof: {
      return FEManager::GetModule().GetMemPool()->New<MIRIntConst>(
          static_cast<int64>(static_cast<MIRAddrofConst*>(childConst)->GetOffset()),
          *GlobalTables::GetTypeTable().GetPrimType(PTY_i64));
    }
    case kConstLblConst: {
      // init by initListExpr, Only MIRConst kind is set here.
      return childConst;
    }
    default: {
      CHECK_FATAL(false, "Unsupported pty type: %d", GetConstantValue()->pty);
      return nullptr;
    }
  }
}

UniqueFEIRExpr ASTCastExpr::Emit2FEExprForComplex(const UniqueFEIRExpr &subExpr, const UniqueFEIRType &srcType,
                                                  std::list<UniqueFEIRStmt> &stmts) const {
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
    realExpr = FEIRBuilder::CreateExprCastPrim(subExpr->Clone(), dst->GetPrimType());
    imagExpr = FEIRBuilder::CreateExprCastPrim(std::move(cloneSubExpr), dst->GetPrimType());
    UniqueFEIRStmt realStmt = std::make_unique<FEIRStmtDAssign>(tmpVar->Clone(), std::move(realExpr), kComplexRealID);
    stmts.emplace_back(std::move(realStmt));
    UniqueFEIRStmt imagStmt = std::make_unique<FEIRStmtDAssign>(tmpVar->Clone(), std::move(imagExpr), kComplexImagID);
    stmts.emplace_back(std::move(imagStmt));
    dreadAgg = FEIRBuilder::CreateExprDRead(std::move(tmpVar));
  }
  return dreadAgg;
}

UniqueFEIRExpr ASTCastExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  const ASTExpr *childExpr = child;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  if (isArrayToPointerDecay) {
    auto childFEExpr = childExpr->Emit2FEExpr(stmts);
    if (childFEExpr->GetKind() == kExprDRead) {
      return std::make_unique<FEIRExprAddrofVar>(
          static_cast<FEIRExprDRead*>(childFEExpr.get())->GetVar()->Clone(), childFEExpr->GetFieldID());
    } else if (childFEExpr->GetKind() == kExprIRead) {
      auto iread = static_cast<FEIRExprIRead*>(childFEExpr.get());
      if (iread->GetFieldID() == 0) {
        auto addrOfExpr = iread->GetClonedOpnd();
        ENCChecker::ReduceBoundaryChecking(stmts, addrOfExpr);
        return addrOfExpr;
      } else {
        return std::make_unique<FEIRExprIAddrof>(iread->GetClonedPtrType(), iread->GetFieldID(),
            iread->GetClonedOpnd());
      }
    } else if (childFEExpr->GetKind() == kExprIAddrof || childFEExpr->GetKind() == kExprAddrofVar ||
        childFEExpr->GetKind() == kExprAddrofFunc || childFEExpr->GetKind() == kExprAddrof) {
      return childFEExpr;
    } else {
      CHECK_FATAL(false, "unsupported expr kind %d", childFEExpr->GetKind());
    }
  }
  UniqueFEIRExpr subExpr = childExpr->Emit2FEExpr(stmts);
  if (isUnoinCast && dst->GetKind() == kTypeUnion) {
    return subExpr;
  }
  if (isBitCast) {
    if (src->GetPrimType() == dst->GetPrimType() && src->IsScalarType()) {
      // This case may show up when casting from a 1-element vector to its scalar type.
      return subExpr;
    }
    UniqueFEIRType dstType = std::make_unique<FEIRTypeNative>(*dst);
    if (dst->GetKind() == kTypePointer) {
      CheckNonnullFieldInStruct();
      return subExpr;
    } else {
      return std::make_unique<FEIRExprTypeCvt>(std::move(dstType), OP_retype, std::move(subExpr));
    }
  }
  if (isVectorSplat) {
    return EmitExprVdupVector(dst->GetPrimType(), subExpr);
  }
  if (complexType != nullptr) {
    UniqueFEIRType srcType = std::make_unique<FEIRTypeNative>(*src);
    return Emit2FEExprForComplex(subExpr, srcType, stmts);
  }
  if (!IsNeededCvt(subExpr)) {
    return subExpr;
  }
  if (IsPrimitiveFloat(subExpr->GetPrimType()) && IsPrimitiveInteger(dst->GetPrimType()) &&
      dst->GetPrimType() != PTY_u1) {
    return FEIRBuilder::CreateExprCvtPrim(OP_trunc, std::move(subExpr), dst->GetPrimType());
  }
  return FEIRBuilder::CreateExprCastPrim(std::move(subExpr), dst->GetPrimType());
}

UniqueFEIRExpr ASTCastExpr::EmitExprVdupVector(PrimType primtype, UniqueFEIRExpr &subExpr) const {
MIRIntrinsicID intrinsic;
  switch (primtype) {
#define SET_VDUP(TY)                                                          \
    case PTY_##TY:                                                            \
      intrinsic = INTRN_vector_from_scalar_##TY;                              \
      break;

    SET_VDUP(v2i64)
    SET_VDUP(v4i32)
    SET_VDUP(v8i16)
    SET_VDUP(v16i8)
    SET_VDUP(v2u64)
    SET_VDUP(v4u32)
    SET_VDUP(v8u16)
    SET_VDUP(v16u8)
    SET_VDUP(v2f64)
    SET_VDUP(v4f32)
    SET_VDUP(v2i32)
    SET_VDUP(v4i16)
    SET_VDUP(v8i8)
    SET_VDUP(v2u32)
    SET_VDUP(v4u16)
    SET_VDUP(v8u8)
    SET_VDUP(v2f32)
    case PTY_i64:
      intrinsic = INTRN_vector_from_scalar_v1i64;
      break;
    case PTY_u64:
      intrinsic = INTRN_vector_from_scalar_v1f64;
      break;
    case PTY_f64:
      intrinsic = INTRN_vector_from_scalar_v1f64;
      break;
    default:
      CHECK_FATAL(false, "Unhandled vector type in CreateExprVdupAnyVector");
  }
  UniqueFEIRType feType = FEIRTypeHelper::CreateTypeNative(*GlobalTables::GetTypeTable().GetPrimType(primtype));
  std::vector<std::unique_ptr<FEIRExpr>> argOpnds;
  argOpnds.push_back(std::move(subExpr));
  return std::make_unique<FEIRExprIntrinsicopForC>(std::move(feType), intrinsic, argOpnds);
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
  CHECK_NULL_FATAL(subType);
  if (childFEIRExpr->GetPrimType() != dstType) {
    UniqueFEIRExpr minusExpr = std::make_unique<FEIRExprUnary>(
        FEIRTypeHelper::CreateTypeNative(*subType), OP_neg, std::move(childFEIRExpr));
    return FEIRBuilder::CreateExprCastPrim(std::move(minusExpr), dstType);
  }
  return std::make_unique<FEIRExprUnary>(FEIRTypeHelper::CreateTypeNative(*subType), OP_neg, std::move(childFEIRExpr));
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
  CHECK_NULL_FATAL(subType);
  if (childFEIRExpr->GetPrimType() != dstType) {
    UniqueFEIRExpr notExpr = std::make_unique<FEIRExprUnary>(
        FEIRTypeHelper::CreateTypeNative(*subType), OP_bnot, std::move(childFEIRExpr));
    return FEIRBuilder::CreateExprCastPrim(std::move(notExpr), dstType);
  }
  return std::make_unique<FEIRExprUnary>(FEIRTypeHelper::CreateTypeNative(*subType), OP_bnot, std::move(childFEIRExpr));
}

UniqueFEIRExpr ASTUOLNotExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  childExpr->SetShortCircuitIdx(falseIdx, trueIdx);
  UniqueFEIRExpr childFEIRExpr = childExpr->Emit2FEExpr(stmts);
  if (childFEIRExpr != nullptr) {
    return FEIRBuilder::CreateExprZeroCompare(OP_eq, std::move(childFEIRExpr));
  }
  return childFEIRExpr;
}

UniqueFEIRExpr ASTUnaryOperatorExpr::ASTUOSideEffectExpr(Opcode op, std::list<UniqueFEIRStmt> &stmts,
    const std::string &varName, bool post) const {
  ASTExpr *childExpr = expr;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr childFEIRExpr = childExpr->Emit2FEExpr(stmts);
  UniqueFEIRVar tempVar;
  if (post) {
    tempVar = FEIRBuilder::CreateVarNameForC(varName, *subType);
    UniqueFEIRStmt readSelfstmt = FEIRBuilder::CreateStmtDAssign(tempVar->Clone(), childFEIRExpr->Clone());
    readSelfstmt->SetDummy();
    stmts.emplace_back(std::move(readSelfstmt));
  }

  PrimType subPrimType = subType->GetPrimType();
  UniqueFEIRExpr subExpr = (subPrimType == PTY_ptr) ? std::make_unique<FEIRExprConst>(pointeeLen, PTY_i32) :
      FEIRBuilder::CreateExprConstAnyScalar(subPrimType, 1);
  UniqueFEIRExpr sideEffectExpr = FEIRBuilder::CreateExprMathBinary(op, childFEIRExpr->Clone(), std::move(subExpr));
  UniqueFEIRStmt sideEffectStmt = FEIRBuilder::AssginStmtField(childFEIRExpr->Clone(), std::move(sideEffectExpr), 0);
  stmts.emplace_back(std::move(sideEffectStmt));

  if (post) {
    return FEIRBuilder::CreateExprDRead(std::move(tempVar));
  }
  return childFEIRExpr;
}

UniqueFEIRExpr ASTUOPostIncExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  return ASTUOSideEffectExpr(OP_add, stmts, tempVarName, true);
}

UniqueFEIRExpr ASTUOPostDecExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  return ASTUOSideEffectExpr(OP_sub, stmts, tempVarName, true);
}

UniqueFEIRExpr ASTUOPreIncExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  return ASTUOSideEffectExpr(OP_add, stmts);
}

UniqueFEIRExpr ASTUOPreDecExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  return ASTUOSideEffectExpr(OP_sub, stmts);
}

MIRConst *ASTUOAddrOfExpr::GenerateMIRConstImpl() const {
  switch (expr->GetASTOp()) {
    case kASTOpCompoundLiteralExp: {
      auto astCompoundLiteralExpr = static_cast<ASTCompoundLiteralExpr*>(expr);
      // CompoundLiteral Symbol
      MIRSymbol *compoundLiteralMirSymbol = FEManager::GetMIRBuilder().GetOrCreateGlobalDecl(
          astCompoundLiteralExpr->GetInitName(),
          *astCompoundLiteralExpr->GetCompoundLiteralType());

      auto child = static_cast<ASTCompoundLiteralExpr*>(expr)->GetASTExpr();
      auto mirConst = child->GenerateMIRConst(); // InitListExpr in CompoundLiteral gen struct
      compoundLiteralMirSymbol->SetKonst(mirConst);

      MIRAddrofConst *mirAddrofConst = FEManager::GetModule().GetMemPool()->New<MIRAddrofConst>(
          compoundLiteralMirSymbol->GetStIdx(), 0, *astCompoundLiteralExpr->GetCompoundLiteralType());
      return mirAddrofConst;
    }
    case kASTOpRef:
    case kASTSubscriptExpr:
    case kASTMemberExpr: {
      return expr->GenerateMIRConst();
    }
    case kASTStringLiteral: {
      return FEManager::GetModule().GetMemPool()->New<MIRStrConst>(
          expr->GetConstantValue()->val.strIdx, *GlobalTables::GetTypeTable().GetPrimType(PTY_a64));
    }
    default: {
      CHECK_FATAL(false, "lValue in expr: %d NIY", expr->GetASTOp());
    }
  }
  return nullptr;
}

UniqueFEIRExpr ASTUOAddrOfExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  UniqueFEIRExpr addrOfExpr;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr childFEIRExpr = childExpr->Emit2FEExpr(stmts);
  if (childFEIRExpr->GetKind() == kExprDRead) {
    addrOfExpr = std::make_unique<FEIRExprAddrofVar>(
        static_cast<FEIRExprDRead*>(childFEIRExpr.get())->GetVar()->Clone(), childFEIRExpr->GetFieldID());
  } else if (childFEIRExpr->GetKind() == kExprIRead) {
    auto ireadExpr = static_cast<FEIRExprIRead*>(childFEIRExpr.get());
    if (ireadExpr->GetFieldID() == 0) {
      addrOfExpr = ireadExpr->GetClonedOpnd();
      ENCChecker::ReduceBoundaryChecking(stmts, addrOfExpr);
    } else {
      addrOfExpr = std::make_unique<FEIRExprIAddrof>(ireadExpr->GetClonedPtrType(), ireadExpr->GetFieldID(),
                                                     ireadExpr->GetClonedOpnd());
    }
  } else if (childFEIRExpr->GetKind() == kExprIAddrof || childFEIRExpr->GetKind() == kExprAddrofVar ||
      childFEIRExpr->GetKind() == kExprAddrofFunc || childFEIRExpr->GetKind() == kExprAddrof) {
    return childFEIRExpr;
  } else {
    CHECK_FATAL(false, "unsupported expr kind %d", childFEIRExpr->GetKind());
  }
  return addrOfExpr;
}

// ---------- ASTUOAddrOfLabelExpr ---------
MIRConst *ASTUOAddrOfLabelExpr::GenerateMIRConstImpl() const {
  return FEManager::GetMIRBuilder().GetCurrentFuncCodeMp()->New<MIRLblConst>(
      FEManager::GetMIRBuilder().GetOrCreateMIRLabel(labelName),
      FEManager::GetMIRBuilder().GetCurrentFunction()->GetPuidx(),  // GetCurrentFunction need to be optimized
      *GlobalTables::GetTypeTable().GetVoidPtr());                  // when parallel features
}

UniqueFEIRExpr ASTUOAddrOfLabelExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  (void)stmts;
  return FEIRBuilder::CreateExprAddrofLabel(labelName, std::make_unique<FEIRTypeNative>(*uoType));
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
  InsertNonnullChecking(stmts, childFEIRExpr->Clone());
  if (InsertBoundaryChecking(stmts, childFEIRExpr->Clone())) {
    childFEIRExpr->SetIsBoundaryChecking(true);
  }
  UniqueFEIRExpr derefExpr = FEIRBuilder::CreateExprIRead(std::move(retType), std::move(ptrType),
                                                          std::move(childFEIRExpr));
  return derefExpr;
}

void ASTUODerefExpr::InsertNonnullChecking(std::list<UniqueFEIRStmt> &stmts, UniqueFEIRExpr baseExpr) const {
  if (!FEOptions::GetInstance().IsNpeCheckDynamic()) {
    return;
  }
  if (baseExpr->GetPrimType() == PTY_ptr) {
    UniqueFEIRStmt stmt = std::make_unique<FEIRStmtAssertNonnull>(OP_assertnonnull, std::move(baseExpr));
    stmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
    stmts.emplace_back(std::move(stmt));
  }
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
  (void)stmts;
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
    condFEIRExpr = FEIRBuilder::CreateExprZeroCompare(OP_ne, std::move(condFEIRExpr));
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

MIRConst *ASTCompoundLiteralExpr::GenerateMIRConstImpl() const {
  return child->GenerateMIRConst();
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

void ASTOffsetOfExpr::SetFieldName(const std::string &fName) {
  fieldName = fName;
}

UniqueFEIRExpr ASTOffsetOfExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  (void)stmts;
  return FEIRBuilder::CreateExprConstU64(static_cast<uint64>(offset));
}

// ---------- ASTInitListExpr ----------
MIRConst *ASTInitListExpr::GenerateMIRConstImpl() const {
  if (initListType->GetKind() == kTypeArray) {
    return GenerateMIRConstForArray();
  } else if (initListType->GetKind() == kTypeStruct || initListType->GetKind() == kTypeUnion) {
    return GenerateMIRConstForStruct();
  } else if (isTransparent) {
    return initExprs[0]->GenerateMIRConst();
  } else {
    CHECK_FATAL(false, "not handle now");
  }
}

MIRConst *ASTInitListExpr::GenerateMIRConstForArray() const {
  if (initExprs.size() == 1 && initExprs[0]->GetASTOp() == kASTStringLiteral) {
    return initExprs[0]->GenerateMIRConst();
  }
  MIRAggConst *aggConst = FEManager::GetModule().GetMemPool()->New<MIRAggConst>(FEManager::GetModule(), *initListType);
  CHECK_FATAL(initListType->GetKind() == kTypeArray, "Must be array type");
  auto arrayMirType = static_cast<MIRArrayType*>(initListType);
  CHECK_FATAL(initExprs.size() <= arrayMirType->GetSizeArrayItem(0), "InitExpr size must less or equal array size");
  for (size_t i = 0; i < initExprs.size(); ++i) {
    auto konst = initExprs[i]->GenerateMIRConst();
    if (konst->GetKind() == kConstLblConst) {
      // init by initListExpr, Only MIRConst kind is set here.
      return konst;
    }
    aggConst->AddItem(konst, 0);
  }
  if (HasArrayFiller()) {
    auto fillerConst = arrayFillerExpr->GenerateMIRConst();
    for (uint32 i = initExprs.size(); i < arrayMirType->GetSizeArrayItem(0); ++i) {
      aggConst->AddItem(fillerConst, 0);
    }
  }
  return aggConst;
}

MIRConst *ASTInitListExpr::GenerateMIRConstForStruct() const {
  if (initExprs.empty()) {
    return nullptr; // No var constant generation
  }
  bool hasFiller = false;
  for (auto e : initExprs) {
    if (e != nullptr) {
      hasFiller = true;
      break;
    }
  }
  if (!hasFiller) {
    return nullptr;
  }
  MIRAggConst *aggConst = FEManager::GetModule().GetMemPool()->New<MIRAggConst>(FEManager::GetModule(), *initListType);
  CHECK_FATAL(initExprs.size() <= UINT_MAX, "Too large elem size");
  for (uint32 i = 0; i < static_cast<uint32>(initExprs.size()); ++i) {
    if (initExprs[i] == nullptr) {
      continue;
    }
    auto konst = initExprs[i]->GenerateMIRConst();
    if (konst->GetKind() == kConstLblConst) {
      // init by initListExpr, Only MIRConst kind is set here.
      return konst;
    }
    aggConst->AddItem(konst, i + 1);
  }
  ENCChecker::CheckNullFieldInGlobalStruct(*initListType, *aggConst, initExprs);
  return aggConst;
}

UniqueFEIRExpr ASTInitListExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRVar feirVar = FEIRBuilder::CreateVarNameForC(varName, *initListType);
  if (initListType->GetKind() == MIRTypeKind::kTypeArray) {
    UniqueFEIRExpr arrayExpr = FEIRBuilder::CreateExprAddrofVar(feirVar->Clone());
    auto base = std::variant<std::pair<UniqueFEIRVar, FieldID>, UniqueFEIRExpr>(arrayExpr->Clone());
    ProcessInitList(base, const_cast<ASTInitListExpr*>(this), stmts);
  } else if (initListType->IsStructType()) {
    auto base = std::variant<std::pair<UniqueFEIRVar, FieldID>, UniqueFEIRExpr>(std::make_pair(feirVar->Clone(), 0));
    ProcessInitList(base, const_cast<ASTInitListExpr*>(this), stmts);
  } else if (isTransparent) {
    CHECK_FATAL(initExprs.size() == 1, "Transparent init list size must be 1");
    return initExprs[0]->Emit2FEExpr(stmts);
  } else if (hasVectorType) {
    auto base = std::variant<std::pair<UniqueFEIRVar, FieldID>, UniqueFEIRExpr>(std::make_pair(feirVar->Clone(), 0));
    ProcessInitList(base, const_cast<ASTInitListExpr*>(this), stmts);
  } else {
    CHECK_FATAL(true, "Unsupported init list type");
  }
  return nullptr;
}

void ASTInitListExpr::ProcessInitList(std::variant<std::pair<UniqueFEIRVar, FieldID>, UniqueFEIRExpr> &base,
                                      ASTInitListExpr *initList,
                                      std::list<UniqueFEIRStmt> &stmts) const {
  if (initList->initListType->GetKind() == kTypeArray) {
    if (std::holds_alternative<UniqueFEIRExpr>(base)) {
      ProcessArrayInitList(std::get<UniqueFEIRExpr>(base)->Clone(), initList, stmts);
    } else {
      auto addrExpr = std::make_unique<FEIRExprAddrofVar>(
          std::get<std::pair<UniqueFEIRVar, FieldID>>(base).first->Clone());
      addrExpr->SetFieldID(std::get<std::pair<UniqueFEIRVar, FieldID>>(base).second);
      ProcessArrayInitList(addrExpr->Clone(), initList, stmts);
    }
  } else if (initList->initListType->GetKind() == kTypeStruct || initList->initListType->GetKind() == kTypeUnion) {
    ProcessStructInitList(base, initList, stmts);
  } else if (initList->isTransparent) {
    CHECK_FATAL(initList->initExprs.size() == 1, "Transparent init list size must be 1");
    auto feExpr = initList->initExprs[0]->Emit2FEExpr(stmts);
    MIRType *retType = initList->initListType;
    MIRType *retPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*retType);
    UniqueFEIRType fePtrType = std::make_unique<FEIRTypeNative>(*retPtrType);
    if (std::holds_alternative<UniqueFEIRExpr>(base)) {
      auto stmt = FEIRBuilder::CreateStmtIAssign(fePtrType->Clone(), std::get<UniqueFEIRExpr>(base)->Clone(),
                                                 feExpr->Clone(), 0);
      stmts.emplace_back(std::move(stmt));
    } else {
      UniqueFEIRVar feirVar = std::get<std::pair<UniqueFEIRVar, FieldID>>(base).first->Clone();
      FieldID fieldID = std::get<std::pair<UniqueFEIRVar, FieldID>>(base).second;
      auto stmt = FEIRBuilder::CreateStmtDAssignAggField(feirVar->Clone(), feExpr->Clone(), fieldID);
      stmts.emplace_back(std::move(stmt));
    }
  } else if (initList->HasVectorType()) {
    ProcessVectorInitList(base, initList, stmts);
  }
}

void ASTInitListExpr::ProcessStringLiteralInitList(const UniqueFEIRExpr &addrOfCharArray,
                                                   const UniqueFEIRExpr &addrOfStringLiteral,
                                                   size_t stringLength, std::list<UniqueFEIRStmt> &stmts) const {
  std::unique_ptr<std::list<UniqueFEIRExpr>> argExprList = std::make_unique<std::list<UniqueFEIRExpr>>();
  argExprList->emplace_back(addrOfCharArray->Clone());
  argExprList->emplace_back(addrOfStringLiteral->Clone());
  CHECK_FATAL(stringLength <= INT_MAX, "Too large length range");
  argExprList->emplace_back(FEIRBuilder::CreateExprConstI32(static_cast<int32>(stringLength)));
  std::unique_ptr<FEIRStmtIntrinsicCallAssign> memcpyStmt = std::make_unique<FEIRStmtIntrinsicCallAssign>(
      INTRN_C_memcpy, nullptr, nullptr, std::move(argExprList));
  stmts.emplace_back(std::move(memcpyStmt));

  // Handling Implicit Initialization When Incomplete Initialization
  if (addrOfCharArray->GetKind() != kExprAddrofArray) {
    return;
  }
  auto type = static_cast<FEIRExprAddrofArray*>(addrOfCharArray.get())->GetTypeArray()->Clone();
  MIRType *mirType = type->GenerateMIRType();
  if (mirType->GetKind() == kTypeArray) {
    MIRArrayType *arrayType = static_cast<MIRArrayType*>(mirType);
    if (arrayType->GetDim() != 2) { // only processing two-dimensional arrays.
      return;
    }
    uint32 dimSize = arrayType->GetSizeArrayItem(1);
    uint32 elemSize = arrayType->GetElemType()->GetSize();
    ProcessImplicitInit(addrOfCharArray->Clone(), stringLength, dimSize, elemSize, stmts);
  }
}

void ASTInitListExpr::ProcessImplicitInit(const UniqueFEIRExpr &addrExpr, uint32 initSize,
                                          uint32 total, uint32 elemSize, std::list<UniqueFEIRStmt> &stmts) const {
  if (initSize >= total) {
    return;
  }
  std::unique_ptr<std::list<UniqueFEIRExpr>> argExprList = std::make_unique<std::list<UniqueFEIRExpr>>();
  UniqueFEIRExpr realAddr = addrExpr->Clone();
  CHECK_FATAL(elemSize <= INT_MAX, "Too large elem size");
  CHECK_FATAL(initSize <= INT_MAX, "Too large init size");
  UniqueFEIRExpr elemSizeExpr = FEIRBuilder::CreateExprConstI32(static_cast<int32>(elemSize));
  if (initSize != 0) {
    UniqueFEIRExpr initSizeExpr = FEIRBuilder::CreateExprConstI32(static_cast<int32>(initSize));
    if (elemSize != 1) {
      initSizeExpr = FEIRBuilder::CreateExprBinary(OP_mul, std::move(initSizeExpr), elemSizeExpr->Clone());
    }
    realAddr = FEIRBuilder::CreateExprBinary(OP_add, std::move(realAddr), initSizeExpr->Clone());
  }
  argExprList->emplace_back(std::move(realAddr));
  argExprList->emplace_back(FEIRBuilder::CreateExprConstI32(0));
  UniqueFEIRExpr cntExpr = FEIRBuilder::CreateExprConstI32(static_cast<int32>(total - initSize));
  if (elemSize != 1) {
    cntExpr = FEIRBuilder::CreateExprBinary(OP_mul, std::move(cntExpr), elemSizeExpr->Clone());
  }
  argExprList->emplace_back(std::move(cntExpr));
  std::unique_ptr<FEIRStmtIntrinsicCallAssign> memsetStmt = std::make_unique<FEIRStmtIntrinsicCallAssign>(
      INTRN_C_memset, nullptr, nullptr, std::move(argExprList));
  stmts.emplace_back(std::move(memsetStmt));
}

void ASTInitListExpr::ProcessDesignatedInitUpdater(
    std::variant<std::pair<UniqueFEIRVar, FieldID>, UniqueFEIRExpr> &base,
    ASTExpr *expr, std::list<UniqueFEIRStmt> &stmts) const {
  auto designatedInitUpdateExpr = static_cast<ASTDesignatedInitUpdateExpr*>(expr);
  ASTExpr *baseExpr = designatedInitUpdateExpr->GetBaseExpr();
  ASTExpr *updaterExpr = designatedInitUpdateExpr->GetUpdaterExpr();
  auto feExpr = baseExpr->Emit2FEExpr(stmts);
  if (std::holds_alternative<UniqueFEIRExpr>(base)) {
    MIRType *mirType = designatedInitUpdateExpr->GetInitListType();
    MIRType *mirPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*mirType);
    UniqueFEIRType fePtrType = std::make_unique<FEIRTypeNative>(*mirPtrType);
    auto stmt = FEIRBuilder::CreateStmtIAssign(fePtrType->Clone(), std::get<UniqueFEIRExpr>(base)->Clone(),
                                               feExpr->Clone(), 0);
    stmts.emplace_back(std::move(stmt));
  } else {
    UniqueFEIRVar feirVar = std::get<std::pair<UniqueFEIRVar, FieldID>>(base).first->Clone();
    FieldID fieldID = std::get<std::pair<UniqueFEIRVar, FieldID>>(base).second;
    auto stmt = FEIRBuilder::CreateStmtDAssignAggField(feirVar->Clone(), feExpr->Clone(), fieldID);
    stmts.emplace_back(std::move(stmt));
  }
  ProcessInitList(base, static_cast<ASTInitListExpr*>(updaterExpr), stmts);
}

void ASTInitListExpr::ProcessStructInitList(std::variant<std::pair<UniqueFEIRVar, FieldID>, UniqueFEIRExpr> &base,
                                            ASTInitListExpr *initList,
                                            std::list<UniqueFEIRStmt> &stmts) const {
  MIRType *baseStructMirPtrType = nullptr;
  MIRStructType *baseStructMirType = nullptr;
  UniqueFEIRType baseStructFEType = nullptr;
  UniqueFEIRType baseStructFEPtrType = nullptr;
  MIRStructType *curStructMirType = static_cast<MIRStructType*>(initList->initListType);
  UniqueFEIRVar var;
  if (std::holds_alternative<UniqueFEIRExpr>(base)) {
    var = std::get<UniqueFEIRExpr>(base)->GetVarUses().front()->Clone();
    baseStructMirType = static_cast<MIRStructType*>(initList->initListType);
    baseStructMirPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*baseStructMirType);
    baseStructFEType = FEIRTypeHelper::CreateTypeNative(*baseStructMirType);
    baseStructFEPtrType = std::make_unique<FEIRTypeNative>(*baseStructMirPtrType);
  } else {
    var = std::get<std::pair<UniqueFEIRVar, FieldID>>(base).first->Clone();
    baseStructFEType = var->GetType()->Clone();
    baseStructMirType = static_cast<MIRStructType*>(baseStructFEType->GenerateMIRTypeAuto());
    baseStructMirPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*baseStructMirType);
    baseStructFEPtrType = std::make_unique<FEIRTypeNative>(*baseStructMirPtrType);
  }

  FieldID baseFieldID = 0;
  if (!std::holds_alternative<UniqueFEIRExpr>(base)) {
    baseFieldID = std::get<std::pair<UniqueFEIRVar, FieldID>>(base).second;
  }

  if (initList->initExprs.size() == 0) {
    UniqueFEIRExpr addrOfExpr = std::make_unique<FEIRExprAddrofVar>(var->Clone(), 0);
    ProcessImplicitInit(addrOfExpr->Clone(), 0, curStructMirType->GetSize(), 1, stmts);
    return;
  }

  uint32 curFieldTypeSize = 0;
  uint32 offset = 0;
  for (size_t i = 0; i < initList->initExprs.size(); ++i) {
    if (initList->initExprs[i] == nullptr) {
      continue; // skip anonymous field
    }

    FieldID curFieldID = 0;
    uint32 fieldIdx = (curStructMirType->GetKind() == kTypeUnion) ? initList->GetUnionInitFieldIdx() : i;
    FEUtils::TraverseToNamedField(*curStructMirType, curStructMirType->GetElemStrIdx(fieldIdx), curFieldID);
    uint32 fieldID = static_cast<uint32>(baseFieldID + curFieldID);
    MIRType *fieldMirType = curStructMirType->GetFieldType(curFieldID);
    curFieldTypeSize = static_cast<uint32>(fieldMirType->GetSize());
    offset += curFieldTypeSize;

    if (initList->initExprs[i]->GetASTOp() == kASTImplicitValueInitExpr && fieldMirType->GetPrimType() == PTY_agg) {
      UniqueFEIRExpr addrOfExpr;
      if (std::holds_alternative<UniqueFEIRExpr>(base)) {
        UniqueFEIRExpr offsetExpr = FEIRBuilder::CreateExprConstU32(offset - curFieldTypeSize);
        addrOfExpr = FEIRBuilder::CreateExprBinary(OP_add, std::get<UniqueFEIRExpr>(base)->Clone(),
                                                   std::move(offsetExpr));
      } else {
        addrOfExpr = std::make_unique<FEIRExprAddrofVar>(var->Clone(), fieldID);
      }
      ProcessImplicitInit(addrOfExpr->Clone(), 0, fieldMirType->GetSize(), 1, stmts);
      continue;
    }

    if (initList->initExprs[i]->GetASTOp() == kASTOpInitListExpr ||
        initList->initExprs[i]->GetASTOp() == kASTASTDesignatedInitUpdateExpr) {
      std::variant<std::pair<UniqueFEIRVar, FieldID>, UniqueFEIRExpr> subBase;
      if (std::holds_alternative<UniqueFEIRExpr>(base)) {
        auto addrOfElemExpr = std::make_unique<FEIRExprIAddrof>(baseStructFEPtrType->Clone(), fieldID,
                                                                std::get<UniqueFEIRExpr>(base)->Clone());
        subBase = std::variant<std::pair<UniqueFEIRVar, FieldID>, UniqueFEIRExpr>(addrOfElemExpr->Clone());
      } else {
        auto subVar = std::get<std::pair<UniqueFEIRVar, FieldID>>(base).first->Clone();
        subBase = std::variant<std::pair<UniqueFEIRVar, FieldID>, UniqueFEIRExpr>(
            std::make_pair<UniqueFEIRVar, FieldID>(subVar->Clone(), fieldID));
      }
      if (initList->initExprs[i]->GetASTOp() == kASTOpInitListExpr) {
        ProcessInitList(subBase, static_cast<ASTInitListExpr*>(initList->initExprs[i]), stmts);
      } else {
        ProcessDesignatedInitUpdater(subBase, static_cast<ASTInitListExpr*>(initList->initExprs[i]), stmts);
      }
    } else {
      auto elemExpr = initList->initExprs[i]->Emit2FEExpr(stmts);
      if (std::holds_alternative<UniqueFEIRExpr>(base)) {
        if (fieldMirType->GetKind() == kTypeArray && initList->initExprs[i]->GetASTOp() == kASTStringLiteral) {
          auto addrOfElement = std::make_unique<FEIRExprIAddrof>(baseStructFEPtrType->Clone(), fieldID,
                                                                 std::get<UniqueFEIRExpr>(base)->Clone());
          ProcessStringLiteralInitList(addrOfElement->Clone(), elemExpr->Clone(),
                                       static_cast<ASTStringLiteral *>(initList->initExprs[i])->GetLength(), stmts);
        } else {
          auto stmt = std::make_unique<FEIRStmtIAssign>(baseStructFEPtrType->Clone(),
                                                        std::get<UniqueFEIRExpr>(base)->Clone(),
                                                        elemExpr->Clone(),
                                                        fieldID);
          stmt->SetSrcFileInfo(initList->initExprs[i]->GetSrcFileIdx(), initList->initExprs[i]->GetSrcFileLineNum());
          stmts.emplace_back(std::move(stmt));
        }
      } else {
        auto subVar = std::get<std::pair<UniqueFEIRVar, FieldID>>(base).first->Clone();
        if (fieldMirType->GetKind() == kTypeArray && initList->initExprs[i]->GetASTOp() == kASTStringLiteral) {
          auto addrOfElement = std::make_unique<FEIRExprAddrofVar>(subVar->Clone());
          addrOfElement->SetFieldID(static_cast<FieldID>(fieldID));
          ProcessStringLiteralInitList(addrOfElement->Clone(), elemExpr->Clone(),
                                       static_cast<ASTStringLiteral *>(initList->initExprs[i])->GetLength(), stmts);
        } else {
          auto stmt = std::make_unique<FEIRStmtDAssign>(subVar->Clone(), elemExpr->Clone(), fieldID);
          stmt->SetSrcFileInfo(initList->initExprs[i]->GetSrcFileIdx(), initList->initExprs[i]->GetSrcFileLineNum());
          stmts.emplace_back(std::move(stmt));
        }
      }
    }
  }

  // Handling Incomplete Union Initialization
  if (curStructMirType->GetKind() == kTypeUnion) {
    UniqueFEIRExpr addrOfExpr = std::make_unique<FEIRExprAddrofVar>(var->Clone(), 0);
    ProcessImplicitInit(addrOfExpr->Clone(), curFieldTypeSize,
                        curStructMirType->GetSize(), 1, stmts);
  }
}

void ASTInitListExpr::ProcessArrayInitList(const UniqueFEIRExpr &addrOfArray, ASTInitListExpr *initList,
                                           std::list<UniqueFEIRStmt> &stmts) const {
  auto arrayMirType = static_cast<MIRArrayType*>(initList->initListType);
  UniqueFEIRType arrayFEType = FEIRTypeHelper::CreateTypeNative(*arrayMirType);
  MIRType *elementType;
  if (arrayMirType->GetDim() > 1) {
    uint32 subSizeArray[arrayMirType->GetDim()];
    for (int dim = 1; dim < arrayMirType->GetDim(); ++dim) {
      subSizeArray[dim - 1] = arrayMirType->GetSizeArrayItem(static_cast<uint32>(dim));
    }
    elementType = GlobalTables::GetTypeTable().GetOrCreateArrayType(*arrayMirType->GetElemType(),
        static_cast<uint8>(arrayMirType->GetDim() - 1), subSizeArray);
  } else {
    elementType = arrayMirType->GetElemType();
  }
  auto elementPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*elementType);
  auto elementPtrFEType = FEIRTypeHelper::CreateTypeNative(*elementPtrType);
  CHECK_FATAL(initExprs.size() <= INT_MAX, "invalid index");
  for (size_t i = 0; i < initList->initExprs.size(); ++i) {
    std::list<UniqueFEIRExpr> indexExprs;
    UniqueFEIRExpr indexExpr = FEIRBuilder::CreateExprConstI32(static_cast<int32>(i));
    indexExprs.emplace_back(std::move(indexExpr));
    auto addrOfElemExpr = FEIRBuilder::CreateExprAddrofArray(arrayFEType->Clone(), addrOfArray->Clone(), "",
                                                             indexExprs);
    if (initList->initExprs[i]->GetASTOp() == kASTOpInitListExpr) {
      auto base = std::variant<std::pair<UniqueFEIRVar, FieldID>, UniqueFEIRExpr>(addrOfElemExpr->Clone());
      ProcessInitList(base, static_cast<ASTInitListExpr*>(initList->initExprs[i]), stmts);
    } else {
      UniqueFEIRExpr elemExpr = initList->initExprs[i]->Emit2FEExpr(stmts);
      if (elementType->GetKind() == kTypeArray && initList->initExprs[i]->GetASTOp() == kASTStringLiteral) {
        ProcessStringLiteralInitList(addrOfElemExpr->Clone(), elemExpr->Clone(),
                                     static_cast<ASTStringLiteral *>(initList->initExprs[i])->GetLength(), stmts);
      } else {
        auto stmt = FEIRBuilder::CreateStmtIAssign(elementPtrFEType->Clone(), addrOfElemExpr->Clone(),
                                                   elemExpr->Clone(),
                                                   0);
        stmts.emplace_back(std::move(stmt));
      }
    }
  }

  // handling implicit initialization sections
  auto allSize = arrayMirType->GetSize();
  auto elemSize = elementType->GetSize();
  CHECK_FATAL(elemSize != 0, "elemSize should not 0");
  auto allElemCnt = allSize / elemSize;
  ProcessImplicitInit(addrOfArray->Clone(), initList->initExprs.size(), allElemCnt, elemSize, stmts);
}

void ASTInitListExpr::ProcessVectorInitList(std::variant<std::pair<UniqueFEIRVar, FieldID>, UniqueFEIRExpr> &base,
                                            ASTInitListExpr *initList, std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRType srcType = FEIRTypeHelper::CreateTypeNative(*initList->initListType);
  if (std::holds_alternative<UniqueFEIRExpr>(base)) {
    CHECK_FATAL(false, "unsupported case");
  } else {
    UniqueFEIRVar srcVar = std::get<std::pair<UniqueFEIRVar, FieldID>>(base).first->Clone();
    FieldID fieldID = std::get<std::pair<UniqueFEIRVar, FieldID>>(base).second;
    UniqueFEIRExpr dreadVar;
    if (fieldID != 0) {
      dreadVar = FEIRBuilder::CreateExprDReadAggField(srcVar->Clone(), fieldID, srcType->Clone());
    } else {
      dreadVar = FEIRBuilder::CreateExprDRead(srcVar->Clone());
    }
    for (size_t index = 0; index < initList->initExprs.size(); ++index) {
      UniqueFEIRExpr indexExpr = FEIRBuilder::CreateExprConstI32(index);
      UniqueFEIRExpr elemExpr = initList->initExprs[index]->Emit2FEExpr(stmts);
      std::vector<std::unique_ptr<FEIRExpr>> argOpnds;
      argOpnds.push_back(std::move(elemExpr));
      argOpnds.push_back(dreadVar->Clone());
      argOpnds.push_back(std::move(indexExpr));
      UniqueFEIRExpr intrinsicExpr = std::make_unique<FEIRExprIntrinsicopForC>(
          srcType->Clone(), SetVectorSetLane(*initList->initListType), argOpnds);
      auto stmt = FEIRBuilder::CreateStmtDAssignAggField(srcVar->Clone(), std::move(intrinsicExpr), fieldID);
      stmts.emplace_back(std::move(stmt));
    }
  }
}

MIRIntrinsicID ASTInitListExpr::SetVectorSetLane(const MIRType &type) const {
  MIRIntrinsicID Intrinsic;
  switch (type.GetPrimType()) {
#define SETQ_LANE(TY)                                                          \
  case PTY_##TY:                                                               \
    Intrinsic = INTRN_vector_set_element_##TY;                                 \
    break;

    SETQ_LANE(v2i64)
    SETQ_LANE(v4i32)
    SETQ_LANE(v8i16)
    SETQ_LANE(v16i8)
    SETQ_LANE(v2u64)
    SETQ_LANE(v4u32)
    SETQ_LANE(v8u16)
    SETQ_LANE(v16u8)
    SETQ_LANE(v2f64)
    SETQ_LANE(v4f32)
    SETQ_LANE(v2i32)
    SETQ_LANE(v4i16)
    SETQ_LANE(v8i8)
    SETQ_LANE(v2u32)
    SETQ_LANE(v4u16)
    SETQ_LANE(v8u8)
    SETQ_LANE(v2f32)
  case PTY_i64:
    Intrinsic = INTRN_vector_set_element_v1i64;
    break;
  case PTY_u64:
    Intrinsic = INTRN_vector_set_element_v1u64;
    break;
  case PTY_f64:
    Intrinsic = INTRN_vector_set_element_v1f64;
    break;
  default:
    CHECK_FATAL(false, "Unhandled vector type");
    return INTRN_UNDEFINED;
  }
  return Intrinsic;
}

void ASTInitListExpr::SetInitExprs(ASTExpr *astExpr) {
  initExprs.emplace_back(astExpr);
}

void ASTInitListExpr::SetInitListType(MIRType *type) {
  initListType = type;
}

// ---------- ASTImplicitValueInitExpr ----------
MIRConst *ASTImplicitValueInitExpr::GenerateMIRConstImpl() const {
  return FEUtils::CreateImplicitConst(mirType);
}

UniqueFEIRExpr ASTImplicitValueInitExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  return ImplicitInitFieldValue(mirType, stmts);
}

MIRConst *ASTStringLiteral::GenerateMIRConstImpl() const {
  auto *arrayType = static_cast<MIRArrayType*>(mirType);
  uint32 arraySize = arrayType->GetSizeArrayItem(0);
  auto elemType = arrayType->GetElemType();
  auto *val = FEManager::GetModule().GetMemPool()->New<MIRAggConst>(FEManager::GetModule(), *arrayType);
  for (uint32 i = 0; i < arraySize; ++i) {
    MIRConst *cst;
    if (i < codeUnits.size()) {
      cst = FEManager::GetModule().GetMemPool()->New<MIRIntConst>(codeUnits[i], *elemType);
    } else {
      cst = FEManager::GetModule().GetMemPool()->New<MIRIntConst>(0, *elemType);
    }
    val->PushBack(cst);
  }
  return val;
}

UniqueFEIRExpr ASTStringLiteral::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  (void)stmts;
  MIRType *elemType = static_cast<MIRArrayType*>(mirType)->GetElemType();
  UniqueFEIRExpr expr = std::make_unique<FEIRExprAddrofConstArray>(codeUnits, elemType);
  CHECK_NULL_FATAL(expr);
  return expr;
}

// ---------- ASTArraySubscriptExpr ----------
size_t ASTArraySubscriptExpr::CalculateOffset() const {
  size_t offset = 0;
  CHECK_FATAL(idxExpr->GetConstantValue() != nullptr, "Not constant value for constant initializer");
  offset += mirType->GetSize() * static_cast<std::size_t>(idxExpr->GetConstantValue()->val.i64);
  return offset;
}

ASTExpr *ASTArraySubscriptExpr::FindFinalBase() const {
  if (baseExpr->GetASTOp() == kASTSubscriptExpr) {
    return static_cast<ASTArraySubscriptExpr*>(baseExpr)->FindFinalBase();
  }
  return baseExpr;
}

MIRConst *ASTArraySubscriptExpr::GenerateMIRConstImpl() const {
  size_t offset = CalculateOffset();
  const ASTExpr *base = FindFinalBase();
  MIRConst *baseConst = base->GenerateMIRConst();
  if (baseConst->GetKind() == kConstStrConst) {
    MIRStrConst *strConst = static_cast<MIRStrConst*>(baseConst);
    std::string str = GlobalTables::GetUStrTable().GetStringFromStrIdx(strConst->GetValue());
    CHECK_FATAL(str.length() >= offset, "Invalid operation");
    str = str.substr(offset);
    UStrIdx strIdx = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(str);
    return FEManager::GetModule().GetMemPool()->New<MIRStrConst>(
        strIdx, *GlobalTables::GetTypeTable().GetPrimType(PTY_a64));
  } else if (baseConst->GetKind() == kConstAddrof) {
    MIRAddrofConst *konst = static_cast<MIRAddrofConst*>(baseConst);
    CHECK_FATAL(offset <= INT_MAX, "Invalid operation");
    return FEManager::GetModule().GetMemPool()->New<MIRAddrofConst>(konst->GetSymbolIndex(), konst->GetFieldID(),
        konst->GetType(), konst->GetOffset() + static_cast<int32>(offset));
  } else {
    CHECK_FATAL(false, "Unsupported MIRConst: %d", baseConst->GetKind());
  }
}

bool ASTArraySubscriptExpr::CheckFirstDimIfZero(const MIRType *arrayType) const {
  auto tmpArrayType = static_cast<const MIRArrayType*>(arrayType);
  uint32 size = tmpArrayType->GetSizeArrayItem(0);
  uint32 oriDim = tmpArrayType->GetDim();
  if (size == 0 && oriDim >= 2) { // 2 is the array dim
    return true;
  }
  return false;
}

void ASTArraySubscriptExpr::InsertNonnullChecking(std::list<UniqueFEIRStmt> &stmts, const UniqueFEIRExpr &indexExpr,
                                                  const UniqueFEIRExpr &baseAddrExpr) const {
  if (!FEOptions::GetInstance().IsNpeCheckDynamic() || indexExpr->GetKind() != kExprConst) {
    return;
  }
  if (FEIRBuilder::IsZeroConstExpr(indexExpr)) {  // insert nonnull checking when ptr[0]
    UniqueFEIRStmt stmt = std::make_unique<FEIRStmtAssertNonnull>(OP_assertnonnull, baseAddrExpr->Clone());
    stmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
    stmts.emplace_back(std::move(stmt));
  }
}

UniqueFEIRExpr ASTArraySubscriptExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  auto baseAddrFEExpr = baseExpr->Emit2FEExpr(stmts);
  auto retFEType = std::make_unique<FEIRTypeNative>(*mirType);
  MIRType *arrayTypeOpt = arrayType;
  bool isArrayTypeOpt = false;
  if (arrayTypeOpt->GetKind() == kTypePointer && !isVLA) {
    MIRPtrType *ptrTy = static_cast<MIRPtrType*>(arrayTypeOpt);
    MIRType *pointedTy = ptrTy->GetPointedType();
    if (pointedTy->GetKind() == kTypeArray) {
      MIRArrayType *pointedArrTy = static_cast<MIRArrayType*>(pointedTy);
      std::vector<uint32> sizeArray{1};
      for (uint32 i = 0; i < pointedArrTy->GetDim(); ++i) {
        sizeArray.push_back(pointedArrTy->GetSizeArrayItem(i));
      }
      MIRArrayType newArrTy(pointedArrTy->GetElemTyIdx(), sizeArray);
      arrayTypeOpt = static_cast<MIRArrayType*>(GlobalTables::GetTypeTable().GetOrCreateMIRTypeNode(newArrTy));
    } else {
      arrayTypeOpt = GlobalTables::GetTypeTable().GetOrCreateArrayType(*pointedTy, 1);
    }
    isArrayTypeOpt = true;
  }
  UniqueFEIRType arrayFEType = std::make_unique<FEIRTypeNative>(*arrayTypeOpt);
  auto mirPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*mirType);
  auto fePtrType = std::make_unique<FEIRTypeNative>(*mirPtrType);
  UniqueFEIRExpr addrOfArray;
  if (arrayTypeOpt->GetKind() == MIRTypeKind::kTypeArray && !isVLA) {
    if(CheckFirstDimIfZero(arrayTypeOpt)) {
      // return multi-dim array addr directly if its first dim size was 0.
      return baseAddrFEExpr;
    }
    std::list<UniqueFEIRExpr> feIdxExprs;
    if (baseAddrFEExpr->GetKind() == kExprAddrofArray && !isArrayTypeOpt) {
      auto baseArrayExpr = static_cast<FEIRExprAddrofArray*>(baseAddrFEExpr.get());
      for (auto &e : baseArrayExpr->GetExprIndexs()) {
        feIdxExprs.emplace_back(e->Clone());
      }
      arrayFEType = baseArrayExpr->GetTypeArray()->Clone();
      baseAddrFEExpr = baseArrayExpr->GetExprArray()->Clone();
    }
    auto feIdxExpr = idxExpr->Emit2FEExpr(stmts);
    if (isArrayTypeOpt) {
      InsertNonnullChecking(stmts, feIdxExpr, baseAddrFEExpr);
    }
    feIdxExprs.emplace_back(std::move(feIdxExpr));
    addrOfArray = FEIRBuilder::CreateExprAddrofArray(arrayFEType->Clone(), baseAddrFEExpr->Clone(), "", feIdxExprs);
  } else {
    std::vector<UniqueFEIRExpr> offsetExprs;
    UniqueFEIRExpr offsetExpr;
    auto feIdxExpr = idxExpr->Emit2FEExpr(stmts);
    PrimType indexPty = feIdxExpr->GetPrimType();
    UniqueFEIRType sizeType;
    if (IsSignedInteger(indexPty)) {
      sizeType = std::make_unique<FEIRTypeNative>(*GlobalTables::GetTypeTable().GetPrimType(PTY_i64));
      feIdxExpr = FEIRBuilder::CreateExprCvtPrim(std::move(feIdxExpr), GetRegPrimType(indexPty), PTY_i64);
    } else {
      sizeType = std::make_unique<FEIRTypeNative>(*GlobalTables::GetTypeTable().GetPrimType(PTY_ptr));
      feIdxExpr = FEIRBuilder::CreateExprCvtPrim(std::move(feIdxExpr),
                                                 GetRegPrimType(indexPty), PTY_ptr);
    }
    if (mirType->GetSize() != 1) {
      auto typeSizeExpr = std::make_unique<FEIRExprConst>(mirType->GetSize(), sizeType->GetPrimType());
      feIdxExpr = FEIRBuilder::CreateExprBinary(sizeType->Clone(), OP_mul, std::move(feIdxExpr),
                                                std::move(typeSizeExpr));
    }
    offsetExprs.emplace_back(std::move(feIdxExpr));
    if (offsetExprs.size() == 1) {
      offsetExpr = std::move(offsetExprs[0]);
    } else if (offsetExprs.size() >= 2) {
      offsetExpr = FEIRBuilder::CreateExprBinary(sizeType->Clone(), OP_add, std::move(offsetExprs[0]),
                                                 std::move(offsetExprs[1]));
      if (offsetExprs.size() >= 3) {
        for (size_t i = 2; i < offsetExprs.size(); i++) {
          offsetExpr = FEIRBuilder::CreateExprBinary(sizeType->Clone(), OP_add, std::move(offsetExpr),
                                                     std::move(offsetExprs[i]));
        }
      }
    }
    if (isVLA) {
      baseAddrFEExpr = FEIRBuilder::CreateExprDRead(baseAddrFEExpr->GetVarUses().front()->Clone());
    }
    addrOfArray = FEIRBuilder::CreateExprBinary(std::move(sizeType), OP_add, baseAddrFEExpr->Clone(),
                                                std::move(offsetExpr));
  }
  if (InsertBoundaryChecking(stmts, addrOfArray->Clone(), std::move(baseAddrFEExpr))) {
    addrOfArray->SetIsBoundaryChecking(true);
  }
  return FEIRBuilder::CreateExprIRead(std::move(retFEType), fePtrType->Clone(), addrOfArray->Clone());
}

UniqueFEIRExpr ASTExprUnaryExprOrTypeTraitExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  (void)stmts;
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

MIRConst *ASTMemberExpr::GenerateMIRConstImpl() const {
  uint64 fieldOffset = fieldOffsetBits / kOneByte;
  ASTExpr *base = baseExpr;
  while (base->GetASTOp() == kASTMemberExpr) {  // find final BaseExpr and calculate FieldOffsets
    ASTMemberExpr *memberExpr = static_cast<ASTMemberExpr*>(base);
    fieldOffset += memberExpr->GetFieldOffsetBits() / kOneByte;
    base = memberExpr->GetBaseExpr();
  }
  MIRConst *baseConst = base->GenerateMIRConst();
  MIRAddrofConst *konst = nullptr;
  if (baseConst->GetKind() == kConstAddrof) {
    konst = static_cast<MIRAddrofConst*>(baseConst);
  } else if (baseConst->GetKind() == kConstInt) {
    return FEManager::GetModule().GetMemPool()->New<MIRIntConst>(static_cast<int64>(fieldOffset),
                                                                 *GlobalTables::GetTypeTable().GetInt64());
  } else {
    CHECK_FATAL(false, "base const kind NYI.");
  }
  MIRType *baseStructType =
      base->GetType()->IsMIRPtrType() ? static_cast<MIRPtrType*>(base->GetType())->GetPointedType() :
      base->GetType();
  CHECK_FATAL(baseStructType->IsMIRStructType() || baseStructType->GetKind() == kTypeUnion, "Invalid");
  return FEManager::GetModule().GetMemPool()->New<MIRAddrofConst>(konst->GetSymbolIndex(), konst->GetFieldID(),
      konst->GetType(), konst->GetOffset() + static_cast<int32>(fieldOffset));
}

const ASTMemberExpr *ASTMemberExpr::FindFinalMember(const ASTMemberExpr *startExpr,
                                                    std::list<std::string> &memberNames) const {
  memberNames.emplace_back(startExpr->memberName);
  if (startExpr->isArrow || startExpr->baseExpr->GetASTOp() != kASTMemberExpr) {
    return startExpr;
  }
  return FindFinalMember(static_cast<ASTMemberExpr*>(startExpr->baseExpr), memberNames);
}

void ASTMemberExpr::InsertNonnullChecking(std::list<UniqueFEIRStmt> &stmts, UniqueFEIRExpr baseFEExpr) const {
  if (!FEOptions::GetInstance().IsNpeCheckDynamic()) {
    return;
  }
  if (baseFEExpr->GetPrimType() == PTY_ptr) {
    UniqueFEIRStmt stmt = std::make_unique<FEIRStmtAssertNonnull>(OP_assertnonnull, std::move(baseFEExpr));
    stmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
    stmts.emplace_back(std::move(stmt));
  }
}

UniqueFEIRExpr ASTMemberExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRExpr baseFEExpr;
  std::string fieldName = memberName;
  bool tmpIsArrow = this->isArrow;
  MIRType *tmpBaseType = this->baseType;
  if (baseExpr->GetASTOp() == kASTMemberExpr) {
    std::list<std::string> memberNameList;
    memberNameList.emplace_back(memberName);
    const ASTMemberExpr *finalMember = FindFinalMember(static_cast<ASTMemberExpr*>(baseExpr), memberNameList);
    baseFEExpr = finalMember->baseExpr->Emit2FEExpr(stmts);
    tmpIsArrow = finalMember->isArrow;
    tmpBaseType = finalMember->baseType;
    fieldName = ASTUtil::Join(memberNameList, ".");
  } else {
    baseFEExpr = baseExpr->Emit2FEExpr(stmts);
  }
  UniqueFEIRType baseFEType = std::make_unique<FEIRTypeNative>(*tmpBaseType);
  if (tmpIsArrow) {
    CHECK_FATAL(tmpBaseType->IsMIRPtrType(), "Must be ptr type!");
    MIRPtrType *mirPtrType = static_cast<MIRPtrType*>(tmpBaseType);
    MIRType *pointedMirType = mirPtrType->GetPointedType();
    CHECK_FATAL(pointedMirType->IsStructType(), "pointedMirType must be StructType");
    MIRStructType *structType = static_cast<MIRStructType*>(pointedMirType);
    FieldID fieldID = FEUtils::GetStructFieldID(structType, fieldName);
    MIRType *reType = FEUtils::GetStructFieldType(structType, fieldID);
    CHECK_FATAL(reType->GetPrimType() == memberType->GetPrimType(), "traverse fieldID error, type is inconsistent");
    UniqueFEIRType retFEType = std::make_unique<FEIRTypeNative>(*reType);
    if (retFEType->IsArray()) {
      return std::make_unique<FEIRExprIAddrof>(std::move(baseFEType), fieldID, std::move(baseFEExpr));
    } else {
      InsertNonnullChecking(stmts, baseFEExpr->Clone());
      return FEIRBuilder::CreateExprIRead(std::move(retFEType), std::move(baseFEType), std::move(baseFEExpr), fieldID);
    }
  } else {
    CHECK_FATAL(tmpBaseType->IsStructType(), "basetype must be StructType");
    MIRStructType *structType = static_cast<MIRStructType*>(tmpBaseType);
    FieldID fieldID = FEUtils::GetStructFieldID(structType, fieldName);
    MIRType *reType = FEUtils::GetStructFieldType(structType, fieldID);
    CHECK_FATAL(reType->GetPrimType() == memberType->GetPrimType(), "traverse fieldID error, type is inconsistent");
    UniqueFEIRType reFEType = std::make_unique<FEIRTypeNative>(*reType);
    FieldID baseID = baseFEExpr->GetFieldID();
    if (baseFEExpr->GetKind() == FEIRNodeKind::kExprIRead) {
      baseFEExpr->SetFieldID(baseID + fieldID);
      baseFEExpr->SetType(std::move(reFEType));
      return baseFEExpr;
    }
    UniqueFEIRVar tmpVar = static_cast<FEIRExprDRead*>(baseFEExpr.get())->GetVar()->Clone();
    if (reFEType->IsArray()) {
      auto addrofExpr = std::make_unique<FEIRExprAddrofVar>(std::move(tmpVar));
      addrofExpr->SetFieldID(baseID + fieldID);
      return addrofExpr;
    } else {
      return FEIRBuilder::CreateExprDReadAggField(std::move(tmpVar), baseID + fieldID, std::move(reFEType));
    }
  }
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

MIRConst *ASTBinaryOperatorExpr::GenerateMIRConstImpl() const {
  MIRConst *leftConst = leftExpr->GenerateMIRConst();
  MIRConst *rightConst = nullptr;
  if (opcode == OP_lior || opcode == OP_cior) {
    if (!leftConst->IsZero()) {
      return GlobalTables::GetIntConstTable().GetOrCreateIntConst(1,
          *GlobalTables::GetTypeTable().GetPrimType(PTY_i32));
    } else {
      rightConst = rightExpr->GenerateMIRConst();
      if (!rightConst->IsZero()) {
        return GlobalTables::GetIntConstTable().GetOrCreateIntConst(1,
            *GlobalTables::GetTypeTable().GetPrimType(PTY_i32));
      } else {
        return GlobalTables::GetIntConstTable().GetOrCreateIntConst(0,
            *GlobalTables::GetTypeTable().GetPrimType(PTY_i32));
      }
    }
  }
  rightConst = rightExpr->GenerateMIRConst();
  if (leftConst->GetKind() == kConstLblConst || rightConst->GetKind() == kConstLblConst) {
    // init by initListExpr, Only MIRConst kind is set here.
    return leftConst->GetKind() == kConstLblConst ? leftConst : rightConst;
  }
  if (opcode == OP_land || opcode == OP_cand) {
    if (leftConst->IsZero()) {
      return GlobalTables::GetIntConstTable().GetOrCreateIntConst(0,
          *GlobalTables::GetTypeTable().GetPrimType(PTY_i32));
    } else if (rightConst->IsZero()) {
      return GlobalTables::GetIntConstTable().GetOrCreateIntConst(0,
          *GlobalTables::GetTypeTable().GetPrimType(PTY_i32));
    } else {
      return GlobalTables::GetIntConstTable().GetOrCreateIntConst(1,
          *GlobalTables::GetTypeTable().GetPrimType(PTY_i32));
    }
  }
  if (leftConst->GetKind() == rightConst->GetKind()) {
    if (isConstantFolded) {
      return value->Translate2MIRConst();
    }
    switch (leftConst->GetKind()) {
      case kConstInt: {
        return MIRConstGenerator(FEManager::GetModule().GetMemPool(), static_cast<MIRIntConst*>(leftConst),
                                 static_cast<MIRIntConst*>(rightConst), opcode);
      }
      case kConstFloatConst: {
        return MIRConstGenerator(FEManager::GetModule().GetMemPool(), static_cast<MIRFloatConst*>(leftConst),
                                 static_cast<MIRFloatConst*>(rightConst), opcode);
      }
      case kConstDoubleConst: {
        return MIRConstGenerator(FEManager::GetModule().GetMemPool(), static_cast<MIRDoubleConst*>(leftConst),
                                 static_cast<MIRDoubleConst*>(rightConst), opcode);
      }
      default: {
        CHECK_FATAL(false, "Unsupported yet");
        return nullptr;
      }
    }
  }
  if (opcode == OP_add) {
    MIRIntConst *constInt = nullptr;
    MIRConst *baseConst = nullptr;
    if (leftConst->GetKind() == kConstInt) {
      constInt = static_cast<MIRIntConst*>(leftConst);
      baseConst = rightConst;
    } else if (rightConst->GetKind() == kConstInt) {
      constInt = static_cast<MIRIntConst*>(rightConst);
      baseConst = leftConst;
    } else {
      CHECK_FATAL(false, "Unsupported yet");
    }
    int64 value = constInt->GetValue();
    if (baseConst->GetKind() == kConstStrConst) {
      std::string str =
          GlobalTables::GetUStrTable().GetStringFromStrIdx(static_cast<MIRStrConst*>(baseConst)->GetValue());
      CHECK_FATAL(str.length() >= static_cast<std::size_t>(value), "Invalid operation");
      str = str.substr(static_cast<std::size_t>(value));
      UStrIdx strIdx = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(str);
      return FEManager::GetModule().GetMemPool()->New<MIRStrConst>(
          strIdx, *GlobalTables::GetTypeTable().GetPrimType(PTY_a64));
    } else if (baseConst->GetKind() == kConstAddrof) {
      MIRAddrofConst *konst = static_cast<MIRAddrofConst*>(baseConst);
      auto idx = konst->GetSymbolIndex();
      auto id = konst->GetFieldID();
      auto ty = konst->GetType();
      auto offset = konst->GetOffset();
      return FEManager::GetModule().GetMemPool()->New<MIRAddrofConst>(idx, id, ty, offset + value);
    } else {
      CHECK_FATAL(false, "NIY");
    }
  } else if (opcode == OP_sub) {
    CHECK_FATAL(leftConst->GetKind() == kConstAddrof && rightConst->GetKind() == kConstInt, "Unsupported");
    MIRAddrofConst *konst = static_cast<MIRAddrofConst*>(leftConst);
    auto idx = konst->GetSymbolIndex();
    auto id = konst->GetFieldID();
    auto ty = konst->GetType();
    auto offset = konst->GetOffset();
    int64 value = static_cast<MIRIntConst*>(rightConst)->GetValue();
    return FEManager::GetModule().GetMemPool()->New<MIRAddrofConst>(idx, id, ty, offset - value);
  } else {
    CHECK_FATAL(false, "NIY");
  }
  return nullptr;
}

UniqueFEIRType ASTBinaryOperatorExpr::SelectBinaryOperatorType(UniqueFEIRExpr &left, UniqueFEIRExpr &right) const {
  // For arithmetical calculation only
  std::map<PrimType, uint8> binaryTypePriority = {
    {PTY_u1, 0},
    {PTY_i8, 1},
    {PTY_u8, 2},
    {PTY_i16, 3},
    {PTY_u16, 4},
    {PTY_i32, 5},
    {PTY_u32, 6},
    {PTY_i64, 7},
    {PTY_u64, 8},
    {PTY_f32, 9},
    {PTY_f64, 10}
  };
  UniqueFEIRType feirType = std::make_unique<FEIRTypeNative>(*retType);
  if (!cvtNeeded) {
    return feirType;
  }
  if (binaryTypePriority.find(left->GetPrimType()) == binaryTypePriority.end() ||
      binaryTypePriority.find(right->GetPrimType()) == binaryTypePriority.end()) {
    if (left->GetPrimType() != feirType->GetPrimType()) {
      left = FEIRBuilder::CreateExprCastPrim(std::move(left), feirType->GetPrimType());
    }
    if (right->GetPrimType() != feirType->GetPrimType()) {
      right = FEIRBuilder::CreateExprCastPrim(std::move(right), feirType->GetPrimType());
    }
    return feirType;
  }
  MIRType *dstType;
  if (binaryTypePriority[left->GetPrimType()] > binaryTypePriority[right->GetPrimType()]) {
    right = FEIRBuilder::CreateExprCastPrim(std::move(right), left->GetPrimType());
    dstType = left->GetType()->GenerateMIRTypeAuto();
  } else {
    left = FEIRBuilder::CreateExprCastPrim(std::move(left), right->GetPrimType());
    dstType = right->GetType()->GenerateMIRTypeAuto();
  }
  return std::make_unique<FEIRTypeNative>(*dstType);
}

UniqueFEIRExpr ASTBinaryOperatorExpr::Emit2FEExprComplexCalculations(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRVar tempVar = FEIRBuilder::CreateVarNameForC(FEUtils::GetSequentialName("Complex_"), *retType);
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
}

UniqueFEIRExpr ASTBinaryOperatorExpr::Emit2FEExprComplexCompare(std::list<UniqueFEIRStmt> &stmts) const {
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
}

UniqueFEIRExpr ASTBinaryOperatorExpr::Emit2FEExprLogicOperate(std::list<UniqueFEIRStmt> &stmts) const {
  bool inShortCircuit = true;
  uint32 trueLabelIdx = trueIdx;
  uint32 falseLabelIdx = falseIdx;
  uint32 fallthrouLabelIdx, jumpToLabelIdx;
  uint32 rightCondLabelIdx = FEUtils::GetSequentialNumber();
  MIRType *tempVarType = GlobalTables::GetTypeTable().GetUInt32();
  UniqueFEIRVar shortCircuit = FEIRBuilder::CreateVarNameForC(varName, *tempVarType);

  // check short circuit boundary
  if (trueLabelIdx == 0) {
    trueLabelIdx = FEUtils::GetSequentialNumber();
    falseLabelIdx = FEUtils::GetSequentialNumber();
    inShortCircuit = false;
  }

  Opcode op = opcode == OP_cior ? OP_brtrue : OP_brfalse;
  if (op == OP_brtrue) {
    fallthrouLabelIdx = falseLabelIdx;
    jumpToLabelIdx = trueLabelIdx;
    leftExpr->SetShortCircuitIdx(jumpToLabelIdx, rightCondLabelIdx);
  } else {
    fallthrouLabelIdx = trueLabelIdx;
    jumpToLabelIdx = falseLabelIdx;
    leftExpr->SetShortCircuitIdx(rightCondLabelIdx, jumpToLabelIdx);
  }
  rightExpr->SetShortCircuitIdx(trueLabelIdx, falseLabelIdx);

  std::string rightCondLabel = FEUtils::GetSequentialName0("shortCircuit_", rightCondLabelIdx);
  std::string fallthrouLabel = FEUtils::GetSequentialName0("shortCircuit_", fallthrouLabelIdx);
  std::string jumpToLabel = FEUtils::GetSequentialName0("shortCircuit_", jumpToLabelIdx);

  // brfalse/brtrue label (leftCond)
  auto leftFEExpr = leftExpr->Emit2FEExpr(stmts);
  if (leftFEExpr != nullptr) {
    auto leftCond = FEIRBuilder::CreateExprZeroCompare(OP_ne, std::move(leftFEExpr));
    UniqueFEIRStmt leftCondGoToExpr = std::make_unique<FEIRStmtCondGotoForC>(leftCond->Clone(), op, jumpToLabel);
    stmts.emplace_back(std::move(leftCondGoToExpr));
  }

  auto rightCondlabelStmt = std::make_unique<FEIRStmtLabel>(rightCondLabel);
  rightCondlabelStmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
  stmts.emplace_back(std::move(rightCondlabelStmt));

  // brfalse/brtrue label (rightCond)
  auto rightFEExpr = rightExpr->Emit2FEExpr(stmts);
  if (rightFEExpr != nullptr) {
    auto rightCond = FEIRBuilder::CreateExprZeroCompare(OP_ne, std::move(rightFEExpr));
    UniqueFEIRStmt rightCondGoToExpr = std::make_unique<FEIRStmtCondGotoForC>(rightCond->Clone(), op, jumpToLabel);
    UniqueFEIRStmt goStmt = FEIRBuilder::CreateStmtGoto(fallthrouLabel);
    stmts.emplace_back(std::move(rightCondGoToExpr));
    stmts.emplace_back(std::move(goStmt));
  }

  UniqueFEIRExpr returnValue(nullptr);
  // when reaching the outer layer of a short circuit, return explicit value for each branch
  if (!inShortCircuit) {
    std::string nextLabel = FEUtils::GetSequentialName("shortCircuit_");
    UniqueFEIRExpr trueConst = FEIRBuilder::CreateExprConstAnyScalar(PTY_u32, 1);
    UniqueFEIRExpr falseConst = FEIRBuilder::CreateExprConstAnyScalar(PTY_u32, 0);
    UniqueFEIRStmt goStmt = FEIRBuilder::CreateStmtGoto(nextLabel);
    auto trueCircuit = std::make_unique<FEIRStmtDAssign>(shortCircuit->Clone(), std::move(trueConst), 0);
    auto falseCircuit = std::make_unique<FEIRStmtDAssign>(shortCircuit->Clone(), std::move(falseConst), 0);
    auto labelFallthrouStmt = std::make_unique<FEIRStmtLabel>(fallthrouLabel);
    auto labelJumpToStmt = std::make_unique<FEIRStmtLabel>(jumpToLabel);
    auto labelNextStmt = std::make_unique<FEIRStmtLabel>(nextLabel);
    labelJumpToStmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
    labelNextStmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
    stmts.emplace_back(std::move(labelFallthrouStmt));
    stmts.emplace_back(op == OP_brtrue ? std::move(falseCircuit) : std::move(trueCircuit));
    stmts.emplace_back(std::move(goStmt));
    stmts.emplace_back(std::move(labelJumpToStmt));
    stmts.emplace_back(op == OP_brtrue ? std::move(trueCircuit) : std::move(falseCircuit));
    stmts.emplace_back(std::move(labelNextStmt));
    returnValue = FEIRBuilder::CreateExprDRead(shortCircuit->Clone());
  }
  return returnValue;
}

UniqueFEIRExpr ASTBinaryOperatorExpr::Emit2FEExprLogicOperateSimplify(std::list<UniqueFEIRStmt> &stmts) const {
  std::list<UniqueFEIRStmt> lStmts;
  std::list<UniqueFEIRStmt> cStmts;
  std::list<UniqueFEIRStmt> rStmts;
  Opcode op = opcode == OP_cior ? OP_brtrue : OP_brfalse;
  MIRType *tempVarType = GlobalTables::GetTypeTable().GetInt32();
  UniqueFEIRType tempFeirType = std::make_unique<FEIRTypeNative>(*tempVarType);
  UniqueFEIRVar shortCircuit = FEIRBuilder::CreateVarNameForC(varName, *tempVarType);
  std::string labelName = FEUtils::GetSequentialName("shortCircuit_label_");
  auto leftFEExpr = leftExpr->Emit2FEExpr(lStmts);
  leftFEExpr = FEIRBuilder::CreateExprZeroCompare(OP_ne, std::move(leftFEExpr));
  auto leftStmt = std::make_unique<FEIRStmtDAssign>(shortCircuit->Clone(), leftFEExpr->Clone(), 0);
  cStmts.emplace_back(std::move(leftStmt));
  auto dreadExpr = FEIRBuilder::CreateExprDRead(shortCircuit->Clone());
  UniqueFEIRStmt condGoToExpr = std::make_unique<FEIRStmtCondGotoForC>(dreadExpr->Clone(), op, labelName);
  cStmts.emplace_back(std::move(condGoToExpr));
  auto rightFEExpr = rightExpr->Emit2FEExpr(rStmts);
  rightFEExpr = FEIRBuilder::CreateExprZeroCompare(OP_ne, std::move(rightFEExpr));
  if (rStmts.empty()) {
    stmts.splice(stmts.end(), lStmts);
    UniqueFEIRType feirType = SelectBinaryOperatorType(leftFEExpr, rightFEExpr);
    return FEIRBuilder::CreateExprBinary(std::move(feirType), opcode, std::move(leftFEExpr), std::move(rightFEExpr));
  }
  auto rightStmt = std::make_unique<FEIRStmtDAssign>(shortCircuit->Clone(), rightFEExpr->Clone(), 0);
  rStmts.emplace_back(std::move(rightStmt));
  auto labelStmt = std::make_unique<FEIRStmtLabel>(labelName);
  labelStmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
  rStmts.emplace_back(std::move(labelStmt));
  stmts.splice(stmts.end(), lStmts);
  stmts.splice(stmts.end(), cStmts);
  stmts.splice(stmts.end(), rStmts);
  return FEIRBuilder::CreateExprZeroCompare(OP_ne, std::move(dreadExpr));
}

UniqueFEIRExpr ASTBinaryOperatorExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  if (complexElementType != nullptr) {
    if (opcode == OP_add || opcode == OP_sub) {
      return Emit2FEExprComplexCalculations(stmts);
    } else if (opcode == OP_eq || opcode == OP_ne) {
      return Emit2FEExprComplexCompare(stmts);
    } else {
      CHECK_FATAL(false, "NIY");
    }
  } else {
    if (opcode == OP_cior || opcode == OP_cand) {
      if (FEOptions::GetInstance().IsSimplifyShortCircuit()) {
        return Emit2FEExprLogicOperateSimplify(stmts);
      } else {
        return Emit2FEExprLogicOperate(stmts);
      }
    } else {
      auto leftFEExpr = leftExpr->Emit2FEExpr(stmts);
      auto rightFEExpr = rightExpr->Emit2FEExpr(stmts);
      if (FEOptions::GetInstance().IsO2()) {
        Ror ror(opcode, leftFEExpr, rightFEExpr);
        auto rorExpr = ror.Emit2FEExpr();
        if (rorExpr != nullptr) {
          return rorExpr;
        }
      }
      UniqueFEIRType feirType = SelectBinaryOperatorType(leftFEExpr, rightFEExpr);
      return FEIRBuilder::CreateExprBinary(std::move(feirType), opcode, std::move(leftFEExpr), std::move(rightFEExpr));
    }
  }
}

void ASTAssignExpr::GetActualRightExpr(UniqueFEIRExpr &right, const UniqueFEIRExpr &left) const {
  if (right->GetPrimType() != left->GetPrimType() &&
      right->GetPrimType() != PTY_void &&
      right->GetPrimType() != PTY_agg) {
    PrimType dstType = left->GetPrimType();
    if (right->GetPrimType() == PTY_f32 || right->GetPrimType() == PTY_f64) {
      if (left->GetPrimType() == PTY_u8 || left->GetPrimType() == PTY_u16) {
        dstType = PTY_u32;
      }
      if (left->GetPrimType() == PTY_i8 || left->GetPrimType() == PTY_i16) {
        dstType = PTY_i32;
      }
    }
    right = FEIRBuilder::CreateExprCastPrim(std::move(right), dstType);
  }
}

UniqueFEIRExpr ASTAssignExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRExpr rightFEExpr = rightExpr->Emit2FEExpr(stmts); // parse the right expr to generate stmt first
  UniqueFEIRExpr leftFEExpr;
  if (isCompoundAssign) {
    std::list<UniqueFEIRStmt> dummyStmts;
    leftFEExpr = leftExpr->Emit2FEExpr(dummyStmts);
  } else {
    leftFEExpr = leftExpr->Emit2FEExpr(stmts);
  }
  // C89 does not support lvalue casting, but cxx support, needs to improve here
  if (leftFEExpr->GetKind() == FEIRNodeKind::kExprDRead && !leftFEExpr->GetType()->IsArray()) {
    auto dreadFEExpr = static_cast<FEIRExprDRead*>(leftFEExpr.get());
    FieldID fieldID = dreadFEExpr->GetFieldID();
    UniqueFEIRVar var = dreadFEExpr->GetVar()->Clone();
    if (ConditionalOptimize::DeleteRedundantTmpVar(rightFEExpr, stmts, var, leftFEExpr->GetPrimType(), fieldID)) {
      return leftFEExpr;
    }
    GetActualRightExpr(rightFEExpr, leftFEExpr);
    auto preStmt = std::make_unique<FEIRStmtDAssign>(std::move(var), rightFEExpr->Clone(), fieldID);
    stmts.emplace_back(std::move(preStmt));
    return leftFEExpr;
  } else if (leftFEExpr->GetKind() == FEIRNodeKind::kExprIRead) {
    auto ireadFEExpr = static_cast<FEIRExprIRead*>(leftFEExpr.get());
    FieldID fieldID = ireadFEExpr->GetFieldID();
    GetActualRightExpr(rightFEExpr, leftFEExpr);
    auto preStmt = std::make_unique<FEIRStmtIAssign>(ireadFEExpr->GetClonedPtrType(), ireadFEExpr->GetClonedOpnd(),
                                                     rightFEExpr->Clone(), fieldID);
    stmts.emplace_back(std::move(preStmt));
    return leftFEExpr;
  }
  return nullptr;
}

bool ASTAssignExpr::IsInsertNonnullChecking(const UniqueFEIRExpr &rExpr) const {
  if (!FEOptions::GetInstance().IsNpeCheckDynamic()) {
    return false;
  }
  // The pointer assignment
  if (retType == nullptr || !(retType->IsMIRPtrType())) {
    return false;
  }
  // The Rvalue is a pointer type
  if ((rExpr->GetKind() == kExprDRead && rExpr->GetPrimType() == PTY_ptr) ||
      (rExpr->GetKind() == kExprIRead && rExpr->GetFieldID() != 0 && rExpr->GetPrimType() == PTY_ptr)) {
    return true;
  }
  // The Rvalue is NULL
  if (ENCChecker::HasNullExpr(rExpr)) {
    return true;
  }
  return false;
}

UniqueFEIRExpr ASTBOComma::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  (void)leftExpr->Emit2FEExpr(stmts);
  return rightExpr->Emit2FEExpr(stmts);
}

UniqueFEIRExpr ASTBOPtrMemExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  (void)stmts;
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

// ---------- ASTParenExpr ----------
UniqueFEIRExpr ASTParenExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = child;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  childExpr->SetShortCircuitIdx(trueIdx, falseIdx);
  return childExpr->Emit2FEExpr(stmts);
}

// ---------- ASTIntegerLiteral ----------
MIRConst *ASTIntegerLiteral::GenerateMIRConstImpl() const {
  return GlobalTables::GetIntConstTable().GetOrCreateIntConst(val, *GlobalTables::GetTypeTable().GetPrimType(PTY_i64));
}

UniqueFEIRExpr ASTIntegerLiteral::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  (void)stmts;
  UniqueFEIRExpr constExpr = std::make_unique<FEIRExprConst>(val, mirType->GetPrimType());
  return constExpr;
}

// ---------- ASTFloatingLiteral ----------
MIRConst *ASTFloatingLiteral::GenerateMIRConstImpl() const {
  MemPool *mp = FEManager::GetModule().GetMemPool();
  MIRConst *cst;
  MIRType *type;
  if (kind == F32) {
    type = GlobalTables::GetTypeTable().GetPrimType(PTY_f32);
    cst = mp->New<MIRFloatConst>(static_cast<float>(val), *type);
  } else {
    type = GlobalTables::GetTypeTable().GetPrimType(PTY_f64);
    cst = mp->New<MIRDoubleConst>(val, *type);
  }
  return cst;
}

UniqueFEIRExpr ASTFloatingLiteral::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  (void)stmts;
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
  (void)stmts;
  UniqueFEIRExpr constExpr = FEIRBuilder::CreateExprConstAnyScalar(type, val);
  return constExpr;
}

// ---------- ASTConditionalOperator ----------
UniqueFEIRExpr ASTConditionalOperator::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRExpr condFEIRExpr = condExpr->Emit2FEExpr(stmts);
  // a noncomparative conditional expr need to be converted a comparative conditional expr
  if (!(condFEIRExpr->GetKind() == kExprBinary && static_cast<FEIRExprBinary*>(condFEIRExpr.get())->IsComparative())) {
    condFEIRExpr = FEIRBuilder::CreateExprZeroCompare(OP_ne, std::move(condFEIRExpr));
  }
  std::list<UniqueFEIRStmt> trueStmts;
  UniqueFEIRExpr trueFEIRExpr = trueExpr->Emit2FEExpr(trueStmts);
  std::list<UniqueFEIRStmt> falseStmts;
  UniqueFEIRExpr falseFEIRExpr = falseExpr->Emit2FEExpr(falseStmts);
  // when subExpr is void
  if (trueFEIRExpr == nullptr || falseFEIRExpr == nullptr) {
    UniqueFEIRStmt stmtIf = FEIRBuilder::CreateStmtIf(std::move(condFEIRExpr), trueStmts, falseStmts);
    stmts.emplace_back(std::move(stmtIf));
    return nullptr;
  }
  // Otherwise, (e.g., a < 1 ? 1 : a++) create a temporary var to hold the return trueExpr or falseExpr value
  MIRType *retType = trueFEIRExpr->GetType()->GenerateMIRTypeAuto();
  if (retType->GetKind() == kTypeBitField) {
    retType = GlobalTables::GetTypeTable().GetPrimType(retType->GetPrimType());
  }
  UniqueFEIRVar tempVar = FEIRBuilder::CreateVarNameForC(varName, *retType);
  UniqueFEIRVar tempVarCloned1 = tempVar->Clone();
  UniqueFEIRVar tempVarCloned2 = tempVar->Clone();
  UniqueFEIRStmt retTrueStmt = FEIRBuilder::CreateStmtDAssign(std::move(tempVar), std::move(trueFEIRExpr));
  retTrueStmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
  trueStmts.emplace_back(std::move(retTrueStmt));
  UniqueFEIRStmt retFalseStmt = FEIRBuilder::CreateStmtDAssign(std::move(tempVarCloned1), std::move(falseFEIRExpr));
  retFalseStmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
  falseStmts.emplace_back(std::move(retFalseStmt));
  UniqueFEIRStmt stmtIf = FEIRBuilder::CreateStmtIf(std::move(condFEIRExpr), trueStmts, falseStmts);
  stmts.emplace_back(std::move(stmtIf));
  UniqueFEIRExpr expr = FEIRBuilder::CreateExprDRead(std::move(tempVarCloned2));
  if (!FEOptions::GetInstance().IsNpeCheckDynamic() || !FEOptions::GetInstance().IsBoundaryCheckDynamic()) {
    expr->SetKind(kExprTernary);
  }
  return expr;
}

// ---------- ASTConstantExpr ----------
UniqueFEIRExpr ASTConstantExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  return child->Emit2FEExpr(stmts);
}

MIRConst *ASTConstantExpr::GenerateMIRConstImpl() const {
  if (child->GetConstantValue() == nullptr || child->GetConstantValue()->GetPrimType() == PTY_begin) {
    return child->GenerateMIRConst();
  } else {
    return child->GetConstantValue()->Translate2MIRConst();
  }
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
  UniqueFEIRExpr readVaList = child->Emit2FEExpr(stmts);
  // The va_arg_offset temp var is created and assigned from __gr_offs or __vr_offs of va_list
  MIRType *int32Type = GlobalTables::GetTypeTable().GetInt32();
  UniqueFEIRType int32FETRType = std::make_unique<FEIRTypeNative>(*int32Type);
  UniqueFEIRVar offsetVar = FEIRBuilder::CreateVarNameForC(FEUtils::GetSequentialName("va_arg_offs_"), *int32Type);
  UniqueFEIRExpr dreadVaListOffset = FEIRBuilder::ReadExprField(
      readVaList->Clone(), info.isGPReg ? 4 : 5, int32FETRType->Clone());
  UniqueFEIRStmt dassignOffsetVar = FEIRBuilder::CreateStmtDAssign(offsetVar->Clone(), dreadVaListOffset->Clone());
  stmts.emplace_back(std::move(dassignOffsetVar));
  UniqueFEIRExpr dreadOffsetVar = FEIRBuilder::CreateExprDRead(offsetVar->Clone());
  // The va_arg temp var is created and assigned in tow following way:
  // If the va_arg_offs is vaild, i.e., its value should be (0 - (8 - named_arg) * 8)  ~  0,
  // the va_arg will be got from GP or FP/SIMD arg reg, otherwise va_arg will be got from stack.
  // See https://developer.arm.com/documentation/ihi0055/latest/?lang=en#the-va-arg-macro for more info
  const std::string onStackStr = FEUtils::GetSequentialName("va_on_stack_");
  UniqueFEIRExpr cond1 = FEIRBuilder::CreateExprBinary(  // checking validity: regOffs >= 0, goto on_stack
      OP_ge, dreadOffsetVar->Clone(), FEIRBuilder::CreateExprConstI32(0));
  UniqueFEIRStmt condGoTo1 = std::make_unique<FEIRStmtCondGotoForC>(cond1->Clone(), OP_brtrue, onStackStr);
  stmts.emplace_back(std::move(condGoTo1));
  // The va_arg set next reg setoff
  UniqueFEIRExpr ArgAUnitOffs = FEIRBuilder::CreateExprBinary(
      OP_add, dreadOffsetVar->Clone(), FEIRBuilder::CreateExprConstI32(info.regOffset));
  UniqueFEIRStmt assignArgNextOffs = FEIRBuilder::AssginStmtField(
      readVaList->Clone(), std::move(ArgAUnitOffs), info.isGPReg ? 4 : 5);
  stmts.emplace_back(std::move(assignArgNextOffs));
  UniqueFEIRExpr cond2 = FEIRBuilder::CreateExprBinary(  // checking validity: regOffs + next offset > 0, goto on_stack
      OP_gt, dreadVaListOffset->Clone(), FEIRBuilder::CreateExprConstI32(0));
  UniqueFEIRStmt condGoTo2 = std::make_unique<FEIRStmtCondGotoForC>(cond2->Clone(), OP_brtrue, onStackStr);
  stmts.emplace_back(std::move(condGoTo2));
  // The va_arg will be got from GP or FP/SIMD arg reg
  MIRType *sizeType = GlobalTables::GetTypeTable().GetPtrType();
  UniqueFEIRType sizeFEIRType = std::make_unique<FEIRTypeNative>(*sizeType);
  MIRType *vaArgType = !info.isCopyedMem ? mirType : sizeType;
  MIRType *ptrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*vaArgType);
  UniqueFEIRVar vaArgVar = FEIRBuilder::CreateVarNameForC(FEUtils::GetSequentialName("va_arg_"), *ptrType);
  UniqueFEIRExpr dreadVaArgTop = FEIRBuilder::ReadExprField(
      readVaList->Clone(), info.isGPReg ? 2 : 3, sizeFEIRType->Clone());
  ProcessBigEndianForReg(stmts, *vaArgType, offsetVar, info); // process big endian for reg
  UniqueFEIRExpr cvtOffset = FEIRBuilder::CreateExprCastPrim(dreadOffsetVar->Clone(), PTY_i64);
  UniqueFEIRExpr addTopAndOffs = FEIRBuilder::CreateExprBinary(OP_add, std::move(dreadVaArgTop), std::move(cvtOffset));
  UniqueFEIRStmt dassignVaArgFromReg = FEIRBuilder::CreateStmtDAssign(vaArgVar->Clone(), std::move(addTopAndOffs));
  stmts.emplace_back(std::move(dassignVaArgFromReg));
  if (info.HFAType != nullptr && !info.isCopyedMem) {
    CvtHFA2Struct(*static_cast<MIRStructType*>(mirType), *info.HFAType, vaArgVar->Clone(), stmts);
  }
  const std::string endStr = FEUtils::GetSequentialName("va_end_");
  UniqueFEIRStmt goToEnd = FEIRBuilder::CreateStmtGoto(endStr);
  stmts.emplace_back(std::move(goToEnd));
  // Otherwise, the va_arg will be got from stack and set next stack setoff
  UniqueFEIRStmt onStackLabelStmt = std::make_unique<FEIRStmtLabel>(onStackStr);
  stmts.emplace_back(std::move(onStackLabelStmt));
  UniqueFEIRExpr dreadStackTop = FEIRBuilder::ReadExprField(readVaList->Clone(), 1, sizeFEIRType->Clone());
  UniqueFEIRStmt dassignVaArgFromStack = FEIRBuilder::CreateStmtDAssign(vaArgVar->Clone(), dreadStackTop->Clone());
  UniqueFEIRExpr stackAUnitOffs = FEIRBuilder::CreateExprBinary(
      OP_add, dreadStackTop->Clone(), FEIRBuilder::CreateExprConstPtr(info.stackOffset));
  stmts.emplace_back(std::move(dassignVaArgFromStack));
  UniqueFEIRStmt dassignStackNextOffs = FEIRBuilder::AssginStmtField(
      readVaList->Clone(), std::move(stackAUnitOffs), 1);
  stmts.emplace_back(std::move(dassignStackNextOffs));
  ProcessBigEndianForStack(stmts, *vaArgType, vaArgVar);  // process big endian for stack
  // return va_arg
  UniqueFEIRStmt endLabelStmt = std::make_unique<FEIRStmtLabel>(endStr);
  stmts.emplace_back(std::move(endLabelStmt));
  UniqueFEIRExpr dreadRetVar = FEIRBuilder::CreateExprDRead(vaArgVar->Clone());
  if (info.isCopyedMem) {
    UniqueFEIRType ptrPtrFEIRType = std::make_unique<FEIRTypeNative>(
        *GlobalTables::GetTypeTable().GetOrCreatePointerType(*ptrType));
    dreadRetVar = FEIRBuilder::CreateExprIRead(
        std::move(sizeFEIRType), std::move(ptrPtrFEIRType), std::move(dreadRetVar));
  }
  UniqueFEIRType baseFEIRType = std::make_unique<FEIRTypeNative>(*mirType);
  UniqueFEIRType ptrFEIRType = std::make_unique<FEIRTypeNative>(*ptrType);
  UniqueFEIRExpr retExpr = FEIRBuilder::CreateExprIRead(
      std::move(baseFEIRType), std::move(ptrFEIRType), std::move(dreadRetVar));
  return retExpr;
}

void ASTVAArgExpr::ProcessBigEndianForReg(std::list<UniqueFEIRStmt> &stmts, MIRType &vaArgType,
                                          const UniqueFEIRVar &offsetVar, const VaArgInfo &info) const {
  if (!FEOptions::GetInstance().IsBigEndian()) {
    return;
  }
  int offset = 0;
  if (info.isGPReg && !vaArgType.IsStructType() && vaArgType.GetSize() < 8) {  // general reg
    offset = 8 - static_cast<int>(vaArgType.GetSize());
  } else if (info.HFAType != nullptr) {  // HFA
    offset = 16 - static_cast<int>(info.HFAType->GetSize());
  } else if (!info.isGPReg && !vaArgType.IsStructType() && vaArgType.GetSize() < 16) {  // fp/simd reg
    offset = 16 - static_cast<int>(vaArgType.GetSize());
  }
  if (offset == 0) {
    return;
  }
  UniqueFEIRExpr addExpr = FEIRBuilder::CreateExprBinary(
      OP_add, FEIRBuilder::CreateExprDRead(offsetVar->Clone()), FEIRBuilder::CreateExprConstI32(offset));
  UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtDAssign(offsetVar->Clone(), std::move(addExpr));
  stmts.emplace_back(std::move(stmt));
}

void ASTVAArgExpr::ProcessBigEndianForStack(std::list<UniqueFEIRStmt> &stmts, MIRType &vaArgType,
                                            const UniqueFEIRVar &vaArgVar) const {
  if (!FEOptions::GetInstance().IsBigEndian()) {
    return;
  }
  int offset = 0;
  if (!vaArgType.IsStructType() && vaArgType.GetSize() < 8) {
    offset = 8 - static_cast<int>(vaArgType.GetSize());
  }
  if (offset == 0) {
    return;
  }
  UniqueFEIRExpr addExpr = FEIRBuilder::CreateExprBinary(
      OP_add, FEIRBuilder::CreateExprDRead(vaArgVar->Clone()), FEIRBuilder::CreateExprConstU64(offset));
  UniqueFEIRExpr dreadRetVar = FEIRBuilder::CreateExprDRead(vaArgVar->Clone());
  UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtDAssign(vaArgVar->Clone(), std::move(addExpr));
  stmts.emplace_back(std::move(stmt));
}

VaArgInfo ASTVAArgExpr::ProcessValistArgInfo(const MIRType &type) const {
  VaArgInfo info;
  if (type.IsScalarType()) {
    switch (type.GetPrimType()) {
      case PTY_f32:  // float is automatically promoted to double when passed to va_arg
        WARN(kLncWarn, "error: float is promoted to double when passed to va_arg");
      case PTY_f64:  // double
        info = { false, 16, 8, false, nullptr };
        break;
      case PTY_i32:
      case PTY_u32:
      case PTY_i64:
      case PTY_u64:
        info = { true, 8, 8, false, nullptr };
        break;
      default:  // bool, char, short, and unscoped enumerations are converted to int or wider integer types
        WARN(kLncWarn, "error: bool, char, short, and unscoped enumerations are promoted to int or "\
                       "wider integer types when passed to va_arg");
        info = { true, 8, 8, false, nullptr };
        break;
    }
  } else if (type.IsMIRPtrType()) {
    info = { true, 8, 8, false, nullptr };
  } else if (type.IsStructType()) {
    MIRStructType structType = static_cast<const MIRStructType&>(type);
    size_t size = structType.GetSize();
    size = (size + 7) & -8;  // size round up 8
    if (size > 16) {
      info = { true, 8, 8, true, nullptr };
    } else {
      MIRType *hfa = IsHFAType(structType);
      if (hfa != nullptr) {
        int fieldsNum = static_cast<int>(structType.GetSize() / hfa->GetSize());
        info = { false, fieldsNum * 16, static_cast<int>(size), false, hfa };
      } else {
        info = { true, static_cast<int>(size), static_cast<int>(size), false, nullptr };
      }
    }
  } else {
    CHECK_FATAL(false, "unsupport mirtype");
  }
  return info;
}

// Homogeneous Floating-point Aggregate:
// A data type with 2 to 4 identical floating-point members, either floats or doubles.
// (including 1 members here, struct nested array)
MIRType *ASTVAArgExpr::IsHFAType(const MIRStructType &type) const {
  uint32 size = static_cast<uint32>(type.GetFieldsSize());
  if (size < 1 || size > 4) {
    return nullptr;
  }
  MIRType *firstType = nullptr;
  for (uint32 i = 0; i < size; ++i) {
    MIRType *fieldType = type.GetElemType(i);
    if (fieldType->GetKind() == kTypeArray) {
      MIRArrayType *arrayType = static_cast<MIRArrayType*>(fieldType);
      MIRType *elemType = arrayType->GetElemType();
      if (elemType->GetPrimType() != PTY_f32 && elemType->GetPrimType() != PTY_f64) {
        return nullptr;
      }
      fieldType = elemType;
    } else if (fieldType->GetPrimType() != PTY_f32 && fieldType->GetPrimType() != PTY_f64) {
      return nullptr;
    }
    if (firstType == nullptr) {
      firstType = fieldType;
    } else if (fieldType != firstType) {
      return nullptr;
    }
  }
  return firstType;
}

// When va_arg is HFA struct,
// if it is passed as parameter in register then each uniquely addressable field goes in its own register.
// So its fields in FP/SIMD arg reg are still 128 bit and should be converted float or double type fields.
void ASTVAArgExpr::CvtHFA2Struct(const MIRStructType &type, MIRType &fieldType, const UniqueFEIRVar &vaArgVar,
                                 std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRVar copyedVar = FEIRBuilder::CreateVarNameForC(FEUtils::GetSequentialName("va_arg_struct_"), *mirType);
  MIRType *ptrMirType = GlobalTables::GetTypeTable().GetOrCreatePointerType(fieldType);
  UniqueFEIRType baseType = std::make_unique<FEIRTypeNative>(fieldType);
  UniqueFEIRType ptrType = std::make_unique<FEIRTypeNative>(*ptrMirType);
  size_t size = type.GetSize() / fieldType.GetSize();  // fieldType must be nonzero
  for (size_t i = 0; i < size; ++i) {
    UniqueFEIRExpr dreadVaArg = FEIRBuilder::CreateExprDRead(vaArgVar->Clone());
    if (i != 0) {
      dreadVaArg = FEIRBuilder::CreateExprBinary(
          OP_add, std::move(dreadVaArg), FEIRBuilder::CreateExprConstPtr(static_cast<int64>(16 * i)));
    }
    UniqueFEIRExpr ireadVaArg = FEIRBuilder::CreateExprIRead(baseType->Clone(), ptrType->Clone(), dreadVaArg->Clone());
    UniqueFEIRExpr addrofVar = FEIRBuilder::CreateExprAddrofVar(copyedVar->Clone());
    if(i != 0) {
      addrofVar = FEIRBuilder::CreateExprBinary(
          OP_add, std::move(addrofVar), FEIRBuilder::CreateExprConstPtr(static_cast<int64>(fieldType.GetSize() * i)));
    }
    MIRType *fieldPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(fieldType);
    UniqueFEIRType fieldFEIRType = std::make_unique<FEIRTypeNative>(*fieldPtrType);
    UniqueFEIRStmt iassignCopyedVar = FEIRBuilder::CreateStmtIAssign(
        std::move(fieldFEIRType), std::move(addrofVar), std::move(ireadVaArg));
    stmts.emplace_back(std::move(iassignCopyedVar));
  }
  UniqueFEIRExpr addrofCopyedVar = FEIRBuilder::CreateExprAddrofVar(copyedVar->Clone());
  UniqueFEIRStmt assignVar = FEIRBuilder::CreateStmtDAssign(vaArgVar->Clone(), std::move(addrofCopyedVar));
  stmts.emplace_back(std::move(assignVar));
}

// ---------- ASTArrayInitLoopExpr ----------
UniqueFEIRExpr ASTArrayInitLoopExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  (void)stmts;
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTArrayInitIndexExpr ----------
UniqueFEIRExpr ASTArrayInitIndexExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  (void)stmts;
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTExprWithCleanups ----------
UniqueFEIRExpr ASTExprWithCleanups::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  (void)stmts;
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTMaterializeTemporaryExpr ----------
UniqueFEIRExpr ASTMaterializeTemporaryExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  (void)stmts;
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTSubstNonTypeTemplateParmExpr ----------
UniqueFEIRExpr ASTSubstNonTypeTemplateParmExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  (void)stmts;
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTDependentScopeDeclRefExpr ----------
UniqueFEIRExpr ASTDependentScopeDeclRefExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  (void)stmts;
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTAtomicExpr ----------
UniqueFEIRExpr ASTAtomicExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  auto atomicExpr = std::make_unique<FEIRExprAtomic>(mirType, refType, objExpr->Emit2FEExpr(stmts), atomicOp);
  if (atomicOp != kAtomicOpLoadN) {
    static_cast<FEIRExprAtomic*>(atomicExpr.get())->SetVal1Expr(valExpr1->Emit2FEExpr(stmts));
    static_cast<FEIRExprAtomic*>(atomicExpr.get())->SetVal1Type(val1Type);
  }
  if (atomicOp == kAtomicOpExchange) {
    static_cast<FEIRExprAtomic*>(atomicExpr.get())->SetVal2Expr(valExpr2->Emit2FEExpr(stmts));
    static_cast<FEIRExprAtomic*>(atomicExpr.get())->SetVal2Type(val2Type);
  }
  static_cast<FEIRExprAtomic*>(atomicExpr.get())->SetOrderExpr(orderExpr->Emit2FEExpr(stmts));
  auto var = FEIRBuilder::CreateVarNameForC(FEUtils::GetSequentialName("ret.var."), *refType, false, false);
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
  auto *lastCpdStmt = static_cast<ASTCompoundStmt *>(cpdStmt);
  while (lastCpdStmt->GetASTStmtList().back()->GetASTStmtOp() == kASTStmtStmtExpr) {
    auto bodyStmt = static_cast<ASTStmtExprStmt*>(lastCpdStmt->GetASTStmtList().back())->GetBodyStmt();
    lastCpdStmt = static_cast<ASTCompoundStmt*>(bodyStmt);
  }
  if (lastCpdStmt->GetASTStmtList().size() != 0 && lastCpdStmt->GetASTStmtList().back()->GetExprs().size() != 0) {
    UniqueFEIRExpr feirExpr = lastCpdStmt->GetASTStmtList().back()->GetExprs().back()->Emit2FEExpr(stmts0);
    for (size_t i = 0; i < stmts0.size(); ++i) {
      if (!stmts.empty()) {
        stmts.pop_back();
      }
    }
    for (auto &stmt : stmts0) {
      stmts.emplace_back(std::move(stmt));
    }
    return feirExpr;
  }
  return nullptr;
}
}
