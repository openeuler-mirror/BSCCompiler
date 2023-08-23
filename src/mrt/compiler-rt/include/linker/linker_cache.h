/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_RUNTIME_MPL_LINKER_CACHE_H_
#define MAPLE_RUNTIME_MPL_LINKER_CACHE_H_

#include <string>

#include "linker_model.h"

namespace maplert {
#ifdef LINKER_RT_CACHE
class MFileCacheInf {
 public:
  uint16_t Append(const std::string &name, const MUID &muid) {
    auto iter = filter.insert(std::make_pair(name, 0));
    if (iter.second) {
      size_t size = data.size();
      data.push_back(std::make_pair(name, muid));
      iter.first->second = size;
      return size;
    }
    return iter.first->second;
  }

  // Required: id < data.size()
  const std::string &GetName(uint16_t id) const {
    return data[id].first;
  }
  const MUID &GetHash(uint16_t id) const {
    return data[id].second;
  }

  const std::vector<std::pair<std::string, MUID>> &NameList() const {
    return data;
  }

 private:
  std::unordered_map<std::string, uint16_t> filter;
  std::vector<std::pair<std::string, MUID>> data;
};

class LinkerCache : public FeatureModel {
 public:
  using MplInfoStore = std::vector<LinkerMFileInfo*>;
  using LookUpSymbolAddr = LinkerVoidType(maplert::LinkerInvoker::*)(LinkerMFileInfo&, const MUID&, size_t&);
  enum CacheIndex {
    kMethodUndefIndex = 0,
    kMethodDefIndex,
    kDataUndefIndex,
    kDataDefIndex
  };
  struct CacheInfo {
    CacheIndex cacheIndex;
    LookUpSymbolAddr lookUpSymbolAddress;
    MFileInfoFlags notResolved;
    MFileInfoFlags cacheValid;
    bool isMethod;
    bool isUndef;
  };

  static FeatureName featureName;
  explicit LinkerCache(LinkerInvoker &invoker)
      : pInvoker(invoker), pClSoCnt{ 0 } {}
  ~LinkerCache() = default;
  void SetPath(const std::string &path);
  bool GetPath(LinkerMFileInfo &mplInfo, std::string &path,
               LinkerCacheType cacheType, LoadStateType loadState);
  void Reset();
  void FreeAllTables(LinkerMFileInfo &mplInfo) {
    FreeTable(mplInfo, kMethodUndefIndex);
    FreeTable(mplInfo, kMethodDefIndex);
    FreeTable(mplInfo, kDataUndefIndex);
    FreeTable(mplInfo, kDataDefIndex);
  }
  bool RemoveAllTables(LinkerMFileInfo &mplInfo) {
    RemoveTable(mplInfo, kMethodUndefIndex);
    RemoveTable(mplInfo, kMethodDefIndex);
    RemoveTable(mplInfo, kDataUndefIndex);
    RemoveTable(mplInfo, kDataDefIndex);
    return true;
  }

  void ResolveMethodSymbol(LinkerMFileInfo &mplInfo, AddrSlice &addrSlice, MuidSlice &muidSlice);
  void FreeMethodUndefTable(LinkerMFileInfo &mplInfo);

  void RelocateMethodSymbol(LinkerMFileInfo &mplInfo, AddrSlice &addrSlice, MuidSlice &muidSlice);
  void FreeMethodDefTable(LinkerMFileInfo &mplInfo);

  void ResolveDataSymbol(LinkerMFileInfo &mplInfo, AddrSlice &addrSlice, MuidSlice &muidSlice);
  void FreeDataUndefTable(LinkerMFileInfo &mplInfo);

  void RelocateDataSymbol(LinkerMFileInfo &mplInfo, AddrSlice &addrSlice, MuidSlice &muidSlice);
  void FreeDataDefTable(LinkerMFileInfo &mplInfo);

  bool DumpTable(LinkerMFileInfo &mplInfo, CacheIndex cacheIndex);

 private:
  bool GetFastPath(LinkerMFileInfo &mplInfo, LinkerCacheType cacheType, std::string &path);
  void FinishPath(LinkerMFileInfo &mplInfo, LinkerCacheType cacheType, std::string &path);

