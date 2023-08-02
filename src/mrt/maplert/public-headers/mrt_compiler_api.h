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
#ifndef MRT_COMPILER_API_H
#define MRT_COMPILER_API_H
#include "cinterface.h"
#include "exception/eh_personality.h"
#include "metadata_layout.h"
#include "mrt_api_common.h"
#include "mclass.h"
namespace maplert {
// MCC_ prefixed function is external interface, the compiler guarantees parameter safety
// when calling such interface during the compilation phase.
extern "C" {
// exception
MRT_EXPORT void MCC_ThrowNullArrayNullPointerException();
MRT_EXPORT void MCC_CheckThrowPendingException();
MRT_EXPORT void MCC_ThrowNullPointerException();
MRT_EXPORT void MCC_ThrowArithmeticException();
MRT_EXPORT void MCC_ThrowInterruptedException();
MRT_EXPORT void MCC_ThrowClassCastException(const char *msg);
MRT_EXPORT void MCC_ThrowArrayIndexOutOfBoundsException(const char *msg);
MRT_EXPORT void MCC_ThrowUnsatisfiedLinkError();
MRT_EXPORT void MCC_ThrowSecurityException();
MRT_EXPORT void MCC_ThrowExceptionInInitializerError(MrtClass cause);
MRT_EXPORT void MCC_ThrowNoClassDefFoundError(MrtClass classInfo);
MRT_EXPORT void MCC_ThrowStringIndexOutOfBoundsException();
MRT_EXPORT void *MCC_JavaBeginCatch(const _Unwind_Exception *unwind_exception);
MRT_EXPORT void MCC_ThrowException(MrtClass obj);
MRT_EXPORT void MCC_ThrowPendingException();
MRT_EXPORT void MCC_RethrowException(MrtClass obj);

// fast stack unwind
MRT_EXPORT void MCC_SetRiskyUnwindContext(uint32_t *pc, void *fp);
MRT_EXPORT void MCC_SetReliableUnwindContext();

// decouple for lazybinding, called while visiting offset table at first time
MRT_EXPORT int32_t MCC_FixOffsetTableVtable(uint32_t offsetVal, char *offsetEntry);
MRT_EXPORT int32_t MCC_FixOffsetTableField(uint32_t offsetVal, char *offsetEntry);

// string
MRT_EXPORT bool MCC_String_Equals_NotallCompress(jstring thisStr, jstring anotherStr);
MRT_EXPORT jstring MCC_CStrToJStr(const char *ca, jint len);
MRT_EXPORT jstring MCC_GetOrInsertLiteral(jstring literal);
MRT_EXPORT jstring MCC_StringAppend(uint64_t toStringFlag, ...);
MRT_EXPORT jstring MCC_StringAppend_StringString(jstring strObj1, jstring strObj2);
MRT_EXPORT jstring MCC_StringAppend_StringInt(jstring strObj1, jint intValue);
MRT_EXPORT jstring MCC_StringAppend_StringJcharString(jstring strObj1, uint16_t charValue, jstring strObj2);

// mclasslocatormanager
MRT_EXPORT jclass MCC_GetClass(jclass klass, const char *className);
MRT_EXPORT jobject MCC_GetCurrentClassLoader(jobject obj);

// class init
MRT_EXPORT void MCC_PreClinitCheck(ClassMetadata &classInfo __attribute__((unused)));
MRT_EXPORT void MCC_PostClinitCheck(ClassMetadata &classInfo __attribute__((unused)));

// reflection class
MRT_EXPORT void MCC_Array_Boundary_Check(jobjectArray javaArray, jint index);
MRT_EXPORT void MCC_Reflect_ThrowCastException(const jclass sourceInfo, jobject targetObject, jint dim);
MRT_EXPORT void MCC_Reflect_Check_Casting_Array(jclass sourceClass, jobject targetObject, jint arrayDim);
MRT_EXPORT void MCC_ThrowCastException(jclass targetClass, jobject castObj);
MRT_EXPORT void MCC_Reflect_Check_Arraystore(jobject arrayObject, jobject elemObject);
MRT_EXPORT void MCC_Reflect_Check_Casting_NoArray(jclass sourceClass, jobject targetClass);
MRT_EXPORT jboolean MCC_Reflect_IsInstance(jobject, jobject);

MRT_EXPORT uintptr_t MCC_getFuncPtrFromItabSlow64(const MObject *obj, uintptr_t hashCode,
                                                  uintptr_t secondHashCode, const char *signature);
MRT_EXPORT uintptr_t MCC_getFuncPtrFromItabSlow32(const MObject *obj, uint32_t hashCode,
                                                  uint32_t secondHashCode, const char *signature);
MRT_EXPORT uintptr_t MCC_getFuncPtrFromItabInlineCache(uint64_t* cacheEntryAddr, const MClass *klass, uint32_t hashCode,
                                                       uint32_t secondHashCode, const char *signature);
MRT_EXPORT uintptr_t MCC_getFuncPtrFromItabSecondHash64(const uintptr_t *itab, uintptr_t hashCode,
                                                        const char *signature);
MRT_EXPORT uintptr_t MCC_getFuncPtrFromItabSecondHash32(const uint32_t *itab, uint32_t hashCode,
                                                        const char *signature);
#if defined(__arm__)
MRT_EXPORT uintptr_t MCC_getFuncPtrFromItab(const uint32_t *itab, const char *signature, uint32_t hashCode);
#else  // ~__arm__
MRT_EXPORT uintptr_t MCC_getFuncPtrFromItab(const uint32_t *itab, uint32_t hashCode, const char *signature);
#endif  // ~__arm__
MRT_EXPORT uintptr_t MCC_getFuncPtrFromVtab64(const MObject *obj, uint32_t offset);
MRT_EXPORT uintptr_t MCC_getFuncPtrFromVtab32(const MObject *obj, uint32_t offset);

MRT_EXPORT void MCC_ArrayMap_String_Int_put(jstring key, jint value);
MRT_EXPORT jint MCC_ArrayMap_String_Int_size();
MRT_EXPORT jint MCC_ArrayMap_String_Int_getOrDefault(jstring key, jint defaultValue);
MRT_EXPORT void MCC_ArrayMap_String_Int_clear();

// reference api
MRT_EXPORT void MCC_CheckObjAllocated(address_t obj);
MRT_EXPORT address_t MCC_IncRef_NaiveRCFast(address_t obj);
MRT_EXPORT address_t MCC_LoadRefField_NaiveRCFast(address_t obj, address_t *filedAddress);
MRT_EXPORT address_t MCC_DecRef_NaiveRCFast(address_t obj);
MRT_EXPORT void MCC_IncDecRef_NaiveRCFast(address_t incAddr, address_t decAddr);
MRT_EXPORT void MCC_IncDecRefReset(address_t incAddr, address_t *decAddr);
MRT_EXPORT void MCC_DecRefResetPair(address_t *incAddr, address_t *decAddr);
MRT_EXPORT void MCC_CleanupLocalStackRef_NaiveRCFast(address_t *localStart, size_t count);
MRT_EXPORT void MCC_CleanupLocalStackRefSkip_NaiveRCFast(address_t *localStart, size_t count, size_t skip);
MRT_EXPORT void MCC_CleanupNonescapedVar(address_t obj);
MRT_EXPORT void MCC_ClearLocalStackRef(address_t *var);
MRT_EXPORT void MCC_WriteRefFieldStaticNoInc(address_t *field, address_t value);
MRT_EXPORT void MCC_WriteRefFieldStaticNoDec(address_t *field, address_t value);
MRT_EXPORT void MCC_WriteRefFieldStaticNoRC(address_t *field, address_t value);
MRT_EXPORT void MCC_WriteVolatileStaticFieldNoInc(address_t *objAddr, address_t value);
MRT_EXPORT void MCC_WriteVolatileStaticFieldNoDec(address_t *objAddr, address_t value);
MRT_EXPORT void MCC_WriteVolatileStaticFieldNoRC(address_t *objAddr, address_t value);
MRT_EXPORT void MCC_WriteRefFieldNoRC(address_t obj, address_t *field, address_t value);
MRT_EXPORT void MCC_WriteRefFieldNoDec(address_t obj, address_t *field, address_t value);
MRT_EXPORT void MCC_WriteRefFieldNoInc(address_t obj, address_t *field, address_t value);
MRT_EXPORT void MCC_WriteVolatileFieldNoInc(address_t obj, address_t *objAddr, address_t value);
MRT_EXPORT void MCC_WriteWeakField(address_t obj, address_t *fieldAddr, address_t value);
MRT_EXPORT address_t MCC_LoadWeakField(address_t obj, address_t *fieldAddr);
MRT_EXPORT void MCC_WriteVolatileWeakField(address_t obj, address_t *fieldAddr, address_t value);
MRT_EXPORT address_t MCC_LoadVolatileWeakField(address_t obj, address_t *fieldAddr);
MRT_EXPORT void MCC_CleanupLocalStackRef(const address_t *localStart, size_t count);
MRT_EXPORT void MCC_CleanupLocalStackRefSkip(const address_t *localStart, size_t count, size_t skip);
MRT_EXPORT address_t MCC_NewObj(size_t size, size_t align);
MRT_EXPORT address_t MCC_LoadRefStatic(address_t *fieldAddr);
MRT_EXPORT address_t MCC_LoadVolatileStaticField(address_t *fieldAddr);
MRT_EXPORT address_t MCC_LoadReferentField(address_t obj, address_t *fieldAddr);
MRT_EXPORT void MCC_WriteRefFieldStatic(address_t *field, address_t value);
MRT_EXPORT void MCC_WriteVolatileStaticField(address_t *objAddr, address_t value);
MRT_EXPORT address_t MCC_LoadVolatileField(address_t obj, address_t *fieldAddr);
MRT_EXPORT void MCC_PreWriteRefField(address_t obj);
MRT_EXPORT void MCC_WriteRefField(address_t obj, address_t *field, address_t value);
MRT_EXPORT void MCC_WriteVolatileField(address_t obj, address_t *objAddr, address_t value);
MRT_EXPORT void MCC_WriteReferent(address_t obj, address_t value);
MRT_EXPORT void MCC_InitializeLocalStackRef(address_t *localStart, size_t count);
MRT_EXPORT void MCC_RunFinalization();

// Call fast native function with no more than 8 args.
MRT_EXPORT void *MCC_CallFastNative(...);
// Call fast native function with arbitrary number of args.
MRT_EXPORT void *MCC_CallFastNativeExt(...);
// Call slow native function with no more than 8 args.
MRT_EXPORT void *MCC_CallSlowNative0(...);
MRT_EXPORT void *MCC_CallSlowNative1(...);
MRT_EXPORT void *MCC_CallSlowNative2(...);
MRT_EXPORT void *MCC_CallSlowNative3(...);
MRT_EXPORT void *MCC_CallSlowNative4(...);
MRT_EXPORT void *MCC_CallSlowNative5(...);
MRT_EXPORT void *MCC_CallSlowNative6(...);
MRT_EXPORT void *MCC_CallSlowNative7(...);
MRT_EXPORT void *MCC_CallSlowNative8(...);
// Call slow native function with arbitrary number of args.
MRT_EXPORT void *MCC_CallSlowNativeExt(...);
MRT_EXPORT void MCC_RecordStaticField(address_t *field, const char *name);

// libs fast
MRT_EXPORT void MCC_SyncExitFast(address_t obj);
MRT_EXPORT void MCC_SyncEnterFast2(address_t obj);
MRT_EXPORT void MCC_SyncEnterFast0(address_t obj);
MRT_EXPORT void MCC_SyncEnterFast1(address_t obj);
MRT_EXPORT void MCC_SyncEnterFast3(address_t obj);
MRT_EXPORT JNIEnv *MCC_PreNativeCall(jobject caller);
MRT_EXPORT void MCC_PostNativeCall(JNIEnv *env);

// libs
MRT_EXPORT int32_t MCC_DexArrayLength(const void *p);
MRT_EXPORT void MCC_DexArrayFill(void *d, void *s, int32_t len);
MRT_EXPORT void *MCC_DexCheckCast(void *i, void *c);
MRT_EXPORT void *MCC_GetReferenceToClass(void *c);
MRT_EXPORT bool MCC_DexInstanceOf(void *i, void *c);
MRT_EXPORT void MCC_DexInterfaceCall(void *dummy);
MRT_EXPORT bool MCC_IsAssignableFrom(jclass subClass, jclass superClass);
#if defined(__aarch64__)
#define POLYRETURNTYPE jvalue
#elif defined(__arm__)
#define POLYRETURNTYPE jlong
#endif

MRT_EXPORT POLYRETURNTYPE MCC_DexPolymorphicCall(jstring calleeName,
    jstring protoString, int paramNum, jobject methodHandle, ...);

MRT_EXPORT int32_t MCC_JavaArrayLength(const void *p);
MRT_EXPORT void MCC_JavaArrayFill(void *d, void *s, int32_t len);
MRT_EXPORT void *MCC_JavaCheckCast(void *i, void *c);
MRT_EXPORT bool MCC_JavaInstanceOf(void *i, void *c);
MRT_EXPORT void MCC_JavaInterfaceCall(void *dummy);
MRT_EXPORT POLYRETURNTYPE MCC_JavaPolymorphicCall(jstring calleeName,
    jstring protoString, int paramNum, jobject methodHandle, ...);

// chelper
MRT_EXPORT void MCC_SetJavaClass(address_t objaddr, address_t klass);
MRT_EXPORT void MCC_SetObjectPermanent(address_t objaddr);
MRT_EXPORT address_t MCC_NewObj_fixed_class(address_t klass);
MRT_EXPORT address_t MCC_Reflect_ThrowInstantiationError();
MRT_EXPORT address_t MCC_NewObj_flexible_cname(size_t elemSize, size_t nElems,
                                               const char *classNameOrclassObj,
                                               address_t callerObj, unsigned long isClassObj);
MRT_EXPORT address_t MCC_NewObject(address_t klass);
MRT_EXPORT address_t MCC_NewArray8(size_t nElems, address_t klass);
MRT_EXPORT address_t MCC_NewArray16(size_t nElems, address_t klass);
MRT_EXPORT address_t MCC_NewArray32(size_t nElems, address_t klass);
MRT_EXPORT address_t MCC_NewArray64(size_t nElems, address_t klass);
MRT_EXPORT address_t MCC_NewArray(size_t nElems, const char *descriptor, address_t callerObj);
MRT_EXPORT address_t MCC_NewPermanentObject(address_t klass);
MRT_EXPORT address_t MCC_NewPermanentArray(size_t elemSize, size_t nElems,
                                           const char *classNameOrclassObj, address_t callerObj,
                                           unsigned long isClassObj);
MRT_EXPORT address_t MCC_NewPermObject(address_t klass);
MRT_EXPORT address_t MCC_NewPermArray8(size_t nElems, address_t klass);
MRT_EXPORT address_t MCC_NewPermArray16(size_t nElems, address_t klass);
MRT_EXPORT address_t MCC_NewPermArray32(size_t nElems, address_t klass);
MRT_EXPORT address_t MCC_NewPermArray64(size_t nElems, address_t klass);
MRT_EXPORT address_t MCC_NewPermArray(size_t nElems, const char *descriptor, address_t callerObj);

// jsan
MRT_EXPORT void MCC_CheckObjMem(void *obj);

MRT_EXPORT void MCC_CheckRefCount(address_t obj, uint32_t index);

// native binding
MRT_EXPORT jobject MCC_CannotFindNativeMethod(const char *signature);
MRT_EXPORT jstring MCC_CannotFindNativeMethod_S(const char *signature);
MRT_EXPORT jobject MCC_CannotFindNativeMethod_A(const char *signature);
MRT_EXPORT void *MCC_FindNativeMethodPtr(uintptr_t **regFuncTabAddr);
MRT_EXPORT void *MCC_FindNativeMethodPtrWithoutException(uintptr_t **regFuncTabAddr);
MRT_EXPORT void *MCC_DummyNativeMethodPtr();
MRT_EXPORT jobject MCC_DecodeReference(jobject obj);

// deferredaccess
MRT_EXPORT void MCC_DeferredClinitCheck(address_t daiClass, const MObject *caller, const MString *className);
MRT_EXPORT MClass *MCC_DeferredConstClass(address_t daiClass, const MObject *caller, const MString *className);
MRT_EXPORT bool MCC_DeferredInstanceOf(address_t daiClass, const MObject *caller, const MString *className,
                                       const MObject *obj);
MRT_EXPORT MObject *MCC_DeferredCheckCast(address_t daiClass, const MObject *caller, const MString *className,
                                          MObject *obj);
MRT_EXPORT MObject *MCC_DeferredNewInstance(address_t daiClass, const MObject *caller, const MString *className);
MRT_EXPORT MObject *MCC_DeferredNewArray(address_t daiClass, const MObject *caller, const MString *arrayTypeName,
                                         uint32_t length);
MRT_EXPORT MObject *MCC_DeferredFillNewArray(address_t daiClass, const MObject *caller,
                                             const MString *arrayTypeName, uint32_t length, ...);
MRT_EXPORT jvalue MCC_DeferredLoadField(address_t daiField, MObject *caller, const MString *className,
                                        const MString *fieldName, const MString *fieldTypeName, const MObject *obj);
MRT_EXPORT void MCC_DeferredStoreField(address_t daiField, MObject *caller, const MString *className,
                                       const MString *fieldName, const MString *fieldTypeName, MObject *obj,
                                       jvalue value);
MRT_EXPORT jvalue MCC_DeferredInvoke(address_t daiMethod, int kind, const char *className, const char *methodName,
                                     const char *signature, MObject *obj, ...);

// type conversion, these two function is used to convert java double or java float to java long on arm32 platform
MRT_EXPORT int64_t MCC_JDouble2JLong(double num);
MRT_EXPORT int64_t MCC_JFloat2JLong(float num);

// profile
MRT_EXPORT void MCC_SaveProfile();

}  // extern "C"
}  // maplert
#endif
