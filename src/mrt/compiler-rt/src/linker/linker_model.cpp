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
#include "linker/linker_model.h"

#include <thread>
#include <dirent.h>
#include <fstream>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/file.h>
#include <atomic>

#include "file_system.h"
#include "linker/linker_inline.h"
#ifdef LINKER_DECOUPLE
#include "linker/decouple/linker_decouple.h"
#endif
#include "linker/linker_cache.h"
#include "linker/linker_debug.h"
#include "linker/linker_hotfix.h"
#include "linker/linker_lazy_binding.h"
#include "utils/name_utils.h"
#include "file_layout.h"
#include "collector/cp_generator.h"

using namespace maple;
namespace maplert {
using namespace linkerutils;
bool LinkerMFileInfo::BelongsToApp() {
  LinkerInvoker &invoker = LinkerAPI::As<LinkerInvoker&>();
  return (!IsFlag(kIsBoot) && !invoker.IsSystemClassLoader(reinterpret_cast<MObject*>(this->classLoader)));
}

void LinkerMFileInfo::ReleaseReadOnlyMemory() {
  LINKER_VLOG(mpllinker) << "release file-mapped readonly memory for " << name << maple::endl;

   // release .refl_strtab
  ReleaseMemory(this->coldStrTab, this->coldStrTabEnd);

  // release .rometadata for java methods
  ReleaseMemory(this->rometadataMethodStart, this->rometadataMethodEnd);

  // release .rometadata for java fields
  ReleaseMemory(this->rometadataFieldStart, this->rometadataFieldEnd);

  // release .romuidtab
  ReleaseMemory(this->romuidtabStart, this->romuidtabEnd);
}

LinkerInvoker::LinkerInvoker() {
  InitProtectedRegion();
}
LinkerInvoker::~LinkerInvoker() {
  features.clear();
  ClearLinkerMFileInfo();
  pLoader = nullptr;
}

// Fetch the method address by index directly, in the define table of 'handle'.
void *LinkerInvoker::GetMethodSymbolAddress(LinkerMFileInfo &mplInfo, size_t index) {
  size_t methodDefSize = mplInfo.GetTblSize(kMethodDef);
  if (methodDefSize == 0) {
    LINKER_LOG(ERROR) << "failed, the size of methodDef is zero in " << mplInfo.name << maple::endl;
    return nullptr;
  }
  AddrSlice methodDefSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kMethodDefOrig), methodDefSize);
  if (index < methodDefSize) {
    return GetDefTableAddress(mplInfo, methodDefSlice, index, true);
  }
  return nullptr;
}

// Fetch the data address by index directly, in the define table of 'handle'.
void *LinkerInvoker::GetDataSymbolAddress(LinkerMFileInfo &mplInfo, size_t index) {
  size_t dataDefSize = mplInfo.GetTblSize(kDataDef);
  if (dataDefSize == 0) {
    LINKER_LOG(ERROR) << "failed, the size of dataDef is zero in " << mplInfo.name << maple::endl;
    return nullptr;
  }
  AddrSlice dataDefSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kDataDefOrig), dataDefSize);
  if (index < dataDefSize) {
    return GetDefTableAddress(mplInfo, dataDefSlice, index, false);
  }
  return nullptr;
}

bool LinkerInvoker::DoLocateAddress(const LinkerMFileInfo &mplInfo, LinkerLocInfo &info, const void *addr,
    const AddrSlice &addrSlice, const InfTableSlice &infTableSlice, int64_t pos, bool getName) {
  if (pos != -1) {
    info.size = infTableSlice[static_cast<size_t>(pos)].size;
    info.addr = GetDefTableAddress(mplInfo, addrSlice, static_cast<size_t>(pos), true);
    if (getName) {
      info.sym = GetMethodSymbolByOffset(infTableSlice[static_cast<size_t>(pos)]);
      // Get library name.
      info.path = mplInfo.name;
    } else {
      info.sym = "";
      info.path = "";
    }
    return true;
  } else {
    LINKER_VLOG(mpllinker) << "failed, not found addr=" << addr << " in " << mplInfo.name << maple::endl;
  }
  return false;
}

