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
#include <fstream>
#include "types_def.h"
#include "mpl_logging.h"
#include "string_utils.h"

namespace maple {
enum class InputFileType {
  kFileTypeNone,
#define TYPE(ID, SUPPORT, ...) kFileType##ID,
#include "type.def"
#undef TYPE
};

struct TypeInfo {
  InputFileType fileType;
  bool support;
  const std::unordered_set<std::string> tmpSuffix;
};

extern const std::string kFileSeperatorStr;
extern const char kFileSeperatorChar;
extern const std::string kWhitespaceStr;
extern const char kWhitespaceChar;
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
  static std::string GetRealPath(const std::string &filePath, const bool isCheck = true);
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
  static std::string ExecuteShell(const char *cmd);
  static std::string ExecuteShell(const char *cmd, std::string workspace);
  static void GetFileNames(const std::string path, std::vector<std::string> &filenames);
  static bool GetAstFromLib(const std::string libPath, std::vector<std::string> &astInputs);
  static bool CreateFile(const std::string &file);
  static std::string GetGccBin();
  static bool Rmdirs(const std::string &dirPath);
  static std::string GetOutPutDirInTmp(const std::string &inputFile);
  static void Mkdirs(const std::string &path, mode_t mode);
  static bool IsStaticLibOrDynamicLib(const std::string &pathName);
  static InputFileType GetFileTypeByExtension(const std::string &extensionName);
  static bool IsSupportFileType(InputFileType type);
  static void RemoveFile(const std::string &pathName) {
    auto path = GetRealPath(pathName);
    CHECK_FATAL(std::remove(path.c_str()) == 0, "remove file failed, filename: %s.", path.c_str());
  };
  static std::string GetCurDirPath();
  static std::string GetClangAstOptString(const std::string astFilePath);
  static size_t GetIntegerFromMem(std::ifstream &fs, char *buffer, std::streamsize bufferSize,
      bool isNeedChkOptStrId);
  static void GetOptString(std::ifstream &fs, std::string &optString);

  static std::string GetFileNameHashStr(const std::string &str) {
    uint32 hash = 0;
    uint32 seed = 211;
    for (auto name : str) {
      hash = hash * seed + static_cast<unsigned char>(name);
    }
    return std::to_string(hash);
  }

  const std::string &GetTmpFolder() const {
    return tmpFolderPath;
  };

  static std::string GetOutPutDir();
  bool DelTmpDir() const;
  std::string GetTmpFolderPath() const;
 private:
  std::string tmpFolderPath;
  FileUtils() : tmpFolderPath(GetTmpFolderPath()) {}
  ~FileUtils() {
    if (!DelTmpDir()) {
      maple::LogInfo::MapleLogger() << "DelTmpDir failed" << '\n';
    };
  }
  static const uint32 kMagicAST = 0x48435043;
  static const uint32 kMagicELF = 0x464c457f;
  static const uint32 kMagicStaicLib = 0x72613c21;
};

class FileReader {
 public:
  explicit FileReader(const std::string &s) {
    mName = s;
    mDefFile.open(s.c_str(), std::ifstream::in);
    if (!mDefFile.good()) {
      ERR(kLncErr, "unable to read from file %s\n", s.c_str());
    }
  }
  ~FileReader() noexcept {
    mDefFile.close();
  }
  bool EndOfFile() const {
    return mDefFile.eof();
  }
 
  std::string GetLine(const std::string keyWord);
  bool ReadLine();  // the single entry to directly read from file
 private:
  std::string mName;
  std::ifstream mDefFile;
  std::string mCurLine = "";
  unsigned mLineNo = 0;
};
}  // namespace maple
#endif  // MAPLE_DRIVER_INCLUDE_FILE_UTILS_H
