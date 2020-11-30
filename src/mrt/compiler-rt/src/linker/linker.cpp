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
#include "linker/linker.h"

#include <thread>
#include <dirent.h>
#include <fstream>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/file.h>

#include "file_system.h"
#include "linker/linker_inline.h"
#include "linker/linker_debug.h"
#include "linker/linker_cache.h"
#include "linker/linker_lazy_binding.h"
#ifdef LINKER_DECOUPLE
#include "linker/decouple/linker_decouple.h"
#endif
#include "collector/cp_generator.h"

using namespace maple;

namespace maplert {
using namespace linkerutils;
FeatureName Linker::featureName = kFLinker;
bool Linker::HandleSymbol() {
  bool ret = true;
  if (!HandleMethodSymbol()) {
    ret = false;
  }
  if (!HandleDataSymbol()) {
    ret = false;
  }
  return ret;
}

// Handle all methods relevant symbols for the .so file of 'handle', by traverse 'handles' list.
bool Linker::HandleMethodSymbol() {
  if (!LOG_NDEBUG) {
    (void)(pInvoker->ForEachDoAction(pInvoker->Get<Debug>(), &Debug::DumpMethodSymbol));
  }
  bool ret = true;
  if (!(pInvoker->ForEachDoAction(this, &Linker::ResolveMethodSymbol))) {
    ret = false;
  }
  if (!(pInvoker->ForEachDoAction(this, &Linker::RelocateMethodSymbol))) {
    ret = false;
  }
  if (!(pInvoker->ForEachDoAction(this, &Linker::ResolveVTableSymbol))) {
    ret = false;
  }
  if (!(pInvoker->ForEachDoAction(this, &Linker::ResolveITableSymbol))) {
    ret = false;
  }
  if (!LOG_NDEBUG) {
    (void)(pInvoker->ForEachDoAction(pInvoker->Get<Debug>(), &Debug::DumpMethodUndefSymbol));
  }
  return ret;
}

// Handle all data relevant symbols for the .so file of 'handle', by traverse 'handles' list.
bool Linker::HandleDataSymbol() {
  if (!LOG_NDEBUG) {
    (void)(pInvoker->ForEachDoAction(pInvoker->Get<Debug>(), &Debug::DumpDataSymbol));
  }
  bool ret = true;
  if (!(pInvoker->ForEachDoAction(this, &Linker::ResolveDataSymbol))) {
    ret = false;
  }
  if (!(pInvoker->ForEachDoAction(this, &Linker::RelocateDataSymbol))) {
    ret = false;
  }
  if (!(pInvoker->ForEachDoAction(this, &Linker::ResolveSuperClassSymbol))) {
    ret = false;
  }
  if (!(pInvoker->ForEachDoAction(this, &Linker::ResolveGCRootSymbol))) {
    ret = false;
  }

  if (!LOG_NDEBUG) {
    (void)(pInvoker->ForEachDoAction(pInvoker->Get<Debug>(), &Debug::DumpDataUndefSymbol));
  }
  return ret;
}

bool Linker::HandleSymbol(LinkerMFileInfo &mplInfo) {
  // Ignore the system class loader.
  if (!pInvoker->IsSystemClassLoader(reinterpret_cast<MObject*>(mplInfo.classLoader)) &&
      GetLoadState() == kLoadStateApk) {
    pInvoker->SubMultiSoPendingCount();
  }
  bool ret = true;
  if (!HandleMethodSymbol(mplInfo)) {
    ret = false;
  }
  if (!HandleDataSymbol(mplInfo)) {
    ret = false;
  }
  return ret;
}

// Handle all methods relevant symbols for the .so file of 'handle', by traverse 'handles' list.
bool Linker::HandleMethodSymbol(LinkerMFileInfo &mplInfo) {
  if (!LOG_NDEBUG) {
    (void)(pInvoker->DoAction(pInvoker->Get<Debug>(), &Debug::DumpMethodSymbol, mplInfo));
  }
  if (!mplInfo.IsFlag(kIsLazy)) {
    // If not resolved symbols exist after previous trial, we need a full traversal.
    if (methodHasNotResolved) {
      methodHasNotResolved = !pInvoker->ForEachDoAction(this, &Linker::ResolveMethodSymbol);
    } else {
      methodHasNotResolved = !pInvoker->DoAction(this, &Linker::ResolveMethodSymbol, mplInfo);
    }

    if (NeedRelocateSymbol(mplInfo.name)) { // Ignore the procedure for profiling top apps
      // If not relocated symbols exist after previous trial, we need a full traversal.
      if (methodHasNotRelocated) {
        methodHasNotRelocated = !pInvoker->ForEachDoAction(this, &Linker::RelocateMethodSymbol);
      } else {
        methodHasNotRelocated = !pInvoker->DoAction(this, &Linker::RelocateMethodSymbol, mplInfo);
      }
    }

    // If not resolved symbols exist after previous trial, we need a full traversal.
    if (vtableHasNotResolved) {
      vtableHasNotResolved = !pInvoker->ForEachDoAction(this, &Linker::ResolveVTableSymbol);
    } else {
      vtableHasNotResolved = !pInvoker->DoAction(this, &Linker::ResolveVTableSymbol, mplInfo);
    }

    // If not resolved symbols exist after previous trial, we need a full traversal.
    if (itableHasNotResolved) {
      itableHasNotResolved = !pInvoker->ForEachDoAction(this, &Linker::ResolveITableSymbol);
    } else {
      itableHasNotResolved = !pInvoker->DoAction(this, &Linker::ResolveITableSymbol, mplInfo);
    }
  }

  if (!LOG_NDEBUG) {
    (void)(pInvoker->DoAction(pInvoker->Get<Debug>(), &Debug::DumpMethodUndefSymbol, mplInfo));
  }
  return !methodHasNotResolved && !methodHasNotRelocated && !vtableHasNotResolved && !itableHasNotResolved;
}

// Handle all data relevant symbols for the .so file of 'handle', by traverse 'handles' list.
bool Linker::HandleDataSymbol(LinkerMFileInfo &mplInfo) {
  if (!LOG_NDEBUG) {
    (void)(pInvoker->DoAction(pInvoker->Get<Debug>(), &Debug::DumpDataSymbol, mplInfo));
  }
  if (!mplInfo.IsFlag(kIsLazy)) {
    // If not resolved symbols exist after previous trial, we need a full traversal.
    if (dataHasNotResolved) {
      dataHasNotResolved = !pInvoker->ForEachDoAction(this, &Linker::ResolveDataSymbol);
    } else {
      dataHasNotResolved = !pInvoker->DoAction(this, &Linker::ResolveDataSymbol, mplInfo);
    }

    if (NeedRelocateSymbol(mplInfo.name)) { // Ignore the procedure for profiling top apps
      // If not relocated symbols exist after previous trial, we need a full traversal.
      if (dataHasNotRelocated) {
        dataHasNotRelocated = !pInvoker->ForEachDoAction(this, &Linker::RelocateDataSymbol);
      } else {
        dataHasNotRelocated = !pInvoker->DoAction(this, &Linker::RelocateDataSymbol, mplInfo);
      }
    }

    // If not resolved symbols exist after previous trial, we need a full traversal.
    if (superClassHasNotResolved) {
      superClassHasNotResolved = !pInvoker->ForEachDoAction(this, &Linker::ResolveSuperClassSymbol);
    } else {
      superClassHasNotResolved = !pInvoker->DoAction(this, &Linker::ResolveSuperClassSymbol, mplInfo);
    }
  }

  // If not resolved symbols exist after previous trial, we need a full traversal.
  if (gcRootListHasNotResolved) {
    gcRootListHasNotResolved = !pInvoker->ForEachDoAction(this, &Linker::ResolveGCRootSymbol);
  } else {
    gcRootListHasNotResolved = !pInvoker->DoAction(this, &Linker::ResolveGCRootSymbol, mplInfo);
  }

  if (!LOG_NDEBUG) {
    (void)(pInvoker->DoAction(pInvoker->Get<Debug>(), &Debug::DumpDataUndefSymbol, mplInfo));
  }
  return !dataHasNotResolved && !dataHasNotRelocated && !superClassHasNotResolved && !gcRootListHasNotResolved;
}

// Resolve all undefined methods symbols for the .so file of 'handle', by traverse 'handles' list.
bool Linker::ResolveMethodSymbol(LinkerMFileInfo &mplInfo) {
  mplInfo.SetFlag(kIsMethodUndefHasNotResolved, false);
  if (mplInfo.IsFlag(kIsMethodDefResolved) || mplInfo.IsFlag(kIsLazy)) {
    return true;
  }
  size_t undefSize = mplInfo.GetTblSize(kMethodUndef);
  if (undefSize == 0) {
    LINKER_LOG(ERROR) << "undefSize=" << undefSize << ", " << mplInfo.name << maple::endl;
    mplInfo.SetFlag(kIsMethodDefResolved, true);
    return true;
  }
  AddrSlice addrSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kMethodUndef), undefSize);
  MuidSlice muidSlice(mplInfo.GetTblBegin<LinkerMuidTableItem*>(kMethodUndefMuid), undefSize);
  pInvoker->GetClassLoaderList(mplInfo, mplInfo.clList);
