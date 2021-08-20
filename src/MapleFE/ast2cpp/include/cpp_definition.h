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

#ifndef __CPPDEFINITIONEMITTER_HEADER__
#define __CPPDEFINITIONEMITTER_HEADER__

#include "ast_handler.h"
#include "emitter.h"
#include "cpp_declaration.h"

namespace maplefe {

class CppDef : public Emitter {
public:
  Module_Handler *mHandler;
  bool            isInit;
  CppDecl        &mCppDecl;
  CppDef(Module_Handler *h, CppDecl &d) : mHandler(h), mCppDecl(d), Emitter(h->GetASTModule()) {}

  std::string Emit() {
    return EmitTreeNode(GetASTModule());
  }

  virtual std::string EmitIdentifierNode(IdentifierNode *node);
  virtual std::string EmitXXportAsPairNode(XXportAsPairNode *node);
  virtual std::string EmitExportNode(ExportNode *node);
  virtual std::string EmitUnaOperatorNode(UnaOperatorNode *node);
  virtual std::string EmitBinOperatorNode(BinOperatorNode *node);
  virtual std::string EmitBlockNode(BlockNode *node);
  virtual std::string EmitDeclNode(DeclNode *node);
  virtual std::string EmitFieldNode(FieldNode *node);
  virtual std::string EmitArrayLiteralNode(ArrayLiteralNode *node);
  virtual std::string EmitTemplateLiteralNode(TemplateLiteralNode *node);
  virtual std::string EmitLiteralNode(LiteralNode *node);
  virtual std::string EmitCondBranchNode(CondBranchNode *node);
  virtual std::string EmitBreakNode(BreakNode *node);
  virtual std::string EmitContinueNode(ContinueNode *node);
  virtual std::string EmitForLoopNode(ForLoopNode *node);
  virtual std::string EmitSwitchNode(SwitchNode *node);
  virtual std::string EmitCallNode(CallNode *node);
  virtual std::string EmitFunctionNode(FunctionNode *node);
  virtual std::string EmitTypeOfNode(TypeOfNode *node);
  virtual std::string EmitModuleNode(ModuleNode *node);
  virtual std::string EmitPrimTypeNode(PrimTypeNode *node);
  virtual std::string EmitPrimArrayTypeNode(PrimArrayTypeNode *node);
  virtual std::string EmitNewNode(NewNode *node);
  virtual std::string EmitClassProps(TreeNode *node);
  std::string EmitStructLiteralProps(StructLiteralNode* node);
  std::string EmitFuncScopeVarDecls(FunctionNode *node);
};

} // namespace maplefe
#endif
