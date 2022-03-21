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
using Pos = std::pair<uint32, uint32>;
enum AccessKind {
  kPublic,
  kProtected,
  kPrivate,
  kNone
};
typedef std::unordered_map<clang::attr::Kind, std::string> AttributesMap;
class LibAstFile {
 public:
  explicit LibAstFile(MapleList<clang::Decl*> &recordDeclesIn) : recordDecles(recordDeclesIn) {}
  ~LibAstFile() = default;

  bool Open(const std::string &fileName,
            int excludeDeclFromPCH, int displayDiagnostics);
  const AstASTContext *GetAstContext() const;
  AstASTContext *GetNonConstAstContext() const;
  AstUnitDecl *GetAstUnitDecl();
  std::string GetMangledName(const clang::NamedDecl &decl);
  const std::string GetOrCreateMappedUnnamedName(uint32_t id);
  const std::string GetOrCreateCompoundLiteralExprInitName(uint32_t id);

  void EmitTypeName(const clang::QualType qualType, std::stringstream &ss);
  void EmitTypeName(const clang::RecordType &qualType, std::stringstream &ss);
  void EmitQualifierName(const clang::QualType qualType, std::stringstream &ss);
  std::string GetTypedefNameFromUnnamedStruct(const clang::RecordDecl &recoDecl);
  void CollectBaseEltTypeAndSizesFromConstArrayDecl(const clang::QualType &qualType, MIRType *&elemType,
                                                    TypeAttrs &elemAttr, std::vector<uint32_t> &operands);

  void CollectBaseEltTypeAndDimFromVariaArrayDecl(const clang::QualType &qualType, MIRType *&elemType,
                                                  TypeAttrs &elemAttr, uint8_t &dim);

  void CollectBaseEltTypeAndDimFromDependentSizedArrayDecl(const clang::QualType qualType, MIRType *&elemType,
                                                           TypeAttrs &elemAttr, std::vector<uint32_t> &operands);

  void GetCVRAttrs(uint32_t qualifiers, GenericAttrs &genAttrs);
  void GetSClassAttrs(const clang::StorageClass storageClass, GenericAttrs &genAttrs) const;
  void GetStorageAttrs(const clang::NamedDecl &decl, GenericAttrs &genAttrs) const;
  void GetAccessAttrs(AccessKind access, GenericAttrs &genAttrs);
  void GetQualAttrs(const clang::NamedDecl &decl, GenericAttrs &genAttrs);
  void CollectAttrs(const clang::NamedDecl &decl, GenericAttrs &genAttrs, AccessKind access);
  void CollectFuncAttrs(const clang::FunctionDecl &decl, GenericAttrs &genAttrs, AccessKind access);
  void CheckUnsupportedFuncAttrs(const clang::FunctionDecl &decl);
  void CollectVarAttrs(const clang::VarDecl &decl, GenericAttrs &genAttrs, AccessKind access);
  void CheckUnsupportedVarAttrs(const clang::VarDecl &decl);
  void CollectRecordAttrs(const clang::RecordDecl &decl, GenericAttrs &genAttrs, AccessKind access);
  void CheckUnsupportedTypeAttrs(const clang::RecordDecl &decl);
  void CollectFieldAttrs(const clang::FieldDecl &decl, GenericAttrs &genAttrs, AccessKind access);
  MIRType *CvtPrimType(const clang::QualType qualType) const;
  PrimType CvtPrimType(const clang::BuiltinType::Kind) const;
  MIRType *CvtType(const clang::QualType qualType);
  MIRType *CvtOtherType(const clang::QualType srcType);
  MIRType *CvtArrayType(const clang::QualType srcType);
  MIRType *CvtFunctionType(const clang::QualType srcType);
  MIRType *CvtRecordType(const clang::QualType srcType);
  MIRType *CvtFieldType(const clang::NamedDecl &decl);
  MIRType *CvtComplexType(const clang::QualType srcType);
  MIRType *CvtVectorType(const clang::QualType srcType);
  bool TypeHasMayAlias(const clang::QualType srcType);
  static bool IsOneElementVector(const clang::QualType &qualType);
  static bool IsOneElementVector(const clang::Type &type);

  const clang::ASTContext *GetContext() const {
    return astContext;
  }

  const std::string GetAstFileNameHashStr() const {
    return FEUtils::GetFileNameHashStr(astFileName);
  }

  Pos GetDeclPosInfo(const clang::Decl &decl) const;
  Pos GetStmtLOC(const clang::Stmt &stmt) const;
  Pos GetExprLOC(const clang::Expr &expr) const;
  Pos GetLOC(const clang::SourceLocation &srcLoc) const;
  uint32 GetMaxAlign(const clang::Decl &decl) const;
  uint32 RetrieveAggTypeAlign(const clang::Type *ty) const;

 private:
  using RecordDeclMap = std::map<TyIdx, const clang::RecordDecl*>;
  RecordDeclMap recordDeclMap;
  std::set<const clang::RecordDecl*> recordDeclSet;
  std::map<uint32_t, std::string> unnamedSymbolMap;
  std::map<uint32_t, std::string> CompoundLiteralExprInitSymbolMap;
  MIRModule *module = nullptr;

  MapleList<clang::Decl*> &recordDecles;

  clang::ASTContext *astContext = nullptr;
  clang::TranslationUnitDecl *astUnitDecl = nullptr;
  clang::MangleContext *mangleContext = nullptr;
  std::string astFileName;
  const AttributesMap unsupportedFuncAttrsMap = {
    {clang::attr::NoInstrumentFunction, "no_instrument_function"},
    {clang::attr::StdCall, "stdcall"},
    {clang::attr::CDecl, "cdecl"},
    {clang::attr::MipsLongCall, "mips_long_call"},
    {clang::attr::MipsShortCall, "mips_short_call"},
    {clang::attr::ARMInterrupt, "arm_interrupt"},
    {clang::attr::AnyX86Interrupt, "x86_interrupt"},
    {clang::attr::Naked, "naked"},
    {clang::attr::AllocAlign, "alloc_align"},
    {clang::attr::AssumeAligned, "assume_aligned"},
    {clang::attr::Flatten, "flatten"},
    {clang::attr::GNUInline, "gnu_inline"},
    {clang::attr::Cold, "cold"},
    {clang::attr::IFunc, "ifunc"},
    {clang::attr::NoSanitize, "no_sanitize"},
    {clang::attr::NoSplitStack, "no_split_stack"},
    {clang::attr::PatchableFunctionEntry, "patchable_function_entry"},
    {clang::attr::Target, "target"}
  };
  const AttributesMap unsupportedVarAttrsMap = {
    {clang::attr::Mode, "mode"},
    {clang::attr::NoCommon, "nocommon"},
    {clang::attr::TransparentUnion, "transparent_union"},
    {clang::attr::Alias, "alias"},
    {clang::attr::Cleanup, "cleanup"},
    {clang::attr::Common, "common"},
    {clang::attr::Uninitialized, "uninitialized"}
  };
  const AttributesMap unsupportedTypeAttrsMap = {
    {clang::attr::MSStruct, "ms_struct"}
  };
};
} // namespace maple
#endif // HIR2MPL_AST_FILE_INCLUDE_AST_INTERFACE_H
