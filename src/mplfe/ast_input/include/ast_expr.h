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
class ASTStmt;
struct ASTValue {
  union Value {
    int32 i32;
    float f32;
    int64 i64;
    double f64;
    UStrIdx strIdx;
  } val = { 0 };
  PrimType pty = PTY_begin;

  MIRConst *Translate2MIRConst() const;
};

enum ParentFlag {
  kNoParent,
  kArrayParent,
  kStructParent
};

class ASTExpr {
 public:
  explicit ASTExpr(ASTOp o) : op(o) {}
  virtual ~ASTExpr() = default;
  UniqueFEIRExpr Emit2FEExpr(std::list<UniqueFEIRStmt> &stmts) const;
  UniqueFEIRExpr ImplicitInitFieldValue(MIRType *type, std::list<UniqueFEIRStmt> &stmts) const;

  virtual MIRType *GetType() {
    return mirType;
  }

  void SetType(MIRType *type) {
    mirType = type;
  }

  void SetASTDecl(ASTDecl *astDecl) {
    refedDecl = astDecl;
  }

  ASTDecl *GetASTDecl() const {
    return GetASTDeclImpl();
  }

  ASTOp GetASTOp() {
    return op;
  }

  void SetConstantValue(ASTValue *val) {
    isConstantFolded = (val != nullptr);
    value = val;
  }

  ASTValue *GetConstantValue() const {
    return GetConstantValueImpl();
  }

  MIRConst *GenerateMIRConst() const {
    return GenerateMIRConstImpl();
  }

  void SetSrcLOC(uint32 fileIdx, uint32 lineNum) {
    srcFileIdx = fileIdx;
    srcFileLineNum = lineNum;
  }

  uint32 GetSrcFileIdx() const {
    return srcFileIdx;
  }

  uint32 GetSrcFileLineNum() const {
    return srcFileLineNum;
  }

 protected:
  virtual ASTValue *GetConstantValueImpl() const {
    return value;
  }
  virtual MIRConst *GenerateMIRConstImpl() const;
  virtual UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const = 0;

  virtual ASTDecl *GetASTDeclImpl() const {
    return refedDecl;
  }

  ASTOp op;
  MIRType *mirType = nullptr;
  ASTDecl *refedDecl = nullptr;
  bool isConstantFolded = false;
  ASTValue *value = nullptr;

  uint32 srcFileIdx = 0;
  uint32 srcFileLineNum = 0;
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

  MIRType *GetType() override {
    return child->GetType();
  }

  void SetSrcType(MIRType *type) {
    src = type;
  }

  const MIRType *GetSrcType() const {
    return src;
  }

  void SetDstType(MIRType *type) {
    dst = type;
  }

  const MIRType *GetDstType() const {
    return dst;
  }

  void SetNeededCvt(bool cvt) {
    isNeededCvt = cvt;
  }

  bool IsNeededCvt(const UniqueFEIRExpr &expr) const;

  void SetComplexType(MIRType *type) {
    complexType = type;
  }

  void SetComplexCastKind(bool flag) {
    imageZero = flag;
  }

  void SetIsArrayToPointerDecay(bool flag) {
    isArrayToPointerDecay = flag;
  }

  bool IsBuilinFunc() const {
    return isBuilinFunc;
  }

  void SetBuilinFunc(bool flag) {
    isBuilinFunc = flag;
  }

 protected:
  ASTValue *GetConstantValueImpl() const override;
  MIRConst *GenerateMIRConstImpl() const override;
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;

  ASTDecl *GetASTDeclImpl() const override {
    return child->GetASTDecl();
  }

 private:
  MIRConst *GenerateMIRDoubleConst() const;
  MIRConst *GenerateMIRFloatConst() const;
  MIRConst *GenerateMIRIntConst() const;
  ASTExpr *child = nullptr;
  MIRType *src = nullptr;
  MIRType *dst = nullptr;
  bool isNeededCvt = false;
  MIRType *complexType = nullptr;
  bool imageZero = false;
  bool isArrayToPointerDecay = false;
  bool isBuilinFunc = false;
};

class ASTDeclRefExpr : public ASTExpr {
 public:
  ASTDeclRefExpr() : ASTExpr(kASTOpRef) {}
  ~ASTDeclRefExpr() = default;

