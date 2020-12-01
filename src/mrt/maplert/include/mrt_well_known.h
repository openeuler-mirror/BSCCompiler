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
#ifndef MRT_WELL_KNOWN_H_
#define MRT_WELL_KNOWN_H_
#include <vector>

#include "mclass.h"
namespace maplert {
class WellKnown {
 public:
  // public fields
  // array class interface always Ljava/lang/Cloneable; Ljava/io/Serializable;
  static constexpr uint8_t kArrayInterfaceNum = 2;
  static std::vector<MClass*> arrayInterface;
  static size_t kReferenceReferentOffset; // java/lang/Reference::Referent
  static size_t kReferenceQueueOffset; // java/lang/Reference::Queue
  // java/lang/Reference::Pendingnext for libcore or java/lang/Reference::discovered for OpenJDK
  static size_t kReferencePendingnextOffset;
  // java/lang/ref/FinalizerReference::Zombie for libcore only
  static size_t kFinalizereferenceZombieOffset;

  // public methods
  static inline MClass *GetMClassZ() {
    return primitiveClassZ;
  }

  static inline MClass *GetMClassB() {
    return primitiveClassB;
  }

  static inline MClass *GetMClassS() {
    return primitiveClassS;
  }

  static inline MClass *GetMClassC() {
    return primitiveClassC;
  }

  static inline MClass *GetMClassI() {
    return primitiveClassI;
  }

  static inline MClass *GetMClassJ() {
    return primitiveClassJ;
  }

  static inline MClass *GetMClassF() {
    return primitiveClassF;
  }

  static inline MClass *GetMClassD() {
    return primitiveClassD;
  }

  static inline MClass *GetMClassV() {
    return primitiveClassV;
  }

  static inline MClass *GetMClassAZ() {
    return primitiveClassAZ;
  }

  static inline MClass *GetMClassAB() {
    return primitiveClassAB;
  }

  static inline MClass *GetMClassAS() {
    return primitiveClassAS;
  }

  static inline MClass *GetMClassAC() {
    return primitiveClassAC;
  }

  static inline MClass *GetMClassAI() {
    return primitiveClassAI;
  }

  static inline MClass *GetMClassAJ() {
    return primitiveClassAJ;
  }

  static inline MClass *GetMClassAF() {
    return primitiveClassAF;
  }

  static inline MClass *GetMClassAD() {
    return primitiveClassAD;
  }

  static inline MClass *GetMClassAAZ() {
    return primitiveClassAAZ;
  }

  static inline MClass *GetMClassAAB() {
    return primitiveClassAAB;
  }

  static inline MClass *GetMClassAAS() {
    return primitiveClassAAS;
  }

  static inline MClass *GetMClassAAC() {
    return primitiveClassAAC;
  }

  static inline MClass *GetMClassAAI() {
    return primitiveClassAAI;
  }

  static inline MClass *GetMClassAAJ() {
    return primitiveClassAAJ;
  }

  static inline MClass *GetMClassAAF() {
    return primitiveClassAAF;
  }

  static inline MClass *GetMClassAAD() {
    return primitiveClassAAD;
  }

  static inline MClass *GetMClassVoid() {
    return Ljava_2Flang_2FVoid_3B;
  }

  static inline MClass *GetMClassBoolean() {
    return Ljava_2Flang_2FBoolean_3B;
  }

  static inline MClass *GetMClassByte() {
    return Ljava_2Flang_2FByte_3B;
  }

  static inline MClass *GetMClassCharacter() {
    return Ljava_2Flang_2FCharacter_3B;
  }

  static inline MClass *GetMClassShort() {
    return Ljava_2Flang_2FShort_3B;
  }

  static inline MClass *GetMClassInteger() {
    return Ljava_2Flang_2FInteger_3B;
  }

  static inline MClass *GetMClassLong() {
    return Ljava_2Flang_2FLong_3B;
  }

  static inline MClass *GetMClassFloat() {
    return Ljava_2Flang_2FFloat_3B;
  }

  static inline MClass *GetMClassDouble() {
    return Ljava_2Flang_2FDouble_3B;
  }

  static inline MClass *GetMClassNumber() {
    return Ljava_2Flang_2FNumber_3B;
  }

  static inline MClass *GetMClassObject() {
    return Ljava_2Flang_2FObject_3B;
  }

  static inline MClass *GetMClassClass() {
    return Ljava_2Flang_2FClass_3B;
  }

