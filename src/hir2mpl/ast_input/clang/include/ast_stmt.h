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
#ifndef HIR2MPL_AST_INPUT_INCLUDE_AST_STMT_H
#define HIR2MPL_AST_INPUT_INCLUDE_AST_STMT_H
#include "ast_op.h"
#include "ast_expr.h"
#include "feir_stmt.h"

namespace maple {
class ASTDecl;

enum class SafeSS {
  kNoneSS,
  kSafeSS,
  kUnsafeSS,
};

class ASTStmt {
 public:
  explicit ASTStmt(MapleAllocator &allocatorIn, ASTStmtOp o = kASTStmtNone) : exprs(allocatorIn.Adapter()), op(o) {}
  virtual ~ASTStmt() = default;
  void SetASTExpr(ASTExpr* astExpr);

  std::list<UniqueFEIRStmt> Emit2FEStmt() const {
    auto stmts = Emit2FEStmtImpl();
    for (UniqueFEIRStmt &stmt : stmts) {
      if (stmt != nullptr && !stmt->HasSetLOCInfo()) {
        stmt->SetSrcLoc(loc);
      }
    }
    return stmts;
  }

  ASTStmtOp GetASTStmtOp() const {
    return op;
  }

  const MapleVector<ASTExpr*> &GetExprs() const {
    return exprs;
  }

  void SetSrcLoc(const Loc &l) {
    loc = l;
  }

  const Loc &GetSrcLoc() const {
    return loc;
  }

  uint32 GetSrcFileIdx() const {
    return loc.fileIdx;
  }

  uint32 GetSrcFileLineNum() const {
    return loc.line;
  }

 protected:
  virtual std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const = 0;
  MapleVector<ASTExpr*> exprs;
  ASTStmtOp op;
  Loc loc = {0, 0, 0};
};

class ASTStmtDummy : public ASTStmt {
 public:
  explicit ASTStmtDummy(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtDummy) {}
  ~ASTStmtDummy() = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTCompoundStmt : public ASTStmt {
 public:
  explicit ASTCompoundStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtCompound),
      astStmts(allocatorIn.Adapter()) {}
  ~ASTCompoundStmt() = default;
  void SetASTStmt(ASTStmt *astStmt);
  void InsertASTStmtsAtFront(const std::list<ASTStmt*> &stmts);
  const MapleList<ASTStmt*> &GetASTStmtList() const;

  void SetSafeSS(SafeSS state) {
    safeSS = state;
  }

  SafeSS GetSafeSS() const {
    return safeSS;
  }

  void SetEndLoc(const Loc &loc) {
    endLoc = loc;
  }

  const Loc &GetEndLoc() const {
    return endLoc;
  }

 private:
  SafeSS safeSS = SafeSS::kNoneSS;
  MapleList<ASTStmt*> astStmts; // stmts
  Loc endLoc = {0, 0, 0};
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
  mutable bool hasEmitted2MIRScope = false;
};

// Any other expressions or stmts should be extended here
class ASTReturnStmt : public ASTStmt {
 public:
  explicit ASTReturnStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtReturn) {}
  ~ASTReturnStmt() = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTAttributedStmt : public ASTStmt {
 public:
  explicit ASTAttributedStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtAttributed) {}
  ~ASTAttributedStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override{ return {}; };
};

class ASTIfStmt : public ASTStmt {
 public:
  explicit ASTIfStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtIf) {}
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
  explicit ASTForStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtFor) {}
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

  void SetEndLoc(const Loc &loc) {
    endLoc = loc;
  }

  const Loc &GetEndLoc() const {
    return endLoc;
  }
 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
  ASTStmt *initStmt = nullptr;
  ASTExpr *condExpr = nullptr;
  ASTExpr *incExpr = nullptr;
  ASTStmt *bodyStmt = nullptr;
  Loc endLoc = {0, 0, 0};
  mutable bool hasEmitted2MIRScope = false;
};

class ASTWhileStmt : public ASTStmt {
 public:
  explicit ASTWhileStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtWhile) {}
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
  mutable bool hasEmitted2MIRScope = false;
};

class ASTDoStmt : public ASTStmt {
 public:
  explicit ASTDoStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtDo) {}
  ~ASTDoStmt() override = default;

  void SetBodyStmt(ASTStmt *astStmt) {
    bodyStmt = astStmt;
  }

  void SetCondExpr(ASTExpr *astExpr) {
    condExpr = astExpr;
  }

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
  ASTStmt *bodyStmt = nullptr;
  ASTExpr *condExpr = nullptr;
  mutable bool hasEmitted2MIRScope = false;
};

