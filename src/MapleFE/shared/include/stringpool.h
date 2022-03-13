/*
* Copyright (C) [2020-2022] Futurewei Technologies, Inc. All rights reverved.
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
//////////////////////////////////////////////////////////////////////////////
// This file contains the Memory Pool for String pool.                      //
//                                                                          //
// A String is stored in the pool with an ending '\0'.                      //
//////////////////////////////////////////////////////////////////////////////

#ifndef __STRINGPOOL_H__
#define __STRINGPOOL_H__

#include <map>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include "massert.h"

namespace maplefe {

//  Each time when extra memory is needed, a fixed size BLOCK will be allocated.
//  It's defined by BLOCK_SIZE. Anything above this size will not
//  be supported.
#define BLOCK_SIZE 4096

class StringMap;

struct SPBlock {
  char *Addr;        // starting address
  unsigned int Used; // bytes used
};

class StringPool {
private:
  StringMap            *mMap;
  std::vector<SPBlock>  mBlocks;
  int                   mFirstAvail; // -1 means no available.
  bool                  mUseAltStr;  // use alter string

  std::vector<char*> mLongStrings; // for strings longer than block size,
                                   // we allocate them by malloc.

  std::vector<const char*> mStringTable;

  // alternate string which can be used for obfuscation
  std::unordered_set<unsigned> mAltStrIdxSet;
  std::unordered_map<unsigned, unsigned> mAltStrIdxMap;

  friend class StringMap;

public:
  StringPool();
  ~StringPool();

  void SetUseAltStr(bool b) { mUseAltStr = b; }
  void AddAltStrIdx(unsigned idx) { mAltStrIdxSet.insert(idx); }
  unsigned GetAltStrSize() { return mAltStrIdxSet.size(); }
  bool IsAltStrIdx(unsigned idx) {
    return mAltStrIdxSet.find(idx) != mAltStrIdxSet.end();
  }
  void AddAltStrIdxMap(unsigned orig, unsigned alt) { mAltStrIdxMap[orig] = alt; }
  void SetAltStrIdxMap();

  char* AllocBlock();
  char* Alloc(const size_t);
  char* Alloc(const std::string&);
  char* Alloc(const char*);

  // If not found, add into StringPool
  const char* FindString(const std::string&);
  const char* FindString(const char*);
  const char* FindString(const char*, size_t);

  unsigned GetStrIdx(const std::string&);
  unsigned GetStrIdx(const char*);
  unsigned GetStrIdx(const char*, size_t);

  unsigned GetSize() {return mStringTable.size();}

  const char *GetStringFromStrIdx(unsigned idx);

  void Dump();
  void DumpAlt();
};

// Lexing, Parsing, AST Building and IR Building all share one global
// StringPool for their symbols or necessary strings.
extern StringPool gStringPool;

}
#endif  // __STRINGPOOL_H__
