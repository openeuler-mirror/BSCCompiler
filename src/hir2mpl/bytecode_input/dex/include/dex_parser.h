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
#ifndef MPL_FE_DEX_INPUT_DEX_PARSER_H
#define MPL_FE_DEX_INPUT_DEX_PARSER_H
#include "bc_parser-inl.h"
#include "dex_reader.h"
#include "dex_class.h"
#include "types_def.h"

namespace maple {
namespace bc {
class DexParser : public BCParser<DexReader> {
 public:
  DexParser(uint32 fileIdxIn, const std::string &fileNameIn, const std::list<std::string> &classNamesIn);
  ~DexParser() override = default;
  void ProcessDexClassMethod(const std::unique_ptr<DexClass> &dexClass, bool isVirtual,
                             uint32 index, std::pair<uint32, uint32> &idxPair);
  void SetDexFile(std::unique_ptr<IDexFile> iDexFileIn);
  std::unique_ptr<BCClass> FindClassDef(const std::string &className);

 protected:
  const BCReader *GetReaderImpl() const override;
  uint32 CalculateCheckSumImpl(const uint8 *data, uint32 size) override;
  bool ParseHeaderImpl() override;
  bool VerifyImpl() override;
  bool RetrieveIndexTables() override;
  bool RetrieveUserSpecifiedClasses(std::list<std::unique_ptr<BCClass>> &klasses) override;
  bool RetrieveAllClasses(std::list<std::unique_ptr<BCClass>> &klasses) override;
  bool CollectAllDepTypeNamesImpl(std::unordered_set<std::string> &depSet) override;
  bool CollectMethodDepTypeNamesImpl(std::unordered_set<std::string> &depSet, BCClassMethod &bcMethod) const override;
  bool CollectAllClassNamesImpl(std::unordered_set<std::string> &classSet) override;
  void ProcessMethodBodyImpl(BCClassMethod &method,
      uint32 classIdx, uint32 methodItemIdx, bool isVirtual) const override;

 private:
  std::unique_ptr<DexClass> ProcessDexClass(uint32 classIdx);
  void ProcessDexClassDef(const std::unique_ptr<DexClass> &dexClass);
  void ProcessDexClassInterfaceParent(const std::unique_ptr<DexClass> &dexClass);
  void ProcessDexClassFields(const std::unique_ptr<DexClass> &dexClass);
  void ProcessDexClassMethods(const std::unique_ptr<DexClass> &dexClass, bool isVirtual);
  void ProcessDexClassMethodDecls(const std::unique_ptr<DexClass> &dexClass, bool isVirtual);
  void ProcessDexClassAnnotationDirectory(const std::unique_ptr<DexClass> &dexClass);
  void ProcessDexClassStaticFieldInitValue(const std::unique_ptr<DexClass> &dexClass);
};
}  // namespace bc
}  // namespace maple
#endif  // MPL_FE_DEX_INPUT_DEX_PARSER_H