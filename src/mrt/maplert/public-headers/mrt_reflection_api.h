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
#ifndef MAPLE_MRT_REFLECTION_API_H
#define MAPLE_MRT_REFLECTION_API_H

#include "mrt_api_common.h"
#include "jni.h"
#ifdef __cplusplus
namespace maplert {
extern "C" {
#endif

// bridging API between JNI jmethodID/jfieldID to Java Method/Field object
MRT_EXPORT jfieldID  MRT_ReflectFromReflectedField(jobject fieldObj);
MRT_EXPORT jobject   MRT_ReflectToReflectedField(jclass clazz, jfieldID fid);
MRT_EXPORT jmethodID MRT_ReflectFromReflectedMethod(jobject jobj);
MRT_EXPORT jobject   MRT_ReflectToReflectedMethod(jclass clazz, jmethodID mid);

MRT_EXPORT jclass   MRT_ReflectClassForCharName(const char *className, bool init,
                                                jobject ClassLoader, bool internalName = false);
MRT_EXPORT jclass   MRT_ReflectGetObjectClass(jobject jobj);
MRT_EXPORT jobject  MRT_ReflectGetClassLoader(jobject jobj);
MRT_EXPORT jint     MRT_ReflectGetArrayIndexScaleForComponentType(jclass componentClassObj);

MRT_EXPORT bool         MRT_ClassSetGctib(jclass classObj, char *newBuffer, jint offset);
MRT_EXPORT uint32_t     MRT_GetFieldOffset(jfieldID field);
MRT_EXPORT jboolean     MRT_ReflectClassIsArray(jclass classObj);
MRT_EXPORT jboolean     MRT_ReflectClassIsPrimitive(jclass classObj);
MRT_EXPORT jboolean     MRT_ReflectIsString(const jclass classObj);
MRT_EXPORT jboolean     MRT_ReflectIsClass(const jobject classObj);
MRT_EXPORT jboolean     MRT_ReflectClassIsInterface(jclass classObj);
MRT_EXPORT jboolean     MRT_ReflectIsInit(jclass classObj);
MRT_EXPORT jboolean     MRT_ReflectInitialized(jclass classObj);
MRT_EXPORT jclass       MRT_ReflectClassGetSuperClass(jclass clazz);
MRT_EXPORT jboolean     MRT_ReflectIsInstanceOf(jobject jobj, jclass javaClass);
MRT_EXPORT jboolean     MRT_ReflectClassIsAssignableFrom(jclass superClass, jclass subClass);
MRT_EXPORT uint32_t     MRT_ReflectClassGetNumofFields(jclass cls);
MRT_EXPORT jclass       MRT_ReflectClassGetComponentType(jclass classObj);
MRT_EXPORT jint         MRT_ReflectClassGetAccessFlags(jclass classObj);
MRT_EXPORT jobject      MRT_ReflectClassGetDeclaredMethods(jclass classObj, jboolean publicOnly);
MRT_EXPORT jobjectArray MRT_ReflectClassGetInterfaces(jclass classObj);
MRT_EXPORT jint         MRT_ReflectClassGetModifiers(jclass classObj);
MRT_EXPORT jobject      MRT_ReflectClassGetDeclaredFields(jclass classObj, jboolean publicOnly);
MRT_EXPORT jobject      MRT_ReflectClassGetDeclaredAnnotations(jclass classObj);
MRT_EXPORT jobjectArray MRT_ReflectClassGetDeclaredClasses(jclass classObj);
MRT_EXPORT jclass       MRT_ReflectClassGetDeclaringClass(jclass classObj);
MRT_EXPORT jobject      MRT_ReflectClassGetDeclaredConstructors(jclass classObj, jboolean publicOnly);

MRT_EXPORT bool     MRT_IsMetaObject(jobject jobj);
MRT_EXPORT bool     MRT_IsValidMethod(jmethodID jmid);
MRT_EXPORT bool     MRT_IsValidField(jfieldID jfid);
MRT_EXPORT bool     MRT_IsValidClass(jclass jclazz);

MRT_EXPORT jobject  MRT_ReflectAllocObject(const jclass javaClass, bool isJNI = false);
MRT_EXPORT jobject  MRT_ReflectNewObjectA(const jclass javaClass, const jmethodID mid,
                                          const jvalue *args, bool isJNI = false);

MRT_EXPORT jfieldID MRT_ReflectClassGetFieldsPtr(jclass classObj);
MRT_EXPORT jfieldID MRT_ReflectClassGetIndexField(jfieldID head, int i);

MRT_EXPORT jfieldID MRT_ReflectGetCharField(jclass classObj, const char *fieldName, const char *fieldType = nullptr);
MRT_EXPORT jfieldID MRT_ReflectGetStaticCharField(jclass classObj, const char *fieldName);
MRT_EXPORT jclass   MRT_ReflectFieldGetDeclaringClass(jfieldID fieldMeta);
MRT_EXPORT jboolean MRT_ReflectFieldIsStatic(jfieldID fieldObj);
MRT_EXPORT char    *MRT_ReflectFieldGetCharFieldName(jfieldID obj);
MRT_EXPORT char    *MRT_ReflectFieldGetTypeName(jfieldID fieldMeta);
MRT_EXPORT jclass   MRT_ReflectFieldGetType(jfieldID fieldMeta);
MRT_EXPORT jint     MRT_ReflectFieldGetOffset(jobject fieldMeta);

MRT_EXPORT jboolean MRT_ReflectGetFieldjboolean(jfieldID fieldObj, jobject obj);
MRT_EXPORT jbyte    MRT_ReflectGetFieldjbyte(jfieldID fieldObj, jobject obj);
MRT_EXPORT jchar    MRT_ReflectGetFieldjchar(jfieldID fieldObj, jobject obj);
MRT_EXPORT jdouble  MRT_ReflectGetFieldjdouble(jfieldID fieldObj, jobject obj);
MRT_EXPORT jfloat   MRT_ReflectGetFieldjfloat(jfieldID fieldObj, jobject obj);
MRT_EXPORT jint     MRT_ReflectGetFieldjint(jfieldID fieldObj, jobject obj);
MRT_EXPORT jlong    MRT_ReflectGetFieldjlong(jfieldID fieldObj, jobject obj);
MRT_EXPORT jshort   MRT_ReflectGetFieldjshort(jfieldID fieldObj, jobject obj);
MRT_EXPORT jobject  MRT_ReflectGetFieldjobject(jfieldID fieldObj, jobject obj);

MRT_EXPORT void MRT_ReflectSetFieldjboolean(jfieldID fieldObj, jobject obj, jboolean value);
MRT_EXPORT void MRT_ReflectSetFieldjbyte(jfieldID fieldObj, jobject obj, jbyte value);
MRT_EXPORT void MRT_ReflectSetFieldjchar(jfieldID fieldObj, jobject obj, jchar value);
MRT_EXPORT void MRT_ReflectSetFieldjshort(jfieldID fieldObj, jobject obj, jshort value);
MRT_EXPORT void MRT_ReflectSetFieldjint(jfieldID fieldObj, jobject obj, jint value);
MRT_EXPORT void MRT_ReflectSetFieldjfloat(jfieldID fieldObj, jobject obj, jfloat value);
MRT_EXPORT void MRT_ReflectSetFieldjlong(jfieldID fieldObj, jobject obj, jlong value);
MRT_EXPORT void MRT_ReflectSetFieldjdouble(jfieldID fieldObj, jobject obj, jdouble value);
MRT_EXPORT void MRT_ReflectSetFieldjobject(jfieldID fieldObj, jobject obj, jobject value);

// function for dump heap
MRT_EXPORT jboolean MRT_ReflectGetFieldjbooleanUnsafe(jfieldID fieldObj, jobject obj);
MRT_EXPORT jbyte    MRT_ReflectGetFieldjbyteUnsafe(jfieldID fieldObj, jobject obj);
MRT_EXPORT jchar    MRT_ReflectGetFieldjcharUnsafe(jfieldID fieldObj, jobject obj);
MRT_EXPORT jdouble  MRT_ReflectGetFieldjdoubleUnsafe(jfieldID fieldObj, jobject obj);
MRT_EXPORT jfloat   MRT_ReflectGetFieldjfloatUnsafe(jfieldID fieldObj, jobject obj);
MRT_EXPORT jint     MRT_ReflectGetFieldjintUnsafe(jfieldID fieldObj, jobject obj);
MRT_EXPORT jlong    MRT_ReflectGetFieldjlongUnsafe(jfieldID fieldObj, jobject obj);
MRT_EXPORT jshort   MRT_ReflectGetFieldjshortUnsafe(jfieldID fieldObj, jobject obj);
MRT_EXPORT jobject  MRT_ReflectGetFieldjobjectUnsafe(jfieldID fieldObj, jobject obj);

MRT_EXPORT jmethodID    MRT_ReflectGetCharMethod(jclass classObj, const char *methodName, const char *signatureName);
MRT_EXPORT jmethodID    MRT_ReflectGetStaticCharMethod(jclass classObj, const char *methodName,
                                                       const char *signatureName);
MRT_EXPORT jmethodID    MRT_ReflectGetMethodFromMethodID(jclass clazz, jmethodID methodID, const char *signature);
MRT_EXPORT char        *MRT_ReflectGetMethodName(jmethodID mid);
MRT_EXPORT jint         MRT_ReflectGetMethodArgsize(jmethodID mid);
MRT_EXPORT void         MRT_ReflectGetMethodArgsType(const char *signame, const jint argsize, char *shorty);
MRT_EXPORT char        *MRT_ReflectGetMethodSig(jmethodID mid);
MRT_EXPORT jclass       MRT_ReflectMethodGetDeclaringClass(jmethodID methodId);
MRT_EXPORT jboolean     MRT_ReflectMethodIsStatic(jmethodID methodMeta);
MRT_EXPORT jboolean     MRT_ReflectMethodIsConstructor(jmethodID methodMeta);
MRT_EXPORT jobjectArray MRT_ReflectMethodGetExceptionTypes(jobject methodObj);
MRT_EXPORT jobject      MRT_ReflectMethodGetDefaultValue(jobject methodObj);
MRT_EXPORT jobject      MRT_ReflectMethodGetAnnotationNative(jobject executable, jint index, jclass annoClass);
MRT_EXPORT void         MRT_ReflectMethodForward(jobject from, jobject to);
MRT_EXPORT jmethodID    MRT_ReflectMethodClone(jmethodID methodObj);

#ifndef TYIPLE_MRT_REFLECT_INVOKE_DECL
#define TYIPLE_MRT_REFLECT_INVOKE_DECL(TYPE) \
MRT_EXPORT TYPE MRT_ReflectInvokeMethodA##TYPE(jobject obj, const jmethodID mid, const jvalue *args); \
MRT_EXPORT TYPE MRT_ReflectInvokeMethodAZ##TYPE(jobject obj, const jmethodID mid, const jvalue *args, \
                                                uintptr_t calleeFuncAddr);
#endif

// invoke a slow method
TYIPLE_MRT_REFLECT_INVOKE_DECL(void)
TYIPLE_MRT_REFLECT_INVOKE_DECL(jboolean)
TYIPLE_MRT_REFLECT_INVOKE_DECL(jbyte)
TYIPLE_MRT_REFLECT_INVOKE_DECL(jchar)
TYIPLE_MRT_REFLECT_INVOKE_DECL(jdouble)
TYIPLE_MRT_REFLECT_INVOKE_DECL(jfloat)
TYIPLE_MRT_REFLECT_INVOKE_DECL(jint)
TYIPLE_MRT_REFLECT_INVOKE_DECL(jlong)
TYIPLE_MRT_REFLECT_INVOKE_DECL(jobject)
TYIPLE_MRT_REFLECT_INVOKE_DECL(jshort)

// proxy interface api
MRT_EXPORT jclass MRT_ReflectProxyGenerateProxy(jstring name, jobjectArray interfaces, jobject loader,
                                                jobjectArray methods, jobjectArray throws);
MRT_EXPORT jobject MRT_ReflectConstructorNewInstance0(jobject javaMethod, jobjectArray javaArgs);
MRT_EXPORT jobject MRT_ReflectConstructorNewInstanceFromSerialization(jclass ctorClass, const jclass allocClass);
// Executable interface api
MRT_EXPORT jobject      MRT_ReflectExecutableGetSignatureAnnotation(jobject methodObj);
MRT_EXPORT jobject      MRT_ReflectExecutableGetParameterAnnotationsNative(jobject method);
MRT_EXPORT jboolean     MRT_ReflectExecutableIsAnnotationPresentNative(jobject methodObj, jclass annoObj);
MRT_EXPORT jint         MRT_ReflectExecutableCompareMethodParametersInternal(jobject obj1, jobject obj2);
MRT_EXPORT jobjectArray MRT_ReflectExecutableGetDeclaredAnnotationsNative(jobject methodObj);
MRT_EXPORT jobject      MRT_ReflectExecutableGetAnnotationNative(jobject methodObj, jclass annoClass);
MRT_EXPORT jobject      MRT_ReflectExecutableGetParameters0(jobject thisObj);
MRT_EXPORT jobjectArray MRT_ReflectExecutableGetParameterTypesInternal(jobject methodObj);
MRT_EXPORT jint         MRT_ReflectExecutableGetParameterCountInternal(jobject methodObj);
MRT_EXPORT jclass       MRT_ReflectExecutableGetMethodReturnTypeInternal(jobject methodObj);
MRT_EXPORT jstring      MRT_ReflectExecutableGetMethodNameInternal(jobject methodObj);
// MethodHandleImpl interface api
MRT_EXPORT jobject MRT_MethodHandleImplGetMemberInternal(jobject methodHandle);
// openjdk
MRT_EXPORT jclass MRT_ReflectGetOrCreateArrayClassObj(jclass elementClass);

#ifdef __cplusplus
} // extern "C"
} // namespace maplert
#endif

#endif // MAPLE_MRT_REFLECTION_API_H