// Locate the address for method, which defined in the .so of 'handle'.
// Also see int dladdr(void *addr, Dl_info *info)
bool LinkerInvoker::LocateAddress(LinkerMFileInfo &mplInfo, const void *addr, LinkerLocInfo &info, bool getName) {
  size_t methodDefSize = mplInfo.GetTblSize(kMethodDef);
  if (methodDefSize == 0) {
    LINKER_DLOG(mpllinker) << "failed, size is zero, in " << mplInfo.name << maple::endl;
    return false;
  }
  AddrSlice methodDefSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kMethodDefOrig), methodDefSize);
  size_t indexSize = mplInfo.GetTblSize(kMethodMuidIndex);
  if (indexSize == 0) {
    LINKER_LOG(ERROR) << "failed, muidIndexTable size is zero" << " in " << mplInfo.name << maple::endl;
    return false;
  }
  MuidIndexSlice muidIndexSlice = MuidIndexSlice(
      mplInfo.GetTblBegin<LinkerMuidIndexTableItem*>(kMethodMuidIndex), indexSize);
  size_t infSize = mplInfo.GetTblSize(kMethodInfo);
  if (infSize == 0 || infSize != methodDefSize) {
    LINKER_LOG(ERROR) << "failed, infTable size is 0, or tables size are not equal, " << infSize << " vs. " <<
        methodDefSize << " in " << mplInfo.name << maple::endl;
    return false;
  }
  InfTableSlice infTableSlice = InfTableSlice(mplInfo.GetTblBegin<LinkerInfTableItem*>(kMethodInfo), infSize);

  const size_t start = 0;
  size_t end = indexSize - 1;
  size_t scopeStartAddr = mplInfo.GetTblBegin<size_t>(kJavaText);
  size_t scopeEndAddr = scopeStartAddr + mplInfo.GetTblSize(kJavaText);
  int64_t pos = BinarySearchIndex<AddrSlice, MuidIndexSlice, InfTableSlice>(mplInfo, methodDefSlice, start, end,
      reinterpret_cast<size_t>(addr), muidIndexSlice, infTableSlice, scopeStartAddr, scopeEndAddr);
  return DoLocateAddress(mplInfo, info, addr, methodDefSlice, infTableSlice, pos, getName);
}

// Locate the address for method, by traversal all open maple .so files.
// Also see int dladdr(void *addr, Dl_info *info)
bool LinkerInvoker::LocateAddress(const void *addr, LinkerLocInfo &info, bool getName) {
  if (UNLIKELY(addr == nullptr)) {
    return false;
  }
  LinkerMFileInfo *mplInfo = GetLinkerMFileInfo(kFromPC, addr);
  if (mplInfo != nullptr) {
    if (LocateAddress(*mplInfo, addr, info, getName)) {
      return true;
    }
    LINKER_VLOG(mpllinker) << "failed, not found " << addr << maple::endl;
  } else {
    LINKER_VLOG(mpllinker) << "failed, not found in JAVA section for " << addr << maple::endl;
  }
  return false;
}

// Return the LinkerMFileInfo pointer, null if not found.
LinkerMFileInfo *LinkerInvoker::SearchAddress(const void *pc, AddressRangeType type, bool isLazyBinding) {
  if (type == kTypeText) {
    return mplInfoElfAddrSet.SearchJavaText(pc);
  }
  if (isLazyBinding) {
    return mplInfoElfAddrLazyBindingSet.Search(pc, type);
  } else {
    return mplInfoElfAddrSet.Search(pc, type);
  }
}

// Get offset vtable origin address.
void *LinkerInvoker::GetMplOffsetValue(LinkerMFileInfo &mplInfo) {
  void *tableAddr = mplInfo.GetTblBegin(kValueOffset);
  return reinterpret_cast<char*>(tableAddr) + sizeof(LinkerOffsetKeyTableInfo);
}

// Get mpl linker validity code from range table.
MUID LinkerInvoker::GetValidityCode(LinkerMFileInfo &mplInfo) const {
  if (mplInfo.hash.data.words[0] == 0 && mplInfo.hash.data.words[1] == 0) {
    mplInfo.hash.data.words[0] = mplInfo.GetTblBegin<uintptr_t>(kRange);
    mplInfo.hash.data.words[1] = mplInfo.GetTblEnd<uintptr_t>(kRange);
  }
  return mplInfo.hash;
}

