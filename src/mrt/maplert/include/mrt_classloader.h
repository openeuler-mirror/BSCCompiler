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
#ifndef MRT_CLASS_CLASSLOADER_IMP_H_
#define MRT_CLASS_CLASSLOADER_IMP_H_

#include <vector>

#include "gc_roots.h"
#include "object_base.h"
#include "jni.h"
#include "mrt_classloader_api.h"
#include "loader/object_locator.h"
#include "loader/object_loader.h"
#include "mrt_api_common.h"
#include "linker_api.h"
namespace maplert {
class CLCache {
 private:
#ifdef USE_32BIT_REF
  static constexpr int ptrSize = 4;
  using CacheType = uint32_t;
#else
  static constexpr int ptrSize = 8;
  using CacheType = uint64_t;
#endif
  static constexpr int kCacheSize = 2 * 1024; // 2k
  static constexpr int kMutexNum = 64;
  static constexpr int kPerMutexRange = kCacheSize / kMutexNum;
  static constexpr int arraySize = kCacheSize * ptrSize / sizeof(CacheType);
  CacheType classAddrArray[arraySize] = { 0 };
  CacheType resultArray[arraySize] = { 0 };
  std::mutex cacheMtx[kMutexNum];
  std::atomic<bool> valid;
 public:
  static CLCache instance;
  CLCache() {
    valid = true;
  }
  ~CLCache() {}

  jclass GetCache(const jclass contextClass, const std::string &className, uint32_t &index, bool &lockFail) noexcept {
    size_t clNameSize = className.size();
    if (valid.load(std::memory_order_acquire)) {
      constexpr int kSeed = 31;
      constexpr int kDistance = 2;
      uint64_t hash = 0;
      for (size_t i = 0; i < clNameSize; i += kDistance) {
        hash = kSeed * hash + className[i];
      }
      hash += reinterpret_cast<address_t>(contextClass);
      index = hash % kCacheSize;
      size_t mutexIdx = index / kPerMutexRange;
      if (!cacheMtx[mutexIdx].try_lock()) {
        lockFail = true;
        return nullptr;
      }
      jclass retVal = reinterpret_cast<jclass>(static_cast<uint64_t>(resultArray[index]));
      if (classAddrArray[index] == static_cast<CacheType>(reinterpret_cast<uint64_t>(contextClass)) &&
          retVal != nullptr && className == (reinterpret_cast<MClass*>(retVal))->GetName()) {
        cacheMtx[mutexIdx].unlock();
        return retVal;
      }
      cacheMtx[mutexIdx].unlock();
    }
    return nullptr;
  }

  void WriteCache(const jclass klass, const jclass contextClass, uint32_t index) noexcept {
    if (valid.load(std::memory_order_acquire)) {
      size_t mutexIdx = index / kPerMutexRange;
      if (!cacheMtx[mutexIdx].try_lock()) {
        return;
      }
      classAddrArray[index] = static_cast<CacheType>(reinterpret_cast<uint64_t>(contextClass));
      resultArray[index] = static_cast<CacheType>(reinterpret_cast<uint64_t>(klass));
      cacheMtx[mutexIdx].unlock();
    }
  }
  void ResetCache() {
    bool expect = true;
    if (!valid.compare_exchange_strong(expect, false, std::memory_order_acq_rel) && !expect) {
      return; // other thread is Reseting, just return when cas fail
    }
    if (memset_s(classAddrArray, kCacheSize * ptrSize, 0, kCacheSize * ptrSize) != EOK) {
      LOG(FATAL) << "memset_s fail." << maple::endl;
    }
    expect = false;
    if (!valid.compare_exchange_strong(expect, true, std::memory_order_acq_rel) && expect) {
      LOG(FATAL) << "CLCache is working when reseting cache" << maple::endl;
    }
  }
};
class ClassLoaderImpl : public ObjectLoader {
 public:
  ClassLoaderImpl();
  ~ClassLoaderImpl();
  void UnInit() override;
  // API Interfaces Begin
  bool LoadMplFileInBootClassPath(const std::string &pathString) override;
#ifndef __ANDROID__
  bool LoadMplFileInUserClassPath(const std::string &pathString) override;
#endif
  bool LoadMplFileInAppClassPath(jobject classLoader, FileAdapter &adapter) override;
  void ResetCLCache() override;
  jclass GetCache(const jclass contextClass, const std::string &className, uint32_t &index, bool &lockFail) override;
  void WriteCache(const jclass klass, const jclass contextClass, uint32_t index) override;
  void RegisterMplFile(const ObjFile &mplFile) override;
  bool UnRegisterMplFile(const ObjFile &mplFile) override;
  bool RegisterJniClass(IEnv env, jclass javaClass, const std::string &filterName, const std::string &jniClassName,
      INativeMethod methods, int32_t methodCount, bool fake) override;
  jclass FindClass(const std::string &className, const SearchFilter &filter) override;
  // get registered mpl file by exact path-name: the path should be canonicalized
  const ObjFile *GetMplFileRegistered(const std::string &name) override;
  const ObjFile *GetAppMplFileRegistered(const std::string &package) override;
  size_t GetListSize(AdapterFileList type) override;
  void DumpUnregisterNativeFunc(std::ostream &os) override;
  bool GetMappedClassLoaders(
      jobject classLoader, std::vector<std::pair<jobject, const ObjFile*>> &mappedPairs) override;
  bool GetMappedClassLoader(const std::string &fileName, jobject classLoader, jobject &realClassLoader) override;
  void VisitClasses(maple::rootObjectFunc &func) override;
  bool RegisterNativeMethods(ObjFile &objFile, jclass klass, INativeMethod methods, int32_t methodCount) override;
  // MRT_EXPORT Split
  jclass LocateClass(const std::string &className, const SearchFilter &filter) override;
  // API Interfaces End
  MClass *GetPrimitiveClass(const std::string &mplClassName) override;
  void VisitPrimitiveClass(const maple::rootObjectFunc &func) override;
  MClass *CreateArrayClass(const std::string &mplClassName, MClass &componentClass) override;

 protected:
  bool SetMappedClassLoader(const std::string &fileName, MObject *classLoader, MObject *realClassLoader);
  void RemoveMappedClassLoader(const std::string &fileName);
  MClass *FindClassInSingleClassLoader(const std::string &className, SearchFilter &filter);
  MClass *LocateInCurrentClassLoader(const std::string &className, SearchFilter &filter);
  MClass *LocateInParentClassLoader(const std::string &className, SearchFilter &filter);
 private:
  MClass *DoCreateArrayClass(MClass &klass, MClass &componentClass, const std::string &name);
  void LinkStartUpAndMultiSo(std::vector<LinkerMFileInfo*> &mplInfoList, bool hasStartup);
  const std::string kAppSoPostfix = "/maple/arm64/mapleclasses.so";
  const std::string kAppPartialSoPostfix = "/maple/arm64/maplepclasses.so";
  const std::string kSystemLibPath = "/system/lib64";
  const std::string kIgnoreJarList = "core-oj.jar:core-libart.jar";
  const static uint16_t kClIndexUnInit = static_cast<uint16_t>(-1);
  // boot mplfile list
  FileAdapter mMplFilesBoot;
  // DEX classloader list
  FileAdapter mMplFilesOther;
  // Mapping classloader for multiple loading .so
  std::unordered_multimap<std::string, std::pair<MObject*, MObject*>> mMappedClassLoader;
};
} // namespace maplert
#endif // MRT_CLASS_LOCATOR_MANAGER_H_