class ASTBreakStmt : public ASTStmt {
 public:
  explicit ASTBreakStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtBreak) {}
  ~ASTBreakStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTLabelStmt : public ASTStmt {
 public:
  explicit ASTLabelStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtLabel),
      labelName("", allocatorIn.GetMemPool()) {}
  ~ASTLabelStmt() override = default;

  void SetSubStmt(ASTStmt *stmt) {
    subStmt = stmt;
  }

  const ASTStmt* GetSubStmt() const {
    return subStmt;
  }

  void SetLabelName(const std::string &name) {
    labelName = name;
  }

  const std::string GetLabelName() const {
    return labelName.c_str() == nullptr ? "" : labelName.c_str();
  }

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
  MapleString labelName;
  ASTStmt *subStmt = nullptr;
};

class ASTContinueStmt : public ASTStmt {
 public:
  explicit ASTContinueStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtContinue) {}
  ~ASTContinueStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTUnaryOperatorStmt : public ASTStmt {
 public:
  explicit ASTUnaryOperatorStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtUO) {}
  ~ASTUnaryOperatorStmt() = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTBinaryOperatorStmt : public ASTStmt {
 public:
  explicit ASTBinaryOperatorStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtBO) {}
  ~ASTBinaryOperatorStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTGotoStmt : public ASTStmt {
 public:
  explicit ASTGotoStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtGoto),
      labelName("", allocatorIn.GetMemPool()) {}
  ~ASTGotoStmt() = default;

  std::string GetLabelName() const {
    return labelName.c_str() == nullptr ? "" : labelName.c_str();
  }

  void SetLabelName(const std::string &name) {
    labelName = name;
  }

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
  MapleString labelName;
};

class ASTIndirectGotoStmt : public ASTStmt {
 public:
  explicit ASTIndirectGotoStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtIndirectGoto) {}
  ~ASTIndirectGotoStmt() = default;

 protected:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTSwitchStmt : public ASTStmt {
 public:
  explicit ASTSwitchStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtSwitch) {}
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
  mutable bool hasEmitted2MIRScope = false;
};

class ASTCaseStmt : public ASTStmt {
 public:
  explicit ASTCaseStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtCase) {}
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
  int64 lCaseTag = 0;
  int64 rCaseTag = 0;
};

class ASTDefaultStmt : public ASTStmt {
 public:
  explicit ASTDefaultStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtDefault) {}
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
  explicit ASTNullStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtNull) {}
  ~ASTNullStmt() = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTDeclStmt : public ASTStmt {
 public:
  explicit ASTDeclStmt(MapleAllocator &allocatorIn)
      : ASTStmt(allocatorIn, kASTStmtDecl),
        subDecls(allocatorIn.Adapter()),
        subDeclInfos(allocatorIn.Adapter()) {}
  ~ASTDeclStmt() = default;

  void SetSubDecl(ASTDecl *decl) {
    subDecls.emplace_back(decl);
    (void)subDeclInfos.emplace_back(decl);
  }

  void SetVLASizeExpr(ASTExpr *astExpr) {
    (void)subDeclInfos.emplace_back(astExpr);
  }

  const MapleList<ASTDecl*> &GetSubDecls() const {
    return subDecls;
  }

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;

  MapleList<ASTDecl*> subDecls;
  // saved vla size exprs before a vla decl
  MapleList<std::variant<ASTDecl*, ASTExpr*>> subDeclInfos;
};