#ifdef LINKER_RT_CACHE
  pInvoker->Get<LinkerCache>()->ResolveMethodSymbol(mplInfo, addrSlice, muidSlice);
  if (!mplInfo.IsFlag(kIsLazy)) {
    pInvoker->Get<LinkerCache>()->FreeMethodUndefTable(mplInfo);
  }
#else
  for (size_t i = 0; i < undefSize; ++i) {
    LinkerOffsetType tmpAddr = 0;
    if (!pInvoker->ForEachLookUp(muidSlice[i].muid, pInvoker,
        &LinkerInvoker::LookUpMethodSymbolAddress, mplInfo, tmpAddr)) { // Not found
      if (!mplInfo.IsFlag(kIsLazy)) {
        mplInfo.SetFlag(kIsMethodUndefHasNotResolved, true);
      }
    } else { // Found
      addrSlice[i].addr = tmpAddr;
    }
  }
#endif
  if (!mplInfo.IsFlag(kIsMethodUndefHasNotResolved)) {
    mplInfo.SetFlag(kIsMethodDefResolved, true);
    return true;
  } else {
    LINKER_VLOG(mpllinker) << "failed to resolve all MUIDs for " << mplInfo.name << maple::endl;
    return false;
  }
}

#ifdef LINKER_LAZY_BINDING
#ifdef LINKER_32BIT_REF_FOR_DEF_UNDEF
void Linker::InitMethodSymbolLazy32(LinkerMFileInfo &mplInfo, size_t defSize) {
  if (mplInfo.IsFlag(kIsLazy)) {
    size_t undefSize = mplInfo.GetTblSize(kMethodUndef);
    LinkerAddrTableItem *pUndefTable = mplInfo.GetTblBegin<LinkerAddrTableItem*>(kMethodUndef);
    if (undefSize != 0 && pUndefTable != nullptr) {
      for (size_t i = 0; i < undefSize; ++i) {
        // + kBindingStateMethodUndef:0x5
        pUndefTable[i].addr = pInvoker->AddrToUint32(
            reinterpret_cast<void*>(__BindingProtectRegion__ + static_cast<int>(pUndefTable[i].addr)));
      }
    } else {
      LINKER_LOG(ERROR) << "failed, --lazy-binding, pUndefTable is invalid in " << mplInfo.name << maple::endl;
    }
    LinkerAddrTableItem *pDefTable = mplInfo.GetTblBegin<LinkerAddrTableItem*>(kMethodDef);
    if (pDefTable != nullptr) {
      for (size_t i = 0; i < defSize; ++i) {
        // + kBindingStateMethodDef:0x6
        pDefTable[i].addr = pInvoker->AddrToUint32(
            reinterpret_cast<void*>(__BindingProtectRegion__ + static_cast<int>(pDefTable[i].addr)));
      }
    } else {
      LINKER_LOG(ERROR) << "failed, --lazy-binding, pDefTable is null in " << mplInfo.name << maple::endl;
    }
  }
}

void Linker::InitDataSymbolLazy32(LinkerMFileInfo &mplInfo, size_t defSize) {
  if (mplInfo.IsFlag(kIsLazy)) {
    size_t undefSize = mplInfo.GetTblSize(kDataUndef);
    LinkerAddrTableItem *pUndefTable = mplInfo.GetTblBegin<LinkerAddrTableItem*>(kDataUndef);
    if (undefSize != 0 && pUndefTable != nullptr) {
      for (size_t i = 0; i < undefSize; ++i) {
        // + kBindingStateCinfUndef:1, or + kBindingStateDataUndef:3
        pUndefTable[i].addr = pInvoker->AddrToUint32(
            reinterpret_cast<void*>(__BindingProtectRegion__ + reinterpret_cast<int>(pUndefTable[i].addr)));
      }
    } else {
      LINKER_LOG(ERROR) << "failed, --lazy-binding, pUndefTable is invalid in " << mplInfo.name << maple::endl;
    }
    LinkerAddrTableItem *pDefTable = mplInfo.GetTblBegin<LinkerAddrTableItem*>(kDataDef);
    if (pDefTable != nullptr) {
      for (size_t i = 0; i < defSize; ++i) {
        // + kBindingStateCinfDef:2, or + kBindingStateDataDef:4
        pDefTable[i].addr = pInvoker->AddrToUint32(
            reinterpret_cast<void*>(__BindingProtectRegion__ + reinterpret_cast<int>(pDefTable[i].addr)));
      }
    } else {
      LINKER_LOG(ERROR) << "failed, --lazy-binding, pDefTable is null in " << mplInfo.name << maple::endl;
    }
  }
}
#endif // LINKER_32BIT_REF_FOR_DEF_UNDEF
#endif // LINKER_LAZY_BINDING

