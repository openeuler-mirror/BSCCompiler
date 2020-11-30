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
#include "linker/linker_lazy_binding.h"

#include "linker/linker_model.h"
#include "linker/linker_inline.h"
#include "linker/linker_method_builder.h"
#ifdef LINKER_DECOUPLE
#include "linker/decouple/linker_decouple.h"
#include "linker/decouple/linker_field.h"
#include "linker/decouple/linker_gctib.h"
#endif
#include "loader_api.h"
#include "loader/object_loader.h"
#include "chelper.h"
#include "object_base.h"
#include "mrt_class_api.h"
#include "mrt_reflection_method.h"
#include "fieldmeta_inline.h"
#include "exception/mrt_exception.h"
#include "exception/stack_unwinder.h"
#include "mrt_well_known.h"

using namespace maple;

namespace maplert {
using namespace linkerutils;
FeatureName LazyBinding::featureName = kFLazyBinding;
// In most cases, decoupling routine don't call this function here.
// We just add this procedure in case.
bool LazyBinding::HandleSymbolForDecoupling(LinkerMFileInfo &mplInfo, const AddrSlice &addrSlice, size_t index) {
  BindingState state = GetAddrBindingState(addrSlice, index);
  LINKER_VLOG(lazybinding) << "state=" << static_cast<int>(state) << ", addr=" << addrSlice.Data() << ", index=" <<
      index << maple::endl;
  return HandleSymbol(mplInfo, addrSlice.Data() + index, nullptr, state, true);
}

bool LazyBinding::HandleSymbol(const void *offset, const void *pc, BindingState state, bool fromSignal) {
  LinkerMFileInfo *mplInfo = pInvoker->GetLinkerMFileInfo(kFromAddr, offset, true);
  if (mplInfo == nullptr) {
    pInvoker->DumpStackInfoInLog();
    LINKER_DLOG(lazybinding) << "--end--/" << offset << ", " << static_cast<int>(state) << maple::endl;
    LINKER_LOG(FATAL) << "failed, offset=" << offset << ", pc=" << pc << ", state=" << static_cast<int>(state) <<
        ", mplInfo is null." << maple::endl;
  }
  return HandleSymbol(*mplInfo, offset, pc, state, fromSignal);
}

void LazyBinding::GetAddrAndMuidSlice(LinkerMFileInfo &mplInfo,
    BindingState state, AddrSlice &addrSlice, MuidSlice &muidSlice, const void *offset) {
  size_t tableSize = 0;
  size_t muidSize = 0;
  switch (state) {
    case kBindingStateCinfUndef:
    case kBindingStateDataUndef:
      tableSize = mplInfo.GetTblSize(kDataUndef);
      addrSlice = AddrSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kDataUndef), tableSize);
      muidSize = mplInfo.GetTblSize(kDataUndefMuid);
      muidSlice = MuidSlice(mplInfo.GetTblBegin<LinkerMuidTableItem*>(kDataUndefMuid), muidSize);
      break;
    case kBindingStateCinfDef:
    case kBindingStateDataDef:
      tableSize = mplInfo.GetTblSize(kDataDef);
      addrSlice = AddrSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kDataDef), tableSize);
      muidSize = mplInfo.GetTblSize(kDataDefMuid);
      muidSlice = MuidSlice(mplInfo.GetTblBegin<LinkerMuidTableItem*>(kDataDefMuid), muidSize);
      break;
    case kBindingStateMethodUndef:
      tableSize = mplInfo.GetTblSize(kMethodUndef);
      addrSlice = AddrSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kMethodUndef), tableSize);
      muidSize = mplInfo.GetTblSize(kMethodUndefMuid);
      muidSlice = MuidSlice(mplInfo.GetTblBegin<LinkerMuidTableItem*>(kMethodUndefMuid), muidSize);
      break;
    case kBindingStateMethodDef:
      tableSize = mplInfo.GetTblSize(kMethodDef);
      addrSlice = AddrSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kMethodDef), tableSize);
      muidSize = mplInfo.GetTblSize(kMethodDefMuid);
      muidSlice = MuidSlice(mplInfo.GetTblBegin<LinkerMuidTableItem*>(kMethodDefMuid), muidSize);
      break;
    default:
      LINKER_LOG(ERROR) << "failed, offset=" << offset << ", state=" << static_cast<int>(state) << maple::endl;
      return;
  }
}

void *LazyBinding::ResolveClassSymbolClassification(
    LinkerMFileInfo &mplInfo, BindingState state, const AddrSlice &addrSlice, const MuidSlice &muidSlice,
    const size_t &index, MObject *candidateClassLoader, bool &fromUpper, const void *pc, const void *offset) {
  void *data = nullptr;
  switch (state) {
    case kBindingStateCinfUndef:
      if (mplInfo.IsFlag(kIsLazy) && mplInfo.classLoader == nullptr) {
        candidateClassLoader = pInvoker->GetClassLoaderByAddress(mplInfo, pc);
        LINKER_VLOG(lazybinding) << "candidateClassLoader=" << candidateClassLoader << maple::endl;
      }
      data = ResolveClassSymbol(mplInfo, addrSlice, muidSlice, index, candidateClassLoader, fromUpper, false, true);
      break;
    case kBindingStateCinfDef:
      data = ResolveClassSymbol(mplInfo, addrSlice, muidSlice, index, candidateClassLoader, fromUpper, true, true);
      break;
    case kBindingStateDataUndef:
      data = ResolveDataSymbol(mplInfo, addrSlice, muidSlice, index, false);
      break;
    case kBindingStateDataDef:
      data = ResolveDataSymbol(mplInfo, addrSlice, muidSlice, index, true);
      break;
    case kBindingStateMethodUndef:
      data = ResolveMethodSymbol(mplInfo, addrSlice, muidSlice, index, false, true);
      break;
    case kBindingStateMethodDef:
      data = ResolveMethodSymbol(mplInfo, addrSlice, muidSlice, index, true, true);
      break;
    default:
      // Never reach here.
      break;
  }

  if (data == nullptr) {
    LINKER_LOG(ERROR) << "failed, data returns null, offset=" << offset << ", " << mplInfo.name << maple::endl;
    pInvoker->DumpStackInfoInLog();
  }
  return data;
}

bool LazyBinding::HandleSymbol(
    LinkerMFileInfo &mplInfo, const void *offset, const void *pc, BindingState state, bool fromSignal) {
  LINKER_DLOG(lazybinding) << "--start--/" << offset << ", " << state << ", " << fromSignal << maple::endl;
  AddrSlice addrSlice;
  MuidSlice muidSlice;
  GetAddrAndMuidSlice(mplInfo, state, addrSlice, muidSlice, offset);

  if (pInvoker->GetMultiSoPendingCount() > 0) {
    if (state == kBindingStateCinfUndef || state == kBindingStateDataUndef || state == kBindingStateMethodUndef) {
      LINKER_LOG(ERROR) << "waiting for startup asynchronism procedure, " << "name=" << mplInfo.name <<
          ", multiSoPendingCount=" << pInvoker->GetMultiSoPendingCount() << maple::endl;
      (void)MRT_EnterSaferegion(!fromSignal);
      while (pInvoker->GetMultiSoPendingCount() > 0) {
        constexpr int oneMillisecond = 1000;
        (void)usleep(oneMillisecond); // Sleep 1ms
      }
      (void)MRT_LeaveSaferegion();
    }
  }

  size_t index = GetAddrIndex(mplInfo, addrSlice, offset);
  if (GetAddrBindingState(addrSlice, index, true) == kBindingStateResolved) {
    LINKER_VLOG(lazybinding) << "resolved, state=" << state << ", index=" << index << mplInfo.name << maple::endl;
    return true;
  }
  if (index == static_cast<size_t>(-1)) {
    pInvoker->DumpStackInfoInLog();
    LINKER_LOG(FATAL) << "out of range, offset=" << offset << "|" << state << "|" << mplInfo.name << maple::endl;
  }
  bool fromUpper = false;
  MObject *candidateClassLoader = nullptr;
  void *data = ResolveClassSymbolClassification(
      mplInfo, state, addrSlice, muidSlice, index, candidateClassLoader, fromUpper, pc, offset);

  if (state == kBindingStateCinfUndef || state == kBindingStateCinfDef) {
    if (!fromUpper) {
      LinkerMFileInfo *tmpLinkerMFileInfo = pInvoker->GetLinkerMFileInfo(kFromMeta, data); // Not only lazy .so.
      if (tmpLinkerMFileInfo != nullptr) {
        LinkClassInternal(*tmpLinkerMFileInfo, static_cast<MClass*>(data), candidateClassLoader, true);
      } else {
        // If the class is from the Interpreter, it can not be found in the LinkerMFileInfos list.
        // It's allowed to return success in this case and not to link it any more.
        LINKER_VLOG(lazybinding) << "class is lazy but no defined MFileInfo, from Interpreter, " << " klass=" <<
            data << ", name=" << reinterpret_cast<MClass*>(data)->GetName() << maple::endl;
      }
    }
  }
  if (SetAddrInAddrTable(addrSlice, index, reinterpret_cast<MClass*>(data)) == nullptr) {
    LINKER_LOG(ERROR) << "set addr failed, addr = " << data << maple::endl;
  }
  LINKER_VLOG(lazybinding) << "resolved " << data << ", state=" << state << ", index=" << index <<
      ", successfully in " << mplInfo.name << maple::endl;
  return true;
}

size_t LazyBinding::GetAddrIndex(
    const LinkerMFileInfo &mplInfo, const AddrSlice &addrSlice, const void *offset) {
  if (addrSlice.Data() > offset) {
    LINKER_LOG(ERROR) << "failed, pTable=" << addrSlice.Data() << ", offset=" << offset << " in " << mplInfo.name <<
        maple::endl;
    return static_cast<size_t>(-1); // Maximum value of size_t means failure.
  }

  const LinkerAddrTableItem *pAddr = static_cast<LinkerAddrTableItem*>(const_cast<void*>(offset));
  size_t index = static_cast<size_t>(pAddr - addrSlice.Data());
  LINKER_VLOG(lazybinding) << "index=" << index << maple::endl;
  return index;
}

BindingState LazyBinding::GetAddrBindingState(LinkerVoidType addr) {
  int difference = static_cast<int>(reinterpret_cast<uint8_t*>(addr) - __BindingProtectRegion__);
  if (difference >= static_cast<int>(kBindingStateCinfUndef) &&
      difference < static_cast<int>(kBindingStateResolved)) {
    return static_cast<BindingState>(difference);
  } else {
    return kBindingStateResolved;
  }
}

