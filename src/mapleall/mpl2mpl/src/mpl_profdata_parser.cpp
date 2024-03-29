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

void ProfileSummaryImport::ReadSummary(MplProfileData *profData) {
  CHECK_FATAL(profData != nullptr, "sanity check");
  uint64_t magicNum = ReadNum<uint64_t>();
  CHECK_FATAL(magicNum == kMapleProfDataMagicNumber, "magic number error, quit\n");
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
      funcProf->counts[j] = static_cast<FreqType>(ReadNum<uint64_t>());
    }
  }
  return 0;
}

int MplProfDataParser::ReadMapleProfileData() {
  std::string mprofDataFile = Options::profile;
  if (mprofDataFile.empty()) {
    if (const char *envGcovprefix = std::getenv("GCOV_PREFIX")) {
      std::string fileNameWithPath = m.GetFileNameWithPath();
      static_cast<void>(mprofDataFile.append(envGcovprefix));
      if (mprofDataFile.back() != '/') {
        static_cast<void>(mprofDataFile.append("/"));
      }
      if (dumpDetail) {
        LogInfo::MapleLogger() << "set env GCOV_PREFIX= " << mprofDataFile << std::endl;
      }
      uint32_t stripnum = 0;
      if (const char *envGcovprefixstrip = std::getenv("GCOV_PREFIX_STRIP")) {
        std::string strip(envGcovprefixstrip);
        stripnum = static_cast<uint32>(std::stoi(strip));
        if (dumpDetail) {
          LogInfo::MapleLogger() << "set env GCOV_PREFIX_STRIP=" << strip << std::endl;
        }
      }
      // strip path in fileNameWithPath according to stripnum
      while (stripnum > 0 && fileNameWithPath.size() > 1) {
        size_t pos = fileNameWithPath.find_first_of("/", 1);
        if (pos == std::string::npos) {
          break;
        }
        fileNameWithPath= fileNameWithPath.substr(pos);
        stripnum--;
      }
      if (dumpDetail) {
        LogInfo::MapleLogger() << "profdata file stem after strip: " << fileNameWithPath<< std::endl;
      }
      CHECK_FATAL(fileNameWithPath.size() > 0, "sanity check");
      static_cast<void>(mprofDataFile.append(fileNameWithPath));
    } else {
      mprofDataFile = m.GetFileName();
      // strip path in mprofDataFile
      size_t pos = mprofDataFile.find_last_of("/");
      if (pos != std::string::npos) {
        mprofDataFile = mprofDataFile.substr(pos + 1);
      }
    }
    // change the suffix to .mprofdata
    mprofDataFile = mprofDataFile.substr(0, mprofDataFile.find_last_of(".")) + namemangler::kMplProfFileNameExt;
  }
  ASSERT(!mprofDataFile.empty(), "null check");
  LogInfo::MapleLogger() << "profileUse will open " << mprofDataFile << std::endl;
  // create mpl profdata
  profData = mempool->New<MplProfileData>(mempool, &alloc);
  // read .mprofdata
  std::ifstream inputStream(mprofDataFile, (std::ios::in | std::ios::binary));
  if (!inputStream) {
    if (opts::missingProfDataIsError) {
      CHECK_FATAL(inputStream, "Could not open profile data file %s, quit\n", mprofDataFile.c_str());
    } else {
      WARN(kLncWarn, "Could not open profile data file %s\n", mprofDataFile.c_str());
    }
    return 1;
  }
  // get length of file
  static_cast<void>(inputStream.seekg(0, std::ios::end));
  uint32_t length = static_cast<uint32>(inputStream.tellg());
  static_cast<void>(inputStream.seekg(0, std::ios::beg));

  std::unique_ptr<char[]> buffer = std::make_unique<char[]>(length);
  static_cast<void>(inputStream.read(buffer.get(), length));
  inputStream.close();
  // read 1st part summary
  ProfileSummaryImport summaryImport(mprofDataFile, inputStream);
  summaryImport.SetPosition(static_cast<uint8_t*>(static_cast<void*>(buffer.get())));
  summaryImport.ReadSummary(profData);
  if (dumpDetail) {
    profData->summary.DumpSummary();
  }
  // read 2nd part function profile data
  FunctionProfileImport funcImport(mprofDataFile, inputStream);
  funcImport.SetPosition(summaryImport.GetPosition());
  int res = funcImport.ReadFuncProfile(profData);
  if (res != 0) {
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
  bool enableDebug = false;            // true to dump trace
  if (m.GetFunctionList().empty()) {
    return false;       // there is no executable code
  }
  MplProfDataParser parser(m, memPool, enableDebug);
  int res = parser.ReadMapleProfileData();
  if (res != 0) {
    // something wrong
    LogInfo::MapleLogger() << " parse .mprofdata error\n";
    return false;
  }
  m.SetMapleProfile(parser.GetProfData());

  return true;
}

}  // end namespace maple
