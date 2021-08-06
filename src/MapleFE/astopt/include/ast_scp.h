/*
* Copyright (C) [2021] Futurewei Technologies, Inc. All rights reverved.
*
* OpenArkFE is licensed under the Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*
*  http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
* FIT FOR A PARTICULAR PURPOSE.
* See the Mulan PSL v2 for more details.
*/

#ifndef __AST_SCP_HEADER__
#define __AST_SCP_HEADER__

#include <stack>
#include <deque>
#include <utility>

#include "stringpool.h"
#include "ast_module.h"
#include "ast.h"
#include "ast_type.h"
#include "ast_handler.h"
#include "gen_astvisitor.h"

namespace maplefe {

class AST_SCP {
 private:
  Module_Handler *mHandler;
  unsigned        mFlags;

 public:
  explicit AST_SCP(Module_Handler *h, unsigned f) : mHandler(h), mFlags(f) {}
  ~AST_SCP() {};

  void BuildScope();
  void RenameVar();
};

class BuildScopeBaseVisitor : public AstVisitor {
 private:
  unsigned mFlags;

 public:
  std::stack<ASTScope *> mScopeStack;
  std::stack<ASTScope *> mUserScopeStack;

 public:
  explicit BuildScopeBaseVisitor(unsigned f, bool base = false)
    : mFlags(f), AstVisitor((f & FLG_trace_1) && base) {}
  ~BuildScopeBaseVisitor() = default;

#undef  NODEKIND
#define NODEKIND(K) virtual K##Node *Visit##K##Node(K##Node *node) {\
  ASTScope *scope = mScopeStack.top(); \
  node->SetScope(scope); \
  (void) AstVisitor::Visit##K##Node(node); \
  return node; \
}
#include "ast_nk.def"
};

class BuildScopeVisitor : public BuildScopeBaseVisitor {
 private:
  Module_Handler *mHandler;
  ModuleNode     *mASTModule;
  unsigned        mFlags;

 public:
  explicit BuildScopeVisitor(Module_Handler *h, unsigned f, bool base = false)
    : mHandler(h), mFlags(f), BuildScopeBaseVisitor(f, base) {
      mASTModule = mHandler->GetASTModule();
    }
  ~BuildScopeVisitor() = default;

  // scope nodes
  BlockNode *VisitBlockNode(BlockNode *node);
  FunctionNode *VisitFunctionNode(FunctionNode *node);
  LambdaNode *VisitLambdaNode(LambdaNode *node);
  ClassNode *VisitClassNode(ClassNode *node);
  StructNode *VisitStructNode(StructNode *node);
  InterfaceNode *VisitInterfaceNode(InterfaceNode *node);
  ForLoopNode *VisitForLoopNode(ForLoopNode *node);

  // related node with scope : decl, type
  DeclNode *VisitDeclNode(DeclNode *node);
  UserTypeNode *VisitUserTypeNode(UserTypeNode *node);
  TypeAliasNode *VisitTypeAliasNode(TypeAliasNode *node);

};

class RenameVarVisitor : public AstVisitor {
 private:
  Module_Handler *mHandler;
  ModuleNode     *mASTModule;
  unsigned        mFlags;

 public:
  unsigned        mPass;
  unsigned        mOldStrIdx;
  unsigned        mNewStrIdx;
  std::unordered_map<unsigned, TreeNode*> mNodeId2NodeMap;
  std::unordered_map<unsigned, std::deque<unsigned>> mStridx2DeclIdMap;

 public:
  explicit RenameVarVisitor(Module_Handler *h, unsigned f, bool base = false)
    : mHandler(h), mFlags(f), AstVisitor((f & FLG_trace_1) && base) {
      mASTModule = mHandler->GetASTModule();
    }
  ~RenameVarVisitor() = default;

  bool SkipRename(IdentifierNode *node);
  bool IsFuncArg(FunctionNode *func, IdentifierNode *node);
  void InsertToStridx2DeclIdMap(unsigned stridx, IdentifierNode *node);
  IdentifierNode *VisitIdentifierNode(IdentifierNode *node);
};

}
#endif
