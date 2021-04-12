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
#ifndef MPLFE_AST_INPUT_INCLUDE_AST_EXPR_H
#define MPLFE_AST_INPUT_INCLUDE_AST_EXPR_H
#include "ast_op.h"
#include "feir_stmt.h"

namespace maple {
class ASTDecl;
class ASTExpr {
 public:
  explicit ASTExpr(ASTOp o) : op(o) {}
  virtual ~ASTExpr() = default;
  UniqueFEIRExpr Emit2FEExpr(std::list<UniqueFEIRStmt> &stmts) const;

  ASTOp GetASTOp() {
    return op;
  }

 protected:
  virtual UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const = 0;
  ASTOp op;
};

class ASTImplicitCastExpr : public ASTExpr {
 public:
  ASTImplicitCastExpr() : ASTExpr(kASTOpCast) {}
  ~ASTImplicitCastExpr() = default;

  void SetASTExpr(ASTExpr *expr) {
    child = expr;
  }

  ASTExpr *GetASTExpr() const {
    return child;
  }

 protected:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  ASTExpr *child = nullptr;
};

class ASTDeclRefExpr : public ASTExpr {
 public:
  ASTDeclRefExpr() : ASTExpr(kASTOpRef) {}
  ~ASTDeclRefExpr() = default;
  void SetASTDecl(ASTDecl *astDecl);

  ASTDecl *GetASTDecl() const {
    return var;
  }

 protected:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  ASTDecl *var = nullptr;
};

class ASTUnaryOperatorExpr : public ASTExpr {
 public:
  explicit ASTUnaryOperatorExpr(ASTOp o) : ASTExpr(o) {}
  ~ASTUnaryOperatorExpr() = default;
  void SetUOExpr(ASTExpr*);
  void SetSubType(MIRType *type);
  void SetRefName(std::string name) {
    refName = name;
  }

  std::string GetRefName() {
    return refName;
  }

  MIRType *GetMIRType() {
    return subType;
  }

  MIRType *SetUOType(MIRType *type) {
    return uoType = type;
  }

  MIRType *GetUOType() {
    return uoType;
  }

  void SetPointeeLen(int64 len) {
    pointeeLen = len;
  }

  int64 GetPointeeLen() {
    return pointeeLen;
  }

