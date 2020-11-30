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
#include "linker/linker_method_builder.h"

#include <algorithm>

#include "linker/linker_model.h"
#include "itab_util.h"
#include "cinterface.h"
#include "mrt_object.h"
#include "mclass_inline.h"
#include "mrt_reflection_class.h"
#include "modifier.h"
#include "mrt_reflection_method.h"
#include "mrt_class_api.h"

namespace maplert {
using namespace linkerutils;
FeatureName MethodBuilder::featureName = kFMethodBuilder;
inline unsigned int MethodBuilder::GetHashIndex(const char *name) {
  unsigned int hashcode = maple::DJBHash(name);
  return (hashcode % maple::kHashSize);
}

inline bool MethodBuilder::IsValidMethod(const MethodMetaBase &method) const {
  uint32_t methodFlag = method.GetFlag();
  uint32_t methodModifier = method.GetMod();
  return !modifier::IsNotvirtualMethod(methodFlag) &&
      !modifier::IsStatic(methodModifier) && !modifier::IsPrivate(methodModifier);
}

// Check the method and sort them with vtab index,
// then check if it's compact.
inline bool MethodBuilder::CheckMethodMetaNoSort(
    const MClass *klass, std::vector<std::pair<MethodMetaBase*, uint16_t>> &methods) {
  uint32_t methodNum = klass->GetNumOfMethods();
  MethodMeta *methodMetas = klass->GetRawMethodMetas();
  if (UNLIKELY(methodNum == 0 || methodMetas == nullptr)) {
    return false;
  }
  bool isCompactMeta = reinterpret_cast<const MClass*>(klass)->IsCompactMetaMethods();

  // We should create the method metadata list sorted by its index in default,
  // then we can avoid sorting them once here.
  bool isInterface = klass->IsInterface();
  for (uint32_t i = 0; i < methodNum; ++i) {
    MethodMetaBase *method = nullptr;
    if (!isCompactMeta) {
      method = reinterpret_cast<MethodMetaBase*>(&methodMetas[i]);
    } else {
      MethodMetaCompact *compact = MethodMetaCompact::GetMethodMetaCompact(*klass, i);
      method = reinterpret_cast<MethodMetaBase*>(compact);
    }
    if (!isInterface && !IsValidMethod(*method)) {
      continue;
    }
    methods.push_back(std::make_pair(method, i)); // Push back the method and its position.
  }
  return true;
}

inline bool MethodBuilder::CheckMethodMeta(
    const MClass *klass, std::vector<std::pair<MethodMetaBase*, uint16_t>> &methods) {
  bool res = CheckMethodMetaNoSort(klass, methods);
  std::sort(methods.begin(), methods.end(), [](auto &lhs, auto &rhs) {
    return lhs.first->GetVtabIndex() < rhs.first->GetVtabIndex();
  });
  return res;
}

inline void MethodBuilder::BuildAdjMethodList(
    std::vector<MethodItem> &virtualMethods, MplLazyBindingVTableMapT &adjMethods) {
  uint16_t size = virtualMethods.size();
  if (UNLIKELY(size == 0)) {
    return;
  }
  adjMethods.resize(size);
  adjMethods.assign(size, AdjItem());
  for (uint16_t i = 0; i < size; ++i) { // Travel across the methods one by one.
    uint16_t hash = virtualMethods[i].hash;
    uint16_t pos = hash % size;
    adjMethods[i].next = adjMethods[pos].first;
    adjMethods[pos].first = i;
    adjMethods[i].methodMeta = virtualMethods[i].methodMeta;
  }
}

void MethodBuilder::CollectClassMethods(const MClass *klass, const bool &isDecouple, const std::vector<uint16_t> &depth,
    std::vector<MethodItem> &virtualMethods, MplLazyBindingVTableMapT &adjMethods) {
  bool isInterface = klass->IsInterface();
  if (isDecouple || isInterface) {
    std::vector<std::pair<MethodMetaBase*, uint16_t>> methods;
    if (!CheckMethodMeta(klass, methods)) {
      return;
    }
    CollectClassMethodsSlow(klass, depth, virtualMethods, adjMethods, methods);
  } else {
    std::vector<std::pair<MethodMetaBase*, uint16_t>> methods;
    if (!CheckMethodMetaNoSort(klass, methods)) {
      return;
    }
    CollectClassMethodsFast(klass, depth, virtualMethods, methods);
  }

  // If we want to save the mapping info. in memory, so we shouldn't check klass != targetClass.
  BuildAdjMethodList(virtualMethods, adjMethods);
}

void MethodBuilder::OverrideVTabMethod(std::vector<MethodItem> &virtualMethods, uint16_t vtabIndex,
    const MethodMetaBase &method, const std::vector<uint16_t> &depth, uint16_t index, bool isInterface) {
  virtualMethods[vtabIndex].SetMethod(method);
#ifdef LINKER_RT_LAZY_CACHE
  virtualMethods[vtabIndex].depth = (depth.size() > 0) ? depth : virtualMethods[vtabIndex].depth;
  virtualMethods[vtabIndex].index = index;
#endif // LINKER_RT_LAZY_CACHE
  virtualMethods[vtabIndex].isInterface = isInterface || virtualMethods[vtabIndex].isInterface;
  LINKER_DLOG(lazybinding) << "method=" << GetMethodFullName(method) << ", vtabIndex=" << vtabIndex <<
      ", depth size=" << depth.size() << ", index" << index <<
      ", isInterface=" << virtualMethods[vtabIndex].isInterface << maple::endl;
}

void MethodBuilder::AppendVTabMethod(std::vector<MethodItem> &virtualMethods, int16_t vtabIndex,
    const MethodMetaBase &method, const std::vector<uint16_t> &depth, uint16_t index, bool isInterface,
    uint16_t hash) {
  MethodItem methodItem;
  methodItem.SetMethod(method);
#ifdef LINKER_RT_LAZY_CACHE
  methodItem.depth = (depth.size() > 0) ? depth : methodItem.depth;
  methodItem.index = index;
#endif // LINKER_RT_LAZY_CACHE
  methodItem.isInterface = isInterface;
  methodItem.hash = hash;
  if (vtabIndex < 0) {
    virtualMethods.push_back(std::move(methodItem));
  } else {
    virtualMethods[vtabIndex] = std::move(methodItem);
  }
  LINKER_DLOG(lazybinding) << "method=" << GetMethodFullName(method) << ", depth size=" << depth.size() <<
      ", index=" << index << ", isInterface=" << isInterface << ", hash=" << hash << maple::endl;
}

void MethodBuilder::CollectClassMethodsSlow(const MClass *klass, const std::vector<uint16_t> &depth,
    std::vector<MethodItem> &virtualMethods, const MplLazyBindingVTableMapT &adjMethods,
    const std::vector<std::pair<MethodMetaBase*, uint16_t>> &methods) {
  bool isInterface = reinterpret_cast<const MClass*>(klass)->IsInterface();
  for (size_t i = 0; i < methods.size(); ++i) { // Travel across the methods one by one.
    auto methodPair = methods[i];
    MethodMetaBase *method = methodPair.first;
    uint16_t index = methodPair.second;
    uint16_t hash = GetMethodMetaHash(*method);
    uint16_t k = kAdjacencyInvalidValue;
    size_t size = adjMethods.size();
    if (LIKELY(size > 0)) {
      for (k = adjMethods[hash % size].first; k != kAdjacencyInvalidValue; k = adjMethods[k].next) {
        if (EqualMethod(*(virtualMethods[k].GetMethod()), *method)) { // Found.
          break;
        }
      }
    }
    LINKER_VLOG(lazybinding) << "method=" << GetMethodFullName(*method) << ", vtabIndex=" << method->GetVtabIndex() <<
        "->" << k << ", depth/index=" << depth.size() << "/" << index << ", interface=" << isInterface <<
        ", isAbstract=" << modifier::IsAbstract(method->GetMod()) << ", ofClass=" << klass->GetName() << ", hash=" <<
        hash << ", flag=" << method->GetFlag() << ", modifier=" << method->GetMod() << maple::endl;
    if (k != kAdjacencyInvalidValue) { // Found, 'k' is the position.
      // Class, and not abstract. Should we check CanAccess for class type.
      // or Interface, and old method is also interface.
      bool isAbstract = modifier::IsAbstract(method->GetMod());
      bool canAccess = CanAccess(*(virtualMethods[k].GetMethod()), *method);
      MClass *virtualClass = virtualMethods[k].GetMethod()->GetDeclaringClass();
      if (virtualClass == nullptr) {
        LINKER_LOG(FATAL) << "virtualClass is nullptr" << maple::endl;
      }
      if (!isAbstract && ((!isInterface && canAccess) || (isInterface && virtualClass->IsInterface()))) {
        OverrideVTabMethod(virtualMethods, k, *method, depth, index, isInterface);
      } else if (!isAbstract && !isInterface && !canAccess) {
        // -1 means invalid vtab index, push_back directly.
        AppendVTabMethod(virtualMethods, -1, *method, depth, index, isInterface, hash);
      } else {
        virtualMethods[k].isInterface = isInterface || virtualMethods[k].isInterface;
      }
    } else { // Not found.
      // -1 means invalid vtab index, push_back directly.
      AppendVTabMethod(virtualMethods, -1, *method, depth, index, isInterface, hash);
    }
  }
}

// Only for no decoupled classes.
void MethodBuilder::CollectClassMethodsFast(const MClass *klass, const std::vector<uint16_t> &depth,
    std::vector<MethodItem> &virtualMethods, const std::vector<std::pair<MethodMetaBase*, uint16_t>> &methods) {
  bool isInterface = klass->IsInterface();
  size_t methodNum = methods.size();
  size_t size = virtualMethods.size();
  virtualMethods.resize(size + methodNum);
  int16_t ignoredNum = 0;
  for (size_t i = 0; i < methodNum; ++i) { // Travel across the methods one by one.
    auto methodPair = methods[i];
    MethodMetaBase *method = methodPair.first;
    uint16_t index = methodPair.second;
    int16_t vtabIndex = method->GetVtabIndex();
    uint16_t hash = GetMethodMetaHash(*method);
    if (vtabIndex >= 0 && !isInterface) { // Class.
      if (UNLIKELY(static_cast<size_t>(vtabIndex) < size && virtualMethods[vtabIndex].GetMethod() == nullptr)) {
        LINKER_LOG(ERROR) << "null item" << ", method=" << GetMethodFullName(*method) << ", depth/index=" <<
            depth.size() << "/" << index << ", hash=" << hash << "/" << virtualMethods[vtabIndex].hash <<
            ", vtabIndex=" << vtabIndex << "/" << size << ", isAbstract=" << modifier::IsAbstract(method->GetMod()) <<
            ", isInterface=" << isInterface << ", ofClass=" << klass->GetName() << maple::endl;
      }
      if (static_cast<size_t>(vtabIndex) < size &&
          (virtualMethods[vtabIndex].GetMethod() == nullptr ||
          CanAccess(*(virtualMethods[vtabIndex].GetMethod()), *method))) {
        OverrideVTabMethod(virtualMethods, vtabIndex, *method, depth, index, isInterface);
        ignoredNum++;
        continue;
      } else {
        AppendVTabMethod(virtualMethods, vtabIndex, *method, depth, index, isInterface, hash);
        continue;
      }
    }

    LINKER_LOG(FATAL) << "negative vtab index, or interface, " << ", method=" << GetMethodFullName(*method) <<
        ", hash=" << hash << ", vtabIndex=" << method->GetVtabIndex() << "->(" << size << "-" << ignoredNum <<
        "), ofClass=" << klass->GetName() << maple::endl;
  }
  virtualMethods.resize(virtualMethods.size() - ignoredNum);
}

void MethodBuilder::CollectClassMethodsRecursive(MClass *klass, bool &isDecouple, std::set<MClass*> &checkedClasses,
    std::vector<uint16_t> &depth, uint16_t superNum,
    std::vector<MethodItem> &virtualMethods, MplLazyBindingVTableMapT &adjMethods) {
  if (klass->IsColdClass() && !klass->IsLazyBinding()) {
    LINKER_VLOG(lazybinding) << "cold class name=" << klass->GetName() << maple::endl;
    pInvoker->ResolveColdClassSymbol(reinterpret_cast<jclass>(klass));
  }

#ifdef LINKER_RT_LAZY_CACHE
  // superNum == 0xFFFF, means not to record depth.
  if (superNum != static_cast<uint16_t>(-1)) {
    depth.push_back(superNum);
  }
#endif // LINKER_RT_LAZY_CACHE

  uint32_t superClassNum = klass->GetNumOfSuperClasses();
  MClass **superClassArray = klass->GetSuperClassArrayPtr();
  if (superClassNum != 0) {
    // Check the supers of 'klass'.
    for (uint32_t i = 0; i < superClassNum; ++i) {
      MClass *superClass = superClassArray[i];
      uint16_t num = 0;
#ifdef LINKER_RT_LAZY_CACHE
      if (superNum != static_cast<uint16_t>(-1)) {
        num = i;
      } else {
        num = static_cast<uint16_t>(-1);
      }
#endif // LINKER_RT_LAZY_CACHE
      CollectClassMethodsRecursive(superClass, isDecouple, checkedClasses, depth, num, virtualMethods, adjMethods);
    }
  }

  // Check the 'klass'.
  LINKER_VLOG(lazybinding) << "class=" << klass->GetName() << ", " << superNum << maple::endl;
  if (!isDecouple) {
    // Mark the class as not decoupled when its only superclass is object
    isDecouple = (klass->IsDecouple() && (superClassNum != 1 || *superClassArray[0] != MRT_GetClassObject()));
  }
  bool isInterface = klass->IsInterface();
  if (LIKELY(!isInterface || checkedClasses.count(klass) == 0)) {
    CollectClassMethods(klass, isDecouple, depth, virtualMethods, adjMethods);
  }
  if (isInterface) {
    checkedClasses.insert(klass);
  }

#ifdef LINKER_RT_LAZY_CACHE
  if (superNum != static_cast<uint16_t>(-1)) {
    depth.erase(depth.end() - 1);
  }
#endif // LINKER_RT_LAZY_CACHE
}

void MethodBuilder::GenerateAndAttachClassVTable(MClass *klass, std::vector<MethodItem> &virtualMethods) {
  if (UNLIKELY(virtualMethods.size() == 0)) {
    return;
  }

  LinkerVoidType *vtab = reinterpret_cast<LinkerVoidType*>(
      MRT_AllocFromDecouple(sizeof(LinkerVTableItem) * virtualMethods.size(), kVTabArray));
  if (UNLIKELY(vtab == nullptr)) {
    LINKER_LOG(FATAL) << "returns null" << maple::endl;
    return;
  }

  LinkerVTableItem *item = reinterpret_cast<LinkerVTableItem*>(vtab);
  for (uint32_t i = 0; i < virtualMethods.size(); ++i) {
    item->index = reinterpret_cast<size_t>(virtualMethods[i].GetMethod()->GetFuncAddress());
    item++;
  }
  reinterpret_cast<ClassMetadata*>(klass)->vTable.SetDataRef(vtab);
}

inline uint16_t MethodBuilder::GetMethodMetaHash(const MethodMetaBase &method) {
  uint16_t hash = kMethodHashMask;
  if (!method.IsMethodMetaCompact()) {
    hash = reinterpret_cast<MethodMeta*>(const_cast<MethodMetaBase*>(&method))->GetHashCode();
  }
  if (hash == kMethodHashMask) {
    hash = MClass::GetMethodFieldHash(method.GetName(), GetMethodSignature(method).c_str(), true);
  }
  return hash;
}

std::string MethodBuilder::GetMethodFullName(const MethodMetaBase &method) {
  const char *methodName = method.GetName();
  std::string name = methodName;
  name += '|';
  if (!method.IsMethodMetaCompact()) {
    name += reinterpret_cast<const MethodMeta*>(&method)->GetSignature();
  } else {
    std::string signature;
    method.GetSignature(signature);
    name += signature;
  }
  return name;
}

inline std::string MethodBuilder::GetMethodSignature(const MethodMetaBase &method) const {
  if (!method.IsMethodMetaCompact()) {
    return std::string(reinterpret_cast<const MethodMeta*>(&method)->GetSignature());
  } else {
    std::string signature;
    method.GetSignature(signature);
    return signature;
  }
}

// If the method name and signature are equal.
bool MethodBuilder::EqualMethod(const MethodMetaBase &method1, const MethodMetaBase &method2) {
  if (strcmp(method1.GetName(), method2.GetName()) != 0) {
    return false;
  }

  std::string signature1;
  const char *method1Signature;
  if (!method1.IsMethodMetaCompact()) {
    method1Signature = reinterpret_cast<const MethodMeta*>(&method1)->GetSignature();
  } else {
    method1.GetSignature(signature1);
    method1Signature = signature1.c_str();
  }
  std::string signature2;
  const char *method2Signature;
  if (!method2.IsMethodMetaCompact()) {
    method2Signature = reinterpret_cast<const MethodMeta*>(&method2)->GetSignature();
  } else {
    method2.GetSignature(signature2);
    method2Signature = signature2.c_str();
  }
  return strcmp(method1Signature, method2Signature) == 0;
}

inline bool MethodBuilder::EqualMethod(
    const MethodMetaBase *method, const char *methodName, const char *methodSignature) {
  if (strcmp(method->GetName(), methodName) != 0) {
    return false;
  }

  std::string tmp;
  const char *signature;
  if (!method->IsMethodMetaCompact()) {
    signature = reinterpret_cast<const MethodMeta*>(method)->GetSignature();
  } else {
    method->GetSignature(tmp);
    signature = tmp.c_str();
  }
  return strcmp(signature, methodSignature) == 0;
}

// Also see VtableAnalysis::CheckOverrideForCrossPackage at compiler side.
// Return true if virtual functions can be set override relationship.
// We just check if the base method is visibile for current method.
inline bool MethodBuilder::CanAccess(
    const MethodMetaBase &baseMethod, const MethodMetaBase &currentMethod) {
  // For the corss package inheritance, only if the base func is declared
  // as either 'public' or 'protected', we shall set override relationship.
  if (modifier::IsPublic(baseMethod.GetMod()) || modifier::IsProtected(baseMethod.GetMod())) {
    return true;
  }
  MClass *basePackageClass = baseMethod.GetDeclaringClass();
  MClass *currentPackageClass = currentMethod.GetDeclaringClass();
  if (basePackageClass == nullptr || currentPackageClass == nullptr) {
    LINKER_LOG(FATAL) << "class is null" << maple::endl;
  }
  char *basePackageName = basePackageClass->GetName();
  char *currentPackageName = currentPackageClass->GetName();
  char *basePos = strrchr(basePackageName, '/'); // Reverse find the end of package
  char *currentPos = strrchr(currentPackageName, '/'); // Reverse find the end of package
  uint32_t basePackageLen = (basePos == nullptr) ? 0 : static_cast<uint32_t>(basePos - basePackageName);
  uint32_t currentPackageLen = (currentPos == nullptr) ? 0 : static_cast<uint32_t>(currentPos - currentPackageName);
  if (basePackageLen == currentPackageLen && strncmp(basePackageName, currentPackageName, basePackageLen) == 0) {
    return true;
  }

  LINKER_VLOG(lazybinding) << "return false, method=" << GetMethodFullName(currentMethod) << ", baseClass=" <<
      basePackageName << ", currentClass=" << currentPackageName << maple::endl;
  return false;
}

// virtualMethods: In
// firstITabVector: Out
// firstITabConflictVector: Out
// maxFirstITabIndex: Out
inline uint32_t MethodBuilder::ProcessITableFirstTable(std::vector<MethodItem> &virtualMethods,
    std::vector<MethodMetaBase*> &firstITabVector, std::vector<MethodMetaBase*> &firstITabConflictVector,
    uint32_t &maxFirstITabIndex) {
  uint32_t num = 0;
  std::vector<bool> firstITabConflictFlagVector(maple::kItabFirstHashSize, false);
  for (size_t i = 0; i < virtualMethods.size(); ++i) {
    MethodItem methodItem = virtualMethods[i];
    if (!methodItem.isInterface) {
      continue;
    }
    ++num;

    // Should check if we can directly use GetMethodMetaHash, and mod maple::kHashSize.
    auto name = GetMethodFullName(*(methodItem.GetMethod()));
    uint32_t hash = GetHashIndex(name.c_str());
    if (hash > maxFirstITabIndex) {
      maxFirstITabIndex = hash;
    }
    if (firstITabVector[hash] == nullptr && !firstITabConflictFlagVector[hash]) { // First insertion.
      firstITabVector[hash] = methodItem.GetMethod();
    } else { // Conflict
      if (!firstITabConflictFlagVector[hash]) { // First conflict.
        firstITabConflictVector.push_back(firstITabVector[hash]);
        firstITabConflictVector.push_back(methodItem.GetMethod());
        firstITabVector[hash] = nullptr;
        firstITabConflictFlagVector[hash] = true;
      } else { // Successive conflicts.
        firstITabConflictVector.push_back(methodItem.GetMethod());
      }
    }
  }
  return num;
}

// firstITabConflictVector: In
// secondITableMap: Out
// secondITabConflictVector: Out
inline size_t MethodBuilder::ProcessITableSecondTable(std::vector<MethodMetaBase*> &firstITabConflictVector,
    std::map<uint32_t, MethodMetaBase*> &secondITableMap, std::vector<MethodMetaBase*> &secondITabConflictVector) {
  size_t sizeOfMethodsNames = 0;
  for (size_t i = 0; i < firstITabConflictVector.size(); ++i) { // Travel across the methods one by one.
    MethodMetaBase *method = firstITabConflictVector[i];
    auto name = GetMethodFullName(*method);
    sizeOfMethodsNames += name.size() + 1;
    uint32_t hash = maple::GetSecondHashIndex(name.c_str());
    if (secondITableMap.find(hash) == secondITableMap.end()) { // First insertion.
      secondITableMap[hash] = method;
    } else { // Conflict.
      if (secondITableMap[hash] != nullptr) { // First conflict.
        secondITabConflictVector.push_back(secondITableMap[hash]);
        secondITabConflictVector.push_back(method);
        secondITableMap[hash] = nullptr; // Set null as conflict flag temporarily.
      } else { // Successive conflicts.
        secondITabConflictVector.push_back(method);
      }
    }
  }
  return sizeOfMethodsNames;
}

inline void MethodBuilder::GenerateITableFirstTable(LinkerVoidType &itab,
    const std::vector<MethodMetaBase*> &firstITabVector,
    const std::vector<MethodMetaBase*> &firstITabConflictVector, uint32_t maxFirstITabIndex) const {
  // ITable's first table
  size_t firstITabVectorSize = firstITabConflictVector.empty() ? maxFirstITabIndex + 1 : firstITabVector.size();
  for (size_t i = 0; i < firstITabVectorSize; ++i) {
    auto method = firstITabVector[i];
    if (LIKELY(method != nullptr)) {
      (&itab)[i] = reinterpret_cast<uintptr_t>(method->GetFuncAddress());
    }
  }
}

inline void MethodBuilder::GenerateITableSecondTable(
    LinkerVoidType &itab, std::map<uint32_t, MethodMetaBase*> &secondITableMap,
    std::vector<MethodMetaBase*> &secondITabConflictVector, size_t sizeOfITabNoNames) {
  // kHeadSizeOfSecondHash:3
  // The last one points to the second level table
  (&itab)[maple::kHashSize] = reinterpret_cast<uintptr_t>(&itab + maple::kHashSize + 1);
  int pos = maple::kHashSize + 1;
  /// Hash table: size + align
#ifdef USE_32BIT_REF
  uint64_t shiftCountBit = 16; // 16 is 4 x 4
#else
  uint64_t shiftCountBit = 32; // 32 is 8 x 4
#endif
  (&itab)[pos++] = ((secondITabConflictVector.size() | (1UL << (shiftCountBit - 1))) << shiftCountBit) +
      secondITableMap.size();
  (&itab)[pos++] = maple::kFlagAgInHeadOfSecondHash;

  /// Hash table: hash + method address
  for (auto &item : secondITableMap) {
    auto method = item.second;
    (&itab)[pos++] = item.first;
    (&itab)[pos++] = method != nullptr ?
        reinterpret_cast<uintptr_t>(method->GetFuncAddress()) : maple::kFlagSecondHashConflict;
  }

  // Conflict table: method string + address
  char *methodsNames = reinterpret_cast<char*>(&itab + sizeOfITabNoNames);
  int namePos = 0;
  for (size_t i = 0; i < secondITabConflictVector.size(); i++) { // Travel across the methods one by one.
    auto method = secondITabConflictVector[i];
    // Copy the method's name firstly.
    std::string name = GetMethodFullName(*method);
    size_t len = name.size();
    if (UNLIKELY(strcpy_s(methodsNames + namePos, len + 1, name.c_str()) != EOK)) {
      free(&itab);
      LINKER_LOG(FATAL) << "strcpy_s() failed" << maple::endl;
    }
    (methodsNames + namePos)[len] = '\0';

    LINKER_DLOG(lazybinding) << "conflict name[" << pos << "][" << namePos << "]: " <<
        reinterpret_cast<uintptr_t>(method->GetFuncAddress()) << "/" << (methodsNames + namePos) << maple::endl;
    // Set the name and address.
    (&itab)[pos++] = reinterpret_cast<uintptr_t>(methodsNames + namePos);
    (&itab)[pos++] = reinterpret_cast<uintptr_t>(method->GetFuncAddress());

    // Next method's name offset.
    namePos += len + 1;
  }
}

void MethodBuilder::GenerateAndAttachClassITable(MClass *klass, std::vector<MethodItem> &virtualMethods) {
  if (UNLIKELY(klass->IsAbstract())) {
    return;
  }
  // Filter the methods for the first level table of itable.
  uint32_t maxFirstITabIndex = 0;
  std::vector<MethodMetaBase*> firstITabVector(maple::kItabFirstHashSize, nullptr);
  std::vector<MethodMetaBase*> firstITabConflictVector;
  if (ProcessITableFirstTable(
      virtualMethods, firstITabVector, firstITabConflictVector, maxFirstITabIndex) == 0) {
    reinterpret_cast<ClassMetadata*>(klass)->iTable.SetDataRef(nullptr);
    return;
  }
  // Collect the second level table of itable.
  std::map<uint32_t, MethodMetaBase*> secondITableMap;
  std::vector<MethodMetaBase*> secondITabConflictVector;
  size_t sizeOfMethodsNames = ProcessITableSecondTable(
      firstITabConflictVector, secondITableMap, secondITabConflictVector);
  size_t sizeOfITab = 0;
  size_t sizeOfITabNoNames = 0;
  if (firstITabConflictVector.empty()) { // No conflict in first level table.
    sizeOfITab += maxFirstITabIndex + 1;
    sizeOfITab = sizeOfITab * sizeof(LinkerVoidType);
  } else {
    sizeOfITabNoNames = maple::kHashSize + maple::kHeadSizeOfSecondHash +
        secondITableMap.size() * 2 + secondITabConflictVector.size() * 2; // 2 is to skit length of table
    sizeOfITab += sizeOfITabNoNames + sizeOfMethodsNames;
    sizeOfITab = sizeOfITab * sizeof(LinkerVoidType);
  }
  LINKER_DLOG(lazybinding) << "size of itab :" << sizeOfITab << maple::endl;
  LinkerVoidType *itab = reinterpret_cast<LinkerVoidType*>(MRT_AllocFromDecouple(sizeOfITab, kITabAggregate));
  if (UNLIKELY(itab == nullptr)) {
    LINKER_LOG(FATAL) << "MRT_NewPermObj returns null" << maple::endl;
    return;
  }
  if (UNLIKELY(memset_s(itab, sizeOfITab, 0, sizeOfITab) != EOK)) {
    LINKER_LOG(ERROR) << "memset_s() failed" << maple::endl;
    return;
  }
  GenerateITableFirstTable(*itab, firstITabVector, firstITabConflictVector, maxFirstITabIndex);
  if (firstITabConflictVector.empty()) { // No conflict in first level table.
    reinterpret_cast<ClassMetadata*>(klass)->iTable.SetDataRef(itab);
    return;
  }
  GenerateITableSecondTable(*itab, secondITableMap, secondITabConflictVector, sizeOfITabNoNames);
  reinterpret_cast<ClassMetadata*>(klass)->iTable.SetDataRef(itab);
}

inline int32_t MethodBuilder::GetVTableItemIndex(
    const MClass *klass, MplLazyBindingVTableMapT &adjMethods, const char *methodName, const char *methodSignature) {
  size_t size = adjMethods.size();
  if (LIKELY(size > 0)) {
    uint16_t hash = MClass::GetMethodFieldHash(methodName, methodSignature, true);
    uint16_t pos = hash % size;
    uint16_t k = kAdjacencyInvalidValue;
    for (k = adjMethods[pos].first; k != kAdjacencyInvalidValue; k = adjMethods[k].next) {
      MethodMetaBase *method = adjMethods[k].GetMethod();
      if (EqualMethod(method, methodName, methodSignature)) { // Found.
        return k;
      }
    }
  }
  LINKER_LOG(ERROR) << "failed to find method=" << methodName << "|" << methodSignature << ", " << size <<
      ", in class=" << klass->GetName() << maple::endl;
  return -1;
}

int32_t MethodBuilder::UpdateOffsetTableByVTable(const MClass *klass, MplLazyBindingVTableMapT &adjMethods,
    LinkerVTableOffsetItem &vtableOffsetItem, LinkerOffsetValItem &offsetTableItem) {
  DataRefOffset32 *dataMethodName = reinterpret_cast<DataRefOffset32*>(&vtableOffsetItem.methodName);
  const char *method = dataMethodName->GetDataRef<char*>();
  DataRefOffset32 *dataSignatureName = reinterpret_cast<DataRefOffset32*>(&vtableOffsetItem.signatureName);
  const char *signature = dataSignatureName->GetDataRef<char*>();
  int32_t index = GetVTableItemIndex(klass, adjMethods, method, signature);
  if (LIKELY(index >= 0)) {
    offsetTableItem.offset = index * sizeof(LinkerVTableItem);
  }
  return index;
}

int32_t MethodBuilder::UpdateOffsetTableByVTable(const MClass *klass, MplLazyBindingVTableMapT &adjMethods,
    LinkerVTableOffsetItem &vtableOffsetItem, LinkerOffsetValItemLazyLoad &offsetTableItem) {
  DataRefOffset32 *klassMethod = reinterpret_cast<DataRefOffset32*>(&vtableOffsetItem.methodName);
  const char *method = klassMethod->GetDataRef<char*>();
  DataRefOffset32 *klassSignature = reinterpret_cast<DataRefOffset32*>(&vtableOffsetItem.signatureName);
  const char *signature = klassSignature->GetDataRef<char*>();
  int32_t index = GetVTableItemIndex(klass, adjMethods, method, signature);
  if (LIKELY(index >= 0)) {
    offsetTableItem.offset = index * sizeof(LinkerVTableItem);
    offsetTableItem.offsetAddr = AddressToRefField(reinterpret_cast<uintptr_t>(&offsetTableItem.offset));
  }
  return index;
}

inline void MethodBuilder::BuildMethodVTableITable(MClass *klass, std::vector<MethodItem> &virtualMethods) {
  if (reinterpret_cast<ClassMetadata*>(klass)->vTable.refVal == 0) {
    GenerateAndAttachClassVTable(klass, virtualMethods);
  }
  if (reinterpret_cast<ClassMetadata*>(klass)->iTable.refVal == 0) {
    GenerateAndAttachClassITable(klass, virtualMethods);
  }
}

#ifdef LINKER_RT_LAZY_CACHE
MethodMetaBase* MethodBuilder::GetMethodByIndex(MClass *klass, uint16_t index) {
  uint32_t methodNum = klass->GetNumOfMethods();
  MethodMeta *methodMetas = klass->GetRawMethodMetas();
  if (UNLIKELY(methodNum == 0 || methodMetas == nullptr)) {
    LINKER_LOG(WARNING) << "invalid method, " << klass << "/" << klass->GetName() <<
        ", " << methodNum << ", " << methodMetas << maple::endl;
    return nullptr;
  }
  bool isCompactMeta = __MRT_IsCompactMeta(reinterpret_cast<uintptr_t>(methodMetas));
  if (!isCompactMeta) {
    method = reinterpret_cast<MethodMetaBase*>(&methodMetas[index]);
  } else {
    MethodMetaCompact *compact = MethodMetaCompact::GetMethodMetaCompact(*klass, index);
    method = reinterpret_cast<MethodMetaBase*>(compact);
  }
  return method;
}

void MethodBuilder::BuildMethodByCachingIndex(MClass *klass, const std::string &cachingIndex) {
  std::vector<MethodItem> virtualMethods;
  std::string::size_type startPos = 0;
  while (true) {
    std::string::size_type endPos = cachingIndex.find(',', startPos);
    if (endPos == std::string::npos) {
      break;
    }

    bool isInterface = false;
    std::string::size_type indexPos = cachingIndex.find('#', startPos);
    if (indexPos == std::string::npos || indexPos > endPos) {
      indexPos = cachingIndex.find('!', startPos);
      isInterface = true;
    }
    constexpr int decimalBase = 10;
    auto index = std::strtol(cachingIndex.c_str() + indexPos + 1, nullptr, decimalBase);

    // There're 3 type for depth info.:
    // 1. If the depth is not like 0/0/0/, for example 0/1/, we should record the whole depth info.
    // 2. If the depth is like 0/0/0/, we could only record the depth number(2).
    // 3. Further if the depth is 0, we could ignore it as default.
    std::vector<uint16_t> depth;
    char *depthStr == nullptr;
    auto tmp = std::strtol(cachingIndex.c_str() + startPos, &depthStr, decimalBase);
    if (tmp == 0 && depthStr == cachingIndex.c_str() + startPos) { // Type 3, needn't push anything.
    } else if (*depthStr != '/') { // Type 2.
      depth.resize(tmp, 0);
    } else { // Type 1.
      // Ignore the first depth.
      depthStr++;
      while (depthStr != cachingIndex.c_str() + indexPos) {
        tmp = std::strtol(depthStr, &depthStr, decimalBase);
        depth.push_back(tmp);
        depthStr++;
      }
    }

    MClass *superClass = klass;
    for (uint32_t i = 0; i < depth.size(); ++i) {
      uint16_t superNum = depth[i];
      MClass **superClassArray = klass->GetSuperClassArrayPtr();
      // We trust the caching info. and don't check __MRT_Class_getNumofSuperClasses() here.
      superClass = superClassArray[superNum];
    }

    auto method = GetMethodByIndex(superClass, index);
    uint16_t hash = GetMethodMetaHash(*method);
    MethodItem methodItem;
    methodItem.SetMethod(*method);
    methodItem.isInterface = isInterface;
    methodItem.hash = hash;
    virtualMethods.push_back(std::move(methodItem));
    startPos = endPos + 1;
  }

  BuildMethodVTableITable(klass, virtualMethods);
}

std::string MethodBuilder::GetMethodCachingIndexString(MClass *klass, std::vector<MethodItem> &virtualMethods) {
  std::string vtabIndexStr;
  for (uint32_t i = 0; i < virtualMethods.size(); ++i) {
    bool mainLine = true;
    std::string depthStr;
    for (uint32_t j = 0; j < virtualMethods[i].depth.size(); ++j) {
      if (virtualMethods[i].depth[j] != 0) {
        mainLine = false;
      }
      depthStr += std::to_string(virtualMethods[i].depth[j]);
      depthStr += '/';
    }

    // There're 3 type for depth info.:
    // 1. If the depth is not like 0/0/0/, for example 0/1/, we should record the whole depth info.
    // 2. If the depth is like 0/0/0/, we could only record the depth number(2).
    // 3. Further if the depth is 0, we could ignore it as default.
    if (!mainLine) {
      vtabIndexStr += depthStr;
    } else {
      if (virtualMethods[i].depth.size() > 1) {
        vtabIndexStr += std::to_string(virtualMethods[i].depth.size() - 1);
      }
    }
    if (virtualMethods[i].isInterface) {
      vtabIndexStr += '!';
    } else {
      vtabIndexStr += '#';
    }
    vtabIndexStr += std::to_string(virtualMethods[i].index);
    vtabIndexStr += ',';
  }
  LINKER_DLOG(lazybinding) << "vtabIndexStr=" << vtabIndexStr << ", for " << klass->GetName() << maple::endl;
  return vtabIndexStr;
}
#endif // LINKER_RT_LAZY_CACHE

MplLazyBindingVTableMapT MethodBuilder::GetMethodVTableMap(MClass *klass) {
  std::set<MClass*> checkedClasses;
  std::vector<MethodItem> virtualMethods;
  MplLazyBindingVTableMapT adjMethods;
  bool isDecouple = false;
  std::vector<uint16_t> depth;
  CollectClassMethodsRecursive(
      klass, isDecouple, checkedClasses, depth, static_cast<uint16_t>(-1), virtualMethods, adjMethods);
  return adjMethods;
}

std::string MethodBuilder::BuildMethod(MClass *klass) {
  std::set<MClass*> checkedClasses;
  std::vector<MethodItem> virtualMethods;
  MplLazyBindingVTableMapT adjMethods;
  bool isDecouple = false;
  std::vector<uint16_t> depth;
  CollectClassMethodsRecursive(klass, isDecouple, checkedClasses, depth, 0, virtualMethods, adjMethods);
  BuildMethodVTableITable(klass, virtualMethods);
  std::string res;
#ifdef LINKER_RT_LAZY_CACHE
  res = GetMethodCachingIndexString(klass, virtualMethods);
#endif // LINKER_RT_LAZY_CACHE
  return res;
}
} // namespace maplert