  static inline MClass *GetMClassField() {
    return Ljava_2Flang_2Freflect_2FField_3B;
  }

  static inline MClass *GetMClassConstructor() {
    return Ljava_2Flang_2Freflect_2FConstructor_3B;
  }

  static inline MClass *GetMClassMethod() {
    return Ljava_2Flang_2Freflect_2FMethod_3B;
  }

  static inline MClass *GetMClassProxy() {
    return Ljava_2Flang_2Freflect_2FProxy_3B;
  }

  static inline MClass *GetMClassParameter() {
    return Ljava_2Flang_2Freflect_2FParameter_3B;
  }

  static inline MClass *GetMClassGenericSignatureParser() {
    return Llibcore_2Freflect_2FGenericSignatureParser_3B;
  }

  static inline MClass *GetMClassCloneable() {
    return Ljava_2Flang_2FCloneable_3B;
  }

  static inline MClass *GetMClassSerializable() {
    return Ljava_2Fio_2FSerializable_3B;
  }

  static inline MClass *GetMClassString() {
    return Ljava_2Flang_2FString_3B;
  }

  static inline MClass *GetMClassStringFactory() {
    return Ljava_2Flang_2FStringFactory_3B;
  }

  static inline MClass *GetMClassError() {
    return Ljava_2Flang_2FError_3B;
  }

  static inline MClass *GetMClassThrowable() {
    return Ljava_2Flang_2FThrowable_3B;
  }

  static inline MClass *GetMClassArithmeticException() {
    return Ljava_2Flang_2FArithmeticException_3B;
  }

  static inline MClass *GetMClassInterruptedException() {
    return Ljava_2Flang_2FInterruptedException_3B;
  }

  static inline MClass *GetMClassClassCastException() {
    return Ljava_2Flang_2FClassCastException_3B;
  }

  static inline MClass *GetMClassUnsatisfiedLinkError() {
    return Ljava_2Flang_2FUnsatisfiedLinkError_3B;
  }

  static inline MClass *GetMClassStringIndexOutOfBoundsException() {
    return Ljava_2Flang_2FStringIndexOutOfBoundsException_3B;
  }

  static inline MClass *GetMClassNoClassDefFoundError() {
    return Ljava_2Flang_2FNoClassDefFoundError_3B;
  }

  static inline MClass *GetMClassNoSuchMethodError() {
    return Ljava_2Flang_2FNoSuchMethodError_3B;
  }

  static inline MClass *GetMClassNoSuchFieldError() {
    return Ljava_2Flang_2FNoSuchFieldError_3B;
  }

  static inline MClass *GetMClassVerifyError() {
    return Ljava_2Flang_2FVerifyError_3B;
  }

  static inline MClass *GetMClassExceptionInInitializerError() {
    return Ljava_2Flang_2FExceptionInInitializerError_3B;
  }

  static inline MClass *GetMClassRuntimeException() {
    return Ljava_2Flang_2FRuntimeException_3B;
  }

  static inline MClass *GetMClassSecurityException() {
    return Ljava_2Flang_2FSecurityException_3B;
  }

  static inline MClass *GetMClassUndeclaredThrowableException() {
    return Ljava_2Flang_2Freflect_2FUndeclaredThrowableException_3B;
  }

  static inline MClass *GetMClassArrayStoreException() {
    return Ljava_2Flang_2FArrayStoreException_3B;
  }

  static inline MClass *GetMClassArrayIndexOutOfBoundsException() {
    return Ljava_2Flang_2FArrayIndexOutOfBoundsException_3B;
  }

  static inline MClass *GetMClassNullPointerException() {
    return Ljava_2Flang_2FNullPointerException_3B;
  }

  static inline MClass *GetMClassEnum() {
    return Ljava_2Flang_2FEnum_3B;
  }

  static inline MClass *GetMClassAnnotation() {
    return Ljava_2Flang_2Fannotation_2FAnnotation_3B;
  }

  static inline MClass *GetMClassAnnotationMember() {
    return Llibcore_2Freflect_2FAnnotationMember_3B;
  }

  static inline MClass *GetMClassAnnotationFactory() {
    return Llibcore_2Freflect_2FAnnotationFactory_3B;
  }

  static inline MClass *GetMClassDelegateLastClassLoader() {
    return Ldalvik_2Fsystem_2FDelegateLastClassLoader_3B;
  }

