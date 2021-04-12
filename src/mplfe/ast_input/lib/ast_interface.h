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
#ifndef MPLFE_AST_FILE_INCLUDE_AST_INTERFACE_H
#define MPLFE_AST_FILE_INCLUDE_AST_INTERFACE_H
#include <string>
#include "ast_alias.h"

#include "mir_type.h"
#include "mir_nodes.h"
#include "mpl_logging.h"

namespace maple {
enum AccessKind {
  kPublic,
  kProtected,
  kPrivate,
  kNone
};

class LibAstFile {
 public:
  LibAstFile() = default;
  ~LibAstFile() = default;

  bool Open(const std::string &fileName,
            int excludeDeclFromPCH, int displayDiagnostics);
  const AstASTContext *GetAstContext();
  AstUnitDecl *GetAstUnitDecl();
  std::string GetMangledName(const clang::NamedDecl &decl);
  const std::string GetOrCreateMappedUnnamedName(uint32_t id);

  void EmitTypeName(const clang::QualType qualType, std::stringstream &ss);
  void EmitTypeName(const clang::RecordType &qualType, std::stringstream &ss);
  void EmitQualifierName(const clang::QualType qualType, std::stringstream &ss);

  void CollectBaseEltTypeAndSizesFromConstArrayDecl(const clang::QualType &qualType, MIRType *&elemType,
                                                    std::vector<uint32_t> &operands);

  void CollectBaseEltTypeAndDimFromVariaArrayDecl(const clang::QualType &qualType, MIRType *&elemType, uint8_t &dim);

  void CollectBaseEltTypeAndDimFromDependentSizedArrayDecl(const clang::QualType qualType, MIRType *&elemType,
                                                           std::vector<uint32_t> &operands);

  void GetCVRAttrs(uint32_t qualifiers, GenericAttrs &genAttrs);
  void GetSClassAttrs(const clang::StorageClass storageClass, GenericAttrs &genAttrs) const;
  void GetStorageAttrs(const clang::NamedDecl &decl, GenericAttrs &genAttrs) const;
  void GetAccessAttrs(AccessKind access, GenericAttrs &genAttrs);
  void GetQualAttrs(const clang::NamedDecl &decl, GenericAttrs &genAttrs);
  void CollectAttrs(const clang::NamedDecl &decl, GenericAttrs &genAttrs, AccessKind access);
  void CollectFuncAttrs(const clang::FunctionDecl &decl, GenericAttrs &genAttrs, AccessKind access);
  MIRType *CvtPrimType(const clang::QualType qualType) const;
  PrimType CvtPrimType(const clang::BuiltinType::Kind) const;
  MIRType *CvtType(const clang::QualType qualType);
  MIRType *CvtOtherType(const clang::QualType srcType);
  MIRType *CvtArrayType(const clang::QualType srcType);
  MIRType *CvtFunctionType(const clang::QualType srcType);
  MIRType *CvtRecordType(const clang::QualType srcType);
  MIRType *CvtFieldType(const clang::NamedDecl &decl);

  const clang::ASTContext *GetContext() {
    return astContext;
  }

 private:
  using RecordDeclMap = std::map<TyIdx, const clang::RecordDecl*>;
  RecordDeclMap recordDeclMap;
  std::map<uint32_t, std::string> unnamedSymbolMap;
  MIRModule *module;

  clang::ASTContext *astContext = nullptr;
  clang::TranslationUnitDecl *astUnitDecl = nullptr;
  clang::MangleContext *mangleContext = nullptr;
};
} // namespace maple
#endif // MPLFE_AST_FILE_INCLUDE_AST_INTERFACE_H
