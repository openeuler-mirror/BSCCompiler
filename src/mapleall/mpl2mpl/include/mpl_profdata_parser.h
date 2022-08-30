/*
 * Copyright (c) [2022] Futurewei Technologies, Inc. All rights reserved.
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
#include "bb.h"
#include "maple_phase_manager.h"
#include "mempool.h"
#include "mempool_allocator.h"
#include "mpl_profdata.h"

namespace maple {
// maple profile data format
//  Summary layout
//    MagicNumber
//    chesum
//    run times
//    counts number
//    count Maxium value
//    accumulated count sum
//    cout histogram size (count statistic information used for hot function check)
//      count lower bound      <-- begin of histogram details
//      countNum in current range
//      minium value in count range
//      accumulated count sum
//      count range            <-- another histogram element
//      countNum
//      minium value in cout range
//      accumulated count sum
//  Function profile layout (now only including edge profiling counts)
//    function numbers
//    function Ident (key number to identify each function(now use mirfunction->puidx)
//    function line number checksum
//    function cfg checksum
//    counts number in function 0
//      count  <-- begin of count value
//      count
//    counts number in function 1
//      count
//      ...
// now only unsigned number in profile data, use ULEB128 to encode/decode value for file size

const uint32_t kMapleProfDataMagicNumber = 0xA0EFEF;

class ProfDataBinaryImportBase {
 public:
  ProfDataBinaryImportBase(std::string &filename, std::ifstream &input) : fileName(filename), inputStream(input) {}
  template <typename T>
  T ReadNum();
  std::ifstream &GetInputStream() {
    return inputStream;
  }
  std::string &GetProfDataFileName() {
    return fileName;
  }
  void SetPosition(uint8_t *p) {
    pos = p;
  }
  uint8_t *GetPosition() {
    return pos;
  }

 private:
  std::string &fileName;
  std::ifstream &inputStream;
  uint8_t *pos = nullptr;
};

class ProfileSummaryImport : public ProfDataBinaryImportBase {
 public:
  ProfileSummaryImport(std::string &outputFile, std::ifstream &input) : ProfDataBinaryImportBase(outputFile, input) {}
  void ReadSummary(MplProfileData*);

 private:
  void ReadMProfMagic();
};

class FunctionProfileImport : public ProfDataBinaryImportBase {
 public:
  FunctionProfileImport(std::string &inputFile, std::ifstream &input) : ProfDataBinaryImportBase(inputFile, input) {}
  int ReadFuncProfile(MplProfileData *profData);
};

class MplProfDataParser : public AnalysisResult {
 public:
  MplProfDataParser(MIRModule &mirmod, MemPool *mp, bool debug)
      : AnalysisResult(mp), m(mirmod), alloc(memPool), mempool(mp), dumpDetail(debug) {}
  ~MplProfDataParser() = default;
  MplProfileData *GetProfData() {
    return profData;
  }
  int ReadMapleProfileData();

 private:
  MIRModule &m;
  MapleAllocator alloc;
  MemPool *mempool;
  MplProfileData *profData = nullptr;
  bool dumpDetail = false;
};

MAPLE_MODULE_PHASE_DECLARE(MMplProfDataParser)

}  // end of namespace maple

#endif  // MAPLE_MPL2MPL_INCLUDE_GCOVPROFUSE_H
