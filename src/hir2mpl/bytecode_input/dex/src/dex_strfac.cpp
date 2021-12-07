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
#include "dex_strfac.h"
#include "namemangler.h"

namespace maple {
std::string DexStrFactory::GetStringFactoryFuncname(const std::string &funcName) {
#define STR_STRFAC_MAP2(N1, N2)                                                  \
  if (funcName.compare(N1) == 0) {                                              \
    return N2;                                                                  \
  }
#include "dex_strfac_map.def"
#undef STR_STRFAC_MAP2
  return "";
}

bool DexStrFactory::IsStringInit(const std::string &funcName) {
  const std::string &mplName = "Ljava/lang/String;|<init>";
  if (funcName.compare(0, mplName.length(), mplName) == 0) {
    return true;
  }
  return false;
}
}  // namespace maple