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
#include "maple_ast_parser.h"
#include "ast_decl_builder.h"

namespace maple {
bool MapleASTParser::OpenFile() {
  (void)astStructs;
  (void)astFileScopeAsms;
  astFile = std::make_unique<LibMapleAstFile>();
  bool res = astFile->Open(fileName);
  if (!res) {
    return false;
  }
  return true;
}

const uint32 MapleASTParser::GetFileIdx() const {
  return fileIdx;
}

bool MapleASTParser::Verify() const {
  return true;
}

bool MapleASTParser::PreProcessAST() {
  for (uint32 i = 0; i < astFile->GetASTHandler().GetSize(); ++i) {
    maplefe::Module_Handler *handler = astFile->GetASTHandler().GetModuleHandler(i);
    maplefe::ModuleNode *module = handler->GetASTModule();
    for (uint32 i = 0; i < module->GetTreesNum(); ++i) {
      maplefe::TreeNode *node = module->GetTree(i);
      switch (node->GetKind()) {
        case maplefe::NK_Decl:
          globalVarDecles.emplace_back(node);
          break;
        case maplefe::NK_Function:
          funcDecles.emplace_back(node);
          break;
        default: break;
      }
    }
  }
  return true;
}


#define MAPLE_DECL_CASE(CLASS)                                                                           \
  case maplefe::NK_##CLASS: {                                                                            \
    ASTDecl *astDecl = ProcessDecl##CLASS##Node(allocator, static_cast<maplefe::CLASS##Node*>(decl));    \
    if (astDecl != nullptr) {                                                                            \
    }                                                                                                    \
    return astDecl;                                                                                      \
  }

ASTDecl *MapleASTParser::ProcessDecl(MapleAllocator &allocator, maplefe::TreeNode *decl) {
  switch (decl->GetKind()) {
    MAPLE_DECL_CASE(Function)
    MAPLE_DECL_CASE(Identifier)
    MAPLE_DECL_CASE(Decl)
    default:
      CHECK_FATAL(false, "ASTDecl: %s NIY", decl->GetName());
      return nullptr;
  }
  return nullptr;
}

ASTDecl *MapleASTParser::ProcessDeclIdentifierNode(MapleAllocator &allocator, maplefe::IdentifierNode *identifierDecl) {
  std::string varName = identifierDecl->GetName();
  if (varName.empty()) {
    return nullptr;
  }
  MIRType *varType = astFile->MapType(identifierDecl->GetType());
  if (varType == nullptr) {
    return nullptr;
  }
  GenericAttrs attrs;
  ASTVar *astVar = ASTDeclsBuilder::ASTVarBuilder(
      allocator, fileName, varName, std::vector<MIRType*>{varType}, attrs);
  return astVar;
}

ASTDecl *MapleASTParser::ProcessDeclDeclNode(MapleAllocator &allocator, maplefe::DeclNode *varDecl) {
  maplefe::VarListNode *varList = static_cast<maplefe::VarListNode*>(varDecl->GetVar());
  ASTVar *astVar = nullptr;
  for (uint32 i = 0; i < varList->GetVarsNum(); ++i) {
    maplefe::IdentifierNode *iNode = varList->GetVarAtIndex(i);
    astVar = static_cast<ASTVar*>(ProcessDecl(allocator, iNode));
    if (i != varList->GetVarsNum() - 1) {
      astVars.emplace_back(astVar);
    }
  }
  return astVar;
}

ASTDecl *MapleASTParser::ProcessDeclFunctionNode(MapleAllocator &allocator, maplefe::FunctionNode *funcDecl) {
  std::string funcName = funcDecl->GetName();
  if (funcName.empty()) {
    return nullptr;
  }

  std::vector<MIRType*> typeDescIn;
  typeDescIn.push_back(nullptr); // mirFuncType
  MIRType *retType = astFile->MapType(funcDecl->GetType());
  if (retType == nullptr) {
    return nullptr;
  }
  typeDescIn.push_back(retType);

  std::vector<ASTDecl*> paramDecls;
  uint32 numParam = funcDecl->GetParamsNum();
  for (uint32 i = 0; i < numParam; ++i) {
    maplefe::TreeNode *param = funcDecl->GetParam(i);
    ASTDecl *parmVarDecl = nullptr;
    if (param->IsIdentifier()) {
      parmVarDecl = ProcessDecl(allocator, param);
    } else {
      continue;
    }
    paramDecls.push_back(parmVarDecl);
    typeDescIn.push_back(parmVarDecl->GetTypeDesc().front());
  }

  GenericAttrs attrs;
  ASTFunc *astFunc = ASTDeclsBuilder::ASTFuncBuilder(
      allocator, fileName, funcName, typeDescIn, attrs, paramDecls);
  CHECK_FATAL(astFunc != nullptr, "astFunc is nullptr");

  maplefe::BlockNode *astBody = funcDecl->GetBody();
  if (astBody != nullptr) {
    ASTStmt *astCompoundStmt = ProcessStmt(allocator, astBody);
    if (astCompoundStmt != nullptr) {
      astFunc->SetCompoundStmt(astCompoundStmt);
    }
  }
  return astFunc;
}


#define MAPLE_STMT_CASE(CLASS)                                                                        \
  case maplefe::NK_##CLASS: {                                                                         \
    ASTStmt *astStmt = ProcessStmt##CLASS##Node(allocator, static_cast<maplefe::CLASS##Node*>(stmt)); \
    return astStmt;                                                                                   \
  }

ASTStmt *MapleASTParser::ProcessStmt(MapleAllocator &allocator, maplefe::TreeNode *stmt) {
  switch (stmt->GetKind()) {
    MAPLE_STMT_CASE(Block);
    default: {
      CHECK_FATAL(false, "ASTStmt: %s NIY", stmt->GetName());
      return nullptr;
    }
  }
}

ASTStmt *MapleASTParser::ProcessStmtBlockNode(MapleAllocator &allocator, maplefe::BlockNode *stmt) {
  return nullptr;
}

bool MapleASTParser::RetrieveStructs(MapleAllocator &allocator) {
  return true;
}

bool MapleASTParser::RetrieveFuncs(MapleAllocator &allocator) {
  for (auto &decl : funcDecles) {
    ASTFunc *funcDecl = static_cast<ASTFunc*>(ProcessDecl(allocator, decl));
    if (funcDecl == nullptr) {
      return false;
    }
    funcDecl->SetGlobal(true);
    astFuncs.emplace_back(funcDecl);
  }
  return true;
}

bool MapleASTParser::RetrieveGlobalVars(MapleAllocator &allocator) {
  for (auto &decl : globalVarDecles) {
    ASTVar *astVar = static_cast<ASTVar*>(ProcessDecl(allocator, decl));
    if (astVar == nullptr) {
      return false;
    }
    astVars.emplace_back(astVar);
  }
  return true;
}

bool MapleASTParser::RetrieveFileScopeAsms(MapleAllocator &allocator) {
  return true;
}
} // namespace maple