// Get decouple validity code from range table.
MUID LinkerInvoker::GetValidityCodeForDecouple(LinkerMFileInfo &mplInfo) const {
  if (mplInfo.hashOfDecouple.data.words[0] == 0 && mplInfo.hashOfDecouple.data.words[1] == 0) {
    mplInfo.hashOfDecouple.data.words[0] = mplInfo.GetTblBegin<uintptr_t>(kDecouple);
    mplInfo.hashOfDecouple.data.words[1] = mplInfo.GetTblEnd<uintptr_t>(kDecouple);
  }
  return mplInfo.hashOfDecouple;
}

// see if class's super symbol has been resolved
#ifdef LINKER_DECOUPLE
bool LinkerInvoker::IsClassComplete(const MClass &classInfo) {
  MClass **superTable = classInfo.GetSuperClassArrayPtr();
  uint32_t superSize = classInfo.GetNumOfSuperClasses();
  if (superTable != nullptr && superSize > 0) {
    if (!classInfo.IsColdClass()) {
      for (uint32_t i = 0; i < superSize; ++i) {
        LinkerRef ref(superTable[i]);
        if (ref.IsEmpty() || ref.IsIndex() || !IsClassComplete(*superTable[i])) {
          return false;
        }
      }
    } else {
      DecoupleMFileInfo *mplInfo = reinterpret_cast<DecoupleMFileInfo*>(SearchAddress(&classInfo, kTypeClass));
      if (mplInfo == nullptr) {
        LOG(ERROR) << "Decouple::IsClassComplete: not find so for " << classInfo.GetName() << maple::endl;
        return false;
      }
      for (uint32_t i = 0; i < superSize; ++i) {
        MClass *superClass = superTable[i];
        LinkerRef ref(superClass);
        if (ref.IsIndex()) {
          size_t index = ref.GetIndex();
          bool fromUndef = ref.IsFromUndef();
          if (fromUndef && index < mplInfo->dataUndefSlice.Size() && !mplInfo->dataUndefSlice.Empty()) {
            superClass = static_cast<MClass*>(mplInfo->dataUndefSlice[index].Address());
          } else if (!fromUndef && index < mplInfo->dataDefSlice.Size() && !mplInfo->dataDefSlice.Empty()) {
            superClass = static_cast<MClass*>(GetDefTableAddress(*mplInfo, mplInfo->dataDefSlice, index, false));
          }
        }
        if (superClass == nullptr || !IsClassComplete(*superClass)) {
          return false;
        }
      }
    }
  }
  return true;
}
#endif

void *LinkerInvoker::GetClassMetadataLazily(const void *offsetTable, size_t classIndex) {
  LinkerMFileInfo *mplInfo = GetLinkerMFileInfo(kFromAddr, offsetTable, true);
  if (mplInfo == nullptr) {
    LINKER_LOG(FATAL) << "failed to find mplInfo for " << offsetTable << maple::endl;
  }
  return GetClassMetadataLazily(*mplInfo, classIndex);
}

BindingState LinkerInvoker::GetAddrBindingState(LinkerVoidType addr) {
  return Get<LazyBinding>()->GetAddrBindingState(addr);
}
BindingState LinkerInvoker::GetAddrBindingState(const AddrSlice &addrSlice, size_t index, bool isAtomic) {
  return Get<LazyBinding>()->GetAddrBindingState(addrSlice, index, isAtomic);
}
void LinkerInvoker::DumpStackInfoInLog() {
  Get<Debug>()->DumpStackInfoInLog();
}
void *LinkerInvoker::GetClassMetadataLazily(LinkerMFileInfo &mplInfo, size_t classIndex) {
  return Get<LazyBinding>()->GetClassMetadata(mplInfo, classIndex);
}

