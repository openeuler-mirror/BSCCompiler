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
#include <variant>
#include "ast_op.h"
#include "feir_stmt.h"

namespace maple {
class ASTDecl;
class ASTStmt;
struct ASTValue {
  union Value {
    uint8 u8;
    uint16 u16;
    uint32 u32;
    uint64 u64;
    int8 i8;
    int16 i16;
    int32 i32;
    float f32;
    int64 i64;
    double f64;
    UStrIdx strIdx;
  } val = { 0 };
  PrimType pty = PTY_begin;

  PrimType GetPrimType() const {
    return pty;
  }

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

  virtual MIRType *GetType() const {
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

  bool IsConstantFolded() const {
    return isConstantFolded;
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

  virtual void SetShortCircuitParent() {}

  virtual bool GetShortCircuitParent() const {
    return false;
  }

  virtual bool GetShortCircuit() const {
    return false;
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

  virtual UniqueFEIRExpr CreateZeroExprCompare(UniqueFEIRExpr feExpr, Opcode op) const;

  ASTOp op;
  MIRType *mirType = nullptr;
  ASTDecl *refedDecl = nullptr;
  bool isConstantFolded = false;
  ASTValue *value = nullptr;

  uint32 srcFileIdx = 0;
  uint32 srcFileLineNum = 0;
};

class ASTCastExpr : public ASTExpr {
 public:
  ASTCastExpr() : ASTExpr(kASTOpCast) {}
  ~ASTCastExpr() = default;

  void SetASTExpr(ASTExpr *expr) {
    child = expr;
  }

  ASTExpr *GetASTExpr() const {
    return child;
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

  bool IsNeededCvt(const UniqueFEIRExpr &expr) const {
    if (!isNeededCvt || expr == nullptr || dst == nullptr) {
      return false;
    }
    PrimType srcPrimType = expr->GetPrimType();
    return srcPrimType != dst->GetPrimType() && srcPrimType != PTY_agg && srcPrimType != PTY_void;
  }

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

  void SetUnionCast(bool flag) {
    isUnoinCast = flag;
  }

  void SetBitCast(bool flag) {
    isBitCast = flag;
  }

  void SetVectorSplat(bool flag) {
    isVectorSplat = flag;
  }

 protected:
  MIRConst *GenerateMIRConstImpl() const override;
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;

  ASTDecl *GetASTDeclImpl() const override {
    return child->GetASTDecl();
  }

  UniqueFEIRExpr Emit2FEExprForComplex(UniqueFEIRExpr subExpr, UniqueFEIRType srcType,
                                       std::list<UniqueFEIRStmt> &stmts) const;
 private:
  MIRConst *GenerateMIRDoubleConst() const;
  MIRConst *GenerateMIRFloatConst() const;
  MIRConst *GenerateMIRIntConst() const;
  UniqueFEIRExpr EmitExprVdupVector(PrimType primtype, UniqueFEIRExpr &subExpr) const;
  ASTExpr *child = nullptr;
  MIRType *src = nullptr;
  MIRType *dst = nullptr;
  bool isNeededCvt = false;
  bool isBitCast = false;
  MIRType *complexType = nullptr;
  bool imageZero = false;
  bool isArrayToPointerDecay = false;
  bool isBuilinFunc = false;
  bool isUnoinCast = false;
  bool isVectorSplat = false;
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

  ASTExpr *GetUOExpr() const {
    return expr;
  }

  void SetSubType(MIRType *type);

  MIRType *GetMIRType() {
    return subType;
  }

  void SetUOType(MIRType *type) {
    uoType = type;
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

  UniqueFEIRExpr ASTUOSideEffectExpr(Opcode op, std::list<UniqueFEIRStmt> &stmts,
      std::string varName = "", bool post = false) const;

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
  ASTUOLNotExpr() : ASTUnaryOperatorExpr(kASTOpLNot), tempVarName(FEUtils::GetSequentialName("tempVar_")) {}
  ~ASTUOLNotExpr() = default;

  void SetShortCircuitParent() override {
    shortCircuitParent = true;
  }

  bool GetShortCircuitParent() const override {
    return shortCircuitParent;
  }

  bool GetShortCircuit() const override {
    return AstShortCircuitUtil::Instance().GetIsShortCircuit();
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  bool shortCircuitParent = false;
  std::string tempVarName;
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

class ASTUOAddrOfLabelExpr : public ASTUnaryOperatorExpr {
 public:
  ASTUOAddrOfLabelExpr() : ASTUnaryOperatorExpr(kASTOpAddrOfLabel) {}
  ~ASTUOAddrOfLabelExpr() = default;

  void SetLabelName(const std::string &name) {
    labelName = name;
  }

 protected:
  MIRConst *GenerateMIRConstImpl() const override;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  std::string labelName;
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
  MIRType *elementType = nullptr;
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
  MIRType *elementType = nullptr;
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

  MIRType *GetCompoundLiteralType() const {
    return compoundLiteralType;
  }

  void SetASTExpr(ASTExpr*);

  ASTExpr* GetASTExpr() const {
    return child;
  }

  void SetInitName(std::string argInitName) {
    initName = argInitName;
  }

  std::string GetInitName() const {
    return initName;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  ASTExpr *child = nullptr;
  MIRType *compoundLiteralType;
  std::string initName;
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
  void SetInitExprs(ASTExpr *astExpr);
  void SetInitListType(MIRType *type);

  MIRType *GetInitListType() {
    return initListType;
  }

  std::vector<ASTExpr*> GetInitExprs() const {
    return initExprs;
  }

  void SetInitListVarName(const std::string &argVarName) {
    varName = argVarName;
  }

  std::string GetInitListVarName() const {
    return varName;
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

  void SetTransparent(bool flag) {
    isTransparent = flag;
  }

  bool IsTransparent() const {
    return isTransparent;
  }

  void SetArrayFiller(ASTExpr *expr) {
    arrayFillerExpr = expr;
  }

  ASTExpr *GetArrayFillter() {
    return arrayFillerExpr;
  }

  void SetHasVectorType(bool flag) {
    hasVectorType = flag;
  }

  bool HasVectorType() const {
    return hasVectorType;
  }

 private:
  MIRConst *GenerateMIRConstImpl() const override;
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  void ProcessInitList(std::variant<std::pair<UniqueFEIRVar, FieldID>, UniqueFEIRExpr> &base, ASTInitListExpr *initList,
                       std::list<UniqueFEIRStmt> &stmts) const;
  void ProcessArrayInitList(UniqueFEIRExpr addrOfArray, ASTInitListExpr *initList,
                            std::list<UniqueFEIRStmt> &stmts) const;
  void ProcessStructInitList(std::variant<std::pair<UniqueFEIRVar, FieldID>, UniqueFEIRExpr> &base,
                             ASTInitListExpr *initList, std::list<UniqueFEIRStmt> &stmts) const;
  void ProcessVectorInitList(std::variant<std::pair<UniqueFEIRVar, FieldID>, UniqueFEIRExpr> &base,
                             ASTInitListExpr *initList, std::list<UniqueFEIRStmt> &stmts) const;
  MIRIntrinsicID SetVectorSetLane(MIRType &type) const;
  void ProcessDesignatedInitUpdater(std::variant<std::pair<UniqueFEIRVar, FieldID>, UniqueFEIRExpr> &base,
                                    ASTExpr *expr, std::list<UniqueFEIRStmt> &stmts) const;
  void ProcessStringLiteralInitList(UniqueFEIRExpr addrOfCharArray, UniqueFEIRExpr addrOfStringLiteral,
                                    uint32 stringLength, std::list<UniqueFEIRStmt> &stmts) const;
  MIRConst *GenerateMIRConstForArray() const;
  MIRConst *GenerateMIRConstForStruct() const;
  std::vector<ASTExpr*> initExprs;
  ASTExpr *arrayFillerExpr = nullptr;
  MIRType *initListType = nullptr;
  std::string varName;
  ParentFlag parentFlag = kNoParent;
  bool isUnionInitListExpr = false;
  bool hasArrayFiller = false;
  bool isTransparent = false;
  bool hasVectorType = false;
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
  ASTBinaryOperatorExpr()
      : ASTExpr(kASTOpBO), varName("shortCircuit"),
        tempVarName(FEUtils::GetSequentialName("tempVar_")) {}

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

  void SetShortCircuitParent() override {
    shortCircuitParent = true;
  }

  bool GetShortCircuitParent() const override {
    return shortCircuitParent;
  }

  bool GetShortCircuit() const override {
    return opcode == OP_lior || opcode == OP_land;
  }

  UniqueFEIRType SelectBinaryOperatorType(UniqueFEIRExpr &left, UniqueFEIRExpr &right) const;

 protected:
  MIRConst *GenerateMIRConstImpl() const override;
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;

  Opcode opcode = OP_undef;
  MIRType *retType = nullptr;
  MIRType *complexElementType = nullptr;
  ASTExpr *leftExpr = nullptr;
  ASTExpr *rightExpr = nullptr;
  ASTExpr *leftRealExpr = nullptr;
  ASTExpr *leftImagExpr = nullptr;
  ASTExpr *rightRealExpr = nullptr;
  ASTExpr *rightImagExpr = nullptr;
  bool cvtNeeded = false;
  bool shortCircuitParent = false;
  std::string varName;
  std::string tempVarName;
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

  MIRType *GetType() const override {
    return type;
  }

  void SetLength(size_t len) {
    length = len;
  }

  size_t GetLength() {
    return length;
  }

  void SetCodeUnits(std::vector<uint32> &units) {
    codeUnits = std::move(units);
  }

  const std::vector<uint32> &GetCodeUnits() const {
    return codeUnits;
  }

  void SetIsArrayToPointerDecay(bool argIsArrayToPointerDecay) {
    isArrayToPointerDecay = argIsArrayToPointerDecay;
  }

  bool IsArrayToPointerDecay() const {
    return isArrayToPointerDecay;
  }

 protected:
  MIRConst *GenerateMIRConstImpl() const override;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  MIRType *type = nullptr;
  size_t length;
  std::vector<uint32> codeUnits;
  bool isArrayToPointerDecay = false;
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
    idxExpr = astExpr;
  }

  ASTExpr *GetIdxExpr() const {
    return idxExpr;
  }

  void SetArrayType(MIRType *ty) {
    arrayType = ty;
  }

  const MIRType *GetArrayType() const {
    return arrayType;
  }

  int32 CalculateOffset() const;

  void SetIsVLA(bool flag) {
    isVLA = flag;
  }

  void SetVLASizeExpr(ASTExpr *expr) {
    vlaSizeExpr = expr;
  }

 private:
  ASTExpr *FindFinalBase() const;
  MIRConst *GenerateMIRConstImpl() const override;
  bool CheckFirstDimIfZero() const;
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  ASTExpr *baseExpr = nullptr;
  MIRType *arrayType = nullptr;
  ASTExpr *idxExpr = nullptr;
  bool isVLA = false;
  ASTExpr *vlaSizeExpr = nullptr;
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

  void SetFiledOffsetBits(uint64 offset) {
    fieldOffsetBits = offset;
  }

  uint64 GetFieldOffsetBits() const {
    return fieldOffsetBits;
  }

 private:
  MIRConst *GenerateMIRConstImpl() const override;
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  const ASTMemberExpr *FindFinalMember(const ASTMemberExpr *startExpr, std::list<std::string> &memberNames) const;

  ASTExpr *baseExpr = nullptr;
  std::string memberName;
  MIRType *memberType = nullptr;
  MIRType *baseType = nullptr;
  bool isArrow = false;
  uint64 fieldOffsetBits = 0;
};

class ASTDesignatedInitUpdateExpr : public ASTExpr {
 public:
  ASTDesignatedInitUpdateExpr() : ASTExpr(kASTASTDesignatedInitUpdateExpr) {}
  ~ASTDesignatedInitUpdateExpr() = default;

  void SetBaseExpr(ASTExpr *astExpr) {
    baseExpr = astExpr;
  }

  ASTExpr *GetBaseExpr() {
    return baseExpr;
  }

  void SetUpdaterExpr(ASTExpr *astExpr) {
    updaterExpr = astExpr;
  }

  ASTExpr *GetUpdaterExpr() {
    return updaterExpr;
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

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  void GetActualRightExpr(UniqueFEIRExpr &right, UniqueFEIRExpr &left) const;
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

  bool IsNeedRetExpr() const {
    return retType->GetPrimType() != PTY_void;
  }

  bool IsFirstArgRet() const {
    return retType->GetPrimType() == PTY_agg && retType->GetSize() > 16; // Condition needs to be extended synchronously.
  }

  std::string CvtBuiltInFuncName(std::string builtInName) const;
  UniqueFEIRExpr ProcessBuiltinFunc(std::list<UniqueFEIRStmt> &stmts, bool &isFinish) const;
  std::unique_ptr<FEIRStmtAssign> GenCallStmt() const;
  void AddArgsExpr(std::unique_ptr<FEIRStmtAssign> &callStmt, std::list<UniqueFEIRStmt> &stmts) const;
  UniqueFEIRExpr AddRetExpr(std::unique_ptr<FEIRStmtAssign> &callStmt, std::list<UniqueFEIRStmt> &stmts) const;

 private:
  using FuncPtrBuiltinFunc = UniqueFEIRExpr (ASTCallExpr::*)(std::list<UniqueFEIRStmt> &stmts) const;
  static std::unordered_map<std::string, FuncPtrBuiltinFunc> InitBuiltinFuncPtrMap();
  UniqueFEIRExpr CreateIntrinsicopForC(std::list<UniqueFEIRStmt> &stmts, MIRIntrinsicID argIntrinsicID) const;
  UniqueFEIRExpr CreateBinaryExpr(std::list<UniqueFEIRStmt> &stmts, Opcode op) const;
  UniqueFEIRExpr EmitBuiltinFunc(std::list<UniqueFEIRStmt> &stmts) const;
  UniqueFEIRExpr EmitBuiltinVectorZip(std::list<UniqueFEIRStmt> &stmts, bool &isFinish) const;
  UniqueFEIRExpr EmitBuiltinRotate(std::list<UniqueFEIRStmt> &stmts, PrimType rotType, bool isLeft) const;
#define EMIT_BUILTIIN_FUNC(FUNC) EmitBuiltin##FUNC(std::list<UniqueFEIRStmt> &stmts) const
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Ctz);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Ctzl);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Clz);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Clzl);

  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Popcount);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Popcountl);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Popcountll);

  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Parity);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Parityl);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Parityll);

  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Clrsb);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Clrsbl);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Clrsbll);

  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Ffs);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Ffsl);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Ffsll);

  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(IsAligned);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(AlignUp);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(AlignDown);

  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Alloca);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Expect);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(VaStart);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(VaEnd);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(VaCopy);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Prefetch);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Unreachable);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Abs);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(ACos);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(ACosf);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(ASin);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(ASinf);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(ATan);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(ATanf);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Cos);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Cosf);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Cosh);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Coshf);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Sin);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Sinf);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Sinh);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Sinhf);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Exp);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Expf);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Fmax);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Fmin);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Log);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Logf);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Log10);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Log10f);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Isunordered);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Isless);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Islessequal);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Isgreater);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Isgreaterequal);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Islessgreater);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(WarnMemsetZeroLen);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(RotateLeft8);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(RotateLeft16);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(RotateLeft32);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(RotateLeft64);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(RotateRight8);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(RotateRight16);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(RotateRight32);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(RotateRight64);

