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
#ifndef HIR2MPL_AST_FILE_INCLUDE_AST_INTERFACE_H
#define HIR2MPL_AST_FILE_INCLUDE_AST_INTERFACE_H
#include <string>
#include "ast_alias.h"
#include "mir_type.h"
#include "mir_nodes.h"
#include "mpl_logging.h"
#include "generic_attrs.h"
#include "fe_utils.h"
#include "clang/Basic/AttrKinds.h"

namespace maple {
enum AccessKind {
  kPublic,
  kProtected,
  kPrivate,
  kNone
};

class LibAstFile {
 public:
  explicit LibAstFile(MapleAllocator &allocatorIn, MapleList<clang::Decl*> &recordDeclesIn,
                      MapleList<clang::Decl*> &enumDeclesIn)
      : recordDeclSet(allocatorIn.Adapter()),
        unnamedSymbolMap(allocatorIn.Adapter()), compoundLiteralExprInitSymbolMap(allocatorIn.Adapter()),
        recordDecles(recordDeclesIn), enumDecles(enumDeclesIn), astFileName("", allocatorIn.GetMemPool()) {}
  ~LibAstFile() = default;

  bool Open(const MapleString &fileName,
            int excludeDeclFromPCH, int displayDiagnostics);
  void DisposeTranslationUnit();
  const AstASTContext *GetAstContext() const;
  AstASTContext *GetNonConstAstContext() const;
  const AstUnitDecl *GetAstUnitDecl() const;
  std::string GetMangledName(const clang::NamedDecl &decl) const;
  const std::string GetOrCreateMappedUnnamedName(const clang::Decl &decl);
  const std::string GetDeclName(const clang::NamedDecl &decl, bool isRename = false);
  void EmitTypeName(MapleAllocator &allocatorIn, const clang::QualType qualType, std::stringstream &ss);
  void EmitTypeName(const clang::RecordType &recordType, std::stringstream &ss);
  void EmitQualifierName(const clang::QualType qualType, std::stringstream &ss) const;
  std::string GetTypedefNameFromUnnamedStruct(const clang::RecordDecl &recoDecl) const;
  void BuildFieldName(std::stringstream &recordLayoutStr, const clang::FieldDecl &fieldDecl);
  std::string GetSourceText(const clang::Stmt &stmt) const;
  std::string GetSourceTextRaw(const clang::SourceRange range, const clang::SourceManager &sm) const;
  std::string BuildStaticFunctionSignature(const clang::FunctionDecl &funcDecl);
  void BuildStaticFunctionLayout(const clang::FunctionDecl &funcDecl, std::string &funcName);
  bool CheckAndBuildStaticFunctionLayout(const clang::FunctionDecl &funcDecl, std::stringstream &funcNameStream,
                                 std::unordered_set<int64_t> &visitedCalls);
  std::string GetRecordLayoutString(const clang::RecordDecl &recordDecl);
  void BuildFieldLayoutString(std::stringstream &recordLayoutStr, const clang::FieldDecl &fieldDecl);
  void CollectBaseEltTypeAndSizesFromConstArrayDecl(MapleAllocator &allocator, const clang::QualType &currQualType,
                                                    MIRType *&elemType,
                                                    TypeAttrs &elemAttr, std::vector<uint32_t> &operands,
                                                    bool isSourceType);
  void CollectBaseEltTypeAndDimFromVariaArrayDecl(MapleAllocator &allocator, const clang::QualType &currQualType,
                                                  MIRType *&elemType, TypeAttrs &elemAttr, uint8_t &dim,
                                                  bool isSourceType);
  void CollectBaseEltTypeAndDimFromDependentSizedArrayDecl(MapleAllocator &allocator,
                                                           const clang::QualType currQualType,
                                                           MIRType *&elemType, TypeAttrs &elemAttr,
                                                           std::vector<uint32_t> &operands, bool isSourceType);
  void CollectBaseEltTypeFromArrayDecl(MapleAllocator &allocator, const clang::QualType &currQualType,
                                       MIRType *&elemType, TypeAttrs &elemAttr, bool isSourceType = false);
  void GetCVRAttrs(uint32_t qualifiers, MapleGenericAttrs &genAttrs, bool isConst = true) const;
  void GetSClassAttrs(const clang::StorageClass storageClass, MapleGenericAttrs &genAttrs) const;
  void GetStorageAttrs(const clang::NamedDecl &decl, MapleGenericAttrs &genAttrs) const;
  void GetAccessAttrs(AccessKind access, MapleGenericAttrs &genAttrs) const;
  void GetQualAttrs(const clang::NamedDecl &decl, MapleGenericAttrs &genAttrs) const;
  void GetQualAttrs(const clang::QualType &qualType, MapleGenericAttrs &genAttrs, bool isSourceType) const;
  void CollectAttrs(const clang::NamedDecl &decl, MapleGenericAttrs &genAttrs, AccessKind access) const;
  void CollectFuncAttrs(const clang::FunctionDecl &decl, MapleGenericAttrs &genAttrs, AccessKind access) const;
  void CollectFuncReturnVarAttrs(const clang::CallExpr &expr, GenericAttrs &genAttrs) const;
  void SetAttrVisibility(const clang::DeclaratorDecl &decl, GenericAttrs &genAttrs) const;
  void SetAttrTLSModel(const clang::VarDecl &decl, GenericAttrs &genAttrs) const;
  void CheckUnsupportedFuncAttrs(const clang::FunctionDecl &decl) const;
  void CollectVarAttrs(const clang::VarDecl &decl, MapleGenericAttrs &genAttrs, AccessKind access) const;
  void CheckUnsupportedVarAttrs(const clang::VarDecl &decl) const;
  void CollectRecordAttrs(const clang::RecordDecl &decl, GenericAttrs &genAttrs) const;
  void CheckUnsupportedTypeAttrs(const clang::RecordDecl &decl) const;
  void CollectFieldAttrs(const clang::FieldDecl &decl, MapleGenericAttrs &genAttrs, AccessKind access) const;
  void CollectTypeAttrs(const clang::NamedDecl &decl, TypeAttrs &typeAttrs) const;
  MIRType *CvtPrimType(const clang::QualType qualType, bool isSourceType = false) const;
  PrimType CvtPrimType(const clang::BuiltinType::Kind kind, bool isSourceType) const;
  MIRType *CvtPrimType2SourceType(const clang::BuiltinType::Kind kind) const;
  MIRType *CvtSourceType(MapleAllocator &allocator, const clang::QualType qualType);
  MIRType *CvtType(MapleAllocator &allocator, const clang::QualType qualType, bool isSourceType = false,
                   const clang::Type **vlaType = nullptr);
  MIRType *CvtOtherType(MapleAllocator &allocator, const clang::QualType srcType, bool isSourceType,
                        const clang::Type **vlaType);
  MIRType *CvtArrayType(MapleAllocator &allocator, const clang::QualType &srcType, bool isSourceType,
                        const clang::Type **vlaType);
  MIRType *CvtFunctionType(MapleAllocator &allocator, const clang::QualType srcType, bool isSourceType);
  MIRType *CvtEnumType(MapleAllocator &allocator, const clang::QualType &qualType, bool isSourceType);
  MIRType *CvtRecordType(MapleAllocator &allocator, const clang::QualType qualType);
  MIRType *CvtFieldType(const clang::NamedDecl &decl);
  MIRType *CvtComplexType(const clang::QualType srcType) const;
  MIRType *CvtVectorType(MapleAllocator &allocator, const clang::QualType srcType);
  MIRType *CvtVectorSizeType(const MIRType &elemType, MIRType *destType, uint32_t arrLen, uint32_t vecLen,
                             uint32 alignNum) const;
  bool CheckSourceTypeNameNotNull(MapleAllocator &allocator, const clang::QualType &currQualType, MIRType *&elemType,
                                  bool isSourceType);
  MIRType *CvtTypedef(MapleAllocator &allocator, const clang::QualType &qualType);
  MIRType *CvtTypedefDecl(MapleAllocator &allocator, const clang::TypedefNameDecl &typedefDecl);
  bool TypeHasMayAlias(const clang::QualType srcType) const;
  static bool IsOneElementVector(const clang::QualType &qualType);
  static bool IsOneElementVector(const clang::Type &type);

