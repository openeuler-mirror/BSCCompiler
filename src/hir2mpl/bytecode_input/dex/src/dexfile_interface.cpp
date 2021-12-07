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
#include "dexfile_interface.h"
#include "mpl_logging.h"

namespace maple{
void ResolvedMethodType::SignatureTypes(const std::string &mt, std::list<std::string> &types) {
  // three pointers linear scan algo
  size_t startPos = 1;  // pos 0 should be '('
  size_t currentPos = startPos;
  size_t endPos = mt.find(")");
  CHECK_FATAL(endPos != std::string::npos, "(ToIDEUser)Invalid string format: %s", mt.c_str());
  CHECK_FATAL(startPos <= endPos, "(ToIDEUser)Invalid string format: %s", mt.c_str());
  while (startPos < endPos) {
    switch (mt[currentPos]) {
      case 'I':
      case 'Z':
      case 'B':
      case 'C':
      case 'V':
      case 'S':
      case 'J':
      case 'F':
      case 'D': {
        types.push_back(namemangler::EncodeName(mt.substr(startPos, currentPos - startPos + 1)));
        ++currentPos;
        break;
      }
      case '[':
        ++currentPos;
        continue;
      case 'L':
        while (mt[currentPos++] != ';') {}  // empty
        types.push_back(namemangler::EncodeName(mt.substr(startPos, currentPos - startPos)));
        break;
      default:
        std::cerr << "ResolvedMethodType: catastrophic error" << std::endl;
        break;
    }
    startPos = currentPos;
  }
}

std::string ResolvedMethodType::SignatureReturnType(const std::string &mt) {
  size_t endPos = mt.find(")");
  return namemangler::EncodeName(mt.substr(endPos + 1));
}
}  // namespace maple
