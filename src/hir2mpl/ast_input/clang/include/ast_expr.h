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
#ifndef HIR2MPL_AST_INPUT_INCLUDE_AST_EXPR_H
#define HIR2MPL_AST_INPUT_INCLUDE_AST_EXPR_H
#include <variant>
#include "ast_op.h"
#include "feir_stmt.h"

namespace maple {
class ASTDecl;
class ASTFunc;
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

enum class ParentFlag {
  kNoParent,
  kArrayParent,
  kStructParent
};

enum EvaluatedFlag : uint8 {
  kEvaluatedAsZero,
  kEvaluatedAsNonZero,
  kNotEvaluated
};

class ASTExpr {
 public:
  explicit ASTExpr(ASTOp o) : op(o) {}
  virtual ~ASTExpr() = default;
  UniqueFEIRExpr Emit2FEExpr(std::list<UniqueFEIRStmt> &stmts) const;
  UniqueFEIRExpr ImplicitInitFieldValue(MIRType &type, std::list<UniqueFEIRStmt> &stmts) const;

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

  ASTOp GetASTOp() const {
    return op;
  }

  void SetConstantValue(ASTValue *val) {
    isConstantFolded = (val != nullptr);
    value = val;
  }

  void SetIsConstantFolded(bool flag) {
    isConstantFolded = flag;
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

  void SetSrcLoc(const Loc &l) {
    loc = l;
  }

  Loc GetSrcLoc() const {
    return loc;
  }

  uint32 GetSrcFileIdx() const {
    return loc.fileIdx;
  }

  uint32 GetSrcFileLineNum() const {
    return loc.line;
  }

  uint32 GetSrcFileColumn() const {
    return loc.column;
  }

  void SetEvaluatedFlag(EvaluatedFlag flag) {
    evaluatedflag = flag;
    return;
  }

  EvaluatedFlag GetEvaluatedFlag() const {
    return evaluatedflag;
  }

  bool IsRValue() const {
    return isRValue;
  }

  void SetRValue(bool flag) {
    isRValue = flag;
  }

  virtual void SetShortCircuitIdx(uint32 leftIdx, uint32 rightIdx) {}

  ASTExpr *IgnoreParens() {
    return IgnoreParensImpl();
  }

  void SetVLASizeExprs(std::list<ASTExpr*> astExprs) {
    vlaExprInfos = std::move(astExprs);
  }

  void SetCallAlloca(bool isAlloc) {
    isCallAlloca = isAlloc;
  }

  bool IsCallAlloca() const {
    return isCallAlloca;
  }

 protected:
  virtual ASTValue *GetConstantValueImpl() const {
    return value;
  }
  virtual MIRConst *GenerateMIRConstImpl() const;
  virtual UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const = 0;
  virtual ASTExpr *IgnoreParensImpl();

  virtual ASTDecl *GetASTDeclImpl() const {
    return refedDecl;
  }

  void EmitVLASizeExprs(std::list<UniqueFEIRStmt> &stmts) const {
    for (auto &vlaSizeExpr : vlaExprInfos) {
      (void)vlaSizeExpr->Emit2FEExpr(stmts);
    }
  }

  ASTOp op;
  MIRType *mirType = nullptr;
  ASTDecl *refedDecl = nullptr;
  bool isConstantFolded = false;
  ASTValue *value = nullptr;
  Loc loc = {0, 0, 0};
  EvaluatedFlag evaluatedflag = kNotEvaluated;
  bool isRValue = false;

 private:
  std::list<ASTExpr*> vlaExprInfos;
  bool isCallAlloca = false;
};

class ASTCastExpr : public ASTExpr {
 public:
  explicit ASTCastExpr(MapleAllocator &allocatorIn) : ASTExpr(kASTOpCast) {
    (void)allocatorIn;
  }
  ~ASTCastExpr() = default;

  void SetASTExpr(ASTExpr *expr) {
    child = expr;
  }

