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
#ifndef MAPLE_RUNTIME_MPL_LINKER_LAZY_H_
#define MAPLE_RUNTIME_MPL_LINKER_LAZY_H_

#include "linker_method_builder.h"

#ifdef __cplusplus
namespace maplert {
class LazyBinding : public FeatureModel {
 public:
  static FeatureName featureName;
  LazyBinding(LinkerInvoker &invoker, MethodBuilder &build) : pInvoker(&invoker), pMethodBuilder(&build) {};
  ~LazyBinding() {
    pInvoker = nullptr;
    pMethodBuilder = nullptr;
  }
  void *SetClassInDefAddrTable(size_t index, const MClass *klass);
  inline void *SetAddrInAddrTable(AddrSlice &addrSlice, size_t index, const MClass *addr);
  BindingState GetAddrBindingState(LinkerVoidType addr);
  BindingState GetAddrBindingState(const AddrSlice &addrSlice, size_t index, bool isAtomic = true);
  BindingState GetAddrBindingState(const LinkerMFileInfo &mplInfo, const AddrSlice &addrSlice, const void *offset);
  void LinkClass(MClass *klass);
  MClass *LinkDataUndefSuperClassAndInterfaces(LinkerMFileInfo &mplInfo, MObject *candidateClassLoader, size_t index,
      AddrSlice dataUndefSlice, MClass *superClassesItem, IndexSlice superTableSlice, uint32_t i, bool fromUndef);
  MClass *LinkDataDefSuperClassAndInterfaces(LinkerMFileInfo &mplInfo, size_t index, AddrSlice dataDefSlice,
      MClass *superClassesItem, IndexSlice superTableSlice, uint32_t i, bool fromUndef);
  void LinkSuperClassAndInterfaces(
      const MClass *klass, MObject *candidateClassLoader, bool recursive = true, bool forCold = false);

  void GetAddrAndMuidSlice(LinkerMFileInfo &mplInfo,
      BindingState state, AddrSlice &addrSlice, MuidSlice &muidSlice, const void *offset);
  void *ResolveClassSymbolClassification(
      LinkerMFileInfo &mplInfo, BindingState state, const AddrSlice &addrSlice, const MuidSlice &muidSlice,
      const size_t &index, MObject *candidateClassLoader, bool &fromUpper, const void *pc, const void *offset);
  bool HandleSymbol(const void *offset, const void *pc, BindingState state, bool fromSignal);
  bool HandleSymbolForDecoupling(LinkerMFileInfo &mplInfo, const AddrSlice &addrSlice, size_t index);
  bool HandleSymbol(
      LinkerMFileInfo &mplInfo, const void *offset, const void *pc, BindingState state, bool fromSignal);

