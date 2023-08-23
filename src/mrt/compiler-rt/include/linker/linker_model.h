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
#ifndef MAPLE_RUNTIME_MPL_LINKER_MODEL_H_
#define MAPLE_RUNTIME_MPL_LINKER_MODEL_H_

#include "linker_info.h"
#include "linker_api.h"
#include "loader_api.h"
#include "mclass_inline.h"

#include <typeinfo>
#include <atomic>
#include <array>
namespace maplert {
enum FeatureName {
  kFLinker,
#ifdef LINKER_DECOUPLE
  kFDecouple,
#endif
  kFLinkerCache,
  kFDebug,
  kFHotfix,
  kFLazyBinding,
  kFMethodBuilder
};
class FeatureModel {
 public:
  FeatureModel() {}
  virtual ~FeatureModel() {}
  virtual std::string GetName() {
    return "";
  }
};
class LinkerInvoker : public LinkerAPI {
 public:
  friend class FeatureModel;
  struct CodeRange {
    uintptr_t start;
    uintptr_t end;
    LinkerMFileInfo *mplInfo;
  };
  struct UpdateNode {
    MUID symbolId;
    MClass *klass;
    uintptr_t oldAddr;
    uintptr_t newAddr;
    UpdateNode(MClass *cls, const MUID id, uintptr_t oldA, uintptr_t newA) {
      klass = cls;
      symbolId = id;
      oldAddr = oldA;
      newAddr = newA;
    }
  };
  struct LinkerMFileCache {
    MClass *clsArray[4] = { nullptr }; // CacheSize is 4
    LinkerMFileInfo *mpArray[4] = { nullptr }; // CacheSize is 4
    uint8_t idx = 0;
  };
  // API Interfaces Begin
  std::string GetAppInfo() {
    return maplert::linkerutils::GetAppInfo();
  }
  void SetAppInfo(const char *dataPath, int64_t versionCode) {
    maplert::linkerutils::SetAppInfo(dataPath, versionCode);
  }
  AppLoadState GetAppLoadState() {
    return maplert::linkerutils::GetAppLoadState();
  }
  std::string GetAppPackageName() {
    return maplert::linkerutils::GetAppPackageName();
  }
  void *GetSymbolAddr(void *handle, const char *symbol, bool isFunction) {
    return maplert::linkerutils::GetSymbolAddr(handle, symbol, isFunction);
  }
  void GetMplVersion(const LinkerMFileInfo &mplInfo, MapleVersionT &item) {
    return maplert::linkerutils::GetMplVersion(mplInfo, item);
  }
  void GetMplCompilerStatus(const LinkerMFileInfo &mplInfo, uint32_t &status) {
    return maplert::linkerutils::GetMplCompilerStatus(mplInfo, status);
  }
  std::string GetMethodSymbolByOffset(const LinkerInfTableItem &pTable) {
    return maplert::linkerutils::GetMethodSymbolByOffset(pTable);
  }
  bool IsJavaText(const void *addr) {
    return GetLinkerMFileInfo(kFromPC, addr) != nullptr;
  }
  LinkerMFileInfo *GetLinkerMFileInfoByName(const std::string &name) {
    return GetLinkerMFileInfo(kFromName, name.c_str());
  }
  LinkerMFileInfo *GetLinkerMFileInfoByAddress(const void *addr, bool isLazyBinding) {
    return GetLinkerMFileInfo(kFromAddr, addr, isLazyBinding);
  }
  std::string GetMFileNameByPC(const void *pc, bool isLazyBinding) {
    return GetMFileName(kFromPC, pc, isLazyBinding);
  }
  std::string GetMFileNameByClassMetadata(const void *addr, bool isLazyBinding) {
    return GetMFileName(kFromMeta, addr, isLazyBinding);
  }
  LinkerMFileInfo *GetLinkerMFileInfoByClassMetadata(const void *addr, bool isClass) {
    bool isLazyBinding = false;
    if (isClass) {
      isLazyBinding = (reinterpret_cast<const MClass*>(addr))->IsLazyBinding();
    }
    return GetLinkerMFileInfo(kFromMeta, addr, isLazyBinding);
  }
  LinkerInvoker();
  virtual ~LinkerInvoker();
  virtual void PreInit();
  virtual void PostInit();
  virtual void UnInit();
#ifdef LINKER_RT_CACHE
  void SetCachePath(const char *path);
  bool GetCachePath(LinkerMFileInfo &mplInfo, std::string &path, LinkerCacheType cacheType);
#endif
  bool IsFrontPatchMode(const std::string &path);
  bool LinkClassLazily(jclass klass);
  bool ReGenGctib4Class(jclass classInfo);
  uint64_t DumpMetadataSectionSize(std::ostream &os, void *handle, const std::string sectionName);
  void DumpAllMplSectionInfo(std::ostream &os);
  void DumpAllMplFuncProfile(std::unordered_map<std::string, std::vector<FuncProfInfo>> &funcProfileRaw);
  void DumpAllMplFuncIRProfile(std::unordered_map<std::string, MFileIRProfInfo> &funcProfileRaw);
  void DumpBBProfileInfo(std::ostream &os);
  void ClearAllMplFuncProfile();
  void ReleaseBootPhaseMemory(bool isZygote, bool isSystemServer);
  bool CheckLinkerMFileInfoElfBase(LinkerMFileInfo &mplInfo);
  void ClearLinkerMFileInfo();
  bool ContainLinkerMFileInfo(const std::string &name);
  bool ContainLinkerMFileInfo(const void *handle);
  bool GetJavaTextInfo(const void *addr, LinkerMFileInfo **mplInfo, LinkerLocInfo &info, bool getName);
  bool UpdateMethodSymbolAddress(jmethodID method, uintptr_t addr);
  jclass GetSuperClass(ClassMetadata **addr);
  void GetStrTab(jclass dCl, StrTab &strTab);
  char *GetCString(jclass dCl, uint32_t index);
  void DestroyMFileCache();
  bool LocateAddress(const void *addr, LinkerLocInfo &info, bool getName);
  void ResolveColdClassSymbol(jclass classInfo);
  jclass InvokeClassLoaderLoadClass(jobject classLoader, const std::string &className);
  void *LookUpSymbolAddress(const MUID &muid);
  MUID GetMUID(const std::string symbol, bool forSystem);
  void CreateMplInfo(ObjFile &objFile, jobject classLoader);
  bool Add(ObjFile &objFile, jobject classLoader);
  bool Resolve();
  bool Resolve(LinkerMFileInfo &mplInfo, bool decouple); // Resolve the single maple file.
#ifdef LINKER_DECOUPLE
  bool HandleDecouple(std::vector<LinkerMFileInfo*> &mplList);
#endif
  void FinishLink(jobject classLoader);
  bool Link();
  // Resolve the single maple file. MUST invoked Add() before.
  bool Link(LinkerMFileInfo &mplInfo, bool decouple);
  void SetLoadState(LoadStateType state);
  void SetLinkerMFileInfoClassLoader(const ObjFile &objFile, jobject classLoader);
  void SetClassLoaderParent(jobject classLoader, jobject newParent);
  bool InsertClassesFront(ObjFile &objFile, jobject classLoader);
  void SetPatchPath(std::string &path, int32_t mode);
  // API Interfaces END
#ifdef LINKER_DECOUPLE
  bool IsClassComplete(const MClass &classInfo);
#endif
  BindingState GetAddrBindingState(LinkerVoidType addr);
  BindingState GetAddrBindingState(const AddrSlice &addrSlice, size_t index, bool isAtomic = true);
  void DumpStackInfoInLog();
  void *GetMplOffsetValue(LinkerMFileInfo &mplInfo);
  MUID GetValidityCode(LinkerMFileInfo &mplInfo) const;
  MUID GetValidityCodeForDecouple(LinkerMFileInfo &mplInfo) const;
  void *GetMethodSymbolAddress(LinkerMFileInfo &mplInfo, size_t index);
  void *GetDataSymbolAddress(LinkerMFileInfo &mplInfo, size_t index);
  void GetClassLoaderList(const LinkerMFileInfo &mplInfo, ClassLoaderListT &out, bool isNewList = true);
  void ResetClassLoaderList(const MObject *classLoader);
  MObject *GetClassLoaderByAddress(LinkerMFileInfo &mplInfo, const void *addr);
  void GetLinkerMFileInfos(LinkerMFileInfo &mplInfo, LinkerMFileInfoListT &fileList, bool isNewList = true);
  void *GetClassMetadataLazily(LinkerMFileInfo &mplInfo, size_t classIndex);
  void *GetClassMetadataLazily(const void *offsetTable, size_t classIndex);
  LinkerMFileInfo *SearchAddress(const void *pc, AddressRangeType type = kTypeWhole, bool isLazyBinding = false);

