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
#include "loader/object_locator.h"

#include <cinttypes>
#include <algorithm>

#include "sizes.h"
#include "allocator.h"
#include "itab_util.h"
#include "loader/hash_pool.h"
#include "loader/object_loader.h"
using namespace std;

namespace maplert {
// Load classes for all .so files in list.
bool ClassLocator::LoadClasses(vector<ObjFile*> &objList, uint16_t clIndex) {
  maple::SpinAutoLock lock(spLock);
  // Firstly get total classes count in all .so
  uint32_t total = 0;
  for (auto objFile : objList) {
    uint32_t count = GetClassCount(*objFile);
    if (count == 0) {
      return false;
    }
    total += count;

    CL_VLOG(classloader) << "Class Count of " << objFile->GetName() << ":" << count << maple::endl;
  }

  mLoadedClassCount += total;
  CL_VLOG(classloader) << "Class Total Count:" << total << maple::endl;

  // Initialize all class's class loader info. when .so is loaded.
  MClassHashPool *pool;
  pool = new (std::nothrow) MClassHashPool();
  if (pool == nullptr) {
    CL_LOG(FATAL) << "new MClassHashPool failed" << maple::endl;
  }
  pool->Create(total);
  // For performance we construct only one MClassHashPool for boot/system classloader.
  // App can't reflect class from boot classloader's maple file.
  // So we set handle to nullptr.
  if (mHashPools.find(nullptr) != mHashPools.end()) {
    CL_LOG(FATAL) << "Attempt to Add Multi Pool to Same Boot Handle!" << maple::endl;
  }
  mHashPools[nullptr] = pool;
  if (InitClasses(objList, *pool)) {
    pool->Collect();
    SetClasses(objList, *pool, clIndex);
  }
  CL_VLOG(classloader) << "LoadClasses(), Class Hash Conflict Rate:" << pool->GetHashConflictRate() << "/" <<
      pool->GetClassConflictRate() << ", Total Memory Cost=" << pool->CalcMemoryCost() << maple::endl;
  return true;
}

// Load classes for single .so file.
bool ClassLocator::LoadClasses(ObjFile &objFile, uint16_t clIndex) {
  maple::SpinAutoLock lock(spLock);
  // Firstly get class count in .so
  uint32_t count = GetClassCount(objFile);
  if (count == 0) {
    return false;
  }
  mLoadedClassCount += count;
  CL_VLOG(classloader) << "Class Counts of " << objFile.GetName() << ":" << count << maple::endl;

  // Initialize all class's class loader info. when .so is loaded.
  MClassHashPool *pool;
  pool = new (std::nothrow) MClassHashPool();
  if (pool == nullptr) {
    LINKER_LOG(FATAL) << "new MClassHashPool failed" << maple::endl;
  }
  pool->Create(count);
  if (objFile.GetUniqueID() == 0) {
    auto min = min_element(mHashPools.begin(), mHashPools.end(), [](const auto &left, const auto &right) {
      if (left.first == nullptr) {
        return false;
      } else if (right.first == nullptr) {
        return true;
      }
      return left.first->GetUniqueID() < right.first->GetUniqueID();
    });
    if (min != mHashPools.end() && min->first != nullptr) {
      objFile.SetUniqueID(min->first->GetUniqueID() - 1);
    }
  }
  if (mHashPools.find(&objFile) != mHashPools.end()) {
    CL_LOG(FATAL) << "Attempt to Add Multi Pool to Same Handle, objFile=" << &objFile << maple::endl;
  }
  mHashPools[&objFile] = pool;
  InitClasses(objFile, *pool);
  pool->Collect();
  SetClasses(objFile, *pool, clIndex);
  CL_VLOG(classloader) << " conflict:" << pool->GetHashConflictRate() << "/" << pool->GetClassConflictRate() <<
      ", for " << objFile.GetName() << ", Total Memory Cost=" << pool->CalcMemoryCost() << maple::endl;

  return true;
}

void ClassLocator::SetClasses(vector<ObjFile*> &objList, MClassHashPool &pool, uint16_t clIndex) {
  for (auto objFile : objList) {
    SetClasses(*objFile, pool, clIndex);
  }
}

void ClassLocator::SetClasses(const ObjFile &objFile, MClassHashPool &pool, uint16_t clIndex) {
  DataRefOffset *data = objFile.GetMplInfo()->GetTblBegin<DataRefOffset*>(kClassMetadataBucket);
  uint32_t size = static_cast<uint32_t>(objFile.GetMplInfo()->GetTblEnd<DataRefOffset*>(kClassMetadataBucket) - data);
  bool isLazy = objFile.GetMplInfo()->IsFlag(kIsLazy);
  for (uint32_t i = 0; i < size; ++i) {
    DataRefOffset *klass = data + i;
    MClass *classInfo = klass->GetDataRef<MClass*>();
    pool.Set(static_cast<uint32_t>(classInfo->GetMonitor()), *classInfo);
    classInfo->SetMonitor(0);
    classInfo->ResolveVtabItab();
    if (!isLazy) {
      classInfo->SetClIndex(clIndex);
      // Set classInfo->shadow for not lazy binding.
      MRTSetMetadataShadow(reinterpret_cast<ClassMetadata*>(classInfo), WellKnown::GetMClassClass());
    }
    MRT_SetFastAlloc(reinterpret_cast<ClassMetadata*>(classInfo));
    JSAN_ADD_CLASS_METADATA(classInfo);
  }
}

// Initialize all classinfos for all .so in list.
bool ClassLocator::InitClasses(vector<ObjFile*> &objList, MClassHashPool &pool) {
  uint32_t count = 0;
  for (auto objFile : objList) {
    InitClasses(*objFile, pool);
    ++count;
  }
  return count > 0;
}

// Initialize all classinfos in single .so.
void ClassLocator::InitClasses(const ObjFile &objFile, MClassHashPool &pool) {
  DataRefOffset *data = objFile.GetMplInfo()->GetTblBegin<DataRefOffset*>(kClassMetadataBucket);
  uint32_t size = static_cast<uint32_t>(objFile.GetMplInfo()->GetTblEnd<DataRefOffset*>(kClassMetadataBucket) - data);
  for (uint32_t i = 0; i < size; ++i) {
    const DataRefOffset *klass = data + i;
    MClass *classInfo = klass->GetDataRef<MClass*>();
    pool.InitClass(static_cast<uint32_t>(classInfo->GetMonitor()));
  }
}

uint32_t ClassLocator::GetClassCount(const ObjFile &objFile) {
  DataRefOffset *hashTabStart = objFile.GetMplInfo()->GetTblBegin<DataRefOffset*>(kClassMetadataBucket);
  DataRefOffset *hashTabEnd = objFile.GetMplInfo()->GetTblEnd<DataRefOffset*>(kClassMetadataBucket);
  if ((hashTabStart == nullptr) || (hashTabEnd == nullptr)) {
    return 0;
  }
  return static_cast<uint32_t>(hashTabEnd - hashTabStart);
}

void ClassLocator::UnloadClasses() {
  maple::SpinAutoLock lock(spLock);

  for (auto item : mHashPools) {
    MClassHashPool *pool = item.second;
    if (pool == nullptr) {
      continue;
    }
    pool->Destroy();
    delete pool;
    pool = nullptr;
  }
  UnregisterDynamicClasses();
}

void ClassLocator::VisitClasses(const maple::rootObjectFunc &func, bool isBoot) {
  // Remove lock to avoid dead lock with FindClass call.
  for (auto item : mHashPools) {
    MClassHashPool *pool = item.second;
    pool->VisitClasses(func);
  }

  (void)(pthread_rwlock_rdlock(&dClassMapLock));
  for (ClassMapConstIterT it2 = dClassMap.begin(); it2 != dClassMap.end(); ++it2) {
    func(reinterpret_cast<maple::address_t>(it2->second));
  }
  (void)(pthread_rwlock_unlock(&dClassMapLock));

  if (isBoot) { // boot class loader?
    LoaderAPI::As<ObjectLoader&>().VisitPrimitiveClass(func);
  }
}

bool ClassLocator::GetClassNameList(const ObjFile &objFile, vector<std::string> &classVec) {
  maple::SpinAutoLock lock(spLock);

  LinkerMFileInfo *mplInfo = objFile.GetMplInfo();
  DataRefOffset *hashTabStart = mplInfo->GetTblBegin<DataRefOffset*>(kClassMetadataBucket);
  DataRefOffset *hashTabEnd = mplInfo->GetTblEnd<DataRefOffset*>(kClassMetadataBucket);
  if ((hashTabStart == nullptr) || (hashTabEnd == nullptr)) {
    return false;
  }
  DataRefOffset *data = hashTabStart;
  uint32_t size = static_cast<uint32_t>(hashTabEnd - hashTabStart);

  for (uint32_t i = 0; i < size; ++i) {
    DataRefOffset *klass = data + i;
    if (klass != nullptr) {
      MClass *classInfo = klass->GetDataRef<MClass*>();
      classVec.emplace_back(classInfo->GetName());
    }
  }
  return true;
}

// Find a class by class name.
// For array class, if no loaded class found, create a array class and return.
MClass *ClassLocator::InquireClass(const std::string &internalName, SearchFilter &filter) {
  // Return null if it's void array such as "[V".
  if (internalName[0] == '[') {
    uint32_t dim = 0;
    while (internalName[dim] == '[') {
      ++dim;
    }
    if (internalName[dim] == 'V') {
      return nullptr;
    }
  }
  MClass *classInfo = FindClassInternal(internalName, filter);
  if (classInfo != nullptr) {
    JSAN_ADD_CLASS_METADATA(classInfo);
    return classInfo;
  }

  // Try to create a array classe
  if (internalName[0] == '[') {
    classInfo = CreateArrayClassRecursively(internalName, filter);
  }
  JSAN_ADD_CLASS_METADATA(classInfo);
  return classInfo;
}

MClass *ClassLocator::FindLoadedClass(const MClassHashPool &pool, const std::string &className, SearchFilter &filter) {
  MClass *classInfo = nullptr;
  ClassMetadata *metaData = nullptr;
  // Find it in hashpool first.
  classInfo = pool.Get(className);
  // If no class found, we find whether it is dex in the dexFiles.
  if (classInfo == nullptr && filter.currentFile != nullptr && filter.currentFile->GetFileType() == FileType::kDFile) {
    metaData = MClassLocatorInterpEx::BuildClassFromDex(this, className, filter);
    classInfo = reinterpret_cast<MClass*>(metaData);
  }

  if (classInfo != nullptr) {
    filter.outFile = filter.currentFile;
  }
  return classInfo;
}

// Find class without creating array class.
// Here we can find the class in mpl, dex, or created array class.
MClass *ClassLocator::FindClassInternal(const std::string &className, SearchFilter &filter) {
  maple::SpinAutoLock lock(spLock);
  MClass *classInfo = nullptr;
  if (className[0] != '[') {
    // Traversal in all class hash map.
    if (filter.currentFile != nullptr) {
      auto item = mHashPools.find(filter.currentFile);
      if (item == mHashPools.end()) {
        return classInfo;
      }
      MClassHashPool *currentPool = item->second;
      classInfo = FindLoadedClass(*currentPool, className, filter);
    } else {
      for (auto item : mHashPools) {
        filter.currentFile = item.first;
        classInfo = FindLoadedClass(*item.second, className, filter);
        if (classInfo != nullptr) {
          return classInfo;
        }
      }
    }
  }
  // It may be created by an array class or by another runtime class, such as proxy/annotation classes.
  if (classInfo == nullptr) {
    classInfo = FindRuntimeClass(className);
  }
  return classInfo;
}

MClass *ClassLocator::CreateArrayClassRecursively(const std::string &mplClassName, SearchFilter &filter) {
  uint32_t dim = 0;
  while (mplClassName[dim] == '[') {
    ++dim;
  }

  // find the first component class defined. don't recursive
  MClass *componentClass = nullptr;
  uint32_t currentDim;
  for (currentDim = 1; currentDim < dim; ++currentDim) {
    componentClass = FindClassInternal(&mplClassName[currentDim], filter);
    if (componentClass != nullptr) {
      break;
    }
  }

  // The non-array element class type
  if (componentClass == nullptr) {
    componentClass = FindClassInternal(&mplClassName[dim], filter);
  }

  // Cannot find the element class
  if (componentClass == nullptr) {
    return static_cast<MClass*>(nullptr);
  }

  if (componentClass->IsLazyBinding()) {
    (void)LinkerAPI::Instance().LinkClassLazily(*componentClass);
  }

  MClass *currentClass = nullptr;
  for (int i = currentDim - 1; i >= 0; --i) {
    maple::SpinAutoLock lock(spLock);
    currentClass = FindRuntimeClass(&mplClassName[static_cast<uint32_t>(i)]);
    if (currentClass != nullptr) { // Current class already registered
      componentClass = currentClass;
      continue;
    } else { // Not registered
      currentClass = LoaderAPI::As<ObjectLoader&>().CreateArrayClass(
          &mplClassName[static_cast<uint32_t>(i)], *componentClass);
      if (currentClass != nullptr) {
        (void)!RegisterDynamicClass(&mplClassName[static_cast<uint32_t>(i)], *currentClass);
        // No memory leak happens, 'cause it's reserved by RegisterDynamicClass().
        componentClass = currentClass;
      } else {
        break;
      }
    }
  }
  return currentClass;
}

bool ClassLocator::RegisterDynamicClass(const std::string &className, MClass &classObj) {
  if (FindRuntimeClass(className) != nullptr) {
    return true;
  }
  (void)(pthread_rwlock_wrlock(&dClassMapLock));
  if ((dClassMap.emplace(std::pair<const std::string, MClass*>(className, &classObj))).second != true) {
    CL_LOG(ERROR) << "dClassMap insert not return true" << maple::endl;
    (void)(pthread_rwlock_unlock(&dClassMapLock));
    return false;
  }
  MRT_SetFastAlloc(reinterpret_cast<ClassMetadata*>(&classObj));
  (void)(pthread_rwlock_unlock(&dClassMapLock));
  JSAN_ADD_CLASS_METADATA(&classObj);
  return true;
}

void ClassLocator::UnregisterDynamicClassImpl(const ClassMapIterT &it) const {
  MClass *klass = it->second;
  if (klass != nullptr) {
    char *className = klass->GetName();
    if (className != nullptr) {
      klass->SetName(nullptr);
    }
    it->second = nullptr;
  }
}

bool ClassLocator::UnregisterDynamicClass(const std::string &className) {
  (void)(pthread_rwlock_wrlock(&dClassMapLock));
  ClassMapIterT it = dClassMap.find(className);
  if (it != dClassMap.end()) {
    UnregisterDynamicClassImpl(it);
    (void)dClassMap.erase(it);
    (void)(pthread_rwlock_unlock(&dClassMapLock));
    return true;
  }
  (void)(pthread_rwlock_unlock(&dClassMapLock));
  return false;
}

void ClassLocator::UnregisterDynamicClasses() {
  (void)(pthread_rwlock_wrlock(&dClassMapLock));
  for (ClassMapIterT it = dClassMap.begin(); it != dClassMap.end(); ++it) {
    UnregisterDynamicClassImpl(it);
  }
  dClassMap.clear();
  (void)(pthread_rwlock_unlock(&dClassMapLock));
}

// just try to get the array class
MClass *ClassLocator::FindRuntimeClass(const std::string &mplClassName) {
  MClass *classInfo = LoaderAPI::As<ObjectLoader&>().GetPrimitiveClass(mplClassName);
  if (classInfo != nullptr) {
    return classInfo;
  }
  (void)(pthread_rwlock_rdlock(&dClassMapLock));
  ClassMapConstIterT it = dClassMap.find(mplClassName);
  if (it != dClassMap.end()) {
    (void)(pthread_rwlock_unlock(&dClassMapLock));
    return it->second;
  }
  (void)(pthread_rwlock_unlock(&dClassMapLock));
  return nullptr;
}

size_t ClassLocator::GetClassHashMapSize() {
  maple::SpinAutoLock lock(spLock);
  return sizeof(MClassHashPool) * mHashPools.size();
}
} // namespace maplert