  const ASTExpr *GetASTExpr() const {
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

  void SetIsFunctionToPointerDecay(bool flag) {
    isFunctionToPointerDecay = flag;
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

  UniqueFEIRExpr Emit2FEExprForComplex(const UniqueFEIRExpr &subExpr, const UniqueFEIRType &srcType,
                                       std::list<UniqueFEIRStmt> &stmts) const;
  UniqueFEIRExpr Emit2FEExprForFunctionOrArray2Pointer(std::list<UniqueFEIRStmt> &stmts) const;

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
  bool isFunctionToPointerDecay = false;
  bool isBuilinFunc = false;
  bool isUnoinCast = false;
  bool isVectorSplat = false;
};

class ASTDeclRefExpr : public ASTExpr {
 public:
  explicit ASTDeclRefExpr(MapleAllocator &allocatorIn) : ASTExpr(kASTOpRef) {
    (void)allocatorIn;
  }
  ~ASTDeclRefExpr() = default;

 protected:
  MIRConst *GenerateMIRConstImpl() const override;
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTUnaryOperatorExpr : public ASTExpr {
 public:
  explicit ASTUnaryOperatorExpr(ASTOp o) : ASTExpr(o) {}
  ASTUnaryOperatorExpr(MapleAllocator &allocatorIn, ASTOp o) : ASTExpr(o) {
    (void)allocatorIn;
  }
  virtual ~ASTUnaryOperatorExpr() = default;
  void SetUOExpr(ASTExpr *astExpr);

  const ASTExpr *GetUOExpr() const {
    return expr;
  }

  void SetSubType(MIRType *type);

  const MIRType *GetMIRType() const {
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

  int64 GetPointeeLen() const {
    return pointeeLen;
  }

  void SetGlobal(bool isGlobalArg) {
    isGlobal = isGlobalArg;
  }

  bool IsGlobal() const {
    return isGlobal;
  }

  UniqueFEIRExpr ASTUOSideEffectExpr(Opcode op, std::list<UniqueFEIRStmt> &stmts,
      const std::string &varName = "", bool post = false) const;

 protected:
  bool isGlobal = false;
  ASTExpr *expr = nullptr;
  MIRType *subType = nullptr;
  MIRType *uoType = nullptr;
  int64 pointeeLen = 0;
};

class ASTUOMinusExpr : public ASTUnaryOperatorExpr {
 public:
  explicit ASTUOMinusExpr(MapleAllocator &allocatorIn) : ASTUnaryOperatorExpr(allocatorIn, kASTOpMinus) {}
  ~ASTUOMinusExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTUONotExpr : public ASTUnaryOperatorExpr {
 public:
  explicit ASTUONotExpr(MapleAllocator &allocatorIn) : ASTUnaryOperatorExpr(allocatorIn, kASTOpNot) {}
  ~ASTUONotExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTUOLNotExpr : public ASTUnaryOperatorExpr {
 public:
  explicit ASTUOLNotExpr(MapleAllocator &allocatorIn) : ASTUnaryOperatorExpr(allocatorIn, kASTOpLNot) {}
  ~ASTUOLNotExpr() = default;

  void SetShortCircuitIdx(uint32 leftIdx, uint32 rightIdx) override {
    trueIdx = leftIdx;
    falseIdx = rightIdx;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  uint32 trueIdx = 0;
  uint32 falseIdx = 0;
};

class ASTUOPostIncExpr : public ASTUnaryOperatorExpr {
 public:
  explicit ASTUOPostIncExpr(MapleAllocator &allocatorIn) : ASTUnaryOperatorExpr(allocatorIn, kASTOpPostInc),
      tempVarName(FEUtils::GetSequentialName("postinc_")) {}
  ~ASTUOPostIncExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  std::string tempVarName;
};

class ASTUOPostDecExpr : public ASTUnaryOperatorExpr {
 public:
  explicit ASTUOPostDecExpr(MapleAllocator &allocatorIn) : ASTUnaryOperatorExpr(allocatorIn, kASTOpPostDec),
      tempVarName(FEUtils::GetSequentialName("postdec_")) {}
  ~ASTUOPostDecExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  std::string tempVarName;
};

class ASTUOPreIncExpr : public ASTUnaryOperatorExpr {
 public:
  explicit ASTUOPreIncExpr(MapleAllocator &allocatorIn) : ASTUnaryOperatorExpr(allocatorIn, kASTOpPreInc) {}
  ~ASTUOPreIncExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTUOPreDecExpr : public ASTUnaryOperatorExpr {
 public:
  explicit ASTUOPreDecExpr(MapleAllocator &allocatorIn) : ASTUnaryOperatorExpr(allocatorIn, kASTOpPreDec) {}
  ~ASTUOPreDecExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  std::string tempVarName;
};

class ASTUOAddrOfExpr : public ASTUnaryOperatorExpr {
 public:
  ASTUOAddrOfExpr() : ASTUnaryOperatorExpr(kASTOpAddrOf) {}
  explicit ASTUOAddrOfExpr(MapleAllocator &allocatorIn) : ASTUnaryOperatorExpr(allocatorIn, kASTOpAddrOf) {}
  ~ASTUOAddrOfExpr() = default;

 protected:
  MIRConst *GenerateMIRConstImpl() const override;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTUOAddrOfLabelExpr : public ASTUnaryOperatorExpr {
 public:
  explicit ASTUOAddrOfLabelExpr(MapleAllocator &allocatorIn) : ASTUnaryOperatorExpr(allocatorIn, kASTOpAddrOfLabel),
      labelName("", allocatorIn.GetMemPool()) {}
  ~ASTUOAddrOfLabelExpr() = default;

  void SetLabelName(const std::string &name) {
    labelName = name;
  }

  const std::string GetLabelName() const {
    return labelName.c_str() == nullptr ? "" : labelName.c_str();
  }

 protected:
  MIRConst *GenerateMIRConstImpl() const override;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  MapleString labelName;
};

class ASTUODerefExpr : public ASTUnaryOperatorExpr {
 public:
  explicit ASTUODerefExpr(MapleAllocator &allocatorIn) : ASTUnaryOperatorExpr(allocatorIn, kASTOpDeref) {}
  ~ASTUODerefExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  void InsertNonnullChecking(std::list<UniqueFEIRStmt> &stmts, UniqueFEIRExpr baseExpr) const;
  bool InsertBoundaryChecking(std::list<UniqueFEIRStmt> &stmts, UniqueFEIRExpr expr) const;
};

class ASTUOPlusExpr : public ASTUnaryOperatorExpr {
 public:
  explicit ASTUOPlusExpr(MapleAllocator &allocatorIn) : ASTUnaryOperatorExpr(allocatorIn, kASTOpPlus) {}
  ~ASTUOPlusExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTUORealExpr : public ASTUnaryOperatorExpr {
 public:
  explicit ASTUORealExpr(MapleAllocator &allocatorIn) : ASTUnaryOperatorExpr(allocatorIn, kASTOpReal) {}
  ~ASTUORealExpr() = default;

  void SetElementType(MIRType *type) {
    elementType = type;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  MIRType *elementType = nullptr;
};

class ASTUOImagExpr : public ASTUnaryOperatorExpr {
 public:
  explicit ASTUOImagExpr(MapleAllocator &allocatorIn) : ASTUnaryOperatorExpr(allocatorIn, kASTOpImag) {}
  ~ASTUOImagExpr() = default;

  void SetElementType(MIRType *type) {
    elementType = type;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  MIRType *elementType = nullptr;
};

class ASTUOExtensionExpr : public ASTUnaryOperatorExpr {
 public:
  explicit ASTUOExtensionExpr(MapleAllocator &allocatorIn) : ASTUnaryOperatorExpr(allocatorIn, kASTOpExtension) {}
  ~ASTUOExtensionExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTUOCoawaitExpr : public ASTUnaryOperatorExpr {
 public:
  explicit ASTUOCoawaitExpr(MapleAllocator &allocatorIn) : ASTUnaryOperatorExpr(allocatorIn, kASTOpCoawait) {}
  ~ASTUOCoawaitExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTPredefinedExpr : public ASTExpr {
 public:
  explicit ASTPredefinedExpr(MapleAllocator &allocatorIn) : ASTExpr(kASTOpPredefined) {
    (void)allocatorIn;
  }
  ~ASTPredefinedExpr() = default;
  void SetASTExpr(ASTExpr *astExpr);

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  ASTExpr *child = nullptr;
};

class ASTOpaqueValueExpr : public ASTExpr {
 public:
  explicit ASTOpaqueValueExpr(MapleAllocator &allocatorIn) : ASTExpr(kASTOpOpaqueValue) {
    (void)allocatorIn;
  }
  ~ASTOpaqueValueExpr() = default;
  void SetASTExpr(ASTExpr *astExpr);

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  ASTExpr *child = nullptr;
};

class ASTNoInitExpr : public ASTExpr {
 public:
  explicit ASTNoInitExpr(MapleAllocator &allocatorIn) : ASTExpr(kASTOpNoInitExpr) {
    (void)allocatorIn;
  }
  ~ASTNoInitExpr() = default;
  void SetNoInitType(MIRType *type);

 private:
  MIRConst *GenerateMIRConstImpl() const override;
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  MIRType *noInitType = nullptr;
};

class ASTCompoundLiteralExpr : public ASTExpr {
 public:
  explicit ASTCompoundLiteralExpr(MapleAllocator &allocatorIn) : ASTExpr(kASTOpCompoundLiteralExpr) {
    (void)allocatorIn;
  }
  ~ASTCompoundLiteralExpr() = default;
  void SetCompoundLiteralType(MIRType *clType);
  void SetASTExpr(ASTExpr *astExpr);

  void SetAddrof(bool flag) {
    isAddrof = flag;
  }

  void SetVariableArrayExpr(ASTExpr *expr) {
    variableArrayExpr = expr;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  MIRConst *GenerateMIRConstImpl() const override;
  MIRConst *GenerateMIRPtrConst() const;
  ASTExpr *child = nullptr;
  MIRType *compoundLiteralType = nullptr;
  bool isAddrof = false;
  ASTExpr *variableArrayExpr = nullptr;
};

class ASTOffsetOfExpr : public ASTExpr {
 public:
  explicit ASTOffsetOfExpr(MapleAllocator &allocatorIn) : ASTExpr(kASTOpOffsetOfExpr) {
    (void)allocatorIn;
  }
  ~ASTOffsetOfExpr() = default;
  void SetStructType(MIRType *stype);
  void SetFieldName(const std::string &fName);

  void SetOffset(size_t val) {
    offset = val;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  MIRType *structType = nullptr;
  std::string fieldName;
  size_t offset = 0;
};

class ASTInitListExpr : public ASTExpr {
 public:
  explicit ASTInitListExpr(MapleAllocator &allocatorIn)
      : ASTExpr(kASTOpInitListExpr), initExprs(allocatorIn.Adapter()), varName("", allocatorIn.GetMemPool()) {}
  ~ASTInitListExpr() = default;
  void SetInitExprs(ASTExpr *astExpr);
  void SetInitListType(MIRType *type);

  const MIRType *GetInitListType() const {
    return initListType;
  }

  MapleVector<ASTExpr*> GetInitExprs() const {
    return initExprs;
  }

  void SetInitListVarName(const std::string &argVarName) {
    varName = argVarName;
  }

  const std::string GetInitListVarName() const {
    return varName.c_str() == nullptr ? "" : varName.c_str();
  }

  void SetParentFlag(ParentFlag argParentFlag) {
    parentFlag = argParentFlag;
  }

  void SetUnionInitFieldIdx(uint32 idx) {
    unionInitFieldIdx = idx;
  }

  uint32 GetUnionInitFieldIdx() const {
    return unionInitFieldIdx;
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

  const ASTExpr *GetArrayFillter() const {
    return arrayFillerExpr;
  }

  void SetHasVectorType(bool flag) {
    hasVectorType = flag;
  }

  bool HasVectorType() const {
    return hasVectorType;
  }

  void SetElemLen(size_t num) {
    elemLen = num;
  }

  size_t GetElemLen() const {
    return elemLen;
  }

 private:
  MIRConst *GenerateMIRConstImpl() const override;
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  void ProcessInitList(std::variant<std::pair<UniqueFEIRVar, FieldID>, UniqueFEIRExpr> &base,
                       const ASTInitListExpr &initList, std::list<UniqueFEIRStmt> &stmts) const;
  void ProcessArrayInitList(const UniqueFEIRExpr &addrOfArray, const ASTInitListExpr &initList,
                            std::list<UniqueFEIRStmt> &stmts) const;
  void SolveArrayElementInitWithInitListExpr(const UniqueFEIRExpr &addrOfArray, const UniqueFEIRExpr &addrOfElementExpr,
                                             const MIRType &elementType, const ASTExpr &subExpr, size_t index,
                                             std::list<UniqueFEIRStmt> &stmts) const;
  void HandleImplicitInitSections(const UniqueFEIRExpr &addrOfArray, const ASTInitListExpr &initList,
                                  const MIRType &elementType, std::list<UniqueFEIRStmt> &stmts) const;
  void ProcessStructInitList(std::variant<std::pair<UniqueFEIRVar, FieldID>, UniqueFEIRExpr> &base,
                             const ASTInitListExpr &initList, std::list<UniqueFEIRStmt> &stmts) const;
  void SolveInitListFullOfZero(const MIRStructType &baseStructType, FieldID baseFieldID, const UniqueFEIRVar &var,
                               const ASTInitListExpr &initList, std::list<UniqueFEIRStmt> &stmts) const;
  bool SolveInitListPartialOfZero(std::variant<std::pair<UniqueFEIRVar, FieldID>, UniqueFEIRExpr> &base,
                                  FieldID fieldID, uint32 &index, const ASTInitListExpr &initList,
                                  std::list<UniqueFEIRStmt> &stmts) const;
  void SolveInitListExprOrDesignatedInitUpdateExpr(std::tuple<FieldID, uint32, MIRType*> fieldInfo, ASTExpr &initExpr,
      const UniqueFEIRType &baseStructPtrType, std::variant<std::pair<UniqueFEIRVar, FieldID>, UniqueFEIRExpr> &base,
      std::list<UniqueFEIRStmt> &stmts) const;
  void SolveStructFieldOfArrayTypeInitWithStringLiteral(std::tuple<FieldID, uint32, MIRType*> fieldInfo,
      const ASTExpr &initExpr, const UniqueFEIRType &baseStructPtrType,
      std::variant<std::pair<UniqueFEIRVar, FieldID>, UniqueFEIRExpr> &base, std::list<UniqueFEIRStmt> &stmts) const;
  void SolveStructFieldOfBasicType(FieldID fieldID, const ASTExpr &initExpr, const UniqueFEIRType &baseStructPtrType,
                                   std::variant<std::pair<UniqueFEIRVar, FieldID>, UniqueFEIRExpr> &base,
                                   std::list<UniqueFEIRStmt> &stmts) const;
  std::tuple<FieldID, uint32, MIRType*> GetStructFieldInfo(uint32 fieldIndex, FieldID baseFieldID,
                                                           MIRStructType &structMirType) const;
  UniqueFEIRExpr CalculateStartAddressForMemset(const UniqueFEIRVar &varIn, uint32 initSizeIn, FieldID fieldIDIn,
      const std::variant<std::pair<UniqueFEIRVar, FieldID>, UniqueFEIRExpr> &baseIn) const;
  UniqueFEIRExpr GetAddrofArrayFEExprByStructArrayField(MIRType &fieldType,
                                                        const UniqueFEIRExpr &addrOfArrayField) const;
  void ProcessVectorInitList(std::variant<std::pair<UniqueFEIRVar, FieldID>, UniqueFEIRExpr> &base,
                             const ASTInitListExpr &initList, std::list<UniqueFEIRStmt> &stmts) const;
  MIRIntrinsicID SetVectorSetLane(const MIRType &type) const;
  void ProcessDesignatedInitUpdater(std::variant<std::pair<UniqueFEIRVar, FieldID>, UniqueFEIRExpr> &base,
                                    const UniqueFEIRExpr &addrOfCharArray, ASTExpr *expr,
                                    std::list<UniqueFEIRStmt> &stmts) const;
  void ProcessNoBaseDesignatedInitUpdater(std::variant<std::pair<UniqueFEIRVar, FieldID>, UniqueFEIRExpr> &base,
                                    ASTExpr *expr, std::list<UniqueFEIRStmt> &stmts) const;
  void ProcessStringLiteralInitList(const UniqueFEIRExpr &addrOfCharArray, const UniqueFEIRExpr &addrOfStringLiteral,
                                    size_t stringLength, std::list<UniqueFEIRStmt> &stmts) const;
  void ProcessImplicitInit(const UniqueFEIRExpr &addrExpr, uint32 initSize, uint32 total, uint32 elemSize,
                           std::list<UniqueFEIRStmt> &stmts, const Loc loc = {0, 0, 0}) const;
  MIRConst *GenerateMIRConstForArray() const;
  MIRConst *GenerateMIRConstForStruct() const;
  MapleVector<ASTExpr*> initExprs;
  ASTExpr *arrayFillerExpr = nullptr;
  MIRType *initListType = nullptr;
  MapleString varName;
  ParentFlag parentFlag = ParentFlag::kNoParent;
  uint32 unionInitFieldIdx = UINT32_MAX;
  size_t elemLen = 0;
  bool hasArrayFiller = false;
  bool isTransparent = false;
  bool hasVectorType = false;
  mutable bool isGenerating = false;
};

class ASTBinaryConditionalOperator : public ASTExpr {
 public:
  explicit ASTBinaryConditionalOperator(MapleAllocator &allocatorIn) : ASTExpr(kASTOpBinaryConditionalOperator) {
    (void)allocatorIn;
  }
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
  ASTBinaryOperatorExpr(MapleAllocator &allocatorIn, ASTOp o) : ASTExpr(o) {
    (void)allocatorIn;
  }
  explicit ASTBinaryOperatorExpr(MapleAllocator &allocatorIn)
      : ASTExpr(kASTOpBO), varName(FEUtils::GetSequentialName(FEUtils::kCondGoToStmtLabelNamePrefix),
        allocatorIn.GetMemPool()) {}

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

  void SetShortCircuitIdx(uint32 leftIdx, uint32 rightIdx) override {
    trueIdx = leftIdx;
    falseIdx = rightIdx;
  }

  std::string GetVarName() const {
    return varName.c_str() == nullptr ? "" : varName.c_str();
  }

  UniqueFEIRType SelectBinaryOperatorType(UniqueFEIRExpr &left, UniqueFEIRExpr &right) const;

 protected:
  MIRConst *GenerateMIRConstImpl() const override;
  MIRConst *SolveOpcodeLiorOrCior(const MIRConst &leftConst) const;
  MIRConst *SolveOpcodeLandOrCand(const MIRConst &leftConst, const MIRConst &rightConst) const;
  MIRConst *SolveOpcodeAdd(const MIRConst &leftConst, const MIRConst &rightConst) const;
  MIRConst *SolveOpcodeSub(const MIRConst &leftConst, const MIRConst &rightConst) const;
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  UniqueFEIRExpr Emit2FEExprComplexCalculations(std::list<UniqueFEIRStmt> &stmts) const;
  UniqueFEIRExpr Emit2FEExprComplexCompare(std::list<UniqueFEIRStmt> &stmts) const;
  UniqueFEIRExpr Emit2FEExprLogicOperate(std::list<UniqueFEIRStmt> &stmts) const;
  UniqueFEIRExpr Emit2FEExprLogicOperateSimplify(std::list<UniqueFEIRStmt> &stmts) const;

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
  MapleString varName;
  uint32 trueIdx = 0;
  uint32 falseIdx = 0;
};

class ASTImplicitValueInitExpr : public ASTExpr {
 public:
  explicit ASTImplicitValueInitExpr(MapleAllocator &allocatorIn) : ASTExpr(kASTImplicitValueInitExpr) {
    (void)allocatorIn;
  }
  ~ASTImplicitValueInitExpr() = default;

 protected:
  MIRConst *GenerateMIRConstImpl() const override;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTStringLiteral : public ASTExpr {
 public:
  explicit ASTStringLiteral(MapleAllocator &allocatorIn) : ASTExpr(kASTStringLiteral),
      codeUnits(allocatorIn.Adapter()), str(allocatorIn.Adapter()) {}
  ~ASTStringLiteral() = default;

  void SetLength(size_t len) {
    length = len;
  }

  size_t GetLength() const {
    return length;
  }

  void SetCodeUnits(MapleVector<uint32> &units) {
    codeUnits = std::move(units);
  }

  const MapleVector<uint32> &GetCodeUnits() const {
    return codeUnits;
  }

  void SetStr(const std::string &strIn) {
    if (str.size() > 0) {
      str.clear();
      str.shrink_to_fit();
    }
    (void)str.insert(str.cend(), strIn.cbegin(), strIn.cend());
  }

  const std::string GetStr() const {
    return std::string(str.cbegin(), str.cend());
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
  size_t length = 0;
  MapleVector<uint32> codeUnits;
  MapleVector<char> str;  // Ascii string
  bool isArrayToPointerDecay = false;
};

class ASTArraySubscriptExpr : public ASTExpr {
 public:
  explicit ASTArraySubscriptExpr(MapleAllocator &allocatorIn) : ASTExpr(kASTSubscriptExpr) {
    (void)allocatorIn;
  }
  ~ASTArraySubscriptExpr() = default;

  void SetBaseExpr(ASTExpr *astExpr) {
    baseExpr = astExpr;
  }

  const ASTExpr *GetBaseExpr() const {
    return baseExpr;
  }

  void SetIdxExpr(ASTExpr *astExpr) {
    idxExpr = astExpr;
  }

  const ASTExpr *GetIdxExpr() const {
    return idxExpr;
  }

  void SetArrayType(MIRType *ty) {
    arrayType = ty;
  }

  const MIRType *GetArrayType() const {
    return arrayType;
  }

  size_t CalculateOffset() const;

  void SetIsVLA(bool flag) {
    isVLA = flag;
  }

  void SetVLASizeExpr(ASTExpr *expr) {
    vlaSizeExpr = expr;
  }

 private:
  ASTExpr *FindFinalBase() const;
  MIRConst *GenerateMIRConstImpl() const override;
  bool CheckFirstDimIfZero(const MIRType *arrType) const;
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  MIRType *GetArrayTypeForPointerArray() const;
  UniqueFEIRExpr SolveMultiDimArray(UniqueFEIRExpr &baseAddrFEExpr, UniqueFEIRType &arrayFEType,
                                    bool isArrayTypeOpt, std::list<UniqueFEIRStmt> &stmts) const;
  UniqueFEIRExpr SolveOtherArrayType(const UniqueFEIRExpr &baseAddrFEExpr, std::list<UniqueFEIRStmt> &stmts) const;
  void InsertNonnullChecking(std::list<UniqueFEIRStmt> &stmts, const UniqueFEIRExpr &indexExpr,
                             const UniqueFEIRExpr &baseAddrExpr) const;
  bool InsertBoundaryChecking(std::list<UniqueFEIRStmt> &stmts, UniqueFEIRExpr indexExpr,
                              UniqueFEIRExpr baseAddrFEExpr) const;

  ASTExpr *baseExpr = nullptr;
  MIRType *arrayType = nullptr;
  ASTExpr *idxExpr = nullptr;
  bool isVLA = false;
  ASTExpr *vlaSizeExpr = nullptr;
};

class ASTExprUnaryExprOrTypeTraitExpr : public ASTExpr {
 public:
  explicit ASTExprUnaryExprOrTypeTraitExpr(MapleAllocator &allocatorIn) : ASTExpr(kASTExprUnaryExprOrTypeTraitExpr) {
    (void)allocatorIn;
  }
  ~ASTExprUnaryExprOrTypeTraitExpr() = default;

  void SetIdxExpr(ASTExpr *astExpr) {
    idxExpr = astExpr;
  }
  void SetSizeExpr(ASTExpr *expr) {
    sizeExpr = expr;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  ASTExpr *sizeExpr = nullptr;
  ASTExpr *idxExpr = nullptr;
};

class ASTMemberExpr : public ASTExpr {
 public:
  explicit ASTMemberExpr(MapleAllocator &allocatorIn) : ASTExpr(kASTMemberExpr),
      memberName("", allocatorIn.GetMemPool()) {}
  ~ASTMemberExpr() = default;

  void SetBaseExpr(ASTExpr *astExpr) {
    baseExpr = astExpr;
  }

  const ASTExpr *GetBaseExpr() const {
    return baseExpr;
  }

  void SetMemberName(std::string name) {
    memberName = std::move(name);
  }

  std::string GetMemberName() const {
    return memberName.c_str() == nullptr ? "" : memberName.c_str();
  }

  void SetMemberType(MIRType *type) {
    memberType = type;
  }

  void SetBaseType(MIRType *type) {
    baseType = type;
  }

  const MIRType *GetMemberType() const {
    return memberType;
  }

  const MIRType *GetBaseType() const {
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
  const ASTMemberExpr &FindFinalMember(const ASTMemberExpr &startExpr, std::list<std::string> &memberNames) const;
  void InsertNonnullChecking(std::list<UniqueFEIRStmt> &stmts, UniqueFEIRExpr baseFEExpr) const;

  ASTExpr *baseExpr = nullptr;
  MapleString memberName;
  MIRType *memberType = nullptr;
  MIRType *baseType = nullptr;
  bool isArrow = false;
  uint64 fieldOffsetBits = 0;
};

class ASTDesignatedInitUpdateExpr : public ASTExpr {
 public:
  explicit ASTDesignatedInitUpdateExpr(MapleAllocator &allocatorIn) : ASTExpr(kASTASTDesignatedInitUpdateExpr) {
    (void)allocatorIn;
  }
  ~ASTDesignatedInitUpdateExpr() = default;

  void SetBaseExpr(ASTExpr *astExpr) {
    baseExpr = astExpr;
  }

  const ASTExpr *GetBaseExpr() const{
    return baseExpr;
  }

  void SetUpdaterExpr(ASTExpr *astExpr) {
    updaterExpr = astExpr;
  }

  ASTExpr *GetUpdaterExpr() const{
    return updaterExpr;
  }

  void SetInitListType(MIRType *type) {
    initListType = type;
  }

  const MIRType *GetInitListType() const {
    return initListType;
  }

  void SetInitListVarName(const std::string &name) {
    initListVarName = name;
  }

  const std::string &GetInitListVarName() const {
    return initListVarName;
  }

 private:
  MIRConst *GenerateMIRConstImpl() const override;
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  ASTExpr *baseExpr = nullptr;
  ASTExpr *updaterExpr = nullptr;
  MIRType *initListType = nullptr;
  std::string initListVarName;
};

class ASTAssignExpr : public ASTBinaryOperatorExpr {
 public:
  explicit ASTAssignExpr(MapleAllocator &allocatorIn) : ASTBinaryOperatorExpr(allocatorIn, kASTOpAssign),
      isCompoundAssign(false) {}
  ~ASTAssignExpr() override = default;

  void SetIsCompoundAssign(bool argIsCompoundAssign) {
    isCompoundAssign = argIsCompoundAssign;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  void GetActualRightExpr(UniqueFEIRExpr &right, const UniqueFEIRExpr &left) const;
  bool IsInsertNonnullChecking(const UniqueFEIRExpr &rExpr) const;
  bool isCompoundAssign = false;
};

class ASTBOComma : public ASTBinaryOperatorExpr {
 public:
  explicit ASTBOComma(MapleAllocator &allocatorIn) : ASTBinaryOperatorExpr(allocatorIn, kASTOpComma) {}
  ~ASTBOComma() override = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTBOPtrMemExpr : public ASTBinaryOperatorExpr {
 public:
  explicit ASTBOPtrMemExpr(MapleAllocator &allocatorIn) : ASTBinaryOperatorExpr(allocatorIn, kASTOpPtrMemD) {}
  ~ASTBOPtrMemExpr() override = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTCallExpr : public ASTExpr {
 public:
  explicit ASTCallExpr(MapleAllocator &allocatorIn)
      : ASTExpr(kASTOpCall), args(allocatorIn.Adapter()), funcName("", allocatorIn.GetMemPool()),
        varName(FEUtils::GetSequentialName("retVar_"), allocatorIn.GetMemPool()) {}
  ~ASTCallExpr() = default;
  void SetCalleeExpr(ASTExpr *astExpr) {
    calleeExpr = astExpr;
  }

  const ASTExpr *GetCalleeExpr() const {
    return calleeExpr;
  }

  void SetArgs(MapleVector<ASTExpr*> &argsVector) {
    args = std::move(argsVector);
  }

  const MapleVector<ASTExpr*> &GetArgsExpr() const {
    return args;
  }

  MIRType *GetRetType() const {
    return mirType;
  }

  const std::string GetRetVarName() const {
    return varName.c_str() == nullptr ? "" : varName.c_str();
  }

  void SetFuncName(const std::string &name) {
    funcName = name;
  }

  const std::string GetFuncName() const {
    return funcName.c_str() == nullptr ? "" : funcName.c_str();
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
    return mirType->GetPrimType() != PTY_void;
  }

  bool IsFirstArgRet() const {
    // If the return value exceeds 16 bytes, it is passed as the first parameter.
    return mirType->GetPrimType() == PTY_agg && mirType->GetSize() > 16;
  }

  void SetFuncDecl(ASTFunc *decl) {
    funcDecl = decl;
  }

  void SetReturnVarAttrs(const GenericAttrs &attrs) {
    returnVarAttrs = attrs;
  }

  const GenericAttrs &GetReturnVarAttrs() const {
    return returnVarAttrs;
  }

  std::string CvtBuiltInFuncName(std::string builtInName) const;
  UniqueFEIRExpr ProcessBuiltinFunc(std::list<UniqueFEIRStmt> &stmts, bool &isFinish) const;
  std::unique_ptr<FEIRStmtAssign> GenCallStmt() const;
  void AddArgsExpr(const std::unique_ptr<FEIRStmtAssign> &callStmt, std::list<UniqueFEIRStmt> &stmts) const;
  UniqueFEIRExpr AddRetExpr(const std::unique_ptr<FEIRStmtAssign> &callStmt) const;
  void InsertBoundaryCheckingInArgs(std::list<UniqueFEIRStmt> &stmts) const;
  void InsertBoundaryCheckingInArgsForICall(std::list<UniqueFEIRStmt> &stmts, const UniqueFEIRExpr &calleeFEExpr) const;
  void InsertBoundaryVarInRet(std::list<UniqueFEIRStmt> &stmts) const;
  void InsertNonnullCheckingForIcall(const UniqueFEIRExpr &expr, std::list<UniqueFEIRStmt> &stmts) const;
  void CheckNonnullFieldInStruct() const;

 private:
  using FuncPtrBuiltinFunc = UniqueFEIRExpr (ASTCallExpr::*)(std::list<UniqueFEIRStmt> &stmts) const;
  static std::unordered_map<std::string, FuncPtrBuiltinFunc> InitBuiltinFuncPtrMap();
  UniqueFEIRExpr CreateIntrinsicopForC(std::list<UniqueFEIRStmt> &stmts, MIRIntrinsicID argIntrinsicID,
                                       bool genTempVar = true) const;
  UniqueFEIRExpr CreateIntrinsicCallAssignedForC(std::list<UniqueFEIRStmt> &stmts, MIRIntrinsicID argIntrinsicID) const;
  UniqueFEIRExpr CreateBinaryExpr(std::list<UniqueFEIRStmt> &stmts, Opcode op) const;
  UniqueFEIRExpr EmitBuiltinFunc(std::list<UniqueFEIRStmt> &stmts) const;
  UniqueFEIRExpr EmitBuiltinVectorLoad(std::list<UniqueFEIRStmt> &stmts, bool &isFinish) const;
  UniqueFEIRExpr EmitBuiltinVectorStore(std::list<UniqueFEIRStmt> &stmts, bool &isFinish) const;
  UniqueFEIRExpr EmitBuiltinVectorShli(std::list<UniqueFEIRStmt> &stmts, bool &isFinish) const;
  UniqueFEIRExpr EmitBuiltinVectorShri(std::list<UniqueFEIRStmt> &stmts, bool &isFinish) const;
  UniqueFEIRExpr EmitBuiltinVectorShru(std::list<UniqueFEIRStmt> &stmts, bool &isFinish) const;
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
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Bswap64);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Bswap32);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(Bswap16);
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

  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncAddAndFetch8);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncAddAndFetch4);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncAddAndFetch2);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncAddAndFetch1);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncSubAndFetch8);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncSubAndFetch4);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncSubAndFetch2);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncSubAndFetch1);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncFetchAndSub8);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncFetchAndSub4);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncFetchAndSub2);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncFetchAndSub1);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncFetchAndAdd8);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncFetchAndAdd4);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncFetchAndAdd2);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncFetchAndAdd1);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncBoolCompareAndSwap1);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncBoolCompareAndSwap2);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncBoolCompareAndSwap4);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncBoolCompareAndSwap8);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncLockTestAndSet8);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncLockTestAndSet4);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncLockTestAndSet2);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncLockTestAndSet1);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncValCompareAndSwap8);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncValCompareAndSwap4);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncValCompareAndSwap2);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncValCompareAndSwap1);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncLockRelease8);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncLockRelease4);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncLockRelease2);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncLockRelease1);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncFetchAndAnd1);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncFetchAndAnd2);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncFetchAndAnd4);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncFetchAndAnd8);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncFetchAndOr1);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncFetchAndOr2);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncFetchAndOr4);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncFetchAndOr8);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncFetchAndXor1);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncFetchAndXor2);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncFetchAndXor4);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncFetchAndXor8);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncFetchAndNand1);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncFetchAndNand2);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncFetchAndNand4);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncFetchAndNand8);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncAndAndFetch1);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncAndAndFetch2);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncAndAndFetch4);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncAndAndFetch8);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncOrAndFetch1);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncOrAndFetch2);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncOrAndFetch4);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncOrAndFetch8);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncXorAndFetch1);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncXorAndFetch2);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncXorAndFetch4);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncXorAndFetch8);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncNandAndFetch1);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncNandAndFetch2);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncNandAndFetch4);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncNandAndFetch8);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(SyncSynchronize);

  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(AtomicExchangeN);

  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(ObjectSize);

  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(ReturnAddress);
  UniqueFEIRExpr EMIT_BUILTIIN_FUNC(ExtractReturnAddr);

