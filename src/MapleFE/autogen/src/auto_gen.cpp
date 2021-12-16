/*
* Copyright (C) [2020] Futurewei Technologies, Inc. All rights reverved.
*
* OpenArkFE is licensed under the Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*
*  http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
* FIT FOR A PARTICULAR PURPOSE.
* See the Mulan PSL v2 for more details.
*/
#include <vector>
#include <cstring>

#include "reserved_gen.h"
#include "spec_parser.h"
#include "auto_gen.h"
#include "base_gen.h"
#include "massert.h"
#include "token_table.h"

namespace maplefe {

///////////////////////////////////////////////////////////////////////////////////////
//                       Summary functions during parsing
// During matching phase in the language parser, we often need to print the stack of
// rule table traversing in order to easily find the bug cause. So we need a set of
// functions to help dump the information. As an example, a major request is to dump the
// table name when we step into a new rule table. This request a mapping from the ruletable
// address to the table name.
//
// We generate two files gen_summary.h and gen_summary.cpp to include all the functions we
// may need. These two files are generated by autogen. Don't modify them.
//
// 1. For the rule table name mapping
//
//    In header file, we need
//
//      typedef struct {
//        const RuleTable *mAddr;
//        const char      *mName;
//      }RuleTableSummary;
//      extern RuleTableSummary gRuleTableSummarys[];
//      extern unsigned RuleTableNum;
//      extern std::vector<unsigned> gFailed[621];
//
//    In Cpp file, we need
//
//    #include "gen_summary.h"
//    unsigned RuleTableNum;
//    RuleTableSummary gRuleTableSummarys[] = {
//      {&TblLiteral, "TblLiteral"},
//      ...
//    };
///////////////////////////////////////////////////////////////////////////////////////

FileWriter *gSummaryHFile;
FileWriter *gSummaryCppFile;
unsigned    gRuleTableNum;
std::vector<std::string> gTopRules;

// write the beginning part of summary file
static void PrepareSummaryCppFile() {
  gSummaryCppFile->WriteOneLine("#include \"rule_summary.h\"", 25);
  gSummaryCppFile->WriteOneLine("#include \"common_header_autogen.h\"", 34);
  gSummaryCppFile->WriteOneLine("namespace maplefe {", 19);
  gSummaryCppFile->WriteOneLine("RuleTableSummary gRuleTableSummarys[] = {", 41);
}

// write the ending part of summary file
static void FinishSummaryCppFile() {
  gSummaryCppFile->WriteOneLine("};", 2);
  std::string num = std::to_string(gRuleTableNum);
  std::string num_line = "unsigned RuleTableNum = ";
  num_line += num;
  num_line += ";";
  gSummaryCppFile->WriteOneLine(num_line.c_str(), num_line.size());
  gSummaryCppFile->WriteOneLine("const char* GetRuleTableName(const RuleTable* addr) {", 53);
  gSummaryCppFile->WriteOneLine("  for (unsigned i = 0; i < RuleTableNum; i++) {", 47);
  gSummaryCppFile->WriteOneLine("    RuleTableSummary summary = gRuleTableSummarys[i];", 53);
  gSummaryCppFile->WriteOneLine("    if (summary.mAddr == addr)", 30);
  gSummaryCppFile->WriteOneLine("      return summary.mName;", 27);
  gSummaryCppFile->WriteOneLine("  }", 3);
  gSummaryCppFile->WriteOneLine("  return NULL;", 14);
  gSummaryCppFile->WriteOneLine("}", 1);

  std::string s = "BitVector gFailed[";
  s += std::to_string(gRuleTableNum);
  s += "];";
  gSummaryCppFile->WriteOneLine(s.c_str(), s.size());

  s = "SuccMatch gSucc[";
  s += std::to_string(gRuleTableNum);
  s += "];";
  gSummaryCppFile->WriteOneLine(s.c_str(), s.size());

  s = "unsigned gTopRulesNum = ";
  s += std::to_string(gTopRules.size());
  s += ";";
  gSummaryCppFile->WriteOneLine(s.c_str(), s.size());

  s = "RuleTable* gTopRules[";
  s += std::to_string(gTopRules.size());
  s += "] = {";
  unsigned i = 0;
  for (; i < gTopRules.size(); i++) {
    s += "&";
    s += gTopRules[i];
    if (i < gTopRules.size() - 1)
      s += ",";
  }
  s += "};";
  gSummaryCppFile->WriteOneLine(s.c_str(), s.size());
  gSummaryCppFile->WriteOneLine("}", 1);
}

///////////////////////////////////////////////////////////////////////////////////////
//
//                The major implementation  of Autogen
//
///////////////////////////////////////////////////////////////////////////////////////

void AutoGen::Init() {
  std::string lang_path("../gen/");

  std::string summary_file_name = lang_path + "gen_summary.cpp";
  gSummaryCppFile = new FileWriter(summary_file_name);
  summary_file_name = lang_path + "gen_summary.h";
  gSummaryHFile = new FileWriter(summary_file_name);
  gRuleTableNum = 0;

  PrepareSummaryCppFile();

  std::string hFile = lang_path + "gen_reserved.h";
  std::string cppFile = lang_path + "gen_reserved.cpp";
  mReservedGen = new ReservedGen("../../../autogen/reserved.spec", hFile.c_str(), cppFile.c_str());
  mReservedGen->SetReserved(mReservedGen);
  mGenArray.push_back(mReservedGen);

  hFile = lang_path + "gen_iden.h";
  cppFile = lang_path + "gen_iden.cpp";
  std::string specFile = "../../../";
  specFile += mLang;
  specFile += "/identifier.spec";
  mIdenGen  = new IdenGen(specFile.c_str(), hFile.c_str(), cppFile.c_str());
  mIdenGen->SetReserved(mReservedGen);
  mGenArray.push_back(mIdenGen);

  hFile = lang_path + "gen_literal.h";
  cppFile = lang_path + "gen_literal.cpp";
  specFile = "../../../";
  specFile += mLang;
  specFile += "/literal.spec";
  mLitGen  = new LiteralGen(specFile.c_str(), hFile.c_str(), cppFile.c_str());
  mLitGen->SetReserved(mReservedGen);
  mGenArray.push_back(mLitGen);

  hFile = lang_path + "gen_type.h";
  cppFile = lang_path + "gen_type.cpp";
  specFile = "../../../";
  specFile += mLang;
  specFile += "/type.spec";
  mTypeGen  = new TypeGen(specFile.c_str(), hFile.c_str(), cppFile.c_str());
  mTypeGen->SetReserved(mReservedGen);
  mGenArray.push_back(mTypeGen);

  hFile = lang_path + "gen_attr.h";
  cppFile = lang_path + "gen_attr.cpp";
  specFile = "../../../";
  specFile += mLang;
  specFile += "/attr.spec";
  mAttrGen  = new AttrGen(specFile.c_str(), hFile.c_str(), cppFile.c_str());
  mAttrGen->SetReserved(mReservedGen);
  mGenArray.push_back(mAttrGen);

  hFile = lang_path + "gen_separator.h";
  cppFile = lang_path + "gen_separator.cpp";
  specFile = "../../../";
  specFile += mLang;
  specFile += "/separator.spec";
  mSeparatorGen  = new SeparatorGen(specFile.c_str(), hFile.c_str(), cppFile.c_str());
  mSeparatorGen->SetReserved(mReservedGen);
  mGenArray.push_back(mSeparatorGen);

  hFile = lang_path + "gen_operator.h";
  cppFile = lang_path + "gen_operator.cpp";
  specFile = "../../../";
  specFile += mLang;
  specFile += "/operator.spec";
  mOperatorGen  = new OperatorGen(specFile.c_str(), hFile.c_str(), cppFile.c_str());
  mOperatorGen->SetReserved(mReservedGen);
  mGenArray.push_back(mOperatorGen);

  hFile = lang_path + "gen_keyword.h";
  cppFile = lang_path + "gen_keyword.cpp";
  specFile = "../../../";
  specFile += mLang;
  specFile += "/keyword.spec";
  mKeywordGen  = new KeywordGen(specFile.c_str(), hFile.c_str(), cppFile.c_str());
  mKeywordGen->SetReserved(mReservedGen);
  mGenArray.push_back(mKeywordGen);

  hFile = lang_path + "gen_stmt.h";
  cppFile = lang_path + "gen_stmt.cpp";
  specFile = "../../../";
  specFile += mLang;
  specFile += "/stmt.spec";
  mStmtGen  = new StmtGen(specFile.c_str(), hFile.c_str(), cppFile.c_str());
  mStmtGen->SetReserved(mReservedGen);
  mGenArray.push_back(mStmtGen);

  hFile = lang_path + "gen_token.h";
  cppFile = lang_path + "gen_token.cpp";
  mTokenGen  = new TokenGen(hFile.c_str(), cppFile.c_str());
  mTokenGen->SetReserved(mReservedGen);
  mGenArray.push_back(mTokenGen);
}

AutoGen::~AutoGen() {
  std::vector<BaseGen*>::iterator it = mGenArray.begin();
  for (; it != mGenArray.end(); it++){
    BaseGen *gen = *it;
    delete gen;
  }

  if (gSummaryHFile)
    delete gSummaryHFile;
  if (gSummaryCppFile)
    delete gSummaryCppFile;
}

// When parsing a rule, its elements could be rules in the future rules, or
// it could be in another XxxGen. We simply put Pending for these elements.
//
// BackPatch() is the one coming back to solve these Pending. It has to do
// a traversal to solve one by one.
void AutoGen::BackPatch() {
  std::vector<BaseGen*>::iterator it = mGenArray.begin();
  for (; it != mGenArray.end(); it++){
    BaseGen *gen = *it;
    gen->BackPatch(mGenArray);
  }
}

void AutoGen::Run() {
  std::vector<BaseGen*>::iterator it = mGenArray.begin();
  for (; it != mGenArray.end(); it++){
    BaseGen *gen = *it;
    gen->Run(mParser);
  }
}

void AutoGen::Gen() {
  Init();
  Run();
  BackPatch();

  // Prepare the tokens in all rules, by replacing all keyword, operator,
  // separators with tokens. All rules will contain index to the
  // system token.
  gTokenTable.mOperators = &(mOperatorGen->mOperators);
  gTokenTable.mSeparators = &(mSeparatorGen->mSeparators);
  gTokenTable.mKeywords = mKeywordGen->mKeywords;
  gTokenTable.Prepare();

  std::vector<BaseGen*>::iterator it = mGenArray.begin();
  for (; it != mGenArray.end(); it++){
    BaseGen *gen = *it;
    gen->Generate();
  }

  FinishSummaryCppFile();
}

}