  static inline MClass *GetMClassPathClassLoader() {
    return Ldalvik_2Fsystem_2FPathClassLoader_3B;
  }

  static inline MClass *GetMClassDexClassLoader() {
    return Ldalvik_2Fsystem_2FDexClassLoader_3B;
  }

  static inline MClass *GetMClassInMemoryDexClassLoader() {
    return Ldalvik_2Fsystem_2FInMemoryDexClassLoader_3B;
  }

  static inline MClass *GetMClassMethodType() {
    return Ljava_2Flang_2Finvoke_2FMethodType_3B;
  }

  static inline MClass *GetMClassMethodHandle() {
    return Ljava_2Flang_2Finvoke_2FMethodHandle_3B;
  }

  static inline size_t GetMFieldMethodHandleDataArrayOffset() {
    return Ljava_2Flang_2FMethodHandle_3B_dataArray_offset;
  }

  static inline size_t GetMFieldMethodHandleMetaArrayOffset() {
    return Ljava_2Flang_2FMethodHandle_3B_metaArray_offset;
  }

  static inline size_t GetMFieldMethodHandleTypeArrayOffset() {
    return Ljava_2Flang_2FMethodHandle_3B_typeArray_offset;
  }

  static inline size_t GetMFieldMethodHandleOpArrayOffset() {
    return Ljava_2Flang_2FMethodHandle_3B_opArray_offset;
  }

  static inline size_t GetMFieldMethodHandleIndexOffset() {
    return Ljava_2Flang_2FMethodHandle_3B_index_offset;
  }

  static inline size_t GetMFieldBindToDataReceiverOffset() {
    return Ljava_2Flang_2FBindToData_3B_receiver_offset;
  }

  static inline size_t GetMFieldDropArgumentsDataNumDroppedOffset() {
    return Ljava_2Flang_2FDropArgumentsData_3B_numDropped_offset;
  }

  static inline size_t GetMFieldDropArgumentsDataStartPosOffset() {
    return Ljava_2Flang_2FDropArgumentsData_3B_startPos_offset;
  }

  static inline size_t GetMFieldFilterReturnValueDataTargetOffset() {
    return Ljava_2Flang_2FFilterReturnValueData_3B_target_offset;
  }

  static inline size_t GetMFieldFilterReturnValueDataFilterOffset() {
    return Ljava_2Flang_2FFilterReturnValueData_3B_filter_offset;
  }

  static inline size_t GetMFieldPermuteArgumentsDataTargetOffset() {
    return Ljava_2Flang_2FPermuteArgumentsData_3B_target_offset;
  }

  static inline size_t GetMFieldPermuteArgumentsDataReorderOffset() {
    return Ljava_2Flang_2FPermuteArgumentsData_3B_reorder_offset;
  }

  static inline MClass *GetMClassEmulatedStackFrame() {
    return Ldalvik_2Fsystem_2FEmulatedStackFrame_3B;
  }

  static inline size_t GetMFieldMethodHandleArtFieldOrMethodOffset() {
    return Ljava_2Flang_2FMethodHandle_3B_artFieldOrMethod_offset;
  }

  static inline size_t GetMFieldMethodHandleHandleKindOffset() {
    return Ljava_2Flang_2FMethodHandle_3B_handleKind_offset;
  }

  static inline size_t GetMFieldMethodHandleNominalTypeOffset() {
    return Ljava_2Flang_2FMethodHandle_3B_nominalType_offset;
  }

  static inline size_t GetMFieldMethodHandleTypeOffset() {
    return Ljava_2Flang_2FMethodHandle_3B_type_offset;
  }

  static inline size_t GetMFieldEmStackFrameCallsiteOffset() {
    return Ldalvik_2Fsystem_2FEmulatedStackFrame_3B_callsiteType_offset;
  }

  static inline size_t GetMFieldEmStackFrameReferencesOffset() {
    return Ldalvik_2Fsystem_2FEmulatedStackFrame_3B_references_offset;
  }

  static inline size_t GetMFieldEmStackFrameStackFrameOffset() {
    return Ldalvik_2Fsystem_2FEmulatedStackFrame_3B_stackFrame_offset;
  }

  static inline size_t GetMFieldEmStackFrameTypeOffset() {
    return Ldalvik_2Fsystem_2FEmulatedStackFrame_3B_type_offset;
  }

#ifdef __OPENJDK__
  static inline MClass *GetMClassHashMap() {
    return Ljava_2Futil_2FHashMap_3B;
  }