// Init all defined method symbols.
void Linker::InitMethodSymbol(LinkerMFileInfo &mplInfo) {
  // Fill the address by its offset.
  if (!mplInfo.IsFlag(kIsRelMethodOnce)) {
    mplInfo.SetFlag(kIsRelMethodOnce, true);
    size_t defSize = mplInfo.GetTblSize(kMethodDef);
    if (defSize == 0) {
      return;
    }

    // Check if it's lazy-binding flag for methods.
    if (mplInfo.GetTblSize(kMethodDefOrig) != 0) {
      LINKER_VLOG(mpllinker) << "applied compiler --lazy-binding option in " << mplInfo.name << maple::endl;
      mplInfo.SetFlag(kIsLazy, true);
    }

    LinkerAddrTableItem *pTable = mplInfo.GetTblBegin<LinkerAddrTableItem*>(kMethodDefOrig);
    if (pTable == nullptr) {
      LINKER_LOG(ERROR) << "failed, pTable is null in " << mplInfo.name << maple::endl;
      __MRT_ASSERT(0, "InitMethodSymbol(), pTable is null!\n");
    }

    if (!mplInfo.IsFlag(kIsLazy)) {
      for (size_t i = 0; i < defSize; ++i) {
        AdjustDefTableAddress(mplInfo, *pTable, i);
      }
    }

#ifdef LINKER_LAZY_BINDING
#ifdef LINKER_32BIT_REF_FOR_DEF_UNDEF
    InitMethodSymbolLazy32(mplInfo, defSize);
#endif // LINKER_32BIT_REF_FOR_DEF_UNDEF
#endif // LINKER_LAZY_BINDING
  }
}

// Relocate all defined method symbols.
bool Linker::RelocateMethodSymbol(LinkerMFileInfo &mplInfo) {
  mplInfo.SetFlag(kIsMethodDefHasNotResolved, false);
  if (mplInfo.IsFlag(kIsMethodRelocated) || mplInfo.IsFlag(kIsLazy) || mplInfo.name == maple::fs::kLibcorePath) {
    return true;
  }
  size_t defSize = mplInfo.GetTblSize(kMethodDef);
  if (defSize == 0) {
    mplInfo.SetFlag(kIsMethodRelocated, true);
    LINKER_LOG(ERROR) << "methodDef table is null in" << mplInfo.name << maple::endl;
    return true;
  }
  AddrSlice addrSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kMethodDef), defSize);
  MuidSlice muidSlice(mplInfo.GetTblBegin<LinkerMuidTableItem*>(kMethodDefMuid), defSize);
  pInvoker->GetClassLoaderList(mplInfo, mplInfo.clList);
#ifdef LINKER_RT_CACHE
  pInvoker->Get<LinkerCache>()->RelocateMethodSymbol(mplInfo, addrSlice, muidSlice);
  pInvoker->Get<LinkerCache>()->FreeMethodDefTable(mplInfo);
#else
  // Start binary search def from here.
  for (size_t i = 0; i < defSize; ++i) {
    LinkerOffsetType tmpAddr = 0;
    if (!pInvoker->ForEachLookUp(muidSlice[i].muid, pInvoker,
        &LinkerInvoker::LookUpMethodSymbolAddress, mplInfo, tmpAddr)) { // Not found
      // Never reach here.
      LINKER_LOG(ERROR) << "failed to relocate MUID=" << muidSlice[i].muid.ToStr() << " in " << mplInfo.name <<
          ", tmpAddr=" << tmpAddr;
      mplInfo.SetFlag(kIsMethodDefHasNotResolved, true);
    } else { // Found
      addrSlice[i].addr = tmpAddr;
    }
  }
#endif
  if (!mplInfo.IsFlag(kIsMethodDefHasNotResolved)) {
    mplInfo.SetFlag(kIsMethodRelocated, true);
    return true;
  } else {
    LINKER_VLOG(mpllinker) << "failed to relocate all MUIDs for " << mplInfo.name << maple::endl;
    return false;
  }
}

bool Linker::ResolveUndefVTableSymbol(LinkerMFileInfo &mplInfo, bool fromUndef, size_t index,
    VTableSlice &vTableSlice, size_t i) {
  size_t methodUndefSize = mplInfo.GetTblSize(kMethodUndef);
  AddrSlice methodUndefSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kMethodUndef), methodUndefSize);
  size_t methodDefSize = mplInfo.GetTblSize(kMethodDef);
  AddrSlice methodDefSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kMethodDefOrig), methodDefSize);
  bool hasNotResolved = false;
  if (fromUndef && index < methodUndefSize) {
    if (mplInfo.IsFlag(kIsLazy) && pInvoker->GetAddrBindingState(methodUndefSlice, index) != kBindingStateResolved) {
      size_t muidSize = mplInfo.GetTblSize(kMethodUndefMuid);
      MuidSlice muidSlice = MuidSlice(mplInfo.GetTblBegin<LinkerMuidTableItem*>(kMethodUndefMuid), muidSize);
      void *addr = pInvoker->Get<LazyBinding>()->ResolveMethodSymbol(
          mplInfo, methodUndefSlice, muidSlice, index, false);
      if (addr != nullptr) {
        vTableSlice[i].index = reinterpret_cast<size_t>(addr);
        LINKER_VLOG(lazybinding) << "resolved lazily, " << fromUndef << ", addr=" <<
            methodUndefSlice[index].Address() << " in " << mplInfo.name << maple::endl;
      } else {
        LINKER_LOG(ERROR) << "not resolved lazily, " << fromUndef << ", " << index << ", " << methodUndefSize << ", " <<
            methodDefSize << " in " << mplInfo.name << maple::endl;
        hasNotResolved = true;
      }
    } else {
      LINKER_DLOG(mpllinker) << "undef, " << std::hex << "addr=" << methodUndefSlice[index].Address() <<
          " in " << mplInfo.name << maple::endl;
      vTableSlice[i].index = reinterpret_cast<size_t>(methodUndefSlice[index].Address());
    }
  } else if (!fromUndef && index < methodDefSize) {
    LINKER_DLOG(mpllinker) << "def, " << std::hex << "addr=" << methodDefSlice[index].Address() << " in " <<
        mplInfo.name << maple::endl;
    vTableSlice[i].index = reinterpret_cast<size_t>(GetDefTableAddress(mplInfo, methodDefSlice,
        static_cast<size_t>(index), true));
  } else {
    LINKER_VLOG(mpllinker) << "not resolved, " << fromUndef << ", " << index << ", " << methodUndefSize << ", " <<
        methodDefSize << " in " << mplInfo.name << maple::endl;
    hasNotResolved = true;
  }
  return hasNotResolved;
}

