/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_RUNTIME_GC_LOG_H
#define MAPLE_RUNTIME_GC_LOG_H

#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include "utils/time_utils.h"
#include "mm_config.h"

namespace maplert {
enum Logtype : int {
  kLogtypeGc = 0,
  kLogtypeRcTrace,
  kLogtypeRp,
  kLogtypeCycle,
  kLogTypeAllocFrag,
  kLogTypeAllocator,
  kLogTypeMix,
  kLogtypeGcOrStderr,
  kLogtypeNum
};

// gclog file stream buffer size.
const int kGcLogBufSize = 4096;

class GCLogImpl {
 public:
  GCLogImpl() = default;
  ~GCLogImpl() = default;
  std::ostream &Stream(uint32_t logType = kLogtypeGc) noexcept {
    return *os[logType];
  }

  void Init(bool openGClog);
  void OnPreFork();
  void OnPostFork();

  void OnGCStart();
  void OnGCEnd();

  bool IsWriteToFile(uint32_t logType = kLogtypeGc) const noexcept {
    return writeToFile[logType];
  }

 private:
  void OpenFile();
  void CloseFile();

  void SetFlags();

  std::ostream *os[kLogtypeNum]; // Current stream
  bool doWriteToFile;
  bool doOpenFileOnStartup;

  bool writeToFile[kLogtypeNum] = { false };
  bool openFileOnStartup[kLogtypeNum] = { false };

  std::ofstream file[kLogtypeNum]; // Open file

  char *buffer[kLogtypeNum] = { nullptr }; // buffer for file stream
  bool openGCLog = false;
};

GCLogImpl &GCLog();
std::string Pretty(uint64_t number);
std::string Pretty(uint32_t number);
std::string Pretty(int64_t number);
std::string PrettyOrderInfo(uint64_t number, std::string unit);
std::string PrettyOrderMathNano(uint64_t number, std::string unit);

#if __MRT_DEBUG
#define MRT_PASTE_ARGS_EXPANDED(x, y) x ## y
#define MRT_PASTE(x, y) MRT_PASTE_ARGS_EXPANDED(x, y)
#define MRT_PHASE_TIMER(...) PhaseTimerImpl MRT_PASTE(MRT_pt_, __LINE__)(__VA_ARGS__)
#else
#define MRT_PHASE_TIMER(...)
#endif
class PhaseTimerImpl {
 public:
  explicit PhaseTimerImpl(std::string name, uint32_t type = kLogtypeGc) : name(name), logType(type) {
    if (GCLog().IsWriteToFile(type)) {
      startTime = timeutils::MicroSeconds();
    }
  }

  ~PhaseTimerImpl() {
    if (GCLog().IsWriteToFile(logType)) {
      uint64_t stopTime = timeutils::MicroSeconds();
      uint64_t diffTime = stopTime - startTime;
      GCLog().Stream(logType) << name << " time: " << Pretty(diffTime) << "us\n";
    }
  }

 private:
  std::string name;
  uint64_t startTime = 0;
  uint32_t logType;
};

// The LOG2FILE(type) macro can only be used in this fashion:
//
//   LOG2FILE(xxxx) << blah << blah << blah;
//
// The if statement in the macro will nullify the effect of the << expression
// following the LOG2FILE macro.
#if __MRT_DEBUG
#define LOG2FILE(type) if (GCLog().IsWriteToFile(type)) GCLog().Stream(type)
#define FLUSH2FILE(type) if (GCLog().IsWriteToFile(type)) GCLog().Stream(type).flush()
#else
#define LOG2FILE(type) if (false) GCLog().Stream(type)
#define FLUSH2FILE(type) if (false) GCLog().Stream(type).flush()
#endif
} // namespace maplert

#endif
