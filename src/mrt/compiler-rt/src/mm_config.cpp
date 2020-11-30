/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "mm_config.h"

namespace maplert {
long MrtEnvConf(const char *envName, long defaultValue) {
  const char *ev = getenv(envName);
  if (ev != nullptr) {
    char *endptr = nullptr;
    long rv = std::strtol(ev, &endptr, 0); // support dec, oct and hex
    if (*endptr == '\0') {
      return rv;
    } else {
      return defaultValue;
    }
  } else {
    return defaultValue;
  }
}
}
