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
/////////////////////////////////////////////////////////////////////
// This is the base functions of auto generation.                  //
// Each individual part is supposed to inherit from this class.    //
/////////////////////////////////////////////////////////////////////

#ifndef __PARSER_H__
#define __PARSER_H__

#include <iostream>
#include <fstream>
#include <stack>
#include <list>
#include <map>

#include "feopcode.h"
#include "lexer.h"

// tyidx for int for the time being
#define inttyidx 1

class Automata;
class Module;
class Function;
class Stmt;
class Token;
class RuleTable;
class TableData;
class ASTTree;
class TreeNode;

typedef enum {
  FailWasFailed,
  FailLooped,
  FailNotIdentifier,
  FailNotLiteral,
  FailChildrenFailed,
  Succ,
  SuccWasSucc,
  NA
}AppealStatus;

class AppealNode;
class AppealNode{
public:
  bool mIsTable;     // A AppealNode could relate to either rule table or token.
  bool mIsSecondTry; // The node is created after second try. This flag is used during SortOut.
public:
  union {
    RuleTable *mTable;
    Token     *mToken;
  }mData;
  unsigned   mStartIndex;     // index of start matching token
  std::list<AppealNode*> mChildren;   // Use list instead of vector since SortOut
                                      // will remove some nodes, and it happens often

  // I use an additional vector for the sorted out children. Why do we have two duplicated
  // children vectors? The reason is coming from sortout. After SortOut we need remove some
  // failed children and only keep the successful children. However, a successful child could
  // be SuccWasSucc, and the real successfully matching sub-tree could be hidden in a previously
  // faild tree.
  //
  // During AST tree generation, for the SuccWasSucc child we need find the original matching
  // tree. That means the original mChildren vector needs to be traversed to locate that tree.
  // So we keep mChildren untouched and define a second vector for the SortOut-ed children.
  std::vector<AppealNode*> mSortedChildren;

  AppealNode *mParent;
  AppealStatus mBefore;
  AppealStatus mAfter;

  AppealNode() {mData.mTable=NULL; mParent = NULL; mBefore = NA; mAfter = NA;
                mIsTable = true; mIsSecondTry = false;}
  ~AppealNode(){}

  void AddChild(AppealNode *n) { mChildren.push_back(n); }
  void RemoveChild(AppealNode *n);
  void ClearChildren() { mChildren.clear(); }

  bool IsSucc() { return (mAfter == Succ) || (mAfter == SuccWasSucc); }
  bool IsFail() { return !IsSucc(); }

  bool IsTable(){ return mIsTable; }
  bool IsToken(){ return !mIsTable; }
  void SetTable(RuleTable *t) { mIsTable = true; mData.mTable = t; }
  void SetToken(Token *t)     { mIsTable = false; mData.mToken = t; }
  RuleTable* GetTable() { return mData.mTable; }
  Token*     GetToken() { return mData.mToken; }
};

// Design of the cached success info.
// Starting from a certian token, a rule can have multiple matchings, for example,
// an expression,  this.x, can be matched by Primary either by one token 'this', or
// three tokens 'this.x'.
//
// Assuming index of 'this' is 9, we say Primary has two matches indicated by two numbers
// 9 and 11, which represent the last successfully matched token.
//
// To cache all the successful results of a rule, we define a vector. The vector is composed
// of a set of segments with variable length. One segment represents the successful matches
// starting from a specific token. The structure is as below.
//      9, 2, 9, 11
// the first number 9 is the starting token. The second number, 2, is the number of matches.
// The third number and the forth number are the last successfully matched tokens.
// If the rule has 3 matches for token 20, its segment is 20, 3, 20, 22, 23
// Put them together, we get the mSucc for this rule 9, 2, 9, 11, 20, 3, 20, 22, 23

// This is a per-rule data structure, saving the success info of a rule.
class SuccMatch {
public:
  // I use vector instead of list, although we will remove some elements in SortOut.
  // The reason is we will use operator[] a lot during query. Vector is better.
  std::vector<unsigned> mCache;
private:
  unsigned mTempIndex;          // A temporary index
public:
  SuccMatch(){}
  ~SuccMatch() {mCache.clear();}
public:
  // Usually a rule may have a few matches, less than 3. So we define 3 special interfaces.
  void AddOneMatch(unsigned t, unsigned m);                               // rule has only 1 match
  void AddTwoMatch(unsigned t, unsigned m1, unsigned m2);                 // rule has only 2 match
  void AddThreeMatch(unsigned t, unsigned m1, unsigned m2, unsigned m3);  // rule has only 3 match
  // For general cases we will append matches one by one. The following two
  // functions will be used together, as the first one set the start token, the second one
  // add the end matching tokens one by one.
  void AddStartToken(unsigned token);
  void AddOneMoreMatch(unsigned last_succ_token);  // add one more match to the cach

  // Reduce tokens, used during SortOut

  // Query functions.
  // The four function need to be used together since mTempIndex is only defined in
  // GetStartToken(unsigned).
  bool     GetStartToken(unsigned t);    // trying to get succ info for 't'
  unsigned GetMatchNum();                // number of matches at a token;
  unsigned GetOneMatch(unsigned i);      // Get the i-th matching token. Starts from 0.
  bool     ReduceMatches(unsigned starttoken, unsigned val);    // Reduce all matches except 'val'.
  void     ReduceMatches(unsigned idx);    // Reduce all matches except idx-th.
};

class Parser {
public:
  Lexer *mLexer;
  const char *filename;
  Automata *mAutomata;
  Module *mModule;
  Function *currfunc;