// vector builtinfunc
#define DEF_MIR_INTRINSIC(STR, NAME, INTRN_CLASS, RETURN_TYPE, ...)         \
UniqueFEIRExpr EmitBuiltin##STR(std::list<UniqueFEIRStmt> &stmts) const;
#include "intrinsic_vector.def"
#include "intrinsic_vector_new.def"
#undef DEF_MIR_INTRINSIC

  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;

  static std::unordered_map<std::string, FuncPtrBuiltinFunc> builtingFuncPtrMap;
  MapleVector<ASTExpr*> args;
  ASTExpr *calleeExpr = nullptr;
  MapleString funcName;
  FuncAttrs funcAttrs;
  bool isIcall = false;
  MapleString varName;
  ASTFunc *funcDecl = nullptr;
  GenericAttrs returnVarAttrs;
};

class ASTParenExpr : public ASTExpr {
 public:
  explicit ASTParenExpr(MapleAllocator &allocatorIn) : ASTExpr(kASTParen) {
    (void)allocatorIn;
  }
  ~ASTParenExpr() = default;

  void SetASTExpr(ASTExpr *astExpr) {
    child = astExpr;
  }

  void SetShortCircuitIdx(uint32 leftIdx, uint32 rightIdx) override {
    trueIdx = leftIdx;
    falseIdx = rightIdx;
  }

