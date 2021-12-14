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

//////////////////////////////////////////////////////////////////////////////////////////////
//                This is the interface to translate AST to MapleIR.
//////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __AST2MPL_BUILDER_HEADER__
#define __AST2MPL_BUILDER_HEADER__

#include "ast.h"
#include "ast_handler.h"

#include "mir_module.h"
#include "generic_attrs.h"
#include "maplefe_mir_builder.h"

namespace maplefe {

#define NOTYETIMPL(K)      { if (mTraceA2m) { MNYI(K);      }}
#define AST2MPLMSG0(K)     { if (mTraceA2m) { MMSG0(K);     }}
#define AST2MPLMSG(K,v)    { if (mTraceA2m) { MMSG(K,v);    }}
#define AST2MPLMSG2(K,v,w) { if (mTraceA2m) { MMSG2(K,v,w); }}

enum StmtExprKind {
  SK_Null,
  SK_Stmt,
  SK_Expr
};

enum bool3 {
  false3,
  true3,
  maybe3
};

class Ast2MplBuilder {
private:
  AST_Handler    *mASTHandler;
  const char     *mFilename;
  bool            mTraceA2m;
  FEMIRBuilder    *mMirBuilder;
  maple::MIRType *mDefaultType;
  FieldData       *mFieldData;
  unsigned        mUniqNum;
  unsigned        mFlags;

public:
  maple::MIRModule *mMirModule;
  // use type's uniq name as key
  std::map<unsigned, maple::MIRType*> mNodeTypeMap;
  std::map<unsigned, std::vector<maple::MIRFunction*>> mNameFuncMap;
  std::map<BlockNode*, maple::BlockNode*> mBlockNodeMap;
  std::map<BlockNode*, maple::MIRFunction*> mBlockFuncMap;
  std::map<TreeNode*, maple::MIRFunction*> mFuncMap;
  std::map<std::pair<unsigned, BlockNode*>, maple::MIRSymbol*> mNameBlockVarMap;

  Ast2MplBuilder(AST_Handler *h, unsigned f);
  ~Ast2MplBuilder();

  void Build();

  void Init();
  bool IsStmt(TreeNode *tnode);
  bool3 IsCompatibleTo(maple::PrimType expected, maple::PrimType prim);

  void UpdateFuncName(maple::MIRFunction *func);

  virtual const char *Type2Label(const maple::MIRType *type);
  void Type2Name(std::string &str, const maple::MIRType *type);

  ClassNode *GetSuperClass(ClassNode *klass);
  BlockNode *GetSuperBlock(BlockNode *block);
  maple::MIRSymbol *GetSymbol(TreeNode *tnode, BlockNode *block);
  maple::MIRSymbol *CreateSymbol(TreeNode *tnode, BlockNode *block);
  maple::MIRSymbol *CreateTempVar(const char *prefix, maple::MIRType *type);
  maple::MIRFunction *GetCurrFunc(BlockNode *block);
  maple::MIRFunction *SearchFunc(unsigned idx, maple::MapleVector<maple::BaseNode *> &args);
  maple::MIRFunction *SearchFunc(TreeNode *method, maple::MapleVector<maple::BaseNode *> &args, BlockNode *block);
  maple::MIRClassType *GetClass(BlockNode *block);
  void UpdateUniqName(std::string &str);

  maple::PrimType MapPrim(TypeId id);
  maple::MIRType *MapPrimType(TypeId id);
  maple::MIRType *MapPrimType(PrimTypeNode *tnode);

  maple::MIRType *MapType(TreeNode *tnode);
  void MapAttr(maple::GenericAttrs &attr, AttrId id);
  void MapAttr(maple::GenericAttrs &attr, IdentifierNode *inode);
  void MapAttr(maple::GenericAttrs &attr, FunctionNode *fnode);

  maple::Opcode MapUnaOpcode(OprId);
  maple::Opcode MapBinOpcode(OprId);
  maple::Opcode MapBinCmpOpcode(OprId);
  maple::Opcode MapBinComboOpcode(OprId);

  maple::BaseNode *ProcessNodeDecl(StmtExprKind, TreeNode *tnode, BlockNode *);
  maple::BaseNode *ProcessNode(StmtExprKind, TreeNode *tnode, BlockNode *);

  // Process different TreeNode kind while expecting StmtExprKind as stmt or expr
#undef  NODEKIND
#define NODEKIND(K) maple::BaseNode *Process##K(StmtExprKind, TreeNode*, BlockNode*);
#include "ast_nk.def"

  maple::BaseNode *ProcessClassDecl(StmtExprKind, TreeNode*, BlockNode*);
  maple::BaseNode *ProcessInterfaceDecl(StmtExprKind, TreeNode*, BlockNode*);
  maple::BaseNode *ProcessFieldDecl(StmtExprKind, TreeNode*, BlockNode*);
  maple::BaseNode *ProcessFuncDecl(StmtExprKind, TreeNode*, BlockNode*);
  maple::BaseNode *ProcessBlockDecl(StmtExprKind, TreeNode*, BlockNode*);

  maple::BaseNode *ProcessFuncSetup(StmtExprKind, TreeNode*, BlockNode*);

  maple::BaseNode *ProcessUnaOperatorMpl(StmtExprKind skind,
                                         maple::Opcode op,
                                         maple::BaseNode *bn,
                                         BlockNode *block);
  maple::BaseNode *ProcessBinOperatorMplAssign(StmtExprKind skind,
                                               maple::BaseNode *lhs,
                                               maple::BaseNode *rhs,
                                               BlockNode *block);
  maple::BaseNode *ProcessBinOperatorMplComboAssign(StmtExprKind skind,
                                                    maple::Opcode op,
                                                    maple::BaseNode *lhs,
                                                    maple::BaseNode *rhs,
                                                    BlockNode *block);
  maple::BaseNode *ProcessBinOperatorMplArror(StmtExprKind skind,
                                              maple::BaseNode *lhs,
                                              maple::BaseNode *rhs,
                                              BlockNode *block);

  maple::BaseNode *ProcessLoopCondBody(StmtExprKind skind, TreeNode *cond, TreeNode *body, BlockNode *block);
  maple::BaseNode *GetNewNodeLhs(NewNode *node, BlockNode *block);
};

}
#endif
