/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MPLFE_AST_INPUT_INCLUDE_AST_DECL_BUILDER_H
#define MPLFE_AST_INPUT_INCLUDE_AST_DECL_BUILDER_H
#include "ast_decl.h"
#include "mempool_allocator.h"

namespace maple {
static std::map<int64, ASTDecl*> declesTable;
class ASTDeclsBuilder {
 public:
  static ASTDecl *GetASTDecl(int64 id) {
    ASTDecl *decl = declesTable[id];
    return decl;
  }

  static ASTDecl *ASTDeclBuilder(MapleAllocator &allocator, const std::string &srcFile,
      const std::string &nameIn, const std::vector<MIRType*> &typeDescIn, int64 id = INT64_MAX) {
    if (id == INT64_MAX) {
      return allocator.GetMemPool()->New<ASTDecl>(srcFile, nameIn, typeDescIn);  // for temp decl
    } else if (declesTable[id] == nullptr) {
      declesTable[id] = allocator.GetMemPool()->New<ASTDecl>(srcFile, nameIn, typeDescIn);
    }
    return declesTable[id];
  }

  static ASTVar *ASTVarBuilder(MapleAllocator &allocator, const std::string &srcFile, const std::string &varName,
      const std::vector<MIRType*> &desc, const GenericAttrs &genAttrsIn, int64 id = INT64_MAX) {
    if (id == INT64_MAX) {
      return allocator.GetMemPool()->New<ASTVar>(srcFile, varName, desc, genAttrsIn);
    } else if (declesTable[id] == nullptr) {
      declesTable[id] = allocator.GetMemPool()->New<ASTVar>(srcFile, varName, desc, genAttrsIn);
    }
    return static_cast<ASTVar*>(declesTable[id]);
  }

  static ASTLocalEnumDecl *ASTLocalEnumDeclBuilder(MapleAllocator &allocator, const std::string &srcFile,
      const std::string &varName, const std::vector<MIRType*> &desc, const GenericAttrs &genAttrsIn) {
    return allocator.GetMemPool()->New<ASTLocalEnumDecl>(srcFile, varName, desc, genAttrsIn);
  }

  static ASTFunc *ASTFuncBuilder(MapleAllocator &allocator, const std::string &srcFile, const std::string &nameIn,
                                 const std::vector<MIRType*> &typeDescIn, const GenericAttrs &genAttrsIn,
                                 const std::vector<std::string> &parmNamesIn, int64 id = INT64_MAX) {
    if (id == INT64_MAX) {
      allocator.GetMemPool()->New<ASTFunc>(srcFile, nameIn, typeDescIn, genAttrsIn, parmNamesIn);
    } else if (declesTable[id] == nullptr) {
      declesTable[id] = allocator.GetMemPool()->New<ASTFunc>(srcFile, nameIn, typeDescIn, genAttrsIn, parmNamesIn);
    }
    return static_cast<ASTFunc*>(declesTable[id]);
  }

  template<typename T>
  static T *ASTStmtBuilder(MapleAllocator &allocator) {
    return allocator.GetMemPool()->New<T>();
  }

  template<typename T>
  static T *ASTExprBuilder(MapleAllocator &allocator) {
    return allocator.GetMemPool()->New<T>();
  }

  static ASTStruct *ASTStructBuilder(MapleAllocator &allocator, const std::string &srcFile, const std::string &nameIn,
      const std::vector<MIRType*> &typeDescIn, const GenericAttrs &genAttrsIn, int64 id = INT64_MAX) {
    if (id == INT64_MAX) {
      allocator.GetMemPool()->New<ASTStruct>(srcFile, nameIn, typeDescIn, genAttrsIn);
    } else if (declesTable[id] == nullptr) {
      declesTable[id] = allocator.GetMemPool()->New<ASTStruct>(srcFile, nameIn, typeDescIn, genAttrsIn);
    }
    return static_cast<ASTStruct*>(declesTable[id]);
  }

  static ASTField *ASTFieldBuilder(MapleAllocator &allocator, const std::string &srcFile, const std::string &varName,
      const std::vector<MIRType*> &desc, const GenericAttrs &genAttrsIn) {
    return allocator.GetMemPool()->New<ASTField>(srcFile, varName, desc, genAttrsIn);
  }
};
}  // namespace maple
#endif  // MPLFE_AST_INPUT_INCLUDE_AST_DECL_BUILDER_H
