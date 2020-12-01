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

#include "mrt_classloader.h"

#include <cassert>
#include <cstdio>
#include <dlfcn.h>

#include "cpphelper.h"
#include "mrt_well_known.h"
#include "exception/mrt_exception.h"
#include "loader/object_locator.h"
#include "yieldpoint.h"
#include "interp_support.h"
#include "mrt_primitive_class.def"
#include "base/systrace.h"
#include "chosen.h"
namespace maplert {
LoaderAPI *LoaderAPI::pInstance = nullptr;
LoaderAPI &LoaderAPI::Instance() {
  if (pInstance == nullptr) {
    pInstance = new (std::nothrow) ClassLoaderImpl();
    if (pInstance == nullptr) {
      CL_LOG(FATAL) << "new ClassLoaderImpl failed" << maple::endl;
    }
  }
  return *pInstance;
}
ClassLoaderImpl::ClassLoaderImpl() {
  primitiveClasses.push_back(reinterpret_cast<MClass*>(&MRT_CLASSINFO(ALjava_2Flang_2FObject_3B)));
  primitiveClasses.push_back(reinterpret_cast<MClass*>(&MRT_CLASSINFO(ALjava_2Flang_2FString_3B)));
  primitiveClasses.push_back(reinterpret_cast<MClass*>(&MRT_CLASSINFO(ALjava_2Flang_2FClass_3B)));
  primitiveClasses.push_back(reinterpret_cast<MClass*>(&MRT_CLASSINFO(ALjava_2Futil_2FFormatter_24Flags_3B)));
  primitiveClasses.push_back(reinterpret_cast<MClass*>(&MRT_CLASSINFO(ALjava_2Futil_2FHashMap_24Node_3B)));
  primitiveClasses.push_back(reinterpret_cast<MClass*>(&MRT_CLASSINFO(ALjava_2Futil_2FFormatter_24FormatString_3B)));
  primitiveClasses.push_back(reinterpret_cast<MClass*>(&MRT_CLASSINFO(ALjava_2Flang_2FCharSequence_3B)));
  primitiveClasses.push_back(reinterpret_cast<MClass*>(
      &MRT_CLASSINFO(ALjava_2Flang_2FThreadLocal_24ThreadLocalMap_24Entry_3B)));
#ifdef __OPENJDK__
  primitiveClasses.push_back(reinterpret_cast<MClass*>(&MRT_CLASSINFO(ALjava_2Futil_2FHashtable_24Entry_3B)));
#else // libcore
  primitiveClasses.push_back(reinterpret_cast<MClass*>(&MRT_CLASSINFO(ALjava_2Futil_2FHashtable_24HashtableEntry_3B)));
  primitiveClasses.push_back(reinterpret_cast<MClass*>(&MRT_CLASSINFO(ALlibcore_2Freflect_2FAnnotationMember_3B)));
  primitiveClasses.push_back(reinterpret_cast<MClass*>(&MRT_CLASSINFO(ALsun_2Fsecurity_2Futil_2FDerValue_3B)));
  primitiveClasses.push_back(reinterpret_cast<MClass*>(&MRT_CLASSINFO(ALsun_2Fsecurity_2Fx509_2FAVA_3B)));
#endif // __OPENJDK__
}
ClassLoaderImpl::~ClassLoaderImpl() {
  mMappedClassLoader.clear();
}

void ClassLoaderImpl::UnInit() {
  std::set<jobject> reqMplClsLoaders;
  mMplFilesOther.GetObjLoaders(reqMplClsLoaders);
  mMplFilesBoot.GetObjLoaders(reqMplClsLoaders);
  for (auto it = reqMplClsLoaders.begin(); it != reqMplClsLoaders.end(); ++it) {
    UnloadClasses(reinterpret_cast<const MObject*>(*it));
  }

  std::vector<const ObjFile*> regMplFiles;
  mMplFilesOther.GetObjFiles(regMplFiles);
  mMplFilesBoot.GetObjFiles(regMplFiles);

  for (auto it = regMplFiles.begin(); it != regMplFiles.end(); ++it) {
    delete *it;
    *it = nullptr;
  }
  ObjectLoader::UnInit();
}

void ClassLoaderImpl::ResetCLCache() {
  CLCache::instance.ResetCache();
}

jclass ClassLoaderImpl::GetCache(const jclass contextClass, const std::string &className,
                                 uint32_t &index, bool &lockFail) {
  return CLCache::instance.GetCache(contextClass, className, index, lockFail);
}

void ClassLoaderImpl::WriteCache(const jclass klass, const jclass contextClass, uint32_t index) {
  CLCache::instance.WriteCache(klass, contextClass, index);
}

// API Interfaces Begin
void ClassLoaderImpl::RegisterMplFile(const ObjFile &objFile) {
  const ObjFile *pmf = mMplFilesOther.Get(objFile.GetName());

  if (pmf == nullptr) {
    mMplFilesOther.Put(objFile.GetName(), objFile);
  }
}

bool ClassLoaderImpl::UnRegisterMplFile(const ObjFile &objFile) {
  if (GetMplFileRegistered(objFile.GetName()) == nullptr) {
    RemoveMappedClassLoader(objFile.GetName());
    delete &objFile;
    return true;
  }
  return false;
}

bool ClassLoaderImpl::RegisterJniClass(IEnv env, jclass javaClass, const std::string &mFileName,
    const std::string &jniClassName, INativeMethod methods, int32_t methodCount, bool fake) {
  if (mFileName.empty()) {
    return mMplFilesOther.Register(env, javaClass, jniClassName, methods, methodCount, fake);
  }
  const ObjFile *mf = mMplFilesBoot.Get(mFileName);
  if (mf == nullptr) {
    mf = mMplFilesOther.Get(mFileName);
  }
  ObjFile *objFile = const_cast<ObjFile*>(mf);
  if (mf != nullptr && objFile->CanRegisterNativeMethods(jniClassName) && (objFile->RegisterNativeMethods(env,
      javaClass, jniClassName, methods, methodCount, fake) == true)) {
    return true;
  }
  return false;
}

size_t ClassLoaderImpl::GetListSize(AdapterFileList type) {
  if (type == kMplFileBootList) {
    return mMplFilesBoot.GetRegisterSize();
  } else {
    return mMplFilesOther.GetRegisterSize();
  }
}

const ObjFile *ClassLoaderImpl::GetMplFileRegistered(const std::string &name) {
  const ObjFile *pmf = mMplFilesBoot.Get(name);

  if (pmf != nullptr) {
    return pmf;
  }

  return mMplFilesOther.Get(name);
}

const ObjFile *ClassLoaderImpl::GetAppMplFileRegistered(const std::string &packageApk) {
  std::vector<const ObjFile*> files;
  mMplFilesOther.GetObjFiles(files);
  std::string package;
  std::string apkName;
  std::string::size_type index = packageApk.find("/");
  if (index == std::string::npos) {
    CL_LOG(ERROR) << "packageApk must $package_name/xx.apk " << packageApk.c_str() << maple::endl;
    return nullptr;
  } else {
    package = packageApk.substr(0, index);
    apkName = packageApk.substr(index + 1);
  }
  for (auto it = files.begin(); it != files.end(); ++it) {
    std::string soName = (*it)->GetName();
    // different installation, different so name
    // adb push: /system/priv-app/Calendar/Calendar.apk!/maple/arm64/mapleclasses.so
    // adb install: /data/app/com.android.calendar-C5VubFGVWn3V4prwdHIuHQ==/base.apk!/maple/arm64/mapleclasses.so
    if ((soName.find(kAppSoPostfix) != std::string::npos || soName.find(kAppPartialSoPostfix) != std::string::npos) &&
        (soName.find(package) != std::string::npos || soName.find(apkName) != std::string::npos)) {
      return *it;
    }
  }
  return nullptr;
}

bool ClassLoaderImpl::LoadMplFileInBootClassPath(const std::string &pathString) {
  maple::ScopedTrace trace("LoadMplFileInBootClassPath, %p", pathString.c_str());
  if (pathString.length() < 1) {
    return false;
  }
  // split class_pathString to arraylist;
  std::vector<std::string> classPaths = stringutils::Split(pathString, ':');
  // store the boot class path
  jobject classLoader = nullptr;
  // loop to process boot classpath
  std::vector<ObjFile*> objList;
  pLinker->SetLoadState(kLoadStateBoot);
  for (std::string &path : classPaths) {
    std::string jarName = path;
    size_t pos = path.rfind("/");
    if (pos != std::string::npos && pos < path.length()) {
      jarName = path.substr(pos + 1);
    }
    if (kIgnoreJarList.find(jarName) != std::string::npos) {
      continue;
    }

    FileAdapter adapter(path, pAdapterEx);
    ObjFile *objFile = adapter.OpenObjectFile(classLoader);

    if (objFile == nullptr) {
      std::string message = "Failed to dlopen " + path;
      (void)(pAdapterEx->EnableMygote(false, message));
      return false;
    }

    if (objFile->GetFileType() == FileType::kMFile) {
      if (!pLinker->Add(*objFile, classLoader)) {
        CL_LOG(ERROR) << "invalid maple file" << path << ", lazy=" << objFile->IsLazyBinding() << objFile->GetName() <<
            maple::endl;
        delete objFile;
        continue;
      }

      mMplFilesBoot.Put(objFile->GetName(), *objFile);
      objFile->Load();
      objList.push_back(objFile);
    } else {
      CL_LOG(ERROR) << "open Non maple file: " << path << ", " << objFile->GetName() << maple::endl;
      delete objFile;
      objFile = nullptr;
    }
  }
  if (!LoadClasses(reinterpret_cast<MObject*>(classLoader), objList)) {
    (void)pAdapterEx->EnableMygote(false, "Failed to load classes.");
    return false;
  }
  // For boot class loader, we just link when load so.
  (void)(pLinker->Link());
  SetLinked(classLoader, false);
  return true;
}

bool ClassLoaderImpl::LoadMplFileInAppClassPath(jobject clLoader, FileAdapter &adapter) {
  maple::ScopedTrace trace("LoadMplFileInAppClassPath, %p", adapter.GetConvertPath().c_str());
  MObject *classLoader = reinterpret_cast<MObject*>(clLoader);
  bool isFallBack = false;
  do {
    std::vector<std::string> pathList;
    adapter.GetObjFileList(pathList, isFallBack);
    // Load classes in all MplFiles and add LinkerMFileInfo
    std::vector<LinkerMFileInfo*> mplInfoList;
    for (auto path : pathList) {
      // 1. check mpl file if registered
      const ObjFile *pmfCookie = GetMplFileRegistered(path);
      if (LIKELY(pmfCookie == nullptr)) {
        // 2. open mpl file
        ObjFile *pmf = adapter.OpenObjectFile(clLoader, isFallBack, path);
        if (pmf == nullptr) {
          CL_LOG(ERROR) << "failed to open maple file " << path << ", classloader:" << classLoader << maple::endl;
          if (isFallBack) {
            MRT_ThrowNewException("java/io/IOException", path.c_str());
          }
          continue;
        }
        if (!pLinker->IsFrontPatchMode(path) && classLoader != nullptr) {
          // 3. load mpl file's classes to classloader
          if (!LoadClassesFromMplFile(classLoader, *pmf, mplInfoList, adapter.HasSiblings())) {
            delete pmf;
            CL_LOG(ERROR) << "failed to load maple file " << path << ", classloader:" << classLoader << maple::endl;
            MRT_ThrowNewException("java/io/IOException", path.c_str());
            continue;
          }
        }
        RegisterMplFile(*pmf);
        pmfCookie = pmf;
      } else if (pmfCookie->GetClassLoader() != clLoader) {
        std::string message = "Attempt to load the same maple file " + path + " in with multiple class loaders";
        CL_LOG(ERROR) << message << maple::endl;
        if (!SetMappedClassLoader(path, classLoader, reinterpret_cast<MObject*>(pmfCookie->GetClassLoader()))) {
          MRT_ThrowNewException("java/lang/InternalError", message.c_str());
          continue;
        } else if (VLOG_IS_ON(classloader)) {
          adapter.DumpMethodName();
        }
      }
      if (pmfCookie != nullptr) {
        adapter.Put(adapter.GetOriginPath(), *pmfCookie);
      }
    }
    // A workaround for class's muid failed to resolve issue when multi-so loading
    if (!isFallBack && adapter.HasSiblings()) {
      LinkStartUpAndMultiSo(mplInfoList, adapter.HasStartUp());
    }
    isFallBack = !isFallBack && (adapter.GetSize() == 0 || adapter.IsPartialAot());
  } while (isFallBack);
  return (adapter.GetSize() > 0) ? true : false;
}

#ifndef __ANDROID__
// Only for QEMU mplsh test.
bool ClassLoaderImpl::LoadMplFileInUserClassPath(const std::string &paths) {
  maple::ScopedTrace trace("LoadMplFileInUserClassPath, %p", paths.c_str());
  CL_VLOG(classloader) << paths << maple::endl;

  if (paths.length() < 1) {
    CL_LOG(ERROR) << "class path is null for SystemClassLoader!!" << maple::endl;
    return true;
  }

  // split class path string (paths) to arraylist;
  std::vector<std::string> clsPaths = stringutils::Split(paths, ':');

  MObject *classLoader = mSystemClassLoader;
  std::vector<ObjFile*> objList;
  for (auto path : clsPaths) {
    FileAdapter adapter(path, pAdapterEx);
    ObjFile *objFile = adapter.OpenObjectFile(reinterpret_cast<jobject>(classLoader));  // check for MFile first
    if (objFile != nullptr && objFile->GetFileType() == FileType::kMFile) {
      // MFile open OK. continue the linking process
      if (!pLinker->Add(*objFile, reinterpret_cast<jobject>(classLoader))) {
        CL_LOG(ERROR) << "invalid maple file or multiple loading:" << path << ", " << objFile->GetName() << maple::endl;
        delete objFile;
        continue;
      }
      mMplFilesOther.Put(path, *objFile);
      objFile->Load();
      objList.push_back(objFile);
    } else {
      objFile = adapter.OpenObjectFile(reinterpret_cast<jobject>(classLoader), true);
      // try MFile failed, then check for DFile
      if (objFile == nullptr) {  // try DFile also failed, stop loading.
        CL_LOG(ERROR) << "open " << path << "failed!" << maple::endl;
        return false;
      }
      // DFile open OK. continue the linking process
      // Note: objFile should be of type FileType::kDFile
      std::vector<LinkerMFileInfo*> infoList;
      if (!LoadClassesFromMplFile(classLoader, *objFile, infoList)) {
        // loading the classes list in this dexFile failed
        CL_LOG(ERROR) << "open " << path << "failed! size=" << infoList.size() << maple::endl;
        delete objFile;
        continue;
      }
      mMplFilesOther.Put(path, *objFile);
    }
  }
  (void)(pLinker->Link());
  SetLinked(reinterpret_cast<jobject>(classLoader), false);
  if (!LoadClasses(classLoader, objList)) {
    CL_LOG(ERROR) << "load class failed!!" << maple::endl;
    return false;
  }
  return true;
}
#endif

jclass ClassLoaderImpl::FindClass(const std::string &className, const SearchFilter &constFilter) {
  std::string javaDescriptor; // Like Ljava/lang/Object;
  MClass *klass = nullptr;
  jobject systemClassLoader = reinterpret_cast<jobject>(mSystemClassLoader);
  SearchFilter &filter = const_cast<SearchFilter&>(constFilter);
  // Like Ljava/lang/Object;
  javaDescriptor = filter.isInternalName ? GetClassNametoDescriptor(className) : className;
  if (UNLIKELY(javaDescriptor.empty())) {
    CL_LOG(ERROR) << "javaDescriptor is nullptr" << maple::endl;
  }
  filter.contextCL = IsBootClassLoader(filter.specificCL) ? nullptr : filter.specificCL;
  if (filter.isLowerDelegate && filter.contextCL != nullptr) {
    // For delegate last class loader, the search order is as below:
    //  . boot class loader
    //  . current class loader
    //  . parent class loader
    // Otherwise, the order is as below:
    //  . parent class loader (Includes boot class loader)
    //  . current class loader
    if (filter.isDelegateLast) {
      klass = LocateInCurrentClassLoader(javaDescriptor, filter.Reset());
    } else {
      klass = LocateInParentClassLoader(javaDescriptor, filter.Reset());
    }
    if (klass == nullptr && !filter.IsNullOrSystem(systemClassLoader)) {
      filter.currentCL = systemClassLoader;
      klass = reinterpret_cast<MClass*>(LocateClass(javaDescriptor, filter.ResetFile()));
    }
    if (klass == nullptr && !filter.IsBootOrSystem(systemClassLoader)) {
      klass = reinterpret_cast<MClass*>(
          LinkerAPI::Instance().InvokeClassLoaderLoadClass(filter.contextCL, javaDescriptor));
    }
  } else {
    klass = FindClassInSingleClassLoader(javaDescriptor, filter.Reset());
  }
  // It could be normal here. We are searching class by parent-delegation-model, so we may not find class in parent
  // classLoader temporarily.
  if (UNLIKELY(klass == nullptr)) {
    CL_DLOG(classloader) << "failed, classloader=" << filter.contextCL << ", cl name=" <<
        (filter.contextCL == nullptr ? "BootCL(null)" : reinterpret_cast<MObject*>(
        filter.contextCL)->GetClass()->GetName()) << ", class name=" << className << maple::endl;
    return nullptr;
  }
  MObject *pendingClassLoader = GetClassCL(klass);
  if (pendingClassLoader == nullptr && klass->IsLazyBinding()) {
    CL_VLOG(classloader) << className << ", from boot, and lazy" << maple::endl;
    SetClassCL(reinterpret_cast<jclass>(klass), filter.contextCL);
  }
  return reinterpret_cast<jclass>(klass);
}

void ClassLoaderImpl::DumpUnregisterNativeFunc(std::ostream &os) {
  if (VLOG_IS_ON(binding)) {
    CL_VLOG(classloader) << "boot class:" << maple::endl;
    mMplFilesBoot.DumpUnregisterNativeFunc(os);
    CL_VLOG(classloader) << "application class:" << maple::endl;
    mMplFilesOther.DumpUnregisterNativeFunc(os);
  }
}

void ClassLoaderImpl::VisitClasses(maple::rootObjectFunc &func) {
  std::vector<const ObjFile*> files;
  std::vector<const ObjFile*> files2;
  mMplFilesOther.GetObjFiles(files);
  mMplFilesBoot.GetObjFiles(files2);
  std::unordered_set<const MObject*> loaders;

  for (auto it = files.begin(); it != files.end(); ++it) {
    (void)(loaders.insert(reinterpret_cast<const MObject*>((*it)->GetClassLoader())));  // de-duplicate
  }

  for (auto it = files2.begin(); it != files2.end(); ++it) {
    (void)(loaders.insert(reinterpret_cast<const MObject*>((*it)->GetClassLoader())));  // de-duplicate
  }

  (void)(loaders.insert(nullptr));  // bootstrap class loader

  for (auto it = loaders.begin(); it != loaders.end(); ++it) {
    VisitClassesByLoader(*it, func);
  }
}

// Get all mapped {classLoader, mpl_file_handle} by latter classloader.
bool ClassLoaderImpl::GetMappedClassLoaders(const jobject classLoader,
    std::vector<std::pair<jobject, const ObjFile*>> &mappedPairs) {
  bool ret = false;

  for (auto it = mMappedClassLoader.begin(); it != mMappedClassLoader.end(); ++it) {
    std::string fileName = it->first;
    jobject latterClassLoader = reinterpret_cast<jobject>(it->second.first);
    jobject mappedClassLoader = reinterpret_cast<jobject>(it->second.second);

    if (classLoader == latterClassLoader) {
      const ObjFile *pmfCookies = GetMplFileRegistered(fileName);
      mappedPairs.push_back(std::make_pair(mappedClassLoader, pmfCookies));
      ret = true;
      CL_LOG(INFO) << classLoader << "->{" << mappedClassLoader << "," << fileName << "}" << maple::endl;
    }
  }
  return ret;
}

bool ClassLoaderImpl::GetMappedClassLoader(const std::string &fileName, jobject classLoader, jobject &realClassLoader) {
  auto range = mMappedClassLoader.equal_range(fileName);

  for (auto val = range.first; val != range.second; ++val) {
    if (classLoader == reinterpret_cast<jobject>(val->second.first)) {
      realClassLoader = reinterpret_cast<jobject>(val->second.second);
      CL_LOG(INFO) << "get mapped classLoader from " << classLoader << ":" << realClassLoader << " for " << fileName <<
          maple::endl;
      return true;
    }
  }

  CL_LOG(ERROR) << "failed, get mapped classLoader from " << classLoader << " for " << fileName << maple::endl;
  return false;
}

bool ClassLoaderImpl::RegisterNativeMethods(ObjFile &objFile,
    jclass klass, INativeMethod methods, int32_t methodCount) {
  return MClassLocatorManagerInterpEx::RegisterNativeMethods(*this, objFile,
      klass, methods.As<const JNINativeMethod*>(), methodCount);
}
// -----------------------MRT_EXPORT Split--------------------------------------
jclass ClassLoaderImpl::LocateClass(const std::string &className, const SearchFilter &constFilter) {
  SearchFilter &filter = const_cast<SearchFilter&>(constFilter);
  ClassLocator *classLocator = GetCLClassTable(filter.currentCL).As<ClassLocator*>();
  if (classLocator == nullptr) {
    return nullptr;
  }
  MClass *klass = classLocator->InquireClass(className, filter);
  if (klass == nullptr) {
    return nullptr;
  }

  if (klass->GetClIndex() == kClIndexUnInit) { // For lazy binding.
    SetClassCL(reinterpret_cast<jclass>(klass), filter.currentCL);
  }

  return reinterpret_cast<jclass>(klass);
}
// API Interfaces End
bool ClassLoaderImpl::SetMappedClassLoader(const std::string &fileName,
    MObject *classLoader, MObject *realClassLoader) {
  auto range = mMappedClassLoader.equal_range(fileName);

  for (auto val = range.first; val != range.second; ++val) {
    if (classLoader == val->second.first) {
      CL_LOG(ERROR) << "failed, shouldn't double map " << classLoader << " to " << realClassLoader << " for " <<
          fileName << maple::endl;
      return true;
    }
  }
  (void)(mMappedClassLoader.emplace(fileName, std::make_pair(classLoader, realClassLoader)));
  CL_LOG(INFO) << "mapped " << classLoader << " to " << realClassLoader << " for " << fileName << maple::endl;
  return true;
}

void ClassLoaderImpl::RemoveMappedClassLoader(const std::string &fileName) {
  (void)(mMappedClassLoader.erase(fileName));
}

// For delegate last class loader, the search order is as below:
//  . boot class loader
//  . current class loader
//  . parent class loader
MClass *ClassLoaderImpl::LocateInCurrentClassLoader(const std::string &className, SearchFilter &filter) {
  MClass *klass = nullptr;
  if (IsBootClassLoader(filter.contextCL)) { // Boot class loder
    klass = reinterpret_cast<MClass*>(LocateClass(className, filter.Clear()));
    return klass;
  }

  // Current class loader
  klass = reinterpret_cast<MClass*>(LocateClass(className, filter.ResetClear()));
  if (klass != nullptr) {
    return klass;
  }
  // To traverse in parents.
  filter.currentCL = GetCLParent(filter.contextCL);
  filter.ignoreBootSystem = true;
  klass = LocateInParentClassLoader(className, filter.ResetFile());
  return klass;
}

MClass *ClassLoaderImpl::LocateInParentClassLoader(const std::string &className, SearchFilter &filter) {
  MClass *klass = nullptr;
  if (IsBootClassLoader(filter.currentCL)) { // Boot class loder
    if (filter.ignoreBootSystem) { // Not to search in BootClassLoader and SystemClassLoader
      return nullptr;
    }
    klass = reinterpret_cast<MClass*>(LocateClass(className, filter.ClearFile()));
    return klass;
  }

  jobject classLoader = filter.currentCL;
  filter.currentCL = GetCLParent(classLoader);
  if ((klass = LocateInParentClassLoader(className, filter)) == nullptr) {
    filter.currentCL = classLoader;
    klass = reinterpret_cast<MClass*>(LocateClass(className, filter.ResetFile()));
  }
  return klass;
}

// Reduce cyclomatic complexity of FindClass().
// Before find the class in current classloader, check the boot firstly.
MClass *ClassLoaderImpl::FindClassInSingleClassLoader(const std::string &javaDescriptor, SearchFilter &filter) {
  MClass *klass = nullptr;
  jobject systemClassLoader = reinterpret_cast<jobject>(mSystemClassLoader);
  // If current is boot classloader, or not boot but its parent is null, we check the boot classLoader firsly.
  if (filter.contextCL == nullptr || GetCLParent(filter.contextCL) == nullptr) {
    klass = reinterpret_cast<MClass*>(LocateClass(javaDescriptor, filter.Clear()));
  }
  // check the current classloader, if it's not boot or system classloader
  if (klass == nullptr && !filter.IsBootOrSystem(systemClassLoader)) {
    // Find in current classloader.
    klass = reinterpret_cast<MClass*>(LocateClass(javaDescriptor, filter.Reset()));
  }

  // at last for the shared libraries, we check system classloader
  if (klass == nullptr && filter.contextCL != nullptr && systemClassLoader != nullptr) {
    // Find in systemclassloader.
    filter.currentCL = systemClassLoader;
    klass = reinterpret_cast<MClass*>(LocateClass(javaDescriptor, filter.ClearFile()));
  }
  return klass;
}

void ClassLoaderImpl::LinkStartUpAndMultiSo(std::vector<LinkerMFileInfo*> &mplInfoList, bool hasStartup) {
  if (mplInfoList.size() > 0) {  // We always load the first one, no matter is startup or not.
    (void)(pLinker->Link(*(mplInfoList[0]), false));
  }
  for (size_t i = 1; i < mplInfoList.size(); ++i) {
    auto mplInfo = mplInfoList[i];
    void *param[] = { reinterpret_cast<void*>(pLinker), reinterpret_cast<void*>(mplInfo) };
    if (hasStartup) {
      pAdapterEx->CreateThreadAndLoadFollowingClasses([](void *data)->void* {
          void **p =  reinterpret_cast<void**>(data);
          LinkerAPI *linker = reinterpret_cast<LinkerAPI*>(p[0]);
          LinkerMFileInfo *info = reinterpret_cast<LinkerMFileInfo*>(p[1]);
          (void)(linker->Link(*info, false));
          return nullptr;
      }, param);
    } else {
      (void)(pLinker->Link(*mplInfo, false));
    }
  }
#ifdef LINKER_DECOUPLE
  (void)pLinker->HandleDecouple(mplInfoList);
#endif
}

MClass *ClassLoaderImpl::GetPrimitiveClass(const std::string &mplClassName) {
  MClass *classInfo = nullptr;
  // check the dimension of the type
  size_t dim = 0;
  while (mplClassName[dim] == '[') {
    ++dim;
  }
  // predefined primitive types has a dimension <= 3
  if (dim > 3) {
    return nullptr;
  }

  char typeChar = mplClassName[0];
  if (dim > 0) {
    typeChar = mplClassName[dim];
  }
  switch (typeChar) {
    case 'Z':
      classInfo = reinterpret_cast<MClass*>(__mrt_pclasses_Z[dim]); // boolean
      break;
    case 'B':
      classInfo = reinterpret_cast<MClass*>(__mrt_pclasses_B[dim]); // byte
      break;
    case 'S':
      classInfo = reinterpret_cast<MClass*>(__mrt_pclasses_S[dim]); // short
      break;
    case 'C':
      classInfo = reinterpret_cast<MClass*>(__mrt_pclasses_C[dim]); // char
      break;
    case 'I':
      classInfo = reinterpret_cast<MClass*>(__mrt_pclasses_I[dim]); // int
      break;
    case 'F':
      classInfo = reinterpret_cast<MClass*>(__mrt_pclasses_F[dim]); // float
      break;
    case 'D':
      classInfo = reinterpret_cast<MClass*>(__mrt_pclasses_D[dim]); // double
      break;
    case 'J':
      classInfo = reinterpret_cast<MClass*>(__mrt_pclasses_J[dim]); // long
      break;
    case 'V':
      classInfo = reinterpret_cast<MClass*>(__mrt_pclasses_V[dim]); // void
      break;
    default:
      break;
  }
  if (classInfo != nullptr) {
    JSAN_ADD_CLASS_METADATA(classInfo); // Need move this func to init func.
    classInfo->SetClIndex(static_cast<uint16_t>(kClIndexFlag | 0)); // Initialize each class cl index as boot cl.
  }
  return classInfo;
}

void ClassLoaderImpl::VisitPrimitiveClass(const maple::rootObjectFunc &func) {
  // primitive and primitive array classes
  func((maple::address_t)__mrt_pclasses_V[0]); // void
  // predefined primitive types has a dimension <= 3
  for (int dim = 0; dim <= 3; ++dim) {
    func((maple::address_t)__mrt_pclasses_Z[dim]); // boolean
    func((maple::address_t)__mrt_pclasses_B[dim]); // byte
    func((maple::address_t)__mrt_pclasses_S[dim]); // short
    func((maple::address_t)__mrt_pclasses_C[dim]); // char
    func((maple::address_t)__mrt_pclasses_I[dim]); // int
    func((maple::address_t)__mrt_pclasses_F[dim]); // float
    func((maple::address_t)__mrt_pclasses_D[dim]); // double
    func((maple::address_t)__mrt_pclasses_J[dim]); // long
  }
}

MClass *ClassLoaderImpl::DoCreateArrayClass(MClass &klass, MClass &componentClass, const std::string &name) {
  MRTSetMetadataShadow(reinterpret_cast<ClassMetadata*>(&klass), WellKnown::GetMClassClass());
  klass.SetMonitor(0);
  klass.SetClIndex(componentClass.GetClIndex());
  klass.SetObjectSize(sizeof(reffield_t)); // here should all be object classes

#ifdef USE_32BIT_REF
  klass.SetFlag(FLAG_CLASS_ARRAY);
  klass.SetNumOfSuperClasses(0);
#endif // USE_32BIT_REF

  ClassMetadataRO *classMetadataRo = reinterpret_cast<ClassMetadataRO*>(
      reinterpret_cast<uintptr_t>(&klass) + sizeof(ClassMetadata));
  klass.SetClassMetaRoData(reinterpret_cast<uintptr_t>(classMetadataRo));
  klass.SetItable(0);
  klass.SetVtable(VTAB_OBJECT);
  klass.SetGctib(reinterpret_cast<uintptr_t>(GCTIB_OBJECT_ARRAY));
  classMetadataRo->className.SetRef(name.c_str());
  classMetadataRo->fields.SetDataRef(nullptr);
  classMetadataRo->methods.SetDataRef(nullptr);
  classMetadataRo->componentClass.SetDataRef(&componentClass);
  uint32_t modifiers = componentClass.GetArrayModifiers();
  classMetadataRo->numOfFields = 0;
  classMetadataRo->numOfMethods = 0;
#ifndef USE_32BIT_REF
  classMetadataRo->flag = FLAG_CLASS_ARRAY;
  classMetadataRo->numOfSuperclasses = 0;
  classMetadataRo->padding = 0;
#endif // !USE_32BIT_REF
  classMetadataRo->mod = modifiers;
  classMetadataRo->annotation.SetDataRef(nullptr);
  classMetadataRo->clinitAddr.SetDataRef(nullptr);
  // set this class as initialized with a readable address *klass*
  klass.SetInitStateRawValue(reinterpret_cast<uintptr_t>(&klass));
  return &klass;
}

// Only create this array class, don't recursively create missing ones
MClass *ClassLoaderImpl::CreateArrayClass(const std::string &mplClassName, MClass &componentClass) {
  if (mplClassName.empty()) {
    CL_LOG(ERROR) << "failed, mplClassName is null." << maple::endl;
    return nullptr;
  }
  MClass *klass = reinterpret_cast<MClass*>(MRT_AllocFromMeta(sizeof(ClassMetadata) + sizeof(ClassMetadataRO),
                                                              kClassMetaData));
  const std::string *allocName;
  // must alloc head obj, otherwise mplClassName will be free in advance
  allocName = new (std::nothrow) std::string(mplClassName);
  if (allocName == nullptr) {
    LOG(FATAL) << "ClassLoaderImpl::CreateArrayClass: new string failed" << maple::endl;
  }
  return DoCreateArrayClass(*klass, componentClass, *allocName);
}

#ifdef __cplusplus
extern "C" {
#endif
// MRT API Interfaces Begin
bool MRT_IsClassInitialized(jclass klass) {
  return reinterpret_cast<MClass*>(klass)->GetClIndex() != static_cast<uint16_t>(-1);
}

jobject MRT_GetNativeContexClassLoader() {
  jclass contextCls = MRT_GetNativeContexClass();
  if (contextCls != nullptr) {
    return MRT_GetClassLoader(contextCls);
  }
  return nullptr;
}

jclass MRT_GetNativeContexClass() {
  UnwindContext context;
  UnwindContext &lastContext = maplert::TLMutator().GetLastJavaContext();
  (void)MapleStack::GetLastJavaContext(context, lastContext, 0);
  jclass clazz = nullptr;
  if (!TryGetNativeContexClassLoaderForInterp(context, clazz)) {
    if (context.IsCompiledContext()) {
      clazz = context.frame.GetDeclaringClass();
    }
  }
  return clazz;
}

jobject MRT_GetClassLoader(jclass klass) {
  if (klass == nullptr) {
    CL_LOG(ERROR) << "failed, class object is null!" << maple::endl;
    return nullptr;
  }
  MObject *classLoader = LoaderAPI::As<ClassLoaderImpl&>().GetClassCL(reinterpret_cast<MClass*>(klass));
  if (classLoader == nullptr) {
    CL_DLOG(classloader) << "failed, classLoader returns null" << maple::endl;
  }
  return reinterpret_cast<jobject>(classLoader);
}

jobject MRT_ReflectGetClassLoader(jobject jobj)
__attribute__ ((alias ("MCC_GetCurrentClassLoader")));

jobject MCC_GetCurrentClassLoader(jobject caller) {
  MClass *callerObj = reinterpret_cast<MClass*>(caller);
  MClass *callerClass = callerObj->GetClass();
  // when the caller function is a static method, the caller itself is the classInfo
  if (callerClass == WellKnown::GetMClassClass()) {
    callerClass = callerObj;
  }
  jobject classLoader = MRT_GetClassLoader(reinterpret_cast<jclass>(callerClass));
  if (classLoader != nullptr) {
    RC_LOCAL_INC_REF(classLoader);
  }
  return classLoader;
}

jobject MRT_GetBootClassLoader() {
  // nullptr represents BootClassLoader in lower implementation.
  // For upper layer, always return BootClassLoader instance, but not nullptr.
  return LoaderAPI::As<ClassLoaderImpl&>().GetBootClassLoaderInstance();
}

// Get class in specific classloader.
// If classLoader is null, it means finding class in bootclassloader.
jclass MRT_GetClassByClassLoader(jobject classLoader, const std::string className) {
  bool isDelegateLast = false;
  MObject *mapleCl = reinterpret_cast<MObject*>(classLoader);
  if (classLoader != nullptr) {
    MClass *classLoaderClass = mapleCl->GetClass();
    isDelegateLast = classLoaderClass == WellKnown::GetMClassDelegateLastClassLoader();
    if (isDelegateLast) {
      CL_VLOG(classloader) << "name:" << className << ", clname:" << classLoaderClass->GetName() << maple::endl;
    }
  }
  jclass klass = LoaderAPI::Instance().FindClass(className, SearchFilter(classLoader, false, true, isDelegateLast));
  if (klass != nullptr) {
    (void)LinkerAPI::Instance().LinkClassLazily(klass);
  }
  return klass;
}

// Get class by reference to class in context.
// If context class is null, it means finding class in bootclassloader.
jclass MRT_GetClassByContextClass(jclass contextClass, const std::string className) {
  bool isInternalName = true;
  size_t len = className.size();
  if (len == 0) {
    return nullptr;
  }

  // try cache first
  uint32_t index = 0;
  bool lockFail(false);
  jclass cacheResult = CLCache::instance.GetCache(contextClass, className, index, lockFail);
  if (cacheResult) {
    return cacheResult;
  }

  if (className[len - 1] == ';') {
    isInternalName = false;
  }

  jclass klass = nullptr;
  if (contextClass == nullptr) {
    klass = LoaderAPI::Instance().FindClass(className, SearchFilter(nullptr, isInternalName, true, false));
  } else {
    klass = LoaderAPI::Instance().FindClass(className,
        SearchFilter(MRT_GetClassLoader(contextClass), isInternalName, true, false));
  }
  if (klass != nullptr) {
    (void)LinkerAPI::Instance().LinkClassLazily(klass);
  }
  if (!lockFail) {
    CLCache::instance.WriteCache(klass, contextClass, index);
  }
  return klass;
}

// Get class by reference to class in context.
// If context object is null, it means finding class in bootclassloader.
jclass MRT_GetClassByContextObject(jobject obj, const std::string className) {
  if (obj == nullptr) {
    return MRT_GetClassByContextClass(nullptr, className);;
  }
  MObject *clsObj = reinterpret_cast<MObject*>(obj);
  return MRT_GetClassByContextClass(reinterpret_cast<jclass>(clsObj->GetClass()), className);
}
CLCache CLCache::instance;

jclass MRT_GetClass(jclass caller, const std::string className) {
  uint32_t index = 0;
  bool lockFail(false);
  jclass cacheResult = CLCache::instance.GetCache(caller, className, index, lockFail);
  if (cacheResult != nullptr) {
    return cacheResult;
  }

  MClass *callerCls = reinterpret_cast<MClass*>(caller);
  MClass *callerClass = callerCls->GetClass();
  // When the caller function is a static method, the caller itself is the classInfo
  if (callerClass == WellKnown::GetMClassClass()) {
    callerClass = callerCls;
  }
  jobject classLoader = MRT_GetClassLoader(reinterpret_cast<jclass>(callerClass));
  jclass klass = LoaderAPI::Instance().FindClass(className, SearchFilter(classLoader, false, true, false));
  if (klass != nullptr) {
    (void)LinkerAPI::Instance().LinkClassLazily(klass);
  } else {
    CL_LOG(ERROR) << "callerClass=" << callerClass->GetName() << ", lazy=" << callerClass->IsLazyBinding() <<
        ", classLoader=" << classLoader << ", className=" << className << maple::endl;
  }
  if (!lockFail) {
    CLCache::instance.WriteCache(klass, caller, index);
  }
  return klass;
}

// Notice: It's invoked by compiler generating routine.
jclass MCC_GetClass(jclass caller, const char *className) {
  constexpr int eightBit = 256;
  jclass klass = MRT_GetClass(caller, className);
  if (UNLIKELY(klass == nullptr)) {
    char msg[eightBit] = { 0 };

    MClass *callerClass = reinterpret_cast<MClass*>(caller)->GetClass();
    // When the caller function is a static method, the caller itself is the classInfo
    if (callerClass == WellKnown::GetMClassClass()) {
      callerClass = reinterpret_cast<MClass*>(caller);
    }
    if (sprintf_s(msg, sizeof(msg), "No class found for %s, by %s", className, callerClass->GetName()) < 0) {
      CL_LOG(ERROR) << "sprintf_s failed" << maple::endl;
    }
    MRT_ThrowNoClassDefFoundErrorUnw(msg);
    return nullptr;
  }
  return klass;
}

void MRT_RegisterDynamicClass(jobject classLoader, jclass klass) {
  LoaderAPI::As<ClassLoaderImpl&>().RegisterDynamicClass(
      reinterpret_cast<const MObject*>(classLoader), reinterpret_cast<const MClass*>(klass));
}

void MRT_UnregisterDynamicClass(jobject classLoader, jclass klass) {
  LoaderAPI::As<ClassLoaderImpl&>().UnregisterDynamicClass(
      reinterpret_cast<const MObject*>(classLoader), reinterpret_cast<const MClass*>(klass));
}
#ifdef __cplusplus
}
#endif
}
