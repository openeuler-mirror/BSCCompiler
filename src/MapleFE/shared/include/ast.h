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
// I decided to build AST tree after the parsing, instead of generating expr, stmt and etc.
//
// A module (compilation unit) could have multiple trees, depending on how many top level
// syntax constructs in it. Take Java for example, the top level constructs have Class, Interface.
// For C, it has only functions.
//
// AST is generated from the Appeal tree after SortOut. We walk through the appeal tree, and on
// each node, we apply the 'action' of the rule table which help build the tree node.
//////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __AST_HEADER__
#define __AST_HEADER__

// A node in AST could be one of the following types.
//
// 1. Token
//    This is the leaf node in an AST. It could be a variable, a literal.
//
// 2. Operator
//    This is one of the operators defined in supported_operators.def
//    As you may know, operators, literals and variables (identifiers) are all preprocessed
//    to be a token. But we also define 'Token' as a NodeKind. So please keep in mind, we
//    catagorize operator to a dedicated NodeKind.
//
// 3. Construct
//    This is syntax construct, such as for(..) loop construct. This is language specific,
//    and most of these nodes are defined under each language, such as java/ directory.
//    However, I do define some popular construct in shared/ directory.
//
// 4. Function
//    A Function node have its arguments as children node. The return value is not counted.
//

#include "ast_mempool.h"
#include "container.h"

#include "supported.h"
#include "token.h"

namespace maplefe {

enum NodeKind {
#undef  NODEKIND
#define NODEKIND(K) NK_##K,
#include "ast_nk.def"
  NK_Null,
};

// TreeNode has a VIRTUAL destructor. Why?
//
// Derived TreeNodes are allocated in a mempool and are referenced identically as TreeNode*,
// no matter what derived class they are. So when free the mempool, we call the destructor
// of each node through a TreeNode pointer. In this way, a virtual destructor of TreeNode
// if needed in order to invoke the derived class destructor.

class AnnotationNode;
class TreeNode {
protected:
  NodeKind  mKind;
  unsigned  mNodeId;
  TreeNode *mParent;
  TreeNode *mLabel;   // label of a statement, or expression.
  const char *mName;

  bool      mIsStmt;  // if a node is a statement

public:
  TreeNode() : mKind(NK_Null), mLabel(NULL),
               mParent(NULL), mName(NULL), mIsStmt(false) {}
  virtual ~TreeNode() {}

#undef  NODEKIND
#define NODEKIND(K) bool Is##K() const {return mKind == NK_##K;}
#include "ast_nk.def"

  bool IsScope() {return IsBlock() || IsClass() || IsFunction() || IsInterface();}
  bool TypeEquivalent(TreeNode*);

  NodeKind GetKind() {return mKind;}
  void SetNodeId(unsigned id) {mNodeId = id;}
  void SetParent(TreeNode *p) {mParent = p;}
  void SetLabel (TreeNode *p) {mLabel = p;}
  unsigned  GetNodeId() {return mNodeId;}
  TreeNode* GetParent() {return mParent;}
  TreeNode* GetLabel()  {return mLabel;}

  bool IsStmt()    {return mIsStmt;}
  void SetIsStmt() {mIsStmt = true;}

  virtual const char* GetName() {return mName;}
  virtual void SetName(const char *s) {mName = s;}
  virtual void ReplaceChild(TreeNode *oldchild, TreeNode *newchild){}
  virtual void AddAttr(AttrId) {}
  virtual void AddAnnotation(AnnotationNode *n){}
  virtual void Dump(unsigned){}

  void DumpIndentation(unsigned);
  void DumpLabel(unsigned);

  // Release the dynamically allocated memory by this tree node.
  virtual void Release(){}
private:
  static unsigned GetNextNodeId() {static unsigned id = 1; return id++; }
};

//////////////////////////////////////////////////////////////////////////
//                     Package Node
//////////////////////////////////////////////////////////////////////////

class PackageNode : public TreeNode {
private:
  TreeNode *mPackage;
public:
  PackageNode(){mKind = NK_Package;}
  PackageNode(const char *s) {mKind = NK_Package; mName = s;}
  ~PackageNode() {}

  TreeNode* GetPackage() {return mPackage;}
  void SetPackage(TreeNode *t) {mPackage = t;}

  void SetName(const char *s) {mName = s;}
  void Dump(unsigned indent);
};

//////////////////////////////////////////////////////////////////////////
//                     XXportAsPair Node
// In JS, the import or export support: xxport {x as y}
// Its kind of like a mapping of internal name to extern name.
//
// Some special case.
//
// 1. Export may export a declaration like:
//      export function(xxx) {...}
//    In this case only mBefore is used to point to the function or other
//    declarations.
// 2. Export/Import everything '*' as 'xxx'.
//    In this case, mIsEverything is set, mAfter is pointing to 'xxx'.
// 3. so on.
//////////////////////////////////////////////////////////////////////////

class XXportAsPairNode : public TreeNode {
private:
  bool      mIsDefault;     // import or export 'default'
  bool      mIsEverything;  // import or export '*', which is everything
  TreeNode *mBefore;      // name before 'as'
  TreeNode *mAfter;       // name after 'as'

public:
  XXportAsPairNode() : mIsDefault(false), mIsEverything(false),
                       mBefore(NULL), mAfter(NULL) {mKind = NK_XXportAsPair;}
  ~XXportAsPairNode() {}

  bool IsDefault()    {return mIsDefault;}
  void SetIsDefault() {mIsDefault = true;}

  bool IsEverything()    {return mIsEverything;}
  void SetIsEverything() {mIsEverything = true;}

  TreeNode* GetBefore() {return mBefore;}
  void SetBefore(TreeNode *t) {mBefore = t;}

  TreeNode* GetAfter() {return mAfter;}
  void SetAfter(TreeNode *t) {mAfter = t;}

  void Dump(unsigned indent);
};

//////////////////////////////////////////////////////////////////////////
//                         Export Nodes
// export first comes from Javascript.
//
// If Export only exports a decl or a statement, it will be saved in
// mPairs as the only pair. This is the same in ImportNode.
//////////////////////////////////////////////////////////////////////////

class ExportNode : public TreeNode {
private:
  TreeNode       *mTarget;    // the exported package in Java or module in JS
  SmallVector<XXportAsPairNode*> mPairs;
public:
  ExportNode() : mTarget(NULL) {mKind = NK_Export;}
  ~ExportNode(){}

  void SetTarget(TreeNode *t) {mTarget = t;}
  TreeNode* GetTarget() {return mTarget;}

  unsigned GetPairsNum() {return mPairs.GetNum();}
  XXportAsPairNode* GetPair(unsigned i) {return mPairs.ValueAtIndex(i);}
  void SetPair(unsigned i, XXportAsPairNode* n) {*(mPairs.RefAtIndex(i)) = n;}
  void AddPairs(TreeNode *p);

  void Dump(unsigned indent);
};


//////////////////////////////////////////////////////////////////////////
//                     Import Node
// Java import, c/c++ include, are the same scenarios. We just save the
// original string and let the language's verifier to check. We do borrow
// the idea from Java so as to separate type.vs.static and single.vs.ondemand.
// We also borrow the idea of system directory vs. local directory from c/c++.
//////////////////////////////////////////////////////////////////////////

// The property is useful in Java right now. Javascript use the XXportAsPair.
enum ImportProperty {
  ImpNone = 0,
  ImpType = 1,        // Java like, import type
  ImpStatic = 1 << 1, // Java like, import static field.
                      // If we don't specify the data type of imported, it's
                      // everything. This is c/c++ style.
  ImpSingle = 1 << 2,
  ImpAll = 1 << 3,    // Java's OnDemand import. c/c++ style too.