// Resolve all VTable symbols for the .so file of 'handle'.
bool Linker::ResolveVTableSymbol(LinkerMFileInfo &mplInfo) {
  if (mplInfo.IsFlag(kIsVTabResolved) || mplInfo.IsFlag(kIsLazy)) {
    return true;
  }

  bool hasNotResolved = false;
  size_t vSize = mplInfo.GetTblSize(kVTable);
  if (vSize == 0) {
    mplInfo.SetFlag(kIsVTabResolved, true);
    LINKER_LOG(ERROR) << "vTable is null in " << mplInfo.name << maple::endl;
    return true;
  }
  VTableSlice vTableSlice = VTableSlice(mplInfo.GetTblBegin<LinkerVTableItem*>(kVTable), vSize);

  for (size_t i = 0; i < vSize; ++i) {
    LinkerRef ref(static_cast<uintptr_t>(vTableSlice[i].index));
    if (ref.IsVTabIndex()) { // Index of undefine table.
      hasNotResolved = ResolveUndefVTableSymbol(mplInfo, ref.IsTabUndef(), ref.GetTabIndex(), vTableSlice, i);
    } else if (vTableSlice[i].index & kNegativeNum) { // Offset of address.
      DataRefOffset *data = reinterpret_cast<DataRefOffset*>(&(vTableSlice[i].index));
#ifdef USE_32BIT_REF
      // To allow re-parse Vtable by the proper way.
      if (!(static_cast<uint32_t>(vTableSlice[i].index) < kDsoLoadedAddressEnd &&
          static_cast<uint32_t>(vTableSlice[i].index) >= kDsoLoadedAddressStart)) {
        vTableSlice[i].index = data->GetDataRef<size_t>();
      } else {
        LINKER_DLOG(mpllinker) << "not re-parse for patch, " << std::hex << "addr=" << vTableSlice[i].index <<
            ">X>" << data->GetDataRef<size_t>() << " in " << mplInfo.name << maple::endl;
      }
#else
      vTableSlice[i].index = data->GetDataRef<size_t>();
#endif
    }
  }
  if (!hasNotResolved) {
    mplInfo.SetFlag(kIsVTabResolved, true);
    LINKER_VLOG(mpllinker) << "successfully resolved VTable for " << mplInfo.name << maple::endl;
    return true;
  } else {
    LINKER_VLOG(mpllinker) << "failed to resolve all VTable for " << mplInfo.name << maple::endl;
    return false;
  }
}

bool Linker::ResolveUndefITableSymbol(
    LinkerMFileInfo &mplInfo, bool fromUndef, size_t index, ITableSlice &iTableSlice, size_t i) {
  size_t methodUndefSize = mplInfo.GetTblSize(kMethodUndef);
  AddrSlice methodUndefSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kMethodUndef), methodUndefSize);
  size_t methodDefSize = mplInfo.GetTblSize(kMethodDef);
  AddrSlice methodDefSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kMethodDefOrig), methodDefSize);
  bool hasNotResolved = false;
  if (fromUndef && index < methodUndefSize && !methodUndefSlice.Empty()) {
    if (mplInfo.IsFlag(kIsLazy) && pInvoker->GetAddrBindingState(methodUndefSlice, index) != kBindingStateResolved) {
      size_t muidSize = mplInfo.GetTblSize(kMethodUndefMuid);
      MuidSlice muidSlice = MuidSlice(mplInfo.GetTblBegin<LinkerMuidTableItem*>(kMethodUndefMuid), muidSize);
      void *addr = pInvoker->Get<LazyBinding>()->ResolveMethodSymbol(
          mplInfo, methodUndefSlice, muidSlice, index, false);
      if (addr != nullptr) {
        iTableSlice[i].index = reinterpret_cast<size_t>(addr);
        LINKER_VLOG(lazybinding) << "resolved lazily, " << fromUndef << ", addr=" <<
            methodUndefSlice[index].Address() << " in " << mplInfo.name << maple::endl;
      } else {
        LINKER_LOG(ERROR) << "not resolved lazily, " << fromUndef << ", " << index << ", " << methodUndefSize << ", " <<
            methodDefSize << " in " << mplInfo.name << maple::endl;
        hasNotResolved = true;
      }
    } else {
      iTableSlice[i].index = reinterpret_cast<size_t>(methodUndefSlice[index].Address());
    }
  } else if (!fromUndef && index < methodDefSize && !methodDefSlice.Empty()) {
    iTableSlice[i].index = reinterpret_cast<size_t>(GetDefTableAddress(mplInfo, methodDefSlice,
        static_cast<size_t>(index), true));
  } else {
    hasNotResolved = true;
  }
  return hasNotResolved;
}

// Resolve all ITable symbols for the .so file of 'handle'.
bool Linker::ResolveITableSymbol(LinkerMFileInfo &mplInfo) {
  if (mplInfo.IsFlag(kIsITabResolved) || mplInfo.IsFlag(kIsLazy)) {
    return true;
  }

  bool hasNotResolved = false;
  size_t iSize = mplInfo.GetTblSize(kITable);
  if (iSize == 0) {
    mplInfo.SetFlag(kIsITabResolved, true);
    LINKER_LOG(ERROR) << "iTable is null in " << mplInfo.name << maple::endl;
    return true;
  }
  ITableSlice iTableSlice = ITableSlice(mplInfo.GetTblBegin<LinkerITableItem*>(kITable), iSize);

  for (size_t i = 0; i < iSize; ++i) {
    if (i % 2 == 1) { // i % 2 is 1 means the number is odd.
      LinkerRef ref(static_cast<uintptr_t>(iTableSlice[i].index));
      if (ref.IsITabIndex()) { // Index of undefine table.
        hasNotResolved = ResolveUndefITableSymbol(mplInfo, ref.IsTabUndef(), ref.GetTabIndex(), iTableSlice, i);
      }
    } else { // Offset of address
#ifdef USE_32BIT_REF
      MByteRef32 *ref = reinterpret_cast<MByteRef32*>(&(iTableSlice[i].index));
      void *addr = ref->GetRef<void*>();
      ref->SetRef(addr);
#else
      MByteRef *ref = reinterpret_cast<MByteRef*>(&(iTableSlice[i].index));
      void *addr = ref->GetRef<void*>();
      ref->SetRef(addr);
#endif
    }
  }

  if (!hasNotResolved) {
    mplInfo.SetFlag(kIsITabResolved, true);
    LINKER_VLOG(mpllinker) << "successfully resolved ITable for " << mplInfo.name << maple::endl;
    return true;
  } else {
    LINKER_VLOG(mpllinker) << "failed to resolve all ITable for " << mplInfo.name << maple::endl;
    return false;
  }
}