  static inline MClass *GetMClassAnnotationParser() {
    return Lsun_2Freflect_2Fannotation_2FAnnotationParser_3B;
  }
#endif // __OPENJDK__

  static inline MClass *GetMClassIntegerCache() {
    return Ljava_2Flang_2FInteger_24IntegerCache_3B;
  }

  static inline MClass *GetMClassByteCache() {
    return Ljava_2Flang_2FByte_24ByteCache_3B;
  }

  static inline MClass *GetMClassShortCache() {
    return Ljava_2Flang_2FShort_24ShortCache_3B;
  }

  static inline MClass *GetMClassCharacterCache() {
    return Ljava_2Flang_2FCharacter_24CharacterCache_3B;
  }

  static inline MClass *GetMClassLongCache() {
    return Ljava_2Flang_2FLong_24LongCache_3B;
  }

  static inline MClass *GetMClassReference() {
    return Ljava_2Flang_2Fref_2FReference_3B;
  }

  static inline MClass *GetMClassFinalizerReference() {
    return Ljava_2Flang_2Fref_2FFinalizerReference_3B;
  }

  static inline MClass *GetMClassAObject() {
    return ALjava_2Flang_2FObject_3B;
  }

  static inline MClass *GetMClassAClass() {
    return ALjava_2Flang_2FClass_3B;
  }

  static inline MClass *GetMClassAField() {
    return ALjava_2Flang_2Freflect_2FField_3B;
  }

  static inline MClass *GetMClassAString() {
    return ALjava_2Flang_2FString_3B;
  }

  static inline MClass *GetMClassABoolean() {
    return ALjava_2Flang_2FBoolean_3B;
  }

  static inline MClass *GetMClassAByte() {
    return ALjava_2Flang_2FByte_3B;
  }

  static inline MClass *GetMClassACharacter() {
    return ALjava_2Flang_2FCharacter_3B;
  }

  static inline MClass *GetMClassAShort() {
    return ALjava_2Flang_2FShort_3B;
  }

  static inline MClass *GetMClassAInteger() {
    return ALjava_2Flang_2FInteger_3B;
  }

  static inline MClass *GetMClassALong() {
    return ALjava_2Flang_2FLong_3B;
  }

  static inline MClass *GetMClassAFloat() {
    return ALjava_2Flang_2FFloat_3B;
  }

  static inline MClass *GetMClassADouble() {
    return ALjava_2Flang_2FDouble_3B;
  }

  static inline MClass *GetMClassAMethod() {
    return ALjava_2Flang_2Freflect_2FMethod_3B;
  }

  static inline MClass *GetMClassAAnnotation() {
    return ALjava_2Flang_2Fannotation_2FAnnotation_3B;
  }

  static inline MClass *GetMClassAConstructor() {
    return ALjava_2Flang_2Freflect_2FConstructor_3B;
  }

  static inline MClass *GetMClassAParameter() {
    return ALjava_2Flang_2Freflect_2FParameter_3B;
  }

  static inline MClass *GetMClassAAAnnotation() {
    return AALjava_2Flang_2Fannotation_2FAnnotation_3B;
  }

  static inline MClass *GetPrimitiveArrayClass(maple::Primitive::Type pType) {
    return primitiveArrayClass[pType];
  }

  static inline FieldMeta *GetFieldMetaIntegerCache() {
    return Ljava_2Flang_2FInteger_24IntegerCache_3B_cache;
  }

  static inline FieldMeta *GetFieldMetaIntegerCacheLow() {
    return Ljava_2Flang_2FInteger_24IntegerCache_3B_low;
  }

  static inline FieldMeta *GetFieldMetaIntegerCacheHigh() {
    return Ljava_2Flang_2FInteger_24IntegerCache_3B_high;
  }

  static inline FieldMeta *GetFieldMetaBooleanTrue() {
    return Ljava_2Flang_2FBoolean_3B_TRUE;
  }

  static inline FieldMeta *GetFieldMetaBooleanFalse() {
    return Ljava_2Flang_2FBoolean_3B_FALSE;
  }

  static inline FieldMeta *GetFieldMetaByteCache() {
    return Ljava_2Flang_2FByte_24ByteCache_3B_cache;
  }