  ImpLocal = 1 << 4,
  ImpSystem = 1 << 5
};

inline ImportProperty& operator|=(ImportProperty& t, ImportProperty p) {
    return t = static_cast<ImportProperty>(static_cast<unsigned>(t) | static_cast<unsigned>(p));
}

inline ImportProperty operator&(ImportProperty p, ImportProperty q) {
    return static_cast<ImportProperty>(static_cast<unsigned>(p) & static_cast<unsigned>(q));
}

class ImportNode : public TreeNode {
private:
  // Solely for Java.
  ImportProperty  mProperty;  // This is solely for Java.

  // Solely for javascript right now.
  SmallVector<XXportAsPairNode*> mPairs;

  // the imported target, a package in Java, or a module in JS
  TreeNode       *mTarget;

public:
  ImportNode() {mName = NULL; mProperty = ImpNone; mKind = NK_Import;}
  ~ImportNode(){mPairs.Release();}

  void SetProperty(ImportProperty p) {mProperty = p;}
  ImportProperty GetProperty() {return mProperty;}
  void SetTarget(TreeNode *t) {mTarget = t;}
  TreeNode* GetTarget() {return mTarget;}

  void SetImportType()   {mProperty |= ImpType;}
  void SetImportStatic() {mProperty |= ImpStatic;}
  void SetImportSingle() {mProperty |= ImpSingle;}
  void SetImportAll()    {mProperty |= ImpAll;}
  void SetImportLocal()  {mProperty |= ImpLocal;}
  void SetImportSystem() {mProperty |= ImpSystem;}
  bool IsImportType()   {return mProperty & ImpType;}
  bool IsImportStatic() {return mProperty & ImpStatic;}
  bool IsImportSingle() {return mProperty & ImpSingle;}
  bool IsImportAll()    {return mProperty & ImpAll;}
  bool IsImportLocal()  {return mProperty & ImpLocal;}
  bool IsImportSystem() {return mProperty & ImpSystem;}

  unsigned GetPairsNum() {return mPairs.GetNum();}
  XXportAsPairNode* GetPair(unsigned i) {return mPairs.ValueAtIndex(i);}
  void SetPair(unsigned i, XXportAsPairNode* n) {*(mPairs.RefAtIndex(i)) = n;}
  void AddPairs(TreeNode *p);

  void Dump(unsigned indent);
};

//////////////////////////////////////////////////////////////////////////
//              Operator Nodes
//////////////////////////////////////////////////////////////////////////

enum OperatorProperty {
  Unary = 1,   // Unary operator, in front of symbol, like +a, -a
  Binary= 2,
  Ternary = 4,
  Pre = 8,     // PreCompute, compute before evaluate value, --, ++
  Post = 16,   // PostCompute,evaluate before compute,
  OperatorProperty_NA = 32
};

struct OperatorDesc {
  OprId     mOprId;
  unsigned  mDesc;  // The type was defined as OperatorProperty, but clang complains
                    // in the .cpp where Unary|Binary is treated as 'int'. Clang
                    // doesn't allow 'int' converted to OperatorProperty
};
extern OperatorDesc gOperatorDesc[OPR_NA];
extern unsigned GetOperatorProperty(OprId);

class UnaOperatorNode : public TreeNode {
private:
  bool      mIsPost;  // if it's a postfix operation?
  OprId     mOprId;
  TreeNode *mOpnd;
public:
  UnaOperatorNode(OprId id) : mOprId(id), mOpnd(NULL), mIsPost(false)
    {mKind = NK_UnaOperator;}
  UnaOperatorNode() : mOpnd(NULL), mIsPost(false) {mKind = NK_UnaOperator;}
  ~UnaOperatorNode() {}

  void SetIsPost(bool b)    {mIsPost = b;}
  void SetOpnd(TreeNode* t) {mOpnd = t; t->SetParent(this);}
  void SetOprId(OprId o)    {mOprId = o;}

  bool      IsPost()  {return mIsPost;}
  TreeNode* GetOpnd() {return mOpnd;}
  OprId     GetOprId(){return mOprId;}

  void ReplaceChild(TreeNode*, TreeNode*);

  void Dump(unsigned);
};

class BinOperatorNode : public TreeNode {
private:
  OprId     mOprId;
  TreeNode *mOpndA;
  TreeNode *mOpndB;
public:
  BinOperatorNode(OprId id) : mOprId(id) {mKind = NK_BinOperator;}
  BinOperatorNode() {mKind = NK_BinOperator;}
  ~BinOperatorNode() {}

  OprId     GetOprId() {return mOprId;}
  TreeNode* GetOpndA() {return mOpndA;}
  TreeNode* GetOpndB() {return mOpndB;}
  void SetOprId(OprId o)     {mOprId = o;}
  void SetOpndA(TreeNode* t) {mOpndA = t; t->SetParent(this);}
  void SetOpndB(TreeNode* t) {mOpndB = t; t->SetParent(this);}

  void ReplaceChild(TreeNode*, TreeNode*);
  void Dump(unsigned);
};

// TerOperatorNode is for an expression like
//   a > b ? c : d
class TerOperatorNode : public TreeNode {
private:
  TreeNode *mOpndA;
  TreeNode *mOpndB;
  TreeNode *mOpndC;
public:
  TerOperatorNode() {mKind = NK_TerOperator;}
  ~TerOperatorNode() {}

  TreeNode* GetOpndA() {return mOpndA;}
  TreeNode* GetOpndB() {return mOpndB;}
  TreeNode* GetOpndC() {return mOpndC;}
  void SetOpndA(TreeNode* t) {mOpndA = t; t->SetParent(this);}
  void SetOpndB(TreeNode* t) {mOpndB = t; t->SetParent(this);}
  void SetOpndC(TreeNode* t) {mOpndC = t; t->SetParent(this);}

  void Dump(unsigned);
};

//////////////////////////////////////////////////////////////////////////
//                      New and Delete operation
// Java and C++ have new operation, with different syntax and semantics.
// C++ also has delete operation. The tree nodes below contains most of
// the information many similar languages need. But the semantic
// verification will have to be done specifically by each language.
//////////////////////////////////////////////////////////////////////////

class BlockNode;
class NewNode : public TreeNode {
private:
  TreeNode  *mId;                  // A name could be like Outer.Inner
                                   // Hard to give a const char* as name.
                                   // So give it an id.
  SmallVector<TreeNode*> mArgs;    //
  BlockNode *mBody;                // When body is not empty, it's an
                                   // anonymous class.
public:
  NewNode() : mId(NULL), mBody(NULL) {mKind = NK_New;}
  ~NewNode() {mArgs.Release();}

  TreeNode* GetId()          {return mId;}
  void SetId(TreeNode *n)    {mId = n;}
  BlockNode* GetBody()       {return mBody;}
  void SetBody(BlockNode *n) {mBody = n;}

  unsigned  GetArgsNum()        {return mArgs.GetNum();}
  TreeNode* GetArg(unsigned i)  {return mArgs.ValueAtIndex(i);}
  void      SetArg(unsigned i, TreeNode* n) {*(mArgs.RefAtIndex(i)) = n;}
  void      AddArg(TreeNode *t) {mArgs.PushBack(t);}

  void ReplaceChild(TreeNode *oldone, TreeNode *newone);
  void Dump(unsigned);
};

class DeleteNode : public TreeNode {
};

//////////////////////////////////////////////////////////////////////////
//                         Identifier Nodes
// Everything having a name will be treated as an Identifier node at the
// first place. There are some issues here.
// 1) Each appearance of identifier will be given a new IdentifierNode at
//    the first place. Later, when the scope tree is ready, we can reduce
//    the nodes by merging multiple nodes into one if they are actually
//    semantically equal. For example, a same variable appears in multiple
//    places in a function, and these multiple appearance will be same node.
// 2) The IdentifierNode at the declaration point will have mType. The other
//    appearance of the identifier node won't have mType. After merging, they
//    will point to the same node.
//
//                        Dimension of IdentifierNode
//
// For a simple case like, int a[], the dimension info could be attached to
// type info or identifier. I finally decided to put in the identifier since
// it's actually a feature of the object not the type. The type only tells
// what each element is. But anyway, it doesn't matter much in either way.
// A slight advantage puting dimension in identifier is we don't need so many
// types.
//////////////////////////////////////////////////////////////////////////

// mDimensions[N] tells the Nst dimension.
// mDimensions[N] = 0, means the length of that dimension is unspecified.
class DimensionNode : public TreeNode {
private:
  SmallVector<unsigned> mDimensions;
public:
  DimensionNode() {mKind = NK_Dimension;}
  ~DimensionNode(){Release();}

