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
#ifndef HIR2MPL_AST_FILE_INCLUDE_AST_ALIAS_H
#define HIR2MPL_AST_FILE_INCLUDE_AST_ALIAS_H
#include "clang-c/Index.h"
#include "libclang/CIndexer.h"
#include "libclang/CXTranslationUnit.h"
#include "clang/Frontend/ASTUnit.h"
#include "clang/AST/Decl.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/GlobalDecl.h"
#include "clang/AST/Mangle.h"
#include "clang/AST/VTableBuilder.h"
#include "clang/AST/VTTBuilder.h"
#include "clang/Lex/Lexer.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/AST/DeclBase.h"

namespace maple {
  using AstASTContext = clang::ASTContext;
  using AstUnitDecl = clang::TranslationUnitDecl;
  using AstASTUnit = clang::ASTUnit;
} // namespace maple
#endif // HIR2MPL_AST_FILE_INCLUDE_AST_ALIAS_H
