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
#ifndef HIR2MPL_AST_INPUT_INCLUDE_AST_PARSER_H
#define HIR2MPL_AST_INPUT_INCLUDE_AST_PARSER_H
#include <string>
#include <functional>
#include "mempool_allocator.h"
#include "ast_decl.h"
#include "ast_decl_builder.h"
#include "ast_interface.h"

namespace maple {
class ASTParser {
 public:
  ASTParser(MapleAllocator &allocatorIn, uint32 fileIdxIn, const std::string &fileNameIn,
            MapleList<ASTStruct*> &astStructsIn, MapleList<ASTFunc*> &astFuncsIn, MapleList<ASTVar*> &astVarsIn,
            MapleList<ASTFileScopeAsm*> &astFileScopeAsmsIn, MapleList<ASTEnumDecl*> &astEnumsIn)
      : fileIdx(fileIdxIn),
        fileName(fileNameIn, allocatorIn.GetMemPool()),
        globalVarDecles(allocatorIn.Adapter()),
        funcDecles(allocatorIn.Adapter()),
        recordDecles(allocatorIn.Adapter()),
        globalEnumDecles(allocatorIn.Adapter()),
        globalTypeDefDecles(allocatorIn.Adapter()),
        globalFileScopeAsm(allocatorIn.Adapter()),
        astStructs(astStructsIn),
        astFuncs(astFuncsIn),
        astVars(astVarsIn),
        astFileScopeAsms(astFileScopeAsmsIn),
        astEnums(astEnumsIn),
        vlaSizeMap(allocatorIn.Adapter()),
        structFileNameMap(allocatorIn.Adapter()) {}
  virtual ~ASTParser() = default;
  bool OpenFile(MapleAllocator &allocator);
  bool Release(MapleAllocator &allocator) const;

  bool Verify() const;
  bool PreProcessAST();

  bool RetrieveStructs(MapleAllocator &allocator);
  bool RetrieveFuncs(MapleAllocator &allocator);
  bool RetrieveGlobalVars(MapleAllocator &allocator);
  bool RetrieveFileScopeAsms(MapleAllocator &allocator);
  bool RetrieveGlobalTypeDef(MapleAllocator &allocator);
  bool RetrieveEnums(MapleAllocator &allocator);

  const std::string GetSourceFileName() const;
  const uint32 GetFileIdx() const;

  // ProcessStmt
  ASTStmt *ProcessStmt(MapleAllocator &allocator, const clang::Stmt &stmt);
  ASTStmt *ProcessFunctionBody(MapleAllocator &allocator, const clang::CompoundStmt &compoundStmt);
#define PROCESS_STMT(CLASS) ProcessStmt##CLASS(MapleAllocator &allocator, const clang::CLASS &expr)
  ASTStmt *PROCESS_STMT(AttributedStmt);
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
  ASTStmt *PROCESS_STMT(DeclRefExpr);
  ASTStmt *PROCESS_STMT(UnaryExprOrTypeTraitExpr);
  ASTStmt *PROCESS_STMT(AddrLabelExpr);
  ASTStmt *PROCESS_STMT(MemberExpr);
  bool HasDefault(const clang::Stmt &stmt);

