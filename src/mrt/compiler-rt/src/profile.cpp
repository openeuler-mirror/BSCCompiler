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
#include "profile.h"

#include <cstddef>
#include <vector>
#include <map>
#include <iterator>
#include <iostream>
#include <iomanip>

#include "mm_config.h"
#include "cpphelper.h"
#include "yieldpoint.h"
#include "chosen.h"
#include "collector/stats.h"
#include "collector/native_gc.h"
#include "mstring_inline.h"

namespace maplert {
struct InstanceInfo {
  size_t bytes;
  size_t count;
  size_t size;
  std::string name;

  InstanceInfo() {
    bytes = 0;
    count = 0;
    size = 0;
  }
};

struct FuncInfo {
  uint32_t enterCount;
  std::string soname;
  std::string funcName;
  uint64_t funcaddr;
  explicit FuncInfo(std::string sonamein, std::string funcname, uint64_t faddr) {
    soname = std::move(sonamein);
    funcName = std::move(funcname);
    funcaddr = std::move(faddr);
    enterCount = 1;
  }
};

static std::unordered_map<uint64_t, FuncInfo*> recordFunc;
static std::mutex recordMtx;

static std::unordered_map<address_t*, const std::string> recordStaticField;
static std::mutex recordStaticFieldMtx;

void DumpRCAndGCPerformanceInfo(std::ostream &os) {
  stats::GCStats &curStats = *(stats::gcStats);

  os << "Registered native bytes allocated: " << NativeGCStats::Instance().GetNativeBytes() << std::endl;

  os << "Maximum stop-the-world time:\t" << curStats.MaxSTWNanos() << std::endl;
  os << "GC triggered:\t" << curStats.NumGCTriggered() << std::endl;
  os << "Average memory leak:\t" << curStats.AverageMemoryLeak() << std::endl;
  os << "Total memory leak:\t" << curStats.TotalMemoryLeak() << std::endl;
  os << "Number of heap allocation anomalies:\t" << curStats.NumAllocAnomalies() << std::endl;
  os << "Number of RC anomalies:\t" << curStats.NumRCAnomalies() << std::endl;
  os << "Total string num in intern pool:\t" << ConstStringPoolNum(false) << std::endl;
  os << "Total string size in intern pool:\t" << ConstStringPoolSize(false) << std::endl;
  os << "Total string num in literal pool:\t" << ConstStringPoolNum(true) << std::endl;
  os << "Total string size in literal pool:\t" << ConstStringPoolSize(true) << std::endl;

  size_t totalObjBytes = 0;

  // klass -> statistics map.
  std::map<MClass*, InstanceInfo> instanceInfos;

  {
    LOG(INFO) << "Stopping the world for usage-by-class statistics" << maple::endl;

    // Stop the world for statistics.
    ScopedStopTheWorld stw;

    // Now all threads are in safe region.
    // We enumerate all objects
    (void)(*theAllocator).ForEachObj([&instanceInfos, &totalObjBytes](address_t obj) {
      MObject *mObj = reinterpret_cast<MObject*>(obj);
      MClass *klass = mObj->GetClass();
      size_t bytes = mObj->GetSize();

      InstanceInfo &info = instanceInfos[klass];   // Creates element if not found.
      info.bytes += bytes;
      info.count += 1;

      // We leave the InstanceInfo::name field uninitialized here, for performance reason.
      totalObjBytes += bytes;
    }, true);
  } // The world starts here.

  os << "Total object size:\t" << totalObjBytes << std::endl;

  // Copy into a vector for sorting.
  std::vector<InstanceInfo> vec;
  for (auto &kv : instanceInfos) {
    auto &klass = kv.first;
    InstanceInfo &info = kv.second;
    info.size = klass->GetObjectSize();
    vec.push_back(info);
    klass->GetBinaryName(vec.back().name);
  }

  // Sort by bytes, count and name.
  std::sort(vec.begin(), vec.end(), [](InstanceInfo &a, InstanceInfo &b) {
    if (a.bytes != b.bytes) {
      return a.bytes > b.bytes;
    } else if (a.count != b.count) {
      return a.count > b.count;
    } else {
      return a.name < b.name;
    }
  });

  // Print, use std::setw() to ensure alignment
  os << "Heap usage by class:" << std::endl;
  os << std::setw(10) << "num" << std::setw(16) <<  "#instance" << std::setw(16) << "#bytes" <<
      std::setw(16) << "size" << "  class name" << std::endl;
  os << "-------------------------------------------------------------------" << std::endl;
  size_t rank = 1;

  for (auto &info : vec) {
    os << std::setw(10) << rank << std::setw(16) << info.count << std::setw(16) << info.bytes <<
        std::setw(16) << info.size << "  " << info.name << std::endl;
    rank++;
  }

  os << std::endl;
}

__attribute__((used)) void RecordMethod(uint64_t faddr, std::string &func, std::string &soname) {
  if (func.empty()) {
    LOG(INFO) << "func is empty  " << faddr << std::endl;
  }
  std::lock_guard<std::mutex> lock(recordMtx);
  recordFunc.insert(std::make_pair(faddr, new FuncInfo(soname, func, faddr)));
}

bool CheckMethodResolved(uint64_t faddr) {
  std::lock_guard<std::mutex> lock(recordMtx);
  auto item = recordFunc.find(faddr);
  if (item != recordFunc.end()) {
    (item->second)->enterCount++;
    return true;
  }
  return false;
}

void ClearFuncProfile() {
  std::lock_guard<std::mutex> lock(recordMtx);
  recordFunc.clear();
}

void DumpMethodUse(std::ostream &os) {
  os << "start record func " << std::endl;
  std::vector<FuncInfo*> funsInfo;
  // sort by calltimes
  for (auto &p : recordFunc) {
    if (p.second != nullptr) {
      funsInfo.push_back(p.second);
    }
  }
  std::sort(funsInfo.begin(), funsInfo.end(), [](const FuncInfo *a, const FuncInfo *b) {
    return a->enterCount < b->enterCount;
  });

  for (auto &p : funsInfo) {
#if RECORD_FUNC_NAME
    os << p->soname << "\t" << p->funcName << "\t" << p->enterCount << "\t" << std::hex <<
        p->funcaddr << std::dec << std::endl;
#else
    os << std::hex << "0x" << p->funcaddr << "\t" << p->enterCount << std::dec << std::endl;
#endif
  }
  os << "end record func " << std::endl;
}

void RecordStaticField(address_t *addr, const std::string name) {
  std::lock_guard<std::mutex> lock(recordStaticFieldMtx);
  if (recordStaticField.find(addr) == recordStaticField.end()) {
    recordStaticField.insert(std::make_pair(addr, name));
  }
}

void DumpStaticField(std::ostream &os) {
  os << "start record static fields " << std::endl;
  for (auto &p : recordStaticField) {
    os << p.first << " " << p.second << std::endl;
  }
  os << "end record static fields " << std::endl;
}
}