  static inline FieldMeta *GetFieldMetaShortCache() {
    return Ljava_2Flang_2FShort_24ShortCache_3B_cache;
  }

  static inline FieldMeta *GetFieldMetaCharacterCache() {
    return Ljava_2Flang_2FCharacter_24CharacterCache_3B_cache;
  }

  static inline FieldMeta *GetFieldMetaLongCache() {
    return Ljava_2Flang_2FLong_24LongCache_3B_cache;
  }

  static inline uintptr_t GetMMethodClassLoaderLoadClassAddr() {
    return Ljava_2Flang_2FClassLoader_3B_LoadClass_Addr;
  }

  static inline uintptr_t GetMMethodAnnotationMemberInitAddr() {
    return Llibcore_2Freflect_2FAnnotationMember_3B_7C_3Cinit_Addr;
  }

  static inline uintptr_t GetMMethodAnnotationFactoryCreateAnnotationAddr() {
    return Llibcore_2Freflect_2FAnnotationFactory_3B_7CcreateAnnotation_Addr;
  }

  static inline uintptr_t GetMMethodBooleanValueOfAddr() {
    return Ljava_2Flang_2FBoolean_3B_ValueOf_Addr;
  }

  static inline uintptr_t GetMMethodByteValueOfAddr() {
    return Ljava_2Flang_2FByte_3B_ValueOf_Addr;
  }

  static inline uintptr_t GetMMethodCharacterValueOfAddr() {
    return Ljava_2Flang_2FCharacter_3B_ValueOf_Addr;
  }

  static inline uintptr_t GetMMethodShortValueOfAddr() {
    return Ljava_2Flang_2FShort_3B_ValueOf_Addr;
  }

  static inline uintptr_t GetMMethodIntegerValueOfAddr() {
    return Ljava_2Flang_2FInteger_3B_ValueOf_Addr;
  }

  static inline uintptr_t GetMMethodLongValueOfAddr() {
    return Ljava_2Flang_2FLong_3B_ValueOf_Addr;
  }

  static inline uintptr_t GetMMethodFloatValueOfAddr() {
    return Ljava_2Flang_2FFloat_3B_ValueOf_Addr;
  }

  static inline uintptr_t GetMMethodDoubleValueOfAddr() {
    return Ljava_2Flang_2FDouble_3B_ValueOf_Addr;
  }

  static inline size_t GetMFieldBooleanValueOffset() {
    return Ljava_2Flang_2FBoolean_3B_value_offset;
  }

  static inline size_t GetMFieldByteValueOffset() {
    return Ljava_2Flang_2FByte_3B_value_offset;
  }

  static inline size_t GetMFieldCharacterValueOffset() {
    return Ljava_2Flang_2FCharacter_3B_value_offset;
  }

  static inline size_t GetMFieldShortValueOffset() {
    return Ljava_2Flang_2FShort_3B_value_offset;
  }

  static inline size_t GetMFieldIntegerValueOffset() {
    return Ljava_2Flang_2FInteger_3B_value_offset;
  }

  static inline size_t GetMFieldLongValueOffset() {
    return Ljava_2Flang_2FLong_3B_value_offset;
  }

  static inline size_t GetMFieldFloatValueOffset() {
    return Ljava_2Flang_2FFloat_3B_value_offset;
  }

  static inline size_t GetMFieldDoubleValueOffset() {
    return Ljava_2Flang_2FDouble_3B_value_offset;
  }

  static inline size_t GetMFieldMethodHandlePTypesOffset() {
    return Ljava_2Flang_2FMethodType_3B_ptypes_offset;
  }

  static inline size_t GetMFieldMethodHandlepRTypeOffset() {
    return Ljava_2Flang_2FMethodType_3B_rtype_offset;
  }

  static void InitCache();
  static void InitArrayInterfaceVector();
  static void InitCacheClasses();
  static void InitCacheMethodHandleClasses();
  static void InitCachePrimitiveBoxClass();
  static void InitCacheMethodAddrs();
  static void InitCacheArrayClass();
  static void InitCacheFieldOffsets();
  static void InitCacheFieldMethodHandleOffsets();
  static void InitCacheExceptionClass();
  static void InitCacheFieldMetas();
  static void InitCacheClass(MClass *&cls, const char *className);
  static void InitCacheMethodAddr(uintptr_t &methodAddr, const MClass &cls,
                                  const char *methodName, const char *signatureName);
  static void InitCacheFieldOffset(size_t &fieldOffset, const MClass &cls, const char *fieldName);
  static void InitCacheFieldMeta(FieldMeta *&fieldMeta, const MClass &cls, const char *fieldName);
  static MClass *GetCacheArrayClass(const MClass &componentClass);
  static MClass *GetWellKnowClassWithFlag(uint8_t classFlag, const MClass &caller, const char *className);
  static MethodMeta *GetStringFactoryConstructor(const MethodMeta &stringConstructor);

