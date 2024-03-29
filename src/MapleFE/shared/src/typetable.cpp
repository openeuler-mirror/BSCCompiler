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
//                                                                          //
// This file contains the implementation of string pool.                    //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#include <cstdlib>
#include <cstring>

#include "typetable.h"
#include "gen_astdump.h"

namespace maplefe {

TypeTable gTypeTable;

TypeEntry::TypeEntry(TreeNode *node) {
  mType = node;
  if (!node->IsTypeIdNone()) {
    mTypeId = node->GetTypeId();
  } else {
    switch (node->GetKind()) {
      case NK_Struct:
      case NK_StructLiteral:
      case NK_Class:
      case NK_Interface:
        mTypeId = TY_Class;
        break;
      case NK_ArrayLiteral:
        mTypeId = TY_Array;
        break;
      case NK_UserType:
        mTypeId = TY_User;
        break;
      default:
        mTypeId = TY_None;
        break;
    }
  }
}

TreeNode *TypeTable::CreatePrimType(std::string name, TypeId tid) {
  unsigned stridx = gStringPool.GetStrIdx(name);
  PrimTypeNode *ptype = (PrimTypeNode*)gTreePool.NewTreeNode(sizeof(PrimTypeNode));
  new (ptype) PrimTypeNode();
  ptype->SetStrIdx(stridx);
  ptype->SetPrimType(tid);
  ptype->SetTypeId(tid);

  mTypeId2TypeMap[tid] = ptype;
  return ptype;
}

TreeNode *TypeTable::CreateBuiltinType(std::string name, TypeId tid) {
  unsigned stridx = gStringPool.GetStrIdx(name);
  IdentifierNode *id = (IdentifierNode*)gTreePool.NewTreeNode(sizeof(IdentifierNode));
  new (id) IdentifierNode(stridx);
  // use TY_Class for Object type
  (tid == TY_Object) ? id->SetTypeId(TY_Class) : id->SetTypeId(tid);

  UserTypeNode *utype = (UserTypeNode*)gTreePool.NewTreeNode(sizeof(UserTypeNode));
  new (utype) UserTypeNode(id);
  utype->SetStrIdx(stridx);
  utype->SetTypeId(TY_Class);
  id->SetParent(utype);

  mTypeId2TypeMap[tid] = utype;
  mStrIdx2BuiltInTypeIdxMap[stridx] = tid;
  return utype;
}

bool TypeTable::AddType(TreeNode *node) {
  unsigned nid = node->GetNodeId();
  if (mNodeId2TypeIdxMap.find(nid) != mNodeId2TypeIdxMap.end()) {
    return false;
  }
  unsigned tidx = mTypeTable.size();
  mNodeId2TypeIdxMap[nid] = tidx;
  node->SetTypeIdx(tidx);
  if (node->IsUserType()) {
    static_cast<UserTypeNode*>(node)->GetId()->SetTypeIdx(tidx);
  }
  TypeEntry *entry = new TypeEntry(node);
  mTypeTable.push_back(entry);
  return true;
}

void TypeTable::AddPrimTypeId(TypeId tid) {
  mPrimTypeId.insert(tid);
}

#undef  TYPE
#undef  PRIMTYPE
void TypeTable::AddPrimAndBuiltinTypes() {
  // only initialize once
  if (mTypeTable.size() != 0) {
    return;
  }

  TreeNode *node;
  // add a NULL entry so real typeidx starting from 1
  TypeEntry *entry = new TypeEntry();
  mTypeTable.push_back(entry);

  // first are primitive types, and their typeid TY_Xyz is their typeidx as well
#define TYPE(T)
#define PRIMTYPE(T) node = CreatePrimType(#T, TY_##T); AddType(node); AddPrimTypeId(TY_##T);
#include "supported_types.def"
  // add additional primitive types for number and string
  PRIMTYPE(Number);
  PRIMTYPE(String);

  mPrimSize = size();

#define TYPE(T) node = CreateBuiltinType(#T, TY_##T); AddType(node);
#define PRIMTYPE(T)
  // additional usertype Boolean
  TYPE(Boolean);
#include "supported_types.def"

  mPreBuildSize = size();

  // add language builtin types
  unsigned stridx = 0;
#define BUILTIN(T) \
  stridx = gStringPool.GetStrIdx(#T);\
  if (mStrIdx2BuiltInTypeIdxMap.find(stridx) == mStrIdx2BuiltInTypeIdxMap.end()) {\
    node = CreateBuiltinType(#T, TY_Class);\
    AddType(node);\
    mStrIdx2BuiltInTypeIdxMap[stridx] = node->GetTypeIdx();\
  }
#include "lang_builtin.def"
  return;
}

bool TypeTable::IsBuiltInType(TreeNode *node) {
  return mStrIdx2BuiltInTypeIdxMap.find(node->GetStrIdx()) != mStrIdx2BuiltInTypeIdxMap.end();
}

unsigned TypeTable::GetBuiltInTypeIdx(unsigned stridx) {
  if (mStrIdx2BuiltInTypeIdxMap.find(stridx) != mStrIdx2BuiltInTypeIdxMap.end()) {
    return mStrIdx2BuiltInTypeIdxMap[stridx];
  }
  return 0;
}

unsigned TypeTable::GetBuiltInTypeIdx(TreeNode *node) {
  unsigned stridx = node->GetStrIdx();
  if (mStrIdx2BuiltInTypeIdxMap.find(stridx) != mStrIdx2BuiltInTypeIdxMap.end()) {
    return GetBuiltInTypeIdx(stridx);
  }
  return 0;
}

TypeEntry *TypeTable::GetTypeEntryFromTypeIdx(unsigned tidx) {
  MASSERT(tidx < mTypeTable.size() && "type index out of range");
  return mTypeTable[tidx];
}

TreeNode *TypeTable::GetTypeFromTypeIdx(unsigned tidx) {
  MASSERT(tidx < mTypeTable.size() && "type index out of range");
  return mTypeTable[tidx]->GetType();
}

TreeNode *TypeTable::GetTypeFromStrIdx(unsigned stridx) {
  for (auto entry : mTypeTable) {
    TreeNode *node = entry->GetType();
    if (node && node->GetStrIdx() == stridx) {
      return node;
    }
  }
  return NULL;
}

unsigned TypeTable::GetOrCreateFunctionTypeIdx(FunctionTypeNode *node) {
  for (auto tidx: mFuncTypeIdx) {
    TreeNode *type = GetTypeFromTypeIdx(tidx);
    FunctionTypeNode *functype = static_cast<FunctionTypeNode *>(type);
    bool found = functype->IsEqual(node);
    if (found) {
      return tidx;
    }
  }
  bool status = AddType(node);
  MASSERT(status && "failed to add a functiontype");
  unsigned tidx = node->GetTypeIdx();
  mFuncTypeIdx.insert(tidx);

  std::string str("FuncType__");
  str += std::to_string(tidx);
  unsigned stridx = gStringPool.GetStrIdx(str);
  node->SetStrIdx(stridx);

  return tidx;
}

void TypeTable::Dump() {
  std::cout << "===================== TypeTable =====================" << std::endl;
  std::cout << " tid:type-name: node-kind  node-id" << std::endl;
  std::cout << "--------------------------------" << std::endl;
  unsigned idx = 1;
  for (unsigned idx = 1; idx < mTypeTable.size(); idx++) {
    TypeEntry *entry = mTypeTable[idx];
    TreeNode *node = entry->GetType();
    TypeId tid = node->GetTypeId();
    if (node->IsUserType()) {
      tid = static_cast<UserTypeNode*>(node)->GetId()->GetTypeId();
    }
    std::cout << "  " << idx << " : " << node->GetName() << " : " <<
              AstDump::GetEnumNodeKind(node->GetKind()) << " " <<
              AstDump::GetEnumTypeId(tid) << " " <<
              "(typeid " << tid << ") " <<
              "(typeidx " << node->GetTypeIdx() << ") " <<
              "(stridx " << node->GetStrIdx() << ") " <<
              "(nodeid " << node->GetNodeId() << ")" << std::endl;
  }
  std::cout << "===================== End TypeTable =====================" << std::endl;
}

}