// Resolve all undefined data symbols for the .so file of 'handle', by traverse 'handles' list.
bool Linker::ResolveDataSymbol(LinkerMFileInfo &mplInfo) {
  mplInfo.SetFlag(kIsDataUndefHasNotResolved, false);
  if (mplInfo.IsFlag(kIsDataDefResolved) || mplInfo.IsFlag(kIsLazy)) {
    return true;
  }
  size_t undefSize = mplInfo.GetTblSize(kDataUndef);
  if (undefSize == 0) {
    mplInfo.SetFlag(kIsDataDefResolved, true);
    LINKER_LOG(ERROR) << "DataUndef table is null in " << mplInfo.name << maple::endl;
    return true;
  }
  AddrSlice addrSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kDataUndef), undefSize);
  MuidSlice muidSlice(mplInfo.GetTblBegin<LinkerMuidTableItem*>(kDataUndefMuid), undefSize);
  pInvoker->GetClassLoaderList(mplInfo, mplInfo.clList);
#ifdef LINKER_RT_CACHE
  pInvoker->Get<LinkerCache>()->ResolveDataSymbol(mplInfo, addrSlice, muidSlice);
  if (!mplInfo.IsFlag(kIsLazy)) {
    pInvoker->Get<LinkerCache>()->FreeDataUndefTable(mplInfo);
  }
#else
  for (size_t i = 0; i < undefSize; ++i) {
    LinkerOffsetType tmpAddr = 0;
    if (!pInvoker->ForEachLookUp(muidSlice[i].muid, pInvoker,
        &LinkerInvoker::LookUpDataSymbolAddress, mplInfo, tmpAddr)) { // Not found
      if (!mplInfo.IsFlag(kIsLazy)) {
        mplInfo.SetFlag(kIsDataUndefHasNotResolved, true);
      }
    } else { // Found
      addrSlice[i].addr = tmpAddr;
    }
  }
#endif // LINKER_RT_CACHE
  if (!mplInfo.IsFlag(kIsDataUndefHasNotResolved)) {
    mplInfo.SetFlag(kIsDataDefResolved, true);
    return true;
  } else {
    LINKER_VLOG(mpllinker) << "failed to resolve all MUIDs for " << mplInfo.name << maple::endl;
    return false;
  }
}

inline void Linker::AdjustDefTableAddress(
    const LinkerMFileInfo &mplInfo, LinkerAddrTableItem &pTable, size_t index) {
#ifndef LINKER_ADDRESS_VIA_BASE
#ifdef LINKER_32BIT_REF_FOR_DEF_UNDEF
  (&pTable)[index].addr = pInvoker->AddrToUint32((&pTable)[index].AddressFromOffset());
#else
  (&pTable)[index].addr = reinterpret_cast<LinkerOffsetType>((&pTable)[index].AddressFromOffset());
#endif // USE_32BIT_REF
#else // LINKER_ADDRESS_VIA_BASE
#ifdef LINKER_32BIT_REF_FOR_DEF_UNDEF
  (&pTable)[index].addr = pInvoker->AddrToUint32((&pTable)[index].AddressFromBase(mplInfo.elfBase));
#else
  (&pTable)[index].addr = reinterpret_cast<LinkerOffsetType>((&pTable)[index].Address());
#endif // USE_32BIT_REF
#endif // LINKER_ADDRESS_VIA_BASE
  (void)(&mplInfo);
}

// Init all defined data symbols.
void Linker::InitDataSymbol(LinkerMFileInfo &mplInfo) {
  // Resolve the literal initialization entries based on the Literal pool
  void **cTable = mplInfo.GetTblBegin<void**>(kDataConstStr);
  if (cTable != nullptr) {
    for (size_t i = 0; i < mplInfo.GetTblSize(kDataConstStr); ++i) {
      DataRef *constStringRef = reinterpret_cast<DataRef*>(&cTable[i]);
      MString *oldStrObj = constStringRef->GetDataRef<MString*>();
      DCHECK(oldStrObj != nullptr);
      cTable[i] = GetOrInsertLiteral(*oldStrObj);
    }
  }

  // Fill the address by its offset.
  if (!mplInfo.IsFlag(kIsRelDataOnce)) {
    mplInfo.SetFlag(kIsRelDataOnce, true);
    size_t defSize = mplInfo.GetTblSize(kDataDef);
    if (defSize == 0) {
      return;
    }

#ifdef LINKER_LAZY_BINDING
    // Check if it's lazy-binding flag for cinf or data.
    if (mplInfo.GetTblSize(kDataDefOrig) != 0) {
      LINKER_VLOG(mpllinker) << "applied compiler --lazy-binding option in " << mplInfo.name << maple::endl;
      mplInfo.SetFlag(kIsLazy, true);
    }
#endif // LINKER_LAZY_BINDING

    LinkerAddrTableItem *pTable = mplInfo.GetTblBegin<LinkerAddrTableItem*>(kDataDefOrig);
    if (pTable == nullptr) {
      LINKER_LOG(ERROR) << "failed, pTable is null in " << mplInfo.name << maple::endl;
      __MRT_ASSERT(0, "InitDataSymbol(), pTable is null!\n");
    }

#ifdef LINKER_LAZY_BINDING
    if (!mplInfo.IsFlag(kIsLazy)) {
#endif // LINKER_LAZY_BINDING
      for (size_t i = 0; i < defSize; ++i) {
        AdjustDefTableAddress(mplInfo, *pTable, i);
      }
#ifdef LINKER_LAZY_BINDING
    }
#endif // LINKER_LAZY_BINDING

#ifdef LINKER_LAZY_BINDING
#ifdef LINKER_32BIT_REF_FOR_DEF_UNDEF
    InitDataSymbolLazy32(mplInfo, defSize);
#endif // LINKER_32BIT_REF_FOR_DEF_UNDEF
#endif // LINKER_LAZY_BINDING
  }
}

