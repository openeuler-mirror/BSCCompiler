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

#ifndef __AST_TYPE_INFERENCE_HEADER__
#define __AST_TYPE_INFERENCE_HEADER__

#include <stack>
#include <utility>
#include "ast_module.h"
#include "ast.h"
#include "ast_type.h"
#include "gen_astvisitor.h"
#include "ast_common.h"

namespace maplefe {

class Module_Handler;

class TypeInfer {
 private:
  Module_Handler *mHandler;
  unsigned        mFlags;

 public:
  explicit TypeInfer(Module_Handler *h, unsigned f) : mHandler(h), mFlags(f) {}
  ~TypeInfer() {}

  void TypeInference();
  void CheckType();
};

class BuildIdNodeToDeclVisitor : public AstVisitor {
  Module_Handler *mHandler;

  public:
  explicit BuildIdNodeToDeclVisitor(Module_Handler *h, unsigned f, bool base = false)
    : AstVisitor((f & FLG_trace_1) && base), mHandler(h) {}
  ~BuildIdNodeToDeclVisitor() = default;

  IdentifierNode *VisitIdentifierNode(IdentifierNode *node);
};

class BuildIdDirectFieldVisitor : public AstVisitor {
  Module_Handler *mHandler;
  unsigned        mFlags;

  public:
  explicit BuildIdDirectFieldVisitor(Module_Handler *h, unsigned f, bool base = false)
    : AstVisitor((f & FLG_trace_1) && base), mHandler(h), mFlags(f) {}
  ~BuildIdDirectFieldVisitor() = default;

  TreeNode *GetParentVarClass(TreeNode *node);
  Module_Handler *GetHandler(TreeNode *node);

  FieldNode *VisitFieldNode(FieldNode *node);
  FieldLiteralNode *VisitFieldLiteralNode(FieldLiteralNode *node);
  ArrayElementNode *VisitArrayElementNode(ArrayElementNode *node);
  void Dump();
};

class TypeInferBaseVisitor : public AstVisitor {
 public:
  explicit TypeInferBaseVisitor(unsigned f, bool base = false)
    : AstVisitor((f & FLG_trace_1) && base) {}
  ~TypeInferBaseVisitor() = default;

#undef  NODEKIND
#define NODEKIND(K) virtual K##Node *Visit##K##Node(K##Node *node) { \
  (void) AstVisitor::Visit##K##Node(node); \
  return node; \
}
#include "ast_nk.def"
};

class ChangeTypeIdxVisitor : public AstVisitor {
 private:
  Module_Handler *mHandler;
  unsigned        mStrIdx;
  unsigned        mTypeIdx;

 public:
  explicit ChangeTypeIdxVisitor(Module_Handler *h, unsigned f, bool base = false)
    : AstVisitor((f & FLG_trace_1) && base), mHandler(h) {}
  ~ChangeTypeIdxVisitor() = default;

  void Setup(unsigned stridx, unsigned tidx) { mStrIdx = stridx; mTypeIdx = tidx;}

  IdentifierNode *VisitIdentifierNode(IdentifierNode *node);
};

class TypeInferVisitor : public TypeInferBaseVisitor {
 private:
  Module_Handler *mHandler;
  unsigned        mFlags;
  bool            mUpdated;
  AST_INFO       *mInfo;
  AST_XXport     *mXXport;
  AstOpt         *mAstOpt;

  ChangeTypeIdxVisitor *mChangeTypeIdxVisitor;

  std::unordered_map<unsigned, std::unordered_set<TreeNode *>> mParam2ArgArrayDeclMap;;

  // func nodeid to typeidx
  std::unordered_map<unsigned, unsigned> mFuncIsNodeMap;;
  std::unordered_map<unsigned, std::unordered_set<unsigned>> mCbFuncIsDone;

 public:
  explicit TypeInferVisitor(Module_Handler *h, unsigned f, bool base = false)
    : TypeInferBaseVisitor(f, base), mHandler(h), mFlags(f) {
      mChangeTypeIdxVisitor = new ChangeTypeIdxVisitor(h, f, true);
      mInfo = h->GetINFO();
      mXXport = h->GetASTXXport();
      mAstOpt = h->GetAstOpt();
    }

  ~TypeInferVisitor() = default;

  bool IsPrimTypeId(TypeId tid);

  bool GetUpdated() {return mUpdated;}
  void SetUpdated(bool b = true) {mUpdated = b;}

  void SetTypeId(TreeNode *node, TypeId tid);
  void SetTypeId(TreeNode *node1, TreeNode *node2);
  void UpdateTypeId(TreeNode *node, TypeId tid);
  void UpdateTypeId(TreeNode *node1, TreeNode *node2);

