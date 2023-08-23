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
#ifndef MAPLE_RUNTIME_MPL_LINKER_H_
#define MAPLE_RUNTIME_MPL_LINKER_H_

#include "linker_model.h"

#ifdef __cplusplus

namespace maplert {
// Maple linker tools.
class Linker : public FeatureModel {
 public:
  static FeatureName featureName;
  explicit Linker(LinkerInvoker &invoker) : pInvoker(&invoker) {};
  ~Linker() {
    pInvoker = nullptr;
  }
  bool HandleSymbol();
  bool HandleMethodSymbol();
  bool HandleDataSymbol();
  bool HandleSymbol(LinkerMFileInfo &mplInfo);
  bool HandleMethodSymbol(LinkerMFileInfo &mplInfo);
  bool HandleDataSymbol(LinkerMFileInfo &mplInfo);
  bool ResolveMethodSymbol(LinkerMFileInfo &mplInfo);
  bool RelocateMethodSymbol(LinkerMFileInfo &mplInfo);
  void InitMethodSymbol(LinkerMFileInfo &mplInfo);
  bool ResolveVTableSymbol(LinkerMFileInfo &mplInfo);
  bool ResolveITableSymbol(LinkerMFileInfo &mplInfo);
  bool ResolveDataSymbol(LinkerMFileInfo &mplInfo);

  bool ResolveSuperClassSymbol(LinkerMFileInfo &mplInfo);
  bool ResolveGCRootSymbol(LinkerMFileInfo &mplInfo);
  bool RelocateDataSymbol(LinkerMFileInfo &mplInfo);
  void InitDataSymbol(LinkerMFileInfo &mplInfo);

  void *LookUpSymbolAddress(const void *handle, const MUID &muid);
  void *LookUpSymbolAddress(const MUID &muid);
  void *LookUpSymbolAddress(LinkerMFileInfo &mplInfo, const MUID &muid);

  bool InitLinkerMFileInfo(LinkerMFileInfo &mplInfo, int32_t pos = -1);
  void FreeAllCacheTables(const MObject *classLoader) const;

  void AdjustDefTableAddress(const LinkerMFileInfo &mplInfo, LinkerAddrTableItem &pTable, size_t index);
  template<typename T1, typename T2>
  int64_t BinarySearch(T1 &pTable, int64_t start, int64_t end, const T2 &value);
  template<typename T1, typename T2, typename T3>
  int64_t BinarySearch(T1 &pTable, const T2 &pMuidIndexTable, int64_t start, int64_t end, const T3 &value);

 private:
  void InitLinkerMFileInfoIgnoreSysCL(LinkerMFileInfo &mplInfo);
  void InitLinkerMFileInfoTableAddr(LinkerMFileInfo &mplInfo) const;
  bool HandleMethodSymbolNoLazy(LinkerMFileInfo &mplInfo);
  bool HandleDataSymbolNoLazy(LinkerMFileInfo &mplInfo);
  bool DoResolveSuperClassSymbol(LinkerMFileInfo &mplInfo, IndexSlice &superTableSlice,
      const AddrSlice &dataUndefSlice, const AddrSlice &dataDefSlice, size_t i);
  bool ResolveUndefVTableSymbol(
      LinkerMFileInfo &mplInfo, bool fromUndef, size_t index, VTableSlice &vTableSlice, size_t i);
  bool ResolveUndefITableSymbol(
      LinkerMFileInfo &mplInfo, bool fromUndef, size_t index, ITableSlice &iTableSlice, size_t i);

#if defined(LINKER_LAZY_BINDING) && defined(LINKER_32BIT_REF_FOR_DEF_UNDEF)
  void InitMethodSymbolLazy32(LinkerMFileInfo &mplInfo, size_t defSize);
  void InitDataSymbolLazy32(LinkerMFileInfo &mplInfo, size_t defSize);
#endif // LINKER_32BIT_REF_FOR_DEF_UNDEF LINKER_LAZY_BINDING

  bool methodHasNotResolved = false;
  bool dataHasNotResolved = false;
  bool methodHasNotRelocated = false;
  bool dataHasNotRelocated = false;
  bool vtableHasNotResolved = false;
  bool itableHasNotResolved = false;
  bool superClassHasNotResolved = false;
  bool gcRootListHasNotResolved = false;
  LinkerInvoker *pInvoker = nullptr;
};
} // namespace maplert
#endif // __cplusplus
#endif  // MAPLE_RUNTIME_MPL_LINKER_H_
