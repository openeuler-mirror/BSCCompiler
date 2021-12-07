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
#include "bc_io.h"
namespace maple {
namespace bc {
BCIO::BCIO(const std::string &fileName) : BasicIOMapFile(fileName), isBigEndianHost(IsBigEndian()) {}

BCReader::BCReader(const std::string &fileName) : BCIO(fileName), BasicIORead(*this, false) {}

BCReader::~BCReader() {
  Close();
}

bool BCReader::RetrieveHeader(RawData &data) {
  return RetrieveHeaderImpl(data);
}

void BCReader::SetEndianTag(bool isBigEndianIn) {
  isBigEndian = isBigEndianIn;
}

bool BCReader::RetrieveHeaderImpl(RawData &data) {
  (void)data;
  CHECK_FATAL(false, "NIY");
  return false;
}

std::string BCReader::GetStringFromIdx(uint32 idx) const {
  return GetStringFromIdxImpl(idx);
}

std::string BCReader::GetTypeNameFromIdx(uint32 idx) const {
  return GetTypeNameFromIdxImpl(idx);
}

BCReader::ClassElem BCReader::GetClassMethodFromIdx(uint32 idx) const {
  return GetClassMethodFromIdxImpl(idx);
}

BCReader::ClassElem BCReader::GetClassFieldFromIdx(uint32 idx) const {
  return GetClassFieldFromIdxImpl(idx);
}

std::string BCReader::GetSignature(uint32 idx) const {
  return GetSignatureImpl(idx);
}

uint32 BCReader::GetFileIndex() const {
  return GetFileIndexImpl();
}

std::string BCReader::GetIRSrcFileSignature() const {
  return irSrcFileSignature;
}
}  // namespace bc
}  // namespace maple