 protected:
  MIRConst *GenerateMIRConstImpl() const override {
    return child->GenerateMIRConst();
  }

  ASTExpr *IgnoreParensImpl() override {
    return child->IgnoreParens();
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;

  ASTValue *GetConstantValueImpl() const override {
    return child->GetConstantValue();
  }

  ASTExpr *child = nullptr;
  uint32 trueIdx = 0;
  uint32 falseIdx = 0;
};

class ASTIntegerLiteral : public ASTExpr {
 public:
  explicit ASTIntegerLiteral(MapleAllocator &allocatorIn) : ASTExpr(kASTIntegerLiteral) {
    (void)allocatorIn;
  }
  ~ASTIntegerLiteral() = default;

  int64 GetVal() const {
    return val;
  }

  void SetVal(int64 valIn) {
    val = valIn;
  }

 protected:
  MIRConst *GenerateMIRConstImpl() const override;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;

  int64 val = 0;
};

enum class FloatKind {
  F32,
  F64
};

class ASTFloatingLiteral : public ASTExpr {
 public:
  explicit ASTFloatingLiteral(MapleAllocator &allocatorIn) : ASTExpr(kASTFloatingLiteral) {
    (void)allocatorIn;
  }
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
  double val = 0;
  FloatKind kind = FloatKind::F32;
};

class ASTCharacterLiteral : public ASTExpr {
 public:
  explicit ASTCharacterLiteral(MapleAllocator &allocatorIn) : ASTExpr(kASTCharacterLiteral) {
    (void)allocatorIn;
  }
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
  int64 val = 0;
  PrimType type = PTY_begin;
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
  explicit ASTVAArgExpr(MapleAllocator &allocatorIn) : ASTExpr(kASTVAArgExpr) {
    (void)allocatorIn;
  }
  ~ASTVAArgExpr() = default;

