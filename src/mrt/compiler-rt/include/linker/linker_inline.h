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
#ifndef MAPLE_RUNTIME_MPL_LINKER_INLINE_H_
#define MAPLE_RUNTIME_MPL_LINKER_INLINE_H_

#include "linker_api.h"
#include "linker_hotfix.h"
#include "linker_lazy_binding.h"
#include "linker.h"

namespace maplert {
// Do action for each maple file
template<typename Type, typename F, typename Data>
bool LinkerInvoker::ForEachDoAction(Type obj, F action, const Data &data) {
  bool ret = true;
  auto handle = [&](LinkerMFileInfo &mplInfo) {
    if (!(obj->*action)(mplInfo, *data)) {
      ret = false;
    }
  };
  (void)mplInfoListCLMap.ForEach(handle);
  return ret;
}
template<typename Type, typename F>
bool LinkerInvoker::ForEachDoAction(Type obj, F action) {
  bool ret = true;
  auto handle = [&](LinkerMFileInfo &mplInfo) {
    if (!(obj->*action)(mplInfo)) {
      ret = false;
    }
  };
  (void)mplInfoListCLMap.ForEach(handle);
  return ret;
}
template<typename Type, typename F>
bool LinkerInvoker::DoAction(Type obj, F action, LinkerMFileInfo &mplInfo) {
  bool ret = true;
  ret = (obj->*action)(mplInfo);
  return ret;
}

// Return the position in un|define table, -1 if not found.
// pTable: Main table, maintaining all data.
// pMuidIndexTable: Index table, use it to find the index in muid table.
// start: Start index of sorted table, if index table exists, use index table start index.
// End: End index of sorted table, if index table exists, use index table end index.
// value: Value, to search.
template<typename T1, typename T2, typename T3>
int64_t Linker::BinarySearch(T1 &pTable, const T2 &pMuidIndexTable, int64_t start, int64_t end, const T3 &value) {
  int64_t mid = 0;
  while (start <= end) {
    mid = (start + end) / 2;
    // reserve index low 31 bits
    uint64_t index = (&pMuidIndexTable)[mid].index & ~(1UL << 31);
    T1 *tmp = &pTable + index;
    if (*tmp < value) { // Less than value
      start = mid + 1;
    } else if (*tmp > value) { // Greater than value
      end = mid - 1;
    } else { // Resolved
      return static_cast<int64_t>(index);
    }
  }
  return -1; // Not resolved
}

// Return the position in un|define table, -1 if not found.
// pTable: Main table, maintaining all data.
// start: Start index of sorted table, if index table exists, use index table statrt index.
// End: End index of sorted table, if index table exists, use index table end index.
// value: Value, to search.
template<typename T1, typename T2>
int64_t Linker::BinarySearch(T1 &pTable, int64_t start, int64_t end, const T2 &value) {
  int64_t mid = 0;
  while (start <= end) {
    mid = (start + end) / 2;
    T1 *tmp = &pTable + mid;
    if (*tmp < value) { // Less than value
      start = mid + 1;
    } else if (*tmp > value) { // Greater than value
      end = mid - 1;
    } else { // Resolved
      return mid;
    }
  }
  return -1; // Not resolved
}

inline bool AddrInScope(size_t addr, size_t start, size_t end, uint32_t index, const int skipInvalidBitsNum) {
  return ((!((addr >= start) && (addr < end))) || ((index >> static_cast<uint32_t>(skipInvalidBitsNum)) & 1));
}
// Return the position in un|define table, -1 if not found.
//
// table: Main table, maintaining all data.
// start: Start index of sorted table, if index table exists, use index table statrt index.
// end: End index of sorted table, if index table exists, use index table end index.
// value: Value, to search.
// indexTable: Index table, containing sorted indice.
// infoTable: Info. table, containing sorted indice.
// scopeStartAddr: Java text section start address.
// scopeEndAddr: Java text section end address.
template<typename T1, typename T2, typename T3>
int64_t LinkerInvoker::BinarySearchIndex(const LinkerMFileInfo &mplInfo, const T1 &table, size_t start,
    size_t end, const size_t value, const T2 &indexTable, T3 &infTable, size_t scopeStartAddr,
    size_t scopeEndAddr) {
  constexpr int skipInvalidBitsNum = 31;
  while (AddrInScope(reinterpret_cast<size_t>(linkerutils::GetDefTableAddress(mplInfo, table, start, true)),
      scopeStartAddr, scopeEndAddr, indexTable[start].index, skipInvalidBitsNum) && (start <= end)) {
    ++start; // Skip invalid address towards END. Check 32 bit
  }
  while (AddrInScope(reinterpret_cast<size_t>(linkerutils::GetDefTableAddress(mplInfo, table, end, true)),
      scopeStartAddr, scopeEndAddr, indexTable[end].index, skipInvalidBitsNum) && (end >= start)) {
    --end; // Skip invalid address towards START. Check 32 bit
  }
  while (start <= end) {
    size_t mid = (start + end) / 2;
    while (AddrInScope(reinterpret_cast<size_t>(linkerutils::GetDefTableAddress(mplInfo, table, mid, true)),
        scopeStartAddr, scopeEndAddr, indexTable[mid].index, skipInvalidBitsNum) && (mid < end)) {
      ++mid; // Skip invalid address towards END. Check 32 bit
    }
    if (mid == end) { // Check if all addresses are invalid between MID and END
      mid = (start + end) / 2;
      while (AddrInScope(reinterpret_cast<size_t>(linkerutils::GetDefTableAddress(mplInfo, table, mid, true)),
          scopeStartAddr, scopeEndAddr, indexTable[mid].index, skipInvalidBitsNum) && (mid > start)) {
        --mid; // Skip invalid address towards START. Check 32 bit
      }
      // Here, mid is equal to start or a position between start and end.
    }
    size_t index = mid;
    void *addr = linkerutils::GetDefTableAddress(mplInfo, table, index, true);
    size_t startAddr = reinterpret_cast<size_t>(addr);
    size_t endAddr = reinterpret_cast<size_t>(addr) + static_cast<size_t>(infTable[index].size) - 1;
    if (endAddr < value) { // Less than value
      start = mid + 1;
      while (AddrInScope(reinterpret_cast<size_t>(linkerutils::GetDefTableAddress(mplInfo, table, start, true)),
          scopeStartAddr, scopeEndAddr, indexTable[mid].index, skipInvalidBitsNum) && (start < end)) {
        ++start; // Skip invalid address towards END. Check 32 bit
      }
    } else if (startAddr > value) { // Greater than value
      end = mid - 1;
      while (AddrInScope(reinterpret_cast<size_t>(linkerutils::GetDefTableAddress(mplInfo, table, end, true)),
          scopeStartAddr, scopeEndAddr, indexTable[mid].index, skipInvalidBitsNum) && (end > start)) {
        --end; // Skip invalid address towards START. Check 32 bit
      }
    } else {
      return static_cast<int64_t>(index); // Resolved
    }
  }
  return -1; // Not resolved
}

// Look up for each maple file.
template<typename Type, typename F>
bool LinkerInvoker::ForEachLookUp(const MUID &muid, Type obj, F lookup, LinkerMFileInfo &mplInfo,
#ifdef LINKER_RT_CACHE
    LinkerMFileInfo **resInfo, size_t &index,
#endif // LINKER_RT_CACHE
    LinkerOffsetType &pAddr) {
  // Find LinkerMFileInfo list by class loader.
  auto handle = [&](LinkerMFileInfo &item)->bool {
    LinkerVoidType addr = 0;
    size_t tmp = 0;
    addr = (obj->*lookup)(item, muid, tmp);
    if (addr != 0) {
      *reinterpret_cast<LinkerVoidType*>(&pAddr) = addr;
#ifdef LINKER_RT_CACHE
      *resInfo = &item;
      index = tmp;
#endif // LINKER_RT_CACHE
      return true;
    }
    return false;
  };
  ClassLoaderListT &classLoaderList = mplInfo.clList;
  // Look up the address.
  for (auto iter = classLoaderList.rbegin(); iter != classLoaderList.rend(); ++iter) {
    MObject *classLoader = reinterpret_cast<MObject*>(*iter);
#ifdef LINKER_RT_CACHE
    // Ignore the LinkerMFileInfos not from system(Boot class loader) for lazy binding with half option.
    if (mplInfo.IsFlag(kIsLazy) && !IsBootClassLoader(classLoader) && classLoader != nullptr &&
        !IsSystemClassLoader(classLoader)) { // Not system
      continue;
    }
#endif // LINKER_RT_CACHE
    if (mplInfoListCLMap.FindIf(classLoader, handle)) {
      return true;
    }
  }
  return false;
}

template<typename TableItem>
bool LinkerInvoker::ResolveSymbolLazily(LinkerMFileInfo &mplInfo, bool isSuper, const AddrSlice &dstSlice,
    size_t index, bool fromUpper, bool fromUndef, TableItem &tableItem, size_t tableIndex, const void *addr) {
  size_t muidSize = fromUndef ?
      (isSuper ? (mplInfo.GetTblSize(kDataUndefMuid)) : (mplInfo.GetTblSize(kMethodUndefMuid))) :
      (isSuper ? (mplInfo.GetTblSize(kDataDefMuid)) : (mplInfo.GetTblSize(kMethodDefMuid)));
  MuidSlice muidSlice = fromUndef ?
      (isSuper ? (MuidSlice(mplInfo.GetTblBegin<LinkerMuidTableItem*>(kDataUndefMuid), muidSize)) :
      (MuidSlice(mplInfo.GetTblBegin<LinkerMuidTableItem*>(kMethodUndefMuid), muidSize))) :
      (isSuper ? (MuidSlice(mplInfo.GetTblBegin<LinkerMuidTableItem*>(kDataDefMuid), muidSize)) :
      (MuidSlice(mplInfo.GetTblBegin<LinkerMuidTableItem*>(kMethodDefMuid), muidSize)));
  if (isSuper) {
    addr = Get<LazyBinding>()->ResolveClassSymbol(mplInfo, dstSlice, muidSlice, index, nullptr, fromUpper, !fromUndef);
  } else {
    addr = Get<LazyBinding>()->ResolveMethodSymbol(mplInfo, dstSlice, muidSlice, index, !fromUndef);
  }
  if (addr != nullptr) {
    __atomic_store_n(&tableItem.index, reinterpret_cast<size_t>(addr), __ATOMIC_RELEASE);
    return false;
  } else {
    LINKER_LOG(ERROR) << "not resolved lazily, " << fromUndef << ", " << index << ", " << " in " <<
        mplInfo.name << maple::endl;
    return isSuper ? true : ((tableIndex != 0) ?  true : false);
  }
}

template<SectTabIndex UndefName, SectTabIndex DefName, typename TableItem, bool isSuper>
bool LinkerInvoker::ResolveSymbolByClass(LinkerMFileInfo &mplInfo,
    TableItem &tableItem, size_t index, size_t tableIndex, bool fromUndef) {
  bool fromUpper = false;
  void *addr = nullptr;
  AddrSlice dstSlice;
  bool ret = false;
  size_t undefSize = mplInfo.GetTblSize(UndefName);
  AddrSlice undefSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(UndefName), undefSize);
  size_t defSize = mplInfo.GetTblSize(DefName);
  AddrSlice defSlice(mplInfo.GetTblBegin<LinkerAddrTableItem*>(DefName), defSize);
  if (fromUndef && index < undefSize && !undefSlice.Empty()) {
    if (mplInfo.IsFlag(kIsLazy) && GetAddrBindingState(undefSlice, index) != kBindingStateResolved) {
      dstSlice = undefSlice;
      ret = ResolveSymbolLazily<TableItem>(
          mplInfo, isSuper, dstSlice, index, fromUpper, fromUndef, tableItem, tableIndex, addr);
    } else {
      __atomic_store_n(&tableItem.index, reinterpret_cast<size_t>(undefSlice[index].Address()), __ATOMIC_RELEASE);
    }
  } else if (!fromUndef && index < defSize && !defSlice.Empty()) {
    if (mplInfo.IsFlag(kIsLazy)) {
      dstSlice = defSlice;
      ret = ResolveSymbolLazily<TableItem>(
          mplInfo, isSuper, dstSlice, index, fromUpper, fromUndef, tableItem, tableIndex, addr);
    } else {
      __atomic_store_n(&tableItem.index, reinterpret_cast<size_t>(isSuper ? linkerutils::GetDefTableAddress(
          mplInfo, defSlice, index, false) : defSlice[index].Address()), __ATOMIC_RELEASE);
    }
  } else {
    return isSuper ? true : ((tableIndex != 0) ?  true : false);
  }
  return ret;
}
} // namespace maplert
#endif // MAPLE_RUNTIME_MPL_LINKER_INLINE_H_
