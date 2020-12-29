/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "file_layout.h"

namespace maple {
std::string GetLayoutTypeString(uint32_t type) {
  switch (type) {
    case kLayoutBootHot:
      return "BootHot";
    case kLayoutBothHot:
      return "BothHot";
    case kLayoutRunHot:
      return "RunHot";
    case kLayoutStartupOnly:
      return "StartupOnly";
    case kLayoutUsedOnce:
      return "UsedOnce";
    case kLayoutExecuted:
      return "UsedMaybe";
    case kLayoutUnused:
      return "Unused";
    default:
      std::cerr << "no such type" << std::endl;
      return "";
  }
}
} // namespace maple
