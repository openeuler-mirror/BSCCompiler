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

#ifndef __ASTEMITTER_HEADER__
#define __ASTEMITTER_HEADER__

#include "ast.h"
#include "ast_attr.h"
#include "ast_module.h"
#include "ast_type.h"

#include "gen_astdump.h"

namespace maplefe {

class AstEmitter {
public:
  using Precedence = char;

private:
  ASTModule *mASTModule;
  std::ostream *mOs;
  Precedence mPrecedence;

public:
  AstEmitter(ASTModule *m) : mASTModule(m), mOs(nullptr) {}

  void AstEmit(const char *title, std::ostream *os) {
    mOs = os;
    *mOs << "// [Beginning of AstEmitter: " << title << "\n// Filename: " << mASTModule->mFileName << "\n";
    for (auto it : mASTModule->mTrees)
      *mOs << AstEmitTreeNode(it->mRootNode);
    *mOs << "// End of AstEmitter]\n";
  }

  std::string Clean(std::string &s) {
    auto len = s.length();
    if(len >= 2 && s.substr(len - 2) == ";\n")
      return s.erase(len - 2);
    return s;
  }

  std::string AstEmitAnnotationNode(AnnotationNode *node);
  std::string AstEmitPackageNode(PackageNode *node);
  std::string AstEmitImportNode(ImportNode *node);
  std::string AstEmitUnaOperatorNode(UnaOperatorNode *node);
  std::string AstEmitBinOperatorNode(BinOperatorNode *node);
  std::string AstEmitTerOperatorNode(TerOperatorNode *node);
  std::string AstEmitBlockNode(BlockNode *node);
  std::string AstEmitNewNode(NewNode *node);
  std::string AstEmitDeleteNode(DeleteNode *node);
  std::string AstEmitDimensionNode(DimensionNode *node);
  std::string AstEmitIdentifierNode(IdentifierNode *node);
  std::string AstEmitDeclNode(DeclNode *node);
  std::string AstEmitAnnotationTypeNode(AnnotationTypeNode *node);
  std::string AstEmitCastNode(CastNode *node);
  std::string AstEmitParenthesisNode(ParenthesisNode *node);
  std::string AstEmitFieldNode(FieldNode *node);
  std::string AstEmitArrayElementNode(ArrayElementNode *node);
  std::string AstEmitArrayLiteralNode(ArrayLiteralNode *node);
  std::string AstEmitStructNode(StructNode *node);
  std::string AstEmitFieldLiteralNode(FieldLiteralNode *node);
  std::string AstEmitStructLiteralNode(StructLiteralNode *node);
  std::string AstEmitVarListNode(VarListNode *node);
  std::string AstEmitExprListNode(ExprListNode *node);
  std::string AstEmitLiteralNode(LiteralNode *node);
  std::string AstEmitThrowNode(ThrowNode *node);
  std::string AstEmitCatchNode(CatchNode *node);
  std::string AstEmitFinallyNode(FinallyNode *node);
  std::string AstEmitTryNode(TryNode *node);
  std::string AstEmitExceptionNode(ExceptionNode *node);
  std::string AstEmitReturnNode(ReturnNode *node);
  std::string AstEmitCondBranchNode(CondBranchNode *node);
  std::string AstEmitBreakNode(BreakNode *node);
  std::string AstEmitContinueNode(ContinueNode *node);
  std::string AstEmitForLoopNode(ForLoopNode *node);
  std::string AstEmitWhileLoopNode(WhileLoopNode *node);
  std::string AstEmitDoLoopNode(DoLoopNode *node);
  std::string AstEmitSwitchLabelNode(SwitchLabelNode *node);
  std::string AstEmitSwitchCaseNode(SwitchCaseNode *node);
  std::string AstEmitSwitchNode(SwitchNode *node);
  std::string AstEmitAssertNode(AssertNode *node);
  std::string AstEmitCallNode(CallNode *node);
  std::string AstEmitFunctionNode(FunctionNode *node);
  std::string AstEmitInterfaceNode(InterfaceNode *node);
  std::string AstEmitClassNode(ClassNode *node);
  std::string AstEmitPassNode(PassNode *node);
  std::string AstEmitLambdaNode(LambdaNode *node);
  std::string AstEmitInstanceOfNode(InstanceOfNode *node);
  std::string AstEmitTypeOfNode(TypeOfNode *node);
  std::string AstEmitAttrNode(AttrNode *node);
  std::string AstEmitUserTypeNode(UserTypeNode *node);
  std::string AstEmitPrimTypeNode(PrimTypeNode *node);
  std::string AstEmitPrimArrayTypeNode(PrimArrayTypeNode *node);

  std::string AstEmitTreeNode(TreeNode *node);

  static const char *GetEnumTypeId(TypeId k);
  //static const char *GetEnumSepId(SepId k);
  static const char *GetEnumOprId(OprId k, bool isUna, bool isPost);
  //static const char *GetEnumLitId(LitId k);
  //static const char *GetEnumAttrId(AttrId k);
  //static const char *GetEnumImportProperty(ImportProperty k);
  //static const char *GetEnumOperatorProperty(OperatorProperty k);
  static const char *GetEnumDeclProp(DeclProp k);
  //static const char *GetEnumStructProp(StructProp k);
  //static const char *GetEnumForLoopProp(ForLoopProp k);
  //static const char *GetEnumLambdaProperty(LambdaProperty k);

};

} // namespace maplefe
#endif
