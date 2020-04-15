/*
* Copyright (C) [2020] Futurewei Technologies, Inc. All rights reverved.
*
* OpenArkFE is licensed under the Mulan PSL v1.
* You can use this software according to the terms and conditions of the Mulan PSL v1.
* You may obtain a copy of Mulan PSL v1 at:
*
*  http://license.coscl.org.cn/MulanPSL
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
* FIT FOR A PARTICULAR PURPOSE.
* See the Mulan PSL v1 for more details.
*/

#include "vfy.h"
#include "ast.h"
#include "ast_module.h"
#include "ast_scope.h"
#include "massert.h"

///////////////////////////////////////////////////////////////////////////////
// The verification is done in the following rules.
// 1. Verifier works scope by scope.
// 2. It first works on the top scope.
// 3. In a scope, it works on the children trees sequentially.
// 4. when it verifies a child tree, it goes further into the child. It returns
//    until the verification of current child is done. So this is a
//    resursive process.
///////////////////////////////////////////////////////////////////////////////

Verifier::Verifier() {
  mCurrScope = NULL;
  mTempParent = NULL;
}

Verifier::~Verifier() {
}

void Verifier::Do() {
  VerifyGlobalScope();
  mLog.Dump();
}

// Each time when we enter a new scope, we need handle possible local
// variable declarations or type declaration. So it's different than
// VerifyTree().
//
// If you have a sharp eye, you can tell the Decl list of the scope is
// built progressively with verification of sub trees. This means
// only Decl-s before a subtree can be seen by it, which is reasonable
// for most languages.

void Verifier::VerifyGlobalScope() {
  mCurrScope = gModule.mRootScope;
  std::vector<ASTTree*>::iterator tree_it = gModule.mTrees.begin();
  for (; tree_it != gModule.mTrees.end(); tree_it++) {
    ASTTree *asttree = *tree_it;
    TreeNode *tree = asttree->mRootNode;
    // Step 1. Try to add decl.
    mCurrScope->TryAddDecl(tree);
    // Step 2. Try to add type.
    mCurrScope->TryAddType(tree);
    // Step 3. Verify the tree.
    VerifyTree(tree);
  }
}

// Before entering VerifyTree(), tree has been done with VerifyScope if it
// is a scope. It's caller's duty to assure this assumption.

void Verifier::VerifyTree(TreeNode *tree) {
#undef  NODEKIND
#define NODEKIND(K) if (tree->Is##K()) Verify##K(tree);
#include "ast_nk.def"
}

// This collect all decls and types in a whole scope. This is useful for
// some languages like C++/Java where a field or function has scope of
// the whole class body. Even the code before the field decl can still
// reference it.
void Verifier::CollectAllDeclsTypes(ASTScope *scope) {
  TreeNode *t = scope->GetTree();
  MASSERT(t->IsClass() && "right now we only support class node here");
  ClassNode *klass = (ClassNode*)t;

  // All fields, methods, local classes, local interfaces are decls.
  // All local classes/interfaces are types.
  for (unsigned i = 0; i < klass->GetFieldsNum(); i++) {
    IdentifierNode *in = klass->GetField(i);
    scope->AddDecl(in);
  }
  for (unsigned i = 0; i < klass->GetMethodsNum(); i++) {
    FunctionNode *n = klass->GetMethod(i);
    scope->AddDecl(n);
  }
  for (unsigned i = 0; i < klass->GetLocalClassesNum(); i++) {
    ClassNode *n = klass->GetLocalClass(i);
    scope->AddDecl(n);
    scope->AddType(n);
  }
  for (unsigned i = 0; i < klass->GetLocalInterfacesNum(); i++) {
    InterfaceNode *n = klass->GetLocalInterface(i);
    scope->AddDecl(n);
    scope->AddType(n);
  }
}

// This function has two jobs.
// 1. It looks into the Decl's of this scope and its ancestors' scopes,
//    to find a decl with the same name. If not found, the identifier
//    is undeclaraed.
// 2. Replace this node with the decl node, since they are the same node.
//    The abandoned one was created at the first time Parser saw it. At
//    that time parser has no idea what it is, and just give it a new
//    identifier node. So After verification, they should point to the
//    real one.
void Verifier::VerifyIdentifier(IdentifierNode *inode) {
  ASTScope *scope = mCurrScope;
  IdentifierNode *decl = NULL;
  while (scope) {
    if (decl = scope->FindDeclOf(inode))
      break;
    scope = scope->GetParent();
  }

  if (!decl) {
    mLog.MissDecl(inode);
  } else {
    // Replace the temp IdentifierNode with the found Decl.
    // Sometimes inode and decl are the same, which happens for the declaration statement.
    // We will verify decl statement as the others, so its inode is the same as decl.
    if (inode != decl) {
      mTempParent->ReplaceChild(inode, decl);
      //std::cout << "Replace " << inode << " with " << decl << std::endl;
    }
  }
}

void Verifier::VerifyDimension(DimensionNode *tree){
}

// Nothing needed for Attr
void Verifier::VerifyAttr(AttrNode *tree){
  return;
}

// Nothing needed for PrimType
void Verifier::VerifyPrimType(PrimTypeNode *tree){
  return;
}

