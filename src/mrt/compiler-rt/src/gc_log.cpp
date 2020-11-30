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
#include "gc_log.h"

#include <sys/stat.h>
#include "panic.h"
#include "collector/stats.h"
#include "cinterface.h"
#include "linker_api.h"

#define SET_LOG_FLAGS(type, usefile, verbose, open) do { \
  writeToFile[type] = MRT_ENVCONF(usefile, usefile##_DEFAULT) || VLOG_IS_ON(verbose); \
  openFileOnStartup[type] = MRT_ENVCONF(open, open##_DEFAULT) || VLOG_IS_ON(verbose); \
} while (0)

namespace maplert {
class NullBuffer : public std::streambuf {
 public:
  int overflow(int c) override {
    return c;
  }
};

class NullStream : public std::ostream {
 public:
  NullStream() : std::ostream(&sb) {}
  ~NullStream() = default;
 private:
  NullBuffer sb;
};

static NullStream nullStream;

namespace {
const char *kPrefixName[kLogtypeNum] = {
    "gclog_", "rctracelog_", "rplog_", "cyclelog_", "allocatorfragmentlog_", "allocatorlog_", "mixlog_", "stderr_"
};

// Orders of magnitudes.  Note: The upperbound of uint64_t is 16E (16 * (1024 ^ 6))
const char *kOrdersOfManitude[] = { "", "K", "M", "G", "T", "P", "E" };

// Orders of magnitudes.  Note: The upperbound of uint64_t is 16E (16 * (1024 ^ 6))
const char *kOrdersOfMagnitudeFromNano[] = { "n", "u", "m", nullptr };

// number of digits in a pretty format segment (100,000,000 each has three digits)
constexpr int kNumDigitsPerSegment = 3;
}

void GCLogImpl::Init(bool gcLog) {
  openGCLog = gcLog;
  SetFlags();
  if (doWriteToFile && doOpenFileOnStartup) {
    OpenFile();
  }
}

void GCLogImpl::SetFlags() {
  SET_LOG_FLAGS(kLogtypeGc, MRT_GCLOG_USE_FILE, opengclog, MRT_GCLOG_OPEN_ON_STARTUP);
  SET_LOG_FLAGS(kLogtypeRcTrace, MRT_RCTRACELOG_USE_FILE, openrctracelog, MRT_RCTRACELOG_OPEN_ON_STARTUP);
  SET_LOG_FLAGS(kLogtypeRp, MRT_RPLOG_USE_FILE, openrplog, MRT_RPLOG_OPEN_ON_STARTUP);
  SET_LOG_FLAGS(kLogtypeCycle, MRT_CYCLELOG_USE_FILE, opencyclelog, MRT_CYCLELOG_OPEN_ON_STARTUP);
  SET_LOG_FLAGS(kLogTypeAllocFrag, MRT_ALLOCFRAGLOG_USE_FILE, allocatorfragmentlog, MRT_ALLOCFRAGLOG_OPEN_ON_STARTUP);
  SET_LOG_FLAGS(kLogTypeAllocator, MRT_ALLOCATORLOG_USE_FILE, allocatorlog, MRT_ALLOCATOR_OPEN_ON_STARTUP);
  SET_LOG_FLAGS(kLogTypeMix, MRT_MIXLOG_USE_FILE, openmixlog, MRT_MIXLOG_OPEN_ON_STARTUP);

  if (openGCLog) {
    writeToFile[kLogtypeGc] = true;
    openFileOnStartup[kLogtypeGc] = true;
  }
  doWriteToFile = false;
  doOpenFileOnStartup = false;
  for (int i = 0; i < kLogtypeNum; ++i) {
    if (i == kLogtypeGcOrStderr) {
#ifdef __ANDROID__
      writeToFile[i] = false;
      (void)file[i].rdbuf()->pubsetbuf(nullptr, 0); // set 0 as unbuffered stream to save memory when gclog disabled
      os[i] = &nullStream;
      doWriteToFile = doWriteToFile || writeToFile[i];
      doOpenFileOnStartup = doOpenFileOnStartup || openFileOnStartup[i];
#else
      writeToFile[i] = true;
      os[i] = &std::cerr;
#endif
    } else {
      (void)file[i].rdbuf()->pubsetbuf(nullptr, 0); // set 0 as unbuffered stream to save memory when gclog disabled
      os[i] = &nullStream;
      doWriteToFile = doWriteToFile || writeToFile[i];
      doOpenFileOnStartup = doOpenFileOnStartup || openFileOnStartup[i];
    }
  }
}

void GCLogImpl::OpenFile() {
  if (!doWriteToFile) {
    return;
  }

  pid_t pid = getpid();
  std::string dateDigit = timeutils::GetDigitDate();
#ifdef __ANDROID__
  std::string dirName = util::GetLogDir();
#else
  std::string dirName = ".";
#endif

  for (int i = 0; i < kLogtypeNum; ++i) {
    if (!writeToFile[i] || (i == kLogtypeGcOrStderr)) {
      continue;
    }

    std::string baseName = kPrefixName[i] + std::to_string(pid) + "_" + dateDigit + ".txt";
    std::string fileName = dirName + "/" + baseName;
    file[i] = std::ofstream(fileName, std::ofstream::app); // Assignment closes the old file.
    if (!file[i]) {
      LOG(ERROR) << "GCLogImpl::OpenFile(): fail to open the file" << maple::endl;
      continue;
    }
    buffer[i] = new (std::nothrow) char[kGcLogBufSize];
    if (buffer[i] == nullptr) {
      LOG(ERROR) << "GCLogImpl::OpenFile(): new char[kGcLogBufSize] failed" << maple::endl;
      continue;
    }
    (void)file[i].rdbuf()->pubsetbuf(buffer[i], kGcLogBufSize);
    os[i] = &file[i];
  }
#ifdef __ANDROID__
    writeToFile[kLogtypeGcOrStderr] = writeToFile[kLogtypeGc];
    os[kLogtypeGcOrStderr] = &file[kLogtypeGc];
#endif
}

void GCLogImpl::CloseFile() {
  if (!doWriteToFile) {
    return;
  }

  for (int i = 0; i < kLogtypeNum; ++i) {
    if (writeToFile[i] && (i != kLogtypeGcOrStderr)) {
      if (os[i] == &file[i]) {
        os[i] = &nullStream;
        file[i].close();
        delete[] buffer[i];
        buffer[i] = nullptr;
      }
    }
  }
}

void GCLogImpl::OnPreFork() {
  CloseFile();
}

void GCLogImpl::OnPostFork() {
  SetFlags();
  OpenFile();
}

void GCLogImpl::OnGCStart() {
  std::string dateDigit = timeutils::GetDigitDate();
  Stream(kLogtypeGc) << "Begin GC log. Time: " << dateDigit << '\n';
  Stream(kLogtypeGc) << "Current allocated: " << Pretty(stats::gcStats->CurAllocBytes()) <<
      " Current threshold: " << Pretty(stats::gcStats->CurGCThreshold()) << '\n';
  Stream(kLogtypeGc) << "Current allocated native bytes (before GC): " << Pretty(MRT_GetNativeAllocBytes()) << '\n';
}

void GCLogImpl::OnGCEnd() {
  Stream(kLogtypeGc) << "Current allocated native bytes (after  GC): " << Pretty(MRT_GetNativeAllocBytes()) << '\n';
  Stream(kLogtypeGc) << "End of GC log.\n\n";
  Stream(kLogtypeGc).flush();
}

GCLogImpl gcLogInstance;

GCLogImpl &GCLog() {
  return gcLogInstance;
}

std::string Pretty(uint64_t number) {
  std::string orig = std::to_string(number);
  int pos = static_cast<int>(orig.length()) - kNumDigitsPerSegment;
  while (pos > 0) {
    orig.insert(pos, ",");
    pos -= kNumDigitsPerSegment;
  }
  return orig;
}

std::string Pretty(int64_t number) {
  std::string orig = std::to_string(number);
  int pos = static_cast<int>(orig.length()) - kNumDigitsPerSegment;
  while (pos > 0) {
    orig.insert(pos, ",");
    pos -= kNumDigitsPerSegment;
  }
  return orig;
}

std::string Pretty(uint32_t number) {
  return Pretty(static_cast<uint64_t>(number));
}

// Useful for informatic units, such as KiB, MiB, GiB, ...
std::string PrettyOrderInfo(uint64_t number, std::string unit) {
  size_t order = 0;
  const uint64_t factor = 1024;

  while (number > factor) {
    number /= factor;
    order += 1;
  }

  const char *prefix = kOrdersOfManitude[order];
  const char *infix = order > 0 ? "i" : ""; // 1KiB = 1024B, but there is no "1iB"

  return std::to_string(number) + std::string(prefix) + std::string(infix) + unit;
}

// Useful for scientific units where number is in nanos: ns, us, ms, s
std::string PrettyOrderMathNano(uint64_t number, std::string unit) {
  size_t order = 0;
  const uint64_t factor = 1000; // show in us if under 10ms

  while (number > factor && kOrdersOfMagnitudeFromNano[order] != nullptr) {
    number /= factor;
    order += 1;
  }

  const char *prefix = kOrdersOfMagnitudeFromNano[order];
  if (prefix == nullptr) {
    prefix = "";
  }

  return std::to_string(number) + std::string(prefix) + unit;
}
}