// vector builtinfunc
#define DEF_MIR_INTRINSIC(STR, NAME, INTRN_CLASS, RETURN_TYPE, ...)         \
UniqueFEIRExpr EmitBuiltin##STR(std::list<UniqueFEIRStmt> &stmts) const;
#include "intrinsic_vector.def"
#undef DEF_MIR_INTRINSIC

  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;

  static std::unordered_map<std::string, FuncPtrBuiltinFunc> builtingFuncPtrMap;
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
  ASTParenExpr() : ASTExpr(kASTParen), tempVarName(FEUtils::GetSequentialName("tempVar_")) {}
  ~ASTParenExpr() = default;

  void SetASTExpr(ASTExpr *astExpr) {
    child = astExpr;
  }

  void SetShortCircuitParent() override {
    shortCircuitParent = true;
  }

  bool GetShortCircuitParent() const override {
    return shortCircuitParent;
  }

  bool GetShortCircuit() const override {
    return AstShortCircuitUtil::Instance().GetIsShortCircuit();
  }

 protected:
  MIRConst *GenerateMIRConstImpl() const override {
    return child->GenerateMIRConst();
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;

  ASTValue *GetConstantValueImpl() const override {
    return child->GetConstantValue();
  }

  ASTExpr *child = nullptr;
  bool shortCircuitParent = false;
  std::string tempVarName;
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

 protected:
  MIRConst *GenerateMIRConstImpl() const override;

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
  MIRConst *GenerateMIRConstImpl() const override;
  double val;
  FloatKind kind;
};

