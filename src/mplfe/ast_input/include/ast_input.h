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
#ifndef MPLFE_AST_INPUT_INCLUDE_AST_INPUT_H
#define MPLFE_AST_INPUT_INCLUDE_AST_INPUT_H
#include <string>
#include "mir_module.h"
#include "ast_decl.h"
#include "ast_parser.h"

namespace maple {
class ASTInput {
 public:
  ASTInput(MIRModule &moduleIn, MapleAllocator &allocatorIn);
  ~ASTInput() = default;
  bool ReadASTFile(MapleAllocator &allocatorIn, uint32 index, const std::string &fileName);
  bool ReadASTFiles(MapleAllocator &allocatorIn, const std::vector<std::string> &fileNames);
  const MIRModule &GetModule() const {
    return module;
  }

  void RegisterFileInfo(const std::string &fileName);
  const MapleList<ASTStruct*> &GetASTStructs() const {
    return astStructs;
  }

  void AddASTStruct(ASTStruct *astStruct) {
    astStructs.emplace_back(astStruct);
  }

  const MapleList<ASTFunc*> &GetASTFuncs() const {
    return astFuncs;
  }

  void AddASTFunc(ASTFunc *astFunc) {
    astFuncs.emplace_back(astFunc);
  }

  const MapleList<ASTVar*> &GetASTVars() const {
    return astVars;
  }

  void AddASTVar(ASTVar *astVar) {
    astVars.emplace_back(astVar);
  }

  const MapleList<ASTFileScopeAsm*> &GetASTFileScopeAsms() const {
    return astFileScopeAsms;
  }

  void AddASTFileScopeAsm(ASTFileScopeAsm *fileScopeAsm) {
    astFileScopeAsms.emplace_back(fileScopeAsm);
  }

 private:
  MIRModule &module;
  MapleAllocator &allocator;
  MapleMap<std::string, ASTParser*> astParserMap;  // map<filename, BCParser>

  MapleList<ASTStruct*> astStructs;
  MapleList<ASTFunc*> astFuncs;
  MapleList<ASTVar*> astVars;
  MapleList<ASTFileScopeAsm*> astFileScopeAsms;
};
}
#endif  // MPLFE_AST_INPUT_INCLUDE_AST_INPUT_H