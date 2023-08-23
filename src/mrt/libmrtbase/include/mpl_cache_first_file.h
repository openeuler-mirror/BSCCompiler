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

#ifndef MPL_CACHE_FIRST_FILE_H
#define MPL_CACHE_FIRST_FILE_H

#include <ostream>
#include <sstream>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <sys/uio.h>
#include "base/macros.h"

namespace maple {
// equal MplLinkCacheType::kCacheButt
const int32_t kDecoupleCacheTypeNum = 10;          // 10 means cache file num(equal CACHE_FILE_NUM in mpl_cache_gen.cpp)
static const int32_t kDecoupleCacheTypeStart = 2;  // cache type start from 2, which means vtb cache.
static const int32_t kMaxNonStubSoNum = 999;       // non stub so's idx must be less than 999
static const int32_t kMultiSoStart = 2;            // multi so start from 2

struct LinkerCacheFileFdPerSo {
  int32_t mplDecoupleCacheFd[kDecoupleCacheTypeNum];
  bool mplStubSoExist;
  uint32_t validCacheTypeNum;
};

class MplCacheFirstFile {
 public:
  static bool GetInstallStat();
  static void SetInstallStat();
  static void ResetInstallStat();
  static int32_t GetFd(const std::string &mplSoPath, uint32_t cacheType);
  static void ResetFdAll();
  static bool GetNeedStatFlag();
  static void SetNeedStatFlag(bool value);
  static bool GetGenerateFileExit();
  static void SetGenerateFileExit(bool value);
  static void SetNonStubFdStr(std::string &str);
  static void SetStubFdStr(std::string &str);
  static bool ParseFdStr();
 private:
  static bool GetStrWithDivision(std::string& mplSoFdStrRemain, std::string& mplSoFdStrOut,
                                 const std::string& divisionStr);
  static bool ParseFdStrOneSo(const std::string &mplSoFdStrPerSo, bool mplStubSoExist);
  static bool GetSoIdxFromPath(const std::string &mplSoPath, uint32_t &soIdx, bool &stubSoFlag);
  static bool mplDecoupleInstallStat;
  static bool needStatFlag;
  static bool generateFileExit;
  static std::string mplSoFdStrArg;
  static std::string mplStubSoFdStrArg;
  static std::vector<LinkerCacheFileFdPerSo> mplCacheFileFd;
};
}  // namespace maple

#endif  // MPL_CACHE_FIRST_FILE_H
