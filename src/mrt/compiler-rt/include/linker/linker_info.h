/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_RUNTIME_MPL_LINKER_INFO_H_
#define MAPLE_RUNTIME_MPL_LINKER_INFO_H_

#include <mutex>
#include <algorithm>

#include "linker_utils.h"
#include "methodmeta.h"

namespace maplert {
MUID GetHash(const std::vector<LinkerMFileInfo*> &mplInfos, bool getSolidHash);

class LinkerMFileInfoListT {
 public:
  using InfoList = std::vector<LinkerMFileInfo*>;

  LinkerMFileInfoListT() {}
  ~LinkerMFileInfoListT() {}

  LinkerMFileInfoListT(const LinkerMFileInfoListT &other) = delete;
  LinkerMFileInfoListT &operator=(const LinkerMFileInfoListT&) = delete;

  void Merge(const InfoList &other) {
    std::lock_guard<std::mutex> lock(mutex);
    (void)infos.insert(infos.end(), other.begin(), other.end());
  }

  void Clear() {
    std::lock_guard<std::mutex> lock(mutex);
    infos.clear();
  }

  std::vector<LinkerMFileInfo*> Clone() {
    std::lock_guard<std::mutex> lock(mutex);
    return infos;
  }

  LinkerMFileInfo *Get(size_t index) {
    std::lock_guard<std::mutex> lock(mutex);
    return infos[index];
  }

  size_t Size() {
    std::lock_guard<std::mutex> lock(mutex);
    return infos.size();
  }

  bool Empty() {
    std::lock_guard<std::mutex> lock(mutex);
    return infos.empty();
  }

  void Append(LinkerMFileInfo &mplInfo, int32_t pos) {
    std::lock_guard<std::mutex> lock(mutex);
    if (pos <= -1) {
      infos.push_back(&mplInfo);
    } else {
      (void)infos.insert(infos.begin() + pos, &mplInfo);
    }
  }
  template<typename UnaryFunction>
  void ForEach(const UnaryFunction f) {
    std::lock_guard<std::mutex> lock(mutex);
    for (auto mplInfo : infos) {
      f(*mplInfo);
    }
  }
  template<class UnaryPredicate>
  bool FindIf(const UnaryPredicate p) {
    std::lock_guard<std::mutex> lock(mutex);
    for (auto mplInfo : infos) {
      if (p(*mplInfo)) {
        return true;
      }
    }
    return false;
  }
  template<class UnaryPredicate>
  bool FindIfNot(UnaryPredicate p) {
    std::lock_guard<std::mutex> lock(mutex);
    for (auto mplInfo : infos) {
      if (!p(*mplInfo)) {
        return false;
      }
    }
    return true;
  }
  MUID Hash(bool getSolidHash) {
    std::lock_guard<std::mutex> lock(mutex);
    return GetHash(infos, getSolidHash);
  }

 private:
  std::mutex mutex;
  InfoList infos;
};

class LinkerMFileInfoListCLMapT {
 public:
  using ClInfoMapT = std::unordered_map<const MObject*, std::vector<LinkerMFileInfo*>>;

  LinkerMFileInfoListCLMapT() {
    (void)pthread_rwlock_init(&lock, nullptr);
  }
  ~LinkerMFileInfoListCLMapT() {}

  LinkerMFileInfoListCLMapT(const LinkerMFileInfoListCLMapT &other) = delete;
  LinkerMFileInfoListCLMapT &operator=(const LinkerMFileInfoListCLMapT&) = delete;

