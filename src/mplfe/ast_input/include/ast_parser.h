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
#ifndef MPLFE_AST_INPUT_INCLUDE_AST_PARSER_H
#define MPLFE_AST_INPUT_INCLUDE_AST_PARSER_H
#include <string>
#include <functional>
#include "mempool_allocator.h"
#include "ast_decl.h"
#include "ast_interface.h"

namespace maple {
class ASTInput;
class ASTParser {
 public:
  ASTParser(MapleAllocator &allocatorIn, uint32 fileIdxIn, const std::string &fileNameIn,
            MapleList<ASTStruct*> &astStructsIn, MapleList<ASTFunc*> &astFuncsIn, MapleList<ASTVar*> &astVarsIn,
            MapleList<ASTFileScopeAsm*> &astFileScopeAsmsIn)
      : fileIdx(fileIdxIn), fileName(fileNameIn), globalVarDecles(allocatorIn.Adapter()),
        funcDecles(allocatorIn.Adapter()), recordDecles(allocatorIn.Adapter()),
        globalEnumDecles(allocatorIn.Adapter()), globalTypeDefDecles(allocatorIn.Adapter()),
        globalFileScopeAsm(allocatorIn.Adapter()), astStructs(astStructsIn), astFuncs(astFuncsIn),
        astVars(astVarsIn), astFileScopeAsms(astFileScopeAsmsIn) {}
  virtual ~ASTParser() = default;
  bool OpenFile();
  bool Verify() const;
  bool PreProcessAST();

  bool RetrieveStructs(MapleAllocator &allocator);
  bool RetrieveFuncs(MapleAllocator &allocator);
  bool RetrieveGlobalVars(MapleAllocator &allocator);
  bool RetrieveFileScopeAsms(MapleAllocator &allocator);

  bool ProcessGlobalTypeDef(MapleAllocator &allocator);

  const std::string &GetSourceFileName() const;
  const uint32 GetFileIdx() const;
  void SetAstIn(ASTInput *in) {
    astIn = in;
  }
  // ProcessStmt
  ASTStmt *ProcessStmt(MapleAllocator &allocator, const clang::Stmt &stmt);
  ASTStmt *ProcessFunctionBody(MapleAllocator &allocator, const clang::CompoundStmt &cpdStmt);
#define PROCESS_STMT(CLASS) ProcessStmt##CLASS(MapleAllocator&, const clang::CLASS&)
  ASTStmt *PROCESS_STMT(UnaryOperator);
  ASTStmt *PROCESS_STMT(BinaryOperator);
  ASTStmt *PROCESS_STMT(CompoundAssignOperator);
  ASTStmt *PROCESS_STMT(ImplicitCastExpr);
  ASTStmt *PROCESS_STMT(ParenExpr);
  ASTStmt *PROCESS_STMT(IntegerLiteral);
  ASTStmt *PROCESS_STMT(FloatingLiteral);
  ASTStmt *PROCESS_STMT(VAArgExpr);
  ASTStmt *PROCESS_STMT(ConditionalOperator);
  ASTStmt *PROCESS_STMT(CharacterLiteral);
  ASTStmt *PROCESS_STMT(StmtExpr);
  ASTStmt *PROCESS_STMT(CallExpr);
  ASTStmt *PROCESS_STMT(ReturnStmt);
  ASTStmt *PROCESS_STMT(IfStmt);
  ASTStmt *PROCESS_STMT(ForStmt);
  ASTStmt *PROCESS_STMT(WhileStmt);
  ASTStmt *PROCESS_STMT(DoStmt);
  ASTStmt *PROCESS_STMT(BreakStmt);
  ASTStmt *PROCESS_STMT(LabelStmt);
  ASTStmt *PROCESS_STMT(ContinueStmt);
  ASTStmt *PROCESS_STMT(CompoundStmt);
  ASTStmt *PROCESS_STMT(GotoStmt);
  ASTStmt *PROCESS_STMT(IndirectGotoStmt);
  ASTStmt *PROCESS_STMT(SwitchStmt);
  ASTStmt *PROCESS_STMT(CaseStmt);
  ASTStmt *PROCESS_STMT(DefaultStmt);
  ASTStmt *PROCESS_STMT(NullStmt);
  ASTStmt *PROCESS_STMT(CStyleCastExpr);
  ASTStmt *PROCESS_STMT(DeclStmt);
  ASTStmt *PROCESS_STMT(AtomicExpr);
  ASTStmt *PROCESS_STMT(GCCAsmStmt);
  ASTStmt *PROCESS_STMT(OffsetOfExpr);
  ASTStmt *PROCESS_STMT(GenericSelectionExpr);
  bool HasDefault(const clang::Stmt &stmt);