 private:
  static constexpr uint8_t kMaxPrimitiveSize = 9;
  static constexpr uint8_t kMaxFrameworksSize = 7;
  static constexpr uint32_t kCacheArrayClassSize = 16;
  static uint32_t currentCacheArrayClassIndex;

  static MClass *primitiveClassZ;
  static MClass *primitiveClassB;
  static MClass *primitiveClassS;
  static MClass *primitiveClassC;
  static MClass *primitiveClassI;
  static MClass *primitiveClassJ;
  static MClass *primitiveClassF;
  static MClass *primitiveClassD;
  static MClass *primitiveClassV;

  static MClass *primitiveClassAZ;
  static MClass *primitiveClassAB;
  static MClass *primitiveClassAS;
  static MClass *primitiveClassAC;
  static MClass *primitiveClassAI;
  static MClass *primitiveClassAJ;
  static MClass *primitiveClassAF;
  static MClass *primitiveClassAD;

  static MClass *primitiveClassAAZ;
  static MClass *primitiveClassAAB;
  static MClass *primitiveClassAAS;
  static MClass *primitiveClassAAC;
  static MClass *primitiveClassAAI;
  static MClass *primitiveClassAAJ;
  static MClass *primitiveClassAAF;
  static MClass *primitiveClassAAD;

  static MClass *Ljava_2Flang_2FVoid_3B;
  static MClass *Ljava_2Flang_2FBoolean_3B;
  static MClass *Ljava_2Flang_2FByte_3B;
  static MClass *Ljava_2Flang_2FCharacter_3B;
  static MClass *Ljava_2Flang_2FShort_3B;
  static MClass *Ljava_2Flang_2FInteger_3B;
  static MClass *Ljava_2Flang_2FLong_3B;
  static MClass *Ljava_2Flang_2FFloat_3B;
  static MClass *Ljava_2Flang_2FDouble_3B;
  static MClass *Ljava_2Flang_2FNumber_3B;

  static MClass *Ljava_2Flang_2FObject_3B;
  static MClass *Ljava_2Flang_2FClass_3B;
  static MClass *Ljava_2Flang_2FClassLoader_3B;
  static MClass *Ljava_2Flang_2Freflect_2FField_3B;
  static MClass *Ljava_2Flang_2Freflect_2FConstructor_3B;
  static MClass *Ljava_2Flang_2Freflect_2FMethod_3B;
  static MClass *Ljava_2Flang_2Freflect_2FProxy_3B;
  static MClass *Ljava_2Flang_2Freflect_2FParameter_3B;
  static MClass *Llibcore_2Freflect_2FGenericSignatureParser_3B;
  static MClass *Ljava_2Flang_2Fref_2FReference_3B;
  static MClass *Ljava_2Flang_2Fref_2FFinalizerReference_3B;

  static MClass *Ljava_2Flang_2FCloneable_3B;
  static MClass *Ljava_2Fio_2FSerializable_3B;

  static MClass *Ljava_2Flang_2FString_3B;
  static MClass *Ljava_2Flang_2FStringFactory_3B;

  static MClass *Ljava_2Flang_2FError_3B;
  static MClass *Ljava_2Flang_2FThrowable_3B;
  static MClass *Ljava_2Flang_2FArithmeticException_3B;
  static MClass *Ljava_2Flang_2FInterruptedException_3B;
  static MClass *Ljava_2Flang_2FClassCastException_3B;
  static MClass *Ljava_2Flang_2FUnsatisfiedLinkError_3B;
  static MClass *Ljava_2Flang_2FStringIndexOutOfBoundsException_3B;
  static MClass *Ljava_2Flang_2FNoClassDefFoundError_3B;
  static MClass *Ljava_2Flang_2FNoSuchMethodError_3B;
  static MClass *Ljava_2Flang_2FNoSuchFieldError_3B;
  static MClass *Ljava_2Flang_2FVerifyError_3B;
  static MClass *Ljava_2Flang_2FExceptionInInitializerError_3B;
  static MClass *Ljava_2Flang_2FRuntimeException_3B;
  static MClass *Ljava_2Flang_2FSecurityException_3B;
  static MClass *Ljava_2Flang_2Freflect_2FUndeclaredThrowableException_3B;
  static MClass *Ljava_2Flang_2FArrayStoreException_3B;
  static MClass *Ljava_2Flang_2FArrayIndexOutOfBoundsException_3B;
  static MClass *Ljava_2Flang_2FNullPointerException_3B;

