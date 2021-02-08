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
#include "bc_parser_base.h"
#include "mpl_logging.h"
#include "mir_module.h"
namespace maple {
namespace bc {
BCParserBase::BCParserBase(uint32 fileIdxIn, const std::string &fileNameIn, const std::list<std::string> &classNamesIn)
    : fileIdx(fileIdxIn), fileName(fileNameIn), classNames(classNamesIn),
      fileNameHashId(-1) {}

const BCReader *BCParserBase::GetReader() const {
  return GetReaderImpl();
}

bool BCParserBase::OpenFile() {
  return OpenFileImpl();
}

bool BCParserBase::ParseHeader() {
  return ParseHeaderImpl();
}

bool BCParserBase::Verify() {
  return VerifyImpl();
}

uint32 BCParserBase::CalculateCheckSum(const uint8 *data, uint32 size) {
  return CalculateCheckSumImpl(data, size);
}

bool BCParserBase::RetrieveClasses(std::list<std::unique_ptr<BCClass>> &klasses) {
  if (RetrieveIndexTables() == false) {
    ERR(kLncErr, "RetrieveIndexTables failed");
    return false;
  }
  if (!classNames.empty()) {
    return RetrieveUserSpecifiedClasses(klasses);
  } else {
    return RetrieveAllClasses(klasses);
  }
}

bool BCParserBase::CollectAllDepTypeNames(std::unordered_set<std::string> &depSet) {
  return CollectAllDepTypeNamesImpl(depSet);
}

bool BCParserBase::CollectMethodDepTypeNames(std::unordered_set<std::string> &depSet, BCClassMethod &bcMethod) const {
  return CollectMethodDepTypeNamesImpl(depSet, bcMethod);
}

bool BCParserBase::CollectAllClassNames(std::unordered_set<std::string> &classSet) {
  return CollectAllClassNamesImpl(classSet);
}
}  // namespace bc
}  // namespace maple