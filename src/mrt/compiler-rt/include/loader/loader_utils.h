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
#ifndef __MAPLE_LOADER_UTILS__
#define __MAPLE_LOADER_UTILS__

#include <string>

#include "loader_api.h"
#include "version.h"
#include "base/macros.h"

namespace maplert {
namespace loaderutils {
  std::string GetNicePath(const std::string &fullPath);
  std::string Dex2MplPath(const std::string &filePath);
  bool CheckVersion(const ObjFile &mplFile, const maplert::LinkerMFileInfo &mplInfo);
  bool CheckCompilerStatus(const ObjFile &mplFile, const maplert::LinkerMFileInfo &mplInfo);
}
} // end namespace maple
#endif // endif __MAPLE_LOADER_UTILS__