void Verifier::VerifyVarList(VarListNode *vlnode){
  TreeNode *old_temp_parent = mTempParent;
  mTempParent = vlnode;
  for (unsigned i = 0; i < vlnode->GetNum(); i++) {
    IdentifierNode *n = vlnode->VarAtIndex(i);
    VerifyIdentifier(n);
  }
  mTempParent = old_temp_parent;
}

// Nothing needed for Literal
void Verifier::VerifyLiteral(LiteralNode *tree){
  return;
}

void Verifier::VerifyUnaOperator(UnaOperatorNode *tree){
}

void Verifier::VerifyBinOperator(BinOperatorNode *binop){
  TreeNode *old_temp_parent = mTempParent;
  mTempParent = binop;
  VerifyTree(binop->mOpndA);
  VerifyTree(binop->mOpndB);
  mTempParent = old_temp_parent;
}

void Verifier::VerifyTerOperator(TerOperatorNode *tree){
}

void Verifier::VerifyBlock(BlockNode *block){
  ASTScope *scope = gModule.NewScope(mCurrScope);
  mCurrScope = scope;
  scope->SetTree(block);

  for (unsigned i = 0; i < block->GetChildrenNum(); i++) {
    TreeNode *t = block->GetChildAtIndex(i);
    // Step 1. Try to add decl.
    scope->TryAddDecl(t);
    // Step 2. Try to add type.
    scope->TryAddType(t);
    // Step 3. Verify the t.
    VerifyTree(t);
  }
}

void Verifier::VerifyFunction(FunctionNode *tree){
}

/////////////////////////////////////////////////////////////////////////////////////
//                              ClassNode verification
// ClassNode is the most complicated. We decided to separate it into multiple
// functions to allow the language specific implementation more easilier.
/////////////////////////////////////////////////////////////////////////////////////

// This verification follows the most common rules of many languages.
// Each language can have its own implementation, and maybe reuse this implementation
// together with its own specific ones.
void Verifier::VerifyClassFields(ClassNode *klass) {
  // rule 1. No duplicated fields name with another decls.
  for (unsigned i = 0; i < klass->GetFieldsNum(); i++) {
    IdentifierNode *na = klass->GetField(i);
    bool hit_self = false;
    for (unsigned j = 0; j < mCurrScope->GetDeclNum(); j++) {
      TreeNode *nb = mCurrScope->GetDecl(j);
      if (na->GetName() == nb->GetName()) {
        if (nb->IsIdentifier()) {
          if (!hit_self)
            hit_self = true;
          else
            mLog.Duplicate("Field Decl Duplication! ", na, nb);
        } else {
          mLog.Duplicate("Field Decl Duplication! ", na, nb);
        }
      }
    }
  }
}

// Several things to verify.
// 1) Duplication decl. We dont check against field since they are already checked.
// 2) Verify the function body, which is complicated.
void Verifier::VerifyClassMethods(ClassNode *klass) {
  for (unsigned i = 0; i < klass->GetMethodsNum(); i++) {
    FunctionNode *method = klass->GetMethod(i);
    // step 1. verify the duplication
    bool hit_self = false;
    for (unsigned j = 0; j < mCurrScope->GetDeclNum(); j++) {
      TreeNode *nb = mCurrScope->GetDecl(j);
      // Fields have been checked. No need here.
      if (nb->IsIdentifier())
        continue;

      if (method->GetName() == nb->GetName()) {
        if (nb->IsFunction()) {
          if (!hit_self)
            hit_self = true;
          else
            mLog.Duplicate("Function Decl Duplication! ", method, nb);
        } else {
          mLog.Duplicate("Function Decl Duplication! ", method, nb);
        }
      }
    }

    // step 2. verify functioin.
    VerifyFunction(method);
  }
}

void Verifier::VerifyClassSuperClasses(ClassNode *klass) {
}

void Verifier::VerifyClassSuperInterfaces(ClassNode *klass) {
}

void Verifier::VerifyClass(ClassNode *klass){
  // Step 1. Create a new scope
  ASTScope *scope = gModule.NewScope(mCurrScope);
  mCurrScope = scope;
  scope->SetTree(klass);

  CollectAllDeclsTypes(mCurrScope);

  // Step 2. Verifiy Fields.
  VerifyClassFields(klass);

  // Step 3. Verifiy Methods.
  VerifyClassMethods(klass);

  // Step 4. Verifiy super classes.
  VerifyClassSuperClasses(klass);

  // Step 5. Verifiy super interfaces.
  VerifyClassSuperInterfaces(klass);
}

void Verifier::VerifyInterface(InterfaceNode *tree){
}

void Verifier::VerifyAnnotationType(AnnotationTypeNode *tree){
}

void Verifier::VerifyAnnotation(AnnotationNode *tree){
}

void Verifier::VerifyException(ExceptionNode *tree){
}

void Verifier::VerifyReturn(ReturnNode *tree){
}

void Verifier::VerifyCondBranch(CondBranchNode *tree){
}

void Verifier::VerifyBreak(BreakNode *tree){
}

void Verifier::VerifyForLoop(ForLoopNode *tree){
}

void Verifier::VerifyWhileLoop(WhileLoopNode *tree){
}

void Verifier::VerifyDoLoop(DoLoopNode *tree){
}

void Verifier::VerifySwitchLabel(SwitchLabelNode *tree){
}

void Verifier::VerifySwitchCase(SwitchCaseNode *tree){
}

void Verifier::VerifySwitch(SwitchNode *tree){
}

void Verifier::VerifyPass(PassNode *tree){
}
