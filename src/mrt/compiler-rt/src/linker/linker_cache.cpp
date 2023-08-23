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
#include "linker/linker_cache.h"

#include <fstream>
#include <thread>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>

#include "mrt_object.h"
#include "linker/linker_model.h"
#include "linker/linker_inline.h"
#include "mpl_cache_first_file.h"
namespace maplert {
#ifdef LINKER_RT_CACHE
using namespace linkerutils;
const uint64_t kCacheMagicNumber = 0xcac7e5272799710bull;
namespace {
  constexpr char kLinkerLazyInvalidSoName[] = "LAZY";
  constexpr char kLinkerInvalidName[] = "X";
}
static constexpr struct LinkerCache::CacheInfo methodUndef = {
    LinkerCache::kMethodUndefIndex,
    &LinkerInvoker::LookUpMethodSymbolAddress,
    kIsMethodUndefHasNotResolved,
    kIsMethodUndefCacheValid,
    true,
    true
};
static constexpr struct LinkerCache::CacheInfo dataUndef = {
    LinkerCache::kDataUndefIndex,
    &LinkerInvoker::LookUpDataSymbolAddress,
    kIsDataUndefHasNotResolved,
    kIsDataUndefCacheValid,
    false,
    true
};
static constexpr struct LinkerCache::CacheInfo methodDef = {
    LinkerCache::kMethodDefIndex,
    &LinkerInvoker::LookUpMethodSymbolAddress,
    kIsMethodDefHasNotResolved,
    kIsMethodCacheValid,
    true,
    false
};
static constexpr struct LinkerCache::CacheInfo dataDef = {
    LinkerCache::kDataDefIndex,
    &LinkerInvoker::LookUpDataSymbolAddress,
    kIsDataDefHasNotResolved,
    kIsDataCacheValid,
    false,
    false
};

const MUID LinkerCache::kInvalidHash = {{{ 0 }}};
FeatureName LinkerCache::featureName = kFLinkerCache;
void LinkerCache::SetPath(const std::string &path) {
  LINKER_VLOG(mpllinker) << pCachePath << " --> " << path << maple::endl;
  pCachePath = path;
}

LinkerMFileInfo *LinkerCache::FindLinkerMFileInfo(uint16_t soid, const MFileCacheInf &inf, MplInfoStore &store) {
  if (soid >= store.size()) {
    store.resize(soid + 1);
  }
  auto res = store[soid];
  if (res == nullptr) {
    res = pInvoker.GetLinkerMFileInfoByName(inf.GetName(soid));
    store[soid] = res;
  }
  return res;
}

bool LinkerCache::GetPath(LinkerMFileInfo &mplInfo, std::string &path,
    LinkerCacheType cacheType, LoadStateType loadState) {
  if (GetFastPath(mplInfo, cacheType, path)) {
    return true;
  }
  if (loadState == kLoadStateBoot || loadState == kLoadStateSS) {
    path = pCachePath; // Use preset value from SetCachePath().
    if (path.empty()) {
      if (loadState == kLoadStateBoot) {
        path = kLinkerRootCachePath;
      } else { // If loadState is equal to kLoadStateSS
        path = kLinkerSystemCachePath;
      }
    }
  } else if (loadState == kLoadStateApk) {
    path = GetAppInfo();
  } else {
    LINKER_LOG(ERROR) << "(" << static_cast<uint32_t>(cacheType) << "), failed to prepare folder, loadState=" <<
        loadState << maple::endl;
    return false;
  }
  path.append("/").append(kLinkerCacheFold).append("/");
  if (!PrepareFolder(path)) {
    LINKER_LOG(ERROR) << "(" << static_cast<uint32_t>(cacheType) << "), failed to prepare folder:" << path <<
        maple::endl;
    return false;
  }
  // Use the full path in case of the same so name.
  // e.g.
  //      /system/lib64/libmaplecore-all.so
  //  ==> _system_lib64_libmaplecore-all.so
  //
  //      /system/app/KeyChain/KeyChain.apk!/maple/arm64/mapleclasses.so
  //  ==> _system_app_KeyChain_KeyChain.apk!_maple_arm64_mapleclasses.so
  std::string name = mplInfo.name;
  std::replace(name.begin(), name.end(), '/', '_');
  path.append(name);
  FinishPath(mplInfo, cacheType, path);
  LINKER_VLOG(mpllinker) << "(" << static_cast<uint32_t>(cacheType) << "), loadState=" << loadState << ", " << path <<
      maple::endl;
  return true;
}

bool LinkerCache::GetFastPath(LinkerMFileInfo &mplInfo, LinkerCacheType cacheType, std::string &path) {
  if (cacheType == LinkerCacheType::kLinkerCacheLazy && mplInfo.rep.lazyCachePath.length() != 0) {
    path = mplInfo.rep.lazyCachePath;
    return true;
  } else if (cacheType == LinkerCacheType::kLinkerCacheMethodUndef && mplInfo.rep.methodUndefCachePath.length() != 0) {
    path = mplInfo.rep.methodUndefCachePath;
    return true;
  } else if (cacheType == LinkerCacheType::kLinkerCacheMethodDef && mplInfo.rep.methodDefCachePath.length() != 0) {
    path = mplInfo.rep.methodDefCachePath;
    return true;
  } else if (cacheType == LinkerCacheType::kLinkerCacheDataUndef && mplInfo.rep.dataUndefCachePath.length() != 0) {
    path = mplInfo.rep.dataUndefCachePath;
    return true;
  } else if (cacheType == LinkerCacheType::kLinkerCacheDataDef && mplInfo.rep.dataDefCachePath.length() != 0) {
    path = mplInfo.rep.dataDefCachePath;
    return true;
  }
  return false;
}

void LinkerCache::FinishPath(LinkerMFileInfo &mplInfo, LinkerCacheType cacheType, std::string &path) {
  if (cacheType == LinkerCacheType::kLinkerCacheLazy) {
    path += GetLinkerCacheTypeStr(cacheType);
    mplInfo.rep.lazyCachePath = path;
  } else if (cacheType == LinkerCacheType::kLinkerCacheMethodUndef) {
    path += GetLinkerCacheTypeStr(cacheType);
    mplInfo.rep.methodUndefCachePath = path;
  } else if (cacheType == LinkerCacheType::kLinkerCacheMethodDef) {
    path += GetLinkerCacheTypeStr(cacheType);
    mplInfo.rep.methodDefCachePath = path;
  } else if (cacheType == LinkerCacheType::kLinkerCacheDataUndef) {
    path += GetLinkerCacheTypeStr(cacheType);
    mplInfo.rep.dataUndefCachePath = path;
  } else if (cacheType == LinkerCacheType::kLinkerCacheDataDef) {
    path += GetLinkerCacheTypeStr(cacheType);
    mplInfo.rep.dataDefCachePath = path;
  } else {
    path += GetLinkerCacheTypeStr(cacheType);
  }
}

void LinkerCache::Reset() {
  pCachePath.clear();
}

inline LinkerCacheType LinkerCache::GetLinkerCacheType(CacheIndex cacheIndex) {
  switch (cacheIndex) {
    case kMethodUndefIndex:
      return LinkerCacheType::kLinkerCacheMethodUndef;
    case kMethodDefIndex:
      return LinkerCacheType::kLinkerCacheMethodDef;
    case kDataUndefIndex:
      return LinkerCacheType::kLinkerCacheDataUndef;
    case kDataDefIndex:
      return LinkerCacheType::kLinkerCacheDataDef;
  };
}

// Load the table from FS cache if exist.
// Return true if the cache exists and is valid, or return false.
bool LinkerCache::LoadTable(LinkerMFileInfo &mplInfo, CacheIndex cacheIndex) {
  if (LINKER_INSTALL_STATE) {
    return false;
  }
  LinkerCacheType cacheType = GetLinkerCacheType(cacheIndex);
  std::string path;
  if (!GetPath(mplInfo, path, cacheType, GetLoadState())) {
    LINKER_DLOG(mpllinker) << "(" << cacheIndex << "), failed to prepare folder for " <<
        path << ", " << errno << maple::endl;
    return false;
  }
  if (LoadTable(mplInfo, path, cacheIndex)) {
    LINKER_VLOG(mpllinker) << "(" << cacheIndex << "), runtime load cache:" << path << " OK" << maple::endl;
    return true;
  }
  LINKER_DLOG(mpllinker) << "load runtime path:" << path << " fail" << maple::endl;
  // try to load cache generated in HOTA/Install process
  if (LoadInstallCache(mplInfo, cacheType)) {
    LINKER_VLOG(mpllinker) << "(" << cacheIndex << "), install load cache:" << path << " OK" << maple::endl;
    return true;
  }
  return false;
}

bool LinkerCache::LoadTable(LinkerMFileInfo &mplInfo, const std::string &path, CacheIndex cacheIndex) {
  bool res = true;
  BufferSlice buf;
  size_t cacheSize = 0;
  void *content = LoadCache(path, cacheSize);
  if (content == nullptr) {
    res = false;
    goto END;
  }
  buf = BufferSlice(reinterpret_cast<char*>(content), cacheSize);
  // update before clhash and clsocnt.
  UpdateProperty(mplInfo, cacheIndex);
  if (!LoadFooter(buf, cacheIndex)) {
    res = false;
    goto END;
  }
  if (!LoadMeta(mplInfo, buf, cacheIndex)) {
    res = false;
    goto END;
  }
  LoadData(mplInfo, buf, cacheIndex);
  if (buf.Size() != 0) {
    res = false;
    LINKER_LOG(ERROR) << "invalid format, still have cache dat to resolve:" << mplInfo.name  << maple::endl;
  }
END:
  if (content != MAP_FAILED) {
    munmap(content, cacheSize);
  }
  if (!res) {
    LINKER_VLOG(mpllinker) << "(" << cacheIndex << "), to remove " << path << maple::endl;
    RemoveTable(mplInfo, cacheIndex); // In case of more exceptions, from here to re-save the cache.
  }
  return res;
}

void *LinkerCache::LoadCache(const std::string &path, size_t &cacheSize) {
  struct stat sb;
  int fd = open(path.c_str(), O_RDONLY);
  if (fd < 0) {
    if (errno != ENOENT && errno != EACCES) {
      LINKER_LOG(ERROR) << "failed to open " << path << ", " << errno << maple::endl;
    } else {
      if (errno == EACCES) {
        LINKER_LOG(ERROR) << "EACCES, failed to open " << path << ", " << errno << maple::endl;
      }
    }
    return nullptr;
  }
  // shared lock for read
  if (flock(fd, LOCK_SH) < 0) {
    LINKER_LOG(ERROR) << "failed to flock(" << errno << ") " << path << maple::endl;
    close(fd);
    return nullptr;
  }
  if (fstat(fd, &sb) < 0) {
    LINKER_LOG(ERROR) << "to obtain file size fail(" << errno << ") "<< path << maple::endl;
    close(fd);
    return nullptr;
  }
  cacheSize = sb.st_size;
  // {head:|MinVersion(int32)|MaxVersion(int32)|ValidityCode(MUID)|clhash(MUID)|clSoCount(uint16)|},
  // {data:|mapSize(uint32)|}, {footer:|contentLen(uint32)|cacheType(uint8)|CacheValidity(MUID)|magicNumber(uint64)|}
  const int minSize = sizeof(int32_t) * 2 + sizeof(MUID) * 2 + sizeof(uint16_t) + sizeof(uint32_t) +
      sizeof(uint32_t) + sizeof(uint8_t) + sizeof(MUID) + sizeof(uint64_t);
  if (cacheSize < minSize) {
    LINKER_LOG(INFO) << "failed, read no data for " << path << ", " << errno << maple::endl;
    close(fd);
    return nullptr;
  }
  void *content = mmap(nullptr, cacheSize, PROT_READ, MAP_FILE | MAP_PRIVATE, fd, 0);
  if (content == MAP_FAILED) {
    LINKER_LOG(ERROR) << "failed to mmap " << cacheSize << " (" << errno << ")for " << path << maple::endl;
    close(fd);
    return nullptr;
  }
  if (madvise(content, cacheSize, MADV_WILLNEED | MADV_SEQUENTIAL) < 0) {
    LINKER_LOG(ERROR) << "madvise failed(" << errno << ") " << path << maple::endl;
  }
  close(fd);
  return content;
}

bool LinkerCache::LoadFooter(BufferSlice &buf, CacheIndex cacheIndex) {
  size_t size = buf.Size();
  // 1. checking magic number.
  size -= sizeof(kCacheMagicNumber); // 8 bytes
  uint64_t magicNum = *(reinterpret_cast<uint64_t*>(&buf[size]));
  if (magicNum != kCacheMagicNumber) {
    LINKER_LOG(ERROR) << "magic number checking failed," << " wrong number:" << std::hex << magicNum << maple::endl;
    return false;
  }
  // 2. checking the validity.
  size -= sizeof(MUID);
  MUID lastCacheValidity = *(reinterpret_cast<MUID*>(&buf[size]));
  // Generate the digest for validity, excluding the length of content.
  MUID rtCacheValidity;
  GenerateMUID(buf.Data(), size, rtCacheValidity);
  if (lastCacheValidity != rtCacheValidity) {
    LINKER_LOG(ERROR) << "cache validity checking failed," << rtCacheValidity.ToStr() <<
        " vs. " << lastCacheValidity.ToStr() << maple::endl;
    return -1;
  }
  // 3. Read the cache type
  size -= sizeof(uint8_t); // 1 bytes
  uint8_t type = *(reinterpret_cast<uint8_t*>(&buf[size]));
  uint8_t wantedType = static_cast<uint8_t>(cacheIndex);
  if (type != wantedType) {
    LINKER_LOG(ERROR) << "cache index type failed," << wantedType << " vs. " << type << maple::endl;
    return false;
  }
  // 4. Read the length of content.
  size -= sizeof(uint32_t); // 4 bytes
  uint32_t contentSize = *(reinterpret_cast<uint32_t*>(&buf[size]));
  if (contentSize != size) {
    LINKER_LOG(ERROR) << "cache length checking failed," << contentSize <<
        " vs. " << size << maple::endl;
    return false;
  }
  buf = Slice<char>(buf.Data(), size);
  return true;
}

bool LinkerCache::LoadMeta(LinkerMFileInfo &mplInfo, BufferSlice &buf, CacheIndex cacheIndex) {
  // 1. Read maximum version.
  int32_t maxVersion = *(reinterpret_cast<int32_t*>(buf.Data()));
  if (maxVersion != 0 && maxVersion != GetMaxVersion()) {
    LINKER_LOG(ERROR) << "failed to check max version," << maxVersion <<
        " vs. " << GetMaxVersion() << maple::endl;
    return false;
  }
  buf += sizeof(maxVersion); // 4 bytes
  // 2. Read minimum version.
  int32_t minVersion = *(reinterpret_cast<int32_t*>(buf.Data()));
  if (minVersion != 0 && minVersion != GetMinVersion()) {
    LINKER_LOG(ERROR) << "failed to check min version," << minVersion << " vs. " <<
        GetMinVersion() << maple::endl;
    return false;
  }
  buf += sizeof(minVersion); // 4 bytes
  // 3. Read the hash from cache file, comparing with .so.
  MUID lastSoValidity = *(reinterpret_cast<MUID*>(buf.Data()));
  MUID rtSoValidity = pInvoker.GetValidityCode(mplInfo);
  if (lastSoValidity != rtSoValidity) {
    LINKER_LOG(ERROR) << "so validity checking failed," << rtSoValidity.ToStr() << " vs. " <<
        lastSoValidity.ToStr() << " in " << mplInfo.name << maple::endl;
    return false;
  }
  buf += sizeof(lastSoValidity); // 8 bytes
  // 4. Read the clhash from cache file
  MUID lastClValidity = *(reinterpret_cast<MUID*>(buf.Data()));
  MUID rtClValidity = pClHash[cacheIndex];
  if (lastClValidity != rtClValidity) {
    LINKER_LOG(ERROR) << "classloader validity checking failed, " << rtClValidity.ToStr() <<
        " vs. " << lastClValidity.ToStr() << " in " << mplInfo.name << maple::endl;
    return false;
  }
  buf += sizeof(lastClValidity); // 8 bytes
  // 5. Read the classloader .so count.
  uint16_t lastClSoCnt = *(reinterpret_cast<uint16_t*>(buf.Data()));
  uint16_t rtClSoCnt = pClSoCnt[cacheIndex];
  if (lastClSoCnt != rtClSoCnt) {
    LINKER_LOG(ERROR) << "classloader so count checking failed, " << rtClSoCnt << " vs. "
        << lastClSoCnt << " in " << mplInfo.name << maple::endl;
    return false;
  }
  buf += sizeof(lastClSoCnt); // 2 bytes
  return true;
}

bool LinkerCache::LoadData(LinkerMFileInfo &mplInfo, BufferSlice &buf, CacheIndex cacheIndex) {
  // 1. Read the map size
  uint32_t mapSize = *(reinterpret_cast<uint32_t*>(buf.Data()));
  buf += sizeof(mapSize); // 4 bytes
  if (mapSize == 0) {
    return true;
  }
  // 2. Read So Name List Info.
  LoadNameList(buf, cacheIndex);
  // 3. Read Bucket for resolving table.
  uint32_t mapBucketCnt = *(reinterpret_cast<uint32_t*>(buf.Data()));
  buf += sizeof(mapBucketCnt); // 4 bytes
  auto &cacheMap = *GetCacheMap(mplInfo, cacheIndex);
  cacheMap.rehash(mapBucketCnt);
  for (uint32_t i = 0; i < mapSize; ++i) {
    // 4. Read the undef index.
    uint32_t undefIndex = *(reinterpret_cast<uint32_t*>(buf.Data()));
    buf += sizeof(undefIndex);
    // 5. Read the addr index.
    uint32_t addrIndex = *(reinterpret_cast<uint32_t*>(buf.Data()));
    buf += sizeof(addrIndex);
    // 6. Read the soid
    uint16_t soid = *(reinterpret_cast<uint16_t*>(buf.Data()));
    buf += sizeof(soid);
    LinkerCacheTableItem pItem(addrIndex, soid);
    cacheMap.insert(std::make_pair(undefIndex, pItem));
  }
  return true;
}

bool LinkerCache::LoadNameList(BufferSlice &buf, CacheIndex cacheIndex) {
  auto &inf = pMplCacheInf[cacheIndex];
  // 6. listsize
  uint32_t listSize = *reinterpret_cast<uint32_t*>(buf.Data());
  buf += sizeof(listSize);
  for (uint32_t i = 0; i < listSize; ++i) {
    // 7. name size
    uint32_t nameSize = *reinterpret_cast<uint32_t*>(buf.Data());
    buf += sizeof(nameSize);
    // 8. name content
    const char *name = buf.Data();
    buf += nameSize;
    // 9. so hash.
    const MUID *muid = reinterpret_cast<const MUID*>(buf.Data());
    buf += sizeof(*muid);
    inf.Append(std::string(name, nameSize), *muid);
  }
  return true;
}

MplCacheMapT *LinkerCache::GetCacheMap(LinkerMFileInfo &mplInfo, CacheIndex cacheIndex) {
  switch (cacheIndex) {
    case kMethodUndefIndex:
      return &(mplInfo.rep.methodUndefCacheMap);
    case kMethodDefIndex:
      return &(mplInfo.rep.methodDefCacheMap);
    case kDataUndefIndex:
      return &(mplInfo.rep.dataUndefCacheMap);
    case kDataDefIndex:
      return &(mplInfo.rep.dataDefCacheMap);
  };
}

// Save the table into FS cache.
bool LinkerCache::SaveTable(LinkerMFileInfo &mplInfo, CacheIndex cacheIndex) {
  bool res = true;
  std::string path;
  std::string buffer;
  int fd = GetCacheFd(mplInfo, path, cacheIndex);
  if (fd < 0) {
    res = false;
    goto END;
  }
  // exclusive lock for write
  if (flock(fd, LOCK_EX) < 0) {
    LINKER_LOG(ERROR) << "failed to flock(" << errno << ") " << path << maple::endl;
    res = false;
    goto END;
  }
  if (ftruncate(fd, 0) < 0) {
    LINKER_LOG(ERROR) << "failed to ftruncate zero(" << errno << ") path:" << path << maple::endl;
    res = false;
    goto END;
  }
  UpdateProperty(mplInfo, cacheIndex);
  SaveMeta(mplInfo, buffer, cacheIndex);
  SaveData(mplInfo, buffer, cacheIndex);
  if (!WriteTable(fd, buffer, cacheIndex)) {
    res = false;
  }
END:
  if (!res) {
    RemoveTable(mplInfo, cacheIndex); // In case of more exceptions, from here to re-save the cache.
  }
  return CleanSavingTable(mplInfo, fd, res);
}

int LinkerCache::GetCacheFd(LinkerMFileInfo &mplInfo, std::string &path, CacheIndex cacheIndex) {
  int fd = -1;
  LinkerCacheType cacheType = GetLinkerCacheType(cacheIndex);
  if (LINKER_INSTALL_STATE) {
    if (mplInfo.IsFlag(kIsLazy)) {
      LINKER_LOG(WARNING) << "(" << cacheIndex << "), not save install cache for lazy binding." << maple::endl;
      return -1;
    }
    fd = maple::MplCacheFirstFile::GetFd(mplInfo.name, static_cast<int32_t>(cacheType));
    if (fd < 0) {
      LINKER_VLOG(mpllinker) << "(" << cacheIndex << ") get invalid fd when install" << maple::endl;
      return -1;
    }
  } else {
    if (!GetPath(mplInfo, path, cacheType, GetLoadState())) {
      LINKER_LOG(ERROR) << "(" << cacheIndex << "), failed to prepare folder for " <<
          path << ", " << errno << maple::endl;
      return -1;
    }
    fd = open(path.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
      if (errno != EACCES) {
        LINKER_LOG(ERROR) << "(" << cacheIndex << "), failed to open " <<
            path << ", " << errno << maple::endl;
      } else {
        LINKER_LOG(ERROR) << "(" << cacheIndex << "), EACCES, failed to open " <<
            path << ", " << errno << maple::endl;
      }
      return -1;
    }
  }
  return fd;
}

void LinkerCache::SaveMeta(LinkerMFileInfo &mplInfo, std::string &buffer, CacheIndex cacheIndex) {
  // Prepare all the data in buffer firstly.
  // 1. Write maximum version.
  int32_t maxVersion = GetMaxVersion();
  buffer.append(reinterpret_cast<char*>(&maxVersion), sizeof(maxVersion));
  // 2. Write minimum version.
  int32_t minVersion = GetMinVersion();
  buffer.append(reinterpret_cast<char*>(&minVersion), sizeof(minVersion));
  // 3. Write the hash from .so.
  MUID soValidity = pInvoker.GetValidityCode(mplInfo);
  buffer.append(reinterpret_cast<char*>(&soValidity), sizeof(soValidity));
  // 4. Write the clhash from classloader *.so
  buffer.append(reinterpret_cast<char*>(&pClHash[cacheIndex]), sizeof(pClHash[cacheIndex]));
  // 5. Write the classloader .so count.
  buffer.append(reinterpret_cast<char*>(&pClSoCnt[cacheIndex]), sizeof(pClSoCnt[cacheIndex]));
}

bool LinkerCache::SaveNameList(std::string &buffer, CacheIndex cacheIndex) {
  auto &inf = pMplCacheInf[cacheIndex];
  const auto &nameList = inf.NameList();
  // 1. Write the listsize
  uint32_t listSize = static_cast<uint32_t>(nameList.size());
  buffer.append(reinterpret_cast<char*>(&listSize), sizeof(listSize));
  for (const auto &item : nameList) {
    // 2. Write the name size
    const std::string &name = item.first;
    uint32_t size = static_cast<uint32_t>(name.size());
    buffer.append(reinterpret_cast<const char*>(&size), sizeof(size));
    // 3. Write the name data
    buffer.append(name.data(), name.size());
    // 4. Write the so hash.
    const MUID &hash = item.second;
    buffer.append(reinterpret_cast<const char*>(&hash), sizeof(hash));
  }
  return true;
}

bool LinkerCache::SaveData(LinkerMFileInfo &mplInfo, std::string &buffer, CacheIndex cacheIndex) {
  const auto &cacheMap = *GetCacheMap(mplInfo, cacheIndex);
  // 1. Write the size and nBucket of map.
  uint32_t mapSize = static_cast<uint32_t>(cacheMap.size());
  buffer.append(reinterpret_cast<char*>(&mapSize), sizeof(mapSize));
  if (mapSize == 0) {
    return true;
  }
  // 2. Write the so name list info.
  SaveNameList(buffer, cacheIndex);
  // 3. Write the bucket number.
  uint32_t mapBucketCnt = static_cast<uint32_t>(cacheMap.bucket_count());
  buffer.append(reinterpret_cast<char*>(&mapBucketCnt), sizeof(mapBucketCnt));
  for (auto it = cacheMap.begin(); it != cacheMap.end(); ++it) {
    const LinkerCacheTableItem &tableItem = it->second;
    // 4. Write the undef index.
    uint32_t undefIndex = it->first;
    buffer.append(reinterpret_cast<char*>(&undefIndex), sizeof(undefIndex));
    // 5. Write the addr index.
    uint32_t addrIndex = tableItem.AddrId();
    buffer.append(reinterpret_cast<char*>(&addrIndex), sizeof(addrIndex));
    // 6. Write the so index.
    uint16_t soid = tableItem.SoId();
    buffer.append(reinterpret_cast<char*>(&soid), sizeof(soid));
  }
  return true;
}

bool LinkerCache::WriteTable(int fd, std::string &buffer, CacheIndex cacheIndex) {
  // 1. Append content data
  uint32_t bufferSize = static_cast<uint32_t>(buffer.size());
  buffer.append(reinterpret_cast<char*>(&bufferSize), sizeof(bufferSize));
  // 2. Append cache index type.
  uint8_t type = static_cast<uint8_t>(cacheIndex);
  buffer.append(reinterpret_cast<char*>(&type), sizeof(type));
  // 2. Append validity crc.
  MUID cacheValidity;
  GenerateMUID(buffer.data(), buffer.size(), cacheValidity);
  buffer.append(reinterpret_cast<char*>(&cacheValidity), sizeof(cacheValidity));
  // 3. Append magic number.
  buffer.append(reinterpret_cast<const char*>(&kCacheMagicNumber), sizeof(kCacheMagicNumber));
  // Write for all cache data.
  if (write(fd, buffer.data(), buffer.size()) < 0) {
    LINKER_LOG(ERROR) << "failed to write cache content," << errno << maple::endl;
    return false;
  }
  return true;
}

bool LinkerCache::CleanSavingTable(LinkerMFileInfo &mplInfo, int fd, bool res) {
#ifdef LINKER_DECOUPLE_CACHE
  // chmod from 600 to 644. When installing/updating app and generating decouple cache, need to
  // read these cache as OTHER
  if (res) {
    if (fchmod(fd, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) < 0) {
      LINKER_LOG(ERROR) << "failed to chmod cache," << errno << " name:" << mplInfo.name << maple::endl;
      res = false;
    }
  }
#endif
  if (!LINKER_INSTALL_STATE) {
    close(fd);
  }
  return res;
}

// For debug only.
bool LinkerCache::DumpTable(LinkerMFileInfo &mplInfo, CacheIndex cacheIndex) {
  bool res = false;
  std::ifstream in;
  std::ofstream out;
  BufferSlice buf;
  std::stringstream ss;
  if (!InitDump(mplInfo, in, out, cacheIndex)) {
    goto END;
  }
  // Read all the data in buffer firstly.
  in.seekg(0, std::ios::end);
  buf = Slice<char>(new char[in.tellg()], in.tellg());
  in.seekg(0, std::ios::beg);
  if (!in.read(buf.Data(), buf.Size())) {
    LINKER_LOG(ERROR) << "(" << cacheIndex << "), failed to read all data," << errno << maple::endl;
    return false;
  }
  if (!DumpMetaValidity(ss, buf) || !DumpMetaVersion(mplInfo, ss, buf) || !DumpMetaCl(ss, buf, cacheIndex)) {
    goto END;
  }
  for (uint32_t type = 0; type < 2; ++type) { // 2 types is undef:0 or def:1
    if (type == 0) {
      ss << "\n===== UNDEF =====\n";
    } else {
      ss << "\n===== DEF =====\n";
    }
    auto mapSize = DumpMap(ss, buf);
    if (mapSize < 0) {
      goto END;
    } else if (mapSize == 0) {
      continue;
    }
    if (!DumpData(ss, buf, mapSize)) {
      goto END;
    }
  }
  if (!DumpFile(out, ss)) {
      goto END;
  }
  res = true;
END:
  in.close();
  out.close();
  if (!buf.Empty()) {
    delete []buf.Data();
  }
  return res;
}

bool LinkerCache::InitDump(LinkerMFileInfo &mplInfo, std::ifstream &in, std::ofstream &out, CacheIndex cacheIndex) {
  LinkerCacheType cacheType = GetLinkerCacheType(cacheIndex);
  std::string path;
  if (!GetPath(mplInfo, path, cacheType, GetLoadState())) {
    LINKER_LOG(ERROR) << "(" << cacheIndex << "), failed to prepare folder for " <<
        path << ", " << errno << maple::endl;
    return false;
  }
  in.open(path, std::ios::binary);
  bool ifStream = (!in);
  if (ifStream) {
    if (errno != ENOENT && errno != EACCES) {
      LINKER_LOG(ERROR) << "(" << cacheIndex << "), failed to open " << path << ", " << errno << maple::endl;
    }
    return false;
  }
  std::string dump = path + ".dump";
  out.open(dump, std::ios::trunc);
  bool ofstream = (!out);
  if (ofstream) {
    if (errno != EACCES) {
      LINKER_LOG(ERROR) << "(" << cacheIndex << "), failed to open " << dump << ", " << errno << maple::endl;
    }
    return false;
  }
  return true;
}

bool LinkerCache::DumpMetaValidity(std::stringstream &ss, BufferSlice &buf) {
  if (buf.Size() < 50) { // 50 is count all sizeof.
    LINKER_LOG(ERROR) << "failed, read no data," << errno << maple::endl;
    return false;
  }
  // 0. Read EOF.
  uint32_t eof = *(reinterpret_cast<const uint32_t*>(&buf[buf.Size() - sizeof(eof)]));
  if (eof != static_cast<uint32_t>(EOF)) {
    LINKER_LOG(ERROR) << "cache EOF checking failed, eof=" << std::hex << eof << maple::endl;
    return false;
  }
  // 1. Read the validity.
  MUID lastCacheValidity = *(reinterpret_cast<const MUID*>(buf.Data()));
  buf += sizeof(lastCacheValidity);
  // 2. Read the length of content.
  uint32_t contentSize = *(reinterpret_cast<const uint32_t*>(buf.Data()));
  buf += sizeof(contentSize);
  size_t size = buf.Size() - sizeof(eof);
  ss << "LEN:\t" << contentSize << "\n";
  ss << "\t" << contentSize << " vs. " << static_cast<uint32_t>(size) << "\n";
  if (contentSize != static_cast<uint32_t>(size)) {
    LINKER_LOG(ERROR) << "cache length checking failed," << contentSize << " vs. " <<
        static_cast<uint32_t>(size) << maple::endl;
    ss << "\t[FAILED]" << "\n";
    return false;
  }
  ss << "VALIDITY:\t" << lastCacheValidity.ToStr() << "\n";
  // Generate the digest for validity, excluding the length of content.
  MUID cacheValidity;
  GenerateMUID(buf.Data(), size, cacheValidity);
  ss << "\t" << lastCacheValidity.ToStr() << " vs. " << cacheValidity.ToStr() << "\n";
  if (lastCacheValidity != cacheValidity) {
    LINKER_LOG(ERROR) << "cache validity checking failed" << maple::endl;
    ss << "\t[FAILED]" << "\n";
    return false;
  }
  return true;
}

bool LinkerCache::DumpMetaVersion(LinkerMFileInfo &mplInfo, std::stringstream &ss, BufferSlice &buf) {
  // 3. Read maximum version.
  int32_t maxVersion = *(reinterpret_cast<const int32_t*>(buf.Data()));
  ss << "MAX:\t" << maxVersion << "\n";
  ss << "\t" << maxVersion << " vs. " << GetMaxVersion() << "\n";
  if (maxVersion != GetMaxVersion()) {
    LINKER_LOG(ERROR) << "failed to check max version for " << maxVersion << " vs. " << GetMaxVersion() << maple::endl;
    ss << "\t[FAILED]" << "\n";
    return false;
  }
  buf += sizeof(maxVersion);
  // 4. Read minimum version.
  int32_t minVersion = *(reinterpret_cast<const int32_t*>(buf.Data()));
  ss << "MIN:\t" << maxVersion << "\n";
  ss << "\t" << minVersion << " vs. " << GetMinVersion() << "\n";
  if (minVersion != GetMinVersion()) {
    LINKER_LOG(ERROR) << "failed to check min version for " << minVersion << " vs. " << GetMinVersion() << maple::endl;
    ss << "\t[FAILED]" << "\n";
    return false;
  }
  buf += sizeof(minVersion);
  // 5. Read the hash from cache file, comparing with .so.
  MUID lastSoValidity = *(reinterpret_cast<const MUID*>(buf.Data()));
  MUID soValidity = pInvoker.GetValidityCode(mplInfo);
  ss << "SO_HASH:\t" << lastSoValidity.ToStr() << "\n";
  ss << "\t" << lastSoValidity.ToStr() << " vs. " << soValidity.ToStr() << "\n";
  if (lastSoValidity != soValidity) {
    LINKER_LOG(ERROR) << "so validity checking failed, " << soValidity.ToStr() <<
        " vs. " << lastSoValidity.ToStr() << " in " << mplInfo.name << maple::endl;
    ss << "\t[FAILED]" << "\n";
    return false;
  }
  buf += sizeof(lastSoValidity);
  return true;
}

bool LinkerCache::DumpMetaCl(std::stringstream &ss, BufferSlice &buf, CacheIndex cacheIndex) {
  // 6. Read the hash from classloader.
  MUID lastClValidity = *(reinterpret_cast<const MUID*>(buf.Data()));
  MUID rtClValidity = pClHash[cacheIndex];
  ss << "CL_HASH:\t" << lastClValidity.ToStr() << "\n";
  ss << "\t" << lastClValidity.ToStr() << " vs. " << rtClValidity.ToStr() << "\n";
  if (lastClValidity != rtClValidity) {
    LINKER_LOG(ERROR) << "(" << cacheIndex << "), classloader validity checking failed, " <<
        rtClValidity.ToStr() << " vs. " << lastClValidity.ToStr();
    ss << "\t[FAILED]" << "\n";
    return false;
  }
  buf += sizeof(lastClValidity);
  // 7. Read the .so count.
  uint16_t lastClSoCnt = *(reinterpret_cast<const uint16_t*>(buf.Data()));
  uint16_t rtClSoCnt = pClSoCnt[cacheIndex];
  ss << "\tCL_nSO:\t" << lastClSoCnt << "\n";
  ss << "\t" << lastClSoCnt << " vs. " << rtClSoCnt << "\n";
  if (lastClSoCnt != rtClSoCnt) {
    LINKER_LOG(ERROR) << "(" << cacheIndex << "), classloader so cnt checking failed, " <<
        rtClSoCnt << " vs. " << lastClSoCnt;
    ss << "\t[FAILED]" << "\n";
    return false;
  }
  buf += sizeof(uint16_t);
  return true;
}

bool LinkerCache::DumpData(std::stringstream &ss, BufferSlice &buf, size_t mapSize) {
  std::vector<std::pair<std::string, MUID>> soList;
  for (size_t i = 0; i < mapSize; ++i) {
    MUID hash;
    uint16_t arrayIndex = 0;
    // 9. Read the undef index.
    uint32_t undefIndex = *(reinterpret_cast<const uint32_t*>(buf.Data()));
    ss << "\t*UNDEF_INX:\t" << undefIndex << "\n";
    buf += sizeof(undefIndex);
    // 10. Read the name length firstly.
    uint16_t len = *(reinterpret_cast<const uint16_t*>(buf.Data()));
    ss << "\tLEN_FLAG:\t" << len << "\n";
    if (len > PATH_MAX) {
      LINKER_LOG(ERROR) << "failed, length is too long:" << len << maple::endl;
      return false;
    }
    buf += sizeof(len);
    // 11. Read the name by its length.
    if (len == 0) { // Name index.
      arrayIndex = *(reinterpret_cast<const uint16_t*>(buf.Data()));
      buf += sizeof(arrayIndex);
      std::pair<std::string, MUID> pair = soList[arrayIndex];
      ss << "\tNAME:\t" << pair.first << "\n" << "\tHASH:\t" << pair.second.ToStr() << "\n";
    } else { // Name string, including INVALID_NAME.
      std::string text(buf.Data(), len);
      buf += len;
      if (len == kLinkerInvalidNameLen) {
        LINKER_LOG(ERROR) << "has not resolved symbol" << maple::endl;
        ss << "\tNAME:\tX" << "\n" << "\tHASH:\t(nul)" << "\n";
      } else {
        // 12. Read the hash code of .so
        hash = *(reinterpret_cast<const MUID*>(buf.Data()));
        buf += sizeof(hash);
        ss << "\tNAME:\t" << text << "\n" << "\tHASH:\t" << hash.ToStr() << "\n";
        auto soListIt = std::find_if (soList.begin(), soList.end(),
            [&text](const std::pair<std::string, MUID> &item){ return item.first == text; });
        if (soListIt == soList.end()) {
          soList.push_back(std::make_pair(text, hash));
        } else {
          LINKER_LOG(ERROR) << "bad format, has multiple name string" << maple::endl;
          ss << "\t[FAILED]" << "\n";
          return false;
        }
      }
    }
    // 13. Read the index.
    uint32_t idx = *(reinterpret_cast<const uint32_t*>(buf.Data()));
    ss << "\t*INDEX:\t" << idx << "\n";
    buf += sizeof(uint32_t);
  }
  return true;
}

int LinkerCache::DumpMap(std::stringstream &ss, BufferSlice &buf) {
  // 8. Read the map size and nBucket for resolving table.
  uint32_t mapSize = *(reinterpret_cast<const uint32_t*>(buf.Data()));
  ss << "MAP_SIZE:\t" << mapSize << "\n";
  ss << "\t" << mapSize << " vs. 0" << "\n";
  if (mapSize == 0) {
    ss << "\t[SUCC]" << "\n";
    return 0;
  }
  buf += sizeof(mapSize);
  uint32_t mapBucketCnt = *(reinterpret_cast<const uint32_t*>(buf.Data()));
  ss << "nBUCKET:\t" << mapBucketCnt << "\n";
  ss << "\t" << mapBucketCnt << " vs. 0" << "\n";
  if (mapBucketCnt == 0) {
    LINKER_LOG(ERROR) << "map nBucket is 0" << maple::endl;
    ss << "\t[FAILED]" << "\n";
    return -1;
  }
  buf += sizeof(mapBucketCnt);
  return mapSize;
}

bool LinkerCache::DumpFile(std::ofstream &out, std::stringstream &ss) {
  // 14. Dump the content.
  std::string content = ss.str();
  if (!out.write(content.data(), content.length())) {
    LINKER_LOG(ERROR) << "failed to write cache content:" << errno << maple::endl;
    return false;
  }
  return true;
}

// Remove FS cache.
bool LinkerCache::RemoveTable(LinkerMFileInfo &mplInfo, CacheIndex cacheIndex) {
  LinkerCacheType cacheType = GetLinkerCacheType(cacheIndex);
  std::string path;
  if (!GetPath(mplInfo, path, cacheType, GetLoadState())) {
    LINKER_LOG(ERROR) << "(" << cacheIndex << "), failed to prepare folder for " <<
        path << ", " << errno << maple::endl;
    return false;
  }
  if (std::remove(path.c_str())) {
    if (errno != ENOENT) {
      LINKER_LOG(ERROR) << "(" << cacheIndex << "), failed to remove " << path << ", " << errno << maple::endl;
    }
    return false;
  }
  LINKER_LOG(INFO) << "(" << cacheIndex << "), remove " << path << " successfully" << maple::endl;
  return true;
}

// Release the memory allocated.
// Notice: MUST only invoke ONCE after BOTH def&undef finished!
void LinkerCache::FreeTable(LinkerMFileInfo &mplInfo, CacheIndex cacheIndex) {
  switch (cacheIndex) {
    case kMethodUndefIndex:
      mplInfo.rep.methodUndefCacheMap.clear();
      MplCacheMapT().swap(mplInfo.rep.methodUndefCacheMap);
      mplInfo.SetFlag(kIsMethodUndefCacheValid, false);
      break;
    case kMethodDefIndex:
      mplInfo.rep.methodDefCacheMap.clear();
      MplCacheMapT().swap(mplInfo.rep.methodDefCacheMap);
      mplInfo.SetFlag(kIsMethodCacheValid, false);
      break;
    case kDataUndefIndex:
      mplInfo.rep.dataUndefCacheMap.clear();
      MplCacheMapT().swap(mplInfo.rep.dataUndefCacheMap);
      mplInfo.SetFlag(kIsDataUndefCacheValid, false);
      break;
    case kDataDefIndex:
      mplInfo.rep.dataDefCacheMap.clear();
      MplCacheMapT().swap(mplInfo.rep.dataDefCacheMap);
      mplInfo.SetFlag(kIsDataCacheValid, false);
      break;
  };
}

static constexpr uint32_t kLinkerMaxCacheNumPerType = 10;
bool LinkerCache::LoadInstallCache(LinkerMFileInfo &mplInfo, LinkerCacheType cacheType) {
  std::string path;
  if (!GetInstallCachePath(mplInfo, path, cacheType)) {
    return false;
  }
  path += '.';
  for (uint32_t i = 0; i < kLinkerMaxCacheNumPerType; ++i) {
    std::string file = path + static_cast<char>('0' + i);
    LINKER_DLOG(mpllinker) << "name:" << mplInfo.name << " get file:" << file << maple::endl;
    if (access(file.c_str(), R_OK) != 0) {
      LINKER_VLOG(mpllinker) << "name:" << mplInfo.name << " access file:" << file << " fail" << maple::endl;
      break;
    }

    bool ret = false;
    switch (cacheType) {
      case LinkerCacheType::kLinkerCacheMethodUndef:
        ret = LoadTable(mplInfo, file, kMethodUndefIndex);
        break;
      case LinkerCacheType::kLinkerCacheMethodDef:
        ret = LoadTable(mplInfo, file, kMethodDefIndex);
        break;
      case LinkerCacheType::kLinkerCacheDataUndef:
        ret = LoadTable(mplInfo, file, kDataUndefIndex);
        break;
      case LinkerCacheType::kLinkerCacheDataDef:
        ret = LoadTable(mplInfo, file, kDataDefIndex);
        break;
      default:
        break;
    }
    if (ret) {
      return true;
    }
  }
  return false;
}

void LinkerCache::UpdateProperty(LinkerMFileInfo &mplInfo, CacheIndex cacheIndex) {
  LinkerMFileInfoListT mplFileList;
  pInvoker.GetLinkerMFileInfos(mplInfo, mplFileList);
  pClHash[cacheIndex] = mplFileList.Hash(false);
  pClSoCnt[cacheIndex] = mplFileList.Size();
}

void LinkerCache::ResolveMethodSymbol(LinkerMFileInfo &mplInfo, AddrSlice &addrSlice, MuidSlice &muidSlice) {
  bool saveRtCache = false;
  if (mplInfo.rep.methodUndefCacheSize == -1) {
    mplInfo.SetFlag(kIsMethodUndefCacheValid, LoadTable(mplInfo, kMethodUndefIndex));
    mplInfo.rep.methodUndefCacheSize = mplInfo.rep.methodUndefCacheMap.size();
    if (mplInfo.rep.methodUndefCacheMap.size() < addrSlice.Size()) {
      mplInfo.SetFlag(kIsMethodUndefCacheValid, false);
    }
  }
  if (mplInfo.IsFlag(kIsMethodUndefCacheValid)) {
    saveRtCache = ProcessTable(mplInfo, addrSlice, muidSlice, methodUndef);
  } else {
    saveRtCache = LookUpUndefSymbol(mplInfo, addrSlice, muidSlice, methodUndef);
  }
  if (saveRtCache) {
    SaveTable(mplInfo, kMethodUndefIndex);
  }
}

void LinkerCache::FreeMethodUndefTable(LinkerMFileInfo &mplInfo) {
  FreeTable(mplInfo, kMethodUndefIndex);
}

bool LinkerCache::LookUpDefAddr(LinkerMFileInfo &mplInfo, const MUID &muid, LinkerOffsetType &addr,
    LinkerCacheTableItem &pItem, CacheInfo cacheInfo) {
  LinkerMFileInfo *resInfo = nullptr;
  LinkerOffsetType resAddr = 0;
  size_t index = 0;
  if (!pInvoker.ForEachLookUp(muid, &pInvoker, cacheInfo.lookUpSymbolAddress, mplInfo,
      &resInfo, index, resAddr)) {
    LINKER_LOG(ERROR) << "[RT/binary searching] failed to relocate MUID=" <<
        muid.ToStr() << " in " << mplInfo.name << maple::endl;
    mplInfo.SetFlag(cacheInfo.notResolved, true);
  } else {
    addr = resAddr;
    auto &inf = pMplCacheInf[cacheInfo.cacheIndex];
    uint16_t soid = inf.Append(resInfo->name, resInfo->hash);
    pItem.SetIds(index, soid).SetFilled();
    return true;
  }
  return false;
}

bool LinkerCache::LookUpDefSymbol(LinkerMFileInfo &mplInfo, AddrSlice &addrSlice, MuidSlice &muidSlice,
    CacheInfo cacheInfo) {
  auto &inf = pMplCacheInf[cacheInfo.cacheIndex];
  bool saveRtCache = false;
  std::mutex mtx;
  auto lookFunc = [&](size_t begSize, size_t endSize) {
    LinkerOffsetType resAddr = 0;
    LinkerMFileInfo *resInfo = nullptr;
    size_t index = 0;
    // Start binary search def from here.
    for (size_t i = begSize; i < endSize; ++i) {
      if (!pInvoker.ForEachLookUp(muidSlice[i].muid, &pInvoker, cacheInfo.lookUpSymbolAddress, mplInfo,
                                  &resInfo, index, resAddr)) { // Not found
        // Never reach here.
        LINKER_LOG(ERROR) << "failed to relocate MUID=" << muidSlice[i].muid.ToStr() <<
            " in " << mplInfo.name << ", resAddr=" << resAddr << maple::endl;
        mplInfo.SetFlag(cacheInfo.notResolved, true);
      } else { // Found
        if (mplInfo.name != resInfo->name) { // It's not in self SO.
          std::lock_guard<std::mutex> lck(mtx);
          addrSlice[i].addr = resAddr;
          uint16_t soid = inf.Append(resInfo->name, resInfo->hash);
          LinkerCacheTableItem pItem(static_cast<uint32_t>(index), soid);
          pItem.SetFilled();
          auto &cacheMap = *GetCacheMap(mplInfo, cacheInfo.cacheIndex);
          cacheMap[static_cast<uint32_t>(i)] = pItem;
        }
        saveRtCache = true;
      }
    }
  };
  const int threadNumber = 4; // half of the cpu number of phone.
  ParallelLookUp(lookFunc, threadNumber, addrSlice.Size());
  mplInfo.SetFlag(cacheInfo.cacheValid, true);
  return saveRtCache;
}

template<typename F>
void LinkerCache::ParallelLookUp(F const &lookFunc, int numThreads, size_t defSize) {
  size_t blockStart;
  size_t blockEnd;
  size_t blockSize = defSize / numThreads;
  std::vector<std::thread> threads;
  for (int i = 0; i < numThreads - 1; ++i) {
    blockStart = i * blockSize;
    blockEnd = (i + 1) * blockSize;
    threads.push_back(std::thread(lookFunc, blockStart, blockEnd));
  }
  lookFunc((numThreads - 1) * blockSize, defSize);
  for (auto &thr : threads) {
    thr.join();
  }
}

void LinkerCache::RelocateMethodSymbol(LinkerMFileInfo &mplInfo, AddrSlice &addrSlice, MuidSlice &muidSlice) {
  bool saveRtCache = false;
  if (mplInfo.rep.methodDefCacheSize == -1) {
    mplInfo.SetFlag(kIsMethodCacheValid, LoadTable(mplInfo, kMethodDefIndex));
    mplInfo.rep.methodDefCacheSize = mplInfo.rep.methodDefCacheMap.size();
  }
  if (mplInfo.IsFlag(kIsMethodCacheValid)) {
    saveRtCache = ProcessTable(mplInfo, addrSlice, muidSlice, methodDef);
  } else {
    saveRtCache = LookUpDefSymbol(mplInfo, addrSlice, muidSlice, methodDef);
  }
  if (saveRtCache) {
    SaveTable(mplInfo, kMethodDefIndex);
  }
}

void LinkerCache::FreeMethodDefTable(LinkerMFileInfo &mplInfo) {
  FreeTable(mplInfo, kMethodDefIndex);
}

bool LinkerCache::ProcessTable(LinkerMFileInfo &mplInfo, AddrSlice &addrSlice, MuidSlice &muidSlice,
    CacheInfo cacheInfo) {
  auto &inf = pMplCacheInf[cacheInfo.cacheIndex];
  auto &store = pMplStore[cacheInfo.cacheIndex];
  bool saveRtCache = false;
  auto &cacheMap = *GetCacheMap(mplInfo, cacheInfo.cacheIndex);
  for (auto it = cacheMap.begin(); it != cacheMap.end(); ++it) {
    bool noRtCache = false;
    uint32_t i = it->first;
    LinkerCacheTableItem &pItem = it->second;
    if (pItem.Filled()) { // Already done
      continue;
    } else if (pItem.LazyInvalidName() && cacheInfo.isUndef) {
      continue; // Ignore the symbols not found from system(Boot class loader).
    } else if (pItem.Valid()) {
      LinkerMFileInfo *res = FindLinkerMFileInfo(pItem.SoId(), inf, store);
      if (res != nullptr) {
        // We suppose that dex(.so) list is always changed with increment in runtime.
        // If so, we use dex(.so) list hash and its count to check the cache validity.
        const MUID &hash = inf.GetHash(pItem.SoId());
        if (hash == res->hash) {
          addrSlice[i].addr = GetAddr(*res, pItem, cacheInfo.isMethod);
          pItem.SetFilled();
        } else {
          noRtCache = true;
          if (VLOG_IS_ON(mpllinker)) {
            LINKER_VLOG(mpllinker) << "runtime cache is invalid(expired) {" << hash.ToStr() << " vs. " <<
                res->hash.ToStr() << "} for " << mplInfo.name << "->" << res->name << maple::endl;
          }
        }
      } else {
        mplInfo.SetFlag(cacheInfo.notResolved, true); // noRtCache is true; // Wait for next loading so.
      }
    } else if (cacheInfo.isUndef) {
      noRtCache = true;
    } else {
      // Never reach here!!
      LINKER_LOG(ERROR) << "no runtime cache so found? for " << mplInfo.name << maple::endl;
    }
    // If no runtime cached one found, start binary searching.
    if (cacheInfo.isUndef) {
      if (noRtCache && LookUpUndefAddr(mplInfo, muidSlice[i].muid, addrSlice[i].addr, pItem, dataUndef)) {
        saveRtCache = true;
      }
    } else {
      if (noRtCache && LookUpDefAddr(mplInfo, muidSlice[i].muid, addrSlice[i].addr, pItem, dataUndef)) {
        saveRtCache = true;
      }
    }
  }
  return saveRtCache;
}

bool LinkerCache::LookUpUndefAddr(LinkerMFileInfo &mplInfo, const MUID &muid,
                                  LinkerOffsetType &addr, LinkerCacheTableItem &pItem, CacheInfo cacheInfo) {
  auto &inf = pMplCacheInf[cacheInfo.cacheIndex];
  LinkerMFileInfo *resInfo = nullptr;
  size_t index = 0;
  LinkerOffsetType resAddr = 0;
  if (!pInvoker.ForEachLookUp(muid, &pInvoker,
      cacheInfo.lookUpSymbolAddress, mplInfo, &resInfo, index, resAddr)) { // Not found
    // Ignore the symbols not found from system(Boot class loader) for lazy binding.
    if (!mplInfo.IsFlag(kIsLazy)) {
      mplInfo.SetFlag(cacheInfo.notResolved, true);
    }
  } else { // Found
    // Ignore the symbols found not from system(Boot class loader).
    if (mplInfo.IsFlag(kIsLazy) && !resInfo->IsFlag(kIsBoot) &&
        !pInvoker.IsSystemClassLoader(reinterpret_cast<const MObject*>(resInfo->classLoader))) {
      uint16_t soid = inf.Append(kLinkerLazyInvalidSoName, kInvalidHash);
      pItem.SetLazyInvalidSoId(soid).SetFilled();
    } else {
      addr = resAddr;
      uint16_t soid = inf.Append(resInfo->name, resInfo->hash);
      pItem.SetIds(index, soid).SetFilled();
    }
    return true;
  }
  return false;
}

bool LinkerCache::UndefSymbolFailHandler(LinkerMFileInfo &mplInfo, uint32_t idx, CacheInfo cacheInfo) {
  auto &inf = pMplCacheInf[cacheInfo.cacheIndex];
  LinkerCacheTableItem pItem;
  // Ignore the symbols not found from system(Boot class loader).
  if (mplInfo.IsFlag(kIsLazy)) {
    uint16_t soid = inf.Append(kLinkerLazyInvalidSoName, kInvalidHash);
    pItem.SetLazyInvalidSoId(soid).SetFilled();
  } else {
    uint16_t soid = inf.Append(kLinkerInvalidName, kInvalidHash);
    pItem.SetInvalidSoId(soid);
  }
  auto &cacheMap = *GetCacheMap(mplInfo, cacheInfo.cacheIndex);
  cacheMap.insert(std::make_pair(idx, pItem));
  if (!mplInfo.IsFlag(kIsLazy)) {
    mplInfo.SetFlag(cacheInfo.notResolved, true);
  }
  return true;
}

bool LinkerCache::LookUpUndefSymbol(LinkerMFileInfo &mplInfo, AddrSlice &addrSlice, MuidSlice &muidSlice,
    CacheInfo cacheInfo) {
  auto &inf = pMplCacheInf[cacheInfo.cacheIndex];
  bool saveRtCache = false;
  // Start binary search from here.
  for (size_t i = 0; i < addrSlice.Size(); ++i) {
    // To check pTable[i].addr == 0 later?
    LinkerMFileInfo *resInfo = nullptr;
    size_t index = 0;
    LinkerOffsetType resAddr = 0;
    if (!pInvoker.ForEachLookUp(muidSlice[i].muid, &pInvoker,
        cacheInfo.lookUpSymbolAddress, mplInfo, &resInfo, index, resAddr)) { // Not found
      saveRtCache = UndefSymbolFailHandler(mplInfo, static_cast<uint32_t>(i), cacheInfo);
    } else { // Found
      LinkerCacheTableItem pItem;
      // Ignore the symbols found not from system(Boot class loader).
        if (mplInfo.IsFlag(kIsLazy) && !resInfo->IsFlag(kIsBoot) &&
            !pInvoker.IsSystemClassLoader(reinterpret_cast<const MObject*>(resInfo->classLoader))) {
        uint16_t soid = inf.Append(kLinkerLazyInvalidSoName, kInvalidHash);
        pItem.SetLazyInvalidSoId(soid).SetFilled();
      } else {
        addrSlice[i].addr = resAddr;
        uint16_t soid = inf.Append(resInfo->name, resInfo->hash);
        pItem.SetIds(index, soid).SetFilled();
      }
      auto &cacheMap = *GetCacheMap(mplInfo, cacheInfo.cacheIndex);
      cacheMap.insert(std::make_pair(i, pItem));
      saveRtCache = true;
    }
  }
  mplInfo.SetFlag(cacheInfo.cacheValid, true);
  return saveRtCache;
}

void LinkerCache::ResolveDataSymbol(LinkerMFileInfo &mplInfo, AddrSlice &addrSlice, MuidSlice &muidSlice) {
  bool saveRtCache = false;
  if (mplInfo.rep.dataUndefCacheSize == -1) {
    mplInfo.SetFlag(kIsDataUndefCacheValid, LoadTable(mplInfo, kDataUndefIndex));
    mplInfo.rep.dataUndefCacheSize = mplInfo.rep.dataUndefCacheMap.size();
    if (mplInfo.rep.dataUndefCacheMap.size() < addrSlice.Size()) {
      mplInfo.SetFlag(kIsDataUndefCacheValid, false);
    }
  }
  if (mplInfo.IsFlag(kIsDataUndefCacheValid)) {
    saveRtCache = ProcessTable(mplInfo, addrSlice, muidSlice, dataUndef);
  } else {
    saveRtCache = LookUpUndefSymbol(mplInfo, addrSlice, muidSlice, dataUndef);
  }
  if (saveRtCache) {
    SaveTable(mplInfo, kDataUndefIndex);
  }
}

void LinkerCache::FreeDataUndefTable(LinkerMFileInfo &mplInfo) {
  FreeTable(mplInfo, kDataUndefIndex);
}

void LinkerCache::RelocateDataSymbol(LinkerMFileInfo &mplInfo, AddrSlice &addrSlice, MuidSlice &muidSlice) {
  bool saveRtCache = false;
  if (mplInfo.rep.dataDefCacheSize == -1) {
    mplInfo.SetFlag(kIsDataCacheValid, LoadTable(mplInfo, kDataDefIndex));
    mplInfo.rep.dataDefCacheSize = mplInfo.rep.dataDefCacheMap.size();
  }
  if (mplInfo.IsFlag(kIsDataCacheValid)) {
    saveRtCache = ProcessTable(mplInfo, addrSlice, muidSlice, dataDef);
  } else {
    saveRtCache = LookUpDefSymbol(mplInfo, addrSlice, muidSlice, dataDef);
  }
  if (saveRtCache) {
    SaveTable(mplInfo, kDataDefIndex);
  }
}

void LinkerCache::FreeDataDefTable(LinkerMFileInfo &mplInfo) {
  FreeTable(mplInfo, kDataDefIndex);
}

LinkerOffsetType LinkerCache::GetAddr(LinkerMFileInfo &res, LinkerCacheTableItem &pItem, bool isMethod) {
  if (isMethod) {
#ifdef USE_32BIT_REF
    return pInvoker.AddrToUint32(pInvoker.GetMethodSymbolAddress(res, pItem.AddrId()));
#else
    return reinterpret_cast<LinkerOffsetType>(pInvoker.GetMethodSymbolAddress(res, pItem.AddrId()));
#endif // USE_32BIT_REF
  } else {
#ifdef USE_32BIT_REF
    return pInvoker.AddrToUint32(pInvoker.GetDataSymbolAddress(res, pItem.AddrId()));
#else
    return reinterpret_cast<LinkerOffsetType>(pInvoker.GetDataSymbolAddress(res, pItem.AddrId()));
#endif // USE_32BIT_REF
  }
}

#endif
} // namespace maplert
