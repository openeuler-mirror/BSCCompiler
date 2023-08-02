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
#ifndef MRT_MAPLERT_INCLUDE_MCLASS_INLINE_H_
#define MRT_MAPLERT_INCLUDE_MCLASS_INLINE_H_
#include "mclass.h"
#include "mobject_inline.h"
#include "methodmeta_inline.h"
#include "fieldmeta_inline.h"
#include "mrt_class_init.h"
#include "mrt_profile.h"
#include "modifier.h"
#include "mrt_well_known.h"
#include "linker_api.h"
#include "mstring_inline.h"

namespace maplert {
inline ClassMetadata *MClass::GetClassMeta() const {
  return reinterpret_cast<ClassMetadata*>(const_cast<MClass*>(this));
}

inline ClassMetadataRO *MClass::GetClassMetaRo() const {
  const ClassMetadata *cls = GetClassMeta();
  return cls->classInfoRo.GetDataRef<ClassMetadataRO*>();
}

inline char *MClass::GetName() const {
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  char *name = classInfoRo->className.GetRef<char*>();
  __MRT_Profile_CString(name);
  return name;
}

inline uint32_t MClass::GetFlag() const {
#ifdef USE_32BIT_REF
  const ClassMetadata *cls = GetClassMeta();
  return cls->flag;
#else
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  return classInfoRo->flag;
#endif // USE_32BIT_REF
}

inline uint32_t MClass::GetModifier() const {
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  return classInfoRo->mod;
}

inline uint16_t MClass::GetClIndex() const {
  const ClassMetadata *cls = GetClassMeta();
  return cls->clIndex;
}

inline int32_t MClass::GetMonitor() const {
  return this->monitor;
}

inline uint32_t MClass::GetNumOfSuperClasses() const {
#if USE_32BIT_REF
  const ClassMetadata *cls = GetClassMeta();
  return cls->numOfSuperclasses;
#else
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  return classInfoRo->numOfSuperclasses;
#endif // USE_32BIT_REF
}

inline uint32_t MClass::GetNumOfInterface() const {
  uint32_t numOfInterface = 0;
  if (IsArrayClass()) {
    // array class has 2 interfaces.
    numOfInterface = 2;
  } else {
    uint32_t numOfSuper = GetNumOfSuperClasses();
    numOfInterface = (numOfSuper == 0) ? 0 : (IsInterface() ? numOfSuper : numOfSuper - 1);
  }
  return numOfInterface;
}

inline MClass *MClass::GetComponentClass() const {
  if (UNLIKELY(!IsArrayClass())) {
    return nullptr;
  }
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  MClass *componentClass = classInfoRo->componentClass.GetDataRef<MClass*>();
  return componentClass;
}

inline uint32_t MClass::GetComponentSize() const {
  DCHECK(IsArrayClass()) << "must array class." << maple::endl;
  const ClassMetadata *cls = GetClassMeta();
  return cls->sizeInfo.componentSize;
}

inline uint8_t *MClass::GetItab() const {
  const ClassMetadata *cls = GetClassMeta();
  return cls->iTable.GetDataRef<uint8_t*>();
}

inline uint8_t *MClass::GetVtab() const {
  const ClassMetadata *cls = GetClassMeta();
  return cls->vTable.GetDataRef<uint8_t*>();
}

inline uint8_t *MClass::GetGctib() const {
  ClassMetadata *cls = GetClassMeta();
  return cls->gctib.GetGctibRef<uint8_t*>();
}

inline uint32_t MClass::GetNumOfFields() const {
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  return classInfoRo->numOfFields;
}

inline uint32_t MClass::GetNumOfMethods() const {
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  return classInfoRo->numOfMethods;
}

inline uint32_t MClass::GetObjectSize() const {
  const ClassMetadata *cls = GetClassMeta();
  return cls->sizeInfo.objSize;
}

inline uintptr_t MClass::GetClinitFuncAddr() const {
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  return classInfoRo->clinitAddr.GetDataRef<uintptr_t>();
}

inline char *MClass::GetRawAnnotation() const {
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  return classInfoRo->annotation.GetDataRef<char*>();
}

inline MClass *MClass::GetSuperClass(uint32_t index) const {
  MClass **superClassArray = GetSuperClassArray();
  DCHECK(index < GetNumOfSuperClasses()) << "index too big." << maple::endl;
  return ResolveSuperClass(superClassArray + index);
}

inline MClass *MClass::GetSuperClass() const {
  MClass *super = nullptr;
  if (!IsInterface() && (GetNumOfSuperClasses() > 0)) {
    super = GetSuperClass(0);
  } else if (IsArrayClass()) {
    super = WellKnown::GetMClassObject();
  }
  return super;
}

ALWAYS_INLINE inline MClass **MClass::GetSuperClassArray() const {
  MClass *cls = const_cast<MClass*>(this);
  if (UNLIKELY(IsColdClass())) {
    LinkerAPI::Instance().ResolveColdClassSymbol(*cls);
  }
  return GetSuperClassArrayPtr();
}

inline MClass *MClass::ResolveSuperClass(MClass **pSuperClass) {
  MClass *super = *pSuperClass;
  LinkerRef ref(super->AsUintptr());
  // Need more fast check interface.
  if (UNLIKELY(ref.IsIndex())) {
    return reinterpret_cast<MClass*>(LinkerAPI::Instance().GetSuperClass(
        reinterpret_cast<ClassMetadata**>(pSuperClass)));
  } else {
    return super;
  }
}

inline MClass **MClass::GetSuperClassArrayPtr() const {
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  return classInfoRo->superclass.GetDataRef<MClass**>();
}

inline MClass *MClass::GetCacheTrueClass() const {
#ifdef USE_32BIT_REF
  const ClassMetadata *cls = GetClassMeta();
  return cls->cacheTrueClass.GetRawValue<MClass*>();
#else
  return nullptr;
#endif
}

inline  MClass *MClass::GetCacheFalseClass() const {
#ifdef USE_32BIT_REF
  const ClassMetadata *cls = GetClassMeta();
  return cls->cacheFalseClass.GetDataRef<MClass*>();
#else
  return nullptr;
#endif
}

inline void MClass::SetCacheTrueClass(const MClass &cacheClass) {
#ifdef USE_32BIT_REF
  ClassMetadata *cls = GetClassMeta();
  if (LIKELY(IsInitialized())) {
    cls->cacheTrueClass.SetDataRef(&cacheClass);
  }
#else
  (void)cacheClass;
#endif
}

inline void MClass::SetCacheFalseClass(const MClass &cacheClass) {
#ifdef USE_32BIT_REF
  ClassMetadata *cls = GetClassMeta();
  if (LIKELY(IsInitialized())) {
    cls->cacheFalseClass.SetDataRef(&cacheClass);
  }
#else
  (void)cacheClass;
#endif
}

inline FieldMeta *MClass::GetFieldMetas() const {
  __MRT_Profile_FieldMeta(*this);
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  FieldMeta *fields = nullptr;
  if (classInfoRo->fields.IsCompact()) {
    fields = FieldMetaCompact::DecodeCompactFieldMetas(*const_cast<MClass*>(this));
  } else {
    fields = classInfoRo->fields.GetDataRef<FieldMeta*>();
  }
  return fields;
}

inline MethodMeta *MClass::GetMethodMetas() const {
  __MRT_Profile_MethodMeta(*this);
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  MethodMeta *methods = nullptr;
  if (classInfoRo->methods.IsCompact()) {
    methods = MethodMetaCompact::DecodeCompactMethodMetas(*const_cast<MClass*>(this));
  } else {
    methods = classInfoRo->methods.GetDataRef<MethodMeta*>();
  }
  return methods;
}

inline MethodMeta *MClass::GetMethodMeta(uint32_t index) const {
  CHECK(index < GetNumOfMethods()) << "index too large." << maple::endl;
  MethodMeta *methods = GetMethodMetas();
  return &methods[index];
}

inline uint32_t MClass::GetMethodMetaIndex(const MethodMeta &srcMethodMeta) const {
  MethodMeta *methods = GetMethodMetas();
  DCHECK(methods != nullptr) << "MClass::GetMethodMetaIndex: methods is null." << maple::endl;
  uint32_t numOfMethods = GetNumOfMethods();
  for (uint32_t index = 0; index < numOfMethods; ++index) {
    if (&methods[index] == &srcMethodMeta) {
      return index;
    }
  }
  BUILTIN_UNREACHABLE();
}

inline bool MClass::HasFinalizer() const {
  uint32_t flags = GetFlag();
  return modifier::hasFinalizer(flags);
}

inline bool MClass::IsObjectArrayClass() const {
  MClass *componentClass = GetComponentClass();
  if (UNLIKELY(componentClass == nullptr)) {
    return false;
  }
  return !componentClass->IsPrimitiveClass();
}

inline bool MClass::IsPrimitiveClass() const {
  uint32_t flag = GetFlag();
  return modifier::IsPrimitiveClass(flag);
}

inline bool MClass::IsArrayClass() const {
  uint32_t flag = GetFlag();
  return modifier::IsArrayClass(flag);
}

inline bool MClass::IsStringClass() const {
  return this == WellKnown::GetMClassString();
}

inline bool MClass::IsAnonymousClass() const {
  uint32_t flag = GetFlag();
  return modifier::IsAnonymousClass(flag);
}

inline bool MClass::IsEnum() const {
  return modifier::IsEnum(GetModifier());
}

inline bool MClass::IsFinalizable() const {
  return modifier::IsFinalizable(GetModifier());
}

inline bool MClass::IsDecouple() const {
  return modifier::IsDecoupleClass(GetFlag());
}

inline bool MClass::IsInterface() const {
  uint32_t modifier = GetModifier();
  return modifier::IsInterface(modifier);
}

inline bool MClass::IsPublic() const {
  uint32_t modifier = GetModifier();
  return modifier::IsPublic(modifier);
}

inline bool MClass::IsProtected() const {
  uint32_t modifier = GetModifier();
  return modifier::IsProtected(modifier);
}

inline bool MClass::IsAbstract() const {
  uint32_t modifier = GetModifier();
  return modifier::IsAbstract(modifier);
}

inline bool MClass::IsProxy() const {
  uint32_t modifier = GetModifier();
  return modifier::IsProxy(modifier);
}

inline bool MClass::IsFinal() const {
  uint32_t modifier = GetModifier();
  return modifier::IsFinal(modifier);
}

inline bool MClass::IsColdClass() const {
#ifdef USE_32BIT_REF
  ClassMetadata *cls = GetClassMeta();
  uint16_t flag = __atomic_load_n(&cls->flag, __ATOMIC_ACQUIRE);
  return modifier::IsColdClass(flag);
#else
  ClassMetadataRO *clsRo = GetClassMetaRo();
  uint16_t flag = __atomic_load_n(&clsRo->flag, __ATOMIC_ACQUIRE);
  return modifier::IsColdClass(flag);
#endif // USE_32BIT_REF
}

inline bool MClass::IsLazyBinding() const {
#ifdef USE_32BIT_REF
  uint16_t flag = __atomic_load_n(&GetClassMeta()->flag, __ATOMIC_ACQUIRE);
  return modifier::IsLazyBindingClass(flag);
#else
  uint16_t flag = __atomic_load_n(&GetClassMetaRo()->flag, __ATOMIC_ACQUIRE);
  return modifier::IsLazyBindingClass(flag);
#endif // USE_32BIT_REF
}

inline bool MClass::IsLazy() const {
#ifdef USE_32BIT_REF
  uint16_t flag = __atomic_load_n(&GetClassMeta()->flag, __ATOMIC_ACQUIRE);
  return  modifier::IsLazyBindingClass(flag) || modifier::IsLazyBoundClass(flag);
#else
  uint16_t flag = __atomic_load_n(&GetClassMetaRo()->flag, __ATOMIC_ACQUIRE);
  return  modifier::IsLazyBindingClass(flag) || modifier::IsLazyBoundClass(flag);
#endif // USE_32BIT_REF
}

inline bool MClass::IsVerified() const {
#ifdef USE_32BIT_REF
  uint16_t flag = __atomic_load_n(&GetClassMeta()->flag, __ATOMIC_ACQUIRE);
  return !modifier::IsNotVerifiedClass(flag);
#else
  uint16_t flag = __atomic_load_n(&GetClassMetaRo()->flag, __ATOMIC_ACQUIRE);
  return !modifier::IsNotVerifiedClass(flag);
#endif // USE_32BIT_REF
}

inline void MClass::SetVerified() {
  uint16_t verifiedFlag = static_cast<uint16_t>(~modifier::kClassRuntimeVerify);
  ReSetFlag(verifiedFlag);
}

inline void MClass::SetClassMetaRoData(uintptr_t ro) {
  ClassMetadata *cls = GetClassMeta();
  cls->classInfoRo.SetDataRef(ro);
}

inline void MClass::SetFlag(uint16_t newFlag) {
#ifdef USE_32BIT_REF
  ClassMetadata *cls = GetClassMeta();
  uint16_t flag = __atomic_load_n(&cls->flag, __ATOMIC_ACQUIRE) | newFlag;
  __atomic_store_n(&cls->flag, flag, __ATOMIC_RELEASE);
#else
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  uint16_t flag = __atomic_load_n(&classInfoRo->flag, __ATOMIC_ACQUIRE) | newFlag;
  __atomic_store_n(&classInfoRo->flag, flag, __ATOMIC_RELEASE);
#endif // USE_32BIT_REF
}

inline void MClass::SetClIndex(uint16_t clIndex) {
  ClassMetadata *cls = GetClassMeta();
  cls->clIndex = clIndex;
}

inline void MClass::SetMonitor(int32_t monitor) {
  this->monitor = monitor;
}

inline void MClass::ReSetFlag(uint16_t newFlag) {
#ifdef USE_32BIT_REF
  ClassMetadata *cls = GetClassMeta();
  uint16_t flag = __atomic_load_n(&cls->flag, __ATOMIC_ACQUIRE) & newFlag;
  __atomic_store_n(&cls->flag, flag, __ATOMIC_RELEASE);
#else
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  uint16_t flag = __atomic_load_n(&classInfoRo->flag, __ATOMIC_ACQUIRE) & newFlag;
  __atomic_store_n(&classInfoRo->flag, flag, __ATOMIC_RELEASE);
#endif
}

inline void MClass::SetNumOfSuperClasses(uint32_t numOfSuperclasses) {
#if USE_32BIT_REF
  ClassMetadata *cls = GetClassMeta();
  cls->numOfSuperclasses = static_cast<uint16_t>(numOfSuperclasses);
#else
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  classInfoRo->numOfSuperclasses = numOfSuperclasses;
#endif // USE_32BIT_REF
}

inline void MClass::SetHotClass() {
  ReSetFlag(~modifier::kClassIsColdClass);
}

inline MethodMeta *MClass::GetRawMethodMetas() const {
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  MethodMeta *methods = classInfoRo->methods.GetDataRef<MethodMeta*>();
  return methods;
}

inline FieldMeta *MClass::GetRawFieldMetas() const {
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  FieldMeta *fields = classInfoRo->fields.GetDataRef<FieldMeta*>();
  return fields;
}

inline FieldMeta *MClass::GetFieldMeta(uint32_t index) const {
  DCHECK(index < GetNumOfFields()) << "index too large." << maple::endl;
  FieldMeta *fields = GetRawFieldMetas();
  DCHECK(fields != nullptr) << "fields is nullptr." << maple::endl;
  return fields + index;
}

inline bool MClass::IsAssignableFrom(MClass &cls) const {
#ifdef USE_32BIT_REF
  if ((&cls == this) || (cls.GetCacheTrueClass() == this)) {
    return true;
  } else if (cls.GetCacheFalseClass() == this) {
    return false;
  }
#endif
  bool result = IsAssignableFromImpl(cls);
#ifdef USE_32BIT_REF
  if (result) {
    cls.SetCacheTrueClass(*this);
  } else {
    cls.SetCacheFalseClass(*this);
  }
#endif
  return result;
}

ALWAYS_INLINE inline uint16_t MClass::GetMethodFieldHash(const MString *name, const MArray *signature, bool isMethod) {
  DCHECK(name != nullptr) << "name must non null." << maple::endl;
  uint32_t hash = 5381; // initial value for DJB hash algorithm
  constexpr uint32_t kShift = 5;
  // cal method name
  if (name->IsCompress()) {
    uint32_t nameLen = name->GetLength();
    uint8_t *contents = name->GetContentsPtr();
    for (uint32_t i = 0; i < nameLen; ++i) {
      hash += (hash << kShift) + (*contents++);
    }
  } else {
    std::string charMethodName = name->GetChars();
    // Note: charMethodName is the name encoded in UTF-8 as a sequence of bytes.
    for (char byte : charMethodName) {
      // Iterate through the byte sequence.  Treat each byte as a uint8_t, and zero-extend it to 32 bits.  This is
      // consistent with the algorithm used by the AoT compiler.
      uint32_t byteU32 = static_cast<uint32_t>(static_cast<uint8_t>(byte));
      hash += (hash << kShift) + byteU32;
    }
  }
  if (isMethod) {
    // cal signature
    hash += (hash << kShift) + '(';
    if (signature != nullptr) {
      uint32_t arrayLen = signature->GetLength();
      for (uint32_t i = 0; i < arrayLen; ++i) {
        MClass *clsObj = static_cast<MClass*>(signature->GetObjectElementNoRc(i));
        // we have check the args non null
        char *className = clsObj->GetName();
        while (*className) {
          hash += (hash << kShift) + (*className++);
        }
      }
    }
    hash += (hash << kShift) + ')';
  }
  return (hash & 0x7FFFFFFF) % modifier::kMethodFieldHashSize;
}

ALWAYS_INLINE inline uint16_t MClass::GetMethodFieldHash(const char *name, const char *signature, bool isMethod) {
  DCHECK(name != nullptr) << "name must non null." << maple::endl;
  DCHECK(!isMethod || (signature != nullptr)) << "signature must non null." << maple::endl;
  unsigned int hash = 5381; // initial value for DJB hash algorithm
  constexpr uint32_t kShift = 5;
  while (*name) {
    hash += (hash << kShift) + (*name++);
  }
  if (isMethod) {
    while (*signature) {
      hash += (hash << kShift) + (*signature++);
      if (*(signature - 1) == ')') {
        break;
      }
    }
  }
  return (hash & 0x7FFFFFFF) % modifier::kMethodFieldHashSize;
}

template<typename T1, typename T2, typename T3>
ALWAYS_INLINE inline T1 *MClass::CmpHashEqual(int32_t mid, uint32_t num, T1 *metas, T2 name,
                                              T3 signature, uint16_t srcHash) const {
  int32_t numOfMetas = static_cast<int32_t>(num);
  while (++mid < numOfMetas) {
    T1 *meta = metas + mid;
    if (srcHash != meta->GetHashCode()) {
      break;
    }
  }
  // find left
  while (--mid >= 0) {
    T1 *meta = metas + mid;
    if (srcHash == meta->GetHashCode()) {
      if (meta->Cmp(name, signature)) {
        return meta;
      }
    } else {
      break;
    }
  }
  return nullptr;
}

template<typename T1, typename T2, typename T3>
ALWAYS_INLINE inline T1 *MClass::GetDeclareMethodFieldUtil(T1 *metas, uint32_t num, T2 name,
                                                           T3 signature, bool isMethod) const {
  uint16_t srcHash = GetMethodFieldHash(name, signature, isMethod);
  int32_t low = 0;
  int32_t high = static_cast<int32_t>(num) - 1;
  int32_t mid = 0;
  while (low <= high) {
    mid = static_cast<int32_t>(static_cast<uint32_t>(low + high) >> 1);
    T1 *meta = metas + mid;
    uint16_t dstHash = meta->GetHashCode();
    if (dstHash == srcHash) {
      if (meta->Cmp(name, signature) && !isMethod) {
        return meta;
      }
      T1 *result = CmpHashEqual(mid, num, metas, name, signature, srcHash);
      if (result != nullptr) {
        return result;
      }
      break;
    }
    if (dstHash < srcHash) {
      low = mid + 1;
    } else {
      high = mid - 1;
    }
  }

  // the following code just for compatibility, this may impact performance when field not found
  // now, kMajorMplVersion is same in maple 2.1 && 2.2. we will optimise these code when version changed
  for (int i = static_cast<int32_t>(num) - 1; i >= 0 ; --i) {
    T1 *meta = metas + i;
    if (modifier::kHashConflict == meta->GetHashCode()) {
      if (meta->Cmp(name, signature)) {
        return meta;
      }
    } else {
      break;
    }
  }
  return nullptr;
}

template<typename T1, typename T2>
ALWAYS_INLINE inline MethodMeta *MClass::GetDeclaredMethodUtil(T1 methodName, T2 methodSignature) const {
  MethodMeta *methods = GetMethodMetas();
  uint32_t num = GetNumOfMethods();
  return GetDeclareMethodFieldUtil(methods, num, methodName, methodSignature, true);
}

ALWAYS_INLINE inline MethodMeta *MClass::GetDeclaredMethod(const char *methodName, const char *signatureName) const {
  if ((methodName == nullptr) || (signatureName == nullptr)) {
    return nullptr;
  }
  return GetDeclaredMethodUtil(methodName, signatureName);
}

ALWAYS_INLINE inline MethodMeta *MClass::GetDeclaredMethod(const MString *methodName,
                                                           const MArray *signatureClass) const {
  if ((methodName == nullptr) || ((signatureClass != nullptr) && signatureClass->HasNullElement())) {
    return nullptr;
  }
  MethodMeta *methodMeta = GetDeclaredMethodUtil(methodName, signatureClass);
  return (methodMeta != nullptr) ? (methodMeta->IsConstructor() ? nullptr : methodMeta) : nullptr;
}

ALWAYS_INLINE inline MethodMeta *MClass::GetDefaultConstructor() const {
  MethodMeta *methods = GetMethodMetas();
  uint32_t num = GetNumOfMethods();
  for (uint32_t i = 0; i < num; ++i) {
    MethodMeta *methodMeta = &methods[i];
    // defaultConstructor has 1 arg
    if (methodMeta->IsConstructor() && methodMeta->GetArgSize() == 1) {
      return methodMeta;
    }
  }
  return nullptr;
}

ALWAYS_INLINE inline MethodMeta *MClass::GetVirtualMethod(const MethodMeta &srcMethod) const {
  MClass *declarClass = srcMethod.GetDeclaringClass();
  if ((declarClass == this) || srcMethod.IsDirectMethod()) {
    return const_cast<MethodMeta*>(&srcMethod);
  }
  return GetMethodForVirtual(srcMethod);
}

template<typename T1, typename T2>
ALWAYS_INLINE inline FieldMeta *MClass::GetDeclaredFieldUtil(T1 fieldName, T2 fieldType) const {
  uint32_t num = GetNumOfFields();
  FieldMeta *fields = GetFieldMetas();
  return GetDeclareMethodFieldUtil(fields, num, fieldName, fieldType, false);
}

ALWAYS_INLINE inline FieldMeta *MClass::GetDeclaredField(const char *name, const char *type) const {
  return GetDeclaredFieldUtil(name, type);
}

ALWAYS_INLINE inline FieldMeta *MClass::GetDeclaredField(const MString *name) const {
  return GetDeclaredFieldUtil(name, nullptr);
}

template<typename T1, typename T2>
ALWAYS_INLINE inline FieldMeta *MClass::GetFieldUtil(T1 fieldName, T2 fieldType, bool isPublic) const {
  FieldMeta *result = nullptr;
  const MClass *superClass = this;
  while (superClass != nullptr) {
    result = superClass->GetDeclaredFieldUtil(fieldName, fieldType);
    if ((result != nullptr) && (!isPublic || result->IsPublic())) {
      return result;
    }
    uint32_t numOfInterface = superClass->GetNumOfInterface();
    MClass *interfaceVector[numOfInterface];
    superClass->GetDirectInterfaces(interfaceVector, numOfInterface);
    for (uint32_t i = 0; i < numOfInterface; ++i) {
      auto interface = interfaceVector[i];
      result = interface->GetFieldUtil(fieldName, fieldType, isPublic);
      if ((result != nullptr) && (!isPublic || result->IsPublic())) {
        return result;
      }
    }
    if (superClass->IsInterface()) {
      break;
    }
    superClass = superClass->GetSuperClass();
  }
  return nullptr;
}

ALWAYS_INLINE inline FieldMeta *MClass::GetField(const char *fieldName, const char *fieldType, bool isPublic) const {
  return GetFieldUtil(fieldName, fieldType, isPublic);
}

ALWAYS_INLINE inline FieldMeta *MClass::GetField(const MString *fieldName, bool isPublic) const {
  return GetFieldUtil(fieldName, nullptr, isPublic);
}

template<typename T>
ALWAYS_INLINE inline MethodMeta *MClass::GetDeclaredConstructorUtil(T signature) const {
  MethodMeta *methods = GetMethodMetas();
  uint32_t num = GetNumOfMethods();
  for (uint32_t i = 0; i < num; ++i) {
    MethodMeta *methodMeta = &methods[i];
    if (methodMeta->IsConstructor() && !methodMeta->IsStatic() && methodMeta->SignatureCmp(signature)) {
      return methodMeta;
    }
  }
  return nullptr;
}

inline MethodMeta *MClass::GetDeclaredConstructor(const char *signature) const {
  return GetDeclaredConstructorUtil(signature);
}

inline MethodMeta *MClass::GetDeclaredConstructor(const MArray *signature) const {
  return GetDeclaredConstructorUtil(signature);
}

inline ClassInitState MClass::GetInitState() const {
  ClassMetadata *cls = GetClassMeta();
  uintptr_t val = cls->GetInitStateRawValue();
  uintptr_t state = 0;
  // previously ClassInitState::kClassInitStateUninitialized is defined as 0.
  if (val == reinterpret_cast<uintptr_t>(&classInitProtectRegion[kClassUninitialized - 1])) {
    return kClassUninitialized;
  } else if (kSEGVAddrForClassInitStateMin < val && val < kSEGVAddrFoClassInitStateMax) {
    state = val - static_cast<intptr_t>(kSEGVAddrForClassInitStateMin);
  } else {
    return kClassInitialized;
  }

  if (kClassInitStateMin < state && state < kClassInitStateMax) {
    return static_cast<ClassInitState>(state);
  } else if (kClassInitStateMax <= state) {
    return kClassInitialized;
  }
  BUILTIN_UNREACHABLE();
}

inline void MClass::SetInitState(ClassInitState state) {
  if (kClassInitStateMin < state && state < kClassInitStateMax) {
    uintptr_t stateVal = kSEGVAddrForClassInitStateMin + state;
    ClassMetadata *cls = GetClassMeta();
    cls->SetInitStateRawValue(stateVal);
    return;
  }
  BUILTIN_UNREACHABLE();
}

inline bool MClass::IsInitialized() const {
  return GetInitState() == kClassInitialized;
}

inline bool MClass::InitClassIfNeeded() const {
  return IsInitialized() ? true : MRT_InitClassIfNeeded(*this);
}

inline uint32_t MClass::GetDimension() const {
  return GetDimensionFromDescriptor(GetName());
}

inline uint32_t MClass::GetDimensionFromDescriptor(const std::string &descriptor) {
  uint32_t dim = 0;
  while (descriptor[dim] == '[') {
    dim++;
  }
  return dim;
}

inline MClass *MClass::GetPrimitiveClass(const char *descriptor) {
  DCHECK(descriptor != nullptr);
  switch (*descriptor) {
    case 'Z':
      return WellKnown::GetMClassZ();
    case 'B':
      return WellKnown::GetMClassB();
    case 'C':
      return WellKnown::GetMClassC();
    case 'S':
      return WellKnown::GetMClassS();
    case 'I':
      return WellKnown::GetMClassI();
    case 'J':
      return WellKnown::GetMClassJ();
    case 'F':
      return WellKnown::GetMClassF();
    case 'D':
      return WellKnown::GetMClassD();
    case 'V':
      return WellKnown::GetMClassV();
    default:
      return nullptr;
    }
}

inline MClass *MClass::GetClassFromDescriptor(const MObject *context, const char *descriptor, bool throwException) {
  // first check primitive class
  MClass *cls = GetPrimitiveClass(descriptor);
  return (cls != nullptr) ? cls : GetClassFromDescriptorUtil(context, descriptor, throwException);
}

template<typename T>
inline MClass *MClass::JniCast(T c) {
  static_assert(std::is_same<T, jobject>::value || std::is_same<T, jclass>::value, "wrong type");
  return reinterpret_cast<MClass*>(c);
}

template<typename T>
inline MClass *MClass::JniCastNonNull(T c) {
  DCHECK(c != nullptr);
  return JniCast(c);
}

inline jclass MClass::AsJclass() const {
  return reinterpret_cast<jclass>(const_cast<MClass*>(this));
}
} // namespace maplert
#endif // MRT_MAPLERT_INCLUDE_MCLASS_INLINE_H_

