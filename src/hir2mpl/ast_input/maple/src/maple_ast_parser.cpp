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
  ASTDecl *astDecl = ASTDeclsBuilder::GetASTDecl(decl->GetNodeId());
  if (astDecl != nullptr) {
    return astDecl;
  }

  switch (decl->GetKind()) {
    MAPLE_DECL_CASE(Function)
    MAPLE_DECL_CASE(Identifier)
    default:
      decl->Dump(0);
      CHECK_FATAL(false, "ASTDecl NIY");
      return nullptr;
  }
  return nullptr;
}

ASTDecl *MapleASTParser::ProcessDeclIdentifierNode(MapleAllocator &allocator, maplefe::IdentifierNode *identifierDecl) {
  ASTVar *astVar = static_cast<ASTVar*>(ASTDeclsBuilder::GetASTDecl(identifierDecl->GetNodeId()));
  if (astVar != nullptr) {
    return astVar;
  }

  std::string varName = identifierDecl->GetName();
  if (varName.empty()) {
    return nullptr;
  }
  MIRType *varType = astFile->MapType(identifierDecl->GetType());
  if (varType == nullptr) {
    return nullptr;
  }
  GenericAttrs attrs;
  astVar = ASTDeclsBuilder::ASTVarBuilder(
      allocator, fileName, varName, std::vector<MIRType*>{varType}, attrs, identifierDecl->GetNodeId());

  if (identifierDecl->GetInit() != nullptr) {
    auto astInitExpr = ProcessExpr(allocator, identifierDecl->GetInit());
    astVar->SetInitExpr(astInitExpr);
  }

  return astVar;
}

std::list<ASTDecl*> MapleASTParser::ProcessDeclNode(MapleAllocator &allocator, maplefe::DeclNode *varDecl) {
  maplefe::VarListNode *varList = static_cast<maplefe::VarListNode*>(varDecl->GetVar());
  std::list<ASTDecl*> astVars;
  for (uint32 i = 0; i < varList->GetVarsNum(); ++i) {
    maplefe::IdentifierNode *iNode = varList->GetVarAtIndex(i);
    ASTVar *astVar = static_cast<ASTVar*>(ProcessDecl(allocator, iNode));
    astVars.emplace_back(astVar);
  }
  return astVars;
}

ASTDecl *MapleASTParser::ProcessDeclFunctionNode(MapleAllocator &allocator, maplefe::FunctionNode *funcDecl) {
  ASTFunc *astFunc = static_cast<ASTFunc*>(ASTDeclsBuilder::GetASTDecl(funcDecl->GetNodeId()));
  if (astFunc != nullptr) {
    return astFunc;
  }

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
  astFunc = ASTDeclsBuilder::ASTFuncBuilder(
      allocator, fileName, funcName, typeDescIn, attrs, paramDecls, funcDecl->GetNodeId());
  CHECK_FATAL(astFunc != nullptr, "astFunc is nullptr");

  maplefe::BlockNode *astBody = funcDecl->GetBody();
  if (astBody != nullptr) {
    ASTStmt *astCompoundStmt = ProcessStmt(allocator, astBody);
    if (astCompoundStmt != nullptr) {
      astFunc->SetCompoundStmt(astCompoundStmt);
    }
  }
  astFuncMap[funcDecl->GetStrIdx()] = astFunc;
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
    MAPLE_STMT_CASE(Decl);
    MAPLE_STMT_CASE(Return);
    default: {
      stmt->Dump(0);
      CHECK_FATAL(false, "ASTStmt NIY");
      return nullptr;
    }
  }
}

ASTStmt *MapleASTParser::ProcessStmtBlockNode(MapleAllocator &allocator, maplefe::BlockNode *stmt) {
  ASTCompoundStmt *astCompoundStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTCompoundStmt>(allocator);
  CHECK_FATAL(astCompoundStmt != nullptr, "astCompoundStmt is nullptr");
  ASTStmt *childStmt = nullptr;
  for (uint32 i = 0; i < stmt->GetChildrenNum(); ++i) {
    maplefe::TreeNode *child = stmt->GetChildAtIndex(i);
    childStmt = ProcessStmt(allocator, child);
    if (childStmt != nullptr) {
      astCompoundStmt->SetASTStmt(childStmt);
    } else {
      continue;
    }
  }
  return astCompoundStmt;
}

ASTStmt *MapleASTParser::ProcessStmtDeclNode(MapleAllocator &allocator, maplefe::DeclNode *stmt) {
  ASTDeclStmt *declStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTDeclStmt>(allocator);
  std::list<ASTDecl*> astVars = ProcessDeclNode(allocator, stmt);
  for (auto &var : astVars) {
    declStmt->SetSubDecl(var);
  }
  return declStmt;
}

ASTStmt *MapleASTParser::ProcessStmtReturnNode(MapleAllocator &allocator, maplefe::ReturnNode *stmt) {
  ASTReturnStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTReturnStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, stmt->GetResult());
  astStmt->SetASTExpr(astExpr);
  return astStmt;
}