MObject *LinkerInvoker::GetClassLoaderByAddress(LinkerMFileInfo &mplInfo, const void *addr) {
  if (addr != nullptr) {
    LINKER_VLOG(lazybinding) << "addr=" << addr << ", in " << mplInfo.name << maple::endl;
    LinkerLocInfo info;
    if (LocateAddress(mplInfo, addr, info, false) == false) {
      LINKER_LOG(ERROR) << "failed to locate, addr=" << addr << maple::endl;
      return nullptr;
    }
    uint32_t *method = static_cast<uint32_t*>(const_cast<void*>(info.addr));
    // To get the declaring class of the method.
    void *md = JavaFrame::GetMethodMetadata(method);
    if (md != nullptr) {
      const MethodMetaBase *methodMeta = reinterpret_cast<const MethodMetaBase*>(md);
      return reinterpret_cast<MObject*>(MRT_GetClassLoader(*methodMeta->GetDeclaringClass()));
    }
  }
  return nullptr;
}

// Look up the method address with MUID, in the define table of 'handle'.
// Also see LookUpSymbol()
LinkerVoidType LinkerInvoker::LookUpMethodSymbolAddress(LinkerMFileInfo &mplInfo, const MUID &muid, size_t &index) {
  index = 0;
  size_t methodDefSize = mplInfo.GetTblSize(kMethodDef);
  if (methodDefSize == 0) {
    LINKER_DLOG(mpllinker) << "failed, the size of methodDef table is zero, " << maple::endl;
    return 0;
  }
  AddrSlice methodDefSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kMethodDefOrig), methodDefSize);
  if (methodDefSlice.Empty()) {
    LINKER_DLOG(mpllinker) << "failed, methodDef table is null" << maple::endl;
    return 0;
  }
  LinkerMuidTableItem *pMuidTable = mplInfo.GetTblBegin<LinkerMuidTableItem*>(kMethodDefMuid);
  if (pMuidTable == nullptr) {
    LINKER_DLOG(mpllinker) << "failed, pMuidTable is null in " << mplInfo.name << maple::endl;
    return 0;
  }

  // Because method define table is sorted by address ascending order,
  // we must add an index side-table for binary searching address with muid.
  LinkerMuidIndexTableItem *pMuidIndexTable = mplInfo.GetTblBegin<LinkerMuidIndexTableItem*>(kMethodMuidIndex);
  size_t indexSize = mplInfo.GetTblSize(kMethodMuidIndex);
  if (pMuidIndexTable == nullptr || indexSize == 0) {
    LINKER_DLOG(mpllinker) << "failed, pMuidIndexTable is null, or size is zero" << maple::endl;
    return 0;
  }

  const int64_t start = 0;
  int64_t end = static_cast<int64_t>(methodDefSize) - 1;
  int64_t pos = Get<Linker>()->BinarySearch<LinkerMuidTableItem, LinkerMuidIndexTableItem, MUID>(
      *pMuidTable, *pMuidIndexTable, start, end, muid);
  if (pos != -1) {
    index = static_cast<size_t>(pos);
#ifdef LINKER_32BIT_REF_FOR_DEF_UNDEF
    return AddrToUint32(GetDefTableAddress(mplInfo, methodDefSlice, index, true));
#else
    return reinterpret_cast<LinkerVoidType>(GetDefTableAddress(mplInfo, methodDefSlice, index, true));
#endif // USE_32BIT_REF
  } else {
    LINKER_DLOG(mpllinker) << "failed, not found MUID=" << muid.ToStr() << maple::endl;
  }
  return 0;
}

// Look up the data with MUID, in the data define table of 'handle'.
// Also see LookUpSymbol()
LinkerVoidType LinkerInvoker::LookUpDataSymbolAddress(LinkerMFileInfo &mplInfo, const MUID &muid, size_t &index) {
  size_t dataDefSize = mplInfo.GetTblSize(kDataDef);
  if (dataDefSize == 0) {
    LINKER_DLOG(mpllinker) << "failed, the size of dataDef table is zero" << maple::endl;
    return 0;
  }
  AddrSlice dataDefSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kDataDefOrig), dataDefSize);
  if (dataDefSlice.Empty()) {
    LINKER_DLOG(mpllinker) << "failed, dataDef table is null" << maple::endl;
    return 0;
  }
  LinkerMuidTableItem *pMuidTable = mplInfo.GetTblBegin<LinkerMuidTableItem*>(kDataDefMuid);
  if (pMuidTable == nullptr) {
    LINKER_DLOG(mpllinker) << "failed, pMuidTable is null in " << mplInfo.name << maple::endl;
    return 0;
  }

  const int64_t start = 0;
  int64_t end = static_cast<int64_t>(dataDefSize) - 1;
  int64_t pos = Get<Linker>()->BinarySearch<LinkerMuidTableItem, MUID>(*pMuidTable, start, end, muid);
  if (pos != -1) {
    index = static_cast<size_t>(pos);
#ifdef LINKER_32BIT_REF_FOR_DEF_UNDEF
    return AddrToUint32(GetDefTableAddress(mplInfo, dataDefSlice, index, false));
#else
    return reinterpret_cast<LinkerVoidType>(GetDefTableAddress(mplInfo, dataDefSlice, index, false));
#endif // USE_32BIT_REF
  } else {
    LINKER_DLOG(mpllinker) << "failed, not found MUID=" << muid.ToStr() << maple::endl;
    return 0;
  }
}

