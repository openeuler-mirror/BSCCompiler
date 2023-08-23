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
#ifndef MAPLE_RUNTIME_CHELPER_H
#define MAPLE_RUNTIME_CHELPER_H

/**
 * The purpose of this file is to help C programs to correctly create managed
 * objects and access its fields.
 *
 * Some functions serve the MapleJava project, only.  They are for the
 * convenience of the C programmers.
 *
 * ==========
 * HOW TO USE
 * ==========
 *
 * When compiling .mpl files, add the -gen-c-macro-def option to mplcg:
 *
 *   mplcg -gen-c-macro-def foo.mpl
 *
 * That will generate foo.macros.def alongside foo.s. If you have multiple
 * .macros.def files, merge them with `cat foo.macros.def bar.macros.def | sort
 * | uniq > unified.macros.def` so that duplicated entries are removed.
 *
 * In C files (.c), add the following in the beginning:
 *
 *   #include "chelper.h"
 *
 *   #ifndef __UNIFIED_MACROS_DEF__
 *   #define __UNIFIED_MACROS_DEF__
 *   #include "unified.macros.def"
 *   #endif
 *
 * This will create symbol definitions for class-related information, such as
 * class instance sizes, and the offset of each class field.  This will enable
 * all the macros in this header.
 *
 * The __UNIFIED_MACROS_DEF__ is useful when we have the habit of including one
 * .c file into another, and both need to use macros in this header.
 *
 * Create Java scalar objects using MRT_NEWOBJ(java_class_name), for example:
 *
 *   address_t obj_addr = MRT_NEWOBJ(Ljava/lang/Object;);
 *   MRT_IncRef(obj_addr);
 *
 * or if you defined your own `jobject` type:
 *
 *   jobject obj = (jobject)MRT_NEWOBJ(Ljava/lang/Object;);
 *   MRT_IncRef((address_t)obj_addr);
 *
 * Don't worry about GCTIB, itable, vtable or the klass field.  MRT_NEWOBJ
 * already filled those fields for you.  In the future, when we change our
 * naming convention, we change this header.
 *
 * Creating Java arrays using MRT_NEW_*_ARRAY(len), for example:
 *
 *   address_t jint_array = MRT_NEW_JINT_ARRAY(100);
 *   MRT_IncRef(jint_array);
 *
 *   address_t jobj_array = MRT_NEW_JOBJECT_ARRAY(100);
 *   MRT_IncRef(jobj_array);
 *
 * MRT_NEW_*_ARRAY handles the length field and the elemSize (component size)
 * field, too.
 *
 * String has a custom layout, and cannot be allocated in the generic way.  See
 * `dex2mpl/projects/helloworld/HelloWorld.jni.c` for more information.
 *
 * Load and store object fields using the MRT_LOAD_*(object, offset) and the
 * MRT_STORE_*(object, offset, value) macros:
 *
 *   jint v = MRT_LOAD_JINT(foo, MRT_FIELD_OFFSET(Lcom_2Fexample_2FSomeObject_3B, someIntField))
 *   MRT_STORE_JINT(foo, MRT_FIELD_OFFSET(Lcom_2Fexample_2FSomeObject_3B, someIntField), newval)
 *
 * The MRT_FIELD_OFFSET(className, fieldName) returns the offset of that field.
 * Note that className must be the class in which fieldName is defined.
 * Subclasses may define fields of the same name if the field is private, so our
 * macros make a compromise by not inheriting fields into subclasses.
 * (Interestingly, according to the Java Language Specification, subclasses
 * really do not "inherit" fields from its parents.)
 *
 * When linking, link with `mapleall/runtime/src/buildaarch64/libmplrt.a`.
 */
#include "collector/collector.h"
#include "namemangler.h"
#include "mm_config.h"
#include "cinterface.h"
#include "mrt_fields_api.h" // Load and store operations for fields
#include "panic.h"
#include "jsan.h"
#include "mrt_compiler_api.h"
#include "mrt_reflection_api.h"
// Some Macro magics.
//
// Force expansion before pasting.
#define __MRT_MAGIC_PASTE(x, y) __MRT_MAGIC_PASTE2(x, y)
#define __MRT_MAGIC_PASTE2(x, y) x##y

// Defer expansion
#define __MRT_MAGIC_EMPTY()
#define __MRT_MAGIC_DEFER(x) x __MRT_MAGIC_EMPTY()
#define __MRT_MAGIC_DEFER_IGNORE(x) __MRT_MAGIC_EMPTY()