  static MClass *Ljava_2Flang_2FEnum_3B;
  static MClass *Ljava_2Flang_2Fannotation_2FAnnotation_3B;
  static MClass *Llibcore_2Freflect_2FAnnotationMember_3B;
  static MClass *Llibcore_2Freflect_2FAnnotationFactory_3B;
  static MClass *Ldalvik_2Fsystem_2FDelegateLastClassLoader_3B;
  static MClass *Ldalvik_2Fsystem_2FPathClassLoader_3B;
  static MClass *Ldalvik_2Fsystem_2FDexClassLoader_3B;
  static MClass *Ldalvik_2Fsystem_2FInMemoryDexClassLoader_3B;

  static MClass *Ljava_2Flang_2Finvoke_2FMethodType_3B;
  static MClass *Ljava_2Flang_2Finvoke_2FMethodHandle_3B;

#ifdef __OPENJDK__
  static MClass *Ljava_2Futil_2FHashMap_3B;
  static MClass *Lsun_2Freflect_2Fannotation_2FAnnotationParser_3B;

#endif  // __OPENJDK__

  static MClass *Ljava_2Flang_2FInteger_24IntegerCache_3B;
  static MClass *Ljava_2Flang_2FByte_24ByteCache_3B;
  static MClass *Ljava_2Flang_2FShort_24ShortCache_3B;
  static MClass *Ljava_2Flang_2FCharacter_24CharacterCache_3B;
  static MClass *Ljava_2Flang_2FLong_24LongCache_3B;

  static MClass *Ljava_2Flang_2Finvoke_2FInvokeData_24BindToData_3B;
  static MClass *Ljava_2Flang_2Finvoke_2FInvokeData_24DropArgumentsData_3B;
  static MClass *Ljava_2Flang_2Finvoke_2FInvokeData_24FilterReturnValueData_3B;
  static MClass *Ljava_2Flang_2Finvoke_2FInvokeData_24PermuteArgumentsData_3B;
  static size_t Ljava_2Flang_2FMethodHandle_3B_dataArray_offset;
  static size_t Ljava_2Flang_2FMethodHandle_3B_metaArray_offset;
  static size_t Ljava_2Flang_2FMethodHandle_3B_typeArray_offset;
  static size_t Ljava_2Flang_2FMethodHandle_3B_opArray_offset;
  static size_t Ljava_2Flang_2FMethodHandle_3B_index_offset;
  static size_t Ljava_2Flang_2FBindToData_3B_receiver_offset;
  static size_t Ljava_2Flang_2FDropArgumentsData_3B_numDropped_offset;
  static size_t Ljava_2Flang_2FDropArgumentsData_3B_startPos_offset;
  static size_t Ljava_2Flang_2FFilterReturnValueData_3B_target_offset;
  static size_t Ljava_2Flang_2FFilterReturnValueData_3B_filter_offset;
  static size_t Ljava_2Flang_2FPermuteArgumentsData_3B_target_offset;
  static size_t Ljava_2Flang_2FPermuteArgumentsData_3B_reorder_offset;

  static MClass *Ldalvik_2Fsystem_2FEmulatedStackFrame_3B;
  static size_t Ljava_2Flang_2FMethodHandle_3B_artFieldOrMethod_offset;
  static size_t Ljava_2Flang_2FMethodHandle_3B_handleKind_offset;
  static size_t Ljava_2Flang_2FMethodHandle_3B_nominalType_offset;
  static size_t Ljava_2Flang_2FMethodHandle_3B_type_offset;
  static size_t Ldalvik_2Fsystem_2FEmulatedStackFrame_3B_callsiteType_offset;
  static size_t Ldalvik_2Fsystem_2FEmulatedStackFrame_3B_references_offset;
  static size_t Ldalvik_2Fsystem_2FEmulatedStackFrame_3B_stackFrame_offset;
  static size_t Ldalvik_2Fsystem_2FEmulatedStackFrame_3B_type_offset;

