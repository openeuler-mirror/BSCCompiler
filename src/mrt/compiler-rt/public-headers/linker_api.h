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
#ifndef __MAPLE_LINKER_API__
#define __MAPLE_LINKER_API__

#include "linker/linker_utils.h"
#include "file_adapter.h"

#ifdef __cplusplus
namespace maplert {
#define LINKER_LOG(level) LOG(level) << __FUNCTION__ << "(MplLinker), "
#define LINKER_VLOG(module) VLOG(module) << __FUNCTION__ << "(MplLinker), "
#define LINKER_DLOG(module) DLOG(module) << __FUNCTION__ << "(MplLinker), "
struct SignalInfo {
  SignalInfo(void *pc, void *offset) : pc(pc), offset(offset) {};
  void *pc;
  void *offset;
};
struct StrTab {
  char *startHotStrTab = nullptr;
  char *bothHotStrTab = nullptr;
  char *runHotStrTab = nullptr;
  char *coldStrTab = nullptr;
};
class LinkerAPI {
 public:
  MRT_EXPORT static LinkerAPI &Instance();
  template<typename T>
  static T As() {
    return reinterpret_cast<T>(Instance());
  }
  LinkerAPI() {};
  virtual ~LinkerAPI() = default;
  virtual void PreInit() = 0;
  virtual void PostInit() = 0;
  virtual void UnInit() = 0;
  // reference by stack_unwinder.h | jsan.cpp | jsan_lite.cpp | mm_utils.cpp | fault_handler_arm64.cc |
  //   fault_handler_linux.cc
  virtual bool IsJavaText(const void *addr) = 0;
  // reference by stack_unwinder.h | eh_personality.cpp | jsan.cpp | jsan_lite.cpp | libs.cpp |
  //   mm_utils.cpp | stack_unwinder.cpp | mpl_linkerTest.cpp
  virtual bool LocateAddress(const void *addr, LinkerLocInfo &info, bool getName) = 0;
  // reference by chelper.cpp | mclass.cpp | mrt_class_init.cpp | mrt_mclasslocatormanager.cpp |
  // mrt_reflection_class.cpp
  virtual bool LinkClassLazily(jclass klass) = 0;
  // reference by chelper.cpp | mrt_reflection_class | mpl_field_gctib.cpp | mclass.cpp
  virtual void ResolveColdClassSymbol(jclass classinfo) = 0;
  // reference by cinterface.cpp | collector_ms.cpp
  virtual void ReleaseBootPhaseMemory(bool isZygote, bool isSystemServer) = 0;
  // reference by collector_ms.cpp
  virtual void ClearAllMplFuncProfile() = 0;
  // reference by libs.cpp | dalvik_system_VMStack.cc | jvm.cpp
  virtual std::string GetMFileNameByPC(const void *pc, bool isLazyBinding) = 0;
  // reference by mm_utils.cpp | runtime.cc | mpl_linkerTest.cpp
  virtual std::string GetAppInfo() = 0;
  // reference by stack_unwinder.cpp
  virtual LinkerMFileInfo *GetLinkerMFileInfoByName(const std::string &name) = 0;
  virtual bool CheckLinkerMFileInfoElfBase(LinkerMFileInfo &mplInfo) = 0;
  // reference by fieldmeta.cpp | methodmeta.cpp | mrt_annotation.cpp
  virtual void GetStrTab(jclass dCl, StrTab &strTab) = 0;
  virtual char *GetCString(jclass dCl, uint32_t index) = 0;
  virtual void DestroyMFileCache() = 0;
  // reference by mrt_class_init.cpp | mrt_profile.cpp
  virtual LinkerMFileInfo *GetLinkerMFileInfoByAddress(const void *addr, bool isLazyBinding) = 0;
  // reference by mrt_mclasslocatormanager.cpp
  virtual jclass InvokeClassLoaderLoadClass(jobject classLoader, const std::string &className) = 0;
  // reference by mrt_profile.cpp
  virtual void DumpAllMplFuncProfile(
      std::unordered_map<std::string, std::vector<FuncProfInfo>> &funcProfileRaw) = 0;
  virtual void DumpBBProfileInfo(std::ostream &os) = 0;
  virtual void DumpAllMplFuncIRProfile(std::unordered_map<std::string, MFileIRProfInfo> &funcProfileRaw) = 0;
  // reference by mrt_reflection_method.cpp
  virtual bool UpdateMethodSymbolAddress(jmethodID method, uintptr_t addr) = 0;
  // reference by dalvik_system_VMStack.cc | class_linker.cc | jvm.cpp
  virtual LinkerMFileInfo *GetLinkerMFileInfoByClassMetadata(const void *addr, bool isClass) = 0;
  // reference by class_linker.cc | mpl_linkerTest.cpp
  virtual void *GetSymbolAddr(void *handle, const char *symbol, bool isFunction) = 0;
  // reference by mpl_file_checker.cc
  virtual void GetMplVersion(const LinkerMFileInfo &mplInfo, MapleVersionT &item) = 0;
  virtual void GetMplCompilerStatus(const LinkerMFileInfo &mplInfo, uint32_t &status) = 0;
  // reference by mpl_native_stack.cpp
  virtual bool GetJavaTextInfo(
      const void *addr, LinkerMFileInfo **mplInfo, LinkerLocInfo &info, bool getName) = 0;
  // reference by signal_catcher.cc | mpl_linkerTest.cpp
  virtual void DumpAllMplSectionInfo(std::ostream &os) = 0;
  // reference by mpl_linkerTest.cpp | NativeEntry.cpp
  virtual bool ContainLinkerMFileInfo(const std::string &name) = 0;
  virtual bool ContainLinkerMFileInfo(const void *handle) = 0;
  virtual void SetAppInfo(const char *dataPath, int64_t versionCode) = 0;
  virtual AppLoadState GetAppLoadState() = 0;
  virtual uint64_t DumpMetadataSectionSize(std::ostream &os, void *handle, const std::string sectionName) = 0;
  virtual std::string GetMethodSymbolByOffset(const LinkerInfTableItem &pTable) = 0;
  // reference by mclass_inline.h
  virtual jclass GetSuperClass(ClassMetadata **addr) = 0;
  // reference by jvm.cpp
  virtual std::string GetMFileNameByClassMetadata(const void *addr, bool isLazyBinding) = 0;
  // reference by mrt_prifole.cpp
  virtual std::string GetAppPackageName() = 0;
  // reference by mrt_mclasslocator_interpreter.cpp
  virtual bool ReGenGctib4Class(jclass classInfo) = 0;
  // reference by NativeEntry.cpp of cbg
  virtual void *LookUpSymbolAddress(const MUID &muid) = 0;
  virtual MUID GetMUID(const std::string symbol, bool forSystem) = 0;
  virtual bool IsFrontPatchMode(const std::string &path) = 0;