BindingState LazyBinding::GetAddrBindingState(const AddrSlice &addrSlice, size_t index, bool isAtomic) {
  if (isAtomic) {
    LinkerOffsetType *tmp = const_cast<LinkerOffsetType*>(&addrSlice[index].addr);
    LinkerVoidType addr = __atomic_load_n(tmp, __ATOMIC_ACQUIRE);
    return GetAddrBindingState(addr);
  } else {
    return GetAddrBindingState(static_cast<LinkerVoidType>(addrSlice[index].addr));
  }
}

BindingState LazyBinding::GetAddrBindingState(
    const LinkerMFileInfo &mplInfo, const AddrSlice &addrSlice, const void *offset) {
  size_t index = GetAddrIndex(mplInfo, addrSlice, offset);
  BindingState state = GetAddrBindingState(addrSlice, index);
  return state;
}

// Set the 'addrSlice[index].data()' as 'klass'
// addrSlice is data def|undef table, but not def original table.
// Returns the value of 'klass' in void *type.
inline void *LazyBinding::SetAddrInAddrTable(AddrSlice &addrSlice, size_t index, const MClass *addr) {
  if (addr != nullptr) {
#ifdef LINKER_32BIT_REF_FOR_DEF_UNDEF
    __atomic_store_n(&addrSlice[index].addr, pInvoker->AddrToUint32(addr), __ATOMIC_RELEASE);
#else
    __atomic_store_n(&addrSlice[index].addr, reinterpret_cast<LinkerOffsetType>(addr), __ATOMIC_RELEASE);
#endif // USE_32BIT_REF
    return addrSlice[index].Address();
  }
  return nullptr;
}

void *LazyBinding::SetClassInDefAddrTable(size_t index, const MClass *klass) {
  LinkerMFileInfo *mplInfo = pInvoker->GetLinkerMFileInfo(kFromMeta, klass, true);
  size_t dataDefSize = mplInfo->GetTblSize(kDataDef);
  AddrSlice dataDefSlice(mplInfo->GetTblBegin<LinkerAddrTableItem*>(kDataDef), dataDefSize);
  return SetAddrInAddrTable(dataDefSlice, index, klass);
}

void *LazyBinding::SearchInUndef(LinkerMFileInfo &mplInfo, const AddrSlice &addrSlice, const MuidSlice &muidSlice,
    size_t index, MObject *candidateClassLoader, bool &fromUpper, bool isDef, std::string className) {
  void *res = nullptr;
  LinkerMFileInfo *tmpLinkerMFileInfo = nullptr;
  void *addr = LookUpDataSymbolAddressLazily(nullptr, muidSlice[index].muid, &tmpLinkerMFileInfo, true);
  LINKER_LOG(WARNING) << "(" << (isDef ? "DEF" : "UNDEF") << "), double resolve MUID=" <<
      muidSlice[index].muid.ToStr() << ", addr=" << addr << ", candidateClassLoader=" << candidateClassLoader <<
      ", in " << mplInfo.name << maple::endl;
  if (addr != nullptr) { // To check candidateClassLoader != nullptr?
    if (className.empty()) {
      className = reinterpret_cast<MClass*>(addr)->GetName();
    }
    res = ResolveClassSymbolInternal(
        mplInfo, addrSlice, muidSlice, index, candidateClassLoader, fromUpper, isDef, className);
    if (res == nullptr) {
      LINKER_LOG(WARNING) << "(" << (isDef ? "DEF" : "UNDEF") << "), failed to double resolve MUID=" <<
          muidSlice[index].muid.ToStr() << ", use addr=" << addr << ", candidateClassLoader=" <<
          candidateClassLoader << ", in " << mplInfo.name << maple::endl;
      MClass *pseudo = reinterpret_cast<MClass*>(addr);
      res = reinterpret_cast<void*>(pseudo);
      if (pseudo != nullptr && !MRT_IsClassInitialized(*pseudo)) {
        LoaderAPI::Instance().SetClassCL(*pseudo, tmpLinkerMFileInfo->classLoader);
      }
      fromUpper = false;
    }
  }
  return res;
}

void *LazyBinding::ResolveClassSymbolForAPK(LinkerMFileInfo &mplInfo, size_t index, bool &fromUpper, bool isDef) {
  void *res = nullptr;
  size_t dataDefSize = mplInfo.GetTblSize(kDataDef);
  AddrSlice dataDefSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kDataDefOrig), dataDefSize);
  MClass *klass = reinterpret_cast<MClass*>(GetDefTableAddress(mplInfo, dataDefSlice,
      static_cast<size_t>(index), false));
  res = reinterpret_cast<void*>(klass);
  if (klass != nullptr && !MRT_IsClassInitialized(*klass)) {
    LoaderAPI::Instance().SetClassCL(*klass, mplInfo.classLoader);
    LINKER_VLOG(lazybinding) << isDef << "for apk loading, klass=" << klass->GetName() << "in" <<
        mplInfo.name.c_str() << maple::endl;
  }
  fromUpper = false;
  return res;
}

// Just resolve without checking.
void *LazyBinding::ResolveClassSymbol(LinkerMFileInfo &mplInfo, const AddrSlice &addrSlice, const MuidSlice &muidSlice,
    size_t index, MObject *candidateClassLoader, bool &fromUpper, bool isDef, bool clinit) {
  void *res = nullptr;
  // Optimize for app loading.
  // WARNING: On the premise of that we've filtered out the class in APK,
  //          which is already defined in boot class loader jar path.
  if (isDef && GetAppLoadState() == kAppLoadBaseOnly) {
    res = ResolveClassSymbolForAPK(mplInfo, index, fromUpper, isDef);
    return res;
  }

  // Building class loader hierarchy.
  ClassLoaderListT classLoaderList;
  pInvoker->GetClassLoaderList(mplInfo, classLoaderList);
  // Look up the address with class loader.
  string className;
  for (jobject &loader : classLoaderList) {
    MClass *pseudo = nullptr;
    MObject *classLoader = reinterpret_cast<MObject*>(loader);
    if (isDef) {
      // Check the real def table directly.
      size_t dataDefSize = mplInfo.GetTblSize(kDataDef);
      AddrSlice dataDefSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kDataDefOrig), dataDefSize);
      pseudo = reinterpret_cast<MClass*>(GetDefTableAddress(mplInfo, dataDefSlice,
          static_cast<size_t>(index), false));
    } else {
      // We must find the real cinfo firstly with its muid.
      pseudo = reinterpret_cast<MClass*>(LookUpDataSymbolAddressLazily(classLoader, muidSlice[index].muid));
    }
    if (pseudo == nullptr) {
      LINKER_VLOG(lazybinding) << isDef << ", pseudo=nil, not resolved MUID=" << muidSlice[index].muid.ToStr() <<
          ", with classloader=" << classLoader << " in " << mplInfo.name << maple::endl;
      continue;
    }
    className = pseudo->GetName();
    void *tempRes = ResolveClassSymbolInternal(
        mplInfo, addrSlice, muidSlice, index, classLoader, fromUpper, isDef, className);
    if (tempRes != nullptr) {
      res = tempRes;
    }
  }

  // Add more searching without classloader for UNDEF.
  if (!isDef && res == nullptr) {
    res = SearchInUndef(mplInfo, addrSlice, muidSlice, index, candidateClassLoader, fromUpper, isDef, className);
  }

  if (res == nullptr) {
    LINKER_LOG(ERROR) << "failed, className = " << className.c_str() << maple::endl;
    char msg[256] = { 0 }; // 256 is maxBuffSize
    (void)sprintf_s(msg, sizeof(msg), "No class definition found");
    MRT_ThrowNoClassDefFoundErrorUnw(msg);
    return nullptr;
  }
  // We will check if we can open (void)MRT_TryInitClass(res, false);
  if (clinit && (static_cast<MClass*>(res)->IsLazyBinding())) {
    LINKER_DLOG(lazybinding) << isDef << ", CLINIT for " << res << ", in " << mplInfo.name << maple::endl;
  }
  LINKER_VLOG(lazybinding) << isDef << ", success, res=" << res << " in " << mplInfo.name << maple::endl;
  return res;
}

void *LazyBinding::ResolveClassSymbolInternal(
    LinkerMFileInfo &mplInfo, const AddrSlice &addrSlice, const MuidSlice &muidSlice,
    size_t index, MObject *classLoader, bool &fromUpper, bool isDef, const std::string &className) {
  void *res = nullptr;
  MClass *klass = nullptr;
  LoaderAPI *loader = const_cast<LoaderAPI*>(pInvoker->GetLoader());
  klass = reinterpret_cast<MClass*>(loader->FindClass(className, SearchFilter(reinterpret_cast<jobject>(classLoader))));
  if (klass != nullptr) {
    LINKER_VLOG(lazybinding) << "(" << (isDef ? "DEF" : "UNDEF") << "), delegation, resolved MUID=" <<
        muidSlice[index].muid.ToStr() << ", name=" << klass->GetName() << ", addr={" <<
        addrSlice[index].Address() << "->" << klass << "},  with classloader=" << classLoader << " in " <<
        mplInfo.name << ", cold=" << klass->IsColdClass() << ", lazy=" << klass->IsLazyBinding() << ", init=" <<
        MRT_IsClassInitialized(*klass) << maple::endl;
    res = reinterpret_cast<void*>(klass);
    fromUpper = false;
    return res;
  } else {
    MObject *bootClassLoader = reinterpret_cast<MObject*>(loader->GetBootClassLoaderInstance());
    // Ignore lower classloaders
    if (classLoader != nullptr && classLoader != bootClassLoader && !pInvoker->IsSystemClassLoader(classLoader)) {
      klass = reinterpret_cast<MClass*>(pInvoker->InvokeClassLoaderLoadClass(*classLoader, className));
      if (klass != nullptr) {
        LINKER_VLOG(lazybinding) << "(" << (isDef ? "DEF" : "UNDEF") << "), route, resolved MUID=" <<
            muidSlice[index].muid.ToStr() << ", name=" << klass->GetName() << ", addr={" <<
            addrSlice[index].Address() << "->" << klass << "}, with classloader=" << classLoader << "/" <<
            bootClassLoader << "/" << loader->GetSystemClassLoader() << " in " << mplInfo.name << ", cold=" <<
            klass->IsColdClass() << ", lazy=" << klass->IsLazyBinding() << ", init=" <<
            MRT_IsClassInitialized(*klass) << maple::endl;
        LinkStaticSymbol(mplInfo, klass);
        res = reinterpret_cast<void*>(klass);
        fromUpper = true;
        return res;
      } else {
        LINKER_VLOG(lazybinding) << "(" << (isDef ? "DEF" : "UNDEF") << "), route, not resolved MUID=" <<
            muidSlice[index].muid.ToStr() << ", name=" << className << ", with classloader=" << classLoader <<
            "/" << bootClassLoader <<  "/" << loader->GetSystemClassLoader() << " in " << mplInfo.name << maple::endl;
      }
    }
  }
  return nullptr;
}