  // ProcessExpr
  const clang::Expr *PeelParen(const clang::Expr &expr) const;
  const clang::Expr *PeelParen2(const clang::Expr &expr) const;
  ASTUnaryOperatorExpr *AllocUnaryOperatorExpr(MapleAllocator &allocator, const clang::UnaryOperator &expr) const;
  ASTValue *AllocASTValue(const MapleAllocator &allocator) const;
  ASTValue *TranslateExprEval(MapleAllocator &allocator, const clang::Expr *expr) const;
  ASTExpr *EvaluateExprAsConst(MapleAllocator &allocator, const clang::Expr *expr);
  bool HasLabelStmt(const clang::Stmt *expr);
  ASTExpr *ProcessExpr(MapleAllocator &allocator, const clang::Expr *expr);
  void SaveVLASizeExpr(MapleAllocator &allocator, const clang::Type &type, MapleList<ASTExpr*> &vlaSizeExprs) {
    if (!type.isVariableArrayType()) {
      return;
    }
    const clang::VariableArrayType *vlaType = llvm::cast<clang::VariableArrayType>(&type);
    if (vlaSizeMap.find(vlaType->getSizeExpr()) != vlaSizeMap.cend()) {
      return;  // vla size expr already exists
    }
    const clang::QualType qualType = type.getCanonicalTypeInternal();
    ASTExpr *vlaSizeExpr = BuildExprToComputeSizeFromVLA(allocator, qualType.getCanonicalType());
    if (vlaSizeExpr == nullptr) {
      return;
    }
    ASTDeclRefExpr *vlaSizeVarExpr = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
    MIRType *vlaSizeType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(PTY_u64);
    ASTDecl *vlaSizeVar = ASTDeclsBuilder::GetInstance(allocator).ASTDeclBuilder(
        allocator, MapleString("", allocator.GetMemPool()), FEUtils::GetSequentialName("vla_size."),
        MapleVector<MIRType*>({vlaSizeType}, allocator.Adapter()));
    vlaSizeVar->SetIsParam(true);
    vlaSizeVarExpr->SetASTDecl(vlaSizeVar);
    ASTAssignExpr *expr = ASTDeclsBuilder::ASTStmtBuilder<ASTAssignExpr>(allocator);
    expr->SetLeftExpr(vlaSizeVarExpr);
    expr->SetRightExpr(vlaSizeExpr);
    vlaSizeMap[vlaType->getSizeExpr()] = vlaSizeVarExpr;
    (void)vlaSizeExprs.emplace_back(expr);
    SaveVLASizeExpr(allocator, *(vlaType->getElementType().getCanonicalType().getTypePtr()), vlaSizeExprs);
  }
  ASTExpr *ProcessTypeofExpr(MapleAllocator &allocator, clang::QualType type);
  ASTBinaryOperatorExpr *AllocBinaryOperatorExpr(MapleAllocator &allocator, const clang::BinaryOperator &bo) const;
  ASTExpr *ProcessExprCastExpr(MapleAllocator &allocator, const clang::CastExpr &expr,
                               const clang::Type **vlaType = nullptr);
  ASTExpr *SolvePointerOffsetOperation(MapleAllocator &allocator, const clang::BinaryOperator &bo,
                                       ASTBinaryOperatorExpr &astBinOpExpr, ASTExpr &astRExpr, ASTExpr &astLExpr);
  ASTExpr *SolvePointerSubPointerOperation(MapleAllocator &allocator, const clang::BinaryOperator &bo,
                                           ASTBinaryOperatorExpr &astBinOpExpr) const;
  void SetFieldLenNameInMemberExpr(MapleAllocator &allocator, ASTMemberExpr &astMemberExpr,
      const clang::FieldDecl &fieldDecl);
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

  MapleVector<ASTDecl*> SolveFuncParameterDecls(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl,
                                                MapleVector<MIRType*> &typeDescIn, std::list<ASTStmt*> &stmts,
                                                bool needBody);
  GenericAttrs SolveFunctionAttributes(const clang::FunctionDecl &funcDecl, std::string &funcName) const;
  ASTDecl *ProcessDecl(MapleAllocator &allocator, const clang::Decl &decl);
  ASTStmt *SolveFunctionBody(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl, ASTFunc &astFunc,
                             const std::list<ASTStmt*> &stmts);

  void SetInitExprForASTVar(MapleAllocator &allocator, const clang::VarDecl &varDecl, const GenericAttrs &attrs,
                            ASTVar &astVar);
  void SetAlignmentForASTVar(const clang::VarDecl &varDecl, ASTVar &astVar) const;
#define PROCESS_DECL(CLASS) ProcessDecl##CLASS##Decl(MapleAllocator &allocator, const clang::CLASS##Decl &decl)
  ASTDecl *PROCESS_DECL(Field);
  ASTDecl *PROCESS_DECL(Record);
  ASTDecl *PROCESS_DECL(Var);
  ASTDecl *PROCESS_DECL(ParmVar);
  ASTDecl *PROCESS_DECL(Enum);
  ASTDecl *PROCESS_DECL(Typedef);
  ASTDecl *PROCESS_DECL(EnumConstant);
  ASTDecl *PROCESS_DECL(FileScopeAsm);
  ASTDecl *PROCESS_DECL(Label);
  ASTDecl *PROCESS_DECL(StaticAssert);
  ASTDecl *ProcessDeclFunctionDecl(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl,
                                   bool needBody = false);
  static ASTExpr *GetAddrShiftExpr(MapleAllocator &allocator, ASTExpr &expr, uint32 typeSize);
  static ASTExpr *GetSizeMulExpr(MapleAllocator &allocator, ASTExpr &expr, ASTExpr &ptrSizeExpr);

