/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef OPENARKCOMPILER_LITEPGO_H
#define OPENARKCOMPILER_LITEPGO_H

#include <set>
#include <string>
#include <sstream>
#include "types_def.h"

namespace maple {
class MIRLexer;
class LiteProfile {
 public:
  struct BBInfo {
    uint32 funcHash = 0;
    std::vector<uint32> counter;
    std::pair<bool, bool> verified = {false, false};
    BBInfo() = default;
    BBInfo(uint64 hash, std::vector<uint32> &&counter)
        : funcHash(hash), counter(counter) {}
    BBInfo(uint64 hash, const std::initializer_list<uint32> &iList)
        : funcHash(hash), counter(iList) {}
    ~BBInfo() = default;
  };
  // default get all kind profile
  bool HandleLitePGOFile(const std::string &fileName, const std::string &moduleName);
  bool HandleLitePgoWhiteList(const std::string &fileName) const;
  BBInfo *GetFuncBBProf(const std::string &funcName);
  bool isExtremelyCold(const std::string &funcName) {
    return extremelyColdFuncs.count(funcName);
  }
  static bool IsInWhiteList(const std::string &funcName) {
    return whiteList.empty() ? true : (whiteList.count(funcName) != 0);
  }
  static uint32 GetBBNoThreshold() {
    return bbNoThreshold;
  }
  static std::string FlatenName(const std::string &name);

 private:
  static std::set<std::string> whiteList;
  static uint32 bbNoThreshold;
  static bool loaded;
  bool debugPrint = false;
  std::unordered_map<std::string, BBInfo> funcBBProfData;
  std::set<std::string> extremelyColdFuncs;
  void ParseFuncProfile(MIRLexer &fdLexer, const std::string &moduleName);
  void ParseCounters(MIRLexer &fdLexer, const std::string &funcName, uint32 cfghash);
};
}
#endif // OPENARKCOMPILER_LITEPGO_H