  unsigned GetDimensionsNum() {return mDimensions.GetNum();}
  unsigned GetDimension(unsigned i) {return mDimensions.ValueAtIndex(i);} // 0 means unspecified.
  void     SetDimension(unsigned i, unsigned n) {*(mDimensions.RefAtIndex(i)) = n;}
  void AddDim(unsigned i = 0) {mDimensions.PushBack(i);}
  void Merge(const TreeNode*);

  void Release() {mDimensions.Release();}
  void Dump();
};

// IdentifierNode is the most important node, and we usually call it symbol.
// It has most of the syntax information.
class IdentifierNode : public TreeNode {
private:
  SmallVector<AttrId> mAttrs;
  TreeNode      *mType; // PrimTypeNode or UserTypeNode
  TreeNode      *mInit; // Init value
  DimensionNode *mDims;
public:
  IdentifierNode(const char *s) : mType(NULL), mInit(NULL), mDims(NULL){
    mKind = NK_Identifier;
    SetName(s); }
  IdentifierNode(const char *s, TreeNode *t) : mType(t), mInit(NULL), mDims(NULL) {
    mKind = NK_Identifier;
    SetName(s);}
  ~IdentifierNode(){}

  TreeNode*   GetType() {return mType;}
  TreeNode*   GetInit() {return mInit;}
  DimensionNode* GetDims() {return mDims;}

  void SetType(TreeNode *t)      {mType = t;}
  void SetInit(TreeNode *t)      {mInit = t; t->SetParent(this);}
  void ClearInit()               {mInit = NULL;}
  void SetDims(DimensionNode *t) {mDims = t;}

  unsigned GetDimsNum()          {return mDims->GetDimensionsNum();}
  unsigned GetDim(unsigned n)    {return mDims->GetDimension(n);} // 0 means unspecified.
  bool     IsArray()             {return mDims && GetDimsNum() > 0;}
  unsigned AddDim(unsigned i = 0){mDims->AddDim(i);}           // 0 means unspecified
  unsigned GetNthNum(unsigned n) {return mDims->GetDimension(n);} // 0 means unspecified.
  void     SetNthNum(unsigned n, unsigned i) {mDims->SetDimension(n, i);}

  // Attributes related
  unsigned GetAttrsNum() const        {return mAttrs.GetNum();}
  void     AddAttr(AttrId a)          {mAttrs.PushBack(a);}
  AttrId   GetAttrAtIndex(unsigned i) {return mAttrs.ValueAtIndex(i);}
  void     SetAttrAtIndex(unsigned i, AttrId n) {*(mAttrs.RefAtIndex(i)) = n;}

  void Release() { if (mDims) mDims->Release();}
  void Dump(unsigned);
};

//////////////////////////////////////////////////////////////////////////
//                       DeclNode
// A DeclNode defines one single variable or a VarList.
// The type info and init expr are both inside the IdentifierNode.
// DeclNode only tells this is a declaration.
//////////////////////////////////////////////////////////////////////////

// special property of Javascript
enum DeclProp {
  JS_Var,
  JS_Let,
  JS_Const,
  DP_NA
};

class DeclNode : public TreeNode {
private:
  TreeNode *mVar;
  TreeNode *mInit; // Init value
  DeclProp  mProp;
public:
  DeclNode() : mVar(NULL), mProp(DP_NA) {mKind = NK_Decl;}
  DeclNode(TreeNode *id) : mVar(id), mProp(DP_NA) {mKind = NK_Decl;}
  ~DeclNode(){}

  TreeNode* GetVar() {return mVar;}
  TreeNode* GetInit() {return mInit;}

  void SetVar(TreeNode *t) {mVar = t; t->SetParent(this);}
  void SetInit(TreeNode *t) {mInit = t; t->SetParent(this);}

  DeclProp GetProp() {return mProp;}
  void SetProp(DeclProp p) {mProp = p;}

  void Dump(unsigned);
};

//////////////////////////////////////////////////////////////////////////
//                         AnnotationNode
//
// Pragma or Annotation are language constructs to help (1) compiler
// (2) runtime (3) other tools, to better analyze or execute the program.
// It doesn't change the program behaviour, but may be improve static
// and/or dynamic analysis and/or performance.
//
// We have a dedicated AnnotationNode here. The program/annotation syntax
// could be complicated, eg. c99 introduce function call like pragma.
//
// The difference between Java annotation and C/c++ pragma is annotation
// is user defined and pragma is compiler/system defined. In another word
// Java annoation has unlimited number while pragmas are limited.
//////////////////////////////////////////////////////////////////////////

// AnnotationTypeNode defines a new Annotation
class AnnotationTypeNode : public TreeNode {
private:
  IdentifierNode *mId;
public:
  IdentifierNode* GetId() {return mId;}
  void SetId(IdentifierNode *n) {mId = n;}
  void Dump(unsigned);
};

// Annotation/Pragma is complicated, but everything starts from the
// name. So at construction, we will take in the name at first.
// Later we will initialize mType, mExpr based on the 'name' and other
// expression.
class AnnotationNode : public TreeNode {
private:
  IdentifierNode     *mId;
  AnnotationTypeNode *mType;
  TreeNode           *mExpr;
public:
  AnnotationNode() : mId(NULL), mType(NULL), mExpr(NULL) {
    mKind = NK_Annotation;}
  ~AnnotationNode(){}

  IdentifierNode* GetId() {return mId;}
  void SetId(IdentifierNode *n) {mId = n;}
  AnnotationTypeNode* GetType() {return mType;}
  void SetType(AnnotationTypeNode *n) {mType = n;}
  TreeNode* GetExpr() {return mExpr;}
  void SetExpr(TreeNode *n) {mExpr = n;}
};

//////////////////////////////////////////////////////////////////////////
//                           CastNode
// This node is for type casting node, implicit or explicit.
//////////////////////////////////////////////////////////////////////////

class CastNode : public TreeNode {
private:
  TreeNode *mDestType;
  TreeNode *mExpr;
public:
  CastNode() : mDestType(NULL), mExpr(NULL) {mKind = NK_Cast;}
  ~CastNode(){}

  TreeNode* GetDestType() {return mDestType;}
  void SetDestType(TreeNode *t) {mDestType = t;}

  TreeNode* GetExpr() {return mExpr;}
  void SetExpr(TreeNode *t) {mExpr = t;}

  const char* GetName();
  void Dump(unsigned);
};

//////////////////////////////////////////////////////////////////////////
//                           ParenthesisNode
// In many languages like Java, c, c++, an Cast Expression has common
// patten like (TypeExpr)ValueExpr. The type is represented by a parenthesis
// expression. However, as language designers writing the Rules of spec,
// the (TypeExpr) and ValueExpr could be matched by different rules, which
// are not recoganized as a cast expression. It needs some manipulation
// after the tree is built at the first time.
//
// We define a Parenthesis node to help ASTTree::Manipulate2Cast().
//////////////////////////////////////////////////////////////////////////

class ParenthesisNode : public TreeNode {
private:
  TreeNode *mExpr;
public:
  ParenthesisNode() : mExpr(NULL) {mKind = NK_Parenthesis;}
  ~ParenthesisNode(){}

  TreeNode* GetExpr() {return mExpr;}
  void SetExpr(TreeNode *t) {mExpr = t; t->SetParent(this);}