  void Clear() {
    (void)(pthread_rwlock_wrlock(&lock));
    infos.clear();
    (void)(pthread_rwlock_unlock(&lock));
  }
  void Append(LinkerMFileInfo &mplInfo, int32_t pos) {
    (void)(pthread_rwlock_wrlock(&lock));
    const MObject *classLoader = reinterpret_cast<const MObject*>(mplInfo.classLoader);
    auto &list = infos[classLoader];
    if (pos <= -1) {
      list.push_back(&mplInfo);
    } else {
      (void)list.insert(list.begin() + pos, &mplInfo);
    }
    (void)(pthread_rwlock_unlock(&lock));
  }
  template<typename UnaryFunction>
  bool FindIf(const MObject *classLoader, const UnaryFunction f) {
    (void)(pthread_rwlock_rdlock(&lock));
    auto iter = infos.find(classLoader);
    if (iter != infos.end()) {
      auto &list = iter->second;
      for (auto mplInfo : list) {
        if (f(*mplInfo)) {
          (void)(pthread_rwlock_unlock(&lock));
          return true;
        }
      }
    }
    (void)(pthread_rwlock_unlock(&lock));
    return false;
  }
  template<typename UnaryFunction>
  bool ForEach(const UnaryFunction f) {
    (void)(pthread_rwlock_rdlock(&lock));
    for (auto &item : infos) {
      auto &list = item.second;
      for (auto mplinfo : list) {
        f(*mplinfo);
      }
    }
    (void)(pthread_rwlock_unlock(&lock));
    return true;
  }
  template<typename UnaryFunction>
  bool ForEach(const MObject *classLoader, const UnaryFunction f) {
    (void)(pthread_rwlock_rdlock(&lock));
    auto iter = infos.find(classLoader);
    if (iter != infos.end()) {
      auto &list = iter->second;
      for (auto mplinfo : list) {
        f(*mplinfo);
      }
    }
    (void)(pthread_rwlock_unlock(&lock));
    return true;
  }
  void FindToExport(const MObject *classLoader, LinkerMFileInfoListT &fileList) {
    (void)(pthread_rwlock_rdlock(&lock));
    auto iter = infos.find(classLoader);
    if (iter != infos.end()) {
      fileList.Merge(iter->second);
    }
    (void)(pthread_rwlock_unlock(&lock));
  }
  bool FindListInfo(const MObject *classLoader, size_t &size, MUID &hash) {
    (void)(pthread_rwlock_rdlock(&lock));
    auto iter = infos.find(classLoader);
    if (iter != infos.end()) {
      auto &list = iter->second;
      size = list.size();
      hash = GetHash(list, false);
      (void)(pthread_rwlock_unlock(&lock));
      return true;
    }
    (void)(pthread_rwlock_unlock(&lock));
    return false;
  }

 private:
  pthread_rwlock_t lock;
  ClInfoMapT infos;
};

class LinkerMFileInfoNameMapT {
 public:
  using NameInfoMapT = std::unordered_map<std::string, LinkerMFileInfo*>;

  void Clear() {
    std::lock_guard<std::mutex> lock(mutex);
    nameInfos.clear();
  }
  void Append(LinkerMFileInfo &mplInfo) {
    std::lock_guard<std::mutex> lock(mutex);
    (void)nameInfos.insert(std::make_pair(mplInfo.name, &mplInfo));
  }
  bool Find(const std::string &name) {
    std::lock_guard<std::mutex> lock(mutex);
    return nameInfos.find(name) != nameInfos.end();
  }
  bool Find(const std::string &name, LinkerMFileInfo *&res) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = nameInfos.find(name);
    if (it != nameInfos.end()) {
      res = it->second;
      return true;
    }
    res = nullptr;
    return false;
  }
  LinkerMFileInfo *FindByDecoupleHash(const MUID &hash) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = nameInfos.begin();
    for (; it != nameInfos.end(); ++it) {
      if (it->second->hashOfDecouple == hash) {
        return it->second;
      }
    }
    return nullptr;
  }
 private:
  std::mutex mutex;
  NameInfoMapT nameInfos;
};

class LinkerMFileInfoHandleMapT {
 public:
  using HandleInfoMapT = std::unordered_map<const void*, LinkerMFileInfo*>;

  bool Find(const void *handle) {
    std::lock_guard<std::mutex> lock(mutex);
    return handleInfos.find(handle) != handleInfos.end();
  }
  bool Find(const void *handle, LinkerMFileInfo *&res) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = handleInfos.find(handle);
    if (it != handleInfos.end()) {
      res = it->second;
      return true;
    }
    res = nullptr;
    return false;
  }
  void Clear() {
    std::lock_guard<std::mutex> lock(mutex);
    handleInfos.clear();
  }
  void Append(LinkerMFileInfo &mplInfo) {
    std::lock_guard<std::mutex> lock(mutex);
    (void)handleInfos.insert(std::make_pair(mplInfo.handle, &mplInfo));
  }

 private:
  std::mutex mutex;
  HandleInfoMapT handleInfos;
};

