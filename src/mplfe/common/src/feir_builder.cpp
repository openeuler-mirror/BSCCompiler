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
#include "feir_builder.h"
#include "mpl_logging.h"
#include "global_tables.h"
#include "feir_var_reg.h"
#include "feir_var_name.h"
#include "fe_type_manager.h"
#include "feir_type_helper.h"

namespace maple {
extern const char *GetPrimTypeName(PrimType primType);
extern uint32 GetPrimTypeSize(PrimType primType);

UniqueFEIRType FEIRBuilder::CreateType(PrimType basePty, const GStrIdx &baseNameIdx, uint32 dim) {
  UniqueFEIRType type = std::make_unique<FEIRTypeDefault>(basePty, baseNameIdx, dim);
  CHECK_NULL_FATAL(type);
  return type;
}

UniqueFEIRType FEIRBuilder::CreateArrayElemType(const UniqueFEIRType &arrayType) {
  std::string typeName = arrayType->GetTypeName();
  ASSERT(typeName.length() > 1 && typeName.at(0) == 'A', "Invalid array type: %s", typeName.c_str());
  std::unique_ptr<FEIRTypeDefault> type = std::make_unique<FEIRTypeDefault>();
  type->LoadFromJavaTypeName(typeName.substr(1), true);
  return type;
}

UniqueFEIRType FEIRBuilder::CreateRefType(const GStrIdx &baseNameIdx, uint32 dim) {
  return CreateType(PTY_ref, baseNameIdx, dim);
}

UniqueFEIRType FEIRBuilder::CreateTypeByJavaName(const std::string &typeName, bool inMpl) {
  UniqueFEIRType type = std::make_unique<FEIRTypeDefault>(PTY_ref);
  CHECK_NULL_FATAL(type);
  FEIRTypeDefault *ptrType = static_cast<FEIRTypeDefault*>(type.get());
  ptrType->LoadFromJavaTypeName(typeName, inMpl);
  return type;
}

UniqueFEIRVar FEIRBuilder::CreateVarReg(uint32 regNum, PrimType primType, bool isGlobal) {
  UniqueFEIRVar var = std::make_unique<FEIRVarReg>(regNum, primType);
  CHECK_NULL_FATAL(var);
  var->SetGlobal(isGlobal);
  return var;
}

UniqueFEIRVar FEIRBuilder::CreateVarReg(uint32 regNum, UniqueFEIRType type, bool isGlobal) {
  UniqueFEIRVar var = std::make_unique<FEIRVarReg>(regNum, std::move(type));
  CHECK_NULL_FATAL(var);
  var->SetGlobal(isGlobal);
  return var;
}

UniqueFEIRVar FEIRBuilder::CreateVarName(GStrIdx nameIdx, PrimType primType, bool isGlobal, bool withType) {
  UniqueFEIRVar var = std::make_unique<FEIRVarName>(nameIdx, withType);
  CHECK_NULL_FATAL(var);
  var->GetType()->SetPrimType(primType);
  var->SetGlobal(isGlobal);
  return var;
}

UniqueFEIRVar FEIRBuilder::CreateVarName(const std::string &name, PrimType primType, bool isGlobal,
                                         bool withType) {
  GStrIdx nameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(name);
  return CreateVarName(nameIdx, primType, isGlobal, withType);
}

UniqueFEIRVar FEIRBuilder::CreateVarNameForC(GStrIdx nameIdx, MIRType &mirType, bool isGlobal, bool withType) {
  UniqueFEIRType type = std::make_unique<FEIRTypeNative>(mirType);
  UniqueFEIRVar var = std::make_unique<FEIRVarName>(nameIdx, std::move(type), withType);
  var->SetGlobal(isGlobal);
  return var;
}

UniqueFEIRVar FEIRBuilder::CreateVarNameForC(const std::string &name, MIRType &mirType, bool isGlobal, bool withType) {
  GStrIdx nameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(name);
  return CreateVarNameForC(nameIdx, mirType, isGlobal, withType);
}

UniqueFEIRExpr FEIRBuilder::CreateExprDRead(UniqueFEIRVar srcVar) {
  UniqueFEIRExpr expr = std::make_unique<FEIRExprDRead>(std::move(srcVar));
  CHECK_NULL_FATAL(expr);
  return expr;
}

UniqueFEIRExpr FEIRBuilder::CreateExprIRead(UniqueFEIRType returnType, UniqueFEIRType ptrType,
                                            FieldID id, UniqueFEIRExpr expr) {
  UniqueFEIRExpr feirExpr = std::make_unique<FEIRExprIRead>(std::move(returnType), std::move(ptrType),
                                                            id, std::move(expr));
  return feirExpr;
}

UniqueFEIRExpr FEIRBuilder::CreateExprAddrof(const std::vector<uint32> &array) {
  UniqueFEIRExpr expr = std::make_unique<FEIRExprAddrof>(array);
  CHECK_NULL_FATAL(expr);
  return expr;
}

UniqueFEIRExpr FEIRBuilder::CreateExprAddrofVar(UniqueFEIRVar srcVar) {
  UniqueFEIRExpr expr = std::make_unique<FEIRExprAddrofVar>(std::move(srcVar));
  return expr;
}

UniqueFEIRExpr FEIRBuilder::CreateExprAddrofFunc(const std::string &addr) {
  UniqueFEIRExpr expr = std::make_unique<FEIRExprAddrofFunc>(addr);
  return expr;
}

UniqueFEIRExpr FEIRBuilder::CreateExprTernary(Opcode op, UniqueFEIRType type, UniqueFEIRExpr cExpr,
                                              UniqueFEIRExpr tExpr, UniqueFEIRExpr fExpr) {
  UniqueFEIRExpr expr = std::make_unique<FEIRExprTernary>(op, std::move(type), std::move(cExpr),
                                                          std::move(tExpr), std::move(fExpr));
  return expr;
}

UniqueFEIRExpr FEIRBuilder::CreateExprConstRefNull() {
  return std::make_unique<FEIRExprConst>(int64{ 0 }, PTY_ref);
}

UniqueFEIRExpr FEIRBuilder::CreateExprConstPtrNull() {
  return std::make_unique<FEIRExprConst>(int64{ 0 }, PTY_ptr);
}

UniqueFEIRExpr FEIRBuilder::CreateExprConstI8(int8 val) {
  return std::make_unique<FEIRExprConst>(int64{ val }, PTY_i8);
}

UniqueFEIRExpr FEIRBuilder::CreateExprConstI16(int16 val) {
  return std::make_unique<FEIRExprConst>(int64{ val }, PTY_i16);
}

UniqueFEIRExpr FEIRBuilder::CreateExprConstI32(int32 val) {
  return std::make_unique<FEIRExprConst>(int64{ val }, PTY_i32);
}

UniqueFEIRExpr FEIRBuilder::CreateExprConstI64(int64 val) {
  return std::make_unique<FEIRExprConst>(val, PTY_i64);
}

UniqueFEIRExpr FEIRBuilder::CreateExprConstU64(uint64 val) {
  return std::make_unique<FEIRExprConst>(val, PTY_u64);
}

UniqueFEIRExpr FEIRBuilder::CreateExprConstF32(float val) {
  return std::make_unique<FEIRExprConst>(val);
}

UniqueFEIRExpr FEIRBuilder::CreateExprConstF64(double val) {
  return std::make_unique<FEIRExprConst>(val);
}

// Create a const expr of specified prime type with fixed value.
// Note that loss of precision, byte value is only supported.
UniqueFEIRExpr FEIRBuilder::CreateExprConstAnyScalar(PrimType primType, int8 val) {
  switch (primType) {
    case PTY_u8:
    case PTY_u16:
    case PTY_u32:
    case PTY_u64:
    case PTY_i8:
    case PTY_i16:
    case PTY_i32:
    case PTY_i64:
      return std::make_unique<FEIRExprConst>(static_cast<int64>(val), primType);
    case PTY_f128:
      // Not Implemented
      CHECK_FATAL(false, "Not Implemented");
      return nullptr;
    case PTY_f32:
      return CreateExprConstF32(static_cast<float>(val));
    case PTY_f64:
      return CreateExprConstF64(static_cast<double>(val));
    default:
      CHECK_FATAL(false, "unsupported const prime type");
      return nullptr;
  }
}

UniqueFEIRExpr FEIRBuilder::CreateExprMathUnary(Opcode op, UniqueFEIRVar var0) {
  UniqueFEIRExpr opnd0 = CreateExprDRead(std::move(var0));
  return std::make_unique<FEIRExprUnary>(op, std::move(opnd0));
}

UniqueFEIRExpr FEIRBuilder::CreateExprMathUnary(Opcode op, UniqueFEIRExpr expr) {
  return std::make_unique<FEIRExprUnary>(op, std::move(expr));
}

UniqueFEIRExpr FEIRBuilder::CreateExprMathBinary(Opcode op, UniqueFEIRVar var0, UniqueFEIRVar var1) {
  UniqueFEIRExpr opnd0 = CreateExprDRead(std::move(var0));
  UniqueFEIRExpr opnd1 = CreateExprDRead(std::move(var1));
  return std::make_unique<FEIRExprBinary>(op, std::move(opnd0), std::move(opnd1));
}

UniqueFEIRExpr FEIRBuilder::CreateExprMathBinary(Opcode op, UniqueFEIRExpr expr0, UniqueFEIRExpr expr1) {
  return std::make_unique<FEIRExprBinary>(op, std::move(expr0), std::move(expr1));
}

UniqueFEIRExpr FEIRBuilder::CreateExprBinary(Opcode op, UniqueFEIRExpr expr0, UniqueFEIRExpr expr1) {
  return std::make_unique<FEIRExprBinary>(op, std::move(expr0), std::move(expr1));
}

UniqueFEIRExpr FEIRBuilder::CreateExprSExt(UniqueFEIRVar srcVar) {
  return std::make_unique<FEIRExprExtractBits>(OP_sext, PTY_i32,
      std::make_unique<FEIRExprDRead>(std::move(srcVar)));
}

UniqueFEIRExpr FEIRBuilder::CreateExprSExt(UniqueFEIRExpr srcExpr, PrimType dstType) {
  return std::make_unique<FEIRExprExtractBits>(OP_sext, dstType, std::move(srcExpr));
}

UniqueFEIRExpr FEIRBuilder::CreateExprZExt(UniqueFEIRVar srcVar) {
  return std::make_unique<FEIRExprExtractBits>(OP_zext, PTY_i32,
      std::make_unique<FEIRExprDRead>(std::move(srcVar)));
}

UniqueFEIRExpr FEIRBuilder::CreateExprZExt(UniqueFEIRExpr srcExpr, PrimType dstType) {
  return std::make_unique<FEIRExprExtractBits>(OP_zext, dstType, std::move(srcExpr));
}

UniqueFEIRExpr FEIRBuilder::CreateExprCvtPrim(UniqueFEIRVar srcVar, PrimType dstType) {
  return CreateExprCvtPrim(CreateExprDRead(std::move(srcVar)), dstType);
}

UniqueFEIRExpr FEIRBuilder::CreateExprCvtPrim(UniqueFEIRExpr srcExpr, PrimType dstType) {
  UniqueFEIRExpr expr = std::make_unique<FEIRExprTypeCvt>(OP_cvt, std::move(srcExpr));
  CHECK_NULL_FATAL(expr);
  FEIRExprTypeCvt *ptrExpr = static_cast<FEIRExprTypeCvt*>(expr.get());
  ptrExpr->GetType()->SetPrimType(dstType);
  return expr;
}

UniqueFEIRExpr FEIRBuilder::CreateExprJavaNewInstance(UniqueFEIRType type) {
  UniqueFEIRExpr expr = std::make_unique<FEIRExprJavaNewInstance>(std::move(type));
  CHECK_NULL_FATAL(expr);
  return expr;
}

UniqueFEIRExpr FEIRBuilder::CreateExprJavaNewInstance(UniqueFEIRType type, uint32 argTypeID) {
  UniqueFEIRExpr expr = std::make_unique<FEIRExprJavaNewInstance>(std::move(type), argTypeID);
  CHECK_NULL_FATAL(expr);
  return expr;
}

UniqueFEIRExpr FEIRBuilder::CreateExprJavaNewInstance(UniqueFEIRType type, uint32 argTypeID, bool isRcPermanent) {
  UniqueFEIRExpr expr = std::make_unique<FEIRExprJavaNewInstance>(std::move(type), argTypeID, isRcPermanent);
  CHECK_NULL_FATAL(expr);
  return expr;
}

UniqueFEIRExpr FEIRBuilder::CreateExprJavaNewArray(UniqueFEIRType type, UniqueFEIRExpr exprSize) {
  UniqueFEIRExpr expr = std::make_unique<FEIRExprJavaNewArray>(std::move(type), std::move(exprSize));
  CHECK_NULL_FATAL(expr);
  return expr;
}

UniqueFEIRExpr FEIRBuilder::CreateExprJavaNewArray(UniqueFEIRType type, UniqueFEIRExpr exprSize, uint32 typeID) {
  UniqueFEIRExpr expr = std::make_unique<FEIRExprJavaNewArray>(std::move(type), std::move(exprSize), typeID);
  CHECK_NULL_FATAL(expr);
  return expr;
}

UniqueFEIRExpr FEIRBuilder::CreateExprJavaNewArray(UniqueFEIRType type, UniqueFEIRExpr exprSize, uint32 typeID,
                                                   bool isRcPermanent) {
  UniqueFEIRExpr expr = std::make_unique<FEIRExprJavaNewArray>(
      std::move(type), std::move(exprSize), typeID, isRcPermanent);
  CHECK_NULL_FATAL(expr);
  return expr;
}

UniqueFEIRExpr FEIRBuilder::CreateExprJavaArrayLength(UniqueFEIRExpr exprArray) {
  UniqueFEIRExpr expr = std::make_unique<FEIRExprJavaArrayLength>(std::move(exprArray));
  CHECK_NULL_FATAL(expr);
  return expr;
}

UniqueFEIRExpr FEIRBuilder::CreateExprArrayStoreForC(UniqueFEIRExpr argExprArray, UniqueFEIRExpr argExprIndex,
                                                     UniqueFEIRType argTypeNative) {
  UniqueFEIRExpr expr = std::make_unique<FEIRExprArrayStoreForC>(std::move(argExprArray), std::move(argExprIndex),
                                                                                          std::move(argTypeNative));
  CHECK_NULL_FATAL(expr);
  return expr;
}

UniqueFEIRStmt FEIRBuilder::CreateStmtDAssign(UniqueFEIRVar dstVar, UniqueFEIRExpr srcExpr, bool hasException) {
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtDAssign>(std::move(dstVar), std::move(srcExpr));
  FEIRStmtDAssign *ptrStmt = static_cast<FEIRStmtDAssign*>(stmt.get());
  ptrStmt->SetHasException(hasException);
  return stmt;
}

UniqueFEIRStmt FEIRBuilder::CreateStmtGoto(uint32 targetLabelIdx) {
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtGoto>(targetLabelIdx);
  CHECK_NULL_FATAL(stmt);
  return stmt;
}

UniqueFEIRStmt FEIRBuilder::CreateStmtGoto(const std::string &labelName) {
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtGotoForC>(labelName);
  CHECK_NULL_FATAL(stmt);
  return stmt;
}

UniqueFEIRStmt FEIRBuilder::CreateStmtCondGoto(uint32 targetLabelIdx, Opcode op, UniqueFEIRExpr expr) {
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtCondGoto>(op, targetLabelIdx, std::move(expr));
  CHECK_NULL_FATAL(stmt);
  return stmt;
}

UniqueFEIRStmt FEIRBuilder::CreateStmtSwitch(UniqueFEIRExpr expr) {
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtSwitch>(std::move(expr));
  CHECK_NULL_FATAL(stmt);
  return stmt;
}

UniqueFEIRStmt FEIRBuilder::CreateStmtIfWithoutElse(UniqueFEIRExpr cond, std::list<UniqueFEIRStmt> &thenStmts) {
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtIf>(std::move(cond), thenStmts);
  return stmt;
}

UniqueFEIRStmt FEIRBuilder::CreateStmtIf(UniqueFEIRExpr cond,
                                         std::list<UniqueFEIRStmt> &thenStmts,
                                         std::list<UniqueFEIRStmt> &elseStmts) {
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtIf>(std::move(cond), thenStmts, elseStmts);
  return stmt;
}

UniqueFEIRStmt FEIRBuilder::CreateStmtJavaConstClass(UniqueFEIRVar dstVar, UniqueFEIRType type) {
  UniqueFEIRType dstType = FETypeManager::kFEIRTypeJavaClass->Clone();
  dstVar->SetType(std::move(dstType));
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtJavaConstClass>(std::move(dstVar), std::move(type));
  return stmt;
}

UniqueFEIRStmt FEIRBuilder::CreateStmtJavaConstString(UniqueFEIRVar dstVar, const std::string &strVal) {
  UniqueFEIRType dstType = FETypeManager::kFEIRTypeJavaString->Clone();
  dstVar->SetType(std::move(dstType));
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtJavaConstString>(std::move(dstVar), strVal,
      0, GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(strVal));
  return stmt;
}

UniqueFEIRStmt FEIRBuilder::CreateStmtJavaCheckCast(UniqueFEIRVar dstVar, UniqueFEIRVar srcVar, UniqueFEIRType type) {
  UniqueFEIRExpr expr = CreateExprDRead(std::move(srcVar));
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtJavaTypeCheck>(std::move(dstVar), std::move(expr), std::move(type),
                                                                FEIRStmtJavaTypeCheck::CheckKind::kCheckCast);
  return stmt;
}

UniqueFEIRStmt FEIRBuilder::CreateStmtJavaCheckCast(UniqueFEIRVar dstVar, UniqueFEIRVar srcVar, UniqueFEIRType type,
                                                    uint32 argTypeID) {
  UniqueFEIRExpr expr = CreateExprDRead(std::move(srcVar));
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtJavaTypeCheck>(std::move(dstVar), std::move(expr), std::move(type),
                                                                FEIRStmtJavaTypeCheck::CheckKind::kCheckCast,
                                                                argTypeID);
  return stmt;
}

UniqueFEIRStmt FEIRBuilder::CreateStmtJavaInstanceOf(UniqueFEIRVar dstVar, UniqueFEIRVar srcVar, UniqueFEIRType type) {
  UniqueFEIRExpr expr = CreateExprDRead(std::move(srcVar));
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtJavaTypeCheck>(std::move(dstVar), std::move(expr), std::move(type),
                                                                FEIRStmtJavaTypeCheck::CheckKind::kInstanceOf);
  return stmt;
}

UniqueFEIRStmt FEIRBuilder::CreateStmtJavaInstanceOf(UniqueFEIRVar dstVar, UniqueFEIRVar srcVar, UniqueFEIRType type,
                                                     uint32 argTypeID) {
  UniqueFEIRExpr expr = CreateExprDRead(std::move(srcVar));
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtJavaTypeCheck>(std::move(dstVar), std::move(expr), std::move(type),
                                                                FEIRStmtJavaTypeCheck::CheckKind::kInstanceOf,
                                                                argTypeID);
  return stmt;
}

UniqueFEIRStmt FEIRBuilder::CreateStmtJavaFillArrayData(UniqueFEIRVar srcVar, const int8 *arrayData,
                                                        uint32 size, const std::string &arrayName) {
  UniqueFEIRExpr expr = CreateExprDRead(std::move(srcVar));
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtJavaFillArrayData>(std::move(expr), arrayData, size, arrayName);
  return stmt;
}

std::list<UniqueFEIRStmt> FEIRBuilder::CreateStmtArrayStore(UniqueFEIRVar varElem, UniqueFEIRVar varArray,
                                                            UniqueFEIRVar varIndex) {
  std::list<UniqueFEIRStmt> ans;
  UniqueFEIRType arrayType = varElem->GetType()->Clone();
  (void)arrayType->ArrayIncrDim();
  UniqueFEIRExpr exprElem = CreateExprDRead(std::move(varElem));
  UniqueFEIRExpr exprArray = CreateExprDRead(std::move(varArray));
  UniqueFEIRExpr exprIndex = CreateExprDRead(std::move(varIndex));
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtArrayStore>(std::move(exprElem), std::move(exprArray),
                                                             std::move(exprIndex), std::move(arrayType));
  ans.push_back(std::move(stmt));
  return ans;
}

UniqueFEIRStmt FEIRBuilder::CreateStmtArrayStoreOneStmt(UniqueFEIRVar varElem, UniqueFEIRVar varArray,
                                                        UniqueFEIRExpr exprIndex) {
  UniqueFEIRType arrayType = varElem->GetType()->Clone();
  (void)arrayType->ArrayIncrDim();
  UniqueFEIRExpr exprElem = CreateExprDRead(std::move(varElem));
  UniqueFEIRExpr exprArray = CreateExprDRead(std::move(varArray));
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtArrayStore>(std::move(exprElem), std::move(exprArray),
                                                             std::move(exprIndex), std::move(arrayType));
  return stmt;
}

UniqueFEIRStmt FEIRBuilder::CreateStmtArrayStoreOneStmtForC(UniqueFEIRExpr exprElem, UniqueFEIRExpr exprArray,
                                                            UniqueFEIRExpr exprIndex, UniqueFEIRType arrayType) {
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtArrayStore>(std::move(exprElem), std::move(exprArray),
                                                             std::move(exprIndex), std::move(arrayType));
  return stmt;
}

std::list<UniqueFEIRStmt> FEIRBuilder::CreateStmtArrayLoad(UniqueFEIRVar varElem, UniqueFEIRVar varArray,
                                                           UniqueFEIRVar varIndex) {
  std::list<UniqueFEIRStmt> ans;
  UniqueFEIRExpr exprArray = CreateExprDRead(std::move(varArray));
  UniqueFEIRExpr exprIndex = CreateExprDRead(std::move(varIndex));
  UniqueFEIRType arrayType = varElem->GetType()->Clone();
  (void)arrayType->ArrayIncrDim();
  UniqueFEIRExpr expr = std::make_unique<FEIRExprArrayLoad>(std::move(exprArray), std::move(exprIndex),
                                                            std::move(arrayType));
  UniqueFEIRStmt stmt = CreateStmtDAssign(std::move(varElem), std::move(expr), true);
  ans.push_back(std::move(stmt));
  return ans;
}

UniqueFEIRStmt FEIRBuilder::CreateStmtArrayLength(UniqueFEIRVar varLength, UniqueFEIRVar varArray) {
  UniqueFEIRExpr exprArray = CreateExprDRead(std::move(varArray));
  UniqueFEIRExpr exprIntriOp = CreateExprJavaArrayLength(std::move(exprArray));
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtDAssign>(std::move(varLength), std::move(exprIntriOp));
  return stmt;
}

UniqueFEIRStmt FEIRBuilder::CreateStmtRetype(UniqueFEIRVar varDst, const UniqueFEIRVar &varSrc) {
  // ref -> ref    : retype
  // primitive
  // short -> long : cvt  GetPrimTypeSize
  // long -> short : JAVAMERGE
  PrimType dstType = varDst->GetType()->GetPrimType();
  PrimType srcType = varSrc->GetType()->GetPrimType();
  uint32 srcPrmTypeSize = GetPrimTypeSize(varSrc->GetType()->GetPrimType());
  uint32 dstPrmTypeSize = GetPrimTypeSize(dstType);
  UniqueFEIRExpr dreadExpr = std::make_unique<FEIRExprDRead>(varSrc->Clone());
  if (dstType == PTY_ref && srcType == PTY_ref) {
    std::unique_ptr<FEIRType> ptrTy = FEIRTypeHelper::CreatePointerType(varDst->GetType()->Clone(), PTY_ref);
    UniqueFEIRExpr expr = std::make_unique<FEIRExprTypeCvt>(std::move(ptrTy), OP_retype, std::move(dreadExpr));
    return FEIRBuilder::CreateStmtDAssign(std::move(varDst), std::move(expr), false);
  } else if (srcPrmTypeSize < dstPrmTypeSize || (IsPrimitiveFloat(srcType) && IsPrimitiveInteger(dstType)) ||
             dstType == PTY_ref) {
    UniqueFEIRExpr expr = std::make_unique<FEIRExprTypeCvt>(varDst->GetType()->Clone(), OP_cvt, std::move(dreadExpr));
    return FEIRBuilder::CreateStmtDAssign(std::move(varDst), std::move(expr), false);
  } else if (((IsPrimitiveInteger(dstType) || IsPrimitiveFloat(dstType)) &&
             (IsPrimitiveInteger(srcType) || IsPrimitiveFloat(srcType)) &&
             IsPrimitiveInteger(srcType) && IsPrimitiveFloat(dstType) &&
             GetPrimTypeBitSize(srcType) <= GetPrimTypeBitSize(dstType)) ||
             (IsPrimitiveInteger(srcType) && IsPrimitiveInteger(dstType))) {
    std::vector<std::unique_ptr<FEIRExpr>> exprs;
    exprs.emplace_back(std::move(dreadExpr));
    UniqueFEIRExpr javaMerge = std::make_unique<FEIRExprJavaMerge>(varDst->GetType()->Clone(), exprs);
    return std::make_unique<FEIRStmtDAssign>(std::move(varDst), std::move(javaMerge));
  } else {
    WARN(kLncWarn, "Unsafe convert %s to %s", GetPrimTypeName(srcType), GetPrimTypeName(dstType));
    UniqueFEIRExpr expr = std::make_unique<FEIRExprTypeCvt>(varDst->GetType()->Clone(), OP_cvt, std::move(dreadExpr));
    return FEIRBuilder::CreateStmtDAssign(std::move(varDst), std::move(expr), false);
  }
  return nullptr;  // Cannot retype, maybe introduced by redundant phi.
}

UniqueFEIRStmt FEIRBuilder::CreateStmtComment(const std::string &comment) {
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtPesudoComment>(comment);
  return stmt;
}
}  // namespace maple
