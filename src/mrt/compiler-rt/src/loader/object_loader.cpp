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

#include "loader/object_loader.h"

#include "chelper.h"
#include "cpphelper.h"
#include "base/systrace.h"
#include "yieldpoint.h"
#include "interp_support.h"

using namespace std;
namespace maplert {
ObjectLoader::ObjectLoader() : mBootClassLoader(nullptr), mSystemClassLoader(nullptr) {}
ObjectLoader::~ObjectLoader() {
  mBootClassLoader = nullptr;
  mSystemClassLoader = nullptr;
  pLinker = nullptr;
  pAdapterEx = nullptr;
}

// API Interfaces Begin
void ObjectLoader::PreInit(IAdapterEx adapterEx) {
  pLinker = &LinkerAPI::Instance();
  pAdapterEx = adapterEx.As<AdapterExAPI*>();
}
void ObjectLoader::PostInit(jobject systemClassLoader) {
  mSystemClassLoader = reinterpret_cast<MObject*>(systemClassLoader);
  RC_RUNTIME_INC_REF(systemClassLoader);
}
void ObjectLoader::UnInit() {
  if (mSystemClassLoader != nullptr) {
    RC_RUNTIME_DEC_REF(reinterpret_cast<jobject>(mSystemClassLoader));
  }
}
// If boot class loader, directly use nullptr as class loader.
// If so, we can reduce once iteration.
bool ObjectLoader::IsBootClassLoader(jobject classLoader) {
  if (classLoader == nullptr) {
    return true;
  }
  // save Inc/DecRef
  // If mBootClassLoader is not initialized, it must false, compare return false
  // If mBootClassLoader is initialized, return compare result
  return mBootClassLoader == reinterpret_cast<MObject*>(classLoader);
}

// WARNING: speical handle return value's RC for this method
// parent class loader return result can only be used in maple linker runtime code
// CANNOT return to java code
// save inc/dec cost
jobject ObjectLoader::GetCLParent(jobject classLoader) {
  if (IsBootClassLoader(classLoader)) {
    return nullptr;
  }
  if (WorldStopped()) {
    jobject res = MRT_ReflectGetFieldjobject(GetCLField(kFieldParent), classLoader);
    // warning, this is exceptional case, no incref for return value
    RC_LOCAL_DEC_REF(res);
    return res;
  }
  maplert::ScopedObjectAccess soa;
  jobject res = MRT_ReflectGetFieldjobject(GetCLField(kFieldParent), classLoader);
  // warning, this is exceptional case, no incref for return value
  RC_LOCAL_DEC_REF(res);
  return res;
}

void ObjectLoader::SetCLParent(jobject classLoader, jobject parentClassLoader) {
  if (IsBootClassLoader(classLoader)) {
    return;
  }
  maplert::ScopedObjectAccess soa;
  MRT_ReflectSetFieldjobject(GetCLField(kFieldParent), classLoader, parentClassLoader);
}

bool ObjectLoader::LoadClasses(jobject classLoader, ObjFile &objFile) {
  if (objFile.GetFileType() == FileType::kMFile) {
    return LoadClasses(reinterpret_cast<MObject*>(classLoader), objFile);
  } else if (objFile.GetFileType() == FileType::kDFile) {
    return MClassLocatorManagerInterpEx::LoadClasses(*this, classLoader, objFile);
  }
  CL_LOG(ERROR) << "unknow class file type" << maple::endl;
  return false;
}

size_t ObjectLoader::GetLoadedClassCount() {
  size_t totalCount = 0;
  ClassLocator *classLocator = nullptr;

  // Boot Class loader
  classLocator = reinterpret_cast<ClassLocator*>(const_cast<MObject*>(mCLTable[0]));
  totalCount += classLocator->GetLoadedClassCount();

  for (int i = 1; i < kMaxClassloaderNum; ++i) {
    const MObject *classLoader = mCLTable[i];
    jobject jClassLoader = reinterpret_cast<jobject>(const_cast<MObject*>(classLoader));
    if (classLoader == nullptr) {
      break;
    }
    classLocator = GetCLClassTable(jClassLoader).As<ClassLocator*>();
    if (classLocator == nullptr) {
      CL_LOG(ERROR) << "failed, cl=" << classLoader << ", classLocator is null!" << maple::endl;
      continue;
    }
    totalCount += classLocator->GetLoadedClassCount();
  }

  return totalCount;
}

size_t ObjectLoader::GetAllHashMapSize() {
  size_t totalSize = 0;
  // Boot Class loader
  ClassLocator *classLocator = reinterpret_cast<ClassLocator*>(const_cast<MObject*>(mCLTable[0]));
  totalSize += classLocator->GetClassHashMapSize();

  for (int i = 1; i < kMaxClassloaderNum; ++i) {
    const MObject *classLoader = mCLTable[i];
    jobject jClassLoader = reinterpret_cast<jobject>(const_cast<MObject*>(classLoader));
    if (classLoader == nullptr) {
      break;
    }
    classLocator = GetCLClassTable(jClassLoader).As<ClassLocator*>();
    if (classLocator == nullptr) {
      CL_LOG(ERROR) << "failed, classloader=" << classLoader << ", to CHECK why classLocator is null!" << maple::endl;
      continue;
    }
    totalSize += classLocator->GetClassHashMapSize();
  }

  return totalSize;
}

bool ObjectLoader::IsLinked(jobject classLoader) {
  ClassLocator *classLocator = GetCLClassTable(classLoader).As<ClassLocator*>();
  if (classLocator == nullptr) {
    return true; // Not to link if no classes loaded.
  }
  return classLocator->IsLinked();
}

void ObjectLoader::SetLinked(jobject classLoader, bool isLinked) {
  ClassLocator *classLocator = GetCLClassTable(classLoader).As<ClassLocator*>();
  if (classLocator == nullptr) {
    return;
  }
  classLocator->SetLinked(isLinked);
}

void ObjectLoader::ReTryLoadClassesFromMplFile(jobject classLoader, ObjFile &mplFile) {
  std::vector<LinkerMFileInfo*> fakeInfoList; // just for not multi-so, it is a empty list and never used
  (void)LoadClassesFromMplFile(reinterpret_cast<const MObject*>(classLoader), mplFile, fakeInfoList);
}

bool ObjectLoader::GetClassNameList(jobject classLoader, ObjFile &objFile, vector<std::string> &classVec) {
  ClassLocator *classLocator = GetCLClassTable(classLoader).As<ClassLocator*>();
  if (classLocator == nullptr) {
    return false;
  }
  return classLocator->GetClassNameList(objFile, classVec);
}
// MRT_EXPORT Split
// visit class laoder gc roots
void ObjectLoader::VisitGCRoots(const RefVisitor &function) {
  // system class loader is also recorded is Runtime, not visit there
  function(reinterpret_cast<address_t&>(mSystemClassLoader));
  for (int i = 1; i < kMaxClassloaderNum; ++i) {
    if (mCLTable[i] == nullptr) {
      break;
    }
    function(reinterpret_cast<address_t&>(mCLTable[i]));
  }
}

jobject ObjectLoader::GetSystemClassLoader() {
  // exceptional case, no need incref, as this only used in runtime and not return to java code
  // warning: use it with special care of RC
  return reinterpret_cast<jobject>(mSystemClassLoader);
}

jobject ObjectLoader::GetBootClassLoaderInstance() {
#ifndef __OPENJDK__
  if (mBootClassLoader == nullptr) {
    MClass *cls = reinterpret_cast<MClass*>(LocateClass("Ljava/lang/BootClassLoader;", SearchFilter()));
    MethodMeta *getInstanceMth = cls->GetMethod("getInstance", "()Ljava/lang/BootClassLoader;");
    mBootClassLoader = reinterpret_cast<MObject*>(getInstanceMth->InvokeJavaMethodFast<jobject>(nullptr));
    // warning, this is exceptional case, no incref for return value
    RC_LOCAL_DEC_REF(reinterpret_cast<jobject>(mBootClassLoader));
    return reinterpret_cast<jobject>(mBootClassLoader);
  }
#endif // !__OPENJDK__
  return reinterpret_cast<jobject>(mBootClassLoader);
}

void ObjectLoader::SetClassCL(jclass kls, jobject cl) {
  MClass *klass = reinterpret_cast<MClass*>(kls);
  MObject *classLoader = reinterpret_cast<MObject*>(cl);
  if (klass == nullptr) {
    CL_LOG(ERROR) << "failed, classloader=" << cl << ", error parameter: klass is null!" << maple::endl;
    return;
  }
  if (IsBootClassLoader(cl)) { // for bootstrap class loader
    klass->SetClIndex(static_cast<uint16_t>(kClIndexFlag | 0));
    return;
  }
  int16_t pos = -1;
  bool found = LocateIndex(classLoader, pos);
  if (found) {
    klass->SetClIndex(static_cast<uint16_t>(kClIndexFlag | static_cast<uint16_t>(pos)));
  } else if (!found && pos != -1) {
    mCLTable[pos] = classLoader;
    RC_RUNTIME_INC_REF(classLoader);
    klass->SetClIndex(static_cast<uint16_t>(kClIndexFlag | static_cast<uint16_t>(pos)));
  } else {
    CL_LOG(ERROR) << "failed, classloader=" << classLoader << ", table is null!" << maple::endl;
  }
}

// Get class locator from Class Loader's 'classTable' field.
// Notice that we get special locator at the beginning of mCLTable for boot class loader.
IObjectLocator ObjectLoader::GetCLClassTable(jobject classLoader) {
  if (IsBootClassLoader(classLoader)) {
    return reinterpret_cast<ClassLocator*>(const_cast<MObject*>(mCLTable[0]));
  }
  return reinterpret_cast<ClassLocator*>(MRT_ReflectGetFieldjlong(GetCLField(kFieldClassTable), classLoader));
}

void ObjectLoader::SetCLClassTable(jobject classLoader, IObjectLocator classLocator) {
  if (IsBootClassLoader(classLoader)) { // Boot classlocator is fixed at mCLTable[0], use it directly.
    mCLTable[0] = classLocator.As<const MObject*>();
    return;
  }
  // We set class locator for all classloaders, except bootclassloader.
  MRT_ReflectSetFieldjlong(GetCLField(kFieldClassTable), classLoader, classLocator.As<jlong>());
}
// API Interfaces End
bool ObjectLoader::LoadClassesFromMplFile(const MObject *classLoader, ObjFile &objFile,
    std::vector<LinkerMFileInfo*> &mplInfoList, bool hasSiblings) {
  bool isDecouple = true;
  jobject jClassLoader = reinterpret_cast<jobject>(const_cast<MObject*>(classLoader));
  if (objFile.GetFileType() == FileType::kMFile) {
    if (!pLinker->Add(objFile, jClassLoader)) {
      CL_LOG(ERROR) << "invalid maple file, or multiple loading " << objFile.GetName() << maple::endl;
    }
    LinkerMFileInfo *mplInfo = objFile.GetMplInfo();
    objFile.Load();
    if (hasSiblings) {
      mplInfoList.push_back(mplInfo);
    } else {
      static_cast<void>(pLinker->Link(*mplInfo, isDecouple));
    }

    // check mpl file version
    if (!loaderutils::CheckVersion(objFile, *mplInfo)) {
      return false;
    }

    // check mpl file compiler status, such as is gconly
    if (!loaderutils::CheckCompilerStatus(objFile, *mplInfo)) {
      return false;
    }

    SetLinked(jClassLoader, false);

    if (!LoadClasses(jClassLoader, objFile)) {
      CL_LOG(ERROR) << "failed to load classes from " << objFile.GetName() << ", classloader: " << classLoader <<
          maple::endl;
      return false;
    }
    objFile.SetClassTableLoaded();
  } else if (objFile.GetFileType() == FileType::kDFile) {
    if (!pAdapterEx->IsStarted()) {
      // Now, ClassLocatorManager.LoadClasses is needed only for one case:
      // loading Dex files in classpath into SystemClassLoader.
      if (!LoadClasses(jClassLoader, objFile)) {
        CL_LOG(ERROR) << "load class for dex file failed: " << objFile.GetName() << maple::endl;
        return false;
      }
    }
  } else {
    return false;
  }
  return true;
}

void ObjectLoader::UnloadClasses(const MObject *classLoader) {
  jobject jClassLoader = reinterpret_cast<jobject>(const_cast<MObject*>(classLoader));
  ClassLocator *classLocator = GetCLClassTable(jClassLoader).As<ClassLocator*>();
  if (classLocator == nullptr) {
    CL_LOG(ERROR) << "failed, classLocator is null." << maple::endl;
    return;
  }
  classLocator->UnloadClasses();

  // Destruct class loader's references properly.
  if (IsBootClassLoader(jClassLoader)) { // for boot class loader
    mCLTable[0] = nullptr; // Dereference the class locator of boot class loader.
  } else if (!RecycleIndex(classLoader)) { // Dereference the class loader, and recycle mCLTable index.
    CL_LOG(ERROR) << "RecycleIndex fail" << maple::endl;
  }
  delete classLocator; // Delete the class locator of class loader.
}

void ObjectLoader::VisitClassesByLoader(const MObject *loader, const maple::rootObjectFunc &func) {
  jobject jClassLoader = reinterpret_cast<jobject>(const_cast<MObject*>(loader));
  ClassLocator *locator = GetCLClassTable(jClassLoader).As<ClassLocator*>();
  if (locator == nullptr) {
    CL_LOG(ERROR) << "VisitClasses locator null for loader:" << loader << maple::endl;
    return;
  }
  locator->VisitClasses(func, IsBootClassLoader(jClassLoader));
}

jfieldID ObjectLoader::GetCLField(const FieldName &field) {
  static jfieldID cachedFields[] = { nullptr, nullptr };
  static const char *cachedNames[] = { "parent", "classTable" };
  if (cachedFields[field] == nullptr) {
    if (mCLTable[0] != nullptr) {
      jclass klass = FindClass("Ljava/lang/ClassLoader;", SearchFilter());
      if (klass == nullptr) {
        CL_LOG(ERROR) << "failed, can't find \"java/lang/ClassLoader\"!" << maple::endl;
        return nullptr;
      }
      cachedFields[field] = MRT_ReflectGetCharField(klass, cachedNames[field]);
      if (cachedFields[field] == nullptr) {
        CL_LOG(ERROR) << "failed, mParentField should not null!" << maple::endl;
      }
    } else {
      CL_LOG(ERROR) << "failed for mCLTable[0] is null!" << maple::endl;
    }
  }
  return cachedFields[field];
}

MObject *ObjectLoader::GetClassCL(MClass *klass) {
  uint16_t index = klass->GetClIndex();
  // Remove after clIndex can be initialized by compiler.
  if ((kClIndexFlagMask & index) == kClIndexFlag && index != kClIndexInintValue) { // Once initialized
    index = kClIndexValueMask & index;
  } else { // Not initialized
    index = 0;
    klass->SetClIndex(static_cast<uint16_t>(kClIndexFlag | 0));
    CL_DLOG(classloader) << "not initialized for " << klass->GetName() << maple::endl;
  }

  if (index == 0) { // for boot class loader
    return nullptr;
  } else if (index > 0 && index < kMaxClassloaderNum) {
    return const_cast<MObject*>(mCLTable[index]);
  } else {
    CL_LOG(ERROR) << "failed for index=" << index << maple::endl;
    return nullptr;
  }
}

// Assign index for classloader in advance, and return index with mask.
uint16_t ObjectLoader::PresetClassCL(const MObject *classLoader) {
  jobject jClassLoader = reinterpret_cast<jobject>(const_cast<MObject*>(classLoader));
  if (IsBootClassLoader(jClassLoader)) { // for boot class loader
    return static_cast<uint16_t>(kClIndexFlag | 0);
  }
  int16_t pos = -1;
  bool found = LocateIndex(classLoader, pos);
  if (found) {
    return static_cast<uint16_t>(kClIndexFlag | static_cast<uint16_t>(pos));
  } else if (!found && pos != -1) {
    maplert::ScopedObjectAccess soa;
    mCLTable[pos] = classLoader;
    RC_RUNTIME_INC_REF(classLoader);
    return static_cast<uint16_t>(kClIndexFlag | static_cast<uint16_t>(pos));
  } else {
    CL_LOG(ERROR) << "failed, classloader=" << classLoader << ", table is null!" << maple::endl;
    return 0;
  }
}

// Return the index if matched, or found a new valid index.
bool ObjectLoader::LocateIndex(const MObject *classLoader, int16_t &pos) const {
  pos = -1;
  for (int16_t i = 1; i < kMaxClassloaderNum; ++i) {
    if (mCLTable[i] == classLoader) { // Matched class loader set before.
      pos = i;
      return true;
    } else if (mCLTable[i] == nullptr) {
      pos = i;
      return false;
    }
  }
  return false;
}

// Recycle the index, we can rearrange the index to other class loader.
bool ObjectLoader::RecycleIndex(const MObject *classLoader) {
  for (int32_t i = 1; i < kMaxClassloaderNum; ++i) {
    if (mCLTable[i] == classLoader) { // Matched class loader set before.
      mCLTable[i] = nullptr;
      RC_RUNTIME_DEC_REF(classLoader);
      return true;
    }
  }

  return false;
}

void ObjectLoader::RegisterDynamicClass(const MObject *classLoader, const MClass *klass) {
  jclass jKlass = reinterpret_cast<jclass>(const_cast<MClass*>(klass));
  jobject jClassLoader = reinterpret_cast<jobject>(const_cast<MObject*>(classLoader));
  ClassLocator *classLocator = GetCLClassTable(jClassLoader).As<ClassLocator*>();
  if (classLocator == nullptr) {
    CL_LOG(ERROR) << "failed, classloader=" << classLoader << ", to CHECK why classLocator is null!" << maple::endl;
    return;
  }
  char *className = klass->GetName();
  if (!classLocator->RegisterDynamicClass(className, *const_cast<MClass*>(klass))) {
    CL_LOG(ERROR) << "failed to registerDynamicClass,name:className" << maple::endl;
  }
  SetClassCL(jKlass, jClassLoader);
}

void ObjectLoader::UnregisterDynamicClass(const MObject *classLoader, const MClass *klass) {
  jobject jClassLoader = reinterpret_cast<jobject>(const_cast<MObject*>(classLoader));
  ClassLocator *classLocator = GetCLClassTable(jClassLoader).As<ClassLocator*>();
  if (classLocator == nullptr) {
    CL_LOG(ERROR) << "failed, classloader=" << classLoader << ", to CHECK why classLocator is null!" << maple::endl;
    return;
  }
  char *className = klass->GetName();
  if (!classLocator->UnregisterDynamicClass(className)) {
    CL_LOG(ERROR) << "failed to unregisterDynamicClass, name:className" << maple::endl;
  }
}

// load classes from one ObjFile.
bool ObjectLoader::LoadClasses(const MObject *classLoader, ObjFile &objFile) {
  jobject jClassLoader = reinterpret_cast<jobject>(const_cast<MObject*>(classLoader));
  ClassLocator *classLocator = GetCLClassTable(jClassLoader).As<ClassLocator*>();
  if (classLocator == nullptr) {
    classLocator = new (std::nothrow) ClassLocator();
    if (classLocator == nullptr) {
      LINKER_LOG(FATAL) << "new ClassLocator failed" << maple::endl;
    }
    SetCLClassTable(jClassLoader, classLocator);
  }
  if (classLocator == nullptr) {
    CL_LOG(ERROR) << "failed, classloader=" << classLoader << ", classLocator is null" << maple::endl;
    return false;
  }

  // We will check the 'mplInfo' lazy binding flag in the ClassLocator.
  // If it's lazy, we will not set ClassLoader Index during initialization,
  // even though set 'initCLIndex' parameter here.
  if (!classLocator->LoadClasses(objFile, PresetClassCL(classLoader))) {
    CL_LOG(ERROR) << "failed: classloader=" << classLoader << maple::endl;
    return false;
  }

  return true;
}

bool ObjectLoader::LoadClasses(const MObject *classLoader, vector<ObjFile*> &objList) {
  jobject jClassLoader = reinterpret_cast<jobject>(const_cast<MObject*>(classLoader));
  ClassLocator *classLocator = GetCLClassTable(jClassLoader).As<ClassLocator*>();
  if (classLocator == nullptr) {
    classLocator = new (std::nothrow) ClassLocator();
    if (classLocator == nullptr) {
      LINKER_LOG(FATAL) << "new ClassLocator failed!" << maple::endl;
    }
    SetCLClassTable(jClassLoader, classLocator);
  }
  if (classLocator == nullptr) {
    CL_LOG(ERROR) << "failed, classloader = " << classLoader << ", classLocator is null" << maple::endl;
    return false;
  }

  // We will check the 'mplInfo' lazy binding flag in each ClassLocator.
  // If it's lazy, we will not set ClassLoader Index during initialization,
  // even though set 'initCLIndex' parameter here.
  if (!classLocator->LoadClasses(objList, PresetClassCL(classLoader))) {
    return false;
  }
  if (IsBootClassLoader(jClassLoader)) {
    // BootClassLoader will only call this function to load classes.
    // now register predefined array (object) classes. This is required for fast cycle detection
    for (MClass* primitiveItem: primitiveClasses) {
      RegisterDynamicClass(classLoader, primitiveItem);
    }
  }
  return true;
}
} // end namespace maple
