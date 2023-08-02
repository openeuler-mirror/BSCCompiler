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
#ifndef MRT_MAPLERT_INCLUDE_MCLASS_H_
#define MRT_MAPLERT_INCLUDE_MCLASS_H_
#include "mobject.h"
#include "metadata_inline.h"
#include "fieldmeta.h"
#include "methodmeta.h"

namespace maplert {
// NOTE: class object layout is different from java.lang.Class
//       its layout defind in struct ClassMetadata
class FieldMeta;
class MethodMeta;
class FieldMetaCompact;
class MethodMetaCompact;
class MClass : public MObject {
 public:
  char *GetName() const;
  uint16_t GetClIndex() const;
  uint32_t GetFlag() const;
  int32_t GetMonitor() const;
  uint32_t GetNumOfSuperClasses() const;
  uint32_t GetNumOfInterface() const;
  MClass *GetComponentClass() const;
  uint32_t GetComponentSize() const;
  uint8_t *GetItab() const;
  uint8_t *GetVtab() const;
  uint8_t *GetGctib() const;
  uint32_t GetNumOfFields() const;
  uint32_t GetNumOfMethods() const;
  uint32_t GetModifier() const;
  uint32_t GetObjectSize() const;
  uintptr_t GetClinitFuncAddr() const;
  std::string GetAnnotation() const;
  char *GetRawAnnotation() const;
  // super class in java
  MClass *GetSuperClass() const;
  // include interface
  MClass *GetSuperClass(uint32_t index) const;
  // resolve data
  MClass **GetSuperClassArray() const;
  // unresolve data
  MClass **GetSuperClassArrayPtr() const;
  MClass *GetCacheTrueClass() const;
  MClass *GetCacheFalseClass() const;
  bool HasFinalizer() const;
  bool IsObjectArrayClass() const;
  bool IsPrimitiveClass() const;
  bool IsArrayClass() const;
  bool IsInterface() const;
  bool IsPublic() const;
  bool IsProtected() const;
  bool IsAbstract() const;
  bool IsProxy() const;
  bool IsStringClass() const;
  bool IsAnonymousClass() const;
  bool IsEnum() const;
  bool IsDecouple() const;
  bool IsFinalizable() const;
  bool IsColdClass() const;
  bool IsLazyBinding() const;
  bool IsAssignableFrom(MClass &cls) const;
  bool IsLazy() const;
  bool IsFinal() const;
  bool IsInnerClass() const;
  bool IsVerified() const;
  void SetVerified();

  // java.lang.Class<java.lang.Object>
  void GetPrettyClass(std::string &dstName) const;
  // java.lang.Object, [Ljava.lang.Object;, int, [I
  void GetBinaryName(std::string &dstName) const;
  // java.lang.Object, java.lang.Object[], int, int[]
  void GetTypeName(std::string &dstName) const;
  // Ljava/lang/Object; [Ljava/lang/Object; I, [I
  void GetDescriptor(std::string &dstName) const;
  static void ConvertDescriptorToTypeName(const std::string &descriptor, std::string &defineName);
  static void ConvertDescriptorToBinaryName(const std::string &descriptor, std::string &binaryName);
  uint32_t GetDimension() const;
  static uint32_t GetDimensionFromDescriptor(const std::string &descriptor);

  FieldMetaCompact *GetCompactFields() const;
  FieldMeta *GetFieldMetas() const;
  FieldMeta *GetFieldMeta(uint32_t index) const;
  MethodMetaCompact *GetCompactMethods() const;
  MethodMeta *GetMethodMeta(uint32_t index) const;
  MethodMeta *GetMethodMetas() const;
  MethodMeta *GetRawMethodMetas() const;
  MethodMeta *GetDeclaredMethod(const char *methodName, const char *signatureName) const;
  MethodMeta *GetDeclaredMethod(const MString *methodName, const MArray *signatureName) const;
  MethodMeta *GetClinitMethodMeta() const;
  MethodMeta *GetVirtualMethod(const MethodMeta &srcMethod) const;
  // recursive search method
  MethodMeta *GetMethod(const char *methodName, const char *signatureName) const;
  MethodMeta *GetInterfaceMethod(const char *methodName, const char *signatureName) const;
  MethodMeta *GetMethodForVirtual(const MethodMeta &srcMethod) const;
  MethodMeta *GetDeclaredConstructor(const char *signature) const;
  MethodMeta *GetDeclaredConstructor(const MArray *signature) const;
  MethodMeta *GetDefaultConstructor() const;
  MethodMeta *GetDeclaredFinalizeMethod() const;
  MethodMeta *GetFinalizeMethod() const;
  FieldMeta *GetDeclaredField(const char *name, const char *type = nullptr) const;
  FieldMeta *GetDeclaredField(const MString *name) const;
  // recursive search field
  FieldMeta *GetField(const char *fieldName, const char *fieldType = nullptr, bool isPublic = false) const;
  FieldMeta *GetField(const MString *fieldName, bool isPublic) const;
  FieldMeta *GetRawFieldMetas() const;
  bool IsCompactMetaMethods() const;
  bool IsCompactMetaFields() const;
  uint32_t GetMethodMetaIndex(const MethodMeta &srcMethodMeta) const;
  void GetDeclaredMethods(std::vector<MethodMeta*> &methodsVector, bool publicOnly) const;
  void GetSuperClassInterfaces(uint32_t numOfSuperClass, MClass **superArray,
                               std::vector<MClass*> &interfaceVector, uint32_t firstSuperClass) const;
  void GetInterfaces(std::vector<MClass*> &interfaceVector) const;
  void GetDirectInterfaces(MClass *interfaceVector[], uint32_t size) const;
  bool InitClassIfNeeded() const;
  ClassInitState GetInitState() const;
  bool IsInitialized() const;
  void SetInitState(ClassInitState state);
  MObject *GetSignatureAnnotation() const;