#ifdef __cplusplus
namespace maplert {
extern "C" {
#endif

// Query instance size, field offset and field size.
//
// className is an identifier.  These macros automatically paste identifiers.
#define MRT_INSTANCE_SIZE(className) __MRT_MAGIC_PASTE(__MRT_instance_size__, className)
#define MRT_FIELD_OFFSET(className, fieldName) \
    __MRT_MAGIC_PASTE(__MRT_MAGIC_PASTE(__MRT_MAGIC_PASTE(__MRT_field_offset__, className), __), fieldName)
#define MRT_FIELD_SIZE(className, fieldName) \
    __MRT_MAGIC_PASTE(__MRT_MAGIC_PASTE(__MRT_MAGIC_PASTE(__MRT_field_size__, className), __), fieldName)

#define MRT_GCTIB(className)  __MRT_MAGIC_PASTE(GCTIB_PREFIX, className)
#define MRT_ITABLE(className) __MRT_MAGIC_PASTE(ITAB_PREFIX, className)
#define MRT_VTABLE(className) __MRT_MAGIC_PASTE(VTAB_PREFIX, className)
#define MRT_CLASSINFO(className) __MRT_MAGIC_PASTE(CLASSINFO_PREFIX, className)
#define MRT_PRIMITIVECLASSINFO(className) __MRT_MAGIC_PASTE(PRIMITIVECLASSINFO_PREFIX, className)
#define MRT_FIELDS(className)  __MRT_MAGIC_PASTE(FIELDINFO_PREFIX, className)
#define MRT_METHODS(className) __MRT_MAGIC_PASTE(METHODINFO_PREFIX, className)

#define MRT_GETKLASS(className) MRT_GetClassByClassLoader(nullptr, TO_STR(className))

// Expand the definitions from the .macros.def files generated by
// `mplcg --gen-c-macro-def`
//
// The following two are not really a vararg macros. That's for preventing
// expanding className and offset too early. For example, when fieldName is
// "errno" or "stdout", it will be a problem.
#define MRT_FIELD_OFFSET_DEF(className, fieldName) __MRT_field_offset__##className##__##fieldName
#define MRT_FIELD_SIZE_DEF(className, fieldName) __MRT_field_size__##className##__##fieldName

#define __MRT_CLASS(className, size, superclass, ...) \
    static const size_t MRT_INSTANCE_SIZE(className##__VA_ARGS__) = size; \
    extern void *MRT_CLASSINFO(className##__VA_ARGS__);

#define __MRT_CLASS_FIELD(className, fieldName, offset, size, ...) \
    static const size_t MRT_FIELD_OFFSET_DEF(className##__VA_ARGS__, fieldName##__VA_ARGS__) = offset; \
    static const size_t MRT_FIELD_SIZE_DEF(className##__VA_ARGS__, fieldName##__VA_ARGS__) = size;

//
// Creating an object in Java involves four steps:
//
// 0. Allocating the memory for the object.
// 1. Do proper initialization for the language-agnostic runtime
//    - e.g. To implement GC, this will assign the GCTIB to the object header.
// 2. Do proper initialization for the language runtime
//    - e.g. To implement Java, this will assign the fields not visible to the
//      Java langauge, including the pointers to I-table, V-table, the Class
//      object, and the monitor word.
// 3. Call the user-defined object initializer
//    - e.g. For Java, it is the `<init>` method, of the proper signature if
//      overloaded.
//
// The following macros perform one or more of the above steps.
address_t __MRT_chelper_newobj_0(size_t size);

address_t __MRT_chelper_newobj_flexible_0(size_t elemSize, size_t len);
address_t MRT_ChelperNewobjFlexible(size_t elemSize, size_t len, address_t klass, bool isJNI = false);

#define MRT_NEWOBJ_0(className) \
    __MRT_chelper_newobj_0(MRT_INSTANCE_SIZE(className))
// Must use MRT_GETKLASS(className) instead of MRT_CLASSINFO(className) here due to
// dynamic class loading
#define MRT_NEWOBJ_1(className) \
    MRT_ReflectAllocObject(MRT_GETKLASS(className))

#define MRT_NEWOBJ_FLEXIBLE_0(elemSize, len, cls) \
    __MRT_chelper_newobj_flexible_0(elemSize, len)

#define MRT_NEWOBJ_FLEXIBLE_1(elemSize, len, cls) \
    MRT_ChelperNewobjFlexible(elemSize, len, reinterpret_cast<address_t>(cls))

#define MRT_NEWOBJ_FLEXIBLE_JNI_1(elemSize, len, cls, jni) \
    MRT_ChelperNewobjFlexible(elemSize, len, reinterpret_cast<address_t>(cls), jni)

// CONFIG ME: Set the following macro to the desired level.
//
// Setting to 2 will automatically assign itab, vtab and klass.  Since these
// assignments are idempotent, it does not affect the correctness if <init>()
// also initializes those fields.
#define __MRT_DESIRED_INIT_LEVEL 1

#define __THE_MRT_NEWOBJ __MRT_MAGIC_PASTE(MRT_NEWOBJ_, __MRT_DESIRED_INIT_LEVEL)
#define __THE_MRT_NEWOBJ_FLEXIBLE __MRT_MAGIC_PASTE(MRT_NEWOBJ_FLEXIBLE_, __MRT_DESIRED_INIT_LEVEL)
#define THE_MRT_NEWOBJ_FLEXIBLE_JNI __MRT_MAGIC_PASTE(MRT_NEWOBJ_FLEXIBLE_JNI_, __MRT_DESIRED_INIT_LEVEL)

// This is the intended way to allocate heap objects in C
#define MRT_NEWOBJ(className) __THE_MRT_NEWOBJ(className)

#define MRT_NEW_JOBJECT_ARRAY(len, klass) __THE_MRT_NEWOBJ_FLEXIBLE(sizeof(reffield_t), len, klass)

#define MRT_NEW_PRIMITIVE_ARRAY(elemSize, len, klass) __THE_MRT_NEWOBJ_FLEXIBLE(elemSize, len, klass)

#define MRT_NEW_PRIMITIVE_ARRAY_JNI(elemSize, len, klass, jni) THE_MRT_NEWOBJ_FLEXIBLE_JNI(elemSize, len, klass, jni)

// ---  Write barriers  ---
// Object *var = value;  ----> MRT_WRITE_REF_VAR(var, value)
// not used now, Undefined
#define MRT_WRITE_REF_VAR(var, value) \
    MRT_WriteRefVar(reinterpret_cast<address_t*>(&(var)), reinterpret_cast<address_t>(value))

// obj->field = value; ----> MRT_WRITE_REF_FIELD(obj, field, value)
#define MRT_WRITE_REF_FIELD(obj, field, value) \
    MRT_WriteRefField(reinterpret_cast<address_t>(obj), \
    reinterpret_cast<address_t*>(&(obj->field)), reinterpret_cast<address_t>(value))

// release(obj); ----> MRT_RELEASE_REF_VAR(obj)
#define MRT_RELEASE_REF_VAR(obj) MRT_ReleaseRefVar(reinterpret_cast<address_t>(obj))

// RC operations used in MRT
#define ENABLE_RC_FASTPATH
#ifdef ENABLE_RC_FASTPATH
#define RC_LOCAL_INC_REF(obj) (void)(MRT_IncRefNaiveRCFast(reinterpret_cast<address_t>(obj)))
#define RC_RUNTIME_INC_REF(obj) (void)(MRT_IncRefNaiveRCFast(reinterpret_cast<address_t>(obj)))
#define RC_LOCAL_DEC_REF(obj) (void)MRT_DecRefNaiveRCFast(reinterpret_cast<address_t>(obj))
#define RC_RUNTIME_DEC_REF(obj) (void)MRT_DecRefNaiveRCFast(reinterpret_cast<address_t>(obj))
#else
#define RC_LOCAL_INC_REF(obj) MRT_IncRef(reinterpret_cast<address_t>(obj))
#define RC_RUNTIME_INC_REF(obj) MRT_IncRef(reinterpret_cast<address_t>(obj))
#define RC_LOCAL_DEC_REF(obj) MRT_DecRef(reinterpret_cast<address_t>(obj))
#define RC_RUNTIME_DEC_REF(obj) MRT_DecRef(reinterpret_cast<address_t>(obj))
#endif

constexpr std::size_t kMrtKlassOffset = 0;

// Have to add these, too, in order not to depend on the C++ header sizes.h
// GREP-ARRAYLAYOUT: Grep me to see if all these places agree.
void MRT_SetJavaClass(address_t objAddr, address_t klass);
void MRT_SetObjectPermanent(address_t objAddr);
void MRT_SetJavaArrayClass(address_t objAddr, address_t klass);
#if ALLOC_USE_FAST_PATH
void MRT_SetFastAlloc(ClassMetadata*);
#else
#define MRT_SetFastAlloc(x)
#endif
void MRT_CheckRefCount(address_t obj, uint32_t index);
void MRT_ReflectThrowNegtiveArraySizeException();
void MRT_SetThreadPriority(pid_t tid, int32_t priority);

inline void MRT_MemoryBarrier(void) {
#if defined(__aarch64__) || defined(__arm__)
  __asm__ ("dmb ish":::);
#else
#error "Unsupported architecture"
#endif
}

#ifdef __cplusplus
} // extern "C"
} // namespace maplert
#endif

#endif
