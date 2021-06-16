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

#include <stack>
#include <set>
#include "ast_handler.h"
#include "ast_ti.h"

#define NOTYETIMPL(K)      { if (mTrace) { MNYI(K);      }}

namespace maplefe {

void TypeInfer::TypeInference() {
  if (mTrace) std::cout << "============== TypeInfer ==============" << std::endl;
  TypeInferVisitor visitor(mHandler, mTrace, true);
  visitor.SetUpdated(true);
  int count = 0;
  while (visitor.GetUpdated()) {
    count++;
    visitor.SetUpdated(false);
    visitor.Visit(mHandler->GetASTModule());
    if (count > 10) break;
  }
  if (mTrace) std::cout << "\n>>>>>> TypeInference() iterated " << count << " times\n" << std::endl;
}

TypeId TypeInferVisitor::MergeTypeId(TypeId tia,  TypeId tib) {
  if (tia == tib) {
    return tia;
  } else if (tib == TY_None) {
    return tia;
  } else if (tia == TY_None) {
    return tib;
  } else if (tib == TY_Object) {
    return tib;
  } else if (tia == TY_Object) {
    return tia;
  } else if ((tia == TY_Int && tib == TY_Long) ||
             (tib == TY_Int && tia == TY_Long)) {
    return TY_Long;
  } else if ((tia == TY_Int && tib == TY_Float) ||
             (tib == TY_Int && tia == TY_Float)) {
    return TY_Float;
  } else if ((tia == TY_Long && tib == TY_Float) ||
             (tib == TY_Long && tia == TY_Float)) {
    return TY_Double;
  }
  NOTYETIMPL("MergeTypeId()");
  return TY_None;
}

void TypeInferVisitor::UpdateTypeId(TreeNode *node, TypeId id) {
  if (id == TY_None) {
    return;
  }
  if (node->GetTypeId() != id) {
    if (node->GetTypeId() != TY_None) {
      if (mTrace) std::cout << "UpdateTypeId() new non TY_None: " << std::endl;
      id = MergeTypeId(node->GetTypeId(), id);
    }
    node->SetTypeId(id);
    mUpdated = true;
  }
}

DeclNode *TypeInferVisitor::VisitDeclNode(DeclNode *node) {
  (void) AstVisitor::VisitDeclNode(node);
  TreeNode *init = node->GetInit();
  if (init) {
    VisitTreeNode(init);
    UpdateTypeId(node, init->GetTypeId());
  }
  TreeNode *var = node->GetVar();
  UpdateTypeId(var, node->GetTypeId());
  return node;
}

LiteralNode *TypeInferVisitor::VisitLiteralNode(LiteralNode *node) {
  (void) AstVisitor::VisitLiteralNode(node);
  LitId id = node->GetData().mType;
  switch (id) {
    case LT_IntegerLiteral:
      UpdateTypeId(node, TY_Int);
      break;
    case LT_FPLiteral:
      UpdateTypeId(node, TY_Float);
      break;
    case LT_DoubleLiteral:
      UpdateTypeId(node, TY_Double);
      break;
    case LT_StringLiteral:
      UpdateTypeId(node, TY_String);
      break;
    default:
      break;
  }
  return node;
}

FieldNode *TypeInferVisitor::VisitFieldNode(FieldNode *node) {
  (void) AstVisitor::VisitFieldNode(node);
  TreeNode *upper = node->GetUpper();
  IdentifierNode *field = static_cast<IdentifierNode *>(node->GetField());
  TreeNode *decl = NULL;
  if (!upper) {
    decl = mHandler->FindDecl(field);
  } else {
    switch (upper->GetKind()) {
      case NK_Literal: {
        LiteralNode *ln = static_cast<LiteralNode *>(upper);
        // this.f
        if (ln->GetData().mType == LT_ThisLiteral) {
          decl = mHandler->FindDecl(field);
        }
        break;
      }
      default:
        break;
    }
  }
  if (decl) {
    UpdateTypeId(node, decl->GetTypeId());
  }
  return node;
}

TreeNode *TypeInferVisitor::VisitClassField(TreeNode *node) {
  if (node->IsIdentifier()) {
    IdentifierNode *id = static_cast<IdentifierNode *>(node);
    TreeNode *init = id->GetInit();
    if (init) {
      VisitTreeNode(init);
      UpdateTypeId(node, init->GetTypeId());
    }
  }
  return node;
}

IdentifierNode *TypeInferVisitor::VisitIdentifierNode(IdentifierNode *node) {
  (void) AstVisitor::VisitIdentifierNode(node);
  TreeNode *decl = mHandler->FindDecl(node);
  if (decl) {
    UpdateTypeId(node, decl->GetTypeId());
  }
  return node;
}

BinOperatorNode *TypeInferVisitor::VisitBinOperatorNode(BinOperatorNode *node) {
  (void) AstVisitor::VisitBinOperatorNode(node);
  OprId op = node->GetOprId();
  TreeNode *ta = node->GetOpndA();
  TreeNode *tb = node->GetOpndB();
  // modified operand
  TreeNode *mod = NULL;
  TypeId tia = ta->GetTypeId();
  TypeId tib = tb->GetTypeId();
  switch (op) {
    case OPR_EQ:
    case OPR_NE:
    case OPR_GT:
    case OPR_LT:
    case OPR_GE:
    case OPR_LE: {
      if (tia != TY_None && tib == TY_None) {
        UpdateTypeId(tb, tia);
        mod = tb;
      } else if (tia == TY_None && tib != TY_None) {
        UpdateTypeId(ta, tib);
        mod = ta;
      }
      UpdateTypeId(node, TY_Boolean);
      break;
    }
    case OPR_Assign:
    case OPR_AddAssign:
    case OPR_SubAssign:
    case OPR_MulAssign:
    case OPR_DivAssign:
    case OPR_ModAssign:
    case OPR_ShlAssign:
    case OPR_ShrAssign:
    case OPR_BandAssign:
    case OPR_BorAssign:
    case OPR_BxorAssign:
    case OPR_ZextAssign: {
      if (ta->GetTypeId() == TY_None) {
        UpdateTypeId(ta, tib);
        mod = ta;
      } else if (tb->GetTypeId() == TY_None) {
        UpdateTypeId(tb, tia);
        mod = tb;
      }
      UpdateTypeId(node, tib);
      break;
    }
    case OPR_Add:
    case OPR_Sub:
    case OPR_Mul:
    case OPR_Div:
    case OPR_Mod:
    case OPR_Band:
    case OPR_Bor:
    case OPR_Bxor:
    case OPR_Shl:
    case OPR_Shr:
    case OPR_Zext:
    case OPR_Land:
    case OPR_Lor: {
      if (tia != TY_None && tib == TY_None) {
        UpdateTypeId(tb, tia);
        mod = tb;
      } else if (tia == TY_None && tib != TY_None) {
        UpdateTypeId(ta, tib);
        mod = ta;
      }
      TypeId ti = MergeTypeId(tia, tib);
      UpdateTypeId(node, ti);
      break;
    }
    default: {
      NOTYETIMPL("VisitBinOperatorNode()");
      break;
    }
  }
  // if mod is an identifier, update its decl
  if (mod && mod->IsIdentifier()) {
    TreeNode *decl = mHandler->FindDecl(static_cast<IdentifierNode *>(mod));
    if (decl) {
      UpdateTypeId(decl, mod->GetTypeId());
    }
  }
  return node;
}

UnaOperatorNode *TypeInferVisitor::VisitUnaOperatorNode(UnaOperatorNode *node) {
  (void) AstVisitor::VisitUnaOperatorNode(node);
  OprId op = node->GetOprId();
  TreeNode *ta = node->GetOpnd();
  switch (op) {
    case OPR_Add:
    case OPR_Sub:
      UpdateTypeId(node, ta->GetTypeId());
      break;
    case OPR_Inc:
    case OPR_Dec:
      UpdateTypeId(ta, TY_Int);
      UpdateTypeId(node, TY_Int);
      break;
    case OPR_Bcomp:
      UpdateTypeId(ta, TY_Int);
      UpdateTypeId(node, TY_Int);
      break;
    case OPR_Not:
      UpdateTypeId(ta, TY_Boolean);
      UpdateTypeId(node, TY_Boolean);
      break;
    default: {
      NOTYETIMPL("VisitUnaOperatorNode()");
      break;
    }
  }
  return node;
}

FunctionNode *TypeInferVisitor::VisitFunctionNode(FunctionNode *node) {
  UpdateTypeId(node, TY_Function);
  (void) AstVisitor::VisitFunctionNode(node);
  return node;
}

LambdaNode *TypeInferVisitor::VisitLambdaNode(LambdaNode *node) {
  UpdateTypeId(node, TY_Function);
  (void) AstVisitor::VisitLambdaNode(node);
  return node;
}

ClassNode *TypeInferVisitor::VisitClassNode(ClassNode *node) {
  UpdateTypeId(node, TY_Class);
  for (unsigned i = 0; i < node->GetFieldsNum(); ++i) {
    TreeNode *t = node->GetField(i);
    (void) VisitClassField(t);
  }
  (void) AstVisitor::VisitClassNode(node);
  return node;
}

InterfaceNode *TypeInferVisitor::VisitInterfaceNode(InterfaceNode *node) {
  UpdateTypeId(node, TY_Class);
  for (unsigned i = 0; i < node->GetFieldsNum(); ++i) {
    TreeNode *t = node->GetFieldAtIndex(i);
    (void) VisitClassField(t);
  }
  (void) AstVisitor::VisitInterfaceNode(node);
  return node;
}

ReturnNode *TypeInferVisitor::VisitReturnNode(ReturnNode *node) {
  (void) AstVisitor::VisitReturnNode(node);
  if (node->GetResult()) {
    UpdateTypeId(node, node->GetResult()->GetTypeId());
  }
  return node;
}

}
