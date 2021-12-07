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
#ifndef HIR2MPL_INCLUDE_COMMON_FE_JAVA_STRING_MANAGER_H
#define HIR2MPL_INCLUDE_COMMON_FE_JAVA_STRING_MANAGER_H
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "mir_module.h"
#include "mir_builder.h"

namespace maple {
class FEJavaStringManager {
 public:
  FEJavaStringManager(MIRModule &argModule, MIRBuilder &mirBuilderIn);
  ~FEJavaStringManager();
  void ClearStringMetaClassSymbolExternFlag();
  // profiling
  void LoadProfilingData(const std::string &profileFileName);
  MIRSymbol *GetLiteralPtrVar(const MIRSymbol *var) const;
  MIRSymbol *GetLiteralPtrVar(const std::string &str) const;
  MIRSymbol *GetLiteralPtrVar(const std::u16string &strU16) const;
  // methods for string
  MIRSymbol *CreateLiteralVar(MIRBuilder &mirBuilder, const std::string &str, bool isFieldValue);
  MIRSymbol *CreateLiteralVar(MIRBuilder &mirBuilder, const std::u16string &strU16, bool isFieldValue);
  MIRSymbol *GetLiteralVar(const std::string &str) const;
  MIRSymbol *GetLiteralVar(const std::u16string &strU16) const;
  static std::string GetLiteralGlobalName(const std::u16string &strU16);
  static bool IsAllASCII(const std::u16string &strU16);
  void GenStringMetaClassVar();

 private:
  using DWBuffer = struct {
    uint64_t data;
    uint32_t pos;
  };

  MIRArrayType *ConstructArrayType4Str(const std::u16string &strU16, bool compressible) const;
  MIRAggConst *CreateByteArrayConst(const std::u16string &str, MIRArrayType &byteArrayType, bool compressible) const;
  static std::vector<uint8> SwapBytes(const std::u16string &strU16);
  static uint16 ExchangeBytesPosition(uint16 input);
  template <typename T>
  static void AddDataIntoByteArray(MIRAggConst &newConst, MemPool &mp, DWBuffer &buf, T data, MIRType &uInt64);
  static void FinishByteArray(MIRAggConst &newConst, MemPool &mp, DWBuffer &buf, MIRType &uInt64);

  MIRModule &module;
  MIRBuilder &mirBuilder;
  bool useCompressedJavaString = true;
  std::unordered_set<std::string> preloadSet;
  std::unordered_set<MIRSymbol*> literalSet;
  std::unordered_set<MIRSymbol*> fieldValueSet;
  std::map<const MIRSymbol*, MIRSymbol*> literalMap;
  MIRType *typeString = nullptr;
  MIRSymbol *stringMetaClassSymbol = nullptr;
};
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_COMMON_FE_JAVA_STRING_MANAGER_H