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
#ifndef HIR2MPL_INCLUDE_COMMON_FE_FILE_OPS_H
#define HIR2MPL_INCLUDE_COMMON_FE_FILE_OPS_H
#include <string>

namespace maple {
class FEFileOps {
 public:
  static std::string GetFilePath(const std::string &pathName);
  static std::string GetFileNameWithExt(const std::string &pathName);
  static std::string GetFileName(const std::string &pathName);
  static std::string GetFileExtName(const std::string &pathName);

 private:
  FEFileOps() = default;
  ~FEFileOps() = default;
};
} // namespace maple
#endif