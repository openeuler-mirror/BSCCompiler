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
#ifndef HIR2MPL_INCLUDE_COMMON_FEIR_BUILDER_H
#define HIR2MPL_INCLUDE_COMMON_FEIR_BUILDER_H
#include <memory>
#include "mir_function.h"
#include "mpl_logging.h"
#include "feir_var.h"
#include "feir_stmt.h"

namespace maple {
class FEIRBuilder {
 public:
  FEIRBuilder() = default;
  ~FEIRBuilder() = default;
  // Type
  static UniqueFEIRType CreateType(PrimType basePty, const GStrIdx &baseNameIdx, uint32 dim);
  static UniqueFEIRType CreateArrayElemType(const UniqueFEIRType &arrayType);
  static UniqueFEIRType CreateRefType(const GStrIdx &baseNameIdx, uint32 dim);
  static UniqueFEIRType CreateTypeByJavaName(const std::string &typeName, bool inMpl);
  // Var
  static UniqueFEIRVar CreateVarReg(uint32 regNum, PrimType primType, bool isGlobal = false);
  static UniqueFEIRVar CreateVarReg(uint32 regNum, UniqueFEIRType type, bool isGlobal = false);
  static UniqueFEIRVar CreateVarName(GStrIdx nameIdx, PrimType primType, bool isGlobal = false,
                                     bool withType = false);
  static UniqueFEIRVar CreateVarName(const std::string &name, PrimType primType, bool isGlobal = false,
                                     bool withType = false);
  static UniqueFEIRVar CreateVarNameForC(GStrIdx nameIdx, MIRType &mirType, bool isGlobal = false,
                                         bool withType = false);
  static UniqueFEIRVar CreateVarNameForC(const std::string &name, MIRType &mirType, bool isGlobal = false,
                                         bool withType = false);
  static UniqueFEIRVar CreateVarNameForC(const std::string &name, UniqueFEIRType type,
                                         bool isGlobal = false, bool withType = false);
  // Expr
  static UniqueFEIRExpr CreateExprSizeOfType(UniqueFEIRType ty);
  static UniqueFEIRExpr CreateExprDRead(UniqueFEIRVar srcVar);
  static UniqueFEIRExpr CreateExprDReadAggField(UniqueFEIRVar srcVar, FieldID fieldID, UniqueFEIRType fieldType);
  static UniqueFEIRExpr CreateExprAddrofLabel(const std::string &lbName, UniqueFEIRType exprTy);
  static UniqueFEIRExpr CreateExprAddrofVar(UniqueFEIRVar srcVar);
  static UniqueFEIRExpr CreateExprAddrofFunc(const std::string &addr);
  static UniqueFEIRExpr CreateExprAddrofArray(UniqueFEIRType argTypeNativeArray,
                                              UniqueFEIRExpr argExprArray, std::string argArrayName,
                                              std::list<UniqueFEIRExpr> &argExprIndexs);
  static UniqueFEIRExpr CreateExprIRead(UniqueFEIRType returnType, UniqueFEIRType ptrType,
                                        UniqueFEIRExpr expr, FieldID id = 0);
  static UniqueFEIRExpr CreateExprTernary(Opcode op, UniqueFEIRType type, UniqueFEIRExpr cExpr,
                                          UniqueFEIRExpr tExpr, UniqueFEIRExpr fExpr);
  static UniqueFEIRExpr CreateExprConstRefNull();
  static UniqueFEIRExpr CreateExprConstPtrNull();
  static UniqueFEIRExpr CreateExprConstI8(int8 val);
  static UniqueFEIRExpr CreateExprConstI16(int16 val);
  static UniqueFEIRExpr CreateExprConstI32(int32 val);
  static UniqueFEIRExpr CreateExprConstU32(uint32 val);
  static UniqueFEIRExpr CreateExprConstI64(int64 val);
  static UniqueFEIRExpr CreateExprConstU64(uint64 val);
  static UniqueFEIRExpr CreateExprConstF32(float val);
  static UniqueFEIRExpr CreateExprConstF64(double val);
  static UniqueFEIRExpr CreateExprConstPtr(int64 val);
  static UniqueFEIRExpr CreateExprConstAnyScalar(PrimType primType, int64 val);
  static UniqueFEIRExpr CreateExprVdupAnyVector(PrimType primtype, int64 val);
  static UniqueFEIRExpr CreateExprMathUnary(Opcode op, UniqueFEIRVar var0);
  static UniqueFEIRExpr CreateExprMathUnary(Opcode op, UniqueFEIRExpr expr);
  static UniqueFEIRExpr CreateExprZeroCompare(Opcode op, UniqueFEIRExpr expr);
  static UniqueFEIRExpr CreateExprMathBinary(Opcode op, UniqueFEIRVar var0, UniqueFEIRVar var1);
  static UniqueFEIRExpr CreateExprMathBinary(Opcode op, UniqueFEIRExpr expr0, UniqueFEIRExpr expr1);
  static UniqueFEIRExpr CreateExprBinary(Opcode op, UniqueFEIRExpr expr0, UniqueFEIRExpr expr1);
  static UniqueFEIRExpr CreateExprBinary(UniqueFEIRType exprType, Opcode op,
                                         UniqueFEIRExpr expr0, UniqueFEIRExpr expr1);
  static UniqueFEIRExpr CreateExprSExt(UniqueFEIRVar srcVar);
  static UniqueFEIRExpr CreateExprSExt(UniqueFEIRExpr srcExpr, PrimType dstType);
  static UniqueFEIRExpr CreateExprZExt(UniqueFEIRVar srcVar);
  static UniqueFEIRExpr CreateExprZExt(UniqueFEIRExpr srcExpr, PrimType dstType);
  static UniqueFEIRExpr CreateExprCvtPrim(UniqueFEIRVar srcVar, PrimType dstType);
  static UniqueFEIRExpr CreateExprCvtPrim(UniqueFEIRExpr srcExpr, PrimType dstType);
  static UniqueFEIRExpr CreateExprCvtPrim(UniqueFEIRExpr srcExpr, PrimType srcType, PrimType dstType);
  static UniqueFEIRExpr CreateExprCvtPrim(Opcode argOp, UniqueFEIRExpr srcExpr, PrimType dstType);
  static UniqueFEIRExpr CreateExprCastPrim(UniqueFEIRExpr srcExpr, PrimType dstType);
  static UniqueFEIRExpr CreateExprJavaNewInstance(UniqueFEIRType type);
  static UniqueFEIRExpr CreateExprJavaNewInstance(UniqueFEIRType type, uint32 argTypeID);
  static UniqueFEIRExpr CreateExprJavaNewInstance(UniqueFEIRType type, uint32 argTypeID, bool isRcPermanent);
  static UniqueFEIRExpr CreateExprJavaNewArray(UniqueFEIRType type, UniqueFEIRExpr exprSize);
  static UniqueFEIRExpr CreateExprJavaNewArray(UniqueFEIRType type, UniqueFEIRExpr exprSize, uint32 typeID);
  static UniqueFEIRExpr CreateExprJavaNewArray(UniqueFEIRType type, UniqueFEIRExpr exprSize, uint32 typeID,
                                               bool isRcPermanent);
  static UniqueFEIRExpr CreateExprJavaArrayLength(UniqueFEIRExpr exprArray);

