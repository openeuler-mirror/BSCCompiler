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
#ifndef MAPLE_LINKER_UTILS_H_
#define MAPLE_LINKER_UTILS_H_

#include <link.h>
#include <stddef.h>
#include <linux/limits.h>

#include <string.h>
#include <iostream>
#include <string>

#include "linker_common.h"

namespace maplert {
namespace linkerutils {
void ReleaseMemory(void *start, void *end);

void *GetSymbolAddr(void *handle, const char *symbol, bool isFunction);

int32_t GetMaxVersion();
int32_t GetMinVersion();
void GetMplVersion(const LinkerMFileInfo &mplInfo, MapleVersionT &item);
void GetMplCompilerStatus(const LinkerMFileInfo &mplInfo, uint32_t &status);
std::string GetAppPackageName();
const std::string &GetAppInfo();
void ClearAppInfo();
void SetAppInfo(const char *dataPath, int64_t versionCode);
const LoadStateType &GetLoadState();
void SetLoadState(const LoadStateType state);
const AppLoadState &GetAppLoadState();
void SetAppLoadState(const AppLoadState state);
const std::string &GetAppBaseStr();
void SetAppBaseStr(const std::string &str);

bool NeedRelocateSymbol(const std::string &name);
void *GetDefTableAddress(const LinkerMFileInfo &mplInfo, const AddrSlice &pTable, size_t index, bool forMethod);
std::string GetMethodSymbolByOffset(const LinkerInfTableItem &pTable);
const char *GetMuidDefFuncTableFuncname(const LinkerInfTableItem &pTable);
std::string GetMuidDefFuncTableSigname(const LinkerInfTableItem &pTable);
const char *GetMuidDefFuncTableClassname(const LinkerInfTableItem &pTable);
// Return N microseconds.
// isEnd: false starts counting, true ends counting.
inline long CountTimeConsuming(bool isEnd) {
  static struct timespec timeSpec[2]; // 2 means startTimeSpec and endTimeSpec

  if (!isEnd) {
    clock_gettime(CLOCK_MONOTONIC, &timeSpec[0]);
    return 0;
  } else {
    clock_gettime(CLOCK_MONOTONIC, &timeSpec[1]);
    // 1000000 means MILLION, sec to microsec, 1000 means THOUAND, nsec to micromsec
    return (timeSpec[1].tv_sec - timeSpec[0].tv_sec) * 1000000LL + (timeSpec[1].tv_nsec - timeSpec[0].tv_nsec) / 1000LL;
  }
}

inline void DumpMUID(const MUID &muid) {
  for (unsigned int i = 0; i < kMuidLength; ++i) {
    fprintf(stderr, "\t%02x", muid.data.bytes[i]);
  }
}

inline void GenerateMUID(const void *data, unsigned long size, MUID &muid) {
  auto *srcStr = reinterpret_cast<const unsigned char*>(data);
  GetMUIDHash(*srcStr, size, muid);
}
inline void GenerateMUID(const char *const str, MUID &muid) {
  GenerateMUID(str, strlen(str), muid);
}

#ifdef LINKER_RT_CACHE
const std::string &GetLinkerCacheTypeStr(LinkerCacheType index);
bool FileExists(const char *name);
bool FolderExists(const char *name);
bool PrepareFolder(const std::string &dir);
bool GetInstallCachePath(LinkerMFileInfo &mplInfo, std::string &path, LinkerCacheType cacheType);
#endif
}
}
#endif // MAPLE_LINKER_UTILS_H_
