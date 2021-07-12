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
#include "ast_input.h"
#include "global_tables.h"
#include "fe_macros.h"
namespace maple {
ASTInput::ASTInput(MIRModule &moduleIn, MapleAllocator &allocatorIn)
    : module(moduleIn), allocator(allocatorIn), astParserMap(allocatorIn.Adapter()),
      astStructs(allocatorIn.Adapter()), astFuncs(allocatorIn.Adapter()), astVars(allocatorIn.Adapter()),
      astFileScopeAsms(allocatorIn.Adapter()) {}

bool ASTInput::ReadASTFile(MapleAllocator &allocatorIn, uint32 index, const std::string &fileName) {
  ASTParser *astParser = allocator.GetMemPool()->New<ASTParser>(allocator, index, fileName,
                                                                astStructs, astFuncs, astVars, astFileScopeAsms);
  astParser->SetAstIn(this);
  TRY_DO(astParser->OpenFile());
  TRY_DO(astParser->Verify());
  TRY_DO(astParser->PreProcessAST());
  // Some implicit record decl would be retrieved in func body at use,
  // so we put `RetrieveFuncs` before `RetrieveStructs`
  TRY_DO(astParser->RetrieveFuncs(allocatorIn));
  TRY_DO(astParser->RetrieveStructs(allocatorIn));
  TRY_DO(astParser->RetrieveGlobalVars(allocatorIn));
  TRY_DO(astParser->RetrieveFileScopeAsms(allocatorIn));
  astParserMap.emplace(fileName, astParser);
  return true;
}

bool ASTInput::ReadASTFiles(MapleAllocator &allocator, const std::vector<std::string> &fileNames) {
  bool res = true;
  for (uint32 i = 0; res && i < fileNames.size(); ++i) {
    res = res && ReadASTFile(allocator, i, fileNames[i]);
    RegisterFileInfo(fileNames[i]);
  }
  return res;
}

void ASTInput::RegisterFileInfo(const std::string &fileName) {
  GStrIdx fileNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(fileName);
  GStrIdx fileInfoIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("INFO_filename");
  module.PushFileInfoPair(MIRInfoPair(fileInfoIdx, fileNameIdx));
  module.PushFileInfoIsString(true);
}
}