struct ElfAddrCmp {
  using is_transparent = void; // stl::set search will use this defing, cannot remove
  const void *addr = nullptr;
  AddressRangeType searchType = kTypeText;
  bool operator()(const ElfAddrCmp *leftObj, const LinkerMFileInfo *rightObj) const {
    switch (leftObj->searchType) {
      case kTypeText:
        return leftObj->addr > rightObj->tableAddr[kJavaText][kTable1stIndex];
      case kTypeClass:
        return leftObj->addr > rightObj->tableAddr[kTabClassMetadata][kTable1stIndex];
      case kTypeData:
        return leftObj->addr > rightObj->tableAddr[kBssSection][kTable1stIndex];
      default:
        return leftObj->addr > rightObj->elfStart;
    }
  }
  bool operator()(const LinkerMFileInfo *leftObj, const ElfAddrCmp *rightObj) const {
    switch (rightObj->searchType) {
      case kTypeText:
        return leftObj->tableAddr[kJavaText][kTable1stIndex] > rightObj->addr;
      case kTypeClass:
        return leftObj->tableAddr[kTabClassMetadata][kTable1stIndex] > rightObj->addr;
      case kTypeData:
        return leftObj->tableAddr[kBssSection][kTable1stIndex] > rightObj->addr;
      default:
        return leftObj->elfStart > rightObj->addr;
    }
  }
  bool operator()(const LinkerMFileInfo *leftObj, const LinkerMFileInfo *rightObj) const {
    // NOTICE: 'tableAddr' must be initialized before here! Check InitLinkerMFileInfo()->GetValidityCode().
    // We expect the ELF files don't overlap with each other, so we just us JAVA TEXT to compare.
    return leftObj->tableAddr[kJavaText][kTable1stIndex] > rightObj->tableAddr[kJavaText][kTable1stIndex];
  }
  bool operator()(const LinkerMFileInfo *leftObj, const void *address) const {
    return leftObj->tableAddr[kJavaText][kTable1stIndex] > address;
  }
  bool operator()(const void *address, const LinkerMFileInfo *rightObj) const {
    return address > rightObj->tableAddr[kJavaText][kTable1stIndex];
  }
};
class LinkerMFileInfoElfAddrSetT {
 public:
  using InfoSetT = std::set<LinkerMFileInfo*, ElfAddrCmp>;

  void Clear() {
    std::lock_guard<std::mutex> lock(mutex);
    infos.clear();
  }

  bool Append(LinkerMFileInfo &mplInfo) {
    std::lock_guard<std::mutex> lock(mutex);
    return infos.insert(&mplInfo).second;
  }

  LinkerMFileInfo *Search(const void *pc, AddressRangeType type) {
    std::lock_guard<std::mutex> lock(mutex);
    ElfAddrCmp tmpObj;
    tmpObj.addr = pc;
    tmpObj.searchType = type;
    const void *endAddr = nullptr;
    auto iter = infos.lower_bound(&tmpObj);
    if (iter == infos.end()) {
      return nullptr;
    }
    switch (type) {
      case kTypeText:
        endAddr = (*iter)->tableAddr[kJavaText][kTable2ndIndex];
        break;
      case kTypeClass:
        endAddr = (*iter)->tableAddr[kTabClassMetadata][kTable2ndIndex];
        break;
      case kTypeData:
        endAddr = (*iter)->tableAddr[kBssSection][kTable2ndIndex];
        break;
      default:
        endAddr = (*iter)->elfEnd;
        break;
    }
    if (pc <= endAddr) {
      return *iter;
    }
    return nullptr;
  }

  LinkerMFileInfo *SearchJavaText(const void *pc) {
    if (UNLIKELY(pc == nullptr)) {
      return nullptr;
    }
    auto iter = infos.lower_bound(pc);
    if (iter != infos.end() && pc <= (*iter)->tableAddr[kJavaText][kTable2ndIndex]) {
      return *iter;
    }
    return nullptr;
  }

 private:
  std::mutex mutex;
  InfoSetT infos;
};

// Map from classloader object to loadClass method.
class MplClassLoaderLoadClassMethodMapT {
 public:
  // Map from classloader object to loadClass method.
  using ClMethodMapT = std::unordered_map<MClass*, MethodMeta*>;

  MethodMeta *Find(MClass *classLoaderClass) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = clMethodInfo.find(classLoaderClass);
    if (it != clMethodInfo.end()) {
      return it->second;
    }
    return nullptr;
  }
  void Append(MClass *classLoaderClass, MethodMeta *method) {
    std::lock_guard<std::mutex> lock(mutex);
    (void)clMethodInfo.insert(std::make_pair(classLoaderClass, method));
  }

 private:
  std::mutex mutex;
  ClMethodMapT clMethodInfo;
};
} // namespace maplert
#endif