void *LazyBinding::ResolveDataSymbol(const AddrSlice &addrSlice, const MuidSlice &muidSlice, size_t index, bool isDef) {
  LinkerMFileInfo *mplInfo = pInvoker->GetLinkerMFileInfo(kFromAddr, &addrSlice, true);
  if (mplInfo == nullptr) {
    LINKER_LOG(ERROR) << "failed, " << addrSlice.Data() << ", mplInfo is null." << maple::endl;
    return nullptr;
  }
  return ResolveDataSymbol(*mplInfo, addrSlice, muidSlice, index, isDef);
}

void *LazyBinding::ResolveDataSymbol(LinkerMFileInfo &mplInfo, const AddrSlice &addrSlice,
    const MuidSlice &muidSlice, size_t index, bool isDef) {
  void *res = nullptr;
  // Optimize for app loading.
  // WARNING: On the premise of that we've filtered out the class in APK,
  //          which is already defined in boot class loader jar path.
  if (isDef) {
    if (GetAppLoadState() == kAppLoadBaseOnly) {
      size_t dataDefSize = mplInfo.GetTblSize(kDataDef);
      AddrSlice dataDefSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kDataDefOrig), dataDefSize);
      res = GetDefTableAddress(mplInfo, dataDefSlice, static_cast<size_t>(index), false);
      LINKER_DLOG(lazybinding) << isDef << ", optimized for App, resolved MUID=" << muidSlice[index].muid.ToStr() <<
          ", addr={" << addrSlice[index].Address() << "->" << res << "} in " << mplInfo.name << maple::endl;
      return res;
    }
  }

  // Building class loader hierarchy.
  ClassLoaderListT classLoaderList;
  pInvoker->GetClassLoaderList(mplInfo, classLoaderList);
  for (jobject &classLoader : classLoaderList) { // Look up the address.
    void *addr = LookUpDataSymbolAddressLazily(reinterpret_cast<MObject*>(classLoader), muidSlice[index].muid);
    if (addr != nullptr) {
      LINKER_VLOG(lazybinding) << isDef << ", resolved MUID=" << muidSlice[index].muid.ToStr() << ", addr={" <<
          addrSlice[index].Address() << "->" << addr << "}, in " << mplInfo.name << maple::endl;
      res = addr;
    }
  }
  if (!isDef && res == nullptr) { // Add more searching without classloader for UNDEF.
    LinkerMFileInfo *tmpLinkerMFileInfo = nullptr;
    void *addr = LookUpDataSymbolAddressLazily(nullptr, muidSlice[index].muid, &tmpLinkerMFileInfo, true);
    if (addr != nullptr) {
      LINKER_LOG(WARNING) << isDef << ", double resolve MUID=" << muidSlice[index].muid.ToStr() << ", addr=" << addr <<
          ", in " << mplInfo.name << ", from " << tmpLinkerMFileInfo->name << maple::endl;
      res = addr;
    }
  }

  if (res == nullptr) {
    LINKER_LOG(ERROR) << isDef << ", failed to resolve MUID=" << muidSlice[index].muid.ToStr() << " in " <<
        mplInfo.name << maple::endl;
    char msg[256] = { 0 }; // 256 is maxBuffSize
    (void)sprintf_s(msg, sizeof(msg), "No static field MUID:%s found in the class",
        muidSlice[index].muid.ToStr().c_str());
    MRT_ThrowNoSuchFieldErrorUnw(msg);
  }
  return res;
}

void *LazyBinding::ResolveMethodSymbol(
    const AddrSlice &addrSlice, const MuidSlice &muidSlice, size_t index, bool isDef) {
  LinkerMFileInfo *mplInfo = pInvoker->GetLinkerMFileInfo(kFromAddr, &addrSlice, true);
  if (mplInfo == nullptr) {
    LINKER_LOG(ERROR) << "failed, " << addrSlice.Data() << ", mplInfo is null." << maple::endl;
    return nullptr;
  }
  return ResolveMethodSymbol(*mplInfo, addrSlice, muidSlice, index, isDef, true);
}

void *LazyBinding::ResolveMethodSymbol(LinkerMFileInfo &mplInfo, const AddrSlice &addrSlice,
    const MuidSlice &muidSlice, size_t index, bool isDef, bool forClass) {
  void *res = nullptr;
  // Optimize for app loading.
  // WARNING: On the premise of that we've filtered out the class in APK,
  //          which is already defined in boot class loader jar path.
  if (isDef) {
    if (GetAppLoadState() == kAppLoadBaseOnly) {
      size_t dataDefSize = mplInfo.GetTblSize(kDataDef);
      AddrSlice dataDefSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kMethodDefOrig), dataDefSize);
      res = GetDefTableAddress(mplInfo, dataDefSlice, static_cast<size_t>(index), true);
      LINKER_DLOG(lazybinding) << isDef << forClass << ", MUID=" << muidSlice[index].muid.ToStr() <<
          ", addr={" << addrSlice[index].Address() << "->" << res << "} in " << mplInfo.name << maple::endl;
#ifdef LINKER_LAZY_BINDING_METHOD_TO_CLASS
      if (forClass) {
        ResolveMethodsClassSymbol(mplInfo, isDef, res);
      }
#endif // LINKER_LAZY_BINDING_METHOD_TO_CLASS
      return res;
    }
  }

  ClassLoaderListT classLoaderList; // Building class loader hierarchy.
  pInvoker->GetClassLoaderList(mplInfo, classLoaderList);
  for (jobject &classLoader : classLoaderList) { // Look up the address with class loader.
    void *tempRes = LookUpMethodSymbolAddressLazily(reinterpret_cast<MObject*>(classLoader), muidSlice[index].muid);
    if (tempRes != nullptr) {
      LINKER_VLOG(lazybinding) << isDef << ", MUID=" << muidSlice[index].muid.ToStr() << ", addr={" <<
          addrSlice[index].Address() << "->" << tempRes << "}, in " << mplInfo.name << maple::endl;
#ifdef LINKER_LAZY_BINDING_METHOD_TO_CLASS
      if (forClass) {
        ResolveMethodsClassSymbol(mplInfo, isDef, tempRes);
      }
#endif // LINKER_LAZY_BINDING_METHOD_TO_CLASS
      res = tempRes;
    }
  }

  if (!isDef && res == nullptr) { // Add more searching without classloader for UNDEF.
    res = LookUpMethodSymbolAddressLazily(nullptr, muidSlice[index].muid, true);
    LINKER_LOG(WARNING) << isDef << ", double resolve MUID=" << muidSlice[index].muid.ToStr() << ", addr=" << res <<
        " in " << mplInfo.name << maple::endl;
#ifdef LINKER_LAZY_BINDING_METHOD_TO_CLASS
    if (res != nullptr && forClass) {
      ResolveMethodsClassSymbol(mplInfo, isDef, res);
    }
#endif // LINKER_LAZY_BINDING_METHOD_TO_CLASS
  }

  if (res == nullptr) {
    char msg[256] = { 0 }; // 256 is maxBuffSize
    (void)sprintf_s(msg, sizeof(msg), "No method MUID:%s found in the class", muidSlice[index].muid.ToStr().c_str());
    MRT_ThrowNoSuchMethodErrorUnw(msg);
  }
  return res;
}

MClass *LazyBinding::GetUnresolvedClass(MClass *klass, bool &fromUpper, bool isDef) {
  // The klass must be DEF, of course.
  LinkerMFileInfo *mplInfo = pInvoker->GetLinkerMFileInfo(kFromMeta, static_cast<void*>(klass), true);
  if (mplInfo == nullptr) {
    LINKER_LOG(FATAL) << "LinkerMFileInfo not found in lazy list, klass=" << klass << "name = " <<
        klass->GetName()<< maple::endl;
    return nullptr;
  }
  size_t dataDefSize = mplInfo->GetTblSize(kDataDef);
  AddrSlice dataDefSlice(mplInfo->GetTblBegin<LinkerAddrTableItem*>(kDataDef), dataDefSize);
  size_t muidSize = mplInfo->GetTblSize(kDataDefMuid);
  MuidSlice muidSlice = MuidSlice(mplInfo->GetTblBegin<LinkerMuidTableItem*>(kDataDefMuid), muidSize);

  // To get the index in the pTable
  MClass *indexClass = klass->GetClass();
  uint32_t defIndex = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(indexClass));
  if (defIndex == 0) { // It's not lazy, we use 0 as flag of not lazy.
    LINKER_LOG(FATAL) << "class is lazy, but no def index?klass=" << klass << "name=" << klass->GetName()<< maple::endl;
    return nullptr;
  } else {
    if (indexClass == WellKnown::GetMClassClass()) {
      return nullptr;
    }
    --defIndex;
  }

  if (GetAddrBindingState(dataDefSlice, defIndex, true) == kBindingStateResolved) {
    return nullptr;
  }
  return reinterpret_cast<MClass*>(ResolveClassSymbol(
      *mplInfo, dataDefSlice, muidSlice, defIndex, nullptr, fromUpper, isDef, true));
}

