/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
  static ASTDeclsBuilder &GetInstance(MapleAllocator &allocator) {
    static ASTDeclsBuilder instance(allocator);
    return instance;
  }

  ASTDecl *GetASTDecl(int64 id) {
    ASTDecl *decl = declesTable[id];
    return decl;
  }

  void Clear() {
    declesTable.clear();
  }

  ASTDecl *ASTDeclBuilder(const MapleAllocator &allocator, const MapleString &srcFile,
      const std::string &nameIn, const MapleVector<MIRType*> &typeDescIn, int64 id = INT64_MAX) {
    MapleString nameStr(nameIn, allocator.GetMemPool());
    if (id == INT64_MAX) {
      return allocator.GetMemPool()->New<ASTDecl>(srcFile, nameStr, typeDescIn);  // for temp decl
    } else if (declesTable[id] == nullptr) {
      declesTable[id] = allocator.GetMemPool()->New<ASTDecl>(srcFile, nameStr, typeDescIn);
    }
    return declesTable[id];
  }

  ASTVar *ASTVarBuilder(const MapleAllocator &allocator, const MapleString &srcFile, const std::string &varName,
      const MapleVector<MIRType*> &desc, const GenericAttrs &genAttrsIn, int64 id = INT64_MAX) {
    MapleString varNameStr(varName, allocator.GetMemPool());
    if (id == INT64_MAX) {
      return allocator.GetMemPool()->New<ASTVar>(srcFile, varNameStr, desc, genAttrsIn);
    } else if (declesTable[id] == nullptr) {
      declesTable[id] = allocator.GetMemPool()->New<ASTVar>(srcFile, varNameStr, desc, genAttrsIn);
    }
    return static_cast<ASTVar*>(declesTable[id]);
  }

  ASTEnumConstant *ASTEnumConstBuilder(const MapleAllocator &allocator, const MapleString &srcFile,
      const std::string &varName, const MapleVector<MIRType*> &desc,
      const GenericAttrs &genAttrsIn, int64 id = INT64_MAX) {
    MapleString varNameStr(varName, allocator.GetMemPool());
    if (id == INT64_MAX) {
      return allocator.GetMemPool()->New<ASTEnumConstant>(srcFile, varNameStr, desc, genAttrsIn);
    } else if (declesTable[id] == nullptr) {
      declesTable[id] = allocator.GetMemPool()->New<ASTEnumConstant>(srcFile, varNameStr, desc, genAttrsIn);
    }
    return static_cast<ASTEnumConstant*>(declesTable[id]);
  }

  ASTEnumDecl *ASTLocalEnumDeclBuilder(MapleAllocator &allocator, const MapleString &srcFile,
      const std::string &varName, const MapleVector<MIRType*> &desc, const GenericAttrs &genAttrsIn,
      int64 id = INT64_MAX) {
    MapleString varNameStr(varName, allocator.GetMemPool());
    if (id == INT64_MAX) {
      return allocator.GetMemPool()->New<ASTEnumDecl>(allocator, srcFile, varNameStr, desc, genAttrsIn);
    } else if (declesTable[id] == nullptr) {
      declesTable[id] = allocator.GetMemPool()->New<ASTEnumDecl>(allocator, srcFile, varNameStr, desc, genAttrsIn);
    }
    return static_cast<ASTEnumDecl*>(declesTable[id]);
  }

  ASTFunc *ASTFuncBuilder(const MapleAllocator &allocator, const MapleString &srcFile,
                          const std::string &originalNameIn, const std::string &nameIn,
                          const MapleVector<MIRType*> &typeDescIn, const GenericAttrs &genAttrsIn,
                          MapleVector<ASTDecl*> &paramDeclsIn, int64 id = INT64_MAX) {
    MapleString originalFuncName(originalNameIn, allocator.GetMemPool());
    MapleString funcNameStr(nameIn, allocator.GetMemPool());
    if (id == INT64_MAX) {
      return allocator.GetMemPool()->New<ASTFunc>(srcFile, originalFuncName, funcNameStr, typeDescIn, genAttrsIn,
                                                  paramDeclsIn, id);
    } else if (declesTable[id] == nullptr) {
      declesTable[id] = allocator.GetMemPool()->New<ASTFunc>(srcFile, originalFuncName, funcNameStr, typeDescIn,
                                                             genAttrsIn, paramDeclsIn, id);
    }
    return static_cast<ASTFunc*>(declesTable[id]);
  }

  ASTTypedefDecl *ASTTypedefBuilder(const MapleAllocator &allocator, const MapleString &srcFile,
      const std::string &varName, const MapleVector<MIRType*> &desc,
      const GenericAttrs &genAttrsIn, int64 id = INT64_MAX) {
    MapleString varNameStr(varName, allocator.GetMemPool());
    if (id == INT64_MAX) {
      return allocator.GetMemPool()->New<ASTTypedefDecl>(srcFile, varNameStr, desc, genAttrsIn);
    } else if (declesTable[id] == nullptr) {
      declesTable[id] = allocator.GetMemPool()->New<ASTTypedefDecl>(srcFile, varNameStr, desc, genAttrsIn);
    }
    return static_cast<ASTTypedefDecl*>(declesTable[id]);
  }

  template<typename T>
  static T *ASTStmtBuilder(MapleAllocator &allocator) {
    return allocator.GetMemPool()->New<T>(allocator);
  }

  template<typename T>
  static T *ASTExprBuilder(MapleAllocator &allocator) {
    return allocator.GetMemPool()->New<T>(allocator);
  }

  ASTStruct *ASTStructBuilder(MapleAllocator &allocator, const MapleString &srcFile,
                              const std::string &nameIn, const MapleVector<MIRType*> &typeDescIn,
                              const GenericAttrs &genAttrsIn, int64 id = INT64_MAX) {
    MapleString structNameStr(nameIn, allocator.GetMemPool());
    if (id == INT64_MAX) {
      return allocator.GetMemPool()->New<ASTStruct>(allocator, srcFile, structNameStr, typeDescIn, genAttrsIn);
    } else if (declesTable[id] == nullptr) {
      declesTable[id] = allocator.GetMemPool()->New<ASTStruct>(allocator, srcFile, structNameStr, typeDescIn,
                                                               genAttrsIn);
    }
    return static_cast<ASTStruct*>(declesTable[id]);
  }

  ASTField *ASTFieldBuilder(const MapleAllocator &allocator, const MapleString &srcFile,
                            const std::string &varName, const MapleVector<MIRType*> &desc,
                            const GenericAttrs &genAttrsIn, int64 id = INT64_MAX,
                            bool isAnonymous = false) {
    MapleString varNameStr(varName, allocator.GetMemPool());
    if (id == INT64_MAX) {
      return allocator.GetMemPool()->New<ASTField>(srcFile, varNameStr, desc, genAttrsIn, isAnonymous);
    } else if (declesTable[id] == nullptr) {
      declesTable[id] = allocator.GetMemPool()->New<ASTField>(srcFile, varNameStr, desc, genAttrsIn,
                                                              isAnonymous);
    }
    return static_cast<ASTField*>(declesTable[id]);
  }
  ~ASTDeclsBuilder() = default;

 private:
  explicit ASTDeclsBuilder(MapleAllocator &allocator) : declesTable(allocator.Adapter()) {}
  MapleMap<int64, ASTDecl*> declesTable;
};
}  // namespace maple
#endif  // HIR2MPL_AST_INPUT_INCLUDE_AST_DECL_BUILDER_H
