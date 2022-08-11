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
  explicit LibAstFile(MapleAllocator &allocatorIn, MapleList<clang::Decl*> &recordDeclesIn)
      : recordDeclMap(allocatorIn.Adapter()), recordDeclSet(allocatorIn.Adapter()),
        unnamedSymbolMap(allocatorIn.Adapter()), compoundLiteralExprInitSymbolMap(allocatorIn.Adapter()),
        recordDecles(recordDeclesIn), astFileName("", allocatorIn.GetMemPool()) {}
  ~LibAstFile() = default;

  bool Open(const MapleString &fileName,
            int excludeDeclFromPCH, int displayDiagnostics);
  void DisposeTranslationUnit();
  const AstASTContext *GetAstContext() const;
  AstASTContext *GetNonConstAstContext() const;
  const AstUnitDecl *GetAstUnitDecl() const;
  std::string GetMangledName(const clang::NamedDecl &decl) const;
  const std::string GetOrCreateMappedUnnamedName(uint32_t id);

  void EmitTypeName(const clang::QualType qualType, std::stringstream &ss);
  void EmitTypeName(const clang::RecordType &recordType, std::stringstream &ss);
  void EmitQualifierName(const clang::QualType qualType, std::stringstream &ss) const;
  std::string GetTypedefNameFromUnnamedStruct(const clang::RecordDecl &recoDecl) const;
  void CollectBaseEltTypeAndSizesFromConstArrayDecl(const clang::QualType &currQualType, MIRType *&elemType,
                                                    TypeAttrs &elemAttr, std::vector<uint32_t> &operands,
                                                    bool isSourceType);
  void CollectBaseEltTypeAndDimFromVariaArrayDecl(const clang::QualType &currQualType, MIRType *&elemType,
                                                  TypeAttrs &elemAttr, uint8_t &dim, bool isSourceType);
  void CollectBaseEltTypeAndDimFromDependentSizedArrayDecl(const clang::QualType currQualType, MIRType *&elemType,
                                                           TypeAttrs &elemAttr, std::vector<uint32_t> &operands,
                                                           bool isSourceType);
  void CollectBaseEltTypeFromArrayDecl(const clang::QualType &currQualType, MIRType *&elemType, TypeAttrs &elemAttr,
                                       bool isSourceType = false);
  void GetCVRAttrs(uint32_t qualifiers, GenericAttrs &genAttrs, bool isConst = true) const;
  void GetSClassAttrs(const clang::StorageClass storageClass, GenericAttrs &genAttrs) const;
  void GetStorageAttrs(const clang::NamedDecl &decl, GenericAttrs &genAttrs) const;
  void GetAccessAttrs(AccessKind access, GenericAttrs &genAttrs) const;
  void GetQualAttrs(const clang::NamedDecl &decl, GenericAttrs &genAttrs) const;
  void GetQualAttrs(const clang::QualType &qualType, GenericAttrs &genAttrs, bool isSourceType) const;
  void CollectAttrs(const clang::NamedDecl &decl, GenericAttrs &genAttrs, AccessKind access) const;
  void CollectFuncAttrs(const clang::FunctionDecl &decl, GenericAttrs &genAttrs, AccessKind access) const;
  void CollectFuncReturnVarAttrs(const clang::CallExpr &expr, GenericAttrs &genAttrs) const;
  void CheckUnsupportedFuncAttrs(const clang::FunctionDecl &decl) const;
  void CollectVarAttrs(const clang::VarDecl &decl, GenericAttrs &genAttrs, AccessKind access) const;
  void CheckUnsupportedVarAttrs(const clang::VarDecl &decl) const;
  void CollectRecordAttrs(const clang::RecordDecl &decl, GenericAttrs &genAttrs) const;
  void CheckUnsupportedTypeAttrs(const clang::RecordDecl &decl) const;
  void CollectFieldAttrs(const clang::FieldDecl &decl, GenericAttrs &genAttrs, AccessKind access) const;
  MIRType *CvtPrimType(const clang::QualType qualType) const;
  PrimType CvtPrimType(const clang::BuiltinType::Kind kind) const;
  MIRType *CvtSourceType(const clang::QualType qualType);
  MIRType *CvtType(const clang::QualType qualType, bool isSourceType = false);
  MIRType *CvtOtherType(const clang::QualType srcType, bool isSourceType);
  MIRType *CvtArrayType(const clang::QualType &srcType, bool isSourceType);
  MIRType *CvtFunctionType(const clang::QualType srcType, bool isSourceType);
  MIRType *CvtEnumType(const clang::QualType &qualType, bool isSourceType);
  MIRType *CvtRecordType(const clang::QualType qualType);
  MIRType *CvtFieldType(const clang::NamedDecl &decl);
  MIRType *CvtComplexType(const clang::QualType srcType) const;
  MIRType *CvtVectorType(const clang::QualType srcType);
  MIRType *CvtTypedef(const clang::QualType &qualType);
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
  using RecordDeclMap = MapleMap<TyIdx, const clang::RecordDecl*>;
  RecordDeclMap recordDeclMap;
  MapleSet<const clang::RecordDecl*> recordDeclSet;
  MapleMap<uint32_t, std::string> unnamedSymbolMap;
  MapleMap<uint32_t, std::string> compoundLiteralExprInitSymbolMap;
  MIRModule *module = nullptr;

  MapleList<clang::Decl*> &recordDecles;

  clang::ASTContext *astContext = nullptr;
  clang::TranslationUnitDecl *astUnitDecl = nullptr;
  clang::MangleContext *mangleContext = nullptr;
  CXTranslationUnit translationUnit = nullptr;
  CXIndex index = nullptr;
  MapleString astFileName;
};
} // namespace maple
#endif // HIR2MPL_AST_FILE_INCLUDE_AST_INTERFACE_H