// Relocate all defined data symbols.
bool Linker::RelocateDataSymbol(LinkerMFileInfo &mplInfo) {
  mplInfo.SetFlag(kIsDataDefHasNotResolved, false);
  if (mplInfo.IsFlag(kIsDataRelocated) || mplInfo.IsFlag(kIsLazy) || mplInfo.name == maple::fs::kLibcorePath) {
    return true;
  }
  size_t defSize = mplInfo.GetTblSize(kDataDef);
  if (defSize == 0) {
    mplInfo.SetFlag(kIsDataRelocated, true);
    LINKER_LOG(ERROR) << "dataDef table is null in " << mplInfo.name << maple::endl;
    return true;
  }
  AddrSlice addrSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kDataDef), defSize);
  MuidSlice muidSlice(mplInfo.GetTblBegin<LinkerMuidTableItem*>(kDataDefMuid), defSize);
#ifdef LINKER_RT_CACHE
  pInvoker->Get<LinkerCache>()->RelocateDataSymbol(mplInfo, addrSlice, muidSlice);
  pInvoker->Get<LinkerCache>()->FreeDataDefTable(mplInfo);
#else
  pInvoker->GetClassLoaderList(mplInfo, mplInfo.clList);
  for (size_t i = 0; i < defSize; ++i) {
    LinkerOffsetType tmpAddr = 0;
    if (!pInvoker->ForEachLookUp(muidSlice[i].muid, pInvoker,
        &LinkerInvoker::LookUpDataSymbolAddress, mplInfo, tmpAddr)) { // Not found
      // Never reach here.
      LINKER_LOG(ERROR) << "failed to relocate MUID=" << muidSlice[i].muid.ToStr() << " in " << mplInfo.name <<
          ", tmpAddr=" << tmpAddr;
      mplInfo.SetFlag(kIsDataDefHasNotResolved, true);
    } else { // Found
      addrSlice[i].addr = tmpAddr;
    }
  }
#endif // LINKER_RT_CACHE
  if (!mplInfo.IsFlag(kIsDataDefHasNotResolved)) {
    mplInfo.SetFlag(kIsDataRelocated, true);
    return true;
  } else {
    LINKER_VLOG(mpllinker) << "failed to relocate all MUIDs for " << mplInfo.name << maple::endl;
    return false;
  }
}

bool Linker::DoResolveSuperClassSymbol(LinkerMFileInfo &mplInfo, IndexSlice &superTableSlice,
    const AddrSlice &dataUndefSlice, const AddrSlice &dataDefSlice, size_t i) {
  bool fromUpper = false;
  LinkerRef ref(superTableSlice[i].index);
  if (ref.IsIndex()) {
    size_t index = ref.GetIndex();
    bool fromUndef = ref.IsFromUndef();
    if (fromUndef && index < dataUndefSlice.Size() && !dataUndefSlice.Empty()) {
      if (mplInfo.IsFlag(kIsLazy) && pInvoker->GetAddrBindingState(dataUndefSlice, index) != kBindingStateResolved) {
        size_t muidSize = mplInfo.GetTblSize(kDataUndefMuid);
        MuidSlice muidSlice = MuidSlice(mplInfo.GetTblBegin<LinkerMuidTableItem*>(kDataUndefMuid), muidSize);
        void *addr = pInvoker->Get<LazyBinding>()->ResolveClassSymbol(
            mplInfo, dataUndefSlice, muidSlice, index, nullptr, fromUpper, false);
        if (addr != nullptr) {
          superTableSlice[i].index = reinterpret_cast<size_t>(addr);
          LINKER_VLOG(lazybinding) << "resolved lazily, " << "addr=" << dataUndefSlice[index].Address() << maple::endl;
        } else {
          LINKER_LOG(ERROR) << "not resolved lazily, " << fromUndef << ", " << index << ", " << dataUndefSlice.Size() <<
              ", " << dataDefSlice.Size() << " in " << mplInfo.name << maple::endl;
          return true;
        }
      } else {
        superTableSlice[i].index = reinterpret_cast<size_t>(dataUndefSlice[index].Address());
        LINKER_DLOG(mpllinker) << "undef, addr=" << std::hex << superTableSlice[i].index << maple::endl;
      }
    } else if (!fromUndef && index < dataDefSlice.Size() && !dataDefSlice.Empty()) {
      if (mplInfo.IsFlag(kIsLazy) && pInvoker->GetAddrBindingState(dataDefSlice, index) != kBindingStateResolved) {
        size_t muidSize = mplInfo.GetTblSize(kDataDefMuid);
        MuidSlice muidSlice = MuidSlice(mplInfo.GetTblBegin<LinkerMuidTableItem*>(kDataDefMuid), muidSize);
        void *addr = pInvoker->Get<LazyBinding>()->ResolveClassSymbol(
            mplInfo, dataDefSlice, muidSlice, index, nullptr, fromUpper, true);
        if (addr != nullptr) {
          superTableSlice[i].index = reinterpret_cast<size_t>(addr);
          LINKER_VLOG(lazybinding) << "resolved lazily, " << "addr=" << dataDefSlice[index].Address() << maple::endl;
        } else {
          LINKER_LOG(ERROR) << "not resolved lazily, " << fromUndef << ", " << index << ", " << dataUndefSlice.Size() <<
              ", " << dataDefSlice.Size() << " in " << mplInfo.name << maple::endl;
          return true;
        }
      } else {
        superTableSlice[i].index = reinterpret_cast<size_t>(dataDefSlice[index].Address());
        LINKER_DLOG(mpllinker) << "def, addr=" << std::hex << superTableSlice[i].index << maple::endl;
      }
    } else {
      return true;
    }
  }
  return false;
}

