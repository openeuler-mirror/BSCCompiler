/*
* Copyright (C) [2021-2022] Futurewei Technologies, Inc. All rights reverved.
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

#include <stack>
#include <set>
#include <algorithm>
#include "ast_handler.h"
#include "typetable.h"
#include "ast_info.h"
#include "ast_adj.h"
#include "ast_util.h"

namespace maplefe {

void AST_ADJ::AdjustAST() {
  MSGNOLOC0("============== AdjustAST ==============");
  ModuleNode *module = mHandler->GetASTModule();
  for(unsigned i = 0; i < module->GetTreesNum(); i++) {
    TreeNode *it = module->GetTree(i);
    it->SetParent(module);
  }

  // adjust ast
  AdjustASTVisitor adjust_visitor(mHandler, mFlags, true);
  adjust_visitor.Visit(module);
}

// set parent for some identifier's type
IdentifierNode *AdjustASTVisitor::VisitIdentifierNode(IdentifierNode *node) {
  (void) AstVisitor::VisitIdentifierNode(node);
  CheckAndRenameCppKeywords(node);

  TreeNode *type = node->GetType();
  if (type && type->IsUserType()) {
    type->SetParent(node);
  }
  return node;
}

ClassNode *AdjustASTVisitor::VisitClassNode(ClassNode *node) {
  (void) AstVisitor::VisitClassNode(node);
  CheckAndRenameCppKeywords(node);
  AssignPseudoName(node);
  // skip getting canonical type if not only fields
  if (node->GetMethodsNum() || node->GetSuperClassesNum() || node->GetSuperInterfacesNum() ||
      node->GetSuperClassesNum() || node->GetTypeParamsNum()) {
    return node;
  }

  TreeNode *parent = node->GetParent();
  TreeNode *newnode = mInfo->GetCanonicStructNode(node);
  if (newnode == node) {
    return node;
  }

  // create a TypeAlias for duplicated type if top level
  if (parent && parent->IsModule()) {
    newnode = (ClassNode*)(mInfo->CreateTypeAliasNode(newnode, node));
  }

  return (ClassNode*)newnode;
}

InterfaceNode *AdjustASTVisitor::VisitInterfaceNode(InterfaceNode *node) {
  (void) AstVisitor::VisitInterfaceNode(node);
  CheckAndRenameCppKeywords(node);
  // skip getting canonical type if not only fields
  if (node->GetMethodsNum() || node->GetSuperInterfacesNum()) {
    return node;
  }

  TreeNode *parent = node->GetParent();
  TreeNode *newnode = mInfo->GetCanonicStructNode(node);
  if (newnode == node) {
    return node;
  }

  // create a TypeAlias for duplicated type if top level
  if (parent && parent->IsModule()) {
    newnode = (InterfaceNode*)(mInfo->CreateTypeAliasNode(newnode, node));
  }
  return (InterfaceNode*)newnode;
}

StructLiteralNode *AdjustASTVisitor::VisitStructLiteralNode(StructLiteralNode *node) {
  (void) AstVisitor::VisitStructLiteralNode(node);

  unsigned size = node->GetFieldsNum();
  for (unsigned fid = 0; fid < size; fid++) {
    FieldLiteralNode *field = node->GetField(fid);
    TreeNode *name = field->GetFieldName();
    TreeNode *lit = field->GetLiteral();
    if (!name || !lit || !lit->IsLiteral()) {
      return node;
    }
  }

  TreeNode *newnode = mInfo->GetCanonicStructNode(node);
  if (newnode != node) {
    node->SetTypeIdx(newnode->GetTypeIdx());
  }

  return node;
}

StructNode *AdjustASTVisitor::VisitStructNode(StructNode *node) {
  (void) AstVisitor::VisitStructNode(node);
  CheckAndRenameCppKeywords(node);
  // skip getting canonical type for TypeAlias
  TreeNode *parent_orig = node->GetParent();
  TreeNode *p = parent_orig;
  while (p) {
    if (p->IsTypeAlias()) {
      return node;
    }
    p = p->GetParent();
  }

  // skip getting canonical type if not only fields
  if (node->GetMethodsNum() || node->GetSupersNum() || node->GetTypeParamsNum()) {
    return node;
  }

  TreeNode *newnode = mInfo->GetCanonicStructNode(node);

  // if returned itself that means it should be added to the moudule if not yet
  if (newnode == node) {
    ModuleNode *module = mHandler->GetASTModule();
    bool found = false;
    for (unsigned i = 0; i < module->GetTreesNum(); ++i) {
      if (newnode == module->GetTree(i)) {
        found = true;
        break;
      }
    }
    if (!found) {
      module->AddTreeFront(newnode);
    }
  }

  // create a TypeAlias for duplicated type if top level
  // except newly added anonymous type which has updated parent
  TreeNode *parent = node->GetParent();
  if (newnode != node && parent_orig && parent && parent == parent_orig && parent->IsModule()) {
    newnode = mInfo->CreateTypeAliasNode(newnode, node);
  }

  // for non top level tree node, replace it with a UserType node
  if (!parent_orig || !parent_orig->IsModule()) {
    if (newnode->GetStrIdx()) {
      // for anonymous class, check it is given a name
      // not skipped due to type parameters
      newnode = mInfo->CreateUserTypeNode(newnode->GetStrIdx());
    }
  }

  return (StructNode*)newnode;
}

// convert prim array node to array type node
PrimArrayTypeNode *AdjustASTVisitor::VisitPrimArrayTypeNode(PrimArrayTypeNode *node) {
  (void) AstVisitor::VisitPrimArrayTypeNode(node);
  ArrayTypeNode *arr = mHandler->NewTreeNode<ArrayTypeNode>();
  DimensionNode *dim = node->GetDims();
  arr->SetDims(dim);
  arr->SetElemType(node->GetPrim());

  return (PrimArrayTypeNode *)arr;
}

// set UserTypeNode's mStrIdx to be its mId's
UserTypeNode *AdjustASTVisitor::VisitUserTypeNode(UserTypeNode *node) {
  (void) AstVisitor::VisitUserTypeNode(node);
  TreeNode *id = node->GetId();
  if (id) {
    node->SetStrIdx(id->GetStrIdx());
  }

  // use array type node
  DimensionNode *dim = node->GetDims();
  bool isarr = (dim != NULL);
  // element type
  TreeNode *etype = NULL;

  if (isarr) {
    etype = node;
  } else if (id && id->IsIdentifier()) {
    IdentifierNode *idnode = static_cast<IdentifierNode *>(id);
    isarr = (idnode->GetStrIdx() == gStringPool.GetStrIdx("Array"));
    if (isarr) {
      if (unsigned s = node->GetTypeGenericsNum()) {
        if (s == 1) {
          etype = node->GetTypeGeneric(0);
        } else {
          NOTYETIMPL("array usertype with multiple generic type");
        }
      }
    }
  }

  if (isarr) {
    ArrayTypeNode *arr = mHandler->NewTreeNode<ArrayTypeNode>();
    arr->SetDims(dim);
    arr->SetElemType(etype);
    node = (UserTypeNode *)arr;
  }

  return node;
}

// set TypeAliasNode's mStrIdx to be its mId's
TypeAliasNode *AdjustASTVisitor::VisitTypeAliasNode(TypeAliasNode *node) {
  (void) AstVisitor::VisitTypeAliasNode(node);
  UserTypeNode *id = node->GetId();
  if (id && id->GetId()) {
    node->SetStrIdx(id->GetId()->GetStrIdx());
  }
  return node;
}

LiteralNode *AdjustASTVisitor::VisitLiteralNode(LiteralNode *node) {
  (void) AstVisitor::VisitLiteralNode(node);
  if (node->IsThis()) {
    unsigned stridx = gStringPool.GetStrIdx("this");
    IdentifierNode *id = mInfo->CreateIdentifierNode(stridx);
    TreeNode *type = node->GetType();
    if (type) {
      id->SetType(type);
      id->SetTypeId(type->GetTypeId());
      id->SetTypeIdx(type->GetTypeIdx());
    }
    node = (LiteralNode *)id;
  }
  return node;
}

UnaOperatorNode *AdjustASTVisitor::VisitUnaOperatorNode(UnaOperatorNode *node) {
  (void) AstVisitor::VisitUnaOperatorNode(node);
  TreeNode *opnd = node->GetOpnd();
  OprId op = node->GetOprId();
  if (op == OPR_Plus || op == OPR_Minus) {
    if (opnd->IsLiteral()) {
      LiteralNode *lit = static_cast<LiteralNode *>(opnd);
      if (lit->GetData().mType == LT_StringLiteral) {
        unsigned stridx = lit->GetData().mData.mStrIdx;
        std::string str = gStringPool.GetStringFromStrIdx(stridx);
        double d = std::stod(str);
        if (op == OPR_Minus) {
          d = -d;
        }
        long l = (long)d;
        LitData data;
        if (d == l) {
          data.mType = LT_IntegerLiteral;
          data.mData.mInt = l;
        } else {
          data.mType = LT_DoubleLiteral;
          data.mData.mDouble = d;
        }
        LiteralNode *n = mHandler->NewTreeNode<LiteralNode>();
        n->SetData(data);
        n->SetParent(node->GetParent());
        return (UnaOperatorNode*)n;
      }
    }
  }
  return node;
}

FunctionNode *AdjustASTVisitor::VisitFunctionNode(FunctionNode *node) {
  (void) AstVisitor::VisitFunctionNode(node);
  CheckAndRenameCppKeywords(node);

  TreeNode *type = node->GetType();
  if (type && type->IsUserType()) {
    type->SetParent(node);
  }

  // Refine function TypeParames
  for(unsigned i = 0; i < node->GetTypeParamsNum(); i++) {
    type = node->GetTypeParamAtIndex(i);
    if (type->IsIdentifier()) {
      IdentifierNode *inode = static_cast<IdentifierNode *>(type);
      TreeNode *newtype = mInfo->CreateUserTypeNode(inode);
      node->SetTypeParamAtIndex(i, newtype);
      newtype->SetParent(node);
    }
  }
  return node;
}

// move init from identifier to decl
// copy stridx to decl
DeclNode *AdjustASTVisitor::VisitDeclNode(DeclNode *node) {
  TreeNode *var = node->GetVar();
  if (var->IsIdentifier()) {
    IdentifierNode *inode = static_cast<IdentifierNode *>(var);
    (void) AstVisitor::VisitIdentifierNode(inode);

    // copy stridx from Identifier to Decl
    unsigned stridx = inode->GetStrIdx();
    if (stridx) {
      node->SetStrIdx(stridx);
      mUpdated = true;
    }

    // move init from Identifier to Decl
    TreeNode *init = inode->GetInit();
    if (init) {
      node->SetInit(init);
      inode->ClearInit();
      mUpdated = true;
    }
  } else if (var->IsBindingPattern()) {
    BindingPatternNode *bind = static_cast<BindingPatternNode *>(var);
    VisitBindingPatternNode(bind);
  } else {
    NOTYETIMPL("decl not idenfier or bind pattern");
  }

  // reorg before processing init
  (void) AstVisitor::VisitDeclNode(node);
  return node;
}

ImportNode *AdjustASTVisitor::VisitImportNode(ImportNode *node) {
  (void) AstVisitor::VisitImportNode(node);

  return node;
}

// if-then-else : use BlockNode for then and else bodies
// split export decl and body
// export {func add(y)}  ==> export {add}; func add(y)
ExportNode *AdjustASTVisitor::VisitExportNode(ExportNode *node) {
  (void) AstVisitor::VisitExportNode(node);
  TreeNode *parent = node->GetParent();
  if (!parent || parent->IsNamespace()) {
    // Export declarations are not permitted in a namespace
    return node;
  }
  if (node->GetPairsNum() == 1) {
    XXportAsPairNode *p = node->GetPair(0);
    TreeNode *bfnode = p->GetBefore();
    if (bfnode) {
      if (!bfnode->IsIdentifier()) {
        switch (bfnode->GetKind()) {
          case NK_Function: {
            FunctionNode *func = static_cast<FunctionNode *>(bfnode);
            IdentifierNode *n = mInfo->CreateIdentifierNode(func->GetStrIdx());
            // update p
            p->SetBefore(n);
            n->SetParent(p);
            mUpdated = true;
            // insert func into AST after node
            if (parent->IsBlock()) {
              static_cast<BlockNode *>(parent)->InsertStmtAfter(func, node);
            } else if (parent->IsModule()) {
              static_cast<ModuleNode *>(parent)->InsertAfter(func, node);
            }
            // cp annotation from node to func
            for (unsigned i = 0; i < node->GetAnnotationsNum(); i++) {
              func->AddAnnotation(node->GetAnnotationAtIndex(i));
            }
            node->ClearAnnotation();
            break;
          }
          case NK_Class: {
            ClassNode *classnode = static_cast<ClassNode *>(bfnode);
            IdentifierNode *n = mInfo->CreateIdentifierNode(classnode->GetStrIdx());
            // update p
            p->SetBefore(n);
            n->SetParent(p);
            mUpdated = true;
            // insert classnode into AST after node
            if (parent->IsBlock()) {
              static_cast<BlockNode *>(parent)->InsertStmtAfter(classnode, node);
            } else if (parent->IsModule()) {
              static_cast<ModuleNode *>(parent)->InsertAfter(classnode, node);
            }
            // cp annotation from node to classnode
            for (unsigned i = 0; i < node->GetAnnotationsNum(); i++) {
              classnode->AddAnnotation(node->GetAnnotationAtIndex(i));
            }
            node->ClearAnnotation();
            break;
          }
          default:
            NOTYETIMPL("VisitExportNode neither function nor class");
            break;
        }
      }
    }
  }
  return node;
}

NamespaceNode *AdjustASTVisitor::VisitNamespaceNode(NamespaceNode *node) {
  (void) AstVisitor::VisitNamespaceNode(node);
  TreeNode *id = node->GetId();
  if (id) {
    // for namespace with nested id, split into namespaces
    if (id->IsField()) {
      FieldNode *fld = static_cast<FieldNode *>(id);
      TreeNode *upper = fld->GetUpper();
      TreeNode *field = fld->GetField();
      // rename node with upper
      node->SetId(upper);

      NamespaceNode *ns = mHandler->NewTreeNode<NamespaceNode>();
      // name ns with field
      ns->SetId(field);
      // move elements of node to ns
      for (unsigned i = 0; i < node->GetElementsNum(); i++) {
        ns->AddElement(node->GetElementAtIndex(i));
      }
      node->Release();
      // add ns as element of node
      node->AddElement(ns);

      // recursive if needed
      if (!upper->IsIdentifier()) {
        node = VisitNamespaceNode(static_cast<NamespaceNode *>(node));
      }
    }
  }
  return node;
}

CondBranchNode *AdjustASTVisitor::VisitCondBranchNode(CondBranchNode *node) {
  TreeNode *tn = VisitTreeNode(node->GetCond());
  tn = VisitTreeNode(node->GetTrueBranch());
  if (tn && !tn->IsBlock()) {
    BlockNode *blk = mHandler->NewTreeNode<BlockNode>();
    blk->AddChild(tn);
    node->SetTrueBranch(blk);
    mUpdated = true;
  }
  tn = VisitTreeNode(node->GetFalseBranch());
  if (tn && !tn->IsBlock()) {
    BlockNode *blk = mHandler->NewTreeNode<BlockNode>();
    blk->AddChild(tn);
    node->SetFalseBranch(blk);
    mUpdated = true;
  }
  return node;
}

// for : use BlockNode for body
ForLoopNode *AdjustASTVisitor::VisitForLoopNode(ForLoopNode *node) {
  (void) AstVisitor::VisitForLoopNode(node);
  TreeNode *tn = node->GetBody();
  if (tn && !tn->IsBlock()) {
    BlockNode *blk = mHandler->NewTreeNode<BlockNode>();
    blk->AddChild(tn);
    node->SetBody(blk);
    mUpdated = true;
  }
  return node;
}

static unsigned uniq_number = 1;

// lamda : create a FunctionNode for it
//         use BlockNode for body, add a ReturnNode
LambdaNode *AdjustASTVisitor::VisitLambdaNode(LambdaNode *node) {
  FunctionNode *func = mHandler->NewTreeNode<FunctionNode>();

  // func name
  std::string str("__lambda_");
  str += std::to_string(uniq_number++);
  str += "__";
  unsigned stridx = gStringPool.GetStrIdx(str);
  IdentifierNode *name = mInfo->CreateIdentifierNode(stridx);
  func->SetStrIdx(stridx);
  func->SetFuncName(name);
  mInfo->AddFromLambda(func->GetNodeId());

  // func parameters
  for (int i = 0; i < node->GetParamsNum(); i++) {
    func->AddParam(node->GetParam(i));
  }

  // func Attributes
  for (int i = 0; i < node->GetAttrsNum(); i++) {
    func->AddAttr(node->GetAttrAtIndex(i));
  }

  // func type parameters
  for (int i = 0; i < node->GetTypeParamsNum(); i++) {
    func->AddTypeParam(node->GetTypeParamAtIndex(i));
  }

  // func body
  TreeNode *tn = VisitTreeNode(node->GetBody());
  if (tn) {
    if (tn->IsBlock()) {
      func->SetBody(static_cast<BlockNode*>(tn));
      func->SetType(node->GetType());
    } else {
      BlockNode *blk = mHandler->NewTreeNode<BlockNode>();
      ReturnNode *ret = mHandler->NewTreeNode<ReturnNode>();
      ret->SetResult(tn);
      blk->AddChild(ret);

      func->SetBody(blk);
    }
  }

  // func return type
  if (node->GetType()) {
    func->SetType(node->GetType());
  }

  mUpdated = true;
  // note: the following conversion is only for the visitor to notice the node is updated
  return (LambdaNode*)func;
}

void AdjustASTVisitor::CheckAndRenameCppKeywords(TreeNode *node) {
  if (!mHandler->IsTS() || node->GetStrIdx() == 0) {
    return;
  }

  unsigned stridx = node->GetStrIdx();
  if (mRenameMap.find(stridx) != mRenameMap.end()) {
    node->SetStrIdx(mRenameMap[stridx]);
    return;
  }

  if (mUtil->IsCppKeyWord(stridx)) {
    std::string name = gStringPool.GetStringFromStrIdx(stridx);
    unsigned newidx = gStringPool.GetStrIdx(name + RENAMINGSUFFIX);
    node->SetStrIdx(newidx);
    mRenameMap[stridx] = newidx;
  }
}

void AdjustASTVisitor::AssignPseudoName(TreeNode *node) {
  if (!mHandler->IsTS() || node->GetStrIdx() != 0) {
    return;
  }
  static int pseudo = 0;
  // Set a pseudo name
  unsigned newidx = gStringPool.GetStrIdx("__Pseudo_" + std::to_string(++pseudo));
  node->SetStrIdx(newidx);
}

}
