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
#ifndef __MAPLE_LOADER_API__
#define __MAPLE_LOADER_API__

#include "file_adapter.h"
#include "gc_roots.h"

namespace maplert {
#define CL_LOG(level) LOG(level) << __FUNCTION__ << "(ClassLoader), "
#define CL_VLOG(module) VLOG(module) << __FUNCTION__ << "(ClassLoader), "
#define CL_DLOG(module) DLOG(module) << __FUNCTION__ << "(ClassLoader), "
enum AdapterFileList {
  kMplFileBootList,
  kMplFileOtherList
};
class SearchFilter {
 public:
  jobject specificCL = nullptr; // assign by 'User', immutable
  jobject contextCL = nullptr; // assign by 'Module',  low mutable
  jobject currentCL = nullptr; // assign by 'Condition', high mutable
  ObjFile *outFile = nullptr;
  ObjFile *specificFile = nullptr; // assign by 'User', immutable
  ObjFile *currentFile = nullptr; // assign by 'Condition', high mutable
  bool isInternalName = false;
  bool isLowerDelegate = false;
  bool isDelegateLast = false;
  bool ignoreBootSystem = false;
  SearchFilter() {};
  SearchFilter(jobject loader) : specificCL(loader) {};
  SearchFilter(jobject specific, jobject context) : specificCL(specific), contextCL(context) {};
  SearchFilter(jobject loader, bool lowerDelegate, ObjFile *file)
      : specificCL(loader),
        specificFile(file),
        isLowerDelegate(lowerDelegate) {};
  SearchFilter(jobject loader, bool internalName, bool lowerDelegate, ObjFile *file)
      : specificCL(loader),
        specificFile(file),
        isInternalName(internalName),
        isLowerDelegate(lowerDelegate) {};
  SearchFilter(jobject loader, bool internalName, bool lowerDelegate, bool delegateLast)
      : specificCL(loader),
        isInternalName(internalName),
        isLowerDelegate(lowerDelegate),
        isDelegateLast(delegateLast) {};
  SearchFilter(const SearchFilter &filter)
      : specificCL(filter.specificCL),
        specificFile(filter.specificFile),
        isInternalName(filter.isInternalName),
        isLowerDelegate(filter.isLowerDelegate),
        isDelegateLast(filter.isDelegateLast),
        ignoreBootSystem(filter.ignoreBootSystem) {};
  ~SearchFilter() {
    outFile = nullptr;
    specificFile = nullptr;
    currentFile = nullptr;
    specificCL = nullptr;
    currentCL = nullptr;
    contextCL = nullptr;
  }
  inline SearchFilter &Clear() {
    currentFile = nullptr;
    currentCL = nullptr;
    return *this;
  }
  inline SearchFilter &Reset() {
    currentFile = specificFile;
    currentCL = contextCL;
    return *this;
  }
  inline SearchFilter &ClearFile() {
    currentFile = nullptr;
    return *this;
  }
  inline SearchFilter &ResetFile() {
    currentFile = specificFile;
    return *this;
  }
  inline SearchFilter &ResetClear() {
    currentFile = nullptr;
    currentCL = contextCL;
    return *this;
  }
  inline bool IsBootOrSystem(const jobject systemLoader) const {
    return (contextCL == nullptr) || (contextCL == systemLoader);
  }
  inline bool IsNullOrSystem(const jobject systemLoader) const {
    return (systemLoader == nullptr) || (contextCL == systemLoader);
  }
  inline void Dump(const std::string &msg, const jclass klass) {
    CL_LOG(INFO) << msg << ", isInternalName=" << isInternalName << ", isLowerDelegate=" << isLowerDelegate <<
        ", isDelegateLast=" << isDelegateLast << ", ignoreBootSystem=" << ignoreBootSystem << ", specificCL=" <<
        specificCL << ", contextCL=" << contextCL << ", currentCL=" << currentCL << ", outFile=" << outFile <<
        ", specificFile=" << specificFile << ", currentFile=" << currentFile << ", klass=" << klass << maple::endl;
  }
};
class LoaderAPI {
 public:
  MRT_EXPORT static LoaderAPI &Instance();
  template<typename T>
  static T As() {
    return reinterpret_cast<T>(Instance());
  }
  LoaderAPI() {};
  virtual ~LoaderAPI() {};
  virtual void PreInit(IAdapterEx interpEx) = 0;
  virtual void PostInit(jobject systemClassLoader) = 0;
  virtual void UnInit() = 0;
  virtual jobject GetCLParent(jobject classLoader) = 0;
  virtual void SetCLParent(jobject classLoader, jobject parentClassLoader) = 0;
  virtual bool IsBootClassLoader(jobject classLoader) = 0;
  virtual bool LoadMplFileInBootClassPath(const std::string &pathString) = 0;
#ifndef __ANDROID__
  virtual bool LoadMplFileInUserClassPath(const std::string &pathString) = 0;
#endif
  virtual bool LoadMplFileInAppClassPath(jobject classLoader, FileAdapter &adapter) = 0;
  virtual bool LoadClasses(jobject classLoader, ObjFile &objFile) = 0;
  virtual void RegisterMplFile(const ObjFile &mplFile) = 0;
  virtual bool UnRegisterMplFile(const ObjFile &mplFile) = 0;
  virtual bool RegisterJniClass(IEnv env, jclass javaClass, const std::string &filterName,
      const std::string &jniClassName, INativeMethod methods, int32_t methodCount, bool fake) = 0;
  virtual jclass FindClass(const std::string &className, const SearchFilter &filter) = 0;
  // get registered mpl file by exact path-name: the path should be canonicalized
  virtual const ObjFile *GetMplFileRegistered(const std::string &name) = 0;
  virtual const ObjFile *GetAppMplFileRegistered(const std::string &package) = 0;
  virtual size_t GetListSize(AdapterFileList type) = 0;
  virtual size_t GetLoadedClassCount() = 0;
  virtual size_t GetAllHashMapSize() = 0;
  virtual void DumpUnregisterNativeFunc(std::ostream &os) = 0;
  virtual void VisitClasses(maple::rootObjectFunc &func) = 0;
  virtual bool IsLinked(jobject classLoader) = 0;
  virtual void SetLinked(jobject classLoader, bool isLinked) = 0;
  virtual bool GetClassNameList(jobject classLoader, ObjFile &objFile, std::vector<std::string> &classVec) = 0;
  virtual bool GetMappedClassLoaders(const jobject classLoader,
      std::vector<std::pair<jobject, const ObjFile*>> &mappedPairs) = 0;
  virtual bool GetMappedClassLoader(const std::string &fileName, jobject classLoader, jobject &realClassLoader) = 0;
  virtual void ReTryLoadClassesFromMplFile(jobject classLoader, ObjFile &mplFile) = 0;
  virtual bool RegisterNativeMethods(ObjFile &objFile, jclass klass, INativeMethod methods, int32_t methodCount) = 0;
  // MRT_EXPORT Split
  virtual void VisitGCRoots(const RefVisitor &visitor) = 0;
  virtual jobject GetSystemClassLoader() = 0;
  virtual jobject GetBootClassLoaderInstance() = 0;
  virtual void SetClassCL(jclass klass, jobject classLoader) = 0;
  virtual IObjectLocator GetCLClassTable(jobject classLoader) = 0;
  virtual void SetCLClassTable(jobject classLoader, IObjectLocator classLocator) = 0;
  virtual jclass LocateClass(const std::string &className, const SearchFilter &filter) = 0;
  virtual IAdapterEx GetAdapterEx() = 0;
  virtual void ResetCLCache() = 0;
  virtual jclass GetCache(const jclass contextClass, const std::string &className, uint32_t &index, bool&) = 0;
  virtual void WriteCache(const jclass klass, const jclass contextClass, uint32_t index) = 0;
 protected:
  static LoaderAPI *pInstance;
};
class IAdapterExObj {
 public:
  IAdapterExObj() = default;
  virtual ~IAdapterExObj() = default;
  virtual bool IsSystemServer() = 0;
  virtual bool IsGcOnly() = 0;
  virtual bool IsStarted() = 0;
};
template<typename Type>
class AdapterExObj : public IAdapterExObj {
 public:
  using TypeFlagCall = bool (Type::*)() const;
  AdapterExObj(Type &obj, TypeFlagCall api) {
    this->obj = &obj;
    this->isSystemServer = api;
  }
  ~AdapterExObj() {
    obj = nullptr;
    isSystemServer = nullptr;
  }
  bool IsSystemServer() {
    return (obj->*isSystemServer)();
  }
  bool IsGcOnly() {
    return (obj->*isGcOnly)();
  }
  bool IsStarted() {
    return (obj->*isStarted)();
  }
 protected:
  Type *obj;
  union {
    TypeFlagCall isSystemServer;
    TypeFlagCall isGcOnly;
    TypeFlagCall isStarted;
  };
};
class AdapterExAPI {
 public:
  AdapterExAPI()
      : openDexFile(nullptr),
        openMplFile(nullptr),
        enableMygote(nullptr),
        getDexFileList(nullptr),
        createThread(nullptr),
        isSystemServer(nullptr),
        isGcOnly(nullptr),
        isStarted(nullptr) {};
  ~AdapterExAPI() {
    openDexFile = nullptr;
    openMplFile = nullptr;
    enableMygote = nullptr;
    getDexFileList = nullptr;
    createThread = nullptr;
    delete isSystemServer;
    delete isGcOnly;
    delete isStarted;
    isSystemServer = nullptr;
    isGcOnly = nullptr;
    isStarted = nullptr;
  }
  using MplFileOpenCall = ObjFile *(*)(jobject classLoader, const std::string &path);
  using LoadMplFileCall = bool (*)(LoaderAPI &loader, ObjFile &mplFile, jobject classLoader);
  using EnableMygoteCall = int (*)(bool enable, const std::string &message);
  using GetDexFileListCall = void (*)(const std::string &fileName, std::vector<std::string> &fileList);
  using CreateThreadCall = void (*)(void* (*loadingCallback)(void*), void* args);
  using DexFileOpenCall = MplFileOpenCall;
  void RegisterOpenDexFileAPI(DexFileOpenCall api) {
    openDexFile = api;
  }
  void RegisterOpenMplFileAPI(MplFileOpenCall api) {
    openMplFile = api;
  }
  void RegisterEnableMygoteAPI(EnableMygoteCall api) {
    enableMygote = api;
  }
  void RegisterGetDexFileListAPI(GetDexFileListCall api) {
    getDexFileList = api;
  }
  void RegisterCreateThreadAPI(CreateThreadCall api) {
    createThread = api;
  }
  template<typename Type>
  void RegisterIsSystemServerAPI(AdapterExObj<Type> &obj) {
    isSystemServer = &obj;
  }
  template<typename Type>
  void RegisterIsGconlyAPI(AdapterExObj<Type> &obj) {
    isGcOnly = &obj;
  }
  template<typename Type>
  void RegisterIsStartedAPI(AdapterExObj<Type> &obj) {
    isStarted = &obj;
  }
  ObjFile *OpenDexFile(jobject classLoader, const std::string &path) const {
    return (*openDexFile)(classLoader, path);
  }
  ObjFile *OpenMplFile(jobject classLoader, const std::string &path) const {
    return (*openMplFile)(classLoader, path);
  }
  int EnableMygote(bool enable, const std::string &message) const {
    return (*enableMygote)(enable, message);
  }
  void GetListOfDexFileToLoad(const std::string &fileName, std::vector<std::string> &fileList) const {
    (*getDexFileList)(fileName, fileList);
  }
  void CreateThreadAndLoadFollowingClasses(void* (*loadingCallback)(void*), void* args) const {
    (*createThread)(loadingCallback, args);
  }
  bool IsSystemServer() const {
    return isSystemServer->IsSystemServer();
  }
  bool IsGcOnly() const {
    return isGcOnly->IsGcOnly();
  }
  bool IsStarted() const {
    return isStarted->IsStarted();
  }
 protected:
  MplFileOpenCall openDexFile;
  MplFileOpenCall openMplFile;
  EnableMygoteCall enableMygote;
  GetDexFileListCall getDexFileList;
  CreateThreadCall createThread;
  IAdapterExObj *isSystemServer;
  IAdapterExObj *isGcOnly;
  IAdapterExObj *isStarted;
};
} // namespace maplert
#endif // __MAPLE_LOADER_API__
