#ifndef MAPLE_UTIL_INCLUDE_GCOV_PROFILE_H
#define MAPLE_UTIL_INCLUDE_GCOV_PROFILE_H

#include "mempool_allocator.h"
#include <string>
#include <unordered_map>

namespace maple {
typedef unsigned gcov_unsigned_t;
typedef int64_t  gcov_type;
typedef uint64_t gcov_type_unsigned;
typedef unsigned  location_t;

class GcovFuncInfo {
public:
  GcovFuncInfo(MapleAllocator* alloc, unsigned funcIdent, unsigned lineno_cs, unsigned cfg_cs) :
      ident(funcIdent), lineno_checksum(lineno_cs), cfg_checksum(cfg_cs), counts(alloc->Adapter()) {};
  int64_t GetFuncFrequency() const { return entry_freq; }
  void SetStmtFreq(uint32_t stmtID, int64_t freq) {
    stmtFreqs[stmtID] = freq;
  }
  int64_t GetStmtFreq(uint32_t stmtID) {
    if (stmtFreqs.count(stmtID) > 0) {
      return stmtFreqs[stmtID];
    }
    return 0;
  }

  /* Name of function.  */
  char *name;
  unsigned ident;
  unsigned lineno_checksum;
  unsigned cfg_checksum;

  /* Array of basic blocks.  Like in GCC, the entry block is
     at blocks[0] and the exit block is at blocks[1].  */
#if 0
  block_t *blocks;
  unsigned num_blocks;
  unsigned blocks_executed;
#endif
  /* Raw arc coverage counts.  */
  unsigned num_counts;
  MapleVector<gcov_type> counts;
  int64_t entry_freq; // record entry bb frequence
  std::unordered_map<uint32_t, uint64_t> stmtFreqs; // stmt_id is key, counter value
};

class GcovProfileData {
public:
  GcovProfileData(MapleAllocator *alloc) : funcsCounter(alloc->Adapter()) {}

  GcovFuncInfo *GetFuncProfile(unsigned puidx) {
    if (funcsCounter.count(puidx) > 0) {
        return funcsCounter[puidx];
    }
    return nullptr;
  }

  MapleUnorderedMap<unsigned, GcovFuncInfo*> funcsCounter; // use puidx as key
};

} // end namespace
#endif // MAPLE_UTIL_INCLUDE_GCOV_PROFILE_H