  const clang::ASTContext *GetContext() const {
    return astContext;
  }

  const std::string GetAstFileNameHashStr() const {
    std::string fileName = (astFileName.c_str() == nullptr ? "" : astFileName.c_str());
    return FEUtils::GetFileNameHashStr(fileName);
  }

  Loc GetStmtLOC(const clang::Stmt &stmt) const;
  Loc GetExprLOC(const clang::Expr &expr) const;
  Loc GetLOC(const clang::SourceLocation &srcLoc) const;
  uint32 GetMaxAlign(const clang::Decl &decl) const;
  uint32 RetrieveAggTypeAlign(const clang::Type *ty) const;

 private:
  MapleSet<const clang::Decl *> recordDeclSet;
  MapleMap<int64, uint32> unnamedSymbolMap;
  MapleMap<uint32_t, std::string> compoundLiteralExprInitSymbolMap;
  MIRModule *module = nullptr;

  MapleList<clang::Decl *> &recordDecles;
  MapleList<clang::Decl *> &enumDecles;

  clang::ASTContext *astContext = nullptr;
  clang::TranslationUnitDecl *astUnitDecl = nullptr;
  clang::MangleContext *mangleContext = nullptr;
  CXTranslationUnit translationUnit = nullptr;
  CXIndex index = nullptr;
  MapleString astFileName;
  static std::map<Loc, uint32> unnamedLocMap;
};

class CallCollector : public clang::RecursiveASTVisitor<CallCollector> {
 public:
  explicit CallCollector(clang::ASTContext *myAstContext) : astContext(myAstContext) {}

  bool VisitCallExpr(clang::CallExpr *expr) {
    (void)callExprs.emplace(expr->getID(*astContext), expr);
    return true;
  }

  bool VisitDeclRefExpr(clang::DeclRefExpr *expr) {
    if (auto *varDecl = llvm::dyn_cast<clang::VarDecl>(expr->getDecl())) {
      if (!(varDecl->getType().isConstQualified()) && (varDecl->getStorageClass() == clang::SC_Static)) {
        needToBeUnique = true;
        return false;
      }
    }
    return true;
  }

  std::map<int64_t, clang::CallExpr *> GetCallExprs() {
    return callExprs;
  }

  bool IsNeedToBeUniq() const {
    return needToBeUnique;
  }

 private:
  clang::ASTContext *astContext;
  std::map<int64_t, clang::CallExpr *> callExprs;
  bool needToBeUnique = false;
};
}  // namespace maple
#endif  // HIR2MPL_AST_FILE_INCLUDE_AST_INTERFACE_H
