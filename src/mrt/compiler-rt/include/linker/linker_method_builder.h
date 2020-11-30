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
#ifndef MAPLE_RUNTIME_MPL_LINKER_METHOD_H_
#define MAPLE_RUNTIME_MPL_LINKER_METHOD_H_

#include "linker_model.h"
#include "methodmeta.h"

namespace maplert {
constexpr uint16_t kAdjacencyInvalidValue = static_cast<uint16_t>(-1);

struct MethodItem {
  LinkerVoidType methodMeta; // MethodMetaBase*
  uint32_t hash;
#ifdef LINKER_RT_LAZY_CACHE
  std::vector<uint16_t> depth;
  uint16_t index;
#endif // LINKER_RT_LAZY_CACHE
  bool isInterface;

 public:
  void SetMethod(const MethodMetaBase &method) {
    methodMeta = static_cast<LinkerVoidType>(reinterpret_cast<uintptr_t>(&method));
  }

  MethodMetaBase *GetMethod() const {
    return reinterpret_cast<MethodMetaBase*>(static_cast<uintptr_t>(methodMeta));
  }
};

struct AdjItem {
  uint16_t first;
  uint16_t next;
  LinkerVoidType methodMeta; // MethodMetaBase*

 public:
  AdjItem(uint16_t firstVal, uint16_t nextVal, uint16_t methodVal)
      : first(firstVal), next(nextVal), methodMeta(methodVal) {};
  AdjItem() : first(kAdjacencyInvalidValue), next(kAdjacencyInvalidValue), methodMeta(0) {};

  void SetMethod(const MethodMetaBase &method) {
    methodMeta = static_cast<LinkerVoidType>(reinterpret_cast<uintptr_t>(&method));
  }

  MethodMetaBase *GetMethod() const {
    return reinterpret_cast<MethodMetaBase*>(static_cast<uintptr_t>(methodMeta));
  }
};

// Map from class object to its vtab index mapping.
using MplLazyBindingVTableMapT = std::vector<AdjItem>;
class MethodBuilder : public FeatureModel {
 public:
  static FeatureName featureName;
  explicit MethodBuilder(LinkerInvoker &invoker) : pInvoker(&invoker) {};
  ~MethodBuilder() {
    pInvoker = nullptr;
  }
  int32_t UpdateOffsetTableByVTable(
      const MClass *klass, MplLazyBindingVTableMapT &adjMethods,
      LinkerVTableOffsetItem &vtableOffsetItem, LinkerOffsetValItem &offsetTableItem);
  int32_t UpdateOffsetTableByVTable(
      const MClass *klass, MplLazyBindingVTableMapT &adjMethods,
      LinkerVTableOffsetItem &vtableOffsetItem, LinkerOffsetValItemLazyLoad &offsetTableItem);
#ifdef LINKER_RT_LAZY_CACHE
  void BuildMethodByCachingIndex(MClass *klass, const std::string &cachingIndex);
#endif // LINKER_RT_LAZY_CACHE
  std::string BuildMethod(MClass *klass);
  MplLazyBindingVTableMapT GetMethodVTableMap(MClass *klass);

 private:
  unsigned int GetHashIndex(const char *name);
  inline uint16_t GetMethodMetaHash(const MethodMetaBase &method);
  std::string GetMethodFullName(const MethodMetaBase &method);
  inline std::string GetMethodSignature(const MethodMetaBase &method) const;
  bool EqualMethod(const MethodMetaBase &method1, const MethodMetaBase &method2);
  bool EqualMethod(const MethodMetaBase *method, const char *methodName, const char *methodSignature);
  bool CanAccess(const MethodMetaBase &baseMethod, const MethodMetaBase &currentMethod);
  bool CheckMethodMeta(const MClass *klass, std::vector<std::pair<MethodMetaBase*, uint16_t>> &methods);
  bool CheckMethodMetaNoSort(const MClass *klass, std::vector<std::pair<MethodMetaBase*, uint16_t>> &methods);
  void OverrideVTabMethod(std::vector<MethodItem> &virtualMethods, uint16_t vtabIndex,
      const MethodMetaBase &method, const std::vector<uint16_t> &depth, uint16_t index, bool isInterface);
  void AppendVTabMethod(std::vector<MethodItem> &virtualMethods, int16_t vtabIndex, const MethodMetaBase &method,
      const std::vector<uint16_t> &depth, uint16_t index, bool isInterface, uint16_t hash);
  void CollectClassMethodsFast(const MClass *klass, const std::vector<uint16_t> &depth,
      std::vector<MethodItem> &virtualMethods, const std::vector<std::pair<MethodMetaBase*, uint16_t>> &methods);
  void CollectClassMethodsSlow(const MClass *klass, const std::vector<uint16_t> &depth,
      std::vector<MethodItem> &virtualMethods, const MplLazyBindingVTableMapT &adjMethods,
      const std::vector<std::pair<MethodMetaBase*, uint16_t>> &methods);
  void CollectClassMethodsRecursive(MClass *klass, bool &isDecouple, std::set<MClass*> &checkedClasses,
      std::vector<uint16_t> &depth, uint16_t superNum,
      std::vector<MethodItem> &virtualMethods, MplLazyBindingVTableMapT &adjMethods);
  void CollectClassMethods(const MClass *klass, const bool &isDecouple, const std::vector<uint16_t> &depth,
      std::vector<MethodItem> &virtualMethods, MplLazyBindingVTableMapT &adjMethods);
  void BuildAdjMethodList(std::vector<MethodItem> &virtualMethods, MplLazyBindingVTableMapT &adjMethods);
  void GenerateAndAttachClassVTable(MClass *klass, std::vector<MethodItem> &virtualMethods);
  uint32_t ProcessITableFirstTable(std::vector<MethodItem> &virtualMethods,
      std::vector<MethodMetaBase*> &firstITabVector, std::vector<MethodMetaBase*> &firstITabConflictVector,
      uint32_t &maxFirstITabIndex);
  size_t ProcessITableSecondTable(std::vector<MethodMetaBase*> &firstITabConflictVector,
      std::map<uint32_t, MethodMetaBase*> &secondITableMap, std::vector<MethodMetaBase*> &secondITabConflictVector);
  inline void GenerateITableFirstTable(LinkerVoidType &itab, const std::vector<MethodMetaBase*> &firstITabVector,
      const std::vector<MethodMetaBase*> &firstITabConflictVector, uint32_t maxFirstITabIndex) const;
  void GenerateITableSecondTable(LinkerVoidType &itab, std::map<uint32_t, MethodMetaBase*> &secondITableMap,
      std::vector<MethodMetaBase*> &secondITabConflictVector, size_t sizeOfITabNoNames);
  void GenerateAndAttachClassITable(MClass *klass, std::vector<MethodItem> &virtualMethods);
  inline void BuildMethodVTableITable(MClass *klass, std::vector<MethodItem> &virtualMethods);
  inline bool IsValidMethod(const MethodMetaBase &method) const;
  int32_t GetVTableItemIndex(
      const MClass *klass, MplLazyBindingVTableMapT &adjMethods, const char *methodName, const char *methodSignature);
#ifdef LINKER_RT_LAZY_CACHE
  MethodMetaBase *GetMethodByIndex(MClass *klass, uint16_t index);
  std::string GetMethodCachingIndexString(MClass *klass, std::vector<MethodItem> &virtualMethods);
#endif // LINKER_RT_LAZY_CACHE
  LinkerInvoker *pInvoker = nullptr;
};
} // namespace maplert
#endif // MAPLE_RUNTIME_MPL_LINKER_METHOD_H_
