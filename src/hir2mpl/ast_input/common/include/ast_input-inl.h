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
#include "ast_input.h"
#include "global_tables.h"
#include "fe_macros.h"

namespace maple {
template<class T>
ASTInput<T>::ASTInput(MIRModule &moduleIn, MapleAllocator &allocatorIn)
    : module(moduleIn), allocator(allocatorIn), parserMap(allocatorIn.Adapter()),
      astStructs(allocatorIn.Adapter()), astFuncs(allocatorIn.Adapter()), astVars(allocatorIn.Adapter()),
      astFileScopeAsms(allocatorIn.Adapter()), astEnums(allocatorIn.Adapter()) {}

template<class T>
bool ASTInput<T>::ReadASTFile(MapleAllocator &allocatorIn, uint32 index, const std::string &fileName) {
  T *parser = allocator.GetMemPool()->New<T>(allocator, index, fileName,
                                             astStructs, astFuncs, astVars,
                                             astFileScopeAsms, astEnums);
  TRY_DO(parser->OpenFile(allocatorIn));
  TRY_DO(parser->Verify());
  TRY_DO(parser->PreProcessAST());
  // Some implicit record or enum decl would be retrieved in func body at use,
  // so we put `RetrieveFuncs` before `RetrieveStructs`
  TRY_DO(parser->RetrieveFuncs(allocatorIn));
  TRY_DO(parser->RetrieveStructs(allocatorIn));
  if (FEOptions::GetInstance().IsDbgFriendly()) {
    TRY_DO(parser->RetrieveEnums(allocatorIn));
    TRY_DO(parser->RetrieveGlobalTypeDef(allocatorIn));
  }
  TRY_DO(parser->RetrieveGlobalVars(allocatorIn));
  TRY_DO(parser->RetrieveFileScopeAsms(allocatorIn));
  TRY_DO(parser->Release());
  parserMap.emplace(fileName, parser);
  return true;
}

template<class T>
bool ASTInput<T>::ReadASTFiles(MapleAllocator &allocatorIn, const std::vector<std::string> &fileNames) {
  bool res = true;
  for (uint32 i = 0; res && i < fileNames.size(); ++i) {
    res = res && ReadASTFile(allocatorIn, i, fileNames[i]);
    RegisterFileInfo(fileNames[i]);
  }
  return res;
}

template<class T>
void ASTInput<T>::RegisterFileInfo(const std::string &fileName) {
  GStrIdx fileNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(fileName);
  GStrIdx fileInfoIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("INFO_filename");
  module.PushFileInfoPair(MIRInfoPair(fileInfoIdx, fileNameIdx));
  module.PushFileInfoIsString(true);
}
}