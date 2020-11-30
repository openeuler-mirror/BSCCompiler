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
#include "loader/loader_utils.h"

#include <climits>

#include "utils/string_utils.h"
#include "base/file_utils.h"
#include "loader_api.h"
#include "file_system.h"
#include "linker_api.h"
#include "file_adapter.h"

using namespace maple;
namespace maplert {
namespace loaderutils {
bool IsZipMagic(uint32_t magic) {
  const uint8_t lowZipBit = 8;
  const uint8_t lowZipMask = 0xff;
  return (((magic & lowZipMask) == 'P') && (((magic >> lowZipBit) & lowZipMask) == 'K'));
}
std::string GetNicePath(const std::string &path) {
  // Make canonical path.
  std::string file(path);
  if (file.find("/./") != std::string::npos || file.find("/../") != std::string::npos) {
    if (file.length() > PATH_MAX) {
      CL_LOG(ERROR) << "the path name exceeds the length limit! " << file.length() << ", " << file << maple::endl;
      return "";
    }
    char canonical[PATH_MAX + 1] = { 0 };
    if (realpath(file.c_str(), canonical)) {
      file = canonical;
    } else {
      CL_LOG(ERROR) << "returned null! " << file << maple::endl;
      return "";
    }
  }
  return file;
}
std::string GetJarPath(const std::string &curDir,
    const std::string &fileWithPostfix, bool isAppDataPath, bool isSystemPath) {
  if (stringutils::WithSuffix(fileWithPostfix.c_str(), ".jar")) {
    size_t jarPos = fileWithPostfix.rfind(".jar");
    std::string newFileWithPostFix(fileWithPostfix);
    newFileWithPostFix = "libmaple" + newFileWithPostFix.replace(jarPos, sizeof(".jar"), ".so");
    // PATH 2: system jar in /system/ or /apex/, and end with .jar
    // return /system/lib64/libmaple<jar-name-without-postfix>.so
    if (isSystemPath) {
      std::string newFilePath = maple::fs::kSystemLibPath + newFileWithPostFix;
      CL_VLOG(classloader) << "return " << newFilePath << maple::endl;
      return newFilePath;
    }

    // PATH 3: not starts with /data/data/ and /data/user/, and end with .jar
    // return <current-dir>/maple/arm64/libmaple<jar-name-without-postfix>.so
    if (!isAppDataPath) {
      CL_VLOG(classloader) << "return " << curDir << "maple/arm64/" << newFileWithPostFix << maple::endl;
      return curDir + "maple/arm64/" + newFileWithPostFix;
    }
  }
  return "";
}
std::string GetApkPath(const std::string &curDir, const std::string &fileWithPostfix, bool isAppDataPath) {
  const std::string appSoPostfix = "maple/arm64/mapleclasses.so";
  const std::string appPartialSoPostfix = "maple/arm64/maplepclasses.so";
  if (stringutils::WithSuffix(fileWithPostfix.c_str(), ".apk")) {
    // PATH 4: not starts with /data/data/ and /data/user/, and end with .apk
    // return <current-dir>/maple/arm64/mapleclasses.so for full AOT
    // return <current-dir>/maple/arm64/maplepclasses.so for partial AOT
    if (!isAppDataPath) {
      std::string result = curDir + appSoPostfix;
      // check PATH 4 existence
      if (FileUtils::FileExists(result)) {
        CL_VLOG(classloader) << "return " << result << maple::endl;
        return result;
      }
      // check partial classes
      result = curDir + appPartialSoPostfix;
      if (FileUtils::FileExists(result)) {
        CL_VLOG(classloader) << "return " << result << maple::endl;
        return result;
      }
    }
  }
  return "";
}
std::string Dex2MplPath(const std::string &filePath) {
  std::string file(filePath);
  CL_VLOG(classloader) << "input " << file << maple::endl;

#ifndef __OPENJDK__
  size_t curDirPos = file.rfind('/');
  // PATH 1: if there is no path, just file name
  // return input directly, like libmaplecore-all.so
  if (curDirPos == std::string::npos) {
    CL_VLOG(classloader) << "return " << file << maple::endl;
    return file;
  }

  std::string curDir = file.substr(0, curDirPos + 1);
  std::string fileWithPostfix = file.substr(curDirPos + 1);

  // PATH 2: system path starts with /system/ or /apex/
  bool isAppDataPath = stringutils::WithPrefix(file.c_str(), "/data/data/") ||
      stringutils::WithPrefix(file.c_str(), "/data/user/");
  bool isSystemPath = stringutils::WithPrefix(file.c_str(), "/system/") ||
      stringutils::WithPrefix(file.c_str(), "/apex/");

  std::string resultPath = GetJarPath(curDir, fileWithPostfix, isAppDataPath, isSystemPath);
  if (!resultPath.empty()) {
    return resultPath;
  }

  resultPath = GetApkPath(curDir, fileWithPostfix, isAppDataPath);
  if (!resultPath.empty()) {
    return resultPath;
  }

  // return <current-dir>/maple/arm64/libmaple<name-with-postfix>.so
  if (isAppDataPath) {
    std::string result = curDir + "maple/arm64/libmaple" + fileWithPostfix + ".so";
    // check PATH 5 existence
    if (FileUtils::FileExists(result)) {
      CL_VLOG(classloader) << "return " << result << maple::endl;
      return result;
    }
  }

  // read and check file magic number
  std::unique_ptr<File> fileInput;
  fileInput.reset(FileUtils::OpenFileReadOnly(file));
  if (fileInput.get() != nullptr) {
    uint32_t magic = 0;
    int ret = static_cast<int>(fileInput->Read(
        reinterpret_cast<char*>(&magic), static_cast<int64_t>(sizeof(magic)), 0));
    if ((ret == static_cast<int>(sizeof(magic))) && IsZipMagic(magic)) {
      // PATH 6: if the others all failed
      // return <file-path>!/maple/arm64/mapleclasses.so
      CL_VLOG(classloader) << "return " << file << "!/maple/arm64/mapleclasses.so" << maple::endl;
      return file + "!/maple/arm64/mapleclasses.so";
    }
  }

  // default path
  CL_VLOG(classloader) << "return " << file << maple::endl;
  return file;
#else
  CL_VLOG(classloader) << "return " << file << maple::endl;
  return file;
#endif
}

bool CheckVersion(const ObjFile &mplFile, const LinkerMFileInfo &mplInfo) {
  maplert::MapleVersionT mplFileVersion;
  maplert::LinkerAPI::Instance().GetMplVersion(mplInfo, mplFileVersion);
  if (mplFileVersion.mplMajorVersion > Version::kMajorMplVersion) {
    CL_LOG(ERROR) << "Compiler and Runtime major version are inconsistent in " << mplFile.GetName() <<
        " Compiler version : " << mplFileVersion.mplMajorVersion << "." << mplFileVersion.compilerMinorVersion <<
        ", Runtime version : " << Version::kMajorMplVersion << "." << Version::kMinorRuntimeVersion << maple::endl;
    return false;
  }
  return true;
}

bool CheckCompilerStatus(const ObjFile &mplFile, const LinkerMFileInfo &mplInfo) {
  // check mpl file compiler status
  constexpr uint32_t gcOnlyMask = 1;
  uint32_t compilerStatus = 0;
  maplert::LinkerAPI::Instance().GetMplCompilerStatus(mplInfo, compilerStatus);
  AdapterExAPI *adapter = maplert::LoaderAPI::Instance().GetAdapterEx().As<AdapterExAPI*>();
  bool gcOnlyInProcess = adapter->IsGcOnly();
  bool gcOnlyInMfile = static_cast<bool>(compilerStatus & gcOnlyMask);

  // if mfile is gc only, the process must be gconly mode;
  if (gcOnlyInMfile && !gcOnlyInProcess) {
    CL_LOG(FATAL) << "Compiler status: " << compilerStatus << " in " << mplFile.GetName() <<
        " dont match the process gc only mode" << maple::endl;
  }

  // in zygote startup mode, all mfile should be rc and process in rc mode
  // in application startup mode, if in process rc mode, all after-loaded mfle
  // should be rc mfile, otherwise in process gc-only mode, all after-loaded mfile
  // can be rc mfile or gc mfile. butr rc mfile don't friendly performance
  if (gcOnlyInMfile != gcOnlyInProcess) {
    CL_LOG(INFO) << "Compiler status: " << compilerStatus << " in " << mplFile.GetName() << " in not-friendly mode" <<
        " process is in" << (gcOnlyInProcess ? " gconly mode" : " rc mode") << maple::endl;
  }
  return true;
}
}
} // end namespace maplert