  void SetASTExpr(ASTExpr *astExpr) {
    child = astExpr;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  VaArgInfo ProcessValistArgInfo(const MIRType &type) const;
  MIRType *IsHFAType(const MIRStructType &type) const;
  void CvtHFA2Struct(const MIRStructType &type, MIRType &fieldType, const UniqueFEIRVar &vaArgVar,
                     std::list<UniqueFEIRStmt> &stmts) const;
  void ProcessBigEndianForReg(std::list<UniqueFEIRStmt> &stmts, MIRType &vaArgType,
                              const UniqueFEIRVar &offsetVar, const VaArgInfo &info) const;
  void ProcessBigEndianForStack(std::list<UniqueFEIRStmt> &stmts, MIRType &vaArgType,
                                const UniqueFEIRVar &vaArgVar) const;

  ASTExpr *child = nullptr;
};

class ASTConstantExpr : public ASTExpr {
 public:
  explicit ASTConstantExpr(MapleAllocator &allocatorIn) : ASTExpr(kConstantExpr) {
    (void)allocatorIn;
  }
  ~ASTConstantExpr() = default;
  void SetASTExpr(ASTExpr *astExpr) {
    child = astExpr;
  }

  const ASTExpr *GetChild() const{
    return child;
  }

 protected:
  MIRConst *GenerateMIRConstImpl() const override;

