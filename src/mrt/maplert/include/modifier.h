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
#ifndef MRT_MAPLERT_MODIFIER_H_
#define MRT_MAPLERT_MODIFIER_H_
#include <cstdint>
#include <string>

namespace maplert {
namespace modifier {
static constexpr uint32_t kModifierPublic = 0x00000001;
static constexpr uint32_t kModifierPrivate = 0x00000002;
static constexpr uint32_t kModifierProtected = 0x00000004;
static constexpr uint32_t kModifierStatic = 0x00000008;
static constexpr uint32_t kModifierFinal = 0x00000010;
static constexpr uint32_t kModifierSynchronized = 0x00000020;
static constexpr uint32_t kModifierVolatile = 0x00000040;
static constexpr uint32_t kModifierTransient = 0x00000080;
static constexpr uint32_t kModifierNative = 0x00000100;
static constexpr uint32_t kModifierInterface = 0x00000200;
static constexpr uint32_t kModifierAbstract = 0x00000400;
static constexpr uint32_t kModifierStrict = 0x00000800;
static constexpr uint32_t kModifierSynthetic = 0x00001000;
static constexpr uint32_t kModifierAnnotation = 0x00002000;
static constexpr uint32_t kModifierEnum = 0x00004000;
static constexpr uint32_t kModifierMandated = 0x00008000;
static constexpr uint32_t kModifierConstructor = 0x00010000;
static constexpr uint32_t kModifierCriticalNative = 0x00200000;
static constexpr uint32_t kModifierDefault = 0x00400000;
static constexpr uint32_t kAFOriginPublic = 0x08000000;
static constexpr uint32_t kLocalClass = 0x10000000;
static constexpr uint32_t kLocalClassVaild = 0x20000000;

// use for annotation
static constexpr size_t kModifierRCUnowned = 0x00800000;
static constexpr size_t kModifierRCWeak = 0x01000000;

static constexpr uint32_t kModifierBridge = 0x00000040;
static constexpr uint32_t kModifierVarargs = 0x00000080;
static constexpr uint32_t kModifierFinalizable = 0x80000000;
static constexpr uint32_t kModifierProxy = 0x00040000;

// class flag
static constexpr uint16_t kClassPrim = 0x0001;
static constexpr uint16_t kClassArray = 0x0002;
static constexpr uint16_t kClassHasFinalizer = 0x0004;
static constexpr uint16_t kClassSoftReference = 0x0008;
static constexpr uint16_t kClassWeakReference = 0x0010;
static constexpr uint16_t kClassPhantomReference = 0x0020;
static constexpr uint16_t kClassFinalizerReference = 0x0040;
static constexpr uint16_t kClassCleaner = 0x0080;
static constexpr uint16_t kClassFinalizerReferenceSentinel = 0x0100;
static constexpr uint32_t kClassFastAlloc = 0x0200;
static constexpr uint16_t kClassIsAnonymousClass = 0x0400;
static constexpr uint16_t kClassIsColdClass = 0x0800;
static constexpr uint16_t kClassIsDecoupleClass = 0x1000;
static constexpr uint16_t kClassLazyBindingClass = 0x2000;
static constexpr uint16_t kClassLazyBoundClass = 0x4000;
static constexpr uint16_t kClassRuntimeVerify = 0x8000; // True if need verifier in runtime (error or deferred check).

static constexpr uint16_t kClassReference = (kClassSoftReference | kClassWeakReference |
    kClassCleaner | kClassFinalizerReference | kClassPhantomReference);

// method & field Hash Size & Conflict
static constexpr uint32_t kHashConflict = 1023;
static constexpr uint32_t kMethodFieldHashSize = 1022;

// field flag, type: uint16_t
// reserved high 10bits for field's hashcode: 0xFFC0
static constexpr uint32_t kFieldOffsetIspOffset = 0x0001;

// method flag, type: uint16_t
// reserved high 10bits for method's hashcode: 0xFFC0
static constexpr uint32_t kMethodNotVirtual = 0x00000001;
static constexpr uint32_t kMethodFinalize = 0x00000002;
static constexpr uint32_t kMethodMetaCompact = 0x00000004;
static constexpr uint32_t kMethodParametarType = 0x00000008;
static constexpr uint32_t kMethodAbstract = 0x00000010;
static constexpr uint32_t kMethodNativeSignature = 0x00000020;

static inline bool IsAFOriginPublic(uint32_t modifier) {
  return (modifier & kAFOriginPublic) != 0;
}

static inline bool IsLocalClass(uint32_t modifier) {
  return (modifier & kLocalClass) != 0;
}

static inline bool IsLocalClassVaild(uint32_t modifier) {
  return (modifier & kLocalClassVaild) != 0;
}

static inline bool IsPublic(uint32_t modifier) {
  return (modifier & kModifierPublic) != 0;
}

static inline bool IsPrivate(uint32_t modifier) {
  return (modifier & kModifierPrivate) != 0;
}

static inline bool IsProtected(uint32_t modifier) {
  return (modifier & kModifierProtected) != 0;
}

static inline bool IsStatic(uint32_t modifier) {
  return (modifier & kModifierStatic) != 0;
}

static inline bool IsFinal(uint32_t modifier) {
  return (modifier & kModifierFinal) != 0;
}

static inline bool IsSynchronized(uint32_t modifier) {
  return (modifier & kModifierSynchronized) != 0;
}

static inline bool IsVolatile(uint32_t modifier) {
  return (modifier & kModifierVolatile) != 0;
}

static inline bool IsTransient(uint32_t modifier) {
  return (modifier & kModifierTransient) != 0;
}

static inline bool IsNative(uint32_t modifier) {
  return (modifier & kModifierNative) != 0;
}

static inline bool IsCriticalNative(uint32_t modifier) {
  return (modifier & kModifierCriticalNative) != 0;
}

static inline bool IsInterface(uint32_t modifier) {
  return (modifier & kModifierInterface) != 0;
}

static inline bool IsAbstract(uint32_t modifier) {
  return (modifier & kModifierAbstract) != 0;
}

static inline bool IsStrict(uint32_t modifier) {
  return (modifier & kModifierStrict) != 0;
}

static inline bool IsSynthetic(uint32_t modifier) {
  return (modifier & kModifierSynthetic) != 0;
}

static inline bool IsConstructor(uint32_t modifier) {
  return (modifier & kModifierConstructor) != 0;
}

static inline bool IsDefault(uint32_t modifier) {
  return (modifier & kModifierDefault) != 0;
}

static inline bool IsAnnotation(uint32_t modifier) {
  return (modifier & kModifierAnnotation) != 0;
}

static inline bool IsEnum(uint32_t modifier) {
  return (modifier & kModifierEnum) != 0;
}

static inline bool IsMandated(uint32_t modifier) {
  return (modifier & kModifierMandated) != 0;
}

static inline bool IsBridge(uint32_t modifier) {
  return (modifier & kModifierBridge) != 0;
}

static inline bool IsVarargs(uint32_t modifier) {
  return (modifier & kModifierVarargs) != 0;
}

static inline bool IsFinalizable(uint32_t modifier) {
  return (modifier & kModifierFinalizable) != 0;
}

static inline bool IsProxy(uint32_t modifier) {
  return (modifier & kModifierProxy) != 0;
}

static inline bool IsUnowned(size_t modifier) {
  return (modifier & kModifierRCUnowned) != 0;
}

static inline bool IsWeakRef(size_t modifier) {
  return (modifier & kModifierRCWeak) != 0;
}

static inline bool IsDirectMethod(uint32_t modifier) {
  constexpr uint32_t direct = kModifierStatic | kModifierPrivate | kModifierConstructor;
  return (modifier & direct) != 0;
}

static inline uint32_t GetInterfaceModifier() {
  return kModifierInterface;
}

static inline uint32_t GetStaticModifier() {
  return kModifierStatic;
}

static inline uint32_t GetAbstractModifier() {
  return kModifierAbstract;
}

static inline uint32_t GetFinalModifier() {
  return kModifierFinal;
}

static inline uint32_t GetNativeModifier() {
  return kModifierNative;
}

static inline uint32_t GetSynchronizedModifier() {
  return kModifierSynchronized;
}

static inline bool IsPrimitiveClass(uint32_t flag) {
  return (flag & kClassPrim) != 0;
}

static inline bool IsArrayClass(uint32_t flag) {
  return (flag & kClassArray) != 0;
}

static inline bool hasFinalizer(uint32_t flag) {
  return (flag & kClassHasFinalizer) != 0;
}

static inline bool IsReferenceClass(uint32_t flag) {
  return (flag & kClassReference) != 0;
}

static inline bool IsFastAllocClass(uint32_t flag) {
  return (flag & kClassFastAlloc) != 0;
}

static inline bool IsAnonymousClass(uint32_t flag) {
  return (flag & kClassIsAnonymousClass) != 0;
}

static inline bool IsColdClass(uint32_t flag) {
  return (flag & kClassIsColdClass) != 0;
}

static inline bool IsDecoupleClass(uint32_t flag) {
  return (flag & kClassIsDecoupleClass) != 0;
}

static inline bool IsLazyBindingClass(uint32_t flag) {
  return (flag & kClassLazyBindingClass) != 0;
}

static inline bool IsLazyBoundClass(uint32_t flag) {
  return (flag & kClassLazyBoundClass) != 0;
}

static inline bool IsNotVerifiedClass(uint32_t flag) {
  return (flag & kClassRuntimeVerify) != 0;
}

static inline bool IsNotvirtualMethod(uint32_t flag) {
  return (flag & kMethodNotVirtual) != 0;
}

static inline bool IsFinalizeMethod(uint32_t flag) {
  return (flag & kMethodFinalize) != 0;
}
void JavaAccessFlagsToString(uint32_t accessFlags, std::string &result);
} // namespace modifier
} // namespace maplert
#endif
