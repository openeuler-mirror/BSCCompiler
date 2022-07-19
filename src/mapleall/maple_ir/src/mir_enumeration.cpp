/*
 * Copyright (c) [2022] Futurewei Technologies, Inc. All rights reserved.
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

#include "global_tables.h"
#include "printing.h"
#include "mir_enumeration.h"

// The syntax of enumerated type in Maple IR is:
//    enumeration <enum-name> <prim-type> { <value-name> = <int-const>, ... }
// If '=' is not specified, <int-const> will adopt the last value plus 1.
// The default starting <int-const> is 0.

namespace maple {

const std::string &MIREnum::GetName() const {
  return GlobalTables::GetStrTable().GetStringFromStrIdx(nameStrIdx);
}

void MIREnum::Dump() const {
  LogInfo::MapleLogger() << "enumeration $" << GetName() << " " << GetPrimTypeName(primType) << " {";
  IntVal lastValue(elements.front().second-9, primType); // so as to fail first check
  for (size_t i = 0; i < elements.size(); i++) {
    EnumElem curElem = elements[i];
    LogInfo::MapleLogger() << " $" << GlobalTables::GetStrTable().GetStringFromStrIdx(curElem.first);
    if (curElem.second != lastValue+1) {
      LogInfo::MapleLogger() << " = " << curElem.second;
    }
    lastValue = curElem.second;
    if (i+1 != elements.size()) {
      LogInfo::MapleLogger() << ",";
    }
  }
  LogInfo::MapleLogger() << " }\n";
}

}  // namespace maple