  uint32_t GetArrayModifiers() const;
  void SetName(const char *name);
  void SetModifier(uint32_t newMod);
  void SetGctib(uintptr_t newGctib);
  void SetSuperClassArray(uintptr_t newValue);
  void SetObjectSize(uint32_t newSize);
  void SetItable(uintptr_t newItb);
  void SetVtable(uintptr_t newVtab);
  void SetNumOfFields(uint32_t newValue);
  void SetNumOfMethods(uint32_t newValue);
  void SetMethods(const MethodMeta &newMethods);
  void SetFields(const FieldMeta &newFields);
  void SetInitStateRawValue(uintptr_t newValue);
  void SetClassMetaRoData(uintptr_t ro);
  void SetNumOfSuperClasses(uint32_t numOfSuperclasses);
  void SetHotClass();
  void SetFlag(uint16_t newFlag);
  void SetClIndex(uint16_t clIndex);
  void SetMonitor(int32_t monitor);
  void ReSetFlag(uint16_t newFlag);
  void SetNumSuperClasses(uint32_t numOfSuperclasses);
  void SetComponentClass(const MClass &klass);
  // for instanceOf cache
  void SetCacheTrueClass(const MClass &cacheClass);
  void SetCacheFalseClass(const MClass &cacheClass);

  void ResolveVtabItab();
  // super class may not visible, resolve by other thread
  static MClass *ResolveSuperClass(MClass **super);
  static MClass *GetArrayClass(const MClass &componentClass);

  // context can caller Class, or its instance
  static MClass *GetClassFromDescriptor(const MObject *context, const char *descriptor, bool throwException = true);
  static MClass *GetClassFromDescriptorUtil(const MObject *context, const char *descriptor, bool throwException);
  static MClass *GetPrimitiveClass(const char *descriptor);

  // alloc class memory from perm space if newClsMem is 0,
  // newClsMem size: sizeof(ClassMetadata) + sizeof(ClassMetadataRO)
  static MClass *NewMClass(uintptr_t newClsMem = 0);
  static uint16_t GetMethodFieldHash(const char *name, const char *signature, bool isMethod);

  operator jclass () {
    return reinterpret_cast<jclass>(this);
  }

  operator jclass () const {
    return reinterpret_cast<jclass>(const_cast<MClass*>(this));
  }

  template<typename T>
  static inline MClass *JniCast(T c);
  template<typename T>
  static inline MClass *JniCastNonNull(T c);
  jclass inline AsJclass() const;

 private:
  ClassMetadataRO *GetClassMetaRo() const;
  ClassMetadata *GetClassMeta() const;
  bool IsAssignableFromImpl(const MClass &cls) const;
  // this class is interface
  bool IsAssignableFromInterface(const MClass &cls) const;
  static uint16_t GetMethodFieldHash(const MString *name, const MArray *signature, bool isMethod);

  template<typename T1, typename T2, typename T3>
  T1 *CmpHashEqual(int32_t mid, uint32_t num, T1 *metas, T2 name, T3 signature, uint16_t srcHash) const;
  template<typename T1, typename T2, typename T3>
  T1 *GetDeclareMethodFieldUtil(T1 *metas, uint32_t num, T2 name, T3 signature, bool isMethod) const;
  template<typename T1, typename T2>
  MethodMeta *GetDeclaredMethodUtil(T1 methodName, T2 methodSignature) const;
  template<typename T1, typename T2>
  FieldMeta *GetDeclaredFieldUtil(T1 fieldName, T2 fieldType) const;
  template<typename T1, typename T2>
  FieldMeta *GetFieldUtil(T1 fieldName, T2 fieldType, bool isPublic) const;
  template<typename T1, typename T2>
  FieldMeta *CmpHashEqualField(int32_t mid, T1 fieldName, T2 fieldType, uint16_t srchash) const;
  template<typename T>
  MethodMeta *GetDeclaredConstructorUtil(T signature) const;
};
} // namespace maplert
#endif // MRT_MAPLERT_INCLUDE_MCLASS_H_

