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
#ifndef HIR2MPL_AST_INPUT_INCLUDE_AST_DECL_BUILDER_H
#define HIR2MPL_AST_INPUT_INCLUDE_AST_DECL_BUILDER_H
#include "ast_decl.h"
#include "mempool_allocator.h"

namespace maple {
class ASTDeclsBuilder {
 public:
  static ASTDecl *GetASTDecl(int64 id) {
    ASTDecl *decl = declesTable[id];
    return decl;
  }

  static ASTDecl *ASTDeclBuilder(const MapleAllocator &allocator, const std::string &srcFile,
      const std::string &nameIn, const std::vector<MIRType*> &typeDescIn, int64 id = INT64_MAX) {
    if (id == INT64_MAX) {
      return allocator.GetMemPool()->New<ASTDecl>(srcFile, nameIn, typeDescIn);  // for temp decl
    } else if (declesTable[id] == nullptr) {
      declesTable[id] = allocator.GetMemPool()->New<ASTDecl>(srcFile, nameIn, typeDescIn);
    }
    return declesTable[id];
  }

  static ASTVar *ASTVarBuilder(const MapleAllocator &allocator, const std::string &srcFile, const std::string &varName,
      const std::vector<MIRType*> &desc, const GenericAttrs &genAttrsIn, int64 id = INT64_MAX) {
    if (id == INT64_MAX) {
      return allocator.GetMemPool()->New<ASTVar>(srcFile, varName, desc, genAttrsIn);
    } else if (declesTable[id] == nullptr) {
      declesTable[id] = allocator.GetMemPool()->New<ASTVar>(srcFile, varName, desc, genAttrsIn);
    }
    return static_cast<ASTVar*>(declesTable[id]);
  }

  static ASTEnumConstant *ASTEnumConstBuilder(const MapleAllocator &allocator, const std::string &srcFile,
      const std::string &varName, const std::vector<MIRType*> &desc,
      const GenericAttrs &genAttrsIn, int64 id = INT64_MAX) {
    if (id == INT64_MAX) {
      return allocator.GetMemPool()->New<ASTEnumConstant>(srcFile, varName, desc, genAttrsIn);
    } else if (declesTable[id] == nullptr) {
      declesTable[id] = allocator.GetMemPool()->New<ASTEnumConstant>(srcFile, varName, desc, genAttrsIn);
    }
    return static_cast<ASTEnumConstant*>(declesTable[id]);
  }

  static ASTEnumDecl *ASTLocalEnumDeclBuilder(const MapleAllocator &allocator, const std::string &srcFile,
      const std::string &varName, const std::vector<MIRType*> &desc, const GenericAttrs &genAttrsIn,
      int64 id = INT64_MAX) {
    if (id == INT64_MAX) {
      return allocator.GetMemPool()->New<ASTEnumDecl>(srcFile, varName, desc, genAttrsIn);
    } else if (declesTable[id] == nullptr) {
      declesTable[id] = allocator.GetMemPool()->New<ASTEnumDecl>(srcFile, varName, desc, genAttrsIn);
    }
    return static_cast<ASTEnumDecl*>(declesTable[id]);
  }

  static ASTFunc *ASTFuncBuilder(const MapleAllocator &allocator, const std::string &srcFile, const std::string &nameIn,
                                 const std::vector<MIRType*> &typeDescIn, const GenericAttrs &genAttrsIn,
                                 const std::vector<ASTDecl*> &paramDeclsIn, int64 id = INT64_MAX) {
    if (id == INT64_MAX) {
      return allocator.GetMemPool()->New<ASTFunc>(srcFile, nameIn, typeDescIn, genAttrsIn, paramDeclsIn);
    } else if (declesTable[id] == nullptr) {
      declesTable[id] = allocator.GetMemPool()->New<ASTFunc>(srcFile, nameIn, typeDescIn, genAttrsIn, paramDeclsIn);
    }
    return static_cast<ASTFunc*>(declesTable[id]);
  }

  template<typename T>
  static T *ASTStmtBuilder(const MapleAllocator &allocator) {
    return allocator.GetMemPool()->New<T>();
  }

  template<typename T>
  static T *ASTExprBuilder(const MapleAllocator &allocator) {
    return allocator.GetMemPool()->New<T>();
  }

  static ASTStruct *ASTStructBuilder(const MapleAllocator &allocator, const std::string &srcFile, const std::string &nameIn,
      const std::vector<MIRType*> &typeDescIn, const GenericAttrs &genAttrsIn, int64 id = INT64_MAX) {
    if (id == INT64_MAX) {
      allocator.GetMemPool()->New<ASTStruct>(srcFile, nameIn, typeDescIn, genAttrsIn);
    } else if (declesTable[id] == nullptr) {
      declesTable[id] = allocator.GetMemPool()->New<ASTStruct>(srcFile, nameIn, typeDescIn, genAttrsIn);
    }
    return static_cast<ASTStruct*>(declesTable[id]);
  }

  static ASTField *ASTFieldBuilder(const MapleAllocator &allocator, const std::string &srcFile, const std::string &varName,
      const std::vector<MIRType*> &desc, const GenericAttrs &genAttrsIn, int64 id = INT64_MAX,
      bool isAnonymous = false) {
    if (id == INT64_MAX) {
      allocator.GetMemPool()->New<ASTField>(srcFile, varName, desc, genAttrsIn, isAnonymous);
    } else if (declesTable[id] == nullptr) {
      declesTable[id] = allocator.GetMemPool()->New<ASTField>(srcFile, varName, desc, genAttrsIn, isAnonymous);
    }
    return static_cast<ASTField*>(declesTable[id]);
  }

 private:
  static std::map<int64, ASTDecl*> declesTable;
};
}  // namespace maple
#endif  // HIR2MPL_AST_INPUT_INCLUDE_AST_DECL_BUILDER_H