void LazyBinding::ResolveMethodsClassSymbol(const LinkerMFileInfo &mplInfo, bool isDef, const void *addr) {
  // To get the declaring class of the method.
  void *md = JavaFrame::GetMethodMetadata(static_cast<const uint32_t*>(addr));
  if (md != nullptr) {
    const MethodMetaBase *methodMeta = reinterpret_cast<const MethodMetaBase*>(md);
    void *classAddr = methodMeta->GetDeclaringClass();
    MClass *klass = static_cast<MClass*>(classAddr);
    if (klass == nullptr) {
      LINKER_LOG(FATAL) << "klass is nullptr" << maple::endl;
    }
    LINKER_VLOG(lazybinding) << "(" << (isDef ? "DEF" : "UNDEF") << "), to resolve klass=" << klass << ", name=" <<
        klass->GetName() << ", prim=" << klass->IsPrimitiveClass() << ", array=" << klass->IsArray() << ", cold=" <<
        modifier::IsColdClass(klass->GetFlag()) << ", lazy=" << klass->IsLazyBinding() <<
        ", in " << mplInfo.name << maple::endl;
    if (!klass->IsPrimitiveClass() && !klass->IsArrayClass() && klass->IsLazyBinding()) {
      bool fromUpper = false;
      MClass *unresolvedClass = GetUnresolvedClass(klass, fromUpper, isDef);
      if (unresolvedClass == nullptr) {
        LINKER_LOG(ERROR) << "(" << (isDef ? "DEF" : "UNDEF") << "), class is lazy, but no def index? klass=" <<
            klass << ", name=" << klass->GetName() << maple::endl;
        // Should not reach here?
        return;
      } else {
        if (!fromUpper) {
          LINKER_VLOG(lazybinding) << "(" << (isDef ? "DEF" : "UNDEF") << "), found class, unresolved class=" <<
              unresolvedClass << ", class name=" << unresolvedClass->GetName() << maple::endl;
          // Not only lazy .so.
          LinkerMFileInfo *tmpLinkerMFileInfo = pInvoker->GetLinkerMFileInfo(kFromMeta, unresolvedClass);
          if (tmpLinkerMFileInfo != nullptr) {
            LinkClassInternal(*tmpLinkerMFileInfo, unresolvedClass, nullptr, true);
          } else {
            LINKER_LOG(FATAL) << "class is lazy, but no lazy LinkerMFileInfo?" << " klass=" << unresolvedClass <<
                ", name=" << unresolvedClass->GetName() << maple::endl;
          }
          return;
        } else {
          // We don't set the method address any more.
          LINKER_VLOG(lazybinding) << "(" << (isDef ? "DEF" : "UNDEF") << "), not set the method address for klass=" <<
              klass << ", name=" << klass->GetName() << maple::endl;
        }
      }
    } else {
      // Already resolved the class before, just set address for the method.
      return;
    }
  } else {
    LINKER_LOG(FATAL) << "(" << (isDef ? "DEF" : "UNDEF") << "), md is null, addr=" << addr << maple::endl;
  }
  return;
}

void LazyBinding::LinkStaticSymbol(LinkerMFileInfo &mplInfo, const MClass *target) {
  LazyBinding::LinkStaticMethod(mplInfo, target);
  LazyBinding::LinkStaticField(mplInfo, target);
}

void LazyBinding::LinkStaticMethodSlow(MethodMeta &meta, std::string name,
    LinkerMFileInfo &mplInfo, LinkerMFileInfo &oldMplInfo, AddrSlice &srcTable, AddrSlice &dstTable) {
  MethodMeta *method = &meta;
  LinkerVoidType dstAddr = 0;
  size_t iSrcIndex = 0;
  size_t iDstIndex = 0;
  std::string methodName = name;
  methodName += "|";
  methodName += method->GetName();
  methodName += "|";
  methodName += method->GetSignature();
  std::string encodedMethodName = namemangler::EncodeName(methodName);
  MUID methodMuid = GetMUID(encodedMethodName, true);
  LinkerVoidType srcAddr = pInvoker->LookUpMethodSymbolAddress(oldMplInfo, methodMuid, iSrcIndex);
  if (UNLIKELY(&oldMplInfo != &mplInfo)) {
    dstAddr = pInvoker->LookUpMethodSymbolAddress(mplInfo, methodMuid, iDstIndex);
  } else {
    dstAddr = srcAddr;
    iDstIndex = iSrcIndex;
  }
  if (srcAddr != 0) {
    void *addr = reinterpret_cast<void*>(srcAddr);
    LINKER_VLOG(lazybinding) << "resolved MUID=" << methodMuid.ToStr() << ", Address=" << dstAddr <<
        ", copy form srcAddr=" << srcTable[iSrcIndex].Address() << ", srcIndex=" << iSrcIndex << " to dstAddr=" <<
        dstTable[iDstIndex].Address() << ", dstIndex=" << iDstIndex << ", methodName=" << methodName <<
        ", form mplInfo=" << oldMplInfo.name << " to mplInfo=" << mplInfo.name << maple::endl;
    if (SetAddrInAddrTable(dstTable, iDstIndex, reinterpret_cast<MClass*>(addr)) == nullptr) {
      LINKER_LOG(ERROR) << "set addr failed, addr = " << addr << maple::endl;
    }
  } else {
    LINKER_LOG(ERROR) << "failed to resolve MUID=" << methodMuid.ToStr() << ", Address=" << dstAddr <<
        ", encodedMethodName=" << encodedMethodName << ", srcAddr=" << srcTable[iSrcIndex].Address() <<
        ", srcIndex" << iSrcIndex << ", methodName=" << methodName.c_str() << ", from mplInfo=" <<
        oldMplInfo.name << " to mplInfo=" << mplInfo.name << maple::endl;
  }
}

void LazyBinding::LinkStaticMethodFast(MethodMeta &meta, const std::string name, size_t index,
    LinkerMFileInfo &mplInfo, const LinkerMFileInfo &oldMplInfo, AddrSlice &srcTable, AddrSlice &dstTable) {
  MethodMeta *method = &meta;
  LinkerVoidType dstAddr = 0;
  size_t iSrcIndex = index;
  size_t iDstIndex = 0;
  std::string methodName = name;
#ifdef LINKER_32BIT_REF_FOR_DEF_UNDEF
  LinkerVoidType methodSrcAddr = pInvoker->AddrToUint32(GetDefTableAddress(mplInfo, srcTable, iSrcIndex, true));
#else
  LinkerVoidType methodSrcAddr =
      reinterpret_cast<LinkerVoidType>(GetDefTableAddress(mplInfo, srcTable, iSrcIndex, true));
#endif // USE_32BIT_REF
  if (methodSrcAddr != 0) {
    method->SetAddress(methodSrcAddr);
    dstAddr = methodSrcAddr;
    if (UNLIKELY(&oldMplInfo != &mplInfo)) {
      methodName += "|";
      methodName += method->GetName();
      methodName += "|";
      methodName += method->GetSignature();
      std::string encodedMethodName = namemangler::EncodeName(methodName);
      MUID methodMuid = GetMUID(encodedMethodName, true);
      dstAddr = pInvoker->LookUpMethodSymbolAddress(mplInfo, methodMuid, iDstIndex);
      if (SetAddrInAddrTable(dstTable, iDstIndex, reinterpret_cast<MClass*>(methodSrcAddr)) == nullptr) {
        LINKER_LOG(ERROR) << "set addr failed, addr = " << methodSrcAddr << maple::endl;
      }
    } else {
      iDstIndex = iSrcIndex;
      if (SetAddrInAddrTable(dstTable, iDstIndex, reinterpret_cast<MClass*>(methodSrcAddr)) == nullptr) {
        LINKER_LOG(ERROR) << "set addr failed, addr = " << methodSrcAddr << maple::endl;
      }
    }
    LINKER_VLOG(lazybinding) << "resolved " << ", Address=" << dstAddr << ", copy form methodSrcAddr=" <<
        srcTable[iSrcIndex].Address() << ", srcIndex=" << iSrcIndex << " to dstAddr=" <<
        dstTable[iDstIndex].Address() << ", dstIndex=" << iDstIndex << ", methodName=" <<
        methodName << ", form mplInfo=" << oldMplInfo.name << " to mplInfo=" << mplInfo.name << maple::endl;
  } else {
    LINKER_LOG(ERROR) << "failed to resolve " << ", Address=" << methodSrcAddr << ", methodSrcAddr=" <<
        srcTable[iSrcIndex].Address() << ", srcIndex" << iSrcIndex << ", methodName=" << methodName <<
        ", from mplInfo=" << oldMplInfo.name << " to mplInfo=" << mplInfo.name << maple::endl;
  }
}

// In maple, actually all methods are linked to static symbols.
void LazyBinding::LinkStaticMethod(LinkerMFileInfo &mplInfo, const MClass *target) {
  if (UNLIKELY(target == nullptr)) {
    LINKER_DLOG(lazybinding) << "failed, target is null!" << maple::endl;
    return;
  }
  LinkerMFileInfo *oldMplInfo = pInvoker->GetLinkerMFileInfo(kFromMeta, target);
  size_t iTableSize = 0;
  AddrSlice srcTable;
  AddrSlice dstTable;
  if (oldMplInfo == nullptr) {
    oldMplInfo = &mplInfo;
  }
  if (UNLIKELY(oldMplInfo != &mplInfo)) {
    iTableSize = oldMplInfo->GetTblSize(kMethodDefOrig);
    size_t dstTableSize = mplInfo.GetTblSize(kMethodDef);
    srcTable = AddrSlice(oldMplInfo->GetTblBegin<LinkerAddrTableItem*>(kMethodDefOrig), iTableSize);
    dstTable = AddrSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kMethodDef), dstTableSize);
  } else {
    iTableSize = mplInfo.GetTblSize(kMethodDefOrig);
    srcTable = AddrSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kMethodDefOrig), iTableSize);
    dstTable = AddrSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kMethodDef), iTableSize);
  }
  if (dstTable.Empty() || srcTable.Empty()) {
    LINKER_DLOG(lazybinding) << "failed, table is null" << maple::endl;
    return;
  }
  MethodMeta *methods = target->GetMethodMetas();
  uint32_t numOfMethods = target->GetNumOfMethods();
  for (uint32_t i = 0; i < numOfMethods; ++i) {
    MethodMeta *method = methods + i;
    // Consider as the case of default methods, we don't check if (__MRT_Class_isInterface(target)).
    if (method == nullptr || UNLIKELY(method->IsAbstract())) {
      continue;
    }
    std::string methodName = target->GetName();
    int32_t index = method->GetDefTabIndex();
    if (index != -1) {
      size_t offset = static_cast<size_t>(index);
      if (offset >= iTableSize) {
        continue;
      }
      LinkStaticMethodFast(*method, methodName, offset, mplInfo, *oldMplInfo, srcTable, dstTable);
    } else {
      LinkStaticMethodSlow(*method, methodName, mplInfo, *oldMplInfo, srcTable, dstTable);
    }
  }
}

