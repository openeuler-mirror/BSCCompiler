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
#include "ast_interface.h"
#include <string>
#include "mpl_logging.h"
#include "ast_util.h"
#include "fe_utils.h"
#include "fe_manager.h"

namespace maple {
const std::unordered_map<clang::attr::Kind, std::string> kUnsupportedFuncAttrsMap = {
    {clang::attr::NoInstrumentFunction, "no_instrument_function"},
    {clang::attr::StdCall, "stdcall"},
    {clang::attr::CDecl, "cdecl"},
    {clang::attr::MipsLongCall, "mips_long_call"},
    {clang::attr::MipsShortCall, "mips_short_call"},
    {clang::attr::ARMInterrupt, "arm_interrupt"},
    {clang::attr::AnyX86Interrupt, "x86_interrupt"},
    {clang::attr::Naked, "naked"},
    {clang::attr::AssumeAligned, "assume_aligned"},
    {clang::attr::Flatten, "flatten"},
    {clang::attr::Cold, "cold"},
    {clang::attr::IFunc, "ifunc"},
    {clang::attr::NoSanitize, "no_sanitize"},
    {clang::attr::NoSplitStack, "no_split_stack"},
    {clang::attr::PatchableFunctionEntry, "patchable_function_entry"},
    {clang::attr::Target, "target"},
    {clang::attr::Alias, "alias"}};
const std::unordered_map<clang::attr::Kind, std::string> kUnsupportedVarAttrsMap = {
    {clang::attr::Mode, "mode"},
    {clang::attr::NoCommon, "nocommon"},
    {clang::attr::TransparentUnion, "transparent_union"},
    {clang::attr::Alias, "alias"},
    {clang::attr::Cleanup, "cleanup"},
    {clang::attr::Common, "common"},
    {clang::attr::Uninitialized, "uninitialized"}};
const std::unordered_map<clang::attr::Kind, std::string> kUnsupportedTypeAttrsMap = {
    {clang::attr::MSStruct, "ms_struct"}};

bool LibAstFile::Open(const MapleString &fileName,
                      int excludeDeclFromPCH, int displayDiagnostics) {
  astFileName = fileName;
  index = clang_createIndex(excludeDeclFromPCH, displayDiagnostics);
  translationUnit = clang_createTranslationUnit(index, fileName.c_str());
  CHECK_FATAL(translationUnit != nullptr, "The astfile %s content format is Non-conformance or astfile"
              " version is different from hir2mpl version.", fileName.c_str());
  clang::ASTUnit *astUnit = translationUnit->TheASTUnit;
  if (astUnit == nullptr) {
    return false;
  }
  astContext = &astUnit->getASTContext();
  if (astContext == nullptr) {
    return false;
  }
  astUnitDecl = astContext->getTranslationUnitDecl();
  if (astUnitDecl == nullptr) {
    return false;
  }
  mangleContext = astContext->createMangleContext();
  if (mangleContext == nullptr) {
    return false;
  }
  return true;
}

void LibAstFile::DisposeTranslationUnit() {
    clang_disposeIndex(index);
    clang_disposeTranslationUnit(translationUnit);
    delete mangleContext;
    mangleContext = nullptr;
    translationUnit = nullptr;
    index = nullptr;
}

const AstASTContext *LibAstFile::GetAstContext() const {
  return astContext;
}

AstASTContext *LibAstFile::GetNonConstAstContext() const {
  return astContext;
}

const AstUnitDecl *LibAstFile::GetAstUnitDecl() const {
  return astUnitDecl;
}

std::string LibAstFile::GetMangledName(const clang::NamedDecl &decl) const {
  std::string mangledName;
  if (!mangleContext->shouldMangleDeclName(&decl)) {
    mangledName = decl.getNameAsString();
  } else {
    llvm::raw_string_ostream ostream(mangledName);
    mangleContext->mangleName(&decl, ostream);
    ostream.flush();
  }
  return mangledName;
}

Loc LibAstFile::GetStmtLOC(const clang::Stmt &stmt) const {
  return GetLOC(stmt.getBeginLoc());
}

Loc LibAstFile::GetExprLOC(const clang::Expr &expr) const {
  return GetLOC(expr.getExprLoc());
}

Loc LibAstFile::GetLOC(const clang::SourceLocation &srcLoc) const {
  clang::PresumedLoc pLoc = astContext->getSourceManager().getPresumedLoc(srcLoc);
  if (pLoc.isInvalid()) {
    return {0, 0, 0};
  }
  if (srcLoc.isFileID()) {
    std::string fileName = pLoc.getFilename();
    if (fileName.empty()) {
      return {0, 0, 0};
    }
    unsigned line = pLoc.getLine();
    unsigned colunm = pLoc.getColumn();
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(fileName);
    for (const auto &info : FEManager::GetModule().GetSrcFileInfo()) {
      if (info.first == strIdx) {
        return {info.second, static_cast<uint32>(line), static_cast<uint32>(colunm)};
      }
    }
    if (FEManager::GetModule().GetSrcFileInfo().empty()) {
      // src files start from 2, 1 is mpl file
      FEManager::GetModule().PushbackFileInfo(MIRInfoPair(strIdx, 2));
      return {2, static_cast<uint32>(line), static_cast<uint32>(colunm)};
    } else {
      auto last = FEManager::GetModule().GetSrcFileInfo().rbegin();
      FEManager::GetModule().PushbackFileInfo(MIRInfoPair(strIdx, last->second + 1));
      return {last->second + 1, static_cast<uint32>(line), static_cast<uint32>(colunm)};
    }
  } else {  // For macro line: The expansion location is the line in the source code where the macro was expanded
    return GetLOC(astContext->getSourceManager().getExpansionLoc(srcLoc));
  }
}

uint32 LibAstFile::GetMaxAlign(const clang::Decl &decl) const {
  uint32 align = 0;
  const clang::Decl *canonicalDecl = decl.getCanonicalDecl();
  if (canonicalDecl->getKind() == clang::Decl::Field) {
    const clang::FieldDecl *fieldDecl = llvm::cast<clang::FieldDecl>(canonicalDecl);
    clang::QualType qualTy = fieldDecl->getType().getCanonicalType();
    align = RetrieveAggTypeAlign(qualTy.getTypePtr());
  }
  uint32 selfAlign = canonicalDecl->getMaxAlignment();
  return align > selfAlign ? align : selfAlign;
}

uint32 LibAstFile::RetrieveAggTypeAlign(const clang::Type *ty) const {
  ASSERT_NOT_NULL(ty);
  if (ty->isRecordType()) {
    const auto *recordType = llvm::cast<clang::RecordType>(ty);
    clang::RecordDecl *recordDecl = recordType->getDecl();
    return (recordDecl->getMaxAlignment()) >> 3;  // 8 bit = 2^3 bit = 1 byte
  } else if (ty->isArrayType()) {
    const clang::Type *elemType = ty->getArrayElementTypeNoTypeQual();
    return RetrieveAggTypeAlign(elemType);
  }
  return 0;
}

void LibAstFile::GetCVRAttrs(uint32_t qualifiers, MapleGenericAttrs &genAttrs, bool isConst) const {
  if (isConst && (qualifiers & clang::Qualifiers::Const) != 0) {
    genAttrs.SetAttr(GENATTR_const);
  }
  if ((qualifiers & clang::Qualifiers::Restrict) != 0) {
    genAttrs.SetAttr(GENATTR_restrict);
  }
  if ((qualifiers & clang::Qualifiers::Volatile) != 0) {
    genAttrs.SetAttr(GENATTR_volatile);
  }
}

void LibAstFile::GetSClassAttrs(const clang::StorageClass storageClass, MapleGenericAttrs &genAttrs) const {
  switch (storageClass) {
    case clang::SC_Extern:
    case clang::SC_PrivateExtern:
      genAttrs.SetAttr(GENATTR_extern);
      break;
    case clang::SC_Static:
      genAttrs.SetAttr(GENATTR_static);
      break;
    default:
      break;
  }
}

void LibAstFile::GetStorageAttrs(const clang::NamedDecl &decl, MapleGenericAttrs &genAttrs) const {
  switch (decl.getKind()) {
    case clang::Decl::Function:
    case clang::Decl::CXXMethod: {
      const auto *funcDecl = llvm::cast<clang::FunctionDecl>(&decl);
      const clang::StorageClass storageClass = funcDecl->getStorageClass();
      GetSClassAttrs(storageClass, genAttrs);
      // static or extern maybe missing in current FunctionDecls,
      // Since a given function can be declared several times in a program,
      // Only one of those FunctionDecls will be found when traversing the list of declarations in the context.
      const clang::FunctionDecl *prev = funcDecl->getPreviousDecl();
      while (prev != nullptr && prev->isDefined()) {
        GetStorageAttrs(*prev, genAttrs);
        prev = prev->getPreviousDecl();
      }
      break;
    }
    case clang::Decl::ParmVar:
    case clang::Decl::Var: {
      const auto *varDecl = llvm::cast<clang::VarDecl>(&decl);
      const clang::StorageClass storageClass = varDecl->getStorageClass();
      if (storageClass == clang::SC_Extern) {
        varDecl = varDecl->getPreviousDecl();
        while (varDecl != nullptr) {
          auto preClass = varDecl->getStorageClass();
          if (preClass == clang::SC_Static) {
            GetSClassAttrs(preClass, genAttrs);
            break;
          }
          varDecl = varDecl->getPreviousDecl();
        }
      }
      GetSClassAttrs(storageClass, genAttrs);
      break;
    }
    case clang::Decl::Field:
    default:
      break;
  }
  return;
}

void LibAstFile::GetAccessAttrs(AccessKind access, MapleGenericAttrs &genAttrs) const {
  switch (access) {
    case kPublic:
      genAttrs.SetAttr(GENATTR_public);
      break;
    case kProtected:
      genAttrs.SetAttr(GENATTR_protected);
      break;
    case kPrivate:
      genAttrs.SetAttr(GENATTR_private);
      break;
    case kNone:
      break;
    default:
      ASSERT(false, "shouldn't reach here");
      break;
  }
  return;
}

void LibAstFile::GetQualAttrs(const clang::NamedDecl &decl, MapleGenericAttrs &genAttrs) const {
  switch (decl.getKind()) {
    case clang::Decl::Function:
    case clang::Decl::CXXMethod:
    case clang::Decl::ParmVar:
    case clang::Decl::Var:
    case clang::Decl::Field: {
      const auto *valueDecl = llvm::dyn_cast<clang::ValueDecl>(&decl);
      ASSERT(valueDecl != nullptr, "ERROR:null pointer!");
      const clang::QualType qualType = valueDecl->getType();
      uint32_t qualifiers = qualType.getCVRQualifiers();
      GetCVRAttrs(qualifiers, genAttrs);
      break;
    }
    default:
      break;
  }
}

void LibAstFile::GetQualAttrs(const clang::QualType &qualType, MapleGenericAttrs &genAttrs, bool isSourceType) const {
  uint32_t qualifiers = qualType.getCVRQualifiers();
  GetCVRAttrs(qualifiers, genAttrs, isSourceType);
}

void LibAstFile::CollectAttrs(const clang::NamedDecl &decl, MapleGenericAttrs &genAttrs, AccessKind access) const {
  GetStorageAttrs(decl, genAttrs);
  GetAccessAttrs(access, genAttrs);
  GetQualAttrs(decl, genAttrs);
  if (decl.isImplicit()) {
    genAttrs.SetAttr(GENATTR_implicit);
  }
  if (decl.isUsed()) {
    genAttrs.SetAttr(GENATTR_used);
  }
  if (decl.hasAttr<clang::WeakAttr>()) {
    genAttrs.SetAttr(GENATTR_weak);
  }
  if (decl.hasAttr<clang::NonNullAttr>() && decl.getKind() != clang::Decl::Function) {
    for (const auto *nonNull : decl.specific_attrs<clang::NonNullAttr>()) {
      if (nonNull->args_size() > 0) {
        // nonnull with args in function type pointers need special handling to mark nonnull arg
        continue;
      }
      genAttrs.SetAttr(GENATTR_nonnull);
    }
  }
}

void LibAstFile::CollectFuncReturnVarAttrs(const clang::CallExpr &expr, GenericAttrs &genAttrs) const {
  if (LibAstFile::IsOneElementVector(expr.getCallReturnType(*astContext))) {
    genAttrs.SetAttr(GenericAttrKind::GENATTR_oneelem_simd);
  }
}

void LibAstFile::SetAttrVisibility(const clang::DeclaratorDecl &decl, GenericAttrs &genAttrs) const {
  if (decl.getLinkageAndVisibility().isVisibilityExplicit()) {
    auto visibilityInfo = decl.getLinkageAndVisibility().getVisibility();
    switch (visibilityInfo) {
      case clang::Visibility::HiddenVisibility:
        genAttrs.SetAttr(GENATTR_visibility_hidden);
        break;
      case clang::Visibility::ProtectedVisibility:
        genAttrs.SetAttr(GENATTR_visibility_protected);
        break;
      default:
        break;
    }
  }
}

void LibAstFile::SetAttrTLSModel(const clang::VarDecl &decl, GenericAttrs &genAttrs) const {
  if (decl.hasAttr<clang::TLSModelAttr>()) {
    const clang::TLSModelAttr *tlsAttr = decl.getAttr<clang::TLSModelAttr>();
    const std::string tlsModelName = tlsAttr->getModel().str();
    if (tlsModelName == "local-exec") {
      genAttrs.SetAttr(GENATTR_local_exec);
    } else if (tlsModelName == "local-dynamic") {
      genAttrs.SetAttr(GENATTR_local_dynamic);
    } else if (tlsModelName == "initial-exec") {
      genAttrs.SetAttr(GENATTR_initial_exec);
    } else if (tlsModelName == "global-dynamic") {
      genAttrs.SetAttr(GENATTR_global_dynamic);
    } else {
      CHECK_FATAL(false, "TLSMODE: %s is not support", tlsModelName.c_str());
    }
  }
}

void LibAstFile::CollectFuncAttrs(const clang::FunctionDecl &decl, MapleGenericAttrs &genAttrs, AccessKind access)
    const {
  CollectAttrs(decl, genAttrs, access);
  if (decl.isVirtualAsWritten()) {
    genAttrs.SetAttr(GENATTR_virtual);
  }
  if (decl.isDeletedAsWritten()) {
    genAttrs.SetAttr(GENATTR_delete);
  }
  if (decl.isPure()) {
    genAttrs.SetAttr(GENATTR_pure);
  }
  if (decl.isInlineSpecified()) {
    genAttrs.SetAttr(GENATTR_inline);
  } else if (decl.hasAttr<clang::NoInlineAttr>()) {
    genAttrs.SetAttr(GENATTR_noinline);
  }
  if (decl.hasAttr<clang::AlwaysInlineAttr>()) {
    genAttrs.SetAttr(GENATTR_always_inline);
  }
  if (decl.hasAttr<clang::GNUInlineAttr>()) {
    genAttrs.SetAttr(GENATTR_gnu_inline);
  }
  if (decl.hasAttr<clang::HotAttr>()) {
    genAttrs.SetAttr(GENATTR_hot);
  }
  if (decl.hasAttr<clang::ColdAttr>()) {
    genAttrs.SetAttr(GENATTR_cold);
  }
  if (decl.isDefaulted()) {
    genAttrs.SetAttr(GENATTR_default);
  }
  if (decl.getKind() == clang::Decl::CXXConstructor) {
    genAttrs.SetAttr(GENATTR_constructor);
  }
  if (decl.getKind() == clang::Decl::CXXDestructor) {
    genAttrs.SetAttr(GENATTR_destructor);
  }
  if (decl.isVariadic()) {
    genAttrs.SetAttr(GENATTR_varargs);
  }
  if (decl.isNoReturn()) {
    genAttrs.SetAttr(GENATTR_noreturn);
  }
  clang::AliasAttr *aliasAttr = decl.getAttr<clang::AliasAttr>();
  if (aliasAttr != nullptr) {
    genAttrs.SetAttr(GENATTR_alias);
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(aliasAttr->getAliasee().str());
    genAttrs.InsertStrIdxContentMap(GENATTR_alias, strIdx);
  }
  clang::ConstructorAttr *constructorAttr = decl.getAttr<clang::ConstructorAttr>();
  if (constructorAttr != nullptr) {
    genAttrs.SetAttr(GENATTR_constructor_priority);
    genAttrs.InsertIntContentMap(GENATTR_constructor_priority, constructorAttr->getPriority());
  }
  clang::DestructorAttr *destructorAttr = decl.getAttr<clang::DestructorAttr>();
  if (destructorAttr != nullptr) {
    genAttrs.SetAttr(GENATTR_destructor_priority);
    genAttrs.InsertIntContentMap(GENATTR_destructor_priority, destructorAttr->getPriority());
  }
  // one element vector type in rettype
  if (LibAstFile::IsOneElementVector(decl.getReturnType())) {
    genAttrs.SetAttr(GENATTR_oneelem_simd);
  }
  if (FEOptions::GetInstance().IsEnableSafeRegion()) {
    if (decl.getSafeSpecifier() == clang::SS_Unsafe) {
      genAttrs.SetAttr(GENATTR_unsafed);
    } else if (decl.getSafeSpecifier() == clang::SS_Safe || FEOptions::GetInstance().IsDefaultSafe()) {
      genAttrs.SetAttr(GENATTR_safed);
    }
  }
  // If a non-static function defined with inline has the following 2 kinds function declaration, it should be
  // an externally visible function:
  // (1) non-inline func decl
  // (2) inline explicit extern func decl
  if (decl.isThisDeclarationADefinition() && genAttrs.GetAttr(GENATTR_inline) &&
      !genAttrs.GetAttr(GENATTR_gnu_inline) && !genAttrs.GetAttr(GENATTR_static)) {
    bool isExternallyVisible = false;
    for (clang::FunctionDecl *funcDecl : decl.redecls()) {
      // skip func definition and block scope func decl
      if (!funcDecl->isThisDeclarationADefinition() && !funcDecl->isLocalExternDecl()) {
        const bool declInline = funcDecl->isInlineSpecified();
        const bool declExtern = funcDecl->getStorageClass() == clang::SC_Extern;
        if (!declInline || (declInline && declExtern)) {
          isExternallyVisible = true;
          break;
        }
      }
    }
    if (isExternallyVisible) {
      genAttrs.SetAttr(GENATTR_extern);
    }
  }
  // If a function is defined with attrinute 'gnu_inline' but without 'extern', the 'extern' from function declarations
  // should be ignored.
  if (decl.isThisDeclarationADefinition() && genAttrs.GetAttr(GENATTR_gnu_inline)) {
    auto sc = decl.getStorageClass();
    if (sc == clang::SC_Extern || sc == clang::SC_PrivateExtern) {
      genAttrs.SetAttr(GENATTR_extern);
    } else {
      genAttrs.ResetAttr(GENATTR_extern);
    }
  }
  SetAttrVisibility(decl, genAttrs);
  VisitAllOfFuncAttrs(decl, genAttrs);
}

void LibAstFile::VisitAllOfFuncAttrs(const clang::FunctionDecl &decl, GenericAttrs &genAttrs) const {
  if (!decl.hasAttrs()) {
    return;
  }
  std::string unsupportedFuncAttrs = "";
  const clang::AttrVec &funcAttrs = decl.getAttrs();
  for (const auto *attr : funcAttrs) {
    clang::attr::Kind attrKind = attr->getKind();
    if (attrKind == clang::attr::FunctionLikeMacro) {
      genAttrs.SetAttr(GENATTR_like_macro);
    }
    auto iterator = kUnsupportedFuncAttrsMap.find(attrKind);
    if (iterator != kUnsupportedFuncAttrsMap.end()) {
      unsupportedFuncAttrs += iterator->second + " ";
    }
  }
  CHECK_FATAL(unsupportedFuncAttrs.empty(), "%s:%d error: The function %s has unsupported attribute(s): %s",
              FEManager::GetModule().GetFileNameFromFileNum(GetLOC(decl.getLocation()).fileIdx).c_str(),
              GetLOC(decl.getLocation()).line,
              GetMangledName(decl).c_str(),
              unsupportedFuncAttrs.c_str());
}

void LibAstFile::CollectVarAttrs(const clang::VarDecl &decl, MapleGenericAttrs &genAttrs, AccessKind access) const {
  CollectAttrs(decl, genAttrs, access);
  SetAttrVisibility(decl, genAttrs);
  SetAttrTLSModel(decl, genAttrs);
  // handle __thread
  if (decl.getTLSKind() == clang::VarDecl::TLS_Static) {
    genAttrs.SetAttr(GENATTR_tls_static);
  } else if (decl.getTLSKind() == clang::VarDecl::TLS_Dynamic) {
    genAttrs.SetAttr(GENATTR_tls_dynamic);
  }
  // one elem vector type
  if (IsOneElementVector(decl.getType())) {
    genAttrs.SetAttr(GENATTR_oneelem_simd);
  }
  CheckUnsupportedVarAttrs(decl);
}

void LibAstFile::CheckUnsupportedVarAttrs(const clang::VarDecl &decl) const {
  if (!decl.hasAttrs()) {
    return;
  }
  std::string unsupportedVarAttrs = "";
  const clang::AttrVec &varAttrs = decl.getAttrs();
  for (const auto *attr : varAttrs) {
    clang::attr::Kind attrKind = attr->getKind();
    auto iterator = kUnsupportedVarAttrsMap.find(attrKind);
    if (iterator != kUnsupportedVarAttrsMap.end()) {
      unsupportedVarAttrs += iterator->second + " ";
    }
  }
  CHECK_FATAL(unsupportedVarAttrs.empty(), "%s:%d error: The variable %s has unsupported attribute(s): %s",
              FEManager::GetModule().GetFileNameFromFileNum(GetLOC(decl.getLocation()).fileIdx).c_str(),
              GetLOC(decl.getLocation()).line,
              GetMangledName(decl).c_str(),
              unsupportedVarAttrs.c_str());
}

void LibAstFile::CollectTypeAttrs(const clang::NamedDecl &decl, TypeAttrs &typeAttrs) const {
  const clang::AttrVec &typeAttrVec = decl.getAttrs();
  for (const auto *attr : typeAttrVec) {
    clang::attr::Kind attrKind = attr->getKind();
    if (attrKind == clang::attr::Aligned) {
      typeAttrs.SetAttr(ATTR_aligned);
    }
  }
}

void LibAstFile::CollectRecordAttrs(const clang::RecordDecl &decl, GenericAttrs &genAttrs) const {
  clang::PackedAttr *packedAttr = decl.getAttr<clang::PackedAttr>();
  if (packedAttr != nullptr) {
    genAttrs.SetAttr(GENATTR_packed);
  }
  clang::MaxFieldAlignmentAttr *maxFieldAlignAttr = decl.getAttr<clang::MaxFieldAlignmentAttr>();
  if (maxFieldAlignAttr != nullptr) {
    genAttrs.SetAttr(GENATTR_pack);
    int value = static_cast<int>(maxFieldAlignAttr->getAlignment() / 8); // bits to byte
    genAttrs.InsertIntContentMap(GENATTR_pack, value);
  }
  CheckUnsupportedTypeAttrs(decl);
}

void LibAstFile::CheckUnsupportedTypeAttrs(const clang::RecordDecl &decl) const {
  if (!decl.hasAttrs()) {
    return;
  }
  std::string unsupportedTypeAttrs = "";
  const clang::AttrVec &typeAttrs = decl.getAttrs();
  for (const auto *attr : typeAttrs) {
    clang::attr::Kind attrKind = attr->getKind();
    auto iterator = kUnsupportedTypeAttrsMap.find(attrKind);
    if (iterator != kUnsupportedTypeAttrsMap.end()) {
      unsupportedTypeAttrs += iterator->second + " ";
    }
  }
  CHECK_FATAL(unsupportedTypeAttrs.empty(), "%s:%d error: struct or union %s has unsupported type attribute(s): %s",
              FEManager::GetModule().GetFileNameFromFileNum(GetLOC(decl.getLocation()).fileIdx).c_str(),
              GetLOC(decl.getLocation()).line,
              GetMangledName(decl).c_str(),
              unsupportedTypeAttrs.c_str());
}

void LibAstFile::CollectFieldAttrs(const clang::FieldDecl &decl, MapleGenericAttrs &genAttrs, AccessKind access) const {
  CollectAttrs(decl, genAttrs, access);
  clang::PackedAttr *packedAttr = decl.getAttr<clang::PackedAttr>();
  if (packedAttr != nullptr) {
    genAttrs.SetAttr(GENATTR_packed);
  }
  clang::AlignedAttr *alignedAttr = decl.getAttr<clang::AlignedAttr>();
  if (alignedAttr != nullptr) {
    genAttrs.SetAttr(GENATTR_aligned);
  }
}

void LibAstFile::EmitTypeName(MapleAllocator &allocatorIn, const clang::QualType qualType, std::stringstream &ss) {
  switch (qualType->getTypeClass()) {
    case clang::Type::LValueReference: {
      ss << "R";
      const clang::QualType pointeeType = qualType->castAs<clang::ReferenceType>()->getPointeeType();
      EmitTypeName(allocatorIn, pointeeType, ss);
      break;
    }
    case clang::Type::Pointer: {
      ss << "P";
      const clang::QualType pointeeType = qualType->castAs<clang::PointerType>()->getPointeeType();
      EmitTypeName(allocatorIn, pointeeType, ss);
      break;
    }
    case clang::Type::Record: {
      EmitTypeName(*qualType->getAs<clang::RecordType>(), ss);
      break;
    }
    default: {
      EmitQualifierName(qualType, ss);
      MIRType *type = CvtType(allocatorIn, qualType);
      ss << ASTUtil::GetTypeString(*type);
      break;
    }
  }
}

void LibAstFile::EmitQualifierName(const clang::QualType qualType, std::stringstream &ss) const {
  uint32_t cvrQual = qualType.getCVRQualifiers();
  if ((cvrQual & clang::Qualifiers::Const) != 0) {
    ss << "K";
  }
  if ((cvrQual & clang::Qualifiers::Volatile) != 0) {
    ss << "U";
  }
}

void LibAstFile::BuildFieldName(std::stringstream &recordLayoutStr, const clang::FieldDecl &fieldDecl) {
  std::string canonicalTypeName = fieldDecl.getType().getCanonicalType().getAsString();
  std::string fieldName = GetMangledName(fieldDecl);
  if (fieldName.empty() && fieldDecl.isAnonymousStructOrUnion()) {
    fieldName = GetOrCreateMappedUnnamedName(*(fieldDecl.getType()->getAsRecordDecl()));
  }
  FEUtils::EraseFileNameforClangTypeStr(canonicalTypeName);
  recordLayoutStr << fieldName << "-" << canonicalTypeName;
  const auto *fieldRDcl = fieldDecl.getType()->getAsRecordDecl();
  if (fieldRDcl != nullptr) {
    recordLayoutStr << ":{";
    for (const auto field : fieldRDcl->fields()) {
      BuildFieldName(recordLayoutStr, *field);
    }
    recordLayoutStr << "}";
  }
  recordLayoutStr << ",";
}

const std::string LibAstFile::GetOrCreateMappedUnnamedName(const clang::Decl &decl) {
  if (FEOptions::GetInstance().GetWPAA()) {
    std::stringstream recordLayoutStr;
    const clang::RecordDecl *myRecordDecl = nullptr;
    if (const auto *fieldDecl = llvm::dyn_cast<clang::FieldDecl>(&decl)) {
      myRecordDecl = fieldDecl->getType()->getAsRecordDecl();
    } else if (const auto *recordDecl = llvm::dyn_cast<clang::RecordDecl>(&decl)) {
      myRecordDecl = recordDecl;
    }
    if (myRecordDecl) {
      recordLayoutStr << myRecordDecl->getKindName().str() << ":{";
      for (const auto field : myRecordDecl->fields()) {
        BuildFieldName(recordLayoutStr, *field);
      }
      recordLayoutStr << "}";
    }
    return "unnamed." + FEUtils::GetHashStr(recordLayoutStr.str());
  }

  uint32 uid;
  if (FEOptions::GetInstance().NeedMangling() && !decl.getLocation().isMacroID()) {
    // use loc as key for wpaa mode
    Loc l = GetLOC(decl.getLocation());
    CHECK_FATAL(l.fileIdx != 0, "loc is invaild");
    std::map<Loc, uint32>::const_iterator itLoc = unnamedLocMap.find(l);
    if (itLoc == unnamedLocMap.cend()) {
      uid = FEUtils::GetSequentialNumber();
      unnamedLocMap[l] = uid;
    } else {
      uid = itLoc->second;
    }
    return FEUtils::GetSequentialName0("unnamed.", uid);
  }
  std::map<int64, uint32>::const_iterator it = unnamedSymbolMap.find(decl.getID());
  if (it == unnamedSymbolMap.cend()) {
    uid = FEUtils::GetSequentialNumber();
    unnamedSymbolMap[decl.getID()] = uid;
  } else {
    uid = it->second;
  }
  return FEUtils::GetSequentialName0("unnamed.", uid);
}

const std::string LibAstFile::GetDeclName(const clang::NamedDecl &decl, bool isRename) {
  std::string name = decl.getNameAsString();
  if (name.empty()) {
    name = GetOrCreateMappedUnnamedName(decl);
  }
  if (isRename && !decl.isDefinedOutsideFunctionOrMethod()) {
    Loc l = GetLOC(decl.getLocation());
    std::stringstream ss;
    ss << name << "_" << l.line << "_" << l.column;
    name = ss.str();
  }
  return name;
}

void LibAstFile::EmitTypeName(const clang::RecordType &recordType, std::stringstream &ss) {
  clang::RecordDecl *recordDecl = recordType.getDecl();
  std::string str = recordType.desugar().getAsString();
  if (!recordDecl->isAnonymousStructOrUnion() && str.find("unnamed") == std::string::npos) {
    clang::DeclContext *ctx = recordDecl->getDeclContext();
    MapleStack<clang::NamedDecl*> nsStack(module->GetMPAllocator().Adapter());
    while (!ctx->isTranslationUnit()) {
      auto *primCtxNsDc = llvm::dyn_cast<clang::NamespaceDecl>(ctx->getPrimaryContext());
      if (primCtxNsDc != nullptr) {
        nsStack.push(primCtxNsDc);
      }
      auto *primCtxRecoDc = llvm::dyn_cast<clang::RecordDecl>(ctx->getPrimaryContext());
      if (primCtxRecoDc != nullptr) {
        nsStack.push(primCtxRecoDc);
      }
      ctx = ctx->getParent();
    }
    while (!nsStack.empty()) {
      auto *nsDc = llvm::dyn_cast<clang::NamespaceDecl>(nsStack.top());
      if (nsDc != nullptr) {
        ss << nsDc->getName().data() << "|";
      }
      auto *rcDc = llvm::dyn_cast<clang::RecordDecl>(nsStack.top());
      if (rcDc != nullptr) {
        EmitTypeName(*rcDc->getTypeForDecl()->getAs<clang::RecordType>(), ss);
      }
      nsStack.pop();
    }
    auto nameStr = recordDecl->getName().str();
    if (nameStr.empty()) {
      nameStr = GetTypedefNameFromUnnamedStruct(*recordDecl);
    }
    if (nameStr.empty()) {
      nameStr = GetOrCreateMappedUnnamedName(*recordDecl);
    }
    ss << nameStr;
  } else {
    ss << GetOrCreateMappedUnnamedName(*recordDecl);
  }
  if (FEOptions::GetInstance().NeedMangling()) {
    std::string layout = recordDecl->getDefinition() == nullptr ? "" : GetRecordLayoutString(*recordDecl);
    ss << FEUtils::GetFileNameHashStr(layout);
  }
  CHECK_FATAL(ss.rdbuf()->in_avail() != 0, "stringstream is empty");
}
void LibAstFile::BuildFieldLayoutString(std::stringstream &recordLayoutStr, const clang::FieldDecl &fieldDecl) {
  std::string canonicalTypeName = fieldDecl.getType().getCanonicalType().getAsString();
  std::string fieldName = GetMangledName(fieldDecl);
  if (fieldName.empty() && fieldDecl.isAnonymousStructOrUnion()) {
    fieldName = GetOrCreateMappedUnnamedName(fieldDecl);
  }

  FEUtils::EraseFileNameforClangTypeStr(canonicalTypeName);
  recordLayoutStr << fieldName << "-" << canonicalTypeName;
  const auto *fieldCl = fieldDecl.getType()->getAsRecordDecl();
  if (fieldCl != nullptr) {
    recordLayoutStr << ":{";
    for (const auto field : fieldCl->fields()) {
      BuildFieldLayoutString(recordLayoutStr, *field);
    }
    recordLayoutStr << "}";
  }
  recordLayoutStr << ",";
}

std::string LibAstFile::GetRecordLayoutString(const clang::RecordDecl &recordDecl) {
  std::stringstream recordLayoutStr;
  recordLayoutStr << recordDecl.getKindName().str() << ":{";
  for (auto field : recordDecl.fields()) {
    BuildFieldLayoutString(recordLayoutStr, *field);
  }
  recordLayoutStr << "}";
  return recordLayoutStr.str();
}

// get TypedefDecl name for the unnamed struct, e.g. typedef struct {} foo;
std::string LibAstFile::GetTypedefNameFromUnnamedStruct(const clang::RecordDecl &recoDecl) const {
  // typedef is parsed in debug mode
  if (FEOptions::GetInstance().IsDbgFriendly()) {
    return std::string();
  }
  auto *defnameDcel = recoDecl.getTypedefNameForAnonDecl();
  if (defnameDcel != nullptr) {
    return defnameDcel->getQualifiedNameAsString();
  }
  return std::string();
}
// with macro enpanded, preproceeded code
std::string LibAstFile::GetSourceText(const clang::Stmt &stmt) const {
  std::string s;
  llvm::raw_string_ostream sos(s);
  clang::PrintingPolicy pp(astContext->getLangOpts());
  stmt.printPretty(sos, nullptr, pp);
  sos.flush();
  return s;
}

// non preproceeded code
std::string LibAstFile::GetSourceTextRaw(const clang::SourceRange range, const clang::SourceManager &sm) const {
  clang::LangOptions lo;
  auto startLoc = sm.getSpellingLoc(range.getBegin());
  auto lastTokenLoc = sm.getSpellingLoc(range.getEnd());
  auto endLoc = clang::Lexer::getLocForEndOfToken(lastTokenLoc, 0, sm, lo);
  auto printableRange = clang::SourceRange{startLoc, endLoc};
  return clang::Lexer::getSourceText(clang::CharSourceRange::getCharRange(printableRange), sm, clang::LangOptions())
      .str();
}

// when function is considered unique, return true;
bool LibAstFile::CheckAndBuildStaticFunctionLayout(const clang::FunctionDecl &funcDecl,
                                                   std::stringstream &funcNameStream,
                                                   std::unordered_set<int64_t> &visitedCalls) {
  if (!funcDecl.getBody()) {
    return false;
  }
  std::string funcSignature = BuildStaticFunctionSignature(*funcDecl.getDefinition());
  funcNameStream << funcSignature << "{";
  std::string funcSourceCode = "";
  funcSourceCode = GetSourceText(*(funcDecl.getBody()));
  funcNameStream << funcSourceCode << "};";
  CallCollector collector = CallCollector(astContext);

  collector.TraverseStmt(funcDecl.getBody());
  if (collector.IsNeedToBeUniq()) {
    return true;
  }
  auto callExprs = collector.GetCallExprs();
  funcNameStream << "->${";
  for (auto const &pair : callExprs) {
    if (visitedCalls.count(pair.first) > 0) {
      // recursive call
      continue;
    }
    (void)visitedCalls.insert(pair.first);
    if (const clang::FunctionDecl *currentFuncDecl = pair.second->getDirectCallee()) {
      if (CheckAndBuildStaticFunctionLayout(*currentFuncDecl, funcNameStream, visitedCalls)) {
        return true;
      }
    }
  }
  funcNameStream << "}$, ";
  return false;
}

void LibAstFile::BuildStaticFunctionLayout(const clang::FunctionDecl &funcDecl, std::string &funcName) {
  std::stringstream funcNameStream;
  funcNameStream << funcName;
  std::unordered_set<int64_t> visitedCalls;
  if (CheckAndBuildStaticFunctionLayout(funcDecl, funcNameStream, visitedCalls)) {
    funcName = funcName + ".static_involved" + this->GetAstFileNameHashStr();
  } else {
    funcName = funcName + FEUtils::GetFileNameHashStr(funcNameStream.str());
  }
}

std::string LibAstFile::BuildStaticFunctionSignature(const clang::FunctionDecl &funcDecl) {
  std::string signature;
  signature += funcDecl.getReturnType().getCanonicalType().getAsString();
  signature += " ";
  signature += funcDecl.getNameAsString();
  signature += "(";

  // If the function definition is a noprototype declaration,
  // which is allowed in c99, the parameter information will not be recorded, as well as func(void);
  if (const auto *funcType = funcDecl.getType()->getAs<clang::FunctionProtoType>()) {
    for (unsigned int i = 0; i < funcType->getNumParams(); i++) {
      const auto paramType = funcType->getParamType(i).getCanonicalType();
      signature += paramType.getAsString() + ",";
    }
    if (signature.back() == ',') {
      signature.pop_back();
    }
  }
  signature += ");";
  return signature;
}
}  // namespace maple