  static MClass *ALjava_2Flang_2FObject_3B;
  static MClass *ALjava_2Flang_2FClass_3B;
  static MClass *ALjava_2Flang_2Freflect_2FField_3B;
  static MClass *ALjava_2Flang_2Freflect_2FMethod_3B;
  static MClass *ALjava_2Flang_2Fannotation_2FAnnotation_3B;
  static MClass *ALjava_2Flang_2Freflect_2FConstructor_3B;
  static MClass *ALjava_2Flang_2Freflect_2FParameter_3B;
  static MClass *ALjava_2Flang_2FString_3B;
  static MClass *ALjava_2Flang_2FBoolean_3B;
  static MClass *ALjava_2Flang_2FByte_3B;
  static MClass *ALjava_2Flang_2FCharacter_3B;
  static MClass *ALjava_2Flang_2FShort_3B;
  static MClass *ALjava_2Flang_2FInteger_3B;
  static MClass *ALjava_2Flang_2FLong_3B;
  static MClass *ALjava_2Flang_2FFloat_3B;
  static MClass *ALjava_2Flang_2FDouble_3B;

  static MClass *AALjava_2Flang_2Fannotation_2FAnnotation_3B;

  static MClass *primitiveArrayClass[];
  static MClass *cacheArrayClasses[];
  static MClass *arrayFrameWorksClasses[];

  static FieldMeta *Ljava_2Flang_2FBoolean_3B_TRUE;
  static FieldMeta *Ljava_2Flang_2FBoolean_3B_FALSE;
  static FieldMeta *Ljava_2Flang_2FByte_24ByteCache_3B_cache;
  static FieldMeta *Ljava_2Flang_2FShort_24ShortCache_3B_cache;
  static FieldMeta *Ljava_2Flang_2FCharacter_24CharacterCache_3B_cache;
  static FieldMeta *Ljava_2Flang_2FLong_24LongCache_3B_cache;
  static FieldMeta *Ljava_2Flang_2FInteger_24IntegerCache_3B_cache;
  static FieldMeta *Ljava_2Flang_2FInteger_24IntegerCache_3B_low;
  static FieldMeta *Ljava_2Flang_2FInteger_24IntegerCache_3B_high;

  // method loadClass address
  static uintptr_t Ljava_2Flang_2FClassLoader_3B_LoadClass_Addr;

  // Annotation method address
  static uintptr_t Llibcore_2Freflect_2FAnnotationMember_3B_7C_3Cinit_Addr;
  static uintptr_t Llibcore_2Freflect_2FAnnotationFactory_3B_7CcreateAnnotation_Addr;

  // method ValueOf address
  static uintptr_t Ljava_2Flang_2FBoolean_3B_ValueOf_Addr;
  static uintptr_t Ljava_2Flang_2FByte_3B_ValueOf_Addr;
  static uintptr_t Ljava_2Flang_2FCharacter_3B_ValueOf_Addr;
  static uintptr_t Ljava_2Flang_2FShort_3B_ValueOf_Addr;
  static uintptr_t Ljava_2Flang_2FInteger_3B_ValueOf_Addr;
  static uintptr_t Ljava_2Flang_2FLong_3B_ValueOf_Addr;
  static uintptr_t Ljava_2Flang_2FFloat_3B_ValueOf_Addr;
  static uintptr_t Ljava_2Flang_2FDouble_3B_ValueOf_Addr;

  // field value offset
  static size_t Ljava_2Flang_2FBoolean_3B_value_offset;
  static size_t Ljava_2Flang_2FByte_3B_value_offset;
  static size_t Ljava_2Flang_2FCharacter_3B_value_offset;
  static size_t Ljava_2Flang_2FShort_3B_value_offset;
  static size_t Ljava_2Flang_2FInteger_3B_value_offset;
  static size_t Ljava_2Flang_2FLong_3B_value_offset;
  static size_t Ljava_2Flang_2FFloat_3B_value_offset;
  static size_t Ljava_2Flang_2FDouble_3B_value_offset;
  static size_t Ljava_2Flang_2FMethodType_3B_ptypes_offset;
  static size_t Ljava_2Flang_2FMethodType_3B_rtype_offset;
};
} // namespace maplert
#endif // MRT_WELL_KNOWN_H_