void LinkerInvoker::ResolveVTableSymbolByClass(LinkerMFileInfo &mplInfo, const MClass *klass, bool forVtab) {
  LinkerVTableItem *pVTable = forVtab ? reinterpret_cast<LinkerVTableItem*>(
      klass->GetVtab()) : reinterpret_cast<LinkerVTableItem*>(klass->GetItab());
  if (pVTable == nullptr) {
    return;
  }
  bool hasNotResolved = false;
  uint32_t i = 0;
  bool endFlag = false;
  while (!endFlag) {
    size_t vtabIndexOrg = pVTable[i].index;
    LinkerRef vRef(vtabIndexOrg & (~kGoldSymbolTableEndFlag));
    endFlag = static_cast<bool>(vtabIndexOrg & kGoldSymbolTableEndFlag);
    if (vRef.IsVTabIndex()) { // Index of undefine table.
      hasNotResolved = ResolveSymbolByClass<kMethodUndef, kMethodDef, LinkerVTableItem, false>(mplInfo, pVTable[i],
          vRef.GetTabIndex(), vRef.GetRawValue<size_t>(), vRef.IsTabUndef());
    } else if (pVTable[i].index & kNegativeNum) { // Offset of address.
      DataRefOffset *klassIndex = reinterpret_cast<DataRefOffset*>(&pVTable[i].index);
#ifdef USE_32BIT_REF
      // To allow re-parse Vtable by the proper way.
      if (!(static_cast<uint32_t>(pVTable[i].index) < kDsoLoadedAddressEnd &&
          static_cast<uint32_t>(pVTable[i].index) >= kDsoLoadedAddressStart)) {
        pVTable[i].index = klassIndex->GetDataRef<size_t>();
      } else {
        LINKER_VLOG(mpllinker) << "(" << forVtab << "), not re-parse for patch, " << std::hex << "addr=" <<
            pVTable[i].index << ">X>" << klassIndex->GetDataRef<size_t>() << " in " << mplInfo.name << maple::endl;
      }
#else
      pVTable[i].index = klassIndex->GetDataRef<size_t>();
#endif
    } else if (!forVtab) {
      MByteRef32 *ref = reinterpret_cast<MByteRef32*>(&(pVTable[i].index));
      if (ref->IsOffset()) {
        std::abort();
      }
    }
    ++i;
  }
  if (!forVtab && i == kItabHashSize + 1) {
    ResolveITableSymbolByClass(mplInfo, klass);
  }
  if (hasNotResolved) {
    char *className = klass->GetName();
    LINKER_LOG(ERROR) << "failed, " << "mplInfo=" << mplInfo.name << ", class=" << className << maple::endl;
    return;
  }
}

