#ifndef MAPLE_MPL2MPL_INCLUDE_GCOVPROFUSE_H
#define MAPLE_MPL2MPL_INCLUDE_GCOVPROFUSE_H

#include "mempool.h"
#include "mempool_allocator.h"
//#include "me_pgo_instrument.h"
#include "bb.h"
#include "maple_phase_manager.h"
#include "gcov_profile.h"

namespace maple {

class MGcovParser : public AnalysisResult {
 public:
  MGcovParser(MIRModule &mirmod, MemPool *memPool, bool debug) : AnalysisResult(memPool), m(mirmod),
      alloc(memPool), localMP(memPool), gcovData(nullptr), dumpDetail(debug) {}
  virtual ~MGcovParser() = default;
  int read_count_file();
  void DumpFuncInfo();

  GcovProfileData *GetGcovData() { return gcovData; }

 private:
  MIRModule &m;
  MapleAllocator alloc;
  MemPool *localMP;
  GcovProfileData *gcovData;
  bool dumpDetail;
};

MAPLE_MODULE_PHASE_DECLARE(M2MGcovParser)

} // end of namespace maple

#endif  // MAPLE_MPL2MPL_INCLUDE_GCOVPROFUSE_H
