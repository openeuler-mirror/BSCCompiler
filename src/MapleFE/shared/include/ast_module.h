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

#ifndef __AST_MODULE_H__
#define __AST_MODULE_H__

#include <vector>

#include "ast_scope.h"
#include "container.h"

namespace maplefe {

enum SrcLang {
  SrcLangUnknown,
  SrcLangJava,
  SrcLangJavaScript,
  SrcLangTypeScript
};

// The module is a member of class Parser.
class ModuleNode : public TreeNode {
public:
  const char              *mFileName;
  PackageNode             *mPackage;
  SmallVector<ImportNode*> mImports;
public:
  SmallVector<TreeNode*> mTrees;    // All trees in the module.
  ASTScope              *mRootScope; // the scope corresponding to a module. All other scopes
                                     // are children of mRootScope.
  ASTScopePool           mScopePool; // All the scopes are store in this pool. It also contains
                                     // a vector of ASTScope pointer for traversal.
  SrcLang                mSrcLang;
public:
  ModuleNode();
  ~ModuleNode();

  void        SetFileName(const char *f) {mFileName = f;}
  const char *GetFileName() {return mFileName;}

  void         SetPackage(PackageNode *p);
  PackageNode *GetPackage() {return mPackage;};

  void    SetSrcLang(SrcLang l);
  SrcLang GetSrcLang();
  std::string GetSrcLangString();

  unsigned    GetImportsNum()       {return mImports.GetNum();}
  ImportNode* GetImport(unsigned i) {return mImports.ValueAtIndex(i);}
  void        SetImport(unsigned i, ImportNode* n) {*(mImports.RefAtIndex(i)) = n;}
  void        AddImport(ImportNode *imp)           {mImports.PushBack(imp);}

  unsigned  GetTreesNum()       {return mTrees.GetNum();}
  TreeNode* GetTree(unsigned i) {return mTrees.ValueAtIndex(i);}
  void SetTree(unsigned i, TreeNode* t)  {*(mTrees.RefAtIndex(i)) = t;}
  void AddTree(TreeNode* t)     {mTrees.PushBack(t); }

  ASTScope* GetRootScope()            {return mRootScope;}
  void      SetRootScope(ASTScope *s) {mRootScope = s;}

  ASTScopePool& GetScopePool()        {return mScopePool;}
  void          SetScopePool(ASTScopePool &s) {mScopePool = s;}

  ASTScope* NewScope(ASTScope *p);

  void Dump();
};

// Assume currently only one global module is being processed.
extern ModuleNode gModule;

}
#endif
