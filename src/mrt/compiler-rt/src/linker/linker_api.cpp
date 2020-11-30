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
#include "linker_api.h"

#include <thread>
#include <dirent.h>
#include <fstream>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/file.h>

#include "file_system.h"
#include "object_type.h"
#include "linker/linker.h"
#include "linker/linker_inline.h"
#include "linker/linker_model.h"
#include "linker/linker_cache.h"
#include "linker/linker_debug.h"
#include "linker/linker_hotfix.h"
#include "linker/linker_lazy_binding.h"
#include "linker/linker_gctib.h"
#ifdef LINKER_DECOUPLE
#include "linker/decouple/linker_decouple.h"
#endif
#include "utils/name_utils.h"
#include "file_layout.h"
#include "collector/cp_generator.h"

using namespace maple;
namespace maplert {
using namespace linkerutils;
void LinkerInvoker::PreInit() {
  pLoader = &LoaderAPI::Instance();
#ifdef LINKER_RT_CACHE
  SetCachePath(kLinkerRootCachePath);
#endif
}
void LinkerInvoker::PostInit() {
  multiSoPendingCount = 0;
}
void LinkerInvoker::UnInit() {
  multiSoPendingCount = 0;
}
#ifdef LINKER_RT_CACHE
void LinkerInvoker::SetCachePath(const char *path) {
  Get<LinkerCache>()->SetPath(path);
}
bool LinkerInvoker::GetCachePath(LinkerMFileInfo &mplInfo, std::string &path, LinkerCacheType cacheType) {
  return Get<LinkerCache>()->GetPath(mplInfo, path, cacheType, maplert::linkerutils::GetLoadState());
}
#endif
bool LinkerInvoker::LinkClassLazily(jclass klass) {
  Get<LazyBinding>()->LinkClass(reinterpret_cast<MClass*>(klass));
  return reinterpret_cast<MClass*>(klass)->IsLazyBinding();
}
bool LinkerInvoker::ReGenGctib4Class(jclass classInfo) {
#ifdef LINKER_DECOUPLE
  MplGctibAnalysis &gctib = Get<Decouple>()->GetGctibResolver();
  return gctib.ReGenGctib4Class(reinterpret_cast<ClassMetadata*>(classInfo));
#else
  return ReGenGctib(reinterpret_cast<ClassMetadata*>(classInfo));
#endif
}
uint64_t LinkerInvoker::DumpMetadataSectionSize(std::ostream &os, void *handle, const std::string sectionName) {
  return Get<Debug>()->DumpMetadataSectionSize(os, handle, sectionName);
}

void LinkerInvoker::DumpAllMplSectionInfo(std::ostream &os) {
  Get<Debug>()->DumpAllMplSectionInfo(os);
}
void LinkerInvoker::DumpAllMplFuncProfile(std::unordered_map<std::string, std::vector<FuncProfInfo>> &funcProfileRaw) {
  Get<Debug>()->DumpAllMplFuncProfile(funcProfileRaw);
}
void LinkerInvoker::DumpAllMplFuncIRProfile(std::unordered_map<std::string, MFileIRProfInfo> &funcProfileRaw) {
  Get<Debug>()->DumpAllMplFuncIRProfile(funcProfileRaw);
}
void LinkerInvoker::DumpBBProfileInfo(std::ostream &os) {
  Get<Debug>()->DumpBBProfileInfo(os);
}
void LinkerInvoker::ClearAllMplFuncProfile() {
  auto handle = [](LinkerMFileInfo &mplInfo)->void {
    size_t size = mplInfo.GetTblSize(kMethodProfile);
    LinkerFuncProfileTableItem *pTable = mplInfo.GetTblBegin<LinkerFuncProfileTableItem*>(kMethodProfile);
    if (size == 0) {
      return;
    }
    for (size_t i = 0; i < size; ++i) {
      LinkerFuncProfileTableItem item = pTable[i];
      if (item.callTimes) {
        (void)mplInfo.funProfileMap.insert(std::make_pair(static_cast<uint32_t>(i),
            FuncProfInfo(item.callTimes, static_cast<uint8_t>(kLayoutBootHot))));
      }
    }
  };
  mplInfoList.ForEach(handle);
}

// 1.ignore zygote process
// 2.ignore system library in app process
// 3.ignore library whose javaTextTableSize < 100KB
// 4.release cold string table, JAVA_TEXT_TABLE and read-only memory of muid-tables:
//   METHOD_INF_TABLE, METHOD_DEF_MUID_TABLE, DATA_DEF_MUID_TABLE, METHOD_MUID_INDEX_TABLE
void LinkerInvoker::ReleaseBootPhaseMemory(bool isZygote, bool isSystemServer) {
  if (isZygote) {
    // release other mygote only boot-memory
    return;
  }
  auto workingLinkerMFileInfoList = mplInfoList.Clone();
  for (auto mplInfo : workingLinkerMFileInfoList) {
    // ignore system library memory release in app process, which reduce release about 7M/8M
    if (!isSystemServer && !mplInfo->BelongsToApp()) {
      continue;
    }

    // according to statistics, the library memory is small if its javaTextTableSize < 100kB
    // ignore all the sections release with the small library
    size_t javaTextTableSize = mplInfo->GetTblSize(kJavaText);
    if (javaTextTableSize < kLeastReleaseMemoryByteSize) {
      continue;
    }

    // release .java_text
    void *startAddr = mplInfo->GetTblBegin(kJavaText);
    void *endAddr = mplInfo->GetTblEnd(kJavaText);
    ReleaseMemory(startAddr, endAddr);

    mplInfo->ReleaseReadOnlyMemory();
  }
}

bool LinkerInvoker::CheckLinkerMFileInfoElfBase(LinkerMFileInfo &mplInfo) {
  if (mplInfo.elfEnd == nullptr) {
    mplInfo.elfEnd = GetSymbolAddr(mplInfo.handle, "MRT_GetMapleEnd", true);
  }
  int result = dl_iterate_phdr(
    [](struct dl_phdr_info *phdrInfo, size_t, void *data)->int {
      if (!LOG_NDEBUG) {
        std::string name(phdrInfo->dlpi_name);
        if (name.find("libmaple") != std::string::npos) {
          LINKER_DLOG(mpllinker) << "name=" << phdrInfo->dlpi_name << " (" << phdrInfo->dlpi_phnum << " segments)" <<
              "base=" << std::hex << phdrInfo->dlpi_addr << maple::endl;
          for (int i = 0; i < phdrInfo->dlpi_phnum; ++i) {
            LINKER_DLOG(mpllinker) << "header[" << i << "][type:" << phdrInfo->dlpi_phdr[i].p_type << "]: address=" <<
                std::hex << reinterpret_cast<void*>(phdrInfo->dlpi_addr + phdrInfo->dlpi_phdr[i].p_vaddr) <<
                maple::endl;
          }
        }
      }
      LinkerMFileInfo *mplInfo = static_cast<LinkerMFileInfo*>(data);
      if (mplInfo->name != phdrInfo->dlpi_name) {
        // If not equel, to check next dl_phdr_info
        return 0;
      }
      mplInfo->elfBase = reinterpret_cast<void*>(phdrInfo->dlpi_addr);
      return 1;
    }, &mplInfo);
  // To match abnormal path, such as /data/maple/lib64/libmaplecore-all.so -> /system/lib64/libmaplecore-all.so
  bool flagResult = static_cast<bool>(result);
  if (!flagResult) {
    result = dl_iterate_phdr(
      [](struct dl_phdr_info *phdrInfo, size_t, void *data)->int {
        LinkerMFileInfo *mplInfo = static_cast<LinkerMFileInfo*>(data);
        size_t pos = mplInfo->name.rfind('/');
        if (pos == std::string::npos) {
          return 0;
        }
        std::string fileName = mplInfo->name.substr(pos + 1);
        std::string name(phdrInfo->dlpi_name);
        if (name.find(fileName) == std::string::npos) {
          // If not equel, to check next dl_phdr_info
          return 0;
        }
        mplInfo->elfBase = reinterpret_cast<void*>(phdrInfo->dlpi_addr);
        LINKER_VLOG(mpllinker) << "for abnormal path, elfBase=" << std::hex << mplInfo->elfBase << ", for " <<
            mplInfo->name << maple::endl;
        return 1;
      }, &mplInfo);
  }

  LINKER_VLOG(mpllinker) << "base=" << mplInfo.elfBase << ", " <<
      GetSymbolAddr(mplInfo.handle, "MRT_GetMapleEnd", true) <<
      ", range={" << mplInfo.elfStart << ", " <<  mplInfo.elfEnd << "}, for " << mplInfo.name << maple::endl;
  return static_cast<bool>(result);
}

// Check if the maple file of name is open for the maple file.
// Return true, if it's valid maple file handle, or return false.
bool LinkerInvoker::ContainLinkerMFileInfo(const std::string &name) {
  return mplInfoNameMap.Find(name);
}

// Check if the handle is open for the maple file.
// Return true, if it's valid maple file handle, or return false.
bool LinkerInvoker::ContainLinkerMFileInfo(const void *handle) {
  return mplInfoHandleMap.Find(handle);
}

bool LinkerInvoker::GetJavaTextInfo(const void *addr, LinkerMFileInfo **mplInfo, LinkerLocInfo &info, bool getName) {
  *mplInfo = GetLinkerMFileInfo(kFromPC, addr);
  if (*mplInfo != nullptr && LocateAddress(**mplInfo, addr, info, getName)) {
    return true;
  }
  LINKER_VLOG(mpllinker) << "failed, not found " << addr << maple::endl;
  return false;
}

void LinkerInvoker::GetStrTab(jclass klass, StrTab &strTab) {
  const MClass *dCl = reinterpret_cast<const MClass*>(klass);
  if (dCl->IsProxy() && dCl->GetNumOfSuperClasses() >= 1) {
    dCl = dCl->GetSuperClassArray()[1];
  }
  constexpr int32_t cacheSize = 4;
  LinkerMFileCache *lc = static_cast<LinkerMFileCache*>(maple::tls::GetTLS(maple::tls::kSlotMFileCache));
  if (lc == nullptr) {
    lc = new (std::nothrow) LinkerMFileCache;
    if (lc == nullptr) {
      LOG(FATAL) << "new LinkerMFileCache fail" << maple::endl;
    }
  }
  LinkerMFileInfo *mplInfo = nullptr;
  for (uint8_t i = 0; i < cacheSize; ++i) {
    if (lc->clsArray[i] == dCl) {
      mplInfo = lc->mpArray[i];
    }
  }
  if (mplInfo == nullptr) {
    mplInfo = GetLinkerMFileInfo(kFromMeta, dCl, dCl->IsLazyBinding());
    lc->clsArray[lc->idx] = const_cast<MClass*>(dCl);
    lc->mpArray[lc->idx] = mplInfo;
    lc->idx = (lc->idx + 1) % cacheSize;
    maple::tls::StoreTLS(static_cast<void*>(lc), maple::tls::kSlotMFileCache);
  }
  if (mplInfo == nullptr) {
    LINKER_VLOG(mpllinker) << dCl->GetName() << ", mplInfo is nullptr" << maple::endl;
    return;
  }
  strTab.startHotStrTab = mplInfo->startHotStrTab;
  strTab.bothHotStrTab = mplInfo->bothHotStrTab;
  strTab.runHotStrTab = mplInfo->runHotStrTab;
  strTab.coldStrTab = mplInfo->coldStrTab;
}

char *LinkerInvoker::GetCString(jclass klass, uint32_t srcIndex) {
  const MClass *dCl = reinterpret_cast<const MClass*>(klass);
  constexpr int32_t realIndexStart = 2;
  LinkerMFileInfo *mplInfo = GetLinkerMFileInfo(kFromMeta, dCl, dCl->IsLazyBinding());
  if (mplInfo == nullptr) {
    LINKER_VLOG(mpllinker) << dCl->GetName() << ", mplInfo is nullptr" << maple::endl;
    return nullptr;
  }
  char *cStrStart = nullptr;
  uint32_t index = srcIndex & 0xFFFFFFFF;
  // 0x03 is 0011, index & 0x03 is to check isHotReflectStr.
  bool isHotReflectStr = (index & 0x03) != 0;
  uint32_t cStrIndex = index >> realIndexStart;
  if (isHotReflectStr) {
    uint32_t tag = (index & 0x03) - kCStringShift;
    if (tag == static_cast<uint32_t>(kLayoutBootHot)) {
      cStrStart = mplInfo->startHotStrTab;
    } else if (tag == static_cast<uint32_t>(kLayoutBothHot)) {
      cStrStart = mplInfo->bothHotStrTab;
    } else {
      cStrStart = mplInfo->runHotStrTab;
    }
  } else {
    cStrStart = mplInfo->coldStrTab;
  }
  return cStrStart + cStrIndex;
}

void LinkerInvoker::DestroyMFileCache() {
  LinkerMFileCache *lc = static_cast<LinkerMFileCache*>(maple::tls::GetTLS(maple::tls::kSlotMFileCache));
  if (lc != nullptr) {
    delete lc;
    lc = nullptr;
    maple::tls::StoreTLS(nullptr, maple::tls::kSlotMFileCache);
  }
}

// Thread-Unsafe
bool LinkerInvoker::UpdateMethodSymbolAddress(jmethodID method, uintptr_t newAddr) {
  MethodMeta *methodInfo = reinterpret_cast<MethodMeta*>(method);
  MClass *klass = methodInfo->GetDeclaringClass();
  if (klass == nullptr) {
    LINKER_LOG(FATAL) << "klass is nullptr" << maple::endl;
  }
  // build symbol name
  std::string symbolName(klass->GetName());
  symbolName += "|";
  symbolName += methodInfo->GetName();
  symbolName += "|";
  symbolName += methodInfo->GetSignature();
  // generate unique id
  MUID symbolId = GetMUID(namemangler::EncodeName(symbolName), true);
  if (!UpdateMethodSymbolAddressDef(klass, symbolId, newAddr)) {
    return false;
  }
  UpdateNode node(klass, symbolId, 0, newAddr);
  // traverse all libraries and update external reference table
  (void)ForEachDoAction(this, &LinkerInvoker::UpdateMethodSymbolAddressUndef, &node);
  // constructor, private or static method
  if (methodInfo->IsDirectMethod()) {
    return true; // all done
  }
  uintptr_t oldAddr = reinterpret_cast<uintptr_t>(methodInfo->GetFuncAddress());
  node.oldAddr = oldAddr;
#ifdef LINKER_DECOUPLE
  if (!MRT_CLASS_IS_DECOUPLE(klass)) {
    // traverse all libraries and update vtable and itable (non-decoupled class)
    (void)ForEachDoAction(this, &LinkerInvoker::UpdateMethodSymbolAddressDecouple, &node);
  } else {
    // update vtable and itable in the perm space (decoupled class)
    MRT_VisitDecoupleObjects([oldAddr, newAddr](address_t objAddr) {
      int tag = DecoupleAllocHeader::GetTag(objAddr);
      if (static_cast<DecoupleTag>(tag) != kITabAggregate && static_cast<DecoupleTag>(tag) != kVTabArray) {
        return; // next block
      }
      size_t blockSize = DecoupleAllocHeader::GetSize(objAddr);
      size_t size = static_cast<size_t>(blockSize / sizeof(LinkerVTableItem));
      LinkerVTableItem *pTable = reinterpret_cast<LinkerVTableItem*>(objAddr);
      if (size != 0 && pTable != nullptr) {
        for (size_t i = 0; i < size; ++i) {
          if (static_cast<uint32_t>(pTable[i].index) == oldAddr) { // compare by address
            LINKER_VLOG(mpllinker) << "Update [perm]" << (pTable + i) << ":" << tag << ":" << oldAddr << maple::endl;
            pTable[i].index = newAddr;
          }
        }
      }
    });
  }
#else
  // traverse all libraries and update vtable and itable (non-decoupled class)
  (void)ForEachDoAction(this, &LinkerInvoker::UpdateMethodSymbolAddressDecouple, &node);
#endif
  return true;
}

jclass LinkerInvoker::GetSuperClass(ClassMetadata **addr) {
  MClass *klass = reinterpret_cast<MClass*>(__atomic_load_n(addr, __ATOMIC_ACQUIRE));
  // To check if the super class is not resolved.
  // If so, we'd try to resolve it once again.
  LinkerRef ref(klass);
  if (ref.IsIndex()) {
    size_t index = ref.GetIndex();
    bool fromUndef = ref.IsFromUndef();
    LINKER_LOG(ERROR) << "unresolved super class=" << klass << ", " << index <<
        ", fromUndef=" << fromUndef << maple::endl;
    LinkerMFileInfo *mplInfo = SearchAddress(addr);
    if (mplInfo == nullptr) {
      LINKER_LOG(FATAL) << "LinkerMFileInfo is null, addr=" << addr << ", " << klass << ", " << index << maple::endl;
      return 0;
    }
    size_t dataUndefSize = mplInfo->GetTblSize(kDataUndef);
    AddrSlice dataUndefSlice(mplInfo->GetTblBegin<LinkerAddrTableItem*>(kDataUndef), dataUndefSize);
    size_t dataDefSize = mplInfo->GetTblSize(kDataDef);
    AddrSlice dataDefSlice(mplInfo->GetTblBegin<LinkerAddrTableItem*>(kDataDef), dataDefSize);
    if (fromUndef && index < dataUndefSize && !dataUndefSlice.Empty()) {
      if (mplInfo->IsFlag(kIsLazy)) {
        LINKER_LOG(FATAL) << "should not resolve lazily, " << fromUndef << ", " << index << ", " << dataUndefSize <<
            ", " << dataDefSize << " in " << mplInfo->name << maple::endl;
      } else {
        klass = reinterpret_cast<MClass*>(dataUndefSlice[index].Address());
        LINKER_LOG(ERROR) << "(UNDEF), klass=" << klass->GetName() << "addr=" <<
            dataUndefSlice[index].Address() << " in " << mplInfo->name << maple::endl;
      }
    } else if (!fromUndef && index < dataDefSize && !dataDefSlice.Empty()) {
      if (mplInfo->IsFlag(kIsLazy)) {
        LINKER_LOG(FATAL) << "(DEF), should not resolve lazily, " << fromUndef << ", " << index << ", " <<
            dataUndefSize << ", " << dataDefSize << " in " << mplInfo->name << maple::endl;
      } else {
        klass = reinterpret_cast<MClass*>(GetDefTableAddress(*mplInfo, dataDefSlice,
            static_cast<size_t>(index), false));
        LINKER_LOG(ERROR) << "(DEF), klass=" << klass->GetName() << "addr=" <<
            dataDefSlice[index].Address() << " in " << mplInfo->name << maple::endl;
      }
    } else {
      LINKER_LOG(FATAL) << "(DEF), not resolved, " << fromUndef << ", " << index << ", " << dataUndefSize << ", " <<
          dataDefSize << " in " << mplInfo->name << maple::endl;
    }
  }

  return reinterpret_cast<jclass>(klass);
}

// Locate the address for method, which defined in the .so of 'handle'.
// Also see int dladdr(void *addr, Dl_info *info)
bool LinkerInvoker::LocateAddress(const void *handle, const void *addr, LinkerLocInfo &info, bool getName) {
  LinkerMFileInfo *mplInfo = GetLinkerMFileInfo(kFromHandle, handle);
  if (UNLIKELY(mplInfo != nullptr)) {
    return LocateAddress(&mplInfo, addr, info, getName);
  }
  return false;
}

void LinkerInvoker::ResolveColdClassSymbol(jclass cls) {
  MClass *klass = reinterpret_cast<MClass*>(cls);
  static std::mutex resolveClassSymbolMutex;
  LinkerMFileInfo *mplInfo = SearchAddress(*klass, kTypeClass);
  if (mplInfo == nullptr) {
    LINKER_DLOG(mpllinker) << "not find so for class=" << klass->GetName() << maple::endl;
    return;
  }
  {
    std::lock_guard<std::mutex> lock(resolveClassSymbolMutex);
    if (!klass->IsColdClass()) {
      return;
    }

#ifdef LINKER_DECOUPLE
    if (mplInfo->IsFlag(kIsLazy) && mplInfo->GetDecoupleLevel() != 0) {
      LINKER_LOG(FATAL) << "failed to resolve class = " << klass->GetName() << " for " << mplInfo->name <<
          ", lazy=" << klass->IsLazyBinding() << ", cold=" << klass->IsColdClass() << ", decouple=" <<
          klass->IsDecouple() << maple::endl;
    }
#endif

    __MRT_Profile_ClassMeta(*klass);
#ifdef LINKER_DECOUPLE
    if (!MRT_CLASS_IS_DECOUPLE(klass)) {
      ResolveVTableSymbolByClass(*mplInfo, klass, true);
      ResolveVTableSymbolByClass(*mplInfo, klass, false);
    }
#else
  ResolveVTableSymbolByClass(*mplInfo, klass, true);
  ResolveVTableSymbolByClass(*mplInfo, klass, false);
#endif

    ResolveSuperClassSymbolByClass(*mplInfo, klass);
    klass->ReSetFlag(0xF7FF); // 0xF7FF is Cold Flag, Clear Cold Flag
  }
}

jclass LinkerInvoker::InvokeClassLoaderLoadClass(jobject clsLoader, const std::string &className) {
  MObject *classLoader = reinterpret_cast<MObject*>(clsLoader);
  MethodMeta *method = nullptr;
  MClass *classLoaderClass = reinterpret_cast<MClass*>(MRT_ReflectGetObjectClass(
      reinterpret_cast<jobject>(classLoader)));

  // To find the loadClass() method (for the classLoader class) in cached map.
  method = mplClassLoaderLoadClassMethodMap.Find(classLoaderClass);
  // If no cached method found
  if (method == nullptr) {
    // Find the loadClass() method in classLoader class or its super class.
    method = classLoaderClass->GetMethod("loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    if (method == nullptr) {
      LINKER_LOG(ERROR) << "failed, MethodMeta is null, " << classLoaderClass->GetName() << maple::endl;
      return nullptr;
    }
    // Insert it into cached map
    mplClassLoaderLoadClassMethodMap.Append(classLoaderClass, method);
  }

  std::string dotName = nameutils::SlashNameToDot(className);
  MClass *ret = nullptr;
  ScopedHandles sHandles;
  ObjHandle<MObject, false> classLoaderRef(classLoader);
  ObjHandle<MString> javaClassName(NewStringUTF(dotName.c_str(), static_cast<int>(dotName.length())));
  // Method shouldn't be nullptr
  if (!method->NeedsInterp()) { // Method is in compiled code
    uintptr_t funcAddr = method->GetFuncAddress();
    ret = RuntimeStub<MClass*>::FastCallCompiledMethod(funcAddr, classLoaderRef(), javaClassName());
  } else { // Method needs to be interpreted
    address_t param = javaClassName.AsRaw();
    ret = reinterpret_cast<MClass*>(MRT_ReflectInvokeMethodAjobject(
        classLoaderRef.AsJObj(), reinterpret_cast<jmethodID>(method), reinterpret_cast<jvalue*>(&param)));
  }
  LINKER_VLOG(lazybinding) << "(" << dotName << "), classLoader=" << classLoaderRef() << ", ret=" << ret <<
      ", has exception=" << MRT_HasPendingException() << maple::endl;
  ObjHandle<MObject> ex(MRT_PendingException());
  if (ex() != 0) {
    if (VLOG_IS_ON(lazybinding)) {
      std::string exceptionString;
      MRT_DumpException(reinterpret_cast<jthrowable>(ex()), &exceptionString);
      LINKER_VLOG(mpllinker) << "(" << dotName << "), exception=" << exceptionString.c_str() << maple::endl;
      LINKER_LOG(ERROR) << "(), Pending Exception: " << reinterpret_cast<MObject*>(ex())->GetClass()->GetName() <<
          ", with classLoader=" << classLoaderRef() << maple::endl;
    }
    MRT_ClearPendingException(); // Not throw by MRT_ThrowException_Unw((MObject*)ex);
    // not dec ret because its class type and off heap
    return nullptr; // Not found
  }
  return reinterpret_cast<jclass>(ret);
}

void *LinkerInvoker::LookUpSymbolAddress(const MUID &muid) {
  return Get<Linker>()->LookUpSymbolAddress(muid);
}

MUID LinkerInvoker::GetMUID(const std::string symbol, bool forSystem) {
  MUID muid;
  if (symbol.empty()) {
    LINKER_LOG(ERROR) << "failed, symbol is null." << maple::endl;
    return muid; // All fields are 0
  }
  GenerateMUID(symbol.c_str(), muid);

  if (forSystem) {
    muid.SetSystemNameSpace();
  } else {
    muid.SetApkNameSpace();
  }
  return muid;
}

bool LinkerInvoker::Add(ObjFile &objFile, jobject classLoader) {
  std::lock_guard<std::mutex> lock(mLinkLock);
  // Filter non-maple .so out.
  //
  // Maple so includes two type:
  // Type 1: maple so
  //         e.g. /system/lib64/libmaplecore-all.so
  //              /system/lib64/libmapleframework.so
  // Type 2: apk wrapped maple so
  //         e.g. /system/priv-app/HwSystemServer/HwSystemServer.apk!/maple/arm64/mapleclasses.so
  //              /system/app/KeyChain/KeyChain.apk!/maple/arm64/mapleclasses.so
  const void *handle = objFile.GetHandle();
  std::string libName = objFile.GetName();
  LINKER_VLOG(mpllinker) << "old name=" << libName << ", handle=" << handle << ", cl=" << classLoader << maple::endl;
  std::string::size_type index = libName.rfind('/');
  if (index == std::string::npos) {
    libName = maple::fs::kSystemLibPath + libName;
  }
  // Eliminate . and ..
  if (libName.find("/./") != std::string::npos || libName.find("/../") != std::string::npos) {
    if (libName.length() > PATH_MAX) {
      LINKER_LOG(ERROR) << "failed: path exceeds limit! " << libName.length() << ", " << libName << maple::endl;
      return false;
    }
    char canonical[PATH_MAX + 1] = { 0 };
    if (realpath(libName.c_str(), canonical)) {
      libName = canonical;
    } else {
      return false;
    }
  }

  MFileInfoSource searchKey[] = { kFromHandle, kFromName };
  const void *data[] = { handle, libName.c_str() };
  for (uint32_t i = 0; i < sizeof(searchKey) / sizeof(MFileInfoSource); ++i) {
    auto resInfo = GetLinkerMFileInfo(searchKey[i], data[i]);
    if (resInfo != nullptr) {
      objFile.SetMplInfo(*resInfo);
      LINKER_VLOG(mpllinker) << "failed:" << libName << ", handle=" << handle << ", cl=" << classLoader << maple::endl;
      return false;
    }
  }

  LINKER_VLOG(mpllinker) << "new name=" << libName << ", handle=" << handle << ", cl=" << classLoader << maple::endl;
  CreateMplInfo(objFile, classLoader);
  return true;
}

void LinkerInvoker::CreateMplInfo(ObjFile &objFile, jobject classLoader) {
  LinkerMFileInfo *mplInfo;
#ifdef LINKER_DECOUPLE
  mplInfo = new (std::nothrow) DecoupleMFileInfo();
#else
  mplInfo = new (std::nothrow) LinkerMFileInfo();
#endif
  if (mplInfo == nullptr) {
    LINKER_LOG(FATAL) << "new mplInfo failed" << maple::endl;
  }
  mplInfo->name = objFile.GetName();
  mplInfo->handle = const_cast<void*>(objFile.GetHandle());
  mplInfo->classLoader = classLoader;
  mplInfo->SetFlag(kIsMethodDefResolved, false);
  mplInfo->SetFlag(kIsDataDefResolved, false);
  mplInfo->SetFlag(kIsVTabResolved, false);
  mplInfo->SetFlag(kIsITabResolved, false);
  int32_t pos = -1; // pos -1 means isn't hotfix
  if (objFile.GetUniqueID() == 0) { // hotfix
    pos = 0;
  }
  if (Get<Linker>()->InitLinkerMFileInfo(*mplInfo, pos)) {
    objFile.SetLazyBinding();
  }
  objFile.SetMplInfo(*mplInfo);
}

// Resolve all undefined symbols for all the maple .so.
bool LinkerInvoker::Resolve() {
  std::lock_guard<std::mutex> lock(mLinkLock);
  bool ret = true;
  if (!Get<Linker>()->HandleSymbol()) {
    ret = false;
  }
#ifdef LINKER_DECOUPLE
  if (!Get<Decouple>()->HandleDecouple()) {
    ret = false;
  }
#endif
  return ret;
}

// Resolve all undefined symbols for the single maple .so of 'handle'.
bool LinkerInvoker::Resolve(LinkerMFileInfo &mplInfo, bool decouple) {
  std::lock_guard<std::mutex> lock(mLinkLock);
  bool ret = true;
  if (!Get<Linker>()->HandleSymbol(mplInfo)) {
    ret = false;
  }
#ifdef LINKER_DECOUPLE
  if (decouple && !Get<Decouple>()->HandleDecouple(&mplInfo)) {
    ret = false;
  }
#else
  (void)decouple;
#endif

  LINKER_VLOG(mpllinker) << "pre clinit " << mplInfo.name << maple::endl;
  // invoke __MRT_PreinitModuleClasses() to preinit specified classes for current .so file
  (void)GetSymbolAddr(mplInfo.handle, "MRT_PreinitModuleClasses", true);
  return ret;
}

#ifdef LINKER_DECOUPLE
bool LinkerInvoker::HandleDecouple(std::vector<LinkerMFileInfo*> &mplList) {
  if (mplList.size() > 0) {
    std::lock_guard<std::mutex> lock(mLinkLock);
    return Get<Decouple>()->HandleDecouple();
  }
  return true;
}
#endif

// Notify all resolving jobs finished.
void LinkerInvoker::FinishLink(jobject classLoader) {
  std::lock_guard<std::mutex> lock(mLinkLock);
  Get<Linker>()->FreeAllCacheTables(reinterpret_cast<MObject*>(classLoader));
}

// Link all the maple .so library added by Add() before.
bool LinkerInvoker::Link() {
  return Resolve();
}

// Just resolve the single maple file. MUST invoked Add() before.
bool LinkerInvoker::Link(LinkerMFileInfo &mplInfo, bool decouple) {
  return Resolve(mplInfo, decouple);
}

void LinkerInvoker::SetLoadState(LoadStateType state) {
  maplert::linkerutils::SetLoadState(state);
#ifdef LINKER_RT_CACHE
  Get<LinkerCache>()->Reset();
#endif // LINKER_RT_CACHE
}

void LinkerInvoker::SetLinkerMFileInfoClassLoader(const ObjFile &objFile, jobject classLoader) {
  LinkerMFileInfo *mplInfo = nullptr;
  mplInfo = GetLinkerMFileInfo(kFromHandle, objFile.GetHandle());
  if (mplInfo == nullptr) {
    LINKER_LOG(ERROR) << "handle exists, failed to change classLoader, name=" << objFile.GetName() << ", handle=" <<
        objFile.GetHandle() << ", classLoader=" << classLoader << maple::endl;
    return;
  }
  LinkerMFileInfo *mplInfo2 = GetLinkerMFileInfoByName(objFile.GetName());
  if (mplInfo2 == nullptr || mplInfo2 != mplInfo) {
    LINKER_LOG(ERROR) << "name not exists or not equal with handle, " << "failed to change classLoader, name=" <<
        objFile.GetName() << ", handle=" << objFile.GetHandle() << ", classLoader=" << classLoader << maple::endl;
    return;
  }
  mplInfo->classLoader = classLoader;
}

void LinkerInvoker::SetClassLoaderParent(jobject classLoader, jobject newParent) {
  std::lock_guard<std::mutex> lock(mLinkLock);
  Get<Hotfix>()->SetClassLoaderParent(reinterpret_cast<MObject*>(classLoader), reinterpret_cast<MObject*>(newParent));
}

bool LinkerInvoker::InsertClassesFront(ObjFile &objFile, jobject classLoader) {
  if (!Add(objFile, classLoader)) {
    LINKER_LOG(ERROR) << "InsertClassesFront failed" << maple::endl;
    return false;
  }
  std::lock_guard<std::mutex> lock(mLinkLock);
  Get<Hotfix>()->InsertClassesFront(reinterpret_cast<MObject*>(classLoader), *(objFile.GetMplInfo()),
      objFile.GetName());
  return true;
}
bool LinkerInvoker::IsFrontPatchMode(const std::string &path) {
  std::lock_guard<std::mutex> lock(mLinkLock);
  return Get<Hotfix>()->IsFrontPatchMode(path);
}
void LinkerInvoker::SetPatchPath(std::string &path, int32_t mode) {
  std::lock_guard<std::mutex> lock(mLinkLock);
  Get<Hotfix>()->SetPatchPath(path, mode);
}

void LinkerInvoker::InitArrayCache(uintptr_t pc, uintptr_t addr) {
  LinkerLocInfo locInfo;
  LinkerMFileInfo *mplInfo = nullptr;
  DataRefOffset *srcAddr = reinterpret_cast<DataRefOffset*>(addr);
  bool isJava = GetJavaTextInfo(reinterpret_cast<uint8_t*>(pc), &mplInfo, locInfo, false);
  if (!isJava) {
    MRT_ThrowNullPointerExceptionUnw();
    return;
  }
  DataRefOffset *tabStart = mplInfo->GetTblBegin<DataRefOffset*>(kArrayClassCacheIndex);
  size_t tableSize = mplInfo->GetTblSize(kArrayClassCacheIndex);
  if (tabStart > srcAddr ||
      ((reinterpret_cast<uintptr_t>(tabStart) + tableSize) <= reinterpret_cast<uintptr_t>(srcAddr))) {
    MRT_ThrowNullPointerExceptionUnw();
    return;
  }

  size_t index = srcAddr - tabStart;
  DataRefOffset *classNameTabStart = mplInfo->GetTblBegin<DataRefOffset*>(kArrayClassCacheNameIndex);
  DataRefOffset *classNameItem = classNameTabStart + index;
  char *className = classNameItem->GetDataRef<char*>();
  uint64_t *md = JavaFrame::GetMethodMetadata(reinterpret_cast<uint32_t*>(locInfo.addr));
  MClass *callerCls = reinterpret_cast<MClass*>(JavaFrame::GetDeclaringClass(md));
  MClass *classArrayCache = MClass::GetClassFromDescriptor(callerCls, className);
  if (classArrayCache == nullptr) {
    MRT_CheckThrowPendingExceptionUnw();
    return;
  }
  srcAddr->SetRawValue(classArrayCache->AsUintptr());
}
#ifdef __cplusplus
extern "C" {
#endif
__attribute__((aligned(4096), visibility("default")))
uint8_t __BindingProtectRegion__[kBindingStateMax] = { 0 };

static int64_t sigvLazyBindingCountCinfUndef = 0;
static int64_t sigvLazyBindingCountCinfDef = 0;
static int64_t sigvLazyBindingCountDataUndef = 0;
static int64_t sigvLazyBindingCountDataDef = 0;
static int64_t sigvLazyBindingCountMethodUndef = 0;
static int64_t sigvLazyBindingCountMethodDef = 0;

static inline int64_t IncSigvLazyBindingCount(int64_t &count) {
  return __atomic_add_fetch(&count, 1, __ATOMIC_ACQ_REL);
}

void InitProtectedRegion() {
  if (mprotect(__BindingProtectRegion__, 4096, PROT_NONE)) { // 4096 is 2^12, region page size is 4k.
    LINKER_LOG(ERROR) << "protect __BindingProtectRegion__ failed" << maple::endl;
  }
  LINKER_VLOG(lazybinding) << "__BindingProtectRegion__=" << static_cast<void*>(__BindingProtectRegion__) <<
      maple::endl;
}

bool MRT_RequestLazyBindingForSignal(const SignalInfo &data) {
  void *pc = data.pc;
  void *offset = data.offset;
  delete &data;
  return MRT_RequestLazyBinding(offset, pc, true);
}

bool MRT_RequestLazyBindingForInitiative(const void *data) {
  LinkerMFileInfo *mplInfo = LinkerAPI::As<LinkerInvoker&>().GetLinkerMFileInfoByAddress(data, true);
  if (mplInfo == nullptr) {
    LINKER_VLOG(lazybinding) << "data=" << data << ", not found LinkerMFileInfo." << maple::endl;
    return false;
  }
  return MRT_RequestLazyBinding(data, 0, false);
}

bool MRT_RequestLazyBinding(const void *offset, const void *pc, bool fromSignal) {
  BindingState state =
      LinkerAPI::As<LinkerInvoker&>().GetAddrBindingState(*(reinterpret_cast<const LinkerVoidType*>(offset)));
  LINKER_VLOG(lazybinding) << "*offset=" <<
      reinterpret_cast<void*>(*(reinterpret_cast<const LinkerVoidType*>(offset))) <<
      ", __BindingProtectRegion__=" << reinterpret_cast<void*>(__BindingProtectRegion__) << maple::endl;
  bool res = false;
  LazyBinding *lazyBinding = LinkerAPI::As<LinkerInvoker&>().Get<LazyBinding>();
  switch (state) {
    case kBindingStateCinfUndef:
      LINKER_VLOG(lazybinding) << "cinf undef SEGV count=" <<
          IncSigvLazyBindingCount(sigvLazyBindingCountCinfUndef) << maple::endl;
      res = lazyBinding->HandleSymbol(offset, pc, state, fromSignal);
      break;
    case kBindingStateDataUndef:
      LINKER_VLOG(lazybinding) << "data undef SEGV count=" <<
          IncSigvLazyBindingCount(sigvLazyBindingCountDataUndef) << maple::endl;
      res = lazyBinding->HandleSymbol(offset, pc, state, fromSignal);
      break;
    case kBindingStateCinfDef:
      LINKER_VLOG(lazybinding) << "cinf def SEGV count=" <<
          IncSigvLazyBindingCount(sigvLazyBindingCountCinfDef) << maple::endl;
      res = lazyBinding->HandleSymbol(offset, pc, state, fromSignal);
      break;
    case kBindingStateDataDef:
      LINKER_VLOG(lazybinding) << "data def SEGV count=" <<
          IncSigvLazyBindingCount(sigvLazyBindingCountDataDef) << maple::endl;
      res = lazyBinding->HandleSymbol(offset, pc, state, fromSignal);
      break;
    case kBindingStateMethodUndef:
      LINKER_VLOG(lazybinding) << "method undef SEGV count=" <<
          IncSigvLazyBindingCount(sigvLazyBindingCountMethodUndef) << maple::endl;
      res = lazyBinding->HandleSymbol(offset, pc, state, fromSignal);
      break;
    case kBindingStateMethodDef:
      LINKER_VLOG(lazybinding) << "method def SEGV count=" <<
          IncSigvLazyBindingCount(sigvLazyBindingCountMethodDef) << maple::endl;
      res = lazyBinding->HandleSymbol(offset, pc, state, fromSignal);
      break;
    default:
      // Handle exception here...
      if (VLOG_IS_ON(lazybinding)) {
        LINKER_LOG(ERROR) << "wrong state " << static_cast<int>(state) << "! offset=" << offset << ", pc=" << pc <<
            ", __BindingProtectRegion__=" << reinterpret_cast<void*>(__BindingProtectRegion__) << maple::endl;
        LinkerAPI::As<LinkerInvoker&>().DumpStackInfoInLog();
      }
      break;
  }
  return res;
}

int32_t MCC_FixOffsetTableVtable(uint32_t offsetVal, char *offsetEntry) {
#ifdef LINKER_DECOUPLE
  Decouple *decouple = LinkerAPI::As<LinkerInvoker&>().Get<Decouple>();
  // highest bit is fix flag
  const uint32_t kMplOffsetFixFlag = 0x80000000;
  (void)offsetVal; // placeholder para
  int32_t offsetValue = *(reinterpret_cast<int32_t*>(offsetEntry));
  if (static_cast<uint32_t>(offsetValue) & kMplOffsetFixFlag) {
    uint32_t offsetIndex = (static_cast<uint32_t>(offsetValue) & (~kMplOffsetFixFlag));
    LinkerOffsetValItem tmpItem;
    offsetEntry -= reinterpret_cast<char*>(&tmpItem.offset) - reinterpret_cast<char*>(&tmpItem);
    LinkerOffsetValItem *offsetTable = reinterpret_cast<LinkerOffsetValItem*>(offsetEntry) - offsetIndex;
    LinkerOffsetKeyTableInfo *keyTableInfo = reinterpret_cast<LinkerOffsetKeyTableInfo*>(offsetTable) - 1;
    if (offsetIndex >= keyTableInfo->vtableOffsetTableSize) {
      LINKER_LOG(FATAL) << "offsetIndex = " << offsetIndex << maple::endl;
    }
    return decouple->FixOffsetTableLazily(reinterpret_cast<char*>(offsetTable), offsetIndex);
  } else {
    return offsetValue;
  }
#else
  (void)offsetVal;
  (void)offsetEntry;
  return -1;
#endif
}

int32_t MCC_FixOffsetTableField(uint32_t offsetVal, char *offsetEntry) {
#ifdef LINKER_DECOUPLE
  Decouple *decouple = LinkerAPI::As<LinkerInvoker&>().Get<Decouple>();
  // highest bit is fix flag
  const uint32_t kMplOffsetFixFlag = 0x80000000;
  (void)offsetVal; // placeholder para
  uint32_t offsetValue = *(reinterpret_cast<uint32_t*>(offsetEntry));
  if (offsetValue & kMplOffsetFixFlag) {
    uint32_t offsetIndex = (offsetValue & (~kMplOffsetFixFlag));
    LinkerOffsetValItem tmpItem;
    offsetEntry -= reinterpret_cast<char*>(&tmpItem.offset) - reinterpret_cast<char*>(&tmpItem);
    LinkerOffsetValItem *offsetTable = reinterpret_cast<LinkerOffsetValItem*>(offsetEntry) - offsetIndex;
    LinkerOffsetKeyTableInfo *keyTableInfo = reinterpret_cast<LinkerOffsetKeyTableInfo*>(offsetTable) - 1;
    if (offsetIndex < keyTableInfo->vtableOffsetTableSize) {
      LINKER_LOG(FATAL) << "offsetIndex = " << offsetIndex << maple::endl;
    }
    return decouple->FixOffsetTableLazily(reinterpret_cast<char*>(offsetTable), offsetIndex);
  } else {
    return static_cast<int32_t>(offsetValue);
  }
#else
  (void)offsetVal;
  (void)offsetEntry;
  return -1;
#endif
}

void MRT_FixOffsetTableLazily(LinkerOffsetValItemLazyLoad &offsetEntry) {
#ifdef LINKER_DECOUPLE
  Decouple *decouple = LinkerAPI::As<LinkerInvoker&>().Get<Decouple>();
  // highest bit is fix flag
  const uint32_t kMplOffsetFixFlag = 0x80000000;
  uint32_t offsetValue = static_cast<uint32_t>(offsetEntry.offset);
  if (offsetValue & kMplOffsetFixFlag) {
    uint32_t offsetIndex = (offsetValue & (~kMplOffsetFixFlag));
    (void)decouple->FixOffsetTableLazily(reinterpret_cast<char*>(&offsetEntry - offsetIndex), offsetIndex);
  }
#else
  (void)offsetEntry;
#endif
}

void MRT_FixStaticAddrTable(LinkerStaticAddrItem &addrTableItem) {
#ifdef LINKER_DECOUPLE
  Decouple *decouple = LinkerAPI::As<LinkerInvoker&>().Get<Decouple>();
  MplStaticDecouple &staticResolver = decouple->GetStaticResolver();
  uint32_t signalIndex = static_cast<uint32_t>(addrTableItem.index);
  MplStaticAddrTabHead *staticAddrTabHead = reinterpret_cast<MplStaticAddrTabHead*>(&addrTableItem - signalIndex) - 1;
  LinkerMFileInfo *mplInfo = staticResolver.GetLinkerMFileInfoFromHead(*staticAddrTabHead);
  LinkerStaticAddrItem *addrTableItems = reinterpret_cast<LinkerStaticAddrItem*>(&addrTableItem - signalIndex);
  LinkerStaticDecoupleClass *keyTableItems = mplInfo->GetTblBegin<LinkerStaticDecoupleClass*>(kStaticDecoupleKey);
  size_t keyTableSize = mplInfo->GetTblSize(kStaticDecoupleKey);
  if (UNLIKELY(addrTableItems == nullptr || keyTableItems == nullptr || keyTableSize == 0)) {
    LINKER_LOG(FATAL) << "Fail to get static address table from " << mplInfo->name << maple::endl;
    return;
  }
  bool resolved = staticResolver.FixClassClinit(*keyTableItems, *addrTableItems, signalIndex, false);
  LINKER_VLOG(staticdcp) << "exit. class initialized=" << resolved << maple::endl;
#else
  (void)addrTableItem;
#endif
}

bool MRT_IsLazyBindingState(const uint8_t *address) {
  if (address >= __BindingProtectRegion__ && address < &__BindingProtectRegion__[kBindingStateMax]) {
    return true;
  }
  return false;
}

void MRT_FixStaticAddrTableLazily(LinkerStaticAddrItem &addrTableItem) {
#ifdef LINKER_DECOUPLE
  Decouple *decouple = LinkerAPI::As<LinkerInvoker&>().Get<Decouple>();
  MplStaticDecouple &staticResolver = decouple->GetStaticResolver();
  LINKER_VLOG(staticdcp) << "entered, addrTableItem=" << std::hex << &addrTableItem << maple::endl;
  uint32_t signalIndex = static_cast<uint32_t>(addrTableItem.index);
  MplStaticAddrTabHead *staticAddrTabHead = reinterpret_cast<MplStaticAddrTabHead*>(&addrTableItem - signalIndex) - 1;
  LinkerMFileInfo *mplInfo = staticResolver.GetLinkerMFileInfoFromHead(*staticAddrTabHead);
  LinkerStaticAddrItem *addrTableItems = reinterpret_cast<LinkerStaticAddrItem*>(&addrTableItem - signalIndex);
  LinkerStaticDecoupleClass *keyTableItems = mplInfo->GetTblBegin<LinkerStaticDecoupleClass*>(kStaticDecoupleKey);
  size_t keyTableSize = mplInfo->GetTblSize(kStaticDecoupleKey);
  if (UNLIKELY(addrTableItems == nullptr || keyTableItems == nullptr || keyTableSize == 0)) {
    LINKER_LOG(FATAL) << "Fail to get static address table from " << mplInfo->name << " keyTableSize=" <<
        keyTableSize << " addrTableItems=" << std::hex << addrTableItems << " keyTableItems=" << keyTableItems <<
        maple::endl;
    return;
  }

  int32_t index = staticResolver.GetClassInfoIndex(*addrTableItems, signalIndex);
  ClassMetadata *clsCallee = staticResolver.GetClassMetadata(mplInfo, keyTableItems[index].callee);
  if (UNLIKELY(clsCallee == nullptr)) {
    LINKER_LOG(FATAL) << "exit: null pointer to current class" << maple::endl;
    return;
  }
  bool isResolved = true;
  if (static_cast<int>(signalIndex) <= (index + static_cast<int>(keyTableItems[index].fieldsNum))) {
    isResolved = staticResolver.SetStaticFieldAddr(mplInfo, *clsCallee,
        keyTableItems[signalIndex], addrTableItems[signalIndex]);
  } else {
    isResolved = staticResolver.SetStaticMethodAddr(mplInfo, *clsCallee,
        keyTableItems[signalIndex], addrTableItems[signalIndex]);
  }
  bool initialized = staticResolver.FixClassClinit(*keyTableItems, *addrTableItems, signalIndex, true);
  LINKER_VLOG(staticdcp) << "exit. isResolved = " << isResolved << " clinit = " << initialized << maple::endl;
#else
  (void)addrTableItem;
#endif
}

// init array class cache reference by compiler code
bool MRT_RequestInitArrayCache(SignalInfo *info) {
  uintptr_t pc = reinterpret_cast<uintptr_t>(info->pc);
  // array cache addr
  uintptr_t addr = reinterpret_cast<uintptr_t>(info->offset);
  delete info;
  LinkerAPI::As<LinkerInvoker&>().InitArrayCache(pc, addr);
  return true;
}
#ifdef __cplusplus
}
#endif
} // namespace maple

