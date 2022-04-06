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
#include "mpl_logging.h"
#include "ast_util.h"
#include "fe_utils.h"
#include "fe_manager.h"

namespace maple {
bool LibAstFile::Open(const std::string &fileName,
                      int excludeDeclFromPCH, int displayDiagnostics) {
  astFileName = fileName;
  CXIndex index = clang_createIndex(excludeDeclFromPCH, displayDiagnostics);
  CXTranslationUnit translationUnit = clang_createTranslationUnit(index, fileName.c_str());
  if (translationUnit == nullptr) {
    return false;
  }
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

const AstASTContext *LibAstFile::GetAstContext() const {
  return astContext;
}

AstASTContext *LibAstFile::GetNonConstAstContext() const {
  return astContext;
}

AstUnitDecl *LibAstFile::GetAstUnitDecl() {
  return astUnitDecl;
}

std::string LibAstFile::GetMangledName(const clang::NamedDecl &decl) {
  std::string mangledName;
  if (!mangleContext->shouldMangleDeclName(&decl)) {
    mangledName = decl.getNameAsString();
  } else {
    llvm::raw_string_ostream ostream(mangledName);
    if (llvm::isa<clang::CXXConstructorDecl>(&decl)) {
      const auto *ctor = static_cast<const clang::CXXConstructorDecl*>(&decl);
      mangleContext->mangleCtorBlock(ctor, static_cast<clang::CXXCtorType>(0), nullptr, ostream);
    } else if (llvm::isa<clang::CXXDestructorDecl>(&decl)) {
      const auto *dtor = static_cast<const clang::CXXDestructorDecl*>(&decl);
      mangleContext->mangleDtorBlock(dtor, static_cast<clang::CXXDtorType>(0), nullptr, ostream);
    } else {
      mangleContext->mangleName(&decl, ostream);
    }
    ostream.flush();
  }
  return mangledName;
}

Pos LibAstFile::GetDeclPosInfo(const clang::Decl &decl) const {
  clang::FullSourceLoc fullLocation = astContext->getFullLoc(decl.getBeginLoc());
  return std::make_pair(static_cast<uint32>(fullLocation.getSpellingLineNumber()),
                        static_cast<uint32>(fullLocation.getSpellingColumnNumber()));
}

Pos LibAstFile::GetStmtLOC(const clang::Stmt &stmt) const {
  return GetLOC(stmt.getBeginLoc());
}

Pos LibAstFile::GetExprLOC(const clang::Expr &expr) const {
  return GetLOC(expr.getExprLoc());
}

Pos LibAstFile::GetLOC(const clang::SourceLocation &srcLoc) const {
  if (srcLoc.isInvalid()) {
    return std::make_pair(0, 0);
  }
  if (srcLoc.isFileID()) {
    clang::PresumedLoc pLOC = astContext->getSourceManager().getPresumedLoc(srcLoc);
    if (pLOC.isInvalid()) {
      return std::make_pair(0, 0);
    }
    std::string fileName = pLOC.getFilename();
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(fileName);
    for (const auto &info : FEManager::GetModule().GetSrcFileInfo()) {
      if (info.first == strIdx) {
        return std::make_pair(info.second, static_cast<uint32>(pLOC.getLine()));
      }
    }
    if (FEManager::GetModule().GetSrcFileInfo().empty()) {
      // src files start from 2, 1 is mpl file
      FEManager::GetModule().PushbackFileInfo(MIRInfoPair(strIdx, 2));
      return std::make_pair(2, static_cast<uint32>(pLOC.getLine()));
    } else {
      auto last = FEManager::GetModule().GetSrcFileInfo().rbegin();
      FEManager::GetModule().PushbackFileInfo(MIRInfoPair(strIdx, last->second + 1));
      return std::make_pair(last->second + 1, static_cast<uint32>(pLOC.getLine()));
    }
  }

  return GetLOC(astContext->getSourceManager().getExpansionLoc(srcLoc));
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

void LibAstFile::GetCVRAttrs(uint32_t qualifiers, GenericAttrs &genAttrs) {
  if (qualifiers & clang::Qualifiers::Const) {
    genAttrs.SetAttr(GENATTR_const);
  }
  if (qualifiers & clang::Qualifiers::Restrict) {
    genAttrs.SetAttr(GENATTR_restrict);
  }
  if (qualifiers & clang::Qualifiers::Volatile) {
    genAttrs.SetAttr(GENATTR_volatile);
  }
}

void LibAstFile::GetSClassAttrs(const clang::StorageClass storageClass, GenericAttrs &genAttrs) const {
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

void LibAstFile::GetStorageAttrs(const clang::NamedDecl &decl, GenericAttrs &genAttrs) const {
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
      GetSClassAttrs(storageClass, genAttrs);
      break;
    }
    case clang::Decl::Field:
    default:
      break;
  }
  return;
}

void LibAstFile::GetAccessAttrs(AccessKind access, GenericAttrs &genAttrs) {
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

void LibAstFile::GetQualAttrs(const clang::NamedDecl &decl, GenericAttrs &genAttrs) {
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

void LibAstFile::CollectAttrs(const clang::NamedDecl &decl, GenericAttrs &genAttrs, AccessKind access) {
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

void LibAstFile::CollectFuncAttrs(const clang::FunctionDecl &decl, GenericAttrs &genAttrs, AccessKind access) {
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
  CheckUnsupportedFuncAttrs(decl);
}

void LibAstFile::CheckUnsupportedFuncAttrs(const clang::FunctionDecl &decl) {
  if (!decl.hasAttrs()) {
    return;
  }
  std::string unsupportedFuncAttrs = "";
  const clang::AttrVec &funcAttrs = decl.getAttrs();
  for (const auto *attr : funcAttrs) {
    clang::attr::Kind attrKind = attr->getKind();
    auto iterator = LibAstFile::unsupportedFuncAttrsMap.find(attrKind);
    if (iterator != LibAstFile::unsupportedFuncAttrsMap.end()) {
      unsupportedFuncAttrs += iterator->second + " ";
    }
  }
  CHECK_FATAL(unsupportedFuncAttrs.empty(), "%s:%d error: The function %s has unsupported attribute(s): %s",
              FEManager::GetModule().GetFileNameFromFileNum(GetLOC(decl.getLocation()).first).c_str(),
              GetLOC(decl.getLocation()).second,
              GetMangledName(decl).c_str(),
              unsupportedFuncAttrs.c_str());
}

void LibAstFile::CollectVarAttrs(const clang::VarDecl &decl, GenericAttrs &genAttrs, AccessKind access) {
  CollectAttrs(decl, genAttrs, access);
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

void LibAstFile::CheckUnsupportedVarAttrs(const clang::VarDecl &decl) {
  if (!decl.hasAttrs()) {
    return;
  }
  std::string unsupportedVarAttrs = "";
  const clang::AttrVec &varAttrs = decl.getAttrs();
  for (const auto *attr : varAttrs) {
    clang::attr::Kind attrKind = attr->getKind();
    auto iterator = LibAstFile::unsupportedVarAttrsMap.find(attrKind);
    if (iterator != LibAstFile::unsupportedVarAttrsMap.end()) {
      unsupportedVarAttrs += iterator->second + " ";
    }
  }
  CHECK_FATAL(unsupportedVarAttrs.empty(), "%s:%d error: The variable %s has unsupported attribute(s): %s",
              FEManager::GetModule().GetFileNameFromFileNum(GetLOC(decl.getLocation()).first).c_str(),
              GetLOC(decl.getLocation()).second,
              GetMangledName(decl).c_str(),
              unsupportedVarAttrs.c_str());
}

void LibAstFile::CollectRecordAttrs(const clang::RecordDecl &decl, GenericAttrs &genAttrs, AccessKind access) {
  clang::PackedAttr *packedAttr = decl.getAttr<clang::PackedAttr>();
  if (packedAttr != nullptr) {
    genAttrs.SetAttr(GENATTR_pack);
    genAttrs.InsertIntContentMap(GENATTR_pack, 1); // 1 byte
  }
  clang::MaxFieldAlignmentAttr *maxFieldAlignAttr = decl.getAttr<clang::MaxFieldAlignmentAttr>();
  if (maxFieldAlignAttr != nullptr) {
    genAttrs.SetAttr(GENATTR_pack);
    genAttrs.InsertIntContentMap(GENATTR_pack, static_cast<uint32>(maxFieldAlignAttr->getAlignment() / 8));
  }
  CheckUnsupportedTypeAttrs(decl);
}

void LibAstFile::CheckUnsupportedTypeAttrs(const clang::RecordDecl &decl) {
  if (!decl.hasAttrs()) {
    return;
  }
  std::string unsupportedTypeAttrs = "";
  const clang::AttrVec &typeAttrs = decl.getAttrs();
  for (const auto *attr : typeAttrs) {
    clang::attr::Kind attrKind = attr->getKind();
    auto iterator = LibAstFile::unsupportedTypeAttrsMap.find(attrKind);
    if (iterator != LibAstFile::unsupportedTypeAttrsMap.end()) {
      unsupportedTypeAttrs += iterator->second + " ";
    }
  }
  CHECK_FATAL(unsupportedTypeAttrs.empty(), "%s:%d error: struct or union %s has unsupported type attribute(s): %s",
              FEManager::GetModule().GetFileNameFromFileNum(GetLOC(decl.getLocation()).first).c_str(),
              GetLOC(decl.getLocation()).second,
              GetMangledName(decl).c_str(),
              unsupportedTypeAttrs.c_str());
}

void LibAstFile::CollectFieldAttrs(const clang::FieldDecl &decl, GenericAttrs &genAttrs, AccessKind access) {
  CollectAttrs(decl, genAttrs, access);
  clang::PackedAttr *packedAttr = decl.getAttr<clang::PackedAttr>();
  if (packedAttr != nullptr) {
    genAttrs.SetAttr(GENATTR_pack);
    genAttrs.InsertIntContentMap(GENATTR_pack, 1); // 1 byte
  }
}

void LibAstFile::EmitTypeName(const clang::QualType qualType, std::stringstream &ss) {
  switch (qualType->getTypeClass()) {
    case clang::Type::LValueReference: {
      ss << "R";
      const clang::QualType pointeeType = qualType->castAs<clang::ReferenceType>()->getPointeeType();
      EmitTypeName(pointeeType, ss);
      break;
    }
    case clang::Type::Pointer: {
      ss << "P";
      const clang::QualType pointeeType = qualType->castAs<clang::PointerType>()->getPointeeType();
      EmitTypeName(pointeeType, ss);
      break;
    }
    case clang::Type::Record: {
      EmitTypeName(*qualType->getAs<clang::RecordType>(), ss);
      break;
    }
    default: {
      EmitQualifierName(qualType, ss);
      MIRType *type = CvtType(qualType);
      ss << ASTUtil::GetTypeString(*type);
      break;
    }
  }
}

void LibAstFile::EmitQualifierName(const clang::QualType qualType, std::stringstream &ss) {
  uint32_t cvrQual = qualType.getCVRQualifiers();
  if ((cvrQual & clang::Qualifiers::Const) != 0) {
    ss << "K";
  }
  if (cvrQual & clang::Qualifiers::Volatile) {
    ss << "U";
  }
}

const std::string LibAstFile::GetOrCreateMappedUnnamedName(uint32_t id) {
  std::map<uint32_t, std::string>::iterator it = unnamedSymbolMap.find(id);
  if (it == unnamedSymbolMap.end()) {
    std::string name = FEUtils::GetSequentialName("unNamed");
    unnamedSymbolMap[id] = name;
  }
  return unnamedSymbolMap[id];
}

void LibAstFile::EmitTypeName(const clang::RecordType &recoType, std::stringstream &ss) {
  clang::RecordDecl *recoDecl = recoType.getDecl();
  std::string str = recoType.desugar().getAsString();
  if (!recoDecl->isAnonymousStructOrUnion() && str.find("anonymous") == std::string::npos) {
    clang::DeclContext *ctx = recoDecl->getDeclContext();
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
    auto nameStr = recoDecl->getName().str();
    if (nameStr.empty()) {
      nameStr = GetTypedefNameFromUnnamedStruct(*recoDecl);
    }
    if (nameStr.empty()) {
      uint32_t id = recoType.getDecl()->getLocation().getRawEncoding();
      nameStr = GetOrCreateMappedUnnamedName(id);
    }
    ss << nameStr;
  } else {
    uint32_t id = recoType.getDecl()->getLocation().getRawEncoding();
    ss << GetOrCreateMappedUnnamedName(id);
  }

  if (!recoDecl->isDefinedOutsideFunctionOrMethod()) {
    Pos p = GetDeclPosInfo(*recoDecl);
    ss << "_" << p.first << "_" << p.second;
  }
}

// get TypedefDecl name for the unnamed struct, e.g. typedef struct {} foo;
std::string LibAstFile::GetTypedefNameFromUnnamedStruct(const clang::RecordDecl &recoDecl) {
  auto *defnameDcel = recoDecl.getTypedefNameForAnonDecl();
  if (defnameDcel != nullptr) {
    return defnameDcel->getQualifiedNameAsString();
  }
  return std::string();
}
} // namespace maple
