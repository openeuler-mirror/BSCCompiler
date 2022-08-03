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

#include "mpl_profdata_parser.h"

#include <getopt.h>

#include <cassert>
#include <cinttypes>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>

#include "mpl_logging.h"
#include "option.h"

namespace maple {

template <typename T>
T ProfDataBinaryImportBase::ReadNum() {
  unsigned numBytesRead = 0;
  uint64_t val = namemangler::DecodeULEB128(pos, &numBytesRead);

  pos += numBytesRead;
  return static_cast<T>(val);
}

int ProfileSummaryImport::ReadSummary(MplProfileData *profData) {
  CHECK_FATAL(profData != nullptr, "sanity check");
  uint64_t magicNum = ReadNum<uint64_t>();
  if (magicNum != kMapleProfDataMagicNumber) {
    LogInfo::MapleLogger() << "magic number error, quit\n";
    return 1;
  }
  uint64_t checksum = ReadNum<uint64_t>();
  uint32_t runtimes = ReadNum<uint32_t>();
  uint32_t numofCounts = ReadNum<uint32_t>();
  uint64_t maxCount = ReadNum<uint64_t>();
  uint64_t sumCount = ReadNum<uint64_t>();
  profData->summary.SetSummary(checksum, runtimes, numofCounts, maxCount, sumCount);
  uint32_t veclen = ReadNum<uint32_t>();

  for (uint32_t i = 0; i < veclen; i++) {
    uint32_t r1 = ReadNum<uint32_t>();
    uint32_t r2 = ReadNum<uint32_t>();
    uint64_t r3 = ReadNum<uint64_t>();
    uint64_t r4 = ReadNum<uint64_t>();
    profData->summary.AddHistogramRecord(r1, r2, r3, r4);
  }

  return 0;
}

int FunctionProfileImport::ReadFuncProfile(MplProfileData *profData) {
  CHECK_FATAL(profData != nullptr, "sanity check");
  uint32_t funcNums = ReadNum<uint32_t>();
  if (funcNums == 0) {
    return 1;
  }

  for (uint32_t i = 0; i < funcNums; i++) {
    uint32_t funcIdent = ReadNum<uint32_t>();
    uint32_t linocheckSum = ReadNum<uint32_t>();
    uint32_t cfgcheckSum = ReadNum<uint32_t>();
    uint32_t countNum = ReadNum<uint32_t>();
    FuncProfInfo *funcProf = profData->AddNewFuncProfile(funcIdent, linocheckSum, cfgcheckSum, countNum);
    CHECK_FATAL(funcProf != nullptr, "nullptr check");
    funcProf->counts.resize(countNum);
    for (uint32_t j = 0; j < countNum; j++) {
      funcProf->counts[j] = ReadNum<uint64_t>();
    }
  }
  return 0;
}

int MplProfDataParser::ReadMapleProfileData() {
  std::string mprofDataFile = Options::profile;
  if (mprofDataFile.empty()) {
    if (const char *env_gcovprefix = std::getenv("GCOV_PREFIX")) {
      mprofDataFile.append(env_gcovprefix);
      if (mprofDataFile.back() != '/') {
        mprofDataFile.append("/");
      }
      if (dumpDetail) {
        LogInfo::MapleLogger() << "set env gcov_prefix= " << mprofDataFile << std::endl;
      }
      uint32_t stripnum = 0;
      if (const char *env_gcovprefixstrip = std::getenv("GCOV_PREFIX_STRIP")) {
        std::string strip(env_gcovprefixstrip);
        stripnum = std::stoi(strip);
        if (dumpDetail) {
          LogInfo::MapleLogger() << "set env gcov_prefix_strip=" << strip << std::endl;
        }
      }
      std::string profDataFileName = m.GetProfileDataFileName();
      if (dumpDetail) {
        LogInfo::MapleLogger() << "module profdata Name: " << profDataFileName << std::endl;
      }
      // reduce path in profDataFileName
      while (stripnum > 0 && profDataFileName.size() > 1) {
        size_t pos = profDataFileName.find_first_of("/", 1);
        if (pos == std::string::npos) {
          break;
        }
        profDataFileName = profDataFileName.substr(pos);
        stripnum--;
      }
      if (dumpDetail) {
        LogInfo::MapleLogger() << "after strip, module profdata Name: " << profDataFileName << std::endl;
      }
      CHECK_FATAL(profDataFileName.size() > 0, "sanity check");
      mprofDataFile.append(profDataFileName);
    } else {
      // if gcov_prefix is not set, find .mprofdata according to m.profiledata
      mprofDataFile = m.GetProfileDataFileName();
      if (dumpDetail) {
        LogInfo::MapleLogger() << "NO ENV, module profdata Name: " << mprofDataFile << std::endl;
      }
    }
    // add .mprofdata
    mprofDataFile.append(namemangler::kMplProfFileNameExt);
  }
  ASSERT(!mprofDataFile.empty(), "null check");
  if (dumpDetail) {
    LogInfo::MapleLogger() << "will open mprofileData " << mprofDataFile << std::endl;
  }
  // create mpl profdata
  profData = mempool->New<MplProfileData>(mempool, &alloc);
  // read .mprofdata
  std::ifstream inputStream(mprofDataFile, (std::ios::in | std::ios::binary));
  if (!inputStream) {
    LogInfo::MapleLogger() << "Could not open the file " << mprofDataFile << "\n";
    return 1;
  }
  // get length of file
  inputStream.seekg(0, std::ios::end);
  uint32_t length = inputStream.tellg();
  inputStream.seekg(0, std::ios::beg);
  const uint32_t sizeThreshold = 1024 * 10;
  CHECK_FATAL(length <= sizeThreshold, "NYI::large .mprofdata file size is larger than threashold, do chunk memory\n");

  std::unique_ptr<char[]> buffer = std::make_unique<char[]>(length);
  inputStream.read(buffer.get(), length);
  inputStream.close();
  // read 1st part summary
  ProfileSummaryImport summaryImport(mprofDataFile, inputStream);
  summaryImport.SetPosition((uint8_t*)(buffer.get()));
  int res = summaryImport.ReadSummary(profData);
  if (res) {
    LogInfo::MapleLogger() << "no summary part\n";
    return 1;
  }
  if (dumpDetail) {
    profData->summary.DumpSummary();
  }
  // read 2nd part function profile data
  FunctionProfileImport funcImport(mprofDataFile, inputStream);
  funcImport.SetPosition(summaryImport.GetPosition());
  res = funcImport.ReadFuncProfile(profData);
  if (res) {
    LogInfo::MapleLogger() << "no function profile part\n";
    return 1;
  }
  if (dumpDetail) {
    profData->DumpFunctionsProfile();
  }
  return 0;
}

void MMplProfDataParser::GetAnalysisDependence(AnalysisDep &aDep) const {
  aDep.SetPreservedAll();
}

bool MMplProfDataParser::PhaseRun(maple::MIRModule &m) {
  MemPool *memPool = m.GetMemPool();  // use global pool to store profile data
  bool enableDebug = true;            // true to dump trace
  MplProfDataParser parser(m, memPool, enableDebug);
  int res = parser.ReadMapleProfileData();
  if (res) {
    // something wrong
    LogInfo::MapleLogger() << " parse .mprofdata error\n";
    return false;
  }
  m.SetMapleProfile(parser.GetProfData());

  return true;
}

}  // end namespace maple