// Resolve all super-class symbols for the .so file of 'handle'.
bool Linker::ResolveSuperClassSymbol(LinkerMFileInfo &mplInfo) {
  if (mplInfo.IsFlag(kIsSuperClassResolved) || mplInfo.IsFlag(kIsLazy)) {
    return true;
  }

  bool hasNotResolved = false;
  size_t superSize = mplInfo.GetTblSize(kDataSuperClass);
  if (superSize == 0) {
    mplInfo.SetFlag(kIsSuperClassResolved, true);
    LINKER_LOG(ERROR) << "dataSuperClass table is null in " << mplInfo.name << maple::endl;
    return true;
  }
  IndexSlice superTableSlice = IndexSlice(mplInfo.GetTblBegin<LinkerSuperClassTableItem*>(kDataSuperClass), superSize);
  size_t dataUndefSize = mplInfo.GetTblSize(kDataUndef);
  AddrSlice dataUndefSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kDataUndef), dataUndefSize);
  size_t dataDefSize = mplInfo.GetTblSize(kDataDef);
  AddrSlice dataDefSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kDataDefOrig), dataDefSize);
  for (size_t i = 0; i < superSize; ++i) {
    hasNotResolved = DoResolveSuperClassSymbol(mplInfo, superTableSlice, dataUndefSlice, dataDefSlice, i);
  }
  if (!hasNotResolved) {
    mplInfo.SetFlag(kIsSuperClassResolved, true);
    LINKER_VLOG(mpllinker) << "successfully resolved super-class for " << mplInfo.name << maple::endl;
    return true;
  } else {
    LINKER_VLOG(mpllinker) << "failed to resolve all super-class for " << mplInfo.name << maple::endl;
    return false;
  }
}

// Resolve all super-class symbols for the .so file of 'handle'.
bool Linker::ResolveGCRootSymbol(LinkerMFileInfo &mplInfo) {
  if (mplInfo.IsFlag(kIsGCRootListResolved)) {
    LINKER_DLOG(mpllinker) << "already resolved for " << mplInfo.name << maple::endl;
    return true;
  }

  bool hasNotResolved = false;
  size_t gcRootSize = mplInfo.GetTblSize(kDataGcRoot);
  if (gcRootSize == 0) {
    mplInfo.SetFlag(kIsGCRootListResolved, true);
    LINKER_LOG(ERROR) << "gcRootTable is null in " << mplInfo.name << maple::endl;
    return true;
  }
  LinkerGCRootTableItem *gcRootTable = mplInfo.GetTblBegin<LinkerGCRootTableItem*>(kDataGcRoot);
  size_t dataDefSize = mplInfo.GetTblSize(kDataDef);
  AddrSlice dataDefSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kDataDefOrig), dataDefSize);

  for (size_t i = 0; i < gcRootSize; ++i) {
    LinkerRef ref(gcRootTable[i].index);
    if (ref.IsIndex()) { // Index
      size_t index = ref.GetIndex();
      if (!ref.IsFromUndef() && index < dataDefSize) {
        gcRootTable[i].index = reinterpret_cast<size_t>(GetDefTableAddress(mplInfo,
            dataDefSlice, static_cast<size_t>(index), false));
      } else {
        hasNotResolved = true;
      }
    }
  }
  if (!hasNotResolved) {
    mplInfo.SetFlag(kIsGCRootListResolved, true);
    LINKER_VLOG(mpllinker) << "successfully resolved GCRoots for " << mplInfo.name << maple::endl;
    return true;
  } else {
    LINKER_VLOG(mpllinker) << "failed to resolve all GCRoots for " << mplInfo.name << maple::endl;
    return false;
  }
}

// Look up the MUID in both method table and data table.
// Also see void *dlsym(void *handle, const char *symbol)
void *Linker::LookUpSymbolAddress(const void *handle, const MUID &muid) {
  LinkerMFileInfo *mplInfo = pInvoker->GetLinkerMFileInfo(kFromHandle, handle);
  if (mplInfo != nullptr) {
    return LookUpSymbolAddress(mplInfo, muid);
  }
  return nullptr;
}

// Look up the MUID in both method table and data table.
// Also see void *dlsym(void *handle, const char *symbol)
void *Linker::LookUpSymbolAddress(LinkerMFileInfo &mplInfo, const MUID &muid) {
  void *symbol = nullptr;
  size_t index = 0;
  symbol = reinterpret_cast<void*>(pInvoker->LookUpMethodSymbolAddress(mplInfo, muid, index));
  if (symbol == nullptr) {
    symbol = reinterpret_cast<void*>(pInvoker->LookUpDataSymbolAddress(mplInfo, muid, index));
  }
  return symbol;
}

// Look up the MUID in both method table and data table.
// Also see void *dlsym(void *handle, const char *symbol)
void *Linker::LookUpSymbolAddress(const MUID &muid) {
  void *addr = nullptr;
  auto handle = [this, &muid, &addr](LinkerMFileInfo &mplInfo)->bool {
    addr = this->LookUpSymbolAddress(mplInfo, muid);
    if (addr != nullptr) {
      LINKER_VLOG(mpllinker) << "found address of " << muid.ToStr() << ", in " << mplInfo.name << maple::endl;
      return true;
    }
    return false;
  };
  (void)pInvoker->mplInfoList.FindIf(handle);
  return addr;
}

void Linker::InitLinkerMFileInfoIgnoreSysCL(LinkerMFileInfo &mplInfo) {
  mplInfo.hash = pInvoker->GetValidityCode(mplInfo);
  mplInfo.hashOfDecouple = pInvoker->GetValidityCodeForDecouple(mplInfo);
  bool isBoot = (mplInfo.classLoader == nullptr ||
      pInvoker->IsBootClassLoader(reinterpret_cast<const MObject*>(mplInfo.classLoader)));
  mplInfo.SetFlag(kIsBoot, isBoot);
  if (!pInvoker->IsSystemClassLoader(reinterpret_cast<const MObject*>(mplInfo.classLoader)) &&
      GetLoadState() == kLoadStateApk) {
    pInvoker->AddMultiSoPendingCount();
    if (GetAppLoadState() == kAppLoadBaseOnlyReady) {
      SetAppLoadState(kAppLoadBaseOnly);

      // For multi-so, set kGlobalAppBaseStr as
      // "/data/app/com.sina.weibolite-ylociBbJwvlbG4TkHkRGkQ==/maple/arm64/"
      // without "mapleclasses.so"
      size_t posSeparator = mplInfo.name.rfind("/");
      SetAppBaseStr(mplInfo.name.substr(0, posSeparator + 1));
      LINKER_VLOG(mpllinker) << "GlobalAppBaseStr=" << GetAppBaseStr().c_str() << " in " << mplInfo.name << maple::endl;
    } else if (GetAppLoadState() == kAppLoadBaseOnly) {
      size_t posSeparator = mplInfo.name.rfind("/");
      if (mplInfo.name.substr(0, posSeparator + 1) != GetAppBaseStr()) {
        LINKER_VLOG(lazybinding) << "kAppLoadBaseOnly-->kAppLoadBaseAndOthers, " << mplInfo.name << maple::endl;
        SetAppLoadState(kAppLoadBaseAndOthers);
      }
    }
  }
}

