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
#ifndef __MAPLE_LOADER_FILE_LOADER__
#define __MAPLE_LOADER_FILE_LOADER__

#include <string>

#include "base/macros.h"
namespace maple {
class IMplLoader {
 public:
  virtual void *Open(const std::string &name) = 0;
  virtual void Close(void *handle) = 0;
  virtual ~IMplLoader() = default;
};
} // end namespace maple
#endif // endif __MAPLE_LOADER_FILE_LOADER__