 protected:
  MIRConst *GenerateMIRConstImpl() const override;
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTUnaryOperatorExpr : public ASTExpr {
 public:
  explicit ASTUnaryOperatorExpr(ASTOp o) : ASTExpr(o) {}
  ~ASTUnaryOperatorExpr() = default;
  void SetUOExpr(ASTExpr*);
  void SetSubType(MIRType *type);

  MIRType *GetMIRType() {
    return subType;
  }

  MIRType *SetUOType(MIRType *type) {
    return uoType = type;
  }

  const MIRType *GetUOType() const {
    return uoType;
  }

  void SetPointeeLen(int64 len) {
    pointeeLen = len;
  }

  int64 GetPointeeLen() {
    return pointeeLen;
  }

  void SetGlobal(bool isGlobalArg) {
    isGlobal = isGlobalArg;
  }

  bool IsGlobal() {
    return isGlobal;
  }

 protected:
  bool isGlobal = false;
  ASTExpr *expr = nullptr;
  MIRType *subType;
  MIRType *uoType;
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
  ASTUOPostIncExpr() : ASTUnaryOperatorExpr(kASTOpPostInc), tempVarName(FEUtils::GetSequentialName("postinc_")) {}
  ~ASTUOPostIncExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  std::string tempVarName;
};

class ASTUOPostDecExpr: public ASTUnaryOperatorExpr {
 public:
  ASTUOPostDecExpr() : ASTUnaryOperatorExpr(kASTOpPostDec), tempVarName(FEUtils::GetSequentialName("postdec_")) {}
  ~ASTUOPostDecExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  std::string tempVarName;
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
  std::string tempVarName;
};

class ASTUOAddrOfExpr: public ASTUnaryOperatorExpr {
 public:
  ASTUOAddrOfExpr() : ASTUnaryOperatorExpr(kASTOpAddrOf) {}
  ~ASTUOAddrOfExpr() = default;

 protected:
  MIRConst *GenerateMIRConstImpl() const override;

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

  void SetElementType(MIRType *type) {
    elementType = type;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  MIRType *elementType;
};

class ASTUOImagExpr: public ASTUnaryOperatorExpr {
 public:
  ASTUOImagExpr() : ASTUnaryOperatorExpr(kASTOpImag) {}
  ~ASTUOImagExpr() = default;