 private:
  void ProcessNonnullFuncAttrs(const clang::FunctionDecl &funcDecl, ASTFunc &astFunc) const;
  void ProcessNonnullFuncPtrAttrs(MapleAllocator &allocator, const clang::ValueDecl &valueDecl, ASTDecl &astVar);
  void ProcessBoundaryFuncAttrs(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl, ASTFunc &astFunc);
  void ProcessByteBoundaryFuncAttrs(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl, ASTFunc &astFunc);
  void ProcessBoundaryFuncAttrsByIndex(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl,
                                       ASTFunc &astFunc);
  void ProcessBoundaryParamAttrs(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl, ASTFunc &astFunc);
  void ProcessBoundaryParamAttrsByIndex(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl,
                                        ASTFunc &astFunc);
  void ProcessBoundaryVarAttrs(MapleAllocator &allocator, const clang::VarDecl &varDecl, ASTVar &astVar);
  void ProcessBoundaryFieldAttrs(MapleAllocator &allocator, const ASTStruct &structDecl,
                                 const clang::RecordDecl &recDecl);
  void ProcessBoundaryFuncPtrAttrs(MapleAllocator &allocator, const clang::ValueDecl &valueDecl, ASTDecl &astDecl);
  template <typename T>
  bool ProcessBoundaryFuncPtrAttrsForParams(T *attr, MapleAllocator &allocator, const MIRFuncType &funcType,
                                            const clang::FunctionProtoType &proto, std::vector<TypeAttrs> &attrsVec);
  template <typename T>
  bool ProcessBoundaryFuncPtrAttrsForRet(T *attr, MapleAllocator &allocator, const MIRFuncType &funcType,
                                         const clang::FunctionType &clangFuncType, TypeAttrs &retAttr);
  void ProcessBoundaryFuncPtrAttrsByIndex(MapleAllocator &allocator, const clang::ValueDecl &valueDecl,
                                          ASTDecl &astDecl, const MIRFuncType &funcType);
  template <typename T>
  bool ProcessBoundaryFuncPtrAttrsByIndexForParams(T *attr, ASTDecl &astDecl, const MIRFuncType &funcType,
                                                   std::vector<TypeAttrs> &attrsVec) const;
  void ProcessBoundaryLenExpr(MapleAllocator &allocator, ASTDecl &ptrDecl, const clang::QualType &qualType,
                              const std::function<ASTExpr* ()> &getLenExprFromStringLiteral,
                              ASTExpr *lenExpr, bool isSize);
  void ProcessBoundaryLenExprInFunc(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl,
                                    unsigned int idx, ASTFunc &astFunc, ASTExpr *lenExpr, bool isSize);
  void ProcessBoundaryLenExprInFunc(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl,
                                    unsigned int idx, ASTFunc &astFunc, unsigned int lenIdx, bool isSize);
  void ProcessBoundaryLenExprInVar(MapleAllocator &allocator, ASTDecl &ptrDecl, const clang::VarDecl &varDecl,
                                   ASTExpr *lenExpr, bool isSize);
  void ProcessBoundaryLenExprInVar(MapleAllocator &allocator, ASTDecl &ptrDecl, const clang::QualType &qualType,
                                   ASTExpr *lenExpr, bool isSize);
  void ProcessBoundaryLenExprInField(MapleAllocator &allocator, ASTDecl &ptrDecl, const ASTStruct &structDecl,
                                     const clang::QualType &qualType, ASTExpr *lenExpr, bool isSize);
  ASTValue *TranslateConstantValue2ASTValue(MapleAllocator &allocator, const clang::Expr *expr) const;
  ASTValue *TranslateLValue2ASTValue(MapleAllocator &allocator,
      const clang::Expr::EvalResult &result, const clang::Expr *expr) const;
  void TraverseDecl(const clang::Decl *decl, std::function<void (clang::Decl*)> const &functor) const;
  ASTDecl *GetAstDeclOfDeclRefExpr(MapleAllocator &allocator, const clang::Expr &expr);
  uint32 GetSizeFromQualType(const clang::QualType qualType) const;
  ASTExpr *GetSizeOfExpr(MapleAllocator &allocator, const clang::UnaryExprOrTypeTraitExpr &expr,
                         clang::QualType qualType);
  uint32_t GetNumsOfInitListExpr(const clang::InitListExpr &expr);
  ASTExpr *GetSizeOfType(MapleAllocator &allocator, const clang::QualType &qualType);
  ASTExpr *GetTypeSizeFromQualType(MapleAllocator &allocator, const clang::QualType qualType);
  uint32_t GetAlignOfType(const clang::QualType currQualType, clang::UnaryExprOrTypeTrait exprKind) const;
  uint32_t GetAlignOfExpr(const clang::Expr &expr, clang::UnaryExprOrTypeTrait exprKind) const;
  ASTExpr *BuildExprToComputeSizeFromVLA(MapleAllocator &allocator, const clang::QualType &qualType);
  ASTExpr *ProcessExprBinaryOperatorComplex(MapleAllocator &allocator, const clang::BinaryOperator &bo);
  bool CheckIncContinueStmtExpr(const clang::Stmt &bodyStmt) const;
  void CheckVarNameValid(const std::string &varName) const;
  void ParserExprVLASizeExpr(MapleAllocator &allocator, const clang::Type &type, ASTExpr &expr);
  void ParserStmtVLASizeExpr(MapleAllocator &allocator, const clang::Type &type, std::list<ASTStmt*> &stmts);
  void SetAtomExprValType(MapleAllocator &allocator, const clang::AtomicExpr &atomicExpr, ASTAtomicExpr &astExpr);
  void SetAtomExchangeType(MapleAllocator &allocator, const clang::AtomicExpr &atomicExpr, ASTAtomicExpr &astExpr);
  clang::Expr *GetAtomValExpr(clang::Expr *valExpr) const;
  clang::QualType GetPointeeType(const clang::Expr &expr) const;
  bool IsNeedGetPointeeType(const clang::FunctionDecl &funcDecl) const;
  MapleVector<MIRType*> CvtFuncTypeAndRetType(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl,
                                              const clang::QualType &qualType) const;
  void CheckAtomicClearArg(const clang::CallExpr &expr) const;
  std::string GetFuncNameFromFuncDecl(const clang::FunctionDecl &funcDecl) const;
using FuncPtrBuiltinFunc = ASTExpr *(ASTParser::*)(MapleAllocator &allocator, const clang::CallExpr &expr,
                                                   std::stringstream &ss, ASTCallExpr &astCallExpr) const;
static std::map<std::string, FuncPtrBuiltinFunc> InitBuiltinFuncPtrMap();
ASTExpr *ProcessBuiltinFuncByName(MapleAllocator &allocator, const clang::CallExpr &expr, std::stringstream &ss,
                                  const std::string &name) const;
ASTExpr *ParseBuiltinFunc(MapleAllocator &allocator, const clang::CallExpr &expr, std::stringstream &ss,
                          ASTCallExpr &astCallExpr) const;
#define PARSE_BUILTIIN_FUNC(FUNC) ParseBuiltin##FUNC(MapleAllocator &allocator, const clang::CallExpr &expr, \
                                                     std::stringstream &ss, ASTCallExpr &astCallExpr) const
  ASTExpr *PARSE_BUILTIIN_FUNC(ClassifyType);
  ASTExpr *PARSE_BUILTIIN_FUNC(ConstantP);
  ASTExpr *PARSE_BUILTIIN_FUNC(Isinfsign);
  ASTExpr *PARSE_BUILTIIN_FUNC(HugeVal);
  ASTExpr *PARSE_BUILTIIN_FUNC(HugeValf);
  ASTExpr *PARSE_BUILTIIN_FUNC(HugeVall);
  ASTExpr *PARSE_BUILTIIN_FUNC(Inf);
  ASTExpr *PARSE_BUILTIIN_FUNC(Infl);
  ASTExpr *PARSE_BUILTIIN_FUNC(Inff);
  ASTExpr *PARSE_BUILTIIN_FUNC(Nan);
  ASTExpr *PARSE_BUILTIIN_FUNC(Nanl);
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
  ASTExpr *PARSE_BUILTIIN_FUNC(AtomicClear);
  ASTExpr *PARSE_BUILTIIN_FUNC(AtomicTestAndSet);

  static std::map<std::string, FuncPtrBuiltinFunc> builtingFuncPtrMap;
  uint32 fileIdx;
  const MapleString fileName;
  LibAstFile *astFile = nullptr;
  const AstUnitDecl *astUnitDecl = nullptr;
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
  MapleList<ASTEnumDecl*> &astEnums;
  MapleMap<clang::Expr*, ASTExpr*> vlaSizeMap;

  MapleUnorderedMap<MapleString, MapleVector<MapleString>, MapleString::MapleStringHash> structFileNameMap;
};
}  // namespace maple
#endif // HIR2MPL_AST_INPUT_INCLUDE_AST_PARSER_H
