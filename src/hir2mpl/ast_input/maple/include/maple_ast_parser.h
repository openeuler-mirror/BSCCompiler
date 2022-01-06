/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_AST_PARSER_H
#define MAPLE_AST_PARSER_H
#include <string>
#include <stdio.h>
#include "mempool_allocator.h"
#include "ast_decl.h"
#include "maple_ast_interface.h"
#include "ast.h"

namespace maple {
class MapleASTParser {
 public:
  MapleASTParser(MapleAllocator &allocatorIn, uint32 fileIdxIn, const std::string &fileNameIn,
                 MapleList<ASTStruct*> &astStructsIn, MapleList<ASTFunc*> &astFuncsIn, MapleList<ASTVar*> &astVarsIn,
                 MapleList<ASTFileScopeAsm*> &astFileScopeAsmsIn)
      : fileIdx(fileIdxIn), fileName(fileNameIn),
        globalVarDecles(allocatorIn.Adapter()), funcDecles(allocatorIn.Adapter()),
        astStructs(astStructsIn), astFuncs(astFuncsIn),
        astVars(astVarsIn), astFileScopeAsms(astFileScopeAsmsIn),
        astFuncMap(allocatorIn.Adapter()) {}
  virtual ~MapleASTParser() = default;
  bool OpenFile();
  const uint32 GetFileIdx() const;
  bool Verify() const;
  bool PreProcessAST();

  std::list<ASTDecl*> ProcessDeclNode(MapleAllocator &allocator, maplefe::DeclNode *decl);
  ASTDecl *ProcessDecl(MapleAllocator &allocator, maplefe::TreeNode *decl);
#define MAPLE_PROCESS_DECL(CLASS) ProcessDecl##CLASS##Node(MapleAllocator &allocator, maplefe::CLASS##Node*)
  ASTDecl *MAPLE_PROCESS_DECL(Function);
  ASTDecl *MAPLE_PROCESS_DECL(Identifier);

  ASTStmt *ProcessStmt(MapleAllocator &allocator, maplefe::TreeNode *stmt);
#define MAPLE_PROCESS_STMT(CLASS) ProcessStmt##CLASS##Node(MapleAllocator&, maplefe::CLASS##Node*)
  ASTStmt *MAPLE_PROCESS_STMT(Block);
  ASTStmt *MAPLE_PROCESS_STMT(Decl);
  ASTStmt *MAPLE_PROCESS_STMT(Return);

  ASTExpr *ProcessExpr(MapleAllocator &allocator, maplefe::TreeNode *expr);
#define MAPLE_PROCESS_EXPR(CLASS) ProcessExpr##CLASS##Node(MapleAllocator&, maplefe::CLASS##Node*)
  ASTExpr *MAPLE_PROCESS_EXPR(Literal);
  ASTExpr *MAPLE_PROCESS_EXPR(Call);
  ASTExpr *MAPLE_PROCESS_EXPR(Identifier);

  bool RetrieveStructs(MapleAllocator &allocator);
  bool RetrieveFuncs(MapleAllocator &allocator);
  bool RetrieveGlobalVars(MapleAllocator &allocator);
  bool RetrieveFileScopeAsms(MapleAllocator &allocator);

 private:
  uint32 fileIdx;
  const std::string fileName;
  std::unique_ptr<LibMapleAstFile> astFile;
  MapleList<maplefe::TreeNode*> globalVarDecles;
  MapleList<maplefe::TreeNode*> funcDecles;
  MapleList<ASTStruct*> &astStructs;
  MapleList<ASTFunc*> &astFuncs;
  MapleList<ASTVar*> &astVars;
  MapleList<ASTFileScopeAsm*> &astFileScopeAsms;
  MapleMap<uint32, ASTFunc*> astFuncMap;
};
} // namespace maple
#endif // MAPLE_AST_PARSER_H