  bool LocateAddress(const void *handle, const void *addr, LinkerLocInfo &info, bool getName);
  bool LocateAddress(LinkerMFileInfo &mplInfo, const void *addr, LinkerLocInfo &info, bool getName);
  bool DoLocateAddress(const LinkerMFileInfo &mplInfo, LinkerLocInfo &info, const void *addr,
      const AddrSlice &pTable, const InfTableSlice &infTableSlice, int64_t pos, bool getName);
  LinkerVoidType LookUpDataSymbolAddress(LinkerMFileInfo &mplInfo, const MUID &muid, size_t &index);
  LinkerVoidType LookUpMethodSymbolAddress(LinkerMFileInfo &mplInfo, const MUID &muid, size_t &index);
  void ResolveVTableSymbolByClass(LinkerMFileInfo &mplInfo, const MClass *classInfo, bool flag);
  void ResolveITableSymbolByClass(LinkerMFileInfo &mplInfo, const MClass *classInfo);
  void ResolveSuperClassSymbolByClass(LinkerMFileInfo &mplInfo, const MClass *classInfo);
  bool UpdateMethodSymbolAddressDef(const MClass *klass, const MUID &symbolId, const uintptr_t newAddr);
  bool UpdateMethodSymbolAddressUndef(LinkerMFileInfo &mplInfo, const UpdateNode &node);
  bool UpdateMethodSymbolAddressDecouple(LinkerMFileInfo &mplInfo, const UpdateNode &node);
  LinkerMFileInfo *GetLinkerMFileInfo(MFileInfoSource source, const void *key, bool isLazyBinding = false);
  std::string GetMFileName(MFileInfoSource source, const void *key, bool isLazyBinding = false);
  void InitArrayCache(uintptr_t pc, uintptr_t addr);
  template<typename Type, typename F>
  bool ForEachDoAction(Type obj, F action);
  template<typename Type, typename F, typename Data>
  bool ForEachDoAction(Type obj, F action, const Data &data);
  template<typename Type, typename F>
  bool DoAction(Type obj, F action, LinkerMFileInfo &mplInfo);
  template<typename Type, typename F>
  bool ForEachLookUp(const MUID &muid, Type obj, F lookup, LinkerMFileInfo &mplInfo,
#ifdef LINKER_RT_CACHE
                     LinkerMFileInfo **resInfo, size_t &index,
#endif // LINKER_RT_CACHE
                     LinkerOffsetType &pAddr);
  template<typename T1, typename T2, typename T3>
  int64_t BinarySearchIndex(const LinkerMFileInfo &mplInfo, const T1 &pTable, size_t start,
      size_t end, const size_t value, const T2 &pIndexTable, T3 &pInfTable, size_t scopeStartAddr,
      size_t scopeEndAddr);
  template<typename TableItem>
  bool ResolveSymbolLazily(LinkerMFileInfo &mplInfo, bool isSuper, const AddrSlice &dstSlice, size_t index,
      bool fromUpper, bool fromUndef, TableItem &tableItem, size_t tableIndex, const void *addr);
  template<SectTabIndex UndefName, SectTabIndex DefName, typename TableItem, bool isSuper>
  bool ResolveSymbolByClass(LinkerMFileInfo &mplInfo, TableItem &tableItem, size_t index, size_t tableIndex,
      bool fromUndef);
  template<class T>
  T *Get() {
    return reinterpret_cast<T*>(features[T::featureName]);
  }
  inline const LoaderAPI *GetLoader() const {
    return pLoader;
  }
  inline bool IsSystemClassLoader(const MObject *classLoader) {
    return pLoader->GetSystemClassLoader() == reinterpret_cast<jclass>(const_cast<MObject*>(classLoader));
  }
  inline bool IsBootClassLoader(const MObject *classLoader) {
    return pLoader->IsBootClassLoader(reinterpret_cast<jclass>(const_cast<MObject*>(classLoader)));
  }
  // Reset class loader hierarchy list
  inline void ResetMplInfoClList(LinkerMFileInfo &mplInfo) {
    mplInfo.clList.clear();
  }
  inline int32_t GetMultiSoPendingCount() const {
    return multiSoPendingCount;
  }
  inline void AddMultiSoPendingCount() {
    ++multiSoPendingCount;
  }
  inline void SubMultiSoPendingCount() {
    --multiSoPendingCount;
  }
  inline uint32_t AddrToUint32(const void *addr) {
    if (addr == nullptr) {
      LINKER_LOG(FATAL) << "AddrToUint32: addr is nullptr!" << maple::endl;
    }
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(addr));
  }
  LinkerMFileInfoListT mplInfoList;
  LinkerMFileInfoListCLMapT mplInfoListCLMap;
  LinkerMFileInfoNameMapT mplInfoNameMap;
  LinkerMFileInfoHandleMapT mplInfoHandleMap;
  LinkerMFileInfoElfAddrSetT mplInfoElfAddrSet;
  // Compare the lazy binding ELF file only.
  LinkerMFileInfoElfAddrSetT mplInfoElfAddrLazyBindingSet;
  MplClassLoaderLoadClassMethodMapT mplClassLoaderLoadClassMethodMap;
 protected:
  std::mutex mLinkLock;
  const uint32_t kItabHashSize = 23;
  LoaderAPI *pLoader = nullptr;
  std::map<FeatureName, FeatureModel*> features;
  int32_t multiSoPendingCount = 0;
  static constexpr size_t kLeastReleaseMemoryByteSize = 100 * 1024; // 100KiB (1KiB is 1024B)
};
} // namespace maplert
#endif // MAPLE_RUNTIME_MPL_LINKER_MODEL_H_