void LazyBinding::LinkStaticFieldSlow(FieldMeta &meta, std::string name,
    LinkerMFileInfo &mplInfo, LinkerMFileInfo &oldMplInfo, AddrSlice &srcTable, AddrSlice &dstTable) {
  FieldMeta *field = &meta;
  LinkerVoidType dstAddr = 0;
  size_t iSrcIndex = 0;
  size_t iDstIndex = 0;
  std::string fieldName = name;
  fieldName += "|";
  fieldName += field->GetName();
  std::string encodedFieldName = namemangler::EncodeName(fieldName);
  MUID fieldMuid = GetMUID(encodedFieldName, true);
  LinkerVoidType srcAddr = pInvoker->LookUpDataSymbolAddress(oldMplInfo, fieldMuid, iSrcIndex);
  if (UNLIKELY(&oldMplInfo != &mplInfo)) {
    dstAddr = pInvoker->LookUpDataSymbolAddress(mplInfo, fieldMuid, iDstIndex);
  } else {
    dstAddr = srcAddr;
    iDstIndex = iSrcIndex;
  }
  if (srcAddr != 0) {
    void *addr = reinterpret_cast<void*>(srcAddr);
    LINKER_VLOG(lazybinding) << "resolved MUID=" << fieldMuid.ToStr() << ", Address=" << dstAddr <<
        ", copy form srcAddr=" << srcTable[iSrcIndex].Address() << ", srcIndex=" << iSrcIndex << " to dstAddr=" <<
        dstTable[iDstIndex].Address() << ", dstIndex=" << iDstIndex << ", fieldName=" << fieldName <<
        ", form mplInfo=" << oldMplInfo.name << " to mplInfo=" << mplInfo.name << maple::endl;
    if (SetAddrInAddrTable(dstTable, iDstIndex, reinterpret_cast<MClass*>(addr)) == nullptr) {
      LINKER_LOG(ERROR) << "set addr failed, addr = " << addr << maple::endl;
    }
  } else {
    LINKER_LOG(ERROR) << "failed to resolve MUID=" << fieldMuid.ToStr() << ", Address=" << dstAddr <<
        ", encodedFieldName=" << encodedFieldName << ", srcAddr=" << srcTable[iSrcIndex].Address() << ", srcIndex" <<
        iSrcIndex << ", fieldName=" << fieldName.c_str() << ", from mplInfo=" << oldMplInfo.name <<
        " to mplInfo=" << mplInfo.name << maple::endl;
  }
}

void LazyBinding::LinkStaticFieldFast(FieldMeta &meta, const std::string name, size_t index,
    LinkerMFileInfo &mplInfo, const LinkerMFileInfo &oldMplInfo, AddrSlice &srcTable, AddrSlice &dstTable) {
  FieldMeta *field = &meta;
  LinkerVoidType dstAddr = 0;
  size_t iSrcIndex = index;
  size_t iDstIndex = 0;
  std::string fieldName = name;
#ifdef LINKER_32BIT_REF_FOR_DEF_UNDEF
  LinkerVoidType fieldSrcAddr = pInvoker->AddrToUint32(GetDefTableAddress(mplInfo, srcTable, iSrcIndex, false));
#else
  LinkerVoidType fieldSrcAddr =
      reinterpret_cast<LinkerVoidType>(GetDefTableAddress(mplInfo, srcTable, iSrcIndex, false));
#endif // USE_32BIT_REF
  if (fieldSrcAddr != 0) {
    field->SetStaticAddr(fieldSrcAddr);
    dstAddr = fieldSrcAddr;
    if (UNLIKELY(&oldMplInfo != &mplInfo)) {
      fieldName += "|";
      fieldName += field->GetName();
      std::string encodedFieldName = namemangler::EncodeName(fieldName);
      MUID fieldMuid = GetMUID(encodedFieldName, true);
      dstAddr = pInvoker->LookUpDataSymbolAddress(mplInfo, fieldMuid, iDstIndex);
      if (SetAddrInAddrTable(dstTable, iDstIndex, reinterpret_cast<MClass*>(fieldSrcAddr)) == nullptr) {
        LINKER_LOG(ERROR) << "set addr failed, addr = " << fieldSrcAddr << maple::endl;
      }
    } else {
      iDstIndex = iSrcIndex;
      if (SetAddrInAddrTable(dstTable, iDstIndex, reinterpret_cast<MClass*>(fieldSrcAddr)) == nullptr) {
        LINKER_LOG(ERROR) << "set addr failed, addr = " << fieldSrcAddr << maple::endl;
      }
    }
    LINKER_VLOG(lazybinding) << "resolved " << ", Address=" << dstAddr << ", copy form fieldSrcAddr=" <<
        srcTable[iSrcIndex].Address() << ", srcIndex=" << iSrcIndex << " to dstAddr=" <<
        dstTable[iDstIndex].Address() << ", dstIndex=" << iDstIndex << ", fieldName=" <<
        fieldName << ", form mplInfo=" << oldMplInfo.name << " to mplInfo=" << mplInfo.name << maple::endl;
  } else {
    LINKER_LOG(ERROR) << "failed to resolve " << ", Address=" << fieldSrcAddr << ", fieldSrcAddr=" <<
        srcTable[iSrcIndex].Address() << ", srcIndex" << iSrcIndex << ", fieldName=" << fieldName <<
        ", from mplInfo=" << oldMplInfo.name << " to mplInfo=" << mplInfo.name << maple::endl;
  }
}

void LazyBinding::LinkStaticField(LinkerMFileInfo &mplInfo, const MClass *target) {
  if (UNLIKELY(target == nullptr)) {
    LINKER_DLOG(lazybinding) << "failed, target is null!" << maple::endl;
    return;
  }
  size_t iTableSize = 0;
  AddrSlice srcTable;
  AddrSlice dstTable;
  LinkerMFileInfo *oldMplInfo = pInvoker->GetLinkerMFileInfo(kFromMeta, target);
  if (oldMplInfo == nullptr) {
    oldMplInfo = &mplInfo;
  }
  if (UNLIKELY(oldMplInfo != &mplInfo)) {
    iTableSize = oldMplInfo->GetTblSize(kDataDefOrig);
    size_t dstTableSize = mplInfo.GetTblSize(kDataDef);
    srcTable = AddrSlice(oldMplInfo->GetTblBegin<LinkerAddrTableItem*>(kDataDefOrig), iTableSize);
    dstTable = AddrSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kDataDef), dstTableSize);
  } else {
    iTableSize = mplInfo.GetTblSize(kDataDefOrig);
    srcTable = AddrSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kDataDefOrig), iTableSize);
    dstTable = AddrSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kDataDef), iTableSize);
  }
  if (dstTable.Empty() || srcTable.Empty()) {
    LINKER_DLOG(lazybinding) << "failed, table is null" << maple::endl;
    return;
  }
  FieldMeta *fields = target->GetFieldMetas();
  uint32_t numOfFields = target->GetNumOfFields();
  for (uint32_t i = 0; i < numOfFields; ++i) {
    FieldMeta *field = fields + i;
    // Not check !field->IsPublic() any more.
    if (field == nullptr || !field->IsStatic()) {
      continue;
    }
    std::string fieldName = target->GetName();
    int32_t index = field->GetDefTabIndex();
    if (index != -1) {
      size_t offset = static_cast<size_t>(index);
      if (offset >= iTableSize) {
        continue;
      }
      LinkStaticFieldFast(*field, fieldName, offset, mplInfo, *oldMplInfo, srcTable, dstTable);
    } else {
      LinkStaticFieldSlow(*field, fieldName, mplInfo, *oldMplInfo, srcTable, dstTable);
    }
  }
}

void LazyBinding::LinkSuperClassAndInterfaces(
    const MClass *klass, MObject *candidateClassLoader, bool recursive, bool forCold) {
  LinkerMFileInfo *mplInfo = pInvoker->GetLinkerMFileInfo(kFromMeta, klass, !forCold);
  if (mplInfo != nullptr && (forCold || mplInfo->IsFlag(kIsLazy))) {
    LinkSuperClassAndInterfaces(*mplInfo, klass, candidateClassLoader, recursive, forCold);
  }
}

MClass *LazyBinding::LinkDataUndefSuperClassAndInterfaces(
    LinkerMFileInfo &mplInfo, MObject *candidateClassLoader, size_t index, AddrSlice dataUndefSlice,
    MClass *superClassesItem, IndexSlice superTableSlice, uint32_t i, bool fromUndef) {
  size_t muidSize = mplInfo.GetTblSize(kDataUndefMuid);
  MuidSlice muidSlice = MuidSlice(mplInfo.GetTblBegin<LinkerMuidTableItem*>(kDataUndefMuid), muidSize);
  if (GetAddrBindingState(dataUndefSlice, index, true) != kBindingStateResolved) {
    bool fromUpper = false;
    superClassesItem = reinterpret_cast<MClass*>(ResolveClassSymbol(
        mplInfo, dataUndefSlice, muidSlice, index, candidateClassLoader, fromUpper, false, true));
    if (superClassesItem != nullptr) {
      superTableSlice[i].index = reinterpret_cast<size_t>(superClassesItem);
      LINKER_VLOG(lazybinding) << "fromUpper:"<< fromUpper << "resolved UNDEF lazily, " << fromUndef <<
          ", addr=" << dataUndefSlice[index].Address() << ", class=" <<  superClassesItem->GetName() <<
          " in " << mplInfo.name << maple::endl;
    } else {
      __MRT_ASSERT(0, "LazyBinding::LinkSuperClassAndInterfaces(), not resolved UNDEF lazily\n");
    }
  } else {
    LINKER_VLOG(lazybinding) << "already resolved UNDEF, just use, " << fromUndef << ", addr=" <<
        dataUndefSlice[index].Address() << ", class=" <<
        reinterpret_cast<MClass*>(dataUndefSlice[index].Address())->GetName() << " in " << mplInfo.name <<
        maple::endl;
    superTableSlice[i].index = reinterpret_cast<size_t>(dataUndefSlice[index].Address());

    // To link lazy class even if it's resolved before.
    MClass *resolvedClass = reinterpret_cast<MClass*>(dataUndefSlice[index].Address());
    if (resolvedClass->IsLazyBinding()) {
      superClassesItem = resolvedClass;
    }
  }
  return superClassesItem;
}