  void Dump(unsigned);
};

//////////////////////////////////////////////////////////////////////////
//                           FieldNode
// This is used for field reference. The field could be a member field or
// a member function.
//////////////////////////////////////////////////////////////////////////

class FieldNode : public TreeNode {
private:
  TreeNode       *mUpper; // The upper enclosing structure
  IdentifierNode *mField;
public:
  FieldNode() : TreeNode(), mField(NULL), mUpper(NULL) {mKind = NK_Field;}
  ~FieldNode(){}

  IdentifierNode* GetField() {return mField;}
  void SetField(IdentifierNode *f) {mField = f;}

  TreeNode *GetUpper()       {return mUpper;}
  void SetUpper(TreeNode *n) {
    TreeNode *up = n;
    while (up->IsParenthesis()) {
      ParenthesisNode *pn = (ParenthesisNode*)up;
      up = pn->GetExpr();
    }
    mUpper = up;
  }

  void Dump(unsigned);
};

//////////////////////////////////////////////////////////////////////////
//                           Array Related Nodes
// ArrayElementNode, ArrayLiteralNode.
//////////////////////////////////////////////////////////////////////////

// Array element is a[b][c].
class ArrayElementNode : public TreeNode {
private:
  IdentifierNode        *mArray;
  SmallVector<TreeNode*> mExprs;  // index expressions.
public:
  ArrayElementNode() {mKind = NK_ArrayElement; mArray = NULL;}
  ~ArrayElementNode() {Release();}

  IdentifierNode* GetArray()                  {return mArray;}
  void            SetArray(IdentifierNode *n) {mArray = n;}

  unsigned  GetExprsNum()       {return mExprs.GetNum();}
  TreeNode* GetExprAtIndex(unsigned i) {return mExprs.ValueAtIndex(i);}
  void      SetExprAtIndex(unsigned i, TreeNode* n) {*(mExprs.RefAtIndex(i)) = n;}
  void      AddExpr(TreeNode *n){mExprs.PushBack(n);}

  void Release() {mExprs.Release();}
  void Dump(unsigned);
};


// Array literal is [1, 2 , 0, -4]. It's an arrya of literals.
class ArrayLiteralNode : public TreeNode {
private:
  SmallVector<TreeNode*> mLiterals;
public:
  ArrayLiteralNode() {mKind = NK_ArrayLiteral;}
  ~ArrayLiteralNode() {Release();}

  unsigned  GetLiteralsNum()       {return mLiterals.GetNum();}
  TreeNode* GetLiteral(unsigned i) {return mLiterals.ValueAtIndex(i);}
  void      SetLiteral(unsigned i, TreeNode* n) {*(mLiterals.RefAtIndex(i)) = n;}
  void      AddLiteral(TreeNode *n){mLiterals.PushBack(n);}

  void Release() {mLiterals.Release();}
  void Dump(unsigned);
};


//////////////////////////////////////////////////////////////////////////
//                           Struct Node
// This is first coming from C struct. Typescript 'interface' has the
// similar structure.
//////////////////////////////////////////////////////////////////////////

enum StructProp {
  SProp_CStruct,
  SProp_TSInterface,
  SProp_NA
};

class StructNode : public TreeNode {
private:
  StructProp      mProp;
  IdentifierNode *mStructId;
  SmallVector<IdentifierNode*> mFields;
public:
  StructNode() {mKind = NK_Struct; mName = NULL; mProp = SProp_NA;}
  StructNode(IdentifierNode *n) {mKind = NK_Struct; mStructId = n; mProp = SProp_NA;}
  ~StructNode() {Release();}

  StructProp GetProp() {return mProp;}
  IdentifierNode* GetStructId() {return mStructId;}
  void SetProp(StructProp p) {mProp = p;}
  void SetStructId(IdentifierNode *n) {mStructId = n;}

  unsigned        GetFieldsNum() {return mFields.GetNum();}
  IdentifierNode* GetField(unsigned i) {return mFields.ValueAtIndex(i);}
  void            SetField(unsigned i, IdentifierNode* n) {*(mFields.RefAtIndex(i)) = n;}
  void            AddField(IdentifierNode *n) {mFields.PushBack(n);}

  void Release() {mFields.Release();}
  void Dump(unsigned);
};

// We define StructLiteral for C/C++ struct literal, TS/JS object literal.
// It contains a list of duple <fieldname, value>
class FieldLiteralNode : public TreeNode{
public:
  IdentifierNode *mFieldName;
  TreeNode       *mLiteral;

  void SetFieldName(IdentifierNode *id) {mFieldName = id;}
  void SetLiteral(TreeNode *id) {mLiteral = id;}

  IdentifierNode* GetFieldName() {return mFieldName;}
  TreeNode* GetLiteral() {return mLiteral;}

  FieldLiteralNode() {mKind = NK_FieldLiteral;}
  ~FieldLiteralNode(){}
};

class StructLiteralNode : public TreeNode {
private:
  SmallVector<FieldLiteralNode*> mFields;
public:
  StructLiteralNode() {mKind = NK_StructLiteral;}
  ~StructLiteralNode(){Release();}

  unsigned          GetFieldsNum() {return mFields.GetNum();}
  FieldLiteralNode* GetField(unsigned i) {return mFields.ValueAtIndex(i);}
  void              SetField(unsigned i, FieldLiteralNode* n) {*(mFields.RefAtIndex(i)) = n;}
  void              AddField(FieldLiteralNode *d) {mFields.PushBack(d);}

  void Dump(unsigned);
};

//////////////////////////////////////////////////////////////////////////
//                           VarList Node
// Why do we need a VarListNode? Often in the program we have multiple
// variables like parameters in function, or varable list in declaration.
//////////////////////////////////////////////////////////////////////////

class VarListNode : public TreeNode {
private:
  SmallVector<IdentifierNode*> mVars;
public:
  VarListNode() {mKind = NK_VarList;}
  ~VarListNode() {}

  unsigned GetVarsNum() {return mVars.GetNum();}
  IdentifierNode* GetVarAtIndex(unsigned i) {return mVars.ValueAtIndex(i);}
  void            SetVarAtIndex(unsigned i, IdentifierNode* n) {*(mVars.RefAtIndex(i)) = n;}

  void AddVar(IdentifierNode *n);
  void Merge(TreeNode*);
  void Release() {mVars.Release();}
  void Dump(unsigned);
};

//////////////////////////////////////////////////////////////////////////
//                           ExprList Node
// It's for list of expressions. This covers more than VarList.
// This is used mostly as arguments of call site.
//////////////////////////////////////////////////////////////////////////

class ExprListNode : public TreeNode {
private:
  SmallVector<TreeNode*> mExprs;
public:
  ExprListNode() {mKind = NK_ExprList;}
  ~ExprListNode() {}

  unsigned GetExprsNum() {return mExprs.GetNum();}
  TreeNode* GetExprAtIndex(unsigned i) {return mExprs.ValueAtIndex(i);}
  void      SetExprAtIndex(unsigned i, TreeNode* n) {*(mExprs.RefAtIndex(i)) = n;}

  void AddExpr(TreeNode *n) {mExprs.PushBack(n);}
  void Merge(TreeNode*);
  void Release() {mExprs.Release();}
  void Dump(unsigned);
};

//////////////////////////////////////////////////////////////////////////
//                         Literal Nodes
//////////////////////////////////////////////////////////////////////////

class LiteralNode : public TreeNode {
private:
  LitData mData;
private:
  void InitName();

public:
  LiteralNode(LitData d) : mData(d) {
    mKind = NK_Literal;
  }
  ~LiteralNode(){}

  LitData GetData() {return mData;}
  void Dump(unsigned);
};

//////////////////////////////////////////////////////////////////////////
//                         ThrowNode
// This is the throw statement.
// In Java some functions throw exception in declaration, and they are
// saved in FunctionNode::mThrows.
//////////////////////////////////////////////////////////////////////////

class ThrowNode : public TreeNode {
private:
  SmallVector<TreeNode *> mExceptions;
public:
  ThrowNode() {mKind = NK_Throw;}
  ~ThrowNode(){}