  // ProcessExpr
  const clang::Expr *PeelParen(const clang::Expr &expr);
  const clang::Expr *PeelParen2(const clang::Expr &expr);
  ASTUnaryOperatorExpr *AllocUnaryOperatorExpr(MapleAllocator &allocator, const clang::UnaryOperator &expr);
  ASTValue *AllocASTValue(MapleAllocator &allocator) const;
  ASTValue *TranslateExprEval(MapleAllocator &allocator, const clang::Expr *expr) const;
  ASTExpr *EvaluateExprAsConst(MapleAllocator &allocator, const clang::Expr *expr);
  ASTExpr *ProcessExpr(MapleAllocator &allocator, const clang::Expr *expr);
  ASTExpr *ProcessExprInType(MapleAllocator &allocator, const clang::QualType &qualType);
  ASTBinaryOperatorExpr *AllocBinaryOperatorExpr(MapleAllocator &allocator, const clang::BinaryOperator &bo);
  ASTExpr *ProcessExprCastExpr(MapleAllocator &allocator, const clang::CastExpr &expr);
#define PROCESS_EXPR(CLASS) ProcessExpr##CLASS(MapleAllocator&, const clang::CLASS&)
  ASTExpr *PROCESS_EXPR(UnaryOperator);
  ASTExpr *PROCESS_EXPR(AddrLabelExpr);
  ASTExpr *PROCESS_EXPR(NoInitExpr);
  ASTExpr *PROCESS_EXPR(PredefinedExpr);
  ASTExpr *PROCESS_EXPR(OpaqueValueExpr);
  ASTExpr *PROCESS_EXPR(BinaryConditionalOperator);
  ASTExpr *PROCESS_EXPR(CompoundLiteralExpr);
  ASTExpr *PROCESS_EXPR(OffsetOfExpr);
  ASTExpr *PROCESS_EXPR(InitListExpr);
  ASTExpr *PROCESS_EXPR(BinaryOperator);
  ASTExpr *PROCESS_EXPR(ImplicitValueInitExpr);
  ASTExpr *PROCESS_EXPR(StringLiteral);
  ASTExpr *PROCESS_EXPR(ArraySubscriptExpr);
  ASTExpr *PROCESS_EXPR(UnaryExprOrTypeTraitExpr);
  ASTExpr *PROCESS_EXPR(MemberExpr);
  ASTExpr *PROCESS_EXPR(DesignatedInitUpdateExpr);
  ASTExpr *PROCESS_EXPR(ImplicitCastExpr);
  ASTExpr *PROCESS_EXPR(DeclRefExpr);
  ASTExpr *PROCESS_EXPR(ParenExpr);
  ASTExpr *PROCESS_EXPR(IntegerLiteral);
  ASTExpr *PROCESS_EXPR(FloatingLiteral);
  ASTExpr *PROCESS_EXPR(CharacterLiteral);
  ASTExpr *PROCESS_EXPR(ConditionalOperator);
  ASTExpr *PROCESS_EXPR(VAArgExpr);
  ASTExpr *PROCESS_EXPR(GNUNullExpr);
  ASTExpr *PROCESS_EXPR(SizeOfPackExpr);
  ASTExpr *PROCESS_EXPR(UserDefinedLiteral);
  ASTExpr *PROCESS_EXPR(ShuffleVectorExpr);
  ASTExpr *PROCESS_EXPR(TypeTraitExpr);
  ASTExpr *PROCESS_EXPR(ConstantExpr);
  ASTExpr *PROCESS_EXPR(ImaginaryLiteral);
  ASTExpr *PROCESS_EXPR(CallExpr);
  ASTExpr *PROCESS_EXPR(CompoundAssignOperator);
  ASTExpr *PROCESS_EXPR(StmtExpr);
  ASTExpr *PROCESS_EXPR(CStyleCastExpr);
  ASTExpr *PROCESS_EXPR(ArrayInitLoopExpr);
  ASTExpr *PROCESS_EXPR(ArrayInitIndexExpr);
  ASTExpr *PROCESS_EXPR(ExprWithCleanups);
  ASTExpr *PROCESS_EXPR(MaterializeTemporaryExpr);
  ASTExpr *PROCESS_EXPR(SubstNonTypeTemplateParmExpr);
  ASTExpr *PROCESS_EXPR(DependentScopeDeclRefExpr);
  ASTExpr *PROCESS_EXPR(AtomicExpr);
  ASTExpr *PROCESS_EXPR(ChooseExpr);
  ASTExpr *PROCESS_EXPR(GenericSelectionExpr);

  ASTDecl *ProcessDecl(MapleAllocator &allocator, const clang::Decl &decl);
#define PROCESS_DECL(CLASS) ProcessDecl##CLASS##Decl(MapleAllocator &allocator, const clang::CLASS##Decl&)
  ASTDecl *PROCESS_DECL(Field);
  ASTDecl *PROCESS_DECL(Function);
  ASTDecl *PROCESS_DECL(Record);
  ASTDecl *PROCESS_DECL(Var);
  ASTDecl *PROCESS_DECL(ParmVar);
  ASTDecl *PROCESS_DECL(Enum);
  ASTDecl *PROCESS_DECL(Typedef);
  ASTDecl *PROCESS_DECL(EnumConstant);
  ASTDecl *PROCESS_DECL(FileScopeAsm);
  ASTDecl *PROCESS_DECL(Label);
  ASTDecl *PROCESS_DECL(StaticAssert);