  size_t GetAddrIndex(const LinkerMFileInfo &mplInfo, const AddrSlice &addrSlice, const void *offset);
  void *SearchInUndef(LinkerMFileInfo &mplInfo, const AddrSlice &addrSlice, const MuidSlice &muidSlice,
      size_t index, MObject *candidateClassLoader, bool &fromUpper, bool isDef, std::string className);
  void *ResolveClassSymbolForAPK(LinkerMFileInfo &mplInfo, size_t index, bool &fromUpper, bool isDef);
  void *ResolveClassSymbol(LinkerMFileInfo &mplInfo, const AddrSlice &addrSlice, const MuidSlice &muidSlice,
      size_t index, MObject *candidateClassLoader, bool &fromUpper, bool isDef, bool clinit = false);
  void *ResolveClassSymbolInternal(LinkerMFileInfo &mplInfo, const AddrSlice &addrSlice, const MuidSlice &muidSlice,
      size_t index, MObject *classLoader, bool &fromUpper, bool isDef, const std::string &className);
  void *ResolveDataSymbol(const AddrSlice &addrSlice, const MuidSlice &muidSlice, size_t index, bool isDef);
  void *ResolveDataSymbol(LinkerMFileInfo &mplInfo, const AddrSlice &addrSlice,
      const MuidSlice &muidSlice, size_t index, bool isDef);
  void *ResolveMethodSymbol(const AddrSlice &addrSlice, const MuidSlice &muidSlice, size_t index, bool isDef);
  void *ResolveMethodSymbol(LinkerMFileInfo &mplInfo, const AddrSlice &addrSlice, const MuidSlice &muidSlice,
      size_t index, bool isDef, bool forClass = false);
  MClass *GetUnresolvedClass(MClass *klass, bool &fromUpper, bool isDef);
  void ResolveMethodsClassSymbol(const LinkerMFileInfo &mplInfo, bool isDef, const void *addr);
  void LinkStaticSymbol(LinkerMFileInfo &mplInfo, const MClass *target);
  void LinkStaticMethod(LinkerMFileInfo &mplInfo, const MClass *target);
  void LinkStaticField(LinkerMFileInfo &mplInfo, const MClass *target);
  void LinkStaticMethodSlow(MethodMeta &meta, std::string name, LinkerMFileInfo &mplInfo,
      LinkerMFileInfo &oldLinkerMFileInfo, AddrSlice &srcTable, AddrSlice &dstTable);
  void LinkStaticMethodFast(MethodMeta &meta, const std::string name, size_t index, LinkerMFileInfo &mplInfo,
      const LinkerMFileInfo &oldLinkerMFileInfo, AddrSlice &srcTable, AddrSlice &dstTable);
  void LinkStaticFieldSlow(FieldMeta &meta, std::string name, LinkerMFileInfo &mplInfo,
      LinkerMFileInfo &oldLinkerMFileInfo, AddrSlice &srcTable, AddrSlice &dstTable);
  void LinkStaticFieldFast(FieldMeta &meta, const std::string name, size_t index, LinkerMFileInfo &mplInfo,
      const LinkerMFileInfo &oldLinkerMFileInfo, AddrSlice &srcTable, AddrSlice &dstTable);
  void LinkClassInternal(MClass *klass, MObject *candidateClassLoader);
  void LinkClassInternal(LinkerMFileInfo &mplInfo, MClass *klass, MObject *candidateClassLoader, bool first = true);
  void LinkSuperClassAndInterfaces(LinkerMFileInfo &mplInfo, const MClass *klass, MObject *candidateClassLoader,
      bool recursive = true, bool forCold = false);
  void LinkMethods(LinkerMFileInfo &mplInfo, MClass *klass, bool first, MetaRef index);
  inline void LinkFields(const LinkerMFileInfo &mplInfo, MClass *klass);
  void *GetClassMetadata(LinkerMFileInfo &mplInfo, size_t classIndex);
  void *LookUpDataSymbolAddressLazily(const MObject *classLoader, const MUID &muid,
      LinkerMFileInfo **outLinkerMFileInfo = nullptr, bool ignoreClassLoader = false);
  void *LookUpMethodSymbolAddressLazily(const MObject *classLoader, const MUID &muid, bool ignoreClassLoader = false);
  inline std::string ResolveClassMethod(MClass *klass);

  void PreLinkLazyMethod(LinkerMFileInfo &mplInfo);
#if defined(LINKER_RT_CACHE) && defined(LINKER_RT_LAZY_CACHE)
  bool LoadLazyCache(LinkerMFileInfo &mplInfo, std::string &cachingIndex);
  bool LoadLazyCacheInternal(
      LinkerMFileInfo &mplInfo, std::string &path, std::string &cachingIndex,
      char *buf, std::streamsize &index, std::streamsize &byteCount);
  bool SaveLazyCache(LinkerMFileInfo &mplInfo, const std::string &cachingIndex);
  bool SaveLazyCacheInternal(std::string &path, int &fd, std::vector<char> &bufVector);
#endif // defined(LINKER_RT_CACHE) && defined(LINKER_RT_LAZY_CACHE)
 private:
  LinkerInvoker *pInvoker = nullptr;
  MethodBuilder *pMethodBuilder = nullptr;
};
extern "C" {
#endif // __cplusplus
#ifdef __cplusplus
} // extern "C"
} // namespace maplert
#endif // __cplusplus
#endif // MAPLE_RUNTIME_MPL_LINKER_LAZY_H_
