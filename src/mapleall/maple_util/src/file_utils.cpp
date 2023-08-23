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
#include <random>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include "mpl_logging.h"
#include "securec.h"
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

const static TypeInfo kTypeInfos[] = {
#define TYPE(ID, SUPPORT, ...) \
  {InputFileType::kFileType##ID, SUPPORT,  {__VA_ARGS__}},
#include "type.def"
#undef TYPE
};
const char kWhitespaceChar = ' ';
const std::string kWhitespaceStr = std::string(1, kWhitespaceChar);

std::string FileUtils::SafeGetenv(const char *envVar) {
  const char *tmpEnvPtr = std::getenv(envVar);
  if (tmpEnvPtr == nullptr) {
    return "";
  }
  std::string tmpStr(tmpEnvPtr);
  return tmpStr;
}

std::string FileUtils::ExecuteShell(const char *cmd, std::string workspace) {
  if (!workspace.empty()) {
    char buffer[PATH_MAX];
    CHECK_FATAL(getcwd(buffer, sizeof(buffer)) != nullptr, "get current path failed");
    CHECK_FATAL(chdir(workspace.c_str()) == 0, "change workspace failed");
    workspace = buffer;
  }
  std::string result = ExecuteShell(cmd);
  if (!workspace.empty()) {
    CHECK_FATAL(chdir(workspace.c_str()) == 0, "change workspace failed");
  }
  return result;
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
  return path + kFileSeperatorStr;
}

void FileUtils::Mkdirs(const std::string &path, mode_t mode) {
  size_t pos = 0;
  while ((pos = path.find_first_of(kFileSeperatorStr, pos + 1)) != std::string::npos) {
    std::string dir = path.substr(0, pos);
    if (access(dir.c_str(), W_OK | R_OK) != 0) {
      CHECK_FATAL(mkdir(dir.c_str(), mode) == 0, "Failed to create tmp folder");
    }
  }
}

std::string FileUtils::GetOutPutDirInTmp(const std::string &inputFile) {
  std::string tmpDir = FileUtils::GetInstance().GetTmpFolder();
  if (inputFile.find(kFileSeperatorStr) == std::string::npos) {
    return tmpDir;
  }
  tmpDir.pop_back();
  std::string realFilePath = FileUtils::GetRealPath(inputFile);
  std::string fileDir = StringUtils::GetStrBeforeLast(realFilePath, kFileSeperatorStr, true) + kFileSeperatorStr;
  std::string dirToMake = tmpDir + fileDir;
  mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
  Mkdirs(dirToMake, mode);
  return dirToMake;
}

std::string FileUtils::GetRealPath(const std::string &filePath, const bool isCheck) {
  if (filePath.size() > PATH_MAX) {
    CHECK_FATAL(false, "invalid file path");
  }
#ifdef _WIN32
  char *path = nullptr;
  bool isReal = PathCanonicalize(path, filePath.c_str();
#else
  char path[PATH_MAX] = {0};
  bool isReal = realpath(filePath.c_str(), path) != nullptr;
#endif
  CHECK_FATAL(!(isCheck && !isReal), "invalid file path %s", filePath.c_str());
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

InputFileType FileUtils::GetFileTypeByExtension(const std::string &extensionName) {
  for (auto item : kTypeInfos) {
    for (auto ext : item.tmpSuffix) {
      if (ext == extensionName) {
        return item.fileType;
      }
    }
  }
  return InputFileType::kFileTypeNone;
}

InputFileType FileUtils::GetFileType(const std::string &filePath) {
  std::string extensionName = GetFileExtension(filePath);
  InputFileType type = GetFileTypeByExtension(extensionName);
  if (type == InputFileType::kFileTypeObj) {
    type = GetFileTypeByMagicNumber(filePath);
  } else if (type == InputFileType::kFileTypeMpl || type == InputFileType::kFileTypeBpl) {
    if (filePath.find("VtableImpl") == std::string::npos) {
      if (filePath.find(".me.mpl") != std::string::npos) {
        type = InputFileType::kFileTypeMeMpl;
      } else {
        type = extensionName == "mpl" ? InputFileType::kFileTypeMpl : InputFileType::kFileTypeBpl;
      }
    } else {
      type = InputFileType::kFileTypeVtableImplMpl;
    }
  } else if (type == InputFileType::kFileTypeNone) {
    std::string regexPattern = R"(\.(so|a)+((?:\.\d+)*)$)";
    if (StringUtils::IsMatchAtEnd(filePath, regexPattern)) {
      type = InputFileType::kFileTypeLib;
    }
  }
  return type;
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

bool FileUtils::CreateFile(const std::string &file) {
  if (IsFileExists(file)) {
    return true;
  }
  std::ofstream fileCreate;
  fileCreate.open(file);
  if (!fileCreate) {
    return false;
  }
  fileCreate.close();
  return true;
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
  return Rmdirs(FileUtils::GetInstance().GetTmpFolder());
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
  switch (magic) {
    case kMagicAST:
      return InputFileType::kFileTypeOast;
    case kMagicELF:
      return InputFileType::kFileTypeObj;
    default:
      return InputFileType::kFileTypeNone;
  }
}

bool FileUtils::IsSupportFileType(InputFileType type) {
  for (auto item : kTypeInfos) {
    if (item.fileType == type) {
      return item.support;
    }
  }
  return true;
}

void FileUtils::GetFileNames(const std::string path, std::vector<std::string> &filenames) {
  DIR *pDir = nullptr;
  struct dirent *ptr;
  if (!(pDir = opendir(path.c_str()))) {
    return;
  }
  struct stat s;
  while ((ptr = readdir(pDir)) != nullptr) {
    std::string filename = path + ptr->d_name;
    if (stat(filename.c_str(), &s) == 0) {
      if ((s.st_mode & S_IFREG) != 0) {
        filenames.push_back(filename);
      }
    }
  }
  CHECK_FATAL(closedir(pDir) == 0, "close dir failed");
}

std::unordered_map<std::string, bool> libName;

bool FileUtils::GetAstFromLib(const std::string libPath, std::vector<std::string> &astInputs) {
  if (libName.count(libPath) > 0) {
    return libName[libPath];
  }
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
  std::vector<std::string> filenames;
  result = ExecuteShell(cmd.c_str());
  StringUtils::Split(result, filenames, '\n');
  std::random_device rd;
  std::mt19937 gen(rd());
  std::string tmpLibFolder = GetInstance().GetTmpFolder() + std::to_string(gen()) + kFileSeperatorStr;
  mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
  Mkdirs(tmpLibFolder, mode);
  cmd = binAr + " x " + realLibPath;
  CHECK_FATAL(ExecuteShell(cmd.c_str(), tmpLibFolder) == "", "Failed to execute %s.", cmd.c_str());
  for (const auto &file : filenames) {
    std::string tmpPath = tmpLibFolder + file;
    if (GetFileTypeByMagicNumber(tmpPath) == InputFileType::kFileTypeOast) {
      astInputs.push_back(tmpPath);
    } else {
      elfFlag = true;
    }
  }
  libName.emplace(std::pair<std::string, bool>(libPath, elfFlag));
  return elfFlag;
}

std::string FileUtils::GetGccBin() {
#ifdef ANDROID
  return "prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/";
#else
  if (SafeGetenv(kGccPath) != "") {
    std::string gccPath = SafeGetenv(kGccPath) + " -dumpversion";
    CheckGCCVersion(gccPath.c_str());
    return SafeGetenv(kGccPath);
  } else if (SafeGetenv(kMapleRoot) != "") {
    return SafeGetenv(kMapleRoot) + "/tools/bin/aarch64-linux-gnu-gcc";
  }
  std::string gccPath = SafeGetPath("which aarch64-linux-gnu-gcc", "aarch64-linux-gnu-gcc") + " -dumpversion";
  CheckGCCVersion(gccPath.c_str());
  return SafeGetPath("which aarch64-linux-gnu-gcc", "aarch64-linux-gnu-gcc");
#endif
}

bool FileUtils::Rmdirs(const std::string &dirPath) {
  DIR *dir = opendir(dirPath.c_str());
  if (dir == nullptr) {
      return false;
  }
  dirent *entry = nullptr;
  while ((entry = readdir(dir)) != nullptr) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }
    std::string fullPath = std::string(dirPath) + kFileSeperatorChar + entry->d_name;
    if (entry->d_type == DT_DIR) {
      if (!Rmdirs(fullPath)) {
        (void)closedir(dir);
        return false;
      }
    } else {
        if (remove(fullPath.c_str()) != 0) {
          (void)closedir(dir);
          return false;
        }
    }
  }
  (void)closedir(dir);
  if (rmdir(dirPath.c_str()) != 0) {
    return false;
  }
  return true;
}

std::string FileUtils::GetCurDirPath() {
  char *cwd = get_current_dir_name();
  std::string path(cwd);
  free(cwd);
  cwd = nullptr;
  return path;
}

static constexpr std::streamsize kASTHeaderSize = 4;
static constexpr std::streamsize kBlockSize = 4;
static constexpr std::streamsize kWordSize = 4;
static constexpr std::streamsize koptStringIdSize = 3;
static constexpr std::streamsize kmagicNumberSize = 4;
static constexpr std::streamsize kControlBlockHeadSize = 8;
static constexpr std::streamsize kOptStringHeader = 5;
static constexpr std::streamsize kOptStringInfoSize = 4;
static constexpr std::streamsize kOptStringId = 0x5442;
static constexpr char kClangASTMagicNum[] = "CPCH";
size_t FileUtils::GetIntegerFromMem(std::ifstream &fs, char *buffer, std::streamsize bufferSize,
    bool isNeedChkOptStrId) {
  (void)fs.read(buffer, bufferSize);
  if (isNeedChkOptStrId) {
    // we use optStringId first byte and second byte with last byte low 3bits(0x7U) to compare
    buffer[bufferSize - 1] = static_cast<uint8_t>(buffer[bufferSize - 1]) & 0x7U;
  }
  size_t integerSize = 0;
  errno_t ret = memcpy_s(&integerSize, sizeof(integerSize), buffer, static_cast<size_t>(bufferSize));
  CHECK_FATAL(ret == EOK, "get Integer From Memory failed");
  return integerSize;
}

void FileUtils::GetOptString(std::ifstream &fs, std::string &optString) {
  (void)fs.seekg(kOptStringHeader, std::ios::cur);
  char optStringInfo[kOptStringInfoSize];
  std::streamsize index = kOptStringInfoSize;
  while (index == kOptStringInfoSize) {
    (void)fs.read(optStringInfo, static_cast<std::streamsize>(sizeof(optStringInfo)));
    index = 0;
    while (index < kOptStringInfoSize && optStringInfo[index] != '\0') {
      optString += optStringInfo[index];
      index++;
    }
  }
}

std::string FileUtils::GetClangAstOptString(const std::string astFilePath) {
  std::string optString;
  std::ifstream fs(astFilePath);
  if (!fs.is_open()) {
    return "";
  }
  char magicNumber[kmagicNumberSize];
  (void)fs.read(magicNumber, static_cast<std::streamsize>(sizeof(magicNumber)));
  if (memcmp(magicNumber, kClangASTMagicNum, sizeof(magicNumber)) == 0) {
    (void)fs.seekg(kASTHeaderSize, std::ios::cur);
    char blockInfoBlockSizeInfo[kBlockSize];
    size_t blockInfoBlockSize = GetIntegerFromMem(fs, blockInfoBlockSizeInfo, kBlockSize, false);
    (void)fs.seekg(static_cast<std::streamsize>(blockInfoBlockSize * kWordSize) + kControlBlockHeadSize, std::ios::cur);
    char optStringId[koptStringIdSize];
    std::streamsize optStringIdInfo =
        static_cast<std::streamsize>(GetIntegerFromMem(fs, optStringId, koptStringIdSize, true));
    if (optStringIdInfo == kOptStringId) {
      GetOptString(fs, optString);
    }
  }

  fs.close();
  return optString;
}

// Just read a line from the file. It doesn't do anything more.
// The reason for this wrapper function is to provide a single entry to
// read file. Thus we can easily manipulate the line number, column number,
// cursor, etc.
// Returns true : if file is good()
//        false : if file is not good()
bool FileReader::ReadLine() {
  (void)std::getline(mDefFile, mCurLine);
  mLineNo++;
  if (mDefFile.good()) {
    return true;
  } else {
    return false;
  }
}

std::string FileReader::GetLine(const std::string keyWord) {
  while (ReadLine()) {
    if (mCurLine.length() == 0) {
      continue;
    }
    if (mCurLine.find(keyWord) != std::string::npos) {
      return StringUtils::TrimWhitespace(mCurLine);
    }
    if (EndOfFile()) {
      break;
    }
  }
  return "";
}

}  // namespace maple
