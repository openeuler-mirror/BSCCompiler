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
#include "types_def.h"
#include "mpl_logging.h"
#include "mempool.h"
#include "string_utils.h"

namespace maple {
  enum class InputFileType {
  kFileTypeNone,
  kFileTypeClass,
  kFileTypeJar,
  kFileTypeAst,
  kFileTypeCpp,
  kFileTypeC,
  kFileTypeDex,
  kFileTypeMpl,
  kFileTypeVtableImplMpl,
  kFileTypeS,
  kFileTypeObj,
  kFileTypeBpl,
  kFileTypeMeMpl,
  kFileTypeMbc,
  kFileTypeLmbc,
  kFileTypeH,
  kFileTypeI,
  kFileTypeOast,
};

extern const std::string kFileSeperatorStr;
extern const char kFileSeperatorChar;
// Use char[] since getenv receives char* as parameter
constexpr char kMapleRoot[] = "MAPLE_ROOT";
constexpr char kClangPath[] = "BiShengC_Clang_Path";
constexpr char kClangPathPure[] = "PURE_CLANG_PATH";
constexpr char kAsPath[] = "BiShengC_AS_Path";
constexpr char kGccPath[] = "BiShengC_GCC_Path";
constexpr char kLdLibPath[] = "LD_LIBRARY_PATH";
constexpr char kGetOsVersion[] = "BISHENGC_GET_OS_VERSION";
constexpr char kArPath[] = "BiShengC_AR_PATH";
constexpr char kEnhancedClang[] = "ENHANCED_CLANG_PATH";
constexpr char kGccLibPath[] = "BiShengC_GCC_LibPath";

class FileUtils {
 public:
  static FileUtils &GetInstance() {
    static FileUtils instance;
    return instance;
  }
  FileUtils(const FileUtils&) = delete;
  FileUtils &operator=(const FileUtils&) = delete;
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
  static InputFileType GetFileType(const std::string &filePath);
  static InputFileType GetFileTypeByMagicNumber(const std::string &pathName);
  static char* LoadFile(const char *filename);
  static std::string ExecuteShell(const char *cmd);
  static bool GetAstFromLib(const std::string libPath, std::vector<std::string> &astInputs);

  const std::string &GetTmpFolder() const {
    return tmpFolderPath;
  };

  MemPool &GetMemPool() {
    return *tempMP;
  };
  static std::string GetOutPutDir();
  bool DelTmpDir() const;
  std::string GetTmpFolderPath() const;
 private:
  std::string tmpFolderPath;
  MemPool *tempMP = nullptr;
  FileUtils() : tmpFolderPath(GetTmpFolderPath()), tempMP(memPoolCtrler.NewMemPool("file_utils", true)) {}
  ~FileUtils() {
    memPoolCtrler.DeleteMemPool(tempMP);
    if (!DelTmpDir()) {
      maple::LogInfo::MapleLogger() << "DelTmpDir failed" << '\n';
    };
  }
  static const uint32 kMagicAST = 0x48435043;
  static const uint32 kMagicELF = 0x464c457f;
};
}  // namespace maple
#endif  // MAPLE_DRIVER_INCLUDE_FILE_UTILS_H
