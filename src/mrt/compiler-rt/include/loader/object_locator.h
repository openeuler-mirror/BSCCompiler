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
#ifndef __MAPLE_LOADER_OBJECT_LOCATOR__
#define __MAPLE_LOADER_OBJECT_LOCATOR__

#include <set>
#include <list>
#include <map>
#include <cstring>
#include <mutex>

#include "object_base.h"
#include "hash_pool.h"
#include "loader_api.h"
#include "linker_api.h"

namespace maplert {
// this is for using class-name as the hash key.
struct ObjCmpStr {
  bool operator()(const std::string &a, const std::string &b) const {
    return a.compare(b) < 0;
  }
};

struct ObjFileCmp {
  bool operator()(const ObjFile *a, const ObjFile *b) const {
    if (a == nullptr) {
      return false;
    } else if (b == nullptr) {
      return true;
    }
    return a->GetUniqueID() < b->GetUniqueID();
  }
};

using ClassMapT = std::map<const std::string, MClass*, ObjCmpStr>;
using ClassMapIterT = std::map<const std::string, MClass*, ObjCmpStr>::iterator;
using ClassMapConstIterT = std::map<const std::string, MClass*, ObjCmpStr>::const_iterator;
using ObjFileHashPool = std::map<ObjFile*, MClassHashPool*, ObjFileCmp>;
// A MFileClasslocator loads class info from MFile.
class ClassLocator {
  friend class MClassLocatorInterpEx;
 public:
  ClassLocator() = default;
  ~ClassLocator() = default;
  // Load classes from MFiles
  bool LoadClasses(std::vector<ObjFile*> &objList, uint16_t clindex);
  // Load classes from one MFile
  bool LoadClasses(ObjFile &objFile, uint16_t clindex);
  MClass *InquireClass(const std::string &internalName, SearchFilter &filter);
  uint32_t GetClassCount(const ObjFile &objFile);
  void UnloadClasses();
  void VisitClasses(const maple::rootObjectFunc &f, bool isBoot = false);

  bool GetClassNameList(const ObjFile &objFile, std::vector<std::string> &classVec);
  void UnregisterDynamicClassImpl(const ClassMapIterT &it) const;
  // from classes loaded by this class-loader
  bool RegisterDynamicClass(const std::string &className, MClass &classObj);
  // MFileClassloader manage dynamic class info
  bool UnregisterDynamicClass(const std::string &className);
  // Unregister all dynamic classes
  void UnregisterDynamicClasses();
  size_t GetLoadedClassCount() {
    maple::SpinAutoLock lock(spLock);
    return mLoadedClassCount;
  }
  size_t GetClassHashMapSize();

  bool IsLinked() const {
    return mLinked;
  }

  void SetLinked(bool isLinked) {
    mLinked = isLinked;
  }
 protected:
  bool InitClasses(std::vector<ObjFile*> &objList, MClassHashPool &pool);
  void InitClasses(const ObjFile &objFile, MClassHashPool &pool);
  void SetClasses(std::vector<ObjFile*> &objList, MClassHashPool &pool, uint16_t clIndex);
  void SetClasses(const ObjFile &objFile, MClassHashPool &pool, uint16_t clIndex);
 private:
  // Find a loaded class from mpl(all) or dex(on-demand) in the order of mhashpools.
  MClass *FindClassInternal(const std::string &className, SearchFilter &filter);
  // Find a loaded class from mpl(all) and dex(on-demand), or created array class in runtime.
  MClass *FindLoadedClass(const MClassHashPool &pool, const std::string &className, SearchFilter &filter);
  MClass *FindRuntimeClass(const std::string &mplClassName);
  MClass *CreateArrayClassRecursively(const std::string &mplClassName, SearchFilter &filter);

  maple::SpinLock spLock;
  ObjFileHashPool mHashPools;
  pthread_rwlock_t dClassMapLock = PTHREAD_RWLOCK_INITIALIZER;
  ClassMapT dClassMap; // map for dynamic classInfo
  size_t mLoadedClassCount = 0;
  bool mLinked = false;
};
} // namespace maplert
#endif // __MAPLE_LOADER_OBJECT_LOCATOR__