class ASTCharacterLiteral : public ASTExpr {
 public:
  ASTCharacterLiteral() : ASTExpr(kASTCharacterLiteral) {}
  ~ASTCharacterLiteral() = default;

  int64 GetVal() const {
    return val;
  }

  void SetVal(int64 valIn) {
    val = valIn;
  }

  void SetPrimType(PrimType primType) {
    type = primType;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  int64 val;
  PrimType type;
};

struct VaArgInfo {
  bool isGPReg;  // GP or FP/SIMD arg reg
  int regOffset;
  int stackOffset;
  // If the argument type is a Composite Type that is larger than 16 bytes,
  // then the argument is copied to memory allocated by the caller and replaced by a pointer to the copy.
  bool isCopyedMem;
  MIRType *HFAType;  // Homogeneous Floating-point Aggregate
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
  MIRType *IsHFAType(MIRStructType &type) const;
  void CvtHFA2Struct(MIRStructType &type, MIRType &fieldType, UniqueFEIRVar vaArgVar,
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

 protected:
  MIRConst *GenerateMIRConstImpl() const override;

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

  MIRConst *GenerateMIRConstImpl() const override {
    MIRConst *condConst = condExpr->GenerateMIRConst();
    if (condConst->IsZero()) {
      return falseExpr->GenerateMIRConst();
    } else {
      return trueExpr->GenerateMIRConst();
    }
  }

  ASTExpr *condExpr = nullptr;
  ASTExpr *trueExpr = nullptr;
  ASTExpr *falseExpr = nullptr;
  std::string varName = FEUtils::GetSequentialName("levVar_");
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

  MIRType *GetType() const override {
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
