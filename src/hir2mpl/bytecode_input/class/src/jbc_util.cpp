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
#include "jbc_util.h"
#include "mpl_logging.h"

namespace maple {
namespace jbc {
std::string JBCUtil::ClassInternalNameToFullName(const std::string &name) {
  if (name[0] == '[') {
    return name;
  } else {
    return "L" + name + ";";
  }
}

JBCPrimType JBCUtil::GetPrimTypeForName(const std::string &name) {
  switch (name[0]) {
    case '[':
    case 'L':
      return kTypeRef;
    case 'I':
      return kTypeInt;
    case 'J':
      return kTypeLong;
    case 'F':
      return kTypeFloat;
    case 'D':
      return kTypeDouble;
    case 'B':
      return kTypeByteOrBoolean;
    case 'Z':
      return kTypeByteOrBoolean;
    case 'C':
      return kTypeChar;
    case 'S':
      return kTypeShort;
    case 'V':
      return kTypeDefault;
    default:
      CHECK_FATAL(false, "Unsupported type name %s", name.c_str());
  }
  return kTypeDefault;
}
}  // namespace jbc
}  // namespace maple