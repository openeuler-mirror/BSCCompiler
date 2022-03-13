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
//////////////////////////////////////////////////////////////////////////////
// This file contains the Memory Pool for String pool.                      //
//                                                                          //
// A String is stored in the pool with an ending '\0'.                      //
//////////////////////////////////////////////////////////////////////////////

#ifndef __TYPETABLE_H__
#define __TYPETABLE_H__

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include "massert.h"
#include "ast.h"
#include "ast_type.h"

namespace maplefe {

class TypeEntry {
 private:
  TreeNode *mType;
  TypeId    mTypeId;

 public:
  TypeEntry() : mType(NULL), mTypeId(TY_None) {}
  TypeEntry(TreeNode *node);
  ~TypeEntry(){};

  TypeId    GetTypeId()   { return mTypeId; }
  TreeNode *GetType()     { return mType; }

  void SetTypeId(TypeId i)     { mTypeId = i; }
  void SetType(TreeNode *n)    { mType = n; }
};

class TypeTable {
private:
  std::vector<TypeEntry *> mTypeTable;
  std::unordered_map<unsigned, unsigned> mNodeId2TypeIdxMap;
  std::unordered_map<TypeId, TreeNode*> mTypeId2TypeMap;
  std::unordered_set<TypeId> mPrimTypeId;
  std::unordered_set<unsigned> mFuncTypeIdx;
  unsigned mPrimSize;
  unsigned mPreBuildSize;

public:
  TypeTable() {};
  ~TypeTable() { mTypeTable.clear(); };

  unsigned size() { return mTypeTable.size(); }
  unsigned GetPreBuildSize() { return mPreBuildSize; }

  bool IsPrimTypeId(TypeId tid) { return mPrimTypeId.find(tid) != mPrimTypeId.end(); }
  unsigned GetPrimSize() { return mPrimSize; }
  TreeNode *CreatePrimType(std::string name, TypeId tid);
  TreeNode *CreateBuiltinType(std::string name, TypeId tid);

  void AddPrimTypeId(TypeId tid);
  void AddPrimAndBuiltinTypes();
  bool AddType(TreeNode *node);

  TypeEntry *GetTypeEntryFromTypeIdx(unsigned tidx);
  TreeNode  *GetTypeFromTypeIdx(unsigned tidx);
  TreeNode  *GetTypeFromTypeId(TypeId tid) { return mTypeId2TypeMap[tid]; }
  TreeNode  *GetTypeFromStrIdx(unsigned strid);

  unsigned GetOrCreateFunctionTypeIdx(FunctionTypeNode *type);

  void Dump();
};

extern TypeTable gTypeTable;


}
#endif  // __TYPETABLE_H__
