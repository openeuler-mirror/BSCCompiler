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
#ifndef MAPLE_DRIVER_INCLUDE_FILE_UTILS_H
#define MAPLE_DRIVER_INCLUDE_FILE_UTILS_H
#include <string>

namespace maple {
extern const std::string kFileSeperatorStr;
extern const char kFileSeperatorChar;
// Use char[] since getenv receives char* as parameter
constexpr char kMapleRoot[] = "MAPLE_ROOT";
constexpr char kClangPath[] = "BiShengC_Clang_Path";
constexpr char kAsPath[] = "BiShengC_AS_Path";
constexpr char kGccPath[] = "BiShengC_GCC_Path";
constexpr char kLdLibPath[] = "LD_LIBRARY_PATH";

class FileUtils {
 public:
  static std::string SafeGetenv(const char *envVar);
  static std::string SafeGetPath(const char *envVar, const char *name);
  static void CheckGCCVersion(const char *cmd);
  static std::string GetRealPath(const std::string &filePath);
  static std::string GetFileName(const std::string &filePath, bool isWithExtension);
  static std::string GetFileExtension(const std::string &filePath);
  static std::string GetFileFolder(const std::string &filePath);
  static std::string GetExecutable();
  static int Remove(const std::string &filePath);
  static bool IsFileExists(const std::string &filePath);
  static std::string AppendMapleRootIfNeeded(bool needRootPath, const std::string &path,
                                             const std::string &defaultRoot = "." + kFileSeperatorStr);
};
}  // namespace maple
#endif  // MAPLE_DRIVER_INCLUDE_FILE_UTILS_H