MClass *LazyBinding::LinkDataDefSuperClassAndInterfaces(
    LinkerMFileInfo &mplInfo, size_t index, AddrSlice dataDefSlice,
    MClass *superClassesItem, IndexSlice superTableSlice, uint32_t i, bool fromUndef) {
  size_t muidSize = mplInfo.GetTblSize(kDataDefMuid);
  MuidSlice muidSlice = MuidSlice(mplInfo.GetTblBegin<LinkerMuidTableItem*>(kDataDefMuid), muidSize);
  // We must don't check resolved state for DEF table.
  // Not check if (GetAddrBindingState(dataDefSlice, index, true) == kBindingStateResolved)
  if (GetAddrBindingState(dataDefSlice, index, true) == kBindingStateResolved) {
    LINKER_VLOG(lazybinding) << "already resolved DEF before, " << fromUndef << ", addr=" <<
        dataDefSlice[index].Address() << ", class=" <<
        reinterpret_cast<MClass*>(dataDefSlice[index].Address())->GetName() << ", in " << mplInfo.name <<
        maple::endl;
  }
  bool fromUpper = false;
  superClassesItem = reinterpret_cast<MClass*>(ResolveClassSymbol(
      mplInfo, dataDefSlice, muidSlice, index, nullptr, fromUpper, true, true));
  if (superClassesItem != nullptr) {
    superTableSlice[i].index = reinterpret_cast<size_t>(superClassesItem);
    LINKER_VLOG(lazybinding) << "fromUpper:" << fromUpper <<"resolved DEF lazily, " << fromUndef << ", addr=" <<
        dataDefSlice[index].Address() << ", class=" << superClassesItem->GetName() << " in " << mplInfo.name <<
        maple::endl;
  } else {
    __MRT_ASSERT(0, "LazyBinding::LinkSuperClassAndInterfaces(), not resolved DEF lazily\n");
  }
  return superClassesItem;
}

void LazyBinding::LinkSuperClassAndInterfaces(
    LinkerMFileInfo &mplInfo, const MClass *klass, MObject *candidateClassLoader, bool recursive, bool forCold) {
  uint32_t superClassSize = klass->GetNumOfSuperClasses();
  IndexSlice superTableSlice = IndexSlice(
      reinterpret_cast<LinkerSuperClassTableItem*>(klass->GetSuperClassArrayPtr()), superClassSize);
  if (superTableSlice.Empty()) {
    return;
  }

  size_t dataUndefSize = mplInfo.GetTblSize(kDataUndef);
  AddrSlice dataUndefSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kDataUndef), dataUndefSize);
  size_t dataDefSize = mplInfo.GetTblSize(kDataDef);
  AddrSlice dataDefSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kDataDef), dataDefSize);
  for (uint32_t i = 0; i < superClassSize; ++i) {
    LinkerRef ref(superTableSlice[i].index);
    MClass *superClassesItem = nullptr;
    LINKER_VLOG(lazybinding) << "super[" << i << "]=" << superTableSlice[i].index << ", defSize=" << dataDefSize <<
        ", undefSize=" << dataUndefSize << ", for " << klass->GetName() << ", in " << mplInfo.name << maple::endl;
    if (ref.IsIndex()) { // Def or Undef Index
      bool fromUndef = ref.IsFromUndef();
      size_t index = ref.GetIndex();
      if (fromUndef && index < dataUndefSize && !dataUndefSlice.Empty()) {
        superClassesItem = LinkDataUndefSuperClassAndInterfaces(mplInfo, candidateClassLoader, index, dataUndefSlice,
            superClassesItem, superTableSlice, i, fromUndef);
      } else if (!fromUndef && index < dataDefSize && !dataDefSlice.Empty()) {
        superClassesItem = LinkDataDefSuperClassAndInterfaces(mplInfo, index, dataDefSlice, superClassesItem,
            superTableSlice, i, fromUndef);
      } else {
        __MRT_ASSERT(0, "LazyBinding::LinkSuperClassAndInterfaces(), must be DEF or UNDEF\n");
      }
    } else { // Address.
      // Already resolved.
      if (!ref.IsEmpty() && forCold) {
        superClassesItem = ref.GetDataRef<MClass*>();
        LINKER_VLOG(lazybinding) << "already resolved, super[" << i << "]=" << superClassesItem << ", defSize=" <<
            dataDefSize << ", undefSize=" << dataUndefSize << ", for " << superClassesItem->GetName() <<
            ", in " << mplInfo.name << maple::endl;
      }
      // Allow re-link.
    }

    if (recursive) {
      if (superClassesItem == nullptr) {
        LINKER_VLOG(lazybinding) << "ignore one item of super in " << klass->GetName() << maple::endl;
        // We allow super class or interfaces as null.
        continue;
      }
      if (forCold) {
        LinkSuperClassAndInterfaces(superClassesItem, candidateClassLoader, true, true);
      } else {
        LinkClassInternal(superClassesItem, candidateClassLoader);
      }
    }
  }
}

void LazyBinding::LinkClass(MClass *klass) {
  LINKER_DLOG(lazybinding) << "klass=" << klass << ", name=" << klass->GetName() << ", lazy=" <<
      klass->IsLazyBinding() << ", cold=" << klass->IsColdClass() << maple::endl;
  // If the array's component is not ready, to link the component class for array class.
  if (klass->IsArrayClass()) {
    MClass *componentClass = klass->GetComponentClass();
    if (componentClass == nullptr) {
      LINKER_LOG(FATAL) << "component class is null, klass=" << klass << ", name=" << klass->GetName() << maple::endl;
      return;
    }
    if (componentClass->IsLazyBinding()) {
      LinkClass(componentClass);
    }
    return;
  }

  // We use the field of shadow to store def table index.
  if (klass->IsPrimitiveClass() || !klass->IsLazyBinding()) {
    return;
  }
  LinkerMFileInfo *mplInfo = pInvoker->GetLinkerMFileInfo(kFromMeta, klass, true);
  if (mplInfo != nullptr) {
    // To get the index in the pTable
    MClass *indexClass = klass->GetClass();
    uint32_t defIndex = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(indexClass));
    if (defIndex == 0) { // It's not lazy, we use 0 as flag of not lazy.
      LINKER_LOG(FATAL) << "class has no def index? klass=" << klass << ", name=" << klass->GetName() << maple::endl;
      return;
    } else {
      if (indexClass == WellKnown::GetMClassClass()) {
        return;
      }
      --defIndex;
    }
    size_t dataDefSize = mplInfo->GetTblSize(kDataDef);
    AddrSlice dataTable(mplInfo->GetTblBegin<LinkerAddrTableItem*>(kDataDef), dataDefSize);
    if (GetAddrBindingState(dataTable, defIndex, true) == kBindingStateResolved) {
      return;
    }
    if (!MRT_IsClassInitialized(*klass)) {
      LoaderAPI::Instance().SetClassCL(*klass, mplInfo->classLoader);
      LinkClassInternal(*mplInfo, klass, reinterpret_cast<MObject*>(mplInfo->classLoader), true);
    } else {
      LinkClassInternal(*mplInfo, klass, reinterpret_cast<MObject*>(MRT_GetClassLoader(*klass)), true);
    }
  } else {
    LINKER_LOG(FATAL) << "lazy class has no MFileInfo? klass=" << klass << ", name=" << klass->GetName() << maple::endl;
  }
}

// Only used in LinkSuperClassAndInterfaces().
void LazyBinding::LinkClassInternal(MClass *klass, MObject *candidateClassLoader) {
  LinkerMFileInfo *mplInfo = pInvoker->GetLinkerMFileInfo(kFromMeta, klass, true);
  if (mplInfo != nullptr) {
    LinkClassInternal(*mplInfo, klass, candidateClassLoader, true);
  }
}

void LazyBinding::LinkClassInternal(
    LinkerMFileInfo &mplInfo, MClass *klass, MObject *candidateClassLoader, bool first) {
  LINKER_VLOG(lazybinding) << "klass=" << klass << ", name=" << klass->GetName() << ", lazy=" <<
      klass->IsLazyBinding() << ", cold=" << klass->IsColdClass() << ", decouple=" <<
      klass->IsDecouple() << ", in " << mplInfo.name << maple::endl;
  if (!klass->IsLazyBinding() || *klass == MRT_GetClassObject()) {
    return;
  }
  if (maple::ObjectBase::MonitorEnter(reinterpret_cast<address_t>(klass)) == JNI_ERR) {
    LINKER_LOG(ERROR) << ", maple::ObjectBase::MonitorEnter() failed" << maple::endl;
  }

  // Check if we link it before, or no need linking.
  // (Notice:Lazy binding class must also be cold class,
  // but lazy-binding is optional, lazy binding class maybe not cold.)
  // To prevent deadlock, we will checking klass status after got MonitorEnter lock.
  if (!klass->IsLazyBinding() || *klass == MRT_GetClassObject()) {
    if (maple::ObjectBase::MonitorExit(reinterpret_cast<address_t>(klass)) == JNI_ERR) {
      LINKER_LOG(ERROR) << "maple::ObjectBase::MonitorExit() failed" << maple::endl;
    }
    return;
  }

  LinkSuperClassAndInterfaces(mplInfo, klass, candidateClassLoader, true);

  // Set the def index all the time.
  MClass *indexClass = klass->GetClass();
  MetaRef index = static_cast<MetaRef>(reinterpret_cast<uintptr_t>(indexClass));
  if (index == 0) { // It's not lazy, we use 0 as flag of not lazy.
    LINKER_LOG(FATAL) << "class is lazy, but no def index? klass=" << klass << ", name=" <<
        klass->GetName() << maple::endl;
  } else {
    if (indexClass == WellKnown::GetMClassClass()) {
      if (maple::ObjectBase::MonitorExit(reinterpret_cast<address_t>(klass)) == JNI_ERR) {
        LINKER_LOG(ERROR) << "maple::ObjectBase::MonitorExit() failed" << maple::endl;
      }
      return;
    }
    --index;
  }

  LinkStaticSymbol(mplInfo, klass);
  // To handle decoupling routine.
  LinkMethods(mplInfo, klass, first, index);
  LinkFields(mplInfo, klass);

  // The same as MRTSetMetadataShadow(reinterpret_cast<ClassMetadata*>(klass), WellKnown::GetMClassClass())
  // If first loaded, we clear the def index in 'shadow' field which multiplex used for lazy binding.
  // Set shadow to CLASS. We multiplex use the 'shadow' before class's being intialized,
  MRTSetMetadataShadow(reinterpret_cast<ClassMetadata*>(klass), WellKnown::GetMClassClass());
  if (SetClassInDefAddrTable(index, klass) == nullptr) {
    LINKER_LOG(ERROR) << "SetClassInDefAddrTable failed" << maple::endl;
  }

  // Set cold class as hot.
  klass->SetHotClass();
  // Clear lazy binding flag.
  klass->ReSetFlag(0xDFFF); // 0xDFFF is LazyBinding flag
  klass->SetFlag(modifier::kClassLazyBoundClass);

  if (maple::ObjectBase::MonitorExit(reinterpret_cast<address_t>(klass)) == JNI_ERR) {
    LINKER_LOG(ERROR) << "maple::ObjectBase::MonitorExit() failed" << maple::endl;
  }
}


