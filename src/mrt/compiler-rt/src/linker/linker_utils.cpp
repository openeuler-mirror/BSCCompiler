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
#include "linker/linker_utils.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dlfcn.h>

#include "mrt_object.h"
#include "mclass_inline.h"
#include "modifier.h"
#include "mrt_object.h"

namespace maplert {
namespace linkerutils {
static int64_t kGlobalVersionCode = -1;
static LoadStateType kGlobalLoadState = kLoadStateNone;
static AppLoadState kGlobalAppLoadState = kAppLoadNone;
static std::string kGlobalAppDataDir;
static std::string kGlobalAppBaseStr;
// Release the (physical) memory to kernel. Note: only for RO sections
void ReleaseMemory(void *start, void *end) {
  constexpr int maxPageSize = 15;
  uintptr_t startAddr = reinterpret_cast<uintptr_t>(start);
  uintptr_t endAddr = reinterpret_cast<uintptr_t>(end);
  if (startAddr == 0 || endAddr == 0 || startAddr >= endAddr) {
    return;
  }

  // round up start to page size: assuming 4K page size, 4K=2^12
  startAddr = (startAddr + (1 << 12) - 1) & (~(static_cast<uintptr_t>((1 << 12) - 1)));
  if (endAddr < (startAddr + (1 << maxPageSize))) {
    return; // the region is not big enough, just skip
  }
  // calculate section size, and round it to 4K page size, 4K=2^12
  size_t length = (endAddr - startAddr) & (~(static_cast<uintptr_t>((1 << 12) - 1)));

  // doing madvise, ignore its return status, it should be safe for text/ro sections.
  (void)(madvise(reinterpret_cast<void*>(startAddr), length, MADV_DONTNEED));
}

// For un|define tables' address.
void *GetSymbolAddr(void *handle, const char *symbol, bool isFunction) {
  if (handle == nullptr) {
    LINKER_VLOG(mpllinker) << "failed, handle is null" << maple::endl;
    return nullptr;
  }

  PtrFuncType func = reinterpret_cast<PtrFuncType>(dlsym(handle, symbol));
  if (func == nullptr) {
    LINKER_VLOG(mpllinker) << "failed, function " << symbol << " is null" << maple::endl;
    return nullptr;
  }
  return isFunction ? func() : reinterpret_cast<void*>(func);
}

// Get maxmium version from range table.
int32_t GetMaxVersion() {
  constexpr int maxSize = 32;
  if (kGlobalLoadState == kLoadStateApk) {
    return static_cast<int32_t>(static_cast<uint64_t>(kGlobalVersionCode) >> maxSize);
  }
  return 0;
}

// Get minmium version from range table.
int32_t GetMinVersion() {
  if (kGlobalLoadState == kLoadStateApk) {
    return static_cast<int32_t>(kGlobalVersionCode);
  }
  return 0;
}

void GetMplVersion(const LinkerMFileInfo &mplInfo, MapleVersionT &version) {
  void *begin = static_cast<void*>(GetSymbolAddr(mplInfo.handle, kMapleVersionBegin, true));
  void *end = static_cast<void*>(GetSymbolAddr(mplInfo.handle, kMapleVersionEnd, true));
  if (begin == end) {
    LINKER_VLOG(mpllinker) << mplInfo.name << " has no version number!\n" << maple::endl;
    return;
  }
  MapleVersionT *item = static_cast<MapleVersionT*>(begin);
  version.mplMajorVersion = item->mplMajorVersion;
  version.compilerMinorVersion = item->compilerMinorVersion;
}

void GetMplCompilerStatus(const LinkerMFileInfo &mplInfo, uint32_t &status) {
  void *begin = static_cast<void*>(GetSymbolAddr(mplInfo.handle, kMapleCompileStatusBegin, true));
  void *end = static_cast<void*>(GetSymbolAddr(mplInfo.handle, kMapleCompileStatusEnd, true));
  if (begin == end) {
    status = 0;
    return;
  }
  status = *(static_cast<uint32_t*>(begin));
}

std::string GetAppPackageName() {
  size_t pos = kGlobalAppDataDir.find_last_of('/');
  std::string packageName = kGlobalAppDataDir.substr(pos + 1);
  return packageName;
}

const std::string &GetAppInfo() {
  return kGlobalAppDataDir;
}
void ClearAppInfo() {
  kGlobalAppDataDir.clear();
}
const LoadStateType &GetLoadState() {
  return kGlobalLoadState;
}
void SetLoadState(const LoadStateType state) {
  kGlobalLoadState = state;
}
const AppLoadState &GetAppLoadState() {
  return kGlobalAppLoadState;
}
void SetAppLoadState(const AppLoadState state) {
  kGlobalAppLoadState = state;
}
const std::string &GetAppBaseStr() {
  return kGlobalAppBaseStr;
}
void SetAppBaseStr(const std::string &str) {
  kGlobalAppBaseStr = str;
}
// Set running app's state, if data path and version code are valid.
void SetAppInfo(const char *dataPath, int64_t appVersionCode) {
  if (kGlobalLoadState == kLoadStateApk) {
    LINKER_LOG(ERROR) << "(" << dataPath << ", " << appVersionCode << "), failed, already set before? appDataDir=" <<
        kGlobalAppDataDir << ", versionCode=" << kGlobalVersionCode << maple::endl;
    return;
  }

  kGlobalAppDataDir = std::string(dataPath);
  kGlobalVersionCode = appVersionCode;
  if (!kGlobalAppDataDir.empty() && kGlobalVersionCode > 0) {
    kGlobalLoadState = kLoadStateApk;
    kGlobalAppLoadState = kAppLoadBaseOnlyReady;
  } else {
    LINKER_LOG(ERROR) << "(" << dataPath << ", " << appVersionCode << "), failed, appDataDir=" << kGlobalAppDataDir <<
        ", versionCode=" << kGlobalVersionCode << maple::endl;
    kGlobalLoadState = kLoadStateNone;
  }
}

bool NeedRelocateSymbol(const std::string &name) {
  // 900,4088... is VersionCode.
  static std::map<std::string, int64_t> apps = {
      { "com.ss.android.article.news", 900 },
      { "com.sina.weibolite", 4088 },
      { "com.xunmeng.pinduoduo", 46600 },
      { "com.tencent.kg.android.lite", 282 },
      { "com.Qunar", 198 },
      { "com.wuba", 82400 }
  };
  for (auto &app : apps) {
    if (kGlobalVersionCode == app.second) {
      if (name.find(app.first) != std::string::npos) {
        return false;
      }
    }
  }
  return true;
}

void *GetDefTableAddress(const LinkerMFileInfo &mplInfo, const AddrSlice &defSlice, size_t index, bool forMethod) {
  if (mplInfo.IsFlag(kIsLazy) || (!mplInfo.IsFlag(kIsRelDataOnce) && !forMethod) ||
      (!mplInfo.IsFlag(kIsRelMethodOnce) && forMethod)) {
    return defSlice[index].AddressFromOffset();
  } else {
    return defSlice[index].Address();
  }
}

const char *GetMuidDefFuncTableFuncname(const LinkerInfTableItem &pInfTable) {
  const DataRefOffset32 *data = reinterpret_cast<const DataRefOffset32*>(&pInfTable.funcNameOffset);
  MethodMetaBase *methodInfo = data->GetDataRef<MethodMetaBase*>();
  return methodInfo->GetName();
}

std::string GetMuidDefFuncTableSigname(const LinkerInfTableItem &pInfTable) {
  const DataRefOffset32 *data = reinterpret_cast<const DataRefOffset32*>(&pInfTable.funcNameOffset);
  MethodMetaBase *methodInfo = data->GetDataRef<MethodMetaBase*>();
  std::string signature;
  methodInfo->GetSignature(signature);
  return signature;
}

const char *GetMuidDefFuncTableClassname(const LinkerInfTableItem &pInfTable) {
  const DataRefOffset32 *data = reinterpret_cast<const DataRefOffset32*>(&pInfTable.funcNameOffset);
  MethodMetaBase *methodInfo = data->GetDataRef<MethodMetaBase*>();
  MClass *klass = methodInfo->GetDeclaringClass();
  if (klass == nullptr) {
    LINKER_LOG(FATAL) << "klass is null" << maple::endl;
  }
  return klass->GetName();
}

// Get method symbol by the offsets of class name, method name and signature.
std::string GetMethodSymbolByOffset(const LinkerInfTableItem &infTable) {
  std::string sym;
  const char *name = GetMuidDefFuncTableClassname(infTable);
  if (name != nullptr) {
    sym += name;
    sym += '|';
  } else {
    LINKER_LOG(WARNING) << "class name is null!?" <<  maple::endl;
  }

  name = GetMuidDefFuncTableFuncname(infTable);
  if (name != nullptr) {
    sym += name;
    sym += '|';
  } else {
    LINKER_LOG(WARNING) << "function name is null!?" <<  maple::endl;
  }

  std::string signature = GetMuidDefFuncTableSigname(infTable);
  if (!signature.empty()) {
    sym += signature;
  } else {
    LINKER_LOG(WARNING) << "signature is null!?" <<  maple::endl;
  }
  return sym;
}

#ifdef LINKER_RT_CACHE
constexpr uint8_t kLinkerCacheTypeStrSize = 11; // 11 is LinkerCacheType::kLinkerCacheLazy + 1
constexpr char kLinkerLazyCacheSuffix[] = ".lzy";
constexpr char kLinkerUndefCacheSuffixFcn[] = ".fcn";
constexpr char kLinkerDefCacheSuffixFcn[] = ".def.fcn";
constexpr char kLinkerUndefCacheSuffixVar[] = ".var";
constexpr char kLinkerDefCacheSuffixVar[] = ".def.var";
constexpr char kLinkerCacheSuffixVTable[] = ".vtb";
constexpr char kLinkerCacheSuffixCTable[] = ".ctb";
constexpr char kLinkerCacheSuffixITable[] = ".itb";
constexpr char kLinkerCacheSuffixFieldOffset[] = ".fos";
constexpr char kLinkerCacheSuffixFieldTable[] = ".ftb";
constexpr char kLinkerCacheSuffixStaAddrTable[] = ".sta";
const std::string LinkerCacheTypeStr[kLinkerCacheTypeStrSize] = {
    kLinkerUndefCacheSuffixVar,
    kLinkerDefCacheSuffixVar,
    kLinkerUndefCacheSuffixFcn,
    kLinkerDefCacheSuffixFcn,
    kLinkerCacheSuffixVTable,
    kLinkerCacheSuffixCTable,
    kLinkerCacheSuffixFieldOffset,
    kLinkerCacheSuffixFieldTable,
    kLinkerCacheSuffixITable,
    kLinkerCacheSuffixStaAddrTable,
    kLinkerLazyCacheSuffix
};

// get LinkerCacheTypeStr
const std::string &GetLinkerCacheTypeStr(LinkerCacheType index) {
  return LinkerCacheTypeStr[static_cast<int>(index)];
}

bool FileExists(const char *name) {
  struct stat st;
  if (stat(name, &st) == 0) {
    return S_ISREG(st.st_mode);
  } else {
    return false;
  }
}

bool FolderExists(const char *name) {
  struct stat st;
  if (stat(name, &st) == 0) {
    return S_ISDIR(st.st_mode);
  } else {
    return false;
  }
}

bool PrepareFolder(const std::string &dir) {
  size_t first = 0;
  size_t second = 0;
  struct stat dirStat;
  if (stat(dir.c_str(), &dirStat) == 0) {
    if (!S_ISDIR(dirStat.st_mode)) {
      LINKER_LOG(ERROR) << "exist, but not directory! " << dir << maple::endl;
      return false;
    } else {
      return true;
    }
  }

  // It's not "No such file or directory"?
  if (errno != ENOENT) {
    LINKER_LOG(ERROR) << strerror(errno) << maple::endl;
    return false;
  }

  for(;;) {
    // Get the parent directory.
    first = second;
    if (first == dir.length() - 1) { // Exit if reach the end, or root /.
      return true;
    }
    second = dir.find('/', first + 1);
    if (second == std::string::npos) { // If it's not ended by /.
      second = dir.length() - 1;
    }

    // Check if exist.
    std::string substr = dir.substr(0, second + 1);
    if (stat(substr.c_str(), &dirStat) == 0) {
      if (!S_ISDIR(dirStat.st_mode)) {
        LINKER_LOG(ERROR) << "parent exist, but not directory! " << substr << maple::endl;
        return false;
      } else {
        LINKER_DLOG(mpllinker) << "parent exist! " << substr << maple::endl;
        continue;
      }
    }

    // Create the folder.
    mode_t mode = S_IRWXU | S_IRWXG | S_IXOTH;
    if (mkdir(substr.c_str(), mode) != 0) {
      LINKER_LOG(ERROR) << "failed to create dir " << substr << ":" << strerror(errno) << maple::endl;
      return false;
    }
    if (chmod(substr.c_str(), mode) != 0) {
      LINKER_LOG(ERROR) << "failed to chmod dir " << substr << ":" << strerror(errno) << maple::endl;
      return false;
    }
    LINKER_DLOG(mpllinker) << "created dir " << substr << " successfully!" << maple::endl;
  }
}

// note: the install cache is specified by installd(see mpl_cache_gen.cpp)
static constexpr char kLinkerDataAppPath[] = "/data/app/";
static constexpr char kLinkerDalvikDcpCachePath[] = "/data/dalvik-cache/instdcp/";
bool GetInstallCachePath(LinkerMFileInfo &mplInfo, std::string &path, LinkerCacheType cacheType) {
  if (mplInfo.name.find(kLinkerDataAppPath) == 0) {
    // for data/app
    size_t pos = mplInfo.name.find('/', strlen(kLinkerDataAppPath));
    if (pos != std::string::npos) {
      path = mplInfo.name.substr(0, pos + 1);
      path += "instdcp/";
      std::string name = mplInfo.name.substr(pos + 1);
      std::replace(name.begin(), name.end(), '/', '_');
      path += name;
    } else {
      LINKER_VLOG(mpllinker) << "failed to get app path for " << mplInfo.name << maple::endl;
      return false;
    }
  } else {
    // for system/app
    path = kLinkerDalvikDcpCachePath;
    std::string name = mplInfo.name;
    std::replace(name.begin(), name.end(), '/', '_');
    path += name;
  }
  path += GetLinkerCacheTypeStr(cacheType);
  return true;
}
#endif
}
}