  void SetElementType(MIRType *type) {
    elementType = type;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  MIRType *elementType;
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
  void SetFieldName(std::string fName);

  void SetOffset(size_t val) {
    offset = val;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  MIRType *structType;
  std::string fieldName;
  size_t offset;
};

class ASTInitListExpr : public ASTExpr {
 public:
  ASTInitListExpr() : ASTExpr(kASTOpInitListExpr)  {}
  ~ASTInitListExpr() = default;
  void SetFillerExprs(ASTExpr*);
  void SetInitListType(MIRType *type);

  MIRType *GetInitListType() {
    return initListType;
  }

  void SetInitListVarName(const std::string &argVarName) {
    varName = argVarName;
  }

  void SetParentFlag(ParentFlag argParentFlag) {
    parentFlag = argParentFlag;
  }

  void SetIsUnionInitListExpr(bool flag) {
    isUnionInitListExpr = flag;
  }

  bool IsUnionInitListExpr() const {
    return isUnionInitListExpr;
  }

  void SetHasArrayFiller(bool flag) {
    hasArrayFiller = flag;
  }

  bool HasArrayFiller() const {
    return hasArrayFiller;
  }

 private:
  MIRConst *GenerateMIRConstImpl() const override;
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  void Emit2FEExprForStruct4ArrayElemIsStruct(uint32 i, std::list<UniqueFEIRStmt> &stmts) const;
  void Emit2FEExprForArray(std::list<UniqueFEIRStmt> &stmts) const;
  void Emit2FEExprForArrayForNest(UniqueFEIRType typeNative, UniqueFEIRExpr arrayExpr,
                                  std::list<UniqueFEIRStmt> &stmts) const;
  void Emit2FEExprForStruct(std::list<UniqueFEIRStmt> &stmts) const;
  MIRConst *GenerateMIRConstForArray() const;
  MIRConst *GenerateMIRConstForStruct() const;
  std::vector<ASTExpr*> fillers;
  MIRType *initListType;
  std::string varName;
  ParentFlag parentFlag = kNoParent;
  bool isUnionInitListExpr = false;
  bool hasArrayFiller = false;
};

class ASTBinaryConditionalOperator : public ASTExpr {
 public:
  ASTBinaryConditionalOperator() : ASTExpr(kASTOpBinaryConditionalOperator) {}
  ~ASTBinaryConditionalOperator() = default;
  void SetCondExpr(ASTExpr *expr);
  void SetFalseExpr(ASTExpr *expr);

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  ASTExpr *condExpr = nullptr;
  ASTExpr *falseExpr = nullptr;
};

class ASTBinaryOperatorExpr : public ASTExpr {
 public:
  explicit ASTBinaryOperatorExpr(ASTOp o) : ASTExpr(o) {}
  ASTBinaryOperatorExpr() : ASTExpr(kASTOpBO), varName(FEUtils::GetSequentialName("shortCircuit_")) {}

  ~ASTBinaryOperatorExpr() override = default;

  void SetRetType(MIRType *type) {
    retType = type;
  }

  MIRType *GetRetType() const {
    return retType;
  }

  void SetLeftExpr(ASTExpr *expr) {
    leftExpr = expr;
  }

  void SetRightExpr(ASTExpr *expr) {
    rightExpr = expr;
  }

  void SetOpcode(Opcode op) {
    opcode = op;
  }

  Opcode GetOp() const {
    return opcode;
  }

  void SetComplexElementType(MIRType *type) {
    complexElementType = type;
  }

  void SetComplexLeftRealExpr(ASTExpr *expr) {
    leftRealExpr = expr;
  }

  void SetComplexLeftImagExpr(ASTExpr *expr) {
    leftImagExpr = expr;
  }

  void SetComplexRightRealExpr(ASTExpr *expr) {
    rightRealExpr = expr;
  }

  void SetComplexRightImagExpr(ASTExpr *expr) {
    rightImagExpr = expr;
  }

  void SetCvtNeeded(bool needed) {
    cvtNeeded = needed;
  }

 protected:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;

  Opcode opcode;
  MIRType *retType = nullptr;
  MIRType *complexElementType = nullptr;
  ASTExpr *leftExpr = nullptr;
  ASTExpr *rightExpr = nullptr;
  ASTExpr *leftRealExpr = nullptr;
  ASTExpr *leftImagExpr = nullptr;
  ASTExpr *rightRealExpr = nullptr;
  ASTExpr *rightImagExpr = nullptr;
  bool cvtNeeded = false;
  std::string varName;
};

class ASTImplicitValueInitExpr : public ASTExpr {
 public:
  ASTImplicitValueInitExpr() : ASTExpr(kASTImplicitValueInitExpr) {}
  ~ASTImplicitValueInitExpr() = default;

  void SetType(MIRType *srcType) {
    type = srcType;
  }

 protected:
  MIRConst *GenerateMIRConstImpl() const override;

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

  MIRType *GetType() override {
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

 protected:
  MIRConst *GenerateMIRConstImpl() const override;

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

  ASTExpr *GetBaseExpr() const {
    return baseExpr;
  }

  void SetIdxExpr(ASTExpr *astExpr) {
    idxExprs.push_back(astExpr);
  }

  std::vector<ASTExpr*> GetIdxExpr() const {
    return idxExprs;
  }

  void SetBaseExprType(MIRType *ty) {
    baseExprTypes.push_back(ty);
  }

  const std::vector<MIRType*> &GetBaseExprType() const {
    return baseExprTypes;
  }

  int32 TranslateArraySubscript2Offset() const;

  void SetMemberExpr(ASTExpr &astExpr) {
    memberExpr = &astExpr;
  }

  ASTExpr* GetMemberExpr() const {
    return memberExpr;
  }

  std::string GetBaseExprVarName() const {
    return baseExprVarName;
  }

  void SetBaseExprVarName(std::string argBaseExprVarName) {
    baseExprVarName = argBaseExprVarName;
  }

  void SetAddrOfFlag(bool flag) {
    isAddrOf = flag;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  ASTExpr *baseExpr = nullptr;
  ASTExpr *memberExpr = nullptr;
  std::vector<MIRType*> baseExprTypes;
  std::vector<ASTExpr*> idxExprs;
  bool isAddrOf = false;
  std::string baseExprVarName;
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

  ASTExpr *GetBaseExpr() const {
    return baseExpr;
  }

  void SetMemberName(std::string name) {
    memberName = std::move(name);
  }

  std::string GetMemberName() const {
    return memberName;
  }

  void SetMemberType(MIRType *type) {
    memberType = type;
  }

  void SetBaseType(MIRType *type) {
    baseType = type;
  }

  MIRType *GetMemberType() const {
    return memberType;
  }

  MIRType *GetBaseType() const {
    return baseType;
  }

  void SetIsArrow(bool arrow) {
    isArrow = arrow;
  }

  bool GetIsArrow() const {
    return isArrow;
  }

  ASTMemberExpr *findFinalMember(ASTMemberExpr *startExpr, std::list<std::string> &memberNames) const;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  void Emit2FEExprImplForArrayElemIsStruct(UniqueFEIRExpr baseFEExpr, std::string &tmpStructName,
                                           std::list<UniqueFEIRStmt> &stmts) const;
  ASTExpr *baseExpr = nullptr;
  std::string memberName;
  MIRType *memberType = nullptr;
  MIRType *baseType = nullptr;
  bool isArrow = false;
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

  void SetInitListType(MIRType *type) {
    initListType = type;
  }

  MIRType *GetInitListType() const {
    return initListType;
  }

  void SetInitListVarName (std::string name) {
    initListVarName = name;
  }

  std::string GetInitListVarName () const {
    return initListVarName;
  }

 private:
  MIRConst *GenerateMIRConstImpl() const override;
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  ASTExpr *baseExpr;
  ASTExpr *updaterExpr;
  MIRType *initListType;
  std::string initListVarName;
};

class ASTAssignExpr : public ASTBinaryOperatorExpr {
 public:
  ASTAssignExpr() : ASTBinaryOperatorExpr(kASTOpAssign), isCompoundAssign(false) {}
  ~ASTAssignExpr() override = default;

  void SetIsCompoundAssign(bool argIsCompoundAssign) {
    isCompoundAssign = argIsCompoundAssign;
  }

 protected:
  UniqueFEIRExpr ProcessAssign(std::list<UniqueFEIRStmt> &stmts, UniqueFEIRExpr leftExpr,
                               UniqueFEIRExpr rightExpr) const;
  void ProcessAssign4ExprArrayStoreForC(std::list<UniqueFEIRStmt> &stmts, UniqueFEIRExpr leftFEExpr,
                                        UniqueFEIRExpr rightFEExpr) const;
 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  bool isCompoundAssign;
};

class ASTBOComma : public ASTBinaryOperatorExpr {
 public:
  ASTBOComma() : ASTBinaryOperatorExpr(kASTOpComma) {}
  ~ASTBOComma() override = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTBOPtrMemExpr : public ASTBinaryOperatorExpr {
 public:
  ASTBOPtrMemExpr() : ASTBinaryOperatorExpr(kASTOpPtrMemD) {}
  ~ASTBOPtrMemExpr() override = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTCallExpr : public ASTExpr {
 public:
  ASTCallExpr() : ASTExpr(kASTOpCall), varName(FEUtils::GetSequentialName("retVar_")) {}
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

  void SetFuncAttrs(const FuncAttrs &attrs) {
    funcAttrs = attrs;
  }

  const FuncAttrs &GetFuncAttrs() const {
    return funcAttrs;
  }

  void SetIcall(bool icall) {
    isIcall = icall;
  }

  bool IsIcall() const {
    return isIcall;
  }

  std::string CvtBuiltInFuncName(std::string builtInName) const;

 private:
  using FuncPtrBuiltinFunc = UniqueFEIRExpr (ASTCallExpr::*)(std::list<UniqueFEIRStmt> &stmts) const;
  static std::map<std::string, FuncPtrBuiltinFunc> InitFuncPtrMap();
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  UniqueFEIRExpr Emit2FEExprCall(std::list<UniqueFEIRStmt> &stmts) const;
  UniqueFEIRExpr Emit2FEExprICall(std::list<UniqueFEIRStmt> &stmts) const;

  static std::map<std::string, FuncPtrBuiltinFunc> funcPtrMap;
  std::vector<ASTExpr*> args;
  ASTExpr *calleeExpr = nullptr;
  MIRType *retType = nullptr;
  std::string funcName;
  FuncAttrs funcAttrs;
  bool isIcall = false;
  std::string varName;
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

enum FloatKind {
  F32,
  F64
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

  void SetKind(FloatKind argKind) {
    kind = argKind;
  }

  FloatKind GetKind() const {
    return kind;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  double val;
  FloatKind kind;
};

class ASTCharacterLiteral : public ASTExpr {
 public:
  ASTCharacterLiteral() : ASTExpr(kASTCharacterLiteral) {}
  ~ASTCharacterLiteral() = default;

  int8 GetVal() const {
    return val;
  }

  void SetVal(int8 valIn) {
    val = valIn;
  }

  void SetPrimType(PrimType primType) {
    type = primType;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  int8 val;
  PrimType type;
};

struct VaArgInfo {
  bool isGPReg;  // GP or FP/SIMD arg reg
  int regOffset;
  int stackOffset;
  // If the argument type is a Composite Type that is larger than 16 bytes,
  // then the argument is copied to memory allocated by the caller and replaced by a pointer to the copy.
  bool isCopyedMem;
  bool isHFA;  // Homogeneous Floating-point Aggregate
};

class ASTVAArgExpr : public ASTExpr {
 public:
  ASTVAArgExpr() : ASTExpr(kASTVAArgExpr) {}
  ~ASTVAArgExpr() = default;

  void SetASTExpr(ASTExpr *astExpr) {
    child = astExpr;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  VaArgInfo ProcessValistArgInfo(MIRType &type) const;
  bool IsHFAType(MIRStructType &type) const;
  void CvtHFA2Struct(MIRStructType &structType, UniqueFEIRVar vaArgVar, UniqueFEIRVar copyedVar,
                     std::list<UniqueFEIRStmt> &stmts) const;

  ASTExpr *child = nullptr;
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

  void SetComplexType(MIRType *structType) {
    complexType = structType;
  }

  void SetElemType(MIRType *type) {
    elemType = type;
  }

 private:
  MIRType *complexType = nullptr;
  MIRType *elemType = nullptr;
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

  void SetCanCastArray(bool shouldCastArr) {
    canCastArray = shouldCastArr;
  }

  bool CanCastArray() const {
    return canCastArray;
  }

  void SetDecl(ASTDecl *d) {
    decl = d;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  ASTExpr *child = nullptr;
  MIRType *srcType = nullptr;
  MIRType *destType = nullptr;
  bool canCastArray = false;
  ASTDecl *decl = nullptr;
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

  MIRType *GetType() override {
    return type;
  }

  MIRType *GetRefType() const {
    return refType;
  }

  ASTAtomicOp GetAtomicOp() const {
    return atomicOp;
  }

  void SetValExpr1(ASTExpr *val) {
    valExpr1 = val;
  }

  void SetValExpr2(ASTExpr *val) {
    valExpr2 = val;
  }

  void SetObjExpr(ASTExpr *obj) {
    objExpr = obj;
  }

  ASTExpr *GetValExpr1() const {
    return valExpr1;
  }

  ASTExpr *GetValExpr2() const {
    return valExpr2;
  }

  ASTExpr *GetObjExpr() const {
    return objExpr;
  }

  void SetVal1Type(MIRType *ty) {
    val1Type = ty;
  }

  void SetVal2Type(MIRType *ty) {
    val2Type = ty;
  }

  void SetFromStmt(bool fromStmt) {
    isFromStmt = fromStmt;
  }

  bool IsFromStmt() const {
    return isFromStmt;
  }
 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  MIRType *type = nullptr;
  MIRType *refType = nullptr;
  MIRType *val1Type = nullptr;
  MIRType *val2Type = nullptr;
  ASTExpr *objExpr = nullptr;
  ASTExpr *valExpr1 = nullptr;
  ASTExpr *valExpr2 = nullptr;
  ASTAtomicOp atomicOp;
  bool isFromStmt = false;
};

class ASTExprStmtExpr : public ASTExpr {
 public:
  ASTExprStmtExpr() : ASTExpr(kASTOpStmtExpr) {}
  ~ASTExprStmtExpr() = default;
  void SetCompoundStmt(ASTStmt *sub) {
    cpdStmt = sub;
  }

  ASTStmt *GetSubExpr() const {
    return cpdStmt;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;

  ASTStmt *cpdStmt = nullptr;
};
}
#endif //MPLFE_AST_INPUT_INCLUDE_AST_EXPR_H
