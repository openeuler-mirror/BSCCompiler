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
#ifndef __MAPLE_LOADER_FILE_ADAPTER__
#define __MAPLE_LOADER_FILE_ADAPTER__

#include <string>

#include "linker/linker_common.h"
#include "utils/string_utils.h"
#include "allocator/page_allocator.h"
#include "utils/time_utils.h"
namespace maplert {
enum FileType {
  kMFile,
  kDFile,
  kUnknown
};
// file manager for (all) maple supported class-file types
// one instance per class-file
class ObjFile {
 public:
  ObjFile(jobject classLoader, const std::string &path, FileType fileType)
      : fileAddr(nullptr),
        classLoader(classLoader),
        name(path),
        fileType(fileType),
        fileSize(0),
        lazyBinding(false),
        classTableLoaded(false),
        mplInfo(nullptr) {
    static int32_t globalUniqueKey = 0x100000;
    uniqueID = __atomic_add_fetch(&globalUniqueKey, 1, __ATOMIC_ACQ_REL);
  }

  virtual bool Open() = 0;
  virtual bool Close() = 0;
  virtual void Load() = 0;

  // check if the jni class in the maple file by jni class name
  virtual bool CanRegisterNativeMethods(const std::string &jniClassName) = 0;
  // if the register success, it will return true, othewise false;
  virtual bool RegisterNativeMethods(IEnv env, jclass javaClass, const std::string &jniClassName,
      INativeMethod methods, int32_t methodCount, bool fake) = 0;
  virtual void DumpUnregisterNativeFunc(std::ostream &os) = 0;

  virtual void *GetHandle() const { // different File manager might use different handle
    return fileAddr;
  }

  virtual ~ObjFile() {
    fileAddr = nullptr;
    classLoader = nullptr;
    mplInfo = nullptr;
  };

  inline int32_t GetUniqueID() const {
    return uniqueID;
  }

  inline LinkerMFileInfo *GetMplInfo() const {
    return mplInfo;
  }

  inline void SetMplInfo(LinkerMFileInfo &outMplInfo) {
    mplInfo = &outMplInfo;
  }
  inline const std::string &GetName() const {
    return name;
  }

  inline const jobject &GetClassLoader() const {
    return classLoader;
  }

  inline void SetClassLoader(jobject loader) {
    classLoader = loader;
  }

  inline void SetUniqueID(int32_t id) {
    this->uniqueID = id;
  }

  inline FileType GetFileType() const {
    return fileType;
  }

  inline size_t GetFileSize() const {
    return fileSize;
  };

  inline bool IsLazyBinding() const {
    return lazyBinding;
  }

  inline void SetLazyBinding() {
    lazyBinding = true;
  }

  inline bool IsClassTableLoaded() const {
    return classTableLoaded;
  }

  inline void SetClassTableLoaded() {
    classTableLoaded = true;
  }

  void *fileAddr;         // dlopen/mmaped address

 protected: // need to be accessed by derived class
  jobject classLoader;    // class loader to load classes in the file
  int32_t uniqueID;        // use createTime to sort
  std::string name;       // name of class file to open
  FileType fileType;      // type of file
  size_t fileSize;        // file size
  bool lazyBinding;       // it's lazy binding file.
  bool classTableLoaded;  // whether the class table has been loaded
  LinkerMFileInfo *mplInfo; // memory map for objfile
};
class FileAdapter {
 public:
  template<class Key, class Value>
  using ObjFileMap = std::unordered_map<Key, Value, std::hash<Key>, std::equal_to<Key>,
      StdContainerAllocator<std::pair<const Key, Value>, kClassLoaderAllocator>>;
  FileAdapter() {};
  MRT_EXPORT FileAdapter(std::string srcPath);
  MRT_EXPORT FileAdapter(std::string srcPath, IAdapterEx adapter);

  ~FileAdapter() {
    mMplLibs.clear();
    mMplSeqList.clear();
  }
  size_t GetRegisterSize() const {
    std::lock_guard<std::mutex> lock(mMplLibLock);
    return mMplLibs.size();
  }
  size_t GetSize() const {
    return mMplSeqList.size();
  }
  const std::vector<const ObjFile*> &GetMplFiles() const {
    return mMplSeqList;
  }
  const std::string &GetOriginPath() const {
    return originalPath;
  }
  const std::string &GetConvertPath() const {
    return convertPath;
  }
  bool IsThirdApp() const {
    return isThirdApp;
  }
  bool IsPartialAot() const {
    return isPartialAot;
  }
  bool HasStartUp() const {
    return hasStartUp;
  }
  bool HasSiblings() const {
    return hasSiblings;
  }
  void SetInterpExAPI(const IAdapterEx api) {
    pAdapterEx = api;
  }
  ObjFile *OpenObjectFile(jobject classLoader, bool isFallBack = false, const std::string specialPath = "");
  bool CloseObjectFile(ObjFile &objFile);
  void GetObjFiles(std::vector<const ObjFile*> &bootClassPath);
  void GetObjLoaders(std::set<jobject> &classLoaders);
  const ObjFile *Get(const std::string &path);
  void Put(const std::string &path, const ObjFile &objFile);
  bool Register(IEnv env, jclass javaClass, const std::string &jniClassName,
      INativeMethod methods, int32_t methodCount, bool fake);
  void DumpUnregisterNativeFunc(std::ostream &os);
  void DumpMethodName();
  // Get MFile list from filePath, and store split file path into pathList
  // returns: true if "startup MFile" found in the filePath false if not found in the filePath
  void GetObjFileList(std::vector<std::string> &pathList, bool isFallBack);
 protected:
  const ObjFile *GetLocked(const std::string &path);
  bool GetObjFileListInternal(std::vector<std::string> &pathList);

  std::string originalPath;
  std::string convertPath;
  // If it is third App
  bool isThirdApp = false;
  // If it is partial AOT
  bool isPartialAot = false;
  // If it is has startup so
  bool hasStartUp = false;
  // If it is has mulito so
  bool hasSiblings = false;
  // Input dex path
  IAdapterEx pAdapterEx;
  mutable std::mutex mMplLibLock;
  // the string represents mplefile libpath
  ObjFileMap<std::string, const ObjFile*> mMplLibs;
  std::vector<const ObjFile*> mMplSeqList;
};
} // namespace maplert
#endif // __MAPLE_LOADER_FILE_ADAPTER__
