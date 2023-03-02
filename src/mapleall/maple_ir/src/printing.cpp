/*
 * Copyright (c) [2019] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "printing.h"
#include "mir_module.h"
#include "types_def.h"

namespace maple {
const std::string kBlankString = "                                                                                ";
constexpr int kIndentunit = 2;  // number of blank chars of each indentation

void PrintIndentation(int32 indent) {
  int64 indentAmount = static_cast<int64>(indent) * kIndentunit;
  do {
    LogInfo::MapleLogger() << kBlankString.substr(0, indentAmount);
    indentAmount -= static_cast<int64>(kBlankString.length());
  } while (indentAmount > 0);
}

void PrintString(const std::string &str) {
  size_t i = 0;
  LogInfo::MapleLogger() << " \"";
  while (i < str.length()) {
    unsigned char c = str[i++];
    // differentiate printable and non-printable charactors
    if (c >= 0x20 && c <= 0x7e) {
      // escape "
      switch (c) {
        case '"':
          LogInfo::MapleLogger() << "\\\"";
          break;
        case '\\':
          LogInfo::MapleLogger() << "\\\\";
          break;
        default:
          LogInfo::MapleLogger() << c;
          break;
      }
    } else {
      constexpr int kFieldWidth = 2;
      LogInfo::MapleLogger() << "\\x" << std::hex << std::setfill('0') << std::setw(kFieldWidth)
                             << static_cast<unsigned int>(c) << std::dec;
    }
  }
  LogInfo::MapleLogger() << "\"";
}
}  // namespace maple