inline std::string LazyBinding::ResolveClassMethod(MClass *klass) {
  return pMethodBuilder->BuildMethod(klass);
}

#if defined(LINKER_RT_CACHE) && defined(LINKER_RT_LAZY_CACHE)
void LazyBinding::PreLinkLazyMethod(LinkerMFileInfo &mplInfo) {
  if (mplInfo.IsFlag(kIsLazy)) {
    std::string cachingStr;
    if (!LoadLazyCache(&mplInfo, cachingStr)) {
      return;
    }
    mplInfo.methodCachingStr = cachingStr;

    std::vector<std::string> cachingIndexes;
    std::istringstream issCaching(cachingStr);
    std::string tmp;
    while (getline(issCaching, tmp, ';')) {
      cachingIndexes.push_back(tmp);
    }
    LINKER_VLOG(lazybinding) << "klass num=" << cachingIndexes.size() << ", in " << mplInfo.name << maple::endl;

    LinkerAddrTableItem *pDefRealTable = mplInfo.GetTblBegin<LinkerAddrTableItem*>(kDataDefOrig);
    for (size_t i = 0; i < cachingIndexes.size(); ++i) {
      std::string indexStr = cachingIndexes[i];
      std::string::size_type pos = indexStr.find(':');
      if (pos == std::string::npos) { // Error
        continue;
      }
      constexpr int decimalBase = 10;
      auto index = std::strtol(indexStr.c_str(), nullptr, decimalBase);
      MClass *klass = reinterpret_cast<MClass*>(GetDefTableAddress(mplInfo, *pDefRealTable, index, false));
      LinkStaticSymbol(mplInfo, klass);
      LinkSuperClassAndInterfaces(mplInfo, klass, nullptr, true, true);
      std::string cachingIndex = indexStr.substr(pos + 1);
      pMethodBuilder->BuildMethodByCachingIndex(klass, cachingIndex);

      // To handle decoupling routine.
      LinkFields(mplInfo, klass);

      // The same as MRTSetMetadataShadow(reinterpret_cast<ClassMetadata*>(klass), WellKnown::GetMClassClass())
      // If first loaded, we clear the def index in 'shadow' field which multiplex used for lazy binding.
      // Set shadow to CLASS.
      MRTSetMetadataShadow(reinterpret_cast<ClassMetadata*>(klass), WellKnown::GetMClassClass());
      (void)SetClassInDefAddrTable(index, klass);

      // Set Classloader for the class.
      LoaderAPI::Instance().SetClassCL(*klass, mplInfo.classLoader);
      // Set cold class as hot.
      klass->SetHotClass();
      // Clear lazy binding flag.
      klass->ReSetFlag(0xDFFF); // 0xDFFF is LazyBinding flag
      klass->SetFlag(modifier::kClassLazyBoundClass);

      ++(mplInfo.methodCachingNum);
    }
  }
}
#endif // defined(LINKER_RT_CACHE) && defined(LINKER_RT_LAZY_CACHE)

void LazyBinding::LinkMethods(LinkerMFileInfo &mplInfo, MClass *klass, bool first, MetaRef index) {
  LINKER_VLOG(lazybinding) << "(" << &mplInfo << ", " << klass->GetName() << "), first=" << first << ", index=" <<
      index << maple::endl;
#ifdef LINKER_DECOUPLE
  if (mplInfo.IsFlag(kIsLazy) && mplInfo.GetDecoupleLevel() != 0) {
    // Inteface has no vtable or itable.
    if (klass->IsInterface() || !first) {
      return;
    }
    if (klass->IsDecouple()) {
      if (reinterpret_cast<ClassMetadata*>(klass)->vTable.refVal != 0) {
        LINKER_LOG(ERROR) << "(" << klass->GetName() << "), already set, index=" << index << maple::endl;
        return;
      }
#if defined(LINKER_RT_CACHE) && defined(LINKER_RT_LAZY_CACHE)
      if (mplInfo.methodCachingNum < 2000) { // We don't need save all classes, here we use 2000 as upper threshold.
        std::lock_guard<std::mutex> autoLock(mplInfo->methodCachingLock);
        std::string cachingStr = ResolveClassMethod(klass);
        if (!cachingStr.empty()) {
          mplInfo.methodCachingStr += std::to_string(index) + ':';
          mplInfo.methodCachingStr += cachingStr;
          mplInfo.methodCachingStr += ';';
          ++(mplInfo.methodCachingNum);
        }

        if (mplInfo.methodCachingNum % 500 == 0) { // We use 500 as upper threshold, to reduce the times of saving.
          SaveLazyCache(mplInfo, mplInfo.methodCachingStr);
        }
      } else {
        (void)ResolveClassMethod(klass);
      }
#else // defined(LINKER_RT_CACHE) && defined(LINKER_RT_LAZY_CACHE)
      (void)ResolveClassMethod(klass);
#endif // defined(LINKER_RT_CACHE) && defined(LINKER_RT_LAZY_CACHE)
    }
  } else if (klass->IsColdClass()) {
#else
  if (klass->IsColdClass()) {
#endif
    LINKER_VLOG(lazybinding) << "for non-decouple class, name=" << klass->GetName() << ", index=" << index <<
        ", lazy=" << klass->IsLazyBinding() << ", cold=" << klass->IsColdClass() << ", decouple=" <<
        klass->IsDecouple() << ", in " << mplInfo.name << maple::endl;
    pInvoker->ResolveVTableSymbolByClass(mplInfo, klass, true);
    pInvoker->ResolveVTableSymbolByClass(mplInfo, klass, false);
  }
}

inline void LazyBinding::LinkFields(const LinkerMFileInfo &mplInfo, MClass *klass) {
  LINKER_VLOG(lazybinding) << "(" << &mplInfo << ", " << klass->GetName() << ")" << maple::endl;
#ifdef LINKER_DECOUPLE
  if (mplInfo.IsFlag(kIsLazy) && mplInfo.GetDecoupleLevel() != 0) {
    Decouple *decouple = pInvoker->Get<Decouple>();
    MplFieldDecouple *fieldDecouple = &decouple->GetFieldResolver();
    MplGctibAnalysis *gctibResolver = &decouple->GetGctibResolver();
    fieldDecouple->ResolveFieldOffsetAndObjSizeLazily(*(reinterpret_cast<ClassMetadata*>(klass)));
    (void)gctibResolver->ReGenGctib4Class(reinterpret_cast<ClassMetadata*>(klass));
  }
#endif
}

#if defined(LINKER_RT_CACHE) && defined(LINKER_RT_LAZY_CACHE)
inline bool LazyBinding::SaveLazyCacheInternal(std::string &path, int &fd, std::vector<char> &bufVector) {
  char *buf = nullptr;
  MUID cacheValidity;
  uint32_t eof = static_cast<uint32_t>(EOF);

  // Ready to write all the data in buffer.
  buf = reinterpret_cast<char*>(bufVector.data());
  std::streamsize byteCount = bufVector.size();
  // 1. Write for validity.
  GenerateMUID(buf, byteCount, cacheValidity);
  if (write(fd, reinterpret_cast<char*>(&cacheValidity.data), sizeof(cacheValidity.data)) < 0) {
    LINKER_LOG(ERROR) << "..validity, " << path << ", " << errno << maple::endl;
    return false;
  }
  // 2. Write for the content length.
  uint32_t contentSize = static_cast<uint32_t>(byteCount);
  if (write(fd, reinterpret_cast<char*>(&contentSize), sizeof(contentSize)) < 0) {
    LINKER_LOG(ERROR) << "..content length, " << path << ", " << errno << maple::endl;
    return false;
  }
  // 3. Write for the content.
  if (write(fd, buf, byteCount) < 0) {
    LINKER_LOG(ERROR) << "..content, " << path << ", " << errno << maple::endl;
    return false;
  }
  // 4. Write EOF.
  if (write(fd, reinterpret_cast<char*>(&eof), sizeof(eof)) < 0) {
    LINKER_LOG(ERROR) << "write EOF error, " << path << ", " << errno << maple::endl;
    return false;
  }
  return true;
}

bool LazyBinding::SaveLazyCache(LinkerMFileInfo &mplInfo, const std::string &cachingIndex) {
  bool res = true;
  std::vector<char> bufVector;
  char *str = nullptr;
  std::string path;
  LinkerCacheType cacheType = LinkerCacheType::kLinkerCacheLazy;

  if (!pInvoker->GetCachePath(mplInfo, path, cacheType)) {
    LINKER_LOG(ERROR) << "fail to prepare " << path << ", " << errno << maple::endl;
    return false;
  }
  int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644); // 0644 means file's permissions
  if (fd == -1) {
    LINKER_LOG(ERROR) << "fail to open " << path << ", " << errno << maple::endl;
    res = false;
    goto END;
  }

  // Prepare all the data in buffer firstly.
  // 1. Insert maximum version into content.
  int32_t maxVersion = GetMaxVersion();
  str = reinterpret_cast<char*>(&maxVersion);
  bufVector.insert(bufVector.end(), str, str + sizeof(maxVersion));
  // 2. Insert minimum version into content.
  int32_t minVersion = GetMinVersion();
  str = reinterpret_cast<char*>(&minVersion);
  bufVector.insert(bufVector.end(), str, str + sizeof(minVersion));
  // 3. Insert the hash from .so into content.
  MUID soValidity = pInvoker->GetValidityCode(mplInfo);
  str = reinterpret_cast<char*>(&soValidity);
  bufVector.insert(bufVector.end(), str, str + sizeof(soValidity));

  // 4. Insert the string length firstly into content.
  uint32_t len = cachingIndex.size();
  str = reinterpret_cast<char*>(&len);
  bufVector.insert(bufVector.end(), str, str + sizeof(len));

  // 5. Insert the caching index info. into content.
  bufVector.insert(bufVector.end(), cachingIndex.begin(), cachingIndex.end());

  if (!SaveLazyCacheInternal(path, fd, bufVector)) {
    res = false;
  }

END:
  close(fd);
  return res;
}