  virtual bool Add(ObjFile &objFile, jobject classLoader) = 0;
  virtual bool Resolve() = 0;
  // Resolve the single maple file.
  virtual bool Resolve(LinkerMFileInfo &mplInfo, bool decouple) = 0;
#ifdef LINKER_DECOUPLE
  virtual bool HandleDecouple(std::vector<LinkerMFileInfo*> &mplList) = 0;
#endif
  virtual void FinishLink(jobject classLoader) = 0;
  virtual bool Link() = 0;
  // Resolve the single maple file. MUST invoked Add() before.
  virtual bool Link(LinkerMFileInfo &mplInfo, bool decouple) = 0;
  virtual void SetLoadState(LoadStateType state) = 0;
  virtual void SetLinkerMFileInfoClassLoader(const ObjFile &objFile, jobject classLoader) = 0;
  virtual void SetClassLoaderParent(jobject classLoader, jobject newParent) = 0;
  virtual bool InsertClassesFront(ObjFile &objFile, jobject classLoader) = 0;
  virtual void SetPatchPath(std::string &path, int32_t mode) = 0;
  virtual void InitArrayCache(uintptr_t pc, uintptr_t addr) = 0;
#ifdef LINKER_RT_CACHE
  // reference by runtime.cc
  virtual void SetCachePath(const char *path) = 0;
#endif // LINKER_RT_CACHE
 protected:
  static LinkerAPI *pInstance;
};
extern "C" {
#endif
extern uint8_t __BindingProtectRegion__[];

bool MRT_RequestLazyBindingForSignal(const SignalInfo &data);
bool MRT_RequestLazyBindingForInitiative(const void *data);
bool MRT_RequestLazyBinding(const void *offset, const void *pc, bool fromSignal);
void MRT_FixOffsetTableLazily(LinkerOffsetValItemLazyLoad &offsetEntry);
void MRT_FixStaticAddrTableLazily(LinkerStaticAddrItem &addrTableItem);
void MRT_FixStaticAddrTable(LinkerStaticAddrItem &addrTableItem);
void InitProtectedRegion();
bool MRT_IsLazyBindingState(const uint8_t *address);
bool MRT_RequestInitArrayCache(SignalInfo *info);
#ifdef __cplusplus
} // extern "C"
} // namespace maplert
#endif
#endif // __MAPLE_LINKER_API__
