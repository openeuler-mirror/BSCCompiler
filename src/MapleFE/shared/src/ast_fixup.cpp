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

#include <filesystem>
#include "ast_fixup.h"
#include "stringpool.h"

namespace maplefe {

bool FixUpVisitor::FixUp() {
  Visit(mASTModule);
  return mUpdated;
}

// Fix up mOprId of a UnaOperatorNode
UnaOperatorNode *FixUpVisitor::VisitUnaOperatorNode(UnaOperatorNode *node) {
  switch(node->GetOprId()) {
    case OPR_Add:
      node->SetOprId(OPR_Plus);
      mUpdated = true;
      break;
    case OPR_Sub:
      node->SetOprId(OPR_Minus);
      mUpdated = true;
    case OPR_Minus:
      if (TreeNode *n = node->GetOpnd(); n && n->IsLiteral()) {
        LiteralNode *lit = static_cast<LiteralNode *>(n);
        LitData data = lit->GetData();
        switch(data.mType) {
          case LT_IntegerLiteral:
            data.mData.mInt = -data.mData.mInt;
            break;
          case LT_FPLiteral:
            data.mData.mFloat = -data.mData.mFloat;
            break;
          case LT_DoubleLiteral:
            data.mData.mDouble = -data.mData.mDouble;
            break;
          default:
            goto skip;
        }
        lit->SetData(data);
        lit->SetParent(node->GetParent());
        mUpdated = true;
        return (UnaOperatorNode *)lit;
      }
    skip:
      break;
    case OPR_Inc:
      if(!node->IsPost()) {
        node->SetOprId(OPR_PreInc);
        mUpdated = true;
      }
      break;
    case OPR_Dec:
      if(!node->IsPost()) {
        node->SetOprId(OPR_PreDec);
        mUpdated = true;
      }
  }
  return AstVisitor::VisitUnaOperatorNode(node);
}

// Fix up the name string of a UserTypeNode
// Fix up literal boolean 'true' or 'false' as a type
UserTypeNode *FixUpVisitor::VisitUserTypeNode(UserTypeNode *node) {
  auto id = node->GetId();

  // Java FE 'java2mpl' needs this
  if(id)
    if(auto n = id->GetStrIdx())
      if(node->GetStrIdx() != n) {
        node->SetStrIdx(n);
        mUpdated = true;
      }

  if(id && id->IsIdentifier()) {
    auto n = id->GetStrIdx();
    auto true_id = gStringPool.GetStrIdx("true");
    if(n == true_id || n == gStringPool.GetStrIdx("false")) {
      if(node->GetType() == UT_Regular
          && node->GetDims() == nullptr
          && node->GetUnionInterTypesNum() == 0
          && node->GetTypeGenericsNum() == 0
          && node->GetAttrsNum() == 0
          && node->GetAsTypesNum() == 0) {
        mUpdated = true;
        LitData data;
        data.mType = LT_BooleanLiteral;
        data.mData.mBool = n == true_id;
        LiteralNode *lit = new (gTreePool.NewTreeNode(sizeof(LiteralNode))) LiteralNode(data);
        return (UserTypeNode*)lit;
      }
    }
  }
  return AstVisitor::VisitUserTypeNode(node);
}

// Fix up literal 'true' or 'false'
IdentifierNode *FixUpVisitor::VisitIdentifierNode(IdentifierNode *node) {
  auto p = node->GetParent();
  if(p && node->GetInit() == nullptr &&
      (p->IsFieldLiteral() || p->IsTerOperator() || p->IsIdentifier() || p->IsBinOperator())) {
    if(auto n = node->GetStrIdx()) {
      auto true_id = gStringPool.GetStrIdx("true");
      if(n == true_id || n == gStringPool.GetStrIdx("false")) {
        mUpdated = true;
        LitData data;
        data.mType = LT_BooleanLiteral;
        data.mData.mBool = n == true_id;
        LiteralNode *lit = new (gTreePool.NewTreeNode(sizeof(LiteralNode))) LiteralNode(data);
        return (IdentifierNode*)lit;
      }
    }
  }
  return AstVisitor::VisitIdentifierNode(node);
}

// Fix up the filename of a ModuleNode
ModuleNode *FixUpVisitor::VisitModuleNode(ModuleNode *node) {
  const char* filename = node->GetFilename();
  std::filesystem::path orig = filename;
  std::filesystem::path uniq;
  try {
    uniq = std::filesystem::canonical(orig);
  }
  catch(std::filesystem::filesystem_error const& ex) {
    // Use orig if std::filesystem::filesystem_error is thrown
    uniq = orig;
  }
  std::string p = uniq.string();
  if(p != filename) {
    const char *res = gStringPool.FindString(p.c_str());
    node->SetFilename(res);
    mUpdated = true;
  }
  return AstVisitor::VisitModuleNode(node);;
}

// Fux up a PassNode for tagged template literal
PassNode *FixUpVisitor::VisitPassNode(PassNode *node) {
  AstVisitor::VisitPassNode(node);
  unsigned num = node->GetChildrenNum();
  if (num == 2) {
    TreeNode *child0 = node->GetChild(0);
    TreeNode *child1 = node->GetChild(1);
    if (child0 && child1 && child1->IsTemplateLiteral()) {
      CallNode *call = new (gTreePool.NewTreeNode(sizeof(CallNode))) CallNode();
      call->SetMethod(child0);
      call->SetTaggedTemplate(static_cast<TemplateLiteralNode *>(child1));
      return (PassNode*)call;
    }
  }
  return node;
}

}
