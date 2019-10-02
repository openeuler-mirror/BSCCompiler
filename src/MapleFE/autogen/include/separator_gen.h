////////////////////////////////////////////////////////////////////////
//                      Separator Generation
// The output of this Separator Generation is a table in gen_separator.cpp 
//
//   SepTableEntry SepTable[SEP_Null] = {
//     {"xxx", SEP_Xxx},
//     ...
//   };
//
//   //Returns SEP_Null if not a separator.
//   SepID GetSeparator(Lexer *, int &len) {...}
//
//   //Return the SepID. It is called only after GetSeparator() returns a valid
//   //SepID. We use 'len' tell if there is an GetSeparator() just did a scanning.
//   //If len > 1, we just need simply return the SepID and move the cursor.
//   SepID GetSeparator(Lexer *, int len) { ... }
//
////////////////////////////////////////////////////////////////////////

#ifndef __SEPARATOR_GEN_H__
#define __SEPARATOR_GEN_H__

#include <list>

#include "base_struct.h"
#include "base_gen.h"

// For each separator, it has three parts involved in the generation.
//   1. SepId:   Used inside autogen, connection between LANGUAGE and SUPPORTED
//               .spec files
//   2. Name:    Name of SEP_ID, to be generated in gen_separator.cpp 
//   4. Keyword: LANGUAGE syntax text, to be in gen_separator.cpp

// The separators supported in autogen, super set of all languages.
#undef  SEPARATOR
#define SEPARATOR(T) SEP_##T,
enum SepId {
#include "supported_separators.def"
SEP_NA
};

// The SUPPORTED separator and their name.
// The 'name' is used as known word in separator.spec
struct SuppSepId {
  std::string mName;   // To be output in gen_separator.cpp
  SepId       mSepId;  // ID linked to LANGUAGE Separator defined below.
};

// The LANGUAGE separator
struct Separator {
  const char *mKeyword;// Syntax text in LANGUAGE, output in gen_separator.cpp
  SepId       mID;     // ID linked to SUPPORTED SuppSepId defined above.
};

extern SepId       FindSeparatorId(const std::string &s);
extern std::string FindSeparatorName(SepId id);

// The class of SeparatorGen
class SeparatorGen : public BaseGen {
public:
  std::list<Separator> mSeparators;
public:
  SeparatorGen(const char *dfile, const char *hfile, const char *cfile)
      : BaseGen(dfile, hfile, cfile) {}
  ~SeparatorGen(){}

  bool Empty();

  // Generate the output files: gen_separator.h/.cpp
  void ProcessStructData();
  void Generate();
  void GenCppFile();
  void GenHeaderFile();

  void AddEntry(const char *s, SepId i) { mSeparators.push_back({s, i}); }

  // Functions to dump Enum.
  std::list<Separator>::iterator mEnumIter;
  const std::string EnumName(){return "";}
  const std::string EnumNextElem();
  const void EnumBegin();
  const bool EnumEnd();
};

#endif