#define MAPLE_EXPR_CASE(CLASS)                                                                         \
  case maplefe::NK_##CLASS: {                                                                          \
    ASTExpr *astExpr = ProcessExpr##CLASS##Node(allocator, static_cast<maplefe::CLASS##Node*>(expr));  \
    if (astExpr == nullptr) {                                                                          \
      return nullptr;                                                                                  \
    }                                                                                                  \
    return astExpr;                                                                                    \
  }

ASTExpr *MapleASTParser::ProcessExpr(MapleAllocator &allocator, maplefe::TreeNode *expr) {
  if (expr == nullptr) {
    return nullptr;
  }

  switch (expr->GetKind()) {
    MAPLE_EXPR_CASE(Literal);
    MAPLE_EXPR_CASE(Call);
    MAPLE_EXPR_CASE(Identifier);
    default:
      expr->Dump(0);
      CHECK_FATAL(false, "ASTExpr NIY");
      return nullptr;
  }
}

ASTExpr *MapleASTParser::ProcessExprLiteralNode(MapleAllocator &allocator, maplefe::LiteralNode *expr) {
  ASTExpr *literalExpr = nullptr;
  switch (expr->GetData().mType) {
    case maplefe::LT_IntegerLiteral:
      literalExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
      literalExpr->SetType(GlobalTables::GetTypeTable().GetInt64());
      static_cast<ASTIntegerLiteral*>(literalExpr)->SetVal(expr->GetData().mData.mInt64);
      break;
    case maplefe::LT_FPLiteral:
      literalExpr = ASTDeclsBuilder::ASTExprBuilder<ASTFloatingLiteral>(allocator);
      literalExpr->SetType(GlobalTables::GetTypeTable().GetFloat());
      static_cast<ASTFloatingLiteral*>(literalExpr)->SetVal(expr->GetData().mData.mFloat);
      break;
    case maplefe::LT_DoubleLiteral:
      literalExpr = ASTDeclsBuilder::ASTExprBuilder<ASTFloatingLiteral>(allocator);
      literalExpr->SetType(GlobalTables::GetTypeTable().GetDouble());
      static_cast<ASTFloatingLiteral*>(literalExpr)->SetVal(expr->GetData().mData.mDouble);
      break;
    default:
      CHECK_FATAL(false, "NYI");
      break;
  }
  return literalExpr;
}

ASTExpr *MapleASTParser::ProcessExprIdentifierNode(MapleAllocator &allocator, maplefe::IdentifierNode *expr) {
  ASTDeclRefExpr *astRefExpr = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
  CHECK_FATAL(astRefExpr != nullptr, "astRefExpr is nullptr");
  ASTDecl *astDecl = ASTDeclsBuilder::GetASTDecl(expr->GetNodeId());
  if (astDecl == nullptr) {
    astDecl = ProcessDecl(allocator, expr);
  }
  astRefExpr->SetASTDecl(astDecl);
  astRefExpr->SetType(astDecl->GetTypeDesc().front());
  return astRefExpr;
}

ASTExpr *MapleASTParser::ProcessExprCallNode(MapleAllocator &allocator, maplefe::CallNode *expr) {
  ASTCallExpr *astCallExpr = ASTDeclsBuilder::ASTExprBuilder<ASTCallExpr>(allocator);
  ASSERT(astCallExpr != nullptr, "astCallExpr is nullptr");
  // callee
  ASTExpr *astCallee = ProcessExpr(allocator, expr->GetMethod());
  if (astCallee == nullptr) {
    return nullptr;
  }
  astCallExpr->SetCalleeExpr(astCallee);
  // return
  // C language function names can be used as unique identifiers
  MIRType *retType = astFuncMap[expr->GetMethod()->GetStrIdx()]->GetTypeDesc()[1];
  astCallExpr->SetRetType(retType);
  // args
  std::vector<ASTExpr*> args;
  for (uint32_t i = 0; i < expr->GetArgsNum(); ++i) {
    maplefe::TreeNode *subExpr = expr->GetArg(i);
    ASTExpr *arg = ProcessExpr(allocator, subExpr);
    args.push_back(arg);
  }
  astCallExpr->SetArgs(args);
  if (expr->GetMethod() != nullptr) {
    GenericAttrs attrs;
    astCallExpr->SetFuncName(astCallee->GetASTDecl()->GetName());
    astCallExpr->SetFuncAttrs(attrs.ConvertToFuncAttrs());
  } else {
    astCallExpr->SetIcall(true);
  }
  return astCallExpr;
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
    std::list<ASTDecl*> astVarList = ProcessDeclNode(allocator, static_cast<maplefe::DeclNode*>(decl));
    for (auto &astVar : astVarList) {
      astVars.emplace_back(static_cast<ASTVar*>(astVar));
    }
  }
  return true;
}

bool MapleASTParser::RetrieveFileScopeAsms(MapleAllocator &allocator) {
  return true;
}
} // namespace maple