  void SetTypeIdx(TreeNode *node, unsigned tidx);
  void SetTypeIdx(TreeNode *node1, TreeNode *node2);
  void UpdateTypeIdx(TreeNode *node, unsigned tidx);
  void UpdateTypeIdx(TreeNode *node1, TreeNode *node2);

  void UpdateFuncRetTypeId(FunctionNode *node, TypeId tid, unsigned tidx);
  void UpdateTypeUseNode(TreeNode *target, TreeNode *input);
  void UpdateArgArrayDecls(unsigned nid, TypeId tid);
  void UpdateArrayElemTypeIdMap(TreeNode *node, TypeId tid);
  bool UpdateVarTypeWithInit(TreeNode *var, TreeNode *init);
  TypeId GetArrayElemTypeId(TreeNode *node);

  TypeId MergeTypeId(TypeId tia, TypeId tib);
  unsigned MergeTypeIdx(unsigned tia, unsigned tib);

  bool IsArray(TreeNode *node);
  // refer to shared/include/supported_types.def
  bool IsPrimTypeIdx(unsigned tidx) { return tidx > 0 && tidx < TY_Void; }

  PrimTypeNode *GetOrClonePrimTypeNode(PrimTypeNode *node, TypeId tid);

  TreeNode *VisitClassField(TreeNode *node);

  ArrayElementNode *VisitArrayElementNode(ArrayElementNode *node);
  ArrayLiteralNode *VisitArrayLiteralNode(ArrayLiteralNode *node);
  BinOperatorNode *VisitBinOperatorNode(BinOperatorNode *node);
  CallNode *VisitCallNode(CallNode *node);
  ClassNode *VisitClassNode(ClassNode *node);
  CondBranchNode *VisitCondBranchNode(CondBranchNode *node);
  DeclNode *VisitDeclNode(DeclNode *node);
  ExportNode *VisitExportNode(ExportNode *node);
  FieldLiteralNode *VisitFieldLiteralNode(FieldLiteralNode *node);
  FieldNode *VisitFieldNode(FieldNode *node);
  ForLoopNode *VisitForLoopNode(ForLoopNode *node);
  FunctionNode *VisitFunctionNode(FunctionNode *node);
  IdentifierNode *VisitIdentifierNode(IdentifierNode *node);
  ImportNode *VisitImportNode(ImportNode *node);
  InterfaceNode *VisitInterfaceNode(InterfaceNode *node);
  IsNode *VisitIsNode(IsNode *node);
  LambdaNode *VisitLambdaNode(LambdaNode *node);
  LiteralNode *VisitLiteralNode(LiteralNode *node);
  NewNode *VisitNewNode(NewNode *node);
  ReturnNode *VisitReturnNode(ReturnNode *node);
  StructLiteralNode *VisitStructLiteralNode(StructLiteralNode *node);
  StructNode *VisitStructNode(StructNode *node);
  TemplateLiteralNode *VisitTemplateLiteralNode(TemplateLiteralNode *node);
  TerOperatorNode *VisitTerOperatorNode(TerOperatorNode *node);
  TypeAliasNode *VisitTypeAliasNode(TypeAliasNode *node);
  TypeOfNode *VisitTypeOfNode(TypeOfNode *node);
  UnaOperatorNode *VisitUnaOperatorNode(UnaOperatorNode *node);
  UserTypeNode *VisitUserTypeNode(UserTypeNode *node);
};

class ShareUTVisitor : public AstVisitor {
 private:
  std::stack<ASTScope *> mScopeStack;

 public:
  explicit ShareUTVisitor(Module_Handler *h, unsigned f, bool base = false)
    : AstVisitor((f & FLG_trace_1) && base) {}
  ~ShareUTVisitor() = default;

  void Push(ASTScope *scope) { mScopeStack.push(scope); }
  void Pop() { mScopeStack.pop(); }

  UserTypeNode *VisitUserTypeNode(UserTypeNode *node);
};

class CheckTypeVisitor : public AstVisitor {
 private:
  Module_Handler *mHandler;
  unsigned        mFlags;

  std::stack<ASTScope *> mScopeStack;

 public:
  explicit CheckTypeVisitor(Module_Handler *h, unsigned f, bool base = false)
    : AstVisitor((f & FLG_trace_1) && base), mHandler(h), mFlags(f) {}
  ~CheckTypeVisitor() = default;

  // check if typeid "tid" is compatible with "target"
  bool IsCompatible(TypeId tid, TypeId target);

  IdentifierNode *VisitIdentifierNode(IdentifierNode *node);
};

}
#endif