void Linker::InitLinkerMFileInfoTableAddr(LinkerMFileInfo &mplInfo) const {
  static const void *mapleCoreElfStart = nullptr;
  uint64_t *rangeStart = static_cast<uint64_t*>(GetSymbolAddr(mplInfo.handle, kRangeTableFunc, true));
  uint64_t *rangeEnd = static_cast<uint64_t*>(GetSymbolAddr(mplInfo.handle, kRangeTableEndFunc, true));
  mplInfo.tableAddr = reinterpret_cast<PtrMatrixType>(rangeStart);
  uint64_t maxLength = static_cast<uint64_t>(rangeEnd - rangeStart) / kTable2ndDimMaxCount;
  mplInfo.rangeTabSize = static_cast<uint32_t>(maxLength);

  mplInfo.elfStart = GetSymbolAddr(mplInfo.handle, kMapleStartFunc, true);
  mplInfo.elfEnd = GetSymbolAddr(mplInfo.handle, kMapleEndFunc, true);

  // Update the start and end address of the maple file(ELF). according by libmaplecore-all.so
  if (mapleCoreElfStart == mplInfo.elfStart) {
    mplInfo.elfStart = nullptr;
  }

  if (mplInfo.elfStart == nullptr || mplInfo.elfEnd == nullptr) {
    // Update with the start of range table.
    void *start = mplInfo.tableAddr;
    mplInfo.elfStart = (mplInfo.elfStart == nullptr || mplInfo.elfStart > start) ? start : mplInfo.elfStart;
    // Update with outline of range table end.
    void *end = mplInfo.tableAddr + maxLength;
    mplInfo.elfEnd = mplInfo.elfEnd > end ? mplInfo.elfEnd : end;
    // Start from kVTable, ignore kRange.
    for (size_t i = static_cast<size_t>(kVTable); i < static_cast<size_t>(maxLength); ++i) {
      // Update the start address.
      start = mplInfo.tableAddr[i][kTable1stIndex];
      mplInfo.elfStart = (start != nullptr && mplInfo.elfStart > start) ? start : mplInfo.elfStart;
      // Update the end address.
      end = mplInfo.tableAddr[i][kTable2ndIndex];
      mplInfo.elfEnd = mplInfo.elfEnd > end ? mplInfo.elfEnd : end;
    }
  }
  if (mplInfo.name == maple::fs::kLibcorePath) {
    mapleCoreElfStart = mplInfo.elfStart;
  }
}

// Set the maple file's handle and path name.
// We save them in a map, then we can query the path by handle in the map.
// Also see GetDlopenMapleFiles().
bool Linker::InitLinkerMFileInfo(LinkerMFileInfo &mplInfo, int32_t pos) {
  InitLinkerMFileInfoTableAddr(mplInfo);
  // Ignore the system class loader.
  InitLinkerMFileInfoIgnoreSysCL(mplInfo);
  InitMethodSymbol(mplInfo);
  InitDataSymbol(mplInfo);

#ifdef LINKER_DECOUPLE
  DecoupleMFileInfo *dpInfo = reinterpret_cast<DecoupleMFileInfo*>(&mplInfo);
  size_t dataDefSize = mplInfo.GetTblSize(kDataDef);
  size_t dataUndefSize = mplInfo.GetTblSize(kDataUndef);
  dpInfo->dataDefSlice = AddrSlice(dpInfo->GetTblBegin<LinkerAddrTableItem*>(kDataDefOrig), dataDefSize);
  dpInfo->dataUndefSlice = AddrSlice(dpInfo->GetTblBegin<LinkerAddrTableItem*>(kDataUndef), dataUndefSize);
  pInvoker->Get<Decouple>()->InitDecoupledClasses(dpInfo);
#endif // LINKER_DECOUPLE

  mplInfo.startHotStrTab = reinterpret_cast<char*>(GetSymbolAddr(mplInfo.handle, kStartHotStrTabBegin, true));
  mplInfo.bothHotStrTab = reinterpret_cast<char*>(GetSymbolAddr(mplInfo.handle, kBothHotStrTabBegin, true));
  mplInfo.runHotStrTab = reinterpret_cast<char*>(GetSymbolAddr(mplInfo.handle, kRunHotStrTabBegin, true));
  mplInfo.coldStrTab = reinterpret_cast<char*>(GetSymbolAddr(mplInfo.handle, kColdStrTabBegin, true));
  mplInfo.coldStrTabEnd = reinterpret_cast<char*>(GetSymbolAddr(mplInfo.handle, kColdStrTabEnd, true));

  mplInfo.rometadataFieldStart = GetSymbolAddr(mplInfo.handle, kMetadataFieldStart, true);
  mplInfo.rometadataFieldEnd = GetSymbolAddr(mplInfo.handle, kMetadataFieldEnd, true);

  mplInfo.rometadataMethodStart = GetSymbolAddr(mplInfo.handle, kMetadataMethodStart, true);
  mplInfo.rometadataMethodEnd = GetSymbolAddr(mplInfo.handle, kMetadataMethodEnd, true);

  mplInfo.romuidtabStart = GetSymbolAddr(mplInfo.handle, kMuidTabStart, true);
  mplInfo.romuidtabEnd = GetSymbolAddr(mplInfo.handle, kMuidTabEnd, true);

  pInvoker->mplInfoNameMap.Append(mplInfo);
  pInvoker->mplInfoHandleMap.Append(mplInfo);
  pInvoker->mplInfoList.Append(mplInfo, pos);
  pInvoker->mplInfoListCLMap.Append(mplInfo, pos);
  bool isSuccess = pInvoker->mplInfoElfAddrSet.Append(mplInfo);
  __MRT_ASSERT(isSuccess, "mplInfoElfAddrSet.insert failed\n");
  if (mplInfo.IsFlag(kIsLazy)) {
    isSuccess = pInvoker->mplInfoElfAddrLazyBindingSet.Append(mplInfo);
    __MRT_ASSERT(isSuccess, "mplInfoElfAddrLazyBindingSet.insert failed\n");
#if defined(LINKER_RT_CACHE) && defined(LINKER_RT_LAZY_CACHE)
    pInvoker->PreLinkLazyMethod(&mplInfo);
#endif // defined(LINKER_RT_CACHE) && defined(LINKER_RT_LAZY_CACHE)
  }
  return mplInfo.IsFlag(kIsLazy);
}

void Linker::FreeAllCacheTables(const MObject *classLoader) const {
#ifdef LINKER_RT_CACHE
  auto handle = [this](LinkerMFileInfo &mplInfo) {
    this->pInvoker->Get<LinkerCache>()->FreeAllTables(mplInfo);
  };
  pInvoker->mplInfoListCLMap.ForEach(classLoader, handle);
#endif
  (void)classLoader;
}
} // namespace maplert