 private:
  ASTExpr *child = nullptr;
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTImaginaryLiteral : public ASTExpr {
 public:
  explicit ASTImaginaryLiteral(MapleAllocator &allocatorIn) : ASTExpr(kASTImaginaryLiteral) {
    (void)allocatorIn;
  }
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
  explicit ASTConditionalOperator(MapleAllocator &allocatorIn) : ASTExpr(kASTConditionalOperator) {
    (void)allocatorIn;
  }
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
  explicit ASTArrayInitLoopExpr(MapleAllocator &allocatorIn) : ASTExpr(kASTOpArrayInitLoop) {
    (void)allocatorIn;
  }
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
  explicit ASTArrayInitIndexExpr(MapleAllocator &allocatorIn) : ASTExpr(kASTOpArrayInitLoop) {
    (void)allocatorIn;
  }
  ~ASTArrayInitIndexExpr() = default;

  void SetPrimType(MIRType *pType) {
    primType = pType;
  }

  void SetValueStr(const std::string &val) {
    valueStr = val;
  }

  const MIRType *GetPrimeType() const {
    return primType;
  }

  std::string GetValueStr() const {
    return valueStr;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  MIRType *primType = nullptr;
  std::string valueStr;
};

class ASTExprWithCleanups : public ASTExpr {
 public:
  explicit ASTExprWithCleanups(MapleAllocator &allocatorIn) : ASTExpr(kASTOpExprWithCleanups) {
    (void)allocatorIn;
  }
  ~ASTExprWithCleanups() = default;

  void SetSubExpr(ASTExpr *sub) {
    subExpr = sub;
  }

  const ASTExpr *GetSubExpr() const {
    return subExpr;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  ASTExpr *subExpr = nullptr;
};

class ASTMaterializeTemporaryExpr : public ASTExpr {
 public:
  explicit ASTMaterializeTemporaryExpr(MapleAllocator &allocatorIn) : ASTExpr(kASTOpMaterializeTemporary) {
    (void)allocatorIn;
  }
  ~ASTMaterializeTemporaryExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTSubstNonTypeTemplateParmExpr : public ASTExpr {
 public:
  explicit ASTSubstNonTypeTemplateParmExpr(MapleAllocator &allocatorIn) : ASTExpr(kASTOpSubstNonTypeTemplateParm) {
    (void)allocatorIn;
  }
  ~ASTSubstNonTypeTemplateParmExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTDependentScopeDeclRefExpr : public ASTExpr {
 public:
  explicit ASTDependentScopeDeclRefExpr(MapleAllocator &allocatorIn) : ASTExpr(kASTOpDependentScopeDeclRef) {
    (void)allocatorIn;
  }
  ~ASTDependentScopeDeclRefExpr() = default;

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
};

class ASTAtomicExpr : public ASTExpr {
 public:
  explicit ASTAtomicExpr(MapleAllocator &allocatorIn) : ASTExpr(kASTOpAtomic),
      varName(FEUtils::GetSequentialName("ret.var.")) {
    (void)allocatorIn;
  }
  ~ASTAtomicExpr() = default;

  void SetRefType(MIRType *ref) {
    refType = ref;
  }

  const std::string GetRetVarName() const {
    return varName.c_str();
  }

  void SetAtomicOp(ASTAtomicOp op) {
    atomicOp = op;
  }

  const MIRType *GetRefType() const {
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

  void SetOrderExpr(ASTExpr *order) {
    orderExpr = order;
  }

  const ASTExpr *GetValExpr1() const {
    return valExpr1;
  }

  const ASTExpr *GetValExpr2() const {
    return valExpr2;
  }

  const ASTExpr *GetObjExpr() const {
    return objExpr;
  }

  const ASTExpr *GetOrderExpr() const {
    return orderExpr;
  }

  void SetVal1Type(MIRType *ty) {
    val1Type = ty;
  }

  void SetVal2Type(MIRType *ty) {
    val2Type = ty;
  }

  void SetFirstParamType(MIRType *ty) {
    firstType = ty;
  }

  void SetSecondParamType(MIRType *ty) {
    secondType = ty;
  }

  void SetThirdParamType(MIRType *ty) {
    thirdType = ty;
  }

  void SetFromStmt(bool fromStmt) {
    isFromStmt = fromStmt;
  }

  bool IsFromStmt() const {
    return isFromStmt;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;
  MIRType *refType = nullptr;
  MIRType *val1Type = nullptr;
  MIRType *val2Type = nullptr;
  MIRType *firstType = nullptr;
  MIRType *secondType = nullptr;
  MIRType *thirdType = nullptr;
  ASTExpr *objExpr = nullptr;
  ASTExpr *valExpr1 = nullptr;
  ASTExpr *valExpr2 = nullptr;
  ASTExpr *orderExpr = nullptr;
  ASTAtomicOp atomicOp = kAtomicOpUndefined;
  bool isFromStmt = false;
  const std::string varName;
};

class ASTExprStmtExpr : public ASTExpr {
 public:
  explicit ASTExprStmtExpr(MapleAllocator &allocatorIn) : ASTExpr(kASTOpStmtExpr) {
    (void)allocatorIn;
  }
  ~ASTExprStmtExpr() = default;
  void SetCompoundStmt(ASTStmt *sub) {
    cpdStmt = sub;
  }

  const ASTStmt *GetSubExpr() const {
    return cpdStmt;
  }

 private:
  UniqueFEIRExpr Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const override;

  ASTStmt *cpdStmt = nullptr;
};
}
#endif // HIR2MPL_AST_INPUT_INCLUDE_AST_EXPR_H
