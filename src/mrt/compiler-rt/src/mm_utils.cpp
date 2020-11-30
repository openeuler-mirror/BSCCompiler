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
#include "mm_utils.h"

#include <cinttypes>
#include <limits>
#include <dlfcn.h>
#include <dirent.h>
#include <mutex>
#include <sys/stat.h>

#include "mm_config.h"
#include "chosen.h"

namespace maplert {
namespace util {
const uint32_t kInstructionSize = 4;
const uint32_t kDefaultPrintStackDepth = 30;
const uint32_t kDefaultPrintStackSkip = 2; // skip first two frames in mmutils

void GetSymbolName(const void *exactPc, std::string &funcPath, std::string &funcName, Dl_info &dlinfo) {
  LinkerLocInfo mInfo;
  dlinfo.dli_fname = nullptr;
  dlinfo.dli_fbase = nullptr;
  dlinfo.dli_sname = nullptr;
  dlinfo.dli_saddr = nullptr;

  bool dladdrOk = dladdr(exactPc, &dlinfo) != 0;
  if (dladdrOk && dlinfo.dli_sname != nullptr) {
    funcName = dlinfo.dli_sname;
    funcPath = dlinfo.dli_fname;
  } else {
    if (LinkerAPI::Instance().IsJavaText(exactPc)) {
      (void)LinkerAPI::Instance().LocateAddress(exactPc, mInfo, true);
    }

    funcName = "unknown function";
    funcPath = "unknown image";
    if (!mInfo.sym.empty()) {
      funcName = mInfo.sym;
    }
    if (!mInfo.path.empty()) {
      funcPath = mInfo.path;
    }
  }
}

void PrintPCSymbol(const void *pc) {
  static constexpr int byteShift = 4;
  void *exactPc = reinterpret_cast<void*>(reinterpret_cast<intptr_t>(pc) - byteShift);
  // convert pc to symbol/offset in elf file
  std::string funcName, funcPath;
  Dl_info dlinfo;

  GetSymbolName(exactPc, funcPath, funcName, dlinfo);

#ifdef __ANDROID__
  LOG(ERROR) << "PC: " << std::hex << exactPc << " " << funcPath << " (+" <<
      reinterpret_cast<void*>(dlinfo.dli_fbase == nullptr ? 0 :
          (reinterpret_cast<intptr_t>(exactPc) - (reinterpret_cast<intptr_t>(dlinfo.dli_fbase)))) <<
      ") " << funcName << " (+" <<
      reinterpret_cast<void*>(dlinfo.dli_saddr == nullptr ? 0 :
          (reinterpret_cast<intptr_t>(exactPc) - (reinterpret_cast<intptr_t>(dlinfo.dli_saddr)))) <<
      ")" << std::dec << maple::endl;
#else
  printf("PC: %p %s (+%p) %s (+%p)\n",
         exactPc,
         funcPath.c_str(),
         reinterpret_cast<void*>(dlinfo.dli_fbase == nullptr ? 0 :
             (reinterpret_cast<intptr_t>(exactPc) - (reinterpret_cast<intptr_t>(dlinfo.dli_fbase)))),
         funcName.c_str(),
         reinterpret_cast<void*>(dlinfo.dli_saddr == nullptr ? 0 :
             (reinterpret_cast<intptr_t>(exactPc) - (reinterpret_cast<intptr_t>(dlinfo.dli_saddr)))));
#endif
}

void PrintPCSymbolToLog(const void *pc, uint32_t logtype, bool printBt) {
  void *exactPc = reinterpret_cast<void*>(reinterpret_cast<intptr_t>(pc) - kInstructionSize);
  // convert pc to symbol/offset in elf file
  std::string funcName = "";
  std::string funcPath = "";
  Dl_info dlinfo;

  GetSymbolName(exactPc, funcPath, funcName, dlinfo);

  LOG2FILE(logtype) << "PC: " << std::hex << exactPc << " " << funcPath << " (+" <<
      reinterpret_cast<void*>(dlinfo.dli_fbase == nullptr ? 0 :
          (reinterpret_cast<intptr_t>(exactPc) - (reinterpret_cast<intptr_t>(dlinfo.dli_fbase)))) <<
      ") " << funcName << " (+" <<
      reinterpret_cast<void*>(dlinfo.dli_saddr == nullptr ? 0 :
          (reinterpret_cast<intptr_t>(exactPc) - (reinterpret_cast<intptr_t>(dlinfo.dli_fbase)))) <<
      ") " << printBt << std::dec << std::endl;
}

void PrintPCSymbolToLog(const void *pc, std::ostream &ofs, bool printBt) {
  void *exactPc = reinterpret_cast<void*>(reinterpret_cast<intptr_t>(pc) - kInstructionSize);
  // convert pc to symbol/offset in elf file
  std::string funcName, funcPath;
  Dl_info dlinfo;

  GetSymbolName(exactPc, funcPath, funcName, dlinfo);

  ofs << "PC: " << std::hex << exactPc << " " << funcPath << " (+" <<
      reinterpret_cast<void*>(dlinfo.dli_fbase == nullptr ? 0 :
          (reinterpret_cast<intptr_t>(exactPc) - reinterpret_cast<intptr_t>(dlinfo.dli_fbase))) <<
      ") " << funcName << " (+" <<
      reinterpret_cast<void*>(dlinfo.dli_saddr == nullptr ? 0 :
          (reinterpret_cast<intptr_t>(exactPc) - reinterpret_cast<intptr_t>(dlinfo.dli_saddr))) <<
      ") " << printBt << std::dec << std::endl;
}

void PrintBacktrace() {
  PrintBacktrace(kDefaultPrintStackDepth, -1);
}

void PrintBacktrace(int32_t logFile) {
  PrintBacktrace(kDefaultPrintStackDepth, logFile);
}

std::mutex btmutex;

void PrintBacktrace(size_t limit, int32_t logFile) {
  std::lock_guard<std::mutex> l(btmutex);
  const size_t hardLimit = 30;
  std::vector<uint64_t> stackArray;

  if (limit > hardLimit) {
    limit = hardLimit;
  }

  if (logFile < 0) {
    printf("%lu: PrintBacktrace()\n", pthread_self());
  } else {
    LOG2FILE(logFile) << pthread_self() << ": PrintBacktrace()" << std::endl;
  }

  MapleStack::FastRecordCurrentStackPCsByUnwind(stackArray, limit);
  size_t size = stackArray.size();
  // skip first two frame, record stack and print stack
  for (size_t i = kDefaultPrintStackSkip; i < size; ++i) {
    if (logFile < 0) {
      PrintPCSymbol(reinterpret_cast<void*>(stackArray[i]));
    } else {
      PrintPCSymbolToLog(reinterpret_cast<void*>(stackArray[i]), logFile, false);
    }
  }
  if (logFile < 0) {
    fflush(stdout);
  }
}

// [obj] addr header rc meta
// content from meta
const size_t kDumpWordsPerLine = 4;
void DumpObject(address_t obj, std::ostream &ofs) {
  MObject *mObject = reinterpret_cast<MObject*>(obj);
  MClass *classInfo = mObject->GetClass();
  ofs << "[obj] " << std::hex << obj << " " << GCHeader(obj) <<
      " " << RefCount(obj) << " " << WeakRefCount(obj) << " " << classInfo <<
      std::dec << " " << GetObjectDWordSize(*mObject) << std::endl;
  if (VLOG_IS_ON(dumpheapsimple)) {
    auto refFunc = [&ofs](reffield_t &field, uint64_t kind) {
      address_t ref = RefFieldToAddress(field);
      if (IS_HEAP_ADDR(ref)) {
        ofs << std::hex << ref << (kind == kWeakRefBits ? " w" : (kind == kUnownedRefBits ? " u" : "")) <<
            std::dec << std::endl;
      }
    };
    ForEachRefField(obj, refFunc);
  } else {
    size_t word = GetObjectDWordSize(*mObject);
    for (size_t i = 0; i < word; i += kDumpWordsPerLine) {
      ofs << std::hex << (obj + (i * sizeof(address_t))) << ": ";
      size_t bound = (i + kDumpWordsPerLine) < word ? (i + kDumpWordsPerLine) : word;
      for (size_t j = i; j < bound; j++) {
        ofs << std::hex << *reinterpret_cast<reffield_t*>(obj + (j * sizeof(address_t))) << " ";
#ifdef USE_32BIT_REF
        ofs << std::hex << *reinterpret_cast<reffield_t*>(obj + (j * sizeof(address_t) + sizeof(reffield_t))) << " ";
#endif // USE_32BIT_REF
      }
      ofs << std::dec << std::endl;
    }
  }
}

// We must not fork until we're single-threaded again.
// Wait until /proc shows we're down to just one thread.
void WaitUntilAllThreadsStopped() {
  // If there are only 3 itmes in /proc/selftask,
  // we can confirm that all threads stopped except
  // main thread. for example:
  // $ls -a /proc/self/task
  // . .. <main tid>
  static constexpr int maxNumEntryOfSingleThreadProc = 3;
  struct dirent *entryPtr = nullptr;
  int numOfEntries;

  // All background threads are stopped already. We're just waiting
  // for their OS counterparts to finish as well. This shouldn't take
  // much time so spinning is ok here.
  for (;;) {
    // Find threads by listing files in "/proc/self/task".
    DIR *dirp = ::opendir("/proc/self/task");
    __MRT_ASSERT(dirp != nullptr, "WaitUntilAllThreadsStopped dirp=nullptr");
    for (numOfEntries = 0; numOfEntries <= maxNumEntryOfSingleThreadProc; ++numOfEntries) {
      entryPtr = ::readdir(dirp);
      if (entryPtr == nullptr) {
        break;
      }
    }
    ::closedir(dirp);

    if (numOfEntries == maxNumEntryOfSingleThreadProc) {
      return;
    }

    // Yield before try again.
    std::this_thread::yield();
  }
}

std::string GetLogDir() {
  string appDir = LinkerAPI::Instance().GetAppInfo();
  if (appDir.empty()) {
    // must be system server
    appDir = "/data/log/maple";
  } else {
    appDir = appDir + "/maple";
  }
  // Best effort.
  mode_t mode = S_IRWXU;
  if (mkdir(appDir.c_str(), mode) != 0) {
    int myErrno = errno;
    errno = 0;
    if (myErrno != EEXIST) {
      LOG(ERROR) << "Failed to create directory for GCLog." << maple::endl;
      LOG(ERROR) << "  Directory: " << appDir << maple::endl;
      LOG(ERROR) << "  Error code: " << strerror(myErrno) << maple::endl;
    }
  }

  return appDir;
}
} // namespace util
} // namespace maplert
