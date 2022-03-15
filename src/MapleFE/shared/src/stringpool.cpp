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
//                                                                          //
// This file contains the implementation of string pool.                    //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#include <cstdlib>
#include <cstring>
#include <stdlib.h>

#include "stringpool.h"
#include "stringmap.h"

namespace maplefe {

// The global string pool for lexing, parsing, ast building and ir building
// for the symbols, etc.
StringPool gStringPool;

StringPool::StringPool() {
  mUseAltStr = false;
  mMap = new StringMap();
  mMap->SetPool(this);
  mFirstAvail = -1;
  // make string idx starting from 1
  mStringTable.push_back("");
  // empty string idx is 1
  mStringTable.push_back("");
}

StringPool::~StringPool() {
  // Release the mBlocks
  std::vector<SPBlock>::iterator it;
  for (it = mBlocks.begin(); it != mBlocks.end(); it++) {
    SPBlock block = *it;
    char *addr = block.Addr;
    free(addr);
  }
  mStringTable.clear();

  // Release the long strings
  std::vector<char*>::iterator long_it;
  for (long_it = mLongStrings.begin(); long_it != mLongStrings.end(); long_it++) {
    char *addr = *long_it;
    free(addr);
  }
  mLongStrings.clear();

  // Release the StringMap
  delete mMap;
}

char* StringPool::Alloc(const std::string &s) {
  return Alloc(s.c_str());
}

// 's' must guarantee to end with NULL
char* StringPool::Alloc(const char *s) {
  size_t size = strlen(s) + 1;
  char *addr = NULL;
  if (size > BLOCK_SIZE) {
    addr = (char*)malloc(size);
    mLongStrings.push_back(addr);
  } else {
    addr = Alloc(size);
  }

  MASSERT (addr && "StringPool failed to alloc for string");

  strncpy(addr, s, size - 1);
  *(addr + size -1) = '\0';
  return addr;
}

// The searching for available block is going forward in the vector of 'mBlocks',
// and if one block is not big enough to hold 'size', it will be skipped and
// we search the next block.
//
// the 'firstavil' is always incremental, so it means even if a block before
// 'mFirstAvail' contains enough space for 'size', we still won't allocate
// from that block. So, Yes, we are wasting space.
//
// TODO: will come back to remove this wasting.
//
char* StringPool::Alloc(const size_t size) {
  char *addr = NULL;

  if (mFirstAvail == -1) {
    addr = AllocBlock();
    MASSERT (addr && "StringPool failed to alloc a block");
    mFirstAvail = 0;
  }

  int avail = BLOCK_SIZE - mBlocks[mFirstAvail].Used;

  if (avail < size) {
    mFirstAvail++;
    if (mFirstAvail >= mBlocks.size()) {
      addr = AllocBlock();
      MASSERT (addr && "StringPool failed to alloc a block");
    }
  }

  // At this point, it's guaranteed that 'b' is able to hold 'size'
  SPBlock *b = &mBlocks[mFirstAvail];
  addr = b->Addr + b->Used;
  b->Used += size;

  return addr;
}

char* StringPool::AllocBlock() {
  char *addr = (char*)malloc(BLOCK_SIZE);
  SPBlock block = {addr, 0};
  mBlocks.push_back(block);
  return addr;
}

// This is the public interface to find a string in the pool.
// If not found, allocate in the pool and save in the map.
const char* StringPool::FindString(const std::string &s) {
  return mMap->LookupEntryFor(s)->GetAddr();
}

// This is the public interface to find a string in the pool.
// If not found, allocate in the pool and save in the map.
const char* StringPool::FindString(const char *str) {
  std::string s(str);
  return mMap->LookupEntryFor(s)->GetAddr();
}

// This is the public interface to find a string in the pool.
// If not found, allocate in the pool and save in the map.
const char* StringPool::FindString(const char *str, size_t len) {
  std::string s;
  s.assign(str, len);
  return mMap->LookupEntryFor(s)->GetAddr();
}

// This is the public interface to find a string in the pool.
// If not found, allocate in the pool and save in the map.
unsigned StringPool::GetStrIdx(const std::string &s) {
  if (s.empty()) return 1;
  return mMap->LookupEntryFor(s)->GetStrIdx();
}

// This is the public interface to find a string in the pool.
// If not found, allocate in the pool and save in the map.
unsigned StringPool::GetStrIdx(const char *str) {
  if (strlen(str) == 0) return 1;
  std::string s(str);
  return mMap->LookupEntryFor(s)->GetStrIdx();
}

// This is the public interface to find a string in the pool.
// If not found, allocate in the pool and save in the map.
unsigned StringPool::GetStrIdx(const char *str, size_t len) {
  if (len == 0) return 1;
  std::string s;
  s.assign(str, len);
  return mMap->LookupEntryFor(s)->GetStrIdx();
}

const char *StringPool::GetStringFromStrIdx(unsigned idx) {
  MASSERT(idx < mStringTable.size() && "string index out of range");
  if (mUseAltStr) {
    if (mAltStrIdxMap.find(idx) != mAltStrIdxMap.end()) {
      idx = mAltStrIdxMap[idx];
    }
  }
  return mStringTable[idx];
}

// This is the public interface to setup AltStrIdxMap used for obfuscation
// a name is mapped to a fixed length random unused name.
// starting from 2-letter names, [a-zA-Z] [a-zA-Z], which will cover over 2K names
// AA Aa AB Ab, ...., zz
// if not enough will extend to use 3-letter or 4-letter for over 7 million names
void StringPool::SetAltStrIdxMap() {
  // starting from 2-letter names
  unsigned len = 2;
  bool done = false;

  // names use [A-Z] and [a-z] total 52 letters
  int k = 52;

  // total number of names can be handled for len = 4, 3, 2, 1 respectively
  int Size[4] = {k*k*k*k, k*k*k, k*k, k};

  // names, trailing '\0'
  char A[5] = {0, 0, 0, 0, 0};

  // names already encounted, either existing name or new names
  std::unordered_set<unsigned> used;

  for (auto stridx : mAltStrIdxSet) {
    done = false;
    while (!done) {
      unsigned offset = 4 - len;
      int mod = Size[offset];

      int n = rand();
      int r = n % mod;

      // check if already encounted
      if (used.find(r) != used.end()) {
        // expand to use one more leter if close to limit
        if (used.size() > mod - Size[offset + 1]) {
          len++;
          MASSERT(len < 5 && "Need more names");
        }
        continue;
      }

      // have un-encounted name
      used.insert(r);

      int q;
      bool odd;
      int i = 0;
      while (i < len - 1) {
        mod = Size[offset + 1 + i];
        q = r / mod;
        r = r % mod;

        // char, use upper case for odd number
        odd = q%2;
        A[i++] = (odd ? 'A' : 'a') + q/2;
      }

      // last char, use upper case for odd number
      odd = r%2;
      A[i] = (odd ? 'A' : 'a') + r/2;

      unsigned size = GetSize();
      unsigned alt = GetStrIdx(A);
      // make sure alt is a new string
      if (alt == size) {
        mAltStrIdxMap[stridx] = alt;
        done = true;
      }
    }
  }
}

void StringPool::Dump() {
  std::cout << "===================== StringTable =====================" << std::endl;
  for (unsigned idx = 1; idx < mStringTable.size(); idx++) {
    std::cout << "  " << idx << " : " << mStringTable[idx] << std::endl;
  }
}

void StringPool::DumpAlt() {
  std::cout << "================= Alt String Map ======================" << std::endl;
  unsigned count = 0;
  for (auto stridx : mAltStrIdxSet) {
    unsigned alt = mAltStrIdxMap[stridx];
    std::cout << "count #" << stridx
              << " str " << GetStringFromStrIdx(stridx)
              << " --> "
              << " alt " << GetStringFromStrIdx(alt)
              << std::endl;
  }
}

}

