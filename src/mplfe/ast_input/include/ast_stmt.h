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
#ifndef MPLFE_AST_INPUT_INCLUDE_AST_STMT_H
#define MPLFE_AST_INPUT_INCLUDE_AST_STMT_H
#include "ast_op.h"
#include "ast_expr.h"
#include "feir_stmt.h"

namespace maple {
class ASTDecl;
class ASTStmt {
 public:
  explicit ASTStmt(ASTStmtOp o = kASTStmtNone) : op(o) {}
  virtual ~ASTStmt() = default;
  void SetASTExpr(ASTExpr* astExpr);

  std::list<UniqueFEIRStmt> Emit2FEStmt() const {
    return Emit2FEStmtImpl();
  }

  ASTStmtOp GetASTStmtOp() const {
    return op;
  }

  const std::vector<ASTExpr*> &GetExprs() const {
    return exprs;
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

  void UseCompareAsCondFEExpr(UniqueFEIRExpr &condFEExpr) const;

 protected:
  virtual std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const = 0;
  ASTStmtOp op;
  std::vector<ASTExpr*> exprs;

  uint32 srcFileIdx = 0;
  uint32 srcFileLineNum = 0;
};

class ASTStmtDummy : public ASTStmt {
 public:
  ASTStmtDummy() : ASTStmt(kASTStmtDummy) {}
  ~ASTStmtDummy() = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTCompoundStmt : public ASTStmt {
 public:
  ASTCompoundStmt() : ASTStmt(kASTStmtCompound) {}
  ~ASTCompoundStmt() = default;
  void SetASTStmt(ASTStmt*);
  void InsertASTStmtsAtFront(const std::list<ASTStmt*> &stmts);
  const std::list<ASTStmt*> &GetASTStmtList() const;

 private:
  std::list<ASTStmt*> astStmts; // stmts
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

// Any other expressions or stmts should be extended here
class ASTReturnStmt : public ASTStmt {
 public:
  ASTReturnStmt() : ASTStmt(kASTStmtReturn) {}
  ~ASTReturnStmt() = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTIfStmt : public ASTStmt {
 public:
  ASTIfStmt() : ASTStmt(kASTStmtIf) {}
  ~ASTIfStmt() override = default;

  void SetCondExpr(ASTExpr *astExpr) {
    condExpr = astExpr;
  }

  void SetThenStmt(ASTStmt *astStmt) {
    thenStmt = astStmt;
  }

  void SetElseStmt(ASTStmt *astStmt) {
    elseStmt = astStmt;
  }

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
  ASTExpr *condExpr = nullptr;
  ASTStmt *thenStmt = nullptr;
  ASTStmt *elseStmt = nullptr;
};

class ASTForStmt : public ASTStmt {
 public:
  ASTForStmt() : ASTStmt(kASTStmtFor) {}
  ~ASTForStmt() override = default;

  void SetInitStmt(ASTStmt *astStmt) {
    initStmt = astStmt;
  }

  void SetCondExpr(ASTExpr *astExpr) {
    condExpr = astExpr;
  }

  void SetIncExpr(ASTExpr *astExpr) {
    incExpr = astExpr;
  }

  void SetBodyStmt(ASTStmt *astStmt) {
    bodyStmt = astStmt;
  }

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
  ASTStmt *initStmt = nullptr;
  ASTExpr *condExpr = nullptr;
  ASTExpr *incExpr = nullptr;
  ASTStmt *bodyStmt = nullptr;
};

class ASTWhileStmt : public ASTStmt {
 public:
  ASTWhileStmt() : ASTStmt(kASTStmtWhile) {}
  ~ASTWhileStmt() override = default;

  void SetCondExpr(ASTExpr *astExpr) {
    condExpr = astExpr;
  }

  void SetBodyStmt(ASTStmt *astStmt) {
    bodyStmt = astStmt;
  }

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
  ASTExpr *condExpr = nullptr;
  ASTStmt *bodyStmt = nullptr;
};

class ASTDoStmt : public ASTStmt {
 public:
  ASTDoStmt() : ASTStmt(kASTStmtDo) {}
  ~ASTDoStmt() override = default;

  void SetCondExpr(ASTExpr *astExpr) {
    condExpr = astExpr;
  }

  void SetBodyStmt(ASTStmt *astStmt) {
    bodyStmt = astStmt;
  }

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
  ASTExpr *condExpr = nullptr;
  ASTStmt *bodyStmt = nullptr;
};

class ASTBreakStmt : public ASTStmt {
 public:
  ASTBreakStmt() : ASTStmt(kASTStmtBreak) {}
  ~ASTBreakStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTLabelStmt : public ASTStmt {
 public:
  ASTLabelStmt() : ASTStmt(kASTStmtLabel) {}
  ~ASTLabelStmt() override = default;

  void SetSubStmt(ASTStmt *stmt) {
    subStmt = stmt;
  }

  void SetLabelName(std::string &name) {
    labelName = name;
  }

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
  std::string labelName;
  ASTStmt *subStmt = nullptr;
};

class ASTContinueStmt : public ASTStmt {
 public:
  ASTContinueStmt() : ASTStmt(kASTStmtContinue) {}
  ~ASTContinueStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTUnaryOperatorStmt : public ASTStmt {
 public:
  ASTUnaryOperatorStmt() : ASTStmt(kASTStmtUO) {}
  ~ASTUnaryOperatorStmt() = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTBinaryOperatorStmt : public ASTStmt {
 public:
  ASTBinaryOperatorStmt() : ASTStmt(kASTStmtBO) {}
  ~ASTBinaryOperatorStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTGotoStmt : public ASTStmt {
 public:
  ASTGotoStmt() : ASTStmt(kASTStmtGoto) {}
  ~ASTGotoStmt() = default;

  std::string GetLabelName() const {
    return labelName;
  }

  void SetLabelName(const std::string &name) {
    labelName = name;
  }

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
  std::string labelName;
};

class ASTIndirectGotoStmt : public ASTStmt {
 public:
  ASTIndirectGotoStmt() : ASTStmt(kASTStmtIndirectGoto) {}
  ~ASTIndirectGotoStmt() = default;

 protected:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTSwitchStmt : public ASTStmt {
 public:
  ASTSwitchStmt() : ASTStmt(kASTStmtSwitch) {}
  ~ASTSwitchStmt() = default;

  void SetCondStmt(ASTStmt *cond) {
    condStmt = cond;
  }

  void SetBodyStmt(ASTStmt *body) {
    bodyStmt = body;
  }

  void SetCondExpr(ASTExpr *cond) {
    condExpr = cond;
  }

  const ASTStmt *GetCondStmt() const {
    return condStmt;
  }

  const ASTExpr *GetCondExpr() const {
    return condExpr;
  }

  const ASTStmt *GetBodyStmt() const {
    return bodyStmt;
  }

  void SetHasDefault(bool argHasDefault) {
    hasDefualt = argHasDefault;
  }

  bool HasDefault() const {
    return hasDefualt;
  }

  void SetCondType(MIRType *type) {
    condType = type;
  }

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
  ASTStmt *condStmt = nullptr;
  ASTExpr *condExpr = nullptr;
  ASTStmt *bodyStmt = nullptr;
  MIRType *condType = nullptr;
  bool hasDefualt = false;
};

class ASTCaseStmt : public ASTStmt {
 public:
  ASTCaseStmt() : ASTStmt(kASTStmtCase) {}
  ~ASTCaseStmt() = default;

  void SetLHS(ASTExpr *l) {
    lhs = l;
  }

  void SetRHS(ASTExpr *r) {
    rhs = r;
  }

  void SetSubStmt(ASTStmt *sub) {
    subStmt = sub;
  }

  const ASTExpr *GetLHS() const {
    return lhs;
  }

  const ASTExpr *GetRHS() const {
    return rhs;
  }

  const ASTStmt *GetSubStmt() const {
    return subStmt;
  }

  int64 GetLCaseTag() const {
    return lCaseTag;
  }

  int64 GetRCaseTag() const {
    return rCaseTag;
  }

  void SetLCaseTag(int64 l) {
    lCaseTag = l;
  }

  void SetRCaseTag(int64 r) {
    rCaseTag = r;
  }

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
  ASTExpr *lhs = nullptr;
  ASTExpr *rhs = nullptr;
  ASTStmt *subStmt = nullptr;
  int64 lCaseTag;
  int64 rCaseTag;
};

class ASTDefaultStmt : public ASTStmt {
 public:
  ASTDefaultStmt() : ASTStmt(kASTStmtDefault) {}
  ~ASTDefaultStmt() = default;

  void SetChildStmt(ASTStmt* ch) {
    child = ch;
  }

  const ASTStmt* GetChildStmt() const {
    return child;
  }

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
  ASTStmt* child = nullptr;
};

class ASTNullStmt : public ASTStmt {
 public:
  ASTNullStmt() : ASTStmt(kASTStmtNull) {}
  ~ASTNullStmt() = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTDeclStmt : public ASTStmt {
 public:
  ASTDeclStmt() : ASTStmt(kASTStmtDecl) {}
  ~ASTDeclStmt() = default;

  void SetSubDecl(ASTDecl *decl) {
    subDecls.emplace_back(decl);
  }

  const std::list<ASTDecl*>& GetSubDecls() const {
    return subDecls;
  }

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
  std::list<ASTDecl*> subDecls;
};

class ASTCompoundAssignOperatorStmt : public ASTStmt {
 public:
  ASTCompoundAssignOperatorStmt() : ASTStmt(kASTStmtCAO) {}
  ~ASTCompoundAssignOperatorStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTImplicitCastExprStmt : public ASTStmt {
 public:
  ASTImplicitCastExprStmt() : ASTStmt(kASTStmtImplicitCastExpr) {}
  ~ASTImplicitCastExprStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTParenExprStmt : public ASTStmt {
 public:
  ASTParenExprStmt() : ASTStmt(kASTStmtParenExpr) {}
  ~ASTParenExprStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTIntegerLiteralStmt : public ASTStmt {
 public:
  ASTIntegerLiteralStmt() : ASTStmt(kASTStmtIntegerLiteral) {}
  ~ASTIntegerLiteralStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTFloatingLiteralStmt : public ASTStmt {
 public:
  ASTFloatingLiteralStmt() : ASTStmt(kASTStmtFloatingLiteral) {}
  ~ASTFloatingLiteralStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTVAArgExprStmt : public ASTStmt {
 public:
  ASTVAArgExprStmt() : ASTStmt(kASTStmtVAArgExpr) {}
  ~ASTVAArgExprStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTConditionalOperatorStmt : public ASTStmt {
 public:
  ASTConditionalOperatorStmt() : ASTStmt(kASTStmtConditionalOperator) {}
  ~ASTConditionalOperatorStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTCharacterLiteralStmt : public ASTStmt {
 public:
  ASTCharacterLiteralStmt() : ASTStmt(kASTStmtCharacterLiteral) {}
  ~ASTCharacterLiteralStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTStmtExprStmt : public ASTStmt {
 public:
  ASTStmtExprStmt() : ASTStmt(kASTStmtStmtExpr) {}
  ~ASTStmtExprStmt() override = default;

  void SetBodyStmt(ASTStmt *stmt) {
    cpdStmt = stmt;
  }

  ASTStmt *GetBodyStmt() {
    return cpdStmt;
  }

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;

  ASTStmt *cpdStmt = nullptr;
};

class ASTCStyleCastExprStmt : public ASTStmt {
 public:
  ASTCStyleCastExprStmt() : ASTStmt(kASTStmtCStyleCastExpr) {}
  ~ASTCStyleCastExprStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTCallExprStmt : public ASTStmt {
 public:
  ASTCallExprStmt() : ASTStmt(kASTStmtCallExpr), varName(FEUtils::GetSequentialName("retVar_")) {}
  ~ASTCallExprStmt() override = default;

 private:
  using FuncPtrBuiltinFunc = std::list<UniqueFEIRStmt> (ASTCallExprStmt::*)() const;
  static std::map<std::string, FuncPtrBuiltinFunc> InitFuncPtrMap();
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;

  std::string varName;
};

class ASTAtomicExprStmt : public ASTStmt {
 public:
  ASTAtomicExprStmt() : ASTStmt(kASTStmtAtomicExpr) {}
  ~ASTAtomicExprStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTGCCAsmStmt : public ASTStmt {
 public:
  ASTGCCAsmStmt() : ASTStmt(kASTStmtGCCAsmStmt) {}
  ~ASTGCCAsmStmt() override = default;

  void SetAsmStr(const std::string &str) {
    asmStr = str;
  }

  void InsertOutput(std::tuple<std::string, std::string, bool> &&output) {
    outputs.emplace_back(output);
  }

  void InsertInput(std::pair<std::string, std::string> &&input) {
    inputs.emplace_back(input);
  }

  void InsertClobber(std::string &&clobber) {
    clobbers.emplace_back(clobber);
  }

  void InsertLabel(const std::string &label) {
    labels.emplace_back(label);
  }

  void SetIsGoto(bool flag) {
    isGoto = flag;
  }

  void SetIsVolatile(bool flag) {
    isVolatile = flag;
  }

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
  std::string asmStr;
  std::vector<std::tuple<std::string, std::string, bool>> outputs;
  std::vector<std::pair<std::string, std::string>> inputs;
  std::vector<std::string> clobbers;
  std::vector<std::string> labels;
  bool isGoto = false;
  bool isVolatile = false;
};

class ASTOffsetOfStmt : public ASTStmt {
 public:
  ASTOffsetOfStmt() : ASTStmt(kASTOffsetOfStmt) {}
  ~ASTOffsetOfStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTGenericSelectionExprStmt : public ASTStmt {
 public:
  ASTGenericSelectionExprStmt() : ASTStmt(kASTGenericSelectionExprStmt) {}
  ~ASTGenericSelectionExprStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};
}  // namespace maple
#endif // MPLFE_AST_INPUT_INCLUDE_AST_STMT_H
