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
#ifndef MAPLEALL_MAPLERT_JAVA_ANDROID_MRT_INCLUDE_MRT_REFLECTION_H_
#define MAPLEALL_MAPLERT_JAVA_ANDROID_MRT_INCLUDE_MRT_REFLECTION_H_

#include "mclass_inline.h"
#include "modifier.h"
#include "mrt_annotation.h"
#include "interp_support.h"
#include "mrt_classloader_api.h"

using namespace std;
namespace maplert {
namespace reflection {
static inline bool IsInSamePackage(const MClass &declaringClass, const MClass &callingClass, bool clFlag = true) {
  MClass *class1 = const_cast<MClass*>(&declaringClass);
  MClass *class2 = const_cast<MClass*>(&callingClass);
  if (class1 == class2) {
    return true;
  }
  // clFlag is used for classLoader.
  if (clFlag) {
    jobject classLoader1 = MRT_GetClassLoader(*class1);
    jobject classLoader2 = MRT_GetClassLoader(*class2);
    // The premise of comparison is the same class loader.
    if (classLoader1 != classLoader2) {
      return false;
    }
  }
  // Find and compare elements of arrays.
  while (class1->IsArrayClass()) {
    class1 = class1->GetComponentClass();
  }
  while (class2->IsArrayClass()) {
    class2 = class2->GetComponentClass();
  }
  if (class1 == class2) {
    return true;
  }
  // Compare whether in the same package.
  const char *declaringClassName = class1->GetName();
  const char *callingClassName = class2->GetName();
  const char *declaringPackageChar = strrchr(declaringClassName, '/');
  const char *callingPackageChar = strrchr(callingClassName, '/');
  if ((declaringPackageChar == nullptr) && (callingPackageChar == nullptr)) {
    return true;
  }
  if ((declaringPackageChar == nullptr) || (callingPackageChar == nullptr)) {
    return false;
  }
  size_t declaringPackageLen = static_cast<size_t>(declaringPackageChar - declaringClassName);
  size_t callingPackageLen = static_cast<size_t>(callingPackageChar - callingClassName);
  return (declaringPackageLen == callingPackageLen) ?
      (strncmp(declaringClassName, callingClassName, declaringPackageLen) == 0) : false;
}

static inline MClass *GetCallerClass(uint32_t level) {
  std::vector<UnwindContext> uwContextStack;
  constexpr uint32_t kReserveSize = 10;
  uwContextStack.reserve(kReserveSize);
  MapleStack::FastRecordCurrentJavaStack(uwContextStack, level + 1);
  MClass *klass = nullptr;
  uint32_t size = static_cast<uint32_t>(uwContextStack.size());
  if (size > level && !uwContextStack.empty()) {
    if (!uwContextStack[level].IsInterpretedContext()) {
      klass = MClass::JniCast(uwContextStack[level].frame.GetDeclaringClass());
    } else {
      klass = UnwindContextInterpEx::GetDeclaringClassFromUnwindContext(uwContextStack[level]);
    }
  }
  return klass;
}

static ALWAYS_INLINE inline bool CanAccess(const MClass &thatKlass, const MClass &callerClass) {
  return thatKlass.IsPublic() || IsInSamePackage(thatKlass, callerClass) ||
         (modifier::IsAFOriginPublic(thatKlass.GetModifier()) && thatKlass.IsInnerClass());
}

static ALWAYS_INLINE inline bool VerifyAccess(const MObject *obj, const MClass *declaringClass,
                                              uint32_t accessFlags, MClass *&callingClass, uint32_t level) {
  DCHECK(declaringClass != nullptr) << "VerifyAccess: declaringClass is nullptr!" << maple::endl;
  if (modifier::IsPublic(accessFlags)) {
    return true;
  }
  if (callingClass == nullptr) {
    callingClass = GetCallerClass(level);
  }
  if (declaringClass == callingClass || callingClass == nullptr) {
    return true;
  }
  if (modifier::IsPrivate(accessFlags)) {
    return false;
  }
  if (modifier::IsProtected(accessFlags)) {
    if (obj != nullptr && !obj->IsInstanceOf(*callingClass) && !IsInSamePackage(*declaringClass, *callingClass)) {
      return false;
    } else if (declaringClass->IsAssignableFrom(*callingClass)) {
      return true;
    }
  }
  return IsInSamePackage(*declaringClass, *callingClass);
}

extern "C" void MRT_ThrowNewException(const char *className, const char *msg);
static ALWAYS_INLINE inline bool CheckIsInstaceOf(const MClass &declaringClass, const MObject *mo) {
  if (UNLIKELY(mo == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException", "null receiver");
    return false;
  }
  if (mo->IsInstanceOf(declaringClass)) {
    return true;
  }
  // Throw Exception
  std::string declaringClassName, objectClassName;
  declaringClass.GetTypeName(declaringClassName);
  MClass *objectClass = mo->GetClass();
  objectClass->GetTypeName(objectClassName);
  std::ostringstream msg;
  msg << "Expected receiver of type " << declaringClassName << ", but got " << objectClassName;
  MRT_ThrowNewException("java/lang/IllegalArgumentException", msg.str().c_str());
  return false;
}
}
}
#endif // MAPLEALL_MAPLERT_JAVA_ANDROID_MRT_INCLUDE_MRT_REFLECTION_H_
