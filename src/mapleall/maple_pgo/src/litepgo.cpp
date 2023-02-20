#include "litepgo.h"
#include "itab_util.h"
#include "lexer.h"

namespace maple {
bool LiteProfile::loaded = false;
std::string LiteProfile::FlatenName(const std::string &name) {
  std::string filteredName = name;
  size_t startPos = name.find_last_of("/") == std::string::npos ? 0 : name.find_last_of("/") + 1U; // skip /
  size_t endPos = name.find_last_of(".") == std::string::npos ? 0 : name.find_last_of(".");
  CHECK_FATAL(endPos > startPos, "invalid module name");
  filteredName = filteredName.substr(startPos, endPos - startPos);
  std::replace(filteredName.begin(), filteredName.end(), '-', '_');
  return filteredName;
}

LiteProfile::BBInfo *LiteProfile::GetFuncBBProf(const std::string &funcName) {
  auto item = funcBBProfData.find(funcName);
  if (item == funcBBProfData.end()) {
    return nullptr;
  }
  return &item->second;
}

bool LiteProfile::HandleLitePGOFile(const std::string &fileName, const std::string &moduleName) {
  if (loaded) {
    LogInfo::MapleLogger() << "this Profile has been handled before" << '\n';
    return false;
  }
  loaded = true;
  /* init a lexer for parsing lite-pgo function data */
  MemPool *funcDatatMp = memPoolCtrler.NewMemPool("LitePgoFuncData Mempool", true);
  MapleAllocator funcDataMa(funcDatatMp);
  MIRLexer funcDataLexer(nullptr, funcDataMa);
  funcDataLexer.PrepareForFile(fileName);
  funcDataLexer.NextToken();
  bool atEof = false;
  while (!atEof) {
    TokenKind currentTokenKind = funcDataLexer.GetTokenKind();
    if (currentTokenKind == TK_eof) {
      atEof = true;
      continue;
    }
    if (currentTokenKind == TK_flavor) {
      /* skip beginning of lite-pgo data, not implement yet */
    }
    if (currentTokenKind == TK_func) {
      ParseFuncProfile(funcDataLexer, moduleName);
      continue;
    }
    funcDataLexer.NextToken();
  }
  delete funcDatatMp;
  return true;
}

/* lite-pgo keyword format ";${keyword}" */
static inline void ParseLitePgoKeyWord(MIRLexer &fdLexer, const std::string &keyWord) {
  /* parse counterSz */
  fdLexer.NextToken();
  if (fdLexer.GetTokenKind() != TK_coma) {
    CHECK_FATAL_FALSE("expect coma here  ");
  }
  fdLexer.NextToken();
  if (fdLexer.GetTokenKind() != TK_invalid) {
    CHECK_FATAL_FALSE("expect string after coma  ");
  }
  const std::string &lexerKey = fdLexer.GetName();
  if (lexerKey != keyWord) {
    CHECK_FATAL_FALSE("expect %s after coma  ", keyWord.c_str());
  }
}

static inline bool VerifyModuleHash(uint64 pgoId, const std::string &moduleName) {
  uint32 curHash = DJBHash(LiteProfile::FlatenName(moduleName).c_str());
  if (pgoId > UINT32_MAX) {
    LogInfo::MapleLogger() << "module ID overflow plz check lite-pgo-gen " << '\n';
    return false;
  }
  return (static_cast<uint32>(pgoId) == curHash);
}

/*
 * litepgo function profile format
 * func &<funcname>;funcid <id>;counterSz <num>;cfghash <hashname>;
 * counters
 */
void LiteProfile::ParseFuncProfile(MIRLexer &fdLexer, const std::string &moduleName) {
  /* parse func name */
  fdLexer.NextToken();
  if (fdLexer.GetTokenKind() != TK_fname) {
    CHECK_FATAL_FALSE("expect function name for func");
  }
  const std::string funcName = fdLexer.GetName();

  /* parse funcid */
  fdLexer.NextToken();
  if (fdLexer.GetTokenKind() != TK_funcid) {
    CHECK_FATAL_FALSE("expect funcid here");
  }
  fdLexer.NextToken();
  if (fdLexer.GetTokenKind() != TK_intconst) {
    CHECK_FATAL_FALSE("expect integer after funcid  ");
  }
  uint64 identity = fdLexer.GetTheIntVal();
  if (!VerifyModuleHash(identity, moduleName)) {
    if (debugPrint) {
      LogInfo::MapleLogger() << "LITEPGO log : func " <<
                             funcName << " --- module hash mismatch (skipping)" << '\n';
    }
    return;
  }

  /* parse counterSz */
  ParseLitePgoKeyWord(fdLexer, "counterSz");
  fdLexer.NextToken();
  if (fdLexer.GetTokenKind() != TK_intconst) {
    CHECK_FATAL_FALSE("expect integer after counterSz  ");
  }
  uint64 countersize = fdLexer.GetTheIntVal();

  /* parse cfghash */
  ParseLitePgoKeyWord(fdLexer, "cfghash");
  fdLexer.NextToken();
  if (fdLexer.GetTokenKind() != TK_intconst) {
    CHECK_FATAL_FALSE("expect integer after counterSz  ");
  }
  uint64 cfghash = fdLexer.GetTheIntVal();
  if (cfghash > UINT32_MAX) {
    CHECK_FATAL_FALSE("unexpect cfg hash data type");
  }

  /* parse counters*/
  fdLexer.NextToken();
  if (fdLexer.GetTokenKind() != TK_coma) {
    CHECK_FATAL_FALSE("expect coma here  ");
  }
  fdLexer.NextToken();
  if (countersize) {
    ParseCounters(fdLexer, funcName, static_cast<uint32>(cfghash));
  } else {
    LogInfo::MapleLogger() << "LITEPGO log : func " << funcName << " ---  no counters?" << '\n';
  }
}

void LiteProfile::ParseCounters(MIRLexer &fdLexer, const std::string &funcName, uint32 cfghash) {
  if (fdLexer.GetTokenKind() == TK_func) {
    if (debugPrint) {
      LogInfo::MapleLogger() << "LITEPGO log : func " << funcName << " ---  extremely cold" << '\n';
    }
    extremelyColdFuncs.emplace(funcName);
  }
  while (fdLexer.GetTokenKind() == TK_intconst) {
    uint64 counterVal = fdLexer.GetTheIntVal();
    if (funcBBProfData.find(funcName) != funcBBProfData.end()) {
      funcBBProfData.find(funcName)->second.counter.emplace_back(counterVal);
    } else {
      std::vector<uint32> temp;
      temp.emplace_back(counterVal);
      BBInfo bbInfo(cfghash, std::move(temp));
      funcBBProfData.emplace(funcName, bbInfo);
    }
    fdLexer.NextToken();
  }
}

bool LiteProfile::HandleLitePgoWhiteList(const std::string &fileName) const {
  /* init a lexer for parsing lite-pgo white list */
  MemPool *whitelistMp = memPoolCtrler.NewMemPool("LitePgoWhiteList Mempool", true);
  MapleAllocator whitelistMa(whitelistMp);
  MIRLexer whiteListLexer(nullptr, whitelistMa);
  whiteListLexer.PrepareForFile(fileName);
  whiteListLexer.NextToken();
  bool atEof = false;
  while (!atEof) {
    TokenKind currentTokenKind = whiteListLexer.GetTokenKind();
    if (currentTokenKind == TK_eof) {
      atEof = true;
      continue;
    }
    if (currentTokenKind == TK_invalid) {
      whiteList.emplace(whiteListLexer.GetName());
      whiteListLexer.NextToken();
    } else {
      CHECK_FATAL(false, "unexpected format in instrumentation white list");
      delete whitelistMp;
      return false;
    }
  }
  delete whitelistMp;
  return true;
}

std::set<std::string> LiteProfile::whiteList = {};

uint32 LiteProfile::bbNoThreshold = 100000;
}


