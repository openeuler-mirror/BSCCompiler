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
#include "ast_interface.h"
#include "mpl_logging.h"
#include "ast_util.h"
#include "fe_utils.h"
#include "fe_manager.h"

namespace maple {
bool LibAstFile::Open(const std::string &fileName,
                      int excludeDeclFromPCH, int displayDiagnostics) {
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
  if (astUnit == nullptr) {
    return false;
  }
  astUnitDecl = astContext->getTranslationUnitDecl();
  if (astUnit == nullptr) {
    return false;
  }
  mangleContext = astContext->createMangleContext();
  if (mangleContext == nullptr) {
    return false;
  }
  return true;
}

const AstASTContext *LibAstFile::GetAstContext() {
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
    const std::string name = FEUtils::GetSequentialName("unNamed");
    unnamedSymbolMap[id] = name;
  }
  return unnamedSymbolMap[id];
}

const std::string LibAstFile::GetOrCreateCompoundLiteralExprInitName(uint32_t id) {
  std::map<uint32_t, std::string>::iterator it = CompoundLiteralExprInitSymbolMap.find(id);
  if (it == CompoundLiteralExprInitSymbolMap.end()) {
    const std::string name = FEUtils::GetSequentialName("unNamedInit");
    CompoundLiteralExprInitSymbolMap[id] = name;
  }
  return CompoundLiteralExprInitSymbolMap[id];
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
} // namespace maple
