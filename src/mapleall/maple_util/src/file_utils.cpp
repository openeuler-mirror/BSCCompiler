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
#include <cstdio>
#include <cstring>
#include <climits>
#include <fstream>
#include <unistd.h>
#include "mpl_logging.h"
#include "file_utils.h"

#ifdef _WIN32
#include <shlwapi.h>
#endif

namespace {
const char kFileSeperatorLinuxStyleChar = '/';
const char kFileSeperatorWindowsStyleChar = '\\';
const std::string kFileSeperatorLinuxStyleStr = std::string(1, kFileSeperatorLinuxStyleChar);
const std::string kFileSeperatorWindowsStyleStr = std::string(1, kFileSeperatorWindowsStyleChar);
} // namespace

namespace maple {
#if defined(__WIN32) && __WIN32
const char kFileSeperatorChar = kFileSeperatorWindowsStyleChar;
#else
const char kFileSeperatorChar = kFileSeperatorLinuxStyleChar;
#endif

#if defined(__WIN32) && __WIN32
const std::string kFileSeperatorStr = kFileSeperatorWindowsStyleStr;
#else
const std::string kFileSeperatorStr = kFileSeperatorLinuxStyleStr;
#endif

std::string FileUtils::SafeGetenv(const char *envVar) {
  const char *tmpEnvPtr = std::getenv(envVar);
  if (tmpEnvPtr == nullptr) {
    return "";
  }
  std::string tmpStr(tmpEnvPtr);
  return tmpStr;
}

std::string FileUtils::ExecuteShell(const char *cmd) {
  const int size = 1024;
  FILE *fp = nullptr;
  char buf[size] = {0};
  std::string result = "";
  fp = popen(cmd, "r");
  if (fp == nullptr) {
    if (StringUtils::StartsWith(cmd, "rm -rf")) {
      return result;
    }
    CHECK_FATAL((fp = popen(cmd, "r")) != nullptr, "Failed to execute shell (%s). \n", cmd);
  }
  while (fgets(buf, size, fp) != nullptr) {
    result += std::string(buf);
  }
  (void)pclose(fp);
  fp = nullptr;
  if (result[result.length() - 1] == '\n') {
    result = result.substr(0, result.length() - 1);
  }
  return result;
}

std::string FileUtils::SafeGetPath(const char *envVar, const char *name) {
  std::string path = ExecuteShell(envVar);
  CHECK_FATAL(path.find(name) != std::string::npos, "Failed! Unable to find path of %s \n", name);
  std::string tmp = name;
  size_t index = path.find(tmp) + tmp.length();
  path = path.substr(0, index);
  return path;
}

void FileUtils::CheckGCCVersion(const char *cmd) {
  std::string gccVersion = ExecuteShell(cmd);
  std::vector<std::string> ver;
  StringUtils::Split(gccVersion, ver, '.');
  bool isEarlierVersion = std::stoi(ver[0].c_str()) < std::stoi("5") ||
      (std::stoi(ver[0].c_str()) == std::stoi("5") && std::stoi(ver[1].c_str()) < std::stoi("5"));
  CHECK_FATAL(!isEarlierVersion, "The aarch64-linux-gnu-gcc version cannot be earlier than 5.5.0.\n");
}

std::string FileUtils::GetOutPutDir() {
  return "./";
}

std::string FileUtils::GetTmpFolderPath() const {
  const char *cmd = "mktemp -d";
  std::string path = ExecuteShell(cmd);
  CHECK_FATAL(path.size() != 0, "Failed to create tmp folder");
  std::string tmp = "\n";
  size_t index = path.find(tmp) == std::string::npos ? path.length() : path.find(tmp);
  path = path.substr(0, index);
  return path + "/";
}

std::string FileUtils::GetRealPath(const std::string &filePath) {
#ifdef _WIN32
  char *path = nullptr;
  if (filePath.size() > PATH_MAX  || !PathCanonicalize(path, filePath.c_str())) {
    CHECK_FATAL(false, "invalid file path");
  }
#else
  char path[PATH_MAX] = {0};
  if (filePath.size() > PATH_MAX  || realpath(filePath.c_str(), path) == nullptr) {
    CHECK_FATAL(false, "invalid file path");
  }
#endif
  std::string result(path, path + strlen(path));
  return result;
}

std::string FileUtils::GetFileName(const std::string &filePath, bool isWithExtension) {
  std::string fullFileName = StringUtils::GetStrAfterLast(filePath, kFileSeperatorStr);
#ifdef _WIN32
  fullFileName = StringUtils::GetStrAfterLast(fullFileName, kFileSeperatorLinuxStyleStr);
#endif
  if (isWithExtension) {
    return fullFileName;
  }
  return StringUtils::GetStrBeforeLast(fullFileName, ".");
}

std::string FileUtils::GetFileExtension(const std::string &filePath) {
  std::string  fileExtension = StringUtils::GetStrAfterLast(filePath, ".", true);
  return fileExtension;
}

InputFileType FileUtils::GetFileType(const std::string &filePath) {
  InputFileType fileType = InputFileType::kFileTypeNone;
  std::string extensionName = GetFileExtension(filePath);
  if (extensionName == "class") {
    fileType = InputFileType::kFileTypeClass;
  } else if (extensionName == "dex") {
    fileType = InputFileType::kFileTypeDex;
  } else if (extensionName == "c") {
    fileType = InputFileType::kFileTypeC;
  } else if (extensionName == "cpp") {
    fileType = InputFileType::kFileTypeCpp;
  } else if (extensionName == "ast") {
    fileType = InputFileType::kFileTypeAst;
  } else if (extensionName == "jar") {
    fileType = InputFileType::kFileTypeJar;
  } else if (extensionName == "mpl" || extensionName == "bpl") {
    if (filePath.find("VtableImpl") == std::string::npos) {
      if (filePath.find(".me.mpl") != std::string::npos) {
        fileType = InputFileType::kFileTypeMeMpl;
      } else {
        fileType = extensionName == "mpl" ? InputFileType::kFileTypeMpl : InputFileType::kFileTypeBpl;
      }
    } else {
      fileType = InputFileType::kFileTypeVtableImplMpl;
    }
  } else if (extensionName == "s" || extensionName == "S") {
    fileType = InputFileType::kFileTypeS;
  } else if (extensionName == "o") {
    fileType = GetFileTypeByMagicNumber(filePath);
  } else if (extensionName == "mbc") {
    fileType = InputFileType::kFileTypeMbc;
  } else if (extensionName == "lmbc") {
    fileType = InputFileType::kFileTypeLmbc;
  } else if (extensionName == "h") {
    fileType = InputFileType::kFileTypeH;
  } else if (extensionName == "i") {
    fileType = InputFileType::kFileTypeI;
  } else if (extensionName == "oast") {
    fileType = InputFileType::kFileTypeOast;
  }

  return fileType;
}

std::string FileUtils::GetExecutable() {
#ifdef _WIN32
  static_assert(false, "Implement me for Windows");

#else /* _WIN32 */
  /* Linux Implementation */
  char exePath[PATH_MAX];
  const char *symLinkToCurrentExe = "/proc/self/exe";

  int len = static_cast<int>(readlink(symLinkToCurrentExe, exePath, sizeof(exePath)));
  if (len < 0) {
    LogInfo::MapleLogger(kLlErr) << "Currently toolchain supports only Linux System\n";
    return "";
  }

  /* Add Null terminate for the string: readlink does not
   * append a terminating null byte to buf */
  len = std::min(len, static_cast<int>(sizeof(exePath) - 1));
  exePath[len] = '\0';

  /* On Linux, /proc/self/exe always looks through symlinks. However, on
   * GNU/Hurd, /proc/self/exe is a symlink to the path that was used to start
   * the program, and not the eventual binary file. Therefore, call realpath
   * so this behaves the same on all platforms.
   */
  if (char *realPath = realpath(exePath, nullptr)) {
    std::string ret = std::string(realPath);
    free(realPath);
    return ret;
  }
  ASSERT(false, "Something wrong: %s\n", exePath);

  return "";
#endif /* _WIN32 */
}

std::string FileUtils::GetFileFolder(const std::string &filePath) {
  std::string folder = StringUtils::GetStrBeforeLast(filePath, kFileSeperatorStr, true);
  std::string curSlashType = kFileSeperatorStr;
#ifdef _WIN32
  if (folder.empty()) {
    curSlashType = kFileSeperatorLinuxStyleStr;
    folder = StringUtils::GetStrBeforeLast(filePath, curSlashType, true);
  }
#endif
  return folder.empty() ? ("." + curSlashType) : (folder + curSlashType);
}

int FileUtils::Remove(const std::string &filePath) {
  return remove(filePath.c_str());
}

bool FileUtils::IsFileExists(const std::string &filePath) {
  std::ifstream f(filePath);
  return f.good();
}

std::string FileUtils::AppendMapleRootIfNeeded(bool needRootPath, const std::string &path,
                                               const std::string &defaultRoot) {
  if (!needRootPath) {
    return path;
  }
  std::ostringstream ostrStream;
  if (getenv(kMapleRoot) == nullptr) {
    ostrStream << defaultRoot << path;
  } else {
    ostrStream << getenv(kMapleRoot) << kFileSeperatorStr << path;
  }
  return ostrStream.str();
}

bool FileUtils::DelTmpDir() const {
  if (FileUtils::GetInstance().GetTmpFolder() == "") {
    return true;
  }
  std::string tmp = "rm -rf " + FileUtils::GetInstance().GetTmpFolder();
  std::string result = ExecuteShell(tmp.c_str());
  if (result != "") {
    return false;
  }
  return true;
}

InputFileType FileUtils::GetFileTypeByMagicNumber(const std::string &pathName) {
  std::ifstream file(GetRealPath(pathName));
  if (!file.is_open()) {
    ERR(kLncErr, "unable to open file %s", pathName.c_str());
    return InputFileType::kFileTypeNone;
  }
  uint32 magic = 0;
  int length = static_cast<int>(sizeof(uint32));
  (void)file.read(reinterpret_cast<char*>(&magic), length);
  file.close();
  return magic == kMagicAST ? InputFileType::kFileTypeOast : magic == kMagicELF ? InputFileType::kFileTypeObj :
                                                                                  InputFileType::kFileTypeNone;
}

char* FileUtils::LoadFile(const char *filename) {
  size_t maxlength = 102400; // 100K
  std::string specFile = GetRealPath(filename);
  char *specs;
  char *content;
  std::ifstream readSpecFile(specFile);
  if (!readSpecFile.is_open()) {
    CHECK_FATAL(false, "unable to open file %s", specFile.c_str());
    return nullptr;
  }
  content = FileUtils::GetInstance().GetMemPool().NewArray<char>(maxlength);
  if (content) {
    char buffer[maxlength];
    size_t len = 0;
    while (readSpecFile.getline(buffer, static_cast<long>(maxlength))) {
      for (size_t i = 0; i < maxlength; i++) {
        char tmp = buffer[i];
        if (tmp == '\0') {
          content[len] = '\n';
          len++;
          break;
        }
        content[len] = tmp;
        len++;
      }
    }
    content[len] = '\0';
    specs = FileUtils::GetInstance().GetMemPool().NewArray<char>(++len);
    if (!specs) {
      CHECK_FATAL(false, "unable to open file %s", specFile.c_str());
    }
    for (size_t i = 0; i < len; i++) {
      specs[i] = content[i];
    }
    return specs;
  }
  return nullptr;
}


bool FileUtils::GetAstFromLib(const std::string libPath, std::vector<std::string> &astInputs) {
  bool elfFlag = false;
  std::string binAr = "";
  std::string cmd = "which ar";
  std::string result = "";
  if (SafeGetenv(kArPath) != "") {
    binAr = SafeGetenv(kArPath);
  } else {
    result = ExecuteShell(cmd.c_str());
    CHECK_FATAL((result != ""), "Unable find where is ar via cmd (which ar).");
    binAr = result;
  }
  std::string realLibPath = GetRealPath(libPath);
  cmd = binAr + " t " + realLibPath;
  std::vector<std::string> astVec;
  result = ExecuteShell(cmd.c_str());
  StringUtils::Split(result, astVec, '\n');
  for (auto tmp : astVec) {
    if (tmp.empty()) {
      continue;
    }
    cmd = binAr + " x " + realLibPath + " " + tmp;
    CHECK_FATAL((ExecuteShell(cmd.c_str()) == ""), "Failed to execute %s.", cmd.c_str());
    result = GetFileExtension(GetRealPath(tmp));
    if (result == "o" && GetFileTypeByMagicNumber(GetRealPath(tmp)) == InputFileType::kFileTypeOast) {
      cmd = binAr + " d " + realLibPath + " " + tmp;
      astInputs.push_back(GetRealPath(tmp));
    } else {
      elfFlag = true;
    }
  }
  return elfFlag;
}

}  // namespace maple
