/*
* Copyright (C) [2020] Futurewei Technologies, Inc. All rights reverved.
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

#include "ast_module.h"
#include "ast.h"

namespace maplefe {

ModuleNode *gModule;

ModuleNode::ModuleNode() : TreeNode(NK_Module), mPackage(NULL), mSrcLang(SrcLangUnknown) {
  mRootScope = mScopePool.NewScope(NULL);
  mRootScope->SetTree(this);
}

ModuleNode::~ModuleNode() {
  mTrees.Release();
  mImports.Release();
}

// AFAIK, all languages allow only one package name if it allows.
void ModuleNode::SetPackage(PackageNode *p) {
  MASSERT(!mPackage);
  mPackage = p;
}

void ModuleNode::SetSrcLang(SrcLang l) {
  mSrcLang = l;
}

SrcLang ModuleNode::GetSrcLang() {
  return mSrcLang;
}

std::string ModuleNode::GetSrcLangString() {
  switch (mSrcLang) {
    case SrcLangJava: return "Java";
    case SrcLangTypeScript: return "TypeScript";
    case SrcLangJavaScript: return "JavaScript";
    default: break;
  }
  return "Unknown";
}
// Return a new scope newly created.
// Set the parent<->child relation between it and p.
ASTScope* ModuleNode::NewScope(ASTScope *p) {
  ASTScope *newscope = mScopePool.NewScope(p);
  return newscope;
}

void ModuleNode::Dump() {
  std::cout << "============= Module ===========" << std::endl;
  for (unsigned i = 0; i < mTrees.GetNum(); i++) {
    TreeNode *tree = GetTree(i);
    DUMP0("== Sub Tree ==");
    tree->Dump(0);
    DUMP_RETURN();
  }
}
}