  std::vector<std::string> mVars;

  // debug info
  unsigned mIndentation;    //
  bool mTraceTable;         // trace enter/exit rule tables
  bool mTraceAppeal;        // trace appealing
  bool mTraceSecondTry;     // trace second try in parser.
  bool mTraceFailed;        // trace mFailed
  bool mTraceVisited;       // trace mVisitedStack
  bool mTraceSortOut;       // trace Sort out.
  bool mTraceWarning;       // print the warning.

  const char* GetRuleTableName(const RuleTable*);
  void DumpEnterTable(const char *tablename, unsigned indent);
  void DumpExitTable(const char *tablename, unsigned indent,
                     bool succ, AppealStatus reason = Succ);
  void DumpAppeal(RuleTable *table, unsigned token);
  void DumpSuccTokens();
  void DumpSortOut();
  void DumpSortOutNode(AppealNode*);

private:
  std::vector<Token*>   mTokens;         // Storage of all tokens, including active, discarded,
                                         // and pending.
  std::vector<Token*>   mActiveTokens;   // vector for tokens during matching.
  std::vector<unsigned> mStartingTokens; // The starting token of each self-complete statement.
                                         // It's an index of mActiveTokens.
  unsigned              mCurToken;       // index in mActiveTokens, the next token to be matched.
  unsigned              mPending;        // index in mActiveTokens, the first pending token.
                                         // All tokens after it are pending.

  bool                  mInSecondTry;    // A temporary flag to tell we are in second try.

  // I'm using two data structures to record the status of cycle reference.
  // See the detailed comments in the implementation of Parser::Parse().
  //   1. mVisited tells if we are in a loop.
  //   2. mVisitedStack tells the token position of each iteration in the loop
  std::map<RuleTable *, bool> mVisited;
  std::map<RuleTable *, std::vector<unsigned>> mVisitedStack;

  // Using this map to record all the failed tokens for a rule.
  // See the detailed comments in Parser::Parse().
  std::map<RuleTable *, std::vector<unsigned>> mFailed;

  // Using this map to record all the succ info of rules.
  std::map<RuleTable*, SuccMatch*> mSucc;
  std::vector<SuccMatch*> mSuccPool;
  SuccMatch* FindSucc(RuleTable*);
  SuccMatch* FindOrCreateSucc(RuleTable*);
  void ClearSucc();

  bool TraverseStmt();                              // success if all tokens are matched.
  bool TraverseRuleTable(RuleTable*, AppealNode*);  // success if all tokens are matched.
  bool TraverseTableData(TableData*, AppealNode*);  // success if all tokens are matched.
  bool TraverseConcatenate(RuleTable*, AppealNode*);
  bool TraverseOneof(RuleTable*, AppealNode*);
  bool TraverseZeroormore(RuleTable*, AppealNode*);
  bool TraverseZeroorone(RuleTable*, AppealNode*);
  bool TraverseLiteral(RuleTable*, AppealNode*);
  bool TraverseIdentifier(RuleTable*, AppealNode*);

  bool IsVisited(RuleTable*);
  void SetVisited(RuleTable*);
  void ClearVisited(RuleTable*);
  void VisitedPush(RuleTable*);
  void VisitedPop(RuleTable*);

  void ClearFailed() {mFailed.clear();}
  void AddFailed(RuleTable*, unsigned);
  void ResetFailed(RuleTable*, unsigned);
  bool WasFailed(RuleTable*, unsigned);

  bool MoveCurToken();  // move mCurToken one step.

  // Every language has a fixed number of entry point to start parsing. Eg. in Java
  // the top level language construct is class, so the entry point is ClassDeclaration.
  // In C, the top level construct is either function or statement.
  //
  // In driver function of parser needs to set up these top level rule tables which
  // the traversal will start with.
  std::vector<RuleTable*> mTopTables;

  // Appealing System
  std::vector<AppealNode*> mAppealNodes;
  AppealNode *mRootNode;
  void ClearAppealNodes();
  void Appeal(AppealNode*);
  void AppealTraverse(AppealNode *node, AppealNode *root);

  // Sort Out
  std::deque<AppealNode*> to_be_sorted;  // a temp data structure during sort out.
  void SortOut();
  void SortOutNode(AppealNode*);
  void SortOutOneof(AppealNode*);
  void SortOutZeroormore(AppealNode*);
  void SortOutZeroorone(AppealNode*);
  void SortOutConcatenate(AppealNode*);
  void SortOutData(AppealNode*);
  void CleanFailedSecondTry(AppealNode*);

  // Build AST
  std::vector<ASTTree*> mASTTrees;      // All AST trees in this module
  ASTTree*  BuildAST(AppealNode*); // Each top level construct gets a AST
  void      SimplifySortedTree(AppealNode*);
  TreeNode* NewTreeNode(ASTTree*, AppealNode*);

public:
  Parser(const char *f, Module *m);
  Parser(const char *f);
  ~Parser();

  TK_Kind GetTokenKind(const char c);
  TK_Kind GetTokenKind(const char *str);

  std::string GetTokenKindString(const TK_Kind tk) { return mLexer->GetTokenKindString(tk); }

  FEOpcode GetFEOpcode(const char c);
  FEOpcode GetFEOpcode(const char *str);

  void SetVerbose(int i) { mLexer->SetVerbose(i); }
  int GetVerbose() { mLexer->GetVerbose(); }

  void Dump();

  bool Parse();
  bool ParseStmt();
  void InitPredefinedTokens();
  void SetupTopTables();  //Each language parser will implement this by itself. 
  unsigned LexOneLine();
};

#endif