  unsigned  GetExceptionsNum() {return mExceptions.GetNum();}
  TreeNode* GetExceptionAtIndex(unsigned i) {return mExceptions.ValueAtIndex(i);}
  void      SetExceptionAtIndex(unsigned i, TreeNode* n) {*(mExceptions.RefAtIndex(i)) = n;}
  void      AddException(TreeNode *n);

  void Dump(unsigned);
};

//////////////////////////////////////////////////////////////////////////
//                         Try, Catch, Finally
// I can build one single TryNode to contain all the information. However,
// I built all three types of nodes, since some language could have complex
// syntax of catch or finally.
//////////////////////////////////////////////////////////////////////////

class CatchNode : public TreeNode {
private:
  SmallVector<TreeNode *> mParams;  // In Java, this sould be exception node.
  BlockNode              *mBlock;
public:
  CatchNode() : mBlock(NULL) {mKind = NK_Catch;}
  ~CatchNode(){}

  BlockNode* GetBlock()       {return mBlock;}
  void SetBlock(BlockNode *n) {mBlock = n;}

  unsigned  GetParamsNum() {return mParams.GetNum();}
  TreeNode* GetParamAtIndex(unsigned i) {return mParams.ValueAtIndex(i);}
  void      SetParamAtIndex(unsigned i, TreeNode* n) {*(mParams.RefAtIndex(i)) = n;}
  void      AddParam(TreeNode *n);

  void Dump(unsigned);
};

class FinallyNode : public TreeNode {
private:
  BlockNode              *mBlock;
public:
  FinallyNode() : mBlock(NULL) {mKind = NK_Finally;}
  ~FinallyNode(){}

  BlockNode* GetBlock()       {return mBlock;}
  void SetBlock(BlockNode *n) {mBlock = n;}

  void Dump(unsigned);
};

class TryNode : public TreeNode {
private:
  BlockNode   *mBlock;
  FinallyNode *mFinally;
  SmallVector<CatchNode*> mCatches; // There could be >1 catches.

public:
  TryNode() : mBlock(NULL), mFinally(NULL) {mKind = NK_Try;}
  ~TryNode(){}

  BlockNode* GetBlock()       {return mBlock;}
  void SetBlock(BlockNode *n) {mBlock = n;}

  FinallyNode* GetFinally()       {return mFinally;}
  void SetFinally(FinallyNode *n) {mFinally = n;}

  unsigned   GetCatchesNum() {return mCatches.GetNum();}
  CatchNode* GetCatchAtIndex(unsigned i) {return mCatches.ValueAtIndex(i);}
  void       SetCatchAtIndex(unsigned i, CatchNode* n) {*(mCatches.RefAtIndex(i)) = n;}
  void       AddCatch(TreeNode *n);

  void Dump(unsigned);
};

//////////////////////////////////////////////////////////////////////////
//                         Exception Node
//////////////////////////////////////////////////////////////////////////

class ExceptionNode : public TreeNode {
private:
  // right now I only put a name for it. It could have more properties.
  IdentifierNode *mException;
public:
  ExceptionNode() : mException(NULL) {mKind = NK_Exception;}
  ExceptionNode(IdentifierNode *inode) : mException(inode) {mKind = NK_Exception;}
  ~ExceptionNode(){}

  IdentifierNode* GetException()       {return mException;}
  void SetException(IdentifierNode *n) {mException = n;}

  void Dump(unsigned);
};

//////////////////////////////////////////////////////////////////////////
//          Statement Node, Control Flow related nodes
//////////////////////////////////////////////////////////////////////////

class ReturnNode : public TreeNode {
private:
  TreeNode *mResult;
public:
  ReturnNode() : mResult(NULL) {mKind = NK_Return;}
  ~ReturnNode(){}

  void SetResult(TreeNode *t) {mResult = t;}
  TreeNode* GetResult() { return mResult; }
  void Dump(unsigned);
};

class CondBranchNode : public TreeNode {
private:
  TreeNode *mCond;
  TreeNode *mTrueBranch;
  TreeNode *mFalseBranch;
public:
  CondBranchNode();
  ~CondBranchNode(){}

  void SetCond(TreeNode *t)       {mCond = t; t->SetParent(this);}
  void SetTrueBranch(TreeNode *t) {mTrueBranch = t; t->SetParent(this);}
  void SetFalseBranch(TreeNode *t){mFalseBranch = t; t->SetParent(this);}

  TreeNode* GetCond()        {return mCond;}
  TreeNode* GetTrueBranch()  {return mTrueBranch;}
  TreeNode* GetFalseBranch() {return mFalseBranch;}

  void Dump(unsigned);
};

// Break statement. Break targets could be one identifier or empty.
class BreakNode : public TreeNode {
private:
  TreeNode* mTarget;
public:
  BreakNode() {mKind = NK_Break; mTarget = NULL;}
  ~BreakNode(){}

  TreeNode* GetTarget()           {return mTarget;}
  void      SetTarget(TreeNode* t){mTarget = t;}
  void      Dump(unsigned);
};


// Continue statement. Continue targets could be one identifier or empty.
class ContinueNode : public TreeNode {
private:
  TreeNode* mTarget;
public:
  ContinueNode() {mKind = NK_Continue; mTarget = NULL;}
  ~ContinueNode(){}

  TreeNode* GetTarget()           {return mTarget;}
  void      SetTarget(TreeNode* t){mTarget = t;}
  void      Dump(unsigned);
};

// Javascript makes for loop complicated. It creates two special syntax as below
//   for (var in set) {...}
//   for (var of set) {...}
// We use  FL_Prop to differentiate them with regular for loop.
enum ForLoopProp {
  FLP_Regular,  // this is the default property
  FLP_JSIn,
  FLP_JSOf,
  FLP_NA
};

class ForLoopNode : public TreeNode {
private:
  ForLoopProp mProp;

  // Regular for loop
  SmallVector<TreeNode *> mInits;
  TreeNode               *mCond;
  SmallVector<TreeNode *> mUpdates;

  // JS In or JS Of
  TreeNode *mVariable;
  TreeNode *mSet;

  // shared by all kinds
  TreeNode *mBody;   // This is a block node

public:
  ForLoopNode() {mCond = NULL; mBody = NULL;
                 mVariable = NULL; mSet = NULL;
                 mProp = FLP_Regular; mKind = NK_ForLoop;}
  ~ForLoopNode() {Release();}

  void AddInit(TreeNode *t)   {mInits.PushBack(t);}
  void AddUpdate(TreeNode *t) {mUpdates.PushBack(t);}
  void SetCond(TreeNode *t)   {mCond = t;}
  void SetBody(TreeNode *t)   {mBody = t;}

  unsigned GetInitsNum()     {return mInits.GetNum();}
  unsigned GetUpdatesNum()   {return mUpdates.GetNum();}
  TreeNode* GetInitAtIndex(unsigned i)   {return mInits.ValueAtIndex(i);}
  void      SetInitAtIndex(unsigned i, TreeNode* n) {*(mInits.RefAtIndex(i)) = n;}
  TreeNode* GetUpdateAtIndex(unsigned i) {return mUpdates.ValueAtIndex(i);}
  void      SetUpdateAtIndex(unsigned i, TreeNode* n) {*(mUpdates.RefAtIndex(i)) = n;}
  TreeNode* GetCond() {return mCond;}
  TreeNode* GetBody() {return mBody;}

  ForLoopProp GetProp() {return mProp;}
  void SetProp(ForLoopProp p) {mProp = p;}

  TreeNode* GetVariable() {return mVariable;}
  void SetVariable(TreeNode *t) {mVariable = t;}

  TreeNode* GetSet() {return mSet;}
  void SetSet(TreeNode *t) {mSet = t;}

  void Release() {mInits.Release(); mUpdates.Release();}
  void Dump(unsigned);
};

class WhileLoopNode : public TreeNode {
private:
  TreeNode *mCond;
  TreeNode *mBody; // This is a block node
public:
  WhileLoopNode() {mCond = NULL; mBody = NULL; mKind = NK_WhileLoop;}
  ~WhileLoopNode() {Release();}

