#ifndef OPENARKCOMPILER_LITEPGO_H
#define OPENARKCOMPILER_LITEPGO_H

#include "types_def.h"
#include <set>
#include <string>
#include <sstream>

namespace maple {
class MIRLexer;
class LiteProfile {
 public:
  struct BBInfo {
    uint32 funcHash = 0;
    std::vector<uint32> counter;
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
    return whiteList.empty() ? true : whiteList.count(funcName);
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
#endif //OPENARKCOMPILER_LITEPGO_H
