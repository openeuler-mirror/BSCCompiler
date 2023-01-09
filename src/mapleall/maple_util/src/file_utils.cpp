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
#include "string_utils.h"
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
#if __WIN32
const char kFileSeperatorChar = kFileSeperatorWindowsStyleChar;
#else
const char kFileSeperatorChar = kFileSeperatorLinuxStyleChar;
#endif

#if __WIN32
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

std::string FileUtils::SafeGetPath(const char *cmd, const char *name) {
  int size = 1024;
  FILE *fp = NULL;
  char buf[size];
  if( (fp = popen(cmd, "r")) != NULL) {
    while (fgets(buf, size, fp) != NULL) {}
    pclose(fp);
    fp = NULL;
  } else {
    CHECK_FATAL(false, "Failed! Unable to find path of %s \n", name);
  }
  std::string path(buf);
  int last = path.find_last_of('/') + 1; // 1 means include "/"
  path = path.substr(0, last);
  return path;
}

void FileUtils::checkGCCVersion(const char *cmd) {
  int size = 1024;
  FILE *fp = NULL;
  char buf[size];
  if( (fp = popen(cmd, "r")) != NULL) {
    while (fgets(buf, size, fp) != NULL) {}
    pclose(fp);
    fp = NULL;
  } else {
    CHECK_FATAL(false, "Failed to obtain the aarch64-linux-gnu-gcc version number.\n");
  }
  if (buf[0] != 0) {
    // 53 is ASCII of 5
    if(buf[0] < 53) {
      CHECK_FATAL(false, "The aarch64-linux-gnu-gcc version cannot be earlier than 5.5.0.\n");
    }
  }
  if (buf[2] != 0) {
    if(buf[1] < 53) {
      CHECK_FATAL(false, "The aarch64-linux-gnu-gcc version cannot be earlier than 5.5.0.\n");
    }
  }
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
  return StringUtils::GetStrAfterLast(filePath, ".", true);
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
}  // namespace maple