  bool InitDump(LinkerMFileInfo &mplInfo, std::ifstream &in, std::ofstream &out, CacheIndex cacheIndex);
  bool DumpMetaValidity(std::stringstream &ss, BufferSlice &buf);
  bool DumpMetaVersion(LinkerMFileInfo &mplInfo, std::stringstream &ss, BufferSlice &buf);
  bool DumpMetaCl(std::stringstream &ss, BufferSlice &buf, CacheIndex cacheIndex);
  int DumpMap(std::stringstream &ss, BufferSlice &buf);
  bool DumpData(std::stringstream &ss, BufferSlice &buf, size_t mapSize);
  bool DumpFile(std::ofstream &out, std::stringstream &ss);

  void UpdateProperty(LinkerMFileInfo &mplInfo, CacheIndex cacheIndex);
  LinkerCacheType GetLinkerCacheType(CacheIndex cacheIndex);
  MplCacheMapT *GetCacheMap(LinkerMFileInfo &mplInfo, CacheIndex cacheIndex);

  bool RemoveTable(LinkerMFileInfo &mplInfo, CacheIndex cacheIndex);
  void FreeTable(LinkerMFileInfo &mplInfo, CacheIndex cacheIndex);

  bool LoadInstallCache(LinkerMFileInfo &mplInfo, LinkerCacheType cacheType);

  bool SaveTable(LinkerMFileInfo &mplInfo, CacheIndex cacheIndex);
  int GetCacheFd(LinkerMFileInfo &mplInfo, std::string &path, CacheIndex cacheIndex);
  void SaveMeta(LinkerMFileInfo &mplInfo, std::string &buffer, CacheIndex cacheIndex);
  bool SaveData(LinkerMFileInfo &mplInfo, std::string &buffer, CacheIndex cacheIndex);
  bool SaveNameList(std::string &buffer, CacheIndex cacheIndex);
  bool WriteTable(int fd, std::string &buffer, CacheIndex cacheIndex);
  bool CleanSavingTable(LinkerMFileInfo &mplInfo, int fd, bool res);

  bool LoadTable(LinkerMFileInfo &mplInfo, CacheIndex cacheIndex);
  bool LoadTable(LinkerMFileInfo &mplInfo, const std::string &path, CacheIndex cacheIndex);
  void *LoadCache(const std::string &path, size_t &cacheSize);
  bool LoadFooter(BufferSlice &buf, CacheIndex cacheIndex);
  bool LoadMeta(LinkerMFileInfo &mplInfo, BufferSlice &buf, CacheIndex cacheIndex);
  bool LoadData(LinkerMFileInfo &mplInfo, BufferSlice &buf, CacheIndex cacheIndex);
  bool LoadNameList(BufferSlice &buf, CacheIndex cacheIndex);

  bool ProcessTable(LinkerMFileInfo &mplInfo, AddrSlice &addrSlice, MuidSlice &muidSlice, CacheInfo cacheInfo);
  bool LookUpUndefSymbol(LinkerMFileInfo &mplInfo, AddrSlice &addrSlice, MuidSlice &muidSlice, CacheInfo cacheInfo);
  bool UndefSymbolFailHandler(LinkerMFileInfo &mplInfo, uint32_t idx, CacheInfo cacheInfo);
  bool LookUpDefSymbol(LinkerMFileInfo &mplInfo, AddrSlice &addrSlice, MuidSlice &muidSlice, CacheInfo cacheInfo);
  bool LookUpUndefAddr(LinkerMFileInfo &mplInfo, const MUID &muid,
                       LinkerOffsetType &addr, LinkerCacheTableItem &pItem, CacheInfo cacheInfo);
  bool LookUpDefAddr(LinkerMFileInfo &mplInfo, const MUID &muid,
                     LinkerOffsetType &addr, LinkerCacheTableItem &pItem, CacheInfo cacheInfo);
  LinkerMFileInfo *FindLinkerMFileInfo(uint16_t soid, const MFileCacheInf &inf, MplInfoStore &store);
  LinkerOffsetType GetAddr(LinkerMFileInfo &res, LinkerCacheTableItem &pItem, bool isMethod);

  template<typename F>
  void ParallelLookUp(F const &lookFunc, int numThreads, size_t defSize);

  static constexpr int kLinkerInvalidNameLen = 1;
  static constexpr int kMaxCacheIndex = kDataDefIndex + 1;
  static const MUID kInvalidHash;
  LinkerInvoker &pInvoker;
  std::string pCachePath;
  MUID pClHash[kMaxCacheIndex];      // Hash code of class loader .so list
  uint16_t pClSoCnt[kMaxCacheIndex]; // .so count in class loader
  MFileCacheInf pMplCacheInf[kMaxCacheIndex];
  MplInfoStore pMplStore[kMaxCacheIndex];
};
#endif // LINKER_RT_CACHE
} // namespace maplert
#endif
