/////////////////////////////////////////////////////////////////////
// This is the base functions of auto generation.                  //
// Each individual part is supposed to inherit from this class.    //
/////////////////////////////////////////////////////////////////////

#ifndef __SPEC_PARSER_H__
#define __SPEC_PARSER_H__

#include <iostream>
#include <fstream>
#include <stack>

#include "rule.h"
#include "base_struct.h"
#include "spec_lexer.h"

#include "stringpool.h"

class MemPool;
class RuleElemPool;
class ReservedGen;
class BaseGen;

class SPECParser {
public:
  SPECLexer     mLexer;
  BaseGen      *mBaseGen;

public:
  SPECParser() : mLexer() {}
  SPECParser(const std::string &dfile) : mLexer() { ResetParser(dfile); }
  ~SPECParser() {};

  // for all ParseXXX routines
  // Return true  : succeed
  //        false : failed
  bool Parse();
  void ResetParser(const std::string &dfile);

  bool ParseRule();
  bool ParseAction(RuleElem *&elem);
  bool ParseElement(RuleElem *&elem, bool allowConcat);
  bool ParseElementSet(RuleElem *parent);
  bool ParseConcatenate(RuleElem *parent);

  bool ParseStruct();
  bool ParseStructElements();
  bool ParseElemData(StructElem *elem);
  
  void SetVerbose(int i) { mLexer.SetVerbose(i); }
  int GetVerbose() { return mLexer.GetVerbose(); }

  void DumpRules();
  void DumpStruct();
  void Dump() { DumpStruct(); DumpRules(); }
};

#endif