  // Stmt
  static UniqueFEIRStmt CreateStmtDAssign(UniqueFEIRVar dstVar, UniqueFEIRExpr srcExpr, bool hasException = false);
  static UniqueFEIRStmt CreateStmtDAssignAggField(UniqueFEIRVar dstVar, UniqueFEIRExpr srcExpr, FieldID fieldID);
  static UniqueFEIRStmt CreateStmtIAssign(UniqueFEIRType dstType, UniqueFEIRExpr dstExpr,
                                          UniqueFEIRExpr srcExpr, FieldID fieldID = 0);
  static UniqueFEIRStmt CreateStmtGoto(uint32 targetLabelIdx);
  static UniqueFEIRStmt CreateStmtGoto(const std::string &labelName);
  static UniqueFEIRStmt CreateStmtIGoto(UniqueFEIRExpr targetExpr);
  static UniqueFEIRStmt CreateStmtCondGoto(uint32 targetLabelIdx, Opcode op, UniqueFEIRExpr expr);
  static UniqueFEIRStmt CreateStmtSwitch(UniqueFEIRExpr expr);
  static UniqueFEIRStmt CreateStmtIfWithoutElse(UniqueFEIRExpr cond, std::list<UniqueFEIRStmt> &thenStmts);
  static UniqueFEIRStmt CreateStmtIf(UniqueFEIRExpr cond, std::list<UniqueFEIRStmt> &thenStmts,
                                     std::list<UniqueFEIRStmt> &elseStmts);
  static UniqueFEIRStmt CreateStmtJavaConstClass(UniqueFEIRVar dstVar, UniqueFEIRType type);
  static UniqueFEIRStmt CreateStmtJavaConstString(UniqueFEIRVar dstVar, const std::string &strVal);
  static UniqueFEIRStmt CreateStmtJavaCheckCast(UniqueFEIRVar dstVar, UniqueFEIRVar srcVar, UniqueFEIRType type);
  static UniqueFEIRStmt CreateStmtJavaCheckCast(UniqueFEIRVar dstVar, UniqueFEIRVar srcVar, UniqueFEIRType type,
                                                                                            uint32 argTypeID);
  static UniqueFEIRStmt CreateStmtJavaInstanceOf(UniqueFEIRVar dstVar, UniqueFEIRVar srcVar, UniqueFEIRType type);
  static UniqueFEIRStmt CreateStmtJavaInstanceOf(UniqueFEIRVar dstVar, UniqueFEIRVar srcVar, UniqueFEIRType type,
                                                                                             uint32 argTypeID);
  static UniqueFEIRStmt CreateStmtJavaFillArrayData(UniqueFEIRVar argVar, const int8 *arrayData,
                                                    uint32 size, const std::string &arrayName);
  static std::list<UniqueFEIRStmt> CreateStmtArrayStore(UniqueFEIRVar varElem, UniqueFEIRVar varArray,
                                                        UniqueFEIRVar varIndex);
  static UniqueFEIRStmt CreateStmtArrayStoreOneStmt(UniqueFEIRVar varElem, UniqueFEIRVar varArray,
                                                    UniqueFEIRExpr exprIndex);
  static UniqueFEIRStmt CreateStmtArrayStoreOneStmtForC(UniqueFEIRExpr exprElem, UniqueFEIRExpr exprArray,
                                                        UniqueFEIRExpr exprIndex, UniqueFEIRType arrayType);
  static UniqueFEIRStmt CreateStmtArrayStoreOneStmtForC(UniqueFEIRExpr exprElem, UniqueFEIRExpr exprArray,
                                                        UniqueFEIRExpr exprIndex, UniqueFEIRType arrayType,
                                                        const std::string &argArrayName);
  static UniqueFEIRStmt CreateStmtArrayStoreOneStmtForC(UniqueFEIRExpr exprElem, UniqueFEIRExpr exprArray,
                                                        std::list<UniqueFEIRExpr> exprIndexs,
                                                        UniqueFEIRType arrayType, const std::string &argArrayName);
  static UniqueFEIRStmt CreateStmtArrayStoreOneStmtForC(UniqueFEIRExpr exprElem, UniqueFEIRExpr exprArray,
                                                        UniqueFEIRExpr exprIndex, UniqueFEIRType arrayType,
                                                        UniqueFEIRType elemType, const std::string &argArrayName);
  static std::list<UniqueFEIRStmt> CreateStmtArrayLoad(UniqueFEIRVar varElem, UniqueFEIRVar varArray,
                                                       UniqueFEIRVar varIndex);
  static UniqueFEIRStmt CreateStmtArrayLength(UniqueFEIRVar varLength, UniqueFEIRVar varArray);
  static UniqueFEIRStmt CreateStmtRetype(UniqueFEIRVar varDst, const UniqueFEIRVar &varSrc);
  static UniqueFEIRStmt CreateStmtComment(const std::string &comment);
  static UniqueFEIRExpr ReadExprField(UniqueFEIRExpr expr, FieldID fieldID, UniqueFEIRType fieldType);
  static UniqueFEIRStmt AssginStmtField(const UniqueFEIRExpr &addrExpr, UniqueFEIRExpr srcExpr, FieldID fieldID);
  static bool IsZeroConstExpr(const UniqueFEIRExpr &expr);
};  // class FEIRBuilder
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_COMMON_FEIR_BUILDER_H