class ASTCompoundAssignOperatorStmt : public ASTStmt {
 public:
  explicit ASTCompoundAssignOperatorStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtCAO) {}
  ~ASTCompoundAssignOperatorStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTImplicitCastExprStmt : public ASTStmt {
 public:
  explicit ASTImplicitCastExprStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtImplicitCastExpr) {}
  ~ASTImplicitCastExprStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTParenExprStmt : public ASTStmt {
 public:
  explicit ASTParenExprStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtParenExpr) {}
  ~ASTParenExprStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTIntegerLiteralStmt : public ASTStmt {
 public:
  explicit ASTIntegerLiteralStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtIntegerLiteral) {}
  ~ASTIntegerLiteralStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTFloatingLiteralStmt : public ASTStmt {
 public:
  explicit ASTFloatingLiteralStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtFloatingLiteral) {}
  ~ASTFloatingLiteralStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTVAArgExprStmt : public ASTStmt {
 public:
  explicit ASTVAArgExprStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtVAArgExpr) {}
  ~ASTVAArgExprStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTConditionalOperatorStmt : public ASTStmt {
 public:
  explicit ASTConditionalOperatorStmt(MapleAllocator &allocatorIn)
      : ASTStmt(allocatorIn, kASTStmtConditionalOperator) {}
  ~ASTConditionalOperatorStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTCharacterLiteralStmt : public ASTStmt {
 public:
  explicit ASTCharacterLiteralStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtCharacterLiteral) {}
  ~ASTCharacterLiteralStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTStmtExprStmt : public ASTStmt {
 public:
  explicit ASTStmtExprStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtStmtExpr) {}
  ~ASTStmtExprStmt() override = default;

  void SetBodyStmt(ASTStmt *stmt) {
    cpdStmt = stmt;
  }

  const ASTStmt *GetBodyStmt() const {
    return cpdStmt;
  }

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;

  ASTStmt *cpdStmt = nullptr;
};

class ASTCStyleCastExprStmt : public ASTStmt {
 public:
  explicit ASTCStyleCastExprStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtCStyleCastExpr) {}
  ~ASTCStyleCastExprStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTCallExprStmt : public ASTStmt {
 public:
  ASTCallExprStmt(MapleAllocator &allocatorIn, const std::string &varNameIn)
      : ASTStmt(allocatorIn, kASTStmtCallExpr), varName(varNameIn) {}
  ~ASTCallExprStmt() override = default;

 private:
  using FuncPtrBuiltinFunc = std::list<UniqueFEIRStmt> (ASTCallExprStmt::*)() const;
  static std::map<std::string, FuncPtrBuiltinFunc> InitFuncPtrMap();
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;

  std::string varName;
};

class ASTAtomicExprStmt : public ASTStmt {
 public:
  explicit ASTAtomicExprStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtAtomicExpr) {}
  ~ASTAtomicExprStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTGCCAsmStmt : public ASTStmt {
 public:
  explicit ASTGCCAsmStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtGCCAsmStmt),
      asmStr("", allocatorIn.GetMemPool()), outputs(allocatorIn.Adapter()), inputs(allocatorIn.Adapter()),
      clobbers(allocatorIn.Adapter()), labels(allocatorIn.Adapter()) {}
  ~ASTGCCAsmStmt() override = default;

  void SetAsmStr(const std::string &str) {
    asmStr = str;
  }

  const std::string GetAsmStr() const {
    return asmStr.c_str() == nullptr ? "" : asmStr.c_str();
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
  MapleString asmStr;
  MapleVector<std::tuple<std::string, std::string, bool>> outputs;
  MapleVector<std::pair<std::string, std::string>> inputs;
  MapleVector<std::string> clobbers;
  MapleVector<std::string> labels;
  bool isGoto = false;
  bool isVolatile = false;
};

class ASTOffsetOfStmt : public ASTStmt {
 public:
  explicit ASTOffsetOfStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTOffsetOfStmt) {}
  ~ASTOffsetOfStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTGenericSelectionExprStmt : public ASTStmt {
 public:
  explicit ASTGenericSelectionExprStmt(MapleAllocator &allocatorIn)
      : ASTStmt(allocatorIn, kASTGenericSelectionExprStmt) {}
  ~ASTGenericSelectionExprStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTDeclRefExprStmt : public ASTStmt {
 public:
  explicit ASTDeclRefExprStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtDeclRefExpr) {}
  ~ASTDeclRefExprStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTUnaryExprOrTypeTraitExprStmt : public ASTStmt {
 public:
  explicit ASTUnaryExprOrTypeTraitExprStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtDeclRefExpr) {}
  ~ASTUnaryExprOrTypeTraitExprStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};

class ASTUOAddrOfLabelExprStmt : public ASTStmt {
 public:
  explicit ASTUOAddrOfLabelExprStmt(MapleAllocator &allocatorIn) : ASTStmt(allocatorIn, kASTStmtAddrOfLabelExpr) {}
  ~ASTUOAddrOfLabelExprStmt() override = default;

 private:
  std::list<UniqueFEIRStmt> Emit2FEStmtImpl() const override;
};
}  // namespace maple
#endif // HIR2MPL_AST_INPUT_INCLUDE_AST_STMT_H