void LinkerInvoker::ResolveITableSymbolByClass(LinkerMFileInfo &mplInfo, const MClass *klass) {
#ifdef USE_32BIT_REF
  int32_t *address = reinterpret_cast<int32_t*>(klass->GetItab()) + kItabHashSize;
  LinkerITableItem *pITable = reinterpret_cast<LinkerITableItem*>(static_cast<unsigned long>(*address) & 0xffffffff);
  size_t noConflictItabSize = (pITable != nullptr) ? pITable[0].index & 0xffff : 0;
#else
  LinkerITableItem *pITable = reinterpret_cast<LinkerITableItem*>(*reinterpret_cast<int64_t*>(
      reinterpret_cast<int64_t*>(klass->GetItab()) + kItabHashSize));
  size_t noConflictItabSize = (pITable != nullptr) ? pITable[0].index & 0xffffffff : 0;
#endif
  if (pITable == nullptr) {
    return;
  }
  size_t conflictItabStart = noConflictItabSize * 2 + 2; // 2 is to count conflictItabStart.
  size_t i = 2;
  bool endFlag = false;
  while (!endFlag) {
    if (i % 2 == 1) { // i % 2 == 1 means the number is odd.
      size_t itabIndexOrg = pITable[i].index;
      LinkerRef ref(itabIndexOrg & (~kGoldSymbolTableEndFlag));
      endFlag = ((itabIndexOrg & kGoldSymbolTableEndFlag) != 0);
      if (ref.IsITabIndex()) { // Index of undefine table.
        (void)ResolveSymbolByClass<kMethodUndef, kMethodDef, LinkerITableItem, false>(mplInfo, pITable[i],
            ref.GetTabIndex(), ref.GetRawValue<size_t>(), ref.IsTabUndef());
      } else if (ref.GetRawValue<size_t>() != 1) {
        LOG(FATAL) << "ResolveITableSymbolByClass failed, pITableIndex isn't 1" << maple::endl;
      }
    } else if (conflictItabStart <= i) {
#ifdef USE_32BIT_REF
      MByteRef32 *ref = reinterpret_cast<MByteRef32*>(&(pITable[i].index));
#else
      MByteRef *ref = reinterpret_cast<MByteRef*>(&(pITable[i].index));
#endif
      void *addr = ref->GetRef<void*>();
      ref->SetRef(addr);
    }
    ++i;
  }
}

void LinkerInvoker::ResolveSuperClassSymbolByClass(LinkerMFileInfo &mplInfo, const MClass *klass) {
  bool hasNotResolved = false;
  uint32_t superSize = klass->GetNumOfSuperClasses();
  if (superSize == 0) {
    return;
  }
  LinkerSuperClassTableItem *pSuperTable = reinterpret_cast<LinkerSuperClassTableItem*>(klass->GetSuperClassArrayPtr());
  if (pSuperTable == nullptr) {
    return;
  }

  for (uint32_t i = 0; i < superSize; ++i) {
    LinkerRef ref(pSuperTable[i].index);
    if (ref.IsIndex()) {
      size_t index = ref.GetIndex();
      bool fromUndef = ref.IsFromUndef();
      hasNotResolved = ResolveSymbolByClass<kDataUndef, kDataDef, LinkerSuperClassTableItem, true>(
          mplInfo, pSuperTable[i], index, 0, fromUndef);
    }
  }
  if (hasNotResolved) {
    char *className = klass->GetName();
    LINKER_LOG(ERROR) << "failed, " << "mplInfo=" << mplInfo.name << ", class=" << className << maple::endl;
    return;
  }
}

LinkerMFileInfo *LinkerInvoker::GetLinkerMFileInfo(MFileInfoSource source, const void *key, bool isLazyBinding) {
  if (source == kFromPC) {
    return SearchAddress(key, kTypeText, isLazyBinding);
  } else if (source == kFromAddr) {
    return SearchAddress(key, kTypeWhole, isLazyBinding);
  } else if (source == kFromMeta) {
    return SearchAddress(key, kTypeClass, isLazyBinding);
  } else if (source == kFromHandle) {
    LinkerMFileInfo *mplInfo = nullptr;
    (void)mplInfoHandleMap.Find(key, mplInfo);
    return mplInfo;
  } else if (source == kFromName) {
    const std::string name = reinterpret_cast<const char*>(key);
    LinkerMFileInfo *mplInfo = nullptr;
    (void)mplInfoNameMap.Find(name, mplInfo);
    return mplInfo;
  } else if (source == kFromClass) {
    return SearchAddress(key, kTypeClass);
  }
  return nullptr;
}

std::string LinkerInvoker::GetMFileName(MFileInfoSource source, const void *key, bool isLazyBinding) {
  LinkerMFileInfo *mplInfo = GetLinkerMFileInfo(source, key, isLazyBinding);
  return (mplInfo != nullptr) ? mplInfo->name : "";
}