 private:
  void ProcessNonnullFuncAttrs(const clang::FunctionDecl &funcDecl, ASTFunc &astFunc);
  void ProcessBoundaryFuncAttrs(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl, ASTFunc &astFunc);
  void ProcessBoundaryParamAttrs(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl, ASTFunc &astFunc);
  void ProcessBoundaryLenExpr(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl, unsigned int idx,
                              ASTFunc &astFunc, ASTExpr *lenExpr, bool isSize);
  ASTValue *TranslateConstantValue2ASTValue(MapleAllocator &allocator, const clang::Expr *expr) const;
  ASTValue *TranslateLValue2ASTValue(MapleAllocator &allocator,
      const clang::Expr::EvalResult &result, const clang::Expr *expr) const;
  void TraverseDecl(const clang::Decl *decl, std::function<void (clang::Decl*)> const &functor);
  ASTDecl *GetAstDeclOfDeclRefExpr(MapleAllocator &allocator, const clang::Expr &expr);
  uint32 GetSizeFromQualType(const clang::QualType qualType);
  ASTExpr *GetTypeSizeFromQualType(MapleAllocator &allocator, const clang::QualType qualType);
  uint32_t GetAlignOfType(const clang::QualType currQualType, clang::UnaryExprOrTypeTrait exprKind);
  uint32_t GetAlignOfExpr(const clang::Expr &expr, clang::UnaryExprOrTypeTrait exprKind);
  void GetAddrShiftExpr(MapleAllocator &allocator, ASTExpr *&expr, uint32 typeSize);
  ASTExpr *BuildExprToComputeSizeFromVLA(MapleAllocator &allocator, const clang::QualType &qualType);
  ASTExpr *ProcessExprBinaryOperatorComplex(MapleAllocator &allocator, const clang::BinaryOperator &bo);

using FuncPtrBuiltinFunc = ASTExpr *(ASTParser::*)(MapleAllocator &allocator, const clang::CallExpr &expr,
                                                   std::stringstream &ss) const;
static std::map<std::string, FuncPtrBuiltinFunc> InitBuiltinFuncPtrMap();
ASTExpr *ProcessBuiltinFuncByName(MapleAllocator &allocator, const clang::CallExpr &expr, std::stringstream &ss,
                                  const std::string &name) const;
ASTExpr *ParseBuiltinFunc(MapleAllocator &allocator, const clang::CallExpr &expr, std::stringstream &ss) const;
#define PARSE_BUILTIIN_FUNC(FUNC) ParseBuiltin##FUNC(MapleAllocator &allocator, const clang::CallExpr &expr,\
                                                     std::stringstream &ss) const
  ASTExpr *PARSE_BUILTIIN_FUNC(ClassifyType);
  ASTExpr *PARSE_BUILTIIN_FUNC(ConstantP);
  ASTExpr *PARSE_BUILTIIN_FUNC(Isinfsign);
  ASTExpr *PARSE_BUILTIIN_FUNC(HugeVal);
  ASTExpr *PARSE_BUILTIIN_FUNC(HugeValf);
  ASTExpr *PARSE_BUILTIIN_FUNC(Inf);
  ASTExpr *PARSE_BUILTIIN_FUNC(Inff);
  ASTExpr *PARSE_BUILTIIN_FUNC(Nan);
  ASTExpr *PARSE_BUILTIIN_FUNC(Nanf);
  ASTExpr *PARSE_BUILTIIN_FUNC(Signbit);
  ASTExpr *PARSE_BUILTIIN_FUNC(SignBitf);
  ASTExpr *PARSE_BUILTIIN_FUNC(SignBitl);
  ASTExpr *PARSE_BUILTIIN_FUNC(Trap);
  ASTExpr *PARSE_BUILTIIN_FUNC(IsUnordered);
  ASTExpr *PARSE_BUILTIIN_FUNC(Copysignf);
  ASTExpr *PARSE_BUILTIIN_FUNC(Copysign);
  ASTExpr *PARSE_BUILTIIN_FUNC(Copysignl);
  ASTExpr *PARSE_BUILTIIN_FUNC(Objectsize);

  static std::map<std::string, FuncPtrBuiltinFunc> builtingFuncPtrMap;
  uint32 fileIdx;
  const std::string fileName;
  std::unique_ptr<LibAstFile> astFile;
  AstUnitDecl *astUnitDecl = nullptr;
  MapleList<clang::Decl*> globalVarDecles;
  MapleList<clang::Decl*> funcDecles;
  MapleList<clang::Decl*> recordDecles;
  MapleList<clang::Decl*> globalEnumDecles;
  MapleList<clang::Decl*> globalTypeDefDecles;
  MapleList<clang::Decl*> globalFileScopeAsm;

  MapleList<ASTStruct*> &astStructs;
  MapleList<ASTFunc*> &astFuncs;
  MapleList<ASTVar*> &astVars;
  MapleList<ASTFileScopeAsm*> &astFileScopeAsms;

  ASTInput *astIn = nullptr;
};
}  // namespace maple
#endif // MPLFE_AST_INPUT_INCLUDE_AST_PARSER_H