  void SetCond(TreeNode *t) {mCond = t;}
  void SetBody(TreeNode *t) {mBody = t;}
  TreeNode* GetCond()       {return mCond;}
  TreeNode* GetBody()       {return mBody;}

  void Release(){}
  void Dump(unsigned);
};

class DoLoopNode : public TreeNode {
private:
  TreeNode *mCond;
  TreeNode *mBody; // This is a block node
public:
  DoLoopNode() {mCond = NULL; mBody = NULL; mKind = NK_DoLoop;}
  ~DoLoopNode(){Release();}

  void SetCond(TreeNode *t) {mCond = t;}
  void SetBody(TreeNode *t) {mBody = t;}
  TreeNode* GetCond()       {return mCond;}
  TreeNode* GetBody()       {return mBody;}

  void Release(){}
  void Dump(unsigned);
};

// The switch-case statement is divided into three components,
// SwitchLabelNode  : The expression of each case. The reason it's called a label is
//                    it is written as
//                       case 123:
//                    which looks like a label.
// SwitchCaseNode   : SwitchLabelNode + the statements under this label
// SwitchNode       : The main node of all.

class SwitchLabelNode : public TreeNode {
private:
  bool      mIsDefault; // default lable
  TreeNode *mValue;     // the constant expression
public:
  SwitchLabelNode() : mIsDefault(false), mValue(NULL) {mKind = NK_SwitchLabel;}
  ~SwitchLabelNode(){}

  void SetIsDefault(bool b) {mIsDefault = b;}
  void SetValue(TreeNode *t){mValue = t;}
  bool IsDefault()     {return mIsDefault;}
  TreeNode* GetValue() {return mValue;}

  void Dump(unsigned);
};

// One thing to note is about the mStmts. When creating this node, we should
// dig into subtree and resolve all the PassNode and make sure every node
// in mStmts and mLabels has semanteme. PassNode has no semanteme.
class SwitchCaseNode : public TreeNode {
private:
  SmallVector<SwitchLabelNode*> mLabels;
  SmallVector<TreeNode*>    mStmts;

public:
  SwitchCaseNode() {mKind = NK_SwitchCase;}
  ~SwitchCaseNode() {Release();}

  unsigned  GetLabelsNum()            {return mLabels.GetNum();}
  void      AddLabel(TreeNode*);
  SwitchLabelNode* GetLabelAtIndex(unsigned i) {return mLabels.ValueAtIndex(i);}
  void             SetLabelAtIndex(unsigned i, SwitchLabelNode* n) {*(mLabels.RefAtIndex(i)) = n;}

  unsigned  GetStmtsNum()            {return mStmts.GetNum();}
  TreeNode* GetStmtAtIndex(unsigned i) {return mStmts.ValueAtIndex(i);}
  void      SetStmtAtIndex(unsigned i, TreeNode* n) {*(mStmts.RefAtIndex(i)) = n;}
  void      AddStmt(TreeNode*);

  void Release() {mStmts.Release(); mLabels.Release();}
  void Dump(unsigned);
};

class SwitchNode : public TreeNode {
private:
  TreeNode *mExpr;
  SmallVector<SwitchCaseNode*> mCases;
public:
  SwitchNode() : mExpr(NULL) {mKind = NK_Switch;}
  ~SwitchNode() {Release();}

  TreeNode* GetExpr() {return mExpr;}
  void SetExpr(TreeNode *c) {mExpr = c;}

  unsigned  GetCasesNum() {return mCases.GetNum();}
  void      AddCase(TreeNode *c);
  SwitchCaseNode* GetCaseAtIndex(unsigned i) {return mCases.ValueAtIndex(i);}
  void            SetCaseAtIndex(unsigned i, SwitchCaseNode* n) {*(mCases.RefAtIndex(i)) = n;}

  void Release() {mCases.Release();}
  void Dump(unsigned);
};

// This is for an 'assert' statement in languages like Java.
// It contains two parts, one evaluation expression, the other the detailed message.
//
// In languages like C, assert is defined as a normal function, and don't go
// through this node.

class AssertNode : public TreeNode {
private:
  TreeNode  *mExpr;
  TreeNode  *mMsg;
public:
  AssertNode() : mExpr(NULL), mMsg(NULL) {mKind = NK_Assert;}
  ~AssertNode(){}

  TreeNode* GetExpr() {return mExpr;}
  TreeNode* GetMsg() {return mMsg;}
  void SetExpr(TreeNode *t) {mExpr = t;}
  void SetMsg(TreeNode *t)  {mMsg = t;}

  void Dump(unsigned);
};

// This is the node for a call site.
// 1. The method could be class method, a global method, or ...
// 2. The argument could be any expression even including another callsite.
// So I'm using TreeNode for all of them.
class CallNode : public TreeNode {
private:
  TreeNode    *mMethod;
  ExprListNode mArgs;
public:
  CallNode() : mMethod(NULL){mKind = NK_Call;}
  ~CallNode(){}

  void Init();

  TreeNode* GetMethod() {return mMethod;}
  void SetMethod(TreeNode *t) {mMethod = t;}

  void AddArg(TreeNode *t);
  unsigned GetArgsNum() {return mArgs.GetExprsNum();}
  TreeNode* GetArg(unsigned index) {return mArgs.GetExprAtIndex(index);}
  void      SetArg(unsigned i, TreeNode* n) {mArgs.SetExprAtIndex(i, n);}

  void Release() {mArgs.Release();}
  void Dump(unsigned);
};

//////////////////////////////////////////////////////////////////////////
//                         Block Nodes
// A block is common in all languages, and it serves different function in
// different context. Here is a few examples.
//   1) As function body, it could contain variable decl, statement, lambda,
//      inner function, inner class, instance initializer....
//   2) As a function block, it could contain variable declaration, statement
//      lambda, ...
//
//                       Instance Initializer
//
// Java instance initializer is a block node, and it's a shared part of all
// constructors. It's always executed. We simply let BlockNode describe
// instance initializer.
// C++ has similiar instance initializer, but its bound to a specific constructor.
// We may move those initializer into the constructor body.
//////////////////////////////////////////////////////////////////////////

class BlockNode : public TreeNode {
public:
  SmallList<TreeNode*> mChildren;

  // Some blocks are instance initializer in languages like Java,
  // and they can have attributes, such as static instance initializer.
  bool                mIsInstInit; // Instance Initializer
  SmallVector<AttrId> mAttrs;

  TreeNode           *mSync;       // Java allows a sync object on a Block.

public:
  BlockNode(){mKind = NK_Block; mIsInstInit = false; mSync = NULL;}
  ~BlockNode() {Release();}

  // Instance Initializer and Attributes related
  bool IsInstInit()    {return mIsInstInit;}
  void SetIsInstInit() {mIsInstInit = true;}
  unsigned GetAttrsNum()              {return mAttrs.GetNum();}
  void     AddAttr(AttrId a)          {mAttrs.PushBack(a);}
  AttrId   GetAttrAtIndex(unsigned i) {return mAttrs.ValueAtIndex(i);}
  void     SetAttrAtIndex(unsigned i, AttrId n) {*(mAttrs.RefAtIndex(i)) = n;}

  void  SetSync(TreeNode *n) {mSync = n;}
  TreeNode* GetSync() {return mSync;}

  unsigned  GetChildrenNum()            {return mChildren.GetNum();}
  TreeNode* GetChildAtIndex(unsigned i) {return mChildren.ValueAtIndex(i);}
  void      SetChildAtIndex(unsigned i, TreeNode* n) {*(mChildren.RefAtIndex(i)) = n;}
  void      AddChild(TreeNode *c);
  void      ClearChildren()             {mChildren.Clear();}