bool LinkerInvoker::UpdateMethodSymbolAddressDef(const MClass *klass, const MUID &symbolId, const uintptr_t newAddr) {
  // update symbol table (defining library only)
  // get the defining library
  LinkerMFileInfo *mplInfo = GetLinkerMFileInfo(kFromMeta, klass);
  if (mplInfo == nullptr) { // it should be a dynamic-created class
    LINKER_LOG(ERROR) << "No defining library of " << klass->GetName() << maple::endl;
    return false;
  }
  LinkerMuidIndexTableItem *pIndexTable = mplInfo->GetTblBegin<LinkerMuidIndexTableItem*>(kMethodMuidIndex);
  size_t size = mplInfo->GetTblSize(kMethodDef);
  LinkerAddrTableItem *pTable = mplInfo->GetTblBegin<LinkerAddrTableItem*>(kMethodDef);
  LinkerMuidTableItem *pMuidTable = mplInfo->GetTblBegin<LinkerMuidTableItem*>(kMethodDefMuid);
  // search by unique id
  const int64_t start = 0;
  int64_t end = static_cast<int64_t>(size) - 1;
  int64_t pos = Get<Linker>()->BinarySearch<LinkerMuidTableItem, LinkerMuidIndexTableItem, MUID>(
      *pMuidTable, *pIndexTable, start, end, symbolId);
  if (pos == -1) { // no idea about it
    LINKER_LOG(ERROR) << "No symbol " << klass->GetName() << " in " << mplInfo->name << maple::endl;
    return false;
  }
  LINKER_VLOG(mpllinker) << "Update " << mplInfo->name << ":DefTab#" << pos << ":" << pTable[pos].addr << maple::endl;
  pTable[pos].addr = static_cast<LinkerOffsetType>(newAddr);
  return true;
}

bool LinkerInvoker::UpdateMethodSymbolAddressUndef(LinkerMFileInfo &mplInfo, const UpdateNode &node) {
  size_t methodUndefSize = mplInfo.GetTblSize(kMethodUndef);
  AddrSlice methodUndefSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(kMethodUndef), methodUndefSize);
  LinkerMuidTableItem *pMuidTable = mplInfo.GetTblBegin<LinkerMuidTableItem*>(kMethodUndefMuid);
  if (methodUndefSize != 0 && !methodUndefSlice.Empty() && pMuidTable != nullptr) {
    const int64_t start = 0;
    int64_t end = static_cast<int64_t>(methodUndefSize) - 1;
    int64_t pos = Get<Linker>()->BinarySearch<LinkerMuidTableItem, MUID>(*pMuidTable, start, end, node.symbolId);
    if (pos != -1 && (!mplInfo.IsFlag(kIsLazy) ||
        GetAddrBindingState(methodUndefSlice, static_cast<size_t>(pos)) == kBindingStateResolved)) {
      LINKER_VLOG(mpllinker) << "Update " << mplInfo.name << ":UndefTab#" << pos << ":" <<
          methodUndefSlice[static_cast<size_t>(pos)].addr << maple::endl;
      methodUndefSlice[static_cast<size_t>(pos)].addr = static_cast<LinkerOffsetType>(node.newAddr);
    }
  }
  return true;
}

bool LinkerInvoker::UpdateMethodSymbolAddressDecouple(LinkerMFileInfo &mplInfo, const UpdateNode &node) {
  { // update vtable
    size_t size = mplInfo.GetTblSize(kVTable);
    LinkerVTableItem *pTable = mplInfo.GetTblBegin<LinkerVTableItem*>(kVTable);
    if (size != 0 && pTable != nullptr) {
      for (size_t i = 0; i < size; ++i) {
        if (static_cast<uint32_t>(pTable[i].index) == node.oldAddr) { // compare by address
          LINKER_VLOG(mpllinker) << "Update " << mplInfo.name <<
              ":Vtable#" << i << ":" << node.oldAddr << maple::endl;
          pTable[i].index = reinterpret_cast<uintptr_t>(node.newAddr);
        }
      }
    }
  }

  if (node.klass->IsInterface()) {
    // interface has no itable
    return true;
  }

  { // update itable
    size_t size = mplInfo.GetTblSize(kITable);
    LinkerITableItem *pTable = mplInfo.GetTblBegin<LinkerITableItem*>(kITable);
    if (size != 0 && pTable != nullptr) {
      for (size_t i = 0; i < size; ++i) {
        if (static_cast<uint32_t>(pTable[i].index) == node.oldAddr) { // compare by address
          LINKER_VLOG(mpllinker) << "Update " << mplInfo.name <<
              ":Itable#" << i << ":" << node.oldAddr << maple::endl;
          pTable[i].index = reinterpret_cast<uintptr_t>(node.newAddr);
        }
      }
    }
  }
  return true; // next library
}

