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
#ifndef MRT_MAPLERT_INCLUDE_METHODMETA_INLINE_H_
#define MRT_MAPLERT_INCLUDE_METHODMETA_INLINE_H_
#include "methodmeta.h"
#include "marray_inline.h"
#include "mrt_primitive_util.h"
#include "mrt_annotation.h"
#include "mrt_util.h"
#include "interp_support.h"
#include "modifier.h"
#include "mstring_inline.h"
namespace maplert {
inline int32_t MethodAddress::GetDefTabIndex() const {
  return static_cast<int32_t>(defTabIndex.GetRawValue());
}

inline uintptr_t MethodAddress::GetAddr() const {
  return addr.GetDataRef<uintptr_t>();
}

inline void MethodAddress::SetAddr(uintptr_t address) {
  addr.SetDataRef(address);
}

inline char *MethodSignature::GetSignature() const {
  char *signature = signatureName.GetDataRef<char*>();
  __MRT_Profile_CString(signature);
  return signature;
}

inline void MethodSignature::SetSignature(const char *signature) {
  signatureName.SetDataRef(signature);
}

inline MetaRef *MethodSignature::GetTypes() const {
  return pTypes.GetDataRef<MetaRef*>();
}

inline char *MethodMeta::GetName() const {
  char *name = methodName.GetDataRef<char*>();
  __MRT_Profile_CString(name);
  return name;
}

inline char *MethodMeta::GetSignature() const {
  return GetMethodSignature()->GetSignature();
}

inline uint32_t MethodMeta::GetMod() const {
  return mod;
}

inline std::string MethodMeta::GetAnnotation() const {
  return AnnotationUtil::GetAnnotationUtil(GetAnnotationRaw());
}

inline char *MethodMeta::GetAnnotationRaw() const {
  char *annotationRaw = annotationValue.GetDataRef<char*>();
  __MRT_Profile_CString(annotationRaw);
  return annotationRaw;
}

// This function is inlined in Fterp. If modified, change Fterp accordingly.
inline uintptr_t MethodMeta::GetFuncAddress() const {
  MethodAddress *pMethodAddress = GetpMethodAddress();
  return pMethodAddress->GetAddr();
}

inline uint16_t MethodMeta::GetFlag() const {
  return flag;
}

inline uint16_t MethodMeta::GetArgSize() const {
  return argumentSize;
}

inline int16_t MethodMeta::GetVtabIndex() const {
  return vtabIndex;
}

inline bool MethodMeta::IsStatic() const {
  return modifier::IsStatic(GetMod());
}

inline bool MethodMeta::IsAbstract() const {
  return modifier::IsAbstract(GetMod());
}

inline bool MethodMeta::IsPublic() const {
  return modifier::IsPublic(GetMod());
}

inline bool MethodMeta::IsPrivate() const {
  return modifier::IsPrivate(GetMod());
}
inline bool MethodMeta::IsDirectMethod() const {
  return modifier::IsDirectMethod(GetMod());
}

inline bool MethodMeta::IsConstructor() const {
  return modifier::IsConstructor(GetMod());
}

inline bool MethodMeta::IsDefault() const {
  return modifier::IsDefault(GetMod());
}

inline bool MethodMeta::IsProtected() const {
  return modifier::IsProtected(GetMod());
}

inline bool MethodMeta::IsFinalizeMethod() const {
  return modifier::IsFinalizeMethod(GetFlag());
}

inline bool MethodMeta::IsFinalMethod() const {
  return modifier::IsFinal(GetMod());
}

inline bool MethodMeta::IsNative() const {
  return modifier::IsNative(GetMod());
}

inline bool MethodMeta::IsCriticalNative() const {
  return false;
}

inline bool MethodMeta::IspAddress() const {
  return static_cast<bool>(pAddr.GetRawValue() & kMethodMetaAddrIspAddress);
}

inline bool MethodMeta::IsEnableParametarType() const {
  return (GetFlag() & modifier::kMethodParametarType) == modifier::kMethodParametarType;
}

inline bool MethodMeta::IsSynchronized() const {
  return modifier::IsSynchronized(GetMod());
}

inline int32_t MethodMeta::GetDefTabIndex() const {
  if (!IspAddress()) {
    return -1;
  }
  MethodAddress *pMethodAddress = GetpMethodAddress();
  return pMethodAddress->GetDefTabIndex();
}

inline MClass *MethodMeta::GetDeclaringClass() const {
  return declaringClass.GetDataRef<MClass*>();
}

inline uint32_t MethodMeta::GetParameterCount() const {
  uint32_t argSize = GetArgSize();
  return IsStatic() ? argSize : argSize - 1;
}

inline char *MethodMeta::GetReturnTypeName() const {
  char *methodSig = GetSignature();
  char *rtTypeName = methodSig;
  while (*(rtTypeName++) != ')') { }
  return rtTypeName;
}

inline char MethodMeta::GetReturnPrimitiveType() const {
  char *typeName = GetReturnTypeName();
  DCHECK(typeName != nullptr) << "MethodMeta::GetReturnPrimitiveType: typeName is nullptr!" << maple::endl;
  return typeName[0];
}

inline void MethodMeta::AddFlag(const uint16_t methodFlag) {
  flag |= methodFlag;
}

// This function is inlined in Fterp. If modified, change Fterp accordingly.
inline MethodAddress *MethodMeta::GetpMethodAddress() const {
  if (IspAddress()) {
    uintptr_t value = pAddr.GetDataRef<uintptr_t>();
    // compile will +2 as flag, here need -2 to remove it
    return reinterpret_cast<MethodAddress*>(value - 2);
  } else {
    uintptr_t value = reinterpret_cast<uintptr_t>(&pAddr);
    return reinterpret_cast<MethodAddress*>(value);
  }
}

inline MethodSignature *MethodMeta::GetMethodSignature() const {
  if (IsEnableParametarType()) {
    return pMethodSignature.GetDataRef<MethodSignature*>();
  } else {
    uintptr_t value = reinterpret_cast<uintptr_t>(&signatureOffset);
    return reinterpret_cast<MethodSignature*>(value);
  }
}

inline uint16_t MethodMeta::GetHashCode() const {
  return (flag >> kMethodFlagBits) & kMethodHashMask;
}

inline void MethodMeta::SetHashCode(uint16_t hash) {
  flag &= ~(kMethodHashMask << kMethodFlagBits);
  flag |= (hash & kMethodHashMask) << kMethodFlagBits;
}

ALWAYS_INLINE inline bool MethodMeta::NameCmp(const MString *name) const {
  if (UNLIKELY(name == nullptr)) {
    return false;
  }
  return name->Cmp(std::string(GetName()));
}

ALWAYS_INLINE inline bool MethodMeta::NameCmp(const char *name) const {
  if (UNLIKELY(name == nullptr)) {
    return false;
  }
  return !strcmp(name, GetName());
}

ALWAYS_INLINE inline bool MethodMeta::SignatureCmp(const MArray *signature) const {
  // signature always like (Ljava/lang/String;I), we can skip first char '('.
  char *desSigName = GetSignature() + 1;
  uint32_t arrayLen = signature == nullptr ? 0 : signature->GetLength();
  uint32_t index = 0;
  for (uint32_t i = 0; i < arrayLen; ++i) {
    MClass *clsObj = static_cast<MClass*>(signature->GetObjectElementNoRc(i));
    if (UNLIKELY(clsObj == nullptr)) {
      return false;
    }
    char *className = clsObj->GetName();
    for (uint32_t clsNameIndex = 0; className[clsNameIndex] != '\0' && desSigName[clsNameIndex] != '\0';
        ++clsNameIndex, ++index) {
      if (className[clsNameIndex] != desSigName[index]) {
        return false;
      }
    }
  }
  return desSigName[index] == ')' ? true : false;
}

ALWAYS_INLINE inline bool MethodMeta::SignatureCmp(const char *signature) const {
  if (UNLIKELY(signature == nullptr)) {
    return false;
  }
  return !strcmp(signature, GetSignature());
}

ALWAYS_INLINE inline bool MethodMeta::Cmp(const MString *mthName, const MArray *sigArray) const {
  return NameCmp(mthName) && SignatureCmp(sigArray);
}

ALWAYS_INLINE inline bool MethodMeta::Cmp(const char *name, const char *signature) const {
  return NameCmp(name) && SignatureCmp(signature);
}

template<calljavastubconst::ArgsType argsType>
ALWAYS_INLINE inline void MethodMeta::BuildArgstoJvalues(char type, BaseArgValue &values, uintptr_t args) {
  bool isVaArg = (argsType == calljavastubconst::kVaArg);
  switch (type) {
    case 'B':
      values.AddInt32(isVaArg ? va_arg(*reinterpret_cast<va_list*>(args), int32_t)
                              : (*reinterpret_cast<jvalue*>(args)).b);
      break;
    case 'C':
      values.AddInt32(isVaArg ? va_arg(*reinterpret_cast<va_list*>(args), int32_t)
                              : static_cast<int32_t>(static_cast<uint32_t>((*reinterpret_cast<jvalue*>(args)).c)));
      break;
    case 'I':
      values.AddInt32(isVaArg ? va_arg(*reinterpret_cast<va_list*>(args), int32_t)
                              : (*reinterpret_cast<jvalue*>(args)).i);
      break;
    case 'J':
      values.AddInt64(isVaArg ? va_arg(*reinterpret_cast<va_list*>(args), int64_t)
                              : (*reinterpret_cast<jvalue*>(args)).j);
      break;
    case 'S':
      values.AddInt32(isVaArg ? va_arg(*reinterpret_cast<va_list*>(args), int32_t)
                              : (*reinterpret_cast<jvalue*>(args)).s);
      break;
    case 'Z':
      values.AddInt32(isVaArg ? va_arg(*reinterpret_cast<va_list*>(args), int32_t)
                              : static_cast<int32_t>(static_cast<uint32_t>((*reinterpret_cast<jvalue*>(args)).z)));
      break;
    case 'D':
      values.AddDouble(isVaArg ? va_arg(*reinterpret_cast<va_list*>(args), double)
                               : (*reinterpret_cast<jvalue*>(args)).d);
      break;
    case 'F':
      values.AddFloat(isVaArg ? va_arg(*reinterpret_cast<va_list*>(args), double)
                              : (*reinterpret_cast<jvalue*>(args)).f);
      break;
    case 'L':
    case '[':
      values.AddReference(reinterpret_cast<MObject*>(isVaArg ? va_arg(*reinterpret_cast<va_list*>(args), jobject)
                                                             : (*reinterpret_cast<jvalue*>(args)).l));
      break;
    default:;
  }
}

template<calljavastubconst::ArgsType argsType>
ALWAYS_INLINE inline void MethodMeta::BuildJavaMethodArgJvalues(MObject *obj, BaseArgValue &values) const {
  uint32_t parameterCount = GetParameterCount();
  char retTypeNames[parameterCount];
  GetShortySignature(retTypeNames, parameterCount);
  if (!IsStatic()) {
    values.AddReference(obj);
  }
  if (values.GetMethodArgs() == 0) {
    return;
  }
  uintptr_t arg = values.GetMethodArgs();
  for (uint32_t i = 0; i < parameterCount; ++i) {
    char c = retTypeNames[i];
    if (argsType == calljavastubconst::kJvalue) {
      arg = reinterpret_cast<uintptr_t>((reinterpret_cast<jvalue*>(values.GetMethodArgs())) + i);
    }

    BuildArgstoJvalues<argsType>(c, values, arg);
  }
}

template<typename T, calljavastubconst::ArgsType argsType>
ALWAYS_INLINE inline T MethodMeta::InvokeJavaMethod(MObject *obj, const uintptr_t methodAargs,
                                                    uintptr_t calleeFuncAddr) const {
  ArgValue argValues(methodAargs);
  BuildJavaMethodArgJvalues<argsType>(obj, argValues);

  uintptr_t funcPtr = (calleeFuncAddr == 0) ? GetFuncAddress() : calleeFuncAddr;
  T result = RuntimeStub<T>::SlowCallCompiledMethod(funcPtr, argValues.GetData(),
                                                    argValues.GetStackSize(), argValues.GetFRegSize());
  return result;
}

template<typename T>
inline T MethodMeta::InvokeJavaMethodFast(MObject *obj) const {
  if (NeedsInterp()) {
    return interpreter::InterpJavaMethod<T, calljavastubconst::kJvalue>(const_cast<MethodMeta*>(this), obj, 0);
  } else {
    jvalue argJvalues[1];
    argJvalues[0].l = reinterpret_cast<jobject>(obj);
    uintptr_t funcPtr = GetFuncAddress();
    return RuntimeStub<T>::SlowCallCompiledMethod(funcPtr, argJvalues, 0, 0);
  }
}

template<typename RetType, calljavastubconst::ArgsType argsType, typename T>
inline RetType MethodMeta::Invoke(MObject *obj, const T methodAargs, uintptr_t calleeFuncAddr) const {
  static_assert(std::is_same<T, jvalue*>::value || std::is_same<T, const jvalue*>::value ||
      std::is_same<T, va_list*>::value, "wrong type");
  RetType result = NeedsInterp() ? interpreter::InterpJavaMethod<RetType, argsType>(const_cast<MethodMeta*>(this),
      obj, reinterpret_cast<const uintptr_t>(methodAargs)) :
      InvokeJavaMethod<RetType, argsType>(obj, reinterpret_cast<const uintptr_t>(methodAargs), calleeFuncAddr);
  return result;
}

inline void MethodMeta::GetShortySignature(char shorty[], const uint32_t size) const {
  GetShortySignature(GetSignature(), shorty, size);
}

inline void MethodMeta::GetShortySignature(const char srcSignature[], char shorty[], const uint32_t size) {
  if (size == 0) { // ()V
  } else if (size == 1) {
    // if size == 1, just need first signature char.
    shorty[0] = srcSignature[size]; // (I)V -> I
  } else {
    GetShortySignatureUtil(srcSignature, shorty, size);
  }
}

inline bool MethodMeta::GetParameterTypes(MClass *parameterTypes[], uint32_t size) const {
  if (size == 0) {
    return true;
  } else if (size == 1) {
    // first check Primitive Class
    MClass *c = MClass::GetPrimitiveClass(GetSignature() + 1); // (I)V
    if (c != nullptr) {
      parameterTypes[0] = c;
      return true;
    }
  }
  bool isEnableParametarType = IsEnableParametarType();
  if (isEnableParametarType) {
    MetaRef *pTypes = GetMethodSignature()->GetTypes();
    uint32_t index = 0;
    for (; index < size; ++index) {
      if (pTypes[index] != 0) {
        parameterTypes[index] = reinterpret_cast<MClass*>(pTypes[index]);
      } else {
        break;
      }
    }
    if (index == size) {
      return true;
    }
  }
  bool isSuccess = GetParameterTypesUtil(parameterTypes, size);
  if (isSuccess && isEnableParametarType) {
    MetaRef *pTypes = GetMethodSignature()->GetTypes();
    for (uint32_t i = 0; i < size; ++i) {
      pTypes[i] = static_cast<MetaRef>(parameterTypes[i]->AsUintptr());
    }
  }
  return isSuccess;
}

template<typename T>
inline MethodMeta *MethodMeta::JniCast(T methodMeta) {
  static_assert(std::is_same<T, jmethodID>::value || std::is_same<T, jobject>::value, "wrong type");
  return reinterpret_cast<MethodMeta*>(methodMeta);
}

template<typename T>
inline MethodMeta *MethodMeta::JniCastNonNull(T methodMeta) {
  DCHECK(methodMeta != 0);
  return JniCast(methodMeta);
}

template<typename T>
inline MethodMeta *MethodMeta::Cast(T methodMeta) {
  static_assert(std::is_same<T, void*>::value || std::is_same<T, address_t>::value ||
      std::is_same<T, uint64_t>::value, "wrong type");
  return reinterpret_cast<MethodMeta*>(methodMeta);
}

template<typename T>
inline MethodMeta *MethodMeta::CastNonNull(T methodMeta) {
  DCHECK(methodMeta != 0);
  return Cast(methodMeta);
}

inline jmethodID MethodMeta::AsJmethodID() {
  return reinterpret_cast<jmethodID>(this);
}
} // namespace maplert
#endif // MRT_MAPLERT_INCLUDE_METHODMETA_INLINE_H_
