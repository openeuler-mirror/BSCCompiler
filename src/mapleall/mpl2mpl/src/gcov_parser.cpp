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

#include <stdio.h>
#include <getopt.h>
#include <stdint.h>
#include <assert.h>
#include <inttypes.h>
#include <cstdlib>
#include <cstring>
#include <stdarg.h>
#include "option.h"
#include "mpl_logging.h"
#include "gcov_parser.h"

namespace maple {
#define GCOV_DATA_MAGIC ((gcov_unsigned_t)0x67636461) /* "gcda" */
#define GCOV_VERSION ((gcov_unsigned_t)0x4137352a)

#ifndef ATTRIBUTE_UNUSED
#define ATTRIBUTE_UNUSED __attribute__((unused))
#endif

/* Convert a magic or version number to a 4 character string.  */
#define GCOV_UNSIGNED2STRING(ARRAY,VALUE)   \
  ((ARRAY)[0] = (char)((VALUE) >> 24),      \
   (ARRAY)[1] = (char)((VALUE) >> 16),      \
   (ARRAY)[2] = (char)((VALUE) >> 8),       \
   (ARRAY)[3] = (char)((VALUE) >> 0))

#define GCOV_TAG_FUNCTION    ((gcov_unsigned_t)0x01000000)
#define GCOV_TAG_FUNCTION_LENGTH (3)
#define GCOV_TAG_BLOCKS      ((gcov_unsigned_t)0x01410000)
#define GCOV_TAG_BLOCKS_LENGTH(NUM) (NUM)
#define GCOV_TAG_BLOCKS_NUM(LENGTH) (LENGTH)
#define GCOV_TAG_ARCS        ((gcov_unsigned_t)0x01430000)
#define GCOV_TAG_ARCS_LENGTH(NUM)  (1 + (NUM) * 2)
#define GCOV_TAG_ARCS_NUM(LENGTH)  (((LENGTH) - 1) / 2)
#define GCOV_TAG_LINES       ((gcov_unsigned_t)0x01450000)
#define GCOV_TAG_COUNTER_BASE    ((gcov_unsigned_t)0x01a10000)
#define GCOV_TAG_COUNTER_LENGTH(NUM) ((NUM) * 2)
#define GCOV_TAG_COUNTER_NUM(LENGTH) ((LENGTH) / 2)
#define GCOV_TAG_OBJECT_SUMMARY  ((gcov_unsigned_t)0xa1000000) /* Obsolete */
#define GCOV_TAG_PROGRAM_SUMMARY ((gcov_unsigned_t)0xa3000000)
#define GCOV_TAG_SUMMARY_LENGTH(NUM)  \
        (1 + GCOV_COUNTERS_SUMMABLE * (10 + 3 * 2) + (NUM) * 5)
#define GCOV_TAG_AFDO_FILE_NAMES ((gcov_unsigned_t)0xaa000000)
#define GCOV_TAG_AFDO_FUNCTION ((gcov_unsigned_t)0xac000000)
#define GCOV_TAG_AFDO_WORKING_SET ((gcov_unsigned_t)0xaf000000)

// Convert a counter index to a tag
#define GCOV_TAG_FOR_COUNTER(COUNT)             \
    (GCOV_TAG_COUNTER_BASE + ((gcov_unsigned_t)(COUNT) << 17))

// Return the number of set bits in X
static int popcount_hwi (unsigned long long x) {
  int i, ret = 0;
  size_t bits = sizeof (x) * CHAR_BIT;
  for (i = 0; i < bits; i += 1) {
    ret += x & 1;
    x >>= 1;
  }
  return ret;
}

gcov_position_t MGcovParser::GcovGetPosition (void) {
  ((void)(0 && (gcovVar->mode > 0)));
  return gcovVar->start + gcovVar->offset;
}

int MGcovParser::GcovOpenFile(const char *name, int mode) {
  ((void)(0 && (!gcovVar->file)));
  gcovVar->start = 0;
  gcovVar->offset = gcovVar->length = 0;
  gcovVar->overread = -1u;
  gcovVar->error = 0;
  gcovVar->endian = 0;
  gcovVar->alloc = 0;
  if (mode >= 0) {
    gcovVar->file = fopen(name, (mode > 0) ? "rb" : "r+b");
  }
  if (gcovVar->file) {
    mode = 1;
  } else if (mode <= 0) {
    gcovVar->file = fopen(name, "w+b");
  }
  if (!gcovVar->file) return 0;
  gcovVar->mode = mode ? mode : 1;
  setbuf(gcovVar->file, (char *)0);
  return 1;
}

void MGcovParser::GcovAllocate (unsigned length) {
  size_t new_size = gcovVar->alloc;
  if (!new_size) {
    new_size = (1 << 10);
  }
  new_size += length;
  new_size *= 2;
  gcovVar->alloc = new_size;
  gcovVar->buffer= ((gcov_unsigned_t *) realloc ((gcovVar->buffer), (new_size << 2)));
}

int MGcovParser::GcovReadMagic(gcov_unsigned_t magic, gcov_unsigned_t expected) {
  if (magic == expected) return 1;
  magic = (magic >> 16) | (magic << 16);
  magic = ((magic & 0xff00ff) << 8) | ((magic >> 8) & 0xff00ff);
  if (magic == expected) {
    gcovVar->endian = 1;
    return -1;
  }
  return 0;
}

void MGcovParser::GcovSync(gcov_position_t base, gcov_unsigned_t length) {
  ((void)(0 && (gcovVar->mode > 0)));
  base += length;
  if (base - gcovVar->start <= gcovVar->length) {
    gcovVar->offset = base - gcovVar->start;
  } else {
    gcovVar->offset = gcovVar->length = 0;
    fseek (gcovVar->file, base << 2, 0);
    gcovVar->start = ftell (gcovVar->file) >> 2;
  }
}

const gcov_unsigned_t * MGcovParser::GcovReadWords (unsigned words) {
  const gcov_unsigned_t *result;
  unsigned excess = gcovVar->length - gcovVar->offset;
  if (gcovVar->mode <= 0)
    return nullptr;
  if (excess < words) {
    gcovVar->start += gcovVar->offset;
    if (excess) {
      memmove_s(gcovVar->buffer, excess * 4, gcovVar->buffer + gcovVar->offset, excess * 4);
    }
    gcovVar->offset = 0;
    gcovVar->length = excess;
    if (gcovVar->length + words > gcovVar->alloc) {
      GcovAllocate(gcovVar->length + words);
    }
    excess = gcovVar->alloc - gcovVar->length;
    excess = fread (gcovVar->buffer + gcovVar->length, 1, excess << 2, gcovVar->file) >> 2;
    gcovVar->length += excess;
    if (gcovVar->length < words) {
      gcovVar->overread += words - gcovVar->length;
      gcovVar->length = 0;
      return 0;
    }
  }
  result = &gcovVar->buffer[gcovVar->offset];
  gcovVar->offset += words;
  return result;
}

gcov_unsigned_t MGcovParser::from_file(gcov_unsigned_t value) {
  if (gcovVar->endian) {
    value = (value >> 16) | (value << 16);
    value = ((value & 0xff00ff) << 8) | ((value >> 8) & 0xff00ff);
  }
  return value;
}

gcov_unsigned_t MGcovParser::GcovReadUnsigned(void) {
  gcov_unsigned_t value;
  const gcov_unsigned_t *buffer = GcovReadWords(1);
  if (!buffer) return 0;
  value = from_file(buffer[0]);
  return value;
}

int MGcovParser::GcovCloseFile (void) {
  if (gcovVar->file) {
    fclose (gcovVar->file);
    gcovVar->file = 0;
    gcovVar->length = 0;
  }
  free (gcovVar->buffer);
  gcovVar->alloc = 0;
  gcovVar->buffer = nullptr;
  gcovVar->mode = 0;
  return gcovVar->error;
}

gcov_type MGcovParser::GcovReadCounter (void) {
  gcov_type value;
  const gcov_unsigned_t *buffer = GcovReadWords(2);
  if (!buffer) return 0;
  value = from_file (buffer[0]);
  if (sizeof (value) > sizeof (gcov_unsigned_t)) {
    value |= ((gcov_type) from_file (buffer[1])) << 32;
  } else if (buffer[1]) {
    gcovVar->error = -1;
  }
  return value;
}

void MGcovParser::GcovReadSummary (struct gcov_summary *summary) {
  unsigned ix, h_ix, bv_ix, h_cnt = 0;
  struct gcov_ctr_summary *csum;
  unsigned histo_bitvector[(252 + 31) / 32];
  unsigned cur_bitvector;
  summary->checksum = GcovReadUnsigned();
  for (csum = summary->ctrs, ix = (GCOV_COUNTER_ARCS + 1); ix--; csum++) {
    csum->num = GcovReadUnsigned();
    csum->runs = GcovReadUnsigned();
    csum->sum_all = GcovReadCounter();
    csum->run_max = GcovReadCounter();
    csum->sum_max = GcovReadCounter();
    memset_s(csum->histogram, sizeof (gcov_bucket_type) * 252, 0, sizeof (gcov_bucket_type) * 252);
    for (bv_ix = 0; bv_ix < (252 + 31) / 32; bv_ix++) {
      histo_bitvector[bv_ix] = GcovReadUnsigned();
      h_cnt += popcount_hwi (histo_bitvector[bv_ix]);
    }
    bv_ix = 0;
    h_ix = 0;
    cur_bitvector = 0;
    while (h_cnt--) {
      while (!cur_bitvector) {
        h_ix = bv_ix * 32;
        if (bv_ix >= (252 + 31) / 32) {
          CHECK_FATAL(false, "corrupted profile info: summary histogram " "bitvector is corrupt");
        }
        cur_bitvector = histo_bitvector[bv_ix++];
      }
      while (!(cur_bitvector & 0x1)) {
        h_ix++;
        cur_bitvector >>= 1;
      }
      if (h_ix >= 252) {
        CHECK_FATAL(0, "corrupted profile info: summary histogram " "index is corrupt");
      }
      csum->histogram[h_ix].num_counters = GcovReadUnsigned();
      csum->histogram[h_ix].min_value = GcovReadCounter();
      csum->histogram[h_ix].cum_value = GcovReadCounter();
      cur_bitvector >>= 1;
      h_ix++;
    }
  }
}

static unsigned object_runs;
static unsigned program_count;
//static unsigned bbg_stamp = 0;
// reference
int MGcovParser::ReadGcdaFile() {
  std::string gcovDataFile = Options::profile;
  if (gcovDataFile.empty()) {
    if (const char* env_p = std::getenv("GCOV_PREFIX")) {
      gcovDataFile.append(env_p);
    } else {
      gcovDataFile.append(".");
    }
    gcovDataFile.append("/");
    gcovDataFile.append(m.GetProfileDataFileName());
  }
  ASSERT(!gcovDataFile.empty(), "null check");
  if (!GcovOpenFile(gcovDataFile.c_str(), 1)) {
    LogInfo::MapleLogger() << "no data file " << gcovDataFile << " \n";
    Options::profileUse = false; // reset profileUse
    return 0;
  }

  if (!GcovReadMagic(GcovReadUnsigned(), GCOV_DATA_MAGIC)) {
    LogInfo::MapleLogger() <<  gcovDataFile << " not a gcov data file\n";
    GcovCloseFile ();
    return 1;
  }
  unsigned version = GcovReadUnsigned();
  if (version != GCOV_VERSION) {
    char v[4], e[4];
    GCOV_UNSIGNED2STRING (v, version);
    GCOV_UNSIGNED2STRING (e, GCOV_VERSION);
    LogInfo::MapleLogger() << gcovDataFile <<  " version " << v << " prefer version " << e << "\n";
  }
  // read stam value, stamp is generated by time function
  // the value should be same in .gcno file but skip compare here
  // bbg_stamp
  unsigned tag = GcovReadUnsigned();

  gcovData = localMP->New<GcovProfileData>(&alloc);
  GcovFuncInfo *funcInfo = nullptr;
  while ((tag = GcovReadUnsigned())) {
    unsigned length = GcovReadUnsigned();
    unsigned long base = GcovGetPosition();

    if (tag == GCOV_TAG_PROGRAM_SUMMARY) {
      struct gcov_summary summary;
      GcovReadSummary(&summary);
      object_runs += summary.ctrs[GCOV_COUNTER_ARCS].runs;
      program_count++;
    } else if (tag == GCOV_TAG_FUNCTION && length == GCOV_TAG_FUNCTION_LENGTH) {
      unsigned ident;
      /* Try to find the function in the list.  To speed up the
         search, first start from the last function found.  */
      ident = GcovReadUnsigned();
      unsigned lineno_checksum = GcovReadUnsigned();
      unsigned cfg_checksum = GcovReadUnsigned();
      funcInfo = localMP->New<GcovFuncInfo>(&alloc, ident, lineno_checksum, cfg_checksum);
      (gcovData->funcsCounter)[ident] = funcInfo;
    } else if (tag == GCOV_TAG_FOR_COUNTER (GCOV_COUNTER_ARCS)) {
      funcInfo->num_counts = GCOV_TAG_COUNTER_NUM(length);
      for (int ix = 0; ix != funcInfo->num_counts; ix++) {
        funcInfo->counts.push_back(GcovReadCounter());
      }
    }
    GcovSync(base, length);
  }
  GcovCloseFile();

  if (dumpDetail) {
    DumpFuncInfo();
  }
  return 0;
}

void MGcovParser::DumpFuncInfo() {
  for (auto &it : gcovData->funcsCounter) {
    GcovFuncInfo *funcInfo = it.second;
    LogInfo::MapleLogger() << "\nfunction ident " << std::dec << funcInfo->ident;
    LogInfo::MapleLogger() << "  lino_checksum 0x" << std::hex << funcInfo->lineno_checksum;
    LogInfo::MapleLogger() << "  cfg_checksum 0x" << std::hex <<  funcInfo->cfg_checksum << "\n";
    LogInfo::MapleLogger() << "  num_counts " <<  std::dec << funcInfo->num_counts << " : ";
    for (int i = 0; i < funcInfo->num_counts; i++) {
      LogInfo::MapleLogger() << std::dec << "  " << funcInfo->counts[i];
    }
  }
  LogInfo::MapleLogger() << "\n" ;
}

void M2MGcovParser::GetAnalysisDependence(AnalysisDep &aDep) const {
  aDep.SetPreservedAll();
}

bool M2MGcovParser::PhaseRun(maple::MIRModule &m) {
  MemPool *memPool = m.GetMemPool(); // use global pool to store gcov data
  MGcovParser gcovParser(m, memPool, true);
  int res = gcovParser.ReadGcdaFile();
  if (res) {
    // something wrong
    return false;
  }
  m.SetGcovProfile(gcovParser.GetGcovData());
  return true;
}


} // end namespace maple
