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
#include "mrt_well_known.h"
#include "mrt_classloader_api.h"
#include "chelper.h"
#include "mclass_inline.h"
#include "fieldmeta_inline.h"
#include "methodmeta_inline.h"
#include "mrt_primitive_api.h"
#include "mrt_array_api.h"
namespace maplert {
// Init Ljava_2Flang_2FString_3B direct
// mpl-linker use it when load const jstring
extern "C" {
  extern void *MRT_CLASSINFO(Ljava_2Flang_2FString_3B);
  extern void *MRT_CLASSINFO(Ljava_2Flang_2FClass_3B);
}

MClass *WellKnown::Ljava_2Flang_2FString_3B = reinterpret_cast<MClass*>(&MRT_CLASSINFO(Ljava_2Flang_2FString_3B));
MClass *WellKnown::Ljava_2Flang_2FClass_3B = reinterpret_cast<MClass*>(&MRT_CLASSINFO(Ljava_2Flang_2FClass_3B));

MClass *WellKnown::primitiveClassZ;
MClass *WellKnown::primitiveClassB;
MClass *WellKnown::primitiveClassS;
MClass *WellKnown::primitiveClassC;
MClass *WellKnown::primitiveClassI;
MClass *WellKnown::primitiveClassJ;
MClass *WellKnown::primitiveClassF;
MClass *WellKnown::primitiveClassD;
MClass *WellKnown::primitiveClassV;

MClass *WellKnown::primitiveClassAZ;
MClass *WellKnown::primitiveClassAB;
MClass *WellKnown::primitiveClassAS;
MClass *WellKnown::primitiveClassAC;
MClass *WellKnown::primitiveClassAI;
MClass *WellKnown::primitiveClassAJ;
MClass *WellKnown::primitiveClassAF;
MClass *WellKnown::primitiveClassAD;

MClass *WellKnown::primitiveClassAAZ;
MClass *WellKnown::primitiveClassAAB;
MClass *WellKnown::primitiveClassAAS;
MClass *WellKnown::primitiveClassAAC;
MClass *WellKnown::primitiveClassAAI;
MClass *WellKnown::primitiveClassAAJ;
MClass *WellKnown::primitiveClassAAF;
MClass *WellKnown::primitiveClassAAD;

MClass *WellKnown::Ljava_2Flang_2FVoid_3B;
MClass *WellKnown::Ljava_2Flang_2FBoolean_3B;
MClass *WellKnown::Ljava_2Flang_2FByte_3B;
MClass *WellKnown::Ljava_2Flang_2FCharacter_3B;
MClass *WellKnown::Ljava_2Flang_2FShort_3B;
MClass *WellKnown::Ljava_2Flang_2FInteger_3B;
MClass *WellKnown::Ljava_2Flang_2FLong_3B;
MClass *WellKnown::Ljava_2Flang_2FFloat_3B;
MClass *WellKnown::Ljava_2Flang_2FDouble_3B;
MClass *WellKnown::Ljava_2Flang_2FNumber_3B;

MClass *WellKnown::Ljava_2Flang_2FObject_3B;
MClass *WellKnown::Ljava_2Flang_2FClassLoader_3B;
MClass *WellKnown::Ljava_2Flang_2Freflect_2FField_3B;
MClass *WellKnown::Ljava_2Flang_2Freflect_2FConstructor_3B;
MClass *WellKnown::Ljava_2Flang_2Freflect_2FMethod_3B;
MClass *WellKnown::Ljava_2Flang_2Freflect_2FProxy_3B;
MClass *WellKnown::Ljava_2Flang_2Freflect_2FParameter_3B;
MClass *WellKnown::Llibcore_2Freflect_2FGenericSignatureParser_3B;
MClass *WellKnown::Ljava_2Flang_2Fref_2FReference_3B;
MClass *WellKnown::Ljava_2Flang_2Fref_2FFinalizerReference_3B;

MClass *WellKnown::Ljava_2Flang_2FCloneable_3B;
MClass *WellKnown::Ljava_2Fio_2FSerializable_3B;

MClass *WellKnown::Ljava_2Flang_2FStringFactory_3B;
MClass *WellKnown::Ljava_2Flang_2FError_3B;
MClass *WellKnown::Ljava_2Flang_2FThrowable_3B;
MClass *WellKnown::Ljava_2Flang_2FArithmeticException_3B;
MClass *WellKnown::Ljava_2Flang_2FInterruptedException_3B;
MClass *WellKnown::Ljava_2Flang_2FClassCastException_3B;
MClass *WellKnown::Ljava_2Flang_2FUnsatisfiedLinkError_3B;
MClass *WellKnown::Ljava_2Flang_2FStringIndexOutOfBoundsException_3B;
MClass *WellKnown::Ljava_2Flang_2FNoClassDefFoundError_3B;
MClass *WellKnown::Ljava_2Flang_2FNoSuchMethodError_3B;
MClass *WellKnown::Ljava_2Flang_2FNoSuchFieldError_3B;
MClass *WellKnown::Ljava_2Flang_2FVerifyError_3B;
MClass *WellKnown::Ljava_2Flang_2FExceptionInInitializerError_3B;
MClass *WellKnown::Ljava_2Flang_2FRuntimeException_3B;
MClass *WellKnown::Ljava_2Flang_2FSecurityException_3B;
MClass *WellKnown::Ljava_2Flang_2Freflect_2FUndeclaredThrowableException_3B;
MClass *WellKnown::Ljava_2Flang_2FArrayStoreException_3B;
MClass *WellKnown::Ljava_2Flang_2FArrayIndexOutOfBoundsException_3B;
MClass *WellKnown::Ljava_2Flang_2FNullPointerException_3B;

MClass *WellKnown::Ljava_2Flang_2FEnum_3B;
MClass *WellKnown::Ljava_2Flang_2Fannotation_2FAnnotation_3B;
MClass *WellKnown::Llibcore_2Freflect_2FAnnotationMember_3B;
MClass *WellKnown::Llibcore_2Freflect_2FAnnotationFactory_3B;
MClass *WellKnown::Ldalvik_2Fsystem_2FDelegateLastClassLoader_3B;
MClass *WellKnown::Ldalvik_2Fsystem_2FPathClassLoader_3B;
MClass *WellKnown::Ldalvik_2Fsystem_2FDexClassLoader_3B;
MClass *WellKnown::Ldalvik_2Fsystem_2FInMemoryDexClassLoader_3B;

MClass *WellKnown::Ljava_2Flang_2Finvoke_2FMethodType_3B;
MClass *WellKnown::Ljava_2Flang_2Finvoke_2FMethodHandle_3B;

#ifdef __OPENJDK__
MClass *WellKnown::Ljava_2Futil_2FHashMap_3B;
MClass *WellKnown::Lsun_2Freflect_2Fannotation_2FAnnotationParser_3B;
#endif // __OPENJDK__

MClass *WellKnown::Ljava_2Flang_2FInteger_24IntegerCache_3B;
MClass *WellKnown::Ljava_2Flang_2FByte_24ByteCache_3B;
MClass *WellKnown::Ljava_2Flang_2FShort_24ShortCache_3B;
MClass *WellKnown::Ljava_2Flang_2FCharacter_24CharacterCache_3B;
MClass *WellKnown::Ljava_2Flang_2FLong_24LongCache_3B;

MClass *WellKnown::Ljava_2Flang_2Finvoke_2FInvokeData_24BindToData_3B;
MClass *WellKnown::Ljava_2Flang_2Finvoke_2FInvokeData_24DropArgumentsData_3B;
MClass *WellKnown::Ljava_2Flang_2Finvoke_2FInvokeData_24FilterReturnValueData_3B;
MClass *WellKnown::Ljava_2Flang_2Finvoke_2FInvokeData_24PermuteArgumentsData_3B;
size_t WellKnown::Ljava_2Flang_2FMethodHandle_3B_dataArray_offset;
size_t WellKnown::Ljava_2Flang_2FMethodHandle_3B_metaArray_offset;
size_t WellKnown::Ljava_2Flang_2FMethodHandle_3B_typeArray_offset;
size_t WellKnown::Ljava_2Flang_2FMethodHandle_3B_opArray_offset;
size_t WellKnown::Ljava_2Flang_2FMethodHandle_3B_index_offset;
size_t WellKnown::Ljava_2Flang_2FBindToData_3B_receiver_offset;
size_t WellKnown::Ljava_2Flang_2FDropArgumentsData_3B_numDropped_offset;
size_t WellKnown::Ljava_2Flang_2FDropArgumentsData_3B_startPos_offset;
size_t WellKnown::Ljava_2Flang_2FFilterReturnValueData_3B_target_offset;
size_t WellKnown::Ljava_2Flang_2FFilterReturnValueData_3B_filter_offset;
size_t WellKnown::Ljava_2Flang_2FPermuteArgumentsData_3B_target_offset;
size_t WellKnown::Ljava_2Flang_2FPermuteArgumentsData_3B_reorder_offset;

MClass *WellKnown::Ldalvik_2Fsystem_2FEmulatedStackFrame_3B;
size_t WellKnown::Ljava_2Flang_2FMethodHandle_3B_artFieldOrMethod_offset;
size_t WellKnown::Ljava_2Flang_2FMethodHandle_3B_handleKind_offset;
size_t WellKnown::Ljava_2Flang_2FMethodHandle_3B_nominalType_offset;
size_t WellKnown::Ljava_2Flang_2FMethodHandle_3B_type_offset;
size_t WellKnown::Ldalvik_2Fsystem_2FEmulatedStackFrame_3B_callsiteType_offset;
size_t WellKnown::Ldalvik_2Fsystem_2FEmulatedStackFrame_3B_references_offset;
size_t WellKnown::Ldalvik_2Fsystem_2FEmulatedStackFrame_3B_stackFrame_offset;
size_t WellKnown::Ldalvik_2Fsystem_2FEmulatedStackFrame_3B_type_offset;

MClass *WellKnown::ALjava_2Flang_2FObject_3B;
MClass *WellKnown::ALjava_2Flang_2FClass_3B;
MClass *WellKnown::ALjava_2Flang_2Freflect_2FField_3B;
MClass *WellKnown::ALjava_2Flang_2Freflect_2FMethod_3B;
MClass *WellKnown::ALjava_2Flang_2Fannotation_2FAnnotation_3B;
MClass *WellKnown::ALjava_2Flang_2Freflect_2FConstructor_3B;
MClass *WellKnown::ALjava_2Flang_2Freflect_2FParameter_3B;
MClass *WellKnown::ALjava_2Flang_2FString_3B;
MClass *WellKnown::ALjava_2Flang_2FBoolean_3B;
MClass *WellKnown::ALjava_2Flang_2FByte_3B;
MClass *WellKnown::ALjava_2Flang_2FCharacter_3B;
MClass *WellKnown::ALjava_2Flang_2FShort_3B;
MClass *WellKnown::ALjava_2Flang_2FInteger_3B;
MClass *WellKnown::ALjava_2Flang_2FLong_3B;
MClass *WellKnown::ALjava_2Flang_2FFloat_3B;
MClass *WellKnown::ALjava_2Flang_2FDouble_3B;

MClass *WellKnown::AALjava_2Flang_2Fannotation_2FAnnotation_3B;

FieldMeta *WellKnown::Ljava_2Flang_2FBoolean_3B_TRUE;
FieldMeta *WellKnown::Ljava_2Flang_2FBoolean_3B_FALSE;
FieldMeta *WellKnown::Ljava_2Flang_2FByte_24ByteCache_3B_cache;
FieldMeta *WellKnown::Ljava_2Flang_2FShort_24ShortCache_3B_cache;
FieldMeta *WellKnown::Ljava_2Flang_2FCharacter_24CharacterCache_3B_cache;
FieldMeta *WellKnown::Ljava_2Flang_2FLong_24LongCache_3B_cache;
FieldMeta *WellKnown::Ljava_2Flang_2FInteger_24IntegerCache_3B_cache;
FieldMeta *WellKnown::Ljava_2Flang_2FInteger_24IntegerCache_3B_low;
FieldMeta *WellKnown::Ljava_2Flang_2FInteger_24IntegerCache_3B_high;

uintptr_t WellKnown::Ljava_2Flang_2FClassLoader_3B_LoadClass_Addr;
uintptr_t WellKnown::Llibcore_2Freflect_2FAnnotationMember_3B_7C_3Cinit_Addr;
uintptr_t WellKnown::Llibcore_2Freflect_2FAnnotationFactory_3B_7CcreateAnnotation_Addr;
uintptr_t WellKnown::Ljava_2Flang_2FBoolean_3B_ValueOf_Addr;
uintptr_t WellKnown::Ljava_2Flang_2FByte_3B_ValueOf_Addr;
uintptr_t WellKnown::Ljava_2Flang_2FCharacter_3B_ValueOf_Addr;
uintptr_t WellKnown::Ljava_2Flang_2FShort_3B_ValueOf_Addr;
uintptr_t WellKnown::Ljava_2Flang_2FInteger_3B_ValueOf_Addr;
uintptr_t WellKnown::Ljava_2Flang_2FLong_3B_ValueOf_Addr;
uintptr_t WellKnown::Ljava_2Flang_2FFloat_3B_ValueOf_Addr;
uintptr_t WellKnown::Ljava_2Flang_2FDouble_3B_ValueOf_Addr;

size_t WellKnown::Ljava_2Flang_2FBoolean_3B_value_offset;
size_t WellKnown::Ljava_2Flang_2FByte_3B_value_offset;
size_t WellKnown::Ljava_2Flang_2FCharacter_3B_value_offset;
size_t WellKnown::Ljava_2Flang_2FShort_3B_value_offset;
size_t WellKnown::Ljava_2Flang_2FInteger_3B_value_offset;
size_t WellKnown::Ljava_2Flang_2FLong_3B_value_offset;
size_t WellKnown::Ljava_2Flang_2FFloat_3B_value_offset;
size_t WellKnown::Ljava_2Flang_2FDouble_3B_value_offset;
size_t WellKnown::Ljava_2Flang_2FMethodType_3B_ptypes_offset;
size_t WellKnown::Ljava_2Flang_2FMethodType_3B_rtype_offset;

size_t WellKnown::kReferenceReferentOffset;
size_t WellKnown::kReferenceQueueOffset;
size_t WellKnown::kReferencePendingnextOffset;
size_t WellKnown::kFinalizereferenceZombieOffset;
uint32_t WellKnown::currentCacheArrayClassIndex = 0;

std::vector<MClass*> WellKnown::arrayInterface;

MClass *WellKnown::primitiveArrayClass[kMaxPrimitiveSize];

MClass *WellKnown::cacheArrayClasses[kCacheArrayClassSize];

// frameworks
// ALandroid_2Fcontent_2Fpm_2FPackageParser_24ActivityIntentInfo_3B         0x01
// ALandroid_2Fcontent_2Fpm_2FSignature_3B                                  0x02
// ALcom_2Fandroid_2Fserver_2Fam_2FBroadcastFilter_3B                       0x03
// ALandroid_2Fcontent_2Fpm_2FPackageParser_24ServiceIntentInfo_3B          0x04
// ALandroid_2Fcontent_2FIntent_3B                                          0x05
// ALcom_2Fandroid_2Finternal_2Fos_2FBatteryStatsImpl_24StopwatchTimer_3B   0x06
MClass *WellKnown::arrayFrameWorksClasses[kMaxPrimitiveSize] = { nullptr };

MClass *WellKnown::GetCacheArrayClass(const MClass &componentClass) {
  for (uint32_t i = 0; i < kCacheArrayClassSize; ++i) {
    MClass *arrayClass = cacheArrayClasses[i];
    if (arrayClass != nullptr && arrayClass->GetComponentClass() == &componentClass) {
      return arrayClass;
    }
  }

  std::string arrayName("[");
  arrayName.append(componentClass.GetName());
  MClass *arrayClass = MClass::GetClassFromDescriptor(&componentClass, arrayName.c_str());
  CHECK(arrayClass != nullptr) << "Get array class fail." << maple::endl;
  uint32_t index = currentCacheArrayClassIndex;
  cacheArrayClasses[index] = arrayClass;
  currentCacheArrayClassIndex = (index + 1) % kCacheArrayClassSize;
  return arrayClass;
}

MClass *WellKnown::GetWellKnowClassWithFlag(uint8_t classFlag, const MClass &caller, const char *className) {
  MClass *arrayClass = nullptr;
  if (classFlag < kMaxFrameworksSize) {
    arrayClass = arrayFrameWorksClasses[classFlag];
    if (arrayClass == nullptr) {
      arrayClass = MClass::JniCast(MRT_GetClass(caller.AsJclass(), className));
      arrayFrameWorksClasses[classFlag] = arrayClass;
    }
  }
  return arrayClass;
}

void WellKnown::InitArrayInterfaceVector() {
  arrayInterface.push_back(Ljava_2Flang_2FCloneable_3B);
  arrayInterface.push_back(Ljava_2Fio_2FSerializable_3B);
}

void WellKnown::InitCacheClass(MClass *&cls, const char *className) {
  // boot class loader
  cls = MClass::JniCast(MRT_GetClassByContextClass(nullptr, className));
  CHECK(cls != nullptr) << "InitCacheClass fail, not find class: " << className << maple::endl;
}

void WellKnown::InitCacheMethodAddr(uintptr_t &methodAddr, const MClass &cls, const char *methodName,
                                    const char *signatureName) {
  MethodMeta *methodMeta = cls.GetDeclaredMethod(methodName, signatureName);
  if (methodMeta == nullptr) {
    LOG(FATAL) << "InitCacheMethodAddr, init fail, " << cls.GetName() << ", " <<
        methodName << signatureName << maple::endl;
    return;
  }
  methodAddr = methodMeta->GetFuncAddress();
}

void WellKnown::InitCacheFieldOffset(size_t &fieldOffset, const MClass &cls, const char *fieldName) {
  FieldMeta *fieldMeta = cls.GetDeclaredField(fieldName);
  if (fieldMeta == nullptr) {
    LOG(FATAL) << "InitCacheFieldOffset, init fail, " << cls.GetName() << ", " << fieldName << maple::endl;
    return;
  }
  fieldOffset = fieldMeta->GetOffset();
}

void WellKnown::InitCacheFieldMeta(FieldMeta *&fieldMeta, const MClass &cls, const char *fieldName) {
  fieldMeta = cls.GetDeclaredField(fieldName);
  if (fieldMeta == nullptr) {
    LOG(FATAL) << "InitCacheFieldOffset, init fail, " << cls.GetName() << ", " << fieldName << maple::endl;
    return;
  }
}

void WellKnown::InitCachePrimitiveBoxClass() {
  InitCacheClass(primitiveClassZ, "Z");
  InitCacheClass(primitiveClassB, "B");
  InitCacheClass(primitiveClassS, "S");
  InitCacheClass(primitiveClassC, "C");
  InitCacheClass(primitiveClassI, "I");
  InitCacheClass(primitiveClassJ, "J");
  InitCacheClass(primitiveClassF, "F");
  InitCacheClass(primitiveClassD, "D");
  InitCacheClass(primitiveClassV, "V");

  InitCacheClass(primitiveClassAZ, "[Z");
  InitCacheClass(primitiveClassAB, "[B");
  InitCacheClass(primitiveClassAS, "[S");
  InitCacheClass(primitiveClassAC, "[C");
  InitCacheClass(primitiveClassAI, "[I");
  InitCacheClass(primitiveClassAJ, "[J");
  InitCacheClass(primitiveClassAF, "[F");
  InitCacheClass(primitiveClassAD, "[D");

  InitCacheClass(primitiveClassAAZ, "[[Z");
  InitCacheClass(primitiveClassAAB, "[[B");
  InitCacheClass(primitiveClassAAS, "[[S");
  InitCacheClass(primitiveClassAAC, "[[C");
  InitCacheClass(primitiveClassAAI, "[[I");
  InitCacheClass(primitiveClassAAJ, "[[J");
  InitCacheClass(primitiveClassAAF, "[[F");
  InitCacheClass(primitiveClassAAD, "[[D");

  InitCacheClass(Ljava_2Flang_2FVoid_3B, "Ljava/lang/Void;");
  InitCacheClass(Ljava_2Flang_2FBoolean_3B, "Ljava/lang/Boolean;");
  InitCacheClass(Ljava_2Flang_2FByte_3B, "Ljava/lang/Byte;");
  InitCacheClass(Ljava_2Flang_2FCharacter_3B, "Ljava/lang/Character;");
  InitCacheClass(Ljava_2Flang_2FShort_3B, "Ljava/lang/Short;");
  InitCacheClass(Ljava_2Flang_2FInteger_3B, "Ljava/lang/Integer;");
  InitCacheClass(Ljava_2Flang_2FLong_3B, "Ljava/lang/Long;");
  InitCacheClass(Ljava_2Flang_2FFloat_3B, "Ljava/lang/Float;");
  InitCacheClass(Ljava_2Flang_2FDouble_3B, "Ljava/lang/Double;");
  InitCacheClass(Ljava_2Flang_2FNumber_3B, "Ljava/lang/Number;");
}

void WellKnown::InitCacheArrayClass() {
  InitCacheClass(ALjava_2Flang_2FObject_3B, "[Ljava/lang/Object;");
  InitCacheClass(ALjava_2Flang_2FClass_3B, "[Ljava/lang/Class;");
  InitCacheClass(ALjava_2Flang_2Freflect_2FField_3B, "[Ljava/lang/reflect/Field;");
  InitCacheClass(ALjava_2Flang_2Freflect_2FMethod_3B, "[Ljava/lang/reflect/Method;");
  InitCacheClass(ALjava_2Flang_2Fannotation_2FAnnotation_3B, "[Ljava/lang/annotation/Annotation;");
  InitCacheClass(ALjava_2Flang_2Freflect_2FConstructor_3B, "[Ljava/lang/reflect/Constructor;");
  InitCacheClass(ALjava_2Flang_2Freflect_2FParameter_3B, "[Ljava/lang/reflect/Parameter;");
  InitCacheClass(ALjava_2Flang_2FString_3B, "[Ljava/lang/String;");
  InitCacheClass(ALjava_2Flang_2FBoolean_3B, "[Ljava/lang/Boolean;");
  InitCacheClass(ALjava_2Flang_2FByte_3B, "[Ljava/lang/Byte;");
  InitCacheClass(ALjava_2Flang_2FCharacter_3B, "[Ljava/lang/Character;");
  InitCacheClass(ALjava_2Flang_2FShort_3B, "[Ljava/lang/Short;");
  InitCacheClass(ALjava_2Flang_2FInteger_3B, "[Ljava/lang/Integer;");
  InitCacheClass(ALjava_2Flang_2FLong_3B, "[Ljava/lang/Long;");
  InitCacheClass(ALjava_2Flang_2FFloat_3B, "[Ljava/lang/Float;");
  InitCacheClass(ALjava_2Flang_2FDouble_3B, "[Ljava/lang/Double;");
  InitCacheClass(AALjava_2Flang_2Fannotation_2FAnnotation_3B, "[[Ljava/lang/annotation/Annotation;");
}

void WellKnown::InitCacheExceptionClass() {
  InitCacheClass(Ljava_2Flang_2FError_3B, "Ljava/lang/Error;");
  InitCacheClass(Ljava_2Flang_2FThrowable_3B, "Ljava/lang/Throwable;");
  InitCacheClass(Ljava_2Flang_2FArithmeticException_3B, "Ljava/lang/ArithmeticException;");
  InitCacheClass(Ljava_2Flang_2FInterruptedException_3B, "Ljava/lang/InterruptedException;");
  InitCacheClass(Ljava_2Flang_2FClassCastException_3B, "Ljava/lang/ClassCastException;");
  InitCacheClass(Ljava_2Flang_2FUnsatisfiedLinkError_3B, "Ljava/lang/UnsatisfiedLinkError;");
  InitCacheClass(Ljava_2Flang_2FStringIndexOutOfBoundsException_3B, "Ljava/lang/StringIndexOutOfBoundsException;");
  InitCacheClass(Ljava_2Flang_2FNoClassDefFoundError_3B, "Ljava/lang/NoClassDefFoundError;");
  InitCacheClass(Ljava_2Flang_2FNoSuchMethodError_3B, "Ljava/lang/NoSuchMethodError;");
  InitCacheClass(Ljava_2Flang_2FNoSuchFieldError_3B, "Ljava/lang/NoSuchFieldError;");
  InitCacheClass(Ljava_2Flang_2FVerifyError_3B, "Ljava/lang/VerifyError;");
  InitCacheClass(Ljava_2Flang_2FExceptionInInitializerError_3B, "Ljava/lang/ExceptionInInitializerError;");
  InitCacheClass(Ljava_2Flang_2FRuntimeException_3B, "Ljava/lang/RuntimeException;");
  InitCacheClass(Ljava_2Flang_2FSecurityException_3B, "Ljava/lang/SecurityException;");
  InitCacheClass(Ljava_2Flang_2Freflect_2FUndeclaredThrowableException_3B,
                 "Ljava/lang/reflect/UndeclaredThrowableException;");
  InitCacheClass(Ljava_2Flang_2FArrayStoreException_3B, "Ljava/lang/ArrayStoreException;");
  InitCacheClass(Ljava_2Flang_2FArrayIndexOutOfBoundsException_3B, "Ljava/lang/ArrayIndexOutOfBoundsException;");
  InitCacheClass(Ljava_2Flang_2FNullPointerException_3B, "Ljava/lang/NullPointerException;");
}

void WellKnown::InitCacheMethodAddrs() {
  InitCacheMethodAddr(Ljava_2Flang_2FClassLoader_3B_LoadClass_Addr, *Ljava_2Flang_2FClassLoader_3B,
                      "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
#ifndef __OPENJDK__
  InitCacheMethodAddr(Llibcore_2Freflect_2FAnnotationMember_3B_7C_3Cinit_Addr,
                      *Llibcore_2Freflect_2FAnnotationMember_3B,
                      "<init>", "(Ljava/lang/String;Ljava/lang/Object;Ljava/lang/Class;Ljava/lang/reflect/Method;)V");
  InitCacheMethodAddr(Llibcore_2Freflect_2FAnnotationFactory_3B_7CcreateAnnotation_Addr,
                      *Llibcore_2Freflect_2FAnnotationFactory_3B, "createAnnotation",
                      "(Ljava/lang/Class;[Llibcore/reflect/AnnotationMember;)Ljava/lang/annotation/Annotation;");
#endif
  InitCacheMethodAddr(Ljava_2Flang_2FBoolean_3B_ValueOf_Addr, *Ljava_2Flang_2FBoolean_3B,
                      "valueOf", "(Z)Ljava/lang/Boolean;");
  InitCacheMethodAddr(Ljava_2Flang_2FByte_3B_ValueOf_Addr, *Ljava_2Flang_2FByte_3B, "valueOf", "(B)Ljava/lang/Byte;");
  InitCacheMethodAddr(Ljava_2Flang_2FCharacter_3B_ValueOf_Addr, *Ljava_2Flang_2FCharacter_3B,
                      "valueOf", "(C)Ljava/lang/Character;");
  InitCacheMethodAddr(Ljava_2Flang_2FShort_3B_ValueOf_Addr, *Ljava_2Flang_2FShort_3B,
                      "valueOf", "(S)Ljava/lang/Short;");
  InitCacheMethodAddr(Ljava_2Flang_2FInteger_3B_ValueOf_Addr, *Ljava_2Flang_2FInteger_3B,
                      "valueOf", "(I)Ljava/lang/Integer;");
  InitCacheMethodAddr(Ljava_2Flang_2FLong_3B_ValueOf_Addr, *Ljava_2Flang_2FLong_3B, "valueOf", "(J)Ljava/lang/Long;");
  InitCacheMethodAddr(Ljava_2Flang_2FFloat_3B_ValueOf_Addr, *Ljava_2Flang_2FFloat_3B,
                      "valueOf", "(F)Ljava/lang/Float;");
  InitCacheMethodAddr(Ljava_2Flang_2FDouble_3B_ValueOf_Addr, *Ljava_2Flang_2FDouble_3B,
                      "valueOf", "(D)Ljava/lang/Double;");
}

void WellKnown::InitCacheFieldMethodHandleOffsets() {
  InitCacheFieldOffset(Ljava_2Flang_2FMethodType_3B_ptypes_offset, *Ljava_2Flang_2Finvoke_2FMethodType_3B, "ptypes");
  InitCacheFieldOffset(Ljava_2Flang_2FMethodType_3B_rtype_offset, *Ljava_2Flang_2Finvoke_2FMethodType_3B, "rtype");
#ifdef METHODHANDLE_OPENJDK
  InitCacheFieldOffset(Ljava_2Flang_2FMethodHandle_3B_dataArray_offset,
                       *Ljava_2Flang_2Finvoke_2FMethodHandle_3B, "dataArray");
  InitCacheFieldOffset(Ljava_2Flang_2FMethodHandle_3B_metaArray_offset,
                       *Ljava_2Flang_2Finvoke_2FMethodHandle_3B, "metaArray");
  InitCacheFieldOffset(Ljava_2Flang_2FMethodHandle_3B_typeArray_offset,
                       *Ljava_2Flang_2Finvoke_2FMethodHandle_3B, "typeArray");
  InitCacheFieldOffset(Ljava_2Flang_2FMethodHandle_3B_opArray_offset,
                       *Ljava_2Flang_2Finvoke_2FMethodHandle_3B, "opArray");
  InitCacheFieldOffset(Ljava_2Flang_2FMethodHandle_3B_index_offset,
                       *Ljava_2Flang_2Finvoke_2FMethodHandle_3B, "index");
  InitCacheFieldOffset(Ljava_2Flang_2FBindToData_3B_receiver_offset,
                       *Ljava_2Flang_2Finvoke_2FInvokeData_24BindToData_3B, "receiver");
  InitCacheFieldOffset(Ljava_2Flang_2FDropArgumentsData_3B_numDropped_offset,
                       *Ljava_2Flang_2Finvoke_2FInvokeData_24DropArgumentsData_3B, "numDropped");
  InitCacheFieldOffset(Ljava_2Flang_2FDropArgumentsData_3B_startPos_offset,
                       *Ljava_2Flang_2Finvoke_2FInvokeData_24DropArgumentsData_3B, "startPos");
  InitCacheFieldOffset(Ljava_2Flang_2FFilterReturnValueData_3B_target_offset,
                       *Ljava_2Flang_2Finvoke_2FInvokeData_24FilterReturnValueData_3B, "target");
  InitCacheFieldOffset(Ljava_2Flang_2FFilterReturnValueData_3B_filter_offset,
                       *Ljava_2Flang_2Finvoke_2FInvokeData_24FilterReturnValueData_3B, "filter");
  InitCacheFieldOffset(Ljava_2Flang_2FPermuteArgumentsData_3B_target_offset,
                       *Ljava_2Flang_2Finvoke_2FInvokeData_24PermuteArgumentsData_3B, "target");
  InitCacheFieldOffset(Ljava_2Flang_2FPermuteArgumentsData_3B_reorder_offset,
                       *Ljava_2Flang_2Finvoke_2FInvokeData_24PermuteArgumentsData_3B, "reorder");
#else
#ifndef __OPENJDK__
  InitCacheFieldOffset(Ljava_2Flang_2FMethodHandle_3B_artFieldOrMethod_offset,
                       *Ljava_2Flang_2Finvoke_2FMethodHandle_3B, "artFieldOrMethod");
  InitCacheFieldOffset(Ljava_2Flang_2FMethodHandle_3B_handleKind_offset,
                       *Ljava_2Flang_2Finvoke_2FMethodHandle_3B, "handleKind");
  InitCacheFieldOffset(Ljava_2Flang_2FMethodHandle_3B_nominalType_offset,
                       *Ljava_2Flang_2Finvoke_2FMethodHandle_3B, "nominalType");
  InitCacheFieldOffset(Ljava_2Flang_2FMethodHandle_3B_type_offset,
                       *Ljava_2Flang_2Finvoke_2FMethodHandle_3B, "type");
  InitCacheFieldOffset(Ldalvik_2Fsystem_2FEmulatedStackFrame_3B_callsiteType_offset,
                       *Ldalvik_2Fsystem_2FEmulatedStackFrame_3B, "callsiteType");
  InitCacheFieldOffset(Ldalvik_2Fsystem_2FEmulatedStackFrame_3B_references_offset,
                       *Ldalvik_2Fsystem_2FEmulatedStackFrame_3B, "references");
  InitCacheFieldOffset(Ldalvik_2Fsystem_2FEmulatedStackFrame_3B_stackFrame_offset,
                       *Ldalvik_2Fsystem_2FEmulatedStackFrame_3B, "stackFrame");
  InitCacheFieldOffset(Ldalvik_2Fsystem_2FEmulatedStackFrame_3B_type_offset,
                       *Ldalvik_2Fsystem_2FEmulatedStackFrame_3B, "type");
#endif // OPENJDK
#endif // METHODHANDLE_OPENJDK
}

void WellKnown::InitCacheFieldOffsets() {
  InitCacheFieldOffset(Ljava_2Flang_2FBoolean_3B_value_offset, *Ljava_2Flang_2FBoolean_3B, "value");
  InitCacheFieldOffset(Ljava_2Flang_2FByte_3B_value_offset, *Ljava_2Flang_2FByte_3B, "value");
  InitCacheFieldOffset(Ljava_2Flang_2FCharacter_3B_value_offset, *Ljava_2Flang_2FCharacter_3B, "value");
  InitCacheFieldOffset(Ljava_2Flang_2FShort_3B_value_offset, *Ljava_2Flang_2FShort_3B, "value");
  InitCacheFieldOffset(Ljava_2Flang_2FInteger_3B_value_offset, *Ljava_2Flang_2FInteger_3B, "value");
  InitCacheFieldOffset(Ljava_2Flang_2FLong_3B_value_offset, *Ljava_2Flang_2FLong_3B, "value");
  InitCacheFieldOffset(Ljava_2Flang_2FFloat_3B_value_offset, *Ljava_2Flang_2FFloat_3B, "value");
  InitCacheFieldOffset(Ljava_2Flang_2FDouble_3B_value_offset, *Ljava_2Flang_2FDouble_3B, "value");
  InitCacheFieldOffset(kReferenceReferentOffset, *Ljava_2Flang_2Fref_2FReference_3B, "referent");
  InitCacheFieldOffset(kReferenceQueueOffset, *Ljava_2Flang_2Fref_2FReference_3B, "queue");
  InitCacheFieldMethodHandleOffsets();
#ifdef __OPENJDK__
  InitCacheFieldOffset(kReferencePendingnextOffset, *Ljava_2Flang_2Fref_2FReference_3B, "discovered");
#else // libcore
  InitCacheFieldOffset(kReferencePendingnextOffset, *Ljava_2Flang_2Fref_2FReference_3B, "pendingNext");
  InitCacheFieldOffset(kFinalizereferenceZombieOffset, *Ljava_2Flang_2Fref_2FFinalizerReference_3B, "zombie");
#endif // __OPENJDK__
}

void WellKnown::InitCacheMethodHandleClasses() {
  InitCacheClass(Ljava_2Flang_2Finvoke_2FMethodHandle_3B, "Ljava/lang/invoke/MethodHandle;");
  InitCacheClass(Ljava_2Flang_2Finvoke_2FMethodType_3B, "Ljava/lang/invoke/MethodType;");
#ifdef METHODHANDLE_OPENJDK
  InitCacheClass(Ljava_2Flang_2Finvoke_2FInvokeData_24BindToData_3B,
      "Ljava/lang/invoke/InvokeData$BindToData;");
  InitCacheClass(Ljava_2Flang_2Finvoke_2FInvokeData_24DropArgumentsData_3B,
      "Ljava/lang/invoke/InvokeData$DropArgumentsData;");
  InitCacheClass(Ljava_2Flang_2Finvoke_2FInvokeData_24FilterReturnValueData_3B,
      "Ljava/lang/invoke/InvokeData$FilterReturnValueData;");
  InitCacheClass(Ljava_2Flang_2Finvoke_2FInvokeData_24PermuteArgumentsData_3B,
      "Ljava/lang/invoke/InvokeData$PermuteArgumentsData;");
#else
#ifndef __OPENJDK__
  InitCacheClass(Ldalvik_2Fsystem_2FEmulatedStackFrame_3B, "Ldalvik/system/EmulatedStackFrame;");
#endif // __OPENJDK__
#endif // METHODHANDLE_OPENJDK
}

void WellKnown::InitCacheClasses() {
  InitCachePrimitiveBoxClass();
  InitCacheArrayClass();
  InitCacheExceptionClass();
  InitCacheMethodHandleClasses();
  InitCacheClass(Ljava_2Flang_2FObject_3B, "Ljava/lang/Object;");
  InitCacheClass(Ljava_2Flang_2FClassLoader_3B, "Ljava/lang/ClassLoader;");
  InitCacheClass(Ljava_2Flang_2Freflect_2FField_3B, "Ljava/lang/reflect/Field;");
  InitCacheClass(Ljava_2Flang_2Freflect_2FConstructor_3B, "Ljava/lang/reflect/Constructor;");
  InitCacheClass(Ljava_2Flang_2Freflect_2FMethod_3B, "Ljava/lang/reflect/Method;");
  InitCacheClass(Ljava_2Flang_2Freflect_2FProxy_3B, "Ljava/lang/reflect/Proxy;");
  InitCacheClass(Ljava_2Flang_2Freflect_2FParameter_3B, "Ljava/lang/reflect/Parameter;");
#ifndef __OPENJDK__
  InitCacheClass(Llibcore_2Freflect_2FGenericSignatureParser_3B, "Llibcore/reflect/GenericSignatureParser;");
#endif // __OPENJDK__
  InitCacheClass(Ljava_2Flang_2FCloneable_3B, "Ljava/lang/Cloneable;");
  InitCacheClass(Ljava_2Fio_2FSerializable_3B, "Ljava/io/Serializable;");
  InitCacheClass(Ljava_2Flang_2FStringFactory_3B, "Ljava/lang/StringFactory;");
  InitCacheClass(Ljava_2Flang_2FEnum_3B, "Ljava/lang/Enum;");
  InitCacheClass(Ljava_2Flang_2Fannotation_2FAnnotation_3B, "Ljava/lang/annotation/Annotation;");
#ifndef __OPENJDK__
  InitCacheClass(Llibcore_2Freflect_2FAnnotationMember_3B, "Llibcore/reflect/AnnotationMember;");
  InitCacheClass(Llibcore_2Freflect_2FAnnotationFactory_3B, "Llibcore/reflect/AnnotationFactory;");
  InitCacheClass(Ldalvik_2Fsystem_2FDelegateLastClassLoader_3B, "Ldalvik/system/DelegateLastClassLoader;");
  InitCacheClass(Ldalvik_2Fsystem_2FPathClassLoader_3B, "Ldalvik/system/PathClassLoader;");
  InitCacheClass(Ldalvik_2Fsystem_2FDexClassLoader_3B, "Ldalvik/system/DexClassLoader;");
  InitCacheClass(Ldalvik_2Fsystem_2FInMemoryDexClassLoader_3B, "Ldalvik/system/InMemoryDexClassLoader;");
#endif // __OPENJDK__
#ifdef __OPENJDK__
  InitCacheClass(Ljava_2Futil_2FHashMap_3B, "Ljava/util/HashMap;");
  InitCacheClass(Lsun_2Freflect_2Fannotation_2FAnnotationParser_3B, "Lsun/reflect/annotation/AnnotationParser;");
#else // libcore
  InitCacheClass(Ljava_2Flang_2Fref_2FFinalizerReference_3B, "Ljava/lang/ref/FinalizerReference;");
#endif // __OPENJDK__
  InitCacheClass(Ljava_2Flang_2Fref_2FReference_3B, "Ljava/lang/ref/Reference;");

  InitCacheClass(Ljava_2Flang_2FInteger_24IntegerCache_3B, "Ljava/lang/Integer$IntegerCache;");
  InitCacheClass(Ljava_2Flang_2FByte_24ByteCache_3B, "Ljava/lang/Byte$ByteCache;");
  InitCacheClass(Ljava_2Flang_2FShort_24ShortCache_3B, "Ljava/lang/Short$ShortCache;");
  InitCacheClass(Ljava_2Flang_2FCharacter_24CharacterCache_3B, "Ljava/lang/Character$CharacterCache;");
  InitCacheClass(Ljava_2Flang_2FLong_24LongCache_3B, "Ljava/lang/Long$LongCache;");

  InitArrayInterfaceVector();
  // keep order with maple::Primitive::Type
  primitiveArrayClass[maple::Primitive::kNot] = nullptr;
  primitiveArrayClass[maple::Primitive::kBoolean] = primitiveClassAZ;
  primitiveArrayClass[maple::Primitive::kByte] = primitiveClassAB;
  primitiveArrayClass[maple::Primitive::kChar] = primitiveClassAC;
  primitiveArrayClass[maple::Primitive::kShort] = primitiveClassAS;
  primitiveArrayClass[maple::Primitive::kInt] = primitiveClassAI;
  primitiveArrayClass[maple::Primitive::kLong] = primitiveClassAJ;
  primitiveArrayClass[maple::Primitive::kFloat] = primitiveClassAF;
  primitiveArrayClass[maple::Primitive::kDouble] = primitiveClassAD;
};

void WellKnown::InitCacheFieldMetas() {
  InitCacheFieldMeta(Ljava_2Flang_2FBoolean_3B_TRUE, *Ljava_2Flang_2FBoolean_3B, "TRUE");
  InitCacheFieldMeta(Ljava_2Flang_2FBoolean_3B_FALSE, *Ljava_2Flang_2FBoolean_3B, "FALSE");
  InitCacheFieldMeta(Ljava_2Flang_2FByte_24ByteCache_3B_cache, *Ljava_2Flang_2FByte_24ByteCache_3B, "cache");
  InitCacheFieldMeta(Ljava_2Flang_2FShort_24ShortCache_3B_cache, *Ljava_2Flang_2FShort_24ShortCache_3B, "cache");
  InitCacheFieldMeta(Ljava_2Flang_2FCharacter_24CharacterCache_3B_cache,
                     *Ljava_2Flang_2FCharacter_24CharacterCache_3B, "cache");
  InitCacheFieldMeta(Ljava_2Flang_2FLong_24LongCache_3B_cache, *Ljava_2Flang_2FLong_24LongCache_3B, "cache");
  InitCacheFieldMeta(Ljava_2Flang_2FInteger_24IntegerCache_3B_cache,
                     *Ljava_2Flang_2FInteger_24IntegerCache_3B, "cache");
  InitCacheFieldMeta(Ljava_2Flang_2FInteger_24IntegerCache_3B_low,
                     *Ljava_2Flang_2FInteger_24IntegerCache_3B, "low");
  InitCacheFieldMeta(Ljava_2Flang_2FInteger_24IntegerCache_3B_high,
                     *Ljava_2Flang_2FInteger_24IntegerCache_3B, "high");
}

MethodMeta *WellKnown::GetStringFactoryConstructor(const MethodMeta &stringConstructor) {
  struct StringInitMap {
    const char *initSignature;
    const char *stringFacName;
    const char *stringFacSignature;
    MethodMeta *stringFactoryConstructor;
  };
  StringInitMap initMap[] = {
      { "()V", "newEmptyString", "()Ljava/lang/String;", nullptr },
      { "([B)V", "newStringFromBytes", "([B)Ljava/lang/String;", nullptr },
      { "([BI)V", "newStringFromBytes", "([BI)Ljava/lang/String;", nullptr },
      { "([BII)V", "newStringFromBytes", "([BII)Ljava/lang/String;", nullptr },
      { "([BIII)V", "newStringFromBytes", "([BIII)Ljava/lang/String;", nullptr },
      { "([BIILjava/lang/String;)V", "newStringFromBytes", "([BIILjava/lang/String;)Ljava/lang/String;", nullptr },
      { "([BLjava/lang/String;)V", "newStringFromBytes", "([BLjava/lang/String;)Ljava/lang/String;", nullptr },
      { "([BIILjava/nio/charset/Charset;)V", "newStringFromBytes",
          "([BIILjava/nio/charset/Charset;)Ljava/lang/String;", nullptr },
      { "([BLjava/nio/charset/Charset;)V", "newStringFromBytes",
          "([BLjava/nio/charset/Charset;)Ljava/lang/String;", nullptr },
      { "([C)V", "newStringFromChars", "([C)Ljava/lang/String;", nullptr },
      { "([CII)V", "newStringFromChars", "([CII)Ljava/lang/String;", nullptr },
      { "(II[C)V", "newStringFromChars", "(II[C)Ljava/lang/String;", nullptr },
      { "(Ljava/lang/String;)V", "newStringFromString", "(Ljava/lang/String;)Ljava/lang/String;", nullptr },
      { "(Ljava/lang/StringBuffer;)V", "newStringFromStringBuffer",
          "(Ljava/lang/StringBuffer;)Ljava/lang/String;", nullptr },
      { "([III)V", "newStringFromCodePoints", "([III)Ljava/lang/String;", nullptr },
      { "(Ljava/lang/StringBuilder;)V", "newStringFromStringBuilder",
          "(Ljava/lang/StringBuilder;)Ljava/lang/String;", nullptr }
  };
  MClass *classObj = stringConstructor.GetDeclaringClass();
  CHECK(classObj->IsStringClass()) << "must String Class." << maple::endl;
  CHECK(stringConstructor.IsConstructor()) << "must String Constructor." << maple::endl;
  const char *signature = stringConstructor.GetSignature();
  constexpr uint32_t length = sizeof(initMap) / sizeof(struct StringInitMap);
  uint32_t index = 0;
  for (; index < length; ++index) {
    if (strcmp(signature, initMap[index].initSignature) == 0) {
      break;
    }
  }
  CHECK(index < length) << "Not find String constructor from StringFactory" << maple::endl;
  MethodMeta *stringFactoryCtor = initMap[index].stringFactoryConstructor;
  if (stringFactoryCtor != nullptr) {
    return stringFactoryCtor;
  }
  MethodMeta *ctor = WellKnown::GetMClassStringFactory()->GetDeclaredMethod(initMap[index].stringFacName,
      initMap[index].stringFacSignature);
  initMap[index].stringFactoryConstructor = ctor;
  return ctor;
}

void MRT_BootstrapWellKnown(void) {
  WellKnown::InitCacheClasses();
  WellKnown::InitCacheMethodAddrs();
  WellKnown::InitCacheFieldOffsets();
  WellKnown::InitCacheFieldMetas();
}

jclass MRT_GetPrimitiveClassVoid(void) {
  return WellKnown::GetMClassV()->AsJclass();
}

jclass MRT_GetClassClass(void) {
  return WellKnown::GetMClassClass()->AsJclass();
}

jclass MRT_GetClassObject(void) {
  return WellKnown::GetMClassObject()->AsJclass();
}

jclass MRT_GetClassString(void) {
  return WellKnown::GetMClassString()->AsJclass();
}

jclass MRT_GetPrimitiveArrayClassJboolean(void) {
  return WellKnown::GetMClassAZ()->AsJclass();
}

jclass MRT_GetPrimitiveArrayClassJbyte(void) {
  return WellKnown::GetMClassAB()->AsJclass();
}

jclass MRT_GetPrimitiveArrayClassJchar(void) {
  return WellKnown::GetMClassAC()->AsJclass();
}

jclass MRT_GetPrimitiveArrayClassJdouble(void) {
  return WellKnown::GetMClassAD()->AsJclass();
}

jclass MRT_GetPrimitiveArrayClassJfloat(void) {
  return WellKnown::GetMClassAF()->AsJclass();
}

jclass MRT_GetPrimitiveArrayClassJint(void) {
  return WellKnown::GetMClassAI()->AsJclass();
}

jclass MRT_GetPrimitiveArrayClassJlong(void) {
  return WellKnown::GetMClassAJ()->AsJclass();
}

jclass MRT_GetPrimitiveArrayClassJshort(void) {
  return WellKnown::GetMClassAS()->AsJclass();
}

jclass MRT_GetPrimitiveClassJboolean(void) {
  return WellKnown::GetMClassZ()->AsJclass();
}

jclass MRT_GetPrimitiveClassJbyte(void) {
  return WellKnown::GetMClassB()->AsJclass();
}

jclass MRT_GetPrimitiveClassJchar(void) {
  return WellKnown::GetMClassC()->AsJclass();
}

jclass MRT_GetPrimitiveClassJdouble(void) {
  return WellKnown::GetMClassD()->AsJclass();
}

jclass MRT_GetPrimitiveClassJfloat(void) {
  return WellKnown::GetMClassF()->AsJclass();
}

jclass MRT_GetPrimitiveClassJint(void) {
  return WellKnown::GetMClassI()->AsJclass();
}

jclass MRT_GetPrimitiveClassJlong(void) {
  return WellKnown::GetMClassJ()->AsJclass();
}

jclass MRT_GetPrimitiveClassJshort(void) {
  return WellKnown::GetMClassS()->AsJclass();
}
} // namespace maplert
