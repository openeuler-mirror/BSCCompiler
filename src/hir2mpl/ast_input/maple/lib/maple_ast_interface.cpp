/*
 * Copyright (c) [2021-2022] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
#include "maple_ast_interface.h"
#include <fstream>
#include <iterator>
#include "gen_aststore.h"
#include "gen_astload.h"
#include "global_tables.h"

namespace maple {
bool LibMapleAstFile::Open(const std::string &fileName) {
  std::ifstream input(fileName, std::ifstream::binary);
  input >> std::noskipws;
  std::istream_iterator<uint8_t> s(input), e;
  maplefe::AstBuffer vec(s, e);
  maplefe::AstLoad loadAst;
  maplefe::ModuleNode *mod = loadAst.LoadFromAstBuf(vec);
  // add mod to the vector
  while (mod) {
    handler.AddModule(mod);
    mod = loadAst.Next();
  }
  return true;
}

PrimType LibMapleAstFile::MapPrim(maplefe::TypeId id) {
  PrimType prim;
  switch (id) {
    case maplefe::TY_Boolean:
      prim = PTY_u1;
      break;
    case maplefe::TY_Byte:
      prim = PTY_u8;
      break;
    case maplefe::TY_Short:
      prim = PTY_i16;
      break;
    case maplefe::TY_Int:
      prim = PTY_i32;
      break;
    case maplefe::TY_Long:
      prim = PTY_i64;
      break;
    case maplefe::TY_Char:
      prim = PTY_u16;
      break;
    case maplefe::TY_Float:
      prim = PTY_f32;
      break;
    case maplefe::TY_Double:
      prim = PTY_f64;
      break;
    case maplefe::TY_Void:
      prim = PTY_void;
      break;
    case maplefe::TY_Null:
      prim = PTY_void;
      break;
    default:
      CHECK_FATAL(false, "Unsupported PrimType");
      break;
  }
  return prim;
}

MIRType *LibMapleAstFile::MapPrimType(maplefe::TypeId id) {
  PrimType prim = MapPrim(id);
  TyIdx tid(prim);
  return GlobalTables::GetTypeTable().GetTypeFromTyIdx(tid);
}

MIRType *LibMapleAstFile::MapPrimType(maplefe::PrimTypeNode *ptnode) {
  return MapPrimType(ptnode->GetPrimType());
}

MIRType *LibMapleAstFile::MapType(maplefe::TreeNode *type) {
  if (type == nullptr) {
    return GlobalTables::GetTypeTable().GetVoid();
  }

  MIRType *mirType = nullptr;
  if (type->IsPrimType()) {
    maplefe::PrimTypeNode *ptnode = static_cast<maplefe::PrimTypeNode*>(type);
    mirType = MapPrimType(ptnode);
  } else if (type->IsUserType()) {
    if (type->IsIdentifier()) {
      maplefe::IdentifierNode *inode = static_cast<maplefe::IdentifierNode*>(type);
      mirType = MapType(inode->GetType());
    } else {
      CHECK_FATAL(false, "MapType IsUserType");
    }
  } else {
    CHECK_FATAL(false, "MapType unknown type");
  }

  return mirType;
}
}