  void Release() {mChildren.Release();}
  void Dump(unsigned);
};

//////////////////////////////////////////////////////////////////////////
//                         Function Nodes
// Normal constructors are treated as function too, with mIsConstructor
// being true.
//////////////////////////////////////////////////////////////////////////

class ASTScope;

class FunctionNode : public TreeNode {
private:
  SmallVector<AttrId>          mAttrs;
  SmallVector<AnnotationNode*> mAnnotations; //annotation or pragma
  SmallVector<ExceptionNode*>  mThrows;      // exceptions it can throw
  TreeNode                    *mType;        // return type
  SmallVector<TreeNode*>       mParams;      //
  BlockNode                   *mBody;
  DimensionNode               *mDims;
  bool                         mIsConstructor;

public:
  FunctionNode();
  ~FunctionNode() {Release();}

  // After function body is added, we need some clean up work, eg. cleaning
  // the PassNode in the tree.
  void CleanUp();

  BlockNode* GetBody() {return mBody;}
  void SetBody(BlockNode *b) {mBody = b; b->SetParent(this); CleanUp();}

  bool IsConstructor()    {return mIsConstructor;}
  void SetIsConstructor() {mIsConstructor = true;}

  unsigned  GetParamsNum()        {return mParams.GetNum();}
  TreeNode* GetParam(unsigned i)  {return mParams.ValueAtIndex(i);}
  void      SetParam(unsigned i, TreeNode* n) {*(mParams.RefAtIndex(i)) = n;}
  void      AddParam(TreeNode *t) {mParams.PushBack(t); t->SetParent(this);}

  // Attributes related
  unsigned GetAttrsNum()              {return mAttrs.GetNum();}
  void     AddAttr(AttrId a)          {mAttrs.PushBack(a);}
  AttrId   GetAttrAtIndex(unsigned i) {return mAttrs.ValueAtIndex(i);}
  void     SetAttrAtIndex(unsigned i, AttrId n) {*(mAttrs.RefAtIndex(i)) = n;}

  // Annotation/Pragma related
  unsigned GetAnnotationsNum()           {return mAnnotations.GetNum();}
  void     AddAnnotation(AnnotationNode *n) {mAnnotations.PushBack(n);}
  AnnotationNode* GetAnnotationAtIndex(unsigned i) {return mAnnotations.ValueAtIndex(i);}
  void            SetAnnotationAtIndex(unsigned i, AnnotationNode* n) {*(mAnnotations.RefAtIndex(i)) = n;}

  // Exception/throw related
  unsigned       GetThrowsNum()               {return mThrows.GetNum();}
  void           AddThrow(ExceptionNode *n)   {mThrows.PushBack(n);}
  ExceptionNode* GetThrowAtIndex(unsigned i)  {return mThrows.ValueAtIndex(i);}
  void           SetThrowAtIndex(unsigned i, ExceptionNode* n) {*(mThrows.RefAtIndex(i)) = n;}

  void SetType(TreeNode *t) {mType = t;}
  TreeNode* GetType(){return mType;}

  DimensionNode* GetDims()       {return mDims;}
  void SetDims(DimensionNode *t) {mDims = t;}
  unsigned GetDimsNum()          {return mDims->GetDimensionsNum();}
  bool     IsArray()             {return mDims->GetDimensionsNum() > 0;}
  unsigned AddDim(unsigned i = 0){mDims->AddDim(i);}           // 0 means unspecified
  unsigned GetNthNum(unsigned n) {return mDims->GetDimension(n);} // 0 means unspecified.
  void     SetNthNum(unsigned n, unsigned i) {mDims->SetDimension(n, i);}

  // Override equivalent.
  bool OverrideEquivalent(FunctionNode*);

  void Release() { mAttrs.Release();
                   mParams.Release();
                   mAnnotations.Release();
                   if (mDims) mDims->Release();}
  void Dump(unsigned);
};

//////////////////////////////////////////////////////////////////////////
//                         Interface Nodes
// This is from Java originally. Java Annotation is also treated as an
// InterfaceNode.
//////////////////////////////////////////////////////////////////////////

class InterfaceNode : public TreeNode {
private:
  bool        mIsAnnotation;
  SmallVector<InterfaceNode*>  mSuperInterfaces;
  SmallVector<IdentifierNode*> mFields;
  SmallVector<FunctionNode*>   mMethods;
public:
  InterfaceNode() : mIsAnnotation(false) {mKind = NK_Interface;}
  ~InterfaceNode() {}

  unsigned       GetSuperInterfacesNum()              {return mSuperInterfaces.GetNum();}
  void           AddSuperInterface(InterfaceNode* a)  {mSuperInterfaces.PushBack(a);}
  InterfaceNode* GetSuperInterfaceAtIndex(unsigned i) {return mSuperInterfaces.ValueAtIndex(i);}
  void           SetSuperInterfaceAtIndex(unsigned i, InterfaceNode* n) {*(mSuperInterfaces.RefAtIndex(i)) = n;}

  unsigned        GetFieldsNum()              {return mFields.GetNum();}
  void            AddField(IdentifierNode* a) {mFields.PushBack(a);}
  IdentifierNode* GetFieldAtIndex(unsigned i) {return mFields.ValueAtIndex(i);}
  void            SetFieldAtIndex(unsigned i, IdentifierNode* n) {*(mFields.RefAtIndex(i)) = n;}

  unsigned      GetMethodsNum()              {return mMethods.GetNum();}
  void          AddMethod(FunctionNode* a)   {mMethods.PushBack(a);}
  FunctionNode* GetMethodAtIndex(unsigned i) {return mMethods.ValueAtIndex(i);}
  void          SetMethodAtIndex(unsigned i, FunctionNode* n) {*(mMethods.RefAtIndex(i)) = n;}

  void SetIsAnnotation(bool b) {mIsAnnotation = b;}
  bool IsAnnotation()          {return mIsAnnotation;}

  void Construct(BlockNode *);
  void Dump(unsigned);
};

//////////////////////////////////////////////////////////////////////////
//                         Class Nodes
//////////////////////////////////////////////////////////////////////////

class ClassNode : public TreeNode {
private:
  // Java Enum is defined in a similar way as class, with many restrictions
  // and special semantic rules. We define JavaEnum here too. For other
  // languages like C/C++ which have very simply Enum, we will have a
  // dedicated EnumNode for them.
  bool                         mIsJavaEnum;

  SmallVector<ClassNode*>      mSuperClasses;
  SmallVector<InterfaceNode*>  mSuperInterfaces;
  SmallVector<AttrId>          mAttributes;

  SmallVector<IdentifierNode*> mFields;       // aka Field
  SmallVector<FunctionNode*>   mMethods;
  SmallVector<FunctionNode*>   mConstructors;
  SmallVector<BlockNode*>      mInstInits;     // instance initializer
  SmallVector<ClassNode*>      mLocalClasses;
  SmallVector<InterfaceNode*>  mLocalInterfaces;

public:
  ClassNode(){mKind = NK_Class; mIsJavaEnum = false;}
  ~ClassNode() {Release();}

  bool IsJavaEnum() {return mIsJavaEnum;}
  void SetIsJavaEnum(){mIsJavaEnum = true;}

  void AddSuperClass(ClassNode *n)         {mSuperClasses.PushBack(n);}
  void AddSuperInterface(InterfaceNode *n) {mSuperInterfaces.PushBack(n);}
  void AddAttr(AttrId a) {mAttributes.PushBack(a);}

