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
#ifndef __MAPLE_LOADER_OBJECT_LOADER__
#define __MAPLE_LOADER_OBJECT_LOADER__

#include "object_locator.h"
#include "gc_roots.h"
#include "loader/loader_utils.h"

namespace maplert {
enum FieldName {
  kFieldParent = 0,
  kFieldClassTable
};

class ObjectLoader : public LoaderAPI {
 public:
  ObjectLoader();
  virtual ~ObjectLoader();
  // API Interfaces Begin
  virtual void PreInit(IAdapterEx interpEx) override;
  virtual void PostInit(jobject systemClassLoader) override;
  virtual void UnInit() override;
  jobject GetCLParent(jobject classLoader) override;
  void SetCLParent(jobject classLoader, jobject parentClassLoader) override;
  bool IsBootClassLoader(jobject classLoader) override;
  bool LoadClasses(jobject classLoader, ObjFile &objFile) override;
  // get registered mpl file by exact path-name: the path should be canonicalized
  size_t GetLoadedClassCount() override;
  size_t GetAllHashMapSize() override;
  bool IsLinked(jobject classLoader) override;
  void SetLinked(jobject classLoader, bool isLinked) override;
  bool GetClassNameList(jobject classLoader, ObjFile &objFile, std::vector<std::string> &classVec) override;
  void ReTryLoadClassesFromMplFile(jobject jobject, ObjFile &mplFile) override;
  // MRT_EXPORT Split
  // visit class loader gc roots
  void VisitGCRoots(const RefVisitor &visitor) override;
  jobject GetSystemClassLoader() override;
  jobject GetBootClassLoaderInstance() override;
  void SetClassCL(jclass klass, jobject classLoader) override;
  IObjectLocator GetCLClassTable(jobject classLoader) override;
  void SetCLClassTable(jobject classLoader, IObjectLocator classLocator) override;
  // API Interfaces End
  MObject *GetClassCL(MClass *klass);
  void SetCLParent(const MObject *classLoader, const MObject *parentClassLoader);
  void RegisterDynamicClass(const MObject *classLoader, const MClass *klass);
  void UnregisterDynamicClass(const MObject *classLoader, const MClass *klass);
  virtual MClass *GetPrimitiveClass(const std::string &mplClassName) = 0;
  virtual void VisitPrimitiveClass(const maple::rootObjectFunc &func) = 0;
  virtual MClass *CreateArrayClass(const std::string &mplClassName, MClass &componentClass) = 0;
  IAdapterEx GetAdapterEx() override {
    return pAdapterEx;
  }
 protected:
  void UnloadClasses(const MObject *classLoader);
  uint16_t PresetClassCL(const MObject *classLoader);
  bool LocateIndex(const MObject *classLoader, int16_t &pos) const;
  bool RecycleIndex(const MObject *classLoader);
  void VisitClassesByLoader(const MObject *loader, const maple::rootObjectFunc &func);
  bool LoadClassesFromMplFile(const MObject *classLoader, ObjFile &mplFile,
      std::vector<LinkerMFileInfo*> &infoList, bool hasSiblings = false);
  bool LoadClasses(const MObject *classLoader, ObjFile &objFile);
  bool LoadClasses(const MObject *classLoader, std::vector<ObjFile*> &objList);

  LinkerAPI *pLinker = nullptr;
  AdapterExAPI *pAdapterEx = nullptr;
  static const int kMaxClassloaderNum = 256;
  const unsigned int kClIndexValueMask = 0x00FF;
  const unsigned int kClIndexFlagMask = 0xFF00;
  const unsigned int kClIndexFlag = 0xAB00;
  const unsigned int kClIndexInintValue = 0xABCD;
  // mCLTable[] stores all class loader, except index 0.
  // In each class loader, classTable filed will store its classlocator instance.
  // 'Cause NULL is for Bootstrap class loader, mCLTable[0]'d point to locator directly.
  // Index ---> Value
  // ---
  // [0]   -------------------------> MClassLocator_0_for_boot_class_loader
  // [1]   --->  CLASS_LOADER_1  ===> MClassLocator_1
  // [2]   --->  CLASS_LOADER_2  ===> MClassLocator_2
  // ...   --->  ..............  ===> ...
  // [255] ---> CLASS_LOADER_255 ===> MClassLocator_255
  const MObject *mCLTable[kMaxClassloaderNum] = { 0 };
  MObject *mBootClassLoader = nullptr;
  MObject *mSystemClassLoader = nullptr;
  std::vector<MClass*> primitiveClasses;
 private:
  jfieldID GetCLField(const FieldName &name);
};
} // end namespace maple
#endif // endif __MAPLE_LOADER_OBJECT_LOADER__
