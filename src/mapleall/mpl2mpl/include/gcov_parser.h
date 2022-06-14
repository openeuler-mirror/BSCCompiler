/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_MPL2MPL_INCLUDE_GCOVPROFUSE_H
#define MAPLE_MPL2MPL_INCLUDE_GCOVPROFUSE_H
#include "mempool.h"
#include "mempool_allocator.h"
#include "bb.h"
#include "maple_phase_manager.h"
#include "gcov_profile.h"

namespace maple {

#define CHAR_BIT __CHAR_BIT__
typedef unsigned gcov_unsigned_t;
typedef int64_t gcov_type;
typedef uint64_t gcov_type_unsigned;
typedef unsigned  location_t;
typedef unsigned gcov_unsigned_t;
typedef unsigned gcov_position_t;
// counter defined in gcov-counter.def
enum {
  GCOV_COUNTER_ARCS,
  GCOV_COUNTER_V_INTERVAL,
  GCOV_COUNTER_V_POW2,
  GCOV_COUNTER_V_SINGLE,
  GCOV_COUNTER_V_INDIR,
  GCOV_COUNTER_AVERAGE,
  GCOV_COUNTER_IOR,
  GCOV_TIME_PROFILER,
  GCOV_COUNTER_ICALL_TOPNV,
  GCOV_COUNTERS
};

class GcovVar {
public:
  GcovVar() {
    file = nullptr;
    buffer = nullptr;
  }
  FILE *file;
  gcov_position_t start;
  unsigned offset;
  unsigned length;
  unsigned overread;
  int error;
  int mode;
  int endian;
  size_t alloc;
  gcov_unsigned_t* buffer;
};

class MGcovParser : public AnalysisResult {
 public:
  MGcovParser(MIRModule &mirmod, MemPool *memPool, bool debug) : AnalysisResult(memPool), m(mirmod),
      alloc(memPool), localMP(memPool), gcovData(nullptr), dumpDetail(debug) {
    gcovVar = localMP->New<GcovVar>();
  }
  virtual ~MGcovParser() = default;
  int ReadGcdaFile();
  void DumpFuncInfo();
  GcovProfileData *GetGcovData() { return gcovData; }

 private:
  typedef struct {
    gcov_unsigned_t num_counters;
    gcov_type min_value;
    gcov_type cum_value;
  } gcov_bucket_type;

  struct gcov_ctr_summary {
    gcov_unsigned_t num;
    gcov_unsigned_t runs;
    gcov_type sum_all;
    gcov_type run_max;
    gcov_type sum_max;
    gcov_bucket_type histogram[252];
  };
  struct gcov_summary {
    gcov_unsigned_t checksum;
    struct gcov_ctr_summary ctrs[(GCOV_COUNTER_ARCS + 1)];
  };

  int GcovReadMagic(gcov_unsigned_t magic, gcov_unsigned_t expected);
  int GcovOpenFile(const char *name, int mode);
  gcov_unsigned_t from_file(gcov_unsigned_t value);
  const gcov_unsigned_t * GcovReadWords(unsigned words);
  void GcovSync(gcov_position_t base, gcov_unsigned_t length);
  int GcovCloseFile(void);
  void GcovAllocate(unsigned length);
  gcov_unsigned_t GcovReadUnsigned(void);
  gcov_position_t GcovGetPosition (void);
  gcov_type GcovReadCounter(void);
  void GcovReadSummary(struct gcov_summary *summary);

  MIRModule &m;
  MapleAllocator alloc;
  MemPool *localMP;
  GcovProfileData *gcovData;
  GcovVar *gcovVar;
  bool dumpDetail;
};

MAPLE_MODULE_PHASE_DECLARE(M2MGcovParser)

} // end of namespace maple

#endif  // MAPLE_MPL2MPL_INCLUDE_GCOVPROFUSE_H
