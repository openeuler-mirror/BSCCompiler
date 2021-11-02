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
#ifndef MPLFE_BC_INPUT_INCLUDE_BC_PARSER_BASE_H
#define MPLFE_BC_INPUT_INCLUDE_BC_PARSER_BASE_H
#include <memory>
#include <list>
#include <string>
#include "types_def.h"
#include "bc_io.h"

namespace maple {
namespace bc {
class BCClass;
class BCClassMethod;
class BCParserBase {
 public:
  BCParserBase(uint32 fileIdxIn, const std::string &fileNameIn, const std::list<std::string> &classNamesIn);
  virtual ~BCParserBase() = default;
  bool OpenFile();
  bool ParseHeader();
  bool Verify();
  int32 GetFileNameHashId() const {
    return fileNameHashId;
  }
  uint32 CalculateCheckSum(const uint8 *data, uint32 size);
  bool RetrieveClasses(std::list<std::unique_ptr<BCClass>> &klasses);
  const BCReader *GetReader() const;
  bool CollectAllDepTypeNames(std::unordered_set<std::string> &depSet);
  bool CollectMethodDepTypeNames(std::unordered_set<std::string> &depSet, BCClassMethod &bcMethod) const;
  bool CollectAllClassNames(std::unordered_set<std::string> &classSet);
  void ProcessMethodBody(BCClassMethod &method, uint32 classIdx, uint32 methodItemIdx, bool isVirtual) const {
    ProcessMethodBodyImpl(method, classIdx, methodItemIdx, isVirtual);
  }

 protected:
  virtual const BCReader *GetReaderImpl() const = 0;
  virtual bool OpenFileImpl() = 0;
  virtual uint32 CalculateCheckSumImpl(const uint8 *data, uint32 size) = 0;
  virtual bool ParseHeaderImpl() = 0;
  virtual bool VerifyImpl() = 0;
  virtual bool RetrieveIndexTables() = 0;
  virtual bool RetrieveUserSpecifiedClasses(std::list<std::unique_ptr<BCClass>> &klasses) = 0;
  virtual bool RetrieveAllClasses(std::list<std::unique_ptr<BCClass>> &klasses) = 0;
  virtual bool CollectAllDepTypeNamesImpl(std::unordered_set<std::string> &depSet) = 0;
  virtual bool CollectMethodDepTypeNamesImpl(std::unordered_set<std::string> &depSet,
      BCClassMethod &bcMethod) const = 0;
  virtual bool CollectAllClassNamesImpl(std::unordered_set<std::string> &classSet) = 0;
  virtual void ProcessMethodBodyImpl(BCClassMethod &method,
      uint32 classIdx, uint32 methodItemIdx, bool isVirtual) const = 0;

  uint32 fileIdx;
  const std::string fileName;
  const std::list<std::string> &classNames;
  const int32 fileNameHashId;
};
}  // namespace bc
}  // namespace maple
#endif  // MPLFE_BC_INPUT_INCLUDE_BC_PARSER_BASE_H