// Get class loader hierarchy list
void LinkerInvoker::GetClassLoaderList(const LinkerMFileInfo &mplInfo, ClassLoaderListT &clList, bool isNewList) {
  if (!isNewList || !clList.empty()) {
    return;
  }
  MObject *parent = reinterpret_cast<MObject*>(mplInfo.classLoader);
  MObject *system = reinterpret_cast<MObject*>(pLoader->GetSystemClassLoader());
  // first insert system class loader will cause last search
  if (parent != system && system != nullptr && mplInfo.name != Get<Hotfix>()->GetPatchPath()) {
    LINKER_VLOG(mpllinker) << "push SystemClassLoader(" << system << ")" << maple::endl;
    clList.push_back(reinterpret_cast<jobject>(system));
  }
  LINKER_VLOG(mpllinker) << ">>> building hierarchy for " << mplInfo.name << maple::endl;
  for (;;) {
    // Skip if it's boot class loader instance.
    if (parent == nullptr || !IsBootClassLoader(parent)) {
      LINKER_VLOG(mpllinker) << "push " << parent << maple::endl;
      clList.push_back(reinterpret_cast<jobject>(parent));
    } else {
      LINKER_VLOG(mpllinker) << "skip BootClassLoader(" << parent << ")" << maple::endl;
    }
    if (parent == nullptr) { // 'parent is null' means it reaches the boot class loader.
      LINKER_VLOG(mpllinker) << "<<< building hierarchy for " << mplInfo.name << maple::endl;
      break;
    }
    parent = reinterpret_cast<MObject*>(pLoader->GetCLParent(reinterpret_cast<jobject>(parent)));
  }
}

void LinkerInvoker::ResetClassLoaderList(const MObject *classLoader) {
  auto handle = [this](LinkerMFileInfo &mplInfo) {
    this->ResetMplInfoClList(mplInfo);
  };
  (void)mplInfoListCLMap.ForEach(classLoader, handle);
}

void LinkerInvoker::GetLinkerMFileInfos(LinkerMFileInfo &mplInfo, LinkerMFileInfoListT &fileList, bool isNewList) {
  // Lazy building class loader hierarchy.
  GetClassLoaderList(mplInfo, mplInfo.clList, isNewList);
  ClassLoaderListT &classLoaderList = mplInfo.clList;
  if (classLoaderList.empty()) {
    LINKER_LOG(ERROR) << "clList null for " << mplInfo.name << maple::endl;
    return;
  }

  // Iteration from parent to child.
  for (ClassLoaderListRevItT it = classLoaderList.rbegin(); it != classLoaderList.rend(); ++it) {
    mplInfoListCLMap.FindToExport(reinterpret_cast<MObject*>(*it), fileList);
  }
}

void LinkerInvoker::ClearLinkerMFileInfo() {
  auto handle = [this](LinkerMFileInfo &item) {
    (void)this;
#ifdef LINKER_RT_CACHE
    this->Get<LinkerCache>()->FreeAllTables(item);
#endif // LINKER_RT_CACHE
    delete (&item);
  };
  mplInfoList.ForEach(handle);
  mplInfoList.Clear();
  mplInfoNameMap.Clear();
  mplInfoHandleMap.Clear();
  mplInfoElfAddrSet.Clear();
  mplInfoElfAddrLazyBindingSet.Clear();
  mplInfoListCLMap.Clear();
  ClearAppInfo();
}
} // namespace maplert