 protected:
  ASTExpr *expr = nullptr;
  MIRType *subType;
  MIRType *uoType;
  std::string refName;
  int64 pointeeLen;
};

class ASTUOMinusExpr: public ASTUnaryOperatorExpr {
 public:
  ASTUOMinusExpr() : ASTUnaryOperatorExpr(kASTOpMinus) {}
  ~ASTUOMinusExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTUONotExpr: public ASTUnaryOperatorExpr {
 public:
  ASTUONotExpr() : ASTUnaryOperatorExpr(kASTOpNot) {}
  ~ASTUONotExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTUOLNotExpr: public ASTUnaryOperatorExpr {
 public:
  ASTUOLNotExpr() : ASTUnaryOperatorExpr(kASTOpLNot) {}
  ~ASTUOLNotExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTUOPostIncExpr: public ASTUnaryOperatorExpr {
 public:
  ASTUOPostIncExpr() : ASTUnaryOperatorExpr(kASTOpPostInc) {}
  ~ASTUOPostIncExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTUOPostDecExpr: public ASTUnaryOperatorExpr {
 public:
  ASTUOPostDecExpr() : ASTUnaryOperatorExpr(kASTOpPostDec) {}
  ~ASTUOPostDecExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTUOPreIncExpr: public ASTUnaryOperatorExpr {
 public:
  ASTUOPreIncExpr() : ASTUnaryOperatorExpr(kASTOpPreInc) {}
  ~ASTUOPreIncExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTUOPreDecExpr: public ASTUnaryOperatorExpr {
 public:
  ASTUOPreDecExpr() : ASTUnaryOperatorExpr(kASTOpPreDec) {}
  ~ASTUOPreDecExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTUOAddrOfExpr: public ASTUnaryOperatorExpr {
 public:
  ASTUOAddrOfExpr() : ASTUnaryOperatorExpr(kASTOpAddrOf) {}
  ~ASTUOAddrOfExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTUODerefExpr: public ASTUnaryOperatorExpr {
 public:
  ASTUODerefExpr() : ASTUnaryOperatorExpr(kASTOpDeref) {}
  ~ASTUODerefExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTUOPlusExpr: public ASTUnaryOperatorExpr {
 public:
  ASTUOPlusExpr() : ASTUnaryOperatorExpr(kASTOpPlus) {}
  ~ASTUOPlusExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTUORealExpr: public ASTUnaryOperatorExpr {
 public:
  ASTUORealExpr() : ASTUnaryOperatorExpr(kASTOpReal) {}
  ~ASTUORealExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTUOImagExpr: public ASTUnaryOperatorExpr {
 public:
  ASTUOImagExpr() : ASTUnaryOperatorExpr(kASTOpImag) {}
  ~ASTUOImagExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTUOExtensionExpr: public ASTUnaryOperatorExpr {
 public:
  ASTUOExtensionExpr() : ASTUnaryOperatorExpr(kASTOpExtension) {}
  ~ASTUOExtensionExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTUOCoawaitExpr: public ASTUnaryOperatorExpr {
 public:
  ASTUOCoawaitExpr() : ASTUnaryOperatorExpr(kASTOpCoawait) {}
  ~ASTUOCoawaitExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTPredefinedExpr : public ASTExpr {
 public:
  ASTPredefinedExpr() : ASTExpr(kASTOpPredefined) {}
  ~ASTPredefinedExpr() = default;
  void SetASTExpr(ASTExpr*);

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  ASTExpr *child;
};

class ASTOpaqueValueExpr : public ASTExpr {
 public:
  ASTOpaqueValueExpr() : ASTExpr(kASTOpOpaqueValue) {}
  ~ASTOpaqueValueExpr() = default;
  void SetASTExpr(ASTExpr*);

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  ASTExpr *child;
};

class ASTNoInitExpr : public ASTExpr {
 public:
  ASTNoInitExpr() : ASTExpr(kASTOpNoInitExpr) {}
  ~ASTNoInitExpr() = default;
  void SetNoInitType(MIRType *type);

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  MIRType *noInitType;
};

class ASTCompoundLiteralExpr : public ASTExpr {
 public:
  ASTCompoundLiteralExpr() : ASTExpr(kASTOpCompoundLiteralExp) {}
  ~ASTCompoundLiteralExpr() = default;
  void SetCompoundLiteralType(MIRType *clType);
  void SetASTExpr(ASTExpr*);

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  ASTExpr *child = nullptr;
  MIRType *compoundLiteralType;
};

class ASTOffsetOfExpr : public ASTExpr {
 public:
  ASTOffsetOfExpr() : ASTExpr(kASTOpOffsetOfExpr) {}
  ~ASTOffsetOfExpr() = default;
  void SetStructType(MIRType *stype);
  void SetFieldName(std::string);

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  MIRType *structType;
  std::string fieldName;
};

class ASTInitListExpr : public ASTExpr {
 public:
  ASTInitListExpr() : ASTExpr(kASTOpInitListExpr)  {}
  ~ASTInitListExpr() = default;
  void SetFillerExprs(ASTExpr*);
  void SetInitListType(MIRType *type);

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  std::vector<ASTExpr*> fillers;
  MIRType *initListType;
};

class ASTBinaryConditionalOperator : public ASTExpr {
 public:
  ASTBinaryConditionalOperator() : ASTExpr(kASTOpBinaryConditionalOperator) {}
  ~ASTBinaryConditionalOperator() = default;
  void SetRetType(MIRType *type);
  void SetCondExpr(ASTExpr*);
  void SetFalseExpr(ASTExpr*);

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  MIRType *retType;
  ASTExpr *cExpr = nullptr;
  ASTExpr *fExpr = nullptr;
};

class ASTBinaryOperatorExpr : public ASTExpr {
 public:
  explicit ASTBinaryOperatorExpr(ASTOp o) : ASTExpr(o) {}
  ~ASTBinaryOperatorExpr() override = default;

  void SetRetType(MIRType *type) {
    retType = type;
  }

  void SetLeftExpr(ASTExpr *leftExpr) {
    left = leftExpr;
  }

  void SetRightExpr(ASTExpr *rightExpr) {
    right = rightExpr;
  }

 protected:
  MIRType *retType;
  ASTExpr *left = nullptr;
  ASTExpr *right = nullptr;
};

class ASTImplicitValueInitExpr : public ASTExpr {
 public:
  ASTImplicitValueInitExpr() : ASTExpr(kASTImplicitValueInitExpr) {}
  ~ASTImplicitValueInitExpr() = default;

  void SetType(MIRType *srcType) {
    type = srcType;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  MIRType *type;
};

class ASTStringLiteral : public ASTExpr {
 public:
  ASTStringLiteral() : ASTExpr(kASTStringLiteral) {}
  ~ASTStringLiteral() = default;

  void SetType(MIRType *srcType) {
    type = srcType;
  }

  MIRType *GetType() const {
    return type;
  }

  void SetLength(size_t len) {
    length = len;
  }

  void SetCodeUnits(std::vector<uint32_t> &units) {
    codeUnits = std::move(units);
  }

  const std::vector<uint32_t> &GetCodeUnits() const {
    return codeUnits;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  MIRType *type;
  size_t length;
  std::vector<uint32_t> codeUnits;
};

class ASTArraySubscriptExpr : public ASTExpr {
 public:
  ASTArraySubscriptExpr() : ASTExpr(kASTSubscriptExpr) {}
  ~ASTArraySubscriptExpr() = default;

  void SetBaseExpr(ASTExpr *astExpr) {
    baseExpr = astExpr;
  }

  void SetIdxExpr(ASTExpr *astExpr) {
    idxExpr = astExpr;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  ASTExpr *baseExpr;
  ASTExpr *idxExpr;
};

class ASTExprUnaryExprOrTypeTraitExpr : public ASTExpr {
 public:
  ASTExprUnaryExprOrTypeTraitExpr() : ASTExpr(kASTExprUnaryExprOrTypeTraitExpr) {}
  ~ASTExprUnaryExprOrTypeTraitExpr() = default;

  void SetIsType(bool type) {
    isType = type;
  }

  void SetArgType(MIRType *type) {
    argType = type;
  }

  void SetArgExpr(ASTExpr *astExpr) {
    argExpr = astExpr;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  bool isType;
  MIRType *argType;
  ASTExpr *argExpr;
};

class ASTMemberExpr : public ASTExpr {
 public:
  ASTMemberExpr() : ASTExpr(kASTMemberExpr) {}
  ~ASTMemberExpr() = default;

  void SetBaseExpr(ASTExpr *astExpr) {
    baseExpr = astExpr;
  }

  void SetMemberName(std::string name) {
    memberName = std::move(name);
  }

  void SetIsArrow(bool arrow) {
    isArrow = arrow;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  ASTExpr *baseExpr;
  std::string memberName;
  bool isArrow;
};

class ASTDesignatedInitUpdateExpr : public ASTExpr {
 public:
  ASTDesignatedInitUpdateExpr() : ASTExpr(kASTASTDesignatedInitUpdateExpr) {}
  ~ASTDesignatedInitUpdateExpr() = default;

  void SetBaseExpr(ASTExpr *astExpr) {
    baseExpr = astExpr;
  }

  void SetUpdaterExpr(ASTExpr *astExpr) {
    updaterExpr = astExpr;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  ASTExpr *baseExpr;
  ASTExpr *updaterExpr;
};

class ASTBOAddExpr : public ASTBinaryOperatorExpr {
 public:
  ASTBOAddExpr() : ASTBinaryOperatorExpr(kASTOpAdd) {}
  ~ASTBOAddExpr() override = default;

 protected:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTBOMulExpr : public ASTBinaryOperatorExpr {
 public:
  ASTBOMulExpr() : ASTBinaryOperatorExpr(kASTOpMul) {}
  ~ASTBOMulExpr() override = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTBODivExpr : public ASTBinaryOperatorExpr {
 public:
  ASTBODivExpr() : ASTBinaryOperatorExpr(kASTOpDiv) {}
  ~ASTBODivExpr() override = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTBORemExpr : public ASTBinaryOperatorExpr {
 public:
  ASTBORemExpr() : ASTBinaryOperatorExpr(kASTOpRem) {}
  ~ASTBORemExpr() override = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTBOSubExpr : public ASTBinaryOperatorExpr {
 public:
  ASTBOSubExpr() : ASTBinaryOperatorExpr(kASTOpSub) {}
  ~ASTBOSubExpr() override = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTBOShlExpr : public ASTBinaryOperatorExpr {
 public:
  ASTBOShlExpr() : ASTBinaryOperatorExpr(kASTOpShl) {}
  ~ASTBOShlExpr() override = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTBOShrExpr : public ASTBinaryOperatorExpr {
 public:
  ASTBOShrExpr() : ASTBinaryOperatorExpr(kASTOpShr) {}
  ~ASTBOShrExpr() override = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTBOLTExpr : public ASTBinaryOperatorExpr {
 public:
  ASTBOLTExpr() : ASTBinaryOperatorExpr(kASTOpLT) {}
  ~ASTBOLTExpr() override = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTBOGTExpr : public ASTBinaryOperatorExpr {
 public:
  ASTBOGTExpr() : ASTBinaryOperatorExpr(kASTOpGT) {}
  ~ASTBOGTExpr() override = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTBOLEExpr : public ASTBinaryOperatorExpr {
 public:
  ASTBOLEExpr() : ASTBinaryOperatorExpr(kASTOpLE) {}
  ~ASTBOLEExpr() override = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTBOGEExpr : public ASTBinaryOperatorExpr {
 public:
  ASTBOGEExpr() : ASTBinaryOperatorExpr(kASTOpGE) {}
  ~ASTBOGEExpr() override = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTBOEQExpr : public ASTBinaryOperatorExpr {
 public:
  ASTBOEQExpr() : ASTBinaryOperatorExpr(kASTOpEQ) {}
  ~ASTBOEQExpr() override = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTBONEExpr : public ASTBinaryOperatorExpr {
 public:
  ASTBONEExpr() : ASTBinaryOperatorExpr(kASTOpNE) {}
  ~ASTBONEExpr() override = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTBOAndExpr : public ASTBinaryOperatorExpr {
 public:
  ASTBOAndExpr() : ASTBinaryOperatorExpr(kASTOpAnd) {}
  ~ASTBOAndExpr() override = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTBOXorExpr : public ASTBinaryOperatorExpr {
 public:
  ASTBOXorExpr() : ASTBinaryOperatorExpr(kASTOpXor) {}
  ~ASTBOXorExpr() override = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTBOOrExpr : public ASTBinaryOperatorExpr {
 public:
  ASTBOOrExpr() : ASTBinaryOperatorExpr(kASTOpOr) {}
  ~ASTBOOrExpr() override = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTBOLAndExpr : public ASTBinaryOperatorExpr {
 public:
  ASTBOLAndExpr() : ASTBinaryOperatorExpr(kASTOpLAnd) {}
  ~ASTBOLAndExpr() override = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTBOLOrExpr : public ASTBinaryOperatorExpr {
 public:
  ASTBOLOrExpr() : ASTBinaryOperatorExpr(kASTOpLOr) {}
  ~ASTBOLOrExpr() override = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTBOEqExpr : public ASTBinaryOperatorExpr {
 public:
  ASTBOEqExpr() : ASTBinaryOperatorExpr(kASTOpEQ) {}
  ~ASTBOEqExpr() override = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTBOAssign : public ASTBinaryOperatorExpr {
 public:
  ASTBOAssign() : ASTBinaryOperatorExpr(kASTOpAssign) {}
  ASTBOAssign(ASTOp o) : ASTBinaryOperatorExpr(o) {}
  ~ASTBOAssign() override = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTBOComma : public ASTBinaryOperatorExpr {
 public:
  ASTBOComma() : ASTBinaryOperatorExpr(kASTOpComma) {}
  ~ASTBOComma() override = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTBOPtrMemD : public ASTBinaryOperatorExpr {
 public:
  ASTBOPtrMemD() : ASTBinaryOperatorExpr(kASTOpPtrMemD) {}
  ~ASTBOPtrMemD() override = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTBOPtrMemI : public ASTBinaryOperatorExpr {
 public:
  ASTBOPtrMemI() : ASTBinaryOperatorExpr(kASTOpPtrMemI) {}
  ~ASTBOPtrMemI() override = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTCallExpr : public ASTExpr {
 public:
  ASTCallExpr() : ASTExpr(kASTOpCall) {}
  ~ASTCallExpr() = default;
  void SetCalleeExpr(ASTExpr *astExpr) {
    calleeExpr = astExpr;
  }

  ASTExpr *GetCalleeExpr() const {
    return calleeExpr;
  }

  void SetArgs(std::vector<ASTExpr*> &argsVector){
    args = std::move(argsVector);
  }

  const std::vector<ASTExpr*> &GetArgsExpr() const {
    return args;
  }

  void SetRetType(MIRType *typeIn) {
    retType = typeIn;
  }

  MIRType *GetRetType() const {
    return retType;
  }

  void SetFuncName(const std::string &name) {
    funcName = name;
  }

  const std::string &GetFuncName() const {
    return funcName;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;

  std::vector<ASTExpr*> args;
  ASTExpr *calleeExpr = nullptr;
  MIRType *retType = nullptr;
  std::string funcName;
};

class ASTParenExpr : public ASTExpr {
 public:
  ASTParenExpr() : ASTExpr(kASTParen) {}
  ~ASTParenExpr() = default;

  void SetASTExpr(ASTExpr *astExpr) {
    child = astExpr;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  ASTExpr *child = nullptr;
};

class ASTIntegerLiteral : public ASTExpr {
 public:
  ASTIntegerLiteral() : ASTExpr(kASTIntegerLiteral) {}
  ~ASTIntegerLiteral() = default;

  uint64 GetVal() const {
    return val;
  }

  void SetVal(uint64 valIn) {
    val = valIn;
  }

  void SetType(PrimType pType) {
    type = pType;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;

  uint64 val;
  PrimType type;
};

class ASTFloatingLiteral : public ASTExpr {
 public:
  ASTFloatingLiteral() : ASTExpr(kASTFloatingLiteral) {}
  ~ASTFloatingLiteral() = default;

  double GetVal() const {
    return val;
  }

  void SetVal(double valIn) {
    val = valIn;
  }

  void SetFlag(bool tf) {
    isFloat = tf;
  }

  bool GetFlag() {
    return isFloat;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  double val;
  bool isFloat;
};

class ASTCharacterLiteral : public ASTExpr {
 public:
  ASTCharacterLiteral() : ASTExpr(kASTCharacterLiteral) {}
  ~ASTCharacterLiteral() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTVAArgExpr : public ASTExpr {
 public:
  ASTVAArgExpr() : ASTExpr(kASTVAArgExpr) {}
  ~ASTVAArgExpr() = default;

  void SetASTExpr(ASTExpr *astExpr) {
    child = astExpr;
  }

 private:
  ASTExpr *child = nullptr;
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTCompoundAssignOperatorExpr : public ASTBinaryOperatorExpr {
 public:
  ASTCompoundAssignOperatorExpr() : ASTBinaryOperatorExpr(kCpdAssignOp) {}
  ~ASTCompoundAssignOperatorExpr() override = default;

 protected:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};


class ASTConstantExpr : public ASTExpr {
 public:
  ASTConstantExpr() : ASTExpr(kConstantExpr) {}
  ~ASTConstantExpr() = default;
  void SetASTExpr(ASTExpr *astExpr) {
    child = astExpr;
  }

 private:
  ASTExpr *child = nullptr;
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTImaginaryLiteral : public ASTExpr {
 public:
  ASTImaginaryLiteral() : ASTExpr(kASTImaginaryLiteral) {}
  ~ASTImaginaryLiteral() = default;
  void SetASTExpr(ASTExpr *astExpr) {
    child = astExpr;
  }

 private:
  ASTExpr *child = nullptr;
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTConditionalOperator : public ASTExpr {
 public:
  ASTConditionalOperator() : ASTExpr(kASTConditionalOperator) {}
  ~ASTConditionalOperator() = default;

  void SetCondExpr(ASTExpr *astExpr) {
    condExpr = astExpr;
  }

  void SetTrueExpr(ASTExpr *astExpr) {
    trueExpr = astExpr;
  }

  void SetFalseExpr(ASTExpr *astExpr) {
    falseExpr = astExpr;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  ASTExpr *condExpr = nullptr;
  ASTExpr *trueExpr = nullptr;
  ASTExpr *falseExpr = nullptr;
};

class ASTCStyleCastExpr : public ASTExpr {
 public:
  ASTCStyleCastExpr() : ASTExpr(kASTOpCast) {}
  ~ASTCStyleCastExpr() = default;

  void SetSubExpr(ASTExpr *sub) {
    child = sub;
  }

  void SetSrcType(MIRType *src) {
    srcType = src;
  }

  void SetDestType(MIRType *dest) {
    destType = dest;
  }

  MIRType *GetSrcType() const {
    return srcType;
  }

  MIRType *SetDestType() const {
    return destType;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  ASTExpr *child = nullptr;
  MIRType *srcType;
  MIRType *destType;
};

class ASTArrayInitLoopExpr : public ASTExpr {
 public:
  ASTArrayInitLoopExpr() : ASTExpr(kASTOpArrayInitLoop) {}
  ~ASTArrayInitLoopExpr() = default;

  void SetCommonExpr(ASTExpr *expr) {
    commonExpr = expr;
  }

  const ASTExpr *GetCommonExpr() const {
    return commonExpr;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  ASTExpr* commonExpr = nullptr;
};

class ASTArrayInitIndexExpr : public ASTExpr {
 public:
  ASTArrayInitIndexExpr() : ASTExpr(kASTOpArrayInitLoop) {}
  ~ASTArrayInitIndexExpr() = default;

  void SetPrimType(MIRType *pType) {
    primType = pType;
  }

  void SetValue(std::string val) {
    value = val;
  }

  MIRType *GetPrimeType() const {
    return primType;
  }

  std::string GetValue() const {
    return value;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  MIRType *primType;
  std::string value;
};

class ASTExprWithCleanups : public ASTExpr {
 public:
  ASTExprWithCleanups() : ASTExpr(kASTOpExprWithCleanups) {}
  ~ASTExprWithCleanups() = default;

  void SetSubExpr(ASTExpr *sub) {
    subExpr = sub;
  }

  ASTExpr *GetSubExpr() const {
    return subExpr;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  ASTExpr *subExpr = nullptr;
};

class ASTMaterializeTemporaryExpr : public ASTExpr {
 public:
  ASTMaterializeTemporaryExpr() : ASTExpr(kASTOpMaterializeTemporary) {}
  ~ASTMaterializeTemporaryExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTSubstNonTypeTemplateParmExpr : public ASTExpr {
 public:
  ASTSubstNonTypeTemplateParmExpr() : ASTExpr(kASTOpSubstNonTypeTemplateParm) {}
  ~ASTSubstNonTypeTemplateParmExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTDependentScopeDeclRefExpr : public ASTExpr {
 public:
  ASTDependentScopeDeclRefExpr() : ASTExpr(kASTOpDependentScopeDeclRef) {}
  ~ASTDependentScopeDeclRefExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTAtomicExpr : public ASTExpr {
 public:
  ASTAtomicExpr() : ASTExpr(kASTOpAtomic) {}
  ~ASTAtomicExpr() = default;

  void SetType(MIRType *ty) {
    type = ty;
  }

  void SetRefType(MIRType *ref) {
    refType = ref;
  }

  void SetAtomicOp(ASTAtomicOp op) {
    atomicOp = op;
  }

  MIRType *GetType() const {
    return type;
  }

  MIRType *GetRefType() const {
    return refType;
  }

  ASTAtomicOp GetAtomicOp() const {
    return atomicOp;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  MIRType *type;
  MIRType *refType;
  ASTAtomicOp atomicOp;
};
}
#endif //MPLFE_AST_INPUT_INCLUDE_AST_EXPR_H