inline bool LazyBinding::LoadLazyCacheInternal(
    LinkerMFileInfo &mplInfo, std::string &path, std::string &cachingIndex,
    char *buf, std::streamsize &index, std::streamsize &byteCount) {
  MUID cacheValidity;

  // 1. Read the validity.
  MUID lastCacheValidity = *(reinterpret_cast<MUID*>(&buf[index]));
  index += sizeof(lastCacheValidity);
  // 2. Read the length of content.
  uint32_t contentSize = *(reinterpret_cast<uint32_t*>(&buf[index]));
  index += sizeof(contentSize);
  if (contentSize != static_cast<uint32_t>(byteCount - index)) {
    LINKER_LOG(ERROR) << "wrong cache length, " << path << ", " << contentSize << " vs. " <<
        static_cast<uint32_t>(byteCount - index) << maple::endl;
    return false;
  }

  // Generate the digest for validity, excluding the length of content.
  GenerateMUID(&buf[index], byteCount - index, cacheValidity);
  if (lastCacheValidity != cacheValidity) {
    LINKER_LOG(ERROR) << "cache validity checking failed, " << path << maple::endl;
    return false;
  }

  // 3. Read maximum version.
  int32_t maxVersion = *(reinterpret_cast<int32_t*>(&buf[index]));
  if (maxVersion != GetMaxVersion()) {
    LINKER_LOG(ERROR) << "wrong max version, " << path << ", " << maxVersion << maple::endl;
    return false;
  }
  index += sizeof(maxVersion);

  // 4. Read minimum version.
  int32_t minVersion = *(reinterpret_cast<int32_t*>(&buf[index]));
  if (minVersion != GetMinVersion()) {
    LINKER_LOG(ERROR) << "wrong min version, " << path << ", " << minVersion << maple::endl;
    return false;
  }
  index += sizeof(minVersion);

  // 5. Read the hash from cache file, comparing with .so.
  MUID lastSoValidity = *(reinterpret_cast<MUID*>(&buf[index]));
  MUID soValidity = pInvoker->GetValidityCode(mplInfo);
  if (lastSoValidity != soValidity) {
    LINKER_LOG(ERROR) << "wrong validity, " << soValidity.ToStr() << " vs. " << lastSoValidity.ToStr() << " in " <<
        mplInfo.name << maple::endl;
    return false;
  }
  index += sizeof(lastSoValidity);

  // 6. Read the content length firstly.
  uint32_t len = *(reinterpret_cast<uint32_t*>(&buf[index]));
  index += sizeof(len);

  // 7. Read the content.
  cachingIndex.resize(len + 1);
  cachingIndex.assign(&buf[index], len);
  index += len;
  return true;
}

bool LazyBinding::LoadLazyCache(LinkerMFileInfo &mplInfo, std::string &cachingIndex) {
  bool res = false;
  std::ifstream in;
  std::streamsize index = 0;
  std::vector<char> bufVector;
  char *buf = nullptr;
  std::string path;

  if (!pInvoker->GetCachePath(&mplInfo, path, LinkerCacheType::kLinkerCacheLazy)) {
    LINKER_LOG(ERROR) << "fail to prepare " << path << ", " << errno << maple::endl;
    return res;
  }
  in.open(path, std::ios::binary);
  bool failIn = (!in);
  if (failIn) {
    LINKER_LOG(ERROR) << "fail to open " << path << ", " << errno << maple::endl;
    goto END;
  }

  // Read all the data in buffer firstly.
  in.seekg(0, std::ios::end);
  std::streamsize byteCount = in.tellg();
  in.seekg(0, std::ios::beg);
  bufVector.resize(byteCount);
  buf = reinterpret_cast<char*>(bufVector.data());
  if (!in.read(buf, byteCount)) {
    LINKER_LOG(ERROR) << "read data error, " << path << ", " << errno << maple::endl;
    goto END;
  }
  // 0. Read EOF.
  uint32_t eof = *(reinterpret_cast<uint32_t*>(&buf[byteCount - sizeof(eof)]));
  if (eof != static_cast<uint32_t>(EOF)) {
    LINKER_LOG(ERROR) << "wrong EOF, " << path << ", eof=" << std::hex << eof << maple::endl;
    goto END;
  }
  byteCount -= sizeof(eof);
  if (!LoadLazyCacheInternal(&mplInfo, path, cachingIndex, buf, index, byteCount)) {
    goto END;
  }
  if (index == byteCount) {
    res = true;
  }

END:
  if (in.is_open()) {
    in.close();
  }
  return res;
}
#endif // defined(LINKER_RT_CACHE) && defined(LINKER_RT_LAZY_CACHE)

void *LazyBinding::GetClassMetadata(LinkerMFileInfo &mplInfo, size_t classIndex) {
  LinkerRef ref(classIndex);
  void *addr = nullptr;
  if (ref.IsIndex()) { // Index
    size_t index = ref.GetIndex();
    bool fromUndef = ref.IsFromUndef();
    if (fromUndef && index < mplInfo.GetTblSize(kDataUndef)) {
      LinkerMuidTableItem *pMuidTable = mplInfo.GetTblBegin<LinkerMuidTableItem*>(kDataUndefMuid);
      size_t dataUndefSize = mplInfo.GetTblSize(kDataUndef);
      AddrSlice dataUndefTable(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kDataUndef), dataUndefSize);
      if (mplInfo.IsFlag(kIsLazy) && GetAddrBindingState(dataUndefTable, index) != kBindingStateResolved) {
        if (HandleSymbolForDecoupling(mplInfo, dataUndefTable, index)) {
          addr = dataUndefTable[index].Address();
          LINKER_VLOG(lazybinding) << "(UNDEF), resolved lazily, index=" << index << ", name=" <<
              reinterpret_cast<MClass*>(addr)->GetName() << " for " << mplInfo.name << maple::endl;
        } else {
          addr = dataUndefTable[index].Address();
          LINKER_LOG(ERROR) << "(UNDEF), resolve class error, index=" << index << ", muid=" <<
              pMuidTable[index].muid.ToStr() << ", addr=" << addr << ", resolved=" <<
              (GetAddrBindingState(dataUndefTable, index) == kBindingStateResolved) <<
              " for " << mplInfo.name << maple::endl;
        }
      } else {
        addr = dataUndefTable[index].Address();
      }
    } else if (!fromUndef && index < mplInfo.GetTblSize(kDataDef)) {
      size_t dataDefSize = mplInfo.GetTblSize(kDataDef);
      AddrSlice dataDefTable(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kDataDef), dataDefSize);
      if (mplInfo.IsFlag(kIsLazy) && GetAddrBindingState(dataDefTable, index) != kBindingStateResolved) {
        if (HandleSymbolForDecoupling(mplInfo, dataDefTable, index)) {
          addr = dataDefTable[index].Address();
          LINKER_VLOG(lazybinding) << "(DEF), resolved lazily, index=" << index << ", name=" <<
              reinterpret_cast<MClass*>(addr)->GetName() << " for " << mplInfo.name << maple::endl;
        } else {
          addr = GetDefTableAddress(mplInfo, dataDefTable, static_cast<size_t>(index), false);
          LinkerMuidTableItem *dataDefMuidTable = mplInfo.GetTblBegin<LinkerMuidTableItem*>(kDataDefMuid);
          LINKER_LOG(ERROR) << "(DEF), resolve class error, index=" << index << ", muid=" <<
              dataDefMuidTable[index].muid.ToStr() << ", addr=" << addr << ", resolved=" <<
              (GetAddrBindingState(dataDefTable, index) == kBindingStateResolved) <<
              " for " << mplInfo.name << maple::endl;
        }
      } else {
        addr = dataDefTable[index].Address();
      }
    }
  }
  return addr;
}

void *LazyBinding::LookUpDataSymbolAddressLazily(
    const MObject *classLoader, const MUID &muid, LinkerMFileInfo **outLinkerMFileInfo, bool ignoreClassLoader) {
  void *res = nullptr;
  if (!ignoreClassLoader) {
    auto handle = [this, &muid, &res](LinkerMFileInfo &mplInfo)->bool {
      size_t tmp = 0;
      LinkerVoidType addr = this->pInvoker->LookUpDataSymbolAddress(mplInfo, muid, tmp);
      if (addr != 0) {
        res = reinterpret_cast<void*>(addr);
        return true;
      }
      return false;
    };
    (void)pInvoker->mplInfoListCLMap.FindIf(classLoader, handle);
  } else {
    auto handle = [this, &muid, outLinkerMFileInfo, &res](LinkerMFileInfo &mplInfo)->bool {
      size_t tmp = 0;
      LinkerVoidType addr = this->pInvoker->LookUpDataSymbolAddress(mplInfo, muid, tmp);
      if (addr != 0) {
        if (outLinkerMFileInfo != nullptr) {
          *outLinkerMFileInfo = &mplInfo;
        }
        res = reinterpret_cast<void*>(addr);
        return true;
      }
      return false;
    };
    (void)pInvoker->mplInfoList.FindIf(handle);
  }
  return res;
}

void *LazyBinding::LookUpMethodSymbolAddressLazily(
    const MObject *classLoader, const MUID &muid, bool ignoreClassLoader) {
  void *res = nullptr;
  auto handle = [this, &muid, &res](LinkerMFileInfo &mplInfo)->bool {
    size_t tmp = 0;
    LinkerVoidType addr = this->pInvoker->LookUpMethodSymbolAddress(mplInfo, muid, tmp);
    if (addr != 0) {
      res = reinterpret_cast<void*>(addr);
      return true;
    }
    return false;
  };
  if (!ignoreClassLoader) {
    (void)pInvoker->mplInfoListCLMap.FindIf(classLoader, handle);
  } else {
    (void)pInvoker->mplInfoList.FindIf(handle);
  }
  return res;
}
} // namespace maplert