  unsigned GetSuperClassesNum()    {return mSuperClasses.GetNum();}
  unsigned GetSuperInterfacesNum() {return mSuperInterfaces.GetNum();}
  unsigned GetAttributesNum()      {return mAttributes.GetNum();}
  unsigned GetFieldsNum()          {return mFields.GetNum();}
  unsigned GetMethodsNum()         {return mMethods.GetNum();}
  unsigned GetConstructorsNum()     {return mConstructors.GetNum();}
  unsigned GetInstInitsNum()       {return mInstInits.GetNum();}
  unsigned GetLocalClassesNum()    {return mLocalClasses.GetNum();}
  unsigned GetLocalInterfacesNum() {return mLocalInterfaces.GetNum();}
  ClassNode* GetSuperClass(unsigned i)         {return mSuperClasses.ValueAtIndex(i);}
  void       SetSuperClass(unsigned i, ClassNode* n) {*(mSuperClasses.RefAtIndex(i)) = n;}
  InterfaceNode* GetSuperInterface(unsigned i) {return mSuperInterfaces.ValueAtIndex(i);}
  void           SetSuperInterface(unsigned i, InterfaceNode* n) {*(mSuperInterfaces.RefAtIndex(i)) = n;}
  AttrId GetAttribute(unsigned i)              {return mAttributes.ValueAtIndex(i);}
  void   SetAttribute(unsigned i, AttrId n) {*(mAttributes.RefAtIndex(i)) = n;}
  IdentifierNode* GetField(unsigned i)         {return mFields.ValueAtIndex(i);}
  void            SetField(unsigned i, IdentifierNode* n) {*(mFields.RefAtIndex(i)) = n;}
  FunctionNode* GetMethod(unsigned i)          {return mMethods.ValueAtIndex(i);}
  void          SetMethod(unsigned i, FunctionNode* n) {*(mMethods.RefAtIndex(i)) = n;}
  FunctionNode* GetConstructor(unsigned i)     {return mConstructors.ValueAtIndex(i);}
  void          SetConstructor(unsigned i, FunctionNode* n) {*(mConstructors.RefAtIndex(i)) = n;}
  BlockNode* GetInstInit(unsigned i)       {return mInstInits.ValueAtIndex(i);}
  void       SetInstInit(unsigned i, BlockNode* n) {*(mInstInits.RefAtIndex(i)) = n;}
  ClassNode* GetLocalClass(unsigned i)     {return mLocalClasses.ValueAtIndex(i);}
  void       SetLocalClass(unsigned i, ClassNode* n) {*(mLocalClasses.RefAtIndex(i)) = n;}
  InterfaceNode* GetLocalInterface(unsigned i)  {return mLocalInterfaces.ValueAtIndex(i);}
  void           SetLocalInterface(unsigned i, InterfaceNode* n) {*(mLocalInterfaces.RefAtIndex(i)) = n;}

  void Construct(BlockNode*);
  void Release();
  void Dump(unsigned);
};

//////////////////////////////////////////////////////////////////////////
//                         PassNode
// When Autogen creates rule tables, it could create some temp tables without
// any actions. The corresponding AppealNode will be given a PassNode to
// pass its subtrees to the parent.
//
// Also some rules in .spec don't have action at the beginning, and they
// will be given a PassNode too.
//////////////////////////////////////////////////////////////////////////

class PassNode : public TreeNode {
private:
  SmallVector<TreeNode*> mChildren;
public:
  PassNode() {mKind = NK_Pass;}
  ~PassNode() {Release();}

  unsigned  GetChildrenNum() {return mChildren.GetNum();}
  TreeNode* GetChild(unsigned i) {return mChildren.ValueAtIndex(i);}
  void      SetChild(unsigned i, TreeNode* n) {*(mChildren.RefAtIndex(i)) = n;}

  void AddChild(TreeNode *c) {mChildren.PushBack(c); c->SetParent(this);}
  void Dump(unsigned);
  void Release() {mChildren.Release();}
};

////////////////////////////////////////////////////////////////////////////
//                  Lambda Expression
// Java Lambda expression and JS arrow function have similar syntax.
// Also in Typescript, FunctionType and Constructor Type have the similar syntax.
// We put them in the same node.
////////////////////////////////////////////////////////////////////////////

enum LambdaProperty {
  LP_JavaLambda,
  LP_JSArrowFunction,
  LP_TSFunctionType,
  LP_TSConstructorType,
  LP_NA
};

class LambdaNode : public TreeNode {
private:
  LambdaProperty         mProperty;
  TreeNode              *mType;         // The return type. NULL as Java Lambda.
  SmallVector<TreeNode*> mParams;       // A param could be an IdentifierNode or DeclNode.
  TreeNode              *mBody;         // the body is block.
                                        // NULL as TS FunctionType and ConstructorType
public:
  LambdaNode() {mBody = NULL; mKind = NK_Lambda; mProperty = LP_JSArrowFunction; mType = NULL;}
  ~LambdaNode(){Release();}


  TreeNode* GetBody()            {return mBody;}
  void      SetBody(TreeNode *n) {mBody = n; n->SetParent(this);}

  LambdaProperty  GetProperty()                 {return mProperty;}
  void            SetProperty(LambdaProperty p) {mProperty = p;}

  TreeNode* GetType()            {return mType;}
  void      SetType(TreeNode* t) {mType = t;}

  unsigned  GetParamsNum()        {return mParams.GetNum();}
  TreeNode* GetParam(unsigned i)  {return mParams.ValueAtIndex(i);}
  void      SetParam(unsigned i, TreeNode* n) {*(mParams.RefAtIndex(i)) = n;}
  void      AddParam(TreeNode *n) {mParams.PushBack(n); n->SetParent(this);}

  void Release() {mParams.Release();}
  void Dump(unsigned);
};

////////////////////////////////////////////////////////////////////////////
//                  InstanceOf Expression
// This is first created for Java's instanceof operation. It has the form of
// left instanceof right.
////////////////////////////////////////////////////////////////////////////

class InstanceOfNode : public TreeNode {
private:
  TreeNode *mLeft;
  TreeNode *mRight;
public:
  InstanceOfNode() {mLeft = NULL; mRight = NULL; mKind = NK_InstanceOf;}
  ~InstanceOfNode(){Release();}

  TreeNode* GetLeft() {return mLeft;}
  void SetLeft(TreeNode *n) {mLeft = n;}
  TreeNode* GetRight() {return mRight;}
  void SetRight(TreeNode *n){mRight = n;}

  void Dump(unsigned);
};

////////////////////////////////////////////////////////////////////////////
//                  TypeOf Expression
// First coming from typescript.
////////////////////////////////////////////////////////////////////////////

class TypeOfNode : public TreeNode {
private:
  TreeNode *mExpr;
public:
  TypeOfNode() {mExpr = NULL; mKind = NK_TypeOf;}
  ~TypeOfNode(){Release();}

  TreeNode* GetExpr()            {return mExpr;}
  void      SetExpr(TreeNode *n) {mExpr = n;}

  void Dump(unsigned);
};

////////////////////////////////////////////////////////////////////////////
//                  In Expression
// First coming from Javascript. It's like
// A is IN B.
// B is a set of properties. A is one of the properties.
////////////////////////////////////////////////////////////////////////////

class InNode : public TreeNode {
private:
  TreeNode *mLeft;
  TreeNode *mRight;
public:
  InNode() {mLeft = NULL; mRight = NULL; mKind = NK_In;}
  ~InNode(){Release();}

  TreeNode* GetLeft() {return mLeft;}
  void SetLeft(TreeNode *n) {mLeft = n;}
  TreeNode* GetRight() {return mRight;}
  void SetRight(TreeNode *n){mRight = n;}

  void Dump(unsigned);
};

////////////////////////////////////////////////////////////////////////////
//                  The AST Tree
////////////////////////////////////////////////////////////////////////////

class AppealNode;
class ASTBuilder;

class ASTTree {
public:
  TreeNode   *mRootNode;

private:
  // We need a set of functions to deal with some common manipulations of
  // most languages during AST Building. You can disable it if some functions
  // are not what you want.
  TreeNode* Manipulate(AppealNode*);
  TreeNode* Manipulate2Binary(TreeNode*, TreeNode*);
  TreeNode* Manipulate2Cast(TreeNode*, TreeNode*);

public:
  ASTTree();
  ~ASTTree();

  TreeNode* NewTreeNode(AppealNode*);

  TreeNode* BuildBinaryOperation(TreeNode *, TreeNode *, OprId);
  TreeNode* BuildPassNode();

  void Dump(unsigned);

};

}
#endif
